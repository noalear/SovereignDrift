## Operation 8, The Envoy: Speaker Tenn of the Arcean Union flies from
## Saturn to Earth in one unarmed ship.  The pilots meet it in the belt and
## bring it to the Terran starbase; on the way the pirate captain Mags
## Corrigan tries to take it (the story's one pirate set piece).  At the
## starbase comes the second story choice: show Tenn the evidence from
## Ceres and the free port (the Arceans side with Terra), or keep it for
## Fleet Intelligence (a bonus, and the Arceans stay out).
extends StoryOp

const MEET_FROM := 0.55           # the envoy is this far along from Saturn to Earth when it begins
const MEET_REACH := 1000.0
const AMBUSH_AT := 0.45           # of the way from the meeting point to Earth
const AMBUSH_FROM := 2600.0       # ahead of the envoy
const PIRATES := ["Raider", "Scout", "Scout"]
const FLEE_AT := 0.4              # Corrigan's hull fraction when she cuts her losses
const ARRIVE_REACH := 1100.0      # from the Terran starbase
const ENVOY_HULL := 3.0           # the envoy's hull, times a Cruiser's
const BONUS := 1000               # Intelligence's pay for keeping the evidence

var envoy: Ship
var corrigan: Ship
var pirates: Array = []
var dest: Waypoint                # the starbase, for the envoy's autopilot
var brandt: Ship
var start_dist := 1.0
var hit_said := false


func setup() -> void:
	var saturn := planet("Saturn")
	var earth := planet("Earth")
	var base: Ship = world.starbase_of(1)
	var home: Node2D = base if base != null else earth
	dest = waypoint(Names.fill("the {terran} starbase"), home, 160.0)
	var at := saturn.global_position.lerp(home.global_position, MEET_FROM)
	at = clear_spot(at, randf() * TAU, 10.0)
	# an Arcean hull under a truce: Terrans may not fire on it
	envoy = spawn("Speaker Tenn", 0, "Cruiser", "Quiet Accord", at, Vector2.ZERO, -1, 0)
	envoy.velocity = _drift_at(at)
	envoy.body.vel = envoy.velocity
	envoy.rotation = (home.global_position - at).angle()
	envoy.reset_physics_interpolation()
	envoy.hull_max *= ENVOY_HULL
	envoy.hull = envoy.hull_max
	envoy.died.connect(func(_s, _k): fail("The envoy's ship was lost.", "env_fail_lost"))
	envoy.damaged.connect(_on_envoy_hit)
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = p
	ai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	say("env_start")
	objective("meet", "Meet Speaker Tenn's ship in the belt", envoy, Names.fill("{arcean} envoy"))
	objective("hold_fire", "Nothing touches the envoy's ship", null, "", true)
	go("meet")


## The envoy waits on a slow orbit of the Sun until its escort comes.
func _drift_at(at: Vector2) -> Vector2:
	var sun: Planet = world.sun
	if sun == null:
		return Vector2.ZERO
	return world.space.circular_orbit_velocity(sun.body, at)


func step(_delta: float) -> void:
	_hold_slow()
	match stage:
		"meet":
			if pilot_near(envoy.global_position, MEET_REACH) != null:
				complete("meet")
				say("env_tenn_1")
				say("env_tenn_2")
				objective("escort", Names.fill("Escort the envoy to the {terran} starbase"), envoy, Names.fill("{arcean} envoy"))
				envoy.nav_target = dest
				envoy.autopilot = true
				start_dist = maxf(envoy.global_position.distance_to(dest.global_position), 1.0)
				go("escort")
		"escort":
			_keep_course()
			if 1.0 - envoy.global_position.distance_to(dest.global_position) / start_dist > AMBUSH_AT and stage_time > 10.0:
				_ambush()
				go("ambush")
			_arrival()
		"ambush":
			_keep_course()
			if alive(corrigan) and corrigan.hull < corrigan.hull_max * FLEE_AT:
				say("env_corrigan_flee")
				world.fx.explosion(corrigan.global_position, 30.0, Color(0.6, 0.8, 1.0))
				world.remove_ship(corrigan)     # she jumps out
				corrigan = null
			if _pirates_left() == 0 and stage_time > 4.0:
				complete("pirates")
				say("env_clear")
				go("escort2")
			_arrival()
		"escort2":
			_keep_course()
			_arrival()
		"choice":
			if stage_time > 1.0:
				say("env_choice")
				ask("envoy", Names.fill("Speaker Tenn asks why the {arceans} should join Terra's war."), [
					["share", "Show Tenn the evidence", Names.fill("The Ceres log and the free port records go to the {arceans}. They learn the raids were {drengin}, and their ships may stand with Terra.")],
					["keep", "Keep it for Fleet Intelligence", Names.fill("Intelligence pays a %d credit bonus for your discretion. The {arceans} stay out of the war.") % BONUS]])
				go("asked")
		"after":
			if stage_time > 6.0 or not story.speaking():
				pay(600)
				win()


func on_choice(id: String, key: String) -> void:
	if id != "envoy" or stage != "asked":
		return
	complete("talk")
	if key == "share":
		flag("arceans", "allied")
		say("env_share_1")
		say("env_share_2")
	else:
		flag("arceans", "neutral")
		say("env_keep_1")
		say("env_keep_2")
		for s in pilots():
			s.earn(BONUS)
			world.notify(s, "Intelligence bonus: %d credits." % BONUS)
	go("after")


## The envoy never warps ahead of its escort.
func _hold_slow() -> void:
	if alive(envoy):
		envoy.warp_block = 999.0
		if envoy.warping:
			envoy.end_warp()


func _keep_course() -> void:
	if alive(envoy) and not envoy.autopilot and envoy.global_position.distance_to(dest.global_position) > ARRIVE_REACH:
		envoy.autopilot = true


func _arrival() -> void:
	if stage == "choice" or not alive(envoy):
		return
	if envoy.global_position.distance_to(dest.global_position) < ARRIVE_REACH:
		complete("escort")
		if alive(corrigan) or _pirates_left() > 0:
			# they gave up the chase at the starbase's guns
			for r in pirates:
				if alive(r):
					world.remove_ship(r)
			complete("pirates")
		complete("hold_fire")
		envoy.autopilot = false
		say("env_arrive")
		objective("talk", "Answer Speaker Tenn", envoy, Names.fill("{arcean} envoy"))
		go("choice")


## Mags Corrigan and her crew come out ahead of the envoy.
func _ambush() -> void:
	var dir := envoy.velocity.angle() if envoy.velocity.length() > 10.0 else envoy.rotation
	var from := envoy.global_position + Vector2.RIGHT.rotated(dir) * AMBUSH_FROM
	var list: Array = PIRATES.duplicate()
	for k in range(list.size(), scaled(list.size())):
		list.append(PIRATES[1 + k % 2])
	for k in list.size():
		var at := clear_spot(from + Vector2(0, (k - list.size() * 0.5) * 220.0).rotated(dir), dir, 1.0)
		var r := spawn("Mags Corrigan" if k == 0 else "Corrigan's crew", 3, list[k], "Lucky Mags" if k == 0 else "Corsair %d" % k, at, envoy.velocity, 6 if k == 0 else 4)
		r.rotation = dir + PI
		r.reset_physics_interpolation()
		r.target = envoy if k % 2 == 0 else lead_pilot()
		var ai: AIPilot = r.get_node("AI")
		ai.duel = true                  # they came for the yacht: no going home to mend
		pirates.append(r)
	corrigan = pirates[0]
	say("env_corrigan_1")
	say("env_corrigan_2")
	say("env_weapons")
	objective("pirates", "Drive off Mags Corrigan's pirates", corrigan, "Mags Corrigan")


func _pirates_left() -> int:
	var n := 0
	for r in pirates:
		if alive(r):
			n += 1
	return n


func _on_envoy_hit(_s: Ship, _amount: float) -> void:
	if not hit_said and stage == "ambush":
		hit_said = true
		say("env_tenn_hit")


func on_breach(attacker: Node, victim: Node) -> void:
	if is_pilot(attacker) and (victim == envoy or (victim is Ship and victim.race_id == 0)):
		fail_objective("hold_fire")
		fail(Names.fill("You fired on the {arcean} envoy."), "env_fail_breach")
