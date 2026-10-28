# Screenshots of point defence: the player in a Terran Battleship (two
# PDCs) with missiles and a torpedo coming in, its turrets firing, and the
# shipyard part line.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/pdc_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var shooter: Ship = null


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved " + name)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_free_flight()
	if frames <= 30 or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	p.invulnerable = 1e9
	if frames == 40:
		p.refit("Battleship")
		w.target_zoom = 1.6
		var at: Vector2 = p.global_position + Vector2(9000, 0)
		p.global_position = at
		p.body.pos = at
		p.rotation = -0.4
		p.body.angle = -0.4
		shooter = w.spawn_ship("Shooter", 2, "Cruiser", "Shooter", at + Vector2(1000, -300), p.velocity, -1)
		shooter.invulnerable = 1e9
	if frames > 40:
		for k in p.inputs.keys():
			p.inputs[k] = false if typeof(p.inputs[k]) == TYPE_BOOL else 0.0
		p.body.ang_vel = 0.0
		p.velocity = shooter.velocity
		p.body.vel = shooter.velocity
	if frames in [60, 75, 90, 100, 110, 120]:
		var gd = root.get_node("/root/GameData")
		var mid := "004" if frames == 90 else "008"
		var off := Vector2(0, (frames % 3 - 1) * 80.0)
		var from: Vector2 = shooter.global_position + off
		var dir: Vector2 = (p.global_position - from).normalized()
		w.spawn_missile(shooter, mid, from, shooter.velocity + dir * gd.MISSILES[mid]["fire_v"] * gd.LIGHT_SPEED, dir.angle(), p)
	var rounds := w.missiles.filter(func(m): return is_instance_valid(m) and m.missile_id == "022").size()
	if frames > 60 and frames < 400 and rounds >= 4 and not FileAccess.file_exists(out_dir + "/pdc_firing.png"):
		_shot("pdc_firing")
	if frames == 400:
		if not FileAccess.file_exists(out_dir + "/pdc_firing.png"):
			_shot("pdc_firing")
		main.open_builder(p, w.starbase_of(p.race_id))
	if frames == 430:
		_shot("pdc_shipyard")
		quit(0)
	return false
