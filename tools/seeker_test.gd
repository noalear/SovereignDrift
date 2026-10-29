# Seeker test: do homing missiles hit a ship that dodges?  (Kyle,
# 2026-10-29: "the seeker missiles can't hit anything, they're super easy
# to shoot down and don't really seek.")
#   godot --headless --fixed-fps 60 --path . -s tools/seeker_test.gd
# Out in empty space a stock Terran Scout fires its seeker rack, one shot
# at a time, at a Drengin Scout that flies one of several evasions:
# crossing at full speed, running away on the booster, a hard turning
# circle, random jinks, a late break (a hard turn across the shot's path
# when it is close) and going cold (engines off, coasting).  Counts hits,
# misses (the shot ran out of fuel) and shots downed by a PDC fitted to
# the target.  --runs=N sets the shots per case (default 10), --mid=008
# (004, 021) tests the missile rack (torpedo, drone bay) from a Destroyer
# instead, --moves=a,b only those evasions, --report only prints the
# table, --no-pdc skips the PDC cases (and the checks).
# --duel=Hull: computer pilots instead (run it without --fixed-fps: the
# pilots time their launches by the clock), a Terran Scout against a
# Drengin ship of that hull, 40 s a fight: how its seekers end (hit, out
# of fuel, shot down by guns or PDCs).
# Measured 2026-10-29, average hit rate with no PDC, before -> after the
# steering fix: seeker 14% -> 98%, missile 28% -> 87%, drone 47% -> 100%,
# torpedo 39% -> 51%; Scout fights: seekers hit 23% -> 80%, downed by
# guns 40% -> 20%.
extends SceneTree

var main: Node
var gd: Node
var w
var fails := 0
var runs := 10
var mid := "009"
var report := false
var pdc_cases := [false, true]
var moves := ["cross", "flee", "circle", "jink", "break", "cold"]
var duel := ""                   # --duel=Hull: computer-pilot fights instead
var rows := []


func _init() -> void:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--runs="):
			runs = int(arg.get_slice("=", 1))
		elif arg.begins_with("--mid="):
			mid = arg.get_slice("=", 1)
		elif arg == "--report":
			report = true
		elif arg.begins_with("--moves="):
			moves = Array(arg.get_slice("=", 1).split(","))
		elif arg.begins_with("--duel="):
			duel = arg.get_slice("=", 1)
		elif arg == "--no-pdc":
			pdc_cases = [false]
			report = true
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	if report:
		return
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
	seed(29)
	if duel != "":
		await _duels()
		quit(0)
		return
	var res := {}
	for pdc in pdc_cases:
		for dist in [500.0, 900.0]:
			for move in moves:
				var r: Dictionary = await _case(move, dist, pdc)
				res["%s %d %s" % [move, int(dist), "pdc" if pdc else "-"]] = r
				rows.append("  %-7s %4d px  %-6s  hit %3d%%  missed %3d%%  shot down %3d%%  (closest %d px)" % [
					move, int(dist), "PDC" if pdc else "no PDC", r["hit"], r["miss"], r["down"], r["closest"]])
	print("%s v a dodging Scout, %d shots per case:" % [gd.MISSILES[mid]["name"], runs])
	print("\n".join(rows))
	var all_hit := 0
	var n := 0
	for k in res:
		if not k.ends_with("pdc"):
			all_hit += res[k]["hit"]
			n += 1
	print("  average hit rate without PDC: %d%%" % (all_hit / maxi(n, 1)))
	if mid == "009" and not report:
		_check(res["cross 900 -"]["hit"] >= 80, "seekers hit a Scout crossing at full speed (%d%%)" % res["cross 900 -"]["hit"])
		_check(res["circle 500 -"]["hit"] >= 70, "seekers hit a Scout turning hard (%d%%)" % res["circle 500 -"]["hit"])
		_check(res["jink 900 -"]["hit"] >= 70, "seekers hit a Scout jinking at random (%d%%)" % res["jink 900 -"]["hit"])
		_check(res["flee 500 -"]["hit"] >= 60, "seekers run down a Scout fleeing on its booster (%d%%)" % res["flee 500 -"]["hit"])
		_check(all_hit / maxi(n, 1) >= 75, "seekers hit at least 75%% of the time without a PDC (%d%%)" % (all_hit / maxi(n, 1)))
		# Kyle (2026-10-28): a PDC stops single shots; salvos beat it
		_check(res["cross 900 pdc"]["down"] >= 50, "one PDC still stops most lone seekers (%d%%)" % res["cross 900 pdc"]["down"])
	print("seeker_test: %s (%d failed)" % ["OK" if fails == 0 else "FAILED", fails])
	quit(0 if fails == 0 else 1)


func _clear(ships: Array) -> void:
	for s in ships:
		if is_instance_valid(s) and not s.dead:
			s.dead = true
			s.queue_free()
			w.ships.erase(s)
			w.space.remove_body(s.body)
	for m in w.missiles.duplicate():
		if is_instance_valid(m) and not m.exploded:
			m.shoot_down()


## One case: `runs` single shots from `dist` px at a Scout flying `move`.
func _case(move: String, dist: float, pdc: bool) -> Dictionary:
	var hit := 0
	var miss := 0
	var down := 0
	var closest := 0.0
	for i in runs:
		var r: Dictionary = await _shot(move, dist, pdc)
		match r["end"]:
			"hit": hit += 1
			"down": down += 1
			_: miss += 1
		closest += r["closest"]
	return {"hit": 100 * hit / runs, "miss": 100 * miss / runs, "down": 100 * down / runs, "closest": int(closest / runs)}


func _shot(move: String, dist: float, pdc: bool) -> Dictionary:
	var at: Vector2 = w.player.global_position + Vector2(12000.0, 0.0).rotated(randf() * TAU)
	var base: Vector2 = w.player.velocity
	var shooter: Ship = w.spawn_ship("Shooter", 1, "Scout", "Shooter", at, base, -1)
	if mid != "009":
		# other launchers ride a Destroyer's turret (a Scout can't take a rack)
		var fit: Dictionary = gd.stock_design("Destroyer", 1)
		fit["weapons"][0][0] = {"008": "4008", "004": "4003", "021": "4021"}[mid]
		shooter.refit(fit)
	var away := Vector2.RIGHT.rotated(randf() * TAU)
	var tpos: Vector2 = at + away * dist
	var t: Ship = w.spawn_ship("Target", 2, "Scout", "Target", tpos, base, -1)
	if pdc:
		var fit: Dictionary = gd.stock_design("Scout", 2)
		fit["modules"].append("4022")
		t.refit(fit)
	var side := away.orthogonal() * (1.0 if randf() < 0.5 else -1.0)
	var tv: Vector2 = base
	match move:
		"cross", "cold", "break":
			tv += side * t.max_speed
			t.rotation = side.angle()
		"flee":
			tv += away * t.max_speed * 0.6
			t.rotation = away.angle()
		"circle", "jink":
			tv += side * t.max_speed * 0.8
			t.rotation = side.angle()
	# it has been flying a while: warm, as a ship in a fight is
	t.sensors.heat = 60.0
	t.velocity = tv
	t.body.vel = tv
	t.body.angle = t.rotation
	shooter.rotation = away.angle()
	shooter.body.angle = shooter.rotation
	shooter.target = t
	t.reset_physics_interpolation()
	await physics_frame
	await physics_frame
	var tube: Dictionary = {}
	for wp in shooter.weapons:
		if wp["missile"] == mid:
			tube = wp
	var before: int = w.missiles.size()
	shooter.energy = shooter.energy_max
	shooter._fire_tube(tube)
	var m: Missile = null
	for k in range(before, w.missiles.size()):
		if w.missiles[k].missile_id == mid:
			m = w.missiles[k]
	if m == null:
		_clear([shooter, t])
		return {"end": "none", "closest": 0.0}
	var hp0: float = t.hull + t.shield
	var shot0: int = t.pdc.shot_down
	var closest := INF
	var turn := 1.0
	var flip := randf_range(0.5, 1.2)
	var frames := 0
	while frames < 60 * 12:
		frames += 1
		# the shooter sits still; the target flies its evasion
		shooter.velocity = base
		shooter.body.vel = base
		var inp: Dictionary = t.inputs
		inp["thrust"] = 1.0
		inp["boost"] = move == "flee"
		match move:
			"cross", "flee":
				inp["turn"] = 0.0
			"break":
				# a pilot's dodge: fly on until the shot is close, then
				# turn square across its path and boost
				if is_instance_valid(m) and not m.exploded and m.global_position.distance_to(t.global_position) < 260.0:
					var across: Vector2 = m.velocity.orthogonal().normalized()
					if across.dot(t.velocity) < 0.0:
						across = -across
					inp["turn"] = clampf(angle_difference(t.rotation, across.angle()) * 3.0, -1.0, 1.0)
					inp["boost"] = true
				else:
					inp["turn"] = 0.0
			"cold":
				# engines off: coast on, cooling, so a heat seeker loses the scent
				inp["thrust"] = 0.0
				inp["turn"] = 0.0
			"circle":
				inp["turn"] = 1.0
			"jink":
				flip -= 1.0 / 60.0
				if flip <= 0.0:
					turn = -turn if randf() < 0.7 else 0.0 if turn != 0.0 else 1.0
					flip = randf_range(0.5, 1.2)
				inp["turn"] = turn
		t.energy = t.energy_max
		t.fuel = t.fuel_max
		await physics_frame
		if not is_instance_valid(m) or m.exploded:
			break
		closest = minf(closest, m.global_position.distance_to(t.global_position) - t.size)
		if OS.has_environment("SEEKER_DEBUG") and frames % 15 == 0:
			print("  f%d acc %.0f vmax %.0f boost %.1fx heat %.0f track %s d %.0f tgt %s" % [frames, t.accel, t.max_speed, t.boost_accel, t.signature("heat"), str(shooter.has_track(t, "heat")), m.global_position.distance_to(t.global_position), str(m.target != null)])
	for i in 10:
		await physics_frame
	var end := "miss"
	if t.pdc.shot_down > shot0:
		end = "down"
	elif t.hull + t.shield < hp0 - 1.0 or t.dead:
		end = "hit"
		closest = 0.0
	_clear([shooter, t])
	return {"end": end, "closest": maxf(closest, 0.0) if closest != INF else 0.0}


## Computer pilots: a Terran Scout against a Drengin `duel`, `runs`
## fights of up to 40 s; how the Scout's seekers ended.
func _duels() -> void:
	var f0: Dictionary = Missile.fates.get(mid, {"hit": 0, "blast": 0, "spent": 0, "down": 0}).duplicate()
	var pdc_down := 0
	for i in runs:
		var at: Vector2 = w.player.global_position + Vector2(12000.0, 0.0).rotated(randf() * TAU)
		var v: Vector2 = w.player.velocity
		var off := Vector2(450.0, 0.0).rotated(randf() * TAU)
		var a: Ship = w.spawn_ship("A", 1, "Scout", "A", at + off, v, 2)
		var b: Ship = w.spawn_ship("B", 2, duel, "B", at - off, v, 2)
		for pr in [[a, b], [b, a]]:
			var ai = pr[0].get_node("AI")
			ai.ferry = false
			ai.bombard = false
			ai.raid = false
			ai.duel = true
			pr[0].target = pr[1]
		var frames := 0
		while frames < 60 * 40 and not a.dead and not b.dead:
			frames += 1
			await physics_frame
		pdc_down += b.pdc.shot_down
		_clear([a, b])
		for k in 30:
			await physics_frame
	var f1: Dictionary = Missile.fates.get(mid, {"hit": 0, "blast": 0, "spent": 0, "down": 0})
	var n := 0
	var d := {}
	for k in f1:
		d[k] = f1[k] - f0.get(k, 0)
		n += d[k]
	print("%s from a Terran Scout v a Drengin %s, %d fights: %d fired, hit %d%%, out of fuel %d%%, shot down %d%% (by PDC %d, by guns %d)" % [
		gd.MISSILES[mid]["name"], duel, runs, n, 100 * d["hit"] / maxi(n, 1), 100 * (d["spent"] + d["blast"]) / maxi(n, 1),
		100 * d["down"] / maxi(n, 1), pdc_down, d["down"] - pdc_down])
