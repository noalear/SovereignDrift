## Main -- switches between the title screen and the game, and owns the
## pause menu.
extends Node

var menu: MainMenu
var menu_stars: Starfield
var menu_camera: Camera2D
var world: World = null
var hud: HUD = null
var ui_layer: CanvasLayer
var pause_panel: Control = null
var font: Font


func _ready() -> void:
	font = ThemeDB.fallback_font
	ui_layer = CanvasLayer.new()
	ui_layer.layer = 10
	add_child(ui_layer)
	_show_menu()


func _show_menu() -> void:
	if world != null:
		world.queue_free()
		world = null
	if hud != null:
		hud.queue_free()
		hud = null
	menu_camera = Camera2D.new()
	add_child(menu_camera)
	menu_camera.make_current()
	menu_stars = Starfield.new()
	menu_stars.camera = menu_camera
	add_child(menu_stars)
	menu = MainMenu.new()
	ui_layer.add_child(menu)
	menu.play_pressed.connect(_start_game)
	menu.quit_pressed.connect(func(): get_tree().quit())


func _process(delta: float) -> void:
	if menu_camera != null and is_instance_valid(menu_camera):
		menu_camera.global_position += Vector2(30.0, 6.0) * delta


func _start_game() -> void:
	Settings.save_settings()
	if menu != null:
		menu.queue_free()
		menu = null
	if menu_stars != null:
		menu_stars.queue_free()
		menu_stars = null
	if menu_camera != null:
		menu_camera.queue_free()
		menu_camera = null
	world = World.new()
	world.name = "World"
	add_child(world)
	hud = HUD.new()
	ui_layer.add_child(hud)
	hud.attach(world)
	world.notify_all("Welcome, %s. You launch from %s. Press H for the controls." % [Settings.pilot_name, GameData.STARBASES.get(Settings.race_id, "home")], Color(1, 0.95, 0.8))


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("fullscreen"):
		Settings.toggle_fullscreen()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("pause_menu") and world != null:
		if pause_panel == null:
			_open_pause()
		else:
			_close_pause()
		get_viewport().set_input_as_handled()


func _open_pause() -> void:
	world.set_paused(true)
	pause_panel = Control.new()
	pause_panel.set_anchors_preset(Control.PRESET_FULL_RECT)
	pause_panel.process_mode = Node.PROCESS_MODE_ALWAYS
	ui_layer.add_child(pause_panel)
	var bg := ColorRect.new()
	bg.color = Color(0, 0, 0, 0.55)
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	pause_panel.add_child(bg)
	var box := VBoxContainer.new()
	box.set_anchors_preset(Control.PRESET_CENTER)
	box.anchor_left = 0.5
	box.anchor_right = 0.5
	box.anchor_top = 0.5
	box.anchor_bottom = 0.5
	box.offset_left = -160
	box.offset_right = 160
	box.offset_top = -140
	box.offset_bottom = 140
	box.add_theme_constant_override("separation", 12)
	pause_panel.add_child(box)
	var title := Label.new()
	title.text = "Paused"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 32)
	box.add_child(title)
	var b1 := Button.new()
	b1.text = "Resume"
	b1.add_theme_font_size_override("font_size", 20)
	b1.pressed.connect(_close_pause)
	box.add_child(b1)
	var b2 := Button.new()
	b2.text = "Next song"
	b2.add_theme_font_size_override("font_size", 20)
	b2.pressed.connect(func(): Music.next_track())
	box.add_child(b2)
	var b3 := Button.new()
	b3.text = "Switch to window" if Settings.fullscreen else "Switch to full screen"
	b3.add_theme_font_size_override("font_size", 20)
	b3.pressed.connect(func():
		Settings.toggle_fullscreen()
		b3.text = "Switch to window" if Settings.fullscreen else "Switch to full screen")
	box.add_child(b3)
	var b4 := Button.new()
	b4.text = "Leave to the menu"
	b4.add_theme_font_size_override("font_size", 20)
	b4.pressed.connect(func():
		_close_pause()
		_save_career()
		_show_menu())
	box.add_child(b4)
	var b5 := Button.new()
	b5.text = "Quit the game"
	b5.add_theme_font_size_override("font_size", 20)
	b5.pressed.connect(func():
		_save_career()
		get_tree().quit())
	box.add_child(b5)
	b1.grab_focus()


func _close_pause() -> void:
	if pause_panel != null:
		pause_panel.queue_free()
		pause_panel = null
	if world != null:
		world.set_paused(false)


func _save_career() -> void:
	if world != null and world.player != null:
		Settings.career["score"] = world.player.score
		Settings.career["kills"] = world.player.kills
		Settings.save_settings()


func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_save_career()
		get_tree().quit()
