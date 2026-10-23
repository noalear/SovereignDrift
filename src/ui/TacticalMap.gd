## TacticalMap -- the whole system on one screen (Tab).  Planets in their
## owners' colours, friendly ships always, enemies where a friendly sensor
## can see them, the round clock, the planet tally and the pilot roster.
## Click a planet to make it the nav target.
class_name TacticalMap
extends Control

var world: World
var font: Font
var time := 0.0
var hover_planet: Planet = null

const PANEL := Color(0.01, 0.02, 0.06, 0.9)
const TEXT := Color(0.9, 0.95, 1.0)
const DIM := Color(0.55, 0.65, 0.8)


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	font = ThemeDB.fallback_font


func _process(delta: float) -> void:
	time += delta
	queue_redraw()


# ---------------------------------------------------------------- layout

func _map_rect() -> Rect2:
	var vp := get_viewport_rect().size
	var side := minf(vp.y - 80.0, vp.x - 440.0)
	return Rect2(30.0, 40.0, side, side)


func _scale() -> float:
	var extent := 0.0
	for p in world.planets:
		extent = maxf(extent, p.global_position.length() + 400.0)
	extent = maxf(extent, GameData.UNIVERSE_RADIUS * 0.47)
	return _map_rect().size.x * 0.5 / extent


func to_map(pos: Vector2) -> Vector2:
	return _map_rect().get_center() + pos * _scale()


func planet_at(screen: Vector2) -> Planet:
	var best: Planet = null
	var best_d := 18.0
	for p in world.planets:
		var d := to_map(p.global_position).distance_to(screen)
		if d < best_d:
			best_d = d
			best = p
	return best


# ----------------------------------------------------------------- input

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		hover_planet = planet_at(event.position)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var p := planet_at(event.position)
		if p != null and world.player != null:
			world.player.nav_target = p
			world.notify(world.player, "Nav target: %s.  G flies there." % p.body_name)
			Sfx.play_ui("click", 0.4)
		accept_event()


# ------------------------------------------------------------------ draw

func _draw() -> void:
	if world == null:
		return
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.01, 0.03, 0.82))
	var mr := _map_rect()
	draw_rect(mr, PANEL)
	draw_rect(mr, Color(0.4, 0.6, 0.9, 0.5), false, 1.2)
	var s := _scale()
	var c := mr.get_center()
	var player := world.player
	var friendly_race: int = player.race_id if player != null else -1
	# orbits, faint
	for p in world.planets:
		if p.orbit_radius <= 0.0 or p.body_class in ["Moon", "Minor"]:
			continue
		var centre := to_map(p.parent_body.global_position if p.parent_body != null else Vector2.ZERO)
		draw_arc(centre, p.orbit_radius * s, 0, TAU, 96, Color(0.3, 0.4, 0.6, 0.18), 1.0, true)
	# the asteroid belt as one soft band
	draw_arc(c, GameData.UNIVERSE_RADIUS * 0.268 * s, 0, TAU, 128, Color(0.5, 0.45, 0.35, 0.07), GameData.UNIVERSE_RADIUS * 0.03 * s, true)
	# planets
	for p in world.planets:
		var pos := to_map(p.global_position)
		var r := clampf(p.radius * s * 2.2, 2.0, 13.0)
		if p.look == "Sun":
			draw_circle(pos, 9.0, Color(1.0, 0.85, 0.4))
			draw_circle(pos, 14.0, Color(1.0, 0.7, 0.3, 0.25))
			continue
		var col := Color(0.45, 0.45, 0.5)
		if p.is_colonised():
			col = GameData.race(p.owner_race)["color"]
			draw_circle(pos, r + 3.0, Color(col.r, col.g, col.b, 0.25))
		draw_circle(pos, r, col)
		if p.under_attack > 0.0:
			draw_arc(pos, r + 6.0 + 2.0 * sin(time * 8.0), 0, TAU, 24, Color(1.0, 0.4, 0.2, 0.9), 1.5, true)
		if p == hover_planet or (player != null and p == player.nav_target):
			draw_arc(pos, r + 8.0, 0, TAU, 32, Color.WHITE, 1.2, true)
		if p.is_colonised() or p.body_class == "Planet" or p == hover_planet:
			draw_string(font, pos + Vector2(r + 4.0, 4.0), p.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0.85, 0.9, 1.0, 0.85 if p.is_colonised() else 0.5))
	# what your own sensors reach: ships show only inside this circle;
	# starbases are charted and always on the map
	if player != null and not player.dead:
		var sp := to_map(player.global_position)
		var sr := player.sensor_range * s
		draw_circle(sp, sr, Color(0.4, 0.8, 0.6, 0.06))
		draw_arc(sp, sr, 0, TAU, 48, Color(0.4, 0.8, 0.6, 0.45), 1.0, true)
	for sh in world.ships:
		if sh.dead:
			continue
		var seen: bool = sh.is_station or (player != null and not player.dead and sh.visible_to(player))
		if not seen:
			continue
		var pos := to_map(sh.global_position)
		var col := sh.race_color()
		if sh.is_station:
			draw_rect(Rect2(pos - Vector2(4, 4), Vector2(8, 8)), col)
			draw_rect(Rect2(pos - Vector2(6, 6), Vector2(12, 12)), Color(col.r, col.g, col.b, 0.6), false, 1.0)
		else:
			var fwd := Vector2.RIGHT.rotated(sh.rotation)
			var side := fwd.orthogonal()
			var sz := 4.0 + sh.size * 0.05
			draw_colored_polygon(PackedVector2Array([pos + fwd * sz * 1.6, pos - fwd * sz + side * sz, pos - fwd * sz - side * sz]), col)
		if sh == player:
			draw_arc(pos, 9.0, 0, TAU, 24, Color.WHITE, 1.5, true)
		elif sh == (player.target if player != null else null):
			draw_arc(pos, 8.0, 0, TAU, 16, Color(1, 0.9, 0.5), 1.2, true)
	# nav line
	if player != null and player.nav_target != null and is_instance_valid(player.nav_target):
		_dashed(to_map(player.global_position), to_map(player.nav_target.global_position), Color(0.6, 0.9, 1.0, 0.7))
	# the right column
	_draw_side(Rect2(mr.end.x + 20.0, mr.position.y, vp.x - mr.end.x - 50.0, mr.size.y))
	draw_string(font, Vector2(mr.position.x, mr.end.y + 24.0), "TACTICAL MAP   ships show inside your sensor range   click a planet to set the nav target   Tab closes", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)


func _dashed(a: Vector2, b: Vector2, col: Color) -> void:
	var d := b - a
	var n := int(d.length() / 10.0)
	for i in n:
		if i % 2 == 0:
			draw_line(a + d * (float(i) / n), a + d * (float(i + 1) / n), col, 1.0)


func _draw_side(r: Rect2) -> void:
	draw_rect(r, PANEL)
	draw_rect(r, Color(0.4, 0.6, 0.9, 0.5), false, 1.2)
	var rd: Round = world.round_rules
	var x := r.position.x + 16.0
	var y := r.position.y + 30.0
	if rd != null:
		var phase_text: String = {"opening": "Opening", "war": "War", "final": "FINAL PHASE", "sudden_death": "SUDDEN DEATH", "over": "Round over"}.get(rd.phase, "")
		draw_string(font, Vector2(x, y), ("Time left  %s" if rd.length > 0.0 else "Round time  %s") % rd.clock_text(), HORIZONTAL_ALIGNMENT_LEFT, -1, 22, TEXT)
		draw_string(font, Vector2(x, y + 24), phase_text, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1, 0.8, 0.4) if rd.phase in ["final", "sudden_death"] else DIM)
		y += 56
		draw_string(font, Vector2(x, y), "PLANETS HELD", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.6, 0.8, 1.0))
		y += 10
		var counts := rd.planet_counts()
		var total := 0
		for k in counts:
			total += counts[k]
		var rows := rd.standings()
		for row in rows:
			var col: Color = GameData.race(row["race"])["color"]
			y += 22
			draw_string(font, Vector2(x, y), GameData.race(row["race"])["name"], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, col)
			var bw := r.size.x - 150.0
			draw_rect(Rect2(x + 100, y - 11, bw, 12), Color(0, 0, 0, 0.5))
			draw_rect(Rect2(x + 100, y - 11, bw * float(row["planets"]) / maxf(total, 1.0), 12), col)
			draw_string(font, Vector2(x + 100, y), str(row["planets"]), HORIZONTAL_ALIGNMENT_RIGHT, bw - 4, 12, Color.WHITE)
		if counts.get(3, 0) > 0:
			y += 22
			draw_string(font, Vector2(x, y), "Pirates", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, GameData.race(3)["color"])
			draw_string(font, Vector2(x + 100, y), str(counts[3]), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, DIM)
		y += 36
	# the roster
	draw_string(font, Vector2(x, y), "PILOTS", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.6, 0.8, 1.0))
	y += 8
	for race in [0, 1, 2, 3]:
		var names := []
		for sh in world.ships:
			if sh.race_id == race and not sh.is_station:
				names.append(sh)
		if names.is_empty():
			continue
		names.sort_custom(func(a, b): return a.score > b.score)
		y += 20
		draw_string(font, Vector2(x, y), GameData.race(race)["name"], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, GameData.race(race)["color"])
		for sh in names:
			y += 16
			if y > r.end.y - 10:
				return
			var line := "%s  %s" % [sh.pilot_name, sh.design_name]
			var col := TEXT if not sh.dead else Color(0.5, 0.5, 0.55)
			if sh.is_player:
				col = Color(1, 0.95, 0.7)
			draw_string(font, Vector2(x + 10, y), line, HORIZONTAL_ALIGNMENT_LEFT, r.size.x - 110, 12, col)
			var k: int = rd.round_kills(sh) if rd != null else sh.kills
			draw_string(font, Vector2(x, y), "%d" % k, HORIZONTAL_ALIGNMENT_RIGHT, r.size.x - 30, 12, DIM)
