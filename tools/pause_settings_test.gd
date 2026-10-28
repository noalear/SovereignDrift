# Pause-menu settings test: the title screen's Settings page opened over a
# running game, with real clicks and key presses.  Changes apply at once,
# Esc steps back to the pause menu, and nothing meant for the menu reaches
# the ship, even online where the round goes on behind it.
#   godot --headless --path . -s tools/pause_settings_test.gd
extends SceneTree

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "settings_menu" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	var settings = root.get_node("/root/Settings")
	var saved := [settings.sfx_volume, settings.music_volume, settings.camera_zoom, settings.screen_shake]
	await process_frame
	main._start_free_flight()
	await process_frame
	await process_frame
	var w = main.world
	var p = w.player
	# Esc opens the pause menu, which has a Settings button
	_key(KEY_ESCAPE)
	await process_frame
	_check(main.pause_panel != null, "Esc opens the pause menu")
	_check(w.menu_open, "the world knows a menu is open")
	var btn: Button = null
	for b in main.pause_panel.find_children("*", "Button", true, false):
		if b.text == "Settings":
			btn = b
	_check(btn != null, "the pause menu has a Settings button")
	if btn == null:
		_finish(settings, saved)
		return
	btn.pressed.emit()
	await process_frame
	await process_frame
	var m = main.settings_menu
	_check(m != null and m.in_game and m.page == "settings", "Settings opens the Settings page over the game")
	_check(not main.pause_panel.visible, "the pause menu steps aside while Settings is open")
	_check(main.world == w and main.menu == null, "the game is still running behind it (no title screen)")
	if m == null:
		_finish(settings, saved)
		return
	var labels: Array = m.buttons.map(func(b): return b[1])
	_check(not "Until conquest" in labels, "Round length is left out in game (%d buttons)" % labels.size())
	_check("Full screen" in labels and "Window" in labels and "Wide" in labels and "Off" in labels, "graphics settings are there: display, camera, screen shake")
	_check("Controls and keys" in labels, "Controls and keys is reachable in game")
	# effects volume 25%: the second "25%" row (music volume is the first)
	var vols := _all(m, "25%")
	_check(vols.size() == 2, "music and effects volume rows are drawn")
	if vols.size() == 2:
		_click(m.buttons[vols[1]][0].get_center())
		await process_frame
		_check(is_equal_approx(settings.sfx_volume, 0.25), "clicking 25%% sets the effects volume (%.2f)" % settings.sfx_volume)
		_click(m.buttons[vols[0]][0].get_center())
		await process_frame
		_check(is_equal_approx(settings.music_volume, 0.25), "clicking 25%% sets the music volume (%.2f)" % settings.music_volume)
	var wide := _all(m, "Wide")
	if not wide.is_empty():
		_click(m.buttons[wide[0]][0].get_center())
		await process_frame
		_check(is_equal_approx(w.target_zoom, 0.7) and is_equal_approx(w.camera.zoom.x, 0.7 * w.BASE_ZOOM), "Camera: Wide zooms the game out at once (%.2f)" % w.camera.zoom.x)
	# online the round goes on behind the menu: the ship must not fly or shoot
	w.set_paused(false)
	await physics_frame
	Input.action_press("fire_primary")
	Input.action_press("thrust")
	var light := _all(m, "Light")
	if not light.is_empty():
		_click(m.buttons[light[0]][0].get_center())
	await physics_frame
	await physics_frame
	_check(is_equal_approx(settings.screen_shake, 0.5), "Screen shake: Light (%.2f)" % settings.screen_shake)
	var fired := false
	for k in p.inputs:
		if typeof(p.inputs[k]) == TYPE_BOOL and p.inputs[k]:
			fired = true
	_check(not fired and is_zero_approx(float(p.inputs.get("thrust", 0.0))), "clicks and held keys in the menu reach no gun or engine")
	Input.action_release("fire_primary")
	Input.action_release("thrust")
	w.set_paused(true)
	# Controls page, then Esc twice back to the pause menu
	var ck := _all(m, "Controls and keys")
	if not ck.is_empty():
		_click(m.buttons[ck[0]][0].get_center())
		await process_frame
		_check(m.page == "controls", "Controls and keys opens the key page")
		_key(KEY_ESCAPE)
		await process_frame
		_check(main.settings_menu == m and m.page == "settings", "Esc on Controls goes back to Settings")
	_key(KEY_ESCAPE)
	await process_frame
	await process_frame
	_check(main.settings_menu == null, "Esc on Settings closes it")
	_check(main.pause_panel != null and main.pause_panel.visible, "and the pause menu is back")
	_check(w.paused, "the game is still paused")
	var cfg := ConfigFile.new()
	cfg.load("user://settings.cfg")
	_check(is_equal_approx(float(cfg.get_value("audio", "sfx_volume", -1.0)), 0.25), "the new settings are saved")
	_key(KEY_ESCAPE)
	await process_frame
	await process_frame
	_check(main.pause_panel == null, "Esc on the pause menu resumes")
	_check(not w.menu_open and not w.paused, "resuming gives the ship back its controls")
	_finish(settings, saved)


func _finish(settings, saved: Array) -> void:
	settings.sfx_volume = saved[0]
	settings.music_volume = saved[1]
	settings.camera_zoom = saved[2]
	settings.screen_shake = saved[3]
	settings.save_settings()
	print("pause settings test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _all(menu, label: String) -> Array:
	var out := []
	for k in menu.buttons.size():
		if menu.buttons[k][1] == label:
			out.append(k)
	return out


func _key(code: int) -> void:
	var ev := InputEventKey.new()
	ev.physical_keycode = code
	ev.keycode = code
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


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


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
