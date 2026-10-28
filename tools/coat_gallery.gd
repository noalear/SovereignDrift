# Pictures of the armor coats and shield families, for checking how each
# looks on every empire's hulls: coats_<hull>.png (a row per coat, a column
# per empire) and shields.png (each family full, half charged, and being
# raised).  Also prints how long each coat takes to bake.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/coat_gallery.gd
extends SceneTree

const HULLS := ["Cruiser", "Scout", "Battleship", "Dreadnought"]
const ORDER := [[1, "Terran"], [0, "Arcean"], [2, "Drengin"], [3, "Pirate"]]
const COATS := ["", "5502", "5504", "5505", "5506", "5507", "5508"]

var out_dir := "/tmp/claude-0/sf/shots"
var canvas: Node2D
var page := -1
var frames := 0
var bubbles: Array = []


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	canvas = Node2D.new()
	canvas.draw.connect(_draw_page)
	root.add_child.call_deferred(canvas)


func _process(_delta: float) -> bool:
	ShipArt.bake_now = true          # (set here: the class reloads once the autoloads are up)
	frames += 1
	if frames % 8 != 0:
		return false
	if page >= 0:
		var img := root.get_viewport().get_texture().get_image()
		var name := "shields" if page >= HULLS.size() else "coats_%s" % HULLS[page].to_lower()
		img.save_png(out_dir + "/" + name + ".png")
		print("saved ", name)
	page += 1
	if page > HULLS.size():
		return true
	if page == HULLS.size():
		_shield_page()
	canvas.queue_redraw()
	return false


func _draw_page() -> void:
	if page < 0:
		return
	var vp := Vector2(root.get_visible_rect().size)
	canvas.draw_rect(Rect2(Vector2.ZERO, vp), Color(0.03, 0.035, 0.06))
	var font := ThemeDB.fallback_font
	var gd := root.get_node("GameData")       # -s scripts can't name autoloads
	if page >= HULLS.size():
		for b in bubbles:
			canvas.draw_string(font, b[1] + Vector2(-120, -110), b[2], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.8, 0.85, 0.9))
			_ship("Cruiser", 1, "5502", b[1], Vector2(300, 170))
		return
	var hull: String = HULLS[page]
	var label_w := 120.0
	var cw := (vp.x - label_w) / ORDER.size()
	var ch := vp.y / COATS.size()
	for r in COATS.size():
		var coat: String = COATS[r]
		canvas.draw_string(font, Vector2(8, (r + 0.5) * ch), "none" if coat == "" else gd.coat_label(coat), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.8, 0.85, 0.9))
		for c in ORDER.size():
			var race: int = ORDER[c][0]
			var h := "Raider" if race == 3 and hull == "Destroyer" else hull
			if r == 0:
				canvas.draw_string(font, Vector2(label_w + c * cw + 8, 16), "%s %s" % [ORDER[c][1], h], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.8, 0.85, 0.9))
			var t0 := Time.get_ticks_usec()
			_ship(h, race, coat, Vector2(label_w + (c + 0.5) * cw, (r + 0.5) * ch + 6), Vector2(cw, ch) * 0.9)
			var dt := (Time.get_ticks_usec() - t0) / 1000.0
			if dt > 5.0:
				print("bake %s %s %s: %.0f ms" % [ORDER[c][1], h, coat, dt])


## Draw a hull's stock design with coat `coat`, scaled to fit `box`, nose right.
func _ship(hull: String, race: int, coat: String, centre: Vector2, box: Vector2) -> void:
	var gd := root.get_node("GameData")
	var size: float = gd.SHIPS[hull]["size"]
	var tex := ShipArt.hull_texture(hull, race)
	var aspect := 0.4 if tex == null else float(tex.get_height()) / tex.get_width()
	var length := size * ShipArt.LENGTH
	var scale := minf(box.x / length, box.y / (length * aspect))
	var design: Dictionary = gd.stock_design(hull, race)
	var mods := []
	for id in design["modules"]:
		if gd.MODULES[id]["func"] != "armor":
			mods.append(id)
	if coat != "":
		mods.append(coat)
	design["modules"] = mods
	ShipArt.draw_ship(canvas, design, race, size, gd.race(race)["color"], 0.5, 1.0, 0.0, Transform2D(0.0, Vector2.ONE * scale, 0.0, centre))


## Three rows of shield bubbles (Deflector, Barrier, Flicker), each full,
## half charged, and halfway through being raised.
func _shield_page() -> void:
	var vp := Vector2(root.get_visible_rect().size)
	var shader: Shader = load("res://src/ships/shield.gdshader")
	var img := Image.create(4, 4, false, Image.FORMAT_RGBA8)
	img.fill(Color.WHITE)
	var white := ImageTexture.create_from_image(img)
	var fams := ["Deflector", "Barrier", "Flicker"]
	var states := [["full", 1.0, 0.0], ["half", 0.45, 0.0], ["raising", 0.0, 0.5]]
	for f in 3:
		for k in 3:
			var centre := Vector2((k + 0.5) * vp.x / 3.0, (f + 0.5) * vp.y / 3.0)
			bubbles.append([null, centre, "%s, %s" % [fams[f], states[k][0]]])
			var spr := Sprite2D.new()
			spr.texture = white
			spr.scale = Vector2(220, 110) / 4.0 * 2.0
			spr.position = centre
			spr.z_index = 1
			var mat := ShaderMaterial.new()
			mat.shader = shader
			mat.set_shader_parameter("aspect", Vector2(1.0, 0.5))
			mat.set_shader_parameter("family", f)
			mat.set_shader_parameter("strength", states[k][1])
			mat.set_shader_parameter("strength_front", states[k][1])
			mat.set_shader_parameter("strength_rear", states[k][1])
			mat.set_shader_parameter("boot", states[k][2])
			var hits := PackedVector4Array()
			for i in 4:
				hits.append(Vector4(0.9, -0.2, 0.18, 1.0) if i == 0 and k == 0 else Vector4(0, 0, 9.0, 0.0))
			mat.set_shader_parameter("hits", hits)
			spr.material = mat
			canvas.add_child(spr)
