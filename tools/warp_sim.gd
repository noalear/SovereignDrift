# Warp report: a normal Sol round with every computer pilot flying, counting
# each ship's warp starts, how many ended before the jump (charge aborted)
# and how many came straight after the last one ended.  A ship that keeps
# starting warps reads as chaos on screen (jump flashes again and again).
#   godot --headless --fixed-fps 60 --path . -s tools/warp_sim.gd -- --minutes=6
extends SceneTree

var main: Node
var minutes := 6.0
var was := {}            # ship -> [warping, charge, state, start time]
var stats := {}          # ship -> {starts, aborted, quick, by_state}
var last_end := {}       # ship -> elapsed when its last warp ended
var ends := {}           # "state:why" -> count
const STATE_NAMES := ["PATROL", "ATTACK", "EVADE", "RETREAT", "ESCORT", "BOARD", "FERRY", "FLEE", "PLAYER"]


func _init() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--minutes="):
			minutes = float(a.get_slice("=", 1))
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	process_frame.connect(_tick)


static func _ai(s: Ship) -> AIPilot:
	for c in s.get_children():
		if c is AIPilot:
			return c
	return null


func _state(s: Ship) -> String:
	var ai := _ai(s)
	return STATE_NAMES[ai.state] if ai != null else "PLAYER"


func _tick() -> void:
	var w: World = main.world
	if w == null:
		return
	var now: float = w.elapsed
	for s in w.ships:
		if not is_instance_valid(s) or s.dead:
			continue
		var prev: Array = was.get(s, [false, 0.0, "", 0.0])
		var st: Dictionary = stats.get(s, {"starts": 0, "aborted": 0, "quick": 0, "by_state": {}})
		stats[s] = st
		if s.warping and not prev[0]:
			st["starts"] += 1
			var k := _state(s)
			st["by_state"][k] = int(st["by_state"].get(k, 0)) + 1
			if now - float(last_end.get(s, -99.0)) < 3.0:
				st["quick"] += 1
			prev = [true, 0.0, k, now]
		elif not s.warping and prev[0]:
			var jumped: bool = prev[1] >= Ship.WARP_CHARGE_TIME - 0.05
			if not jumped:
				st["aborted"] += 1
			var why := "jumped" if jumped else "aborted"
			if s.warp_block > 0.0:
				why += "+blocked"
			if s.energy < 200.0:
				why += "+energy"
			var key := "%s %s" % [_state(s), why]
			ends[key] = int(ends.get(key, 0)) + 1
			last_end[s] = now
			prev = [false, 0.0, "", now]
		elif s.warping:
			prev[1] = s.warp_charge if s.warp_charge > 0.0 else prev[1]
		was[s] = prev
	if now >= minutes * 60.0:
		_report(now)
		quit(0)


func _report(now: float) -> void:
	var gd := root.get_node("GameData")
	var rows := []
	var total := 0
	var quick := 0
	var aborted := 0
	for s in stats:
		if not is_instance_valid(s):
			continue
		var st: Dictionary = stats[s]
		total += st["starts"]
		quick += st["quick"]
		aborted += st["aborted"]
		rows.append([st["starts"], "%-9s %-18s %-14s starts %3d  aborted %3d  quick %3d  %s" % [gd.race(s.race_id)["adj"], s.ship_name, s.design_name,
			st["starts"], st["aborted"], st["quick"], st["by_state"]]])
	rows.sort_custom(func(a, b): return a[0] > b[0])
	print("WARP REPORT after %.0f s: %d starts, %d aborted, %d restarted within 3 s" % [now, total, aborted, quick])
	for r in rows.slice(0, 15):
		print("  " + r[1])
	print("  ends: ", ends)
	print("WARP_STARTS_PER_SHIP_MIN %.2f" % (float(total) / maxf(stats.size(), 1) / (now / 60.0)))
