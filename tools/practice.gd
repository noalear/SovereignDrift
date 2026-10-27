# Practice duel test: pick "Practice duel" on the menu with the mouse and
# check the mode: no round clock, no fleets, one pirate Scout in sensor
# range, a fresh pirate near you after each kill, and quiet colonies.
#   godot --headless --path . -s tools/practice.gd
extends SceneTree

var frames := 0
var main: Node
var failed := false
var hull_at_mars := 0.0
var mars_from := -1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		# Main.gd failed to compile: usually a stale class cache after new
		# class_name files were pulled.  Fix: godot --headless --path . --import
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	await process_frame
	var b: Array = main.menu.buttons[0]
	for mb in main.menu.buttons:
		if mb[1] == "Practice duel":
			b = mb
	_check(b[1] == "Practice duel", "the menu has a Practice duel button (%s)" % b[1])
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = (b[0] as Rect2).get_center()
	ev.global_position = ev.position
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)
	await process_frame
	await process_frame
	_check(main.world != null and main.world.practice, "clicking it starts a practice duel")


func _pirate(w) -> Ship:
	for s in w.ships:
		if s.race_id == 3:
			return s
	return null


func _process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null:
		return false
	var w = main.world
	var p: Ship = w.player
	if frames == 10:
		_check(w.round_rules == null, "no round clock")
		var bases: Array = w.ships.filter(func(s): return s.is_station)
		_check(w.ships.size() == 3 and bases.size() == 1, "only you, your starbase and one pirate (%d ships)" % w.ships.size())
		var pr := _pirate(w)
		_check(pr != null and pr.design_name == "Scout", "the opponent is a pirate Scout")
		if pr != null:
			var d := pr.global_position.distance_to(p.global_position)
			# (it may already have cloaked to sneak up on you)
			_check(d < p.sensor_range and (pr.visible_to(p) or pr.cloaked), "the pirate starts inside sensor range (%d of %d)" % [d, p.sensor_range])
	if frames == 240:
		var pr := _pirate(w)
		pr.take_damage(1e6, p)
	if frames == 300:
		_check(w.duel_score[0] == 1, "your kill is counted (%s)" % [w.duel_score])
	if frames > 300 and mars_from < 0:
		var pr := _pirate(w)
		if pr != null and not pr.dead:
			var d := pr.global_position.distance_to(p.global_position)
			_check(d < p.sensor_range, "a new pirate launched close by (%d)" % d)
			mars_from = frames
			var mars = w.planet_by_name["Mars"]
			p.global_position = mars.global_position + Vector2(mars.radius + 300.0, 0)
			hull_at_mars = p.hull + p.shield
		elif frames > 4000:
			_check(false, "a new pirate launched")
			mars_from = frames
	if mars_from > 0 and frames < mars_from + 1500:
		# park the pirate far off and sit beside the Drengin colony on Mars
		var pr := _pirate(w)
		if pr != null and pr.global_position.distance_to(p.global_position) < 20000.0:
			pr.global_position += Vector2(30000, 30000)
		var mars = w.planet_by_name["Mars"]
		p.global_position = mars.global_position + Vector2(mars.radius + 300.0, 0)
		p.velocity = mars.velocity
	if mars_from > 0 and frames == mars_from + 1500:
		_check(p.hull + p.shield >= hull_at_mars - 1.0, "the Drengin colony on Mars held its fire")
		_check(w.starbase_of(p.race_id).get_node_or_null("AI") == null and w.starbase_of(p.race_id).get_children().filter(func(c): return c is AIPilot).is_empty(), "your starbase has no gunner in practice")
		_check_map_click(w)
		main._show_menu()
		main._start_free_flight()
	if mars_from > 0 and frames == mars_from + 1520:
		_check(w.practice and not w.practice_opponent and w.ships.size() == 2 and _pirate(w) == null, "Free flight: just you and your starbase (%d ships)" % w.ships.size())
		print("practice duel: ", "FAILED" if failed else "passed")
		quit(1 if failed else 0)
	return false


## A click on Mars on the Tab map picks Mars, even a little toward a moon.
func _check_map_click(w) -> void:
	var map := TacticalMap.new()
	map.world = w
	main.ui_layer.add_child(map)
	map.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var mars = w.planet_by_name["Mars"]
	var deimos = w.planet_by_name["Deimos"]
	var at: Vector2 = map.to_map(mars.global_position)
	var toward: Vector2 = (map.to_map(deimos.global_position) - at)
	var off := toward.normalized() * minf(6.0, toward.length() * 0.45) if toward.length() > 0.5 else Vector2.ZERO
	var hit: Planet = map.planet_at(at + off)
	_check(hit == mars, "a map click on Mars picks Mars (got %s, moon %.1f px away)" % [hit.body_name if hit else "nothing", toward.length()])
	map.queue_free()


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
