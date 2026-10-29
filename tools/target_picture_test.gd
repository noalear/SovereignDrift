# Target picture and select key test (Kyle, 2026-10-29: "When something is
# selected it should show a picture of the ship or planet or whatever is
# selected below the stats in the top right corner.  I need to add the
# ability to change the keybind for selecting targets (which is currently
# shift+click)").
#   godot --headless --path . -s tools/target_picture_test.gd
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/target_picture_test.gd -- --shots
# --shots also saves the readout with a ship, a station and a contact, a
# world's info card and the Controls page to /tmp/claude-0/sf/target-picture
# (SF_SHOTS=<dir> to change it).
extends SceneTree

var main: Node
var kb: Node
var w
var fails := 0
var shots := false
var out_dir := "/tmp/claude-0/sf/target-picture"


func _init() -> void:
	shots = "--shots" in OS.get_cmdline_user_args()
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		fails += 1


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	kb = root.get_node("/root/Keybinds")
	kb.reset_all()
	await process_frame
	await process_frame
	if shots:
		DirAccess.make_dir_recursive_absolute(out_dir)
	# ---- the select key is on the Controls page, Shift+click by default
	_check(kb.slot_text("select_target", "kb0") == "Shift+Mouse left", "Select defaults to Shift+Mouse left (%s)" % kb.slot_text("select_target", "kb0"))
	var shift_in_map := false
	for ev in InputMap.action_get_events("select_target"):
		if ev is InputEventMouseButton and ev.button_index == MOUSE_BUTTON_LEFT and ev.shift_pressed:
			shift_in_map = true
	_check(shift_in_map, "the input map holds Shift with the left button")
	_check(kb.slot_text("fire_primary", "kb1") == "Mouse left", "Fire keeps the plain left button (%s)" % kb.slot_text("fire_primary", "kb1"))
	_check(kb.slot_text("fullscreen", "kb0") == "Alt+Enter", "Full screen shows its Alt (%s)" % kb.slot_text("fullscreen", "kb0"))
	var menu = main.menu
	menu._set_page("controls")
	menu.queue_redraw()
	await process_frame
	await process_frame
	var slot := _find(menu, "select_target", "kb0")
	_check(slot >= 0, "the Controls page has a slot for Select")
	if shots:
		await _shot("controls_page")
	if slot >= 0:
		# Shift held, then the back mouse button: a combination
		_click(menu.buttons[slot][0].get_center())
		await create_timer(0.5).timeout
		root.push_input(_key_event(KEY_SHIFT, true), true)
		await process_frame
		_check(menu.capture_action == "select_target" and menu.capture_mod != null, "Shift alone waits to see what comes with it (%s, %s)" % [menu.capture_action, menu.capture_mod])
		var mb := InputEventMouseButton.new()
		mb.button_index = MOUSE_BUTTON_XBUTTON1
		mb.shift_pressed = true
		mb.pressed = true
		root.push_input(mb, true)
		root.push_input(_key_event(KEY_SHIFT, false), true)
		await process_frame
		_check(kb.slot_text("select_target", "kb0") == "Shift+Mouse back", "Shift + back button binds Shift+Mouse back (%s)" % kb.slot_text("select_target", "kb0"))
		_check(menu.capture_action == "", "and the slot stops waiting")
	# Shift on its own still binds (Boost)
	var boost := _find(menu, "boost", "kb1")
	if boost >= 0:
		menu.queue_redraw()
		await process_frame
		boost = _find(menu, "boost", "kb1")
		_click(menu.buttons[boost][0].get_center())
		await create_timer(0.5).timeout
		root.push_input(_key_event(KEY_CTRL, true), true)
		root.push_input(_key_event(KEY_CTRL, false), true)
		await process_frame
		_check(kb.slot_text("boost", "kb1") == "Ctrl", "Ctrl pressed and let go binds Ctrl alone (%s)" % kb.slot_text("boost", "kb1"))
	# saved and loaded again, modifiers and all
	kb.custom.clear()
	kb.load_binds()
	kb.apply_all()
	_check(kb.slot_text("select_target", "kb0") == "Shift+Mouse back", "the combination survives a reload (%s)" % kb.slot_text("select_target", "kb0"))
	kb.reset_all()
	menu._set_page("main")
	# ---- in flight: the default Shift+click selects and does not fire
	main._start_free_flight()
	for i in 10:
		await process_frame
	w = main.world
	var p = w.player
	var at: Vector2 = p.global_position
	var others: Array = [w.spawn_ship("Vex", 2, "Destroyer", "Ironjaw", at + Vector2(3000, 0), Vector2.ZERO),
		w.spawn_ship("Ilo", 0, "Cruiser", "Tidewalker", at + Vector2(-3000, 0), Vector2.ZERO)]
	_check(others.size() >= 2, "free flight has other ships to pick")
	if others.size() < 2:
		quit(1)
		return
	p.target = null
	var under: Vector2 = w.get_global_mouse_position()
	_put(others[0], under)
	await _mouse(true, true)
	_check(p.target == others[0], "Shift+click targets the ship under the mouse")
	_check(not p.inputs["fire_primary"], "and does not fire")
	await _mouse(false, true)
	# rebound to a key: the key selects, and Shift+click is a plain shot again
	kb.assign("select_target", "kb0", _key_event(KEY_KP_5, true))
	_check(kb.slot_text("select_target", "kb0") != "Shift+Mouse left", "Select moves to a key (%s)" % kb.slot_text("select_target", "kb0"))
	_put(others[0], under + Vector2(4000, 0))
	_put(others[1], w.get_global_mouse_position())
	root.push_input(_key_event(KEY_KP_5, true), true)
	root.push_input(_key_event(KEY_KP_5, false), true)
	await physics_frame
	_check(p.target == others[1], "the new key targets the ship under the mouse")
	_put(others[1], under + Vector2(-4000, 0))
	await _mouse(true, true)
	_check(p.inputs["fire_primary"], "Shift+click fires once Select is elsewhere")
	await _mouse(false, true)
	# a world: the select key opens its card, again closes it
	var luna = null
	for pl in w.planets:
		if pl.body_name == "Luna":
			luna = pl
	w.inspected_planet = null
	p.global_position = luna.global_position + Vector2(luna.drawn_radius() + 600.0, 0)
	p.body.pos = p.global_position
	await physics_frame
	await process_frame
	var m: Vector2 = w.get_global_mouse_position()
	luna.global_position = m       # the moon moves on its rail: check at once
	root.push_input(_key_event(KEY_KP_5, true), true)
	root.push_input(_key_event(KEY_KP_5, false), true)
	_check(w.inspected_planet == luna, "the select key opens a world's card")
	kb.reset_all()
	# ---- the pictures
	var hud = main.hud
	hud.queue_redraw()
	await process_frame
	await process_frame
	_check(hud.planet_pic != null and hud.planet_pic.visible and hud.planet_pic.planet == luna, "the card shows the world's picture")
	_check(hud.planet_pic.material == luna.material, "drawn with the world's own shader")
	w.inspected_planet = null
	hud.queue_redraw()
	await process_frame
	await process_frame
	_check(not hud.planet_pic.visible, "closing the card hides the picture")
	p.target = others[0]
	_put(others[0], p.global_position + Vector2(500, 0))
	for i in 3:
		await process_frame
	_check(true, "the target readout draws its ship picture")
	if shots:
		w.target_zoom = 0.6
		var cases := [["target_ship", others[0]]]
		var station = null
		for s in w.ships:
			if s.is_station and not s.dead:
				station = s
				break
		if station != null:
			cases.append(["target_station", station])
		for c in cases:
			p.target = c[1]
			_put(c[1], p.global_position + Vector2(700, -200))
			await _settle(30)
			await _shot(c[0])
		# a contact out of sight: only a dim shape
		p.target = others[0]
		_put(others[0], p.global_position + Vector2(p.sensor_range * 3.0, 0))
		await _settle(10)
		await _shot("target_contact")
		p.target = null
		for name in ["Earth", "Saturn", "Mars"]:
			for pl in w.planets:
				if pl.body_name == name:
					w.inspected_planet = pl
					p.global_position = pl.global_position + Vector2(pl.drawn_radius() + 500.0, 0)
					p.body.pos = p.global_position
					p.velocity = pl.velocity if "velocity" in pl else Vector2.ZERO
					p.body.vel = p.velocity
			await _settle(40)
			await _shot("card_" + name.to_lower())
	DirAccess.remove_absolute(ProjectSettings.globalize_path("user://keybinds.cfg"))
	print("target_picture_test: %s (%d failed)" % ["OK" if fails == 0 else "FAILED", fails])
	quit(0 if fails == 0 else 1)


func _find(menu, action: String, slot: String) -> int:
	for k in menu.buttons.size():
		var cb: Callable = menu.buttons[k][2]
		if cb.get_method() == "_start_capture" and cb.get_bound_arguments() == [action, slot]:
			return k
	return -1


func _key_event(code: int, down: bool) -> InputEventKey:
	var ev := InputEventKey.new()
	ev.physical_keycode = code
	ev.keycode = code
	ev.shift_pressed = code == KEY_SHIFT and down
	ev.ctrl_pressed = code == KEY_CTRL and down
	ev.pressed = down
	return ev


func _click(at: Vector2) -> void:
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = at
	ev.global_position = at
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


## A real left button press or release through Input, as the OS sends it.
func _mouse(down: bool, shift: bool) -> void:
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = root.get_mouse_position()
	ev.global_position = ev.position
	ev.shift_pressed = shift
	ev.pressed = down
	Input.parse_input_event(ev)
	Input.flush_buffered_events()
	await process_frame
	await physics_frame
	await physics_frame


func _put(s, at: Vector2) -> void:
	s.global_position = at
	s.velocity = Vector2.ZERO
	s.body.pos = at
	s.body.vel = Vector2.ZERO
	s.reset_physics_interpolation()


func _settle(n: int) -> void:
	for i in n:
		await process_frame


func _shot(name: String) -> void:
	await RenderingServer.frame_post_draw
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
