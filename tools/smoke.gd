# Smoke test: run the game headless for a while with scripted input and
# report errors and counts.  Usage:
#   godot --headless --path . -s tools/smoke.gd
extends SceneTree

var frames := 0
var main: Node
var phase := 0


func _init() -> void:
	print("t=", Time.get_ticks_msec(), " init")
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	main._start_game()
	print("t=", Time.get_ticks_msec(), " game started; ships=", main.world.ships.size(), " planets=", main.world.planets.size())


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 6050:
		print("smoke test done")
		quit()
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	if p != null and not p.dead:
		# fly around and shoot at whatever is nearest
		p.inputs["thrust"] = 1.0
		p.inputs["turn"] = sin(frames * 0.02)
		p.inputs["fire_primary"] = frames % 40 < 20
		p.inputs["fire_beam"] = frames % 90 < 30
		p.inputs["fire_mine"] = frames % 300 == 0
		p.inputs["fire_missile"] = frames % 200 == 0
		if frames == 60:
			p.target = w.nearest_enemy_ship(p.global_position, p.race_id, 1e9, p)
			w.cycle_nav(p)
		if frames == 400:
			p.start_warp()
		if frames == 700:
			p.autopilot = true
		if frames == 900:
			p.take_damage(1e6, w.ships[1])
		if frames == 1500:
			w.try_refit(p)
			p.set_cloak(true)
	if frames % 1000 == 0:
		var alive := 0
		for s in w.ships:
			if not s.dead:
				alive += 1
		print("t=", Time.get_ticks_msec(), " frame ", frames, " alive=", alive, " missiles=", w.missiles.size(), " pickups=", w.powerups.size(),
			" player hull=", (int(p.hull) if p else -1), " pos=", (p.global_position.round() if p else Vector2.ZERO), " score=", (p.score if p else 0))
	if frames == 6000:
		var kills := 0
		for s in w.ships:
			kills += s.kills
		print("total kills=", kills, " planets per race=", w.planet_counts)
		main._show_menu()
	return false
