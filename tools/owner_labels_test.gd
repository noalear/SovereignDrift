# Owner labels test: are ship, world, moon and station names in the
# colour of the empire that holds them?  (Kyle, 2026-10-29: "all names/
# labels/etc on ships planets etc for terran are blue, drengin are red,
# pirates are yellow, and arcean are green."  Unowned stays grey.)
#   godot --headless --path . -s tools/owner_labels_test.gd
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/owner_labels_test.gd -- --shots
# --shots also saves the labels near Earth and the system map to
# /tmp/claude-0/sf/owner-labels (SF_SHOTS=<dir> to change it).
extends SceneTree

var main: Node
var gd: Node
var w
var fails := 0
var shots := false
var out_dir := "/tmp/claude-0/sf/owner-labels"


func _init() -> void:
	shots = "--shots" in OS.get_cmdline_user_args()
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		fails += 1


func _start() -> void:
	gd = root.get_node("/root/GameData")   # autoloads are not names in a -s script
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	main._start_free_flight()
	for i in 10:
		await process_frame
	w = main.world
	var p = w.player
	var W = w.get_script()
	# each empire's own colour, whoever looks and whatever the politics
	var hue := {
		0: func(c: Color): return c.g > c.r + 0.3 and c.g > c.b + 0.3,             # Arcean green
		1: func(c: Color): return c.b > c.r + 0.3 and c.b >= c.g,                   # Terran blue
		2: func(c: Color): return c.r > c.g + 0.4 and c.r > c.b + 0.4,             # Drengin red
		3: func(c: Color): return c.r > 0.9 and c.g > 0.8 and c.b < 0.4,            # Pirate yellow
	}
	var names := {0: "Arcean green", 1: "Terran blue", 2: "Drengin red", 3: "Pirate yellow"}
	for race in hue:
		_check(hue[race].call(w.label_color(race)), "%s labels are %s %s" % [gd.race(race)["name"], names[race], w.label_color(race)])
	_check(w.label_color(gd.NEUTRAL) == W.LABEL_NONE, "the free port's labels are grey")
	_check(w.label_color(-1) == W.LABEL_NONE, "nobody's is grey")
	var seen := {}
	var empty = null
	for pl in w.planets:
		if pl.is_colonised():
			seen[pl.owner_race] = true
			if w.planet_label_color(pl) != w.label_color(pl.owner_race):
				_check(false, "%s is in its empire's colour" % pl.body_name)
		elif empty == null and pl.look != "Sun":
			empty = pl
	_check(seen.size() >= 3, "Sol has colonies of %d empires" % seen.size())
	_check(empty != null and w.planet_label_color(empty) == W.LABEL_NONE, "an empty world (%s) is grey" % (empty.body_name if empty != null else "none"))
	var drengin = null
	for pl in w.planets:
		if pl.is_colonised() and pl.owner_race == 2:
			drengin = pl
			break
	if drengin != null:
		w.accord = true
		_check(hue[2].call(w.planet_label_color(drengin)), "%s stays Drengin red under the Accord" % drengin.body_name)
		w.accord = false
	for s in w.ships:
		if s.is_station and not s.dead and s.race_id == p.race_id:
			_check(hue[1].call(w.label_color(s.race_id)), "our starbase %s is Terran blue" % s.ship_name)
			break
	# the labels are drawn without errors, on the HUD and the map
	for i in 5:
		await process_frame
	if shots:
		DirAccess.make_dir_recursive_absolute(out_dir)
		# out between Earth and Luna, zoomed out, so worlds and the starbase are in view
		var earth = null
		var luna = null
		for pl in w.planets:
			if pl.body_name == "Earth":
				earth = pl
			elif pl.body_name == "Luna":
				luna = pl
		if earth != null and luna != null:
			p.global_position = (earth.global_position + luna.global_position) * 0.5
			p.body.pos = p.global_position
			p.velocity = Vector2.ZERO
			p.body.vel = Vector2.ZERO
		w.target_zoom = 0.25
		await _settle(90)
		_shot("labels_world")
		var ev := InputEventAction.new()
		ev.action = "tactical_map"
		ev.pressed = true
		Input.parse_input_event(ev)
		await _settle(30)
		_shot("labels_map")
	else:
		var tm = load("res://src/ui/TacticalMap.gd").new()
		tm.world = w
		main.ui_layer.add_child(tm)
		for i in 5:
			await process_frame
		tm.queue_free()
	print("owner_labels_test: %s (%d failed)" % ["OK" if fails == 0 else "FAILED", fails])
	quit(0 if fails == 0 else 1)


func _settle(n: int) -> void:
	for i in n:
		await process_frame


func _shot(name: String) -> void:
	await RenderingServer.frame_post_draw
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved ", out_dir + "/" + name + ".png")
