# Governor test (the Resources update, phase 6): each empire's governor
# reads the threats to its worlds, builds and recruits on them, picks an
# enemy world for the fleet, sends a supply ship to put a station up, and
# posts a job board.  The pilot takes a haul job and is paid for it; a
# loaded supply ship becomes a convoy with an escort, and the pilot's
# escort job pays when it arrives and fails when it is lost.  Raiders and
# the pirate go for lone supply ships.
#   godot --headless --path . -s tools/governor_test.gd
extends SceneTree

var main: Node
var failed := false
var step := 0
var t0 := 0.0
var runner: Ship = null
var frames0 := 0
var job_id := 0
var credits0 := 0
var convoy: Ship = null
var guard: Ship = null


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)
	await process_frame
	await process_frame
	main._start_game()


func _check(ok: bool, what: String) -> void:
	print(("PASS  " if ok else "FAIL  ") + what)
	if not ok:
		failed = true


func _put(s: Ship, at: Vector2, vel: Vector2) -> void:
	s.global_position = at
	s.velocity = vel
	s.body.pos = at
	s.body.vel = vel


## Park a ship just above a body, moving with it.
func _park(w: World, s: Ship, p: Planet, alt: float = 60.0) -> void:
	var dir := (p.global_position - w.sun.global_position).normalized().rotated(0.7)
	_put(s, p.global_position + dir * (p.radius + alt), p.velocity)


func _freeze(s: Ship, on: bool) -> void:
	var ai: AIPilot = s.get_node_or_null("AI")
	if ai != null:
		ai.set_physics_process(not on)
	if on:
		for k in s.inputs.keys():
			s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
		if s.warping:
			s.end_warp()


func _terran_haulers(w: World) -> Array:
	return w.ships.filter(func(s): return s.support and s.race_id == 1 and not s.dead and s.design_name == "Freighter")


func _frames(w: World, race: int) -> int:
	return w.ships.filter(func(s): return s.is_station and s.race_id == race and not s.dead and not s.is_starbase()).size()


func _process(_delta: float) -> bool:
	if main == null or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	var p: Ship = w.player
	var now := w.elapsed
	p.invulnerable = 1e9
	if runner != null:
		for h in _terran_haulers(w):
			h.invulnerable = 1e9      # the kit run calls at Uranus, among Drengin warships; the test needs both later
	var earth: Planet = w.planet_by_name["Earth"]
	var luna: Planet = w.planet_by_name["Luna"]
	var mars: Planet = w.planet_by_name["Mars"]
	match step:
		0:
			if now < 1.0:
				return false
			_check(p.race_id == 1, "flying for the Terrans")
			w.message.connect(func(t, _c): print("  msg: " + t))
			_check(w.governors.size() == 3 and w.governors.has(0) and w.governors.has(2), "each empire has a governor")
			var gov: Governor = w.governors[p.race_id]
			var drengin: Governor = w.governors[2]
			# a Drengin warship over Luna: a threat
			var foe: Ship = null
			for s in w.ships:
				if s.race_id == 2 and not s.support and not s.is_station and not s.dead and not s.weapons.is_empty():
					foe = s
					break
			_freeze(foe, true)
			foe.invulnerable = 1e9
			_park(w, foe, luna, 900.0)
			gov._read_threats()
			_check(gov.threatened(luna), "a Drengin warship over Luna makes it threatened (%.1f)" % gov.threat.get(luna, 0.0))
			_check(is_equal_approx(gov.garrison_target(luna), float(Ground.GARRISON_TARGET[luna.colony_design]) * Governor.THREAT_GARRISON), "a threatened world wants more troops (%.1f)" % gov.garrison_target(luna))
			_put(foe, foe.global_position + Vector2(0, 30000), foe.velocity)
			# buildings: Earth has its barracks and shield, so industry is next
			earth.build_q.clear()
			earth.stock["metals"] = 200.0
			earth.stock["alloys"] = 40.0
			gov._read_threats()
			var key := gov.building_for(earth)
			gov._build_on(earth)
			_check(key != "" and not earth.build_q.is_empty() and earth.build_q[0][0] == key, "Earth's governor queues a building: %s" % key)
			# troops: an empty Mars recruits
			mars.garrison = {}
			mars.recruit_q.clear()
			mars.stock["metals"] = 60.0
			drengin._recruit_on(mars)
			_check(not mars.recruit_q.is_empty(), "an empty garrison recruits (%s)" % [mars.recruit_q])
			# the war: a world to go after, and attack duty goes for it
			drengin._pick_target()
			var tgt := drengin.target
			_check(tgt != null and tgt.owner_race != 2 and tgt.is_colonised(), "the Drengin pick an enemy world to go after: %s" % (tgt.body_name if tgt != null else "none"))
			var attacker: AIPilot = null
			for s in w.ships:
				if s.race_id == 2 and not s.support and not s.dead and s.get_node_or_null("AI") != null and s.get_node("AI").bombard:
					attacker = s.get_node("AI")
					break
			if attacker != null and tgt != null and not tgt.is_defenceless():
				_freeze(attacker.ship, true)
				_put(attacker.ship, tgt.global_position + Vector2(tgt.radius + 3000.0, 0), tgt.velocity)
				_check(attacker._colony_to_bombard() == tgt, "a pilot on attack duty goes for it")
				_freeze(attacker.ship, false)
			# raiders: the lone supply ship before the convoy
			var haulers := _terran_haulers(w)
			var lone: Ship = haulers[0]
			var guarded: Ship = haulers[1]
			var raider: AIPilot = foe.get_node("AI")
			raider.raid = true
			var spot := mars.global_position + Vector2(0, -(mars.radius + 2500.0))
			for h in [lone, guarded]:
				_freeze(h, true)
				if h.is_landing():
					h._end_landing()      # a kit run may start with a landing for its materials
			_put(foe, spot, mars.velocity)
			_put(guarded, spot + Vector2(400, 0), mars.velocity)
			_put(lone, spot + Vector2(-900, 0), mars.velocity)
			guarded.convoy = true
			# the raider's sensors cache what they saw a moment ago, from where it was
			foe.sensors._own.clear()
			foe.sensors._seen.clear()
			_check(raider._raid_target() == lone, "a raider goes for the lone supply ship, not the convoy")
			guarded.convoy = false
			_check(raider._raid_target() == guarded, "and for the nearest when neither is guarded")
			_freeze(lone, false)
			_freeze(guarded, false)
			_put(foe, foe.global_position + Vector2(0, 30000), foe.velocity)
			var pirate: Ship = null
			for s in w.ships:
				if s.race_id == 3:
					pirate = s
			_check(pirate != null and pirate.get_node("AI").raid, "the pirate raider hunts supply ships too")
			# a station kit: the governor sends an empty supply ship
			for h in _terran_haulers(w):
				h.cargo.clear()
				h.get_node("AI").state = AIPilot.State.PATROL
				h.get_node("AI").job = ""
			# a gas giant has room for every empire's skimmer; a rock holds one mine
			var uranus: Planet = w.planet_by_name["Uranus"]
			var theirs: Ship = w.build_station(2, "GasSkimmer", uranus, uranus.global_position + Vector2(uranus.radius + 400.0, 0))
			var site: Dictionary = gov.choose_site([])
			_check(site.get("key", "") == "GasSkimmer" and site.get("body") == uranus, "a Drengin skimmer at Uranus leaves room for a Terran one (%s at %s)" % [
				site.get("key", "-"), site["body"].body_name if site.get("body") != null else "-"])
			var ours: Ship = w.build_station(1, "GasSkimmer", uranus, uranus.global_position + Vector2(-uranus.radius - 400.0, 0))
			var rock: Dictionary = gov.choose_site([ours])
			var mine: Ship = null
			if rock.get("key", "") == "MiningStation":
				mine = w.build_station(2, "MiningStation", rock["body"], rock["body"].global_position + Vector2(rock["body"].radius + 300.0, 0))
				site = gov.choose_site([ours])
				_check(site.get("body") != rock["body"], "a Drengin mine at %s turns the Terran mine elsewhere (%s at %s)" % [rock["body"].body_name,
					site.get("key", "-"), site["body"].body_name if site.get("body") != null else "-"])
				w.remove_ship(mine)
			else:
				_check(false, "with a skimmer up, the Terrans want a mine next (%s)" % rock.get("key", "-"))
			w.remove_ship(ours)
			w.remove_ship(theirs)
			frames0 = _frames(w, 1)
			for k in ["metals", "alloys", "exotics"]:
				earth.stock[k] = Economy.cap(earth) * 0.5
			gov._station_kits()
			for h in _terran_haulers(w):
				if h.get_node("AI").job in ["stock", "kit"]:
					runner = h
			_check(runner != null, "the governor sends a supply ship for a %s kit at %s" % [runner.get_node("AI").haul_res if runner != null else "?", runner.get_node("AI").kit_body.body_name if runner != null else "?"])
			if runner == null:
				step = 3
				return false
			var base: Ship = w.starbase_of(1)
			_put(runner, base.global_position + Vector2(base.size + runner.size + 120.0, 0), base.velocity)
			t0 = now
			step = 1
		1:
			var ai: AIPilot = runner.get_node("AI")
			if ai.job != "deploy" and now - t0 < 90.0:
				return false
			_check(ai.job == "deploy" and runner.kit != "", "it took the kit aboard at the starbase (%s; %s %s)" % [runner.kit, AIPilot.State.keys()[ai.state], ai.job])
			var cost: Dictionary = GameData.STATIONS.get(runner.kit, {}).get("cost", {"?": 1})
			_check(cost.keys().all(func(k): return runner.cargo.get(k, 0.0) >= float(cost[k])), "with the frame's materials from Earth aboard (%s)" % [runner.cargo])
			# a long flight, shortened
			_park(w, runner, ai.kit_body, 500.0)
			t0 = now
			step = 2
		2:
			if _frames(w, 1) == frames0 and now - t0 < 30.0:
				return false
			_check(_frames(w, 1) == frames0 + 1 and runner.kit == "", "and put the frame up (%d stations)" % _frames(w, 1))
			var rai: AIPilot = runner.get_node("AI")
			_check(rai.job == "supply" and rai.dock_at != null and rai.dock_at.building, "and goes straight alongside it with the materials (%s)" % rai.job)
			step = 3
		3:
			# the job board: Luna short of Food, Earth with plenty
			var gov: Governor = w.governors[p.race_id]
			for h in w.ships:
				if h.support and h.race_id == 1:
					_freeze(h, true)
					var ai: AIPilot = h.get_node("AI")
					ai.haul_to = null
					ai.job = ""
					ai.job_planet = null
			gov.jobs.clear()
			luna.stock["food"] = 0.0
			earth.stock["food"] = 900.0
			gov.refresh_board()
			var job := {}
			for j in gov.jobs:
				if j["kind"] == "haul" and j["to"] == luna and j["res"] == "food":
					job = j
			_check(not job.is_empty(), "the board posts a Food run to Luna (%s)" % (gov.job_text(job) if not job.is_empty() else ", ".join(gov.jobs.map(func(j): return gov.job_text(j)))))
			if job.is_empty():
				step = 5
				return false
			p.refit("Freighter")
			p.rank = 3
			w.job_command(p, job["id"])
			_check(job["taken"] == p and p.nav_target == job["from"], "the pilot took it: nav to %s" % w.port_name(job["from"]))
			_check(gov.taken_runs().has([luna, "food"]), "the computer supply ships leave that run alone")
			var rows := gov.pack_board()
			_check(rows.size() == gov.jobs.size() and rows[0].size() == 6 and Governor.nav_node(w, Governor.nav_ref(w, luna)) == luna, "the board packs for the wire")
			job_id = job["id"]
			_park(w, p, job["from"])
			w.try_land(p, job["from"])
			t0 = now
			step = 4
		4:
			var gov: Governor = w.governors[p.race_id]
			var job := gov.job_by_id(job_id)
			if not p.is_landed() and p.get_meta("leg", 0) == 0:
				return false
			match int(p.get_meta("leg", 0)):
				0:
					w.cargo_command(p, "food", job["amount"])
					p.set_meta("leg", 1)
				1:
					if not p.cargo_orders.is_empty():
						return false
					p.leave_ground_now()
					_park(w, p, luna)
					w.try_land(p, luna)
					p.set_meta("leg", 2)
				2:
					if not p.is_landed():
						return false
					credits0 = p.credits
					w.cargo_command(p, "food", -job["amount"])
					p.set_meta("leg", 3)
				3:
					if not p.cargo_orders.is_empty() and now - t0 < 40.0:
						return false
					_check(gov.job_by_id(job_id).is_empty(), "delivered: the job is done")
					_check(p.credits > credits0 + Governor.JOB_FLAT, "and paid (%d credits)" % (p.credits - credits0))
					p.leave_ground_now()
					step = 5
		5:
			if p.is_landing():
				return false
			# a convoy: a loaded supply ship bound for Luna, a pilot on escort duty
			var gov: Governor = w.governors[p.race_id]
			convoy = _terran_haulers(w)[0]
			for s in w.ships:
				if s.race_id == 1 and not s.support and not s.is_station and not s.dead and not s.is_player and s.get_node_or_null("AI") != null:
					guard = s
					break
			_freeze(convoy, true)
			_freeze(guard, true)
			var cai: AIPilot = convoy.get_node("AI")
			convoy.cargo = {"metals": 200.0}
			cai.job = "deliver"
			cai.job_planet = luna
			cai.dock_at = null
			cai.state = AIPilot.State.FERRY
			var far := luna.global_position + Vector2(6000, 0)
			_put(convoy, far, luna.velocity)
			var gai: AIPilot = guard.get_node("AI")
			gai.escort_duty = true
			gai.escort = null
			gai.state = AIPilot.State.PATROL
			guard.hull = guard.hull_max
			_put(guard, far + Vector2(600, 0), luna.velocity)
			gov.form_convoys()
			_check(convoy.convoy and gov.convoys.has(convoy), "a supply ship with 200 Metals aboard is a convoy")
			_check(gai.escort == convoy and guard.guarding == convoy and gai.state == AIPilot.State.ESCORT, "a pilot on escort duty flies with it")
			var ej := {}
			for j in gov.jobs:
				if j["kind"] == "escort" and j["convoy"] == convoy:
					ej = j
			_check(not ej.is_empty(), "and the board posts an escort job: %s, %d credits" % [gov.job_text(ej) if not ej.is_empty() else "-", ej.get("pay", 0)])
			if ej.is_empty():
				step = 7
				return false
			p.refit("Destroyer")
			w.job_command(p, ej["id"])
			_check(ej["taken"] == p and p.nav_target == luna, "the pilot took the escort job: nav to Luna, where it is bound")
			# the run, shortened: the convoy reaches Luna with the pilot alongside
			_park(w, convoy, luna)
			_put(p, convoy.global_position + Vector2(300, 0), luna.velocity)
			credits0 = p.credits
			job_id = ej["id"]
			w.try_land(convoy, luna)
			t0 = now
			step = 6
		6:
			var gov: Governor = w.governors[p.race_id]
			_put(p, convoy.global_position + Vector2(300, 0), luna.velocity)
			if not gov.job_by_id(job_id).is_empty() and now - t0 < 8.0:
				return false
			_check(gov.job_by_id(job_id).is_empty() and p.credits > credits0, "the convoy landed: the escort is paid (%d credits)" % (p.credits - credits0))
			_check(not convoy.convoy and guard.guarding == null and guard.get_node("AI").escort == null, "and its escort goes back to patrol")
			# a convoy lost
			var other: Ship = _terran_haulers(w)[1]
			_freeze(other, true)
			var oai: AIPilot = other.get_node("AI")
			other.cargo = {"exotics": 60.0}
			oai.job = "deliver"
			oai.job_planet = earth
			oai.dock_at = null
			_put(other, earth.global_position + Vector2(7000, 0), earth.velocity)
			gov.form_convoys()
			var lost := {}
			for j in gov.jobs:
				if j["kind"] == "escort" and j["convoy"] == other:
					lost = j
			w.job_command(p, lost.get("id", 0))
			other.invulnerable = 0.0
			other.take_damage(1e7, null)
			gov.form_convoys()
			_check(not lost.is_empty() and gov.job_by_id(lost["id"]).is_empty() and not gov.convoys.has(other), "a convoy lost: its escort job is gone")
			step = 7
		7:
			print("governor test " + ("FAILED" if failed else "passed"))
			quit(1 if failed else 0)
			return true
	return false
