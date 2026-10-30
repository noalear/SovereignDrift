## Operation 7, Wolf Pack: the player leads a wing of their own (Lantern
## Three and Four, ordered like any wing) against a Drengin supply convoy
## on the Venus to Mars lane.  Three freighters and their escort burn for
## Mars; two of them getting there ends it.
extends StoryOp

const FREIGHTERS := ["Varr Provider", "Varr Bounty", "Varr Tithe"]
const ESCORTS := ["Destroyer", "Scout"]
const START_OFF := 2600.0         # the convoy sets out this far from Venus's surface
const SIGHT := 4500.0             # a pilot this close to a freighter: contact
const ARRIVE := 1800.0            # from Mars's surface: a freighter got through
const LOST_AT := 2                # this many through and it is over
const WING_HULL := 1.3            # Lantern Three and Four, a little sturdier than a stock Scout

var venus: Planet
var mars: Planet
var dest: Waypoint                # the convoy's goal, riding with Mars
var freighters: Array = []
var escorts: Array = []
var wingmen: Array = []
var killed := 0
var through := 0


func setup() -> void:
	venus = planet("Venus")
	mars = planet("Mars")
	dest = waypoint("Mars", mars, 120.0, (venus.global_position - mars.global_position).normalized() * (mars.radius + 900.0))
	var dir := (mars.global_position - venus.global_position).angle()
	var at := clear_spot(venus.global_position, dir, venus.radius + START_OFF)
	for i in FREIGHTERS.size():
		var f := spawn(Names.fill("{drengin} hauler"), 2, "Freighter", FREIGHTERS[i], at + Vector2(-260.0 * i, (i - 1) * 220.0).rotated(dir), venus.velocity)
		f.rotation = dir
		f.reset_physics_interpolation()
		f.support = true
		f.cargo = {"fuel": 150.0}
		f._trim_cargo()
		f.nav_target = dest
		f.autopilot = true
		f.died.connect(_on_freighter_died)
		freighters.append(f)
	var list: Array = ESCORTS.duplicate()
	for k in range(list.size(), scaled(list.size())):
		list.append(ESCORTS[k % ESCORTS.size()])
	for k in list.size():
		var e := spawn(Names.fill("{drengin} escort"), 2, list[k], "Varr Guard %d" % (k + 1), at + Vector2(200, (k - 1) * 300.0).rotated(dir), venus.velocity, 5)
		var ai: AIPilot = e.get_node("AI")
		ai.guard(freighters[k % freighters.size()])
		escorts.append(e)
	# the wing: two for the flight lead, one more for each friend flying along
	var squad := pilots()
	var names := ["Lantern Three", "Lantern Four", "Lantern Five", "Lantern Six", "Lantern Seven"]
	for k in squad.size() + 1:
		var lead: Ship = squad[0] if k < 2 else squad[k - 1]
		var w := spawn(["Lt. Vance", "Lt. Osei", "Lt. Marsh", "Lt. Ibarra", "Lt. Kade"][k % 5], 1, "Scout", names[k % names.size()],
			lead.global_position + Vector2(-160, 140 * (1 if k % 2 == 0 else -1)).rotated(lead.rotation), lead.velocity, 5)
		w.hull_max *= WING_HULL
		w.hull = w.hull_max
		var ai: AIPilot = w.get_node("AI")
		ai.join(lead, false)
		w.died.connect(func(s, _k):
			if not s.captured:
				fail_objective("wing")
				say("wp_wing_lost"))
		wingmen.append(w)
	world.wing.dirty = true
	say("wp_start")
	objective("intercept", Names.fill("Intercept the {drengin} supply convoy on the Venus to Mars lane"), freighters[0], Names.fill("{drengin} convoy"))
	objective("wing", "Bring your wing home: %s sends them at your target, %s for every order" % [Keybinds.short_text("wing_attack"), Keybinds.short_text("wing_menu")], null, "", true)
	go("intercept")


func step(_delta: float) -> void:
	_hold_slow()
	_arrivals()
	if state != "running":
		return
	match stage:
		"intercept":
			for f in freighters:
				if alive(f) and pilot_near(f.global_position, SIGHT) != null:
					complete("intercept")
					say("wp_sighted")
					say("wp_orders")
					objective("convoy", _convoy_text(), _first_freighter, "Supply ship")
					go("attack")
					break
		"attack":
			set_text("convoy", _convoy_text())
			if _left() == 0:
				complete("convoy")
				if not _wing_lost():
					complete("wing")
				say("wp_kesh")
				say("wp_done")
				say("wp_done2")
				flag("convoy_through", through)
				pay(600)
				win()


func _convoy_text() -> String:
	return "Destroy the supply ships before they reach Mars (%d of %d down%s)" % [killed, FREIGHTERS.size(), ", %d through" % through if through > 0 else ""]


## The freighters burn at a freighter's pace: no warping past the wolves.
func _hold_slow() -> void:
	for f in freighters:
		if alive(f):
			f.warp_block = 999.0
			if f.warping:
				f.end_warp()
			if not f.autopilot and f.global_position.distance_to(mars.global_position) - mars.radius > ARRIVE:
				f.autopilot = true


func _arrivals() -> void:
	for f in freighters.duplicate():
		if alive(f) and f.global_position.distance_to(mars.global_position) - mars.radius < ARRIVE:
			through += 1
			freighters.erase(f)
			world.remove_ship(f)
			if through >= LOST_AT:
				fail("The convoy got through to Mars.", "wp_fail")
				return
			say("wp_escaped")
			story.changed.emit()


func _left() -> int:
	var n := 0
	for f in freighters:
		if alive(f):
			n += 1
	return n


func _wing_lost() -> bool:
	for w in wingmen:
		if not alive(w):
			return true
	return false


func _first_freighter() -> Variant:
	for f in freighters:
		if alive(f):
			return f
	return null


func _on_freighter_died(s: Ship, _k: Node) -> void:
	if not freighters.has(s):
		return
	killed += 1
	if _left() == 1:
		say("wp_two" if killed >= 2 else "wp_one")
	elif _left() > 1:
		say("wp_one")
	story.changed.emit()
