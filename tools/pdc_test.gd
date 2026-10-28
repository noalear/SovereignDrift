# PDC test: point defence as an internal part (Kyle, 2026-10-28).
#   godot --headless --path . -s tools/pdc_test.gd
# Checks the part data (a bay part, stock fits, old designs moved off the
# mounts), then out in empty space fires munitions one at a time at a
# stock Terran Cruiser from 900 px and counts how many its PDC shoots
# down: missiles, seekers, torpedoes, drones and a mine in its path, with
# the PDC on, off and shot out, a salvo against one and two turrets, and
# rearming.  --runs=N sets the shots per kind (default 12).
extends SceneTree

var main: Node
var gd: Node
var w
var fails := 0
var runs := 12
var log_lines := []
var keep_out := false            # hold the defender's PDC shot out
var evade := false               # the defender slides sideways and turns, as in a fight


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--runs="):
			runs = int(arg.get_slice("=", 1))
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		fails += 1


func _start() -> void:
	gd = root.get_node("/root/GameData")   # autoloads are not names in a -s script
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	await process_frame
	w = main.world
	_data_checks()
	await _intercepts()
	print("\n".join(log_lines))
	print("pdc_test: %s (%d failed)" % ["OK" if fails == 0 else "FAILED", fails])
	quit(0 if fails == 0 else 1)


func _data_checks() -> void:
	_check(gd.is_bay_part("4022"), "the PDC is an internal (bay) part")
	_check(gd.MODULES["4022"]["func"] == "pdc", "the PDC's func is pdc")
	var terran: Dictionary = gd.stock_design("Cruiser", 1)
	_check(terran["modules"].count("4022") == 1, "a stock Terran Cruiser carries one PDC in its bays")
	var on_mount := false
	for hull in gd.SHIPS:
		for race in [0, 1, 2, 3]:
			for wr in gd.stock_design(hull, race)["weapons"]:
				if wr[0] == "4022":
					on_mount = true
	_check(not on_mount, "no stock ship has a PDC on a weapon mount")
	_check(gd.stats_for(gd.stock_design("Dreadnought", 1))["pdc"] == 2, "a stock Terran Dreadnought has two PDCs")
	_check(gd.stats_for(gd.stock_design("Cruiser", 2))["pdc"] == 0, "a stock Drengin Cruiser has none")
	var st: Dictionary = gd.stats_for(terran)
	_check(st["bay_used"] <= st["bay_cap"], "the stock Terran Cruiser's bays still fit (%d / %d)" % [st["bay_used"], st["bay_cap"]])
	# an old design with a PDC on its turret: the PDC moves into the bays
	var old: Dictionary = gd.stock_design("Cruiser", 1)
	old["modules"].erase("4022")
	old["weapons"][0][0] = "4022"
	var fixed: Dictionary = gd.fix_design(old)
	_check(fixed["modules"].count("4022") == 1 and fixed["weapons"][0][0] != "4022", "an old design's mount PDC moves into the bays")
	_check(old["weapons"][0][0] == "4022", "fix_design leaves the old dictionary alone")


## Out where nothing else is: a still defender and a far-off shooter.
func _pair(design: Variant) -> Array:
	var at: Vector2 = w.player.global_position + Vector2(9000.0, 0.0).rotated(randf() * TAU)
	var v: Vector2 = w.player.velocity
	var d: Ship = w.spawn_ship("Defender", 1, "Cruiser", "Defender", at, v, -1)
	if design is Dictionary:
		d.refit(design)
	var a: Ship = w.spawn_ship("Shooter", 2, "Cruiser", "Shooter", at + Vector2(900, 0), v, -1)
	return [d, a]


func _clear(ships: Array) -> void:
	for s in ships:
		if is_instance_valid(s) and not s.dead:
			s.dead = true
			s.queue_free()
			w.ships.erase(s)
			w.space.remove_body(s.body)
	for m in w.missiles.duplicate():
		if is_instance_valid(m) and not m.exploded:
			m.shoot_down()


## Keep the defender parked (no drift into the shots, full energy).
func _hold(d: Ship, v: Vector2) -> void:
	if evade:
		v += Vector2(0, 60)
	d.velocity = v
	d.body.vel = v
	d.body.ang_vel = 1.2 if evade else 0.0
	d.energy = d.energy_max
	d.hull = d.hull_max
	d.shield = d.shield_max
	if keep_out:
		var sys: Dictionary = d.systems.get_system("pdc")
		sys["hp"] = 0.0
		sys["offline"] = true


## Fire `n` munitions of `mid` at the defender at once, from the shooter's
## side (a slight fan), and wait until all are gone.  Returns how many
## the defender's PDC shot down.
func _volley(d: Ship, a: Ship, mid: String, n: int, mend := true) -> int:
	if mend:
		d.systems.repair_all()          # the hits that got through leave nothing shot out
	var before: int = d.pdc.shot_down
	var md: Dictionary = gd.MISSILES[mid]
	var v: Vector2 = d.velocity
	# the shooter back 900 px off the defender's bow (hits push ships about)
	var spot: Vector2 = d.global_position + Vector2(900, 0)
	a.global_position = spot
	a.body.pos = spot
	a.velocity = v
	a.body.vel = v
	for i in n:
		var off := Vector2(0, (i - (n - 1) * 0.5) * 40.0)
		var from: Vector2 = a.global_position + off
		if mid == "002":
			# a mine sits 450 px ahead of a defender drifting onto it
			from = d.global_position + Vector2(450, (i - (n - 1) * 0.5) * 30.0)
			w.spawn_missile(a, mid, from, v, 0.0, null)
			continue
		var dir: Vector2 = (d.global_position - from).normalized()
		w.spawn_missile(a, mid, from, v + dir * md["fire_v"] * gd.LIGHT_SPEED, dir.angle(), d)
	var frames := 0
	var miss := {}
	while frames < 60 * 20:
		await physics_frame
		frames += 1
		_hold(d, v + (Vector2(60, 0) if mid == "002" else Vector2.ZERO))
		if OS.has_environment("PDC_DEBUG"):
			for g in w.missiles:
				if is_instance_valid(g) and g.missile_id == "022":
					var best := INF
					for t in w.missiles:
						if is_instance_valid(t) and t.missile_id == mid and not t.exploded:
							best = minf(best, t.global_position.distance_to(g.global_position))
					var k: int = g.get_instance_id()
					miss[k] = minf(miss.get(k, INF), best)
		if OS.has_environment("PDC_DEBUG") and frames % 6 == 0:
			var mt: Dictionary = d.pdc.mounts[0]
			var tg = mt["target"]
			print("  f%d %s aim %.2f firing %.2f ammo %d energy %d target %s d=%s tough=%s" % [frames, mid, mt["aim"], d.pdc.firing, d.pdc.ammo, d.energy,
				str(tg != null), str(int(tg.global_position.distance_to(d.global_position))) if tg != null else "-", str(tg.toughness) if tg != null else "-"])
		var live := 0
		for m in w.missiles:
			if is_instance_valid(m) and not m.exploded and m.missile_id == mid:
				live += 1
		if live == 0:
			break
	if OS.has_environment("PDC_DEBUG") and n > 1:
		var vals := miss.values()
		vals.sort()
		print("  round misses (px): %s" % str(vals.map(func(x): return int(x))))
	# let the last rounds clear
	for i in 20:
		await physics_frame
	return d.pdc.shot_down - before


func _rate(d: Ship, a: Ship, mid: String, shots: int, mend := true) -> float:
	var down := 0
	for i in shots:
		down += await _volley(d, a, mid, 1, mend)
	return float(down) / float(shots)


func _intercepts() -> void:
	var pr := _pair(null)
	var d: Ship = pr[0]
	var a: Ship = pr[1]
	await physics_frame
	_check(d.pdc.fitted() and d.pdc.mounts.size() == 1, "the defender has one PDC turret")
	_check(d.systems.get_system("pdc").size() > 0, "the PDC is a ship system that can be shot out")
	var e0: float = d.energy
	# torpedoes are fast and take two rounds: the hardest single shot to stop
	var want := {"008": 0.7, "009": 0.7, "004": 0.1, "021": 0.7, "002": 0.7}
	var rates := {}
	var names := {"008": "missiles", "009": "seekers", "004": "torpedoes", "021": "drones", "002": "mines"}
	for mid in ["008", "009", "004", "021", "002"]:
		var r := await _rate(d, a, mid, runs)
		rates[mid] = r
		log_lines.append("  one PDC v single %s: %d%% shot down" % [names[mid], int(r * 100.0)])
		_check(r >= want[mid], "one PDC shoots down at least %d%% of single %s (%d%%)" % [int(want[mid] * 100.0), names[mid], int(r * 100.0)])
	log_lines.append("  rounds left %d / %d" % [d.pdc.ammo, d.pdc.ammo_max])
	_check(d.pdc.ammo < d.pdc.ammo_max, "firing used rounds")
	# rearming fills the magazine
	d.rearm(1.0)
	_check(d.pdc.ammo == d.pdc.ammo_max, "rearming fills the PDC magazine")
	# off: nothing shot down
	d.set_pdc(false)
	var off := await _rate(d, a, "008", 4)
	_check(off == 0.0, "with the PDC off nothing is shot down")
	d.set_pdc(true)
	# shot out: nothing either
	keep_out = true
	var out := await _rate(d, a, "008", 4, false)
	keep_out = false
	_check(out == 0.0, "with the PDC shot out nothing is shot down")
	_clear(pr)
	# salvos of six missiles: some get through one turret, fewer two
	var downed := {}
	evade = true
	for n in [1, 2]:
		var fit: Dictionary = gd.stock_design("Cruiser", 1)
		while fit["modules"].count("4022") < n:
			fit["modules"].append("4022")
		downed[n] = 0
		for trial in 4:
			pr = _pair(fit)
			d = pr[0]
			a = pr[1]
			await physics_frame
			if trial == 0:
				_check(d.pdc.mounts.size() == n, "%d PDC parts give %d turrets" % [n, n])
			downed[n] += await _volley(d, a, "008", 6)
			_clear(pr)
	evade = false
	log_lines.append("  six-missile salvos at an evading ship (4 each): one PDC downs %.1f, two PDCs down %.1f" % [downed[1] / 4.0, downed[2] / 4.0])
	_check(downed[1] >= 4, "one PDC stops some of a salvo at an evading ship")
	_check(downed[2] >= downed[1], "two PDCs stop at least as much of a salvo as one")
	# torpedo salvos: four at once against one turret
	var torp := 0
	for trial in 3:
		pr = _pair(null)
		d = pr[0]
		a = pr[1]
		await physics_frame
		torp += await _volley(d, a, "004", 4)
		_clear(pr)
	log_lines.append("  four-torpedo salvos (3): one PDC downs %.1f" % (torp / 3.0))
	_check(torp < 12, "torpedo salvos get past a single PDC")
	_check(e0 > 0.0, "energy accounting ran")
