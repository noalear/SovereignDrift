## Operation 4, Picket Line: Luna gets a defense station.  Engineers have
## the frame up in Luna orbit with its Metals; the player lands on Earth
## for the Alloys, delivers them with Lt. Brandt along, and guards the
## frame while it is built.  The "pirates" who come for it fly like
## Drengin.
extends StoryOp

const ALLOYS := 8.0
const FRAME_OFF := 520.0          # from Luna's surface
const RAID_AFTER := 20.0          # seconds into the build
const RAID_FROM := 2600.0         # from the frame
const RAID_HULL := 1.6            # the raiders' hulls, times their design's
const HOME_REACH := 900.0

var luna: Planet
var earth: Planet
var frame: Ship
var brandt: Ship
var raiders: Array = []


func setup() -> void:
	luna = planet("Luna")
	earth = planet("Earth")
	# the Alloys are waiting on Earth
	earth.stock["alloys"] = maxf(earth.stock.get("alloys", 0.0), ALLOYS * 3.0)
	var sun: Node2D = world.sun
	var out := (luna.global_position - earth.global_position).normalized()
	if sun != null:
		out = (out + (sun.global_position - luna.global_position).normalized() * 0.5).normalized()
	frame = world.build_station(1, "DefenseStation", luna, luna.global_position + out * (luna.radius + FRAME_OFF), true)
	frame.set_meta("story", true)
	spawned.append(frame)
	frame.cargo["metals"] = float(GameData.STATIONS["DefenseStation"]["cost"]["metals"])
	frame.died.connect(_on_frame_died)
	var p := player()
	brandt = spawn("Lt. Brandt", 1, "Scout", "Lantern Two", p.global_position + Vector2(-140, 120).rotated(p.rotation), p.velocity, 6)
	var ai: AIPilot = brandt.get_node("AI")
	ai.escort = p
	ai.state = AIPilot.State.ESCORT
	say("pk_start")
	objective("load", "Land on Earth (%s) and load %d Alloys (%s)" % [Keybinds.short_text("land"), int(ALLOYS), Keybinds.short_text("cargo")], earth, "Earth")
	go("load")


func step(_delta: float) -> void:
	match stage:
		"load":
			var loaded: bool = frame.cargo.get("alloys", 0.0) >= ALLOYS - 0.01
			for s in pilots():
				loaded = loaded or s.cargo.get("alloys", 0.0) >= ALLOYS - 0.01
			if loaded:
				complete("load")
				say("pk_loaded")
				objective("deliver", "Deliver the Alloys to the station frame over Luna: come alongside and unload (%s)" % Keybinds.short_text("cargo"), frame, "Station frame")
				go("deliver")
		"deliver":
			if frame.cargo.get("alloys", 0.0) >= ALLOYS - 0.01:
				complete("deliver")
				say("pk_delivered")
				objective("guard", "Guard the frame while it is built", frame, "Luna defense station")
				go("build")
		"build":
			var total := float(GameData.STATIONS["DefenseStation"]["build"])
			if frame.building:
				set_text("guard", "Guard the frame while it is built (%d%%)" % int(frame.build_t / total * 100.0))
			if raiders.is_empty() and stage_time > RAID_AFTER:
				_raid()
			var left := _raiders_left()
			if not raiders.is_empty() and left == 0 and frame.building:
				# with the raid beaten the crews hurry the last of it
				frame.build_t = maxf(frame.build_t, total - 8.0)
			if not raiders.is_empty() and stage_time > RAID_AFTER + 25.0:
				say_once("pk_formation")
			if not frame.building and not frame.dead and left == 0 and not raiders.is_empty():
				complete("guard")
				complete("raiders")
				say("pk_built")
				objective("home", Names.fill("Return to the {terran} starbase"), home_base, Names.fill("{terran} starbase"))
				go("home")
		"home":
			if pilot_near(home_base().global_position, HOME_REACH) != null:
				complete("home")
				say("pk_done")
				say("pk_done2")
				pay(400)
				flag("luna_station", true)
				# the station stays: it is Luna's now
				spawned.erase(frame)
				frame.remove_meta("story")
				win()


## Three "pirates" come round Luna for the frame (more with friends along).
func _raid() -> void:
	var dir := (frame.global_position - luna.global_position).angle() + 2.2
	var from := frame.global_position + Vector2(RAID_FROM, 0).rotated(dir)
	var names := ["Rust Hound", "Rust Kite", "Rust Moth", "Rust Gnat", "Rust Shrike", "Rust Wasp"]
	for i in scaled(3):
		var at := from + Vector2(0, (i - 1) * 140.0).rotated(dir) - Vector2(i % 2 * 120.0, 0).rotated(dir)
		var r := spawn("Raider %d" % (i + 1), 3, "Raider" if i == 0 else "Scout", names[i % names.size()], at, luna.velocity, 5)
		r.hull_max *= RAID_HULL
		r.hull = r.hull_max
		r.rotation = (frame.global_position - at).angle()
		r.reset_physics_interpolation()
		var ai: AIPilot = r.get_node("AI")
		ai.patrol_point = frame.global_position
		r.target = frame
		raiders.append(r)
	say("pk_raid")
	say("pk_raid2")
	objective("raiders", "Drive off the raiders", null, "", false)


func _raiders_left() -> int:
	var n := 0
	for r in raiders:
		if alive(r):
			n += 1
	return n


func _on_frame_died(_s: Ship, _killer: Node) -> void:
	fail("The station frame was destroyed.", "pk_fail_frame")


func on_breach(attacker: Node, victim: Node) -> void:
	if is_pilot(attacker) and (victim is Ship and victim.race_id == 2 or victim is Planet and victim.owner_race != 1):
		fail("You fired first on an Accord ship.", "fw_fail_breach")
