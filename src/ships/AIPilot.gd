## AIPilot -- a computer pilot that flies like a person: it hunts within
## sensor range, leads its shots, breaks off when its shields are gone,
## goes home to recharge, and answers calls for help.  It boards ships it
## has disabled when its crew can take them, and a prize crew flies as
## the wingman of the pilot who captured the ship (`escort`).
class_name AIPilot
extends Node

enum State { PATROL, ATTACK, EVADE, RETREAT, ESCORT, BOARD }

## A boarding that would take longer than this is not worth trying.
const BOARD_PATIENCE := 20.0
## A prize keeps within this of the ship it escorts, and fights what
## comes inside it.
const ESCORT_LEASH := 1500.0

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
var escort: Ship = null            # prize crews fly with their captor
var board_rolls := {}              # target instance id -> whether we try to take it alive


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
	if escort != null and (not is_instance_valid(escort) or escort.dead or escort.race_id != ship.race_id):
		escort = null              # the captor is gone: the prize joins its new empire's patrols
		if state == State.ESCORT:
			state = State.PATROL
	# acquire
	var tgt := ship.target
	if tgt == null or not is_instance_valid(tgt) or (tgt is Ship and (tgt.dead or not tgt.visible_to(ship) or tgt.race_id == ship.race_id)):
		tgt = null
	# a friend is taking that one alive: leave it be
	if tgt is Ship and tgt.boarded_by != ship and tgt.claimed_by != ship and tgt.spoken_for(ship.race_id):
		tgt = null
	if escort != null:
		tgt = _escort_target(tgt)
	elif tgt == null or (randf() < 0.1 and state != State.BOARD):
		var best: Ship = world.nearest_enemy_ship(ship.global_position, ship.race_id, ship.sensor_range, ship)
		# answer a recent attacker first
		if ship.last_attacker != null and is_instance_valid(ship.last_attacker) and ship.last_attacker is Ship and not ship.last_attacker.dead \
				and Time.get_ticks_msec() / 1000.0 - ship.last_attacked_at < 6.0 and ship.last_attacker.race_id != ship.race_id:
			best = ship.last_attacker
		if best != null:
			tgt = best
	ship.target = tgt
	var weak := shield_frac < 0.15 and hull_frac < 0.55
	if escort != null and tgt == null and state != State.BOARD:
		state = State.ESCORT
	if state == State.ATTACK and _wants_to_board(tgt):
		state = State.BOARD
		tgt.claimed_by = ship
	match state:
		State.ESCORT:
			if tgt != null:
				state = State.ATTACK
				reaction = (1.0 - skill) * 1.2
		State.BOARD:
			_think_board(tgt)
		State.PATROL:
			if tgt != null:
				state = State.ATTACK
				reaction = (1.0 - skill) * 1.2
			elif ship.global_position.distance_to(patrol_point) < 300.0:
				patrol_point = _random_patrol_point()
			elif (hull_frac < 0.6 or ship.energy < ship.energy_max * 0.2 or ship.fuel_fraction() < 0.15 or ship.crew_fraction() < 0.4) and home != null:
				state = State.RETREAT
		State.ATTACK:
			if tgt == null:
				state = State.PATROL
			elif (weak or ship.fuel_fraction() < 0.1) and randf() < 0.8:
				state = State.RETREAT if home != null and escort == null else State.EVADE
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
			elif hull_frac > 0.9 and shield_frac > 0.8 and ship.energy > ship.energy_max * 0.6 and ship.fuel_fraction() > 0.6 and ship.crew_fraction() > 0.8:
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


## A prize crew fights near its captor: the captor's own target first,
## else whatever comes within the leash.
func _escort_target(current: Node2D) -> Node2D:
	var lead: Node2D = escort.target
	if lead is Ship and is_instance_valid(lead) and not lead.dead and lead.race_id != ship.race_id and lead.visible_to(ship) \
			and lead.global_position.distance_to(escort.global_position) < ESCORT_LEASH:
		return lead
	if current != null and current.global_position.distance_to(escort.global_position) < ESCORT_LEASH:
		return current
	var near: Ship = world.nearest_enemy_ship(escort.global_position, ship.race_id, ESCORT_LEASH * 0.8, ship)
	return near


## Board this one instead of finishing it?  Decided once per target, and
## only when our crew would win quickly.
func _wants_to_board(tgt: Node2D) -> bool:
	if not tgt is Ship or ship.is_station or not tgt.is_disabled() or tgt.is_station:
		return false
	var why := Capture.why_not(ship, tgt)
	if why != "" and why != "far":
		return false
	var key := tgt.get_instance_id()
	if not board_rolls.has(key):
		board_rolls[key] = randf() < 0.55 + 0.4 * aggression
	if not board_rolls[key]:
		return false
	return Capture.board_estimate(ship, tgt) < BOARD_PATIENCE


func _think_board(tgt: Node2D) -> void:
	if not tgt is Ship or tgt.dead or tgt.race_id == ship.race_id:
		state = State.PATROL if escort == null else State.ESCORT
		return
	if ship.boarding == tgt:
		return
	var why := Capture.why_not(ship, tgt)
	if why == "":
		if not world.capture.try_board(ship, tgt):
			board_rolls[tgt.get_instance_id()] = false
			state = State.ATTACK
	elif why != "far":
		# it got its engines back, someone beat us to it, or we were
		# thrown back: finish it with guns instead
		board_rolls[tgt.get_instance_id()] = false
		state = State.ATTACK


## Fly to a point that moves with `vel`, arriving at rest against it:
## burn along the difference between the velocity we want relative to it
## and the one we have.
func _close_on(point: Vector2, vel: Vector2) -> void:
	var to := point - ship.global_position
	var dist := to.length()
	var rel := ship.velocity - vel
	var a := maxf(ship.accel * 0.6, 20.0)
	var want := to.normalized() * minf(sqrt(2.0 * a * dist), ship.max_speed) if dist > 5.0 else Vector2.ZERO
	var dv := want - rel
	var i := ship.inputs
	if dv.length() < 8.0:
		i["thrust"] = 0.0
		i["strafe"] = 0.0
		i["brake"] = false
		i["turn"] = clampf(wrapf(to.angle() - ship.rotation, -PI, PI) * 2.0, -1.0, 1.0)
		return
	var diff := wrapf(dv.angle() - ship.rotation, -PI, PI)
	i["turn"] = clampf(diff * 2.5, -1.0, 1.0)
	i["thrust"] = clampf(dv.length() / 60.0, 0.2, 1.0) if absf(diff) < 0.6 else 0.0
	i["strafe"] = clampf(dv.dot(Vector2.RIGHT.rotated(ship.rotation).orthogonal()) / 60.0, -1.0, 1.0)
	i["brake"] = false


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
		State.ESCORT:
			if escort == null:
				return
			# hold station off the captor's quarter, a little ahead of its motion
			var side := 1.0 if ship.get_instance_id() % 2 == 0 else -1.0
			var off := Vector2(-1.0, side * 0.9).normalized().rotated(escort.rotation) * (escort.size + ship.size + 140.0)
			var spot: Vector2 = escort.global_position + off + escort.velocity * 0.8
			_avoid_planets(delta)
			_steer_to(spot, delta, ship.global_position.distance_to(spot) < 900.0)
			if ship.qdrv > 0.0 and ship.global_position.distance_to(spot) > 5000.0 and not ship.warping and absf(i["turn"]) < 0.1:
				ship.start_warp()
		State.BOARD:
			if tgt == null or not is_instance_valid(tgt):
				return
			i["fire_primary"] = false
			i["fire_secondary"] = false
			i["fire_beam"] = false
			i["fire_missile"] = false
			if ship.boarding == tgt:
				# locked: the tractor does the rest; just hold still beside it
				i["thrust"] = 0.0
				i["strafe"] = 0.0
				i["brake"] = false
				i["turn"] = clampf(wrapf((tgt.global_position - ship.global_position).angle() - ship.rotation, -PI, PI) * 2.0, -1.0, 1.0)
			else:
				var to: Vector2 = tgt.global_position - ship.global_position
				var stand: Vector2 = to.normalized() * (ship.size + tgt.size + 60.0)
				_close_on(tgt.global_position - stand, tgt.velocity)
			_avoid_planets(delta)
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
