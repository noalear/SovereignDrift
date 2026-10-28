# Subsystem damage, crews and capture test.  Starts a practice duel and:
#  - measures how much hull is left when hits first disable a ship, for
#    hits from random sides and from astern (the balance report);
#  - checks hits kill crew, damage control restarts systems, slower with
#    a thin crew, and an offline engine stops the drive;
#  - boards the disabled pirate (K) and checks the prize flies with you;
#  - has a computer pilot board a disabled ship on its own.
#   godot --headless --path . -s tools/capture_test.gd
extends SceneTree

var frames := 0
var main: Node
var failed := false
var phase := "start"
var phase_at := 0
var prize: Ship = null
var wing: Ship = null
var victim: Ship = null


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
	main._start_practice()


func _pirate(w) -> Ship:
	for s in w.ships:
		if s.race_id == 3 and not s.is_prize:
			return s
	return null


## Hit a fresh copy of a hull until it is disabled; the hull share left.
func _disable_run(w, hull: String, astern: bool, rng: RandomNumberGenerator) -> float:
	var s := Ship.new()
	s.world = null
	w.ships_layer.add_child(s)
	s.setup(w, hull, 3, "Dummy", "Dummy")
	s.set_physics_process(false)
	s.shield = 0.0
	var poly: PackedVector2Array = s.body.hull
	var left := -1.0
	for k in 400:
		var a := PI + rng.randf_range(-0.6, 0.6) if astern else rng.randf() * TAU
		# the point of the hull outline in that direction
		var best := Vector2.ZERO
		var best_dot := -INF
		var dir := Vector2.RIGHT.rotated(a)
		for p in poly:
			if p.dot(dir) > best_dot:
				best_dot = p.dot(dir)
				best = p
		s.take_damage(150.0, null, s.global_position + best.rotated(s.rotation))
		if s.dead:
			if OS.has_environment("SF_DEBUG"):
				var parts := []
				for e in s.systems.list:
					parts.append("%s %d%%" % [e["id"], int(e["hp"] / e["max"] * 100.0)])
				print("  destroyed first: %s crew %.1f  %s" % [hull, s.crew, ", ".join(parts)])
			break
		if s.is_disabled():
			left = s.hull / s.hull_max
			break
	s.queue_free()
	w.ships.erase(s)
	w.space.remove_body(s.body)
	return left


func _process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null:
		return false
	var w = main.world
	var p: Ship = w.player
	if frames == 10:
		var pr := _pirate(w)
		_check(pr != null, "a pirate to practise on")
		var kinds := []
		for e in pr.systems.list:
			kinds.append(e["id"])
		_check("engines" in kinds and "reactor" in kinds and "shields" in kinds and "sensors" in kinds and "w0" in kinds,
			"a Scout has engines, reactor, shields, sensors and weapon mounts (%s)" % [kinds])
		_check(w.starbase_of(p.race_id).systems.list.is_empty(), "starbases have no systems to shoot out")
		_check(pr.crew == GameData.HULL_CREW["Scout"], "a Scout carries its crew (%d)" % pr.crew)
		_balance(w)
		_crew_and_repair(w)
		# park the pirate beside us and shoot its engines out from astern
		pr.get_node("AI").set_physics_process(false)
		pr.global_position = p.global_position + Vector2(400, 0)
		pr.velocity = p.velocity
		pr.rotation = 0.0
		for k in pr.inputs.keys():
			pr.inputs[k] = false if typeof(pr.inputs[k]) == TYPE_BOOL else 0.0
		pr.shield = 0.0
		var stern: Vector2 = pr.systems.get_system("engines")["pts"][0]
		var hits := 0
		while not pr.is_disabled() and hits < 50 and not pr.dead:
			pr.take_damage(150.0, p, pr.global_position + stern.rotated(pr.rotation))
			hits += 1
		_check(pr.is_disabled() and not pr.dead, "shots into the stern disable the pirate before it dies (%d hits, hull %d%%)" % [hits, int(pr.hull / pr.hull_max * 100.0)])
		_check(pr.thrust_force == 0.0, "its engines push nothing")
		_check(pr.crew < pr.crew_max, "the hits killed some of its crew (%.1f of %d)" % [pr.crew, pr.crew_max])
		pr.systems.quiet = -1000.0        # no damage control during the test
		p.target = pr
		p.global_position = pr.global_position + Vector2(-90, 0)
		p.velocity = pr.velocity + Vector2(20, 0)
		victim = pr
		phase = "board"
		phase_at = frames
		var ev := InputEventAction.new()
		ev.action = "board"
		ev.pressed = true
		Input.parse_input_event(ev)
	if phase == "board":
		if frames == phase_at + 3:
			_check(p.boarding == victim, "K locks the tractor on the disabled pirate")
		victim.systems.quiet = -1000.0
		if victim.dead:
			# the captured pirate left quietly; its prize stayed behind
			for s in w.ships:
				if s.is_prize:
					prize = s
			_check(prize != null and prize.race_id == p.race_id and prize.prize_of == p, "the pirate is captured and becomes our prize (%s)" % [prize])
			if prize != null:
				_check(prize.hull_race == 3 and prize.design_name == "Scout", "the prize keeps its pirate hull")
				_check(not prize.is_disabled(), "the prize crew patched it back online")
				_check(prize.crew >= 1.0 and p.crew < p.crew_max, "a prize crew went aboard from ours (%.1f; we keep %.1f)" % [prize.crew, p.crew])
				_check(p.ships_captured == 1, "the capture is on our record")
			phase = "escort"
			phase_at = frames
		elif frames > phase_at + 60 * 25:
			_check(false, "boarding finished within 25 s (progress %.2f, crew %.1f vs %.1f)" % [p.board_progress, p.crew, victim.crew])
			_finish()
	if phase == "escort" and frames == phase_at + 60 * 8:
		var d := prize.global_position.distance_to(p.global_position) if prize != null else INF
		_check(d < 900.0, "the prize flies with us (%d px)" % d)
		_start_ai_boarding(w)
	if phase == "ai":
		victim.systems.quiet = -1000.0
		if victim.dead and victim.captured:
			var got := false
			for s in w.ships:
				if s.is_prize and s.prize_of == wing:
					got = true
			_check(got, "a computer pilot boarded and took the disabled ship")
			_finish()
		elif frames > phase_at + 60 * 40:
			var ai: AIPilot = wing.get_node("AI")
			_check(false, "a computer pilot boards a disabled ship within 40 s (state %d, boarding %s, dist %d)" % [ai.state, wing.boarding, wing.global_position.distance_to(victim.global_position)])
			_finish()
	return false


## The balance report: how much hull is left when a ship is first disabled.
func _balance(w) -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 7
	for hull in ["Scout", "Destroyer", "Cruiser", "Dreadnought"]:
		for astern in [false, true]:
			var sum := 0.0
			var n := 0
			var died := 0
			for k in 20:
				var left := _disable_run(w, hull, astern, rng)
				if left < 0.0:
					died += 1
				else:
					sum += left
					n += 1
			var avg := sum / maxf(n, 1)
			print("balance: %-11s %-7s disabled with %3d%% hull left on average, destroyed first %d/20" % [hull, "astern" if astern else "random", int(avg * 100.0), died])
			_check(died <= 4 and avg > (0.3 if astern else 0.15), "%s from %s: disabled before destroyed" % [hull, "astern" if astern else "all sides"])


func _crew_and_repair(w) -> void:
	var s := Ship.new()
	w.ships_layer.add_child(s)
	s.setup(w, "Destroyer", 2, "Dummy", "Dummy")
	s.set_physics_process(false)
	var e: Dictionary = s.systems.get_system("engines")
	e["hp"] = 0.0
	e["offline"] = true
	s.apply_systems()
	_check(s.is_disabled(), "an offline engine disables the ship")
	var eta_full: float = s.systems.restart_eta(1.0)["eta"]
	var eta_thin: float = s.systems.restart_eta(0.5)["eta"]
	_check(eta_thin > eta_full, "a thinned crew repairs slower (%.0f s against %.0f s)" % [eta_thin, eta_full])
	var t := 0.0
	while s.systems.is_offline("engines") and t < 120.0:
		s.systems.tick(0.1, 0.0, 1.0)
		t += 0.1
	_check(not s.systems.is_offline("engines") and absf(t - eta_full) < 1.0, "damage control restarts the engines in %.1f s (estimate %.1f)" % [t, eta_full])
	s.apply_systems()
	_check(s.thrust_force > 0.0 and s.thrust_force < s.systems.base["thrust_force"], "restarted engines run damaged, below full thrust")
	s.queue_free()
	w.ships.erase(s)
	w.space.remove_body(s.body)


## A Terran computer pilot beside a freshly disabled pirate.
func _start_ai_boarding(w) -> void:
	var p: Ship = w.player
	victim = null
	for k in 600:
		victim = _pirate(w)
		if victim != null and not victim.dead:
			break
	if victim == null or victim.dead:
		# the pirate is still relaunching: make a fresh one
		victim = w._make_ship("Pirate Dummy", 3, "Scout", "Dummy", false)
	if victim.has_node("AI"):
		victim.get_node("AI").set_physics_process(false)
	for k in victim.inputs.keys():
		victim.inputs[k] = false if typeof(victim.inputs[k]) == TYPE_BOOL else 0.0
	victim.global_position = p.global_position + Vector2(0, 2500)
	victim.velocity = p.velocity
	# a plain hull: a pirate's Shroud with its reactor dead hides it from
	# the wingman's sensors, and this checks boarding, not stealth
	victim.refit(root.get_node("GameData").stock_design("Scout", 3))
	var e: Dictionary = victim.systems.get_system("reactor")
	e["hp"] = 0.0
	e["offline"] = true
	victim.apply_systems()
	victim.systems.quiet = -1000.0
	wing = w._make_ship("Wingman", p.race_id, "Destroyer", "Test Wing", false)
	var ai := AIPilot.new()
	ai.name = "AI"
	wing.add_child(ai)
	ai.setup(wing, w, w._home_planet(p.race_id), 6)
	wing.global_position = victim.global_position + Vector2(-700, 300)
	wing.velocity = victim.velocity
	wing.target = victim
	ai.state = AIPilot.State.ATTACK
	ai.board_rolls[victim.get_instance_id()] = true
	phase = "ai"
	phase_at = frames


func _finish() -> void:
	print("capture test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
