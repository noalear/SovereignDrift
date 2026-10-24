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
	_draw_status(p, vp)
	_draw_weapons(p, vp)
	_draw_target(p, vp)
	_draw_radar(p, vp)
	_draw_messages(vp)
	_draw_rank(p, vp)
	_draw_nav(p, vp)
	_draw_music(vp)
	_draw_aim(p, vp)
	_draw_pointers(p, vp)
	_draw_heat(p, vp)
	if p.dead:
		_draw_dead(vp)
	if show_help:
		_draw_help(vp)
	else:
		draw_string(font, Vector2(vp.x * 0.5 - 150, vp.y - 14), "H: help   Esc: menu", HORIZONTAL_ALIGNMENT_CENTER, 300, 14, Color(1, 1, 1, 0.45))


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
	if p.inputs["boost"] and p.has_boost and p.inputs["thrust"] > 0.0:
		flags.append("BOOST")
	draw_string(font, Vector2(x, y + 106), "Speed %4d   %s" % [int(p.local_velocity().length()), "  ".join(flags)], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.9, 0.95, 1.0))


func _draw_weapons(p: Ship, vp: Vector2) -> void:
	var x := 350.0
	var y := vp.y - 130.0
	var lines := []
	var seen := {}
	for w in p.weapons:
		var key: String = w["slot"]
		if seen.has(key):
			seen[key] += 1
			continue
		seen[key] = 1
	var keymap := {"primary": "Space", "secondary": "Ctrl", "missile": "F", "mine": "M", "beam": "L"}
	for w in p.weapons:
		var key: String = w["slot"]
		if seen.get(key, 0) == 0:
			continue
		var count: int = seen[key]
		seen[key] = 0
		var label: String = GameData.missile_display_name(p.race_id, w["missile"]) if w["kind"] == "tube" else w["name"]
		var ammo := ""
		if w["ammo"] >= 0:
			ammo = "  x%d" % w["ammo"]
		var ready: bool = w["cooldown"] <= 0.0 and p.energy >= w["cost"]
		lines.append(["%s  %s%s%s" % [keymap.get(key, "?"), label, ("  (%d)" % count) if count > 1 else "", ammo], ready])
	draw_rect(Rect2(x - 10, y - 34, 260, 134), Color(0.02, 0.04, 0.08, 0.6))
	draw_string(font, Vector2(x, y - 12), "Weapons", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(1, 1, 1, 0.9))
	var ly := y + 10
	for l in lines:
		draw_string(font, Vector2(x, ly), l[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.9, 1.0, 0.9) if l[1] else Color(0.6, 0.6, 0.6))
		ly += 18
	var sys := []
	if p.can_cloak:
		sys.append("C cloak")
	if p.qdrv > 0.0:
		sys.append("J quantum drive")
	if p.has_boost:
		sys.append("Shift boost")
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
	if t is Ship and t.race_id == p.race_id:
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
	draw_rect(Rect2(x - 10, y - 4, 320, 112), Color(0.02, 0.04, 0.08, 0.6))
	if t is Ship:
		draw_string(font, Vector2(x, y + 14), "%s   %s %s" % [t.pilot_name, GameData.race(t.race_id)["adj"], t.design_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, col)
		draw_string(font, Vector2(x, y + 34), "%s   range %d" % [GameData.rank_name(t.rank), int(t.global_position.distance_to(p.global_position))], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.9, 0.9, 0.9))
		_bar(Vector2(x, y + 44), 300, 18, t.shield / maxf(t.shield_max, 1.0), Color(0.3, 0.6, 1.0, 0.85), "Shields", "%d" % int(t.shield))
		_bar(Vector2(x, y + 66), 300, 18, t.hull / maxf(t.hull_max, 1.0), Color(0.95, 0.55, 0.25, 0.85), "Hull", "%d" % int(t.hull))
		if not vis:
			draw_string(font, Vector2(x, y + 102), "Contact lost", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1, 0.7, 0.5))


func _draw_nav(p: Ship, vp: Vector2) -> void:
	var t := p.nav_target
	if t == null or not is_instance_valid(t):
		return
	var cam := world.camera
	var screen := (world.drawn_position(t) - world.camera_centre()) * cam.zoom + vp * 0.5
	var col := Color(0.6, 1.0, 0.8)
	var dist := int(t.global_position.distance_to(p.global_position) - t.radius)
	if screen.x > -50 and screen.x < vp.x + 50 and screen.y > -50 and screen.y < vp.y + 50:
		var r: float = t.radius * cam.zoom.x + 14.0
		draw_arc(screen, r, 0, TAU, 48, Color(col.r, col.g, col.b, 0.5), 1.5, true)
		draw_string(font, screen + Vector2(-40, -r - 6), "%s  %d" % [t.body_name, dist], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, col)
	else:
		var dir := (screen - vp * 0.5).normalized()
		var edge := _edge_anchor(dir, vp)
		draw_circle(edge, 5.0, col)
		var lpos := edge - dir * 30.0 + Vector2(-64, 5)
		draw_string_outline(font, lpos, "%s  %d" % [t.body_name, dist], HORIZONTAL_ALIGNMENT_CENTER, 128, 13, 3, Color(0, 0, 0, 0.8))
		draw_string(font, lpos, "%s  %d" % [t.body_name, dist], HORIZONTAL_ALIGNMENT_CENTER, 128, 13, col)


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
		var hostile := s.race_id != p.race_id
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
	var rank := GameData.rank_name(p.rank)
	var next := ""
	if p.rank + 1 < GameData.RANKS.size():
		var nr: Dictionary = GameData.RANKS[p.rank + 1]
		next = "   next: %d pts, %d kills" % [nr["score"], nr["kills"]]
	_text(Vector2(x, y), "%s %s   score %d   kills %d%s" % [rank, p.pilot_name, p.score, p.kills, next], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 0.95, 0.8))
	_text(Vector2(x, y + 20), "credits %d   hold %d/%d%s" % [p.credits, p.hold.size(), Ship.HOLD_SIZE, ("   blueprints %d" % p.blueprints.size()) if not p.blueprints.is_empty() else ""], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1.0, 0.85, 0.4))
	# docking prompt
	if not p.dead and world.nearest_base(p) != null:
		# on its own dark plate low on the screen, so the starbase art never hides it
		var dock_text := "In docking range:  U  opens the shipyard" + ("" if p.speed() <= 220.0 else "  (brake first)")
		var plate := Rect2(vp.x * 0.5 - 250, vp.y - 98, 500, 32)
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
		var hostile: bool = p.target.race_id != p.race_id
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
	if hover != null and hover != p and hover.race_id != p.race_id:
		col = Color(1.0, 0.45, 0.4, 0.95)
	draw_arc(screen, 11.0, 0.0, TAU, 24, col, 1.5)
	for d in [Vector2.LEFT, Vector2.RIGHT, Vector2.UP, Vector2.DOWN]:
		draw_line(screen + d * 6.0, screen + d * 16.0, col, 1.5)


func _draw_dead(vp: Vector2) -> void:
	var a := clampf(dead_time, 0.0, 1.0)
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.3, 0.0, 0.0, 0.25 * a))
	draw_string(font, Vector2(0, vp.y * 0.4), "Your ship was destroyed", HORIZONTAL_ALIGNMENT_CENTER, vp.x, 40, Color(1, 0.8, 0.7, a))
	var left := maxf(World.RESPAWN_DELAY - dead_time, 0.0)
	draw_string(font, Vector2(0, vp.y * 0.4 + 40), "Relaunching in %d..." % int(ceil(left)), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 20, Color(1, 1, 1, a))


func _draw_help(vp: Vector2) -> void:
	var left := [
		["FLIGHT", ""],
		["W / Up", "thrust"], ["S / Down", "reverse"], ["A D / Left Right", "turn"], ["Q E", "strafe"],
		["Shift", "boost (with thrust)"], ["X", "brake"], ["J", "quantum drive (warp)"],
		["Mouse", "aims the turrets"], ["V", "flight assist on / off"],
		["", ""],
		["SYSTEM", ""],
		["H / F1", "this card"], ["Esc", "menu"], ["F9", "next song"], ["Alt+Enter", "full screen"],
		["Mouse wheel", "camera zoom"], ["+ / -", "radar zoom"],
		["", ""],
		["GAMEPAD", ""],
		["Sticks", "left flies, right aims"], ["RT / LT", "torpedoes / secondary"],
		["A B X Y", "boost, brake, missile, mine"], ["Bumpers", "strafe"], ["D-pad", "targets, nav, autopilot"],
	]
	var right := [
		["WEAPONS", ""],
		["Space / LMB", "torpedoes"], ["Ctrl / RMB", "plasma, grenades, cannon"], ["F", "seeking missiles"],
		["M", "drop a mine"], ["L", "laser"], ["C", "cloak"],
		["", ""],
		["TARGETS AND NAVIGATION", ""],
		["T", "next target"], ["Tab / Back", "tactical map (hold or tap)"], ["R", "nearest enemy"], ["Shift+click", "target under cursor"],
		["N", "next planet on the nav display"], ["G", "autopilot to the nav planet"], ["U", "shipyard, at your starbase"], ["O", "trade with the targeted pilot (online)"],
	]
	var rows := maxi(left.size(), right.size())
	var w := 980.0
	var h := 70.0 + rows * 22.0 + 50.0
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
				draw_string(font, Vector2(col[1], ly), key, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1, 1, 1))
				draw_string(font, Vector2(col[1] + 150.0, ly), desc, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.85, 0.9, 1.0))
			ly += 22
	var fy := y + 66 + rows * 22.0 + 10
	draw_string(font, Vector2(x, fy), "Destroy enemy ships and bombard enemy colonies until they surrender.", HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
	draw_string(font, Vector2(x, fy + 22), "Wrecks drop parts, credits and blueprints.  Dock at your starbase (U) to build your ship from them.  Rank up for bigger hulls.", HORIZONTAL_ALIGNMENT_CENTER, w, 15, Color(0.95, 0.97, 1.0))
