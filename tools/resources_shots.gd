# Screenshots of the Resources update for a visual check: the main menu's
# campaign buttons, Earth's planet card with its stockpile, and the ledger.
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
			return true
	return false
