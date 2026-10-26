# Trade test (the Resources update, phase 4): the free port at Jupiter's
# Trojan point; prices that move with the stock; selling and buying there;
# a trade station of your own that takes a fee from foreigners; a shipyard
# station that refits away from home, takes Alloys and Exotics for bigger
# hulls and builds ordered ships, which launch; and a computer trade ship
# that sells its empire's goods at the free port.
#   godot --headless --path . -s tools/trade_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var gd: Node                 # the GameData autoload (not an identifier in a tool script)
var port: Ship = null
var market: Ship = null
var yard: Ship = null
var trader: Ship = null
var foreigner: Ship = null
var was := 0.0
var credits_before := 0
var launched := 0


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("/root/GameData")
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()
	await process_frame
	await process_frame


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _haulers(w: World, on: bool) -> void:
	for s in w.ships:
		if s.support:
			var ai: AIPilot = s.get_node_or_null("AI")
			ai.set_physics_process(on)
			if not on:
				for k in s.inputs.keys():
					s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel


## Hold a ship alongside a station, at rest against it.
func _alongside(s: Ship, st: Ship) -> void:
	var c: Vector2 = st.station_body.global_position if st.station_body != null else Vector2.ZERO
	var out := (st.global_position - c).normalized()
	_put(s, st.global_position + out * (st.size + s.size + 60.0), st.velocity)


func _process(_delta: float) -> bool:
	if main == null or main.world == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	if p == null:
		return false
	var now := w.elapsed
	p.invulnerable = 1e9
	var jupiter: Planet = w.planet_by_name["Jupiter"]
	match step:
		0:
			if now < 1.0:
				return false
			_haulers(w, false)
			for s in w.ships:
				if s.race_id == gd.NEUTRAL:
					port = s
			_check(port != null and Economy.is_market(port), "the free port is a working market")
			_check(w.ships.filter(func(s): return s.support and s.design_name == "Trader").size() == 3, "each empire has a computer trade ship")
			# prices follow the stock
			var lo := Economy.market_price(port, "food")
			port.cargo["food"] = 20.0
			var bare := Economy.market_price(port, "food")
			port.cargo["food"] = 600.0
			var glut := Economy.market_price(port, "food")
			port.cargo["food"] = 160.0
			_check(bare > lo and lo > glut, "food at the free port: %.2f bare, %.2f stocked, %.2f glutted" % [bare, lo, glut])
			_check(Economy.buy_price(port, "food", p) > Economy.sell_price(port, "food", p), "buying costs more than selling pays")
			_check(not Economy.pays_fee(port, p), "nobody takes a fee at the free port")
			# nobody fights over it
			_check(w.nearest_enemy_ship(port.global_position, p.team(), 5000.0) != port, "the free port is nobody's enemy")
			var hull := port.hull
			port.take_damage(500.0, p)
			_check(port.hull == hull, "and takes no damage")
			# sell food there
			p.refit("Freighter")
			p.cargo = {"food": 100.0}
			p.credits = 100
			_alongside(p, port)
			_check(w.cargo_port(p) == port, "alongside the free port the hold trades there")
			was = Economy.market_price(port, "food")
			credits_before = p.credits
			w.cargo_command(p, "food", -100.0)
			t0 = now
			step = 1
		1:
			_alongside(p, port)
			if not p.cargo_orders.is_empty() and now - t0 < 10.0:
				return false
			var gain := p.credits - credits_before
			_check(p.cargo.get("food", 0.0) < 1.0 and gain > 50, "sold 100 Food for %d credits" % gain)
			_check(Economy.market_price(port, "food") < was, "the food price fell as the market filled (%.2f -> %.2f)" % [was, Economy.market_price(port, "food")])
			# buy exotics there
			was = Economy.market_price(port, "exotics")
			credits_before = p.credits
			w.cargo_command(p, "exotics", 30.0)
			t0 = now
			step = 2
		2:
			_alongside(p, port)
			if not p.cargo_orders.is_empty() and now - t0 < 10.0:
				return false
			var spent := credits_before - p.credits
			_check(absf(p.cargo.get("exotics", 0.0) - 30.0) < 0.5 and spent > 30 * 4, "bought 30 Exotics for %d credits" % spent)
			_check(Economy.market_price(port, "exotics") > was, "the exotics price rose as the market emptied")
			# a purse that runs dry stops the buying
			p.cargo.clear()
			p.credits = 10
			w.cargo_command(p, "metals", 100.0)
			t0 = now
			step = 3
		3:
			_alongside(p, port)
			if not p.cargo_orders.is_empty() and now - t0 < 10.0:
				return false
			_check(p.credits >= 0 and p.cargo.get("metals", 0.0) < 10.0 and p.cargo.get("metals", 0.0) > 5.0, "10 credits bought %.1f Metals and no more (credits %d)" % [p.cargo.get("metals", 0.0), p.credits])
			# a Terran trade station; a Drengin pilot pays the founder a fee
			market = w.build_station(p.race_id, "TradeStation", jupiter, jupiter.global_position + Vector2(jupiter.radius + 700.0, 0), false)
			market.founder = p.pilot_name
			market.invulnerable = 1e9
			_check(Economy.is_market(market) and market.cargo_max == 2000.0, "a finished trade station is a market")
			for s in w.ships:
				if s.race_id == 2 and not s.is_station and not s.support and s.design_name != "Starbase" and s.landing == Ship.Landing.NONE:
					foreigner = s
					break
			foreigner.get_node("AI").set_physics_process(false)
			foreigner.invulnerable = 1e9
			foreigner.refit("Freighter")
			foreigner.cargo = {"ice": 100.0}
			_alongside(foreigner, market)
			_check(w.cargo_port(foreigner) == market and Economy.pays_fee(market, foreigner), "a Drengin may trade at the Terran market, for a fee")
			credits_before = p.credits
			w.cargo_command(foreigner, "ice", -100.0)
			t0 = now
			step = 4
		4:
			_alongside(foreigner, market)
			if not foreigner.cargo_orders.is_empty() and now - t0 < 10.0:
				return false
			_check(p.credits - credits_before >= 10, "the founder took %d credits in fees" % (p.credits - credits_before))
			# a shipyard station refits away from home and builds ships
			yard = w.build_station(p.race_id, "Shipyard", jupiter, jupiter.global_position + Vector2(-jupiter.radius - 700.0, 0), false)
			yard.invulnerable = 1e9
			_alongside(p, yard)
			_check(w.nearest_base(p) == yard and CargoPanel.can_open(w, p), "alongside a shipyard station: the shipyard and the panel open")
			p.refit("Scout")
			p.rank = 7
			p.credits = 20000
			_check(World.materials_short(w.yard_stock(yard), "Cruiser") != "", "an empty yard can't build a Cruiser: %s" % World.materials_short(w.yard_stock(yard), "Cruiser"))
			w.apply_build(p, gd.stock_design("Cruiser"))
			_check(p.design_name == "Scout", "and the launch is refused")
			# a supply ship brings the materials, paid for what the yard is short of
			var hauler: Ship = null
			for s in w.ships:
				if s.support and s.race_id == p.race_id and s.design_name == "Freighter":
					hauler = s
					break
			hauler.invulnerable = 1e9
			hauler.cargo = {"metals": 40.0, "alloys": 60.0, "exotics": 20.0, "food": 50.0}
			hauler.cargo_origin = w.planet_by_name["Earth"]
			_alongside(hauler, yard)
			was = hauler.credits
			w.cargo_command(hauler, "all", -1.0)
			t0 = now
			step = 5
		5:
			var hauler: Ship = null
			for s in w.ships:
				if s.support and s.race_id == p.race_id and s.design_name == "Freighter":
					hauler = s
					break
			_alongside(hauler, yard)
			if not hauler.cargo_orders.is_empty() and now - t0 < 10.0:
				return false
			_check(yard.cargo.get("alloys", 0.0) >= 59.0 and yard.cargo.get("food", 0.0) < 1.0 and hauler.cargo.get("food", 0.0) >= 49.0,
				"the yard took the Metals, Alloys and Exotics and left the Food (yard %s)" % str(yard.cargo))
			_check(hauler.credits - was >= 40 + 60 * 3 + 20 * 4 - 5, "the hauler was paid %d for stocking the yard" % (hauler.credits - was))
			w.apply_build(p, gd.stock_design("Cruiser"))
			_check(p.design_name == "Cruiser" and absf(yard.cargo.get("alloys", 0.0) - 50.0) < 0.5, "a Cruiser launched from the shipyard, 10 Alloys used (%.0f left)" % yard.cargo.get("alloys", 0.0))
			# orders: an escort and a trade ship
			_alongside(p, yard)
			credits_before = p.credits
			w.order_command(p, "Destroyer")
			w.order_command(p, "Trader")
			_check(yard.orders.size() == 2 and p.credits == credits_before - 900 - 600, "ordered a Destroyer escort and a trade ship (%d credits)" % (credits_before - p.credits))
			p.rank = 0
			_check(w.order_refusal(p, yard, "Cruiser").contains("rank"), "a junior pilot can't order warships: %s" % w.order_refusal(p, yard, "Cruiser"))
			p.rank = 7
			launched = w.ships.filter(func(o): return o.has_meta("ordered")).size()
			t0 = now
			step = 6
		6:
			_alongside(p, yard)
			for s in w.ships:
				if s.has_meta("ordered"):
					s.invulnerable = 1e9      # Jupiter's Arcean moons shoot at anything Terran
			if not yard.orders.is_empty() and now - t0 < 120.0:
				return false
			var escort: Ship = null
			var hired: Ship = null
			for s in w.ships:
				if s.has_meta("ordered") and s.design_name == "Destroyer":
					escort = s
				if s.has_meta("ordered") and s.design_name == "Trader":
					hired = s
			_check(escort != null and escort.is_prize and escort.prize_of == p and escort.get_node("AI").escort == p, "the Destroyer launched as your wingman")
			_check(hired != null and hired.support and hired.get_node("AI").trader, "the trade ship launched and joined the empire's traders")
			# counted by the order, since the system's own ships come and go meanwhile
			_check(w.ships.filter(func(o): return o.has_meta("ordered")).size() == launched + 2, "two ships joined the system")
			# an ordered ship that is lost stays lost
			escort.invulnerable = 0.0
			escort.take_damage(1e6, null)
			t0 = now
			step = 7
		7:
			if now - t0 < w.AI_RESPAWN_DELAY + 2.0:
				return false
			_check(not w.ships.any(func(s): return s.has_meta("ordered") and s.design_name == "Destroyer"), "a lost escort does not come back")
			# a computer trade ship sells its load at the free port
			for s in w.ships:
				if s.support and s.design_name == "Trader" and s.race_id == p.race_id and not s.has_meta("ordered"):
					trader = s
			trader.invulnerable = 1e9
			trader.cargo = {"food": 200.0}
			trader.cargo_origin = w.planet_by_name["Earth"]
			_put(trader, port.global_position + Vector2(1500.0, 0).rotated(port.global_position.angle()), port.velocity)
			var ai: AIPilot = trader.get_node("AI")
			ai.plan_timer = 0.0
			ai.state = AIPilot.State.PATROL
			ai.set_physics_process(true)
			was = trader.credits
			t0 = now
			step = 8
		8:
			trader.invulnerable = 1e9
			if (trader.cargo.get("food", 0.0) >= 1.0 or not trader.cargo_orders.is_empty()) and now - t0 < 90.0:
				return false
			_check(trader.cargo.get("food", 0.0) < 1.0 and trader.credits > was, "the computer trade ship sold its Food at %s for %d credits (%.0f s)" % [
				trader.get_node("AI").dock_at.ship_name if trader.get_node("AI").dock_at != null else "the free port", trader.credits - was, now - t0])
			print("trade test %s" % ("FAILED" if failed else "passed"))
			quit(1 if failed else 0)
			return true
	return false
