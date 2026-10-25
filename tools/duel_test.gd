# Online duel test: two headless processes, one hosting a free-for-all
# duel and one joining it.  Both pilots fly for the same empire, so the
# test also proves a duel ignores sides.  Usage:
#   godot --headless --path . -s tools/duel_test.gd -- --host-duel
#   godot --headless --path . -s tools/duel_test.gd -- --join=127.0.0.1
# Start the joiner a few seconds after the host.  The host plays a kill,
# a round win and the next round; the joiner checks it saw each, then
# leaves, and the host checks a computer pilot took its place.  Exit
# code 1 when a check fails.
extends SceneTree

var frames := 0
var main: Node
var net: Node
var settings: Node
var failed := false
var is_host := "--host-duel" in OS.get_cmdline_user_args()
var joiner_seen_at := -1
var world_seen_at := -1
var left_at := -1
var step := 0
var career_before := -1
# what the joiner saw
var saw_killer := ""
var saw_winner := ""
var saw_round := 0
var saw_host_kills := 0
var board_size := 0
var deaths_seen := 0
var respawned := false
var died_at := -1
var round2_deaths := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	net = root.get_node("/root/Net")
	settings = root.get_node("/root/Settings")
	settings.use_profile("online")
	settings.pilot_name = "HostPilot" if is_host else "Duelist"
	settings.race_id = 0              # the same empire: a duel must not care
	career_before = int(settings.career.get("score", 0))
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		# Main.gd failed to compile: usually a stale class cache after new
		# class_name files were pulled.  Fix: godot --headless --path . --import
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host-duel / --join itself
	print("[%s] started, role=%d" % [_tag(), net.role])


func _tag() -> String:
	return "host" if is_host else "client"


func _find(w, pilot: String) -> Ship:
	for s in w.ships:
		if s.pilot_name == pilot:
			return s
	return null


func _process(_delta: float) -> bool:
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
	if w != null and w.player != null and not w.player.dead and not w.player.is_puppet:
		w.player.inputs["thrust"] = 0.3
		w.player.inputs["turn"] = 0.2
	var done: bool = (left_at > 0 and now - left_at > 4000) or (world_seen_at > 0 and now - world_seen_at > 60000) if is_host \
		else (world_seen_at > 0 and now - world_seen_at > 30000)
	if done:
		_finish(w)
		quit(1 if failed else 0)
	return false


func _host_step(w, now: int) -> void:
	if w == null or w.duel_rules == null:
		return
	var p: Ship = w.player
	var joiner := _find(w, "Duelist")
	if joiner != null and joiner_seen_at < 0:
		joiner_seen_at = now
		_check(w.duel_rules.leaderboard().size() == Duel.PILOTS, "a computer pilot made room for the joiner (%d pilots)" % w.duel_rules.leaderboard().size())
		_check(joiner.design_name == Duel.HULL and joiner.race_id == 0, "the joiner flies a %s of their own empire" % joiner.design_name)
		# the computer pilots sit out round 1, so only the test's own
		# kills happen in it (round 2 brings them back)
		for s in w.duel_rules.bots():
			w._bench(s, 1e9)
	if joiner_seen_at < 0:
		return
	var t := now - joiner_seen_at
	if step == 0 and t > 2000 and joiner != null:
		step = 1
		# same empire, still enemies: beams, shots and targeting all see them
		joiner.global_position = p.global_position + Vector2(300, 0)
		joiner.velocity = p.velocity
		joiner.body.pos = joiner.global_position   # the beam traces bodies, which move on the next step
		joiner.invulnerable = 0.0
		var hit: Dictionary = w.raycast_beam(p, p.global_position, Vector2.RIGHT, 1000.0)
		_check(hit["body"] == joiner, "a beam hits a same-empire pilot in a duel")
		_check(p.team() != joiner.team(), "pilots of one empire are on different sides")
		_check(w.nearest_enemy_ship(p.global_position, p.team(), 400.0, null) == joiner, "the nearest enemy is the same-empire joiner")
		_check(w.capture.why_not(p, joiner) == "duel", "no boarding in a duel")
		joiner.take_damage(1e6, p)
		_check(p.kills == 1 and joiner.deaths == 1, "the kill counted (kills %d, joiner deaths %d)" % [p.kills, joiner.deaths])
	if step == 1 and t > 7500 and joiner != null and not joiner.dead:
		step = 2
		# one kill short of the win, then the winning kill on the joiner again
		p.kills = Duel.KILLS_TO_WIN - 1
		joiner.invulnerable = 0.0
		joiner.take_damage(1e6, p)
		_check(w.duel_rules.break_left > 0.0 and w.duel_rules.winner_name == "HostPilot", "the tenth kill wins the round")
	if step == 2 and joiner != null and not joiner.dead and joiner.invulnerable < 2.5:
		step = 3
		joiner.take_damage(50.0, p)
		_check(joiner.hull == joiner.hull_max and w.duel_rules.break_left > 0.0, "nobody takes damage in the ceasefire")
	if step == 3 and w.duel_rules.round_no == 2:
		step = 4
		var clean := true
		for s in w.duel_rules.leaderboard():
			clean = clean and s.kills == 0 and s.deaths == 0 and not s.dead
		_check(clean, "round 2 starts with a clean slate and everyone alive")
	if joiner == null and step >= 4 and left_at < 0:
		left_at = now
	if left_at > 0 and now - left_at > 2000 and step == 4:
		step = 5
		_check(w.duel_rules.leaderboard().size() == Duel.PILOTS and w.duel_rules.bots().size() == Duel.PILOTS - 1,
			"a computer pilot took the leaver's place (%d pilots, %d bots)" % [w.duel_rules.leaderboard().size(), w.duel_rules.bots().size()])


func _client_step(w, now: int) -> void:
	if w == null or w.player == null or w.duel_rules == null:
		return
	var du = w.duel_rules
	board_size = maxi(board_size, du.leaderboard().size())
	var host := _find(w, "HostPilot")
	if host != null:
		saw_host_kills = maxi(saw_host_kills, host.kills)
	if w.player.dead and died_at < 0:
		died_at = now
	if died_at > 0 and not w.player.dead:
		respawned = true
	deaths_seen = maxi(deaths_seen, w.player.deaths)
	if du.killed_by != "" and saw_killer == "":
		saw_killer = du.killed_by
	if du.break_left > 0.0 and du.winner_name != "":
		saw_winner = du.winner_name
	if du.round_no == 2 and saw_round < 2:
		round2_deaths = w.player.deaths
	saw_round = maxi(saw_round, du.round_no)


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


func _finish(w) -> void:
	_check(w != null and w.duel_rules != null, "the world is a duel")
	if w == null or w.duel_rules == null:
		print("[%s] FAILED" % _tag())
		return
	_check(w.round_rules == null, "no Conquest round in a duel")
	_check(not w.ships.any(func(s): return s.is_station or s.support), "no starbases or supply ships")
	_check(not w.records_career() and int(settings.career.get("score", 0)) == career_before, "the duel stays off the career record")
	if is_host:
		_check(joiner_seen_at > 0, "the joiner took a seat")
		_check(step == 5, "the host played every step (%d of 5)" % step)
	else:
		_check(board_size == Duel.PILOTS, "the joiner sees all %d pilots (%d)" % [Duel.PILOTS, board_size])
		_check(w.player.design_name == Duel.HULL and not w.player.is_puppet, "the joiner flies its own %s" % w.player.design_name)
		_check(saw_killer == "HostPilot", "the death screen names the killer (%s)" % saw_killer)
		_check(deaths_seen >= 1 and respawned, "the joiner died and relaunched")
		_check(saw_host_kills >= Duel.KILLS_TO_WIN, "the board showed the host's kills (%d)" % saw_host_kills)
		_check(deaths_seen >= 2, "both of the host's kills on the joiner counted (%d deaths)" % deaths_seen)
		_check(saw_winner == "HostPilot", "the winner was announced (%s)" % saw_winner)
		_check(saw_round == 2 and round2_deaths == 0, "round 2 arrived with a clean slate (round %d, deaths %d)" % [saw_round, round2_deaths])
	print("[%s] %s" % [_tag(), "FAILED" if failed else "passed"])
