## Survival -- co-op defence: one to four pilots guard their empire's
## starbase against waves of raiders, each heavier than the last, with a
## boss every BOSS_EVERY waves.  Between waves there is a short break to
## dock at the starbase (U), mend and spend the bounties.  Pilots relaunch
## from the starbase while it stands; when it falls the run is over, and
## after a pause a new run starts from wave 1.  The host runs it; clients
## hold a mirror for the HUD (Net._rpc_mode).  Hosted competitive, the
## friends who join are hunters instead: they fly for a rival empire with
## the raiders, launch with each wave and wait out the breaks.
class_name Survival
extends RefCounted

const FIRST_BREAK := 20.0          # time to look round before wave 1
const BREAK_SECONDS := 30.0        # between waves: dock, mend, refit
const RESTART_SECONDS := 15.0      # after the starbase falls, then a new run
const BOSS_EVERY := 5              # every this many waves a big ship leads
const MAX_PLAYERS := 4
const MAX_WAVE_SHIPS := 16         # most raiders in one wave
const ENEMY_TEAM := 3              # every raider fights on one side, whatever its colours
## Raiders arrive on a ring this far from the starbase, bunched on one or
## two bearings so the defenders can see where the wave is coming from.
const ARRIVE_MIN := 3200.0
const ARRIVE_MAX := 4200.0
## Each wave the starbase gets this much of its hull back.
const BASE_MEND := 0.3
## The starbase is built heavier here than in Conquest: alone it holds the
## first four waves, and a boss wave needs the pilots there to stop it.
const BASE_TOUGHNESS := 2.5
const HUNTER_NOTE := "Competitive Survival: you hunt with the raiders. You launch with each wave; bring down the starbase to end the run."
const PIRATE_NAMES := ["Vex", "Grist", "Mordo", "Sable", "Kett", "Rook", "Harrow", "Jinx", "Skerry", "Tallow", "Brand", "Cutter"]

var world: Node
var race := 1                      # the defenders' empire
var wave := 0                      # the wave in play (or just fought)
var break_left := FIRST_BREAK      # > 0: the break before wave + 1
var enemies_left := 0
var over := false                  # the starbase fell: this run is done
var restart_left := 0.0
var run := 1
var best := 0                      # most waves held in a run on this machine's profile
var from_text := ""                # where the wave is coming from, for the HUD
var killed_by := ""                # who last destroyed this machine's pilot
var baseline := {}                 # pilot ship -> [score, kills] when the run began
var kills := {}                    # pilot ship -> raiders destroyed this run (host)


func setup(p_world: Node) -> void:
	world = p_world
	race = clampi(Settings.race_id, 0, 2)
	best = int(Settings.career.get("survival_best", 0))


func base() -> Ship:
	return world.starbase_of(race)


func home() -> Planet:
	return world._home_planet(race)


## Raiders are one side whatever their colours (a client only sees the
## colours: anyone not of the defenders' empire is a raider).
func team_of(s: Ship) -> int:
	return ENEMY_TEAM if s.has_meta("wave") or s.race_id != race else s.race_id


## Competitive: a friend flying with the raiders.  A client knows only
## its own ship is one (it is not of the defenders' empire).
func is_hunter(s: Ship) -> bool:
	return s.has_meta("hunter") or (not s.has_meta("wave") and s.race_id != race and (s.is_player or kills.has(s)))


func hunters() -> Array:
	return world.ships.filter(func(s): return is_hunter(s) and not s.is_station)


## The defenders, best first.
func pilots() -> Array:
	var out := []
	for s in world.ships:
		if not s.is_station and not s.support and not s.is_prize and team_of(s) == race:
			out.append(s)
	out.sort_custom(func(a, b): return run_kills(a) > run_kills(b) if run_kills(a) != run_kills(b) else a.pilot_name < b.pilot_name)
	return out


## The wave still in the fight (hunters do not count: the wave is beaten
## without them).
func raiders() -> Array:
	return world.ships.filter(func(s): return team_of(s) == ENEMY_TEAM and not s.dead and not s.is_station and not is_hunter(s))


func run_kills(s: Ship) -> int:
	if kills.has(s):
		return kills[s]
	return maxi(s.kills - int(baseline.get(s, [0, s.kills])[1]), 0)


func clock_text() -> String:
	var t := int(ceil(restart_left if over else break_left))
	return "%d:%02d" % [t / 60, t % 60]


## Waves beaten this run.
func held() -> int:
	return wave if break_left > 0.0 and not over else maxi(wave - 1, 0)


## Two short lines for the message log when the mode starts.
func intro() -> Array:
	if world.player != null and is_hunter(world.player):
		return [HUNTER_NOTE, "The %s starbase at %s holds out for as long as its pilots do. Every pilot you bring down counts." % [GameData.race(race)["name"], home().body_name]]
	return ["Survival: guard the %s starbase at %s against waves of raiders. A boss leads every %dth wave." % [GameData.race(race)["name"], home().body_name, BOSS_EVERY],
		"Between waves, dock (U) to mend and spend your bounties. Lose the starbase and the run is over."]


# ------------------------------------------------------------- the host

## The starbase and the host's own pilot.
func spawn_all() -> void:
	var h := home()
	var b: Ship = world._make_ship("%s Command" % GameData.race(race)["name"], race, "Starbase", GameData.race(race)["name"] + " Starbase", false)
	b.global_position = world._station_spot(h)
	b.velocity = world.space.circular_orbit_velocity(h.body, b.global_position)
	b.reset_physics_interpolation()
	var ai := AIPilot.new()
	b.add_child(ai)
	ai.setup(b, world, h, 14)
	_toughen(b)
	world._keep_station()
	world._spawn_player(race)
	world._place_at_home(world.player)
	_start_pilot(world.player)


func _toughen(b: Ship) -> void:
	b.hull_max *= BASE_TOUGHNESS
	b.hull = b.hull_max


## A pilot's run starts here: what they had when it began is kept aside,
## so the board shows only this run.
func _start_pilot(s: Ship) -> void:
	baseline[s] = [s.score, s.kills]
	kills[s] = 0


## A joiner flies for the defenders in a fresh ship at the starbase.  Null
## when four pilots are already in.  Competitive (versus), a hunter: a
## rival empire's ship that waits for the next wave to launch with it.
func pilot_joined(pilot: String, career_score: int, career_kills: int, versus := false) -> Ship:
	if pilots().size() + hunters().size() >= MAX_PLAYERS:
		return null
	var level := GameData.rank_for(career_score, career_kills)
	if versus:
		var empire: int = [0, 1, 2].filter(func(r): return r != race)[hunters().size() % 2]
		var h: Ship = world._make_ship(pilot, empire, world._design_for_rank(level, empire), pilot + "'s ship", false)
		h.set_meta("hunter", true)
		h.score = maxi(career_score, 0)
		h.kills = maxi(career_kills, 0)
		h.rank = level
		h.credits = World.START_CREDITS
		world._place_at_home(h)
		world._bench(h, 0.0)          # out until the next wave comes in
		_start_pilot(h)
		return h
	var s: Ship = world._make_ship(pilot, race, world._design_for_rank(level, race), pilot + "'s ship", false)
	s.score = maxi(career_score, 0)
	s.kills = maxi(career_kills, 0)
	s.rank = level
	s.credits = World.START_CREDITS
	world._place_at_home(s)
	if base() == null:
		world._bench(s, 1e9)          # the run is over: they launch with the next one
	_start_pilot(s)
	return s


func pilot_left(s: Ship) -> void:
	baseline.erase(s)
	kills.erase(s)
	world.remove_ship(s)


## Pilots relaunch at the starbase while it stands; raiders never do, and
## hunters only with the next wave (_launch_wave).
func may_relaunch(s: Ship) -> bool:
	return not over and not s.has_meta("wave") and not s.has_meta("hunter")


## Every tick on the host.
func tick(delta: float) -> void:
	if over:
		restart_left -= delta
		if restart_left <= 0.0:
			_new_run()
		return
	if break_left > 0.0:
		break_left -= delta
		if break_left <= 0.0:
			_launch_wave()
		return
	enemies_left = raiders().size()
	if enemies_left == 0:
		_wave_cleared()


## What wave `n` brings: hull names, the boss first.
static func roster(n: int, players: int) -> Array:
	var out := []
	if n % BOSS_EVERY == 0:
		out.append("Dreadnought" if n >= BOSS_EVERY * 3 else ("BattleCruiser" if n >= BOSS_EVERY * 2 else "Battleship"))
	var count := mini(2 + n + (players - 1) * (1 + n / 3), MAX_WAVE_SHIPS - out.size())
	for i in count:
		var roll := float(i) / maxf(count, 1)
		if n >= 9 and roll < 0.3:
			out.append("BattleCruiser" if n >= 12 and roll < 0.12 else "Cruiser")
		elif n >= 5 and roll < 0.45:
			out.append("Cruiser" if roll < 0.2 else "Destroyer")
		elif n >= 3 and roll < 0.6:
			out.append("Destroyer")
		else:
			out.append("Scout")
	return out


## How good the raiders of wave `n` fly, as a rank.
static func skill(n: int) -> int:
	return clampi(n, 1, 13)


func _launch_wave() -> void:
	wave += 1
	var b := base()
	if b == null:
		return
	var rivals := [0, 1, 2].filter(func(r): return r != race)
	var empire: int = rivals[randi() % rivals.size()]
	var bearing := randf() * TAU
	var second := bearing + randf_range(1.2, 2.4) * (1.0 if randf() < 0.5 else -1.0)
	var hulls := roster(wave, pilots().size())
	var names := _names(empire)
	for i in hulls.size():
		var hull: String = hulls[i]
		# pirates bring the light hulls, the rival empire the heavy ones
		var pirate: bool = hull in ["Scout", "Destroyer"] and randf() < 0.4
		var r := 3 if pirate else empire
		if pirate and hull == "Destroyer":
			hull = "Raider"
		var pilot: String = ("Pirate " + PIRATE_NAMES[randi() % PIRATE_NAMES.size()]) if pirate else names[i % names.size()]
		var s: Ship = world._make_ship(pilot, r, hull, "%s raider" % GameData.race(r)["name"], false)
		s.set_meta("wave", wave)
		s.rank = skill(wave) + (2 if i == 0 and wave % BOSS_EVERY == 0 else 0)
		var ai := AIPilot.new()
		ai.name = "AI"
		s.add_child(ai)
		ai.setup(s, world, home(), s.rank)
		ai.duel = true                 # hunt round the starbase, never go home to mend
		ai.ferry = false
		ai.bombard = false
		ai.raid = false
		ai.patrol_point = b.global_position + Vector2(randf_range(-900, 900), randf_range(-900, 900))
		var ang: float = (bearing if i % 3 != 2 else second) + randf_range(-0.25, 0.25)
		s.global_position = b.global_position + Vector2(cos(ang), sin(ang)) * randf_range(ARRIVE_MIN, ARRIVE_MAX)
		s.velocity = b.velocity
		s.rotation = (b.global_position - s.global_position).angle()
		s.reset_physics_interpolation()
		s.target = b
		Net.host_ship_added(s)
	_launch_hunters(b, bearing)
	enemies_left = hulls.size()
	from_text = _bearing_name(bearing)
	var boss := ("  A %s leads them." % hulls[0]) if wave % BOSS_EVERY == 0 else ""
	world.notify_all("Wave %d: %d raiders coming in from the %s.%s" % [wave, hulls.size(), from_text, boss], Color(1.0, 0.7, 0.45))
	Sfx.play_ui("alarm", 0.6)
	Net.host_mode_changed()


## The hunters waiting out the break come in with the wave, on its bearing.
func _launch_hunters(b: Ship, bearing: float) -> void:
	for s in hunters():
		if not s.dead:
			continue
		world.respawn_queue = world.respawn_queue.filter(func(e): return e[0] != s)
		var ang := bearing + randf_range(-0.3, 0.3)
		var at: Vector2 = b.global_position + Vector2(cos(ang), sin(ang)) * randf_range(ARRIVE_MIN, ARRIVE_MAX)
		s.respawn(at, b.velocity)
		s.rotation = (b.global_position - at).angle()
		s.reset_physics_interpolation()
		Net.host_ship_respawned(s)


func _names(empire: int) -> Array:
	var out := []
	for r in GameData.ROBOTS:
		if int(r[1]) == empire:
			out.append(r[0])
	out.shuffle()
	return out if not out.is_empty() else ["Raider"]


## "north-east": where on screen the wave comes from, seen from the starbase.
static func _bearing_name(a: float) -> String:
	var names := ["east", "south-east", "south", "south-west", "west", "north-west", "north", "north-east"]
	return names[int(round(fposmod(a, TAU) / (TAU / 8.0))) % 8]


func _wave_cleared() -> void:
	break_left = BREAK_SECONDS
	var b := base()
	if b != null:
		b.hull = minf(b.hull + b.hull_max * BASE_MEND, b.hull_max)
		b.shield = b.shield_max
	# what is left of the wreckage stays to be scooped up, but no raider
	# relaunches: clear them out of the world
	for s in world.ships.duplicate():
		if s.has_meta("wave"):
			world.remove_ship(s)
	var bonus := 50 + wave * 25
	for s in pilots():
		s.earn(bonus)
	world.notify_all("Wave %d beaten!  %d credits each.  Next wave in %d seconds: dock at the starbase (U) to mend and refit." % [wave, bonus, int(BREAK_SECONDS)], Color(0.7, 1.0, 0.7))
	Sfx.play_ui("rankup", 0.8)
	_note_best(wave)
	Net.host_mode_changed()


## A ship is gone.  The starbase going ends the run; a raider counts for
## whoever brought it down.  (World pays the bounty and drops the loot.)
func on_death(victim: Ship, killer: Ship) -> void:
	if victim.has_meta("wave") or victim.has_meta("hunter"):
		if killer != null and not killer.has_meta("wave") and not killer.has_meta("hunter") and kills.has(killer):
			kills[killer] += 1
		enemies_left = raiders().size()
	elif killer != null and killer.has_meta("hunter") and kills.has(killer) and (team_of(victim) == race):
		kills[killer] += 1             # a hunter's tally: the defenders and their starbase
		if victim.is_starbase() and victim.race_id == race and not over:
			_run_over()
	elif victim.is_starbase() and victim.race_id == race and not over:
		_run_over()
	Net.host_mode_changed()


func _run_over() -> void:
	over = true
	restart_left = RESTART_SECONDS
	var held_waves := maxi(wave - 1, 0)
	_note_best(held_waves)
	world.notify_all("The starbase has fallen after %d wave%s.  A new run starts in %d seconds." % [held_waves, "" if held_waves == 1 else "s", int(RESTART_SECONDS)], Color(1.0, 0.55, 0.45))
	Sfx.play_ui("alarm", 0.8)


## The best run so far, on this machine's profile (off the rank ladder,
## like duels).  Clients note theirs from the mirror.
func _note_best(n: int) -> void:
	best = maxi(best, int(Settings.career.get("survival_best", 0)))
	if n > best:
		best = n
		Settings.career["survival_best"] = n
		Settings.save_settings()


## A clean slate: raiders gone, the starbase back at full strength and
## every pilot relaunched in the hull their rank gives them.
func _new_run() -> void:
	run += 1
	wave = 0
	over = false
	break_left = FIRST_BREAK
	enemies_left = 0
	from_text = ""
	# hunters stay benched (or are benched now) until wave 1 brings them in
	world.respawn_queue = world.respawn_queue.filter(func(e): return is_instance_valid(e[0]) and e[0].has_meta("hunter"))
	for s in world.ships.duplicate():
		if s.has_meta("wave"):
			world.remove_ship(s)
	for h in hunters():
		var old: Array = baseline.get(h, [h.score, h.kills])
		h.score = old[0]
		h.kills = old[1]
		_start_pilot(h)
		if not h.dead:
			world._bench(h, 0.0)
			Net.host_ship_died(h, null)
	var b: Ship = null
	for s in world.ships:
		if s.is_starbase() and s.race_id == race:
			b = s
	if b != null:
		var at: Vector2 = world._station_spot(home())
		b.respawn(at, world.space.circular_orbit_velocity(home().body, at))
		_toughen(b)
		Net.host_ship_respawned(b)
	for s in pilots():
		var old: Array = baseline.get(s, [s.score, s.kills])
		s.score = old[0]
		s.kills = old[1]
		s.rank = GameData.rank_for(s.score, s.kills)
		s.credits = World.START_CREDITS
		s.hold.clear()
		s.refit(world._design_for_rank(s.rank, race))
		Net.host_refit(s)
		_start_pilot(s)
		world._place_at_home(s)
		s.respawn(s.global_position, s.velocity)
		Net.host_ship_respawned(s)
		if s.is_player:
			world.player_respawned.emit()
	world.notify_all("Run %d: the starbase is back.  Wave 1 in %d seconds." % [run, int(FIRST_BREAK)], Color(1.0, 0.9, 0.6))
	Net.host_mode_changed()


# -------------------------------------------------------- on the wire

func pack() -> Array:
	var board := []
	for s in pilots() + hunters():
		board.append([s.net_id, run_kills(s)])
	return [race, wave, break_left, enemies_left, over, restart_left, run, from_text, board]


func unpack(a: Array) -> void:
	if a.size() < 9:
		return
	race = a[0]
	wave = a[1]
	break_left = a[2]
	enemies_left = a[3]
	var was_over := over
	over = a[4]
	restart_left = a[5]
	run = a[6]
	from_text = a[7]
	kills.clear()
	for row in a[8]:
		var s: Node = Net.ships_by_id.get(int(row[0]), null)
		if s != null:
			kills[s] = int(row[1])
	if over and not was_over:
		_note_best(maxi(wave - 1, 0))
	elif not over and break_left > 0.0 and wave > 0:
		_note_best(wave)


## A client's clocks run on between the host's updates.
func client_tick(delta: float) -> void:
	if over:
		restart_left = maxf(restart_left - delta, 0.0)
	elif break_left > 0.0:
		break_left = maxf(break_left - delta, 0.0)
