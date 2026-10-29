# Screenshot of the wing panel with a battered prize, a healthy wingman and a lost one.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/wing_hud_shot.gd
extends SceneTree

var main: Node


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	for k in 20:
		await process_frame
	var w: World = main.world
	var p: Ship = w.player
	var hulls := [0.04, 1.0, 0.6]
	for k in 3:
		var s := w._make_ship("Wing %d" % k, p.race_id, ["Scout", "Cruiser", "Destroyer"][k], "Mate %d" % k, false)
		var ai := AIPilot.new()
		ai.name = "AI"
		s.add_child(ai)
		ai.setup(s, w, null, 3)
		ai.join(p, false)
		s.global_position = p.global_position + Vector2(-200 - k * 120, 150)
		s.hull = s.hull_max * hulls[k]
		if k == 0:
			s.is_prize = true
	for k in 90:
		await process_frame
	var img := root.get_viewport().get_texture().get_image()
	img.save_png("/tmp/claude-0/scratch/wing_hud.png")
	print("saved")
	quit(0)
