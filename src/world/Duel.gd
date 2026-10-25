## Duel -- the online free-for-all: every pilot against every other, all
## in the same hull, in open space by Mars.  Kills are all that count:
## the first pilot to KILLS_TO_WIN, or the leader when the clock runs
## out, wins the round; after a short ceasefire everyone relaunches with
## a clean slate.  Computer pilots keep the arena busy until enough
## people join, and each joiner takes a computer pilot's place.  The host
## runs it; clients hold a mirror for the HUD (Net._rpc_duel).
class_name Duel
extends RefCounted

const ARENA := "Mars"              # the world the fight circles
const HULL := "Destroyer"          # everyone flies the same ship
const KILLS_TO_WIN := 10
const ROUND_SECONDS := 600.0
const BREAK_SECONDS := 10.0        # the ceasefire between rounds
const PILOTS := 4                  # computer pilots fill the arena up to this many
const BOT_LEVEL := 5               # their skill, as a rank
## Pilots launch on a ring this far from the arena's world, at the spot
## furthest from anyone alive.
const SPAWN_MIN := 700.0
const SPAWN_MAX := 1500.0

var world: Node
var elapsed := 0.0                 # into this round
var round_no := 1
var break_left := 0.0              # > 0: the ceasefire after a round
var winner_name := ""              # who took the last round
var killed_by := ""                # who last destroyed this machine's pilot


func setup(p_world: Node) -> void:
	world = p_world


func arena() -> Node2D:
	return world.planet_by_name.get(ARENA, world.planets[0])


func time_left() -> float:
	return maxf(ROUND_SECONDS - elapsed, 0.0)


func clock_text() -> String:
	var t := int(ceil(time_left()))
	return "%d:%02d" % [t / 60, t % 60]


## Everyone in the fight, best first: most kills, then fewest deaths.
func leaderboard() -> Array:
	var out := []
	for s in world.ships:
		if not s.is_station and not s.support and not s.is_prize:
			out.append(s)
	out.sort_custom(func(a, b):
		if a.kills != b.kills:
			return a.kills > b.kills
		if a.deaths != b.deaths:
			return a.deaths < b.deaths
		return a.pilot_name < b.pilot_name)
	return out


func bots() -> Array:
	return leaderboard().filter(func(s): return not s.is_player and not Net.seats.values().has(s.net_id))


# ------------------------------------------------------------- the host

## The host's own pilot and the computer pilots, at the start.
func spawn_all() -> void:
	var p: Ship = world._make_ship(Settings.pilot_name, Settings.race_id, HULL, Settings.pilot_name + "'s ship", true)
	var pc := PlayerController.new()
	pc.name = "Controller"
	p.add_child(pc)
	pc.setup(p, world)
	world.player = p
	place(p)
	while leaderboard().size() < PILOTS:
		add_bot()


## A computer pilot with a name from the roster nobody in the arena has.
func add_bot() -> Ship:
	var taken := {}
	for s in world.ships:
		taken[s.pilot_name] = true
	var pick: Array = GameData.ROBOTS[randi() % GameData.ROBOTS.size()]
	var rows: Array = GameData.ROBOTS.duplicate()
	rows.shuffle()
	for r in rows:
		if not taken.has(r[0]):
			pick = r
			break
	var race: int = clampi(int(pick[1]), 0, 2)
	var s: Ship = world._make_ship(pick[0], race, HULL, pick[3], false)
	s.rank = BOT_LEVEL
	var ai := AIPilot.new()
	ai.name = "AI"
	s.add_child(ai)
	ai.setup(s, world, arena(), BOT_LEVEL)
	ai.duel = true
	ai.ferry = false
	ai.bombard = false
	ai.patrol_point = ai._random_patrol_point()   # round the arena, now it knows
	place(s)
	return s


## A joiner's own ship, in their own empire's colours; a computer pilot
## makes room when the arena is full enough.  Net seats them in it.
func pilot_joined(pilot: String, race: int) -> Ship:
	var spare := bots()
	if not spare.is_empty() and leaderboard().size() >= PILOTS:
		# one that is out of the fight right now, when there is one
		spare.sort_custom(func(a, b): return a.dead and not b.dead)
		world.remove_ship(spare[0])
	var s: Ship = world._make_ship(pilot, clampi(race, 0, 2), HULL, pilot + "'s ship", false)
	place(s)
	return s


## A joiner left: their ship goes, and a computer pilot comes back if the
## arena is getting empty.
func pilot_left(s: Ship) -> void:
	world.remove_ship(s)
	while leaderboard().size() < PILOTS:
		Net.host_ship_added(add_bot())


## Launch point: the spot on the ring furthest from every live pilot,
## drifting with the arena's world and facing into the fight.
func place(s: Ship) -> void:
	var a: Node2D = arena()
	var best := Vector2.ZERO
	var best_d := -1.0
	for i in 16:
		var ang := TAU * i / 16.0 + randf_range(-0.15, 0.15)
		var at: Vector2 = a.global_position + Vector2(cos(ang), sin(ang)) * randf_range(SPAWN_MIN, SPAWN_MAX)
		var near := INF
		for o in world.ships:
			if o != s and not o.dead and not o.is_station:
				near = minf(near, at.distance_to(o.global_position))
		if near > best_d:
			best_d = near
			best = at
	s.global_position = best
	s.velocity = a.velocity
	s.rotation = (a.global_position - best).angle() + randf_range(-0.5, 0.5)
	s.reset_physics_interpolation()


## Every tick on the host.
func tick(delta: float) -> void:
	if break_left > 0.0:
		break_left -= delta
		for s in leaderboard():
			s.invulnerable = maxf(s.invulnerable, 0.5)   # ceasefire
		if break_left <= 0.0:
			_new_round()
		return
	elapsed += delta
	if elapsed >= ROUND_SECONDS:
		var board := leaderboard()
		_end_round(board[0] if not board.is_empty() and board[0].kills > 0 else null)


## A pilot is gone.  A kill counts for whoever made it, never in the
## ceasefire; a crash into the Sun or a rock counts for nobody.
func on_death(victim: Ship, killer: Ship) -> void:
	if killer != null and killer != victim and break_left <= 0.0:
		killer.kills += 1
		killer.score += 10
		world.notify_all("%s destroyed %s.  %s: %d kill%s" % [killer.pilot_name, victim.pilot_name, killer.pilot_name, killer.kills, "" if killer.kills == 1 else "s"],
			killer.race_color() if killer.is_player or victim.is_player else Color(0.85, 0.85, 0.85))
		if killer.kills >= KILLS_TO_WIN:
			_end_round(killer)
	else:
		world.notify_all("%s was lost." % victim.pilot_name, Color(0.75, 0.75, 0.75))
	Net.host_duel_changed()


func _end_round(winner: Ship) -> void:
	winner_name = winner.pilot_name if winner != null else ""
	break_left = BREAK_SECONDS
	if winner != null:
		world.notify_all("%s wins round %d with %d kills!  Next round in %d seconds." % [winner_name, round_no, winner.kills, int(BREAK_SECONDS)], winner.race_color())
	else:
		world.notify_all("Round %d is over: nobody scored.  Next round in %d seconds." % [round_no, int(BREAK_SECONDS)], Color(0.85, 0.9, 1.0))
	Sfx.play_ui("rankup", 1.0)
	Net.host_duel_changed()


## Clean slate: tallies back to zero and everyone relaunches.
func _new_round() -> void:
	round_no += 1
	elapsed = 0.0
	break_left = 0.0
	world.respawn_queue.clear()
	for s in leaderboard():
		s.kills = 0
		s.deaths = 0
		s.score = 0
		place(s)
		s.respawn(s.global_position, s.velocity)
		Net.host_ship_respawned(s)
		if s.is_player:
			world.player_respawned.emit()
	world.notify_all("Round %d: fight!  First to %d kills." % [round_no, KILLS_TO_WIN], Color(1.0, 0.9, 0.6))
	Net.host_duel_changed()


# -------------------------------------------------------- on the wire

func pack() -> Array:
	return [elapsed, round_no, break_left, winner_name]


func unpack(a: Array) -> void:
	if a.size() < 4:
		return
	elapsed = a[0]
	round_no = a[1]
	break_left = a[2]
	winner_name = a[3]


## A client's clock runs on between the host's updates.
func client_tick(delta: float) -> void:
	if break_left > 0.0:
		break_left = maxf(break_left - delta, 0.0)
	else:
		elapsed += delta
