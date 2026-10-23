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
	root.add_child(main)
	await process_frame
	await process_frame
	var b: Array = main.menu.buttons[1]
	_check(b[1] == "Practice duel", "second menu button is Practice duel (%s)" % b[1])
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
			_check(d < p.sensor_range and pr.visible_to(p), "the pirate starts inside sensor range (%d of %d)" % [d, p.sensor_range])
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
		print("practice duel: ", "FAILED" if failed else "passed")
		quit(1 if failed else 0)
	return false


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
