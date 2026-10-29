# The story campaign in co-op: two headless processes, one hosting its
# story and one joining.  Usage (start the joiner a few seconds after):
#   godot --headless --path . -s tools/story_net_test.gd -- --host-story
#   godot --headless --path . -s tools/story_net_test.gd -- --join=127.0.0.1
# The joiner builds the story's bigger Sol, takes a Terran seat (never a
# story ship), sees the briefing come ready, the operation, its objectives
# and marks, and hears the host's lines.  The joiner reaching the
# Cormorant completes the host's objective; the joiner's answer to a
# choice is a vote and the host decides; a failed operation's retry takes
# everyone back to the briefing in a fresh world.
# Exit code 1 when a check fails.
extends SceneTree

var main: Node
var net: Node
var failed := false
var args := OS.get_cmdline_user_args()
var is_host := "--host-story" in args
var joiner_seen_at := -1
var world_seen_at := -1
var step := 0
var step_at := 0
var worlds: Array = []            # each world this process built, in order
# what the joiner saw
var saw_mirror := false
var saw_scale := 0.0
var saw_ready := false
var saw_op := ""
var saw_objectives := 0
var saw_mark_on_cormorant := false
var heard: Array = []
var saw_meet_done := false
var voted := false
var saw_choice_panel := false
var saw_choice_closed := false
var saw_flag := ""
var saw_failed := ""
var saw_retry_ready := false
var quit_at := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	var settings := root.get_node("/root/Settings")
	if is_host:
		# a fresh story each run
		SaveGame.erase(SaveGame.STORY_PATH)
		settings.use_profile("single")
	else:
		settings.use_profile("online")
	settings.pilot_name = "HostPilot" if is_host else "Joiner"
	settings.race_id = 1
	net = root.get_node("/root/Net")
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host-story / --join itself


func _tag() -> String:
	return "host" if is_host else "client"


func _find(w, pilot: String) -> Ship:
	for s in w.ships:
		if s.pilot_name == pilot:
			return s
	return null


func _physics_process(_delta: float) -> bool:
	var w = main.world if main != null else null
	var now := Time.get_ticks_msec()
	if w != null and not worlds.has(w):
		worlds.append(w)
		if world_seen_at < 0:
			world_seen_at = now
		w.message.connect(func(t, _c): print("[%s] msg: %s" % [_tag(), t]))
		if w.story != null:
			w.story.line_started.connect(func(id): heard.append(id))
	if is_host:
		_host_step(w, now)
	else:
		_client_step(w, now)
	var done: bool = (step >= 9) or (world_seen_at > 0 and now - world_seen_at > 150000) if is_host \
		else ((world_seen_at > 0 and now - world_seen_at > 140000) or (quit_at > 0 and now > quit_at))
	if done:
		_finish(w)
		quit(1 if failed else 0)
	return false


func _go(n: int, now: int) -> void:
	step = n
	step_at = now


func _host_step(w, now: int) -> void:
	if w == null or w.story == null:
		return
	var st: Story = w.story
	var j := _find(w, "Joiner")
	if step == 0:
		if j == null or not net.seats.values().has(j.net_id):
			return
		joiner_seen_at = now
		_check(w.map_scale == Story.MAP_SCALE and net.mode == "story", "the host runs the story online")
		_check(j.race_id == 1 and not j.has_meta("story"), "the joiner flies a Terran ship that is not the story's")
		var op := StoryOp.new()
		op.story = st
		op.world = w
		_check(op.pilots().size() == 2 and op.scaled(4) == 6, "two pilots: half again the enemies (%d)" % op.scaled(4))
		st.unlock_at = w.elapsed
		_go(1, now)
	elif step == 1 and now - step_at > 4000:
		st.start_op()
		_check(st.op != null and st.op.spec["id"] == "first_watch", "the host launched First Watch")
		_go(2, now)
	elif step == 2 and now - step_at > 4000:
		# the joiner flies to the Cormorant: it counts for the squad
		var c: Ship = st.op.cormorant
		var at: Vector2 = c.global_position + Vector2(300, 0)
		j.global_position = at
		j.body.pos = at
		j.velocity = c.velocity
		j.body.vel = c.velocity
		_go(3, now)
	elif step == 3 and now - step_at > 3000:
		_check(st.op != null and st.op.done("meet"), "the joiner meeting the Cormorant completes the host's objective")
		st.ask("test", "Which way?", [["a", "The host's way", "One."], ["b", "The joiner's way", "Two."]])
		_go(4, now)
	elif step == 4:
		if st.votes.get("Joiner", "") == "b":
			print("[host] the joiner voted b")
			st.choose("a")
			_go(5, now)
		elif now - step_at > 45000:
			_check(false, "the joiner's vote reached the host")
			st.choose("a")
			_go(5, now)
	elif step == 5 and now - step_at > 4000:
		_check(st.flags.get("choice_test", "") == "a", "the host's answer stands")
		st.op.fail("A test failure.")
		_go(6, now)
	elif step == 6 and now - step_at > 4000:
		_check(st.failed != "", "the operation failed")
		main.retry_operation()
		_go(7, now)
	elif step == 7 and ((j != null and net.seats.values().has(j.net_id)) or now - step_at > 40000):
		_check(worlds.size() == 2, "the retry built a fresh world (%d)" % worlds.size())
		_check(st.briefing_ready() and st.op == null, "the retry is back at the briefing")
		_check(j != null and net.seats.values().has(j.net_id), "the joiner came back into the retry")
		_go(8, now)
	elif step == 8:
		if net.seats.is_empty():
			_go(9, now)
		elif now - step_at > 40000:
			_check(false, "the joiner left")
			_go(9, now)


func _client_step(w, now: int) -> void:
	if w == null or w.player == null or w.story == null:
		return
	var st: Story = w.story
	saw_mirror = st.mirror
	saw_scale = w.map_scale
	if worlds.size() == 1:
		saw_ready = saw_ready or st.briefing_ready()
		if st.op_title() != "":
			saw_op = st.op_title()
			saw_objectives = maxi(saw_objectives, st.objectives().size())
			for o in st.objectives():
				if o["text"].begins_with("Meet the Cormorant") and o["done"]:
					saw_meet_done = true
			var cormorant: Ship = null
			for s in w.ships:
				if s.ship_name == "TSS Cormorant":
					cormorant = s
			for m in st.marks():
				if cormorant != null and m[0].distance_to(cormorant.global_position) < 80.0:
					saw_mark_on_cormorant = true
		if main.story_choice != null:
			saw_choice_panel = true
		if main.story_choice != null and not voted:
			voted = true
			main.story_choice.chosen.emit("b")     # the joiner's pick: a vote
		if voted and st.choice.is_empty() and main.story_choice == null:
			saw_choice_closed = true
		if st.flags.has("choice_test"):
			saw_flag = st.flags["choice_test"]
		if st.failed != "":
			saw_failed = st.failed
	elif st.briefing_ready() and not saw_retry_ready:
		saw_retry_ready = true
		quit_at = now + 3000


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok" if ok else "FAILED", what])
	if not ok:
		failed = true


func _finish(_w) -> void:
	if not is_host:
		_check(saw_mirror and saw_scale == Story.MAP_SCALE, "the joiner's world is the story's (mirror, scale %.1f)" % saw_scale)
		_check(saw_ready, "the joiner saw the briefing come ready")
		_check(saw_op == "First Watch", "the joiner saw the operation (%s)" % saw_op)
		_check(saw_objectives >= 1, "the joiner saw its objectives (%d)" % saw_objectives)
		_check(saw_mark_on_cormorant, "the joiner's mark rides on the Cormorant")
		_check(heard.has("fw_start"), "the joiner heard the host's lines (%s)" % ", ".join(heard.slice(0, 6)))
		_check(saw_meet_done, "the joiner saw the meet objective done")
		_check(saw_choice_panel, "the joiner got the choice")
		_check(saw_choice_closed and saw_flag == "a", "the host's answer closed it (%s)" % saw_flag)
		_check(saw_failed != "", "the joiner saw the operation fail")
		_check(worlds.size() == 2 and saw_retry_ready, "the retry took the joiner back to the briefing (%d worlds)" % worlds.size())
	print("[%s] %s" % [_tag(), "STORY NET TEST FAILED" if failed else "STORY NET TEST PASSED"])
