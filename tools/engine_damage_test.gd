# Engine damage test (Kyle, 2026-10-29).  In free flight, out in open
# space, checks that:
#  - damaged engines turn, push and brake the ship less than whole ones;
#  - with the engines out, nothing the pilot does (thrust, brake, turn,
#    strafe, flight assist) changes the ship's motion: it drifts and spins
#    exactly as it was going when they failed;
#  - the same holds for a computer pilot;
#  - the helm comes back when damage control restarts the engines.
#   godot --headless --path . -s tools/engine_damage_test.gd
extends SceneTree

const OPEN_SPACE := Vector2(30000, 30000)

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	for i in 30:
		await physics_frame
	await _run(main.world)
	print("engine_damage_test: %s" % ("FAILED" if failed else "all checks passed"))
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	print(("  ok    " if ok else "  FAIL  ") + what)
	if not ok:
		failed = true


func _frames(n: int) -> void:
	for i in n:
		await physics_frame


## Put a ship still (or moving and spinning as asked) out in open space.
func _place(s: Ship, pos: Vector2, vel := Vector2.ZERO, spin := 0.0) -> void:
	s.global_position = pos
	s.velocity = vel
	s.rotation = 0.0
	s.body.pos = pos
	s.body.vel = vel
	s.body.angle = 0.0
	s.body.ang_vel = spin
	s.angular_velocity = spin
	s.fuel = s.fuel_max
	s.reset_physics_interpolation()


func _inputs(s: Ship, thrust := 0.0, turn := 0.0, brake := false, strafe := 0.0) -> void:
	s.inputs["thrust"] = thrust
	s.inputs["turn"] = turn
	s.inputs["brake"] = brake
	s.inputs["strafe"] = strafe
	s.inputs["boost"] = false


## Set the engines' health (0 = shot out) and hold off damage control.
func _engines(s: Ship, health: float) -> void:
	var e: Dictionary = s.systems.get_system("engines")
	e["hp"] = e["max"] * health
	e["offline"] = health <= 0.0
	s.systems.quiet = -1000.0
	s.apply_systems()


## Spin rate after a second of full turn, speed after half a second of
## full thrust, and speed shed by half a second of braking from 300.
func _helm(s: Ship) -> Dictionary:
	var out := {}
	_place(s, OPEN_SPACE)
	_inputs(s, 0.0, 1.0)
	await _frames(60)
	out["turn"] = absf(s.body.ang_vel)
	_place(s, OPEN_SPACE)
	_inputs(s, 1.0)
	await _frames(30)
	out["push"] = s.velocity.length()
	_place(s, OPEN_SPACE, Vector2(300, 0))
	_inputs(s, 0.0, 0.0, true)
	await _frames(30)
	out["brake"] = 300.0 - s.velocity.length()
	_inputs(s)
	return out


func _run(w) -> void:
	var p: Ship = w.player
	p.get_node("Controller").set_physics_process(false)
	p.get_node("Controller").set_process(false)
	p.flight_assist = true
	_check(not p.systems.get_system("engines").is_empty(), "the player's %s has engines" % p.design_name)

	var whole: Dictionary = await _helm(p)
	_engines(p, 0.2)
	var hurt: Dictionary = await _helm(p)
	print("  whole: turn %.2f rad/s, push %.0f px/s, brake %.0f px/s;  engines at 20%%: turn %.2f, push %.0f, brake %.0f" % [
		whole["turn"], whole["push"], whole["brake"], hurt["turn"], hurt["push"], hurt["brake"]])
	_check(whole["turn"] > 0.5 and hurt["turn"] < whole["turn"] * 0.7, "damaged engines turn the ship slower")
	_check(whole["push"] > 5.0 and hurt["push"] < whole["push"] * 0.7, "damaged engines push less")
	_check(whole["brake"] > 5.0 and hurt["brake"] < whole["brake"] * 0.8, "damaged engines brake less")

	# shot out while moving and turning: the pilot leans on every control
	_engines(p, 0.0)
	_check(p.engines_out() and p.is_disabled(), "engines at zero are offline and the ship is disabled")
	var vel := Vector2(150, 40)
	var spin := 1.2
	_place(p, OPEN_SPACE, vel, spin)
	_inputs(p, 1.0, -1.0, true, 1.0)
	await _frames(180)
	_check(p.velocity.distance_to(vel) < 1.0, "engines out: thrust, brake and strafe change nothing (velocity %s, was %s)" % [p.velocity, vel])
	_check(absf(p.body.ang_vel - spin) < 0.01, "engines out: turning and flight assist do not stop the spin (%.3f rad/s, was %.3f)" % [p.body.ang_vel, spin])
	_check(p.global_position.distance_to(OPEN_SPACE + vel * 3.0) < 5.0, "engines out: the ship drifts on its course")
	_check(p.engine_level == 0.0, "engines out: no engine glow")
	var world_below: Planet = null
	for pl in w.planets:
		if pl.has_surface():
			world_below = pl
			break
	if world_below != null:
		_check(p.landing_refusal(world_below).begins_with("Engines out"), "engines out: no landing (%s)" % p.landing_refusal(world_below))

	# damage control brings them back and the helm answers again
	p.systems.repair_all()
	p.apply_systems()
	_inputs(p)
	var back: Dictionary = await _helm(p)
	_check(absf(back["turn"] - whole["turn"]) < 0.05 and absf(back["push"] - whole["push"]) < 1.0, "repaired engines fly like new")

	# a computer pilot gets no helm either
	var ai: Ship = w.spawn_ship("Drifter", 3, "Destroyer", "Drifter", OPEN_SPACE + Vector2(3000, 0), Vector2.ZERO, 3)
	await _frames(10)
	_engines(ai, 0.0)
	_place(ai, OPEN_SPACE + Vector2(3000, 0), Vector2(-60, 90), -0.8)
	ai.target = p
	await _frames(180)
	_check(ai.velocity.distance_to(Vector2(-60, 90)) < 1.0 and absf(ai.body.ang_vel + 0.8) < 0.01,
		"engines out: a computer pilot drifts and spins too (velocity %s, spin %.3f)" % [ai.velocity, ai.body.ang_vel])
