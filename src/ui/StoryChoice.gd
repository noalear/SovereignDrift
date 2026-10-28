## StoryChoice -- a story choice put to the player (Story.ask): the
## question and a button for each answer with what it means.  Offline the
## war holds still while it is open; the call window keeps talking beside
## it.  There is no "not now": the operation waits on the answer.
class_name StoryChoice
extends Control

signal chosen(key: String)

var story: Story
var font: Font
var time := 0.0
var buttons: Array = []           # [Button, option row]


func setup(p_story: Story) -> void:
	story = p_story


func _ready() -> void:
	font = ThemeDB.fallback_font
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	for row in story.choice.get("options", []):
		var b := Button.new()
		b.text = str(row[1])
		b.add_theme_font_size_override("font_size", 20)
		b.process_mode = Node.PROCESS_MODE_ALWAYS
		var key: String = row[0]
		b.pressed.connect(func(): chosen.emit(key))
		add_child(b)
		buttons.append([b, row])
	if not buttons.is_empty():
		buttons[0][0].grab_focus()
	resized.connect(queue_redraw)


func _panel() -> Rect2:
	var vp := get_viewport_rect().size
	var w := minf(680.0, vp.x - 380.0)
	var h := 150.0 + buttons.size() * 112.0
	return Rect2(maxf(vp.x * 0.5 - w * 0.5, 360.0), vp.y * 0.5 - h * 0.5, w, h)


func _process(delta: float) -> void:
	time += delta
	var r := _panel()
	for i in buttons.size():
		var b: Button = buttons[i][0]
		b.position = Vector2(r.position.x + 24.0, r.position.y + 110.0 + i * 112.0)
		b.size = Vector2(r.size.x - 48.0, 46.0)
	queue_redraw()


func _draw() -> void:
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.01, 0.03, 0.55))
	var r := _panel()
	draw_rect(r, Color(0.02, 0.04, 0.08, 0.95))
	draw_rect(r, Color(1.0, 0.85, 0.45, 0.5 + 0.2 * sin(time * 3.0)), false, 2.0)
	draw_string(font, r.position + Vector2(24, 36), "YOUR DECISION", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1.0, 0.85, 0.5))
	var y := r.position.y + 72.0
	for ln in _wrap(str(story.choice.get("question", "")), r.size.x - 48.0, 22):
		draw_string(font, Vector2(r.position.x + 24, y), ln, HORIZONTAL_ALIGNMENT_LEFT, -1, 22, Color(0.95, 0.97, 1.0))
		y += 28.0
	for i in buttons.size():
		var row: Array = buttons[i][1]
		var ty := r.position.y + 110.0 + i * 112.0 + 66.0
		for ln in _wrap(str(row[2]), r.size.x - 48.0, 15):
			draw_string(font, Vector2(r.position.x + 28, ty), ln, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.75, 0.82, 0.92))
			ty += 19.0


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
