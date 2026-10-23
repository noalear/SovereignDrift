## NetClient -- the client's view of the host's world: prediction and
## reconciliation for the own ship, interpolation buffers for every other
## ship.  Owned by Net; nothing here talks to the network itself.
class_name NetClient
extends RefCounted

const TICK := 1.0 / 60.0
const INTERP_DELAY := 0.1          # seconds behind the newest snapshot
const MAX_EXTRAPOLATE := 0.25      # past the newest snapshot, keep flying this long
const HISTORY := 120               # inputs kept for replay (2 s)
const SNAP_DISTANCE := 260.0       # own ship: snap beyond this, blend inside
const BUFFER := 10                 # snapshots kept per puppet

var history: Array = []            # [{seq, inputs, aim}] newest last
var buffers := {}                  # net_id -> Array of {t, pos, vel, rot, warping}
var render_time := -1.0            # host clock we draw the puppets at
var newest_time := 0.0
var correction_px := 0.0           # last reconciliation error, for debugging


func record_input(seq: int, inputs: Dictionary, aim: Vector2) -> void:
	history.append({"seq": seq, "inputs": inputs.duplicate(), "aim": aim})
	if history.size() > HISTORY:
		history.pop_front()


## The host's state of the own ship for input `ack_seq`: rewind and replay
## the inputs it has not seen.  `_fly` is the ship's own flight step.
func reconcile(ship: Node, ack_seq: int, pos: Vector2, vel: Vector2, rot: float) -> void:
	var before: Vector2 = ship.global_position
	if ship.warping:
		# a warp is the host's: no replay, just follow
		ship.global_position = pos
		ship.velocity = vel
		ship.rotation = rot
		history.clear()
		return
	var saved_inputs: Dictionary = ship.inputs.duplicate()
	var saved_aim: Vector2 = ship.aim_point
	var saved_energy: float = ship.energy
	ship.global_position = pos
	ship.velocity = vel
	ship.rotation = rot
	var keep := []
	for h in history:
		if h["seq"] <= ack_seq:
			continue
		keep.append(h)
		for k in h["inputs"]:
			ship.inputs[k] = h["inputs"][k]
		ship.aim_point = h["aim"]
		ship._fly(TICK)
	history = keep
	for k in saved_inputs:
		ship.inputs[k] = saved_inputs[k]
	ship.aim_point = saved_aim
	ship.energy = saved_energy            # energy is the host's gauge, set elsewhere
	var err := before.distance_to(ship.global_position)
	correction_px = err
	if err < SNAP_DISTANCE and err > 0.5:
		# small disagreements are eased out instead of jumping
		ship.global_position = ship.global_position.lerp(before, 0.6)


func push_state(net_id: int, t: float, pos: Vector2, vel: Vector2, rot: float, warping: bool) -> void:
	var buf: Array = buffers.get(net_id, [])
	buf.append({"t": t, "pos": pos, "vel": vel, "rot": rot, "warping": warping})
	while buf.size() > BUFFER:
		buf.pop_front()
	buffers[net_id] = buf
	newest_time = maxf(newest_time, t)
	if render_time < 0.0:
		render_time = t - INTERP_DELAY


func forget(net_id: int) -> void:
	buffers.erase(net_id)


## Advance the render clock and place every puppet on its buffer.
func tick(delta: float, ships_by_id: Dictionary) -> void:
	if render_time < 0.0:
		return
	render_time += delta
	# keep the clock INTERP_DELAY behind the newest snapshot, nudging softly
	var want := newest_time - INTERP_DELAY
	var off := want - render_time
	if absf(off) > 0.5:
		render_time = want
	else:
		render_time += off * 0.1
	for id in buffers:
		var s: Node = ships_by_id.get(id, null)
		if s == null or not is_instance_valid(s) or s.dead:
			continue
		var buf: Array = buffers[id]
		if buf.is_empty():
			continue
		var newest: Dictionary = buf[buf.size() - 1]
		if render_time >= newest["t"] or newest["warping"]:
			# nothing newer yet: dead-reckon from the last state
			var age: float = clampf(render_time - newest["t"], 0.0, MAX_EXTRAPOLATE)
			if newest["warping"]:
				age = clampf(newest_time - newest["t"] + INTERP_DELAY, 0.0, 0.5)
			s.global_position = newest["pos"] + newest["vel"] * age
			s.velocity = newest["vel"]
			s.rotation = newest["rot"]
			continue
		var a: Dictionary = buf[0]
		var b: Dictionary = newest
		for i in range(buf.size() - 1):
			if buf[i]["t"] <= render_time and buf[i + 1]["t"] >= render_time:
				a = buf[i]
				b = buf[i + 1]
				break
		var span: float = maxf(b["t"] - a["t"], 0.0001)
		var f: float = clampf((render_time - a["t"]) / span, 0.0, 1.0)
		s.global_position = a["pos"].lerp(b["pos"], f)
		s.velocity = a["vel"].lerp(b["vel"], f)
		s.rotation = lerp_angle(a["rot"], b["rot"], f)
