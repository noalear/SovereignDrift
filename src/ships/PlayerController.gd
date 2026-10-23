## PlayerController -- turns keyboard and mouse input into ship inputs.
class_name PlayerController
extends Node

var ship: Ship
var world: Node
var mouse_steer := false


func setup(p_ship: Ship, p_world: Node) -> void:
	ship = p_ship
	world = p_world
	ship.flight_assist = Settings.flight_assist


func _unhandled_input(event: InputEvent) -> void:
	if ship == null or ship.dead or world.paused or ship.docked:
		return
	if event.is_action_pressed("cloak"):
		if Net.is_client():
			Net.client_command("cloak")
		else:
			ship.set_cloak(not ship.cloaked)
	elif event.is_action_pressed("target_next"):
		world.cycle_target(ship)
	elif event.is_action_pressed("target_nearest"):
		ship.target = world.nearest_enemy_ship(ship.global_position, ship.race_id, ship.sensor_range * 1.5, ship)
		if ship.target == null:
			# fall back on what the rest of the fleet can see
			var known: Ship = world.nearest_known_enemy(ship)
			if known != null:
				ship.target = known
				world.notify(ship, "Fleet contact: %s, range %d. Follow the arrow." % [known.pilot_name, int(ship.global_position.distance_to(known.global_position))])
			else:
				world.notify(ship, "No enemy contacts. Tab shows the system.")
	elif event.is_action_pressed("nav_next"):
		world.cycle_nav(ship)
	elif event.is_action_pressed("autopilot"):
		if ship.nav_target == null:
			world.cycle_nav(ship)
		ship.autopilot = not ship.autopilot
		world.notify(ship, ("Autopilot to %s." % ship.nav_target.body_name) if ship.autopilot and ship.nav_target else "Autopilot off.")
	elif event.is_action_pressed("warp"):
		if Net.is_client():
			Net.client_command("warp")
		elif ship.warping:
			ship.end_warp()
		else:
			ship.start_warp()   # says why when it cannot
	elif event.is_action_pressed("refit"):
		if Net.is_client():
			Net.client_command("refit")
		else:
			world.try_refit(ship)
	elif event.is_action_pressed("flight_assist"):
		ship.flight_assist = not ship.flight_assist
		Settings.flight_assist = ship.flight_assist
		Settings.save_settings()
		world.notify(ship, "Flight assist on: the thrusters hold your turn." if ship.flight_assist else "Flight assist off: you spin until you stop yourself.")
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT and event.shift_pressed:
		var pos: Vector2 = world.get_global_mouse_position()
		var s: Ship = world.ship_at(pos, 40.0 / world.camera.zoom.x)
		if s != null and s != ship:
			ship.target = s
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_MIDDLE:
		var pos: Vector2 = world.get_global_mouse_position()
		var s: Ship = world.ship_at(pos, 40.0 / world.camera.zoom.x)
		if s != null and s != ship:
			ship.target = s


func _physics_process(_delta: float) -> void:
	if ship == null or ship.dead:
		return
	var i := ship.inputs
	if world.paused or ship.docked:
		for k in i.keys():
			i[k] = false if typeof(i[k]) == TYPE_BOOL else 0.0
		return
	var manual := false
	var thrust := Input.get_action_strength("thrust") - Input.get_action_strength("reverse")
	var turn := Input.get_action_strength("turn_right") - Input.get_action_strength("turn_left")
	var strafe := Input.get_action_strength("strafe_right") - Input.get_action_strength("strafe_left")
	if absf(thrust) > 0.0 or absf(turn) > 0.0 or absf(strafe) > 0.0 or Input.is_action_pressed("brake"):
		manual = true
	if manual and ship.autopilot:
		ship.autopilot = false
		world.notify(ship, "Manual control.")
	if manual and ship.warping and (absf(turn) > 0.0 or thrust < 0.0):
		ship.end_warp()
		world.notify(ship, "Warp ended: turning or reversing drops you out.")
	if not ship.autopilot:
		i["thrust"] = thrust
		i["turn"] = turn
		i["strafe"] = strafe
		i["brake"] = Input.is_action_pressed("brake")
	i["boost"] = Input.is_action_pressed("boost")
	# turret aim: right stick when deflected, else the mouse
	var stick := Vector2(Input.get_action_strength("aim_right") - Input.get_action_strength("aim_left"),
		Input.get_action_strength("aim_down") - Input.get_action_strength("aim_up"))
	if stick.length() > 0.3:
		ship.aim_point = ship.global_position + stick.normalized() * 1000.0
	elif Settings.mouse_aim:
		ship.aim_point = world.get_global_mouse_position()
	else:
		ship.aim_point = Vector2.INF
	i["fire_primary"] = Input.is_action_pressed("fire_primary") and not Input.is_key_pressed(KEY_SHIFT)
	i["fire_secondary"] = Input.is_action_pressed("fire_secondary")
	i["fire_missile"] = Input.is_action_pressed("fire_missile")
	i["fire_mine"] = Input.is_action_pressed("fire_mine")
	i["fire_beam"] = Input.is_action_pressed("fire_beam")
