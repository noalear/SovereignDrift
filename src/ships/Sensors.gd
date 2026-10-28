## Sensors -- what a ship gives off and what it can see (Kyle's sensors
## and stealth design, 2026-10-28).  Every ship has four signatures:
## "radar" (the return off its hull: size and armour coat), "heat"
## (reactor, engines, boosters, the jammer and every shot), "em" (what it
## transmits: active radar and lidar, pings, laser fire, raised shields)
## and "temporal" (the ripple a quantum jump sends out, like a sonic boom).
## Every sensor reads one channel (GameData.SENSOR_FAMILIES) and sees a
## target out to rated range x signature / 100.  What a viewer knows of a
## target is a grade: 3 a track (a lock: guns aim, missiles home, the AI
## attacks), 2 a contact (somewhere about there, no lock), 1 a bearing
## only, 0 nothing.  Allies share what they see by data link.
##
## One per ship (Ship.sensors).  The balance lives in the constants.
class_name Sensors
extends RefCounted

const CHANNELS := ["radar", "heat", "em", "temporal"]

# ------------------------------------------------------------- balance

## A tracking sensor gives a contact out to this many times its reach.
const CONTACT_REACH := 1.6
## EM receivers and temporal sensors give a bearing out to this many times.
const BEARING_REACH := 2.0
## A track is held this long after the target slips out, so locks don't flicker.
const TRACK_GRACE := 0.5
## A lost contact is remembered (drawn as a ghost) this long.
const GHOST_TIME := 8.0
## What a viewer knows of a target is worked out at most this often.
const CACHE_TIME := 0.2
## Allies share tracks and contacts within this range; sensor stations
## share theirs with the whole empire.
const LINK_RANGE := 4000.0
## Infrared reach on a ship inside the Sun's heat band (its glare).
const SUN_GLARE := 0.5
## How far off a contact is drawn, as a share of its distance.
const CONTACT_ERROR := 0.12

## Radar return of a hull of size 44 (a Battleship) with plain plate.
const RADAR_PER_SIZE := 100.0 / 44.0
## Heat: life support and the idle reactor (x sqrt(size / 44)), the
## reactor's load, the engines at full burn (a Scout's; heavier hulls run
## hotter, see _thrust_heat), the booster, a running jammer.
const IDLE_HEAT := 20.0
const REACTOR_HEAT := 25.0
const THRUST_HEAT := 45.0
const BOOST_HEAT := 60.0
const JAM_HEAT := 40.0
## A heat sink soaks this share of the heat until it is full, then vents
## all of it in a flare; it drains while the ship coasts below SINK_COOL.
const SINK_SOAK := 0.7
const SINK_VENT := 200.0
const SINK_VENT_TIME := 2.0
const SINK_DRAIN := 50.0
const SINK_COOL := 30.0
## Emissions: shields raised, shields recharging (instead), a quantum
## drive charging, and an active sensor: heard at twice its rated range by
## an EM receiver I (rated 1500).
const SHIELD_EM := 20.0
const RECHARGE_EM := 40.0
const QDRV_CHARGE_EM := 60.0
const EM_PER_RANGE := 2.0 / 15.0
## Lidar is heard only inside its beam, at 1.5x its range.
const LIDAR_EM_SHARE := 0.75
## Temporal: a drive charging, a ship in the jump, the ripple at either end.
const CHARGE_TEMPORAL := 150.0
const JUMP_TEMPORAL := 400.0
const RIPPLE_TIME := 1.5
## The jammer: a jammed ship's sensors keep this share of their range
## (lidar's narrow beam keeps more), the jam bites after JAM_BITE seconds
## on a target, and costs JAM_DRAIN energy a second.
const JAM_FACTOR := 0.4
const JAM_FACTOR_LIDAR := 0.7
const JAM_BITE := 1.0
const JAM_DRAIN := 55.0
## A ping tracks what it finds for PING_HOLD seconds, costs PING_COST
## energy and is heard for PING_HEARD seconds.
const PING_HOLD := 4.0
const PING_COST := 150.0
const PING_HEARD := 3.0
## Silent running: the reactor at this share, and radars take this long
## to warm up again afterwards.
const EMCON_REGEN := 0.5
const EMCON_WAKE := 1.0

## What a shot gives away, per missile id: [heat, em] for SHOT_TIME
## seconds (the weapons thread tunes these); a beam while it fires.
const SHOT_SIGNATURE := {
	"001": [40.0, 0.0], "010": [40.0, 0.0], "011": [40.0, 0.0], "018": [80.0, 0.0], "004": [50.0, 0.0],
	"002": [10.0, 0.0], "008": [30.0, 0.0], "009": [30.0, 0.0], "021": [30.0, 0.0], "020": [30.0, 40.0],
}
const SHOT_TIME := 2.0
const BEAM_SIGNATURE := [10.0, 60.0]

const _TRACK_BITS := {"radar": 1, "heat": 2}

var ship: Node = null
var suite := {}               # family -> rated range in px, as fitted
var efficiency := 1.0         # the sensors system's health (blind sensors see less)
var heat := 0.0               # the steady heat, lagging the burn by the armour's thermal mass
var heat_raw := 0.0           # what the heat is heading for
var sink_store := 0.0         # heat-seconds in the heat sink
var spikes := {}              # channel -> [[amount, seconds left, seconds]]
var emcon := false            # silent running
var emcon_wake := 0.0         # radars warming up after silent running
var ping_ready := 0.0         # seconds until the pulse radar can ping again
var pinged := {}              # target instance id -> time the ping's track runs out
var jam_target: Node = null   # who our jammer is on
var jam_bite := 0.0           # seconds on that target
var jam_out := 0.0            # seconds the jammer stays knocked out (a disruptor hit)
var jammed_until := -1.0      # we are jammed until this time
var jammed_by: Node = null
var net_sig := []             # online clients: the host's signatures for this ship
var _own := {}                # target id -> [time, grade, active, last tracked, track bits]
var _seen := {}               # target id -> [time, grade]
var last_seen := {}           # target id -> [position, velocity, time] (for ghosts)


func setup(s: Node) -> void:
	ship = s


## Read the fit off the ship's stats after a refit.
func fit(stats: Dictionary) -> void:
	suite = stats.get("sensors", {}).duplicate()
	if not suite.has("optics"):
		suite["optics"] = GameData.SENSOR_FAMILIES["optics"]["rated"]
	_own.clear()
	_seen.clear()
	pinged.clear()
	jam_target = null
	jam_bite = 0.0
	sink_store = 0.0


func reset() -> void:
	spikes.clear()
	heat = 0.0
	heat_raw = 0.0
	sink_store = 0.0
	emcon = false
	emcon_wake = 0.0
	ping_ready = 0.0
	jam_out = 0.0
	jammed_until = -1.0
	jammed_by = null
	fit(ship.stats)


func now() -> float:
	if ship != null and ship.world != null and "elapsed" in ship.world:
		return ship.world.elapsed
	return Time.get_ticks_msec() / 1000.0


func has(family: String) -> bool:
	return suite.has(family)


## A family's working range against a signature of 100: damage and a
## jammer on us cut it.
func rated(family: String, at: float = -1.0) -> float:
	var r: float = suite.get(family, 0.0) * efficiency
	if is_jammed(at):
		r *= JAM_FACTOR_LIDAR if family == "lidar" else JAM_FACTOR
	return r


## Radar and lidar are off in silent running, and while they warm up.
func active_on() -> bool:
	return not emcon and emcon_wake <= 0.0


func is_jammed(at: float = -1.0) -> bool:
	return jammed_until > (now() if at < 0.0 else at)


# --------------------------------------------------------- signatures

## How loud this ship is on a channel.  `toward` is the viewer's position
## (lidar is heard only inside its beam).
func signature(channel: String, toward: Vector2 = Vector2.INF) -> float:
	if ship.landing == Ship.Landing.LANDED or ship.dead:
		return 0.0
	if ship.is_puppet and net_sig.size() == 4 and channel != "radar":
		return net_sig[CHANNELS.find(channel)]
	var v := 0.0
	match channel:
		"radar":
			v = radar_return()
		"heat":
			v = heat
		"em":
			v = _em(toward)
		"temporal":
			v = _temporal()
	return v + spike(channel)


func radar_return() -> float:
	return ship.size * RADAR_PER_SIZE * float(ship.stats.get("armor_sig", 1.0))


## The loudest spike still dying away on a channel (a volley is as loud as
## its loudest shot, so beams can refresh theirs every frame).
func spike(channel: String) -> float:
	var best := 0.0
	for sp in spikes.get(channel, []):
		best = maxf(best, sp[0] * sp[1] / maxf(sp[2], 0.001))
	return best


func add_spike(channel: String, amount: float, seconds: float) -> void:
	if amount <= 0.0 or not channel in CHANNELS:
		return
	if not spikes.has(channel):
		spikes[channel] = []
	spikes[channel].append([amount, maxf(seconds, 0.02), maxf(seconds, 0.02)])


func _em(toward: Vector2) -> float:
	var e := 0.0
	if ship.shield_max > 0.0 and ship.get("shields_raised") != false:
		e += RECHARGE_EM if ship.shield < ship.shield_max - 0.5 else SHIELD_EM
	if active_on():
		if suite.has("radar"):
			e += suite["radar"] * EM_PER_RANGE
		if suite.has("lidar") and toward != Vector2.INF and in_cone(toward, "lidar"):
			e += suite["lidar"] * EM_PER_RANGE * LIDAR_EM_SHARE
	if ship.warping and ship.warp_charge < Ship.WARP_CHARGE_TIME:
		e += QDRV_CHARGE_EM
	return e


func _temporal() -> float:
	if not ship.warping:
		return 0.0
	return CHARGE_TEMPORAL if ship.warp_charge < Ship.WARP_CHARGE_TIME else JUMP_TEMPORAL


## Is a point inside a beam family's cone off the nose?
func in_cone(pos: Vector2, family: String) -> bool:
	var cone: float = GameData.SENSOR_FAMILIES[family]["cone"]
	if cone <= 0.0:
		return true
	var rel: Vector2 = pos - ship.global_position
	return absf(wrapf(rel.angle() - ship.rotation, -PI, PI)) <= cone


## Engine heat at full burn: a Scout's THRUST_HEAT, more for heavier
## hulls (armour weight burns hotter as well as slower).
func _thrust_heat() -> float:
	return THRUST_HEAT * clampf(pow(ship.wet_mass() / 4000.0, 0.25), 1.0, 2.2)


## What a design gives off, for the shipyard: radar return, heat coasting
## and at full burn (before a heat sink), and emissions with shields up.
static func fit_signatures(design: Dictionary, stats: Dictionary) -> Dictionary:
	var size: float = float(design.get("size", 22.0))
	var radar: float = size * RADAR_PER_SIZE * float(stats.get("armor_sig", 1.0))
	var heat: float = IDLE_HEAT * sqrt(size / 44.0)
	var gen: float = stats.get("power_gen", 0.0)
	if gen > 0.0:
		heat += REACTOR_HEAT * clampf(stats.get("power_draw", 0.0) / gen, 0.0, 1.0)
	var burn: float = heat + THRUST_HEAT * clampf(pow(float(stats.get("mass", 0.0)) / 4000.0, 0.25), 1.0, 2.2)
	var em := 0.0
	if stats.get("shield_max", 0.0) > 0.0:
		em += SHIELD_EM
	em += float(stats.get("sensors", {}).get("radar", 0.0)) * EM_PER_RANGE
	return {"radar": radar, "heat": heat, "burn": burn, "em": em}


# --------------------------------------------------------------- tick

func tick(delta: float) -> void:
	for ch in spikes.keys():
		var list: Array = spikes[ch]
		for sp in list:
			sp[1] -= delta
		spikes[ch] = list.filter(func(sp): return sp[1] > 0.0)
	ping_ready = maxf(ping_ready - delta, 0.0)
	emcon_wake = maxf(emcon_wake - delta, 0.0)
	jam_out = maxf(jam_out - delta, 0.0)
	# the steady heat, soaked by a heat sink, lagging by thermal mass
	var raw := IDLE_HEAT * sqrt(ship.size / 44.0)
	if not emcon and ship.power_gen > 0.0:
		raw += REACTOR_HEAT * clampf(ship.power_draw / ship.power_gen, 0.0, 1.0)
	raw += _thrust_heat() * clampf(ship.engine_level, 0.0, 1.0)
	if ship.boosting:
		raw += BOOST_HEAT
	if ship.jamming:
		raw += JAM_HEAT
	var cap: float = ship.stats.get("heat_sink", 0.0)
	if cap > 0.0:
		if raw > SINK_COOL:
			var soak := raw * SINK_SOAK
			raw -= soak
			sink_store += soak * delta
			if sink_store >= cap:
				sink_store = 0.0
				add_spike("heat", SINK_VENT, SINK_VENT_TIME)
				if ship.is_player and ship.world != null:
					ship.world.notify(ship, "Heat sink full: venting.")
		else:
			sink_store = maxf(sink_store - SINK_DRAIN * delta, 0.0)
	heat_raw = raw
	var lag: float = maxf(float(ship.stats.get("thermal_lag", 1.0)), 0.05)
	heat += (raw - heat) * (1.0 - exp(-delta / lag))
	_jam_tick(delta)


# ---------------------------------------------------------- detection

## What this viewer's own sensors make of a target:
## [time, grade, active, last tracked, track bits].
static func own(v: Node, t: Node) -> Array:
	var sv: Sensors = v.sensors
	var at := sv.now()
	var id := t.get_instance_id()
	var c: Array = sv._own.get(id, [])
	if not c.is_empty() and at - c[0] < CACHE_TIME:
		return c
	var grade := 0
	var active := false
	var bits := 0
	var last_track: float = c[3] if not c.is_empty() else -100.0
	if not t.dead and t.landing != Ship.Landing.LANDED and not v.dead:
		var d: float = v.global_position.distance_to(t.global_position)
		var los := -1
		var glare := -1
		for fam in sv.suite:
			var info: Dictionary = GameData.SENSOR_FAMILIES[fam]
			if info["active"] and fam != "pulse" and not sv.active_on():
				continue
			if fam == "pulse":
				if sv.pinged.get(id, -1.0) > at:
					grade = 3
					active = true
					bits |= _TRACK_BITS["radar"]
				continue
			var r := sv.rated(fam, at)
			if r <= 0.0:
				continue
			if info["cone"] > 0.0 and not sv.in_cone(t.global_position, fam):
				continue
			var ch: String = info["channel"]
			var sig: float = t.sensors.signature(ch, v.global_position)
			if sig <= 0.0:
				continue
			if ch == "heat":
				if glare < 0:
					glare = 1 if v.world != null and v.world.has_method("in_sun_glare") and v.world.in_sun_glare(t.global_position) else 0
				if glare == 1:
					sig *= SUN_GLARE
			r *= sig / 100.0
			var reach := r * (CONTACT_REACH if info["track"] else BEARING_REACH)
			if d > reach:
				continue
			if info["los"]:
				if los < 0:
					los = 1 if v.world == null or not v.world.has_method("line_clear") or v.world.line_clear(v.global_position, t.global_position) else 0
				if los == 0:
					continue
			if info["track"]:
				if d <= r:
					grade = 3
					bits |= _TRACK_BITS.get(ch, 0)
					if info["active"]:
						active = true
				else:
					grade = maxi(grade, 2)
			else:
				grade = maxi(grade, 2 if d <= r else 1)
	if grade == 3:
		last_track = at
	elif at - last_track < TRACK_GRACE and not t.dead and t.landing != Ship.Landing.LANDED:
		grade = 3
		bits = c[4] if not c.is_empty() else bits
	c = [at, grade, active, last_track, bits]
	sv._own[id] = c
	return c


## What a viewer knows of a target, with its allies' data link: 0 nothing,
## 1 a bearing, 2 a contact, 3 a track.
static func quality(v: Node, t: Node) -> int:
	if v == t:
		return 3
	if t == null or t.dead or t.landing == Ship.Landing.LANDED:
		return 0
	var sv: Sensors = v.sensors
	var at := sv.now()
	var id := t.get_instance_id()
	var c: Array = sv._seen.get(id, [])
	if not c.is_empty() and at - c[0] < CACHE_TIME:
		return c[1]
	var grade: int = own(v, t)[1]
	if grade < 3 and not sv.is_jammed(at) and v.world != null and v.world.has_method("sensor_links"):
		for a in v.world.sensor_links(v.team()):
			if a == v or a == t or a.dead or a.sensors.is_jammed(at):
				continue
			if a.design_name != "SensorStation" and a.global_position.distance_squared_to(v.global_position) > LINK_RANGE * LINK_RANGE:
				continue
			grade = maxi(grade, own(a, t)[1])
			if grade == 3:
				break
	if grade > 0:
		sv.last_seen[id] = [t.global_position, t.velocity, at]
	sv._seen[id] = [at, grade]
	return grade


## Does the viewer hold a lock, with any sensor (channel "") or with its
## own sensors on one channel ("radar" or "heat")?
static func track(v: Node, t: Node, channel: String = "") -> bool:
	if channel == "":
		return quality(v, t) >= 3
	if v == t:
		return true
	return (int(own(v, t)[4]) & int(_TRACK_BITS.get(channel, 0))) != 0


## Where a contact seems to be: its true place for a track, off by a
## wandering error for a contact.
static func estimate(v: Node, t: Node) -> Vector2:
	var q := quality(v, t)
	if q >= 3 or q == 0:
		return t.global_position
	var d: float = v.global_position.distance_to(t.global_position)
	var sv: Sensors = v.sensors
	var k := float(t.get_instance_id() % 997)
	var ph := sv.now() * 0.35 + k
	var off := Vector2(sin(ph * 1.3 + k), cos(ph * 0.9 - k)) * d * CONTACT_ERROR
	return t.global_position + off


## Enemies whose active sensors hold a track on this ship: the warning
## receiver every hull carries.  [[ship, family kind ("radar"/"lidar"/"ping")]]
static func painters(s: Node) -> Array:
	var out := []
	if s.world == null:
		return out
	for o in s.world.ships:
		if o == s or o.dead or o.team() == s.team() or o.race_id == GameData.NEUTRAL or o.is_landing():
			continue
		if o.global_position.distance_squared_to(s.global_position) > 64000000.0:
			continue
		var c := own(o, s)
		if c[1] >= 3 and c[2]:
			var kind := "radar"
			if o.sensors.pinged.get(s.get_instance_id(), -1.0) > o.sensors.now():
				kind = "ping"
			elif o.sensors.has("lidar") and o.sensors.in_cone(s.global_position, "lidar") and not o.sensors.has("radar"):
				kind = "lidar"
			out.append([o, kind])
	return out


# ------------------------------------------------------- active tools

## Sweep with the pulse radar: everything it finds is tracked for
## PING_HOLD seconds, and every EM receiver around hears it.
func ping() -> String:
	if not suite.has("pulse"):
		return "No pulse radar fitted."
	if ping_ready > 0.0:
		return "Pulse radar recharging (%.0f s)." % ceilf(ping_ready)
	if ship.energy < PING_COST:
		return "Not enough energy to ping (needs %d)." % int(PING_COST)
	ship.energy -= PING_COST
	ping_ready = float(ship.stats.get("ping_cooldown", 7.0))
	var at := now()
	var r0 := rated("pulse", at)
	var found := 0
	if ship.world != null:
		for t in ship.world.ships:
			if t == ship or t.dead or t.landing == Ship.Landing.LANDED:
				continue
			var r: float = r0 * t.sensors.signature("radar") / 100.0
			if ship.global_position.distance_to(t.global_position) > r:
				continue
			if ship.world.has_method("line_clear") and not ship.world.line_clear(ship.global_position, t.global_position):
				continue
			pinged[t.get_instance_id()] = at + PING_HOLD
			_own.erase(t.get_instance_id())
			_seen.erase(t.get_instance_id())
			if t.team() != ship.team():
				found += 1
	add_spike("em", suite["pulse"] * EM_PER_RANGE, PING_HEARD)
	if ship.world != null and ship.world.get("fx") != null and ship.world.fx.has_method("sensor_ping"):
		ship.world.fx.sensor_ping(ship.global_position, r0, ship.race_color())
	Sfx.play("warpexit", ship.global_position, 0.5, 1.8)
	return "Ping: %d contact%s." % [found, "" if found == 1 else "s"]


func set_emcon(on: bool) -> void:
	if on == emcon:
		return
	emcon = on
	if not on:
		emcon_wake = EMCON_WAKE
	_own.clear()
	_seen.clear()


## Why the jammer cannot go on now, or "".
func jam_refusal() -> String:
	if not ship.can_jam:
		return "No jammer fitted." if not ship.stats.get("can_jam", false) else "Jammer offline."
	if jam_out > 0.0:
		return "Jammer knocked out (%.0f s)." % ceilf(jam_out)
	if ship.energy < 80.0:
		return "Not enough energy to jam."
	return ""


## The jammer works on the pilot's current target while it is in reach,
## in sight and was tracked when the jam went on it.
func _jam_tick(delta: float) -> void:
	if not ship.jamming:
		jam_target = null
		jam_bite = 0.0
		return
	if jam_out > 0.0 or not ship.can_jam:
		ship.set_jam(false)
		return
	var cost := JAM_DRAIN * delta
	if ship.energy < cost:
		ship.set_jam(false)
		return
	ship.energy -= cost
	var t: Node = ship.target
	var reach: float = ship.stats.get("jam_reach", 2500.0)
	var ok: bool = t is Ship and is_instance_valid(t) and not t.dead and t.team() != ship.team() and not t.is_landing() \
			and t.global_position.distance_to(ship.global_position) <= reach
	if ok and ship.world != null and ship.world.has_method("line_clear"):
		ok = ship.world.line_clear(ship.global_position, t.global_position)
	if ok and t != jam_target:
		ok = quality(ship, t) >= 3           # a new target must be locked first
		if ok:
			jam_target = t
			jam_bite = 0.0
	if not ok:
		if t != jam_target:
			jam_target = null
		jam_bite = 0.0
		return
	jam_bite += delta
	if jam_bite >= JAM_BITE:
		var ts: Sensors = t.sensors
		var was := ts.is_jammed()
		ts.jammed_until = now() + 0.3
		ts.jammed_by = ship
		if not was:
			ts._own.clear()
			ts._seen.clear()
			if t.is_player and t.world != null:
				t.world.notify(t, "Sensors jammed by %s." % ship.pilot_name)


## Is the jammer biting on someone right now?
func jam_active() -> bool:
	return ship.jamming and jam_target != null and is_instance_valid(jam_target) and jam_bite >= JAM_BITE
