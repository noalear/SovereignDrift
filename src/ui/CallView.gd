## CallView -- draws a video call: the caller in the place they are calling
## from, with the signal's flaws on top.  Comms (in flight) and Briefing
## (the ops room) both draw through it.
##
## Paintings come first when they exist: assets/comms/places/<place>.png
## behind, assets/comms/people/<speaker>.png in front (with _talk and _blink
## frames for the mouth and eyes).  Until then both are drawn here, so
## every caller has a face and every place a look.
class_name CallView
extends RefCounted

const PLACE_DIR := "res://assets/comms/places/%s.png"
const PEOPLE_DIR := "res://assets/comms/people/%s%s.png"

static var _tex := {}             # path -> Texture2D or null, loaded once


static func _texture(path: String) -> Texture2D:
	if not _tex.has(path):
		_tex[path] = load(path) if ResourceLoader.exists(path) else null
	return _tex[path]


## Draw a call into `r` on `c` (call from c's _draw).  `talk` 0..1 is how
## open the mouth is, `noise` 0..1 how bad the signal is, `t` a clock.
static func draw_call(c: CanvasItem, r: Rect2, speaker_id: String, place: String, talk: float, noise: float, t: float) -> void:
	var spk: Dictionary = StoryLines.SPEAKERS.get(speaker_id, {})
	c.draw_rect(r, Color(0.01, 0.015, 0.03))
	var bg := _texture(PLACE_DIR % place)
	if bg != null:
		_draw_cover(c, bg, r)
	else:
		_draw_place(c, r, place, t)
	var blink := fmod(t + float(speaker_id.hash() % 7), 4.3) < 0.13
	var face := _texture(PEOPLE_DIR % [speaker_id, ""])
	if face != null:
		var frame := face
		if talk > 0.35:
			frame = _texture(PEOPLE_DIR % [speaker_id, "_talk"]) if _texture(PEOPLE_DIR % [speaker_id, "_talk"]) != null else face
		elif blink and _texture(PEOPLE_DIR % [speaker_id, "_blink"]) != null:
			frame = _texture(PEOPLE_DIR % [speaker_id, "_blink"])
		var bob := sin(t * 1.6) * r.size.y * 0.006
		var h := r.size.y * 0.92
		var w := h * frame.get_width() / maxf(frame.get_height(), 1.0)
		c.draw_texture_rect(frame, Rect2(r.position.x + (r.size.x - w) * 0.5, r.end.y - h + bob, w, h), false)
	else:
		_draw_person(c, r, spk, talk, blink, t)
	_draw_signal(c, r, noise, t)


## A painting fills the window, cut to its shape.
static func _draw_cover(c: CanvasItem, tex: Texture2D, r: Rect2) -> void:
	var ts := Vector2(tex.get_width(), tex.get_height())
	var k := maxf(r.size.x / ts.x, r.size.y / ts.y)
	var src_size := r.size / k
	var src := Rect2((ts - src_size) * 0.5, src_size)
	c.draw_texture_rect_region(tex, r, src)


# ------------------------------------------------------------ places

static func _stars(c: CanvasItem, area: Rect2, n: int, seed_value: int, a: float = 0.8) -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_value
	for i in n:
		var p := area.position + Vector2(rng.randf() * area.size.x, rng.randf() * area.size.y)
		c.draw_rect(Rect2(p, Vector2.ONE * (1.0 + float(rng.randf() < 0.15))), Color(0.85, 0.9, 1.0, a * rng.randf_range(0.3, 1.0)))


static func _draw_place(c: CanvasItem, r: Rect2, place: String, t: float) -> void:
	var x := r.position.x
	var y := r.position.y
	var w := r.size.x
	var h := r.size.y
	match place:
		"ops_room":
			# a wide window on Earth's limb, consoles below, a map wall
			c.draw_rect(r, Color(0.05, 0.08, 0.13))
			var win := Rect2(x + w * 0.05, y + h * 0.08, w * 0.9, h * 0.42)
			c.draw_rect(win, Color(0.01, 0.02, 0.05))
			_stars(c, win, 40, 11)
			c.draw_circle(Vector2(x + w * 0.78, y + h * 1.05), h * 0.62, Color(0.18, 0.4, 0.75, 0.9))
			c.draw_arc(Vector2(x + w * 0.78, y + h * 1.05), h * 0.62, PI * 1.1, PI * 1.6, 32, Color(0.6, 0.85, 1.0, 0.7), 2.0)
			c.draw_rect(Rect2(x, y + h * 0.5, w, h * 0.5), Color(0.06, 0.09, 0.14))
			for i in 3:
				c.draw_rect(Rect2(win.position.x + win.size.x * (0.33 * (i + 1)) - 2, win.position.y, 4, win.size.y), Color(0.08, 0.11, 0.16))
			c.draw_rect(win, Color(0.25, 0.35, 0.5), false, 2.0)
			for i in 6:
				var lx := x + w * (0.06 + i * 0.16)
				var on := 0.4 + 0.3 * sin(t * 1.7 + i * 1.3)
				c.draw_rect(Rect2(lx, y + h * 0.62, w * 0.1, h * 0.03), Color(0.3, 0.7, 1.0, on))
		"admiralty":
			c.draw_rect(r, Color(0.09, 0.08, 0.07))
			# the big plot of Sol behind the admiral
			var mc := Vector2(x + w * 0.5, y + h * 0.36)
			c.draw_rect(Rect2(x + w * 0.12, y + h * 0.06, w * 0.76, h * 0.56), Color(0.02, 0.05, 0.08))
			for i in 4:
				c.draw_arc(mc, h * (0.06 + i * 0.065), 0, TAU, 40, Color(0.35, 0.6, 0.8, 0.45), 1.0)
			c.draw_circle(mc, 4.0, Color(1.0, 0.85, 0.4))
			c.draw_circle(mc + Vector2(h * 0.125, 0).rotated(t * 0.2), 2.5, Color(0.4, 0.7, 1.0))
			c.draw_circle(mc + Vector2(h * 0.19, 0).rotated(t * 0.13 + 2.0), 2.5, Color(1.0, 0.4, 0.35))
			c.draw_rect(Rect2(x + w * 0.12, y + h * 0.06, w * 0.76, h * 0.56), Color(0.5, 0.45, 0.3), false, 2.0)
			c.draw_rect(Rect2(x, y + h * 0.66, w, h * 0.34), Color(0.13, 0.11, 0.09))
		"cockpit_terran", "cockpit_pirate":
			var pirate := place == "cockpit_pirate"
			c.draw_rect(r, Color(0.0, 0.01, 0.03))
			_stars(c, r, 50, 23 if pirate else 7)
			# the canopy frame and the seat behind the pilot
			var frame := Color(0.12, 0.08, 0.06) if pirate else Color(0.1, 0.13, 0.18)
			c.draw_colored_polygon(PackedVector2Array([Vector2(x, y), Vector2(x + w * 0.22, y), Vector2(x + w * 0.05, y + h), Vector2(x, y + h)]), frame)
			c.draw_colored_polygon(PackedVector2Array([Vector2(x + w, y), Vector2(x + w * 0.78, y), Vector2(x + w * 0.95, y + h), Vector2(x + w, y + h)]), frame)
			c.draw_rect(Rect2(x, y, w, h * 0.07), frame)
			c.draw_rect(Rect2(x + w * 0.27, y + h * 0.22, w * 0.46, h * 0.78), Color(0.18, 0.12, 0.1) if pirate else Color(0.14, 0.17, 0.22))
			var glow := Color(1.0, 0.25, 0.15, 0.25 + 0.15 * sin(t * 3.0)) if pirate else Color(0.3, 0.75, 1.0, 0.18)
			c.draw_rect(Rect2(x, y + h * 0.82, w, h * 0.18), glow)
			if pirate:
				c.draw_line(Vector2(x + w * 0.3, y), Vector2(x + w * 0.36, y + h * 0.3), Color(0.1, 0.1, 0.1), 3.0)
		"bridge_freighter":
			c.draw_rect(r, Color(0.1, 0.08, 0.05))
			for i in 4:
				c.draw_rect(Rect2(x, y + h * (0.1 + i * 0.12), w, 5), Color(0.2, 0.16, 0.1))
			c.draw_rect(Rect2(x + w * 0.6, y + h * 0.12, w * 0.32, h * 0.3), Color(0.02, 0.03, 0.04))
			_stars(c, Rect2(x + w * 0.6, y + h * 0.12, w * 0.32, h * 0.3), 12, 31)
			c.draw_circle(Vector2(x + w * 0.12, y + h * 0.18), 5, Color(1.0, 0.7, 0.2, 0.6 + 0.3 * sin(t * 2.0)))
		_:
			c.draw_rect(r, Color(0.04, 0.05, 0.08))


# ------------------------------------------------------------ people

static func _ellipse(c: CanvasItem, centre: Vector2, rx: float, ry: float, col: Color) -> void:
	var pts := PackedVector2Array()
	for i in 28:
		var a := TAU * i / 28.0
		pts.append(centre + Vector2(cos(a) * rx, sin(a) * ry))
	c.draw_colored_polygon(pts, col)


static func _draw_person(c: CanvasItem, r: Rect2, spk: Dictionary, talk: float, blink: bool, t: float) -> void:
	var look: Dictionary = spk.get("look", {})
	var skin: Color = look.get("skin", Color(0.8, 0.65, 0.55))
	var hair: Color = look.get("hair", Color(0.2, 0.15, 0.1))
	var uniform: Color = look.get("uniform", Color(0.2, 0.25, 0.35))
	var accent: Color = spk.get("color", Color(0.6, 0.8, 1.0))
	var u := r.size.y / 225.0
	var bob := sin(t * 1.6) * 1.5 * u
	var cx := r.position.x + r.size.x * 0.5
	var head := Vector2(cx, r.position.y + r.size.y * 0.46 + bob)
	# shoulders and collar
	var sh_y := r.end.y - 52.0 * u + bob * 0.5
	c.draw_colored_polygon(PackedVector2Array([Vector2(cx - 92 * u, r.end.y), Vector2(cx - 80 * u, sh_y + 8 * u), Vector2(cx - 30 * u, sh_y - 6 * u),
		Vector2(cx + 30 * u, sh_y - 6 * u), Vector2(cx + 80 * u, sh_y + 8 * u), Vector2(cx + 92 * u, r.end.y)]), uniform)
	c.draw_line(Vector2(cx - 30 * u, sh_y - 4 * u), Vector2(cx - 62 * u, sh_y + 8 * u), accent, 2.0 * u)
	c.draw_rect(Rect2(cx - 9 * u, head.y + 30 * u, 18 * u, sh_y - head.y - 26 * u), skin.darkened(0.15))
	if look.get("long_hair", false):
		_ellipse(c, head + Vector2(0, 12 * u), 38 * u, 46 * u, hair)
	if look.get("helmet", false):
		_ellipse(c, head + Vector2(0, -2 * u), 46 * u, 50 * u, uniform.lightened(0.15))
	_ellipse(c, head, 31 * u, 39 * u, skin)
	if not look.get("helmet", false):
		# hair on top
		c.draw_colored_polygon(PackedVector2Array([head + Vector2(-32, -6) * u, head + Vector2(-26, -34) * u, head + Vector2(0, -44) * u,
			head + Vector2(26, -34) * u, head + Vector2(32, -6) * u, head + Vector2(22, -26) * u, head + Vector2(-20, -28) * u]), hair)
	if look.get("cap", false):
		c.draw_rect(Rect2(head.x - 34 * u, head.y - 44 * u, 68 * u, 16 * u), uniform.darkened(0.2))
		c.draw_rect(Rect2(head.x - 38 * u, head.y - 30 * u, 76 * u, 5 * u), Color(0.1, 0.1, 0.1))
		c.draw_circle(head + Vector2(0, -36) * u, 4 * u, Color(0.9, 0.8, 0.4))
	if look.get("visor", false):
		c.draw_rect(Rect2(head.x - 30 * u, head.y - 14 * u, 60 * u, 18 * u), Color(0.08, 0.06, 0.04, 0.95))
		c.draw_line(head + Vector2(-24, -10) * u, head + Vector2(10, -10) * u, Color(1, 0.6, 0.3, 0.6), 2.0 * u)
	else:
		var eye_h := 1.2 * u if blink else 4.0 * u
		for sx in [-1.0, 1.0]:
			c.draw_rect(Rect2(head.x + sx * 12 * u - 4 * u, head.y - 6 * u - eye_h * 0.5, 8 * u, eye_h), Color(0.08, 0.06, 0.06))
		c.draw_line(head + Vector2(-17, -15) * u, head + Vector2(-7, -16) * u, hair.darkened(0.2), 2.0 * u)
		c.draw_line(head + Vector2(17, -15) * u, head + Vector2(7, -16) * u, hair.darkened(0.2), 2.0 * u)
	# the mouth opens with the voice
	var mh := lerpf(1.5, 9.0, clampf(talk, 0.0, 1.0)) * u
	c.draw_rect(Rect2(head.x - 8 * u, head.y + 17 * u - mh * 0.5, 16 * u, mh), Color(0.3, 0.1, 0.1))
	if look.get("helmet", false):
		# a headset boom across the cheek
		c.draw_line(head + Vector2(30, 0) * u, head + Vector2(12, 18) * u, Color(0.15, 0.15, 0.15), 3.0 * u)
		c.draw_circle(head + Vector2(12, 18) * u, 3.0 * u, Color(0.15, 0.15, 0.15))


# ------------------------------------------------------------ the signal

static func _draw_signal(c: CanvasItem, r: Rect2, noise: float, t: float) -> void:
	# scanlines, always faint
	var y := r.position.y
	while y < r.end.y:
		c.draw_line(Vector2(r.position.x, y), Vector2(r.end.x, y), Color(0, 0, 0, 0.12), 1.0)
		y += 3.0
	if noise > 0.02:
		# torn lines and snow
		var rng := RandomNumberGenerator.new()
		rng.seed = int(t * 24.0)
		var bands := int(3 + noise * 10.0)
		for i in bands:
			var by := r.position.y + rng.randf() * r.size.y
			var bh := rng.randf_range(2.0, 4.0 + noise * 14.0)
			var shift := rng.randf_range(-1.0, 1.0) * noise * 30.0
			c.draw_rect(Rect2(r.position.x + maxf(shift, 0.0), by, r.size.x - absf(shift), minf(bh, r.end.y - by)), Color(0.8, 0.85, 0.9, noise * rng.randf_range(0.08, 0.3)))
		for i in int(noise * 160.0):
			var p := r.position + Vector2(rng.randf() * r.size.x, rng.randf() * r.size.y)
			c.draw_rect(Rect2(p, Vector2(2, 2)), Color(1, 1, 1, rng.randf() * noise))
		if noise > 0.8:
			c.draw_rect(r, Color(0.6, 0.65, 0.7, (noise - 0.8) * 3.0))
	c.draw_rect(r, Color(0.5, 0.65, 0.8, 0.5), false, 1.5)
