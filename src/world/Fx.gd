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
var glow: Node2D             # additive layer
const WRECK_LIFE := 3.2

static var _soft: Texture2D = null


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
	for d in droplets:
		var vt: float = d["age"] - d["wet"]
		if vt <= 0.0:
			continue
		var t: float = vt / d["life"]
		var r: float = d["size"] * (2.0 + 9.0 * sqrt(t))
		draw_texture_rect(soft, Rect2(d["pos"] - Vector2(r, r), Vector2(r, r) * 2.0), false, Color(0.75, 1.0, 0.85, 0.22 * (1.0 - t)))
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
	for b in beams:
		var c: Color = b["color"]
		draw_line(b["a"], b["b"], Color(c.r, c.g, c.b, 0.35), 7.0)
		draw_line(b["a"], b["b"], c, 3.0)
		draw_line(b["a"], b["b"], Color.WHITE, 1.2)
		draw_circle(b["b"], 5.0, Color.WHITE)


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
	for s in sparks:
		var t: float = s["age"] / s["life"]
		var c: Color = s["color"]
		c.a = 1.0 - t
		glow.draw_line(s["pos"], s["pos"] - s["vel"] * 0.04, c, 2.0)
