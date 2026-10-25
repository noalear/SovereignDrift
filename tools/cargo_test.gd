# Cargo test (the Resources update, phase 2): load Food at Earth, find
# unloading where it was loaded pays nothing, carry it to Luna and get paid
# for the shortfall, fill Luna to its cap, spill a wreck's hold and scoop it
# up, take a supply ship's cargo by capture, and watch the computer supply
# ships haul between colonies on their own.
#   godot --headless --path . -s tools/cargo_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var credits_before := 0
var score_before := 0
var earth_food := 0.0
var mass_before := 0.0
var deliveries := 0
var loads := 0


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	await process_frame
	await process_frame
	for s in main.world.ships:
		s.touched_down.connect(_on_touchdown)


func _on_touchdown(s: Ship, p: Planet) -> void:
	if not s.support:
		return
	var ai: AIPilot = s.get_node_or_null("AI")
	if ai == null:
		return
	if ai.job == "load":
		loads += 1
	elif ai.job == "deliver":
		deliveries += 1
	print("  supply ship %s touched down on %s to %s (hold %d, for %s)" % [s.ship_name, p.body_name, ai.job, int(s.cargo_total()), ai.haul_to.body_name if ai.haul_to != null else "-"])


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _haulers(w: World, on: bool) -> void:
	for s in w.ships:
		if s.support:
			var ai: AIPilot = s.get_node_or_null("AI")
			ai.set_physics_process(on)
			if not on:
				for k in s.inputs.keys():
					s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0


func _ferries_off(w: World) -> void:
	for s in w.ships:
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai != null and not s.support:
			ai.ferry = false


func _park(w: World, p: Planet, alt: float = 90.0) -> void:
	var s: Ship = w.player
	var dir := (p.global_position - w.sun.global_position).normalized().rotated(0.7)
	s.global_position = p.global_position + dir * (p.radius + alt)
	s.velocity = p.velocity
	s.body.pos = s.global_position
	s.body.vel = s.velocity


func _process(_delta: float) -> bool:
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	var now := w.elapsed
	if p == null:
		return false
	p.invulnerable = 1e9
	var earth: Planet = w.planet_by_name["Earth"]
	var luna: Planet = w.planet_by_name["Luna"]
	match step:
		0:
			if now < 1.0:
				return false
			_check(p.race_id == 1, "the test pilot flies for the Terrans")
			var haulers := w.ships.filter(func(s): return s.support)
			_check(haulers.size() == 6 and haulers.all(func(s): return s.design_name == "Freighter"), "two supply Freighters per empire (%d)" % haulers.size())
			_check(haulers.all(func(s): return (s.get_node("AI") as AIPilot).hauler and not (s.get_node("AI") as AIPilot).ferry), "they run the supply-ship AI, not the colonist ferry")
			_haulers(w, false)
			_ferries_off(w)
			p.refit("Freighter")
			_check(p.cargo_max == 300.0 and p.cargo_total() == 0.0, "a Freighter holds 300 units")
			var scout := Ship.new()
			_check(GameData.CARGO_HOLD["Scout"] < GameData.CARGO_HOLD["Cruiser"], "bigger hulls carry more")
			scout.free()
			earth.stock["food"] = 400.0
			luna.stock["food"] = 0.0
			mass_before = p.wet_mass()
			_park(w, earth)
			w.try_land(p)
			t0 = now
			step = 1
		1:
			if now - t0 < Ship.DESCENT_TIME + 0.3:
				return false
			_check(p.is_landed() and p.land_body == earth, "landed on Earth")
			earth_food = earth.stock["food"]
			w.cargo_command(p, "food", 120.0)
			_check(p.cargo_orders.get("food", 0.0) == 120.0, "ordered 120 Food aboard")
			t0 = now
			step = 2
		2:
			if now - t0 < 5.0:
				return false
			_check(absf(p.cargo.get("food", 0.0) - 120.0) < 0.5, "120 Food aboard (%.1f)" % p.cargo.get("food", 0.0))
			_check(earth.stock["food"] < earth_food - 100.0, "Earth's stock went down (%.0f -> %.0f)" % [earth_food, earth.stock["food"]])
			_check(p.cargo_origin == earth, "the cargo remembers it came from Earth")
			_check(p.wet_mass() > mass_before + 40.0, "a loaded hold is heavier (%.0f -> %.0f)" % [mass_before, p.wet_mass()])
			credits_before = p.credits
			w.cargo_command(p, "food", -20.0)
			t0 = now
			step = 3
		3:
			if now - t0 < 3.0:
				return false
			_check(absf(p.cargo.get("food", 0.0) - 100.0) < 0.5 and p.credits == credits_before, "unloading where it was loaded pays nothing (%d -> %d)" % [credits_before, p.credits])
			w.try_land(p)
			t0 = now
			step = 4
		4:
			if now - t0 < Ship.ASCENT_TIME + 0.2:
				return false
			_check(p.cargo_orders.is_empty() and p.cargo_total() >= 99.0, "lifted off with the cargo")
			_park(w, luna)
			w.try_land(p)
			t0 = now
			step = 5
		5:
			if now - t0 < Ship.DESCENT_TIME + 0.3:
				return false
			_check(p.is_landed() and p.land_body == luna, "landed on Luna")
			luna.stock["food"] = 0.0
			credits_before = p.credits
			score_before = p.score
			# Luna (an Outpost, cap 200) is short of Food and makes none:
			# 50 units to a quarter of its cap pay 1 each, the next 50 half
			var expect := Economy.delivery_pay(luna, earth, "food", 100.0)
			_check(absf(expect - 75.0) < 0.01, "the delivery is worth 75 credits (%.1f)" % expect)
			w.cargo_command(p, "all", -1.0)
			t0 = now
			step = 6
		6:
			if now - t0 < 5.0:
				return false
			_check(p.cargo_total() < 0.5 and luna.stock["food"] >= 99.0, "unloaded everything on Luna (stock %.0f)" % luna.stock["food"])
			_check(p.credits - credits_before >= 74 and p.credits - credits_before <= 76, "paid for the shortfall (%d credits)" % (p.credits - credits_before))
			_check(p.score > score_before, "and a little score (%d -> %d)" % [score_before, p.score])
			_check(p.cargo_origin == null, "an empty hold forgets where it loaded")
			# the cap: only the room Luna has is unloaded
			luna.stock["food"] = 190.0
			p.cargo["food"] = 50.0
			p.cargo_origin = earth
			w.cargo_command(p, "food", -50.0)
			t0 = now
			step = 7
		7:
			if now - t0 < 3.0:
				return false
			_check(absf(luna.stock["food"] - 200.0) < 0.5 and absf(p.cargo.get("food", 0.0) - 40.0) < 0.5, "unloads stop at Luna's cap (stock %.0f, aboard %.0f)" % [luna.stock["food"], p.cargo.get("food", 0.0)])
			_check(p.cargo_orders.is_empty(), "no orders left after the cap")
			w.try_land(p)
			t0 = now
			step = 8
		8:
			if now - t0 < Ship.ASCENT_TIME + 0.2:
				return false
			# a wreck spills half its hold
			var victim: Ship = null
			for s in w.ships:
				if not s.is_player and not s.is_station and not s.dead and s.race_id != p.race_id and not s.support:
					victim = s
					break
			victim.cargo = {"metals": 30.0, "exotics": 9.0}
			var before := w.powerups.size()
			victim._die(null)
			var chips := w.powerups.filter(func(x): return is_instance_valid(x) and x.kind == "cargo")
			_check(chips.size() == 2, "the wreck spilled two cargo crates (%d new pickups)" % (w.powerups.size() - before))
			_check(chips.any(func(x): return x.module_id == "metals" and x.amount == 15) and chips.any(func(x): return x.module_id == "exotics" and x.amount == 4), "half of each: 15 Metals, 4 Exotics")
			_check(victim.cargo.is_empty(), "the wreck's hold is gone")
			p.cargo = {"food": 40.0}
			for c in chips:
				c.global_position = p.global_position
				c.body.pos = p.global_position
			t0 = now
			step = 9
		9:
			if now - t0 < 0.5:
				return false
			_check(absf(p.cargo.get("metals", 0.0) - 15.0) < 0.1 and absf(p.cargo.get("exotics", 0.0) - 4.0) < 0.1, "flying through the crates scoops them up (%s)" % str(p.cargo))
			# capture: the captor takes the hold up to its room, the prize keeps the rest
			var hauler: Ship = null
			for s in w.ships:
				if s.support and s.race_id != p.race_id and not s.dead:
					hauler = s
					break
			hauler.cargo = {"alloys": 280.0}
			var room := p.cargo_space()
			var before_ships := w.ships.size()
			w.capture.take(p, hauler)
			_check(absf(p.cargo.get("alloys", 0.0) - room) < 0.5, "the captor took %.0f Alloys, all its hold had room for" % room)
			var prize: Ship = w.ships[w.ships.size() - 1] if w.ships.size() > before_ships else null
			_check(prize != null and prize.is_prize and absf(prize.cargo.get("alloys", 0.0) - (280.0 - room)) < 0.5, "the prize keeps the rest (%s)" % (str(prize.cargo) if prize != null else "no prize"))
			_check(hauler.dead and hauler.cargo.is_empty(), "the captured pilot relaunches with an empty hold")
			# now leave the computer supply ships to it, with work waiting:
			# Earth piled with Food, Luna empty of it
			p.cargo.clear()
			_haulers(w, true)
			earth.stock["food"] = 1000.0
			luna.stock["food"] = 0.0
			t0 = now
			step = 10
			print("watching the supply ships for 200 s of game time...")
		10:
			if now - t0 < 200.0 and deliveries < 3:
				return false
			print("supply loads: %d, deliveries: %d" % [loads, deliveries])
			_check(loads >= 1, "supply ships land to load")
			_check(deliveries >= 1, "and deliver")
			print("cargo test %s" % ("FAILED" if failed else "passed"))
			quit(1 if failed else 0)
	return false
