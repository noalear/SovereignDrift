# A story raid can never hang on a raider nobody sees (Kyle, 2026-11-05:
# First Watch stuck with no raiders in sight, the Cormorant at the port and
# no marker).  A hurt raider used to fly home to mend and then wander the
# system, still counted.  Checks, on First Watch's raid: the open objective
# marks the nearest raider; a hurt raider stays in the fight (no retreat,
# never runs off); one that runs anyway is driven off and the op moves on;
# a disabled raider is out of the fight; and the raid ends when the
# Cormorant makes the port.
#   godot --headless --path . --fixed-fps 60 -s tools/raid_straggler_test.gd
extends SceneTree

var main: Node
var failed := false
var w: World
var op
var hold_at := Vector2.ZERO
var hold := true


func _init() -> void:
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel
	s.reset_physics_interpolation()


func _frames(n: int) -> void:
	for i in n:
		if w.story.op == null:
			return
		w.story.line_left = minf(w.story.line_left, 0.02)
		w.player.invulnerable = 1e9
		var c: Ship = op.cormorant
		if is_instance_valid(c):
			c.invulnerable = 1e9
			if hold:
				c.autopilot = false
				_put(c, hold_at, Vector2.ZERO)
			# the pilot hangs back: nobody shoots the raiders
			_put(w.player, c.global_position + Vector2(0, 2600), Vector2.ZERO)
		await physics_frame


func _raid_done() -> bool:
	return op.done("raiders") and op.stage == "last_leg"


func _reraid() -> void:
	op.raiders.clear()
	for o in op.objectives:
		if o["id"] == "raiders":
			o["done"] = false
	op.go("raid")
	op._raid()


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	Story.interludes = false
	main._start_game(false, true, "story_new")
	await process_frame
	await process_frame
	w = main.world
	w.story.unlock_at = w.elapsed
	await physics_frame
	w.story.start_op()
	op = w.story.op
	var c: Ship = op.cormorant
	# skip to the raid, far out on the way to the port
	op.complete("meet")
	op.start_dist = maxf(c.global_position.distance_to(op.port.global_position), 1.0)
	hold_at = c.global_position.lerp(op.port.global_position, 0.5)
	await _frames(1)
	if is_instance_valid(op.brandt):
		w.remove_ship(op.brandt)       # nobody fights for her: the raiders live
	op.go("raid")
	op._raid()
	await _frames(5)
	_check(op.raiders.size() >= 3, "the raid comes (%d raiders)" % op.raiders.size())
	var marks: Array = op.marks()
	var raider_mark := false
	for m in marks:
		if m[1] == "Raider" and op.raiders.has(m[3]):
			raider_mark = true
	_check(raider_mark, "the raid objective marks a raider")

	# a hurt raider, the others gone
	var hurt: Ship = op.raiders[0]
	for r in op.raiders:
		if r != hurt:
			r.take_damage(1e6, w.player, r.global_position)
	hurt.shield = 0.0
	hurt.hull = hurt.hull_max * 0.3
	var retreated := false
	var farthest := 0.0
	for k in 40:
		await _frames(30)
		if not op.alive(hurt):
			break
		var ai: AIPilot = hurt.get_node("AI")
		retreated = retreated or ai.state == AIPilot.State.RETREAT
		farthest = maxf(farthest, hurt.global_position.distance_to(c.global_position))
	_check(op.alive(hurt), "the hurt raider is still about after 20 s")
	_check(not retreated, "the hurt raider never turns for home")
	_check(farthest < op.RAID_FLED, "it never runs off (%.0f px at most)" % farthest)
	_check(op.stage == "raid" and op._raiders_left() == 1, "the raid waits for it")
	var on_it := false
	for m in op.marks():
		on_it = on_it or m[3] == hurt
	_check(on_it, "the marker is on the last raider")

	# it runs anyway: driven off, the operation goes on
	var away: Vector2 = (hurt.global_position - c.global_position).normalized()
	if away == Vector2.ZERO:
		away = Vector2.RIGHT
	_put(hurt, c.global_position + away * (op.RAID_FLED + 3000.0), away * 600.0)
	await _frames(3)
	_check(not is_instance_valid(hurt) or not w.ships.has(hurt), "a raider that runs is driven off")
	_check(_raid_done(), "and the raid is over (stage %s)" % op.stage)

	# a disabled raider is out of the fight
	_reraid()
	await _frames(5)
	var last: Ship = op.raiders[0]
	for r in op.raiders:
		if r != last:
			r.take_damage(1e6, w.player, r.global_position)
	last.crew = 0.0
	await _frames(3)
	_check(last.is_disabled() and op.alive(last), "the last raider is disabled, not destroyed")
	_check(_raid_done(), "a disabled raider ends the raid (stage %s)" % op.stage)

	# the Cormorant makes the port mid-raid: they give up the chase
	_reraid()
	await _frames(5)
	hold_at = op.port.global_position + Vector2(200, 0)
	await _frames(3)
	_check(op._raiders_left() == 0, "at the port the raiders break off")
	_check(op.done("raiders"), "the raid objective is done at the port (stage %s)" % op.stage)
	await _frames(3)
	_check(op.stage == "home", "and the Cormorant is in: home next (stage %s)" % op.stage)

	print("raid_straggler_test: %s" % ("FAILED" if failed else "all passed"))
	quit(1 if failed else 0)
