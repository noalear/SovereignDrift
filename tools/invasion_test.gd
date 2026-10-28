# Invasion test (the Resources update, phase 5): recruit and build at home,
# take troops aboard a drop ship, find Mars's planetary shield in the way,
# shoot it down, knock the colony out, land, send the troops in, watch the
# commandos blow up the shield generator and the battle take the world with
# its stockpile and buildings.  Then a landing too small to win, marines in
# a boarding, upkeep and desertion, flak, industry and farms, the wire
# format, and a computer drop ship making a landing of its own.
#   godot --headless --path . -s tools/invasion_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var mars_pop := 0.0
var mars_stock := {}
var earth_inf := 0.0
var dropper: Ship = null


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


## Park the player just above a body, moving with it.
func _park(w: World, p: Planet, alt: float = 60.0) -> void:
	var s: Ship = w.player
	var dir := (p.global_position - w.sun.global_position).normalized().rotated(0.7)
	s.global_position = p.global_position + dir * (p.radius + alt)
	s.velocity = p.velocity
	s.body.pos = s.global_position
	s.body.vel = s.velocity


func _freeze_droppers(w: World, on: bool) -> void:
	for s in w.ships:
		if s.support and s.design_name == "DropShip":
			var ai: AIPilot = s.get_node("AI")
			ai.set_physics_process(not on)
			if on:
				for k in s.inputs.keys():
					s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
				if s.warping:
					s.end_warp()


func _process(_delta: float) -> bool:
	if main == null or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	var now := w.elapsed
	p.invulnerable = 1e9
	var gd: Node = root.get_node("GameData")
	var earth: Planet = w.planet_by_name["Earth"]
	var mars: Planet = w.planet_by_name["Mars"]
	match step:
		0:
			if now < 1.0:
				return false
			_freeze_droppers(w, true)
			# the scenario's ground forces
			_check(mars.buildings.has("shield") and mars.buildings.has("barracks") and mars.pshield_up(), "Mars starts with a barracks and a standing planetary shield (%s)" % [mars.buildings])
			_check(earth.buildings.has("shield") and absf(earth.pshield - Ground.PSHIELD_HP) < 1.0, "Earth's shield starts charged")
			_check(Ground.count(mars.garrison) == 4 and Ground.strength(mars.garrison) == 7.0, "Mars's garrison: %s" % Ground.units_text(mars.garrison))
			_check(Ground.count(w.planet_by_name["Phobos"].garrison) == 0, "outposts start with militia only")
			_check(absf(Ground.militia(mars) - 2.4) < 0.01, "1.2 B people raise militia 2.4 (%.2f)" % Ground.militia(mars))
			var drops := w.ships.filter(func(s): return s.support and s.design_name == "DropShip")
			_check(drops.size() == 3, "each empire flies a computer drop ship (%d)" % drops.size())
			_check("DropShip" in gd.yard_hulls() and gd.ORDERS.has("DropShip"), "the shipyard sells and builds drop ships")
			var card: Array = mars.info_lines(p.race_id)
			_check(card.any(func(r): return r[0].begins_with("Planetary shield")) and card.any(func(r): return r[0].begins_with("Garrison")), "Mars's card shows its shield and garrison")
			# home: build and recruit
			p.rank = 7
			p.refit("DropShip")
			_check(p.troop_max == 6 and p.troop_count() == 0, "a drop ship carries six units")
			_park(w, earth)
			w.try_land(p, earth)
			t0 = now
			step = 1
		1:
			if not p.is_landed():
				if now - t0 > 8.0:
					_check(false, "landed on Earth")
					step = 99
				return false
			_check(p.is_landed(), "landed on Earth")
			earth.stock["metals"] = maxf(earth.stock["metals"], 60.0)
			earth.stock["alloys"] = maxf(earth.stock["alloys"], 20.0)
			var metals: float = earth.stock["metals"]
			var pop := earth.population
			_check(Ground.recruit_refusal(p, earth, "infantry") == "", "Earth can recruit (%s)" % Ground.recruit_refusal(p, earth, "infantry"))
			earth_inf = earth.garrison.get("infantry", 0.0)
			w.recruit_command(p, "infantry")
			_check(earth.recruit_q.size() == 1 and absf(earth.stock["metals"] - (metals - 2.0)) < 0.5 and absf(pop - earth.population - 50e6) < 1e6, "recruiting takes 2 Metals and 50 M people")
			_check("already" in Ground.building_refusal(p, earth, "shield"), "one shield a world (%s)" % Ground.building_refusal(p, earth, "shield"))
			w.build_command(p, "flak")
			_check(earth.build_q.size() == 1 and earth.build_q[0][0] == "flak", "a flak battery goes up on Earth")
			_check("busy" in Ground.building_refusal(p, earth, "laser"), "one building at a time (%s)" % Ground.building_refusal(p, earth, "laser"))
			p.rank = 1
			_check("Lt. Cmdr." in Ground.recruit_refusal(p, earth, "infantry"), "recruiting needs Lt. Cmdr. (%s)" % Ground.recruit_refusal(p, earth, "infantry"))
			p.rank = 7
			# troops aboard: everything the garrison has
			var before := Ground.count(earth.garrison)
			w.troop_command(p, "load", "")
			_check(p.troop_count() == mini(before, 6) and Ground.count(earth.garrison) == before - p.troop_count(), "the garrison went aboard (%s)" % Ground.units_text(p.troops))
			w.troop_command(p, "unload", "armour")
			_check(p.troops.get("armour", 0.0) == 0.0 and earth.garrison.get("armour", 0.0) == 1.0, "one unit back ashore")
			# the landing force for Mars
			p.troops = {"infantry": 3.0, "armour": 2.0, "commandos": 1.0}
			w.try_land(p)
			t0 = now
			step = 2
		2:
			if now - t0 < Ship.ASCENT_TIME + 0.3:
				return false
			mars_stock = mars.stock.duplicate()
			_park(w, mars)
			var why := p.landing_refusal(mars)
			_check("planetary shield" in why, "Mars's shield keeps the drop ship out (%s)" % why)
			_check(Ground.bunker_factor(mars) == 1.5, "Mars's defenders sit behind bunkers")
			# shoot it down
			var pop := mars.population
			mars.take_damage(Ground.PSHIELD_HP * 0.5, p)
			_check(mars.pshield_up() and mars.population == pop and mars.hull == mars.hull_max, "the shield takes the fire, and nobody below dies")
			mars.take_damage(Ground.PSHIELD_HP, p)
			_check(not mars.pshield_up() and absf(mars.pshield_down - Ground.PSHIELD_DOWN) < 0.01, "the shield collapses for %d s" % int(Ground.PSHIELD_DOWN))
			_check(p.landing_refusal(mars) == "", "with the shield down the drop ship may land (%s)" % p.landing_refusal(mars))
			# knock the colony out too: the bunkers stop counting
			mars.take_damage(1e6, p)
			_check(mars.is_defenceless() and Ground.bunker_factor(mars) == 1.0, "knocked out, its defenders fight at 1x")
			_park(w, mars)
			w.try_land(p, mars)
			t0 = now
			step = 3
		3:
			mars.silenced = maxf(mars.silenced, 5.0)      # the fleet keeps it quiet
			if not p.is_landed():
				if now - t0 > 8.0:
					_check(false, "landed on Mars (%s)" % p.landing_refusal(mars))
					step = 99
				return false
			_check("enemy colony" in Ground.troop_refusal(p, "unload", ""), "troops can't just go ashore on an enemy world")
			# two more infantry dug in, so the battle outlasts the commandos'
			# first SABOTAGE_TIME however many of our warships pass overhead
			mars.garrison["infantry"] = mars.garrison.get("infantry", 0.0) + 2.0
			w.troop_command(p, "land")
			_check(not mars.battle.is_empty() and int(mars.battle["race"]) == p.race_id and p.troop_count() == 0, "the troops went in: a ground battle on Mars")
			var card: Array = mars.info_lines(p.race_id)
			_check(card.any(func(r): return r[0].begins_with("GROUND BATTLE")) and card.any(func(r): return r[1] == "bar"), "the card shows the battle and its bar")
			var packed := Ground.pack(mars)
			var copy := Planet.new()
			Ground.unpack(copy, packed)
			_check(copy.battle.hash() == mars.battle.hash() and copy.garrison.hash() == mars.garrison.hash() and copy.buildings == mars.buildings, "the ground state survives the wire")
			copy.free()
			t0 = now
			step = 4
		4:
			mars.silenced = maxf(mars.silenced, 5.0)
			if mars.owner_race == 2 and not mars.battle.is_empty() and now - t0 < 90.0:
				mars_pop = mars.population      # passing ships bomb the defenceless cities meanwhile
				return false
			_check(mars.owner_race == p.race_id, "the troops took Mars in %d s" % int(now - t0))
			_check(Ground.count(mars.garrison) >= 1 and mars.battle.is_empty(), "the survivors hold it: %s" % Ground.units_text(mars.garrison))
			_check(not mars.buildings.has("shield") and mars.buildings.has("barracks"), "the commandos blew up the shield generator; the barracks changed hands (%s)" % [mars.buildings])
			var kept := true
			for k in mars_stock:
				if mars.stock.get(k, 0.0) < mars_stock[k] * 0.5 - 1.0:
					kept = false
			_check(kept, "Mars's stockpile changed hands with it")
			_check(mars.population >= mars_pop * 0.95, "its people stayed (%s of %s)" % [gd.fmt_pop(mars.population), gd.fmt_pop(mars_pop)])
			# a landing too small: one infantry against Venus's garrison
			w.try_land(p)
			p.troops = {"infantry": 1.0}
			t0 = now
			step = 5
		5:
			if now - t0 < Ship.ASCENT_TIME + 0.3:
				return false
			var venus: Planet = w.planet_by_name["Venus"]
			_park(w, venus)
			w.try_land(p, venus)
			t0 = now
			step = 6
		6:
			var venus: Planet = w.planet_by_name["Venus"]
			if not p.is_landed():
				if now - t0 > 8.0:
					_check(false, "landed on Venus (%s)" % p.landing_refusal(venus))
					step = 99
				return false
			w.troop_command(p, "land")
			t0 = now
			step = 7
		7:
			var venus: Planet = w.planet_by_name["Venus"]
			if not venus.battle.is_empty() and now - t0 < 30.0:
				return false
			_check(venus.owner_race == 2 and venus.battle.is_empty(), "one infantry against Venus's garrison is wiped out")
			_check(Ground.count(venus.garrison) >= 1, "Venus's garrison holds: %s" % Ground.units_text(venus.garrison))
			# marines: a boarding goes faster with infantry aboard, faster still with commandos
			var foe: Ship = null
			for s in w.ships:
				if s.race_id == 2 and not s.is_station and not s.support and not s.dead:
					foe = s
					break
			p.refit("Cruiser")
			foe.crew = foe.crew_max * 0.3
			p.troops = {}
			var e0 := Capture.board_estimate(p, foe)
			p.troops = {"infantry": 2.0}
			var e1 := Capture.board_estimate(p, foe)
			p.troops = {"infantry": 1.0, "commandos": 1.0}
			var e2 := Capture.board_estimate(p, foe)
			_check(e1 < e0 * 0.75 and e2 < e1, "marines speed up a boarding (%.1f s, with 2 infantry %.1f s, with commandos %.1f s)" % [e0, e1, e2])
			p.troops = {}
			# upkeep: a world with no Fuel or Food loses its shield, then its troops
			var luna: Planet = w.planet_by_name["Luna"]
			luna.buildings.append("shield")
			luna.pshield = Ground.PSHIELD_HP
			luna.garrison = {"infantry": 2.0}
			luna.stock["fuel"] = 0.0
			luna.stock["food"] = 0.0
			w.ground._upkeep(luna, 60.0)
			_check(luna.short.has("fuel") and luna.short.has("food"), "Luna's shield and troops go short (%s)" % [luna.short.keys()])
			w.ground._upkeep(luna, Ground.DESERT_TIME - 59.0)
			_check(Ground.count(luna.garrison) == 1, "a unit deserted after %d s short (%s)" % [int(Ground.DESERT_TIME), Ground.units_text(luna.garrison)])
			t0 = now
			step = 8
		8:
			var luna: Planet = w.planet_by_name["Luna"]
			luna.stock["fuel"] = 0.0
			if now - t0 < 2.0:
				return false
			_check(luna.pshield < Ground.PSHIELD_HP * 0.99, "without Fuel the shield fades (%d)" % int(luna.pshield))
			# industry and farms
			var io: Planet = w.planet_by_name["Io"]
			var before: Dictionary = Economy.production(io)
			io.buildings.append("industry")
			io.buildings.append("farm")
			io.short.erase("ice")
			var after: Dictionary = Economy.production(io)
			_check(absf(after["metals"] - before["metals"] * 1.5) < 0.01 and after["food"] > before["food"] + 1.0, "industry adds half to Metals, farms add Food (%.1f -> %.1f, %.1f -> %.1f)" % [before["metals"], after["metals"], before["food"], after["food"]])
			_check(Economy.refine_rate(io) >= Ground.INDUSTRY_REFINE, "industry lets an outpost refine Alloys")
			io.buildings.erase("industry")
			io.buildings.erase("farm")
			step = 9
		9:
			# the barracks and the builders finish on Earth
			if now - t0 < 50.0:
				return false
			_check(earth.garrison.get("infantry", 0.0) >= 1.0, "the recruit finished training (%s)" % Ground.units_text(earth.garrison))
			_check(earth.buildings.has("flak"), "the flak battery is up on Earth")
			# flak: an enemy torpedo near Earth is shot down
			var foe: Ship = null
			for s in w.ships:
				if s.race_id == 2 and not s.is_station and not s.dead:
					foe = s
					break
			var at := earth.global_position + Vector2(earth.radius + 300.0, 0)
			w.spawn_missile(foe, "008", at, earth.velocity, PI, earth)
			t0 = now
			step = 10
		10:
			if now - t0 < 1.0:
				return false
			var near := 0
			for m in w.missiles:
				if is_instance_valid(m) and not m.exploded and m.owner_race == 2 and m.global_position.distance_to(earth.global_position) < earth.radius + Ground.FLAK_RANGE:
					near += 1
			_check(near == 0, "Earth's flak shot the missile down")
			# a computer drop ship lands troops of its own: on knocked-out Europa
			var europa: Planet = w.planet_by_name["Europa"]
			for s in w.ships:
				if s.support and s.design_name == "DropShip" and s.race_id == 1:
					dropper = s
			europa.take_damage(1e6, p)
			dropper.troops = {"infantry": 4.0, "armour": 1.0}
			if dropper.warping:
				dropper.end_warp()
			var dai: AIPilot = dropper.get_node("AI")
			dai.state = AIPilot.State.PATROL
			dai.job_planet = null
			dai.plan_timer = 0.0
			dropper.leave_ground_now()     # frozen mid-muster on some world: up off it first, or it stays put there
			dropper.global_position = europa.global_position + (europa.global_position - w.sun.global_position).normalized() * (europa.radius + 600.0)
			dropper.velocity = europa.velocity
			dropper.body.pos = dropper.global_position
			dropper.body.vel = dropper.velocity
			dropper.invulnerable = 1e9
			_freeze_droppers(w, false)
			print("watching the Terran drop ship for up to 120 s...")
			t0 = now
			step = 11
		11:
			var europa: Planet = w.planet_by_name["Europa"]
			europa.silenced = maxf(europa.silenced, 5.0)
			dropper.invulnerable = 1e9
			# the Terran governor's war aim, so its pick of a target elsewhere doesn't draw the drop ship off
			w.governors[1].target = europa
			if europa.owner_race != 1 and europa.battle.is_empty() and now - t0 < 120.0:
				return false
			var ai: AIPilot = dropper.get_node("AI")
			_check(europa.owner_race == 1 or not europa.battle.is_empty(), "the computer drop ship landed its troops on Europa (job %s, %d s)" % [ai.job, int(now - t0)])
			step = 99
		99:
			print("invasion test %s" % ("FAILED" if failed else "passed"))
			quit(1 if failed else 0)
	return false
