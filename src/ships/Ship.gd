## Ship -- a flyable, fightable starship (or starbase) built from a design
## in GameData.  Flight is Newtonian: thrust adds velocity, nothing slows
## you down but your own engines, brakes and gravity.  Ships point along
## their +X axis.
class_name Ship
extends Node2D

signal died(ship: Ship, killer: Node)
signal damaged(ship: Ship, amount: float)
signal rank_changed(ship: Ship, level: int)

const BOOST_SPEED_MULT := 1.5
const BOOST_ACCEL_MULT := 2.2
const WARP_CHARGE_TIME := 1.1

var world: Node = null
var design_name := "Scout"
var design: Dictionary = {}
var race_id := 1
var pilot_name := "Pilot"
var ship_name := "Ship"
var is_player := false
var is_station := false

# flight
var velocity := Vector2.ZERO
var size := 22.0
var max_speed := 150.0
var accel := 120.0
var turn_speed := 180.0
var mass := 1.0
var thrust_level := 0.0            # for the engine glow, 0..1

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
var net_id := 0                    # online: the id the host and clients share
var is_puppet := false             # online: shown from the host's snapshots, not simulated
var docked := false                # in the shipyard: controls are off
var art_model: Dictionary = {}     # the sprite model, rebuilt on refit


func setup(p_world: Node, p_design: String, p_race: int, p_pilot: String, p_ship_name: String) -> void:
	world = p_world
	race_id = p_race
	pilot_name = p_pilot
	ship_name = p_ship_name
	refit(p_design)
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
	energy = energy_max
	shield = shield_max
	hull = hull_max
	_build_weapons()
	art_model = ShipArt.build_model(design, race_id, size)
	queue_redraw()


func _build_weapons() -> void:
	weapons.clear()
	has_turret = false
	for w in design["weapons"]:
		var m: Dictionary = GameData.MODULES[w[0]]
		# shots leave from the turret as drawn on the painted hull
		var spot := ShipArt.weapon_spot(design["name"], race_id, size, w)
		var entry := {
			"id": w[0], "name": m["name"], "kind": m["func"],
			"mount_angle": spot.angle(), "mount_radius": spot.length(),
			"fire_angle": deg_to_rad(float(w[3])),
			"arc0": deg_to_rad(float(w[4])), "arc1": deg_to_rad(float(w[5])),
			"cooldown": 0.0, "cost": 0.0, "ammo": -1, "missile": "", "slot": "primary",
			"range": 0.0,
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
	_fly(delta)
	_systems(delta)
	_weapons(delta)
	queue_redraw()


func _fly(delta: float) -> void:
	var fwd := forward()
	var boosting: bool = inputs["boost"] and has_boost and energy > 5.0 and inputs["thrust"] > 0.0
	var cap := max_speed * (BOOST_SPEED_MULT if boosting else 1.0)
	var a := accel * (BOOST_ACCEL_MULT if boosting else 1.0)
	if engine_sound != null:
		engine_sound.update(thrust_level, boosting, warping, delta)

	if warping:
		_warp(delta)
		return

	if autopilot and nav_target != null and is_instance_valid(nav_target):
		_autopilot(delta)

	var turn: float = clampf(inputs["turn"], -1.0, 1.0)
	rotation += deg_to_rad(turn_speed) * turn * delta
	thrust_level = lerpf(thrust_level, clampf(absf(inputs["thrust"]) + (0.5 if boosting else 0.0), 0.0, 1.0), 10.0 * delta)

	var acc := Vector2.ZERO
	var t: float = clampf(inputs["thrust"], -1.0, 1.0)
	if t > 0.0:
		acc += fwd * a * t
	elif t < 0.0:
		acc += fwd * a * 0.6 * t
	var s: float = clampf(inputs["strafe"], -1.0, 1.0)
	if s != 0.0:
		acc += fwd.orthogonal() * a * 0.7 * s
	if inputs["brake"] and velocity.length() > 1.0:
		var brake := velocity.normalized() * maxf(a * brake_power * 1.5, 170.0 * brake_power)
		if brake.length() * delta > velocity.length():
			velocity = Vector2.ZERO
		else:
			acc -= brake
	# engines cost energy
	if acc.length_squared() > 0.0:
		var drain := 30.0 * delta * (1.0 + (1.5 if boosting else 0.0))
		if energy < drain:
			acc *= 0.3
		energy = maxf(energy - drain, 0.0)
	# gravity
	if world != null:
		acc += world.gravity_at(global_position, mass)
	velocity += acc * delta
	# soft speed cap: engines cannot push past it, gravity can
	var sp := velocity.length()
	if sp > cap:
		var over := sp - cap
		var pull := minf(over, (a * 1.5 + 60.0) * delta)
		velocity = velocity.normalized() * (sp - pull)
	global_position += velocity * delta


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


func _warp(delta: float) -> void:
	var fwd := forward()
	if warp_charge < WARP_CHARGE_TIME:
		warp_charge += delta
		velocity = velocity.lerp(fwd * max_speed * 0.5, 2.0 * delta)
		global_position += velocity * delta
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
	global_position += velocity * delta
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
	if qdrv <= 0.0:
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
		if get_meta("detour_around", null) != around and world != null:
			set_meta("detour_around", around)
			world.notify(self, "Autopilot: routing around %s." % ("the Sun" if around.look == "Sun" else around.body_name))
		return
	if has_meta("detour_around"):
		remove_meta("detour_around")
	var desired := to.angle()
	var diff := wrapf(desired - rotation, -PI, PI)
	inputs["turn"] = clampf(diff * 3.0, -1.0, 1.0)
	var closing := velocity.dot(to.normalized())
	var stop_dist := closing * closing / maxf(2.0 * accel * 0.8, 1.0)
	warp_block = maxf(warp_block - delta, 0.0)
	if dist > 3500.0 and absf(diff) < 0.15 and qdrv > 0.0 and energy > 400.0 and warp_block <= 0.0 and not _body_in_path(to):
		start_warp()
		return
	if dist > nav_target.radius * 0.6 + 260.0 and stop_dist < dist - 200.0:
		inputs["thrust"] = 1.0 if absf(diff) < 0.6 else 0.0
		inputs["brake"] = false
	else:
		inputs["thrust"] = 0.0
		inputs["brake"] = speed() > 40.0
		if speed() <= 40.0 and dist < nav_target.radius * 0.6 + 300.0:
			autopilot = false
			inputs["brake"] = false
			if world != null:
				world.notify(self, "Arrived at %s." % nav_target.body_name)


func _systems(delta: float) -> void:
	energy = minf(energy + energy_regen * delta * 0.1, energy_max)
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
	w["cooldown"] = w["rate"]
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
		hit["body"].take_damage(w["dps"] * delta, self, end)
	beam_targets.append([origin, end])
	# bigger hulls hum lower
	Sfx.beam(self, clampf(1.3 - size / 80.0, 0.7, 1.2), int(str(w["id"]).right(1)))


# --------------------------------------------------------------- damage

func take_damage(amount: float, attacker: Node, at: Vector2 = Vector2.ZERO) -> void:
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
	if shield > 0.0:
		var absorbed := minf(shield, remaining)
		shield -= absorbed
		remaining -= absorbed
		if absorbed > 4.0:
			Sfx.play(Sfx.leveled("shield", module_levels.get("shield", 1)), global_position, 0.5, randf_range(0.92, 1.08))
	if remaining > 0.0:
		remaining = maxf(remaining - armor * 0.02 * remaining, remaining * 0.6)
		hull -= remaining
		if remaining > 4.0:
			var armor_level: int = module_levels.get("armor", 0)
			Sfx.play(Sfx.leveled("hit_armor", armor_level) if armor_level > 0 else "hit", global_position, 0.55, randf_range(0.85, 1.15))
			if world != null:
				world.spawn_sparks(at if at != Vector2.ZERO else global_position, 4, race_color())
	damaged.emit(self, amount)
	if hull <= 0.0:
		_die(attacker)


func _die(killer: Node) -> void:
	if dead:
		return
	dead = true
	hull = 0.0
	deaths += 1
	set_cloak(false)
	warping = false
	if engine_sound != null:
		engine_sound.silence()
	died.emit(self, killer)
	hide()
	set_physics_process(false)


func respawn(at: Vector2, vel: Vector2) -> void:
	dead = false
	refit(design)
	global_position = at
	velocity = vel
	rotation = randf() * TAU
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
	match f:
		"shield":
			if level <= have:
				return false
			var add: float = m["p"][0] - (shield_max if have > 0 else 0.0)
			shield_max = m["p"][0]
			shield_regen = shield_max * 0.06
			shield = minf(shield + maxf(add, 0.0), shield_max)
		"eng":
			if level <= have:
				return false
			accel *= 1.25
		"sensor":
			if level <= have:
				return false
			sensor_range = 600.0 + m["p"][0] * 40.0
		"power":
			if level <= have:
				return false
			energy_max += m["power"] * 2.0
			energy_regen += m["power"] * 0.2
		"stealth":
			if can_cloak:
				return false
			can_cloak = true
		"boost":
			if has_boost and level <= have:
				return false
			has_boost = true
		"armor":
			if level <= have:
				return false
			armor = m["p"][0]
			hull_max += m["p"][0] * 2
			hull += m["p"][0] * 2
		_:
			return false
	module_levels[f] = level
	return true


# ---------------------------------------------------------------- drawing

func _draw() -> void:
	if dead:
		return
	var base := race_color()
	var dmg := 1.0 - hull / maxf(hull_max, 1.0)
	var aims := []
	for w in weapons:
		aims.append(w.get("aim_vis", w["fire_angle"]))
	if art_model.is_empty():
		art_model = ShipArt.build_model(design, race_id, size)
	ShipArt.draw_model(self, art_model, base, thrust_level, cloak_alpha, dmg, Transform2D.IDENTITY, aims, clampf(hit_flash * 5.0, 0.0, 1.0))
	# shield ripple where the shot landed
	if hit_flash > 0.0 and shield > 0.0:
		var c := Color(0.5, 0.75, 1.0, hit_flash * 5.0 * cloak_alpha)
		var spread := 0.9 + (0.15 - hit_flash) * 6.0
		draw_arc(Vector2.ZERO, size * 1.35, last_hit_angle - spread, last_hit_angle + spread, 24, c, 3.5, true)
		var c2 := Color(0.7, 0.85, 1.0, hit_flash * 2.0 * cloak_alpha)
		draw_arc(Vector2.ZERO, size * 1.35, 0, TAU, 40, c2, 1.5, true)
	# turret arcs for the pilot's own ship
	if is_player and Settings.mouse_aim and aim_point != Vector2.INF:
		for w in weapons:
			if is_turret(w) and w["arc1"] - w["arc0"] < TAU - 0.2:
				draw_arc(Vector2.ZERO, size * 1.1, w["arc0"], w["arc1"], 24, Color(1, 1, 1, 0.12), 1.0, true)
	if invulnerable > 0.0:
		var c := Color(1, 1, 1, 0.25 + 0.25 * sin(invulnerable * 20.0))
		draw_arc(Vector2.ZERO, size * 1.5, 0, TAU, 40, c, 2.0, true)
	if warping and warp_charge < WARP_CHARGE_TIME:
		var c := Color(0.6, 0.8, 1.0, warp_charge / WARP_CHARGE_TIME)
		draw_arc(Vector2.ZERO, size * (2.6 - 1.4 * warp_charge / WARP_CHARGE_TIME), 0, TAU, 48, c, 2.5, true)
	elif warping:
		for i in 6:
			var c := Color(0.5, 0.7, 1.0, 0.3 - i * 0.045)
			draw_line(Vector2(-size * (1.0 + i * 0.9), randf_range(-size, size) * 0.6), Vector2(-size * (2.5 + i * 1.2), randf_range(-size, size) * 0.6), c, 2.0)
