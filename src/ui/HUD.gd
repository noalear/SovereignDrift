## HUD -- the cockpit overlay: ship status, target readout, radar, message
## log, rank, music notice, the wing and its orders wheel, and the help card.
class_name HUD
extends Control

var world: World
var font: Font
var messages: Array = []       # [text, color, time]
var now_playing := ""
var now_playing_time := -100.0
var radar_range := 3000.0
var show_help := false
var time := 0.0
var dead_time := 0.0
var wheel: OrdersWheel = null
var radio: AudioStreamPlayer = null   # wingmen's voices
var planet_pic: PlanetPicture = null  # the inspected world's picture on its info card

const PIC_H := 130.0                  # the picture under the target readout
const CARD_PIC_H := 140.0             # the picture at the foot of a world's info card


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	Music.track_changed.connect(_on_track)
	if not Music.now_playing().is_empty():
		now_playing = Music.now_playing()
		now_playing_time = 0.0


func attach(p_world: World) -> void:
	world = p_world
	world.message.connect(_on_message)
	world.player_died.connect(func(): dead_time = 0.0)
	world.radio_heard.connect(_on_radio)
	wheel = OrdersWheel.new()
	wheel.world = world
	add_child(wheel)
	planet_pic = PlanetPicture.new()
	planet_pic.visible = false
	add_child(planet_pic)
	radio = AudioStreamPlayer.new()
	radio.bus = "Voice" if AudioServer.get_bus_index("Voice") >= 0 else "Master"
	add_child(radio)


## A wingman's voice, unless Commander Hale is talking (the words are in
## the log either way).
func _on_radio(path: String) -> void:
	var tut: Node = get_parent().get_parent().get("tutorial") if get_parent() != null and get_parent().get_parent() != null else null
	if tut != null and is_instance_valid(tut) and tut.speaking():
		return
	if not ResourceLoader.exists(path) or (radio.playing and radio.get_playback_position() < 0.6):
		return
	radio.stream = load(path)
	radio.play()
	Music.duck_for(0.3)


func _on_track(title: String, style: String) -> void:
	now_playing = "%s  (%s)" % [title, style]
	now_playing_time = time


func _on_message(text: String, color: Color) -> void:
	messages.append([text, color, time])
	if messages.size() > 7:
		messages.pop_front()


func _unhandled_input(event: InputEvent) -> void:
	if world != null and world.mode == "skirmish" and world.mode_rules != null and world.player != null and world.player.dead and not world.menu_open:
		# waiting to launch: 1 to 5, left and right, or a click pick the ship
		var hulls: Array = Skirmish.HULLS
		var pick: String = world.mode_rules.picks.get(world.player, world.player.design_name)
		var i := hulls.find(pick)
		if event is InputEventKey and event.pressed and not event.echo and event.keycode >= KEY_1 and event.keycode < KEY_1 + hulls.size():
			_pick_hull(hulls[event.keycode - KEY_1])
			get_viewport().set_input_as_handled()
			return
		if event.is_action_pressed("turn_left") or event.is_action_pressed("turn_right"):
			_pick_hull(hulls[posmod(i + (1 if event.is_action_pressed("turn_right") else -1), hulls.size())])
			get_viewport().set_input_as_handled()
			return
		if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
			for c in _hull_cards(get_viewport_rect().size):
				if c[1].has_point(event.position):
					_pick_hull(c[0])
					get_viewport().set_input_as_handled()
					return
	if event.is_action_pressed("help"):
		show_help = not show_help
	elif event.is_action_pressed("radar_zoom_in"):
		radar_range = maxf(radar_range * 0.7, 800.0)
	elif event.is_action_pressed("radar_zoom_out"):
		radar_range = minf(radar_range * 1.4, 30000.0)
	elif event.is_action_pressed("music_next"):
		Music.next_track()
	elif event is InputEventMouseButton and event.pressed and world != null:
		if event.button_index == MOUSE_BUTTON_WHEEL_UP:
			world.zoom_by(1.1)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			world.zoom_by(0.9)


func _process(delta: float) -> void:
	time += delta
	if world != null and world.player != null and world.player.dead:
		dead_time += delta
	queue_redraw()


func _draw() -> void:
	if planet_pic != null:
		planet_pic.visible = false      # _draw_planet_card shows it again
	if world == null or world.player == null:
		return
	# the shipyard, tactical map and scoreboard cover the game; the HUD
	# would only show through them as clutter
	for c in get_parent().get_children():
		if c is Builder or c is TacticalMap or c is Scoreboard:
			return
	var p := world.player
	var vp := get_viewport_rect().size
	_draw_labels(p, vp)
	_draw_status(p, vp)
	_draw_weapons(p, vp)
	_draw_target(p, vp)
	_draw_radar(p, vp)
	_draw_messages(vp)
	_draw_rank(p, vp)
	_draw_wing(p, vp)
	_draw_nav(p, vp)
	_draw_planet_card(p, vp)
	_draw_music(vp)
	_draw_aim(p, vp)
	_draw_pointers(p, vp)
	_draw_heat(p, vp)
	if not p.dead:
		_draw_disabled(p, vp)
	if p.dead:
		_draw_dead(vp)
	if show_help:
		planet_pic.visible = false
		_draw_help(vp)
	else:
		draw_string(font, Vector2(vp.x * 0.5 - 150, vp.y - 14), "H: help   Esc: menu", HORIZONTAL_ALIGNMENT_CENTER, 300, 14, Color(1, 1, 1, 0.45))


## Names in the world: every planet and moon under its body, and every
## ship you can see above it, green for your side and red for anyone else.
## Worlds, moons and stations are coloured by who holds them
## (World.owner_color): green yours, yellow friendly, red enemy, grey
## nobody's.
## Planets are placed first; a label that would land on one already
## placed is skipped, so crowded moons and fleets don't pile up.
func _draw_labels(p: Ship, vp: Vector2) -> void:
	var cam := world.camera
	var centre := world.camera_centre()
	var screen_rect := Rect2(Vector2(-80, -40), vp + Vector2(160, 80))
	var placed: Array[Rect2] = []
	var bodies := world.planets.duplicate()
	bodies.sort_custom(func(a, b): return a.parent_body == null and b.parent_body != null)
	for pl in bodies:
		var at: Vector2 = (world.drawn_position(pl) - centre) * cam.zoom + vp * 0.5
		var r: float = pl.drawn_radius() * cam.zoom.x
		if not screen_rect.grow(r).has_point(at):
			continue
		var moon: bool = pl.parent_body != null
		var size := 12 if moon else 14
		var col := world.planet_label_color(p, pl)
		col.a = 0.85 if moon else 1.0
		var pos := at + Vector2(0, r + 4.0 + size)
		var w := font.get_string_size(pl.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x
		var box := Rect2(pos + Vector2(-w * 0.5, -size), Vector2(w, size + 4))
		if _crowded(box, placed):
			continue
		placed.append(box)
		_text(pos - Vector2(w * 0.5, 0), pl.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, size, col)
	var my_wing: Array = world.wing.members(p) if world.wing != null else []
	for sh in world.ships:
		if sh == p or sh.dead or sh.docked or not sh.visible or not sh.visible_to(p):
			continue
		var at: Vector2 = (world.drawn_position(sh) - centre) * cam.zoom + vp * 0.5
		if not screen_rect.has_point(at):
			continue
		var reach: float = sh.size
		if sh.shield_bubble != null:
			reach = maxf(reach, maxf(sh.shield_bubble.half.x, sh.shield_bubble.half.y) + sh.shield_bubble.position.length())
		var friend: bool = sh.team() == p.team()
		var col := Color(0.45, 1.0, 0.5) if friend else (Color(1.0, 0.38, 0.32) if world.shown_hostile(p, sh) else World.PEACE_COLOR)
		if sh.is_station:
			col = world.owner_color(p, sh.team())
		elif sh.race_id == GameData.NEUTRAL:
			col = sh.race_color()
		var text: String = station_label(sh) if sh.is_station else sh.pilot_name
		if friend and my_wing.has(sh):
			col = WING_COLOR
			text = "%s  [W%d]" % [sh.pilot_name, my_wing.find(sh) + 1]
		var pos := at - Vector2(0, reach * cam.zoom.x + 8.0)
		var w := font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x
		var box := Rect2(pos + Vector2(-w * 0.5, -13), Vector2(w, 17))
		if _crowded(box, placed):
			continue
		placed.append(box)
		_text(pos - Vector2(w * 0.5, 0), text, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, col)


const WING_COLOR := Color(0.55, 0.95, 1.0)


## The wing, over the radar: its order, the quick keys, and each wingman
## with its hull and what it is doing.
func _draw_wing(p: Ship, vp: Vector2) -> void:
	if world.wing == null or world.duel_rules != null:
		return
	var wing: Array = world.wing.members(p)
	if wing.is_empty():
		return
	var o: Dictionary = world.wing.order_of(p)
	var order: String = o.get("order", Wing.DEFAULT)
	var w := 330.0
	var x := vp.x - w - 20.0
	var rows := wing.size()
	var h := 48.0 + rows * 20.0
	var y := vp.y - 340.0 - h        # clear of the radar's lock warnings
	draw_rect(Rect2(x - 10, y, w + 20, h), Color(0.02, 0.04, 0.08, 0.6))
	var head := "WING  ·  %s" % Wing.LABELS.get(order, order)
	var t: Variant = o.get("target", null)     # may be freed: a target removed from the world
	if order == "attack" and t != null and is_instance_valid(t):
		head = "WING  ·  Attacking " + (t.pilot_name if t is Ship else t.body_name)
	draw_string(font, Vector2(x, y + 18), head, HORIZONTAL_ALIGNMENT_LEFT, w - 70, 15, WING_COLOR)
	draw_string(font, Vector2(x, y + 33), "HULL", HORIZONTAL_ALIGNMENT_LEFT, 40, 9, Color(0.6, 0.7, 0.8, 0.7))
	draw_string(font, Vector2(x, y + 18), "%s orders" % Keybinds.short_text("wing_menu"), HORIZONTAL_ALIGNMENT_RIGHT, w, 12, Color(0.7, 0.8, 0.95, 0.8))
	var ly := y + 46.0
	for i in wing.size():
		var s: Ship = wing[i]
		# the bar and the number are the wingman's hull; a battered one
		# still shows a red sliver, so an empty bar only ever means lost
		var frac: float = 0.0 if s.dead else clampf(s.hull / maxf(s.hull_max, 1.0), 0.0, 1.0)
		var hc := Color(0.4, 0.9, 0.5) if frac > 0.5 else (Color(1.0, 0.75, 0.3) if frac > 0.25 else Color(1.0, 0.3, 0.25))
		draw_rect(Rect2(x, ly - 10, 40, 6), Color(0, 0, 0, 0.5))
		if not s.dead:
			draw_rect(Rect2(x, ly - 10, maxf(40 * frac, 3.0), 6), hc)
		draw_string(font, Vector2(x + 44, ly - 2), "--" if s.dead else "%d%%" % ceili(frac * 100.0), HORIZONTAL_ALIGNMENT_LEFT, 34, 11,
			Color(0.55, 0.58, 0.65) if s.dead else hc)
		var kind := "prize" if s.is_prize and not s.has_meta("ordered") else ("escort" if s.has_meta("ordered") else s.design_name)
		var col := Color(0.92, 0.96, 1.0) if not s.dead else Color(0.55, 0.58, 0.65)
		draw_string(font, Vector2(x + 80, ly - 2), "W%d %s  %s" % [i + 1, s.pilot_name, kind], HORIZONTAL_ALIGNMENT_LEFT, 150, 13, col)
		draw_string(font, Vector2(x + 80, ly - 2), world.wing.status(s), HORIZONTAL_ALIGNMENT_RIGHT, w - 80, 12, Color(0.7, 0.85, 0.95, 0.9))
		ly += 20.0


## A station's name; a frame says how far along it is.
static func station_label(sh: Ship) -> String:
	if not sh.building or not GameData.STATIONS.has(sh.design_name):
		return sh.ship_name
	if sh.cargo_total() < sh.cargo_max - 0.01:
		return "%s frame: materials %d / %d" % [sh.ship_name, int(sh.cargo_total()), int(sh.cargo_max)]
	return "%s frame: building, %d s" % [sh.ship_name, int(ceil(float(GameData.STATIONS[sh.design_name]["build"]) - sh.build_t))]


func _crowded(box: Rect2, placed: Array[Rect2]) -> bool:
	for b in placed:
		if b.intersects(box):
			return true
	return false


func _bar(pos: Vector2, w: float, h: float, frac: float, color: Color, label: String, value: String) -> void:
	draw_rect(Rect2(pos, Vector2(w, h)), Color(0, 0, 0, 0.45))
	draw_rect(Rect2(pos, Vector2(w * clampf(frac, 0.0, 1.0), h)), color)
	draw_rect(Rect2(pos, Vector2(w, h)), Color(1, 1, 1, 0.35), false, 1.0)
	draw_string_outline(font, pos + Vector2(6, h - 5), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, 4, Color(0, 0, 0, 0.85))
	draw_string(font, pos + Vector2(6, h - 5), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1, 1, 1, 0.95))
	draw_string_outline(font, pos + Vector2(0, h - 5), value, HORIZONTAL_ALIGNMENT_RIGHT, w - 6, 14, 4, Color(0, 0, 0, 0.85))
	draw_string(font, pos + Vector2(0, h - 5), value, HORIZONTAL_ALIGNMENT_RIGHT, w - 6, 14, Color(1, 1, 1, 0.95))


## The shield as its two halves side by side, fore on the left, each as
## wide as its share of the shield.
func _shield_bar(pos: Vector2, w: float, h: float, s: Ship, full: bool) -> void:
	var col := Color(0.3, 0.6, 1.0, 0.85)
	draw_rect(Rect2(pos, Vector2(w, h)), Color(0, 0, 0, 0.45))
	var label := "Shields"
	var value := "%d" % int(s.shield)
	if s.shield_max <= 0.0:
		value = "none"
	elif not s.shields_raised:
		label = "Shields lowered"
		value = "%s to raise" % Keybinds.short_text("shields_toggle") if full else "down"
	elif s.shield_boot > 0.0:
		label = "Shields raising"
		value = "%.1f s" % s.shield_boot
	else:
		var wf := w * s.shield_share
		draw_rect(Rect2(pos, Vector2(wf * clampf(s.shield_front / maxf(s.shield_cap(true), 1.0), 0.0, 1.0), h)), col)
		draw_rect(Rect2(pos + Vector2(wf, 0), Vector2((w - wf) * clampf(s.shield_rear / maxf(s.shield_cap(false), 1.0), 0.0, 1.0), h)), col.darkened(0.15))
		draw_line(pos + Vector2(wf, -2), pos + Vector2(wf, h + 2), Color(1, 1, 1, 0.8), 2.0)
		if full:
			label = "Shields  %s" % GameData.SHIELD_FOCUS_NAMES[s.shield_focus]
			value = "fore %d  aft %d" % [int(s.shield_front), int(s.shield_rear)]
	draw_rect(Rect2(pos, Vector2(w, h)), Color(1, 1, 1, 0.35), false, 1.0)
	draw_string_outline(font, pos + Vector2(6, h - 5), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, 4, Color(0, 0, 0, 0.85))
	draw_string(font, pos + Vector2(6, h - 5), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1, 1, 1, 0.95))
	draw_string_outline(font, pos + Vector2(0, h - 5), value, HORIZONTAL_ALIGNMENT_RIGHT, w - 6, 14, 4, Color(0, 0, 0, 0.85))
	draw_string(font, pos + Vector2(0, h - 5), value, HORIZONTAL_ALIGNMENT_RIGHT, w - 6, 14, Color(1, 1, 1, 0.95))


## After the fuel gauge: refuelling, or how fast the engines and the
## booster are drinking the tank (a share of it per second).
func _fuel_note(p: Ship) -> String:
	if p.refuelling:
		return "  refuelling"
	var rate := p.fuel_use / maxf(p.fuel_max, 1.0) * 100.0
	if rate >= 0.05:
		return "  -%.1f%%/s%s" % [rate, " boost" if p.boosting else ""]
	return ""


func _draw_status(p: Ship, vp: Vector2) -> void:
	var x := 24.0
	var y := vp.y - 144.0
	var w := 300.0
	draw_rect(Rect2(x - 10, y - 34, w + 20, 148), Color(0.02, 0.04, 0.08, 0.6))
	draw_string(font, Vector2(x, y - 12), "%s   %s" % [p.ship_name, p.design_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, p.race_color())
	if not p.systems.list.is_empty():
		draw_rect(Rect2(x - 10, y - 66, w + 20, 30), Color(0.02, 0.04, 0.08, 0.6))
		_draw_systems(p, Vector2(x, y - 60), w)
	_shield_bar(Vector2(x, y), w, 19, p, true)
	_bar(Vector2(x, y + 22), w, 19, p.hull / maxf(p.hull_max, 1.0), Color(0.95, 0.55, 0.25, 0.85) if p.hull / p.hull_max > 0.3 else Color(1, 0.2, 0.2, 0.9), "Hull", "%d / %d" % [int(p.hull), int(p.hull_max)])
	_bar(Vector2(x, y + 44), w, 19, p.energy / maxf(p.energy_max, 1.0), Color(0.95, 0.85, 0.3, 0.85), "Energy", "%d" % int(p.energy))
	var fuel_col := Color(0.4, 0.9, 0.6, 0.85) if p.fuel_fraction() > 0.2 else Color(1, 0.35, 0.2, 0.9)
	_bar(Vector2(x, y + 66), w, 19, p.fuel_fraction(), fuel_col, "Fuel", "%d%%%s" % [int(p.fuel_fraction() * 100.0), _fuel_note(p)])
	var flags := []
	if p.fuel <= 0.0:
		flags.append("NO FUEL")
	if not p.flight_assist:
		flags.append("ASSIST OFF")
	if p.orbit_guard:
		flags.append("ORBIT GUARD")
	if p.sensors.jam_active():
		flags.append("JAMMING %s" % p.sensors.jam_target.pilot_name.to_upper())
	elif p.jamming:
		flags.append("JAMMER ON")
	if p.sensors.emcon:
		flags.append("SILENT RUNNING")
	if p.warping:
		flags.append("QUANTUM DRIVE")
	if p.autopilot:
		flags.append("AUTOPILOT")
	if p.is_disabled():
		flags.append("DISABLED")
	if p.engines_out():
		flags.append("ADRIFT")
	elif p.systems.efficiency("engines") < 1.0:
		flags.append("ENGINES %d%%" % int(round(p.systems.efficiency("engines") * 100.0)))
	if p.boarding != null and is_instance_valid(p.boarding):
		flags.append("BOARDING %d%%" % int(p.board_progress * 100.0))
	if p.boarded_by != null and is_instance_valid(p.boarded_by):
		flags.append("BOARDERS!")
	if p.boosting:
		flags.append("BOOST")
	if p.ablative_max > 0.0:
		flags.append("ABLATIVE %d%%" % int(100.0 * p.ablative / p.ablative_max))
	if p.reactive_max > 0:
		flags.append("REACTIVE %d/%d" % [p.reactive_tiles, p.reactive_max])
	draw_string(font, Vector2(x, y + 106), "Speed %4d   Crew %d/%d   %s" % [int(p.local_velocity().length()), int(ceil(p.crew)), p.crew_max, "  ".join(flags)], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.9, 0.95, 1.0))


## One pill per system: green whole, amber damaged, red offline, with a
## fill showing its health.
func _draw_systems(s: Ship, pos: Vector2, width: float) -> void:
	var n := s.systems.list.size()
	if n == 0:
		return
	var gap := 3.0
	var w := minf((width - gap * (n - 1)) / n, 46.0)
	var x := pos.x
	for e in s.systems.list:
		var h: float = e["hp"] / e["max"]
		var col := Color(0.35, 0.9, 0.45)
		if e["offline"]:
			col = Color(1.0, 0.25, 0.2) if fmod(time * 2.0, 1.0) < 0.65 else Color(0.5, 0.1, 0.1)
		elif h < ShipSystems.DAMAGED_BELOW:
			col = Color(1.0, 0.75, 0.25)
		var r := Rect2(Vector2(x, pos.y), Vector2(w, 18))
		draw_rect(r, Color(0, 0, 0, 0.5))
		draw_rect(Rect2(r.position, Vector2(w * clampf(h, 0.0, 1.0), 18)), Color(col.r, col.g, col.b, 0.45))
		draw_rect(r, col, false, 1.0)
		draw_string(font, r.position + Vector2(0, 13), e["tag"], HORIZONTAL_ALIGNMENT_CENTER, w, 11, Color(1, 1, 1, 0.95))
		x += w + gap


## The cockpit's warnings: dead in space, boarders aboard, our own boarding.
func _draw_disabled(p: Ship, vp: Vector2) -> void:
	var y := vp.y * 0.22
	if p.boarded_by != null and is_instance_valid(p.boarded_by):
		var b: Ship = p.boarded_by
		_text(Vector2(0, y), "BOARDERS FROM %s" % b.ship_name.to_upper(), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 26, Color(1.0, 0.35, 0.25))
		_text(Vector2(0, y + 28), "Our crew %d against their %d.  Shoot them off, or get the engines back and run." % [int(ceil(p.crew)), int(ceil(b.crew))], HORIZONTAL_ALIGNMENT_CENTER, vp.x, 16, Color(1, 0.9, 0.85))
		return
	if p.is_disabled():
		var eta := p.systems.restart_eta(p.crew_fraction())
		var what := "No crew left aboard" if p.crew < 0.5 else ("%s offline" % eta.get("label", "Engines"))
		_text(Vector2(0, y), "DEAD IN SPACE: %s" % what.to_upper(), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 26, Color(1.0, 0.45, 0.25))
		if not eta.is_empty():
			_text(Vector2(0, y + 28), "Damage control: back online in about %d s" % int(ceil(eta["eta"])), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 16, Color(1, 0.9, 0.8))
		if p.engines_out():
			_text(Vector2(0, y + 48), "No thrust, no brakes, no turning: we drift and spin as we were going.", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 16, Color(1, 0.9, 0.8))


func _draw_weapons(p: Ship, vp: Vector2) -> void:
	var x := 350.0
	var y := vp.y - 130.0
	var pd: PointDefence = p.pdc
	draw_rect(Rect2(x - 10, y - 34, 300, 134 + (18 if pd.fitted() else 0)), Color(0.02, 0.04, 0.08, 0.6))
	draw_string(font, Vector2(x, y - 12), "Weapons", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(1, 1, 1, 0.9))
	draw_string(font, Vector2(x + 80, y - 12), "%s fires  ·  %s alt (group %d)" % [Keybinds.short_text("fire_primary"), Keybinds.short_text("fire_secondary"), Settings.alt_fire_group],
		HORIZONTAL_ALIGNMENT_LEFT, 205, 11, Color(0.7, 0.8, 0.95, 0.8))
	var ly := y + 10
	for g in range(1, GameData.WEAPON_GROUPS + 1):
		var names := p.group_weapons(g)
		var ready := false
		var out := 0
		var count := 0
		for w in p.weapons:
			if int(w["group"]) != g:
				continue
			count += 1
			# a mount that is shot out does not fire
			if p.systems.is_offline("w%d" % w["index"]):
				out += 1
			elif w["cooldown"] <= 0.0 and p.energy >= w["cost"] and w["ammo"] != 0:
				ready = true
		var selected := g == p.weapon_group
		if selected:
			draw_rect(Rect2(x - 6, ly - 13, 288, 17), Color(0.25, 0.45, 0.8, 0.45))
		var text := "%d  %s" % [g, ", ".join(names) if not names.is_empty() else "-"]
		if out > 0:
			text += "  OFFLINE" if out == count else "  (%d out)" % out
		var col := Color(0.9, 1.0, 0.9) if ready else Color(0.6, 0.6, 0.6)
		if names.is_empty():
			col = Color(0.45, 0.5, 0.55)
		draw_string(font, Vector2(x, ly), text, HORIZONTAL_ALIGNMENT_LEFT, 250, 14, col)
		if g == Settings.alt_fire_group:
			draw_string(font, Vector2(x + 250, ly), "ALT", HORIZONTAL_ALIGNMENT_RIGHT, 30, 11, Color(1.0, 0.85, 0.5, 0.9))
		ly += 18
	var sys := []
	if p.can_jam:
		sys.append("%s jammer" % Keybinds.short_text("cloak"))
	if p.sensors.has("pulse"):
		sys.append("%s ping%s" % [Keybinds.short_text("ping"), (" (%d)" % ceili(p.sensors.ping_ready)) if p.sensors.ping_ready > 0.0 else ""])
	sys.append("%s silent" % Keybinds.short_text("silent_running"))
	if p.qdrv > 0.0:
		sys.append("%s quantum drive" % Keybinds.short_text("warp"))
	if p.has_boost:
		sys.append("%s boost" % Keybinds.short_text("boost"))
	draw_string(font, Vector2(x, y + 94), "  ".join(sys), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.75, 0.85, 1.0, 0.8))
	if pd.fitted():
		_draw_pdc(p, Vector2(x, y + 112))


## The point defence line: on or off, firing, rounds left, kills.
func _draw_pdc(p: Ship, at: Vector2) -> void:
	var pd: PointDefence = p.pdc
	var state := "ON"
	var col := Color(0.6, 1.0, 0.7)
	if not pd.enabled:
		state = "OFF"
		col = Color(0.6, 0.6, 0.6)
	elif p.systems.is_offline("pdc"):
		state = "OFFLINE"
		col = Color(1.0, 0.45, 0.3)
	elif pd.ammo == 0:
		state = "NO ROUNDS"
		col = Color(1.0, 0.45, 0.3)
	elif pd.firing < 0.3:
		state = "FIRING"
		col = Color(1.0, 0.9, 0.45)
	var n := pd.mounts.size()
	var text := "%s  PDC%s %s" % [Keybinds.short_text("pdc_toggle"), (" x%d" % n) if n > 1 else "", state]
	if pd.ammo_max > 0:
		text += "   %d rds" % pd.ammo
	if pd.shot_down > 0:
		text += "   %d shot down" % pd.shot_down
	draw_string(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, 285, 13, col)


func _draw_target(p: Ship, vp: Vector2) -> void:
	var t := p.target
	if t == null or not is_instance_valid(t) or (t is Ship and t.dead):
		return
	# bracket in world space -> screen
	var cam := world.camera
	var screen := (world.drawn_position(t) - world.camera_centre()) * cam.zoom + vp * 0.5
	var r: float = (t.size if t is Ship else t.radius) * cam.zoom.x + 10.0
	var col: Color = t.race_color() if t is Ship else Color(1, 1, 1)
	if t is Ship and t.team() == p.team():
		col = Color(0.5, 1.0, 0.5)
	var vis: bool = t is Planet or t.visible_to(p)
	if vis and screen.x > -50 and screen.x < vp.x + 50 and screen.y > -50 and screen.y < vp.y + 50:
		var pts := [Vector2(-r, -r), Vector2(r, -r), Vector2(r, r), Vector2(-r, r)]
		for i in 4:
			var a: Vector2 = screen + pts[i]
			var b: Vector2 = screen + pts[(i + 1) % 4]
			draw_line(a, a.lerp(b, 0.3), col, 2.0)
			draw_line(b, b.lerp(a, 0.3), col, 2.0)
	else:
		# off-screen arrow
		var dir := (screen - vp * 0.5).normalized()
		var edge := vp * 0.5 + dir * (minf(vp.x, vp.y) * 0.42)
		draw_line(edge, edge - dir * 22.0 + dir.orthogonal() * 10.0, col, 2.0)
		draw_line(edge, edge - dir * 22.0 - dir.orthogonal() * 10.0, col, 2.0)
	# readout
	var x := vp.x - 330.0
	var y := 24.0
	var boardable_station: bool = t is Ship and t.is_station and t.team() != p.team() and Capture.why_not(p, t) not in ["starbase", "neutral", "duel", "arena", "friendly"]
	var tall := 112.0 + (60.0 if t is Ship and not t.systems.list.is_empty() else (24.0 if boardable_station and vis else 0.0))
	draw_rect(Rect2(x - 10, y - 4, 320, tall), Color(0.02, 0.04, 0.08, 0.6))
	if t is Ship:
		_draw_ship_picture(t, Rect2(x - 10, y + tall + 2, 320, PIC_H), vis, col)
	if t is Ship:
		draw_string(font, Vector2(x, y + 14), "%s   %s %s" % [t.pilot_name, GameData.race(t.race_id)["adj"], t.design_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, col)
		draw_string(font, Vector2(x, y + 34), "%s   range %d" % [GameData.rank_name(t.rank), int(t.global_position.distance_to(p.global_position))], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.9, 0.9, 0.9))
		_shield_bar(Vector2(x, y + 44), 300, 18, t, false)
		_bar(Vector2(x, y + 66), 300, 18, t.hull / maxf(t.hull_max, 1.0), Color(0.95, 0.55, 0.25, 0.85), "Hull", "%d" % int(t.hull))
		if not vis:
			var q := p.contact_quality(t)
			var why := "Contact lost" if q == 0 else ("Contact: no lock" if q == 2 else "Bearing only: no lock")
			draw_string(font, Vector2(x, y + 102), why, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1, 0.7, 0.5))
		elif not t.systems.list.is_empty():
			_draw_systems(t, Vector2(x, y + 90), 300)
			var line := "Crew %d/%d" % [int(ceil(t.crew)), t.crew_max]
			var lcol := Color(0.9, 0.9, 0.9)
			if p.boarding == t:
				line += "   BOARDING %d%%   (%s calls it off)" % [int(p.board_progress * 100.0), Keybinds.short_text("board")]
				lcol = Color(0.55, 0.9, 1.0)
				_bar(Vector2(x, y + 132), 300, 10, p.board_progress, Color(0.5, 0.85, 1.0, 0.9), "", "")
			elif t.is_disabled() and t.team() != p.team():
				var why := Capture.why_not(p, t)
				lcol = Color(1.0, 0.6, 0.3) if fmod(time * 2.0, 1.0) < 0.7 else Color(1.0, 0.85, 0.5)
				if why == "far":
					line += "   DISABLED: close to %d, then %s" % [int(Capture.board_range(p, t)), Keybinds.short_text("board")]
				else:
					line += "   DISABLED: " + (("%s to board" % Keybinds.short_text("board")) if why == "" else Capture.reason_text(why, p, t))
			draw_string(font, Vector2(x, y + 126), line, HORIZONTAL_ALIGNMENT_LEFT, 300, 13, lcol)
		elif boardable_station:
			# an enemy station: its crew, and whether it can be boarded yet
			var line := "Crew %d/%d" % [int(ceil(t.crew)), t.crew_max]
			var lcol := Color(0.9, 0.9, 0.9)
			if p.boarding == t:
				line += "   BOARDING %d%%   (%s calls it off)" % [int(p.board_progress * 100.0), Keybinds.short_text("board")]
				lcol = Color(0.55, 0.9, 1.0)
				_bar(Vector2(x, y + 112), 300, 10, p.board_progress, Color(0.5, 0.85, 1.0, 0.9), "", "")
			elif t.is_breached():
				var why := Capture.why_not(p, t)
				lcol = Color(1.0, 0.6, 0.3) if fmod(time * 2.0, 1.0) < 0.7 else Color(1.0, 0.85, 0.5)
				if why == "far":
					line += "   BREACHED: close to %d, then %s" % [int(Capture.board_range(p, t)), Keybinds.short_text("board")]
				else:
					line += "   BREACHED: " + (("%s to board" % Keybinds.short_text("board")) if why == "" else Capture.reason_text(why, p, t))
			else:
				line += "   board when shields down, hull < %d%%" % int(Capture.STATION_BREACH_HULL * 100.0)
			draw_string(font, Vector2(x, y + 104), line, HORIZONTAL_ALIGNMENT_LEFT, 300, 13, lcol)


## The target as it looks right now, under its readout: its own painting,
## coat, turrets and damage.  A contact you can't see is only a dim shape.
func _draw_ship_picture(t: Ship, box: Rect2, vis: bool, col: Color) -> void:
	draw_rect(box, Color(0.02, 0.04, 0.08, 0.6))
	draw_rect(box, Color(col.r, col.g, col.b, 0.35), false, 1.0)
	var m: Dictionary = t.art_model if not t.art_model.is_empty() else ShipArt.build_model(t.design, t.hull_race if t.hull_race >= 0 else t.race_id, t.size)
	var rect: Rect2 = m["rect"]
	var scale := minf(box.size.x * 0.84 / maxf(rect.size.x, 1.0), box.size.y * 0.8 / maxf(rect.size.y, 1.0))
	var tex: Texture2D = m["tex"]
	if tex != null:
		scale = minf(scale, 1.9 * tex.get_width() / maxf(rect.size.x, 1.0))   # no blurrier than the shipyard shows it
	var at := Transform2D(0.0, Vector2.ONE * scale, 0.0, box.get_center() - rect.get_center() * scale)
	if vis:
		var aims := []
		for w in t.weapons:
			aims.append(w.get("aim_vis", w["fire_angle"]))
		ShipArt.draw_model(self, m, t.race_color(), t.thrust_level, 1.0, 1.0 - t.hull / maxf(t.hull_max, 1.0), at, aims,
			clampf(t.hit_flash * 5.0, 0.0, 1.0), t.systems.broken_map(), t._coat_wear(), t.pdc.aims())
	else:
		ShipArt.draw_model(self, m, Color(0.5, 0.55, 0.6), 0.0, 0.18, 0.0, at)
		draw_string(font, Vector2(box.position.x, box.end.y - 10), "NO VISUAL", HORIZONTAL_ALIGNMENT_CENTER, box.size.x, 12, Color(1, 0.7, 0.5, 0.8))
	draw_set_transform_matrix(Transform2D.IDENTITY)


func _draw_nav(p: Ship, vp: Vector2) -> void:
	var t := p.nav_target
	if t == null or not is_instance_valid(t):
		return
	var cam := world.camera
	var screen := (world.drawn_position(t) - world.camera_centre()) * cam.zoom + vp * 0.5
	var col := Color(0.6, 1.0, 0.8)
	var dist := int(t.global_position.distance_to(p.global_position) - t.radius)
	var tag: String = t.body_name
	if t is Planet and t.is_colonised():
		tag = "%s  %s %s" % [t.body_name, GameData.race(t.owner_race)["adj"], GameData.fmt_pop(t.population)]
	if screen.x > -50 and screen.x < vp.x + 50 and screen.y > -50 and screen.y < vp.y + 50:
		var r: float = t.radius * cam.zoom.x + 14.0
		draw_arc(screen, r, 0, TAU, 48, Color(col.r, col.g, col.b, 0.5), 1.5, true)
		draw_string(font, screen + Vector2(-40, -r - 6), "%s  %d" % [tag, dist], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, col)
	else:
		var dir := (screen - vp * 0.5).normalized()
		var edge := _edge_anchor(dir, vp)
		draw_circle(edge, 5.0, col)
		var lpos := edge - dir * 30.0 + Vector2(-64, 5)
		draw_string_outline(font, lpos, "%s  %d" % [t.body_name, dist], HORIZONTAL_ALIGNMENT_CENTER, 128, 13, 3, Color(0, 0, 0, 0.8))
		draw_string(font, lpos, "%s  %d" % [t.body_name, dist], HORIZONTAL_ALIGNMENT_CENTER, 128, 13, col)


## The shift-clicked world's info card: beside the world, kept on screen,
## with a ring on the world itself.
func _draw_planet_card(p: Ship, vp: Vector2) -> void:
	var pl: Planet = world.inspected_planet
	if pl == null or not is_instance_valid(pl):
		return
	var cam := world.camera
	var screen := (world.drawn_position(pl) - world.camera_centre()) * cam.zoom + vp * 0.5
	var r: float = pl.drawn_radius() * cam.zoom.x
	var on_screen := Rect2(Vector2.ZERO, vp).grow(r).has_point(screen)
	if on_screen:
		draw_arc(screen, r + 8.0, 0, TAU, 64, Color(0.9, 0.95, 1.0, 0.6), 1.5, true)
	var rows: Array = pl.info_lines(p.race_id)
	rows.append(["Select it again to close (%s)" % Keybinds.short_text("select_target"), "hint"])
	var w := 0.0
	for row in rows:
		w = maxf(w, font.get_string_size(row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 18 if row[1] == "title" else 14).x)
	w = maxf(w, 200.0)
	var size := Vector2(w + 28.0, 18.0 + rows.size() * 20.0 + 8.0 + CARD_PIC_H)
	var at := screen + Vector2(r + 24.0, -size.y * 0.5)
	if at.x + size.x > vp.x - 16.0:
		at.x = screen.x - r - 24.0 - size.x
	at.x = clampf(at.x, 16.0, vp.x - size.x - 16.0)
	at.y = clampf(at.y, 70.0, vp.y - size.y - 60.0)
	var box := Rect2(at, size)
	draw_rect(box, Color(0.02, 0.04, 0.1, 0.88))
	draw_rect(box, Color(0.5, 0.7, 1.0, 0.6), false, 1.5)
	if pl.is_colonised():
		draw_rect(Rect2(at, Vector2(4.0, size.y)), world.colony_color(world.player.race_id if world.player != null else -1, pl.owner_race))
	var y := at.y + 26.0
	for row in rows:
		if row[1] == "bar":
			# a ground battle: the attackers' share of the strength on the ground
			var bar := Rect2(at.x + 14.0, y - 11.0, size.x - 28.0, 9.0)
			draw_rect(bar, row[4].darkened(0.25))
			draw_rect(Rect2(bar.position, Vector2(bar.size.x * clampf(row[2], 0.0, 1.0), bar.size.y)), row[3])
			y += 20.0
			continue
		var col: Color = {"title": Color(1, 1, 1), "sub": Color(0.6, 0.72, 0.9), "good": Color(0.55, 1.0, 0.65),
			"warn": Color(1.0, 0.7, 0.3), "hint": Color(0.6, 0.7, 0.85, 0.7)}.get(row[1], Color(0.88, 0.92, 1.0))
		draw_string(font, Vector2(at.x + 14.0, y), row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 18 if row[1] == "title" else 14, col)
		y += 22.0 if row[1] == "title" else 20.0
	# the world itself at the foot of the card, lit as it is now
	var pic := Rect2(at.x + 6.0, box.end.y - CARD_PIC_H - 4.0, size.x - 12.0, CARD_PIC_H)
	draw_rect(pic, Color(0.0, 0.01, 0.04, 0.9))
	planet_pic.show_planet(pl, pic)


func _draw_radar(p: Ship, vp: Vector2) -> void:
	var r := 120.0
	var c := Vector2(vp.x - r - 30.0, vp.y - r - 30.0)
	draw_circle(c, r + 4, Color(0.02, 0.04, 0.08, 0.7))
	draw_arc(c, r, 0, TAU, 64, Color(0.4, 0.8, 0.6, 0.6), 1.5, true)
	draw_arc(c, r * 0.5, 0, TAU, 48, Color(0.4, 0.8, 0.6, 0.2), 1.0, true)
	var scale := r / radar_range
	# sensor range circle
	var sr := p.sensor_range * scale
	if sr < r:
		draw_arc(c, sr, 0, TAU, 48, Color(0.4, 0.8, 0.6, 0.3), 1.0, true)
	for pl in world.planets:
		var d := (pl.global_position - p.global_position) * scale
		if d.length() > r:
			continue
		var pr := clampf(pl.radius * scale, 1.5, 14.0)
		var col := Color(0.6, 0.6, 0.6)
		if pl.look == "Sun":
			col = Color(1.0, 0.8, 0.3)
		elif pl.is_colonised():
			col = world.colony_color(p.race_id, pl.owner_race)
		draw_circle(c + d, pr, col)
	for s in world.ships:
		if s.dead or s == p:
			continue
		var q := p.contact_quality(s)
		if q == 0:
			continue
		var col := world.ship_color(p, s)
		if q == 1:
			# a bearing only (an EM or temporal receiver): a tick on the rim
			var dir := (s.global_position - p.global_position).normalized()
			draw_line(c + dir * (r - 10.0), c + dir * r, Color(1.0, 0.72, 0.3, 0.85), 2.0)
			continue
		var at: Vector2 = s.global_position if q >= 3 else Sensors.estimate(p, s)
		var d := (at - p.global_position) * scale
		if d.length() > r:
			continue
		if q == 2:
			# a contact: somewhere about there, no lock
			draw_arc(c + d, 5.5, 0, TAU, 12, Color(col, 0.55), 1.2)
			draw_string(font, c + d + Vector2(-3, 4), "?", HORIZONTAL_ALIGNMENT_LEFT, -1, 10, Color(1, 1, 1, 0.75))
			continue
		var hostile := world.shown_hostile(p, s)
		if s.is_station:
			draw_rect(Rect2(c + d - Vector2(4, 4), Vector2(8, 8)), col)
		else:
			_heading_mark(c + d, s.travel_dir(), 3.5, col)
		if hostile and not s.is_station:
			draw_arc(c + d, 7.0, 0, TAU, 14, Color(1.0, 0.25, 0.2, 0.9), 1.2)
		if s == p.target:
			draw_arc(c + d, 9.0, 0, TAU, 18, Color.WHITE, 1.5)
	# ghosts: where lost contacts were last seen, drifting on as they went
	var now := p.sensors.now()
	for id in p.sensors.last_seen.keys():
		var g: Array = p.sensors.last_seen[id]
		var age: float = now - g[2]
		var o := instance_from_id(id)
		if age > Sensors.GHOST_TIME or o == null or o.dead:
			p.sensors.last_seen.erase(id)
			continue
		if age < Sensors.CACHE_TIME * 2.0:
			continue
		var d: Vector2 = (g[0] + g[1] * age - p.global_position) * scale
		if d.length() > r:
			continue
		var fade := 1.0 - age / Sensors.GHOST_TIME
		for k in 4:
			var a0 := k * TAU / 4.0 + time
			draw_arc(c + d, 5.0, a0, a0 + TAU / 8.0, 4, Color(0.8, 0.8, 0.85, 0.6 * fade), 1.2)
	_draw_signature(p, c, r)
	# own ship: the mark points where you are going, the tick where you face
	draw_line(c + p.forward() * 10.0, c + p.forward() * 16.0, Color(1, 1, 1, 0.6), 1.5)
	_heading_mark(c, p.travel_dir(), 4.0, Color.WHITE)
	draw_string(font, c + Vector2(-r, r + 22), "Radar %d   +/- zoom" % int(radar_range), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.6, 0.9, 0.7, 0.8))


## A ship on the radar: a narrow dart pointing along dir, about s across.
func _heading_mark(at: Vector2, dir: Vector2, s: float, col: Color) -> void:
	var side := dir.orthogonal()
	for k in 2:
		# a dark rim first so the dart reads over planets
		var e := s + (1.2 if k == 0 else 0.0)
		var tip := at + dir * e * 2.2
		var tail := at - dir * e * 1.3
		draw_colored_polygon(PackedVector2Array([tip, tail + side * e, tail - side * e]), Color(0, 0, 0, 0.55) if k == 0 else col)



const SIG_COLORS := {"radar": Color(0.45, 0.95, 0.6), "heat": Color(1.0, 0.55, 0.25), "em": Color(0.4, 0.8, 1.0), "temporal": Color(0.75, 0.55, 1.0)}

## How loud you are (left of the radar): one bar per channel, full at 200
## (a battleship's radar return is 100), and the warning receiver.
func _draw_signature(p: Ship, c: Vector2, r: float) -> void:
	var x := c.x - r - 58.0
	var bottom := c.y + r - 10.0
	var h := 96.0
	draw_rect(Rect2(x - 8, bottom - h - 26, 54, h + 48), Color(0.02, 0.04, 0.08, 0.6))
	draw_string(font, Vector2(x - 4, bottom - h - 10), "Signature", HORIZONTAL_ALIGNMENT_LEFT, 50, 10, Color(0.7, 0.85, 0.95, 0.8))
	var i := 0
	for ch in Sensors.CHANNELS:
		var v := p.signature(ch)
		var f := clampf(v / 200.0, 0.0, 1.0)
		var bx := x + i * 10.0
		draw_rect(Rect2(bx, bottom - h, 7, h), Color(1, 1, 1, 0.08))
		if f > 0.0:
			draw_rect(Rect2(bx, bottom - h * f, 7, h * f), SIG_COLORS[ch])
		draw_string(font, Vector2(bx, bottom + 13), ch.substr(0, 1).to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 10, SIG_COLORS[ch])
		i += 1
	# the warning receiver: who has a lock on us, and our sensors jammed
	var warn := []
	for pt in Sensors.painters(p):
		var w: String = {"ping": "PINGED", "lidar": "LIDAR LOCK"}.get(pt[1], "PAINTED")
		if not w in warn:
			warn.append(w)
	if p.sensors.is_jammed():
		warn.append("JAMMED")
	if warn.is_empty():
		return
	var flash := fmod(time * 2.5, 1.0) < 0.65
	var y := c.y - r - 30.0
	for w in warn:
		var tw := font.get_string_size(w, HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x
		draw_rect(Rect2(c.x - tw * 0.5 - 6, y - 14, tw + 12, 19), Color(0.35, 0.05, 0.03, 0.75) if flash else Color(0.2, 0.04, 0.03, 0.6))
		draw_string(font, Vector2(c.x - tw * 0.5, y), w, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1.0, 0.45, 0.3) if flash else Color(1.0, 0.7, 0.55))
		y -= 22.0

func _draw_messages(vp: Vector2) -> void:
	var y := 72.0
	var x := 24.0
	for m in messages:
		var age: float = time - m[2]
		var a := clampf(1.0 - (age - 8.0) / 3.0, 0.0, 1.0)
		if a <= 0.0:
			continue
		var col: Color = m[1]
		col.a = a
		_text(Vector2(x, y), m[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, col)
		y += 20


func _draw_rank(p: Ship, vp: Vector2) -> void:
	var x := 24.0
	var y := 24.0
	if world.duel_rules != null:
		_draw_duel(p, vp)
		return
	if world.mode == "skirmish" and world.mode_rules != null:
		_draw_skirmish(p, vp)
		return
	var rank := GameData.rank_name(p.rank)
	var next := ""
	if p.rank + 1 < GameData.RANKS.size():
		var nr: Dictionary = GameData.RANKS[p.rank + 1]
		next = "   next: %d pts, %d kills" % [nr["score"], nr["kills"]]
	_text(Vector2(x, y), "%s %s   score %d   kills %d%s" % [rank, p.pilot_name, p.score, p.kills, next], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 0.95, 0.8))
	_text(Vector2(x, y + 20), "credits %d   hold %d/%d%s%s%s" % [p.credits, p.hold.size(), Ship.HOLD_SIZE, ("   blueprints %d" % p.blueprints.size()) if not p.blueprints.is_empty() else "",
		("   colonists %s / %s" % [GameData.fmt_pop(p.colonists), GameData.fmt_pop(p.cabin)]) if p.cabin > 0.0 else "",
		(("   %s kit aboard" % GameData.STATIONS[p.kit]["label"].to_lower()) if p.kit != "" else ("   cargo %d / %d" % [int(p.cargo_total()), int(p.cargo_max)])) if p.cargo_max > 0.0 else ""], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1.0, 0.85, 0.4))
	if not p.dead:
		_draw_landing(p, vp)
	# docking prompt
	var dock_text := ""
	if not p.dead and not p.is_landing() and world.nearest_base(p) != null:
		dock_text = "In docking range:  U  opens the shipyard   %s  cargo, kits and ship orders" % Keybinds.short_text("cargo") + ("" if p.speed() <= 220.0 else "  (brake first)")
	elif not p.dead and not p.is_landing():
		var port = world.cargo_port(p)
		if port != null and p.cargo_max > 0.0:
			dock_text = "Alongside %s:  %s  %s" % [world.port_name(port), Keybinds.short_text("cargo"), "buys and sells" if Economy.is_market(port as Ship) else "moves cargo"]
		elif p.kit != "":
			dock_text = "%s kit aboard:  %s  deploys it here" % [GameData.STATIONS[p.kit]["label"], Keybinds.short_text("cargo")]
	if dock_text != "":
		# on its own dark plate low on the screen, so the starbase art never hides it
		var plate := Rect2(vp.x * 0.5 - 330, vp.y - 98, 660, 32)
		draw_rect(plate, Color(0.02, 0.06, 0.05, 0.78))
		draw_rect(plate, Color(0.4, 1.0, 0.7, 0.55), false, 1.5)
		draw_string_outline(font, Vector2(plate.position.x, plate.position.y + 22), dock_text, HORIZONTAL_ALIGNMENT_CENTER, plate.size.x, 16, 4, Color(0, 0, 0, 0.85))
		draw_string(font, Vector2(plate.position.x, plate.position.y + 22), dock_text, HORIZONTAL_ALIGNMENT_CENTER, plate.size.x, 16, Color(0.7, 1.0, 0.8, 0.85 + 0.15 * sin(time * 4.0)))
	if world.mode == "survival" and world.mode_rules != null:
		_draw_survival(p, vp)
		return
	# empire standings
	var sx := vp.x * 0.5 - 195.0
	_text(Vector2(sx - 70, 26), "Planets", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.7, 0.8, 0.9, 0.9))
	var parts := []
	for r in [0, 1, 2, 3]:
		var n: int = world.planet_counts.get(r, 0)
		var col: Color = GameData.race(r)["color"]
		draw_circle(Vector2(sx, 20), 7.5, Color(0, 0, 0, 0.7))
		draw_circle(Vector2(sx, 20), 6, col)
		_text(Vector2(sx + 12, 26), "%s %d" % [GameData.race(r)["name"], n], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.92, 0.92, 0.92))
		sx += 102
	_draw_round(vp)


## A prompt plate low on the screen, like the docking one.
func _plate(text: String, y: float, col: Color, border: Color) -> void:
	var plate := Rect2(vp_size().x * 0.5 - 270, y, 540, 32)
	draw_rect(plate, Color(0.02, 0.05, 0.08, 0.8))
	draw_rect(plate, border, false, 1.5)
	draw_string_outline(font, Vector2(plate.position.x, plate.position.y + 22), text, HORIZONTAL_ALIGNMENT_CENTER, plate.size.x, 16, 4, Color(0, 0, 0, 0.85))
	draw_string(font, Vector2(plate.position.x, plate.position.y + 22), text, HORIZONTAL_ALIGNMENT_CENTER, plate.size.x, 16, col)


func vp_size() -> Vector2:
	return get_viewport_rect().size


## Landing: the prompt when a world is close below, the descent, and the
## ground panel with the colony, the cabin and what the keys do.
func _draw_landing(p: Ship, vp: Vector2) -> void:
	if p.is_landed():
		_draw_ground_panel(p, vp)
		return
	if p.is_landing():
		var going := "Landing on" if p.landing == Ship.Landing.DESCENDING else "Lifting off from"
		_plate("%s %s..." % [going, p.land_body.body_name], vp.y - 98, Color(0.85, 0.95, 1.0), Color(0.5, 0.75, 1.0, 0.55))
		return
	if p.is_station or p.warping:
		return
	var b: Planet = world.landing_body(p)
	if b == null or not b.has_surface():
		return
	var alt: float = p.global_position.distance_to(b.global_position) - b.radius
	if alt > Ship.LAND_ALTITUDE + p.size:
		return
	var fast: bool = (p.velocity - b.velocity).length() > Ship.LAND_SPEED
	var what := "B  lands on %s" % b.body_name
	if b.is_colonised():
		what += "  (%s %s)" % [GameData.race(b.owner_race)["adj"], GameData.fmt_pop(b.population)]
	if fast:
		what += "   slow down first"
	var y := vp.y - (136.0 if world.nearest_base(p) != null else 98.0)
	_plate(what, y, Color(1.0, 0.92, 0.7, 0.85 + 0.15 * sin(time * 4.0)) if not fast else Color(1.0, 0.75, 0.55, 0.8), Color(1.0, 0.8, 0.45, 0.55))


func _draw_ground_panel(p: Ship, vp: Vector2) -> void:
	var b: Planet = p.land_body
	var w := 660.0
	var h := 128.0
	var x := (vp.x - w) * 0.5
	var y := vp.y - h - 70.0
	draw_rect(Rect2(x, y, w, h), Color(0.02, 0.05, 0.08, 0.86))
	var friendly: bool = b.is_colonised() and b.owner_race == p.race_id
	var border: Color = GameData.race(b.owner_race)["color"] if b.is_colonised() else Color(0.7, 0.7, 0.7)
	border.a = 0.7
	draw_rect(Rect2(x, y, w, h), border, false, 1.5)
	var title := "ON THE GROUND:  %s" % b.body_name.to_upper()
	_text(Vector2(x + 16, y + 24), title, HORIZONTAL_ALIGNMENT_LEFT, -1, 17, Color(0.9, 0.95, 1.0))
	var line := ""
	if b.is_colonised():
		line = "%s colony, %s people (the world supports %s)%s" % [GameData.race(b.owner_race)["adj"], GameData.fmt_pop(b.population),
			GameData.fmt_pop(b.optimal_population(b.owner_race)), "   DEFENCES DOWN" if b.is_defenceless() else ""]
	else:
		line = "Uninhabited: supports %s %s" % [GameData.fmt_pop(b.optimal_population(p.race_id)), GameData.race(p.race_id).get("plural", GameData.race(p.race_id)["name"])]
	_text(Vector2(x + 16, y + 48), line, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.85, 0.9, 1.0))
	# the cabin
	var frac := p.colonists / maxf(p.cabin, 1.0)
	var label := "Colonists"
	if p.transfer > 0:
		label = "Colonists boarding"
	elif p.transfer < 0:
		label = "Colonists going ashore"
	_bar(Vector2(x + 16, y + 58), w - 32, 19, frac, Color(0.95, 0.8, 0.35, 0.85), label, "%s / %s" % [GameData.fmt_pop(p.colonists), GameData.fmt_pop(p.cabin)])
	var keys := []
	if friendly:
		keys.append("1  take colonists on")
		keys.append("2  put colonists down")
		if p.cargo_max > 0.0:
			keys.append("%s  cargo (%d / %d)" % [Keybinds.short_text("cargo"), int(p.cargo_total()), int(p.cargo_max)])
	elif p.colonists > 0.0:
		keys.append("2  %s" % ("invade" if b.is_colonised() else "settle"))
	keys.append("B  lift off")
	_text(Vector2(x, y + 104), "     ".join(keys), HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(1.0, 0.92, 0.7))
	var hint := ""
	if friendly:
		hint = "Refuelling and repairing at the spaceport."
	elif b.is_colonised():
		hint = "Invaders fight twice as hard once its defences are down."
	_text(Vector2(x, y + 122), hint, HORIZONTAL_ALIGNMENT_CENTER, w, 12, Color(0.7, 0.8, 0.9, 0.8))


## The round clock under the standings, the phase, and the countdown when
## one empire holds every colony.
func _draw_round(vp: Vector2) -> void:
	var rd: Round = world.round_rules
	if rd == null:
		return
	var cx := vp.x * 0.5
	var urgent := rd.phase in ["final", "sudden_death"]
	var col := Color(1.0, 0.75, 0.35, 0.75 + 0.25 * sin(time * 5.0)) if urgent else Color(0.85, 0.92, 1.0, 0.9)
	var label := rd.clock_text()
	if rd.phase == "sudden_death":
		label = "SUDDEN DEATH"
	elif rd.phase == "final":
		label += "   FINAL PHASE: double income"
	elif rd.phase == "opening":
		label += "   opening"
	_text(Vector2(cx - 250, 50), label, HORIZONTAL_ALIGNMENT_CENTER, 500, 17, col)
	if rd.hold_race >= 0 and not rd.over:
		var hc: Color = GameData.race(rd.hold_race)["color"]
		var left := maxf(Round.HOLD_SECONDS - rd.hold_timer, 0.0)
		var txt := "The %s hold every colony: %d s to victory" % [GameData.race(rd.hold_race).get("plural", GameData.race(rd.hold_race)["name"]), int(ceil(left))]
		_text(Vector2(cx - 300, 76), txt, HORIZONTAL_ALIGNMENT_CENTER, 600, 16, hc)
		var bw := 320.0
		draw_rect(Rect2(cx - bw * 0.5, 84, bw, 5), Color(0, 0, 0, 0.5))
		draw_rect(Rect2(cx - bw * 0.5, 84, bw * rd.hold_timer / Round.HOLD_SECONDS, 5), hc)


## A duel: your tally top left, the round's leaderboard top centre in
## two columns, and the winner between rounds.
func _draw_duel(p: Ship, vp: Vector2) -> void:
	var du: Duel = world.duel_rules
	_text(Vector2(24, 24), "%s   kills %d   deaths %d" % [p.pilot_name, p.kills, p.deaths], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 0.95, 0.8))
	_text(Vector2(24, 44), "Free-for-all: everyone is your enemy.  %s targets the nearest pilot." % Keybinds.short_text("target_nearest"), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.75, 0.82, 0.95, 0.85))
	var cx := vp.x * 0.5
	var head := "FREE-FOR-ALL   round %d   first to %d kills   %s left" % [du.round_no, Duel.KILLS_TO_WIN, du.clock_text()]
	if du.break_left > 0.0:
		head = "CEASEFIRE   next round in %d" % int(ceil(du.break_left))
	_text(Vector2(cx - 360, 22), head, HORIZONTAL_ALIGNMENT_CENTER, 720, 16, Color(1.0, 0.85, 0.45) if du.break_left > 0.0 else Color(0.85, 0.92, 1.0))
	var board := du.leaderboard()
	for i in mini(board.size(), 8):
		var s: Ship = board[i]
		var col_x := cx - 350.0 + float(i / 4) * 360.0
		var y := 44.0 + float(i % 4) * 17.0
		var me := s == p
		var c: Color = Color(1, 0.95, 0.6) if me else (Color(0.55, 0.55, 0.6) if s.dead else Color(0.9, 0.9, 0.92))
		draw_circle(Vector2(col_x + 6, y - 5), 4.5, s.race_color())
		_text(Vector2(col_x + 16, y), "%d. %s" % [i + 1, s.pilot_name], HORIZONTAL_ALIGNMENT_LEFT, 220, 14, c)
		_text(Vector2(col_x + 220, y), "%d  /  %d" % [s.kills, s.deaths], HORIZONTAL_ALIGNMENT_RIGHT, 110, 14, c)
	if du.break_left > 0.0:
		var msg := ("%s wins round %d!" % [du.winner_name, du.round_no]) if du.winner_name != "" else ("Round %d is over." % du.round_no)
		_text(Vector2(0, vp.y * 0.3), msg, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 40, Color(1.0, 0.9, 0.55))
		_text(Vector2(0, vp.y * 0.3 + 36), "Next round in %d.  Everyone relaunches with a clean slate." % int(ceil(du.break_left)), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 18, Color(1, 1, 1, 0.9))


## Survival: the wave, what is left of it or the break's countdown, the
## starbase's hull, and the defenders' tally.
func _draw_survival(p: Ship, vp: Vector2) -> void:
	var sv: Survival = world.mode_rules
	var cx := vp.x * 0.5
	var head := "SURVIVAL   run %d   wave %d   %d raider%s left" % [sv.run, sv.wave, sv.enemies_left, "" if sv.enemies_left == 1 else "s"]
	var col := Color(1.0, 0.75, 0.5)
	if sv.over:
		head = "THE STARBASE HAS FALLEN   new run in %s" % sv.clock_text()
		col = Color(1.0, 0.55, 0.45)
	elif sv.break_left > 0.0:
		head = "SURVIVAL   run %d   wave %d in %s   dock (U) to mend and refit" % [sv.run, sv.wave + 1, sv.clock_text()]
		col = Color(0.75, 1.0, 0.8)
	elif sv.from_text != "":
		head += "   from the " + sv.from_text
	_text(Vector2(cx - 400, 22), head, HORIZONTAL_ALIGNMENT_CENTER, 800, 16, col)
	# the starbase's hull
	var b: Ship = sv.base()
	var bw := 320.0
	var frac := 0.0 if b == null else clampf(b.hull / maxf(b.hull_max, 1.0), 0.0, 1.0)
	_text(Vector2(cx - bw * 0.5 - 90, 44), "Starbase", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.7, 0.8, 0.9, 0.9))
	draw_rect(Rect2(cx - bw * 0.5, 34, bw, 8), Color(0, 0, 0, 0.55))
	draw_rect(Rect2(cx - bw * 0.5, 34, bw * frac, 8), Color(0.4, 1.0, 0.55).lerp(Color(1.0, 0.3, 0.2), 1.0 - frac))
	if b != null and b.shield_max > 0.0:
		draw_rect(Rect2(cx - bw * 0.5, 43, bw * clampf(b.shield / b.shield_max, 0.0, 1.0), 3), Color(0.45, 0.75, 1.0, 0.9))
	_text(Vector2(cx + bw * 0.5 + 10, 44), "best %d wave%s" % [sv.best, "" if sv.best == 1 else "s"], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.7, 0.8, 0.9, 0.9))
	var pilots := sv.pilots()
	for i in mini(pilots.size(), Survival.MAX_PLAYERS):
		var s: Ship = pilots[i]
		var x := cx - 350.0 + float(i) * 180.0
		var c: Color = Color(1, 0.95, 0.6) if s == p else (Color(0.55, 0.55, 0.6) if s.dead else Color(0.9, 0.9, 0.92))
		_text(Vector2(x, 68), "%s  %d" % [s.pilot_name, sv.run_kills(s)], HORIZONTAL_ALIGNMENT_CENTER, 170, 14, c)


## Skirmish: your tally top left, the two sides' scores and the clock top
## centre with each side's pilots under them, and the winner between rounds.
func _draw_skirmish(p: Ship, vp: Vector2) -> void:
	var sk: Skirmish = world.mode_rules
	_text(Vector2(24, 24), "%s   kills %d   deaths %d   %s" % [p.pilot_name, p.kills, p.deaths, p.design_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 0.95, 0.8))
	_text(Vector2(24, 44), "Skirmish: the %s against the %s.  %s targets the nearest enemy." % [sk.side_name(0), sk.side_name(1), Keybinds.short_text("target_nearest")], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.75, 0.82, 0.95, 0.85))
	var cx := vp.x * 0.5
	var head := "round %d   first to %d   %s left" % [sk.round_no, Skirmish.KILLS_TO_WIN, sk.clock_text()]
	if sk.break_left > 0.0:
		head = "CEASEFIRE   next round in %d" % int(ceil(sk.break_left))
	_text(Vector2(cx - 360, 22), head, HORIZONTAL_ALIGNMENT_CENTER, 720, 15, Color(1.0, 0.85, 0.45) if sk.break_left > 0.0 else Color(0.85, 0.92, 1.0))
	for side in [0, 1]:
		var col: Color = GameData.race(sk.teams[side])["color"]
		var x := cx - 350.0 + float(side) * 360.0
		draw_circle(Vector2(x + 6, 39), 5.5, col)
		_text(Vector2(x + 16, 46), "%s   %d" % [sk.side_name(side), sk.score[side]], HORIZONTAL_ALIGNMENT_LEFT, 320, 18, col.lerp(Color.WHITE, 0.35))
		var board := sk.roster(side)
		for i in mini(board.size(), Skirmish.TEAM_SIZE):
			var s: Ship = board[i]
			var y := 64.0 + float(i) * 16.0
			var c: Color = Color(1, 0.95, 0.6) if s == p else (Color(0.55, 0.55, 0.6) if s.dead else Color(0.9, 0.9, 0.92))
			_text(Vector2(x + 16, y), "%s  (%s)" % [s.pilot_name, s.design_name], HORIZONTAL_ALIGNMENT_LEFT, 230, 13, c)
			_text(Vector2(x + 240, y), "%d / %d" % [s.kills, s.deaths], HORIZONTAL_ALIGNMENT_RIGHT, 90, 13, c)
	if sk.break_left > 0.0:
		var msg := ("The %s take round %d!" % [sk.side_name(sk.winner), sk.round_no]) if sk.winner >= 0 else ("Round %d is a draw." % sk.round_no)
		_text(Vector2(0, vp.y * 0.3), msg, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 40, Color(1.0, 0.9, 0.55))
		_text(Vector2(0, vp.y * 0.3 + 36), "Next round in %d.  Everyone relaunches." % int(ceil(sk.break_left)), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 18, Color(1, 1, 1, 0.9))


## Skirmish, while waiting to launch: a card per hull with its launch time;
## 1 to 5 or a click picks one.
func _hull_cards(vp: Vector2) -> Array:
	var out := []
	var w := 150.0
	var x0 := vp.x * 0.5 - (w + 10.0) * Skirmish.HULLS.size() * 0.5 + 5.0
	for i in Skirmish.HULLS.size():
		out.append([Skirmish.HULLS[i], Rect2(x0 + i * (w + 10.0), vp.y * 0.52, w, 150)])
	return out


func _draw_hull_picker(vp: Vector2, a: float) -> void:
	var sk: Skirmish = world.mode_rules
	var p := world.player
	var pick: String = sk.picks.get(p, p.design_name)
	var col: Color = p.race_color()
	_text(Vector2(0, vp.y * 0.52 - 14), "Pick your ship for the next launch", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 17, Color(1, 1, 1, a))
	var cards := _hull_cards(vp)
	for i in cards.size():
		var hull: String = cards[i][0]
		var r: Rect2 = cards[i][1]
		var on := hull == pick
		draw_rect(r, Color(0.05, 0.08, 0.14, 0.8 * a))
		draw_rect(r, Color(1.0, 0.85, 0.4, a) if on else Color(0.5, 0.6, 0.8, 0.5 * a), false, 2.5 if on else 1.0)
		ShipArt.draw_ship(self, hull, p.race_id, 34.0, col, 0.4, 1.0, 0.0, Transform2D(-PI * 0.5, r.get_center() + Vector2(0, -12)))
		_text(Vector2(r.position.x, r.position.y + 20), "%d  %s" % [i + 1, hull], HORIZONTAL_ALIGNMENT_CENTER, r.size.x, 15, Color(1, 0.95, 0.7, a) if on else Color(0.9, 0.9, 0.95, a))
		_text(Vector2(r.position.x, r.end.y - 10), "launch %d s" % int(Skirmish.LAUNCH_DELAY[hull]), HORIZONTAL_ALIGNMENT_CENTER, r.size.x, 13, Color(0.7, 0.8, 0.9, a))


func _pick_hull(hull: String) -> void:
	var sk: Skirmish = world.mode_rules
	sk.picks[world.player] = hull
	if Net.is_client():
		Net.client_command("hull:" + hull)
	else:
		sk.choose(world.player, hull)
	Sfx.play_ui("click", 0.4)


func _draw_music(vp: Vector2) -> void:
	if now_playing.is_empty():
		return
	var age := time - now_playing_time
	var a := clampf(1.0 - (age - 4.0) / 1.5, 0.0, 1.0)
	if a <= 0.0:
		return
	var text := "Now playing: " + now_playing
	if Settings.dynamic_music and Settings.music_mode != "off":
		text += "  -  " + Music.intensity_name()
	_text(Vector2(vp.x * 0.5 - 300, vp.y - 36), text, HORIZONTAL_ALIGNMENT_CENTER, 600, 13, Color(0.9, 0.85, 1.0, a * 0.85))


## Text with a dark outline, so it stays readable over the Sun, planets
## and ships that pass under the HUD.
func _text(pos: Vector2, text: String, align: HorizontalAlignment, width: float, size: int, col: Color) -> void:
	draw_string_outline(font, pos, text, align, width, size, 4, Color(0, 0, 0, 0.75 * col.a))
	draw_string(font, pos, text, align, width, size, col)


## Where an edge marker in direction `dir` goes: on an inset screen
## edge, slid inward until neither the marker nor its label (drawn 30 px
## further in) touches a HUD panel or the round timer.  Each marker kind
## gets a small sideways nudge so two in the same direction do not stack.
func _edge_anchor(dir: Vector2, vp: Vector2) -> Vector2:
	var panels := [
		Rect2(vp.x - 345, 0, 345, 140 + PIC_H),    # target panel and its picture
		Rect2(0, vp.y - 190, 640, 190),            # status and weapons
		Rect2(vp.x - 300, vp.y - 310, 300, 310),   # radar
		Rect2(vp.x * 0.5 - 360, 0, 720, 110),      # planets, clock, hold bar
		Rect2(0, 0, 560, 50),                      # rank and credits
	]
	var centre := vp * 0.5
	var half := vp * 0.5 - Vector2(40, 40)
	var k := minf(absf(half.x / maxf(absf(dir.x), 0.001)), absf(half.y / maxf(absf(dir.y), 0.001)))
	var at := centre + dir * k
	for tries in 80:
		var lrect := Rect2(at - dir * 30.0 + Vector2(-64, -12), Vector2(128, 20))
		var blocked := false
		var tip := at + dir * 18.0
		for r in panels:
			if r.grow(8).has_point(at) or r.grow(4).has_point(tip) or r.intersects(lrect.grow(4)):
				blocked = true
				break
		if not blocked:
			break
		at -= dir * 10.0
	return at


## Arrows on the screen edge when they are off screen: the current
## target (red for an enemy), and your own starbase in green so you can
## always find the shipyard.  Arrows slide inward to stay clear of the
## HUD panels.
func _draw_pointers(p: Ship, vp: Vector2) -> void:
	if p.dead:
		return
	var xf := world.get_viewport().get_canvas_transform()
	var marks := []
	if p.target != null and is_instance_valid(p.target) and p.target is Ship and not p.target.dead:
		var hostile: bool = world.shown_hostile(p, p.target)
		marks.append([p.target, Color(1.0, 0.35, 0.3) if hostile else Color(1.0, 0.9, 0.4), p.target.pilot_name])
	var base: Ship = world.starbase_of(p.race_id)
	if base != null and base != p.target:
		marks.append([base, Color(0.45, 1.0, 0.6), "Starbase"])
	elif base == null and world.starbase_back_in(p.race_id) >= 0.0:
		# shot down: home is still home, and the arrow says when it is back
		marks.append([world._home_planet(p.race_id), Color(0.6, 0.7, 0.65), "Starbase back in %ds" % int(ceil(world.starbase_back_in(p.race_id)))])
	var centre := vp * 0.5
	for mk in marks:
		var t: Node2D = mk[0]
		var sp: Vector2 = xf * t.global_position
		if Rect2(Vector2(40, 40), vp - Vector2(80, 80)).has_point(sp):
			continue
		var dir := (sp - centre).normalized()
		var at := _edge_anchor(dir, vp)
		var label := "%s  %d" % [mk[2], int(p.global_position.distance_to(t.global_position))]
		var col: Color = mk[1]
		var side := dir.orthogonal()
		draw_colored_polygon(PackedVector2Array([at + dir * 16.0, at - dir * 6.0 + side * 10.0, at - dir * 6.0 - side * 10.0]), col)
		var lpos := at - dir * 30.0 + Vector2(-64, 5)
		draw_string_outline(font, lpos, label, HORIZONTAL_ALIGNMENT_CENTER, 128, 12, 3, Color(0, 0, 0, 0.8))
		draw_string(font, lpos, label, HORIZONTAL_ALIGNMENT_CENTER, 128, 12, col)


## The Sun's heat: an orange wash at the edges and a warning that grows.
func _draw_heat(p: Ship, vp: Vector2) -> void:
	var h: float = world.sun_heat
	if h <= 0.0 or p.dead:
		return
	var pulse := 0.5 + 0.5 * sin(time * (4.0 + 10.0 * h))
	var a := 0.12 + 0.35 * h * h
	var band := 40.0 + 220.0 * h
	# a soft vignette: nested frames that fade toward the middle, drawn
	# without overlapping corners
	var steps := 28
	var t := band / steps
	for i in steps:
		var k := 1.0 - float(i) / steps
		var col := Color(1.0, 0.45, 0.1, a * k * k * (0.7 + 0.3 * pulse))
		var o := i * t
		draw_rect(Rect2(o, o, vp.x - 2.0 * o, t), col)
		draw_rect(Rect2(o, vp.y - o - t, vp.x - 2.0 * o, t), col)
		draw_rect(Rect2(o, o + t, t, vp.y - 2.0 * o - 2.0 * t), col)
		draw_rect(Rect2(vp.x - o - t, o + t, t, vp.y - 2.0 * o - 2.0 * t), col)
	var text := "SOLAR HEAT: TURN AWAY FROM THE SUN" if h > 0.25 else "Approaching the Sun"
	var tc := Color(1.0, 0.85 - 0.5 * h, 0.4 - 0.3 * h, 0.75 + 0.25 * pulse)
	draw_string_outline(font, Vector2(0, vp.y * 0.32), text, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 26, 6, Color(0, 0, 0, 0.8))
	draw_string(font, Vector2(0, vp.y * 0.32), text, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 26, tc)


## Crosshair where the turrets are pointed (mouse or right stick).
func _draw_aim(p: Ship, vp: Vector2) -> void:
	if p.dead or p.aim_point == Vector2.INF or not p.has_turret:
		return
	var cam := world.camera
	var screen := (p.aim_point - world.camera_centre()) * cam.zoom + vp * 0.5
	if screen.x < 0 or screen.y < 0 or screen.x > vp.x or screen.y > vp.y:
		return
	var col := Color(0.6, 1.0, 0.7, 0.85)
	var hover := world.ship_at(p.aim_point, 40.0 / cam.zoom.x)
	if hover != null and hover != p and world.shown_hostile(p, hover):
		col = Color(1.0, 0.45, 0.4, 0.95)
	draw_arc(screen, 11.0, 0.0, TAU, 24, col, 1.5)
	for d in [Vector2.LEFT, Vector2.RIGHT, Vector2.UP, Vector2.DOWN]:
		draw_line(screen + d * 6.0, screen + d * 16.0, col, 1.5)


func _draw_dead(vp: Vector2) -> void:
	var a := clampf(dead_time, 0.0, 1.0)
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.3, 0.0, 0.0, 0.25 * a))
	var killer: String = world.arena_rules().killed_by if world.is_arena() else ""
	draw_string(font, Vector2(0, vp.y * 0.4), "Your ship was captured" if world.player.captured else ("Destroyed by %s" % killer if killer != "" else "Your ship was destroyed"), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 40, Color(1, 0.8, 0.7, a))
	var left := maxf(World.RESPAWN_DELAY - dead_time, 0.0)
	var line := "Relaunching in %d..." % int(ceil(left))
	if world.mode == "skirmish" and world.mode_rules != null:
		left = maxf(world.mode_rules.launch_delay(world.player) - dead_time, 0.0)
		line = "Launching in %d..." % int(ceil(left)) if left > 0.0 else "Launching..."
		_draw_hull_picker(vp, a)
	elif world.mode == "survival" and world.mode_rules != null and world.mode_rules.over:
		line = "The starbase has fallen.  A new run starts in %s." % world.mode_rules.clock_text()
	elif world.starbase_back_in(world.player.race_id) >= 0.0:
		line += "   (the starbase is down: you launch over %s)" % world._home_planet(world.player.race_id).body_name
	draw_string(font, Vector2(0, vp.y * 0.4 + 40), line, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 20, Color(1, 1, 1, a))


## A help row for an action as it is bound right now: "W / Up  (RT)".
func _keys(action: String) -> String:
	if not InputMap.has_action(action):
		return ""
	var kb := []
	for slot in ["kb0", "kb1"]:
		var t := Keybinds.slot_text(action, slot)
		if not t.is_empty():
			kb.append(t)
	var pad := Keybinds.slot_text(action, "pad")
	var out := " / ".join(kb) if not kb.is_empty() else "-"
	return out + ("  (%s)" % pad if not pad.is_empty() else "")


func _draw_help(vp: Vector2) -> void:
	var left := [
		["FLIGHT", ""],
		[_keys("thrust"), "thrust"], [_keys("reverse"), "reverse"],
		[_keys("turn_left"), "turn left"], [_keys("turn_right"), "turn right"],
		[_keys("strafe_left"), "strafe left"], [_keys("strafe_right"), "strafe right"],
		[_keys("boost"), "boost (with thrust, burns fuel)"], [_keys("brake"), "brake"], [_keys("warp"), "quantum drive (warp)"],
		[_keys("flight_assist"), "flight assist on / off"], ["Mouse / right stick", "aims the turrets"],
		["", ""],
		["SYSTEM", ""],
		[_keys("help"), "this card"], [_keys("pause_menu"), "menu"], [_keys("music_next"), "next song"],
		[_keys("fullscreen"), "full screen"], ["Mouse wheel", "camera zoom"], [_keys("radar_zoom_in"), "radar zoom in"], [_keys("radar_zoom_out"), "radar zoom out"],
	]
	var right := [
		["WEAPONS", ""],
		[_keys("fire_primary"), "fire the selected group"], [_keys("fire_secondary"), "alt-fire: group %d" % Settings.alt_fire_group],
		["%s - %s" % [Keybinds.short_text("weapon_group_1"), Keybinds.short_text("weapon_group_4")], "select weapon group 1 - 4"],
		[_keys("weapon_group_next"), "next weapon group"], [_keys("cloak"), "jammer on your target"],
		[_keys("ping"), "pulse radar ping"], [_keys("silent_running"), "silent running"],
		[_keys("pdc_toggle"), "point defence (PDC) on / off"],
		["", ""],
		["TARGETS AND NAVIGATION", ""],
		[_keys("target_next"), "next target"], [_keys("target_nearest"), "nearest enemy"], [_keys("select_target"), "target a ship, world info"],
		[_keys("tactical_map"), "system map (hold or tap)"], [_keys("nav_next"), "next nav planet"], [_keys("autopilot"), "autopilot to the nav planet"],
		[_keys("ledger"), "empire ledger: resources"],
		["", ""],
		["SHIP", ""],
		[_keys("board"), "board a disabled ship or breached station"], [_keys("refit"), "shipyard, at your starbase"], [_keys("trade"), "trade with your target (online)"],
		[_keys("cargo"), "cargo: colonies, stations, station kits"],
	]
	if InputMap.has_action("wing_menu"):
		left.append_array([["", ""], ["WING", ""], [_keys("wing_menu"), "orders wheel (hold, point, let go)"],
			[_keys("wing_attack"), "wing: attack my target"], [_keys("wing_form"), "wing: form up"]])
	if InputMap.has_action("land"):
		right.append_array([["", ""], ["COLONISTS", ""]])
	for extra in [["land", "land on the world below / lift off"], ["colonists_board", "take colonists on (landed)"], ["colonists_unload", "put them down: join, settle, invade"]]:
		if InputMap.has_action(extra[0]):
			right.append([_keys(extra[0]), extra[1]])
	var rows := maxi(left.size(), right.size())
	var w := 1060.0
	var h := 70.0 + rows * 22.0 + 94.0
	var x := (vp.x - w) * 0.5
	var y := (vp.y - h) * 0.5
	draw_rect(Rect2(x, y, w, h), Color(0.02, 0.04, 0.1, 0.92))
	draw_rect(Rect2(x, y, w, h), Color(0.5, 0.7, 1.0, 0.6), false, 1.5)
	draw_string(font, Vector2(x, y + 30), "CONTROLS", HORIZONTAL_ALIGNMENT_CENTER, w, 20, Color(0.85, 0.92, 1.0))
	for col in [[left, x + 30.0], [right, x + 500.0]]:
		var ly := y + 66
		for row in col[0]:
			var key: String = row[0]
			var desc: String = row[1]
			if desc.is_empty():
				draw_string(font, Vector2(col[1], ly), key, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
			else:
				draw_string(font, Vector2(col[1], ly), key, HORIZONTAL_ALIGNMENT_LEFT, 195, 14, Color(1, 1, 1))
				draw_string(font, Vector2(col[1] + 200.0, ly), desc, HORIZONTAL_ALIGNMENT_LEFT, 250, 15, Color(0.85, 0.9, 1.0))
			ly += 22
	var fy := y + 66 + rows * 22.0 + 10
	draw_string(font, Vector2(x, fy), "Knock out enemy colonies' defences, then land colonists to take them.  Settle empty moons and asteroids.", HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
	draw_string(font, Vector2(x, fy + 22), "Capture enemy ships: shoot out engines or reactor, then board with %s. Stations: shields down, hull under half." % Keybinds.short_text("board"), HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
	draw_string(font, Vector2(x, fy + 44), "Wrecks drop parts and blueprints.  Dock at your starbase (%s) to refit and set weapon groups." % Keybinds.short_text("refit"), HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
	draw_string(font, Vector2(x, fy + 66), "Change any key or gamepad button in Settings > Controls.", HORIZONTAL_ALIGNMENT_CENTER, w, 14, Color(0.7, 0.8, 0.95))


## A world's picture on its info card: drawn by the planet's own shader
## material, so it is the same world, lit from the same side.  A child of
## the HUD because a material belongs to a whole canvas item.
class PlanetPicture extends Node2D:
	var planet: Planet = null

	func show_planet(pl: Planet, box: Rect2) -> void:
		planet = pl
		material = pl.material_
		# room for the corona or the atmosphere
		var reach := 1.4 if pl.look == "Sun" else 1.3
		var px := minf(box.size.x, box.size.y) * 0.5 / reach
		position = box.get_center()
		scale = Vector2.ONE * px / maxf(pl.radius, 1.0)
		visible = true
		queue_redraw()

	func _draw() -> void:
		if planet == null or not is_instance_valid(planet):
			return
		var r := planet.radius
		if planet.look == "Saturn" or planet.look == "Uranus":
			var rc := Color(0.85, 0.8, 0.65, 0.45) if planet.look == "Saturn" else Color(0.7, 0.85, 0.9, 0.3)
			for i in 5:
				draw_arc(Vector2.ZERO, r * (1.35 + i * 0.09), 0, TAU, 96, rc, r * 0.06, true)
		var ext := planet.quad_extent()
		draw_texture_rect(Planet.white_tex, Rect2(-r * ext, -r * ext, r * ext * 2.0, r * ext * 2.0), false)
