# Disruptor aim test (Kyle, 2026-10-29: "disruptors shoot straight behind
# you ... it should be a lock/mouse aim weapon").
#   godot --headless --path . -s tools/disruptor_test.gd
# Fits a disruptor on a Scout's rear rack and on its nose, then fires it
# by hand and checks where each bolt goes: at the mouse with no lock, at
# a locked enemy ship (ahead of the mouse), off the nose with neither,
# and for a computer pilot at its target behind it.  A mass driver on the
# nose stays a fixed gun.
extends SceneTree

var main: Node
var gd: Node
var w
var fails := 0


func _init() -> void:
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		fails += 1


func _start() -> void:
	gd = root.get_node("/root/GameData")   # autoloads are not names in a -s script
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	await process_frame
	w = main.world
	_data_checks()
	await _player_shots()
	await _ai_shots()
	print("disruptor_test: %s (%d failed)" % ["OK" if fails == 0 else "FAILED", fails])
	quit(0 if fails == 0 else 1)


func _data_checks() -> void:
	_check(gd.default_weapon_group("4020") == 2, "the disruptor is in group 2 (lasers and disruptors)")
	var hp: Array = gd.hull_hardpoints("Scout")
	_check(int(hp[2][2]) == 180 and gd.weapon_class("4020") <= int(hp[2][5]), "a disruptor fits the Scout's rear rack (fire angle 180)")


## A Scout with `ids` on its three mounts (turret, nose, rear rack).
func _scout(ids: Array) -> Dictionary:
	var d: Dictionary = gd.stock_design("Scout", 1)
	for i in ids.size():
		d["weapons"][i][0] = ids[i]
	return d


## Hold `s` still out in open space, nose east.
func _park(s, at: Vector2) -> void:
	s.global_position = at
	s.body.pos = at
	s.velocity = Vector2.ZERO
	s.body.vel = Vector2.ZERO
	s.rotation = 0.0
	s.body.angle = 0.0
	s.energy = s.energy_max
	s.reset_physics_interpolation()


## Fire mount `index` of `s` once and return the bolt's heading relative
## to the shooter (INF when nothing came out).
func _shot_dir(s, index: int) -> float:
	var before: int = w.missiles.size()
	var wpn: Dictionary = s.weapons[index]
	wpn["cooldown"] = 0.0
	s.energy = s.energy_max
	s._fire_tube(wpn)
	if w.missiles.size() <= before:
		return INF
	var m = w.missiles[w.missiles.size() - 1]
	var rel: Vector2 = m.velocity - s.velocity
	m._free()
	w.missiles.erase(m)
	return rel.angle()


func _near(a: float, b: float, tol: float = 0.15) -> bool:
	return a != INF and absf(wrapf(a - b, -PI, PI)) <= tol


func _player_shots() -> void:
	var p = w.player
	var at: Vector2 = p.global_position + Vector2(12000.0, 0.0)
	# disruptor on the rear rack (where the mine layer was)
	p.refit(_scout(["4501", "4001", "4020"]))
	_park(p, at)
	p.target = null
	p.aim_point = at + Vector2(0.0, -1000.0)          # mouse due north
	var a := _shot_dir(p, 2)
	_check(_near(a, -PI / 2.0), "rear-rack disruptor fires at the mouse (north), not behind (got %.2f)" % a)
	p.aim_point = at + Vector2(1000.0, 0.0)            # mouse dead ahead
	a = _shot_dir(p, 2)
	_check(_near(a, 0.0), "rear-rack disruptor fires forward at a mouse ahead (got %.2f)" % a)
	p.aim_point = Vector2.INF                          # keyboard flying, no target
	a = _shot_dir(p, 2)
	_check(_near(a, 0.0), "with no mouse and no target it fires off the nose, not behind (got %.2f)" % a)
	# disruptor on the nose (arc -50..50): the mouse is outside the arc
	p.refit(_scout(["4501", "4020", "4002"]))
	_park(p, at)
	p.aim_point = at + Vector2(-700.0, 700.0)          # south-west, behind the arc
	a = _shot_dir(p, 1)
	_check(_near(a, (Vector2(-1, 1)).angle()), "nose disruptor follows the mouse outside its arc (got %.2f)" % a)
	# a locked enemy beats the mouse
	var foe = w.spawn_ship("Foe", 2, "Scout", "Foe", at + Vector2(-500.0, 0.0), Vector2.ZERO)
	foe.rotation = 0.0
	p.target = foe
	for i in 10:
		_park(p, at)
		foe.global_position = at + Vector2(-500.0, 0.0)
		foe.body.pos = foe.global_position
		await physics_frame
	_check(foe.visible_to(p), "the foe behind is tracked (locked)")
	p.aim_point = at + Vector2(0.0, -1000.0)
	a = _shot_dir(p, 1)
	_check(_near(a, PI), "nose disruptor fires at the locked foe behind, not the mouse (got %.2f)" % a)
	# a mass driver on the nose stays a fixed gun
	p.refit(_scout(["4501", "4001", "4002"]))
	_park(p, at)
	p.target = null
	p.aim_point = at + Vector2(-700.0, 700.0)
	a = _shot_dir(p, 1)
	_check(_near(a, 0.0), "a nose mass driver still fires straight ahead (got %.2f)" % a)
	foe.queue_free()
	w.ships.erase(foe)
	await physics_frame


func _ai_shots() -> void:
	var at: Vector2 = w.player.global_position + Vector2(-12000.0, 0.0)
	var s = w.spawn_ship("Gunner", 2, "Scout", "Gunner", at, Vector2.ZERO)
	s.refit(_scout(["4001", "4001", "4020"]))
	var foe = w.spawn_ship("Foe", 1, "Scout", "Foe", at + Vector2(0.0, 500.0), Vector2.ZERO)
	s.target = foe
	for i in 10:
		_park(s, at)
		foe.global_position = at + Vector2(0.0, 500.0)
		foe.body.pos = foe.global_position
		await physics_frame
	var a := _shot_dir(s, 2)
	_check(_near(a, PI / 2.0), "a computer pilot's rear-rack disruptor fires at its target (south) (got %.2f)" % a)
	s.target = null
	a = _shot_dir(s, 2)
	_check(_near(a, 0.0), "with nothing targeted it fires off the nose (got %.2f)" % a)
