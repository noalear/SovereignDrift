# Starbase respawn test: a destroyed home starbase stays down for
# World.STARBASE_RESPAWN_DELAY (about a minute, Kyle) before it is rebuilt.
# Meanwhile the HUD has a countdown, the player relaunches over the home
# world, and nobody can dock there; then it is back where it was.
#   godot --headless --path . -s tools/starbase_respawn_test.gd
extends SceneTree

var main: Node
var failed := false
var notes: Array[String] = []


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
	w.message.connect(func(text: String, _c: Color): notes.append(text))
	var p: Ship = w.player
	var delay: float = w.STARBASE_RESPAWN_DELAY
	_check(delay >= 50.0 and delay <= 70.0, "a starbase is rebuilt after about a minute (%d s)" % int(delay))
	var own: Ship = w.starbase_of(p.race_id)
	var enemy_race := 2 if p.race_id != 2 else 0
	var enemy: Ship = w.starbase_of(enemy_race)
	_check(own != null and enemy != null, "both starbases stand at the start")
	_check(w.starbase_back_in(p.race_id) < 0.0, "no countdown while the starbase stands")
	var own_spot: Vector2 = own.global_position - w._home_planet(p.race_id).global_position
	var t0: float = w.elapsed
	own.take_damage(1e9, null)
	enemy.take_damage(1e9, p)
	await physics_frame
	_check(own.dead and enemy.dead, "both starbases destroyed")
	_check(w.starbase_of(p.race_id) == null, "starbase_of() is empty while it is down")
	var left: float = w.starbase_back_in(p.race_id)
	_check(left > delay - 1.0 and left <= delay, "the countdown starts at the full delay (%.1f s)" % left)
	_check(notes.any(func(n): return "is down" in n and "rebuilt in %d seconds" % int(delay) in n), "everyone is told when it will be back")
	_check(not World.is_yard(own), "no shipyard at a destroyed starbase")
	# the player dies while home is down: relaunches over the home world
	_park(p, w._home_planet(p.race_id).global_position + Vector2(3000, 0), Vector2.ZERO)
	p.invulnerable = 0.0
	p.take_damage(1e9, null)
	await physics_frame
	_check(p.dead, "the player is shot down")
	var t := 0.0
	while p.dead and t < 15.0:
		await physics_frame
		t += 1.0 / 60.0
	_check(not p.dead, "the player relaunches while the starbase is down (%.1f s)" % t)
	var home: Planet = w._home_planet(p.race_id)
	_check(p.global_position.distance_to(home.global_position) < home.radius + 800.0, "...in orbit over %s (%d px from it)" % [home.body_name, int(p.global_position.distance_to(home.global_position))])
	_check(notes.any(func(n): return n == "Launched from %s." % home.body_name), "...and is told it launched from %s" % home.body_name)
	# still down half way; back by the delay
	while w.elapsed - t0 < delay * 0.5:
		await physics_frame
		p.hull = p.hull_max
		p.invulnerable = 1.0
	_check(own.dead and enemy.dead and w.starbase_of(p.race_id) == null, "still down at %d s" % int(w.elapsed - t0))
	_check(absf(w.starbase_back_in(p.race_id) - (delay - (w.elapsed - t0))) < 0.5, "the countdown runs down (%.1f s left)" % w.starbase_back_in(p.race_id))
	var back_at := -1.0
	while w.elapsed - t0 < delay + 3.0:
		await physics_frame
		p.hull = p.hull_max
		p.invulnerable = 1.0
		if back_at < 0.0 and not own.dead:
			back_at = w.elapsed - t0
	_check(back_at >= delay - 0.1 and back_at <= delay + 1.0, "the starbase is back after %.1f s" % back_at)
	_check(not enemy.dead, "so is the enemy's")
	_check(w.starbase_of(p.race_id) == own and w.starbase_back_in(p.race_id) < 0.0, "starbase_of() finds it again; no countdown")
	_check(own.hull >= own.hull_max * 0.99, "rebuilt at full hull")
	var spot: Vector2 = own.global_position - w._home_planet(p.race_id).global_position
	_check(absf(spot.length() - own_spot.length()) < 5.0 and absf(angle_difference(spot.angle(), own_spot.angle())) < 0.05, "on its old station beside %s" % home.body_name)
	_check(notes.any(func(n): return "is rebuilt and back on station" in n), "everyone is told it is back")
	print("starbase respawn test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _park(p: Ship, at: Vector2, vel: Vector2) -> void:
	p.global_position = at
	p.velocity = vel
	p.autopilot = false
	if p.body != null:
		p.body.pos = at
		p.body.vel = vel
	p.reset_physics_interpolation()


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
