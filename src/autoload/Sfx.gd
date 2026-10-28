## Sfx -- the sound-effect engine.  Effects are pre-rendered by
## tools/sfx/generate.py into assets/sfx/<name>_<NN>.ogg; most names have
## several variants that rotate so a repeated shot never sounds identical.
##
## World sounds play through a pool of 2D players with per-sound voice caps,
## retrigger gaps and priorities (a big explosion may steal a rapid-fire
## voice, never the reverse).  Four buses: "SFX" (near, glued by a
## compressor), "SFXFar" (distant: dulled and roomier), "UI" and "Voice"
## (spoken lines, on Settings.voice_volume).  Bullet
## time dulls the SFX bus.  A small cockpit monitor watches the player's
## ship for lock-on, shield collapse, hull alarm, incoming missiles, etc.
##
## Modules have a voice per level: "engine3", "shield5", "beam_loop2" ...
## leveled("shield", 5) picks the best match that exists.
extends Node

const DIR := "res://assets/sfx/"
const POOL := 40
const UI_POOL := 6
const MAX_DIST := 2600.0     # screen pixels, matches the players' max_distance
const FAR_DIST := 1100.0     # beyond this a sound goes to the far bus
const MAX_LOOPS := 10        # rocket loops alive at once

## name -> [max voices, min gap s, priority, pitch jitter, gain]
const SPEC := {
	"torpedo": [6, 0.03, 4, 0.05, 0.85],
	"rapid": [6, 0.025, 3, 0.07, 0.7],
	"plasma": [5, 0.04, 5, 0.05, 0.85],
	"grenade": [4, 0.05, 5, 0.05, 0.9],
	"missile": [4, 0.06, 5, 0.06, 0.85],
	"mine": [3, 0.1, 4, 0.04, 0.9],
	"cannon": [3, 0.06, 6, 0.05, 1.0],
	"beam": [4, 0.08, 4, 0.06, 0.7],
	"beam_start": [4, 0.05, 4, 0.05, 0.8],
	"beam_stop": [4, 0.05, 3, 0.05, 0.6],
	"battery": [5, 0.05, 5, 0.05, 0.85],
	"ignite": [4, 0.15, 4, 0.04, 0.7],
	"boost_kick": [3, 0.15, 5, 0.04, 0.8],
	"hit": [5, 0.05, 5, 0.12, 0.9],
	"hit_armor": [5, 0.05, 5, 0.1, 0.9],
	"shield": [5, 0.06, 4, 0.1, 0.8],
	"shield_down": [2, 0.5, 7, 0.03, 1.0],
	"impact": [6, 0.03, 5, 0.1, 0.9],
	"collide": [3, 0.15, 5, 0.1, 0.9],
	"explode": [5, 0.04, 8, 0.1, 1.0],
	"bigexplode": [3, 0.1, 9, 0.06, 1.0],
	"warpin": [3, 0.1, 6, 0.0, 0.85],
	"warpout": [3, 0.1, 7, 0.03, 0.9],
	"warpexit": [3, 0.1, 6, 0.03, 0.85],
	"cloak": [3, 0.1, 5, 0.03, 0.8],
	"uncloak": [3, 0.1, 5, 0.03, 0.8],
	"pickup": [3, 0.08, 6, 0.0, 0.8],
	"dock": [2, 0.2, 6, 0.0, 0.9],
	"launch": [2, 0.3, 6, 0.02, 0.8],
}
const DEFAULT_SPEC := [4, 0.05, 5, 0.04, 0.9]
const LOOPS := ["engine", "engine_large", "boost", "beam_loop", "rocket_loop"]
const FLAT := ["rankup", "captured", "victory", "alarm", "incoming", "lock", "power_low",
	"notify", "ui_hover", "ui_click", "ui_back", "ui_start"]
## names the old placeholder engine used
const ALIAS := {"click": "ui_click", "thrust": "engine1", "engine_small": "engine1", "boost_loop": "boost1",
	"shield": "shield1", "warpin": "warpin1", "warpout": "warpout1", "beam_loop": "beam_loop1",
	"beam_start": "beam_start1", "beam_stop": "beam_stop1"}
const MAX_LEVEL := 7

var banks := {}             # name -> Array[AudioStream]
var last_variant := {}      # name -> index
var last_played := {}       # name -> msec
var voices: Array = []      # [{p, name, prio, at}]
var ui_players: Array[AudioStreamPlayer] = []
var beams := {}             # ship instance id -> {p, seen, ship}
var loop_count := 0
var slowmo_lp: AudioEffectLowPassFilter = null

# cockpit monitor state
var watched: Node = null
var w_target: Node = null
var w_dead := false
var w_shield := 0.0
var w_energy_low := false
var w_alarm_at := 0.0
var w_incoming_at := 0.0
var w_world: Node = null
var w_game_over := false


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	_make_buses()
	_load_banks()
	for i in POOL:
		var p := AudioStreamPlayer2D.new()
		p.max_distance = MAX_DIST
		p.attenuation = 1.6
		p.bus = "SFX"
		p.process_mode = Node.PROCESS_MODE_PAUSABLE
		add_child(p)
		voices.append({"p": p, "name": "", "prio": 0, "at": 0})
	for i in UI_POOL:
		var u := AudioStreamPlayer.new()
		u.bus = "UI"
		add_child(u)
		ui_players.append(u)
	get_tree().node_added.connect(_on_node_added)


func _make_buses() -> void:
	if AudioServer.get_bus_index("SFX") >= 0:
		return
	var master := AudioServer.get_bus_index("Master")
	# a brick-wall limiter on the master keeps music + effects from clipping
	var lim := AudioEffectHardLimiter.new()
	lim.ceiling_db = -0.5
	AudioServer.add_bus_effect(master, lim)

	AudioServer.add_bus()
	var sfx := AudioServer.bus_count - 1
	AudioServer.set_bus_name(sfx, "SFX")
	AudioServer.set_bus_send(sfx, "Master")
	var comp := AudioEffectCompressor.new()
	comp.threshold = -14.0
	comp.ratio = 3.0
	comp.attack_us = 2000.0
	comp.release_ms = 180.0
	AudioServer.add_bus_effect(sfx, comp)
	slowmo_lp = AudioEffectLowPassFilter.new()
	slowmo_lp.cutoff_hz = 20000.0
	slowmo_lp.resonance = 0.6
	AudioServer.add_bus_effect(sfx, slowmo_lp)
	AudioServer.set_bus_effect_enabled(sfx, 1, false)

	AudioServer.add_bus()
	var far := AudioServer.bus_count - 1
	AudioServer.set_bus_name(far, "SFXFar")
	AudioServer.set_bus_send(far, "SFX")
	var flp := AudioEffectLowPassFilter.new()
	flp.cutoff_hz = 2200.0
	AudioServer.add_bus_effect(far, flp)
	var rev := AudioEffectReverb.new()
	rev.room_size = 0.7
	rev.damping = 0.6
	rev.wet = 0.35
	rev.dry = 0.8
	rev.spread = 0.8
	AudioServer.add_bus_effect(far, rev)

	AudioServer.add_bus()
	var ui := AudioServer.bus_count - 1
	AudioServer.set_bus_name(ui, "UI")
	AudioServer.set_bus_send(ui, "Master")

	# spoken lines, on their own volume
	AudioServer.add_bus()
	var voice := AudioServer.bus_count - 1
	AudioServer.set_bus_name(voice, "Voice")
	AudioServer.set_bus_send(voice, "Master")


func _load_banks() -> void:
	# Probe <name>_01.ogg, _02 ... (works in exported builds, where the
	# directory only holds .import remaps).
	var bases: Array = SPEC.keys()
	bases.append_array(LOOPS)
	bases.append_array(FLAT)
	for base in bases:
		_load_bank(base, base in LOOPS)
		for lvl in range(1, MAX_LEVEL + 1):
			_load_bank("%s%d" % [base, lvl], base in LOOPS)


func _load_bank(name: String, loop: bool) -> void:
	var list: Array[AudioStream] = []
	for i in range(1, 13):
		var path := "%s%s_%02d.ogg" % [DIR, name, i]
		if not ResourceLoader.exists(path):
			break
		var s: AudioStream = load(path)
		if loop and s is AudioStreamOggVorbis:
			s.loop = true
		list.append(s)
	if not list.is_empty():
		banks[name] = list


## The sound for a module of the given level: name+level if it exists,
## else the nearest lower level, else the plain name.
func leveled(name: String, level: int) -> String:
	for l in range(clampi(level, 1, MAX_LEVEL), 0, -1):
		if banks.has("%s%d" % [name, l]):
			return "%s%d" % [name, l]
	return name


## Voice settings for a name, falling back from "shield5" to "shield".
func _spec(name: String) -> Array:
	if SPEC.has(name):
		return SPEC[name]
	var base := name.rstrip("0123456789")
	return SPEC.get(base, DEFAULT_SPEC)


func _resolve(name: String) -> String:
	return ALIAS.get(name, name)


func _variant(name: String) -> AudioStream:
	var bank: Array = banks[name]
	if bank.size() == 1:
		return bank[0]
	var i := randi() % bank.size()
	if i == last_variant.get(name, -1):
		i = (i + 1 + randi() % (bank.size() - 1)) % bank.size()
	last_variant[name] = i
	return bank[i]


func has(name: String) -> bool:
	return banks.has(_resolve(name))


func stream(name: String) -> AudioStream:
	name = _resolve(name)
	return _variant(name) if banks.has(name) else null


func _screen_distance(at: Vector2) -> float:
	var vp := get_viewport()
	var sp := vp.get_canvas_transform() * at
	return sp.distance_to(vp.get_visible_rect().size * 0.5)


# ------------------------------------------------------------- one-shots

## A positional sound in the world.
func play(name: String, at: Vector2, volume: float = 1.0, pitch: float = 1.0) -> void:
	name = _resolve(name)
	if not banks.has(name):
		return
	var spec: Array = _spec(name)
	var now := Time.get_ticks_msec()
	if now - int(last_played.get(name, -100000)) < int(spec[1] * 1000.0):
		return
	var dist := _screen_distance(at)
	if dist > MAX_DIST:
		return
	var v: Variant = _take_voice(name, spec)
	if v == null:
		return
	last_played[name] = now
	var p: AudioStreamPlayer2D = v["p"]
	p.stop()
	p.stream = _variant(name)
	p.global_position = at
	p.bus = "SFXFar" if dist > FAR_DIST else "SFX"
	var j: float = spec[3]
	p.pitch_scale = maxf(pitch * (1.0 + randf_range(-j, j)), 0.05)
	p.volume_db = linear_to_db(clampf(volume * spec[4] * randf_range(0.9, 1.0), 0.0001, 2.0))
	p.play()
	v["name"] = name
	v["prio"] = spec[2]
	v["at"] = now


func _take_voice(name: String, spec: Array) -> Variant:
	var same: Array = []
	var free: Variant = null
	for v in voices:
		if not v["p"].playing:
			if free == null:
				free = v
		elif v["name"] == name:
			same.append(v)
	# too many of this sound already: recycle the oldest one
	if same.size() >= int(spec[0]):
		same.sort_custom(func(a, b): return a["at"] < b["at"])
		return same[0]
	if free != null:
		return free
	# pool full: steal the oldest, least important voice we outrank
	var best: Variant = null
	for v in voices:
		if v["prio"] > int(spec[2]):
			continue
		if best == null or v["prio"] < best["prio"] or (v["prio"] == best["prio"] and v["at"] < best["at"]):
			best = v
	return best


## A flat (non-positional) sound: interface and cockpit.
func play_ui(name: String, volume: float = 1.0, pitch: float = 1.0) -> void:
	name = _resolve(name)
	if not banks.has(name):
		return
	var now := Time.get_ticks_msec()
	if now - int(last_played.get("ui:" + name, -100000)) < 40:
		return
	last_played["ui:" + name] = now
	var pick: AudioStreamPlayer = null
	for u in ui_players:
		if not u.playing:
			pick = u
			break
	if pick == null:
		pick = ui_players[0]
		ui_players.push_back(ui_players.pop_front())
	pick.stream = _variant(name)
	pick.volume_db = linear_to_db(clampf(volume, 0.0001, 2.0))
	pick.pitch_scale = pitch
	pick.play()


# ----------------------------------------------------------------- loops

## A looping 2D player the caller adds to its own node.
func make_loop(name: String) -> AudioStreamPlayer2D:
	var p := AudioStreamPlayer2D.new()
	p.stream = stream(name)
	p.max_distance = 1800.0
	p.attenuation = 1.5
	p.bus = "SFX"
	p.volume_db = -80.0
	return p


## Hang a looping sound on a moving node (e.g. a seeker missile).  Returns
## null when too many such loops are already running.
func attach_loop(node: Node2D, name: String, volume: float = 1.0) -> AudioStreamPlayer2D:
	if loop_count >= MAX_LOOPS or not has(name):
		return null
	var p := make_loop(name)
	p.volume_db = linear_to_db(volume)
	p.max_distance = MAX_DIST * 0.6
	node.add_child(p)
	p.play(randf() * 1.5)
	loop_count += 1
	p.tree_exiting.connect(func(): loop_count -= 1)
	return p


## Call every frame a ship's beam is firing; the hum starts, follows the
## ship and stops on its own a moment after the calls stop.  Level picks the
## laser's voice (I thin laser, II phaser, III particle lance).
func beam(ship: Node2D, pitch: float = 1.0, level: int = 1) -> void:
	var id := ship.get_instance_id()
	var b: Variant = beams.get(id)
	if b == null:
		var loop_name := leveled("beam_loop", level)
		if not has(loop_name):
			return
		var p := make_loop(loop_name)
		p.max_distance = MAX_DIST
		p.volume_db = linear_to_db(0.55)
		p.pitch_scale = pitch
		ship.add_child(p)
		p.play(randf() * 1.5)
		play(leveled("beam_start", level), ship.global_position, 0.8, pitch)
		b = {"p": p, "seen": 0, "ship": ship, "level": level}
		beams[id] = b
	b["seen"] = Time.get_ticks_msec()


func _update_beams() -> void:
	var now := Time.get_ticks_msec()
	for id in beams.keys():
		var b: Dictionary = beams[id]
		var alive := is_instance_valid(b["ship"]) and is_instance_valid(b["p"])
		if alive and (now - int(b["seen"]) < 120 or get_tree().paused):
			continue
		if alive:
			play(leveled("beam_stop", b["level"]), b["ship"].global_position, 0.8, b["p"].pitch_scale)
			b["p"].queue_free()
		beams.erase(id)


# --------------------------------------------------------- per-frame work

func _process(delta: float) -> void:
	# volume and bullet time live on the buses, so loops follow at once
	var vol_db := linear_to_db(maxf(Settings.sfx_volume, 0.0001))
	var sb := AudioServer.get_bus_index("SFX")
	AudioServer.set_bus_volume_db(sb, vol_db)
	AudioServer.set_bus_volume_db(AudioServer.get_bus_index("UI"), vol_db)
	AudioServer.set_bus_volume_db(AudioServer.get_bus_index("Voice"), linear_to_db(maxf(Settings.voice_volume, 0.0001)))
	if slowmo_lp != null:
		var target := 900.0 if Engine.time_scale < 0.99 else 20000.0
		slowmo_lp.cutoff_hz = lerpf(slowmo_lp.cutoff_hz, target, 1.0 - exp(-delta * 10.0))
		AudioServer.set_bus_effect_enabled(sb, 1, slowmo_lp.cutoff_hz < 19000.0)
	_update_beams()
	_cockpit()


func _cockpit() -> void:
	var world := _current_world()
	if world != w_world:
		w_world = world
		w_game_over = false
		watched = null
		if world != null and world.has_signal("message"):
			world.message.connect(_on_world_message)
	if world == null or get_tree().paused:
		return
	if "game_over" in world and world.game_over and not w_game_over:
		w_game_over = true
		play_ui("victory", 0.9)
	var s: Node = world.player if "player" in world else null
	if s == null or not is_instance_valid(s):
		return
	if s != watched:
		watched = s
		w_target = s.target
		w_dead = s.dead
		w_shield = s.shield
		return
	var now := Time.get_ticks_msec() / 1000.0
	if w_dead and not s.dead:
		play_ui("launch", 0.7)
	w_dead = s.dead
	if s.dead:
		return
	if s.target != w_target:
		w_target = s.target
		if s.target != null:
			play_ui("lock", 0.6)
	if s.shield_max > 0.0 and w_shield > s.shield_max * 0.15 and s.shield <= 0.5:
		play("shield_down", s.global_position, 0.9)
	w_shield = s.shield
	var low: bool = s.energy < s.energy_max * 0.05
	if low and not w_energy_low:
		play_ui("power_low", 0.6)
	w_energy_low = low
	if s.hull < s.hull_max * 0.3 and now - w_alarm_at > 1.6:
		w_alarm_at = now
		play_ui("alarm", 0.4)
	if now - w_incoming_at > 0.4 and "missiles" in world:
		for m in world.missiles:
			if is_instance_valid(m) and m.target == s and m.data.get("guided", false) \
					and m.global_position.distance_to(s.global_position) < 1100.0:
				w_incoming_at = now
				play_ui("incoming", 0.45)
				break


func _current_world() -> Node:
	var scene := get_tree().current_scene
	if scene == null:
		# started without a main scene (tools/smoke.gd adds Main by hand)
		for c in get_tree().root.get_children():
			if "world" in c:
				scene = c
	if scene == null or not ("world" in scene):
		return null
	var w: Variant = scene.world
	return w if w != null and is_instance_valid(w) else null


func _on_world_message(_text: String, color: Color) -> void:
	# World.notify (messages meant for the player alone) uses this colour
	if color.is_equal_approx(Color(0.85, 0.9, 1.0)):
		play_ui("notify", 0.5)


# Every Button in the game gets hover and press sounds without asking.
func _on_node_added(node: Node) -> void:
	if node is BaseButton:
		node.mouse_entered.connect(func(): play_ui("ui_hover", 0.7))
		node.focus_entered.connect(func(): play_ui("ui_hover", 0.5))
		node.pressed.connect(func(): play_ui("ui_click", 0.6))


func _exit_tree() -> void:
	banks.clear()
