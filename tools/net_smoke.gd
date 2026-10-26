# Online smoke test: two headless processes, one hosting and one joining,
# fly together for a while and report what crossed the wire.  Usage:
#   godot --headless --path . -s tools/net_smoke.gd -- --host
#   godot --headless --path . -s tools/net_smoke.gd -- --join=127.0.0.1
# The host runs 30 s, the client 20 s; exit code 1 when a check fails.
extends SceneTree

var frames := 0
var main: Node
var failed := false
var is_host := "--host" in OS.get_cmdline_user_args()
var peak_ships := 0
var moved := 0.0
var start_pos := Vector2.INF
var net: Node
var shipyard_opened := false
var build_sent := false
var hooked := false
var trade_seen := false
var trade_hooked := false
var joiner_seen_at := -1
var world_seen_at := -1
var trade_requested := false
var parked := false
var trade_done := false
var credits_before := -1
var joiner_score_at_seat := -1
var offer_sent := false
var disabled_one := false
var captured_one := false
var capture_victim: Node = null
var saw_disabled := false
var saw_prize := false
var saw_cargo := false
var saw_supply := false
var saw_frame := false
var saw_station := false
var depot: Node = null
var saw_free_port := false
var saw_order := false
var saw_ground := false
var saw_troops := false
var board_posted := false
var job_sent := false
var saw_job := false
var order_put := false


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	net = root.get_node("/root/Net")
	var settings = root.get_node("/root/Settings")
	settings.pilot_name = "HostPilot" if is_host else "Joiner"
	if not is_host:
		settings.career["score"] = 777   # a career the host must seat us with
		settings.career["kills"] = 3
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		# Main.gd failed to compile: usually a stale class cache after new
		# class_name files were pulled.  Fix: godot --headless --path . --import
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)   # Main reads --host / --join itself
	print("[%s] started, role=%d" % [_tag(), net.role])


func _tag() -> String:
	return "host" if is_host else "client"


func _process(_delta: float) -> bool:
	frames += 1
	var w = main.world if main != null else null
	var p = w.player if w != null else null
	if w != null and not hooked:
		hooked = true
		w.shipyard_requested.connect(func(s, _b):
			shipyard_opened = true
			print("[%s] shipyard opened for %s with %d credits" % [_tag(), s.pilot_name, s.credits]))
	if w != null and not trade_hooked:
		trade_hooked = true
		w.message.connect(func(t, _c): print("[%s] msg: %s" % [_tag(), t]))
		net.trade_opened.connect(func(d):
			trade_seen = true
			print("[%s] trade opened with %s, my credits %d" % [_tag(), d["partner_name"], d["credits"]])
			# offer: the host gives 150 credits, the joiner gives a part if it has one
			# and the joiner 3 Metals from its cargo hold
			var offer := {"credits": 150 if is_host else 0, "parts": ([] if is_host or d["hold"].is_empty() else [0]), "blueprints": [],
				"cargo": ({} if is_host else {"metals": 3})}
			net.trade_cmd("offer", {"id": d["id"], "offer": offer})
			offer_sent = true
			net.trade_cmd("accept", {"id": d["id"]}))
		net.trade_updated.connect(func(d):
			# accept again whenever the host resets it, but only once our own offer is on the table
			if offer_sent and not d["accepted"].values().all(func(v): return v):
				net.trade_cmd("accept", {"id": d["id"]}))
		net.trade_closed.connect(func(d):
			trade_done = d.get("done", false)
			print("[%s] trade closed: %s credits now %d" % [_tag(), d.get("reason", ""), d.get("credits", -1)]))
	var now := Time.get_ticks_msec()
	if w != null and world_seen_at < 0:
		world_seen_at = now
	if is_host and w != null and joiner_seen_at < 0:
		for s in w.ships:
			if s.pilot_name == "Joiner":
				joiner_seen_at = now
				joiner_score_at_seat = s.score
	if is_host and w != null and joiner_seen_at > 0:
		for s in w.ships:
			if s.pilot_name == "Joiner" and not s.dead and s.cargo_max >= 3.0:
				s.cargo["metals"] = 3.0      # the client should see its hold (again after a death or a refit)
			if s.pilot_name == "Joiner" and not s.dead:
				s.cargo_max = maxf(s.cargo_max, 300.0)   # room enough for a haul job
		# the job board: Luna short of Food, Earth with plenty
		if not board_posted and now - joiner_seen_at > 4000:
			board_posted = true
			w.planet_by_name["Luna"].stock["food"] = 0.0
			w.planet_by_name["Earth"].stock["food"] = 900.0
			for s in w.ships:
				if s.pilot_name == "Joiner":
					# a run of its own, whatever the computer supply ships are on
					var gov: Governor = w.governors[s.race_id]
					var j := gov._job("haul")
					j["from"] = w.planet_by_name["Earth"]
					j["to"] = w.planet_by_name["Luna"]
					j["res"] = "food"
					j["amount"] = 100.0
					j["pay"] = 100
					gov.jobs.append(j)
					print("[host] posted a job for the joiner's empire (%d)" % s.race_id)
		# a station frame goes up, and is finished a few seconds later
		if depot == null:
			# out at Jupiter, well clear of the pilots launching from the starbases
			var jupiter = w.planet_by_name["Jupiter"]
			depot = w.build_station(1, "SupplyDepot", jupiter, jupiter.global_position + Vector2(0, jupiter.radius + 900.0))
			depot.invulnerable = 1e9
		elif depot.building and now - joiner_seen_at > 7000:
			w.finish_station(depot)
		# the ground war: a battle for Venus held where it is, Mars's shield,
		# and marines aboard the joiner's ship
		if now - joiner_seen_at > 5000:
			var venus = w.planet_by_name["Venus"]
			venus.garrison = {"armour": 2.0}
			venus.battle = {"race": 1, "units": {"infantry": 3.0}, "militia": 1.0, "pilot": "Joiner", "sabotage": 0.0}
			for s in w.ships:
				if s.pilot_name == "Joiner" and not s.dead and s.troop_max >= 1:
					s.troops = {"infantry": 1.0}
		# an order on the Terran starbase's slip (long, so it is still there at the end)
		if not order_put and now - joiner_seen_at > 5000:
			order_put = true
			w.starbase_of(1).orders.append(["Freighter", 600.0, "Joiner"])
	if not is_host and w != null:
		for s in w.ships:
			if s.design_name == "SupplyDepot":
				if s.building and s.cargo_max == 25.0 and s.design["modules"] == ["1001"]:
					saw_frame = true
				elif not s.building and s.cargo_max == 1500.0 and s.design["modules"].size() == 3:
					saw_station = true
	if not is_host and w != null and p != null and p.cargo.get("metals", 0.0) == 3.0:
		saw_cargo = true
	# the board reaches the joiner, who takes a haul job; the host's word comes back
	if not is_host and w != null and p != null:
		for row in w.job_board:
			if row[1] == "haul" and row[4] == "" and not job_sent:
				job_sent = true
				w.job_command(p, row[0])
				print("[client] took job: %s" % row[2])
			if row[4] == p.pilot_name:
				saw_job = true
	if not is_host and w != null:
		for s in w.ships:
			if s.race_id == 4 and s.design_name == "TradeStation" and s.cargo_total() > 500.0:
				saw_free_port = true
			if s.is_starbase() and s.orders.size() == 1 and s.orders[0][0] == "Freighter":
				saw_order = true
	if not is_host and w != null:
		var venus = w.planet_by_name["Venus"]
		var mars = w.planet_by_name["Mars"]
		if venus.garrison.get("armour", 0.0) == 2.0 and int(venus.battle.get("race", -1)) == 1 and mars.buildings.has("shield") and mars.pshield > 0.0:
			saw_ground = true
		if p != null and p.troops.get("infantry", 0.0) == 1.0:
			saw_troops = true
	if not is_host and w != null and w.ships.any(func(x): return x.support and x.design_name == "Freighter"):
		saw_supply = true
	if is_host and w != null and joiner_seen_at > 0 and now - joiner_seen_at > 3000 and not trade_requested:
		trade_requested = true
		# bring the joiner alongside and open a trade with them
		for s in w.ships:
			if s.pilot_name == "Joiner":
				s.global_position = p.global_position + Vector2(200, 0)
				s.velocity = p.velocity
				s.hold.append("5002")
				p.credits = 500
				p.cargo.clear()
				credits_before = s.credits
				p.target = s
				net.request_trade(p, s)
				print("[host] trade requested with the joiner")
	if is_host and w != null and joiner_seen_at > 0 and now - joiner_seen_at > 7000 and not parked:
		parked = true
		# park the joiner at their starbase so the shipyard lets them in
		for s in w.ships:
			if s.pilot_name == "Joiner":
				var base = w.nearest_base(s)
				for o in w.ships:
					if o.is_starbase() and o.race_id == s.race_id and not o.dead:
						base = o
				if base != null:
					s.global_position = base.global_position + Vector2(base.size + 100.0, 0)
					s.velocity = base.velocity
					print("[host] parked the joiner at the starbase")
	# systems and capture: the host shoots out an enemy's engines, then
	# boards it; the client should see both
	if is_host and w != null and joiner_seen_at > 0 and now - joiner_seen_at > 4000 and not disabled_one:
		disabled_one = true
		for s in w.ships:
			if not s.dead and not s.is_station and s.race_id != p.race_id and not s.is_player:
				capture_victim = s
				break
		if capture_victim != null:
			var e: Dictionary = capture_victim.systems.get_system("engines")
			e["hp"] = 0.0
			e["offline"] = true
			capture_victim.systems.quiet = -1000.0
			capture_victim.apply_systems()
			print("[host] knocked out the engines of %s" % capture_victim.pilot_name)
	if is_host and capture_victim != null and is_instance_valid(capture_victim) and not capture_victim.dead and not captured_one:
		capture_victim.systems.quiet = -1000.0
		if now - joiner_seen_at > 6000:
			captured_one = true
			var purse: int = p.credits
			w.capture.take(p, capture_victim)
			p.credits = purse       # keep the trade check's arithmetic
			print("[host] captured %s" % capture_victim.ship_name)
	if not is_host and w != null:
		for s in w.ships:
			if s.is_prize:
				saw_prize = true
			elif not s.dead and s.systems.is_offline("engines"):
				saw_disabled = true
	if not is_host and p != null and world_seen_at > 0 and now - world_seen_at > 9000 and frames % 7 == 0 and not shipyard_opened and not build_sent:
		net.client_command("refit")   # the host parked us at the base a moment ago
	if not is_host and shipyard_opened and not build_sent and p != null:
		build_sent = true
		net.client_build(p, p.design)
		print("[%s] build sent" % _tag())
	if p != null and not p.dead and not p.is_puppet and not p.docked:
		# fly in a slow circle and shoot
		p.inputs["thrust"] = 1.0
		p.inputs["turn"] = 0.4
		p.inputs["fire_primary"] = frames % 40 < 20
		if start_pos == Vector2.INF:
			start_pos = p.global_position
		moved = maxf(moved, p.global_position.distance_to(start_pos))
	if w != null:
		peak_ships = maxi(peak_ships, w.ships.size())
	if frames % 300 == 0:
		print("[%s] wall=%dms phys=%d t=%ds ships=%d missiles=%d snapshots=%d inputs=%d seats=%d player=%s pos=%s" % [
			_tag(), Time.get_ticks_msec(), Engine.get_physics_frames(), frames / 60, (w.ships.size() if w else -1), (w.missiles.size() if w else -1),
			net.snapshots_received, net.inputs_received, net.seats.size(),
			(p.pilot_name if p else "none"), (p.global_position.round() if p else Vector2.ZERO)] + ("  ping=%dms corr=%.1fpx pickups=%d" % [int(net.ping_ms), (net.client.correction_px if net.client else 0.0), (w.powerups.size() if w else 0)] if not is_host else ""))
	var done: bool = (now - world_seen_at > 36000) if is_host else (world_seen_at > 0 and now - world_seen_at > 15000)
	if done:
		_finish(w)
		quit(1 if failed else 0)
	return false


func _check(ok: bool, what: String) -> void:
	print("[%s] %s: %s" % [_tag(), "ok  " if ok else "FAIL", what])
	if not ok:
		failed = true


func _finish(w) -> void:
	_check(w != null, "world exists")
	if w == null:
		return
	if is_host:
		_check(net.inputs_received > 200, "received inputs from the joiner (%d)" % net.inputs_received)
		_check(net.seats.size() >= 1 or net.inputs_received > 0, "a joiner took a seat")
		var human := 0
		for s in w.ships:
			if s.pilot_name == "Joiner":
				human += 1
		_check(human == 1 or net.seats.is_empty(), "the joiner's ship is named after them")
		_check(joiner_score_at_seat >= 777, "joiner seated with their career score (%d)" % joiner_score_at_seat)
		_check(trade_seen, "trade window opened for the host")
		_check(trade_done, "trade settled")
		for s in w.ships:
			if s.pilot_name == "Joiner":
				_check(s.credits == credits_before + 150 and s.hold.is_empty(), "joiner got 150 credits and gave the part (credits %d, hold %d)" % [s.credits, s.hold.size()])
		_check(w.player.credits == 350 and w.player.hold.has("5002"), "host paid 150 and got the part (credits %d)" % w.player.credits)
		_check(w.player.cargo.get("metals", 0.0) >= 3.0, "and the joiner's 3 Metals (hold %s)" % [w.player.cargo])
	else:
		_check(net.snapshots_received > 100, "received snapshots (%d)" % net.snapshots_received)
		_check(peak_ships >= 10, "roster arrived (%d ships)" % peak_ships)
		_check(w.player != null and not w.player.is_puppet, "own ship is predicted locally")
		_check(moved > 300.0, "own ship moved (%d px)" % int(moved))
		var moving := 0
		for s in w.ships:
			if s.is_puppet and not s.dead and s.velocity.length() > 1.0:
				moving += 1
		_check(moving > 3, "puppets move from snapshots (%d)" % moving)
		_check(net.ping_ms > 0.0, "ping measured (%d ms)" % int(net.ping_ms))
		_check(shipyard_opened, "shipyard opened for the joiner")
		_check(build_sent and not w.player.docked, "build accepted and the joiner undocked")
		_check(trade_seen and trade_done, "trade window opened and settled for the joiner")
		var rd = w.round_rules
		_check(rd != null and rd.elapsed > 5.0 and rd.phase != "", "round clock arrived (%s, %s)" % [(rd.clock_text() if rd else "-"), (rd.phase if rd else "-")])
		_check(saw_disabled, "a ship's shot-out engines showed on the client")
		_check(saw_prize, "the host's prize ship appeared on the client")
		var earth_stock: float = w.planet_by_name["Earth"].stock.get("food", 0.0)
		_check(earth_stock > 0.0, "the stockpiles arrived (Earth food %d)" % int(earth_stock))
		_check(saw_cargo, "the joiner's cargo hold arrived")
		_check(saw_supply, "the supply ships arrived as supply ships")
		_check(saw_frame, "a station frame went up on the client")
		_check(saw_station, "and was finished there")
		_check(saw_free_port, "the free port and its market stock arrived")
		_check(saw_order, "a starbase's order book arrived")
		_check(saw_ground, "garrisons, a ground battle and the planetary shields arrived")
		_check(saw_troops, "the marines aboard the joiner's ship arrived")
		_check(saw_job, "the job board arrived, and the joiner took a haul job (%d jobs)" % w.job_board.size())
		_check(net.client != null and net.client.correction_px < 60.0, "reconciliation error small (%.1f px)" % (net.client.correction_px if net.client else -1.0))
	print("[%s] %s" % [_tag(), "FAILED" if failed else "passed"])
