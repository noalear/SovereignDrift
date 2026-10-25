# Screenshot of the three empires' supply ships side by side, for checking
# the Freighter paintings, their engines and the turret on each.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/supply_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var shown: Array = []
var spot := Vector2.ZERO


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _hold(s: Ship, at: Vector2, thrust: bool) -> void:
	for k in s.inputs.keys():
		s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
	s.inputs["thrust"] = 1.0 if thrust else 0.0
	if not thrust:
		s.global_position = at
		s.velocity = Vector2.ZERO
		s.rotation = 0.0
		s.body.pos = at
		s.body.vel = Vector2.ZERO
		s.body.angle = 0.0
	s.invulnerable = 1e9


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_game()
	if frames > 30 and main.world != null and main.world.player != null:
		var w: World = main.world
		var p: Ship = w.player
		if frames == 40:
			# out past Neptune, away from the war
			spot = w.planet_by_name["Neptune"].global_position + Vector2(0, 4000)
			for r in 3:
				shown.append(w._make_ship("Supply %d" % r, r, "Freighter", "Supply %d" % r, false))
			w.target_zoom = 0.75
		if frames > 40:
			_hold(p, spot + Vector2(-330, 0), false)
			for r in shown.size():
				_hold(shown[r], spot + Vector2(150, -260 + r * 260), frames > 150)
		if frames == 162:
			var img := root.get_viewport().get_texture().get_image()
			img.save_png(out_dir + "/supply_ships.png")
			print("saved supply_ships")
			return true
	return false
