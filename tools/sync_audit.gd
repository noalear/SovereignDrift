# Online sync audit: the host and a joiner each write what the player can
# see (every ship, planet and the round) once a second, keyed by the
# host's clock; tools/sync_audit.py then reports every field the joiner
# never caught up on.  The host stirs things up meanwhile: damage, fuel,
# credits, a capture, a landing.  Usage:
#   godot --headless --path . -s tools/sync_audit.gd -- --host
#   godot --headless --path . -s tools/sync_audit.gd -- --join=127.0.0.1
#   python3 tools/sync_audit.py /tmp/sync_host.jsonl /tmp/sync_client.jsonl
# The dumps go to SYNC_OUT (default /tmp/sync_<role>.jsonl).
extends SceneTree

var main: Node
var net: Node
var is_host := "--host" in OS.get_cmdline_user_args()
var out: FileAccess
var next_dump := 0.0
var started_at := 0
var seated_at := -1
var step := 0


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	net = root.get_node("/root/Net")
	var settings = root.get_node("/root/Settings")
	settings.use_profile("online")
	settings.pilot_name = "HostPilot" if is_host else "Joiner"
	var path := OS.get_environment("SYNC_OUT")
	if path == "":
		path = "/tmp/sync_%s.jsonl" % ("host" if is_host else "client")
	out = FileAccess.open(path, FileAccess.WRITE)
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	started_at = Time.get_ticks_msec()


func _joiner(w) -> Node:
	for peer_id in net.seats:
		return net.ships_by_id.get(net.seats[peer_id], null)
	return null


func _process(_delta: float) -> bool:
	var now := Time.get_ticks_msec()
	var w = main.world if main != null else null
	if w == null or (not is_host and (w.player == null or net.local_net_id == 0)):
		return false
	var me: Node = _joiner(w) if is_host else w.player
	if me == null:
		return false
	if seated_at < 0:
		seated_at = now
	var t := now - seated_at
	if is_host:
		_stir(w, me, t)
	else:
		# fly a little so the engines burn
		me.inputs["thrust"] = 1.0 if t < 8000 else 0.0
	if w.elapsed >= next_dump:
		next_dump = floor(w.elapsed) + 1.0
		out.store_line(JSON.stringify(_dump(w, me)))
		out.flush()
	if t > 40000:
		print("[%s] done" % ("host" if is_host else "client"))
		quit(0)
	return false


## Changes the joiner must hear about.
func _stir(w, me: Node, t: int) -> void:
	if step == 0 and t > 10000:
		step = 1
		me.take_damage(me.hull_max * 0.3 + me.shield, null)
		me.fuel = me.fuel_max * 0.4
		me.earn(1234)
		me.crew = me.crew_max * 0.5
	if step == 1 and t > 16000:
		step = 2
		# a capture: the first colony not the joiner's own empire's
		for p in w.planets:
			if p.is_colonised() and p.owner_race != me.race_id and p.owner_race <= 2:
				w.planet_captured(p, me.race_id, me)
				break
		for p in w.planets:
			if p.is_colonised():
				p.under_attack = 8.0
				p.shield = p.shield_max * 0.5
				p.hull = p.hull_max * 0.7
				break
	if step == 2 and t > 22000:
		step = 3
		# park by the Sun's heat band for a few seconds
		me.global_position = w.sun.global_position + Vector2(w.sun.radius * 1.5, 0)
		me.velocity = Vector2.ZERO


func _dump(w, me: Node) -> Dictionary:
	var rd = w.round_rules
	var ships := {}
	for id in net.ships_by_id:
		var s: Node = net.ships_by_id[id]
		if not is_instance_valid(s):
			continue
		ships[str(id)] = {"name": s.pilot_name, "race": s.race_id, "design": s.design_name, "dead": s.dead,
			"hull": s.hull / maxf(s.hull_max, 1.0), "shield": s.shield / maxf(s.shield_max, 1.0), "energy": s.energy / maxf(s.energy_max, 1.0),
			"score": s.score, "kills": s.kills, "deaths": s.deaths, "rank": s.rank, "captures": s.captures,
			"crew": s.crew_fraction(), "disabled": s.is_disabled(), "landing": s.landing, "station": s.is_station, "prize": s.is_prize}
	var mine := {"fuel": me.fuel_fraction(), "refuelling": me.refuelling, "credits": me.credits, "colonists": me.colonists,
		"troops": me.troop_count(), "cargo": me.cargo_total(), "hold": me.hold.size(), "blueprints": me.blueprints.size(),
		"kit": me.kit, "hull": me.hull / maxf(me.hull_max, 1.0), "crew": me.crew_fraction(), "rank": me.rank,
		"sun_heat": 0.0 if me.dead else (w.sun_heat if not is_host else _heat_of(w, me))}
	var planets := []
	for p in w.planets:
		planets.append({"name": p.body_name, "owner": p.owner_race, "design": p.colony_design, "pop": snappedf(p.population, 0.01),
			"silenced": p.silenced > 0.0, "under_attack": p.under_attack > 0.0, "pshield": p.pshield_up(),
			"shield": p.shield / maxf(p.shield_max, 1.0), "hull": p.hull / maxf(p.hull_max, 1.0),
			"garrison": Ground.count(p.garrison), "buildings": p.buildings.size(), "stock": p.stock.duplicate()})
	var rnd := {}
	if rd != null:
		rnd = {"length": rd.length, "phase": rd.phase, "elapsed": rd.elapsed, "hold_race": rd.hold_race, "over": rd.over,
			"standings": rd.standings(), "counts": rd.planet_counts()}
	return {"t": snappedf(w.elapsed, 1.0), "ships": ships, "me": mine, "planets": planets, "round": rnd,
		"planet_counts": w.planet_counts, "ship_count": net.ships_by_id.size()}


## The host keeps the Sun's heat for its own pilot only: the joiner's, for comparison.
func _heat_of(w, s: Node) -> float:
	return w.heat_at(s.global_position)
