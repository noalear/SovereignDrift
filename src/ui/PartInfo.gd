## PartInfo -- the shipyard's mouse-over descriptions (Kyle, 2026-11-02:
## "explain what it does, how it works, limitations, and what affects its
## values").  Every part, hull, mount and line of the performance panel
## gets a tip: a title, a dim line under it, a row of live numbers, and up
## to four short sections.
##
## The numbers come from GameData, Ship, Sensors and PointDefence as they
## stand, so a balance change shows up here by itself; only the words are
## written by hand.  Builder draws the tips (Builder._draw_tip).
class_name PartInfo
extends RefCounted

const WHAT := "What it does"
const HOW := "How it works"
const LIMITS := "Limits"
const CHANGES := "What changes it"


# ------------------------------------------------------------- helpers

## A tip: {"title", "sub", "facts": [String], "sections": [[heading, text]]}.
static func _tip(title: String, sub: String, facts: Array, sections: Array) -> Dictionary:
	var out := []
	for s in sections:
		if s[1] != "":
			out.append([s[0], Names.fill(s[1])])
	return {"title": title, "sub": sub, "facts": facts, "sections": out}


## The key (or button) bound to an action, in brackets: "[Shift]".  Keys
## are the pilot's own, so tips follow a rebind.
static func key(action: String) -> String:
	var tree := Engine.get_main_loop() as SceneTree
	var kb: Node = tree.root.get_node_or_null("/root/Keybinds") if tree != null else null
	if kb == null or not kb.has_method("short_text"):
		return ""
	var t: String = kb.short_text(action)
	return "" if t == "?" or t == "unbound" else "[%s]" % t


## "press [Shift]" or "press its key" when the action is unbound.
static func _press(action: String) -> String:
	var k := key(action)
	return "press %s" % k if k != "" else "press its key"


static func _pct(v: float) -> String:
	return "%d%%" % int(round(v * 100.0))


## Which empire sells a part, and how the others get it.
static func _where(id: String) -> String:
	var r := GameData.part_race(id)
	if r < 0:
		return "Every empire's yard sells it."
	var adj: String = GameData.race(r)["adj"]
	return "Only %s yards sell it; other pilots need its blueprint, salvaged from %s wrecks." % [adj, adj]


## The coats that stop a fair share of a damage type, best first.
static func _resisted_by(dtype: String) -> String:
	var best := []
	for id in GameData.ARMOR:
		var r: float = GameData.ARMOR[id]["resist"].get(dtype, 0.0)
		if r >= 0.3:
			best.append([r, GameData.coat_label(id)])
	best.sort_custom(func(a, b): return a[0] > b[0])
	var seen := {}
	var parts := []
	for b in best:
		if seen.has(b[1]):
			continue
		seen[b[1]] = true
		parts.append("%s %s" % [b[1], _pct(b[0])])
	return ", ".join(parts.slice(0, 3))


## How a damage type lands on shields and hull, and which coats stop it.
static func _dtype_text(dtype: String) -> String:
	var s := GameData.damage_mult(dtype, "shield")
	var h := GameData.damage_mult(dtype, "hull")
	var t := "%s damage: x%s against shields, x%s against the hull." % [dtype.capitalize(), _x(s), _x(h)]
	var by := _resisted_by(dtype)
	t += (" Armor that stops most of it: %s." % by) if by != "" else " No armor coat resists it."
	return t


static func _x(v: float) -> String:
	return ("%.2f" % v).trim_suffix("0").trim_suffix("0").trim_suffix(".") if not is_equal_approx(v, roundf(v)) else "%d" % int(v)


## Mass and volume, and power if it draws any, for the facts row.
static func _build_facts(id: String) -> Array:
	var m: Dictionary = GameData.MODULES[id]
	var out := []
	if GameData.draws_power(m):
		out.append("draws %d power" % int(round(-m["power"] * (0.25 if not GameData.is_bay_part(id) else (0.05 if m["func"] == "qdrv" else 0.35)))))
	out.append("mass %.1f t" % (m["mass"] / 1000.0))
	out.append("vol %d" % m["vol"])
	if GameData.price(id) > 0:
		out.append("%d cr" % GameData.price(id))
	return out


## The sub line: type, size class or bay part, and its empire.
static func _sub(id: String) -> String:
	var m: Dictionary = GameData.MODULES[id]
	var kind := ""
	if GameData.is_bay_part(id):
		kind = "Internal part"
	else:
		kind = "%s weapon" % GameData.class_name_of(GameData.weapon_class(id)).capitalize()
	var r := GameData.part_race(id)
	if r >= 0:
		kind += "  ·  %s" % GameData.race(r)["adj"]
	if GameData.tier(id) > 0 and m["func"] != "armor":
		kind += "  ·  mark %d" % GameData.tier(id)
	return kind


## The yard hulls with a mount of size class `cls` or bigger, by name.
static func _hulls_with(cls: int) -> String:
	var out := []
	for h in GameData.yard_hulls():
		for hp in GameData.hull_hardpoints(h):
			if int(hp[5]) >= cls:
				out.append(GameData.hull_label(h))
				break
	return ", ".join(out) if not out.is_empty() else "no hull the yards sell"


## What an overloaded reactor does to a shot's energy cost.
static func _overload_mult(st: Dictionary) -> float:
	return 1.0 + (float(st.get("overload", 1.0)) - 1.0) * 0.5


# ----------------------------------------------------------------- parts

## The tip for a part.  `st` is the ship's stats as fitted now (for what
## an overloaded reactor does to its costs); `race_id` names its rounds.
static func part(id: String, st: Dictionary = {}, race_id: int = 1) -> Dictionary:
	if not GameData.MODULES.has(id):
		return {}
	var m: Dictionary = GameData.MODULES[id]
	var t: Dictionary
	match m["func"]:
		"tube": t = _tube(id, st, race_id)
		"beam": t = _beam(id, st)
		"power": t = _power(id)
		"eng": t = _engine(id)
		"qdrv": t = _qdrv(id)
		"shield": t = _shield(id)
		"armor": t = _armor(id)
		"sensor": t = _sensor(id)
		"jammer": t = _jammer(id)
		"heatsink": t = _heatsink(id)
		"boost": t = _booster(id)
		"brake": t = _brake(id)
		"regen": t = _regen(id)
		"repair": t = _repair(id)
		"ram": t = _ram(id)
		"pdc": t = _pdc(id)
		_: t = _tip(m["name"], "Station part", [], [[WHAT, "Part of a station's fit; ships don't carry it."]])
	t["facts"] = t["facts"] + _build_facts(id)
	if GameData.part_race(id) >= 0:
		t["sections"].append(["Where to get it", _where(id)])
	return t


static func _tube(id: String, st: Dictionary, race_id: int) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var p: Array = m["p"]
	var mid: String = p[3]
	var md: Dictionary = GameData.MISSILES[mid]
	var dmg: float = md["warhead"] * GameData.DAMAGE_SCALE
	var rate: float = GameData.TUBE_COOLDOWN.get(mid, 0.3)
	var cost: float = clampf(float(p[1]) * 0.025, 20.0, 350.0)
	var cost_now := cost * _overload_mult(st)
	var reach: float = md["fire_v"] * GameData.LIGHT_SPEED * md["life"] * 0.8
	var ammo: int = p[2]
	var round_name := GameData.missile_display_name(race_id, mid).to_lower()
	var facts := ["damage %d" % int(round(dmg))]
	if mid != "002":
		facts.append("%.2f s a shot" % rate if rate < 0.2 else "%.1f s a shot" % rate)
		facts.append("%d dps" % int(round(dmg / rate)))
	facts.append("%d energy a shot" % int(round(cost)))
	if mid != "002":
		facts.append("reach %d" % int(round(reach)))
	facts.append("%d rounds" % ammo if ammo < 400 else "unlimited rounds")
	var over := ""
	if cost_now > cost + 0.5:
		over = " Right now your reactor is overloaded, so each shot costs %d instead of %d." % [int(round(cost_now)), int(round(cost))]
	var common_changes := "An overloaded reactor makes every shot cost up to half again as much energy." + over + " A damaged mount reloads slower. What reaches the hull depends on the target's shields and armor coat."
	var ammo_text := ""
	if ammo < 400:
		ammo_text = " Its magazine holds %d; rearm at a friendly base or colony while you refuel." % ammo
	var what := ""
	var how := ""
	var limits := ""
	var changes := common_changes
	match mid:
		"001":
			what = "Fires solid slugs at very high speed: the long-reach gun, hitting out to about %d px." % int(round(reach))
			how = "Each %s flies straight and unguided, faster than any ship, and knocks the target with its momentum. It fires with its weapon group; on a turret it swings to the cursor, or leads a locked target." % round_name
			limits = "At long range a target that changes course has time to dodge. Every shot is a big gulp of energy, so a small ship's battery runs dry in a long exchange. Plate and reactive armor stop much of it." + ammo_text
		"010":
			what = "Sprays small rounds at about %d a second: close-in hosing that wears down shields and hull fast." % int(round(1.0 / rate))
			how = "Each %s scatters a few degrees off the line, so the stream covers a moving target at short range. Its rounds can knock down missiles, torpedoes and drones they cross." % round_name
			limits = "Reach is short (about %d px) and the spread wastes rounds further out. The energy adds up to about %d a second while it fires. Heavy for its size, and kinetic: plate and reactive armor blunt it." % [int(round(reach)), int(round(cost / rate))]
		"018":
			what = "The heavy gun: slow, hard-hitting shells that punch through armor."
			how = "A %s ignores %s of the target's armor resistance, so it hurts even the best-coated hulls. Each shot kicks the ship back at the mount, so a cannon off to one side twists the ship." % [round_name, _pct(md.get("pierce", 0.0))]
			limits = "Needs a heavy mount (%s). Slow reload, short reach (about %d px) and the shells are unguided, so it wants big or slow targets. Each shot costs %d energy.%s" % [_hulls_with(2), int(round(reach)), int(round(cost)), ammo_text]
		"020":
			what = "Fires ion bolts that strip shields, drain the target's battery and knock out its systems."
			how = "Each hit drains %d energy from the target, knocks its jammer out for 3 s, and hits its systems %sx harder than a normal shot. It aims itself: at your locked target, else at the cursor, from any mount and all round." % [int(md.get("drain", 0)), _x(md.get("sys", 1.0))]
			limits = "Ion does little to bare hull, so it opens a ship for other guns rather than killing it. Short reach (about %d px), and the bolts can be dodged." % int(round(reach))
		"002":
			what = "Drops proximity mines behind the ship. Each one goes off when an enemy hull comes within %d px of it." % int(md.get("prox", 0))
			how = "A mine drifts to a stop, arms after 1.5 s and waits %d s. Its blast hits everything within %d px, more at the centre. Lay them across a chaser's path or around something you defend." % [int(md["life"]), int(md.get("blast", 0))]
			limits = "Only ever drops astern; the mount can't turn. Point defense and gun fire can shoot mines down (two hits each). Each mine costs %d energy." % int(round(cost)) + ammo_text
		"004":
			what = "Launches torpedoes: slow, heavy warheads that home on your target and burst close to it."
			how = "A torpedo steers toward your locked target, or whatever its own head picks up (%d px x radar return / 100), and goes off within %d px of a hull, with a %d px blast." % [int(md["seek_range"] * 2), int(md.get("prox", 0)), int(md.get("blast", 0))]
			limits = "Turns slowly, so a nimble ship can sidestep one; best against big, slow targets. It burns out after %.1f s (about %d px). Point defense and guns can shoot it down, though it takes two hits." % [md["life"], int(round(reach))] + ammo_text
			changes = "A bigger, louder target (radar return) is found from further off. " + common_changes
		"008":
			what = "Fires agile guided missiles that chase your target."
			how = "A missile homes while you hold a radar track on the target, or once its own head sees it (%d px x the target's radar return / 100). It turns hard and hits on impact. Fires with group 3 by default." % int(md["seek_range"] * 2)
			limits = "Won't fit a Scout. Stealthy ships (low radar return, a jammer on you) are hard to lock. Point defense and gun fire can shoot missiles down. Flies about %.1f s." % md["life"] + ammo_text
			changes = "Your sensors' radar lock and the target's radar return decide how far it homes. " + common_changes
		"009":
			what = "Fires heat-seeking missiles that home on engine exhaust, so their hits land in the engines."
			how = "A seeker homes while you hold an infrared track, or once its own head feels the target's heat (%d px x heat / 100). Hits from behind go into the engines and can leave the target adrift." % int(md["seek_range"] * 2)
			limits = "A target coasting with its engines off, behind a heat sink, or in the Sun's glare is hard to find. Point defense and gun fire can shoot seekers down." + ammo_text
			changes = "The target's heat: full burn and boosting make it easy prey. An infrared sensor lets it lock from further off. " + common_changes
		"021":
			what = "Launches drones: small guided craft that hunt enemy ships on their own."
			how = "A drone looks for an enemy within %d px almost all round, turns tightly and circles back if it misses. It flies for %d s, so launch close and let it work." % [int(md["seek_range"] * 2), int(md["life"])]
			limits = "Each drone hits lightly (%d); the damage comes from numbers. Point defense swats them, and gun fire can too." % int(round(dmg)) + ammo_text
		_:
			what = "Fires %ss." % round_name
	var sections := [[WHAT, what], [HOW, how], [LIMITS, limits], ["Damage type", _dtype_text(md.get("dtype", "kinetic"))], [CHANGES, changes]]
	return _tip(m["name"], _sub(id), facts, sections)


static func _beam(id: String, st: Dictionary) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var dps := GameData.beam_dps(id)
	var reach := GameData.beam_range(id)
	var cost := 90.0
	var cost_now := cost * _overload_mult(st)
	var facts := ["%d dps close in" % int(round(dps)), "%d at full reach" % int(round(dps * GameData.BEAM_FALLOFF)), "reach %d" % int(round(reach)), "%d energy a second" % int(cost), "no ammo"]
	var over := ""
	if cost_now > cost + 0.5:
		over = " Right now your reactor is overloaded, so it costs %d a second." % int(round(cost_now))
	return _tip(m["name"], _sub(id), facts, [
		[WHAT, "A laser: a beam that burns whatever it touches for as long as you hold the trigger. It never misses inside its reach."],
		[HOW, "It locks onto your targeted ship by itself while it is in reach and in the mount's arc, and follows the cursor otherwise. Damage falls to %s at full reach. Beams burn plating, not people: they kill a tenth of the crew other hits do." % _pct(GameData.BEAM_FALLOFF)],
		[LIMITS, "Short reach: you have to close in. It costs energy every second it fires, so it is battery-hungry, and it is loud on EM receivers while it burns. A {drengin} Deflector shield bounces the beam away, and a Reflective coat stops most of it."],
		["Damage type", _dtype_text("beam")],
		[CHANGES, "Distance to the target (full damage up close). An overloaded reactor raises the energy cost." + over + " A damaged mount burns weaker."],
	])


static func _power(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var pw: float = m["power"]
	var regen := pw * 0.45 * 0.1 * GameData.ENERGY_REGEN_SCALE
	return _tip(m["name"], _sub(id), ["+%d power" % int(pw), "battery %d" % int(pw * 5.0), "recharge %d/s" % int(round(regen))], [
		[WHAT, "The reactor: it makes the power every system runs on and fills the battery that weapons, shields, the warp drive and the jammer spend."],
		[HOW, "A bigger plant makes more power, holds a bigger battery (5 energy per point of power) and refills it faster. Every part that runs on power draws a share of it all the time: bay parts 35% of their rating, weapons 25%, a warp drive 5%. The Reactor bar shows made against drawn."],
		[LIMITS, "One per ship. Draw more than it makes and the ship is overloaded: the battery refills slower and every shot costs more. A reactor shot out drains the battery %s a second and leaves the ship dead in space until damage control restarts it. The big plants are heavy." % _pct(Ship.REACTOR_OUT_DRAIN)],
		[CHANGES, "Only the mark. In flight, reactor damage slows the recharge, silent running halves it, and lowering shields frees their draw on an overloaded ship."],
	])


static func _engine(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var thrust: float = m["p"][0]
	return _tip(m["name"], _sub(id), ["thrust %d" % int(thrust), "fuel tank +%d" % int(thrust * Ship.FUEL_PER_ENGINE), "runs on fuel"], [
		[WHAT, "A main engine: it pushes the ship. More thrust means quicker acceleration and harder braking, and every engine brings its own fuel tank."],
		[HOW, "Engines stack: their thrust adds up, and acceleration is total thrust against the ship's mass. They burn fuel, not energy: a full tank lasts about %d s at full burn. Top speed comes from the hull and its mass, not from the engines." % int(Ship.FUEL_PER_ENGINE / Ship.BURN_PER_ENGINE)],
		[LIMITS, "Engines sit at the stern: hits from behind damage them, and with them out the ship has no helm and drifts. With the tank empty the ship coasts until a friendly base or colony refuels it (the reactor trickles a little back while the engines rest). A hard burn shows up on infrared, more on heavy hulls."],
		[CHANGES, "The ship's mass (parts, and in flight fuel and cargo) and engine damage. Boosters multiply the push while boosting."],
	])


static func _qdrv(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var ws: float = float(m["p"][0]) * GameData.LIGHT_SPEED * 1.25
	return _tip(m["name"], _sub(id), ["warp speed %d" % int(ws), "200 energy to start", "160 energy a second"], [
		[WHAT, "The quantum drive: crosses the system at many times normal speed."],
		[HOW, "%s: the drive charges for about a second, then the ship flies straight ahead at warp speed until you press it again, turn, brake or the battery runs low. Higher marks fly faster. Autopilot uses it on long legs." % _press("warp").capitalize()],
		[LIMITS, "Needs 200 energy to start and drains 160 a second. It drops out short of planets and the Sun's heat. Charging is loud on EM, and every jump sends out a time ripple that temporal sensors pick up far away. No guns fire in warp. Engines or the drive shot out: no warp."],
		[CHANGES, "Only the mark. Warp drive III draws far more power than the others."],
	])


static func _shield(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var fam_id := GameData.shield_family(id)
	var fam: Dictionary = GameData.SHIELD_FAMILIES[fam_id]
	var cap: float = m["p"][0]
	var facts := ["holds %d" % int(cap), "refills %s/s" % _pct(fam["rate"]), "waits %.1f s after a hit" % fam["pause"]]
	var what := "A shield generator: a bubble that soaks up hits before they reach the hull."
	var family := ""
	match fam_id:
		"standard":
			family = "Standard shield: the all-rounder, holds a fair charge and recovers at a fair pace."
		"barrier":
			family = "Barrier: holds far more than a standard shield of its size, but waits %.1f s after a hit (%.0f s once emptied) and refills slowly. Built to outlast a big exchange, not to bounce back." % [fam["pause"], fam["zero_pause"]]
		"flicker":
			family = "Flicker: holds less but recovers almost at once (%.1f s after a hit) and refills fast. On %s of hits, %s of the damage leaks through to the hull." % [fam["pause"], _pct(fam["leak_chance"]), _pct(fam["leak"])]
		"deflector":
			family = "Deflector: turns gun rounds and laser beams off the bubble's edge instead of absorbing them; a deflected shot is yours from then on and can hit its own side. Each deflection still costs %s of what the hit would have taken. Holds less and draws far more power than the others." % _pct(fam.get("deflect", 1.0))
	return _tip(m["name"], _sub(id), facts, [
		[WHAT, what + " " + family],
		[HOW, "The charge splits between a front and a rear half (%s cycles balanced, forward and aft). A hit pauses the recharge of the half it lands on; then it refills %s of its size a second, at 1.5 energy per point. Generators stack: their charge adds, and the biggest one sets how the shield behaves." % [key("shield_focus") if key("shield_focus") != "" else "shield focus", _pct(fam["rate"])]],
		[LIMITS, "Only one family per ship: fitting another family swaps them. Raised shields show on EM receivers (louder while recharging); %s lowers them to run quiet, and raising them takes %d s and starts at %s. Generators can be shot out." % [key("shields_toggle") if key("shields_toggle") != "" else "the shields key", int(GameData.SHIELD_BOOT), _pct(GameData.SHIELD_BOOT_CHARGE)]],
		[CHANGES, "Damage types hit shields differently: ion x%s, plasma x%s, lasers x%s, explosives x%s. Damaged generators hold less; an empty battery stops the recharge; silent running holds it." % [_x(GameData.damage_mult("ion", "shield")), _x(GameData.damage_mult("plasma", "shield")), _x(GameData.damage_mult("beam", "shield")), _x(GameData.damage_mult("explosive", "shield"))]],
	])


static func _armor(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var a: Dictionary = GameData.ARMOR[id]
	var res: Dictionary = a["resist"]
	var facts := []
	for t in ["beam", "plasma", "kinetic", "explosive"]:
		facts.append("%s %s" % [{"beam": "laser", "plasma": "plasma", "kinetic": "kinetic", "explosive": "explosive"}[t], _pct(res[t])])
	if m["hp"] > 0:
		facts.append("hull +%d" % m["hp"])
	if not is_equal_approx(a["radar"], 1.0):
		facts.append("radar x%s" % _x(a["radar"]))
	var what := ""
	var how := ""
	var limits := ""
	match a["coat"]:
		"plate":
			what = "Plain armor plate: stops a share of every kinetic, explosive and plasma hit, and adds the most hull of any coat."
			how = "Each hit on the hull loses the coat's share for its damage type. Higher marks stop more and add more hull, at a lot more mass."
			limits = "Does nothing against lasers or ion. Heavy: it slows acceleration and turning, and burns hotter at full throttle."
		"reflective":
			what = "A mirror finish that turns most of a laser's burn, and some plasma."
			how = "Laser hits lose %s before they reach the hull. Light, so it barely slows the ship." % _pct(res["beam"])
			limits = "Useless against guns, missiles and mines. Once the hull is below %s the scorched finish works half as well. Shiny: the ship's radar return goes up %s." % [_pct(GameData.REFLECTIVE_SCORCHED), _pct(a["radar"] - 1.0)]
		"ablative":
			what = "A layer that burns away as it absorbs hits: strong against plasma and lasers, decent against everything else."
			how = "The layer is worth %s of the ship's hull. Each hit loses the coat's share, and what it stops comes off the layer, until the layer is gone." % _pct(GameData.ABLATIVE_LAYER)
			limits = "Once the layer is spent it stops nothing until a friendly base or colony restocks it (a repair bay rebuilds it slowly once the hull is whole). Adds no hull of its own."
		"reactive":
			what = "Explosive tiles that blow incoming heavy hits apart: the best coat against cannon shells, torpedoes and mines."
			how = "A single hit of %d or more to the hull fires a tile and loses %s of its damage. A hull carries 3 tiles (Scout) to 10 (Dreadnought); one comes back every %d s out of combat." % [int(GameData.REACTIVE_TRIGGER), _pct(res["kinetic"]), int(GameData.REACTIVE_REARM)]
			limits = "Smaller hits pass straight through, so rapid fire and lasers ignore it. Each tile fired flashes on infrared. Heavy."
		"shroud":
			what = "A radar-absorbing coating: the stealth coat. It shrinks the ship on radar, optics and lidar to %s." % _pct(a["radar"])
			how = "Enemy radar sees the ship only at %s of its usual range, so you can get close before anyone locks on. It also stops a little of every hit." % _pct(a["radar"])
			limits = "Weak protection (%s against everything). It does nothing for heat or emissions: pair it with a heat sink, silent running and passive sensors for a quiet ship." % _pct(res["kinetic"])
		"bulkheads":
			what = "Internal bulkheads: they protect the systems and crew more than the hull."
			how = "Systems take %s of the damage that reaches them, crew losses are halved, and your crew fights boarders %s harder. It also stops a little of each hit." % [_pct(GameData.BULKHEAD_SYSTEMS), _pct(GameData.BULKHEAD_BOARD - 1.0)]
			limits = "Barely slows the hull damage itself. Heavy, and it hardly shows outside."
	return _tip(m["name"], "Armor coat  ·  internal part" + ("  ·  %s" % GameData.race(GameData.part_race(id))["adj"] if GameData.part_race(id) >= 0 else ""), facts, [
		[WHAT, what],
		[HOW, how],
		[LIMITS, "One coat per ship. " + limits],
		[CHANGES, "The damage type of each hit (the numbers above). A cannon's piercing shells ignore half of any coat. Armor mass is thermal mass: the ship's heat rises and falls more slowly (about %.1f s more per tonne of coat), and heavy coats soften the Sun's heat too." % (1000.0 / 2000.0)],
	])


static func _sensor(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var fam_id: String = m["fam"]
	var fam: Dictionary = GameData.SENSOR_FAMILIES[fam_id]
	var rated: float = m["p"][0]
	var facts := ["rated %d" % int(rated), "reads %s" % ("radar return" if fam["channel"] == "radar" else fam["channel"]), "active" if fam["active"] else "passive", "locks" if fam["track"] else "contacts only"]
	var what := ""
	var how := ""
	var limits := ""
	match fam_id:
		"radar":
			what = "Active radar: sweeps all round and locks onto ships by their radar return."
			how = "It sees a ship out to its rating x the target's radar return / 100, gives a fuzzy contact out to %sx that, and holds a lock inside it. Locks aim your guns and guide missiles." % _x(Sensors.CONTACT_REACH)
			limits = "It transmits: EM receivers hear it at about twice its range, and every ship it locks gets a warning. Silent running switches it off. Planets block it."
		"pulse":
			what = "Pulse radar: silent until you ping, then one powerful sweep that locks everything it finds."
			how = "%s: the ping costs %d energy and holds a lock on everything in reach for %d s. It recharges in %d s." % [_press("ping").capitalize(), int(Sensors.PING_COST), int(Sensors.PING_HOLD), int(m["p"][1])]
			limits = "Between pings it sees nothing. Each ping is heard by EM receivers for %d s. Planets block it." % int(Sensors.PING_HEARD)
		"lidar":
			what = "Lidar: a narrow laser beam off the nose that locks at long range."
			how = "It sees only inside a %d° cone ahead (%d° each side), reading radar return like radar, but much further." % [int(round(rad_to_deg(fam["cone"]) * 2.0)), int(round(rad_to_deg(fam["cone"])))]
			limits = "Point the nose at what you want to see. Only ships inside the beam hear it, and a jammer hurts it less than radar. Off in silent running; planets block it."
		"infrared":
			what = "Infrared: a passive heat sensor that locks onto ships by their heat."
			how = "It sees a ship out to its rating x the target's heat / 100. Engines at full burn, boosting, shots and jammers make a ship easy to see; a coasting ship is dim. Seekers home on what it locks."
			limits = "It sends nothing, so it never gives you away. Cold ships (coasting, heat sinks) hide from it, ships in the Sun's glare look half as hot, and planets block it."
		"em":
			what = "EM receiver: listens for what other ships transmit."
			how = "It hears radar, lasers, raised shields and pings out to its rating x their emissions / 100, giving a contact or a bearing. Active radars are heard at about twice their own range."
			limits = "Gives contacts only, never a lock: pair it with a tracking sensor. A silent ship (radar off, shields down) is invisible to it."
		"temporal":
			what = "Temporal sensor: feels the time ripples of warp jumps."
			how = "It picks up a ship charging or flying its warp drive, and the ripple of each jump, out to its rating x the ripple / 100. It sees through planets."
			limits = "Gives contacts only, never a lock, and only ships that are jumping. Sees nothing in a fight where nobody warps."
	return _tip(m["name"], _sub(id), facts, [
		[WHAT, what],
		[HOW, how],
		[LIMITS, "One of each kind counts (the best mark). " + limits],
		[CHANGES, "The target's signature on this sensor's channel, which its build decides (see Radar return, Heat and Emissions). Sensor damage blinds it to %s; an enemy jammer cuts it to %s (lidar %s)." % [_pct(ShipSystems.BLIND_SENSORS), _pct(Sensors.JAM_FACTOR), _pct(Sensors.JAM_FACTOR_LIDAR)]],
	])


static func _jammer(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["reach %d" % int(m["p"][0]), "%d energy a second" % int(Sensors.JAM_DRAIN)], [
		[WHAT, "Blinds the sensors of the ship you have targeted."],
		[HOW, "Lock a target and %s. After %d s its sensors keep only %s of their range (lidar %s) and its data link goes dark, so it can lose its lock on you. Firing doesn't stop it." % [_press("cloak"), int(Sensors.JAM_BITE), _pct(Sensors.JAM_FACTOR), _pct(Sensors.JAM_FACTOR_LIDAR)]],
		[LIMITS, "One target at a time, within %d px and in line of sight, and you need a lock to start. Costs %d energy a second and adds heat. A disruptor hit knocks it out for 3 s, and it can be shot out." % [int(m["p"][0]), int(Sensors.JAM_DRAIN)]],
		[CHANGES, "Fixed. Running low on energy switches it off."],
	])


static func _heatsink(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["holds %d heat-seconds" % int(m["p"][0]), "soaks %s" % _pct(Sensors.SINK_SOAK)], [
		[WHAT, "Soaks up most of the ship's heat, so infrared sensors and seekers have a hard time finding it."],
		[HOW, "While the ship runs hot it stores %s of its heat. Coasting cool, it empties again at %d a second." % [_pct(Sensors.SINK_SOAK), int(Sensors.SINK_DRAIN)]],
		[LIMITS, "When it is full it vents everything in a bright flare that infrared sees for %d s. Shot flashes aren't soaked. One per ship." % int(Sensors.SINK_VENT_TIME)],
		[CHANGES, "How hot you run: full burn, boosting and the jammer fill it fast."],
	])


static func _booster(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	var g := GameData.boost_gain(m["p"])
	return _tip(m["name"], _sub(id), ["top speed x%.2f" % (1.0 + g.x), "push x%.2f" % (1.0 + g.y), "burns x%.1f fuel" % ((1.0 + g.y) * Ship.BOOST_BURN), "runs on fuel"], [
		[WHAT, "An afterburner: hold %s while thrusting to go faster and accelerate harder." % (key("boost") if key("boost") != "" else "boost")],
		[HOW, "While boosting, top speed and push go up by the numbers above. Boosters stack: the best one counts in full, each more at half. Higher marks boost more."],
		[LIMITS, "Thirsty: the extra push burns fuel %sx less efficiently than the engines, so it drains the tank fast (see Boost burn time). Boosting is hot on infrared. Badly damaged engines can't boost." % _x(Ship.BOOST_BURN)],
		[CHANGES, "The mark and how many you carry; your engines and mass set what it multiplies."],
	])


static func _brake(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["brakes x%.1f" % (1.6 / 0.6)], [
		[WHAT, "Retro thrusters: %s stops the ship much faster." % _press("brake")],
		[HOW, "Braking power goes from 0.6 to 1.6 of the engines' push."],
		[LIMITS, "Big and power-hungry for what it does. Damaged engines brake weaker."],
		[CHANGES, "Your engines and mass: braking is a share of their acceleration."],
	])


static func _regen(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["heals %d hull/s" % int(m["p"][0])], [
		[WHAT, "Living hull: the ship heals itself once the shooting stops."],
		[HOW, "4 s after the last hit, it mends %d hull a second, for free. It also speeds damage control on shot-out systems." % int(m["p"][0])],
		[LIMITS, "Does nothing while you are under fire."],
		[CHANGES, "Only the mark."],
	])


static func _repair(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["heals %d hull/s" % int(m["p"][0]), "2 energy per point"], [
		[WHAT, "Repair crews and drones that patch the hull even mid-fight."],
		[HOW, "It mends %d hull a second at any time, paying 2 energy for each point. It also speeds damage control, and rebuilds an ablative layer once the hull is whole." % int(m["p"][0])],
		[LIMITS, "Competes with your guns and shields for energy, and stops when the battery is nearly empty."],
		[CHANGES, "The mark, and how much energy you have to spare."],
	])


static func _ram(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["ram damage x%d" % int(m["p"][0]), "hull +%d" % (int(m["hp"]) + 200)], [
		[WHAT, "An armored prow that turns a collision into a weapon."],
		[HOW, "Hitting a ship nose first deals %dx the crash damage, and what you take from a head-on crash is divided by %d. Faster and heavier hits hurt more." % [int(m["p"][0]), int(m["p"][0])]],
		[LIMITS, "Only counts nose first. Very heavy for a bay part."],
		[CHANGES, "Your speed into the target and both ships' mass."],
	])


static func _pdc(id: String) -> Dictionary:
	var m: Dictionary = GameData.MODULES[id]
	return _tip(m["name"], _sub(id), ["reach %d" % int(PointDefence.REACH), "%d rounds a second" % int(round(1.0 / PointDefence.RATE)), "%d energy a round" % int(PointDefence.SHOT_ENERGY), "%d rounds" % PointDefence.MAGAZINE], [
		[WHAT, "Point defense: a small gatling turret on the hull that shoots down incoming missiles, seekers, torpedoes, drones and mines by itself."],
		[HOW, "It picks whatever will reach you (or a friend nearby) soonest and fires a led stream at it. Each PDC adds a turret; several spread over a salvo before doubling up."],
		[LIMITS, "A salvo bigger than your turrets gets through. Its stream of fire shows on infrared: %s turns them off to hide. Its magazine refills where launchers rearm." % _press("pdc_toggle")],
		[CHANGES, "How many you fit. Torpedoes and mines take two hits each."],
	])


# ----------------------------------------------------------------- hulls

const HULL_TEXT := {
	"Scout": "The smallest warship and every pilot's first: quick, nimble and cheap to lose, with little room inside.",
	"Destroyer": "A step up from the Scout: more mounts and bay room, still quick on its feet.",
	"Cruiser": "The line warship: medium mounts all round and a proper set of bays, slower to turn.",
	"BattleCruiser": "Nearly a battleship's size with a cruiser's speed: a hunter with a huge bay.",
	"Battleship": "Heavy mounts and a deep hull: hits hard and takes a beating, but turns slowly.",
	"Dreadnought": "The biggest hull a pilot can fly: heavy turrets and the most firepower, turning like a station.",
	"Freighter": "The supply ship: hauls resources between worlds and stations, and carries the most colonists. Lightly armed.",
	"Trader": "The trade ship: the biggest cargo hold and a quick hull for a civilian.",
	"DropShip": "Carries ground troops down to take worlds: the most troop bays of any hull.",
	"Raider": "A pirate's fast attack hull.",
}


static func hull(name: String, race_id: int = 1) -> Dictionary:
	if not GameData.SHIPS.has(name):
		return {}
	var d: Dictionary = GameData.SHIPS[name]
	var hps := GameData.hull_hardpoints(name)
	var cls := [0, 0, 0]
	for hp in hps:
		cls[clampi(int(hp[5]), 0, 2)] += 1
	var mounts := []
	for c in [2, 1, 0]:
		if cls[c] > 0:
			mounts.append("%d %s" % [cls[c], GameData.class_name_of(c)])
	var facts := [
		"%s mounts" % (", ".join(mounts) if not mounts.is_empty() else "no"),
		"bays %d" % GameData.HULL_BAYS.get(name, 0),
		"top speed %d" % int(d["speed"] * GameData.LIGHT_SPEED),
		"turns %d°/s" % int(d["turn"]),
		"crew %d" % GameData.HULL_CREW.get(name, 0),
		"cargo %d" % GameData.CARGO_HOLD.get(name, 0),
	]
	if int(GameData.COLONIST_CABIN.get(name, 0)) > 0:
		facts.append("%dM colonists" % GameData.COLONIST_CABIN[name])
	if int(GameData.TROOP_BAYS.get(name, 0)) > 0:
		facts.append("%d troop bays" % GameData.TROOP_BAYS[name])
	var cost := "%d cr" % GameData.HULL_PRICE.get(name, 0) if GameData.HULL_PRICE.get(name, 0) > 0 else "free"
	if GameData.HULL_MATERIALS.has(name):
		var mats := []
		var need: Dictionary = GameData.HULL_MATERIALS[name]
		for k in need:
			mats.append("%d %s" % [need[k], Economy.NAMES[k]])
		cost += " plus %s from the yard's stock" % " and ".join(mats)
	return _tip(GameData.hull_label(name), "Hull  ·  needs %s" % GameData.rank_name(d["rank"]), facts, [
		[WHAT, HULL_TEXT.get(name, "")],
		[HOW, "The hull sets the mounts, the bay volume and the handling: speed and turn above are for its stock fit. A lighter build is up to 12% faster and 25% quicker to turn, a heavier one up to 25% slower and 45% slower to turn. Crew fight boarders and run damage control."],
		[LIMITS, "Needs %s rank or higher. Costs %s; your old hull stays at the yard, with the stock parts it came with." % [GameData.rank_name(d["rank"]), cost]],
	])


## An empty mount (a filled one shows its weapon).
static func mount(index: int, cls: int, arc: int) -> Dictionary:
	var fits: String = ["light", "light and medium", "any"][clampi(cls, 0, 2)]
	var arc_text := "fires all round" if arc >= 348 else "covers %d°" % arc
	return _tip("Mount %d  (empty)" % (index + 1), "%s weapon mount  ·  %s" % [GameData.class_name_of(cls).capitalize(), arc_text], [], [
		[WHAT, "A hardpoint for one weapon. It takes %s weapons." % fits],
		[HOW, "Pick it, then a weapon from the parts list. Its arc is where it can aim; drag or scroll over a turret on the ship to turn it, and Move swaps weapons between mounts. Turning and moving are free."],
		[LIMITS, "Size goes by the weapon's volume: light up to 50, medium up to 150, heavy above."],
	])


## The "add a part" slot of the bays.
static func bays(used: int, cap: int) -> Dictionary:
	return _tip("Internal bays", "%d of %d volume used" % [used, cap], [], [
		[WHAT, "Room inside the hull for everything that isn't a gun: reactor, engines, shields, armor, sensors and the rest."],
		[HOW, "Every part takes its volume from the hull's bay space. Engines, shields and boosters stack; of every other kind the ship carries one (one sensor of each kind), and fitting another swaps it out."],
		[LIMITS, "A ship over its bay volume can't launch."],
	])


## The G1-G4 badge on a mount.
static func fire_group(g: int) -> Dictionary:
	var sel := key("weapon_group_%d" % g)
	return _tip("Fire group %d" % g, "Click to move this weapon to the next group (or %s)" % "G", [], [
		[WHAT, "Which trigger fires this weapon. Weapons in one group fire together."],
		[HOW, "Select a group with %s and fire it with %s; alt-fire (%s) fires the group set in Settings. New weapons start in their kind's group: 1 guns, 2 lasers and disruptors, 3 launchers, 4 mines." % [sel if sel != "" else "its number key", key("fire_primary") if key("fire_primary") != "" else "the fire key", key("fire_secondary") if key("fire_secondary") != "" else "its key"]],
		[CHANGES, "Moving a weapon to another group is free; an upgrade keeps its weapon's group."],
	])


const TYPE_TEXT := {
	"Lasers": "Beams that lock onto your target and never miss inside their short reach. Battery-hungry, and strong against hulls.",
	"Guns": "Unguided rounds: mass drivers reach furthest, gatlings spray close in, cannons pierce armor, disruptors strip shields and systems.",
	"Missiles": "Guided missiles that chase a locked target: agile, but point defense can shoot them down.",
	"Torpedoes": "Slow, heavy homing warheads that burst near their target. Best against big ships.",
	"Mines": "Proximity mines dropped behind you, for chasers and choke points.",
	"Drones": "Small craft that hunt enemy ships on their own.",
	"Power": "Reactors: power for every system and the battery that weapons and shields spend.",
	"Engines": "Main drives: thrust and fuel. They decide acceleration, not top speed.",
	"Drives": "Warp drives for crossing the system, boosters for bursts of speed, brakes for stopping.",
	"Shields": "Generators that soak hits before the hull. Four families behave differently.",
	"Armor": "One coat per ship: each resists damage types its own way, and some trade protection for stealth.",
	"Sensors": "How far you see and lock. Active ones see far but give you away; passive ones stay quiet.",
	"Stealth": "The jammer blinds your target; the heat sink hides you from infrared.",
	"Repair": "Self-repair: regenerators heal out of combat, repair bays mend for energy.",
	"Defense": "Point defense turrets that shoot down incoming munitions, and the ram prow.",
}


static func part_type(t: String) -> Dictionary:
	return _tip(t, "Part type", [], [[WHAT, TYPE_TEXT.get(t, "")]])


# -------------------------------------------------------- performance

## A line of the performance panel, by key (Builder names them), with
## the ship's stats now.
static func stat(k: String, st: Dictionary = {}) -> Dictionary:
	match k:
		"max_speed":
			return _tip("Top speed", "px per second", [], [
				[WHAT, "How fast the ship cruises with the throttle open."],
				[HOW, "Set by the hull, then by mass: with its stock fit a hull makes its rated speed; lighter builds go up to 12% faster, heavier ones down to 25% slower."],
				[LIMITS, "Engines don't raise it, they only get you there sooner. Boosting goes past it (see Boost speed); damaged engines drop it to as low as 60%."],
				[CHANGES, "Hull and total mass. Heavy armor, Laser II and big reactors weigh the most."],
			])
		"accel":
			return _tip("Acceleration", "px per second, each second", [], [
				[WHAT, "How hard the ship speeds up, and how hard it can brake."],
				[HOW, "Total engine thrust against the ship's mass: every engine adds, every tonne takes away."],
				[LIMITS, "In flight fuel and cargo weigh too, so a full hold is slower off the mark. Damaged engines push less."],
				[CHANGES, "Engines (more or better), mass, and boosters while boosting."],
			])
		"turn":
			return _tip("Turn rate", "degrees per second", [], [
				[WHAT, "How fast the ship can swing its nose round."],
				[HOW, "Set by the hull, then by mass: lighter than the stock fit turns up to 25% faster, heavier down to 45% slower. The thrusters sip a little fuel."],
				[LIMITS, "Damaged engines turn slower; with them out the ship can't turn at all."],
				[CHANGES, "Hull and total mass."],
			])
		"mass":
			return _tip("Mass", "tonnes, as fitted", [], [
				[WHAT, "Everything bolted on, added up."],
				[HOW, "Heavier ships accelerate, turn and top out slower, and burn hotter on infrared at full throttle. Armor mass also slows how fast the ship's heat changes."],
				[LIMITS, "In flight fuel and cargo add to it."],
				[CHANGES, "Every part's mass: armor, Laser II and big reactors are the heavy ones; sensors weigh almost nothing."],
			])
		"energy_max":
			return _tip("Energy", "battery size", [], [
				[WHAT, "The battery that weapons, shield recharge, the warp drive, jammer, pings and repair bays spend."],
				[HOW, "5 energy per point of reactor power."],
				[LIMITS, "An empty battery means no shots, no shield recharge and no warp until it refills."],
				[CHANGES, "Only the power plant."],
			])
		"energy_regen":
			return _tip("Energy regen", "energy per second", [], [
				[WHAT, "How fast the battery refills."],
				[HOW, "Comes from the reactor's power. If the parts draw more than it makes (overloaded), the refill is divided by how far over you are."],
				[LIMITS, "Reactor damage slows it, and silent running halves it. Shields recharging cost 1.5 energy per point on top of what you shoot."],
				[CHANGES, "A bigger power plant, or drawing less: fewer or smaller powered parts. Lowering shields frees their draw."],
			])
		"shields":
			return _tip("Shields", "total charge", [], [
				[WHAT, "How much the shield bubble soaks before hits reach the hull."],
				[HOW, "The charge of every generator added up, split between front and rear halves (shield focus moves it). Each half recharges on its own after a pause."],
				[LIMITS, "Ion and plasma tear through shields faster; lasers and explosives slower. Damaged generators hold less."],
				[CHANGES, "Number and size of shield generators. Their family (Shield type) sets how they recharge."],
			])
		"hull":
			return _tip("Hull", "damage it can take", [], [
				[WHAT, "How much damage the ship takes before it is destroyed."],
				[HOW, "Every part adds structure: bay parts their full hit points, weapons half. Armor coats add the most (Plate armor III +%d); a ram prow adds 200 more." % int(GameData.MODULES["5503"]["hp"])],
				[LIMITS, "Systems near a hit (engines at the stern, reactor deep inside) can fail long before the hull gives, leaving the ship adrift or open to boarding."],
				[CHANGES, "The parts you fit, armor above all. The armor coat also decides how much of each hit gets through."],
			])
		"sensor_range":
			return _tip("Lock range", "px, against a signature of 100", [], [
				[WHAT, "How far your best tracking sensor holds a lock: guns aim, missiles home, the jammer can start."],
				[HOW, "Shown against a signature of 100 (a Battleship in plain plate on radar). The real distance is this x the target's signature / 100: a Scout is seen about half as far, a Shroud-coated ship much closer. Every hull has 700 px of optics."],
				[LIMITS, "A pulse radar only locks during a ping. Planets block most sensors; an enemy jammer cuts them to %s; damaged sensors to %s." % [_pct(Sensors.JAM_FACTOR), _pct(ShipSystems.BLIND_SENSORS)]],
				[CHANGES, "Radar, pulse radar, lidar or infrared sensors (the best one counts here)."],
			])
		"fuel":
			return _tip("Fuel tank", "fuel units", [], [
				[WHAT, "Propellant for the engines and boosters (they don't run on energy)."],
				[HOW, "Each engine carries %d fuel per point of thrust. A full tank lasts about %d s at full burn without boosting." % [int(Ship.FUEL_PER_ENGINE), int(Ship.FUEL_PER_ENGINE / Ship.BURN_PER_ENGINE)]],
				[LIMITS, "Run dry and you coast until a friendly base or colony refuels you; the reactor trickles a little back while the engines rest."],
				[CHANGES, "More or bigger engines. Boosting burns it much faster."],
			])
		"boost_speed":
			return _tip("Boost speed", "top speed while boosting", [], [
				[WHAT, "Top speed with the booster lit (%s while thrusting)." % (key("boost") if key("boost") != "" else "boost")],
				[HOW, "Top speed x the boosters' speed factor: the best booster in full, each more at half."],
				[LIMITS, "Only while thrusting with fuel in the tank and the engines whole."],
				[CHANGES, "Booster marks and count; anything that changes top speed."],
			])
		"boost_accel":
			return _tip("Boost accel", "acceleration while boosting", [], [
				[WHAT, "How hard the ship accelerates with the booster lit."],
				[HOW, "Acceleration x the boosters' push factor: the best booster in full, each more at half."],
				[LIMITS, "The extra push burns fuel %sx less efficiently than the engines." % _x(Ship.BOOST_BURN)],
				[CHANGES, "Booster marks and count, engines and mass."],
			])
		"boost_burn":
			return _tip("Boost burn time", "seconds", [], [
				[WHAT, "How long a full tank lasts boosting flat out."],
				[HOW, "The engines alone last about %d s; boosting multiplies the burn by the push and wastes %sx more on top." % [int(Ship.FUEL_PER_ENGINE / Ship.BURN_PER_ENGINE), _x(Ship.BOOST_BURN)]],
				[LIMITS, "Bigger boosters push harder and burn faster: this goes down as Boost accel goes up."],
				[CHANGES, "Booster marks and count. More engines don't help: each brings its own fuel but burns it too."],
			])
		"warp":
			return _tip("Warp speed", "px per second in warp", [], [
				[WHAT, "How fast the ship crosses the system in warp."],
				[HOW, "Set by the warp drive's mark alone."],
				[LIMITS, "Needs 200 energy to start, drains 160 a second, and drops out short of planets and the Sun."],
				[CHANGES, "Only the warp drive."],
			])
		"radar":
			return _tip("Radar return", "lower is stealthier", [], [
				[WHAT, "How big you look to radar, optics and lidar."],
				[HOW, "Hull size x the armor coat's radar factor. Enemy sensors see you out to their rating x this / 100."],
				[LIMITS, "You can't change your hull's size here except by picking a smaller hull."],
				[CHANGES, "The armor coat: Shroud x0.4, Reflective x1.3, the rest x1."],
			])
		"heat_burn":
			return _tip("Heat (burning)", "infrared signature at full throttle", [], [
				[WHAT, "How hot the ship looks to infrared sensors and seekers with the engines at full burn."],
				[HOW, "Life support, the reactor's load and the engines' burn (heavier ships burn hotter). Boosting, the jammer and every shot flash on top."],
				[LIMITS, "This is before a heat sink, which soaks %s of it until it fills." % _pct(Sensors.SINK_SOAK)],
				[CHANGES, "Mass, how loaded the reactor is, and a heat sink. Armor mass makes heat rise and fall more slowly."],
			])
		"em":
			return _tip("Emissions", "EM signature", [], [
				[WHAT, "What the ship transmits, which EM receivers hear from far off."],
				[HOW, "Raised shields (%d, %d while recharging) plus active radar (its range x 2/15). Laser fire, pings and a warp drive charging add while they run." % [int(Sensors.SHIELD_EM), int(Sensors.RECHARGE_EM)]],
				[LIMITS, "Silent running (%s) switches the radar off; lowering shields takes theirs away." % (key("silent_running") if key("silent_running") != "" else "its key")],
				[CHANGES, "Radar mark (bigger radar, louder), shields, and passive sensors instead of active ones."],
			])
		"shield_type":
			var fam: String = st.get("shield_family", "standard")
			var lines := {
				"standard": "Standard: the all-rounder.",
				"barrier": "Barrier: holds the most, but waits long after a hit and refills slowly.",
				"flicker": "Flicker: holds less, recovers almost at once, but some hits leak through.",
				"deflector": "Deflector: bounces gun rounds and lasers away; holds less and draws far more power.",
			}
			return _tip("Shield type", "the shield's family", [], [
				[WHAT, "How the shield behaves: " + str(lines.get(fam, "")) if st.get("shield_max", 0.0) > 0.0 else "No shield generator fitted."],
				[HOW, "With generators of one family fitted, that family decides how the whole shield recharges and what it does to hits."],
				[CHANGES, "The shield generators you fit; one family per ship."],
			])
		"armor":
			return _tip("Armor", "the coat", [], [
				[WHAT, "Which armor coat the ship wears, if any. Point at the coat in the bays for what it stops."],
				[HOW, "A coat cuts each hull hit by its share for the hit's damage type."],
				[CHANGES, "The armor part in the bays; one per ship."],
			])
		"armor_sig":
			return _tip("Armor radar factor", "x radar return", [], [
				[WHAT, "How the armor coat changes your radar return."],
				[HOW, "Multiplies your radar return: Shroud 0.4x hides you, Reflective 1.3x makes you shine, the rest 1x."],
				[CHANGES, "Only the armor coat."],
			])
		"heat_coast":
			return _tip("Heat coasting", "infrared signature, engines off", [], [
				[WHAT, "How hot the ship looks drifting with the engines off: the quietest it gets in normal flight."],
				[HOW, "Life support (bigger hulls more) plus the reactor's load."],
				[LIMITS, "Silent running takes the reactor's share away too."],
				[CHANGES, "Hull size and how hard the reactor works: drawing less power runs cooler."],
			])
		"reactor":
			return _tip("Reactor", "power made and drawn", [], [
				[WHAT, "Your power budget: what the power plant makes against what the fitted parts draw."],
				[HOW, "Bay parts draw 35% of their rating, weapons 25%, a warp drive 5%; engines and boosters burn fuel instead."],
				[LIMITS, "Over budget the ship is overloaded: the battery refills slower (divided by the overload) and every shot costs up to half again as much energy."],
				[CHANGES, "A bigger power plant, or fewer power-hungry parts. In flight, lowering shields frees their draw."],
			])
	return {}
