# Names and colours test (Kyle 2026-10-31): every empire's guns go by the
# shipyard's part names (no "Barb gun"; the rapid tube is the Gatling gun),
# hulls by their labels, empires by their colours (Skethra red, Solborn
# blue, Ephyri green, pirates yellow), and a Skethra cruiser can buy more
# Gatling guns for its turret and nose (they were light mounts, so the
# shipyard said "too heavy").
#   godot --headless --path . -s tools/names_test.gd
extends SceneTree

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	var gd = root.get_node("/root/GameData")
	# --- names
	_check(gd.MODULES["4010"]["name"] == "Gatling gun", "the rapid tube is called the Gatling gun")
	var old := []
	for id in gd.MODULES:
		var n: String = gd.MODULES[id]["name"]
		if "Barb" in n or "Rapid tube" in n:
			old.append(n)
	_check(old.is_empty(), "no part keeps an old name (%s)" % [old])
	for r in 4:
		for m in ["001", "010", "003", "011"]:
			var n: String = gd.missile_display_name(r, m)
			_check(n.to_lower() == str(gd.MISSILES[m]["name"]).to_lower(), "empire %d calls munition %s by its own name (%s)" % [r, m, n])
		_check(not gd.RACES[r].has("weapon1"), "no per-empire gun names left")
	_check(gd.hull_label("BattleCruiser") == "Battlecruiser" and gd.hull_label("DropShip") == "Drop ship" and gd.hull_label("DefenseStation") == "Defense station",
		"hulls and stations show their labels (%s, %s, %s)" % [gd.hull_label("BattleCruiser"), gd.hull_label("DropShip"), gd.hull_label("DefenseStation")])
	# --- colours: hue of each empire
	var want := {0: "green", 1: "blue", 2: "red", 3: "yellow"}
	for r in want:
		var c: Color = gd.race(r)["color"]
		_check(_hue_name(c) == want[r], "%s are %s (%s, hue %.2f)" % [gd.race(r)["plural"], want[r], _hue_name(c), c.h])
	# --- mounts: cruisers and up take a Gatling gun on every gun mount
	for hull in ["Cruiser", "BattleCruiser", "Battleship", "Dreadnought"]:
		var hp: Array = gd.hull_hardpoints(hull)
		for i in hp.size():
			if int(hp[i][0]) == 180:
				continue      # the rear mine rack
			_check(gd.weapon_class("4010") <= int(hp[i][5]), "a Gatling gun fits %s mount %d" % [hull, i + 1])
	# stock fits are unchanged by the bigger mounts
	var sk: Dictionary = gd.stock_design("Cruiser", 2)
	var ids := []
	for w in sk["weapons"]:
		ids.append(w[0])
	_check(ids == ["4020", "4003", "4010", "4010", "4002"], "the Skethra cruiser's stock fit (%s)" % [ids])
	# --- in game: a Skethra pilot's weapons panel and shipyard (the
	# player's settings file is put back afterwards, so other tests still
	# fly for the Solborn)
	var saved := FileAccess.get_file_as_bytes("user://settings.cfg")
	root.get_node("/root/Settings").race_id = 2
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
	_check(p.race_id == 2, "flying a %s ship" % gd.race(p.race_id)["name"])
	var labels := []
	for g in range(1, 5):
		labels.append_array(p.group_weapons(g))
	var names := []
	for wpn in p.weapons:
		names.append(wpn["name"])
	for l in labels:
		_check(str(l).split(" x")[0] in names, "weapons panel shows %s, a part name" % l)
	for e in p.systems.list:
		if e.get("kind", "") == "weapon":
			_check(e["label"] in names, "systems show %s, a part name" % e["label"])
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
	b._load_hull("Cruiser", {}, false)
	b.credits = 100000
	for i in [0, 1]:
		b.sel_kind = "hp"
		b.sel_index = i
		b.cat_mode = "switch"
		var row := {}
		var heavy := {}
		for e in b._catalogue_rows():
			if e["id"] == "4010":
				row = e
			if e["id"] == "4018":
				heavy = e
		_check(not row.is_empty() and row["ok"], "the Skethra cruiser's mount %d takes a Gatling gun (%s)" % [i + 1, row.get("price", "no row")])
		_check(heavy.is_empty() or (not heavy["ok"] and heavy["price"] == "heavy mount"), "a cannon on mount %d says it needs a heavy mount (%s)" % [i + 1, heavy.get("price", "")])
	root.get_node("/root/Settings").race_id = 1
	if not saved.is_empty():
		var fa := FileAccess.open("user://settings.cfg", FileAccess.WRITE)
		fa.store_buffer(saved)
		fa.close()
	print("names test: %s" % ("FAILED" if failed else "ok"))
	quit(1 if failed else 0)


static func _hue_name(c: Color) -> String:
	var h := c.h * 360.0
	if h < 25.0 or h >= 330.0:
		return "red"
	if h < 70.0:
		return "yellow"
	if h < 170.0:
		return "green"
	if h < 260.0:
		return "blue"
	return "other"


func _check(ok: bool, what: String) -> void:
	if not ok:
		failed = true
	print(("  ok    " if ok else "  FAIL  ") + what)
