## Fx -- explosions, sparks, beams and the break-up of a wrecked ship, all
## drawn by one node to keep the scene tree small.  Nothing here collides
## with anything.  Fire, embers and glows go to an additive child layer so
## they light up instead of painting flat discs.
class_name Fx
extends Node2D

var explosions: Array = []   # fireballs {pos, vel, age, life, size, hot, cool}
var smoke: Array = []        # {pos, vel, age, life, size, shade}
var sparks: Array = []       # {pos, vel, age, life, color}
var beams: Array = []        # {a, b, color, age}
var shockwaves: Array = []
var shards: Array = []       # pieces of a wrecked painting {tex, poly, uvs, pos, vel, angle, spin, age, life, burn}
var droplets: Array = []     # Arcean coolant {pos, vel, age, wet, life, size}
var gravel: Array = []       # bits of a broken asteroid {pos, vel, age, life, size, color}
var ripples: Array = []      # time ripples from quantum jumps and sensor pings {pos, age, life, size, kind, color}
var muzzles: Array = []      # gun discharges {pos, vel, ang, age, life, size, color, kind}
var puffs: Array = []        # exhaust and gun smoke {pos, vel, age, life, size, color, hot, shade}
var crackles: Array = []     # ion discharges after a disruptor hit {pos, age, life, size, color, seed}
var glow: Node2D             # additive layer
const WRECK_LIFE := 3.2

static var _soft: Texture2D = null
static var _beam: Texture2D = null


func _ready() -> void:
	glow = Node2D.new()
	glow.name = "Glow"
	var mat := CanvasItemMaterial.new()
	mat.blend_mode = CanvasItemMaterial.BLEND_MODE_ADD
	glow.material = mat
	glow.draw.connect(_draw_glow)
	add_child(glow)


## A round, soft-edged dot for fire, smoke and vapour.
static func soft_texture() -> Texture2D:
	if _soft == null:
		var n := 64
		var img := Image.create(n, n, false, Image.FORMAT_RGBA8)
		for y in n:
			for x in n:
				var d := Vector2(x + 0.5 - n * 0.5, y + 0.5 - n * 0.5).length() / (n * 0.5)
				var a := clampf(1.0 - d, 0.0, 1.0)
				img.set_pixel(x, y, Color(1, 1, 1, a * a * (3.0 - 2.0 * a)))
		_soft = ImageTexture.create_from_image(img)
	return _soft


## A strip of light, bright down the middle and fading to nothing at both
## edges, stretched along a beam.
static func beam_texture() -> Texture2D:
	if _beam == null:
		var n := 32
		var img := Image.create(4, n, false, Image.FORMAT_RGBA8)
		for y in n:
			var v := (y + 0.5 - n * 0.5) / (n * 0.5)
			var a := exp(-v * v * 4.5) * clampf((1.0 - absf(v)) * 4.0, 0.0, 1.0)
			for x in 4:
				img.set_pixel(x, y, Color(1, 1, 1, a))
		_beam = ImageTexture.create_from_image(img)
	return _beam


func explosion(pos: Vector2, size: float, color: Color, big: bool = false, sound: String = "") -> void:
	var hot := color.lerp(Color(1.0, 0.85, 0.55), 0.6)
	explosions.append({"pos": pos, "vel": Vector2.ZERO, "age": 0.0, "life": 0.9 if not big else 1.6, "size": size, "hot": hot, "cool": color})
	if big:
		shockwaves.append({"pos": pos, "age": 0.0, "life": 1.2, "size": size * 4.0})
	var n := 10 if not big else 30
	for i in n:
		var a := randf() * TAU
		var sp := randf_range(60.0, 260.0 if not big else 480.0)
		sparks.append({"pos": pos, "vel": Vector2(cos(a), sin(a)) * sp, "age": 0.0, "life": randf_range(0.4, 1.1), "color": color.lerp(Color(1, 0.8, 0.4), randf())})
	if sound.is_empty():
		sound = "bigexplode" if big else "explode"
	Sfx.play(sound, pos, 1.0 if big else 0.85, clampf(1.25 - size / 160.0, 0.75, 1.15))


## An asteroid breaks: no fire, just a puff of dust and a spray of gravel
## that drifts off with the rock's own motion.
func rock_burst(pos: Vector2, vel: Vector2, r: float, color: Color) -> void:
	for i in 5 + int(r * 0.4):
		var a := randf() * TAU
		var off := Vector2(cos(a), sin(a))
		smoke.append({"pos": pos + off * r * randf() * 0.6, "vel": vel + off * randf_range(8.0, 40.0), "age": 0.0,
				"life": randf_range(1.2, 2.4), "size": r * randf_range(0.5, 0.9), "shade": color.v * randf_range(1.3, 1.9)})
	for i in 10 + int(r * 1.2):
		var a := randf() * TAU
		var off := Vector2(cos(a), sin(a))
		var shade := randf_range(0.6, 1.3)
		gravel.append({"pos": pos + off * r * randf() * 0.7, "vel": vel + off * randf_range(20.0, 120.0), "age": 0.0,
				"life": randf_range(1.0, 2.6), "size": randf_range(0.5, 0.07 * r + 0.7),
				"color": Color(color.r * shade, color.g * shade, color.b * shade)})
	explosions.append({"pos": pos, "vel": vel, "age": 0.0, "life": 0.25, "size": r * 0.5, "hot": Color(1.0, 0.95, 0.85), "cool": color})
	Sfx.play("explode", pos, 0.7, clampf(1.3 - r / 60.0, 0.9, 1.25))


## A ship dies: a fireball, and its painting breaks into jagged shards that
## burn, char and drift apart while the turrets spin away on their own.
## Arcean hulls are living things full of coolant, which sprays out as
## glowing liquid and boils off into vapour in the vacuum.
func ship_destroyed(s: Ship) -> void:
	var m: Dictionary = s.art_model if not s.art_model.is_empty() else ShipArt.build_model(s.design, s.race_id, s.size)
	var pos := s.global_position
	var vel := s.velocity
	var ang := s.rotation
	var big := s.size > 40.0
	var fire := Color(1.0, 0.45, 0.15) if s.race_id != 0 else Color(0.55, 1.0, 0.45)
	explosion(pos, s.size * 1.5, fire, big)
	for i in 3 + (3 if big else 0):
		var a := randf() * TAU
		explosions.append({"pos": pos + Vector2(cos(a), sin(a)) * s.size * randf_range(0.3, 0.9), "vel": vel * 0.6, "age": -randf_range(0.05, 0.4),
			"life": randf_range(0.5, 0.9), "size": s.size * randf_range(0.5, 0.9), "hot": Color(1.0, 0.85, 0.55), "cool": fire})
	var tex: Texture2D = m["tex"]
	var rect: Rect2 = m["rect"]
	if tex != null:
		for cell in _shatter(rect, clampi(int(s.size / 6.0), 5, 12)):
			var c := Vector2.ZERO
			for q in cell:
				c += q
			c /= cell.size()
			var local := PackedVector2Array()
			var uvs := PackedVector2Array()
			for q in cell:
				local.append(q - c)
				uvs.append((q - rect.position) / rect.size)
			var off := c.rotated(ang)
			var out := off.normalized() if off.length() > 0.01 else Vector2.RIGHT.rotated(randf() * TAU)
			shards.append({"tex": tex, "poly": local, "uvs": uvs, "pos": pos + off, "vel": vel + out * randf_range(18.0, 60.0),
				"angle": ang, "spin": s.angular_velocity + randf_range(-1.6, 1.6), "age": 0.0,
				"life": WRECK_LIFE + randf_range(-0.4, 0.6), "burn": randf_range(0.6, 1.0)})
	# the turrets come off whole
	for t in m["turrets"]:
		var w: float = t["w"]
		var h: float = t["h"]
		var poly := PackedVector2Array([Vector2(-w * 0.5, -h * 0.5), Vector2(w * 0.5, -h * 0.5), Vector2(w * 0.5, h * 0.5), Vector2(-w * 0.5, h * 0.5)])
		var off: Vector2 = (t["pos"] as Vector2).rotated(ang)
		var out := off.normalized() if off.length() > 0.01 else Vector2.RIGHT.rotated(randf() * TAU)
		shards.append({"tex": t["tex"], "poly": poly, "uvs": PackedVector2Array([Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1)]),
			"pos": pos + off, "vel": vel + out * randf_range(50.0, 120.0), "angle": ang + float(t["fire"]), "spin": randf_range(-5.0, 5.0),
			"age": 0.0, "life": WRECK_LIFE * 0.8, "burn": 0.5})
	if s.race_id == 0:
		_spill(pos, vel, ang, rect, s.size)
	# embers and smoke from the break
	for i in 10 + int(s.size * 0.4):
		var a := randf() * TAU
		sparks.append({"pos": pos + Vector2(cos(a), sin(a)) * s.size * randf(), "vel": vel * 0.5 + Vector2(cos(a), sin(a)) * randf_range(20.0, 110.0),
			"age": 0.0, "life": randf_range(0.8, 2.0), "color": Color(1.0, randf_range(0.45, 0.7), 0.2) if s.race_id != 0 else Color(0.6, 1.0, 0.55)})
	for i in 4 + int(s.size * 0.1):
		var a := randf() * TAU
		smoke.append({"pos": pos + Vector2(cos(a), sin(a)) * s.size * randf() * 0.6, "vel": vel * 0.7 + Vector2(cos(a), sin(a)) * randf_range(6.0, 24.0),
			"age": -randf_range(0.1, 0.5), "life": randf_range(1.8, 3.0), "size": s.size * randf_range(0.6, 1.1), "shade": randf_range(0.12, 0.22)})


## Arcean coolant: streams of bright liquid out of the breaks, each drop
## flashing into a puff of vapour once the vacuum boils it.
func _spill(pos: Vector2, vel: Vector2, ang: float, rect: Rect2, size: float) -> void:
	var jets := 5 + int(size / 12.0)
	for j in jets:
		var src := Vector2(randf_range(rect.position.x, rect.end.x) * 0.7, randf_range(rect.position.y, rect.end.y) * 0.6).rotated(ang)
		var dir := (src.normalized() if src.length() > 1.0 else Vector2.RIGHT.rotated(randf() * TAU)).rotated(randf_range(-0.5, 0.5))
		var speed := randf_range(70.0, 190.0)
		for k in 8 + randi() % 6:
			droplets.append({"pos": pos + src, "vel": vel + dir.rotated(randf_range(-0.18, 0.18)) * speed * randf_range(0.6, 1.2),
				"age": -randf_range(0.0, 0.5), "wet": randf_range(0.25, 0.6), "life": randf_range(1.2, 2.0), "size": randf_range(1.6, 3.2)})


## Cut a rectangle into convex Voronoi cells around scattered sites.
func _shatter(rect: Rect2, n: int) -> Array:
	var sites := []
	for i in n:
		sites.append(rect.position + Vector2(randf(), randf()) * rect.size)
	var box := PackedVector2Array([rect.position, Vector2(rect.end.x, rect.position.y), rect.end, Vector2(rect.position.x, rect.end.y)])
	var cells := []
	for i in n:
		var cell := box
		for j in n:
			if i == j or cell.size() < 3:
				continue
			var a: Vector2 = sites[i]
			var b: Vector2 = sites[j]
			cell = _clip(cell, (a + b) * 0.5, (b - a).normalized())
		if cell.size() >= 3:
			cells.append(cell)
	return cells


## Keep the part of a convex polygon on the side of the line through `p`
## that the normal `nrm` points away from.
func _clip(poly: PackedVector2Array, p: Vector2, nrm: Vector2) -> PackedVector2Array:
	var out := PackedVector2Array()
	var n := poly.size()
	for i in n:
		var a := poly[i]
		var b := poly[(i + 1) % n]
		var da := (a - p).dot(nrm)
		var db := (b - p).dot(nrm)
		if da <= 0.0:
			out.append(a)
		if (da < 0.0) != (db < 0.0) and absf(da - db) > 0.0001:
			out.append(a + (b - a) * (da / (da - db)))
	return out


func spark(pos: Vector2, count: int, color: Color) -> void:
	for i in count:
		var a := randf() * TAU
		sparks.append({"pos": pos, "vel": Vector2(cos(a), sin(a)) * randf_range(40.0, 160.0), "age": 0.0, "life": randf_range(0.2, 0.5), "color": color})


func beam(a: Vector2, b: Vector2, color: Color) -> void:
	beams.append({"a": a, "b": b, "color": color, "age": 0.0})


## A gun fires.  `kind` picks the look: "rail" (mass driver and rapid
## tube: a white flash, a jet of ionised gas thrown forward with the shot,
## puffs venting off the rails), "cannon" (a fireball and a cloud of
## smoke), "pdc" (a quick flicker), "energy" (disruptor and plasma: a
## bloom of the shot's light) or "launch" (missiles, torpedoes and drones
## leave the tube in a puff of smoke).  It rides along with the shooter.
func muzzle_flash(pos: Vector2, ang: float, vel: Vector2, color: Color, size: float = 10.0, kind: String = "rail") -> void:
	var life: float = {"rail": 0.16, "cannon": 0.22, "pdc": 0.06, "energy": 0.18, "launch": 0.14}.get(kind, 0.16)
	muzzles.append({"pos": pos, "vel": vel, "ang": ang, "age": 0.0, "life": life, "size": size, "color": color, "kind": kind})
	var fwd := Vector2.RIGHT.rotated(ang)
	match kind:
		"cannon":
			for i in 6:
				var d := fwd.rotated(randf_range(-0.6, 0.6)) * randf_range(20.0, 90.0)
				puffs.append({"pos": pos + fwd * size * randf_range(0.2, 1.2), "vel": vel + d, "age": -randf() * 0.04, "life": randf_range(0.6, 1.1),
					"size": size * randf_range(0.5, 0.9), "color": Color(1.0, 0.6, 0.25), "hot": 0.25, "shade": randf_range(0.3, 0.45)})
			for i in 8:
				var d := fwd.rotated(randf_range(-0.35, 0.35)) * randf_range(150.0, 380.0)
				sparks.append({"pos": pos, "vel": vel + d, "age": 0.0, "life": randf_range(0.15, 0.35), "color": Color(1.0, randf_range(0.55, 0.85), 0.3)})
		"launch":
			# the motor lights as the round clears the tube: smoke blown back
			for i in 4:
				var d := (-fwd).rotated(randf_range(-0.9, 0.9)) * randf_range(10.0, 50.0)
				puffs.append({"pos": pos - fwd * size * randf_range(0.0, 0.8), "vel": vel + d, "age": 0.0, "life": randf_range(0.5, 0.9),
					"size": size * randf_range(0.45, 0.7), "color": color, "hot": 0.12, "shade": randf_range(0.42, 0.55)})
		"energy":
			for i in 5:
				var d := fwd.rotated(randf_range(-0.8, 0.8)) * randf_range(80.0, 220.0)
				sparks.append({"pos": pos, "vel": vel + d, "age": 0.0, "life": randf_range(0.1, 0.25), "color": color.lerp(Color.WHITE, 0.4)})


## A burning motor leaves a puff of exhaust: hot for a moment in the
## engine's colour, then a pale wisp that spreads and fades.  Drones run
## on a cleaner ion drive: no smoke, only a short glowing wake.
func exhaust(pos: Vector2, vel: Vector2, color: Color, size: float, look: String) -> void:
	var drift := Vector2.RIGHT.rotated(randf() * TAU) * randf_range(3.0, 12.0)
	if look == "drone":
		puffs.append({"pos": pos, "vel": vel + drift, "age": 0.0, "life": 0.35, "size": size * 0.8, "color": color, "hot": 0.35, "shade": 0.0})
		return
	var big := 1.4 if look == "torpedo" else 1.0
	puffs.append({"pos": pos, "vel": vel + drift, "age": 0.0, "life": randf_range(0.55, 0.9) * big, "size": size * 0.9 * big,
		"color": color, "hot": 0.12, "shade": randf_range(0.45, 0.6)})


## A munition goes off.  How it looks goes by what it is and what it did:
## `how` is "hit" (struck something), "blast" (a warhead going off with a
## blast radius `splash`) or "down" (shot out of the sky, or a shot that
## popped on nothing).  `normal` is the struck surface's, if known.
func munition_burst(look: String, dtype: String, pos: Vector2, vel: Vector2, normal: Vector2, color: Color, radius: float, splash: float, how: String, sound: String = "") -> void:
	var back := -vel.normalized() if vel.length_squared() > 1.0 else Vector2.RIGHT.rotated(randf() * TAU)
	var away := normal if normal.length_squared() > 0.5 else back
	if splash > 0.0:
		# a torpedo or mine: a fireball, a ring of light, smoke and debris
		var fire := Color(1.0, 0.5, 0.18)
		explosion(pos, splash * 0.5, fire, look == "mine", sound)
		_flash(pos, splash * 1.2, Color(1.0, 0.7, 0.4), 0.15)
		for i in 6:
			var a := randf() * TAU
			puffs.append({"pos": pos + Vector2(cos(a), sin(a)) * splash * randf() * 0.3, "vel": vel * 0.3 + Vector2(cos(a), sin(a)) * randf_range(10.0, 40.0),
				"age": -randf_range(0.05, 0.25), "life": randf_range(1.0, 1.8), "size": splash * randf_range(0.25, 0.4), "color": fire, "hot": 0.2, "shade": randf_range(0.18, 0.3)})
		_debris(pos, vel * 0.3, 10, splash * 2.5, Color(0.55, 0.52, 0.5))
		return
	if how == "down":
		# shot down: a small pop, a puff of smoke, a few broken pieces
		var guided := look in ["missile", "seeker", "torpedo", "drone"]
		explosions.append({"pos": pos, "vel": vel * 0.2, "age": 0.0, "life": 0.35, "size": radius * (3.0 if guided else 1.6),
			"hot": Color(1.0, 0.9, 0.7), "cool": Color(1.0, 0.55, 0.2) if guided else color})
		if guided or look == "mine":
			puffs.append({"pos": pos, "vel": vel * 0.2, "age": 0.0, "life": 0.9, "size": radius * 2.2, "color": Color(1.0, 0.5, 0.2), "hot": 0.15, "shade": 0.35})
			_debris(pos, vel * 0.25, 5, 120.0, Color(0.6, 0.6, 0.62))
			Sfx.play(sound if sound != "" else "impact", pos, 0.6, 1.3)
		else:
			spark(pos, 2, color)
		return
	match look:
		"missile", "seeker", "drone":
			var fire := Color(1.0, 0.5, 0.2)
			explosion(pos, 8.0 + radius * 3.0, fire, false, sound)
			for i in 3:
				puffs.append({"pos": pos + away * radius * randf(), "vel": vel * 0.1 + away.rotated(randf_range(-1.0, 1.0)) * randf_range(15.0, 45.0),
					"age": -randf() * 0.1, "life": randf_range(0.8, 1.3), "size": radius * randf_range(2.0, 3.0), "color": fire, "hot": 0.2, "shade": 0.25})
		"tracer":
			_flash(pos, radius * 3.0, color, 0.08)
			_spray(pos, away, 3, 0.9, 80.0, 220.0, color, 0.15)
		"disruptor":
			_flash(pos, radius * 4.0, color, 0.2)
			crackles.append({"pos": pos, "age": 0.0, "life": 0.45, "size": radius * 6.0, "color": color, "seed": randi()})
			_spray(pos, away, 8, 1.6, 60.0, 200.0, Color(0.8, 0.7, 1.0), 0.3)
			Sfx.play(sound if sound != "" else "impact", pos, 0.8, 1.2)
		"plasma":
			_flash(pos, radius * 4.5, color, 0.25)
			_spray(pos, away, 10, 1.3, 50.0, 180.0, color.lerp(Color.WHITE, 0.3), 0.5)
			Sfx.play(sound if sound != "" else "impact", pos, 0.8)
		_:
			# kinetic: a white-hot flash where the round strikes, sparks
			# thrown off the struck plate, and bits of hull
			var heavy := look == "shell"
			_flash(pos, radius * (5.0 if heavy else 3.5), color.lerp(Color(1.0, 0.95, 0.85), 0.6), 0.18 if heavy else 0.12)
			_spray(pos, away, 14 if heavy else 7, 1.1, 90.0, 320.0 if heavy else 240.0, Color(1.0, 0.75, 0.4), 0.35)
			if heavy:
				explosion(pos, radius * 2.5, Color(1.0, 0.55, 0.2), false, sound)
				_debris(pos, Vector2.ZERO, 6, 160.0, Color(0.6, 0.6, 0.62))
				puffs.append({"pos": pos + away * radius, "vel": away * 25.0, "age": 0.0, "life": 1.1, "size": radius * 2.5, "color": Color(1.0, 0.55, 0.2), "hot": 0.15, "shade": 0.3})
			else:
				Sfx.play(sound if sound != "" else "impact", pos, 0.75, randf_range(0.95, 1.1))


## A quick bloom of light that does not travel.
func _flash(pos: Vector2, size: float, color: Color, life: float) -> void:
	explosions.append({"pos": pos, "vel": Vector2.ZERO, "age": 0.0, "life": life, "size": size * 0.6, "hot": Color(1, 1, 0.95), "cool": color})


## Sparks thrown off a surface, in a cone `spread` radians either side of `dir`.
func _spray(pos: Vector2, dir: Vector2, n: int, spread: float, lo: float, hi: float, color: Color, life: float) -> void:
	for i in n:
		var d := dir.rotated(randf_range(-spread, spread)) * randf_range(lo, hi)
		sparks.append({"pos": pos, "vel": d, "age": 0.0, "life": randf_range(life * 0.5, life * 1.3), "color": color.lerp(Color(1, 0.9, 0.6), randf() * 0.5)})


## Bits of metal: dark grains that tumble off, lit on the Sun side.
func _debris(pos: Vector2, vel: Vector2, n: int, speed: float, color: Color) -> void:
	for i in n:
		var a := randf() * TAU
		var shade := randf_range(0.6, 1.2)
		gravel.append({"pos": pos, "vel": vel + Vector2(cos(a), sin(a)) * randf_range(speed * 0.2, speed), "age": 0.0,
			"life": randf_range(0.6, 1.4), "size": randf_range(0.6, 1.6), "color": Color(color.r * shade, color.g * shade, color.b * shade)})


## A quantum jump tears time: rings of warped light race out from the
## spot like a sonic boom.
func time_ripple(pos: Vector2, ship_size: float) -> void:
	ripples.append({"pos": pos, "age": 0.0, "life": 1.8, "size": 900.0 + ship_size * 10.0, "kind": "time", "color": Color(0.65, 0.6, 1.0)})


## A pulse radar's sweep: one thin ring out to its reach.
func sensor_ping(pos: Vector2, reach: float, color: Color) -> void:
	ripples.append({"pos": pos, "age": 0.0, "life": 0.9, "size": reach, "kind": "ping", "color": color.lerp(Color(0.7, 1.0, 0.9), 0.5)})


func _process(delta: float) -> void:
	for e in explosions:
		e["age"] += delta
		e["pos"] += e["vel"] * delta
	explosions = explosions.filter(func(e): return e["age"] < e["life"])
	for s in smoke:
		s["age"] += delta
		s["pos"] += s["vel"] * delta
	smoke = smoke.filter(func(s): return s["age"] < s["life"])
	for s in sparks:
		s["age"] += delta
		s["pos"] += s["vel"] * delta
		s["vel"] *= 1.0 - 1.5 * delta
	sparks = sparks.filter(func(s): return s["age"] < s["life"])
	for b in beams:
		b["age"] += delta
	beams = beams.filter(func(b): return b["age"] < 0.08)
	for w in shockwaves:
		w["age"] += delta
	shockwaves = shockwaves.filter(func(w): return w["age"] < w["life"])
	for p in shards:
		p["age"] += delta
		p["pos"] += p["vel"] * delta
		p["vel"] *= 1.0 - 0.4 * delta
		p["angle"] += p["spin"] * delta
		p["spin"] *= 1.0 - 0.5 * delta
	shards = shards.filter(func(p): return p["age"] < p["life"])
	for d in droplets:
		d["age"] += delta
		if d["age"] > 0.0:
			d["pos"] += d["vel"] * delta
			if d["age"] > d["wet"]:
				d["vel"] *= 1.0 - 1.2 * delta   # the vapour spreads and slows
	droplets = droplets.filter(func(d): return d["age"] < d["wet"] + d["life"])
	for g in gravel:
		g["age"] += delta
		g["pos"] += g["vel"] * delta
	gravel = gravel.filter(func(g): return g["age"] < g["life"])
	for m in muzzles:
		m["age"] += delta
		m["pos"] += m["vel"] * delta
	muzzles = muzzles.filter(func(m): return m["age"] < m["life"])
	for p in puffs:
		p["age"] += delta
		p["pos"] += p["vel"] * delta
		p["vel"] *= 1.0 - 1.2 * delta
	puffs = puffs.filter(func(p): return p["age"] < p["life"])
	for c in crackles:
		c["age"] += delta
	crackles = crackles.filter(func(c): return c["age"] < c["life"])
	for r in ripples:
		r["age"] += delta
	ripples = ripples.filter(func(r): return r["age"] < r["life"])
	queue_redraw()
	glow.queue_redraw()


func _draw() -> void:
	var soft := soft_texture()
	# smoke and vapour sit under the wreck
	for s in smoke:
		if s["age"] < 0.0:
			continue
		var t: float = s["age"] / s["life"]
		var r: float = s["size"] * (0.6 + 1.2 * t)
		var sh: float = s["shade"]
		draw_texture_rect(soft, Rect2(s["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(sh, sh * 0.95, sh * 0.9, 0.55 * (1.0 - t)))
	for p in puffs:
		if p["age"] < 0.0 or p["shade"] <= 0.0:
			continue
		var t: float = p["age"] / p["life"]
		var r: float = p["size"] * (0.6 + 1.8 * sqrt(t))
		var sh: float = p["shade"]
		draw_texture_rect(soft, Rect2(p["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(sh, sh * 0.97, sh * 0.95, 0.4 * (1.0 - t) * (1.0 - t)))
	for d in droplets:
		var vt: float = d["age"] - d["wet"]
		if vt <= 0.0:
			continue
		var t: float = vt / d["life"]
		var r: float = d["size"] * (2.0 + 9.0 * sqrt(t))
		draw_texture_rect(soft, Rect2(d["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(0.75, 1.0, 0.85, 0.22 * (1.0 - t)))
	for g in gravel:
		# a grain lit on the side facing the Sun, at the origin
		var c: Color = g["color"]
		var a := clampf((1.0 - g["age"] / g["life"]) / 0.3, 0.0, 1.0)
		var sz: float = g["size"]
		var sun: Vector2 = -(g["pos"] as Vector2).normalized()
		draw_circle(g["pos"], sz, Color(c.r * 0.35, c.g * 0.35, c.b * 0.35, a))
		draw_circle(g["pos"] + sun * sz * 0.35, sz * 0.65, Color(c.r * 1.4, c.g * 1.4, c.b * 1.4, a))
	# the wreck: pieces of the painting, hot, then charred, then gone
	for p in shards:
		var t: float = p["age"] / p["life"]
		var heat: float = clampf(1.0 - t * 3.0, 0.0, 1.0) * p["burn"]
		var char_ := clampf(t * 1.6, 0.0, 1.0)
		var shade := lerpf(1.0, 0.42, char_)
		var col := Color(shade, shade * 0.95, shade * 0.9).lerp(Color(1.0, 0.62, 0.35), heat * 0.55)
		col.a = clampf((1.0 - t) / 0.35, 0.0, 1.0)
		var xf := Transform2D(p["angle"], p["pos"])
		draw_polygon(xf * (p["poly"] as PackedVector2Array), PackedColorArray([col]), p["uvs"], p["tex"])
	for w in shockwaves:
		var t: float = w["age"] / w["life"]
		# a soft-edged ring of light: three widths, faint
		for k in 3:
			draw_arc(w["pos"], w["size"] * t, 0, TAU, 64, Color(1, 0.85, 0.6, (1.0 - t) * 0.07), (14.0 - k * 4.5) * (1.0 - t) + 2.0, true)
	for r in ripples:
		_draw_ripple(r)
	for b in beams:
		# the hot core, so a beam reads over bright worlds too; its glow is
		# on the additive layer (_draw_glow)
		var c: Color = b["color"]
		draw_line(b["a"], b["b"], c.lerp(Color.WHITE, 0.6), 2.0, true)


## Laser light: a wide soft halo, a bright body and a white-hot thread,
## flickering a little, with a glow at the emitter and a splash where it
## burns.
func _draw_beams(soft: Texture2D) -> void:
	var tex := beam_texture()
	for b in beams:
		var c: Color = b["color"]
		var d: Vector2 = b["b"] - b["a"]
		var flick := randf_range(0.85, 1.0)
		glow.draw_set_transform(b["a"], d.angle())
		glow.draw_texture_rect(tex, Rect2(0.0, -15.0 * flick, d.length(), 30.0 * flick), false, Color(c, 0.22))
		glow.draw_texture_rect(tex, Rect2(0.0, -6.0 * flick, d.length(), 12.0 * flick), false, Color(c, 0.6))
		glow.draw_texture_rect(tex, Rect2(0.0, -2.0, d.length(), 4.0), false, Color(c.lerp(Color.WHITE, 0.7), 1.0))
		glow.draw_set_transform(Vector2.ZERO)
		var r := 16.0 * flick
		glow.draw_texture_rect(soft, Rect2(b["b"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(c, 0.8))
		glow.draw_texture_rect(soft, Rect2(b["b"] - Vector2(r, r) * 0.45, Vector2(r, r) * 0.9), false, Color(1, 1, 1, 0.9))
		glow.draw_texture_rect(soft, Rect2(b["a"] - Vector2(8, 8), Vector2(16, 16)), false, Color(c, 0.6))


func _draw_ripple(r: Dictionary) -> void:
	var t: float = r["age"] / r["life"]
	var c: Color = r["color"]
	if r["kind"] == "ping":
		draw_arc(r["pos"], r["size"] * t, 0, TAU, 96, Color(c, 0.35 * (1.0 - t)), 2.0, true)
		return
	# three wavefronts, each a wobbling ring, the first fastest
	for k in 3:
		var tk := clampf(t * (1.0 - k * 0.18) - k * 0.04, 0.0, 1.0)
		if tk <= 0.0:
			continue
		var rad: float = r["size"] * (1.0 - pow(1.0 - tk, 2.2))
		var pts := PackedVector2Array()
		var n := 72
		for i in n + 1:
			var a := TAU * i / n
			var wob := 1.0 + 0.025 * sin(a * 7.0 + r["age"] * 9.0 + k) + 0.015 * sin(a * 13.0 - r["age"] * 5.0)
			pts.append(r["pos"] + Vector2(cos(a), sin(a)) * rad * wob)
		var fade := (1.0 - tk) * (0.5 - k * 0.12)
		draw_polyline(pts, Color(c, fade * 0.35), 10.0 * (1.0 - tk) + 2.0, true)
		draw_polyline(pts, Color(1, 1, 1, fade * 0.5), 1.5, true)


## Light: fireballs, burning shards, embers and the coolant's glow.
func _draw_glow() -> void:
	var soft := soft_texture()
	for e in explosions:
		if e["age"] < 0.0:
			continue
		var t: float = e["age"] / e["life"]
		var r: float = e["size"] * (0.5 + 1.6 * sqrt(t))
		var cool: Color = e["cool"]
		var hot: Color = e["hot"]
		var k := (1.0 - t) * (1.0 - t)
		glow.draw_texture_rect(soft, Rect2(e["pos"] - Vector2(r, r) * 1.6, Vector2(r, r) * 3.2), false, Color(cool.r, cool.g, cool.b, 0.45 * k))
		glow.draw_texture_rect(soft, Rect2(e["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(hot.r, hot.g, hot.b, 0.9 * k))
		var c := r * 0.45 * (1.0 - t)
		glow.draw_texture_rect(soft, Rect2(e["pos"] - Vector2(c, c), Vector2(c, c) * 2.0), false, Color(1, 1, 0.95, k))
	for p in shards:
		var heat: float = clampf(1.0 - p["age"] / p["life"] * 3.0, 0.0, 1.0) * p["burn"]
		if heat <= 0.0:
			continue
		var r: float = 6.0 + (p["poly"] as PackedVector2Array)[0].length() * 0.6
		glow.draw_texture_rect(soft, Rect2(p["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(1.0, 0.5, 0.2, 0.35 * heat))
	for d in droplets:
		if d["age"] < 0.0 or d["age"] > d["wet"]:
			continue
		var r: float = d["size"]
		var tail: Vector2 = d["vel"] * 0.03
		glow.draw_line(d["pos"] - tail, d["pos"], Color(0.4, 1.0, 0.6, 0.5), r)
		glow.draw_texture_rect(soft, Rect2(d["pos"] - Vector2(r, r) * 2.0, Vector2(r, r) * 4.0), false, Color(0.55, 1.0, 0.65, 0.9))
	for p in puffs:
		if p["age"] < 0.0 or p["age"] > p["hot"]:
			continue
		var t: float = p["age"] / p["hot"]
		var r: float = p["size"] * (0.7 + 0.6 * t)
		var c: Color = p["color"]
		glow.draw_texture_rect(soft, Rect2(p["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(c, 0.55 * (1.0 - t)))
	for c in crackles:
		_draw_crackle(soft, c)
	_draw_beams(soft)
	for m in muzzles:
		_draw_muzzle(soft, m)
	for s in sparks:
		# an ember: a soft streak along its flight, not a hard stick
		var t: float = s["age"] / s["life"]
		var c: Color = s["color"]
		c.a = 1.0 - t
		var v: Vector2 = s["vel"]
		var len := clampf(v.length() * 0.04, 2.0, 16.0)
		glow.draw_set_transform(s["pos"], v.angle())
		glow.draw_texture_rect(soft, Rect2(-len, -1.6, len + 1.6, 3.2), false, c)
		glow.draw_texture_rect(soft, Rect2(-1.4, -1.4, 2.8, 2.8), false, Color(1, 1, 0.9, c.a))
	glow.draw_set_transform(Vector2.ZERO)


## The charge a disruptor leaves on a hull: forks of light that flicker
## out from the hit, drawn as chains of soft sparks (no hard lines).
func _draw_crackle(soft: Texture2D, c: Dictionary) -> void:
	var t: float = c["age"] / c["life"]
	var col: Color = c["color"]
	var rng := RandomNumberGenerator.new()
	# a new pattern every 50 ms, so the forks dance
	rng.seed = int(c["seed"]) + int(c["age"] * 20.0)
	var size: float = c["size"]
	glow.draw_texture_rect(soft, Rect2(c["pos"] - Vector2(size, size) * 0.6, Vector2(size, size) * 1.2), false, Color(col, 0.25 * (1.0 - t)))
	for f in 4:
		var p: Vector2 = c["pos"]
		var a := rng.randf() * TAU
		var step := size * 0.12
		for k in 7:
			a += rng.randf_range(-0.9, 0.9)
			p += Vector2(cos(a), sin(a)) * step
			var w := size * 0.07 * (1.0 - k / 8.0)
			glow.draw_texture_rect(soft, Rect2(p - Vector2(w, w) * 1.5, Vector2(w, w) * 3.0), false, Color(col.lerp(Color.WHITE, 0.4), 0.8 * (1.0 - t)))


func _draw_muzzle(soft: Texture2D, m: Dictionary) -> void:
	match m["kind"]:
		"rail":
			_draw_rail_muzzle(soft, m)
			return
	var t: float = m["age"] / m["life"]
	var k := (1.0 - t) * (1.0 - t)
	var sz: float = m["size"]
	var c: Color = m["color"]
	glow.draw_set_transform(m["pos"], m["ang"])
	match m["kind"]:
		"cannon":
			# a fireball out of the barrel, a forward tongue of flame
			var tongue := sz * (2.0 + 3.0 * t)
			glow.draw_texture_rect(soft, Rect2(-sz * 0.3, -sz * 0.9, tongue, sz * 1.8), false, Color(1.0, 0.55, 0.2, 0.8 * k))
			glow.draw_texture_rect(soft, Rect2(-sz * 0.2, -sz * 0.45, tongue * 0.6, sz * 0.9), false, Color(1.0, 0.9, 0.6, k))
			var f := sz * (1.6 - 0.5 * t)
			glow.draw_texture_rect(soft, Rect2(-f, -f, f * 2.0, f * 2.0), false, Color(1.0, 0.85, 0.6, k))
		"pdc":
			var f := sz * 1.2
			glow.draw_texture_rect(soft, Rect2(-f * 0.5, -f * 0.4, f * 2.2, f * 0.8), false, Color(1.0, 0.85, 0.5, k))
			glow.draw_texture_rect(soft, Rect2(-f * 0.5, -f * 0.5, f, f), false, Color(1, 1, 0.9, k))
		"energy":
			var f := sz * (1.2 + 1.2 * t)
			glow.draw_texture_rect(soft, Rect2(-f, -f, f * 2.0, f * 2.0), false, Color(c, 0.7 * k))
			glow.draw_texture_rect(soft, Rect2(-f * 0.4, -f * 0.4, f * 0.8, f * 0.8), false, Color(1, 1, 1, k))
		"launch":
			# the tube's hatch flash and the motor catching
			var f := sz * (1.0 - 0.4 * t)
			glow.draw_texture_rect(soft, Rect2(-f * 1.6, -f * 0.6, f * 2.0, f * 1.2), false, Color(c, 0.8 * k))
			glow.draw_texture_rect(soft, Rect2(-f * 0.6, -f * 0.6, f * 1.2, f * 1.2), false, Color(1, 1, 0.95, 0.8 * k))
	glow.draw_set_transform(Vector2.ZERO)


func _draw_rail_muzzle(soft: Texture2D, m: Dictionary) -> void:
	var t: float = m["age"] / m["life"]
	var k := (1.0 - t) * (1.0 - t)
	var sz: float = m["size"]
	var c: Color = m["color"]
	glow.draw_set_transform(m["pos"], m["ang"])
	# the jet, stretching out ahead as it fades
	var jet := sz * (2.5 + 5.0 * t)
	glow.draw_texture_rect(soft, Rect2(-sz * 0.4, -sz * 0.7, jet, sz * 1.4), false, Color(c.r, c.g, c.b, 0.75 * k))
	glow.draw_texture_rect(soft, Rect2(-sz * 0.2, -sz * 0.3, jet * 0.6, sz * 0.6), false, Color(1, 1, 1, 0.9 * k))
	# gas venting sideways out of the rails
	var vent := sz * (0.8 + 1.6 * t)
	for side in [-1.0, 1.0]:
		glow.draw_texture_rect(soft, Rect2(-sz * 0.9 - vent * 0.4, side * (sz * 0.2 + vent * 0.5) - vent * 0.4, vent * 0.8, vent * 0.8), false, Color(c.r, c.g, c.b, 0.35 * k))
	# the white flash itself
	var f := sz * (1.4 - 0.6 * t)
	glow.draw_texture_rect(soft, Rect2(-f, -f, f * 2.0, f * 2.0), false, Color(1, 1, 1, k))
	glow.draw_set_transform(Vector2.ZERO)
