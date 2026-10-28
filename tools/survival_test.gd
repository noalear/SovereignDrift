# Survival test (offline): pick Survival on the menu, then play a short
# run at speed: the first wave arrives, every raider is on one side, kills
# count and pay, a cleared wave mends the starbase and opens a break, the
# player relaunches at the starbase, the boss waves bring big hulls, and
# when the starbase falls the run ends, the best is kept on the profile
# and a new run starts.  It backs up and restores user://settings.cfg.
#   godot --headless --path . -s tools/survival_test.gd
extends SceneTree

const CFG := "user://settings.cfg"

var frames := 0
var main: Node
var failed := false
var step := 0
var backup := PackedByteArray()
var had_cfg := false
var wait_until := 0


func _init() -> void:
	had_cfg = FileAccess.file_exists(CFG)
	if had_cfg:
		backup = FileAccess.get_file_as_bytes(CFG)
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
	var settings = root.get_node("/root/Settings")
	settings.race_id = 2            # Drengin: the starbase is at Mars
	var b: Array = []
	for mb in main.menu.buttons:
		if mb[1] == "Survival":
			b = mb
	_check(not b.is_empty(), "the main menu has a Survival button")
	if b.is_empty():
		_done()
		return
	_click((b[0] as Rect2).get_center())
	await process_frame
	await process_frame
	_check(main.world != null and main.world.mode == "survival" and main.world.mode_rules is Survival, "clicking it starts Survival")


func _click(at: Vector2) -> void:
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = at
	ev.global_position = at
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


func _physics_process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null or main.world.mode_rules == null:
		if frames > 600:
			_check(false, "Survival started")
			_done()
		return false
	var w = main.world
	var sv: Survival = w.mode_rules
	var p: Ship = w.player
	if frames < wait_until:
		return false
	match step:
		0:
			step = 1
			_check(w.round_rules == null and w.is_arena() and not w.records_career(), "no Conquest round, and off the record")
			var b: Ship = sv.base()
			_check(b != null and b.race_id == 2 and w.ships.size() == 2, "just the Drengin starbase and you (%d ships)" % w.ships.size())
			_check(sv.wave == 0 and sv.break_left > 10.0, "a break before wave 1 (%.0f s)" % sv.break_left)
			_check(p.global_position.distance_to(b.global_position) < 600.0, "you launch at the starbase")
			w.try_land(p)
			_check(not p.is_landing(), "no landing in Survival")
			sv.break_left = 0.05
			wait_until = frames + 5
		1:
			step = 2
			var raiders := sv.raiders()
			_check(sv.wave == 1 and raiders.size() == Survival.roster(1, 1).size() and raiders.size() == 3, "wave 1 brings %d raiders (%d)" % [Survival.roster(1, 1).size(), raiders.size()])
			var sides := {}
			for s in raiders:
				sides[s.team()] = true
				_check(s.race_id != 2, "a raider is not of the defenders' empire (%s)" % s.pilot_name)
				var d: float = s.global_position.distance_to(sv.base().global_position)
				_check(d > Survival.ARRIVE_MIN - 10.0, "raiders arrive out of range of the starbase (%d)" % d)
			_check(sides.size() == 1 and sides.has(Survival.ENEMY_TEAM), "every raider is on one side (%s)" % [sides.keys()])
			_check(w.nearest_known_enemy(p) != null, "the defenders know where the wave is")
			var c0 := p.credits
			for s in raiders:
				s.invulnerable = 0.0
				s.take_damage(1e6, p)
			_check(sv.run_kills(p) == raiders.size(), "your kills count for the run (%d)" % sv.run_kills(p))
			_check(p.credits > c0, "the kills paid bounties (%d -> %d)" % [c0, p.credits])
			var b: Ship = sv.base()
			b.hull = b.hull_max * 0.5
			wait_until = frames + 5
		2:
			step = 3
			_check(sv.break_left > Survival.BREAK_SECONDS - 1.0 and sv.enemies_left == 0, "a cleared wave opens the break (%.1f s)" % sv.break_left)
			var b: Ship = sv.base()
			_check(absf(b.hull - b.hull_max * (0.5 + Survival.BASE_MEND)) < 1.0, "the starbase got some hull back")
			_check(sv.raiders().is_empty() and not w.ships.any(func(s): return s.has_meta("wave")), "the wrecks' raiders are gone from the world")
			_check(int(w.get_node("/root/Settings").career.get("survival_best", 0)) >= 1, "the best run is on the profile")
			_check(w.nearest_base(p) != null or p.global_position.distance_to(b.global_position) > 300.0, "the starbase is a shipyard")
			# the boss waves
			var r5 := Survival.roster(5, 1)
			var r10 := Survival.roster(10, 2)
			var r15 := Survival.roster(15, 4)
			_check(r5[0] == "Battleship" and r10[0] == "BattleCruiser" and r15[0] == "Dreadnought", "bosses lead waves 5, 10 and 15 (%s, %s, %s)" % [r5[0], r10[0], r15[0]])
			_check(r15.size() == Survival.MAX_WAVE_SHIPS and Survival.roster(4, 1).size() < Survival.roster(4, 3).size(), "waves grow with the wave and the pilots, up to %d" % Survival.MAX_WAVE_SHIPS)
			_check(r5.has("Cruiser") and not Survival.roster(2, 1).has("Cruiser"), "heavier hulls come later")
			# you die: back at the starbase
			p.invulnerable = 0.0
			p.take_damage(1e6, null)
			wait_until = frames + int(World.RESPAWN_DELAY * 60) + 30
		3:
			step = 4
			_check(not p.dead and p.global_position.distance_to(sv.base().global_position) < 600.0, "you relaunch at the starbase")
			sv.break_left = 0.05
			wait_until = frames + 5
		4:
			step = 5
			_check(sv.wave == 2 and sv.raiders().size() == Survival.roster(2, 1).size(), "wave 2 arrives (%d raiders)" % sv.raiders().size())
			var b: Ship = sv.base()
			b.invulnerable = 0.0
			b.take_damage(1e7, sv.raiders()[0])
			wait_until = frames + 5
		5:
			step = 6
			_check(sv.over and sv.restart_left > 0.0, "the starbase fell: the run is over")
			_check(sv.held() == 1 and sv.best >= 1, "the run held 1 wave (best %d)" % sv.best)
			p.invulnerable = 0.0
			p.take_damage(1e6, null)
			wait_until = frames + int((World.RESPAWN_DELAY + 1.0) * 60)
		6:
			step = 7
			_check(p.dead, "no relaunch while the starbase is down")
			_check(sv.base() == null, "the starbase stays down until the new run")
			sv.restart_left = 0.05
			wait_until = frames + 5
		7:
			step = 8
			_check(sv.run == 2 and sv.wave == 0 and not sv.over, "a new run starts")
			_check(sv.base() != null and sv.base().hull == sv.base().hull_max, "the starbase is back at full strength")
			_check(not p.dead and sv.run_kills(p) == 0 and sv.raiders().is_empty(), "you are back with a clean tally and no raiders about")
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
	# put the player's real settings back
	if had_cfg:
		var f := FileAccess.open(CFG, FileAccess.WRITE)
		f.store_buffer(backup)
		f.close()
	elif FileAccess.file_exists(CFG):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(CFG))
	print("FAILED" if failed else "passed")
	quit(1 if failed else 0)
