# Every yard hull of every empire with boosters, shield emitters and PDCs
# in their sockets, and the empty sockets ringed: a check that the places
# make sense on each painting.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/socket_sheet.gd
# SF_SHOTS=<dir> picks the folder (default /tmp/claude-0/sf/sockets);
# SF_HULLS=Destroyer,Cruiser draws only those, bigger.
extends SceneTree

var out_dir := "/tmp/claude-0/sf/sockets"
var frames := 0
var page := 0
var canvas: Node2D


class Sheet extends Node2D:
	var race := 0
	func _draw() -> void:
		var gd = Engine.get_main_loop().root.get_node("/root/GameData")
		draw_rect(Rect2(0, 0, 1920, 1080), Color(0.02, 0.03, 0.06))
		var font := ThemeDB.fallback_font
		var hulls: Array = gd.yard_hulls()
		if OS.has_environment("SF_HULLS"):
			hulls = OS.get_environment("SF_HULLS").split(",")
		var cols := 3 if hulls.size() > 4 else (2 if hulls.size() > 1 else 1)
		var cw := 1920.0 / cols
		var ch := 1080.0 / ceili(hulls.size() / float(cols))
		for i in hulls.size():
			var name: String = hulls[i]
			var c := Vector2(cw * (i % cols) + cw * 0.5, ch * (i / cols) + ch * 0.5 + 10)
			var d: Dictionary = gd.stock_design(name, race)
			for id in ["9201", "9202", "5001", "5001", "4022", "4022"]:
				d["modules"].append(id)
			var size: float = d["size"]
			var scale := minf(ch * 0.32 / size, cw * 0.27 / size)
			var xf := Transform2D(0.0, Vector2.ONE * scale, 0.0, c)
			ShipArt.draw_ship(self, d, race, size, Color(0.6, 0.8, 1.0), 0.5, 1.0, 0.0, xf)
			draw_set_transform_matrix(Transform2D.IDENTITY)
			for kind in ["ext", "boost"]:
				var col := Color(1, 0.85, 0.3) if kind == "boost" else Color(0.5, 0.9, 1.0)
				var socks: Array = ShipArt.system_sockets(name, race, size, kind)
				for k in socks.size():
					var p: Vector2 = c + socks[k] * scale
					draw_arc(p, 7.0, 0, TAU, 16, col, 1.5, true)
					draw_string(font, p + Vector2(6, -6), str(k), HORIZONTAL_ALIGNMENT_LEFT, -1, 10, col)
			draw_string(font, Vector2(cw * (i % cols) + 12, ch * (i / cols) + 20), "%s  (%s)" % [name, gd.race(race)["adj"]], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color.WHITE)


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	call_deferred("_start")


func _start() -> void:
	canvas = Sheet.new()
	root.add_child(canvas)


func _process(_delta: float) -> bool:
	frames += 1
	if frames % 20 == 0:
		var img := root.get_viewport().get_texture().get_image()
		img.save_png("%s/race_%d.png" % [out_dir, canvas.race])
		print("saved race ", canvas.race)
		canvas.race += 1
		if canvas.race >= root.get_node("/root/GameData").RACES.size():
			return true
		canvas.queue_redraw()
	return false
