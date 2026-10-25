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
	if event.is_action_pressed("land"):
		world.try_land(ship)
		return
	# 1 and 2 also pick weapon groups: they move colonists only on the ground
	if ship.is_landing() and event.is_action_pressed("colonists_board"):
		world.colonist_command(ship, true)
		return
	if ship.is_landing() and event.is_action_pressed("colonists_unload"):
		world.colonist_command(ship, false)
		return
	if ship.is_landing():
		return                          # on the way down or on the ground: B lifts off
	if event.is_action_pressed("cloak"):
		if Net.is_client():
			Net.client_command("cloak")
		else:
			ship.set_cloak(not ship.cloaked)
	elif event.is_action_pressed("target_next"):
		world.cycle_target(ship)
	elif event.is_action_pressed("target_nearest"):
		ship.target = world.nearest_enemy_ship(ship.global_position, ship.team(), ship.sensor_range * 1.5, ship)
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
	elif event.is_action_pressed("trade"):
		if Net.is_active():
			Net.request_trade(ship, ship.target)
		else:
			world.notify(ship, "Trading is between online pilots.")
	elif _group_key(event) > 0:
		select_group(_group_key(event))
	elif event.is_action_pressed("weapon_group_next"):
		select_group(_next_group())
	elif event.is_action_pressed("board"):
		world.try_board(ship)
	elif event.is_action_pressed("flight_assist"):
		ship.flight_assist = not ship.flight_assist
		Settings.flight_assist = ship.flight_assist
		Settings.save_settings()
		world.notify(ship, "Flight assist on: the thrusters hold your turn." if ship.flight_assist else "Flight assist off: you spin until you stop yourself.")
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT and event.shift_pressed:
		# a ship under the cursor becomes the target; a world opens its info
		# card (again closes it); empty space closes the card
		var pos: Vector2 = world.get_global_mouse_position()
		var s: Ship = world.ship_at(pos, 40.0 / world.camera.zoom.x)
		if s != null and s != ship:
			ship.target = s
		else:
			var pl: Planet = world.planet_at(pos, 30.0 / world.camera.zoom.x)
			world.inspected_planet = null if pl == world.inspected_planet else pl
			if pl != null:
				Sfx.play_ui("click", 0.4)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_MIDDLE:
		var pos: Vector2 = world.get_global_mouse_position()
		var s: Ship = world.ship_at(pos, 40.0 / world.camera.zoom.x)
		if s != null and s != ship:
			ship.target = s


func _physics_process(_delta: float) -> void:
	if ship == null or ship.dead:
		return
	var i := ship.inputs
	if world.paused or ship.docked or ship.is_landing():
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
	# weapon groups: Fire shoots the selected group, Alt-fire its own group
	var fire := Input.is_action_pressed("fire_primary")
	if fire and Input.is_key_pressed(KEY_SHIFT) and Input.is_mouse_button_pressed(MOUSE_BUTTON_LEFT):
		fire = _down_without_mouse("fire_primary")   # Shift+click picks a target
	var alt := Input.is_action_pressed("fire_secondary")
	for k in Ship.GROUP_INPUTS:
		i[k] = false
	i["fire_beam"] = false
	if fire:
		i[Ship.GROUP_INPUTS[clampi(ship.weapon_group, 1, 4) - 1]] = true
	if alt:
		i[Ship.GROUP_INPUTS[clampi(Settings.alt_fire_group, 1, 4) - 1]] = true


## Which group key this event is, 1-4, or 0.  While landed the number
## keys move colonists instead.
func _group_key(event: InputEvent) -> int:
	if "landing" in ship and int(ship.landing) != 0:
		return 0
	for g in range(1, GameData.WEAPON_GROUPS + 1):
		if event.is_action_pressed("weapon_group_%d" % g):
			return g
	return 0


func _next_group() -> int:
	for step in range(1, GameData.WEAPON_GROUPS + 1):
		var g := (ship.weapon_group - 1 + step) % GameData.WEAPON_GROUPS + 1
		if not ship.group_weapons(g).is_empty():
			return g
	return ship.weapon_group


func select_group(g: int) -> void:
	ship.weapon_group = g
	var names := ship.group_weapons(g)
	if names.is_empty():
		world.notify(ship, "Group %d is empty. Set weapon groups in the shipyard (%s at your starbase)." % [g, Keybinds.short_text("refit")])
	else:
		world.notify(ship, "Group %d: %s." % [g, ", ".join(names)])
	Sfx.play_ui("click", 0.3)


## Is the action held by anything but the mouse?
func _down_without_mouse(action: String) -> bool:
	for ev in InputMap.action_get_events(action):
		if ev is InputEventKey and Input.is_physical_key_pressed(ev.physical_keycode):
			return true
		if ev is InputEventJoypadButton and Input.is_joy_button_pressed(0, ev.button_index):
			return true
		if ev is InputEventJoypadMotion and Input.get_joy_axis(0, ev.axis) * signf(ev.axis_value) > 0.5:
			return true
	return false
