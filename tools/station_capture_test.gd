# Station capture test: boarding and taking enemy stations.
#  - home starbases and the free port can't be boarded, nor a station
#    with its shields up and its hull whole;
#  - hits thin a station's crew, a breached station blinks open to
#    boarders, and a station left alone musters its crew back;
#  - the player boards a breached enemy mine and takes it: it keeps its
#    world, place, hold and painting, and works for the player's empire;
#  - a frame stays a frame, a defence platform gets new gunners;
#  - a scout's crew is thrown back by a full station crew;
#  - a computer pilot boards a breached station on its own.
#   godot --headless --path . -s tools/station_capture_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var gd: Node                 # the GameData autoload (not an identifier in a tool script)
var enemy := 0
var mine: Ship = null
var mine_body: Planet = null
var credits_before := 0
var guns: Ship = null
var frame: Ship = null
var raider: Ship = null
var prey: Ship = null


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("/root/GameData")
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
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


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel


## Every computer pilot (and gunner) stands down, so nothing else
## shoots at the stations while the test works on them.
func _quiet(w: World) -> void:
	for s in w.ships:
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai != null and s != raider:
			ai.set_physics_process(false)
			for k in s.inputs.keys():
				s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0


## Shoot a station's shields down and its hull to `share`.
func _breach(st: Ship, share: float) -> void:
	st.invulnerable = 0.0
	st.shield = 0.0
	var guard := 0
	while st.hull > st.hull_max * share and guard < 500:
		st.take_damage(st.hull_max * 0.04, main.world.player, st.global_position + Vector2(st.size * 0.5, 0))
		st.shield = 0.0
		guard += 1


## The live station of this design over this world, or null.
func _station_at(w: World, key: String, body: Planet) -> Ship:
	for s in w.ships:
		if s.design_name == key and s.station_body == body and not s.dead:
			return s
	return null


## An uncolonised world with a surface, the nearest to Earth.
func _rock(w: World) -> Planet:
	var best: Planet = null
	var earth: Planet = w.planet_by_name["Earth"]
	for p in w.planets:
		if not p.has_surface() or p.is_colonised() or p.look == "Sun" or Economy.surface_of(p.look, p.habitable) == "gas":
			continue
		if best == null or p.global_position.distance_to(earth.global_position) < best.global_position.distance_to(earth.global_position):
			best = p
	return best


func _process(_delta: float) -> bool:
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	if p == null:
		return false
	var now := w.elapsed
	p.invulnerable = 1e9
	_quiet(w)
	match step:
		0:
			if now < 1.0:
				return false
			enemy = (p.race_id + 1) % 3
			p.refit("Cruiser")
			p.crew = p.crew_max
			# what can't be boarded
			var base := w.starbase_of(enemy)
			_check(Capture.why_not(p, base) == "starbase", "a home starbase can't be boarded: %s" % Capture.reason_text(Capture.why_not(p, base), p, base))
			var port: Ship = null
			for s in w.ships:
				if s.race_id == gd.NEUTRAL and s.is_station:
					port = s
			_check(port != null and Capture.why_not(p, port) == "neutral", "nor the free port: %s" % (Capture.reason_text(Capture.why_not(p, port), p, port) if port != null else "no free port"))
			mine_body = _rock(w)
			mine = w.build_station(enemy, "MiningStation", mine_body, mine_body.global_position + Vector2(mine_body.radius + 300.0, 0), false)
			mine.cargo = {"metals": 40.0, "ice": 12.0}
			_check(Capture.why_not(p, mine) == "intact", "an enemy mine with its shields up can't be boarded: %s" % Capture.reason_text(Capture.why_not(p, mine), p, mine))
			_check(not mine.boardable(), "and isn't open to boarders")
			_breach(mine, 0.45)
			_check(mine.is_breached() and mine.boardable(), "shields down and hull at %d%%: breached" % int(mine.hull / mine.hull_max * 100.0))
			_check(mine.crew < mine.crew_max, "the hits thinned its crew (%.1f of %d)" % [mine.crew, mine.crew_max])
			_check(Capture.why_not(p, mine) == "far", "from Earth it is too far to board")
			_put(p, mine.global_position + Vector2(mine.size + p.size + 80.0, 0), mine.velocity)
			_check(Capture.why_not(p, mine) == "", "alongside, it can be boarded")
			var est := Capture.board_estimate(p, mine)
			_check(est < 30.0, "a Cruiser's %d crew should take it in about %.1f s" % [int(p.crew), est])
			credits_before = p.credits
			p.target = mine
			w.try_board(p)
			_check(p.boarding == mine and mine.boarded_by == p, "boarding the mine")
			t0 = now
			step = 1
		1:
			if p.boarding == mine and now - t0 < 40.0:
				if now - t0 > 1.0 and not has_meta("held"):
					set_meta("held", true)
					_check(mine.shield < 1.0, "the boarders hold the shield room (shield %.0f)" % mine.shield)
					_check(p.global_position.distance_to(mine.global_position) < Capture.board_range(p, mine), "the clamps hold us alongside")
				return false
			_check(mine.dead and mine.captured, "the old mine left without a blast")
			var ns := _station_at(w, "MiningStation", mine_body)
			_check(ns != null and ns.race_id == p.race_id, "the mine over %s is %s now" % [mine_body.body_name, gd.race(ns.race_id)["name"] if ns != null else "gone"])
			if ns == null:
				_finish()
				return false
			_check(ns.hull_race == enemy, "and still wears its %s painting" % gd.race(enemy)["adj"])
			_check(ns.cargo.get("metals", 0.0) >= 39.0 and ns.cargo.get("ice", 0.0) >= 11.0, "its hold came with it: %s" % str(ns.cargo))
			_check(ns.founder == p.pilot_name, "the captor owns it (founder %s)" % ns.founder)
			_check(ns.crew >= 1.0 and ns.crew <= ns.crew_max, "a prize crew of %.1f runs it" % ns.crew)
			_check(ns.global_position.distance_to(mine.global_position) < 50.0, "where the old one stood")
			_check(ns.shield < ns.shield_max * 0.5 and ns.hull < ns.hull_max * 0.5, "still battered")
			_check(p.credits > credits_before, "prize money paid (%d)" % (p.credits - credits_before))
			_check(Capture.why_not(p, ns) == "friendly", "and it's ours: nothing to board")
			# muster: a station left alone fills its crew back
			ns.shield = ns.shield_max
			var c0: float = ns.crew
			t0 = now
			set_meta("muster_from", c0)
			set_meta("muster_ship", ns)
			step = 2
		2:
			var ns: Ship = get_meta("muster_ship")
			ns.shield = ns.shield_max
			if now - t0 < 4.0:
				return false
			_check(ns.crew > float(get_meta("muster_from")) + 0.5, "with its shields up it musters hands (%.1f -> %.1f)" % [float(get_meta("muster_from")), ns.crew])
			# a frame stays a frame
			frame = w.build_station(enemy, "Refinery", mine_body, mine_body.global_position + Vector2(0, mine_body.radius + 600.0))
			frame.cargo = {"metals": 10.0}
			_breach(frame, 0.4)
			p.crew = p.crew_max
			_put(p, frame.global_position + Vector2(frame.size + p.size + 80.0, 0), frame.velocity)
			p.target = frame
			w.try_board(p)
			_check(p.boarding == frame, "boarding a refinery frame")
			t0 = now
			step = 3
		3:
			if p.boarding == frame and now - t0 < 40.0:
				return false
			var nf := _station_at(w, "Refinery", mine_body)
			_check(nf != null and nf.race_id == p.race_id and nf.building, "the frame is ours, still a frame")
			_check(nf != null and nf.cargo.get("metals", 0.0) >= 9.0, "with the materials it had been given")
			# a full crew throws a scout back
			guns = w.build_station(enemy, "DefenseStation", mine_body, mine_body.global_position + Vector2(-(mine_body.radius + 700.0), 0), false)
			guns.shield = 0.0
			guns.hull = guns.hull_max * 0.4
			guns.crew = guns.crew_max
			p.refit("Scout")
			p.crew = p.crew_max
			_put(p, guns.global_position + Vector2(guns.size + p.size + 80.0, 0), guns.velocity)
			_check(Capture.board_estimate(p, guns) == INF, "a scout's crew can't take a full defence platform")
			p.target = guns
			w.try_board(p)
			_check(p.boarding == guns, "but tries")
			t0 = now
			step = 4
		4:
			if p.boarding == guns and now - t0 < 30.0:
				return false
			_check(not guns.dead and guns.race_id == enemy, "and is thrown back")
			_check(p.crew <= p.crew_max * Capture.REPELLED_AT + 0.5, "with a quarter of its crew left (%.1f)" % p.crew)
			# now with a big crew
			p.refit("Battleship")
			p.crew = p.crew_max
			guns.crew = guns.crew_max * 0.6
			guns.shield = 0.0
			_put(p, guns.global_position + Vector2(guns.size + p.size + 80.0, 0), guns.velocity)
			w.try_board(p)
			t0 = now
			step = 5
		5:
			if p.boarding == guns and now - t0 < 60.0:
				return false
			var ng := _station_at(w, "DefenseStation", mine_body)
			_check(ng != null and ng.race_id == p.race_id, "a Battleship's crew takes the defence platform")
			_check(ng != null and ng.get_node_or_null("AI") != null, "and new gunners man it")
			# a computer pilot takes one on its own
			prey = w.build_station(enemy, "SensorStation", mine_body, mine_body.global_position + Vector2(0, -(mine_body.radius + 800.0)), false)
			_breach(prey, 0.4)
			prey.crew = 2.0
			raider = w._make_ship("Raider pilot", p.race_id, "Cruiser", "Raider", false)
			var ai := AIPilot.new()
			ai.name = "AI"
			raider.add_child(ai)
			ai.setup(raider, w, mine_body, 10)
			ai.board_rolls[prey.get_instance_id()] = true
			raider.invulnerable = 1e9
			_put(raider, prey.global_position + Vector2(prey.size + raider.size + 300.0, 0), prey.velocity)
			raider.target = prey
			ai.state = AIPilot.State.ATTACK
			_put(p, w.planet_by_name["Earth"].global_position + Vector2(2000, 0), w.planet_by_name["Earth"].velocity)
			t0 = now
			step = 6
		6:
			if is_instance_valid(prey) and not prey.dead:
				prey.shield = 0.0
			var taken := _station_at(w, "SensorStation", mine_body)
			if (taken == null or taken.race_id != p.race_id) and now - t0 < 60.0:
				return false
			_check(taken != null and taken.race_id == p.race_id, "a computer pilot boarded and took the sensor station (%.1f s)" % (now - t0))
			_finish()
	return false


func _finish() -> void:
	print("station capture test %s" % ("FAILED" if failed else "passed"))
	quit(1 if failed else 0)
	step = 99
