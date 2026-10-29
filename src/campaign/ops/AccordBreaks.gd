## Operation 5, The Accord Breaks: Kesh Varr ends the Accord on every
## channel and the Drengin come for Luna.  The player and Lt. Brandt hold
## it for six minutes until the fleet arrives.  It fails forward: if Luna
## falls, the story goes on without it (flags.luna = "lost").  Either way
## the war is hot from here on.
extends StoryOp

const HOLD_TIME := 360.0          # seconds until the fleet arrives
const WAVES := [                  # [seconds in, designs]
	[8.0, ["Destroyer", "Scout", "Scout", "Scout"]],
	[130.0, ["Cruiser", "Destroyer", "Scout"]],
	[250.0, ["Destroyer", "Destroyer", "Scout", "Scout"]],
]
const WAVE_FROM := 4200.0         # from Luna, on the Mars side
const FALL_AFTER := 35.0          # seconds with Luna's defences down and Drengin overhead
const OVERHEAD := 2600.0
const HOME_REACH := 900.0

var luna: Planet
var brandt: Ship
var fleet: Array = []             # the Drengin assault, as spawned
var wave := 0
var down_t := 0.0                 # seconds Luna has lain open


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
				world.notify_all("The Ceres Accord is over. The Drengin Empire is at war with Terra.", Color(1.0, 0.5, 0.4))
				world.notify_all("The Arcean Union stays out of it: Arcean ships and worlds are still off limits to Terrans.", Color(0.8, 0.9, 1.0))
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
			var base: Ship = world.starbase_of(1)
			if base != null and pilot_near(base.global_position, HOME_REACH) != null:
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
		var s := spawn("Drengin %s" % designs[k], 2, designs[k], "Claw of Varr %d-%d" % [i + 1, k + 1], at, luna.velocity, 5 + i)
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


## The fleet that arrives stays in the war as Terran warships of one life
## each (kept in saves like a ship built to order).
func _relief() -> void:
	var earth := planet("Earth")
	for k in 3:
		var at := luna.global_position + Vector2(900, 0).rotated(TAU * k / 3.0)
		var s: Ship = world.spawn_ship("Terran Captain %d" % (k + 1), 1, "Destroyer" if k > 0 else "Cruiser", "TSS %s" % ["Resolute", "Vigilant", "Steadfast"][k], at, luna.velocity, 7)
		s.remove_meta("tutorial")
		s.set_meta("ordered", true)
		Net.host_ship_added(s)
		var ai: AIPilot = s.get_node("AI")
		ai.home = earth


func _home() -> void:
	objective("home", "Fall back to the Terran starbase" if story.flags.get("luna", "") == "lost" else "Return to the Terran starbase", world.starbase_of(1), "Terran starbase")
	go("home")


static func _clock(t: float) -> String:
	var s := int(ceilf(maxf(t, 0.0)))
	return "%d:%02d" % [s / 60, s % 60]
