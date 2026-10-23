## ShipArt -- procedural hull silhouettes.  Each race has a design
## language: Terran hulls are long and plated, Drengin hulls are angular
## blades, Arcean hulls are swept and organic, Pirates fly patched-up
## Terran hulls.  Ships point along +X.
class_name ShipArt
extends RefCounted


static func hull(design: String, race_id: int, size: float) -> PackedVector2Array:
	var pts := PackedVector2Array()
	var s := size
	match design:
		"Starbase":
			return _station(s)
		"Freighter":
			pts = PackedVector2Array([Vector2(1.0, 0), Vector2(0.7, -0.35), Vector2(-0.2, -0.45), Vector2(-0.9, -0.3), Vector2(-1.0, 0), Vector2(-0.9, 0.3), Vector2(-0.2, 0.45), Vector2(0.7, 0.35)])
		_:
			match race_id:
				0:
					pts = _arcean(design)
				2:
					pts = _drengin(design)
				_:
					pts = _terran(design)
	for i in pts.size():
		pts[i] *= s
	return pts


static func _terran(design: String) -> PackedVector2Array:
	match design:
		"Scout":
			return PackedVector2Array([Vector2(1.1, 0), Vector2(0.3, -0.25), Vector2(-0.3, -0.6), Vector2(-0.8, -0.6), Vector2(-0.6, -0.15), Vector2(-0.9, 0), Vector2(-0.6, 0.15), Vector2(-0.8, 0.6), Vector2(-0.3, 0.6), Vector2(0.3, 0.25)])
		"Destroyer", "Raider":
			return PackedVector2Array([Vector2(1.2, 0), Vector2(0.6, -0.2), Vector2(0.1, -0.3), Vector2(-0.2, -0.7), Vector2(-0.9, -0.7), Vector2(-0.7, -0.25), Vector2(-1.0, 0), Vector2(-0.7, 0.25), Vector2(-0.9, 0.7), Vector2(-0.2, 0.7), Vector2(0.1, 0.3), Vector2(0.6, 0.2)])
		"Cruiser":
			return PackedVector2Array([Vector2(1.3, 0), Vector2(0.9, -0.22), Vector2(0.3, -0.3), Vector2(0.2, -0.55), Vector2(-0.6, -0.6), Vector2(-0.9, -0.35), Vector2(-1.0, 0), Vector2(-0.9, 0.35), Vector2(-0.6, 0.6), Vector2(0.2, 0.55), Vector2(0.3, 0.3), Vector2(0.9, 0.22)])
		"Battleship":
			return PackedVector2Array([Vector2(1.3, 0), Vector2(1.0, -0.3), Vector2(0.4, -0.35), Vector2(0.3, -0.65), Vector2(-0.5, -0.7), Vector2(-0.8, -0.45), Vector2(-1.0, -0.2), Vector2(-1.0, 0.2), Vector2(-0.8, 0.45), Vector2(-0.5, 0.7), Vector2(0.3, 0.65), Vector2(0.4, 0.35), Vector2(1.0, 0.3)])
		"Dreadnought":
			return PackedVector2Array([Vector2(1.4, 0), Vector2(1.1, -0.25), Vector2(0.5, -0.3), Vector2(0.5, -0.7), Vector2(-0.3, -0.8), Vector2(-0.7, -0.5), Vector2(-1.0, -0.4), Vector2(-1.0, 0.4), Vector2(-0.7, 0.5), Vector2(-0.3, 0.8), Vector2(0.5, 0.7), Vector2(0.5, 0.3), Vector2(1.1, 0.25)])
		"Marauder":
			return PackedVector2Array([Vector2(1.2, 0), Vector2(0.5, -0.35), Vector2(-0.1, -0.35), Vector2(-0.4, -0.75), Vector2(-0.9, -0.6), Vector2(-0.8, -0.2), Vector2(-1.0, 0), Vector2(-0.8, 0.2), Vector2(-0.9, 0.6), Vector2(-0.4, 0.75), Vector2(-0.1, 0.35), Vector2(0.5, 0.35)])
		"BattleCruiser":
			return PackedVector2Array([Vector2(1.35, 0), Vector2(1.0, -0.2), Vector2(0.5, -0.4), Vector2(0.2, -0.75), Vector2(-0.5, -0.75), Vector2(-0.7, -0.35), Vector2(-1.0, -0.15), Vector2(-1.0, 0.15), Vector2(-0.7, 0.35), Vector2(-0.5, 0.75), Vector2(0.2, 0.75), Vector2(0.5, 0.4), Vector2(1.0, 0.2)])
	return PackedVector2Array([Vector2(1, 0), Vector2(-0.8, -0.6), Vector2(-0.5, 0), Vector2(-0.8, 0.6)])


static func _drengin(design: String) -> PackedVector2Array:
	match design:
		"Scout":
			return PackedVector2Array([Vector2(1.2, 0), Vector2(0.0, -0.3), Vector2(-0.5, -0.8), Vector2(-0.7, -0.5), Vector2(-0.9, 0), Vector2(-0.7, 0.5), Vector2(-0.5, 0.8), Vector2(0.0, 0.3)])
		"Destroyer", "Raider":
			return PackedVector2Array([Vector2(1.3, 0), Vector2(0.4, -0.25), Vector2(0.0, -0.8), Vector2(-0.6, -0.9), Vector2(-0.6, -0.3), Vector2(-1.0, 0), Vector2(-0.6, 0.3), Vector2(-0.6, 0.9), Vector2(0.0, 0.8), Vector2(0.4, 0.25)])
		"Cruiser":
			return PackedVector2Array([Vector2(1.4, 0), Vector2(0.6, -0.3), Vector2(0.4, -0.7), Vector2(-0.4, -0.9), Vector2(-0.8, -0.5), Vector2(-1.0, 0), Vector2(-0.8, 0.5), Vector2(-0.4, 0.9), Vector2(0.4, 0.7), Vector2(0.6, 0.3)])
		"Battleship":
			return PackedVector2Array([Vector2(1.4, 0), Vector2(0.8, -0.35), Vector2(0.6, -0.9), Vector2(-0.2, -1.0), Vector2(-0.7, -0.6), Vector2(-1.0, -0.2), Vector2(-1.0, 0.2), Vector2(-0.7, 0.6), Vector2(-0.2, 1.0), Vector2(0.6, 0.9), Vector2(0.8, 0.35)])
		"Dreadnought":
			return PackedVector2Array([Vector2(1.5, 0), Vector2(0.9, -0.3), Vector2(0.9, -0.9), Vector2(0.0, -1.1), Vector2(-0.6, -0.7), Vector2(-1.0, -0.35), Vector2(-1.0, 0.35), Vector2(-0.6, 0.7), Vector2(0.0, 1.1), Vector2(0.9, 0.9), Vector2(0.9, 0.3)])
		"Marauder":
			return PackedVector2Array([Vector2(1.3, 0), Vector2(0.2, -0.4), Vector2(-0.3, -1.0), Vector2(-0.8, -0.7), Vector2(-1.0, 0), Vector2(-0.8, 0.7), Vector2(-0.3, 1.0), Vector2(0.2, 0.4)])
		"BattleCruiser":
			return PackedVector2Array([Vector2(1.45, 0), Vector2(0.7, -0.3), Vector2(0.5, -0.95), Vector2(-0.3, -0.95), Vector2(-0.7, -0.5), Vector2(-1.0, -0.15), Vector2(-1.0, 0.15), Vector2(-0.7, 0.5), Vector2(-0.3, 0.95), Vector2(0.5, 0.95), Vector2(0.7, 0.3)])
	return _terran(design)


static func _arcean(design: String) -> PackedVector2Array:
	match design:
		"Scout":
			return _smooth([Vector2(1.1, 0), Vector2(0.2, -0.45), Vector2(-0.6, -0.55), Vector2(-0.9, -0.2), Vector2(-0.9, 0.2), Vector2(-0.6, 0.55), Vector2(0.2, 0.45)])
		"Destroyer", "Raider":
			return _smooth([Vector2(1.2, 0), Vector2(0.5, -0.4), Vector2(-0.3, -0.75), Vector2(-0.9, -0.5), Vector2(-1.0, 0), Vector2(-0.9, 0.5), Vector2(-0.3, 0.75), Vector2(0.5, 0.4)])
		"Cruiser":
			return _smooth([Vector2(1.3, 0), Vector2(0.7, -0.45), Vector2(-0.2, -0.7), Vector2(-0.8, -0.55), Vector2(-1.0, 0), Vector2(-0.8, 0.55), Vector2(-0.2, 0.7), Vector2(0.7, 0.45)])
		"Battleship":
			return _smooth([Vector2(1.3, 0), Vector2(0.8, -0.55), Vector2(0.0, -0.8), Vector2(-0.7, -0.7), Vector2(-1.0, -0.2), Vector2(-1.0, 0.2), Vector2(-0.7, 0.7), Vector2(0.0, 0.8), Vector2(0.8, 0.55)])
		"Dreadnought":
			return _smooth([Vector2(1.4, 0), Vector2(0.9, -0.6), Vector2(0.1, -0.95), Vector2(-0.6, -0.85), Vector2(-1.0, -0.3), Vector2(-1.0, 0.3), Vector2(-0.6, 0.85), Vector2(0.1, 0.95), Vector2(0.9, 0.6)])
		"Marauder":
			return _smooth([Vector2(1.2, 0), Vector2(0.3, -0.6), Vector2(-0.5, -0.85), Vector2(-0.95, -0.4), Vector2(-0.95, 0.4), Vector2(-0.5, 0.85), Vector2(0.3, 0.6)])
		"BattleCruiser":
			return _smooth([Vector2(1.35, 0), Vector2(0.8, -0.5), Vector2(0.0, -0.9), Vector2(-0.7, -0.75), Vector2(-1.0, -0.2), Vector2(-1.0, 0.2), Vector2(-0.7, 0.75), Vector2(0.0, 0.9), Vector2(0.8, 0.5)])
	return _terran(design)


static func _station(s: float) -> PackedVector2Array:
	var pts := PackedVector2Array()
	var n := 12
	for i in n:
		var a := TAU * i / n
		var r := s * (1.0 if i % 2 == 0 else 0.72)
		pts.append(Vector2(cos(a), sin(a)) * r)
	return pts


## Chaikin smoothing for the organic Arcean hulls.
static func _smooth(src: Array) -> PackedVector2Array:
	var pts := PackedVector2Array(src)
	for _pass in 2:
		var out := PackedVector2Array()
		for i in pts.size():
			var a := pts[i]
			var b := pts[(i + 1) % pts.size()]
			out.append(a.lerp(b, 0.25))
			out.append(a.lerp(b, 0.75))
		pts = out
	return pts


## Engine nozzle positions (local, before scaling).
static func nozzles(design: String, size: float) -> Array:
	match design:
		"Starbase":
			return []
		"Scout":
			return [Vector2(-0.85, 0) * size]
		"Freighter":
			return [Vector2(-0.95, -0.15) * size, Vector2(-0.95, 0.15) * size]
		"Dreadnought", "Battleship", "BattleCruiser":
			return [Vector2(-0.95, -0.3) * size, Vector2(-0.95, 0.3) * size, Vector2(-0.95, 0) * size]
	return [Vector2(-0.9, -0.25) * size, Vector2(-0.9, 0.25) * size]


static func draw_ship(c: CanvasItem, design: String, race_id: int, size: float, base: Color, thrusting: float, cloak_alpha: float, damage: float) -> void:
	var pts := hull(design, race_id, size)
	var fill := base.darkened(0.55)
	fill.a = cloak_alpha
	var line := base
	line.a = cloak_alpha
	# engine glow
	if thrusting > 0.01 and cloak_alpha > 0.05:
		for nz in nozzles(design, size):
			var l := size * (0.8 + 0.8 * thrusting) * randf_range(0.85, 1.15)
			var flame := PackedVector2Array([nz + Vector2(0, -size * 0.18), nz + Vector2(-l, 0), nz + Vector2(0, size * 0.18)])
			var fc := Color(1.0, 0.75, 0.3, 0.75 * cloak_alpha)
			c.draw_colored_polygon(flame, fc)
			c.draw_colored_polygon(PackedVector2Array([nz + Vector2(0, -size * 0.08), nz + Vector2(-l * 0.55, 0), nz + Vector2(0, size * 0.08)]), Color(1, 1, 0.85, 0.9 * cloak_alpha))
	c.draw_colored_polygon(pts, fill)
	# inner plating
	var inner := PackedVector2Array()
	for p in pts:
		inner.append(p * 0.62)
	var inner_col := base.darkened(0.25)
	inner_col.a = cloak_alpha * 0.9
	c.draw_colored_polygon(inner, inner_col)
	if design == "Starbase":
		c.draw_circle(Vector2.ZERO, size * 0.35, base.darkened(0.6))
		c.draw_arc(Vector2.ZERO, size * 0.35, 0, TAU, 32, line, 2.0)
		c.draw_arc(Vector2.ZERO, size * 0.5, 0, TAU, 32, line.darkened(0.3), 1.5)
	var closed := PackedVector2Array(pts)
	closed.append(pts[0])
	c.draw_polyline(closed, line, 2.0, true)
	# cockpit / bridge light
	var bridge := Color(0.85, 0.95, 1.0, cloak_alpha)
	c.draw_circle(Vector2(size * 0.45, 0) if design != "Starbase" else Vector2.ZERO, maxf(size * 0.09, 2.0), bridge)
	# damage: scorch marks
	if damage > 0.3:
		var rng := RandomNumberGenerator.new()
		rng.seed = int(size * 7.0) + race_id
		var marks := int(damage * 6.0)
		for i in marks:
			var p := pts[rng.randi() % pts.size()] * rng.randf_range(0.3, 0.9)
			c.draw_circle(p, size * rng.randf_range(0.08, 0.18), Color(0.05, 0.03, 0.02, 0.8 * cloak_alpha))
