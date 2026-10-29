## StoryPanel -- the operation on the screen: its objectives on the right,
## under the target readout, and a ring (or an edge arrow) on whatever an
## objective points at.  Between operations it says when the next briefing
## is ready.
class_name StoryPanel
extends Control

const W := 320.0

var story: Story
var main: Node
var font: Font
var time := 0.0


func setup(p_story: Story, p_main: Node) -> void:
	story = p_story
	main = p_main


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)


func _process(delta: float) -> void:
	time += delta
	queue_redraw()


func _covered() -> bool:
	if main == null:
		return false
	return main.get("builder") != null or main.get("tactical") != null or main.get("ledger") != null \
		or main.get("briefing") != null or main.get("scoreboard") != null or main.get("pause_panel") != null


func _draw() -> void:
	if story == null or _covered():
		return
	var w: Node = story.world
	if w == null or w.player == null:
		return
	var vp := get_viewport_rect().size
	_draw_marks(vp)
	_draw_list(vp)


func _draw_list(vp: Vector2) -> void:
	var x := vp.x - W - 20.0
	var y := 212.0
	var key := Keybinds.short_text("briefing")
	var title := ""
	var rows: Array = []            # [text, state] state: open | done | failed | optional | note
	if story.op_title() != "":
		title = "OPERATION  ·  %s" % story.op_title().to_upper()
		for o in story.objectives():
			var st := "done" if o["done"] else ("failed" if o["failed"] else ("optional" if o["optional"] else "open"))
			rows.append([o["text"], st])
	elif story.failed != "":
		title = "OPERATION FAILED"
		rows.append([story.failed, "failed"])
		rows.append(["The host tries it again from the briefing" if story.mirror else "Press %s to try again from the briefing" % key, "note"])
	elif story.briefing_ready():
		title = "BRIEFING READY"
		rows.append(["Operation %s. Press %s" % [story.current()["title"], key], "open"])
	elif not story.current().is_empty() and story.unlock_at < INF:
		# between operations: say what comes next and when
		var left := int(ceilf(maxf(story.unlock_at - story.world.elapsed, 0.0)))
		title = "NEXT OPERATION"
		rows.append(["%s. Fleet calls you in %d:%02d" % [story.current()["title"], left / 60, left % 60], "note"])
	else:
		return
	var lines := []
	for r in rows:
		for ln in _wrap(r[0], W - 40.0, 15):
			lines.append([ln, r[1]])
		lines.append(["", "gap"])
	var h := 38.0 + lines.size() * 19.0
	draw_rect(Rect2(x - 10, y, W + 10, h), Color(0.02, 0.04, 0.08, 0.72))
	draw_rect(Rect2(x - 10, y, 3, h), Color(0.55, 0.8, 1.0))
	_text(Vector2(x + 4, y + 24), title, 14, Color(0.6, 0.85, 1.0) if story.failed == "" else Color(1.0, 0.55, 0.45))
	var ly := y + 48.0
	var first := true
	for l in lines:
		if l[1] == "gap":
			ly += 5.0
			first = true
			continue
		var col := Color(0.95, 0.95, 0.9)
		match l[1]:
			"done":
				col = Color(0.5, 0.9, 0.6, 0.85)
			"failed":
				col = Color(1.0, 0.5, 0.4)
			"optional":
				col = Color(0.7, 0.85, 0.75)
			"note":
				col = Color(0.75, 0.8, 0.9)
			"open":
				col = Color(1.0, 0.88, 0.5)
		if first:
			var m := Vector2(x + 8, ly - 5)
			if l[1] == "done":
				draw_polyline(PackedVector2Array([m + Vector2(-5, 0), m + Vector2(-1, 4), m + Vector2(6, -5)]), col, 2.0)
			elif l[1] == "failed":
				draw_line(m + Vector2(-4, -4), m + Vector2(4, 4), col, 2.0)
				draw_line(m + Vector2(-4, 4), m + Vector2(4, -4), col, 2.0)
			elif l[1] != "note":
				if l[1] == "open":
					draw_rect(Rect2(m - Vector2(4, 4), Vector2(8, 8)), col)
				else:
					draw_rect(Rect2(m - Vector2(4, 4), Vector2(8, 8)), col, false, 1.5)
		_text(Vector2(x + 22, ly), l[0], 15, col)
		ly += 19.0
		first = false


## What an objective points at: a ring on it, or an arrow at the edge.
func _draw_marks(vp: Vector2) -> void:
	var w: Node = story.world
	var p: Ship = w.player
	if p.dead:
		return
	var xf: Transform2D = w.get_viewport().get_canvas_transform()
	for m in story.marks():
		var sp: Vector2 = xf * m[0]
		var col: Color = m[2]
		var label := "%s  %d" % [m[1], int(p.global_position.distance_to(m[0]))]
		var inner := Rect2(Vector2(60, 60), vp - Vector2(120, 120))
		if inner.has_point(sp):
			var r := 30.0 + 4.0 * sin(time * 4.0)
			draw_arc(sp, r, 0, TAU, 48, Color(col.r, col.g, col.b, 0.7), 1.5)
			draw_arc(sp, r + 6.0, time * 1.5, time * 1.5 + PI * 0.5, 16, col, 2.0)
			draw_arc(sp, r + 6.0, time * 1.5 + PI, time * 1.5 + PI * 1.5, 16, col, 2.0)
			draw_string_outline(font, sp + Vector2(-110, -r - 12.0), label, HORIZONTAL_ALIGNMENT_CENTER, 220, 13, 4, Color(0, 0, 0, 0.7))
			draw_string(font, sp + Vector2(-110, -r - 12.0), label, HORIZONTAL_ALIGNMENT_CENTER, 220, 13, col)
			continue
		var centre := vp * 0.5
		var dir := (sp - centre).normalized()
		var half := vp * 0.5 - Vector2(80, 80)
		var k := minf(absf(half.x / maxf(absf(dir.x), 0.001)), absf(half.y / maxf(absf(dir.y), 0.001)))
		var at := centre + dir * (k + 3.0 * sin(time * 5.0))
		var side := dir.orthogonal()
		draw_colored_polygon(PackedVector2Array([at + dir * 18.0, at - dir * 6.0 + side * 11.0, at - dir * 6.0 - side * 11.0]), col)
		draw_string_outline(font, at - dir * 36.0 + Vector2(-90, 5), label, HORIZONTAL_ALIGNMENT_CENTER, 180, 13, 4, Color(0, 0, 0, 0.7))
		draw_string(font, at - dir * 36.0 + Vector2(-90, 5), label, HORIZONTAL_ALIGNMENT_CENTER, 180, 13, col)


func _wrap(text: String, width: float, size: int) -> PackedStringArray:
	var out := PackedStringArray()
	var cur := ""
	for word in text.split(" "):
		var t := word if cur == "" else cur + " " + word
		if cur != "" and font.get_string_size(t, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x > width:
			out.append(cur)
			cur = word
		else:
			cur = t
	if cur != "":
		out.append(cur)
	return out


func _text(pos: Vector2, text: String, size: int, col: Color) -> void:
	draw_string_outline(font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, 4, Color(0, 0, 0, 0.75 * col.a))
	draw_string(font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, col)
