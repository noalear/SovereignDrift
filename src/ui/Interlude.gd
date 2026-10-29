## Interlude -- between the story's operations: the screen fades to black,
## the next operation's stardate comes up with how long it has been, and
## it fades back in at the starbase with the briefing ready.
class_name Interlude
extends Control

signal midpoint()                 # all black: the days pass behind the card
signal finished()

const FADE := 1.2
const HOLD := 3.6

var index := 0
var font: Font
var t := 0.0
var _mid := false


func setup(p_index: int) -> void:
	index = clampi(p_index, 0, Story.OPS.size() - 1)


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_STOP
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)


func _process(delta: float) -> void:
	t += delta
	if not _mid and t >= FADE:
		_mid = true
		midpoint.emit()
	if t >= FADE * 2.0 + HOLD:
		finished.emit()
		queue_free()
		return
	queue_redraw()


func _draw() -> void:
	var vp := get_viewport_rect().size
	var black := clampf(t / FADE, 0.0, 1.0) if t < FADE + HOLD else clampf(1.0 - (t - FADE - HOLD) / FADE, 0.0, 1.0)
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0, 0, 0, black))
	# the card comes up once it is dark and goes before the light returns
	var a := clampf((t - FADE * 0.7) / 0.6, 0.0, 1.0) * clampf((FADE + HOLD - t) / 0.5, 0.0, 1.0)
	if a <= 0.0:
		return
	var op: Dictionary = Story.OPS[index]
	var y := vp.y * 0.5 - 70.0
	var since := Story.days_since(index)
	_line(y, "STARDATE  %s" % Story.stardate(index), 44, Color(0.75, 0.88, 1.0, a))
	if since != "":
		_line(y + 42.0, since, 20, Color(0.85, 0.85, 0.8, a * 0.85))
	_line(y + 100.0, "Operation  %s" % op["title"], 30, Color(1.0, 1.0, 0.95, a))
	_line(y + 132.0, op["where"], 18, Color(0.65, 0.72, 0.8, a * 0.9))


func _line(y: float, text: String, size: int, col: Color) -> void:
	draw_string(font, Vector2(0, y), text, HORIZONTAL_ALIGNMENT_CENTER, get_viewport_rect().size.x, size, col)
