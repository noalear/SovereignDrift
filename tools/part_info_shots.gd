# Pictures of the shipyard's mouse-over descriptions for a visual check:
# a weapon in the parts list, a mount on the ship, a performance line, a
# hull and a bay part.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/part_info_shots.gd
# SF_SHOTS=<dir> picks the folder (default /tmp/claude-0/sf/part_info).
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/part_info"
var plan := []          # [frame, focus key, picture name]


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


## Point at `key` as the mouse would: the button's index becomes the hover.
func _point(b, key: String) -> void:
	b.kb_nav = false
	b.hover = -1
	for i in b.buttons.size():
		if b.buttons[i][3] == key:
			b.hover = i
	b.focus_key = key


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_free_flight()
	var w = main.world
	if w == null or w.player == null:
		return false
	var p: Ship = w.player
	if frames == 60:
		var base: Ship = w.starbase_of(p.race_id)
		p.global_position = base.global_position + Vector2(base.size + 80, 0)
		p.velocity = base.velocity
		p.credits = 6000
		p.rank = 7
		w.try_refit(p)
	if main.builder == null:
		return frames > 600
	var b = main.builder
	match frames:
		80:
			b._select("hp", 0)
			b.cat_mode = "switch"
			b.cat_type = "Guns"
		90:
			_point(b, "part:4010")
		140:
			_shot("tip_part_gatling")
			_point(b, "hp:0")
		190:
			_shot("tip_mount")
			_point(b, "stat:accel")
		240:
			_shot("tip_stat_accel")
			_point(b, "stat:em")
		290:
			_shot("tip_stat_emissions")
			_point(b, "hull:Cruiser")
		340:
			_shot("tip_hull")
			_point(b, "bay:1")
		390:
			_shot("tip_bay")
			b.kb_nav = true
			b.hover = -1
			b.focus_key = "stat:reactor"
		440:
			_shot("tip_keys_reactor")
			b._select("bay", 0)
			b.cat_mode = "switch"
			b.cat_type = "Armor"
		450:
			_point(b, "part:5506")
		500:
			_shot("tip_part_reactive")
			return true
	return false
