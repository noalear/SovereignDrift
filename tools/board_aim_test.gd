# Boarding fire, gun convergence, engines-out flight and retro jets
# (Kyle, 2026-10-31).  In free flight, out in open space, checks that:
#  - a wingman holds fire on a disabled enemy the player has targeted (it
#    used to copy the leader's target and shoot it), and turns to another
#    enemy instead; once the player locks the tractor on, the wingman's
#    stray shots do it no harm, while the player's own still do;
#  - pressing the board key out of range claims the prize for the side;
#  - every gun under the player's hand points from its own mount at the
#    cursor, so the shots cross there (and never closer than CONVERGE_MIN);
#  - with the engines shot out, holding thrust, boost and brake through
#    the real controls changes nothing and lights no flame;
#  - thrusting ahead lights the main plume, reversing lights the bow's
#    retro jets instead.
#   godot --headless --path . -s tools/board_aim_test.gd
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
	var w = main.world
	await _flight(w)
	await _aim(w)
	await _boarding(w)
	print("board_aim_test: %s" % ("FAILED" if failed else "all checks passed"))
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	print(("  ok    " if ok else "  FAIL  ") + what)
	if not ok:
		failed = true


func _frames(n: int) -> void:
	for i in n:
		await physics_frame


func _place(s: Ship, pos: Vector2, vel := Vector2.ZERO) -> void:
	s.global_position = pos
	s.velocity = vel
	s.rotation = 0.0
	s.body.pos = pos
	s.body.vel = vel
	s.body.angle = 0.0
	s.body.ang_vel = 0.0
	s.angular_velocity = 0.0
	s.fuel = s.fuel_max
	s.reset_physics_interpolation()


func _release() -> void:
	for a in ["thrust", "reverse", "brake", "boost", "turn_left", "turn_right"]:
		Input.action_release(a)


func _flight(w) -> void:
	var p: Ship = w.player
	p.systems.repair_all()
	p.apply_systems()
	p.flight_assist = false
	_place(p, OPEN_SPACE)
	await _frames(5)
	Input.action_press("thrust")
	await _frames(30)
	_check(p.thrust_level > 0.3 and p.retro_level < 0.05, "thrusting ahead lights the main plume (main %.2f, retro %.2f)" % [p.thrust_level, p.retro_level])
	_release()
	await _frames(30)
	var ahead0 := p.local_velocity().dot(p.forward())
	Input.action_press("reverse")
	await _frames(30)
	_check(p.retro_level > 0.3 and p.thrust_level < 0.1, "reversing lights the bow's retro jets instead (main %.2f, retro %.2f)" % [p.thrust_level, p.retro_level])
	_check(p.local_velocity().dot(p.forward()) < ahead0 - 10.0, "and pushes the ship astern (%.1f px/s ahead, was %.1f)" % [p.local_velocity().dot(p.forward()), ahead0])
	_release()
	_check(p.art_model.get("retro", []).size() == 2, "the hull has a retro jet either side of the bow")
	# engines shot out for real: hits into the stern
	_place(p, OPEN_SPACE, Vector2(120, -30))
	p.shield = 0.0
	p.systems.quiet = -1000.0
	var stern: Vector2 = p.systems.get_system("engines")["pts"][0]
	var hits := 0
	while not p.engines_out() and hits < 80 and not p.dead:
		p.hull = p.hull_max
		p.take_damage(40.0, null, p.global_position + stern.rotated(p.rotation))
		hits += 1
	_check(p.engines_out(), "hits into the stern knock the player's engines out (%d hits)" % hits)
	p.systems.quiet = -1000.0
	p.hull = p.hull_max
	await _frames(20)
	var v0 := p.velocity
	Input.action_press("thrust")
	Input.action_press("boost")
	await _frames(40)
	_check(p.velocity.distance_to(v0) < 0.5, "engines out: thrust and boost change nothing (%s, was %s)" % [p.velocity, v0])
	_check(p.thrust_level < 0.02 and not p.boosting, "engines out: no flame and no boost (main %.2f)" % p.thrust_level)
	_release()
	Input.action_press("brake")
	await _frames(40)
	_check(p.velocity.distance_to(v0) < 0.5, "engines out: the brake does nothing either (%s)" % p.velocity)
	_check(p.retro_level < 0.02, "engines out: no retro flame (%.2f)" % p.retro_level)
	Input.action_press("reverse")
	await _frames(30)
	_check(p.velocity.distance_to(v0) < 0.5 and p.retro_level < 0.02, "engines out: reversing does nothing")
	_release()
	p.systems.repair_all()
	p.apply_systems()
	await _frames(5)


func _aim(w) -> void:
	var p: Ship = w.player
	p.refit(root.get_node("GameData").stock_design("Cruiser", p.race_id))
	_place(p, OPEN_SPACE)
	await _frames(3)
	for spot in [Vector2(700, 90), Vector2(520, -160), Vector2(900, 0)]:
		var at: Vector2 = p.global_position + spot
		p.aim_point = at
		var crossing := 0
		var aimed := 0
		for wpn in p.weapons:
			if wpn["missile"] == "002":
				continue
			var rel: float = p._aim_angle(wpn)
			var from: Vector2 = p.mount_point(wpn)
			var dir := Vector2.RIGHT.rotated(p.rotation + rel)
			# a gun that can reach the spot inside its arc points right at it
			var want := wrapf((at - from).angle() - p.rotation, -PI, PI)
			if not Ship.in_arc(wpn, want):
				continue
			aimed += 1
			var miss := absf((at - from).cross(dir))
			if miss < 1.5 and (at - from).dot(dir) > 0.0:
				crossing += 1
		_check(aimed >= 2 and crossing == aimed, "aim %s: %d of %d guns in arc point straight at the cursor from their mounts" % [spot, crossing, aimed])
	# off-centre mounts angle in: two side turrets do not fire parallel
	p.aim_point = p.global_position + Vector2(600, 0)
	var sides := []
	for wpn in p.weapons:
		if wpn["mount_radius"] > 1.0 and absf(sin(wpn["mount_angle"])) > 0.3 and wpn["missile"] != "002":
			sides.append(p._aim_angle(wpn))
	_check(sides.size() >= 2 and absf(sides[0] - sides[1]) > 0.01, "side mounts angle in toward the cursor (%s)" % [sides])
	# a cursor on the hull: the guns meet no closer than CONVERGE_MIN sizes
	p.aim_point = p.global_position + Vector2(5, 0)
	var ok := true
	for wpn in p.weapons:
		if wpn["missile"] == "002" or wpn["kind"] == "beam":
			continue
		var rel: float = p._aim_angle(wpn)
		var from: Vector2 = p.mount_point(wpn)
		var far: Vector2 = p.global_position + p.forward() * p.size * Ship.CONVERGE_MIN
		var want := wrapf((far - from).angle() - p.rotation, -PI, PI)
		if Ship.in_arc(wpn, want) and absf(wrapf(rel - want, -PI, PI)) > 0.01:
			ok = false
	_check(ok, "a cursor on the hull: the guns meet %.1f sizes out" % Ship.CONVERGE_MIN)
	# the turret pictures follow the aim (every mount but the mine rack)
	p.aim_point = p.global_position + Vector2(500, 200)
	await _frames(60)
	var drawn := 0
	for wpn in p.weapons:
		if wpn["missile"] != "002" and absf(wrapf(float(wpn.get("aim_vis", wpn["fire_angle"])) - p._aim_angle(wpn), -PI, PI)) < 0.05:
			drawn += 1
	_check(drawn >= p.weapons.size() - 1, "the guns are drawn turned to where they shoot (%d of %d)" % [drawn, p.weapons.size()])
	p.aim_point = Vector2.INF
	# online, the aim travels with its distance so the host's guns meet there too
	var pk := NetCodec.pack_input(1, p.inputs, OPEN_SPACE + Vector2(640, 0), OPEN_SPACE)
	var un := NetCodec.unpack_input(pk)
	_check(absf(float(un["aim_dist"]) - 640.0) < 1.0 and absf(float(un["aim_angle"])) < 0.001, "the input packet carries the aim's distance (%s)" % un["aim_dist"])


func _boarding(w) -> void:
	var p: Ship = w.player
	p.refit(root.get_node("GameData").stock_design("Destroyer", p.race_id))
	_place(p, OPEN_SPACE)
	var foe: Ship = w.spawn_ship("Foe", 3, "Scout", "Test Foe", OPEN_SPACE + Vector2(900, 0), Vector2.ZERO, 3)
	foe.get_node("AI").set_physics_process(false)
	for k in foe.inputs.keys():
		foe.inputs[k] = false if typeof(foe.inputs[k]) == TYPE_BOOL else 0.0
	var other: Ship = w.spawn_ship("Other", 2, "Scout", "Test Other", OPEN_SPACE + Vector2(600, 700), Vector2.ZERO, 3)
	other.get_node("AI").set_physics_process(false)
	var e: Dictionary = foe.systems.get_system("reactor")
	e["hp"] = 0.0
	e["offline"] = true
	foe.apply_systems()
	foe.systems.quiet = -1000.0
	_check(foe.is_disabled(), "the foe is disabled")
	var wing: Ship = w.spawn_ship("Wingman", p.race_id, "Destroyer", "Test Wing", OPEN_SPACE + Vector2(200, 250), Vector2.ZERO, 6)
	var ai: AIPilot = wing.get_node("AI")
	ai.escort = p
	ai.state = AIPilot.State.ATTACK
	wing.target = foe
	p.target = foe
	_check(foe.spoken_for(p.team()), "a disabled enemy the player targets is theirs to take")
	var held := true
	var hit_foe := false
	var went_other := false
	var hull0 := foe.hull
	await _frames(70)          # (a computer pilot thinks again within a second)
	for i in 240:
		await physics_frame
		if wing.target == foe:
			if held and OS.has_environment("SF_DEBUG"):
				print("  wingman on the foe at frame %d (state %d)" % [i, ai.state])
			held = false
		if foe.hull < hull0:
			hit_foe = true
		if wing.target == other:
			went_other = true
	_check(held and not hit_foe, "the wingman holds fire on it (target %s, foe hull %d of %d)" % [wing.target.ship_name if wing.target != null else "none", int(foe.hull), int(hull0)])
	_check(went_other, "and goes after the other enemy instead")
	# a friend's stray shot does no harm while the player's prize; the player's own does
	foe.shield = 0.0
	var h := foe.hull
	foe.take_damage(60.0, wing, foe.global_position)
	_check(foe.hull == h, "a wingman's stray shot leaves it be")
	foe.take_damage(60.0, p, foe.global_position)
	_check(foe.hull < h, "the player's own shot still lands")
	# an order to the wing to attack it: the wing finishes it
	var o: Dictionary = w.wing.order_of(p)
	var was: String = o["order"]
	o["order"] = "attack"
	o["target"] = foe
	_check(not foe.spoken_for(p.team()), "ordered to attack it, the wing may finish it")
	o["order"] = was
	o["target"] = null
	# not targeted any more: fair game again
	p.target = null
	_check(not foe.spoken_for(p.team()), "untargeted, it is fair game again")
	# the board key out of range claims it for the side
	p.target = foe
	w.capture.try_board(p, foe)
	_check(foe.claimed_by == p and p.boarding == null, "the board key out of range claims it without locking on")
	_check(foe.spoken_for(p.team()), "and the side leaves it alone")
	# locked on: still nobody else's to shoot
	_place(p, foe.global_position + Vector2(-90, 0), foe.velocity)
	w.capture.try_board(p, foe)
	_check(p.boarding == foe, "close in, the tractor locks on")
	p.target = null
	_check(foe.spoken_for(p.team()), "while boarding it stays the player's, whatever is targeted")
	h = foe.hull
	foe.take_damage(60.0, wing, foe.global_position)
	_check(foe.hull == h, "and a wingman's shot does it no harm")
	w.capture.cancel(p, "")
