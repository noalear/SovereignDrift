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
var settings_menu: MainMenu = null   # the title screen's Settings page, opened from the pause menu
var builder: Builder = null
var trade: Trade = null
var tactical: TacticalMap = null
var ledger: Ledger = null
var cargo_panel: CargoPanel = null
var tactical_opened_at := 0
var scoreboard: Scoreboard = null
var font: Font


func _ready() -> void:
	font = ThemeDB.fallback_font
	ui_layer = CanvasLayer.new()
	ui_layer.layer = 10
	add_child(ui_layer)
	_show_menu()
	Net.session_ended.connect(_on_session_ended)
	Net.trade_opened.connect(_open_trade)
	Net.trade_updated.connect(func(d): if trade != null: trade.update(d))
	Net.trade_closed.connect(_close_trade)
	Net.status.connect(_on_net_status)
	_start_from_command_line()


## `--host` hosts a game on port 7777, `--host-duel` a free-for-all duel;
## `--join=1.2.3.4` joins either (after `--` when run from the editor
## binary).  The Play online screen replaces this.
func _start_from_command_line() -> void:
	var args := OS.get_cmdline_user_args() + OS.get_cmdline_args()
	for a in args:
		if a == "--host" or a == "--host-duel":
			Settings.use_profile("online")
			Net.callsign = Settings.pilot_name
			Net.mode = "duel" if a == "--host-duel" else ""
			if Net.host() == OK:
				_start_game(false, true, "", a == "--host-duel")
			return
		if a.begins_with("--join="):
			Settings.use_profile("online")
			Net.callsign = Settings.pilot_name
			var target: String = a.substr(7)
			var port := Net.PORT
			if ":" in target:
				port = int(target.get_slice(":", 1))
				target = target.get_slice(":", 0)
			if Net.join(target, port) == OK:
				Net.session_started.connect(_start_game, CONNECT_ONE_SHOT)
			return


func _on_net_status(text: String) -> void:
	print(text)
	if world != null:
		world.notify_all(text, Color(0.7, 0.9, 1.0))
	elif menu != null:
		menu.online_status = text


func _on_session_ended(reason: String) -> void:
	print(reason)
	if world != null:
		_close_pause()
		_save_career()
		_show_menu()
	if menu != null:
		menu.online_status = reason


## `duel`: a free-for-all instead of co-op Conquest.
func _host_game(duel := false) -> void:
	Settings.use_profile("online")
	Net.callsign = Settings.pilot_name
	Net.mode = "duel" if duel else ""
	if Net.host() == OK:
		_start_game(false, true, "", duel)
	elif menu != null:
		menu.online_status = "Could not open port %d. Is another game hosting?" % Net.PORT


func _join_game(address: String) -> void:
	Settings.use_profile("online")
	Net.callsign = Settings.pilot_name
	var target := address.strip_edges()
	var port := Net.PORT
	if ":" in target:
		port = int(target.get_slice(":", 1))
		target = target.get_slice(":", 0)
	if target.is_empty():
		menu.online_status = "Type the host's address first."
		return
	Settings.last_join_ip = address.strip_edges()
	Settings.save_settings()
	if Net.join(target, port) == OK:
		menu.online_status = "Connecting to %s..." % target
		Net.session_started.connect(_start_game, CONNECT_ONE_SHOT)


func _show_menu() -> void:
	Net.leave()
	if builder != null:
		builder.queue_free()
		builder = null
	_close_tactical()
	_close_ledger()
	_close_cargo()
	if scoreboard != null:
		scoreboard.queue_free()
		scoreboard = null
	Engine.time_scale = 1.0
	get_tree().paused = false
	_close_trade({})
	if world != null:
		world.queue_free()
		world = null
	if hud != null:
		hud.queue_free()
		hud = null
	menu_camera = Camera2D.new()
	# the menu camera drifts every frame, not on physics ticks
	menu_camera.physics_interpolation_mode = Node.PHYSICS_INTERPOLATION_MODE_OFF
	add_child(menu_camera)
	menu_camera.make_current()
	menu_stars = Starfield.new()
	menu_stars.camera = menu_camera
	add_child(menu_stars)
	menu = MainMenu.new()
	ui_layer.add_child(menu)
	menu.play_pressed.connect(_start_game)
	menu.practice_pressed.connect(_start_practice)
	menu.free_flight_pressed.connect(_start_free_flight)
	menu.campaign_new_pressed.connect(func(): _start_game(false, true, "new"))
	menu.campaign_continue_pressed.connect(func(): _start_game(false, true, "continue"))
	menu.host_pressed.connect(_host_game)
	menu.duel_pressed.connect(_host_game.bind(true))
	menu.join_pressed.connect(_join_game)
	menu.quit_pressed.connect(func(): get_tree().quit())


func _process(delta: float) -> void:
	if menu_camera != null and is_instance_valid(menu_camera):
		menu_camera.global_position += Vector2(30.0, 6.0) * delta


func _start_practice() -> void:
	_start_game(true)


func _start_free_flight() -> void:
	_start_game(true, false)


## `campaign` is "" for a Conquest round, "new" for a fresh campaign or
## "continue" for the saved one; `duel` hosts a free-for-all.
func _start_game(practice := false, opponent := true, campaign := "", duel := false) -> void:
	if not Net.is_active():
		Settings.use_profile("single")
	Settings.save_settings()
	var saved := {}
	if campaign == "continue":
		saved = SaveGame.read()
		if saved.is_empty():
			campaign = "new"
	elif campaign == "new":
		SaveGame.erase()
	Sfx.play_ui("ui_start", 0.8)
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
	world.practice = practice
	world.practice_opponent = opponent
	world.duel = duel
	world.campaign = campaign != ""
	world.campaign_data = saved
	add_child(world)
	hud = HUD.new()
	ui_layer.add_child(hud)
	hud.attach(world)
	world.shipyard_requested.connect(open_builder)
	world.round_over.connect(_on_round_over)
	if campaign == "new":
		world.notify_all("Campaign: no clock. The war goes on until one empire holds every colony. It saves every 2 minutes, with %s, and when you leave." % Keybinds.short_text("quick_save"), Color(0.8, 0.9, 1.0))
		world.save_campaign(false)
	elif campaign == "continue":
		world.notify_all("Campaign resumed: %s into the war. %s shows your empire's resources." % [Round._fmt(world.elapsed), Keybinds.short_text("ledger")], Color(0.8, 0.9, 1.0))
	elif world.round_rules != null and world.round_rules.length > 0.0:
		world.notify_all("Conquest: %d minutes. Hold the most planets when time runs out, or every colony for 45 seconds. Tab shows the system." % int(world.round_rules.length / 60.0), Color(0.8, 0.9, 1.0))
	if practice and not opponent:
		world.notify_all("Free flight: no enemies, no round clock, and the colonies hold their fire. Try long autopilot routes with N and G. Esc for the menu.", Color(0.8, 0.9, 1.0))
	elif practice:
		world.notify_all("Practice duel: one pirate Scout at a time, no round clock, no fleets, and the colonies hold their fire. Esc for the menu.", Color(0.8, 0.9, 1.0))
	if duel:
		# two short lines: a long one would run under the leaderboard
		world.notify_all("Free-for-all duel: everyone is your enemy, and everyone flies a %s." % Duel.HULL, Color(0.8, 0.9, 1.0))
		world.notify_all("First to %d kills wins the round. Computer pilots fill in until friends join." % Duel.KILLS_TO_WIN, Color(0.8, 0.9, 1.0))
	if campaign == "continue" or duel:
		return
	world.notify_all("Welcome, %s. You launch from the %s starbase at %s. Press H for the controls." % [Settings.pilot_name, GameData.race(Settings.race_id)["name"], GameData.STARBASES.get(Settings.race_id, "home")], Color(1, 0.95, 0.8))


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("fullscreen"):
		Settings.toggle_fullscreen()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("tactical_map") and world != null and builder == null and pause_panel == null and scoreboard == null:
		if tactical == null:
			_close_ledger()
			tactical = TacticalMap.new()
			tactical.world = world
			ui_layer.add_child(tactical)
			tactical_opened_at = Time.get_ticks_msec()
			Sfx.play_ui("click", 0.3)
		else:
			_close_tactical()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("ledger") and world != null and builder == null and pause_panel == null and scoreboard == null:
		if ledger == null:
			_close_tactical()
			ledger = Ledger.new()
			ledger.world = world
			ui_layer.add_child(ledger)
			Sfx.play_ui("click", 0.3)
		else:
			_close_ledger()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("cargo") and world != null and world.player != null and builder == null and pause_panel == null and scoreboard == null:
		if cargo_panel != null:
			_close_cargo()
		elif CargoPanel.can_open(world, world.player):
			cargo_panel = CargoPanel.new()
			cargo_panel.world = world
			cargo_panel.close_requested.connect(_close_cargo)
			ui_layer.add_child(cargo_panel)
			Sfx.play_ui("click", 0.3)
		else:
			var s := world.player
			world.notify(s, "Cargo hold %d / %d. Land on one of your colonies (B) or come alongside one of your stations or a trade station to load, unload or trade." % [int(s.cargo_total()), int(s.cargo_max)])
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("pause_menu") and cargo_panel != null:
		_close_cargo()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("quick_save") and world != null and world.campaign and scoreboard == null:
		world.save_campaign()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("pause_menu") and ledger != null:
		_close_ledger()
		get_viewport().set_input_as_handled()
	elif event.is_action_released("tactical_map") and tactical != null and event is InputEventKey:
		# a long press is "hold to look"; a tap leaves the map open
		if Time.get_ticks_msec() - tactical_opened_at > 350:
			_close_tactical()
	elif event.is_action_pressed("pause_menu") and tactical != null:
		_close_tactical()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("pause_menu") and world != null and builder == null and scoreboard == null:
		if pause_panel == null:
			_open_pause()
		else:
			_close_pause()
		get_viewport().set_input_as_handled()


func _open_pause() -> void:
	if not Net.is_active():
		world.set_paused(true)   # online the round goes on behind the menu
	world.menu_open = true       # either way the ship ignores the keys and clicks meant for the menu
	pause_panel = PausePanel.new()
	pause_panel.process_mode = Node.PROCESS_MODE_ALWAYS
	pause_panel.resume.connect(_close_pause)
	ui_layer.add_child(pause_panel)
	pause_panel.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	var bg := ColorRect.new()
	bg.color = Color(0, 0, 0, 0.55)
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
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
	b3.text = "Settings"
	b3.add_theme_font_size_override("font_size", 20)
	b3.pressed.connect(_open_settings.bind(b3))
	box.add_child(b3)
	if world.campaign:
		var bs := Button.new()
		bs.text = "Save the campaign"
		bs.add_theme_font_size_override("font_size", 20)
		bs.pressed.connect(func():
			bs.text = "Saved" if world.save_campaign() else "Could not save")
		box.add_child(bs)
		box.offset_top -= 30
		box.offset_bottom += 30
	var b4 := Button.new()
	b4.text = "Leave the game" if Net.is_active() else ("Save and leave to the menu" if world.campaign else "Leave to the menu")
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


## The pause menu's backdrop.  It runs while the game is paused, which Main
## does not, so Esc (or B on a gamepad) here resumes.
class PausePanel extends Control:
	signal resume

	func _unhandled_input(event: InputEvent) -> void:
		if visible and (event.is_action_pressed("pause_menu") or event.is_action_pressed("ui_cancel")):
			get_viewport().set_input_as_handled()
			resume.emit()


## Graphics, sound and controls without leaving the game: the title
## screen's own Settings page over the pause menu.
func _open_settings(from: Button) -> void:
	if settings_menu != null or pause_panel == null:
		return
	pause_panel.visible = false
	settings_menu = MainMenu.new()
	settings_menu.in_game = true
	settings_menu.world = world
	ui_layer.add_child(settings_menu)
	settings_menu.closed.connect(func():
		_close_settings()
		if pause_panel != null:
			pause_panel.visible = true
			from.grab_focus())


func _close_settings() -> void:
	if settings_menu != null:
		settings_menu.queue_free()
		settings_menu = null


func _close_tactical() -> void:
	if tactical != null:
		tactical.queue_free()
		tactical = null


func _close_cargo() -> void:
	if cargo_panel != null:
		cargo_panel.queue_free()
		cargo_panel = null


func _close_ledger() -> void:
	if ledger != null:
		ledger.queue_free()
		ledger = null


## The round is decided: let the slow-motion finish play for a moment,
## then stop the world and show the scoreboard.
func _on_round_over(result: Dictionary) -> void:
	var w := world
	await get_tree().create_timer(3.0, true, false, true).timeout
	if world == null or world != w or scoreboard != null:
		return
	_close_tactical()
	_close_pause()
	if builder != null:
		builder.queue_free()
		builder = null
	world.set_paused(true)
	Engine.time_scale = 1.0
	scoreboard = Scoreboard.new()
	scoreboard.open(result, world.player.race_id if world.player != null else Settings.race_id)
	ui_layer.add_child(scoreboard)
	scoreboard.play_again.connect(func():
		_show_menu()
		_start_game())
	scoreboard.to_menu.connect(_show_menu)


## A trade the host opened for our pilot: a window over the running round.
func _open_trade(data: Dictionary) -> void:
	if world == null or scoreboard != null:
		return
	_close_trade({})
	_close_tactical()
	trade = Trade.new()
	ui_layer.add_child(trade)
	trade.open(data)


func _close_trade(data: Dictionary) -> void:
	if trade != null:
		trade.queue_free()
		trade = null
	if world != null and data.has("reason") and world.player != null:
		world.notify(world.player, data["reason"])


func open_builder(ship: Ship, base: Ship) -> void:
	if builder != null or pause_panel != null or scoreboard != null:
		return
	_close_tactical()
	if Net.is_active():
		ship.docked = true       # online the round goes on; the ship sits at the dock
	else:
		world.set_paused(true)
	world.visible = false
	builder = Builder.new()
	ui_layer.add_child(builder)
	builder.open(ship, base, world)
	builder.closed.connect(func(launched: bool):
		builder = null
		if world != null:
			world.visible = true
			if Net.is_active():
				ship.docked = false
				if Net.is_client() and not launched:
					Net.client_command("undock")
			else:
				world.set_paused(false))


func _close_pause() -> void:
	_close_settings()
	if pause_panel != null:
		pause_panel.queue_free()
		pause_panel = null
	if world != null:
		world.set_paused(false)
		world.menu_open = false


## Leaving or quitting: the career, and in a campaign the war itself.
func _save_career() -> void:
	if world != null and world.campaign and not world.game_over:
		world.save_campaign(false)
	if world != null and world.player != null and world.records_career():
		Settings.career["score"] = world.player.score
		Settings.career["kills"] = world.player.kills
		Settings.save_settings()


func _notification(what: int) -> void:
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		_save_career()
		get_tree().quit()
