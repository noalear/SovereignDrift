## Sfx -- sound effects synthesised at start-up (no sample files are carried
## over from the original game).  Each effect is rendered once into an
## AudioStreamWAV and played through a small pool of 2D players.
extends Node

const RATE := 22050
const POOL := 24

var sounds := {}
var players: Array[AudioStreamPlayer2D] = []
var flat_players: Array[AudioStreamPlayer] = []
var next_player := 0
var rng := RandomNumberGenerator.new()


func _ready() -> void:
	rng.seed = 1997
	sounds["torpedo"] = _render(0.18, func(t, d): return _env(t, d, 0.002, 0.9) * (_saw(t, 420.0 - 900.0 * t) * 0.5 + _noise() * 0.2))
	sounds["plasma"] = _render(0.3, func(t, d): return _env(t, d, 0.005, 0.8) * (sin(TAU * (220.0 - 300.0 * t) * t) * 0.6 + _square(t, 110.0) * 0.2))
	sounds["rapid"] = _render(0.08, func(t, d): return _env(t, d, 0.001, 0.95) * _saw(t, 900.0 - 2500.0 * t) * 0.4)
	sounds["grenade"] = _render(0.35, func(t, d): return _env(t, d, 0.01, 0.7) * (sin(TAU * 90.0 * t) * 0.7 + _noise() * 0.3))
	sounds["missile"] = _render(0.6, func(t, d): return _env(t, d, 0.02, 0.5) * (_noise() * 0.35 + sin(TAU * (600.0 + 400.0 * t) * t) * 0.2))
	sounds["mine"] = _render(0.25, func(t, d): return _env(t, d, 0.01, 0.6) * _square(t, 160.0) * 0.3)
	sounds["cannon"] = _render(0.4, func(t, d): return _env(t, d, 0.002, 0.6) * (sin(TAU * (70.0 - 40.0 * t) * t) * 0.8 + _noise() * 0.4))
	sounds["beam"] = _render(0.25, func(t, d): return _env(t, d, 0.01, 0.99) * (_saw(t, 1200.0 + 300.0 * sin(60.0 * t)) * 0.25))
	sounds["hit"] = _render(0.15, func(t, d): return _env(t, d, 0.001, 0.8) * (_noise() * 0.5 + sin(TAU * 300.0 * t) * 0.3))
	sounds["shield"] = _render(0.2, func(t, d): return _env(t, d, 0.002, 0.85) * (sin(TAU * (1800.0 - 1200.0 * t) * t) * 0.35))
	sounds["explode"] = _render(1.4, func(t, d): return _env(t, d, 0.01, 0.3) * (_noise() * 0.9 + sin(TAU * (60.0 - 30.0 * t) * t) * 0.5))
	sounds["bigexplode"] = _render(2.6, func(t, d): return _env(t, d, 0.02, 0.25) * (_noise() * 1.0 + sin(TAU * (45.0 - 20.0 * t) * t) * 0.7))
	sounds["thrust"] = _render(0.5, func(t, d): return 0.18 * _noise() * (0.6 + 0.4 * sin(TAU * 30.0 * t)), true)
	sounds["boost"] = _render(0.5, func(t, d): return 0.22 * (_noise() * 0.7 + _saw(t, 55.0) * 0.3), true)
	sounds["warpin"] = _render(1.2, func(t, d): return _env(t, d, 0.3, 0.9) * sin(TAU * (120.0 + 1400.0 * t * t) * t) * 0.5)
	sounds["warpout"] = _render(0.8, func(t, d): return _env(t, d, 0.01, 0.6) * sin(TAU * (1500.0 - 1300.0 * t) * t) * 0.5)
	sounds["cloak"] = _render(0.7, func(t, d): return _env(t, d, 0.05, 0.8) * (sin(TAU * (900.0 - 700.0 * t) * t) * 0.3 + _noise() * 0.08))
	sounds["uncloak"] = _render(0.7, func(t, d): return _env(t, d, 0.05, 0.8) * (sin(TAU * (200.0 + 700.0 * t) * t) * 0.3 + _noise() * 0.08))
	sounds["pickup"] = _render(0.4, func(t, d): return _env(t, d, 0.01, 0.8) * (sin(TAU * 660.0 * t) + sin(TAU * 990.0 * minf(t * 2.0, 1.0) * t)) * 0.25)
	sounds["rankup"] = _render(1.2, func(t, d): return _env(t, d, 0.02, 0.85) * (sin(TAU * 523.0 * t) * (1.0 if t < 0.3 else 0.0) + sin(TAU * 659.0 * t) * (1.0 if t >= 0.3 and t < 0.6 else 0.0) + sin(TAU * 784.0 * t) * (1.0 if t >= 0.6 else 0.0)) * 0.3)
	sounds["click"] = _render(0.05, func(t, d): return _env(t, d, 0.001, 0.9) * sin(TAU * 1200.0 * t) * 0.3)
	sounds["alarm"] = _render(0.5, func(t, d): return _env(t, d, 0.01, 0.95) * _square(t, 440.0 if fmod(t, 0.25) < 0.125 else 330.0) * 0.2)
	sounds["dock"] = _render(0.6, func(t, d): return _env(t, d, 0.05, 0.85) * (sin(TAU * 330.0 * t) + sin(TAU * 440.0 * t)) * 0.2)
	for i in POOL:
		var p := AudioStreamPlayer2D.new()
		p.max_distance = 2600.0
		p.attenuation = 1.4
		p.bus = "Master"
		add_child(p)
		players.append(p)
	for i in 4:
		var p := AudioStreamPlayer.new()
		add_child(p)
		flat_players.append(p)


func play(name: String, at: Vector2, volume: float = 1.0, pitch: float = 1.0) -> void:
	if not sounds.has(name) or Settings.sfx_volume <= 0.001:
		return
	var p := players[next_player]
	next_player = (next_player + 1) % POOL
	p.stream = sounds[name]
	p.global_position = at
	p.volume_db = linear_to_db(clampf(volume * Settings.sfx_volume, 0.001, 1.0))
	p.pitch_scale = pitch
	p.play()


func play_ui(name: String, volume: float = 1.0) -> void:
	if not sounds.has(name) or Settings.sfx_volume <= 0.001:
		return
	for p in flat_players:
		if not p.playing:
			p.stream = sounds[name]
			p.volume_db = linear_to_db(clampf(volume * Settings.sfx_volume, 0.001, 1.0))
			p.play()
			return


## A looping player the caller owns (engine noise).
func make_loop(name: String) -> AudioStreamPlayer2D:
	var p := AudioStreamPlayer2D.new()
	p.stream = sounds[name]
	p.max_distance = 1800.0
	p.attenuation = 1.5
	p.volume_db = -80.0
	return p


# ------------------------------------------------------------ synthesis

func _render(seconds: float, f: Callable, loop: bool = false) -> AudioStreamWAV:
	var n := int(seconds * RATE)
	var data := PackedByteArray()
	data.resize(n * 2)
	var last := 0.0
	for i in n:
		var t := float(i) / RATE
		var v: float = f.call(t, seconds)
		# gentle one-pole low pass to tame the noise
		v = last + 0.55 * (v - last)
		last = v
		var s := int(clampf(v, -1.0, 1.0) * 32000.0)
		data.encode_s16(i * 2, s)
	var wav := AudioStreamWAV.new()
	wav.format = AudioStreamWAV.FORMAT_16_BITS
	wav.mix_rate = RATE
	wav.stereo = false
	wav.data = data
	if loop:
		wav.loop_mode = AudioStreamWAV.LOOP_FORWARD
		wav.loop_begin = 0
		wav.loop_end = n
	return wav


func _env(t: float, d: float, attack: float, decay_shape: float) -> float:
	var a := minf(t / maxf(attack, 0.0001), 1.0)
	var r := 1.0 - t / d
	return a * pow(maxf(r, 0.0), 1.0 + decay_shape * 3.0)


func _noise() -> float:
	return rng.randf_range(-1.0, 1.0)


func _saw(t: float, hz: float) -> float:
	var ph := fmod(t * hz, 1.0)
	return ph * 2.0 - 1.0


func _square(t: float, hz: float) -> float:
	return 1.0 if fmod(t * hz, 1.0) < 0.5 else -1.0
