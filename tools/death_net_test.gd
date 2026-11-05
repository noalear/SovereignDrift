# Death penalty, wreck salvage, starbase guns and the starbase launch rule
# online (Kyle, 2026-10-31): two headless processes, one hosting co-op and
# one joining.  Usage (start the joiner a few seconds after the host):
#   godot --headless --path . -s tools/death_net_test.gd -- --host
#   godot --headless --path . -s tools/death_net_test.gd -- --join=127.0.0.1
# The host shoots the joiner down with 1000 credits aboard (half go with
# the wreck, with one part), flies its ship back through the credit chip,
# parks an enemy by the starbase for its guns, then takes the starbase
# down and shoots the joiner again: no launch until it is rebuilt.  The
# joiner checks its purse, its death screen note, the starbase turrets
# swinging on its screen and the wait.  Exit code 1 when a check fails.
extends SceneTree

var main: Node
var failed := false
var args := OS.get_cmdline_user_args()
var is_host := "--host" in args
var world_seen_at := -1
var joiner_seen_at := -1
var step := 0
var spot := Vector2.ZERO
var chip: Node = null
var decoy: Ship = null
# joiner
var deaths := 0
var was_dead := false
var died_at := -1
var purse_ok := false
var swung := false
var waited_from := -1
var waited_long := false
var quit_at := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	var settings: Node = root.get_node("/root/Settings")
	for profile in ["single", "online"]:
		settings.use_profile(profile)
		settings.pilot_name = "HostPilot" if is_host else "Joiner"
		settings.race_id = 1
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host / --join itself


func _find(w, pilot: String) -> Ship:
	for s in w.ships:
		if s.pilot_name == pilot:
			return s
	return null


func _physics_process(_delta: float) -> bool:
	var w = main.world if main != null else null
	var now := Time.get_ticks_msec()
	if w != null and world_seen_at < 0:
		world_seen_at = now
	if is_host:
		_host(w, now)
	elif w != null and w.player != null:
		_joiner(w, now)
	var done: bool = (step >= 99) or (world_seen_at > 0 and now - world_seen_at > 120000) if is_host \
		else ((quit_at > 0 and now > quit_at) or (world_seen_at > 0 and now - world_seen_at > 110000))
	if done:
		if is_host:
			_check(joiner_seen_at > 0, "the joiner took a seat")
			_check(step >= 99, "the host ran every step (stopped at %d)" % step)
		else:
			_check(deaths >= 2, "the joiner was shot down twice (%d)" % deaths)
			_check(purse_ok, "the joiner's purse came back halved, and its death screen said so")
			_check(swung, "the starbase turrets swing onto an enemy on the joiner's screen")
			_check(waited_long, "the joiner waits while its starbase is down")
		print("[%s] %s" % [_tag(), "FAILED" if failed else "passed"])
		quit(1 if failed else 0)
	return false


func _host(w, now: int) -> void:
	if w == null:
		return
	var j := _find(w, "Joiner")
	if j == null:
		return
	if joiner_seen_at < 0:
		joiner_seen_at = now
	var t := now - joiner_seen_at
	var base: Ship = w.starbase_of(j.race_id)
	if decoy != null and is_instance_valid(decoy) and not decoy.dead and base != null:
		# an enemy the starbase guns can chew on all through the test
		decoy.hull = decoy.hull_max
		decoy.global_position = base.global_position + Vector2(650, 0)
		decoy.body.pos = decoy.global_position
		decoy.velocity = base.velocity
		decoy.body.vel = base.velocity
	match step:
		0:
			if t > 3000:
				j.credits = 1000     # the joiner hears of it with the next purse update
				step = 5
		5:
			if t > 8000 and not j.dead:
				spot = w._home_planet(j.race_id).global_position + Vector2(0, 7000)
				_park(j, spot)
				j.credits = 1000
				j.invulnerable = 0.0
				var before: Array = w.powerups.duplicate()
				j.take_damage(1e9, null)
				var parts := 0
				for q in w.powerups:
					if not before.has(q) and is_instance_valid(q):
						if q.kind == "credits":
							chip = q
						elif q.kind == "part":
							parts += 1
				_check(j.dead and j.credits == 500, "the joiner's ship is shot down and half its purse is gone (%d)" % j.credits)
				_check(chip != null and chip.amount >= 500, "the wreck spills the lost half (%d)" % (chip.amount if chip != null else 0))
				_check(parts == 1, "and one part (%d)" % parts)
				decoy = w.spawn_ship("Decoy", 2, "Dreadnought", "Decoy", base.global_position + Vector2(650, 0), base.velocity, -1)
				root.get_node("/root/Net").host_ship_added(decoy)   # the joiner sees it too
				step = 1
		1:
			if not j.dead and chip != null and is_instance_valid(chip):
				var was := j.credits
				_park(j, chip.global_position)
				step = 2
				set_meta("was", was)
		2:
			if chip == null or not is_instance_valid(chip):
				_check(j.credits >= int(get_meta("was")) + 500, "a pilot online scoops the credit chip up (%d -> %d)" % [int(get_meta("was")), j.credits])
				step = 3
				set_meta("t3", t)
			else:
				_park(j, chip.global_position)
		3:
			if t - int(get_meta("t3")) > 8000:
				# home goes down, then the joiner: no launch until the rebuild
				base.take_damage(1e9, null)
				_park(j, spot)
				j.invulnerable = 0.0
				j.take_damage(1e9, null)
				set_meta("t4", t)
				step = 4
		4:
			if t - int(get_meta("t4")) > 15000:
				_check(j.dead and w.waits_for_starbase(j), "15 s on, the joiner still waits for the starbase")
				step = 99


func _joiner(w, now: int) -> void:
	var p: Ship = w.player
	if p.dead and not was_dead:
		deaths += 1
		died_at = now
	was_dead = p.dead
	if deaths == 1 and p.dead and not purse_ok and int(p.get_meta("purse_lost", 0)) == 500 and p.credits == 500:
		purse_ok = true
		print("[client] ok  : purse halved (500) and the death screen has the note")
	if OS.has_environment("SF_DEBUG") and Engine.get_physics_frames() % 120 == 0:
		for s in w.ships:
			if s.is_starbase() and s.race_id == p.race_id:
				print("[client] dbg starbase puppet=%s dead=%s guns=%d marks=%s decoy=%s" % [s.is_puppet, s.dead, s.weapons.size(), s.weapons.map(func(x): return x.get("mark")), _find(w, "Decoy")])
	for s in w.ships:
		if s.is_starbase() and s.race_id == p.race_id and not s.dead:
			for wpn in s.weapons:
				if wpn["id"] == "4010" and absf(angle_difference(float(wpn.get("aim_vis", wpn["fire_angle"])), wpn["fire_angle"])) > 0.3:
					swung = true
	if deaths >= 2 and p.dead and w.waits_for_starbase(p):
		if waited_from < 0:
			waited_from = now
		elif now - waited_from > 10000 and not waited_long:
			waited_long = true
			quit_at = now + 1000


func _park(s: Ship, at: Vector2) -> void:
	s.global_position = at
	s.velocity = Vector2.ZERO
	s.autopilot = false
	if s.body != null:
		s.body.pos = at
		s.body.vel = Vector2.ZERO
	s.reset_physics_interpolation()


func _tag() -> String:
	return "host" if is_host else "client"


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
