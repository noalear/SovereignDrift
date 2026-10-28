# Tutorial test: pick "Tutorial" on the menu with the mouse, check the calm
# Terran world it starts, then do every lesson the way a cadet would (keys
# where they matter, a nudge where flying would take minutes) and check each
# one notices and moves on, to the end.  The voice is fast-forwarded.
#   godot --headless --path . -s tools/tutorial_test.gd
extends SceneTree

var frames := 0
var main: Node
var failed := false
var last_index := -1
var step_frame := 0          # frames since this lesson began
var done_ids := []
var skipped := false
var drone_disabled_hull := -1.0


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
	_lines_check()
	var b: Array = []
	for mb in main.menu.buttons:
		if str(mb[1]).begins_with("Tutorial"):
			b = mb
	_check(not b.is_empty(), "the menu has a Tutorial button")
	if b.is_empty():
		_end()
		return
	_click((b[0] as Rect2).get_center())
	await process_frame
	await process_frame
	_check(main.world != null and main.world.tutorial and main.tutorial != null, "clicking it starts the tutorial")


## Every lesson has its line, every line its recording, and every lesson
## with a goal a check for it.
func _lines_check() -> void:
	var t := Tutorial.new()
	for s in Tutorial.STEPS:
		var id: String = s["id"]
		_check(TutorialLines.text(id) != "", "lesson %s has a line" % id)
		if s.get("goal", "") != "":
			_check(t.has_method("_done_" + id), "lesson %s has a goal check" % id)
	var missing := []
	for row in TutorialLines.LINES:
		var path := TutorialLines.audio_path(row[0])
		var st: AudioStream = load(path) if ResourceLoader.exists(path) else null
		if st == null or st.get_length() < 0.4:
			missing.append(row[0])
	_check(missing.is_empty(), "every line is recorded (%d lines; missing %s)" % [TutorialLines.LINES.size(), missing])
	t.free()


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


func _action(name: String, pressed: bool = true) -> void:
	var ev := InputEventAction.new()
	ev.action = name
	ev.pressed = pressed
	root.push_input(ev, true)


func _key(code: Key, shift := false) -> void:
	var ev := InputEventKey.new()
	ev.keycode = code
	ev.physical_keycode = code
	ev.shift_pressed = shift
	ev.pressed = true
	root.push_input(ev, true)


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel
	s.reset_physics_interpolation()


## Low over a world, at its speed: where a cadet would press B.
func _over(p: Ship, pl: Planet) -> void:
	_put(p, pl.global_position + Vector2(pl.radius + 120.0, 0), pl.velocity)


func _process(_delta: float) -> bool:
	frames += 1
	if frames > 60000:
		_check(false, "the tutorial finished in time (stuck at %s)" % (main.tutorial.step.get("id", "?") if main.tutorial != null else "?"))
		_end()
		return true
	if main == null or main.world == null or main.tutorial == null:
		return false
	var w: World = main.world
	var t: Tutorial = main.tutorial
	var p: Ship = w.player
	if p == null:
		return false
	p.invulnerable = 1e9
	if frames == 5:
		_world_check(w, p)
	# Hale talks fast in a test
	if t.speaking():
		t.voice.stop()
		t.line_time = t.line_len
	if t.finished:
		_check(done_ids.size() == Tutorial.STEPS.size(), "every lesson came round (%d of %d)" % [done_ids.size(), Tutorial.STEPS.size()])
		_check(skipped, "Enter skipped a lesson")
		_end()
		return true
	if t.index != last_index:
		last_index = t.index
		step_frame = 0
		done_ids.append(t.step["id"])
	step_frame += 1
	_lesson(w, t, p, t.step["id"], step_frame)
	return false


func _world_check(w: World, p: Ship) -> void:
	_check(p.race_id == 1 and p.design_name == "Scout", "a Terran cadet in a Scout (race %d, %s)" % [p.race_id, p.design_name])
	_check(w.round_rules == null, "no round clock")
	_check(w.starbase_of(1) != null, "the Terran starbase is there")
	var enemies := w.ships.filter(func(s): return s.race_id != 1 and s.race_id != 4)
	_check(enemies.is_empty(), "no enemy ships to start with (%d)" % enemies.size())
	var haulers := w.ships.filter(func(s): return s.support and s.race_id == 1)
	_check(haulers.size() == 3, "our two supply ships and trade ship are at work (%d)" % haulers.size())
	_check(w.ships.any(func(s): return s.design_name == "TradeStation"), "the free port is open")
	_check(not w.records_career(), "nothing here reaches the career")


func _lesson(w: World, t: Tutorial, p: Ship, id: String, f: int) -> void:
	var earth: Planet = w.planet_by_name["Earth"]
	var luna: Planet = w.planet_by_name["Luna"]
	if f != 3 and not id in ["warp", "map", "board", "land", "to_luna", "colonists", "unload", "stealth", "pdc", "jammer", "shield_focus", "shields_down", "shipyard_tour", "loot", "ground_tabs", "assist", "groups"]:
		return
	match id:
		"thrust":
			var base := w.starbase_of(1)
			_put(p, base.global_position + Vector2(1500, 0), base.velocity)
			t.turned = 2.0
			t.held["thrust"] = 1.0
		"strafe":
			t.held["strafe_left"] = 0.5
			t.held["reverse"] = 0.5
		"brake":
			t.held["brake"] = 0.5
			_put(p, p.global_position, p.frame_velocity())
		"boost":
			t.held["boost_thrust"] = 1.5
		"assist":
			if f == 3 or f == 10:
				_action("flight_assist")
				_action("flight_assist", false)
		"beacon":
			_put(p, t.beacon_position(), earth.velocity)
		"nav":
			for i in 40:
				if p.nav_target == luna:
					break
				w.cycle_nav(p)
		"autopilot":
			_put(p, luna.global_position + Vector2(luna.radius + 300.0, 0), luna.velocity)
		"warp":
			if f == 3:
				_put(p, earth.global_position + Vector2(0, -6000), earth.velocity)
				_action("warp")
				_action("warp", false)
			elif f == 30:
				_check(p.warping, "the quantum drive engages")
				_action("warp")
				_action("warp", false)
		"map":
			if f == 3:
				_action("tactical_map")
			elif f == 4:
				_action("tactical_map", false)
			elif f == 10:
				_check(main.tactical != null, "the system map opens")
				_action("tactical_map")
			elif f == 11:
				_action("tactical_map", false)
		"info":
			w.inspected_planet = luna
		"zoom":
			# skip this one with Enter, as a cadet in a hurry would
			_key(KEY_ENTER)
			skipped = true
		"target":
			_check(t.drone != null and t.drone.race_id == 3 and not t.drone.dead, "a target drone is out")
			_check(t.drone.global_position.distance_to(p.global_position) < p.sensor_range, "the drone is in sensor range")
			_action("target_nearest")
			_action("target_nearest", false)
		"fire":
			t.drone.take_damage(t.drone.shield_max + 20.0, p, t.drone.global_position)
		"groups":
			if f == 3:
				_action("weapon_group_4")
				_action("weapon_group_4", false)
			elif f == 10:
				_check(p.weapon_group == 4, "group 4 is selected")
				_action("weapon_group_1")
				_action("weapon_group_1", false)
		"disable":
			# hits on the stern, as Hale says
			var d := t.drone
			for i in 400:
				if d.dead or d.is_disabled():
					break
				d.take_damage(40.0, p, d.global_position - d.forward() * d.size * 0.8)
			_check(not d.dead and d.is_disabled(), "stern hits disable the drone before it blows up (hull %d / %d)" % [d.hull, d.hull_max])
		"board":
			if f == 3:
				var d := t.drone
				_put(p, d.global_position + Vector2(d.size + 60.0, 0), d.velocity)
				p.target = d
				_action("board")
				_action("board", false)
			elif f == 2000:
				_check(false, "boarding the drone finishes (progress %d%%)" % int(p.board_progress * 100.0))
		"stealth":
			if f == 3:
				_action("silent_running")
				_action("silent_running", false)
			elif f == 10:
				_check(p.sensors.emcon, "silent running goes on")
				_action("silent_running")
				_action("silent_running", false)
		"pdc":
			if f == 3:
				_check(p.pdc.fitted(), "the yard fits a PDC for the PDC lesson")
				_check(t.launcher != null and not t.launcher.dead, "a missile launcher comes in for the PDC lesson")
			elif t.pdc_fired >= 1 and p.pdc.shot_down - t.pdc_kills_at >= Tutorial.PDC_KILLS and p.pdc.enabled and not t.seen.has("pdc_off"):
				_action("pdc_toggle")
				_action("pdc_toggle", false)
			elif t.seen.has("pdc_off") and not p.pdc.enabled:
				_action("pdc_toggle")
				_action("pdc_toggle", false)
			elif f == 3000:
				_check(false, "the PDC shoots the lesson's missiles down (%d of %d down, %d fired)" % [p.pdc.shot_down - t.pdc_kills_at, Tutorial.PDC_KILLS, t.pdc_fired])
		"jammer":
			var pr := t.pirate
			if f == 3:
				_check(pr != null and not pr.dead, "the pirate comes in for the jammer lesson")
				_put(pr, p.global_position + Vector2(700.0, 0), p.velocity)
				p.target = pr
			elif f == 90:            # the radar is back from silent running
				_check(p.has_track(pr), "the pirate is locked at 700 px")
				_action("cloak")
				_action("cloak", false)
			elif f == 120:
				_check(p.jamming, "the jammer goes on against the locked pirate")
			elif f == 400:
				_check(false, "the jam bites (jamming %s)" % p.jamming)
		"shield_focus":
			if f == 3 or f == 10 or f == 17:
				_action("shield_focus")
				_action("shield_focus", false)
			elif f == 6:
				_check(p.shield_focus == 1, "the focus key swings the shield forward")
		"shields_down":
			if f == 3:
				_action("shields_toggle")
				_action("shields_toggle", false)
			elif f == 6:
				_check(not p.shields_raised and p.shield == 0.0, "lowered shields hold nothing")
				_action("shields_toggle")
				_action("shields_toggle", false)
		"spar":
			var pr := t.pirate
			_check(pr != null and not pr.dead and pr.get_node_or_null("AI") != null, "a pirate with a pilot comes in")
			_check(not pr.can_jam, "the rookie pirate does not jam you")
			pr.take_damage(1e6, p)
		"loot":
			if f == 3:
				_check(not w.powerups.is_empty(), "there is loot to pick up")
			if f >= 3 and f % 20 == 3 and not w.powerups.is_empty():
				var pu: Node2D = w.powerups[0]
				_put(p, pu.global_position, pu.velocity)
		"dock":
			var base := w.starbase_of(1)
			_put(p, base.global_position + Vector2(base.size + 120.0, 0), base.velocity)
		"shipyard":
			_action("refit")
			_action("refit", false)
		"shipyard_tour":
			if f == 3:
				_check(main.builder != null and w.paused, "the shipyard opens and the game waits")
			elif f == 30 and main.builder != null:
				main.builder._cancel()
		"base_cargo":
			_action("cargo")
			_action("cargo", false)
		"land":
			if f == 3:
				main._close_cargo()
				_over(p, earth)
				_action("land")
				_action("land", false)
		"colonists":
			if f == 3:
				_check(p.is_landed() and p.land_body == earth, "landed on Earth")
				_action("colonists_board")
				_action("colonists_board", false)
		"cargo":
			_check(Economy.RESOURCES.has(t.cargo_res), "the cargo lesson names a resource (%s)" % t.cargo_res)
			w.cargo_command(p, t.cargo_res, 10.0)
		"ground_tabs":
			if f == 3 and main.cargo_panel == null:
				_action("cargo")
				_action("cargo", false)
			elif f == 10:
				_check(main.cargo_panel != null, "the cargo panel opens on the ground")
				if main.cargo_panel != null:
					main.cargo_panel.tab = "troops"
		"to_luna":
			if f == 3:
				main._close_cargo()
			if f % 60 != 3:
				return
			if p.is_landed() and p.land_body == earth:
				_action("land")            # lift off
				_action("land", false)
			elif not p.is_landing():
				_over(p, luna)
				_action("land")
				_action("land", false)
		"unload":
			if f == 3:
				_check(p.is_landed() and p.land_body == luna, "landed on Luna")
				_check(p.colonists > 0.0 and p.cargo_total() > 0.0, "colonists and cargo aboard (%s, %d)" % [str(int(p.colonists)), int(p.cargo_total())])
				_action("colonists_unload")
				_action("colonists_unload", false)
				for k in p.cargo:
					w.cargo_command(p, k, -p.cargo[k])
		"ledger":
			_action("ledger")
			_action("ledger", false)
		"freeport":
			_check(not t._mark().is_empty(), "the free port is marked")
		"haulers":
			_check(not t._mark().is_empty(), "a supply ship is marked")


func _check(ok: bool, what: String) -> void:
	print(("ok   " if ok else "FAIL ") + what)
	if not ok:
		failed = true


func _end() -> void:
	print("TUTORIAL TEST " + ("FAILED" if failed else "PASSED"))
	quit(1 if failed else 0)
