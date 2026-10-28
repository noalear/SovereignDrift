# Shipyard test: parts taken off go to the hold (asking to sell or cancel
# when it is full), stock parts sell for scrap and go back with a traded-in
# hull, keyboard and gamepad commands walk the screen; Equip from the hold (replaces a part of the same kind,
# else fills an empty mount or adds to the bays, greyed when it cannot
# go), and the parts list showing only upgrades for a picked part until
# Switch out is pressed.
#   godot --headless --path . -s tools/shipyard_test.gd
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
	# a weapon of the same kind as the first mounted one replaces it
	var first := -1
	for i in b.mounts.size():
		if b.mounts[i] != null:
			first = i
			break
	var have: String = b.mounts[first]["id"]
	var twin := ""
	for id in gd.MODULES:
		if id != have and not id in gd.STATION_ONLY and b._same_family(id, have) and gd.weapon_class(id) <= b.hardpoints[first][5]:
			twin = id
			break
	if twin.is_empty():
		twin = have            # no other tier fits here: swapping like for like still works
	var was_stock: bool = b.mounts[first]["stock"]
	b.hold = [twin]
	var slot: Array = b._equip_slot(twin)
	_check(slot == ["hp", first], "Equip puts %s in place of %s on mount %d (%s)" % [gd.MODULES[twin]["name"], gd.MODULES[have]["name"], first + 1, slot])
	b._equip_hold(0)
	_check(b.mounts[first]["id"] == twin and b.hold == [have] and b._hold_is_stock(0) == was_stock, "the new part is fitted and the old one is in the hold, marked stock if it was (%s)" % [b.hold])
	# a bay part of a kind already fitted replaces it
	var bay_have: String = b.modules[0]["id"]
	b.hold = [bay_have]
	_check(b._equip_slot(bay_have) == ["bay", 0], "a bay part replaces the one of its kind")
	# something with nowhere to go is greyed
	var heavy := ""
	for id in gd.MODULES:
		if not gd.is_bay_part(id) and gd.weapon_class(id) == 2 and not id in gd.STATION_ONLY:
			heavy = id
			break
	var big_mount := false
	for hp in b.hardpoints:
		if hp[5] >= 2:
			big_mount = true
	if not big_mount and not heavy.is_empty():
		b.hold = [heavy]
		_check(b._equip_slot(heavy).is_empty(), "a heavy weapon on a hull with no heavy mount cannot be equipped")
	# the parts list: upgrades only, then everything that fits
	b.hold = []
	b._select("hp", first)
	var ups: Array = b._catalogue_rows()
	var all_same := ups.all(func(e): return b._is_upgrade(twin, e["id"]))
	_check(all_same, "picking a fitted weapon lists only its upgrades (%d rows)" % ups.size())
	b.cat_mode = "switch"
	var all: Array = b._catalogue_rows()
	_check(all.size() > ups.size(), "Switch out lists everything for that mount (%d rows)" % all.size())
	b._select("hp", first)
	_check(b.cat_mode == "upgrade", "picking a slot again starts on upgrades")
	# marks: only higher ones are upgrades
	var tiered := ""
	for m in b.modules:
		if gd.tier(m["id"]) >= 2:
			tiered = m["id"]
			break
	if not tiered.is_empty():
		b._select("bay", b.modules.map(func(m): return m["id"]).find(tiered))
		var rows: Array = b._catalogue_rows()
		_check(rows.all(func(e): return gd.tier(e["id"]) > gd.tier(tiered)), "%s lists only higher marks (%s)" % [gd.MODULES[tiered]["name"], rows.map(func(e): return e["name"])])
	await _hold_and_controls(b, gd)
	print("shipyard test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


## Removing parts, the full-hold question, and the keyboard/gamepad commands.
func _hold_and_controls(b, gd) -> void:
	b.cat_mode = "upgrade"
	b.hold = []
	b.hold_stock = {}
	b.taken = []
	# a stock part taken off lands in the hold, marked, and sells for scrap
	var bay_id: String = b.modules[0]["id"]
	_check(b.modules[0]["stock"], "the first bay part is stock")
	b._select("bay", 0)
	b._remove_selected()
	_check(b.hold == [bay_id] and int(b.hold_stock.get(bay_id, 0)) == 1, "a stock part taken off goes to the hold (%s %s)" % [b.hold, b.hold_stock])
	_check(b._hold_is_stock(0) and b._sale_value(bay_id, true) == int(gd.price(bay_id) * b.SCRAP), "it sells for scrap, %d cr" % b._sale_value(bay_id, true))
	# fitted back, it is stock again; taken off again, back in the hold
	b._select("bay", b.modules.size())
	b._mount(bay_id)
	_check(b.hold.is_empty() and b.hold_stock.is_empty() and b.modules[-1]["stock"], "fitting it back keeps it a stock part")
	# a weapon swapped out goes to the hold too
	var w0: Dictionary = b.mounts[0]
	b.credits = 100000
	var other := ""
	for id in gd.MODULES:
		if not gd.is_bay_part(id) and id != w0["id"] and not id in gd.STATION_ONLY and gd.fits_hull(id, b.hull) and gd.weapon_class(id) <= b.hardpoints[0][5] and gd.part_race(id) < 0:
			other = id
			break
	b._select("hp", 0)
	b._mount(other)
	_check(b.mounts[0]["id"] == other and b.hold.has(w0["id"]), "a replaced weapon goes to the hold (%s)" % [b.hold])
	# full hold: taking a part off asks, and Cancel leaves everything
	while b.hold.size() < 16:
		b.hold.append("9202")
	var credits_was: int = b.credits
	var mods_was: Array = b.modules.duplicate(true)
	b._select("bay", 0)
	b._remove_selected()
	_check(not b.confirm.is_empty() and b.modules == mods_was and b.credits == credits_was, "with the hold full, taking a part off asks first and changes nothing yet")
	b._answer(1)
	_check(b.confirm.is_empty() and b.modules == mods_was and b.credits == credits_was and b.hold.size() == 16, "Cancel leaves the ship and the hold as they were")
	# ... and Sell sells it on the spot
	var gone_id: String = b.modules[0]["id"]
	var worth: int = b._sale_value(gone_id, b.modules[0]["stock"])
	b._remove_selected()
	b._answer(0)
	_check(b.modules.size() == mods_was.size() - 1 and b.credits == credits_was + worth and b.hold.size() == 16, "Sell takes it off and pays %d cr" % worth)
	# mounting over a part with the hold full asks too
	b.hold = []
	b.hold_stock = {}
	while b.hold.size() < 16:
		b.hold.append("9202")
	b._select("hp", 0)
	var now_id: String = b.mounts[0]["id"]
	b._mount(w0["id"] if w0["id"] != now_id else other)
	_check(not b.confirm.is_empty() and b.mounts[0]["id"] == now_id, "swapping a weapon with the hold full asks first")
	b._answer(1)
	# a traded-in hull takes its stock parts back
	b.hold = []
	b.hold_stock = {}
	b.taken = []
	var hull_was: String = b.hull
	b._select("bay", 0)
	var stock_id: String = b.modules[0]["id"]
	var was_stock: bool = b.modules[0]["stock"]
	b._remove_selected()
	var target := "Destroyer" if hull_was != "Destroyer" else "Scout"
	b._switch_hull(target)
	_check(b.hull == target, "hull switched to %s" % target)
	if was_stock:
		_check(not b.hold.has(stock_id), "stock parts taken off a traded-in hull go back with it (%s)" % [b.hold])
	b._switch_hull(hull_was)
	# keyboard / gamepad: sections, picking a slot, fitting, backing out
	b.hold = ["9202"]
	b.hold_stock = {}
	b.kb_nav = false
	b.focus_key = "hp:0"
	b.sel_kind = ""
	b.queue_redraw()
	await process_frame
	await process_frame
	b._command("down")
	_check(b.kb_nav and b.focus_key == "hp:0", "the first key press only shows the focus")
	b._command("next")
	_check(b._section_of(b.focus_key) == "hold" or b._section_of(b.focus_key) == "parts", "Tab / RB moves to the next section (%s)" % b.focus_key)
	b._command("prev")
	_check(b._section_of(b.focus_key) == "fit", "Shift+Tab / LB comes back (%s)" % b.focus_key)
	b._command("prev")
	_check(b.focus_key == "hull:" + b.hull, "... and on to the hull list (%s)" % b.focus_key)
	b._command("down")
	_check(b.focus_key.begins_with("hull:") and b.focus_key != "hull:" + b.hull, "down walks the hull list (%s)" % b.focus_key)
	b._command("next")
	b.focus_key = "bay:%d" % b.modules.size()       # the Add a part card
	await process_frame
	await process_frame
	b._command("accept")
	_check(b.sel_kind == "bay" and b.focus_key.begins_with("part:"), "picking a slot hands the focus to its parts (%s)" % b.focus_key)
	await process_frame
	await process_frame
	var n_mods: int = b.modules.size()
	var part: String = b.focus_key.trim_prefix("part:")
	b._command("accept")
	await process_frame
	_check(b.modules.size() >= n_mods and b.modules.map(func(m): return m["id"]).has(part) and b.focus_key.begins_with("bay:"), "A / Enter fits the part and goes back to the slot (%s)" % b.focus_key)
	b._command("remove")
	_check(b.hold.has(part), "Y / Del takes the focused part off to the hold")
	b.focus_key = "hold:%d" % b.hold.find("9202")
	var cr: int = b.credits
	b._command("remove")
	_check(not b.hold.has("9202") and b.credits > cr, "Y / Del on a hold row sells it")
	b._command("back")
	b._command("back")
	b._command("back")
	_check(not b.confirm.is_empty(), "backing out of a changed ship asks before leaving")
	b._command("back")
	_check(b.confirm.is_empty() and main.builder != null, "B / Esc on that question keeps building")
	# launch saves the hold's stock marks on the ship
	b.hold = [stock_id]
	b.hold_stock = {stock_id: 1}
	b.credits = 100000
	var p = main.world.player
	b._command("launch")
	if main.builder == null:
		_check(p.hold_stock.get(stock_id, 0) == 1, "the ship keeps which hold parts are stock")
	else:
		_check(false, "Launch did not launch: %s" % [b._problems()])
