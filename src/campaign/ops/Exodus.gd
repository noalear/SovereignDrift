## Operation 11, Exodus: Kesh Varr has promised Luna fire.  Three
## transports go up from Earth to bring its people home: the pilots escort
## them to Luna, cover them while shuttles fill them beside it, and send
## them home with the story key when the way is clear.  Drengin bombers
## come from Mars all the while.  If the Hammer got out of its cradle (Burn
## the Yards) it comes for the transports as well, too strong to fight, so
## waiting too long is fatal; if it was crippled, bombers cut across the run
## home instead.  Luna lost (The Accord Breaks): the people are Terran
## holdouts in the old mine tunnels, and Rhee's commandos keep the Drengin
## guns there quiet.  Two of the three transports must reach Earth.
extends StoryOp

const TRANSPORTS := ["TSS Haven", "TSS Meridian", "TSS Providence"]
const TRANSPORT_HULL := 2.0       # the transports' hulls, times a Freighter's
const ESCORT := [["Destroyer", "TSS Valiant", "Lt. Cdr. Imre"], ["Destroyer", "TSS Tenacity", "Lt. Cdr. Sato"]]
const NEED := 2                   # transports home for the operation to count
const PICKUP_OFF := 260.0         # the pickup point, this far over Luna's surface on its Earth side
const ON_STATION := 320.0         # from the pickup point: loading
const LOAD_TIME := 50.0           # seconds on station to fill a transport
const WAVES := [                  # [seconds into the loading, designs], from Mars
	[4.0, ["Destroyer", "Scout", "Scout"]],
	[40.0, ["Cruiser", "Destroyer", "Scout"]],
]
const WAIT_WAVE := ["Destroyer", "Scout"]
const WAIT_EVERY := 35.0          # full and waiting for the word: another wave this often
const WAVE_FROM := 4200.0         # from Luna, on the Mars side
const RUN_WAVE := ["Destroyer", "Destroyer", "Scout"]   # no Hammer: they cut across the run home
const RUN_WAVE_AFTER := 6.0
const HAMMER_AFTER := 60.0        # seconds into the loading when the launched Hammer is seen
const HAMMER_FROM := 9000.0       # from Luna, on the Mars side
const HOME_REACH := 700.0         # from Earth's surface: the transport goes down

var luna: Planet
var earth: Planet
var lost := false                 # Luna is Drengin: the holdouts
var pickup: Waypoint
var brandt: Ship
var transports: Array = []
var escort: Array = []
var filled := {}                  # transport -> seconds on station
var load_t := 0.0                 # seconds since the loading began
var saved := {}                   # transport -> true once it is going down at Earth
var fleet: Array = []
var hammer: Ship = null
var wave := 0
var last_wave := 0.0              # stage_time of the last wave while waiting
var run_waved := false
var half_said := false


func setup() -> void:
	luna = planet("Luna")
	earth = planet("Earth")
	lost = story.flags.get("luna", "") == "lost"
	if not lost and (not luna.is_colonised() or luna.owner_race != 1):
		luna.surrender_to(1, maxf(luna.population, GameData.MINOR_COLONY))
	lost = luna.owner_race != 1
	var to_earth := (earth.global_position - luna.global_position).normalized()
	pickup = waypoint("Pickup point over Luna", luna, 80.0, to_earth * (luna.radius + PICKUP_OFF))
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var bai: AIPilot = brandt.get_node("AI")
	bai.escort = p
	bai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	for k in TRANSPORTS.size():
		var at := p.global_position + Vector2(260.0 + 140.0 * k, (k - 1) * 220.0).rotated(p.rotation)
		var t := spawn("Transport pilot", 1, "Freighter", TRANSPORTS[k], at, p.velocity)
		t.hull_max *= TRANSPORT_HULL
		t.hull = t.hull_max
		t.rotation = p.rotation
		t.reset_physics_interpolation()
		t.died.connect(_on_transport_died)
		transports.append(t)
		filled[t] = 0.0
	for k in ESCORT.size():
		var e := spawn(ESCORT[k][2], 1, ESCORT[k][0], ESCORT[k][1], transports[k].global_position + Vector2(-200, 160 - 320 * k).rotated(p.rotation), p.velocity, 6)
		e.get_node("AI").guard(transports[k])
		escort.append(e)
	_hush_luna()
	say("ex_start")
	say("ex_ruiz")
	objective("escort", "Escort the transports up to Luna", _first_flying, "Transports")
	objective("all", "Bring all three transports home", null, "", true)
	go("out")


func step(delta: float) -> void:
	_hush_luna()
	if alive(hammer):
		hammer.invulnerable = 1.0       # out here nothing we have gets through its shields
	match stage:
		"out":
			_keep_station()
			for t in transports:
				if alive(t) and t.global_position.distance_to(pickup.global_position) < ON_STATION:
					complete("escort")
					say("ex_arrive")
					objective("load", _load_text(), luna, "Luna")
					go("load")
					return
		"load":
			_keep_station()
			load_t += delta
			_loading(delta)
			while wave < WAVES.size() and stage_time >= WAVES[wave][0]:
				_send_wave(WAVES[wave][1], wave)
				wave += 1
			_hammer_check()
			set_text("load", _load_text())
			if not half_said and _progress() >= 0.5:
				half_said = true
				say("ex_half")
			if _progress() >= 1.0:
				complete("load")
				say("ex_loaded")
				say("ex_signal")
				objective("send", "Send the transports home when the way is clear (%s)" % Keybinds.short_text("briefing"), luna, "Luna")
				last_wave = 0.0
				go("loaded")
		"loaded":
			_keep_station()
			load_t += delta
			_hammer_check()
			if stage_time - last_wave >= WAIT_EVERY:
				last_wave = stage_time
				_send_wave(WAIT_WAVE, wave)
				wave += 1
		"run":
			for t in transports:
				if not alive(t) or saved.has(t):
					continue
				if not t.is_landing():
					t.nav_target = earth
					t.autopilot = true
					if t.global_position.distance_to(earth.global_position) - earth.radius < HOME_REACH:
						t.start_landing(earth)
						Net.host_landing(t)
						saved[t] = true
						say("ex_landed")
			set_text("home", _home_text())
			if not run_waved and hammer == null and stage_time > RUN_WAVE_AFTER:
				run_waved = true
				_send_wave(RUN_WAVE, wave, earth.global_position.lerp(luna.global_position, 0.5))
				say("ex_run_wave")
			if saved.size() >= NEED and saved.size() == _alive_count():
				complete("home")
				if saved.size() == TRANSPORTS.size():
					complete("all")
				flag("evacuated", saved.size())
				say("ex_done")
				say("ex_done2")
				pay(700 + 150 * saved.size())
				win()


## Luna lost: Rhee's commandos keep its Drengin guns quiet.
func _hush_luna() -> void:
	if lost:
		luna.silenced = maxf(luna.silenced, 5.0)


## Until the word is given the transports sit at the pickup point.
func _keep_station() -> void:
	for t in transports:
		if alive(t) and not t.is_landing():
			if t.nav_target != pickup or (not t.autopilot and t.global_position.distance_to(pickup.global_position) > ON_STATION * 0.6):
				t.nav_target = pickup
				t.autopilot = true


func _loading(delta: float) -> void:
	for t in transports:
		if alive(t) and t.global_position.distance_to(pickup.global_position) < ON_STATION:
			filled[t] = minf(filled[t] + delta, LOAD_TIME)


## How full the living transports are, 0..1.
func _progress() -> float:
	var n := 0
	var sum := 0.0
	for t in transports:
		if alive(t):
			n += 1
			sum += filled[t] / LOAD_TIME
	return sum / n if n > 0 else 0.0


func _load_text() -> String:
	return "Cover the transports while they load beside Luna (%d%%)" % int(_progress() * 100.0)


func _home_text() -> String:
	return "Get the transports home to Earth (%d of %d down)" % [saved.size(), _alive_count()]


func _alive_count() -> int:
	var n := 0
	for t in transports:
		if alive(t):
			n += 1
	return n


func _first_flying() -> Variant:
	for t in transports:
		if alive(t) and not saved.has(t):
			return t
	return null


## A pilot pressed the story key: the full transports run for Earth.
func signal_label() -> String:
	return "Send the transports home" if stage == "loaded" else ""


func on_signal(by: Ship) -> void:
	if stage != "loaded":
		return
	complete("send")
	world.notify_all("%s sends the transports home." % by.pilot_name, Color(0.7, 1.0, 0.75))
	say("ex_go")
	for t in transports:
		if alive(t):
			t.nav_target = earth
			t.autopilot = true
	objective("home", _home_text(), _first_flying, "Transports")
	go("run")


## The Hammer flew (Burn the Yards): a minute into the loading it is seen
## coming from Mars, and it comes for the transports.
func _hammer_check() -> void:
	if hammer != null or story.flags.get("hammer", "") != "launched" or load_t < HAMMER_AFTER:
		return
	var mars := planet("Mars")
	var dir := (mars.global_position - luna.global_position).normalized() if mars != null else Vector2.RIGHT
	hammer = spawn("Kesh Varr", 2, "Dreadnought", "Hammer of Mars", luna.global_position + dir * HAMMER_FROM, luna.velocity, 14)
	hammer.set_meta("no_boarding", true)
	hammer.invulnerable = 1.0
	hammer.rotation = dir.angle() + PI
	hammer.reset_physics_interpolation()
	var ai: AIPilot = hammer.get_node("AI")
	ai.duel = true
	ai.patrol_point = pickup.global_position
	var t0: Variant = _first_flying()
	if t0 != null:
		hammer.target = t0
	say("ex_hammer")
	say("ex_hammer2")
	say("ex_kesh")


func _send_wave(designs_in: Array, i: int, at_point: Vector2 = Vector2.INF) -> void:
	var mars := planet("Mars")
	var dir := (mars.global_position - luna.global_position).angle() if mars != null else 0.0
	var from := luna.global_position + Vector2(WAVE_FROM, 0).rotated(dir)
	if at_point != Vector2.INF:
		from = at_point + Vector2(WAVE_FROM * 0.7, 0).rotated(dir)
	var designs: Array = designs_in.duplicate()
	for k in range(designs.size(), scaled(designs.size())):
		designs.append(designs_in[k % designs_in.size()])
	var marks := transports.filter(func(t): return alive(t) and not saved.has(t))
	for k in designs.size():
		var at := from + Vector2((k % 3) * -160.0, (k - designs.size() * 0.5) * 180.0).rotated(dir)
		var s := spawn(Names.fill("{drengin} %s") % designs[k], 2, designs[k], "Claw of Varr %d-%d" % [i + 1, k + 1], at, luna.velocity, 5 + mini(i, 3))
		s.rotation = dir + PI
		s.reset_physics_interpolation()
		var ai: AIPilot = s.get_node("AI")
		ai.duel = true                  # they came for the transports
		ai.patrol_point = pickup.global_position if at_point == Vector2.INF else at_point
		if not marks.is_empty():
			s.target = marks[k % marks.size()]
		fleet.append(s)
	if i == 0:
		say("ex_wave1")
	elif at_point == Vector2.INF:
		say("ex_wave")


func _on_transport_died(s: Ship, _k: Node) -> void:
	fail_objective("all")
	if _alive_count() < NEED:
		fail("Too many transports were lost.", "ex_fail")
	elif not s.captured:
		say("ex_lost_one")
