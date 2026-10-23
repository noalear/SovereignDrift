# Physics checks: run the space on its own and report what holds.  Usage:
#   godot --headless --path . -s tools/physics_test.gd
extends SceneTree

var failures := 0


func _init() -> void:
	call_deferred("_run")


func _check(name: String, ok: bool, detail: String = "") -> void:
	print(("PASS  " if ok else "FAIL  ") + name + ("" if detail.is_empty() else "  (" + detail + ")"))
	if not ok:
		failures += 1


func _box(w: float, h: float, mass: float) -> PhysicsBody:
	var b := PhysicsBody.new()
	b.set_hull(PackedVector2Array([Vector2(-w, -h), Vector2(w, -h), Vector2(w, h), Vector2(-w, h)]))
	b.set_mass(mass)
	return b


func _run() -> void:
	_inertia()
	_head_on()
	_off_centre_spin()
	_stacking_separation()
	_bullet_sweep()
	_orbit()
	await _ship_orbit()
	await _rock_belt()
	await _timing()
	print("physics test done: %d failure(s)" % failures)
	quit(1 if failures > 0 else 0)


func _inertia() -> void:
	# a 2w x 2h plate about its centre: m (w^2 + h^2) / 3
	var b := _box(10, 5, 12.0)
	var expect := 12.0 * (100.0 + 25.0) / 3.0
	_check("polygon inertia matches the plate formula", absf(b.inertia - expect) < 0.01, "%f vs %f" % [b.inertia, expect])
	var c := PhysicsBody.new()
	c.set_circle(4.0)
	c.set_mass(2.0)
	_check("circle inertia is m r^2 / 2", is_equal_approx(c.inertia, 16.0))


func _head_on() -> void:
	var sp := PhysicsSpace.new()
	var a := _box(10, 10, 2.0)
	var b := _box(10, 10, 1.0)
	a.pos = Vector2(-40, 0)
	a.vel = Vector2(100, 0)
	b.pos = Vector2(40, 0)
	b.vel = Vector2(-50, 0)
	a.restitution = 0.5
	b.restitution = 0.5
	sp.add_body(a)
	sp.add_body(b)
	var p0 := a.vel * a.mass + b.vel * b.mass
	var hits := 0
	for i in 120:
		sp.step(1.0 / 60.0)
		hits += sp.contacts.size()
	var p1 := a.vel * a.mass + b.vel * b.mass
	_check("head-on collision happens", hits > 0)
	_check("momentum is conserved", p0.distance_to(p1) < 0.01, "%s -> %s" % [p0, p1])
	_check("bodies separate after the bounce", b.vel.x > a.vel.x, "a=%s b=%s" % [a.vel, b.vel])
	_check("the lighter body is thrown back faster", absf(b.vel.x - (-50.0)) > absf(a.vel.x - 100.0))


func _off_centre_spin() -> void:
	var s := PhysicsBody.new()
	var hull := CollisionHulls.for_ship("Scout", 1, 22.0)
	s.set_hull(hull)
	s.set_mass(4500.0)
	_check("the Scout's collision hull is traced from its painting", hull.size() >= 6 and s.radius > 25.0 and s.radius < 45.0, "%d verts, radius %.1f" % [hull.size(), s.radius])
	var j := Vector2(0, 1) * 150.0 * 180.0    # a torpedo hit at the nose
	s.apply_impulse(j, s.pos + Vector2(20, 0))
	_check("a nose hit shoves the hull", absf(s.vel.y - 6.0) < 0.5, "dv=%f" % s.vel.y)
	_check("a nose hit spins the hull", absf(s.ang_vel) > 0.2, "w=%f rad/s" % s.ang_vel)


func _stacking_separation() -> void:
	var sp := PhysicsSpace.new()
	var a := _box(20, 20, 1.0)
	var b := _box(20, 20, 1.0)
	b.pos = Vector2(25, 3)      # deeply overlapping at rest
	sp.add_body(a)
	sp.add_body(b)
	for i in 120:
		sp.step(1.0 / 60.0)
	var gap := b.pos.x - a.pos.x
	_check("overlapping boxes are pushed apart", gap >= 40.0 - PhysicsSpace.SLOP * 2.0, "gap=%f" % gap)
	_check("push-out does not explode velocities", a.vel.length() < 60.0 and b.vel.length() < 60.0, "%s %s" % [a.vel, b.vel])


func _bullet_sweep() -> void:
	var sp := PhysicsSpace.new()
	var wall := _box(2, 40, 1000.0)
	wall.pos = Vector2(300, 0)
	var shot := PhysicsBody.new()
	shot.kind = PhysicsBody.Kind.PROJECTILE
	shot.set_circle(3.0)
	shot.set_mass(10.0)
	shot.bullet = true
	shot.pos = Vector2(0, 10)
	shot.vel = Vector2(3000, 0)     # 50 px per frame, more than the wall is thick
	sp.add_body(wall)
	sp.add_body(shot)
	var hit := false
	for i in 20:
		sp.step(1.0 / 60.0)
		if not sp.bullet_hits.is_empty():
			hit = true
			var h: Dictionary = sp.bullet_hits[0]
			_check("sweep reports the hit point on the wall's face", absf(h["point"].x - 298.0) < 1.0, str(h["point"]))
			break
	_check("a fast shot does not tunnel through a thin hull", hit)


func _orbit() -> void:
	var sp := PhysicsSpace.new()
	var sun := PhysicsBody.new()
	sun.kind = PhysicsBody.Kind.PLANET
	sun.set_circle(420.0)
	sun.is_sun = true
	sun.gm = PhysicsSpace.SUN_GM
	sp.add_body(sun)
	var b := PhysicsBody.new()
	b.set_circle(5.0)
	b.set_mass(100.0)
	b.pos = Vector2(4800, 0)
	b.vel = Vector2(0, sqrt(PhysicsSpace.SUN_GM / 4800.0))
	sp.add_body(b)
	var far := b.pos.length()
	b.vel = Vector2.ZERO
	for i in 60 * 120:
		sp.step(1.0 / 60.0)
	_check("beyond the sun's reach a body is left alone", absf(b.pos.length() - far) < 1.0 and b.vel.length() < 60.0, "r=%f" % b.pos.length())
	# inside the well a circular orbit holds
	b.pos = Vector2(1200, 0)
	b.vel = Vector2(0, sqrt(PhysicsSpace.SUN_GM / 1200.0))
	var r_min := INF
	var r_max := 0.0
	for i in 60 * 300:   # five minutes, several orbits
		sp.step(1.0 / 60.0)
		var r := b.pos.length()
		r_min = minf(r_min, r)
		r_max = maxf(r_max, r)
	_check("a circular orbit in the sun's well stays circular for five minutes", r_min > 1150.0 and r_max < 1250.0, "r in [%f, %f]" % [r_min, r_max])


func _ship_orbit() -> void:
	# a ship parked beside a planet that itself orbits the sun must stay beside it
	var main: Node = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	main._start_game()
	await process_frame
	var w: World = main.world
	var earth: Planet = w.planet_by_name["Earth"]
	var s: Ship = w.player
	s.inputs["thrust"] = 0.0
	s.inputs["turn"] = 0.0
	# only the player flies here; the other pilots sit out so nothing bumps it
	for o in w.ships:
		if o != s:
			o.body.enabled = false
	# park it in a circular orbit 180 px up
	s.global_position = earth.global_position + Vector2(earth.radius + 180.0, 0).rotated(randf() * TAU)
	s.velocity = w.space.circular_orbit_velocity(earth.body, s.global_position)
	var d0 := s.global_position.distance_to(earth.global_position)
	var max_d := 0.0
	var step := 1.0 / 60.0
	for i in 60 * 300:   # five minutes of drifting
		for p in w.planets:
			p.update_orbit(step)
			p.sync_body()
		s.pre_step(step)
		w.space.step(step)
		s.post_step(step)
		max_d = maxf(max_d, s.global_position.distance_to(earth.global_position))
		if i % 1800 == 0:
			print("    t=%ds d=%.0f relv=%.1f g=%.2f v=%s pos=%s" % [i / 60, s.global_position.distance_to(earth.global_position), (s.velocity - earth.velocity).length(), s.body.gravity.length(), s.velocity, s.global_position])
	_check("a parked ship keeps station beside its moving planet for five minutes", max_d < d0 * 2.5, "start %f, max %f" % [d0, max_d])
	_check("a parked ship is not pulled into the sun", s.global_position.length() > 3000.0, "r=%f" % s.global_position.length())
	# a moon's rail and its planet's gravity agree
	var luna: Planet = w.planet_by_name["Luna"]
	var v_rail := (luna.velocity - earth.velocity).length()
	var v_kepler := w.space.orbital_speed(earth.body, luna.orbit_radius)
	_check("Luna's rail speed is its Kepler speed round Earth", absf(v_rail - v_kepler) < 1.0, "%f vs %f" % [v_rail, v_kepler])
	main._show_menu()
	main.queue_free()


func _rock_belt() -> void:
	var main: Node = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	main._start_game()
	await process_frame
	var w: World = main.world
	var rocks := w.debris.filter(func(d): return d.is_rock)
	_check("the belt has rocks", rocks.size() > 50, str(rocks.size()))
	# nobody flies here: an unpiloted ship drifting into the belt is not the belt's fault
	for o in w.ships:
		o.body.enabled = false
	var r0 := []
	for d in rocks:
		r0.append(d.global_position.length())
	var step := 1.0 / 60.0
	for i in 60 * 300:
		for p in w.planets:
			p.update_orbit(step)
			p.sync_body()
		for d in w.debris:
			d.pre_step(step)
		w.space.step(step)
		for d in w.debris:
			d.post_step(step)
	var worst := 0.0
	for i in rocks.size():
		worst = maxf(worst, absf(rocks[i].global_position.length() - r0[i]) / r0[i])
	_check("rocks stay in the belt for five minutes", worst < 0.15, "worst radial drift %.1f%%" % (worst * 100.0))
	main._show_menu()
	main.queue_free()


func _timing() -> void:
	var main: Node = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	main._start_game()
	await process_frame
	var w: World = main.world
	var total := 0.0
	var worst := 0.0
	for i in 600:
		w._physics_process(1.0 / 60.0)
		total += w.space.step_time_ms
		worst = maxf(worst, w.space.step_time_ms)
	print("physics step: mean %.2f ms, worst %.2f ms over 600 frames, %d bodies" % [total / 600.0, worst, w.space.bodies.size()])
	_check("the step stays well inside a frame", total / 600.0 < 5.0)
	main._show_menu()
	main.queue_free()
