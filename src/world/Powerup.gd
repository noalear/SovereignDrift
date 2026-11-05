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
var spawn_id := ""                # the id it was made from, as the network sends it

const CRATE_LENGTH := 40.0        # a cargo container's long side, world px


## kind: "part" (a module id), "blueprint" ("bp:" + module id),
## "credits" ("cr:" + amount) or "cargo" ("rs:" + resource + ":" + units,
## spilled from a wreck's hold; module_id is then the resource).
var kind := "part"
var amount := 0


func setup(id: String, pos: Vector2, vel: Vector2) -> void:
	module_id = id
	spawn_id = id
	global_position = pos
	velocity = vel
	rotation = randf() * TAU
	reset_physics_interpolation()
	if id.begins_with("bp:"):
		kind = "blueprint"
		module_id = id.substr(3)
		label = "%s blueprint" % GameData.MODULES[module_id]["name"]
	elif id.begins_with("cr:"):
		kind = "credits"
		amount = int(id.substr(3))
		label = "%d credits" % amount
		life = 45.0               # long enough to fly back from the starbase for a lost purse
	elif id.begins_with("rs:"):
		kind = "cargo"
		module_id = id.get_slice(":", 1)
		amount = int(id.get_slice(":", 2))
		relabel()
		life = 60.0
		# the container painting is mipmapped: shrunk to a few dozen pixels
		# it stays smooth instead of shimmering as it tumbles
		texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
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


func relabel() -> void:
	if kind == "cargo":
		label = "%d %s" % [amount, Economy.NAMES.get(module_id, module_id)]
		spawn_id = "rs:%s:%d" % [module_id, amount]


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
	# salvage reads as the real thing: the part itself tumbling in a soft
	# glow (cyan and ghostly for a blueprint), credits as a gold glimmer
	var fade := clampf((life - age) / 5.0, 0.0, 1.0)
	var pulse := 0.75 + 0.25 * sin(age * 4.0)
	var soft := Fx.soft_texture()
	match kind:
		"credits":
			draw_texture_rect(soft, Rect2(-16, -16, 32, 32), false, Color(1.0, 0.75, 0.25, 0.45 * pulse * fade))
			draw_texture_rect(soft, Rect2(-6, -6, 12, 12), false, Color(1.0, 0.95, 0.7, 0.95 * fade))
		"cargo":
			# a tumbling shipping container painted with what it holds, in a
			# faint glow of the resource's colour so it can be found in the dark
			var c: Color = Economy.COLORS.get(module_id, Color.WHITE)
			draw_texture_rect(soft, Rect2(-26, -26, 52, 52), false, Color(c.r, c.g, c.b, 0.28 * pulse * fade))
			var tex := Economy.crate_texture(module_id)
			if tex == null:
				draw_rect(Rect2(-8, -6, 16, 12), Color(c.r, c.g, c.b, fade))
			else:
				var h := CRATE_LENGTH * tex.get_height() / tex.get_width()
				draw_texture_rect(tex, Rect2(-CRATE_LENGTH * 0.5, -h * 0.5, CRATE_LENGTH, h), false, Color(1, 1, 1, fade))
		"blueprint":
			draw_texture_rect(soft, Rect2(-18, -18, 36, 36), false, Color(0.3, 0.9, 1.0, 0.35 * pulse * fade))
			_draw_part(Color(0.55, 1.0, 1.0, 0.75 * fade))
		_:
			draw_texture_rect(soft, Rect2(-18, -18, 36, 36), false, Color(0.75, 0.85, 1.0, 0.3 * pulse * fade))
			_draw_part(Color(1, 1, 1, fade))


func _draw_part(mod: Color) -> void:
	var tex := ShipArt.part_texture(ShipArt.module_icon(module_id))
	if tex == null:
		draw_circle(Vector2.ZERO, 5.0, mod)
		return
	var w := 22.0
	var h := w * tex.get_height() / tex.get_width()
	draw_texture_rect(tex, Rect2(-w * 0.5, -h * 0.5, w, h), false, mod)
