# Command wings test: the player calls the empire's pilots to its wing
# (seats by rank, Captains only for their equals), the wing flies in
# formation, and each order works: form up, attack my target (and drops
# back to cover when it is down), cover, hold a spot while the leader
# flies off, engage at will, return to base and rejoin, dismiss.  A wing
# whose leader dies goes home and waits; AI admirals call wings of their
# own; the orders wheel gives orders from the keyboard; wings travel
# online (pack/unpack) and in the campaign save.
#   godot --headless --path . -s tools/wing_test.gd
extends SceneTree

var main: Node
var failed := false


func _init() -> void:
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _wait(n: int) -> void:
	for i in n:
		await physics_frame


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel
	s.reset_physics_interpolation()


## Somewhere quiet near Earth, out of the starbase's way.
func _spot(w: World) -> Vector2:
	var earth: Planet = w.planet_by_name["Earth"]
	return earth.global_position + (earth.global_position - w.sun.global_position).normalized() * 2600.0


func _wing(w: World) -> Array:
	return w.wing.members(w.player).filter(func(s): return not s.dead)


func _keep_out(w: World, p: Ship) -> void:
	# no stray enemy wanders into the tests
	for s in w.ships:
		if s.race_id != p.race_id and not s.is_station and not s.dead and s.global_position.distance_to(p.global_position) < 9000.0 and not s.has_meta("tutorial"):
			_put(s, s.global_position + (s.global_position - p.global_position).normalized() * 20000.0, s.velocity)


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	await _wait(60)
	var w: World = main.world
	var p: Ship = w.player
	w.message.connect(func(t, _c): print("  msg: " + t))
	p.invulnerable = 1e9
	_put(p, _spot(w), w.planet_by_name["Earth"].velocity)
	# --- calling the wing
	p.rank = 0
	_check(Wing.seats(0) == 1 and Wing.seats(3) == 2 and Wing.seats(9) == 4 and Wing.seats(16) == Wing.MAX_SEATS, "seats grow with rank: 1, 2 at Lieutenant, up to %d" % Wing.MAX_SEATS)
	_check(Wing.would_serve(3, p) and not Wing.would_serve(9, p), "a junior pilot flies for a cadet, a Senior Captain does not")
	p.rank = 6
	# make sure some Terran pilots are free for the call
	var terrans := w.ships.filter(func(s): return s.race_id == 1 and not s.is_player and not s.support and not s.is_station and s.get_node_or_null("AI") != null)
	for s in terrans:
		var ai: AIPilot = s.get_node("AI")
		s.rank = mini(s.rank, 5)
		ai.ferry = false
		s.colonists = 0.0
		if ai.convoy_guard:
			ai.release()
	w.wing_command(p, "call")
	var wing := _wing(w)
	_check(wing.size() == Wing.seats(6), "the call brings %d pilots for a Cmdr. (got %d)" % [Wing.seats(6), wing.size()])
	_check(wing.all(func(s): return s.get_node("AI").called and s.get_node("AI").escort == p), "they are called up and fly with the player")
	w.wing_command(p, "call")
	_check(w.wing.called(p).size() == Wing.seats(6), "a full wing takes no more")
	_check(w.wing.order_of(p)["order"] == "cover", "a new wing covers its leader")
	# bring them alongside so the test needn't wait for the flight
	for i in wing.size():
		_put(wing[i], p.global_position + Vector2(-400, 300 * (i - 1)), p.velocity)
	_keep_out(w, p)
	await _wait(240)
	var near := wing.all(func(s): return s.global_position.distance_to(p.global_position) < AIPilot.ESCORT_LEASH)
	_check(near, "the wing keeps close in Cover (%s)" % [wing.map(func(s): return int(s.global_position.distance_to(p.global_position)))])
	# --- form up: tight
	w.wing_command(p, "form")
	await _wait(300)
	var tight := wing.all(func(s): return s.global_position.distance_to(s.get_node("AI")._wing_spot()) < 700.0)
	_check(w.wing.order_of(p)["order"] == "form" and tight, "Form up: each wingman in its slot (%s)" % [wing.map(func(s): return int(s.global_position.distance_to(s.get_node("AI")._wing_spot())))])
	var slots := {}
	for s in wing:
		slots[s.get_node("AI").wing_slot] = true
	_check(slots.size() == wing.size(), "every wingman has its own slot")
	# --- attack my target
	var foe := w.spawn_ship("Dummy", 2, "Scout", "Target", p.global_position + Vector2(1300, 0), p.velocity, 0)
	foe.hull_max *= 4.0
	foe.hull = foe.hull_max
	foe.invulnerable = 1e9         # alive until the wing has turned on it
	p.target = null
	w.wing_command(p, "attack")
	_check(w.wing.order_of(p)["order"] == "form", "an attack with no target is refused")
	p.target = foe
	w.wing_command(p, "attack")
	_check(w.wing.order_of(p)["order"] == "attack" and w.wing.order_of(p)["target"] == foe, "Attack my target: the order holds the target")
	await _wait(120)
	_check(wing.filter(func(s): return s.target == foe).size() >= wing.size() - 1, "the wing goes for it (%s)" % [wing.map(func(s): return s.target.pilot_name if s.target is Ship else str(s.target))])
	foe.invulnerable = 0.0
	var hull0 := foe.hull
	for i in 1200:
		await physics_frame
		if foe.dead or foe.hull < hull0 * 0.5:
			break
	_check(foe.dead or foe.hull < hull0, "the wing hurts the target (%d -> %d)" % [int(hull0), int(foe.hull)])
	if not foe.dead:
		foe.take_damage(1e6, wing[0])
	await _wait(30)
	_check(w.wing.order_of(p)["order"] == "cover", "with the target down the wing covers again")
	# --- hold a spot while the leader flies off
	for i in wing.size():
		_put(wing[i], p.global_position + Vector2(-300, 250 * (i - 1)), p.velocity)
	p.target = null
	w.wing_command(p, "hold")
	var held: Vector2 = w.wing.hold_point(p)
	_put(p, p.global_position + Vector2(0, 7000), p.velocity)
	await _wait(420)
	var stayed := wing.all(func(s): return s.global_position.distance_to(w.wing.hold_point(p)) < Wing.HOLD_LEASH)
	_check(stayed, "Hold: the wing stays at the spot when the leader leaves (%s)" % [wing.map(func(s): return int(s.global_position.distance_to(w.wing.hold_point(p))))])
	_check(w.wing.hold_point(p).distance_to(held) < 2000.0, "the hold point moves only with its world")
	var luna: Planet = w.planet_by_name["Luna"]
	p.target = luna
	w.wing_command(p, "hold")
	_check(w.wing.order_of(p)["body"] == luna and w.wing.hold_point(p).distance_to(luna.global_position) < luna.radius + 600.0, "Hold with a world targeted: over that world")
	p.target = null
	# --- engage at will: hunts round the leader
	_put(p, _spot(w), w.planet_by_name["Earth"].velocity)
	for i in wing.size():
		_put(wing[i], p.global_position + Vector2(-300, 250 * (i - 1)), p.velocity)
	_keep_out(w, p)
	var prey := w.spawn_ship("Prey", 2, "Scout", "Prey", p.global_position + Vector2(1000, 300), p.velocity, 0)
	prey.get_node("AI").make_hauler()       # it keeps out of the fight
	prey.invulnerable = 1e9
	w.wing_command(p, "form")
	await _wait(120)
	_check(not wing.any(func(s): return s.target == prey), "in formation the wing leaves a ship that isn't shooting alone")
	w.wing_command(p, "engage")
	await _wait(120)
	_check(wing.any(func(s): return s.target == prey), "Engage at will: the wing hunts it")
	w.remove_ship(prey)
	# --- return to base, and back
	var hurt: Ship = wing[0]
	hurt.hull = hurt.hull_max * 0.4
	w.wing_command(p, "home")
	await _wait(30)
	var hai: AIPilot = hurt.get_node("AI")
	_check(hai.sent_home and hai.state == AIPilot.State.RETREAT, "Return to base: home for repairs")
	_check(w.wing.status(hurt).begins_with("home"), "the HUD says so (%s)" % w.wing.status(hurt))
	_put(hurt, hai.home.global_position + Vector2(hai.home.radius + 220.0, 0), hai.home.velocity)
	hurt.hull = hurt.hull_max
	hurt.shield = hurt.shield_max
	hurt.energy = hurt.energy_max
	hurt.fuel = hurt.fuel_max
	hurt.crew = hurt.crew_max
	await _wait(60)
	_check(not hai.sent_home and hai.state == AIPilot.State.ESCORT, "mended, it flies back to the wing")
	# --- the leader dies: the wing goes home and waits
	p.invulnerable = 0.0
	p.take_damage(1e7, null)
	await _wait(45)              # a think or two
	var gone_home := wing.all(func(s): return s.get_node("AI").escort == p and (s.dead or s.get_node("AI").sent_home))
	if not gone_home:
		for s in wing:
			var a: AIPilot = s.get_node("AI")
			print("  wingman %s state %d escort %s home %s dead %s landing %s" % [s.pilot_name, a.state, a.escort == p, a.sent_home, s.dead, s.is_landing()])
	_check(p.dead and gone_home, "with the leader down the wing goes home, still its wing")
	await _wait(int(World.RESPAWN_DELAY * 60) + 60)
	p.invulnerable = 1e9
	_check(not p.dead and w.wing.members(p).size() == wing.size(), "the leader relaunches and has its wing")
	# --- AI admirals
	var arceans := w.ships.filter(func(s): return s.race_id == 0 and not s.support and not s.is_station and s.get_node_or_null("AI") != null)
	for s in arceans:
		var ai: AIPilot = s.get_node("AI")
		if ai.called:
			ai.leave_wing()
		s.rank = mini(s.rank, 6)
		ai.ferry = false
		s.colonists = 0.0
	var admiral: Ship = arceans.filter(func(s): return not s.dead)[0]
	admiral.rank = 12
	w.wing._admirals()
	_check(w.wing.called(admiral).size() == 2, "an Arcean admiral calls up a wing of two (%d)" % w.wing.called(admiral).size())
	_check(w.wing.called(admiral).all(func(s): return s.rank < admiral.rank), "of pilots junior to it")
	# --- online and the save
	var rows := w.wing.pack()
	var mine := rows.filter(func(r): return r[1] == w.wing.order_of(p)["order"] and r[4].size() == wing.size())
	_check(rows.size() >= 2 and not mine.is_empty(), "the wings pack for the clients (%d wings)" % rows.size())
	var save := SaveGame.capture(w)
	var saved: Array = save["ships"].filter(func(r): return r.get("wing_of", "") == p.pilot_name)
	_check(saved.size() == wing.size(), "called pilots are saved with their leader (%d)" % saved.size())
	# --- the orders wheel
	var wheel: OrdersWheel = main.hud.wheel
	var ev := InputEventAction.new()
	ev.action = "wing_menu"
	ev.pressed = true
	root.push_input(ev, true)
	await process_frame
	_check(wheel.open, "the wheel key opens the wheel")
	var up := InputEventAction.new()
	up.action = "wing_menu"
	up.pressed = false
	root.push_input(up, true)
	await process_frame
	_check(wheel.open and wheel.tapped, "a tap leaves it open")
	var at: Vector2 = root.get_viewport().get_visible_rect().size * 0.5 + Vector2(0, -120)
	var mv := InputEventMouseMotion.new()
	mv.position = at
	root.push_input(mv, true)
	await process_frame
	_check(wheel.pick == 0, "pointing up picks Form up")
	var click := InputEventMouseButton.new()
	click.button_index = MOUSE_BUTTON_LEFT
	click.pressed = true
	click.position = at
	root.push_input(click, true)
	await process_frame
	_check(not wheel.open and w.wing.order_of(p)["order"] == "form", "a click on the slice orders Form up")
	var q := InputEventAction.new()
	q.action = "wing_form"
	q.pressed = true
	w.wing_command(p, "cover")
	root.push_input(q, true)
	await process_frame
	_check(w.wing.order_of(p)["order"] == "form", "the quick key orders Form up")
	# --- dismiss
	w.wing_command(p, "dismiss")
	_check(w.wing.called(p).is_empty() and wing.all(func(s): return s.get_node("AI").escort == null), "Dismiss sends the called pilots back to the empire")
	# --- a player's call comes before a computer admiral's
	var mates := w.ships.filter(func(s): return s.race_id == p.race_id and not s.dead and not s.is_player and not s.support and not s.is_station and s.get_node_or_null("AI") != null and s.get_node("AI").escort == null)
	var boss: Ship = mates[0]
	boss.rank = 12
	p.rank = 9
	w.wing.call_up(boss, 2)
	var taken := w.wing.called(boss)
	var busy := []
	for s in mates:
		if s != boss and not s in taken:
			s.get_node("AI").hauler = true
			busy.append(s)
	w.wing_command(p, "call")
	_check(w.wing.called(p).size() == taken.size() and taken.size() == 2 and w.wing.called(boss).is_empty(), "with nobody else free the player gets the admiral's called pilots (%d of %d)" % [w.wing.called(p).size(), taken.size()])
	for s in busy:
		s.get_node("AI").hauler = false
	print("wing_test %s" % ("FAILED" if failed else "passed"))
	quit(1 if failed else 0)
