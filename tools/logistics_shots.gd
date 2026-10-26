# Screenshots of phase 6 (the Resources update): the ledger's job board
# with a job taken, a convoy and its escort on the system map, and the
# trade window with resources from the cargo hold on the table.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/logistics_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var convoy: Ship = null
var trade: Trade = null


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
	var luna: Planet = w.planet_by_name["Luna"]
	var gov: Governor = w.governors[p.race_id]
	if frames == 60:
		# shortages for the board, and a job taken
		luna.stock["food"] = 0.0
		luna.stock["metals"] = 0.0
		w.planet_by_name["Mars"].stock["ice"] = 0.0
		earth.stock["food"] = 900.0
		earth.stock["metals"] = 600.0
		gov.jobs.clear()
		gov.refresh_board()
		p.refit("Freighter")
		for j in gov.jobs:
			if j["kind"] == "haul":
				w.job_command(p, j["id"])
				break
		# a convoy bound for Luna, with an escort
		for s in w.ships:
			if s.support and s.race_id == p.race_id and s.design_name == "Freighter" and convoy == null:
				convoy = s
		var ai: AIPilot = convoy.get_node("AI")
		ai.set_physics_process(false)
		convoy.cargo = {"metals": 220.0, "alloys": 30.0}
		ai.job = "deliver"
		ai.job_planet = luna
		ai.dock_at = null
		ai.state = AIPilot.State.FERRY
		var at := earth.global_position + Vector2(-1800, -1200)
		_put(convoy, at, earth.velocity)
		for s in w.ships:
			if s.race_id == p.race_id and not s.support and not s.is_station and not s.is_player and s.get_node_or_null("AI") != null:
				s.get_node("AI").escort_duty = true
				s.get_node("AI").state = AIPilot.State.PATROL
				s.get_node("AI").escort = null
				_put(s, at + Vector2(300, 200), earth.velocity)
				break
		gov.form_convoys()
		_put(p, at + Vector2(-500, 300), earth.velocity)
	if frames > 60 and convoy != null:
		convoy.cargo = {"metals": 220.0, "alloys": 30.0}
	if frames == 90:
		main.ledger = Ledger.new()
		main.ledger.world = w
		main.ui_layer.add_child(main.ledger)
	if frames == 120:
		_shot("logistics_ledger")
		main._close_ledger()
		main.tactical = TacticalMap.new()
		main.tactical.world = w
		main.ui_layer.add_child(main.tactical)
	if frames == 150:
		_shot("logistics_map")
		main._close_tactical()
		trade = Trade.new()
		main.ui_layer.add_child(trade)
		trade.open({"id": 1, "me": 1, "partner": 2, "partner_name": "Kyle", "credits": 2400, "hold": ["5002"], "blueprints": [],
			"cargo": {"metals": 120.0, "exotics": 14.0}})
		trade.offers[1] = {"credits": 200, "parts": [], "blueprints": [], "cargo": {"metals": 60}}
		trade.offers[2] = {"credits": 0, "parts": [], "blueprints": [], "cargo": {"fuel": 40, "food": 25}, "part_names": ["Shield II"]}
	if frames == 180:
		_shot("logistics_trade")
		return true
	return false
