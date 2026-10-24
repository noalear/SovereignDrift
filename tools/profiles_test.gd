# Profiles test: single player and online keep separate pilots and careers,
# practice duels and free flight never reach a career, and the Marauder is
# gone.  Puts the pilot's own settings file back afterwards.
#   godot --headless --path . -s tools/profiles_test.gd
extends SceneTree

const CFG := "user://settings.cfg"
const BACKUP := "user://settings.cfg.profiles_test"

var main: Node
var failed := false
var st: Node


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	var had_cfg := FileAccess.file_exists(CFG)
	if had_cfg:
		DirAccess.copy_absolute(ProjectSettings.globalize_path(CFG), ProjectSettings.globalize_path(BACKUP))
	root.add_child(main)
	st = root.get_node("/root/Settings")
	await process_frame
	# two profiles, each its own pilot and career
	st.use_profile("single")
	st.pilot_name = "Solo"
	st.race_id = 1
	st.career = {"score": 5000, "kills": 40, "deaths": 1, "rounds": 2, "wins": 1}
	st.use_profile("online")
	_check(st.career["score"] != 5000 or st.pilot_name != "Solo", "the online profile is not the single-player one")
	st.pilot_name = "Netter"
	st.race_id = 0
	st.career = {"score": 77, "kills": 3, "deaths": 0, "rounds": 0, "wins": 0}
	st.use_profile("single")
	_check(st.pilot_name == "Solo" and st.race_id == 1 and st.career["score"] == 5000, "switching back brings the single-player pilot and career (%s %d)" % [st.pilot_name, st.career["score"]])
	# saved and loaded again
	st.save_settings()
	st.career = {}
	st.profiles = {}
	st.load_settings()
	_check(st.profile == "single" and st.career.get("score") == 5000, "the single-player career survives a reload")
	_check(int(st.profile_info("online")["career"]["score"]) == 77 and st.profile_info("online")["name"] == "Netter", "so does the online one")
	# free flight earns nothing
	main._start_free_flight()
	await process_frame
	await process_frame
	var p = main.world.player
	_check(p.score == 5000, "free flight starts from the career's rank and score (%d)" % p.score)
	p.score += 20000
	p.kills += 500
	main.world._on_rank_changed(p, 9)
	main._save_career()
	main._show_menu()
	await process_frame
	_check(st.career["score"] == 5000 and st.career["kills"] == 40, "free flight did not touch the career (%d, %d)" % [st.career["score"], st.career["kills"]])
	# practice duel: deaths do not count either
	main._start_practice()
	await process_frame
	await process_frame
	p = main.world.player
	p.score += 999
	p.take_damage(1e9, null)
	await physics_frame
	main._save_career()
	main._show_menu()
	await process_frame
	_check(st.career["score"] == 5000 and st.career["deaths"] == 1, "a practice duel did not touch the career (score %d, deaths %d)" % [st.career["score"], st.career["deaths"]])
	# a real game does record
	main._start_game()
	await process_frame
	await process_frame
	main.world.player.score += 100
	main._save_career()
	main._show_menu()
	await process_frame
	_check(st.career["score"] == 5100, "a Conquest game still records (%d)" % st.career["score"])
	_check(int(st.profile_info("online")["career"]["score"]) == 77, "and only to the single-player profile")
	# the Marauder is gone
	var gd = root.get_node("/root/GameData")
	_check(not gd.SHIPS.has("Marauder") and not "Marauder" in gd.REFIT_ORDER, "the Marauder hull is gone")
	# put the pilot's own settings back
	if had_cfg:
		DirAccess.copy_absolute(ProjectSettings.globalize_path(BACKUP), ProjectSettings.globalize_path(CFG))
		DirAccess.remove_absolute(ProjectSettings.globalize_path(BACKUP))
	else:
		DirAccess.remove_absolute(ProjectSettings.globalize_path(CFG))
	print("profiles test: ", "FAILED" if failed else "passed")
	quit(1 if failed else 0)


func _check(ok: bool, what: String) -> void:
	print("%s %s" % ["ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
