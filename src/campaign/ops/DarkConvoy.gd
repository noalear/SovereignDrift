## Operation 2, Dark Convoy: find the lost fuel convoy near Ceres, scan the
## two wrecks (Drengin disruptor burns), then fight off the "pirates" who
## come out of the rocks and board one of them for its log.
extends StoryOp

const SITE_OFF := 900.0           # the wrecks drift this far off Ceres
const QUIET := 4500.0             # and at least this far from any colony's guns or armed station
const BELT_STEP := 0.05           # rad along Ceres's orbit per try when Mars is too near
const ARRIVE_REACH := 1600.0      # from the site
const SCAN_REACH := 320.0         # from a wreck's hull
const SCAN_TIME := 2.0            # seconds alongside to scan it
const AMBUSH_FROM := 2300.0
const LEAD_HULL := 3.0            # the lead raider's hull, times a Scout's: its engines go before it does
const HOME_REACH := 900.0

var site := Vector2.ZERO          # from Ceres's centre, so it moves with it
var wrecks: Array = []            # [ship, objective id, seconds scanned]
var lead: Ship                    # the raider to take alive
var wing: Ship
var wings: Array = []             # the lead's wingmen: one, and another for each two friends along
var brandt: Ship
var captured := false


func setup() -> void:
	var ceres := planet("Ceres")
	site = _quiet_site(ceres) - ceres.global_position
	var at := _site()
	for row in [["Tern", Vector2(-180, -60)], ["Petrel", Vector2(200, 120)]]:
		var w := spawn("Convoy %s" % row[0], 1, "Freighter", "TSS %s" % row[0], at + row[1], ceres.velocity)
		w.crew = 0.0                         # nobody aboard: dead in space
		w.hull = w.hull_max * 0.3
		w.rotation = randf() * TAU
		w.reset_physics_interpolation()
		var id: String = "scan_" + str(row[0]).to_lower()
		wrecks.append([w, id, 0.0])
	var p := player()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = p
	ai.state = AIPilot.State.ESCORT
	brandt.died.connect(_on_brandt_died)
	say("dc_start")
	objective("site", "Fly to the Tern convoy's last position in the belt", _site, "Last known position")
	go("travel")


## Where the wrecks lie: off Ceres, or further along the belt when Mars
## (its guns, the Drengin starbase) or any other armed place is near, so
## the raiders' ambush plays out between them and the pilots alone.
func _quiet_site(ceres: Planet) -> Vector2:
	var sun: Planet = world.sun
	var c := sun.global_position if sun != null else Vector2.ZERO
	var best := clear_spot(ceres.global_position, (c - ceres.global_position).angle(), ceres.radius + SITE_OFF)
	var best_room := guns_near(best)
	var k := 0
	while best_room < QUIET and k < 48:
		# step along Ceres's orbit either way, a little in or out of it
		k += 1
		var turn := (k / 2 + 1) / 2 * BELT_STEP * (1.0 if k % 2 == 0 else -1.0)
		var out := 1.0 + (0.06 if k % 4 < 2 else -0.06)
		var at := c + (ceres.global_position - c).rotated(turn) * out
		at = clear_spot(at, randf() * TAU, 1.0)
		var room := guns_near(at)
		if room > best_room:
			best = at
			best_room = room
	return best


func _site() -> Vector2:
	var ceres := planet("Ceres")
	return ceres.global_position + site


func step(delta: float) -> void:
	match stage:
		"travel":
			if pilot_near(_site(), ARRIVE_REACH) != null:
				complete("site")
				say("dc_arrive")
				say("dc_scan")
				for w in wrecks:
					objective(w[1], "Scan the %s: fly alongside" % w[0].ship_name, w[0], w[0].ship_name)
				go("scan")
		"scan":
			var left := 0
			for w in wrecks:
				if done(w[1]):
					continue
				left += 1
				var ship: Ship = w[0]
				if not alive(ship):
					continue
				if pilot_near(ship.global_position, SCAN_REACH + ship.size) != null:
					w[2] += delta
					if w[2] >= SCAN_TIME:
						complete(w[1])
						left -= 1
						say("dc_scan_1" if left == 1 else "dc_scan_2")
			if left == 0:
				go("calm")
		"calm":
			if stage_time > 3.0 and not story.speaking():
				_ambush()
				go("fight")
		"fight":
			if alive(lead) and not lead.is_prize:
				if lead.is_disabled():
					keep_down(lead)
					if not done("disable"):
						complete("disable")
						say("dc_disabled")
						objective("board", "Board the disabled raider (%s, close and slow)" % Keybinds.short_text("board"), lead, "Disabled raider")
				elif lead.hull < lead.hull_max * 0.35:
					keep_down(lead)              # its engines give out before its hull does
				if lead.hull < lead.hull_max * 0.2:
					lead.invulnerable = 0.5      # and what is left holds together for the boarders
			if captured:
				complete("board")
				say("dc_captured")
				say("dc_captured2")
				objective("home", "Bring the raider's log home to the Terran starbase", world.starbase_of(1), "Terran starbase")
				go("home")
		"home":
			var base: Ship = world.starbase_of(1)
			if base != null and pilot_near(base.global_position, HOME_REACH) != null:
				complete("home")
				complete("wing")
				say("dc_done")
				say("dc_done2")
				pay(500)
				story.flags["drengin_log"] = true
				win()


## Two raiders with pirate transponders out of the rocks.
func _ambush() -> void:
	var p := lead_pilot()
	var from := p.global_position if alive(p) else _site()
	var a := randf() * TAU
	lead = spawn("Raider Skeg", 3, "Scout", "Rust Wake", clear_spot(from, a, AMBUSH_FROM), planet("Ceres").velocity, 4)
	lead.hull_max *= LEAD_HULL
	lead.hull = lead.hull_max
	lead.crew = 3.0                    # a small crew: a boarding party wins
	lead.died.connect(_on_lead_died)
	var squad := pilots()
	for i in scaled(1):
		wings.append(spawn("Raider %s" % ["Holt", "Dace", "Wrenn", "Corve"][i % 4], 3, "Scout", "Rust %s" % ["Fang", "Claw", "Tooth", "Spur"][i % 4],
			clear_spot(from, a + 0.5 + i * 0.4, AMBUSH_FROM + 200.0), planet("Ceres").velocity, 3))
	wing = wings[0]
	for r in [lead] + wings:
		r.rotation = (from - r.global_position).angle()
		r.reset_physics_interpolation()
		var mark: Ship = squad[wings.find(r) % squad.size()] if r != lead and not squad.is_empty() else p
		r.target = mark if alive(mark) else p
		var ai: AIPilot = r.get_node("AI")
		ai.duel = true                  # it came for you, not for the supply ships
	say("dc_ambush")
	say("dc_raider")
	say("dc_weapons")
	objective("disable", "Knock out the lead raider's engines (shoot its stern)", lead, "Lead raider")
	objective("wing", "Drive off or destroy the second raider" if wings.size() == 1 else "Drive off or destroy the other raiders", wing, "Raider", true)
	for r in wings:
		r.died.connect(func(_s, _k):
			for o in wings:
				if alive(o):
					return
			complete("wing"))


func _on_lead_died(s: Ship, killer: Node) -> void:
	if s.captured and killer is Ship and killer.race_id == 1:
		captured = true
	elif not captured:
		fail("The lead raider was destroyed before anyone could board it.", "dc_fail_lost")


func _on_brandt_died(s: Ship, _killer: Node) -> void:
	if not s.captured:
		say("brandt_down")

