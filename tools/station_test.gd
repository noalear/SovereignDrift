# Station test (the Resources update, phase 3): buy a gas skimmer kit at
# the starbase, deploy it at Jupiter, deliver the materials to the frame
# and get paid, watch it build, take its Fuel; destroy a refinery frame
# and scoop up all it was given; see what a sensor station sees, fill up
# at a depot, refine at a refinery; then leave a frame and a mine to the
# computer supply ships.
#   godot --headless --path . -s tools/station_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var credits_before := 0
var skimmer: Ship = null
var frame: Ship = null
var depot_frame: Ship = null
var mine: Ship = null
var mine_had := 0.0
var gd: Node                 # the GameData autoload (not an identifier in a tool script)


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("/root/GameData")
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	await process_frame
	await process_frame


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


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel


## Hold the player alongside a station, at rest against it.
func _alongside(w: World, st: Ship) -> void:
	var out := (st.global_position - st.station_body.global_position).normalized() if st.station_body != null else Vector2.RIGHT
	_put(w.player, st.global_position + out * (st.size + w.player.size + 60.0), st.velocity)


## An uncolonised world with a surface and something to mine, the
## nearest to Earth that no other empire's colony guns can reach.
func _rock(w: World) -> Planet:
	var best: Planet = null
	var earth: Planet = w.planet_by_name["Earth"]
	for p in w.planets:
		if not p.has_surface() or p.is_colonised() or p.look == "Sun" or Economy.surface_of(p.look, p.habitable) == "gas" \
				or p.richness.get("metals", 0) <= 0:
			continue
		var hostile := false
		for q in w.planets:
			if q.is_colonised() and q.owner_race != w.player.race_id and q.global_position.distance_to(p.global_position) < 4000.0:
				hostile = true
		if not hostile and (best == null or p.global_position.distance_to(earth.global_position) < best.global_position.distance_to(earth.global_position)):
			best = p
	return best


## Stations here sit within reach of enemy colonies' guns: keep the ones
## the test needs standing.
func _watch(st: Ship) -> void:
	st.invulnerable = 1e9
	st.died.connect(func(s, killer): print("  ! %s destroyed by %s at t=%.1f" % [s.ship_name, killer.pilot_name if killer is Ship else str(killer), main.world.elapsed]))


func _process(_delta: float) -> bool:
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	if p == null:
		return false
	var now := w.elapsed
	p.invulnerable = 1e9
	var jupiter: Planet = w.planet_by_name["Jupiter"]
	match step:
		0:
			if now < 1.0:
				return false
			_haulers(w, false)
			_check(gd.is_station_hull("GasSkimmer") and gd.is_station_hull("Starbase") and not gd.is_station_hull("Freighter"), "station hulls are stations")
			_check(not gd.ship_names_for_rank(20).has("GasSkimmer"), "and never offered at the shipyard")
			var base := w.starbase_of(p.race_id)
			_put(p, base.global_position + Vector2(base.size + 120.0, 0), base.velocity)
			_check(w.nearest_base(p) == base and CargoPanel.can_open(w, p), "alongside the starbase the cargo panel opens for kits")
			p.rank = 0
			_check(World.kit_refusal(p, "GasSkimmer").contains("rank"), "a junior pilot can't buy a kit: %s" % World.kit_refusal(p, "GasSkimmer"))
			p.rank = gd.KIT_RANK
			_check(World.kit_refusal(p, "GasSkimmer").contains("Freighter"), "a small hold can't take one: %s" % World.kit_refusal(p, "GasSkimmer"))
			p.refit("Freighter")
			p.credits = 2000
			credits_before = p.credits
			w.kit_command(p, "GasSkimmer")
			_check(p.kit == "GasSkimmer" and p.credits == credits_before - 250, "bought a gas skimmer kit for 250 credits (%d left)" % p.credits)
			_check(p.cargo_space() == 0.0, "the kit fills the hold")
			w.kit_command(p, "Refinery")
			_check(p.kit == "GasSkimmer", "one kit at a time")
			# the wrong world
			var mars: Planet = w.planet_by_name["Mars"]
			_put(p, mars.global_position + Vector2(mars.radius + 400.0, 0), mars.velocity)
			_check(w.station_site(p, "GasSkimmer")["why"].contains("gas giant"), "Mars is no place for a skimmer: %s" % w.station_site(p, "GasSkimmer")["why"])
			var earth: Planet = w.planet_by_name["Earth"]
			_put(p, earth.global_position + Vector2(earth.radius + 300.0, 0), earth.velocity)
			_check(w.station_site(p, "MiningStation")["why"].contains("colony"), "nor a mine over Earth, with a colony there: %s" % w.station_site(p, "MiningStation")["why"])
			var neptune: Planet = w.planet_by_name["Neptune"]
			_put(p, neptune.global_position * 1.3, neptune.velocity)
			_check(w.station_site(p, "GasSkimmer")["why"].contains("Fly within"), "out in deep space the kit can't go up")
			_put(p, jupiter.global_position + Vector2(0, -(jupiter.radius + 500.0)), jupiter.velocity)
			_check(w.station_site(p, "GasSkimmer")["why"] == "", "over Jupiter the kit can go up")
			var before := w.ships.size()
			w.deploy_kit(p)
			_check(w.ships.size() == before + 1 and p.kit == "", "deployed: a new frame, and the hold is free")
			skimmer = w.ships[w.ships.size() - 1]
			_watch(skimmer)
			_check(skimmer.design_name == "GasSkimmer" and skimmer.building and skimmer.is_station and not skimmer.is_starbase(), "it is a gas skimmer frame")
			_check(skimmer.design["modules"] == ["1001"] and skimmer.weapons.is_empty() and skimmer.cargo_max == 25.0, "a bare frame waiting for 25 units")
			_check(skimmer.hull_max < 2000.0, "with little hull (%d)" % int(skimmer.hull_max))
			_check(skimmer.station_body == jupiter and skimmer.race_id == p.race_id, "kept over Jupiter for the Terrans")
			p.kit = "GasSkimmer"
			_check(w.station_site(p, "GasSkimmer")["why"].contains("already"), "a second skimmer at Jupiter is refused: %s" % w.station_site(p, "GasSkimmer")["why"])
			p.kit = ""
			t0 = now
			step = 1
		1:
			if now - t0 < 1.0:
				return false
			var off: Vector2 = skimmer.global_position - jupiter.global_position
			_check(absf(off.length() - float(skimmer.get_meta("station_r"))) < 1.0 and skimmer.velocity.distance_to(jupiter.velocity) < 0.1, "the frame rides along with Jupiter")
			# bring the materials: Metals and Alloys, plus Food it has no use for
			p.cargo = {"metals": 30.0, "alloys": 5.0, "food": 10.0}
			p.cargo_origin = w.planet_by_name["Earth"]
			_alongside(w, skimmer)
			_check(w.cargo_port(p) == skimmer, "alongside the frame it is a cargo port")
			_check(w.port_room(skimmer, "metals") == 20.0 and w.port_room(skimmer, "food") == 0.0, "it wants 20 Metals and no Food")
			credits_before = p.credits
			w.cargo_command(p, "all", -1.0)
			t0 = now
			step = 2
		2:
			_alongside(w, skimmer)
			if now - t0 < 3.0:
				return false
			_check(absf(skimmer.cargo.get("metals", 0.0) - 20.0) < 0.01 and absf(skimmer.cargo.get("alloys", 0.0) - 5.0) < 0.01, "the frame took 20 Metals and 5 Alloys")
			_check(absf(p.cargo.get("metals", 0.0) - 10.0) < 0.01 and absf(p.cargo.get("food", 0.0) - 10.0) < 0.01, "the rest stays aboard (%s)" % str(p.cargo))
			_check(p.credits - credits_before == 35, "paid the full price: 20 + 5 x 3 = %d credits" % (p.credits - credits_before))
			_check(skimmer.build_t > 0.5, "the work has begun (%.1f s)" % skimmer.build_t)
			skimmer.build_t = gd.STATIONS["GasSkimmer"]["build"] - 0.5
			t0 = now
			step = 3
		3:
			if now - t0 < 1.0:
				return false
			_check(not skimmer.building and skimmer.design["modules"].size() == 3 and skimmer.cargo_max == 300.0, "the skimmer is finished")
			_check(skimmer.cargo_total() < 1.0 and skimmer.hull_max > 1000.0, "built from its materials, with a station's hull (%d)" % int(skimmer.hull_max))
			skimmer.cargo.clear()
			Economy.station_step(skimmer, 60.0)
			_check(absf(skimmer.cargo.get("fuel", 0.0) - 15.0) < 0.01, "it skims 15 Fuel a minute from Jupiter (%.1f)" % skimmer.cargo.get("fuel", 0.0))
			p.cargo.clear()
			_alongside(w, skimmer)
			_check(w.port_room(skimmer, "metals") == 0.0, "a skimmer takes nothing in")
			w.cargo_command(p, "all", 1.0)
			t0 = now
			step = 4
		4:
			_alongside(w, skimmer)
			if now - t0 < 2.0:
				return false
			_check(p.cargo.get("fuel", 0.0) >= 14.0 and p.cargo_origin == skimmer, "loaded its Fuel (%.1f)" % p.cargo.get("fuel", 0.0))
			var luna: Planet = w.planet_by_name["Luna"]
			luna.stock["fuel"] = 0.0
			_check(w.cargo_pay(luna, skimmer, "fuel", 10.0) > 0.0, "skimmed Fuel pays where it is short")
			# a refinery frame, part-supplied, then shot to pieces
			var mars: Planet = w.planet_by_name["Mars"]
			frame = w.build_station(p.race_id, "Refinery", mars, mars.global_position + Vector2(-(mars.radius + 500.0), 0))
			frame.cargo = {"metals": 30.0, "alloys": 2.0}
			var before := w.powerups.size()
			frame._die(null)
			var crates := w.powerups.filter(func(x): return is_instance_valid(x) and x.kind == "cargo")
			_check(crates.any(func(x): return x.module_id == "metals" and x.amount == 30) and crates.any(func(x): return x.module_id == "alloys" and x.amount == 2),
				"a wrecked frame spills all it was given: 30 Metals, 2 Alloys (%d new pickups)" % (w.powerups.size() - before))
			t0 = now
			step = 5
		5:
			if now - t0 < 4.0:
				return false
			_check(not is_instance_valid(frame) or not w.ships.has(frame), "and is gone for good, not rebuilt")
			# a sensor station sees what the pilot can't
			var mars: Planet = w.planet_by_name["Mars"]
			var post := w.build_station(p.race_id, "SensorStation", mars, mars.global_position + Vector2(0, mars.radius + 600.0), false)
			_check(post.sensor_range >= 6000.0, "a sensor station sees 6000 px")
			var enemy: Ship = null
			for s in w.ships:
				if s.race_id == 2 and not s.is_station and not s.dead and not s.support and not s.is_landing() and not s.warping:
					enemy = s
					break
			_put(p, post.global_position - Vector2(3000.0, 0), mars.velocity)   # 5500 px from the enemy, well past its own sensors
			# (a station's 6000 px dish sees a cruiser-sized return out to about 5000)
			_put(enemy, post.global_position + Vector2(2500.0, 0), mars.velocity)
			w._update_sensor_links()
			for s in w.ships:
				s.sensors._own.clear()
				s.sensors._seen.clear()
			_check(enemy.global_position.distance_to(p.global_position) > p.sensor_range and enemy.visible_to(p), "an enemy 2500 px from it shows to the pilot far away")
			_put(enemy, post.global_position + Vector2(7000.0, 0), mars.velocity)
			for s in w.ships:        # what was seen is cached for a moment: look afresh
				s.sensors._own.clear()
				s.sensors._seen.clear()
			_check(not enemy.visible_to(p), "one 7000 px out does not")
			# a depot refuels; a refinery refines
			var depot := w.build_station(p.race_id, "SupplyDepot", jupiter, jupiter.global_position + Vector2(jupiter.radius + 700.0, 0), false)
			_alongside(w, depot)
			p.fuel = p.fuel_max * 0.3
			t0 = now
			step = 6
			set_meta("depot", depot)
			set_meta("fuel0", p.fuel)
		6:
			var depot: Ship = get_meta("depot")
			_alongside(w, depot)
			if now - t0 < 1.0:
				return false
			_check(p.fuel > float(get_meta("fuel0")) + 1.0, "a supply depot refuels ships alongside")
			_check(w.port_room(depot, "food") > 1000.0, "and stores anything")
			var ref := w.build_station(p.race_id, "Refinery", jupiter, jupiter.global_position + Vector2(-(jupiter.radius + 700.0), 0), false)
			_check(w.port_room(ref, "metals") > 0.0 and w.port_room(ref, "food") == 0.0, "a refinery takes Metals and Fuel only")
			ref.cargo = {"metals": 20.0, "fuel": 10.0}
			Economy.station_step(ref, 60.0)
			_check(absf(ref.cargo.get("alloys", 0.0) - 6.0) < 0.01 and absf(ref.cargo["metals"] - 8.0) < 0.01 and absf(ref.cargo["fuel"] - 4.0) < 0.01, "and refines 6 Alloys a minute from 2 Metals and 1 Fuel each (%s)" % str(ref.cargo))
			# a defensive station gets its gunners when it is finished
			var guns := w.build_station(p.race_id, "DefenseStation", jupiter, jupiter.global_position + Vector2(0, jupiter.radius + 900.0))
			_check(guns.get_node_or_null("AI") == null and guns.weapons.is_empty(), "a defensive frame has no guns yet")
			w.finish_station(guns)
			_check(guns.get_node_or_null("AI") != null and guns.weapons.size() == 3, "finished, it is armed and crewed")
			# a mine site, and the supply ships at work: a depot frame by
			# Earth and a full mine for them
			var rock := _rock(w)
			_check(rock != null, "an empty rock to mine: %s" % (rock.body_name if rock != null else "none"))
			p.kit = "MiningStation"
			_put(p, rock.global_position + Vector2(rock.radius + 300.0, 0), rock.velocity)
			_check(w.station_site(p, "MiningStation")["why"] == "", "a mining station can go up over %s %s" % [rock.body_name, w.station_site(p, "MiningStation")["why"]])
			p.kit = ""
			mine = w.build_station(p.race_id, "MiningStation", rock, rock.global_position + Vector2(rock.radius + 300.0, 0), false)
			Economy.station_step(mine, 60.0)
			_check(mine.cargo_total() > 1.0, "it mines %s a minute" % str(Economy.station_production(mine)))
			mine.cargo = {"metals": 200.0}
			mine_had = 200.0
			var earth: Planet = w.planet_by_name["Earth"]
			earth.stock["metals"] = 450.0
			depot_frame = w.build_station(p.race_id, "SupplyDepot", earth, earth.global_position + Vector2(0, -(earth.radius + 900.0)))
			_watch(depot_frame)
			_watch(mine)
			_put(p, w.starbase_of(p.race_id).global_position + Vector2(300, 0), w.starbase_of(p.race_id).velocity)
			_haulers(w, true)
			for s in w.ships:
				if s.support and s.race_id == p.race_id:
					s.invulnerable = 1e9      # this checks their runs, not how they fare under fire
			t0 = now
			step = 7
			print("watching the supply ships for up to 240 s of game time...")
		7:
			var done_frame := depot_frame.cargo_total() >= depot_frame.cargo_max - 0.01 or not depot_frame.building
			var done_mine := mine.cargo_total() < mine_had - 50.0
			if int(now) % 10 == 0 and not has_meta("tick%d" % int(now)):
				set_meta("tick%d" % int(now), true)
				for s in w.ships:
					if s.support and s.race_id == p.race_id:
						var ai: AIPilot = s.get_node("AI")
						print("  t=%d %s: state %d job %s dock %s hold %d, %.0f px from the mine, rel v %.0f, port %s" % [int(now - t0), s.ship_name, ai.state, ai.job, ai.dock_at.ship_name if ai.dock_at != null else "-", int(s.cargo_total()), s.global_position.distance_to(mine.global_position), (s.velocity - mine.velocity).length(), str(w.cargo_port(s))])
			if now - t0 < 240.0 and not (done_frame and done_mine):
				return false
			for s in w.ships:
				if s.support and s.race_id == p.race_id:
					var ai: AIPilot = s.get_node("AI")
					print("  hauler %s: state %d job %s dock %s hold %s, %.0f px from the mine" % [s.ship_name, ai.state, ai.job, ai.dock_at.ship_name if ai.dock_at != null else "-", str(s.cargo), s.global_position.distance_to(mine.global_position)])
			_check(done_frame, "the supply ships brought the depot frame its 25 Metals (%d / %d)" % [int(depot_frame.cargo_total()), int(depot_frame.cargo_max)])
			_check(done_mine, "and emptied the mine (%d left of %d)" % [int(mine.cargo_total()), int(mine_had)])
			print("station test %s" % ("FAILED" if failed else "passed"))
			quit(1 if failed else 0)
	return false
