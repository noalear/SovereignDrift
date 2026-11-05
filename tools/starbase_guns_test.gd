# Starbase guns test (Kyle, 2026-10-31: "My starbase should have its own
# turrets for defense and it should be fairly powerful"): the starbase's
# four defence turrets and its other guns each take their own mark and
# shoot down a raid on their own; they hold fire with the station's pilot
# switched off (a story siege), and leave a ship a friend is boarding be.
#   godot --headless --path . -s tools/starbase_guns_test.gd
extends SceneTree

var main: Node
var failed := false


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
	main._start_game()
	await process_frame
	await process_frame
	var w = main.world
	var p: Ship = w.player
	var base: Ship = w.starbase_of(p.race_id)
	var foe_race := 2 if p.race_id != 2 else 0
	for s in w.ships.duplicate():
		if s != p and not s.is_starbase():
			w.remove_ship(s)
	p.invulnerable = 1e9
	var turrets := 0
	for wpn in base.weapons:
		if wpn["id"] == "4010" and base.is_turret(wpn):
			turrets += 1
	_check(turrets == 4, "the starbase carries four defence turrets (%d)" % turrets)
	# a raid of two Cruisers
	var raid: Array = []
	for i in 2:
		var a := PI * i + 0.4
		var at: Vector2 = base.global_position + Vector2(cos(a), sin(a)) * 1300.0
		var s: Ship = w.spawn_ship("Raider", foe_race, "Cruiser", "Raider", at, base.velocity, 6)
		s.target = base
		s.get_node("AI").state = AIPilot.State.ATTACK
		raid.append(s)
	var fired := {}
	var last := {}
	var t := 0.0
	while t < 40.0 and raid.any(func(s): return is_instance_valid(s) and not s.dead):
		await physics_frame
		t += 1.0 / 60.0
		_hold(p, base)
		_count(base, fired, last)
	_check(raid.all(func(s): return not is_instance_valid(s) or s.dead), "the raid is shot down (%.1f s)" % t)
	_check(not base.dead and base.hull > base.hull_max * 0.8, "the starbase stands (hull %d%%)" % int(base.hull / base.hull_max * 100.0))
	var turret_shots := 0
	var turrets_firing := 0
	for i in base.weapons.size():
		if base.weapons[i]["id"] == "4010":
			turret_shots += fired.get(i, 0)
			turrets_firing += 1 if fired.get(i, 0) > 0 else 0
	_check(turrets_firing >= 3, "the defence turrets did the shooting (%d of 4 fired, %d shots)" % [turrets_firing, turret_shots])
	_check(fired.get(2, 0) > 0, "and the station battery (%d shots)" % fired.get(2, 0))
	# its pilot switched off (a story siege pins the guns): no fire
	var ai: AIPilot = null
	for c in base.get_children():
		if c is AIPilot:
			ai = c
	ai.set_physics_process(false)
	for k in ["fire_primary", "fire_secondary", "fire_missile", "fire_mine", "fire_beam"]:
		base.inputs[k] = false
	var quiet: Ship = w.spawn_ship("Quiet", foe_race, "Cruiser", "Quiet", base.global_position + Vector2(700, 0), base.velocity, -1)
	fired.clear()
	last.clear()
	_count(base, {}, last)       # where each cooldown stands now
	for i in 90:
		await physics_frame
		_hold(p, base)
		quiet.global_position = base.global_position + Vector2(700, 0)
		quiet.body.pos = quiet.global_position
		_count(base, fired, last)
	var any := 0
	for k in fired:
		any += fired[k]
	_check(any == 0, "with its pilot off the guns hold fire (%d shots: %s)" % [any, fired])
	ai.set_physics_process(true)
	# a ship a friend is boarding is left be
	quiet.boarded_by = p
	p.boarding = quiet
	base.pick_gun_marks()
	_check(base.weapons.all(func(wpn): return wpn.get("mark") != quiet), "no gun marks a ship a friend is boarding")
	quiet.boarded_by = null
	p.boarding = null
	base.pick_gun_marks()
	_check(base.weapons.any(func(wpn): return wpn.get("mark") == quiet), "...and they mark it again once the boarding is off")
	print("starbase guns test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


## The player out of the way, far off and unharmed.
func _hold(p: Ship, base: Ship) -> void:
	p.global_position = base.global_position + Vector2(30000, 0)
	p.body.pos = p.global_position
	p.velocity = base.velocity
	p.body.vel = base.velocity


## Shots per gun: a tube's cooldown jumps up each time it fires.
func _count(base: Ship, fired: Dictionary, last: Dictionary) -> void:
	for i in base.weapons.size():
		var cd: float = base.weapons[i]["cooldown"]
		if cd > float(last.get(i, 0.0)) + 0.001:
			fired[i] = fired.get(i, 0) + 1
		last[i] = cd


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
