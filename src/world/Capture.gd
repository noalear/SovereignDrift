## Capture -- boarding disabled ships, as the original game did with its
## docking tractor: close on a ship whose engines or reactor are shot out,
## lock on (K), and the tractor pulls it alongside and matches speeds
## while your crew fights its crew aboard.  Stay close: when the last
## defender falls the ship is yours, and if your boarders fall below a
## quarter of your crew they are thrown back.  Crews are thinned by hits
## (Ship.take_damage), so a battered ship is quick to take.  Marines
## aboard (infantry or commandos in the troop bays) make a boarding party
## fight harder (Ground.board_power).
##
## A ship flown by the computer becomes a prize: a prize crew patches it
## up and it flies as your wingman under your colours (the pilot who lost
## it relaunches at home in a new ship).  A ship flown by a person is
## stripped for salvage instead, and that pilot relaunches as if killed.
## The host runs all of this; clients see it through Net.
class_name Capture
extends RefCounted

# ------------------------------------------------------------- balance

## Lock-on range beyond the two hulls' sizes, in pixels.
const BOARD_RANGE := 220.0
## The crew can only cross while the two ships move this slowly against
## each other (the tractor takes care of it once locked).
const BOARD_SPEED := 70.0
## The fight aboard: each boarder takes out this many defenders a second,
## and each defender this many boarders (defenders know the ship).
const BOARDER_KILLS := 0.15
const DEFENDER_KILLS := 0.11
## The boarders pull back when they are down to this share of their crew.
const REPELLED_AT := 0.25
## Share of the captor's crew that stays aboard a prize to fly it.
const PRIZE_CREW_SHARE := 0.35
## Seconds the lock survives being out of range or too fast.
const BOARD_GRACE := 1.5
## How hard the tractor matches the prize's motion to yours, per second.
const TRACTOR_RATE := 2.5
## Prizes a pilot can keep flying; another capture sends the oldest home.
const MAX_PRIZES_PER_PILOT := 2
## Prizes in the whole system; past this a capture is salvaged instead.
const MAX_PRIZES := 10

var world: Node


func _init(p_world: Node) -> void:
	world = p_world


## How a boarding of `victim` by `captor` would go, in seconds, or INF
## when the boarders would be thrown back.  Lanchester's square law: the
## side with more hands left wins, and quicker the more it has.
static func board_estimate(captor: Node, victim: Node) -> float:
	var a: float = captor.crew
	var d: float = victim.crew
	var floor_a: float = captor.crew_max * REPELLED_AT
	var power := Ground.board_power(captor)
	var t := 0.0
	var dt := 0.1
	while d > 0.05:
		if a <= floor_a or t > 60.0:
			return INF
		var da := d * DEFENDER_KILLS * dt
		d -= a * BOARDER_KILLS * power * dt
		a -= da
		t += dt
	return t


static func board_range(captor: Node, victim: Node) -> float:
	return captor.size + victim.size + BOARD_RANGE


## Why `captor` cannot board `victim` now, or "" when it can.  "far" and
## "fast" are the ones a pilot can fix by flying.
static func why_not(captor: Node, victim: Node) -> String:
	if victim == null or not is_instance_valid(victim) or not victim is Ship or victim.dead:
		return "no target"
	if captor.world != null and captor.world.get("duel_rules") != null:
		return "duel"
	if captor.world != null and captor.world.get("mode_rules") != null:
		return "arena"
	if victim == captor or victim.race_id == captor.race_id:
		return "friendly"
	if captor.is_station or victim.is_station:
		return "station"
	if victim.is_landing():
		return "landed"
	if not victim.is_disabled():
		return "not disabled"
	if victim.boarded_by != null and is_instance_valid(victim.boarded_by) and victim.boarded_by != captor and not victim.boarded_by.dead:
		return "taken"
	if captor.crew <= captor.crew_max * REPELLED_AT:
		return "crew"
	if captor.warping:
		return "warp"
	if captor.global_position.distance_to(victim.global_position) > board_range(captor, victim):
		return "far"
	return ""


static func reason_text(why: String, captor: Node, victim: Node) -> String:
	match why:
		"no target":
			return "No target to board."
		"friendly":
			return "That ship is on your side."
		"duel":
			return "No boarding in a duel."
		"arena":
			return "No boarding in %s." % captor.world.mode.capitalize()
		"station":
			return "Stations can't be boarded."
		"landed":
			return "It's down on the surface. Wait for it to lift off."
		"not disabled":
			return "Disable it first: shoot out its engines (from behind) or its reactor."
		"taken":
			return "Someone else is already boarding it."
		"crew":
			return "Not enough crew to board (%d of %d). Take on more at a friendly base or colony." % [int(ceil(captor.crew)), captor.crew_max]
		"warp":
			return "Drop out of warp to board."
		"far":
			return "Close to %d to board (now %d)." % [int(board_range(captor, victim)), int(captor.global_position.distance_to(victim.global_position))]
	return why


## Lock the tractor on and start boarding.  Says why not when it can't.
func try_board(captor: Ship, victim: Node) -> bool:
	if captor.boarding != null and captor.boarding == victim:
		cancel(captor, "Boarding called off.")
		return false
	var why := why_not(captor, victim)
	if why != "":
		world.notify(captor, reason_text(why, captor, victim))
		return false
	if captor.boarding != null:
		cancel(captor, "")
	captor.boarding = victim
	captor.board_progress = 0.0
	captor.board_lost = 0.0
	captor.board_start_crew = maxf(victim.crew, 0.01)
	victim.boarded_by = captor
	Sfx.play("dock", captor.global_position, 0.8)
	var est := board_estimate(captor, victim)
	var marines := (" and %s" % Ground.units_text(captor.troops)) if captor.troop_count() > 0 else ""
	world.notify(captor, ("Tractor locked on %s. Boarding: your %d crew%s against their %d, about %d s. Stay close." % [victim.ship_name, int(ceil(captor.crew)), marines, int(ceil(victim.crew)), int(ceil(est))]) if est < INF
		else ("Tractor locked on %s. Warning: their %d crew will likely throw your %d back." % [victim.ship_name, int(ceil(victim.crew)), int(ceil(captor.crew))]))
	world.notify(victim, "%s is boarding us! Fight them off or get the engines back." % captor.pilot_name)
	return true


func cancel(captor: Ship, why: String) -> void:
	var v: Ship = captor.boarding
	if v != null and is_instance_valid(v) and v.boarded_by == captor:
		v.boarded_by = null
	captor.boarding = null
	captor.board_progress = 0.0
	captor.board_lost = 0.0
	if why != "":
		world.notify(captor, why)


## Every host physics frame: hold, pull and count every boarding.
func tick(delta: float) -> void:
	for s: Ship in world.ships.duplicate():
		if s.boarding == null:
			if s.boarded_by != null and (not is_instance_valid(s.boarded_by) or s.boarded_by.boarding != s):
				s.boarded_by = null
			continue
		var v: Ship = s.boarding
		if s.dead or not is_instance_valid(v) or v.dead:
			cancel(s, "")
			continue
		var why := why_not(s, v)
		if why == "not disabled":
			cancel(s, "Boarding failed: %s got its systems back." % v.ship_name)
			continue
		if why != "" and why != "far":
			cancel(s, "Boarding called off: " + reason_text(why, s, v))
			continue
		var d := s.global_position.distance_to(v.global_position)
		var rel := v.velocity - s.velocity
		# the tractor: match the prize's motion to ours and hold it alongside
		if d < board_range(s, v) * 1.6:
			var want := (s.size + v.size) * 1.15
			var dir := (v.global_position - s.global_position) / maxf(d, 0.001)
			var dv := -rel * minf(TRACTOR_RATE * delta, 1.0) - dir * clampf(d - want, -60.0, 60.0) * minf(TRACTOR_RATE * delta, 1.0)
			v.apply_hit_impulse(dv * v.body.mass, v.global_position)
		if why == "far" or rel.length() > BOARD_SPEED:
			s.board_lost += delta
			if s.board_lost > BOARD_GRACE:
				cancel(s, "Boarding lost: %s." % ("out of range" if why == "far" else "too fast"))
			continue
		s.board_lost = 0.0
		# the fight aboard
		var lost := v.crew * DEFENDER_KILLS * delta
		v.crew = maxf(v.crew - s.crew * BOARDER_KILLS * Ground.board_power(s) * delta, 0.0)
		s.crew = maxf(s.crew - lost, 0.0)
		s.board_progress = clampf(1.0 - v.crew / s.board_start_crew, 0.0, 1.0)
		if v.crew <= 0.05:
			take(s, v)
		elif s.crew <= s.crew_max * REPELLED_AT:
			cancel(s, "Boarders repelled: %d crew left." % int(ceil(s.crew)))
			world.notify(v, "Boarders thrown back!")


## The ship is ours.
func take(captor: Ship, victim: Ship) -> void:
	cancel(captor, "")
	var human: bool = victim.is_player or victim.net_id in Net.seats.values()
	var prizes := prizes_of(captor)
	var total := 0
	for s in world.ships:
		if s.is_prize and not s.dead:
			total += 1
	var keep := not human and total < MAX_PRIZES
	if keep and prizes.size() >= MAX_PRIZES_PER_PILOT:
		send_home(prizes[0])
	var points := 15 + victim.rank * 3
	captor.add_score(points, true)
	captor.ships_captured += 1
	var salvage := 60 + victim.rank * 10 + int(GameData.HULL_PRICE.get(victim.design_name, 300) * 0.3)
	if not keep:
		salvage *= 2
	if captor.is_player or captor.net_id in Net.seats.values():
		captor.earn(salvage)
		# their empire's parts come with the ship
		for id in victim.design["modules"]:
			var r := GameData.part_race(id)
			if r >= 0 and r != captor.race_id and not captor.blueprints.has(id):
				captor.blueprints.append(id)
		world.notify(captor, ("Captured %s! A prize crew takes her: she flies with you. Salvage %d credits." if keep
			else "Captured %s! Stripped for salvage: %d credits.") % [victim.ship_name, salvage])
	var round_rules = world.round_rules
	if round_rules != null:
		round_rules.on_kill(captor.race_id)
		round_rules.note("%s captured %s's %s" % [captor.pilot_name, victim.pilot_name, victim.design_name], captor.race_id)
	world.notify_all("%s (%s) captured %s's %s." % [captor.pilot_name, GameData.race(captor.race_id)["name"], victim.pilot_name, victim.design_name],
		captor.race_color() if captor.is_player or victim.is_player else Color(0.85, 0.85, 0.75))
	if victim.is_player:
		world.notify(victim, "Your ship was boarded and captured.")
	Sfx.play_ui("captured", 0.7)
	var prize: Ship = _make_prize(captor, victim) if keep else null
	_take_cargo(captor, victim, prize)
	for s in world.ships:
		if s.target == victim:
			s.target = prize if s.race_id != captor.race_id else null
	# the old pilot leaves without a blast and relaunches at home
	victim.captured = true
	victim._die(captor)
	if prize != null:
		Net.host_ship_added(prize)


## The boarders take what the hold carried, as much as their own hold has
## room for; the rest stays aboard the prize (or is lost with a stripped
## hull).
func _take_cargo(captor: Ship, victim: Ship, prize: Ship) -> void:
	if victim.cargo.is_empty():
		return
	var taken := 0.0
	for k in victim.cargo.keys():
		var n := minf(victim.cargo[k], captor.cargo_space())
		if n >= 1.0:
			captor.cargo[k] = captor.cargo.get(k, 0.0) + n
			victim.cargo[k] -= n
			taken += n
		if prize != null and victim.cargo[k] >= 1.0:
			prize.cargo[k] = prize.cargo.get(k, 0.0) + minf(victim.cargo[k], prize.cargo_space())
	if taken >= 1.0:
		# plunder pays like salvage when it is brought home
		if captor.cargo_total() - taken < 1.0:
			captor.cargo_origin = null
		world.notify(captor, "Took %d units of cargo from %s (hold %d / %d)." % [int(taken), victim.ship_name, int(captor.cargo_total()), int(captor.cargo_max)])


func prizes_of(captor: Ship) -> Array:
	var out := []
	for s in world.ships:
		if s.is_prize and not s.dead and s.prize_of == captor:
			out.append(s)
	return out


func _make_prize(captor: Ship, victim: Ship) -> Ship:
	var p: Ship = world._make_ship("Prize crew", captor.race_id, victim.design_name, victim.ship_name, false, victim.hull_race)
	p.is_prize = true
	p.prize_of = captor
	p.refit(victim.design.duplicate(true))
	p.global_position = victim.global_position
	p.velocity = victim.velocity
	p.rotation = victim.rotation
	p.hull = victim.hull
	# part of the boarding party stays aboard to fly her
	var hands := clampf(captor.crew * PRIZE_CREW_SHARE, 1.0, float(p.crew_max))
	p.crew = hands
	captor.crew = maxf(captor.crew - hands, 0.0)
	p.fuel = victim.fuel
	p.energy = victim.energy
	p.systems.copy_from(victim.systems)
	p.systems.jury_rig()
	p.apply_systems()
	p.body.pos = p.global_position
	p.body.vel = p.velocity
	p.body.angle = p.rotation
	for i in mini(p.weapons.size(), victim.weapons.size()):
		p.weapons[i]["ammo"] = victim.weapons[i]["ammo"]
	var ai := AIPilot.new()
	ai.name = "AI"
	p.add_child(ai)
	ai.setup(p, world, world._home_planet(captor.race_id), 3)
	ai.escort = captor
	p.invulnerable = 1.5
	return p


## A prize over the limit jumps home to the shipyard and is sold.
func send_home(prize: Ship) -> void:
	var owner: Ship = prize.prize_of
	var pay := 40 + int(GameData.HULL_PRICE.get(prize.design_name, 300) * 0.3)
	if owner != null and is_instance_valid(owner):
		owner.earn(pay)
		world.notify(owner, "Prize %s sent home to the shipyard: %d credits." % [prize.ship_name, pay])
	world.fx.explosion(prize.global_position, prize.size, Color(0.6, 0.8, 1.0), false)
	Sfx.play("warpout", prize.global_position, 0.6)
	world.remove_ship(prize)
