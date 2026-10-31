# Screenshots of the nebula backdrop in open space: the same spot at three
# zooms, then after a 6500-unit flight (the sky should barely slide).
#   SF_SHOTS=<dir> xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/backdrop_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/backdrop"
var spot := Vector2.ZERO


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	call_deferred("_start")


func _start() -> void:
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved ", name)


func _park(w: World, p: Ship, at: Vector2) -> void:
	p.global_position = at
	p.velocity = Vector2.ZERO
	p.invulnerable = 60.0
	for s in w.ships:
		if s != p and not s.is_station and s.global_position.distance_to(at) < 3000.0:
			s.global_position = at + Vector2(9000, 9000)
	w.camera.reset_smoothing()


func _zoom(w: World, z: float) -> void:
	w.target_zoom = z
	w.camera.zoom = Vector2.ONE * z * World.BASE_ZOOM


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 60:
		main._start_free_flight()
	if main.world == null or main.world.player == null:
		return frames > 3000
	var w: World = main.world
	var p: Ship = w.player
	p.inputs["thrust"] = 0.0
	if frames == 120:
		# deep space between Mars and Jupiter, away from bodies
		var mars: Planet = w.planet_by_name["Mars"]
		spot = mars.global_position * 1.6
		_park(w, p, spot)
		_zoom(w, 1.0)
	if frames == 150:
		_park(w, p, spot)
		_zoom(w, 1.0)
	if frames == 170:
		_shot("zoom_normal")
		_zoom(w, 1.6)
	if frames == 190:
		_shot("zoom_in")
		_zoom(w, 0.35)
	if frames == 210:
		_shot("zoom_out")
		_zoom(w, 1.0)
		_park(w, p, spot + Vector2(6000, 2500))
	if frames == 240:
		_shot("moved_6000")
	if frames == 270:
		quit()
	return false
