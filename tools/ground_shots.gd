# Screenshots of the ground war (the Resources update, phase 5): Mars
# under its planetary shield with its card, the troops and buildings tabs
# landed on Earth, and the card of a world with a ground battle on.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/ground_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	for k in s.inputs.keys():
		s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel
	s.invulnerable = 1e9


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved " + name)


func _panel(on: bool, tab: String = "") -> void:
	if on and main.cargo_panel == null:
		main.cargo_panel = CargoPanel.new()
		main.cargo_panel.world = main.world
		main.cargo_panel.close_requested.connect(main._close_cargo)
		main.ui_layer.add_child(main.cargo_panel)
	elif not on and main.cargo_panel != null:
		main._close_cargo()
	if on and tab != "":
		main.cargo_panel.tab = tab


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_game()
	if frames <= 30 or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	var mars: Planet = w.planet_by_name["Mars"]
	var earth: Planet = w.planet_by_name["Earth"]
	var venus: Planet = w.planet_by_name["Venus"]
	p.invulnerable = 1e9
	p.rank = 7
	# Mars under its shield, hit now and then so the bubble shows
	if frames == 40:
		p.refit("DropShip")
		w.target_zoom = 0.55
	if frames > 40 and frames < 160:
		var out := (mars.global_position - w.sun.global_position).normalized()
		_put(p, mars.global_position + out * (mars.radius + 520.0), mars.velocity)
		if frames % 20 == 0:
			mars.take_damage(60.0, p)
		if frames == 150:
			w.inspected_planet = mars
		if frames == 158:
			_shot("ground_mars_shield")
			w.inspected_planet = null
	# landed on Earth: the troops and the buildings
	if frames == 160:
		w.target_zoom = 1.0
		var out := (earth.global_position - w.sun.global_position).normalized()
		_put(p, earth.global_position + out * (earth.radius + 60.0), earth.velocity)
		earth.stock["metals"] = 140.0
		earth.stock["alloys"] = 30.0
		earth.stock["exotics"] = 12.0
		w.try_land(p, earth)
	if frames == 340:
		p.troops = {"infantry": 2.0, "armour": 1.0}
		w.recruit_command(p, "infantry")
		w.build_command(p, "flak")
		_panel(true, "troops")
	if frames == 400:
		_shot("ground_troops_tab")
		_panel(true, "buildings")
	if frames == 440:
		_shot("ground_buildings_tab")
		_panel(false)
	# a battle for Venus, and its card
	if frames == 450:
		p.leave_ground_now()
		venus.battle = {"race": 1, "units": {"infantry": 4.0, "armour": 2.0}, "militia": 3.0, "pilot": p.pilot_name, "sabotage": 0.0}
		venus.silenced = 30.0
	if frames > 450 and frames < 600:
		var out := (venus.global_position - w.sun.global_position).normalized()
		_put(p, venus.global_position + out * (venus.radius + 420.0), venus.velocity)
		venus.silenced = maxf(venus.silenced, 5.0)
		if venus.owner_race == 2:
			# held mid-fight for the picture
			venus.garrison = {"infantry": 2.0}
			venus.battle["units"] = {"infantry": 4.0, "armour": 2.0}
			venus.battle["militia"] = 3.0
		if frames == 570:
			w.inspected_planet = venus
		if frames == 590:
			_shot("ground_battle_card")
			return true
	return false
