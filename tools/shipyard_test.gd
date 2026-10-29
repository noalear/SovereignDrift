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
	if not b.placing.is_empty():
		_check(b.focus_key == "hp:%d" % first, "Equip with several mounts that take it asks which, starting at the likeliest (%s)" % b.focus_key)
		b._command("accept")
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
	# marks: higher and lower ones both show, to save mass as well as to upgrade
	var tiered := ""
	for m in b.modules:
		if gd.tier(m["id"]) >= 2:
			tiered = m["id"]
			break
	if not tiered.is_empty():
		b._select("bay", b.modules.map(func(m): return m["id"]).find(tiered))
		var rows: Array = b._catalogue_rows()
		_check(rows.all(func(e): return b._same_family(e["id"], tiered) and e["id"] != tiered) and rows.any(func(e): return gd.tier(e["id"]) < gd.tier(tiered)),
			"%s lists its other marks, lower ones too (%s)" % [gd.MODULES[tiered]["name"], rows.map(func(e): return e["name"])])
	# boosters and quantum drives show what a better one adds
	var with_boost := func(ids: Array) -> Dictionary:
		var d: Dictionary = gd.stock_design("Destroyer", 1)
		d["modules"] = d["modules"].filter(func(id): return gd.MODULES[id]["func"] != "boost") + ids
		return gd.stats_for(d)
	var st1: Dictionary = with_boost.call(["9201"])
	var st3: Dictionary = with_boost.call(["9203"])
	var st2x: Dictionary = with_boost.call(["9201", "9201"])
	_check(is_equal_approx(st1["boost_speed"], 1.5) and is_equal_approx(st1["boost_accel"], 1.0 + 1000.0 / 833.0), "Booster I boosts x1.5 speed as before (%.2f, %.2f)" % [st1["boost_speed"], st1["boost_accel"]])
	_check(st3["boost_speed"] > st1["boost_speed"] and st3["boost_accel"] > st1["boost_accel"], "Booster III boosts more than Booster I")
	_check(st2x["boost_speed"] > st1["boost_speed"] and st2x["boost_speed"] < 2.0 * st1["boost_speed"], "a second booster adds, at half")
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
	b.mounts[0]["new"] = false              # a part the pilot owns, not one bought just now
	var now_id: String = b.mounts[0]["id"]
	b._mount(w0["id"] if w0["id"] != now_id else other)
	_check(not b.confirm.is_empty() and b.mounts[0]["id"] == now_id, "swapping a weapon with the hold full asks first")
	b._answer(1)
	# a draft order: buying adds to the bill, taking a bought part off
	# removes it again, money back; the ship's own credits never move
	b.hold = []
	b.hold_stock = {}
	b.order = []
	b.ship.credits = 100000
	b.credits = b.ship.credits
	var bank: int = b.ship.credits
	var bay_new: int = b.modules.size()
	b._select("bay", bay_new)
	var buy := ""
	for id in gd.MODULES:
		if gd.is_bay_part(id) and not id in gd.STATION_ONLY and gd.price(id) <= b.credits and gd.part_race(id) < 0 and gd.stacks(id) and b._bays_fit(id, -1):
			buy = id
			break
	if not buy.is_empty():
		b._mount(buy)
		_check(b.order.size() == 1 and b._order_total() == -gd.price(buy) and b.credits == bank - gd.price(buy) and b.ship.credits == bank,
			"buying %s puts %d cr on the order; the pilot's credits are untouched" % [gd.MODULES[buy]["name"], gd.price(buy)])
		_check(b.modules[b.sel_index].get("new", false), "a bought part is marked new")
		b._remove_selected()
		_check(b.order.is_empty() and b.credits == bank and not b.hold.has(buy), "taking it off again drops it from the order, not into the hold")
		b._select("bay", b.modules.size())
		b._mount(buy)
	var hull_now: String = b.hull
	var other_hull := "Destroyer" if hull_now != "Destroyer" else "Scout"
	b._switch_hull(other_hull)
	_check(not b.order.any(func(e): return e["kind"] == "buy"), "a hull switch takes the parts being bought off the order")
	_check(b.order.filter(func(e): return e["kind"] == "hull").size() <= 1, "one hull on the order at most")
	b._switch_hull(hull_now)
	_check(b.order.is_empty() and b.credits == bank, "switching back leaves an empty order (%s)" % [b.order])
	b._leave()
	_check(not b.confirm.is_empty() or not b._dirty(), "Cancel with changes asks first")
	if not b.confirm.is_empty():
		b._answer(b.confirm["back"])
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
	_check(b.sel_kind == "bay" and b.focus_key.begins_with("type:"), "picking an empty slot hands the focus to the part types (%s)" % b.focus_key)
	await process_frame
	await process_frame
	var view: Array = b._catalogue_view()
	_check(not view.is_empty() and view.all(func(e): return e["kind"] == "type"), "Add a part lists part types first (%d types)" % view.size())
	var first_type: String = b.focus_key.trim_prefix("type:")
	b._command("accept")
	await process_frame
	_check(b.cat_type == first_type and b.focus_key.begins_with("part:"), "A / Enter opens a type and focuses its first part (%s)" % b.focus_key)
	view = b._catalogue_view()
	_check(view.all(func(e): return e["kind"] == "group" or b._part_type(e["id"]) == first_type), "an open type lists only its own parts")
	b._command("back")
	_check(b.cat_type == "" and b.focus_key == "type:" + first_type, "B / Esc goes back to the type buttons (%s)" % b.focus_key)
	# a type with families of several marks folds them under a header
	var grouped := ""
	for t in b.TYPE_ORDER:
		b.cat_type = t
		if b._catalogue_view().any(func(e): return e["kind"] == "group"):
			grouped = t
			break
	b.cat_type = ""
	if grouped != "":
		b.focus_key = "type:" + grouped
		await process_frame
		await process_frame
		b._command("accept")
		await process_frame
		var grp: Dictionary = b._catalogue_view().filter(func(e): return e["kind"] == "group")[0]
		var n_open: int = b._catalogue_view().size()
		b.focus_key = grp["key"]
		await process_frame
		await process_frame
		b._command("accept")
		await process_frame
		_check(b._catalogue_view().size() < n_open and b.focus_key == grp["key"], "A / Enter on a group header folds it (%s)" % grp["name"])
		b._command("accept")
		await process_frame
		_check(b._catalogue_view().size() == n_open, "... and unfolds it again")
		b._command("back")
	else:
		_check(false, "no part type has a family of several marks")
	# fit something that adds a bay rather than swapping one (power plants swap)
	var add_type := first_type
	for t in ["Repair", "Defence", "Drones", "Missiles", "Mines"]:
		if b._catalogue_view().any(func(e): return e["key"] == "type:" + t and e["ok"] > 0):
			add_type = t
			break
	b.focus_key = "type:" + add_type
	await process_frame
	await process_frame
	b._command("accept")
	await process_frame
	await process_frame
	await process_frame
	var n_mods: int = b.modules.size()
	var part: String = b.focus_key.trim_prefix("part:")
	b._command("accept")
	await process_frame
	_check(b.modules.size() >= n_mods and b.modules.map(func(m): return m["id"]).has(part) and b.focus_key.begins_with("bay:"), "A / Enter fits the part and goes back to the slot (%s)" % b.focus_key)
	var cr_before: int = b.credits
	b._command("remove")
	_check(not b.hold.has(part) and b.credits == cr_before + gd.price(part) and not b.order.any(func(e): return e["id"] == part),
		"Y / Del takes a part bought in this order back off it, money back")
	b.focus_key = "hold:%d" % b.hold.find("9202")
	var cr: int = b.credits
	b._command("remove")
	_check(not b.hold.has("9202") and b.credits > cr, "Y / Del on a hold row sells it")
	for n in 3:
		if b.confirm.is_empty():
			b._command("back")
	_check(not b.confirm.is_empty(), "backing out of a changed ship asks before leaving")
	b._command("back")
	_check(b.confirm.is_empty() and main.builder != null, "B / Esc on that question keeps building")
	var moved: Dictionary = await _moving_and_turning(b, gd)
	# launch saves the hold's stock marks on the ship
	b.hold = [stock_id]
	b.hold_stock = {stock_id: 1}
	b.credits = 100000
	var p = main.world.player
	b._command("launch")
	if not b.order.is_empty():
		_check(not b.confirm.is_empty() and b.confirm.has("lines"), "Accept shows the itemised bill first (%d lines)" % b.confirm.get("lines", []).size())
		b._command("accept")
	if main.builder == null:
		_check(p.hold_stock.get(stock_id, 0) == 1, "the ship keeps which hold parts are stock")
		if moved.has("fire"):
			var turned: Array = p.weapons.filter(func(w): return absf(w["fire_angle"] - deg_to_rad(moved["fire"])) < 0.01)
			_check(not turned.is_empty(), "the launched ship's turret faces where it was turned (%d°)" % moved["fire"])
		if moved.has("places"):
			_check(p.design.get("places", {}) == moved["places"], "the launched ship keeps where its systems sit (%s)" % [p.design.get("places", {})])
	else:
		_check(false, "Launch did not launch: %s" % [b._problems()])


## Moving weapons between mounts, turning turrets, moving a socketed
## system on the hull, and choosing where a hold part goes.  All free.
func _moving_and_turning(b, gd) -> Dictionary:
	var out := {}
	b.placing = {}
	var bill: int = b.order.size()
	var a := -1
	var c := -1
	for i in b.hardpoints.size():
		for j in b.hardpoints.size():
			if a < 0 and b.mounts[i] != null and b._can_swap(i, j):
				a = i
				c = j
	if a < 0:
		_check(false, "no two mounts on the %s can swap weapons" % b.hull)
		return out
	var wa: String = b.mounts[a]["id"]
	var wc: String = b.mounts[c]["id"] if b.mounts[c] != null else ""
	b.focus_key = "hp:%d" % a
	b._command("place")
	_check(b.placing.get("kind", "") == "move" and b.placing["targets"].has("hp:%d" % c), "M / LT on a weapon offers the mounts it can go to (%s)" % [b.placing.get("targets", [])])
	b.focus_key = "hp:%d" % c
	b._command("accept")
	_check(b.placing.is_empty() and b.mounts[c]["id"] == wa and ((wc == "" and b.mounts[a] == null) or (b.mounts[a] != null and b.mounts[a]["id"] == wc)),
		"Enter swaps the weapons between mounts %d and %d" % [a + 1, c + 1])
	b.focus_key = "hp:%d" % c
	b._command("place")
	b._command("back")
	_check(b.placing.is_empty() and b.mounts[c]["id"] == wa and main.builder != null, "B / Esc stops a move and nothing changes")
	# turning a turret turns its facing and its arc
	var hp0: Array = b.hardpoints[c]
	for n in 3:
		b._command("turn_l")
	var hp: Array = b._hp_turned(c)
	_check(b.turns[c] == -45 and int(hp[2]) == int(hp0[2]) - 45 and int(hp[3]) == int(hp0[3]) - 45 and int(hp[4]) == int(hp0[4]) - 45,
		"R / right stick turns mount %d and its arc 45° (faces %s)" % [c + 1, b._facing_text(c)])
	var w: Array = b.current_design()["weapons"].filter(func(x): return x[0] == wa and float(x[1]) == float(hp0[0]) and float(x[2]) == float(hp0[1]))[0]
	_check(int(w[3]) == int(hp0[2]) - 45, "the design carries the turned facing")
	out["fire"] = int(w[3])
	# picking another mount turns that one, not the one turned before
	var d := -1
	for i in b.hardpoints.size():
		if d < 0 and i != c and b._can_turn(i):
			d = i
	if d < 0:
		_check(false, "only one turnable mount on the %s" % b.hull)
	else:
		var t_c: int = b.turns[c]
		var t_d: int = b.turns[d]
		b.focus_key = "hp:%d" % c
		b._select("hp", d)
		b._command("turn_r")
		_check(b.turns[c] == t_c and b.turns[d] == wrapi(t_d + b.TURN_STEP, -180, 180), "after picking mount %d, R turns it and not mount %d" % [d + 1, c + 1])
		# dragging on the slip points it at the pointer, the wheel steps it
		var keep_badges: Array = b._badges
		var keep_pic: Rect2 = b._pic_rect
		b._badges = []
		for i in b.hardpoints.size():
			b._badges.append(Vector2(400 + 60 * i, 400))
		b._pic_rect = Rect2(0, 0, 1600, 900)
		b._aim_mount(d, b._badges[d] + Vector2(0, 80))
		_check(int(b._hp_turned(d)[2]) == 90 or int(b._hp_turned(d)[2]) == -270, "dragging mount %d points it down the screen (faces %s)" % [d + 1, b._facing_text(d)])
		var before: int = b.turns[d]
		var wheel := InputEventMouseButton.new()
		wheel.button_index = MOUSE_BUTTON_WHEEL_UP
		wheel.pressed = true
		wheel.position = b._badges[d] + Vector2(3, 3)
		b._gui_input(wheel)
		_check(b.turns[d] == wrapi(before - b.TURN_STEP, -180, 180), "the mouse wheel over a mount turns it a step")
		b._badges = keep_badges
		b._pic_rect = keep_pic
		# a mine layer always drops astern: no turning, and no Turn key shown
		var keep: Variant = b.mounts[d]
		b.mounts[d] = {"id": "4002"}
		var t_mine: int = b.turns[d]
		b.focus_key = "hp:%d" % d
		b._command("turn_r")
		var shows_turn: bool = b._hints().any(func(h): return h[1] == "Turn")
		_check(b.turns[d] == t_mine and not shows_turn and b._turn_block(d) != "", "a mine layer does not turn and shows no Turn key (%s)" % b._turn_block(d))
		b.mounts[d] = keep
		b.focus_key = "hp:%d" % d
		_check(b._hints().any(func(h): return h[1] == "Turn"), "a turret that can turn shows the Turn key")
	# a turned arc that crosses astern still holds a target behind
	var arc := {"arc0": deg_to_rad(150.0), "arc1": deg_to_rad(210.0)}
	_check(Ship.in_arc(arc, deg_to_rad(-170.0)) and not Ship.in_arc(arc, 0.0) and absf(Ship.clamp_to_arc(arc, deg_to_rad(-90.0)) - deg_to_rad(210.0)) < 0.01,
		"arcs turned past astern still work (in_arc, clamp_to_arc)")
	# a booster, shield or PDC moves to another socket on the hull
	var bi := -1
	for i in b.modules.size():
		if b._socket_kind(b.modules[i]["id"]) != "":
			bi = i
			break
	if bi < 0:
		_check(false, "no booster, shield or PDC on the %s to move" % b.hull)
	else:
		var f: String = gd.MODULES[b.modules[bi]["id"]]["func"]
		b.focus_key = "bay:%d" % bi
		b._command("place")
		_check(b.placing.get("kind", "") == "socket" and not b.placing["targets"].is_empty(), "M / LT on a %s offers the other sockets (%d)" % [f, b.placing.get("targets", []).size()])
		var k := int(b.placing["targets"][-1].get_slice(":", 1))
		b.focus_key = b.placing["targets"][-1]
		b._command("accept")
		_check(b._socket_of_bays().get(bi, -1) == k and b.current_design().has("places"), "Enter puts it in socket %d" % k)
		var model: Dictionary = ShipArt.build_model(b.current_design(), b.base.race_id, b.current_design()["size"])
		var spot: Vector2 = ShipArt.system_sockets(b.hull, b.base.race_id, b.current_design()["size"], ShipArt.SOCKET_KIND[f])[k]
		var drawn: bool = model["overlays"].any(func(o): return o["pos"].distance_to(spot) < 0.01) or model["pdc"].any(func(o): return o["pos"].distance_to(spot) < 0.01)
		_check(drawn, "the ship is drawn with it in that socket")
		out["places"] = b.current_design()["places"]
	# one shield module draws one emitter
	var shield_d: Dictionary = gd.stock_design("Destroyer", b.base.race_id)
	shield_d["modules"] = shield_d["modules"].filter(func(id): return gd.MODULES[id]["func"] != "shield") + ["5001"]
	var sm: Dictionary = ShipArt.build_model(shield_d, b.base.race_id, shield_d["size"])
	_check(sm["overlays"].filter(func(o): return o["icon"] == "shield").size() == 1, "one shield module draws one emitter dome")
	# a hold part with more than one place to go asks where
	var stack := ""
	for i in b.modules.size():
		if gd.stacks(b.modules[i]["id"]) and gd.MODULES[b.modules[i]["id"]]["func"] == "boost":
			stack = "9202"
	if stack != "":
		b.hold.append(stack)
		var n_mods: int = b.modules.size()
		b._equip_hold(b.hold.size() - 1)
		_check(b.placing.get("kind", "") == "equip" and b.placing["targets"].has("bay:%d" % n_mods) and b.placing["targets"].size() >= 2,
			"Equip of a second booster asks which bay, or a new one (%s)" % [b.placing.get("targets", [])])
		b._command("back")
		_check(b.placing.is_empty() and b.modules.size() == n_mods and b.hold.has(stack), "B / Esc leaves it in the hold")
		b.hold.erase(stack)
	_check(b.order.size() == bill, "moving, turning and placing cost nothing")
	await process_frame
	return out
