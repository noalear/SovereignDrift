# Economy report (the Resources update, phase 6): a whole Sol round of
# computer empires with the economy, the ground war and the governors on,
# for tuning costs and rates toward 20 to 40 minute rounds.  The player
# sits it out.  Every game minute it prints each empire's worlds, people,
# troops, buildings, stations and stock, and what happened: ground battles,
# worlds taken, convoys, supply ships lost to raiders.
#   godot --headless --fixed-fps 60 --path . -s tools/economy_sim.gd -- --minutes=30
extends SceneTree

var main: Node
var minutes := 30.0
var next_report := 60.0
var battles := {}               # planet -> was fighting last frame
var tally := {"battles": 0, "taken": 0, "convoys": 0, "raided": 0, "boarded": 0, "kits": 0}
var raiders := {}               # what killed supply ships: raid duty, pirate, other warship, colony guns, station
var seen_convoys := {}
var real0 := 0
var gd: Node


func _init() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--minutes="):
			minutes = float(a.get_slice("=", 1))
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("GameData")
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	real0 = Time.get_ticks_msec()
	var w: World = main.world
	w.message.connect(func(t, _c):
		if "deployed a" in t:
			tally["kits"] += 1
			print("  %s: %s" % [Round._fmt(main.world.elapsed), t])
		if "captured" in t.to_lower() and "supply" in t.to_lower():
			print("  " + t))
	w.planet_owner_changed.connect(func(p):
		tally["taken"] += 1
		print("  %s: %s now held by the %s" % [Round._fmt(w.elapsed), p.body_name, gd.race(p.owner_race)["name"] if p.owner_race >= 0 else "nobody"]))
	for s in w.ships:
		_watch(s)
	w.child_entered_tree.connect(func(n):
		if n is Ship:
			_watch(n))


func _watch(s: Ship) -> void:
	if s.died.is_connected(_died):
		return
	s.died.connect(_died.bind(s))


func _died(_victim, killer, s: Ship) -> void:
	if not s.support or s.race_id > 2:
		return
	var kind := "other"
	if killer == null or not is_instance_valid(killer):
		kind = "nothing (sun, rocks)"
	elif killer is Planet:
		kind = "colony guns"
	elif killer is Ship and killer.is_station:
		kind = "station"
	elif killer is Ship and killer.race_id == 3:
		kind = "pirate"
	elif killer is Ship and killer.get_node_or_null("AI") != null and killer.get_node("AI").raid:
		kind = "raid duty"
	elif killer is Ship:
		kind = "warship"
	raiders[kind] = int(raiders.get(kind, 0)) + 1
	var ai: AIPilot = s.get_node_or_null("AI")
	print("  %s: %s supply ship %s lost to %s (%s run, %s)" % [Round._fmt(main.world.elapsed), gd.race(s.race_id)["adj"], s.ship_name, kind,
		ai.job if ai != null else "-", killer.ship_name if killer is Ship else (killer.body_name if killer is Planet else "-")])
	if killer is Ship and killer.race_id != s.race_id:
		tally["raided"] += 1


func _process(_delta: float) -> bool:
	if main == null or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p := w.player
	p.invulnerable = 1e9
	p.global_position = w.sun.global_position + Vector2(0, -gd.UNIVERSE_RADIUS * 0.9)
	p.body.pos = p.global_position
	p.velocity = Vector2.ZERO
	p.body.vel = Vector2.ZERO
	for pl in w.planets:
		var on := not pl.battle.is_empty()
		if on and not battles.get(pl, false):
			tally["battles"] += 1
			print("  %s: ground battle on %s (%s against %s)" % [Round._fmt(w.elapsed), pl.body_name, gd.race(int(pl.battle["race"]))["name"], gd.race(pl.owner_race)["name"]])
		battles[pl] = on
	for g in w.governors.values():
		for h in g.convoys:
			if not seen_convoys.has(h.get_instance_id() * 1000 + int(w.elapsed / 120.0)):
				seen_convoys[h.get_instance_id() * 1000 + int(w.elapsed / 120.0)] = true
				tally["convoys"] += 1
	if w.elapsed >= next_report:
		next_report += 60.0
		_report(w)
	if w.elapsed >= minutes * 60.0 or w.game_over or (w.round_rules != null and w.round_rules.over):
		_report(w)
		print("economy sim done: %s of game time in %d s" % [Round._fmt(w.elapsed), (Time.get_ticks_msec() - real0) / 1000])
		quit(0)
		return true
	return false


func _report(w: World) -> void:
	var line := "t=%s" % Round._fmt(w.elapsed)
	for race in [0, 1, 2, 3]:
		var worlds := 0
		var pop := 0.0
		var troops := 0.0
		var shields := 0
		var bldg := 0
		var stock := 0.0
		for p in w.planets:
			if p.is_colonised() and p.owner_race == race:
				worlds += 1
				pop += p.population
				troops += Ground.strength(p.garrison)
				shields += 1 if p.pshield_up() else 0
				bldg += p.buildings.size()
				for k in p.stock:
					stock += p.stock[k]
		var stations := 0
		for s in w.ships:
			if s.is_station and not s.dead and s.race_id == race and not s.is_starbase():
				stations += 1
		var name: String = gd.race(race)["name"]
		line += " | %s %dw %s %.0ftr %dsh %db %dst %dstk" % [name.substr(0, 4), worlds, gd.fmt_pop(pop), troops, shields, bldg, stations, int(stock)]
	var jobs := 0
	for g in w.governors.values():
		jobs += g.jobs.size()
	var homes := []
	for race in [0, 1, 2]:
		var h: Planet = w._home_planet(race)
		homes.append("%s sh %d fuel %d%s" % [h.body_name, int(h.pshield), int(h.stock.get("fuel", 0.0)), " (theirs)" if h.owner_race != race else ""])
	print("    homes: %s   supply ships lost to: %s" % [", ".join(homes), raiders])
	line += " | battles %d taken %d convoys %d raided %d kits %d jobs %d" % [tally["battles"], tally["taken"], tally["convoys"], tally["raided"], tally["kits"], jobs]
	print(line)
