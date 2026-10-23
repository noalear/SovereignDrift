## Fx -- explosions, sparks, beams and floating module pickups, all drawn
## by one node to keep the scene tree small.
class_name Fx
extends Node2D

var explosions: Array = []   # {pos, age, life, size, color}
var sparks: Array = []       # {pos, vel, age, life, color}
var beams: Array = []        # {a, b, color, age}
var shockwaves: Array = []


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
	queue_redraw()


func _draw() -> void:
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
