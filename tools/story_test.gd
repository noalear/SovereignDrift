# Story campaign test: "New story" starts a Terran war under the Ceres
# Accord (the empires hold their fire, a shot across it makes the shooter
# fair game), Hale calls the player in, the briefing pauses the war and
# launches the operation, and every operation of Acts 1 to 3 plays to the
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
	Story.interludes = false        # stepped through below; _interlude_check runs one fade
	main._start_game(false, true, "story_new")
	await process_frame
	await process_frame
	var w: World = main.world
	_check(w != null and w.story_mode and w.story != null, "New story starts the story campaign")
	_check(w.player.race_id == 1, "the story is flown for the Terrans")
	_check(w.accord, "the Ceres Accord holds at the start")
	_check(FileAccess.file_exists(SAVE), "a new story saves at once")
	_check(main.comms != null and main.story_panel != null, "the call window and the objectives are up")
	var extras := []
	for s in w.ships:
		if not (s.is_player or s.is_starbase() or s.design_name == "TradeStation"):
			extras.append(s.ship_name)
	_check(extras.is_empty(), "the story is scripted: no empire fleets, haulers or pirates of their own (%s)" % [extras])
	# a save from before the scripted war: a stray battleship and a world the old war took
	var stray := w.spawn_ship("Stray", 2, "Battleship", "Old War", w.player.global_position + Vector2(3000, 0), Vector2.ZERO, 8)
	stray.remove_meta("tutorial")
	var venus: Planet = w.planet_by_name["Venus"]
	var venus_was: int = venus.owner_race
	venus.surrender_to(2 if venus_was != 2 else 0, maxf(venus.population, GameData.MINOR_COLONY))
	w.story.tidy_world()
	_check(not w.ships.has(stray) and venus.owner_race == venus_was, "loading an old story save clears the old war's ships and conquests")
	var from := OS.get_environment("STORY_FROM")     # e.g. STORY_FROM=7 runs ops 8 on (quick reruns)
	if from != "":
		w.accord = false
		w.story.index = int(from)
		w.story.flags["arceans"] = "allied"
		if int(from) >= 12:
			w.story.flags["codes"] = "spoof"     # as _prize_crew answers
		w.story.keep_peace()
		for f in [_envoy, _silent_running, _burn_yards, _exodus, _prize_crew, _siege_mars, _sunward].slice(maxi(int(from) - 7, 0)):
			await f.call()
		_end()
		return
	await _accord_check(w)
	await _first_watch()
	await _interlude_check()
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
	await _exodus()
	await _prize_crew()
	await _siege_mars()
	await _sunward()
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
			id = str(id).split("?")[0]          # a briefing line may hang on a flag
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
	_check(w.ship_color(p, d) == World.PEACE_COLOR and not w.shown_hostile(p, d), "under the Accord a Drengin ship shows in peace gold, not hostile red")
	_check(w.colony_color(1, 2) == World.PEACE_COLOR, "and so do Drengin worlds")
	w.accord = false
	_check(w.ship_color(p, d) == d.race_color() and w.shown_hostile(p, d), "at war it shows hostile")
	w.accord = true
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


## Between operations: the fade, the stardate, home to the starbase, and
## the next briefing open.
func _interlude_check() -> void:
	var w: World = main.world
	var st: Story = w.story
	var p := w.player
	var earth: Planet = w.planet_by_name["Earth"]
	_put(p, earth.global_position + Vector2(9000, 0), earth.velocity)
	p.hull = p.hull_max * 0.5
	Story.interludes = true
	st.unlock_at = INF
	st.interlude_at = w.elapsed
	for i in 30:
		await _frames(1)                # the last words of the operation said, it fades
		if main.interlude != null:
			break
	_check(main.interlude != null, "between operations the screen fades to the next stardate")
	_check(Story.stardate(1) == "2471.315" and Story.days_since(1) == "Three days later", "a stardate that makes sense: %s, %s" % [Story.stardate(1), Story.days_since(1)])
	for i in 60 * 12:
		await physics_frame
		if main.briefing != null:
			break
	var base: Ship = w.starbase_of(1)
	_check(main.briefing != null and main.interlude == null, "it fades back in with the next briefing open")
	_check(base != null and p.global_position.distance_to(base.global_position) < 3000.0 and p.hull >= p.hull_max - 0.1, "back at the starbase, mended")
	if main.briefing != null:
		main._close_briefing()
	Story.interludes = false


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
	_check(op.done("meet") and op.stage == "leave_earth", "meeting the Cormorant starts the escort")
	_check(cormorant.autopilot and not cormorant.warping, "the Cormorant burns out from Earth without jumping on her own")
	var port: Ship = op.get("port")
	var earth := w.planet_by_name["Earth"] as Planet
	var course := (port.global_position - cormorant.global_position).normalized()
	var out: Vector2 = earth.global_position + course * (earth.radius + float(op.CLEAR_OF_EARTH) + 400.0)
	_put(cormorant, out, cormorant.velocity)
	cormorant.rotation = course.angle()
	_put(p, out + course.orthogonal() * 250.0, cormorant.velocity)
	await _frames(10)
	_check(op.stage == "escort" and op.jumps_ok, "clear of Earth, the Cormorant is ready to jump with her escort")
	# the player jumps for the port: she jumps too
	p.rotation = course.angle()
	p.reset_physics_interpolation()
	p.energy = p.energy_max
	p.start_warp()
	var jumped := false
	for i in 60 * 4:
		await _frames(1)
		jumped = jumped or cormorant.warping
		if jumped:
			break
	_check(jumped and not op.solo, "the Cormorant jumps on the player's lead")
	# halfway: the scout's jammer pulls everyone out of warp
	var half: Vector2 = cormorant.global_position.lerp(port.global_position, 0.6)
	_put(cormorant, half, cormorant.velocity)
	_put(p, half + course.orthogonal() * (cormorant.size + p.size + 40.0), cormorant.velocity)
	_put(brandt, half + course.orthogonal() * -(cormorant.size + 120.0) - course * 500.0, cormorant.velocity)
	await _frames(3)
	var scout: Ship = op.get("scout")
	_check(op.stage == "shadowed" and scout != null and is_instance_valid(scout), "halfway a Drengin scout pulls the flight out of warp")
	_check(not cormorant.warping and not p.warping, "the Cormorant and the player drop out of warp (%s, %s)" % [cormorant.warping, p.warping])
	var closest := INF
	var ran := false
	var guarded := 0.0
	for i in 60 * 24:
		await _frames(1)
		if is_instance_valid(brandt) and is_instance_valid(cormorant):
			closest = minf(closest, brandt.global_position.distance_to(cormorant.global_position) - brandt.size - cormorant.size)
		if op.scout_mode == "run" and is_instance_valid(scout) and op.scan > 0.0:
			ran = true
			var left := Vector2.RIGHT.rotated(cormorant.rotation).orthogonal()
			guarded = signf(left.dot(p.global_position - cormorant.global_position)) * op.run_side
			break
	_check(closest > 0.0, "Lt. Brandt keeps clear of the Cormorant on the leader's wing (closest gap %.0f)" % closest)
	_check(ran and op.scan > 0.0, "the scout makes a run at the Cormorant and starts reading her hold (%.0f%%)" % (op.scan * 100.0))
	_check(guarded < 0.0, "it runs in on the side the player is not guarding")
	_check(is_instance_valid(scout) and scout.target != p and p.last_attacker != scout, "the scout never fires under the Accord")
	# a bump is no shot across the Accord
	w._impact(scout.body, p.body, w.IMPACT_THRESHOLD + 40.0, scout.global_position)
	await _frames(2)
	_check(st.op == op and st.failed == "", "bumping the scout is not firing first")
	# crowd it: it turns away
	var runs: int = op.runs
	var read: float = op.scan
	var to_c: Vector2 = (cormorant.global_position - scout.global_position).normalized()
	_put(p, scout.global_position + to_c * (scout.size + p.size + 150.0), scout.velocity)
	await _frames(3)
	_check(op.scout_mode == "off" and op.scan <= read + 0.01, "a pilot in its face turns the scout away (%s)" % op.scout_mode)
	# its last run crowded off, it leaves without the manifest
	op.runs = op.RUNS
	op.scout_mode = "run"
	op.scout_t = 0.0
	_put(p, scout.global_position + to_c * (scout.size + p.size + 150.0), scout.velocity)
	await _frames(3)
	_check(op.scout_mode == "leave" and op.done("crowd") and st.flags.get("fw_manifest", "") == "safe", "crowded off its last run, the scout leaves without her manifest")
	# then raiders: fair game
	for i in 60 * 12:
		await _frames(1)
		if op.stage == "raid":
			break
	var raiders: Array = op.get("raiders")
	_check(op.stage == "raid" and raiders.size() >= 3, "raiders in pirate colours come for the Cormorant (%d)" % raiders.size())
	_check(not is_instance_valid(scout) or not w.ships.has(scout) or scout.global_position.distance_to(cormorant.global_position) > 2500.0, "the scout is gone before they come")
	for r in raiders:
		if is_instance_valid(r) and not r.dead:
			r.take_damage(99999.0, p, r.global_position)
	await _frames(5)
	_check(st.op == op and st.failed == "" and op.stage == "last_leg", "shooting the raiders is fair: on to the port")
	# nobody leads the last jump: she goes alone after a wait
	_put(p, cormorant.global_position + course.orthogonal() * 300.0, cormorant.velocity)
	op.stage_time = op.JUMP_WAIT + 1.0
	jumped = false
	for i in 60 * 4:
		await _frames(1)
		jumped = jumped or cormorant.warping
		if jumped:
			break
	_check(jumped and op.solo, "with nobody leading, the Cormorant jumps for the port alone")
	# the port: then home
	_put(cormorant, port.global_position + Vector2(400.0, 0), port.velocity)
	_put(p, port.global_position + Vector2(550.0, 0), port.velocity)
	await _frames(30)
	_check(op.done("escort") and op.stage == "home", "the Cormorant arrives at the free port, then home to the starbase")
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
	# the story keeps no Drengin fleet of its own: bring one to fire on
	var drengin: Ship = w.spawn_ship("Test Drengin", 2, "Scout", "Test Claw", p.global_position + Vector2(900, 0), p.velocity)
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
	# the worst case: Ceres passing right by Mars and the Drengin starbase
	var ceres: Planet = w.planet_by_name["Ceres"]
	ceres.orbit_angle = w.planet_by_name["Mars"].orbit_angle
	ceres.update_orbit(0.0)
	ceres.sync_body()
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
	_check(op.guns_near(site) >= 4500.0, "the wrecks lie well clear of Mars and every colony's guns (%.0f)" % op.guns_near(site))
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
	for i in 240:
		await physics_frame         # the war's own clock: process frames can run far ahead of it
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
	_put(p, luna.global_position + Vector2(luna.radius + 600.0, 0), luna.velocity)
	op.stage_time = op.HOLD_TIME - 1.0
	await _frames(3)
	_check(op.get("fleet").size() == 11, "every wave of the assault came (%d ships)" % op.get("fleet").size())
	await _frames(70)
	_check(op.stage == "home" and w.story.flags.get("luna", "") == "held", "four minutes held: the fleet arrives")
	var relief: Array = op.get("relief")
	var warped := 0
	for s in relief:
		if is_instance_valid(s) and (s.warping or s.global_position.distance_to(luna.global_position) < op.RELIEF_FROM * 0.6):
			warped += 1
	_check(relief.size() == 8 and warped == 8, "Okafor's fleet of eight warps in together (%d of %d)" % [warped, relief.size()])
	var near := 0
	for i in 60 * 10:
		await _frames(1)
		near = 0
		for s in relief:
			if is_instance_valid(s) and not s.warping and s.global_position.distance_to(luna.global_position) < 3500.0:
				near += 1
		if near >= 6:
			break
	_check(near >= 6, "and drops out over Luna (%d)" % near)
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
	_check(deimos.owner_race == 2 and deimos.pshield_up() and droppers.size() == 2, "Deimos is Drengin behind its shield, and two drop ships stand by")
	_check(op.get("strike").size() == 3 and op.get("escort").size() == 2, "a strike group of three flies with the squad, two more guard the drop ships")
	var mars: Planet = w.planet_by_name["Mars"]
	var phobos: Planet = w.planet_by_name["Phobos"]
	_check(mars.is_defenceless() and phobos.is_defenceless(), "Mars's and Phobos's guns are busy with the fleet")
	_check(droppers[0].global_position.distance_to(deimos.global_position) > op.STAGING * 0.8 and op.guns_near(droppers[0].global_position, [mars, phobos]) > 4000.0, "the drop ships wait well out of Deimos's guns (%.0f)" % droppers[0].global_position.distance_to(deimos.global_position))
	_check(droppers[0].troop_count() == 6 and droppers[0].design_name == "DropShip", "each drop ship carries troops (%d)" % droppers[0].troop_count())
	_put(p, deimos.global_position + Vector2(deimos.radius + 3000.0, 0), deimos.velocity)
	await _frames(5)
	_check(op.stage == "shield" and op.get("fleet").size() == 3, "near Deimos its guard comes up (%d)" % op.get("fleet").size())
	_check(droppers[0].landing_refusal(deimos) != "", "nothing lands through the shield")
	deimos.take_damage(1e6, p)
	await _frames(5)
	_check(not deimos.pshield_up() and op.stage == "drop", "the shield comes down")
	await _frames(30)
	_check(droppers[0].global_position.distance_to(deimos.global_position) > op.STAGING * 0.7, "the drop ships wait to be called")
	_check(w.story.signal_label() != "", "the story key calls them (%s)" % w.story.signal_label())
	main._story_key()
	await _frames(2)
	_check(op.called and op.done("call") and droppers[0].nav_target == deimos and droppers[0].warp_block == 0.0, "called in, they head for Deimos")
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
	_put(p, yard2.global_position.lerp(w.planet_by_name["Mars"].global_position, 0.3), yard2.velocity)   # still over Mars: not clear yet
	op.stage_time = 330.0 - 0.5
	await _frames(60)
	var hammer: Variant = op.get("hammer")
	_check(op.stage == "launched" and w.story.flags.get("hammer", "") == "launched" and is_instance_valid(hammer) and hammer.invulnerable <= 0.0, "too slow: the Hammer launches (stage %s, flag %s, valid %s, inv %s)" % [op.stage, w.story.flags.get("hammer", ""), is_instance_valid(hammer), hammer.invulnerable if is_instance_valid(hammer) else -1.0])
	_put(p, w.planet_by_name["Mars"].global_position + Vector2(9000, 0), w.planet_by_name["Mars"].velocity)
	await _frames(10)
	_check(w.story.op == null and w.story.index == 10, "and Burn the Yards still ends, the Hammer loose")


func _exodus() -> void:
	var w: World = main.world
	var st: Story = w.story
	st.index = 10
	var lines := st.brief_lines()
	_check(lines.has("ex_brief_1") and (lines.has("ex_brief_2") != lines.has("ex_brief_2b") or not st.flags.has("luna")), "the Exodus briefing tells the Luna its pilots left behind (%s)" % [lines])
	var op := await _launch("exodus")
	var p := w.player
	var luna: Planet = w.planet_by_name["Luna"]
	var earth: Planet = w.planet_by_name["Earth"]
	var ts: Array = op.get("transports")
	_check(ts.size() == 3 and ts[0].design_name == "Freighter" and op.get("escort").size() == 2, "three transports and two destroyers set out from Earth")
	var pickup: Node2D = op.get("pickup")
	for t in ts:
		_put(t, pickup.global_position + Vector2(randf_range(-80, 80), randf_range(-80, 80)), luna.velocity)
	_put(p, pickup.global_position + Vector2(300, 0), luna.velocity)
	await _frames(5)
	_check(op.stage == "load", "on station beside Luna, the loading begins")
	op.stage_time = 41.0
	await _frames(3)
	_check(op.get("fleet").size() >= 6, "bombers come from Mars for the transports (%d)" % op.get("fleet").size())
	_check(w.story.signal_label() == "", "nothing to send home half full")
	var filled: Dictionary = op.get("filled")
	for t in ts:
		filled[t] = op.LOAD_TIME
	await _frames(3)
	_check(op.stage == "loaded" and w.story.signal_label() != "", "full, they wait for the word (%s)" % w.story.signal_label())
	op.load_t = op.HAMMER_AFTER
	await _frames(3)
	var hammer: Ship = op.get("hammer")
	if st.flags.get("hammer", "") == "launched":
		_check(hammer != null and hammer.design_name == "Dreadnought", "the Hammer comes from Mars for them")
		if hammer != null:
			hammer.take_damage(1e7, p)
			await _frames(2)
			_check(not hammer.dead, "out in open space nothing gets through the Hammer's shields")
	main._story_key()
	await _frames(2)
	_check(op.stage == "run" and ts[0].nav_target == earth, "sent home, they run for Earth")
	var raider: Ship = op.get("fleet")[0]
	ts[2].take_damage(1e7, raider)
	await _frames(3)
	_check(st.op != null and ts[2].dead, "losing one transport is not the end")
	for t in [ts[0], ts[1]]:
		_put(t, earth.global_position + Vector2(earth.radius + 300.0, 0).rotated(randf() * TAU), earth.velocity)
	await _frames(10)
	_check(st.op == null and st.index == 11 and int(st.flags.get("evacuated", 0)) == 2, "two transports down at Earth: Exodus is won")


func _prize_crew() -> void:
	var w: World = main.world
	var st: Story = w.story
	var op := await _launch("prize_crew")
	var p := w.player
	var oath: Ship = op.get("oath")
	_check(oath.design_name == "Battleship" and op.get("escorts").size() >= 2 and op.get("strike").size() == 2, "the Iron Oath and her escorts, the Resolute's pair with the squad")
	_check(op.guns_near(oath.global_position) > 4000.0, "she patrols out of reach of every gun (%.0f)" % op.guns_near(oath.global_position))
	_put(p, oath.global_position + Vector2(2500, 0), oath.velocity)
	await _frames(5)
	_check(op.stage == "fight", "close in, the fight starts")
	oath.hull = oath.hull_max * 0.3          # the fight, short
	await _frames(3)
	_check(oath.is_disabled() and op.done("engines") and oath.crew <= op.SKELETON, "her engines go first, and most of her crew takes to the pods (hull %.2f, disabled %s, crew %.1f)" % [oath.hull / oath.hull_max, oath.is_disabled(), oath.crew])
	oath.hull = oath.hull_max * 0.15
	await _frames(2)
	oath.shield = 0.0
	oath.take_damage(1e7, p)
	await _frames(2)
	_check(not oath.dead, "what is left holds together for the boarders")
	_put(p, oath.global_position + Vector2(oath.size + 60.0, 0), oath.velocity)
	w.capture.take(p, oath)
	await _frames(5)
	var prize: Ship = op.get("prize")
	_check(op.stage == "codes" and prize != null and prize.race_id == 1 and prize.hull >= prize.hull_max * 0.45, "boarded, the prize crew pulls the codes")
	op.stage_time = 36.0
	await _frames(3)
	_check(op.get("fleet").size() >= 5, "Mars sends ships to take her back (%d)" % op.get("fleet").size())
	op.stage_time = op.CODES_TIME
	await _frames(3)
	await _answer("spoof")
	await _frames(5)
	_check(st.flags.get("codes", "") == "spoof" and st.flags.get("choice_codes", "") == "spoof" and op.stage == "clear" and not is_instance_valid(prize), "false orders sent, and Rhee takes the Iron Oath home")
	_put(p, op.get("taken_at") + Vector2(9000, 0), w.planet_by_name["Mars"].velocity)
	await _frames(10)
	_check(st.op == null and st.index == 12, "clear of Mars: Prize Crew is won")


func _siege_mars() -> void:
	var w: World = main.world
	var st: Story = w.story
	_check(st.brief_lines().has("sm_brief_3") and not st.brief_lines().has("sm_brief_3b"), "the siege briefing remembers the false orders")
	var op := await _launch("siege_mars")
	var p := w.player
	var mars: Planet = w.planet_by_name["Mars"]
	var base: Ship = w.starbase_of(2)
	var droppers: Array = op.get("droppers")
	_check(mars.owner_race == 2 and mars.pshield_up() and droppers.size() == 3, "Mars is Drengin behind its shield, and three drop ships stand by")
	_check(base != null and base.hull <= base.hull_max * 0.46, "the starbase has had a long night (%.0f of %.0f)" % [base.hull if base else 0.0, base.hull_max if base else 0.0])
	var allied: bool = st.flags.get("arceans", "") == "allied"
	_check(op.get("fleet").size() == 3 + (3 if allied else 0) and op.get("arceans").size() == (3 if allied else 0), "Okafor's fleet%s" % (" and three Arcean ships" if allied else ""))
	_check(mars.is_defenceless() and op.get("dauntless").global_position.distance_to(mars.global_position) > 5000.0, "the Dauntless stands off Mars and has its batteries pinned")
	_put(p, mars.global_position + Vector2(mars.radius + 4000.0, 0), mars.velocity)
	await _frames(5)
	_check(op.stage == "shield" and op.get("enemies").size() == 3, "near Mars the home fleet comes up, half of it sent away (%d)" % op.get("enemies").size())
	base.take_damage(1e8, p)
	await _frames(5)
	_check(base.dead and op.done("base"), "the starbase breaks")
	await _frames(60 * 11)
	_check(w.starbase_of(2) == null, "and it does not relaunch over Mars")
	mars.take_damage(1e6, p)
	await _frames(5)
	_check(not mars.pshield_up() and op.stage == "drop", "Mars's shield comes down")
	main._story_key()
	await _frames(2)
	_check(op.called and op.done("call"), "the drop ships are called in")
	for d in droppers:
		_put(d, mars.global_position + Vector2(mars.radius + 300.0, 0).rotated(randf() * TAU), mars.velocity)
	for i in 60 * 12:
		await _frames(1)
		if op.stage == "ground":
			break
	_check(op.stage == "ground" and not mars.battle.is_empty() and int(mars.battle.get("race", -1)) == 1, "the drop ships land and Third Drop goes in (stage %s)" % op.stage)
	mars.garrison = {}
	if not mars.battle.is_empty():
		mars.battle["militia"] = 0.0
	await _frames(10)
	_check(mars.owner_race == 1 and op.stage == "flee" and st.flags.get("mars", "") == "taken", "Mars surrenders")
	var hammer: Ship = op.get("hammer")
	_check(hammer != null and hammer.velocity.length() > 100.0, "and the Hammer runs for the Sun")
	op.stage_time = op.FLEE_TIME + 0.1
	await _frames(3)
	_check(st.op == null and st.index == 13, "Siege of Mars is won")
	var stray := w.spawn_ship("Drengin Command", 2, "Starbase", "Drengin Starbase", mars.global_position + Vector2(600, 0), mars.velocity, 14)
	st.tidy_world()
	_check(mars.owner_race == 1 and not w.ships.has(stray), "a load keeps Mars Terran and its old starbase gone")


func _sunward() -> void:
	var w: World = main.world
	var st: Story = w.story
	var op := await _launch("sunward")
	var p := w.player
	var hammer: Ship = op.get("hammer")
	_check(hammer != null and float(op.get("u")) > -0.8 and op.get("ambush").is_empty(), "after the false orders the Hammer has a head start and nobody waits for it")
	_check(w.heat_at(hammer.global_position) == 0.0 and hammer.invulnerable > 0.0, "it starts out of the heat, shields up")
	hammer.take_damage(1e7, p)
	await _frames(2)
	_check(not hammer.dead, "out there nothing gets through")
	op.u = -0.25
	await _frames(3)
	_check(op.in_heat and hammer.invulnerable <= 0.0 and hammer.shield < hammer.shield_max * 0.05, "in the Sun's heat its shields burn away (heat %.2f, inv %.2f, shield %.0f)" % [w.heat_at(hammer.global_position), hammer.invulnerable, hammer.shield])
	var at := hammer.global_position
	await _frames(60)
	_check(hammer.global_position.distance_to(at) > 10.0 and hammer.global_position.distance_to(w.sun.global_position) > w.sun.radius, "it keeps to its course round the Sun")
	op.u = op.ESCAPE_AT - 0.001
	for i in 60:
		await _frames(1)
		if st.op == null:
			break
	_check(st.op == null and st.failed != "", "out the other side, it gets away and the operation fails")
	main.retry_operation()
	for i in 4:
		await process_frame
	w = main.world
	st = w.story
	if main.briefing != null:
		main._close_briefing()
	# listening instead: an ambush on its course
	st.flags["codes"] = "listen"
	op = await _launch("sunward")
	p = w.player
	hammer = op.get("hammer")
	var ambush: Array = op.get("ambush")
	var allied: bool = st.flags.get("arceans", "") == "allied"
	_check(ambush.size() == 3 and ambush[0].race_id == (0 if allied else 1) and float(op.get("u")) <= -0.99, "listening put %s ambush on its course" % ("an Arcean" if allied else "a Terran"))
	op.u = -0.2
	await _frames(3)
	_check(w.story.signal_label() != "", "the story key springs the ambush (%s)" % w.story.signal_label())
	main._story_key()
	await _frames(2)
	_check(op.sprung and op.slow_left > 0.0 and ambush[0].target == hammer, "sprung, they cross its bow")
	hammer.take_damage(1e8, p)
	await _frames(5)
	_check((not is_instance_valid(hammer) or hammer.dead) and op.stage == "home" and st.flags.get("war", "") == "won" and not w.at_war(1, 2), "the Hammer burns, and the Drengin ask for terms")
	var base := w.starbase_of(1)
	_put(p, base.global_position + Vector2(300, 0), base.velocity)
	await _frames(10)
	_check(st.op == null and st.index == Story.OPS.size(), "home: Sunward is won and the story is told")


func _end() -> void:
	_restore(SAVE)
	_restore(SETTINGS)
	print("STORY TEST " + ("FAILED" if failed else "PASSED"))
	quit(1 if failed else 0)
