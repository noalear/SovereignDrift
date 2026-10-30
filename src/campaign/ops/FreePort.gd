## Operation 3, Free Port: the raiders' transponder records are for sale
## at the Jupiter free port.  The player chooses: pay the broker Ilsa
## Marrow her Exotics, or copy the records off her data relay under silent
## running, unseen by port security (and the port charges Terrans more for
## the rest of the war).  No weapons at the port either way.
extends StoryOp

const MEET_REACH := 1100.0        # from the port: Marrow calls
const PAY_REACH := 450.0          # from the port's hull: alongside to hand over the Exotics
const PRICE := 8.0                # Exotics
const EXPENSES := 300             # credits Fleet puts in for the job
const RELAY_OFF := 900.0          # the relay sits this far behind the port
const RELAY_REACH := 220.0
const COPY_TIME := 5.0            # seconds on the relay
const PATROL_R := 650.0           # port security sweeps the port's busy side this far out
const PATROL_SWEEP := 1.4         # rad either side of the sunward line
const PATROL_SPEED := 0.25        # rad / s of the sweep's phase
## Port security watches with passive sensors only (they listen for drive
## heat and emissions): silent running gets under them, a loud ship does not.
const SECURITY_SENSORS := {"infrared": 2000.0, "em": 2500.0, "optics": 400.0}
const WATCH_RANGE := 1300.0       # security only minds ships this close to the relay (the port itself is busy)
const HOME_REACH := 900.0

var port: Ship
var relay: Node2D
var cutters: Array = []           # port security, flown round the port by the script
var sunward := Vector2.LEFT       # from the port: the side security watches
var copied := 0.0
var path := ""                    # pay | steal


func setup() -> void:
	port = world.spawn_free_port()
	for p in pilots():
		p.credits += EXPENSES
		world.notify(p, "Fleet expenses: %d credits." % EXPENSES)
	say("port_start")
	objective("port", "Fly to the free port at Jupiter's Trojan point", port, "Jupiter free port")
	go("travel")


func step(delta: float) -> void:
	_patrol(delta)
	match stage:
		"travel":
			if pilot_near(port.global_position, MEET_REACH) != null:
				complete("port")
				say("port_marrow_1")
				say("port_marrow_2")
				say("port_choice")
				ask("free_port", "Ilsa Marrow wants 8 Exotics for the transponder records.", [
					["pay", "Pay her in Exotics", "Buy 8 Exotics at the port's market and hand them over. The port stays friendly."],
					["steal", "Copy them under silent running", Names.fill("Free, but port security must not get a contact on you, and the port will charge {terrans} more for the rest of the war.")]])
				go("choose")
		"pay":
			var p := _payer()
			if p != null:
				p.cargo["exotics"] -= PRICE
				if p.cargo["exotics"] < 0.01:
					p.cargo.erase("exotics")
				world.notify(p, "Handed Ilsa Marrow %d Exotics." % int(PRICE))
				complete("pay")
				say("port_paid")
				_records()
		"steal":
			for p in pilots():
				if _shot_at(p):
					fail("You fired on port security.", "port_fail_shot")
					return
				if _spotted(p):
					fail("Port security spotted you.", "port_fail_seen")
					return
			if pilot_near(relay.global_position, RELAY_REACH) != null:
				copied += delta
				set_text("steal", "Copying the records: %d%%" % int(minf(copied / COPY_TIME, 1.0) * 100.0))
				if copied >= COPY_TIME:
					complete("steal")
					say("port_copied")
					flag("port_fees", "high")
					story.mark_up_port()
					_records()
			elif copied > 0.0 and copied < COPY_TIME:
				set_text("steal", "Hold on the relay behind the port until the copy is done (%d%%)" % int(copied / COPY_TIME * 100.0))
		"home":
			if path == "steal" and stage_time > 14.0:
				say_once("port_found")
			if pilot_near(home_base().global_position, HOME_REACH) != null:
				complete("home")
				say("port_done")
				pay(300)
				win()


func on_choice(id: String, key: String) -> void:
	if id != "free_port" or stage != "choose":
		return
	path = key
	if key == "pay":
		say("port_pay_go")
		objective("pay", "Buy %d Exotics at the port's market (%s alongside it), then stay alongside while Marrow takes them" % [int(PRICE), Keybinds.short_text("cargo")], port, "Jupiter free port")
		go("pay")
	else:
		say("port_steal_go")
		_bring_security()
		objective("steal", "Run silent (%s), slip round to the relay behind the port and hold on it until the copy is done" % Keybinds.short_text("silent_running"), relay, "Data relay")
		objective("unseen", "Stay off port security's sensors", null, "", true)
		go("steal")


## The relay behind the port (away from the Sun), and two security
## cutters circling the port.
func _bring_security() -> void:
	var sun: Node2D = world.sun
	sunward = (sun.global_position - port.global_position).normalized() if sun != null else Vector2.LEFT
	relay = waypoint("Data relay", port, 40.0, -sunward * RELAY_OFF)
	for i in 2:
		var c := spawn("Port security", GameData.NEUTRAL, "Scout", "Port cutter %d" % (i + 1), port.global_position, port.velocity)
		c.set_meta("phase", PI * i)
		c.sensors.suite = SECURITY_SENSORS.duplicate()
		cutters.append(c)
	_patrol(0.0)


## The cutters sweep to and fro across the port's sunward side, where the
## traffic comes in (the script moves them).
func _patrol(delta: float) -> void:
	for c in cutters:
		if not alive(c):
			continue
		var ph: float = c.get_meta("phase") + PATROL_SPEED * delta
		c.set_meta("phase", ph)
		var a := sunward.angle() + PATROL_SWEEP * sin(ph)
		var at := port.global_position + Vector2(PATROL_R, 0).rotated(a)
		var tangent := Vector2(0, PATROL_R * PATROL_SWEEP * PATROL_SPEED * cos(ph)).rotated(a)
		c.global_position = at
		c.velocity = port.velocity + tangent
		c.body.pos = at
		c.body.vel = c.velocity
		c.rotation = tangent.angle() if tangent.length() > 1.0 else a


## A pilot alongside the port with the Exotics in the hold.
func _payer() -> Ship:
	for p in pilots():
		if alive(p) and p.cargo.get("exotics", 0.0) >= PRICE and p.global_position.distance_to(port.global_position) - port.size < PAY_REACH:
			return p
	return null


func _shot_at(p: Ship) -> bool:
	for c in cutters:
		if not alive(c) or c.hull < c.hull_max - 0.5 or c.last_attacker == p:
			return true
	return false


## Port security has a contact on this pilot near the relay.
func _spotted(p: Ship) -> bool:
	if p.dead or p.global_position.distance_to(relay.global_position) > WATCH_RANGE:
		return false
	for c in cutters:
		if Sensors.quality(c, p) >= 2:
			return true
	return false


func _records() -> void:
	say("port_records_1")
	say("port_records_2")
	flag("records", "varr_shipping")
	objective("home", Names.fill("Bring the records home to the {terran} starbase"), home_base, Names.fill("{terran} starbase"))
	go("home")
	for c in cutters:
		if alive(c):
			world.remove_ship(c)
	cutters.clear()


func on_breach(attacker: Node, victim: Node) -> void:
	if is_pilot(attacker) and (victim is Ship and victim.race_id == 2 or victim is Planet and victim.owner_race != 1):
		fail("You fired first on an Accord ship.", "fw_fail_breach")
