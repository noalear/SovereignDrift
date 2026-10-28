## Operation 2, Dark Convoy: find the lost fuel convoy near Ceres, scan the
## two wrecks (Drengin disruptor burns), then fight off the "pirates" who
## come out of the rocks and board one of them for its log.
extends StoryOp

const SITE_OFF := 900.0           # the wrecks drift this far off Ceres
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
var brandt: Ship
var captured := false


func setup() -> void:
	var ceres := planet("Ceres")
	var sun: Planet = world.sun
	# the wrecks lie on Ceres's sunward side, clear of the rock itself
	var dir := (sun.global_position - ceres.global_position).angle() if sun != null else 0.0
	site = clear_spot(ceres.global_position, dir, ceres.radius + SITE_OFF) - ceres.global_position
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
	objective("site", "Fly to the Tern convoy's last position near Ceres", _site, "Last known position")
	go("travel")


func _site() -> Vector2:
	var ceres := planet("Ceres")
	return ceres.global_position + site


func step(delta: float) -> void:
	var p := player()
	match stage:
		"travel":
			if not p.dead and p.global_position.distance_to(_site()) < ARRIVE_REACH:
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
				if not alive(ship) or p.dead:
					continue
				if p.global_position.distance_to(ship.global_position) - ship.size < SCAN_REACH:
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
			if base != null and not p.dead and p.global_position.distance_to(base.global_position) < HOME_REACH:
				complete("home")
				complete("wing")
				say("dc_done")
				say("dc_done2")
				p.earn(500)
				world.notify(p, "Operation pay: 500 credits.")
				story.flags["drengin_log"] = true
				win()


## Two raiders with pirate transponders out of the rocks.
func _ambush() -> void:
	var p := player()
	var from := p.global_position if not p.dead else _site()
	var a := randf() * TAU
	lead = spawn("Raider Skeg", 3, "Scout", "Rust Wake", clear_spot(from, a, AMBUSH_FROM), planet("Ceres").velocity, 4)
	lead.hull_max *= LEAD_HULL
	lead.hull = lead.hull_max
	lead.crew = 3.0                    # a small crew: a boarding party wins
	lead.died.connect(_on_lead_died)
	wing = spawn("Raider Holt", 3, "Scout", "Rust Fang", clear_spot(from, a + 0.5, AMBUSH_FROM + 200.0), planet("Ceres").velocity, 3)
	for r in [lead, wing]:
		r.rotation = (from - r.global_position).angle()
		r.reset_physics_interpolation()
		r.target = p
		var ai: AIPilot = r.get_node("AI")
		ai.duel = true                  # it came for you, not for the supply ships
	say("dc_ambush")
	say("dc_raider")
	say("dc_weapons")
	objective("disable", "Knock out the lead raider's engines (shoot its stern)", lead, "Lead raider")
	objective("wing", "Drive off or destroy the second raider", wing, "Raider", true)
	wing.died.connect(func(_s, _k): complete("wing"))


func _on_lead_died(s: Ship, killer: Node) -> void:
	if s.captured and killer is Ship and killer.race_id == 1:
		captured = true
	elif not captured:
		fail("The lead raider was destroyed before anyone could board it.", "dc_fail_lost")


func _on_brandt_died(s: Ship, _killer: Node) -> void:
	if not s.captured:
		say("brandt_down")
