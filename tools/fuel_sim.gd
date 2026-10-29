# Fuel report: how long a tank lasts.  First a bench: one ship of each
# kind flown flat out from rest in open space, with and without the
# booster, until the tank is dry.  Then a full round of computer pilots for
# N frames (default 20000, about 5.5 minutes), measuring what each hull
# burns per minute in and out of a fight and on the booster.
#   godot --headless --fixed-fps 60 --path . -s tools/fuel_sim.gd -- --bench
#   godot --headless --fixed-fps 60 --path . -s tools/fuel_sim.gd -- --frames=20000
extends SceneTree

const BENCH := ["Scout", "Destroyer", "Cruiser", "Battleship", "Dreadnought"]

var frames := 0
var limit := 20000
var main: Node
var bench := false
var acc := {}          # design -> totals
var prev := {}         # ship id -> fuel last frame
var dry := {}          # ship id -> true once it ran out


func _init() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--frames="):
			limit = int(a.get_slice("=", 1))
		elif a == "--bench":
			bench = true
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	if bench:
		main._start_game(true, false)          # free flight: nobody else about
		await _bench()
		quit(0)
		return
	main._start_game()
	main.world.player.invulnerable = 1e9    # the player sits it out
	print("--- round: %d frames" % limit)


## Flat out from rest to top speed, one ship at a time, far out of every
## well: how long it takes and what share of the tank it costs, plain and
## on the booster, and how many seconds a full tank lasts at full burn.
func _bench() -> void:
	await process_frame
	var w = main.world
	var p: Ship = w.player
	print("--- bench: rest to 97% of top speed in open space (a share of the tank; 'full burn' = seconds of tank at the pedal floored)")
	print("%-12s %6s %7s %9s %8s %9s %8s %10s %10s" % ["hull", "tank", "speed", "plain s", "plain %", "boost s", "boost %", "burn s", "boost burn s"])
	for hull in BENCH:
		p.refit(root.get_node("GameData").stock_design(hull, p.race_id))
		if not p.has_boost:
			p.has_boost = true
		var plain := await _sprint(p, false)
		var boosted := await _sprint(p, true)
		print("%-12s %6d %7d %9.1f %8.1f %9.1f %8.1f %10.0f %10.0f" % [hull, p.fuel_max, p.max_speed, plain["t"], plain["used"], boosted["t"], boosted["used"], plain["full"], boosted["full"]])


func _sprint(p: Ship, boost: bool) -> Dictionary:
	p.fuel = p.fuel_max
	p.body.vel = Vector2.ZERO
	p.velocity = Vector2.ZERO
	var t := 0.0
	var full := 0.0
	var goal := p.max_speed * (p.boost_speed if boost else 1.0) * 0.97
	while t < 120.0:
		# keep it in open space, away from every well and base
		p.body.pos = Vector2(-60000.0, -60000.0)
		p.global_position = p.body.pos
		p.inputs["thrust"] = 1.0
		p.inputs["boost"] = boost
		p.inputs["turn"] = 0.0
		var f0 := p.fuel
		await physics_frame
		t += 1.0 / Engine.physics_ticks_per_second
		if full == 0.0 and f0 > p.fuel:
			full = p.fuel_max / ((f0 - p.fuel) * Engine.physics_ticks_per_second)
		if p.velocity.length() >= goal:
			break
	p.inputs["thrust"] = 0.0
	p.inputs["boost"] = false
	return {"t": t, "used": (p.fuel_max - p.fuel) * 100.0 / p.fuel_max, "full": full}


func _physics_process(delta: float) -> bool:
	if bench or main == null or main.world == null or main.world.player == null:
		return false
	frames += 1
	var w = main.world
	for s in w.ships:
		if s.dead or s.is_station or s == w.player or s.fuel_max <= 0.0:
			continue
		var id: int = s.get_instance_id()
		var a: Dictionary = acc.get(s.design_name, {})
		if a.is_empty():
			a = {"n": {}, "t": 0.0, "burn": 0.0, "fight_t": 0.0, "fight_burn": 0.0, "boost_t": 0.0, "boost_burn": 0.0, "gain": 0.0, "dry": 0, "low": 0.0, "tank": s.fuel_max}
			acc[s.design_name] = a
		a["n"][id] = true
		a["t"] += delta
		var ai = s.get_node_or_null("AI")
		var fight: bool = ai != null and ai.state in [AIPilot.State.ATTACK, AIPilot.State.EVADE]
		if fight:
			a["fight_t"] += delta
		if s.boosting:
			a["boost_t"] += delta
		if s.fuel_fraction() < 0.2:
			a["low"] += delta
		if prev.has(id):
			var d: float = s.fuel - prev[id]
			if d < 0.0:
				# a full tank counts the same however big the hull: burn in tanks
				a["burn"] += -d / s.fuel_max
				if fight:
					a["fight_burn"] += -d / s.fuel_max
				if s.boosting:
					a["boost_burn"] += -d / s.fuel_max
			else:
				a["gain"] += d / s.fuel_max
		prev[id] = s.fuel
		if s.fuel <= 0.0 and not dry.get(id, false):
			dry[id] = true
			a["dry"] += 1
		elif s.fuel > s.fuel_max * 0.05:
			dry[id] = false
	if frames % 3600 == 0 or frames == limit:
		_report()
	if frames >= limit:
		quit(0)
	return false


func _report() -> void:
	print("t=%ds   (tank %%/min is a share of each ship's own tank; 'lasts' = minutes of that use per full tank)" % [frames / 60])
	print("%-14s %4s %10s %8s %10s %8s %8s %8s %6s %6s" % ["hull", "n", "tank%/min", "lasts", "fight%/min", "lasts", "boost%t", "boost%f", "low%t", "dry"])
	var names := acc.keys()
	names.sort()
	var all_t := 0.0
	var all_b := 0.0
	for k in names:
		var a: Dictionary = acc[k]
		var mins: float = a["t"] / 60.0
		var rate: float = a["burn"] * 100.0 / maxf(mins, 0.001)
		var fmins: float = a["fight_t"] / 60.0
		var frate: float = a["fight_burn"] * 100.0 / maxf(fmins, 0.001)
		all_t += mins
		all_b += a["burn"]
		print("%-14s %4d %10.1f %8s %10.1f %8s %8.1f %8.1f %6.1f %6d" % [k, a["n"].size(), rate, _lasts(rate), frate, _lasts(frate),
			a["boost_t"] * 100.0 / maxf(a["t"], 0.001), a["boost_burn"] * 100.0 / maxf(a["burn"], 0.0001), a["low"] * 100.0 / maxf(a["t"], 0.001), a["dry"]])
	var r := all_b * 100.0 / maxf(all_t, 0.001)
	print("all ships: %.1f %% of a tank per minute (lasts %s)" % [r, _lasts(r)])


func _lasts(rate: float) -> String:
	return "%.1f" % (100.0 / rate) if rate > 0.01 else "-"
