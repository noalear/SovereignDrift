# Story campaign test: "New story" starts a Terran war under the Ceres
# Accord (the empires hold their fire, a shot across it makes the shooter
# fair game), Hale calls the player in, the briefing pauses the war and
# launches the operation, and both operations of Act 1 play to the end the
# way a pilot would (teleports where flying would take minutes).  Also: a
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
	await _accord_check(w)
	await _first_watch()
	await _slots_check()
	await _retry_check()
	await _dark_convoy()
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


func _end() -> void:
	_restore(SAVE)
	_restore(SETTINGS)
	print("STORY TEST " + ("FAILED" if failed else "PASSED"))
	quit(1 if failed else 0)
