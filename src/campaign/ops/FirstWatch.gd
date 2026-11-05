## Operation 1, First Watch: escort the supply ship Cormorant from Earth
## out to the free port at Jupiter's Trojan point with Lt. Brandt.  She
## jumps when her escort jumps.  Halfway a Drengin scout's jammer pulls the
## flight out of warp and it makes runs on her to read her hold: the pilots
## crowd it off without a shot, since the Accord holds and firing first on
## it ends the operation.  When it has gone, raiders in pirate colours hit
## her, and those are fair game.
extends StoryOp

const MEET_REACH := 900.0         # how close "with the Cormorant" is
const ARRIVE_REACH := 700.0       # from the free port
const HOME_REACH := 900.0         # from the starbase
const SPEED_UP := 1.5             # the Cormorant's drive, tuned for the trip (never past her slowest escort)
const CLEAR_OF_EARTH := 1500.0    # how far out she must be before she can jump
const JUMP_REACH := 2600.0        # a pilot this close who jumps takes her along
const JUMP_AIM := 0.7             # radians off her course a jump may point
const DROP_SHORT := 900.0         # she drops out this far short of the port
const JUMP_WAIT := 15.0           # seconds she waits for a lead before jumping alone
const SCOUT_AT := 0.45            # of the way out, the scout pulls the flight out of warp
const INTERDICT := 4500.0         # pilots this close to her drop out with her
const SHADOW_OFF := 1500.0        # the scout's watching distance off her flank
const RUNS := 3                   # runs on the Cormorant before the scout goes
const RUN_OFF := 260.0            # how close off her side a run comes
const RUN_TIME := 16.0            # a run nobody stops ends after this
const SHADOW_TIME := 5.0          # the pause between runs
const CROWD_REACH := 220.0        # a pilot this close to the scout (past both hulls) turns it away
const SCAN_REACH := 700.0         # the scout reads her hold this close to her
const SCAN_TIME := 18.0           # seconds of uncrowded runs to read all of it
const RAID_AFTER := 8.0           # seconds after the scout goes, the raiders come
const RAID_FROM := 1900.0
const CORMORANT_HULL := 2.0       # her hull, times a freighter's: she was armoured for this route

var cormorant: Ship
var brandt: Ship
var scout: Ship
var start_dist := 1.0
var port: Ship                    # the free port
var dest: Node2D                  # the Cormorant's autopilot target, riding on the port
var raiders: Array = []
var jumps_ok := false             # she jumps with her escort on this leg
var interdicted := false
var solo := false                 # her jump is her own, nobody leading it
var scan := 0.0                   # 0..1 of her hold the scout has read
var runs := 0
var scout_mode := ""              # shadow, run, off, leave
var scout_t := 0.0
var run_side := 1.0


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
	cormorant.hull_max *= CORMORANT_HULL
	cormorant.hull = cormorant.hull_max
	cormorant.set_meta("no_boarding", true)    # raiders shoot her; nobody takes her
	cormorant.died.connect(_on_cormorant_died)
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", at + Vector2(0, 160).rotated(dir), vel, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = cormorant
	ai.state = AIPilot.State.ESCORT
	brandt.died.connect(_on_brandt_died)
	say("fw_start")
	objective("meet", Names.fill("Meet the Cormorant and Lt. Brandt by the {terran} starbase"), cormorant, "TSS Cormorant")
	go("meet")


func step(delta: float) -> void:
	_hold_slow()
	match stage:
		"meet":
			var met := pilot_near(cormorant.global_position, MEET_REACH)
			if met != null:
				complete("meet")
				say("fw_formed")
				say("fw_cormorant")
				objective("escort", "Escort the Cormorant to the free port at Jupiter's Trojan point", cormorant, "TSS Cormorant")
				objective("hold_fire", "Hold your fire on Accord ships", null, "", true)
				_speed_up()
				cormorant.nav_target = dest
				cormorant.autopilot = true
				start_dist = maxf(cormorant.global_position.distance_to(port.global_position), 1.0)
				var ai: AIPilot = brandt.get_node_or_null("AI") if alive(brandt) else null
				if ai != null:
					ai.escort = met
				go("leave_earth")
		"leave_earth":
			_fly_on()
			var earth := planet("Earth")
			if cormorant.global_position.distance_to(earth.global_position) - earth.radius > CLEAR_OF_EARTH:
				jumps_ok = true
				say("fw_jump")
				objective("jump", "Jump toward the free port: the Cormorant jumps with you", cormorant, "TSS Cormorant", true)
				go("escort")
		"escort":
			_fly_on()
			_follow_jump(delta)
			if _progress() > SCOUT_AT:
				complete("jump")
				_interdict()
				go("shadowed")
		"shadowed":
			_fly_on()
			_scout_runs(delta)
			if scout_mode == "leave" and scout_t > RAID_AFTER:
				_raid()
				go("raid")
		"raid":
			_fly_on()
			_scout_away(delta)
			if cormorant.hull < cormorant.hull_max * 0.5:
				say_once("fw_hurt")
			if cormorant.global_position.distance_to(port.global_position) < ARRIVE_REACH:
				# under the port's eyes they give up the chase
				for r in raiders:
					if alive(r) and not r.is_disabled():
						world.remove_ship(r)
			if hold_raiders(raiders, cormorant) == 0:
				complete("raiders")
				var bai: AIPilot = brandt.get_node_or_null("AI") if alive(brandt) else null
				if bai != null and alive(lead_pilot()):
					bai.convoy_guard = false
					bai.escort = lead_pilot()     # back on the leader's wing
				say("fw_raid_done")
				say("fw_jump2")
				jumps_ok = true
				go("last_leg")
		"last_leg":
			_fly_on()
			_scout_away(delta)
			_follow_jump(delta)
			if cormorant.global_position.distance_to(port.global_position) < ARRIVE_REACH:
				complete("escort")
				if cormorant.warping:
					cormorant.end_warp()
				cormorant.autopilot = false
				say("fw_arrived")
				say("fw_home")
				objective("home", Names.fill("Return to the {terran} starbase"), home_base, Names.fill("{terran} starbase"))
				go("home")
		"home":
			_scout_away(delta)
			if pilot_near(home_base().global_position, HOME_REACH) != null:
				complete("home")
				complete("hold_fire")
				say("fw_done")
				say("fw_done2")
				pay(300)
				win()


## The Cormorant never warps on her own: only on her escort's lead.
func _hold_slow() -> void:
	if alive(cormorant):
		cormorant.warp_block = 999.0
		cormorant.energy = cormorant.energy_max     # her drive is the trip's, not the fight's


## Her engines are tuned for the run, but she never outruns the slowest
## ship flying with her.
func _speed_up() -> void:
	var b: Dictionary = cormorant.systems.base
	var slowest := INF
	for p in pilots():
		if alive(p):
			slowest = minf(slowest, p.max_speed)
	var top: float = clampf(slowest * 0.9, b.get("max_speed", cormorant.max_speed), b.get("max_speed", cormorant.max_speed) * SPEED_UP)
	var k: float = top / maxf(b.get("max_speed", cormorant.max_speed), 1.0)
	if not b.is_empty():
		b["max_speed"] *= k
		b["thrust_force"] *= k
		cormorant.apply_systems()
	else:
		cormorant.max_speed *= k
		cormorant.thrust_force *= k


## Back on course after a jump or a knock.
func _fly_on() -> void:
	if not alive(cormorant) or cormorant.warping:
		return
	if not cormorant.autopilot and cormorant.global_position.distance_to(port.global_position) > ARRIVE_REACH:
		cormorant.nav_target = dest
		cormorant.autopilot = true


func _progress() -> float:
	return 1.0 - cormorant.global_position.distance_to(port.global_position) / start_dist


## A pilot close to her who jumps toward the port takes her along, at the
## pilot's pace; she drops out with them, off course, or near the port.
## Nobody jumping for a while (no drive, no energy), she jumps alone at
## her own pace and the escort catches up.
func _follow_jump(delta: float) -> void:
	var lead: Ship = null
	for p in pilots():
		if alive(p) and p.warping and p.global_position.distance_to(cormorant.global_position) < JUMP_REACH + (8000.0 if cormorant.warping else 0.0):
			lead = p
	var to_port := port.global_position - cormorant.global_position
	if cormorant.warping:
		var near_port := to_port.length() < ARRIVE_REACH + DROP_SHORT
		var led_off := not solo and (lead == null or absf(wrapf(lead.rotation - to_port.angle(), -PI, PI)) > JUMP_AIM)
		if near_port or led_off or (stage == "escort" and _progress() > SCOUT_AT):
			cormorant.end_warp()
			cormorant.warp_block = 999.0
			return
		if solo:
			cormorant.rotation = rotate_toward(cormorant.rotation, to_port.angle(), deg_to_rad(cormorant.turn_speed) * 0.3 * delta)
			return
		cormorant.rotation = rotate_toward(cormorant.rotation, lead.rotation, deg_to_rad(cormorant.turn_speed) * delta)
		var pace := lead.warp_pace if lead.warp_pace > 0.0 else lead.warp_speed()
		var behind := (lead.global_position - cormorant.global_position).dot(lead.forward())
		cormorant.warp_pace = pace + clampf(behind * 0.5, -0.2 * pace, 0.3 * pace)
		return
	if not jumps_ok or to_port.length() < ARRIVE_REACH + DROP_SHORT * 3.0:
		return
	solo = lead == null
	if solo and stage_time < JUMP_WAIT:
		return
	var aim: float = to_port.angle() if solo else lead.rotation
	if absf(wrapf(aim - to_port.angle(), -PI, PI)) > JUMP_AIM:
		return
	cormorant.autopilot = false
	var diff := wrapf(aim - cormorant.rotation, -PI, PI)
	cormorant.inputs["turn"] = clampf(diff * 3.0, -1.0, 1.0)
	cormorant.inputs["thrust"] = 1.0 if absf(diff) < 0.5 else 0.0
	cormorant.inputs["brake"] = false
	if absf(diff) < 0.15:
		cormorant.start_warp()
		if not cormorant.warping:
			return
		cormorant.warp_pace = 0.0 if solo else (lead.warp_pace if lead.warp_pace > 0.0 else lead.warp_speed())
		if solo:
			say_once("fw_alone")
		elif stage == "last_leg":
			say_once("fw_jumping2")
		else:
			say_once("fw_jumping")


## Halfway out the scout's jammer pulls everyone out of warp, and it
## comes in for a look.
func _interdict() -> void:
	jumps_ok = false
	interdicted = true
	if cormorant.warping:
		cormorant.end_warp()
	for p in pilots() + ([brandt] if alive(brandt) else []):
		if alive(p) and p.warping and p.global_position.distance_to(cormorant.global_position) < INTERDICT:
			p.end_warp()
			world.notify(p, "Warp dropped: a jammer pulled you out.")
	_bring_scout()
	say("fw_dropped")
	say("fw_scout")
	say("fw_hold")
	say("fw_banter")
	scout_mode = "shadow"
	scout_t = SHADOW_TIME * 0.4


func _bring_scout() -> void:
	if alive(scout):
		return
	var fwd := Vector2.RIGHT.rotated((port.global_position - cormorant.global_position).angle())
	var at := cormorant.global_position + fwd.orthogonal() * (SHADOW_OFF + 500.0) + fwd * 900.0
	scout = spawn(Names.fill("{drengin} watcher"), 2, "Scout", "Vigil of Mars", at, cormorant.velocity, 4)
	scout.rotation = (cormorant.global_position - at).angle()
	scout.reset_physics_interpolation()
	scout.can_jam = true
	var ai: AIPilot = scout.get_node_or_null("AI")
	if ai != null:
		ai.set_physics_process(false)     # the operation flies it
	objective("crowd", Names.fill("Crowd the {drengin} scout off the Cormorant. Do not fire"), scout, Names.fill("{drengin} scout"), true)


## Shadow off her flank, then a run in at her side to read her hold; a
## pilot in its face turns it away.  After its runs it heads for Mars.
func _scout_runs(delta: float) -> void:
	if scout_mode == "leave" or not alive(scout):
		scout_mode = "leave"
		_scout_away(delta)
		return
	scout_t += delta
	var c := cormorant
	var side := Vector2.RIGHT.rotated(c.rotation).orthogonal() * run_side
	var crowded := _crowder() != null
	match scout_mode:
		"shadow":
			_fly(scout, c.global_position + side * SHADOW_OFF, c.velocity, c.global_position)
			if scout_t > SHADOW_TIME * 0.5 and scout_t - delta <= SHADOW_TIME * 0.5:
				run_side = _far_side()        # it picks the side nobody is guarding
			if scout_t > SHADOW_TIME:
				scout_mode = "run"
				scout_t = 0.0
				runs += 1
				if runs == 1:
					say("fw_run")
					say("fw_run2")
		"run":
			_fly(scout, c.global_position + side * (RUN_OFF + c.size), c.velocity, c.global_position)
			var close := scout.global_position.distance_to(c.global_position) < SCAN_REACH
			if scout_t < 3.0:
				for p in pilots():
					if alive(p) and p.global_position.distance_to(scout.global_position) < 1400.0:
						_jam(p)
				if runs == 1:
					say_once("fw_jam")
			if close and not crowded:
				scan = minf(scan + delta / SCAN_TIME, 1.0)
			set_text("crowd", Names.fill("Crowd the {drengin} scout off the Cormorant. Do not fire (hold read %d%%)") % roundi(scan * 100.0))
			if scan >= 1.0:
				_scout_leaves(true)
			elif crowded or scout_t > RUN_TIME:
				if crowded:
					say("fw_off_%d" % mini(runs, 3))
				if runs >= RUNS:
					_scout_leaves(false)
				else:
					scout_mode = "off"
					scout_t = 0.0
		"off":
			# veer away, then come round the other side
			_fly(scout, c.global_position + side * (SHADOW_OFF + 700.0), c.velocity, c.global_position)
			if scout_t > 2.5:
				run_side = _far_side()
				scout_mode = "shadow"
				scout_t = 0.0


func _scout_leaves(read: bool) -> void:
	scout_mode = "leave"
	scout_t = 0.0
	flag("fw_manifest", "read" if read else "safe")
	if read:
		fail_objective("crowd")
		say("fw_scanned")
	else:
		complete("crowd")
	say("fw_leave")


## The scout runs for Mars and is gone once out of sight.
func _scout_away(delta: float) -> void:
	scout_t += delta
	if not alive(scout):
		return
	var mars := planet("Mars")
	var away := (mars.global_position - scout.global_position).normalized()
	if not scout.warping:
		_fly(scout, scout.global_position + away * 3000.0, cormorant.velocity + away * scout.max_speed * 0.9, scout.global_position + away)
		if scout_t > 3.0 and absf(wrapf(away.angle() - scout.rotation, -PI, PI)) < 0.2:
			scout.warp_block = 0.0
			scout.energy = maxf(scout.energy, 400.0)
			scout.start_warp()        # and gone
	var lead := lead_pilot()
	if scout_t > 14.0 or (alive(lead) and scout.global_position.distance_to(lead.global_position) > 5000.0):
		world.remove_ship(scout)
		scout = null


## Her side (+1 left, -1 right of her course) away from the nearest pilot.
func _far_side() -> float:
	var near: Ship = null
	for p in pilots():
		if alive(p) and (near == null or p.global_position.distance_to(cormorant.global_position) < near.global_position.distance_to(cormorant.global_position)):
			near = p
	if near == null:
		return run_side
	var left := Vector2.RIGHT.rotated(cormorant.rotation).orthogonal()
	return -1.0 if left.dot(near.global_position - cormorant.global_position) > 0.0 else 1.0


## The pilot crowding the scout, if one is.
func _crowder() -> Ship:
	for p in pilots():
		if alive(p) and p.global_position.distance_to(scout.global_position) < CROWD_REACH + p.size + scout.size:
			return p
	return null


## Fly `s` toward `at`, matching `vel`, nose toward `look` when there.
func _fly(s: Ship, at: Vector2, vel: Vector2, look: Vector2) -> void:
	var i := s.inputs
	var want := vel + (at - s.global_position) * 0.7
	var dv := want - s.velocity
	if dv.length() > s.max_speed * 1.2:
		dv = dv.normalized() * s.max_speed * 1.2
	var face := (look - s.global_position).angle() if dv.length() < 25.0 else dv.angle()
	var diff := wrapf(face - s.rotation, -PI, PI)
	i["turn"] = clampf(diff * 3.0, -1.0, 1.0)
	i["thrust"] = 1.0 if dv.length() >= 25.0 and absf(diff) < 0.5 else 0.0
	i["strafe"] = 0.0
	i["brake"] = false
	i["fire_primary"] = false
	i["fire_secondary"] = false
	i["fire_missile"] = false
	# never bump a pilot: a bump across the Accord counts as a shot
	for o in world.ships:
		if o == s or o.dead or not (o.is_player or is_pilot(o) or o == cormorant):
			continue
		var to: Vector2 = o.global_position - s.global_position
		if to.length() < o.size + s.size + 120.0 and (s.velocity - o.velocity).dot(to) > 0.0:
			i["thrust"] = 0.0
			i["brake"] = true
			i["strafe"] = -1.0 if Vector2.from_angle(s.rotation).orthogonal().dot(to) > 0.0 else 1.0


## The scout's jammer sweeps a pilot.
func _jam(p: Ship) -> void:
	var ps: Sensors = p.sensors
	if not ps.is_jammed():
		ps._own.clear()
		ps._seen.clear()
		world.notify(p, "Sensors jammed by %s." % scout.pilot_name)
	ps.jammed_until = ps.now() + 0.3
	ps.jammed_by = scout


## "Pirates" come at her from ahead, more with friends along.
func _raid() -> void:
	var fwd := Vector2.RIGHT.rotated((port.global_position - cormorant.global_position).angle())
	var from := cormorant.global_position + fwd.rotated(0.5) * RAID_FROM
	var names := ["Rust Jackal", "Rust Crow", "Rust Viper", "Rust Hornet", "Rust Adder", "Rust Rook"]
	for n in scaled(3):
		var at := from + fwd.orthogonal() * (n - 1) * 160.0 - fwd * (n % 2) * 140.0
		var r := spawn("Raider %d" % (n + 1), 3, "Raider", names[n % names.size()], at, cormorant.velocity, 3)
		r.rotation = (cormorant.global_position - at).angle()
		r.reset_physics_interpolation()
		var ai: AIPilot = r.get_node("AI")
		ai.patrol_point = cormorant.global_position
		ai.duel = true                    # they came for her: no going home to mend
		r.target = cormorant
		raiders.append(r)
	var bai: AIPilot = brandt.get_node_or_null("AI") if alive(brandt) else null
	if bai != null:
		bai.guard(cormorant)              # Brandt sticks to her and fights what comes
	say("fw_raid")
	say("fw_raid2")
	objective("raiders", "Drive the raiders off the Cormorant", func(): return nearest_raider(raiders, cormorant.global_position) if alive(cormorant) else null, "Raider")


## Raiders still in the fight (a disabled one is out of it).
func _raiders_left() -> int:
	var n := 0
	for r in raiders:
		if alive(r) and not r.is_disabled():
			n += 1
	return n


func _on_brandt_died(s: Ship, _killer: Node) -> void:
	if not s.captured:
		say("brandt_down")


func _on_cormorant_died(_s: Ship, _killer: Node) -> void:
	fail("The Cormorant was lost.", "fw_fail_cargo")


func on_breach(attacker: Node, victim: Node) -> void:
	if is_pilot(attacker) and (victim == scout or (victim is Ship and victim.race_id == 2) or (victim is Planet and victim.owner_race != 1)):
		fail_objective("hold_fire")
		fail("You fired first on an Accord ship.", "fw_fail_breach")
