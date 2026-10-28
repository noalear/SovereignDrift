## Builder -- the shipyard screen at a starbase or shipyard station.  Pick
## a hull, fill its hardpoints and bays from the catalogue or your salvage,
## watch the numbers change, launch.  A new hull also takes Metals, Alloys
## or Exotics from the yard's stock (GameData.HULL_MATERIALS).  Everything
## is drawn by hand like the menu.
##
## Mouse, keyboard and gamepad all work.  The screen has four sections
## (hulls, the ship's mounts and bays, the parts list, the hold); the arrows
## or D-pad move inside and across them, Tab or LB/RB jump between them.
## Parts taken off the ship go to the hold; when it is full the yard asks
## whether to sell them on the spot or not do it at all.
class_name Builder
extends Control

signal closed(launched: bool)

const BG := Color(0.012, 0.02, 0.045, 1.0)
const CARD := Color(0.04, 0.065, 0.12, 0.96)
const CARD_HI := Color(0.08, 0.13, 0.24, 0.98)
const CARD_SEL := Color(0.13, 0.24, 0.44, 0.98)
const LINE := Color(0.45, 0.62, 0.95, 0.28)
const EDGE := Color(0.5, 0.7, 1.0, 0.55)
const TEXT := Color(0.92, 0.95, 1.0)
const DIM := Color(0.58, 0.66, 0.8)
const FAINT := Color(0.42, 0.46, 0.55)
const LABEL := Color(0.55, 0.75, 1.0)
const GOOD := Color(0.45, 0.95, 0.6)
const BAD := Color(1.0, 0.45, 0.4)
const GOLD := Color(1.0, 0.84, 0.4)
const FOCUS := Color(1.0, 0.93, 0.62)

## Stock parts taken off a hull sell for this share of their price, not
## half: a new hull's stock fit is worth more than the hull costs.
const SCRAP := 0.1

const SECTIONS := ["hulls", "fit", "parts", "hold"]

## The last control the pilot touched was a gamepad: hints show pad buttons.
static var pad_mode := false

var font: Font
var ship: Ship
var base: Ship
var world: World

var hull := "Scout"
var orig_hull := "Scout"
var orig_design: Dictionary = {}
var hardpoints: Array = []          # from GameData.hull_hardpoints
var mounts: Array = []              # per hardpoint: {"id", "stock", "group"} or null
var modules: Array = []             # bay parts: {"id", "stock"}
var hold: Array = []
var hold_stock: Dictionary = {}     # id -> how many in the hold came off a stock fit
var taken: Array = []               # stock parts moved to the hold from the hull on the slip
var blueprints: Array = []
var credits := 0
var hull_paid := 0                  # credits spent on the hull this visit

var sel_kind := ""                  # "hp" | "bay" | "hold"
var sel_index := -1
var cat_mode := "upgrade"           # with a fitted part picked: "upgrade" (same kind) or "switch" (anything that fits)
var cat_scroll := 0.0
var cat_rect := Rect2()
var hold_scroll := 0
var hold_rect := Rect2()
var buttons: Array = []             # [rect, callable, enabled, focus key, pad callable]
var hover := -1
var _hp_hover := {}                 # button index -> hardpoint, for highlighting
var status := ""
var status_time := 0.0
var time := 0.0

var focus_key := ""                 # "hull:<name>", "hp:<i>", "bay:<i>", "part:<id>", "hold:<i>", "modal:<i>"
var kb_nav := false                 # show the focus ring (keys or pad in use)
var confirm: Dictionary = {}        # the open question: {"title", "text", "options": [[label, callable], ...]}
var overflow: Array = []            # [id, stock] that found the hold full during a guarded action
var sell_overflow := false
var _guard_depth := 0
var _opened_at := 0
var _repeat_dir := Vector2i.ZERO
var _repeat_t := 0.0
var _sb := StyleBoxFlat.new()


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
	for id in ship.hold_stock:
		var n := mini(int(ship.hold_stock[id]), hold.count(id))
		if n > 0:
			hold_stock[id] = n
	_opened_at = Time.get_ticks_msec()
	_load_hull(orig_hull, orig_design, true)
	focus_key = "hp:0" if not hardpoints.is_empty() else "bay:0"
	_say("Pick a mount or bay, then a part from the list.")


## Put a hull on the slip.  `fit` is the design to fill it from (the stock
## fit of that hull when empty).  Parts already on the slip that the pilot
## bought go back to the hold; stock parts stay with the old hull.
func _load_hull(name: String, fit: Dictionary, initial: bool) -> void:
	if not initial:
		_strip_to_hold()
	hull = name
	hardpoints = GameData.hull_hardpoints(name)
	if fit.is_empty():
		fit = GameData.stock_design(name, ship.race_id)
	mounts = []
	for i in hardpoints.size():
		mounts.append(null)
	for w in fit["weapons"]:
		# match the fit's weapons to hardpoints by mount angle and radius
		for i in hardpoints.size():
			if mounts[i] == null and hardpoints[i][0] == w[1] and hardpoints[i][1] == w[2]:
				mounts[i] = {"id": w[0], "stock": true, "group": GameData.weapon_group_of(w)}
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
			_to_hold(m["id"], false)
	for m in modules:
		if not m["stock"]:
			_to_hold(m["id"], false)


## A part comes off the ship into the hold.  With the hold full it is sold
## when the pilot already agreed to (sell_overflow), else noted in
## `overflow` so the guarded action can be undone and the pilot asked.
func _to_hold(id: String, stock: bool) -> void:
	if hold.size() < Ship.HOLD_SIZE:
		hold.append(id)
		if stock:
			hold_stock[id] = int(hold_stock.get(id, 0)) + 1
	elif sell_overflow:
		credits += _sale_value(id, stock)
	else:
		overflow.append([id, stock])


func _sale_value(id: String, stock: bool) -> int:
	return int(GameData.price(id) * SCRAP) if stock else GameData.price(id) / 2


## Hold row `i` is one of the stock parts (the first copies of an id count
## as the stock ones).
func _hold_is_stock(i: int) -> bool:
	var id: String = hold[i]
	return hold.slice(0, i + 1).count(id) <= int(hold_stock.get(id, 0))


## Take one `id` out of the hold; true when it was a stock part.
func _take_from_hold(id: String) -> bool:
	var stock := int(hold_stock.get(id, 0)) > 0
	hold.erase(id)
	if stock:
		_drop_stock_count(id)
	return stock


func _drop_stock_count(id: String) -> void:
	var n := int(hold_stock.get(id, 0)) - 1
	if n > 0:
		hold_stock[id] = n
	else:
		hold_stock.erase(id)


func _say(text: String) -> void:
	status = text
	status_time = time


# ------------------------------------------------- full hold: ask first

func _snapshot() -> Dictionary:
	return {"hull": hull, "hardpoints": hardpoints, "mounts": mounts.duplicate(true), "modules": modules.duplicate(true),
		"hold": hold.duplicate(), "hold_stock": hold_stock.duplicate(), "taken": taken.duplicate(), "credits": credits,
		"hull_paid": hull_paid, "sel_kind": sel_kind, "sel_index": sel_index, "cat_mode": cat_mode, "cat_scroll": cat_scroll,
		"status": status, "status_time": status_time}


func _restore(s: Dictionary) -> void:
	hull = s["hull"]
	hardpoints = s["hardpoints"]
	mounts = s["mounts"]
	modules = s["modules"]
	hold = s["hold"]
	hold_stock = s["hold_stock"]
	taken = s["taken"]
	credits = s["credits"]
	hull_paid = s["hull_paid"]
	sel_kind = s["sel_kind"]
	sel_index = s["sel_index"]
	cat_mode = s["cat_mode"]
	cat_scroll = s["cat_scroll"]
	status = s["status"]
	status_time = s["status_time"]


## Run `action`; if a part it takes off finds the hold full, undo it all and
## ask: sell what does not fit and go ahead, or leave things as they were.
func _guarded(action: Callable) -> void:
	if _guard_depth > 0 or sell_overflow:
		action.call()
		return
	var snap := _snapshot()
	overflow.clear()
	_guard_depth += 1
	action.call()
	_guard_depth -= 1
	if overflow.is_empty():
		return
	var parts := overflow.duplicate()
	overflow.clear()
	_restore(snap)
	var value := 0
	var names := []
	for p in parts:
		value += _sale_value(p[0], p[1])
		names.append(GameData.MODULES[p[0]]["name"])
	var what: String = names[0] if names.size() == 1 else "%d parts (%s)" % [names.size(), ", ".join(names)]
	_ask("Hold full", "Your hold is full (%d / %d), so %s %s nowhere to go." % [hold.size(), Ship.HOLD_SIZE, what, "has" if names.size() == 1 else "have"], [
		["Sell for %d cr" % value, func():
			sell_overflow = true
			action.call()
			sell_overflow = false
			_say("%s sold for %d credits." % [what, value])
			Sfx.play_ui("pickup", 0.4)],
		["Cancel", func(): _say("Nothing changed.")],
	])


func _ask(title: String, text: String, options: Array) -> void:
	confirm = {"title": title, "text": text, "options": options, "back": options.size() - 1}
	focus_key = "modal:0"
	Sfx.play_ui("notify", 0.5)


func _answer(i: int) -> void:
	if confirm.is_empty():
		return
	var cb: Callable = confirm["options"][i][1]
	confirm = {}
	cb.call()
	_refocus()


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
			d["weapons"].append([mounts[i]["id"], hp[0], hp[1], hp[2], hp[3], hp[4], int(mounts[i].get("group", GameData.default_weapon_group(mounts[i]["id"])))])
	return d


## The design as it would be with `id` fitted in the selected slot, for the
## numbers while a part in the list has the pointer or the focus.
func _design_with(id: String) -> Dictionary:
	var keep_m := mounts.duplicate(true)
	var keep_b := modules.duplicate(true)
	if sel_kind == "hp" and sel_index >= 0 and sel_index < mounts.size():
		mounts[sel_index] = {"id": id, "stock": false, "group": GameData.default_weapon_group(id)}
	elif sel_kind == "bay":
		var at := sel_index
		if at >= 0 and at < modules.size():
			modules[at] = {"id": id, "stock": false}
		else:
			modules.append({"id": id, "stock": false})
			at = modules.size() - 1
		var f: String = GameData.MODULES[id]["func"]
		for i in range(modules.size() - 1, -1, -1):
			var other: String = modules[i]["id"]
			if i != at and GameData.MODULES[other]["func"] == f and (not GameData.stacks(id) or (f == "shield" and GameData.shield_family(other) != GameData.shield_family(id))):
				modules.remove_at(i)
				if i < at:
					at -= 1
	var d := current_design()
	mounts = keep_m
	modules = keep_b
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
	var armed := hardpoints.is_empty()      # a trade ship has no mounts to fill
	for m in mounts:
		if m != null:
			armed = true
	if not armed:
		out.append("No weapon mounted.")
	if hull != orig_hull:
		var short := World.materials_short(world.yard_stock(base), hull)
		if short != "":
			out.append(short)
	return out


## Anything changed since the yard opened?
func _dirty() -> bool:
	return hull != orig_hull or credits != ship.credits or hold != ship.hold or current_design()["weapons"] != orig_design["weapons"] \
		or current_design()["modules"] != orig_design["modules"]


func _hull_price(name: String) -> int:
	if name == orig_hull:
		return 0
	return int(GameData.HULL_PRICE.get(name, 0))


func _switch_hull(name: String) -> void:
	_guarded(_switch_hull_now.bind(name))


func _switch_hull_now(name: String) -> void:
	if name == hull:
		return
	var cost := _hull_price(name)
	if cost - hull_paid > credits:
		_say("Not enough credits for a %s (%d)." % [name, cost])
		return
	# stock parts pulled off the hull on the slip go back with it
	var returned := 0
	for id in taken:
		if int(hold_stock.get(id, 0)) > 0 and hold.has(id):
			hold.erase(id)
			_drop_stock_count(id)
			returned += 1
	taken.clear()
	credits += hull_paid
	credits -= cost
	hull_paid = cost
	_load_hull(name, {}, false)
	Sfx.play_ui("dock", 0.5)
	_say("%s on the slip: stock fit loaded, your own parts are in the hold%s." % [GameData.hull_label(name),
		(", %d stock part%s went back with the old hull" % [returned, "" if returned == 1 else "s"]) if returned > 0 else ""])


## Mount a part into the selected slot, from the hold if there is one
## there, else bought from the catalogue.
func _mount(id: String) -> void:
	_guarded(_mount_now.bind(id))


func _mount_now(id: String) -> void:
	var from_hold := hold.has(id)
	var cost := 0 if from_hold else GameData.price(id)
	if not from_hold and cost > credits:
		_say("Not enough credits: %s costs %d." % [GameData.MODULES[id]["name"], cost])
		return
	if not GameData.fits_hull(id, hull):
		_say("%s will not fit a %s." % [GameData.MODULES[id]["name"], GameData.hull_label(hull)])
		return
	if sel_kind == "hp" and GameData.weapon_class(id) > hardpoints[sel_index][5]:
		_say("That mount only takes %s weapons." % GameData.class_name_of(hardpoints[sel_index][5]))
		return
	if sel_kind != "hp" and sel_kind != "bay":
		return
	# out of the hold first, so the part it replaces always has room there
	var stock := false
	if from_hold:
		stock = _take_from_hold(id)
	else:
		credits -= cost
	var swapped: Array = []
	if sel_kind == "hp":
		var group := GameData.default_weapon_group(id)
		var old = mounts[sel_index]
		if old != null:
			swapped.append(GameData.MODULES[old["id"]]["name"])
			if _same_family(old["id"], id):
				group = int(old.get("group", group))     # an upgrade keeps its fire group
		_unmount_hp(sel_index)
		mounts[sel_index] = {"id": id, "stock": stock, "group": group}
	elif sel_kind == "bay":
		if sel_index >= 0 and sel_index < modules.size():
			swapped.append(GameData.MODULES[modules[sel_index]["id"]]["name"])
			_unmount_bay(sel_index)
			modules.insert(sel_index, {"id": id, "stock": stock})
		else:
			modules.append({"id": id, "stock": stock})
			sel_index = modules.size() - 1
		swapped += _one_of_a_kind(sel_index)
	Sfx.play_ui("click", 0.5)
	_say("%s fitted%s%s." % [GameData.MODULES[id]["name"], " from the hold" if from_hold else " for %d credits" % cost,
		(", %s to the hold" % ", ".join(swapped)) if not swapped.is_empty() else ""])


## A ship carries one part of each kind except engines, shields and
## boosters (GameData.STACKING), and shields of one family: the part just
## fitted at `keep` pushes out the others to the hold.
func _one_of_a_kind(keep: int) -> Array:
	var id: String = modules[keep]["id"]
	var f: String = GameData.MODULES[id]["func"]
	var out := []
	var i := modules.size() - 1
	while i >= 0:
		var other: String = modules[i]["id"]
		var clash := false
		if i != keep and GameData.MODULES[other]["func"] == f:
			clash = not GameData.stacks(id) or (f == "shield" and GameData.shield_family(other) != GameData.shield_family(id))
		if clash:
			out.append(GameData.MODULES[other]["name"])
			_unmount_bay(i)
			if i < keep:
				keep -= 1
		i -= 1
	sel_index = keep
	return out


func _unmount_hp(i: int) -> void:
	var m = mounts[i]
	if m == null:
		return
	_to_hold(m["id"], m["stock"])
	if m["stock"]:
		taken.append(m["id"])
	mounts[i] = null


func _unmount_bay(i: int) -> void:
	if i < 0 or i >= modules.size():
		return
	var m: Dictionary = modules[i]
	_to_hold(m["id"], m["stock"])
	if m["stock"]:
		taken.append(m["id"])
	modules.remove_at(i)


func _remove_selected() -> void:
	_guarded(_remove_selected_now)


func _remove_selected_now() -> void:
	if sel_kind == "hp" and sel_index >= 0 and sel_index < mounts.size():
		var m = mounts[sel_index]
		if m != null:
			_say("%s moved to the hold." % GameData.MODULES[m["id"]]["name"])
			_unmount_hp(sel_index)
			Sfx.play_ui("click", 0.4)
	elif sel_kind == "bay" and sel_index >= 0 and sel_index < modules.size():
		var m: Dictionary = modules[sel_index]
		_say("%s moved to the hold." % GameData.MODULES[m["id"]]["name"])
		_unmount_bay(sel_index)
		sel_index = mini(sel_index, modules.size())
		Sfx.play_ui("click", 0.4)


const GROUP_COLORS := [Color(0.35, 0.65, 1.0), Color(1.0, 0.6, 0.3), Color(0.5, 0.95, 0.5), Color(0.9, 0.5, 0.95)]


## Move a mounted weapon to the next weapon group (1 > 2 > 3 > 4 > 1).
func _cycle_group(i: int) -> void:
	if i < 0 or i >= mounts.size() or mounts[i] == null:
		return
	var g: int = int(mounts[i].get("group", GameData.default_weapon_group(mounts[i]["id"])))
	g = g % GameData.WEAPON_GROUPS + 1
	mounts[i]["group"] = g
	Sfx.play_ui("click", 0.4)
	_say("%s now fires with group %d (keys %s - %s select a group in flight)." % [GameData.MODULES[mounts[i]["id"]]["name"], g, Keybinds.short_text("weapon_group_1"), Keybinds.short_text("weapon_group_4")])


## Two parts of one kind: the same function, for launch tubes the same
## ammunition, for sensors the same family.  An upgrade or an Equip swaps like for like.
static func _same_family(a: String, b: String) -> bool:
	var ma: Dictionary = GameData.MODULES[a]
	var mb: Dictionary = GameData.MODULES[b]
	if ma["func"] != mb["func"]:
		return false
	if ma["func"] == "sensor":
		return ma["fam"] == mb["fam"]     # radar for radar, infrared for infrared
	return ma["func"] != "tube" or ma["p"][3] == mb["p"][3]


## Where Equip would put a hold part: ["hp", mount] or ["bay", index]
## (index == modules.size() adds a bay part), or [] when it has nowhere to
## go.  It replaces a part of the same kind when there is one, otherwise
## takes an empty mount or adds to the bays if they have room.
func _equip_slot(id: String) -> Array:
	if GameData.is_bay_part(id):
		for i in modules.size():
			if _same_family(modules[i]["id"], id):
				return ["bay", i] if _bays_fit(id, i) else []
		return ["bay", modules.size()] if _bays_fit(id, -1) else []
	if not GameData.fits_hull(id, hull):
		return []
	var cls := GameData.weapon_class(id)
	for i in hardpoints.size():
		if mounts[i] != null and _same_family(mounts[i]["id"], id) and hardpoints[i][5] >= cls:
			return ["hp", i]
	for i in hardpoints.size():
		if mounts[i] == null and hardpoints[i][5] >= cls:
			return ["hp", i]
	return []


## Would the bays hold `id`, replacing bay part `replace` (or added, -1)?
func _bays_fit(id: String, replace: int) -> bool:
	var d := current_design()
	if replace >= 0:
		d["modules"][replace] = id
	else:
		d["modules"].append(id)
	var st := GameData.stats_for(d)
	return st["bay_used"] <= st["bay_cap"]


func _equip_hold(i: int) -> void:
	_guarded(_equip_hold_now.bind(i))


func _equip_hold_now(i: int) -> void:
	if i < 0 or i >= hold.size():
		return
	var id: String = hold[i]
	var slot := _equip_slot(id)
	if slot.is_empty():
		_say("Nowhere to fit %s on this hull." % GameData.MODULES[id]["name"])
		return
	sel_kind = slot[0]
	sel_index = slot[1]
	cat_mode = "upgrade"
	_mount_now(id)


func _sell_hold(i: int) -> void:
	if i < 0 or i >= hold.size():
		return
	var id: String = hold[i]
	var stock := _hold_is_stock(i)
	var v := _sale_value(id, stock)
	credits += v
	hold.remove_at(i)
	if stock:
		_drop_stock_count(id)
	Sfx.play_ui("pickup", 0.4)
	_say("%s sold for %d credits%s." % [GameData.MODULES[id]["name"], v, " (stock parts sell for scrap)" if stock else ""])
	if focus_key.begins_with("hold:") and not hold.is_empty():
		focus_key = "hold:%d" % mini(i, hold.size() - 1)


func _launch() -> void:
	var problems := _problems()
	if not problems.is_empty():
		_say(problems[0])
		Sfx.play_ui("ui_back", 0.5)
		return
	ship.credits = credits
	ship.hold = hold
	ship.hold_stock = hold_stock
	ship.blueprints = blueprints
	world.apply_build(ship, current_design())
	closed.emit(true)
	queue_free()


func _cancel() -> void:
	closed.emit(false)
	queue_free()


## Leave without launching; asks first when something would be lost.
func _leave() -> void:
	if not _dirty():
		_cancel()
		return
	_ask("Leave the shipyard?", "Your changes to the ship, the hold and your credits will be undone.", [
		["Leave", _cancel],
		["Keep building", func(): pass],
	])


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
		if event.relative.length() > 2.0:
			kb_nav = false
			pad_mode = false
		if hover >= 0 and buttons[hover][3] != "":
			focus_key = buttons[hover][3]
	elif event is InputEventMouseButton and event.pressed:
		pad_mode = false
		if event.button_index == MOUSE_BUTTON_LEFT:
			for i in range(buttons.size() - 1, -1, -1):
				if buttons[i][0].has_point(event.position) and buttons[i][2]:
					buttons[i][1].call()
					accept_event()
					return
		elif event.button_index in [MOUSE_BUTTON_WHEEL_UP, MOUSE_BUTTON_WHEEL_DOWN]:
			var d := -1 if event.button_index == MOUSE_BUTTON_WHEEL_UP else 1
			if cat_rect.has_point(event.position):
				cat_scroll = maxf(cat_scroll + d * 3.0, 0.0)
			elif hold_rect.has_point(event.position):
				hold_scroll = maxi(hold_scroll + d * 2, 0)


func _input(event: InputEvent) -> void:
	if event.is_action_pressed("refit"):
		get_viewport().set_input_as_handled()
		return
	# the press that opened the yard is not a shipyard command
	if Time.get_ticks_msec() - _opened_at < 150:
		if not event is InputEventMouse:
			get_viewport().set_input_as_handled()
		return
	var cmd := ""
	if event is InputEventKey and event.pressed:
		pad_mode = false
		var k: Key = event.physical_keycode
		match k:
			KEY_UP, KEY_W: cmd = "up"
			KEY_DOWN, KEY_S: cmd = "down"
			KEY_LEFT, KEY_A: cmd = "left"
			KEY_RIGHT, KEY_D: cmd = "right"
			KEY_ENTER, KEY_KP_ENTER:
				cmd = "launch" if event.ctrl_pressed else "accept"
			KEY_SPACE: cmd = "accept"
			KEY_ESCAPE, KEY_BACKSPACE: cmd = "back"
			KEY_TAB: cmd = "prev" if event.shift_pressed else "next"
			KEY_Q: cmd = "prev"
			KEY_E: cmd = "next"
			KEY_X: cmd = "switch"
			KEY_DELETE: cmd = "remove"
			KEY_G: cmd = "group"
			KEY_L: cmd = "launch"
		if event.echo and not cmd in ["up", "down", "left", "right"]:
			cmd = ""
		if cmd == "" and event.is_action_pressed("pause_menu"):
			cmd = "back"
	elif event is InputEventJoypadButton and event.pressed:
		pad_mode = true
		match event.button_index:
			JOY_BUTTON_A: cmd = "accept"
			JOY_BUTTON_B: cmd = "back"
			JOY_BUTTON_X: cmd = "switch"
			JOY_BUTTON_Y: cmd = "remove"
			JOY_BUTTON_BACK: cmd = "group"
			JOY_BUTTON_START: cmd = "launch"
			JOY_BUTTON_LEFT_SHOULDER: cmd = "prev"
			JOY_BUTTON_RIGHT_SHOULDER: cmd = "next"
			JOY_BUTTON_DPAD_UP: cmd = "up"
			JOY_BUTTON_DPAD_DOWN: cmd = "down"
			JOY_BUTTON_DPAD_LEFT: cmd = "left"
			JOY_BUTTON_DPAD_RIGHT: cmd = "right"
		if cmd in ["up", "down", "left", "right"]:
			_repeat_dir = _dir_of(cmd)
			_repeat_t = 0.4
	elif event is InputEventJoypadMotion and absf(event.axis_value) > 0.6 and event.axis in [JOY_AXIS_LEFT_X, JOY_AXIS_LEFT_Y]:
		pad_mode = true
		var d := _stick_dir()
		if d != Vector2i.ZERO and d != _repeat_dir:
			_repeat_dir = d
			_repeat_t = 0.4
			_command("left" if d.x < 0 else ("right" if d.x > 0 else ("up" if d.y < 0 else "down")))
		get_viewport().set_input_as_handled()
		return
	if cmd != "":
		_command(cmd)
		get_viewport().set_input_as_handled()
	elif event is InputEventKey or event is InputEventJoypadButton:
		get_viewport().set_input_as_handled()      # nothing behind the yard acts on keys


static func _dir_of(cmd: String) -> Vector2i:
	return {"up": Vector2i.UP, "down": Vector2i.DOWN, "left": Vector2i.LEFT, "right": Vector2i.RIGHT}[cmd]


func _stick_dir() -> Vector2i:
	var v := Vector2(Input.get_joy_axis(0, JOY_AXIS_LEFT_X), Input.get_joy_axis(0, JOY_AXIS_LEFT_Y))
	if v.length() < 0.55:
		return Vector2i.ZERO
	if absf(v.x) > absf(v.y):
		return Vector2i(int(signf(v.x)), 0)
	return Vector2i(0, int(signf(v.y)))


## One shipyard command from the keyboard or a gamepad.
func _command(cmd: String) -> void:
	var was_hidden := not kb_nav
	kb_nav = true
	hover = -1
	if not confirm.is_empty():
		match cmd:
			"left", "up", "prev":
				_move_modal(-1)
			"right", "down", "next":
				_move_modal(1)
			"accept":
				_answer(int(focus_key.trim_prefix("modal:")) if focus_key.begins_with("modal:") else 0)
			"back":
				_answer(confirm["back"])
		return
	match cmd:
		"up", "down", "left", "right":
			if was_hidden and _focus_index() >= 0:
				return             # the first press just shows where the focus is
			_move_focus(_dir_of(cmd))
		"next":
			_jump_section(1)
		"prev":
			_jump_section(-1)
		"accept":
			if was_hidden and _focus_index() >= 0:
				return
			_activate_focus()
		"back":
			_back()
		"switch":
			if not _selected_part().is_empty():
				cat_mode = "switch" if cat_mode == "upgrade" else "upgrade"
				cat_scroll = 0.0
				Sfx.play_ui("click", 0.4)
		"remove":
			if focus_key.begins_with("hold:"):
				_sell_hold(int(focus_key.trim_prefix("hold:")))
			else:
				var slot := _focus_slot()
				if not slot.is_empty():
					sel_kind = slot[0]
					sel_index = slot[1]
				_remove_selected()
		"group":
			var slot := _focus_slot()
			if slot.is_empty() and sel_kind == "hp":
				slot = ["hp", sel_index]
			if not slot.is_empty() and slot[0] == "hp":
				_cycle_group(slot[1])
		"launch":
			_launch()
	_refocus()


## The mount or bay the focus is on: ["hp", i], ["bay", i] or [].
func _focus_slot() -> Array:
	for kind in ["hp", "bay"]:
		if focus_key.begins_with(kind + ":"):
			return [kind, int(focus_key.get_slice(":", 1))]
	return []


func _section_of(key: String) -> String:
	match key.get_slice(":", 0):
		"hull": return "hulls"
		"hp", "bay": return "fit"
		"part": return "parts"
		"hold": return "hold"
	return ""


## Back out one step: from the parts list to its slot, from a slot to no
## slot, then out of the yard.
func _back() -> void:
	Sfx.play_ui("ui_back", 0.5)
	var sec := _section_of(focus_key)
	if sec == "parts" and (sel_kind == "hp" or sel_kind == "bay"):
		focus_key = "%s:%d" % [sel_kind, sel_index]
	elif sec == "hold" and sel_kind == "hold":
		sel_kind = ""
		sel_index = -1
	elif sec != "fit" and sec != "":
		focus_key = "%s:%d" % [sel_kind, sel_index] if sel_kind == "hp" or sel_kind == "bay" else ("hp:0" if not hardpoints.is_empty() else "bay:0")
	elif sel_kind != "":
		sel_kind = ""
		sel_index = -1
	else:
		_leave()


func _activate_focus() -> void:
	var i := _focus_index()
	if i < 0:
		return
	var b: Array = buttons[i]
	if not b[2]:
		Sfx.play_ui("ui_back", 0.4)
		return
	var key: String = b[3]
	var cb: Callable = b[4] if b[4].is_valid() else b[1]
	cb.call()
	if confirm.is_empty():
		# a picked slot hands the focus to its parts; a fitted part hands it back
		if key.begins_with("hp:") or key.begins_with("bay:"):
			var rows := _catalogue_rows()
			if rows.is_empty() and not _selected_part().is_empty():
				cat_mode = "switch"         # nothing better: show what else fits
				rows = _catalogue_rows()
			for e in rows:
				if e["ok"]:
					focus_key = "part:" + e["id"]
					break
		elif key.begins_with("part:") and (sel_kind == "hp" or sel_kind == "bay"):
			focus_key = "%s:%d" % [sel_kind, sel_index]
	_refocus()


func _focus_index() -> int:
	for i in buttons.size():
		if buttons[i][3] == focus_key and focus_key != "":
			return i
	return -1


## The ordered keys of a list section, all of them (drawn or scrolled away).
func _list_keys(sec: String) -> Array:
	var out := []
	match sec:
		"hulls":
			for n in GameData.yard_hulls():
				out.append("hull:" + n)
		"parts":
			if sel_kind == "hp" or sel_kind == "bay":
				for e in _catalogue_rows():
					out.append("part:" + e["id"])
		"hold":
			for i in hold.size():
				out.append("hold:%d" % i)
		"fit":
			for i in hardpoints.size():
				out.append("hp:%d" % i)
			for i in modules.size() + 1:
				out.append("bay:%d" % i)
	return out


## Arrows and D-pad: up and down walk a list one row at a time (scrolling
## it), anything else goes to the nearest control that way on screen.
func _move_focus(dir: Vector2i) -> void:
	var sec := _section_of(focus_key)
	if dir.y != 0 and sec in ["hulls", "parts", "hold"]:
		var keys := _list_keys(sec)
		var at := keys.find(focus_key)
		if at >= 0 and at + dir.y >= 0 and at + dir.y < keys.size():
			focus_key = keys[at + dir.y]
			_scroll_to_focus()
			Sfx.play_ui("click", 0.2)
		return
	var i := _focus_index()
	if i < 0:
		_refocus()
		return
	var cur: Rect2 = buttons[i][0]
	var c := cur.get_center()
	var best := -1
	var best_score := INF
	for k in buttons.size():
		var key: String = buttons[k][3]
		if k == i or key == "" or key.begins_with("modal:"):
			continue
		var r: Rect2 = buttons[k][0]
		var d := r.get_center() - c
		var along := d.x * dir.x + d.y * dir.y
		var across := absf(d.x * dir.y) + absf(d.y * dir.x)
		# must lie that way and not overlap the current one along it
		if along <= 4.0:
			continue
		if dir.x != 0 and (r.position.x - cur.end.x) * dir.x < -cur.size.x * 0.5:
			continue
		if dir.y != 0 and (r.position.y - cur.end.y if dir.y > 0 else cur.position.y - r.end.y) < -4.0:
			continue
		var score := along + across * 2.5
		if score < best_score:
			best_score = score
			best = k
	if best >= 0:
		focus_key = buttons[best][3]
		_scroll_to_focus()
		Sfx.play_ui("click", 0.2)


## LB/RB, Tab: hulls > ship > parts > hold, skipping a section with
## nothing in it.
func _jump_section(step: int) -> void:
	var at := SECTIONS.find(_section_of(focus_key))
	for n in SECTIONS.size():
		at = posmod(at + step, SECTIONS.size())
		var sec: String = SECTIONS[at]
		var keys := _list_keys(sec)
		if keys.is_empty():
			continue
		match sec:
			"hulls":
				focus_key = "hull:" + hull
			"fit":
				focus_key = "%s:%d" % [sel_kind, sel_index] if sel_kind == "hp" or sel_kind == "bay" else keys[0]
			_:
				focus_key = keys[0]
		_scroll_to_focus()
		Sfx.play_ui("click", 0.3)
		return


func _move_modal(step: int) -> void:
	var n: int = confirm["options"].size()
	var at := int(focus_key.trim_prefix("modal:")) if focus_key.begins_with("modal:") else 0
	focus_key = "modal:%d" % posmod(at + step, n)
	Sfx.play_ui("click", 0.2)


## Keep the focus on something that exists after the screen changed.
func _refocus() -> void:
	if not confirm.is_empty():
		if not focus_key.begins_with("modal:"):
			focus_key = "modal:0"
		return
	var sec := _section_of(focus_key)
	var keys := _list_keys(sec)
	if keys.has(focus_key):
		_scroll_to_focus()
		return
	if sec == "hold" and not hold.is_empty():
		focus_key = "hold:%d" % (hold.size() - 1)
	elif sel_kind == "hp" or sel_kind == "bay":
		focus_key = "%s:%d" % [sel_kind, sel_index]
	else:
		focus_key = "hp:0" if not hardpoints.is_empty() else "bay:0"
	_scroll_to_focus()


func _scroll_to_focus() -> void:
	if focus_key.begins_with("part:"):
		var at := _list_keys("parts").find(focus_key)
		var visible := maxi(int(cat_rect.size.y / CAT_ROW), 1)
		if at >= 0:
			cat_scroll = clampf(cat_scroll, at - visible + 1, at)
	elif focus_key.begins_with("hold:"):
		var at := int(focus_key.trim_prefix("hold:"))
		var visible := maxi(int(hold_rect.size.y / HOLD_ROW), 1)
		hold_scroll = clampi(hold_scroll, at - visible + 1, at)


func _process(delta: float) -> void:
	time += delta
	# a held D-pad or stick repeats
	if _repeat_dir != Vector2i.ZERO:
		var still := _stick_dir() == _repeat_dir
		var btn: int = {Vector2i.UP: JOY_BUTTON_DPAD_UP, Vector2i.DOWN: JOY_BUTTON_DPAD_DOWN, Vector2i.LEFT: JOY_BUTTON_DPAD_LEFT, Vector2i.RIGHT: JOY_BUTTON_DPAD_RIGHT}[_repeat_dir]
		still = still or Input.is_joy_button_pressed(0, btn)
		if not still:
			_repeat_dir = Vector2i.ZERO
		else:
			_repeat_t -= delta
			if _repeat_t <= 0.0:
				_repeat_t = 0.11
				_command({Vector2i.UP: "up", Vector2i.DOWN: "down", Vector2i.LEFT: "left", Vector2i.RIGHT: "right"}[_repeat_dir])
	queue_redraw()


# --------------------------------------------------------------- drawing

const CAT_ROW := 38.0
const HOLD_ROW := 31.0


## A rounded card.
func _card(rect: Rect2, fill: Color, border: Color = LINE, radius: float = 8.0, bw: float = 1.0) -> void:
	_sb.bg_color = fill
	_sb.border_color = border
	_sb.set_border_width_all(int(bw))
	_sb.set_corner_radius_all(int(radius))
	_sb.anti_aliasing = true
	_sb.shadow_size = 0
	draw_style_box(_sb, rect)


func _focus_ring(rect: Rect2, key: String) -> void:
	if kb_nav and key != "" and key == focus_key:
		_card(rect.grow(3), Color(0, 0, 0, 0), Color(FOCUS.r, FOCUS.g, FOCUS.b, 0.7 + 0.3 * sin(time * 6.0)), 10.0, 2.0)


func _button(rect: Rect2, label: String, cb: Callable, enabled: bool = true, size: int = 16, accent: bool = false, key: String = "", glyph: String = "") -> void:
	buttons.append([rect, cb, enabled, key, Callable()])
	var idx := buttons.size() - 1
	var lit := hover == idx
	var bg := CARD_HI if not lit else CARD_SEL
	var edge := EDGE
	if accent:
		bg = Color(0.08, 0.32, 0.2, 0.98) if not lit else Color(0.12, 0.45, 0.28, 1.0)
		edge = Color(0.45, 1.0, 0.65, 0.8)
	if not enabled:
		bg = Color(0.07, 0.08, 0.1, 0.8)
		edge = Color(0.5, 0.55, 0.65, 0.25)
	_card(rect, bg, edge, 7.0)
	var tw := font.get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x
	var gw := _glyph_w(glyph, size - 3) + 8.0 if glyph != "" else 0.0
	var x := rect.position.x + (rect.size.x - tw - gw) * 0.5
	var base_y := rect.position.y + rect.size.y * 0.5 + size * 0.36
	if glyph != "":
		_glyph(Vector2(x, rect.position.y + rect.size.y * 0.5), glyph, size - 3, enabled)
		x += gw
	draw_string(font, Vector2(x, base_y), label, HORIZONTAL_ALIGNMENT_LEFT, -1, size, TEXT if enabled else FAINT)
	_focus_ring(rect, key)


## A key or pad button picture, left edge at `pos.x`, centred on `pos.y`.
## Returns its width.
func _glyph(pos: Vector2, g: String, size: int = 13, enabled: bool = true) -> float:
	var w := _glyph_w(g, size)
	var h := size + 9.0
	var r := Rect2(pos.x, pos.y - h * 0.5, w, h)
	var face := {"A": Color(0.3, 0.8, 0.4), "B": Color(0.95, 0.35, 0.3), "X": Color(0.3, 0.55, 1.0), "Y": Color(1.0, 0.8, 0.25)}
	var a := 1.0 if enabled else 0.4
	if pad_mode and face.has(g):
		draw_circle(r.get_center(), h * 0.5, Color(0.05, 0.07, 0.12, a))
		draw_arc(r.get_center(), h * 0.5, 0, TAU, 24, Color(face[g], a), 1.6, true)
		draw_string(font, Vector2(r.position.x, r.position.y + h * 0.5 + size * 0.36), g, HORIZONTAL_ALIGNMENT_CENTER, w, size, Color(face[g], a))
	else:
		_card(r, Color(0.1, 0.13, 0.2, a), Color(0.75, 0.82, 0.95, 0.55 * a), 4.0)
		draw_string(font, Vector2(r.position.x, r.position.y + h * 0.5 + size * 0.36), g, HORIZONTAL_ALIGNMENT_CENTER, w, size, Color(0.92, 0.95, 1.0, a))
	return w


func _glyph_w(g: String, size: int) -> float:
	if pad_mode and g in ["A", "B", "X", "Y"]:
		return size + 9.0
	return maxf(font.get_string_size(g, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x + 12.0, size + 9.0)


## The key or pad button for a shipyard command, as the hints show it.
func _key(cmd: String) -> String:
	if pad_mode:
		return {"accept": "A", "back": "B", "switch": "X", "remove": "Y", "group": "Back", "launch": "Start", "section": "LB / RB", "move": "D-pad"}[cmd]
	return {"accept": "Enter", "back": "Esc", "switch": "X", "remove": "Del", "group": "G", "launch": "L", "section": "Tab", "move": "Arrows"}[cmd]


func _section_label(pos: Vector2, text: String, extra: String = "") -> void:
	draw_string(font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, LABEL)
	if extra != "":
		var w := font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, 14).x
		draw_string(font, pos + Vector2(w + 10, 0), extra, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)


## A part's picture from the parts sheet, fitted inside `box`.
func _icon(id: String, box: Rect2, alpha: float = 1.0) -> void:
	var tex := ShipArt.part_texture(ShipArt.module_icon(id))
	if tex == null:
		return
	var ts := Vector2(tex.get_size())
	var s := minf(box.size.x / ts.x, box.size.y / ts.y)
	var size := ts * s
	var tint := ShipArt.part_tint(base.race_id)
	tint.a = alpha
	draw_texture_rect(tex, Rect2(box.get_center() - size * 0.5, size), false, tint)


## Colour per kind of part, for the bay bar and the icon wells.
static func _func_color(id: String) -> Color:
	match GameData.MODULES[id]["func"]:
		"beam", "tube": return Color(1.0, 0.55, 0.3)
		"power": return Color(1.0, 0.85, 0.35)
		"eng", "boost", "brake", "qdrv": return Color(0.4, 0.7, 1.0)
		"shield": return Color(0.35, 0.95, 1.0)
		"armor", "ram": return Color(0.75, 0.78, 0.85)
		"sensor", "jammer", "heatsink": return Color(0.5, 1.0, 0.6)
	return Color(0.85, 0.55, 1.0)


func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), BG)
	var rc: Color = GameData.race(base.race_id)["color"]
	# a faint race-coloured wash from the top
	for i in 6:
		draw_rect(Rect2(0, i * 14.0, vp.x, 14.0), Color(rc.r, rc.g, rc.b, 0.035 * (6 - i) / 6.0))
	var m := 16.0
	var header := Rect2(m, m, vp.x - 2 * m, 64)
	var footer := Rect2(m, vp.y - m - 40, vp.x - 2 * m, 40)
	var top := header.end.y + 10
	var bottom := footer.position.y - 10
	var left := Rect2(m, top, 372, bottom - top)
	var right := Rect2(vp.x - m - 540, top, 540, bottom - top)
	var centre := Rect2(left.end.x + 10, top, right.position.x - left.end.x - 20, bottom - top)
	_draw_header(header, rc)
	_draw_left(left, rc)
	_draw_centre(centre, rc)
	_draw_right(right)
	_draw_footer(footer)
	if not confirm.is_empty():
		_draw_modal(vp)


func _draw_header(r: Rect2, rc: Color) -> void:
	_card(r, CARD, LINE, 10.0)
	draw_rect(Rect2(r.position.x + 1, r.position.y + 12, 4, r.size.y - 24), rc)
	draw_string(font, r.position + Vector2(22, 30), "SHIPYARD", HORIZONTAL_ALIGNMENT_LEFT, -1, 24, rc)
	draw_string(font, r.position + Vector2(22, 52), "%s   ·   %s %s" % [base.ship_name, GameData.rank_name(ship.rank), ship.pilot_name], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	# credits and the yard's building stock, in pills
	var x := r.position.x + 330
	var cr := "%d cr" % credits
	var cw := font.get_string_size(cr, HORIZONTAL_ALIGNMENT_LEFT, -1, 22).x + 48
	var pill := Rect2(x, r.position.y + 13, cw, 38)
	_card(pill, Color(0.12, 0.1, 0.04, 0.9), Color(GOLD.r, GOLD.g, GOLD.b, 0.5), 19.0)
	draw_circle(pill.position + Vector2(19, 19), 8.0, GOLD)
	draw_circle(pill.position + Vector2(19, 19), 4.5, Color(0.55, 0.42, 0.1))
	draw_string(font, pill.position + Vector2(34, 27), cr, HORIZONTAL_ALIGNMENT_LEFT, -1, 22, GOLD)
	x = pill.end.x + 14
	var stock := world.yard_stock(base)
	draw_string(font, Vector2(x, r.position.y + 37), "Yard stock", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	x += 76
	for k in ["metals", "alloys", "exotics"]:
		var t := "%s %d" % [Economy.NAMES[k], int(stock.get(k, 0))]
		var w := font.get_string_size(t, HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x + 30
		var p := Rect2(x, r.position.y + 20, w, 24)
		_card(p, Color(0.06, 0.09, 0.15, 0.9), Color(0.5, 0.6, 0.8, 0.25), 12.0)
		draw_circle(p.position + Vector2(12, 12), 4.5, Economy.COLORS[k])
		draw_string(font, p.position + Vector2(22, 17), t, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, TEXT)
		x += w + 6
	var problems := _problems()
	_button(Rect2(r.end.x - 316, r.position.y + 12, 170, 40), "Launch", _launch, problems.is_empty(), 18, true, "", _key("launch"))
	_button(Rect2(r.end.x - 138, r.position.y + 12, 124, 40), "Leave", _leave, true, 18, false, "", _key("back") if sel_kind == "" else "")


func _draw_footer(r: Rect2) -> void:
	_card(r, CARD, LINE, 10.0)
	var x := r.position.x + 14
	var cy := r.position.y + r.size.y * 0.5
	for h in _hints():
		x += _glyph(Vector2(x, cy), h[0], 12) + 6
		draw_string(font, Vector2(x, cy + 5), h[1], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
		x += font.get_string_size(h[1], HORIZONTAL_ALIGNMENT_LEFT, -1, 13).x + 18
	# the status line, or what stops a launch
	var problems := _problems()
	var sw := r.end.x - 14 - (x + 10)
	if sw < 120:
		return
	var age := time - status_time
	var a := clampf(1.0 - (age - 6.0) / 2.0, 0.0, 1.0)
	if not problems.is_empty():
		draw_string(font, Vector2(x + 10, cy + 5), _fit("Can't launch: " + problems[0], sw, 14), HORIZONTAL_ALIGNMENT_RIGHT, sw, 14, BAD)
	elif a > 0.0:
		draw_string(font, Vector2(x + 10, cy + 5), _fit(status, sw, 14), HORIZONTAL_ALIGNMENT_RIGHT, sw, 14, Color(0.85, 0.9, 1.0, a))


## What the keys and buttons do right now, for the bar at the bottom.
func _hints() -> Array:
	if not confirm.is_empty():
		return [[_key("accept"), "Choose"], [_key("back"), confirm["options"][confirm["back"]][0]]]
	var out := [[_key("move"), "Move"]]
	var sec := _section_of(focus_key)
	var slot := _focus_slot()
	match sec:
		"hulls":
			out.append([_key("accept"), "Put on the slip"])
		"fit":
			out.append([_key("accept"), "Choose part"])
		"parts":
			out.append([_key("accept"), "Fit"])
		"hold":
			out.append([_key("accept"), "Equip"])
			out.append([_key("remove"), "Sell"])
	if sec == "fit" and not slot.is_empty() and ((slot[0] == "hp" and slot[1] < mounts.size() and mounts[slot[1]] != null) or (slot[0] == "bay" and slot[1] < modules.size())):
		out.append([_key("remove"), "To hold"])
		if slot[0] == "hp":
			out.append([_key("group"), "Fire group"])
	if not _selected_part().is_empty():
		out.append([_key("switch"), "Upgrades" if cat_mode == "switch" else "Switch out"])
	out.append([_key("section"), "Section"])
	out.append([_key("back"), "Back" if sel_kind != "" or sec != "fit" else "Leave"])
	out.append([_key("launch"), "Launch"])
	return out


func _draw_modal(vp: Vector2) -> void:
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0, 0, 0, 0.6))
	var w := 600.0
	var lines := _wrap(confirm["text"], w - 56, 16)
	var h := 150.0 + lines.size() * 22.0
	var r := Rect2(vp.x * 0.5 - w * 0.5, vp.y * 0.5 - h * 0.5, w, h)
	_card(r, Color(0.05, 0.08, 0.15, 0.99), Color(GOLD.r, GOLD.g, GOLD.b, 0.6), 12.0, 2.0)
	draw_string(font, r.position + Vector2(28, 42), confirm["title"], HORIZONTAL_ALIGNMENT_LEFT, -1, 22, GOLD)
	var y := r.position.y + 76
	for ln in lines:
		draw_string(font, Vector2(r.position.x + 28, y), ln, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
		y += 22
	var opts: Array = confirm["options"]
	var bw := 200.0
	var bx := r.end.x - 28 - opts.size() * (bw + 12) + 12
	for i in opts.size():
		var br := Rect2(bx + i * (bw + 12), r.end.y - 62, bw, 42)
		_button(br, opts[i][0], _answer.bind(i), true, 17, i == 0, "modal:%d" % i)
	# only the question takes clicks now
	var keep := buttons.filter(func(b): return b[3].begins_with("modal:"))
	buttons.clear()
	buttons.append_array(keep)


func _wrap(text: String, width: float, fsize: int) -> Array:
	var out := []
	var line := ""
	for word in text.split(" "):
		var t := word if line == "" else line + " " + word
		if font.get_string_size(t, HORIZONTAL_ALIGNMENT_LEFT, -1, fsize).x > width and line != "":
			out.append(line)
			line = word
		else:
			line = t
	if line != "":
		out.append(line)
	return out


func _draw_left(r: Rect2, rc: Color) -> void:
	_card(r, CARD, LINE, 10.0)
	_section_label(r.position + Vector2(16, 28), "HULLS")
	var y := r.position.y + 42
	var rows: Array = GameData.yard_hulls()
	var row_h := clampf((r.size.y * 0.52 - 42.0) / rows.size(), 34.0, 52.0)
	for name in rows:
		var d: Dictionary = GameData.SHIPS[name]
		var allowed: bool = d["rank"] <= ship.rank
		var row := Rect2(r.position.x + 10, y, r.size.x - 20, row_h - 5.0)
		var selected: bool = name == hull
		var key: String = "hull:" + name
		buttons.append([row, _switch_hull.bind(name), allowed, key, Callable()])
		var idx := buttons.size() - 1
		var bg := CARD_SEL if selected else (CARD_HI if hover == idx else Color(0.055, 0.085, 0.15, 0.9))
		if not allowed:
			bg = Color(0.05, 0.055, 0.07, 0.8)
		_card(row, bg, Color(rc.r, rc.g, rc.b, 0.7) if selected else Color(0.5, 0.65, 0.95, 0.18), 8.0)
		if selected:
			draw_rect(Rect2(row.position.x + 1, row.position.y + 8, 3, row.size.y - 16), rc)
		# a little picture of the hull
		var cy := row.position.y + row.size.y * 0.5
		ShipArt.draw_ship(self, name, base.race_id, 11.0 + d["size"] * 0.11, rc, 0.0, 1.0 if allowed else 0.3, 0.0, Transform2D(0.0, Vector2(row.position.x + 38, cy)))
		draw_string(font, Vector2(row.position.x + 76, cy - 3), GameData.hull_label(name), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT if allowed else FAINT)
		var sub := "%s · %d mounts · bays %d" % [GameData.rank_name(d["rank"]), GameData.hull_hardpoints(name).size(), GameData.HULL_BAYS[name]] if allowed else "needs %s" % GameData.rank_name(d["rank"])
		var mats_w := 0.0
		var price := _hull_price(name)
		var ptxt := "current" if name == orig_hull else ("%d cr" % price)
		draw_string(font, Vector2(row.end.x - 112, cy - 3), ptxt, HORIZONTAL_ALIGNMENT_RIGHT, 100, 15, GOLD if name != orig_hull and allowed else DIM)
		if name != orig_hull and GameData.HULL_MATERIALS.has(name):
			var mats := []
			var need: Dictionary = GameData.HULL_MATERIALS[name]
			for k in need:
				mats.append("%d %s" % [need[k], Economy.NAMES[k].left(2)])
			var ok := World.materials_short(world.yard_stock(base), name) == ""
			draw_string(font, Vector2(row.end.x - 152, cy + 14), " ".join(mats), HORIZONTAL_ALIGNMENT_RIGHT, 140, 12, Color(0.72, 0.82, 1.0) if ok else BAD)
			mats_w = font.get_string_size(" ".join(mats), HORIZONTAL_ALIGNMENT_LEFT, -1, 12).x + 18
		draw_string(font, Vector2(row.position.x + 76, cy + 14), _fit(sub, row.size.x - 88 - mats_w, 12), HORIZONTAL_ALIGNMENT_LEFT, -1, 12, DIM if allowed else FAINT)
		_focus_ring(row, key)
		y += row_h
	# the hold
	y += 14
	draw_line(Vector2(r.position.x + 16, y - 7), Vector2(r.end.x - 16, y - 7), LINE, 1.0)
	_section_label(Vector2(r.position.x + 16, y + 16), "HOLD", "%d / %d   salvage, free to fit" % [hold.size(), Ship.HOLD_SIZE])
	# a cell per hold place
	var cells := Ship.HOLD_SIZE
	var cw := (r.size.x - 32 - (cells - 1) * 2) / cells
	for i in cells:
		draw_rect(Rect2(r.position.x + 16 + i * (cw + 2), y + 24, cw, 4), (GOLD if hold.size() >= cells else LABEL) if i < hold.size() else Color(1, 1, 1, 0.08))
	y += 36
	hold_rect = Rect2(r.position.x + 10, y, r.size.x - 20, r.end.y - y - 8)
	if hold.is_empty():
		draw_string(font, Vector2(r.position.x + 16, y + 18), "Empty. Wrecks drop parts: fly over them.", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
		draw_string(font, Vector2(r.position.x + 16, y + 38), "Parts you take off the ship land here.", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
		return
	var visible := maxi(int(hold_rect.size.y / HOLD_ROW), 1)
	hold_scroll = clampi(hold_scroll, 0, maxi(hold.size() - visible, 0))
	for i in range(hold_scroll, mini(hold.size(), hold_scroll + visible)):
		var id: String = hold[i]
		var row := Rect2(hold_rect.position.x, y, hold_rect.size.x, HOLD_ROW - 4)
		var selected := sel_kind == "hold" and sel_index == i
		var key := "hold:%d" % i
		buttons.append([row, _select.bind("hold", i), true, key, _equip_hold.bind(i)])
		var idx := buttons.size() - 1
		_card(row, CARD_SEL if selected else (CARD_HI if hover == idx else Color(0.055, 0.085, 0.15, 0.9)), Color(0.5, 0.65, 0.95, 0.18), 6.0)
		_icon(id, Rect2(row.position.x + 6, row.position.y + 2, 30, row.size.y - 4))
		var stock := _hold_is_stock(i)
		var name: String = GameData.MODULES[id]["name"]
		draw_string(font, row.position + Vector2(42, 18), _fit(name, row.size.x - 190 - (40 if stock else 0), 14), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, TEXT)
		if stock:
			var nw := minf(font.get_string_size(name, HORIZONTAL_ALIGNMENT_LEFT, -1, 14).x, row.size.x - 230)
			draw_string(font, row.position + Vector2(48 + nw, 18), "stock", HORIZONTAL_ALIGNMENT_LEFT, -1, 11, FAINT)
		_button(Rect2(row.end.x - 136, row.position.y + 3, 56, row.size.y - 6), "Equip", _equip_hold.bind(i), not _equip_slot(id).is_empty(), 12)
		_button(Rect2(row.end.x - 76, row.position.y + 3, 72, row.size.y - 6), "Sell %d" % _sale_value(id, stock), _sell_hold.bind(i), true, 12)
		_focus_ring(row, key)
		y += HOLD_ROW
	if hold.size() > visible:
		var th := hold_rect.size.y * visible / hold.size()
		var ty := hold_rect.position.y + (hold_rect.size.y - th) * hold_scroll / maxf(hold.size() - visible, 1)
		draw_rect(Rect2(hold_rect.end.x + 3, ty, 3, th), Color(0.5, 0.7, 1.0, 0.5))


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
	cat_mode = "upgrade"


func _draw_centre(r: Rect2, rc: Color) -> void:
	_card(r, CARD, LINE, 10.0)
	var design := current_design()
	var st := GameData.stats_for(design)
	draw_string(font, r.position + Vector2(18, 34), "%s %s" % [GameData.race(base.race_id)["adj"], GameData.hull_label(hull)], HORIZONTAL_ALIGNMENT_LEFT, -1, 24, rc)
	var hw := font.get_string_size("%s %s" % [GameData.race(base.race_id)["adj"], GameData.hull_label(hull)], HORIZONTAL_ALIGNMENT_LEFT, -1, 24).x
	draw_string(font, r.position + Vector2(30 + hw, 33), "%d mounts  ·  %d bay volume  ·  %s" % [hardpoints.size(), st["bay_cap"], GameData.coat_label(st["coat"]) + " armor" if st["coat"] != "" else "no armor coat"], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	# which mount the pointer or focus is on, on the ship or in the list
	# (from last frame's buttons, which share this frame's layout)
	var hot: int = _hp_hover.get(hover, -1)
	if kb_nav and focus_key.begins_with("hp:"):
		hot = int(focus_key.trim_prefix("hp:"))
	_hp_hover.clear()
	# the lists below the picture take what they need; the ship gets the rest
	var mount_cols := 4
	var bay_cols := 3
	var mount_rows := ceili(hardpoints.size() / float(mount_cols))
	var bay_rows := ceili((modules.size() + 1) / float(bay_cols))
	var lists_h := 30.0 + mount_rows * 52.0 + 64.0 + bay_rows * 52.0 + 10.0
	var pic := Rect2(r.position.x + 12, r.position.y + 48, r.size.x - 24, clampf(r.size.y - 60.0 - lists_h, 200.0, r.size.y * 0.55))
	_draw_dock(pic, rc)
	var centre := pic.get_center()
	var size: float = design["size"]
	var scale := minf(pic.size.y * 0.36 / size, pic.size.x * 0.3 / size)
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
		buttons.append([Rect2(pos - Vector2(14, 14), Vector2(28, 28)), _select.bind("hp", i), true, "", Callable()])
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
	# weapon mounts
	var y := pic.end.y + 10
	if hardpoints.is_empty():
		_section_label(Vector2(r.position.x + 18, y + 16), "WEAPON MOUNTS", "none on this hull")
	else:
		_section_label(Vector2(r.position.x + 18, y + 16), "WEAPON MOUNTS")
	y += 28
	var mw := (r.size.x - 36 - (mount_cols - 1) * 8) / mount_cols
	for i in hardpoints.size():
		var chip := Rect2(r.position.x + 18 + (i % mount_cols) * (mw + 8), y + (i / mount_cols) * 52, mw, 46)
		var hp: Array = hardpoints[i]
		var mt = mounts[i]
		var key := "hp:%d" % i
		buttons.append([chip, _select.bind("hp", i), true, key, Callable()])
		_hp_hover[buttons.size() - 1] = i
		var fc := CARD_SEL if i == focus else (CARD_HI if i == hot else Color(0.055, 0.085, 0.15, 0.9))
		if mt == null and i != focus and i != hot:
			fc = Color(0.03, 0.045, 0.08, 0.8)
		_card(chip, fc, Color(rc.r, rc.g, rc.b, 0.6) if i == focus else Color(0.5, 0.65, 0.95, 0.25 if mt != null else 0.14), 8.0)
		var well := Rect2(chip.position + Vector2(5, 5), Vector2(36, 36))
		_card(well, Color(0.02, 0.03, 0.06, 0.9), Color(_func_color(mt["id"]), 0.45) if mt != null else Color(1, 1, 1, 0.08), 6.0)
		if mt != null:
			_icon(mt["id"], well.grow(-3))
		draw_string(font, well.position + Vector2(0, 23), "" if mt != null else str(i + 1), HORIZONTAL_ALIGNMENT_CENTER, 36, 13, DIM)
		var name: String = GameData.MODULES[mt["id"]]["name"] if mt != null else "Empty mount %d" % (i + 1)
		var tw := mw - 56 - (34 if mt != null else 0)
		draw_string(font, chip.position + Vector2(48, 20), _fit(name, tw, 14), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, TEXT if mt != null else DIM)
		var arc := int(round(float(hp[4]) - float(hp[3])))
		var sub := "%d  ·  %s  ·  %s" % [i + 1, GameData.class_name_of(hp[5]), "turret" if arc >= 150 else "arc %d°" % arc]
		draw_string(font, chip.position + Vector2(48, 37), _fit(sub, mw - 56, 11), HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
		if mt != null:
			# weapon group badge: click to move the weapon to the next group
			var g: int = int(mt.get("group", GameData.default_weapon_group(mt["id"])))
			var gbox := Rect2(chip.end.x - 34, chip.position.y + 6, 28, 18)
			buttons.append([gbox, _cycle_group.bind(i), true, "", Callable()])
			var glit := hover == buttons.size() - 1
			_card(gbox, GROUP_COLORS[g - 1].darkened(0.1 if glit else 0.5), GROUP_COLORS[g - 1], 5.0)
			draw_string(font, gbox.position + Vector2(0, 14), "G%d" % g, HORIZONTAL_ALIGNMENT_CENTER, gbox.size.x, 12, Color.WHITE)
		_focus_ring(chip, key)
	# bays: a bar split by part, then a card per part
	var by := y + mount_rows * 52 + 8
	var cap: int = st["bay_cap"]
	var used: int = st["bay_used"]
	_section_label(Vector2(r.position.x + 18, by + 16), "INTERNAL BAYS", "%d / %d volume" % [used, cap])
	var bar := Rect2(r.position.x + 18, by + 26, r.size.x - 36, 10)
	_card(bar, Color(0, 0, 0, 0.5), Color(1, 1, 1, 0.08), 5.0)
	var bx := bar.position.x
	for mm in modules:
		var vol := float(GameData.MODULES[mm["id"]]["vol"])
		var w := bar.size.x * vol / maxf(float(maxi(cap, used)), 1.0)
		if w >= 1.0:
			draw_rect(Rect2(bx, bar.position.y + 1, maxf(w - 1.0, 1.0), bar.size.y - 2), Color(_func_color(mm["id"]), 0.85))
		bx += w
	if used > cap:
		draw_string(font, Vector2(bar.end.x - 200, by + 16), "over by %d" % (used - cap), HORIZONTAL_ALIGNMENT_RIGHT, 200, 13, BAD)
	by += 46
	var cw := (r.size.x - 36 - (bay_cols - 1) * 8) / bay_cols
	for i in modules.size() + 1:
		var chip := Rect2(r.position.x + 18 + (i % bay_cols) * (cw + 8), by + (i / bay_cols) * 52, cw, 46)
		if chip.end.y > r.end.y - 6:
			break
		var selected := sel_kind == "bay" and sel_index == i
		var key := "bay:%d" % i
		buttons.append([chip, _select.bind("bay", i), true, key, Callable()])
		var idx := buttons.size() - 1
		var lit := hover == idx or (kb_nav and focus_key == key)
		if i < modules.size():
			var id: String = modules[i]["id"]
			var mod: Dictionary = GameData.MODULES[id]
			_card(chip, CARD_SEL if selected else (CARD_HI if lit else Color(0.055, 0.085, 0.15, 0.9)), Color(rc.r, rc.g, rc.b, 0.6) if selected else Color(0.5, 0.65, 0.95, 0.25), 8.0)
			var well := Rect2(chip.position + Vector2(5, 5), Vector2(36, 36))
			_card(well, Color(0.02, 0.03, 0.06, 0.9), Color(_func_color(id), 0.45), 6.0)
			_icon(id, well.grow(-3))
			draw_string(font, chip.position + Vector2(48, 20), _fit(mod["name"], cw - 56, 14), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, TEXT)
			var pw: int = mod["power"]
			draw_string(font, chip.position + Vector2(48, 37), "vol %d   ·   %s%d power" % [mod["vol"], "+" if pw > 0 else "", pw], HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM)
		else:
			_card(chip, CARD_SEL if selected else (CARD_HI if lit else Color(0.03, 0.045, 0.08, 0.6)), Color(0.5, 0.65, 0.95, 0.3), 8.0)
			draw_string(font, chip.position + Vector2(0, 28), "+  Add a part", HORIZONTAL_ALIGNMENT_CENTER, chip.size.x, 14, DIM)
		_focus_ring(chip, key)


## The slip the ship sits on: a blueprint grid, corner brackets and a
## soft pool of light under the hull.
func _draw_dock(r: Rect2, rc: Color) -> void:
	_card(r, Color(0.02, 0.035, 0.07, 0.9), Color(1, 1, 1, 0.04), 8.0)
	var step := 32.0
	var gc := Color(rc.r, rc.g, rc.b, 0.05)
	var x := r.position.x + fmod(r.size.x * 0.5, step)
	while x < r.end.x:
		draw_line(Vector2(x, r.position.y + 2), Vector2(x, r.end.y - 2), gc, 1.0)
		x += step
	var y := r.position.y + fmod(r.size.y * 0.5, step)
	while y < r.end.y:
		draw_line(Vector2(r.position.x + 2, y), Vector2(r.end.x - 2, y), gc, 1.0)
		y += step
	var c := r.get_center()
	for i in 8:
		draw_circle(c, r.size.y * (0.5 - i * 0.05), Color(rc.r, rc.g, rc.b, 0.012))
	draw_line(Vector2(r.position.x + 20, c.y), Vector2(r.end.x - 20, c.y), Color(rc.r, rc.g, rc.b, 0.1), 1.0)
	var k := 18.0
	var bc := Color(rc.r, rc.g, rc.b, 0.45)
	for corner in [r.position + Vector2(10, 10), Vector2(r.end.x - 10, r.position.y + 10), Vector2(r.position.x + 10, r.end.y - 10), r.end - Vector2(10, 10)]:
		var sx := 1.0 if corner.x < c.x else -1.0
		var sy := 1.0 if corner.y < c.y else -1.0
		draw_line(corner, corner + Vector2(k * sx, 0), bc, 2.0)
		draw_line(corner, corner + Vector2(0, k * sy), bc, 2.0)


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


## The part in the list under the pointer or the focus, if any.
func _preview_part() -> String:
	var key := ""
	if hover >= 0 and hover < buttons.size():
		key = buttons[hover][3]
	elif kb_nav:
		key = focus_key
	if key.begins_with("part:"):
		var id := key.trim_prefix("part:")
		if GameData.MODULES.has(id) and GameData.fits_hull(id, hull):
			return id
	return ""


func _draw_right(r: Rect2) -> void:
	_card(r, CARD, LINE, 10.0)
	var preview := _preview_part()
	var design := current_design() if preview == "" else _design_with(preview)
	var st := GameData.stats_for(design)
	var was := GameData.stats_for(orig_design)
	var x := r.position.x + 16
	var y := r.position.y + 28
	_section_label(Vector2(x, y), "PERFORMANCE")
	var legend := "now  →  with %s" % GameData.MODULES[preview]["name"] if preview != "" else "now  →  new build"
	draw_string(font, Vector2(r.end.x - 316, y), _fit(legend, 300, 13), HORIZONTAL_ALIGNMENT_RIGHT, 300, 13, GOLD if preview != "" else DIM)
	y += 14
	var rows := [
		["Top speed", was["max_speed"], st["max_speed"], "%d", true],
		["Acceleration", was["accel"], st["accel"], "%d", true],
		["Turn rate", was["turn"], st["turn"], "%d°/s", true],
		["Mass", was["mass"] / 1000.0, st["mass"] / 1000.0, "%.1f t", false],
		["Energy", was["energy_max"], st["energy_max"], "%d", true],
		["Energy regen", was["energy_regen"] * 0.1, st["energy_regen"] * 0.1, "%.0f/s", true],
		["Shields", was["shield_max"], st["shield_max"], "%d", true],
		["Hull", was["hull_max"], st["hull_max"], "%d", true],
		["Lock range", was["sensor_range"], st["sensor_range"], "%d", true],
	]
	# how loud the build is: lower is stealthier
	var sw := Sensors.fit_signatures(orig_design, was)
	var sn := Sensors.fit_signatures(design, st)
	rows.append(["Radar return", sw["radar"], sn["radar"], "%d", false])
	rows.append(["Heat (burning)", sw["burn"], sn["burn"], "%d", false])
	rows.append(["Emissions", sw["em"], sn["em"], "%d", false])
	for row in rows:
		y += 22
		_stat_row(Rect2(x, y - 14, r.size.x - 32, 18), row[0], float(row[1]), float(row[2]), row[3], row[4])
	y += 24
	var texts := [
		["Shield type", _shield_text(was), _shield_text(st)],
		["Armor", GameData.coat_label(was["coat"]), GameData.coat_label(st["coat"])],
		["Armor radar factor", "%.1fx" % was["armor_sig"], "%.1fx" % st["armor_sig"]],
		["Heat coasting", "%d" % sw["heat"], "%d" % sn["heat"]],
	]
	for row in texts:
		draw_string(font, Vector2(x, y), row[0], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
		var same: bool = row[1] == row[2]
		draw_string(font, Vector2(r.end.x - 316, y), row[1] if same else "%s  →  %s" % [row[1], row[2]], HORIZONTAL_ALIGNMENT_RIGHT, 300, 13, TEXT if same else GOLD)
		y += 19
	# power budget
	var gen: float = st["power_gen"]
	var draw_p: float = st["power_draw"]
	var pcol := GOOD if draw_p <= gen else BAD
	y += 4
	draw_string(font, Vector2(x, y), "Reactor", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(r.end.x - 316, y), "%d made  ·  %d drawn" % [gen, draw_p], HORIZONTAL_ALIGNMENT_RIGHT, 300, 13, pcol)
	y += 7
	var pbar := Rect2(x, y, r.size.x - 32, 8)
	_card(pbar, Color(0, 0, 0, 0.5), Color(1, 1, 1, 0.06), 4.0)
	draw_rect(Rect2(pbar.position + Vector2(1, 1), Vector2((pbar.size.x - 2) * clampf(draw_p / maxf(gen, 1.0), 0.0, 1.0), pbar.size.y - 2)), pcol)
	y += 24
	if draw_p > gen:
		draw_string(font, Vector2(x, y - 2), "Overloaded: weapons cost more and energy comes back slower.", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, BAD)
		y += 18
	# systems, as pills
	var flags := []
	if st["can_jam"]:
		flags.append("Jammer")
	if st["heat_sink"] > 0.0:
		flags.append("Heat sink")
	if st["has_boost"]:
		flags.append("Booster")
	if st["qdrv"] > 0.0:
		flags.append("Warp %d" % st["qdrv"])
	if st["regen"] > 0.0:
		flags.append("Regen %d/s" % st["regen"])
	if st["repair"] > 0.0:
		flags.append("Repair %d/s" % st["repair"])
	if st["ram"] > 0.0:
		flags.append("Ram x%d" % st["ram"])
	var px := x
	if flags.is_empty():
		draw_string(font, Vector2(x, y + 4), "No special systems", HORIZONTAL_ALIGNMENT_LEFT, -1, 12, FAINT)
	for f in flags:
		var w := font.get_string_size(f, HORIZONTAL_ALIGNMENT_LEFT, -1, 12).x + 20
		if px + w > r.end.x - 16:
			break
		_card(Rect2(px, y - 10, w, 22), Color(0.07, 0.12, 0.2, 0.9), Color(0.5, 0.7, 1.0, 0.3), 11.0)
		draw_string(font, Vector2(px + 10, y + 5), f, HORIZONTAL_ALIGNMENT_LEFT, -1, 12, TEXT)
		px += w + 6
	y += 28
	draw_line(Vector2(x, y - 6), Vector2(r.end.x - 16, y - 6), LINE, 1.0)
	_draw_catalogue(Rect2(x, y, r.size.x - 32, r.end.y - y - 10))


## One stat: a bar for the new value with a tick where it is now, and the
## numbers, green or red by whether it got better.
func _stat_row(r: Rect2, label: String, now: float, new: float, fmt: String, higher_better: bool) -> void:
	draw_string(font, Vector2(r.position.x, r.position.y + 13), label, HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	var bar := Rect2(r.position.x + 120, r.position.y + 5, r.size.x - 120 - 170, 8)
	_card(bar, Color(0, 0, 0, 0.45), Color(1, 1, 1, 0.05), 4.0)
	var top := maxf(maxf(now, new), 0.001) * 1.15
	var better := (new > now) == higher_better
	var col := LABEL if is_equal_approx(now, new) else (GOOD if better else BAD)
	draw_rect(Rect2(bar.position + Vector2(1, 1), Vector2((bar.size.x - 2) * clampf(new / top, 0.0, 1.0), bar.size.y - 2)), Color(col, 0.75))
	var tx := bar.position.x + bar.size.x * clampf(now / top, 0.0, 1.0)
	draw_line(Vector2(tx, bar.position.y - 3), Vector2(tx, bar.end.y + 3), Color(1, 1, 1, 0.75), 2.0)
	var nt := fmt % now
	var wt := fmt % new
	if nt == wt:
		draw_string(font, Vector2(r.end.x - 160, r.position.y + 13), wt, HORIZONTAL_ALIGNMENT_RIGHT, 160, 14, TEXT)
	else:
		draw_string(font, Vector2(r.end.x - 160, r.position.y + 13), nt, HORIZONTAL_ALIGNMENT_RIGHT, 70, 13, FAINT)
		draw_string(font, Vector2(r.end.x - 88, r.position.y + 13), "→", HORIZONTAL_ALIGNMENT_CENTER, 16, 13, FAINT)
		draw_string(font, Vector2(r.end.x - 70, r.position.y + 13), wt, HORIZONTAL_ALIGNMENT_RIGHT, 70, 14, col)


func _draw_catalogue(r: Rect2) -> void:
	var x := r.position.x
	var y := r.position.y + 16
	var slot_text := ""
	match sel_kind:
		"hp":
			slot_text = "for mount %d  (%s)" % [sel_index + 1, GameData.class_name_of(hardpoints[sel_index][5])]
		"bay":
			slot_text = "for the bays"
		"hold":
			slot_text = "pick a mount or bay to fit %s" % GameData.MODULES[hold[sel_index]]["name"] if sel_index < hold.size() else ""
		_:
			slot_text = "pick a mount or bay on the ship"
	_section_label(Vector2(x, y), "PARTS", slot_text)
	var current := _selected_part()
	if not current.is_empty():
		y += 16
		var what := ("Upgrades for %s" if cat_mode == "upgrade" else "Everything that fits instead of %s") % GameData.MODULES[current]["name"]
		draw_string(font, Vector2(x, y + 12), _fit(what, r.size.x - 260, 13), HORIZONTAL_ALIGNMENT_LEFT, -1, 13, GOLD)
		_button(Rect2(r.end.x - 118, y - 4, 118, 26), "To hold", _remove_selected, true, 13, false, "", _key("remove"))
		_button(Rect2(r.end.x - 250, y - 4, 126, 26), "Upgrades" if cat_mode == "switch" else "Switch out", func():
			cat_mode = "switch" if cat_mode == "upgrade" else "upgrade"
			cat_scroll = 0.0, true, 13, false, "", _key("switch"))
		y += 14
	y += 12
	cat_rect = Rect2(x, y, r.size.x, r.end.y - y)
	if sel_kind != "hp" and sel_kind != "bay":
		_card(cat_rect, Color(0, 0, 0, 0.2), Color(1, 1, 1, 0.04), 8.0)
		draw_string(font, cat_rect.position + Vector2(0, cat_rect.size.y * 0.5), "Choose a weapon mount or a bay to see what fits.", HORIZONTAL_ALIGNMENT_CENTER, cat_rect.size.x, 14, FAINT)
		return
	var list := _catalogue_rows()
	if list.is_empty() and not current.is_empty():
		draw_string(font, cat_rect.position + Vector2(8, 22), "This is the best of its kind. Switch out to see what else fits.", HORIZONTAL_ALIGNMENT_LEFT, cat_rect.size.x - 16, 13, DIM)
		return
	var visible := maxi(int(cat_rect.size.y / CAT_ROW), 1)
	cat_scroll = clampf(cat_scroll, 0.0, maxf(list.size() - visible, 0.0))
	var start := int(cat_scroll)
	for k in range(start, mini(list.size(), start + visible)):
		var e: Dictionary = list[k]
		var row := Rect2(cat_rect.position.x, cat_rect.position.y + (k - start) * CAT_ROW, cat_rect.size.x - 8, CAT_ROW - 4)
		var enabled: bool = e["ok"]
		var key: String = "part:" + e["id"]
		buttons.append([row, _mount.bind(e["id"]), enabled, key, Callable()])
		var idx := buttons.size() - 1
		var lit := hover == idx or (kb_nav and focus_key == key)
		_card(row, CARD_HI if lit and enabled else (Color(0.055, 0.085, 0.15, 0.9) if enabled else Color(0.04, 0.05, 0.07, 0.7)), Color(0.5, 0.65, 0.95, 0.2 if enabled else 0.08), 7.0)
		_icon(e["id"], Rect2(row.position.x + 6, row.position.y + 3, 36, row.size.y - 6), 1.0 if enabled else 0.35)
		var col := TEXT if enabled else FAINT
		draw_string(font, row.position + Vector2(50, 15), _fit(e["name"], row.size.x - 170, 14), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, col)
		draw_string(font, row.position + Vector2(50, 29), _fit(e["info"], row.size.x - 170, 11), HORIZONTAL_ALIGNMENT_LEFT, -1, 11, DIM if enabled else Color(0.36, 0.38, 0.44))
		draw_string(font, row.position + Vector2(row.size.x - 122, 22), e["price"], HORIZONTAL_ALIGNMENT_RIGHT, 112, 13, e["pcol"])
		_focus_ring(row, key)
	if list.size() > visible:
		var frac := float(visible) / list.size()
		var th := cat_rect.size.y * frac
		var ty := cat_rect.position.y + (cat_rect.size.y - th) * (cat_scroll / maxf(list.size() - visible, 1.0))
		draw_rect(Rect2(cat_rect.end.x - 3, ty, 3, th), Color(0.5, 0.7, 1.0, 0.5))


## Rows of the catalogue for the selected slot: common parts, this
## empire's parts, blueprinted enemy parts, and greyed rows for the rest.
func _catalogue_rows() -> Array:
	var out := []
	var want_bay := sel_kind == "bay"
	var cls: int = hardpoints[sel_index][5] if sel_kind == "hp" else 2
	var current := _selected_part()
	var upgrades_only := not current.is_empty() and cat_mode == "upgrade"
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
		if upgrades_only and not _is_upgrade(current, id):
			continue
		var race := GameData.part_race(id)
		var locked := race >= 0 and race != base.race_id and not blueprints.has(id)
		var banned := not GameData.fits_hull(id, hull)
		var too_big := banned or (not want_bay and GameData.weapon_class(id) > cls)
		var in_hold := hold.has(id)
		var price := GameData.price(id)
		var ok := not locked and not too_big and (in_hold or price <= credits)
		var ptxt := ""
		var pcol := GOLD
		if locked:
			ptxt = "%s blueprint" % GameData.race(race)["adj"]
			pcol = Color(0.4, 0.6, 0.65)
		elif too_big:
			ptxt = "won't fit" if banned else "too heavy"
			pcol = Color(0.5, 0.45, 0.45)
		elif in_hold:
			ptxt = "in hold"
			pcol = GOOD
		else:
			ptxt = "%d cr" % price
			pcol = GOLD if price <= credits else BAD
		out.append({"id": id, "name": mod["name"], "info": _part_info(id), "price": ptxt, "pcol": pcol, "ok": ok})
	return out


## Is `id` an upgrade for the fitted part: the same kind, a higher mark
## where parts come in marks (Power plant III > IV), else any other model.
static func _is_upgrade(current: String, id: String) -> bool:
	if id == current or not _same_family(id, current):
		return false
	if GameData.tier(current) > 1 or GameData.tier(id) > 1:
		return GameData.tier(id) > GameData.tier(current)
	return true


## The part fitted in the selected mount or bay slot, or "".
func _selected_part() -> String:
	if sel_kind == "hp" and sel_index >= 0 and sel_index < mounts.size() and mounts[sel_index] != null:
		return mounts[sel_index]["id"]
	if sel_kind == "bay" and sel_index >= 0 and sel_index < modules.size():
		return modules[sel_index]["id"]
	return ""


func _shield_text(st: Dictionary) -> String:
	if st["shield_max"] <= 0.0:
		return "none"
	var fam: String = st["shield_family"]
	return GameData.SHIELD_FAMILIES[fam]["label"] if GameData.SHIELD_FAMILIES.has(fam) else fam.capitalize()


func _part_info(id: String) -> String:
	var mod: Dictionary = GameData.MODULES[id]
	var p: Array = mod["p"]
	match mod["func"]:
		"power":
			return "+%d power   energy %d" % [mod["power"], mod["power"] * 5]
		"eng":
			return "thrust %d   vol %d" % [p[0], mod["vol"]]
		"shield":
			var fam: Dictionary = GameData.SHIELD_FAMILIES[GameData.shield_family(id)]
			var leak := "  leaks %d%% of half the hits" % int(fam["leak"] * 100.0) if fam["leak"] > 0.0 else ""
			return "%s %d  %d%%/s after %.1f s%s" % [fam["label"].to_lower(), p[0], int(fam["rate"] * 100.0), fam["pause"], leak]
		"armor":
			var res: Dictionary = GameData.ARMOR[id]["resist"]
			var parts := []
			for t in ["beam", "plasma", "kinetic", "explosive"]:
				parts.append("%s %d" % [t.left(3), int(res[t] * 100.0)])
			return "%s  hull +%d  mass %d" % ["/".join(parts), mod["hp"], mod["mass"]]
		"sensor":
			var fam: Dictionary = GameData.SENSOR_FAMILIES[mod["fam"]]
			return "%s %s   %s   range %d" % [fam["channel"], "lock" if fam["track"] else "contacts", "active" if fam["active"] else "passive", p[0]]
		"jammer":
			return "jams your target's sensors   reach %d" % p[0]
		"heatsink":
			return "soaks 70%% of your heat, %d heat-seconds" % p[0]
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
