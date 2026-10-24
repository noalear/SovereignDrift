## ShieldBubble -- the shield drawn around a whole ship: faint, soft-edged
## and the same colour for everyone.  How bright it is, the cells still lit
## and a flicker near the end show how strong it is; hits ripple out from
## where they land and it shatters when it goes down.
class_name ShieldBubble
extends Node2D

static var _shader: Shader = null
static var _white: ImageTexture = null

var ship: Ship
var half := Vector2(40, 20)         # half size of the bubble, ship space
var hits: Array = []                # [point (-1..1), age, weight]
var collapse := 1.0
var was_up := true
var last_shield := -1.0
var mat: ShaderMaterial


func setup(p_ship: Ship) -> void:
	ship = p_ship
	if _shader == null:
		_shader = load("res://src/ships/shield.gdshader")
		var img := Image.create(4, 4, false, Image.FORMAT_RGBA8)
		img.fill(Color.WHITE)
		_white = ImageTexture.create_from_image(img)
	mat = ShaderMaterial.new()
	mat.shader = _shader
	material = mat
	fit()


## Size the bubble so the whole hull painting sits inside it, with room
## around it: an ellipse around the painted pixels, centred on them.
func fit() -> void:
	var fitted: Array = ship.art_model.get("shield_fit", [])
	if fitted.is_empty():
		fitted = _fit_painting()
		if not ship.art_model.is_empty():
			ship.art_model["shield_fit"] = fitted   # models are shared, so this is worked out once per hull
	position = fitted[0]
	half = fitted[1]
	var longest := maxf(half.x, half.y)
	mat.set_shader_parameter("aspect", half / longest)
	queue_redraw()


func _fit_painting() -> Array:
	var rect: Rect2 = ship.art_model.get("rect", Rect2(-ship.size, -ship.size, ship.size * 2.0, ship.size * 2.0))
	var mask: Dictionary = ship.art_model.get("mask", {})
	var pts := PackedVector2Array()
	if not mask.is_empty():
		var gw: int = mask["gw"]
		var gh: int = mask["gh"]
		var cell := rect.size / Vector2(gw, gh)
		for y in gh:
			for x in gw:
				if mask["depth"][y * gw + x] > 0.0:
					var c := rect.position + (Vector2(x, y) + Vector2(0.5, 0.5)) * cell
					for corner in [Vector2(-0.5, -0.5), Vector2(0.5, -0.5), Vector2(-0.5, 0.5), Vector2(0.5, 0.5)]:
						pts.append(c + corner * cell)
	if pts.is_empty():
		pts = PackedVector2Array([rect.position, rect.end])
	var lo := pts[0]
	var hi := pts[0]
	for p in pts:
		lo = lo.min(p)
		hi = hi.max(p)
	var centre := (lo + hi) * 0.5
	var box := (hi - lo) * 0.5
	var shape := Vector2.ONE * maxf(box.x, box.y) if ship.is_station else Vector2(box.x, maxf(box.y, box.x * 0.4))
	var k := 0.0
	for p in pts:
		k = maxf(k, ((p - centre) / shape).length())
	return [centre, shape * k * 1.06 + Vector2(8.0, 8.0)]


## A hit on the shield at `angle` (ship space), `weight` 0..1.
func hit(angle: float, weight: float) -> void:
	var d := Vector2(cos(angle), sin(angle)) / half
	var p := d / d.length()
	hits.push_front([p, 0.0, clampf(weight, 0.45, 1.0)])
	if hits.size() > 4:
		hits.pop_back()


func _process(delta: float) -> void:
	if ship == null or ship.dead:
		visible = false
		return
	visible = true
	for h in hits:
		h[1] += delta
	hits = hits.filter(func(h): return h[1] < 0.9)
	var s := ship.shield / maxf(ship.shield_max, 1.0) if ship.shield_max > 0.0 else 0.0
	var up := s > 0.001
	if was_up and not up and ship.shield_max > 0.0:
		collapse = 0.0
	was_up = up
	collapse += delta
	# any drop in the shield is a hit; this also works for puppets online,
	# whose shield arrives in snapshots rather than through take_damage
	if last_shield >= 0.0 and ship.shield < last_shield - 0.5:
		hit(ship.last_hit_angle, (last_shield - ship.shield) / maxf(ship.shield_max * 0.15, 1.0))
	last_shield = ship.shield
	var packed := PackedVector4Array()
	for i in 4:
		if i < hits.size():
			var h: Array = hits[i]
			packed.append(Vector4(h[0].x, h[0].y, h[1], h[2]))
		else:
			packed.append(Vector4(0, 0, 9.0, 0.0))
	mat.set_shader_parameter("hits", packed)
	mat.set_shader_parameter("strength", s)
	mat.set_shader_parameter("collapse", collapse)
	mat.set_shader_parameter("guard", clampf(ship.invulnerable, 0.0, 1.0))
	mat.set_shader_parameter("alpha", ship.cloak_alpha)


func _draw() -> void:
	draw_texture_rect(_white, Rect2(-half, half * 2.0), false)
