# Shipyard test: Equip from the hold (replaces a part of the same kind,
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
	_check(b.mounts[first]["id"] == twin and b.hold == ([] if was_stock else [have]), "the new part is fitted and the old one is in the hold, or scrapped if welded stock (%s)" % [b.hold])
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
	print("shipyard test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
