## Ledger -- the empire's resources on one screen (I): what every world of
## yours holds and makes a minute, the empire's totals, the job board (runs
## the governor wants flown, for pay) and the richest worlds nobody holds
## yet.  Click a world to make it the nav target, a job to take it.
class_name Ledger
extends Control

var world: World
var font: Font
var time := 0.0
var rows_at: Array = []        # [rect, planet] for clicks
var jobs_at: Array = []        # [rect, job id] for clicks
var hover_job := 0
var hover: Planet = null

const PANEL := Color(0.01, 0.02, 0.06, 0.92)
const TEXT := Color(0.9, 0.95, 1.0)
const DIM := Color(0.55, 0.65, 0.8)
const ROW_H := 30.0
const JOB_H := 28.0
const JOB_ROWS := 6


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	font = ThemeDB.fallback_font


func _process(delta: float) -> void:
	time += delta
	queue_redraw()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		hover = _row_at(event.position)
		hover_job = _job_at(event.position)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var id := _job_at(event.position)
		if id != 0 and world.player != null:
			world.job_command(world.player, id)
			Sfx.play_ui("click", 0.4)
			accept_event()
			return
		var p := _row_at(event.position)
		if p != null and world.player != null:
			world.player.nav_target = p
			world.notify(world.player, "Nav target: %s.  G flies there." % p.body_name)
			Sfx.play_ui("click", 0.4)
		accept_event()


func _row_at(pos: Vector2) -> Planet:
	for r in rows_at:
		if r[0].has_point(pos):
			return r[1]
	return null


func _job_at(pos: Vector2) -> int:
	for r in jobs_at:
		if r[0].has_point(pos):
			return r[1]
	return 0


## The board: the host's own, or the copy the host sends a client.
func board() -> Array:
	if Net.is_client():
		return world.job_board
	var gov: Governor = world.governors.get(_race(), null)
	return gov.pack_board() if gov != null else []


func _race() -> int:
	return world.player.race_id if world.player != null else Settings.race_id


static func _num(v: float) -> String:
	return str(int(v)) if v >= 10.0 or v == 0.0 else "%.1f" % v


func _draw() -> void:
	if world == null or world.economy == null:
		return
	rows_at.clear()
	jobs_at.clear()
	var jobs := board()
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.01, 0.03, 0.82))
	var w := minf(vp.x - 60.0, 1400.0)
	var x := (vp.x - w) * 0.5
	var y := 50.0
	var race := _race()
	var rinfo: Dictionary = GameData.race(race)
	draw_string(font, Vector2(x, y + 24), "%s empire ledger" % rinfo["adj"], HORIZONTAL_ALIGNMENT_LEFT, -1, 30, rinfo["color"])
	draw_string(font, Vector2(x, y + 50), "What your worlds hold and make a minute. Stock stays on the world that made it; supply ships will carry it. Click a world to fly there.", HORIZONTAL_ALIGNMENT_LEFT, w, 15, DIM)
	y += 74.0
	# columns: world, colony, people, then one per resource
	var cols: Array[float] = [x, x + 190.0, x + 300.0, x + 400.0]
	var res_x := x + 510.0
	var res_w := (w - 510.0) / Economy.RESOURCES.size()
	draw_rect(Rect2(x - 12, y - 6, w + 24, vp.y - y - 40), PANEL)
	draw_string(font, Vector2(cols[0], y + 20), "World", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, DIM)
	draw_string(font, Vector2(cols[1], y + 20), "Colony", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, DIM)
	draw_string(font, Vector2(cols[2], y + 20), "People", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, DIM)
	draw_string(font, Vector2(cols[3], y + 20), "Troops", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, DIM)
	for i in Economy.RESOURCES.size():
		var k: String = Economy.RESOURCES[i]
		draw_string(font, Vector2(Economy.draw_icon(self, k, res_x + i * res_w, y + 14), y + 20), Economy.NAMES[k], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Economy.COLORS[k])
	y += 30.0
	# the empire's totals
	var tot: Dictionary = world.economy.totals(race)
	draw_string(font, Vector2(cols[0], y + 21), "All worlds", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, TEXT)
	for i in Economy.RESOURCES.size():
		var k: String = Economy.RESOURCES[i]
		_cell(Vector2(res_x + i * res_w, y), tot["stock"][k], tot["rates"][k], -1.0, 18)
	y += ROW_H + 6.0
	draw_line(Vector2(x, y), Vector2(x + w, y), Color(0.4, 0.6, 0.9, 0.4), 1.0)
	y += 6.0
	var holdings: Array = world.economy.holdings(race)
	if holdings.is_empty():
		draw_string(font, Vector2(cols[0], y + 22), "Your empire holds no worlds.", HORIZONTAL_ALIGNMENT_LEFT, -1, 17, DIM)
		y += ROW_H
	var max_rows := int((vp.y - y - 260.0 - maxf(minf(jobs.size(), JOB_ROWS), 1.0) * JOB_H) / ROW_H)
	for row in holdings.slice(0, maxi(max_rows, 1)):
		var p: Planet = row["planet"]
		var rect := Rect2(x - 6, y, w + 12, ROW_H)
		rows_at.append([rect, p])
		var lit := p == hover or (world.player != null and p == world.player.nav_target)
		if lit:
			draw_rect(rect, Color(0.18, 0.3, 0.55, 0.45))
		var fighting := not p.battle.is_empty()
		var name_col := TEXT if p.silenced <= 0.0 and not fighting else Color(1.0, 0.55, 0.45)
		draw_string(font, Vector2(cols[0], y + 21), p.body_name + ("  (battle)" if fighting else ("  (down)" if p.silenced > 0.0 else "")), HORIZONTAL_ALIGNMENT_LEFT, 180, 17, name_col)
		draw_string(font, Vector2(cols[1], y + 21), p.colony_design, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, DIM)
		draw_string(font, Vector2(cols[2], y + 21), GameData.fmt_pop(p.population), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, DIM)
		# the garrison's units, and whether a planetary shield stands over them
		var troops := "%d%s" % [Ground.count(p.garrison), "  shield" if p.pshield_up() else ""]
		draw_string(font, Vector2(cols[3], y + 21), troops, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(1.0, 0.7, 0.4) if p.short.has("food") or p.short.has("fuel") else DIM)
		for i in Economy.RESOURCES.size():
			var k: String = Economy.RESOURCES[i]
			_cell(Vector2(res_x + i * res_w, y), row["stock"].get(k, 0.0), row["rates"][k], Economy.cap(p), 16)
		y += ROW_H
	if holdings.size() > max_rows and max_rows > 0:
		draw_string(font, Vector2(cols[0], y + 20), "...and %d more" % (holdings.size() - max_rows), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
		y += ROW_H
	# the job board
	y += 14.0
	draw_string(font, Vector2(cols[0], y + 20), "Job board", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, TEXT)
	draw_string(font, Vector2(cols[0] + 110, y + 20), "Runs your empire's ships are not on. Click one to take it: it pays on top of the usual pay.", HORIZONTAL_ALIGNMENT_LEFT, w - 110, 14, DIM)
	y += 28.0
	if jobs.is_empty():
		draw_string(font, Vector2(cols[0], y + 18), "Nothing posted: every run is in hand.", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
		y += JOB_H
	var me: String = world.player.pilot_name if world.player != null else ""
	for row in jobs.slice(0, JOB_ROWS):
		var rect := Rect2(x - 6, y, w + 12, JOB_H - 2)
		var mine: bool = row[4] == me and me != ""
		var other: bool = row[4] != "" and not mine
		if not other:
			jobs_at.append([rect, row[0]])
		if mine:
			draw_rect(rect, Color(0.2, 0.45, 0.3, 0.45))
		elif row[0] == hover_job and not other:
			draw_rect(rect, Color(0.18, 0.3, 0.55, 0.45))
		var col := TEXT if not other else DIM
		draw_string(font, Vector2(cols[0], y + 19), row[2], HORIZONTAL_ALIGNMENT_LEFT, w - 360, 16, col if not mine else Color(0.75, 1.0, 0.8))
		draw_string(font, Vector2(x + w - 330, y + 19), "%d credits" % row[3], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.6, 0.95, 0.6) if not other else DIM)
		var state: String = "click to drop it" if mine else ("taken by " + row[4] if other else "click to take it")
		draw_string(font, Vector2(x + w - 200, y + 19), ("Yours: " if mine else "") + state, HORIZONTAL_ALIGNMENT_LEFT, 200, 14, DIM)
		y += JOB_H
	# what is out there for the taking
	y += 14.0
	draw_string(font, Vector2(cols[0], y + 20), "Unclaimed and rich", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, TEXT)
	y += 28.0
	var open := []
	for p in world.planets:
		if p.is_colonised() or p.body_class == "Star":
			continue
		var best := 0
		for k in Economy.RAW:
			best = maxi(best, p.richness.get(k, 0))
		if best >= 3:
			open.append([p, best])
	open.sort_custom(func(a, b): return a[1] > b[1])
	var cx: float = cols[0]
	var line := 0
	for e in open.slice(0, 8):
		var p: Planet = e[0]
		var how := "skimmer" if not p.has_surface() else "settle"
		var label := "%s: %s (%s)" % [p.body_name, Economy.yields_text(p.richness), how]
		var lw: float = font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, 15).x + 30.0
		if cx + lw > x + w:
			cx = cols[0]
			line += 1
			if line > 2:
				break
		var rect := Rect2(cx - 4, y + line * 26.0, lw - 10, 24)
		rows_at.append([rect, p])
		if p == hover:
			draw_rect(rect, Color(0.18, 0.3, 0.55, 0.45))
		draw_string(font, Vector2(cx, y + line * 26.0 + 18), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM if p != hover else TEXT)
		cx += lw
	draw_string(font, Vector2(0, vp.y - 26), "%s or Esc closes the ledger" % Keybinds.short_text("ledger"), HORIZONTAL_ALIGNMENT_CENTER, vp.x, 14, Color(1, 1, 1, 0.45))


## "120  +8.5": stock, then the minute's output; at the cap it reads full.
func _cell(at: Vector2, stock: float, rate: float, cap: float, size: int) -> void:
	var have := _num(stock)
	draw_string(font, at + Vector2(0, 21), have, HORIZONTAL_ALIGNMENT_LEFT, -1, size, TEXT if stock >= 0.5 else DIM)
	var hw := font.get_string_size(have, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x
	if cap > 0.0 and stock >= cap - 0.01 and rate > 0.0:
		draw_string(font, at + Vector2(hw + 8, 21), "full", HORIZONTAL_ALIGNMENT_LEFT, -1, size - 3, Color(1.0, 0.8, 0.4))
	elif rate >= 0.05:
		draw_string(font, at + Vector2(hw + 8, 21), "+" + _num(rate), HORIZONTAL_ALIGNMENT_LEFT, -1, size - 3, Color(0.55, 0.9, 0.55))
