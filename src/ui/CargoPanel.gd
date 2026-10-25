## CargoPanel -- the cargo hold, on the ground at one of your colonies (L):
## what is aboard, what the world has, and buttons to load or unload each
## resource.  Goods unloaded where the world is short of them pay; the
## spaceport moves them a few dozen units a second.
class_name CargoPanel
extends Control

signal close_requested

var world: World
var font: Font
var time := 0.0
var buttons: Array = []        # [rect, label, callable, enabled]
var hover := -1

const W := 560.0
const ROW_H := 30.0
const PANEL := Color(0.01, 0.03, 0.07, 0.93)
const TEXT := Color(0.9, 0.95, 1.0)
const DIM := Color(0.55, 0.65, 0.8)


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_STOP


func _ship() -> Ship:
	return world.player if world != null else null


func _process(delta: float) -> void:
	time += delta
	var s := _ship()
	if s == null or s.dead or not s.is_landed():
		close_requested.emit()
		return
	var vp := get_viewport_rect().size
	var h := 196.0 + Economy.RESOURCES.size() * ROW_H
	# beside the ground panel, clear of the radar
	position = Vector2(vp.x - W - 30.0, maxf(vp.y * 0.5 - h * 0.5 - 60.0, 90.0))
	size = Vector2(W, h)
	queue_redraw()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		hover = _button_at(event.position)
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		var i := _button_at(event.position)
		if i >= 0 and buttons[i][3]:
			Sfx.play_ui("click", 0.4)
			buttons[i][2].call()
		accept_event()


func _button_at(pos: Vector2) -> int:
	for i in buttons.size():
		if buttons[i][0].has_point(pos):
			return i
	return -1


func _order(res: String, amount: float) -> void:
	world.cargo_command(_ship(), res, amount)


func _draw() -> void:
	var s := _ship()
	if s == null or not s.is_landed():
		return
	buttons.clear()
	var p := s.land_body
	var friendly := p.is_colonised() and p.owner_race == s.race_id
	draw_rect(Rect2(Vector2.ZERO, size), PANEL)
	draw_rect(Rect2(Vector2.ZERO, size), Color(0.95, 0.85, 0.45, 0.6), false, 1.5)
	draw_string(font, Vector2(16, 28), "CARGO HOLD  at %s" % p.body_name.to_upper(), HORIZONTAL_ALIGNMENT_LEFT, -1, 18, TEXT)
	# the hold
	var frac := s.cargo_total() / maxf(s.cargo_max, 1.0)
	var bar := Rect2(16, 40, W - 32, 18)
	draw_rect(bar, Color(0.1, 0.12, 0.16, 0.9))
	draw_rect(Rect2(bar.position, Vector2(bar.size.x * clampf(frac, 0.0, 1.0), bar.size.y)), Color(0.95, 0.8, 0.35, 0.85))
	var moving := "   loading..." if s.cargo_orders.values().any(func(v): return v > 0.0) else ("   unloading..." if not s.cargo_orders.is_empty() else "")
	draw_string(font, bar.position + Vector2(6, 14), "%d / %d units%s" % [int(s.cargo_total()), int(s.cargo_max), moving], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.05, 0.05, 0.08) if frac > 0.35 else TEXT)
	if not friendly:
		draw_string(font, Vector2(16, 90), "Cargo only moves through your own colonies' spaceports.", HORIZONTAL_ALIGNMENT_LEFT, W - 32, 15, Color(1.0, 0.7, 0.5))
		_footer()
		return
	# all at once
	var y := 68.0
	_button(Rect2(16, y, 160, 26), "Load all", func(): _order("all", 1.0), s.cargo_space() >= 1.0)
	_button(Rect2(186, y, 160, 26), "Unload all", func(): _order("all", -1.0), s.cargo_total() >= 1.0)
	y += 40.0
	# headings
	draw_string(font, Vector2(16, y + 4), "Resource", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(130, y + 4), "Aboard", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(206, y + 4), "Here", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	y += 10.0
	var c := Economy.cap(p)
	for k in Economy.RESOURCES:
		var aboard: float = s.cargo.get(k, 0.0)
		var here: float = p.stock.get(k, 0.0)
		draw_string(font, Vector2(16, y + 20), Economy.NAMES[k], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Economy.COLORS[k])
		draw_string(font, Vector2(130, y + 20), str(int(aboard)), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT if aboard >= 1.0 else DIM)
		draw_string(font, Vector2(206, y + 20), "%d / %d" % [int(here), int(c)], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT if here >= 1.0 else DIM)
		if Economy.need(p, k) > 0.5:
			draw_string(font, Vector2(290, y + 20), "short", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1.0, 0.75, 0.4))
		var bx := W - 16.0 - 3 * 64.0
		_button(Rect2(bx, y + 3, 58, 24), "+10", func(): _order(k, 10.0), here >= 1.0 and s.cargo_space() >= 1.0)
		_button(Rect2(bx + 64, y + 3, 58, 24), "+all", func(): _order(k, 1e6), here >= 1.0 and s.cargo_space() >= 1.0)
		_button(Rect2(bx + 128, y + 3, 58, 24), "-all", func(): _order(k, -1e6), aboard >= 1.0 and here < c - 0.5)
		y += ROW_H
	draw_string(font, Vector2(16, y + 20), "Unloading pays where the world is short of the goods", HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	draw_string(font, Vector2(16, y + 37), "and makes them worse than the world you loaded them at.", HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	_footer()


func _footer() -> void:
	draw_string(font, Vector2(0, size.y - 10), "%s or Esc closes   B lifts off" % Keybinds.short_text("cargo"), HORIZONTAL_ALIGNMENT_CENTER, W, 13, Color(1, 1, 1, 0.45))


func _button(rect: Rect2, label: String, cb: Callable, enabled: bool) -> void:
	var lit := enabled and buttons.size() == hover
	var fill := Color(0.2, 0.32, 0.55, 0.9) if lit else (Color(0.1, 0.16, 0.28, 0.9) if enabled else Color(0.08, 0.09, 0.12, 0.7))
	draw_rect(rect, fill)
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.6 if enabled else 0.2), false, 1.0)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + 5), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, 14, TEXT if enabled else Color(0.5, 0.5, 0.55))
	buttons.append([rect, label, cb, enabled])
