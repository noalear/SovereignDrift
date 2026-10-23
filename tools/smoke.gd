# Smoke test: run the game headless for a while with scripted input and
# report errors and counts.  Usage:
#   godot --headless --path . -s tools/smoke.gd
extends SceneTree

var frames := 0
var main: Node
var phase := 0


func _init() -> void:
	print("t=", Time.get_ticks_msec(), " init")
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	main._start_game()
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
	if frames % 1000 == 0:
		var alive := 0
		for s in w.ships:
			if not s.dead:
				alive += 1
		print("t=", Time.get_ticks_msec(), " frame ", frames, " alive=", alive, " missiles=", w.missiles.size(), " pickups=", w.powerups.size(),
			" player hull=", (int(p.hull) if p else -1), " pos=", (p.global_position.round() if p else Vector2.ZERO), " score=", (p.score if p else 0))
	if frames == 6000:
		var kills := 0
		for s in w.ships:
			kills += s.kills
		print("total kills=", kills, " planets per race=", w.planet_counts)
		main._show_menu()
	return false


## Open the shipyard next to the pilot's starbase, buy and fit parts from
## the catalogue and the hold, change hull, launch, and check the result.
func _exercise_shipyard(w: World, p: Ship) -> void:
	var base: Ship = null
	for s in w.ships:
		if s.is_station and s.race_id == p.race_id and not s.dead:
			base = s
	assert(base != null, "no friendly starbase")
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
	var before := b.credits
	b._select("hold", 0)
	b._sell_hold(0)
	assert(b.credits > before, "sell did not pay")
	print("shipyard: problems=", b._problems(), " credits=", b.credits, " bays=", main.get_node("/root/GameData").stats_for(b.current_design())["bay_used"])
	b._launch()
	assert(main.builder == null, "shipyard did not close on launch")
	assert(p.design_name == "Cruiser", "launch did not refit")
	assert(p.design["modules"].has("5503") and p.design["modules"].has("1004"), "parts not fitted")
	print("shipyard: launched ", p.design_name, " modules=", p.design["modules"], " weapons=", p.design["weapons"].size(), " turn=", p.turn_speed, " accel=", p.accel, " overload=", p.overload)
