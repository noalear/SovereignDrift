# Duel timer: two computer pilots in stock Scouts of different empires
# fight in open space, away from the player and the worlds, and the
# report gives how long each fight lasted from the first hit to the
# kill (Kyle, 2026-10-28: a Scout duel should take 6-7 s; tune it with
# GameData.DAMAGE_SCALE).
#   godot --headless --path . -s tools/duel_timer.gd -- --runs=3 --hull=Scout
extends SceneTree

const PAIRS := [[1, 2], [0, 1], [2, 0], [3, 1], [3, 2], [3, 0]]
const CAP := 60.0 * 45       # frames: a fight still going after 45 s is a draw

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


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--runs="):
			runs = int(arg.get_slice("=", 1))
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
	if pair >= PAIRS.size():
		_report()
		return
	var p: Array = PAIRS[pair]
	# out where nothing else is: well off the player's side of the system
	var at: Vector2 = w.player.global_position + Vector2(9000.0, 0.0).rotated(randf() * TAU)
	var v: Vector2 = w.player.velocity
	var off := Vector2(260.0, 0.0).rotated(randf() * TAU)
	a = w.spawn_ship("A", p[0], hull, "A", at + off, v, 2)
	b = w.spawn_ship("B", p[1], hull, "B", at - off, v, 2)
	a.rotation = off.angle() + PI
	b.rotation = off.angle()
	started = -1
	spawned = Engine.get_physics_frames()
	run += 1


func _hurt(s: Ship) -> bool:
	return s.hull < s.hull_max or s.shield < s.shield_max - 1.0


func _process(_delta: float) -> bool:
	if a == null or b == null:
		return false
	var now := Engine.get_physics_frames()
	if started < 0 and (_hurt(a) or _hurt(b)):
		started = now
	var over := a.dead or b.dead or not is_instance_valid(a) or not is_instance_valid(b)
	if over or now - spawned > CAP:
		var p: Array = PAIRS[pair]
		var key := "%s v %s" % [gd.race(p[0])["name"], gd.race(p[1])["name"]]
		var secs := -1.0 if not over or started < 0 else float(now - started) / 60.0
		var winner := "draw"
		if over:
			winner = gd.race(p[1])["name"] if a.dead else gd.race(p[0])["name"]
		results.get_or_add(key, []).append([secs, winner])
		print("%s run %d: %s, won by %s" % [key, run, "%.1f s" % secs if secs >= 0.0 else "no kill", winner])
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
	print("\n%s duels, damage scale %.2f" % [hull, gd.DAMAGE_SCALE])
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
