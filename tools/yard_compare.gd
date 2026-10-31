# The shipyard slip close up, one hull after another, for checking how
# sharp the painting is and where its parts sit: <SF_SHOTS>/yard_<empire>_<hull>.png,
# cropped to the slip.  SF_YARD=drengin:Freighter,terran:Dreadnought picks
# the hulls (default: every warship and support hull of the three empires).
# Run at the window size to check, e.g. a 4K screen:
#   xvfb-run -a -s "-screen 0 3840x2160x24" godot --path . --rendering-driver opengl3 --resolution 3840x2160 -s tools/yard_compare.gd
extends SceneTree

const RACES := {"arcean": 0, "terran": 1, "drengin": 2}

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/yard"
var jobs := []
var job := -1


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	var pick := OS.get_environment("SF_YARD")
	if pick == "":
		for r in ["terran", "arcean", "drengin"]:
			for h in ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought", "Freighter", "Trader", "DropShip"]:
				jobs.append([r, h])
	else:
		for p in pick.split(","):
			jobs.append(p.split(":"))
	call_deferred("_start")


func _start() -> void:
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


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
		p.credits = 99999
		p.rank = 14
		w.try_refit(p)
	if main.builder == null:
		return frames > 400
	var b = main.builder
	if frames < 80 or frames % 12 != 0:
		return false
	if job >= 0:
		var img := root.get_viewport().get_texture().get_image()
		var k := img.get_width() / maxf(root.get_visible_rect().size.x, 1.0)
		var r: Rect2 = b._pic_rect
		img = img.get_region(Rect2i(Vector2i(r.position * k), Vector2i(r.size * k)))
		img.save_png("%s/yard_%s_%s.png" % [out_dir, jobs[job][0], jobs[job][1]])
	job += 1
	if job >= jobs.size():
		return true
	b.art_race = RACES[jobs[job][0]]
	b._switch_hull(jobs[job][1])
	b.sel_kind = ""
	b.sel_index = -1
	return false
