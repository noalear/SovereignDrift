## PlayerController -- turns keyboard and mouse input into ship inputs.
class_name PlayerController
extends Node

var ship: Ship
var world: Node
var mouse_steer := false
## Mouse buttons fire only when the press reached the game: a click that a
## menu or panel took never starts the guns.  Keys and the pad are polled.
var mouse_fire := {"fire_primary": false, "fire_secondary": false}


func setup(p_ship: Ship, p_world: Node) -> void:
	ship = p_ship
	world = p_world
	ship.flight_assist = Settings.flight_assist


## Releases are watched here, before the menus, so a button let go over a
## panel still stops the guns.
func _input(event: InputEvent) -> void:
	if event is InputEventMouseButton and not event.pressed:
		for action in mouse_fire:
			if event.is_action(action):
				mouse_fire[action] = false


func _notification(what: int) -> void:
	if what == NOTIFICATION_APPLICATION_FOCUS_OUT:
		for action in mouse_fire:
			mouse_fire[action] = false


func _unhandled_input(event: InputEvent) -> void:
	if ship == null or ship.dead or world.paused or world.menu_open or ship.docked:
		return
	if event is InputEventMouseButton and event.pressed and not ship.is_landing() \
			and not (event.button_index == MOUSE_BUTTON_LEFT and event.shift_pressed):   # Shift+click picks a target
		for action in mouse_fire:
			if event.is_action(action):
				mouse_fire[action] = true
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
		# the jammer works on the target, so the target travels with the command
		if Net.is_client():
			Net.client_command("jam:%d" % (ship.target.net_id if ship.target is Ship and not ship.jamming else 0))
		else:
			ship.set_jam(not ship.jamming)
	elif event.is_action_pressed("ping"):
		if Net.is_client():
			Net.client_command("ping")
			ship.sensors.ping()         # the host pings too; this is our own picture
		else:
			ship.ping()
	elif event.is_action_pressed("silent_running"):
		if Net.is_client():
			Net.client_command("silent")
		else:
			ship.set_silent(not ship.sensors.emcon)
	elif event.is_action_pressed("pdc_toggle"):
		if Net.is_client():
			Net.client_command("pdc")
			ship.pdc.enabled = not ship.pdc.enabled   # our own picture; the host flips its copy
		else:
			ship.set_pdc(not ship.pdc.enabled)
	elif event.is_action_pressed("shield_focus"):
		if Net.is_client():
			Net.client_command("shield_focus")
		ship.cycle_shield_focus()
	elif event.is_action_pressed("shields_toggle"):
		if Net.is_client():
			Net.client_command("shields")
		ship.set_shields_raised(not ship.shields_raised)
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
	if world.paused or world.menu_open or ship.docked or ship.is_landing():
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
	var fire := _fire_held("fire_primary")
	var alt := _fire_held("fire_secondary")
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


## Is the fire action held by a key or the pad, or by a mouse button
## whose press went to the game rather than a menu?
func _fire_held(action: String) -> bool:
	if not Input.is_action_pressed(action):
		mouse_fire[action] = false    # a release we never saw
		return false
	if mouse_fire[action] or _down_without_mouse(action):
		return true
	# held by neither a key we know nor the mouse: another pad, a script
	for ev in InputMap.action_get_events(action):
		if ev is InputEventMouseButton and Input.is_mouse_button_pressed(ev.button_index):
			return false
	return true


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
