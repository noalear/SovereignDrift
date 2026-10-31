## Skirmish -- team versus in a small arena: two empires, TEAM_SIZE
## pilots a side, every ship ready-built from its empire's stock.  Each
## launch the pilot picks a hull (bigger hulls take longer to launch).
## The first side to KILLS_TO_WIN, or the side ahead when the clock runs
## out, takes the round; after a short ceasefire everyone relaunches.
## Computer pilots fill the empty seats, and each joiner takes one's place
## on the side with fewer people (hosted co-op: on the host's side).  The host runs it; clients hold a mirror
## for the HUD (Net._rpc_mode).
class_name Skirmish
extends RefCounted

const ARENA := "Uranus"            # the world the fight circles: alone, no moons, far from the Sun
const TEAM_SIZE := 4
const KILLS_TO_WIN := 20
const ROUND_SECONDS := 600.0
const BREAK_SECONDS := 10.0        # the ceasefire between rounds
const HULLS := ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship"]
## Seconds from death to launch, by the hull picked: a Scout is back in
## the fight at once, a Battleship takes a while to crew.
const LAUNCH_DELAY := {"Scout": 3.0, "Destroyer": 4.0, "Cruiser": 6.0, "BattleCruiser": 8.0, "Battleship": 10.0}
const BOT_LEVEL := 6               # the computer pilots' skill, as a rank
## The two sides launch from opposite sides of the arena's world.
const SPAWN_MIN := 900.0
const SPAWN_MAX := 1400.0

var world: Node
var teams := [1, 2]                # the two empires: the host's first
var score := [0, 0]                # kills this round, per side
var elapsed := 0.0
var round_no := 1
var break_left := 0.0
var winner := -1                   # the side (0 or 1) that took the last round, -1 a draw
var side_angle := 0.0              # where side 0 launches; side 1 opposite
var killed_by := ""
var picks := {}                    # ship -> hull for its next launch (host; the local pick on a client)
var died_at := {}                  # ship -> World.elapsed when it was lost (host)


func setup(p_world: Node) -> void:
	world = p_world
	var mine := clampi(Settings.race_id, 0, 2)
	var rivals := [0, 1, 2].filter(func(r): return r != mine)
	teams = [mine, rivals[randi() % rivals.size()]]
	side_angle = randf() * TAU


func arena() -> Node2D:
	return world.planet_by_name.get(ARENA, world.planets[0])


func side_of(s: Ship) -> int:
	return 0 if s.race_id == teams[0] else 1


func team_of(s: Ship) -> int:
	return s.race_id


func time_left() -> float:
	return maxf(ROUND_SECONDS - elapsed, 0.0)


func clock_text() -> String:
	var t := int(ceil(time_left()))
	return "%d:%02d" % [t / 60, t % 60]


func side_name(i: int) -> String:
	return GameData.race(teams[i])["name"]


## Everyone in the fight, by side, best first.
func roster(side: int) -> Array:
	var out := []
	for s in world.ships:
		if not s.is_station and not s.support and not s.is_prize and s.race_id == teams[side]:
			out.append(s)
	out.sort_custom(func(a, b):
		if a.kills != b.kills:
			return a.kills > b.kills
		if a.deaths != b.deaths:
			return a.deaths < b.deaths
		return a.pilot_name < b.pilot_name)
	return out


func is_bot(s: Ship) -> bool:
	return not s.is_player and not Net.seats.values().has(s.net_id)


func humans(side: int) -> int:
	return roster(side).filter(func(s): return not is_bot(s)).size()


## How long the launch takes for the hull this pilot picked.
func launch_delay(s: Ship) -> float:
	return LAUNCH_DELAY.get(picks.get(s, s.design_name), 5.0)


## Two short lines for the message log when the mode starts.
func intro() -> Array:
	return ["Skirmish: the %s against the %s at %s, %d a side. First side to %d kills takes the round." % [side_name(0), side_name(1), ARENA, TEAM_SIZE, KILLS_TO_WIN],
		"Pick your ship while you wait to launch: 1 to 5, or click. Bigger hulls take longer to launch."]


# ------------------------------------------------------------- the host

func spawn_all() -> void:
	var p: Ship = world._make_ship(Settings.pilot_name, teams[0], "Destroyer", Settings.pilot_name + "'s ship", true)
	var pc := PlayerController.new()
	pc.name = "Controller"
	p.add_child(pc)
	pc.setup(p, world)
	world.player = p
	place(p)
	for side in [0, 1]:
		while roster(side).size() < TEAM_SIZE:
			add_bot(side)


## A computer pilot of that side's empire, with a name nobody here has.
func add_bot(side: int) -> Ship:
	var race: int = teams[side]
	var taken := {}
	for s in world.ships:
		taken[s.pilot_name] = true
	var pick: Array = []
	var rows: Array = GameData.ROBOTS.duplicate()
	rows.shuffle()
	for r in rows:
		if int(r[1]) == race and not taken.has(r[0]):
			pick = r
			break
	if pick.is_empty():
		pick = ["%s %d" % [GameData.race(race)["name"], world.ships.size()], race, BOT_LEVEL, "Wing"]
	var s: Ship = world._make_ship(pick[0], race, _bot_hull(), pick[3], false)
	s.rank = BOT_LEVEL
	var ai := AIPilot.new()
	ai.name = "AI"
	s.add_child(ai)
	ai.setup(s, world, arena(), BOT_LEVEL)
	ai.duel = true
	ai.ferry = false
	ai.bombard = false
	ai.raid = false
	ai.escort_duty = false
	ai.patrol_point = ai._random_patrol_point()
	place(s)
	return s


## Computer pilots mostly fly the middle hulls.
func _bot_hull() -> String:
	var roll := randf()
	return "Scout" if roll < 0.2 else ("Destroyer" if roll < 0.55 else ("Cruiser" if roll < 0.85 else ("BattleCruiser" if roll < 0.95 else "Battleship")))


## A joiner flies for the side with fewer people (competitive), or the
## host's side (co-op, against the computer), in a computer pilot's place
## when the side is full.  Null when there is no seat left for people.
func pilot_joined(pilot: String, versus := true) -> Ship:
	var side := 0 if humans(0) <= humans(1) else 1
	if not versus:
		side = 0
		if humans(0) >= TEAM_SIZE:
			return null
	elif humans(side) >= TEAM_SIZE:
		side = 1 - side
		if humans(side) >= TEAM_SIZE:
			return null
	var spare := roster(side).filter(is_bot)
	if not spare.is_empty() and roster(side).size() >= TEAM_SIZE:
		spare.sort_custom(func(a, b): return a.dead and not b.dead)
		world.remove_ship(spare[0])
	var s: Ship = world._make_ship(pilot, teams[side], "Destroyer", pilot + "'s ship", false)
	place(s)
	return s


func pilot_left(s: Ship) -> void:
	var side := side_of(s)
	picks.erase(s)
	died_at.erase(s)
	world.remove_ship(s)
	while roster(side).size() < TEAM_SIZE:
		Net.host_ship_added(add_bot(side))


## A pilot (the host's own, or a joiner by command) picks the hull for its
## next launch.
func choose(s: Ship, hull: String) -> void:
	if not hull in HULLS:
		return
	picks[s] = hull
	if s.dead and s.is_player:
		world.notify(s, "Next launch: %s." % hull)


## Launch point: the pilot's side of the arena, drifting with its world,
## facing the other side.  The hull picked is fitted here.
func place(s: Ship) -> void:
	var a: Node2D = arena()
	var hull: String = picks.get(s, "")
	if hull == "" and is_bot(s) and s.dead:
		hull = _bot_hull()
	if hull != "" and hull != s.design_name:
		s.refit(hull)
		Net.host_refit(s)
	var ang := side_angle + (0.0 if side_of(s) == 0 else PI) + randf_range(-0.5, 0.5)
	var at: Vector2 = a.global_position + Vector2(cos(ang), sin(ang)) * randf_range(SPAWN_MIN, SPAWN_MAX)
	s.global_position = at
	s.velocity = a.velocity
	s.rotation = ang + PI + randf_range(-0.3, 0.3)
	s.reset_physics_interpolation()


## The launch waits on the hull picked, which may change while waiting.
func may_relaunch(s: Ship) -> bool:
	return world.elapsed - float(died_at.get(s, -1e9)) >= launch_delay(s)


func tick(delta: float) -> void:
	if break_left > 0.0:
		break_left -= delta
		for s in roster(0) + roster(1):
			s.invulnerable = maxf(s.invulnerable, 0.5)   # ceasefire
		if break_left <= 0.0:
			_new_round()
		return
	elapsed += delta
	if elapsed >= ROUND_SECONDS:
		_end_round(0 if score[0] > score[1] else (1 if score[1] > score[0] else -1))


## A pilot is gone: the kill counts for the other side, never in the
## ceasefire.  A crash or a shot from a friend counts for nobody.
func on_death(victim: Ship, killer: Ship) -> void:
	died_at[victim] = world.elapsed
	if killer != null and killer != victim and killer.race_id != victim.race_id and break_left <= 0.0:
		killer.kills += 1
		killer.score += 10
		var side := side_of(killer)
		score[side] += 1
		world.notify_all("%s destroyed %s.   %s %d  -  %d %s" % [killer.pilot_name, victim.pilot_name, side_name(0), score[0], score[1], side_name(1)],
			killer.race_color() if killer.is_player or victim.is_player else Color(0.85, 0.85, 0.85))
		if score[side] >= KILLS_TO_WIN:
			_end_round(side)
	else:
		world.notify_all("%s was lost." % victim.pilot_name, Color(0.75, 0.75, 0.75))
	Net.host_mode_changed()


func _end_round(side: int) -> void:
	winner = side
	break_left = BREAK_SECONDS
	if side >= 0:
		world.notify_all("The %s take round %d, %d to %d!  Next round in %d seconds." % [side_name(side), round_no, score[side], score[1 - side], int(BREAK_SECONDS)], GameData.race(teams[side])["color"])
	else:
		world.notify_all("Round %d is a draw, %d all.  Next round in %d seconds." % [round_no, score[0], int(BREAK_SECONDS)], Color(0.85, 0.9, 1.0))
	Sfx.play_ui("rankup", 1.0)
	Net.host_mode_changed()


func _new_round() -> void:
	round_no += 1
	elapsed = 0.0
	break_left = 0.0
	score = [0, 0]
	side_angle = randf() * TAU
	world.respawn_queue.clear()
	died_at.clear()
	for s in roster(0) + roster(1):
		s.kills = 0
		s.deaths = 0
		s.score = 0
		place(s)
		s.respawn(s.global_position, s.velocity)
		Net.host_ship_respawned(s)
		if s.is_player:
			world.player_respawned.emit()
	world.notify_all("Round %d: fight!  First side to %d kills." % [round_no, KILLS_TO_WIN], Color(1.0, 0.9, 0.6))
	Net.host_mode_changed()


# -------------------------------------------------------- on the wire

func pack() -> Array:
	return [teams, score, elapsed, round_no, break_left, winner, side_angle]


func unpack(a: Array) -> void:
	if a.size() < 7:
		return
	teams = a[0]
	score = a[1]
	elapsed = a[2]
	round_no = a[3]
	break_left = a[4]
	winner = a[5]
	side_angle = a[6]


func client_tick(delta: float) -> void:
	if break_left > 0.0:
		break_left = maxf(break_left - delta, 0.0)
	else:
		elapsed += delta
