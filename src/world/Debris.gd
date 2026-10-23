## Debris -- an asteroid of the belt: a tumbling PhysicsBody polygon in
## the world's space that chips and splits when shot.  (Wrecked ships leave
## no rigid pieces; their break-up is a collision-free effect in Fx.)
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


func pre_step(_delta: float) -> void:
	body.pos = global_position
	body.angle = rotation


func post_step(delta: float) -> void:
	global_position = body.pos
	rotation = body.angle
	age += delta
	queue_redraw()


## Weapons chip at the rock until it splits.
func take_damage(amount: float, _attacker: Node, at: Vector2 = Vector2.ZERO) -> void:
	hp -= amount
	if world != null:
		world.spawn_sparks(at if at != Vector2.ZERO else global_position, 3, line)
	if hp <= 0.0:
		world.break_rock(self)


func _draw() -> void:
	draw_colored_polygon(poly, color)
	var closed := PackedVector2Array(poly)
	closed.append(poly[0])
	draw_polyline(closed, line, 1.5, true)
	# a few craters, fixed per rock
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_val + 7
	for i in 3:
		var p := poly[rng.randi() % poly.size()] * rng.randf_range(0.2, 0.6)
		draw_circle(p, rock_radius * rng.randf_range(0.12, 0.25), color.darkened(0.35))
