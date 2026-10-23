## Music -- plays the soundtrack: the eight genre remixes and the ten new
## tracks, shuffled, with a "now playing" notice for the HUD.
extends Node

signal track_changed(title: String, style: String)

const REMIXES := [
	["remix1-space-ambient", "Stellar Frontier I", "space ambient"],
	["remix2-synthwave", "Stellar Frontier II", "synthwave"],
	["remix3-liquid-dnb", "Stellar Frontier III", "liquid drum & bass"],
	["remix4-chiptune", "Stellar Frontier IV", "chiptune"],
	["remix5-edm-trance", "Stellar Frontier V", "trance"],
	["remix6-symphonic-metal", "Stellar Frontier VI", "symphonic metal"],
	["remix7-epic-orchestral", "Stellar Frontier VII", "epic orchestral"],
	["remix8-dubstep", "Stellar Frontier VIII", "dubstep"],
	["remix8-darkwave", "Stellar Frontier VIII", "darkwave"],
]
const NEW_TRACKS := [
	["new01-graveyard-orbit", "Graveyard Orbit", "trip-hop"],
	["new02-across-the-heliopause", "Across the Heliopause", "progressive house"],
	["new03-lagrange-drift", "Lagrange Drift", "atmospheric jungle"],
	["new04-tritone-run", "Tritone Run", "big beat"],
	["new05-nebula-caravan", "Nebula Caravan", "goa trance"],
	["new06-black-shipyards", "Black Shipyards", "industrial"],
	["new07-rimward-patrol", "Rimward Patrol", "cinematic electronica"],
	["new08-long-haul-to-kepler-gate", "Long Haul to Kepler Gate", "Detroit techno"],
	["new09-far-sun-meridian", "Far Sun Meridian", "space synth"],
	["new10-lantern-orbit", "Lantern Orbit", "ambient techno"],
]

var player: AudioStreamPlayer
var queue: Array = []
var current: Array = []
var history: Array = []


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	player = AudioStreamPlayer.new()
	player.bus = "Master"
	add_child(player)
	player.finished.connect(_on_finished)
	set_volume(Settings.music_volume)
	start()


func playlist() -> Array:
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


func start() -> void:
	queue.clear()
	if Settings.music_mode == "off":
		player.stop()
		current = []
		return
	next_track()


func next_track() -> void:
	if Settings.music_mode == "off":
		return
	if queue.is_empty():
		queue = playlist()
		queue.shuffle()
		# Avoid repeating the song that just ended.
		if queue.size() > 1 and not current.is_empty() and queue[0][0] == current[0]:
			queue.append(queue.pop_front())
	current = queue.pop_front()
	var path := "res://assets/music/%s/%s.ogg" % ["remix" if current[0].begins_with("remix") else "new", current[0]]
	var stream := load(path)
	if stream == null:
		push_warning("Missing music: " + path)
		return
	player.stream = stream
	player.play()
	history.append(current)
	track_changed.emit(current[1], current[2])


func set_volume(v: float) -> void:
	Settings.music_volume = v
	player.volume_db = linear_to_db(clampf(v, 0.0001, 1.0)) if v > 0.001 else -80.0


func now_playing() -> String:
	if current.is_empty():
		return ""
	return "%s  (%s)" % [current[1], current[2]]


func _on_finished() -> void:
	next_track()
