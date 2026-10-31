# Screenshots of the drive plumes and gun convergence: a Cruiser thrusting
# ahead (main plume), reversing (the bow's retro jets) and firing with
# every gun on the cursor.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/retro_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"


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


func _hold(p: Ship, v: Vector2) -> void:
	p.global_position = Vector2(30000, 30000)
	p.body.pos = p.global_position
	p.velocity = v
	p.body.vel = v
	p.rotation = -0.5
	p.body.angle = -0.5
	p.body.ang_vel = 0.0
	p.fuel = p.fuel_max
	p.energy = p.energy_max


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
		p.refit(root.get_node("/root/GameData").stock_design("Cruiser", p.race_id))
		w.target_zoom = 2.2
	if frames > 40 and frames < 260:
		_hold(p, Vector2.ZERO)
	if frames == 60:
		Input.action_press("thrust")
	if frames == 100:
		_shot("plume_ahead")
		Input.action_release("thrust")
	if frames == 130:
		Input.action_press("reverse")
	if frames == 170:
		_shot("plume_reverse")
		Input.action_release("reverse")
	if frames == 190:
		w.target_zoom = 1.3
		var vp := root.get_viewport().get_visible_rect().size
		root.get_viewport().warp_mouse(vp * 0.5 + Vector2.RIGHT.rotated(-0.5) * 360.0 + Vector2(60, 90))
		Input.action_press("fire_primary")
		Input.action_press("fire_secondary")
	if frames == 236:
		_shot("guns_converge")
		Input.action_release("fire_primary")
		Input.action_release("fire_secondary")
		quit(0)
	return false
