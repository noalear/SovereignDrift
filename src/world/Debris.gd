## Debris -- an asteroid of the belt: a tumbling PhysicsBody polygon in
## the world's space that chips and splits when shot.  It is drawn as a
## lumpy, cratered rock lit by the Sun (rock.gdshader) on a rough outline
## around its collision polygon.  (Wrecked ships leave no rigid pieces;
## their break-up is a collision-free effect in Fx.)
class_name Debris
extends Node2D

var world: Node
var body: PhysicsBody
var poly := PackedVector2Array()      # local, for drawing
var color := Color(0.4, 0.4, 0.45)
var line := Color(0.6, 0.6, 0.65)
var is_rock := true
var age := 0.0
var hp := 100.0
var rock_radius := 10.0
var seed_val := 0
var albedo := Color(0.3, 0.29, 0.27)
var metal := 0.0
var flash := 0.0
var mat: ShaderMaterial
var mesh_points := PackedVector2Array()   # a fan: centre, outline, outer skirt
var mesh_uvs := PackedVector2Array()
var mesh_colors := PackedColorArray()
var mesh_index := PackedInt32Array()

static var _shader: Shader = null


func setup_rock(p_world: Node, pos: Vector2, vel: Vector2, r: float, rng_seed: int) -> void:
	world = p_world
	is_rock = true
	rock_radius = r
	seed_val = rng_seed
	var rng := RandomNumberGenerator.new()
	rng.seed = rng_seed
	var n := 7 + rng.randi() % 4
	# most rocks are potato-shaped rather than round
	var stretch := rng.randf_range(0.62, 1.0)
	var axis := rng.randf() * PI
	poly = PackedVector2Array()
	for i in n:
		var a := TAU * i / n + rng.randf_range(-0.15, 0.15)
		var v := Vector2(cos(a), sin(a)) * r * rng.randf_range(0.78, 1.0)
		poly.append((v.rotated(-axis) * Vector2(1.0, stretch)).rotated(axis))
	# what it is made of: mostly dark carbon-rich rock, some stony, a few metal
	var kind := rng.randf()
	metal = 0.0
	if kind < 0.68:
		var sh := rng.randf_range(0.26, 0.34)
		albedo = Color(sh, sh * 0.96, sh * 0.92)
	elif kind < 0.9:
		albedo = Color(rng.randf_range(0.46, 0.54), rng.randf_range(0.38, 0.43), rng.randf_range(0.3, 0.34))
	else:
		albedo = Color(0.36, 0.36, 0.38)
		metal = 1.0
	color = albedo
	line = albedo.lightened(0.35)
	_build_mesh(rng)
	hp = r * r * 1.5
	body = PhysicsBody.new()
	body.kind = PhysicsBody.Kind.ROCK
	body.node = self
	body.set_hull(poly)
	body.set_mass(r * r * 6.0)
	body.restitution = 0.35
	body.friction = 0.5
	body.belt_gravity = true
	body.pos = pos
	body.vel = vel
	body.angle = rng.randf() * TAU
	body.ang_vel = rng.randf_range(-0.6, 0.6)
	global_position = pos
	rotation = body.angle
	reset_physics_interpolation()
	z_index = -2


func pre_step(_delta: float) -> void:
	body.pos = global_position
	body.angle = rotation


func post_step(delta: float) -> void:
	global_position = body.pos
	rotation = body.angle
	age += delta


## Weapons chip at the rock until it splits.
func take_damage(amount: float, _attacker: Node, at: Vector2 = Vector2.ZERO) -> void:
	hp -= amount
	flash = 1.0
	if world != null:
		world.spawn_sparks(at if at != Vector2.ZERO else global_position, 3, line)
	if hp <= 0.0:
		world.break_rock(self)


## The drawn outline: the collision polygon's edges broken up into a rough
## silhouette, as a fan from the centre whose UV runs 0 -> 1 to the edge,
## with a thin transparent skirt outside it for a soft edge.
func _build_mesh(rng: RandomNumberGenerator) -> void:
	var outline := PackedVector2Array()
	var steps := 6
	for i in poly.size():
		var a := poly[i]
		var b := poly[(i + 1) % poly.size()]
		for k in steps:
			var t := float(k) / steps
			var p := a.lerp(b, t)
			# bumps and dents, larger in the middle of each edge
			var bump := sin(t * PI) * rng.randf_range(-0.07, 0.1) + rng.randf_range(-0.025, 0.025)
			outline.append(p * (1.0 + bump))
	mesh_points = PackedVector2Array([Vector2.ZERO])
	mesh_uvs = PackedVector2Array([Vector2.ZERO])
	mesh_colors = PackedColorArray([Color.WHITE])
	var m := outline.size()
	for p in outline:
		mesh_points.append(p)
		mesh_uvs.append(p / p.length())
		mesh_colors.append(Color.WHITE)
	for p in outline:
		mesh_points.append(p + p.normalized() * 0.7)
		mesh_uvs.append(p / p.length())
		mesh_colors.append(Color(1, 1, 1, 0))
	mesh_index = PackedInt32Array()
	for i in m:
		var j := (i + 1) % m
		mesh_index.append_array([0, 1 + i, 1 + j])
		mesh_index.append_array([1 + i, 1 + m + i, 1 + m + j])
		mesh_index.append_array([1 + i, 1 + m + j, 1 + j])
	if _shader == null:
		_shader = load("res://src/world/rock.gdshader")
	mat = ShaderMaterial.new()
	mat.shader = _shader
	mat.set_shader_parameter("seed", float(seed_val % 997) + 0.37)
	mat.set_shader_parameter("radius", rock_radius)
	mat.set_shader_parameter("albedo", Vector3(albedo.r, albedo.g, albedo.b))
	mat.set_shader_parameter("metal", metal)
	material = mat


## Pieces of a broken rock are made of the same stuff as the rock.
func match_material(other: Debris) -> void:
	albedo = other.albedo
	metal = other.metal
	color = other.color
	line = other.line
	mat.set_shader_parameter("albedo", Vector3(albedo.r, albedo.g, albedo.b))
	mat.set_shader_parameter("metal", metal)


func _process(delta: float) -> void:
	if mat == null:
		return
	# the Sun sits at the origin; light comes from it, in the rock's own space
	var to_sun := (-global_position).normalized().rotated(-rotation)
	mat.set_shader_parameter("light_dir", to_sun)
	if flash > 0.0:
		flash = maxf(flash - delta * 6.0, 0.0)
		mat.set_shader_parameter("flash", flash * 0.5)


func _draw() -> void:
	if mesh_index.is_empty():
		return
	RenderingServer.canvas_item_add_triangle_array(get_canvas_item(), mesh_index, mesh_points, mesh_colors, mesh_uvs)
