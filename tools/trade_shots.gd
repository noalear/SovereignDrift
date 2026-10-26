# Screenshots of trade and shipyards (the Resources update, phase 4): the
# free port and its market panel, a shipyard station with its order book,
# and the shipyard screen with the civilian hulls and hull materials.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/trade_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var port: Ship = null
var yard: Ship = null
var builder: Builder = null


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


func _alongside(p: Ship, st: Ship) -> void:
	var c: Vector2 = st.station_body.global_position if st.station_body != null else Vector2.ZERO
	var out := (st.global_position - c).normalized()
	_put(p, st.global_position + out * (st.size + p.size + 70.0), st.velocity)


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
	p.rank = 7
	if frames == 40:
		for s in w.ships:
			if s.race_id == gd.NEUTRAL:
				port = s
		p.refit("Freighter")
		p.credits = 2400
		p.cargo = {"food": 120.0, "ice": 60.0}
		port.cargo["exotics"] = 40.0
		port.cargo["food"] = 420.0
		w.target_zoom = 1.0
	# at the free port, with the market open
	if frames > 40 and frames < 200:
		_alongside(p, port)
		if frames == 120:
			_panel(true)
		if frames == 190:
			_shot("trade_free_port")
			_panel(false)
	# a Terran shipyard station over Jupiter, with its order book
	if frames == 200:
		var out := jupiter.global_position.normalized()
		yard = w.build_station(p.race_id, "Shipyard", jupiter, jupiter.global_position + out * (jupiter.radius + 900.0), false)
		yard.invulnerable = 1e9
		yard.cargo = {"metals": 64.0, "alloys": 37.0, "exotics": 9.0}
		yard.orders = [["Trader", 31.0, p.pilot_name]]
		p.refit("Cruiser")
		p.cargo.clear()
		p.credits = 5200
	if frames >= 200 and frames < 400:
		_alongside(p, yard)
		yard.invulnerable = 1e9
		if frames == 260:
			_panel(true, "orders")
		if frames == 330:
			_shot("trade_yard_orders")
			_panel(false)
		if frames == 340:
			builder = Builder.new()
			main.ui_layer.add_child(builder)
			builder.open(p, yard, w)
		if frames == 390:
			_shot("trade_yard_builder")
			builder.queue_free()
			return true
	return false
