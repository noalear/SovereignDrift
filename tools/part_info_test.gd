# Shipyard descriptions test: every part, hull, mount, part type and line
# of the performance panel has a mouse-over description with something in
# it and no unfilled placeholders; the description shows for the part
# under the pointer and for the keyboard/gamepad focus after a short rest;
# the performance lines are a section of their own that the keys reach;
# and a ship can carry one sensor of each kind.
#   godot --headless --path . -s tools/part_info_test.gd
extends SceneTree

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	await process_frame
	await process_frame
	var w = main.world
	var p = w.player
	var base = w.starbase_of(p.race_id)
	p.global_position = base.global_position + Vector2(base.size + 80, 0)
	p.velocity = base.velocity
	w.try_refit(p)
	await process_frame
	await process_frame
	var b = main.builder
	_check(b != null, "the shipyard opened")
	if b == null:
		quit(1)
		return
	var gd = root.get_node("/root/GameData")
	var info = load("res://src/ui/PartInfo.gd")
	var st: Dictionary = gd.stats_for(b.current_design())

	# every part a yard can sell or a wreck can drop
	var bad := []
	for id in gd.MODULES:
		if id in gd.STATION_ONLY:
			continue
		var t: Dictionary = info.part(id, st, p.race_id)
		var why := _bad_tip(t, 4)
		if why != "":
			bad.append("%s (%s): %s" % [gd.MODULES[id]["name"], id, why])
	_check(bad.is_empty(), "every part has a full description %s" % [bad])
	# an overloaded reactor shows up in a weapon's energy cost
	var hot := st.duplicate()
	hot["overload"] = 2.0
	var mdrv: Dictionary = info.part("4001", hot, p.race_id)
	_check(_text(mdrv).contains("overloaded, so each shot costs"), "a weapon's description says what an overloaded reactor does to its cost now")
	# hulls, part types, empty mounts, the bays, fire groups
	bad.clear()
	for h in gd.yard_hulls():
		var why := _bad_tip(info.hull(h, p.race_id), 3)
		if why != "":
			bad.append("hull %s: %s" % [h, why])
	for t in b.TYPE_ORDER:
		if t != "Other" and _bad_tip(info.part_type(t), 1) != "":
			bad.append("type %s" % t)
	if _bad_tip(info.mount(0, 1, 180), 3) != "" or _bad_tip(info.bays(500, 1300), 3) != "" or _bad_tip(info.fire_group(2), 3) != "":
		bad.append("mount, bays or fire group")
	_check(bad.is_empty(), "hulls, part types, mounts, bays and fire groups have descriptions %s" % [bad])

	# the performance panel: every line drawn has a description
	b.queue_redraw()
	await process_frame
	await process_frame
	var keys: Array = b._stat_keys.duplicate()
	bad.clear()
	_check(keys.size() >= 14, "the performance panel's lines are pointable (%d)" % keys.size())
	for k in keys:
		var why := _bad_tip(b._tip_for(k), 1)
		if why != "":
			bad.append("%s: %s" % [k, why])
	_check(bad.is_empty(), "every performance line has a description %s" % [bad])
	# every key the screen draws has one
	var none := []
	for btn in b.buttons:
		var k: String = btn[3]
		if k != "" and not k.begins_with("modal:") and not k.begins_with("sock:") and b._tip_for(k).is_empty():
			none.append(k)
	_check(none.is_empty(), "everything the keys can reach has a description %s" % [none])

	# the pointer on a mount: after a short rest its description is drawn
	b.kb_nav = false
	var mi := -1
	for i in b.buttons.size():
		if b.buttons[i][3] == "hp:0":
			mi = i
	b.hover = mi
	b.queue_redraw()
	await process_frame
	_check(b._tip_key == "hp:0", "the pointer on mount 1 picks its description (%s)" % b._tip_key)
	var t0: float = b.time
	while b.time - t0 < b.TIP_DELAY + 0.1:
		await process_frame
	var tip: Dictionary = b._tip_for("hp:0")
	var want: String = gd.MODULES[b.mounts[0]["id"]]["name"] if b.mounts[0] != null else "Mount 1"
	_check(String(tip.get("title", "")).begins_with(want), "mount 1's description is its weapon's (%s)" % tip.get("title", ""))
	b.hover = -1

	# keys and pad: Tab / RB reaches the performance lines, and the focus
	# there shows their description
	b.sel_kind = ""
	b.hold = []
	b.focus_key = "hold:0"
	b.kb_nav = true
	var seen := []
	b.focus_key = "hp:0"
	for n in 6:
		b._command("next")
		seen.append(b._section_of(b.focus_key))
	_check(seen.has("stats"), "Tab / RB reaches the performance lines (%s)" % [seen])
	b.focus_key = keys[0]
	b._command("down")
	_check(b.focus_key == keys[1], "down walks the performance lines (%s)" % b.focus_key)
	b.queue_redraw()
	await process_frame
	t0 = b.time
	while b.time - t0 < b.TIP_DELAY_KEYS + 0.1:
		await process_frame
	_check(b._tip_key == keys[1] and not b._tip_target().is_empty(), "the focused line has its description up (%s)" % b._tip_key)
	# left and right from the ship still go to the parts, not the panel
	b.focus_key = "bay:0"
	b._command("accept")
	await process_frame
	await process_frame
	var from: String = "bay:0"
	b.focus_key = from
	b._command("right")
	_check(b._section_of(b.focus_key) != "stats", "right from the bays doesn't land on the performance panel (%s)" % b.focus_key)

	# one sensor of each kind: an infrared joins a radar instead of replacing it
	b._cancel()
	await process_frame
	w.try_refit(p)
	await process_frame
	await process_frame
	b = main.builder
	var radar := -1
	for i in b.modules.size():
		if gd.MODULES[b.modules[i]["id"]]["func"] == "sensor" and gd.MODULES[b.modules[i]["id"]]["fam"] == "radar":
			radar = i
	for i in range(b.modules.size() - 1, -1, -1):
		if gd.MODULES[b.modules[i]["id"]]["func"] == "sensor" and gd.MODULES[b.modules[i]["id"]]["fam"] != "radar":
			b._unmount_bay(i)
	if radar < 0:
		b.hold = ["7001"]
		b.sel_kind = "bay"
		b.sel_index = b.modules.size()
		b._mount("7001")
	b.credits = 999999
	b.sel_kind = "bay"
	b.sel_index = b.modules.size()
	b._mount("7301")
	var fams := []
	for m in b.modules:
		if gd.MODULES[m["id"]]["func"] == "sensor":
			fams.append(gd.MODULES[m["id"]]["fam"])
	_check(fams.has("radar") and fams.has("infrared"), "a radar and an infrared fit together (%s)" % [fams])
	b.sel_kind = "bay"
	b.sel_index = b.modules.size()
	b._mount("7302")
	var ir := 0
	for m in b.modules:
		if gd.MODULES[m["id"]]["func"] == "sensor" and gd.MODULES[m["id"]]["fam"] == "infrared":
			ir += 1
	_check(ir == 1, "a second infrared replaces the first (%d)" % ir)
	print("part info test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


## Why a description falls short, or "".
func _bad_tip(t: Dictionary, min_sections: int) -> String:
	if t.is_empty():
		return "none"
	if String(t.get("title", "")) == "":
		return "no title"
	var secs: Array = t.get("sections", [])
	if secs.size() < min_sections:
		return "%d sections" % secs.size()
	var all := _text(t)
	var re := RegEx.create_from_string("(?<![0-9])%[-+0-9.]*[dsf]|[{}]| [.,]")
	var hit := re.search(all)
	if hit != null:
		return "left '%s' in: %s" % [hit.get_string(), all.substr(maxi(hit.get_start() - 40, 0), 80)]
	return ""


func _text(t: Dictionary) -> String:
	var all: String = t.get("title", "") + " " + t.get("sub", "") + " " + " ".join(t.get("facts", []))
	for s in t.get("sections", []):
		all += " " + s[0] + " " + s[1]
	return all


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
