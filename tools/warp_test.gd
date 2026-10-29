# Warp test: computer pilots must jump once and hold the warp, never flicker
# in and out of warp (Kyle 2026-10-29: "ships keep trying to jump over and
# over").  Before the fix a pilot braked the moment its warp charged, which
# ends a warp, and started the next one the following frame.
#   1. Free flight: a Drengin Dreadnought patrols to a point far out in clear
#      space.  It must start one warp, hold it for real distance and drop
#      out near the point.
#   2. Its warp is cut short by hand: no new warp before Ship.WARP_COOLDOWN.
#   3. Wingmen jump with their leader (Kyle 2026-10-29): the player warps,
#      a wingman with the energy warps alongside and drops out with the
#      player; one without the energy stays put; on Hold nobody follows.
#   4. A normal round, every computer pilot flying: no ship starts a warp
#      within 3 s of its last one ending.
#   godot --headless --path . -s tools/warp_test.gd
extends SceneTree

const ROUND_SECONDS := 60.0

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("  ok    " if ok else "  FAIL  ") + what)
	if not ok:
		failed = true


func _frames(n: int) -> void:
	for k in n:
		await physics_frame


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	await _frames(3)
	var ai: AIPilot = await _patrol_trial()
	await _cooldown_trial(ai)
	await _wing_trial()
	main._close_game()
	await _frames(3)
	main._start_game()
	await _frames(3)
	await _round_trial()
	print("warp test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


## A Dreadnought in clear space with a patrol point 30000 px off, the line
## to it clear of every body.
func _patroller() -> AIPilot:
	var w: World = main.world
	var s := w._make_ship("Warp Test", w.player.race_id, "Dreadnought", "Long Haul", false)   # a friend of Earth's guns
	var ai := AIPilot.new()
	ai.name = "AI"
	s.add_child(ai)
	ai.setup(s, w, null, 12)
	ai.ferry = false            # just patrol: no colonist runs, raids or bombing
	ai.bombard = false
	ai.raid = false
	ai.escort_duty = false
	_aim(ai)
	ai.state = AIPilot.State.PATROL
	return ai


## Put the ship 4000 px off Earth, facing a patrol point 30000 px further
## out along a line clear of every body.
func _aim(ai: AIPilot) -> void:
	var w: World = main.world
	var s := ai.ship
	var earth: Planet = w.planet_by_name["Earth"]
	for k in 72:
		var dir := Vector2.RIGHT.rotated(TAU * k / 72.0)
		s.global_position = earth.global_position + dir * (earth.radius + 4000.0)
		if not s._body_in_path(dir * 30000.0) and w.nearest_body(s.global_position + dir * 30000.0).global_position.distance_to(s.global_position + dir * 30000.0) > 6000.0:
			ai.patrol_point = s.global_position + dir * 30000.0
			s.rotation = dir.angle()
			break
	s.velocity = Vector2.ZERO
	s.energy = s.energy_max
	s.warp_block = 0.0
	s.reset_physics_interpolation()


func _patrol_trial() -> AIPilot:
	print("patrol warp:")
	var ai := _patroller()
	var s := ai.ship
	_check(s.qdrv > 0.0, "the Dreadnought has a quantum drive (qdrv %.0f)" % s.qdrv)
	var starts := 0
	var held := 0.0             # seconds in warp after the charge
	var was := false
	var end_dist := -1.0
	for f in 60 * 40:
		await physics_frame
		if s.warping and not was:
			starts += 1
		if s.warping and s.warp_charge >= Ship.WARP_CHARGE_TIME:
			held += 1.0 / 60.0
		if was and not s.warping and end_dist < 0.0:
			end_dist = s.global_position.distance_to(ai.patrol_point)
		was = s.warping
		if end_dist >= 0.0 and f > 60 * 5:
			break
	_check(starts == 1, "one warp start on the way (%d)" % starts)
	_check(held > 3.0, "the warp held for a real jump (%.1f s past the charge)" % held)
	_check(end_dist >= 0.0 and end_dist < s.warp_drop_distance() + 1500.0,
		"dropped out near the point (%.0f px off, drop-out at %.0f)" % [end_dist, s.warp_drop_distance()])
	return ai


func _cooldown_trial(ai: AIPilot) -> void:
	print("cooldown after a warp ends:")
	var s := ai.ship
	_aim(ai)
	var t := 0
	while not (s.warping and s.warp_charge >= Ship.WARP_CHARGE_TIME) and t < 60 * 10:
		await physics_frame
		t += 1
	_check(s.warping, "warp started")
	s.end_warp()
	var restart := -1.0
	for f in 60 * 10:
		await physics_frame
		if s.warping:
			restart = f / 60.0
			break
	_check(restart >= Ship.WARP_COOLDOWN - 0.1, "no new warp for %.0f s (restarted after %.1f s)" % [Ship.WARP_COOLDOWN, restart])
	_check(restart > 0.0, "warps again once the pause is over, still far from the point")


func _wing_trial() -> void:
	print("wingmen jump with the leader:")
	var w: World = main.world
	var p: Ship = w.player
	var mates := []
	for k in 2:
		var s := w._make_ship("Wing %d" % k, p.race_id, "Dreadnought", "Mate %d" % k, false)
		var ai := AIPilot.new()
		ai.name = "AI"
		s.add_child(ai)
		ai.setup(s, w, null, 3)
		ai.join(p, false)
		mates.append(s)
	var low: Ship = mates[1]
	# the player in clear space, the wing in its slots facing the same way
	var helper: AIPilot = mates[0].get_node("AI")
	var spot := p.global_position
	var earth: Planet = w.planet_by_name["Earth"]
	for k in 72:
		var dir := Vector2.RIGHT.rotated(TAU * k / 72.0)
		var at: Vector2 = earth.global_position + dir * (earth.radius + 4000.0)
		p.global_position = at
		if not p._body_in_path(dir * 40000.0):
			spot = at
			p.rotation = dir.angle()
			break
	p.global_position = spot
	p.velocity = Vector2.ZERO
	p.qdrv = maxf(p.qdrv, 1.0)
	p.energy = p.energy_max
	p.reset_physics_interpolation()
	for s in mates:
		var ai: AIPilot = s.get_node("AI")
		s.global_position = ai._wing_spot()
		s.rotation = p.rotation
		s.velocity = Vector2.ZERO
		s.energy = s.energy_max
		s.warp_block = 0.0
		s.reset_physics_interpolation()
	await _frames(30)
	low.energy = 100.0
	w.wing.command(p, "cover", null)
	await _frames(5)
	p.start_warp()
	var followed := false
	var low_jumped := false
	for f in 60 * 6:
		await physics_frame
		p.energy = p.energy_max
		low.energy = minf(low.energy, 100.0)
		low_jumped = low_jumped or low.warping
		if mates[0].warping and mates[0].warp_charge >= Ship.WARP_CHARGE_TIME:
			followed = true
	var gap: float = mates[0].global_position.distance_to(p.global_position)
	_check(followed and mates[0].warping, "a wingman with the energy jumps with the leader")
	_check(gap < 1500.0, "and keeps pace with the leader, whatever its own drive (%.0f px off)" % gap)
	_check(not low_jumped, "one without the energy stays behind (%.0f px off)" % low.global_position.distance_to(p.global_position))
	p.end_warp()
	var dropped := -1.0
	for f in 60 * 3:
		await physics_frame
		if not mates[0].warping:
			dropped = f / 60.0
			break
	_check(dropped >= 0.0, "drops out with the leader (after %.1f s)" % dropped)
	# Hold: the wing keeps the spot
	await _frames(60 * 5)
	mates[0].energy = mates[0].energy_max
	p.energy = p.energy_max
	p.warp_block = 0.0
	w.wing.command(p, "hold", null)
	await _frames(30)
	p.start_warp()
	var held := true
	for f in 60 * 3:
		await physics_frame
		if mates[0].warping:
			held = false
	_check(held, "on Hold the wing stays put when the leader jumps")
	p.end_warp()
	_check(helper.order() == "hold", "the order was Hold")


func _round_trial() -> void:
	print("a normal round, %.0f s:" % ROUND_SECONDS)
	var w: World = main.world
	var was := {}
	var ended := {}
	var starts := 0
	var quick := 0
	var worst := ""
	for f in int(60 * ROUND_SECONDS):
		await physics_frame
		for s in w.ships.duplicate():
			if not is_instance_valid(s) or s.dead or s.is_player:
				continue
			var now := f / 60.0
			if s.warping and not was.get(s, false):
				starts += 1
				if now - float(ended.get(s, -99.0)) < 3.0:
					quick += 1
					worst = "%s %s" % [s.ship_name, s.design_name]
			elif not s.warping and was.get(s, false):
				ended[s] = now
			was[s] = s.warping
	_check(quick == 0, "no warp restarted within 3 s of the last (%d of %d starts%s)" % [quick, starts, ", e.g. " + worst if quick > 0 else ""])
