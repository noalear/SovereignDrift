# Smoke test: run the game headless for a while with scripted input and
# report errors and counts.  Usage:
#   godot --headless --path . -s tools/smoke.gd
# After pulling new class_name scripts, run `godot --headless --path . --import` first.
extends SceneTree

var frames := 0
var main: Node
var phase := 0


func _init() -> void:
	print("t=", Time.get_ticks_msec(), " init")
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		# Main.gd failed to compile: usually a stale class cache after new
		# class_name files were pulled.  Fix: godot --headless --path . --import
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	await process_frame
	# a real click on "Play" must start the game (full-screen Controls
	# under a CanvasLayer once stayed at size zero and ate no clicks)
	assert(main.menu.size != Vector2.ZERO, "main menu has no size")
	_click(main.menu.buttons[0][0].get_center())
	await process_frame
	await process_frame
	assert(main.world != null, "clicking Play did not start the game")
	print("menu: Play clicked with the mouse, game started")
	var base: Ship = main.world.starbase_of(main.world.player.race_id)
	var gap: float = main.world.player.global_position.distance_to(base.global_position)
	assert(gap < 600.0, "player did not launch at the starbase (%d)" % gap)
	var pirates: Array = main.world.ships.filter(func(s): return s.race_id == 3 and s.design_name == "Scout")
	assert(pirates.size() == 1 and pirates[0].dead, "expected one pirate raider, benched for a rare visit, found %d" % pirates.size())
	print("t=", Time.get_ticks_msec(), " game started; ships=", main.world.ships.size(), " planets=", main.world.planets.size())


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 6050:
		print("smoke test done")
		quit()
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	if p != null and not p.dead:
		# fly around and shoot at whatever is nearest
		p.inputs["thrust"] = 1.0
		p.inputs["turn"] = sin(frames * 0.02)
		p.inputs["fire_primary"] = frames % 40 < 20
		p.inputs["fire_beam"] = frames % 90 < 30
		p.inputs["fire_mine"] = frames % 300 == 0
		p.inputs["fire_missile"] = frames % 200 == 0
		if frames == 60:
			p.target = w.nearest_enemy_ship(p.global_position, p.race_id, 1e9, p)
			w.cycle_nav(p)
		if frames == 400:
			p.start_warp()
		if frames == 700:
			p.autopilot = true
		if frames == 900:
			p.take_damage(1e6, w.ships[1])
		if frames == 1500:
			w.try_refit(p)
			p.set_cloak(true)
		if frames == 2000:
			_exercise_shipyard(w, p)
		if frames == 2500:
			_exercise_tactical_map(w)
	if frames == 3000:
		_check_starbases(w)
	if frames == 5000:
		_exercise_round(w)
	if frames % 1000 == 0:
		var alive := 0
		for s in w.ships:
			if not s.dead:
				alive += 1
		print("t=", Time.get_ticks_msec(), " frame ", frames, " alive=", alive, " missiles=", w.missiles.size(), " pickups=", w.powerups.size(),
			" player hull=", (int(p.hull) if p else -1), " pos=", (p.global_position.round() if p else Vector2.ZERO), " score=", (p.score if p else 0))
	if frames == 5600:
		assert(main.scoreboard != null, "scoreboard did not open after the round")
		print("round: scoreboard open, winner=", main.scoreboard.result["winner"], " how=", main.scoreboard.result["how"], " pilots=", main.scoreboard.result["pilots"].size(),
			" career rounds=", main.get_node("/root/Settings").career.get("rounds", 0))
		main.scoreboard.play_again.emit()
	if frames == 5700:
		assert(main.world != null and main.scoreboard == null and not main.world.game_over, "play again did not start a fresh round")
		print("round: fresh round started, clock=", main.world.round_rules.clock_text())
	if frames == 6000:
		var kills := 0
		for s in w.ships:
			kills += s.kills
		print("total kills=", kills, " planets per race=", w.planet_counts)
		var boarded := 0
		var prizes := 0
		var disabled := 0
		for s in w.ships:
			boarded += s.ships_captured
			prizes += 1 if s.is_prize and not s.dead else 0
			disabled += 1 if not s.dead and s.is_disabled() else 0
		print("ships captured=", boarded, " prizes flying=", prizes, " disabled now=", disabled)
		main._show_menu()
	return false


## Open the shipyard next to the pilot's starbase, buy and fit parts from
## the catalogue and the hold, change hull, launch, and check the result.
func _exercise_shipyard(w: World, p: Ship) -> void:
	var base: Ship = null
	for s in w.ships:
		if s.is_starbase() and s.race_id == p.race_id and not s.dead:
			base = s                    # the starbase: the governors put up other stations too
	assert(base != null, "no friendly starbase")
	if main.builder != null:
		main.builder._cancel()          # opened earlier by the frame-1500 refit try
		await process_frame
	p.global_position = base.global_position + Vector2(200, 0)
	p.velocity = base.velocity
	p.credits = 9000
	p.hold = ["5503", "4003", "7001"]
	p.score = 30000
	p.kills = 2500
	p.rank = 11
	w.try_refit(p)
	assert(main.builder != null, "shipyard did not open")
	var b: Builder = main.builder
	assert(b.size != Vector2.ZERO, "shipyard has no size")
	b.queue_redraw()
	await process_frame
	await process_frame
	await process_frame
	var hp_btn := -1
	for k in b.buttons.size():
		var cb: Callable = b.buttons[k][1]
		var args := cb.get_bound_arguments()
		if args.size() == 2 and args[0] == "hp" and args[1] == 1:
			hp_btn = k
	assert(hp_btn >= 0, "no hardpoint button drawn")
	_click(b.buttons[hp_btn][0].get_center())
	await process_frame
	assert(b.sel_kind == "hp" and b.sel_index == 1, "clicking a hardpoint did not select it")
	print("shipyard: hardpoint selected with the mouse")
	# a bigger hull, then parts
	b._switch_hull("Cruiser")
	print("shipyard: after hull switch hull=%s status=%s credits=%d" % [b.hull, b.status, b.credits])
	assert(b.hull == "Cruiser", "hull switch failed")
	b._select("bay", b.modules.size())
	b._mount("5503")                 # armour from the hold
	assert(not b.hold.has("5503"), "hold part not used")
	b._select("bay", b.modules.size())
	b._mount("1004")                 # bought power plant
	b._select("hp", 1)
	b._mount("4003")                 # grenade launcher from the hold
	b._select("hp", 0)
	b._remove_selected()
	# sell from the hold with a real click on the Sell button
	var before := b.credits
	var held := b.hold.size()
	assert(held > 0, "nothing in the hold to sell")
	b.queue_redraw()
	await process_frame
	await process_frame
	var sell_btn := -1
	for k in b.buttons.size():
		var cb: Callable = b.buttons[k][1]
		if cb.get_method() == "_sell_hold" and cb.get_bound_arguments() == [0]:
			sell_btn = k
	assert(sell_btn >= 0, "no Sell button drawn")
	_click(b.buttons[sell_btn][0].get_center())
	await process_frame
	assert(b.credits > before and b.hold.size() == held - 1, "clicking Sell did not sell (credits %d -> %d)" % [before, b.credits])
	print("shipyard: sold a hold part with the mouse, credits %d -> %d" % [before, b.credits])
	print("shipyard: problems=", b._problems(), " credits=", b.credits, " bays=", main.get_node("/root/GameData").stats_for(b.current_design())["bay_used"])
	b._launch()
	assert(main.builder == null, "shipyard did not close on launch")
	assert(p.design_name == "Cruiser", "launch did not refit")
	assert(p.design["modules"].has("5503") and p.design["modules"].has("1004"), "parts not fitted")
	print("shipyard: launched ", p.design_name, " modules=", p.design["modules"], " weapons=", p.design["weapons"].size(), " turn=", p.turn_speed, " accel=", p.accel, " overload=", p.overload)


## Open and close the tactical map, set a nav target from it.
func _exercise_tactical_map(w: World) -> void:
	var ev := InputEventAction.new()
	ev.action = "tactical_map"
	ev.pressed = true
	main._unhandled_input(ev)
	assert(main.tactical != null, "tactical map did not open")
	var earth: Planet = w.planet_by_name["Earth"]
	assert(main.tactical.size != Vector2.ZERO, "tactical map has no size")
	_click(main.tactical.to_map(earth.global_position))
	assert(w.player.nav_target == earth, "map click did not set the nav target")
	main._unhandled_input(ev)
	assert(main.tactical == null, "tactical map did not close")
	print("tactical map: open, nav to Earth, closed")


## Run the clock to its last minutes, then hand one empire every colony
## and let the hold timer end the round.
func _exercise_round(w: World) -> void:
	var rd: Round = w.round_rules
	assert(rd != null, "no round rules")
	rd.elapsed = maxf(rd.length - 200.0, 100.0)
	rd.tick(0.1)
	print("round: phase=", rd.phase, " multiplier=", rd.bonus_multiplier(), " clock=", rd.clock_text())
	assert(rd.length <= 0.0 or rd.phase == "final", "final phase did not start")
	var winner: int = w.player.race_id
	for p in w.planets:
		if p.is_colonised() and p.owner_race != winner:
			w.planet_captured(p, winner, w.player)
	rd.hold_timer = Round.HOLD_SECONDS - 0.5
	rd.hold_race = winner
	for i in 20:
		rd.tick(0.05)
	assert(rd.over and rd.result["winner"] == winner, "conquest did not end the round")
	print("round: over, winner=", rd.result["winner"], " standings=", rd.result["standings"])


## A left click pushed through the window like a real mouse.
func _click(at: Vector2) -> void:
	var move := InputEventMouseMotion.new()
	move.position = at
	move.global_position = at
	root.push_input(move, true)
	for pressed in [true, false]:
		var ev := InputEventMouseButton.new()
		ev.button_index = MOUSE_BUTTON_LEFT
		ev.pressed = pressed
		ev.position = at
		ev.global_position = at
		root.push_input(ev, true)


## Starbases keep station by their home world, well outside the Sun's heat,
## and a pilot who strays into the outer heat has seconds to turn away.
func _check_starbases(w: World) -> void:
	var sun: Planet = w.planet_by_name["Sol"]
	var edge: float = sun.radius * (1.0 + w.SUN_HEAT_RADII)
	for s in w.ships:
		if s.is_starbase() and not s.dead:
			var home: Planet = w.planet_by_name[main.get_node("/root/GameData").STARBASES[s.race_id]]
			var dh := s.global_position.distance_to(home.global_position)
			var ds := s.global_position.distance_to(sun.global_position)
			assert(dh < home.radius + 600.0, "%s drifted from its planet (%d)" % [s.pilot_name, dh])
			assert(ds > edge, "%s is in the Sun's heat (%d)" % [s.pilot_name, ds])
	var bases := w.ships.filter(func(s): return s.is_starbase())
	assert(bases.size() == 3, "expected one starbase per empire, found %d" % bases.size())
	print("starbases: on station, clear of the Sun's heat (edge %d), %d in all" % [edge, bases.size()])
	# the free port rides Jupiter's leading Trojan point
	var gd = main.get_node("/root/GameData")
	var ports := w.ships.filter(func(s): return s.race_id == gd.NEUTRAL)
	assert(ports.size() == 1, "expected one free port, found %d" % ports.size())
	var jup: Planet = w.planet_by_name["Jupiter"]
	var port: Ship = ports[0]
	var r_ok := absf((port.global_position - sun.global_position).length() - (jup.global_position - sun.global_position).length()) < 5.0
	var lead := absf(absf(wrapf((port.global_position - sun.global_position).angle() - (jup.global_position - sun.global_position).angle(), -PI, PI)) - PI / 3.0)
	assert(r_ok and lead < 0.01, "the free port is off Jupiter's Trojan point")
	print("free port: at Jupiter's Trojan point, %d units in its market" % int(port.cargo_total()))
