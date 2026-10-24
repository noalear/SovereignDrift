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

const BOOST_SPEED_MULT := 1.5
const BOOST_ACCEL_MULT := 2.2
const WARP_CHARGE_TIME := 1.1
## Engine thrust per point of engine power, in mass units * px/s^2.
const THRUST_PER_ENGINE := 4260.0
## Fuel units carried per point of engine power (300 s of full burn).
const FUEL_PER_ENGINE := 6.0
## Fuel burned per second at full thrust per point of engine power.
const BURN_PER_ENGINE := 0.02
## Mass of one fuel unit.
const FUEL_MASS := 0.5
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
var thrust_level := 0.0            # for the engine glow, 0..1
var flight_assist := true          # RCS holds the turn rate and stops a tumble
var orbit_guard := false           # the engines are busy steering clear of a planet
var refuelling := false            # near a friendly base or colony this frame

# systems
var energy := 1000.0
var energy_max := 1000.0
var energy_regen := 200.0
var shield := 0.0
var shield_max := 0.0
var shield_regen := 0.0
var hull := 100.0
var hull_max := 100.0
var armor := 0.0
var sensor_range := 1200.0
var can_cloak := false
var cloaked := false
var cloak_alpha := 1.0
var has_boost := false
var has_dock := false
var qdrv := 0.0
var warping := false
var warp_block := 0.0              # seconds before autopilot may warp again after a drop-out
var warp_charge := 0.0
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
var blueprints: Array = []         # enemy parts this pilot may buy
const HOLD_SIZE := 16
# this round's record, for the scoreboard
var captures := 0
var credits_earned := 0

var weapons: Array = []            # see _build_weapons
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
		design = GameData.stock_design(p_design)
	else:
		design = p_design
	design_name = design["name"]
	is_station = design_name == "Starbase"
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
	sensor_range = stats["sensor_range"]
	can_cloak = stats["can_cloak"]
	has_boost = stats["has_boost"]
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
	fuel = fuel_max
	crew_max = GameData.HULL_CREW.get(design_name, 10)
	crew = crew_max
	accel = thrust_force / wet_mass()
	energy = energy_max
	shield = shield_max
	hull = hull_max
	_build_weapons()
	art_model = ShipArt.build_model(design, hull_race, size)
	systems.build(self)
	apply_systems()
	_build_body()
	if shield_bubble != null:
		shield_bubble.fit()
	queue_redraw()


## Set the ship's working numbers from its systems' health: damaged
## engines push less, a hurt reactor recharges slower, broken shield
## generators hold less, blind sensors see less.  Called whenever a
## system's efficiency moves.
func apply_systems() -> void:
	var b := systems.base
	if b.is_empty():
		return
	systems.changed = false
	var eng := systems.efficiency("engines")
	thrust_force = b["thrust_force"] * eng
	max_speed = b["max_speed"] * (0.6 + 0.4 * eng if eng > 0.0 else 1.0)
	has_boost = b["has_boost"] and eng >= 1.0
	energy_regen = b["energy_regen"] * systems.efficiency("reactor")
	var sh := systems.efficiency("shields")
	shield_max = b["shield_max"] * sh
	shield_regen = b["shield_regen"] * sh
	shield = minf(shield, shield_max)
	var sn := systems.efficiency("sensors")
	sensor_range = b["sensor_range"] * lerpf(ShipSystems.BLIND_SENSORS, 1.0, sn)
	can_cloak = b["can_cloak"] and not systems.is_offline("cloak")
	if cloaked and not can_cloak:
		set_cloak_forced_off()
	qdrv = b["qdrv"] if not systems.is_offline("qdrv") else 0.0
	if warping and (qdrv <= 0.0 or engines_out()):
		end_warp()


## Dead in space: engines or reactor shot out, or nobody left aboard.
func is_disabled() -> bool:
	return systems.disabled() or (crew < 0.5 and not is_station)


## Is a pilot of `race` boarding us, or on the way to?  Its friends hold
## their fire so it can take us alive.
func spoken_for(race: int) -> bool:
	for c in [boarded_by, claimed_by]:
		if c != null and is_instance_valid(c) and not c.dead and c.race_id == race and (c.boarding == self or c.target == self):
			return true
	return false


func crew_fraction() -> float:
	return crew / maxf(crew_max, 1.0)


func engines_out() -> bool:
	return systems.is_offline("engines")


func set_cloak_forced_off() -> void:
	cloaked = false
	Sfx.play("uncloak", global_position, 0.6)


## Dry mass plus the fuel on board.
func wet_mass() -> float:
	return mass + fuel * FUEL_MASS


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
	for w in design["weapons"]:
		var m: Dictionary = GameData.MODULES[w[0]]
		# shots leave from the turret as drawn on the painted hull
		var spot := ShipArt.weapon_spot(design["name"], hull_race, size, w)
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
			entry["range"] = float(m["p"][2]) * 2.2
			entry["dps"] = 45.0 + float(m["p"][1]) * 0.15
			entry["rate"] = 0.0
		weapons.append(entry)
		if is_turret(entry):
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


func is_cloaked_to(viewer: Node) -> bool:
	return cloaked and viewer != self and cloak_alpha < 0.35


func visible_to(viewer: Ship) -> bool:
	if viewer == self:
		return true
	var d := global_position.distance_to(viewer.global_position)
	if d > viewer.sensor_range * (0.25 if cloaked else 1.0):
		return false
	return true


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
	_systems(delta)
	_weapons(delta)
	queue_redraw()


## Called by the world before the physics step: copy the node's state into
## the body (other code may have moved the ship) and apply this frame's
## engine and RCS forces.
func pre_step(delta: float) -> void:
	body.enabled = not dead
	if dead:
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
	if dead:
		return
	global_position = body.pos
	velocity = body.vel
	rotation = body.angle
	angular_velocity = body.ang_vel


func _fly(delta: float) -> void:
	var fwd := forward()
	var boosting: bool = inputs["boost"] and has_boost and energy > 5.0 and inputs["thrust"] > 0.0 and fuel > 0.0
	var vmax := max_speed * (BOOST_SPEED_MULT if boosting else 1.0)
	# the rating is the speed the drive holds in open space; inside a
	# gravity well the cap is on the energy, so a ship can always climb out:
	# locally that means the escape speed on top of the rating
	var v_esc := escape_speed()
	vmax = sqrt(vmax * vmax + v_esc * v_esc)
	var f0 := thrust_force * (BOOST_ACCEL_MULT if boosting else 1.0)
	accel = thrust_force / wet_mass()

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
	var dead_drive := fuel <= 0.0 or engines_out()
	if dead_drive:
		torque_max *= 0.3          # reaction wheels on reactor power only
	if flight_assist or turn != 0.0:
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
		var brake_a := minf(maxf(accel * brake_power * 1.5, 170.0 * brake_power), sp / delta)
		f -= lv.normalized() * brake_a * body.mass
	var engine_level := f.length() / maxf(thrust_force, 1.0)
	# the orbit guard: with assist on, a course that would end on a planet's
	# surface is flown into a circular orbit on the ship's own engines
	var hands_off: bool = t == 0.0 and st == 0.0 and not inputs["brake"]
	var guard := _orbit_guard_force(fwd, f0, delta) if flight_assist and (hands_off or not is_player) else Vector2.ZERO
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
	# --- fuel: engines burn by thrust, boost burns hotter, RCS sips
	var burn := engine_level * burn_rate * (2.5 if boosting else 1.0) + rcs_level * RCS_BURN
	if burn > 0.0:
		if fuel < burn * delta and world != null and is_player and fuel > 0.0:
			world.notify(self, "Fuel exhausted. Drift to a friendly base or colony to refuel.")
		fuel = maxf(fuel - burn * delta, 0.0)
	if boosting:
		energy = maxf(energy - 75.0 * delta, 0.0)
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
	cloak_alpha = lerpf(cloak_alpha, 0.12 if cloaked else 1.0, 4.0 * delta)
	if engine_sound != null:
		engine_sound.update(thrust_level, false, warping, delta)
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


func _warp(delta: float) -> void:
	var fwd := forward()
	if warp_charge < WARP_CHARGE_TIME:
		warp_charge += delta
		velocity = velocity.lerp(fwd * max_speed * 0.5, 2.0 * delta)
		thrust_level = lerpf(thrust_level, 1.0, 5.0 * delta)
		if warp_charge >= WARP_CHARGE_TIME:
			Sfx.play(Sfx.leveled("warpout", module_levels.get("qdrv", 1)), global_position, 0.8)
		return
	var drain := 160.0 * delta
	if energy < drain or inputs["brake"]:
		end_warp()
		return
	energy -= drain
	var wspeed := qdrv * GameData.LIGHT_SPEED * 1.25
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
		if dist < wspeed * 0.6 + 600.0:
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
	elif cloaked:
		why = "Can't warp while cloaked."
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
	warping = false
	warp_charge = 0.0
	velocity = forward() * max_speed * 0.8
	Sfx.play("warpexit", global_position, 0.8)


## How far autopilot keeps from a body's centre: the Sun's whole heat
## band plus a margin, a planet or moon 500 beyond its surface.
func _clearance(body: Node2D) -> float:
	if body.look == "Sun":
		var band: float = world.SUN_HEAT_RADII if "SUN_HEAT_RADII" in world else 4.0
		return body.radius * (1.0 + band) + size + 250.0
	return body.radius + size + 500.0


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
		if absf(rel.cross(dir)) < _clearance(p):
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
		leg = body.global_position + perp * _clearance(body) * 1.6 - global_position
	if around == null:
		return {"to": to, "detour": false}
	return {"to": leg, "detour": true, "around": around}


func _autopilot(delta: float) -> void:
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
		warp_block = maxf(warp_block - delta, 0.0)
		if wv.length() > 3500.0 and absf(wrapf(wv.angle() - rotation, -PI, PI)) < 0.15 and qdrv > 0.0 and energy > 400.0 \
				and warp_block <= 0.0 and not _body_in_path(wv):
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
	warp_block = maxf(warp_block - delta, 0.0)
	if dist > 3500.0 and absf(diff) < 0.15 and qdrv > 0.0 and energy > 400.0 and warp_block <= 0.0 and not _body_in_path(to):
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
	energy = minf(energy + energy_regen * delta * 0.1, energy_max)
	# the reactor cracks a trickle of propellant, so nobody is stranded for good
	if fuel < fuel_max and energy > energy_max * 0.3 and not refuelling:
		fuel = minf(fuel + fuel_max * 0.0025 * delta, fuel_max)
	if shield < shield_max and energy > 2.0:
		var regen := minf(shield_regen * delta, shield_max - shield)
		var cost := regen * 1.5
		if energy >= cost:
			shield += regen
			energy -= cost
	if cloaked:
		var cost := 55.0 * delta
		if energy < cost or warping:
			set_cloak(false)
		else:
			energy -= cost
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
	cloak_alpha = lerpf(cloak_alpha, 0.12 if cloaked else 1.0, 4.0 * delta)
	if is_player and cloaked:
		cloak_alpha = maxf(cloak_alpha, 0.3)


func set_cloak(on: bool) -> void:
	if on == cloaked or not can_cloak:
		return
	if on and (energy < 80.0 or warping):
		return
	cloaked = on
	Sfx.play("cloak" if on else "uncloak", global_position, 0.6)


func _weapons(delta: float) -> void:
	beam_targets.clear()
	for w in weapons:
		if w["cooldown"] > 0.0:
			w["cooldown"] -= delta
		# turrets visibly swing toward where they would shoot
		if is_turret(w):
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
		if not want or warping:
			continue
		if systems.is_offline("w%d" % w["index"]):
			continue
		if w["kind"] == "beam":
			_fire_beam(w, delta)
		elif w["cooldown"] <= 0.0:
			_fire_tube(w)


func is_turret(w: Dictionary) -> bool:
	return w["arc1"] - w["arc0"] >= deg_to_rad(150.0)


func _aim_angle(w: Dictionary) -> float:
	# Fire toward the target if it is inside the arc, else straight down the arc centre.
	var centre: float = w["fire_angle"]
	# a turret under a pilot's hand points where the pilot points
	if aim_point != Vector2.INF and is_turret(w):
		var rel := wrapf((aim_point - global_position).angle() - rotation, -PI, PI)
		if w["arc1"] - w["arc0"] >= TAU - 0.2:
			return rel
		return clampf(rel, w["arc0"], w["arc1"])
	if target != null and is_instance_valid(target) and not (target is Ship and target.dead):
		var to := target.global_position - global_position
		var lead := to
		if target is Ship and w["kind"] == "tube":
			var md: Dictionary = GameData.MISSILES[w["missile"]]
			var mv: float = md["fire_v"] * GameData.LIGHT_SPEED
			var tt := to.length() / maxf(mv + 1.0, 1.0)
			lead = to + (target.velocity - velocity) * tt * 0.9
		var rel := wrapf(lead.angle() - rotation, -PI, PI)
		if rel >= w["arc0"] - 0.01 and rel <= w["arc1"] + 0.01:
			return rel
		if w["arc1"] - w["arc0"] >= TAU - 0.2:
			return rel
	return centre


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
	if cloaked:
		set_cloak(false)
	var md: Dictionary = GameData.MISSILES[w["missile"]]
	var rel := _aim_angle(w)
	if w["missile"] == "002":
		rel = PI   # mines drop behind
	var ang := rotation + rel
	var mount: Vector2 = global_position + Vector2.RIGHT.rotated(rotation + w["mount_angle"]) * w["mount_radius"]
	var dir := Vector2.RIGHT.rotated(ang)
	var v: Vector2 = velocity + dir * md["fire_v"] * GameData.LIGHT_SPEED
	if w["missile"] == "002":
		v = velocity * 0.6
	if Net.is_client():
		return   # the host fires it and tells everyone, us included
	world.spawn_missile(self, w["missile"], mount + dir * size * 0.3, v, ang, target)
	if is_player:
		world.last_player_fire = world.elapsed
	var snd := "torpedo"
	match w["missile"]:
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
	if cloaked:
		set_cloak(false)
	energy -= cost
	var rel := _aim_angle(w)
	var ang := rotation + rel
	var origin := global_position + Vector2.RIGHT.rotated(ang) * size * 0.6
	var hit: Dictionary = world.raycast_beam(self, origin, Vector2.RIGHT.rotated(ang), w["range"])
	var end: Vector2 = hit["point"]
	if hit["body"] != null and not Net.is_client():   # damage is the host's
		if hit["body"] is Ship:
			hit["body"].take_damage(w["dps"] * delta * systems.weapon_efficiency(w["index"]), self, end, 1.0, "", 0.1)
		else:
			hit["body"].take_damage(w["dps"] * delta * systems.weapon_efficiency(w["index"]), self, end)
		# a beam leans on what it burns
		world.push_node(hit["body"], Vector2.RIGHT.rotated(ang) * w["dps"] * delta * World.IMPULSE_PER_DAMAGE * 0.5, end)
	beam_targets.append([origin, end])
	# the beam itself is drawn by the world's Fx, so a laser is seen as well as felt
	if world != null and world.has_method("add_beam"):
		world.add_beam(origin, end, race_color())
	# bigger hulls hum lower
	Sfx.beam(self, clampf(1.3 - size / 80.0, 0.7, 1.2), int(str(w["id"]).right(1)))


# --------------------------------------------------------------- damage

## `at` is where the hit landed (Vector2.ZERO: nowhere in particular);
## `sys_mult` and `focus` say how hard it is on the ship's systems (see
## ShipSystems.hit).
func take_damage(amount: float, attacker: Node, at: Vector2 = Vector2.ZERO, sys_mult: float = 1.0, focus: String = "", crew_mult: float = 1.0) -> void:
	if dead or invulnerable > 0.0:
		return
	if attacker is Ship and attacker != self:
		last_attacker = attacker
		last_attacked_at = Time.get_ticks_msec() / 1000.0
		if is_player and autopilot and attacker.race_id != race_id:
			autopilot = false
			if warping:
				end_warp()
			target = attacker
			if world != null:
				world.notify(self, "Autopilot off: under fire from %s. Targeted." % attacker.pilot_name)
	if cloaked:
		set_cloak(false)
	hit_flash = 0.15
	if at != Vector2.ZERO:
		last_hit_angle = (at - global_position).angle() - rotation
	elif attacker is Node2D:
		last_hit_angle = (attacker.global_position - global_position).angle() - rotation
	var remaining := amount
	var bleed := 0.0
	if shield > 0.0:
		var absorbed := minf(shield, remaining)
		shield -= absorbed
		remaining -= absorbed
		bleed = absorbed * ShipSystems.SHIELD_BLEED
		if absorbed > 4.0:
			Sfx.play(Sfx.leveled("shield", module_levels.get("shield", 1)), global_position, 0.5, randf_range(0.92, 1.08))
	if remaining > 0.0:
		remaining = maxf(remaining - armor * 0.02 * remaining, remaining * 0.6)
		hull -= remaining
		_hit_systems(remaining + bleed, attacker, at, sys_mult, focus)
		if not Net.is_client() and not is_station:
			crew = maxf(crew - crew_max * remaining / maxf(hull_max, 1.0) * CREW_LOSS * crew_mult, 0.0)
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


func _die(killer: Node) -> void:
	if dead:
		return
	dead = true
	hull = 0.0
	deaths += 1
	boarding = null
	board_progress = 0.0
	boarded_by = null
	set_cloak(false)
	warping = false
	if engine_sound != null:
		engine_sound.silence()
	died.emit(self, killer)
	if body != null:
		body.enabled = false
	hide()
	set_physics_process(false)


func respawn(at: Vector2, vel: Vector2) -> void:
	dead = false
	captured = false
	refit(design)
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
	var level := int(module_id.substr(3, 1))
	var f: String = m["func"]
	var have: int = module_levels.get(f, 0)
	# the systems' whole numbers change; apply_systems derates them again
	var b := systems.base
	match f:
		"shield":
			if level <= have:
				return false
			var add: float = m["p"][0] - (b["shield_max"] if have > 0 else 0.0)
			b["shield_max"] = m["p"][0]
			b["shield_regen"] = b["shield_max"] * 0.06
			shield = minf(shield + maxf(add, 0.0), b["shield_max"])
		"eng":
			if level <= have:
				return false
			b["thrust_force"] *= 1.25
			fuel = fuel_max
		"sensor":
			if level <= have:
				return false
			b["sensor_range"] = 600.0 + m["p"][0] * 40.0
		"power":
			if level <= have:
				return false
			energy_max += m["power"] * 2.0
			b["energy_regen"] += m["power"] * 0.2
		"stealth":
			if b["can_cloak"]:
				return false
			b["can_cloak"] = true
		"boost":
			if b["has_boost"] and level <= have:
				return false
			b["has_boost"] = true
		"armor":
			if level <= have:
				return false
			armor = m["p"][0]
			hull_max += m["p"][0] * 2
			hull += m["p"][0] * 2
		_:
			return false
	apply_systems()
	module_levels[f] = level
	return true


# ---------------------------------------------------------------- drawing

## Smoke, fire and sparks where the systems are hurt: a thin wisp on a
## damaged one, flames and arcing on one that is out.
func _draw_system_damage() -> void:
	var a := cloak_alpha
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
	ShipArt.draw_model(self, art_model, base, thrust_level, cloak_alpha, dmg, Transform2D.IDENTITY, aims, clampf(hit_flash * 5.0, 0.0, 1.0), broken)
	if not broken.is_empty():
		_draw_system_damage()
	if is_disabled() and cloak_alpha > 0.3:
		# a dead ship's running light blinks red, so it can be found and boarded
		var blink := 0.5 + 0.5 * sin(Time.get_ticks_msec() / 1000.0 * 5.0)
		draw_arc(Vector2.ZERO, size * 1.25, 0, TAU, 40, Color(1.0, 0.25, 0.15, 0.25 + 0.3 * blink), 1.5, true)
	if boarding != null and is_instance_valid(boarding) and not boarding.dead:
		# the docking tractor holds the prize alongside
		var to := (boarding.global_position - global_position).rotated(-rotation)
		var c := Color(0.55, 0.9, 1.0, 0.35 + 0.25 * sin(Time.get_ticks_msec() / 90.0))
		draw_line(Vector2.ZERO, to, c, 3.0, true)
		draw_line(Vector2.ZERO, to, Color(1, 1, 1, 0.5), 1.0, true)
	# the shield is drawn by shield_bubble
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
