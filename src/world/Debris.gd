## Debris -- a tumbling rigid chunk: a piece of a wrecked hull, or an
## asteroid of the belt.  Both are PhysicsBody polygons in the world's
## space; wreckage fades after a while, rocks last and split when shot.
class_name Debris
extends Node2D

var world: Node
var body: PhysicsBody
var poly := PackedVector2Array()      # local, for drawing
var color := Color(0.4, 0.4, 0.45)
var line := Color(0.6, 0.6, 0.65)
var is_rock := false
var life := 25.0
var age := 0.0
var hp := 100.0
var rock_radius := 10.0
var seed_val := 0


func setup_rock(p_world: Node, pos: Vector2, vel: Vector2, r: float, rng_seed: int) -> void:
	world = p_world
	is_rock = true
	rock_radius = r
	seed_val = rng_seed
	var rng := RandomNumberGenerator.new()
	rng.seed = rng_seed
	var n := 7 + rng.randi() % 4
	poly = PackedVector2Array()
	for i in n:
		var a := TAU * i / n + rng.randf_range(-0.15, 0.15)
		poly.append(Vector2(cos(a), sin(a)) * r * rng.randf_range(0.7, 1.0))
	var shade := rng.randf_range(0.3, 0.5)
	color = Color(shade, shade * 0.95, shade * 0.9)
	line = color.lightened(0.3)
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
	z_index = -2


func setup_wreck(p_world: Node, chunk: PackedVector2Array, pos: Vector2, vel: Vector2, angle: float, ang_vel: float, chunk_mass: float, base: Color) -> void:
	world = p_world
	is_rock = false
	poly = chunk
	color = base.darkened(0.6)
	line = base.darkened(0.2)
	life = randf_range(18.0, 30.0)
	body = PhysicsBody.new()
	body.kind = PhysicsBody.Kind.DEBRIS
	body.node = self
	body.set_hull(chunk)
	body.set_mass(chunk_mass)
	body.restitution = 0.3
	body.friction = 0.4
	body.ang_damping = 0.08
	body.pos = pos
	body.vel = vel
	body.angle = angle
	body.ang_vel = ang_vel
	global_position = pos
	rotation = angle
	z_index = -1


func pre_step(_delta: float) -> void:
	body.pos = global_position
	body.angle = rotation


func post_step(delta: float) -> void:
	global_position = body.pos
	rotation = body.angle
	age += delta
	if not is_rock and age >= life:
		world.remove_debris(self)
		return
	queue_redraw()


## Weapons chip at rocks; wreckage just absorbs the hit.
func take_damage(amount: float, _attacker: Node, at: Vector2 = Vector2.ZERO) -> void:
	if not is_rock:
		return
	hp -= amount
	if world != null:
		world.spawn_sparks(at if at != Vector2.ZERO else global_position, 3, line)
	if hp <= 0.0:
		world.break_rock(self)


func _draw() -> void:
	var fade := 1.0
	if not is_rock:
		fade = clampf((life - age) / 5.0, 0.0, 1.0)
	var fc := color
	fc.a = fade
	var lc := line
	lc.a = fade
	draw_colored_polygon(poly, fc)
	var closed := PackedVector2Array(poly)
	closed.append(poly[0])
	draw_polyline(closed, lc, 1.5, true)
	if is_rock:
		# a few craters, fixed per rock
		var rng := RandomNumberGenerator.new()
		rng.seed = seed_val + 7
		for i in 3:
			var p := poly[rng.randi() % poly.size()] * rng.randf_range(0.2, 0.6)
			draw_circle(p, rock_radius * rng.randf_range(0.12, 0.25), color.darkened(0.35))
	else:
		# scorched edge
		draw_circle(poly[0] * 0.5, maxf(body.radius * 0.25, 2.0), Color(0.05, 0.03, 0.02, 0.7 * fade))
