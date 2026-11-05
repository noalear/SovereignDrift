## MainMenu -- the title screen: painted key art, the game's logo, a column
## of choices down the left, and the Single Player, Online, Practice,
## pilot, settings and controls pages on glass panels.  The pause menu opens it in_game, on the
## Settings page over the running game.
class_name MainMenu
extends Control

signal play_pressed()
signal practice_pressed()
signal free_flight_pressed()
signal tutorial_pressed()
signal campaign_new_pressed()
signal campaign_continue_pressed()
signal story_new_pressed()
signal story_continue_pressed()
signal story_load_pressed()
signal host_pressed(versus: bool, campaign: String)   # Conquest hosted: a round (""), or the saving campaign ("new", "continue")
signal duel_pressed()
signal story_host_pressed(fresh: bool)  # the story campaign, hosted for friends: a new war or the saved one
signal mode_pressed(which: String, online: bool, versus: bool)   # "survival" or "skirmish", alone or hosted
signal join_pressed(address: String)
signal quit_pressed()
signal closed()              # in_game: Back or Esc on the Settings page

const KEYART := "res://assets/menu/keyart.jpg"
const LOGO := "res://assets/menu/logo.png"
const FONT_HEAD := "res://assets/fonts/Orbitron.ttf"           # titles, the logo's stand-in
const FONT_BODY := "res://assets/fonts/Rajdhani-Medium.ttf"    # paragraphs and notes
const FONT_LABEL := "res://assets/fonts/Rajdhani-SemiBold.ttf" # buttons and labels

# the palette: cold steel and cyan light, amber for anything that needs a look
const INK := Color(0.94, 0.97, 1.0)
const DIM := Color(0.64, 0.74, 0.88)
const MUTE := Color(0.5, 0.58, 0.72)
const CYAN := Color(0.38, 0.86, 1.0)
const AMBER := Color(1.0, 0.76, 0.42)
const GLASS := Color(0.015, 0.035, 0.075, 0.86)
const GLASS_LIT := Color(0.08, 0.2, 0.36, 0.9)
const EDGE := Color(0.45, 0.75, 1.0, 0.26)
const EDGE_LIT := Color(0.6, 0.9, 1.0, 0.85)

const INTRO_TIME := 1.3         # seconds the title screen takes to fade and slide in
const PAGE_TIME := 0.22         # seconds a page takes to settle after a change
const PARALLAX := 16.0          # pixels the key art shifts for the mouse at the screen's edge
const DUST := 46                # motes drifting over the painting

var font: Font                  # body
var label_font: Font
var head_font: Font             # Orbitron, bold and spaced
var caps_font: Font             # small spaced capitals for section names
var keyart: Texture2D = null
var logo: Texture2D = null
var glow_tex: GradientTexture2D
var vignette_tex: GradientTexture2D
var page := "main"      # main | single | online | practice | pilot | settings | controls
var versus := false      # the Online page hosts competitive games (friends against you), not co-op
var buttons: Array = []   # [rect, label, callback, enabled]
var hover := -1
var focus := 0            # keyboard / gamepad focus, an index into buttons
var name_edit: LineEdit
var ip_edit: LineEdit
var online_status := ""
var time := 0.0
var frame_dt := 0.0
var intro := 1.0             # 0 -> 1 as the title screen comes in
var page_fade := 0.0         # 1 -> 0 after a page change: the content slides into place
var mouse_norm := Vector2.ZERO   # the mouse, -0.5..0.5 across the screen
var parallax := Vector2.ZERO     # eased towards mouse_norm
var lit_amount := {}         # button label -> 0..1, how far its highlight has come up
var row_anim := 1.0          # alpha of the row being drawn during the intro
var content_alpha := 1.0     # alpha of the page's content during a page change
var dust: Array = []
var capture_action := ""     # Controls page: the action waiting for a key
var capture_slot := ""       # "kb0", "kb1" or "pad"
var capture_since := 0
var capture_mod: InputEventKey = null   # Shift, Ctrl, Alt held while waiting: a combination, or the key itself on release
var controls_status := ""
var campaign_note := ""      # the warning before a new campaign replaces the saved one
var save_info := {}          # the saved campaign's summary, read once
var story_info := {}         # the story campaign's, the same
var story_slots := false     # there are story save slots to load
var story_note := ""         # the warning before a new story replaces the saved one
var in_game := false         # opened from the pause menu: Settings and Controls only
var world: World = null      # in_game: the camera setting zooms it at once


func _ready() -> void:
	_load_fonts()
	keyart = load(KEYART) as Texture2D if ResourceLoader.exists(KEYART) else null
	logo = load(LOGO) as Texture2D if ResourceLoader.exists(LOGO) else null
	glow_tex = _radial([Color(1, 1, 1, 1), Color(1, 1, 1, 0.35), Color(1, 1, 1, 0)], [0.0, 0.35, 1.0])
	vignette_tex = _radial([Color(0, 0, 0, 0), Color(0, 0, 0, 0), Color(0, 0, 0, 0.85)], [0.0, 0.5, 1.0])
	save_info = SaveGame.summary()
	story_info = SaveGame.summary(SaveGame.STORY_PATH)
	story_slots = SaveGame.has_slots()
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	if in_game:
		process_mode = Node.PROCESS_MODE_ALWAYS   # the game is paused behind it
		page = "settings"
	else:
		intro = 0.0
	name_edit = LineEdit.new()
	name_edit.max_length = 16
	name_edit.text = Settings.pilot_name
	name_edit.placeholder_text = "Pilot name"
	_style_edit(name_edit)
	name_edit.text_changed.connect(func(t): Settings.pilot_name = t.strip_edges() if not t.strip_edges().is_empty() else "Pilot")
	add_child(name_edit)
	name_edit.hide()
	ip_edit = LineEdit.new()
	ip_edit.max_length = 64
	ip_edit.text = Settings.last_join_ip
	ip_edit.placeholder_text = "Host address, e.g. 192.168.1.20"
	_style_edit(ip_edit)
	ip_edit.text_submitted.connect(func(t): join_pressed.emit(t))
	add_child(ip_edit)
	ip_edit.hide()
	var rng := RandomNumberGenerator.new()
	rng.seed = 7
	for i in DUST:
		dust.append({"pos": Vector2(rng.randf(), rng.randf()), "vel": Vector2(rng.randf_range(0.004, 0.014), rng.randf_range(-0.004, 0.002)),
			"size": rng.randf_range(1.0, 2.6), "phase": rng.randf() * TAU, "a": rng.randf_range(0.25, 0.7)})


## Orbitron for titles, Rajdhani for everything else; the engine's own
## font stands in if a file is missing, so a broken import never hides
## the menu.
func _load_fonts() -> void:
	var body := load(FONT_BODY) as Font if ResourceLoader.exists(FONT_BODY) else null
	var label := load(FONT_LABEL) as Font if ResourceLoader.exists(FONT_LABEL) else null
	var head := load(FONT_HEAD) as Font if ResourceLoader.exists(FONT_HEAD) else null
	font = body if body != null else ThemeDB.fallback_font
	label_font = label if label != null else font
	var hv := FontVariation.new()
	hv.base_font = head if head != null else label_font
	if head != null:
		hv.variation_opentype = {TextServerManager.get_primary_interface().name_to_tag("weight"): 700}
	hv.spacing_glyph = 3
	head_font = hv
	var cv := FontVariation.new()
	cv.base_font = label_font
	cv.spacing_glyph = 2
	caps_font = cv


## The menu's look as a Theme, for the engine-drawn panels that sit over
## the game (the pause menu): the same fonts, glass buttons and cyan edge.
static func make_theme() -> Theme:
	var t := Theme.new()
	var label := load(FONT_LABEL) as Font if ResourceLoader.exists(FONT_LABEL) else ThemeDB.fallback_font
	var head := load(FONT_HEAD) as Font if ResourceLoader.exists(FONT_HEAD) else label
	var hv := FontVariation.new()
	hv.base_font = head
	hv.spacing_glyph = 3
	t.set_font("font", "Label", hv)
	t.set_color("font_color", "Label", INK)
	t.set_font("font", "Button", label)
	t.set_font("font", "LineEdit", label)
	t.set_color("font_color", "Button", DIM)
	t.set_color("font_hover_color", "Button", INK)
	t.set_color("font_focus_color", "Button", INK)
	t.set_color("font_pressed_color", "Button", INK)
	t.set_color("font_disabled_color", "Button", Color(0.4, 0.44, 0.52))
	for state in ["normal", "hover", "pressed", "focus", "disabled"]:
		var sb := StyleBoxFlat.new()
		var lit: bool = state in ["hover", "pressed", "focus"]
		sb.bg_color = GLASS_LIT if lit else GLASS
		sb.border_color = EDGE_LIT if lit else EDGE
		sb.set_border_width_all(1)
		sb.border_width_left = 4
		sb.set_content_margin_all(10.0)
		sb.content_margin_left = 18.0
		sb.content_margin_right = 18.0
		if state == "focus":
			sb.draw_center = false
			sb.border_color = Color(1.0, 0.95, 0.75, 0.6)
			sb.border_width_left = 1
		t.set_stylebox(state, "Button", sb)
	return t


func _radial(colors: Array, offsets: Array) -> GradientTexture2D:
	var g := Gradient.new()
	g.colors = PackedColorArray(colors)
	g.offsets = PackedFloat32Array(offsets)
	var t := GradientTexture2D.new()
	t.gradient = g
	t.fill = GradientTexture2D.FILL_RADIAL
	t.fill_from = Vector2(0.5, 0.5)
	t.fill_to = Vector2(1.0, 0.5)
	t.width = 256
	t.height = 256
	return t


func _style_edit(e: LineEdit) -> void:
	e.add_theme_font_override("font", label_font)
	e.add_theme_font_size_override("font_size", 22)
	e.add_theme_color_override("font_color", INK)
	e.add_theme_color_override("font_placeholder_color", MUTE)
	e.add_theme_color_override("caret_color", CYAN)
	for state in ["normal", "focus", "read_only"]:
		var sb := StyleBoxFlat.new()
		sb.bg_color = Color(0.03, 0.07, 0.14, 0.9)
		sb.border_color = EDGE_LIT if state == "focus" else EDGE
		sb.set_border_width_all(1)
		sb.set_corner_radius_all(3)
		sb.content_margin_left = 12.0
		sb.content_margin_right = 12.0
		e.add_theme_stylebox_override(state, sb)


func _process(delta: float) -> void:
	time += delta
	frame_dt = delta
	if intro < 1.0:
		intro = minf(intro + delta / INTRO_TIME, 1.0)
	if page_fade > 0.0:
		page_fade = maxf(page_fade - delta / PAGE_TIME, 0.0)
	parallax = parallax.lerp(mouse_norm, minf(delta * 4.0, 1.0))
	for d in dust:
		d["pos"] += d["vel"] * delta
		if d["pos"].x > 1.05:
			d["pos"].x = -0.05
			d["pos"].y = randf()
		if d["pos"].y < -0.05 or d["pos"].y > 1.05:
			d["pos"].y = clampf(d["pos"].y, -0.05, 1.05)
			d["vel"].y = -d["vel"].y
	queue_redraw()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var vp := get_viewport_rect().size
		mouse_norm = Vector2(event.position.x / maxf(vp.x, 1.0) - 0.5, event.position.y / maxf(vp.y, 1.0) - 0.5)
		var was := hover
		hover = -1
		for i in buttons.size():
			if buttons[i][0].has_point(event.position):
				hover = i
				focus = i
		if hover != was and hover >= 0 and buttons[hover][3]:
			Sfx.play_ui("ui_hover", 0.7)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for i in buttons.size():
			if buttons[i][0].has_point(event.position) and buttons[i][3]:
				Sfx.play_ui("ui_click", 0.6)
				buttons[i][2].call()
				accept_event()
				return


## While a Controls slot waits for a key, the next key, mouse button or
## gamepad control goes into it, before anything else sees it.  Shift,
## Ctrl or Alt wait to see what comes with them: a key or mouse button
## pressed while one is held makes a combination (Shift+Mouse left);
## letting go with nothing else binds the modifier on its own.
func _input(event: InputEvent) -> void:
	if capture_action.is_empty():
		return
	if Time.get_ticks_msec() - capture_since < 180:
		get_viewport().set_input_as_handled()
		return
	var pressed := false
	if event is InputEventKey:
		pressed = event.pressed and not event.echo
	elif event is InputEventMouseButton or event is InputEventJoypadButton:
		pressed = event.pressed
	elif event is InputEventJoypadMotion:
		pressed = absf(event.axis_value) > 0.6
	var mod_key: bool = event is InputEventKey and event.physical_keycode in Keybinds.MOD_KEYS
	if mod_key and pressed:
		if capture_mod == null:
			capture_mod = event
		get_viewport().set_input_as_handled()
		return
	if mod_key and not event.pressed and capture_mod != null and event.physical_keycode == capture_mod.physical_keycode:
		event = capture_mod             # let go with nothing else: the modifier on its own
		pressed = true
	if not pressed:
		if not event is InputEventMouseMotion:
			get_viewport().set_input_as_handled()
		return
	get_viewport().set_input_as_handled()
	capture_mod = null
	var label := _action_label(capture_action)
	if event is InputEventKey and event.physical_keycode == KEY_ESCAPE and capture_action != "pause_menu":
		controls_status = "Cancelled."
	elif event is InputEventKey and event.physical_keycode in [KEY_BACKSPACE, KEY_DELETE]:
		Keybinds.clear(capture_action, capture_slot)
		controls_status = "%s: slot cleared." % label
	else:
		var slot := capture_slot
		if event is InputEventJoypadButton or event is InputEventJoypadMotion:
			slot = "pad"
		elif slot == "pad":
			slot = "kb0"
		var moved := Keybinds.assign(capture_action, slot, event)
		controls_status = "%s: %s.%s" % [label, Keybinds.slot_text(capture_action, slot), ("  (taken from %s)" % moved) if not moved.is_empty() else ""]
		Sfx.play_ui("ui_click", 0.6)
	capture_action = ""
	capture_slot = ""


func _action_label(action: String) -> String:
	for row in Keybinds.ACTIONS:
		if row[0] == action:
			return row[1]
	return action


## Every key the menu uses stops here, so nothing behind it (the HUD, the
## pause menu) acts on it too.
func _unhandled_input(event: InputEvent) -> void:
	if (event.is_action_pressed("pause_menu") or event.is_action_pressed("ui_cancel")) and page != "main":
		Sfx.play_ui("ui_back", 0.6)
		_back()
	elif event.is_action_pressed("ui_accept"):
		_activate_focus()
	elif event.is_action_pressed("ui_down", true):
		_move_focus(0, 1)
	elif event.is_action_pressed("ui_up", true):
		_move_focus(0, -1)
	elif event.is_action_pressed("ui_right", true):
		_move_focus(1, 0)
	elif event.is_action_pressed("ui_left", true):
		_move_focus(-1, 0)
	elif event.is_action_pressed("music_next"):
		Music.next_track()
	elif event.is_action_pressed("fullscreen"):
		Settings.toggle_fullscreen()
	else:
		return
	get_viewport().set_input_as_handled()


## One page up: Controls to Settings, then every page to the title screen,
## or in game back to the pause menu.
func _back() -> void:
	if page == "controls":
		_set_page("settings")
	elif in_game:
		_set_page("settings")   # saves
		closed.emit()
	else:
		_set_page("main")


## Keyboard and gamepad walk the buttons: up and down change row, left and
## right move along a row of choices.  Buttons are rebuilt every frame, so
## the focus is an index and the rows come from the rectangles.
func _move_focus(dx: int, dy: int) -> void:
	if buttons.is_empty():
		return
	if focus < 0 or focus >= buttons.size():
		focus = 0
		queue_redraw()
		return
	var cur: Rect2 = buttons[focus][0]
	var best := -1
	var best_score := INF
	for i in buttons.size():
		if i == focus:
			continue
		var r: Rect2 = buttons[i][0]
		var same_row := absf(r.position.y - cur.position.y) < 8.0
		var score := INF
		if dy != 0 and not same_row and signf(r.position.y - cur.position.y) == float(dy):
			score = absf(r.position.y - cur.position.y) * 4.0 + absf(r.get_center().x - cur.get_center().x) * 0.5
		elif dx != 0 and same_row and signf(r.position.x - cur.position.x) == float(dx):
			score = absf(r.position.x - cur.position.x)
		if score < best_score:
			best_score = score
			best = i
	if best >= 0:
		focus = best
		hover = -1
		Sfx.play_ui("click", 0.25)
		queue_redraw()


func _activate_focus() -> void:
	if focus >= 0 and focus < buttons.size() and buttons[focus][3]:
		Sfx.play_ui("click", 0.5)
		buttons[focus][2].call()


func _set_page(p: String) -> void:
	if p != page:
		page_fade = 1.0
	page = p
	focus = 0
	hover = -1
	story_note = ""          # a warning before a new war is for the page it was given on
	campaign_note = ""
	name_edit.visible = p == "pilot"
	ip_edit.visible = p == "online"
	if p == "online":
		online_status = ""
		Net.lan.listen()
	elif Net.lan.listening:
		Net.lan.stop()
	if p != "pilot":
		Settings.save_settings()
		Music.set_volume(Settings.music_volume)


# ---------------------------------------------------------------- drawing helpers

## A colour at the alpha of whatever is fading in right now.
func _ink(c: Color) -> Color:
	return Color(c.r, c.g, c.b, c.a * content_alpha * row_anim)


func _text(pos: Vector2, s: String, size: int, col: Color, align := HORIZONTAL_ALIGNMENT_LEFT, width := -1.0, f: Font = null) -> void:
	draw_string(f if f != null else font, pos, s, align, width, size, _ink(col))


## A horizontal gradient: one quad with a colour at each end.
func _hgrad(rect: Rect2, left: Color, right: Color) -> void:
	var p := PackedVector2Array([rect.position, rect.position + Vector2(rect.size.x, 0), rect.end, rect.position + Vector2(0, rect.size.y)])
	draw_polygon(p, PackedColorArray([_ink(left), _ink(right), _ink(right), _ink(left)]))


func _vgrad(rect: Rect2, top: Color, bottom: Color) -> void:
	var p := PackedVector2Array([rect.position, rect.position + Vector2(rect.size.x, 0), rect.end, rect.position + Vector2(0, rect.size.y)])
	draw_polygon(p, PackedColorArray([_ink(top), _ink(top), _ink(bottom), _ink(bottom)]))


func _glow(rect: Rect2, col: Color) -> void:
	draw_texture_rect(glow_tex, rect, false, _ink(col))


## How lit a button is, eased over a few frames so highlights come up and
## fade rather than snap.
func _lit(key: String, target: float) -> float:
	var v: float = lit_amount.get(key, target)
	v = lerpf(v, target, minf(frame_dt * 14.0, 1.0))
	lit_amount[key] = v
	return v


## A glass panel: dark, a thin light edge, a brighter line along the top.
func _panel(rect: Rect2, title := "") -> void:
	draw_rect(rect, _ink(GLASS))
	_vgrad(Rect2(rect.position, Vector2(rect.size.x, 90.0)), Color(0.2, 0.45, 0.8, 0.14), Color(0.2, 0.45, 0.8, 0.0))
	draw_rect(rect, _ink(EDGE), false, 1.0)
	_hgrad(Rect2(rect.position, Vector2(rect.size.x, 2.0)), Color(CYAN, 0.9), Color(CYAN, 0.0))
	# a corner bracket, top left and bottom right
	var c := _ink(Color(CYAN, 0.7))
	draw_line(rect.position + Vector2(0, 0), rect.position + Vector2(26, 0), c, 2.0)
	draw_line(rect.position + Vector2(0, 0), rect.position + Vector2(0, 26), c, 2.0)
	draw_line(rect.end - Vector2(26, 0), rect.end, c, 2.0)
	draw_line(rect.end - Vector2(0, 26), rect.end, c, 2.0)
	if not title.is_empty():
		_text(rect.position + Vector2(30, 46), title.to_upper(), 24, INK, HORIZONTAL_ALIGNMENT_LEFT, -1, head_font)
		_hgrad(Rect2(rect.position + Vector2(30, 58), Vector2(rect.size.x - 60.0, 1.0)), Color(CYAN, 0.8), Color(CYAN, 0.0))


## A pill button on a page: glass, lit with a cyan wash and a glow.
func _button(rect: Rect2, label: String, cb: Callable, enabled: bool = true) -> void:
	buttons.append([rect, label, cb, enabled])
	var idx := buttons.size() - 1
	var lit := _lit("b:%s:%d" % [label, int(rect.position.y)], 1.0 if (hover == idx or focus == idx) and enabled else 0.0)
	if enabled and lit > 0.02:
		_glow(rect.grow_individual(rect.size.y * 0.8, rect.size.y * 0.9, rect.size.y * 0.8, rect.size.y * 0.9), Color(CYAN, 0.22 * lit))
	var bg := GLASS.lerp(GLASS_LIT, lit) if enabled else Color(0.04, 0.05, 0.08, 0.5)
	draw_rect(rect, _ink(bg))
	if enabled:
		_hgrad(rect, Color(CYAN, 0.18 * lit), Color(CYAN, 0.0))
	draw_rect(rect, _ink(EDGE.lerp(EDGE_LIT, lit) if enabled else Color(EDGE, 0.1)), false, 1.0)
	if enabled:
		draw_rect(Rect2(rect.position, Vector2(3.0, rect.size.y)), _ink(Color(CYAN, 0.35 + 0.65 * lit)))
	if focus == idx and enabled:
		draw_rect(rect.grow(3), _ink(Color(1.0, 0.95, 0.75, 0.45 + 0.3 * sin(time * 6.0))), false, 1.0)
	var col := INK if enabled else Color(0.42, 0.47, 0.56)
	_text(rect.position + Vector2(0, rect.size.y * 0.5 + 7.0), label, 21, col, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, label_font)


## A row of options with one of them selected.
func _choice(y: float, label: String, options: Array, current: Variant, setter: Callable, vp: Vector2, at_x := -1.0) -> void:
	var x := at_x if at_x >= 0.0 else vp.x * 0.5 - 300.0
	_text(Vector2(x, y + 27), label, 21, DIM, HORIZONTAL_ALIGNMENT_LEFT, -1, label_font)
	var bx := x + 220.0
	# numbers set by other means (the mouse-wheel zoom) light the nearest option
	var match_value: Variant = current
	if typeof(current) == TYPE_FLOAT:
		var best := INF
		for o in options:
			if typeof(o[0]) in [TYPE_FLOAT, TYPE_INT] and absf(float(o[0]) - current) < best:
				best = absf(float(o[0]) - current)
				match_value = o[0]
	for o in options:
		var w := 24.0 + label_font.get_string_size(str(o[1]), HORIZONTAL_ALIGNMENT_LEFT, -1, 19).x
		var rect := Rect2(bx, y, w, 40)
		var selected: bool = o[0] == match_value
		var idx := buttons.size()
		buttons.append([rect, o[1], setter.bind(o[0]), true])
		var lit := _lit("c:%s:%s" % [label, str(o[1])], 1.0 if hover == idx or focus == idx else 0.0)
		if selected:
			draw_rect(rect, _ink(Color(0.16, 0.5, 0.82, 0.9)))
			_vgrad(rect, Color(1, 1, 1, 0.14), Color(1, 1, 1, 0.0))
		else:
			draw_rect(rect, _ink(GLASS.lerp(GLASS_LIT, lit)))
		draw_rect(rect, _ink(EDGE_LIT if selected else EDGE.lerp(EDGE_LIT, lit)), false, 1.0)
		if focus == idx:
			draw_rect(rect.grow(3), _ink(Color(1.0, 0.95, 0.75, 0.45 + 0.3 * sin(time * 6.0))), false, 1.0)
		_text(rect.position + Vector2(12, 27), str(o[1]), 19, INK if selected else DIM.lerp(INK, lit), HORIZONTAL_ALIGNMENT_LEFT, -1, label_font)
		bx += w + 8.0


## A title-screen entry: a wide quiet strip that lights up with a cyan wash,
## an accent bar and a glow, with the name set large and a note under it.
func _row(rect: Rect2, label: String, note: String, cb: Callable, enabled: bool = true, size := 30) -> void:
	buttons.append([rect, label, cb, enabled])
	var idx := buttons.size() - 1
	var lit := _lit("r:" + label, 1.0 if (hover == idx or focus == idx) and enabled else 0.0)
	if enabled and lit > 0.02:
		_glow(Rect2(rect.position - Vector2(rect.size.y * 0.5, rect.size.y * 0.9), Vector2(rect.size.x * 0.9, rect.size.y * 2.8)), Color(CYAN, 0.2 * lit))
		_hgrad(rect, Color(CYAN, 0.2 * lit), Color(CYAN, 0.0))
		_hgrad(Rect2(rect.position + Vector2(0, rect.size.y - 1.0), Vector2(rect.size.x, 1.0)), Color(CYAN, 0.9 * lit), Color(CYAN, 0.0))
	draw_rect(Rect2(rect.position, Vector2(4.0, rect.size.y)), _ink(Color(CYAN, (0.15 + 0.85 * lit) if enabled else 0.06)))
	if focus == idx and enabled:
		draw_rect(Rect2(rect.position + Vector2(-5.0, 0), Vector2(2.0, rect.size.y)), _ink(Color(1.0, 0.95, 0.75, 0.5 + 0.3 * sin(time * 6.0))))
	var shift := 10.0 * lit
	var col := DIM.lerp(INK, lit) if enabled else Color(0.38, 0.42, 0.5)
	var has_note := not note.is_empty() and rect.size.y >= 54.0
	var ty := rect.size.y * 0.5 + size * 0.36 - (8.0 if has_note else 0.0)
	_text(rect.position + Vector2(18.0 + shift, ty), label.to_upper(), size, col, HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
	if has_note:
		_text(rect.position + Vector2(19.0 + shift, ty + 19.0), note, 15, MUTE.lerp(DIM, lit) if enabled else Color(0.3, 0.33, 0.4), HORIZONTAL_ALIGNMENT_LEFT, -1, font)


## A small choice beside a row's name: New, Continue, Load.
func _chip(rect: Rect2, label: String, cb: Callable, enabled: bool = true) -> void:
	buttons.append([rect, label, cb, enabled])
	var idx := buttons.size() - 1
	var lit := _lit("k:%s:%d" % [label, int(rect.position.y)], 1.0 if (hover == idx or focus == idx) and enabled else 0.0)
	if enabled and lit > 0.02:
		_glow(rect.grow(rect.size.y * 0.7), Color(CYAN, 0.2 * lit))
	draw_rect(rect, _ink(GLASS.lerp(GLASS_LIT, lit) if enabled else Color(0.04, 0.05, 0.08, 0.45)))
	draw_rect(rect, _ink(EDGE.lerp(EDGE_LIT, lit) if enabled else Color(EDGE, 0.12)), false, 1.0)
	if focus == idx and enabled:
		draw_rect(rect.grow(3), _ink(Color(1.0, 0.95, 0.75, 0.45 + 0.3 * sin(time * 6.0))), false, 1.0)
	_text(rect.position + Vector2(0, rect.size.y * 0.5 + 6.0), label.to_upper(), 16, (DIM.lerp(INK, lit)) if enabled else Color(0.38, 0.42, 0.5), HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, caps_font)


func _chip_width(label: String) -> float:
	return 28.0 + caps_font.get_string_size(label.to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 16).x


# ---------------------------------------------------------------- the screens

func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	content_alpha = 1.0
	row_anim = 1.0
	if in_game:
		draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.015, 0.04, 0.84))
		draw_texture_rect(vignette_tex, Rect2(Vector2.ZERO, vp), false, Color(1, 1, 1, 0.8))
		content_alpha = 1.0 - page_fade
		draw_set_transform(Vector2(0, 14.0 * page_fade))
		_text(Vector2(0, vp.y * 0.14), "SETTINGS" if page == "settings" else "CONTROLS AND KEYS", 38, INK, HORIZONTAL_ALIGNMENT_CENTER, vp.x, head_font)
		_text(Vector2(0, vp.y * 0.14 + 30), "Changes apply at once.   Esc goes back.", 16, DIM, HORIZONTAL_ALIGNMENT_CENTER, vp.x, font)
		if page == "controls":
			_draw_controls(vp)
		else:
			_draw_settings(vp)
		draw_set_transform(Vector2.ZERO)
		return
	_draw_backdrop(vp)
	# the title screen comes in: the painting first, then the logo and the rows
	var art_a := smoothstep(0.0, 0.55, intro)
	if art_a < 1.0:
		draw_rect(Rect2(Vector2.ZERO, vp), Color(0, 0, 0, 1.0 - art_a))
	content_alpha = smoothstep(0.3, 0.8, intro) * (1.0 - page_fade)
	draw_set_transform(Vector2(0, 14.0 * page_fade))
	match page:
		"main":
			_draw_main(vp)
		"pilot":
			_draw_pilot(vp)
		"settings":
			_draw_settings(vp)
		"single":
			_draw_single(vp)
		"practice":
			_draw_practice(vp)
		"online":
			_draw_online(vp)
		"controls":
			_draw_controls(vp)
	draw_set_transform(Vector2.ZERO)
	content_alpha = smoothstep(0.5, 1.0, intro)
	_draw_update(vp)
	_draw_bottom_bar(vp)


## The painting covers the screen, drifting slowly and leaning a little
## with the mouse; motes of dust cross it; the edges darken.
func _draw_backdrop(vp: Vector2) -> void:
	if keyart != null:
		var ts := keyart.get_size()
		var cover := maxf(vp.x / ts.x, vp.y / ts.y)
		var zoom := cover * (1.04 + 0.025 * sin(time * 0.07))
		var size := ts * zoom
		var drift := Vector2(sin(time * 0.05) * 9.0, cos(time * 0.065) * 6.0)
		var pos := (vp - size) * 0.5 + drift - parallax * PARALLAX
		pos.x = clampf(pos.x, vp.x - size.x, 0.0)
		pos.y = clampf(pos.y, vp.y - size.y, 0.0)
		draw_texture_rect(keyart, Rect2(pos, size), false)
	else:
		draw_rect(Rect2(Vector2.ZERO, vp), Color(0.01, 0.02, 0.05))
	# the column's side goes dark so the names read over the painting
	_hgrad(Rect2(Vector2.ZERO, Vector2(vp.x * 0.55, vp.y)), Color(0.0, 0.01, 0.03, 0.82), Color(0.0, 0.01, 0.03, 0.0))
	_vgrad(Rect2(Vector2(0, vp.y * 0.72), Vector2(vp.x, vp.y * 0.28)), Color(0, 0, 0, 0.0), Color(0, 0, 0, 0.75))
	_vgrad(Rect2(Vector2.ZERO, Vector2(vp.x, vp.y * 0.18)), Color(0, 0, 0, 0.45), Color(0, 0, 0, 0.0))
	draw_texture_rect(vignette_tex, Rect2(Vector2.ZERO, vp), false, Color(1, 1, 1, 0.75))
	if page != "main":
		draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.01, 0.03, 0.45))
	for d in dust:
		var p: Vector2 = d["pos"] * vp + Vector2(0, sin(time * 0.6 + d["phase"]) * 6.0)
		var a: float = d["a"] * (0.55 + 0.45 * sin(time * 1.3 + d["phase"]))
		draw_circle(p, d["size"] * 2.2, Color(0.6, 0.85, 1.0, a * 0.12))
		draw_circle(p, d["size"] * 0.8, Color(0.85, 0.95, 1.0, a * 0.7))


## The logo, with a soft dark halo behind it so it reads over anything,
## and a faint light sweep that crosses it now and then.
func _draw_logo(x: float, y: float, w: float) -> Rect2:
	var a := smoothstep(0.2, 0.75, intro)
	var scale := 1.0 + 0.06 * (1.0 - a)
	if logo != null:
		var ts := logo.get_size()
		var h := w * ts.y / ts.x
		var rect := Rect2(Vector2(x, y), Vector2(w, h))
		var centre := rect.get_center()
		var sr := Rect2(centre - rect.size * 0.5 * scale, rect.size * scale)
		draw_texture_rect(glow_tex, sr.grow_individual(sr.size.x * 0.1, sr.size.y * 0.9, sr.size.x * 0.1, sr.size.y * 0.9), false, Color(0.0, 0.02, 0.06, 0.8 * a))
		draw_texture_rect(logo, sr, false, Color(1, 1, 1, a))
		var sweep := fmod(time * 0.16, 1.0)
		if sweep < 0.25:
			var sx := sr.position.x + sr.size.x * (sweep / 0.25) * 1.2 - sr.size.x * 0.1
			_hgrad(Rect2(Vector2(sx - 70.0, sr.position.y + sr.size.y * 0.25), Vector2(70.0, sr.size.y * 0.5)), Color(1, 1, 1, 0.0), Color(1, 1, 1, 0.18 * a))
			_hgrad(Rect2(Vector2(sx, sr.position.y + sr.size.y * 0.25), Vector2(70.0, sr.size.y * 0.5)), Color(1, 1, 1, 0.18 * a), Color(1, 1, 1, 0.0))
		return rect
	# no painting: the name set in the title face with a glow behind it
	var size := int(w * 0.105)
	draw_string(head_font, Vector2(x, y + size), Names.GAME.to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, size, Color(CYAN, 0.35 * a))
	draw_string(head_font, Vector2(x, y + size), Names.GAME.to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, size, Color(INK, a))
	return Rect2(Vector2(x, y), Vector2(w, size * 1.25))


func _draw_main(vp: Vector2) -> void:
	var x0 := clampf(vp.x * 0.07, 48.0, 150.0)
	var lw := clampf(vp.x * 0.40, 420.0, 780.0)
	var logo_rect := _draw_logo(x0 - lw * 0.04, vp.y * 0.085, lw)
	var y := logo_rect.end.y + 14.0
	_hgrad(Rect2(Vector2(x0, y), Vector2(lw * 0.9, 1.0)), Color(CYAN, 0.7), Color(CYAN, 0.0))
	_text(Vector2(x0, y + 26), _profile_line("single").to_upper(), 15, AMBER, HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
	y += 50.0
	var rows := 7
	var bottom := vp.y - 64.0
	var row_h := clampf((bottom - y) / rows, 42.0, 74.0)
	var rw := clampf(vp.x * 0.27, 330.0, 470.0)
	var story_line := story_note if not story_note.is_empty() else (_story_line() if not story_info.is_empty() else "")
	var rr := func(k: int) -> Rect2:   # row k's strip
		return Rect2(Vector2(x0, y + k * row_h), Vector2(rw, row_h - 6.0))
	var ra := func(k: int) -> void:    # the row's slice of the intro: later rows come later
		row_anim = smoothstep(0.3 + k * 0.05, 0.7 + k * 0.05, intro)
		draw_set_transform(Vector2(-34.0 * (1.0 - row_anim), 14.0 * page_fade))
	# 1  the story campaign: its name is a heading, the chips are the buttons
	ra.call(0)
	var cx := _heading_with_chips(rr.call(0), "The Frontier War", [["New", _new_story, true],
		["Continue", func(): story_continue_pressed.emit(), not story_info.is_empty()],
		["Load", func(): story_load_pressed.emit(), story_slots]], 30)
	if not story_line.is_empty():
		var r: Rect2 = rr.call(0)
		_text(Vector2(cx + 14.0, r.position.y + r.size.y * 0.5 + 5.0), story_line, 15, AMBER if not story_note.is_empty() else MUTE, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	var entries := [
		["Single Player", "Campaign, Conquest, Survival and Skirmish against the computer", func(): _set_page("single")],
		["Online", "Host or join friends: co-op or competitive, duels and The Frontier War", func(): _set_page("online")],
		["Practice", "Duels and free flight off the record, and the voiced tutorial", func(): _set_page("practice")],
		["Pilot", "Name, empire and career", func(): _set_page("pilot")],
		["Settings", "Sound, display, controls and keys", func(): _set_page("settings")],
		["Quit", "", func(): quit_pressed.emit()],
	]
	for k in entries.size():
		ra.call(k + 1)
		_row(rr.call(k + 1), entries[k][0], entries[k][1], entries[k][2])
	row_anim = 1.0
	draw_set_transform(Vector2(0, 14.0 * page_fade))


## A row whose name is only a heading, with its choices as chips beside it
## (New, Continue, Load), after the row or (`inside`) at its right end.
## Returns where the chips end.
func _heading_with_chips(r: Rect2, label: String, chips: Array, size := 30, note := "", enabled := true, note_col := MUTE, inside := false) -> float:
	var has_note := not note.is_empty() and r.size.y >= 54.0
	var ty := r.size.y * 0.5 + size * 0.36 - (8.0 if has_note else 0.0)
	_text(r.position + Vector2(18.0, ty), label.to_upper(), size, DIM if enabled else Color(0.38, 0.42, 0.5), HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
	if has_note:
		_text(r.position + Vector2(19.0, ty + 19.0), note, 15, note_col if enabled else Color(note_col, 0.75), HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	draw_rect(Rect2(r.position, Vector2(4.0, r.size.y)), _ink(Color(AMBER, 0.6 if enabled else 0.15)))
	var cx := r.end.x + 12.0
	if inside:
		cx = r.end.x - 12.0
		for c in chips:
			cx -= _chip_width(c[0]) + 6.0
	var ch := minf(34.0, r.size.y - 10.0)
	var cy := r.position.y + (r.size.y - ch) * 0.5
	for c in chips:
		var cw := _chip_width(c[0])
		_chip(Rect2(Vector2(cx, cy), Vector2(cw, ch)), c[0], c[1], c[2] and enabled)
		cx += cw + 6.0
	return cx


## A page of modes: a glass panel down the middle of the screen, a row per
## mode (name, a line about it) and Back under them.  Returns the panel.
func _mode_panel(vp: Vector2, title: String, n: int, extra := 0.0, width := 860.0, x := -1.0) -> Rect2:
	var row_h := 72.0
	var h := 84.0 + extra + n * row_h + 96.0
	var top := clampf(vp.y * 0.5 - h * 0.5 - 20.0, 90.0, maxf(vp.y - 56.0 - h - 12.0, 90.0))
	var px := x if x >= 0.0 else vp.x * 0.5 - width * 0.5
	var rect := Rect2(Vector2(px, top), Vector2(width, h))
	_panel(rect, title)
	return rect


func _mode_row(panel: Rect2, k: int, extra: float) -> Rect2:
	return Rect2(panel.position + Vector2(30.0, 84.0 + extra + k * 72.0), Vector2(panel.size.x - 60.0, 64.0))


func _back_button(panel: Rect2) -> void:
	_button(Rect2(panel.position + Vector2(30.0, panel.size.y - 72.0), Vector2(160, 48)), "Back", func(): _set_page("main"))


## Single Player: the campaign, a Conquest round, Survival and Skirmish,
## against the computer.
func _draw_single(vp: Vector2) -> void:
	var panel := _mode_panel(vp, "Single player", 4, 30.0)
	_text(panel.position + Vector2(30.0, 100.0), _profile_line("single"), 17, AMBER, HORIZONTAL_ALIGNMENT_LEFT, panel.size.x - 60.0, label_font)
	var has_save := not save_info.is_empty()
	# the note gives way to the saved war, or the warning before a new one replaces it
	var camp_line := campaign_note if not campaign_note.is_empty() else (_save_line() if has_save else "No clock: the war runs until one empire holds every colony. Saves as it goes.")
	_heading_with_chips(_mode_row(panel, 0, 30.0), "Campaign", [["New", _new_campaign, true], ["Continue", func(): campaign_continue_pressed.emit(), has_save]], 26,
		camp_line, true, AMBER if not campaign_note.is_empty() else MUTE, true)
	_row(_mode_row(panel, 1, 30.0), "Conquest", _conquest_note(), func(): play_pressed.emit(), true, 26)
	_row(_mode_row(panel, 2, 30.0), "Survival", _survival_note(), func(): mode_pressed.emit("survival", false, false), true, 26)
	_row(_mode_row(panel, 3, 30.0), "Skirmish", "%d a side at %s in ready-built ships, first to %d kills" % [Skirmish.TEAM_SIZE, Skirmish.ARENA, Skirmish.KILLS_TO_WIN], func(): mode_pressed.emit("skirmish", false, false), true, 26)
	_back_button(panel)


func _conquest_note() -> String:
	var length := "until one empire holds every colony" if Settings.round_minutes <= 0 else "%d-minute round" % Settings.round_minutes
	return "One empire against two for every world of Sol, %s" % length


func _survival_note() -> String:
	var best := int(Settings.profile_info("single")["career"].get("survival_best", 0))
	return "Hold your starbase against waves of raiders" + ((", best %d" % best) if best > 0 else "")


## Practice: off the record.  The tutorial lives here too.
func _draw_practice(vp: Vector2) -> void:
	var panel := _mode_panel(vp, "Practice", 3, 30.0)
	_text(panel.position + Vector2(30.0, 100.0), "Nothing here counts toward your career.", 16, DIM, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	_row(_mode_row(panel, 0, 30.0), "Practice duel", "One pirate Scout at a time, no clock, no fleets", func(): practice_pressed.emit(), true, 26)
	_row(_mode_row(panel, 1, 30.0), "Free flight", "Sol with nobody shooting: learn the ship and the autopilot", func(): free_flight_pressed.emit(), true, 26)
	_row(_mode_row(panel, 2, 30.0), "Tutorial (voiced, %s)" % Names.TERRAN, "Commander Hale walks you through flying, fighting and taking worlds", func(): tutorial_pressed.emit(), true, 26)
	_back_button(panel)


## The bar along the bottom: the song, the keys and the credit line.
func _draw_bottom_bar(vp: Vector2) -> void:
	var y := vp.y - 44.0
	_hgrad(Rect2(Vector2(0, y), Vector2(vp.x * 0.6, 1.0)), Color(CYAN, 0.0), Color(CYAN, 0.5))
	_hgrad(Rect2(Vector2(vp.x * 0.6, y), Vector2(vp.x * 0.4, 1.0)), Color(CYAN, 0.5), Color(CYAN, 0.0))
	var np := Music.now_playing()
	if not np.is_empty():
		_text(Vector2(28, y + 28), "NOW PLAYING", 13, Color(CYAN, 0.8), HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
		_text(Vector2(28 + caps_font.get_string_size("NOW PLAYING", HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x + 14.0, y + 28), "%s     F9 next" % np, 15, DIM, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	if page == "main":
		_text(Vector2(0, y + 28), "Arrows, D-pad or stick to move   ·   Enter or A to choose   ·   Esc or B to go back", 14, MUTE, HORIZONTAL_ALIGNMENT_CENTER, vp.x, font)
	_text(Vector2(vp.x - 448, y + 28), "Based on %s (c) Stardock.  Non-commercial remake." % Names.ORIGINAL, 13, Color(MUTE, 0.8), HORIZONTAL_ALIGNMENT_RIGHT, 420, font)


## Top right: the build and the updater's news, with Restart once an
## update is downloaded, or a link to the installer when it needs one.
func _draw_update(vp: Vector2) -> void:
	if Updater.state == "off":
		return
	var ok := Updater.state in ["current", "checking", "downloading"]
	var col := Color(DIM, 0.8) if ok else AMBER
	_text(Vector2(vp.x - 724, 34), "Build %s   ·   %s" % [Updater.build, Updater.note], 15, col, HORIZONTAL_ALIGNMENT_RIGHT, 700, font)
	if Updater.state == "ready":
		_button(Rect2(vp.x - 244, 48, 220, 42), "Restart to update", Updater.restart)
	elif Updater.state == "installer":
		_button(Rect2(vp.x - 244, 48, 220, 42), "Get the installer", func(): OS.shell_open(Updater.installer_url), not Updater.installer_url.is_empty())


## A new story over a saved one asks first, like a campaign.  `hosted`:
## for friends, from the Online page.
func _new_story(hosted := false) -> void:
	if SaveGame.exists(SaveGame.STORY_PATH) and story_note.is_empty():
		story_note = "This replaces your saved story (save slots stay). Press New again."
		return
	story_note = ""
	if hosted:
		story_host_pressed.emit(true)
	else:
		story_new_pressed.emit()


## "Operation 2 of 14 next, 0:42:10 in"
func _story_line() -> String:
	var t := int(story_info["elapsed"])
	var op := int(story_info.get("op", 0))
	if op >= Story.OPS.size():
		return "Saved story: every operation so far done, %d:%02d:%02d in" % [t / 3600, (t / 60) % 60, t % 60]
	return "Saved story: operation %d, %s, %d:%02d:%02d in" % [op + 1, Story.OPS[maxi(op, 0)]["title"], t / 3600, (t / 60) % 60, t % 60]


## A new campaign over a saved one asks first: the second press starts it.
func _new_campaign(hosted := false) -> void:
	if SaveGame.exists() and campaign_note.is_empty():
		campaign_note = "This replaces your saved campaign. Press New again."
		return
	campaign_note = ""
	if hosted:
		host_pressed.emit(versus, "new")
	else:
		campaign_new_pressed.emit()


## "Saved campaign: Terran, 1:42:10 in, 6 worlds held"
func _save_line() -> String:
	var info := save_info
	if info.is_empty():
		return ""
	var t := int(info["elapsed"])
	return "Saved campaign: %s, %d:%02d:%02d in, %d world%s held" % [GameData.race(info["race"])["name"], t / 3600, (t / 60) % 60, t % 60, info["held"], "" if info["held"] == 1 else "s"]


## "Rear Admiral Joiner of the Terran Alliance   score 30750   kills 2500"
func _profile_line(which: String) -> String:
	var p: Dictionary = Settings.profile_info(which)
	var c: Dictionary = p["career"]
	var rank := GameData.rank_name(GameData.rank_for(int(c.get("score", 0)), int(c.get("kills", 0))))
	return "%s %s of the %s   score %d   kills %d" % [rank, p["name"], GameData.race(int(p["race"]))["name"], int(c.get("score", 0)), int(c.get("kills", 0))]


## Online: host any mode for friends, co-op (on your side) or competitive
## (against you), on the left; join a game by address or from the LAN on
## the right.
func _draw_online(vp: Vector2) -> void:
	var extra := 130.0
	var total := 820.0 + 30.0 + 520.0
	var x0 := maxf((vp.x - total) * 0.5, 24.0)
	var panel := _mode_panel(vp, "Play online", 6, extra, 820.0, x0)
	var px := panel.position.x + 30.0
	var py := panel.position.y
	_text(Vector2(px, py + 100.0), _profile_line("online"), 17, AMBER, HORIZONTAL_ALIGNMENT_LEFT, panel.size.x - 60.0, label_font)
	_text(Vector2(px, py + 120.0), "Your online pilot (the Pilot page changes it).  The Frontier War and the campaign are your single-player pilot's own.", 14, MUTE, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	_choice(py + 136.0, "Friends play", [[false, "Co-op"], [true, "Competitive"]], versus, func(v):
		versus = v
		story_note = ""
		campaign_note = "", vp, px)
	_text(Vector2(px, py + 200.0), "Co-op: friends join your side against the computer." if not versus else
		"Competitive: friends fly against you, for a rival empire (in Survival, with the raiders).", 15, DIM, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	# 1  the story: co-op only
	var story_line := story_note if not story_note.is_empty() else (_story_line() if not story_info.is_empty() else "Friends fly as Lantern Squadron and share every objective.")
	if versus:
		story_line = "Co-op only: choose Co-op above to host the story."
	_heading_with_chips(_mode_row(panel, 0, extra), "The Frontier War", [["New", _new_story.bind(true), true],
		["Continue", func(): story_host_pressed.emit(false), not story_info.is_empty()]], 26, story_line, not versus,
		AMBER if versus or not story_note.is_empty() else MUTE, true)
	# 2  the saving campaign, hosted
	var has_save := not save_info.is_empty()
	var camp_line := campaign_note if not campaign_note.is_empty() else (_save_line() if has_save else "Your saving war, with friends %s." % ("beside you" if not versus else "against you"))
	_heading_with_chips(_mode_row(panel, 1, extra), "Campaign", [["New", _new_campaign.bind(true), true],
		["Continue", func(): host_pressed.emit(versus, "continue"), has_save]], 26, camp_line, true, AMBER if not campaign_note.is_empty() else MUTE, true)
	# 3 - 6  the rounds
	_row(_mode_row(panel, 2, extra), "Conquest", _conquest_note(), func(): host_pressed.emit(versus, ""), true, 26)
	_row(_mode_row(panel, 3, extra), "Survival", ("Hold the starbase together, up to %d pilots" % Survival.MAX_PLAYERS) if not versus else "You hold the starbase; friends hunt you with the raiders",
		func(): mode_pressed.emit("survival", true, versus), true, 26)
	_row(_mode_row(panel, 4, extra), "Skirmish", ("Your side together against the computer, %d a side" % Skirmish.TEAM_SIZE) if not versus else ("Friends split between the two sides, %d a side" % Skirmish.TEAM_SIZE),
		func(): mode_pressed.emit("skirmish", true, versus), true, 26)
	_row(_mode_row(panel, 5, extra), "Duels", "A free-for-all, everyone against everyone in a %s" % Duel.HULL, func(): duel_pressed.emit(), true, 26)
	_back_button(panel)
	# the join side
	var jp := Rect2(Vector2(panel.end.x + 30.0, panel.position.y), Vector2(520.0, panel.size.y))
	_panel(jp, "Join a game")
	var jx := jp.position.x + 30.0
	var jw := jp.size.x - 60.0
	var y := jp.position.y + 84.0
	_text(Vector2(jx, y + 16), "JOIN BY ADDRESS", 15, Color(CYAN, 0.9), HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
	ip_edit.position = Vector2(jx, y + 30)
	ip_edit.size = Vector2(jw - 120.0, 44)
	_button(Rect2(jx + jw - 110.0, y + 28, 110, 48), "Join", func(): join_pressed.emit(ip_edit.text))
	_text(Vector2(jx, y + 100), "Friends join with your address.  Online, open UDP %d" % Net.PORT, 14, MUTE, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	_text(Vector2(jx, y + 118), "on your router, or use Tailscale.", 14, MUTE, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
	_text(Vector2(jx, y + 160), "GAMES ON THIS NETWORK", 15, Color(CYAN, 0.9), HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
	var gy := y + 176.0
	var games: Array = Net.lan.list()
	var room := int((jp.end.y - 90.0 - gy) / 52.0)
	if games.is_empty():
		_text(Vector2(jx, gy + 22), "Listening... a host on your network", 15, MUTE, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
		_text(Vector2(jx, gy + 42), "shows up here within a few seconds.", 15, MUTE, HORIZONTAL_ALIGNMENT_LEFT, -1, font)
		gy += 56.0
	for g in games.slice(0, maxi(room, 1)):
		var ok: bool = int(g.get("protocol", -1)) == Net.PROTOCOL
		var label := "%s  ·  %s  ·  %d/%d" % [g["name"], _game_kind(g), int(g["players"]), int(g["max"])]
		_button(Rect2(jx, gy, jw, 44), label if ok else "%s  (other version)" % g["name"], func(): join_pressed.emit("%s:%d" % [g["ip"], int(g["port"])]), ok)
		gy += 52.0
	if not online_status.is_empty():
		_text(Vector2(jx, jp.end.y - 40.0), online_status, 15, AMBER, HORIZONTAL_ALIGNMENT_LEFT, jw, font)


## "Survival, competitive": what a host on the LAN is playing.
func _game_kind(g: Dictionary) -> String:
	var mode: String = g.get("mode", "")
	if mode == "story":
		return "The Frontier War"
	if mode == "duel":
		return "duel"
	var name: String = {"survival": "Survival", "skirmish": "Skirmish"}.get(mode, "campaign" if g.get("campaign", false) else "Conquest")
	return name + (", competitive" if g.get("versus", false) else ", co-op")


func _draw_pilot(vp: Vector2) -> void:
	var y := vp.y * 0.4
	var x := vp.x * 0.5 - 300.0
	_panel(Rect2(Vector2(x - 40.0, y - 200.0), Vector2(720.0, 560.0)), "Pilot")
	var profs := []
	for k in Settings.PROFILES:
		profs.append([k, Settings.PROFILE_NAMES[k]])
	_choice(y - 120, "Profile", profs, Settings.profile, func(v):
		Settings.use_profile(v)
		name_edit.text = Settings.pilot_name, vp)
	_text(Vector2(x, y - 52), _profile_line(Settings.profile), 17, AMBER, HORIZONTAL_ALIGNMENT_LEFT, 640, label_font)
	_text(Vector2(x, y - 10), "Pilot name", 21, DIM, HORIZONTAL_ALIGNMENT_LEFT, -1, label_font)
	name_edit.position = Vector2(vp.x * 0.5 - 80, y - 36)
	name_edit.size = Vector2(300, 40)
	var races := []
	for r in [0, 1, 2]:
		races.append([r, GameData.race(r)["name"]])
	_choice(y + 40, "Empire", races, Settings.race_id, func(v): Settings.race_id = v, vp)
	var desc := {
		0: "The %s Empire: swept hulls, home on Saturn's moon Titan and the moons of Jupiter." % Names.ARCEAN,
		1: "The %s Alliance: plated hulls, home on Earth and Luna." % Names.TERRAN,
		2: "The %s Empire: bladed hulls, home on Mars, Venus and the Martian moons." % Names.DRENGIN,
	}
	_text(Vector2(x, y + 115), desc[Settings.race_id], 16, DIM, HORIZONTAL_ALIGNMENT_LEFT, 600, font)
	var rc: Color = GameData.race(Settings.race_id)["color"]
	_glow(Rect2(Vector2(vp.x * 0.5 + 130, y + 110), Vector2(280, 180)), Color(rc, 0.18 * content_alpha))
	ShipArt.draw_ship(self, "Scout", Settings.race_id, 40.0, rc, 0.4, content_alpha, 0.0, Transform2D(-PI * 0.5, Vector2(vp.x * 0.5 + 220, y + 200)))
	ShipArt.draw_ship(self, "Cruiser", Settings.race_id, 48.0, rc, 0.4, content_alpha, 0.0, Transform2D(-PI * 0.5, Vector2(vp.x * 0.5 + 320, y + 200)))
	_button(Rect2(x, y + 160, 200, 44), "Reset career", func(): Settings.reset_career())
	draw_multiline_string(font, Vector2(x, y + 312), "Single player and online keep separate pilots and careers. Practice duels and free flight never count toward either.",
		HORIZONTAL_ALIGNMENT_LEFT, 600, 14, -1, _ink(MUTE))
	_button(Rect2(x, y + 230, 160, 48), "Back", func(): _set_page("main"))


func _draw_settings(vp: Vector2) -> void:
	var y := vp.y * 0.27
	var step := 48.0
	var rows := 10 if in_game else 11
	_panel(Rect2(Vector2(vp.x * 0.5 - 340.0, y - 86.0), Vector2(880.0, step * rows + 180.0)), "Settings")
	_choice(y, "Music", [["all", "Remixes + new tracks"], ["remixes", "Remixes"], ["new", "New tracks"], ["off", "Off"]], Settings.music_mode,
		func(v):
			Settings.music_mode = v
			Music.start(), vp)
	_choice(y + step, "Song 8 remix", [["dubstep", "Dubstep"], ["darkwave", "Darkwave"]], Settings.song8,
		func(v):
			Settings.song8 = v
			Music.start(), vp)
	_choice(y + step * 2, "Dynamic music", [[true, "On"], [false, "Shuffle"]], Settings.dynamic_music,
		func(v): Settings.dynamic_music = v, vp)
	var vols := []
	for v in [0.0, 0.25, 0.5, 0.75, 1.0]:
		vols.append([v, "%d%%" % int(v * 100)])
	_choice(y + step * 3, "Music volume", vols, Settings.music_volume, func(v): Music.set_volume(v), vp)
	_choice(y + step * 4, "Effects volume", vols, Settings.sfx_volume, func(v): Settings.sfx_volume = v, vp)
	_choice(y + step * 5, "Voice volume", vols, Settings.voice_volume, func(v): Settings.voice_volume = v, vp)
	_choice(y + step * 6, "Display", [[true, "Full screen"], [false, "Window"]], Settings.fullscreen,
		func(v):
			Settings.fullscreen = v
			Settings.apply(), vp)
	_choice(y + step * 7, "Camera", [[1.4, "Close"], [1.0, "Normal"], [0.7, "Wide"], [0.5, "Very wide"]], Settings.camera_zoom, _set_camera_zoom, vp)
	_choice(y + step * 8, "Screen shake", [[0.0, "Off"], [0.5, "Light"], [1.0, "Full"]], Settings.screen_shake, func(v): Settings.screen_shake = v, vp)
	_choice(y + step * 9, "Mouse-aimed turrets", [[true, "On"], [false, "Off"]], Settings.mouse_aim, func(v): Settings.mouse_aim = v, vp)
	if not in_game:    # a round's length is set before it starts
		_choice(y + step * 10, "Round length", [[20, "20 min"], [30, "30 min"], [40, "40 min"], [0, "Until conquest"]], Settings.round_minutes, func(v): Settings.round_minutes = v, vp)
	_button(Rect2(vp.x * 0.5 - 300, y + step * rows + 20, 160, 48), "Back", _back)
	_button(Rect2(vp.x * 0.5 - 130, y + step * rows + 20, 260, 48), "Controls and keys", func(): _set_page("controls"))


func _set_camera_zoom(v: float) -> void:
	Settings.camera_zoom = v
	if world != null:
		world.target_zoom = v
		world.camera.zoom = Vector2.ONE * v * World.BASE_ZOOM   # seen even while paused


## Every action with two keyboard / mouse slots and a gamepad slot.  Click
## (or press Enter on) a slot, then press what should do it.
func _draw_controls(vp: Vector2) -> void:
	var rows: Array = Keybinds.actions()
	var cols := [["Flight", "Ship", "Wing"], ["Weapons", "Interface"]]
	var label_w := 236.0
	var slot_w := 132.0
	var col_w := label_w + 3.0 * (slot_w + 6.0)
	var x0 := (vp.x - (col_w * 2.0 + 60.0)) * 0.5
	# the longer column sets the row height: everything, the Alt-fire row and
	# the buttons included, has to end above the bar along the bottom
	var most := 0
	for col in cols:
		var n: int = col.size()
		for row in rows:
			if row[2] in col:
				n += 1
		most = maxi(most, n)
	var limit := vp.y - 56.0                   # the bar along the bottom
	var footer := 12.0 + 40.0 + 14.0 + 46.0 + 24.0   # gap, Alt-fire row, gap, buttons, the panel's padding
	var top := minf(vp.y * 0.265, limit - footer - 26.0 - most * 16.0)
	top = maxf(top, 110.0)
	var row_h := clampf((limit - footer - top - 26.0) / maxf(most, 1.0), 16.0, 25.0)
	var small := row_h < 20.0
	var panel_h := 26.0 + most * row_h + footer + 84.0
	_panel(Rect2(Vector2(x0 - 30.0, top - 84.0), Vector2(col_w * 2.0 + 120.0, panel_h)), "Controls and keys")
	_text(Vector2(0, top - 14), "Click a slot, then press a key, mouse button or gamepad control.   Backspace clears it,  Esc cancels.",
		15, DIM, HORIZONTAL_ALIGNMENT_CENTER, vp.x, font)
	var bottom := top
	for c in 2:
		var x := x0 + c * (col_w + 60.0)
		var y := top + 6.0
		_text(Vector2(x + label_w, y + 14), "KEY", 12, Color(CYAN, 0.8), HORIZONTAL_ALIGNMENT_CENTER, slot_w, caps_font)
		_text(Vector2(x + label_w + slot_w + 6.0, y + 14), "KEY 2", 12, Color(CYAN, 0.8), HORIZONTAL_ALIGNMENT_CENTER, slot_w, caps_font)
		_text(Vector2(x + label_w + 2.0 * (slot_w + 6.0), y + 14), "GAMEPAD", 12, Color(CYAN, 0.8), HORIZONTAL_ALIGNMENT_CENTER, slot_w, caps_font)
		y += 20.0
		for section in cols[c]:
			_text(Vector2(x, y + 17), section.to_upper(), 14, CYAN, HORIZONTAL_ALIGNMENT_LEFT, -1, caps_font)
			_hgrad(Rect2(Vector2(x, y + 21), Vector2(col_w, 1.0)), Color(CYAN, 0.5), Color(CYAN, 0.0))
			y += row_h
			for row in rows:
				if row[2] != section:
					continue
				_text(Vector2(x + 8, y + (14 if small else 16)), row[1], 14 if small else 16, INK, HORIZONTAL_ALIGNMENT_LEFT, label_w - 12, font)
				var sx := x + label_w
				for slot in ["kb0", "kb1", "pad"]:
					var waiting: bool = capture_action == row[0] and capture_slot == slot
					var text := "press..." if waiting else Keybinds.slot_text(row[0], slot)
					if waiting and capture_mod != null:
						text = Keybinds.describe({"k": capture_mod.physical_keycode}) + "+..."
					_slot_button(Rect2(sx, y + 1, slot_w, row_h - 3), text, _start_capture.bind(row[0], slot), waiting, 12 if small else 14)
					sx += slot_w + 6.0
				y += row_h
		bottom = maxf(bottom, y)
	var by := bottom + 12.0   # under the longer column, however many actions there are
	_choice(by, "Alt-fire shoots", [[1, "Group 1"], [2, "Group 2"], [3, "Group 3"], [4, "Group 4"]], Settings.alt_fire_group,
		func(v): Settings.alt_fire_group = v, vp)
	if not controls_status.is_empty():
		_text(Vector2(vp.x * 0.5 + 300, by + 27), controls_status, 15, AMBER, HORIZONTAL_ALIGNMENT_LEFT, 520, font)
	_button(Rect2(vp.x * 0.5 - 300, by + 54, 160, 46), "Back", _back)
	_button(Rect2(vp.x * 0.5 - 130, by + 54, 260, 46), "Reset to defaults", func():
		Keybinds.reset_all()
		controls_status = "All controls are back to the defaults.")


func _start_capture(action: String, slot: String) -> void:
	capture_action = action
	capture_slot = slot
	capture_since = Time.get_ticks_msec()
	capture_mod = null
	controls_status = "Press the new control for %s..." % _action_label(action)


func _slot_button(rect: Rect2, text: String, cb: Callable, waiting: bool, size := 14) -> void:
	buttons.append([rect, text, cb, true])
	var idx := buttons.size() - 1
	var lit := 1.0 if hover == idx or focus == idx else 0.0
	var bg := Color(0.03, 0.06, 0.12, 0.8).lerp(GLASS_LIT, lit)
	if waiting:
		bg = Color(0.5, 0.38, 0.1, 0.55 + 0.3 * sin(time * 8.0))
	draw_rect(rect, _ink(bg))
	draw_rect(rect, _ink(EDGE.lerp(EDGE_LIT, lit) if not waiting else AMBER), false, 1.0)
	if focus == idx:
		draw_rect(rect.grow(2), _ink(Color(1.0, 0.95, 0.75, 0.45 + 0.3 * sin(time * 6.0))), false, 1.0)
	_text(rect.position + Vector2(0, rect.size.y * 0.5 + 5), text if not text.is_empty() else "-", size,
		INK if not text.is_empty() else Color(0.4, 0.45, 0.55), HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, label_font)
