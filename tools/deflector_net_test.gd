# Deflector shields online: a joiner sees a turned shot change hands and
# fly back.  Two headless processes:
#   godot --headless --path . -s tools/deflector_net_test.gd -- --host
#   godot --headless --path . -s tools/deflector_net_test.gd -- --join=127.0.0.1
# The host parks a Drengin Cruiser (Deflector) and a Terran Cruiser far
# out, and once the joiner is in, the Terran fires a mass driver bolt into
# the Deflector every half second.  The joiner checks that shots fly on its
# screen, that some turn from Terran to Drengin, and that a turned one
# heads back west.  Exit code 1 when a check fails.
extends SceneTree

var is_host := "--host" in OS.get_cmdline_user_args()
var main: Node
var net: Node
var gd: Node
var frames := 0
var failed := false
var dren = null
var terr = null
var at := Vector2.ZERO
var fire_in := 0.0
var shots := {}           # joiner: net id -> [first pos, last pos, first race, last race, last velocity]


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	net = root.get_node("/root/Net")
	gd = root.get_node("/root/GameData")
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host / --join itself


func _tag() -> String:
	return "host" if is_host else "client"


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


func _park(s, pos: Vector2, rot: float) -> void:
	s.global_position = pos
	s.body.pos = pos
	s.velocity = Vector2.ZERO
	s.body.vel = Vector2.ZERO
	s.body.ang_vel = 0.0
	s.rotation = rot
	s.body.angle = rot


func _physics_process(_delta: float) -> bool:
	frames += 1
	var w = main.get("world") if main != null else null
	if w == null:
		return false
	if is_host:
		return _host(w)
	return _client(w)


func _host(w) -> bool:
	if dren == null and w.player != null:
		at = w.player.global_position + Vector2(9000.0, 7000.0)
		dren = w.spawn_ship("Wall", 2, "Cruiser", "Wall", at, Vector2.ZERO)
		terr = w.spawn_ship("Gunner", 1, "Cruiser", "Gunner", at + Vector2(-600.0, 0.0), Vector2.ZERO)
		for s in [dren, terr]:
			s.set_physics_process(false)
			net.host_ship_added(s)
	if dren != null:
		_park(dren, at, PI)
		_park(terr, at + Vector2(-600.0, 0.0), 0.0)
		dren.shield = dren.shield_max
		terr.hull = terr.hull_max
		dren.invulnerable = 0.0
		terr.invulnerable = 0.0
		if not net.seats.is_empty():
			fire_in -= 1.0 / 60.0
			if fire_in <= 0.0:
				fire_in = 0.5
				var v: Vector2 = Vector2.RIGHT * gd.MISSILES["001"]["fire_v"] * gd.LIGHT_SPEED
				w.spawn_missile(terr, "001", terr.global_position + Vector2(80.0, 0.0), v, 0.0, null)
	if frames >= 60 * 30:
		_check(w.deflected > 5, "the Deflector turned shots (%d)" % w.deflected)
		print("[host] %s" % ("FAILED" if failed else "passed"))
		quit(1 if failed else 0)
		return true
	return false


func _client(w) -> bool:
	for m in w.missiles:
		if is_instance_valid(m) and m.has_meta("net_id") and not m.exploded and m.missile_id == "001":
			var id: int = m.get_meta("net_id")
			if not shots.has(id):
				shots[id] = [m.global_position, m.global_position, m.owner_race, m.owner_race, m.velocity]
			shots[id][1] = m.global_position
			shots[id][3] = m.owner_race
			shots[id][4] = m.velocity
	if frames >= 60 * 22:
		var moved := 0
		var turned := 0
		var back := 0
		for id in shots:
			var s: Array = shots[id]
			if s[0].distance_to(s[1]) > 20.0:
				moved += 1
			if s[2] == 1 and s[3] == 2:
				turned += 1
				if s[4].x < 0.0:
					back += 1
		_check(shots.size() > 5, "the joiner saw the bolts (%d)" % shots.size())
		_check(moved > shots.size() / 2, "shots fly on the joiner's screen (%d of %d moved)" % [moved, shots.size()])
		_check(turned > 2, "turned shots became the Drengin's on the joiner's screen (%d)" % turned)
		_check(back > 2, "and flew back west (%d)" % back)
		print("[client] %s" % ("FAILED" if failed else "passed"))
		quit(1 if failed else 0)
		return true
	return false
