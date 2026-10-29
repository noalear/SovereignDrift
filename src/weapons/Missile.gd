## Missile -- torpedoes, plasma bolts, grenades, mines, seekers and shells.
## Each one is a small PhysicsBody: fast shots are swept against hulls so
## nothing tunnels through, and every hit carries momentum into its target.
class_name Missile
extends Node2D

var world: Node
var body: PhysicsBody = null
var hit_point := Vector2.INF     # where the last sweep touched, set by the world
var hit_normal := Vector2.ZERO
var owner_ship: Node = null
var owner_race := -1
var owner_team := -1              # the shooter's side (Ship.team()); friend or foe goes by this
var missile_id := "001"
var data: Dictionary
var velocity := Vector2.ZERO
var life := 3.0
var age := 0.0
var radius := 4.0
var warhead := 150.0
var target: Node2D = null
var armed := 0.0
var color := Color.WHITE
var exploded := false
var toughness := 1               # gun rounds it takes to shoot it down

static var _slug_mat: CanvasItemMaterial = null


## Mass driver slugs glow: they are drawn additively, like the Fx lights.
static func slug_material() -> CanvasItemMaterial:
	if _slug_mat == null:
		_slug_mat = CanvasItemMaterial.new()
		_slug_mat.blend_mode = CanvasItemMaterial.BLEND_MODE_ADD
	return _slug_mat


func setup(p_world: Node, p_owner: Node, p_race: int, p_id: String, p_pos: Vector2, p_vel: Vector2, p_angle: float, p_target: Node2D) -> void:
	world = p_world
	owner_ship = p_owner
	owner_race = p_race
	owner_team = p_owner.team() if p_owner is Ship else p_race
	missile_id = p_id
	data = GameData.MISSILES[p_id]
	global_position = p_pos
	velocity = p_vel
	rotation = p_angle
	reset_physics_interpolation()
	life = data["life"]
	radius = data["radius"]
	warhead = data["warhead"] * GameData.DAMAGE_SCALE
	target = p_target
	toughness = int(data.get("tough", 1))
	armed = 0.35 if data["trigger"] == "prox" else 0.05
	if p_id == "002":
		armed = 1.5
	body = PhysicsBody.new()
	body.kind = PhysicsBody.Kind.PROJECTILE
	body.node = self
	body.set_circle(radius)
	body.set_mass(radius * radius * 2.0)
	body.bullet = p_id != "002"
	body.gravity_scale = 0.0 if p_id == "002" else 1.0
	body.pos = p_pos
	body.prev_pos = p_pos
	body.vel = p_vel
	body.angle = p_angle
	if data["guided"]:
		Sfx.attach_loop(self, "rocket_loop", 0.5)
	var rc: Color = GameData.race(p_race)["color"]
	match data["look"]:
		"bolt": color = rc.lerp(Color.WHITE, 0.4)
		"slug":
			color = rc.lerp(Color(0.55, 0.9, 1.0), 0.6)
			material = slug_material()
		"plasma": color = rc.lerp(Color(1, 0.6, 1), 0.5)
		"grenade": color = Color(1.0, 0.85, 0.4)
		"mine": color = Color(0.7, 0.7, 0.75)
		"missile": color = Color(1.0, 1.0, 1.0)
		"shell": color = Color(1.0, 0.6, 0.2)
		"disruptor": color = Color(0.7, 0.4, 1.0)
		"drone": color = rc.lerp(Color.WHITE, 0.2)
		"tracer": color = Color(1.0, 0.92, 0.6)


func _physics_process(delta: float) -> void:
	age += delta
	if armed > 0.0:
		armed -= delta
	if age >= life:
		if data["trigger"] == "prox" or missile_id == "002":
			explode(null)
		else:
			_free()
		return
	if data["guided"]:
		_seek(delta)
	var cap: float = data["max_v"] * GameData.LIGHT_SPEED
	if velocity.length() > cap:
		velocity = velocity.normalized() * cap
	if missile_id == "002":
		velocity = velocity.lerp(Vector2.ZERO, 1.5 * delta)
	if data["look"] != "mine":
		rotation = velocity.angle()
	queue_redraw()


func pre_step(_delta: float) -> void:
	body.enabled = not exploded
	body.pos = global_position
	body.vel = velocity
	body.angle = rotation


func post_step(_delta: float) -> void:
	global_position = body.pos
	velocity = body.vel
	rotation = body.angle


## Momentum this shot carries into whatever it hits.
func momentum() -> Vector2:
	return velocity * body.mass


func _free() -> void:
	exploded = true
	if world != null:
		world.release_body(body)
	queue_free()


## A guided shot keeps after a ship while its launcher holds a lock on
## the shot's channel (heat for seekers, radar return for the rest) or
## its own seeker head still sees it: seek range x signature / 100.
func _lock_ok(t: Node2D) -> bool:
	if not t is Ship:
		return true
	var ch := "heat" if data.get("heat", false) else "radar"
	if is_instance_valid(owner_ship) and owner_ship is Ship and not owner_ship.dead and owner_ship.has_track(t, ch):
		return true
	var head: float = data["seek_range"] * 2.0 * t.signature(ch) / 100.0
	return global_position.distance_to(t.global_position) <= head


func _seek(delta: float) -> void:
	if target == null or not is_instance_valid(target) or (target is Ship and target.dead) or not _lock_ok(target):
		target = world.nearest_enemy_ship(global_position, owner_team, data["seek_range"] * 2.0)
		if target != null and not _lock_ok(target):
			target = null
	var accel: float = data.get("accel", 200.0)
	var dir := Vector2.RIGHT.rotated(rotation)
	if target != null:
		var to: Vector2 = target.global_position - global_position
		var tt := to.length() / maxf(velocity.length(), 50.0)
		var aim: Vector2 = to + target.velocity * tt * 0.5
		var desired := aim.angle()
		rotation = rotate_toward(rotation, desired, data.get("turn", 3.0) * delta)
		dir = Vector2.RIGHT.rotated(rotation)
	velocity += dir * accel * delta


## Proximity check against a ship's hull (for grenades and mines).
func check_hit(target_body: PhysicsBody) -> bool:
	if exploded or armed > 0.0:
		return false
	var prox: float = data.get("prox", 0.0)
	return target_body.surface_distance(global_position) <= radius + prox


func explode(hit: Node2D) -> void:
	if exploded:
		return
	exploded = true
	world.missile_detonated(self, hit, float(data.get("blast", data.get("prox", 0.0))))
	world.release_body(body)
	queue_free()


## Can a gun's shot knock this down?  Missiles, seekers, drones, torpedoes
## and mines can be shot out of the sky (Kyle).
func shootable() -> bool:
	return data.get("shootable", false)


## Is this a gun's shot: an unguided round that goes off when it hits?
func is_gun_shot() -> bool:
	return not data["guided"] and data["trigger"] == "impact"


## Shot down: it pops where it is and harms nobody.
func shoot_down() -> void:
	if exploded:
		return
	exploded = true
	world.missile_detonated(self, null, 0.0)
	world.release_body(body)
	queue_free()


func _draw() -> void:
	match data["look"]:
		"slug":
			# a mass driver slug: a white-hot core in a stretched blue glow,
			# with an ionised wake that grows over the first instants of flight
			var soft := Fx.soft_texture()
			var r := radius
			var wake := r * lerpf(5.0, 16.0, clampf(age / 0.12, 0.0, 1.0))
			draw_texture_rect(soft, Rect2(-wake, -r * 1.3, wake + r * 2.0, r * 2.6), false, Color(color.r, color.g, color.b, 0.3))
			draw_texture_rect(soft, Rect2(-wake * 0.55, -r * 0.6, wake * 0.55 + r * 1.8, r * 1.2), false, Color(color.r, color.g, color.b, 0.8))
			draw_texture_rect(soft, Rect2(-r * 1.5, -r * 2.0, r * 4.0, r * 4.0), false, Color(color.r, color.g, color.b, 0.55))
			for k in 2:
				draw_texture_rect(soft, Rect2(-r * 1.8, -r * 0.65, r * 3.2, r * 1.3), false, Color(1, 1, 1, 0.95))
		"bolt":
			draw_line(Vector2(-radius * 3.0, 0), Vector2(radius * 1.5, 0), color, radius * 0.9, true)
			draw_circle(Vector2(radius, 0), radius * 0.8, Color.WHITE)
		"tracer":
			# a point defence round: a thin streak, read as a stream of fire
			draw_line(Vector2(-radius * 7.0, 0), Vector2(radius, 0), Color(color.r, color.g, color.b, 0.75), radius * 0.7, true)
		"plasma":
			draw_circle(Vector2.ZERO, radius * 1.8, Color(color.r, color.g, color.b, 0.35))
			draw_circle(Vector2.ZERO, radius, color)
			draw_circle(Vector2.ZERO, radius * 0.5, Color.WHITE)
		"grenade":
			draw_circle(Vector2.ZERO, radius, color)
			draw_circle(Vector2.ZERO, radius * 0.5, Color(0.4, 0.2, 0.1))
		"mine":
			# a mine must read from across the screen: a red trigger ring
			# that pulses, a soft glow, and a spiked body with a beacon that
			# blinks faster once it is armed
			var hot := armed <= 0.0
			var rate := 3.0 if hot else 1.0
			var blink := fmod(age * rate, 1.0) < 0.3
			var pulse := 0.5 + 0.5 * sin(age * TAU * rate * 0.5)
			var trigger: float = radius + float(data.get("prox", 0.0))
			draw_circle(Vector2.ZERO, trigger, Color(1, 0.15, 0.1, (0.05 + 0.05 * pulse) if hot else 0.03))
			draw_arc(Vector2.ZERO, trigger, 0, TAU, 40, Color(1, 0.25, 0.15, (0.45 + 0.35 * pulse) if hot else 0.2), 2.0, true)
			draw_circle(Vector2.ZERO, radius * 2.2, Color(1, 0.3, 0.2, 0.18 + 0.12 * pulse))
			for i in 6:
				var a := TAU * i / 6.0 + age * 0.5
				draw_line(Vector2.ZERO, Vector2(cos(a), sin(a)) * radius * 1.9, color, 3.0, true)
			draw_circle(Vector2.ZERO, radius * 1.1, color)
			draw_circle(Vector2.ZERO, radius * 1.1, Color(0.2, 0.2, 0.22), false, 1.5, true)
			draw_circle(Vector2.ZERO, radius * 0.5, Color(1, 0.25, 0.2) if blink else Color(0.45, 0.1, 0.1))
		"missile":
			draw_line(Vector2(-radius * 3.0, 0), Vector2(radius * 2.0, 0), color, radius * 0.8)
			draw_line(Vector2(-radius * 3.0, 0), Vector2(-radius * 6.0, 0), Color(1, 0.7, 0.3, 0.7), radius * 0.6)
			draw_circle(Vector2(radius * 2.0, 0), radius * 0.6, Color(1, 0.3, 0.3))
		"shell":
			draw_circle(Vector2.ZERO, radius, color)
			draw_line(Vector2(-radius * 4.0, 0), Vector2.ZERO, Color(color.r, color.g, color.b, 0.4), radius)
		"disruptor":
			var wob := sin(age * 40.0) * radius * 0.6
			draw_line(Vector2(-radius * 3.0, wob), Vector2(radius * 1.5, -wob), color, radius * 0.7, true)
			draw_circle(Vector2.ZERO, radius * 1.4, Color(color.r, color.g, color.b, 0.3))
			draw_circle(Vector2(radius, 0), radius * 0.6, Color.WHITE)
		"drone":
			var tri := PackedVector2Array([Vector2(radius * 2.0, 0), Vector2(-radius * 1.5, -radius * 1.5), Vector2(-radius * 0.5, 0), Vector2(-radius * 1.5, radius * 1.5)])
			draw_colored_polygon(tri, color)
			draw_circle(Vector2(-radius * 0.8, 0), radius * 0.5, Color(0.6, 1.0, 0.7))
