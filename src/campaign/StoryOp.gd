## StoryOp -- one operation of the story campaign.  An operation script
## extends this: setup() brings in its ships and first objectives, step()
## watches the war and moves the story on, and win() or fail() ends it.
## Story runs it on the host.  In co-op every friend flying with the host
## counts: pilots(), pilot_near() and is_pilot() are how an operation asks
## "did someone get there", and scaled() grows its enemies with the squad.
class_name StoryOp
extends RefCounted

var story: Story
var world: Node
var spec: Dictionary = {}
var state := "running"            # running | won | failed
var fail_reason := ""
var time := 0.0                   # seconds since it began
var stage := ""                   # where the script is: its own words
var stage_time := 0.0             # seconds in this stage
## {id, text, done, failed, optional, mark (a Node2D, a Vector2 or a Callable returning one), label}
var objectives: Array = []
var spawned: Array = []           # the ships it brought in, taken away when it ends
var _said := {}


func setup() -> void:
	pass


func step(_delta: float) -> void:
	pass


func tick(delta: float) -> void:
	time += delta
	stage_time += delta
	if state == "running":
		step(delta)


func go(next_stage: String) -> void:
	stage = next_stage
	stage_time = 0.0


func player() -> Ship:
	return world.player


func planet(n: String) -> Planet:
	return world.planet_by_name.get(n, null)


## Everyone flying the operation: the host's pilot and each friend seated
## with them online (alive or waiting to relaunch).
func pilots() -> Array:
	var out := []
	if world.player != null:
		out.append(world.player)
	if Net.is_host():
		for s in Net.seated_ships():
			if not out.has(s):
				out.append(s)
	return out


## The nearest living pilot within `reach` of `at` (null: nobody there).
func pilot_near(at: Vector2, reach: float) -> Ship:
	var best: Ship = null
	for s in pilots():
		if alive(s) and s.global_position.distance_to(at) < reach:
			if best == null or s.global_position.distance_to(at) < best.global_position.distance_to(at):
				best = s
	return best


## A living pilot, the host's first, for where the story happens to them.
func lead_pilot() -> Ship:
	for s in pilots():
		if alive(s):
			return s
	return player()


func is_pilot(n: Variant) -> bool:
	return n is Ship and pilots().has(n)


## `n` enemies for one pilot: each friend flying along adds about half again.
func scaled(n: int) -> int:
	return roundi(n * (1.0 + 0.5 * (pilots().size() - 1)))


## The operation's pay, to every pilot.
func pay(amount: int) -> void:
	for s in pilots():
		s.earn(amount)
		world.notify(s, "Operation pay: %d credits." % amount)
	# the Lantern pilots who flew it take their share
	for s in spawned:
		if is_instance_valid(s) and s.has_meta("lantern") and Wing.has_purse(s):
			s.earn(int(amount * Wing.OP_SHARE))


# ------------------------------------------------------------ objectives

func objective(id: String, text: String, mark: Variant = null, label: String = "", optional: bool = false) -> void:
	for o in objectives:
		if o["id"] == id:
			return
	objectives.append({"id": id, "text": text, "done": false, "failed": false, "optional": optional, "mark": mark, "label": label})
	story.changed.emit()


func complete(id: String) -> void:
	for o in objectives:
		if o["id"] == id and not o["done"]:
			o["done"] = true
			Sfx.play_ui("click", 0.5)
			story.changed.emit()


## Reword an objective (a countdown, a count).
func set_text(id: String, text: String) -> void:
	for o in objectives:
		if o["id"] == id and o["text"] != text:
			o["text"] = text
			story.changed.emit()


func fail_objective(id: String) -> void:
	for o in objectives:
		if o["id"] == id:
			o["failed"] = true
			story.changed.emit()


func done(id: String) -> bool:
	for o in objectives:
		if o["id"] == id:
			return o["done"]
	return false


## The first open objective, for the launch message.
func objective_line() -> String:
	for o in objectives:
		if not o["done"] and not o["optional"]:
			return o["text"] + "."
	return ""


## [world position, label, colour, what it is on] for each open objective
## with a mark (the last is the node, or null for a bare spot: Story.pack_marks
## sends a ship or a world by its id, so a friend's ring rides on it).
func marks() -> Array:
	var out := []
	for o in objectives:
		if o["done"] or o["failed"] or o["mark"] == null:
			continue
		var at: Variant = o["mark"]
		if at is Callable:
			at = at.call()
		var on: Node2D = null
		if at is Node2D:
			if not is_instance_valid(at) or (at is Ship and at.dead):
				continue
			on = at
			at = on.global_position
		if at is Vector2:
			out.append([at, o["label"] if o["label"] != "" else o["text"], Color(0.6, 1.0, 0.65) if o["optional"] else Color(1.0, 0.85, 0.35), on])
	return out


# ------------------------------------------------------------ the story

func say(id: String) -> void:
	story.say(id)


## Say it the first time only.
func say_once(id: String) -> void:
	if not _said.has(id):
		_said[id] = true
		story.say(id)


func win() -> void:
	if state == "running":
		state = "won"


func fail(reason: String, line: String = "") -> void:
	if state != "running":
		return
	state = "failed"
	fail_reason = reason
	if line != "":
		story.hush()
		story.say(line)


## Someone fired across the Accord during this operation.
func on_breach(_attacker: Node, _victim: Node) -> void:
	pass


## The player answered a choice put with ask().
## What the story key does mid-operation ("" when it only reports the
## objective), and doing it for the pilot `by` (a friend's press comes
## through Net).
func signal_label() -> String:
	return ""


func on_signal(_by: Ship) -> void:
	pass


func on_choice(_id: String, _key: String) -> void:
	pass


func ask(id: String, question: String, options: Array) -> void:
	story.ask(id, question, options)


## Something the story remembers from here on (kept only if the operation
## is won: a retry starts from the save made before it).
func flag(key: String, value: Variant) -> void:
	story.flags[key] = value


# ------------------------------------------------------------ ships

## A ship for this operation, flown by a computer pilot of `rank` (-1:
## nobody at the helm).  It never relaunches, and leaves when the
## operation ends.
func spawn(pilot: String, race: int, design: String, ship_name: String, pos: Vector2, vel: Vector2, rank: int = -1, hull_race: int = -1) -> Ship:
	var s: Ship = world.spawn_ship(pilot, race, design, ship_name, pos, vel, rank, hull_race)
	s.set_meta("story", true)
	story.dress(s, rank)               # a Lantern pilot flies the ship the player outfitted
	spawned.append(s)
	Net.host_ship_added(s)            # friends online see it too
	var ai: AIPilot = s.get_node_or_null("AI")
	if ai != null:
		# the operation gives the orders: no errands of its own
		ai.ferry = false
		ai.bombard = false
		ai.raid = false
		ai.escort_duty = false
	return s


func alive(s) -> bool:              # untyped: a ship taken away is freed
	return s != null and is_instance_valid(s) and not s.dead


## A free spot about `dist` from `from`, clear of every world.
func clear_spot(from: Vector2, dir: float, dist: float) -> Vector2:
	var at := from + Vector2.RIGHT.rotated(dir) * dist
	for tries in 12:
		var ok := true
		for pl in world.planets:
			if at.distance_to(pl.global_position) < pl.radius + 400.0:
				ok = false
		if ok:
			return at
		at = from + (at - from).rotated(PI / 6.0)
	return at


## How far `at` is from the nearest armed place: a colony (its guns and
## batteries) or an empire's station (the free port fires on nobody),
## other than those named in `except`.
func guns_near(at: Vector2, except: Array = []) -> float:
	var best := INF
	for pl in world.planets:
		if pl.is_colonised() and not except.has(pl):
			best = minf(best, at.distance_to(pl.global_position) - pl.radius)
	for s in world.ships:
		if s.is_station and not s.dead and s.race_id != GameData.NEUTRAL and not except.has(s):
			best = minf(best, at.distance_to(s.global_position))
	return best


## Hold a ship dead in space (engines out, no damage control) until it is
## boarded.
func keep_down(s: Ship) -> void:
	if not alive(s):
		return
	var e: Dictionary = s.systems.get_system("engines")
	if not e.is_empty() and not e["offline"]:
		e["hp"] = 0.0
		e["offline"] = true
		s.systems.changed = true
	s.systems.quiet = 0.0


## The Hammer's shipyard over Mars, `height` over its surface on the side
## away from the Drengin starbase (Silent Running and Burn the Yards).
func hammer_yard(height: float) -> Ship:
	var m := planet("Mars")
	var base: Ship = world.starbase_of(2)
	var away := (m.global_position - base.global_position).normalized() if base != null else Vector2.RIGHT
	var y: Ship = world.build_station(2, "Shipyard", m, m.global_position + away * (m.radius + height), false)
	y.set_meta("story", true)
	y.ship_name = "Varr Yards"
	spawned.append(y)
	return y


## The Hammer in its cradle beside the yard, untouchable while it is.
func hold_in_cradle(h: Ship, y: Ship, off: Vector2) -> void:
	if not alive(h) or y == null or not is_instance_valid(y):
		return
	var at := y.global_position + off
	h.global_position = at
	h.velocity = y.velocity
	h.body.pos = at
	h.body.vel = y.velocity
	h.rotation = off.angle() + PI * 0.5
	h.invulnerable = 1.0


func cleanup() -> void:
	for s in spawned:
		if is_instance_valid(s) and s.has_meta("lantern"):
			story.remember(s)          # the ship, purse and rank go on to the next operation
	for s in spawned:
		if is_instance_valid(s) and world.ships.has(s) and not s.is_prize:
			world.remove_ship(s)
	spawned.clear()
	for wp in waypoints:
		if is_instance_valid(wp):
			wp.queue_free()
	waypoints.clear()


## A point a ship's autopilot can fly to (it wants a body: a radius, a
## velocity, a name), riding along with `follow` when given.
class Waypoint extends Node2D:
	var radius := 60.0
	var velocity := Vector2.ZERO
	var body_name := ""
	var look := ""
	var follow: Node2D = null
	var offset := Vector2.ZERO    # from `follow`

	func _physics_process(_delta: float) -> void:
		if follow != null and is_instance_valid(follow):
			global_position = follow.global_position + offset
			velocity = follow.velocity


var waypoints: Array = []


func waypoint(label: String, follow: Node2D, radius: float = 60.0, offset: Vector2 = Vector2.ZERO) -> Waypoint:
	var wp := Waypoint.new()
	wp.body_name = label
	wp.follow = follow
	wp.radius = radius
	wp.offset = offset
	world.add_child(wp)
	wp.global_position = follow.global_position + offset
	wp.velocity = follow.velocity
	waypoints.append(wp)
	return wp
