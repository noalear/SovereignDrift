## Comms -- the story campaign's voice on screen: the video call window
## with whoever is talking, the subtitles under the action, and the sound.
## It plays the line Story says is being spoken, and runs Story's voice
## clock (also while the war waits for a briefing or the shipyard).
class_name Comms
extends Control

const WIN_W := 300.0
const WIN_H := 225.0
const LINGER := 1.6               # seconds the window stays after the last line
const CONNECT_TIME := 0.35        # the static burst as a call comes in

var story: Story
var main: Node
var font: Font
var voice: AudioStreamPlayer
var time := 0.0
var line := ""                    # what is on screen
var line_time := 0.0              # seconds into it
var line_len := 1.0
var shown_speaker := ""           # who is in the window
var shown_place := ""
var connect_t := 0.0              # static left from a new caller
var linger := 0.0
var talk := 0.0                   # how open the mouth is, smoothed
var _analyzer: AudioEffectSpectrumAnalyzerInstance = null


func setup(p_story: Story, p_main: Node) -> void:
	story = p_story
	main = p_main


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	voice = AudioStreamPlayer.new()
	voice.bus = "Voice" if AudioServer.get_bus_index("Voice") >= 0 else "Master"
	voice.process_mode = Node.PROCESS_MODE_ALWAYS
	add_child(voice)
	_analyzer = _voice_meter(voice.bus)
	if story != null:
		story.line_started.connect(_on_line)


## A level meter on the voice bus, put there at run time: it moves the
## caller's mouth with the words.
static func _voice_meter(bus: String) -> AudioEffectSpectrumAnalyzerInstance:
	var idx := AudioServer.get_bus_index(bus)
	if idx < 0:
		return null
	for i in AudioServer.get_bus_effect_count(idx):
		if AudioServer.get_bus_effect(idx, i) is AudioEffectSpectrumAnalyzer:
			return AudioServer.get_bus_effect_instance(idx, i)
	var fx := AudioEffectSpectrumAnalyzer.new()
	fx.buffer_length = 0.1
	AudioServer.add_bus_effect(idx, fx)
	return AudioServer.get_bus_effect_instance(idx, AudioServer.get_bus_effect_count(idx) - 1)


func _on_line(id: String) -> void:
	var r := StoryLines.row(id)
	if r.is_empty():
		return
	if r[1] != shown_speaker or r[2] != shown_place:
		connect_t = CONNECT_TIME
	line = id
	line_time = 0.0
	shown_speaker = r[1]
	shown_place = r[2]
	linger = LINGER
	voice.stop()
	var path := StoryLines.audio_path(id)
	var st: AudioStream = load(path) if ResourceLoader.exists(path) else null
	line_len = Story.line_length(id)
	if st != null:
		voice.stream = st
		voice.play()
	Sfx.play_ui("click", 0.25)


func _menu_up() -> bool:
	return main != null and main.get("pause_panel") != null


func _process(delta: float) -> void:
	time += delta
	if story == null:
		return
	var held := _menu_up()
	voice.stream_paused = held
	if not held:
		story.tick_voice(delta)
		if line != "":
			line_time += delta
		if story.line == "" and line != "":
			line = ""
		if line == "" and linger > 0.0:
			linger -= delta
		connect_t = maxf(connect_t - delta, 0.0)
	if line != "":
		Music.duck_for(0.45)
	# the mouth: the voice's level, or a flap at speaking pace with no sound
	var lvl := 0.0
	if line != "" and line_time < line_len - 0.3:
		if _analyzer != null and voice.playing:
			var m := _analyzer.get_magnitude_for_frequency_range(150.0, 3500.0)
			lvl = clampf((m.x + m.y) * 9.0, 0.0, 1.0)
		if lvl <= 0.01 and not voice.playing:
			lvl = 0.5 + 0.5 * sin(time * 19.0) if fmod(time, 1.7) < 1.4 else 0.0
	talk = lerpf(talk, lvl, clampf(delta * 18.0, 0.0, 1.0))
	queue_redraw()


func _covered() -> bool:
	if main == null:
		return false
	return main.get("builder") != null or main.get("tactical") != null or main.get("ledger") != null or main.get("briefing") != null


## How bad the signal is: a new call, a hit on the player, a jammer.
## Enemy callers always come through clean (Kyle).
func _noise() -> float:
	if StoryLines.SPEAKERS.get(shown_speaker, {}).get("race", 1) != 1:
		return 0.0
	var n := connect_t / CONNECT_TIME
	var w: Node = story.world if story != null else null
	if w != null and w.player != null and not w.player.dead:
		n = maxf(n, clampf(1.0 - (w.elapsed - w.last_player_damage) / 0.6, 0.0, 1.0) * 0.6)
		if w.player.sensors.is_jammed():
			n = maxf(n, 0.55)
	return clampf(n, 0.0, 1.0)


func _draw() -> void:
	if story == null or main == null or main.get("briefing") != null:
		return                         # the briefing draws its own big call
	var vp := get_viewport_rect().size
	var on := line != "" or linger > 0.0
	if not on:
		return
	var a := 1.0 if line != "" else clampf(linger / 0.5, 0.0, 1.0)
	if not _covered():
		var r := Rect2(16.0, maxf(vp.y - 250.0 - WIN_H - 34.0, 300.0), WIN_W, WIN_H)
		_draw_window(r, a)
	if line != "":
		_draw_subtitles(vp)


func _draw_window(r: Rect2, a: float) -> void:
	var spk: Dictionary = StoryLines.SPEAKERS.get(shown_speaker, {})
	var col: Color = spk.get("color", Color(0.6, 0.8, 1.0))
	modulate.a = 1.0
	draw_rect(r.grow(4.0), Color(0.02, 0.04, 0.08, 0.85 * a))
	CallView.draw_call(self, r, shown_speaker, shown_place, talk, _noise(), time)
	if a < 1.0:
		draw_rect(r.grow(5.0), Color(0, 0, 0, 1.0 - a))
	# name plate under the picture
	var plate := Rect2(r.position.x - 4, r.end.y + 4, r.size.x + 8, 30)
	draw_rect(plate, Color(0.02, 0.04, 0.08, 0.9 * a))
	draw_rect(Rect2(plate.position, Vector2(4, plate.size.y)), Color(col.r, col.g, col.b, a))
	_text(Vector2(plate.position.x + 12, plate.position.y + 20), spk.get("name", "").to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(col.r, col.g, col.b, a))
	var place: Dictionary = StoryLines.PLACES.get(shown_place, {})
	_text(Vector2(plate.position.x, plate.position.y + 20), place.get("name", ""), HORIZONTAL_ALIGNMENT_RIGHT, plate.size.x - 10, 12, Color(0.7, 0.78, 0.9, 0.8 * a))
	# live light
	if line != "":
		draw_circle(r.position + Vector2(14, 14), 4.0, Color(1.0, 0.3, 0.25, 0.6 + 0.4 * sin(time * 5.0)))


## The words as they are spoken, low in the middle, clear of the HUD.
func _draw_subtitles(vp: Vector2) -> void:
	var spk: Dictionary = StoryLines.speaker(line)
	var text := StoryLines.text(line)
	var compact := _covered()
	var size := 16 if compact else 18
	var w := minf(760.0, vp.x - 40.0)
	var lines := _wrap(text, w - 40.0, size)
	var lh := 23.0
	var h := 40.0 + lines.size() * lh + 8.0
	var x := (vp.x - w) * 0.5
	var y := (vp.y - h - 24.0) if compact else (vp.y - 215.0 - h)
	draw_rect(Rect2(x, y, w, h), Color(0.02, 0.04, 0.09, 0.9))
	var col: Color = spk.get("color", Color(0.6, 0.8, 1.0))
	draw_rect(Rect2(x, y, 4, h), col)
	_text(Vector2(x + 16, y + 24), "%s  ·  %s" % [spk.get("name", "").to_upper(), spk.get("title", "")], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, col)
	var shown := text.length()
	if line_len > 0.4:
		shown = int(ceil(text.length() * clampf(line_time / maxf(line_len - 0.6, 0.1), 0.0, 1.0)))
	var ly := y + 50.0
	var used := 0
	for ln in lines:
		var part: String = ln
		if used + ln.length() > shown:
			part = ln.left(maxi(shown - used, 0))
		_text(Vector2(x + 16, ly), part, HORIZONTAL_ALIGNMENT_LEFT, -1, size, Color(0.95, 0.97, 1.0))
		used += ln.length() + 1
		ly += lh


func _wrap(text: String, width: float, size: int) -> PackedStringArray:
	var out := PackedStringArray()
	var cur := ""
	for word in text.split(" "):
		var t := word if cur == "" else cur + " " + word
		if cur != "" and font.get_string_size(t, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x > width:
			out.append(cur)
			cur = word
		else:
			cur = t
	if cur != "":
		out.append(cur)
	return out


func _text(pos: Vector2, text: String, align: HorizontalAlignment, width: float, size: int, col: Color) -> void:
	draw_string_outline(font, pos, text, align, width, size, 4, Color(0, 0, 0, 0.75 * col.a))
	draw_string(font, pos, text, align, width, size, col)
