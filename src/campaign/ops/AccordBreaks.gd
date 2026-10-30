## Operation 5, The Accord Breaks: Kesh Varr ends the Accord on every
## channel and the Drengin come for Luna.  The player and Lt. Brandt hold
## it for four minutes until the fleet warps in.  It fails forward: if Luna
## falls, the story goes on without it (flags.luna = "lost").  Either way
## the war is hot from here on.
extends StoryOp

const HOLD_TIME := 240.0          # seconds until the fleet arrives
const WAVES := [                  # [seconds in, designs]
	[8.0, ["Destroyer", "Scout", "Scout", "Scout"]],
	[80.0, ["Cruiser", "Destroyer", "Scout"]],
	[150.0, ["Destroyer", "Destroyer", "Scout", "Scout"]],
]
## Admiral Okafor's fleet: [design, name], warping in from Earth's side.
const RELIEF := [
	["Battleship", "TSS Dauntless"], ["Cruiser", "TSS Resolute"], ["Cruiser", "TSS Vigilant"],
	["Destroyer", "TSS Steadfast"], ["Destroyer", "TSS Valiant"], ["Destroyer", "TSS Tenacity"],
	["Scout", "TSS Swift"], ["Scout", "TSS Kestrel"],
]
const RELIEF_FROM := 9000.0       # how far out from Luna they start their jump
const RELIEF_PACE := 1800.0       # the fleet's warp speed, slowest drive or not
const RELIEF_DROP := 1600.0       # and how far from Luna the first row drops out
const WAVE_FROM := 4200.0         # from Luna, on the Mars side
const FALL_AFTER := 35.0          # seconds with Luna's defences down and Drengin overhead
const OVERHEAD := 2600.0
const HOME_REACH := 900.0

var luna: Planet
var brandt: Ship
var fleet: Array = []             # the Drengin assault, as spawned
var wave := 0
var down_t := 0.0                 # seconds Luna has lain open
var relief: Array = []            # Okafor's fleet


func setup() -> void:
	luna = planet("Luna")
	if not luna.is_colonised() or luna.owner_race != 1:
		# whatever the open war did to it, Luna is Terran when this begins
		luna.surrender_to(1, maxf(luna.population, GameData.MINOR_COLONY))
	var p := player()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = p
	ai.state = AIPilot.State.ESCORT
	say("ab_kesh_1")
	say("ab_kesh_2")
	say("ab_kesh_3")
	say("ab_hot")
	objective("hold", "Hold Luna until the fleet arrives (%s)" % _clock(HOLD_TIME), luna, "Luna")
	go("broadcast")


func step(delta: float) -> void:
	match stage:
		"broadcast":
			if stage_time > 4.0:
				# the war goes hot, for good
				world.accord = false
				story.keep_peace()
				world.notify_all(Names.fill("The Ceres Accord is over. The {drengin} Empire is at war with Terra."), Color(1.0, 0.5, 0.4))
				world.notify_all(Names.fill("The {arcean} Union stays out of it: {arcean} ships and worlds are still off limits to {terrans}."), Color(0.8, 0.9, 1.0))
				go("hold")
		"hold":
			var left := HOLD_TIME - stage_time
			set_text("hold", "Hold Luna until the fleet arrives (%s)" % _clock(left))
			while wave < WAVES.size() and stage_time >= WAVES[wave][0]:
				_send_wave(wave)
				wave += 1
			if left < 90.0:
				say_once("ab_wave3")
			# Luna falls once its guns have been down a while under Drengin guns
			if luna.owner_race != 1:
				_lost()
				return
			if luna.is_defenceless() and _overhead():
				if down_t == 0.0:
					say_once("ab_luna_weak")
				down_t += delta
				if down_t >= FALL_AFTER:
					luna.surrender_to(2, maxf(luna.population * 0.5, GameData.MINOR_COLONY * 0.5))
					_lost()
					return
			else:
				down_t = maxf(down_t - delta * 0.5, 0.0)
			if left <= 0.0:
				_held()
		"home":
			_relief_in()
			if pilot_near(home_base().global_position, HOME_REACH) != null:
				complete("home")
				say("ab_done")
				pay(600)
				win()


func _send_wave(i: int) -> void:
	var mars := planet("Mars")
	var dir := (mars.global_position - luna.global_position).angle() if mars != null else 0.0
	var from := luna.global_position + Vector2(WAVE_FROM, 0).rotated(dir)
	var designs: Array = WAVES[i][1].duplicate()
	for k in range(designs.size(), scaled(designs.size())):
		designs.append(WAVES[i][1][k % WAVES[i][1].size()])
	for k in designs.size():
		var at := from + Vector2((k % 3) * -160.0, (k - designs.size() * 0.5) * 180.0).rotated(dir)
		var s := spawn(Names.fill("{drengin} %s") % designs[k], 2, designs[k], "Claw of Varr %d-%d" % [i + 1, k + 1], at, luna.velocity, 5 + i)
		s.rotation = dir + PI
		s.reset_physics_interpolation()
		var ai: AIPilot = s.get_node("AI")
		ai.bombard = true             # they are here for Luna's guns
		ai.patrol_point = luna.global_position
		fleet.append(s)
	if i == 0:
		say("ab_brandt")
	elif i == 1:
		say("ab_wave2")


## A Drengin warship of the assault close over Luna.
func _overhead() -> bool:
	for s in fleet:
		if alive(s) and s.global_position.distance_to(luna.global_position) < OVERHEAD:
			return true
	return false


func _held() -> void:
	complete("hold")
	flag("luna", "held")
	say("ab_fleet")
	say("ab_held")
	_relief()
	_home()


func _lost() -> void:
	fail_objective("hold")
	flag("luna", "lost")
	say("ab_lost")
	say("ab_lost2")
	_home()


## The fleet warps in together from Earth's side and drops out over Luna;
## it clears what is left of the assault and leaves with the operation.
func _relief() -> void:
	var earth := planet("Earth")
	var dir := (earth.global_position - luna.global_position).angle() if earth != null else PI
	var from := luna.global_position + Vector2(RELIEF_FROM, 0).rotated(dir)
	for k in RELIEF.size():
		var row := k / 3
		var at := from + Vector2(row * 260.0, (k % 3 - 1) * 240.0 + row * 120.0).rotated(dir)
		var s := spawn(Names.fill("{terran} Captain %d") % (k + 1), 1, RELIEF[k][0], RELIEF[k][1], at, luna.velocity, 7)
		s.rotation = dir + PI
		s.reset_physics_interpolation()
		var ai: AIPilot = s.get_node("AI")
		ai.home = earth
		ai.patrol_point = luna.global_position + Vector2(luna.radius + 700.0 + row * 200.0, 0).rotated(dir + (k % 3 - 1) * 0.35)
		ai.set_physics_process(false)     # the jump is the operation's; the captains fight once out
		s.inputs["brake"] = false
		s.energy = s.energy_max
		s.warp_block = 0.0
		s.start_warp()
		s.warp_pace = RELIEF_PACE        # the fleet jumps as one
		relief.append(s)


## The fleet's jump: each ship drops out short of Luna, in its row, and
## its captain takes over.
func _relief_in() -> void:
	for k in relief.size():
		var s: Ship = relief[k]
		if not alive(s):
			continue
		var ai: AIPilot = s.get_node("AI")
		if ai.is_physics_processing():
			continue
		s.inputs["brake"] = false
		s.inputs["thrust"] = 0.0
		var stop := luna.radius + RELIEF_DROP + (k / 3) * 300.0
		if not s.warping or s.global_position.distance_to(luna.global_position) < stop:
			if s.warping:
				s.end_warp()
			ai.set_physics_process(true)


func _home() -> void:
	objective("home", Names.fill("Fall back to the {terran} starbase") if story.flags.get("luna", "") == "lost" else Names.fill("Return to the {terran} starbase"), home_base, Names.fill("{terran} starbase"))
	go("home")


static func _clock(t: float) -> String:
	var s := int(ceilf(maxf(t, 0.0)))
	return "%d:%02d" % [s / 60, s % 60]
