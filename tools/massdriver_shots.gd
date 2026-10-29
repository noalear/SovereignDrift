# Screenshots of the mass driver: a Terran Destroyer firing its mass
# drivers at a drone (muzzle flash, slugs in flight) and the shipyard
# with the new turret on mount 1.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/massdriver_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var target: Ship = null


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
		p.refit("Destroyer")
		var at: Vector2 = p.global_position + Vector2(9000, 0)
		p.global_position = at
		p.body.pos = at
		p.rotation = 0.0
		p.body.angle = 0.0
		target = w.spawn_ship("Drone", 2, "Scout", "Drone", at + Vector2(420, -60), p.velocity, -1)
		target.invulnerable = 1e9
	if frames > 40:
		for k in p.inputs.keys():
			p.inputs[k] = false if typeof(p.inputs[k]) == TYPE_BOOL else 0.0
		p.body.ang_vel = 0.0
		p.rotation = 0.0
		p.body.angle = 0.0
		p.velocity = target.velocity
		p.body.vel = target.velocity
		p.energy = 1e6
		w.target_zoom = 3.0 if frames < 300 else 1.3
		root.warp_mouse(w.get_viewport().get_canvas_transform() * target.global_position)
	# fire every mass driver now and then
	if frames > 60 and frames < 300 and frames % 24 == 0:
		for wp in p.weapons:
			if wp.get("missile", "") == "001":
				wp["cooldown"] = 0.0
				p._fire_tube(wp)
	if frames == 98:
		_shot("massdriver_flash")
	if frames == 101:
		_shot("massdriver_slugs")
	if frames == 300:
		main.open_builder(p, w.starbase_of(p.race_id))
	if frames == 330:
		_shot("massdriver_shipyard")
		quit(0)
	return false
