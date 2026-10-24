# Respawn test: in the Sol system game, ships the player destroys do not
# relaunch on top of the player.  Parks the player beside the Drengin
# starbase, destroys the Drengin pilots and both pirates, and checks that
# none comes back within sight, that they do come back once the player
# leaves, and that pirates are a rare sight (minutes between visits).
#   godot --headless --path . -s tools/respawn_test.gd
extends SceneTree

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
	main._start_game()
	await process_frame
	await process_frame
	var w = main.world
	var p: Ship = w.player
	var enemy := 2 if p.race_id != 2 else 0
	var pirates_out: Array = w.ships.filter(func(s): return s.race_id == 3 and not s.dead)
	_check(pirates_out.is_empty(), "no pirate in play when the game starts (%d)" % pirates_out.size())
	var base: Ship = w.starbase_of(enemy)
	_park(p, base.global_position + Vector2(base.size + 200.0, 0), base.velocity)
	var victims := []
	for s in w.ships:
		if not s.is_station and not s.dead and (s.race_id == enemy or s.race_id == 3):
			victims.append(s)
			s.take_damage(1e9, p)
	await physics_frame
	var dead := victims.filter(func(s): return s.dead).size()
	_check(dead == victims.size(), "destroyed %d enemy and pirate pilots" % dead)
	# 60 s parked at their starbase, then away: every relaunch is out of sight
	var was_dead := {}
	for s in victims:
		was_dead[s] = true
	var back_at := {}       # ship -> [seconds, distance to the player when it relaunched]
	var t := 0.0
	while t < 75.0:
		await physics_frame
		t += 1.0 / 60.0
		if t < 45.0:
			_park(p, base.global_position + Vector2(base.size + 200.0, 0), base.velocity)
		elif t < 45.1:
			_park(p, base.global_position + (base.global_position - w._home_planet(p.race_id).global_position).normalized() * -9000.0, Vector2.ZERO)
		p.hull = p.hull_max
		p.invulnerable = 1.0
		for s in victims:
			if was_dead[s] and not s.dead:
				back_at[s] = [t, s.global_position.distance_to(p.global_position)]
			was_dead[s] = s.dead
	var nearest := INF
	var first_pirate := INF
	var enemy_back := 0
	for s in back_at:
		nearest = minf(nearest, back_at[s][1])
		if s.race_id == 3:
			first_pirate = minf(first_pirate, back_at[s][0])
		else:
			enemy_back += 1
	print("  relaunches: ", back_at.values().map(func(v): return "%.0fs@%dpx" % v))
	_check(nearest >= w.SPAWN_CLEAR, "every relaunch was out of sight (nearest %d px)" % int(nearest))
	_check(enemy_back > 0, "%d enemy pilots relaunched" % enemy_back)
	_check(first_pirate >= w.PIRATE_RETURN.x, "a destroyed pirate stays away for at least %d s (first back at %.0f s)" % [int(w.PIRATE_RETURN.x), first_pirate])
	# and at the start there is no pirate in play at all; the first comes minutes in
	var benched: Array = w.respawn_queue.filter(func(e): return e[0].race_id == 3)
	_check(benched.all(func(e): return e[1] >= w.PIRATE_RETURN.x * 0.5), "the benched pirate is minutes away (%s)" % [benched.map(func(e): return int(e[1]))])
	print("respawn test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _park(p: Ship, at: Vector2, vel: Vector2) -> void:
	p.global_position = at
	p.velocity = vel
	p.hull = p.hull_max
	p.invulnerable = 1.0
	p.autopilot = false
	if p.body != null:
		p.body.pos = at
		p.body.vel = vel
	p.reset_physics_interpolation()


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
