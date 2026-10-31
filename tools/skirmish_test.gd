# Skirmish test (offline): pick Skirmish on the menu and check the mode:
# two empires, four a side, no starbases or economy; a kill counts for the
# killer's side and a shot at a friend counts for nobody; while waiting to
# launch the 5 key picks a Battleship, which launches later than a Scout
# would and in the hull picked; the twentieth kill takes the round, the
# ceasefire holds, and the next round starts level.
#   godot --headless --path . -s tools/skirmish_test.gd
extends SceneTree

var frames := 0
var main: Node
var failed := false
var step := 0
var wait_until := 0
var died_frame := 0
var killer_name := ""


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	await process_frame
	var b: Array = []
	main.menu._set_page("single")   # the mode pages: Single Player and Practice
	await process_frame
	await process_frame
	for mb in main.menu.buttons:
		if mb[1] == "Skirmish":
			b = mb
	_check(not b.is_empty(), "the main menu has a Skirmish button")
	if b.is_empty():
		_done()
		return
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = (b[0] as Rect2).get_center()
	ev.global_position = ev.position
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


func _key(code: Key) -> void:
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


func _physics_process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null or main.world.mode_rules == null:
		if frames > 600:
			_check(false, "Skirmish started")
			_done()
		return false
	var w = main.world
	var sk: Skirmish = w.mode_rules
	var p: Ship = w.player
	if frames < wait_until:
		return false
	match step:
		0:
			step = 1
			_check(w.mode == "skirmish" and w.round_rules == null and not w.records_career(), "Skirmish: no Conquest round, off the record")
			_check(sk.teams[0] != sk.teams[1] and p.race_id == sk.teams[0], "two empires, and you fly for the first")
			_check(sk.roster(0).size() == Skirmish.TEAM_SIZE and sk.roster(1).size() == Skirmish.TEAM_SIZE, "%d a side (%d and %d)" % [Skirmish.TEAM_SIZE, sk.roster(0).size(), sk.roster(1).size()])
			_check(not w.ships.any(func(s): return s.is_station or s.support), "no starbases, stations or supply ships")
			_check(sk.roster(0).all(func(s): return s.design_name in Skirmish.HULLS) and sk.roster(1).all(func(s): return s.design_name in Skirmish.HULLS), "every ship is a ready-built hull")
			var a: Vector2 = sk.arena().global_position
			var mine := Vector2.ZERO
			var theirs := Vector2.ZERO
			for s in sk.roster(0):
				mine += (s.global_position - a) / Skirmish.TEAM_SIZE
			for s in sk.roster(1):
				theirs += (s.global_position - a) / Skirmish.TEAM_SIZE
			_check(mine.dot(theirs) < 0.0, "the sides launch on opposite sides of %s" % Skirmish.ARENA)
			var foe: Ship = sk.roster(1)[0]
			var friend: Ship = sk.roster(0).filter(func(s): return s != p)[0]
			_check(foe.team() != p.team() and friend.team() == p.team(), "friend and foe go by side")
			foe.invulnerable = 0.0
			foe.take_damage(1e6, p)
			friend.invulnerable = 0.0
			friend.take_damage(1e6, p)
			_check(sk.score == [1, 0] and p.kills == 1, "the kill counts for your side, the friend lost for nobody (%s)" % [sk.score])
			w.try_land(p)
			_check(not p.is_landing(), "no landing in Skirmish")
			_check(w.capture.why_not(p, sk.roster(1).filter(func(s): return not s.dead)[0]) == "arena", "no boarding in Skirmish")
			var killer: Ship = sk.roster(1).filter(func(s): return not s.dead)[0]
			killer_name = killer.pilot_name
			p.invulnerable = 0.0
			p.take_damage(1e6, killer)
			died_frame = frames
			wait_until = frames + 3
		1:
			step = 2
			_check(p.dead and sk.killed_by == killer_name, "you died, and the death screen names the killer (%s)" % sk.killed_by)
			_key(KEY_5)
			wait_until = frames + 3
		2:
			step = 3
			_check(sk.picks.get(p, "") == "Battleship", "the 5 key picks a Battleship (%s)" % sk.picks.get(p, ""))
			wait_until = died_frame + int(Skirmish.LAUNCH_DELAY["Scout"] * 60) + 60
		3:
			step = 4
			_check(p.dead, "a Battleship is still launching after a Scout's time")
			wait_until = died_frame + int(Skirmish.LAUNCH_DELAY["Battleship"] * 60) + 60
		4:
			step = 5
			_check(not p.dead and p.design_name == "Battleship", "you launch in a Battleship (%s)" % p.design_name)
			var a: Vector2 = sk.arena().global_position
			var mine := Vector2.ZERO
			for s in sk.roster(0):
				if s != p:
					mine += (s.global_position - a)
			_check((p.global_position - a).dot(mine) > 0.0, "on your own side of the arena")
			# one short of the win, then the win
			sk.score = [Skirmish.KILLS_TO_WIN - 1, 3]
			var foe: Ship = sk.roster(1).filter(func(s): return not s.dead)[0]
			foe.invulnerable = 0.0
			foe.take_damage(1e6, p)
			_check(sk.break_left > 0.0 and sk.winner == 0, "the twentieth kill takes the round")
			wait_until = frames + 60
		5:
			step = 6
			var foe: Ship = sk.roster(1).filter(func(s): return not s.dead)[0]
			var h: float = foe.hull
			foe.take_damage(500.0, p)
			_check(foe.hull == h, "nobody takes damage in the ceasefire")
			sk.break_left = 0.05
			wait_until = frames + 5
		6:
			step = 7
			_check(sk.round_no == 2 and sk.score == [0, 0], "round 2 starts level")
			var all_up: bool = (sk.roster(0) + sk.roster(1)).all(func(s): return not s.dead and s.kills == 0 and s.deaths == 0)
			_check(all_up, "everyone relaunched with a clean slate")
			_done()
	if frames > 4000:
		_check(false, "the test finished in time (step %d)" % step)
		_done()
	return false


func _check(ok: bool, what: String) -> void:
	print("%s: %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


func _done() -> void:
	print("FAILED" if failed else "passed")
	quit(1 if failed else 0)
