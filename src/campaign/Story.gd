## Story -- the director of the story campaign, "The Frontier War".  It
## rides on a campaign World the way Round does and runs on the host: it
## offers one operation at a time, runs it (an Op script), and between
## operations leaves the war to run as it always does.
##
## The war keeps whatever an operation did to it.  An operation is never
## saved halfway: the campaign is saved as one begins, and a failed one is
## tried again from that save (Main.retry_operation).  The voice queue is
## here too, so every player's Comms window plays the same line.
##
## In co-op the host runs it and each friend's game has a mirror (mirror =
## true) that Net fills from pack_live(), pack_marks() and the lines as the
## host's voice starts them.  The mirror never runs an operation: the UI
## asks it through op_title(), objectives(), marks() and briefing_ready().
class_name Story
extends RefCounted

signal line_started(id: String)
signal changed()
signal choice_asked()
signal hushed()

## The operations in order.  wait: seconds of open war before it is
## offered (counted from the end of the one before).
const OPS := [
	{"id": "first_watch", "act": 1, "title": "First Watch", "where": "Earth to the Jupiter free port", "wait": 6.0,
		"call": "call_first_watch", "brief": ["fw_brief_1", "fw_brief_2", "fw_brief_3"],
		"summary": "Escort the supply ship Cormorant from Earth to the free port at Jupiter's Trojan point with Lt. Brandt. The Ceres Accord holds: never fire first on a Drengin or Arcean ship.",
		"script": "res://src/campaign/ops/FirstWatch.gd"},
	{"id": "dark_convoy", "act": 1, "title": "Dark Convoy", "where": "The asteroid belt, near Ceres", "wait": 150.0,
		"call": "call_dark_convoy", "brief": ["dc_brief_1", "dc_brief_2", "dc_brief_3"],
		"summary": "Find the missing fuel convoy near Ceres and scan the wrecks. If you meet the raiders, disable one and board it.",
		"script": "res://src/campaign/ops/DarkConvoy.gd"},
	{"id": "free_port", "act": 1, "title": "Free Port", "where": "The free port at Jupiter's Trojan point", "wait": 150.0,
		"call": "call_free_port", "brief": ["port_brief_1", "port_brief_2", "port_brief_3"],
		"summary": "Get the raiders' transponder records from the broker Ilsa Marrow at the free port. Pay her price, or copy them under silent running. No weapons at the port.",
		"script": "res://src/campaign/ops/FreePort.gd"},
	{"id": "picket_line", "act": 1, "title": "Picket Line", "where": "Earth to Luna", "wait": 150.0,
		"call": "call_picket_line", "brief": ["pk_brief_1", "pk_brief_2", "pk_brief_3"],
		"summary": "Land on Earth for 8 Alloys and deliver them to the defense station frame over Luna with Lt. Brandt, then guard it until it is built.",
		"script": "res://src/campaign/ops/PicketLine.gd"},
	{"id": "accord_breaks", "act": 1, "title": "The Accord Breaks", "where": "Luna", "wait": 120.0,
		"call": "call_accord_breaks", "brief": ["ab_brief_1", "ab_brief_2", "ab_brief_3"],
		"summary": "The Drengin come for Luna. Hold it with Lt. Brandt for six minutes until the fleet arrives. If Luna falls, fall back: the war goes on either way.",
		"script": "res://src/campaign/ops/AccordBreaks.gd"},
	{"id": "shields_down", "act": 2, "title": "Shields Down", "where": "Deimos, over Mars", "wait": 150.0,
		"call": "call_shields_down", "brief": ["sd_brief_1", "sd_brief_2", "sd_brief_3"],
		"summary": "Shoot down Deimos's planetary shield, cover the drop ships Anvil and Bastion down to the surface, and hold over the ground battle until Deimos is Terran.",
		"script": "res://src/campaign/ops/ShieldsDown.gd"},
	{"id": "wolf_pack", "act": 2, "title": "Wolf Pack", "where": "The Venus to Mars lane", "wait": 150.0,
		"call": "call_wolf_pack", "brief": ["wp_brief_1", "wp_brief_2", "wp_brief_3"],
		"summary": "Lead your own wing, Lantern Three and Four, against a Drengin supply convoy running from Venus to Mars. Two freighters through and it's over.",
		"script": "res://src/campaign/ops/WolfPack.gd"},
	{"id": "envoy", "act": 2, "title": "The Envoy", "where": "Saturn to Earth", "wait": 150.0,
		"call": "call_envoy", "brief": ["env_brief_1", "env_brief_2", "env_brief_3"],
		"summary": "Meet the Arcean envoy Speaker Tenn in the belt and bring the ship safely to the Terran starbase. Pirates work that stretch. Never fire on an Arcean ship.",
		"script": "res://src/campaign/ops/Envoy.gd"},
	{"id": "silent_running", "act": 2, "title": "Silent Running", "where": "Mars orbit", "wait": 150.0,
		"call": "call_silent_running", "brief": ["sr_brief_1", "sr_brief_2", "sr_brief_3"],
		"summary": "Slip through the picket net round the shipyard over Mars under silent running, scan the Hammer, and get out unheard. Jam a picket that comes too close.",
		"script": "res://src/campaign/ops/SilentRunning.gd"},
	{"id": "burn_yards", "act": 2, "title": "Burn the Yards", "where": "The Varr shipyard over Mars", "wait": 120.0,
		"call": "call_burn_yards", "brief": ["by_brief_1", "by_brief_2", "by_brief_3"],
		"summary": "Go in with the Resolute's strike group and burn the Hammer's shipyard before its cradle opens. If the Hammer launches, fall back: the war goes on either way.",
		"script": "res://src/campaign/ops/BurnTheYards.gd"},
]
## The story's Sol is this much bigger than Conquest's (Kyle: "scale the
## map up huge ... it takes time to get places").
const MAP_SCALE := 3.0
const ACTS := ["", "Act 1: Cold Orbit", "Act 2: Open War", "Act 3: Hammer of Mars"]
const PORT_MARKUP := 0.25         # on the free port's prices for Terrans once the records were stolen
const BREACH_WARN_EVERY := 30.0   # Hale's "hold your fire" between operations

var world: Node
var index := 0                    # the operation offered or under way; OPS.size() when the story so far is told
var op: StoryOp = null            # the operation under way
var flags := {}                   # what the story remembers: choices, outcomes
var unlock_at := 0.0              # world.elapsed when the next operation is offered
var called := false               # Hale has called the player in for it
var failed := ""                  # why the operation failed ("" = it did not): waiting for a retry
var briefing_open := false        # the player is reading the briefing (Main's Briefing)
## A story choice waiting for the player: {id, question, options: [[key,
## label, what it means], ...]} (Main's StoryChoice shows it).
var choice := {}
var _last_warn := -100.0
var _player_down := false         # Hale has said the player is down
var _was_ready := false           # briefing_ready() last tick: the friends hear when it changes
var _marked_up := false           # the free port's prices for Terrans are up (Free Port, stolen records)
## Online: pilot name -> the answer a friend would pick (the host decides).
var votes := {}
var mirror := false               # a friend's copy of the host's story (Net fills it)
var _live := {}                   # the mirror's operation: {title, objectives, ready}
var _marks: Array = []            # the mirror's marks as sent: [kind, ref, at, label, colour]

# the voice: lines wait their turn, and every player hears the same one
var line := ""                    # the line being spoken
var line_left := 0.0
var queue: Array = []


func _init(p_world: Node, saved: Dictionary = {}, p_mirror := false) -> void:
	world = p_world
	mirror = p_mirror
	if mirror:
		unlock_at = INF
		return
	if saved.is_empty():
		# a new story begins in peacetime
		world.accord = true
		unlock_at = world.elapsed + float(OPS[0]["wait"])
	else:
		index = clampi(int(saved.get("index", 0)), 0, OPS.size())
		flags = saved.get("flags", {}).duplicate(true)
		world.accord = bool(saved.get("accord", true))
		unlock_at = world.elapsed + float(saved.get("wait", 4.0))
		keep_peace()


func pack() -> Dictionary:
	return {"index": index, "flags": flags.duplicate(true), "accord": world.accord, "wait": maxf(unlock_at - world.elapsed, 4.0)}


## Once the Accord is gone the Arceans stay out of Terra's war with Mars,
## unless the story has turned them.
func keep_peace() -> void:
	world.peace = [[0, 1]] if not world.accord and flags.get("arceans", "") != "hostile" else []


func in_op() -> bool:
	return op != null or failed != "" or op_title() != ""


## The operation under way ("" between operations).
func op_title() -> String:
	if op != null:
		return op.spec.get("title", "")
	return str(_live.get("title", "")) if mirror else ""


## Its first open objective, for a reminder.
func objective_line() -> String:
	if op != null:
		return op.objective_line()
	for o in objectives():
		if not o["done"] and not o["optional"]:
			return o["text"] + "."
	return ""


func current() -> Dictionary:
	return OPS[index] if index < OPS.size() else {}


## The next operation can be briefed (no other under way, its time come).
func briefing_ready() -> bool:
	if mirror:
		return bool(_live.get("ready", false))
	return op == null and failed == "" and index < OPS.size() and world.elapsed >= unlock_at


# ------------------------------------------------------------ the voice

func say(id: String) -> void:
	if StoryLines.text(id) == "":
		push_warning("Story: no line %s" % id)
		return
	if line == "" and queue.is_empty():
		_start_line(id)
	elif id != line and not queue.has(id):
		queue.append(id)


## Cut the voice short: everything waiting is dropped too.
func hush() -> void:
	line = ""
	line_left = 0.0
	queue.clear()
	hushed.emit()


func speaking() -> bool:
	return line != "" or not queue.is_empty()


static func line_length(id: String) -> float:
	var path := StoryLines.audio_path(id)
	var st: AudioStream = load(path) if ResourceLoader.exists(path) else null
	if st != null:
		return st.get_length() + 0.35
	return 1.5 + StoryLines.text(id).length() / 16.0     # no recording: reading pace


func _start_line(id: String) -> void:
	line = id
	line_left = line_length(id)
	line_started.emit(id)


## The voice clock runs in real time, also while the war waits for a
## briefing or the shipyard (Comms calls this every frame).
func tick_voice(delta: float) -> void:
	if line == "":
		if not queue.is_empty():
			_start_line(queue.pop_front())
		return
	line_left -= delta
	if line_left <= 0.0:
		line = ""
		if not queue.is_empty():
			_start_line(queue.pop_front())


# ------------------------------------------------------------ operations

func tick(delta: float) -> void:
	if world.game_over:
		return
	if not _marked_up and flags.get("port_fees", "") == "high":
		_marked_up = mark_up_port()
	if op != null:
		var p: Ship = world.player
		if p != null and p.dead and not _player_down:
			say("player_down")
		_player_down = p != null and p.dead
		op.tick(delta)
		match op.state:
			"won":
				_finish()
			"failed":
				_fail(op.fail_reason)
		return
	if briefing_ready() != _was_ready:
		_was_ready = not _was_ready
		changed.emit()
	if not called and briefing_ready() and not briefing_open:
		called = true
		say(current()["call"])
		world.notify_all("Briefing ready: %s. Press %s." % [current()["title"], Keybinds.short_text("briefing")], Color(0.85, 0.9, 1.0))
		changed.emit()


## Launch the operation on offer.  The campaign is saved first: that save
## is where a retry starts.
func start_op() -> void:
	if not briefing_ready():
		return
	world.save_campaign(false)
	var spec := current()
	op = load(spec["script"]).new()
	op.story = self
	op.world = world
	op.spec = spec
	called = true
	op.setup()
	world.notify_all("Operation %s: %s" % [spec["title"], op.objective_line()], Color(0.75, 0.9, 1.0))
	changed.emit()


func _finish() -> void:
	var spec := current()
	choice = {}
	op.cleanup()
	op = null
	flags[spec["id"]] = "won"
	index += 1
	called = false
	if index < OPS.size():
		unlock_at = world.elapsed + float(OPS[index]["wait"])
	else:
		unlock_at = INF
		say("call_more")
	world.notify_all("Operation %s complete." % spec["title"], Color(0.6, 1.0, 0.7))
	Sfx.play_ui("rankup", 0.8)
	world.save_campaign(false)
	changed.emit()


func _fail(reason: String) -> void:
	failed = reason
	choice = {}
	op.cleanup()
	op = null
	world.notify_all("Operation failed: %s %s to try it again from the briefing." % [reason, "The host presses " + Keybinds.short_text("briefing") if Net.is_host() and not Net.seats.is_empty() else "Press " + Keybinds.short_text("briefing")], Color(1.0, 0.6, 0.45))
	changed.emit()


## The free port charges Terrans more for the rest of the war (Economy).
func mark_up_port() -> bool:
	for s in world.ships:
		if s.race_id == GameData.NEUTRAL and s.design_name == "TradeStation" and not s.dead:
			s.set_meta("markup", {1: PORT_MARKUP})
			return true
	return false


# ------------------------------------------------------------ choices

## Put a choice to the player.  The operation hears the answer in
## on_choice, and flags keep it as "choice_<id>".
func ask(id: String, question: String, options: Array) -> void:
	choice = {"id": id, "question": question, "options": options}
	choice_asked.emit()
	changed.emit()


func choose(key: String) -> void:
	if choice.is_empty():
		return
	var id: String = choice["id"]
	choice = {}
	votes = {}
	flags["choice_" + id] = key
	if op != null:
		op.on_choice(id, key)
	changed.emit()


# ------------------------------------------------------------ the war around it

## Someone fired across the Accord (World.accord_breach).
func on_breach(attacker: Node, victim: Node) -> void:
	if op != null:
		op.on_breach(attacker, victim)
		if op.state != "running":
			return
	if attacker == world.player and world.elapsed - _last_warn > BREACH_WARN_EVERY:
		_last_warn = world.elapsed
		say("breach_warn" if world.accord else "breach_warn_arcean")


## A friend online would pick `key` (the host's StoryChoice shows the tally).
func vote(pilot: String, key: String) -> void:
	if choice.is_empty():
		return
	for row in choice.get("options", []):
		if row[0] == key:
			votes[pilot] = key
			changed.emit()


## What the objective panel and the screen marks show.
func objectives() -> Array:
	if op != null:
		return op.objectives
	return _live.get("objectives", []) if mirror else []


## [world position, label, colour] for each open objective with a mark.
func marks() -> Array:
	if op != null:
		return op.marks()
	if not mirror:
		return []
	var out := []
	for m in _marks:
		var at: Vector2 = m[2]
		match int(m[0]):
			1:
				var s: Node = Net.ships_by_id.get(int(m[1]), null)
				if s == null or not is_instance_valid(s) or s.dead:
					continue
				at += s.global_position
			2:
				if int(m[1]) < 0 or int(m[1]) >= world.planets.size():
					continue
				at = world.planets[int(m[1])].global_position
		out.append([at, m[3], m[4]])
	return out


# ------------------------------------------------------------ co-op

## Where the story stands, for the friends' mirrors.
func pack_live() -> Array:
	var rows := []
	for o in objectives():
		rows.append([o["text"], o["done"], o["failed"], o["optional"]])
	return [index, flags.duplicate(true), world.accord, failed, op_title(), rows, briefing_ready(), choice.duplicate(true), votes.duplicate(),
		maxf(unlock_at - world.elapsed, 0.0) if unlock_at != INF else -1.0, world.peace.duplicate(true)]


func apply_live(state: Array) -> void:
	if state.size() < 10:
		return
	var had_choice := not choice.is_empty()
	index = int(state[0])
	flags = state[1]
	world.accord = bool(state[2])
	failed = str(state[3])
	var rows := []
	for r in state[5]:
		rows.append({"text": r[0], "done": r[1], "failed": r[2], "optional": r[3]})
	_live = {"title": str(state[4]), "objectives": rows, "ready": bool(state[6])}
	choice = state[7]
	votes = state[8]
	unlock_at = world.elapsed + float(state[9]) if float(state[9]) >= 0.0 else INF
	if state.size() > 10:
		world.peace = state[10]
	if not had_choice and not choice.is_empty():
		choice_asked.emit()
	changed.emit()


## The marks, by what they ride on: a ship by its net id (and an offset
## from it), a world by its index, else a spot.
func pack_marks() -> Array:
	var out := []
	for m in marks():
		var on: Node2D = m[3] if m.size() > 3 else null
		if on is StoryOp.Waypoint and on.follow is Ship and on.follow.net_id != 0:
			out.append([1, on.follow.net_id, m[0] - on.follow.global_position, m[1], m[2]])
		elif on is Ship and on.net_id != 0:
			out.append([1, on.net_id, Vector2.ZERO, m[1], m[2]])
		elif on is Planet:
			out.append([2, world.planets.find(on), Vector2.ZERO, m[1], m[2]])
		else:
			out.append([0, 0, m[0], m[1], m[2]])
	return out


func apply_marks(rows: Array) -> void:
	_marks = rows
