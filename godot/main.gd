# main.gd -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
#
# Runs the original Stellar Frontier program (frontier.exe, a 32-bit process
# in the game/ folder) through the SFProgram node of the sfport extension,
# and connects it to Godot:
#   - shows the program's 1024x768 desktop, scaled to the window (whole-
#     number scales are used when they fill most of the window),
#   - passes mouse and keyboard input through (Alt+Enter: full screen),
#   - plays the program's sounds (DirectSound buffers) with AudioStreamWAV,
#     while MIDI music plays through the Windows synthesizer,
#   - follows the program's mouse pointer shape, pointer warps, minimise
#     requests, and quits when the program quits.
extends Control

const DESKTOP_W := 1024
const DESKTOP_H := 768

# SF_AUD_* in src/host/sf_host.h
const AUD_PLAY := 3
const AUD_STOP := 4
const AUD_VOLUME := 5
const AUD_DESTROY := 2

var program: Node = null
var voices := {}            # sound id -> {player, stream, frames, pos}
var picture := Rect2(0, 0, DESKTOP_W, DESKTOP_H)
var last_cursor := -2
var was_minimized := false
var quitting := false

@onready var screen: TextureRect = $Screen
@onready var message: Label = $Message


func _ready() -> void:
	get_tree().set_auto_accept_quit(false)
	get_viewport().size_changed.connect(_layout)
	if not ClassDB.class_exists("SFProgram"):
		_fatal("The sfport extension (addons/sfport) could not be loaded.")
		return
	var exe := _game_exe("frontier")
	if not FileAccess.file_exists(exe):
		_fatal("The game files were not found:\n" + exe)
		return
	program = ClassDB.instantiate("SFProgram")
	program.name = "Program"
	add_child(program)
	program.log_line.connect(_on_log)
	program.open_url.connect(func(url): OS.shell_open(url))
	program.run_requested.connect(_on_run_requested)
	program.minimize_requested.connect(_on_minimize_requested)
	program.warp_requested.connect(_on_warp)
	program.exited.connect(_on_exited)
	program.sound_created.connect(_on_sound_created)
	program.audio_command.connect(_on_audio_command)
	if not program.start(exe, _program_args(), DESKTOP_W, DESKTOP_H):
		_fatal("The game could not be started:\n" + str(program.get_error()))
		return
	screen.texture = program.get_texture()
	_size_window()
	_layout()


# ------------------------------------------------------------ locations ----
func _game_dir() -> String:
	var env := OS.get_environment("SF_GAME_DIR")
	if env != "":
		return env
	return OS.get_executable_path().get_base_dir().path_join("game")


func _game_exe(base: String) -> String:
	var ext := ".exe" if OS.get_name() == "Windows" else ""
	return _game_dir().path_join(base + ext)


# Command-line arguments after "--" go to the original program (e.g. -log).
func _program_args() -> PackedStringArray:
	var out := PackedStringArray()
	for a in OS.get_cmdline_user_args():
		out.append(a)
	return out


# --------------------------------------------------------------- window ----
func _size_window() -> void:
	if DisplayServer.get_name() == "headless":
		return
	var screen_id := DisplayServer.window_get_current_screen()
	var usable := DisplayServer.screen_get_usable_rect(screen_id)
	# leave room for the title bar and borders
	var room := Vector2i(usable.size.x - 16, usable.size.y - 48)
	var scale := 1
	for s in range(2, 5):
		if DESKTOP_W * s <= room.x and DESKTOP_H * s <= room.y:
			scale = s
	if DESKTOP_W > room.x or DESKTOP_H > room.y:
		DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_FULLSCREEN)
		return
	var size := Vector2i(DESKTOP_W * scale, DESKTOP_H * scale)
	DisplayServer.window_set_min_size(Vector2i(640, 480))
	DisplayServer.window_set_size(size)
	DisplayServer.window_set_position(usable.position + (usable.size - size) / 2)


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
	var s: float = min(area.x / DESKTOP_W, area.y / DESKTOP_H)
	var whole := floorf(s)
	if whole >= 1.0 and whole / s > 0.9:
		s = whole
	var size := Vector2(DESKTOP_W, DESKTOP_H) * s
	picture = Rect2(((area - size) / 2.0).floor(), size)
	screen.position = picture.position
	screen.size = picture.size
	var integer := absf(s - roundf(s)) < 0.001
	screen.texture_filter = CanvasItem.TEXTURE_FILTER_NEAREST if integer else CanvasItem.TEXTURE_FILTER_LINEAR


func _to_desktop(pos: Vector2) -> Vector2i:
	var p := (pos - picture.position) * Vector2(DESKTOP_W, DESKTOP_H) / picture.size
	return Vector2i(clampi(int(p.x), 0, DESKTOP_W - 1), clampi(int(p.y), 0, DESKTOP_H - 1))


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
	var p := picture.position + Vector2(x + 0.5, y + 0.5) * picture.size / Vector2(DESKTOP_W, DESKTOP_H)
	Input.warp_mouse(p)


func _on_exited(code: int) -> void:
	print("[game] program exited with code ", code)
	get_tree().quit()


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
	v["frames"] = data.size() / ((bits / 8) * (2 if stereo else 1))
	v["pos"] = 0.0
	(v["player"] as AudioStreamPlayer).stream = wav


func _on_audio_command(cmd: int, id: int, loop: bool, volume_mb: int, restart: bool, _path: String) -> void:
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
			player.volume_db = maxf(volume_mb / 100.0, -80.0)
			player.play(0.0 if restart else float(v["pos"]))
		AUD_STOP:
			if player.playing:
				v["pos"] = player.get_playback_position()
			player.stop()
		AUD_VOLUME:
			player.volume_db = maxf(volume_mb / 100.0, -80.0)
		AUD_DESTROY:
			player.queue_free()
			voices.erase(id)
