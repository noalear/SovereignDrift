## Trade -- a window between two pilots: each puts credits, resources from
## the cargo hold, salvage parts and blueprints on the table, both accept,
## the host swaps.  Drawn by hand
## like the shipyard; every change goes to the host through Net.
class_name Trade
extends Control

const PANEL := Color(0.03, 0.05, 0.11, 0.94)
const EDGE := Color(0.5, 0.7, 1.0, 0.55)
const TEXT := Color(0.9, 0.94, 1.0)
const DIM := Color(0.6, 0.68, 0.8)
const GOOD := Color(0.5, 1.0, 0.6)
const GOLD := Color(1.0, 0.85, 0.4)

var font: Font
var trade_id := 0
var my_id := 0
var partner_id := 0
var partner_name := "Pilot"
var credits := 0                     # what I own
var hold: Array = []
var blueprints: Array = []
var cargo := {}                      # resources in my hold
var offers := {}                     # net_id -> {credits, parts, blueprints, cargo}
var accepted := {}                   # net_id -> bool
var buttons: Array = []              # [rect, callable, enabled]
var hover := -1
var status := ""
var time := 0.0


func open(data: Dictionary) -> void:
	font = ThemeDB.fallback_font
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	process_mode = Node.PROCESS_MODE_ALWAYS
	trade_id = data["id"]
	my_id = data["me"]
	partner_id = data["partner"]
	partner_name = data["partner_name"]
	credits = data["credits"]
	hold = data["hold"].duplicate()
	blueprints = data["blueprints"].duplicate()
	cargo = data.get("cargo", {}).duplicate()
	offers = {my_id: _empty_offer(), partner_id: _empty_offer()}
	accepted = {my_id: false, partner_id: false}
	status = "Put something on the table, then accept. Both must accept."


static func _empty_offer() -> Dictionary:
	return {"credits": 0, "parts": [], "blueprints": [], "cargo": {}}


## The host's word on both offers and who has accepted.
func update(data: Dictionary) -> void:
	for k in data.get("offers", {}):
		offers[int(k)] = data["offers"][k]
	for k in data.get("accepted", {}):
		accepted[int(k)] = data["accepted"][k]
	if data.has("status"):
		status = data["status"]


func mine() -> Dictionary:
	return offers.get(my_id, _empty_offer())


func theirs() -> Dictionary:
	return offers.get(partner_id, _empty_offer())


func _send_offer() -> void:
	accepted[my_id] = false
	accepted[partner_id] = false
	Net.trade_cmd("offer", {"id": trade_id, "offer": mine()})


func _add_credits(n: int) -> void:
	var o := mine()
	o["credits"] = clampi(o["credits"] + n, 0, credits)
	offers[my_id] = o
	Sfx.play_ui("click", 0.4)
	_send_offer()


## Put more or less of a resource on the table (`n` 0: none, -1: all).
func _add_cargo(k: String, n: int) -> void:
	var o := mine()
	var c: Dictionary = o.get("cargo", {}).duplicate()
	var have := int(cargo.get(k, 0.0))
	var v := have if n == -1 else (0 if n == 0 else clampi(int(c.get(k, 0)) + n, 0, have))
	if v > 0:
		c[k] = v
	else:
		c.erase(k)
	o["cargo"] = c
	offers[my_id] = o
	Sfx.play_ui("click", 0.4)
	_send_offer()


static func _cargo_text(c: Dictionary) -> String:
	var parts := []
	for k in Economy.RESOURCES:
		if int(c.get(k, 0)) > 0:
			parts.append("%d %s" % [int(c[k]), Economy.NAMES[k]])
	return ", ".join(parts) if not parts.is_empty() else "nothing"


func _toggle_part(index: int) -> void:
	var o := mine()
	var parts: Array = o["parts"]
	if parts.has(index):
		parts.erase(index)
	else:
		parts.append(index)
	o["parts"] = parts
	offers[my_id] = o
	Sfx.play_ui("click", 0.4)
	_send_offer()


func _toggle_blueprint(id: String) -> void:
	var o := mine()
	var bps: Array = o["blueprints"]
	if bps.has(id):
		bps.erase(id)
	else:
		bps.append(id)
	o["blueprints"] = bps
	offers[my_id] = o
	Sfx.play_ui("click", 0.4)
	_send_offer()


func _accept() -> void:
	Sfx.play_ui("click", 0.5)
	Net.trade_cmd("accept", {"id": trade_id})


func _cancel() -> void:
	Net.trade_cmd("cancel", {"id": trade_id})


# ----------------------------------------------------------------- input

func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		hover = -1
		for i in buttons.size():
			if buttons[i][0].has_point(event.position):
				hover = i
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for i in buttons.size():
			if buttons[i][0].has_point(event.position) and buttons[i][2]:
				buttons[i][1].call()
				accept_event()
				return


func _input(event: InputEvent) -> void:
	if event.is_action_pressed("pause_menu"):
		_cancel()
		get_viewport().set_input_as_handled()
	elif event is InputEventKey and event.pressed and event.keycode == KEY_ENTER:
		_accept()
		get_viewport().set_input_as_handled()


func _process(delta: float) -> void:
	time += delta
	queue_redraw()


# --------------------------------------------------------------- drawing

func _button(rect: Rect2, label: String, cb: Callable, enabled: bool = true, size: int = 16, lit: bool = false) -> void:
	buttons.append([rect, cb, enabled])
	var idx := buttons.size() - 1
	var bg := Color(0.08, 0.14, 0.28, 0.9) if hover != idx else Color(0.18, 0.3, 0.55, 0.95)
	if lit:
		bg = Color(0.1, 0.35, 0.2, 0.95) if hover != idx else Color(0.15, 0.5, 0.3, 1.0)
	if not enabled:
		bg = Color(0.1, 0.1, 0.12, 0.6)
	draw_rect(rect, bg)
	draw_rect(rect, EDGE if enabled else Color(0.3, 0.3, 0.35, 0.4), false, 1.0)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + size * 0.36), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, size, TEXT if enabled else DIM)


static func _part_name(id: String) -> String:
	return GameData.MODULES[id]["name"] if GameData.MODULES.has(id) else id


func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	var w := 900.0
	var h := 640.0
	var r := Rect2((vp.x - w) * 0.5, (vp.y - h) * 0.5, w, h)
	draw_rect(r, PANEL)
	draw_rect(r, EDGE, false, 1.5)
	draw_string(font, r.position + Vector2(20, 36), "TRADE with %s" % partner_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 24, GOLD)
	var both: bool = accepted.get(my_id, false) and accepted.get(partner_id, false)
	var st := status
	if accepted.get(partner_id, false) and not accepted.get(my_id, false):
		st = "%s has accepted. Accept to close the deal, or change your offer." % partner_name
	elif accepted.get(my_id, false) and not accepted.get(partner_id, false):
		st = "Waiting for %s to accept..." % partner_name
	elif both:
		st = "Deal."
	draw_string(font, r.position + Vector2(20, h - 20), st, HORIZONTAL_ALIGNMENT_LEFT, w - 300, 15, DIM)
	# columns
	var cw := (w - 60) * 0.5
	_draw_mine(Rect2(r.position + Vector2(20, 56), Vector2(cw, h - 140)))
	_draw_theirs(Rect2(r.position + Vector2(40 + cw, 56), Vector2(cw, h - 140)))
	# accept / cancel
	var ay := r.end.y - 92
	_button(Rect2(r.end.x - 320, ay, 160, 40), ("Accepted" if accepted.get(my_id, false) else "Accept  (Enter)"), _accept, true, 16, accepted.get(my_id, false))
	_button(Rect2(r.end.x - 150, ay, 130, 40), "Cancel  (Esc)", _cancel, true, 16)


func _draw_mine(r: Rect2) -> void:
	draw_rect(r, Color(0, 0, 0, 0.35))
	draw_rect(r, EDGE, false, 1.0)
	var x := r.position.x + 14
	var y := r.position.y + 26
	draw_string(font, Vector2(x, y), "You offer", HORIZONTAL_ALIGNMENT_LEFT, -1, 18, TEXT)
	var o := mine()
	y += 34
	draw_string(font, Vector2(x, y), "Credits  %d  of %d" % [o["credits"], credits], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, GOLD)
	var bx := x + 200
	for step in [-100, -10, 10, 100]:
		var label := ("%+d" % step)
		var can: bool = (step < 0 and o["credits"] > 0) or (step > 0 and o["credits"] < credits)
		_button(Rect2(bx, y - 18, 48, 26), label, _add_credits.bind(step), can, 14)
		bx += 54
	y += 30
	# resources from the cargo hold
	var held := []
	for k in Economy.RESOURCES:
		if cargo.get(k, 0.0) >= 1.0:
			held.append(k)
	if held.is_empty():
		draw_string(font, Vector2(x, y), "Cargo hold empty", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
		y += 30
	for k in held:
		var on := int(o.get("cargo", {}).get(k, 0))
		draw_string(font, Vector2(Economy.draw_icon(self, k, x, y - 6, 15.0), y), "%s  %d  of %d" % [Economy.NAMES[k], on, int(cargo[k])], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Economy.COLORS[k])
		var cx := x + 200
		for step in [-10, 10]:
			var can: bool = (step < 0 and on > 0) or (step > 0 and on < int(cargo[k]))
			_button(Rect2(cx, y - 18, 48, 24), "%+d" % step, _add_cargo.bind(k, step), can, 13)
			cx += 54
		_button(Rect2(cx, y - 18, 102, 24), "All" if on < int(cargo[k]) else "None", _add_cargo.bind(k, -1 if on < int(cargo[k]) else 0), true, 13)
		y += 28
	y += 8
	draw_string(font, Vector2(x, y), "Salvage in the hold (%d)" % hold.size(), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
	y += 8
	if hold.is_empty():
		y += 20
		draw_string(font, Vector2(x, y), "nothing", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	for i in hold.size():
		if y > r.end.y - 120:
			break
		var on: bool = o["parts"].has(i)
		_button(Rect2(x, y + 4, r.size.x - 28, 26), "%s   %d cr" % [_part_name(hold[i]), GameData.price(hold[i])], _toggle_part.bind(i), true, 14, on)
		y += 30
	y += 26
	draw_string(font, Vector2(x, y), "Blueprints (%d)" % blueprints.size(), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
	y += 8
	if blueprints.is_empty():
		y += 20
		draw_string(font, Vector2(x, y), "none", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	for id in blueprints:
		if y > r.end.y - 30:
			break
		var on: bool = o["blueprints"].has(id)
		_button(Rect2(x, y + 4, r.size.x - 28, 26), "Blueprint: %s" % _part_name(id), _toggle_blueprint.bind(id), true, 14, on)
		y += 30


func _draw_theirs(r: Rect2) -> void:
	draw_rect(r, Color(0, 0, 0, 0.35))
	draw_rect(r, EDGE, false, 1.0)
	var x := r.position.x + 14
	var y := r.position.y + 26
	draw_string(font, Vector2(x, y), "%s offers" % partner_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 18, TEXT)
	var o := theirs()
	y += 34
	draw_string(font, Vector2(x, y), "Credits  %d" % o["credits"], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, GOLD)
	y += 30
	draw_string(font, Vector2(x, y), "Cargo  " + _cargo_text(o.get("cargo", {})), HORIZONTAL_ALIGNMENT_LEFT, r.size.x - 28, 15, TEXT)
	y += 36
	draw_string(font, Vector2(x, y), "Parts", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
	var names: Array = o.get("part_names", [])
	if names.is_empty():
		y += 24
		draw_string(font, Vector2(x, y), "nothing", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	for n in names:
		y += 24
		draw_string(font, Vector2(x, y), str(n), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, TEXT)
	y += 36
	draw_string(font, Vector2(x, y), "Blueprints", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, DIM)
	if o["blueprints"].is_empty():
		y += 24
		draw_string(font, Vector2(x, y), "none", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	for id in o["blueprints"]:
		y += 24
		draw_string(font, Vector2(x, y), "Blueprint: %s" % _part_name(id), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, TEXT)
	if accepted.get(partner_id, false):
		draw_string(font, Vector2(x, r.end.y - 14), "%s has accepted" % partner_name, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, GOOD)
