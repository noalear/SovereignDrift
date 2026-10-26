## Economy -- what each world yields and what it has piled up.  Every
## planet and moon gets a richness (0..5) per resource, worked out from its
## Sol data; colonies turn richness into stock every second, scaled by how
## many hands they have, and Minor and Major colonies refine Metals and Fuel
## into Alloys.  Stock stays where it was made: ships move it (the Resources
## update, phase 2 on).  Runs on the host; clients get the stockpiles from
## Net every couple of seconds.
class_name Economy
extends RefCounted

## Raw resources, then the refined one.
const RESOURCES := ["metals", "ice", "fuel", "food", "exotics", "alloys"]
const RAW := ["metals", "ice", "fuel", "food", "exotics"]
const NAMES := {"metals": "Metals", "ice": "Ice", "fuel": "Fuel", "food": "Food", "exotics": "Exotics", "alloys": "Alloys"}
const COLORS := {
	"metals": Color(0.75, 0.78, 0.85), "ice": Color(0.6, 0.85, 1.0), "fuel": Color(1.0, 0.7, 0.3),
	"food": Color(0.55, 0.9, 0.45), "exotics": Color(0.85, 0.5, 1.0), "alloys": Color(0.95, 0.88, 0.55),
}

## Units a minute for each point of richness, with a full workforce.
const RATE_PER_POINT := 4.0
## A colony stops producing a resource once it holds this much of it.
const STOCK_CAP := {"Outpost": 200.0, "Minor": 500.0, "Major": 1000.0}
## Alloys refined a minute (2 Metals + 1 Fuel each) by colony size.
const REFINE_PER_MIN := {"Minor": 4.0, "Major": 8.0}
## New colonies in a fresh game start with this many minutes of output.
const SEED_MINUTES := 3.0

## What each body's surface is, read from its look: the Sol data has no
## column for it, and the look already names the world's character.
const SURFACE := {
	"Sun": "sun", "Mercury": "rock", "Venus": "haze", "Earth": "wet", "Luna": "rock",
	"Mars": "rock", "Aster": "aster", "Jupiter": "gas", "Callisto": "rock", "Europa": "ice",
	"Ganymede": "rock", "Io": "volcanic", "Saturn": "gas", "Titan": "haze", "Rhea": "ice",
	"Uranus": "gas", "Neptune": "gas", "Triton": "ice", "Pluto": "ice",
}

var world: Node
var _tick := 0.0


func _init(p_world: Node = null) -> void:
	world = p_world


static func surface_of(look: String, habitable: int) -> String:
	if SURFACE.has(look):
		return SURFACE[look]
	return "gas" if habitable >= 100 else "rock"


## A body's richness per raw resource, 0..5, from its Sol row: surface,
## temperature (K), hydrosphere (%), mass (Earths), radius (px) and class.
static func richness(look: String, body_class: String, temperature: float, habitable: int, mass: float, radius: float) -> Dictionary:
	var surf := surface_of(look, habitable)
	var r := {"metals": 0, "ice": 0, "fuel": 0, "food": 0, "exotics": 0}
	var t := temperature
	match surf:
		"sun":
			r["exotics"] = 5            # a solar forge in the heat band
			return r
		"gas":
			r["fuel"] = 5 if mass >= 50.0 else 4
			return r
	# metals: asteroids are ore, the hot inner rock is densest, ice is thin
	if surf == "aster":
		r["metals"] = 4
	elif t >= 700.0:
		r["metals"] = 5
	elif surf == "ice":
		r["metals"] = 1
	else:
		r["metals"] = 3 if t >= 250.0 else 2
	# ice
	if surf == "ice":
		r["ice"] = 5 if radius >= 24.0 else 4
	elif surf == "wet" or (habitable > 0 and habitable < 100):
		r["ice"] = 3
	elif surf == "rock" and t < 200.0:
		r["ice"] = 3
	elif surf == "haze" and t < 200.0:
		r["ice"] = 2
	elif surf == "aster" and t < 200.0:
		r["ice"] = 1
	elif surf == "rock" and t < 320.0:
		r["ice"] = 1
	# fuel: methane seas and nitrogen ice
	if surf == "haze" and t < 200.0:
		r["fuel"] = 3
	elif surf == "ice" and t <= 100.0:
		r["fuel"] = 1
	# food: oceans feed the most; elsewhere farms under domes, as far as
	# the water goes.  The owner's climate scales it again in production.
	if surf == "wet":
		r["food"] = 5
	elif surf in ["rock", "haze", "ice"] and r["ice"] > 0:
		r["food"] = 2 if r["ice"] >= 3 else 1
	# exotics: volcanoes, the hot heavy air of a Venus, the inner rock, and
	# the big asteroids
	if surf == "volcanic":
		r["exotics"] = 4
	elif surf == "haze" and t >= 400.0:
		r["exotics"] = 3
	elif t >= 700.0:
		r["exotics"] = 2
	elif surf == "aster":
		r["exotics"] = 3 if body_class == "Minor" and radius >= 16.0 else 1
	return r


static func empty_stock() -> Dictionary:
	var d := {}
	for k in RESOURCES:
		d[k] = 0.0
	return d


## How much of the work a colony gets done: its people against what the
## world supports, softened so a young colony still produces.
static func workforce(p: Planet) -> float:
	if not p.is_colonised():
		return 0.0
	var opt := p.optimal_population(p.owner_race)
	if opt <= 0.0:
		return 0.25
	return sqrt(clampf(p.population / opt, 0.0, 1.0))


static func cap(p: Planet) -> float:
	return STOCK_CAP.get(p.colony_design, 0.0)


## How well a world makes a resource, 0..5: its richness, or for Alloys
## its refinery (Minor 2, Major 4).  Hauling pays only toward a world that
## makes the goods worse than where they came from.
static func makes(p: Planet, k: String) -> float:
	if k == "alloys":
		return refine_rate(p) * 0.5 if p.is_colonised() else 0.0
	return float(p.richness.get(k, 0))


## Alloys a minute a colony can refine: by its size, and more with
## industry (phase 5's planetary buildings).
static func refine_rate(p: Planet) -> float:
	var r: float = REFINE_PER_MIN.get(p.colony_design, 0.0)
	if p.buildings.has("industry"):
		r += Ground.INDUSTRY_REFINE
	return r


## Credits for unloading `n` units of `k` on `p` from `origin` (null:
## salvage).  Units that fill the world up to a quarter of its cap earn the
## full price, up to half of it half the price, beyond that nothing.
static func delivery_pay(p: Planet, origin: Planet, k: String, n: float) -> float:
	if not p.is_colonised() or origin == p:
		return 0.0
	if origin != null and makes(p, k) >= makes(origin, k):
		return 0.0
	var c := cap(p)
	var before: float = p.stock.get(k, 0.0)
	var low := clampf(c * 0.25 - before, 0.0, n)
	var mid := clampf(c * 0.5 - maxf(before, c * 0.25), 0.0, n - low)
	return GameData.RESOURCE_PRICE.get(k, 1.0) * (low + mid * 0.5)


## How badly a colony wants more of a resource, 0..1: short of half its cap
## and not making much itself.  Refineries want Metals and Fuel more.
static func need(p: Planet, k: String) -> float:
	if not p.is_colonised() or p.silenced > 0.0:
		return 0.0
	var c := cap(p)
	var short := clampf(1.0 - p.stock.get(k, 0.0) / maxf(c * 0.5, 1.0), 0.0, 1.0)
	if makes(p, k) >= 3.0:
		short *= 0.2
	if k in ["metals", "fuel"] and refine_rate(p) > 0.0:
		short = minf(short * 1.5, 1.0)
	return short


## Units that would bring a colony up to half its cap.
static func wanted(p: Planet, k: String) -> float:
	if not p.is_colonised():
		return 0.0
	return maxf(cap(p) * 0.5 - p.stock.get(k, 0.0), 0.0)


## What a colony can spare of a resource for a computer hauler: anything
## over a third of its cap.
static func surplus(p: Planet, k: String) -> float:
	if not p.is_colonised():
		return 0.0
	return maxf(p.stock.get(k, 0.0) - cap(p) / 3.0, 0.0)


## Units a minute this world makes of each resource right now (alloys are
## what it can refine with the stock it has).
static func production(p: Planet) -> Dictionary:
	var out := empty_stock()
	if not p.is_colonised() or p.silenced > 0.0:
		return out
	var wf := workforce(p)
	for k in RAW:
		var rate: float = p.richness.get(k, 0) * RATE_PER_POINT * wf
		if k == "food":
			rate *= GameData.climate(p.owner_race, p.temperature)
			if p.buildings.has("farm") and not p.short.has("ice"):
				rate += Ground.FARM_FOOD * wf
		elif k == "metals" and p.buildings.has("industry"):
			rate *= 1.0 + Ground.INDUSTRY_METALS
		out[k] = rate
	var refine := refine_rate(p)
	if refine > 0.0:
		var can := minf(p.stock.get("metals", 0.0) / 2.0, p.stock.get("fuel", 0.0))
		out["alloys"] = refine if can >= 1.0 else 0.0
	return out


## A fresh game: every colony starts with a few minutes of its own output.
func seed_stocks() -> void:
	for p in world.planets:
		p.stock = empty_stock()
		if not p.is_colonised():
			continue
		var rates := production(p)
		for k in RAW:
			p.stock[k] = minf(rates[k] * SEED_MINUTES, cap(p))
		# and what its refinery would have made: a yard needs Alloys for hulls
		p.stock["alloys"] = REFINE_PER_MIN.get(p.colony_design, 0.0) * SEED_MINUTES


## The host's tick: production lands once a second.
func tick(delta: float) -> void:
	_tick += delta
	if _tick < 1.0:
		return
	var dt := _tick
	_tick = 0.0
	for p in world.planets:
		if p.is_colonised():
			step(p, dt)
	for s in world.ships:
		if s.is_station and not s.dead and not s.building:
			station_step(s, dt)
			if s.design_name == "TradeStation":
				market_step(s, dt)


## One world's production over `dt` seconds.
static func step(p: Planet, dt: float) -> void:
	var rates := production(p)
	var c := cap(p)
	for k in RAW:
		if rates[k] <= 0.0:
			continue
		var have: float = p.stock.get(k, 0.0)
		if have < c:
			p.stock[k] = minf(have + rates[k] * dt / 60.0, c)
	if rates["alloys"] > 0.0 and p.stock.get("alloys", 0.0) < c:
		var n := minf(rates["alloys"] * dt / 60.0, c - p.stock.get("alloys", 0.0))
		n = minf(n, minf(p.stock["metals"] / 2.0, p.stock["fuel"]))
		p.stock["metals"] -= n * 2.0
		p.stock["fuel"] -= n
		p.stock["alloys"] = p.stock.get("alloys", 0.0) + n


## Mining stations and skimmers work at this share of a full colony's
## rate for the body's richness: no people, just machines.
const STATION_RATE := 0.75
## Alloys a refinery station makes a minute (2 Metals + 1 Fuel each).
const STATION_REFINE := 6.0


## Units a minute a working station makes of each resource.
static func station_production(s: Ship) -> Dictionary:
	var out := {}
	var p := s.station_body
	match s.design_name:
		"MiningStation":
			if p != null:
				for k in ["metals", "ice", "exotics"]:
					if p.richness.get(k, 0) > 0:
						out[k] = p.richness[k] * RATE_PER_POINT * STATION_RATE
		"GasSkimmer":
			if p != null and p.richness.get("fuel", 0) > 0:
				out["fuel"] = p.richness["fuel"] * RATE_PER_POINT * STATION_RATE
		"Refinery":
			if minf(s.cargo.get("metals", 0.0) / 2.0, s.cargo.get("fuel", 0.0)) >= 1.0:
				out["alloys"] = STATION_REFINE
	return out


## One station's work over `dt` seconds: production goes into its hold
## until the hold is full.
static func station_step(s: Ship, dt: float) -> void:
	var rates := station_production(s)
	if rates.is_empty():
		return
	if s.design_name == "Refinery":
		var n := minf(rates["alloys"] * dt / 60.0, minf(s.cargo.get("metals", 0.0) / 2.0, s.cargo.get("fuel", 0.0)))
		if n > 0.0:
			s.cargo["metals"] -= n * 2.0
			s.cargo["fuel"] -= n
			s.cargo["alloys"] = s.cargo.get("alloys", 0.0) + n
		return
	for k in rates:
		var n := minf(rates[k] * dt / 60.0, s.cargo_space())
		if n > 0.0:
			s.cargo[k] = s.cargo.get(k, 0.0) + n


# ------------------------------------------------------------- markets

## A trade station (the Resources update, phase 4) buys and sells every
## resource at a price set by how much of it the market holds: the base
## price at MARKET_TARGET units, twice it when the market is bare, down to
## a third of it when it is glutted.
const MARKET_TARGET := 150.0
## Pilots buy this much over the middle price and sell this much under it.
const MARKET_SPREAD := 0.1
## A pilot of another empire pays the station's owner this share as well.
const MARKET_FEE := 0.1
## The wider market closes this share of the gap to MARKET_TARGET a second
## (about ten minutes to settle).
const MARKET_DRIFT := 1.0 / 600.0
## What the free port opens with, of every resource.
const FREE_PORT_STOCK := 160.0


static func is_market(s: Ship) -> bool:
	return s != null and s.design_name == "TradeStation" and not s.building and not s.dead


## Does `s` trade at `market` as a foreigner (and pay the owner's fee)?
static func pays_fee(market: Ship, s: Ship) -> bool:
	return market.race_id != GameData.NEUTRAL and market.race_id != s.race_id


## The middle price of a unit of `k` at a market.
static func market_price(market: Ship, k: String) -> float:
	var have: float = market.cargo.get(k, 0.0)
	return GameData.RESOURCE_PRICE.get(k, 1.0) * clampf(2.0 * MARKET_TARGET / (have + MARKET_TARGET), 0.35, 2.0)


## What `s` pays for a unit of `k` here, fee included.
static func buy_price(market: Ship, k: String, s: Ship) -> float:
	return market_price(market, k) * (1.0 + MARKET_SPREAD) * (1.0 + (MARKET_FEE if pays_fee(market, s) else 0.0))


## What `s` gets for a unit of `k` here, after the fee.
static func sell_price(market: Ship, k: String, s: Ship) -> float:
	return market_price(market, k) * (1.0 - MARKET_SPREAD) * (1.0 - (MARKET_FEE if pays_fee(market, s) else 0.0))


## The owner's cut of a trade worth `value` at the middle price.
static func fee_on(market: Ship, s: Ship, value: float) -> float:
	return value * MARKET_FEE if pays_fee(market, s) else 0.0


## The wider market evens the stock out over `dt` seconds.
static func market_step(market: Ship, dt: float) -> void:
	var f := minf(MARKET_DRIFT * dt, 1.0)
	for k in RESOURCES:
		var have: float = market.cargo.get(k, 0.0)
		market.cargo[k] = have + (MARKET_TARGET - have) * f


## An empire's worlds and what they hold: [{planet, stock, rates}], the
## biggest stockpile first.
func holdings(race: int) -> Array:
	var rows := []
	for p in world.planets:
		if p.is_colonised() and p.owner_race == race:
			rows.append({"planet": p, "stock": p.stock, "rates": production(p)})
	rows.sort_custom(func(a, b): return _total(a["stock"]) > _total(b["stock"]))
	return rows


func totals(race: int) -> Dictionary:
	var t := empty_stock()
	var r := empty_stock()
	for row in holdings(race):
		for k in RESOURCES:
			t[k] += row["stock"].get(k, 0.0)
			r[k] += row["rates"].get(k, 0.0)
	return {"stock": t, "rates": r}


static func _total(stock: Dictionary) -> float:
	var n := 0.0
	for k in stock:
		n += stock[k]
	return n


## "Food 5  Metals 3  Ice 3": the resources a world yields (richness out
## of 5), richest first.
static func yields_text(rich: Dictionary) -> String:
	var parts := []
	for k in RAW:
		if rich.get(k, 0) > 0:
			parts.append([k, rich[k]])
	if parts.is_empty():
		return "nothing"
	parts.sort_custom(func(a, b): return a[1] > b[1])
	var s := []
	for p in parts:
		s.append("%s %d" % [NAMES[p[0]], p[1]])
	return "  ".join(s)


## Stockpiles on the wire: one row of six numbers per planet.
func pack() -> Array:
	var out := []
	for p in world.planets:
		var row := PackedFloat32Array()
		for k in RESOURCES:
			row.append(p.stock.get(k, 0.0))
		out.append(row)
	return out


func unpack(rows: Array) -> void:
	for i in mini(rows.size(), world.planets.size()):
		var row: PackedFloat32Array = rows[i]
		var p: Planet = world.planets[i]
		for j in mini(row.size(), RESOURCES.size()):
			p.stock[RESOURCES[j]] = row[j]
