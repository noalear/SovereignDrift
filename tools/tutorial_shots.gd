# Screenshots of the tutorial: the cockpit tour, the beacon, the drone,
# the shipyard and the ground with Commander Hale's subtitles over them.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/tutorial_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var plan := [
	# [lesson, shot name, frames to wait in it]
	["hud_status", "tutorial_cockpit", 40],
	["beacon", "tutorial_beacon", 60],
	["map", "tutorial_map", 40],
	["disable", "tutorial_drone", 60],
	["shipyard_tour", "tutorial_shipyard", 40],
	["cargo", "tutorial_ground", 260],
	["ledger", "tutorial_ledger", 40],
	["freeport", "tutorial_freeport", 40],
]
var at := 0
var entered := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _put(s: Ship, pos: Vector2, vel: Vector2) -> void:
	s.global_position = pos
	s.velocity = vel
	s.body.pos = pos
	s.body.vel = vel
	s.reset_physics_interpolation()


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved " + name)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 20:
		main._start_tutorial()
	if frames <= 20 or main.tutorial == null:
		return false
	var t: Tutorial = main.tutorial
	var w: World = main.world
	var p: Ship = w.player
	p.invulnerable = 1e9
	if at >= plan.size():
		quit(0)
		return true
	var want: String = plan[at][0]
	if t.step.get("id", "") != want:
		# jump ahead, doing what the lessons in between would have done
		var idx := -1
		for i in Tutorial.STEPS.size():
			if Tutorial.STEPS[i]["id"] == want:
				idx = i
		_prepare(w, t, p, want)
		t._go(idx)
		entered = frames
		return false
	# the words fully shown for the picture
	if frames - entered == 2:
		_arrange(w, t, p, want)
	if frames - entered == plan[at][2] - 2:
		# the line fully said, for the picture
		t.voice.stop()
		t.line_time = t.line_len
		t.line_id = want
	if frames - entered >= plan[at][2]:
		_shot(plan[at][1])
		at += 1
	return false


## Before a lesson starts: the state it expects.
func _prepare(w: World, t: Tutorial, p: Ship, id: String) -> void:
	match id:
		"shipyard_tour":
			var base := w.starbase_of(1)
			_put(p, base.global_position + Vector2(base.size + 120, 0), base.velocity)
			w.try_refit(p)
		"disable":
			main._close_tactical()
		"cargo":
			if main.builder != null:
				main.builder._cancel()
			var earth: Planet = w.planet_by_name["Earth"]
			_put(p, earth.global_position + Vector2(earth.radius + 120, 0), earth.velocity)
			w.try_land(p)
		"freeport":
			if main.cargo_panel != null:
				main._close_cargo()
			if p.is_landing():
				p.leave_ground_now()
			var port: Ship = null
			for s in w.ships:
				if s.design_name == "TradeStation":
					port = s
			_put(p, port.global_position + Vector2(-2600, 900), port.velocity)


## Once a lesson is up: its scene for the shot.
func _arrange(w: World, t: Tutorial, p: Ship, id: String) -> void:
	if id == "cargo":
		await create_timer(3.0).timeout     # down on the ground first
	match id:
		"beacon":
			var pos := t.beacon_position()
			_put(p, pos - Vector2(700, -260), w.planet_by_name["Earth"].velocity)
			p.rotation = (pos - p.global_position).angle()
		"disable":
			t.drone.take_damage(t.drone.shield_max + 200.0, p, t.drone.global_position - t.drone.forward() * 15.0)
			t.goal_met = false
		"map":
			main.tactical = TacticalMap.new()
			main.tactical.world = w
			main.ui_layer.add_child(main.tactical)
		"ledger":
			if main.cargo_panel != null:
				main._close_cargo()
			main.ledger = Ledger.new()
			main.ledger.world = w
			main.ui_layer.add_child(main.ledger)
		"cargo":
			if main.cargo_panel == null:
				main.cargo_panel = CargoPanel.new()
				main.cargo_panel.world = w
				main.cargo_panel.close_requested.connect(main._close_cargo)
				main.ui_layer.add_child(main.cargo_panel)
