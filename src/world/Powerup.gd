## Powerup -- a ship module floating in space after an explosion, or made
## by a friendly colony.  Any ship that flies into it takes it aboard if it
## is an upgrade.
class_name Powerup
extends Node2D

var world: Node = null
var body: PhysicsBody = null
var module_id := "5002"
var velocity := Vector2.ZERO
var age := 0.0
var life := 40.0
var label := ""


## kind: "part" (a module id), "blueprint" ("bp:" + module id) or
## "credits" ("cr:" + amount).
var kind := "part"
var amount := 0


func setup(id: String, pos: Vector2, vel: Vector2) -> void:
	module_id = id
	global_position = pos
	velocity = vel
	rotation = randf() * TAU
	if id.begins_with("bp:"):
		kind = "blueprint"
		module_id = id.substr(3)
		label = "%s blueprint" % GameData.MODULES[module_id]["name"]
	elif id.begins_with("cr:"):
		kind = "credits"
		amount = int(id.substr(3))
		label = "%d credits" % amount
		life = 25.0
	else:
		label = GameData.MODULES[id]["name"]
	body = PhysicsBody.new()
	body.kind = PhysicsBody.Kind.PICKUP
	body.node = self
	body.set_circle(8.0)
	body.set_mass(40.0)
	body.gravity_scale = 0.3
	body.sun_gravity = false
	body.pos = pos
	body.vel = vel


func pre_step(_delta: float) -> void:
	body.pos = global_position
	body.vel = velocity


func post_step(delta: float) -> void:
	global_position = body.pos
	velocity = body.vel
	age += delta
	velocity *= 1.0 - 0.4 * delta
	rotation += 0.8 * delta
	if age > life:
		if world != null:
			world.release_body(body)
		queue_free()
	queue_redraw()


func _draw() -> void:
	var fade := clampf((life - age) / 5.0, 0.0, 1.0)
	match kind:
		"credits":
			var g := Color(1.0, 0.85, 0.3, fade)
			var d := PackedVector2Array([Vector2(0, -8), Vector2(7, 0), Vector2(0, 8), Vector2(-7, 0)])
			draw_colored_polygon(d, Color(0.5, 0.35, 0.05, fade))
			d.append(d[0])
			draw_polyline(d, g, 1.5, true)
			draw_circle(Vector2.ZERO, 2.0 + sin(age * 6.0), Color(1, 1, 0.8, fade))
		"blueprint":
			var c := Color(0.4, 1.0, 1.0, fade)
			draw_rect(Rect2(-8, -8, 16, 16), Color(0.05, 0.3, 0.35, fade))
			draw_rect(Rect2(-8, -8, 16, 16), c, false, 1.5)
			draw_line(Vector2(-4, -3), Vector2(4, -3), c, 1.0)
			draw_line(Vector2(-4, 0), Vector2(4, 0), c, 1.0)
			draw_line(Vector2(-4, 3), Vector2(2, 3), c, 1.0)
		_:
			var c := Color(0.8, 0.9, 1.0, fade)
			draw_rect(Rect2(-7, -7, 14, 14), Color(0.2, 0.3, 0.45, fade))
			draw_rect(Rect2(-7, -7, 14, 14), c, false, 1.5)
			draw_circle(Vector2.ZERO, 3.0 + sin(age * 6.0), Color(1, 1, 1, fade))
