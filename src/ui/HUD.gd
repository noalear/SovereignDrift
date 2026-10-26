## HUD -- the cockpit overlay: ship status, target readout, radar, message
## log, rank, music notice and the help card.
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


func _on_track(title: String, style: String) -> void:
	now_playing = "%s  (%s)" % [title, style]
	now_playing_time = time


func _on_message(text: String, color: Color) -> void:
	messages.append([text, color, time])
	if messages.size() > 7:
		messages.pop_front()


func _unhandled_input(event: InputEvent) -> void:
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
		_draw_help(vp)
	else:
		draw_string(font, Vector2(vp.x * 0.5 - 150, vp.y - 14), "H: help   Esc: menu", HORIZONTAL_ALIGNMENT_CENTER, 300, 14, Color(1, 1, 1, 0.45))


## Names in the world: every planet and moon under its body, and every
## ship you can see above it, green for your side and red for anyone else.
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
		var col := Color(0.85, 0.9, 0.97, 0.85 if moon else 1.0)
		var pos := at + Vector2(0, r + 4.0 + size)
		var w := font.get_string_size(pl.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x
		var box := Rect2(pos + Vector2(-w * 0.5, -size), Vector2(w, size + 4))
		if _crowded(box, placed):
			continue
		placed.append(box)
		_text(pos - Vector2(w * 0.5, 0), pl.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, size, col)
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
		var col := Color(0.45, 1.0, 0.5) if friend else Color(1.0, 0.38, 0.32)
		var text: String = station_label(sh) if sh.is_station else sh.pilot_name
		var pos := at - Vector2(0, reach * cam.zoom.x + 8.0)
		var w := font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x
		var box := Rect2(pos + Vector2(-w * 0.5, -13), Vector2(w, 17))
		if _crowded(box, placed):
			continue
		placed.append(box)
		_text(pos - Vector2(w * 0.5, 0), text, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, col)


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


func _draw_status(p: Ship, vp: Vector2) -> void:
	var x := 24.0
	var y := vp.y - 144.0
	var w := 300.0
	draw_rect(Rect2(x - 10, y - 34, w + 20, 148), Color(0.02, 0.04, 0.08, 0.6))
	draw_string(font, Vector2(x, y - 12), "%s   %s" % [p.ship_name, p.design_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, p.race_color())
	if not p.systems.list.is_empty():
		draw_rect(Rect2(x - 10, y - 66, w + 20, 30), Color(0.02, 0.04, 0.08, 0.6))
		_draw_systems(p, Vector2(x, y - 60), w)
	_bar(Vector2(x, y), w, 19, p.shield / maxf(p.shield_max, 1.0), Color(0.3, 0.6, 1.0, 0.85), "Shields", "%d / %d" % [int(p.shield), int(p.shield_max)])
	_bar(Vector2(x, y + 22), w, 19, p.hull / maxf(p.hull_max, 1.0), Color(0.95, 0.55, 0.25, 0.85) if p.hull / p.hull_max > 0.3 else Color(1, 0.2, 0.2, 0.9), "Hull", "%d / %d" % [int(p.hull), int(p.hull_max)])
	_bar(Vector2(x, y + 44), w, 19, p.energy / maxf(p.energy_max, 1.0), Color(0.95, 0.85, 0.3, 0.85), "Energy", "%d" % int(p.energy))
	var fuel_col := Color(0.4, 0.9, 0.6, 0.85) if p.fuel_fraction() > 0.2 else Color(1, 0.35, 0.2, 0.9)
	_bar(Vector2(x, y + 66), w, 19, p.fuel_fraction(), fuel_col, "Fuel", "%d%%%s" % [int(p.fuel_fraction() * 100.0), "  refuelling" if p.refuelling else ""])
	var flags := []
	if p.fuel <= 0.0:
		flags.append("NO FUEL")
	if not p.flight_assist:
		flags.append("ASSIST OFF")
	if p.orbit_guard:
		flags.append("ORBIT GUARD")
	if p.cloaked:
		flags.append("CLOAKED")
	if p.warping:
		flags.append("QUANTUM DRIVE")
	if p.autopilot:
		flags.append("AUTOPILOT")
	if p.is_disabled():
		flags.append("DISABLED")
	if p.boarding != null and is_instance_valid(p.boarding):
		flags.append("BOARDING %d%%" % int(p.board_progress * 100.0))
	if p.boarded_by != null and is_instance_valid(p.boarded_by):
		flags.append("BOARDERS!")
	if p.inputs["boost"] and p.has_boost and p.inputs["thrust"] > 0.0:
		flags.append("BOOST")
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


func _draw_weapons(p: Ship, vp: Vector2) -> void:
	var x := 350.0
	var y := vp.y - 130.0
	draw_rect(Rect2(x - 10, y - 34, 300, 134), Color(0.02, 0.04, 0.08, 0.6))
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
	if p.can_cloak:
		sys.append("%s cloak" % Keybinds.short_text("cloak"))
	if p.qdrv > 0.0:
		sys.append("%s quantum drive" % Keybinds.short_text("warp"))
	if p.has_boost:
		sys.append("%s boost" % Keybinds.short_text("boost"))
	draw_string(font, Vector2(x, y + 94), "  ".join(sys), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.75, 0.85, 1.0, 0.8))


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
	var tall := 112.0 + (60.0 if t is Ship and not t.systems.list.is_empty() else 0.0)
	draw_rect(Rect2(x - 10, y - 4, 320, tall), Color(0.02, 0.04, 0.08, 0.6))
	if t is Ship:
		draw_string(font, Vector2(x, y + 14), "%s   %s %s" % [t.pilot_name, GameData.race(t.race_id)["adj"], t.design_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, col)
		draw_string(font, Vector2(x, y + 34), "%s   range %d" % [GameData.rank_name(t.rank), int(t.global_position.distance_to(p.global_position))], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.9, 0.9, 0.9))
		_bar(Vector2(x, y + 44), 300, 18, t.shield / maxf(t.shield_max, 1.0), Color(0.3, 0.6, 1.0, 0.85), "Shields", "%d" % int(t.shield))
		_bar(Vector2(x, y + 66), 300, 18, t.hull / maxf(t.hull_max, 1.0), Color(0.95, 0.55, 0.25, 0.85), "Hull", "%d" % int(t.hull))
		if not vis:
			draw_string(font, Vector2(x, y + 102), "Contact lost", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1, 0.7, 0.5))
		elif not t.systems.list.is_empty():
			_draw_systems(t, Vector2(x, y + 90), 300)
			var line := "Crew %d/%d" % [int(ceil(t.crew)), t.crew_max]
			var lcol := Color(0.9, 0.9, 0.9)
			if p.boarding == t:
				line += "   BOARDING %d%%   (K calls it off)" % int(p.board_progress * 100.0)
				lcol = Color(0.55, 0.9, 1.0)
				_bar(Vector2(x, y + 132), 300, 10, p.board_progress, Color(0.5, 0.85, 1.0, 0.9), "", "")
			elif t.is_disabled() and t.team() != p.team():
				var why := Capture.why_not(p, t)
				lcol = Color(1.0, 0.6, 0.3) if fmod(time * 2.0, 1.0) < 0.7 else Color(1.0, 0.85, 0.5)
				if why == "far":
					line += "   DISABLED: close to %d, then K" % int(Capture.board_range(p, t))
				else:
					line += "   DISABLED: " + ("K to board" if why == "" else Capture.reason_text(why, p, t))
			draw_string(font, Vector2(x, y + 126), line, HORIZONTAL_ALIGNMENT_LEFT, 300, 13, lcol)


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
	rows.append(["Shift-click it again to close", "hint"])
	var w := 0.0
	for row in rows:
		w = maxf(w, font.get_string_size(row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 18 if row[1] == "title" else 14).x)
	var size := Vector2(w + 28.0, 18.0 + rows.size() * 20.0 + 8.0)
	var at := screen + Vector2(r + 24.0, -size.y * 0.5)
	if at.x + size.x > vp.x - 16.0:
		at.x = screen.x - r - 24.0 - size.x
	at.x = clampf(at.x, 16.0, vp.x - size.x - 16.0)
	at.y = clampf(at.y, 70.0, vp.y - size.y - 60.0)
	var box := Rect2(at, size)
	draw_rect(box, Color(0.02, 0.04, 0.1, 0.88))
	draw_rect(box, Color(0.5, 0.7, 1.0, 0.6), false, 1.5)
	if pl.is_colonised():
		draw_rect(Rect2(at, Vector2(4.0, size.y)), GameData.race(pl.owner_race)["color"])
	var y := at.y + 26.0
	for row in rows:
		var col: Color = {"title": Color(1, 1, 1), "sub": Color(0.6, 0.72, 0.9), "good": Color(0.55, 1.0, 0.65),
			"warn": Color(1.0, 0.7, 0.3), "hint": Color(0.6, 0.7, 0.85, 0.7)}.get(row[1], Color(0.88, 0.92, 1.0))
		draw_string(font, Vector2(at.x + 14.0, y), row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 18 if row[1] == "title" else 14, col)
		y += 22.0 if row[1] == "title" else 20.0


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
			col = GameData.race(pl.owner_race)["color"]
		draw_circle(c + d, pr, col)
	for s in world.ships:
		if s.dead or s == p:
			continue
		if not s.visible_to(p):
			continue
		var d := (s.global_position - p.global_position) * scale
		if d.length() > r:
			continue
		var col := s.race_color()
		var hostile := s.team() != p.team()
		if s.is_station:
			draw_rect(Rect2(c + d - Vector2(4, 4), Vector2(8, 8)), col)
		else:
			draw_circle(c + d, 3.0, col)
		if hostile and not s.is_station:
			draw_arc(c + d, 5.0, 0, TAU, 12, Color(1.0, 0.25, 0.2, 0.9), 1.2)
		if s == p.target:
			draw_arc(c + d, 7.0, 0, TAU, 16, Color.WHITE, 1.5)
	# own ship
	var fwd := Vector2.RIGHT.rotated(p.rotation)
	draw_line(c, c + fwd * 10.0, Color.WHITE, 2.0)
	draw_circle(c, 3.0, Color.WHITE)
	draw_string(font, c + Vector2(-r, r + 22), "Radar %d   +/- zoom" % int(radar_range), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.6, 0.9, 0.7, 0.8))


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
		dock_text = "In docking range:  U  opens the shipyard   %s  cargo and station kits" % Keybinds.short_text("cargo") + ("" if p.speed() <= 220.0 else "  (brake first)")
	elif not p.dead and not p.is_landing():
		var port = world.cargo_port(p)
		if port != null and p.cargo_max > 0.0:
			dock_text = "Alongside %s:  %s  moves cargo" % [world.port_name(port), Keybinds.short_text("cargo")]
		elif p.kit != "":
			dock_text = "%s kit aboard:  %s  deploys it here" % [GameData.STATIONS[p.kit]["label"], Keybinds.short_text("cargo")]
	if dock_text != "":
		# on its own dark plate low on the screen, so the starbase art never hides it
		var plate := Rect2(vp.x * 0.5 - 330, vp.y - 98, 660, 32)
		draw_rect(plate, Color(0.02, 0.06, 0.05, 0.78))
		draw_rect(plate, Color(0.4, 1.0, 0.7, 0.55), false, 1.5)
		draw_string_outline(font, Vector2(plate.position.x, plate.position.y + 22), dock_text, HORIZONTAL_ALIGNMENT_CENTER, plate.size.x, 16, 4, Color(0, 0, 0, 0.85))
		draw_string(font, Vector2(plate.position.x, plate.position.y + 22), dock_text, HORIZONTAL_ALIGNMENT_CENTER, plate.size.x, 16, Color(0.7, 1.0, 0.8, 0.85 + 0.15 * sin(time * 4.0)))
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
		line = "Uninhabited: supports %s %s" % [GameData.fmt_pop(b.optimal_population(p.race_id)), GameData.race(p.race_id)["name"] + "s"]
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
		var txt := "The %s hold every colony: %d s to victory" % [GameData.race(rd.hold_race)["name"], int(ceil(left))]
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
		Rect2(vp.x - 345, 0, 345, 140),            # target panel
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
		var hostile: bool = p.target.team() != p.team()
		marks.append([p.target, Color(1.0, 0.35, 0.3) if hostile else Color(1.0, 0.9, 0.4), p.target.pilot_name])
	var base: Ship = world.starbase_of(p.race_id)
	if base != null and base != p.target:
		marks.append([base, Color(0.45, 1.0, 0.6), "Starbase"])
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
	if hover != null and hover != p and hover.team() != p.team():
		col = Color(1.0, 0.45, 0.4, 0.95)
	draw_arc(screen, 11.0, 0.0, TAU, 24, col, 1.5)
	for d in [Vector2.LEFT, Vector2.RIGHT, Vector2.UP, Vector2.DOWN]:
		draw_line(screen + d * 6.0, screen + d * 16.0, col, 1.5)


func _draw_dead(vp: Vector2) -> void:
	var a := clampf(dead_time, 0.0, 1.0)
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.3, 0.0, 0.0, 0.25 * a))
	var killer: String = world.duel_rules.killed_by if world.duel_rules != null else ""
	draw_string(font, Vector2(0, vp.y * 0.4), "Your ship was captured" if world.player.captured else ("Destroyed by %s" % killer if killer != "" else "Your ship was destroyed"), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 40, Color(1, 0.8, 0.7, a))
	var left := maxf(World.RESPAWN_DELAY - dead_time, 0.0)
	draw_string(font, Vector2(0, vp.y * 0.4 + 40), "Relaunching in %d..." % int(ceil(left)), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 20, Color(1, 1, 1, a))


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
		[_keys("boost"), "boost (with thrust)"], [_keys("brake"), "brake"], [_keys("warp"), "quantum drive (warp)"],
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
		[_keys("weapon_group_next"), "next weapon group"], [_keys("cloak"), "cloak"],
		["", ""],
		["TARGETS AND NAVIGATION", ""],
		[_keys("target_next"), "next target"], [_keys("target_nearest"), "nearest enemy"], ["Shift+click", "target a ship, world info"],
		[_keys("tactical_map"), "system map (hold or tap)"], [_keys("nav_next"), "next nav planet"], [_keys("autopilot"), "autopilot to the nav planet"],
		[_keys("ledger"), "empire ledger: resources"],
		["", ""],
		["SHIP", ""],
		[_keys("board"), "board a disabled target"], [_keys("refit"), "shipyard, at your starbase"], [_keys("trade"), "trade with your target (online)"],
		[_keys("cargo"), "cargo: colonies, stations, station kits"],
	]
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
	draw_string(font, Vector2(x, fy + 22), "Capture enemy ships: shoot out engines or reactor, then board with %s." % Keybinds.short_text("board"), HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
	draw_string(font, Vector2(x, fy + 44), "Wrecks drop parts and blueprints.  Dock at your starbase (%s) to refit and set weapon groups." % Keybinds.short_text("refit"), HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
	draw_string(font, Vector2(x, fy + 66), "Change any key or gamepad button in Settings > Controls.", HORIZONTAL_ALIGNMENT_CENTER, w, 14, Color(0.7, 0.8, 0.95))
