# Music switching test: feeds the music a jittery read of the fight (as the
# game's combat check gives when enemies drift in and out of range) and
# checks that tracks play through instead of flipping.  Usage:
#   godot --headless --path . -s tools/music_test.gd
extends SceneTree

const STEP := 0.5


func _init() -> void:
	call_deferred("_run")


func _run() -> void:
	var music: Node = root.get_node("Music")
	music.set_process(false)
	var settings: Node = root.get_node("Settings")
	settings.dynamic_music = true
	settings.music_mode = "all"
	music.start()
	var changes: Array = []
	music.track_changed.connect(func(title, _style): changes.append([music.time, title]))
	var failed := false
	var C: int = music.Intensity.CRUISE
	var T: int = music.Intensity.TENSION
	var B: int = music.Intensity.BATTLE
	# 1. contacts drifting in and out of range: tension and cruise in turns
	failed = _phase(music, changes, "flap tension/cruise 6 s", 300.0, func(t): return T if int(t / 6.0) % 2 == 0 else C, 2) or failed
	failed = _phase(music, changes, "flap tension/cruise 20 s", 300.0, func(t): return T if int(t / 40.0) % 2 == 0 else C, 2) or failed
	# 2. a skirmish: bursts of fire (battle) with lulls (tension)
	failed = _phase(music, changes, "flap battle/tension 4 s", 300.0, func(t): return B if int(t / 4.0) % 2 == 0 else T, 2) or failed
	failed = _phase(music, changes, "skirmish 8 s on, 12 s off", 300.0, func(t): return B if fmod(t, 20.0) < 8.0 else T, 2) or failed
	# 3. a real fight: calm, then battle held -> must switch to a battle track soon
	_phase(music, changes, "settle in cruise", 200.0, func(_t): return C, 99)
	var before: int = changes.size()
	_phase(music, changes, "sustained battle", 40.0, func(_t): return B, 99)
	var cur: Array = music.current
	if changes.size() == before and cur[3] != B:
		print("FAIL: a sustained fight did not bring in battle music")
		failed = true
	else:
		print("ok: sustained battle -> ", cur[1], " (", cur[2], ")")
	print("FAILED" if failed else "PASSED")
	quit(1 if failed else 0)


## Run the music for `seconds` of game time with the mood given by
## `level_at(t)`; fail when it changed track more than `max_changes` times.
func _phase(music: Node, changes: Array, name: String, seconds: float, level_at: Callable, max_changes: int) -> bool:
	var start: int = changes.size()
	var t := 0.0
	while t < seconds:
		music.set_intensity(level_at.call(t))
		music._process(STEP)
		t += STEP
	var n: int = changes.size() - start
	print("%s: %d track changes in %d s" % [name, n, int(seconds)])
	if n > max_changes:
		print("FAIL: ", name, " changed track too often")
		return true
	return false
