## StoryOp -- one operation of the story campaign.  An operation script
## extends this: setup() brings in its ships and first objectives, step()
## watches the war and moves the story on, and win() or fail() ends it.
## Story runs it on the host.
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


## [world position, label, colour] for each open objective with a mark.
func marks() -> Array:
	var out := []
	for o in objectives:
		if o["done"] or o["failed"] or o["mark"] == null:
			continue
		var at: Variant = o["mark"]
		if at is Callable:
			at = at.call()
		if at is Node2D:
			if not is_instance_valid(at) or (at is Ship and at.dead):
				continue
			at = at.global_position
		if at is Vector2:
			out.append([at, o["label"] if o["label"] != "" else o["text"], Color(0.6, 1.0, 0.65) if o["optional"] else Color(1.0, 0.85, 0.35)])
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


# ------------------------------------------------------------ ships

## A ship for this operation, flown by a computer pilot of `rank` (-1:
## nobody at the helm).  It never relaunches, and leaves when the
## operation ends.
func spawn(pilot: String, race: int, design: String, ship_name: String, pos: Vector2, vel: Vector2, rank: int = -1) -> Ship:
	var s: Ship = world.spawn_ship(pilot, race, design, ship_name, pos, vel, rank)
	s.set_meta("story", true)
	spawned.append(s)
	var ai: AIPilot = s.get_node_or_null("AI")
	if ai != null:
		# the operation gives the orders: no errands of its own
		ai.ferry = false
		ai.bombard = false
		ai.raid = false
		ai.escort_duty = false
	return s


func alive(s: Ship) -> bool:
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


func cleanup() -> void:
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

	func _physics_process(_delta: float) -> void:
		if follow != null and is_instance_valid(follow):
			global_position = follow.global_position
			velocity = follow.velocity


var waypoints: Array = []


func waypoint(label: String, follow: Node2D, radius: float = 60.0) -> Waypoint:
	var wp := Waypoint.new()
	wp.body_name = label
	wp.follow = follow
	wp.radius = radius
	world.add_child(wp)
	wp.global_position = follow.global_position
	wp.velocity = follow.velocity
	waypoints.append(wp)
	return wp
