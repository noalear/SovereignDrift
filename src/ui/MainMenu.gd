## MainMenu -- title screen with pilot, settings and play.
class_name MainMenu
extends Control

signal play_pressed()
signal host_pressed()
signal join_pressed(address: String)
signal quit_pressed()

var font: Font
var page := "main"      # main | pilot | settings | about
var buttons: Array = []   # [rect, label, callback, enabled]
var hover := -1
var name_edit: LineEdit
var ip_edit: LineEdit
var online_status := ""
var time := 0.0
var demo_ships: Array = []


func _ready() -> void:
	font = ThemeDB.fallback_font
	set_anchors_preset(Control.PRESET_FULL_RECT)
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
		hover = -1
		for i in buttons.size():
			if buttons[i][0].has_point(event.position):
				hover = i
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for i in buttons.size():
			if buttons[i][0].has_point(event.position) and buttons[i][3]:
				Sfx.play_ui("click", 0.5)
				buttons[i][2].call()
				accept_event()
				return


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("pause_menu") and page != "main":
		_set_page("main")
	elif event.is_action_pressed("music_next"):
		Music.next_track()
	elif event.is_action_pressed("fullscreen"):
		Settings.toggle_fullscreen()


func _set_page(p: String) -> void:
	page = p
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
	var bg := Color(0.08, 0.14, 0.28, 0.85) if hover != idx else Color(0.18, 0.3, 0.55, 0.95)
	if not enabled:
		bg = Color(0.1, 0.1, 0.12, 0.6)
	draw_rect(rect, bg)
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.7 if enabled else 0.25), false, 1.5)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + 7), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, 20, Color(1, 1, 1) if enabled else Color(0.5, 0.5, 0.5))


func _choice(y: float, label: String, options: Array, current: Variant, setter: Callable, vp: Vector2) -> void:
	var x := vp.x * 0.5 - 300.0
	draw_string(font, Vector2(x, y + 27), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 20, Color(0.85, 0.9, 1.0))
	var bx := x + 220.0
	for o in options:
		var w := 20.0 + font.get_string_size(str(o[1]), HORIZONTAL_ALIGNMENT_LEFT, -1, 18).x
		var rect := Rect2(bx, y, w, 40)
		var selected: bool = o[0] == current
		var idx := buttons.size()
		buttons.append([rect, o[1], setter.bind(o[0]), true])
		var bg := Color(0.25, 0.45, 0.8, 0.95) if selected else (Color(0.08, 0.14, 0.28, 0.85) if hover != idx else Color(0.18, 0.3, 0.55, 0.95))
		draw_rect(rect, bg)
		draw_rect(rect, Color(0.5, 0.7, 1.0, 0.7), false, 1.0)
		draw_string(font, rect.position + Vector2(10, 27), str(o[1]), HORIZONTAL_ALIGNMENT_LEFT, -1, 18, Color.WHITE)
		bx += w + 8.0


func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	# backdrop ships
	for d in demo_ships:
		draw_set_transform(d["pos"], 0.0, Vector2.ONE)
		ShipArt.draw_ship(self, d["design"], d["race"], d["size"], GameData.race(d["race"])["color"], 0.6, 0.35, 0.0)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
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
	var np := Music.now_playing()
	if not np.is_empty():
		draw_string(font, Vector2(24, vp.y - 20), "Now playing: %s     (F9: next song)" % np, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.8, 0.8, 0.95, 0.8))
	draw_string(font, Vector2(vp.x - 420, vp.y - 20), "Based on Stellar Frontier (c) Stardock. Non-commercial remake.", HORIZONTAL_ALIGNMENT_RIGHT, 400, 13, Color(1, 1, 1, 0.4))


func _draw_main(vp: Vector2) -> void:
	var w := 320.0
	var x := (vp.x - w) * 0.5
	var y := vp.y * 0.42
	var rank := GameData.rank_name(GameData.rank_for(Settings.career["score"], Settings.career["kills"]))
	draw_string(font, Vector2(0, y - 20), "%s %s of the %s   score %d   kills %d" % [rank, Settings.pilot_name, GameData.race(Settings.race_id)["name"], Settings.career["score"], Settings.career["kills"]],
		HORIZONTAL_ALIGNMENT_CENTER, vp.x, 17, Color(1, 0.95, 0.8))
	_button(Rect2(x, y, w, 52), "Play: the Sol system", func(): play_pressed.emit())
	_button(Rect2(x, y + 64, w, 52), "Play online", func(): _set_page("online"))
	_button(Rect2(x, y + 128, w, 52), "Pilot", func(): _set_page("pilot"))
	_button(Rect2(x, y + 192, w, 52), "Settings", func(): _set_page("settings"))
	_button(Rect2(x, y + 256, w, 52), "About", func(): _set_page("about"))
	_button(Rect2(x, y + 320, w, 52), "Quit", func(): quit_pressed.emit())


func _draw_online(vp: Vector2) -> void:
	var x := vp.x * 0.5 - 300.0
	var y := vp.y * 0.36
	draw_string(font, Vector2(x, y), "Co-op over a LAN or the internet. The host runs the round; friends drop in as %s pilots." % GameData.race(Settings.race_id)["name"],
		HORIZONTAL_ALIGNMENT_LEFT, 640, 16, Color(0.8, 0.85, 0.95))
	_button(Rect2(x, y + 20, 280, 48), "Host a game (port %d)" % Net.PORT, func(): host_pressed.emit())
	draw_string(font, Vector2(x + 300, y + 50), "Friends join with your address; on the internet open UDP %d or use Tailscale." % Net.PORT,
		HORIZONTAL_ALIGNMENT_LEFT, 340, 13, Color(0.7, 0.75, 0.85))
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
		var label := "%s  (%s, %d/%d players)  %s" % [g["name"], g.get("system", "Sol"), int(g["players"]), int(g["max"]), g["ip"]]
		var ok: bool = int(g.get("protocol", -1)) == Net.PROTOCOL
		_button(Rect2(x, gy, 600, 40), label if ok else label + "  (different version)", func(): join_pressed.emit("%s:%d" % [g["ip"], int(g["port"])]), ok)
		gy += 48
	if not online_status.is_empty():
		draw_string(font, Vector2(x, gy + 30), online_status, HORIZONTAL_ALIGNMENT_LEFT, 640, 16, Color(1.0, 0.9, 0.6))
	_button(Rect2(x, gy + 56, 160, 48), "Back", func(): _set_page("main"))


func _draw_pilot(vp: Vector2) -> void:
	var y := vp.y * 0.4
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
	draw_set_transform(Vector2(vp.x * 0.5 + 220, y + 200), -PI * 0.5, Vector2.ONE)
	ShipArt.draw_ship(self, "Scout", Settings.race_id, 40.0, GameData.race(Settings.race_id)["color"], 0.4, 1.0, 0.0)
	draw_set_transform(Vector2(vp.x * 0.5 + 320, y + 200), -PI * 0.5, Vector2.ONE)
	ShipArt.draw_ship(self, "Cruiser", Settings.race_id, 48.0, GameData.race(Settings.race_id)["color"], 0.4, 1.0, 0.0)
	draw_set_transform(Vector2.ZERO, 0.0, Vector2.ONE)
	_button(Rect2(vp.x * 0.5 - 300, y + 160, 200, 44), "Reset career", func():
		Settings.career = {"score": 0, "kills": 0, "deaths": 0}
		Settings.save_settings())
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
	_button(Rect2(vp.x * 0.5 - 300, y + step * 9 + 20, 160, 48), "Back", func(): _set_page("main"))


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
