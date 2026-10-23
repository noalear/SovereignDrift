## Planet -- a star, planet, moon or asteroid.  Orbits its parent, pulls
## with gravity, and, when colonised, shoots at enemies and can be
## captured.
class_name Planet
extends Node2D

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
var habitable := 0
var velocity := Vector2.ZERO    # orbital velocity, for leading shots
var accel := Vector2.ZERO       # the rail's acceleration (centripetal, plus the parent's)
var body: PhysicsBody = null    # the world's solid, kinematic circle for this body

# colony
var owner_race := -1
var colony_design := ""
var population := 0
var shield := 0.0
var shield_max := 0.0
var hull := 0.0
var hull_max := 0.0
var weapons: Array = []
var defence_range := 0.0
var hit_flash := 0.0
var last_attacker: Node = null
var under_attack := 0.0
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


func set_colony(race: int, design: String, pop: int) -> void:
	owner_race = race
	colony_design = design
	population = pop
	weapons.clear()
	if design == "":
		shield_max = 0.0
		hull_max = 0.0
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


func is_colonised() -> bool:
	return owner_race >= 0 and colony_design != ""


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
		_defend(delta)
		if shield < shield_max and under_attack <= 0.0:
			shield = minf(shield + shield_max * 0.04 * delta, shield_max)
		if hull < hull_max and under_attack <= 0.0:
			hull = minf(hull + hull_max * 0.01 * delta, hull_max)
	queue_redraw()


func _defend(delta: float) -> void:
	var enemy: Ship = world.nearest_enemy_ship(global_position, owner_race, defence_range + radius)
	for w in weapons:
		w["cooldown"] -= delta
		if enemy == null:
			continue
		var to := enemy.global_position - global_position
		if w["kind"] == "tube":
			if w["cooldown"] > 0.0:
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
				hit["body"].take_damage(w["dps"] * delta, self, hit["point"])
			world.add_beam(origin, hit["point"], GameData.race(owner_race)["color"])
			if w["cooldown"] <= 0.0:
				w["cooldown"] = 0.25
				Sfx.play("beam", global_position, 0.3)


func take_damage(amount: float, attacker: Node, _at: Vector2 = Vector2.ZERO) -> void:
	if not is_colonised():
		return
	if attacker is Ship and attacker.race_id == owner_race:
		return
	hit_flash = 0.12
	under_attack = 8.0
	if attacker is Ship:
		last_attacker = attacker
	var remaining := amount
	if shield > 0.0:
		var absorbed := minf(shield, remaining)
		shield -= absorbed
		remaining -= absorbed
	if remaining > 0.0:
		hull -= remaining
	if hull <= 0.0:
		var winner: int = attacker.race_id if attacker is Ship else -1
		if winner >= 0:
			world.planet_captured(self, winner, attacker)


func surrender_to(race: int) -> void:
	owner_race = race
	hull = hull_max * 0.4
	shield = shield_max * 0.3
	population = int(population * 0.6)
	under_attack = 0.0
	# the new owners rebuild the defences in their own colours
	set_colony(race, colony_design, population)
	hull = hull_max * 0.4
	shield = shield_max * 0.3


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
