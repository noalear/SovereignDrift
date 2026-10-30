# Pictures of every world's painted look, for checking planet, moon and
# Sun art: planets.png (all bodies on one sheet) and sun.png (the Sun up
# close, part of its disc and the corona, as a ship flying by sees it),
# then ringed.png (Saturn and Uranus large, with their rings).
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/planet_gallery.gd
extends SceneTree

const LOOKS := ["Mercury", "Venus", "Earth", "Luna", "Mars", "Aster", "Jupiter", "Io",
	"Europa", "Ganymede", "Callisto", "Saturn", "Titan", "Rhea", "Uranus", "Neptune", "Triton", "Pluto"]

var out_dir := "/tmp/claude-0/sf/shots"
var frames := 0
var bodies: Array = []
var bg: ColorRect


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	call_deferred("_start")


func _body(look: String, pos: Vector2, r: float, seed_value: int) -> Planet:
	var p := Planet.new()
	p.look = look
	p.body_name = look
	p.radius = r
	p.process_mode = Node.PROCESS_MODE_DISABLED
	Planet._ensure_resources()
	p.material_ = ShaderMaterial.new()
	p.material_.shader = Planet.planet_shader
	var rng := RandomNumberGenerator.new()
	rng.seed = seed_value
	p._style(rng)
	p.material = p.material_
	p.position = pos
	if Planet.RingsFx.BANDS.has(look):
		p.rings_fx = Planet.RingsFx.new()
		p.rings_fx.planet = p
		p.add_child(p.rings_fx)
	root.add_child(p)
	if p.rings_fx != null:
		# lit from the right, as the shader's default light
		p.rings_fx.mat.set_shader_parameter("away", Vector2(-1, 0))
	return p


func _start() -> void:
	bg = ColorRect.new()
	bg.color = Color(0.01, 0.012, 0.02)
	bg.size = Vector2(1920, 1080)
	root.add_child(bg)
	var cols := 6
	for i in LOOKS.size():
		var c := i % cols
		var r := i / cols
		bodies.append(_body(LOOKS[i], Vector2(160 + c * 320, 180 + r * 360), 110.0, 1000 + i))


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved ", name)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 20:
		_shot("planets")
		for b in bodies:
			b.queue_free()
		bodies.clear()
		# the Sun's edge filling the right of the screen, as seen up close
		bodies.append(_body("Sun", Vector2(1900, 540), 1100.0, 7))
	if frames == 40:
		_shot("sun")
		bodies[0].position = Vector2(960, 540)
		bodies[0].radius = 330.0
		bodies[0].material_.set_shader_parameter("extent", bodies[0].quad_extent())
		bodies[0].queue_redraw()
	if frames == 60:
		_shot("sun_whole")
		bodies[0].queue_free()
		bodies.clear()
		bodies.append(_body("Saturn", Vector2(560, 540), 190.0, 11))
		bodies.append(_body("Uranus", Vector2(1480, 540), 150.0, 14))
	if frames == 80:
		_shot("ringed")
		return true
	return false
