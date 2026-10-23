# launcher.gd -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
# The first screen: Play, Settings (resolution, display, music, volumes), Quit.
# The background is the right-hand part of the game's own menu art
# (art/menu_background.png, from bmp/65536/sf-menu1-1024.bmp).
extends Control

const ACCENT := Color(0.55, 0.75, 1.0)

var settings_panel: PanelContainer
var main_panel: VBoxContainer
var res_option: OptionButton
var display_option: OptionButton
var music_option: OptionButton
var song8_option: OptionButton
var music_slider: HSlider
var effects_slider: HSlider


func _ready() -> void:
	get_tree().set_auto_accept_quit(true)
	_window_for_launcher()
	_build_background()
	_build_main_panel()
	_build_settings_panel()


func _window_for_launcher() -> void:
	if DisplayServer.get_name() == "headless":
		return
	DisplayServer.window_set_mode(DisplayServer.WINDOW_MODE_WINDOWED)
	var usable := DisplayServer.screen_get_usable_rect(DisplayServer.window_get_current_screen())
	var win := Vector2i(1280, 720)
	if win.x > usable.size.x - 16 or win.y > usable.size.y - 48:
		win = Vector2i(1024, 576)
	DisplayServer.window_set_size(win)
	DisplayServer.window_set_position(usable.position + (usable.size - win) / 2)


func _build_background() -> void:
	var bg := ColorRect.new()
	bg.color = Color(0.01, 0.01, 0.03)
	bg.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(bg)
	# the right-hand part of the game's menu art (bmp/65536/sf-menu1-1024.bmp,
	# converted to PNG: Godot cannot read its RLE compression)
	var atlas: Texture2D = load("res://art/menu_background.png")
	var tex := TextureRect.new()
	tex.texture = atlas
	tex.set_anchors_preset(Control.PRESET_FULL_RECT)
	tex.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tex.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_COVERED
	tex.texture_filter = CanvasItem.TEXTURE_FILTER_LINEAR
	tex.modulate = Color(0.75, 0.75, 0.8)
	add_child(tex)
	# darken the left side under the menu
	var shade := TextureRect.new()
	var grad := Gradient.new()
	grad.set_color(0, Color(0, 0, 0, 0.92))
	grad.set_color(1, Color(0, 0, 0, 0.0))
	var gt := GradientTexture2D.new()
	gt.gradient = grad
	gt.fill_from = Vector2(0.0, 0.5)
	gt.fill_to = Vector2(0.75, 0.5)
	shade.texture = gt
	shade.set_anchors_preset(Control.PRESET_FULL_RECT)
	shade.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	shade.stretch_mode = TextureRect.STRETCH_SCALE
	add_child(shade)


func _label(text: String, size: int, color := Color.WHITE) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_font_size_override("font_size", size)
	l.add_theme_color_override("font_color", color)
	return l


func _button(text: String, cb: Callable) -> Button:
	var b := Button.new()
	b.text = text
	b.custom_minimum_size = Vector2(260, 46)
	b.add_theme_font_size_override("font_size", 20)
	b.alignment = HORIZONTAL_ALIGNMENT_LEFT
	b.pressed.connect(cb)
	return b


func _build_main_panel() -> void:
	main_panel = VBoxContainer.new()
	main_panel.position = Vector2(64, 72)
	main_panel.add_theme_constant_override("separation", 10)
	add_child(main_panel)
	main_panel.add_child(_label("STELLAR FRONTIER", 44, ACCENT))
	main_panel.add_child(_label("Enhanced Edition", 22, Color(0.85, 0.85, 0.9)))
	var gap := Control.new()
	gap.custom_minimum_size = Vector2(0, 36)
	main_panel.add_child(gap)
	var play := _button("Play", _on_play)
	main_panel.add_child(play)
	main_panel.add_child(_button("Settings", _on_settings))
	main_panel.add_child(_button("Quit", func(): get_tree().quit()))
	var res := Settings.desktop_size()
	var info := _label("Resolution %dx%d, %s" % [res.x, res.y,
			"full screen" if Settings.display == "fullscreen" else "window"], 14, Color(0.7, 0.7, 0.75))
	info.name = "Info"
	main_panel.add_child(info)
	play.grab_focus()


func _row(grid: GridContainer, title: String, control: Control) -> void:
	grid.add_child(_label(title, 18))
	control.custom_minimum_size = Vector2(280, 36)
	grid.add_child(control)


func _build_settings_panel() -> void:
	settings_panel = PanelContainer.new()
	settings_panel.position = Vector2(64, 160)
	settings_panel.visible = false
	var style := StyleBoxFlat.new()
	style.bg_color = Color(0.03, 0.04, 0.08, 0.92)
	style.border_color = ACCENT * Color(1, 1, 1, 0.6)
	style.set_border_width_all(1)
	style.set_content_margin_all(20)
	settings_panel.add_theme_stylebox_override("panel", style)
	add_child(settings_panel)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 14)
	settings_panel.add_child(box)
	box.add_child(_label("Settings", 26, ACCENT))
	var grid := GridContainer.new()
	grid.columns = 2
	grid.add_theme_constant_override("h_separation", 24)
	grid.add_theme_constant_override("v_separation", 10)
	box.add_child(grid)

	res_option = OptionButton.new()
	var auto := Settings.auto_resolution()
	res_option.add_item("Match my screen (%dx%d)" % [auto.x, auto.y])
	res_option.set_item_metadata(0, "auto")
	for r in Settings.RESOLUTIONS:
		var v := Settings.parse(r)
		res_option.add_item("%d x %d  (%s)" % [v.x, v.y, _aspect_name(v)])
		res_option.set_item_metadata(res_option.item_count - 1, r)
	_select_meta(res_option, Settings.resolution)
	_row(grid, "Resolution", res_option)

	display_option = OptionButton.new()
	display_option.add_item("Full screen")
	display_option.set_item_metadata(0, "fullscreen")
	display_option.add_item("Window")
	display_option.set_item_metadata(1, "window")
	_select_meta(display_option, Settings.display)
	_row(grid, "Display", display_option)

	music_option = OptionButton.new()
	for item in [["Remixes + new tracks (shuffle)", "mix"], ["Remixes", "remix"],
			["Original, high fidelity", "hifi"], ["Original MIDI", "midi"], ["Off", "off"]]:
		music_option.add_item(item[0])
		music_option.set_item_metadata(music_option.item_count - 1, item[1])
	_select_meta(music_option, Settings.music)
	_row(grid, "Music", music_option)

	song8_option = OptionButton.new()
	song8_option.add_item("Dubstep")
	song8_option.set_item_metadata(0, "dubstep")
	song8_option.add_item("Darkwave")
	song8_option.set_item_metadata(1, "darkwave")
	_select_meta(song8_option, Settings.song8)
	_row(grid, "Song 8 remix", song8_option)

	music_slider = HSlider.new()
	music_slider.min_value = -30
	music_slider.max_value = 6
	music_slider.value = Settings.music_volume_db
	_row(grid, "Music volume", music_slider)

	effects_slider = HSlider.new()
	effects_slider.min_value = -30
	effects_slider.max_value = 6
	effects_slider.value = Settings.effects_volume_db
	_row(grid, "Effects volume", effects_slider)

	var note := _label("Larger resolutions show more of the battle; the game's text stays the same size.",
			14, Color(0.7, 0.7, 0.75))
	note.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	note.custom_minimum_size = Vector2(440, 0)
	box.add_child(note)
	var buttons := HBoxContainer.new()
	buttons.add_theme_constant_override("separation", 12)
	box.add_child(buttons)
	var save := Button.new()
	save.text = "Save"
	save.custom_minimum_size = Vector2(120, 40)
	save.pressed.connect(_on_save)
	buttons.add_child(save)
	var cancel := Button.new()
	cancel.text = "Cancel"
	cancel.custom_minimum_size = Vector2(120, 40)
	cancel.pressed.connect(_close_settings)
	buttons.add_child(cancel)


func _aspect_name(v: Vector2i) -> String:
	var a := float(v.x) / float(v.y)
	if absf(a - 4.0 / 3.0) < 0.02:
		return "4:3"
	if absf(a - 16.0 / 10.0) < 0.02:
		return "16:10"
	if absf(a - 16.0 / 9.0) < 0.03:
		return "16:9"
	return "21:9"


func _select_meta(o: OptionButton, value: String) -> void:
	for i in o.item_count:
		if o.get_item_metadata(i) == value:
			o.select(i)
			return
	o.select(0)


func _on_play() -> void:
	get_tree().change_scene_to_file("res://game.tscn")


func _on_settings() -> void:
	main_panel.visible = false
	settings_panel.visible = true


func _close_settings() -> void:
	settings_panel.visible = false
	main_panel.visible = true


func _on_save() -> void:
	Settings.resolution = res_option.get_item_metadata(res_option.selected)
	Settings.display = display_option.get_item_metadata(display_option.selected)
	Settings.music = music_option.get_item_metadata(music_option.selected)
	Settings.song8 = song8_option.get_item_metadata(song8_option.selected)
	Settings.music_volume_db = music_slider.value
	Settings.effects_volume_db = effects_slider.value
	Settings.save_settings()
	var res := Settings.desktop_size()
	(main_panel.get_node("Info") as Label).text = "Resolution %dx%d, %s" % [res.x, res.y,
			"full screen" if Settings.display == "fullscreen" else "window"]
	_close_settings()
