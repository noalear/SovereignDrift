# Armor coats and shields test (the armor and shields rework).  Starts a
# practice duel and checks, on stand-in ships:
#  - each coat stops its share of each damage type (gd.ARMOR), a
#    weapon's pierce ignores part of it, Reflective is half as good once
#    scorched, an Ablative layer wears away, Reactive tiles fire only on
#    big hits and re-arm, Bulkheads spare the systems and crew;
#  - shields: a hit drains only the half it lands on and pauses its
#    recharge, focus moves the charge over 2 s, lowered shields hold
#    nothing and come back at 20% after the boot, Flicker leaks;
#  - the shipyard keeps one part of each kind (engines, shields and
#    boosters stack), computer pilots fly their empire's coat, and the
#    snapshot carries both shield halves.
#   godot --headless --path . -s tools/armor_test.gd
extends SceneTree

var main: Node
var failed := false
var gd: Node                             # -s scripts can't name autoloads


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	gd = root.get_node("GameData")
	root.add_child(main)
	await process_frame
	main._start_practice()
	for i in 30:
		await process_frame
	_run(main.world)
	print("armor_test: %s" % ("FAILED" if failed else "all checks passed"))
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	if not ok:
		failed = true
		print("FAILED: " + what)
	else:
		print("  ok: " + what)


func _near(a: float, b: float, tol: float = 0.5) -> bool:
	return absf(a - b) <= tol


## A stand-in Cruiser far from everything, with `extra` bay parts swapped in.
func _dummy(w, coat: String, shield_id: String = "") -> Ship:
	var s := Ship.new()
	w.ships_layer.add_child(s)
	s.setup(w, "Cruiser", 1, "Dummy", "Dummy")
	s.set_physics_process(false)
	var d: Dictionary = gd.stock_design("Cruiser")
	var mods := []
	for id in d["modules"]:
		var f: String = gd.MODULES[id]["func"]
		if f == "armor" or (f == "shield" and shield_id != ""):
			continue
		mods.append(id)
	if coat != "":
		mods.append(coat)
	if shield_id != "":
		mods.append(shield_id)
	d["modules"] = mods
	s.refit(d)
	s.global_position = Vector2(30000, 30000)
	return s


func _drop(w, s: Ship) -> void:
	w.space.remove_body(s.body)
	s.queue_free()


## Where a hit from straight ahead (or astern) lands on the hull.
func _point(s: Ship, front: bool) -> Vector2:
	return s.global_position + Vector2.RIGHT.rotated(s.rotation) * s.size * (1.0 if front else -1.0)


## Hull lost to one hit of `amount` with the shields down.
func _hull_hit(s: Ship, amount: float, dtype: String, pierce: float = 0.0) -> float:
	s.set_shields_raised(false)
	var was := s.hull
	s.take_damage(amount, null, _point(s, true), 1.0, "", 1.0, dtype, pierce)
	return was - s.hull


func _run(w) -> void:
	# ------------------------------------------------------------ armor
	print("armor coats:")
	for coat in gd.ARMOR:
		var kind: String = gd.ARMOR[coat]["coat"]
		if kind == "reactive" or kind == "ablative":
			continue
		for dtype in ["beam", "plasma", "kinetic", "explosive", "ion"]:
			var s := _dummy(w, coat)
			var lost := _hull_hit(s, 100.0, dtype)
			var want: float = 100.0 * gd.damage_mult(dtype, "hull") * (1.0 - gd.armor_resist(coat, dtype))
			_check(_near(lost, want), "%s vs %s: %.1f of 100 reaches the hull (want %.1f)" % [gd.coat_label(coat), dtype, lost, want])
			_drop(w, s)
	var bare := _dummy(w, "")
	_check(_near(_hull_hit(bare, 100.0, "kinetic"), 100.0 * gd.damage_mult("kinetic", "hull")), "no coat stops nothing")
	_drop(w, bare)
	# lasers go straight through plate, as in the original
	var plate := _dummy(w, "5503")
	_check(_near(_hull_hit(plate, 100.0, "beam"), 100.0 * gd.damage_mult("beam", "hull")), "plate stops no beam damage")
	# pierce: the cannon shell ignores half the resistance
	var pierced := _hull_hit(plate, 100.0, "kinetic", 0.5)
	_check(_near(pierced, 100.0 * gd.damage_mult("kinetic", "hull") * (1.0 - 0.35 * 0.5)), "pierce 0.5 halves Plate III's 35%% (%.1f lost)" % pierced)
	_drop(w, plate)
	# reflective: half as good below half hull
	var mirror := _dummy(w, "5504")
	mirror.hull = mirror.hull_max * 0.4
	var scorched := _hull_hit(mirror, 100.0, "beam")
	_check(_near(scorched, 100.0 * gd.damage_mult("beam", "hull") * 0.7), "scorched Reflective stops 30%% of a beam (%.1f lost)" % scorched)
	_drop(w, mirror)
	# ablative: resists while the layer lasts, the layer loses what it stops
	var abl := _dummy(w, "5505")
	_check(_near(abl.ablative_max, abl.hull_max * gd.ABLATIVE_LAYER, 1.0), "ablative layer is 35%% of the hull (%d)" % int(abl.ablative_max))
	var first := _hull_hit(abl, 100.0, "plasma")
	_check(_near(first, 100.0 * gd.damage_mult("plasma", "hull") * 0.4) and abl.ablative < abl.ablative_max, "ablative stops 60%% of plasma and wears (%.1f lost, layer %d)" % [first, int(abl.ablative)])
	var guard := 0
	while abl.ablative > 0.0 and guard < 500 and not abl.dead:
		abl.hull = abl.hull_max
		_hull_hit(abl, 200.0, "plasma")
		guard += 1
	abl.hull = abl.hull_max
	_check(_near(_hull_hit(abl, 100.0, "plasma"), 100.0 * gd.damage_mult("plasma", "hull")), "a burnt-off layer stops nothing")
	_check(abl._coat_wear() == 0.0, "and its tiles are gone from the hull")
	abl.restock_armor()
	_check(abl.ablative == abl.ablative_max, "a base refills the layer")
	_check(abl._coat_wear() == 1.0, "and they are back on")
	_drop(w, abl)
	# reactive: only big hits, one tile each, re-armed out of combat
	var rx := _dummy(w, "5506")
	var tiles := rx.reactive_tiles
	_check(tiles == gd.reactive_tiles(rx.size) and tiles >= 3, "a Cruiser carries %d reactive tiles" % tiles)
	var small := _hull_hit(rx, 100.0, "kinetic")
	_check(_near(small, 100.0 * gd.damage_mult("kinetic", "hull")) and rx.reactive_tiles == tiles, "a 100-point bolt passes reactive armor and fires no tile")
	rx.hull = rx.hull_max
	var big := _hull_hit(rx, 400.0, "explosive")
	_check(_near(big, 400.0 * gd.damage_mult("explosive", "hull") * 0.3, 1.0) and rx.reactive_tiles == tiles - 1, "a 400-point torpedo fires a tile and loses 70%% (%.1f lost)" % big)
	_check(_near(_hull_hit(rx, 300.0, "beam"), 300.0 * gd.damage_mult("beam", "hull"), 1.0), "beams never fire tiles")
	while rx.reactive_tiles > 0:
		rx.hull = rx.hull_max
		_hull_hit(rx, 400.0, "kinetic")
	rx.hull = rx.hull_max
	_check(_near(_hull_hit(rx, 400.0, "kinetic"), 400.0 * gd.damage_mult("kinetic", "hull"), 1.0), "with every tile spent the big hits go through")
	rx.last_attacked_at = -100.0
	rx._tick_armor(gd.REACTIVE_REARM + 0.1)
	_check(rx.reactive_tiles == 1, "a tile re-arms after %d s out of combat" % int(gd.REACTIVE_REARM))
	_drop(w, rx)
	# bulkheads: the crew and systems suffer less for the same hull damage
	var open_ship := _dummy(w, "")
	var bulk := _dummy(w, "5508")
	open_ship.set_shields_raised(false)
	bulk.set_shields_raised(false)
	var sys0 := _systems_hp(open_ship)
	var sysb := _systems_hp(bulk)
	var c0 := open_ship.crew
	var cb := bulk.crew
	var h0 := open_ship.hull
	var hb := bulk.hull
	# small hits, so no system is knocked to zero and stops counting
	open_ship.take_damage(60.0, null, _point(open_ship, false), 1.0, "", 1.0, "explosive")
	bulk.take_damage(60.0, null, _point(bulk, false), 1.0, "", 1.0, "explosive")
	# crew losses go by the share of the hull lost, so compare per share
	var crew_rate0 := (c0 - open_ship.crew) / open_ship.crew_max / maxf((h0 - open_ship.hull) / open_ship.hull_max, 0.0001)
	var crew_rateb := (cb - bulk.crew) / bulk.crew_max / maxf((hb - bulk.hull) / bulk.hull_max, 0.0001)
	var sys_rate0 := (sys0 - _systems_hp(open_ship)) / maxf(h0 - open_ship.hull, 0.01)
	var sys_rateb := (sysb - _systems_hp(bulk)) / maxf(hb - bulk.hull, 0.01)
	_check(_near(crew_rateb, crew_rate0 * gd.BULKHEAD_CREW, crew_rate0 * 0.05 + 0.0001), "bulkheads halve crew losses per point of hull damage")
	_check(_near(sys_rateb, sys_rate0 * gd.BULKHEAD_SYSTEMS, sys_rate0 * 0.05 + 0.01), "bulkheads take 40%% off system damage (%.2f vs %.2f)" % [sys_rateb, sys_rate0])
	_check(bulk.board_defence() > 1.0 and open_ship.board_defence() == 1.0, "bulkheads help defenders against boarders")
	_drop(w, open_ship)
	_drop(w, bulk)
	# thermal mass and radar signature in the stats
	var st3: Dictionary = gd.stats_for(_design_with("5503"))
	_check(_near(st3["thermal_lag"], 4.5, 0.01), "Plate III lags heat by 4.5 s")
	_check(_near(gd.stats_for(_design_with("5507"))["armor_sig"], 0.4, 0.001) and _near(gd.stats_for(_design_with("5504"))["armor_sig"], 1.3, 0.001), "Shroud 0.4x, Reflective 1.3x radar")
	var cool := World._thermal_mass(_dummy_stats(st3))
	_check(_near(cool, 1.0 / (1.0 + 7000.0 / 8000.0), 0.001), "Plate III takes %.0f%% of the Sun's heat" % (cool * 100.0))

	# ---------------------------------------------------------- shields
	print("shields:")
	var sh := _dummy(w, "", "5004")
	sh.set_shield_focus(0)
	sh.shield_share = 0.5
	sh.shield = sh.shield_max
	var half := sh.shield_max * 0.5
	_check(_near(sh.shield_front, half) and _near(sh.shield_rear, half), "a balanced shield is two equal halves")
	sh.take_damage(100.0, null, _point(sh, true), 1.0, "", 1.0, "kinetic")
	var want_front: float = half - 100.0 * gd.damage_mult("kinetic", "shield")
	_check(_near(sh.shield_front, want_front) and _near(sh.shield_rear, half), "a hit from ahead drains only the front half")
	_check(sh.shield_pause[0] > 0.0 and sh.shield_pause[1] == 0.0, "and pauses only its recharge")
	var before := sh.shield_front
	sh.energy = sh.energy_max
	sh._tick_shields(1.0)
	_check(_near(sh.shield_front, before) and _near(sh.shield_rear, half), "no recharge during the pause")
	sh._tick_shields(2.0)
	sh.energy = sh.energy_max
	before = sh.shield_front
	sh._tick_shields(1.0)
	var rate: float = gd.SHIELD_FAMILIES["deflector"]["rate"]
	_check(_near(sh.shield_front - before, sh.shield_max * rate * 0.5, 1.0), "then the half recharges 10%% of its size a second (+%.1f)" % (sh.shield_front - before))
	# empty a half: the long pause
	sh.take_damage(sh.shield_rear / gd.damage_mult("kinetic", "shield") + 20.0, null, _point(sh, false), 1.0, "", 1.0, "kinetic")
	_check(sh.shield_rear <= 0.01 and _near(sh.shield_pause[1], gd.SHIELD_FAMILIES["deflector"]["zero_pause"], 0.01), "a half knocked to zero waits 5 s")
	sh.hull = sh.hull_max
	sh.systems.repair_all()
	sh.apply_systems()
	# focus
	sh.shield = sh.shield_max
	sh.set_shield_focus(1)
	for i in 10:
		sh.energy = sh.energy_max
		sh._tick_shields(0.1)
	_check(sh.shield_share > 0.5 and sh.shield_share < 0.75, "focus is still moving after 1 s (share %.2f)" % sh.shield_share)
	for i in 12:
		sh.energy = sh.energy_max
		sh._tick_shields(0.1)
	_check(_near(sh.shield_share, 0.75, 0.001), "Forward puts 75%% of the shield in front after 2 s")
	_check(sh.shield_front <= sh.shield_cap(true) + 0.01 and sh.shield_rear <= sh.shield_cap(false) + 0.01, "both halves stay inside their sizes")
	# lowering and raising
	sh.set_shields_raised(false)
	_check(sh.shield == 0.0 and not sh.shields_raised, "lowered shields hold nothing")
	var hull_before := sh.hull
	sh.take_damage(100.0, null, _point(sh, true), 1.0, "", 1.0, "kinetic")
	_check(sh.hull < hull_before, "and hits go to the hull")
	sh.set_shields_raised(true)
	sh._tick_shields(1.0)
	_check(sh.shield == 0.0 and sh.shield_boot > 0.0, "raising takes a moment")
	sh._tick_shields(1.05)
	_check(_near(sh.shield, sh.shield_max * gd.SHIELD_BOOT_CHARGE, 1.0), "they come up at 20%% (%d of %d)" % [int(sh.shield), int(sh.shield_max)])
	_drop(w, sh)
	# families
	var bar := _dummy(w, "", "5012")
	_check(bar.shield_family == "barrier" and _near(bar.shield_max, 800.0, 0.5), "Barrier II holds 800")
	_drop(w, bar)
	var fl := _dummy(w, "", "5022")
	_check(fl.shield_family == "flicker", "Flicker shields are Flicker")
	var leaks := 0
	var hits := 400
	seed(7)
	for i in hits:
		fl.systems.repair_all()          # the bleed through wears the generator down
		fl.apply_systems()
		fl.set_shield_focus(0)
		fl.shield_share = 0.5
		fl.shield = fl.shield_max
		fl.hull = fl.hull_max
		var hb2 := fl.hull
		fl.take_damage(50.0, null, _point(fl, true), 1.0, "", 1.0, "kinetic")
		if fl.hull < hb2 - 0.1:
			leaks += 1
			if leaks == 1:
				_check(_near(hb2 - fl.hull, 50.0 * 0.3 * gd.damage_mult("kinetic", "hull"), 0.6), "a leak lets 30% through")
	_check(leaks > hits * 0.4 and leaks < hits * 0.6, "Flicker leaks on about half the hits (%d of %d)" % [leaks, hits])
	_drop(w, fl)

	# ---------------------------------------------------------- looks
	print("looks:")
	var tex := ShipArt.hull_texture("Cruiser", 1)
	var img := tex.get_image()
	if img.is_compressed():
		img.decompress()
	img.convert(Image.FORMAT_RGBA8)
	var mask := ShipArt.hull_mask(tex)
	var baked := {}
	for coat in ["5501", "5503", "5504", "5505", "5506", "5507", "5508"]:
		var out: Image = ShipArt.bake_coat(img, gd.ARMOR[coat]["coat"], gd.tier(coat), 1, mask)
		baked[coat] = out
		_check(out.get_size() == img.get_size() and out.get_data() != img.get_data(), "%s repaints the hull" % gd.coat_label(coat))
	_check(baked["5501"].get_data() != baked["5503"].get_data(), "Plate I and Plate III look different")
	var other: Image = ShipArt.bake_coat(img, "reflective", 0, 0, mask)
	_check(other.get_data() != baked["5504"].get_data(), "each empire lays a coat out its own way")
	var same_alpha := true
	for p in [Vector2i(5, 5), Vector2i(img.get_width() / 2, img.get_height() / 2), Vector2i(img.get_width() - 3, 2)]:
		if absf(baked["5507"].get_pixelv(p).a - img.get_pixelv(p).a) > 0.002:
			same_alpha = false
	_check(same_alpha, "a coat keeps the painting's outline")
	_check(ShipArt.build_model(_design_with("5506"), 1, 40.0)["coat"] == "5506", "a ship's model knows its coat")

	# ---------------------------------------------------------- shipyard
	print("shipyard and fits:")
	var b := Builder.new()
	b.modules = [{"id": "1001", "stock": true}, {"id": "5501", "stock": true}, {"id": "2002", "stock": true}, {"id": "5003", "stock": true}, {"id": "7001", "stock": true}]
	b.modules.append({"id": "5504", "stock": false})
	var gone: Array = b._one_of_a_kind(b.modules.size() - 1)
	_check(_ids(b.modules).count("5501") == 0 and _ids(b.modules).has("5504") and gone.size() == 1, "a second coat replaces the first")
	b.modules.append({"id": "2002", "stock": false})
	gone = b._one_of_a_kind(b.modules.size() - 1)
	_check(_ids(b.modules).count("2002") == 2 and gone.is_empty(), "engines stack")
	b.modules.append({"id": "5003", "stock": false})
	gone = b._one_of_a_kind(b.modules.size() - 1)
	_check(_ids(b.modules).count("5003") == 2 and gone.is_empty(), "shields of one family stack")
	b.modules.append({"id": "5021", "stock": false})
	gone = b._one_of_a_kind(b.modules.size() - 1)
	_check(_ids(b.modules).count("5003") == 0 and _ids(b.modules).has("5021"), "a Flicker shield replaces Deflectors")
	b.modules.append({"id": "1002", "stock": false})
	b._one_of_a_kind(b.modules.size() - 1)
	b.modules.append({"id": "7002", "stock": false})
	b._one_of_a_kind(b.modules.size() - 1)
	_check(_ids(b.modules).count("1001") == 0 and _ids(b.modules).count("7001") == 0, "one power plant and one sensor suite")
	b.free()
	_check(_ids_of(gd.race_fit("Scout", 0)).has("5504"), "Arcean pilots fly Reflective")
	_check(_ids_of(gd.race_fit("Destroyer", 2)).has("5506"), "Drengin pilots fly Reactive")
	_check(_ids_of(gd.race_fit("Raider", 3)).has("5507"), "pirates fly Shroud")
	_check(_ids_of(gd.race_fit("Cruiser", 1)).has("5501"), "Terrans fly Plate")
	var fr := _ids_of(gd.race_fit("Freighter", 0))
	_check(fr.has("5502") and not fr.has("5504"), "haulers keep their own fit")
	for hull in gd.SHIPS:
		var st: Dictionary = gd.stats_for(gd.race_fit(hull, 2))
		if st["bay_used"] > st["bay_cap"]:
			_check(false, "%s with its coat is over its bays" % hull)
	_check(gd.part_race("5506") == 2 and gd.part_race("5504") == 0 and gd.part_race("5503") == -1 and gd.part_race("5507") == -1 and gd.part_race("5508") == -1,
		"every yard sells every coat but Reactive (Drengin) and Reflective (Arcean)")

	# ---------------------------------------------------------- network
	var net_ship := _dummy(w, "", "5004")
	net_ship.set_shield_focus(1)
	net_ship.shield_share = 0.75
	net_ship.shield = net_ship.shield_max
	net_ship.shield_front *= 0.5
	var snap := NetCodec.unpack_snapshot(NetCodec.pack_snapshot(1.0, 0, [net_ship]))
	var e: Dictionary = snap["ships"][0]
	var copy := _dummy(w, "", "5004")
	copy.set_shield_focus(1)
	copy.shield_share = 0.75
	copy.set_shield_split(e["shield"] * copy.shield_max, e["front"])
	_check(_near(copy.shield_front, net_ship.shield_front, 3.0) and _near(copy.shield_rear, net_ship.shield_rear, 3.0), "the snapshot carries both halves (%d/%d)" % [int(copy.shield_front), int(copy.shield_rear)])
	net_ship.set_shields_raised(false)
	e = NetCodec.unpack_snapshot(NetCodec.pack_snapshot(1.0, 0, [net_ship]))["ships"][0]
	_check((e["flags"] & NetCodec.FLAG_SHIELDS_DOWN) != 0, "and whether they are lowered")
	_drop(w, net_ship)
	_drop(w, copy)


func _systems_hp(s: Ship) -> float:
	var t := 0.0
	for e in s.systems.list:
		t += e["hp"]
	return t


func _design_with(coat: String) -> Dictionary:
	var d: Dictionary = gd.stock_design("Cruiser")
	d["modules"].append(coat)
	return d


func _dummy_stats(st: Dictionary) -> Ship:
	var s := Ship.new()
	s.stats = st
	return s


func _ids(mods: Array) -> Array:
	var out := []
	for m in mods:
		out.append(m["id"])
	return out


func _ids_of(d: Dictionary) -> Array:
	return d["modules"]
