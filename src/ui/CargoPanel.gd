## CargoPanel -- the cargo hold (L): what is aboard, what the port has,
## and buttons to load or unload each resource, at one of your colonies
## (landed) or alongside one of your stations.  Goods unloaded where the
## world is short of them pay, and so does every unit a station frame
## needs; the port moves them a few dozen units a second.  At a trade
## station loading is buying and unloading selling, at market prices.  At
## your starbase or a shipyard it also sells station kits and takes ship
## orders, and with a kit aboard it deploys one where you are.
class_name CargoPanel
extends Control

signal close_requested

var world: World
var font: Font
var time := 0.0
var buttons: Array = []        # [rect, label, callable, enabled]
var hover := -1
var want_h := 300.0            # the height the last draw needed
var tab := ""                  # at a yard: "cargo", "kits" or "orders"

const W := 560.0
const ROW_H := 30.0
const PANEL := Color(0.01, 0.03, 0.07, 0.93)
const TEXT := Color(0.9, 0.95, 1.0)
const DIM := Color(0.55, 0.65, 0.8)
const WARN := Color(1.0, 0.7, 0.5)


## Anything for the panel to show: a port, the starbase's kits, or a kit
## to deploy.
static func can_open(w: World, s: Ship) -> bool:
	if s == null or s.dead:
		return false
	return s.is_landed() or w.cargo_port(s) != null or s.kit != "" or w.nearest_base(s) != null


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_STOP


func _ship() -> Ship:
	return world.player if world != null else null


func _process(delta: float) -> void:
	time += delta
	var s := _ship()
	if not can_open(world, s):
		close_requested.emit()
		return
	var vp := get_viewport_rect().size
	# beside the ground panel, clear of the radar
	position = Vector2(vp.x - W - 30.0, maxf(vp.y * 0.5 - want_h * 0.5 - 60.0, 90.0))
	size = Vector2(W, want_h)
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


func _set_tab(t: String) -> void:
	tab = t


func _order(res: String, amount: float) -> void:
	world.cargo_command(_ship(), res, amount)


func _draw() -> void:
	var s := _ship()
	if not can_open(world, s):
		return
	buttons.clear()
	draw_rect(Rect2(Vector2.ZERO, size), PANEL)
	draw_rect(Rect2(Vector2.ZERO, size), Color(0.95, 0.85, 0.45, 0.6), false, 1.5)
	var port = world.cargo_port(s)
	var base: Ship = world.nearest_base(s) if not s.is_landing() else null
	var where := ""
	if port is Ship:
		where = "  at %s%s" % [port.ship_name.to_upper(), " (FRAME)" if port.building else ""]
	elif port != null:
		where = "  at %s" % world.port_name(port).to_upper()
	elif s.is_landed():
		where = "  at %s" % s.land_body.body_name.to_upper()
	elif base != null:
		where = "  at %s" % base.ship_name.to_upper()
	draw_string(font, Vector2(16, 28), "CARGO HOLD" + where, HORIZONTAL_ALIGNMENT_LEFT, W - 32, 18, TEXT)
	var y := _hold_bar(s, 40.0)
	var trade := port != null and s.cargo_max > 0.0
	if base != null:
		# a yard: its counters on tabs, so the panel stays on the screen
		var tabs := (["cargo"] if trade else []) + ["kits", "orders"]
		if not tabs.has(tab):
			tab = tabs[0]
		var labels := {"cargo": "Cargo", "kits": "Station kits", "orders": "Ship orders"}
		var tw := (W - 32.0 - 8.0 * (tabs.size() - 1)) / tabs.size()
		for i in tabs.size():
			var t: String = tabs[i]
			_button(Rect2(16 + i * (tw + 8.0), y, tw, 26), ("> %s <" % labels[t]) if t == tab else labels[t], _set_tab.bind(t), true)
		y += 34.0
	if s.is_landed() and port == null:
		draw_string(font, Vector2(16, y + 22), "Cargo only moves through your own colonies' spaceports.", HORIZONTAL_ALIGNMENT_LEFT, W - 32, 15, WARN)
		y += 34.0
	elif trade and (base == null or tab == "cargo"):
		y = _port_rows(s, port, y)
	if base != null and tab == "kits":
		y = _kits(s, y)
	elif base != null and tab == "orders":
		y = _orders(s, base, y)
	elif base == null and s.kit != "" and port == null:
		y = _deploy(s, y)
	y += 28.0
	draw_string(font, Vector2(0, y - 10), "%s or Esc closes%s" % [Keybinds.short_text("cargo"), "   B lifts off" if s.is_landed() else ""], HORIZONTAL_ALIGNMENT_CENTER, W, 13, Color(1, 1, 1, 0.45))
	want_h = y


## The hold's fill bar (or the kit that fills it); returns the next y.
func _hold_bar(s: Ship, y: float) -> float:
	var bar := Rect2(16, y, W - 32, 18)
	draw_rect(bar, Color(0.1, 0.12, 0.16, 0.9))
	if s.cargo_max <= 0.0:
		draw_string(font, bar.position + Vector2(6, 14), "This hull has no cargo hold.", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
		return y + 28.0
	if s.kit != "":
		draw_rect(bar, Color(0.55, 0.85, 1.0, 0.75))
		draw_string(font, bar.position + Vector2(6, 14), "%s kit aboard: it fills the hold" % GameData.STATIONS[s.kit]["label"], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.05, 0.05, 0.08))
		return y + 28.0
	var frac := s.cargo_total() / maxf(s.cargo_max, 1.0)
	draw_rect(Rect2(bar.position, Vector2(bar.size.x * clampf(frac, 0.0, 1.0), bar.size.y)), Color(0.95, 0.8, 0.35, 0.85))
	var moving := "   loading..." if s.cargo_orders.values().any(func(v): return v > 0.0) else ("   unloading..." if not s.cargo_orders.is_empty() else "")
	draw_string(font, bar.position + Vector2(6, 14), "%d / %d units%s" % [int(s.cargo_total()), int(s.cargo_max), moving], HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(0.05, 0.05, 0.08) if frac > 0.35 else TEXT)
	return y + 28.0


## Load and unload at a colony or a station.
func _port_rows(s: Ship, port, y: float) -> float:
	if port is Ship and Economy.is_market(port):
		return _market_rows(s, port, y)
	var stock: Dictionary = world.port_stock(port)
	var frame: bool = port is Ship and port.building
	var planet := port is Planet
	_button(Rect2(16, y, 160, 26), "Load all", func(): _order("all", 1.0), s.cargo_space() >= 1.0 and not stock.is_empty())
	_button(Rect2(186, y, 160, 26), "Unload all", func(): _order("all", -1.0), s.cargo_total() >= 1.0)
	y += 40.0
	draw_string(font, Vector2(16, y + 4), "Resource", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(130, y + 4), "Aboard", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(206, y + 4), "Needs" if frame else "Here", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	y += 10.0
	var keys: Array = Economy.RESOURCES
	if frame:
		keys = GameData.STATIONS[port.design_name]["cost"].keys()
	for k in keys:
		var aboard: float = s.cargo.get(k, 0.0)
		var here: float = stock.get(k, 0.0)
		var room: float = world.port_room(port, k)
		draw_string(font, Vector2(16, y + 20), Economy.NAMES[k], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Economy.COLORS[k])
		draw_string(font, Vector2(130, y + 20), str(int(aboard)), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT if aboard >= 1.0 else DIM)
		if frame:
			var need: float = GameData.STATIONS[port.design_name]["cost"][k]
			draw_string(font, Vector2(206, y + 20), "%d / %d" % [int(need - room), int(need)], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT if room < 0.5 else Color(1.0, 0.75, 0.4))
		elif planet:
			draw_string(font, Vector2(206, y + 20), "%d / %d" % [int(here), int(Economy.cap(port))], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT if here >= 1.0 else DIM)
			if Economy.need(port, k) > 0.5:
				draw_string(font, Vector2(290, y + 20), "short", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, Color(1.0, 0.75, 0.4))
		else:
			draw_string(font, Vector2(206, y + 20), str(int(here)), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT if here >= 1.0 else DIM)
		var bx := W - 16.0 - 3 * 64.0
		if not frame:
			_button(Rect2(bx, y + 3, 58, 24), "+10", func(): _order(k, 10.0), here >= 1.0 and s.cargo_space() >= 1.0)
			_button(Rect2(bx + 64, y + 3, 58, 24), "+all", func(): _order(k, 1e6), here >= 1.0 and s.cargo_space() >= 1.0)
		_button(Rect2(bx + 128, y + 3, 58, 24), "-all", func(): _order(k, -1e6), aboard >= 1.0 and room >= 0.5)
		y += ROW_H
	var notes: Array
	if planet:
		notes = ["Unloading pays where the world is short of the goods", "and makes them worse than the world you loaded them at."]
	elif frame:
		notes = ["Every unit the frame needs pays its full price.", "Once it has everything, the station takes %d s to build." % int(GameData.STATIONS[port.design_name]["build"])]
	else:
		notes = [port_note(port.design_name), "Total %d / %d units." % [int(port.cargo_total()), int(port.cargo_max)]]
	draw_string(font, Vector2(16, y + 20), notes[0], HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	draw_string(font, Vector2(16, y + 37), notes[1], HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	return y + 48.0


## Buy and sell at a trade station.
func _market_rows(s: Ship, m: Ship, y: float) -> float:
	_button(Rect2(16, y, 160, 26), "Sell all", func(): _order("all", -1.0), s.cargo_total() >= 1.0)
	draw_string(font, Vector2(196, y + 18), "credits %d" % s.credits, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1.0, 0.85, 0.4))
	y += 40.0
	draw_string(font, Vector2(16, y + 4), "Resource", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(116, y + 4), "Aboard", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(176, y + 4), "Market", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	draw_string(font, Vector2(240, y + 4), "Buy / sell", HORIZONTAL_ALIGNMENT_LEFT, -1, 14, DIM)
	y += 10.0
	for k in Economy.RESOURCES:
		var aboard: float = s.cargo.get(k, 0.0)
		var here: float = m.cargo.get(k, 0.0)
		var buy := Economy.buy_price(m, k, s)
		var sell := Economy.sell_price(m, k, s)
		var base_price: float = GameData.RESOURCE_PRICE.get(k, 1.0)
		draw_string(font, Vector2(16, y + 20), Economy.NAMES[k], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Economy.COLORS[k])
		draw_string(font, Vector2(116, y + 20), str(int(aboard)), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT if aboard >= 1.0 else DIM)
		draw_string(font, Vector2(176, y + 20), str(int(here)), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT if here >= 1.0 else DIM)
		# cheap to buy in green, dear to sell in gold
		draw_string(font, Vector2(240, y + 20), "%.1f" % buy, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.55, 1.0, 0.6) if buy < base_price else TEXT)
		draw_string(font, Vector2(282, y + 20), "/ %.1f" % sell, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(1.0, 0.85, 0.4) if sell > base_price else TEXT)
		var bx := W - 16.0 - 3 * 64.0
		var can_buy := here >= 1.0 and s.cargo_space() >= 1.0 and s.credits >= buy and s.kit == ""
		_button(Rect2(bx, y + 3, 58, 24), "+10", func(): _order(k, 10.0), can_buy)
		_button(Rect2(bx + 64, y + 3, 58, 24), "+50", func(): _order(k, 50.0), can_buy)
		_button(Rect2(bx + 128, y + 3, 58, 24), "-all", func(): _order(k, -1e6), aboard >= 1.0 and world.port_room(m, k) >= 1.0)
		y += ROW_H
	var owner := "Nobody owns the free port: no fee." if m.race_id == GameData.NEUTRAL else (
		"Your empire's market: no fee." if not Economy.pays_fee(m, s) else "A %s market: %d%% of every trade goes to its owners." % [GameData.race(m.race_id)["adj"], int(Economy.MARKET_FEE * 100)])
	draw_string(font, Vector2(16, y + 20), "Prices follow the stock: sell where goods are short, buy where they pile up.", HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	draw_string(font, Vector2(16, y + 37), owner, HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	return y + 48.0


static func port_note(hull: String) -> String:
	match hull:
		"SupplyDepot":
			return "The depot stores anything, and refuels and crews ships alongside."
		"Refinery":
			return "Bring Metals and Fuel: it makes an Alloy from 2 Metals and 1 Fuel."
		"MiningStation", "GasSkimmer":
			return "Its output piles up here until a supply ship takes it."
		"Shipyard":
			return "The yard builds hulls from its Metals, Alloys and Exotics: stock pays."
	return ""


## The yard's kit counter.
func _kits(s: Ship, y: float) -> float:
	y += 14.0
	draw_string(font, Vector2(16, y), "STATION KITS", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
	draw_string(font, Vector2(150, y), "credits %d" % s.credits, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1.0, 0.85, 0.4))
	y += 8.0
	if s.kit != "":
		draw_string(font, Vector2(16, y + 20), "You carry a %s kit." % GameData.STATIONS[s.kit]["label"].to_lower(), HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT)
		_button(Rect2(W - 196, y + 3, 180, 24), "Hand it back (%d)" % GameData.STATIONS[s.kit]["kit"], func(): world.kit_command(_ship(), ""), true)
		return y + 40.0
	# what stops every kit at once, said once
	var general := ""
	if s.rank < GameData.KIT_RANK:
		general = "Kits need the rank of %s." % GameData.RANKS[GameData.KIT_RANK]["name"]
	elif s.cargo_max < GameData.CARGO_HOLD["Freighter"]:
		general = "Kits need a %d-unit hold: refit to a supply ship (U)." % GameData.CARGO_HOLD["Freighter"]
	elif s.cargo_total() >= 1.0:
		general = "Empty the hold first: a kit fills it."
	for key in GameData.STATION_ORDER:
		var st: Dictionary = GameData.STATIONS[key]
		var cost := []
		for k in st["cost"]:
			cost.append("%d %s" % [st["cost"][k], Economy.NAMES[k]])
		draw_string(font, Vector2(16, y + 18), st["label"], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT)
		draw_string(font, Vector2(170, y + 18), ", ".join(cost), HORIZONTAL_ALIGNMENT_LEFT, 200, 13, DIM)
		draw_string(font, Vector2(16, y + 33), st["does"], HORIZONTAL_ALIGNMENT_LEFT, W - 140, 12, Color(0.6, 0.7, 0.85, 0.8))
		_button(Rect2(W - 116, y + 6, 100, 26), "%d cr" % st["kit"], func(): world.kit_command(_ship(), key), World.kit_refusal(s, key) == "")
		y += 40.0
	if general != "":
		draw_string(font, Vector2(16, y + 18), general, HORIZONTAL_ALIGNMENT_LEFT, W - 32, 14, WARN)
		y += 26.0
	return y


## The yard's order book: what it builds for the empire, from its stock.
func _orders(s: Ship, base: Ship, y: float) -> float:
	y += 14.0
	draw_string(font, Vector2(16, y), "SHIP ORDERS", HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
	draw_string(font, Vector2(150, y), "credits %d" % s.credits, HORIZONTAL_ALIGNMENT_LEFT, -1, 14, Color(1.0, 0.85, 0.4))
	var stock: Dictionary = world.yard_stock(base)
	var parts := []
	for k in ["metals", "alloys", "exotics"]:
		parts.append("%s %d" % [Economy.NAMES[k], int(stock.get(k, 0.0))])
	draw_string(font, Vector2(16, y + 20), "Yard stock (%s): %s" % ["its own hold" if base.design_name == "Shipyard" else "the home world's", "   ".join(parts)], HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	y += 28.0
	var wait := 0.0
	for e in base.orders:
		wait += float(e[1])
		draw_string(font, Vector2(16, y + 16), "On the slip: %s for %s, launches in %d s" % [GameData.ORDERS[e[0]]["label"].to_lower(), e[2], int(ceil(wait))], HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, Color(0.6, 1.0, 0.7))
		y += 18.0
	y += 4.0
	var why_all := ""
	for hull in GameData.ORDER_LIST:
		var o: Dictionary = GameData.ORDERS[hull]
		var cost := []
		var need: Dictionary = GameData.HULL_MATERIALS.get(hull, {})
		for k in need:
			cost.append("%d %s" % [need[k], Economy.NAMES[k]])
		var why := world.order_refusal(s, base, hull)
		draw_string(font, Vector2(16, y + 18), o["label"], HORIZONTAL_ALIGNMENT_LEFT, -1, 15, TEXT)
		draw_string(font, Vector2(170, y + 18), ", ".join(cost), HORIZONTAL_ALIGNMENT_LEFT, 200, 13, DIM)
		draw_string(font, Vector2(16, y + 33), "%s; %d s on the slip" % [o["does"], int(o["build"])], HORIZONTAL_ALIGNMENT_LEFT, W - 140, 12, Color(0.6, 0.7, 0.85, 0.8))
		_button(Rect2(W - 116, y + 6, 100, 26), "%d cr" % o["credits"], func(): world.order_command(_ship(), hull), why == "")
		if why != "" and why_all == "":
			why_all = why
		y += 40.0
	if why_all != "":
		draw_string(font, Vector2(16, y + 18), why_all, HORIZONTAL_ALIGNMENT_LEFT, W - 32, 14, WARN)
		y += 26.0
	return y


## With a kit aboard: where it would go up, and the button.
func _deploy(s: Ship, y: float) -> float:
	var st: Dictionary = GameData.STATIONS[s.kit]
	var site: Dictionary = world.station_site(s, s.kit)
	y += 8.0
	draw_string(font, Vector2(16, y + 18), "%s: %s." % [st["label"], st["does"]], HORIZONTAL_ALIGNMENT_LEFT, W - 32, 14, TEXT)
	var cost := []
	for k in st["cost"]:
		cost.append("%d %s" % [st["cost"][k], Economy.NAMES[k]])
	draw_string(font, Vector2(16, y + 38), "The frame then needs %s, and %d s of work." % [", ".join(cost), int(st["build"])], HORIZONTAL_ALIGNMENT_LEFT, W - 32, 13, DIM)
	y += 50.0
	var ok: bool = site["why"] == ""
	var line: String = ("Deploy over %s here." % site["body"].body_name) if ok else site["why"]
	draw_string(font, Vector2(16, y + 18), line, HORIZONTAL_ALIGNMENT_LEFT, W - 200, 14, Color(0.6, 1.0, 0.7) if ok else WARN)
	_button(Rect2(W - 176, y, 160, 28), "Deploy", func(): world.deploy_kit(_ship()), ok)
	return y + 40.0


func _button(rect: Rect2, label: String, cb: Callable, enabled: bool) -> void:
	var lit := enabled and buttons.size() == hover
	var fill := Color(0.2, 0.32, 0.55, 0.9) if lit else (Color(0.1, 0.16, 0.28, 0.9) if enabled else Color(0.08, 0.09, 0.12, 0.7))
	draw_rect(rect, fill)
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.6 if enabled else 0.2), false, 1.0)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + 5), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, 14, TEXT if enabled else Color(0.5, 0.5, 0.55))
	buttons.append([rect, label, cb, enabled])
