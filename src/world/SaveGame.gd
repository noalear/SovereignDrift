## SaveGame -- the campaign's saved game.  A campaign is a Sol war with no
## round clock that runs until one empire holds the whole system, saved
## every couple of minutes, from the pause menu, with F5, and on leaving.
## What is saved is the war: the worlds (owners, people, defences,
## stockpiles, where they are in their orbits), every pilot, supply ship
## and prize with their ships, purses, holds, cargo and troops, who is
## waiting to relaunch, the campaign's clock and record, and on the ground
## each world's buildings, garrison, planetary shield and any battle.  A fight in progress is not: shots in
## flight, subsystem damage and pickups start fresh.  The player's score
## and kills are the pilot's career, so they come from the profile.
class_name SaveGame
extends RefCounted

const PATH := "user://campaign.save"
## The story campaign keeps its own save, apart from the sandbox war's.
const STORY_PATH := "user://story.save"
const VERSION := 1
## Story save slots, as many as the player likes, each with a screenshot
## beside it (<name>.save and <name>.png).
const SLOT_DIR := "user://story_saves/"
const SHOT_SIZE := Vector2i(384, 216)


static func exists(path: String = PATH) -> bool:
	return FileAccess.file_exists(path)


static func erase(path: String = PATH) -> void:
	if exists(path):
		DirAccess.remove_absolute(ProjectSettings.globalize_path(path))


## The menu's line about the saved campaign, from the file alone.
static func summary(path: String = PATH) -> Dictionary:
	var d := read(path)
	if d.is_empty():
		return {}
	var held := 0
	for p in d.get("planets", []):
		if int(p.get("owner", -1)) == int(d.get("race", -1)) and str(p.get("design", "")) != "":
			held += 1
	var story: Dictionary = d.get("story", {})
	return {"elapsed": float(d.get("elapsed", 0.0)), "race": int(d.get("race", 1)), "held": held, "saved_at": float(d.get("saved_at", 0.0)),
		"op": int(story.get("index", -1))}


static func read(path: String = PATH) -> Dictionary:
	if not exists(path):
		return {}
	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
		return {}
	var v: Variant = f.get_var(false)
	if typeof(v) != TYPE_DICTIONARY or int(v.get("version", 0)) != VERSION:
		return {}
	return v


## Write the world to disk (through a temporary file, so a crash mid-write
## never leaves a broken save).  False when the disk refused.
static func write(world: Node, path: String = PATH) -> bool:
	return _store(capture(world), path)


# ------------------------------------------------------------- story slots

## Every story save slot, the newest first: {path, shot (a Texture2D or
## null), info (summary)}.
static func slots() -> Array:
	var out := []
	var dir := DirAccess.open(SLOT_DIR)
	if dir == null:
		return out
	for f in dir.get_files():
		if not f.ends_with(".save"):
			continue
		var path := SLOT_DIR + f
		var info := summary(path)
		if info.is_empty():
			continue
		var shot: Texture2D = null
		var png := path.get_basename() + ".png"
		if FileAccess.file_exists(png):
			var img := Image.load_from_file(ProjectSettings.globalize_path(png))
			if img != null and not img.is_empty():
				shot = ImageTexture.create_from_image(img)
		out.append({"path": path, "shot": shot, "info": info})
	out.sort_custom(func(a, b): return a["info"]["saved_at"] > b["info"]["saved_at"])
	return out


static func has_slots() -> bool:
	var dir := DirAccess.open(SLOT_DIR)
	if dir == null:
		return false
	for f in dir.get_files():
		if f.ends_with(".save"):
			return true
	return false


## A fresh slot's path.
static func new_slot_path() -> String:
	var n := int(Time.get_unix_time_from_system() * 1000.0)
	while FileAccess.file_exists(SLOT_DIR + "save_%d.save" % n):
		n += 1
	return SLOT_DIR + "save_%d.save" % n


## Save the story into a slot (a new one, or over `path`), with `shot` as
## its picture.  During an operation the slot gets the save from the
## operation's start, as Continue would.
static func save_slot(world: Node, path: String, shot: Image = null) -> bool:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(SLOT_DIR))
	var ok := false
	if world.story != null and world.story.in_op():
		var d := read(STORY_PATH)
		if d.is_empty():
			return false
		d["saved_at"] = Time.get_unix_time_from_system()
		ok = _store(d, path)
	else:
		ok = write(world, path)
	if ok and shot != null and not shot.is_empty():
		var img := shot.duplicate() as Image
		img.resize(SHOT_SIZE.x, SHOT_SIZE.y, Image.INTERPOLATE_BILINEAR)
		img.save_png(ProjectSettings.globalize_path(path.get_basename() + ".png"))
	elif ok:
		erase(path.get_basename() + ".png")
	return ok


static func delete_slot(path: String) -> void:
	erase(path)
	erase(path.get_basename() + ".png")


## Load a slot: it becomes the story's own save, which Continue then opens.
static func use_slot(path: String) -> bool:
	var d := read(path)
	return not d.is_empty() and _store(d, STORY_PATH)


static func _store(d: Dictionary, path: String) -> bool:
	var tmp := path + ".tmp"
	var f := FileAccess.open(tmp, FileAccess.WRITE)
	if f == null:
		return false
	f.store_var(d, false)
	f.close()
	var dir := DirAccess.open(path.get_base_dir())
	if dir == null:
		return false
	if dir.file_exists(path.get_file()):
		dir.remove(path.get_file())
	return dir.rename(tmp.get_file(), path.get_file()) == OK


# ------------------------------------------------------------- saving

static func capture(world: Node) -> Dictionary:
	var planets := []
	for p in world.planets:
		planets.append({"name": p.body_name, "angle": p.orbit_angle, "owner": p.owner_race, "design": p.colony_design,
			"pop": p.population, "silenced": p.silenced, "shield": p.shield, "hull": p.hull, "stock": p.stock.duplicate(),
			"ground": Ground.pack(p)})
	var waiting := {}
	for entry in world.respawn_queue:
		if is_instance_valid(entry[0]):
			waiting[entry[0]] = maxf(entry[1] - world.elapsed, 1.0)
	var ships := []
	for s in world.ships:
		if s.has_meta("story") and not s.is_prize:
			continue           # an operation's own ships: it brings them again
		ships.append(_ship(world, s, waiting))
	var rr: Round = world.round_rules
	return {
		"version": VERSION,
		"saved_at": Time.get_unix_time_from_system(),
		"race": world.player.race_id if world.player != null else Settings.race_id,
		"elapsed": world.elapsed,
		"income_timer": world.income_timer,
		"planets": planets,
		"ships": ships,
		"round": {"elapsed": rr.elapsed if rr != null else world.elapsed, "captures": rr.captures.duplicate() if rr != null else {},
			"kills": rr.kills.duplicate() if rr != null else {}, "events": rr.events.duplicate(true) if rr != null else []},
		"story": world.story.pack() if world.get("story") != null else {},
	}


static func _ship(world: Node, s: Ship, waiting: Dictionary) -> Dictionary:
	var pos := s.global_position
	var vel := s.velocity
	if s.is_landing() and s.land_body != null:
		# saved in orbit over the world it was on
		var away := (s.global_position - s.land_body.global_position).normalized()
		if away == Vector2.ZERO:
			away = Vector2.RIGHT
		pos = s.land_body.global_position + away * (s.land_body.radius + 220.0)
		vel = s.land_body.velocity
	var ai: AIPilot = s.get_node_or_null("AI")
	var rr: Round = world.round_rules
	return {
		"pilot": s.pilot_name, "ship_name": s.ship_name, "race": s.race_id, "hull_race": s.hull_race,
		"design": s.design.duplicate(true), "is_player": s.is_player, "is_station": s.is_station,
		"is_prize": s.is_prize, "prize_of": s.prize_of.pilot_name if s.prize_of != null and is_instance_valid(s.prize_of) else "",
		"rank": s.rank, "score": s.score, "kills": s.kills, "deaths": s.deaths,
		"round_score": rr.round_score(s) if rr != null else 0, "round_kills": rr.round_kills(s) if rr != null else 0,
		"credits": s.credits, "credits_earned": s.credits_earned, "captures": s.captures, "ships_captured": s.ships_captured,
		"hold": s.hold.duplicate(), "hold_stock": s.hold_stock.duplicate(), "blueprints": s.blueprints.duplicate(),
		"colonists": s.colonists, "colonist_origin": s.colonist_origin.body_name if s.colonist_origin != null else "",
		"cargo": s.cargo.duplicate(), "cargo_origin": s.cargo_origin.body_name if s.cargo_origin is Planet else "", "support": s.support,
		"kit": s.kit, "building": s.building, "build_t": s.build_t,
		"station_body": s.station_body.body_name if s.station_body != null else "", "station_r": s.get_meta("station_r", 0.0),
		"crew": s.crew, "fuel": s.fuel, "hull": s.hull, "shield": s.shield,
		"shield_focus": s.shield_focus, "shields_raised": s.shields_raised, "ablative": s.ablative, "reactive_tiles": s.reactive_tiles,
		"pos": pos, "vel": vel, "rot": s.rotation,
		"dead": s.dead, "respawn_in": waiting.get(s, -1.0),
		"home": ai.home.body_name if ai != null and ai.home != null else "",
		"station_angle": s.get_meta("station_angle", 0.0),
		"founder": s.founder, "orders": s.orders.duplicate(true), "ordered": s.has_meta("ordered"), "trojan": s.has_meta("trojan"),
		"troops": s.troops.duplicate(),
		# a pilot called up to someone's wing, and the order a leader's wing flies by
		"wing_of": ai.escort.pilot_name if ai != null and ai.called and ai.escort != null and is_instance_valid(ai.escort) else "",
		"wing_order": world.wing.order_of(s)["order"] if world.wing.orders.has(s) and world.wing.order_of(s)["order"] in ["form", "cover", "engage"] else "",
	}


# ------------------------------------------------------------ loading

## Put the saved worlds back: orbits, colonies and stockpiles.
static func restore_planets(world: Node, d: Dictionary) -> void:
	for row in d.get("planets", []):
		var p: Planet = world.planet_by_name.get(row["name"], null)
		if p == null:
			continue
		p.orbit_angle = row["angle"]
		if str(row["design"]) == "" or int(row["owner"]) < 0:
			p.clear_colony()
		else:
			p.set_colony(int(row["owner"]), row["design"], float(row["pop"]))
			p.silenced = row["silenced"]
			p.shield = minf(row["shield"], p.shield_max)
			p.hull = minf(row["hull"], p.hull_max)
		p.stock = Economy.empty_stock()
		for k in row["stock"]:
			p.stock[k] = float(row["stock"][k])
		p.clear_ground()
		if row.has("ground"):
			Ground.unpack(p, row["ground"])
		elif p.is_colonised():
			# a campaign saved before the ground war: the scenario's garrisons
			Ground.seed_planet(p, GameData.STARBASES.get(p.owner_race, "") == p.body_name)
	for p in world.planets:
		p.update_orbit(0.0)
		p.sync_body()
	world.income_timer = d.get("income_timer", 60.0)


## Bring every pilot, starbase and prize back as they were.
static func restore_ships(world: Node, d: Dictionary) -> void:
	world.elapsed = float(d.get("elapsed", 0.0))
	var by_pilot := {}
	var prizes := []
	var wings := []
	for row in d.get("ships", []):
		var s: Ship
		if row["is_player"]:
			world._spawn_player()
			s = world.player
			# the campaign is fought for the empire it was started with,
			# even if the profile has changed sides since
			s.race_id = int(row["race"])
			s.hull_race = int(row["hull_race"])
			s.refit(row["design"])
			s.ship_name = row["ship_name"]
		elif row["is_station"] and row["design"]["name"] != "Starbase":
			# a station built from a kit, or its frame
			s = world._make_ship(row["pilot"], int(row["race"]), row["design"]["name"], row["ship_name"], false, int(row["hull_race"]))
			s.station_body = world.planet_by_name.get(row.get("station_body", ""), null)
			s.building = row.get("building", false)
			s.build_t = float(row.get("build_t", 0.0))
			s.refit(row["design"])
			s.set_meta("station_angle", row["station_angle"])
			if float(row.get("station_r", 0.0)) > 0.0:
				s.set_meta("station_r", float(row["station_r"]))
			if row.get("trojan", false):
				s.set_meta("trojan", true)
			if not s.building:
				world._arm_station(s)
		else:
			s = world._make_ship(row["pilot"], int(row["race"]), row["design"]["name"], row["ship_name"], false, int(row["hull_race"]))
			s.refit(row["design"])
			s.rank = int(row["rank"])
			s.score = int(row["score"])
			s.kills = int(row["kills"])
			s.support = row.get("support", false)
			var ai := AIPilot.new()
			ai.name = "AI"
			s.add_child(ai)
			var home: Planet = world.planet_by_name.get(row["home"], null)
			if home == null:
				home = world._home_planet(s.race_id)
			ai.setup(s, world, home, 14 if row["is_station"] else (3 if row["is_prize"] else s.rank))
			if s.support:
				if s.design_name == "Trader":
					ai.make_trader()
				elif s.design_name == "DropShip":
					ai.make_dropper()
				else:
					ai.make_hauler()
			if row["is_station"]:
				s.set_meta("station_angle", row["station_angle"])
		by_pilot[row["pilot"]] = s
		s.founder = row.get("founder", "")
		s.orders = row.get("orders", []).duplicate(true)
		if row.get("ordered", false):
			s.set_meta("ordered", true)
		s.deaths = int(row["deaths"])
		s.credits = int(row["credits"])
		s.credits_earned = int(row["credits_earned"])
		s.captures = int(row["captures"])
		s.ships_captured = int(row["ships_captured"])
		s.hold = row["hold"].duplicate()
		s.hold_stock = row.get("hold_stock", {}).duplicate()
		s.blueprints = row["blueprints"].duplicate()
		s.colonists = minf(float(row["colonists"]), s.cabin)
		s.colonist_origin = world.planet_by_name.get(row["colonist_origin"], null)
		s.cargo = {}
		for k in row.get("cargo", {}):
			s.cargo[k] = float(row["cargo"][k])
		s._trim_cargo()
		s.troops = {}
		for u in row.get("troops", {}):
			s.troops[u] = float(row["troops"][u])
		s._trim_troops()
		s.cargo_origin = world.planet_by_name.get(row.get("cargo_origin", ""), null)
		if s.cargo_max >= GameData.CARGO_HOLD["Freighter"] and GameData.STATIONS.has(row.get("kit", "")):
			s.kit = row["kit"]
		s.crew = minf(float(row["crew"]), float(s.crew_max))
		s.fuel = minf(float(row["fuel"]), s.fuel_max)
		s.hull = clampf(float(row["hull"]), 1.0, s.hull_max)
		s.set_shield_focus(int(row.get("shield_focus", 0)))
		s.shield_share = GameData.SHIELD_FOCUS[s.shield_focus]
		s.shields_raised = bool(row.get("shields_raised", true))
		s.shield = minf(float(row["shield"]), s.shield_max)
		s.ablative = minf(float(row.get("ablative", s.ablative_max)), s.ablative_max)
		s.reactive_tiles = mini(int(row.get("reactive_tiles", s.reactive_max)), s.reactive_max)
		s.set_meta("saved_round", [int(row["round_score"]), int(row["round_kills"])])
		_place(s, row["pos"], row["vel"], row["rot"])
		if row["is_prize"]:
			s.is_prize = true
			prizes.append([s, row["prize_of"]])
		if row.get("wing_of", "") != "":
			wings.append([s, row["wing_of"]])
		if row.get("wing_order", "") != "":
			world.wing.order_of(s)["order"] = row["wing_order"]
		if row["dead"]:
			if row["is_player"]:
				world._place_at_home(s)        # the player starts the session flying
			else:
				world._bench(s, maxf(float(row["respawn_in"]), 2.0))
	for e in prizes:
		var owner: Ship = by_pilot.get(e[1], null)
		e[0].prize_of = owner
		var ai: AIPilot = e[0].get_node_or_null("AI")
		if ai != null and owner != null:
			ai.escort = owner
	for e in wings:
		var leader: Ship = by_pilot.get(e[1], null)
		var ai: AIPilot = e[0].get_node_or_null("AI")
		if ai != null and leader != null:
			ai.join(leader, true)
	if world.player == null:
		# a save from before the player had a ship: start one at home
		world._spawn_player()
		world._place_at_home(world.player)
	# a campaign saved before the supply ships: they join it now
	for race_id in GameData.SUPPLY_SHIPS:
		if not world.ships.any(func(x): return x.support and x.race_id == race_id):
			for r in GameData.SUPPLY_SHIPS[race_id]:
				world._spawn_hauler(race_id, r[0], r[1])
	# and one saved before the markets: the trade ships and the free port
	for race_id in GameData.TRADE_SHIPS:
		if not world.ships.any(func(x): return x.support and x.design_name == "Trader" and x.race_id == race_id):
			world._spawn_trader(race_id, GameData.TRADE_SHIPS[race_id][0], GameData.TRADE_SHIPS[race_id][1])
	# and one saved before the ground war: the drop ships
	for race_id in GameData.DROP_SHIPS:
		if not world.ships.any(func(x): return x.support and x.design_name == "DropShip" and x.race_id == race_id):
			world._spawn_dropper(race_id, GameData.DROP_SHIPS[race_id][0], GameData.DROP_SHIPS[race_id][1])
	world.spawn_free_port()
	world._keep_station()


static func _place(s: Ship, pos: Vector2, vel: Vector2, rot: float) -> void:
	s.global_position = pos
	s.velocity = vel
	s.rotation = rot
	s.reset_physics_interpolation()
	if s.body != null:
		s.body.pos = pos
		s.body.vel = vel
		s.body.angle = rot


## The campaign's record: its clock, captures, kills and timeline, and
## each pilot's campaign score so the scoreboard counts the whole war.
static func restore_round(world: Node, d: Dictionary) -> void:
	var rr: Round = world.round_rules
	if rr == null:
		return
	var r: Dictionary = d.get("round", {})
	rr.elapsed = float(r.get("elapsed", world.elapsed))
	for k in r.get("captures", {}):
		rr.captures[int(k)] = int(r["captures"][k])
	for k in r.get("kills", {}):
		rr.kills[int(k)] = int(r["kills"][k])
	rr.events = r.get("events", []).duplicate(true)
	if rr.elapsed >= 90.0:
		rr.phase = "war"               # no second "the system is at war"
	for s in world.ships:
		var saved: Array = s.get_meta("saved_round", [0, 0])
		rr.baseline[s] = [s.score - saved[0], s.kills - saved[1]]
