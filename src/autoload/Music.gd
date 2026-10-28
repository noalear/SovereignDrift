## Music -- plays the soundtrack: the eight genre remixes and the
## eighteen new tracks.  In "follows the fight" mode the game reports an
## intensity (cruise, tension, battle) and the music crossfades to a track of that
## energy; otherwise everything plays on shuffle.  Explosions duck it for
## a beat.
extends Node

signal track_changed(title: String, style: String)

enum Intensity { CRUISE, TENSION, BATTLE }

# file, title, style, energy
const REMIXES := [
	["remix1-space-ambient", "Stellar Frontier I", "space ambient", Intensity.CRUISE],
	["remix2-synthwave", "Stellar Frontier II", "synthwave", Intensity.TENSION],
	["remix3-liquid-dnb", "Stellar Frontier III", "liquid drum & bass", Intensity.BATTLE],
	["remix4-chiptune", "Stellar Frontier IV", "chiptune", Intensity.TENSION],
	["remix5-edm-trance", "Stellar Frontier V", "trance", Intensity.BATTLE],
	["remix6-symphonic-metal", "Stellar Frontier VI", "symphonic metal", Intensity.BATTLE],
	["remix7-epic-orchestral", "Stellar Frontier VII", "epic orchestral", Intensity.BATTLE],
	["remix8-dubstep", "Stellar Frontier VIII", "dubstep", Intensity.BATTLE],
	["remix8-darkwave", "Stellar Frontier VIII", "darkwave", Intensity.TENSION],
]
const NEW_TRACKS := [
	["new01-graveyard-orbit", "Graveyard Orbit", "trip-hop", Intensity.CRUISE],
	["new02-across-the-heliopause", "Across the Heliopause", "progressive house", Intensity.TENSION],
	["new03-lagrange-drift", "Lagrange Drift", "atmospheric jungle", Intensity.TENSION],
	["new04-tritone-run", "Tritone Run", "big beat", Intensity.BATTLE],
	["new05-nebula-caravan", "Nebula Caravan", "goa trance", Intensity.TENSION],
	["new06-black-shipyards", "Black Shipyards", "industrial", Intensity.BATTLE],
	["new07-rimward-patrol", "Rimward Patrol", "cinematic electronica", Intensity.TENSION],
	["new08-long-haul-to-kepler-gate", "Long Haul to Kepler Gate", "Detroit techno", Intensity.TENSION],
	["new09-far-sun-meridian", "Far Sun Meridian", "space synth", Intensity.CRUISE],
	["new10-lantern-orbit", "Lantern Orbit", "ambient techno", Intensity.CRUISE],
	["new11-dead-reckoning", "Dead Reckoning", "dub techno", Intensity.CRUISE],
	["new12-solar-wind-archive", "Solar Wind Archive", "Berlin-school sequencer", Intensity.CRUISE],
	["new13-reactor-hall", "Reactor Hall", "industrial techno", Intensity.TENSION],
	["new14-cold-signal", "Cold Signal", "Detroit electro", Intensity.TENSION],
	["new15-hull-breach", "Hull Breach", "hard industrial techno", Intensity.BATTLE],
	["new16-kill-vector", "Kill Vector", "neurofunk drum & bass", Intensity.BATTLE],
	["new17-iron-armada", "Iron Armada", "orchestral industrial", Intensity.BATTLE],
	["new18-quiet-drive", "Quiet Drive", "ambient breaks", Intensity.CRUISE],
]

const CROSSFADE := 2.5
# How long the game's read must hold before the music accepts it.  Calming
# down waits much longer than heating up, so a contact that drifts in and
# out of sensor range does not flip the mood back and forth.
const SWITCH_DELAY := {Intensity.CRUISE: 30.0, Intensity.TENSION: 8.0, Intensity.BATTLE: 3.0}
# How long a track plays before a mood change may cut it: a fight starting
# cuts in fairly soon, calming down lets the track play on for a good while.
const MIN_PLAY_UP := 20.0
const MIN_PLAY_DOWN := 90.0

var players: Array[AudioStreamPlayer] = []
var active := 0                  # index of the player carrying the current track
var fade := 1.0                  # 0..1 progress of the crossfade into `active`
var current: Array = []
var recent: Array = []           # files played lately, avoided when picking
var intensity := Intensity.CRUISE
var wanted := Intensity.CRUISE
var wanted_since := 0.0
var track_started := 0.0         # `time` when the current track began
var duck := 0.0                  # 0..1 extra attenuation
var base_db := 0.0
var time := 0.0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	for i in 2:
		var p := AudioStreamPlayer.new()
		p.bus = "Master"
		add_child(p)
		p.finished.connect(_on_finished.bind(i))
		players.append(p)
	set_volume(Settings.music_volume)
	start()


func all_tracks() -> Array:
	var list := []
	if Settings.music_mode in ["all", "remixes"]:
		for t in REMIXES:
			if t[0] == "remix8-dubstep" and Settings.song8 != "dubstep":
				continue
			if t[0] == "remix8-darkwave" and Settings.song8 != "darkwave":
				continue
			list.append(t)
	if Settings.music_mode in ["all", "new"]:
		list.append_array(NEW_TRACKS)
	return list


func pool(level: int) -> Array:
	var all := all_tracks()
	if not Settings.dynamic_music:
		return all
	var out := all.filter(func(t): return t[3] == level)
	if out.size() < 2:
		# too few tracks of that energy in the chosen set: widen to the neighbours
		out = all.filter(func(t): return absi(t[3] - level) <= 1)
	return out if not out.is_empty() else all


func start() -> void:
	recent.clear()
	if Settings.music_mode == "off":
		for p in players:
			p.stop()
		current = []
		return
	_play_from(pool(intensity), false)


## The game's read of the situation; the music follows after a delay so a
## stray contact does not flip the track.
func set_intensity(level: int) -> void:
	if level != wanted:
		wanted = level
		wanted_since = time


func next_track() -> void:
	if Settings.music_mode == "off":
		return
	_play_from(pool(intensity), true)


func _pick(from: Array) -> Array:
	var choices := from.filter(func(t): return not recent.has(t[0]))
	if choices.is_empty():
		recent.clear()
		choices = from.duplicate()
	if choices.size() > 1 and not current.is_empty():
		choices = choices.filter(func(t): return t[0] != current[0])
	return choices[randi() % choices.size()]


func _play_from(from: Array, crossfade: bool) -> void:
	if from.is_empty():
		return
	var track := _pick(from)
	var path := "res://assets/music/%s/%s.ogg" % ["remix" if track[0].begins_with("remix") else "new", track[0]]
	var stream := load(path)
	if stream == null:
		push_warning("Missing music: " + path)
		return
	var next := 1 - active if crossfade else active
	if not crossfade:
		players[1 - active].stop()
	players[next].stream = stream
	players[next].play()
	active = next
	fade = 0.0 if crossfade else 1.0
	current = track
	track_started = time
	recent.append(track[0])
	if recent.size() > 6:
		recent.pop_front()
	track_changed.emit(track[1], track[2])


func set_volume(v: float) -> void:
	Settings.music_volume = v
	base_db = linear_to_db(clampf(v, 0.0001, 1.0)) if v > 0.001 else -80.0


## Lower the music for a moment (explosions).  amount 0..1.
func duck_for(amount: float) -> void:
	duck = maxf(duck, clampf(amount, 0.0, 1.0))


func now_playing() -> String:
	if current.is_empty():
		return ""
	return "%s  (%s)" % [current[1], current[2]]


func intensity_name() -> String:
	return ["cruise", "tension", "battle"][intensity]


func _process(delta: float) -> void:
	time += delta
	if fade < 1.0:
		fade = minf(fade + delta / CROSSFADE, 1.0)
		if fade >= 1.0:
			players[1 - active].stop()
	duck = maxf(duck - delta * 0.9, 0.0)
	var duck_db := -12.0 * duck
	var a_db := base_db + linear_to_db(maxf(fade, 0.001)) + duck_db
	var b_db := base_db + linear_to_db(maxf(1.0 - fade, 0.001)) + duck_db
	players[active].volume_db = a_db if base_db > -79.0 else -80.0
	players[1 - active].volume_db = b_db if (base_db > -79.0 and fade < 1.0) else -80.0
	# follow the fight
	if not Settings.dynamic_music or Settings.music_mode == "off":
		return
	if wanted != intensity and time - wanted_since >= SWITCH_DELAY[wanted]:
		intensity = wanted
	if current.is_empty() or fade < 1.0 or _fits(current, intensity):
		return
	var min_play := MIN_PLAY_UP if intensity > current[3] else MIN_PLAY_DOWN
	if time - track_started >= min_play:
		_play_from(pool(intensity), true)


## Whether a track may keep playing in this mood: one the pool for that
## mood would pick (the pool widens to neighbouring moods when a set has few
## tracks), or one a step hotter, so a fight that flares up and dies down
## keeps its battle track until the track ends.
func _fits(track: Array, level: int) -> bool:
	if track[3] == level + 1:
		return true
	return pool(level).any(func(t): return t[0] == track[0])


func _on_finished(index: int) -> void:
	if index != active or Settings.music_mode == "off":
		return
	_play_from(pool(intensity), false)
