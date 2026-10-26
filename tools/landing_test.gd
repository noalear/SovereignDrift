# Landing and colonists test: land on a friendly colony, take colonists on,
# lift off into orbit, settle an empty world, invade a knocked-out enemy
# outpost at twenty to one (troops take worlds now: colonists are the weak
# fallback), get beaten back by one whose defences stand, get rounded up
# by a garrison, wipe one out by bombardment, and watch the computer
# pilots ferry colonists themselves.  tools/invasion_test.gd covers troops.
#   godot --headless --path . -s tools/landing_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var ai_touchdowns := 0
var ai_jobs := {}
var luna_before := 0.0
var counts_before := 0
var credits_before := 0
var score_before := 0


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


func _click_play() -> void:
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = (main.menu.buttons[0][0] as Rect2).get_center()
	ev.global_position = ev.position
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


func _on_touchdown(s: Ship, p: Planet) -> void:
	if not s.is_player:
		ai_touchdowns += 1
		var ai: AIPilot = s.get_node_or_null("AI")
		var job: String = ai.job if ai != null else "?"
		ai_jobs[job] = ai_jobs.get(job, 0) + 1
		var gm: Node = root.get_node("GameData")
		print("  AI %s (%s) touched down on %s, job %s, colonists %s" % [s.pilot_name, gm.race(s.race_id)["adj"], p.body_name, job, gm.fmt_pop(s.colonists)])


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _set_ferries(w: World, on: bool) -> void:
	for s in w.ships:
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai == null:
			continue
		if not on:
			ai.set_meta("was_ferry", ai.ferry)
			ai.ferry = false
			ai.job_planet = null
			if ai.state == AIPilot.State.FERRY:
				ai.state = AIPilot.State.PATROL
		elif ai.has_meta("was_ferry"):
			ai.ferry = ai.get_meta("was_ferry")


## Park the player just above a body, moving with it.
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
	var luna: Planet = w.planet_by_name["Luna"]
	var gm: Node = root.get_node("GameData")
	match step:
		0:
			if now < 1.0:
				return false
			# the computer pilots stay off Luna while we test there
			_set_ferries(w, false)
			_park(w, luna)
			luna_before = luna.population
			# the shift-click info card
			_check(w.planet_at(luna.global_position + Vector2(luna.radius, 0), 10.0) == luna, "a click on Luna's edge picks Luna, not Earth")
			var card: Array = luna.info_lines(p.race_id)
			_check(card[0][0] == "Luna" and card.any(func(r): return r[0].begins_with("People")) and card.any(func(r): return r[0].begins_with("Defences")), "Luna's info card has its people and defences")
			_check(p.landing_refusal(luna) == "", "can land on Luna when low and slow (%s)" % p.landing_refusal(luna))
			w.try_land(p)
			_check(p.landing == Ship.Landing.DESCENDING, "B starts the descent")
			t0 = now
			step = 1
		1:
			if now - t0 > Ship.DESCENT_TIME * 0.5 and now - t0 < Ship.DESCENT_TIME * 0.5 + 0.05:
				_check(p.scale.x < 0.9 and p.scale.x > Ship.LANDED_SCALE, "the ship shrinks on the way down (%.2f)" % p.scale.x)
			if now - t0 < Ship.DESCENT_TIME + 0.3:
				return false
			_check(p.is_landed(), "touched down on Luna")
			_check(absf(p.scale.x - Ship.LANDED_SCALE) < 0.01, "landed ship is a speck (%.2f)" % p.scale.x)
			_check(p.global_position.distance_to(luna.global_position) < luna.radius, "the ship sits on Luna's disc")
			_check(p.transfer == 1, "colonists start boarding on a friendly colony")
			_check(w.nearest_enemy_ship(p.global_position, 2, 1e9) != p, "landed ships are not targets")
			t0 = now
			step = 2
		2:
			if now - t0 < 3.6:
				return false
			var spare_left: float = w.spare_colonists(luna)
			_check(p.transfer == 0 and (absf(p.colonists - p.cabin) < 1.0 or spare_left < 5e6), "boarded until the cabin was full or Luna had none to spare: %s of %s" % [gm.fmt_pop(p.colonists), gm.fmt_pop(p.cabin)])
			_check(luna_before - luna.population >= p.colonists - 1e6, "Luna gave them up (%s -> %s)" % [gm.fmt_pop(luna_before), gm.fmt_pop(luna.population)])
			# putting them back where they boarded pays nothing (no farming)
			credits_before = p.credits
			score_before = p.score
			w.colonist_command(p, false)
			t0 = now
			step = 20
		20:
			if now - t0 < 3.6:
				return false
			_check(p.colonists == 0.0 and p.credits == credits_before and p.score == score_before, "unloading where they boarded pays nothing (%d -> %d credits)" % [credits_before, p.credits])
			w.colonist_command(p, true)
			t0 = now
			step = 21
		21:
			if now - t0 < 3.6:
				return false
			_check(p.colonists > 0.0, "boarded again (%s)" % gm.fmt_pop(p.colonists))
			w.try_land(p)
			_check(p.landing == Ship.Landing.ASCENDING, "B lifts off")
			t0 = now
			step = 3
		3:
			if now - t0 < Ship.ASCENT_TIME + 0.2:
				return false
			_check(not p.is_landing() and absf(p.scale.x - 1.0) < 0.001, "back in space at full size")
			_check(p.body.enabled, "solid again")
			var alt := p.global_position.distance_to(luna.global_position) - luna.radius
			_check(alt > p.size, "above the surface (%d px)" % alt)
			var orbit_v: Vector2 = w.space.circular_orbit_velocity(luna.body, p.global_position)
			_check(p.velocity.distance_to(orbit_v) < 60.0, "lifted off into a parking orbit (off by %d px/s)" % p.velocity.distance_to(orbit_v))
			# an empty world: Ceres
			var ceres: Planet = w.planet_by_name["Ceres"]
			counts_before = w.planet_counts.get(p.race_id, 0)
			_park(w, ceres)
			w.try_land(p)
			t0 = now
			step = 4
		4:
			if now - t0 < Ship.DESCENT_TIME + 0.3:
				return false
			var ceres: Planet = w.planet_by_name["Ceres"]
			_check(ceres.is_colonised() and ceres.owner_race == p.race_id, "landing colonists on empty Ceres settles it")
			_check(p.colonists == 0.0 and ceres.population > 30e6, "the settlers went ashore (%s)" % gm.fmt_pop(ceres.population))
			_check(ceres.colony_design == Planet.design_for(ceres.population), "the new colony's defences fit its size (%s)" % ceres.colony_design)
			_check(w.planet_counts.get(p.race_id, 0) == counts_before + 1, "Ceres counts for the empire (%d -> %d)" % [counts_before, w.planet_counts.get(p.race_id, 0)])
			w.try_land(p)
			t0 = now
			step = 5
		5:
			if now - t0 < Ship.ASCENT_TIME + 0.2:
				return false
			var jupiter: Planet = w.planet_by_name["Jupiter"]
			_park(w, jupiter)
			_check("gas giant" in p.landing_refusal(jupiter), "no landing on a gas giant (%s)" % p.landing_refusal(jupiter))
			var mars: Planet = w.planet_by_name["Mars"]
			_park(w, mars, 300.0)
			p.velocity = mars.velocity + Vector2(400, 0)
			_check("Too fast" in p.landing_refusal(mars), "too fast to land says brake (%s)" % p.landing_refusal(mars))
			# invasion: knock Phobos out, then land twenty times its people
			var phobos: Planet = w.planet_by_name["Phobos"]
			_check(Ground.count(phobos.garrison) == 0 and not phobos.pshield_up(), "Phobos has no troops and no planetary shield")
			phobos.population = p.cabin / (gm.COLONIST_RATIO * 1.25)
			phobos.set_meta("before", phobos.population)
			phobos.take_damage(1e6, p)
			_check(phobos.is_defenceless() and phobos.silenced > 0.0, "a colony whose hull falls is knocked out, not captured")
			_check(phobos.owner_race == 2, "it still belongs to the Drengin")
			p.colonists = p.cabin
			_park(w, phobos, 40.0)
			w.try_land(p)
			t0 = now
			step = 6
		6:
			var phobos: Planet = w.planet_by_name["Phobos"]
			if phobos.owner_race == 2:
				phobos.set_meta("before", phobos.population)   # passing ships may bomb it on the way down
			if now - t0 < Ship.DESCENT_TIME + 0.3:
				return false
			_check(phobos.owner_race == p.race_id, "the invasion took Phobos (owner %d)" % phobos.owner_race)
			var held: float = p.cabin - float(phobos.get_meta("before")) * gm.COLONIST_RATIO
			_check(absf(phobos.population - held) < 5e6, "survivors hold it (%s, expected %s)" % [gm.fmt_pop(phobos.population), gm.fmt_pop(held)])
			w.try_land(p)
			t0 = now
			step = 7
		7:
			if now - t0 < Ship.ASCENT_TIME + 0.2:
				return false
			# an invasion against standing defences on even terms fails
			var deimos: Planet = w.planet_by_name["Deimos"]
			deimos.population = p.cabin * 2.0
			p.colonists = p.cabin
			_park(w, deimos, 40.0)
			w.try_land(p)
			t0 = now
			step = 8
		8:
			if now - t0 < Ship.DESCENT_TIME + 0.3:
				return false
			var deimos: Planet = w.planet_by_name["Deimos"]
			var left: float = p.cabin * 2.0 - p.cabin / (gm.COLONIST_RATIO * gm.OFFENSIVE_FACTOR)
			_check(deimos.owner_race == 2 and absf(deimos.population - left) < 2e6, "beaten back by a colony with its defences up (%s left)" % gm.fmt_pop(deimos.population))
			_check(p.colonists == 0.0, "the landing force was lost")
			# a garrison rounds colonists up, however many they are
			deimos.take_damage(1e6, p)
			deimos.garrison = {"infantry": 1.0}
			deimos.population = 1.5e6
			p.colonists = p.cabin
			w.deliver_colonists(p, deimos)
			_check(deimos.owner_race == 2 and p.colonists == 0.0 and deimos.population == 1.5e6, "colonists can't take a world with troops on it")
			deimos.garrison = {}
			# bombardment after the defences fall kills people, down to nobody
			deimos.take_damage(1e6, p)
			deimos.population = 1.5e6
			deimos.take_damage(1000.0, p)
			_check(not deimos.is_colonised(), "bombarding a knocked-out colony to nothing empties the world")
			w.try_land(p)
			t0 = now
			step = 9
			_set_ferries(w, true)
			print("watching the computer pilots for 150 s of game time...")
		9:
			if now - t0 < 150.0:
				return false
			print("AI touchdowns: %d, by job %s" % [ai_touchdowns, ai_jobs])
			_check(ai_touchdowns >= 2, "computer pilots land to ferry colonists")
			var carrying := 0
			for s in w.ships:
				if not s.is_player and s.colonists > 0.0:
					carrying += 1
			print("AI ships carrying colonists now: %d" % carrying)
			var rd: Round = w.round_rules
			print("round events: ", rd.events.map(func(e): return e[1]))
			print("planets per race: ", w.planet_counts)
			print("landing test %s" % ("FAILED" if failed else "passed"))
			quit(1 if failed else 0)
	return false
