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
]

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
	"4001": {"name": "Torpedo tube", "vol": 30, "mass": 50, "power": -200, "hp": 100, "func": "tube", "p": [60, 4000, 800, "001"]},
	"4002": {"name": "Mine layer", "vol": 10, "mass": 50, "power": -500, "hp": 100, "func": "tube", "p": [15, 20000, 10, "002"]},
	"4003": {"name": "Grenade launcher", "vol": 45, "mass": 50, "power": -400, "hp": 100, "func": "tube", "p": [15, 6000, 800, "004"]},
	"4004": {"name": "Mine layer", "vol": 100, "mass": 50, "power": -1000, "hp": 100, "func": "tube", "p": [100, 99999, 4, "002"]},
	"4008": {"name": "Smart missile rack", "vol": 40, "mass": 50, "power": -300, "hp": 200, "func": "tube", "p": [40, 6000, 300, "008"]},
	"4009": {"name": "Seeker rack", "vol": 40, "mass": 50, "power": -300, "hp": 200, "func": "tube", "p": [40, 6000, 32, "009"]},
	"4010": {"name": "Rapid tube", "vol": 80, "mass": 750, "power": -200, "hp": 200, "func": "tube", "p": [60, 1000, 1500, "010"]},
	"4011": {"name": "Station battery", "vol": 4500, "mass": 20000, "power": -200, "hp": 200, "func": "tube", "p": [60, 1000, 1500, "011"]},
	"4018": {"name": "Cannon", "vol": 100, "mass": 50, "power": -500, "hp": 100, "func": "tube", "p": [50, 30000, 100, "018"]},
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
	"5501": {"name": "Armor I", "vol": 100, "mass": 2000, "power": 10, "hp": 300, "func": "armor", "p": [50]},
	"5502": {"name": "Armor II", "vol": 200, "mass": 5000, "power": 10, "hp": 500, "func": "armor", "p": [75]},
	"5503": {"name": "Armor III", "vol": 300, "mass": 10000, "power": 10, "hp": 750, "func": "armor", "p": [100]},
	"6001": {"name": "Brake", "vol": 500, "mass": 500, "power": -200, "hp": 100, "func": "brake", "p": [90]},
	"7001": {"name": "Sensors I", "vol": 10, "mass": 4, "power": -10, "hp": 10, "func": "sensor", "p": [10]},
	"7002": {"name": "Sensors II", "vol": 20, "mass": 8, "power": -100, "hp": 10, "func": "sensor", "p": [25]},
	"7003": {"name": "Sensors III", "vol": 30, "mass": 12, "power": -100, "hp": 10, "func": "sensor", "p": [40]},
	"9101": {"name": "Cloaking device", "vol": 50, "mass": 100, "power": -500, "hp": 100, "func": "stealth", "p": [500, 800]},
	"9201": {"name": "Booster I", "vol": 100, "mass": 500, "power": -100, "hp": 100, "func": "boost", "p": [1000, 150, 30]},
	"9202": {"name": "Booster II", "vol": 140, "mass": 700, "power": -150, "hp": 100, "func": "boost", "p": [1200, 200, 50]},
	"9203": {"name": "Booster III", "vol": 160, "mass": 800, "power": -180, "hp": 100, "func": "boost", "p": [1400, 250, 70]},
	"9301": {"name": "Docking tractor", "vol": 400, "mass": 1000, "power": -100, "hp": 100, "func": "dock", "p": [400, 2400, 200]},
}

# ------------------------------------------------------------- missiles
# warhead, firing velocity (fraction of light), guidance, trigger, seek
# range, max speed (fraction of light), life (seconds), radius

const MISSILES := {
	"001": {"name": "torpedo", "warhead": 150, "fire_v": 0.60, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 1.20, "life": 3.2, "radius": 4, "look": "bolt"},
	"002": {"name": "mine", "warhead": 2500, "fire_v": 0.05, "guided": false, "trigger": "prox", "prox": 50, "seek_range": 0, "max_v": 0.05, "life": 32.0, "radius": 9, "look": "mine"},
	"003": {"name": "plasma", "warhead": 500, "fire_v": 0.35, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 0.90, "life": 3.2, "radius": 6, "look": "plasma"},
	"004": {"name": "grenade", "warhead": 1500, "fire_v": 0.30, "guided": false, "trigger": "prox", "prox": 64, "seek_range": 0, "max_v": 0.95, "life": 3.2, "radius": 7, "look": "grenade"},
	"008": {"name": "smart missile", "warhead": 180, "fire_v": 0.30, "guided": true, "trigger": "impact", "seek_range": 500, "max_v": 0.45, "life": 6.5, "radius": 4, "look": "missile", "turn": 3.5, "accel": 220},
	"009": {"name": "seeker", "warhead": 400, "fire_v": 0.30, "guided": true, "trigger": "impact", "seek_range": 500, "max_v": 0.45, "life": 6.5, "radius": 5, "look": "missile", "turn": 2.5, "accel": 220},
	"010": {"name": "rapid torpedo", "warhead": 100, "fire_v": 0.70, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 0.80, "life": 3.2, "radius": 3, "look": "bolt"},
	"011": {"name": "rapid plasma", "warhead": 400, "fire_v": 0.80, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 1.00, "life": 3.2, "radius": 5, "look": "plasma"},
	"018": {"name": "cannon shell", "warhead": 750, "fire_v": 0.40, "guided": false, "trigger": "impact", "seek_range": 0, "max_v": 0.90, "life": 3.2, "radius": 6, "look": "shell"},
}

## Seconds between shots for each tube missile.
const TUBE_COOLDOWN := {
	"001": 0.28, "002": 1.0, "003": 0.5, "004": 0.55, "008": 0.7, "009": 0.9,
	"010": 0.09, "011": 0.09, "018": 0.45,
}

# ------------------------------------------------------- ship designs
# Hardpoints: [module id, angle (deg) of the mount, radius (px), fire angle
# (deg), arc start (deg), arc end (deg)].  The original stored the angles in
# tenths of a turn (10 degree units); they are converted here.

const SHIPS := {
	"Scout": {"rank": 0, "speed": 0.25, "size": 22, "turn": 200.0,
		"modules": ["1001", "5003", "2002", "3003", "7003", "9201", "9101", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 0, 0, 0, -50, 50]]},
	"Destroyer": {"rank": 2, "speed": 0.23, "size": 28, "turn": 170.0,
		"modules": ["1002", "5002", "2002", "3002", "7001", "9201", "9101", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 40, 0, 0, -90, 90], ["4001", -40, 0, 0, -90, 90]]},
	"Raider": {"rank": 2, "speed": 0.27, "size": 28, "turn": 180.0, "pirate": true,
		"modules": ["1002", "5002", "2002", "3002", "7001", "9201", "9101", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 40, 0, 0, -90, 90], ["4001", -40, 0, 0, -90, 90]]},
	"Cruiser": {"rank": 6, "speed": 0.21, "size": 36, "turn": 130.0,
		"modules": ["1003", "5002", "2003", "3001", "7002", "9201", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4001", 0, 0, 0, -50, 50], ["4010", 50, 18, 0, -30, 150], ["4010", -50, 18, 0, -150, 30]]},
	"Battleship": {"rank": 10, "speed": 0.19, "size": 44, "turn": 100.0,
		"modules": ["1003", "5003", "2003", "3001", "7002", "9201", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4003", 0, 0, 0, -30, 30], ["4001", 40, 0, 0, -30, 30], ["4001", -40, 0, 0, -30, 30]]},
	"Dreadnought": {"rank": 12, "speed": 0.17, "size": 56, "turn": 75.0,
		"modules": ["1004", "5004", "2003", "3002", "7003", "9201", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4003", 50, 18, 0, -30, 150], ["4003", -50, 18, 0, -150, 30], ["4001", 0, 0, 0, 0, 350]]},
	"Starbase": {"rank": 14, "speed": 0.10, "size": 110, "turn": 20.0,
		"modules": ["1006", "5007", "2004", "7002", "0001"],
		"dock": [60, 0],
		"weapons": [["4003", 0, 0, 0, -180, 180], ["4003", 120, 0, 120, 180, 360], ["4011", 240, 0, 240, 0, 350], ["4503", 0, 10, 0, 0, 350]]},
	"Freighter": {"rank": 0, "speed": 0.17, "size": 40, "turn": 90.0,
		"modules": ["1001", "5001", "5502", "2003", "3003", "7003", "9201"],
		"dock": [180, 0],
		"weapons": [["4001", 0, 0, 0, -50, 50]]},
	"Marauder": {"rank": 0, "speed": 0.25, "size": 30, "turn": 170.0, "aces": true,
		"modules": ["1005", "5004", "2003", "3003", "7003", "9203", "9101", "4018"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4003", 40, 0, 0, -90, 90], ["4003", -40, 0, 0, -90, 90]]},
	"BattleCruiser": {"rank": 8, "speed": 0.22, "size": 46, "turn": 110.0, "aces": true,
		"modules": ["1005", "5005", "2004", "3003", "7003", "9203", "4004"],
		"dock": [180, 0],
		"weapons": [["4501", 0, 0, 0, -120, 120], ["4010", 0, 0, 0, -50, 50], ["4003", 50, 18, 0, -30, 150], ["4003", -50, 18, 0, -150, 30]]},
}

## Ships a pilot may refit to at a friendly starbase, in rank order.
const REFIT_ORDER := ["Scout", "Destroyer", "Marauder", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought", "Starbase"]

# ---------------------------------------------------------------- ranks
# name, level, active score, total score, total kills, duty

const RANKS := [
	{"name": "Cadet", "score": 0, "kills": 0, "duty": "colonize"},
	{"name": "Ensign", "score": 10, "kills": 4, "duty": "attack"},
	{"name": "2nd Lt.", "score": 25, "kills": 10, "duty": "colonize"},
	{"name": "1st Lt.", "score": 50, "kills": 20, "duty": "attack"},
	{"name": "Sr. Lt.", "score": 100, "kills": 40, "duty": "colonize"},
	{"name": "Lt. Cmdr.", "score": 250, "kills": 80, "duty": "harass"},
	{"name": "Cmdr.", "score": 500, "kills": 100, "duty": "colonize"},
	{"name": "Captain", "score": 1000, "kills": 150, "duty": "attack"},
	{"name": "Sr. Captain", "score": 2500, "kills": 200, "duty": "harass"},
	{"name": "Fleet Captain", "score": 5000, "kills": 400, "duty": "colonize"},
	{"name": "Commodore", "score": 10000, "kills": 1000, "duty": "attack"},
	{"name": "Rear Admiral", "score": 25000, "kills": 2000, "duty": "harass"},
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
