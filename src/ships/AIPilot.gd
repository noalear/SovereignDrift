## AIPilot -- a computer pilot that flies like a person: it hunts within
## sensor range, leads its shots, breaks off when its shields are gone,
## goes home to recharge, and answers calls for help.  It boards ships it
## has disabled when its crew can take them, and a prize crew flies as
## the wingman of the pilot who captured the ship (`escort`).  Pilots on
## colony duty ferry colonists (land, board, fly, land again); pilots on
## attack duty knock out enemy colonies' defences for them.
class_name AIPilot
extends Node

enum State { PATROL, ATTACK, EVADE, RETREAT, ESCORT, BOARD, FERRY }

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
var ferry := false               # carries colonists for the empire
var bombard := false             # knocks out enemy colonies' defences
var job_planet: Planet = null
var job := ""                    # board | settle | invade | reinforce
var ground_time := 0.0
var ground_order := false        # already told the colonists what to do on this landing
var last_drop: Planet = null     # where we last put colonists down
var plan_timer := 0.0
var hauler := false              # a supply ship: carries the empire's goods, keeps out of fights
var haul_res := ""               # what the run is for
var haul_to: Planet = null       # where the load goes
var duel := false                # a free-for-all: hunt around the arena, never go home to mend


func setup(p_ship: Ship, p_world: Node, p_home: Planet, level: int) -> void:
	ship = p_ship
	world = p_world
	home = p_home
	skill = clampf(0.35 + level * 0.045, 0.35, 0.95)
	aggression = clampf(0.4 + level * 0.04 + randf() * 0.2, 0.3, 1.0)
	patrol_point = _random_patrol_point()
	think_timer = randf()
	var duty: String = GameData.RANKS[clampi(level, 0, GameData.RANKS.size() - 1)]["duty"]
	ferry = duty == "colonize" and not ship.is_station and ship.race_id <= 2 and ship.cabin > 0.0
	bombard = duty in ["attack", "harass"] and not ship.is_station and ship.race_id <= 2


func _random_patrol_point() -> Vector2:
	var centre := home.global_position if home != null else Vector2.ZERO
	# patrol near home, or roam toward an enemy planet when aggressive
	if duel:
		return centre + Vector2(randf_range(-1200, 1200), randf_range(-1200, 1200))
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
	if not ship.is_landing():
		_act(delta)


func _think() -> void:
	if ship.is_landing():
		_think_on_ground()
		return
	var i := ship.inputs
	i["fire_primary"] = false
	i["fire_secondary"] = false
	i["fire_missile"] = false
	i["fire_mine"] = false
	i["fire_beam"] = false
	if hauler:
		_think_hauler()
		return
	var shield_frac := ship.shield / maxf(ship.shield_max, 1.0)
	var hull_frac := ship.hull / maxf(ship.hull_max, 1.0)
	if escort != null and (not is_instance_valid(escort) or escort.dead or escort.race_id != ship.race_id):
		escort = null              # the captor is gone: the prize joins its new empire's patrols
		if state == State.ESCORT:
			state = State.PATROL
	# acquire
	var tgt := ship.target
	if tgt == null or not is_instance_valid(tgt) or (tgt is Ship and (tgt.dead or not tgt.visible_to(ship) or tgt.team() == ship.team())):
		tgt = null
	elif tgt is Planet and (not tgt.is_colonised() or tgt.owner_race == ship.race_id or tgt.is_defenceless()):
		tgt = null                      # knocked out: leave it for the colonists
	# a friend is taking that one alive: leave it be
	if tgt is Ship and tgt.boarded_by != ship and tgt.claimed_by != ship and tgt.spoken_for(ship.team()):
		tgt = null
	if escort != null:
		tgt = _escort_target(tgt)
	elif tgt == null or (randf() < 0.1 and state != State.BOARD):
		var best: Ship = world.nearest_enemy_ship(ship.global_position, ship.team(), ship.sensor_range, ship)
		if best == null and duel:
			best = world.nearest_known_enemy(ship)   # the arena is small: everyone in it is fair game
		# answer a recent attacker first
		if ship.last_attacker != null and is_instance_valid(ship.last_attacker) and ship.last_attacker is Ship and not ship.last_attacker.dead \
				and Time.get_ticks_msec() / 1000.0 - ship.last_attacked_at < 6.0 and ship.last_attacker.team() != ship.team():
			best = ship.last_attacker
		if best != null:
			tgt = best
	# attack duty: with no ship about, go after an enemy colony's defences
	if tgt == null and bombard and state == State.PATROL:
		tgt = _colony_to_bombard()
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
			elif ferry and _plan_ferry():
				state = State.FERRY
			elif ship.global_position.distance_to(patrol_point) < 300.0:
				patrol_point = _random_patrol_point()
			elif (hull_frac < 0.6 or ship.energy < ship.energy_max * 0.2 or ship.fuel_fraction() < 0.15 or ship.crew_fraction() < 0.4) and home != null and not duel:
				state = State.RETREAT
		State.ATTACK:
			if tgt == null:
				state = State.PATROL
			elif (weak or ship.fuel_fraction() < 0.1) and randf() < 0.8:
				state = State.RETREAT if home != null and escort == null and not duel else State.EVADE
				evade_timer = 3.0
			elif randf() < 0.08 * (1.0 - aggression):
				state = State.EVADE
				evade_timer = 1.0 + randf() * 1.5
				evade_dir = 1.0 if randf() < 0.5 else -1.0
		State.EVADE:
			evade_timer -= 0.35
			if evade_timer <= 0.0:
				state = State.ATTACK if tgt != null and not weak else State.PATROL
		State.FERRY:
			if tgt != null and tgt is Ship and tgt.global_position.distance_to(ship.global_position) < 700.0 \
					and Time.get_ticks_msec() / 1000.0 - ship.last_attacked_at < 4.0:
				state = State.ATTACK     # jumped on the way: fight, then carry on
			elif not _job_still_good():
				state = State.FERRY if _plan_ferry() else State.PATROL
			elif (hull_frac < 0.35 or ship.fuel_fraction() < 0.1) and home != null and ship.colonists <= 0.0:
				state = State.RETREAT
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
	if lead is Ship and is_instance_valid(lead) and not lead.dead and lead.team() != ship.team() and lead.visible_to(ship) \
			and lead.global_position.distance_to(escort.global_position) < ESCORT_LEASH:
		return lead
	if current != null and current.global_position.distance_to(escort.global_position) < ESCORT_LEASH:
		return current
	var near: Ship = world.nearest_enemy_ship(escort.global_position, ship.team(), ESCORT_LEASH * 0.8, ship)
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
	if not tgt is Ship or tgt.dead or tgt.team() == ship.team():
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
			# against a colony, distances count from its surface
			var dist: float = to.length() - (tgt.radius if tgt is Planet else 0.0)
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
				i["fire_mine"] = _mine_ok(tgt, dist) and randf() < 0.02
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
				i["fire_mine"] = _mine_ok(tgt, away.length()) and randf() < 0.05
				i["fire_beam"] = false
			_avoid_planets(delta)
		State.FERRY:
			_fly_to_job(delta)
		State.RETREAT:
			if home != null:
				var p := home.global_position + (ship.global_position - home.global_position).normalized() * (home.radius + 220.0)
				_steer_to(p, delta, true)
				i["boost"] = ship.energy > ship.energy_max * 0.5 and ship.global_position.distance_to(p) > 800.0
				if ship.qdrv > 0.0 and ship.global_position.distance_to(p) > 4000.0 and not ship.warping and absf(i["turn"]) < 0.1 and ship.energy > 500.0:
					ship.start_warp()
			_avoid_planets(delta)


# ------------------------------------------------------------ colonists

## Pick the next colonist run.  With an empty cabin: a friendly colony that
## can spare people.  With colonists aboard: an enemy colony whose defences
## are down and that they can beat, else an empty world, else a thin
## friendly colony.
func _plan_ferry() -> bool:
	plan_timer -= 0.35
	if plan_timer > 0.0 and job_planet == null:
		return false
	plan_timer = 3.0
	job_planet = null
	job = ""
	var best_score := 0.0
	var pos := ship.global_position
	if ship.colonists < ship.cabin * 0.5:
		for p in world.planets:
			if not p.is_colonised() or p.owner_race != ship.race_id or p.under_attack > 0.0 or p == last_drop:
				continue
			var spare: float = world.spare_colonists(p)
			if spare < ship.cabin * 0.3:
				continue
			var score := minf(spare, ship.cabin) / (1500.0 + pos.distance_to(p.global_position))
			if score > best_score:
				best_score = score
				job_planet = p
				job = "board"
		return job_planet != null
	for p in world.planets:
		if not p.has_surface():
			continue
		var d := 1500.0 + pos.distance_to(p.global_position)
		var score := 0.0
		var kind := ""
		if p.is_colonised() and p.owner_race != ship.race_id:
			if p.is_defenceless() and ship.colonists * GameData.OFFENSIVE_FACTOR > p.population * 1.15:
				score = 3.0
				kind = "invade"
		elif not p.is_colonised():
			var room: float = p.optimal_population(ship.race_id)
			if room >= 20e6:
				score = 2.0 * clampf(room / 200e6, 0.5, 1.5)
				kind = "settle"
		elif p.population < p.optimal_population(ship.race_id) * 0.3:
			score = 0.8
			kind = "reinforce"
		score = score * 10000.0 / d
		if score > best_score:
			best_score = score
			job_planet = p
			job = kind
	return job_planet != null


func _job_still_good() -> bool:
	if job_planet == null or not is_instance_valid(job_planet):
		return false
	match job:
		"board":
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and world.spare_colonists(job_planet) > 0.0
		"settle":
			return not job_planet.is_colonised() and ship.colonists > 0.0
		"invade":
			return job_planet.is_colonised() and job_planet.owner_race != ship.race_id and ship.colonists > 0.0
		"reinforce":
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and ship.colonists > 0.0
		"load":
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and Economy.surplus(job_planet, haul_res) >= 5.0 \
				and haul_to != null and haul_to.is_colonised() and haul_to.owner_race == ship.race_id
		"deliver":
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and ship.cargo_total() >= 1.0
	return false


## Fly down to the job's world, slow to a landing speed above it, land.
func _fly_to_job(delta: float) -> void:
	var p := job_planet
	if p == null or not is_instance_valid(p):
		return
	var i := ship.inputs
	var out := (ship.global_position - p.global_position)
	if out.length() < 1.0:
		out = Vector2.RIGHT
	var point := p.global_position + out.normalized() * (p.radius + ship.size + 140.0)
	var far := ship.global_position.distance_to(point)
	if far > 2500.0:
		# a long way: the general flight code, warp included
		_avoid_planets(delta)
		_steer_to(point, delta, true)
		if ship.qdrv > 0.0 and far > 5000.0 and not ship.warping and absf(i["turn"]) < 0.1 and ship.energy > 400.0:
			ship.start_warp()
		return
	if ship.warping:
		ship.end_warp()
	# close in: match the world's motion, arriving slowly
	var rel_v := ship.velocity - p.velocity
	var to := point - ship.global_position
	var want_speed := minf(ship.max_speed * 0.8, sqrt(2.0 * ship.accel * 0.5 * maxf(far - 30.0, 0.0)))
	var dv := to.normalized() * want_speed - rel_v
	var diff := wrapf(dv.angle() - ship.rotation, -PI, PI)
	i["turn"] = clampf(diff * 3.0, -1.0, 1.0)
	i["thrust"] = clampf(dv.length() / 80.0, 0.0, 1.0) if absf(diff) < 0.5 else 0.0
	i["strafe"] = 0.0
	i["brake"] = rel_v.length() > want_speed + 60.0
	if ship.landing_refusal(p).is_empty() and (far < 260.0 or rel_v.length() < Ship.LAND_SPEED * 0.6):
		world.try_land(ship, p)
		ground_time = 0.0
		ground_order = false


## On the ground: wait for the colonists, put them down when reinforcing,
## then lift off for the next run.
func _think_on_ground() -> void:
	if not ship.is_landed():
		return
	ground_time += 0.35
	if hauler:
		_haul_on_ground()
		return
	if ship.transfer != 0:
		return
	var here := ship.land_body
	var ours := here.is_colonised() and here.owner_race == ship.race_id
	if not ground_order and ours:
		ground_order = true
		if job == "reinforce" and ship.colonists > 0.0 and here == job_planet:
			world.colonist_command(ship, false)
			last_drop = here
			return
		if job == "board" and ship.colonists < ship.cabin - 1.0 and world.spare_colonists(here) > 0.0:
			world.colonist_command(ship, true)
			return
	if ground_time < 1.2:
		return
	if job != "board":
		last_drop = here
	ground_order = false
	world.try_land(ship)              # lift off
	job_planet = null
	plan_timer = 0.0
	state = State.FERRY if ferry and _plan_ferry() else State.PATROL


# ------------------------------------------------------------ supply runs

## A supply ship's mind: run when shot at, patch up at home when hurt,
## otherwise carry goods from where they pile up to where they are short.
func make_hauler() -> void:
	hauler = true
	ferry = false
	bombard = false
	aggression = 0.0


func _think_hauler() -> void:
	var now := Time.get_ticks_msec() / 1000.0
	var foe: Ship = ship.last_attacker as Ship if is_instance_valid(ship.last_attacker) else null
	if foe != null and not foe.dead and foe.race_id != ship.race_id and now - ship.last_attacked_at < 5.0 \
			and foe.global_position.distance_to(ship.global_position) < 1600.0:
		ship.target = foe
		if state != State.EVADE:
			evade_dir = 1.0 if randf() < 0.5 else -1.0
		state = State.EVADE
		evade_timer = 2.0
		return
	if state == State.EVADE:
		evade_timer -= 0.35
		if evade_timer > 0.0:
			return
		state = State.FERRY if _job_still_good() else State.PATROL
	ship.target = null
	var hurt := ship.hull < ship.hull_max * 0.4 or ship.fuel_fraction() < 0.15
	match state:
		State.RETREAT:
			if ship.hull > ship.hull_max * 0.85 and ship.fuel_fraction() > 0.6:
				state = State.PATROL
		State.FERRY:
			if not _job_still_good():
				state = State.FERRY if _plan_haul() else State.PATROL
		_:
			if hurt and home != null and ship.cargo_total() < 1.0:
				state = State.RETREAT
			elif _plan_haul():
				state = State.FERRY
			else:
				state = State.PATROL
				if ship.global_position.distance_to(patrol_point) < 300.0 or patrol_point.distance_to(home.global_position if home != null else Vector2.ZERO) > 2500.0:
					var c := home.global_position if home != null else Vector2.ZERO
					patrol_point = c + Vector2(randf_range(-1200, 1200), randf_range(-1200, 1200))


## The next supply run.  With goods aboard: the friendly world that wants
## them most.  Empty: the best pairing of a colony with a surplus and one
## that is short of it and makes it worse.
func _plan_haul() -> bool:
	plan_timer -= 0.35
	if plan_timer > 0.0:
		return false
	plan_timer = 3.0
	job_planet = null
	job = ""
	var pos := ship.global_position
	var ours := []
	for p in world.planets:
		if p.is_colonised() and p.owner_race == ship.race_id and p.has_surface() and p.under_attack <= 0.0:
			ours.append(p)
	var best := 0.0
	if ship.cargo_total() >= 1.0:
		for p in ours:
			if p == ship.cargo_origin or p == last_drop:
				continue
			var v := 0.0
			var room := 0.0
			for k in ship.cargo:
				v += Economy.need(p, k) * ship.cargo[k] * GameData.RESOURCE_PRICE.get(k, 1.0)
				room += maxf(Economy.cap(p) - p.stock.get(k, 0.0), 0.0)
			if room < 5.0:
				continue
			# nobody short: any other colony with room takes it
			v = maxf(v, 0.01) / (1500.0 + pos.distance_to(p.global_position))
			if v > best:
				best = v
				job_planet = p
				job = "deliver"
		return job_planet != null
	# runs another supply ship of ours is already on
	var taken := {}
	for o in world.ships:
		if o != ship and o.support and not o.dead and o.race_id == ship.race_id:
			var oa: AIPilot = o.get_node_or_null("AI")
			if oa != null and oa.job in ["load", "deliver"] and oa.haul_to != null:
				taken[[oa.haul_to, oa.haul_res]] = true
	for src in ours:
		for k in Economy.RESOURCES:
			var spare := minf(Economy.surplus(src, k), ship.cargo_max)
			if spare < 20.0:
				continue
			for dst in ours:
				if dst == src or Economy.makes(dst, k) >= Economy.makes(src, k) or taken.has([dst, k]):
					continue
				var want := Economy.need(dst, k)
				if want < 0.3:
					continue
				var amount := minf(spare, maxf(Economy.wanted(dst, k), 20.0))
				var trip: float = 1500.0 + pos.distance_to(src.global_position) + src.global_position.distance_to(dst.global_position)
				var v: float = amount * want * GameData.RESOURCE_PRICE.get(k, 1.0) / trip
				if v > best:
					best = v
					job_planet = src
					job = "load"
					haul_res = k
					haul_to = dst
	return job_planet != null


## On the ground: order the loading or unloading once, wait for the
## spaceport, then lift off for the delivery or the next run.
func _haul_on_ground() -> void:
	var here := ship.land_body
	if not ground_order:
		ground_order = true
		if job == "load" and here == job_planet and haul_to != null:
			# the run's resource first, then anything else the destination wants
			var room := ship.cargo_space()
			var order: Array = [haul_res]
			for k in Economy.RESOURCES:
				if k != haul_res:
					order.append(k)
			for k in order:
				if room < 5.0:
					break
				if k != haul_res and (Economy.need(haul_to, k) < 0.3 or Economy.makes(haul_to, k) >= Economy.makes(here, k)):
					continue
				var n := floorf(minf(minf(Economy.surplus(here, k), room), Economy.wanted(haul_to, k) + 20.0))
				if n >= 5.0:
					world.cargo_command(ship, k, n)
					room -= n
			return
		if job == "deliver" and here == job_planet:
			world.cargo_command(ship, "all", -1.0)
			last_drop = here
			return
	if not ship.cargo_orders.is_empty() or ground_time < 1.2:
		return
	ground_order = false
	world.try_land(ship)              # lift off
	if job == "load" and ship.cargo_total() >= 1.0 and haul_to != null:
		job = "deliver"
		job_planet = haul_to
		last_drop = null
		state = State.FERRY
	else:
		job_planet = null
		job = ""
		plan_timer = 0.0
		state = State.FERRY if _plan_haul() else State.PATROL


## An enemy colony nearby whose defences still stand.
func _colony_to_bombard() -> Planet:
	var best: Planet = null
	var best_d := 3200.0
	for p in world.planets:
		if not p.is_colonised() or p.owner_race == ship.race_id or p.is_defenceless():
			continue
		var d: float = ship.global_position.distance_to(p.global_position) - p.radius
		if d < best_d:
			best_d = d
			best = p
	return best


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


## Mines go out behind a ship, into the path of whoever is chasing it, and
## never point-blank: the pursuer has room to see the mine and turn away.
func _mine_ok(tgt: Node2D, dist: float) -> bool:
	if tgt == null or not is_instance_valid(tgt) or dist < 260.0 or dist > 750.0:
		return false
	var to: Vector2 = tgt.global_position - ship.global_position
	return absf(wrapf(to.angle() - ship.rotation, -PI, PI)) > PI * 0.65
