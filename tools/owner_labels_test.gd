# Owner labels test: are world, moon and station names coloured by who
# holds them, as the player sees it?  (Kyle, 2026-10-29: "Planets/moons/
# stations that are owned by your team should have their labels in green.
# Owned by friendlies should be yellow. Owned by enemies red. Unowned in
# grey.")
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
	var mine := -1
	var other := -1
	var empty := -1
	for i in w.planets.size():
		var pl = w.planets[i]
		if pl.look == "Sun":
			continue
		if pl.is_colonised() and pl.owner_race == p.race_id and mine < 0:
			mine = i
		elif pl.is_colonised() and pl.owner_race != p.race_id and pl.owner_race <= 2 and other < 0:
			other = i
		elif not pl.is_colonised() and empty < 0:
			empty = i
	_check(mine >= 0 and other >= 0 and empty >= 0, "Sol has a world of ours, another empire's and an empty one")
	if mine < 0 or other < 0 or empty < 0:
		quit(1)
		return
	var their: int = w.planets[other].owner_race
	w.accord = false
	w.peace = []
	_check(w.planet_label_color(p, w.planets[mine]) == W.LABEL_OWN, "%s (ours) is green" % w.planets[mine].body_name)
	_check(w.planet_label_color(p, w.planets[other]) == W.LABEL_ENEMY, "%s (at war) is red" % w.planets[other].body_name)
	_check(w.planet_label_color(p, w.planets[empty]) == W.LABEL_NONE, "%s (nobody's) is grey" % w.planets[empty].body_name)
	w.peace = [[p.race_id, their]]
	_check(w.planet_label_color(p, w.planets[other]) == W.LABEL_FRIEND, "%s (at peace) is yellow" % w.planets[other].body_name)
	w.peace = []
	w.accord = true
	_check(w.planet_label_color(p, w.planets[other]) == W.LABEL_FRIEND, "%s under the Accord is yellow" % w.planets[other].body_name)
	_check(w.owner_color(p, 3) == W.LABEL_ENEMY, "pirates stay red under the Accord")
	w.accord = false
	_check(w.owner_color(p, gd.NEUTRAL) == W.LABEL_NONE, "the free port's side is grey")
	_check(w.owner_color(null, p.race_id) != W.LABEL_OWN, "no viewer: no side is anyone's own")
	# stations: the label follows the station's team()
	var own_base = null
	var enemy_base = null
	for s in w.ships:
		if s.is_station and not s.dead:
			if s.team() == p.team() and own_base == null:
				own_base = s
			elif s.team() != p.team() and s.race_id <= 2 and enemy_base == null:
				enemy_base = s
	if own_base != null:
		_check(w.owner_color(p, own_base.team()) == W.LABEL_OWN, "our starbase %s is green" % own_base.ship_name)
	if enemy_base != null:
		_check(w.owner_color(p, enemy_base.team()) == W.LABEL_ENEMY, "enemy station %s is red" % enemy_base.ship_name)
	# the labels are drawn without errors, on the HUD and the map
	for i in 5:
		await process_frame
	if shots:
		DirAccess.make_dir_recursive_absolute(out_dir)
		w.peace = [[p.race_id, 0]]       # Arceans friendly, so all four colours show
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
