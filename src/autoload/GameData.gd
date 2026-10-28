## GameData -- the rulebook of Stellar Frontier, carried over from the
## original game's data files (sf_design_ships, sf_data_modules,
## sf_design_missiles, sf_data_races, sf_data_ranks, server.scn), expressed
## in modern units: pixels, seconds and hit points.
extends Node

## One light speed in world pixels per second.  The original used 250 px/s
## on a 1024x768 screen; the remake world is about 2.4x larger.
const LIGHT_SPEED := 600.0
## Radius of the solar system in pixels (Pluto orbits at 0.46 of it).
const UNIVERSE_RADIUS := 24000.0
## Every ship's energy regeneration is scaled by this (Kyle, 2026-10-28:
## about 15% faster overall).
const ENERGY_REGEN_SCALE := 1.15

# ---------------------------------------------------------------- races

const RACES := [
	{"id": 0, "name": "Arcean", "adj": "Arcean", "home": "Titan",
	 "color": Color(0.35, 0.95, 0.55), "dark": Color(0.1, 0.4, 0.2),
	 "weapon1": "Blaster", "weapon2": "Stinger"},
	{"id": 1, "name": "Terran", "adj": "Terran", "home": "Earth",
	 "color": Color(0.4, 0.7, 1.0), "dark": Color(0.1, 0.25, 0.5),
	 "weapon1": "Mass driver", "weapon2": "Plasma torpedo"},
	{"id": 2, "name": "Drengin", "adj": "Drengin", "home": "Mars",
	 "color": Color(1.0, 0.4, 0.35), "dark": Color(0.45, 0.1, 0.1),
	 "weapon1": "Photon gun", "weapon2": "Disruptor"},
	{"id": 3, "name": "Pirate", "adj": "Pirate", "home": "Ceres",
	 "color": Color(1.0, 0.75, 0.3), "dark": Color(0.4, 0.3, 0.1),
	 "weapon1": "Mass driver", "weapon2": "Plasma torpedo"},
	# nobody's: the free port at Jupiter's Trojan point (the Resources update)
	{"id": 4, "name": "Free port", "adj": "Free", "home": "Jupiter",
	 "color": Color(0.95, 0.88, 0.6), "dark": Color(0.4, 0.36, 0.2),
	 "weapon1": "Mass driver", "weapon2": "Plasma torpedo"},
]
## The side of the free port: every empire's ships may use it, and nobody
## fights over it.
const NEUTRAL := 4

# -------------------------------------------------------------- modules
# id -> {name, vol, mass, power, hp, func, p (parameters)}
# power < 0 drains energy while active, power > 0 produces.

const MODULES := {
	"0001": {"name": "Factory", "vol": 3800, "mass": 40000, "power": -5000, "hp": 1000, "func": "factory", "p": [500]},
	"1001": {"name": "Power plant I", "vol": 100, "mass": 1500, "power": 600, "hp": 100, "func": "power", "p": [50]},
	"1002": {"name": "Power plant II", "vol": 125, "mass": 3000, "power": 800, "hp": 200, "func": "power", "p": [50]},
	"1003": {"name": "Power plant III", "vol": 150, "mass": 4000, "power": 1000, "hp": 300, "func": "power", "p": [50]},
	"1004": {"name": "Power plant IV", "vol": 300, "mass": 8000, "power": 1200, "hp": 600, "func": "power", "p": [50]},
	"1005": {"name": "Power plant V", "vol": 500, "mass": 12000, "power": 1800, "hp": 1200, "func": "power", "p": [50]},
	"1006": {"name": "Station reactor", "vol": 4000, "mass": 25000, "power": 5000, "hp": 2000, "func": "power", "p": [50]},
	"2001": {"name": "Engine I", "vol": 150, "mass": 500, "power": -100, "hp": 50, "func": "eng", "p": [120]},
	"2002": {"name": "Engine II", "vol": 200, "mass": 750, "power": -200, "hp": 100, "func": "eng", "p": [180]},
	"2003": {"name": "Engine III", "vol": 400, "mass": 1000, "power": -400, "hp": 150, "func": "eng", "p": [360]},
	"2004": {"name": "Engine IV", "vol": 600, "mass": 1250, "power": -500, "hp": 200, "func": "eng", "p": [1200]},
	"3001": {"name": "Quantum drive I", "vol": 100, "mass": 100, "power": -100, "hp": 100, "func": "qdrv", "p": [1, 35000]},
	"3002": {"name": "Quantum drive II", "vol": 200, "mass": 200, "power": -200, "hp": 150, "func": "qdrv", "p": [2, 35000]},
	"3003": {"name": "Quantum drive III", "vol": 300, "mass": 300, "power": -1000, "hp": 200, "func": "qdrv", "p": [3, 35000]},
	# tube: [energy frames, energy per shot, ammo, missile id]
	"4001": {"name": "Mass driver", "vol": 30, "mass": 50, "power": -200, "hp": 100, "func": "tube", "p": [60, 4000, 800, "001"]},
	"4002": {"name": "Mine layer", "vol": 10, "mass": 50, "power": -500, "hp": 100, "func": "tube", "p": [15, 20000, 10, "002"]},
	"4003": {"name": "Torpedo launcher", "vol": 45, "mass": 50, "power": -400, "hp": 100, "func": "tube", "p": [15, 6000, 800, "004"]},
	"4004": {"name": "Mine layer", "vol": 100, "mass": 50, "power": -1000, "hp": 100, "func": "tube", "p": [100, 99999, 4, "002"]},
	"4008": {"name": "Smart missile rack", "vol": 40, "mass": 50, "power": -300, "hp": 200, "func": "tube", "p": [40, 6000, 300, "008"]},
	"4009": {"name": "Seeker rack", "vol": 40, "mass": 50, "power": -300, "hp": 200, "func": "tube", "p": [40, 6000, 32, "009"]},
	"4010": {"name": "Rapid tube", "vol": 80, "mass": 750, "power": -200, "hp": 200, "func": "tube", "p": [60, 1000, 1500, "010"]},
	"4011": {"name": "Station battery", "vol": 4500, "mass": 20000, "power": -200, "hp": 200, "func": "tube", "p": [60, 1000, 1500, "011"]},
	"4018": {"name": "Cannon", "vol": 200, "mass": 50, "power": -500, "hp": 100, "func": "tube", "p": [50, 30000, 100, "018"]},
	# beam: [energy frames, energy per frame, range]
	"4501": {"name": "Laser I", "vol": 3, "mass": 50, "power": -200, "hp": 200, "func": "beam", "p": [30, 242, 200]},
	"4502": {"name": "Laser II", "vol": 300, "mass": 15000, "power": -200, "hp": 200, "func": "beam", "p": [30, 300, 250]},
	"4503": {"name": "Laser III", "vol": 4500, "mass": 20000, "power": -200, "hp": 200, "func": "beam", "p": [30, 800, 600]},
	"5001": {"name": "Shield I", "vol": 50, "mass": 500, "power": -2, "hp": 75, "func": "shield", "p": [200]},
	"5002": {"name": "Shield II", "vol": 100, "mass": 900, "power": -3, "hp": 100, "func": "shield", "p": [300]},
	"5003": {"name": "Shield III", "vol": 250, "mass": 1200, "power": -4, "hp": 125, "func": "shield", "p": [400]},
	"5004": {"name": "Shield IV", "vol": 400, "mass": 1600, "power": -5, "hp": 175, "func": "shield", "p": [500]},
	"5005": {"name": "Shield V", "vol": 1000, "mass": 2000, "power": -6, "hp": 200, "func": "shield", "p": [800]},
	"5006": {"name": "Shield VI", "vol": 3500, "mass": 2400, "power": -6, "hp": 250, "func": "shield", "p": [1600]},
	"5007": {"name": "Station shield", "vol": 5000, "mass": 3000, "power": -12, "hp": 250, "func": "shield", "p": [1000]},
	"5501": {"name": "Armor I", "vol": 100, "mass": 2000, "power": 0, "hp": 300, "func": "armor", "p": [50]},
	"5502": {"name": "Armor II", "vol": 200, "mass": 5000, "power": 0, "hp": 500, "func": "armor", "p": [75]},
	"5503": {"name": "Armor III", "vol": 300, "mass": 10000, "power": 0, "hp": 750, "func": "armor", "p": [100]},
	"6001": {"name": "Brake", "vol": 500, "mass": 500, "power": -200, "hp": 100, "func": "brake", "p": [90]},
	"7001": {"name": "Sensors I", "vol": 10, "mass": 4, "power": -10, "hp": 10, "func": "sensor", "p": [10]},
	"7002": {"name": "Sensors II", "vol": 20, "mass": 8, "power": -100, "hp": 10, "func": "sensor", "p": [25]},
	"7003": {"name": "Sensors III", "vol": 30, "mass": 12, "power": -100, "hp": 10, "func": "sensor", "p": [40]},
	"9101": {"name": "Cloaking device", "vol": 50, "mass": 100, "power": -500, "hp": 100, "func": "stealth", "p": [500, 800]},
	"9201": {"name": "Booster I", "vol": 100, "mass": 500, "power": -100, "hp": 100, "func": "boost", "p": [1000, 150, 30]},
	"9202": {"name": "Booster II", "vol": 140, "mass": 700, "power": -150, "hp": 100, "func": "boost", "p": [1200, 200, 50]},
	"9203": {"name": "Booster III", "vol": 160, "mass": 800, "power": -180, "hp": 100, "func": "boost", "p": [1400, 250, 70]},
	"9301": {"name": "Docking tractor", "vol": 400, "mass": 1000, "power": -100, "hp": 100, "func": "dock", "p": [400, 2400, 200]},
	# remake parts with an empire flavour: Arcean regeneration and drones,
	# Terran repair, Drengin rams and disruptors
	"5101": {"name": "Regenerator I", "vol": 60, "mass": 300, "power": -150, "hp": 80, "func": "regen", "p": [6]},
	"5102": {"name": "Regenerator II", "vol": 120, "mass": 500, "power": -250, "hp": 120, "func": "regen", "p": [14]},
	"5201": {"name": "Repair bay I", "vol": 80, "mass": 900, "power": -100, "hp": 150, "func": "repair", "p": [10]},
	"5202": {"name": "Repair bay II", "vol": 160, "mass": 1500, "power": -150, "hp": 250, "func": "repair", "p": [22]},
	"5601": {"name": "Ram prow", "vol": 120, "mass": 2500, "power": 0, "hp": 400, "func": "ram", "p": [3]},
	"4020": {"name": "Disruptor", "vol": 45, "mass": 120, "power": -400, "hp": 120, "func": "tube", "p": [40, 5000, 600, "020"]},
	"4021": {"name": "Drone bay", "vol": 50, "mass": 200, "power": -300, "hp": 150, "func": "tube", "p": [40, 6000, 40, "021"]},
}

## Which empire's shipyard sells a part.  Parts not listed are common.
## Another empire's part needs a blueprint, salvaged from its wrecks.
const MODULE_RACE := {
	"5005": 0, "5006": 0, "7003": 0, "5101": 0, "5102": 0, "4021": 0,
	"5502": 1, "5503": 1, "4008": 1, "4009": 1, "5201": 1, "5202": 1,
	"9101": 2, "9203": 2, "5601": 2,
}

## Parts only a station carries.
const STATION_ONLY := ["0001", "1006", "4011", "4503", "5007", "9301", "2004"]

## Internal bay volume of each hull.
const HULL_BAYS := {
	"Scout": 1300, "Destroyer": 1500, "Raider": 1500, "Cruiser": 1600,
	"BattleCruiser": 3000, "Battleship": 2200, "Dreadnought": 3000, "Starbase": 20000, "Freighter": 1800, "Trader": 1900, "DropShip": 1700,
	"MiningStation": 20000, "GasSkimmer": 20000, "Refinery": 20000, "SupplyDepot": 20000, "SensorStation": 20000, "DefenseStation": 20000,
	"TradeStation": 20000, "Shipyard": 20000,
}

## Crew aboard each hull.  Hits kill crew, damage control works as fast
## as there are hands for it, and boarding is crew against crew.
const HULL_CREW := {
	"Scout": 6, "Destroyer": 12, "Raider": 12, "Freighter": 8, "Cruiser": 24,
	"BattleCruiser": 36, "Battleship": 44, "Dreadnought": 60, "Starbase": 300,
	"MiningStation": 12, "GasSkimmer": 12, "Refinery": 20, "SupplyDepot": 30, "SensorStation": 10, "DefenseStation": 40,
	"Trader": 6, "TradeStation": 30, "Shipyard": 60, "DropShip": 16,
}

## Credits for a hull at the shipyard.
const HULL_PRICE := {
	"Scout": 0, "Destroyer": 400, "Cruiser": 1500,
	"BattleCruiser": 2600, "Battleship": 3600, "Dreadnought": 6000,
	"Freighter": 300, "Trader": 600, "DropShip": 700,
}

## What the yard needs from its stockpile to build a hull besides the
## credits (Kyle's pick: bigger hulls cost Alloys and Exotics).  A starbase
## yard draws on its home world's stockpile, a shipyard station on its own.
const HULL_MATERIALS := {
	"Cruiser": {"alloys": 10},
	"BattleCruiser": {"alloys": 20, "exotics": 4},
	"Battleship": {"alloys": 30, "exotics": 8},
	"Dreadnought": {"alloys": 50, "exotics": 15},
	"Freighter": {"metals": 20},
	"Trader": {"metals": 20, "alloys": 5},
	"DropShip": {"metals": 20, "alloys": 5},
}

# ------------------------------------------------------------- missiles
# warhead, firing velocity (fraction of light), guidance, trigger, seek
# range, max speed (fraction of light), life (seconds), radius.  Optional:
# "sys" scales the damage to the systems near the hit (ShipSystems), and
# "heat" seekers home on the exhaust, so their hits go into the engines.

const MISSILES := {
	"001": {"dtype": "kinetic", "name": "bolt", "warhead": 150, "fire_v": 0.60, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 1.20, "life": 3.2, "radius": 4, "look": "bolt"},
	"002": {"dtype": "explosive", "name": "mine", "warhead": 2500, "fire_v": 0.05, "guided": false, "trigger": "prox", "prox": 50, "seek_range": 0, "max_v": 0.05, "life": 32.0, "radius": 9, "look": "mine"},
	"003": {"dtype": "plasma", "name": "plasma", "warhead": 500, "fire_v": 0.35, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 0.90, "life": 3.2, "radius": 6, "look": "plasma"},
	"004": {"dtype": "explosive", "name": "torpedo", "warhead": 1500, "fire_v": 0.30, "guided": false, "trigger": "prox", "prox": 64, "seek_range": 0, "max_v": 0.95, "life": 3.2, "radius": 7, "look": "grenade"},
	"008": {"dtype": "explosive", "name": "smart missile", "warhead": 180, "fire_v": 0.30, "guided": true, "trigger": "impact", "seek_range": 500, "max_v": 0.45, "life": 6.5, "radius": 4, "look": "missile", "turn": 3.5, "accel": 220},
	"009": {"dtype": "explosive", "name": "seeker", "warhead": 400, "fire_v": 0.30, "guided": true, "trigger": "impact", "seek_range": 500, "max_v": 0.45, "life": 6.5, "radius": 5, "look": "missile", "turn": 2.5, "accel": 220, "heat": true},
	"010": {"dtype": "kinetic", "name": "rapid bolt", "warhead": 100, "fire_v": 0.70, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 0.80, "life": 3.2, "radius": 3, "look": "bolt"},
	"011": {"dtype": "plasma", "name": "rapid plasma", "warhead": 400, "fire_v": 0.80, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 1.00, "life": 3.2, "radius": 5, "look": "plasma"},
	"018": {"dtype": "kinetic", "name": "cannon shell", "recoil": 2.0, "warhead": 750, "fire_v": 0.40, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 0.90, "life": 3.2, "radius": 6, "look": "shell"},
	"020": {"dtype": "ion", "name": "disruptor bolt", "warhead": 220, "fire_v": 0.55, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 1.0, "life": 2.6, "radius": 5, "look": "disruptor", "drain": 320, "sys": 1.8},
	"021": {"dtype": "explosive", "name": "drone", "warhead": 90, "fire_v": 0.25, "guided": true, "trigger": "impact", "seek_range": 700, "max_v": 0.5, "life": 9.0, "radius": 4, "look": "drone", "turn": 4.5, "accel": 260},
}

## Damage types: every weapon deals one, and shields and the hull take it
## at these multipliers (armor resists by type on top, see the armor
## types).  Beams are lasers, ion the disruptor, plasma the station and
## colony batteries and the Sun's heat; rams and collisions are kinetic.
## All 1.0 until the weapon roles are tuned.
const DAMAGE_TYPES := {
	"kinetic": {"shield": 1.0, "hull": 1.0},
	"beam": {"shield": 1.0, "hull": 1.0},
	"explosive": {"shield": 1.0, "hull": 1.0},
	"plasma": {"shield": 1.0, "hull": 1.0},
	"ion": {"shield": 1.0, "hull": 1.0},
}

## How hard a damage type hits a layer ("shield" or "hull").
static func damage_mult(dtype: String, layer: String) -> float:
	return float(DAMAGE_TYPES.get(dtype, DAMAGE_TYPES["kinetic"]).get(layer, 1.0))


## The damage type a tube or beam module deals.
static func module_dtype(id: String) -> String:
	var m: Dictionary = MODULES[id]
	if m["func"] == "beam":
		return "beam"
	if m["func"] == "tube":
		return MISSILES[m["p"][3]].get("dtype", "kinetic")
	return "kinetic"


## Seconds between shots for each tube missile.
const TUBE_COOLDOWN := {
	"001": 0.28, "002": 1.0, "003": 0.5, "004": 0.55, "008": 0.7, "009": 0.9,
	"010": 0.09, "011": 0.09, "018": 0.45, "020": 0.4, "021": 0.5,
}

# ------------------------------------------------------- ship designs
# Hardpoints: [module id, angle (deg) of the mount, radius (px), fire angle
# (deg), arc start (deg), arc end (deg)].  The original stored the angles in
# tenths of a turn (10 degree units); they are converted here.

const SHIPS := {
	"Scout": {"rank": 0, "speed": 0.25, "size": 22, "turn": 200.0,
		"modules": ["1001", "5003", "2002", "3003", "7003", "9201", "9101"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 0, 0, 0, -50, 50], ["4002", 180, 0, 180, 170, 190]]},
	"Destroyer": {"rank": 2, "speed": 0.23, "size": 28, "turn": 170.0,
		"modules": ["1002", "5002", "2002", "3002", "7001", "9201", "9101"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 40, 0, 0, -90, 90], ["4001", -40, 0, 0, -90, 90], ["4002", 180, 0, 180, 170, 190]]},
	"Raider": {"rank": 2, "speed": 0.27, "size": 28, "turn": 180.0, "pirate": true,
		"modules": ["1002", "5002", "2002", "3002", "7001", "9201", "9101"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 40, 0, 0, -90, 90], ["4001", -40, 0, 0, -90, 90]]},
	"Cruiser": {"rank": 6, "speed": 0.21, "size": 36, "turn": 130.0,
		"modules": ["1003", "5002", "2003", "3001", "7002", "9201"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 0, 0, 0, -50, 50], ["4010", 50, 18, 0, -30, 150], ["4010", -50, 18, 0, -150, 30], ["4002", 180, 0, 180, 170, 190]]},
	"Battleship": {"rank": 10, "speed": 0.19, "size": 44, "turn": 100.0,
		"modules": ["1003", "5003", "2003", "3001", "7002", "9201"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4003", 0, 0, 0, -30, 30], ["4001", 40, 0, 0, -30, 30], ["4001", -40, 0, 0, -30, 30], ["4002", 180, 0, 180, 170, 190]]},
	"Dreadnought": {"rank": 12, "speed": 0.17, "size": 56, "turn": 75.0,
		"modules": ["1004", "5004", "2003", "3002", "7003", "9201"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4003", 50, 18, 0, -30, 150], ["4003", -50, 18, 0, -150, 30], ["4001", 0, 0, 0, 0, 350], ["4002", 180, 0, 180, 170, 190]]},
	"Starbase": {"rank": 14, "speed": 0.10, "size": 110, "turn": 20.0,
		"modules": ["1006", "5007", "2004", "7002", "0001"],
		"dock": [60, 0],
		"weapons": [["4003", 0, 0, 0, -180, 180], ["4003", 120, 0, 120, 180, 360], ["4011", 240, 0, 240, 0, 350], ["4503", 0, 10, 0, 0, 350]]},
	"Freighter": {"rank": 0, "speed": 0.17, "size": 40, "turn": 90.0,
		"modules": ["1001", "5001", "5502", "2003", "3003", "7003", "9201"],
		"dock": [180, 0],
		"weapons": [["4001", 0, 0, 0, -50, 50]]},
	# the trade ship (the Resources update, phase 4): unarmed, fast, heavily
	# shielded, with the biggest hold
	"Trader": {"rank": 0, "speed": 0.23, "size": 40, "turn": 110.0,
		"modules": ["1003", "5004", "5003", "2003", "3003", "7002", "9201"],
		"dock": [180, 0],
		"weapons": []},
	# the drop ship (the Resources update, phase 5): carries six ground
	# units down to a world, heavily armoured, with one light gun
	"DropShip": {"rank": 0, "speed": 0.2, "size": 38, "turn": 105.0,
		"modules": ["1003", "5003", "5502", "2003", "3003", "7001", "9201"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120]]},
	# stations built at construction sites (the Resources update, phase 3):
	# fixed installations with no engines, never offered at the shipyard
	"MiningStation": {"rank": 99, "speed": 0.0, "size": 46, "turn": 0.0,
		"modules": ["1003", "5003", "7001"], "weapons": []},
	"GasSkimmer": {"rank": 99, "speed": 0.0, "size": 50, "turn": 0.0,
		"modules": ["1003", "5003", "7001"], "weapons": []},
	"Refinery": {"rank": 99, "speed": 0.0, "size": 50, "turn": 0.0,
		"modules": ["1004", "5003", "7001"], "weapons": []},
	"SupplyDepot": {"rank": 99, "speed": 0.0, "size": 56, "turn": 0.0,
		"modules": ["1004", "5004", "7002"], "weapons": []},
	"SensorStation": {"rank": 99, "speed": 0.0, "size": 40, "turn": 0.0,
		"modules": ["1003", "5002", "7003"], "weapons": []},
	"DefenseStation": {"rank": 99, "speed": 0.0, "size": 50, "turn": 30.0,
		"modules": ["1006", "5005", "7002"],
		"weapons": [["4003", 0, 0, 0, -180, 180], ["4010", 120, 0, 120, 0, 360], ["4010", 240, 0, 240, 0, 360]]},
	"TradeStation": {"rank": 99, "speed": 0.0, "size": 60, "turn": 0.0,
		"modules": ["1004", "5004", "7002"], "weapons": []},
	"Shipyard": {"rank": 99, "speed": 0.0, "size": 64, "turn": 0.0,
		"modules": ["1006", "5004", "7002", "0001"], "weapons": []},
	"BattleCruiser": {"rank": 8, "speed": 0.22, "size": 46, "turn": 110.0, "aces": true,
		"modules": ["1005", "5005", "2003", "3003", "7003", "9203"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4010", 0, 0, 0, -50, 50], ["4003", 50, 18, 0, -30, 150], ["4003", -50, 18, 0, -150, 30], ["4002", 180, 0, 180, 170, 190]]},
}

## Ships a pilot may refit to at a friendly starbase, in rank order.
const REFIT_ORDER := ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought", "Starbase"]
## Support hulls the shipyard also sells: the supply ship, the trade ship
## and the drop ship.
const CIVIL_HULLS := ["Freighter", "Trader", "DropShip"]


## A hull's name as the shipyard lists it.
static func hull_label(hull: String) -> String:
	return {"Freighter": "Supply ship", "Trader": "Trade ship", "DropShip": "Drop ship"}.get(hull, hull)


## The hulls on a shipyard's list, warships first.
static func yard_hulls() -> Array:
	var out := []
	for n in REFIT_ORDER:
		if n != "Starbase":
			out.append(n)
	return out + CIVIL_HULLS

# ---------------------------------------------------------------- ranks
# name, level, active score, total score, total kills, duty: what a
# computer pilot of that rank does (colonize: ferry colonists; attack:
# knock out enemy colonies; escort: guard convoys; raid: hunt enemy
# supply ships; protect: hold the starbase)

const RANKS := [
	{"name": "Cadet", "score": 0, "kills": 0, "duty": "colonize"},
	{"name": "Ensign", "score": 10, "kills": 4, "duty": "attack"},
	{"name": "2nd Lt.", "score": 25, "kills": 10, "duty": "colonize"},
	{"name": "1st Lt.", "score": 50, "kills": 20, "duty": "attack"},
	{"name": "Sr. Lt.", "score": 100, "kills": 40, "duty": "colonize"},
	{"name": "Lt. Cmdr.", "score": 250, "kills": 80, "duty": "escort"},
	{"name": "Cmdr.", "score": 500, "kills": 100, "duty": "escort"},
	{"name": "Captain", "score": 1000, "kills": 150, "duty": "attack"},
	{"name": "Sr. Captain", "score": 2500, "kills": 200, "duty": "raid"},
	{"name": "Fleet Captain", "score": 5000, "kills": 400, "duty": "raid"},
	{"name": "Commodore", "score": 10000, "kills": 1000, "duty": "attack"},
	{"name": "Rear Admiral", "score": 25000, "kills": 2000, "duty": "raid"},
	{"name": "Vice Admiral", "score": 50000, "kills": 3000, "duty": "colonize"},
	{"name": "Admiral", "score": 100000, "kills": 5000, "duty": "attack"},
	{"name": "Fleet Admiral", "score": 250000, "kills": 8000, "duty": "protect"},
	{"name": "Emperor", "score": 500000, "kills": 10000, "duty": "protect"},
	{"name": "Deity", "score": 999999, "kills": 20000, "duty": "protect"},
]

# --------------------------------------------------------------- robots
# The computer pilots of the Sol scenario: name, race, rank, ship name.

const ROBOTS := [
	["SharpClaw", 0, 14, "ArceusPrime"], ["RedFang", 0, 0, "Ursa"], ["LittlePaw", 0, 1, "Claw"],
	["BadWind", 0, 2, "Fang"], ["LongLegs", 0, 3, "Slasher"], ["SharpTooth", 0, 6, "Gnasher"],
	["Slash", 0, 9, "Predator"], ["Scar", 0, 0, "Pouncer"], ["BigPaw", 0, 1, "Meerkat"],
	["LongNeck", 0, 2, "Panther"], ["Hawk", 0, 3, "Leopard"], ["SpikeTail", 0, 6, "Cougar"],
	["RazorScale", 0, 9, "Claw"], ["Talon", 0, 12, "Panther"],
	["Patton", 1, 14, "StarControl"], ["Bradley", 1, 0, "Victorious"], ["MacArthur", 1, 1, "Notorious"],
	["Aldrin", 1, 2, "Triumphant"], ["Armstrong", 1, 3, "Reliant"], ["Collins", 1, 6, "Constellation"],
	["McDivit", 1, 9, "Constitution"], ["Schirra", 1, 0, "Galactica"], ["Cooper", 1, 1, "Atlantis"],
	["Glenn", 1, 2, "Privateer"], ["Yeager", 1, 3, "Providence"], ["Slayton", 1, 6, "Felicity"],
	["Shepard", 1, 9, "Enterprise"], ["Grissom", 1, 12, "Intrepid"],
	["Caesar", 2, 14, "DrenginPrime"], ["Augustus", 2, 0, "Ruthless"], ["Tiberius", 2, 1, "Relentless"],
	["Caligula", 2, 2, "Remorseless"], ["Claudius", 2, 3, "Vicious"], ["Nero", 2, 6, "Merciless"],
	["Pompey", 2, 9, "Fearless"], ["Marius", 2, 12, "Courageous"], ["Cato", 2, 1, "Executioner"],
	["Cicero", 2, 2, "Slasher"], ["Spartacus", 2, 0, "Raider"], ["Brutus", 2, 1, "Plunder"],
]

# ----------------------------------------------------------- Sol system
# Bodies: class, name, orbit (fraction of the universe radius for planets,
# pixels for moons), parent, mass (Earths), look, temperature (K),
# habitability, visual radius.  Angle * means random.

const SOL := [
	["Star", "Sol", 0.0, "", 333000.0, "Sun", 5800, 0, 420],
	["Planet", "Mercury", 0.148, "Sol", 0.06, "Mercury", 800, 0, 40],
	["Planet", "Venus", 0.165, "Sol", 0.81, "Venus", 600, 0, 68],
	["Planet", "Earth", 0.2, "Sol", 1.0, "Earth", 300, 70, 72],
	["Moon", "Luna", 250, "Earth", 0.012, "Luna", 300, 0, 26],
	["Planet", "Mars", 0.25, "Sol", 0.11, "Mars", 260, 0, 50],
	["Moon", "Phobos", 150, "Mars", 0.001, "Aster", 260, 0, 12],
	["Moon", "Deimos", 210, "Mars", 0.001, "Aster", 260, 0, 10],
	["Minor", "Ceres", 0.255, "Sol", 0.002, "Aster", 180, 0, 20],
	["Minor", "Pallas", 0.257, "Sol", 0.001, "Aster", 180, 0, 16],
	["Minor", "Juno", 0.26, "Sol", 0.001, "Aster", 180, 0, 14],
	["Minor", "Vesta", 0.262, "Sol", 0.001, "Aster", 180, 0, 16],
	["Minor", "Astraea", 0.265, "Sol", 0.001, "Aster", 180, 0, 12],
	["Minor", "Hebe", 0.267, "Sol", 0.001, "Aster", 180, 0, 13],
	["Minor", "Iris", 0.27, "Sol", 0.001, "Aster", 180, 0, 12],
	["Minor", "Chiron", 0.272, "Sol", 0.001, "Aster", 180, 0, 11],
	["Minor", "Hermes", 0.275, "Sol", 0.001, "Aster", 180, 0, 12],
	["Minor", "Icarus", 0.28, "Sol", 0.001, "Aster", 180, 0, 11],
	["Minor", "Eros", 0.285, "Sol", 0.001, "Aster", 180, 0, 14],
	["Minor", "Achilles", 0.29, "Sol", 0.001, "Aster", 180, 0, 12],
	["Minor", "Apollo", 0.277, "Sol", 0.001, "Aster", 180, 0, 11],
	["Minor", "Amor", 0.282, "Sol", 0.001, "Aster", 180, 0, 12],
	["Planet", "Jupiter", 0.3, "Sol", 318.0, "Jupiter", 160, 100, 170],
	["Moon", "Callisto", 525, "Jupiter", 0.018, "Callisto", 160, 0, 30],
	["Moon", "Europa", 420, "Jupiter", 0.008, "Europa", 160, 0, 24],
	["Moon", "Ganymede", 470, "Jupiter", 0.025, "Ganymede", 160, 0, 32],
	["Moon", "Io", 370, "Jupiter", 0.015, "Io", 160, 0, 26],
	["Planet", "Saturn", 0.35, "Sol", 95.2, "Saturn", 130, 100, 150],
	["Moon", "Titan", 400, "Saturn", 0.022, "Titan", 170, 0, 34],
	["Moon", "Rhea", 320, "Saturn", 0.004, "Rhea", 130, 0, 20],
	["Planet", "Uranus", 0.4, "Sol", 14.5, "Uranus", 110, 100, 110],
	["Planet", "Neptune", 0.45, "Sol", 17.1, "Neptune", 100, 100, 108],
	["Moon", "Triton", 300, "Neptune", 0.004, "Triton", 100, 0, 22],
	["Planet", "Pluto", 0.46, "Sol", 0.002, "Pluto", 60, 0, 22],
]

## Starting colonies of the Sol scenario: planet -> [race, design, population].
const COLONIES := {
	"Mercury": [0, "Minor", 3000000],
	"Venus": [2, "Minor", 3000000000],
	"Earth": [1, "Major", 3000000000],
	"Luna": [1, "Outpost", 250000000],
	"Mars": [2, "Major", 1200000000],
	"Phobos": [2, "Outpost", 100000000],
	"Deimos": [2, "Outpost", 100000000],
	"Pallas": [3, "Outpost", 100000000],
	"Juno": [3, "Outpost", 100000000],
	"Vesta": [3, "Outpost", 100000000],
	"Callisto": [0, "Major", 700000000],
	"Europa": [0, "Outpost", 250000000],
	"Ganymede": [0, "Major", 700000000],
	"Io": [0, "Outpost", 375000000],
	"Titan": [0, "Major", 700000000],
	"Rhea": [0, "Outpost", 90000000],
	"Triton": [0, "Major", 90000000],
}

## Colony defences by design: weapons as hardpoints, shield id, hull.
const COLONY_DESIGNS := {
	"Outpost": {"shield": "5004", "armor": "5501", "weapons": [["4001", 0, 0, 0, 0, 350]], "range": 700},
	"Minor": {"shield": "5004", "armor": "5502", "weapons": [["4003", 0, 0, 0, 0, 350]], "range": 800},
	"Major": {"shield": "5004", "armor": "5503", "weapons": [["4003", 0, 0, 0, 0, 350], ["4502", 0, 0, 0, 0, 350]], "range": 900},
}

## Where each race's starbase sits (a moon or planet of theirs).
const STARBASES := {0: "Titan", 1: "Earth", 2: "Mars"}

# ------------------------------------------------------------ colonists
# The original moved populations as colonies carried in a ship's dock: a
# pilot orbited a friendly colony, took colonists aboard, and dropped them
# on another world, where they settled it, joined the colony there, or
# fought its people.  Here a ship lands to take them on or put them down.

## Temperature (K) each race thrives at; habitability falls away from it.
const RACE_PREF_TEMP := {0: 160, 1: 300, 2: 380, 3: 180}
## Colonists a hull's cabin holds, in millions.  Starbases never land.
const COLONIST_CABIN := {
	"Scout": 40, "Destroyer": 80, "Raider": 80, "Cruiser": 160,
	"BattleCruiser": 200, "Battleship": 250, "Dreadnought": 400, "Freighter": 800, "Trader": 100, "DropShip": 0, "Starbase": 0,
}
## Cargo a hull's hold carries, in units of a resource (the Resources
## update).  The Freighter is the supply ship; starbases hold stock of their
## own later, not in a hold.
const CARGO_HOLD := {
	"Scout": 10, "Destroyer": 20, "Raider": 30, "Cruiser": 40,
	"BattleCruiser": 60, "Battleship": 80, "Dreadnought": 120, "Freighter": 300, "Trader": 400, "DropShip": 20, "Starbase": 0,
	# a station's hold is its stockpile; a trade station's is its market
	"MiningStation": 300, "GasSkimmer": 300, "Refinery": 400, "SupplyDepot": 1500, "SensorStation": 0, "DefenseStation": 0,
	"TradeStation": 2000, "Shipyard": 600,
}

## Stations built from kits (the Resources update, phase 3).  cost: what
## haulers must deliver to the frame; build: seconds of work once it is all
## there; kit: credits for the kit at a starbase; where: "rock" (a world
## nobody lives on, with Metals, Ice or Exotics), "gas" (a gas giant) or
## "any" (anywhere but the Sun).
const STATIONS := {
	"MiningStation": {"label": "Mining station", "cost": {"metals": 20}, "build": 90.0, "kit": 150, "where": "rock",
		"does": "mines Metals, Ice and Exotics from a world nobody lives on"},
	"GasSkimmer": {"label": "Gas skimmer", "cost": {"metals": 20, "alloys": 5}, "build": 120.0, "kit": 250, "where": "gas",
		"does": "skims Fuel from a gas giant"},
	"Refinery": {"label": "Refinery", "cost": {"metals": 30, "alloys": 5}, "build": 120.0, "kit": 300, "where": "any",
		"does": "refines Metals and Fuel delivered to it into Alloys"},
	"SupplyDepot": {"label": "Supply depot", "cost": {"metals": 25}, "build": 90.0, "kit": 250, "where": "any",
		"does": "stores 1500 units; refuels and crews ships like a starbase"},
	"SensorStation": {"label": "Sensor station", "cost": {"metals": 15, "exotics": 3}, "build": 60.0, "kit": 200, "where": "any",
		"does": "shows your empire every ship within 6000 px"},
	"DefenseStation": {"label": "Defensive station", "cost": {"metals": 30, "alloys": 8}, "build": 120.0, "kit": 350, "where": "any",
		"does": "a turret platform guarding a world, a mine or a lane"},
	# phase 4: trade and shipyards
	"TradeStation": {"label": "Trade station", "cost": {"metals": 40, "alloys": 10}, "build": 180.0, "kit": 400, "where": "any",
		"does": "a market for every resource; you take 10% of foreign trade"},
	"Shipyard": {"label": "Shipyard", "cost": {"metals": 60, "alloys": 20, "exotics": 5}, "build": 240.0, "kit": 500, "where": "any",
		"does": "refits (U), sells kits and builds ships away from home"},
}
const STATION_ORDER := ["MiningStation", "GasSkimmer", "Refinery", "SupplyDepot", "SensorStation", "DefenseStation", "TradeStation", "Shipyard"]
## The rank that may buy and deploy station kits (Lt. Cmdr.).
const KIT_RANK := 5


static func is_station_hull(hull: String) -> bool:
	return hull == "Starbase" or STATIONS.has(hull)


## Units a station's frame needs delivered before the work starts.
static func station_cost_total(key: String) -> float:
	var n := 0.0
	for k in STATIONS[key]["cost"]:
		n += STATIONS[key]["cost"][k]
	return n


## A station under construction: the bare frame, a small power plant and
## no guns, until the materials are in and the work is done.
static func frame_design(key: String) -> Dictionary:
	var d := stock_design(key)
	d["modules"] = ["1001"]
	d["weapons"] = []
	return d


## Ships a pilot can order at a starbase or shipyard (the Resources update,
## phase 4): credits, seconds to build, the rank that may order it, and the
## yard's materials (HULL_MATERIALS).  Supply and trade ships join the
## empire's haulers; a warship flies as the buyer's wingman.
const ORDERS := {
	"Freighter": {"label": "Supply ship", "credits": 400, "build": 40.0, "rank": 5, "does": "hauls your empire's goods"},
	"Trader": {"label": "Trade ship", "credits": 600, "build": 45.0, "rank": 5, "does": "trades your colonies' surplus at the markets"},
	"Destroyer": {"label": "Destroyer escort", "credits": 900, "build": 50.0, "rank": 7, "does": "flies as your wingman"},
	"Cruiser": {"label": "Cruiser escort", "credits": 2200, "build": 75.0, "rank": 7, "does": "flies as your wingman"},
	"DropShip": {"label": "Drop ship", "credits": 700, "build": 45.0, "rank": 5, "does": "lands your empire's troops on enemy worlds"},
}
const ORDER_LIST := ["Freighter", "Trader", "DropShip", "Destroyer", "Cruiser"]
## Supply and trade ships an empire may have on order or flying beyond its
## own computer haulers.
const ORDERED_HAULERS := 4
## What a shipyard station keeps in stock for building hulls; haulers bring
## it up to this much of each, and are paid for it.
const YARD_GOODS := {"metals": 120.0, "alloys": 120.0, "exotics": 40.0}

## Each empire's computer supply ships: [pilot, ship name].
const SUPPLY_SHIPS := {
	0: [["Gatherer", "Plenty"], ["Forager", "Harvest Moon"]],
	1: [["Quartermaster", "Providence"], ["Purser", "Endeavour"]],
	2: [["Tithe", "Tribute"], ["Toll", "Plunder"]],
}
## Each empire's computer trade ship: [pilot, ship name].
const TRADE_SHIPS := {
	0: ["Barterer", "Silk Road"],
	1: ["Factor", "Clipper"],
	2: ["Broker", "Spoils"],
}
## Each empire's computer drop ship: [pilot, ship name].
const DROP_SHIPS := {
	0: ["Pathfinder", "Long Leap"],
	1: ["Sergeant", "Normandy"],
	2: ["Centurion", "Legion"],
}

# ------------------------------------------------------------ ground war
# The Resources update, phase 5: planetary buildings paid from a colony's
# own stockpile, ground units recruited at a barracks, and the planetary
# shield that has to fall before anyone lands.

## Planetary buildings.  cost: from the colony's stockpile; build: seconds;
## upkeep: units a minute from the stockpile.
const BUILDINGS := {
	"shield": {"label": "Planetary shield", "cost": {"alloys": 20, "exotics": 10}, "build": 90.0, "upkeep": {"fuel": 1.0},
		"does": "a bubble over the world: nothing lands and nobody dies while it stands"},
	"laser": {"label": "Laser battery", "cost": {"metals": 20, "alloys": 4}, "build": 60.0, "upkeep": {},
		"does": "a long-range beam for the colony's guns"},
	"silo": {"label": "Missile silo", "cost": {"metals": 15, "alloys": 5, "exotics": 2}, "build": 75.0, "upkeep": {},
		"does": "guided missiles out to 1400 px"},
	"flak": {"label": "Flak battery", "cost": {"metals": 10, "alloys": 2}, "build": 45.0, "upkeep": {},
		"does": "shoots down missiles, and drop ships on their way in"},
	"barracks": {"label": "Barracks and bunkers", "cost": {"metals": 15}, "build": 45.0, "upkeep": {},
		"does": "recruits troops; defenders fight at 1.5x behind the bunkers"},
	"industry": {"label": "Industry", "cost": {"metals": 25, "alloys": 5}, "build": 90.0, "upkeep": {},
		"does": "refines Alloys and adds half again to Metals output"},
	"farm": {"label": "Hydroponic farms", "cost": {"metals": 10, "ice": 10}, "build": 60.0, "upkeep": {"ice": 1.0},
		"does": "Food on a world with a poor climate"},
}
const BUILDING_ORDER := ["shield", "laser", "silo", "flak", "barracks", "industry", "farm"]
## Building slots by colony size.
const BUILDING_SLOTS := {"Outpost": 2, "Minor": 4, "Major": 6}

## Ground units.  cost: from the stockpile at the barracks; people: taken
## from the population; strength: in the ground battle; upkeep: a minute.
const UNITS := {
	"infantry": {"label": "Infantry", "cost": {"metals": 2}, "people": 50e6, "strength": 1.0, "upkeep": {"food": 1.0},
		"does": "a cheap holding force; marines aboard a warship"},
	"armour": {"label": "Armour", "cost": {"metals": 6, "fuel": 2, "alloys": 1}, "people": 0.0, "strength": 4.0, "upkeep": {"fuel": 1.0},
		"does": "breaks garrisons"},
	"commandos": {"label": "Commandos", "cost": {"alloys": 1, "exotics": 2}, "people": 20e6, "strength": 2.0, "upkeep": {"food": 1.0},
		"does": "blow up shields and batteries from the ground; double a boarding party"},
}
const UNIT_ORDER := ["infantry", "armour", "commandos"]
## Ground units a hull carries: warships a squad or two of marines, the
## drop ship a whole landing force.
const TROOP_BAYS := {
	"Scout": 1, "Destroyer": 1, "Raider": 1, "Cruiser": 2, "BattleCruiser": 2, "Battleship": 2, "Dreadnought": 2,
	"DropShip": 6,
}
## What the Sol scenario's colonies start with: a barracks and a garrison
## by size, and the home worlds a planetary shield as well.
const START_GROUND := {
	"Outpost": {"buildings": [], "garrison": {}},
	"Minor": {"buildings": ["barracks"], "garrison": {"infantry": 2}},
	"Major": {"buildings": ["barracks"], "garrison": {"infantry": 3, "armour": 1}},
}
const HOME_BUILDINGS := ["shield"]

## Credits a unit of each resource is worth to a hauler who brings it where
## it is short (and the base price at trade stations later).
const RESOURCE_PRICE := {"metals": 1.0, "ice": 1.0, "food": 1.0, "fuel": 2.0, "exotics": 4.0, "alloys": 3.0}
## An invading colony fights as if it were this many times its size (the
## original's offensive_factor of 200%).  Defenders behind their shields
## fight it on even terms.
const OFFENSIVE_FACTOR := 2.0
## Troops take populated worlds now; colonists still can, but only a
## world with no troops on it, and only when they outnumber its people
## this many to one (Kyle's pick: a very weak fallback).
const COLONIST_RATIO := 20.0
## A colony this small dies out; one this big runs Minor, then Major defences.
const MIN_COLONY := 1000000
const MINOR_COLONY := 150000000
const MAJOR_COLONY := 1000000000


## People shown the way the original did: 3.0 B, 250 M, 40 K.
static func fmt_pop(n: float) -> String:
	if n >= 1e9:
		return "%.1f B" % (n / 1e9)
	if n >= 1e6:
		return "%d M" % int(round(n / 1e6))
	if n >= 1e3:
		return "%d K" % int(round(n / 1e3))
	return str(int(n))


## How well a race lives at a temperature, 0.02..1: the original's
## temperature correlation, softened so harsh worlds still hold a few.
static func climate(race_id: int, temperature: float) -> float:
	var pref: float = RACE_PREF_TEMP.get(race_id, 300)
	var off := absf(log(maxf(temperature, 1.0) / pref))
	return clampf(1.0 - off * 1.2, 0.02, 1.0)


static func race(id: int) -> Dictionary:
	return RACES[clampi(id, 0, RACES.size() - 1)]


static func rank_for(score: int, kills: int) -> int:
	var r := 0
	for i in RANKS.size():
		if score >= RANKS[i]["score"] and kills >= RANKS[i]["kills"]:
			r = i
	return r


static func rank_name(level: int) -> String:
	return RANKS[clampi(level, 0, RANKS.size() - 1)]["name"]


# --------------------------------------------------- shipyard helpers

## Tier of a part (I..V) for pricing and pickups.
static func tier(id: String) -> int:
	var m: Dictionary = MODULES[id]
	if m["func"] in ["power", "eng", "shield", "armor", "sensor", "boost", "qdrv", "regen", "repair", "beam"]:
		return maxi(int(id.substr(3, 1)), 1)
	return 1


## The empire that sells a part, or -1 when every shipyard does.
static func part_race(id: String) -> int:
	return MODULE_RACE.get(id, -1)


## Credits a part costs at the shipyard.
static func price(id: String) -> int:
	var m: Dictionary = MODULES[id]
	var base: float = 30.0 + m["vol"] * 0.5 + absf(m["power"]) * 0.06 + m["hp"] * 0.25 + m["mass"] * 0.004
	if m["func"] == "tube":
		var md: Dictionary = MISSILES[m["p"][3]]
		base += md["warhead"] * 0.12 + (120.0 if md["guided"] else 0.0)
	base *= 1.0 + 0.35 * (tier(id) - 1)
	if part_race(id) >= 0:
		base *= 1.2
	return int(round(base / 5.0)) * 5


## Weapon groups (1-4) the pilot selects with the number keys.  A new
## weapon joins a group by what it is: guns and beams 1, launchers 2,
## guided missiles 3, mines 4.  The shipyard lets the pilot change it.
const WEAPON_GROUPS := 4

static func default_weapon_group(id: String) -> int:
	var m: Dictionary = MODULES[id]
	if m["func"] == "beam":
		return 1
	if m["func"] == "tube":
		match str(m["p"][3]):
			"001", "010":
				return 1
			"002":
				return 4
			"008", "009", "021":
				return 3
			_:
				return 2
	return 1


## The group a design's weapon row belongs to (its 7th field, if set).
static func weapon_group_of(w: Array) -> int:
	if w.size() > 6 and int(w[6]) >= 1 and int(w[6]) <= WEAPON_GROUPS:
		return int(w[6])
	return default_weapon_group(w[0])


## Weapon size class: a hardpoint takes its own class and anything lighter.
static func weapon_class(id: String) -> int:
	var v: int = MODULES[id]["vol"]
	if v <= 50:
		return 0     # light
	if v <= 150:
		return 1     # medium
	return 2         # heavy


static func class_name_of(c: int) -> String:
	return ["light", "medium", "heavy"][clampi(c, 0, 2)]


## Hardpoints of a hull: [angle, radius, fire angle, arc0, arc1, class].
static func hull_hardpoints(hull: String) -> Array:
	var d: Dictionary = SHIPS[hull]
	var out := []
	var i := 0
	for w in d["weapons"]:
		var c := weapon_class(w[0])
		if not is_station_hull(hull):
			if d["rank"] >= 6 and i > 0 and c == 0 and w[2] > 0:
				c = 1          # cruisers and up carry medium turrets
			if d["rank"] >= 10 and c == 0:
				c = 1
			if d["rank"] >= 12 and w[2] > 0:
				c = 2          # dreadnought turrets are heavy
			if w[1] == 180:
				c = 0          # the rear mine rack stays small
		out.append([w[1], w[2], w[3], w[4], w[5], c])
		i += 1
	return out


## A fresh copy of a stock design as a buildable design dictionary.
static func stock_design(hull: String) -> Dictionary:
	var d: Dictionary = SHIPS[hull]
	var weapons := []
	for w in d["weapons"]:
		weapons.append(w.duplicate())
	return {
		"name": hull, "rank": d["rank"], "speed": d["speed"], "turn": d["turn"], "size": d["size"],
		"dock": d.get("dock", [180, 0]), "modules": d["modules"].duplicate(), "weapons": weapons,
		"pirate": d.get("pirate", false), "aces": d.get("aces", false),
	}


## Mass of a hull's stock fit, the reference for turn and speed penalties.
static func stock_mass(hull: String) -> float:
	var d: Dictionary = SHIPS[hull]
	var m := 0.0
	for id in d["modules"]:
		m += MODULES[id]["mass"]
	for w in d["weapons"]:
		m += MODULES[w[0]]["mass"]
	return maxf(m, 1.0)


## Can a part go in an internal bay (as opposed to a hardpoint)?
static func is_bay_part(id: String) -> bool:
	var f: String = MODULES[id]["func"]
	return f not in ["tube", "beam", "factory", "dock"]


## Parts a shipyard of this empire sells to a pilot with these blueprints.
static func catalogue(race_id: int, blueprints: Array, for_station: bool = false) -> Array:
	var out := []
	for id in MODULES:
		if id in STATION_ONLY and not for_station:
			continue
		var r := part_race(id)
		if r >= 0 and r != race_id and not blueprints.has(id):
			continue
		out.append(id)
	out.sort_custom(func(a, b):
		var fa: String = MODULES[a]["func"]
		var fb: String = MODULES[b]["func"]
		if fa != fb:
			return fa < fb
		return a < b)
	return out


## Everything a design adds up to; shared by the ship and the shipyard so
## the numbers on the screen are the numbers in flight.
static func stats_for(design: Dictionary) -> Dictionary:
	var st := {
		"mass": 0.0, "energy_max": 0.0, "energy_regen": 0.0, "shield_max": 0.0, "shield_regen": 0.0,
		"hull_max": 0.0, "armor": 0.0, "sensor_range": 800.0, "can_cloak": false, "has_boost": false,
		"qdrv": 0.0, "brake_power": 0.6, "regen": 0.0, "repair": 0.0, "ram": 0.0,
		"power_gen": 0.0, "power_draw": 0.0, "overload": 1.0, "module_levels": {}, "bay_used": 0,
		"bay_cap": int(HULL_BAYS.get(design["name"], 2000)), "engines": 0, "eng": 0.0,
	}
	var is_station: bool = is_station_hull(design["name"])
	for id in design["modules"]:
		var m: Dictionary = MODULES[id]
		st["mass"] += m["mass"]
		st["hull_max"] += m["hp"]
		st["bay_used"] += m["vol"]
		if m["power"] > 0:
			st["power_gen"] += m["power"]
		else:
			# continuous draw; a quantum drive only pulls hard while it is charging
			st["power_draw"] += -m["power"] * (0.05 if m["func"] == "qdrv" else 0.35)
		var level := tier(id)
		st["module_levels"][m["func"]] = maxi(st["module_levels"].get(m["func"], 0), level)
		match m["func"]:
			"power":
				st["energy_max"] += m["power"] * 5.0
				st["energy_regen"] += m["power"] * 0.45
			"eng":
				st["eng"] += m["p"][0]
				st["engines"] += 1
			"shield":
				st["shield_max"] += m["p"][0]
				st["shield_regen"] += m["p"][0] * 0.06
			"armor":
				st["armor"] += m["p"][0]
				st["hull_max"] += m["p"][0] * 4
			"sensor":
				st["sensor_range"] = maxf(st["sensor_range"], 600.0 + m["p"][0] * 40.0)
			"stealth":
				st["can_cloak"] = true
			"boost":
				st["has_boost"] = true
			"qdrv":
				st["qdrv"] = maxf(st["qdrv"], float(m["p"][0]))
			"brake":
				st["brake_power"] = 1.6
			"regen":
				st["regen"] += m["p"][0]
			"repair":
				st["repair"] += m["p"][0]
			"ram":
				st["ram"] = maxf(st["ram"], float(m["p"][0]))
				st["hull_max"] += 200
			"factory":
				st["energy_regen"] += 400.0
	for w in design["weapons"]:
		var m: Dictionary = MODULES[w[0]]
		st["mass"] += m["mass"]
		st["hull_max"] += m["hp"] * 0.5
		st["power_draw"] += absf(m["power"]) * 0.25
	if is_station:
		st["hull_max"] *= 4.0
		st["shield_max"] *= 2.5
		st["sensor_range"] = 6000.0 if design["name"] == "SensorStation" and design["modules"].size() > 1 else 2600.0
	# the reactor budget: drawing more than it makes slows everything down
	if st["power_gen"] > 0.0 and st["power_draw"] > st["power_gen"]:
		st["overload"] = st["power_draw"] / st["power_gen"]
		st["energy_regen"] /= st["overload"]
	elif st["power_gen"] <= 0.0:
		st["overload"] = 3.0
	st["energy_regen"] *= ENERGY_REGEN_SCALE
	# mass against the hull's stock fit decides handling
	var ratio: float = stock_mass(design["name"]) / maxf(st["mass"], 1.0)
	st["turn"] = design["turn"] * clampf(pow(ratio, 0.5), 0.55, 1.25)
	st["max_speed"] = design["speed"] * LIGHT_SPEED * clampf(pow(ratio, 0.3), 0.75, 1.12)
	st["accel"] = clampf(st["eng"] * 0.9 * (3000.0 / maxf(st["mass"], 1500.0)), 40.0, 420.0)
	if is_station:
		st["accel"] = 12.0
		st["turn"] = design["turn"]
		st["max_speed"] = design["speed"] * LIGHT_SPEED
	return st


static func missile_display_name(race_id: int, missile_id: String) -> String:
	var rc := race(race_id)
	match missile_id:
		"001", "010":
			return rc["weapon1"]
		"003", "011":
			return rc["weapon2"]
	return MISSILES[missile_id]["name"].capitalize()


static func ship_names_for_rank(level: int, include_aces: bool = true) -> Array:
	var out := []
	for n in REFIT_ORDER:
		var d: Dictionary = SHIPS[n]
		if d["rank"] <= level and (include_aces or not d.get("aces", false)):
			out.append(n)
	return out
