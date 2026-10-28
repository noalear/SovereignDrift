# Screenshots of station capture: an enemy mining station breached and
# open to boarders, the boarding under way, and the station taken.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/station_capture_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var st: Ship = null
var body: Planet = null
var step := 0
var t0 := 0.0


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
	for s in w.ships:
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai != null:
			ai.set_physics_process(false)
	match step:
		0:
			if frames < 90:
				return false
			var enemy := (p.race_id + 1) % 3
			var earth: Planet = w.planet_by_name["Earth"]
			for q in w.planets:
				if q.has_surface() and not q.is_colonised() and q.look != "Sun" and Economy.surface_of(q.look, q.habitable) != "gas":
					if body == null or q.global_position.distance_to(earth.global_position) < body.global_position.distance_to(earth.global_position):
						body = q
			st = w.build_station(enemy, "MiningStation", body, body.global_position + Vector2(body.radius + 320.0, 0), false)
			st.shield = 0.0
			while st.hull > st.hull_max * 0.42:
				st.take_damage(st.hull_max * 0.05, p, st.global_position + Vector2(st.size * 0.4, 0))
				st.shield = 0.0
			p.refit("Cruiser")
			p.crew = p.crew_max
			_put(p, st.global_position + Vector2(-(st.size + p.size + 140.0), 60.0), st.velocity)
			p.rotation = 0.0
			p.target = st
			t0 = w.elapsed
			step = 1
		1:
			st.shield = 0.0
			p.velocity = st.velocity
			if w.elapsed - t0 < 1.5:
				return false
			_shot("station_breached")
			w.try_board(p)
			t0 = w.elapsed
			step = 2
		2:
			if w.elapsed - t0 < 1.0:
				return false
			_shot("station_boarding")
			step = 3
		3:
			if p.boarding != null and w.elapsed - t0 < 30.0:
				return false
			t0 = w.elapsed
			step = 4
		4:
			if w.elapsed - t0 < 1.0:
				return false
			for s in w.ships:
				if s.design_name == "MiningStation" and s.station_body == body and not s.dead:
					p.target = s
			_shot("station_captured")
			quit(0)
	return false
