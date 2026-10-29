# Pictures of outfitting a wingman: the yard's ship tabs, a wingman's ship
# with its purse and the pilot's credits, and the bill split between them.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/wing_outfit_shots.gd
# SF_SHOTS=<dir> picks the folder (default /tmp/claude-0/sf/wing_outfit).
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/wing_outfit"


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
		p.credits = 2400
		p.rank = 7
		for k in 2:
			var wm: Ship = w.spawn_ship(["Lt. Brandt", "Lt. Vance"][k], p.race_id, "Scout", ["Lantern Two", "Lantern Three"][k],
				base.global_position + Vector2(base.size + 300, 200 - 400 * k), base.velocity, 6)
			wm.rank = 6
			wm.credits = [450, 260][k]
			wm.get_node("AI").join(p, true)
		w.try_refit(p)
	if main.builder == null:
		return frames > 400
	var b = main.builder
	if frames == 80:
		_shot("tabs_own_ship")
		b._command("ship_next")
	if frames == 95:
		_shot("tabs_wingman")
		b._switch_hull("Destroyer")
		b._give(100)
	if frames == 110:
		_shot("tabs_wingman_destroyer")
		b._command("launch")
	if frames == 125:
		_shot("tabs_wingman_bill")
		b._answer(0)
	if frames == 140:
		_shot("tabs_back_home")
		b._leave()
	return frames > 160
