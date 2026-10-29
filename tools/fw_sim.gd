# First Watch timing: flies the escort as a player would (beside the
# Cormorant, jumping when told, crowding the scout) and prints each stage's
# time, the Cormorant's hull in the raid, the scout's read of her hold.
# Run: godot --headless --path . --fixed-fps 60 -s tools/fw_sim.gd [-- --no-crowd] [-- --kill-raid=N]
extends SceneTree

var main: Node
var args := OS.get_cmdline_user_args()
var crowd := not ("--no-crowd" in args)
var kill_raid := 9999.0
var last_stage := ""
var stage_at := 0.0

func _init() -> void:
	for a in args:
		if a.begins_with("--kill-raid="):
			kill_raid = float(a.split("=")[1])
	call_deferred("_start")

func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at; s.body.pos = at; s.velocity = vel; s.body.vel = vel

func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	Story.interludes = false
	main._start_game(false, true, "story_new")
	await process_frame
	await process_frame
	var w: World = main.world
	w.story.unlock_at = w.elapsed
	await physics_frame
	w.story.start_op()
	var op = w.story.op
	var c: Ship = op.cormorant
	var p: Ship = w.player
	print("player max %.0f warp %.0f; Cormorant max %.0f, port %.0f away" % [p.max_speed, p.warp_speed(), c.max_speed, c.global_position.distance_to(op.port.global_position)])
	c.died.connect(func(s, k): print("   Cormorant died: killer %s, captured %s, hull %.0f, boarders %s, crew %.2f" % [k.ship_name if k is Ship else str(k), s.captured, s.hull, s.get("boarders"), s.crew_fraction()]))
	var t0 := w.elapsed
	var tick := 0
	while w.story.op != null and w.elapsed - t0 < 900.0:
		await physics_frame
		tick += 1
		if w.story.line_left > 0.02:
			w.story.line_left = 0.02
		if not is_instance_valid(c):
			break
		var st: String = op.stage
		if st != last_stage:
			print("%6.1f s  %-12s (+%.1f s)  Cormorant %.0f px/s, %.0f left" % [w.elapsed - t0, st, w.elapsed - stage_at, c.velocity.length(), c.global_position.distance_to(op.port.global_position)])
			last_stage = st
			stage_at = w.elapsed
		var to_port: Vector2 = op.port.global_position - p.global_position
		match st:
			"meet", "leave_earth", "raid":
				_put(p, c.global_position + Vector2(0, 300).rotated(c.rotation), c.velocity)
			"escort", "last_leg":
				if not p.warping:
					_put(p, c.global_position + Vector2(0, 300).rotated(c.rotation), c.velocity)
					p.rotation = to_port.angle()
					if op.jumps_ok and p.warp_block <= 0.0 and tick % 30 == 0:
						p.start_warp()
				elif to_port.length() < p.warp_drop_distance():
					p.end_warp()
			"shadowed":
				var sc: Ship = op.scout
				if crowd and is_instance_valid(sc) and op.scout_mode == "run":
					_put(p, sc.global_position + (c.global_position - sc.global_position).normalized() * 200.0, sc.velocity)
				else:
					_put(p, c.global_position + Vector2(0, -300).rotated(c.rotation), c.velocity)
			"home":
				var b := w.starbase_of(1)
				_put(p, b.global_position + Vector2(300, 0), b.velocity)
		if st == "raid" and tick % 120 == 0:
			print("   raid %.0f s: Cormorant hull %.0f/%.0f, raiders %d" % [w.elapsed - stage_at, c.hull, c.hull_max, op._raiders_left()])
		if st == "raid" and w.elapsed - stage_at > kill_raid:
			for r in op.raiders:
				if is_instance_valid(r) and not r.dead:
					r.take_damage(99999.0, p, r.global_position)
		if st == "last_leg" and tick % 120 == 0:
			print("   last leg: player warping %s energy %.0f block %.1f; Cormorant warping %s autopilot %s %.0f left" % [p.warping, p.energy, p.warp_block, c.warping, c.autopilot, c.global_position.distance_to(op.port.global_position)])
		if st == "shadowed" and tick % 120 == 0:
			var left := Vector2.RIGHT.rotated(c.rotation).orthogonal()
			var sc2: Ship = op.scout
			print("   scout %s, runs %d, read %.0f%%  side %.0f, player %.0f, scout %.0f, brandt %.0f" % [op.scout_mode, op.runs, op.scan * 100.0, op.run_side, signf(left.dot(p.global_position - c.global_position)), signf(left.dot(sc2.global_position - c.global_position)) if is_instance_valid(sc2) else 0.0, signf(left.dot(op.brandt.global_position - c.global_position))])
	print("end %.1f s: op %s failed '%s' flags %s" % [w.elapsed - t0, "running" if w.story.op != null else "over", w.story.failed, w.story.flags])
	quit()
