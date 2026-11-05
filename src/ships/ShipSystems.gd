## ShipSystems -- the parts of a ship that can be shot out before the hull
## gives: engines, reactor, shield generators, sensors, quantum drive,
## jammer and every weapon mount.  Each sits where it is drawn on the hull
## painting, and a hit damages the systems near the impact, so shooting a
## ship in the stern knocks its engines out and a broadside takes the
## shield domes.  A system below DAMAGED_BELOW works at reduced strength;
## at zero it is offline until damage control brings it back.  A ship with
## its engines or reactor offline is disabled: dead in space, and open to
## boarding (see World.try_board).  Damaged engines push, brake and turn
## less; with the engines out the ship has no helm at all and drifts and
## spins as it was going (Kyle, 2026-10-29).
##
## The balance lives in the constants at the top, since time-to-kill is
## still being tuned: systems are meant to give out well before the hull.
class_name ShipSystems
extends RefCounted

# ------------------------------------------------------------- balance

## A system's hit points as a share of the ship's full hull.
const HP_SHARE := {
	"engines": 0.27, "reactor": 0.27, "shields": 0.25, "sensors": 0.16,
	"qdrv": 0.14, "jammer": 0.12, "weapon": 0.15, "pdc": 0.1,
}
## Damage to a system from a hit right on it, per point of hull damage.
const DAMAGE_MULT := 1.2
## How far a hit's damage reaches through the hull, in ship sizes: the
## falloff is a bell curve of this width around the impact point.
const SPREAD := 0.6
## A hit's damage goes this far into the hull (in ship sizes) before it
## spreads, so the systems deep inside, like the reactor, take hits from
## every side.
const PENETRATION := 0.35
## Every hull hit also shakes everything aboard a little, this share.
const SHOCK := 0.12
## A hit with no position (the Sun's heat) reaches each system this much.
const UNLOCATED := 0.15
## Shields pass this share of what they absorb on to the systems.
const SHIELD_BLEED := 0.0
## A system below this health fraction works at reduced strength...
const DAMAGED_BELOW := 0.6
## ...down to this much just before it fails.
const MIN_EFFICIENCY := 0.35
## Damage control waits this long after the last hit...
const REPAIR_DELAY := 6.0
## ...then mends every system at this share of its maximum per second
## with a full crew (less as the crew is thinned)...
const REPAIR_RATE := 0.025
## ...and restarts an offline one when it is back to this share.
const RESTART_AT := 0.3
## What an offline sensor suite still sees, as a share of its range.
const BLIND_SENSORS := 0.35

## The systems that leave a ship dead in space when they fail.
const VITAL := ["engines", "reactor"]
## Display names and the short tags the HUD uses.
const LABELS := {
	"engines": ["Engines", "ENG"], "reactor": ["Reactor", "PWR"], "shields": ["Shields", "SHD"],
	"sensors": ["Sensors", "SNS"], "qdrv": ["Warp drive", "WD"], "jammer": ["Jammer", "JAM"],
	"pdc": ["Point defense", "PDC"],
}
## Which part overlay on the painting stands for which system.
const ICONS := {"reactor": "reactor", "shields": "shield", "sensors": "sensor", "qdrv": "qdrv", "jammer": "cloak"}

## [{id, kind, label, tag, pts: [local Vector2], hp, max, offline, weapon}]
var list: Array = []
## The ship's numbers with every system whole, set by build().
var base := {}
var quiet := 0.0              # seconds since the last hit
var changed := false          # efficiencies moved since the ship last applied them


## Read the systems off a ship that has just been refitted (stats, weapons
## and art model in place).  Starbases get none: they are not boarded.
func build(s: Node) -> void:
	list.clear()
	quiet = 0.0
	base = {
		"thrust_force": s.thrust_force, "max_speed": s.max_speed, "turn_speed": s.turn_speed, "energy_regen": s.energy_regen,
		"shield_max": s.shield_max, "shield_regen": s.shield_regen, "sensor_range": s.sensor_range,
		"can_jam": s.can_jam, "qdrv": s.qdrv, "has_boost": s.has_boost,
	}
	if s.is_station:
		return
	var m: Dictionary = s.art_model
	var size: float = s.size
	var rect: Rect2 = m.get("rect", Rect2(-size * 1.5, -size * 0.75, size * 3.0, size * 1.5))
	var hull: float = s.hull_max
	var funcs := {}
	for id in s.design["modules"]:
		funcs[GameData.MODULES[id]["func"]] = true
	# engines: where the flames come out
	if funcs.has("eng"):
		var pts := []
		for nz in m.get("nozzles", []):
			pts.append(nz["pos"])
		if pts.is_empty():
			pts.append(Vector2(rect.position.x + size * 0.3, 0))
		_add("engines", pts, hull)
	var want := {"reactor": "power", "shields": "shield", "sensors": "sensor", "qdrv": "qdrv", "jammer": "jammer"}
	var fallback := {
		"reactor": [Vector2(-size * 0.15, 0)],
		"shields": [Vector2(0, -rect.size.y * 0.3), Vector2(0, rect.size.y * 0.3)],
		"sensors": [Vector2(size * 0.4, 0)],
		"qdrv": [Vector2(rect.position.x + size * 0.7, 0)],
		"jammer": [Vector2(-size * 0.55, 0)],
	}
	for kind in want:
		if not funcs.has(want[kind]):
			continue
		var pts := []
		for o in m.get("overlays", []):
			if o.get("icon", "") == ICONS[kind]:
				pts.append(o["pos"])
		if pts.is_empty():
			pts = fallback[kind]
		_add(kind, pts, hull)
	# the point defence turrets, one system for all of them
	if s.pdc.fitted():
		var pts := []
		for mt in s.pdc.mounts:
			pts.append(mt["pos"])
		_add("pdc", pts, hull)
	for i in s.weapons.size():
		var w: Dictionary = s.weapons[i]
		var spot: Vector2 = Vector2.RIGHT.rotated(w["mount_angle"]) * w["mount_radius"]
		var e := _add("weapon", [spot], hull)
		e["id"] = "w%d" % i
		e["weapon"] = i
		e["label"] = w["name"]      # the part's own name, as the shipyard shows it
		e["tag"] = "W%d" % (i + 1)
	changed = true


func _add(kind: String, pts: Array, hull: float) -> Dictionary:
	var mx := maxf(hull * HP_SHARE[kind], 10.0)
	var lab: Array = LABELS.get(kind, [kind.capitalize(), kind.left(3).to_upper()])
	var e := {"id": kind, "kind": kind, "label": lab[0], "tag": lab[1], "pts": pts, "hp": mx, "max": mx, "offline": false, "weapon": -1}
	list.append(e)
	return e


func get_system(id: String) -> Dictionary:
	for e in list:
		if e["id"] == id:
			return e
	return {}


## Health fraction of a system, 1 when the ship does not have it.
func health(id: String) -> float:
	var e := get_system(id)
	return 1.0 if e.is_empty() else e["hp"] / e["max"]


func is_offline(id: String) -> bool:
	var e := get_system(id)
	return not e.is_empty() and e["offline"]


## How well a system works: 1 whole, MIN_EFFICIENCY..1 when damaged, 0 offline.
func efficiency(id: String) -> float:
	var e := get_system(id)
	return 1.0 if e.is_empty() else _eff(e)


static func _eff(e: Dictionary) -> float:
	if e["offline"]:
		return 0.0
	var h: float = e["hp"] / e["max"]
	if h >= DAMAGED_BELOW:
		return 1.0
	return lerpf(MIN_EFFICIENCY, 1.0, h / DAMAGED_BELOW)


func weapon_efficiency(index: int) -> float:
	return efficiency("w%d" % index)


## Dead in space: engines or reactor offline.
func disabled() -> bool:
	for id in VITAL:
		if is_offline(id):
			return true
	return false


## The vital system that will come back first, and in how many seconds
## (damage control included), for the HUD.  {} when nothing vital is out.
func restart_eta(hands: float = 1.0) -> Dictionary:
	var best := {}
	for id in VITAL:
		var e := get_system(id)
		if e.is_empty() or not e["offline"]:
			continue
		var need: float = maxf(RESTART_AT * e["max"] - e["hp"], 0.0)
		var t := maxf(REPAIR_DELAY - quiet, 0.0) + need / maxf(REPAIR_RATE * e["max"] * hands, 0.001)
		if best.is_empty() or t < best["eta"]:
			best = {"id": id, "label": e["label"], "eta": t}
	return best


## Hull damage that got through the shields lands at `local` (ship
## space, Vector2.INF for no position).  `mult` scales it for weapons that
## are hard on systems; `focus` names a system that takes the full hit
## whatever its place (heat seekers go for the engines).  Returns the
## systems that went offline.
func hit(amount: float, local: Vector2, size: float, mult: float = 1.0, focus: String = "") -> Array:
	if list.is_empty() or amount <= 0.0:
		return []
	quiet = 0.0
	var broke := []
	var sigma := maxf(size * SPREAD, 4.0)
	var two_s2 := 2.0 * sigma * sigma
	if local != Vector2.INF and local.length() > 0.001:
		local -= local.normalized() * minf(size * PENETRATION, local.length())
	for e in list:
		var w := SHOCK
		if local == Vector2.INF:
			w = UNLOCATED
		else:
			var d2 := INF
			for p: Vector2 in e["pts"]:
				d2 = minf(d2, local.distance_squared_to(p))
			w = maxf(w, exp(-d2 / two_s2))
		if focus != "" and e["kind"] == focus:
			w = 1.0
		var dmg := amount * DAMAGE_MULT * mult * w
		if dmg <= 0.0 or e["hp"] <= 0.0:
			continue
		var before := _eff(e)
		e["hp"] = maxf(e["hp"] - dmg, 0.0)
		if e["hp"] <= 0.0 and not e["offline"]:
			e["offline"] = true
			broke.append(e)
		if _eff(e) != before:
			changed = true
	return broke


## Damage control.  `boost` is extra repair from repair bays and
## regenerators, in hull points per second; `hands` is the share of the
## crew still aboard, and the repair goes as fast as they can work.
## Returns the systems that came back online.
func tick(delta: float, boost: float = 0.0, hands: float = 1.0) -> Array:
	quiet += delta
	var back := []
	if quiet < REPAIR_DELAY:
		return back
	for e in list:
		if e["hp"] >= e["max"]:
			continue
		var before := _eff(e)
		e["hp"] = minf(e["hp"] + (REPAIR_RATE * e["max"] * hands + boost * 0.5) * delta, e["max"])
		if e["offline"] and e["hp"] >= RESTART_AT * e["max"]:
			e["offline"] = false
			back.append(e)
		if absf(_eff(e) - before) > 0.02:
			changed = true
	return back


## A prize crew's first job: everything that is out is patched back
## online, barely.
func jury_rig() -> void:
	for e in list:
		if e["offline"]:
			e["offline"] = false
			e["hp"] = maxf(e["hp"], RESTART_AT * e["max"])
	quiet = REPAIR_DELAY
	changed = true


func repair_all() -> void:
	for e in list:
		e["hp"] = e["max"]
		e["offline"] = false
	changed = true


func copy_from(o: ShipSystems) -> void:
	for e in list:
		var src := o.get_system(e["id"])
		if not src.is_empty():
			e["hp"] = e["max"] * src["hp"] / src["max"]
			e["offline"] = src["offline"]
	changed = true


## Online: one byte per system, health 0..127 plus 128 when offline.
func pack() -> PackedByteArray:
	var out := PackedByteArray()
	for e in list:
		var g := int(clampf(e["hp"] / e["max"], 0.0, 1.0) * 127.0)
		out.append(g | (128 if e["offline"] else 0))
	return out


func unpack(data: PackedByteArray) -> void:
	for i in mini(data.size(), list.size()):
		var e: Dictionary = list[i]
		e["hp"] = e["max"] * float(data[i] & 127) / 127.0
		e["offline"] = (data[i] & 128) != 0
	changed = true


## Damage levels for the ship painting: icon or turret -> 0..1 (1 offline).
func broken_map() -> Dictionary:
	var out := {}
	for e in list:
		var lvl := 1.0 if e["offline"] else clampf(1.0 - (e["hp"] / e["max"]) / DAMAGED_BELOW, 0.0, 1.0)
		if lvl <= 0.0:
			continue
		if e["weapon"] >= 0:
			out["t%d" % e["weapon"]] = lvl
		elif e["kind"] == "engines":
			out["engines"] = lvl
			out["booster"] = lvl
		else:
			out[ICONS.get(e["kind"], e["kind"])] = lvl
	return out
