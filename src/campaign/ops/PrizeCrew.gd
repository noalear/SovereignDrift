## Operation 12, Prize Crew: the Drengin battleship Iron Oath carries the
## fleet codes for everything Kesh Varr has left at Mars, and patrols with
## two escorts on the sunward side of Mars, away from its guns.  The pilots
## and the Resolute's pair knock out her engines (most of her crew takes to
## the pods), board her, and hold her while Major Rhee's prize crew pulls
## the codes, with Mars sending ships to take her back.  Then the third
## choice: send false orders with the codes (flags.codes "spoof": half the
## Mars fleet chases ghosts to Venus, but Kesh learns his codes are blown)
## or keep quiet and listen ("listen": the Hammer's course is known).
extends StoryOp

const SITE_OFF := 6000.0          # the Iron Oath patrols this far from Mars, sunward
const SITE_STEP := 0.3            # rad round Mars per try when guns are too near
const QUIET := 4500.0             # from any colony's guns or armed station
const ARRIVE_REACH := 3200.0      # from the Iron Oath: the fight starts
const OATH_HULL := 2.5            # her hull, times a Battleship's: her engines go long before she does
const ESCORTS := ["Destroyer", "Destroyer"]
const STRIKE := [["Cruiser", "TSS Resolute", "Cdr. Adeyemi"], ["Destroyer", "TSS Vigilant", "Lt. Cdr. Moss"]]
const SKELETON := 4.0             # crew left aboard once her engines go: the rest take to the pods
const CODES_TIME := 60.0          # seconds for the prize crew to pull the codes
const RESPONSE := [               # [seconds into the download, designs], from Mars
	[8.0, ["Destroyer", "Scout", "Scout"]],
	[35.0, ["Cruiser", "Destroyer"]],
]
const RESPONSE_FROM := 3800.0     # from the prize, on the Mars side
const PRIZE_MEND := 0.5           # Rhee's damage control: the prize holds together at this much hull
const CLEAR := 6000.0             # from where she was taken: clear

var mars: Planet
var site := Vector2.ZERO          # from Mars's centre, so it moves with it
var oath: Ship
var escorts: Array = []
var strike: Array = []
var brandt: Ship
var prize: Ship = null            # the Iron Oath under Rhee's prize crew
var taken_at := Vector2.ZERO
var captured := false
var fleet: Array = []
var wave := 0
var half_said := false


func setup() -> void:
	mars = planet("Mars")
	site = _quiet_site() - mars.global_position
	var at := _site()
	oath = spawn("Captain Durak", 2, "Battleship", "Iron Oath", at, mars.velocity, 10)
	oath.hull_max *= OATH_HULL
	oath.hull = oath.hull_max
	oath.died.connect(_on_oath_died)
	var oai: AIPilot = oath.get_node("AI")
	oai.patrol_point = at
	for k in scaled(ESCORTS.size()):
		var e := spawn(Names.fill("{drengin} %s") % ESCORTS[k % ESCORTS.size()], 2, ESCORTS[k % ESCORTS.size()], "Oath Guard %d" % (k + 1),
			clear_spot(at, k * 1.3, 300.0), mars.velocity, 6)
		e.get_node("AI").guard(oath)
		escorts.append(e)
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var bai: AIPilot = brandt.get_node("AI")
	bai.escort = p
	bai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	for k in STRIKE.size():
		var g := spawn(STRIKE[k][2], 1, STRIKE[k][0], STRIKE[k][1], p.global_position + Vector2(-400 - 200 * k, (k - 0.5) * 300.0).rotated(p.rotation), p.velocity, 7)
		g.get_node("AI").guard(p)
		strike.append(g)
	say("pc_start")
	objective("find", "Find the Iron Oath on patrol sunward of Mars", _site, "Iron Oath")
	go("travel")


## Sunward of Mars, out of reach of its guns, Phobos's, Deimos's and the
## Drengin starbase's: round Mars a step at a time until it is quiet.
func _quiet_site() -> Vector2:
	var sun: Planet = world.sun
	var dir := (sun.global_position - mars.global_position).angle() if sun != null else 0.0
	var best := clear_spot(mars.global_position, dir, SITE_OFF)
	var best_room := guns_near(best)
	for k in 12:
		if best_room >= QUIET:
			break
		var turn := (k / 2 + 1) * SITE_STEP * (1.0 if k % 2 == 0 else -1.0)
		var at := clear_spot(mars.global_position, dir + turn, SITE_OFF)
		var room := guns_near(at)
		if room > best_room:
			best = at
			best_room = room
	return best


func _site() -> Vector2:
	return mars.global_position + site


func step(delta: float) -> void:
	match stage:
		"travel":
			if alive(oath) and pilot_near(oath.global_position, ARRIVE_REACH) != null:
				complete("find")
				say("pc_contact")
				say("pc_engines")
				objective("engines", "Knock out the Iron Oath's engines (shoot her stern)", oath, "Iron Oath")
				for k in strike.size():
					if alive(strike[k]) and not escorts.is_empty():
						strike[k].target = escorts[k % escorts.size()]
				go("fight")
		"fight":
			_hold_together()
			if alive(oath) and oath.is_disabled() and not done("engines"):
				complete("engines")
				oath.crew = minf(oath.crew, SKELETON)
				say("pc_disabled")
				say("pc_board")
				objective("board", "Board the Iron Oath (%s, close and slow)" % Keybinds.short_text("board"), oath, "Iron Oath")
			if captured:
				_take_prize()
		"codes":
			while wave < RESPONSE.size() and stage_time >= RESPONSE[wave][0]:
				_send_wave(wave)
				wave += 1
			set_text("codes", _codes_text(CODES_TIME - stage_time))
			if not half_said and stage_time >= CODES_TIME * 0.5:
				half_said = true
				say("pc_half")
			if stage_time >= CODES_TIME:
				complete("codes")
				say("pc_codes")
				say("pc_choice")
				go("choice")
		"choice":
			if stage_time > 1.0 and not story.speaking() and story.choice.is_empty():
				ask("codes", Names.fill("What do we do with the {drengin} fleet codes?"), [
					["spoof", "Send false orders", "Order half the Mars fleet away to Venus. The siege of Mars is easier, but Kesh learns his codes are blown and the Hammer leaves early."],
					["listen", "Listen quietly", "Say nothing and keep the codes secret. The siege is at full strength, but we learn the Hammer's course and can ambush it."],
				])
				go("asked")
		"clear":
			if pilot_near(taken_at, CLEAR) == null:
				complete("clear")
				say("pc_done")
				pay(800)
				win()


## Her engines go before her hull does, and what is left holds together
## for the boarders.
func _hold_together() -> void:
	if not alive(oath):
		return
	if oath.is_disabled() or oath.hull < oath.hull_max * 0.35:
		keep_down(oath)
	if oath.hull < oath.hull_max * 0.2:
		oath.invulnerable = 0.5


## The boarders have her: she is Rhee's now, under the prize crew, and has
## to be held while the codes come off her.
func _take_prize() -> void:
	complete("board")
	for s in world.ships:
		if s.is_prize and not s.dead and s.ship_name == "Iron Oath":
			prize = s
	if prize == null:
		# no room for another prize: the story brings her along all the same
		prize = spawn("Prize crew", 1, "Battleship", "Iron Oath", taken_at, mars.velocity, -1, 2)
	prize.set_meta("no_boarding", true)
	prize.died.connect(func(_s, _k): fail("The Iron Oath was lost, and the codes with her.", "pc_fail_prize"))
	_mend()
	say("pc_captured")
	say("pc_wave")
	objective("codes", _codes_text(CODES_TIME), prize, "Iron Oath")
	go("codes")


func _mend() -> void:
	if alive(prize):
		prize.hull = maxf(prize.hull, prize.hull_max * PRIZE_MEND)


func _codes_text(left: float) -> String:
	return "Hold the Iron Oath while the prize crew pulls the codes (%s)" % _clock(left)


func on_choice(id: String, key: String) -> void:
	if id != "codes":
		return
	flag("codes", key)
	if key == "spoof":
		say("pc_spoof")
		say("pc_spoof2")
	else:
		say("pc_listen")
	say("pc_clear")
	# Rhee takes her home
	if alive(prize):
		if prize.is_prize:
			world.capture.send_home(prize)
		else:
			world.fx.explosion(prize.global_position, prize.size, Color(0.6, 0.8, 1.0), false)
			Sfx.play("warpout", prize.global_position, 0.6)
			world.remove_ship(prize)
	prize = null
	objective("clear", "Get clear before the rest of Mars comes looking", null)
	go("clear")


func _send_wave(i: int) -> void:
	var at0: Vector2 = prize.global_position if alive(prize) else taken_at
	var dir := (mars.global_position - at0).angle()
	var from := at0 + Vector2(RESPONSE_FROM, 0).rotated(dir)
	var designs: Array = RESPONSE[i][1].duplicate()
	for k in range(designs.size(), scaled(designs.size())):
		designs.append(RESPONSE[i][1][k % RESPONSE[i][1].size()])
	for k in designs.size():
		var at := clear_spot(from + Vector2(0, (k - designs.size() * 0.5) * 180.0).rotated(dir), dir, 1.0)
		var s := spawn(Names.fill("{drengin} %s") % designs[k], 2, designs[k], "Claw of Varr %d-%d" % [i + 1, k + 1], at, mars.velocity, 5 + i)
		s.rotation = dir + PI
		s.reset_physics_interpolation()
		var ai: AIPilot = s.get_node("AI")
		ai.duel = true                  # they came for the Iron Oath
		ai.patrol_point = at0
		if alive(prize):
			s.target = prize
		fleet.append(s)
	if i > 0:
		say("pc_wave2")


func _on_oath_died(s: Ship, killer: Node) -> void:
	taken_at = s.global_position
	if s.captured and killer is Ship and killer.race_id == 1:
		captured = true
	elif not captured:
		fail("The Iron Oath was destroyed before anyone could board her.", "pc_fail_lost")


static func _clock(t: float) -> String:
	var s := int(ceilf(maxf(t, 0.0)))
	return "%d:%02d" % [s / 60, s % 60]
