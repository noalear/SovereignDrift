# Deflector shield test (Kyle, 2026-10-29: Drengin Deflectors turn gun
# rounds and lasers like a pool shot, and the shot is theirs from then on).
#   godot --headless --path . -s tools/deflector_test.gd
# Checks, in free flight far from everything:
#  - Drengin stock ships carry Deflectors, the others Standard shields; only
#    Drengin yards sell them; no stock fit overloads its reactor;
#  - a mass driver bolt fired into a Deflector comes back at the angle it
#    went in (mirrored off the bubble's edge), costs the half it struck half
#    of what it would have taken, leaves the hull alone, and then hits the
#    ship that fired it, credited to the Drengin;
#  - a glancing bolt keeps its speed along the edge and only turns the part
#    across it; a reflected bolt passes the Drengin's own friends by;
#  - missiles and torpedoes are not turned; an empty half or lowered
#    shields let bolts through to the hull; a Standard shield never turns;
#  - a laser beam bounces off the Deflector and burns its own ship, and the
#    burn is the Drengin's.
extends SceneTree

var main: Node
var gd: Node
var w
var fails := 0
var at := Vector2.ZERO


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
	at = w.player.global_position + Vector2(14000.0, 9000.0)
	w.player.global_position = at + Vector2(0.0, 4000.0)
	w.player.body.pos = w.player.global_position
	_data_checks()
	await _bolt_checks()
	await _beam_checks()
	print("deflector_test: %s (%d failed)" % ["OK" if fails == 0 else "FAILED", fails])
	quit(0 if fails == 0 else 1)


func _shield_ids(d: Dictionary) -> Array:
	var out := []
	for id in d["modules"]:
		if gd.MODULES[id]["func"] == "shield":
			out.append(id)
	return out


func _data_checks() -> void:
	var over := []
	for hull in ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought", "Freighter", "DropShip"]:
		for r in 3:
			var d: Dictionary = gd.race_fit(hull, r)
			var fams := []
			for id in _shield_ids(d):
				fams.append(gd.shield_family(id))
			var want := "deflector" if r == 2 else "standard"
			_check(fams.size() > 0 and fams.count(want) == fams.size(), "%s %s flies %s shields (%s)" % [gd.race(r)["name"], hull, want, ", ".join(fams)])
			var st: Dictionary = gd.stats_for(d)
			if st["power_draw"] > st["power_gen"]:
				over.append("%s %s %d/%d" % [gd.race(r)["name"], hull, st["power_draw"], st["power_gen"]])
	_check(over.is_empty(), "no stock fit overloads its reactor %s" % str(over))
	_check(gd.catalogue(2, []).has("5031") and not gd.catalogue(1, []).has("5031") and not gd.catalogue(0, []).has("5033"), "only Drengin yards sell Deflectors")
	_check(-int(gd.MODULES["5003"]["power"]) >= 100, "Shield III draws real power now (%d)" % -int(gd.MODULES["5003"]["power"]))
	# power per point of shield: a Deflector's is at least twice any other family's
	var worst := 0.0
	var best_def := INF
	for id in gd.MODULES:
		if gd.MODULES[id]["func"] != "shield" or id == "5007":
			continue
		var per: float = -float(gd.MODULES[id]["power"]) / float(gd.MODULES[id]["p"][0])
		if gd.shield_family(id) == "deflector":
			best_def = minf(best_def, per)
		else:
			worst = maxf(worst, per)
	_check(best_def >= worst * 1.5, "a Deflector draws far more for its size than any other shield (%.2f v %.2f a point)" % [best_def, worst])


## Hold `s` still at `pos`, nose along `rot`, shields full.
func _park(s, pos: Vector2, rot: float = 0.0) -> void:
	s.global_position = pos
	s.body.pos = pos
	s.velocity = Vector2.ZERO
	s.body.vel = Vector2.ZERO
	s.body.ang_vel = 0.0
	s.rotation = rot
	s.body.angle = rot
	s.reset_physics_interpolation()


func _ship(race: int, hull: String, pos: Vector2, rot: float = 0.0):
	var s = w.spawn_ship("Test", race, hull, "Test", pos, Vector2.ZERO)
	s.set_physics_process(false)          # nobody flies them; the world still moves them
	_park(s, pos, rot)
	s.invulnerable = 0.0
	return s


func _full(s) -> void:
	s.shield = s.shield_max
	s.hull = s.hull_max
	s.shield_pause = [0.0, 0.0]


## Clears the sky of earlier shots (a turned bolt still flying would shoot
## the next missile down) and fires one.
func _bolt(owner, id: String, pos: Vector2, dir: Vector2):
	for old in w.missiles:
		if is_instance_valid(old) and not old.exploded:
			old._free()
	var md: Dictionary = gd.MISSILES[id]
	w.spawn_missile(owner, id, pos, dir * md["fire_v"] * gd.LIGHT_SPEED, dir.angle(), null)
	return w.missiles[w.missiles.size() - 1]


## Step the world until `done` says so (or `frames` pass), holding the
## parked ships still.
func _gone(m) -> bool:
	return not is_instance_valid(m) or m.exploded


func _run(ships: Array, frames: int, done: Callable) -> void:
	var where := []
	for s in ships:
		where.append([s.global_position, s.rotation])
	for i in frames:
		await physics_frame
		for k in ships.size():
			_park(ships[k], where[k][0], where[k][1])
		if done.call():
			return


func _bolt_checks() -> void:
	var dren = _ship(2, "Cruiser", at, PI)       # facing the Terran
	var terr = _ship(1, "Cruiser", at + Vector2(-700.0, 0.0))
	_check(dren.shield_family == "deflector" and dren.deflects(), "the Drengin Cruiser's Deflector is up")
	await physics_frame
	# head on: back the way it came, at the shooter
	_full(dren)
	_full(terr)
	var before: float = dren.shield_front
	var t_hull: float = terr.hull
	var d_hull: float = dren.hull
	var n0: int = w.deflected
	var m = _bolt(terr, "001", terr.global_position + Vector2(80.0, 0.0), Vector2.RIGHT)
	var box := [m]
	var warhead: float = m.warhead
	await _run([dren, terr], 120, func(): return w.deflected > n0)
	_check(w.deflected == n0 + 1 and is_instance_valid(m) and m.owner_ship == dren, "a bolt into the Deflector is turned and becomes the Drengin's")
	if is_instance_valid(m):
		_check(m.velocity.x < 0.0 and absf(m.velocity.angle_to(Vector2.LEFT)) < 0.08, "head on, it comes straight back (heading %.2f)" % m.velocity.angle())
		_check(m.owner_team == dren.team() and m.owner_race == 2, "it flies for the Drengin side now")
	var cost: float = warhead * gd.damage_mult("kinetic", "shield") * gd.SHIELD_FAMILIES["deflector"]["deflect"]
	_check(absf(before - dren.shield_front - cost) < 1.0, "the front half paid %.0f for it (want %.0f)" % [before - dren.shield_front, cost])
	_check(dren.hull == d_hull, "the Drengin hull is untouched")
	await _run([dren, terr], 200, func(): return terr.hull < t_hull or _gone(box[0]))
	_check(terr.hull < t_hull or terr.shield < terr.shield_max - 1.0, "the bolt comes back and hits the Terran who fired it")
	_check(terr.last_attacker == dren, "and the hit is the Drengin's")
	# a glancing shot: the speed along the edge stays, the part across it turns
	_full(dren)
	var e: Array = dren.shield_ellipse()
	var top: Vector2 = dren.global_position + Vector2(e[0].x, e[0].y + e[1].y * 0.7).rotated(PI)
	var from: Vector2 = top + Vector2(-500.0, -500.0)
	n0 = w.deflected
	m = _bolt(terr, "001", from, (top - from).normalized())
	box = [m]
	var v_in: Vector2 = m.velocity
	await _run([dren, terr], 120, func(): return w.deflected > n0)
	_check(w.deflected == n0 + 1 and is_instance_valid(m), "a glancing bolt is turned too")
	if is_instance_valid(m):
		var hit: Dictionary = w.bubble_hit(dren, from, v_in, 10.0)
		var n: Vector2 = hit.get("normal", Vector2.UP)
		var v_out: Vector2 = m.velocity
		_check(absf(v_in.dot(n) + v_out.dot(n)) < 2.0 and absf(v_in.dot(n.orthogonal()) - v_out.dot(n.orthogonal())) < 2.0, "angle in is angle out off the edge's normal (in %.0f/%.0f, out %.0f/%.0f)" % [v_in.dot(n), v_in.dot(n.orthogonal()), v_out.dot(n), v_out.dot(n.orthogonal())])
		_check(v_out.y < 0.0, "it glances away upward, not into the hull")
		m._free()
	# the reflected bolt spares a Drengin friend in its way
	_full(dren)
	var friend = _ship(2, "Destroyer", at + Vector2(-350.0, 0.0), PI / 2.0)
	friend.set_shields_raised(false)
	await physics_frame
	var f_hull: float = friend.hull
	n0 = w.deflected
	# fired from just past the friend (a Terran inside the line), straight at the Drengin
	m = _bolt(terr, "001", at + Vector2(-260.0, 0.0), Vector2.RIGHT)
	box = [m]
	await _run([dren, terr, friend], 200, func(): return _gone(box[0]) or box[0].global_position.x < at.x - 520.0)
	_check(w.deflected > n0 and friend.hull == f_hull, "the turned bolt passes the Drengin's friend by (hull %d of %d)" % [friend.hull, f_hull])
	friend.queue_free()
	w.ships.erase(friend)
	w.space.remove_body(friend.body)
	# missiles and torpedoes are not turned
	for id in ["008", "004"]:
		_full(dren)
		n0 = w.deflected
		var sh: float = dren.shield
		m = _bolt(terr, id, terr.global_position + Vector2(80.0, 0.0), Vector2.RIGHT)
		m.target = dren
		box = [m]
		m.target = dren
		await _run([dren, terr], 200, func(): return _gone(box[0]))
		_check(w.deflected == n0 and dren.shield < sh - 1.0, "a %s is not turned: the shield takes it (%.0f -> %.0f)" % [gd.MISSILES[id]["name"], sh, dren.shield])
	# an empty half lets bolts through
	_full(dren)
	dren.shield_front = 0.0
	n0 = w.deflected      # (it faces the Terran: the front half takes the bolts)
	d_hull = dren.hull
	m = _bolt(terr, "001", terr.global_position + Vector2(80.0, 0.0), Vector2.RIGHT)
	box = [m]
	await _run([dren, terr], 120, func(): return _gone(box[0]))
	_check(w.deflected == n0 and dren.hull < d_hull, "with the front half empty the bolt reaches the hull")
	# lowered shields turn nothing
	_full(dren)
	dren.set_shields_raised(false)
	n0 = w.deflected
	d_hull = dren.hull
	m = _bolt(terr, "001", terr.global_position + Vector2(80.0, 0.0), Vector2.RIGHT)
	box = [m]
	await _run([dren, terr], 120, func(): return _gone(box[0]))
	_check(w.deflected == n0 and dren.hull < d_hull, "lowered shields turn nothing")
	dren.set_shields_raised(true)
	dren.shield_boot = 0.0
	# a Standard shield never turns a bolt
	_full(terr)
	n0 = w.deflected
	m = _bolt(dren, "001", dren.global_position + Vector2(-90.0, 0.0), Vector2.LEFT)
	box = [m]
	await _run([dren, terr], 120, func(): return _gone(box[0]))
	_check(w.deflected == n0 and terr.shield < terr.shield_max, "a Standard shield takes the bolt instead of turning it")
	for s in [dren, terr]:
		w.ships.erase(s)
		w.space.remove_body(s.body)
		s.queue_free()
	await physics_frame


func _beam_checks() -> void:
	var dren = _ship(2, "Cruiser", at + Vector2(0.0, 1500.0))
	var arc = _ship(0, "Cruiser", at + Vector2(-300.0, 1500.0))
	await physics_frame
	_full(dren)
	_full(arc)
	var tr: Dictionary = w.trace_beam(arc, arc.global_position + Vector2(40.0, 0.0), Vector2.RIGHT, 1500.0)
	_check(tr["bounces"].size() == 1 and tr["bounces"][0][0] == dren, "a laser beam glances off the Deflector")
	_check(tr["body"] == arc and tr["owner"] == dren, "and comes back to burn the Arcean who fired it, as the Drengin's beam")
	# the real thing: an Arcean laser on the turret
	var wpn: Dictionary = {}
	for x in arc.weapons:
		if x["kind"] == "beam":
			wpn = x
			break
	_check(not wpn.is_empty(), "the Arcean Cruiser has a laser")
	if wpn.is_empty():
		return
	arc.target = dren
	var a_hull: float = arc.hull + arc.shield
	var d_shield: float = dren.shield
	var d_hull: float = dren.hull
	for i in 30:
		arc.energy = arc.energy_max
		arc._fire_beam(wpn, 1.0 / 60.0)
		await physics_frame
		_park(dren, at + Vector2(0.0, 1500.0))
		_park(arc, at + Vector2(-300.0, 1500.0))
	_check(arc.hull + arc.shield < a_hull - 1.0, "half a second of laser burns its own ship (%.0f lost)" % (a_hull - arc.hull - arc.shield))
	_check(arc.last_attacker == dren, "and the burn is the Drengin's")
	_check(dren.hull == d_hull and dren.shield < d_shield, "the Drengin pays in shield (%.0f -> %.0f), not hull" % [d_shield, dren.shield])
