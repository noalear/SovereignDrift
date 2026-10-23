## Builder -- the shipyard screen at a starbase.  Pick a hull, fill its
## hardpoints and bays from the catalogue or your salvage, watch the
## numbers change, launch.  Everything is drawn by hand like the menu.
class_name Builder
extends Control

signal closed(launched: bool)

const PANEL := Color(0.03, 0.05, 0.11, 0.94)
const EDGE := Color(0.5, 0.7, 1.0, 0.55)
const TEXT := Color(0.9, 0.94, 1.0)
const DIM := Color(0.6, 0.68, 0.8)
const GOOD := Color(0.5, 1.0, 0.6)
const BAD := Color(1.0, 0.45, 0.4)
const GOLD := Color(1.0, 0.85, 0.4)

var font: Font
var ship: Ship
var base: Ship
var world: World

var hull := "Scout"
var orig_hull := "Scout"
var orig_design: Dictionary = {}
var hardpoints: Array = []          # from GameData.hull_hardpoints
var mounts: Array = []              # per hardpoint: {"id", "stock"} or null
var modules: Array = []             # bay parts: {"id", "stock"}
var hold: Array = []
var blueprints: Array = []
var credits := 0
var hull_paid := 0                  # credits spent on the hull this visit

var sel_kind := ""                  # "hp" | "bay" | "hold"
var sel_index := -1
var cat_scroll := 0.0
var cat_rect := Rect2()
var buttons: Array = []             # [rect, callable, enabled]
var hover := -1
var _hp_hover := {}                 # button index -> hardpoint, for highlighting
var status := ""
var status_time := 0.0
var time := 0.0


func open(p_ship: Ship, p_base: Ship, p_world: World) -> void:
	ship = p_ship
	base = p_base
	world = p_world
	font = ThemeDB.fallback_font
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	process_mode = Node.PROCESS_MODE_ALWAYS
	orig_design = ship.design.duplicate(true)
	orig_hull = ship.design_name
	hold = ship.hold.duplicate()
	blueprints = ship.blueprints.duplicate()
	credits = ship.credits
	_load_hull(orig_hull, orig_design, true)
	_say("Pick a hardpoint or bay, then a part from the catalogue.")


## Put a hull on the slip.  `fit` is the design to fill it from (the stock
## fit of that hull when empty).  Parts already on the slip that the pilot
## owns go back to the hold; welded stock parts are left behind.
func _load_hull(name: String, fit: Dictionary, initial: bool) -> void:
	if not initial:
		_strip_to_hold()
	hull = name
	hardpoints = GameData.hull_hardpoints(name)
	if fit.is_empty():
		fit = GameData.stock_design(name)
	mounts = []
	for i in hardpoints.size():
		mounts.append(null)
	for w in fit["weapons"]:
		# match the fit's weapons to hardpoints by mount angle and radius
		for i in hardpoints.size():
			if mounts[i] == null and hardpoints[i][0] == w[1] and hardpoints[i][1] == w[2]:
				mounts[i] = {"id": w[0], "stock": true}
				break
	modules = []
	for id in fit["modules"]:
		modules.append({"id": id, "stock": true})
	sel_kind = ""
	sel_index = -1
	cat_scroll = 0.0


func _strip_to_hold() -> void:
	for m in mounts:
		if m != null and not m["stock"]:
			_to_hold(m["id"])
	for m in modules:
		if not m["stock"]:
			_to_hold(m["id"])


func _to_hold(id: String) -> void:
	if hold.size() < Ship.HOLD_SIZE:
		hold.append(id)
	else:
		var v := GameData.price(id) / 2
		credits += v
		_say("Hold full: %s sold for %d credits." % [GameData.MODULES[id]["name"], v])


func _say(text: String) -> void:
	status = text
	status_time = time


# ----------------------------------------------------------- the design

func current_design() -> Dictionary:
	var d := GameData.stock_design(hull)
	d["modules"] = []
	for m in modules:
		d["modules"].append(m["id"])
	d["weapons"] = []
	for i in hardpoints.size():
		if mounts[i] != null:
			var hp: Array = hardpoints[i]
			d["weapons"].append([mounts[i]["id"], hp[0], hp[1], hp[2], hp[3], hp[4]])
	return d


func _problems() -> Array:
	var st := GameData.stats_for(current_design())
	var out := []
	if st["bay_used"] > st["bay_cap"]:
		out.append("Bays over capacity (%d / %d)." % [st["bay_used"], st["bay_cap"]])
	if st["engines"] == 0:
		out.append("No engine fitted.")
	if st["power_gen"] <= 0.0:
		out.append("No power plant fitted.")
	var armed := false
	for m in mounts:
		if m != null:
			armed = true
	if not armed:
		out.append("No weapon mounted.")
	return out


func _hull_price(name: String) -> int:
	if name == orig_hull:
		return 0
	return int(GameData.HULL_PRICE.get(name, 0))


func _switch_hull(name: String) -> void:
	if name == hull:
		return
	var cost := _hull_price(name)
	if cost - hull_paid > credits:
		_say("Not enough credits for a %s (%d)." % [name, cost])
		return
	credits += hull_paid
	credits -= cost
	hull_paid = cost
	_load_hull(name, {}, false)
	Sfx.play_ui("dock", 0.5)
	_say("%s on the slip: stock fit loaded, your parts are in the hold." % name)


## Mount a part into the selected slot, from the hold if there is one
## there, else bought from the catalogue.
func _mount(id: String) -> void:
	var from_hold := hold.has(id)
	var cost := 0 if from_hold else GameData.price(id)
	if not from_hold and cost > credits:
		_say("Not enough credits: %s costs %d." % [GameData.MODULES[id]["name"], cost])
		return
	if sel_kind == "hp":
		if GameData.weapon_class(id) > hardpoints[sel_index][5]:
			_say("That mount only takes %s weapons." % GameData.class_name_of(hardpoints[sel_index][5]))
			return
		_unmount_hp(sel_index)
		mounts[sel_index] = {"id": id, "stock": false}
	elif sel_kind == "bay":
		if sel_index >= 0 and sel_index < modules.size():
			_unmount_bay(sel_index)
			modules.insert(sel_index, {"id": id, "stock": false})
		else:
			modules.append({"id": id, "stock": false})
			sel_index = modules.size() - 1
	else:
		return
	if from_hold:
		hold.erase(id)
	else:
		credits -= cost
	Sfx.play_ui("click", 0.5)
	_say("%s fitted%s." % [GameData.MODULES[id]["name"], " from the hold" if from_hold else " for %d credits" % cost])


func _unmount_hp(i: int) -> void:
	var m = mounts[i]
	if m == null:
		return
	if not m["stock"]:
		_to_hold(m["id"])
	mounts[i] = null


func _unmount_bay(i: int) -> void:
	if i < 0 or i >= modules.size():
		return
	var m: Dictionary = modules[i]
	if not m["stock"]:
		_to_hold(m["id"])
	modules.remove_at(i)


func _remove_selected() -> void:
	if sel_kind == "hp":
		var m = mounts[sel_index]
		if m != null:
			_say("%s removed%s." % [GameData.MODULES[m["id"]]["name"], " to the hold" if not m["stock"] else " (welded parts are scrapped)"])
			_unmount_hp(sel_index)
	elif sel_kind == "bay" and sel_index < modules.size():
		var m: Dictionary = modules[sel_index]
		_say("%s removed%s." % [GameData.MODULES[m["id"]]["name"], " to the hold" if not m["stock"] else " (welded parts are scrapped)"])
		_unmount_bay(sel_index)
		sel_index = mini(sel_index, modules.size())


func _sell_hold(i: int) -> void:
	if i < 0 or i >= hold.size():
		return
	var id: String = hold[i]
	var v := GameData.price(id) / 2
	credits += v
	hold.remove_at(i)
	Sfx.play_ui("pickup", 0.4)
	_say("%s sold for %d credits." % [GameData.MODULES[id]["name"], v])


func _launch() -> void:
	var problems := _problems()
	if not problems.is_empty():
		_say(problems[0])
		return
	ship.credits = credits
	ship.hold = hold
	ship.blueprints = blueprints
	world.apply_build(ship, current_design())
	closed.emit(true)
	queue_free()


func _cancel() -> void:
	closed.emit(false)
	queue_free()


# ----------------------------------------------------------------- input

func _gui_input(event: InputEvent) -> void:
	# controls drawn later sit on top (a Sell button over its hold row), so
	# the last one under the pointer wins
	if event is InputEventMouseMotion:
		hover = -1
		for i in range(buttons.size() - 1, -1, -1):
			if buttons[i][0].has_point(event.position):
				hover = i
				break
	elif event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			for i in range(buttons.size() - 1, -1, -1):
				if buttons[i][0].has_point(event.position) and buttons[i][2]:
					buttons[i][1].call()
					accept_event()
					return
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP and cat_rect.has_point(event.position):
			cat_scroll = maxf(cat_scroll - 3.0, 0.0)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN and cat_rect.has_point(event.position):
			cat_scroll += 3.0


func _input(event: InputEvent) -> void:
	if event.is_action_pressed("pause_menu"):
		_cancel()
		get_viewport().set_input_as_handled()
	elif event is InputEventKey and event.pressed and event.keycode == KEY_ENTER:
		_launch()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("refit"):
		get_viewport().set_input_as_handled()


func _process(delta: float) -> void:
	time += delta
	queue_redraw()


# --------------------------------------------------------------- drawing

func _button(rect: Rect2, label: String, cb: Callable, enabled: bool = true, size: int = 18, accent: bool = false) -> void:
	buttons.append([rect, cb, enabled])
	var idx := buttons.size() - 1
	var bg := Color(0.08, 0.14, 0.28, 0.9) if hover != idx else Color(0.18, 0.3, 0.55, 0.95)
	if accent:
		bg = Color(0.1, 0.35, 0.2, 0.95) if hover != idx else Color(0.15, 0.5, 0.3, 1.0)
	if not enabled:
		bg = Color(0.1, 0.1, 0.12, 0.6)
	draw_rect(rect, bg)
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.7 if enabled else 0.25), false, 1.2)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + size * 0.36), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, size, TEXT if enabled else Color(0.5, 0.5, 0.5))


func _panel(rect: Rect2, title: String) -> void:
	draw_rect(rect, PANEL)
	draw_rect(rect, EDGE, false, 1.2)
	if not title.is_empty():
		draw_string(font, rect.position + Vector2(14, 26), title, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.6, 0.8, 1.0))


func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.01, 0.02, 0.05, 0.9))
	var m := 20.0
	var header := Rect2(m, m, vp.x - 2 * m, 56)
	var left := Rect2(m, m + 66, 380, vp.y - m * 2 - 66)
	var right := Rect2(vp.x - m - 560, m + 66, 560, vp.y - m * 2 - 66)
	var centre := Rect2(left.end.x + 10, m + 66, right.position.x - left.end.x - 20, vp.y - m * 2 - 66)
	_draw_header(header)
	_draw_hulls(left)
	_draw_centre(centre)
	_draw_right(right)


func _draw_header(r: Rect2) -> void:
	_panel(r, "")
	var rc: Color = GameData.race(base.race_id)["color"]
	draw_string(font, r.position + Vector2(18, 36), "SHIPYARD", HORIZONTAL_ALIGNMENT_LEFT, -1, 24, rc)
	draw_string(font, r.position + Vector2(150, 36), "%s   ·   %s %s" % [base.ship_name, GameData.rank_name(ship.rank), ship.pilot_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 17, DIM)
	var cr := "Credits  %d" % credits
	draw_string(font, r.position + Vector2(r.size.x * 0.5 - 150, 36), cr, HORIZONTAL_ALIGNMENT_CENTER, 300, 22, GOLD)
	var problems := _problems()
	_button(Rect2(r.end.x - 300, r.position.y + 8, 150, 40), "Launch  (Enter)", _launch, problems.is_empty(), 17, true)
	_button(Rect2(r.end.x - 140, r.position.y + 8, 124, 40), "Cancel  (Esc)", _cancel, true, 17)
	# status line
	var age := time - status_time
	var a := clampf(1.0 - (age - 6.0) / 2.0, 0.0, 1.0)
	var sx := r.size.x * 0.5 + 160
	var sw := r.size.x - 316 - sx
	if not problems.is_empty():
		draw_string(font, r.position + Vector2(sx, 35), problems[0], HORIZONTAL_ALIGNMENT_RIGHT, sw, 14, BAD)
	elif a > 0.0:
		draw_string(font, r.position + Vector2(sx, 35), status, HORIZONTAL_ALIGNMENT_RIGHT, sw, 14, Color(0.85, 0.9, 1.0, a))


func _draw_hulls(r: Rect2) -> void:
	_panel(r, "HULLS")
	var y := r.position.y + 44
	for name in GameData.REFIT_ORDER:
		if name == "Starbase":
			continue
		var d: Dictionary = GameData.SHIPS[name]
		var allowed: bool = d["rank"] <= ship.rank
		var row := Rect2(r.position.x + 12, y, r.size.x - 24, 46)
		var selected: bool = name == hull
		buttons.append([row, _switch_hull.bind(name), allowed])
		var idx := buttons.size() - 1
		var bg := Color(0.25, 0.45, 0.8, 0.9) if selected else (Color(0.06, 0.1, 0.2, 0.8) if hover != idx else Color(0.14, 0.24, 0.45, 0.9))
		if not allowed:
			bg = Color(0.08, 0.08, 0.1, 0.6)
		draw_rect(row, bg)
		draw_rect(row, Color(0.5, 0.7, 1.0, 0.35), false, 1.0)
		# a little picture of the hull
		ShipArt.draw_ship(self, name, base.race_id, 12.0 + d["size"] * 0.12, GameData.race(base.race_id)["color"], 0.0, 1.0 if allowed else 0.35, 0.0, Transform2D(0.0, row.position + Vector2(34, 23)))
		draw_string(font, row.position + Vector2(74, 20), name, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT if allowed else Color(0.5, 0.5, 0.55))
		var sub := "%s · bays %d" % [GameData.rank_name(d["rank"]), GameData.HULL_BAYS[name]] if allowed else "needs %s" % GameData.rank_name(d["rank"])
		draw_string(font, row.position + Vector2(74, 38), sub, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
		var price := _hull_price(name)
		var ptxt := "current" if name == orig_hull else ("%d cr" % price)
		draw_string(font, row.position + Vector2(row.size.x - 110, 24), ptxt, HORIZONTAL_ALIGNMENT_RIGHT, 100, 15, GOLD if name != orig_hull else DIM)
		y += 50
	# the hold
	y += 10
	draw_string(font, Vector2(r.position.x + 14, y + 14), "HOLD  %d / %d   (salvage; free to fit, sells at half)" % [hold.size(), Ship.HOLD_SIZE], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(0.6, 0.8, 1.0))
	y += 24
	if hold.is_empty():
		draw_string(font, Vector2(r.position.x + 14, y + 16), "Empty. Wrecks drop parts; fly over them.", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	var max_rows := int((r.end.y - y - 10) / 30.0)
	for i in mini(hold.size(), max_rows):
		var id: String = hold[i]
		var row := Rect2(r.position.x + 12, y, r.size.x - 24, 27)
		var selected := sel_kind == "hold" and sel_index == i
		buttons.append([row, _select.bind("hold", i), true])
		var idx := buttons.size() - 1
		draw_rect(row, Color(0.25, 0.45, 0.8, 0.8) if selected else (Color(0.06, 0.1, 0.2, 0.7) if hover != idx else Color(0.14, 0.24, 0.45, 0.9)))
		draw_string(font, row.position + Vector2(8, 19), GameData.MODULES[id]["name"], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, TEXT)
		_button(Rect2(row.end.x - 64, row.position.y + 2, 60, 23), "Sell %d" % (GameData.price(id) / 2), _sell_hold.bind(i), true, 12)
		y += 30


func _select(kind: String, index: int) -> void:
	if sel_kind == "hold" and kind != "hold" and sel_index >= 0 and sel_index < hold.size():
		# a hold part was picked first: drop it into the clicked slot
		var id: String = hold[sel_index]
		sel_kind = kind
		sel_index = index
		var ok := (kind == "hp" and not GameData.is_bay_part(id)) or (kind == "bay" and GameData.is_bay_part(id))
		if ok:
			_mount(id)
			return
	sel_kind = kind
	sel_index = index
	cat_scroll = 0.0


func _draw_centre(r: Rect2) -> void:
	_panel(r, "")
	var design := current_design()
	var st := GameData.stats_for(design)
	var rc: Color = GameData.race(base.race_id)["color"]
	draw_string(font, r.position + Vector2(14, 28), "%s %s" % [GameData.race(base.race_id)["adj"], hull], HORIZONTAL_ALIGNMENT_LEFT, -1, 20, rc)
	# which mount the pointer is over, on the ship or in the list (from
	# last frame's buttons, which share this frame's layout)
	var hot: int = _hp_hover.get(hover, -1)
	_hp_hover.clear()
	# the lists below the picture take what they need; the ship gets the rest
	var mount_cols := 4
	var bay_cols := 3
	var mount_rows := ceili(hardpoints.size() / float(mount_cols))
	var bay_rows := ceili((modules.size() + 1) / float(bay_cols))
	var lists_h := 34.0 + mount_rows * 42.0 + 44.0 + bay_rows * 42.0 + 10.0
	var pic_h := clampf(r.size.y - 50.0 - lists_h, 200.0, r.size.y * 0.55)
	var centre := r.position + Vector2(r.size.x * 0.5, 40 + pic_h * 0.5)
	var size: float = design["size"]
	var scale := minf(pic_h * 0.36 / size, r.size.x * 0.3 / size)
	# never blow a painting up past about twice its pixels; small hulls
	# simply look smaller than big ones, which is the point
	var tex := ShipArt.hull_texture(hull, base.race_id)
	if tex != null:
		scale = minf(scale, 1.9 * tex.get_width() / (size * ShipArt.LENGTH))
	ShipArt.draw_ship(self, design, base.race_id, size, rc, 0.4 + 0.2 * sin(time * 3.0), 1.0, 0.0, Transform2D(0.0, Vector2.ONE * scale, 0.0, centre))
	# hardpoints: a small numbered badge on each turret; names live in the
	# list below, and only the mount in focus shows its firing arc
	var spots := ShipArt.hardpoint_spots(hull, base.race_id, size)
	var placed: Array[Vector2] = []
	var badge := []
	for i in hardpoints.size():
		var pos: Vector2 = centre + spots[i] * scale
		var tries := 0
		while tries < 6 and placed.any(func(q: Vector2) -> bool: return q.distance_to(pos) < 22.0):
			pos += Vector2(0, 22) if spots[i].y >= 0 else Vector2(0, -22)
			tries += 1
		placed.append(pos)
		badge.append(pos)
		buttons.append([Rect2(pos - Vector2(14, 14), Vector2(28, 28)), _select.bind("hp", i), true])
		_hp_hover[buttons.size() - 1] = i
	var focus := sel_index if sel_kind == "hp" else -1
	for i in hardpoints.size():
		if i == focus or i == hot:
			var hp: Array = hardpoints[i]
			var c := Color(1, 1, 1, 0.45) if i == focus else Color(0.7, 0.85, 1.0, 0.35)
			draw_arc(badge[i], 34.0, deg_to_rad(float(hp[3])), deg_to_rad(float(hp[4])), 32, c, 7.0, true)
	for i in hardpoints.size():
		var pos: Vector2 = badge[i]
		var filled: bool = mounts[i] != null
		var lit := i == focus or i == hot
		draw_circle(pos, 10.0, Color(0.02, 0.04, 0.1, 0.85))
		var edge := Color(1, 1, 1, 0.95) if i == focus else (Color(0.75, 0.88, 1.0, 0.9) if lit else Color(rc.r, rc.g, rc.b, 0.75 if filled else 0.4))
		draw_arc(pos, 10.0, 0, TAU, 20, edge, 2.0 if lit else 1.2, true)
		draw_string(font, pos + Vector2(-10, 4.5), str(i + 1), HORIZONTAL_ALIGNMENT_CENTER, 20, 12, TEXT if filled else DIM)
	# weapon mounts list
	var y := r.position.y + 50 + pic_h
	draw_string(font, Vector2(r.position.x + 14, y + 16), "WEAPON MOUNTS", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	y += 26
	var mw := (r.size.x - 28 - (mount_cols - 1) * 8) / mount_cols
	for i in hardpoints.size():
		var chip := Rect2(r.position.x + 14 + (i % mount_cols) * (mw + 8), y + (i / mount_cols) * 42, mw, 36)
		var hp: Array = hardpoints[i]
		var m = mounts[i]
		buttons.append([chip, _select.bind("hp", i), true])
		_hp_hover[buttons.size() - 1] = i
		var fc := Color(0.25, 0.45, 0.8, 0.9) if i == focus else (Color(0.16, 0.27, 0.5, 0.95) if i == hot else Color(0.08, 0.14, 0.28, 0.85))
		if m == null and i != focus and i != hot:
			fc = Color(0.05, 0.08, 0.16, 0.6)
		draw_rect(chip, fc)
		draw_rect(chip, Color(0.5, 0.7, 1.0, 0.5 if m != null else 0.3), false, 1.0)
		draw_string(font, chip.position + Vector2(8, 16), str(i + 1), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(rc.r, rc.g, rc.b))
		var name: String = GameData.MODULES[m["id"]]["name"] if m != null else "empty"
		draw_string(font, chip.position + Vector2(26, 16), _fit(name, mw - 34, 13), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, TEXT if m != null else DIM)
		var arc := int(round(float(hp[4]) - float(hp[3])))
		var sub := "%s  ·  %s" % [GameData.class_name_of(hp[5]), "turret" if arc >= 150 else "arc %d°" % arc]
		draw_string(font, chip.position + Vector2(26, 31), _fit(sub, mw - 34, 11), HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
		if m != null and not m["stock"]:
			draw_circle(chip.end - Vector2(10, 10), 3.0, GOOD)
	# bays
	var by := y + mount_rows * 42 + 8
	var cap: int = st["bay_cap"]
	var used: int = st["bay_used"]
	draw_string(font, Vector2(r.position.x + 14, by + 16), "INTERNAL BAYS   %d / %d volume" % [used, cap], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	var barw := r.size.x - 28
	draw_rect(Rect2(r.position.x + 14, by + 24, barw, 6), Color(0, 0, 0, 0.5))
	draw_rect(Rect2(r.position.x + 14, by + 24, barw * clampf(float(used) / cap, 0.0, 1.0), 6), BAD if used > cap else Color(0.4, 0.7, 1.0))
	by += 40
	var cols := bay_cols
	var cw := (r.size.x - 28 - (cols - 1) * 8) / cols
	for i in modules.size() + 1:
		var col := i % cols
		var row_i := i / cols
		var chip := Rect2(r.position.x + 14 + col * (cw + 8), by + row_i * 42, cw, 36)
		if chip.end.y > r.end.y - 8:
			break
		var selected := sel_kind == "bay" and sel_index == i
		buttons.append([chip, _select.bind("bay", i), true])
		var idx := buttons.size() - 1
		if i < modules.size():
			var id: String = modules[i]["id"]
			var mod: Dictionary = GameData.MODULES[id]
			var fc := Color(0.25, 0.45, 0.8, 0.9) if selected else (Color(0.08, 0.14, 0.28, 0.85) if hover != idx else Color(0.16, 0.27, 0.5, 0.95))
			draw_rect(chip, fc)
			draw_rect(chip, Color(0.5, 0.7, 1.0, 0.5), false, 1.0)
			draw_string(font, chip.position + Vector2(8, 16), mod["name"], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, TEXT)
			var pw: int = mod["power"]
			draw_string(font, chip.position + Vector2(8, 31), "vol %d   %s%d pw" % [mod["vol"], "+" if pw > 0 else "", pw], HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
			if not modules[i]["stock"]:
				draw_circle(chip.end - Vector2(10, 10), 3.0, GOOD)
		else:
			draw_rect(chip, Color(0.05, 0.08, 0.16, 0.6) if not selected else Color(0.2, 0.35, 0.6, 0.8))
			draw_rect(chip, Color(0.5, 0.7, 1.0, 0.3), false, 1.0)
			draw_string(font, chip.position + Vector2(0, 23), "+ add a part", HORIZONTAL_ALIGNMENT_CENTER, chip.size.x, 13, DIM)


func _price_w(price: String) -> float:
	return font.get_string_size(price, HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x


## `text` cut down with an ellipsis to fit `width` at font size `fsize`.
func _fit(text: String, width: float, fsize: int) -> String:
	if font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, fsize).x <= width:
		return text
	var t := text
	while t.length() > 1 and font.get_string_size(t + "…", HORIZONTAL_ALIGNMENT_LEFT, -1, fsize).x > width:
		t = t.substr(0, t.length() - 1)
	return t.strip_edges() + "…"


func _draw_right(r: Rect2) -> void:
	_panel(r, "")
	var design := current_design()
	var st := GameData.stats_for(design)
	var was := GameData.stats_for(orig_design)
	var x := r.position.x + 14
	var y := r.position.y + 26
	draw_string(font, Vector2(x, y), "PERFORMANCE", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0.6, 0.8, 1.0))
	draw_string(font, Vector2(r.end.x - 200, y), "now", HORIZONTAL_ALIGNMENT_RIGHT, 80, 13, DIM)
	draw_string(font, Vector2(r.end.x - 110, y), "new build", HORIZONTAL_ALIGNMENT_RIGHT, 96, 13, DIM)
	y += 22
	var rows := [
		["Mass", "%.1f t" % (was["mass"] / 1000.0), "%.1f t" % (st["mass"] / 1000.0), was["mass"] >= st["mass"]],
		["Top speed", "%d" % was["max_speed"], "%d" % st["max_speed"], st["max_speed"] >= was["max_speed"]],
		["Acceleration", "%d" % was["accel"], "%d" % st["accel"], st["accel"] >= was["accel"]],
		["Turn rate", "%d°/s" % was["turn"], "%d°/s" % st["turn"], st["turn"] >= was["turn"]],
		["Energy", "%d" % was["energy_max"], "%d" % st["energy_max"], st["energy_max"] >= was["energy_max"]],
		["Energy regen", "%.0f/s" % (was["energy_regen"] * 0.1), "%.0f/s" % (st["energy_regen"] * 0.1), st["energy_regen"] >= was["energy_regen"]],
		["Shields", "%d" % was["shield_max"], "%d" % st["shield_max"], st["shield_max"] >= was["shield_max"]],
		["Hull", "%d" % was["hull_max"], "%d" % st["hull_max"], st["hull_max"] >= was["hull_max"]],
		["Armor", "%d" % was["armor"], "%d" % st["armor"], st["armor"] >= was["armor"]],
		["Sensors", "%d" % was["sensor_range"], "%d" % st["sensor_range"], st["sensor_range"] >= was["sensor_range"]],
	]
	for row in rows:
		draw_string(font, Vector2(x, y), row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
		draw_string(font, Vector2(r.end.x - 200, y), row[1], HORIZONTAL_ALIGNMENT_RIGHT, 80, 14, Color(0.8, 0.8, 0.85))
		var same: bool = row[1] == row[2]
		draw_string(font, Vector2(r.end.x - 110, y), row[2], HORIZONTAL_ALIGNMENT_RIGHT, 96, 14, TEXT if same else (GOOD if row[3] else BAD))
		y += 19
	# power budget
	var gen: float = st["power_gen"]
	var draw_p: float = st["power_draw"]
	var pcol := GOOD if draw_p <= gen else BAD
	draw_string(font, Vector2(x, y), "Reactor", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(r.end.x - 300, y), "%d made / %d drawn" % [gen, draw_p], HORIZONTAL_ALIGNMENT_RIGHT, 286, 14, pcol)
	y += 8
	draw_rect(Rect2(x, y, r.size.x - 28, 6), Color(0, 0, 0, 0.5))
	draw_rect(Rect2(x, y, (r.size.x - 28) * clampf(draw_p / maxf(gen, 1.0), 0.0, 1.0), 6), pcol)
	y += 20
	if draw_p > gen:
		draw_string(font, Vector2(x, y), "Overloaded: weapons cost more and energy comes back slower.", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, BAD)
		y += 16
	var flags := []
	if st["can_cloak"]:
		flags.append("cloak")
	if st["has_boost"]:
		flags.append("booster")
	if st["qdrv"] > 0.0:
		flags.append("warp %d" % st["qdrv"])
	if st["regen"] > 0.0:
		flags.append("regen %d/s" % st["regen"])
	if st["repair"] > 0.0:
		flags.append("repair %d/s" % st["repair"])
	if st["ram"] > 0.0:
		flags.append("ram x%d" % st["ram"])
	draw_string(font, Vector2(x, y), "Systems: " + (", ".join(flags) if not flags.is_empty() else "none"), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	y += 26
	# the catalogue for the selected slot
	var title := "PARTS"
	var slot_text := ""
	match sel_kind:
		"hp":
			slot_text = "mount %d (%s)" % [sel_index + 1, GameData.class_name_of(hardpoints[sel_index][5])]
		"bay":
			slot_text = "bay slot"
		"hold":
			slot_text = "hold: click a mount or bay to fit it"
		_:
			slot_text = "click a mount or a bay on the ship"
	draw_string(font, Vector2(x, y), "%s  ·  %s" % [title, slot_text], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	var has_part := (sel_kind == "hp" and mounts[sel_index] != null) or (sel_kind == "bay" and sel_index < modules.size())
	if has_part:
		_button(Rect2(r.end.x - 110, y - 16, 96, 24), "Remove", _remove_selected, true, 13)
	y += 12
	cat_rect = Rect2(x, y, r.size.x - 28, r.end.y - y - 10)
	if sel_kind != "hp" and sel_kind != "bay":
		return
	var list := _catalogue_rows()
	var row_h := 30.0
	var visible := int(cat_rect.size.y / row_h)
	cat_scroll = clampf(cat_scroll, 0.0, maxf(list.size() - visible, 0.0))
	var start := int(cat_scroll)
	draw_rect(cat_rect, Color(0, 0, 0, 0.25))
	for k in range(start, mini(list.size(), start + visible)):
		var e: Dictionary = list[k]
		var row := Rect2(cat_rect.position.x, cat_rect.position.y + (k - start) * row_h, cat_rect.size.x, row_h - 2)
		var enabled: bool = e["ok"]
		buttons.append([row, _mount.bind(e["id"]), enabled])
		var idx := buttons.size() - 1
		draw_rect(row, Color(0.06, 0.1, 0.2, 0.7) if hover != idx or not enabled else Color(0.14, 0.24, 0.45, 0.9))
		var col := TEXT if enabled else Color(0.45, 0.45, 0.5)
		draw_string(font, row.position + Vector2(8, 19), e["name"], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, col)
		var info_w := row.size.x - 210 - _price_w(e["price"]) - 22
		draw_string(font, row.position + Vector2(210, 19), _fit(e["info"], info_w, 12), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, DIM if enabled else Color(0.4, 0.4, 0.45))
		draw_string(font, row.position + Vector2(row.size.x - 130, 19), e["price"], HORIZONTAL_ALIGNMENT_RIGHT, 122, 13, e["pcol"])
	if list.size() > visible:
		var frac := float(visible) / list.size()
		var th := cat_rect.size.y * frac
		var ty := cat_rect.position.y + (cat_rect.size.y - th) * (cat_scroll / maxf(list.size() - visible, 1.0))
		draw_rect(Rect2(cat_rect.end.x - 4, ty, 4, th), Color(0.5, 0.7, 1.0, 0.5))


## Rows of the catalogue for the selected slot: common parts, this
## empire's parts, blueprinted enemy parts, and greyed rows for the rest.
func _catalogue_rows() -> Array:
	var out := []
	var want_bay := sel_kind == "bay"
	var cls: int = hardpoints[sel_index][5] if sel_kind == "hp" else 2
	var ids: Array = GameData.MODULES.keys()
	ids.sort_custom(func(a, b):
		var fa: String = GameData.MODULES[a]["func"]
		var fb: String = GameData.MODULES[b]["func"]
		if fa != fb:
			return fa < fb
		return a < b)
	for id in ids:
		if id in GameData.STATION_ONLY:
			continue
		var mod: Dictionary = GameData.MODULES[id]
		if GameData.is_bay_part(id) != want_bay:
			continue
		var race := GameData.part_race(id)
		var locked := race >= 0 and race != base.race_id and not blueprints.has(id)
		var too_big := not want_bay and GameData.weapon_class(id) > cls
		var in_hold := hold.has(id)
		var price := GameData.price(id)
		var ok := not locked and not too_big and (in_hold or price <= credits)
		var ptxt := ""
		var pcol := GOLD
		if locked:
			ptxt = "%s blueprint" % GameData.race(race)["adj"]
			pcol = Color(0.4, 0.6, 0.65)
		elif too_big:
			ptxt = "too heavy"
			pcol = Color(0.5, 0.45, 0.45)
		elif in_hold:
			ptxt = "in hold"
			pcol = GOOD
		else:
			ptxt = "%d cr" % price
			pcol = GOLD if price <= credits else BAD
		out.append({"id": id, "name": mod["name"], "info": _part_info(id), "price": ptxt, "pcol": pcol, "ok": ok})
	return out


func _part_info(id: String) -> String:
	var mod: Dictionary = GameData.MODULES[id]
	var p: Array = mod["p"]
	match mod["func"]:
		"power":
			return "+%d power   energy %d" % [mod["power"], mod["power"] * 5]
		"eng":
			return "thrust %d   vol %d" % [p[0], mod["vol"]]
		"shield":
			return "shield %d   vol %d" % [p[0], mod["vol"]]
		"armor":
			return "armor %d  hull +%d  mass %d" % [p[0], p[0] * 4, mod["mass"]]
		"sensor":
			return "range %d" % (600 + p[0] * 40)
		"stealth":
			return "cloak   %d power" % mod["power"]
		"boost":
			return "booster %s" % ["", "I", "II", "III"][GameData.tier(id)]
		"qdrv":
			return "warp speed %d   vol %d" % [p[0], mod["vol"]]
		"brake":
			return "stronger brakes"
		"regen":
			return "heals %d hull/s when not under fire" % p[0]
		"repair":
			return "heals %d hull/s for energy" % p[0]
		"ram":
			return "x%d ram damage, hull +200" % p[0]
		"tube":
			var md: Dictionary = GameData.MISSILES[p[3]]
			var ammo: int = p[2]
			return "%s  dmg %d  %s%s" % [GameData.missile_display_name(base.race_id, p[3]).to_lower(), md["warhead"], GameData.class_name_of(GameData.weapon_class(id)),
				("  ammo %d" % ammo) if ammo < 400 else ""]
		"beam":
			return "laser  range %d  %s" % [p[2] * 2.2, GameData.class_name_of(GameData.weapon_class(id))]
	return ""
