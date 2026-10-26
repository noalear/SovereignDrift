# Pictures of every combat hull for every empire with its stock loadout,
# for checking the hull paintings, the weapon sockets and the turrets on
# them: hulls.png (all empires on one sheet) and hulls_<empire>.png (one
# empire close up).
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/hull_gallery.gd
extends SceneTree

const HULLS := ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought"]
const ORDER := [[1, "Terran"], [0, "Arcean"], [2, "Drengin"], [3, "Pirate"]]

var out_dir := "/tmp/claude-0/sf/shots"
var canvas: Node2D
var page := -1
var frames := 0


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	canvas = Node2D.new()
	canvas.draw.connect(_draw_page)
	root.add_child.call_deferred(canvas)


func _process(_delta: float) -> bool:
	frames += 1
	if frames % 6 != 0:
		return false
	if page >= 0:
		var img := root.get_viewport().get_texture().get_image()
		var name := "hulls" if page == 0 else "hulls_%s" % ORDER[page - 1][1].to_lower()
		img.save_png(out_dir + "/" + name + ".png")
		print("saved ", name)
	page += 1
	if page > ORDER.size():
		return true
	canvas.queue_redraw()
	return false


func _hulls_for(race: int) -> Array:
	var out := []
	for h in HULLS:
		out.append("Raider" if race == 3 and h == "Destroyer" else h)
	return out


func _draw_page() -> void:
	if page < 0:
		return
	var vp := Vector2(root.get_visible_rect().size)
	canvas.draw_rect(Rect2(Vector2.ZERO, vp), Color(0.03, 0.035, 0.06))
	var font := ThemeDB.fallback_font
	if page == 0:
		var cw := vp.x / HULLS.size()
		var ch := vp.y / ORDER.size()
		for r in ORDER.size():
			var race: int = ORDER[r][0]
			var hulls := _hulls_for(race)
			for c in hulls.size():
				var centre := Vector2((c + 0.5) * cw, (r + 0.5) * ch + 8)
				_ship(hulls[c], race, centre, Vector2(cw, ch) * 0.86)
				canvas.draw_string(font, Vector2(c * cw + 8, r * ch + 18), "%s %s" % [ORDER[r][1], hulls[c]], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.8, 0.85, 0.9))
		return
	# one empire close up
	var race: int = ORDER[page - 1][0]
	var hulls := _hulls_for(race)
	var cw := vp.x / 3.0
	var ch := vp.y / 2.0
	for i in hulls.size():
		var c := i % 3
		var r := i / 3
		var centre := Vector2((c + 0.5) * cw, (r + 0.5) * ch + 10)
		_ship(hulls[i], race, centre, Vector2(cw, ch) * 0.9)
		canvas.draw_string(font, Vector2(c * cw + 10, r * ch + 24), "%s %s" % [ORDER[page - 1][1], hulls[i]], HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.8, 0.85, 0.9))


## Draw a hull's stock design scaled to fit `box`, nose right.
func _ship(hull: String, race: int, centre: Vector2, box: Vector2) -> void:
	var gd := root.get_node("GameData")       # -s scripts can't name autoloads
	var size: float = gd.SHIPS[hull]["size"]
	var tex := ShipArt.hull_texture(hull, race)
	var aspect := 0.4 if tex == null else float(tex.get_height()) / tex.get_width()
	var length := size * ShipArt.LENGTH
	var scale := minf(box.x / length, box.y / (length * aspect))
	var design: Dictionary = gd.stock_design(hull)
	ShipArt.draw_ship(canvas, design, race, size, gd.race(race)["color"], 0.5, 1.0, 0.0, Transform2D(0.0, Vector2.ONE * scale, 0.0, centre))
