## Operation 14, Sunward: the Hammer runs for Venus the short way, across
## the face of the Sun.  Out in open space its shields are too strong for
## anything Terra has; in the Sun's heat they burn away and its engines
## labour, its reactor struggles to feed its guns, and the heat eats at
## its hull too.  The pilots catch it in the
## heat and finish it before it gets out the other side.  Listening to the
## codes (flags.codes "listen") put an ambush group on its course, sprung
## with the story key: Arcean if the Union stands with Terra.  False orders
## ("spoof") gave Kesh warning: the Hammer has a head start.  A Hammer
## crippled in its yard (Burn the Yards) is weaker.  Then the war is over.
extends StoryOp

const R_OUT := 7000.0             # the Hammer's course begins and ends this far from the Sun's centre
const R_GRAZE := 950.0            # and passes this close to it
const SWEEP := PI                 # round the Sun this far
const START_TURN := 0.6           # rad round from Earth's side, toward Venus, where the course begins
const HEAD_START := -0.72         # course point it starts from after the false orders (-1 otherwise)
const PACE := 95.0                # the Hammer's speed out of the heat
const HEAT_SLOW := 0.85           # in the heat it loses this share of its pace at full heat
const SHIELD_HEAT := 0.2          # heat over this burns its shields away
const HEAT_POWER := 0.6           # and its reactor loses this share of its charge rate at full heat: its guns slow
const HAMMER_HULL := 3.0          # its hull, times a Dreadnought's
const CRIPPLED := 0.65            # and this much of that when it burned in its yard
const ESCORTS := ["Destroyer", "Destroyer"]
const AMBUSH_TERRAN := [["Cruiser", "TSS Resolute"], ["Destroyer", "TSS Harrow"], ["Destroyer", "TSS Kestrel"]]
const AMBUSH_ARCEAN := [["Cruiser", "AUS Deep Current"], ["Destroyer", "AUS Undertow"], ["Destroyer", "AUS Tidewalker"]]
const AMBUSH_AT := 0.3            # course point the ambush waits beside, past the heat
const AMBUSH_OFF := 500.0         # this far outside the course
const AMBUSH_SLOW := 0.5          # crossing its bow, they take this share of its pace
const AMBUSH_TIME := 20.0
const LEAVING_AT := 0.22          # course point past the graze: Hale warns it is nearly out
const ESCAPE_AT := 0.6            # course point well clear of the heat: it is away
const SPOTTED := 3000.0           # a pilot this close: Kesh speaks
const HOME_REACH := 900.0

var sun: Planet
var hammer: Ship
var escorts: Array = []
var ambush: Array = []
var brandt: Ship
var turn := 1.0                   # which way round the Sun the course runs
var start_angle := 0.0
var u := -1.0                     # where the Hammer is on its course, -1..1 (0: closest to the Sun)
var sprung := false
var arcean := false               # the ambush is the Union's
var slow_left := 0.0              # seconds the ambush still has it slowed
var in_heat := false
var regen := 0.0                  # the Hammer's own energy regeneration


func setup() -> void:
	sun = world.sun
	var earth := planet("Earth")
	var venus := planet("Venus")
	var ea := (earth.global_position - sun.global_position).angle()
	var va := (venus.global_position - sun.global_position).angle() if venus != null else ea + 1.0
	turn = 1.0 if wrapf(va - ea, -PI, PI) >= 0.0 else -1.0
	start_angle = ea + turn * START_TURN
	u = HEAD_START if story.flags.get("codes", "") == "spoof" else -1.0
	var at := _course(u)
	hammer = spawn("Kesh Varr", 2, "Dreadnought", "Hammer of Mars", at, _course_vel(u, PACE), 14)
	hammer.hull_max *= HAMMER_HULL * (CRIPPLED if story.flags.get("hammer", "") == "crippled" else 1.0)
	hammer.hull = hammer.hull_max
	hammer.set_meta("no_boarding", true)
	hammer.invulnerable = 1.0
	hammer.warp_block = 1e9
	regen = hammer.energy_regen
	hammer.died.connect(_on_hammer_died)
	var hai: AIPilot = hammer.get_node("AI")
	hai.duel = true                   # it fights whoever comes, and never turns for home
	for k in scaled(ESCORTS.size()):
		var e := spawn(Names.fill("{drengin} %s") % ESCORTS[k % ESCORTS.size()], 2, ESCORTS[k % ESCORTS.size()], "Hammer Guard %d" % (k + 1),
			at + Vector2(-260, (k - 0.5) * 320.0).rotated(hammer.velocity.angle()), hammer.velocity, 7)
		e.get_node("AI").guard(hammer)
		escorts.append(e)
	if story.flags.get("codes", "") == "listen":
		arcean = story.flags.get("arceans", "") == "allied"
		var rows: Array = AMBUSH_ARCEAN if arcean else AMBUSH_TERRAN
		var wait := _course(AMBUSH_AT)
		var out := (wait - sun.global_position).normalized()
		for k in rows.size():
			var s := spawn(Names.fill("{arcean} Captain") if arcean else Names.fill("{terran} Captain"), 0 if arcean else 1, rows[k][0], rows[k][1],
				wait + out * (AMBUSH_OFF + 220.0 * k), Vector2.ZERO, 8, 0 if arcean else -1)
			s.rotation = (-out).angle()
			s.reset_physics_interpolation()
			var ai: AIPilot = s.get_node("AI")
			ai.set_physics_process(false)      # waiting dark until it is sprung
			ambush.append(s)
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var bai: AIPilot = brandt.get_node("AI")
	bai.escort = p
	bai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	say("sw_start")
	objective("catch", "Catch the Hammer before it crosses the Sun", hammer, "Hammer of Mars")
	if not ambush.is_empty():
		objective("ambush", "Spring the ambush when the Hammer is in reach (%s)" % Keybinds.short_text("briefing"), ambush[0], "Ambush group", true)
	go("chase")


## The course: round the Sun from start_angle, closest at u = 0.
func _course(at_u: float) -> Vector2:
	var a := start_angle + turn * (at_u + 1.0) * 0.5 * SWEEP
	var r := R_GRAZE + (R_OUT - R_GRAZE) * at_u * at_u
	return sun.global_position + Vector2(r, 0).rotated(a)


func _course_vel(at_u: float, pace: float) -> Vector2:
	var d := _course(at_u + 0.001) - _course(at_u - 0.001)
	return sun.velocity + d.normalized() * pace


func step(delta: float) -> void:
	match stage:
		"chase":
			_fly_hammer(delta)
			if not sprung:
				for s in ambush:
					if alive(s):
						s.invulnerable = 1.0     # running dark: nobody has seen them yet
			if not alive(hammer):
				return
			if pilot_near(hammer.global_position, SPOTTED) != null:
				say_once("sw_kesh")
				if story.flags.get("codes", "") == "spoof":
					say_once("sw_kesh_codes")
				if not in_heat:
					say_once("sw_shields_up")
			if in_heat:
				say_once("sw_heat")
				set_text("catch", "Finish the Hammer while it is in the Sun's heat")
				if not ambush.is_empty() and not sprung:
					say_once("sw_ambush_ready")
			if hammer.hull < hammer.hull_max * 0.5:
				say_once("sw_half")
			if hammer.hull < hammer.hull_max * 0.25:
				say_once("sw_kesh_low")
			if u > LEAVING_AT:
				say_once("sw_leaving")
			if u >= ESCAPE_AT:
				fail("The Hammer got out of the heat and away to Venus.", "sw_fail_escaped")
		"home":
			if pilot_near(home_base().global_position, HOME_REACH) != null:
				complete("home")
				say("sw_end_2")
				say("sw_end_3")
				say("sw_end_4")
				pay(1500)
				win()


## The Hammer keeps to its course: slower the hotter it gets, slower still
## with the ambush across its bow.  Out of the heat its shields hold
## everything off; in it they burn away.
func _fly_hammer(delta: float) -> void:
	if not alive(hammer):
		return
	var heat: float = world.heat_at(hammer.global_position)
	in_heat = heat >= SHIELD_HEAT
	var pace := PACE * (1.0 - HEAT_SLOW * heat)
	if slow_left > 0.0:
		slow_left -= delta
		pace *= AMBUSH_SLOW
	var d := _course(u + 0.001) - _course(u - 0.001)
	u = minf(u + pace * delta * 0.002 / maxf(d.length(), 0.001), 1.0)
	var at := _course(u)
	var vel := _course_vel(u, pace)
	hammer.global_position = at
	hammer.velocity = vel
	hammer.body.pos = at
	hammer.body.vel = vel
	hammer.energy_regen = regen * (1.0 - HEAT_POWER * heat)
	if in_heat:
		hammer.invulnerable = 0.0
		hammer.shield = 0.0
	else:
		hammer.invulnerable = 1.0
		hammer.shield = hammer.shield_max


## A pilot pressed the story key: the ambush group crosses its bow.
func signal_label() -> String:
	return "Spring the ambush" if stage == "chase" and not ambush.is_empty() and not sprung else ""


func on_signal(by: Ship) -> void:
	if sprung or ambush.is_empty() or stage != "chase":
		return
	sprung = true
	slow_left = AMBUSH_TIME
	complete("ambush")
	world.notify_all("%s springs the ambush." % by.pilot_name, Color(0.7, 1.0, 0.75))
	say("sw_ambush_tenn" if arcean else "sw_ambush_terran")
	for s in ambush:
		if alive(s):
			var ai: AIPilot = s.get_node("AI")
			ai.set_physics_process(true)
			ai.duel = true
			ai.patrol_point = hammer.global_position if alive(hammer) else s.global_position
			if alive(hammer):
				s.target = hammer


func _on_hammer_died(_s: Ship, _k: Node) -> void:
	if stage != "chase":
		return
	complete("catch")
	story.hush()
	say("sw_kesh_last")
	say("sw_killed")
	say("sw_end_1")
	flag("war", "won")
	story.keep_peace()
	world.notify_all("The Hammer of Mars is destroyed.", Color(0.6, 1.0, 0.7))
	for s in escorts:
		if alive(s):
			s.get_node("AI").release()
	objective("home", Names.fill("Come home to the {terran} starbase"), home_base, Names.fill("{terran} starbase"))
	go("home")
