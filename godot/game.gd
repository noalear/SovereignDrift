# game.gd -- Stellar Frontier Enhanced Edition: the game screen.
# NEW FILE 2026-10-12 (as main.gd); Enhanced Edition changes 2026-10-13:
#   - the desktop size comes from Settings (any resolution, 16:9 etc.),
#   - pixels are swapped to RGB on the GPU (screen.gdshader),
#   - music (2026-10-18): the remixes plus the new tracks shuffled
#     (game/music_remix, game/music_new), the remixes in place of their
#     songs, the faithful high-fidelity renders (game/music), the original
#     MIDI through Windows, or none.  The program's MCI emulation swaps the
#     songs for the recordings (SF_MUSIC_LIST / SF_MUSIC_MODE, see
#     src/w32emu/w32_audio.cpp), so the playlist follows their lengths,
#   - when the program quits, the launcher comes back.
#
# Runs the original Stellar Frontier program (frontier.exe, a 32-bit process
# in the game/ folder) through the SFProgram node of the sfport extension,
# and connects it to Godot: display, mouse and keyboard (Alt+Enter: full
# screen), sounds, pointer shape and warps, minimise requests.
extends Control

# SF_AUD_* in src/host/sf_host.h
const AUD_DESTROY := 2
const AUD_PLAY := 3
const AUD_STOP := 4
const AUD_VOLUME := 5
const AUD_MUSIC_PLAY := 6
const AUD_MUSIC_STOP := 7

var desktop := Vector2i(1024, 768)
var program: Node = null
var voices := {}            # sound id -> {player, stream, frames, pos}
var picture := Rect2(0, 0, 1024, 768)
var last_cursor := -2
var was_minimized := false
var quitting := false
var music_player: AudioStreamPlayer
var music_file := ""
var music_pos := 0.0

@onready var screen: TextureRect = $Screen
@onready var message: Label = $Message


func _ready() -> void:
	get_tree().set_auto_accept_quit(false)
	get_viewport().size_changed.connect(_layout)
	desktop = Settings.desktop_size()
	music_player = AudioStreamPlayer.new()
	add_child(music_player)
	if not ClassDB.class_exists("SFProgram"):
		_fatal("The sfport extension (addons/sfport) could not be loaded.")
		return
	var exe := Paths.game_exe("frontier")
	if not FileAccess.file_exists(exe):
		_fatal("The game files were not found:\n" + exe)
		return
	program = ClassDB.instantiate("SFProgram")
	program.name = "Program"
	add_child(program)
	program.raw_bgrx = true
	program.music_enabled = Settings.music == "midi"
	_set_music_environment()
	program.log_line.connect(_on_log)
	program.open_url.connect(func(url): OS.shell_open(url))
	program.run_requested.connect(_on_run_requested)
	program.minimize_requested.connect(_on_minimize_requested)
	program.warp_requested.connect(_on_warp)
	program.exited.connect(_on_exited)
	program.sound_created.connect(_on_sound_created)
	program.audio_command.connect(_on_audio_command)
	if not program.start(exe, _program_args(), desktop.x, desktop.y):
		_fatal("The game could not be started:\n" + str(program.get_error()))
		return
	screen.texture = program.get_texture()
	var mat := ShaderMaterial.new()
	mat.shader = preload("res://screen.gdshader")
	screen.material = mat
	_apply_display_mode()
	_layout()


# Command-line arguments after "--" go to the original program (e.g. -log).
func _program_args() -> PackedStringArray:
	var out := PackedStringArray()
	for a in OS.get_cmdline_user_args():
		out.append(a)
	return out


# --------------------------------------------------------------- window ----
func _apply_display_mode() -> void:
	if DisplayServer.get_name() == "headless":
		return
	if Settings.display == "fullscreen":
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
		return
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	var usable := DisplayServer.screen_get_usable_rect(DisplayServer.window_get_current_screen())
	var room := Vector2(usable.size.x - 16, usable.size.y - 48)
	# the largest whole-number scale that fits, or smaller than 1:1 if needed
	var s: float = min(room.x / desktop.x, room.y / desktop.y)
	if s >= 1.0:
		s = floorf(s)
	var win := Vector2i(int(desktop.x * s), int(desktop.y * s))
	DisplayServer.window_set_min_size(Vector2i(640, 480))
	DisplayServer.window_set_size(win)
	DisplayServer.window_set_position(usable.position + (usable.size - win) / 2)


func _toggle_fullscreen() -> void:
	var mode := DisplayServer.window_get_mode()
	if mode == DisplayServer.WINDOW_MODE_FULLSCREEN or mode == DisplayServer.WINDOW_MODE_EXCLUSIVE_FULLSCREEN:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	else:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)


func _layout() -> void:
	var area := get_viewport_rect().size
	if area.x <= 0 or area.y <= 0:
		return
	var s: float = min(area.x / desktop.x, area.y / desktop.y)
	var whole := floorf(s)
	if whole >= 1.0 and whole / s > 0.9:
		s = whole
	var pic := Vector2(desktop) * s
	picture = Rect2(((area - pic) / 2.0).floor(), pic)
	screen.position = picture.position
	screen.size = picture.size
	var integer := absf(s - roundf(s)) < 0.001
	screen.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST if integer else CanvasItem.TEXTURE_FILTER_LINEAR


func _to_desktop(pos: Vector2) -> Vector2i:
	var p := (pos - picture.position) * Vector2(desktop) / picture.size
	return Vector2i(clampi(int(p.x), 0, desktop.x - 1), clampi(int(p.y), 0, desktop.y - 1))


# ------------------------------------------------------------ main loop ----
func _process(_delta: float) -> void:
	if program == null:
		return
	program.poll()
	var c: int = program.get_cursor()
	if c != last_cursor:
		last_cursor = c
		_apply_cursor(c)
	var minimized := DisplayServer.window_get_mode() == DisplayServer.WINDOW_MODE_MINIMIZED
	if was_minimized and not minimized:
		program.restore()
	was_minimized = minimized


func _apply_cursor(c: int) -> void:
	if c < 0:
		Input.mouse_mode = Input.MOUSE_MODE_HIDDEN
		return
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	var shapes := [Control.CURSOR_ARROW, Control.CURSOR_IBEAM, Control.CURSOR_WAIT,
			Control.CURSOR_MOVE, Control.CURSOR_POINTING_HAND]
	mouse_default_cursor_shape = shapes[c] if c < shapes.size() else Control.CURSOR_ARROW


func _input(event: InputEvent) -> void:
	if program == null:
		return
	if event is InputEventKey:
		var k := event as InputEventKey
		if k.pressed and k.alt_pressed and (k.keycode == KEY_ENTER or k.keycode == KEY_KP_ENTER):
			if not k.echo:
				_toggle_fullscreen()
		else:
			program.key_event(k)
		get_viewport().set_input_as_handled()
	elif event is InputEventMouseMotion:
		var p := _to_desktop((event as InputEventMouseMotion).position)
		program.mouse_move(p.x, p.y)
		get_viewport().set_input_as_handled()
	elif event is InputEventMouseButton:
		var b := event as InputEventMouseButton
		var p := _to_desktop(b.position)
		match b.button_index:
			MOUSE_BUTTON_LEFT:
				program.mouse_button(p.x, p.y, 1, b.pressed)
			MOUSE_BUTTON_RIGHT:
				program.mouse_button(p.x, p.y, 2, b.pressed)
			MOUSE_BUTTON_MIDDLE:
				program.mouse_button(p.x, p.y, 3, b.pressed)
			MOUSE_BUTTON_WHEEL_UP:
				if b.pressed:
					program.mouse_wheel(p.x, p.y, 120)
			MOUSE_BUTTON_WHEEL_DOWN:
				if b.pressed:
					program.mouse_wheel(p.x, p.y, -120)
		get_viewport().set_input_as_handled()


func _notification(what: int) -> void:
	match what:
		NOTIFICATION_WM_CLOSE_REQUEST:
			if program == null or quitting:
				get_tree().quit()
				return
			# ask the program to close its windows, as Windows does
			quitting = true
			program.request_close()
			get_tree().create_timer(10.0).timeout.connect(func(): get_tree().quit())
		NOTIFICATION_APPLICATION_FOCUS_IN:
			if program:
				program.set_focus(true)
		NOTIFICATION_APPLICATION_FOCUS_OUT:
			if program:
				program.set_focus(false)


# ------------------------------------------------------ program events -----
func _on_log(text: String) -> void:
	print("[game] ", text)


func _on_run_requested(cmd: String) -> void:
	print("[game] asked to run: ", cmd)


func _on_minimize_requested(minimize: bool) -> void:
	if minimize:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_MINIMIZED)


func _on_warp(x: int, y: int) -> void:
	var p := picture.position + Vector2(x + 0.5, y + 0.5) * picture.size / Vector2(desktop)
	Input.warp_mouse(p)


func _on_exited(code: int) -> void:
	print("[game] program exited with code ", code)
	music_player.stop()
	if quitting or program == null:
		get_tree().quit()
		return
	# back to the launcher, in a window
	program.queue_free()
	program = null
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	get_tree().change_scene_to_file("res://launcher.tscn")


func _fatal(text: String) -> void:
	push_error(text)
	message.text = text + "\n\nClose this window to quit."
	message.visible = true


# ---------------------------------------------------------------- sound ----
func _on_sound_created(id: int, data: PackedByteArray, bits: int, rate: int, stereo: bool) -> void:
	var wav := AudioStreamWAV.new()
	wav.format = AudioStreamWAV.FORMAT_8_BITS if bits == 8 else AudioStreamWAV.FORMAT_16_BITS
	wav.mix_rate = rate
	wav.stereo = stereo
	wav.data = data
	var v: Dictionary = voices.get(id, {})
	if v.is_empty():
		var player := AudioStreamPlayer.new()
		add_child(player)
		v = {"player": player}
		voices[id] = v
	v["stream"] = wav
	v["frames"] = data.size() / (maxi(bits / 8, 1) * (2 if stereo else 1))
	v["pos"] = 0.0
	(v["player"] as AudioStreamPlayer).stream = wav


func _effects_db(volume_mb: int) -> float:
	return maxf(volume_mb / 100.0 + Settings.effects_volume_db, -80.0)


func _on_audio_command(cmd: int, id: int, loop: bool, volume_mb: int, restart: bool, path: String) -> void:
	if cmd == AUD_MUSIC_PLAY or cmd == AUD_MUSIC_STOP:
		_music_command(cmd, restart, path)
		return
	var v: Dictionary = voices.get(id, {})
	if v.is_empty():
		return
	var player: AudioStreamPlayer = v["player"]
	match cmd:
		AUD_PLAY:
			var wav: AudioStreamWAV = v["stream"]
			wav.loop_mode = AudioStreamWAV.LOOP_FORWARD if loop else AudioStreamWAV.LOOP_DISABLED
			wav.loop_begin = 0
			wav.loop_end = v["frames"]
			player.volume_db = _effects_db(volume_mb)
			player.play(0.0 if restart else float(v["pos"]))
		AUD_STOP:
			if player.playing:
				v["pos"] = player.get_playback_position()
			player.stop()
		AUD_VOLUME:
			player.volume_db = _effects_db(volume_mb)
		AUD_DESTROY:
			player.queue_free()
			voices.erase(id)


# The recordings the program plays instead of its MIDI songs.
func _set_music_environment() -> void:
	var files: Array[String] = []
	var mode := "map"
	var root := Paths.game_dir()
	match Settings.music:
		"mix", "remix":
			for i in range(1, 9):
				var f := root.path_join("music_remix").path_join("sf-music%d.ogg" % i)
				if i == 8 and Settings.song8 == "darkwave":
					f = root.path_join("music_remix_alt").path_join("sf-music8.ogg")
				files.append(f)
			if Settings.music == "mix":
				mode = "shuffle"
				for i in range(1, 11):
					files.append(root.path_join("music_new").path_join("new%02d.ogg" % i))
		"hifi":
			for i in range(1, 9):
				files.append(root.path_join("music").path_join("sf-music%d.ogg" % i))
	var present: Array[String] = []
	for f in files:
		if FileAccess.file_exists(f):
			present.append(f)
	OS.set_environment("SF_MUSIC_LIST", ";".join(present))
	OS.set_environment("SF_MUSIC_MODE", mode)


# Recorded music: the program names the .ogg file to play (see above).
func _music_command(cmd: int, restart: bool, path: String) -> void:
	if cmd == AUD_MUSIC_STOP:
		if music_player.playing:
			music_pos = music_player.get_playback_position()
		music_player.stop()
		return
	if OS.has_environment("SF_DEBUG_MUSIC"):
		print("music: ", path)
	if not path.to_lower().ends_with(".ogg"):
		return
	var file := path
	if file != music_file:
		var stream := AudioStreamOggVorbis.load_from_file(file)
		if stream == null:
			push_warning("cannot play " + path)
			return
		music_player.stream = stream
		music_file = file
		restart = true
	music_player.volume_db = Settings.music_volume_db
	music_player.play(0.0 if restart else music_pos)
