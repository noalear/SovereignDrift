# Online Survival and Skirmish test: two headless processes, one hosting
# and one joining.  Usage (start the joiner a few seconds after the host):
#   godot --headless --path . -s tools/modes_net_test.gd -- --host-survival
#   godot --headless --path . -s tools/modes_net_test.gd -- --join=127.0.0.1
# and the same with --host-skirmish.
# Survival: the joiner flies for the host's empire, sees the waves come and
# go, and its kills count on the board.  Skirmish: the joiner takes a
# computer pilot's seat on the other side, picks a Scout while waiting to
# launch and relaunches in it; when it leaves a computer pilot comes back.
# Exit code 1 when a check fails.
extends SceneTree

var frames := 0
var main: Node
var settings: Node
var failed := false
var args := OS.get_cmdline_user_args()
var is_host := "--host-survival" in args or "--host-skirmish" in args
var which := "survival" if "--host-survival" in args else ("skirmish" if "--host-skirmish" in args else "")
var joiner_seen_at := -1
var world_seen_at := -1
var left_at := -1
var step := 0
var t0 := 0
# what the joiner saw
var saw_mode := ""
var saw_wave := 0
var saw_break_after := false
var saw_raider_enemy := false
var saw_kills := 0
var saw_side := -1
var saw_score := 0
var died_at := -1
var picked := false
var relaunched_as := ""
var teams_ok := false
var quit_at := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	settings = root.get_node("/root/Settings")
	settings.use_profile("online")
	settings.pilot_name = "HostPilot" if is_host else "Joiner"
	settings.race_id = 1
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host-survival / --host-skirmish / --join itself


func _tag() -> String:
	return ("host-" + which) if is_host else "client"


func _find(w, pilot: String) -> Ship:
	for s in w.ships:
		if s.pilot_name == pilot:
			return s
	return null


func _physics_process(_delta: float) -> bool:
	frames += 1
	var w = main.world if main != null else null
	var now := Time.get_ticks_msec()
	if w != null and world_seen_at < 0:
		world_seen_at = now
		w.message.connect(func(t, _c): print("[%s] msg: %s" % [_tag(), t]))
	if is_host:
		_host_step(w, now)
	else:
		_client_step(w, now)
	var done: bool = (left_at > 0 and now - left_at > 4000) or (world_seen_at > 0 and now - world_seen_at > 70000) if is_host \
		else ((world_seen_at > 0 and now - world_seen_at > 40000) or (quit_at > 0 and now > quit_at))
	if done:
		_finish(w)
		quit(1 if failed else 0)
	return false


func _host_step(w, now: int) -> void:
	if w == null or w.mode_rules == null:
		return
	var p: Ship = w.player
	var j := _find(w, "Joiner")
	if j != null and joiner_seen_at < 0:
		joiner_seen_at = now
		if which == "survival":
			var sv: Survival = w.mode_rules
			_check(j.race_id == sv.race and sv.pilots().has(j) and j.team() == p.team(), "the joiner flies for the defenders")
			_check(j.global_position.distance_to(sv.base().global_position) < 700.0, "the joiner launches at the starbase")
		else:
			var sk: Skirmish = w.mode_rules
			_check(sk.side_of(j) == 1 and j.team() != p.team(), "the joiner goes to the side with nobody on it")
			_check(sk.roster(0).size() == Skirmish.TEAM_SIZE and sk.roster(1).size() == Skirmish.TEAM_SIZE, "a computer pilot made room (%d and %d)" % [sk.roster(0).size(), sk.roster(1).size()])
	if joiner_seen_at < 0:
		return
	var t := now - joiner_seen_at
	if which == "survival":
		var sv: Survival = w.mode_rules
		if step == 0 and t > 2000:
			step = 1
			sv.break_left = 0.05
		elif step == 1 and t > 5000:
			step = 2
			var raiders := sv.raiders()
			_check(raiders.size() == Survival.roster(1, 2).size(), "wave 1 has more raiders for two pilots (%d)" % raiders.size())
			for s in raiders:
				s.invulnerable = 0.0
				s.take_damage(1e6, j)
			_check(sv.run_kills(j) == raiders.size(), "the joiner's kills count (%d)" % sv.run_kills(j))
		elif step == 2 and t > 9000:
			step = 3
			_check(sv.break_left > 0.0 and sv.wave == 1, "the wave is beaten and the break is on")
	else:
		var sk: Skirmish = w.mode_rules
		if step == 0 and t > 2000:
			step = 1
			var before: int = sk.score[0]
			j.invulnerable = 0.0
			j.take_damage(1e6, p)
			_check(sk.score[0] == before + 1, "the host's kill on the joiner counts for side 0")
		elif step == 1 and t > 12000 and not j.dead:
			step = 2
			_check(j.design_name == "Scout" and sk.picks.get(j, "") == "Scout", "the joiner relaunched in the Scout it picked (%s)" % j.design_name)
			step = 3
	if j == null and step >= 3 and left_at < 0:
		left_at = now
	if left_at > 0 and now - left_at > 2000 and step == 3:
		step = 4
		if which == "skirmish":
			var sk: Skirmish = w.mode_rules
			_check(sk.roster(1).size() == Skirmish.TEAM_SIZE and sk.roster(1).all(func(s): return sk.is_bot(s)), "a computer pilot took the leaver's place")
		else:
			var sv: Survival = w.mode_rules
			_check(sv.pilots().size() == 1, "the leaver's ship is gone")


func _client_step(w, now: int) -> void:
	if w == null or w.player == null or w.mode_rules == null:
		return
	var p: Ship = w.player
	saw_mode = w.mode
	if w.mode == "survival":
		var sv: Survival = w.mode_rules
		saw_wave = maxi(saw_wave, sv.wave)
		if sv.wave >= 1 and sv.break_left > 0.0 and sv.enemies_left == 0:
			saw_break_after = true
		saw_kills = maxi(saw_kills, sv.run_kills(p))
		for s in sv.raiders():
			if s.team() != p.team() and p.team() == sv.race:
				saw_raider_enemy = true
		if saw_break_after and now - world_seen_at > 16000:
			_leave(now)
	else:
		var sk: Skirmish = w.mode_rules
		saw_side = sk.side_of(p)
		saw_score = maxi(saw_score, sk.score[0])
		teams_ok = sk.teams.size() == 2 and sk.teams[0] == 1 and sk.teams[1] != 1
		if p.dead and died_at < 0:
			died_at = now
		if p.dead and not picked and now - died_at > 300:
			picked = true
			var ev := InputEventKey.new()
			ev.keycode = KEY_1
			ev.physical_keycode = KEY_1
			ev.pressed = true
			root.push_input(ev, true)
		if picked and not p.dead and relaunched_as == "":
			relaunched_as = p.design_name
		if relaunched_as != "" and now - world_seen_at > 20000:
			_leave(now)


## The joiner leaves for the menu; the host sees the seat go.
func _leave(now: int) -> void:
	if quit_at < 0:
		quit_at = now + 1500
		main.call_deferred("_show_menu")


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


func _finish(w) -> void:
	if is_host:
		_check(joiner_seen_at > 0, "the joiner took a seat")
		_check(step == 4, "the host played every step (%d of 4)" % step)
	else:
		_check(saw_mode in ["survival", "skirmish"], "the joiner got the mode from the host (%s)" % saw_mode)
		if saw_mode == "survival":
			_check(saw_wave >= 1, "the joiner saw wave 1")
			_check(saw_raider_enemy, "the joiner sees the raiders as enemies")
			_check(saw_break_after, "the joiner saw the wave beaten")
			_check(saw_kills >= 3, "the board showed the joiner's kills (%d)" % saw_kills)
		elif saw_mode == "skirmish":
			_check(teams_ok and saw_side == 1, "the joiner is on the other side (%d)" % saw_side)
			_check(saw_score >= 1, "the joiner saw the score")
			_check(picked and relaunched_as == "Scout", "the joiner picked a Scout and launched in it (%s)" % relaunched_as)
	print("[%s] %s" % [_tag(), "FAILED" if failed else "passed"])
