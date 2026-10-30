## Operation 13, Siege of Mars: Admiral Okafor's fleet goes for Mars, with
## Arcean ships beside it if Speaker Tenn was shown the evidence (The
## Envoy).  The battleship Dauntless stands off Mars's far side and keeps
## its batteries and the Drengin starbase's guns pinned while it lives; the
## pilots, with the fleet flying on them, break the starbase and shoot
## Mars's planetary shield down, call Major Rhee's three drop ships in with
## the story key, and cover the ground battle until Mars is Terran.  False
## orders sent with the Iron Oath's codes (flags.codes "spoof") have half
## the home fleet off at Venus.  As Mars falls the Hammer runs for the Sun.
extends StoryOp

const GARRISON := {"infantry": 6.0, "armour": 2.0}
const TROOPS := {"infantry": 4.0, "armour": 2.0}   # aboard each drop ship: two landings beat the garrison
const DROPS := ["TSS Anvil", "TSS Bastion", "TSS Citadel"]
const DROP_HULL := 2.5            # the drop ships' hulls, times their design's
const HOLD_OFF := 1800.0          # called in, the drop ships hold this far from Mars for the shield
const STAGING := 7500.0           # until then they wait this far out, on the pilots' side
const SUPPRESS := 5.0             # seconds of silence the pinned guns get each step
const CONTACT := 6000.0           # a pilot this close to Mars: the home fleet comes up
const DESCEND_REACH := 700.0      # from Mars's surface: the drop ships go down
const STARBASE_HULL := 0.3        # of its hull left when the siege begins: the fleet has been at it all night
## Okafor's fleet, flying on the flight lead: [design, name, captain].
const FLEET := [["Cruiser", "TSS Resolute", "Cdr. Adeyemi"], ["Destroyer", "TSS Vigilant", "Lt. Cdr. Moss"],
	["Destroyer", "TSS Steadfast", "Lt. Cdr. Faro"]]
const DAUNTLESS_HULL := 4.0       # the Dauntless's hull, times a Battleship's
const DAUNTLESS_OFF := 5500.0     # it stands this far off Mars, on the far side from the pilots
const ESCORT := [["Destroyer", "TSS Valiant", "Lt. Cdr. Imre"], ["Destroyer", "TSS Tenacity", "Lt. Cdr. Sato"]]
## The Arcean Union keeps its word (flags.arceans "allied").
const ARCEANS := [["Cruiser", "AUS Deep Current"], ["Destroyer", "AUS Undertow"], ["Destroyer", "AUS Tidewalker"]]
const DEFENDERS := ["Battleship", "Cruiser", "Destroyer", "Destroyer", "Scout", "Scout"]
const SPOOFED := ["Cruiser", "Destroyer", "Scout"]   # what is left at home after the false orders
const REINFORCE := ["Destroyer", "Destroyer", "Scout"]
const REINFORCE_AFTER := 25.0     # seconds into the ground battle, from Venus
const FLEE_TIME := 9.0            # seconds the Hammer is seen running before the operation ends
const HAMMER_FLEE := 170.0        # its speed toward the Sun

var mars: Planet
var base: Ship                    # the Drengin starbase
var brandt: Ship
var fleet: Array = []             # Okafor's ships and the Arceans
var dauntless: Ship
var arceans: Array = []
var droppers: Array = []
var escort: Array = []
var hold: Waypoint
var staging: Waypoint
var called := false
var landed := {}
var enemies: Array = []
var reinforced := false
var hammer: Ship = null
var pinned := true                # the Dauntless still has Mars's and the starbase's guns pinned


func setup() -> void:
	mars = planet("Mars")
	_fortify()
	base = world.starbase_of(2)
	if base != null:
		base.hull = base.hull_max * STARBASE_HULL
		base.died.connect(_on_base_died)
	var p := lead_pilot()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var bai: AIPilot = brandt.get_node("AI")
	bai.escort = p
	bai.state = AIPilot.State.ESCORT
	brandt.died.connect(func(s, _k):
		if not s.captured:
			say("brandt_down"))
	# the fleet flies with the flight lead all the way
	var rows: Array = FLEET.duplicate()
	if story.flags.get("arceans", "") == "allied":
		for a in ARCEANS:
			rows.append([a[0], a[1], Names.fill("{arcean} Captain"), true])
	for k in rows.size():
		var arcean: bool = rows[k].size() > 3
		var at: Vector2 = p.global_position + Vector2(-420 - 220 * (k / 3), ((k % 3) - 1) * 280.0).rotated(p.rotation)
		var s := spawn(rows[k][2], 0 if arcean else 1, rows[k][0], rows[k][1], at, p.velocity, 8, 0 if arcean else -1)
		s.get_node("AI").guard(p)
		fleet.append(s)
		if arcean:
			arceans.append(s)
	# the Dauntless stands off the far side of Mars, pinning its guns
	var far := (mars.global_position - p.global_position).normalized()
	dauntless = spawn("Capt. Ward", 1, "Battleship", "TSS Dauntless", mars.global_position + far * DAUNTLESS_OFF, mars.velocity, 8)
	dauntless.rotation = (-far).angle()
	dauntless.reset_physics_interpolation()
	dauntless.get_node("AI").patrol_point = dauntless.global_position
	dauntless.hull_max *= DAUNTLESS_HULL
	dauntless.hull = dauntless.hull_max
	dauntless.died.connect(_on_dauntless_died)
	# the drop ships wait well out on the pilots' side of Mars
	var away := (p.global_position - mars.global_position).normalized()
	hold = waypoint("Drop ships' holding point", mars, 80.0, away.rotated(0.5) * (mars.radius + HOLD_OFF))
	staging = waypoint("Drop ships' staging point", mars, 120.0, away.rotated(0.5) * (mars.radius + STAGING))
	for i in DROPS.size():
		var at: Vector2 = staging.global_position + Vector2(0, -300 + 300 * i).rotated(away.angle())
		var d := spawn("Drop pilot", 1, "DropShip", DROPS[i], at, mars.velocity)
		d.hull_max *= DROP_HULL
		d.hull = d.hull_max
		d.troops = TROOPS.duplicate()
		d.rotation = (mars.global_position - at).angle()
		d.died.connect(_on_dropper_died)
		droppers.append(d)
	for k in ESCORT.size():
		var e := spawn(ESCORT[k][2], 1, ESCORT[k][0], ESCORT[k][1], staging.global_position + Vector2(-300, -300 + 600 * k).rotated(away.angle()), mars.velocity, 6)
		e.get_node("AI").guard(droppers[k])
		escort.append(e)
	_suppress()
	say("sm_start")
	if not arceans.is_empty():
		say("sm_tenn")
	say("sm_rhee")
	objective("approach", "Fly to Mars with Admiral Okafor's fleet", mars, "Mars")
	objective("dauntless", "Keep the Dauntless in the fight", dauntless, "TSS Dauntless", true)
	go("approach")


## Mars as the Drengin hold it, whatever the war has done to it: a home
## world behind a planetary shield, with a garrison dug in.
func _fortify() -> void:
	if not mars.is_colonised() or mars.owner_race != 2:
		mars.surrender_to(2, maxf(mars.population, GameData.MINOR_COLONY))
	mars.battle = {}
	if not mars.buildings.has("shield"):
		mars.buildings.append("shield")
	mars.pshield = Ground.PSHIELD_HP
	mars.pshield_down = 0.0
	mars.stock["fuel"] = maxf(mars.stock.get("fuel", 0.0), 60.0)
	mars.garrison = GARRISON.duplicate()
	mars.fit_batteries()
	Net.host_colony_changed(mars)


func step(_delta: float) -> void:
	_suppress()
	_hold_base()
	match stage:
		"approach":
			_keep_drop_ships()
			if pilot_near(mars.global_position, CONTACT) != null:
				complete("approach")
				_defend()
				say("sm_contact" if story.flags.get("codes", "") != "spoof" else "sm_contact_few")
				say("sm_targets")
				if alive(base):
					objective("base", Names.fill("Break the {drengin} starbase"), base, Names.fill("{drengin} starbase"))
				objective("shield", _shield_text(), mars, "Mars shield")
				if not called:
					objective("call", "Call the drop ships in when the way is clear (%s)" % Keybinds.short_text("briefing"), _first_dropper, "Drop ships")
				objective("droppers", "Get two drop ships down", null, "", true)
				go("shield")
		"shield":
			_keep_drop_ships()
			set_text("shield", _shield_text())
			if not mars.pshield_up():
				complete("shield")
				say("sm_down")
				objective("land", "Cover the drop ships down to Mars before the shield comes back", _first_dropper, "Drop ships")
				go("drop")
		"drop":
			if not called:
				_keep_drop_ships()
			if mars.pshield_up() and landed.is_empty():
				say("sm_again")
				set_text("shield", "Shoot Mars's shield down again")
				for o in objectives:
					if o["id"] == "shield":
						o["done"] = false
				story.changed.emit()
				go("shield")
				return
			_land_droppers()
			if not landed.is_empty() and not mars.battle.is_empty():
				complete("land")
				objective("ground", "Cover the ground battle on Mars from orbit", mars, "Mars")
				go("ground")
		"ground":
			_land_droppers()
			if mars.owner_race == 1:
				_taken()
				return
			if mars.battle.is_empty() and _troops_left() == 0:
				fail("The landing on Mars was beaten back.", "sm_fail_beaten")
				return
			if not reinforced and stage_time > REINFORCE_AFTER:
				reinforced = true
				_reinforce()
			if stage_time > 12.0:
				say_once("sm_battle")
		"flee":
			if alive(hammer):
				hammer.invulnerable = 1.0
				var sun: Planet = world.sun
				var to_sun := (sun.global_position - hammer.global_position).normalized()
				hammer.velocity = mars.velocity + to_sun * HAMMER_FLEE
				hammer.body.vel = hammer.velocity
				hammer.rotation = to_sun.angle()
			if stage_time > FLEE_TIME:
				win()


## Called in and the shield down, the drop ships go for the surface; a
## landed one puts its troops ashore.
func _land_droppers() -> void:
	for d in droppers:
		if not called or not alive(d) or landed.has(d):
			continue
		if not d.is_landing():
			d.nav_target = mars if not mars.pshield_up() else hold
			d.autopilot = true
			if not mars.pshield_up() and d.global_position.distance_to(mars.global_position) - mars.radius < DESCEND_REACH:
				d.start_landing(mars)
				Net.host_landing(d)
		elif d.is_landed():
			landed[d] = true
			world.troop_command(d, "land")
			if landed.size() == 1:
				say("sm_landed")
			if landed.size() >= 2:
				complete("droppers")


func _taken() -> void:
	complete("ground")
	flag("mars", "taken")
	story.keep_mars()
	say("sm_taken")
	say("sm_hammer")
	say("sm_kesh")
	say("sm_done")
	pay(1200)
	# the Hammer lifts off the far side of Mars and runs for the Sun
	var p := lead_pilot()
	var away := (mars.global_position - p.global_position).normalized() if alive(p) else Vector2.RIGHT
	hammer = spawn("Kesh Varr", 2, "Dreadnought", "Hammer of Mars", mars.global_position + away * (mars.radius + 900.0), mars.velocity, 14)
	hammer.set_meta("no_boarding", true)
	hammer.invulnerable = 1.0
	var ai: AIPilot = hammer.get_node_or_null("AI")
	if ai != null:
		ai.set_physics_process(false)
		ai.set_process(false)
	go("flee")


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
	return "" if called or stage in ["approach", "ground", "flee"] else "Call the drop ships in"


func on_signal(by: Ship) -> void:
	if called or stage in ["approach", "ground", "flee"]:
		return
	called = true
	complete("call")
	world.notify_all("%s calls the drop ships in." % by.pilot_name, Color(0.7, 1.0, 0.75))
	say("sm_called")
	for d in droppers:
		if alive(d):
			d.energy = d.energy_max
			d.warp_block = 0.0
			d.nav_target = hold if mars.pshield_up() else mars
			d.autopilot = true


## While the Dauntless fights, Mars's batteries and the Drengin starbase's
## guns are busy with it; Phobos is the cruisers' to watch all along.
func _suppress() -> void:
	var phobos := planet("Phobos")
	if phobos != null and phobos.owner_race != 1:
		phobos.silenced = maxf(phobos.silenced, SUPPRESS)
	if not pinned:
		return
	mars.silenced = maxf(mars.silenced, SUPPRESS)
	if alive(base):
		_base_ai(false)
		base.target = null
		for k in ["fire_primary", "fire_secondary", "fire_missile", "fire_beam"]:
			if base.inputs.has(k):
				base.inputs[k] = false


func _base_ai(on: bool) -> void:
	if base == null or not is_instance_valid(base):
		return
	for c in base.get_children():
		if c is AIPilot:
			c.set_physics_process(on)


## A broken starbase does not relaunch over Mars while the siege is on.
func _hold_base() -> void:
	if base == null or not is_instance_valid(base) or not base.dead:
		return
	for e in world.respawn_queue:
		if e[0] == base:
			e[1] = INF


func _shield_text() -> String:
	return "Shoot down Mars's planetary shield (%d%%)" % int(clampf(mars.pshield / Ground.PSHIELD_HP, 0.0, 1.0) * 100.0)


func _first_dropper() -> Variant:
	for d in droppers:
		if alive(d) and not landed.has(d):
			return d
	return null


## Troops still aboard a drop ship that has not landed.
func _troops_left() -> int:
	var n := 0
	for d in droppers:
		if alive(d) and not landed.has(d):
			n += d.troop_count()
	return n


func _defend() -> void:
	var list: Array = (SPOOFED if story.flags.get("codes", "") == "spoof" else DEFENDERS).duplicate()
	var base_list := list.duplicate()
	for k in range(list.size(), scaled(list.size())):
		list.append(base_list[k % base_list.size()])
	var a := randf() * TAU
	for k in list.size():
		var at := clear_spot(mars.global_position, a + k * 0.6, mars.radius + 1300.0)
		var s := spawn(Names.fill("{drengin} %s") % list[k], 2, list[k], "Varr Home Guard %d" % (k + 1), at, mars.velocity, 6)
		s.get_node("AI").patrol_point = mars.global_position
		enemies.append(s)


func _reinforce() -> void:
	var venus := planet("Venus")
	var dir := (venus.global_position - mars.global_position).angle() if venus != null else 0.0
	for k in scaled(REINFORCE.size()):
		var design: String = REINFORCE[k % REINFORCE.size()]
		var at := clear_spot(mars.global_position, dir + k * 0.3, mars.radius + 3500.0)
		var s := spawn(Names.fill("{drengin} %s") % design, 2, design, "Claw of Varr %d" % (k + 1), at, mars.velocity, 7)
		s.get_node("AI").patrol_point = mars.global_position
		enemies.append(s)
	say("sm_reinforce")


func cleanup() -> void:
	if base != null and is_instance_valid(base):
		_base_ai(true)
		if base.died.is_connected(_on_base_died):
			base.died.disconnect(_on_base_died)
	super.cleanup()


func _on_base_died(_s: Ship, _k: Node) -> void:
	complete("base")
	say("sm_base")


func _on_dauntless_died(_s: Ship, _k: Node) -> void:
	pinned = false
	_base_ai(true)
	fail_objective("dauntless")
	if stage != "flee":
		say("sm_dauntless")


func _on_dropper_died(_s: Ship, _k: Node) -> void:
	var flying := 0
	for d in droppers:
		if alive(d) and not landed.has(d):
			flying += 1
	if landed.size() + flying < 2:
		fail_objective("droppers")
	if landed.is_empty() and flying == 0:
		fail("Every drop ship was lost.", "sm_fail_droppers")
	elif stage != "flee":
		say("sm_lost_drop")
