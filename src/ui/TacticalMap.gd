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
var station_rects: Array[Rect2] = []   # starbase squares, kept clear of labels

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


## Planets and the Sun win a click over their moons: a moon is picked
## only when the click is closer to it and clear of any planet.
func planet_at(screen: Vector2) -> Planet:
	var best: Planet = null
	var best_d := 26.0
	for p in world.planets:
		if p.body_class in ["Moon", "Minor"]:
			continue
		var d := to_map(p.global_position).distance_to(screen)
		if d < best_d:
			best_d = d
			best = p
	if best != null and best_d < 12.0:
		return best
	var moon: Planet = null
	var moon_d := minf(best_d, 14.0) if best != null else 14.0
	for p in world.planets:
		if not p.body_class in ["Moon", "Minor"]:
			continue
		var d := to_map(p.global_position).distance_to(screen)
		if d < moon_d:
			moon_d = d
			moon = p
	return moon if moon != null else best


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
	# planets; labels are placed afterwards so they can dodge each other
	var labels := []
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
		if p.is_defenceless():
			# defences down: open for invasion
			for i in 6:
				var a0 := TAU * i / 6.0 + time
				draw_arc(pos, r + 4.0, a0, a0 + 0.5, 6, Color(1.0, 0.9, 0.5, 0.9), 1.5, true)
		if p == hover_planet or (player != null and p == player.nav_target):
			draw_arc(pos, r + 8.0, 0, TAU, 32, Color.WHITE, 1.2, true)
		if p.is_colonised() or p.body_class == "Planet" or p == hover_planet:
			labels.append([p, pos, r])
	# what your own sensors reach: ships show only inside this circle;
	# starbases are charted and always on the map
	if player != null and not player.dead:
		var sp := to_map(player.global_position)
		var sr := player.sensor_range * s
		draw_circle(sp, sr, Color(0.4, 0.8, 0.6, 0.06))
		draw_arc(sp, sr, 0, TAU, 48, Color(0.4, 0.8, 0.6, 0.45), 1.0, true)
	station_rects.clear()
	var ship_marks: Array[Rect2] = []
	for sh in world.ships:
		if sh.dead:
			continue
		var seen: bool = sh.is_station or (player != null and not player.dead and sh.visible_to(player))
		if not seen:
			continue
		var pos := to_map(sh.global_position)
		var col := sh.race_color()
		if sh == player and not mr.grow(-8).has_point(pos):
			# off the chart after a long warp: pin an arrow to the edge
			var dir := (pos - c).normalized()
			var half := mr.size * 0.5 - Vector2(14, 14)
			var k := minf(absf(half.x / maxf(absf(dir.x), 0.001)), absf(half.y / maxf(absf(dir.y), 0.001)))
			var at := c + dir * k
			var sd := dir.orthogonal()
			draw_colored_polygon(PackedVector2Array([at + dir * 12.0, at - dir * 6.0 + sd * 8.0, at - dir * 6.0 - sd * 8.0]), Color.WHITE)
			draw_string(font, at - dir * 22.0 + Vector2(-40, 4), "You", HORIZONTAL_ALIGNMENT_CENTER, 80, 12, Color.WHITE)
			continue
		if not mr.has_point(pos):
			continue
		if sh.is_station:
			station_rects.append(Rect2(pos - Vector2(7, 7), Vector2(14, 14)))
			draw_rect(Rect2(pos - Vector2(4, 4), Vector2(8, 8)), col)
			draw_rect(Rect2(pos - Vector2(6, 6), Vector2(12, 12)), Color(col.r, col.g, col.b, 0.6), false, 1.0)
		else:
			var fwd := Vector2.RIGHT.rotated(sh.rotation)
			var side := fwd.orthogonal()
			var sz := 4.0 + sh.size * 0.05
			draw_colored_polygon(PackedVector2Array([pos + fwd * sz * 1.6, pos - fwd * sz + side * sz, pos - fwd * sz - side * sz]), col)
			ship_marks.append(Rect2(pos - Vector2(9, 9), Vector2(18, 18)))
		if sh == player:
			draw_arc(pos, 9.0, 0, TAU, 24, Color.WHITE, 1.5, true)
		elif sh == (player.target if player != null else null):
			draw_arc(pos, 8.0, 0, TAU, 16, Color(1, 0.9, 0.5), 1.2, true)
	_draw_labels(labels, ship_marks)
	_draw_world_card()
	# nav line
	if player != null and player.nav_target != null and is_instance_valid(player.nav_target):
		_dashed(to_map(player.global_position), to_map(player.nav_target.global_position), Color(0.6, 0.9, 1.0, 0.7))
	# the right column
	_draw_side(Rect2(mr.end.x + 20.0, mr.position.y, vp.x - mr.end.x - 50.0, mr.size.y))
	draw_string(font, Vector2(mr.position.x, mr.end.y + 24.0), "TACTICAL MAP   ships show inside your sensor range   click a planet to set the nav target   Tab closes", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)


## Planet names: planets first, then moons, each tried right, left, below
## and above its dot; a moon whose name fits nowhere is left unlabelled
## until hovered.
func _draw_labels(labels: Array, ship_marks: Array[Rect2] = []) -> void:
	labels.sort_custom(func(a, b): return (0 if a[0].body_class == "Planet" else 1) < (0 if b[0].body_class == "Planet" else 1))
	# names keep off starbases and ship markers as well as other names
	var taken: Array[Rect2] = station_rects.duplicate()
	taken.append_array(ship_marks)
	for e in labels:
		var p: Planet = e[0]
		var pos: Vector2 = e[1]
		var r: float = e[2]
		taken.append(Rect2(pos - Vector2(r, r), Vector2(r, r) * 2.0))
	for e in labels:
		var p: Planet = e[0]
		var pos: Vector2 = e[1]
		var r: float = e[2]
		var w := font.get_string_size(p.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 12).x
		var spots := [Vector2(r + 4.0, 4.0), Vector2(-r - 4.0 - w, 4.0), Vector2(-w * 0.5, r + 14.0), Vector2(-w * 0.5, -r - 5.0)]
		for off in spots:
			var rect := Rect2(pos + off + Vector2(0, -11), Vector2(w, 13))
			var clash := false
			for t in taken:
				if t.intersects(rect) and not t.has_point(pos):
					clash = true
					break
			if clash and p != hover_planet:
				continue
			taken.append(rect)
			draw_string(font, pos + off, p.body_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, Color(0.85, 0.9, 1.0, 0.9 if p.is_colonised() else 0.62))
			break


## The hovered world: who lives there, how many, and room for more.
func _draw_world_card() -> void:
	var p := hover_planet
	if p == null or p.look == "Sun":
		return
	var player: Ship = world.player
	var rows: Array = p.info_lines(player.race_id if player != null else 1)
	var at := to_map(p.global_position) + Vector2(18, 14)
	var w := 0.0
	for row in rows:
		w = maxf(w, font.get_string_size(row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 12).x)
	var box := Rect2(at, Vector2(w + 16.0, rows.size() * 16.0 + 8.0))
	if box.end.x > size.x - 8.0:
		box.position.x = to_map(p.global_position).x - 18.0 - box.size.x
	box.position.y = clampf(box.position.y, 8.0, size.y - box.size.y - 8.0)
	at = box.position
	draw_rect(box, PANEL)
	draw_rect(box, Color(0.4, 0.6, 0.9, 0.5), false, 1.0)
	for i in rows.size():
		var col: Color = {"title": Color(0.9, 0.95, 1.0), "good": Color(0.55, 1.0, 0.65), "warn": Color(1.0, 0.7, 0.3)}.get(rows[i][1], DIM)
		draw_string(font, at + Vector2(8, 18 + i * 16), rows[i][0], HORIZONTAL_ALIGNMENT_LEFT, -1, 12, col)


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
			# pirates hold colonies but cannot win: a dim bar on the same scale
			var pc: Color = GameData.race(3)["color"]
			var bw := r.size.x - 150.0
			draw_rect(Rect2(x + 100, y - 11, bw, 12), Color(0, 0, 0, 0.5))
			draw_rect(Rect2(x + 100, y - 11, bw * float(counts[3]) / maxf(total, 1.0), 12), Color(pc.r, pc.g, pc.b, 0.45))
			draw_string(font, Vector2(x + 100, y), str(counts[3]), HORIZONTAL_ALIGNMENT_RIGHT, bw - 4, 12, Color.WHITE)
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
