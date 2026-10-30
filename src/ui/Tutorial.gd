## Tutorial -- the voiced Terran tutorial.  Commander Hale talks a cadet
## through every part of the game, one lesson at a time, in a calm Sol: no
## enemy fleets, no round clock, and colonies that hold their fire.
##
## Main makes it for a World started with tutorial = true.  It plays the
## voice (TutorialLines, rendered by tools/voice/generate.py), shows the
## same words as subtitles with the objective and its live keys under them,
## marks what a lesson is about on the screen, and sets the lessons up: a
## nav beacon, a target drone to disable and board (it then flies on the
## cadet's wing and takes orders), a rookie pirate to fight.
##
## A lesson with a goal waits for the cadet to do it; one without moves on
## when the line is spoken.  Enter skips a lesson, Shift+Enter a chapter,
## Backspace says the line again.
class_name Tutorial
extends Control

## id (also the voice line), chapter, goal (keys as {action}; "" = listen),
## mark (what to point at), focus (a HUD panel to outline).
const STEPS := [
	{"id": "intro", "chapter": 0},
	{"id": "intro2", "chapter": 0},
	{"id": "hud_status", "chapter": 0, "focus": "status"},
	{"id": "hud_weapons", "chapter": 0, "focus": "weapons_radar"},
	{"id": "hud_top", "chapter": 0, "focus": "top"},
	{"id": "thrust", "chapter": 1, "goal": "Hold {thrust} to thrust, {turn_left} / {turn_right} to turn. Fly out from the starbase"},
	{"id": "strafe", "chapter": 1, "goal": "Strafe with {strafe_left} / {strafe_right}, and reverse with {reverse}"},
	{"id": "brake", "chapter": 1, "goal": "Hold {brake} until you stop"},
	{"id": "boost", "chapter": 1, "goal": "Hold {boost} while you thrust ({thrust})"},
	{"id": "assist", "chapter": 1, "goal": "Press {flight_assist} to turn flight assist off, then on again"},
	{"id": "beacon", "chapter": 1, "goal": "Fly to the nav beacon", "mark": "beacon"},
	{"id": "gravity", "chapter": 1},
	{"id": "nav", "chapter": 2, "goal": "Press {nav_next} until the nav target is Luna", "mark": "Luna"},
	{"id": "autopilot", "chapter": 2, "goal": "Press {autopilot} for the autopilot, and arrive at Luna", "mark": "Luna"},
	{"id": "warp", "chapter": 2, "goal": "Point at open space, press {warp} to warp, then {warp} again to drop out"},
	{"id": "map", "chapter": 2, "goal": "Tap {tactical_map} to open the system map, then close it"},
	{"id": "info", "chapter": 2, "goal": "Select a planet or moon: {select_target}", "mark": "Luna"},
	{"id": "zoom", "chapter": 2, "goal": "Zoom with the mouse wheel, or the radar with {radar_zoom_in} / {radar_zoom_out}"},
	{"id": "target", "chapter": 3, "goal": "Target the drone: {target_next}, {target_nearest}, or {select_target} on it", "mark": "drone"},
	{"id": "fire", "chapter": 3, "goal": "Aim with the mouse and fire with {fire_primary}. Take its shields down", "mark": "drone"},
	{"id": "groups", "chapter": 3, "goal": "Press {weapon_group_4} for the mine layer, then {weapon_group_1} for the guns again", "mark": "drone"},
	{"id": "seeker", "chapter": 3, "goal": "Press {weapon_group_3} for the seeker rack, put a seeker into the drone ({fire_primary}), then {weapon_group_1} for the guns", "mark": "drone"},
	{"id": "disable", "chapter": 3, "goal": "Shoot the drone's stern until its engines go offline", "mark": "drone"},
	{"id": "board", "chapter": 3, "goal": "Close in, slow down and press {board} to board the drone", "mark": "drone"},
	{"id": "wing", "chapter": 3, "goal": "Hold {wing_menu} for the orders wheel, point at Form up and let go", "mark": "wingman", "focus": "wing"},
	{"id": "wing_call", "chapter": 3},
	{"id": "shield_focus", "chapter": 3, "goal": "Press {shield_focus} to swing your shield forward, then press it until it's Balanced again", "focus": "status"},
	{"id": "shields_down", "chapter": 3, "goal": "Press {shields_toggle} to lower your shields, then {shields_toggle} to raise them", "focus": "status"},
	{"id": "stealth", "chapter": 3, "goal": "Press {silent_running} for silent running, then {silent_running} again", "focus": "signature"},
	{"id": "pdc", "chapter": 3, "goal": "Let your PDC shoot down %d missiles, then press {pdc_toggle} to turn it off and {pdc_toggle} again" % PDC_KILLS, "focus": "weapons_radar"},
	{"id": "jammer", "chapter": 3, "goal": "Target the pirate ({target_nearest}) and press {cloak} to jam it", "mark": "pirate"},
	{"id": "wing_attack", "chapter": 3, "goal": "With the pirate as your target ({target_nearest}), press {wing_attack} to send your wing at it", "mark": "pirate"},
	{"id": "spar", "chapter": 3, "goal": "Destroy or capture the pirate", "mark": "pirate"},
	{"id": "loot", "chapter": 3, "goal": "Fly over a pickup to collect it", "mark": "loot"},
	{"id": "dock", "chapter": 4, "goal": "Fly back to the " + Names.TERRAN + " starbase at Earth", "mark": "starbase"},
	{"id": "shipyard", "chapter": 4, "goal": "Brake ({brake}), then press {refit} for the shipyard", "mark": "starbase"},
	{"id": "armor", "chapter": 4},
	{"id": "shield_types", "chapter": 4},
	{"id": "shipyard_tour", "chapter": 4, "goal": "Look around (wingmen near the yard get tabs too), then launch or close the shipyard"},
	{"id": "base_cargo", "chapter": 4, "goal": "Press {cargo} at the starbase for station kits and ship orders", "mark": "starbase"},
	{"id": "land", "chapter": 5, "goal": "Get low over Earth, slow down and press {land}", "mark": "Earth"},
	{"id": "colonists", "chapter": 5, "goal": "Landed: press {colonists_board} to take colonists on"},
	{"id": "cargo", "chapter": 5, "goal": "Press {cargo} and load {res}"},
	{"id": "ground_tabs", "chapter": 5, "goal": "Open the Troops or Buildings tab in the cargo panel ({cargo})"},
	{"id": "to_luna", "chapter": 5, "goal": "Lift off ({land}) and land on Luna", "mark": "Luna"},
	{"id": "unload", "chapter": 5, "goal": "Press {colonists_unload} to put the colonists down, and unload your cargo ({cargo})"},
	{"id": "ledger", "chapter": 5, "goal": "Press {ledger} for the empire ledger"},
	{"id": "haulers", "chapter": 6, "mark": "hauler"},
	{"id": "stations", "chapter": 6},
	{"id": "station_capture", "chapter": 6},
	{"id": "freeport", "chapter": 6, "mark": "freeport"},
	{"id": "invasion", "chapter": 6, "mark": "Mars"},
	{"id": "invasion2", "chapter": 6, "mark": "Mars"},
	{"id": "strategy", "chapter": 6},
	{"id": "strategy2", "chapter": 6},
	{"id": "modes", "chapter": 6},
	{"id": "outro", "chapter": 6},
]
const CHAPTERS := ["Welcome aboard", "Flight school", "Navigation", "Combat", "The starbase", "Worlds, colonists and cargo", "Empire and strategy"]
const PRAISE := ["good_1", "good_2", "good_3", "good_4", "good_5", "good_6"]

const PANEL_W := 740.0
const TEXT_SIZE := 19
const LINE_H := 25.0
const BEACON_REACH := 260.0
const THRUST_OUT := 1300.0        # how far from the starbase the first flight goes
const PDC_KILLS := 3              # missiles the PDC lesson's launcher sends in
const PDC_SALVO_GAP := 1.6        # seconds between them

var world: World
var main: Node
var font: Font
var voice: AudioStreamPlayer
var index := -1
var step: Dictionary = {}
var step_time := 0.0
var line_id := ""                 # the line playing or last played
var line_time := 0.0              # seconds into it (for the subtitles without audio)
var line_len := 0.0
var line_clock := 0.0             # wall time into it: a voice that stalls (no sound device) still ends
var queue: Array = []             # lines to say after this one
var goal_met := false
var praised := false
var advance_at := -1.0
var finished := false
var finished_time := 0.0
var time := 0.0

# what a lesson tracks
var beacon_offset := Vector2.ZERO  # from Earth's centre
var drone: Ship = null
var pirate: Ship = null
var launcher: Ship = null          # the PDC lesson's missile launcher
var pdc_fired := 0
var pdc_next := 0.0
var pdc_kills_at := 0
var seeker_hits := 0              # seekers that had hit when the seeker lesson began
var drone_taken := false
var pirate_down := false
var seen := {}                    # flags seen during the lesson
var held := {}                    # action -> seconds held during the lesson
var turned := 0.0
var last_rot := 0.0
var assist_flips := 0
var last_assist := true
var credits_at := 0
var hold_at := 0
var zoom_at := 1.0
var radar_at := 0.0
var cargo_res := "food"


func setup(p_world: World, p_main: Node) -> void:
	world = p_world
	main = p_main


func _ready() -> void:
	font = ThemeDB.fallback_font
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # never in the way of the guns or the panels
	process_mode = Node.PROCESS_MODE_ALWAYS      # the shipyard pauses the game; Hale keeps talking
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	voice = AudioStreamPlayer.new()
	voice.bus = "Voice" if AudioServer.get_bus_index("Voice") >= 0 else "Master"
	voice.process_mode = Node.PROCESS_MODE_ALWAYS
	add_child(voice)
	if world != null:
		world.player_died.connect(_on_player_died)
	_go(0)


# ------------------------------------------------------------ the voice

func _say(id: String) -> void:
	line_id = id
	line_time = 0.0
	line_clock = 0.0
	var path := TutorialLines.audio_path(id)
	var stream: AudioStream = load(path) if ResourceLoader.exists(path) else null
	voice.stop()
	if stream != null:
		voice.stream = stream
		line_len = stream.get_length()
		voice.play()
	else:
		# no recording: the subtitles carry it, at a comfortable reading pace
		line_len = 1.5 + TutorialLines.text(id).length() / 16.0


func speaking() -> bool:
	return line_id != "" and line_time < line_len


## A line between lessons (the drone was lost, the cadet's ship too): it
## waits for the one being spoken.
func _interject(id: String) -> void:
	if speaking():
		if not queue.has(id):
			queue.append(id)
	else:
		_say(id)


# ------------------------------------------------------------ lessons

func _go(i: int) -> void:
	if i >= STEPS.size():
		_finish()
		return
	index = i
	step = STEPS[i]
	step_time = 0.0
	goal_met = false
	praised = false
	advance_at = -1.0
	seen = {}
	held = {}
	turned = 0.0
	queue.clear()
	var p := _player()
	if p != null:
		last_rot = p.rotation
		last_assist = p.flight_assist
		credits_at = p.credits
		hold_at = p.hold.size()
	assist_flips = 0
	if world != null:
		zoom_at = world.target_zoom
	radar_at = main.hud.radar_range if main != null and main.get("hud") != null else 0.0
	if has_method("_enter_" + step["id"]):
		call("_enter_" + step["id"])
	_say(step["id"])


func _finish() -> void:
	finished = true
	finished_time = time
	step = {}
	_clear_marks()


func next_step() -> void:
	_go(index + 1)


func next_chapter() -> void:
	var ch: int = step.get("chapter", -1)
	var i := index + 1
	while i < STEPS.size() and STEPS[i]["chapter"] == ch:
		i += 1
	_go(i)


func has_goal() -> bool:
	return step.get("goal", "") != ""


func _process(delta: float) -> void:
	time += delta
	queue_redraw()
	if world == null or finished and not speaking():
		return
	# the pause menu (and its Settings page) holds everything, the voice too
	var menu_up: bool = main != null and main.get("pause_panel") != null
	voice.stream_paused = menu_up
	if menu_up:
		return
	if speaking():
		line_clock += delta
		line_time = voice.get_playback_position() if voice.playing else line_time + delta
		if not voice.playing and voice.stream != null and line_clock > 0.2:
			line_time = line_len         # the player ran to the end
		elif line_clock > line_len + 1.0:
			voice.stop()                 # the sound never got going: go by the clock
			line_time = line_len
		Music.duck_for(0.45)
	elif not queue.is_empty():
		_say(queue.pop_front())
		return
	if finished:
		return
	step_time += delta
	var p := _player()
	if p != null and not p.dead:
		_track(p, delta)
	if has_goal() and not goal_met and p != null:
		goal_met = _check()
	if advance_at < 0.0 and not speaking() and queue.is_empty():
		if not has_goal():
			advance_at = step_time + 0.9
		elif goal_met and not praised:
			praised = true
			_say(PRAISE[randi() % PRAISE.size()])
			advance_at = step_time + line_len + 0.6
	if advance_at >= 0.0 and step_time >= advance_at:
		next_step()


func _check() -> bool:
	var f: String = "_done_" + step["id"]
	return call(f) if has_method(f) else false


## What a lesson watches between frames: keys held, turning, toggles.
func _track(p: Ship, delta: float) -> void:
	for a in ["strafe_left", "strafe_right", "reverse", "brake", "boost", "thrust"]:
		if InputMap.has_action(a) and Input.is_action_pressed(a):
			held[a] = held.get(a, 0.0) + delta
	if Input.is_action_pressed("boost") and Input.is_action_pressed("thrust"):
		held["boost_thrust"] = held.get("boost_thrust", 0.0) + delta
	turned += absf(wrapf(p.rotation - last_rot, -PI, PI))
	last_rot = p.rotation
	if p.flight_assist != last_assist:
		assist_flips += 1
		last_assist = p.flight_assist
	for flag in ["warping", "autopilot"]:
		if p.get(flag):
			seen[flag] = true
	if p.sensors.emcon:
		seen["silent"] = true
	if p.sensors.jam_active():
		seen["jamming"] = true
	if p.pdc.fitted() and not p.pdc.enabled:
		seen["pdc_off"] = true
	if p.shield_focus == 1:
		seen["focus_forward"] = true
	if not p.shields_raised:
		seen["shields_down"] = true
	if main != null and main.get("tactical") != null:
		seen["map"] = true


func _unhandled_input(event: InputEvent) -> void:
	if finished or world == null or not (event is InputEventKey) or not event.pressed or event.echo:
		return
	if main != null and (main.get("pause_panel") != null or main.get("builder") != null):
		return                  # the shipyard and the pause menu use Enter themselves
	if event.keycode in [KEY_ENTER, KEY_KP_ENTER] and not event.alt_pressed:
		voice.stop()
		line_time = line_len
		queue.clear()
		if event.shift_pressed:
			next_chapter()
		else:
			next_step()
		get_viewport().set_input_as_handled()
	elif event.keycode == KEY_BACKSPACE:
		_say(step.get("id", "outro"))
		get_viewport().set_input_as_handled()


func _player() -> Ship:
	return world.player if world != null else null


func _planet(n: String) -> Planet:
	return world.planet_by_name.get(n, null) if world != null else null


# ------------------------------------------------------------ setups and goals

func _done_thrust() -> bool:
	var p := _player()
	var base := world.starbase_of(p.race_id)
	var out := base == null or p.global_position.distance_to(base.global_position) > THRUST_OUT
	return out and turned > 1.2 and held.get("thrust", 0.0) > 0.5


func _done_strafe() -> bool:
	return held.get("strafe_left", 0.0) + held.get("strafe_right", 0.0) > 0.4 and held.get("reverse", 0.0) > 0.4


func _done_brake() -> bool:
	var p := _player()
	return held.get("brake", 0.0) > 0.3 and p.local_velocity().length() < 30.0


func _done_boost() -> bool:
	return held.get("boost_thrust", 0.0) > 1.0


func _done_assist() -> bool:
	return assist_flips >= 2 and _player().flight_assist


func _enter_beacon() -> void:
	var p := _player()
	var earth := _planet("Earth")
	var at := p.global_position + Vector2.RIGHT.rotated(p.rotation) * 2200.0
	# never inside or skimming a world
	for tries in 8:
		var clear := true
		for pl in world.planets:
			if at.distance_to(pl.global_position) < pl.radius + 450.0:
				clear = false
		if clear:
			break
		at = p.global_position + (at - p.global_position).rotated(PI / 4.0)
	beacon_offset = at - (earth.global_position if earth != null else Vector2.ZERO)


func beacon_position() -> Vector2:
	var earth := _planet("Earth")
	return (earth.global_position if earth != null else Vector2.ZERO) + beacon_offset


func _done_beacon() -> bool:
	return _player().global_position.distance_to(beacon_position()) < BEACON_REACH


func _done_nav() -> bool:
	return _player().nav_target == _planet("Luna")


func _done_autopilot() -> bool:
	var luna := _planet("Luna")
	return luna != null and _player().global_position.distance_to(luna.global_position) < luna.radius + 700.0


func _done_warp() -> bool:
	return seen.has("warping") and not _player().warping


func _done_map() -> bool:
	return seen.has("map") and main.get("tactical") == null


func _done_info() -> bool:
	return world.inspected_planet != null


func _done_zoom() -> bool:
	var radar: float = main.hud.radar_range if main.get("hud") != null else radar_at
	return not is_equal_approx(world.target_zoom, zoom_at) or not is_equal_approx(radar, radar_at)


# --- the target drone

func _enter_target() -> void:
	_ensure_drone()


func _ensure_drone() -> void:
	if drone != null and is_instance_valid(drone) and not drone.dead:
		return
	var p := _player()
	var at := p.global_position + Vector2.RIGHT.rotated(p.rotation) * 650.0
	drone = world.spawn_ship("Training drone", 3, "Scout", "Target drone", at, p.velocity)
	drone.refit(GameData.stock_design("Scout", 3))   # bare hull: no pirate Shroud to hide it
	drone.rotation = p.rotation   # its stern toward you: the engines are the easy shot
	drone.reset_physics_interpolation()
	# tough enough that its systems go before its hull, crewed by a skeleton
	# watch so a boarding party wins
	drone.hull_max *= 3.0
	drone.hull = drone.hull_max
	drone.crew = 2.0
	drone.died.connect(_on_drone_died)
	p.target = drone


func _on_drone_died(s: Ship, _killer: Node) -> void:
	if s.captured:
		drone_taken = true
		return
	if step.get("id", "") in ["target", "fire", "groups", "seeker", "disable", "board"]:
		_interject("drone_lost")
		# a fresh one once the wreck has cleared
		get_tree().create_timer(1.5, true).timeout.connect(func():
			if not finished and step.get("id", "") in ["target", "fire", "groups", "seeker", "disable", "board"]:
				_ensure_drone())


func _done_target() -> bool:
	return drone != null and is_instance_valid(drone) and _player().target == drone


func _enter_fire() -> void:
	_ensure_drone()


func _done_fire() -> bool:
	return drone != null and is_instance_valid(drone) and (drone.shield < drone.shield_max * 0.5 or drone.hull < drone.hull_max)


func _enter_groups() -> void:
	_ensure_drone()


func _done_groups() -> bool:
	var g := _player().weapon_group
	if g == 4:
		seen["group4"] = true
	return seen.has("group4") and g == 1


func _enter_seeker() -> void:
	_ensure_drone()
	seeker_hits = _seeker_hits()


func _seeker_hits() -> int:
	return int(Missile.fates.get("009", {}).get("hit", 0))


func _done_seeker() -> bool:
	if _seeker_hits() > seeker_hits:
		seen["seeker_hit"] = true
	return seen.has("seeker_hit") and _player().weapon_group == 1


func _enter_disable() -> void:
	_ensure_drone()


func _done_disable() -> bool:
	return drone != null and is_instance_valid(drone) and not drone.dead and drone.is_disabled()


func _enter_board() -> void:
	drone_taken = false
	_ensure_drone()
	_keep_drone_down()
	_player().target = drone


## The drone stays dead in space for the boarding lesson: its damage
## control is switched off.
func _keep_drone_down() -> void:
	if drone == null or not is_instance_valid(drone) or drone.dead:
		return
	var e: Dictionary = drone.systems.get_system("engines")
	if not e.is_empty() and not e["offline"]:
		e["hp"] = 0.0
		e["offline"] = true
		drone.systems.changed = true
		drone.systems.quiet = 0.0


func _done_board() -> bool:
	if not drone_taken:
		_keep_drone_down()
	return drone_taken


# --- the wing

func _enter_wing() -> void:
	_ensure_wingman()


## A wingman for the wing lessons: the drone's prize crew, or (when the
## boarding was skipped) a cadet sent over from the starbase.
func _ensure_wingman() -> void:
	var p := _player()
	if not world.wing.members(p).filter(func(s): return not s.dead).is_empty():
		return
	var at := p.global_position - Vector2.RIGHT.rotated(p.rotation) * 300.0
	var s := world.spawn_ship("Cadet Reyes", p.race_id, "Scout", "Sparrow", at, p.velocity, 0)
	s.rotation = p.rotation
	s.reset_physics_interpolation()
	s.get_node("AI").join(p, false)


func _done_wing() -> bool:
	return world.wing.order_of(_player())["order"] == "form"


func _enter_wing_attack() -> void:
	_ensure_wingman()
	if not pirate_down:
		_ensure_pirate()


func _done_wing_attack() -> bool:
	var o: Dictionary = world.wing.order_of(_player())
	return pirate_down or (o["order"] == "attack" and o["target"] == pirate)


func _done_stealth() -> bool:
	return seen.has("silent") and not _player().sensors.emcon


func _done_shield_focus() -> bool:
	return seen.has("focus_forward") and _player().shield_focus == 0


func _done_shields_down() -> bool:
	return seen.has("shields_down") and _player().shields_raised


# --- point defence

## The yard fits a PDC in the scout's bays (the cadet keeps it), and an
## unmanned launcher off the bow sends missiles in one at a time.
func _enter_pdc() -> void:
	var p := _player()
	if not p.pdc.fitted():
		var d: Dictionary = p.design.duplicate(true)
		d["modules"].append("4022")
		var pos := p.global_position
		var vel := p.velocity
		var rot := p.rotation
		p.refit(d)
		p.global_position = pos
		p.velocity = vel
		p.rotation = rot
		p.body.pos = pos
		p.body.vel = vel
		p.body.angle = rot
	p.pdc.enabled = true
	pdc_kills_at = p.pdc.shot_down
	pdc_fired = 0
	pdc_next = 4.0                  # let Hale get going first
	_remove_launcher()
	_spawn_launcher()


func _spawn_launcher() -> void:
	var p := _player()
	var a := p.rotation + randf_range(-0.5, 0.5)
	launcher = world.spawn_ship("Training launcher", 3, "Scout", "Target Launcher", p.global_position + Vector2(cos(a), sin(a)) * 1100.0, p.velocity, -1)
	launcher.rotation = a + PI
	launcher.reset_physics_interpolation()


func _done_pdc() -> bool:
	var p := _player()
	var downed := p.pdc.shot_down - pdc_kills_at
	if downed < PDC_KILLS and (launcher == null or not is_instance_valid(launcher) or launcher.dead):
		_spawn_launcher()           # a wingman (or the cadet) shot it up before it was done
	if launcher != null and is_instance_valid(launcher) and not launcher.dead and downed < PDC_KILLS and step_time >= pdc_next:
		# one missile at a time, until enough are down
		pdc_next = step_time + PDC_SALVO_GAP
		pdc_fired += 1
		var from: Vector2 = launcher.global_position
		var dir: Vector2 = (p.global_position - from).normalized()
		var md: Dictionary = GameData.MISSILES["008"]
		world.spawn_missile(launcher, "008", from + dir * 30.0, launcher.velocity + dir * md["fire_v"] * GameData.LIGHT_SPEED, dir.angle(), p)
	return downed >= PDC_KILLS and seen.has("pdc_off") and p.pdc.enabled


func _remove_launcher() -> void:
	if launcher != null and is_instance_valid(launcher) and not launcher.dead:
		world.remove_ship(launcher)
	launcher = null


# --- the sparring pirate

func _enter_jammer() -> void:
	_remove_launcher()
	_ensure_pirate()


func _done_jammer() -> bool:
	return seen.has("jamming") or pirate_down


func _enter_spar() -> void:
	if not pirate_down:          # it may have gone down in the jammer lesson
		_ensure_pirate()


## The rookie pirate, brought in for the jammer lesson and fought in the
## spar (one skipped to with Enter brings its own).
func _ensure_pirate() -> void:
	if pirate != null and is_instance_valid(pirate) and not pirate.dead:
		return
	var p := _player()
	pirate_down = false
	var a := p.rotation + randf_range(-0.8, 0.8)
	var at := p.global_position + Vector2(cos(a), sin(a)) * 1500.0
	pirate = world.spawn_ship("Rookie Kell", 3, "Scout", "Rust Mite", at, p.velocity, 0)
	pirate.refit(GameData.stock_design("Scout", 3))  # no Shroud: a cadet locks it at 700 px
	pirate.rotation = a + PI
	pirate.reset_physics_interpolation()
	# a fair first fight: it comes for you (not our supply ships), stays
	# to the end and never jams you
	pirate.systems.base["can_jam"] = false
	pirate.can_jam = false
	pirate.target = p
	var ai: AIPilot = pirate.get_node("AI")
	ai.raid = false
	ai.duel = true
	pirate.died.connect(func(_s, _k): pirate_down = true)


func _done_spar() -> bool:
	return pirate_down


func _on_player_died() -> void:
	if not finished:
		_interject("spar_died")


func _enter_loot() -> void:
	# a fight skipped with Enter ends here
	if pirate != null and is_instance_valid(pirate) and not pirate.dead:
		world.fx.spark(pirate.global_position, 14, Color(0.6, 0.85, 1.0))
		world.remove_ship(pirate)
	pirate = null
	# whatever the fight left, and a little more close by for certain
	var p := _player()
	var fwd := Vector2.RIGHT.rotated(p.rotation)
	world.spawn_powerup("cr:75", p.global_position + fwd * 260.0, p.velocity)
	world.spawn_powerup("5002", p.global_position + fwd * 320.0 + fwd.orthogonal() * 60.0, p.velocity)


func _done_loot() -> bool:
	var p := _player()
	return p.credits > credits_at or p.hold.size() > hold_at


# --- the starbase

func _enter_dock() -> void:
	_clear_marks()     # a drone left over from a skipped lesson goes home


func _done_dock() -> bool:
	return world.nearest_base(_player()) != null


func _done_shipyard() -> bool:
	return main.get("builder") != null


func _done_shipyard_tour() -> bool:
	return main.get("builder") == null


func _done_base_cargo() -> bool:
	return main.get("cargo_panel") != null


# --- worlds, colonists and cargo

func _landed_on(n: String) -> bool:
	var p := _player()
	return p.is_landed() and p.land_body != null and p.land_body.body_name == n


func _done_land() -> bool:
	return _landed_on("Earth")


func _done_colonists() -> bool:
	return _player().colonists > 0.0


## The cargo lesson loads whatever pays best at Luna, from what Earth has.
func _enter_cargo() -> void:
	var earth := _planet("Earth")
	var luna := _planet("Luna")
	cargo_res = "food"
	if earth == null or luna == null:
		return
	var best := -1.0
	for k in Economy.RESOURCES:
		if earth.stock.get(k, 0.0) < 10.0:
			continue
		var pay := Economy.delivery_pay(luna, earth, k, 10.0)
		if pay > best:
			best = pay
			cargo_res = k


func _done_cargo() -> bool:
	return _player().cargo_total() > 0.0


func _done_ground_tabs() -> bool:
	var cp = main.get("cargo_panel")
	return cp != null and cp.tab in ["troops", "buildings"]


func _done_to_luna() -> bool:
	return _landed_on("Luna")


func _done_unload() -> bool:
	var p := _player()
	return p.colonists < 1.0 and p.cargo_total() < 0.5 and p.transfer == 0


func _done_ledger() -> bool:
	return main.get("ledger") != null


# ------------------------------------------------------------ marks

func _clear_marks() -> void:
	if drone != null and is_instance_valid(drone) and not drone.dead and not drone_taken:
		world.remove_ship(drone)
	drone = null


## What the lesson points at: [world position, label, colour], or [].
func _mark() -> Array:
	var m: String = step.get("mark", "")
	match m:
		"":
			return []
		"beacon":
			return [beacon_position(), "Nav beacon", Color(1.0, 0.85, 0.35)]
		"drone":
			if drone != null and is_instance_valid(drone) and not drone.dead:
				return [drone.global_position, "Target drone", Color(1.0, 0.45, 0.35)]
			return []
		"pirate":
			if pirate != null and is_instance_valid(pirate) and not pirate.dead:
				return [pirate.global_position, "Pirate", Color(1.0, 0.45, 0.35)]
			return []
		"loot":
			var p := _player()
			var best: Node2D = null
			for pu in world.powerups:
				if is_instance_valid(pu) and (best == null or pu.global_position.distance_to(p.global_position) < best.global_position.distance_to(p.global_position)):
					best = pu
			return [best.global_position, "Pickup", Color(0.5, 1.0, 0.6)] if best != null else []
		"starbase":
			var b := world.starbase_of(1)
			return [b.global_position, Names.fill("{terran} starbase"), Color(0.45, 1.0, 0.6)] if b != null else []
		"freeport":
			for s in world.ships:
				if s.race_id == GameData.NEUTRAL and s.design_name == "TradeStation" and not s.dead:
					return [s.global_position, "Free port", GameData.race(GameData.NEUTRAL)["color"]]
			return []
		"wingman":
			var wm: Array = world.wing.members(_player()).filter(func(s): return not s.dead)
			return [wm[0].global_position, "Your wingman", HUD.WING_COLOR] if not wm.is_empty() else []
		"hauler":
			for s in world.ships:
				if s.support and s.race_id == 1 and not s.dead:
					return [s.global_position, "%s (supply)" % s.ship_name, GameData.race(1)["color"]]
			return []
	var pl := _planet(m)
	if pl != null:
		var col: Color = GameData.race(pl.owner_race)["color"] if pl.is_colonised() else Color(0.85, 0.9, 1.0)
		return [pl.global_position, pl.body_name, col]
	return []


# ------------------------------------------------------------ drawing

func _covered() -> bool:
	if main == null:
		return false
	return main.get("builder") != null or main.get("tactical") != null or main.get("ledger") != null


func _draw() -> void:
	if world == null:
		return
	var vp := get_viewport_rect().size
	if not _covered():
		_draw_focus(vp)
		_draw_mark(vp)
	if finished:
		_draw_finished(vp)
		return
	_draw_panel(vp)


## The subtitle panel: who speaks, the words as they are spoken, the goal.
func _draw_panel(vp: Vector2) -> void:
	# over the shipyard or a map: wider, smaller and low, in the space they leave
	var compact := _covered()
	var size := 16 if compact else TEXT_SIZE
	var lh := 21.0 if compact else LINE_H
	var w := minf(900.0 if compact else PANEL_W, vp.x - 40.0)
	var spk := TutorialLines.speaker(line_id if line_id != "" else step.get("id", ""))
	var shown_id: String = line_id if speaking() else step.get("id", "")
	var text := TutorialLines.text(shown_id)
	var lines := _wrap(text, w - 40.0, size)
	var goal := _goal_text()
	var goal_lines := _wrap(goal, w - 64.0, 16) if goal != "" else PackedStringArray()
	var h := 44.0 + lines.size() * lh + (10.0 + goal_lines.size() * 21.0 if goal != "" else 0.0) + (8.0 if compact else 30.0)
	var x := (vp.x - w) * 0.5
	var y := vp.y - 215.0 - h
	if compact:
		y = vp.y - h - 24.0
		if main.get("tactical") != null:
			w = minf(w, 800.0)
			x = vp.x - w - 40.0     # over the map's pilot list, clear of the system itself
	draw_rect(Rect2(x, y, w, h), Color(0.02, 0.04, 0.09, 0.93))
	var accent: Color = spk.get("color", Color(0.55, 0.8, 1.0))
	draw_rect(Rect2(x, y, w, h), Color(accent.r, accent.g, accent.b, 0.55), false, 1.5)
	draw_rect(Rect2(x, y, 4, h), accent)
	# who, and where we are
	var talking := speaking() and voice.playing
	var r := 6.0 + (1.5 * sin(time * 14.0) if talking else 0.0)
	draw_circle(Vector2(x + 22, y + 20), r, accent)
	_text(Vector2(x + 36, y + 26), "%s  ·  %s" % [spk.get("name", "Commander Hale").to_upper(), spk.get("title", "")], HORIZONTAL_ALIGNMENT_LEFT, -1, 14, accent)
	var where := "%s   %d / %d" % [CHAPTERS[step.get("chapter", 0)], index + 1, STEPS.size()]
	_text(Vector2(x, y + 26), where, HORIZONTAL_ALIGNMENT_RIGHT, w - 16.0, 13, Color(0.7, 0.78, 0.9, 0.85))
	# the words, revealed as they are said
	var shown := text.length()
	if speaking() and line_len > 0.0:
		shown = int(ceil(text.length() * clampf(line_time / maxf(line_len - 0.4, 0.1), 0.0, 1.0)))
	var ly := y + 54.0
	var used := 0
	for ln in lines:
		var part := ln
		if used + ln.length() > shown:
			part = ln.left(maxi(shown - used, 0))
		_text(Vector2(x + 20, ly), part, HORIZONTAL_ALIGNMENT_LEFT, -1, size, Color(0.95, 0.97, 1.0))
		used += ln.length() + 1
		ly += lh
	# the objective
	if goal != "":
		ly += 6.0
		var done := goal_met
		var gc := Color(0.55, 1.0, 0.6) if done else Color(1.0, 0.85, 0.45)
		var mark_x := x + 22.0
		if done:
			draw_polyline(PackedVector2Array([Vector2(mark_x - 6, ly - 6), Vector2(mark_x - 1, ly - 1), Vector2(mark_x + 8, ly - 12)]), gc, 2.5)
		else:
			var pulse := 0.6 + 0.4 * sin(time * 4.0)
			draw_colored_polygon(PackedVector2Array([Vector2(mark_x - 5, ly - 13), Vector2(mark_x + 5, ly - 7), Vector2(mark_x - 5, ly - 1)]), Color(gc.r, gc.g, gc.b, pulse))
		for gl in goal_lines:
			_text(Vector2(x + 40, ly), gl, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, gc)
			ly += 21.0
	if compact:
		return
	var keys := "Enter: skip   Shift+Enter: next chapter   Backspace: repeat"
	if not has_goal():
		keys = "Enter: next   Shift+Enter: next chapter   Backspace: repeat"
	_text(Vector2(x, y + h - 10.0), keys, HORIZONTAL_ALIGNMENT_CENTER, w, 12, Color(0.65, 0.72, 0.85, 0.7))


## The goal with the keys as they are bound now, and its progress.
func _goal_text() -> String:
	var g: String = step.get("goal", "")
	if g == "":
		return ""
	var out := g
	var i := out.find("{")
	while i >= 0:
		var j := out.find("}", i)
		if j < 0:
			break
		var key := out.substr(i + 1, j - i - 1)
		var rep := cargo_res.capitalize() if key == "res" else ("[%s]" % Keybinds.short_text(key))
		out = out.substr(0, i) + rep + out.substr(j + 1)
		i = out.find("{", i + rep.length())
	var p := _player()
	if p != null and not goal_met:
		match step.get("id", ""):
			"thrust":
				var base := world.starbase_of(p.race_id)
				if base != null:
					out += "  (%d / %d)" % [mini(int(p.global_position.distance_to(base.global_position)), int(THRUST_OUT)), int(THRUST_OUT)]
			"beacon":
				out += "  (%d away)" % int(p.global_position.distance_to(beacon_position()))
			"board":
				if p.boarding != null:
					out += "  (boarding %d%%)" % int(p.board_progress * 100.0)
	return out + "."


## Word wrap to `width` px at `size`.
func _wrap(text: String, width: float, size: int) -> PackedStringArray:
	var out := PackedStringArray()
	var line := ""
	for word in text.split(" "):
		var t := word if line == "" else line + " " + word
		if line != "" and font.get_string_size(t, HORIZONTAL_ALIGNMENT_LEFT, -1, size).x > width:
			out.append(line)
			line = word
		else:
			line = t
	if line != "":
		out.append(line)
	return out


func _text(pos: Vector2, text: String, align: HorizontalAlignment, width: float, size: int, col: Color) -> void:
	draw_string_outline(font, pos, text, align, width, size, 4, Color(0, 0, 0, 0.75 * col.a))
	draw_string(font, pos, text, align, width, size, col)


## Outline the HUD panel the line is about.  The rectangles follow HUD.gd.
func _draw_focus(vp: Vector2) -> void:
	var rects := []
	match step.get("focus", ""):
		"status":
			rects = [Rect2(14, vp.y - 210, 320, 184)]
		"weapons_radar":
			rects = [Rect2(340, vp.y - 164, 300, 134), Rect2(vp.x - 280, vp.y - 280, 260, 270)]
		"signature":
			rects = [Rect2(vp.x - 346, vp.y - 300, 336, 290)]
		"wing":
			var n: int = maxi(world.wing.members(_player()).size(), 1)
			rects = [Rect2(vp.x - 346, vp.y - 306 - 40.0 - n * 20.0, 336, 52.0 + n * 20.0)]
		"top":
			rects = [Rect2(14, 6, 600, 48), Rect2(vp.x * 0.5 - 280, 4, 520, 34)]
	var a := 0.55 + 0.45 * sin(time * 5.0)
	for r in rects:
		draw_rect(r, Color(1.0, 0.85, 0.35, a), false, 2.5)


## The thing a lesson is about: a ring on it on screen, an arrow on the
## edge when it is off screen.
func _draw_mark(vp: Vector2) -> void:
	var p := _player()
	if p == null or p.dead:
		return
	var m := _mark()
	if m.is_empty():
		return
	var xf := world.get_viewport().get_canvas_transform()
	var sp: Vector2 = xf * m[0]
	var col: Color = m[2]
	var label := "%s  %d" % [m[1], int(p.global_position.distance_to(m[0]))]
	var inner := Rect2(Vector2(60, 60), vp - Vector2(120, 120))
	if inner.has_point(sp):
		var r := 34.0 + 5.0 * sin(time * 4.0)
		if step.get("mark", "") == "beacon":
			# a light at the centre, and the ring you have to reach
			var reach := maxf(BEACON_REACH * xf.get_scale().x, 24.0)
			draw_arc(sp, reach, 0, TAU, 64, Color(col.r, col.g, col.b, 0.35), 1.5)
			var d := 9.0 + 2.0 * sin(time * 6.0)
			draw_circle(sp, d * 2.2, Color(col.r, col.g, col.b, 0.15 + 0.1 * sin(time * 6.0)))
			draw_colored_polygon(PackedVector2Array([sp + Vector2(0, -d), sp + Vector2(d, 0), sp + Vector2(0, d), sp + Vector2(-d, 0)]), col)
		draw_arc(sp, r, 0, TAU, 48, Color(col.r, col.g, col.b, 0.8), 2.0)
		draw_arc(sp, r + 7.0, time * 2.0, time * 2.0 + PI * 0.6, 16, col, 2.5)
		draw_arc(sp, r + 7.0, time * 2.0 + PI, time * 2.0 + PI * 1.6, 16, col, 2.5)
		_text(sp + Vector2(-100, -r - 12.0), label, HORIZONTAL_ALIGNMENT_CENTER, 200, 14, col)
		return
	var centre := vp * 0.5
	var dir := (sp - centre).normalized()
	var half := vp * 0.5 - Vector2(70, 70)
	var k := minf(absf(half.x / maxf(absf(dir.x), 0.001)), absf(half.y / maxf(absf(dir.y), 0.001)))
	var at := centre + dir * k
	var side := dir.orthogonal()
	var bob := 4.0 * sin(time * 5.0)
	at += dir * bob
	draw_colored_polygon(PackedVector2Array([at + dir * 22.0, at - dir * 8.0 + side * 14.0, at - dir * 8.0 - side * 14.0]), col)
	_text(at - dir * 40.0 + Vector2(-90, 5), label, HORIZONTAL_ALIGNMENT_CENTER, 180, 14, col)


func _draw_finished(vp: Vector2) -> void:
	var age := time - finished_time
	var a := clampf(1.0 - (age - 12.0) / 2.0, 0.0, 1.0)
	if a <= 0.0:
		return
	var w := 620.0
	var x := (vp.x - w) * 0.5
	var y := vp.y - 330.0
	draw_rect(Rect2(x, y, w, 92), Color(0.02, 0.04, 0.09, 0.86 * a))
	draw_rect(Rect2(x, y, w, 92), Color(0.55, 0.8, 1.0, 0.6 * a), false, 1.5)
	_text(Vector2(x, y + 36), "TUTORIAL COMPLETE", HORIZONTAL_ALIGNMENT_CENTER, w, 24, Color(0.85, 0.95, 1.0, a))
	_text(Vector2(x, y + 68), "Keep flying as long as you like.  Esc opens the menu.", HORIZONTAL_ALIGNMENT_CENTER, w, 16, Color(0.8, 0.88, 1.0, a))
