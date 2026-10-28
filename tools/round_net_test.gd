# Online Conquest round test: the round clock, the phases, the hold
# countdown, the end of the round and the next round reach the joiners.
# Three headless processes:
#   godot --headless --path . -s tools/round_net_test.gd -- --host
#   godot --headless --path . -s tools/round_net_test.gd -- --join=127.0.0.1
#   godot --headless --path . -s tools/round_net_test.gd -- --join=127.0.0.1 --late
# Start the joiner a few seconds after the host, and the late joiner once
# the host prints ROUND_DECIDED (it joins a decided round and must see the
# scoreboard, then the next round).  The host pushes the clock into the
# war phase, takes every colony for its empire, waits out the hold and
# starts the next round.  Exit code 1 when a check fails.
extends SceneTree

const MINUTES := 20
const HOLD := 45.0                 # Round.HOLD_SECONDS (naming Round here loads it before the autoloads)

var main: Node
var net: Node
var settings: Node
var failed := false
var args := OS.get_cmdline_user_args()
var is_host := "--host" in args
var is_late := "--late" in args
var started_at := 0
var step := 0
var step_at := 0
var first_world := 0            # instance id: a freed world reads as null
var rounds_before := 0
var decided_at := -1
# what a joiner saw
var saw_length := false
var saw_war := false
var saw_hold := false
var saw_board := false
var saw_next := false
var next_at := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	net = root.get_node("/root/Net")
	settings = root.get_node("/root/Settings")
	settings.use_profile("online")
	settings.pilot_name = "HostPilot" if is_host else ("Latecomer" if is_late else "Joiner")
	# a joiner's own setting must not matter: the host's round length rules
	settings.round_minutes = MINUTES if is_host else 40
	rounds_before = int(settings.career.get("rounds", 0))
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host / --join itself
	started_at = Time.get_ticks_msec()
	print("[%s] started, role=%d" % [_tag(), net.role])


func _tag() -> String:
	return "host" if is_host else ("late" if is_late else "client")


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok" if ok else "FAILED", what])
	if not ok:
		failed = true


func _go(n: int, now: int) -> void:
	step = n
	step_at = now


func _process(_delta: float) -> bool:
	var now := Time.get_ticks_msec()
	var w = main.world if main != null else null
	if w != null and first_world == 0:
		first_world = w.get_instance_id()
	if is_host:
		_host_step(w, now)
	else:
		_client_step(w, now)
	var limit := 120000 if is_host else 110000
	if now - started_at > limit:
		_check(false, "finished in time (step %d)" % step)
		quit(1)
	return false


func _seated(w, name: String) -> Node:
	if w == null:
		return null
	for peer_id in net.seats:
		var s: Node = net.ships_by_id.get(net.seats[peer_id], null)
		if s != null and s.pilot_name == name:
			return s
	return null


func _host_step(w, now: int) -> void:
	if w == null or w.round_rules == null:
		return
	var rd = w.round_rules
	var race: int = w.player.race_id if w.player != null else 0
	match step:
		0:
			if _seated(w, "Joiner") != null:
				_go(1, now)
		1:
			if now - step_at > 3000:
				rd.elapsed = 100.0          # past the opening: the war phase
				_go(2, now)
		2:
			if now - step_at > 4000:
				_check(rd.phase == "war", "the host's round is at war (%s)" % rd.phase)
				# every colony to the host's empire: the hold countdown starts
				for p in w.planets:
					if p.is_colonised() and p.owner_race != race:
						w.planet_captured(p, race, null)
				_go(3, now)
		3:
			if rd.hold_race == race and rd.hold_timer < HOLD - 8.0:
				rd.hold_timer = HOLD - 8.0
			if rd.over:
				_check(rd.result.get("winner", -1) == race and rd.result.get("how", "") == "conquest", "the round ends by conquest")
				_go(4, now)
		4:
			if main.scoreboard != null:
				_check(main.scoreboard.online == "host", "the host's scoreboard offers the next round")
				print("ROUND_DECIDED")
				decided_at = now
				_go(5, now)
		5:
			# the late joiner gets a seat on the decided round, then the next round starts
			var late := _seated(w, "Latecomer")
			if (late != null and now - step_at > 8000) or now - step_at > 30000:
				_check(late != null, "a pilot can join while the scoreboard is up")
				main.scoreboard.play_again.emit()
				_go(6, now)
		6:
			_check(net.is_host() and w.get_instance_id() != first_world and net.world == w, "the next round is a fresh world and the session stays open")
			_check(not rd.over and rd.elapsed < 5.0 and rd.length == MINUTES * 60.0, "the new round's clock starts again")
			_go(7, now)
		7:
			if (_seated(w, "Joiner") != null and _seated(w, "Latecomer") != null) or now - step_at > 20000:
				_check(_seated(w, "Joiner") != null, "the joiner is seated in the next round")
				_check(_seated(w, "Latecomer") != null, "the late joiner is seated in the next round")
				_go(8, now)
		8:
			if now - step_at > 6000:     # let the joiners check what they see
				print("[host] %s" % ("passed" if not failed else "FAILED"))
				quit(1 if failed else 0)


func _client_step(w, now: int) -> void:
	if w == null or w.player == null or w.round_rules == null:
		return
	var rd = w.round_rules
	if w.get_instance_id() == first_world:
		if not saw_length and rd.length > 0.0:
			saw_length = true
			_check(rd.length == MINUTES * 60.0, "the round length is the host's (%d s)" % int(rd.length))
		if not is_late and not saw_war and rd.phase == "war":
			saw_war = true
			_check(rd.elapsed >= 100.0 and rd.clock_text() != "", "the war phase and the host's clock reached the joiner (%s)" % rd.clock_text())
		if not is_late and not saw_hold and rd.hold_race >= 0 and rd.hold_timer > HOLD - 10.0:
			saw_hold = true
			_check(rd.hold_race == 0 or rd.hold_race == w.player.race_id, "the hold countdown reached the joiner (%.0f s held)" % rd.hold_timer)
		if not saw_board and main.scoreboard != null:
			saw_board = true
			var res: Dictionary = main.scoreboard.result
			_check(main.scoreboard.online == "client", "the joiner's scoreboard waits for the host")
			_check(res.get("how", "") == "conquest" and res.get("winner", -1) == w.player.race_id, "the joiner sees the host's result (%s)" % str(res.get("how", "")))
			_check(rd.over and rd.phase == "over", "the joiner's round is over")
			var rounds := int(settings.career.get("rounds", 0)) - rounds_before
			if is_late:
				_check(rounds == 0, "a round decided before joining is not on the career (%d)" % rounds)
			else:
				_check(rounds == 1, "the round is on the joiner's career (%d)" % rounds)
				var mine := false
				for pl in res.get("pilots", []):
					mine = mine or (pl["is_player"] and pl["name"] == "Joiner")
				_check(mine, "the joiner's own line is marked on the scoreboard")
	elif not saw_next:
		saw_next = true
		next_at = now
		_check(saw_board, "the scoreboard came before the next round")
		_check(main.scoreboard == null and not rd.over, "the next round replaced the scoreboard")
		_check(w.player.pilot_name == settings.pilot_name and not w.player.is_puppet, "the joiner flies its own ship again")
		if not is_late:
			_check(saw_length and saw_war and saw_hold, "the first round's clock, war and hold all reached the joiner")
	elif now - next_at > 3000:
		_check(rd.elapsed > 1.0 and rd.elapsed < 30.0 and rd.phase == "opening", "the next round's clock runs from the start (%s, %s)" % [rd.clock_text(), rd.phase])
		_check(net.ships_by_id.size() > 5, "the next round's ships arrived (%d)" % net.ships_by_id.size())
		print("[%s] %s" % [_tag(), "passed" if not failed else "FAILED"])
		quit(1 if failed else 0)
