# Competitive Conquest online: two headless processes, one hosting and one
# joining.  Usage (start the joiner a few seconds after the host):
#   godot --headless --path . -s tools/versus_net_test.gd -- --versus --host
#   godot --headless --path . -s tools/versus_net_test.gd -- --join=127.0.0.1
# and the same with --host-campaign (the saving campaign, hosted).  The
# joiner flies for a rival empire, not the host's, and both sides see each
# other as enemies.  Exit code 1 when a check fails.
extends SceneTree

var main: Node
var settings: Node
var failed := false
var args := OS.get_cmdline_user_args()
var is_host := "--host" in args or "--host-campaign" in args
var campaign := "--host-campaign" in args
var world_seen_at := -1
var joiner_seen_at := -1
var checked := false
var quit_at := -1
var had_save := false            # a campaign saved before the test is left alone


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	settings = root.get_node("/root/Settings")
	for profile in ["single", "online"]:
		settings.use_profile(profile)
		settings.pilot_name = "HostPilot" if is_host else "Joiner"
		settings.race_id = 1
	had_save = SaveGame.exists()
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host / --host-campaign / --versus / --join itself


func _find(w, pilot: String) -> Ship:
	for s in w.ships:
		if s.pilot_name == pilot:
			return s
	return null


func _physics_process(_delta: float) -> bool:
	var w = main.world if main != null else null
	var now := Time.get_ticks_msec()
	if w != null and world_seen_at < 0:
		world_seen_at = now
	var net: Node = root.get_node("/root/Net")
	if is_host and w != null:
		var j := _find(w, "Joiner")
		if j != null and joiner_seen_at < 0:
			joiner_seen_at = now
			_check(net.versus, "the host is competitive")
			_check(j.race_id != w.player.race_id and j.race_id in [0, 1, 2], "the joiner flies for a rival empire (%d, host %d)" % [j.race_id, w.player.race_id])
			_check(j.team() != w.player.team(), "the joiner is the host's enemy")
			if campaign:
				_check(w.campaign and w.round_rules.length == 0.0 and net.hosting_campaign, "the hosted game is the saving campaign")
	elif w != null and w.player != null and net.local_net_id != 0 and not checked:
		var h := _find(w, "HostPilot")
		if h != null and now - world_seen_at > 3000:
			checked = true
			_check(w.player.race_id != h.race_id, "the joiner's ship is of another empire than the host's (%d, %d)" % [w.player.race_id, h.race_id])
			_check(w.player.team() != h.team(), "the joiner sees the host as an enemy")
			_check(not w.campaign, "a friend's game never saves the host's war")
			quit_at = now + 4000
	var done: bool = (joiner_seen_at > 0 and now - joiner_seen_at > 10000) or (world_seen_at > 0 and now - world_seen_at > 60000) if is_host \
		else ((quit_at > 0 and now > quit_at) or (world_seen_at > 0 and now - world_seen_at > 40000))
	if done:
		if is_host:
			_check(joiner_seen_at > 0, "the joiner took a seat")
		else:
			_check(checked, "the joiner saw its seat and the host")
		print("[%s] %s" % [_tag(), "FAILED" if failed else "passed"])
		if is_host and campaign and not had_save:
			main.world.campaign = false     # no autosave on the way out
			SaveGame.erase(SaveGame.PATH)
		quit(1 if failed else 0)
	return false


func _tag() -> String:
	return "host" if is_host else "client"


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true
