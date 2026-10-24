# Battle report: a full round of computer pilots for N frames (default
# 20000, about 5.5 minutes), counting kills, disablings, boardings and
# prizes, for tuning the systems and capture balance.
#   godot --headless --path . -s tools/battle_sim.gd -- --frames=20000
extends SceneTree

var frames := 0
var limit := 20000
var main: Node
var log := {"disabled": 0, "captured": 0, "repelled": 0, "destroyed": 0}
var seen_disabled := {}
var board_frames := 0
var boarding_frames := 0
var boardings := 0
var was_boarding := {}


func _init() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--frames="):
			limit = int(a.get_slice("=", 1))
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
	main.world.message.connect(_on_message)
	for s in main.world.ships:
		s.died.connect(func(v, k):
			if OS.has_environment("SF_DEBUG") and (v.boarded_by != null or v.claimed_by != null):
				print("  DIED while spoken for: %s killed by %s (%s), boarder %s claimer %s, crew %.1f hull %d" % [v.pilot_name, k.name if k != null else "nothing", k.get_class() if k else "", v.boarded_by.pilot_name if v.boarded_by else "-", v.claimed_by.pilot_name if is_instance_valid(v.claimed_by) else "-", v.crew, v.hull]))
	main.world.player.invulnerable = 1e9    # the player sits it out


func _on_message(text: String, _c: Color) -> void:
	if text.contains(" captured "):
		log["captured"] += 1
	elif text.contains(" destroyed "):
		log["destroyed"] += 1


func _process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null:
		return false
	var w = main.world
	for s in w.ships:
		if not s.dead and s.is_disabled() and not seen_disabled.get(s.get_instance_id(), false):
			seen_disabled[s.get_instance_id()] = true
			log["disabled"] += 1
		elif not s.dead and not s.is_disabled():
			seen_disabled[s.get_instance_id()] = false
	for s in w.ships:
		var ai = s.get_node_or_null("AI")
		if ai != null and ai.state == AIPilot.State.BOARD:
			board_frames += 1
			if OS.has_environment("SF_DEBUG") and frames % 30 == 0 and s.target is Ship:
				var v: Ship = s.target
				print("  %s -> %s d=%d rel=%d why=%s dis=%s eta=%s inputs t=%.1f" % [s.pilot_name, v.pilot_name, int(s.global_position.distance_to(v.global_position)), int((s.velocity - v.velocity).length()), Capture.why_not(s, v), v.is_disabled(), v.systems.restart_eta(v.crew_fraction()), s.inputs["thrust"]])
		if s.boarding != null:
			boarding_frames += 1
			if not was_boarding.get(s.get_instance_id(), false):
				boardings += 1
		was_boarding[s.get_instance_id()] = s.boarding != null
	if frames % 3600 == 0 or frames == limit:
		print("   pilots in BOARD state: %.1f s total, tractor locked %.1f s, boardings started %d" % [board_frames / 60.0, boarding_frames / 60.0, boardings])
		var prizes := 0
		for s in w.ships:
			prizes += 1 if s.is_prize and not s.dead else 0
		print("t=%ds  destroyed=%d  disabled=%d  captured=%d  prizes flying=%d  ships=%d" % [frames / 60, log["destroyed"], log["disabled"], log["captured"], prizes, w.ships.size()])
	if frames >= limit:
		quit(0)
	return false
