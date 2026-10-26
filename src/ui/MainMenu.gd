## MainMenu -- title screen with pilot, settings and play.
class_name MainMenu
extends Control

signal play_pressed()
signal practice_pressed()
signal free_flight_pressed()
signal campaign_new_pressed()
signal campaign_continue_pressed()
signal host_pressed()
signal duel_pressed()
signal join_pressed(address: String)
signal quit_pressed()

var font: Font
var page := "main"      # main | pilot | settings | about
var buttons: Array = []   # [rect, label, callback, enabled]
var hover := -1
var focus := 0            # keyboard / gamepad focus, an index into buttons
var name_edit: LineEdit
var ip_edit: LineEdit
var online_status := ""
var time := 0.0
var demo_ships: Array = []
var capture_action := ""     # Controls page: the action waiting for a key
var capture_slot := ""       # "kb0", "kb1" or "pad"
var capture_since := 0
var controls_status := ""
var campaign_note := ""      # the warning before a new campaign replaces the saved one
var save_info := {}          # the saved campaign's summary, read once


func _ready() -> void:
	font = ThemeDB.fallback_font
	save_info = SaveGame.summary()
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	name_edit = LineEdit.new()
	name_edit.max_length = 16
	name_edit.text = Settings.pilot_name
	name_edit.placeholder_text = "Pilot name"
	name_edit.add_theme_font_size_override("font_size", 20)
	name_edit.text_changed.connect(func(t): Settings.pilot_name = t.strip_edges() if not t.strip_edges().is_empty() else "Pilot")
	add_child(name_edit)
	name_edit.hide()
	ip_edit = LineEdit.new()
	ip_edit.max_length = 64
	ip_edit.text = Settings.last_join_ip
	ip_edit.placeholder_text = "Host address, e.g. 192.168.1.20"
	ip_edit.add_theme_font_size_override("font_size", 20)
	ip_edit.text_submitted.connect(func(t): join_pressed.emit(t))
	add_child(ip_edit)
	ip_edit.hide()
	var rng := RandomNumberGenerator.new()
	rng.seed = 7
	for i in 7:
		demo_ships.append({"pos": Vector2(rng.randf_range(0, 1920), rng.randf_range(200, 1000)), "vel": Vector2(rng.randf_range(20, 60), rng.randf_range(-8, 8)),
			"design": GameData.REFIT_ORDER[rng.randi() % 6], "race": rng.randi() % 3, "size": rng.randf_range(14, 40)})


func _process(delta: float) -> void:
	time += delta
	for d in demo_ships:
		d["pos"] += d["vel"] * delta
		var vp := get_viewport_rect().size
		if d["pos"].x > vp.x + 100:
			d["pos"].x = -100
			d["pos"].y = randf_range(150, vp.y - 150)
	queue_redraw()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
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
## gamepad control goes into it, before anything else sees it.
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
	if not pressed:
		if not event is InputEventMouseMotion:
			get_viewport().set_input_as_handled()
		return
	get_viewport().set_input_as_handled()
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


func _unhandled_input(event: InputEvent) -> void:
	if (event.is_action_pressed("pause_menu") or event.is_action_pressed("ui_cancel")) and page != "main":
		Sfx.play_ui("ui_back", 0.6)
		_set_page("settings" if page == "controls" else "main")
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
	page = p
	focus = 0
	hover = -1
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


func _button(rect: Rect2, label: String, cb: Callable, enabled: bool = true) -> void:
	buttons.append([rect, label, cb, enabled])
	var idx := buttons.size() - 1
	var lit := hover == idx or focus == idx
	var bg := Color(0.08, 0.14, 0.28, 0.85) if not lit else Color(0.18, 0.3, 0.55, 0.95)
	if not enabled:
		bg = Color(0.1, 0.1, 0.12, 0.6)
	draw_rect(rect, bg)
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.7 if enabled else 0.25), false, 1.5)
	if focus == idx and enabled:
		draw_rect(rect.grow(3), Color(1.0, 0.95, 0.7, 0.6 + 0.3 * sin(time * 6.0)), false, 2.0)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + 7), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, 20, Color(1, 1, 1) if enabled else Color(0.5, 0.5, 0.5))


func _choice(y: float, label: String, options: Array, current: Variant, setter: Callable, vp: Vector2) -> void:
	var x := vp.x * 0.5 - 300.0
	draw_string(font, Vector2(x, y + 27), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.85, 0.9, 1.0))
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
		var w := 20.0 + font.get_string_size(str(o[1]), HORIZONTAL_ALIGNMENT_LEFT, -1, 18).x
		var rect := Rect2(bx, y, w, 40)
		var selected: bool = o[0] == match_value
		var idx := buttons.size()
		buttons.append([rect, o[1], setter.bind(o[0]), true])
		var lit := hover == idx or focus == idx
		var bg := Color(0.25, 0.45, 0.8, 0.95) if selected else (Color(0.08, 0.14, 0.28, 0.85) if not lit else Color(0.18, 0.3, 0.55, 0.95))
		draw_rect(rect, bg)
		draw_rect(rect, Color(0.5, 0.7, 1.0, 0.7), false, 1.0)
		if focus == idx:
			draw_rect(rect.grow(3), Color(1.0, 0.95, 0.7, 0.6 + 0.3 * sin(time * 6.0)), false, 2.0)
		draw_string(font, rect.position + Vector2(10, 27), str(o[1]), HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color.WHITE)
		bx += w + 8.0


func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	# backdrop ships
	for d in demo_ships:
		ShipArt.draw_ship(self, d["design"], d["race"], d["size"], GameData.race(d["race"])["color"], 0.6, 0.35, 0.0, Transform2D(0.0, d["pos"]))
	# title
	var ty := vp.y * 0.18
	draw_string(font, Vector2(3, ty + 3), "STELLAR FRONTIER", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 84, Color(0, 0, 0, 0.6))
	draw_string(font, Vector2(0, ty), "STELLAR FRONTIER", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 84, Color(0.85, 0.92, 1.0))
	draw_string(font, Vector2(0, ty + 40), "R E M A K E", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 24, Color(0.6, 0.75, 1.0, 0.8 + 0.2 * sin(time * 2.0)))
	match page:
		"main":
			_draw_main(vp)
		"pilot":
			_draw_pilot(vp)
		"settings":
			_draw_settings(vp)
		"about":
			_draw_about(vp)
		"online":
			_draw_online(vp)
		"controls":
			_draw_controls(vp)
	var np := Music.now_playing()
	if not np.is_empty():
		draw_string(font, Vector2(24, vp.y - 20), "Now playing: %s     (F9: next song)" % np, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.8, 0.8, 0.95, 0.8))
	draw_string(font, Vector2(vp.x - 420, vp.y - 20), "Based on Stellar Frontier (c) Stardock. Non-commercial remake.", HORIZONTAL_ALIGNMENT_RIGHT, 400, 13, Color(1, 1, 1, 0.4))


func _draw_main(vp: Vector2) -> void:
	var w := 320.0
	var x := (vp.x - w) * 0.5
	var y := vp.y * 0.42
	draw_string(font, Vector2(0, y - 20), _profile_line("single"), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 17, Color(1, 0.95, 0.8))
	_button(Rect2(x, y, w, 52), "Play: the Sol system", func(): play_pressed.emit())
	var has_save := not save_info.is_empty()
	_button(Rect2(x, y + 64, w * 0.5 - 4, 52), "New campaign", _new_campaign)
	_button(Rect2(x + w * 0.5 + 4, y + 64, w * 0.5 - 4, 52), "Continue", func(): campaign_continue_pressed.emit(), has_save)
	if not campaign_note.is_empty():
		draw_string(font, Vector2(x + w + 16, y + 96), campaign_note, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1.0, 0.85, 0.55))
	elif has_save:
		draw_string(font, Vector2(x + w + 16, y + 96), _save_line(), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.7, 0.8, 0.95))
	_button(Rect2(x, y + 128, w * 0.5 - 4, 52), "Practice duel", func(): practice_pressed.emit())
	_button(Rect2(x + w * 0.5 + 4, y + 128, w * 0.5 - 4, 52), "Free flight", func(): free_flight_pressed.emit())
	_button(Rect2(x, y + 192, w, 52), "Play online", func(): _set_page("online"))
	_button(Rect2(x, y + 256, w * 0.5 - 4, 52), "Pilot", func(): _set_page("pilot"))
	_button(Rect2(x + w * 0.5 + 4, y + 256, w * 0.5 - 4, 52), "Settings", func(): _set_page("settings"))
	_button(Rect2(x, y + 320, w * 0.5 - 4, 52), "About", func(): _set_page("about"))
	_button(Rect2(x + w * 0.5 + 4, y + 320, w * 0.5 - 4, 52), "Quit", func(): quit_pressed.emit())
	draw_string(font, Vector2(0, y + 404), "Arrow keys, D-pad or stick to move   ·   Enter or A to choose   ·   Esc or B to go back", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 14, Color(0.6, 0.7, 0.85))


## A new campaign over a saved one asks first: the second press starts it.
func _new_campaign() -> void:
	if SaveGame.exists() and campaign_note.is_empty():
		campaign_note = "This replaces your saved campaign. Press New campaign again to start over."
		return
	campaign_note = ""
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


func _draw_online(vp: Vector2) -> void:
	var x := vp.x * 0.5 - 300.0
	var y := vp.y * 0.36
	draw_string(font, Vector2(0, y - 50), "Online profile: " + _profile_line("online"), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 17, Color(1, 0.95, 0.8))
	draw_string(font, Vector2(0, y - 28), "Change it on the Pilot page.", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 13, Color(0.65, 0.72, 0.85))
	draw_string(font, Vector2(x, y), "Co-op: friends drop in as %s pilots.  Duel: a free-for-all, everyone against everyone." % GameData.race(int(Settings.profile_info("online")["race"]))["name"],
		HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.8, 0.85, 0.95))
	_button(Rect2(x, y + 20, 296, 48), "Host co-op (port %d)" % Net.PORT, func(): host_pressed.emit())
	_button(Rect2(x + 304, y + 20, 296, 48), "Host a duel (free-for-all)", func(): duel_pressed.emit())
	draw_string(font, Vector2(x, y + 90), "Friends join with your address. Online, open UDP %d on your router or use Tailscale." % Net.PORT,
		HORIZONTAL_ALIGNMENT_LEFT, 600, 13, Color(0.7, 0.75, 0.85))
	y += 30
	draw_string(font, Vector2(x, y + 110), "Join by address", HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.85, 0.9, 1.0))
	ip_edit.position = Vector2(x + 180, y + 84)
	ip_edit.size = Vector2(300, 40)
	_button(Rect2(x + 490, y + 80, 110, 48), "Join", func(): join_pressed.emit(ip_edit.text))
	draw_string(font, Vector2(x, y + 170), "Games on this network", HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.85, 0.9, 1.0))
	var games: Array = Net.lan.list()
	var gy := y + 190
	if games.is_empty():
		draw_string(font, Vector2(x, gy + 22), "Listening... a host on your LAN shows up here within a few seconds.", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.65, 0.75))
		gy += 40
	for g in games.slice(0, 5):
		var label := "%s  (%s, %s, %d/%d players)  %s" % [g["name"], "duel" if g.get("mode", "") == "duel" else "co-op", g.get("system", "Sol"), int(g["players"]), int(g["max"]), g["ip"]]
		var ok: bool = int(g.get("protocol", -1)) == Net.PROTOCOL
		_button(Rect2(x, gy, 600, 40), label if ok else label + "  (different version)", func(): join_pressed.emit("%s:%d" % [g["ip"], int(g["port"])]), ok)
		gy += 48
	if not online_status.is_empty():
		draw_string(font, Vector2(x, gy + 30), online_status, HORIZONTAL_ALIGNMENT_LEFT, 640, 16, Color(1.0, 0.9, 0.6))
	_button(Rect2(x, gy + 56, 160, 48), "Back", func(): _set_page("main"))


func _draw_pilot(vp: Vector2) -> void:
	var y := vp.y * 0.4
	var profs := []
	for k in Settings.PROFILES:
		profs.append([k, Settings.PROFILE_NAMES[k]])
	_choice(y - 120, "Profile", profs, Settings.profile, func(v):
		Settings.use_profile(v)
		name_edit.text = Settings.pilot_name, vp)
	draw_string(font, Vector2(0, y - 52), _profile_line(Settings.profile), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 16, Color(1, 0.95, 0.8))
	draw_string(font, Vector2(vp.x * 0.5 - 300, y - 10), "Pilot name", HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.85, 0.9, 1.0))
	name_edit.position = Vector2(vp.x * 0.5 - 80, y - 36)
	name_edit.size = Vector2(300, 40)
	var races := []
	for r in [0, 1, 2]:
		races.append([r, GameData.race(r)["name"]])
	_choice(y + 40, "Empire", races, Settings.race_id, func(v): Settings.race_id = v, vp)
	var desc := {
		0: "The Arcean Empire: swept hulls, home on Saturn's moon Titan and the moons of Jupiter.",
		1: "The Terran Alliance: plated hulls, home on Earth and Luna.",
		2: "The Drengin Empire: bladed hulls, home on Mars, Venus and the Martian moons.",
	}
	draw_string(font, Vector2(vp.x * 0.5 - 300, y + 115), desc[Settings.race_id], HORIZONTAL_ALIGNMENT_LEFT, 600, 15, Color(0.8, 0.85, 0.95))
	ShipArt.draw_ship(self, "Scout", Settings.race_id, 40.0, GameData.race(Settings.race_id)["color"], 0.4, 1.0, 0.0, Transform2D(-PI * 0.5, Vector2(vp.x * 0.5 + 220, y + 200)))
	ShipArt.draw_ship(self, "Cruiser", Settings.race_id, 48.0, GameData.race(Settings.race_id)["color"], 0.4, 1.0, 0.0, Transform2D(-PI * 0.5, Vector2(vp.x * 0.5 + 320, y + 200)))
	_button(Rect2(vp.x * 0.5 - 300, y + 160, 200, 44), "Reset career", func(): Settings.reset_career())
	draw_multiline_string(font, Vector2(vp.x * 0.5 - 300, y + 312), "Single player and online keep separate pilots and careers. Practice duels and free flight never count toward either.",
		HORIZONTAL_ALIGNMENT_LEFT, 600, 14, -1, Color(0.7, 0.76, 0.88))
	_button(Rect2(vp.x * 0.5 - 300, y + 230, 160, 48), "Back", func(): _set_page("main"))


func _draw_settings(vp: Vector2) -> void:
	var y := vp.y * 0.27
	var step := 48.0
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
	_choice(y + step * 5, "Display", [[true, "Full screen"], [false, "Window"]], Settings.fullscreen,
		func(v):
			Settings.fullscreen = v
			Settings.apply(), vp)
	_choice(y + step * 6, "Camera", [[1.4, "Close"], [1.0, "Normal"], [0.7, "Wide"], [0.5, "Very wide"]], Settings.camera_zoom, func(v): Settings.camera_zoom = v, vp)
	_choice(y + step * 7, "Screen shake", [[0.0, "Off"], [0.5, "Light"], [1.0, "Full"]], Settings.screen_shake, func(v): Settings.screen_shake = v, vp)
	_choice(y + step * 8, "Mouse-aimed turrets", [[true, "On"], [false, "Off"]], Settings.mouse_aim, func(v): Settings.mouse_aim = v, vp)
	_choice(y + step * 9, "Round length", [[20, "20 min"], [30, "30 min"], [40, "40 min"], [0, "Until conquest"]], Settings.round_minutes, func(v): Settings.round_minutes = v, vp)
	_button(Rect2(vp.x * 0.5 - 300, y + step * 10 + 20, 160, 48), "Back", func(): _set_page("main"))
	_button(Rect2(vp.x * 0.5 - 130, y + step * 10 + 20, 260, 48), "Controls and keys", func(): _set_page("controls"))


## Every action with two keyboard / mouse slots and a gamepad slot.  Click
## (or press Enter on) a slot, then press what should do it.
func _draw_controls(vp: Vector2) -> void:
	var top := vp.y * 0.265
	draw_string(font, Vector2(0, top - 14), "Click a slot, then press a key, mouse button or gamepad control.   Backspace clears it,  Esc cancels.",
		HORIZONTAL_ALIGNMENT_CENTER, vp.x, 15, Color(0.75, 0.82, 0.95))
	var rows: Array = Keybinds.actions()
	var cols := [["Flight", "Ship"], ["Weapons", "Interface"]]
	var label_w := 220.0
	var slot_w := 132.0
	var col_w := label_w + 3.0 * (slot_w + 6.0)
	var x0 := (vp.x - (col_w * 2.0 + 60.0)) * 0.5
	var row_h := 25.0
	for c in 2:
		var x := x0 + c * (col_w + 60.0)
		var y := top + 6.0
		draw_string(font, Vector2(x + label_w, y + 14), "Key", HORIZONTAL_ALIGNMENT_CENTER, slot_w, 13, Color(0.6, 0.75, 1.0))
		draw_string(font, Vector2(x + label_w + slot_w + 6.0, y + 14), "Key 2", HORIZONTAL_ALIGNMENT_CENTER, slot_w, 13, Color(0.6, 0.75, 1.0))
		draw_string(font, Vector2(x + label_w + 2.0 * (slot_w + 6.0), y + 14), "Gamepad", HORIZONTAL_ALIGNMENT_CENTER, slot_w, 13, Color(0.6, 0.75, 1.0))
		y += 20.0
		for section in cols[c]:
			draw_string(font, Vector2(x, y + 17), section.to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.6, 0.8, 1.0))
			y += row_h
			for row in rows:
				if row[2] != section:
					continue
				draw_string(font, Vector2(x + 8, y + 16), row[1], HORIZONTAL_ALIGNMENT_LEFT, label_w - 12, 15, Color(0.9, 0.93, 1.0))
				var sx := x + label_w
				for slot in ["kb0", "kb1", "pad"]:
					var waiting: bool = capture_action == row[0] and capture_slot == slot
					var text := "press..." if waiting else Keybinds.slot_text(row[0], slot)
					_slot_button(Rect2(sx, y + 1, slot_w, row_h - 3), text, _start_capture.bind(row[0], slot), waiting)
					sx += slot_w + 6.0
				y += row_h
	var by := top + 6.0 + 20.0 + 19.0 * row_h + 12.0
	_choice(by, "Alt-fire shoots", [[1, "Group 1"], [2, "Group 2"], [3, "Group 3"], [4, "Group 4"]], Settings.alt_fire_group,
		func(v): Settings.alt_fire_group = v, vp)
	if not controls_status.is_empty():
		draw_string(font, Vector2(vp.x * 0.5 + 300, by + 27), controls_status, HORIZONTAL_ALIGNMENT_LEFT, 520, 15, Color(1.0, 0.9, 0.6))
	_button(Rect2(vp.x * 0.5 - 300, by + 54, 160, 46), "Back", func(): _set_page("settings"))
	_button(Rect2(vp.x * 0.5 - 130, by + 54, 260, 46), "Reset to defaults", func():
		Keybinds.reset_all()
		controls_status = "All controls are back to the defaults.")


func _start_capture(action: String, slot: String) -> void:
	capture_action = action
	capture_slot = slot
	capture_since = Time.get_ticks_msec()
	controls_status = "Press the new control for %s..." % _action_label(action)


func _slot_button(rect: Rect2, text: String, cb: Callable, waiting: bool) -> void:
	buttons.append([rect, text, cb, true])
	var idx := buttons.size() - 1
	var lit := hover == idx or focus == idx
	var bg := Color(0.06, 0.1, 0.2, 0.85) if not lit else Color(0.18, 0.3, 0.55, 0.95)
	if waiting:
		bg = Color(0.5, 0.4, 0.1, 0.6 + 0.3 * sin(time * 8.0))
	draw_rect(rect, bg)
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.45), false, 1.0)
	if focus == idx:
		draw_rect(rect.grow(2), Color(1.0, 0.95, 0.7, 0.6 + 0.3 * sin(time * 6.0)), false, 2.0)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + 5), text if not text.is_empty() else "-", HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, 13,
		Color(1, 1, 1) if not text.is_empty() else Color(0.45, 0.5, 0.6))


func _draw_about(vp: Vector2) -> void:
	var lines := [
		"Stellar Frontier was made by Stardock in 1997: a real-time 2-D space combat game where",
		"three empires fight for the planets of a solar system, and every ship, from scout to",
		"starbase, is flown by a player or a computer pilot with a rank and a name.",
		"",
		"This remake is built from scratch on Godot 4.  Its rules (ships, modules, weapons, ranks,",
		"the Sol scenario) come from the original game's data files.  The art is new.",
		"",
		"Soundtrack: genre remixes of the eight original songs, and ten new tracks built from",
		"unused fragments of them.",
		"",
		"Non-commercial use only, under the Stardock Shared Source Stellar Frontier License.",
	]
	var y := vp.y * 0.38
	for l in lines:
		draw_string(font, Vector2(0, y), l, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 17, Color(0.9, 0.93, 1.0))
		y += 24
	_button(Rect2(vp.x * 0.5 - 80, y + 30, 160, 48), "Back", func(): _set_page("main"))
