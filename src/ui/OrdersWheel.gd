## OrdersWheel -- the wing's orders on a wheel round the screen's centre.
##
## Hold the wheel key (Y, or Y on the pad), point at an order with the
## mouse or the right stick, and let go to give it.  A tap leaves the
## wheel open: then click a slice, or press its number (1-8).  Esc, a right
## click or the wheel key again closes it.  The ship flies on meanwhile,
## and clicks or number keys spent on the wheel never reach the guns.
##
## The quick keys (attack my target, form up) work without the wheel.
class_name OrdersWheel
extends Control

const RADIUS := 170.0
const INNER := 58.0
const HOLD_MS := 280          # held longer than this, letting go gives the order
const SHORT := {"attack": "Attack target"}   # what fits on a slice

var world: World
var font: Font
var open := false
var opened_at := 0
var tapped := false           # opened with a tap: stays open until a pick
var pick := -1
var time := 0.0
var flash := -1               # the order just given, lit for a moment
var flash_time := 0.0


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # clicks are taken in _input, never in the way of the guns
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)


func _ship() -> Ship:
	return world.player if world != null else null


## The wheel stays shut over the shipyard, the maps and the menus.
func _blocked() -> bool:
	var p := _ship()
	if p == null or p.dead or p.docked or world.menu_open or world.paused:
		return true
	var layer := get_parent().get_parent() if get_parent() != null else null
	if layer != null:
		for c in layer.get_children():
			if c is Builder or c is TacticalMap or c is Scoreboard or c is Ledger or c is CargoPanel:
				return true
	return false


func _input(event: InputEvent) -> void:
	if world == null:
		return
	if _blocked():
		if open:
			_close()
		return
	if not open:
		if event.is_action_pressed("wing_menu"):
			_open()
			get_viewport().set_input_as_handled()
		elif event.is_action_pressed("wing_attack"):
			give("attack")
			get_viewport().set_input_as_handled()
		elif event.is_action_pressed("wing_form"):
			give("form")
			get_viewport().set_input_as_handled()
		return
	if event.is_action_released("wing_menu"):
		if not tapped and Time.get_ticks_msec() - opened_at > HOLD_MS:
			if pick >= 0:
				give(Wing.WHEEL[pick])
			_close()
		else:
			tapped = true
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("wing_menu") or event.is_action_pressed("pause_menu") or event.is_action_pressed("ui_cancel"):
		_close()
		get_viewport().set_input_as_handled()
	elif event is InputEventKey and event.pressed and not event.echo and event.physical_keycode >= KEY_1 and event.physical_keycode <= KEY_8:
		give(Wing.WHEEL[event.physical_keycode - KEY_1])
		_close()
		get_viewport().set_input_as_handled()
	elif event is InputEventMouseMotion:
		_pick_at(event.position - get_viewport_rect().size * 0.5, INNER * 0.7)
	elif event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT and pick >= 0:
			give(Wing.WHEEL[pick])
		if event.button_index in [MOUSE_BUTTON_LEFT, MOUSE_BUTTON_RIGHT]:
			_close()
			get_viewport().set_input_as_handled()


func _open() -> void:
	open = true
	tapped = false
	opened_at = Time.get_ticks_msec()
	pick = -1
	Sfx.play_ui("click", 0.3)


func _close() -> void:
	open = false
	pick = -1


## The slice under a pointer this far from the centre, or none inside `dead`.
func _pick_at(off: Vector2, dead: float) -> void:
	if off.length() < dead:
		pick = -1
		return
	var a := wrapf(off.angle() + PI * 0.5 + PI / Wing.WHEEL.size(), 0.0, TAU)
	pick = int(a / (TAU / Wing.WHEEL.size())) % Wing.WHEEL.size()


## Give an order to the wing.
func give(order: String) -> void:
	var p := _ship()
	if p == null or p.dead:
		return
	world.wing_command(p, order)
	flash = Wing.WHEEL.find(order)
	flash_time = time
	Sfx.play_ui("click", 0.5)


func _process(delta: float) -> void:
	time += delta
	if open:
		if _blocked():
			_close()
		else:
			# the right stick points too
			var stick := Vector2(Input.get_action_strength("aim_right") - Input.get_action_strength("aim_left"),
				Input.get_action_strength("aim_down") - Input.get_action_strength("aim_up"))
			if stick.length() > 0.5:
				_pick_at(stick * RADIUS, 1.0)
	queue_redraw()


## Is an order of no use just now (nobody to give it to, nobody to call)?
func _idle(order: String, wing: Array) -> bool:
	var p := _ship()
	match order:
		"call":
			return world.duel_rules != null or (world.practice and not world.tutorial) or world.wing.called(p).size() >= Wing.seats(p.rank)
		"dismiss":
			return world.wing.called(p).is_empty()
		"attack":
			return wing.is_empty() or p.target == null or not is_instance_valid(p.target) or (p.target is Ship and p.target.team() == p.team())
	return wing.is_empty()


func _draw() -> void:
	if not open or world == null or _ship() == null:
		return
	var p := _ship()
	var vp := get_viewport_rect().size
	var c := vp * 0.5
	var n := Wing.WHEEL.size()
	var wing := world.wing.members(p).filter(func(s): return not s.dead)
	var current: String = world.wing.order_of(p).get("order", Wing.DEFAULT)
	draw_circle(c, RADIUS + 8.0, Color(0.01, 0.03, 0.07, 0.72))
	for i in n:
		var a0 := -PI * 0.5 - PI / n + i * TAU / n
		var a1 := a0 + TAU / n
		var order: String = Wing.WHEEL[i]
		var idle := _idle(order, wing)
		var lit := i == pick or (i == flash and time - flash_time < 0.35)
		var col := Color(0.08, 0.16, 0.3, 0.85)
		if lit:
			col = Color(0.25, 0.5, 0.9, 0.92) if not idle else Color(0.3, 0.32, 0.38, 0.9)
		elif order == current and not order in ["call", "dismiss"] and not wing.is_empty():
			col = Color(0.12, 0.32, 0.22, 0.9)
		var pts := PackedVector2Array()
		for k in 13:
			pts.append(c + Vector2.from_angle(lerpf(a0, a1, k / 12.0)) * RADIUS)
		for k in 13:
			pts.append(c + Vector2.from_angle(lerpf(a1, a0, k / 12.0)) * INNER)
		draw_colored_polygon(pts, col)
		pts.append(pts[0])
		draw_polyline(pts, Color(0.5, 0.75, 1.0, 0.55 if lit else 0.3), 1.2, true)
		var mid := c + Vector2.from_angle((a0 + a1) * 0.5) * (INNER + RADIUS) * 0.5
		var tcol := Color(1, 1, 1) if not idle else Color(0.55, 0.6, 0.68)
		_icon(order, mid + Vector2(0, -14), tcol)
		_centred(mid + Vector2(0, 10), SHORT.get(order, Wing.LABELS[order]), 12, tcol, 96.0)
		_centred(mid + Vector2(0, 25), str(i + 1), 11, Color(0.75, 0.85, 1.0, 0.7), 20.0)
	# the hub: the wing and what the pointed-at order does
	draw_circle(c, INNER - 4.0, Color(0.02, 0.05, 0.1, 0.95))
	draw_arc(c, INNER - 4.0, 0, TAU, 40, Color(0.5, 0.75, 1.0, 0.5), 1.2, true)
	var called := world.wing.called(p).size()
	_centred(c + Vector2(0, -6), "WING %d" % wing.size(), 14, Color(0.85, 0.95, 1.0), INNER * 2.0)
	_centred(c + Vector2(0, 12), "called %d/%d" % [called, Wing.seats(p.rank)], 11, Color(0.6, 0.75, 0.9), INNER * 2.0)
	var hint := ""
	if pick >= 0:
		hint = Wing.HINTS[Wing.WHEEL[pick]]
		if Wing.WHEEL[pick] == "attack" and p.target != null and is_instance_valid(p.target):
			hint = "Everyone on %s" % (p.target.pilot_name if p.target is Ship else p.target.body_name)
	else:
		hint = "Now: %s.  Point and let go, click, or press 1-8" % Wing.LABELS.get(current, current).to_lower() if not wing.is_empty() \
			else "No wingmen yet: Call wingmen (7), or capture a ship"
	var hy := c.y + RADIUS + 32.0
	draw_string_outline(font, Vector2(0, hy), hint, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 16, 4, Color(0, 0, 0, 0.85))
	draw_string(font, Vector2(0, hy), hint, HORIZONTAL_ALIGNMENT_CENTER, vp.x, 16, Color(0.9, 0.95, 1.0))


func _centred(pos: Vector2, text: String, size: int, col: Color, width: float) -> void:
	draw_string_outline(font, pos + Vector2(-width * 0.5, 0), text, HORIZONTAL_ALIGNMENT_CENTER, width, size, 3, Color(0, 0, 0, 0.8))
	draw_string(font, pos + Vector2(-width * 0.5, 0), text, HORIZONTAL_ALIGNMENT_CENTER, width, size, col)


## A small line drawing for each order.
func _icon(order: String, at: Vector2, col: Color) -> void:
	var w := 1.6
	match order:
		"form":
			# a vee of three
			for o in [Vector2(0, -6), Vector2(-8, 4), Vector2(8, 4)]:
				_dart(at + o, col)
		"attack":
			draw_arc(at, 8.0, 0, TAU, 20, col, w, true)
			for d in [Vector2.UP, Vector2.DOWN, Vector2.LEFT, Vector2.RIGHT]:
				draw_line(at + d * 4.0, at + d * 11.0, col, w, true)
		"cover":
			# a shield
			var pts := PackedVector2Array([at + Vector2(-8, -8), at + Vector2(8, -8), at + Vector2(8, 0), at + Vector2(0, 9), at + Vector2(-8, 0), at + Vector2(-8, -8)])
			draw_polyline(pts, col, w, true)
		"hold":
			draw_arc(at, 9.0, 0, TAU, 20, col, w, true)
			draw_circle(at, 3.0, col)
		"engage":
			for d in [Vector2(-1, -1), Vector2(1, -1), Vector2(-1, 1), Vector2(1, 1)]:
				draw_line(at + d * 2.0, at + d * 8.0, col, w, true)
		"home":
			var pts := PackedVector2Array([at + Vector2(-8, 1), at + Vector2(0, -8), at + Vector2(8, 1), at + Vector2(5, 1), at + Vector2(5, 8), at + Vector2(-5, 8), at + Vector2(-5, 1), at + Vector2(-8, 1)])
			draw_polyline(pts, col, w, true)
		"call":
			_dart(at + Vector2(-3, 0), col)
			draw_line(at + Vector2(6, -5), at + Vector2(6, 5), col, w, true)
			draw_line(at + Vector2(1, 0), at + Vector2(11, 0), col, w, true)
		"dismiss":
			_dart(at + Vector2(-3, 0), col)
			draw_line(at + Vector2(1, 0), at + Vector2(11, 0), col, w, true)


func _dart(at: Vector2, col: Color) -> void:
	draw_colored_polygon(PackedVector2Array([at + Vector2(0, -5), at + Vector2(4, 4), at + Vector2(-4, 4)]), col)
