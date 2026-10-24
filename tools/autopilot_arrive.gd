# Autopilot arrival test: fly to planets that have moons, starting on the
# side where a moon sits across the approach, and to Mars from Earth.  The
# autopilot must finish with "Arrived" on its own, not circle the moons.
#   godot --headless --path . -s tools/autopilot_arrive.gd
extends SceneTree

const TRIAL_SECONDS := 90.0
## [target, "via" body the approach crosses, start distance], or
## [target, "from:Planet", 0] for a trip that starts just above that planet.
const TRIALS := [["Mars", "from:Earth", 0.0], ["Earth", "from:Mars", 0.0], ["Mars", "Phobos", 4000.0],
	["Mars", "Deimos", 4000.0], ["Earth", "Luna", 4000.0],
	["Jupiter", "Io", 5000.0], ["Saturn", "Titan", 5000.0]]

var main: Node
var trial := -1
var t0 := 0.0
var failed := false
var notices := {}
var bumps := 0


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
	await process_frame
	await process_frame
	_next_trial()


func _now() -> float:
	return Engine.get_physics_frames() / 60.0   # game time, however fast frames run


func _next_trial() -> void:
	trial += 1
	var only := OS.get_environment("AP_TRIAL")
	var upto := OS.get_environment("AP_UPTO")
	if upto != "" and trial > int(upto):
		trial = TRIALS.size()
	while only != "" and trial < TRIALS.size() and str(trial) != only:
		trial += 1
	if trial >= TRIALS.size():
		print("autopilot arrival: ", "FAILED" if failed else "passed")
		quit(1 if failed else 0)
		return
	var w = main.world
	var p: Ship = w.player
	var target: Planet = w.planet_by_name.get(TRIALS[trial][0], null)
	if target == null:
		print("skip trial %d: no %s" % [trial, TRIALS[trial][0]])
		_next_trial()
		return
	var via: String = TRIALS[trial][1]
	if via.begins_with("from:"):
		var home: Planet = w.planet_by_name[via.substr(5)]
		p.global_position = home.global_position + Vector2(home.radius + 400.0, 0)
		p.velocity = home.velocity
	else:
		var moon: Planet = w.planet_by_name[via]
		var out: Vector2 = (moon.global_position - target.global_position).normalized()
		p.global_position = target.global_position + out * TRIALS[trial][2]
		p.velocity = target.velocity
	p.rotation = (target.global_position - p.global_position).angle()
	p.hull = p.hull_max
	p.energy = p.energy_max
	p.fuel = p.fuel_max
	_repair(p)
	bumps = 0
	p.reset_physics_interpolation()
	if p.body != null:
		p.body.pos = p.global_position
		p.body.vel = p.velocity
	p.nav_target = target
	p.autopilot = true
	if p.has_meta("detour_around"):
		p.remove_meta("detour_around")
	notices.clear()
	t0 = _now()
	print("trial %d: to %s%s" % [trial, target.body_name, (" " + via.replace(":", " ")) if via.begins_with("from:") else (" past " + via)])


## Hull and subsystems back to new: a knock that takes the engines out
## would otherwise leave the ship drifting, and this test is about routes.
func _repair(p: Ship) -> void:
	if "systems" in p and p.systems != null:
		p.systems.repair_all()


func _process(_d: float) -> bool:
	if trial < 0 or trial >= TRIALS.size() or main.world == null:
		return false
	var w = main.world
	var p: Ship = w.player
	var target: Planet = p.nav_target
	if p.hull < p.hull_max:
		bumps += 1              # it touched something on the way
	p.hull = p.hull_max
	_repair(p)
	if p.has_meta("detour_around"):
		var n: String = p.get_meta("detour_around").body_name
		notices[n] = int(notices.get(n, 0)) + 1
	var gap: float = p.global_position.distance_to(target.global_position) - target.radius
	var took := _now() - t0
	if OS.get_environment("AP_DEBUG") != "" and Engine.get_physics_frames() % 120 == 0:
		var r: Dictionary = p._route(target.global_position - p.global_position)
		print("    t=%.0f gap=%d v=%d relv=%d warp=%s around=%s leg=%d" % [took, int(gap), int(p.velocity.length()), int((p.velocity - target.velocity).length()), p.warping,
			r["around"].body_name if r.get("around") != null else "-", int((r["to"] as Vector2).length())])
		print("      fuel=%d thrust=%s brake=%s turn=%.2f accel=%.1f landing=%s fa=%s rot=%.2f" % [int(p.fuel), p.inputs["thrust"], p.inputs["brake"], p.inputs["turn"], p.accel, str(p.get("landing")), p.flight_assist, p.rotation])
	if not p.autopilot or p.dead:
		var ok: bool = not p.dead and gap < target.radius * 0.6 + 320.0
		print("  %s: %s after %.1f s, gap %d, detours %s" % ["ok  " if ok else "FAIL", "arrived" if ok else ("died" if p.dead else "autopilot dropped"), took, int(gap), notices.keys()]
			+ ("  (took damage on %d frames)" % bumps if bumps > 0 else ""))
		failed = failed or not ok
		_next_trial()
	elif took > TRIAL_SECONDS:
		print("  FAIL never arrived in %d s, gap %d, detours %s" % [int(TRIAL_SECONDS), int(gap), notices.keys()])
		failed = true
		_next_trial()
	return false
