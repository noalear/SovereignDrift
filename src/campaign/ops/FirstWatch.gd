## Operation 1, First Watch: escort the supply ship Cormorant from Earth
## out to the free port at Jupiter's Trojan point with Lt. Brandt, while a
## Drengin scout shadows the flight.  The Accord holds: firing first on
## the scout ends the operation.
extends StoryOp

const MEET_REACH := 900.0         # how close "with the Cormorant" is
const SHADOW_OFF := 1500.0        # the scout keeps this far off the convoy's flank
const ARRIVE_REACH := 700.0       # from the free port
const HOME_REACH := 900.0         # from the starbase

var cormorant: Ship
var brandt: Ship
var scout: Ship
var start_dist := 1.0
var port: Ship                    # the free port
var dest: Node2D                  # the Cormorant's autopilot target, riding on the port


func setup() -> void:
	var base: Ship = world.starbase_of(1)
	var earth := planet("Earth")
	var from: Vector2 = base.global_position if base != null else earth.global_position
	port = world.spawn_free_port()
	dest = waypoint("the free port", port, 140.0)
	var dir := (port.global_position - from).angle() + 0.9
	var at := clear_spot(from, dir, 700.0)
	var vel: Vector2 = base.velocity if base != null else earth.velocity
	cormorant = spawn("Captain Ruiz", 1, "Freighter", "TSS Cormorant", at, vel)
	cormorant.rotation = (port.global_position - at).angle()
	cormorant.cargo = {"fuel": 120.0}
	cormorant._trim_cargo()
	cormorant.died.connect(_on_cormorant_died)
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", at + Vector2(0, 160).rotated(dir), vel, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = cormorant
	ai.state = AIPilot.State.ESCORT
	brandt.died.connect(_on_brandt_died)
	say("fw_start")
	objective("meet", "Meet the Cormorant and Lt. Brandt by the Terran starbase", cormorant, "TSS Cormorant")
	go("meet")


func step(_delta: float) -> void:
	match stage:
		"meet":
			var met := pilot_near(cormorant.global_position, MEET_REACH)
			if met != null:
				complete("meet")
				say("fw_formed")
				say("fw_cormorant")
				objective("escort", "Escort the Cormorant to the free port at Jupiter's Trojan point", cormorant, "TSS Cormorant")
				objective("hold_fire", "Hold your fire on Accord ships", null, "", true)
				cormorant.nav_target = dest
				cormorant.autopilot = true
				start_dist = maxf(cormorant.global_position.distance_to(port.global_position), 1.0)
				var ai: AIPilot = brandt.get_node_or_null("AI") if alive(brandt) else null
				if ai != null:
					ai.escort = met
				go("escort")
		"escort", "shadowed":
			_hold_slow()
			if not cormorant.autopilot and cormorant.global_position.distance_to(port.global_position) > ARRIVE_REACH:
				cormorant.autopilot = true       # knocked off course: on again
			var left := cormorant.global_position.distance_to(port.global_position)
			if stage == "escort" and left < start_dist * 0.65 and stage_time > 6.0:
				_bring_scout()
				go("shadowed")
			if stage == "shadowed":
				_shadow()
				if stage_time > 9.0:
					say_once("fw_banter")
			if left < ARRIVE_REACH:
				complete("escort")
				cormorant.autopilot = false
				if not alive(scout):
					_bring_scout()
				say("fw_arrived")
				say("fw_jam")
				say("fw_leave")
				go("jam")
		"jam":
			_hold_slow()
			if alive(scout):
				# a sweep of the jammer, then off toward Mars
				var mars := planet("Mars")
				var ai: AIPilot = scout.get_node_or_null("AI")
				if ai != null:
					ai.patrol_point = mars.global_position
				if stage_time < 6.0:
					for s in pilots():
						if alive(s):
							_jam(s)
				var lead := lead_pilot()
				if stage_time > 12.0 or (alive(lead) and scout.global_position.distance_to(lead.global_position) > 5000.0):
					world.remove_ship(scout)
					scout = null
			if stage_time > 4.0:
				objective("home", "Return to the Terran starbase", world.starbase_of(1), "Terran starbase")
				go("home")
		"home":
			if alive(scout) and stage_time > 10.0:
				world.remove_ship(scout)
				scout = null
			var base: Ship = world.starbase_of(1)
			if base != null and pilot_near(base.global_position, HOME_REACH) != null:
				complete("home")
				complete("hold_fire")
				say("fw_done")
				say("fw_done2")
				pay(300)
				win()


## The Cormorant never warps ahead of her escort.
func _hold_slow() -> void:
	if alive(cormorant):
		cormorant.warp_block = 999.0
		if cormorant.warping:
			cormorant.end_warp()


func _bring_scout() -> void:
	if alive(scout):
		return
	var side := Vector2.RIGHT.rotated(cormorant.rotation).orthogonal()
	var at := cormorant.global_position + side * (SHADOW_OFF + 600.0) - Vector2.RIGHT.rotated(cormorant.rotation) * 400.0
	scout = spawn("Drengin watcher", 2, "Scout", "Vigil of Mars", at, cormorant.velocity, 4)
	scout.rotation = cormorant.rotation
	scout.reset_physics_interpolation()
	scout.can_jam = true
	say("fw_scout")
	say("fw_hold")
	objective("scout", "Let the Drengin scout look. Do not fire first", scout, "Drengin scout", true)


## The scout keeps pace off the flank: close enough to be seen, never
## closing to fight.
func _shadow() -> void:
	if not alive(scout) or not alive(cormorant):
		return
	var ai: AIPilot = scout.get_node_or_null("AI")
	if ai == null:
		return
	var side := Vector2.RIGHT.rotated(cormorant.rotation).orthogonal()
	ai.patrol_point = cormorant.global_position + side * SHADOW_OFF + cormorant.velocity * 1.5
	if ai.state != AIPilot.State.PATROL and scout.target == null:
		ai.state = AIPilot.State.PATROL


## The scout's jammer sweeps the flight as it leaves.
func _jam(p: Ship) -> void:
	var ps: Sensors = p.sensors
	if not ps.is_jammed():
		ps._own.clear()
		ps._seen.clear()
		world.notify(p, "Sensors jammed by %s." % scout.pilot_name)
	ps.jammed_until = ps.now() + 0.3
	ps.jammed_by = scout


func _on_brandt_died(s: Ship, _killer: Node) -> void:
	if not s.captured:
		say("brandt_down")


func _on_cormorant_died(_s: Ship, _killer: Node) -> void:
	fail("The Cormorant was lost.", "fw_fail_cargo")


func on_breach(attacker: Node, victim: Node) -> void:
	if is_pilot(attacker) and (victim == scout or (victim is Ship and victim.race_id == 2) or (victim is Planet and victim.owner_race != 1)):
		fail_objective("hold_fire")
		fail("You fired first on an Accord ship.", "fw_fail_breach")
