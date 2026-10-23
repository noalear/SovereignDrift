## Fx -- explosions, sparks, beams and the burning break-up of a wrecked
## hull, all drawn by one node to keep the scene tree small.  Nothing here
## collides with anything.
class_name Fx
extends Node2D

var explosions: Array = []   # {pos, age, life, size, color}
var sparks: Array = []       # {pos, vel, age, life, color}
var beams: Array = []        # {a, b, color, age}
var shockwaves: Array = []
var wrecks: Array = []       # {pieces: [{poly, pos, vel, angle, spin}], age, life, color}
const WRECK_LIFE := 3.2


func explosion(pos: Vector2, size: float, color: Color, big: bool = false, sound: String = "") -> void:
	explosions.append({"pos": pos, "age": 0.0, "life": 0.9 if not big else 1.8, "size": size, "color": color})
	if big:
		shockwaves.append({"pos": pos, "age": 0.0, "life": 1.2, "size": size * 4.0})
	var n := 10 if not big else 36
	for i in n:
		var a := randf() * TAU
		var sp := randf_range(60.0, 260.0 if not big else 520.0)
		sparks.append({"pos": pos, "vel": Vector2(cos(a), sin(a)) * sp, "age": 0.0, "life": randf_range(0.4, 1.2), "color": color.lerp(Color(1, 0.8, 0.4), randf())})
	if sound.is_empty():
		sound = "bigexplode" if big else "explode"
	Sfx.play(sound, pos, 1.0 if big else 0.85, clampf(1.25 - size / 160.0, 0.75, 1.15))


## A dead hull comes apart: its outline is cut into a fan of slivers that
## drift slowly from the ship, glow, char and fade out in a few seconds.
## Pure drawing, no bodies, so nothing is thrown across the system.
func wreck(hull: PackedVector2Array, pos: Vector2, vel: Vector2, angle: float, spin: float, color: Color) -> void:
	var n := hull.size()
	if n < 3:
		return
	var slices := 5 + (3 if n >= 10 else 0)
	var per := maxi(n / slices, 2)
	var pieces: Array = []
	var i := 0
	while i < n:
		var j := mini(i + per, n)
		if n - j < 2:
			j = n
		var pts := PackedVector2Array([Vector2.ZERO])
		for k in range(i, j + 1):
			pts.append(hull[k % n])
		var c := Vector2.ZERO
		for q in pts:
			c += q
		c /= pts.size()
		var local := PackedVector2Array()
		for q in pts:
			local.append(q - c)
		var off := c.rotated(angle)
		var drift := off.normalized() * randf_range(12.0, 36.0) + Vector2(randf_range(-8, 8), randf_range(-8, 8))
		pieces.append({"poly": local, "pos": pos + off, "vel": vel + drift, "angle": angle, "spin": spin + randf_range(-1.5, 1.5)})
		i = j
	wrecks.append({"pieces": pieces, "age": 0.0, "life": WRECK_LIFE + randf_range(-0.4, 0.6), "color": color})
	# embers from the seams
	for p in pieces:
		for e in 3:
			var a := randf() * TAU
			sparks.append({"pos": p["pos"], "vel": vel * 0.5 + Vector2(cos(a), sin(a)) * randf_range(20.0, 90.0), "age": 0.0, "life": randf_range(0.8, 2.0), "color": Color(1.0, randf_range(0.45, 0.7), 0.2)})


func spark(pos: Vector2, count: int, color: Color) -> void:
	for i in count:
		var a := randf() * TAU
		sparks.append({"pos": pos, "vel": Vector2(cos(a), sin(a)) * randf_range(40.0, 160.0), "age": 0.0, "life": randf_range(0.2, 0.5), "color": color})


func beam(a: Vector2, b: Vector2, color: Color) -> void:
	beams.append({"a": a, "b": b, "color": color, "age": 0.0})


func _process(delta: float) -> void:
	for e in explosions:
		e["age"] += delta
	explosions = explosions.filter(func(e): return e["age"] < e["life"])
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
	for w in wrecks:
		w["age"] += delta
		for p in w["pieces"]:
			p["pos"] += p["vel"] * delta
			p["vel"] *= 1.0 - 0.6 * delta
			p["angle"] += p["spin"] * delta
			p["spin"] *= 1.0 - 0.8 * delta
	wrecks = wrecks.filter(func(w): return w["age"] < w["life"])
	queue_redraw()


func _draw() -> void:
	for w in wrecks:
		var t: float = w["age"] / w["life"]
		var fade := 1.0 - t * t
		var base: Color = w["color"]
		# hot at first, then charred, then gone
		var heat := clampf(1.0 - t * 2.5, 0.0, 1.0)
		var fill := base.darkened(0.55).lerp(Color(1.0, 0.6, 0.25), heat * 0.8)
		fill.a = fade
		var edge := base.lerp(Color(1.0, 0.85, 0.5), heat)
		edge.a = fade
		for p in w["pieces"]:
			var xf := Transform2D(p["angle"], p["pos"])
			var poly: PackedVector2Array = xf * p["poly"]
			draw_colored_polygon(poly, fill)
			var closed := PackedVector2Array(poly)
			closed.append(poly[0])
			draw_polyline(closed, edge, 1.2, true)
			if heat > 0.0:
				draw_circle(p["pos"], 3.0 + 6.0 * heat, Color(1.0, 0.7, 0.3, 0.35 * heat * fade))
	for w in shockwaves:
		var t: float = w["age"] / w["life"]
		draw_arc(w["pos"], w["size"] * t, 0, TAU, 64, Color(1, 0.9, 0.7, (1.0 - t) * 0.5), 6.0 * (1.0 - t) + 1.0, true)
	for e in explosions:
		var t: float = e["age"] / e["life"]
		var r: float = e["size"] * (0.3 + 1.4 * sqrt(t))
		var c: Color = e["color"]
		draw_circle(e["pos"], r, Color(c.r, c.g, c.b, (1.0 - t) * 0.35))
		draw_circle(e["pos"], r * 0.6, Color(1.0, 0.75, 0.3, (1.0 - t) * 0.7))
		draw_circle(e["pos"], r * 0.3 * (1.0 - t), Color(1, 1, 0.9, (1.0 - t)))
	for s in sparks:
		var t: float = s["age"] / s["life"]
		var c: Color = s["color"]
		c.a = 1.0 - t
		draw_line(s["pos"], s["pos"] - s["vel"] * 0.04, c, 2.0)
	for b in beams:
		var c: Color = b["color"]
		draw_line(b["a"], b["b"], Color(c.r, c.g, c.b, 0.35), 7.0)
		draw_line(b["a"], b["b"], c, 3.0)
		draw_line(b["a"], b["b"], Color.WHITE, 1.2)
		draw_circle(b["b"], 5.0, Color.WHITE)
