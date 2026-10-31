## Starfield -- a painted nebula far behind everything, then three
## parallax layers of tiled star textures, drawn relative to the camera.
class_name Starfield
extends Node2D

const TILE := 1024
var layers: Array = []      # [texture, parallax, tint]
var camera: Camera2D = null
var world: Node = null          # when set, follows its camera between physics ticks
var nebula: Texture2D = null

## The nebula is the farthest thing in the sky: a faint backdrop pinned
## almost still behind the stars.  It is drawn in world units like the star
## layers, so it grows and shrinks with the zoom, but it slides only a
## sliver of the camera's travel, as something very far away would.
const NEBULA_WORLD := 3600.0        # one tile's width, world units
const NEBULA_PARALLAX := 0.04       # share of the camera's travel it follows: a crossing of the system slides it ~900 units
const NEBULA_TINT := Color(1, 1, 1, 0.42)


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
	nebula = load("res://assets/backdrop/nebula.jpg") as Texture2D


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
	if nebula != null:
		var off := cam * NEBULA_PARALLAX
		var start := Vector2(fposmod(-off.x, NEBULA_WORLD), fposmod(-off.y, NEBULA_WORLD)) - Vector2(NEBULA_WORLD, NEBULA_WORLD) - view * 0.5
		var tile := Vector2(NEBULA_WORLD, NEBULA_WORLD)
		for i in int(ceil(view.x / NEBULA_WORLD)) + 2:
			for j in int(ceil(view.y / NEBULA_WORLD)) + 2:
				draw_texture_rect(nebula, Rect2(start + Vector2(i, j) * NEBULA_WORLD, tile), false, NEBULA_TINT)
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
