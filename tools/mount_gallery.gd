# One big picture per hull painting with every kind of part fitted at once:
# a gun on every mount, the reactor, warp drive, sensor, jammer, repair bay,
# regenerator, four booster pods and the shield emitters and PDCs filling
# every outer socket, engines lit.  For checking where each part sits on
# the painting.  SF_MARKS=1 also rings and numbers every weapon mount
# (red), booster socket (gold) and outer socket (cyan).  SF_HD=0 draws the
# game-size paintings instead of the shipyard's high-resolution ones.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/mount_gallery.gd
# Writes <SF_SHOTS>/<empire>_<hull>.png (default /tmp/claude-0/sf/mounts).
extends SceneTree

const HULLS := ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought", "Freighter", "Trader", "DropShip"]
const ORDER := [[1, "terran"], [0, "arcean"], [2, "drengin"], [3, "pirate"]]
const EXTRA := ["9101", "5201", "5101", "9201", "9201", "9201", "9201", "5002", "5002", "4022", "4022", "4022", "4022", "4022", "4022"]

var out_dir := "/tmp/claude-0/sf/mounts"
var canvas: Node2D
var jobs := []
var job := -1
var frames := 0


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	var only := OS.get_environment("SF_ONLY")
	for r in ORDER:
		for h in HULLS:
			if r[0] == 3 and h in ["Freighter", "Trader", "DropShip"]:
				continue
			var hull: String = "Raider" if r[0] == 3 and h == "Destroyer" else h
			if only != "" and not ("%s_%s" % [r[1], hull]) in only.split(","):
				continue
			jobs.append([r[0], r[1], hull])
	ShipArt.bake_now = true
	ShipArt.hd = OS.get_environment("SF_HD") != "0"
	canvas = Node2D.new()
	canvas.draw.connect(_draw_job)
	root.add_child.call_deferred(canvas)


func _process(_delta: float) -> bool:
	frames += 1
	if frames % 4 != 0:
		return false
	if job >= 0:
		var img := root.get_viewport().get_texture().get_image()
		img.save_png("%s/%s_%s.png" % [out_dir, jobs[job][1], jobs[job][2]])
	job += 1
	if job >= jobs.size():
		return true
	canvas.queue_redraw()
	return false


func _draw_job() -> void:
	if job < 0 or job >= jobs.size():
		return
	var gd := root.get_node("GameData")       # -s scripts can't name autoloads
	var race: int = jobs[job][0]
	var hull: String = jobs[job][2]
	var vp := Vector2(root.get_visible_rect().size)
	canvas.draw_rect(Rect2(Vector2.ZERO, vp), Color(0.03, 0.035, 0.06))
	var design: Dictionary = gd.stock_design(hull, race)
	var mods := []
	for id in design["modules"]:
		if not gd.MODULES[id]["func"] in ["boost", "pdc", "shield", "jammer", "repair", "regen"]:
			mods.append(id)
	design["modules"] = mods + EXTRA
	var size: float = design["size"]
	var tex := ShipArt.hull_texture(hull, race)
	var aspect := 0.4 if tex == null else float(tex.get_height()) / tex.get_width()
	var length := size * ShipArt.LENGTH
	var scale := minf(vp.x * 0.8 / length, (vp.y - 120.0) / (length * aspect))
	var centre := Vector2(vp.x * 0.54, vp.y * 0.5 + 20)
	ShipArt.hd_width = int(ceil(length * scale / 32.0)) * 32
	ShipArt.draw_ship(canvas, design, race, size, gd.race(race)["color"], 0.6, 1.0, 0.0, Transform2D(0.0, Vector2.ONE * scale, 0.0, centre))
	var font := ThemeDB.fallback_font
	canvas.draw_string(font, Vector2(24, 44), "%s %s" % [gd.race(race)["adj"], gd.hull_label(hull)], HORIZONTAL_ALIGNMENT_LEFT, -1, 30, Color(0.85, 0.9, 0.95))
	if OS.get_environment("SF_MARKS") != "1":
		return
	var sets := [[ShipArt.hardpoint_spots(hull, race, size), Color(1, 0.3, 0.3), "w"],
		[ShipArt.system_sockets(hull, race, size, "boost"), Color(1, 0.8, 0.2), "b"],
		[ShipArt.system_sockets(hull, race, size, "ext"), Color(0.3, 0.9, 1.0), "e"]]
	for s in sets:
		for i in s[0].size():
			var p: Vector2 = centre + s[0][i] * scale
			canvas.draw_arc(p, 14.0, 0, TAU, 24, s[1], 2.0, true)
			canvas.draw_string(font, p + Vector2(12, -12), "%s%d" % [s[2], i + 1], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, s[1])
