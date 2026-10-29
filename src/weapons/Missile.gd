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

## How guided shots ended, for the tutorial and the test tools: missile id -> {"hit",
## "blast" (went off near nothing), "spent" (out of fuel), "down"}.
static var fates := {}

static func _tally(id: String, how: String) -> void:
	if not fates.has(id):
		fates[id] = {"hit": 0, "blast": 0, "spent": 0, "down": 0}
	fates[id][how] += 1

static var _glow_mat: CanvasItemMaterial = null
static var _paintings := {}

const EMPIRES := ["arcean", "terran", "drengin", "pirate"]
## Painted munitions (assets/munitions/<empire>/<look>.png, nose right) and
## how long each is drawn, in shot radii.
const PAINT_LEN := {"slug": 6.5, "bolt": 5.8, "missile": 6.5, "seeker": 6.4, "torpedo": 5.6, "shell": 3.6, "mine": 3.2, "drone": 6.5}
## Each empire's engine light, for exhaust: Terran blue, Arcean green,
## Drengin and Pirate orange.
const EXHAUST := [Color(0.45, 1.0, 0.55), Color(0.5, 0.75, 1.0), Color(1.0, 0.5, 0.18), Color(1.0, 0.62, 0.25)]
## The glow between each empire's rails (the mass driver's look): Arcean
## green, Terran blue, Drengin orange, Pirate blue.
const RAIL := [Color(0.5, 1.0, 0.6), Color(0.55, 0.9, 1.0), Color(1.0, 0.6, 0.25), Color(0.5, 0.95, 1.0)]
const TRAIL_POINTS := 10

var glow: Node2D                  # additive layer: flames, glows, trails
var paint: Texture2D = null
var trail := PackedVector2Array() # recent positions, newest last (world space)
var _puff_at := Vector2.INF       # where the exhaust last left a puff
var race_ix := 1                  # owner_race clamped to the four empires


## Glowing things are drawn additively, like the Fx lights.
static func glow_material() -> CanvasItemMaterial:
	if _glow_mat == null:
		_glow_mat = CanvasItemMaterial.new()
		_glow_mat.blend_mode = CanvasItemMaterial.BLEND_MODE_ADD
	return _glow_mat


## An empire's painting of a munition, or the Terran one, or null.
static func painting(look: String, race_id: int) -> Texture2D:
	var key := look + str(race_id)
	if not _paintings.has(key):
		var tex: Texture2D = null
		for e in [EMPIRES[clampi(race_id, 0, 3)], "terran"]:
			var path := "res://assets/munitions/%s/%s.png" % [e, look]
			if ResourceLoader.exists(path):
				tex = load(path)
				break
		_paintings[key] = tex
	return _paintings[key]


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
	glow = Node2D.new()
	glow.material = glow_material()
	glow.draw.connect(_draw_glow)
	add_child(glow)
	_tint(p_race)
	trail.append(p_pos)


## The shot's look in the shooter's empire's colours and paint.
func _tint(p_race: int) -> void:
	race_ix = clampi(p_race, 0, 3)
	var rc: Color = GameData.race(p_race)["color"]
	var look: String = data["look"]
	match look:
		"slug", "bolt": color = RAIL[race_ix]
		"plasma": color = rc.lerp(Color(1, 0.6, 1), 0.5)
		"torpedo", "missile", "seeker", "drone": color = EXHAUST[race_ix]
		"mine": color = Color(1.0, 0.3, 0.2) if race_ix != 0 else Color(0.4, 1.0, 0.5)
		"shell": color = Color(1.0, 0.6, 0.2)
		"disruptor": color = Color(0.7, 0.45, 1.0)
		"tracer": color = Color(1.0, 0.85, 0.5)
	paint = painting(look, race_ix) if PAINT_LEN.has(look) else null
	# a painted munition's flame sits behind its body
	glow.show_behind_parent = paint != null


## Turned by a Deflector shield (World._deflect_shots): the shot leaves
## from `pos` at `vel` as the deflecting ship's own, in its colours.
func deflect(by: Node, pos: Vector2, vel: Vector2) -> void:
	owner_ship = by
	owner_race = by.race_id if "race_id" in by else (by.owner_race if "owner_race" in by else owner_race)
	owner_team = by.team() if by is Ship else owner_race
	target = null
	armed = 0.0
	global_position = pos
	velocity = vel
	rotation = vel.angle()
	reset_physics_interpolation()
	if body != null:
		body.pos = pos
		body.prev_pos = pos
		body.vel = vel
	_tint(owner_race)
	# the trail starts again from the bounce, not across it
	trail.clear()
	trail.append(pos)
	_puff_at = Vector2.INF
	queue_redraw()
	glow.queue_redraw()


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
	_leave_trail()
	queue_redraw()
	glow.queue_redraw()


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
	if data["guided"]:
		_tally(missile_id, "spent")
	if world != null:
		world.release_body(body)
	queue_free()


## A guided shot keeps after a ship while its launcher holds a lock on
## the shot's channel (heat for seekers, radar return for the rest) or
## its own seeker head still sees it: seek range x signature / 100.  The
## head looks ahead, within "cone" radians of the flight path (HEAD_CONE
## unless the shot says), so a shot that overshoots loses its ship and
## flies on instead of circling back round for it.  A head that has
## the ship keeps it through a short fade (HEAD_HOLD: a burn ending, a
## turn across the nose), so the lock doesn't drop for one bad frame.
const HEAD_CONE := 0.9
const HEAD_HOLD := 0.6
var _unseen := 0.0                # seconds the head has not seen its target

func _lock_ok(t: Node2D) -> bool:
	if not t is Ship:
		return true
	if not _in_cone(t):
		return false
	var ch := "heat" if data.get("heat", false) else "radar"
	if is_instance_valid(owner_ship) and owner_ship is Ship and not owner_ship.dead and owner_ship.has_track(t, ch):
		return true
	var head: float = data["seek_range"] * 2.0 * t.signature(ch) / 100.0
	return global_position.distance_to(t.global_position) <= head


func _in_cone(t: Node2D) -> bool:
	if velocity.length() < 1.0:
		return true
	var to: Vector2 = t.global_position - global_position
	return absf(angle_difference(velocity.angle(), to.angle())) <= float(data.get("cone", HEAD_CONE))


func _keeps(t, delta: float) -> bool:     # untyped: the target may have been freed
	if t == null or not is_instance_valid(t) or (t is Ship and t.dead):
		return false
	if _lock_ok(t):
		_unseen = 0.0
		return true
	_unseen += delta
	return _unseen < HEAD_HOLD


## Guidance steers the flight path itself, not just the nose: each frame
## the velocity swings toward the intercept point by up to "turn" radians
## a second while the motor builds speed by "accel" up to max_v.  (It used
## to turn only the nose and push along it, and the nose was set back to
## the flight path every frame, so shots barely curved: Kyle, 2026-10-29,
## "they don't seem to actively seek anything".)
func _seek(delta: float) -> void:
	if not _keeps(target, delta):
		_unseen = 0.0
		target = world.nearest_enemy_ship(global_position, owner_team, data["seek_range"] * 2.0)
		if target != null and not _lock_ok(target):
			target = null
	var cap: float = data["max_v"] * GameData.LIGHT_SPEED
	var speed := velocity.length()
	var dir := velocity / speed if speed > 1.0 else Vector2.RIGHT.rotated(rotation)
	speed = minf(speed + data.get("accel", 200.0) * delta, maxf(cap, speed))
	if target != null:
		var aim := intercept(target, maxf(speed, 50.0))
		var ang := rotate_toward(dir.angle(), aim.angle(), data.get("turn", 3.0) * delta)
		dir = Vector2.RIGHT.rotated(ang)
	velocity = dir * speed


## Where to point to meet `t` flying at `speed`: the lead point of a
## straight-line intercept, or straight at it when it can't be caught.
func intercept(t: Node2D, speed: float) -> Vector2:
	var r: Vector2 = t.global_position - global_position
	var tv: Vector2 = t.velocity if "velocity" in t else Vector2.ZERO
	var a := tv.dot(tv) - speed * speed
	var b := 2.0 * r.dot(tv)
	var c := r.dot(r)
	var tt := -1.0
	if absf(a) < 0.001:
		tt = -c / b if b < 0.0 else -1.0
	else:
		var disc := b * b - 4.0 * a * c
		if disc >= 0.0:
			var sq := sqrt(disc)
			var t1 := (-b - sq) / (2.0 * a)
			var t2 := (-b + sq) / (2.0 * a)
			tt = minf(t1, t2) if minf(t1, t2) > 0.0 else maxf(t1, t2)
	if tt <= 0.0:
		return r
	return r + tv * minf(tt, maxf(life - age, 0.5))


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
	if data["guided"]:
		_tally(missile_id, "hit" if hit != null else "blast")
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
	if data["guided"]:
		_tally(missile_id, "down")
	world.missile_detonated(self, null, 0.0)
	world.release_body(body)
	queue_free()


## Remember where the shot has been, for its trail, and let a burning
## motor leave puffs of exhaust behind it in the world.
func _leave_trail() -> void:
	trail.append(global_position)
	if trail.size() > TRAIL_POINTS:
		trail.remove_at(0)
	if not data["guided"] or world == null or world.fx == null:
		return
	var tail: Vector2 = global_position - Vector2.RIGHT.rotated(rotation) * radius * PAINT_LEN.get(data["look"], 4.0) * 0.5
	var gap := radius * (0.9 if data["look"] == "drone" else 1.4)
	if _puff_at == Vector2.INF or _puff_at.distance_to(tail) >= gap:
		_puff_at = tail
		world.fx.exhaust(tail, velocity * 0.15, color, radius, data["look"])


func _draw() -> void:
	if paint == null:
		if data["look"] == "mine":
			_draw_mine_body()
		return
	var len: float = radius * PAINT_LEN[data["look"]]
	var sz := Vector2(len, len * paint.get_height() / paint.get_width())
	if data["look"] == "mine":
		# a mine turns slowly where it floats
		draw_set_transform(Vector2.ZERO, age * 0.4)
	draw_texture_rect(paint, Rect2(-sz * 0.5, sz), false)
	draw_set_transform(Vector2.ZERO)


## No painting: a plain spiked mine.
func _draw_mine_body() -> void:
	for i in 6:
		var a := TAU * i / 6.0 + age * 0.5
		draw_line(Vector2.ZERO, Vector2(cos(a), sin(a)) * radius * 1.4, Color(0.7, 0.7, 0.75), 3.0, true)
	draw_circle(Vector2.ZERO, radius * 1.1, Color(0.55, 0.56, 0.6))


## The light of a shot: flames, glowing rounds and their trails, all soft
## textures drawn additively (no hard geometric shapes: Kyle).
func _draw_glow() -> void:
	var soft := Fx.soft_texture()
	var r := radius
	var c := color
	match data["look"]:
		"slug", "bolt":
			# a mass driver slug or rapid round is a solid piece of metal,
			# not a bolt of light (Kyle): only its base still glows from the
			# rails, with a short smear of heat behind it and a faint flicker
			# of the empire's rail charge in the first instant of flight
			var len: float = r * PAINT_LEN[data["look"]]
			var back := -len * 0.5
			var smear := minf(velocity.length() * 0.035, r * 7.0)
			glow.draw_texture_rect(soft, Rect2(back - smear, -r * 0.45, smear + r * 0.8, r * 0.9), false, Color(1.0, 0.5, 0.2, 0.35))
			glow.draw_texture_rect(soft, Rect2(Vector2(back, 0) - Vector2(r, r) * 0.9, Vector2(r, r) * 1.8), false, Color(1.0, 0.55, 0.22, 0.85))
			glow.draw_texture_rect(soft, Rect2(Vector2(back, 0) - Vector2(r, r) * 0.4, Vector2(r, r) * 0.8), false, Color(1.0, 0.9, 0.7, 0.9))
			var charge := clampf(1.0 - age / 0.12, 0.0, 1.0)
			if charge > 0.0:
				glow.draw_texture_rect(soft, Rect2(Vector2(-len, -len) * 0.7, Vector2(len, len) * 1.4), false, Color(c, 0.45 * charge))
			if paint == null:
				glow.draw_texture_rect(soft, Rect2(-len * 0.5, -r * 0.5, len, r), false, Color(0.75, 0.75, 0.8))
		"tracer":
			# a point defence round: a short hot streak, read as a stream of fire
			glow.draw_texture_rect(soft, Rect2(-r * 9.0, -r * 0.9, r * 10.0, r * 1.8), false, Color(c, 0.75))
			glow.draw_texture_rect(soft, Rect2(-r * 3.0, -r * 0.5, r * 4.0, r * 1.0), false, Color(1, 1, 0.9, 0.9))
		"shell":
			# the cannon shell's base burns like a tracer, and the air it
			# never had glows behind it anyway
			_draw_trail(soft, Color(1.0, 0.55, 0.2, 0.3), r * 0.9)
			var back := -r * PAINT_LEN["shell"] * 0.5
			glow.draw_texture_rect(soft, Rect2(back - r * 4.0, -r * 0.8, r * 5.0, r * 1.6), false, Color(1.0, 0.55, 0.2, 0.7))
			glow.draw_texture_rect(soft, Rect2(back - r * 1.2, -r * 1.2, r * 2.4, r * 2.4), false, Color(1.0, 0.85, 0.5, 0.9))
			if paint == null:
				glow.draw_texture_rect(soft, Rect2(-r * 1.5, -r, r * 3.0, r * 2.0), false, Color(1.0, 0.7, 0.35))
		"plasma":
			_draw_trail(soft, Color(c, 0.4), r * 1.6)
			var pulse := 1.0 + 0.12 * sin(age * 30.0)
			glow.draw_texture_rect(soft, Rect2(Vector2(-r, -r) * 3.0 * pulse, Vector2(r, r) * 6.0 * pulse), false, Color(c, 0.35))
			glow.draw_texture_rect(soft, Rect2(Vector2(-r, -r) * 1.6, Vector2(r, r) * 3.2), false, Color(c, 0.9))
			glow.draw_texture_rect(soft, Rect2(Vector2(-r, -r) * 0.8, Vector2(r, r) * 1.6), false, Color(1, 1, 1, 0.95))
		"disruptor":
			# a ball of ion fire with sparks crackling round it and a
			# fading wake of charge
			_draw_trail(soft, Color(c, 0.4), r * 1.3)
			glow.draw_texture_rect(soft, Rect2(Vector2(-r, -r) * 3.2, Vector2(r, r) * 6.4), false, Color(c, 0.3))
			glow.draw_texture_rect(soft, Rect2(Vector2(-r, -r) * 1.5, Vector2(r, r) * 3.0), false, Color(c.lerp(Color.WHITE, 0.3), 0.9))
			glow.draw_texture_rect(soft, Rect2(Vector2(-r, -r) * 0.7, Vector2(r, r) * 1.4), false, Color(1, 1, 1))
			for k in 4:
				var a := age * (17.0 + k * 5.0) + k * 1.7 + sin(age * 53.0 + k) * 2.0
				var d := r * (1.6 + 0.8 * sin(age * 41.0 + k * 2.3))
				var at := Vector2(cos(a), sin(a)) * d
				glow.draw_texture_rect(soft, Rect2(at - Vector2(r, r) * 0.45, Vector2(r, r) * 0.9), false, Color(0.85, 0.75, 1.0, 0.9))
		"missile", "seeker", "torpedo", "drone":
			_draw_flame(soft)
		"mine":
			_draw_mine_glow(soft)


## A fading ribbon of soft light through the last few positions.
func _draw_trail(soft: Texture2D, c: Color, w: float) -> void:
	var n := trail.size()
	if n < 2:
		return
	glow.draw_set_transform_matrix(global_transform.affine_inverse())
	var head := global_position
	for i in n - 1:
		var a: Vector2 = trail[i]
		var b: Vector2 = trail[i + 1] if i + 1 < n - 1 else head
		var t := float(i + 1) / float(n)
		var steps := clampi(int(a.distance_to(b) / maxf(w * 1.2, 1.0)), 1, 6)
		for k in steps:
			var p := a.lerp(b, (k + 0.5) / steps)
			var ww := w * (0.35 + 0.65 * t)
			glow.draw_texture_rect(soft, Rect2(p - Vector2(ww, ww), Vector2(ww, ww) * 2.0), false, Color(c, c.a * t * t))
	glow.draw_set_transform_matrix(Transform2D.IDENTITY)


## The motor of a missile, seeker, torpedo or drone: a flickering flame
## out of the tail, longer while the motor is still building speed.
func _draw_flame(soft: Texture2D) -> void:
	var r := radius
	var c := color
	var back: float = -r * PAINT_LEN.get(data["look"], 4.0) * 0.5 + r * 0.3
	var flick := 0.85 + 0.15 * sin(age * 67.0) + 0.1 * sin(age * 23.0)
	var burn := 1.0 if age < 0.6 else 0.75
	var w := r * (0.9 if data["look"] != "torpedo" else 1.1)
	if data["look"] == "drone":
		w *= 0.75
		burn *= 0.7
	var flame := r * 3.5 * flick * burn
	glow.draw_texture_rect(soft, Rect2(back - flame, -w, flame + r * 0.6, w * 2.0), false, Color(c, 0.75))
	glow.draw_texture_rect(soft, Rect2(back - flame * 0.45, -w * 0.5, flame * 0.45 + r * 0.4, w), false, Color(1, 1, 0.95, 0.95))
	glow.draw_texture_rect(soft, Rect2(Vector2(back, 0) - Vector2(w, w) * 2.2, Vector2(w, w) * 4.4), false, Color(c, 0.3))
	if data["look"] == "seeker":
		# the seeker's eye glows on the nose
		var nose: float = r * PAINT_LEN["seeker"] * 0.42
		glow.draw_texture_rect(soft, Rect2(Vector2(nose, 0) - Vector2(r, r) * 0.8, Vector2(r, r) * 1.6), false, Color(1.0, 0.45, 0.25, 0.6 + 0.3 * sin(age * 12.0)))


## A mine must read from across the screen: a soft red glow over its
## trigger reach that pulses, and a beacon that blinks faster once armed.
func _draw_mine_glow(soft: Texture2D) -> void:
	var hot := armed <= 0.0
	var rate := 3.0 if hot else 1.0
	var blink := fmod(age * rate, 1.0) < 0.3
	var pulse := 0.5 + 0.5 * sin(age * TAU * rate * 0.5)
	var trigger: float = radius + float(data.get("prox", 0.0))
	var c := color
	glow.draw_texture_rect(soft, Rect2(Vector2(-trigger, -trigger) * 1.1, Vector2(trigger, trigger) * 2.2), false, Color(c, (0.03 + 0.03 * pulse) if hot else 0.015))
	glow.draw_arc(Vector2.ZERO, trigger, 0, TAU, 48, Color(c, (0.22 + 0.16 * pulse) if hot else 0.08), 2.0, true)
	var b := radius * (0.9 if blink else 0.5)
	glow.draw_texture_rect(soft, Rect2(Vector2(-b, -b) * 2.0, Vector2(b, b) * 4.0), false, Color(c, 0.9 if blink else 0.35))
