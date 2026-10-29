# Story campaign test: "New story" starts a Terran war under the Ceres
# Accord (the empires hold their fire, a shot across it makes the shooter
# fair game), Hale calls the player in, the briefing pauses the war and
# launches the operation, and every operation of Acts 1 and 2 plays to the
# end the way a pilot would (teleports where flying would take minutes),
# with both answers to the choices and the fail-forward endings.  Also: a
# failed operation is tried again from the save made as it began, the
# story's place survives a save, and every line has its recording.
# Backs up and restores user://story.save and user://settings.cfg.
#   godot --headless --path . -s tools/story_test.gd
extends SceneTree

var main: Node
var failed := false
const SAVE := "user://story.save"
const SETTINGS := "user://settings.cfg"


func _init() -> void:
	call_deferred("_start")


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _backup(path: String) -> void:
	if FileAccess.file_exists(path):
		DirAccess.copy_absolute(ProjectSettings.globalize_path(path), ProjectSettings.globalize_path(path + ".testbak"))


func _restore(path: String) -> void:
	var bak := path + ".testbak"
	if FileAccess.file_exists(bak):
		DirAccess.copy_absolute(ProjectSettings.globalize_path(bak), ProjectSettings.globalize_path(path))
		DirAccess.remove_absolute(ProjectSettings.globalize_path(bak))
	elif FileAccess.file_exists(path) and path == SAVE:
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))


## Run the world; the voice is fast-forwarded so the story never waits on it.
func _frames(n: int) -> void:
	for i in n:
		var w: World = main.world
		if w != null and w.story != null:
			w.story.line_left = minf(w.story.line_left, 0.02)
			if w.player != null:
				w.player.invulnerable = 1e9
		await physics_frame


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel
	s.reset_physics_interpolation()


func _action(name: String) -> void:
	var ev := InputEventAction.new()
	ev.action = name
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	_backup(SAVE)
	_backup(SETTINGS)
	if FileAccess.file_exists(SAVE):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(SAVE))
	root.add_child(main)
	await process_frame
	await process_frame
	_lines_check()
	var newb: Array = []
	var contb: Array = []
	for b in main.menu.buttons:
		# the story's row comes first: New, Continue, Load
		if b[1] == "New" and newb.is_empty():
			newb = b
		elif b[1] == "Continue" and contb.is_empty():
			contb = b
	_check(not newb.is_empty(), "the menu has New story")
	_check(not contb.is_empty() and contb[3] == false, "Continue story is greyed out with no save")
	main._start_game(false, true, "story_new")
	await process_frame
	await process_frame
	var w: World = main.world
	_check(w != null and w.story_mode and w.story != null, "New story starts the story campaign")
	_check(w.player.race_id == 1, "the story is flown for the Terrans")
	_check(w.accord, "the Ceres Accord holds at the start")
	_check(FileAccess.file_exists(SAVE), "a new story saves at once")
	_check(main.comms != null and main.story_panel != null, "the call window and the objectives are up")
	var from := OS.get_environment("STORY_FROM")     # e.g. STORY_FROM=8 runs ops 9 on (quick reruns)
	if from != "":
		w.accord = false
		w.story.index = int(from)
		w.story.flags["arceans"] = "allied"
		w.story.keep_peace()
		for f in [_silent_running, _burn_yards].slice(maxi(int(from) - 8, 0)):
			await f.call()
		_end()
		return
	await _accord_check(w)
	await _first_watch()
	await _slots_check()
	await _retry_check()
	await _dark_convoy()
	await _free_port()
	await _picket_line()
	await _accord_breaks()
	await _shields_down()
	await _wolf_pack()
	await _envoy()
	await _silent_running()
	await _burn_yards()
	_end()


## Every line has words, a speaker, a place and a recording; every
## operation's calls and briefings are lines.
func _lines_check() -> void:
	var missing := []
	for row in StoryLines.LINES:
		_check(StoryLines.SPEAKERS.has(row[1]) and StoryLines.PLACES.has(row[2]) and str(row[3]).length() > 5, "line %s is complete" % row[0])
		var path := StoryLines.audio_path(row[0])
		var st: AudioStream = load(path) if ResourceLoader.exists(path) else null
		if st == null or st.get_length() < 0.4:
			missing.append(row[0])
	_check(missing.is_empty(), "every story line is recorded (%d lines; missing %s)" % [StoryLines.LINES.size(), missing])
	for op in Story.OPS:
		for id in [op["call"]] + op["brief"]:
			_check(StoryLines.text(id) != "", "operation %s has its line %s" % [op["id"], id])
		_check(ResourceLoader.exists(op["script"]), "operation %s has its script" % op["id"])


## Under the Accord a Drengin warship beside a Terran one leaves it alone;
## once the Terran fires, it shoots back.
func _accord_check(w: World) -> void:
	var p := w.player
	var earth: Planet = w.planet_by_name["Earth"]
	var at := earth.global_position + Vector2(earth.radius + 2600.0, 0)
	var t := w.spawn_ship("Test Terran", 1, "Destroyer", "TSS Test", at, earth.velocity, 6)
	var d := w.spawn_ship("Test Drengin", 2, "Destroyer", "Test Claw", at + Vector2(500, 0), earth.velocity, 6)
	for s in [t, d]:
		s.get_node("AI").bombard = false
		s.get_node("AI").ferry = false
	var hull_t := t.hull
	var hull_d := d.hull
	await _frames(240)
	_check(t.hull >= hull_t - 0.1 and d.hull >= hull_d - 0.1, "under the Accord a Terran and a Drengin warship side by side hold their fire")
	_check(d.target != t and t.target != d, "and neither targets the other")
	d.take_damage(5.0, t)
	await _frames(1)
	_check(t.has_meta("breach_at"), "a shot across the Accord marks the shooter")
	for i in 16:
		await _frames(30)
		if d.target == t:
			break
	_check(d.target == t, "and the victim answers it")
	if d.target != t:
		print("  d.target=", d.target, " state=", d.get_node("AI").state, " dead=", d.dead, " t.dead=", t.dead, " dist=", d.global_position.distance_to(t.global_position))
	w.remove_ship(t)
	w.remove_ship(d)
	_check(p != null, "the player is still about")


func _first_watch() -> void:
	var w: World = main.world
	var st: Story = w.story
	st.unlock_at = w.elapsed
	await _frames(10)
	_check(st.briefing_ready() and st.called, "Hale calls the player in when the first briefing is ready")
	_action("briefing")
	await process_frame
	await process_frame
	_check(main.briefing != null, "the briefing key opens the briefing")
	_check(w.paused, "the war waits while the briefing is open")
	_check(st.line == "fw_brief_1" or st.queue.has("fw_brief_2"), "Hale speaks the briefing")
	main.briefing.launch.emit()
	await process_frame
	_check(main.briefing == null and not w.paused, "Launch closes the briefing and the war goes on")
	_check(st.op != null and st.op.spec["id"] == "first_watch", "the first operation is under way")
	var op: StoryOp = st.op
	var p := w.player
	var cormorant: Ship = op.get("cormorant")
	var brandt: Ship = op.get("brandt")
	_check(cormorant != null and brandt != null, "the Cormorant and Lt. Brandt are there")
	_check(st.objectives().size() >= 1 and not st.marks().is_empty(), "the first objective and its mark")
	_put(p, cormorant.global_position + Vector2(200, 0), cormorant.velocity)
	await _frames(20)
	_check(op.done("meet") and op.stage == "escort", "meeting the Cormorant starts the escort")
	_check(cormorant.autopilot, "the Cormorant sets off for the free port")
	var port: Ship = op.get("port")
	# part way: the Drengin scout comes to look
	var half: Vector2 = cormorant.global_position.lerp(port.global_position, 0.6)
	_put(cormorant, half, cormorant.velocity)
	_put(p, half + Vector2(150, 150), cormorant.velocity)
	await _frames(60 * 8)
	var scout: Ship = op.get("scout")
	_check(scout != null and is_instance_valid(scout) and op.stage == "shadowed", "a Drengin scout shadows the flight")
	_check(scout != null and is_instance_valid(scout) and scout.target != p and p.last_attacker != scout, "the scout never fires under the Accord")
	var flying := cormorant.velocity.length()
	_check(cormorant.autopilot and not cormorant.warping, "the Cormorant flies on without warping ahead (%.0f px/s)" % flying)
	# the port: the scout jams, leaves, and the flight goes home
	_put(cormorant, port.global_position + Vector2(400.0, 0), port.velocity)
	_put(p, port.global_position + Vector2(550.0, 0), port.velocity)
	await _frames(30)
	_check(op.done("escort"), "the Cormorant arrives at the free port")
	await _frames(60 * 6)
	_check(op.stage == "home", "then home to the starbase")
	var base := w.starbase_of(1)
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(st.op == null and st.index == 1 and st.flags.get("first_watch", "") == "won", "First Watch is won and the story moves on")
	_check(not is_instance_valid(cormorant) or not w.ships.has(cormorant), "the operation's ships leave when it ends")
	var d := SaveGame.read(SAVE)
	_check(int(d.get("story", {}).get("index", -1)) == 1 and bool(d["story"].get("accord", false)), "the save knows the story's place and the Accord")


## Save slots from the pause menu: a new one with its picture, Overwrite,
## Load (the war as it was in the slot), Delete.
func _slots_check() -> void:
	var w: World = main.world
	var before := SaveGame.slots().size()
	main._open_pause()
	main._open_saves("save", null)
	await process_frame
	var ss: StorySaves = main.story_saves
	_check(ss != null and ss.mode == "save" and not main.pause_panel.visible, "Save the story opens the save slots over the pause menu")
	var img := Image.create(640, 360, false, Image.FORMAT_RGB8)
	img.fill(Color(0.2, 0.4, 0.8))
	ss.shot = img
	var path := SaveGame.new_slot_path()
	ss._save(path)
	var all := SaveGame.slots()
	var mine: Dictionary = {}
	for sl in all:
		if sl["path"] == path:
			mine = sl
	_check(all.size() == before + 1 and not mine.is_empty(), "a new save slot")
	_check(not mine.is_empty() and mine["shot"] != null and mine["shot"].get_width() == SaveGame.SHOT_SIZE.x, "with its screenshot")
	_check(not mine.is_empty() and int(mine["info"]["op"]) == 1, "and the story's place (operation 2 next)")
	w.story.flags["overwritten"] = true
	ss._save(path)
	_check(SaveGame.slots().size() == before + 1 and SaveGame.read(path)["story"]["flags"].has("overwritten"), "Overwrite replaces the slot")
	w.story.flags.erase("overwritten")
	var path2 := SaveGame.new_slot_path()
	ss._save(path2)
	_check(SaveGame.slots().size() == before + 2, "as many slots as you like")
	ss._delete(path2)
	_check(SaveGame.slots().size() == before + 2, "Delete asks first")
	ss._delete(path2)
	_check(SaveGame.slots().size() == before + 1 and not FileAccess.file_exists(path2.get_basename() + ".png"), "and deletes on the second press")
	ss.closed.emit()
	await process_frame
	_check(main.story_saves == null and main.pause_panel.visible, "Back returns to the pause menu")
	# change the story, then load the slot: the story is as saved
	w.story.flags["test_mark"] = true
	main.load_story(path)
	await process_frame
	await process_frame
	w = main.world
	_check(w != null and w.story != null and w.story.index == 1 and not w.story.flags.has("test_mark"), "Load puts the war back as the slot has it")
	_check(main.pause_panel == null and not w.paused and not paused, "and play goes on")
	_check(SaveGame.has_slots(), "the title screen's Load has a slot to offer")
	SaveGame.delete_slot(path)


## Firing on an Accord ship fails First Watch; the retry is the war as it
## was saved when the operation began, with its briefing.
func _retry_check() -> void:
	var w: World = main.world
	var st: Story = w.story
	# put the story back to First Watch for this check
	st.index = 0
	st.unlock_at = w.elapsed
	await _frames(2)
	main.open_briefing()
	await process_frame
	main.briefing.launch.emit()
	await process_frame
	_check(st.op != null, "First Watch again")
	var p := w.player
	var drengin: Ship = null
	for s in w.ships:
		if s.race_id == 2 and not s.dead and not s.is_station and not s.support:
			drengin = s
			break
	_check(drengin != null, "a Drengin ship to fire on")
	drengin.take_damage(3.0, p)
	await _frames(5)
	_check(st.op == null and st.failed != "", "firing first on an Accord ship fails the operation")
	_check(w.save_campaign(false), "saving during a failed operation keeps the save from its start")
	_action("briefing")
	await process_frame
	await process_frame
	await process_frame
	w = main.world
	_check(w != null and w.story != null and w.story.failed == "" and w.story.index == 0, "the briefing key tries again from the save")
	_check(main.briefing != null, "with the briefing open")
	if main.briefing != null:
		main._close_briefing()
	# and past it again for the second operation
	w.story.index = 1
	w.story.flags["first_watch"] = "won"
	w.story.unlock_at = w.elapsed


func _dark_convoy() -> void:
	var w: World = main.world
	var st: Story = w.story
	await _frames(5)
	main.open_briefing()
	await process_frame
	_check(main.briefing != null and st.current()["id"] == "dark_convoy", "the Dark Convoy briefing")
	main.briefing.launch.emit()
	await process_frame
	var op: StoryOp = st.op
	_check(op != null and op.spec["id"] == "dark_convoy", "Dark Convoy is under way")
	var p := w.player
	var site: Vector2 = op.call("_site")
	_put(p, site, w.planet_by_name["Ceres"].velocity)
	await _frames(10)
	_check(op.done("site"), "reaching the convoy's last position")
	for wr in op.get("wrecks"):
		var ship: Ship = wr[0]
		_check(ship.is_disabled(), "the %s is dead in space" % ship.ship_name)
		for i in 150:
			_put(p, ship.global_position + Vector2(ship.size + 120.0, 0), ship.velocity)
			await _frames(1)
			if op.done(wr[1]):
				break
		_check(op.done(wr[1]), "scanning the %s" % ship.ship_name)
	await _frames(60 * 5)
	var lead: Ship = op.get("lead")
	_check(lead != null and op.stage == "fight", "raiders come out of the rocks")
	# its engines go: it stays dead in space for the boarders
	lead.hull = lead.hull_max * 0.3
	await _frames(20)
	_check(lead.is_disabled() and op.done("disable"), "knocking the lead raider's engines out")
	_put(p, lead.global_position + Vector2(lead.size + 60.0, 0), lead.velocity)
	w.capture.take(p, lead)
	await _frames(10)
	_check(op.done("board") and op.stage == "home", "boarding it, then home")
	var base := w.starbase_of(1)
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(st.op == null and st.index == 2 and st.flags.get("drengin_log", false), "Dark Convoy is won: the Drengin log")


## Offer the next operation now and launch it from its briefing.
func _launch(id: String) -> StoryOp:
	var w: World = main.world
	var st: Story = w.story
	st.unlock_at = w.elapsed
	await _frames(3)
	main.open_briefing()
	await process_frame
	_check(main.briefing != null and st.current()["id"] == id, "the %s briefing" % id)
	if main.briefing != null:
		main.briefing.launch.emit()
	await process_frame
	_check(st.op != null and st.op.spec["id"] == id, "%s is under way" % id)
	return st.op


## Wait for the choice card, then answer it.
func _answer(key: String) -> void:
	for i in 120:
		await process_frame
		if main.story_choice != null:
			break
		var w: World = main.world
		if w != null and w.story != null:
			w.story.line_left = minf(w.story.line_left, 0.02)
	_check(main.story_choice != null and main.world.paused, "the choice comes up and the war waits on it")
	if main.story_choice != null:
		main.story_choice.chosen.emit(key)
	await process_frame


func _free_port() -> void:
	var w: World = main.world
	var op := await _launch("free_port")
	var p := w.player
	var port: Ship = op.get("port")
	_put(p, port.global_position + Vector2(900, 0), port.velocity)
	await _frames(5)
	_check(op.done("port") and op.stage == "choose" and not w.story.choice.is_empty(), "at the port Marrow names her price")
	await _answer("steal")
	_check(op.stage == "steal" and op.get("cutters").size() == 2 and op.get("relay") != null, "copying: port security circles the port, the relay behind it")
	await _frames(60)
	_check(w.story.op != null and w.story.failed == "", "being seen at the port itself is fine")
	# loud at the relay: seen
	var relay: Node2D = op.get("relay")
	for i in 30:
		_put(p, relay.global_position, port.velocity)
		await _frames(1)
		if w.story.op == null:
			break
	_check(w.story.op == null and w.story.failed != "", "running loud past port security fails the operation")
	main.retry_operation()
	for i in 4:
		await process_frame
	w = main.world
	_check(main.briefing != null and w.story.current()["id"] == "free_port", "the retry opens the Free Port briefing")
	main.briefing.launch.emit()
	await process_frame
	op = w.story.op
	p = w.player
	port = op.get("port")
	_put(p, port.global_position + Vector2(900, 0), port.velocity)
	await _frames(5)
	await _answer("steal")
	p.set_silent(true)
	for i in 60 * 6:
		_put(p, port.global_position + Vector2(900, 0), port.velocity)
		await _frames(1)
	relay = op.get("relay")
	var seen := false
	for i in 60 * 7:
		_put(p, relay.global_position, port.velocity)
		await _frames(1)
		if w.story.op == null or op.done("steal"):
			break
	for c in op.get("cutters"):
		if is_instance_valid(c):
			seen = seen or Sensors.quality(c, p) >= 2
	_check(op.done("steal") and w.story.failed == "", "silent on the relay, the copy goes through unseen (failed: %s)" % w.story.failed)
	_check(w.story.flags.get("port_fees", "") == "high" and Economy.markup(port, p) > 0.0, "and the port charges Terrans more from now on")
	_check(op.stage == "home", "then home with the records")
	var base := w.starbase_of(1)
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 3 and w.story.flags.get("choice_free_port", "") == "steal", "Free Port is won the quiet way")
	# and the other way: pay her
	w.story.index = 2
	op = await _launch("free_port")
	p.set_silent(false)
	_put(p, port.global_position + Vector2(900, 0), port.velocity)
	await _frames(5)
	await _answer("pay")
	_check(op.stage == "pay", "paying: buy Exotics at the port")
	p.cargo["exotics"] = 8.0
	_put(p, port.global_position + Vector2(port.size + 200.0, 0), port.velocity)
	await _frames(5)
	_check(op.done("pay") and p.cargo.get("exotics", 0.0) < 0.5 and op.stage == "home", "Marrow takes the Exotics and hands over the records")
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 3 and w.story.flags.get("choice_free_port", "") == "pay", "Free Port is won by paying")


func _picket_line() -> void:
	var w: World = main.world
	var op := await _launch("picket_line")
	var p := w.player
	var frame: Ship = op.get("frame")
	_check(frame != null and frame.building and frame.station_body == w.planet_by_name["Luna"], "a station frame waits over Luna")
	_check(w.planet_by_name["Earth"].stock.get("alloys", 0.0) >= 8.0, "with the Alloys waiting on Earth")
	p.cargo["alloys"] = 8.0
	await _frames(3)
	_check(op.stage == "deliver", "Alloys aboard: on to the frame")
	_put(p, frame.global_position + Vector2(frame.size + 120.0, 0), frame.velocity)
	await _frames(2)
	w.cargo_command(p, "alloys", -8.0)
	for i in 600:
		_put(p, frame.global_position + Vector2(frame.size + 120.0, 0), frame.velocity)
		await _frames(1)
		if op.stage == "build":
			break
	_check(op.stage == "build" and frame.cargo.get("alloys", 0.0) >= 7.99, "unloading the Alloys into the frame starts the build (%s)" % frame.cargo)
	op.stage_time = 21.0
	await _frames(3)
	var raiders: Array = op.get("raiders")
	_check(raiders.size() == 3 and raiders[0].race_id == 3, "\"pirates\" come for the frame")
	for r in raiders:
		r.take_damage(1e6, p)
	await _frames(60 * 10)
	_check(not frame.building and not frame.dead and op.stage == "home", "the raid beaten, the station is finished")
	var base := w.starbase_of(1)
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 4 and w.ships.has(frame) and not frame.has_meta("story"), "Picket Line is won, and Luna keeps its station")


func _accord_breaks() -> void:
	var w: World = main.world
	var heard := []
	w.story.line_started.connect(func(id): heard.append(id))
	var op := await _launch("accord_breaks")
	var p := w.player
	var luna: Planet = w.planet_by_name["Luna"]
	_check(w.accord, "the Accord holds while Kesh Varr speaks")
	await _frames(60 * 5)
	_check(not w.accord and heard.has("ab_kesh_1"), "then it is over")
	op.stage_time = 360.0 - 1.0
	await _frames(3)
	_check(op.get("fleet").size() == 11, "every wave of the assault came (%d ships)" % op.get("fleet").size())
	await _frames(70)
	_check(op.stage == "home" and w.story.flags.get("luna", "") == "held", "six minutes held: the fleet arrives")
	var base := w.starbase_of(1)
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 5, "The Accord Breaks is won")
	var d := SaveGame.read(SAVE)
	_check(not bool(d.get("story", {}).get("accord", true)), "the save remembers the war is hot")
	# fail forward: Luna falls and the story goes on
	w.story.index = 4
	op = await _launch("accord_breaks")
	await _frames(60 * 5)
	op.stage_time = 10.0
	await _frames(3)
	var drengin: Ship = op.get("fleet")[0]
	_put(drengin, luna.global_position + Vector2(luna.radius + 300.0, 0), luna.velocity)
	luna.silenced = 999.0
	op.set("down_t", 34.5)
	await _frames(60)
	_check(luna.owner_race == 2 and w.story.flags.get("luna", "") == "lost" and op.stage == "home", "Luna falls: the story carries on without it (owner %d, luna %s, stage %s, down %.1f, defenceless %s)" % [luna.owner_race, w.story.flags.get("luna", ""), op.stage, op.get("down_t"), luna.is_defenceless()])
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 5, "and The Accord Breaks still ends")


## Act 2.  Once the Accord is gone the Arceans keep the peace with Terra.
func _shields_down() -> void:
	var w: World = main.world
	_check(not w.accord and not w.at_war(0, 1) and w.at_war(1, 2) and w.at_war(0, 2), "after the Accord the Arceans stay out of Terra's war")
	var op := await _launch("shields_down")
	var p := w.player
	var deimos: Planet = w.planet_by_name["Deimos"]
	var droppers: Array = op.get("droppers")
	_check(deimos.owner_race == 2 and deimos.pshield_up() and droppers.size() == 2, "Deimos is Drengin behind its shield, and two drop ships come along")
	_check(droppers[0].troop_count() == 6 and droppers[0].design_name == "DropShip", "each drop ship carries troops (%d)" % droppers[0].troop_count())
	_put(p, deimos.global_position + Vector2(deimos.radius + 3000.0, 0), deimos.velocity)
	await _frames(5)
	_check(op.stage == "shield" and op.get("fleet").size() == 3, "near Deimos its guard comes up (%d)" % op.get("fleet").size())
	_check(droppers[0].landing_refusal(deimos) != "", "nothing lands through the shield")
	deimos.take_damage(1e6, p)
	await _frames(5)
	_check(not deimos.pshield_up() and op.stage == "drop", "shooting the shield down sends the drop ships in")
	for d in droppers:
		_put(d, deimos.global_position + Vector2(deimos.radius + 300.0, 0).rotated(randf() * TAU), deimos.velocity)
	for i in 60 * 12:
		await _frames(1)
		if op.stage == "ground":
			break
	_check(op.stage == "ground" and not deimos.battle.is_empty() and int(deimos.battle.get("race", -1)) == 1, "the drop ships land and the troops go in (stage %s)" % op.stage)
	# the garrison breaks
	deimos.garrison = {}
	if not deimos.battle.is_empty():
		deimos.battle["militia"] = 0.0
	await _frames(10)
	_check(deimos.owner_race == 1 and w.story.op == null and w.story.index == 6 and w.story.flags.get("deimos", "") == "taken", "Deimos is taken and Shields Down is won")


func _wolf_pack() -> void:
	var w: World = main.world
	var op := await _launch("wolf_pack")
	var p := w.player
	var wing: Array = w.wing.members(p)
	_check(wing.size() >= 2 and op.get("wingmen").size() == 2, "the flight lead has a wing of two (%d)" % wing.size())
	var freighters: Array = op.get("freighters")
	_check(freighters.size() == 3 and freighters[0].autopilot and op.get("escorts").size() == 2, "three supply ships and their escort burn for Mars")
	_put(p, freighters[0].global_position + Vector2(-1500, 0), freighters[0].velocity)
	await _frames(5)
	_check(op.stage == "attack", "sighting the convoy")
	w.wing.command(p, "attack", freighters[0])
	await _frames(5)
	_check(w.wing.order_of(p).get("order", "") == "attack", "the wing takes the attack order")
	for f in freighters.duplicate():
		f.take_damage(1e7, p)
		await _frames(2)
	await _frames(5)
	_check(w.story.op == null and w.story.index == 7, "the convoy is cut and Wolf Pack is won")


func _envoy() -> void:
	var w: World = main.world
	var op := await _launch("envoy")
	var p := w.player
	var envoy: Ship = op.get("envoy")
	_check(envoy != null and envoy.race_id == 0 and envoy.hull_race == 0, "Speaker Tenn's Arcean ship waits in the belt")
	_put(p, envoy.global_position + Vector2(400, 0), envoy.velocity)
	await _frames(5)
	_check(op.stage == "escort" and envoy.autopilot, "meeting the envoy, it sets off for Earth")
	var dest: Node2D = op.get("dest")
	var half: Vector2 = envoy.global_position.lerp(dest.global_position, 0.6)
	_put(envoy, half, envoy.velocity)
	_put(p, half + Vector2(200, 200), envoy.velocity)
	await _frames(60 * 11)
	var pirates: Array = op.get("pirates")
	_check(op.stage == "ambush" and pirates.size() == 3 and pirates[0].race_id == 3, "Mags Corrigan's pirates come for the envoy (stage %s)" % op.stage)
	var brandt: Ship = op.get("brandt")
	_check(brandt.target != envoy and w.nearest_enemy_ship(envoy.global_position, 1, 3000.0) != envoy, "no Terran takes the Arcean ship for an enemy")
	for r in pirates:
		if is_instance_valid(r) and not r.dead:
			r.take_damage(1e7, p)
	await _frames(60 * 5)
	_check(op.stage == "escort2" and op.done("pirates"), "the pirates driven off, on to Earth")
	_put(envoy, dest.global_position + Vector2(300, 0), dest.velocity)
	await _frames(5)
	_check(op.stage == "choice" or op.stage == "asked", "at the starbase Tenn asks")
	await _answer("share")
	await _frames(60 * 7)
	_check(w.story.op == null and w.story.index == 8 and w.story.flags.get("arceans", "") == "allied" and w.story.flags.get("choice_envoy", "") == "share", "showing Tenn the evidence: the Arceans side with Terra")
	# the other answer
	w.story.index = 7
	op = await _launch("envoy")
	envoy = op.get("envoy")
	var credits := p.credits
	_put(p, envoy.global_position + Vector2(400, 0), envoy.velocity)
	await _frames(5)
	dest = op.get("dest")
	_put(envoy, dest.global_position + Vector2(300, 0), dest.velocity)
	await _frames(5)
	await _answer("keep")
	await _frames(60 * 7)
	_check(w.story.op == null and w.story.flags.get("arceans", "") == "neutral" and p.credits >= credits + 1000, "keeping it: Intelligence pays and the Arceans stay out")
	# firing on the envoy ends it
	w.story.index = 7
	op = await _launch("envoy")
	envoy = op.get("envoy")
	envoy.take_damage(5.0, p)
	await _frames(5)
	_check(w.story.op == null and w.story.failed != "", "firing on the envoy fails the operation")
	main.retry_operation()
	for i in 4:
		await process_frame
	w = main.world
	if main.briefing != null:
		main._close_briefing()
	w.story.index = 8
	w.story.flags["envoy"] = "won"
	w.story.flags["arceans"] = "allied"


func _silent_running() -> void:
	var w: World = main.world
	var op := await _launch("silent_running")
	var p := w.player
	var yard: Ship = op.get("yard")
	var hammer: Ship = op.get("hammer")
	var pickets: Array = op.get("pickets")
	_check(yard != null and yard.design_name == "Shipyard" and hammer.design_name == "Dreadnought" and pickets.size() == 4, "the Hammer in its yard over Mars, inside a net of four pickets")
	# loud, close to a picket: heard
	var c: Ship = pickets[0]
	p.set_silent(false)
	for i in 60 * 5:
		_put(p, c.global_position + Vector2(900, 0), c.velocity)
		p.inputs["thrust"] = 1.0
		await _frames(1)
		if w.story.op == null:
			break
	p.inputs["thrust"] = 0.0
	_check(w.story.op == null and w.story.failed != "", "a picket that hears a loud ship raises the alarm")
	main.retry_operation()
	for i in 4:
		await process_frame
	w = main.world
	if main.briefing != null:
		main.briefing.launch.emit()
	await process_frame
	op = w.story.op
	p = w.player
	hammer = op.get("hammer")
	yard = op.get("yard")
	p.set_silent(true)
	for i in 60 * 3:
		await _frames(1)
		if w.story.op == null:
			break
	_check(w.story.op != null, "running silent from the start, nothing hears (failed: %s, %d from the yard)" % [w.story.failed, p.global_position.distance_to(yard.global_position) if is_instance_valid(yard) else -1])
	if w.story.op == null:
		return
	var off: Vector2 = (hammer.global_position - op.get("yard").global_position).normalized() * (hammer.size + 400.0)
	for i in 60 * 8:
		_put(p, hammer.global_position + off, hammer.velocity)
		await _frames(1)
		if op.stage == "out" or w.story.op == null:
			break
	_check(op.done("scan") and w.story.failed == "", "silent beside the Hammer, the scan goes through unheard (failed: %s)" % w.story.failed)
	_check(hammer.invulnerable > 0.0, "the Hammer cannot be hurt in its cradle")
	_put(p, op.get("yard").global_position + Vector2(9000, 0), w.planet_by_name["Mars"].velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 9 and w.story.flags.get("hammer_scan", false), "out of the net with the scan: Silent Running is won")
	p.set_silent(false)


func _burn_yards() -> void:
	var w: World = main.world
	var op := await _launch("burn_yards")
	var p := w.player
	var yard: Ship = op.get("yard")
	var strike: Array = op.get("strike")
	_check(strike.size() == 3 and strike[0].target == yard, "the Resolute's strike group goes for the yard")
	op.stage_time = 25.0
	await _frames(3)
	_check(op.get("fleet").size() == 3, "the yard's guard comes up from Mars")
	yard.take_damage(1e8, p)
	await _frames(10)
	_check(op.stage == "burned" and w.story.flags.get("hammer", "") == "crippled", "the yard burns with the Hammer in it")
	_put(p, w.planet_by_name["Mars"].global_position + Vector2(9000, 0), w.planet_by_name["Mars"].velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 10, "Burn the Yards is won")
	# fail forward: the Hammer launches
	w.story.index = 9
	op = await _launch("burn_yards")
	var yard2: Ship = op.get("yard")
	_put(p, yard2.global_position + Vector2(5000, 0), yard2.velocity)
	op.stage_time = 330.0 - 0.5
	await _frames(60)
	var hammer: Variant = op.get("hammer")
	_check(op.stage == "launched" and w.story.flags.get("hammer", "") == "launched" and is_instance_valid(hammer) and hammer.invulnerable <= 0.0, "too slow: the Hammer launches (stage %s, flag %s, valid %s, inv %s)" % [op.stage, w.story.flags.get("hammer", ""), is_instance_valid(hammer), hammer.invulnerable if is_instance_valid(hammer) else -1.0])
	_put(p, w.planet_by_name["Mars"].global_position + Vector2(9000, 0), w.planet_by_name["Mars"].velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 10, "and Burn the Yards still ends, the Hammer loose")


func _end() -> void:
	_restore(SAVE)
	_restore(SETTINGS)
	print("STORY TEST " + ("FAILED" if failed else "PASSED"))
	quit(1 if failed else 0)
