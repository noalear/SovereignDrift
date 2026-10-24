## Starfield -- three parallax layers of tiled star textures plus a few
## faint nebulae, drawn relative to the camera.
class_name Starfield
extends Node2D

const TILE := 1024
var layers: Array = []      # [texture, parallax, tint]
var camera: Camera2D = null
var world: Node = null          # when set, follows its camera between physics ticks
var nebulae: Array = []


func _ready() -> void:
	z_index = -100
	# placed by hand every frame, so the engine must not interpolate it
	physics_interpolation_mode = Node.PHYSICS_INTERPOLATION_MODE_OFF
	var rng := RandomNumberGenerator.new()
	rng.seed = 42
	layers = [
		[_make_tile(rng, 260, 1.0, 1.6), 0.08, Color(1, 1, 1, 0.55)],
		[_make_tile(rng, 140, 1.4, 2.4), 0.2, Color(1, 1, 1, 0.8)],
		[_make_tile(rng, 50, 2.0, 3.2), 0.42, Color(1, 1, 1, 1.0)],
	]
	for i in 14:
		nebulae.append([Vector2(rng.randf_range(-30000, 30000), rng.randf_range(-30000, 30000)),
			rng.randf_range(1400, 4200),
			Color(rng.randf_range(0.2, 0.9), rng.randf_range(0.1, 0.6), rng.randf_range(0.4, 1.0), rng.randf_range(0.025, 0.06))])


func _make_tile(rng: RandomNumberGenerator, count: int, min_r: float, max_r: float) -> ImageTexture:
	var img := Image.create(TILE, TILE, false, Image.FORMAT_RGBA8)
	img.fill(Color(0, 0, 0, 0))
	for i in count:
		var x := rng.randi() % TILE
		var y := rng.randi() % TILE
		var r := rng.randf_range(min_r, max_r)
		var temp := rng.randf()
		var c := Color(1, 1, 1)
		if temp < 0.2:
			c = Color(0.75, 0.85, 1.0)
		elif temp > 0.85:
			c = Color(1.0, 0.85, 0.7)
		var bright := rng.randf_range(0.5, 1.0)
		var rr := int(ceil(r))
		for dx in range(-rr, rr + 1):
			for dy in range(-rr, rr + 1):
				var d := Vector2(dx, dy).length()
				if d <= r:
					var a := bright * clampf(1.0 - d / r, 0.0, 1.0)
					var px := (x + dx + TILE) % TILE
					var py := (y + dy + TILE) % TILE
					var old := img.get_pixel(px, py)
					img.set_pixel(px, py, Color(c.r, c.g, c.b, maxf(old.a, a)))
	return ImageTexture.create_from_image(img)


func _process(_delta: float) -> void:
	if camera != null:
		global_position = _centre()
		queue_redraw()


func _centre() -> Vector2:
	if world != null and is_instance_valid(world):
		return world.camera_centre()
	return camera.get_screen_center_position()


func _draw() -> void:
	if camera == null:
		return
	var zoom := camera.zoom.x
	var view := get_viewport_rect().size / zoom
	var cam := _centre()
	for nb in nebulae:
		var p: Vector2 = (nb[0] - cam * 0.05) - cam
		if (p + cam).distance_to(cam) < view.length() + nb[1]:
			draw_circle(p, nb[1], nb[2])
			draw_circle(p * 1.01, nb[1] * 0.5, nb[2])
	for layer in layers:
		var tex: ImageTexture = layer[0]
		var par: float = layer[1]
		var off := cam * par
		var start := Vector2(fposmod(-off.x, TILE), fposmod(-off.y, TILE)) - Vector2(TILE, TILE) - view * 0.5
		var cols := int(ceil(view.x / TILE)) + 2
		var rows := int(ceil(view.y / TILE)) + 2
		for i in cols:
			for j in rows:
				draw_texture_rect(tex, Rect2(start + Vector2(i * TILE, j * TILE), Vector2(TILE, TILE)), false, layer[2])
