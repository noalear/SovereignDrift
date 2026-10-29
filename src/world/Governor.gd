## Governor -- runs one empire on the host (the Resources update, phase 6).
## Every EVERY seconds it reads its worlds' shortages and the threats to
## them and acts: buildings and troops on the worlds, a station kit to the
## best open site, an enemy world for the fleet to go after, and a job
## board of the runs its own ships are not on, for pilots to take for pay.
## Every CONVOY_EVERY seconds it forms convoys: a supply or trade ship
## carrying a valuable load gets warships on escort duty, and pilots an
## escort job.
class_name Governor
extends RefCounted

const EVERY := 20.0
const CONVOY_EVERY := 2.0
## Enemy warships this close to a world make it threatened.
const THREAT_RANGE := 3500.0
## Goods worth this many credits make a run a convoy, with this many escorts.
const CONVOY_VALUE := 120.0
const CONVOY_ESCORTS := 2
## Haul jobs on the board at once.
const BOARD_HAULS := 4
## A haul job pays this share of the goods' price on top of the usual
## delivery pay, plus a flat sum; an escort job this share of the
## convoy's load, to a pilot within ESCORT_RANGE of it when it arrives.
const JOB_SHARE := 0.6
const JOB_FLAT := 40
const ESCORT_SHARE := 0.5
const ESCORT_RANGE := 2500.0
## A taken job lapses after this long.
const JOB_LAPSE := 420.0
## Stations an empire puts up on its own, frames included.
const MAX_STATIONS := 6
## Troops a threatened world keeps, over its usual target.
const THREAT_GARRISON := 1.5

var world: Node
var race := 0
var _govern := 0.0
var _convoy := 0.0
var jobs: Array = []            # see _job(); on the host only
var target: Planet = null       # the enemy world the fleet goes after
var threat := {}                # Planet -> how threatened it is
var convoys := {}               # hauler -> {"guards": [escorts], "end": where the run ends}

static var _next_id := 1


func _init(p_world: Node = null, p_race: int = 0) -> void:
	world = p_world
	race = p_race
	# the empires take turns, not all on one frame
	_govern = 3.0 + p_race * 2.5
	_convoy = p_race * 0.6


## Every host physics frame.
func tick(delta: float) -> void:
	_govern -= delta
	if _govern <= 0.0:
		_govern = EVERY
		govern()
	_convoy -= delta
	if _convoy <= 0.0:
		_convoy = CONVOY_EVERY
		form_convoys()


## The empire's colonies with a surface, for most of what follows.
func colonies() -> Array:
	var out := []
	for p in world.planets:
		if p.is_colonised() and p.owner_race == race:
			out.append(p)
	return out


## One round of decisions.
func govern() -> void:
	_read_threats()
	_pick_target()
	for p in colonies():
		_build_on(p)
		_recruit_on(p)
	_station_kits()
	refresh_board()


# ------------------------------------------------------------ the worlds

func _read_threats() -> void:
	threat.clear()
	for p in colonies():
		var t := 0.0
		for s in world.ships:
			if s.dead or s.support or s.is_station or s.race_id == race or s.race_id == GameData.NEUTRAL:
				continue
			if s.team() == _team() or s.weapons.is_empty():
				continue
			if s.global_position.distance_to(p.global_position) < THREAT_RANGE:
				t += 1.0
		if p.under_attack > 0.0 or not p.battle.is_empty():
			t += 2.0
		if t > 0.0:
			threat[p] = t


func _team() -> int:
	var sb: Ship = world.starbase_of(race)
	return sb.team() if sb != null else race


func threatened(p: Planet) -> bool:
	return threat.get(p, 0.0) > 0.0


## What this world should build next: barracks and a shield where there is
## danger or much to lose, food where it goes hungry, industry on the big
## worlds, guns where the enemy comes.  The first the world can afford.
func building_for(p: Planet) -> String:
	var danger := threatened(p)
	var big: bool = p.colony_design in ["Minor", "Major"]
	var home: bool = GameData.STARBASES.get(race, "") == p.body_name
	var want := []
	if danger or big:
		want.append("barracks")
	if danger or home or p.colony_design == "Major":
		want.append("shield")
	if p.short.has("food") or Economy.need(p, "food") > 0.7:
		want.append("farm")
	if danger:
		want.append("flak")
	if big:
		want.append("industry")
	if danger:
		want.append_array(["laser", "silo"])
	for key in want:
		if Ground.building_refusal(null, p, key) == "":
			return key
	return ""


func _build_on(p: Planet) -> void:
	if not p.build_q.is_empty() or not p.battle.is_empty():
		return
	var key := building_for(p)
	if key != "":
		Ground.queue_building(p, key)


## Keep the garrison up to its target, half as much again under threat.
## Armour now and then when the stockpile runs to it.
func garrison_target(p: Planet) -> float:
	var t := float(Ground.GARRISON_TARGET.get(p.colony_design, 2.0))
	return t * (THREAT_GARRISON if threatened(p) else 1.0)


func _recruit_on(p: Planet) -> void:
	if not p.recruit_q.is_empty() or not p.buildings.has("barracks"):
		return
	if Ground.strength(p.garrison) >= garrison_target(p):
		return
	var unit := "infantry"
	# armour burns Fuel for good: only with FUEL_RESERVE minutes of the
	# world's upkeep, armour's own included, in hand or a well of its own
	var fuel_ok: bool = Economy.makes(p, "fuel") >= 1.0 or p.stock.get("fuel", 0.0) >= 20.0 \
		+ (Ground.upkeep_rate(p).get("fuel", 0.0) + 1.0) * Economy.FUEL_RESERVE
	if p.stock.get("metals", 0.0) >= 40.0 and fuel_ok and p.stock.get("alloys", 0.0) >= 10.0 and randf() < 0.35:
		unit = "armour"
	if Ground.recruit_refusal(null, p, unit) == "":
		Ground.queue_recruit(p, unit)


# ------------------------------------------------------------ the war

## The enemy world worth going after: weak, and close to our own.  The
## choice holds until a clearly better one comes up.
func _pick_target() -> void:
	var ours := colonies()
	if ours.is_empty():
		target = null
		return
	var best: Planet = null
	var best_v := 0.0
	var held := 0.0
	for p in world.planets:
		if not p.is_colonised() or p.owner_race == race or p.owner_race == GameData.NEUTRAL or not p.has_surface():
			continue
		var d := INF
		for o in ours:
			d = minf(d, o.global_position.distance_to(p.global_position))
		var weak: float = Ground.defence(p) + p.pshield / 1000.0 + (0.0 if p.is_defenceless() else 2.0 + p.hull / 1500.0)
		var v: float = 10000.0 / ((2000.0 + d) * (1.0 + weak))
		if p == target:
			held = v
		if v > best_v:
			best_v = v
			best = p
	if target != null and held > 0.0 and best_v < held * 1.5:
		return
	target = best


# ------------------------------------------------------------ stations

## One kit run at a time, up to MAX_STATIONS: a skimmer when the empire is
## short of Fuel, a mine on the richest open rock near home, a refinery
## when Alloys run low, a guard post over a world under threat, a sensor
## station at home.
func _station_kits() -> void:
	var have := []
	for s in world.ships:
		if s.is_station and not s.dead and s.race_id == race and not s.is_starbase():
			have.append(s)
	var runner: Ship = null
	for s in world.ships:
		if s.dead or s.race_id != race or not s.support:
			continue
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai != null and (ai.job in ["stock", "kit", "deploy"] or s.kit != ""):
			return                  # one run at a time
	if have.size() >= MAX_STATIONS:
		return
	var site := choose_site(have)
	if site.is_empty():
		return
	# home's shield running dry: the haulers fetch Fuel first, and only a
	# skimmer (more Fuel) is worth taking one off that
	if site["key"] != "GasSkimmer" and Economy.running_low(world._home_planet(race), "fuel"):
		return
	runner = _idle_hauler(site["body"])
	if runner != null:
		runner.get_node("AI").start_kit(site["key"], site["body"])


## {"key": station, "body": Planet} or {}.
func choose_site(have: Array) -> Dictionary:
	var home: Planet = world._home_planet(race)
	var built := {}
	for s in have:
		built[s.design_name] = int(built.get(s.design_name, 0)) + 1
	var short := {}
	for k in Economy.RESOURCES:
		var n := 0.0
		for p in colonies():
			n += Economy.need(p, k)
		short[k] = n
	# a rock holds one mine, whoever's (World.station_site): a rock another
	# empire got to first turns the kit away on arrival
	var claimed := {}
	for s in world.ships:
		if s.is_station and not s.dead and s.station_body != null and GameData.STATIONS.get(s.design_name, {}).get("where", "") == "rock":
			claimed[s.station_body] = true
	var best := {}
	var best_v := 0.0
	for p in world.planets:
		if p.look == "Sun":
			continue
		var d: float = 3000.0 + home.global_position.distance_to(p.global_position)
		var taken := false
		for s in have:
			if s.station_body == p:
				taken = true
		if taken or _hostile_near(p):
			continue
		var gas := Economy.surface_of(p.look, p.habitable) == "gas"
		if gas:
			var v: float = float(p.richness.get("fuel", 0)) * (1.0 + short["fuel"]) * 10000.0 / d
			if int(built.get("GasSkimmer", 0)) == 0:
				v *= 3.0           # shields and armour run on Fuel: the first skimmer comes early
			if v > best_v:
				best_v = v
				best = {"key": "GasSkimmer", "body": p}
		elif p.has_surface() and not p.is_colonised() and world._minable(p) and not claimed.has(p):
			var v := 0.0
			for k in ["metals", "ice", "exotics"]:
				v += float(p.richness.get(k, 0)) * (1.0 + short[k]) * GameData.RESOURCE_PRICE.get(k, 1.0)
			v *= 6000.0 / d
			if v > best_v:
				best_v = v
				best = {"key": "MiningStation", "body": p}
	if short["alloys"] > 1.0 and int(built.get("Refinery", 0)) == 0 and best_v < 4.0:
		best = {"key": "Refinery", "body": home}
	for p in threat:
		if threat[p] >= 2.0 and int(built.get("DefenseStation", 0)) < 2 and is_instance_valid(p):
			var guarded := false
			for s in have:
				if s.design_name == "DefenseStation" and s.station_body == p:
					guarded = true
			if not guarded:
				return {"key": "DefenseStation", "body": p}
	if best.is_empty() and int(built.get("SensorStation", 0)) == 0:
		best = {"key": "SensorStation", "body": home}
	return best


## A site under an enemy colony's guns is a poor place to build: the
## supply ship parks beside it and the station after it.
func _hostile_near(p: Planet) -> bool:
	for o in world.planets:
		if o.is_colonised() and o.owner_race != race:
			if o.global_position.distance_to(p.global_position) < gun_reach(o) + o.radius + p.radius + 600.0:
				return true
	return false


## How far a colony's guns, lasers and silos reach from its surface.
static func gun_reach(o: Planet) -> float:
	var reach: float = o.defence_range
	for w in o.weapons:
		reach = maxf(reach, w.get("reach", w.get("range", 0.0)))
	return reach


## The supply ship that can best spare the time: empty, not on a convoy,
## nearest the site's way home.
func _idle_hauler(body: Planet) -> Ship:
	var best: Ship = null
	var best_d := INF
	for s in world.ships:
		if s.dead or s.race_id != race or not s.support or s.is_landing():
			continue
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai == null or ai.trader or ai.dropper or not ai.hauler or s.cargo_total() >= 1.0 or s.cargo_max < GameData.CARGO_HOLD["Freighter"]:
			continue
		if convoys.has(s):
			continue
		var d: float = s.global_position.distance_to(body.global_position)
		if d < best_d:
			best_d = d
			best = s
	return best


# ------------------------------------------------------------ convoys

## Is this ship a convoy now: a supply or trade ship of ours on its way
## with a valuable load?
func convoy_worthy(s: Ship) -> bool:
	if s.dead or not s.support or s.race_id != race or s.is_station:
		return false
	var ai: AIPilot = s.get_node_or_null("AI")
	if ai == null or not ai.hauler or ai.dropper or s.is_landing():
		return false
	if not ai.job in ["deliver", "supply", "sell"]:
		return false
	return load_value(s) >= CONVOY_VALUE


static func load_value(s: Ship) -> float:
	var v := 0.0
	for k in s.cargo:
		v += s.cargo[k] * GameData.RESOURCE_PRICE.get(k, 1.0)
	return v


## Where a hauler's run ends: its delivery world or the station it docks at.
static func run_end(s: Ship) -> Node2D:
	var ai: AIPilot = s.get_node_or_null("AI")
	if ai == null:
		return null
	if ai.dock_at != null and is_instance_valid(ai.dock_at):
		return ai.dock_at
	return ai.job_planet


func form_convoys() -> void:
	# convoys that ended: arrived, turned off, lost
	for h in convoys.keys():
		if not is_instance_valid(h) or not convoy_worthy(h):
			end_convoy(h)
	for s in world.ships:
		if convoy_worthy(s) and not convoys.has(s):
			convoys[s] = {"guards": [], "end": run_end(s)}
			s.convoy = true
			_post_escort_job(s)
	# fill the escorts
	for h in convoys:
		var guards: Array = convoys[h]["guards"]
		for g in guards.duplicate():
			if not is_instance_valid(g) or g.dead or g.get_node("AI").escort != h:
				guards.erase(g)
				if is_instance_valid(g):
					g.guarding = null
		var end := run_end(h)
		if end != null and h.global_position.distance_to(end.global_position) < 2000.0:
			continue                 # nearly there
		while guards.size() < CONVOY_ESCORTS:
			var g := _free_escort(h)
			if g == null:
				break
			guards.append(g)
			g.guarding = h
			g.get_node("AI").guard(h)


## A warship on escort duty that is not busy: on patrol, unhurt, nearest.
func _free_escort(h: Ship) -> Ship:
	var best: Ship = null
	var best_d := 9000.0
	for s in world.ships:
		if s.dead or s.race_id != race or s.support or s.is_station or s.is_player or s.guarding != null:
			continue
		if Net.is_active() and s.net_id in Net.seats.values():
			continue
		var ai: AIPilot = s.get_node_or_null("AI")
		# escort duty first; a pilot on attack duty for a rich load
		if ai == null or ai.escort != null or ai.state != AIPilot.State.PATROL:
			continue
		if not ai.escort_duty and not (ai.bombard and load_value(h) >= CONVOY_VALUE * 2.0):
			continue
		if s.hull < s.hull_max * 0.6:
			continue
		var d: float = s.global_position.distance_to(h.global_position) * (1.0 if ai.escort_duty else 2.0)
		if d < best_d:
			best_d = d
			best = s
	return best


## The run is over: the escorts go back to patrol, and an escort job pays
## (the convoy reached the world or station it was bound for), fails (it
## was lost) or is called off (it turned elsewhere).
func end_convoy(h) -> void:
	var c: Dictionary = convoys.get(h, {})
	for g in c.get("guards", []):
		if is_instance_valid(g):
			g.guarding = null
			var ai: AIPilot = g.get_node_or_null("AI")
			if ai != null and ai.escort == h:
				ai.release()
	convoys.erase(h)
	var alive: bool = is_instance_valid(h) and not h.dead
	if alive:
		h.convoy = false
	var end = c.get("end", null)
	var arrived: bool = alive and end != null and is_instance_valid(end) and h.global_position.distance_to(end.global_position) < 2500.0
	for j in jobs.duplicate():
		if j["kind"] != "escort" or j["convoy"] != h:
			continue
		jobs.erase(j)
		var s = j["taken"]
		if s == null or not is_instance_valid(s):
			continue
		if not alive:
			world.notify(s, "The convoy you were escorting was lost. No pay.")
		elif not arrived:
			world.notify(s, "%s turned off its run: the escort job is off." % h.ship_name)
		elif not s.dead and s.global_position.distance_to(h.global_position) < ESCORT_RANGE:
			s.earn(int(j["pay"]))
			s.add_score(maxi(int(j["pay"]) / 40, 1), false)
			world.notify(s, "Escort job done: %s came through. %d credits." % [h.ship_name, j["pay"]])
		else:
			world.notify(s, "%s arrived without you: no pay for the escort job." % h.ship_name)


func _post_escort_job(h: Ship) -> void:
	var end := run_end(h)
	if end == null:
		return
	var j := _job("escort")
	j["convoy"] = h
	j["to"] = end
	j["from"] = h
	j["pay"] = int(load_value(h) * ESCORT_SHARE) + JOB_FLAT
	jobs.append(j)


# ------------------------------------------------------------ the job board

## A job: what it is, where from and to, its pay, and who took it.
func _job(kind: String) -> Dictionary:
	var j := {"id": _next_id, "kind": kind, "race": race, "res": "", "amount": 0.0, "from": null, "to": null,
		"pay": 0, "taken": null, "since": 0.0, "done": 0.0, "convoy": null}
	_next_id += 1
	return j


func job_text(j: Dictionary) -> String:
	match j["kind"]:
		"haul":
			return "Haul %d %s from %s to %s" % [int(j["amount"]), Economy.NAMES[j["res"]], world.port_name(j["from"]), world.port_name(j["to"])]
		"escort":
			var h = j["convoy"]
			var what := []
			if h != null and is_instance_valid(h):
				for k in h.cargo:
					what.append(Economy.NAMES[k])
				return "Escort %s (%s) to %s" % [h.ship_name, ", ".join(what), world.port_name(j["to"])]
	return "?"


## Drop what is no longer wanted, lapse what took too long, then post the
## best runs nobody is on.
func refresh_board() -> void:
	for j in jobs.duplicate():
		if j["kind"] != "haul":
			continue
		var gone: bool = not is_instance_valid(j["from"]) or not is_instance_valid(j["to"])
		if j["taken"] != null:
			if not is_instance_valid(j["taken"]) or j["taken"].dead:
				j["taken"] = null
			elif world.elapsed - float(j["since"]) > JOB_LAPSE:
				world.notify(j["taken"], "Your job lapsed: %s." % job_text(j))
				j["taken"] = null
		if j["taken"] == null and (gone or not _haul_wanted(j)):
			jobs.erase(j)
	var hauls := jobs.filter(func(j): return j["kind"] == "haul").size()
	if hauls >= BOARD_HAULS:
		return
	var busy := _runs_in_hand()
	var cands := []
	var ours := colonies()
	for dst in ours:
		if not dst.has_surface():
			continue
		for k in Economy.RESOURCES:
			var want := Economy.need(dst, k)
			if want < 0.5 or busy.has([dst, k]):
				continue
			for src in ours:
				if src == dst or not src.has_surface() or Economy.makes(dst, k) >= Economy.makes(src, k):
					continue
				var amount := floorf(minf(minf(Economy.surplus(src, k), Economy.wanted(dst, k) + 20.0), 300.0) / 10.0) * 10.0
				if amount < 20.0:
					continue
				var trip: float = 1500.0 + src.global_position.distance_to(dst.global_position)
				cands.append([amount * want * GameData.RESOURCE_PRICE.get(k, 1.0) / trip, src, dst, k, amount])
	cands.sort_custom(func(a, b): return a[0] > b[0])
	for c in cands:
		if hauls >= BOARD_HAULS:
			break
		if busy.has([c[2], c[3]]):
			continue
		var j := _job("haul")
		j["from"] = c[1]
		j["to"] = c[2]
		j["res"] = c[3]
		j["amount"] = c[4]
		j["pay"] = int(c[4] * GameData.RESOURCE_PRICE.get(c[3], 1.0) * JOB_SHARE) + JOB_FLAT
		jobs.append(j)
		busy[[c[2], c[3]]] = true
		hauls += 1


## Is the destination still short of it?
func _haul_wanted(j: Dictionary) -> bool:
	return j["to"] is Planet and j["to"].is_colonised() and j["to"].owner_race == race and Economy.need(j["to"], j["res"]) >= 0.3


## Runs already in hand: the computer supply ships' and the board's.
func _runs_in_hand() -> Dictionary:
	var out := {}
	for s in world.ships:
		if s.dead or s.race_id != race or not s.support:
			continue
		var ai: AIPilot = s.get_node_or_null("AI")
		if ai != null and ai.haul_to != null:
			out[[ai.haul_to, ai.haul_res]] = true
		if ai != null and ai.job == "deliver" and ai.job_planet != null:
			for k in s.cargo:
				out[[ai.job_planet, k]] = true
	for j in jobs:
		if j["kind"] == "haul":
			out[[j["to"], j["res"]]] = true
	return out


## The runs pilots have taken, for the computer supply ships to leave be.
func taken_runs() -> Dictionary:
	var out := {}
	for j in jobs:
		if j["kind"] == "haul" and j["taken"] != null:
			out[[j["to"], j["res"]]] = true
	return out


func job_by_id(id: int) -> Dictionary:
	for j in jobs:
		if j["id"] == id:
			return j
	return {}


func job_of(s: Ship) -> Dictionary:
	for j in jobs:
		if j["taken"] == s:
			return j
	return {}


## Goods unloaded at a port: count them toward the pilot's haul job, and
## pay for the share delivered once the unloading ends.
func delivered(s: Ship, port, moved: Dictionary) -> void:
	var j := job_of(s)
	if j.is_empty() or j["kind"] != "haul" or port != j["to"]:
		return
	var n: float = moved.get(j["res"], 0.0)
	if n <= 0.0:
		return
	var share := minf(n / maxf(float(j["amount"]) - float(j["done"]), 1.0), 1.0)
	j["done"] = float(j["done"]) + n
	var finished := float(j["done"]) >= float(j["amount"]) * 0.8
	var pay := int(j["pay"]) if finished else int(float(j["pay"]) * share)
	if finished:
		jobs.erase(j)
		s.earn(pay)
		s.add_score(maxi(pay / 40, 1), false)
		world.notify(s, "Job done: %d %s delivered to %s. %d credits." % [int(j["done"]), Economy.NAMES[j["res"]], world.port_name(port), pay])
	elif pay > 0:
		j["pay"] = int(j["pay"]) - pay
		s.earn(pay)
		world.notify(s, "Part of the job: %d of %d %s. %d credits; bring the rest for %d more." % [int(j["done"]), int(j["amount"]), Economy.NAMES[j["res"]], pay, j["pay"]])


## The board as clients see it: [id, kind, text, pay, taken by, where to
## fly first ("p:<planet index>" or "s:<ship net id>")].
func pack_board() -> Array:
	var out := []
	for j in jobs:
		var taken: String = j["taken"].pilot_name if j["taken"] != null and is_instance_valid(j["taken"]) else ""
		out.append([j["id"], j["kind"], job_text(j), j["pay"], taken, nav_ref(world, job_nav(j))])
	return out


## The world a pilot taking the job flies to (the nav target is always a
## world): where to load, or where the convoy is bound.
static func job_nav(j: Dictionary) -> Planet:
	var nav = j["from"] if j["kind"] == "haul" else j["to"]
	if nav is Ship and is_instance_valid(nav):
		nav = nav.station_body
	return nav if nav is Planet and is_instance_valid(nav) else null


static func nav_ref(w: Node, n: Node2D) -> String:
	if n is Planet:
		return "p:%d" % w.planets.find(n)
	if n is Ship:
		return "s:%d" % n.net_id
	return ""


static func nav_node(w: Node, ref: String) -> Node2D:
	if ref.begins_with("p:"):
		var i := int(ref.substr(2))
		return w.planets[i] if i >= 0 and i < w.planets.size() else null
	if ref.begins_with("s:"):
		var id := int(ref.substr(2))
		for s in w.ships:
			if s.net_id == id:
				return s
	return null
