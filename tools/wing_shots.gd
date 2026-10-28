# Screenshots of command wings: the wing flying in formation with its HUD
# panel and labels, the orders wheel open, the wing attacking a target, and
# the system map with the wing's order on it.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/wing_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var foe: Ship = null


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel
	s.reset_physics_interpolation()


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved " + name)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_game()
	if frames <= 30 or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	p.invulnerable = 1e9
	var earth: Planet = w.planet_by_name["Earth"]
	if frames == 60:
		p.rank = 9
		p.refit("Destroyer")
		var spot := earth.global_position + (earth.global_position - w.sun.global_position).normalized() * 2400.0
		_put(p, spot, earth.velocity)
		p.rotation = -PI * 0.5
		for s in w.ships:
			if s.race_id == 1 and not s.is_player and not s.support and not s.is_station and s.get_node_or_null("AI") != null:
				s.rank = mini(s.rank, 6)
				s.get_node("AI").ferry = false
				s.colonists = 0.0
		w.wing_command(p, "call")
		var wing: Array = w.wing.members(p)
		for i in wing.size():
			_put(wing[i], p.global_position + Vector2(220 * (i - 1), 300), p.velocity)
		w.wing_command(p, "form")
	if frames == 360:
		_shot("wing_formation")
		main.hud.wheel._open()
		main.hud.wheel.tapped = true
	if frames == 370:
		main.hud.wheel.pick = 1
	if frames == 380:
		_shot("wing_wheel")
		main.hud.wheel._close()
		foe = w.spawn_ship("Augustus", 2, "Cruiser", "Ruthless", p.global_position + Vector2(0, -1300), p.velocity, 3)
		foe.invulnerable = 1e9
		p.target = foe
		w.wing_command(p, "attack")
	if frames == 560:
		_shot("wing_attack")
		main._unhandled_input(_tab())
	if frames == 600:
		_shot("wing_map")
		print("done")
		quit(0)
	return false


func _tab() -> InputEventAction:
	var ev := InputEventAction.new()
	ev.action = "tactical_map"
	ev.pressed = true
	return ev
