## Ground -- the war on the surface (the Resources update, phase 5).  A
## colony builds from its own stockpile: a planetary shield, batteries,
## barracks, industry and farms.  A barracks recruits ground units, which
## garrison the world or ride warships and drop ships to someone else's.
## Taking a populated world means landing an army that beats its garrison
## and militia: the battle runs on its own, Lanchester-style like boarding
## crews, and the world changes hands with its stockpile and buildings.
## Upkeep comes out of the stockpile too: shields burn Fuel and troops eat
## Food, so a world cut off from supply loses its shield and then its
## troops.  The host runs all of it; clients see it through Net's colony
## message.
class_name Ground
extends RefCounted

# ------------------------------------------------------------- balance

## The planetary shield soaks this much, then collapses for PSHIELD_DOWN
## seconds.  It charges at PSHIELD_CHARGE of itself a second while nobody
## is shooting at the world and its defences stand, and without Fuel it
## fades by PSHIELD_FADE a second.
const PSHIELD_HP := 5000.0
const PSHIELD_DOWN := 45.0
const PSHIELD_CHARGE := 0.03
const PSHIELD_FADE := 0.01
## Seconds to recruit a unit, and how many may wait at a barracks.
const RECRUIT_TIME := 30.0
const RECRUIT_QUEUE := 4
## The fight on the ground: each point of strength takes out this much of
## the other side's a second (Lanchester's square law, as in boarding).
const GROUND_KILLS := 0.04
## Defenders behind the bunkers of a barracks, while the colony's
## defences stand.
const BUNKER_FACTOR := 1.5
## Militia: raised free from the people when a world is invaded, this much
## strength per 100 M of them, up to the cap.
const MILITIA_PER_100M := 0.2
const MILITIA_CAP := 5.0
## Warships in orbit over a battle: each within the range adds this much
## strength to the attackers (up to the cap), and kills this share of the
## people a second unless the planetary shield stands.
const SUPPORT_RANGE := 1200.0
const SUPPORT_STRENGTH := 0.5
const SUPPORT_CAP := 3.0
const SUPPORT_KILLS := 0.003
## Commandos on the ground blow up a shield generator or a battery this
## often, in this order.
const SABOTAGE_TIME := 20.0
const SABOTAGE_ORDER := ["shield", "silo", "laser", "flak"]
## A unit deserts this often while its world can't feed or fuel it.
const DESERT_TIME := 90.0
## Flak: its reach beyond the surface, how often it shoots down a missile,
## and its damage a second to an enemy ship on its way down or up.
const FLAK_RANGE := 750.0
const FLAK_RATE := 0.4
const FLAK_DPS := 220.0
## The batteries the colony's guns gain.
const LASER_RANGE := 1100.0
const LASER_DPS := 90.0
const SILO_RANGE := 1400.0
const SILO_RATE := 1.6
## Hydroponic farms: Food a minute with a full workforce, whatever the climate.
const FARM_FOOD := 8.0
## Industry: Alloys a minute it adds to the colony's refining, and its
## share on top of the Metals output.
const INDUSTRY_REFINE := 4.0
const INDUSTRY_METALS := 0.5
## The empire governors (Governor.gd) keep every barracks recruiting up
## to this much strength, half as much again where the enemy comes.
const GARRISON_TARGET := {"Outpost": 2.0, "Minor": 4.0, "Major": 7.0}
## What a computer drop ship leaves behind when it takes troops away: at
## least this much, and half the world's garrison target.
const GARRISON_KEEP := 2.0
## Marines aboard a boarding ship: each infantry unit makes the party
## fight this much harder, and commandos double it.
const MARINE_BONUS := 0.5

var world: Node
var _second := 0.0


func _init(p_world: Node = null) -> void:
	world = p_world


# ------------------------------------------------------------- strength

static func strength(units: Dictionary) -> float:
	var n := 0.0
	for u in units:
		n += float(units[u]) * GameData.UNITS[u]["strength"]
	return n


## Whole units, as a pilot counts them (a battle leaves fractions behind
## until it is over).
static func shown(n: float) -> int:
	return int(ceil(n - 0.05))


static func count(units: Dictionary) -> int:
	var n := 0
	for u in units:
		n += shown(units[u])
	return n


## "3 infantry, 1 armour"
static func units_text(units: Dictionary) -> String:
	var parts := []
	for u in GameData.UNIT_ORDER:
		var n := shown(units.get(u, 0.0))
		if n > 0:
			parts.append("%d %s" % [n, GameData.UNITS[u]["label"].to_lower()])
	return ", ".join(parts) if not parts.is_empty() else "none"


## The militia a world would raise now.
static func militia(p: Planet) -> float:
	return minf(p.population / 1e8 * MILITIA_PER_100M, MILITIA_CAP)


## Everything a world fights a landing with: its garrison and its militia
## (the battle's own militia while one is on).
static func defence(p: Planet) -> float:
	var m: float = p.battle.get("militia", militia(p)) if not p.battle.is_empty() else militia(p)
	return strength(p.garrison) + m


static func bunker_factor(p: Planet) -> float:
	return BUNKER_FACTOR if p.buildings.has("barracks") and not p.is_defenceless() else 1.0


## Seconds until the defenders fall to `attack` (plus `support` from
## orbit, which never tires), or INF when the attackers would lose.
static func estimate(attack: float, support: float, defend: float, factor: float) -> float:
	var a := attack
	var d := defend
	var t := 0.0
	var dt := 0.25
	while d > 0.05:
		if a <= 0.05 or t > 900.0:
			return INF
		var da := d * factor * GROUND_KILLS * dt
		d -= (a + support) * GROUND_KILLS * dt
		a -= da
		t += dt
	return t


## How a boarding party fights with marines aboard (see Capture).
static func board_power(s: Ship) -> float:
	var f := 1.0 + MARINE_BONUS * float(shown(s.troops.get("infantry", 0.0)))
	if s.troops.get("commandos", 0.0) >= 0.5:
		f *= 2.0
	return f


# ---------------------------------------------------------- buildings

static func slots(p: Planet) -> int:
	return int(GameData.BUILDING_SLOTS.get(p.colony_design, 0))


static func cost_text(cost: Dictionary) -> String:
	var parts := []
	for k in cost:
		parts.append("%d %s" % [cost[k], Economy.NAMES[k]])
	return ", ".join(parts)


static func _short_of(stock: Dictionary, cost: Dictionary) -> String:
	for k in cost:
		if stock.get(k, 0.0) < float(cost[k]) - 0.01:
			return "Needs %s (%s has %d %s)." % [cost_text(cost), "the stockpile", int(stock.get(k, 0.0)), Economy.NAMES[k]]
	return ""


## Why `s` (null: the empire itself) can't put up `key` on `p`, or "".
static func building_refusal(s: Ship, p: Planet, key: String) -> String:
	if not GameData.BUILDINGS.has(key):
		return "Nobody builds that."
	if s != null:
		if not s.is_landed() or s.land_body != p:
			return "Land on one of your colonies to build there."
		if s.rank < GameData.KIT_RANK and not s.support:
			return "Only a %s or higher may order planetary buildings." % GameData.RANKS[GameData.KIT_RANK]["name"]
	if not p.is_colonised() or (s != null and p.owner_race != s.race_id):
		return "Only your own colonies build."
	if not p.battle.is_empty():
		return "Not while the battle for %s is on." % p.body_name
	if p.buildings.has(key) or p.build_q.any(func(e): return e[0] == key):
		return "%s already has one." % p.body_name
	if not p.build_q.is_empty():
		return "The builders are busy with the %s." % GameData.BUILDINGS[p.build_q[0][0]]["label"].to_lower()
	if p.buildings.size() >= slots(p):
		return "No free building slots (%s %d of %d)." % [p.colony_design.to_lower(), p.buildings.size(), slots(p)]
	return _short_of(p.stock, GameData.BUILDINGS[key]["cost"])


## Why `s` (null: the empire itself) can't recruit `unit` on `p`, or "".
static func recruit_refusal(s: Ship, p: Planet, unit: String) -> String:
	if not GameData.UNITS.has(unit):
		return "Nobody trains those."
	if s != null:
		if not s.is_landed() or s.land_body != p:
			return "Land on one of your colonies to recruit there."
		if s.rank < GameData.KIT_RANK and not s.support:
			return "Only a %s or higher may recruit." % GameData.RANKS[GameData.KIT_RANK]["name"]
	if not p.is_colonised() or (s != null and p.owner_race != s.race_id):
		return "Only your own colonies recruit."
	if not p.buildings.has("barracks"):
		return "%s has no barracks: build one first." % p.body_name
	if not p.battle.is_empty():
		return "Not while the battle for %s is on." % p.body_name
	if p.recruit_q.size() >= RECRUIT_QUEUE:
		return "The barracks is full: %d units in training." % p.recruit_q.size()
	var d: Dictionary = GameData.UNITS[unit]
	var people: float = d["people"]
	if people > 0.0 and p.population - people < maxf(10e6, people):
		return "%s can't spare %s people for it." % [p.body_name, GameData.fmt_pop(people)]
	return _short_of(p.stock, d["cost"])


## Start a building: its cost leaves the stockpile now.
static func queue_building(p: Planet, key: String) -> void:
	var b: Dictionary = GameData.BUILDINGS[key]
	for k in b["cost"]:
		p.stock[k] = maxf(p.stock.get(k, 0.0) - float(b["cost"][k]), 0.0)
	p.build_q.append([key, float(b["build"])])


static func queue_recruit(p: Planet, unit: String) -> void:
	var d: Dictionary = GameData.UNITS[unit]
	for k in d["cost"]:
		p.stock[k] = maxf(p.stock.get(k, 0.0) - float(d["cost"][k]), 0.0)
	p.population -= float(d["people"])
	p.recruit_q.append([unit, RECRUIT_TIME])


# -------------------------------------------------------------- troops

## Why `s` can't do `op` with its troops now ("load", "unload" or "land";
## `unit` "" means every unit), or "".
static func troop_refusal(s: Ship, op: String, unit: String) -> String:
	if not s.is_landed():
		return "Land first (B): troops go aboard and ashore on the ground."
	var p := s.land_body
	if s.troop_max <= 0:
		return "This hull carries no troops."
	if not p.is_colonised():
		return "Nobody lives here: troops only garrison colonies."
	var ours := p.owner_race == s.race_id
	match op:
		"load":
			if not ours:
				return "Troops only board at your own colonies."
			if not p.battle.is_empty():
				return "Not while the battle for %s is on." % p.body_name
			if s.troop_room() < 1:
				return "The troop bays are full (%d of %d)." % [s.troop_count(), s.troop_max]
			if unit != "" and p.garrison.get(unit, 0.0) < 0.95:
				return "No %s here to take aboard." % GameData.UNITS[unit]["label"].to_lower()
			if unit == "" and count(p.garrison) <= 0:
				return "No troops here to take aboard."
		"unload":
			if not ours:
				return "That's an enemy colony: send the troops in to fight for it."
			if unit != "" and s.troops.get(unit, 0.0) < 0.95:
				return "No %s aboard." % GameData.UNITS[unit]["label"].to_lower()
			if s.troop_count() <= 0:
				return "No troops aboard."
		"land":
			if ours:
				return "%s is yours already: put the troops ashore to garrison it." % p.body_name
			if s.troop_count() <= 0:
				return "No troops aboard. Take some on at one of your colonies with a garrison."
			if not p.battle.is_empty() and int(p.battle["race"]) != s.race_id:
				return "The %s are already fighting for %s." % [GameData.race(int(p.battle["race"]))["name"], p.body_name]
		_:
			return "?"
	return ""


static func _move(from: Dictionary, to: Dictionary, unit: String, n: float) -> void:
	from[unit] = from.get(unit, 0.0) - n
	if from[unit] < 0.05:
		from.erase(unit)
	to[unit] = to.get(unit, 0.0) + n


## Troops aboard or ashore at one of our colonies.  `unit` "": all of
## them (aboard: as many as the bays hold, infantry first).
static func move_troops(s: Ship, op: String, unit: String) -> int:
	var p := s.land_body
	var moved := 0
	var kinds: Array = [unit] if unit != "" else GameData.UNIT_ORDER
	for u in kinds:
		if op == "load":
			while s.troop_room() >= 1 and p.garrison.get(u, 0.0) >= 0.95:
				_move(p.garrison, s.troops, u, 1.0)
				moved += 1
				if unit != "":
					return moved
		else:
			while s.troops.get(u, 0.0) >= 0.95:
				_move(s.troops, p.garrison, u, 1.0)
				moved += 1
				if unit != "":
					return moved
	return moved


## The troops aboard go in against an enemy colony: a battle starts, or
## they join the one their side is already fighting.
func land(s: Ship) -> void:
	var p := s.land_body
	var fresh := p.battle.is_empty()
	if fresh:
		p.battle = {"race": s.race_id, "units": {}, "militia": militia(p), "pilot": s.pilot_name, "sabotage": 0.0}
	var sent := units_text(s.troops)
	for u in s.troops.keys():
		_move(s.troops, p.battle["units"], u, s.troops[u])
	s.troops.clear()
	p.battle["pilot"] = s.pilot_name
	p.under_attack = maxf(p.under_attack, 8.0)
	var est := estimate(strength(p.battle["units"]), _support(p, s.race_id)[1], defence(p), bunker_factor(p))
	world.notify(s, "Troops ashore on %s: %s against %s%s. %s" % [p.body_name, sent, _defenders_text(p),
		"" if bunker_factor(p) <= 1.0 else " behind bunkers", ("About %d s to take it." % int(ceil(est))) if est < INF else "Warning: they are outnumbered."])
	if fresh:
		world.notify_all("Ground battle on %s: the %s have landed." % [p.body_name, GameData.race(s.race_id)["name"]], GameData.race(s.race_id)["color"])
	Sfx.play("dock", p.global_position, 0.7, 0.7)
	Net.host_colony_changed(p)


static func _defenders_text(p: Planet) -> String:
	var g := units_text(p.garrison)
	var m: float = p.battle.get("militia", militia(p))
	return ("%s and militia %.1f" % [g, m]) if g != "none" else ("militia %.1f" % m)


# ------------------------------------------------------------ the host

## Every host physics frame: battles, flak, the queues; upkeep once a
## second.
func tick(delta: float) -> void:
	for p in world.planets:
		if not p.is_colonised():
			continue
		if not p.battle.is_empty():
			_battle(p, delta)
			if not p.is_colonised():
				continue
		if p.buildings.has("flak") and not p.is_defenceless():
			_flak(p, delta)
		_queues(p, delta)
	_second += delta
	if _second >= 1.0:
		var dt := _second
		_second = 0.0
		for p in world.planets:
			if p.is_colonised():
				_upkeep(p, dt)


## Warships of `race` over the battle: [how many, their support strength].
func _support(p: Planet, race: int) -> Array:
	var n := 0
	for s in world.ships:
		if s.dead or s.race_id != race or s.is_landing() or s.support or s.is_station or s.weapons.is_empty():
			continue
		if s.global_position.distance_to(p.global_position) <= p.radius + SUPPORT_RANGE:
			n += 1
	return [n, minf(n * SUPPORT_STRENGTH, SUPPORT_CAP)]


static func _thin(units: Dictionary, k: float) -> void:
	for u in units.keys():
		units[u] = units[u] * k
		if units[u] < 0.02:
			units.erase(u)


func _battle(p: Planet, delta: float) -> void:
	var b: Dictionary = p.battle
	var atk: Dictionary = b["units"]
	var race := int(b["race"])
	var a := strength(atk)
	var d := strength(p.garrison) + float(b["militia"])
	var sup := _support(p, race)
	var to_d: float = (a + sup[1]) * GROUND_KILLS * delta
	var to_a := d * bunker_factor(p) * GROUND_KILLS * delta
	if d > 0.0:
		var kd := maxf(1.0 - to_d / d, 0.0)
		_thin(p.garrison, kd)
		b["militia"] = float(b["militia"]) * kd
	if a > 0.0:
		_thin(atk, maxf(1.0 - to_a / a, 0.0))
	p.under_attack = maxf(p.under_attack, 1.0)
	# fire from orbit helps, and falls on the people as well
	if sup[0] > 0 and not p.pshield_up():
		p.population = maxf(p.population - p.population * SUPPORT_KILLS * sup[0] * delta, GameData.MIN_COLONY * 1.5)
	if atk.get("commandos", 0.0) >= 0.5:
		b["sabotage"] = float(b["sabotage"]) + delta
		if b["sabotage"] >= SABOTAGE_TIME:
			b["sabotage"] = 0.0
			_sabotage(p, race)
	if strength(p.garrison) + float(b["militia"]) <= 0.05:
		_won(p)
	elif strength(atk) <= 0.05:
		_repelled(p)


func _sabotage(p: Planet, race: int) -> void:
	for key in SABOTAGE_ORDER:
		if p.buildings.has(key):
			p.buildings.erase(key)
			if key == "shield":
				p.pshield = 0.0
				p.pshield_down = 0.0
			p.fit_batteries()
			world.fx.explosion(p.global_position + Vector2.RIGHT.rotated(randf() * TAU) * p.radius * 0.6, 26.0, Color(1.0, 0.6, 0.3))
			world.notify_all("Commandos blew up %s's %s." % [p.body_name, GameData.BUILDINGS[key]["label"].to_lower()], GameData.race(race)["color"])
			Net.host_colony_changed(p)
			return


## Whole units again once the fighting stops: a unit worn to a fraction
## still counts, one worn to nearly nothing does not.
static func _whole(units: Dictionary) -> Dictionary:
	var out := {}
	for u in units:
		var n := roundf(units[u])
		if n < 1.0 and units[u] >= 0.3:
			n = 1.0
		if n >= 1.0:
			out[u] = n
	return out


func _won(p: Planet) -> void:
	var b: Dictionary = p.battle
	var race := int(b["race"])
	var survivors := _whole(b["units"])
	if survivors.is_empty():
		survivors = {"infantry": 1.0}
	var pilot: Ship = world.pilot_named(str(b["pilot"]))
	p.battle = {}
	p.garrison = {}
	p.recruit_q.clear()
	p.build_q.clear()
	if pilot != null:
		world.notify(pilot, "%s is taken: %s hold it." % [p.body_name, units_text(survivors)])
	world.planet_captured(p, race, pilot, p.population)
	p.garrison = survivors
	Net.host_colony_changed(p)


func _repelled(p: Planet) -> void:
	var race := int(p.battle["race"])
	p.battle = {}
	p.garrison = _whole(p.garrison)
	world.notify_all("The %s landing on %s was wiped out: %s holds." % [GameData.race(race)["adj"], p.body_name, units_text(p.garrison) if count(p.garrison) > 0 else "its militia"],
		GameData.race(p.owner_race)["color"])
	Net.host_colony_changed(p)


func _flak(p: Planet, delta: float) -> void:
	p.flak_t -= delta
	if p.flak_t <= 0.0:
		p.flak_t = FLAK_RATE
		for m in world.missiles:
			if not is_instance_valid(m) or m.exploded or m.owner_race == p.owner_race or m.owner_race == GameData.NEUTRAL:
				continue
			if m.global_position.distance_to(p.global_position) > p.radius + FLAK_RANGE:
				continue
			var from: Vector2 = p.global_position + (m.global_position - p.global_position).normalized() * p.radius
			world.add_beam(from, m.global_position, Color(1.0, 0.85, 0.5))
			world.spawn_sparks(m.global_position, 6, Color(1.0, 0.8, 0.4))
			m._free()
			break
	for s in world.ships:
		if s.dead or s.land_body != p or s.race_id == p.owner_race or s.race_id == GameData.NEUTRAL:
			continue
		if s.landing == Ship.Landing.DESCENDING or s.landing == Ship.Landing.ASCENDING:
			s.take_damage(FLAK_DPS * delta, p, s.global_position)


func _queues(p: Planet, delta: float) -> void:
	if not p.build_q.is_empty():
		p.build_q[0][1] = float(p.build_q[0][1]) - delta
		if p.build_q[0][1] <= 0.0:
			var key: String = p.build_q.pop_front()[0]
			p.buildings.append(key)
			p.fit_batteries()
			world.notify_race(p.owner_race, "%s finished its %s." % [p.body_name, GameData.BUILDINGS[key]["label"].to_lower()])
			Net.host_colony_changed(p)
	if not p.recruit_q.is_empty() and p.battle.is_empty():
		p.recruit_q[0][1] = float(p.recruit_q[0][1]) - delta
		if p.recruit_q[0][1] <= 0.0:
			var u: String = p.recruit_q.pop_front()[0]
			p.garrison[u] = p.garrison.get(u, 0.0) + 1.0
			Net.host_colony_changed(p)


## The world feeds and fuels what it has; a shortfall fades the shield
## (Planet._process) and, after a while, costs a unit.
## What a world's buildings and garrison eat a minute.
static func upkeep_rate(p: Planet) -> Dictionary:
	var need := {}
	for key in p.buildings:
		var up: Dictionary = GameData.BUILDINGS[key]["upkeep"]
		for k in up:
			need[k] = need.get(k, 0.0) + float(up[k])
	for u in p.garrison:
		var up: Dictionary = GameData.UNITS[u]["upkeep"]
		for k in up:
			need[k] = need.get(k, 0.0) + float(up[k]) * p.garrison[u]
	return need


func _upkeep(p: Planet, dt: float) -> void:
	var need := upkeep_rate(p)
	for k in need:
		need[k] *= dt / 60.0
	var short := {}
	for k in need:
		var have: float = p.stock.get(k, 0.0)
		if have >= need[k]:
			p.stock[k] = have - need[k]
		else:
			p.stock[k] = 0.0
			short[k] = true
	var was_short: bool = p.short.has("fuel") or p.short.has("food")
	p.short = short
	var hungry: Array = []
	for u in p.garrison:
		for k in GameData.UNITS[u]["upkeep"]:
			if short.has(k) and not hungry.has(u):
				hungry.append(u)
	if hungry.is_empty():
		p.desert_t = 0.0
	else:
		p.desert_t += dt
		if p.desert_t >= DESERT_TIME:
			p.desert_t = 0.0
			var u: String = hungry[0]
			p.garrison[u] = p.garrison[u] - 1.0
			if p.garrison[u] < 0.05:
				p.garrison.erase(u)
			world.notify_race(p.owner_race, "%s can't keep its troops: a unit of %s deserted (short of %s)." % [p.body_name,
				GameData.UNITS[u]["label"].to_lower(), " and ".join(short.keys().map(func(k): return Economy.NAMES[k]))])
			Net.host_colony_changed(p)
	if was_short != (short.has("fuel") or short.has("food")):
		Net.host_colony_changed(p)


## A computer drop ship's share of a garrison: what is over the keep.
static func spare_troops(p: Planet) -> float:
	if not p.battle.is_empty():
		return 0.0
	var keep := maxf(GARRISON_KEEP, float(GARRISON_TARGET.get(p.colony_design, 2.0)) * 0.5)
	return maxf(strength(p.garrison) - keep, 0.0)


# ------------------------------------------------------------ set-up

## A fresh Sol game: the colonies' barracks, garrisons and the home
## worlds' shields (with Fuel to run them for a while).
func seed_all() -> void:
	for p in world.planets:
		p.clear_ground()
		if p.is_colonised():
			seed_planet(p, GameData.STARBASES.get(p.owner_race, "") == p.body_name)


static func seed_planet(p: Planet, home: bool) -> void:
	var start: Dictionary = GameData.START_GROUND.get(p.colony_design, {})
	p.buildings = start.get("buildings", []).duplicate()
	if home:
		for key in GameData.HOME_BUILDINGS:
			if not p.buildings.has(key):
				p.buildings.append(key)
	p.garrison = {}
	var g: Dictionary = start.get("garrison", {})
	for u in g:
		p.garrison[u] = float(g[u])
	if p.owner_race > 2:
		p.garrison = {}                 # the pirates' rocks: militia only
	if p.buildings.has("shield"):
		p.pshield = PSHIELD_HP
		p.stock["fuel"] = maxf(p.stock.get("fuel", 0.0), 60.0)
	p.fit_batteries()


# ------------------------------------------------- the wire and the save

static func pack(p: Planet) -> Dictionary:
	return {"b": p.buildings.duplicate(), "bq": p.build_q.duplicate(true), "g": p.garrison.duplicate(), "rq": p.recruit_q.duplicate(true),
		"ps": p.pshield, "pd": p.pshield_down, "bt": p.battle.duplicate(true), "sh": p.short.duplicate(), "dt": p.desert_t}


static func unpack(p: Planet, d: Dictionary) -> void:
	var refit: bool = d.get("b", []) != p.buildings
	p.buildings = d.get("b", []).duplicate()
	p.build_q = d.get("bq", []).duplicate(true)
	p.garrison = {}
	var g: Dictionary = d.get("g", {})
	for u in g:
		p.garrison[u] = float(g[u])
	p.recruit_q = d.get("rq", []).duplicate(true)
	p.pshield = float(d.get("ps", 0.0))
	p.pshield_down = float(d.get("pd", 0.0))
	p.battle = d.get("bt", {}).duplicate(true)
	p.short = d.get("sh", {}).duplicate()
	p.desert_t = float(d.get("dt", 0.0))
	if refit:
		p.fit_batteries()
