# Autopilot test: start on the far side of the Sun from the nav target and
# check the autopilot flies around the Sun's heat instead of through it.
#   godot --headless --path . -s tools/autopilot_sun.gd
# Exit code 1 when the ship enters the damaging heat, dies, or gets nowhere.
extends SceneTree

const TRIAL_FRAMES := 3000
const TRIALS := [["Mars", 0.0], ["Mars", 0.25], ["Venus", -0.2], ["Earth", 0.1]]

var frames := 0
var main: Node
var trial := -1
var trial_start := 0
var start_gap := 0.0
var min_sun := 1e12
var detoured := false
var failed := false
var died_seen := false
var last_line := ""


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		# Main.gd failed to compile: usually a stale class cache after new
		# class_name files were pulled.  Fix: godot --headless --path . --import
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	await process_frame


func _next_trial(w: World) -> void:
	trial += 1
	if trial >= TRIALS.size():
		print("autopilot around the Sun: ", "FAILED" if failed else "passed")
		quit(1 if failed else 0)
		return
	var p: Ship = w.player
	var target: Planet = w.planet_by_name[TRIALS[trial][0]]
	var sun: Planet = w.planet_by_name["Sol"]
	# start opposite the target, at the target's distance from the Sun, a little off the line
	var from_sun: Vector2 = (sun.global_position - target.global_position).normalized().rotated(TRIALS[trial][1])
	var r: float = maxf(target.global_position.distance_to(sun.global_position), sun.radius * 8.0)
	p.global_position = sun.global_position + from_sun * r
	p.velocity = Vector2.ZERO
	p.rotation = (-from_sun).angle()     # nose straight at the Sun, the worst case
	p.hull = p.hull_max
	p.energy = p.energy_max
	p.nav_target = target
	p.autopilot = true
	if p.has_meta("detour_around"):
		p.remove_meta("detour_around")
	start_gap = p.global_position.distance_to(target.global_position)
	min_sun = 1e12
	detoured = false
	trial_start = frames


func _process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	if p == null:
		return false
	if trial < 0:
		if frames > 30:
			_next_trial(w)
		return false
	var sun: Planet = w.planet_by_name["Sol"]
	var target: Planet = p.nav_target
	# keep the computer pilots off us so only the route is tested
	for s in w.ships:
		if s != p and not s.is_station and s.global_position.distance_to(p.global_position) < 4000.0:
			s.global_position += Vector2(20000, 20000)
	if p.dead:
		if not died_seen:
			died_seen = true
			_fail("trial %d: ship died (heat %.2f, last %s)" % [trial, w.sun_heat, last_line])
		return false
	if died_seen:
		died_seen = false
		_next_trial(w)
		return false
	p.autopilot = true
	p.hull = p.hull_max     # enemy fire is not what this test is about
	if p.has_meta("detour_around"):
		detoured = true
	var d: float = p.global_position.distance_to(sun.global_position)
	min_sun = minf(min_sun, d)
	var heat := clampf(1.0 - (d - sun.radius) / (sun.radius * w.SUN_HEAT_RADII), 0.0, 1.0)
	if heat > 0.35:
		_fail("trial %d: entered the damaging heat (%.2f) at %s" % [trial, heat, p.global_position.round()])
		_next_trial(w)
		return false
	var gap: float = p.global_position.distance_to(target.global_position) - target.radius
	if OS.get_environment("AP_DEBUG") != "" and (frames - trial_start) % 60 == 0:
		last_line = "t=%d pos=%s" % [(frames - trial_start) / 60, p.global_position.round()]
		print("  t=%d pos=%s sun=%d v=%d warp=%s ap=%s hull=%d energy=%d block=%.1f gap=%d" % [(frames - trial_start) / 60, p.global_position.round(), int(d),
			int(p.velocity.length()), p.warping, p.autopilot, int(p.hull), int(p.energy), p.warp_block, int(gap)])
		var leg: Vector2 = p._route(target.global_position - p.global_position)["to"]
		var blockers := []
		for b in w.planets:
			var rel: Vector2 = b.global_position - p.global_position
			var along := rel.dot(leg.normalized())
			if b != target and along > 0.0 and along < leg.length():
				var clear: float = p._sun_safe_radius(b) if b.look == "Sun" else b.radius + 500.0
				if absf(rel.cross(leg.normalized())) < clear:
					blockers.append(b.body_name)
		print("    leg=%s len=%d nose_err=%.2f blockers=%s" % [leg.normalized(), int(leg.length()), wrapf(leg.angle() - p.rotation, -PI, PI), blockers])
	if gap < 600.0 or frames - trial_start >= TRIAL_FRAMES:
		var ok := gap < start_gap * 0.25
		print("trial %d to %s: %s, closest to the Sun %d (heat edge %d), detour=%s, gap %d of %d, %.1f s" % [
			trial, target.body_name, "ok" if ok else "FAIL", int(min_sun), int(sun.radius * (1.0 + w.SUN_HEAT_RADII)),
			detoured, int(gap), int(start_gap), (frames - trial_start) / 60.0])
		if not ok:
			failed = true
		_next_trial(w)
	return false


func _fail(what: String) -> void:
	print("FAIL ", what)
	failed = true
