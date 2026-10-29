# Weapon bench: one weapon at a time against a stock warship that does not
# fight back, to compare the weapons part for part (Kyle, 2026-10-29: buff
# the lasers).  The attacker is a stock --hull (default Destroyer) with
# every mount stripped but one nose mount carrying the weapon on test; it
# sits --range px off the target (default 350), nose on, firing it, kept
# whole, with its battery and ammo running down as in a real fight.  The
# target is each empire's computer-pilot fit of --target (armor coat
# included: Arcean reflective, Terran plate, Drengin reactive, pirate
# shroud).  The report gives seconds from the first shot to the kill.
#   godot --headless --path . -s tools/weapon_bench.gd -- --target=Scout
# --weapons=4501,4001 picks the weapons; --races=0,1 the target empires.
extends SceneTree

const CAP := 60.0 * 90       # frames: a target still up after 90 s counts as "no kill"
const WEAPONS := ["4501", "4502", "4001", "4010", "4020", "4009", "4008", "4003", "4018", "4021"]

var main: Node
var gd: Node
var w
var hull := "Destroyer"
var target_hull := "Scout"
var gap := 350.0
var weapons: Array = WEAPONS.duplicate()
var races: Array = [0, 1, 2, 3]
var wi := 0
var ri := 0
var a: Ship = null
var t: Ship = null
var spawned := 0
var results := {}


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--hull="):
			hull = arg.get_slice("=", 1)
		elif arg.begins_with("--target="):
			target_hull = arg.get_slice("=", 1)
		elif arg.begins_with("--range="):
			gap = float(arg.get_slice("=", 1))
		elif arg.begins_with("--weapons="):
			weapons = Array(arg.get_slice("=", 1).split(","))
		elif arg.begins_with("--races="):
			races = []
			for r in arg.get_slice("=", 1).split(","):
				races.append(int(r))
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
	print("%-18s %6s %6s %6s %7s" % ["weapon", "dps", "range", "e/s", "dmg/e"])
	for id in weapons:
		var m: Dictionary = gd.MODULES[id]
		var dps := 0.0
		var cost := 0.0
		var reach := 0.0
		if m["func"] == "beam":
			dps = gd.beam_dps(id)
			cost = 90.0
			reach = gd.beam_range(id)
		else:
			var md: Dictionary = gd.MISSILES[m["p"][3]]
			var rate: float = gd.TUBE_COOLDOWN.get(m["p"][3], 0.3)
			dps = md["warhead"] * gd.DAMAGE_SCALE / rate
			cost = clampf(float(m["p"][1]) * 0.025, 20.0, 350.0) / rate
			reach = md["fire_v"] * gd.LIGHT_SPEED * md["life"] * 0.8
		print("%-18s %6d %6d %6d %7.2f" % [m["name"], dps, reach, cost, dps / maxf(cost, 1.0)])
	_next()


func _next() -> void:
	if ri >= races.size():
		ri = 0
		wi += 1
	if wi >= weapons.size():
		_report()
		return
	# a Terran attacker, a Drengin one against Terran targets
	var side := 2 if races[ri] == 1 else 1
	var design: Dictionary = gd.stock_design(hull, side)
	design["weapons"] = [[weapons[wi], 0, 0, 0, -180, 180]]
	var at: Vector2 = w.player.global_position + Vector2(9000.0, 0.0).rotated(randf() * TAU)
	a = w.spawn_ship("Attacker", side, hull, "Attacker", at + Vector2(gap, 0.0), w.player.velocity)
	a.refit(design)
	t = w.spawn_ship("Target", races[ri], target_hull, "Target", at, w.player.velocity)
	spawned = Engine.get_physics_frames()
	ri += 1


func _aim() -> void:
	var place: Vector2 = t.global_position + (a.global_position - t.global_position).normalized() * gap
	a.global_position = place
	a.body.pos = place
	a.velocity = t.velocity
	a.body.vel = t.velocity
	a.rotation = (t.global_position - place).angle()
	a.body.angle = a.rotation
	a.body.ang_vel = 0.0
	a.target = t
	a.aim_point = t.global_position
	a.hull = a.hull_max
	a.shield = a.shield_max
	for k in ["fire_primary", "fire_secondary", "fire_missile", "fire_beam"]:
		a.inputs[k] = true


func _process(_delta: float) -> bool:
	if a == null or t == null:
		return false
	var now := Engine.get_physics_frames()
	var over: bool = not is_instance_valid(t) or t.dead
	if not over:
		_aim()
		if OS.has_environment("SF_DEBUG") and (now - spawned) % 30 == 0:
			print("  t=%4.1f  hull %5d shield %5d  energy %5d" % [float(now - spawned) / 60.0, t.hull, t.shield, a.energy])
	if over or now - spawned > CAP:
		var kill := float(now - spawned) / 60.0 if over else -1.0
		var key: String = gd.MODULES[weapons[wi]]["name"]
		var race: int = races[(ri - 1) % races.size()]
		results.get_or_add(key, {})[race] = kill
		for s in [a, t]:
			if is_instance_valid(s) and not s.dead:
				s.dead = true
				s.queue_free()
				w.ships.erase(s)
				w.space.remove_body(s.body)
		a = null
		t = null
		_next()
	return false


func _report() -> void:
	var line := "%-18s" % "weapon"
	for r in races:
		line += " %10s" % gd.race(r)["name"]
	print("\n%s (one nose mount) v stock %s at %d px, damage scale %.2f: seconds to kill" % [hull, target_hull, int(gap), gd.DAMAGE_SCALE])
	print(line)
	for k in results:
		line = "%-18s" % k
		for r in races:
			var s: float = results[k].get(r, -1.0)
			line += " %10s" % ("%.1f" % s if s >= 0.0 else "none")
		print(line)
	quit(0)
