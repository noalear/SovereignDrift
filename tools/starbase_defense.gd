# Starbase defence bench: how well a home starbase holds off an attack on
# its own (Kyle, 2026-10-31: "My starbase should have its own turrets for
# defense and it should be fairly powerful").  Every other ship is taken
# out of the world; then --count enemy --hull ships come at the Terran
# starbase from --from px (spread round it) with their computer pilots
# told to attack it.  The report gives when each attacker went down, what
# the starbase lost, and how many shots each of its guns fired.
#   godot --headless --path . -s tools/starbase_defense.gd -- --hull=Cruiser --count=3
# --race=2 picks the attackers' empire (default the Drengin); --cap=S ends
# the run after S seconds (default 120).
extends SceneTree

var main: Node
var w
var hull := "Cruiser"
var count := 3
var race := 2
var from := 1400.0
var cap := 120.0


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--hull="):
			hull = arg.get_slice("=", 1)
		elif arg.begins_with("--count="):
			count = int(arg.get_slice("=", 1))
		elif arg.begins_with("--race="):
			race = int(arg.get_slice("=", 1))
		elif arg.begins_with("--from="):
			from = float(arg.get_slice("=", 1))
		elif arg.begins_with("--cap="):
			cap = float(arg.get_slice("=", 1))
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_game()
	await process_frame
	await process_frame
	w = main.world
	var p: Ship = w.player
	var base: Ship = w.starbase_of(1)
	# nobody else in the fight: the player parked far off and out of harm
	for s in w.ships.duplicate():
		if s != p and not s.is_starbase():
			w.remove_ship(s)
	p.invulnerable = 1e9
	var attackers: Array = []
	for i in count:
		var a := TAU * i / count + 0.3
		var at: Vector2 = base.global_position + Vector2(cos(a), sin(a)) * from
		var s: Ship = w.spawn_ship("Raider %d" % (i + 1), race, hull, "Raider", at, base.velocity, 6)
		s.rotation = (base.global_position - at).angle()
		s.target = base
		var ai: AIPilot = s.get_node("AI")
		ai.state = AIPilot.State.ATTACK
		ai.aggression = 1.0
		attackers.append(s)
	var shots := {}
	var hull0: float = base.hull
	var shield0: float = base.shield
	var down := {}
	var t := 0.0
	var energy_low := INF
	var last_cd := {}
	while t < cap:
		await physics_frame
		t += 1.0 / 60.0
		p.global_position = base.global_position + Vector2(30000, 0)
		p.body.pos = p.global_position
		p.velocity = base.velocity
		p.body.vel = base.velocity
		for s in w.ships:
			if not s.is_starbase() and s != p and not attackers.has(s) and not s.dead:
				w.remove_ship(s)
				break
		if base.dead:
			break
		energy_low = minf(energy_low, base.energy)
		for wpn in base.weapons:
			var k := "%d %s" % [wpn["index"], wpn["name"]]
			var cd: float = wpn["cooldown"]
			if wpn["kind"] == "beam":
				if not base.beam_targets.is_empty():
					shots[k] = shots.get(k, 0.0) + 1.0 / 60.0
			elif cd > float(last_cd.get(k, 0.0)) + 0.001:
				shots[k] = shots.get(k, 0) + 1
			last_cd[k] = cd
		for s in attackers:
			if (not is_instance_valid(s) or s.dead) and not down.has(s):
				down[s] = t
		if down.size() == attackers.size():
			break
	print("\n%d %s %s ship(s) against the %s starbase from %d px" % [count, root.get_node("/root/GameData").race(race)["adj"], hull, root.get_node("/root/GameData").race(1)["adj"], int(from)])
	for s in attackers:
		print("  %-10s %s" % [s.pilot_name if is_instance_valid(s) else "?", ("down at %.1f s" % down[s]) if down.has(s) else "still flying at %.0f s" % t])
	print("  starbase: %s, hull %d -> %d, shield %d -> %d, battery low %d of %d" % ["DESTROYED" if base.dead else "standing", hull0, base.hull, shield0, base.shield, energy_low, base.energy_max])
	for k in shots:
		print("  gun %-22s %s" % [k, ("%.1f s firing" % shots[k]) if shots[k] is float else "%d shots" % shots[k]])
	quit(0)
