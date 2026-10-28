## Planet -- a star, planet, moon or asteroid.  Orbits its parent, pulls
## with gravity, and, when colonised, grows its people, shoots at enemies,
## and can be knocked out and invaded.  Ships land on bodies with a
## surface to take colonists on or put them down (see Ship.start_landing).
class_name Planet
extends Node2D

## Seconds a colony's defences stay down after its hull falls (bombardment
## keeps them down).
const SILENCE_TIME := 30.0
## The capture rule: false means a colony whose hull falls is only knocked
## out, and changes hands when colonists land and win; true brings back
## the old surrender on the spot.
const SURRENDER_ON_HULL := false
## Logistic growth toward the world's optimum, per second.
const GROWTH_RATE := 0.004
## Share of the people killed per point of damage once the defences are down.
const POP_KILL_PER_DAMAGE := 0.00004

const WHITE_TEX_SIZE := 4
static var white_tex: ImageTexture = null
static var planet_shader: Shader = null

var world: Node
var body_name := "Body"
var body_class := "Planet"
var parent_body: Planet = null
var orbit_radius := 0.0
var orbit_angle := 0.0
var orbit_speed := 0.0          # rad / s
var mass := 1.0
var radius := 40.0
var look := "Mercury"
var temperature := 300
var habitable := 0              # hydrosphere, percent (the original's hydpc): 100 is a gas giant
var velocity := Vector2.ZERO    # orbital velocity, for leading shots
var accel := Vector2.ZERO       # the rail's acceleration (centripetal, plus the parent's)
var body: PhysicsBody = null    # the world's solid, kinematic circle for this body
var richness := {}              # resource -> 0..5, what the world yields (Economy.richness)
var stock := {}                 # resource -> units piled up here (Economy)

# colony
var owner_race := -1
var colony_design := ""
var population := 0.0
var silenced := 0.0             # defences down for this many seconds
var _tier_pop := 0.0            # population when the defences were last fitted
var shield := 0.0
var shield_max := 0.0
var hull := 0.0
var hull_max := 0.0
var weapons: Array = []
var defence_range := 0.0
var hit_flash := 0.0
var last_attacker: Node = null
var under_attack := 0.0
# the ground war (the Resources update, phase 5; see Ground)
var buildings: Array = []       # GameData.BUILDINGS keys standing here
var build_q: Array = []         # [key, seconds left]: one building at a time
var garrison := {}              # unit -> how many are stationed here
var recruit_q: Array = []       # [unit, seconds left] at the barracks
var pshield := 0.0              # the planetary shield's charge
var pshield_down := 0.0         # seconds until a collapsed planetary shield can charge again
var battle := {}                # a landing being fought: race, units, militia, pilot, sabotage
var short := {}                 # resource -> true: upkeep the stockpile could not pay
var desert_t := 0.0             # seconds the troops have gone short
var flak_t := 0.0
var shield_fx: ShieldFx = null  # draws the planetary shield (the body's own drawing goes through its shader)
var material_: ShaderMaterial


static func _ensure_resources() -> void:
	if white_tex == null:
		var img := Image.create(WHITE_TEX_SIZE, WHITE_TEX_SIZE, false, Image.FORMAT_RGBA8)
		img.fill(Color.WHITE)
		white_tex = ImageTexture.create_from_image(img)
	if planet_shader == null:
		planet_shader = load("res://src/world/planet.gdshader")


func setup(p_world: Node, row: Array, parent: Planet, seed_value: int) -> void:
	world = p_world
	body_class = row[0]
	body_name = row[1]
	parent_body = parent
	mass = row[4]
	look = row[5]
	temperature = row[6]
	habitable = row[7]
	radius = float(row[8])
	richness = Economy.richness(look, body_class, temperature, habitable, mass, radius)
	stock = Economy.empty_stock()
	if body_class == "Moon":
		orbit_radius = float(row[2]) * 1.6 + (parent.radius if parent else 0.0)
	else:
		orbit_radius = float(row[2]) * GameData.UNIVERSE_RADIUS
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_value
	orbit_angle = rng.randf() * TAU
	if orbit_radius > 0.0:
		if body_class == "Moon":
			# Kepler: a moon's rail uses the GM ships feel from its planet, so
			# a ship matching the moon's velocity really is in that orbit
			var parent_gm := PhysicsSpace.gm_for(parent.look, parent.body_class, parent.radius)
			orbit_speed = sqrt(parent_gm / (orbit_radius * orbit_radius * orbit_radius))
		else:
			# slow, real-feeling periods: inner planets a few minutes
			orbit_speed = TAU / (240.0 + orbit_radius * 0.12)
	_ensure_resources()
	material_ = ShaderMaterial.new()
	material_.shader = planet_shader
	_style(rng)
	material = material_
	z_index = -5
	shield_fx = ShieldFx.new()
	shield_fx.planet = self
	add_child(shield_fx)
	update_orbit(0.0)


func _style(rng: RandomNumberGenerator) -> void:
	var kind := 0
	var a := Color(0.6, 0.45, 0.35)
	var b := Color(0.35, 0.25, 0.2)
	var c := Color(0.5, 0.5, 0.6)
	var atmo := 0.0
	match look:
		"Sun":
			kind = 4; a = Color(1.0, 0.85, 0.5); b = Color(1.0, 0.45, 0.1); c = Color(1, 1, 1)
		"Mercury":
			kind = 6; a = Color(0.55, 0.52, 0.5); b = Color(0.3, 0.28, 0.27); c = Color(0.6, 0.6, 0.6)
		"Venus":
			kind = 0; a = Color(0.9, 0.78, 0.5); b = Color(0.7, 0.55, 0.3); c = Color(0.95, 0.85, 0.6); atmo = 0.9
		"Earth":
			kind = 3; a = Color(0.25, 0.55, 0.2); b = Color(0.55, 0.45, 0.3); c = Color(0.1, 0.3, 0.75); atmo = 0.8
		"Luna":
			kind = 6; a = Color(0.6, 0.6, 0.6); b = Color(0.35, 0.35, 0.37); c = Color(0.7, 0.7, 0.7)
		"Mars":
			kind = 0; a = Color(0.8, 0.4, 0.2); b = Color(0.5, 0.22, 0.12); c = Color(0.9, 0.6, 0.4); atmo = 0.25
		"Aster":
			kind = 5; a = Color(0.45, 0.42, 0.4); b = Color(0.25, 0.23, 0.22); c = Color(0.5, 0.5, 0.5)
		"Jupiter":
			kind = 1; a = Color(0.85, 0.7, 0.55); b = Color(0.6, 0.4, 0.3); c = Color(0.9, 0.45, 0.3); atmo = 0.4
		"Callisto":
			kind = 6; a = Color(0.4, 0.38, 0.35); b = Color(0.2, 0.2, 0.2); c = Color(0.5, 0.5, 0.5)
		"Europa":
			kind = 2; a = Color(0.85, 0.85, 0.8); b = Color(0.6, 0.5, 0.45); c = Color(0.8, 0.9, 1.0)
		"Ganymede":
			kind = 6; a = Color(0.55, 0.5, 0.45); b = Color(0.3, 0.3, 0.3); c = Color(0.6, 0.6, 0.6)
		"Io":
			kind = 0; a = Color(0.9, 0.85, 0.4); b = Color(0.7, 0.4, 0.15); c = Color(0.3, 0.3, 0.3)
		"Saturn":
			kind = 1; a = Color(0.9, 0.82, 0.6); b = Color(0.75, 0.62, 0.42); c = Color(0.95, 0.9, 0.75); atmo = 0.4
		"Titan":
			kind = 0; a = Color(0.85, 0.6, 0.25); b = Color(0.6, 0.4, 0.15); c = Color(0.9, 0.7, 0.4); atmo = 0.9
		"Rhea":
			kind = 2; a = Color(0.75, 0.75, 0.75); b = Color(0.5, 0.5, 0.5); c = Color(0.8, 0.8, 0.85)
		"Uranus":
			kind = 1; a = Color(0.6, 0.85, 0.9); b = Color(0.5, 0.75, 0.85); c = Color(0.75, 0.95, 1.0); atmo = 0.5
		"Neptune":
			kind = 1; a = Color(0.25, 0.4, 0.9); b = Color(0.15, 0.25, 0.7); c = Color(0.5, 0.6, 1.0); atmo = 0.5
		"Triton":
			kind = 2; a = Color(0.8, 0.75, 0.7); b = Color(0.55, 0.5, 0.5); c = Color(0.9, 0.85, 0.8)
		"Pluto":
			kind = 2; a = Color(0.75, 0.65, 0.55); b = Color(0.45, 0.4, 0.4); c = Color(0.8, 0.8, 0.8)
	material_.set_shader_parameter("kind", kind)
	material_.set_shader_parameter("color_a", a)
	material_.set_shader_parameter("color_b", b)
	material_.set_shader_parameter("color_c", c)
	material_.set_shader_parameter("atmosphere", atmo)
	material_.set_shader_parameter("extent", quad_extent())
	material_.set_shader_parameter("seed", rng.randf() * 50.0)
	material_.set_shader_parameter("spin", rng.randf_range(0.01, 0.04) * (1.0 if rng.randf() < 0.7 else -1.0))


## Half the shader quad in body radii: the Sun's corona and planets'
## atmospheres need room to fade out before the quad's edge.
func quad_extent() -> float:
	return 2.6 if look == "Sun" else 1.5


## How far out the drawn body reaches: the shader paints planets a little
## larger than their physical radius, and a colony ring sits outside that.
func drawn_radius() -> float:
	if look == "Sun":
		return radius
	return maxf(radius * 1.19, radius + 12.0 if is_colonised() else 0.0)


## Solid ground to land on: not the Sun, not a gas giant.
func has_surface() -> bool:
	return look != "Sun" and habitable < 100


## How many of a race this world can support: the original's size and
## hydrosphere formula, times how well the race takes the climate.
func optimal_population(race_id: int) -> float:
	if not has_surface():
		return 0.0
	var size := radius * radius * radius * (100.0 - habitable) * (4.0 if habitable > 0 else 1.0)
	return size * 134.0 * GameData.climate(race_id, temperature)


static func design_for(pop: float) -> String:
	if pop >= GameData.MAJOR_COLONY:
		return "Major"
	if pop >= GameData.MINOR_COLONY:
		return "Minor"
	return "Outpost"


static func _tier(design: String) -> int:
	return ["Outpost", "Minor", "Major"].find(design)


func set_colony(race: int, design: String, pop: float) -> void:
	owner_race = race
	colony_design = design
	population = pop
	_tier_pop = pop
	silenced = 0.0
	weapons.clear()
	if design == "":
		shield_max = 0.0
		hull_max = 0.0
		clear_ground()
		return
	var cd: Dictionary = GameData.COLONY_DESIGNS[design]
	shield_max = GameData.MODULES[cd["shield"]]["p"][0] * 3.0
	hull_max = GameData.MODULES[cd["armor"]]["hp"] * 6.0 + 1500.0
	shield = shield_max
	hull = hull_max
	defence_range = cd["range"] * 0.7
	for w in cd["weapons"]:
		var m: Dictionary = GameData.MODULES[w[0]]
		var e := {"id": w[0], "kind": m["func"], "cooldown": randf(), "missile": ""}
		if m["func"] == "tube":
			e["missile"] = m["p"][3]
			e["rate"] = GameData.TUBE_COOLDOWN.get(e["missile"], 0.4) * 1.6
		else:
			e["range"] = float(m["p"][2]) * 2.2
			e["dps"] = 30.0 + float(m["p"][1]) * 0.1
			e["rate"] = 0.0
		weapons.append(e)
	fit_batteries()


## The batteries' guns join the colony's own (and leave when a battery is
## blown up).
func fit_batteries() -> void:
	weapons = weapons.filter(func(w): return not w.get("battery", false))
	if not is_colonised():
		return
	if buildings.has("laser"):
		weapons.append({"id": "4503", "kind": "beam", "range": Ground.LASER_RANGE, "dps": Ground.LASER_DPS, "rate": 0.0, "cooldown": 0.0, "missile": "", "battery": true})
	if buildings.has("silo"):
		weapons.append({"id": "4008", "kind": "tube", "missile": "008", "rate": Ground.SILO_RATE, "cooldown": randf(), "reach": Ground.SILO_RANGE, "battery": true})


## An empty world has nothing on the ground.
func clear_ground() -> void:
	buildings = []
	build_q = []
	garrison = {}
	recruit_q = []
	pshield = 0.0
	pshield_down = 0.0
	battle = {}
	short = {}
	desert_t = 0.0
	weapons = weapons.filter(func(w): return not w.get("battery", false))


func pshield_max() -> float:
	return Ground.PSHIELD_HP if buildings.has("shield") else 0.0


## The planetary shield stands: nothing lands and nobody dies.
func pshield_up() -> bool:
	return buildings.has("shield") and pshield > 0.0 and pshield_down <= 0.0


func is_colonised() -> bool:
	return owner_race >= 0 and colony_design != ""


## The colony died out or was wiped out: an empty world again.
func clear_colony() -> void:
	set_colony(-1, "", 0.0)


## Settlers found a colony here.
func found_colony(race: int, pop: float) -> void:
	set_colony(race, design_for(pop), pop)
	hull = hull_max * 0.5
	shield = shield_max * 0.5


## Defences down: the guns are silent and the shields are out.
func is_defenceless() -> bool:
	return is_colonised() and (silenced > 0.0 or hull <= 0.0)


## A growing colony earns better defences; the hull keeps its share.
func _check_tier() -> void:
	var want := design_for(population)
	if _tier(want) <= _tier(colony_design) or _tier(want) <= _tier(design_for(_tier_pop)):
		return
	var hf := hull / maxf(hull_max, 1.0)
	var sf := shield / maxf(shield_max, 1.0)
	var keep_silence := silenced
	set_colony(owner_race, want, population)
	hull = hull_max * hf
	shield = shield_max * sf
	silenced = keep_silence


## The host grows the people toward the optimum (never past it).
func _grow(delta: float) -> void:
	var opt := optimal_population(owner_race)
	if opt <= 0.0 or population >= opt or silenced > 0.0:
		return
	population = minf(population + GROWTH_RATE * population * (1.0 - population / opt) * delta, opt)


func update_orbit(delta: float) -> void:
	orbit_angle += orbit_speed * delta
	var centre := parent_body.global_position if parent_body != null else Vector2.ZERO
	var old := global_position
	global_position = centre + Vector2(cos(orbit_angle), sin(orbit_angle)) * orbit_radius
	velocity = (global_position - old) / maxf(delta, 0.0001) if delta > 0.0 else Vector2.ZERO
	if parent_body != null:
		velocity = parent_body.velocity + Vector2(-sin(orbit_angle), cos(orbit_angle)) * orbit_speed * orbit_radius
		accel = parent_body.accel - (global_position - centre) * orbit_speed * orbit_speed
	elif orbit_radius > 0.0:
		velocity = Vector2(-sin(orbit_angle), cos(orbit_angle)) * orbit_speed * orbit_radius
		accel = -(global_position - centre) * orbit_speed * orbit_speed


## Hand the rail's state to the body the physics space collides with.
func sync_body() -> void:
	if body != null:
		body.pos = global_position
		body.vel = velocity
		body.accel = accel


func _process(delta: float) -> void:
	if hit_flash > 0.0:
		hit_flash -= delta
	if under_attack > 0.0:
		under_attack -= delta
	if material_ != null:
		var l := (-global_position).normalized() if look != "Sun" else Vector2.RIGHT
		material_.set_shader_parameter("light_dir", Vector2(l.x, -l.y))
	if is_colonised():
		if silenced > 0.0:
			silenced = maxf(silenced - delta, 0.0)
		if not Net.is_client():
			_grow(delta)
			_check_tier()
		_defend(delta)
		if shield < shield_max and under_attack <= 0.0 and silenced <= 0.0:
			shield = minf(shield + shield_max * 0.04 * delta, shield_max)
		if hull < hull_max and under_attack <= 0.0:
			hull = minf(hull + hull_max * 0.01 * delta, hull_max)
		_charge_pshield(delta)
	if shield_fx != null:
		shield_fx.visible = is_colonised() and pshield_up()
		if shield_fx.visible:
			shield_fx.queue_redraw()
	queue_redraw()


## The planetary shield: down for a while after it collapses, fading
## without Fuel, charging while nobody shoots and the defences stand.
func _charge_pshield(delta: float) -> void:
	var top := pshield_max()
	if top <= 0.0:
		return
	if pshield_down > 0.0:
		pshield_down = maxf(pshield_down - delta, 0.0)
	elif short.has("fuel"):
		pshield = maxf(pshield - top * Ground.PSHIELD_FADE * delta, 0.0)
	elif under_attack <= 0.0 and silenced <= 0.0:
		pshield = minf(pshield + top * Ground.PSHIELD_CHARGE * delta, top)


func _defend(delta: float) -> void:
	if "practice" in world and (world.practice or world.duel_rules != null):
		return                          # practice and duels: the colonies hold their fire
	if is_defenceless():
		return                          # knocked out: the guns are silent
	var reach := defence_range
	for w in weapons:
		reach = maxf(reach, w.get("reach", w.get("range", 0.0)))
	var enemy: Ship = world.nearest_enemy_ship(global_position, owner_race, reach + radius)
	for w in weapons:
		w["cooldown"] -= delta
		if enemy == null:
			continue
		var to := enemy.global_position - global_position
		if w["kind"] == "tube":
			if w["cooldown"] > 0.0 or to.length() > w.get("reach", defence_range) + radius:
				continue
			w["cooldown"] = w["rate"]
			var md: Dictionary = GameData.MISSILES[w["missile"]]
			var mv: float = md["fire_v"] * GameData.LIGHT_SPEED
			var tt := to.length() / maxf(mv, 1.0)
			var aim := to + (enemy.velocity - velocity) * tt * 0.85
			var dir := aim.normalized()
			world.spawn_missile(self, w["missile"], global_position + dir * (radius + 12.0), velocity + dir * mv, dir.angle(), enemy)
			Sfx.play("grenade" if w["missile"] == "004" else "torpedo", global_position, 0.5)
		else:
			if to.length() > w["range"] + radius:
				continue
			var origin := global_position + to.normalized() * radius
			var hit: Dictionary = world.raycast_beam(self, origin, to.normalized(), w["range"])
			if hit["body"] != null:
				if hit["body"] is Ship:
					hit["body"].take_damage(w["dps"] * delta, self, hit["point"], 1.0, "", 0.1, "beam")
				else:
					hit["body"].take_damage(w["dps"] * delta, self, hit["point"], "beam")
			world.add_beam(origin, hit["point"], GameData.race(owner_race)["color"])
			if w["cooldown"] <= 0.0:
				w["cooldown"] = 0.25
				Sfx.play("beam", global_position, 0.3)


func take_damage(amount: float, attacker: Node, _at: Vector2 = Vector2.ZERO, dtype: String = "kinetic") -> void:
	if not is_colonised():
		return
	if attacker is Ship and attacker.race_id == owner_race:
		return
	hit_flash = 0.12
	under_attack = 8.0
	if attacker is Ship:
		last_attacker = attacker
	var remaining := amount
	var ms := maxf(GameData.damage_mult(dtype, "shield"), 0.001)
	if pshield_up():
		# the planetary shield takes it all until it collapses
		pshield -= remaining * ms
		if pshield > 0.0:
			return
		pshield = 0.0
		pshield_down = Ground.PSHIELD_DOWN
		if world.has_method("planetary_shield_down"):
			world.planetary_shield_down(self, attacker)
		return
	if shield > 0.0:
		var absorbed := minf(shield, remaining * ms)
		shield -= absorbed
		remaining -= absorbed / ms
	remaining *= GameData.damage_mult(dtype, "hull")
	if remaining <= 0.0:
		return
	var winner: int = attacker.race_id if attacker is Ship else -1
	if hull > 0.0:
		hull -= remaining
		if hull <= 0.0:
			hull = 0.0
			if SURRENDER_ON_HULL:
				if winner >= 0:
					world.planet_captured(self, winner, attacker)
			else:
				silenced = SILENCE_TIME
				shield = 0.0
				if world.has_method("colony_knocked_out"):
					world.colony_knocked_out(self, attacker)
		return
	# defences down: the shots fall on the cities, and keep the guns quiet
	silenced = SILENCE_TIME
	if Net.is_client():
		return
	population -= maxf(population * POP_KILL_PER_DAMAGE * remaining, 2000.0 * remaining)
	if population < GameData.MIN_COLONY and world.has_method("colony_wiped_out"):
		world.colony_wiped_out(self, attacker)


## New owners: by invasion (`pop` is what is left of the invaders) or by
## the old surrender rule (the people stay, fewer of them).
func surrender_to(race: int, pop: float = -1.0) -> void:
	var people := pop if pop >= 0.0 else population * 0.6
	under_attack = 0.0
	# the new owners rebuild the defences in their own colours
	set_colony(race, design_for(people) if pop >= 0.0 else colony_design, people)
	hull = hull_max * 0.4
	shield = shield_max * 0.3


## What a pilot sees on the world's info card (shift-click it, or point at
## it on the system map): [text, kind] rows, kind one of "title", "sub",
## "line", "good" or "warn".  viewer is the race reading the card.
func info_lines(viewer: int) -> Array:
	var rows := [[body_name, "title"]]
	var kind: String = {"Minor": "Asteroid", "Star": "Star"}.get(body_class, body_class)
	if parent_body != null and body_class == "Moon":
		kind = "Moon of %s" % parent_body.body_name
	elif habitable >= 100:
		kind = "Gas giant"
	var mass_text := "under 0.01" if mass < 0.01 else (("%.2f" % mass) if mass < 10.0 else str(int(mass)))
	rows.append(["%s   %d K   %smass %s Earths" % [kind, temperature, ("water %d%%   " % habitable) if habitable < 100 else "", mass_text], "sub"])
	if look == "Sun":
		rows.append(["Yields  %s   (a solar forge station only)" % Economy.yields_text(richness), "line"])
		return rows
	if not has_surface():
		rows.append(["No surface to land on or settle", "line"])
		rows.append(["Yields  %s   (a skimmer station only)" % Economy.yields_text(richness), "line"])
		return rows
	rows.append(["Yields  %s" % Economy.yields_text(richness), "line"])
	var viewer_name: String = GameData.race(viewer)["name"]
	if not is_colonised():
		rows.append(["Uninhabited", "line"])
		var opt_v := optimal_population(viewer)
		rows.append(["Supports %s %ss (climate %d%%)" % [GameData.fmt_pop(opt_v), viewer_name, int(GameData.climate(viewer, temperature) * 100.0)], "line"])
		rows.append(["Land colonists to settle it", "good"])
		return rows
	var owner: Dictionary = GameData.race(owner_race)
	var mine := owner_race == viewer
	rows.append(["%s %s colony%s" % [owner["adj"], colony_design if colony_design != "Outpost" else "outpost", "  (yours)" if mine else ""], "good" if mine else "line"])
	var opt := optimal_population(owner_race)
	rows.append(["People  %s   supports %s" % [GameData.fmt_pop(population), GameData.fmt_pop(opt)], "line"])
	if silenced > 0.0:
		rows.append(["Not growing while its defences are down", "warn"])
	elif opt > 0.0 and population < opt * 0.995:
		var per_min := GROWTH_RATE * population * (1.0 - population / opt) * 60.0
		rows.append(["Growing  +%s a minute" % GameData.fmt_pop(per_min), "line"])
	elif population > opt * 1.005:
		rows.append(["Overcrowded: more people than it supports, not growing", "warn"])
	else:
		rows.append(["Full: as many people as it can support", "line"])
	if is_defenceless():
		rows.append(["DEFENCES DOWN%s" % ((" for %d s" % int(ceil(silenced))) if silenced > 0.0 else ""), "warn"])
	else:
		rows.append(["Defences  %d gun%s, range %d   shields %d%%   hull %d%%" % [weapons.size(), "" if weapons.size() == 1 else "s", int(defence_range), int(100.0 * shield / maxf(shield_max, 1.0)), int(100.0 * hull / maxf(hull_max, 1.0))], "line"])
	rows.append_array(ground_lines(viewer))
	rows.append_array(stock_lines(viewer))
	if owner_race <= 2:
		var mult: float = world.round_rules.bonus_multiplier() if world != null and world.round_rules != null else 1.0
		rows.append(["Pays %s %d credits a minute" % ["your side" if mine else "its owners", int(10 * mult)], "line"])
	if mine:
		var spare: float = world.spare_colonists(self) if world != null else 0.0
		rows.append(["%s colonists to spare: land and press %s to take them on" % [GameData.fmt_pop(spare), Keybinds.short_text("colonists_board")], "good"])
		rows.append(["Land and press %s to build, recruit and take troops aboard" % Keybinds.short_text("cargo"), "good"])
	elif pshield_up():
		rows.append(["Bring its planetary shield down, then land troops", "line"])
	elif is_defenceless():
		rows.append(["Land troops to take it", "warn"])
	else:
		rows.append(["Knock out its defences, then land troops", "line"])
	if not mine and Ground.count(garrison) == 0 and battle.is_empty():
		rows.append(["(or over %s colonists: 20 to 1 once its defences are down)" % GameData.fmt_pop(population * GameData.COLONIST_RATIO), "line"])
	if not mine:
		rows.append(["Would support %s %ss (climate %d%%)" % [GameData.fmt_pop(optimal_population(viewer)), viewer_name, int(GameData.climate(viewer, temperature) * 100.0)], "line"])
	return rows


## The planetary shield, the buildings, the garrison and any battle.
func ground_lines(viewer: int) -> Array:
	var rows := []
	var mine := owner_race == viewer
	var top := pshield_max()
	if top > 0.0:
		var pct := int(100.0 * pshield / top)
		if pshield_down > 0.0:
			rows.append(["PLANETARY SHIELD DOWN for %d s" % int(ceil(pshield_down)), "warn"])
		elif short.has("fuel"):
			rows.append(["Planetary shield %d%%, fading: no Fuel" % pct, "warn"])
		else:
			rows.append(["Planetary shield %d%%" % pct, "line" if pshield > 0.0 else "warn"])
	if not buildings.is_empty() or not build_q.is_empty():
		var names := []
		for key in buildings:
			names.append(GameData.BUILDINGS[key]["label"])
		var line := "Buildings  %s  (%d of %d slots)" % [", ".join(names) if not names.is_empty() else "none yet", buildings.size(), Ground.slots(self)]
		rows.append([line, "line"])
		if mine and not build_q.is_empty():
			rows.append(["Building the %s: %d s" % [GameData.BUILDINGS[build_q[0][0]]["label"].to_lower(), int(ceil(build_q[0][1]))], "good"])
	rows.append(["Garrison  %s   militia %.1f" % [Ground.units_text(garrison), Ground.militia(self)], "line"])
	if mine and not recruit_q.is_empty():
		rows.append(["Training  %d unit%s, the next in %d s" % [recruit_q.size(), "" if recruit_q.size() == 1 else "s", int(ceil(recruit_q[0][1]))], "good"])
	if mine and (short.has("fuel") or short.has("food")):
		rows.append(["Short of %s: troops desert, the shield fades" % " and ".join(short.keys().map(func(k): return Economy.NAMES[k])), "warn"])
	if not battle.is_empty():
		var a := Ground.strength(battle["units"])
		var d := Ground.defence(self)
		var est := Ground.estimate(a, 0.0, d, Ground.bunker_factor(self))
		var race := int(battle["race"])
		rows.append(["GROUND BATTLE  %s %.1f against %.1f   %s" % [GameData.race(race)["adj"], a, d,
			("about %d s" % int(ceil(est))) if est < INF else "the landing is losing"], "warn"])
		rows.append(["", "bar", a / maxf(a + d, 0.01), GameData.race(race)["color"], GameData.race(owner_race)["color"]])
	return rows


## The colony's stockpile and output, for its own side; others only see
## that there is one.
func stock_lines(viewer: int) -> Array:
	if not is_colonised():
		return []
	if owner_race != viewer:
		return [["Stockpile: unknown", "line"]]
	var have := []
	for k in Economy.RESOURCES:
		if stock.get(k, 0.0) >= 0.5:
			have.append("%s %d" % [Economy.NAMES[k], int(stock[k])])
	var rows := [["Stock  %s   (holds %d of each)" % ["  ".join(have) if not have.is_empty() else "empty", int(Economy.cap(self))], "line"]]
	var rates := Economy.production(self)
	var making := []
	for k in Economy.RESOURCES:
		if rates[k] >= 0.05:
			var full: bool = stock.get(k, 0.0) >= Economy.cap(self) - 0.01
			making.append("%s +%s%s" % [Economy.NAMES[k], ("%.1f" % rates[k]) if rates[k] < 10.0 else str(int(rates[k])), " (full)" if full else ""])
	if silenced > 0.0:
		rows.append(["Making nothing while its defences are down", "warn"])
	elif not making.is_empty():
		rows.append(["Making  %s a minute" % "  ".join(making), "line"])
	return rows


func _draw() -> void:
	# planetary rings for Saturn and Uranus
	if look == "Saturn" or look == "Uranus":
		var rc := Color(0.85, 0.8, 0.65, 0.45) if look == "Saturn" else Color(0.7, 0.85, 0.9, 0.3)
		for i in 5:
			draw_arc(Vector2.ZERO, radius * (1.35 + i * 0.09), 0, TAU, 96, rc, radius * 0.06, true)
	var ext := quad_extent()
	draw_texture_rect(white_tex, Rect2(-radius * ext, -radius * ext, radius * ext * 2.0, radius * ext * 2.0), false)
	if is_colonised():
		var rc: Color = GameData.race(owner_race)["color"]
		if hit_flash > 0.0:
			rc = rc.lerp(Color.WHITE, 0.6)
		rc.a = 0.9
		if is_defenceless():
			# knocked out: the ring breaks up and flickers
			var flick := 0.35 + 0.25 * sin(Time.get_ticks_msec() / 90.0)
			var dc := Color(rc.r, rc.g, rc.b, flick)
			for i in 12:
				var a0 := TAU * i / 12.0
				draw_arc(Vector2.ZERO, radius + 10.0, a0, a0 + TAU / 24.0, 8, dc, 2.5, true)
		else:
			draw_arc(Vector2.ZERO, radius + 10.0, 0, TAU, 64, rc, 2.5, true)
		# colony lights
		var dots := 3 if colony_design == "Outpost" else (5 if colony_design == "Minor" else 8)
		for i in dots:
			var a := TAU * i / dots + 0.3
			draw_circle(Vector2(cos(a), sin(a)) * radius * 0.75, maxf(radius * 0.05, 2.0), rc.lerp(Color.WHITE, 0.4))
		if shield > 0.0 and hit_flash > 0.0:
			var sc := rc
			sc.a = hit_flash * 4.0
			draw_arc(Vector2.ZERO, radius + 16.0, 0, TAU, 64, sc, 4.0, true)



## The planetary shield: a faint bubble over the world that flares where
## it is hit.  A child node, so it is drawn plainly, not by the planet's
## shader.
class ShieldFx extends Node2D:
	var planet: Planet

	func _draw() -> void:
		var rc: Color = GameData.race(planet.owner_race)["color"].lerp(Color(0.7, 0.9, 1.0), 0.55)
		var r := planet.radius * 1.28 + 22.0
		var charge := planet.pshield / maxf(planet.pshield_max(), 1.0)
		var flash := clampf(planet.hit_flash * 6.0, 0.0, 1.0)
		draw_circle(Vector2.ZERO, r, Color(rc.r, rc.g, rc.b, 0.05 + 0.12 * flash))
		draw_arc(Vector2.ZERO, r, 0, TAU, 96, Color(rc.r, rc.g, rc.b, 0.25 + 0.3 * charge + 0.4 * flash), 2.5, true)
		draw_arc(Vector2.ZERO, r - 5.0, 0, TAU, 96, Color(rc.r, rc.g, rc.b, 0.08 + 0.2 * flash), 6.0, true)
