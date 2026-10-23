# Online smoke test: two headless processes, one hosting and one joining,
# fly together for a while and report what crossed the wire.  Usage:
#   godot --headless --path . -s tools/net_smoke.gd -- --host
#   godot --headless --path . -s tools/net_smoke.gd -- --join=127.0.0.1
# The host runs 30 s, the client 20 s; exit code 1 when a check fails.
extends SceneTree

var frames := 0
var main: Node
var failed := false
var is_host := "--host" in OS.get_cmdline_user_args()
var peak_ships := 0
var moved := 0.0
var start_pos := Vector2.INF
var net: Node


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	net = root.get_node("/root/Net")
	root.get_node("/root/Settings").pilot_name = "HostPilot" if is_host else "Joiner"
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)   # Main reads --host / --join itself
	print("[%s] started, role=%d" % [_tag(), net.role])


func _tag() -> String:
	return "host" if is_host else "client"


func _process(_delta: float) -> bool:
	frames += 1
	var w = main.world if main != null else null
	var p = w.player if w != null else null
	if p != null and not p.dead and not p.is_puppet:
		# fly in a slow circle and shoot
		p.inputs["thrust"] = 1.0
		p.inputs["turn"] = 0.4
		p.inputs["fire_primary"] = frames % 40 < 20
		if start_pos == Vector2.INF:
			start_pos = p.global_position
		moved = maxf(moved, p.global_position.distance_to(start_pos))
	if w != null:
		peak_ships = maxi(peak_ships, w.ships.size())
	if frames % 300 == 0:
		print("[%s] wall=%dms phys=%d t=%ds ships=%d missiles=%d snapshots=%d inputs=%d seats=%d player=%s pos=%s" % [
			_tag(), Time.get_ticks_msec(), Engine.get_physics_frames(), frames / 60, (w.ships.size() if w else -1), (w.missiles.size() if w else -1),
			net.snapshots_received, net.inputs_received, net.seats.size(),
			(p.pilot_name if p else "none"), (p.global_position.round() if p else Vector2.ZERO)])
	var limit := 30 * 60 if is_host else 20 * 60
	if frames >= limit:
		_finish(w)
		quit(1 if failed else 0)
	return false


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


func _finish(w) -> void:
	_check(w != null, "world exists")
	if w == null:
		return
	if is_host:
		_check(net.inputs_received > 200, "received inputs from the joiner (%d)" % net.inputs_received)
		_check(net.seats.size() >= 1 or net.inputs_received > 0, "a joiner took a seat")
		var human := 0
		for s in w.ships:
			if s.pilot_name == "Joiner":
				human += 1
		_check(human == 1 or net.seats.is_empty(), "the joiner's ship is named after them")
	else:
		_check(net.snapshots_received > 100, "received snapshots (%d)" % net.snapshots_received)
		_check(peak_ships >= 10, "roster arrived (%d ships)" % peak_ships)
		_check(w.player != null and not w.player.is_puppet, "own ship is predicted locally")
		_check(moved > 500.0, "own ship moved (%d px)" % int(moved))
		var moving := 0
		for s in w.ships:
			if s.is_puppet and not s.dead and s.velocity.length() > 1.0:
				moving += 1
		_check(moving > 3, "puppets move from snapshots (%d)" % moving)
	print("[%s] %s" % [_tag(), "FAILED" if failed else "passed"])
