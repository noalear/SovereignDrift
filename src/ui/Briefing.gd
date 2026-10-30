## Briefing -- the operation's briefing, between operations: Commander Hale
## on a big call from the ops room talks it through, beside the operation's
## name, place and orders.  Launch starts it; Not yet goes back to the war
## (the briefing waits).  Offline the war holds still while it is open.
## Online only the host launches; a friend can read the briefing.
class_name Briefing
extends Control

signal launch()
signal closed()

var story: Story
var comms: Comms                   # its voice meter moves the mouth
var font: Font
var time := 0.0
var talk := 0.0
var launch_button: Button


func setup(p_story: Story) -> void:
	story = p_story


func _ready() -> void:
	font = ThemeDB.fallback_font
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	launch_button = _make_button("Launch the operation", func(): launch.emit())
	_make_button("Not yet", func(): closed.emit())
	if story.mirror:
		launch_button.text = "The host launches it"
		launch_button.disabled = true      # a friend reads along; the host's Launch starts it for everyone
	launch_button.grab_focus()
	resized.connect(_place_buttons)
	_place_buttons()
	story.briefing_open = true
	story.hush()
	for id in story.brief_lines():
		story.say(id)


func _exit_tree() -> void:
	if story != null:
		story.briefing_open = false


func _make_button(text: String, on_press: Callable) -> Button:
	var b := Button.new()
	b.text = text
	b.add_theme_font_size_override("font_size", 20)
	b.pressed.connect(on_press)
	b.process_mode = Node.PROCESS_MODE_ALWAYS
	add_child(b)
	return b


func _layout() -> Dictionary:
	var vp := get_viewport_rect().size
	var cw := minf(vp.x * 0.42, 640.0)
	var call := Rect2(vp.x * 0.5 - cw - 20.0, vp.y * 0.18, cw, cw * 0.75)
	var info := Rect2(vp.x * 0.5 + 20.0, vp.y * 0.18, minf(vp.x * 0.4, 600.0), cw * 0.75)
	return {"call": call, "info": info}


func _place_buttons() -> void:
	var l := _layout()
	var info: Rect2 = l["info"]
	var buttons := get_children().filter(func(c): return c is Button)
	for i in buttons.size():
		var b: Button = buttons[i]
		b.position = Vector2(info.position.x + i * 250.0, info.end.y - 52.0)
		b.size = Vector2(236.0 if i == 0 else 150.0, 48.0)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("pause_menu") or event.is_action_pressed("ui_cancel") or event.is_action_pressed("briefing"):
		get_viewport().set_input_as_handled()
		closed.emit()


func _process(delta: float) -> void:
	time += delta
	if comms != null:
		talk = comms.talk
	else:
		var lvl := 0.0
		if story.line != "" and fmod(time, 1.7) < 1.4:
			lvl = 0.5 + 0.5 * sin(time * 19.0)
		talk = lerpf(talk, lvl, clampf(delta * 18.0, 0.0, 1.0))
	queue_redraw()


func _draw() -> void:
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.01, 0.03, 0.9))
	var spec := story.current()
	var l := _layout()
	var call: Rect2 = l["call"]
	var info: Rect2 = l["info"]
	# the call: whoever speaks, else Hale in the ops room waiting
	var speaker := "hale"
	var place := "ops_room"
	if story.line != "":
		var row := StoryLines.row(story.line)
		speaker = row[1]
		place = row[2]
	draw_rect(call.grow(6.0), Color(0.03, 0.06, 0.1))
	CallView.draw_call(self, call, speaker, place, talk, 0.0, time)
	var spk: Dictionary = StoryLines.SPEAKERS.get(speaker, {})
	var col: Color = spk.get("color", Color(0.6, 0.8, 1.0))
	draw_rect(Rect2(call.position.x, call.end.y + 10, call.size.x, 34), Color(0.02, 0.04, 0.08, 0.95))
	draw_rect(Rect2(call.position.x, call.end.y + 10, 4, 34), col)
	draw_string(font, Vector2(call.position.x + 14, call.end.y + 33), "%s  ·  %s" % [spk.get("name", "").to_upper(), spk.get("title", "")], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, col)
	# the words under the call
	if story.line != "":
		var ty := call.end.y + 72.0
		for ln in _wrap(StoryLines.text(story.line), call.size.x, 18):
			draw_string(font, Vector2(call.position.x, ty), ln, HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color(0.92, 0.95, 1.0))
			ty += 24.0
	# the orders
	var x := info.position.x
	var y := info.position.y + 20.0
	draw_string(font, Vector2(x, y), str(Story.ACTS[int(spec.get("act", 1))]).to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.6, 0.75, 0.9))
	y += 46.0
	draw_string(font, Vector2(x, y), "Operation %d: %s" % [story.index + 1, spec.get("title", "")], HORIZONTAL_ALIGNMENT_LEFT, -1, 34, Color(0.92, 0.96, 1.0))
	y += 34.0
	draw_string(font, Vector2(x, y), spec.get("where", ""), HORIZONTAL_ALIGNMENT_LEFT, -1, 17, Color(1.0, 0.85, 0.55))
	y += 40.0
	for ln in _wrap(Names.fill(spec.get("summary", "")), info.size.x, 18):
		draw_string(font, Vector2(x, y), ln, HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color(0.85, 0.9, 0.95))
		y += 25.0
	y += 16.0
	if story.world != null and story.world.player != null:
		draw_string(font, Vector2(x, y), "Your ship: %s   ·   %d credits" % [story.world.player.design_name, story.world.player.credits], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.65, 0.72, 0.85))
		y += 22.0
		draw_string(font, Vector2(x, y), "Refit at the starbase first if you like: the operation waits.", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.65, 0.72, 0.85))
	draw_string(font, Vector2(0, vp.y - 30.0), "Enter: launch   ·   Esc or %s: not yet" % Keybinds.short_text("briefing"), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 14, Color(0.6, 0.7, 0.85))


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
