## AIPilot -- a computer pilot that flies like a person: it hunts within
## sensor range, leads its shots, breaks off when its shields are gone,
## goes home to recharge, and answers calls for help.
class_name AIPilot
extends Node

enum State { PATROL, ATTACK, EVADE, RETREAT, ESCORT }

var ship: Ship
var world: Node
var state := State.PATROL
var think_timer := 0.0
var patrol_point := Vector2.ZERO
var home: Planet = null
var aggression := 0.6
var skill := 0.6
var evade_dir := 1.0
var evade_timer := 0.0
var fire_hold := 0.0
var reaction := 0.0


func setup(p_ship: Ship, p_world: Node, p_home: Planet, level: int) -> void:
	ship = p_ship
	world = p_world
	home = p_home
	skill = clampf(0.35 + level * 0.045, 0.35, 0.95)
	aggression = clampf(0.4 + level * 0.04 + randf() * 0.2, 0.3, 1.0)
	patrol_point = _random_patrol_point()
	think_timer = randf()


func _random_patrol_point() -> Vector2:
	var centre := home.global_position if home != null else Vector2.ZERO
	# patrol near home, or roam toward an enemy planet when aggressive
	if randf() < aggression * 0.5:
		var targets: Array = world.enemy_planets(ship.race_id)
		if not targets.is_empty():
			var p: Planet = targets[randi() % targets.size()]
			return p.global_position + Vector2(randf_range(-900, 900), randf_range(-900, 900))
	return centre + Vector2(randf_range(-1800, 1800), randf_range(-1800, 1800))


func _physics_process(delta: float) -> void:
	if ship == null or ship.dead:
		return
	think_timer -= delta
	if think_timer <= 0.0:
		think_timer = 0.25 + randf() * 0.25
		_think()
	_act(delta)


func _think() -> void:
	var i := ship.inputs
	i["fire_primary"] = false
	i["fire_secondary"] = false
	i["fire_missile"] = false
	i["fire_mine"] = false
	i["fire_beam"] = false
	var shield_frac := ship.shield / maxf(ship.shield_max, 1.0)
	var hull_frac := ship.hull / maxf(ship.hull_max, 1.0)
	# acquire
	var tgt := ship.target
	if tgt == null or not is_instance_valid(tgt) or (tgt is Ship and (tgt.dead or not tgt.visible_to(ship))):
		tgt = null
	if tgt == null or randf() < 0.1:
		var best: Ship = world.nearest_enemy_ship(ship.global_position, ship.race_id, ship.sensor_range, ship)
		# answer a recent attacker first
		if ship.last_attacker != null and is_instance_valid(ship.last_attacker) and ship.last_attacker is Ship and not ship.last_attacker.dead \
				and Time.get_ticks_msec() / 1000.0 - ship.last_attacked_at < 6.0 and ship.last_attacker.race_id != ship.race_id:
			best = ship.last_attacker
		if best != null:
			tgt = best
	ship.target = tgt
	var weak := shield_frac < 0.15 and hull_frac < 0.55
	match state:
		State.PATROL:
			if tgt != null:
				state = State.ATTACK
				reaction = (1.0 - skill) * 1.2
			elif ship.global_position.distance_to(patrol_point) < 300.0:
				patrol_point = _random_patrol_point()
			elif (hull_frac < 0.6 or ship.energy < ship.energy_max * 0.2) and home != null:
				state = State.RETREAT
		State.ATTACK:
			if tgt == null:
				state = State.PATROL
			elif weak and randf() < 0.8:
				state = State.RETREAT if home != null else State.EVADE
				evade_timer = 3.0
			elif randf() < 0.08 * (1.0 - aggression):
				state = State.EVADE
				evade_timer = 1.0 + randf() * 1.5
				evade_dir = 1.0 if randf() < 0.5 else -1.0
		State.EVADE:
			evade_timer -= 0.35
			if evade_timer <= 0.0:
				state = State.ATTACK if tgt != null and not weak else State.PATROL
		State.RETREAT:
			if home == null:
				state = State.PATROL
			elif hull_frac > 0.9 and shield_frac > 0.8 and ship.energy > ship.energy_max * 0.6:
				state = State.PATROL
				patrol_point = _random_patrol_point()
			elif tgt != null and tgt.global_position.distance_to(ship.global_position) < 500.0 and hull_frac > 0.4:
				# cornered: fight back
				state = State.ATTACK
	# cloak when sneaking up, uncloak to fight
	if ship.can_cloak:
		if state == State.ATTACK and tgt != null and tgt.global_position.distance_to(ship.global_position) > 700.0 and ship.energy > ship.energy_max * 0.5:
			ship.set_cloak(true)
		elif state == State.RETREAT and ship.energy > ship.energy_max * 0.4:
			ship.set_cloak(true)
		elif state != State.ATTACK or ship.energy < ship.energy_max * 0.25:
			ship.set_cloak(false)


func _steer_to(point: Vector2, delta: float, arrive: bool) -> float:
	var to := point - ship.global_position
	var desired := to.angle()
	var diff := wrapf(desired - ship.rotation, -PI, PI)
	ship.inputs["turn"] = clampf(diff * 2.5, -1.0, 1.0)
	var dist := to.length()
	var closing := ship.velocity.dot(to.normalized())
	var stop_dist := closing * closing / maxf(2.0 * ship.accel * 0.7, 1.0)
	if arrive and stop_dist > dist - 120.0:
		ship.inputs["thrust"] = 0.0
		ship.inputs["brake"] = true
	else:
		ship.inputs["thrust"] = 1.0 if absf(diff) < 0.9 else 0.3
		ship.inputs["brake"] = false
	# drift correction: thrust against sideways velocity
	var side := ship.velocity.dot(to.normalized().orthogonal())
	ship.inputs["strafe"] = clampf(-side / 120.0, -1.0, 1.0) * (1.0 if absf(diff) < 1.2 else 0.0)
	return diff


func _act(delta: float) -> void:
	var i := ship.inputs
	i["boost"] = false
	if reaction > 0.0:
		reaction -= delta
	var tgt := ship.target
	match state:
		State.PATROL:
			_avoid_planets(delta)
			_steer_to(patrol_point, delta, true)
			i["fire_beam"] = false
			if ship.qdrv > 0.0 and ship.global_position.distance_to(patrol_point) > 5000.0 and not ship.warping and absf(i["turn"]) < 0.1:
				ship.start_warp()
		State.ATTACK:
			if tgt == null or not is_instance_valid(tgt):
				return
			var to: Vector2 = tgt.global_position - ship.global_position
			var dist := to.length()
			var tv: Vector2 = tgt.velocity if "velocity" in tgt else Vector2.ZERO
			# lead for the primary weapon
			var mv := 0.6 * GameData.LIGHT_SPEED
			var tt := dist / maxf(mv + 1.0, 1.0)
			var aim: Vector2 = tgt.global_position + (tv - ship.velocity) * tt * (0.5 + 0.5 * skill)
			var preferred := 260.0 + ship.size * 3.0
			if ship.is_station:
				i["turn"] = clampf(wrapf((aim - ship.global_position).angle() - ship.rotation, -PI, PI) * 2.0, -1.0, 1.0)
				i["thrust"] = 0.0
			elif dist > preferred:
				var diff := _steer_to(aim, delta, false)
				i["boost"] = dist > 900.0 and absf(diff) < 0.3 and ship.energy > ship.energy_max * 0.4
			else:
				# strafe around the target at close range
				var diff := wrapf((aim - ship.global_position).angle() - ship.rotation, -PI, PI)
				i["turn"] = clampf(diff * 3.0, -1.0, 1.0)
				i["thrust"] = 0.4 if dist > preferred * 0.6 else -0.5
				i["strafe"] = evade_dir * 0.8
			_avoid_planets(delta)
			var diff2 := wrapf((aim - ship.global_position).angle() - ship.rotation, -PI, PI)
			var in_range := dist < 1000.0
			var aligned := absf(diff2) < (0.12 + 0.25 * (1.0 - skill))
			if reaction <= 0.0 and in_range:
				fire_hold -= delta
				i["fire_primary"] = aligned and dist < 820.0
				i["fire_secondary"] = aligned and dist < 650.0 and randf() < 0.8
				i["fire_beam"] = dist < 420.0 and absf(diff2) < 1.8
				i["fire_missile"] = dist < 1200.0 and dist > 300.0 and fmod(Time.get_ticks_msec() / 1000.0, 3.0) < 0.2
				i["fire_mine"] = dist < 220.0 and randf() < 0.02
			else:
				i["fire_primary"] = false
		State.EVADE:
			if tgt != null and is_instance_valid(tgt):
				var away: Vector2 = ship.global_position - tgt.global_position
				var dir := away.rotated(evade_dir * 0.9).angle()
				i["turn"] = clampf(wrapf(dir - ship.rotation, -PI, PI) * 3.0, -1.0, 1.0)
				i["thrust"] = 1.0
				i["strafe"] = evade_dir
				i["boost"] = ship.energy > ship.energy_max * 0.3
				i["fire_mine"] = randf() < 0.05
				i["fire_beam"] = false
			_avoid_planets(delta)
		State.RETREAT:
			if home != null:
				var p := home.global_position + (ship.global_position - home.global_position).normalized() * (home.radius + 220.0)
				_steer_to(p, delta, true)
				i["boost"] = ship.energy > ship.energy_max * 0.5 and ship.global_position.distance_to(p) > 800.0
				if ship.qdrv > 0.0 and ship.global_position.distance_to(p) > 4000.0 and not ship.warping and absf(i["turn"]) < 0.1 and ship.energy > 500.0:
					ship.start_warp()
			_avoid_planets(delta)


## Steer away from planets and the sun we are about to hit.
func _avoid_planets(_delta: float) -> void:
	var p: Planet = world.nearest_body(ship.global_position)
	if p == null:
		return
	var to := p.global_position - ship.global_position
	var ahead := ship.velocity.normalized()
	var danger := p.radius + 160.0 + (600.0 if p.look == "Sun" else 0.0)
	var closing := ship.velocity.dot(to.normalized())
	if closing > 0.0 and to.length() < danger + closing * 1.5:
		# the side we are already offset to
		var side: float = sign(ahead.cross(to.normalized()))
		if side == 0.0:
			side = 1.0
		var away := to.normalized().rotated(-side * PI * 0.5)
		ship.inputs["turn"] = clampf(wrapf(away.angle() - ship.rotation, -PI, PI) * 3.0, -1.0, 1.0)
		ship.inputs["thrust"] = 1.0
		ship.inputs["strafe"] = -side
		if to.length() < danger:
			ship.inputs["brake"] = false
