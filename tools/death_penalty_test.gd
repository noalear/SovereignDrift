# Death penalty and wreck salvage test (Kyle, 2026-10-31): a pilot shot
# down loses half their credits, which spill with the wreck as a credit
# chip; every wreck leaves one part it carried floating; pilots scoop up
# both.  Practice keeps the purse whole.
#   godot --headless --path . -s tools/death_penalty_test.gd
extends SceneTree

var main: Node
var failed := false
var notes: Array[String] = []


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
	w.message.connect(func(text: String, _c: Color): notes.append(text))
	var p: Ship = w.player
	var home: Planet = w._home_planet(p.race_id)
	var spot: Vector2 = home.global_position + Vector2(0, 6000)
	# the player, 1000 credits aboard, is shot down out in the dark
	_park(p, spot)
	p.credits = 1000
	p.invulnerable = 0.0
	var before: Array = w.powerups.duplicate()
	p.take_damage(1e9, null)
	await physics_frame
	_check(p.dead, "the player is shot down")
	_check(p.credits == 500, "half the purse is lost (1000 -> %d)" % p.credits)
	_check(int(p.get_meta("purse_lost", 0)) == 500, "the death screen knows how much (%d)" % int(p.get_meta("purse_lost", 0)))
	_check(notes.any(func(n): return "You lost 500 credits" in n), "the pilot is told")
	var chip: Powerup = null
	var parts := 0
	for q in w.powerups:
		if before.has(q) or not is_instance_valid(q) or q.global_position.distance_to(spot) > 400.0:
			continue
		if q.kind == "credits":
			chip = q
		elif q.kind == "part":
			parts += 1
	_check(chip != null and chip.amount >= 500, "the wreck spills a credit chip with the lost half (%d)" % (chip.amount if chip != null else 0))
	_check(parts == 1, "and exactly one part it carried (%d)" % parts)
	# relaunch, fly back and scoop the chip up
	var t := 0.0
	while p.dead and t < 10.0:
		await physics_frame
		t += 1.0 / 60.0
	_check(not p.dead, "the player relaunches (%.1f s)" % t)
	_check(not p.has_meta("purse_lost"), "the death-screen note is cleared on relaunch")
	if chip != null and is_instance_valid(chip):
		var was := p.credits
		var amount := chip.amount
		_park(p, chip.global_position)
		for i in 5:
			await physics_frame
			if is_instance_valid(chip):
				_park(p, chip.global_position)
		_check(p.credits == was + amount, "flying through the chip gets the credits back (%d -> %d)" % [was, p.credits])
	# an enemy wreck: one part and a credit chip, half its purse with it
	var foe_race := 2 if p.race_id != 2 else 0
	var foe: Ship = w.spawn_ship("Target", foe_race, "Cruiser", "Target", spot + Vector2(3000, 0), Vector2.ZERO, 3)
	foe.credits = 400
	await physics_frame
	before = w.powerups.duplicate()
	foe.take_damage(1e9, p)
	await physics_frame
	var foe_chip := 0
	parts = 0
	for q in w.powerups:
		if before.has(q) or not is_instance_valid(q):
			continue
		if q.kind == "credits":
			foe_chip = q.amount
		elif q.kind == "part":
			parts += 1
	_check(parts == 1, "an enemy wreck leaves one part (%d)" % parts)
	_check(foe_chip >= 200, "and its credits, half its purse with them (%d)" % foe_chip)
	# a part can be a gun as well as a system
	var guns := 0
	for i in 200:
		var id: String = w.wreck_part(foe)
		if root.get_node("/root/GameData").MODULES[id]["func"] in ["tube", "beam"]:
			guns += 1
	_check(guns > 0 and guns < 200, "the part is any gun or system it carried (%d guns in 200)" % guns)
	var sb: Ship = w.starbase_of(p.race_id)
	var station_only: Array = root.get_node("/root/GameData").STATION_ONLY
	var ok := true
	for i in 100:
		if station_only.has(w.wreck_part(sb)):
			ok = false
	_check(ok, "a starbase never drops its own station fittings")
	# practice keeps the purse whole
	w.practice = true
	_park(p, spot)
	p.credits = 800
	p.invulnerable = 0.0
	p.take_damage(1e9, null)
	await physics_frame
	_check(p.dead and p.credits == 800, "practice keeps the purse (%d)" % p.credits)
	w.practice = false
	print("death penalty test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _park(p: Ship, at: Vector2) -> void:
	p.global_position = at
	p.velocity = Vector2.ZERO
	p.autopilot = false
	if p.body != null:
		p.body.pos = at
		p.body.vel = Vector2.ZERO
	p.reset_physics_interpolation()


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
