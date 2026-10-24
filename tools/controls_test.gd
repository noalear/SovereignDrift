# Controls test: rebinding keys on the Controls page (with real clicks and
# key presses), conflicts, saving, weapon groups in flight, Alt-fire, and
# the shipyard's group badges.
#   godot --headless --path . -s tools/controls_test.gd
extends SceneTree

var main: Node
var failed := false
var kb: Node


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	kb = root.get_node("/root/Keybinds")
	var settings = root.get_node("/root/Settings")
	kb.reset_all()
	await process_frame
	await process_frame
	# defaults
	_check(kb.slot_text("fire_primary", "kb0") == "Space" and kb.slot_text("fire_primary", "kb1") == "Mouse left", "Fire defaults to Space and the left mouse button (%s / %s)" % [kb.slot_text("fire_primary", "kb0"), kb.slot_text("fire_primary", "kb1")])
	_check(kb.slot_text("fire_primary", "pad") == "RT", "Fire on the gamepad is RT (%s)" % kb.slot_text("fire_primary", "pad"))
	_check(kb.slot_text("weapon_group_1", "kb0") == "1" and kb.slot_text("weapon_group_4", "kb0") == "4", "groups 1-4 sit on the number keys")
	# the Controls page: click Boost's first slot, press Z
	var menu = main.menu
	menu._set_page("controls")
	menu.queue_redraw()
	await process_frame
	await process_frame
	var slot := _find(menu, "boost", "kb0")
	_check(slot >= 0, "the Controls page draws a slot for Boost")
	if slot >= 0:
		_click(menu.buttons[slot][0].get_center())
		await process_frame
		_check(menu.capture_action == "boost", "clicking the slot waits for a key")
		await create_timer(0.3).timeout
		_key(KEY_Z)
		await process_frame
		_check(kb.slot_text("boost", "kb0") == "Z", "pressing Z binds Boost to Z (%s)" % kb.slot_text("boost", "kb0"))
		var has_z := false
		for ev in InputMap.action_get_events("boost"):
			if ev is InputEventKey and ev.physical_keycode == KEY_Z:
				has_z = true
		_check(has_z, "the game's input map now has Z on Boost")
	# a key taken from another action moves
	var moved: String = kb.assign("boost", "kb1", _key_event(KEY_SPACE))
	_check(moved == "Fire selected group" and kb.slot_text("fire_primary", "kb0") == "", "Space moved from Fire to Boost (%s)" % moved)
	# saved and loaded again
	kb.custom.clear()
	kb.load_binds()
	kb.apply_all()
	_check(kb.slot_text("boost", "kb0") == "Z" and kb.slot_text("boost", "kb1") == "Space", "the new keys survive a reload")
	kb.reset_all()
	_check(kb.slot_text("boost", "kb0") == "Shift" and kb.slot_text("fire_primary", "kb0") == "Space", "Reset to defaults restores Shift and Space (%s)" % kb.slot_text("boost", "kb0"))
	# weapon groups in flight
	menu._set_page("main")
	main._start_free_flight()
	await process_frame
	await process_frame
	var w = main.world
	var p = w.player
	_check(p.group_fire, "the player fires by weapon group")
	var groups := {}
	for wp in p.weapons:
		groups[int(wp["group"])] = true
	print("  weapons: ", p.weapons.map(func(x): return "%s=G%d" % [x["name"], x["group"]]))
	_key(KEY_2)
	await physics_frame
	_check(p.weapon_group == 2, "pressing 2 selects group 2")
	Input.action_press("fire_primary")
	await physics_frame
	await physics_frame
	_check(p.inputs["fire_secondary"] and not p.inputs["fire_primary"], "Fire with group 2 selected shoots group 2 only")
	Input.action_release("fire_primary")
	settings.alt_fire_group = 3
	Input.action_press("fire_secondary")
	await physics_frame
	await physics_frame
	_check(p.inputs["fire_missile"] and not p.inputs["fire_secondary"], "Alt-fire shoots its own group (3)")
	Input.action_release("fire_secondary")
	settings.alt_fire_group = 2
	_key(KEY_1)
	await physics_frame
	_check(p.weapon_group == 1, "pressing 1 selects group 1")
	# a group from the shipyard sticks to the launched ship
	var base = w.starbase_of(p.race_id)
	p.global_position = base.global_position + Vector2(base.size + 80, 0)
	p.velocity = base.velocity
	w.try_refit(p)
	await process_frame
	await process_frame
	var b = main.builder
	_check(b != null, "the shipyard opened")
	if b != null:
		var first := -1
		for i in b.mounts.size():
			if b.mounts[i] != null:
				first = i
				break
		var before: int = int(b.mounts[first]["group"])
		b.queue_redraw()
		await process_frame
		await process_frame
		var badge := -1
		for k in b.buttons.size():
			var cb: Callable = b.buttons[k][1]
			if cb.get_method() == "_cycle_group" and cb.get_bound_arguments() == [first]:
				badge = k
		_check(badge >= 0, "each mounted weapon has a group badge")
		if badge >= 0:
			_click(b.buttons[badge][0].get_center())
			await process_frame
		var after: int = int(b.mounts[first]["group"])
		_check(after == before % 4 + 1, "clicking the badge moves the weapon to the next group (%d -> %d)" % [before, after])
		b._launch()
		await process_frame
		var row: Array = p.design["weapons"][0]
		_check(row.size() > 6 and int(row[6]) == after, "the launched design keeps the group")
		var fitted := -1
		for wp in p.weapons:
			if wp["id"] == row[0]:
				fitted = int(wp["group"])
				break
		_check(fitted == after, "the ship's weapon fires with its new group")
	kb.reset_all()
	DirAccess.remove_absolute(ProjectSettings.globalize_path("user://keybinds.cfg"))
	print("controls test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _find(menu, action: String, slot: String) -> int:
	for k in menu.buttons.size():
		var cb: Callable = menu.buttons[k][2]
		if cb.get_method() == "_start_capture" and cb.get_bound_arguments() == [action, slot]:
			return k
	return -1


func _key_event(code: int) -> InputEventKey:
	var ev := InputEventKey.new()
	ev.physical_keycode = code
	ev.keycode = code
	ev.pressed = true
	return ev


func _key(code: int) -> void:
	root.push_input(_key_event(code), true)
	var up := _key_event(code)
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
