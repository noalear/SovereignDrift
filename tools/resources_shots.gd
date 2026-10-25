# Screenshots of the Resources update for a visual check: the main menu's
# campaign buttons, Earth's planet card with its stockpile, the ledger, the
# cargo hold on the ground, cargo crates floating in space and the help card.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/resources_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"


func _initialize() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")


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
	if frames == 60:
		_shot("res_menu")
		main._start_game()
	if frames > 60 and main.world != null and main.world.player != null:
		var w: World = main.world
		if frames == 200:
			for i in 600:
				for p in w.planets:
					if p.is_colonised():
						Economy.step(p, 1.0)
			var earth: Planet = w.planet_by_name["Earth"]
			w.inspected_planet = earth
			w.player.global_position = earth.global_position + Vector2(-380, 260)
			w.player.body.pos = w.player.global_position
			w.player.velocity = earth.velocity
			w.player.body.vel = earth.velocity
		if frames == 260:
			_shot("res_planet_card")
			w.inspected_planet = w.planet_by_name["Jupiter"]
		if frames == 262:
			var lg := Ledger.new()
			lg.world = w
			main.ui_layer.add_child(lg)
			main.ledger = lg
		if frames == 280:
			_shot("res_ledger")
			main._close_ledger()
			w.inspected_planet = null
			var p: Ship = w.player
			p.refit("Freighter")
			var earth: Planet = w.planet_by_name["Earth"]
			var dir := (earth.global_position - w.sun.global_position).normalized().rotated(0.7)
			p.global_position = earth.global_position + dir * (earth.radius + 90.0)
			p.velocity = earth.velocity
			p.body.pos = p.global_position
			p.body.vel = p.velocity
			p.invulnerable = 1e9
			w.try_land(p)
		if frames == 420:
			var p: Ship = w.player
			p.cargo = {"food": 120.0, "metals": 35.0}
			p.cargo_origin = w.planet_by_name["Luna"]
			var ev := InputEventAction.new()
			ev.action = "cargo"
			ev.pressed = true
			main._unhandled_input(ev)
		if frames == 440:
			_shot("res_cargo")
			main._close_cargo()
			w.try_land(w.player)
		if frames == 560:
			var p: Ship = w.player
			for i in 5:
				var k: String = Economy.RESOURCES[i]
				w.spawn_powerup("rs:%s:%d" % [k, 10 + i * 7], p.global_position + Vector2(120 + i * 50, -80 + i * 30), p.velocity)
		if frames == 600:
			_shot("res_crates")
			main.hud.show_help = true
		if frames == 620:
			_shot("res_help")
			return true
	return false
