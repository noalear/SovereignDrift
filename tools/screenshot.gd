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
		if frames == 425:
			# the shipyard, with some salvage and credits to show
			var base: Ship = null
			for s in w.ships:
				if s.is_station and s.race_id == p.race_id:
					base = s
			p.global_position = base.global_position + Vector2(200, 0)
			p.velocity = base.velocity
			p.credits = 3200
			p.hold = ["5502", "4003", "9202"]
			p.score = 1200
			p.kills = 160
			p.rank = 7
			w.try_refit(p)
		if frames == 440:
			_shot("shipyard")
			main.builder._switch_hull("Cruiser")
			main.builder._select("hp", 2)
		if frames == 455:
			_shot("shipyard_cruiser")
			main.builder._mount("4003")
			main.builder._select("bay", main.builder.modules.size())
			main.builder._mount("5502")
			main.builder._launch()
		if frames == 470:
			_shot("game_rebuilt")
			var ev := InputEventAction.new()
			ev.action = "tactical_map"
			ev.pressed = true
			main._unhandled_input(ev)
		if frames == 490:
			_shot("tactical_map")
			var ev := InputEventAction.new()
			ev.action = "tactical_map"
			ev.pressed = true
			main._unhandled_input(ev)
			# end the round: the player's empire takes every colony
			var rd: Round = w.round_rules
			rd.elapsed = rd.length - 120.0
			for pl in w.planets:
				if pl.is_colonised() and pl.owner_race != p.race_id:
					w.planet_captured(pl, p.race_id, p)
			rd.hold_race = p.race_id
			rd.hold_timer = Round.HOLD_SECONDS - 6.0
		if frames == 500:
			_shot("game_final_phase")
		if frames == 520:
			# fly near the Sun to show the heat warning
			var sun: Planet = w.planet_by_name["Sol"]
			p.global_position = sun.global_position + Vector2(sun.radius * 1.9, 0)
			p.velocity = Vector2.ZERO
			p.invulnerable = 30.0
			w.camera.reset_smoothing()
		if frames == 560:
			_shot("game_sun")
			p.global_position = w.planet_by_name["Earth"].global_position + Vector2(600, 0)
			w.camera.reset_smoothing()
	if frames == 1250:
		_shot("scoreboard")
	if frames == 1260:
		quit()
	return false
