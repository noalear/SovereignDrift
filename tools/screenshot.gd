# Takes screenshots of the menu and the game for a visual check.
#   xvfb-run -s "-screen 0 1920x1080x24" godot --path . --windowed -s tools/screenshot.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved ", name)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 90:
		_shot("menu")
	if frames == 100:
		main.menu._set_page("pilot")
	if frames == 120:
		_shot("menu_pilot")
		main.menu._set_page("settings")
	if frames == 140:
		_shot("menu_settings")
		main._start_game()
	if frames >= 150 and main.world != null and main.world.player != null:
		var w: World = main.world
		var p: Ship = w.player
		p.inputs["thrust"] = 1.0 if frames < 400 else 0.0
		p.inputs["turn"] = 0.3 if frames < 250 else 0.0
		p.inputs["fire_primary"] = frames > 200 and frames % 30 < 10
		p.inputs["fire_beam"] = frames > 260 and frames % 60 < 20
		if frames == 200:
			# bring an enemy close for the picture
			var e: Ship = w.nearest_enemy_ship(p.global_position, p.race_id, 1e9)
			if e != null:
				e.global_position = p.global_position + Vector2(500, 120)
				e.velocity = p.velocity
				p.target = e
			w.cycle_nav(p)
		if frames == 230:
			_shot("game_launch")
		if frames == 330:
			_shot("game_combat")
		if frames == 360:
			main.hud.show_help = true
		if frames == 370:
			_shot("game_help")
			main.hud.show_help = false
			# jump next to Jupiter for a planet picture
			var j: Planet = w.planet_by_name["Jupiter"]
			p.global_position = j.global_position + Vector2(-700, 300)
			p.velocity = j.velocity
			w.camera.reset_smoothing()
		if frames == 390:
			_shot("game_jupiter")
			var e: Planet = w.planet_by_name["Earth"]
			p.global_position = e.global_position + Vector2(-350, 200)
			p.velocity = e.velocity
			w.camera.reset_smoothing()
		if frames == 410:
			_shot("game_earth")
			main._open_pause()
		if frames == 420:
			_shot("game_pause")
			main._close_pause()
		if frames == 430:
			quit()
	return false
