# Duel timer: two computer pilots in stock Scouts of different empires
# fight in open space, away from the player and the worlds, and the
# report gives how long each fight lasted from the first hit to the
# kill, counting only the time the two were within 700 px (Kyle, 2026-10-28: a Scout duel should take 6-7 s; tune it with
# GameData.DAMAGE_SCALE).
#   godot --headless --path . -s tools/duel_timer.gd -- --runs=3 --hull=Scout
# --standoff: no pilots; the two ships sit --range px apart (default 350),
# nose to nose, and fire every gun, laser and launcher at each other, a
# best case without misses from flying (the floor under a real fight).
extends SceneTree

const PAIRS := [[1, 2], [0, 1], [2, 0], [3, 1], [3, 2], [3, 0]]
const CAP := 60.0 * 45       # frames: a fight still going after 45 s is a draw
const ENGAGED := 700.0       # px: closer than this counts as time in the fight

var main: Node
var gd: Node
var w
var runs := 3
var hull := "Scout"
var pair := 0
var run := 0
var a: Ship = null
var b: Ship = null
var started := -1
var spawned := 0
var results := {}
var standoff := false
var only := -1
var gap := 350.0
var closest := INF
var engaged := 0
var dist_sum := 0.0
var dist_n := 0.0


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--runs="):
			runs = int(arg.get_slice("=", 1))
		elif arg.begins_with("--pair="):
			only = int(arg.get_slice("=", 1))
			pair = only
		elif arg == "--standoff":
			standoff = true
		elif arg.begins_with("--range="):
			gap = float(arg.get_slice("=", 1))
		elif arg.begins_with("--hull="):
			hull = arg.get_slice("=", 1)
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("/root/GameData")   # autoloads are not names in a -s script
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
	_next()


func _next() -> void:
	if run >= runs:
		run = 0
		pair += 1
	if pair >= PAIRS.size() or (only >= 0 and pair != only):
		_report()
		return
	var p: Array = PAIRS[pair]
	# out where nothing else is: well off the player's side of the system
	var at: Vector2 = w.player.global_position + Vector2(9000.0, 0.0).rotated(randf() * TAU)
	var v: Vector2 = w.player.velocity
	var off := Vector2(gap * 0.5 if standoff else 260.0, 0.0).rotated(randf() * TAU)
	a = w.spawn_ship("A", p[0], hull, "A", at + off, v, -1 if standoff else 2)
	b = w.spawn_ship("B", p[1], hull, "B", at - off, v, -1 if standoff else 2)
	if not standoff:
		for pr in [[a, b], [b, a]]:
			var ai = pr[0].get_node("AI")
			ai.ferry = false
			ai.bombard = false
			ai.raid = false
			ai.duel = true         # fight it out, no going home to mend
			pr[0].target = pr[1]
	a.rotation = off.angle() + PI
	b.rotation = off.angle()
	started = -1
	closest = INF
	engaged = 0
	dist_sum = 0.0
	dist_n = 0.0
	spawned = Engine.get_physics_frames()
	run += 1


## Standoff: hold station beside the foe, nose on, everything firing.
func _aim(s: Ship, foe: Ship) -> void:
	var mid: Vector2 = (s.global_position + foe.global_position) * 0.5
	var place: Vector2 = mid + (s.global_position - mid).normalized() * gap * 0.5
	s.global_position = place
	s.body.pos = place
	s.velocity = foe.velocity
	s.body.vel = foe.velocity
	s.rotation = (foe.global_position - place).angle()
	s.body.angle = s.rotation
	s.body.ang_vel = 0.0
	s.target = foe
	s.aim_point = foe.global_position
	for k in ["fire_primary", "fire_secondary", "fire_missile", "fire_beam"]:
		s.inputs[k] = true


func _left() -> String:
	var t := []
	for s in [a, b]:
		t.append("%d%%/%d%%" % [int(100.0 * s.hull / s.hull_max), int(100.0 * s.shield / maxf(s.shield_max, 1.0))] if is_instance_valid(s) and not s.dead else "dead")
		if is_instance_valid(s) and OS.has_environment("SF_DEBUG"):
			var ws := []
			for w in s.weapons:
				ws.append("%s ammo %d" % [w["name"], w["ammo"]])
			print("    %s energy %d  %s  ai %s" % [s.pilot_name, int(s.energy), ", ".join(ws), s.get_node_or_null("AI").get("state") if s.get_node_or_null("AI") else "-"])
	return " v ".join(t)


func _hurt(s: Ship) -> bool:
	return s.hull < s.hull_max or s.shield < s.shield_max - 1.0


func _process(_delta: float) -> bool:
	if a == null or b == null:
		return false
	var now := Engine.get_physics_frames()
	if standoff and not a.dead and not b.dead:
		_aim(a, b)
		_aim(b, a)
		if OS.has_environment("SF_DEBUG") and (now - spawned) % 60 == 0:
			print("  t=%2d  A hull %4d shield %4d energy %4d | B hull %4d shield %4d energy %4d" % [(now - spawned) / 60, a.hull, a.shield, a.energy, b.hull, b.shield, b.energy])
	if started < 0 and (_hurt(a) or _hurt(b)):
		started = now
	if is_instance_valid(a) and is_instance_valid(b) and not a.dead and not b.dead:
		var d := a.global_position.distance_to(b.global_position)
		closest = minf(closest, d)
		if started >= 0 and d < ENGAGED:
			engaged += 1
		dist_sum += d
		dist_n += 1.0
	var over := a.dead or b.dead or not is_instance_valid(a) or not is_instance_valid(b)
	if over or now - spawned > CAP:
		var p: Array = PAIRS[pair]
		var key := "%s v %s" % [gd.race(p[0])["name"], gd.race(p[1])["name"]]
		# time spent in the fight (inside ENGAGED of each other) from the
		# first hit to the kill, so a chase across the system does not count
		var secs := -1.0 if not over or started < 0 else float(engaged) / 60.0
		var winner := "draw"
		if over:
			winner = gd.race(p[1])["name"] if a.dead else gd.race(p[0])["name"]
		results.get_or_add(key, []).append([secs, winner])
		print("%s run %d: %s, won by %s  (hull left %s, closest %d px, mean %d px)" % [key, run, "%.1f s" % secs if secs >= 0.0 else "no kill", winner, _left(), int(closest), int(dist_sum / maxf(dist_n, 1.0))])
		for s in [a, b]:
			if is_instance_valid(s) and not s.dead:
				s.dead = true
				s.queue_free()
				w.ships.erase(s)
				w.space.remove_body(s.body)
		a = null
		b = null
		_next()
	return false


func _report() -> void:
	var all := []
	print("\n%s %s, damage scale %.2f" % [hull, "standoffs at %d px" % int(gap) if standoff else "duels", gd.DAMAGE_SCALE])
	for key in results:
		var t := []
		for r in results[key]:
			if r[0] >= 0.0:
				t.append(r[0])
				all.append(r[0])
		var wins := {}
		for r in results[key]:
			wins[r[1]] = wins.get(r[1], 0) + 1
		print("  %-20s %s  wins %s" % [key, _avg(t), wins])
	print("  overall %s" % _avg(all))
	quit(0)


func _avg(t: Array) -> String:
	if t.is_empty():
		return "no kills"
	var s := 0.0
	for x in t:
		s += x
	return "avg %.1f s (%d kills, %.1f-%.1f)" % [s / t.size(), t.size(), t.min(), t.max()]
