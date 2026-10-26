# Campaign test (the Resources update): a new campaign has no clock and
# saves at once; the worlds, stockpiles, pilots, purses, the benched
# pirate and the campaign's record come back on Continue; the menu offers
# Continue only when a save exists; winning the war deletes the save.
# Backs up and restores user://campaign.save and user://settings.cfg.
#   godot --headless --path . -s tools/campaign_test.gd
extends SceneTree

var main: Node
var failed := false
const SAVE := "user://campaign.save"
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


func _frames(n: int) -> void:
	for i in n:
		await physics_frame


func _start() -> void:
	_backup(SAVE)
	_backup(SETTINGS)
	if FileAccess.file_exists(SAVE):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(SAVE))
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	_check(main.menu != null and main.menu.save_info.is_empty(), "no saved campaign: the menu has nothing to continue")
	var cont: Array = []
	for b in main.menu.buttons:
		if b[1] == "Continue":
			cont = b
	_check(not cont.is_empty() and cont[3] == false, "Continue is greyed out")
	main._start_game(false, true, "new")
	await process_frame
	await process_frame
	var w: World = main.world
	_check(w.campaign and w.round_rules.length == 0.0, "a campaign has no round clock")
	_check(FileAccess.file_exists(SAVE), "a new campaign saves at once")
	await _frames(240)
	# change the war
	var earth: Planet = w.planet_by_name["Earth"]
	var luna: Planet = w.planet_by_name["Luna"]
	earth.stock["metals"] = 321.0
	earth.stock["exotics"] = 12.0
	luna.set_colony(2, "Outpost", 55000000.0)
	var p := w.player
	p.refit("Freighter")
	p.credits = 777
	p.hold = ["4001", "5002"]
	p.colonists = 12000000.0
	p.cargo = {"ice": 7.0}
	p.cargo_origin = earth
	var at: Vector2 = w.planet_by_name["Mars"].global_position + Vector2(900, 0)
	p.global_position = at
	p.body.pos = at
	p.velocity = w.planet_by_name["Mars"].velocity
	p.body.vel = p.velocity
	var ai: Ship = null
	for s in w.ships:
		if not s.is_player and not s.is_station and s.race_id == 2 and not s.dead:
			ai = s
			break
	ai.credits = 4321
	ai.score = 999
	var pirate: Ship = null
	for s in w.ships:
		if s.race_id == 3:
			pirate = s
	var pirate_dead: bool = pirate.dead
	# stations: a skimmer frame at Jupiter part-supplied, a finished
	# defensive station at Mars, and a kit in the player's hold
	var jupiter: Planet = w.planet_by_name["Jupiter"]
	var frame := w.build_station(1, "GasSkimmer", jupiter, jupiter.global_position + Vector2(jupiter.radius + 300.0, 0))
	frame.cargo = {"metals": 12.0}
	frame.build_t = 0.0
	var mars: Planet = w.planet_by_name["Mars"]
	var guns := w.build_station(2, "DefenseStation", mars, mars.global_position + Vector2(0, mars.radius + 500.0), false)
	var guns_at: Vector2 = guns.global_position - mars.global_position
	p.kit = "SensorStation"
	w.round_rules.note("Test event", 1)
	var ships_before := w.ships.size()
	var names_before := []
	for s in w.ships:
		names_before.append(s.pilot_name)
	names_before.sort()
	var elapsed_before := w.elapsed
	var mars_angle: float = w.planet_by_name["Mars"].orbit_angle
	_check(w.save_campaign(), "the campaign saves")
	var ai_name := ai.pilot_name
	# leave and continue
	main._save_career()
	main._show_menu()
	await process_frame
	await process_frame
	_check(not main.menu.save_info.is_empty(), "the menu sees the saved campaign: %s" % main.menu._save_line())
	main._start_game(false, true, "continue")
	await process_frame
	await process_frame
	w = main.world
	_check(w.campaign and w.round_rules.length == 0.0, "the continued campaign has no clock either")
	_check(absf(w.elapsed - elapsed_before) < 1.0, "the campaign's clock comes back (%.0f s)" % w.elapsed)
	_check(absf(w.planet_by_name["Mars"].orbit_angle - mars_angle) < 0.01, "Mars is where it was in its orbit")
	earth = w.planet_by_name["Earth"]
	_check(absf(earth.stock["metals"] - 321.0) < 1.0 and absf(earth.stock["exotics"] - 12.0) < 0.5, "Earth's stockpile comes back (metals %.0f)" % earth.stock["metals"])
	luna = w.planet_by_name["Luna"]
	_check(luna.owner_race == 2 and luna.colony_design == "Outpost", "Luna is still a Drengin outpost")
	_check(w.ships.size() == ships_before, "every ship comes back (%d of %d)" % [w.ships.size(), ships_before])
	var names_after := []
	for s in w.ships:
		names_after.append(s.pilot_name)
	names_after.sort()
	_check(names_after == names_before, "the same pilots fly")
	p = w.player
	_check(p.credits == 777 and p.hold == ["4001", "5002"] and absf(p.colonists - 12000000.0) < 1.0, "the player's purse, hold and colonists come back")
	_check(absf(p.cargo.get("ice", 0.0) - 7.0) < 0.01 and p.cargo_origin == earth, "the player's cargo comes back, loaded at Earth")
	_check(p.kit == "SensorStation", "and the station kit in the hold")
	var frame2: Ship = null
	var guns2: Ship = null
	for s in w.ships:
		if s.design_name == "GasSkimmer":
			frame2 = s
		elif s.design_name == "DefenseStation":
			guns2 = s
	_check(frame2 != null and frame2.building and frame2.station_body == w.planet_by_name["Jupiter"] and absf(frame2.cargo.get("metals", 0.0) - 12.0) < 0.01 and frame2.cargo_max == 25.0,
		"the skimmer frame comes back at Jupiter with its 12 Metals")
	_check(guns2 != null and not guns2.building and guns2.race_id == 2 and guns2.get_node_or_null("AI") != null and guns2.weapons.size() == 3, "the defensive station comes back armed and crewed")
	mars = w.planet_by_name["Mars"]
	_check(guns2 != null and (guns2.global_position - mars.global_position).distance_to(guns_at) < 5.0, "and keeps its spot over Mars")
	var haulers := w.ships.filter(func(x): return x.support)
	_check(haulers.size() == 6 and haulers.all(func(x): return (x.get_node("AI") as AIPilot).hauler), "the supply ships come back as supply ships (%d)" % haulers.size())
	_check(p.global_position.distance_to(at) < 50.0, "the player is where they were (%.0f px off)" % p.global_position.distance_to(at))
	var ai2: Ship = null
	for s in w.ships:
		if s.pilot_name == ai_name:
			ai2 = s
	_check(ai2 != null and ai2.credits == 4321 and ai2.score == 999 and ai2.get_node_or_null("AI") != null, "a computer pilot comes back with its purse, score and AI")
	var pirate2: Ship = null
	for s in w.ships:
		if s.race_id == 3:
			pirate2 = s
	_check(pirate2 != null and pirate2.dead == pirate_dead, "the pirate is still %s" % ("benched" if pirate_dead else "flying"))
	var bases := 0
	for s in w.ships:
		if s.is_starbase() and not s.dead:
			bases += 1
	_check(bases == 3, "three starbases stand")
	var has_event := false
	for e in w.round_rules.events:
		if e[1] == "Test event":
			has_event = true
	_check(has_event, "the campaign's timeline comes back")
	await _frames(120)
	_check(not p.dead and w.player == p, "the game runs on after loading")
	# winning ends the campaign
	w.round_rules._finish(1, "conquest")
	await process_frame
	_check(not FileAccess.file_exists(SAVE), "the war's end deletes the save")
	_restore(SAVE)
	_restore(SETTINGS)
	print("campaign test %s" % ("FAILED" if failed else "passed"))
	quit(1 if failed else 0)
