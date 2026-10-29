# Pictures of the radar and the Tab map, to check the ship darts point along
# each ship's course.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/radar_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/radar"


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
		main._start_game()
	if frames >= 70 and main.world != null and main.world.player != null:
		var w: World = main.world
		var p: Ship = w.player
		p.inputs["thrust"] = 1.0 if frames < 300 else 0.0
		p.inputs["turn"] = 0.6 if frames > 250 else 0.0
		if frames == 200:
			main.hud.radar_range = 2500.0
			# a few ships close by, each heading its own way
			var n := 0
			for s in w.ships:
				if s == p or s.is_station or s.dead:
					continue
				var a := n * TAU / 6.0
				s.global_position = p.global_position + Vector2.RIGHT.rotated(a) * 800.0
				s.velocity = p.frame_velocity() + Vector2.RIGHT.rotated(a + 1.2) * 200.0
				n += 1
				if n == 6:
					break
		if frames == 320:
			_shot("radar")
		if frames == 330:
			var ev := InputEventAction.new()
			ev.action = "tactical_map"
			ev.pressed = true
			Input.parse_input_event(ev)
		if frames == 345:
			_shot("map")
	if frames > 360:
		return true
	return false
