## Net -- the online session.  One player's game is the host: it runs the
## whole simulation (AI, collisions, damage, capture) and sends world
## snapshots; every other player sends inputs and shows what the host says.
## Offline the role is OFFLINE and nothing here runs, so solo play is
## untouched.  Prototype: see the design doc for what phase 1 adds
## (reconciliation replay, interpolation buffers, lobby, LAN discovery).
extends Node

signal status(text: String)
signal session_started()
signal session_ended(reason: String)

const PORT := 7777
const PROTOCOL := 1
const MAX_PLAYERS := 8
const SNAPSHOT_INTERVAL := 1.0 / 20.0
const SNAP_DISTANCE := 260.0      # own ship: snap to the host beyond this, blend inside

enum Role { OFFLINE, HOST, CLIENT }

var role := Role.OFFLINE
var world: Node = null
var ships_by_id := {}            # net_id -> Ship
var next_net_id := 1
var seats := {}                  # peer_id -> net_id (host)
var pilot_names := {}            # peer_id -> callsign (host)
var remote_inputs := {}          # peer_id -> last decoded input (host)
var last_seq_from := {}          # peer_id -> newest input seq applied (host)
var local_net_id := 0            # the client's own ship
var input_seq := 0
var snapshot_timer := 0.0
var puppet_targets := {}         # net_id -> {pos, vel, rot, at}
var snapshots_received := 0
var inputs_received := 0
var host_elapsed := 0.0
var callsign := "Pilot"


func is_active() -> bool:
	return role != Role.OFFLINE


func is_host() -> bool:
	return role == Role.HOST


func is_client() -> bool:
	return role == Role.CLIENT


# ------------------------------------------------------------- session

func host(port: int = PORT) -> Error:
	leave()
	var peer := ENetMultiplayerPeer.new()
	var err := peer.create_server(port, MAX_PLAYERS - 1)
	if err != OK:
		status.emit("Could not open port %d (%s)." % [port, error_string(err)])
		return err
	peer.host.compress(ENetConnection.COMPRESS_RANGE_CODER)
	multiplayer.multiplayer_peer = peer
	role = Role.HOST
	multiplayer.peer_connected.connect(_on_peer_connected)
	multiplayer.peer_disconnected.connect(_on_peer_disconnected)
	status.emit("Hosting on port %d." % port)
	session_started.emit()
	return OK


func join(ip: String, port: int = PORT) -> Error:
	leave()
	var peer := ENetMultiplayerPeer.new()
	var err := peer.create_client(ip, port)
	if err != OK:
		status.emit("Could not connect to %s:%d (%s)." % [ip, port, error_string(err)])
		return err
	peer.host.compress(ENetConnection.COMPRESS_RANGE_CODER)
	multiplayer.multiplayer_peer = peer
	role = Role.CLIENT
	multiplayer.connected_to_server.connect(_on_connected)
	multiplayer.connection_failed.connect(_on_connection_failed)
	multiplayer.server_disconnected.connect(_on_server_disconnected)
	status.emit("Connecting to %s:%d..." % [ip, port])
	return OK


func leave() -> void:
	if role == Role.OFFLINE:
		return
	for sig in [multiplayer.peer_connected, multiplayer.peer_disconnected, multiplayer.connected_to_server,
			multiplayer.connection_failed, multiplayer.server_disconnected]:
		for c in sig.get_connections():
			sig.disconnect(c["callable"])
	if multiplayer.multiplayer_peer != null and not multiplayer.multiplayer_peer is OfflineMultiplayerPeer:
		multiplayer.multiplayer_peer.close()
	multiplayer.multiplayer_peer = OfflineMultiplayerPeer.new()
	role = Role.OFFLINE
	world = null
	ships_by_id.clear()
	seats.clear()
	pilot_names.clear()
	remote_inputs.clear()
	last_seq_from.clear()
	puppet_targets.clear()
	local_net_id = 0
	next_net_id = 1
	snapshots_received = 0
	inputs_received = 0


func _on_connected() -> void:
	status.emit("Connected. Waiting for the host...")
	session_started.emit()


func _on_connection_failed() -> void:
	var r := "Connection failed."
	leave()
	session_ended.emit(r)


func _on_server_disconnected() -> void:
	leave()
	session_ended.emit("The host left the game.")


# ---------------------------------------------------------- host side

## Called by World once the system and the pilots exist on the host.
func host_attach(p_world: Node) -> void:
	world = p_world
	for s in world.ships:
		_register(s)


func _register(s: Node) -> void:
	if s.net_id == 0:
		s.net_id = next_net_id
		next_net_id += 1
	ships_by_id[s.net_id] = s


func _on_peer_connected(peer_id: int) -> void:
	status.emit("Player %d connected." % peer_id)


func _on_peer_disconnected(peer_id: int) -> void:
	var net_id: int = seats.get(peer_id, 0)
	seats.erase(peer_id)
	remote_inputs.erase(peer_id)
	last_seq_from.erase(peer_id)
	var name: String = pilot_names.get(peer_id, "A player")
	pilot_names.erase(peer_id)
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s):
		_seat_ai(s, true)
		for k in s.inputs.keys():
			s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
		s.aim_point = Vector2.INF
		_rpc_seat_changed.rpc(net_id, s.pilot_name, false)
	if world != null:
		world.notify_all("%s left the game." % name, Color(0.75, 0.75, 0.75))


func _seat_ai(s: Node, on: bool) -> void:
	for c in s.get_children():
		if c is AIPilot:
			c.set_physics_process(on)
			c.set_process(on)


## A joiner takes over a computer pilot of the host's empire (co-op).
func _seat_player(peer_id: int, name: String) -> Node:
	var best: Node = null
	for s in world.ships:
		if s.is_player or s.is_station or s.net_id in seats.values():
			continue
		if s.race_id != world.player.race_id:
			continue
		if best == null or (best.dead and not s.dead):
			best = s
	if best == null:
		return null
	seats[peer_id] = best.net_id
	pilot_names[peer_id] = name
	_seat_ai(best, false)
	best.pilot_name = name
	best.ship_name = name + "'s ship"
	best.target = null
	best.autopilot = false
	return best


func _roster() -> Array:
	var out := []
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if not is_instance_valid(s):
			continue
		out.append([s.net_id, s.pilot_name, s.race_id, s.design_name, s.ship_name, s.score, s.kills, s.rank, s.dead,
			s.global_position, s.velocity, s.rotation])
	return out


@rpc("any_peer", "reliable")
func _rpc_hello(name: String, protocol: int) -> void:
	if not is_host() or world == null:
		return
	var peer_id := multiplayer.get_remote_sender_id()
	if protocol != PROTOCOL:
		_rpc_refused.rpc_id(peer_id, "Different game version (yours %d, host %d)." % [protocol, PROTOCOL])
		return
	var s := _seat_player(peer_id, name)
	if s == null:
		_rpc_refused.rpc_id(peer_id, "No free seat in this round.")
		return
	_rpc_welcome.rpc_id(peer_id, s.net_id, world.elapsed, _roster())
	_rpc_seat_changed.rpc(s.net_id, name, true)
	world.notify_all("%s joined the game as %s." % [name, s.design_name], Color(0.7, 1.0, 0.7))


@rpc("any_peer", "unreliable_ordered", "call_remote", 1)
func _rpc_input(data: PackedByteArray) -> void:
	if not is_host():
		return
	var peer_id := multiplayer.get_remote_sender_id()
	if not seats.has(peer_id):
		return
	var d := NetCodec.unpack_input(data)
	if d["seq"] <= last_seq_from.get(peer_id, -1):
		return
	last_seq_from[peer_id] = d["seq"]
	remote_inputs[peer_id] = d
	inputs_received += 1


func _apply_remote_inputs() -> void:
	for peer_id in remote_inputs:
		var s: Node = ships_by_id.get(seats.get(peer_id, 0), null)
		if s == null or not is_instance_valid(s) or s.dead:
			continue
		var d: Dictionary = remote_inputs[peer_id]
		for k in d["inputs"]:
			s.inputs[k] = d["inputs"][k]
		if d["aim_angle"] == INF:
			s.aim_point = Vector2.INF
		else:
			s.aim_point = s.global_position + Vector2.RIGHT.rotated(d["aim_angle"]) * 1000.0


func _send_snapshots() -> void:
	var list := []
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if is_instance_valid(s):
			list.append(s)
	for peer_id in seats:
		var data := NetCodec.pack_snapshot(world.elapsed, last_seq_from.get(peer_id, 0), list)
		_rpc_snapshot.rpc_id(peer_id, data)


## World hooks on the host: shots, explosions, deaths, respawns, messages.
func host_shot(m: Node, owner: Node, missile_id: String, pos: Vector2, vel: Vector2, angle: float) -> void:
	if not is_host() or seats.is_empty():
		return
	var mid := next_net_id
	next_net_id += 1
	m.set_meta("net_id", mid)
	var owner_id: int = owner.net_id if owner is Ship else 0
	var race: int = owner.race_id if owner is Ship else owner.owner_race
	_rpc_shot.rpc(mid, owner_id, race, missile_id, pos, vel, angle)


func host_boom(m: Node, pos: Vector2, splash: float) -> void:
	if not is_host() or seats.is_empty() or not m.has_meta("net_id"):
		return
	_rpc_boom.rpc(m.get_meta("net_id"), pos, splash)


func host_ship_died(s: Node, killer: Node) -> void:
	if not is_host() or seats.is_empty():
		return
	var kid: int = killer.net_id if killer is Ship else 0
	_rpc_ship_died.rpc(s.net_id, kid)


func host_ship_respawned(s: Node) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_ship_respawned.rpc(s.net_id, s.global_position, s.velocity, s.rotation)


func relay_notify(s: Node, text: String) -> void:
	if not is_host() or s == null:
		return
	for peer_id in seats:
		if seats[peer_id] == s.net_id:
			_rpc_message.rpc_id(peer_id, text, Color(0.85, 0.9, 1.0))


func relay_notify_all(text: String, color: Color) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_message.rpc(text, color)


# -------------------------------------------------------- client side

## Called by World on a client: the system exists, the ships come from the host.
func client_attach(p_world: Node) -> void:
	world = p_world
	_rpc_hello.rpc_id(1, callsign, PROTOCOL)


@rpc("authority", "reliable")
func _rpc_refused(reason: String) -> void:
	leave()
	session_ended.emit(reason)


@rpc("authority", "reliable")
func _rpc_welcome(my_id: int, elapsed: float, roster: Array) -> void:
	if world == null:
		return
	local_net_id = my_id
	host_elapsed = elapsed
	world.elapsed = elapsed
	for row in roster:
		var s: Node = world._make_ship(row[1], row[2], row[3], row[4], row[0] == my_id)
		s.net_id = row[0]
		s.score = row[5]
		s.kills = row[6]
		s.rank = row[7]
		s.global_position = row[9]
		s.velocity = row[10]
		s.rotation = row[11]
		if row[8]:
			s._die(null)
		ships_by_id[s.net_id] = s
		if row[0] == my_id:
			s.is_puppet = false
			s.pilot_name = callsign
			world.player = s
			var pc := PlayerController.new()
			pc.name = "Controller"
			s.add_child(pc)
			pc.setup(s, world)
		else:
			s.is_puppet = true
	status.emit("In the game as %s." % world.player.design_name)
	world.notify_all("Joined the host's game.", Color(0.7, 1.0, 0.7))


@rpc("authority", "reliable")
func _rpc_seat_changed(net_id: int, name: String, human: bool) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s):
		s.pilot_name = name
	if world != null and net_id != local_net_id:
		world.notify_all("%s %s." % [name, "joined" if human else "is flown by the computer again"], Color(0.7, 1.0, 0.7))


@rpc("authority", "unreliable_ordered", "call_remote", 2)
func _rpc_snapshot(data: PackedByteArray) -> void:
	if world == null:
		return
	snapshots_received += 1
	var snap := NetCodec.unpack_snapshot(data)
	# keep the planets on the host's clock
	var drift: float = snap["elapsed"] - world.elapsed
	if absf(drift) > 0.02:
		for p in world.planets:
			p.update_orbit(drift)
		world.elapsed = snap["elapsed"]
	host_elapsed = snap["elapsed"]
	var now := Time.get_ticks_msec() / 1000.0
	for e in snap["ships"]:
		var s: Node = ships_by_id.get(e["net_id"], null)
		if s == null or not is_instance_valid(s):
			continue
		var was_dead: bool = s.dead
		var dead: bool = (e["flags"] & NetCodec.FLAG_DEAD) != 0
		s.hull = e["hull"] * s.hull_max
		s.shield = e["shield"] * s.shield_max
		s.energy = e["energy"] * s.energy_max
		if e["net_id"] == local_net_id:
			# prediction: the own ship flies locally, the host pulls it back
			var err: float = s.global_position.distance_to(e["pos"])
			if err > SNAP_DISTANCE:
				s.global_position = e["pos"]
				s.velocity = e["vel"]
			else:
				s.global_position = s.global_position.lerp(e["pos"], 0.25)
				s.velocity = s.velocity.lerp(e["vel"], 0.25)
			if dead and not was_dead:
				s._die(null)
			continue
		puppet_targets[e["net_id"]] = {"pos": e["pos"], "vel": e["vel"], "rot": e["rot"], "at": now}
		s.thrust_level = e["thrust"]
		s.cloaked = (e["flags"] & NetCodec.FLAG_CLOAKED) != 0
		s.warping = (e["flags"] & NetCodec.FLAG_WARPING) != 0
		if dead and not was_dead:
			s._die(null)


@rpc("authority", "reliable")
func _rpc_shot(mid: int, owner_id: int, race: int, missile_id: String, pos: Vector2, vel: Vector2, angle: float) -> void:
	if world == null:
		return
	var owner: Node = ships_by_id.get(owner_id, null)
	if owner != null and is_instance_valid(owner):
		if owner.cloaked:
			owner.cloaked = false
		var snd := "torpedo"
		match missile_id:
			"003", "011": snd = "plasma"
			"010": snd = "rapid"
			"004": snd = "grenade"
			"008", "009": snd = "missile"
			"002": snd = "mine"
			"018": snd = "cannon"
		Sfx.play(snd, pos, 0.7, randf_range(0.95, 1.05))
	else:
		owner = null
	world.spawn_missile(owner if owner != null else _race_stub(race), missile_id, pos, vel, angle, null)
	world.missiles[world.missiles.size() - 1].set_meta("net_id", mid)


var _stubs := {}
func _race_stub(race: int) -> Node:
	# a stand-in shooter for shots whose owner the client does not know
	if not _stubs.has(race):
		var n := Node.new()
		n.set_script(load("res://src/net/ShotOwner.gd"))
		n.owner_race = race
		add_child(n)
		_stubs[race] = n
	return _stubs[race]


@rpc("authority", "reliable")
func _rpc_boom(mid: int, pos: Vector2, splash: float) -> void:
	if world == null:
		return
	for m in world.missiles:
		if is_instance_valid(m) and m.has_meta("net_id") and m.get_meta("net_id") == mid:
			m.exploded = true
			world.fx.explosion(pos, (splash * 0.5) if splash > 0.0 else (6.0 + m.radius * 2.0), m.color, m.missile_id == "002")
			m.queue_free()
			return


@rpc("authority", "reliable")
func _rpc_ship_died(net_id: int, killer_id: int) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s) and not s.dead:
		s._die(ships_by_id.get(killer_id, null))


@rpc("authority", "reliable")
func _rpc_ship_respawned(net_id: int, pos: Vector2, vel: Vector2, rot: float) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s):
		s.respawn(pos, vel)
		s.rotation = rot
		puppet_targets.erase(net_id)
		if net_id == local_net_id and world != null:
			world.player_respawned.emit()


@rpc("authority", "reliable")
func _rpc_message(text: String, color: Color) -> void:
	if world != null:
		world.message.emit(text, color)


## Runs on the client every physics tick instead of the host-only parts of World.
func client_tick(delta: float) -> void:
	if world == null or local_net_id == 0:
		return
	var p: Node = world.player
	if p != null and not p.dead:
		input_seq += 1
		_rpc_input.rpc_id(1, NetCodec.pack_input(input_seq, p.inputs, p.aim_point, p.global_position))
	# puppets: dead-reckon from the newest snapshot and ease toward it
	var now := Time.get_ticks_msec() / 1000.0
	for id in puppet_targets:
		var s: Node = ships_by_id.get(id, null)
		if s == null or not is_instance_valid(s) or s.dead:
			continue
		var t: Dictionary = puppet_targets[id]
		var age: float = clampf(now - t["at"], 0.0, 0.25)
		var want: Vector2 = t["pos"] + t["vel"] * age
		if s.global_position.distance_to(want) > SNAP_DISTANCE * 3.0:
			s.global_position = want
		else:
			s.global_position = s.global_position.lerp(want, minf(12.0 * delta, 1.0))
		s.velocity = t["vel"]
		s.rotation = lerp_angle(s.rotation, t["rot"], minf(14.0 * delta, 1.0))


# ------------------------------------------------------------- per tick

func _physics_process(delta: float) -> void:
	if not is_host() or world == null:
		return
	_apply_remote_inputs()
	snapshot_timer -= delta
	if snapshot_timer <= 0.0 and not seats.is_empty():
		snapshot_timer += SNAPSHOT_INTERVAL
		_send_snapshots()
