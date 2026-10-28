## AIPilot -- a computer pilot that flies like a person: it hunts within
## sensor range, leads its shots, breaks off when its shields are gone,
## goes home to recharge, and answers calls for help.  It boards ships it
## has disabled when its crew can take them, and a prize crew flies as
## the wingman of the pilot who captured the ship (`escort`).  Pilots on
## colony duty ferry colonists (land, board, fly, land again); pilots on
## attack duty knock out enemy colonies' defences for them.  Supply,
## trade and drop ships work for the empire: drop ships take troops from
## the garrisons to enemy worlds whose planetary shields are down.  Pilots
## on escort duty guard the empire's convoys when its governor asks;
## pilots on raid duty (and the pirate) hunt enemy supply ships, lone ones
## first, and board them for their cargo.  Supply ships fly round enemy
## colonies' guns and run for shelter when an enemy warship comes near.
## Wingmen (prizes, ordered escorts and pilots called to a leader's wing)
## follow their leader's orders (Wing): form up, attack its target, cover
## it, hold a spot, engage at will, or go home to repair and come back.
class_name AIPilot
extends Node

enum State { PATROL, ATTACK, EVADE, RETREAT, ESCORT, BOARD, FERRY, FLEE }

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
var escort: Ship = null            # prize crews fly with their captor; wingmen with their leader (Wing)
var called := false                # called up to `escort`'s wing from the empire's pilots: Dismiss sends it back
var sent_home := false             # a wingman on its way home to repair; rejoins once mended
var wing_slot := 0                 # its place in the leader's formation
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
var haul_frame: Ship = null      # or the station frame it is for
var dock_at: Ship = null         # the station this run docks at (to fill a frame, or empty a mine)
var trader := false              # a trade ship: sells the empire's surplus at the markets, buys what it is short of
var haul_market: Ship = null     # the market a trade run sells at
var dropper := false             # a drop ship: carries the empire's troops to enemy worlds
var duel := false                # a free-for-all: hunt around the arena, never go home to mend
var escort_duty := false         # guards convoys when the governor asks (phase 6)
var convoy_guard := false        # `escort` is a convoy, not the captor of a prize
var raid := false                # hunts enemy supply ships
var kit_body: Planet = null      # a supply ship's kit run: where the station goes up
var kit_turn := 0.0              # how far round the world to try, when the first spot is taken
var kit_side := Vector2.ZERO     # the side of the world the run came in on

## Raiders look for supply ships this far out (still only what they can see).
const RAID_RANGE := 9000.0
## A supply ship runs for shelter when an enemy warship it can see comes
## this close, and goes back to work once the warship is this far off.
const FLEE_RANGE := 2200.0
const SAFE_RANGE := 3200.0
## A warship off raid duty goes after an enemy supply ship only this close.
const PREY_RANGE := 800.0
## A run to or from a place under an enemy colony's guns is worth this
## much of a safe one: taken only when nothing safer is worth doing.
const EXPOSED_WORTH := 0.25
## Supply ships keep this much further off an enemy colony than its guns reach.
const GUN_MARGIN := 350.0

var flee_from: Ship = null       # the warship a supply ship is running from
var shelter: Node2D = null       # and where it is running to: a friendly colony or station
var _route_for := Vector2.INF    # the last route asked for, and the way found round enemy guns
var _route_way := Vector2.ZERO
var _route_at := 0.0
var _exposed := {}               # place -> under enemy guns, for one round of planning


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
	escort_duty = duty == "escort" and not ship.is_station and ship.race_id <= 2
	# the pirate has always lived off other people's cargo
	raid = (duty == "raid" and ship.race_id <= 2 or ship.race_id == 3) and not ship.is_station


## The governor's orders for a pilot on escort duty: fly with this convoy.
func guard(h: Ship) -> void:
	escort = h
	convoy_guard = true
	state = State.ESCORT


## The convoy got there (or was lost): back to patrol.
func release() -> void:
	escort = null
	convoy_guard = false
	if state == State.ESCORT:
		state = State.PATROL
	patrol_point = _random_patrol_point()


# ------------------------------------------------------------ wings

## Fly on `leader`'s wing.  `p_called`: one of the empire's pilots, called up.
func join(leader: Ship, p_called: bool) -> void:
	escort = leader
	called = p_called
	convoy_guard = false
	sent_home = false
	job_planet = null
	job = ""
	if state != State.BOARD:
		state = State.ESCORT
		ship.target = null


## Dismissed (or the leader is gone): back to the empire's patrols.
func leave_wing() -> void:
	escort = null
	called = false
	sent_home = false
	if state in [State.ESCORT, State.RETREAT]:
		state = State.PATROL
	patrol_point = _random_patrol_point()


## Home to repair and refuel; back on the wing once mended.
func go_home() -> void:
	if home == null:
		return
	sent_home = true
	if state != State.BOARD:
		state = State.RETREAT


## A new order: think about it straight away (a little apart, so the
## wing does not move as one machine), and come back from a trip home
## unless the tanks are dry.
func heard_order() -> void:
	think_timer = randf() * 0.3
	if sent_home and ship.fuel_fraction() > Wing.BINGO_FUEL and ship.hull > ship.hull_max * 0.3:
		sent_home = false
		if state == State.RETREAT:
			state = State.ESCORT


## The wing's standing order, or "" when we fly with no leader (or guard a convoy).
func order() -> String:
	if escort == null or convoy_guard or not is_instance_valid(escort) or world.get("wing") == null:
		return ""
	return world.wing.order_of(escort)["order"]


## What a wingman fights, by its leader's order.
func _wing_target(current: Node2D, lead: Ship) -> Node2D:
	if convoy_guard:
		return _escort_target(current)
	if sent_home:
		return _cornered()
	var o: Dictionary = world.wing.order_of(lead)
	match o["order"]:
		"form":
			return _threat(lead)
		"attack":
			var t: Node2D = o["target"]
			if t is Ship and is_instance_valid(t) and not t.dead and t.team() != ship.team() \
					and (t.visible_to(ship) or t.visible_to(lead) or float(o.get("lost", 0.0)) < Wing.TRACK_GRACE):
				return t
			if t is Planet and is_instance_valid(t) and t.is_colonised() and t.owner_race != ship.race_id and not t.is_defenceless():
				return t
		"hold":
			var at: Vector2 = world.wing.hold_point(lead)
			if current != null and current.global_position.distance_to(at) < Wing.HOLD_LEASH:
				return current
			var threat := _threat(lead)
			if threat != null and threat.global_position.distance_to(at) < Wing.HOLD_LEASH * 1.5:
				return threat
			return world.nearest_enemy_ship(at, ship.team(), Wing.HOLD_LEASH, ship)
		"engage":
			if current != null and current.global_position.distance_to(lead.global_position) < Wing.ENGAGE_RANGE:
				return current
			var near: Ship = world.nearest_enemy_ship(lead.global_position, ship.team(), Wing.ENGAGE_RANGE, ship)
			if near != null:
				return near
	return _escort_target(current)


## Whoever just shot at us or at the leader, if it is still close: what a
## wing in formation answers.
func _threat(lead: Ship) -> Ship:
	var now := Time.get_ticks_msec() / 1000.0
	for s in [ship, lead]:
		var a: Node = s.last_attacker
		if a is Ship and is_instance_valid(a) and not a.dead and a.team() != ship.team() and now - s.last_attacked_at < 6.0 \
				and a.global_position.distance_to(lead.global_position) < ESCORT_LEASH * 1.5:
			return a
	return null


## On the way home: fight back only when jumped close.
func _cornered() -> Ship:
	var a: Node = ship.last_attacker
	if a is Ship and is_instance_valid(a) and not a.dead and a.team() != ship.team() \
			and Time.get_ticks_msec() / 1000.0 - ship.last_attacked_at < 4.0 and a.global_position.distance_to(ship.global_position) < 600.0:
		return a
	return null


## Where to fly in the wing: a vee off the leader's quarters, tighter in
## formation; round the spot on Hold.
func _wing_spot() -> Vector2:
	if order() == "hold":
		var at: Vector2 = world.wing.hold_point(escort)
		var n: int = maxi(world.wing.members(escort).size(), 1)
		var ang := TAU * wing_slot / n + escort.get_instance_id() % 7
		return at + Vector2.RIGHT.rotated(ang) * (ship.size + 160.0 + 40.0 * n)
	var row := wing_slot / 2 + 1
	var side := 1.0 if wing_slot % 2 == 0 else -1.0
	var gap := escort.size + ship.size + (70.0 if order() == "form" else 140.0)
	var off := Vector2(-row * gap * 0.75, side * row * gap * 0.7).rotated(escort.rotation)
	return escort.global_position + off + escort.velocity * 0.8


func _governor() -> Governor:
	return world.governors.get(ship.race_id, null) if "governors" in world else null


func _random_patrol_point() -> Vector2:
	var centre := home.global_position if home != null else Vector2.ZERO
	# patrol near home, or roam toward an enemy planet when aggressive
	if duel:
		return centre + Vector2(randf_range(-1200, 1200), randf_range(-1200, 1200))
	# raiders lie in wait where enemy supply ships call: their worlds and stations
	if raid and randf() < 0.75:
		var calls := []
		for p in world.planets:
			if p.is_colonised() and p.owner_race != ship.race_id and p.owner_race <= 2:
				calls.append(p)
		for o in world.ships:
			if o.is_station and not o.dead and not o.is_starbase() and o.race_id != ship.race_id and o.race_id <= 2:
				calls.append(o)
		if not calls.is_empty():
			var c: Node2D = calls[randi() % calls.size()]
			return c.global_position + Vector2(randf_range(-1400, 1400), randf_range(-1400, 1400))
	# attack duty: where the empire is going after
	var gov := _governor()
	if bombard and gov != null and gov.target != null and is_instance_valid(gov.target) and randf() < 0.6:
		return gov.target.global_position + Vector2(randf_range(-900, 900), randf_range(-900, 900))
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
	if escort != null and (not is_instance_valid(escort) or escort.race_id != ship.race_id or (escort.dead and convoy_guard)):
		leave_wing()               # the leader is gone for good: back to the empire's patrols
	# a wing whose leader is down goes home and waits for it to relaunch
	var lead: Ship = escort if escort != null and not escort.dead else null
	var in_wing := escort != null and not convoy_guard
	if in_wing and lead == null and not sent_home:
		go_home()
	if in_wing:
		wing_slot = world.wing.slot_of(ship, escort)
	if lead != null and in_wing and not sent_home and home != null and not duel and ship.fuel_fraction() < Wing.BINGO_FUEL:
		go_home()
		world.wing.report(ship, "bingo")
	# acquire
	var tgt := ship.target
	if tgt == null or not is_instance_valid(tgt) or (tgt is Ship and (tgt.dead or not tgt.visible_to(ship) or tgt.team() == ship.team() or tgt.race_id == GameData.NEUTRAL)):
		tgt = null
	elif tgt is Planet and (not tgt.is_colonised() or tgt.owner_race == ship.race_id or tgt.is_defenceless()):
		tgt = null                      # knocked out: leave it for the colonists
	elif tgt is Ship and tgt.support and not raid and not duel and state != State.BOARD and tgt != ship.last_attacker \
			and tgt.global_position.distance_to(ship.global_position) > PREY_RANGE * 1.25:
		tgt = null                      # a supply ship got away: not worth the chase
	# a friend is taking that one alive: leave it be
	if tgt is Ship and tgt.boarded_by != ship and tgt.claimed_by != ship and tgt.spoken_for(ship.team()):
		tgt = null
	if lead != null:
		tgt = _wing_target(tgt, lead)
	elif in_wing:
		tgt = _cornered()           # waiting at home for the leader
	elif tgt == null or (randf() < 0.1 and state != State.BOARD):
		var best: Ship = world.nearest_enemy_ship(ship.global_position, ship.team(), ship.sensor_range, ship)
		# supply ships are the raiders' business: the rest fight warships,
		# and shoot at a passing supply ship only when it comes close
		if best != null and best.support and not raid and not duel:
			var rival := _nearest_warship(ship.sensor_range)
			if rival != null:
				best = rival
			elif best.global_position.distance_to(ship.global_position) > PREY_RANGE:
				best = null
		if best == null and duel:
			best = world.nearest_known_enemy(ship)   # the arena is small: everyone in it is fair game
		# answer a recent attacker first
		if ship.last_attacker != null and is_instance_valid(ship.last_attacker) and ship.last_attacker is Ship and not ship.last_attacker.dead \
				and Time.get_ticks_msec() / 1000.0 - ship.last_attacked_at < 6.0 and ship.last_attacker.team() != ship.team():
			best = ship.last_attacker
		if best != null:
			tgt = best
	# raid duty: an enemy supply ship in sight, a lone one first
	if tgt == null and raid and escort == null and state == State.PATROL:
		tgt = _raid_target()
	# attack duty: with no ship about, go after an enemy colony's defences
	if tgt == null and bombard and state == State.PATROL and escort == null:
		tgt = _colony_to_bombard()
	# the Accord: no first shot at another empire
	if tgt != null and world.accord and not world.fair_game(ship.team(), tgt):
		tgt = null
	ship.target = tgt
	var weak := shield_frac < 0.15 and hull_frac < 0.55
	if lead != null and tgt == null and state != State.BOARD and not sent_home:
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
			elif sent_home and home != null:
				state = State.RETREAT
			elif ferry and not in_wing and _plan_ferry():
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
				sent_home = false
			elif hull_frac > 0.9 and shield_frac > 0.8 and ship.energy > ship.energy_max * 0.6 and ship.fuel_fraction() > 0.6 and ship.crew_fraction() > 0.8:
				if in_wing and lead == null:
					pass                # mended: wait here for the leader to relaunch
				elif sent_home:
					sent_home = false
					state = State.ESCORT
					world.wing.report(ship, "back")
				else:
					state = State.PATROL
					patrol_point = _random_patrol_point()
			elif tgt != null and tgt.global_position.distance_to(ship.global_position) < 500.0 and hull_frac > 0.4:
				# cornered: fight back
				state = State.ATTACK
	# shields to the side the danger is on: the nose in a fight, the tail running away
	var want_focus := 0
	if state == State.ATTACK and tgt != null:
		want_focus = 1
	elif (state == State.RETREAT or state == State.FLEE or state == State.EVADE) and ship.last_attacker != null:
		want_focus = 2
	if ship.shield_focus != want_focus:
		ship.set_shield_focus(want_focus)
	_think_sensors(tgt)


## The jammer on whatever we fight, a ping when all we have is a contact,
## and with nothing in sight, a look at the nearest contact (or at whoever
## is jamming us).
func _think_sensors(tgt: Node2D) -> void:
	var fighting := state == State.ATTACK and tgt is Ship
	if ship.can_jam:
		if fighting and not ship.jamming and ship.energy > ship.energy_max * 0.3 and ship.has_track(tgt) \
				and tgt.global_position.distance_to(ship.global_position) <= float(ship.stats.get("jam_reach", 2500.0)):
			ship.set_jam(true)
		elif ship.jamming and (not fighting or ship.energy < ship.energy_max * 0.15):
			ship.set_jam(false)
	var sv: Sensors = ship.sensors
	if sv.has("pulse") and sv.ping_ready <= 0.0 and ship.energy > ship.energy_max * 0.4:
		if (tgt is Ship and not ship.has_track(tgt)) or (tgt == null and _nearest_contact() != null):
			ship.ping()
	if tgt == null and state == State.PATROL and not hauler and escort == null:
		var c := _nearest_contact()
		if c != null:
			patrol_point = Sensors.estimate(ship, c)
		elif sv.is_jammed() and sv.jammed_by != null and is_instance_valid(sv.jammed_by) and not sv.jammed_by.dead:
			patrol_point = sv.jammed_by.global_position   # turn on the jammer's heat


## The nearest enemy we know is about but cannot lock: worth a look.
func _nearest_contact() -> Ship:
	var best: Ship = null
	var best_d := ship.sensor_range * Sensors.CONTACT_REACH * 1.5
	for s in world.ships:
		if s.dead or s.team() == ship.team() or s.race_id == GameData.NEUTRAL or s.is_landing() or s.support:
			continue
		var d: float = s.global_position.distance_to(ship.global_position)
		if d < best_d and ship.contact_quality(s) == 2:
			best_d = d
			best = s
	return best


## A prize crew fights near its captor: the captor's own target first,
## else whatever comes within the leash.
func _escort_target(current: Node2D) -> Node2D:
	var lead: Node2D = escort.target
	if lead is Ship and is_instance_valid(lead) and not lead.dead and lead.team() != ship.team() and lead.race_id != GameData.NEUTRAL and lead.visible_to(ship) \
			and lead.global_position.distance_to(escort.global_position) < ESCORT_LEASH:
		return lead
	# the leader's going after a colony's guns: help
	if lead is Planet and is_instance_valid(lead) and lead.is_colonised() and lead.owner_race != ship.race_id and not lead.is_defenceless() \
			and lead.global_position.distance_to(escort.global_position) < ESCORT_LEASH + lead.radius:
		return lead
	if current != null and current.global_position.distance_to(escort.global_position) < ESCORT_LEASH:
		return current
	var near: Ship = world.nearest_enemy_ship(escort.global_position, ship.team(), ESCORT_LEASH * 0.8, ship)
	return near


## The enemy supply ship to go after: in sight (our own or the empire's
## sensors), the nearest, a guarded convoy only when nothing else is about.
func _raid_target() -> Ship:
	var best: Ship = null
	var best_v := INF
	for o in world.ships:
		if o.dead or not o.support or o.is_station or o.race_id == GameData.NEUTRAL or o.team() == ship.team():
			continue
		if o.is_landing() or not o.visible_to(ship):
			continue
		var d: float = o.global_position.distance_to(ship.global_position)
		if d > RAID_RANGE:
			continue
		var v := d + (3000.0 if o.convoy else 0.0)
		if v < best_v:
			best_v = v
			best = o
	return best


## Board this one instead of finishing it?  Decided once per target, and
## only when our crew would win quickly.
func _wants_to_board(tgt: Node2D) -> bool:
	if not tgt is Ship or ship.is_station or ship.support or not tgt.boardable():
		return false
	var why := Capture.why_not(ship, tgt)
	if why != "" and why != "far":
		return false
	var key := tgt.get_instance_id()
	if not board_rolls.has(key):
		# raiders want the cargo, so they nearly always try; a breached
		# station is a prize for the bold
		board_rolls[key] = randf() < (0.9 if raid and tgt.support else (0.3 + 0.5 * aggression if tgt.is_station else 0.55 + 0.4 * aggression))
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
			if escort == null or not is_instance_valid(escort) or escort.dead:
				return
			# hold our place in the wing, a little ahead of the leader's motion
			var spot := _wing_spot() if not convoy_guard else escort.global_position \
				+ Vector2(-1.0, (1.0 if ship.get_instance_id() % 2 == 0 else -1.0) * 0.9).normalized().rotated(escort.rotation) * (escort.size + ship.size + 140.0) + escort.velocity * 0.8
			var far := ship.global_position.distance_to(spot)
			if ship.warping and far < 2500.0:
				ship.end_warp()
			if order() == "hold" and far < 1500.0:
				var hb: Node2D = world.wing.order_of(escort).get("body", null)
				_close_on(spot, hb.velocity if hb != null and is_instance_valid(hb) else Vector2.ZERO)
			else:
				_steer_to(spot, delta, far < 900.0)
			if far < 160.0 and order() != "hold":
				# in our slot: face the way the leader faces
				i["turn"] = clampf(wrapf(escort.rotation - ship.rotation, -PI, PI) * 2.0, -1.0, 1.0)
			_avoid_planets(delta)
			if ship.qdrv > 0.0 and far > 5000.0 and not ship.warping and absf(i["turn"]) < 0.1:
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
				i["fire_beam"] = dist < ship.beam_reach() and absf(diff2) < 1.8
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
		State.FLEE:
			_flee(delta)
		State.FERRY:
			if dock_at != null and is_instance_valid(dock_at):
				_fly_to_dock(delta)
			elif job == "deploy":
				_fly_to_site(delta)
			else:
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
			# settlers only take a world with no troops on it, twenty to one
			if p.is_defenceless() and Ground.count(p.garrison) == 0 and p.battle.is_empty() and not p.pshield_up() and world.at_war(ship.race_id, p.owner_race) \
					and ship.colonists > p.population * GameData.COLONIST_RATIO * 1.15:
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
	# a kit run has no job planet: the starbase, then the site
	if job == "kit":
		return dock_at != null and is_instance_valid(dock_at) and not dock_at.dead and kit_body != null
	if job == "deploy":
		return kit_body != null and is_instance_valid(kit_body) and ship.kit != ""
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
			if haul_frame != null:
				return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and _spare_for_frame(job_planet, haul_res) >= 1.0 \
					and is_instance_valid(haul_frame) and world.supply_wants(haul_frame).get(haul_res, 0.0) >= 0.5
			if haul_market != null:
				return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and Economy.surplus(job_planet, haul_res) >= 5.0 \
					and Economy.is_market(haul_market)
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and Economy.surplus(job_planet, haul_res) >= 5.0 \
				and haul_to != null and haul_to.is_colonised() and haul_to.owner_race == ship.race_id
		"deliver":
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and ship.cargo_total() >= 1.0
		"muster":
			return job_planet.is_colonised() and job_planet.owner_race == ship.race_id and Ground.spare_troops(job_planet) >= 1.0 and ship.troop_room() >= 1
		"assault":
			return job_planet.is_colonised() and job_planet.owner_race != ship.race_id and ship.troop_count() > 0 \
				and (_assault_open(job_planet) or ship.is_landing())
	return false


## A docking run still worth flying: the frame still wants what we carry,
## the mine still has a load for us.
func _dock_job_good() -> bool:
	if dock_at == null or not is_instance_valid(dock_at) or dock_at.dead:
		return false
	if dock_at.race_id != ship.race_id and not Economy.is_market(dock_at):
		return false
	match job:
		"supply":
			var wants: Dictionary = world.supply_wants(dock_at)
			for k in ship.cargo:
				if wants.get(k, 0.0) >= 0.5:
					return true
			return false
		"sell":
			if not Economy.is_market(dock_at) or ship.cargo_total() < 1.0:
				return false
			for k in ship.cargo:
				if world.port_room(dock_at, k) >= 1.0:
					return true
			return false
		"buy":
			return Economy.is_market(dock_at) and dock_at.cargo.get(haul_res, 0.0) >= 5.0 and ship.cargo_space() >= 5.0 \
				and ship.credits >= Economy.buy_price(dock_at, haul_res, ship) * 5.0
		"collect":
			return _collectable(dock_at) >= 10.0 and ship.cargo_space() >= 10.0
	return false


## What a supply ship would take away from a station: a mine's or
## skimmer's output, a refinery's Alloys.
static func _collectable(st: Ship) -> float:
	if st.building:
		return 0.0
	match st.design_name:
		"MiningStation", "GasSkimmer":
			return st.cargo_total()
		"Refinery":
			return st.cargo.get("alloys", 0.0)
	return 0.0


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
	if _fly_far(point, far, delta):
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


## A trade ship: a hauler that works the markets.
func make_trader() -> void:
	make_hauler()
	trader = true


## A drop ship: a hauler whose loads are troops.
func make_dropper() -> void:
	make_hauler()
	dropper = true


## The governor's order for a supply ship: fetch a station kit from the
## starbase and put the frame up over `body`.
func start_kit(key: String, body: Planet) -> void:
	var base: Ship = world.starbase_of(ship.race_id)
	if base == null or body == null:
		return
	job = "kit"
	haul_res = key
	kit_body = body
	kit_turn = 0.0
	kit_side = Vector2.ZERO
	job_planet = null
	haul_to = null
	haul_frame = null
	haul_market = null
	dock_at = base
	ground_order = false
	state = State.FERRY


## Alongside the starbase: the kit comes aboard (the empire pays for it).
func _take_kit() -> void:
	if ship.global_position.distance_to(dock_at.global_position) > dock_at.size + ship.size + 260.0:
		return
	ship.kit = haul_res
	dock_at = null
	job = "deploy"


## Over the site: put the frame up, or give the run up when the site is
## no good after all.
func _deploy_kit() -> void:
	if kit_body == null or not is_instance_valid(kit_body) or ship.kit == "":
		_end_kit()
		return
	if ship.warping or ship.global_position.distance_to(kit_body.global_position) - kit_body.radius > World.SITE_RANGE * 0.8:
		return
	var site: Dictionary = world.station_site(ship, ship.kit)
	if site["why"] == "":
		world.deploy_kit(ship)
		_end_kit()
	elif "Too close" in site["why"] and kit_turn < TAU:
		if ship.global_position.distance_to(_site_point()) < 250.0:
			kit_turn += 0.6         # there, and still crowded: move round the world a little
	elif not "Fly within" in site["why"]:
		ship.kit = ""               # taken meanwhile, or the world changed hands
		_end_kit()


func _end_kit() -> void:
	kit_body = null
	job = ""
	plan_timer = 0.0
	state = State.FERRY if _plan_run() else State.PATROL


func _fly_to_site(delta: float) -> void:
	var p := kit_body
	if p == null or not is_instance_valid(p):
		return
	var point := _site_point()
	var far := ship.global_position.distance_to(point)
	if _fly_far(point, far, delta):
		return
	if ship.warping:
		ship.end_warp()
	_close_on(point, p.velocity)


## Where over the kit's world to put the station up: the near side,
## turned round by kit_turn when that spot is crowded.
func _site_point() -> Vector2:
	var p := kit_body
	if kit_side == Vector2.ZERO:
		kit_side = (ship.global_position - p.global_position).normalized()
		if kit_side == Vector2.ZERO:
			kit_side = Vector2.RIGHT
	return p.global_position + kit_side.rotated(kit_turn) * (p.radius + 450.0)


## The next run: a drop ship's, a trade ship's or a supply ship's.
func _plan_run() -> bool:
	if ship.kit != "" and kit_body == null:
		ship.kit = ""               # a kit with no run (after a reload): back to the empire
	if dropper:
		return _plan_drop()
	return _plan_trade() if trader else _plan_haul()


## Is a landing on `p` open now, or about to be (its planetary shield is
## under fire)?
static func _assault_open(p: Planet) -> bool:
	return not p.pshield_up() or p.under_attack > 0.0


## The next drop.  With troops aboard: the nearest enemy colony they can
## beat whose planetary shield is down or under fire, a knocked-out one
## first.  Otherwise: the friendly garrison with the most to spare.
func _plan_drop() -> bool:
	plan_timer -= 0.35
	if plan_timer > 0.0:
		return false
	plan_timer = 3.0
	job_planet = null
	job = ""
	dock_at = null
	ground_order = false
	var pos := ship.global_position
	var best := 0.0
	var mine := Ground.strength(ship.troops)
	if ship.troop_count() > 0:
		for p in world.planets:
			if not p.is_colonised() or p.owner_race == ship.race_id or p.owner_race == GameData.NEUTRAL or not p.has_surface():
				continue
			if not world.at_war(ship.race_id, p.owner_race):
				continue
			if not _assault_open(p):
				continue
			var ours := 0.0
			if not p.battle.is_empty():
				if int(p.battle["race"]) != ship.race_id:
					continue
				ours = Ground.strength(p.battle["units"])
			var factor := 1.0 if p.is_defenceless() or not p.buildings.has("barracks") else Ground.BUNKER_FACTOR
			if Ground.estimate(mine + ours, 0.0, Ground.defence(p), factor) == INF:
				continue
			var score := 10000.0 / (1500.0 + pos.distance_to(p.global_position))
			if p.is_defenceless():
				score *= 2.0
			var gov := _governor()
			if gov != null and gov.target == p:
				score *= 3.0
			if score > best:
				best = score
				job_planet = p
				job = "assault"
		if job_planet != null or ship.troop_room() < 1:
			return job_planet != null
	for p in world.planets:
		if not p.is_colonised() or p.owner_race != ship.race_id or p.under_attack > 0.0:
			continue
		var spare := Ground.spare_troops(p)
		if spare < 1.0:
			continue
		var score := minf(spare, 6.0) / (1500.0 + pos.distance_to(p.global_position))
		if score > best:
			best = score
			job_planet = p
			job = "muster"
	return job_planet != null


## A drop ship on the ground: take the spare troops aboard, or send them
## in, then lift off.
func _drop_on_ground() -> void:
	var here := ship.land_body
	if not ground_order:
		ground_order = true
		if job == "muster" and here == job_planet and here.owner_race == ship.race_id:
			for u in GameData.UNIT_ORDER:
				while ship.troop_room() >= 1 and here.garrison.get(u, 0.0) >= 0.95 and Ground.spare_troops(here) >= GameData.UNITS[u]["strength"]:
					world.troop_command(ship, "load", u)
		elif job == "assault" and here == job_planet and here.owner_race != ship.race_id and ship.troop_count() > 0:
			world.troop_command(ship, "land")
		return
	if ground_time < 1.2:
		return
	ground_order = false
	world.try_land(ship)              # lift off
	job_planet = null
	job = ""
	plan_timer = 0.0
	state = State.FERRY if _plan_run() else State.PATROL


func _think_hauler() -> void:
	var now := Time.get_ticks_msec() / 1000.0
	var foe: Ship = ship.last_attacker as Ship if is_instance_valid(ship.last_attacker) else null
	if foe == null or foe.dead or foe.race_id == ship.race_id or now - ship.last_attacked_at >= 5.0 \
			or foe.global_position.distance_to(ship.global_position) >= 1600.0:
		# running: until the warship falls well behind, or, once under the
		# shelter's guns, until it keeps its distance
		# (nothing to fear while nothing can hurt us)
		foe = _nearest_warship(SAFE_RANGE if state == State.FLEE and not _sheltered() else FLEE_RANGE) if ship.invulnerable <= 0.0 else null
	if foe != null:
		ship.target = foe
		flee_from = foe
		if state != State.FLEE or shelter == null or not is_instance_valid(shelter):
			shelter = _shelter(foe)
		if shelter != null:
			state = State.FLEE
			return
		# nowhere to run to: jink about and hope
		if state != State.EVADE:
			evade_dir = 1.0 if randf() < 0.5 else -1.0
		state = State.EVADE
		evade_timer = 2.0
		return
	if state == State.FLEE:
		# out of sight or well behind: back to work
		flee_from = null
		shelter = null
		state = State.FERRY if _job_still_good() or _dock_job_good() else State.PATROL
	if state == State.EVADE:
		evade_timer -= 0.35
		if evade_timer > 0.0:
			return
		state = State.FERRY if _job_still_good() or _dock_job_good() else State.PATROL
	ship.target = null
	var hurt := ship.hull < ship.hull_max * 0.4 or ship.fuel_fraction() < 0.15
	match state:
		State.RETREAT:
			if ship.hull > ship.hull_max * 0.85 and ship.fuel_fraction() > 0.6:
				state = State.PATROL
		State.FERRY:
			if job == "kit" and dock_at != null and is_instance_valid(dock_at) and not dock_at.dead:
				_take_kit()
			elif job == "deploy":
				_deploy_kit()
			elif dock_at != null:
				_haul_docked()
			elif not _job_still_good():
				state = State.FERRY if _plan_run() else State.PATROL
		_:
			if hurt and home != null and ship.cargo_total() < 1.0:
				state = State.RETREAT
			elif _plan_run():
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
	dock_at = null
	haul_frame = null
	haul_market = null
	ground_order = false
	_exposed.clear()
	var pos := ship.global_position
	var ours := []
	for p in world.planets:
		if p.is_colonised() and p.owner_race == ship.race_id and p.has_surface() and p.under_attack <= 0.0:
			ours.append(p)
	var frames := []
	var producers := []
	for o in world.ships:
		if o.is_station and not o.dead and o.race_id == ship.race_id:
			if not world.supply_wants(o).is_empty():
				frames.append(o)          # a frame, or a shipyard short of hull materials
			elif _collectable(o) > 0.0:
				producers.append(o)
	var best := 0.0
	if ship.cargo_total() >= 1.0:
		# a frame that needs what we carry comes first: it pays full price
		for f in frames:
			var n := 0.0
			var wants: Dictionary = world.supply_wants(f)
			for k in ship.cargo:
				n += minf(ship.cargo[k], wants.get(k, 0.0)) * GameData.RESOURCE_PRICE.get(k, 1.0)
			var v := n * _priority(f) * _safety(f) / (1500.0 + pos.distance_to(f.global_position))
			if n >= 1.0 and v > best:
				best = v
				dock_at = f
				job = "supply"
		if dock_at != null:
			return true
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
			v = maxf(v, 0.01) * _safety(p) / (1500.0 + pos.distance_to(p.global_position))
			if v > best:
				best = v
				job_planet = p
				job = "deliver"
		return job_planet != null
	# runs another supply ship of ours is already on, and pilots' jobs
	var taken := {}
	var gov := _governor()
	if gov != null:
		taken.merge(gov.taken_runs())
	for o in world.ships:
		if o != ship and o.support and not o.dead and o.race_id == ship.race_id:
			var oa: AIPilot = o.get_node_or_null("AI")
			if oa != null and oa.job in ["load", "deliver"] and oa.haul_to != null:
				taken[[oa.haul_to, oa.haul_res]] = true
	for o in world.ships:
		if o != ship and o.support and not o.dead and o.race_id == ship.race_id:
			var oa: AIPilot = o.get_node_or_null("AI")
			if oa == null:
				continue
			if oa.haul_frame != null:
				taken[[oa.haul_frame, oa.haul_res]] = true
			if oa.dock_at != null:
				taken[[oa.dock_at, oa.job]] = true
	# materials for a frame, from the nearest colony that can spare them
	for f in frames:
		var wants: Dictionary = world.supply_wants(f)
		for k in wants:
			var need: float = wants[k]
			if need < 1.0 or taken.has([f, k]) or taken.has([f, "supply"]):
				continue
			for src in ours:
				var amount := minf(need, _spare_for_frame(src, k))
				if amount < minf(need, 5.0):
					continue
				var trip: float = 1500.0 + pos.distance_to(src.global_position) + src.global_position.distance_to(f.global_position)
				var v: float = amount * GameData.RESOURCE_PRICE.get(k, 1.0) * _priority(f) * _safety(src) * _safety(f) / trip
				if v > best:
					best = v
					job_planet = src
					job = "load"
					haul_res = k
					haul_frame = f
					haul_to = null
	# a mine or skimmer's output, a refinery's Alloys
	for st in producers:
		var amount := minf(_collectable(st), ship.cargo_max)
		if amount < COLLECT_AT or taken.has([st, "collect"]):
			continue
		var worth := 0.0
		for k in st.cargo:
			if st.design_name != "Refinery" or k == "alloys":
				worth += st.cargo[k] * GameData.RESOURCE_PRICE.get(k, 1.0)
		worth *= minf(amount / maxf(_collectable(st), 1.0), 1.0)
		# a full station stops working: fetch it first
		var v: float = worth * (3.0 if st.cargo_space() < st.cargo_max * 0.2 else 2.0) * _safety(st) / (1500.0 + pos.distance_to(st.global_position))
		if v > best:
			best = v
			job_planet = null
			job = "collect"
			dock_at = st
			haul_frame = null
	if dock_at != null:
		return true
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
				var v: float = amount * want * GameData.RESOURCE_PRICE.get(k, 1.0) * _safety(src) * _safety(dst) / trip
				if v > best:
					best = v
					job_planet = src
					job = "load"
					haul_res = k
					haul_to = dst
	return job_planet != null


## The next trade run.  With goods aboard: the market that pays best for
## them (or a colony that wants them, if no market does).  Empty: a
## colony's surplus to the market that pays most for it, or goods a market
## sells cheap that one of our colonies is short of.  Trade ships use their
## own empire's markets and the free port.
func _plan_trade() -> bool:
	plan_timer -= 0.35
	if plan_timer > 0.0:
		return false
	plan_timer = 3.0
	job_planet = null
	job = ""
	dock_at = null
	haul_frame = null
	haul_market = null
	haul_to = null
	ground_order = false
	_exposed.clear()
	var markets := []
	for o in world.ships:
		if Economy.is_market(o) and (o.race_id == ship.race_id or o.race_id == GameData.NEUTRAL):
			markets.append(o)
	var pos := ship.global_position
	var best := 0.0
	if ship.cargo_total() >= 1.0:
		for m in markets:
			if m == ship.cargo_origin:
				continue
			var v := 0.0
			for k in ship.cargo:
				v += Economy.sell_price(m, k, ship) * minf(ship.cargo[k], world.port_room(m, k))
			v *= _safety(m) / (1500.0 + pos.distance_to(m.global_position))
			if v > best:
				best = v
				dock_at = m
				job = "sell"
		if dock_at != null:
			return true
		plan_timer = 0.0
		return _plan_haul()          # nowhere to sell it: to a colony, like a supply ship
	var ours := []
	for p in world.planets:
		if p.is_colonised() and p.owner_race == ship.race_id and p.has_surface() and p.under_attack <= 0.0:
			ours.append(p)
	for src in ours:
		for k in Economy.RESOURCES:
			var spare := minf(Economy.surplus(src, k), ship.cargo_max)
			if spare < 30.0:
				continue
			for m in markets:
				var amount := minf(spare, world.port_room(m, k))
				var trip: float = 1500.0 + pos.distance_to(src.global_position) + src.global_position.distance_to(m.global_position)
				var v: float = Economy.sell_price(m, k, ship) * amount * _safety(src) * _safety(m) / trip
				if amount >= 30.0 and v > best:
					best = v
					job_planet = src
					job = "load"
					haul_res = k
					haul_market = m
	for m in markets:
		for k in Economy.RESOURCES:
			var have: float = m.cargo.get(k, 0.0)
			var price := Economy.buy_price(m, k, ship)
			if have < 40.0 or price > GameData.RESOURCE_PRICE.get(k, 1.0):
				continue           # only what sells cheap
			for dst in ours:
				var want := Economy.need(dst, k)
				if want < 0.4:
					continue
				var amount := minf(minf(have - 10.0, Economy.wanted(dst, k) + 20.0), minf(ship.cargo_max, ship.credits / price))
				if amount < 20.0:
					continue
				var trip: float = 1500.0 + pos.distance_to(m.global_position) + m.global_position.distance_to(dst.global_position)
				var v: float = amount * want * GameData.RESOURCE_PRICE.get(k, 1.0) * 0.8 * _safety(m) * _safety(dst) / trip
				if v > best:
					best = v
					job_planet = null
					job = "buy"
					dock_at = m
					haul_res = k
					haul_to = dst
	if dock_at != null:
		haul_market = null
		return true
	return job_planet != null


## How much a run to or from `n` is worth for its danger: less when an
## enemy colony's guns cover it (EXPOSED_WORTH).
func _safety(n: Node2D) -> float:
	if not _exposed.has(n):
		var under := false
		var r: float = (n as Planet).radius if n is Planet else 0.0
		for p in world.planets:
			if p != n and p.is_colonised() and p.owner_race != ship.race_id and p.owner_race != GameData.NEUTRAL and not p.is_defenceless() \
					and p.global_position.distance_to(n.global_position) < Governor.gun_reach(p) + p.radius + r:
				under = true
				break
		_exposed[n] = under
	return EXPOSED_WORTH if _exposed[n] else 1.0


## On the ground: order the loading or unloading once, wait for the
## spaceport, then lift off for the delivery or the next run.
func _haul_on_ground() -> void:
	if dropper:
		_drop_on_ground()
		return
	var here := ship.land_body
	if not ground_order:
		ground_order = true
		if job == "load" and here == job_planet and haul_frame != null and is_instance_valid(haul_frame):
			# everything the frame (or yard) still needs that this colony can spare
			var room := ship.cargo_space()
			var wants: Dictionary = world.supply_wants(haul_frame)
			for k in wants:
				var n := floorf(minf(minf(wants[k], _spare_for_frame(here, k)), room))
				if n >= 1.0:
					world.cargo_command(ship, k, n)
					room -= n
			return
		if job == "load" and here == job_planet and haul_market != null:
			# the run's resource first, then any other surplus the market pays well for
			var room := ship.cargo_space()
			var order: Array = [haul_res]
			for k in Economy.RESOURCES:
				if k != haul_res and Economy.is_market(haul_market) and Economy.sell_price(haul_market, k, ship) >= GameData.RESOURCE_PRICE.get(k, 1.0) * 0.8:
					order.append(k)
			for k in order:
				var n := floorf(minf(Economy.surplus(here, k), room))
				if n >= 10.0:
					world.cargo_command(ship, k, n)
					room -= n
			return
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
	if job == "load" and ship.cargo_total() >= 1.0 and haul_frame != null and is_instance_valid(haul_frame) and not haul_frame.dead:
		job = "supply"
		dock_at = haul_frame
		job_planet = null
		state = State.FERRY
	elif job == "load" and ship.cargo_total() >= 1.0 and Economy.is_market(haul_market):
		job = "sell"
		dock_at = haul_market
		job_planet = null
		state = State.FERRY
	elif job == "load" and ship.cargo_total() >= 1.0 and haul_to != null:
		job = "deliver"
		job_planet = haul_to
		last_drop = null
		state = State.FERRY
	else:
		job_planet = null
		job = ""
		plan_timer = 0.0
		state = State.FERRY if _plan_run() else State.PATROL


## Frame runs are worth this many times a colony run of the same goods,
## and runs to stock a shipyard this many.
const FRAME_PRIORITY := 4.0
const YARD_PRIORITY := 2.0


static func _priority(st: Ship) -> float:
	return FRAME_PRIORITY if st.building else YARD_PRIORITY
## A station's output is worth fetching once this much has piled up.
const COLLECT_AT := 60.0


## What a colony can give a frame: anything over a sixth of its cap.
static func _spare_for_frame(p: Planet, k: String) -> float:
	return maxf(p.stock.get(k, 0.0) - Economy.cap(p) / 6.0, 0.0)


## Fly to a station and hold alongside it, on its far side from its world.
func _fly_to_dock(delta: float) -> void:
	var st := dock_at
	var out := st.global_position - (st.station_body.global_position if st.station_body != null else ship.global_position)
	if out.length() < 1.0:
		out = Vector2.RIGHT
	var point := st.global_position + out.normalized() * (st.size + ship.size + 70.0)
	var far := ship.global_position.distance_to(point)
	if _fly_far(point, far, delta):
		return
	if ship.warping:
		ship.end_warp()
	_close_on(point, st.velocity)


## Alongside the station: order the transfer once, wait for it, then on
## to the next run.
func _haul_docked() -> void:
	if not _dock_job_good() and ship.cargo_orders.is_empty():
		dock_at = null
		state = State.FERRY if _plan_run() else State.PATROL
		return
	if world.cargo_port(ship) != dock_at:
		ground_order = false
		return
	if not ground_order:
		ground_order = true
		ground_time = 0.0
		if job == "collect":
			if dock_at.design_name == "Refinery":
				world.cargo_command(ship, "alloys", 1e6)
			else:
				world.cargo_command(ship, "all", 1.0)
		elif job == "buy":
			var n := floorf(minf(minf(ship.cargo_space(), dock_at.cargo.get(haul_res, 0.0) - 10.0), Economy.wanted(haul_to, haul_res) + 20.0))
			world.cargo_command(ship, haul_res, maxf(n, 1.0))
		elif job == "supply":
			# only what the station wants: a shipyard takes its hull materials
			var wants: Dictionary = world.supply_wants(dock_at)
			for k in ship.cargo.keys():
				if wants.get(k, 0.0) >= 0.5:
					world.cargo_command(ship, k, -minf(ship.cargo[k], wants[k]))
		else:
			world.cargo_command(ship, "all", -1.0)
		return
	ground_time += 0.35
	if not ship.cargo_orders.is_empty() or ground_time < 1.0:
		return
	if job == "buy" and ship.cargo_total() >= 1.0 and haul_to != null:
		# bought: on to the colony that was short of it
		dock_at = null
		job = "deliver"
		job_planet = haul_to
		last_drop = null
		state = State.FERRY
		return
	dock_at = null
	haul_frame = null
	haul_market = null
	job = ""
	plan_timer = 0.0
	state = State.FERRY if _plan_run() else State.PATROL


## An enemy colony nearby whose defences still stand.
func _colony_to_bombard() -> Planet:
	var gov := _governor()
	if gov != null and gov.target != null and is_instance_valid(gov.target) and gov.target.is_colonised() and gov.target.owner_race != ship.race_id \
			and not gov.target.is_defenceless() and ship.global_position.distance_to(gov.target.global_position) < 6000.0:
		return gov.target
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


## A supply ship's long haul: the general flight code, warp included,
## by way of a spot beside any enemy colony whose guns cover the straight
## line.  False when the point is close and the run's own approach takes over.
func _fly_far(point: Vector2, far: float, delta: float) -> bool:
	var way := _route(point)
	if far <= 2500.0 and way == point:
		return false
	_steer_to(way, delta, way == point)
	_avoid_planets(delta)
	if way == point and ship.qdrv > 0.0 and far > 5000.0 and not ship.warping and absf(ship.inputs["turn"]) < 0.1 and ship.energy > 400.0:
		ship.start_warp()
	return true


## The way to a point that keeps out of enemy colonies' guns and the
## Sun's heat: the point itself, or a spot beside the first of them that
## covers the straight line.  A run that starts or ends inside one can't
## help it, so that one is left out.  Worked out a few times a second.
func _route(point: Vector2) -> Vector2:
	var now := Time.get_ticks_msec() / 1000.0
	if now - _route_at < 0.4 and point.distance_to(_route_for) < 300.0:
		return point if _route_way == _route_for else _route_way
	_route_at = now
	_route_for = point
	_route_way = point
	var from := ship.global_position
	var seg := point - from
	var length := seg.length()
	if length < 1.0:
		return point
	var dir := seg / length
	var first := INF
	for p in world.planets:
		var keep := 0.0
		if p == world.sun:
			keep = p.radius * (1.0 + World.SUN_HEAT_RADII * 0.65) + GUN_MARGIN   # where the heat starts to hurt
		elif p.is_colonised() and p.owner_race != ship.race_id and p.owner_race != GameData.NEUTRAL and not p.is_defenceless():
			keep = Governor.gun_reach(p) + p.radius + GUN_MARGIN
		else:
			continue
		var c: Vector2 = p.global_position
		if c.distance_to(point) < keep or c.distance_to(from) < keep:
			continue
		var t := (c - from).dot(dir)
		if t <= 0.0 or t >= length or t >= first:
			continue
		var on_line := from + dir * t
		var miss := c.distance_to(on_line)
		if miss >= keep:
			continue
		var side := (on_line - c) / miss if miss > 1.0 else dir.orthogonal()
		first = t
		_route_way = c + side * (keep + 250.0)
	return _route_way


## The nearest enemy warship in sight within `reach`: what a supply ship
## runs from, and what a warship would rather fight.
func _nearest_warship(reach: float) -> Ship:
	var best: Ship = null
	var best_d := reach
	for s in world.ships:
		if s.dead or s.support or s.is_station or s.team() == ship.team() or s.race_id == GameData.NEUTRAL or s.is_landing() or s.is_disabled():
			continue
		var d: float = s.global_position.distance_to(ship.global_position)
		if d < best_d and s.visible_to(ship):
			best_d = d
			best = s
	return best


## Where to run from a warship: the nearest friendly colony with its guns
## up, or friendly station, counting a place the warship lies toward as
## further off.
func _shelter(foe: Ship) -> Node2D:
	var best: Node2D = null
	var best_v := INF
	var at := ship.global_position
	var from: Vector2 = (foe.global_position - at).normalized()
	var places: Array = []
	for p in world.planets:
		if p.is_colonised() and p.owner_race == ship.race_id and not p.is_defenceless() and Governor.gun_reach(p) > 0.0:
			places.append(p)
	for s in world.ships:
		if s.is_station and not s.dead and s.race_id == ship.race_id:
			places.append(s)
	for n in places:
		var to: Vector2 = n.global_position - at
		var v := to.length() * (1.0 + 2.0 * maxf(to.normalized().dot(from), 0.0))
		if v < best_v:
			best_v = v
			best = n
	return best


## Close alongside the shelter we are running to.
func _sheltered() -> bool:
	return shelter != null and is_instance_valid(shelter) and ship.global_position.distance_to(shelter.global_position) < _shelter_r() + ship.size + 800.0


func _shelter_r() -> float:
	return (shelter as Planet).radius if shelter is Planet else (shelter as Ship).size


## Run for the shelter at full speed, dropping mines behind, then hold
## alongside it under its guns.
func _flee(delta: float) -> void:
	if shelter == null or not is_instance_valid(shelter):
		return
	var i := ship.inputs
	var out := ship.global_position - shelter.global_position
	if out.length() < 1.0:
		out = Vector2.RIGHT
	var point := shelter.global_position + out.normalized() * (_shelter_r() + ship.size + 200.0)
	var far := ship.global_position.distance_to(point)
	if far < 600.0:
		if ship.warping:
			ship.end_warp()
		_close_on(point, (shelter as Planet).velocity if shelter is Planet else (shelter as Ship).velocity)
		return
	_steer_to(point, delta, true)
	_avoid_planets(delta)
	i["boost"] = ship.energy > ship.energy_max * 0.3 and far > 900.0
	if flee_from != null and is_instance_valid(flee_from):
		i["fire_mine"] = _mine_ok(flee_from, flee_from.global_position.distance_to(ship.global_position)) and randf() < 0.05
	if ship.qdrv > 0.0 and far > 4000.0 and not ship.warping and absf(i["turn"]) < 0.1 and ship.energy > 400.0:
		ship.start_warp()


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
