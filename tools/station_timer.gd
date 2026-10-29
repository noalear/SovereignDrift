# Station timer: how long one stock warship takes to shoot each kind of
# finished station down (Kyle, 2026-10-29: "make the stations a little
# harder to take down").  The attacker sits --range px off the station
# (default 450), nose on, firing every gun, laser and launcher; it is kept
# whole so a Defensive station shooting back does not end the run, but
# its energy and ammo run down as in a real fight.  The report gives the
# seconds from the first shot to the kill, and to the breach (shields at
# 10%, hull at half: the point a boarding party can go in).
#   godot --headless --path . -s tools/station_timer.gd -- --hull=Cruiser
# --stations=DefenseStation,Shipyard picks the stations; --runs=N repeats.
extends SceneTree

const CAP := 60.0 * 240      # frames: a station still up after 4 min counts as "no kill"

var main: Node
var gd: Node
var w
var hull := "Cruiser"
var keys: Array = []
var runs := 1
var gap := 450.0
var idx := 0
var run := 0
var a: Ship = null
var st: Ship = null
var spawned := 0
var breached := -1
var results := {}


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--hull="):
			hull = arg.get_slice("=", 1)
		elif arg.begins_with("--stations="):
			keys = Array(arg.get_slice("=", 1).split(","))
		elif arg.begins_with("--runs="):
			runs = int(arg.get_slice("=", 1))
		elif arg.begins_with("--range="):
			gap = float(arg.get_slice("=", 1))
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("/root/GameData")   # autoloads are not names in a -s script
	if keys.is_empty():
		keys = gd.STATION_ORDER.duplicate()
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	await process_frame
	w = main.world
	print("%-15s %7s %7s %6s %5s" % ["station", "hull", "shield", "regen", "armor"])
	for k in keys:
		var s: Dictionary = gd.stats_for(gd.stock_design(k))
		print("%-15s %7d %7d %6.1f %5.2f" % [k, s["hull_max"], s["shield_max"], s["shield_regen"], s["armor"]])
	_next()


## A world nobody lives on, far from the player, for the station to sit over.
func _site() -> Planet:
	var best: Planet = null
	for p in w.planets:
		if p.look == "Sun" or p.is_colonised():
			continue
		if best == null or p.global_position.distance_to(w.player.global_position) > best.global_position.distance_to(w.player.global_position):
			best = p
	return best


func _next() -> void:
	if run >= runs:
		run = 0
		idx += 1
	if idx >= keys.size():
		_report()
		return
	var p := _site()
	st = w.build_station(2, keys[idx], p, p.global_position + Vector2(p.radius + 500.0, 0.0), false)
	a = w.spawn_ship("Attacker", 1, hull, "Attacker", st.global_position + Vector2(gap, 0.0), st.velocity)
	spawned = Engine.get_physics_frames()
	breached = -1
	run += 1


func _aim() -> void:
	var place: Vector2 = st.global_position + (a.global_position - st.global_position).normalized() * gap
	a.global_position = place
	a.body.pos = place
	a.velocity = st.velocity
	a.body.vel = st.velocity
	a.rotation = (st.global_position - place).angle()
	a.body.angle = a.rotation
	a.body.ang_vel = 0.0
	a.target = st
	a.aim_point = st.global_position
	a.hull = a.hull_max
	a.shield = a.shield_max
	for k in ["fire_primary", "fire_secondary", "fire_missile", "fire_beam"]:
		a.inputs[k] = true


func _process(_delta: float) -> bool:
	if a == null or st == null:
		return false
	var now := Engine.get_physics_frames()
	var over: bool = st.dead or not is_instance_valid(st)
	if not over:
		_aim()
		if breached < 0 and st.is_breached():
			breached = now
		if OS.has_environment("SF_DEBUG") and (now - spawned) % 15 == 0:
			print("  t=%4.1f  hull %5d shield %5d  energy %5d" % [float(now - spawned) / 60.0, st.hull, st.shield, a.energy])
	if over or now - spawned > CAP:
		var kill := float(now - spawned) / 60.0 if over else -1.0
		var br := float(breached - spawned) / 60.0 if breached >= 0 else -1.0
		results.get_or_add(keys[idx], []).append([kill, br])
		print("%s run %d: kill %s, breach %s" % [keys[idx], run, "%.1f s" % kill if kill >= 0.0 else "none", "%.1f s" % br if br >= 0.0 else "none"])
		for s in [a, st]:
			if is_instance_valid(s) and not s.dead:
				s.dead = true
				s.queue_free()
				w.ships.erase(s)
				w.space.remove_body(s.body)
		a = null
		st = null
		_next()
	return false


func _report() -> void:
	print("\n%s against each station at %d px, damage scale %.2f" % [hull, int(gap), gd.DAMAGE_SCALE])
	for k in results:
		var kills := []
		var brs := []
		for r in results[k]:
			if r[0] >= 0.0:
				kills.append(r[0])
			if r[1] >= 0.0:
				brs.append(r[1])
		print("  %-15s kill %s   breach %s" % [k, _avg(kills), _avg(brs)])
	quit(0)


func _avg(t: Array) -> String:
	if t.is_empty():
		return "  none"
	var s := 0.0
	for x in t:
		s += x
	return "%6.1f s" % (s / t.size())
