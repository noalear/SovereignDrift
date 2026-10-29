## Ship -- a flyable, fightable starship (or starbase) built from a design
## in GameData.  Flight is physical: the engines push a hull that has mass
## (modules plus fuel) and rotational inertia, nothing slows you down but
## your own engines, brakes and gravity, and hits shove and spin you.  The
## ship owns a PhysicsBody in the world's PhysicsSpace; it fills in forces
## before the step and copies the state back after.  Ships point along
## their +X axis.
class_name Ship
extends Node2D

signal died(ship: Ship, killer: Node)
signal damaged(ship: Ship, amount: float)
signal rank_changed(ship: Ship, level: int)
signal system_failed(ship: Ship, system: Dictionary, attacker: Node)
signal touched_down(ship: Ship, planet: Planet)
signal lifted_off(ship: Ship, planet: Planet)

const WARP_CHARGE_TIME := 1.1
## After any warp ends, autopilot and computer pilots wait this long before
## starting another, so a ship never flickers in and out of warp.
const WARP_COOLDOWN := 4.0
## Engine thrust per point of engine power, in mass units * px/s^2.
const THRUST_PER_ENGINE := 4260.0
## Fuel units carried per point of engine power (400 s of full burn).
const FUEL_PER_ENGINE := 6.0
## Fuel burned per second at full thrust per point of engine power.
const BURN_PER_ENGINE := 0.015
## Mass of one fuel unit.
const FUEL_MASS := 0.5
## A booster burns its extra push this many times less efficiently than
## the main engines (on top of the push itself, see GameData.boost_gain).
const BOOST_BURN := 1.8
## Share of the tank per second the reactor cracks from nothing while the
## engines are idle, so nobody is stranded for good (about 33 min empty to full).
const FUEL_TRICKLE := 0.0005
## Fuel per second for the reaction control thrusters at full turn.
const RCS_BURN := 0.25
## How long the RCS takes to reach the design turn rate, in seconds, before size.
const SPIN_UP_TIME := 0.12
## Lowest point of a pass over a planet that the orbit guard will allow,
## above the surface plus the hull.
const ORBIT_MARGIN := 40.0
## Share of the crew a hull's worth of damage kills (beams burn through
## plating, not people: they pass a tenth of this, as in the original).
const CREW_LOSS := 0.7

## Landing: how slow (against the body) and how low a ship must be to start
## down, how long the descent and the climb out take, and how small the
## ship looks once it sits on the ground.
const LAND_SPEED := 170.0
const LAND_ALTITUDE := 360.0
const DESCENT_TIME := 2.6
const ASCENT_TIME := 2.0
const LANDED_SCALE := 0.12
enum Landing { NONE, DESCENDING, LANDED, ASCENDING }

var world: Node = null
var design_name := "Scout"
var design: Dictionary = {}
var race_id := 1
var pilot_name := "Pilot"
var ship_name := "Ship"
var is_player := false
var is_station := false

# flight
var body: PhysicsBody = null
var velocity := Vector2.ZERO
var angular_velocity := 0.0        # rad/s, copied from the body after each step
var size := 22.0
var max_speed := 150.0
var accel := 120.0                 # full thrust over wet mass, px/s^2
var turn_speed := 180.0            # design turn rate, deg/s
var mass := 1.0                    # dry mass: modules and weapons
var engine_power := 0.0            # sum of the engine modules' power
var thrust_force := 0.0            # engine_power * THRUST_PER_ENGINE
var fuel := 0.0
var fuel_max := 0.0
var burn_rate := 0.0               # fuel/s at full thrust
var fuel_use := 0.0                # fuel/s burned last physics step, for the HUD
var thrust_level := 0.0            # for the engine glow, 0..1
var flight_assist := true          # RCS holds the turn rate and stops a tumble
var orbit_guard := false           # the engines are busy steering clear of a planet
var refuelling := false            # near a friendly base or colony this frame

# systems
var energy := 1000.0
var energy_max := 1000.0
var energy_regen := 200.0
## The shield is two halves, front and rear (see GameData.SHIELD_FOCUS);
## `shield` is their sum, and setting it spreads the charge by the focus.
var shield: float:
	get:
		return shield_front + shield_rear
	set(value):
		set_shield_split(value, shield_share)
var shield_front := 0.0
var shield_rear := 0.0
var shield_max := 0.0
var shield_regen := 0.0            # charge a second with both halves recharging
var shield_share := 0.5            # the front half's share of the shield
var shield_share_rate := 0.0       # how fast the share moves to the focus
var shield_focus := 0              # GameData.SHIELD_FOCUS index
var shield_pause := [0.0, 0.0]     # front, rear: seconds before the half recharges
var shields_raised := true         # lowered shields draw nothing and give nothing off
var shield_boot := 0.0             # seconds left raising them
var shield_family := "deflector"   # GameData.SHIELD_FAMILIES
var hull := 100.0
var hull_max := 100.0
var armor := 0.0                   # 1 with an armor coat on
var coat := ""                     # the armor coat's module id (GameData.ARMOR)
var coat_kind := ""                # its GameData.ARMOR "coat"
var ablative := 0.0                # what is left of an ablative layer
var ablative_max := 0.0
var reactive_tiles := 0            # reactive tiles still armed
var reactive_max := 0
var reactive_rearm := 0.0          # seconds towards re-arming the next tile
var sensor_range := 1200.0          # the longest lock range against a signature of 100 (see Sensors)
var sensors := Sensors.new()       # signatures, contacts, ping, silent running, the jammer
var can_jam := false
var jamming := false               # the jammer is on (it works on the target; see Sensors._jam_tick)
var boosting := false              # this frame, for the heat signature
var engine_level := 0.0            # share of full thrust this frame, for the heat signature
var has_boost := false
var boost_speed := 1.5               # top speed and push while boosting (GameData.boost_gain)
var boost_accel := 2.2
var has_dock := false
var qdrv := 0.0
var warping := false
var warp_block := 0.0              # seconds before autopilot or the AI may warp again after a drop-out
var warp_charge := 0.0
var warp_pace := 0.0               # a wingman jumping with its leader flies at the leader's warp speed (0: its own)
var brake_power := 0.6
var module_levels := {}            # func -> level (for pickups)
var regen_rate := 0.0              # hull repaired per second (Arcean regenerators)
var repair_rate := 0.0             # hull repaired per second for energy (Terran repair bays)
var ram_power := 0.0               # Drengin ram prow: collision damage multiplier
var power_gen := 0.0
var power_draw := 0.0
var overload := 1.0                # >1 when the reactor cannot feed everything
var stats: Dictionary = {}

# the pilot's purse and salvage, used at the shipyard
var credits := 0
var hold: Array = []               # module ids salvaged from wrecks
var hold_stock: Dictionary = {}    # id -> how many of those in the hold came off a stock fit (they sell for scrap)
var blueprints: Array = []         # enemy parts this pilot may buy
const HOLD_SIZE := 16
# this round's record, for the scoreboard
var captures := 0
var credits_earned := 0

var weapons: Array = []            # see _build_weapons
var pdc := PointDefence.new()      # point defence turrets (internal PDC parts)
var inputs := {
	"thrust": 0.0, "turn": 0.0, "strafe": 0.0, "boost": false, "brake": false,
	"fire_primary": false, "fire_secondary": false, "fire_missile": false,
	"fire_mine": false, "fire_beam": false,
}
var target: Node2D = null
var nav_target: Node2D = null
var autopilot := false

# career
var score := 0
var kills := 0
var deaths := 0
var rank := 0
var dead := false
var invulnerable := 0.0
var hit_flash := 0.0
var last_attacker: Node = null
var last_attacked_at := -100.0
var engine_sound: EngineSound = null
var beam_targets := []             # beams fired this frame, for drawing
var aim_point := Vector2.INF       # where the pilot wants turrets to point (world)
var has_turret := false
var last_hit_angle := 0.0
var shield_bubble: ShieldBubble
var net_id := 0                    # online: the id the host and clients share
var is_puppet := false             # online: shown from the host's snapshots, not simulated
var docked := false                # in the shipyard: controls are off

# landing and colonists
var landing := Landing.NONE
var land_body: Planet = null       # the world we are landing on, sitting on or leaving
var land_t := 0.0                  # 0..1 through the descent or the climb
var land_from := Vector2.ZERO      # offset from the body's centre where the move began
var land_spot := Vector2.ZERO      # offset from the body's centre of the landing site
var land_rot := 0.0
var land_scale := 1.0              # drawn size: shrinks as the ship flies down into the world
var colonists := 0.0               # people aboard
var cabin := 0.0                   # people the hull can carry
var transfer := 0                  # while landed: +1 boarding colonists, -1 putting them down
var colonist_origin: Planet = null # where the colonists aboard came from (no pay for taking them home)
var cargo := {}                    # resource -> units aboard (the Resources update)
var cargo_max := 0.0               # units the hold carries
var cargo_origin: Node2D = null    # the world or station the cargo was last loaded at (no pay for taking it back)
var cargo_orders := {}             # on the ground: resource -> units still to move (+ load, - unload)
var support := false               # a computer hauler: works for the empire, not on the scoreboard
var troops := {}                   # ground units aboard: unit -> how many (the Resources update, phase 5)
var troop_max := 0                 # units the troop bays hold
var convoy := false                # a supply or trade ship on a valuable run, guarded (phase 6)
var guarding: Ship = null          # the convoy this warship escorts
const CARGO_MASS := 0.4            # mass of a unit of cargo
const CARGO_RATE := 30.0           # units a second through a spaceport
const KIT_MASS := 100.0            # a station kit, crated
var kit := ""                      # a station kit aboard (GameData.STATIONS key): it fills the hold
# stations built from kits (the Resources update, phase 3)
var building := false              # a frame: waiting for its materials, then the work
var build_t := 0.0                 # seconds of work done on the frame
var station_body: Planet = null    # the world a built station keeps station over
var founder := ""                  # the pilot who deployed this station (a trade station pays them its fees)
var orders: Array = []             # a yard's build queue: [hull, seconds left, buyer's pilot name]
var art_model: Dictionary = {}     # the sprite model, rebuilt on refit
var hull_race := -1                # whose shipyard built the hull (the painting); -1: race_id

# systems that can be shot out, boarding and prizes (see ShipSystems, World.try_board)
var systems := ShipSystems.new()
var boarding: Ship = null          # the disabled ship we are boarding
var board_progress := 0.0          # 0..1
var board_lost := 0.0              # seconds the boarding conditions have been broken
var boarded_by: Ship = null        # who is boarding us
var claimed_by: Ship = null        # a computer pilot closing in to board us
var board_start_crew := 1.0        # the defenders when the boarding began
var crew := 6.0                    # hands aboard; hits kill them, bases and colonies make up the numbers
var crew_max := 6
var captured := false              # set while the ship is taken: it leaves without a blast
var is_prize := false              # a captured ship flown by a prize crew; never respawns
var prize_of: Ship = null          # the pilot whose prize this is
var ships_captured := 0            # this round's boarding captures, for the scoreboard


## Human pilots (local or seated online) fire by weapon group; computer
## pilots fire by weapon kind.
const GROUP_INPUTS := ["fire_primary", "fire_secondary", "fire_missile", "fire_mine"]
var group_fire := false
var weapon_group := 1               # the group Fire shoots (the player picks it with 1-4)


## The weapon names in a group, with a count for repeats ("Laser I x2").
func group_weapons(g: int) -> Array:
	var counts := {}
	var order := []
	for w in weapons:
		if int(w["group"]) != g:
			continue
		var label: String = GameData.missile_display_name(race_id, w["missile"]) if w["kind"] == "tube" else w["name"]
		if not counts.has(label):
			order.append(label)
			counts[label] = 0
		counts[label] += 1
	var out := []
	for label in order:
		out.append(label if counts[label] == 1 else "%s x%d" % [label, counts[label]])
	return out


func setup(p_world: Node, p_design: String, p_race: int, p_pilot: String, p_ship_name: String) -> void:
	world = p_world
	race_id = p_race
	if hull_race < 0:
		hull_race = p_race
	pilot_name = p_pilot
	group_fire = is_player
	ship_name = p_ship_name
	body = PhysicsBody.new()
	body.kind = PhysicsBody.Kind.SHIP
	body.node = self
	body.restitution = 0.25
	body.friction = 0.35
	refit(p_design)
	shield_bubble = ShieldBubble.new()
	add_child(shield_bubble)
	shield_bubble.setup(self)
	engine_sound = EngineSound.new()
	add_child(engine_sound)
	engine_sound.setup(self)


## Rebuild the ship from a design (a stock hull name or a design
## dictionary from the shipyard), keeping career and name.
func refit(p_design: Variant) -> void:
	if p_design is String:
		# computer pilots fly their empire's armor coat (GameData.RACE_COAT)
		design = GameData.stock_design(p_design, race_id) if is_player or support else GameData.race_fit(p_design, race_id)
	else:
		design = GameData.fix_design(p_design)
	design_name = design["name"]
	is_station = GameData.is_station_hull(design_name)
	size = design["size"]
	has_dock = design.has("dock")
	stats = GameData.stats_for(design)
	mass = stats["mass"]
	max_speed = stats["max_speed"]
	turn_speed = stats["turn"]
	accel = stats["accel"]
	energy_max = stats["energy_max"]
	energy_regen = stats["energy_regen"]
	shield_max = stats["shield_max"]
	shield_regen = stats["shield_regen"]
	hull_max = stats["hull_max"]
	armor = stats["armor"]
	coat = stats["coat"]
	coat_kind = str(GameData.ARMOR.get(coat, {}).get("coat", ""))
	shield_family = stats["shield_family"]
	sensor_range = stats["sensor_range"]
	can_jam = stats["can_jam"]
	if sensors.ship == null:
		sensors.setup(self)
	sensors.fit(stats)
	has_boost = stats["has_boost"]
	boost_speed = stats["boost_speed"]
	boost_accel = stats["boost_accel"]
	qdrv = stats["qdrv"]
	brake_power = stats["brake_power"]
	regen_rate = stats["regen"]
	repair_rate = stats["repair"]
	ram_power = stats["ram"]
	power_gen = stats["power_gen"]
	power_draw = stats["power_draw"]
	overload = stats["overload"]
	module_levels = stats["module_levels"].duplicate()
	# engines against mass: a Scout gets about 170 px/s^2, a Dreadnought 130
	engine_power = stats["eng"]
	thrust_force = engine_power * THRUST_PER_ENGINE
	fuel_max = engine_power * FUEL_PER_ENGINE
	burn_rate = engine_power * BURN_PER_ENGINE
	if is_station:
		# station-keeping thrusters only; the mass does the rest
		thrust_force = mass * 8.0
		burn_rate = engine_power * BURN_PER_ENGINE * 0.1
	cabin = float(GameData.COLONIST_CABIN.get(design_name, 60)) * 1e6
	colonists = minf(colonists, cabin)
	# a frame's hold is the materials it is waiting for
	cargo_max = GameData.station_cost_total(design_name) if building else float(GameData.CARGO_HOLD.get(design_name, 20))
	if kit != "" and cargo_max < GameData.CARGO_HOLD["Freighter"]:
		kit = ""                       # too small a hold for a station kit: it stays at the yard
	_trim_cargo()
	troop_max = int(GameData.TROOP_BAYS.get(design_name, 0))
	_trim_troops()
	fuel = fuel_max
	crew_max = GameData.HULL_CREW.get(design_name, 10)
	crew = crew_max
	accel = thrust_force / wet_mass()
	energy = energy_max
	hull = hull_max
	ablative_max = hull_max * GameData.ABLATIVE_LAYER if coat_kind == "ablative" else 0.0
	ablative = ablative_max
	reactive_max = GameData.reactive_tiles(size) if coat_kind == "reactive" else 0
	reactive_tiles = reactive_max
	shield_share = GameData.SHIELD_FOCUS[shield_focus]
	shield_pause = [0.0, 0.0]
	shield_boot = 0.0
	shield = shield_max
	_build_weapons()
	art_model = ShipArt.build_model(design, hull_race, size)
	pdc.build(int(stats.get("pdc", 0)), ShipArt.pdc_spots(art_model), is_station)
	systems.build(self)
	apply_systems()
	_build_body()
	if shield_bubble != null:
		shield_bubble.fit()
	queue_redraw()


## Set the ship's working numbers from its systems' health: damaged
## engines push, brake and turn less (none at all when they are out), a
## hurt reactor recharges slower, broken shield
## generators hold less, blind sensors see less.  Called whenever a
## system's efficiency moves.
func apply_systems() -> void:
	var b := systems.base
	if b.is_empty():
		return
	systems.changed = false
	var eng := systems.efficiency("engines")
	thrust_force = b["thrust_force"] * eng
	turn_speed = b.get("turn_speed", turn_speed) * eng
	max_speed = b["max_speed"] * (0.6 + 0.4 * eng if eng > 0.0 else 1.0)
	has_boost = b["has_boost"] and eng >= 1.0
	energy_regen = b["energy_regen"] * systems.efficiency("reactor") * _lowered_shield_bonus()
	var sh := systems.efficiency("shields")
	shield_max = b["shield_max"] * sh
	shield_regen = b["shield_regen"] * sh
	_fit_shield_halves()
	var sn := systems.efficiency("sensors")
	sensors.efficiency = lerpf(ShipSystems.BLIND_SENSORS, 1.0, sn)
	sensor_range = b["sensor_range"] * sensors.efficiency
	can_jam = b["can_jam"] and not systems.is_offline("jammer")
	if jamming and not can_jam:
		set_jam(false)
	qdrv = b["qdrv"] if not systems.is_offline("qdrv") else 0.0
	if warping and (qdrv <= 0.0 or engines_out()):
		end_warp()


## The empire's home starbase (the stations built from kits are not).
func is_starbase() -> bool:
	return design_name == "Starbase"


## Dead in space: engines or reactor shot out, or nobody left aboard.
func is_disabled() -> bool:
	return systems.disabled() or (crew < 0.5 and not is_station)


## A station whose shields are down and whose hull is broken open: a
## boarding party can get in (see Capture).  Ships are boarded when
## disabled instead.  One shield half down is enough: the attacker's guns
## only ever bring down the half facing them (Kyle, 2026-10-29).
func is_breached() -> bool:
	if not is_station or is_starbase() or hull > hull_max * Capture.STATION_BREACH_HULL:
		return false
	return shield_front <= shield_cap(true) * Capture.STATION_BREACH_SHIELD \
		or shield_rear <= shield_cap(false) * Capture.STATION_BREACH_SHIELD


## Open to a boarding party: a ship dead in space, or a breached station.
func boardable() -> bool:
	return is_breached() if is_station else is_disabled()


## Whose side the ship is on: its empire's, except in a free-for-all
## duel, where every pilot fights alone.  Compare sides with this, not
## race_id, when deciding friend or foe.
func team() -> int:
	if world != null and world.get("duel_rules") != null and not is_station:
		return get_instance_id()
	if world != null and world.get("mode_rules") != null:
		return world.mode_rules.team_of(self)
	return race_id


## Is a pilot of side `race` (see team()) boarding us, or on the way to?
## Its friends hold their fire so it can take us alive.
func spoken_for(race: int) -> bool:
	for c in [boarded_by, claimed_by]:
		if c != null and is_instance_valid(c) and not c.dead and c.team() == race and (c.boarding == self or c.target == self):
			return true
	return false


func crew_fraction() -> float:
	return crew / maxf(crew_max, 1.0)


func engines_out() -> bool:
	return systems.is_offline("engines")


## Dry mass plus the fuel on board.
func wet_mass() -> float:
	return mass + fuel * FUEL_MASS + cargo_total() * CARGO_MASS + (KIT_MASS if kit != "" else 0.0)


func cargo_total() -> float:
	var n := 0.0
	for k in cargo:
		n += cargo[k]
	return n


func cargo_space() -> float:
	if kit != "":
		return 0.0
	return maxf(cargo_max - cargo_total(), 0.0)


func troop_count() -> int:
	return Ground.count(troops)


func troop_room() -> int:
	return maxi(troop_max - troop_count(), 0)


## Smaller troop bays after a refit: the extra units stay behind (lost).
func _trim_troops() -> void:
	while troop_count() > troop_max:
		for u in ["armour", "commandos", "infantry"]:
			if troops.get(u, 0.0) >= 0.5:
				troops[u] -= 1.0
				if troops[u] < 0.5:
					troops.erase(u)
				break


## A smaller hold after a refit: what no longer fits is lost, the most
## plentiful first.
func _trim_cargo() -> void:
	var over := cargo_total() - cargo_max
	while over > 0.01:
		var top := ""
		for k in cargo:
			if top == "" or cargo[k] > cargo[top]:
				top = k
		var n := minf(cargo[top], over)
		cargo[top] -= n
		over -= n
		if cargo[top] <= 0.01:
			cargo.erase(top)


func _build_body() -> void:
	if body == null:
		return
	body.set_hull(CollisionHulls.for_ship(design, hull_race, size))
	body.set_mass(wet_mass())
	# a starbase holds its spot beside its world: solid, unmovable
	body.set_kinematic(is_station)
	body.pos = global_position
	body.vel = velocity
	body.angle = rotation
	body.ang_vel = 0.0


func _build_weapons() -> void:
	weapons.clear()
	has_turret = false
	# shots leave from the turret as drawn on the painted hull
	var spots := ShipArt.weapon_spots(design["name"], hull_race, size, design["weapons"])
	for w in design["weapons"]:
		var m: Dictionary = GameData.MODULES[w[0]]
		var spot: Vector2 = spots[weapons.size()]
		var entry := {
			"id": w[0], "name": m["name"], "kind": m["func"],
			"mount_angle": spot.angle(), "mount_radius": spot.length(),
			"fire_angle": deg_to_rad(float(w[3])),
			"arc0": deg_to_rad(float(w[4])), "arc1": deg_to_rad(float(w[5])),
			"cooldown": 0.0, "cost": 0.0, "ammo": -1, "missile": "", "slot": "primary",
			"range": 0.0, "group": GameData.weapon_group_of(w), "index": weapons.size(),
		}
		if m["func"] == "tube":
			var mid: String = m["p"][3]
			entry["missile"] = mid
			entry["cost"] = clampf(float(m["p"][1]) * 0.025, 20.0, 350.0) * (1.0 + (overload - 1.0) * 0.5)
			entry["ammo"] = int(m["p"][2]) if int(m["p"][2]) < 400 else -1
			if is_station:
				entry["ammo"] = -1     # a station's magazines are deep
			entry["ammo_max"] = entry["ammo"]
			entry["rate"] = GameData.TUBE_COOLDOWN.get(mid, 0.3)
			var md: Dictionary = GameData.MISSILES[mid]
			entry["range"] = md["fire_v"] * GameData.LIGHT_SPEED * md["life"] * 0.8
			match mid:
				"001", "010":
					entry["slot"] = "primary"
				"002":
					entry["slot"] = "mine"
				"008", "009", "021":
					entry["slot"] = "missile"
				_:
					entry["slot"] = "secondary"
		elif m["func"] == "beam":
			entry["slot"] = "beam"
			entry["cost"] = 90.0 * (1.0 + (overload - 1.0) * 0.5)          # per second
			entry["range"] = GameData.beam_range(w[0])
			entry["dps"] = GameData.beam_dps(w[0])
			entry["rate"] = 0.0
		weapons.append(entry)
		if is_turret(entry) or is_free_aim(entry):
			has_turret = true


func race_color() -> Color:
	return GameData.race(race_id)["color"]


func forward() -> Vector2:
	return Vector2.RIGHT.rotated(rotation)


func speed() -> float:
	return velocity.length()


## Velocity of the local rest frame: the planet whose gravity we sit in,
## or the sun.  Brakes, the speed limit and the HUD work against it, so
## "stopped" means parked beside the planet, not against the stars.
func frame_velocity() -> Vector2:
	if world == null or world.space == null:
		return Vector2.ZERO
	var b: PhysicsBody = world.space.dominant_body(global_position)
	return b.vel if b != null else Vector2.ZERO


func local_velocity() -> Vector2:
	return velocity - frame_velocity()


func fuel_fraction() -> float:
	return fuel / maxf(fuel_max, 1.0)


func has_fuel() -> bool:
	return fuel > 0.0


# ------------------------------------------------------------ sensors

## Does the viewer hold a track (a lock) on this ship, with its own
## sensors or its allies' data link?  Down on a surface nobody sees us.
func visible_to(viewer: Ship) -> bool:
	return Sensors.quality(viewer, self) >= 3


## What this ship knows of a target: 0 nothing, 1 a bearing, 2 a contact
## (no lock), 3 a track.
func contact_quality(t: Node) -> int:
	if not t is Ship:
		return 3 if t != null else 0
	return Sensors.quality(self, t)


## A lock on the target with any sensor (channel ""), or with our own
## sensors on one channel: "radar" or "heat" (a seeker needs heat).
func has_track(t: Node, channel: String = "") -> bool:
	if not t is Ship:
		return t != null
	return Sensors.track(self, t, channel)


## How loud the ship is on a channel: "radar", "heat", "em" or "temporal";
## 100 is seen at a sensor's rated range.
func signature(channel: String) -> float:
	return sensors.signature(channel)


## A spike on a channel that dies away over `seconds` (a shot's flash, a
## reactive tile firing, a ping).
func add_signature(channel: String, amount: float, seconds: float) -> void:
	sensors.add_spike(channel, amount, seconds)


## The jammer: on the pilot's target, if it is locked and in reach.
func set_jam(on: bool) -> void:
	if on == jamming:
		return
	if on:
		var why := sensors.jam_refusal()
		if why == "" and not (target is Ship and is_instance_valid(target) and target.team() != team() and has_track(target)):
			why = "Lock a target first: the jammer works on the ship you have targeted."
		if why != "":
			if is_player and world != null:
				world.notify(self, why)
			return
	jamming = on
	Sfx.play("cloak" if on else "uncloak", global_position, 0.6)


## A disruptor hit knocks the jammer out for a while.
func jam_off(seconds: float) -> void:
	sensors.jam_out = maxf(sensors.jam_out, seconds)
	if jamming:
		set_jam(false)


func ping() -> void:
	var said := sensors.ping()
	if is_player and world != null:
		world.notify(self, said)


## Silent running: radar and lidar off, shields stop recharging, the
## reactor turned down (half the energy regen, no reactor heat).
## Point defence on or off (the PDC key): its stream of fire shows up on
## heat sensors, so a pilot sneaking up may want it quiet.
func set_pdc(on: bool) -> void:
	if not pdc.fitted():
		if is_player and world != null:
			world.notify(self, "No PDC fitted: buy one at the shipyard (an internal part).")
		return
	pdc.enabled = on
	if is_player and world != null:
		world.notify(self, "Point defence on: the PDCs shoot down incoming missiles, torpedoes, drones and mines." if on else "Point defence off: the PDCs hold their fire.")


func set_silent(on: bool) -> void:
	sensors.set_emcon(on)
	if is_player and world != null:
		world.notify(self, "Silent running: radar off, shields hold their charge, reactor at half." if on else "Silent running off: radar back in a second.")


# ------------------------------------------------------------ simulation

func _physics_process(delta: float) -> void:
	if dead:
		return
	if is_puppet:
		_puppet_step(delta)
		return
	if invulnerable > 0.0:
		invulnerable -= delta
	if hit_flash > 0.0:
		hit_flash -= delta
	if Net.is_client() and landing != Landing.NONE:
		_landing_visual(delta)        # the host moves us; we only shrink and grow
	_systems(delta)
	_weapons(delta)
	queue_redraw()


## Called by the world before the physics step: copy the node's state into
## the body (other code may have moved the ship) and apply this frame's
## engine and RCS forces.
func pre_step(delta: float) -> void:
	body.enabled = not dead and landing == Landing.NONE
	if dead:
		return
	if landing != Landing.NONE:
		_landing_step(delta)
		return
	body.pos = global_position
	body.vel = velocity
	body.angle = rotation
	body.set_mass(wet_mass())
	body.phasing = warping
	body.gravity_scale = 0.0 if warping else 1.0
	_fly(delta)


## Called by the world after the step: take the simulated state back.
func post_step(_delta: float) -> void:
	if dead or landing != Landing.NONE:
		return
	global_position = body.pos
	velocity = body.vel
	rotation = body.angle
	angular_velocity = body.ang_vel


func _fly(delta: float) -> void:
	var fwd := forward()
	fuel_use = 0.0
	boosting = inputs["boost"] and has_boost and inputs["thrust"] > 0.0 and fuel > 0.0
	var vmax := max_speed * (boost_speed if boosting else 1.0)
	# the rating is the speed the drive holds in open space; inside a
	# gravity well the cap is on the energy, so a ship can always climb out:
	# locally that means the escape speed on top of the rating
	var v_esc := escape_speed()
	vmax = sqrt(vmax * vmax + v_esc * v_esc)
	var f0 := thrust_force * (boost_accel if boosting else 1.0)
	accel = thrust_force / wet_mass()
	warp_block = maxf(warp_block - delta, 0.0)

	if warping:
		_warp(delta)
		body.vel = velocity
		body.angle = rotation
		body.ang_vel = 0.0
		return

	if autopilot and nav_target != null and is_instance_valid(nav_target):
		_autopilot(delta)

	# --- rotation: reaction control thrusters with real angular inertia
	var turn: float = clampf(inputs["turn"], -1.0, 1.0)
	var w_max := deg_to_rad(turn_speed)
	var spin_up := SPIN_UP_TIME + size / 200.0
	var torque_max := body.inertia * w_max / spin_up
	var rcs_level := 0.0
	# engines shot out: no thrust, no brakes, no thrusters to turn or to
	# stop a spin.  The ship drifts and tumbles as it was going when they
	# failed, and gravity and hits still move it.
	var adrift := engines_out()
	var dead_drive := fuel <= 0.0 or adrift
	if dead_drive:
		torque_max *= 0.3          # reaction wheels on reactor power only
	if (flight_assist or turn != 0.0) and not adrift:
		var w_target := turn * w_max
		var torque := clampf(body.inertia * (w_target - body.ang_vel) / SPIN_UP_TIME, -torque_max, torque_max)
		body.apply_torque(torque)
		rcs_level = absf(torque) / maxf(torque_max, 1.0)

	# --- translation: the drive pushes less the closer it is to its rated
	# speed, measured against the local planet so orbits are free
	var f := Vector2.ZERO
	var t: float = clampf(inputs["thrust"], -1.0, 1.0)
	var lv := local_velocity()
	var along := lv.dot(fwd)
	if t > 0.0:
		f += fwd * f0 * t * _drive_curve(along, vmax)
	elif t < 0.0:
		f += fwd * f0 * 0.6 * t * _drive_curve(-along, vmax * 0.6)
	var st: float = clampf(inputs["strafe"], -1.0, 1.0)
	if st != 0.0:
		var side := fwd.orthogonal()
		f += side * f0 * 0.7 * st * _drive_curve(lv.dot(side) * signf(st), vmax * 0.7)
	var sp := lv.length()
	if inputs["brake"] and sp > 1.0:
		# retro burn, never past a stop
		var brake_a := minf(maxf(accel * brake_power * 1.5, 170.0 * brake_power * systems.efficiency("engines")), sp / delta)
		f -= lv.normalized() * brake_a * body.mass
	engine_level = f.length() / maxf(thrust_force, 1.0)
	# the orbit guard: with assist on, a course that would end on a planet's
	# surface is flown into a circular orbit on the ship's own engines
	var hands_off: bool = t == 0.0 and st == 0.0 and not inputs["brake"]
	var guard := Vector2.ZERO
	if flight_assist and (hands_off or not is_player):
		guard = _orbit_guard_force(fwd, f0, delta)
	else:
		orbit_guard = false     # the pilot has the stick: never leave the guard holding the engines
	if orbit_guard:
		f = guard
		engine_level = f.length() / maxf(thrust_force, 1.0)
	if dead_drive:
		f = Vector2.ZERO
		engine_level = 0.0
	# over the rated speed the drive field drags the ship back toward it
	if sp > vmax and not inputs["brake"] and not orbit_guard and not dead_drive:
		var over := sp - vmax
		var pull_a := minf(over / delta, accel * 1.5 + 60.0)
		f -= lv.normalized() * pull_a * body.mass
	body.apply_force(f)
	# --- fuel: engines and boosters run on propellant, never on the
	# reactor; engines burn by thrust (a booster's extra push is in
	# engine_level), the booster wastes more on top, RCS sips
	var burn := engine_level * burn_rate * (BOOST_BURN if boosting else 1.0) + rcs_level * RCS_BURN
	fuel_use = burn
	if burn > 0.0:
		if fuel < burn * delta and world != null and is_player and fuel > 0.0 and not Net.is_client():   # a client replays inputs; the host says it
			world.notify(self, "Fuel exhausted. Drift to a friendly base or colony to refuel.")
		fuel = maxf(fuel - burn * delta, 0.0)
	thrust_level = lerpf(thrust_level, clampf(minf(engine_level * 1.2, 1.0) + (0.5 if boosting else 0.0), 0.0, 1.0), 10.0 * delta)
	if engine_sound != null:
		engine_sound.update(thrust_level, boosting, warping, delta)


## The pull of a well as the space really applies it at distance d: faded
## at the edge and capped, as GM.
static func well_gm(p: PhysicsBody, d: float) -> float:
	var reach := PhysicsSpace.reach_of(p)
	var fade := clampf((reach - d) / (reach * PhysicsSpace.FADE), 0.0, 1.0)
	return minf(p.gm * fade, PhysicsSpace.MAX_GRAVITY * d * d)


## The speed needed to leave the well we sit in from here; zero in open space.
func escape_speed() -> float:
	if world == null or world.space == null:
		return 0.0
	var p: PhysicsBody = world.space.dominant_body(global_position)
	if p == null:
		return 0.0
	var d := global_position.distance_to(p.pos)
	if d < 1.0:
		return 0.0
	return sqrt(2.0 * well_gm(p, d) / d)


## The engine force that keeps this ship off the planet whose well it sits
## in: when the periapsis of its present course is below the surface (plus
## margin), thrust toward the circular-orbit velocity here, prograde in the
## sense it is already going round.  Thrust is what the engines give in
## each direction (full ahead, 0.6 astern, 0.7 abeam), so a heavy, weakly
## engined hull may still not make it.  Sets orbit_guard.
func _orbit_guard_force(fwd: Vector2, f0: float, delta: float) -> Vector2:
	var was := orbit_guard
	orbit_guard = false
	if world == null or world.space == null or fuel <= 0.0 or is_station or docked or engines_out():
		return Vector2.ZERO
	var p: PhysicsBody = world.space.dominant_body(global_position)
	if p == null or p.is_sun or p.gm <= 0.0:
		return Vector2.ZERO
	var r := global_position - p.pos
	var d := r.length()
	if d < 1.0:
		return Vector2.ZERO
	var gm := well_gm(p, d)
	if gm < 1.0:
		return Vector2.ZERO
	var v := velocity - p.vel
	if r.dot(v) > 0.0:
		return Vector2.ZERO             # climbing: never hold a ship that is leaving
	var h := r.cross(v)
	var eps := v.length_squared() * 0.5 - gm / d
	var e := sqrt(maxf(1.0 + 2.0 * eps * h * h / (gm * gm), 0.0))
	var periapsis := h * h / (gm * (1.0 + e))
	var safe := (p.radius + size + ORBIT_MARGIN) * (1.15 if was else 1.0)
	if periapsis > safe:
		return Vector2.ZERO
	var side := signf(h) if absf(h) > 1.0 else signf(r.cross(fwd))
	if side == 0.0:
		side = 1.0
	var want := Vector2(-r.y, r.x) / d * side * sqrt(gm / d)
	var dv := want - v
	if dv.length() < 1.0:
		return Vector2.ZERO
	orbit_guard = true
	var dir := dv.normalized()
	var along := dir.dot(fwd)
	var across := dir.dot(fwd.orthogonal())
	var cap := f0 * (1.0 if along > 0.0 else 0.6) * absf(along) + f0 * 0.7 * absf(across)
	var need := dv.length() * body.mass / maxf(delta, 0.001)
	return dir * minf(cap, need)


## A puppet only keeps its looks fresh; the host moves it (see Net).
func _puppet_step(delta: float) -> void:
	if invulnerable > 0.0:
		invulnerable -= delta
	if hit_flash > 0.0:
		hit_flash -= delta
	if landing != Landing.NONE:
		_landing_visual(delta)
	if engine_sound != null:
		engine_sound.update(thrust_level, false, warping, delta)
	pdc.tick(self, delta, false)      # the turrets track, the host shoots
	queue_redraw()


## Thrust available at a speed along the push direction: full when moving
## against it, fading to nothing at the rated speed.
static func _drive_curve(v_along: float, vmax: float) -> float:
	if v_along <= 0.0:
		return 1.0
	var x := v_along / maxf(vmax, 1.0)
	return clampf(1.0 - x * x * x * x, 0.0, 1.0)


## Pushed by a hit: an impulse at a world point, which also spins the hull.
func apply_hit_impulse(j: Vector2, at: Vector2) -> void:
	if dead or body == null:
		return
	body.apply_impulse(j, at)
	velocity = body.vel
	angular_velocity = body.ang_vel


## Take fuel aboard, up to the tank.
func refuel(amount: float) -> void:
	fuel = minf(fuel + amount, fuel_max)


# -------------------------------------------------------------- landing

## Why this ship cannot start down onto a body now, or "" when it can.
func landing_refusal(p: Planet) -> String:
	if p == null:
		return "No world below to land on."
	if is_station:
		return "Stations never land."
	if not p.has_surface():
		return ("You cannot land in the Sun." if p.look == "Sun" else "%s is a gas giant: there is no surface to land on." % p.body_name)
	if warping:
		return "Drop out of warp to land."
	if engines_out():
		return "Engines out: we can't fly down until damage control brings them back."
	var alt := global_position.distance_to(p.global_position) - p.radius
	if alt > LAND_ALTITUDE + size:
		return "Too high to land on %s: fly closer." % p.body_name
	if (velocity - p.velocity).length() > LAND_SPEED:
		return "Too fast to land on %s: brake first (X)." % p.body_name
	if p.is_colonised() and p.owner_race != race_id and p.pshield_up():
		return "%s's planetary shield is up: nothing lands until it is shot down." % p.body_name
	return ""


func is_landed() -> bool:
	return landing == Landing.LANDED


func is_landing() -> bool:
	return landing != Landing.NONE


## Fly down into a world.  The ship rides the body's orbit from here on,
## shrinking as it drops toward the landing site below it.
func start_landing(p: Planet) -> void:
	landing = Landing.DESCENDING
	land_body = p
	land_t = 0.0
	land_from = global_position - p.global_position
	land_spot = land_from.normalized() * p.radius * 0.72
	land_rot = rotation
	autopilot = false
	transfer = 0
	set_jam(false)
	body.enabled = false
	Sfx.play("dock", global_position, 0.6, 0.85)


## Climb back out to a parking orbit above the landing site.
func start_takeoff() -> void:
	if landing != Landing.LANDED:
		return
	var p := land_body
	land_t = 0.0
	landing = Landing.ASCENDING
	land_from = land_spot
	land_spot = land_spot.normalized() * (p.radius + size * 2.0 + 110.0)
	land_rot = rotation
	transfer = 0
	Sfx.play("warpexit", global_position, 0.5, 1.25)


static func _ease(x: float) -> float:
	return x * x * (3.0 - 2.0 * x)


func _landing_step(delta: float) -> void:
	var p := land_body
	if p == null or not is_instance_valid(p):
		_end_landing()
		return
	for k in inputs.keys():
		if k != "boost":
			inputs[k] = false if typeof(inputs[k]) == TYPE_BOOL else 0.0
	match landing:
		Landing.DESCENDING:
			land_t = minf(land_t + delta / DESCENT_TIME, 1.0)
			var e := _ease(land_t)
			global_position = p.global_position + land_from.lerp(land_spot, e)
			# nose down toward the site, engines throttling back
			rotation = lerp_angle(land_rot, (-land_spot).angle(), minf(land_t * 1.6, 1.0))
			thrust_level = lerpf(0.6, 0.0, land_t)
			if land_t >= 1.0:
				landing = Landing.LANDED
				touched_down.emit(self, p)
		Landing.LANDED:
			global_position = p.global_position + land_spot
			thrust_level = 0.0
		Landing.ASCENDING:
			land_t = minf(land_t + delta / ASCENT_TIME, 1.0)
			var e := _ease(land_t)
			global_position = p.global_position + land_from.lerp(land_spot, e)
			var out := land_spot.normalized()
			rotation = lerp_angle(land_rot, out.orthogonal().angle(), minf(land_t * 1.4, 1.0))
			thrust_level = lerpf(1.0, 0.5, land_t)
			if land_t >= 1.0:
				_end_landing()
				# into a parking orbit above the site, under our own power
				var v := p.velocity
				if world != null and world.space != null and p.body != null:
					v = world.space.circular_orbit_velocity(p.body, global_position)
				velocity = v
				rotation = (v - p.velocity).angle() if v != p.velocity else out.orthogonal().angle()
				body.pos = global_position
				body.vel = velocity
				body.angle = rotation
				body.ang_vel = 0.0
				lifted_off.emit(self, p)
				return
	velocity = p.velocity
	land_scale = _landing_scale()
	scale = Vector2.ONE * land_scale
	body.pos = global_position
	body.vel = velocity
	if engine_sound != null:
		engine_sound.update(thrust_level, false, false, delta)


## Size on screen through the move: full size up high, a speck on the ground.
func _landing_scale() -> float:
	match landing:
		Landing.DESCENDING:
			return lerpf(1.0, LANDED_SCALE, _ease(land_t))
		Landing.LANDED:
			return LANDED_SCALE
		Landing.ASCENDING:
			return lerpf(LANDED_SCALE, 1.0, _ease(land_t))
	return 1.0


## Online clients: the host flies the ship, this only shrinks and grows it.
func _landing_visual(delta: float) -> void:
	if landing == Landing.DESCENDING:
		land_t = minf(land_t + delta / DESCENT_TIME, 1.0)
	elif landing == Landing.ASCENDING:
		land_t = minf(land_t + delta / ASCENT_TIME, 1.0)
		if land_t >= 1.0:
			landing = Landing.NONE
	land_scale = _landing_scale()
	scale = Vector2.ONE * land_scale


## Back in space at once, in orbit above the site (a human takes the seat
## of a computer pilot that was down on a world).
func leave_ground_now() -> void:
	if landing == Landing.NONE:
		return
	var p := land_body
	var out := land_spot.normalized() if land_spot != Vector2.ZERO else Vector2.RIGHT
	_end_landing()
	if p == null or not is_instance_valid(p):
		return
	global_position = p.global_position + out * (p.radius + size * 2.0 + 110.0)
	velocity = world.space.circular_orbit_velocity(p.body, global_position) if world != null and p.body != null else p.velocity
	rotation = out.orthogonal().angle()
	body.pos = global_position
	body.vel = velocity
	body.angle = rotation
	body.ang_vel = 0.0


func _end_landing() -> void:
	landing = Landing.NONE
	land_t = 0.0
	land_scale = 1.0
	scale = Vector2.ONE
	transfer = 0
	cargo_orders.clear()
	body.enabled = not dead


## How hot the air is on the way down, 0..1, for the re-entry glow.
func reentry_heat() -> float:
	if land_body == null or landing == Landing.NONE or landing == Landing.LANDED:
		return 0.0
	var air: float = 0.0
	match land_body.look:
		"Venus", "Titan": air = 1.0
		"Earth": air = 0.85
		"Mars": air = 0.4
		_: air = 0.0
	var e := land_t if landing == Landing.DESCENDING else 1.0 - land_t
	return air * clampf(sin(e * PI) * 1.4, 0.0, 1.0)


func _warp(delta: float) -> void:
	var fwd := forward()
	if warp_charge < WARP_CHARGE_TIME:
		warp_charge += delta
		velocity = velocity.lerp(fwd * max_speed * 0.5, 2.0 * delta)
		thrust_level = lerpf(thrust_level, 1.0, 5.0 * delta)
		if warp_charge >= WARP_CHARGE_TIME:
			Sfx.play(Sfx.leveled("warpout", module_levels.get("qdrv", 1)), global_position, 0.8)
			_time_ripple()
		return
	var drain := 160.0 * delta
	if energy < drain or inputs["brake"]:
		end_warp()
		return
	energy -= drain
	var wspeed := warp_speed() if warp_pace <= 0.0 else warp_pace
	velocity = fwd * wspeed
	thrust_level = 1.0
	# planets stop a warp a little before impact
	if world != null:
		var p: Node2D = world.nearest_body(global_position)
		if p != null:
			var to_body: Vector2 = p.global_position - global_position
			var d: float = to_body.length() - p.radius
			# only a body ahead of us stops the warp; leaving one is fine
			if d < wspeed * 0.35 and velocity.dot(to_body) > 0.0:
				end_warp()
				warp_block = 6.0
				if is_player:
					world.notify(self, "Warp dropped: %s is dead ahead." % p.body_name)
				return
	# never warp into the Sun's heat, whoever is flying
	if world != null and "planet_by_name" in world:
		var sun: Node2D = world.planet_by_name.get("Sol", null)
		if sun != null and sun != nav_target:
			var to_sun: Vector2 = sun.global_position - global_position
			var vdir := velocity.normalized()
			var ahead := to_sun.dot(vdir)
			# the Sun is ahead, the line we fly passes inside its safe radius, and it is close
			if ahead > 0.0 and absf(to_sun.cross(vdir)) < _sun_safe_radius(sun) and ahead < _sun_safe_radius(sun) + wspeed * 1.2:
				end_warp()
				warp_block = 4.0
				if is_player:
					world.notify(self, "Warp dropped: too close to the Sun.")
				return
	if autopilot and nav_target != null and is_instance_valid(nav_target):
		var to := nav_target.global_position - global_position
		var dist: float = to.length() - nav_target.radius
		var leg: Vector2 = _route(to)["to"]
		var desired := leg.angle()
		rotation = rotate_toward(rotation, desired, deg_to_rad(turn_speed) * 0.5 * delta)
		if dist < warp_drop_distance():
			end_warp()


func start_warp() -> void:
	if warping:
		return
	var why := ""
	if systems.is_offline("qdrv"):
		why = "Quantum drive offline: damage control is on it."
	elif engines_out():
		why = "Engines offline: no warp."
	elif qdrv <= 0.0:
		why = "No quantum drive fitted."
	elif energy < 200.0:
		why = "Not enough energy to warp (needs 200)."
	if not why.is_empty():
		if is_player and world != null:
			world.notify(self, why)
		return
	warping = true
	warp_charge = 0.0
	Sfx.play(Sfx.leveled("warpin", module_levels.get("qdrv", 1)), global_position, 0.8)


func end_warp() -> void:
	if not warping:
		return
	var jumped := warp_charge >= WARP_CHARGE_TIME
	warping = false
	warp_charge = 0.0
	warp_block = maxf(warp_block, WARP_COOLDOWN)
	warp_pace = 0.0
	velocity = forward() * max_speed * 0.8
	Sfx.play("warpexit", global_position, 0.8)
	if jumped:
		_time_ripple()


## Speed in warp, once the drive has charged.
func warp_speed() -> float:
	return qdrv * GameData.LIGHT_SPEED * 1.25


## How far short of where it is going a warp should end, so the ship
## drops out and brakes in rather than overshooting.
func warp_drop_distance() -> float:
	return warp_speed() * 0.6 + 600.0


## Whether autopilot or a computer pilot may start a warp toward `to`
## (relative), `far` away: a drive, the energy, no recent drop-out, a
## clear line, and room for a real jump before the drop-out point.
func warp_worth(to: Vector2, far: float, min_far := 3500.0, min_energy := 400.0) -> bool:
	if warping or qdrv <= 0.0 or energy <= min_energy or warp_block > 0.0:
		return false
	if far <= maxf(min_far, warp_drop_distance() + warp_speed()):
		return false
	return not _body_in_path(to)


## A quantum jump tears time: a ripple spreads from where it starts and
## where it ends, seen on screen and by every temporal sensor about.
func _time_ripple() -> void:
	add_signature("temporal", Sensors.JUMP_TEMPORAL, Sensors.RIPPLE_TIME)
	if world != null and world.get("fx") != null and world.fx.has_method("time_ripple"):
		world.fx.time_ripple(global_position, size)


## How far autopilot keeps from a body's centre: the Sun's whole heat
## band plus a margin, a planet or moon 500 beyond its surface.
func _clearance(body: Node2D) -> float:
	if body.look == "Sun":
		var band: float = world.SUN_HEAT_RADII if "SUN_HEAT_RADII" in world else 4.0
		return body.radius * (1.0 + band) + size + 250.0
	return body.radius + size + 500.0


## The berth autopilot gives a body on the way to the nav target.  Bodies
## that sit beside the destination (its moons, its planet, a belt rock next
## to Mars) are passed close, just clear of their surface: the final
## approach has to go by them, and a wide berth would push the ship off the
## approach for ever.  The same goes for a body we are already close to,
## after lifting off or launching beside it: just climb clear of it.
func _route_clearance(body: Node2D) -> float:
	if body.look != "Sun":
		var full := _clearance(body)
		if global_position.distance_to(body.global_position) < full:
			return body.radius + size + 70.0
		if nav_target != null and nav_target.look != "Sun" \
				and body.global_position.distance_to(nav_target.global_position) < full + _clearance(nav_target) + 300.0:
			return body.radius + size + 70.0
	return _clearance(body)


func _sun_safe_radius(sun: Node2D) -> float:
	return _clearance(sun)


## The first body (other than the destination) whose clearance circle the
## straight line to `to` cuts, or null when the way is clear.
func _first_blocker(to: Vector2) -> Node2D:
	if world == null:
		return null
	var dir := to.normalized()
	var length := to.length()
	var best: Node2D = null
	var best_along := INF
	for p in world.planets:
		if p == nav_target:
			continue
		var rel: Vector2 = p.global_position - global_position
		var along := rel.dot(dir)
		if along <= 0.0 or along >= length or along >= best_along:
			continue
		if absf(rel.cross(dir)) < _route_clearance(p):
			best = p
			best_along = along
	return best


func _body_in_path(to: Vector2) -> bool:
	return _first_blocker(to) != null


## The vector autopilot actually flies along: straight to the target, or,
## when a body lies across that line, to a waypoint beside it that slides
## round the body as we go.  Inside the Sun's safe radius: straight out.
func _route(to: Vector2) -> Dictionary:
	if world == null:
		return {"to": to, "detour": false}
	var sun: Node2D = world.planet_by_name.get("Sol", null) if "planet_by_name" in world else null
	if sun != null and sun != nav_target:
		var out: Vector2 = global_position - sun.global_position
		if out.length() < _clearance(sun):
			return {"to": out.normalized() * 1500.0, "detour": true, "around": sun}
	# on the final approach, inside the moons' own orbits: straight in
	if nav_target != null and to.length() - nav_target.radius < nav_target.radius * 0.6 + 500.0:
		return {"to": to, "detour": false}
	var leg := to
	var around: Node2D = null
	# the leg to a waypoint can itself cross another body: route round that first
	for _i in 3:
		var body := _first_blocker(leg)
		if body == null or body == around:
			break
		around = body
		var rel: Vector2 = body.global_position - global_position
		var perp := leg.normalized().orthogonal()
		if perp.dot(-rel) < 0.0:
			perp = -perp                # pass on the side we are already on
		if absf(perp.dot(-rel)) < 1.0 and get_instance_id() % 2 == 0:
			perp = -perp                # dead on the line: pick a side and keep it
		leg = body.global_position + perp * _route_clearance(body) * 1.6 - global_position
	if around == null:
		return {"to": to, "detour": false}
	return {"to": leg, "detour": true, "around": around}


func _autopilot(_delta: float) -> void:
	var to := nav_target.global_position - global_position
	var dist: float = to.length() - nav_target.radius
	var route := _route(to)
	if route["detour"]:
		# fly the detour at speed; arrival logic waits until the way is clear
		var wv: Vector2 = route["to"]
		# steer the velocity, not just the nose, so sunward drift is cancelled
		var want: Vector2 = wv.normalized() * max_speed * 2.0 - velocity
		var wdiff := wrapf(want.angle() - rotation, -PI, PI)
		inputs["turn"] = clampf(wdiff * 3.0, -1.0, 1.0)
		inputs["thrust"] = 1.0 if absf(wdiff) < 0.7 else 0.25
		inputs["brake"] = false
		if absf(wrapf(wv.angle() - rotation, -PI, PI)) < 0.15 and warp_worth(wv, wv.length()):
			start_warp()
		var around: Node2D = route["around"]
		if (not has_meta("detour_around") or get_meta("detour_around") != around) and world != null:
			set_meta("detour_around", around)
			world.notify(self, "Autopilot: routing around %s." % ("the Sun" if around.look == "Sun" else around.body_name))
		return
	if has_meta("detour_around"):
		remove_meta("detour_around")
	var desired := to.angle()
	var diff := wrapf(desired - rotation, -PI, PI)
	inputs["turn"] = clampf(diff * 3.0, -1.0, 1.0)
	var rel: Vector2 = velocity - nav_target.velocity
	var closing: float = rel.dot(to.normalized())
	var stop_dist: float = closing * closing / maxf(2.0 * accel * 0.8, 1.0)
	if absf(diff) < 0.15 and warp_worth(to, dist):
		start_warp()
		return
	if dist > nav_target.radius * 0.6 + 260.0 and stop_dist < dist - 200.0:
		inputs["thrust"] = 1.0 if absf(diff) < 0.6 else 0.0
		inputs["brake"] = false
	else:
		inputs["thrust"] = 0.0
		inputs["brake"] = rel.length() > 40.0
		if rel.length() <= 40.0 and dist < nav_target.radius * 0.6 + 300.0:
			autopilot = false
			inputs["brake"] = false
			if world != null:
				world.notify(self, "Arrived at %s." % nav_target.body_name)


func _systems(delta: float) -> void:
	# damage control: the host's word is final online, so clients only show it
	if not Net.is_client():
		# damage control works as fast as there are hands for it
		var back := systems.tick(delta, regen_rate + repair_rate, crew_fraction())
		for e in back:
			if world != null:
				world.notify(self, "%s back online." % e["label"])
	if systems.changed:
		apply_systems()
	if systems.is_offline("reactor"):
		# the batteries run flat without a reactor behind them
		energy = maxf(energy - energy_max * 0.25 * delta, 0.0)
	energy = minf(energy + energy_regen * delta * 0.1 * (Sensors.EMCON_REGEN if sensors.emcon else 1.0), energy_max)
	# while the engines rest the reactor cracks a trickle of propellant, so
	# nobody is stranded for good; it never pays for flying
	if fuel < fuel_max and fuel_use <= 0.0 and not refuelling and not systems.is_offline("reactor"):
		fuel = minf(fuel + fuel_max * FUEL_TRICKLE * delta, fuel_max)
	_tick_shields(delta)
	_tick_armor(delta)
	sensors.tick(delta)
	# self-repair: regenerators work on their own once the shooting stops,
	# repair bays always, for energy
	if hull < hull_max:
		var quiet := (Time.get_ticks_msec() / 1000.0) - last_attacked_at > 4.0
		if regen_rate > 0.0 and quiet:
			hull = minf(hull + regen_rate * delta, hull_max)
		if repair_rate > 0.0:
			var cost := repair_rate * delta * 2.0
			if energy > cost + 50.0:
				energy -= cost
				hull = minf(hull + repair_rate * delta, hull_max)


func _weapons(delta: float) -> void:
	beam_targets.clear()
	for w in weapons:
		if w["cooldown"] > 0.0:
			w["cooldown"] -= delta
		# turrets visibly swing toward where they would shoot
		if is_turret(w) or is_free_aim(w):
			w["aim_vis"] = rotate_toward(w.get("aim_vis", w["fire_angle"]), _aim_angle(w), 5.0 * delta)
		var want := false
		if group_fire:
			# a human pilot fires by weapon group: fire_primary..fire_mine
			# carry groups 1..4 (so the network bits stay the same)
			want = inputs[GROUP_INPUTS[clampi(int(w["group"]), 1, 4) - 1]]
		else:
			match w["slot"]:
				"primary":
					want = inputs["fire_primary"]
				"secondary":
					want = inputs["fire_secondary"]
				"missile":
					want = inputs["fire_missile"]
				"mine":
					want = inputs["fire_mine"]
				"beam":
					want = inputs["fire_beam"]
		if not want or warping or landing != Landing.NONE:
			continue
		if systems.is_offline("w%d" % w["index"]):
			continue
		if w["kind"] == "beam":
			_fire_beam(w, delta)
		elif w["cooldown"] <= 0.0:
			_fire_tube(w)
	pdc.tick(self, delta, true)


## How far this ship's longest laser reaches (the AI opens fire inside it).
func beam_reach() -> float:
	var r := 0.0
	for w in weapons:
		if w["kind"] == "beam":
			r = maxf(r, w["range"])
	return r * 0.95


## Does any launcher (or PDC magazine) want reloading?
func needs_rearm() -> bool:
	if pdc.needs_rearm():
		return true
	for w in weapons:
		if w.get("ammo_max", -1) > 0 and w["ammo"] < w["ammo_max"]:
			return true
	return false


## Reload the launchers by `share` of each magazine (bases and colonies
## call this, as they refuel).
func rearm(share: float) -> void:
	pdc.rearm(share)
	for w in weapons:
		var full: int = w.get("ammo_max", -1)
		if full <= 0 or w["ammo"] >= full:
			continue
		w["ammo_f"] = float(w.get("ammo_f", 0.0)) + full * share
		var whole := int(w["ammo_f"])
		if whole > 0:
			w["ammo"] = mini(w["ammo"] + whole, full)
			w["ammo_f"] -= whole


func is_turret(w: Dictionary) -> bool:
	return w["arc1"] - w["arc0"] >= deg_to_rad(150.0)


## Shots that aim themselves from any mount, whatever its arc (Kyle: the
## disruptor is a lock/mouse weapon, it never fires down a rear rack).
const FREE_AIM_SHOTS := ["020"]

func is_free_aim(w: Dictionary) -> bool:
	return w["kind"] == "tube" and w["missile"] in FREE_AIM_SHOTS


## How far `rel` (ship-relative, -PI..PI) lies from the middle of a mount's
## arc, wrapped, so an arc the shipyard turned past astern still works.
static func _off_arc_mid(w: Dictionary, rel: float) -> float:
	return wrapf(rel - (w["arc0"] + w["arc1"]) * 0.5, -PI, PI)


## Is `rel` inside the mount's arc?
static func in_arc(w: Dictionary, rel: float, slack: float = 0.01) -> bool:
	if w["arc1"] - w["arc0"] >= TAU - 0.2:
		return true
	return absf(_off_arc_mid(w, rel)) <= (w["arc1"] - w["arc0"]) * 0.5 + slack


## `rel` held inside the mount's arc.
static func clamp_to_arc(w: Dictionary, rel: float) -> float:
	var half: float = (w["arc1"] - w["arc0"]) * 0.5
	return (w["arc0"] + w["arc1"]) * 0.5 + clampf(_off_arc_mid(w, rel), -half, half)


func _aim_angle(w: Dictionary) -> float:
	# Fire toward the target if it is inside the arc, else straight down the arc centre.
	var centre: float = w["fire_angle"]
	# a laser locks onto the targeted ship by itself (as in the original);
	# with no lock it follows the pilot's mouse like any turret (Kyle)
	if w["kind"] == "beam":
		var lock := _beam_lock(w)
		if lock != INF:
			return lock
	if is_free_aim(w):
		return _free_aim(w)
	# a turret under a pilot's hand points where the pilot points
	if aim_point != Vector2.INF and is_turret(w):
		var rel := wrapf((aim_point - global_position).angle() - rotation, -PI, PI)
		if w["arc1"] - w["arc0"] >= TAU - 0.2:
			return rel
		return clamp_to_arc(w, rel)
	if target != null and is_instance_valid(target) and not (target is Ship and target.dead):
		var rel := _lead_angle(w, target)
		if in_arc(w, rel):
			return rel
	return centre


## The mount-relative angle a laser holds on the targeted ship, or INF
## when there is no lock: no enemy ship targeted, not seen, out of reach
## or outside the mount's arc.
func _beam_lock(w: Dictionary) -> float:
	var t := _locked_foe()
	if t == null:
		return INF
	var to := t.global_position - global_position
	if to.length() > w["range"] + t.size:
		return INF
	var rel := wrapf(to.angle() - rotation, -PI, PI)
	if not in_arc(w, rel):
		return INF
	return rel


## The targeted enemy ship, when we hold a track on it (null otherwise).
func _locked_foe() -> Ship:
	if target == null or not is_instance_valid(target) or not (target is Ship):
		return null
	var t: Ship = target
	if t.dead or t.team() == team() or t.is_landing() or not t.visible_to(self):
		return null
	return t


## Ship-relative angle to fire a shot from `w` at `t`, led for a tube.
func _lead_angle(w: Dictionary, t: Node2D) -> float:
	var to := t.global_position - global_position
	var lead := to
	if t is Ship and w["kind"] == "tube":
		var md: Dictionary = GameData.MISSILES[w["missile"]]
		var mv: float = md["fire_v"] * GameData.LIGHT_SPEED
		var tt := to.length() / maxf(mv + 1.0, 1.0)
		lead = to + (t.velocity - velocity) * tt * 0.9
	return wrapf(lead.angle() - rotation, -PI, PI)


## A free-aim shot (the disruptor) goes at the locked target, else where
## the pilot points (mouse or right stick), else at whatever is targeted,
## else straight off the nose: never down the mount's own arc, which on a
## rear rack is behind the ship.
func _free_aim(w: Dictionary) -> float:
	var foe := _locked_foe()
	if foe != null:
		return _lead_angle(w, foe)
	if aim_point != Vector2.INF:
		return wrapf((aim_point - global_position).angle() - rotation, -PI, PI)
	if target != null and is_instance_valid(target) and not (target is Ship and target.dead):
		return _lead_angle(w, target)
	return 0.0


func _fire_tube(w: Dictionary) -> void:
	if w["ammo"] == 0:
		return
	if energy < w["cost"]:
		return
	energy -= w["cost"]
	# a damaged mount reloads slower
	w["cooldown"] = w["rate"] / maxf(systems.weapon_efficiency(w["index"]), 0.1)
	if w["ammo"] > 0:
		w["ammo"] -= 1
	# the launch flash gives the shooter away
	var flash: Array = Sensors.SHOT_SIGNATURE.get(w["missile"], [40.0, 0.0])
	add_signature("heat", flash[0], Sensors.SHOT_TIME)
	add_signature("em", flash[1], Sensors.SHOT_TIME)
	var md: Dictionary = GameData.MISSILES[w["missile"]]
	var rel := _aim_angle(w)
	if w["missile"] == "002":
		rel = PI   # mines drop behind
	if md.has("spread"):
		rel += deg_to_rad(randf_range(-md["spread"], md["spread"]))   # a spray, not a rifle
	var ang := rotation + rel
	var mount: Vector2 = global_position + Vector2.RIGHT.rotated(rotation + w["mount_angle"]) * w["mount_radius"]
	var dir := Vector2.RIGHT.rotated(ang)
	var v: Vector2 = velocity + dir * md["fire_v"] * GameData.LIGHT_SPEED
	if w["missile"] == "002":
		v = velocity * 0.6
	# a heavy gun kicks the ship back, at the mount, so a side gun twists it
	if md.has("recoil"):
		apply_hit_impulse(-dir * md["warhead"] * World.IMPULSE_PER_DAMAGE * md["recoil"], mount)
	if Net.is_client():
		return   # the host fires it and tells everyone, us included
	world.spawn_missile(self, w["missile"], mount + dir * size * 0.3, v, ang, target)
	if is_player:
		world.last_player_fire = world.elapsed
	var snd := "torpedo"
	match w["missile"]:
		"001": snd = "railgun"
		"003": snd = "plasma"
		"011": snd = "battery"
		"010": snd = "rapid"
		"004": snd = "grenade"
		"008", "009": snd = "missile"
		"002": snd = "mine"
		"018": snd = "cannon"
		"020": snd = "plasma"
		"021": snd = "missile"
	Sfx.play(snd, global_position, 0.7, randf_range(0.95, 1.05))


func _fire_beam(w: Dictionary, delta: float) -> void:
	var cost: float = w["cost"] * delta
	if energy < cost:
		return
	energy -= cost
	# a beam is quiet to infrared but loud to an EM receiver while it fires
	add_signature("heat", Sensors.BEAM_SIGNATURE[0], 0.25)
	add_signature("em", Sensors.BEAM_SIGNATURE[1], 0.25)
	var rel := _aim_angle(w)
	var ang := rotation + rel
	var origin := global_position + Vector2.RIGHT.rotated(ang) * size * 0.6
	var hit: Dictionary = world.raycast_beam(self, origin, Vector2.RIGHT.rotated(ang), w["range"])
	var end: Vector2 = hit["point"]
	if hit["body"] != null and not Net.is_client():   # damage is the host's
		# a beam spreads with distance: full burn up close, less at full reach
		var reach := clampf(origin.distance_to(end) / maxf(w["range"], 1.0), 0.0, 1.0)
		var dmg: float = w["dps"] * delta * systems.weapon_efficiency(w["index"]) * lerpf(1.0, GameData.BEAM_FALLOFF, reach)
		if hit["body"] is Ship:
			hit["body"].take_damage(dmg, self, end, 1.0, "", 0.1, "beam")
		else:
			hit["body"].take_damage(dmg, self, end, "beam")
		# a beam leans on what it burns
		world.push_node(hit["body"], Vector2.RIGHT.rotated(ang) * w["dps"] * delta * World.IMPULSE_PER_DAMAGE * 0.5, end)
	if hit["body"] != null:
		_beam_burn(w, hit["body"], end, delta)
	beam_targets.append([origin, end])
	# the beam itself is drawn by the world's Fx, so a laser is seen as well as felt
	if world != null and world.has_method("add_beam"):
		world.add_beam(origin, end, race_color())
	# bigger hulls hum lower
	Sfx.beam(self, clampf(1.3 - size / 80.0, 0.7, 1.2), int(str(w["id"]).right(1)))


## A beam's damage lands a sliver a frame, too small for take_damage's hit
## sparks and sounds, so the burn shows here: sparks where it touches and
## a shield or hull hit sound a few times a second.
const BEAM_BURN_EVERY := 0.12

func _beam_burn(w: Dictionary, body: Node, at: Vector2, delta: float) -> void:
	w["burn"] = float(w.get("burn", 0.0)) - delta
	if w["burn"] > 0.0 or world == null:
		return
	w["burn"] = BEAM_BURN_EVERY
	var shielded: bool = body is Ship and body.shield > 0.0
	if body is Ship:
		var col: Color = Color(0.6, 0.85, 1.0) if shielded else body.race_color()
		world.spawn_sparks(at, 3, col)
		if shielded:
			Sfx.play(Sfx.leveled("shield", body.module_levels.get("shield", 1)), at, 0.3, randf_range(0.95, 1.1))
		else:
			var armor_level: int = body.module_levels.get("armor", 0)
			Sfx.play(Sfx.leveled("hit_armor", armor_level) if armor_level > 0 else "hit", at, 0.3, randf_range(1.0, 1.2))
	else:
		world.spawn_sparks(at, 2, race_color())


# --------------------------------------------------------------- damage

## `at` is where the hit landed (Vector2.ZERO: nowhere in particular);
## `sys_mult` and `focus` say how hard it is on the ship's systems (see
## ShipSystems.hit).
func take_damage(amount: float, attacker: Node, at: Vector2 = Vector2.ZERO, sys_mult: float = 1.0, focus: String = "", crew_mult: float = 1.0, dtype: String = "kinetic", pierce: float = 0.0) -> void:
	if dead or invulnerable > 0.0 or race_id == GameData.NEUTRAL:
		return                         # (the free port is under every empire's protection)
	if land_scale < 0.5:
		return                         # down in the atmosphere, out of the fight
	if attacker is Ship and attacker != self:
		last_attacker = attacker
		last_attacked_at = Time.get_ticks_msec() / 1000.0
		if is_player and autopilot and attacker.team() != team():
			autopilot = false
			if warping:
				end_warp()
			target = attacker
			if world != null:
				world.notify(self, "Autopilot off: under fire from %s. Targeted." % attacker.pilot_name)
	hit_flash = 0.15
	if at != Vector2.ZERO:
		last_hit_angle = (at - global_position).angle() - rotation
	elif attacker is Node2D:
		last_hit_angle = (attacker.global_position - global_position).angle() - rotation
	var remaining := amount
	var bleed := 0.0
	# the shield half on the side the hit came from takes it
	var front := cos(last_hit_angle) >= 0.0
	var half := 0 if front else 1
	if shields_raised and shield_boot <= 0.0 and shield_max > 0.0:
		var fam: Dictionary = GameData.SHIELD_FAMILIES[shield_family]
		var charge: float = shield_front if front else shield_rear
		if charge > 0.0:
			# a Flicker shield lets part of some hits straight through
			var leak := 0.0
			if fam["leak"] > 0.0 and randf() < fam["leak_chance"]:
				leak = remaining * fam["leak"]
				remaining -= leak
			# the shield takes the hit at its damage type's rate (GameData.DAMAGE_TYPES)
			var ms := maxf(GameData.damage_mult(dtype, "shield"), 0.001)
			var absorbed := minf(charge, remaining * ms)
			charge -= absorbed
			if front:
				shield_front = charge
			else:
				shield_rear = charge
			remaining -= absorbed / ms
			remaining += leak
			bleed = absorbed * ShipSystems.SHIELD_BLEED
			if absorbed > 4.0:
				Sfx.play(Sfx.leveled("shield", module_levels.get("shield", 1)), global_position, 0.5, randf_range(0.92, 1.08))
		# every hit holds that half's recharge back, longer once it is down
		shield_pause[half] = maxf(shield_pause[half], fam["zero_pause"] if charge <= 0.01 else fam["pause"])
	remaining *= GameData.damage_mult(dtype, "hull")
	if remaining > 0.0:
		remaining = _armor_cut(remaining, dtype, pierce, at)
		hull -= remaining
		var bulk := coat_kind == "bulkheads"
		_hit_systems((remaining + bleed) * (GameData.BULKHEAD_SYSTEMS if bulk else 1.0), attacker, at, sys_mult, focus)
		if not Net.is_client() and not is_starbase():
			crew = maxf(crew - crew_max * remaining / maxf(hull_max, 1.0) * CREW_LOSS * crew_mult * (GameData.BULKHEAD_CREW if bulk else 1.0), 0.0)
		if remaining > 4.0:
			var armor_level: int = module_levels.get("armor", 0)
			Sfx.play(Sfx.leveled("hit_armor", armor_level) if armor_level > 0 else "hit", global_position, 0.55, randf_range(0.85, 1.15))
			if world != null:
				world.spawn_sparks(at if at != Vector2.ZERO else global_position, 4, race_color())
	elif bleed > 0.0:
		_hit_systems(bleed, attacker, at, sys_mult, focus)
	damaged.emit(self, amount)
	if hull <= 0.0:
		_die(attacker)


## Pass a hit's hull damage on to the systems near where it landed.
func _hit_systems(amount: float, attacker: Node, at: Vector2, sys_mult: float, focus: String) -> void:
	if Net.is_client() or systems.list.is_empty():
		return
	var local := Vector2.INF
	if at != Vector2.ZERO:
		local = (at - global_position).rotated(-rotation)
	var was_disabled := systems.disabled()
	var broke := systems.hit(amount, local, size, sys_mult, focus)
	if systems.changed:
		apply_systems()
	for e in broke:
		system_failed.emit(self, e, attacker)
		if world != null:
			world.spawn_sparks(global_position + (e["pts"][0] as Vector2).rotated(rotation), 10, Color(1.0, 0.6, 0.2))
	if not was_disabled and systems.disabled():
		Sfx.play("hit", global_position, 0.8, 0.55)


## What reaches the hull of a hit after the armor coat (GameData.ARMOR).
func _armor_cut(dmg: float, dtype: String, pierce: float, at: Vector2) -> float:
	if coat == "":
		return dmg
	var r := GameData.armor_resist(coat, dtype) * (1.0 - clampf(pierce, 0.0, 1.0))
	match coat_kind:
		"reflective":
			if hull < hull_max * GameData.REFLECTIVE_SCORCHED:
				r *= 0.5                  # scorched and cratered
		"ablative":
			if ablative <= 0.0:
				return dmg
			var stopped := minf(dmg * r, ablative)
			ablative -= stopped
			return dmg - stopped
		"reactive":
			if dmg < GameData.REACTIVE_TRIGGER or reactive_tiles <= 0 or r <= 0.0:
				return dmg
			reactive_tiles -= 1
			reactive_rearm = 0.0
			_reactive_blast(at)
	return dmg * (1.0 - r)


## A reactive tile goes off: a flash on the hull that infrared sees.
func _reactive_blast(at: Vector2) -> void:
	add_signature("heat", 150.0, 2.0)  # the blast shows on infrared
	if world != null:
		world.spawn_sparks(at if at != Vector2.ZERO else global_position, 14, Color(1.0, 0.75, 0.35))
	Sfx.play("hit", global_position, 0.7, 1.35)


## The front or rear half's share of the shield's size.
func shield_cap(front: bool) -> float:
	return shield_max * (shield_share if front else 1.0 - shield_share)


## Set the shield's charge, `total`, with `front_share` of it in the front
## half (each half capped at its size); lowered shields hold nothing.
func set_shield_split(total: float, front_share: float) -> void:
	if not shields_raised or shield_boot > 0.0:
		shield_front = 0.0
		shield_rear = 0.0
		return
	total = clampf(total, 0.0, shield_max)
	shield_front = minf(total * clampf(front_share, 0.0, 1.0), shield_cap(true))
	shield_rear = minf(total - shield_front, shield_cap(false))
	shield_front = minf(total - shield_rear, shield_cap(true))


## Keep each half inside its size, passing what spills over to the other.
func _fit_shield_halves() -> void:
	var cf := shield_cap(true)
	var cr := shield_cap(false)
	if shield_front > cf:
		shield_rear = minf(shield_rear + shield_front - cf, cr)
		shield_front = cf
	if shield_rear > cr:
		shield_front = minf(shield_front + shield_rear - cr, cf)
		shield_rear = cr


## Focus the shield: 0 balanced, 1 forward, 2 aft.  The charge moves over
## GameData.SHIELD_FOCUS_TIME, so it can't be flipped at the last moment.
func set_shield_focus(f: int) -> void:
	shield_focus = posmod(f, GameData.SHIELD_FOCUS.size())
	shield_share_rate = absf(GameData.SHIELD_FOCUS[shield_focus] - shield_share) / GameData.SHIELD_FOCUS_TIME


func cycle_shield_focus() -> void:
	set_shield_focus(shield_focus + 1)
	if is_player and world != null:
		world.notify(self, "Shields: %s." % GameData.SHIELD_FOCUS_NAMES[shield_focus])


## Lower the shields (no draw, nothing given off) or raise them again: they
## take GameData.SHIELD_BOOT to come up, at a share of full.
func set_shields_raised(on: bool) -> void:
	if on == shields_raised or float(stats.get("shield_max", 0.0)) <= 0.0:
		return
	shields_raised = on
	shield_front = 0.0
	shield_rear = 0.0
	shield_boot = GameData.SHIELD_BOOT if on else 0.0
	shield_pause = [0.0, 0.0]
	Sfx.play("cloak" if not on else "uncloak", global_position, 0.45, 1.4)
	if is_player and world != null:
		world.notify(self, "Shields raising." if on else "Shields lowered: running quiet.")
	apply_systems()


## Lowered shields free their share of the reactor: an overloaded ship
## recharges its batteries that much faster.
func _lowered_shield_bonus() -> float:
	if shields_raised or overload <= 1.0 or power_gen <= 0.0:
		return 1.0
	var without := maxf((power_draw - float(stats.get("shield_draw", 0.0))) / power_gen, 1.0)
	return overload / without


func _tick_shields(delta: float) -> void:
	if shield_boot > 0.0:
		shield_boot -= delta
		if shield_boot <= 0.0:
			shield_boot = 0.0
			shield_front = shield_cap(true) * GameData.SHIELD_BOOT_CHARGE
			shield_rear = shield_cap(false) * GameData.SHIELD_BOOT_CHARGE
		return
	if not shields_raised:
		return
	# the charge follows the focus
	var want: float = GameData.SHIELD_FOCUS[shield_focus]
	if shield_share != want:
		shield_share = move_toward(shield_share, want, maxf(shield_share_rate, 0.05) * delta)
		_fit_shield_halves()
	for i in 2:
		if shield_pause[i] > 0.0:
			shield_pause[i] = maxf(shield_pause[i] - delta, 0.0)
			continue
		var cap := shield_cap(i == 0)
		var have: float = shield_front if i == 0 else shield_rear
		if have >= cap or energy <= 2.0 or sensors.emcon:
			continue                   # (silent running holds the recharge too)
		var regen := minf(shield_regen * (cap / maxf(shield_max, 1.0)) * delta, cap - have)
		var cost := regen * 1.5
		if energy < cost:
			continue
		energy -= cost
		if i == 0:
			shield_front += regen
		else:
			shield_rear += regen


## Is a shield half recharging (sensors hear a charging shield louder)?
func shield_recharging() -> bool:
	return shields_raised and shield_boot <= 0.0 and shield < shield_max - 0.5 and (shield_pause[0] <= 0.0 or shield_pause[1] <= 0.0)


func _tick_armor(delta: float) -> void:
	if reactive_tiles < reactive_max and (Time.get_ticks_msec() / 1000.0) - last_attacked_at > 4.0:
		reactive_rearm += delta
		if reactive_rearm >= GameData.REACTIVE_REARM:
			reactive_rearm = 0.0
			reactive_tiles += 1
	# a repair bay mends an ablative layer as it mends the hull
	if ablative < ablative_max and repair_rate > 0.0 and hull >= hull_max:
		ablative = minf(ablative + repair_rate * delta, ablative_max)


## A friendly base or colony refills the ablative layer and re-arms the
## reactive tiles.
func restock_armor() -> void:
	ablative = ablative_max
	reactive_tiles = reactive_max
	reactive_rearm = 0.0


func armor_spent() -> bool:
	return ablative < ablative_max or reactive_tiles < reactive_max


## How much of the armor coat still shows on the hull: an ablative layer
## burns away, spent reactive tiles leave gaps, scorched mirror dulls.
func _coat_wear() -> float:
	match coat_kind:
		"ablative":
			return ablative / maxf(ablative_max, 1.0)
		"reactive":
			return 0.35 + 0.65 * float(reactive_tiles) / maxi(reactive_max, 1)
		"reflective":
			return 0.55 if hull < hull_max * GameData.REFLECTIVE_SCORCHED else 1.0
	return 1.0


## How much harder defenders fight off boarders (Bulkheads).
func board_defence() -> float:
	return GameData.BULKHEAD_BOARD if coat_kind == "bulkheads" else 1.0


func _die(killer: Node) -> void:
	if dead:
		return
	dead = true
	hull = 0.0
	deaths += 1
	boarding = null
	board_progress = 0.0
	boarded_by = null
	colonists = 0.0                    # the colonists go down with the ship
	colonist_origin = null
	troops.clear()                     # and so do the troops
	if landing != Landing.NONE:
		_end_landing()
		body.enabled = false
	set_jam(false)
	warping = false
	if engine_sound != null:
		engine_sound.silence()
	died.emit(self, killer)
	cargo.clear()                      # what the wreck spilled is salvage now
	cargo_orders.clear()
	cargo_origin = null
	kit = ""
	if body != null:
		body.enabled = false
	hide()
	set_physics_process(false)


func respawn(at: Vector2, vel: Vector2) -> void:
	dead = false
	captured = false
	if landing != Landing.NONE:
		_end_landing()
	refit(design)
	sensors.reset()
	global_position = at
	velocity = vel
	rotation = randf() * TAU
	angular_velocity = 0.0
	reset_physics_interpolation()
	if body != null:
		body.enabled = true
		body.pos = at
		body.vel = vel
		body.angle = rotation
		body.ang_vel = 0.0
	invulnerable = 3.0
	target = null
	autopilot = false
	show()
	set_physics_process(true)


## Credits paid to the pilot, counted for the scoreboard.
func earn(amount: int) -> void:
	credits += amount
	credits_earned += amount


func add_score(points: int, is_kill: bool) -> void:
	score += points
	if is_kill:
		kills += 1
	var r := GameData.rank_for(score, kills)
	if r != rank:
		rank = r
		rank_changed.emit(self, r)


## A floating module.  The pilot stows it in the hold for the shipyard;
## a computer pilot fits it on the spot if it is an upgrade.
func try_pickup(module_id: String) -> bool:
	if is_player:
		if hold.size() >= HOLD_SIZE:
			return false
		hold.append(module_id)
		return true
	var m: Dictionary = GameData.MODULES[module_id]
	var level := GameData.tier(module_id)
	var f: String = m["func"]
	var have: int = module_levels.get(f, 0)
	# the systems' whole numbers change; apply_systems derates them again
	var b := systems.base
	match f:
		"shield":
			if level <= have or (have > 0 and GameData.shield_family(module_id) != shield_family):
				return false
			var add: float = m["p"][0] - (b["shield_max"] if have > 0 else 0.0)
			shield_family = GameData.shield_family(module_id)
			b["shield_max"] = m["p"][0]
			b["shield_regen"] = b["shield_max"] * GameData.SHIELD_FAMILIES[shield_family]["rate"]
			var was := shield
			shield_max = b["shield_max"]
			shield = minf(was + maxf(add, 0.0), b["shield_max"])
		"eng":
			if level <= have:
				return false
			b["thrust_force"] *= 1.25
			fuel = fuel_max
		"sensor":
			var fam: String = m["fam"]
			if float(m["p"][0]) <= sensors.suite.get(fam, 0.0):
				return false
			sensors.suite[fam] = float(m["p"][0])
			stats["sensors"][fam] = float(m["p"][0])
			if GameData.SENSOR_FAMILIES[fam]["track"]:
				b["sensor_range"] = maxf(b["sensor_range"], float(m["p"][0]))
		"power":
			if level <= have:
				return false
			energy_max += m["power"] * 2.0
			b["energy_regen"] += m["power"] * 0.2 * GameData.ENERGY_REGEN_SCALE
		"jammer":
			if b["can_jam"]:
				return false
			b["can_jam"] = true
			stats["can_jam"] = true
			stats["jam_reach"] = float(m["p"][0])
		"boost":
			if b["has_boost"] and level <= have:
				return false
			b["has_boost"] = true
			var g := GameData.boost_gain(m["p"])
			boost_speed = 1.0 + g.x
			boost_accel = 1.0 + g.y
		"armor":
			# a computer pilot only bolts on thicker plate
			if GameData.ARMOR[module_id]["coat"] != "plate" or (coat != "" and (coat_kind != "plate" or level <= have)):
				return false
			var gain: float = m["hp"] - (GameData.MODULES[coat]["hp"] if coat != "" else 0.0)
			coat = module_id
			coat_kind = "plate"
			armor = 1.0
			hull_max += gain
			hull += gain
		_:
			return false
	apply_systems()
	module_levels[f] = maxi(level, have)
	return true


# ---------------------------------------------------------------- drawing

## Smoke, fire and sparks where the systems are hurt: a thin wisp on a
## damaged one, flames and arcing on one that is out.
## A station going up: scaffold rings round the ghost of the finished
## station, and an arc for the work done once the materials are in.
func _draw_frame() -> void:
	var c := race_color()
	c.a = 0.55
	var r := size * 1.15
	for i in 12:
		var a0 := i * TAU / 12.0 + rotation * -1.0
		draw_arc(Vector2.ZERO, r, a0, a0 + TAU / 24.0, 6, c, 2.0, true)
	draw_line(Vector2(-r, 0).rotated(-rotation), Vector2(r, 0).rotated(-rotation), Color(c, 0.25), 1.5)
	draw_line(Vector2(0, -r).rotated(-rotation), Vector2(0, r).rotated(-rotation), Color(c, 0.25), 1.5)
	var key := design_name
	var done := cargo_total() / maxf(cargo_max, 1.0)
	draw_arc(Vector2.ZERO, r + 7.0, -PI * 0.5 - rotation, -PI * 0.5 - rotation + TAU * done, 40, Color(0.95, 0.8, 0.35, 0.8), 3.0, true)
	if GameData.STATIONS.has(key) and build_t > 0.0:
		var f := build_t / float(GameData.STATIONS[key]["build"])
		draw_arc(Vector2.ZERO, r + 13.0, -PI * 0.5 - rotation, -PI * 0.5 - rotation + TAU * f, 40, Color(0.5, 1.0, 0.6, 0.85), 3.0, true)


func _draw_system_damage() -> void:
	var a := 1.0
	if a < 0.1:
		return
	var now := Time.get_ticks_msec() / 1000.0
	for e in systems.list:
		var h: float = e["hp"] / e["max"]
		if h >= ShipSystems.DAMAGED_BELOW and not e["offline"]:
			continue
		var lvl := 1.0 if e["offline"] else 1.0 - h / ShipSystems.DAMAGED_BELOW
		for p: Vector2 in e["pts"]:
			var seed_v := p.x * 13.1 + p.y * 7.7
			# smoke drifting off the wound
			for k in 3:
				var ph := fmod(now * 0.7 + k / 3.0 + seed_v, 1.0)
				var off := Vector2(-ph * size * 0.5, sin(seed_v + k) * size * 0.12 * ph)
				draw_circle(p + off, size * (0.05 + 0.1 * ph) * (0.6 + lvl), Color(0.12, 0.11, 0.1, (1.0 - ph) * 0.5 * lvl * a))
			if e["offline"]:
				var flick := 0.6 + 0.4 * sin(now * 23.0 + seed_v)
				draw_circle(p, size * 0.08 * flick, Color(1.0, 0.45, 0.1, 0.75 * a))
				draw_circle(p, size * 0.04 * flick, Color(1.0, 0.9, 0.5, 0.9 * a))
				# arcing sparks now and then
				if fmod(now * 3.0 + seed_v, 1.0) < 0.35:
					var ang := now * 9.0 + seed_v
					var q := p + Vector2(cos(ang), sin(ang)) * size * 0.2
					draw_line(p, q, Color(0.7, 0.85, 1.0, 0.8 * a), 1.2)
			elif fmod(now * 1.7 + seed_v, 1.0) < 0.15 * lvl:
				draw_circle(p, size * 0.035, Color(1.0, 0.7, 0.3, 0.8 * a))


func _draw() -> void:
	if dead:
		return
	var base := race_color()
	var dmg := 1.0 - hull / maxf(hull_max, 1.0)
	var aims := []
	for w in weapons:
		aims.append(w.get("aim_vis", w["fire_angle"]))
	if art_model.is_empty():
		art_model = ShipArt.build_model(design, hull_race, size)
	var broken := systems.broken_map()
	ShipArt.draw_model(self, art_model, base, thrust_level, (0.4 if building else 1.0), dmg, Transform2D.IDENTITY, aims, clampf(hit_flash * 5.0, 0.0, 1.0), broken, _coat_wear(), pdc.aims())
	# PDC muzzle flashes
	for mt in pdc.mounts:
		if mt["flash"] > 0.0:
			var tip: Vector2 = mt["pos"] + Vector2.RIGHT.rotated(mt["aim"]) * size * 0.3
			draw_circle(tip, 3.0, Color(1.0, 0.85, 0.45, 0.9))
			draw_circle(tip, 1.6, Color(1, 1, 1, 0.95))
	if building:
		_draw_frame()
	if not broken.is_empty():
		_draw_system_damage()
	if is_disabled() or is_breached():
		# a dead ship's running light blinks red, so it can be found and boarded
		var blink := 0.5 + 0.5 * sin(Time.get_ticks_msec() / 1000.0 * 5.0)
		draw_arc(Vector2.ZERO, size * 1.25, 0, TAU, 40, Color(1.0, 0.25, 0.15, 0.25 + 0.3 * blink), 1.5, true)
	if boarding != null and is_instance_valid(boarding) and not boarding.dead:
		# the docking tractor holds the prize alongside
		var to := (boarding.global_position - global_position).rotated(-rotation)
		var c := Color(0.55, 0.9, 1.0, 0.35 + 0.25 * sin(Time.get_ticks_msec() / 90.0))
		draw_line(Vector2.ZERO, to, c, 3.0, true)
		draw_line(Vector2.ZERO, to, Color(1, 1, 1, 0.5), 1.0, true)
	if sensors.jam_active():
		# the jammer's beam: static crawling toward the target
		var tj: Node2D = sensors.jam_target
		var to := (tj.global_position - global_position).rotated(-rotation)
		var n := 14
		var prev := Vector2.ZERO
		for k in range(1, n + 1):
			var q := to * (float(k) / n)
			if k < n:
				q += to.orthogonal().normalized() * randf_range(-6.0, 6.0)
			if k % 2 == 0:
				draw_line(prev, q, Color(0.85, 0.55, 1.0, 0.35), 1.5, true)
			prev = q
	# the shield is drawn by shield_bubble
	if is_player and landing == Landing.LANDED:
		# where you are on the ground: a beacon round the speck
		var pulse := 0.5 + 0.5 * sin(Time.get_ticks_msec() / 220.0)
		var bc := race_color()
		bc.a = 0.5 + 0.4 * pulse
		draw_arc(Vector2.ZERO, size * (2.6 + pulse * 0.8), 0, TAU, 32, bc, size * 0.35, true)
	var heat := reentry_heat()
	if heat > 0.01:
		# re-entry: a hot bow wave on the leading face, toward the ground
		for i in 3:
			var hc := Color(1.0, 0.55 - i * 0.12, 0.2, heat * (0.55 - i * 0.15))
			draw_arc(Vector2.ZERO, size * (1.05 + i * 0.22), -0.9 + i * 0.1, 0.9 - i * 0.1, 20, hc, size * (0.16 - i * 0.03), true)
	# turret arcs for the pilot's own ship
	if is_player and Settings.mouse_aim and aim_point != Vector2.INF:
		for w in weapons:
			if is_turret(w) and w["arc1"] - w["arc0"] < TAU - 0.2:
				draw_arc(Vector2.ZERO, size * 1.1, w["arc0"], w["arc1"], 24, Color(1, 1, 1, 0.12), 1.0, true)
	if warping and warp_charge < WARP_CHARGE_TIME:
		var c := Color(0.6, 0.8, 1.0, warp_charge / WARP_CHARGE_TIME)
		draw_arc(Vector2.ZERO, size * (2.6 - 1.4 * warp_charge / WARP_CHARGE_TIME), 0, TAU, 48, c, 2.5, true)
	elif warping:
		for i in 6:
			var c := Color(0.5, 0.7, 1.0, 0.3 - i * 0.045)
			draw_line(Vector2(-size * (1.0 + i * 0.9), randf_range(-size, size) * 0.6), Vector2(-size * (2.5 + i * 1.2), randf_range(-size, size) * 0.6), c, 2.0)
