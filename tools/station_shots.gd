# Screenshots of the stations (the Resources update, phase 3): a frame
# going up and the finished stations over Jupiter, the kit counter at the
# starbase, the cargo panel alongside a frame, and a kit ready to deploy.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/station_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var built: Array = []
var frame: Ship = null


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


func _panel(on: bool) -> void:
	if on and main.cargo_panel == null:
		main.cargo_panel = CargoPanel.new()
		main.cargo_panel.world = main.world
		main.cargo_panel.close_requested.connect(main._close_cargo)
		main.ui_layer.add_child(main.cargo_panel)
	elif not on and main.cargo_panel != null:
		main._close_cargo()


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_game()
	if frames <= 30 or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	var gd = root.get_node("/root/GameData")
	var jupiter: Planet = w.planet_by_name["Jupiter"]
	p.invulnerable = 1e9
	p.rank = gd.KIT_RANK
	if frames == 40:
		p.refit("Freighter")
		p.credits = 1500
		w.target_zoom = 0.6
		# one of each over Jupiter, out on the far side from the Sun
		var out := jupiter.global_position.normalized()
		var side := out.orthogonal()
		var at := jupiter.global_position + out * (jupiter.radius + 900.0)
		var keys := ["GasSkimmer", "Refinery", "SupplyDepot", "SensorStation", "DefenseStation"]
		for i in keys.size():
			var st := w.build_station(p.race_id, keys[i], jupiter, at + side * (i - 2) * 420.0, false)
			st.invulnerable = 1e9
			built.append(st)
		frame = w.build_station(p.race_id, "MiningStation", jupiter, at + out * 420.0)
		frame.invulnerable = 1e9
		frame.cargo = {"metals": 12.0}
	if frames > 40 and frames < 200:
		var out := jupiter.global_position.normalized()
		_put(p, jupiter.global_position + out * (jupiter.radius + 1300.0), jupiter.velocity)
	if frames == 160:
		_shot("stations_jupiter")
	# alongside the frame, with the cargo panel
	if frames >= 200 and frames < 300:
		var out := (frame.global_position - jupiter.global_position).normalized()
		_put(p, frame.global_position + out * (frame.size + p.size + 60.0), frame.velocity)
		p.cargo = {"metals": 20.0, "food": 15.0}
		w.target_zoom = 1.0
		if frames == 230:
			_panel(true)
		if frames == 290:
			_shot("station_frame_panel")
			_panel(false)
	# the kit counter at the starbase
	if frames >= 300 and frames < 400:
		var base := w.starbase_of(p.race_id)
		p.cargo.clear()
		_put(p, base.global_position + Vector2(base.size + 140.0, 0), base.velocity)
		if frames == 330:
			_panel(true)
		if frames == 390:
			_shot("station_kits_panel")
			_panel(false)
	# a kit ready to deploy over Saturn
	if frames >= 400 and frames < 500:
		var saturn: Planet = w.planet_by_name["Saturn"]
		p.kit = "GasSkimmer"
		_put(p, saturn.global_position + saturn.global_position.normalized() * (saturn.radius + 500.0), saturn.velocity)
		if frames == 430:
			_panel(true)
		if frames == 490:
			_shot("station_deploy_panel")
			_panel(false)
			return true
	return false
