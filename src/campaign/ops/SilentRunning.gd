## Operation 9, Silent Running: the Hammer lies in a Drengin shipyard over
## Mars inside a net of pickets that listen with passive arrays only.  The
## pilots run silent through the net (jamming a picket that comes too
## close), hold within scanning range of the Hammer until the scan is done,
## and slip back out.  A picket that hears a pilot for long raises the
## alarm, and the operation is over.
extends StoryOp

const YARD_OFF := 2600.0          # the yard's height over Mars's surface, on the side away from the starbase
const NET_R := 2700.0             # the pickets circle the yard this far out
const NET_SPEED := 0.05           # rad / s round the yard
const NET_WOBBLE := 700.0         # in and out of the ring
## Passive arrays only: they hear drive heat and emissions, which silent
## running and a light hand on the throttle keep down.
const PICKET_SENSORS := {"infrared": 2400.0, "em": 3000.0, "optics": 450.0}
const WATCH_RANGE := 6000.0       # from the yard: the pickets mind pilots this close
const ALERT_TIME := 2.5           # seconds of a picket hearing someone before it raises the alarm
const EDGE := 7000.0              # from the yard: the edge of the net
const SCAN_REACH := 900.0         # from the Hammer's hull
const SCAN_TIME := 6.0
const EXIT := 8000.0              # every pilot this far from the yard: out

var mars: Planet
var yard: Ship
var hammer: Ship
var hammer_off := Vector2.ZERO
var pickets: Array = []
var alert := 0.0
var scanned := 0.0


func setup() -> void:
	mars = planet("Mars")
	yard = hammer_yard(YARD_OFF)
	hammer = spawn("Kesh Varr's yard crew", 2, "Dreadnought", "Hammer of Mars", yard.global_position, mars.velocity)
	hammer_off = (yard.global_position - mars.global_position).normalized().orthogonal() * (yard.size + hammer.size + 80.0)
	hold_in_cradle(hammer, yard, hammer_off)
	var n := scaled(4)
	for i in n:
		var c := spawn(Names.fill("{drengin} picket"), 2, "Scout", "Varr Listener %d" % (i + 1), yard.global_position, mars.velocity)
		c.set_meta("phase", TAU * i / n)
		c.sensors.suite = PICKET_SENSORS.duplicate()
		c.damaged.connect(func(hit, _a):
			# a pilot's shot raises the alarm (not a Terran world's guns on Deimos)
			if state == "running" and is_pilot(hit.last_attacker):
				fail_objective("unheard")
				fail("You opened fire inside the net and raised the alarm.", "sr_fail_seen"))
		pickets.append(c)
	_patrol(0.0)
	say("sr_start")
	objective("approach", "Fly to Mars, running silent (%s) once you near the picket net" % Keybinds.short_text("silent_running"), yard, Names.fill("{drengin} shipyard"))
	objective("unheard", "Stay off the pickets' arrays: coast, run silent, jam one that comes close (%s)" % Keybinds.short_text("cloak"), null, "", true)
	go("approach")


func step(delta: float) -> void:
	hold_in_cradle(hammer, yard, hammer_off)
	_patrol(delta)
	if stage != "approach" and _heard(delta):
		return
	match stage:
		"approach":
			if pilot_near(yard.global_position, EDGE) != null:
				complete("approach")
				say("sr_net")
				objective("scan", "Hold within %d of the Hammer until the scan is done" % int(SCAN_REACH), hammer, "The Hammer")
				go("net")
		"net":
			if pilot_near(hammer.global_position, SCAN_REACH + hammer.size) != null:
				scanned += delta
				say_once("sr_scan")
				set_text("scan", "Scanning the Hammer: %d%%" % int(minf(scanned / SCAN_TIME, 1.0) * 100.0))
				if scanned >= SCAN_TIME:
					complete("scan")
					say("sr_seen")
					say("sr_seen2")
					say("sr_out")
					flag("hammer_scan", true)
					objective("out", "Slip back out of the net (%d from the yard)" % int(EXIT), null, "", false)
					go("out")
			elif scanned > 0.0:
				set_text("scan", "Back within %d of the Hammer to finish the scan (%d%%)" % [int(SCAN_REACH), int(scanned / SCAN_TIME * 100.0)])
		"out":
			if pilot_near(yard.global_position, EXIT) == null:
				complete("out")
				complete("unheard")
				say("sr_done")
				pay(600)
				win()


## Is a picket hearing anyone?  Too long, and the alarm goes up.
func _heard(delta: float) -> bool:
	var heard := false
	for s in pilots():
		if not alive(s) or s.global_position.distance_to(yard.global_position) > WATCH_RANGE:
			continue
		for c in pickets:
			if alive(c) and Sensors.own(c, s)[1] >= 2:
				heard = true
				break
		if heard:
			break
	if heard:
		alert += delta
		say_once("sr_warn")
		set_text("unheard", "A picket hears you! Go quiet or jam it (%d%%)" % int(minf(alert / ALERT_TIME, 1.0) * 100.0))
	elif alert > 0.0:
		alert = maxf(alert - delta * 0.5, 0.0)
		if alert == 0.0:
			set_text("unheard", "Stay off the pickets' arrays: coast, run silent, jam one that comes close (%s)" % Keybinds.short_text("cloak"))
	if alert >= ALERT_TIME:
		fail_objective("unheard")
		fail("A picket heard you and the net raised the alarm.", "sr_fail_seen")
		return true
	return false


## The pickets drift round the yard on their ring, in and out (the script
## flies them, so they never break off to chase).
func _patrol(delta: float) -> void:
	for c in pickets:
		if not alive(c):
			continue
		var ph: float = c.get_meta("phase") + NET_SPEED * delta
		c.set_meta("phase", ph)
		var r := NET_R + NET_WOBBLE * sin(ph * 3.0)
		var at := yard.global_position + Vector2(r, 0).rotated(ph)
		var tangent := Vector2(0, r * NET_SPEED).rotated(ph)
		c.global_position = at
		c.velocity = yard.velocity + tangent
		c.body.pos = at
		c.body.vel = c.velocity
		c.rotation = tangent.angle()


## Someone fired: the net knows.
func on_breach(attacker: Node, victim: Node) -> void:
	if is_pilot(attacker) and victim is Ship and victim.race_id == 0:
		fail(Names.fill("You fired on an {arcean} ship."), "breach_warn_arcean")
