## Powerup -- a ship module floating in space after an explosion, or made
## by a friendly colony.  Any ship that flies into it takes it aboard if it
## is an upgrade.
class_name Powerup
extends Node2D

var module_id := "5002"
var velocity := Vector2.ZERO
var age := 0.0
var life := 40.0
var label := ""


func setup(id: String, pos: Vector2, vel: Vector2) -> void:
	module_id = id
	global_position = pos
	velocity = vel
	label = GameData.MODULES[id]["name"]
	rotation = randf() * TAU


func _physics_process(delta: float) -> void:
	age += delta
	global_position += velocity * delta
	velocity *= 1.0 - 0.4 * delta
	rotation += 0.8 * delta
	if age > life:
		queue_free()
	queue_redraw()


func _draw() -> void:
	var fade := clampf((life - age) / 5.0, 0.0, 1.0)
	var c := Color(0.8, 0.9, 1.0, fade)
	draw_rect(Rect2(-7, -7, 14, 14), Color(0.2, 0.3, 0.45, fade))
	draw_rect(Rect2(-7, -7, 14, 14), c, false, 1.5)
	draw_circle(Vector2.ZERO, 3.0 + sin(age * 6.0), Color(1, 1, 1, fade))
