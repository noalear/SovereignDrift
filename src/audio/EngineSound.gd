## EngineSound -- a ship's drive.  Each engine level is its own instrument
## (I chemical rocket, II ion drive, III fusion torch, IV antimatter), each
## booster level its own burner, and stations hum on a capital reactor.
## Throttling up lights the drive with an ignition sound; engaging the
## booster punches.  Pitch and level follow the throttle, smaller hulls sing
## higher, and loops pause when the ship is far off screen.
## Watches the ship's module levels, so refits and salvaged upgrades swap
## the voice by themselves.
class_name EngineSound
extends Node2D

var ship: Node = null
var main: AudioStreamPlayer2D
var boost: AudioStreamPlayer2D
var level := 0.0
var boost_level := 0.0
var idle := 0.0              # stations hum even when still
var base_pitch := 1.0
var eng_level := -1
var boost_mod := -1
var was_thrusting := false
var was_boosting := false
var ignite_at := -10.0


func setup(p_ship: Node) -> void:
	ship = p_ship
	_configure()


func _configure() -> void:
	var levels: Dictionary = ship.module_levels
	eng_level = clampi(int(levels.get("eng", 1)), 1, 4)
	boost_mod = clampi(int(levels.get("boost", 1)), 1, 3)
	idle = 0.35 if ship.is_station else 0.0
	base_pitch = clampf(1.25 - ship.size / 140.0, 0.8, 1.2) * randf_range(0.96, 1.04)
	var main_name := "engine_large" if ship.is_station else Sfx.leveled("engine", eng_level)
	main = _swap(main, main_name, 2000.0)
	boost = _swap(boost, Sfx.leveled("boost", boost_mod), 1600.0)


func _swap(old: AudioStreamPlayer2D, name: String, dist: float) -> AudioStreamPlayer2D:
	if old != null:
		old.queue_free()
	var p := Sfx.make_loop(name)
	p.max_distance = dist
	add_child(p)
	p.play(randf() * 2.0)
	return p


## Call from the ship's physics step.
func update(thrust: float, boosting: bool, warping: bool, delta: float) -> void:
	if main == null:
		return
	var levels: Dictionary = ship.module_levels
	if clampi(int(levels.get("eng", 1)), 1, 4) != eng_level or clampi(int(levels.get("boost", 1)), 1, 3) != boost_mod:
		_configure()
	var far := Sfx._screen_distance(global_position) > main.max_distance * 1.1
	main.stream_paused = far
	boost.stream_paused = far or boost_level < 0.01
	var thrusting := thrust > 0.45
	var now := Time.get_ticks_msec() / 1000.0
	if not far and thrusting and not was_thrusting and not ship.is_station and now - ignite_at > 0.6:
		ignite_at = now
		Sfx.play(Sfx.leveled("ignite", eng_level), global_position, 0.6, base_pitch)
	if not far and boosting and not was_boosting:
		Sfx.play(Sfx.leveled("boost_kick", boost_mod), global_position, 0.8, base_pitch)
	was_thrusting = thrusting
	was_boosting = boosting
	if far:
		return
	var want := maxf(clampf(thrust, 0.0, 1.0), idle) + (0.25 if warping else 0.0)
	level = lerpf(level, want, 1.0 - exp(-delta * 6.0))
	boost_level = lerpf(boost_level, 1.0 if boosting else 0.0, 1.0 - exp(-delta * (8.0 if boosting else 3.0)))
	main.volume_db = linear_to_db(maxf(level * 0.8, 0.0001)) if level > 0.02 else -80.0
	main.pitch_scale = base_pitch * (0.72 + 0.4 * level + 0.15 * boost_level + (0.3 if warping else 0.0))
	boost.volume_db = linear_to_db(maxf(boost_level * 0.75, 0.0001))
	boost.pitch_scale = base_pitch * (0.9 + 0.2 * level)


func silence() -> void:
	level = 0.0
	boost_level = 0.0
	was_thrusting = false
	was_boosting = false
	if main != null:
		main.volume_db = -80.0
		boost.volume_db = -80.0
