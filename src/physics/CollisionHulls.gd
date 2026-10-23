## CollisionHulls -- the convex collision polygon of a painted hull, traced
## from the sprite's alpha once per texture and scaled to the model's rect,
## so a torpedo that misses the painting misses the ship.
class_name CollisionHulls
extends RefCounted

const MAX_VERTS := 14
static var _outlines := {}     # texture path -> PackedVector2Array in 0..1 texture space


## Local-space convex hull for a ship design (a hull name or a shipyard
## design dictionary), in the same frame ShipArt draws it.
static func for_ship(design: Variant, race_id: int, size: float) -> PackedVector2Array:
	var model := ShipArt.build_model(design, race_id, size)
	var rect: Rect2 = model["rect"]
	var tex: Texture2D = model["tex"]
	var unit := PackedVector2Array()
	if tex != null:
		unit = _outline(tex)
	if unit.is_empty():
		return ellipse(rect)
	var out := PackedVector2Array()
	for u in unit:
		out.append(rect.position + Vector2(u.x * rect.size.x, u.y * rect.size.y))
	return out


## A fallback shape when no painting is available.
static func ellipse(rect: Rect2) -> PackedVector2Array:
	var out := PackedVector2Array()
	var c := rect.get_center()
	for i in 12:
		var a := TAU * i / 12.0
		out.append(c + Vector2(cos(a) * rect.size.x * 0.5, sin(a) * rect.size.y * 0.5))
	return out


static func _outline(tex: Texture2D) -> PackedVector2Array:
	var key := tex.resource_path
	if key.is_empty():
		key = str(tex.get_instance_id())
	if _outlines.has(key):
		return _outlines[key]
	var pts := PackedVector2Array()
	var img: Image = tex.get_image()
	if img != null and not img.is_empty():
		if img.is_compressed():
			img.decompress()
		var w := img.get_width()
		var h := img.get_height()
		var step := maxi(1, w / 48)
		# the top and bottom opaque pixel of every sampled column, and the
		# ends of every sampled row, are enough for a convex hull
		for x in range(0, w, step):
			var top := -1
			var bottom := -1
			for y in h:
				if img.get_pixel(x, y).a > 0.4:
					if top < 0:
						top = y
					bottom = y
			if top >= 0:
				pts.append(Vector2(x, top))
				pts.append(Vector2(x, bottom + 1))
		for y in range(0, h, step):
			var left := -1
			var right := -1
			for x in w:
				if img.get_pixel(x, y).a > 0.4:
					if left < 0:
						left = x
					right = x
			if left >= 0:
				pts.append(Vector2(left, y))
				pts.append(Vector2(right + 1, y))
		if pts.size() >= 3:
			pts = _simplify(Geometry2D.convex_hull(pts))
			for i in pts.size():
				pts[i] = Vector2(pts[i].x / w, pts[i].y / h)
	_outlines[key] = pts
	return pts


## Keep at most MAX_VERTS vertices: the farthest point of each angular
## sector round the centroid, hulled again.
static func _simplify(hull: PackedVector2Array) -> PackedVector2Array:
	if hull.size() > 1 and hull[0].is_equal_approx(hull[hull.size() - 1]):
		hull.remove_at(hull.size() - 1)
	if hull.size() <= MAX_VERTS:
		return hull
	var c := Vector2.ZERO
	for p in hull:
		c += p
	c /= hull.size()
	var best := {}
	for p in hull:
		var sector := int(floor((p - c).angle() / TAU * MAX_VERTS + MAX_VERTS)) % MAX_VERTS
		var d := (p - c).length_squared()
		if not best.has(sector) or d > best[sector][1]:
			best[sector] = [p, d]
	var picked := PackedVector2Array()
	for k in best:
		picked.append(best[k][0])
	var out := Geometry2D.convex_hull(picked)
	if out.size() > 1 and out[0].is_equal_approx(out[out.size() - 1]):
		out.remove_at(out.size() - 1)
	return out
