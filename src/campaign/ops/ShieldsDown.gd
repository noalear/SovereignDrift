## Operation 6, Shields Down: Deimos is a Drengin forward base now.  The
## pilots and a Terran strike group shoot its planetary shield down while
## Okafor's fleet keeps Mars, Phobos and the Drengin starbase busy; the
## drop ships Anvil and Bastion wait well back until a pilot calls them in
## (the story key), put Major Rhee's troops on the ground while the shield
## is down, and the squad covers the ground battle until Deimos is Terran.
extends StoryOp

const SHIELD := 3000.0            # Deimos's planetary shield as the operation begins
const GARRISON := {"infantry": 3.0, "armour": 1.0}
const TROOPS := {"infantry": 4.0, "armour": 2.0}   # aboard each drop ship
const DROP_HULL := 2.5            # the drop ships' hulls, times their design's
const HOLD_OFF := 1800.0          # called in, the drop ships hold this far from Deimos for the shield
const STAGING := 7500.0           # until then they wait this far out, beyond Mars from Deimos
const SUPPRESS := 5.0             # seconds of silence Mars, Phobos and the starbase get each step
const CONTACT := 5000.0           # a pilot this close to Deimos: the defenders come up
const DESCEND_REACH := 700.0      # from Deimos's surface: the drop ships go down
const DEFENDERS := ["Destroyer", "Scout", "Scout"]
## The strike group that flies with the squad, and the drop ships' escort.
const STRIKE := [["Cruiser", "TSS Resolute", "Cdr. Adeyemi"], ["Destroyer", "TSS Vigilant", "Lt. Cdr. Moss"], ["Destroyer", "TSS Steadfast", "Lt. Cdr. Faro"]]
const ESCORT := [["Destroyer", "TSS Valiant", "Lt. Cdr. Imre"], ["Destroyer", "TSS Tenacity", "Lt. Cdr. Sato"]]
const REINFORCE := ["Destroyer", "Scout"]
const REINFORCE_AFTER := 25.0     # seconds into the ground battle

var deimos: Planet
var mars: Planet
var brandt: Ship
var droppers: Array = []
var hold: Waypoint                # where the drop ships wait for the shield, riding with Deimos
var staging: Waypoint             # where they wait to be called in
var called := false               # a pilot has called the drop ships in
var strike: Array = []
var escort: Array = []
var quiet: Array = []             # Mars, Phobos: busy with Okafor's fleet
var fleet: Array = []
var landed := {}                  # drop ship -> true once its troops are ashore
var reinforced := false


func setup() -> void:
	deimos = planet("Deimos")
	mars = planet("Mars")
	_fortify()
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = p
	ai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	# the drop ships wait well out on the far side of Deimos from Mars
	var away := (deimos.global_position - mars.global_position).normalized() if mars != null else Vector2.RIGHT
	hold = waypoint("Drop ships' holding point", deimos, 80.0, away.rotated(0.6) * (deimos.radius + HOLD_OFF))
	staging = waypoint("Drop ships' staging point", deimos, 120.0, away.rotated(0.6) * (deimos.radius + STAGING))
	for i in 2:
		var at: Vector2 = staging.global_position + Vector2(0, -200 + 400 * i).rotated(away.angle())
		var d := spawn("Drop pilot", 1, "DropShip", ["TSS Anvil", "TSS Bastion"][i], at, deimos.velocity)
		d.hull_max *= DROP_HULL
		d.hull = d.hull_max
		d.troops = TROOPS.duplicate()
		d.rotation = (deimos.global_position - at).angle()
		d.died.connect(_on_dropper_died)
		droppers.append(d)
	for k in ESCORT.size():
		var e := spawn(ESCORT[k][2], 1, ESCORT[k][0], ESCORT[k][1], staging.global_position + Vector2(-300, -300 + 600 * k).rotated(away.angle()), deimos.velocity, 6)
		e.get_node("AI").guard(droppers[k])
		escort.append(e)
	# the strike group flies with the flight lead
	for k in STRIKE.size():
		var g := spawn(STRIKE[k][2], 1, STRIKE[k][0], STRIKE[k][1], p.global_position + Vector2(-400 - 200 * k, (k - 1) * 260.0).rotated(p.rotation), p.velocity, 6)
		g.get_node("AI").guard(p)
		strike.append(g)
	quiet = [mars, planet("Phobos")]
	_suppress()
	say("sd_start")
	say("sd_rhee")
	say("sd_support")
	objective("approach", "Fly to Deimos with the drop ships Anvil and Bastion", deimos, "Deimos")
	go("approach")


## Deimos as the Drengin hold it: a colony with a planetary shield and a
## garrison, whatever the open war has done to it.
func _fortify() -> void:
	if not deimos.is_colonised() or deimos.owner_race != 2:
		deimos.surrender_to(2, maxf(deimos.population, GameData.MINOR_COLONY))
	deimos.battle = {}
	if not deimos.buildings.has("shield"):
		deimos.buildings.append("shield")
	deimos.pshield = SHIELD
	deimos.pshield_down = 0.0
	deimos.stock["fuel"] = maxf(deimos.stock.get("fuel", 0.0), 60.0)
	deimos.garrison = GARRISON.duplicate()
	deimos.fit_batteries()
	Net.host_colony_changed(deimos)


func step(delta: float) -> void:
	match stage:
		"approach":
			_suppress()
			_keep_drop_ships()
			if pilot_near(deimos.global_position, CONTACT) != null:
				complete("approach")
				_defend()
				say("sd_contact")
				say("sd_shield")
				objective("shield", "Shoot down Deimos's planetary shield", deimos, "Deimos shield")
				if not called:
					objective("call", "Call the drop ships in when the way is clear (%s)" % Keybinds.short_text("briefing"), _first_dropper, "Drop ships")
				objective("droppers", "Keep the drop ships alive", null, "", true)
				go("shield")
		"shield":
			_suppress()
			_keep_drop_ships()
			_shield_text()
			if not deimos.pshield_up():
				complete("shield")
				say("sd_down")
				say("sd_drop")
				objective("land", "Cover the drop ships down to Deimos before the shield comes back", _first_dropper, "Drop ships")
				go("drop")
		"drop":
			_suppress()
			if not called:
				_keep_drop_ships()
			if deimos.pshield_up():
				# too slow: back to the holding point and knock it down again
				say("sd_again")
				set_text("shield", "Shoot Deimos's shield down again")
				for o in objectives:
					if o["id"] == "shield":
						o["done"] = false
				story.changed.emit()
				go("shield")
				return
			for d in droppers:
				if not called or not alive(d) or landed.has(d):
					continue
				if not d.is_landing():
					d.nav_target = deimos
					d.autopilot = true
					if d.global_position.distance_to(deimos.global_position) - deimos.radius < DESCEND_REACH:
						d.start_landing(deimos)
						Net.host_landing(d)
				elif d.is_landed():
					landed[d] = true
					world.troop_command(d, "land")
					if landed.size() == 1:
						say("sd_landed")
			if not landed.is_empty() and not deimos.battle.is_empty():
				complete("land")
				objective("ground", "Cover the ground battle on Deimos from orbit", deimos, "Deimos")
				go("ground")
		"ground":
			_suppress()
			# the second drop ship still comes down if it can
			for d in droppers:
				if alive(d) and not landed.has(d):
					if not deimos.pshield_up() and not d.is_landing() and d.global_position.distance_to(deimos.global_position) - deimos.radius < DESCEND_REACH:
						d.start_landing(deimos)
						Net.host_landing(d)
					elif d.is_landed():
						landed[d] = true
						world.troop_command(d, "land")
					elif not d.is_landing():
						d.nav_target = deimos if not deimos.pshield_up() else hold
						d.autopilot = true
			if deimos.owner_race == 1:
				complete("ground")
				complete("droppers")
				say("sd_taken")
				say("sd_done")
				flag("deimos", "taken")
				pay(700)
				win()
				return
			if deimos.battle.is_empty() and _troops_left() == 0:
				fail("The landing on Deimos was beaten back.", "sd_fail_beaten")
				return
			if not reinforced and stage_time > REINFORCE_AFTER:
				reinforced = true
				_reinforce()
			if stage_time > 12.0:
				say_once("sd_battle")


## Until called, the drop ships sit at the staging point; called, at the
## holding point (warping in) until the shield is down.
func _keep_drop_ships() -> void:
	var at: Waypoint = hold if called else staging
	for d in droppers:
		if alive(d) and not d.is_landing():
			d.warp_block = 0.0 if called else 999.0
			if d.nav_target != at or (not d.autopilot and d.global_position.distance_to(at.global_position) > 300.0):
				d.nav_target = at
				d.autopilot = true


## A pilot pressed the story key: the drop ships come in.
func signal_label() -> String:
	return "" if called or stage in ["ground"] else "Call the drop ships in"


func on_signal(by: Ship) -> void:
	if called or stage == "ground":
		return
	called = true
	complete("call")
	world.notify_all("%s calls the drop ships in." % by.pilot_name, Color(0.7, 1.0, 0.75))
	say("sd_called")
	for d in droppers:
		if alive(d):
			d.energy = d.energy_max
			d.warp_block = 0.0
			d.nav_target = hold if deimos.pshield_up() else deimos
			d.autopilot = true


## Okafor's fleet is hitting Mars from the far side: Mars's and Phobos's
## guns and the Drengin starbase are busy with it, not with Deimos.
func _suppress() -> void:
	for pl in quiet:
		if pl != null:
			pl.silenced = maxf(pl.silenced, SUPPRESS)
	var sb: Ship = world.starbase_of(2)
	if sb != null:
		for c in sb.get_children():
			if c is AIPilot:
				c.set_physics_process(false)
		sb.target = null
		for k in ["fire_primary", "fire_secondary", "fire_missile", "fire_beam"]:
			if sb.inputs.has(k):
				sb.inputs[k] = false


func cleanup() -> void:
	# a starbase shot down meanwhile is still benched for its rebuild: wake
	# its pilot too, or it comes back with its guns asleep
	for sb in world.ships:
		if sb.is_starbase() and sb.race_id == 2:
			for c in sb.get_children():
				if c is AIPilot:
					c.set_physics_process(true)
	super.cleanup()


func _shield_text() -> void:
	set_text("shield", "Shoot down Deimos's planetary shield (%d%%)" % int(clampf(deimos.pshield / Ground.PSHIELD_HP, 0.0, 1.0) * 100.0))


func _first_dropper() -> Variant:
	for d in droppers:
		if alive(d) and not landed.has(d):
			return d
	return null


## Troops still aboard a drop ship that has not landed, or fighting.
func _troops_left() -> int:
	var n := 0
	for d in droppers:
		if alive(d) and not landed.has(d):
			n += d.troop_count()
	return n


func _defend() -> void:
	var a := randf() * TAU
	var list: Array = DEFENDERS.duplicate()
	for k in range(list.size(), scaled(list.size())):
		list.append(DEFENDERS[k % DEFENDERS.size()])
	for k in list.size():
		var at := clear_spot(deimos.global_position, a + k * 0.5, deimos.radius + 900.0)
		var s := spawn("Drengin %s" % list[k], 2, list[k], "Deimos Guard %d" % (k + 1), at, deimos.velocity, 5)
		var ai: AIPilot = s.get_node("AI")
		ai.patrol_point = deimos.global_position
		fleet.append(s)


func _reinforce() -> void:
	var dir := (deimos.global_position - mars.global_position).angle() + PI if mars != null else 0.0
	for k in scaled(REINFORCE.size()):
		var design: String = REINFORCE[k % REINFORCE.size()]
		var at := clear_spot(deimos.global_position, dir + k * 0.3, deimos.radius + 3500.0)
		var s := spawn("Drengin %s" % design, 2, design, "Claw of Varr %d" % (k + 1), at, deimos.velocity, 6)
		var ai: AIPilot = s.get_node("AI")
		ai.patrol_point = deimos.global_position
		fleet.append(s)
	say("sd_reinforce")


func _on_dropper_died(_s: Ship, _k: Node) -> void:
	fail_objective("droppers")
	for d in droppers:
		if alive(d) and not landed.has(d):
			say("sd_lost_one")
			return
	if landed.is_empty():
		fail("Both drop ships were lost.", "sd_fail_droppers")
