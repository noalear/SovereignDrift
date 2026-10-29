## Operation 10, Burn the Yards: a Terran strike group led by the Resolute
## goes for the Hammer's shipyard over Mars, with the pilots and Lt.
## Brandt.  Burn the yard before the Hammer's cradle opens and the Hammer
## is crippled in it; if the clock runs out the Hammer launches and the
## strike group falls back.  It fails forward: flags.hammer is "crippled"
## or "launched", and the war goes on either way.
extends StoryOp

const YARD_OFF := 2600.0          # as in Silent Running
const LAUNCH_TIME := 330.0        # seconds until the Hammer's cradle opens
const YARD_HULL := 3.0            # the yard's hull, times a Shipyard's before GameData.STATION_TOUGHNESS
const STRIKE := [["Cruiser", "TSS Resolute"], ["Destroyer", "TSS Harrow"], ["Destroyer", "TSS Kestrel"]]
const STRIKE_FROM := 7000.0       # the strike group forms this far from the yard
const WAVES := [                  # [seconds in, designs], from Mars
	[20.0, ["Destroyer", "Scout", "Scout"]],
	[130.0, ["Cruiser", "Destroyer"]],
	[230.0, ["Destroyer", "Destroyer", "Scout"]],
]
const CLEAR := 7500.0             # from Mars: clear of it
const RAMPAGE := 25.0             # seconds the launched Hammer hunts before it goes home

var mars: Planet
var yard: Ship
var hammer: Ship
var hammer_off := Vector2.ZERO
var brandt: Ship
var strike: Array = []
var fleet: Array = []
var wave := 0
var outcome := ""                 # crippled | launched
var half_said := false


func setup() -> void:
	mars = planet("Mars")
	yard = hammer_yard(YARD_OFF)
	# the clock was timed on this much hull: keep it so
	yard.hull_max *= YARD_HULL / GameData.STATION_TOUGHNESS
	yard.hull = yard.hull_max
	yard.died.connect(_on_yard_died)
	hammer = spawn("Kesh Varr", 2, "Dreadnought", "Hammer of Mars", yard.global_position, mars.velocity, 14)
	hammer_off = (yard.global_position - mars.global_position).normalized().orthogonal() * (yard.size + hammer.size + 80.0)
	_cradle(true)
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var bai: AIPilot = brandt.get_node("AI")
	bai.escort = p
	bai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	# the strike group waits outside Mars's reach on the pilots' side
	var dir := (p.global_position - yard.global_position).angle()
	var form := clear_spot(yard.global_position, dir, STRIKE_FROM)
	for k in STRIKE.size():
		var s := spawn("Terran Captain", 1, STRIKE[k][0], STRIKE[k][1], form + Vector2(0, (k - 1) * 260.0).rotated(dir), mars.velocity, 8)
		s.rotation = dir + PI
		s.reset_physics_interpolation()
		var ai: AIPilot = s.get_node("AI")
		ai.patrol_point = yard.global_position
		s.target = yard
		strike.append(s)
	say("by_start")
	say("by_start2")
	objective("yard", _yard_text(LAUNCH_TIME), yard, "Varr Yards")
	objective("strike", "Keep the Resolute in the fight", strike[0], "TSS Resolute", true)
	strike[0].died.connect(func(_s, _k): fail_objective("strike"))
	go("strike")


func _yard_text(left: float) -> String:
	return "Burn the Hammer's shipyard before its cradle opens (%s)" % _clock(left)


## The Hammer's AI sleeps in the cradle.
func _cradle(on: bool) -> void:
	var ai: AIPilot = hammer.get_node_or_null("AI") if alive(hammer) else null
	if ai != null:
		ai.set_physics_process(not on)
		ai.set_process(not on)


func step(_delta: float) -> void:
	match stage:
		"strike":
			hold_in_cradle(hammer, yard, hammer_off)
			var left := LAUNCH_TIME - stage_time
			set_text("yard", _yard_text(left))
			_strike_orders()
			while wave < WAVES.size() and stage_time >= WAVES[wave][0]:
				_send_wave(wave)
				wave += 1
			if stage_time > 10.0:
				say_once("by_kesh")
			if not half_said and alive(yard) and yard.hull < yard.hull_max * 0.5:
				half_said = true
				say("by_halfway")
			if left < 60.0:
				say_once("by_minute")
			if left <= 0.0:
				_launch()
		"launched":
			# the Hammer hunts a while, then goes home to Mars
			if alive(hammer) and stage_time > RAMPAGE:
				var ai: AIPilot = hammer.get_node_or_null("AI")
				if ai != null:
					ai.patrol_point = mars.global_position
				hammer.target = null
			_home_check()
		"burned":
			_home_check()


## The strike group's one job is the yard: they never break off for
## anything else while it stands.
func _strike_orders() -> void:
	for s in strike:
		if alive(s) and alive(yard):
			s.target = yard


func _send_wave(i: int) -> void:
	var dir := (yard.global_position - mars.global_position).angle()
	var from := mars.global_position + Vector2(mars.radius + 900.0, 0).rotated(dir + 0.4)
	var designs: Array = WAVES[i][1].duplicate()
	for k in range(designs.size(), scaled(designs.size())):
		designs.append(WAVES[i][1][k % WAVES[i][1].size()])
	for k in designs.size():
		var at := clear_spot(from + Vector2(0, (k - designs.size() * 0.5) * 180.0).rotated(dir), dir, 1.0)
		var s := spawn("Drengin %s" % designs[k], 2, designs[k], "Yard Guard %d-%d" % [i + 1, k + 1], at, mars.velocity, 5 + i)
		var ai: AIPilot = s.get_node("AI")
		ai.patrol_point = yard.global_position
		fleet.append(s)
	if i > 0:
		say("by_wave")


func _on_yard_died(_s: Ship, _k: Node) -> void:
	if stage != "strike":
		return
	outcome = "crippled"
	complete("yard")
	flag("hammer", "crippled")
	if alive(hammer):
		# the cradle burns with the Hammer in it
		world.fx.explosion(hammer.global_position, 60.0, Color(1.0, 0.6, 0.3))
		world.remove_ship(hammer)
		hammer = null
	say("by_burned")
	say("by_burned2")
	_fall_back("Withdraw: get clear of Mars")
	go("burned")


func _launch() -> void:
	outcome = "launched"
	fail_objective("yard")
	flag("hammer", "launched")
	hammer.invulnerable = 0.0
	_cradle(false)
	hammer.target = lead_pilot()
	say("by_launched")
	say("by_launched2")
	say("by_clear")
	_fall_back("Fall back: get clear of Mars")
	go("launched")


func _fall_back(text: String) -> void:
	objective("clear", text, null, "", false)
	for s in strike:
		if alive(s):
			var ai: AIPilot = s.get_node_or_null("AI")
			if ai != null:
				ai.patrol_point = planet("Earth").global_position
			s.target = null


func _home_check() -> void:
	if pilot_near(mars.global_position, CLEAR) == null:
		complete("clear")
		if outcome == "crippled":
			say("by_done_burned")
			pay(900)
		else:
			say("by_done_launched")
			pay(500)
		win()


static func _clock(t: float) -> String:
	var s := int(ceilf(maxf(t, 0.0)))
	return "%d:%02d" % [s / 60, s % 60]
