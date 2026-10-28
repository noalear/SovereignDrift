## Net -- the online session.  One player's game is the host: it runs the
## whole simulation (AI, collisions, damage, capture) and sends world
## snapshots; every other player sends inputs and shows what the host says.
## Offline the role is OFFLINE and nothing here runs, so solo play is
## untouched.  Prediction and interpolation live in NetClient, LAN
## announcements in LanDiscovery.
extends Node

signal status(text: String)
signal session_started()
signal session_ended(reason: String)
signal round_restarted()       # the host started the next Conquest round (client)

const PORT := 7777
const PROTOCOL := 19
const MAX_PLAYERS := 8
const SNAPSHOT_INTERVAL := 1.0 / 20.0
const PING_EVERY := 1.0
const STATS_EVERY := 2.0        # scores, ranks and the round clock
const SYSTEMS_EVERY := 0.25     # damaged systems, crews and boardings (changes only)
const SYSTEMS_FULL_EVERY := 8   # ...and everything every this many sends

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
var ping_timer := 0.0
var stats_timer := 0.0
var systems_timer := 0.0
var systems_sends := 0
var _systems_sent := {}          # net_id -> the bytes last sent (host)
var ping_ms := 0.0
var snapshots_received := 0
var inputs_received := 0
var host_elapsed := 0.0
var callsign := "Pilot"
var mode := ""                   # "duel" for a free-for-all, "" for co-op (set before host())
var client: NetClient = null
var lan: LanDiscovery = null
var powerups_by_id := {}         # net_id -> Powerup


func _ready() -> void:
	lan = LanDiscovery.new()
	lan.name = "Lan"
	add_child(lan)


func is_active() -> bool:
	return role != Role.OFFLINE


func is_host() -> bool:
	return role == Role.HOST


func is_client() -> bool:
	return role == Role.CLIENT


func player_count() -> int:
	return 1 + seats.size() if is_host() else 0


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
	lan.announce(_announcement(port))
	status.emit("Hosting on port %d." % port)
	session_started.emit()
	return OK


func _announcement(port: int) -> Dictionary:
	return {"name": callsign + ("'s duel" if mode == "duel" else "'s game"), "players": player_count(), "max": MAX_PLAYERS, "port": port,
		"protocol": PROTOCOL, "system": "Sol", "mode": mode}


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
	client = NetClient.new()
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
	if lan.announcing:
		lan.stop()
	role = Role.OFFLINE
	mode = ""
	world = null
	client = null
	ships_by_id.clear()
	powerups_by_id.clear()
	seats.clear()
	pilot_names.clear()
	remote_inputs.clear()
	last_seq_from.clear()
	local_net_id = 0
	next_net_id = 1
	snapshots_received = 0
	inputs_received = 0
	ping_ms = 0.0


func _on_connected() -> void:
	status.emit("Connected. Waiting for the host...")
	session_started.emit()


func _on_connection_failed() -> void:
	leave()
	session_ended.emit("Could not reach the host. Check the address, and that port %d is open on their router." % PORT)


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
	var t := _trade_of(net_id)
	if not t.is_empty():
		_close_trade(t, "The trade ended: a pilot left.")
	seats.erase(peer_id)
	remote_inputs.erase(peer_id)
	last_seq_from.erase(peer_id)
	var name: String = pilot_names.get(peer_id, "A player")
	pilot_names.erase(peer_id)
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s) and world != null and world.duel_rules != null:
		world.duel_rules.pilot_left(s)      # their ship goes; a computer pilot refills the arena
	elif s != null and is_instance_valid(s):
		_seat_ai(s, true)
		for k in s.inputs.keys():
			s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
		s.aim_point = Vector2.INF
		_rpc_seat_changed.rpc(net_id, s.pilot_name, false)
	if world != null:
		world.notify_all("%s left the game." % name, Color(0.75, 0.75, 0.75))
	lan.announce(_announcement(PORT))


func _seat_ai(s: Node, on: bool) -> void:
	if "group_fire" in s:
		s.group_fire = not on        # a seated human fires by weapon group
	for c in s.get_children():
		if c is AIPilot:
			c.set_physics_process(on)
			c.set_process(on)


## A joiner takes over a computer pilot of the host's empire (co-op), a
## living one when there is a choice.
## The joiner brings their career score and kills, so their rank and
## the career they save at the end are their own, not the AI's.
func _seat_player(peer_id: int, name: String, career_score: int = 0, career_kills: int = 0) -> Node:
	var best: Node = null
	for s in world.ships:
		if s.is_player or s.is_station or s.is_prize or s.support or s.net_id in seats.values():
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
	if best.is_landing():
		best.leave_ground_now()        # a joiner starts in space, not parked on a world
		host_landing(best)
	best.pilot_name = name
	best.ship_name = name + "'s ship"
	best.score = maxi(career_score, 0)
	best.kills = maxi(career_kills, 0)
	best.rank = GameData.rank_for(best.score, best.kills)
	best.target = null
	best.autopilot = false
	for k in best.inputs.keys():
		best.inputs[k] = false if typeof(best.inputs[k]) == TYPE_BOOL else 0.0
	return best


func _roster() -> Array:
	var out := []
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if not is_instance_valid(s):
			continue
		out.append(_roster_row(s))
	return out


func _roster_row(s: Node) -> Array:
	return [s.net_id, s.pilot_name, s.race_id, s.design_name, s.ship_name, s.score, s.kills, s.rank, s.dead,
		s.global_position, s.velocity, s.rotation, s.design, s.hull_race, s.is_prize, s.support,
		s.building, world.planets.find(s.station_body) if s.station_body != null else -1]


## The round as the HUD and the map show it.
func _round_state() -> Array:
	var rd = world.round_rules
	if rd == null:
		return []
	return [rd.length, rd.phase, rd.elapsed, rd.hold_race, rd.hold_timer, rd.over, rd.kills, rd.captures]


## Every ship's career numbers and what it did this round.
func _stats() -> Array:
	var rd = world.round_rules
	var out := []
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if not is_instance_valid(s):
			continue
		out.append([s.net_id, s.score, s.kills, s.rank, s.captures, s.deaths,
			(rd.round_score(s) if rd != null else s.score), (rd.round_kills(s) if rd != null else s.kills)])
	return out


func _planet_owners() -> PackedInt32Array:
	var out := PackedInt32Array()
	for p in world.planets:
		out.append(p.owner_race)
	return out


func _powerup_list() -> Array:
	var out := []
	for id in powerups_by_id:
		var p: Node = powerups_by_id[id]
		if is_instance_valid(p):
			out.append([id, p.spawn_id, p.global_position, p.velocity, p.age])
	return out


@rpc("any_peer", "reliable")
func _rpc_hello(name: String, protocol: int, career_score: int = 0, career_kills: int = 0, race: int = 0) -> void:
	if not is_host() or world == null:
		return
	var peer_id := multiplayer.get_remote_sender_id()
	if protocol != PROTOCOL:
		_rpc_refused.rpc_id(peer_id, "Different game version (yours %d, host %d)." % [protocol, PROTOCOL])
		return
	var s := _seat_duelist(peer_id, name, race) if world.duel_rules != null else _seat_player(peer_id, name, career_score, career_kills)
	if s == null:
		_rpc_refused.rpc_id(peer_id, "No free seat in this round.")
		return
	if world.duel_rules != null:
		_rpc_duel.rpc_id(peer_id, world.duel_rules.pack())   # before the welcome: the client is in a duel from its first frame
	_rpc_welcome.rpc_id(peer_id, s.net_id, world.elapsed, _roster(), _planet_owners(), _powerup_list())
	_rpc_stats.rpc_id(peer_id, _stats(), _round_state())
	if world.round_rules != null and world.round_rules.over:
		# the round is decided: the joiner sees the scoreboard and waits for the next
		_rpc_round_over.rpc_id(peer_id, world.round_rules.result, false)
	_rpc_seat_changed.rpc(s.net_id, name, true)
	world.notify_all("%s joined the game as %s." % [name, s.design_name], Color(0.7, 1.0, 0.7))
	lan.announce(_announcement(PORT))


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


## A pilot's build from the shipyard.  Friends' co-op: the host checks the
## design is buildable and the purse is not negative, and fits it.
@rpc("any_peer", "reliable")
func _rpc_build(design: Dictionary, credits: int, hold: Array, blueprints: Array) -> void:
	if not is_host():
		return
	var s: Node = ships_by_id.get(seats.get(multiplayer.get_remote_sender_id(), 0), null)
	if s == null or not is_instance_valid(s) or s.dead:
		return
	s.docked = false
	if credits < 0 or not design.has("name") or not design.has("weapons") or not design.has("modules"):
		world.notify(s, "The shipyard refused that build.")
		return
	var why: String = world.build_refusal(s, design)
	if why != "":
		world.notify(s, why)
		_rpc_purse.rpc_id(multiplayer.get_remote_sender_id(), s.credits, s.hold, s.blueprints)
		return
	s.credits = credits
	s.hold = hold
	s.blueprints = blueprints
	world.apply_build(s, design)


@rpc("any_peer", "unreliable")
func _rpc_ping(sent_ms: int) -> void:
	if is_host():
		_rpc_pong.rpc_id(multiplayer.get_remote_sender_id(), sent_ms)


@rpc("authority", "unreliable")
func _rpc_pong(sent_ms: int) -> void:
	ping_ms = lerpf(ping_ms, float(Time.get_ticks_msec() - sent_ms), 0.5 if ping_ms > 0.0 else 1.0)


## A client asks for something only the host may do: a warp, the jammer, a refit.
@rpc("any_peer", "reliable")
func _rpc_command(cmd: String) -> void:
	if not is_host():
		return
	var s: Node = ships_by_id.get(seats.get(multiplayer.get_remote_sender_id(), 0), null)
	if s == null or not is_instance_valid(s) or s.dead:
		return
	match cmd:
		"warp":
			if s.warping:
				s.end_warp()
			else:
				s.start_warp()
		_ when cmd.begins_with("jam:"):
			# the client's target travels with the command; targets are local
			var jt: Node = ships_by_id.get(int(cmd.get_slice(":", 1)), null)
			if s.jamming or jt == null:
				s.set_jam(false)
			else:
				s.target = jt
				s.set_jam(true)
		"ping":
			s.ping()
		"silent":
			s.set_silent(not s.sensors.emcon)
		"refit":
			var base: Node = world.nearest_base(s)
			if base == null:
				world.notify(s, "The shipyard is at your starbase (fly within docking range and press U).")
			elif s.speed() > 220.0:
				world.notify(s, "Too fast to dock: brake first (X).")
			else:
				# the pilot docks: controls off, and their purse goes with the invitation
				s.docked = true
				for k in s.inputs.keys():
					s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
				s.inputs["brake"] = true
				_rpc_open_shipyard.rpc_id(multiplayer.get_remote_sender_id(), base.net_id, s.credits, s.hold, s.blueprints)
		"undock":
			s.docked = false
		_ when cmd.begins_with("board:"):
			# the client's target travels with the command; targets are local
			world.capture.try_board(s, ships_by_id.get(int(cmd.get_slice(":", 1)), null))
		"land":
			world.try_land(s)
		"colonists_board":
			world.colonist_command(s, true)
		"colonists_unload":
			world.colonist_command(s, false)
		_ when cmd.begins_with("cargo:"):
			world.cargo_command(s, cmd.get_slice(":", 1), float(cmd.get_slice(":", 2)))
		_ when cmd.begins_with("kit:"):
			world.kit_command(s, cmd.get_slice(":", 1))
		"deploy":
			world.deploy_kit(s)
		_ when cmd.begins_with("order:"):
			world.order_command(s, cmd.get_slice(":", 1))
		_ when cmd.begins_with("build:"):
			world.build_command(s, cmd.get_slice(":", 1))
		_ when cmd.begins_with("recruit:"):
			world.recruit_command(s, cmd.get_slice(":", 1))
		_ when cmd.begins_with("troops:"):
			world.troop_command(s, cmd.get_slice(":", 1), cmd.get_slice(":", 2))
		_ when cmd.begins_with("job:"):
			world.job_command(s, int(cmd.get_slice(":", 1)))


func _apply_remote_inputs() -> void:
	for peer_id in remote_inputs:
		var s: Node = ships_by_id.get(seats.get(peer_id, 0), null)
		if s == null or not is_instance_valid(s) or s.dead or s.docked:
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


## World hooks on the host: shots, explosions, deaths, respawns, pickups,
## captures and messages.  All no-ops when nobody has joined.
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
	_rpc_ship_died.rpc(s.net_id, kid, s.captured)


func host_ship_respawned(s: Node) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_ship_respawned.rpc(s.net_id, s.global_position, s.velocity, s.rotation)


func host_powerup_spawned(p: Node) -> void:
	if not is_host():
		return
	var pid := next_net_id
	next_net_id += 1
	p.set_meta("net_id", pid)
	powerups_by_id[pid] = p
	if not seats.is_empty():
		_rpc_powerup.rpc(pid, p.spawn_id, p.global_position, p.velocity)


func host_powerup_taken(p: Node, by: Node) -> void:
	if not is_host() or not p.has_meta("net_id"):
		return
	var pid: int = p.get_meta("net_id")
	powerups_by_id.erase(pid)
	if not seats.is_empty():
		_rpc_powerup_taken.rpc(pid, by.net_id)


func host_planet_captured(p: Node, winner: int) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_planet_captured.rpc(world.planets.find(p), winner)


## Landing: a ship started down, touched down or lifted off.
func host_landing(s: Node) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_landing.rpc(s.net_id, s.landing, world.planets.find(s.land_body))


## A colony was founded, invaded, knocked out, wiped out or changed size.
func host_colony_changed(p: Node) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_colonies.rpc(_colony_state())


func _colony_state() -> Array:
	var out := []
	for p in world.planets:
		# with the ground war: buildings, garrison, planetary shield, battle
		out.append([p.owner_race, p.colony_design, p.population, p.silenced, Ground.pack(p)])
	return out


func host_round_over(result: Dictionary) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_stats.rpc(_stats(), _round_state())
	_rpc_round_over.rpc(result)


## The host's Next round: the session stays open, everything tied to the
## old world goes.  Main then builds the new world (host_attach), and
## host_round_restarted sends every player into it.  Net ids keep counting
## up, so a late snapshot of the old world never lands on a new ship.
func host_clear_round() -> void:
	if not is_host():
		return
	world = null
	ships_by_id.clear()
	powerups_by_id.clear()
	seats.clear()
	pilot_names.clear()
	remote_inputs.clear()
	last_seq_from.clear()
	_systems_sent.clear()
	trades.clear()
	snapshot_timer = 0.0
	stats_timer = 0.0


func host_round_restarted() -> void:
	if not is_host() or world == null:
		return
	_rpc_new_round.rpc()
	lan.announce(_announcement(PORT))


func host_refit(s: Node) -> void:
	if not is_host() or seats.is_empty():
		return
	_rpc_refit.rpc(s.net_id, s.design)


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
	_rpc_hello.rpc_id(1, callsign, PROTOCOL, int(Settings.career.get("score", 0)), int(Settings.career.get("kills", 0)), Settings.race_id)


## PlayerController calls this for actions the host must validate.
func client_command(cmd: String) -> void:
	if is_client():
		_rpc_command.rpc_id(1, cmd)


## The Builder's launch on a client: the host fits the build.
func client_build(s: Node, design: Dictionary) -> void:
	if is_client():
		_rpc_build.rpc_id(1, design, s.credits, s.hold, s.blueprints)


@rpc("authority", "reliable")
func _rpc_purse(credits: int, hold: Array, blueprints: Array) -> void:
	var s: Node = ships_by_id.get(local_net_id, null)
	if s == null or not is_instance_valid(s) or s.docked:
		return   # not while the shipyard is open on this purse
	s.credits = credits
	s.hold = hold
	s.blueprints = blueprints


@rpc("authority", "reliable")
func _rpc_open_shipyard(base_id: int, credits: int, hold: Array, blueprints: Array) -> void:
	if world == null:
		return
	var s: Node = ships_by_id.get(local_net_id, null)
	var base: Node = ships_by_id.get(base_id, null)
	if s == null or base == null or not is_instance_valid(s) or not is_instance_valid(base):
		return
	s.credits = credits
	s.hold = hold
	s.blueprints = blueprints
	world.shipyard_requested.emit(s, base)


@rpc("authority", "reliable")
func _rpc_refused(reason: String) -> void:
	leave()
	session_ended.emit(reason)


@rpc("authority", "reliable")
func _rpc_welcome(my_id: int, elapsed: float, roster: Array, owners: PackedInt32Array, pickups: Array) -> void:
	if world == null:
		return
	local_net_id = my_id
	host_elapsed = elapsed
	# planets: the host's clock and the host's owners
	var drift: float = elapsed - world.elapsed
	for p in world.planets:
		p.update_orbit(drift)
	world.elapsed = elapsed
	for i in mini(owners.size(), world.planets.size()):
		var p: Node = world.planets[i]
		if owners[i] != p.owner_race and owners[i] >= 0 and p.is_colonised():
			p.surrender_to(owners[i])
	for row in roster:
		var s: Node = _add_from_row(row, my_id)
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
	for row in pickups:
		_rpc_powerup(row[0], row[1], row[2], row[3])
		var p: Node = powerups_by_id.get(row[0], null)
		if p != null:
			p.age = row[4]
	status.emit("In the game as %s." % world.player.design_name)
	world.notify_all("Joined %s. Press H for the controls." % "the host's game", Color(0.7, 1.0, 0.7))


## A ship from a roster row, as the host sent it.
func _add_from_row(row: Array, my_id: int) -> Node:
	var s: Node = world._make_ship(row[1], row[2], row[3], row[4], row[0] == my_id, row[13] if row.size() > 13 else -1)
	if row.size() > 17:
		s.building = row[16]
		if row[17] >= 0 and row[17] < world.planets.size():
			s.station_body = world.planets[row[17]]
	if row.size() > 12 and row[12] is Dictionary:
		s.refit(row[12])
	s.net_id = row[0]
	s.score = row[5]
	s.kills = row[6]
	s.rank = row[7]
	s.global_position = row[9]
	s.velocity = row[10]
	s.rotation = row[11]
	s.is_prize = row.size() > 14 and row[14]
	s.support = row.size() > 15 and row[15]
	if row[8]:
		s._die(null)
	ships_by_id[s.net_id] = s
	return s


## A ship that joined mid-round: a captured prize.
@rpc("authority", "reliable")
func _rpc_ship_added(row: Array) -> void:
	if world == null or ships_by_id.has(row[0]):
		return
	var s: Node = _add_from_row(row, local_net_id)
	s.is_puppet = true


@rpc("authority", "reliable")
func _rpc_ship_removed(net_id: int) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	ships_by_id.erase(net_id)
	if client != null:
		client.forget(net_id)
	if s != null and is_instance_valid(s) and world != null:
		world.remove_ship(s)


## Damaged systems, crews and boardings, for the HUD and the ship art.
## Per ship: net_id u16, crew u8 (share of crew_max), board target u16,
## board progress u8, system count u8, one byte per system.
@rpc("authority", "unreliable_ordered", "call_remote", 3)
func _rpc_systems(data: PackedByteArray) -> void:
	if world == null:
		return
	var b := StreamPeerBuffer.new()
	b.data_array = data
	var n := b.get_u16()
	for k in n:
		var id := b.get_u16()
		var crew := b.get_u8() / 255.0
		var board_id := b.get_u16()
		var board := b.get_u8() / 255.0
		var count := b.get_u8()
		var bytes := PackedByteArray()
		for i in count:
			bytes.append(b.get_u8())
		var s: Node = ships_by_id.get(id, null)
		if s == null or not is_instance_valid(s):
			continue
		s.crew = crew * s.crew_max
		s.systems.unpack(bytes)
		s.apply_systems()
		var target: Node = ships_by_id.get(board_id, null)
		if s.boarding != null and is_instance_valid(s.boarding) and s.boarding != target:
			s.boarding.boarded_by = null
		s.boarding = target if board_id != 0 else null
		s.board_progress = board
		if s.boarding != null:
			s.boarding.boarded_by = s


func _send_systems(full: bool) -> void:
	var b := StreamPeerBuffer.new()
	var rows := []
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if not is_instance_valid(s) or s.is_station:
			continue
		var bytes: PackedByteArray = s.systems.pack()
		var board_id: int = s.boarding.net_id if s.boarding != null and is_instance_valid(s.boarding) else 0
		var head := PackedByteArray([int(clampf(s.crew_fraction(), 0.0, 1.0) * 255.0), board_id & 255, board_id >> 8, int(s.board_progress * 255.0)])
		var key := head + bytes
		if not full and _systems_sent.get(id, PackedByteArray()) == key:
			continue
		_systems_sent[id] = key
		rows.append([id, head, board_id, bytes])
	if rows.is_empty():
		return
	b.put_u16(rows.size())
	for r in rows:
		b.put_u16(r[0])
		b.put_u8(r[1][0])
		b.put_u16(r[2])
		b.put_u8(r[1][3])
		b.put_u8(r[3].size())
		for x in r[3]:
			b.put_u8(x)
	_rpc_systems.rpc(b.data_array)


func host_ship_added(s: Node) -> void:
	if not is_host():
		return
	_register(s)
	if not seats.is_empty():
		_rpc_ship_added.rpc(_roster_row(s))


func host_ship_removed(s: Node) -> void:
	if not is_host():
		return
	ships_by_id.erase(s.net_id)
	_systems_sent.erase(s.net_id)
	if not seats.is_empty():
		_rpc_ship_removed.rpc(s.net_id)


@rpc("authority", "reliable")
func _rpc_seat_changed(net_id: int, name: String, human: bool) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s):
		s.pilot_name = name
	if world != null and net_id != local_net_id:
		world.notify_all("%s %s." % [name, "joined" if human else "is flown by the computer again"], Color(0.7, 1.0, 0.7))


@rpc("authority", "unreliable_ordered", "call_remote", 2)
func _rpc_snapshot(data: PackedByteArray) -> void:
	if world == null or client == null:
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
			if not s.dead:
				client.reconcile(s, snap["ack_seq"], e["pos"], e["vel"], e["rot"])
			s.jamming = (e["flags"] & NetCodec.FLAG_JAMMING) != 0
			s.sensors.emcon = (e["flags"] & NetCodec.FLAG_SILENT) != 0
			s.sensors.jammed_until = world.elapsed + 0.5 if (e["flags"] & NetCodec.FLAG_JAMMED) != 0 else -1.0
			if s.warping != ((e["flags"] & NetCodec.FLAG_WARPING) != 0):
				s.warping = not s.warping
				s.warp_charge = 0.0 if s.warping else s.warp_charge
			if dead and not was_dead:
				s._die(null)
			continue
		client.push_state(e["net_id"], snap["elapsed"], e["pos"], e["vel"], e["rot"], (e["flags"] & NetCodec.FLAG_WARPING) != 0)
		s.thrust_level = e["thrust"]
		s.jamming = (e["flags"] & NetCodec.FLAG_JAMMING) != 0
		s.sensors.net_sig = e["sig"]
		s.warping = (e["flags"] & NetCodec.FLAG_WARPING) != 0
		if dead and not was_dead:
			s._die(null)


@rpc("authority", "reliable")
func _rpc_shot(mid: int, owner_id: int, race: int, missile_id: String, pos: Vector2, vel: Vector2, angle: float) -> void:
	if world == null:
		return
	var owner: Node = ships_by_id.get(owner_id, null)
	if owner != null and is_instance_valid(owner):
		var snd := "torpedo"
		match missile_id:
			"003": snd = "plasma"
			"011": snd = "battery"
			"010": snd = "rapid"
			"004": snd = "grenade"
			"008", "009": snd = "missile"
			"002": snd = "mine"
			"018": snd = "cannon"
			"022": snd = "rapid"
		Sfx.play(snd, pos, 0.35 if missile_id == "022" else 0.7, randf_range(0.95, 1.05))
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
func _rpc_ship_died(net_id: int, killer_id: int, was_captured: bool = false) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s) and not s.dead:
		s.captured = was_captured
		s._die(ships_by_id.get(killer_id, null))
	if client != null:
		client.forget(net_id)


@rpc("authority", "reliable")
func _rpc_ship_respawned(net_id: int, pos: Vector2, vel: Vector2, rot: float) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s != null and is_instance_valid(s):
		s.respawn(pos, vel)
		s.rotation = rot
		if client != null:
			client.forget(net_id)
			if net_id == local_net_id:
				client.history.clear()
		if net_id == local_net_id and world != null:
			world.player_respawned.emit()


@rpc("authority", "reliable")
func _rpc_powerup(pid: int, module_id: String, pos: Vector2, vel: Vector2) -> void:
	if world == null:
		return
	world.spawn_powerup(module_id, pos, vel)
	var p: Node = world.powerups[world.powerups.size() - 1]
	p.set_meta("net_id", pid)
	powerups_by_id[pid] = p


@rpc("authority", "reliable")
func _rpc_powerup_taken(pid: int, by_id: int) -> void:
	var p: Node = powerups_by_id.get(pid, null)
	powerups_by_id.erase(pid)
	if p != null and is_instance_valid(p):
		var by: Node = ships_by_id.get(by_id, null)
		if by != null and is_instance_valid(by):
			Sfx.play("pickup", by.global_position, 0.7)
		p.queue_free()


@rpc("authority", "reliable")
func _rpc_planet_captured(index: int, winner: int) -> void:
	if world == null or index < 0 or index >= world.planets.size():
		return
	var p: Node = world.planets[index]
	p.surrender_to(winner)
	Sfx.play("dock", p.global_position, 0.8)
	world.planet_owner_changed.emit(p)


@rpc("authority", "reliable")
func _rpc_stats(stats: Array, round_state: Array) -> void:
	if world == null:
		return
	var rd = world.round_rules
	for row in stats:
		var s: Node = ships_by_id.get(row[0], null)
		if s == null or not is_instance_valid(s):
			continue
		s.score = row[1]
		s.kills = row[2]
		s.rank = row[3]
		s.captures = row[4]
		s.deaths = row[5]
		if rd != null:
			rd.baseline[s] = [row[1] - row[6], row[2] - row[7]]
	if rd != null and round_state.size() >= 8:
		rd.length = round_state[0]
		if rd.phase != round_state[1]:
			rd.phase = round_state[1]
			rd.phase_changed.emit(rd.phase)
		rd.elapsed = round_state[2]
		rd.hold_race = round_state[3]
		rd.hold_timer = round_state[4]
		rd.over = round_state[5]
		rd.kills = round_state[6]
		rd.captures = round_state[7]


@rpc("authority", "reliable")
func _rpc_landing(net_id: int, phase: int, planet_index: int) -> void:
	if world == null:
		return
	var s: Node = ships_by_id.get(net_id, null)
	if s == null or not is_instance_valid(s):
		return
	if planet_index >= 0 and planet_index < world.planets.size():
		s.land_body = world.planets[planet_index]
	if phase != s.landing:
		s.landing = phase
		s.land_t = 0.0
		if phase == 0:
			s.land_scale = 1.0
			s.scale = Vector2.ONE


@rpc("authority", "reliable")
func _rpc_colonies(state: Array) -> void:
	if world == null:
		return
	for i in mini(state.size(), world.planets.size()):
		var p: Node = world.planets[i]
		var row: Array = state[i]
		if row[0] != p.owner_race or row[1] != p.colony_design:
			var was: int = p.owner_race
			if row[0] < 0 or row[1] == "":
				p.clear_colony()
			else:
				p.set_colony(row[0], row[1], row[2])
			if was != p.owner_race:
				world.planet_owner_changed.emit(p)
		p.population = row[2]
		p.silenced = row[3]
		if row.size() > 4:
			Ground.unpack(p, row[4])


## The stockpiles, every couple of seconds (the Resources update).
@rpc("authority", "reliable")
func _rpc_economy(rows: Array) -> void:
	if world != null and world.economy != null:
		world.economy.unpack(rows)


## The seated pilot's cargo hold and what the spaceport is still moving.
@rpc("authority", "reliable")
func _rpc_cargo(cargo: Dictionary, orders: Dictionary, kit: String) -> void:
	if world != null and world.player != null:
		world.player.cargo = cargo
		world.player.cargo_orders = orders
		world.player.kit = kit


## Built stations and frames: whether each is still going up, the work
## done and what its hold or stockpile has.
func _station_state() -> Array:
	var out := []
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if is_instance_valid(s) and s.is_station:
			out.append([s.net_id, s.building, s.build_t, s.cargo, s.orders])
	return out


@rpc("authority", "reliable")
func _rpc_stations(rows: Array) -> void:
	for row in rows:
		var s: Node = ships_by_id.get(row[0], null)
		if s == null or not is_instance_valid(s):
			continue
		s.build_t = row[2]
		s.cargo = row[3]
		s.orders = row[4] if row.size() > 4 else []
		if s.building != row[1]:
			# finished: the full station, where the frame stood
			s.building = row[1]
			var pos: Vector2 = s.global_position
			var rot: float = s.rotation
			s.refit(GameData.frame_design(s.design_name) if s.building else GameData.stock_design(s.design_name, s.race_id))
			s.global_position = pos
			s.rotation = rot


## Convoys and their escorts, for the system map: [[hauler id, escort ids...]].
func _convoy_state() -> Array:
	var out := []
	for g in world.governors.values():
		for h in g.convoys:
			if not is_instance_valid(h):
				continue
			var row := [h.net_id]
			for e in g.convoys[h]["guards"]:
				if is_instance_valid(e):
					row.append(e.net_id)
			out.append(row)
	return out


@rpc("authority", "reliable")
func _rpc_convoys(rows: Array) -> void:
	var on := {}
	for row in rows:
		var h: Node = ships_by_id.get(row[0], null)
		if h == null or not is_instance_valid(h):
			continue
		on[h] = true
		for i in range(1, row.size()):
			var e: Node = ships_by_id.get(row[i], null)
			if e != null and is_instance_valid(e):
				on[e] = h
	for id in ships_by_id:
		var s: Node = ships_by_id[id]
		if is_instance_valid(s):
			s.convoy = on.get(s, null) is bool
			s.guarding = on.get(s, null) if on.get(s, null) is Node else null


## The seated pilot's empire's job board (Governor.pack_board rows).
@rpc("authority", "reliable")
func _rpc_jobs(rows: Array) -> void:
	if world != null:
		world.job_board = rows


@rpc("authority", "reliable")
func _rpc_cabin(colonists: float, transfer: int, troops: Dictionary) -> void:
	if world != null and world.player != null:
		world.player.colonists = colonists
		world.player.transfer = transfer
		world.player.troops = troops


## `played` is false for a pilot who joined after the round was decided:
## they see the scoreboard, but it is not a round on their career.
@rpc("authority", "reliable")
func _rpc_round_over(result: Dictionary, played: bool = true) -> void:
	if world == null:
		return
	# the host's pilot is not us
	for pl in result.get("pilots", []):
		pl["is_player"] = pl["name"] == callsign
	if world.round_rules != null:
		world.round_rules.over = true
		world.round_rules.phase = "over"
		world.round_rules.result = result
	if played:
		world._on_round_finished(result)
	else:
		world.game_over = true
		world.notify_all("This round is already decided. The next one starts when the host is ready.", Color(0.8, 0.9, 1.0))
		world.round_over.emit(result)


## The host started the next round: drop this world, build a fresh one and
## say hello again, which seats us like a new joiner.
@rpc("authority", "reliable")
func _rpc_new_round() -> void:
	if not is_client():
		return
	world = null
	ships_by_id.clear()
	powerups_by_id.clear()
	local_net_id = 0
	client = NetClient.new()
	round_restarted.emit()


@rpc("authority", "reliable")
func _rpc_refit(net_id: int, design: Dictionary) -> void:
	var s: Node = ships_by_id.get(net_id, null)
	if s == null or not is_instance_valid(s):
		return
	var pos: Vector2 = s.global_position
	var vel: Vector2 = s.velocity
	var rot: float = s.rotation
	s.refit(design)
	s.global_position = pos
	s.velocity = vel
	s.rotation = rot
	Sfx.play("dock", pos, 0.8)
	if net_id == local_net_id:
		s.docked = false      # the host fitted our build: back to the controls


@rpc("authority", "reliable")
func _rpc_message(text: String, color: Color) -> void:
	if world != null:
		world.message.emit(text, color)


## Runs on the client every physics tick instead of the host-only parts of World.
func client_tick(delta: float) -> void:
	if world == null or local_net_id == 0 or client == null:
		return
	var p: Node = world.player
	if p != null and not p.dead:
		input_seq += 1
		client.record_input(input_seq, p.inputs, p.aim_point)
		_rpc_input.rpc_id(1, NetCodec.pack_input(input_seq, p.inputs, p.aim_point, p.global_position))
	client.tick(delta, ships_by_id)
	if world.duel_rules != null:
		world.duel_rules.client_tick(delta)
	var rd = world.round_rules
	if rd != null and not rd.over:
		rd.elapsed += delta           # between stats the clock runs locally
		if rd.hold_race >= 0:
			rd.hold_timer += delta
	ping_timer -= delta
	if ping_timer <= 0.0:
		ping_timer = PING_EVERY
		_rpc_ping.rpc_id(1, Time.get_ticks_msec())


# --------------------------------------------------------------- duels
# A free-for-all (Duel): each joiner flies their own ship in their own
# empire's colours, against everyone.  The host's rules decide; clients
# get the clock and the kill count.

## A joiner's seat in a duel: a fresh ship, which only the peers already
## seated hear about now (the joiner gets it in the welcome's roster).
func _seat_duelist(peer_id: int, name: String, race: int) -> Node:
	if seats.size() + 1 >= MAX_PLAYERS:
		return null
	var s: Node = world.duel_rules.pilot_joined(name, race)
	_register(s)
	for other in seats:
		_rpc_ship_added.rpc_id(other, _roster_row(s))
	seats[peer_id] = s.net_id
	pilot_names[peer_id] = name
	_seat_ai(s, false)
	return s


## A kill, a round won or a new round: everyone's tally at once, not at
## the next stats tick.
func host_duel_changed() -> void:
	if not is_host() or seats.is_empty() or world == null or world.duel_rules == null:
		return
	_rpc_stats.rpc(_stats(), _round_state())
	_rpc_duel.rpc(world.duel_rules.pack())


@rpc("authority", "reliable")
func _rpc_duel(state: Array) -> void:
	if world == null:
		return
	if world.duel_rules == null:
		# the host is running a duel: no Conquest round here
		mode = "duel"
		world.duel = true
		world.duel_rules = Duel.new()
		world.duel_rules.setup(world)
		world.round_rules = null
		world.notify_all("Free-for-all duel: everyone is your enemy, and everyone flies a %s." % Duel.HULL, Color(0.8, 0.9, 1.0))
		world.notify_all("First to %d kills wins the round. %s targets the nearest pilot." % [Duel.KILLS_TO_WIN, Keybinds.short_text("target_nearest")], Color(0.8, 0.9, 1.0))
	world.duel_rules.unpack(state)


# ------------------------------------------------------------- trading
# Two pilots within TRADE_RANGE put credits, salvage and blueprints on the
# table; both accept; the host swaps.  Both ships sit parked meanwhile.
# The host's own pilot goes through the same code with peer id 1.

signal trade_opened(data: Dictionary)
signal trade_updated(data: Dictionary)
signal trade_closed(data: Dictionary)

const TRADE_RANGE := 900.0

var trades := {}                 # id -> {id, ships: [net_id, net_id], offers: {}, accepted: {}}
var next_trade_id := 1


func _peer_of(net_id: int) -> int:
	if world != null and world.player != null and world.player.net_id == net_id:
		return 1
	for peer_id in seats:
		if seats[peer_id] == net_id:
			return peer_id
	return -1


func _to_peer(peer_id: int, kind: String, data: Dictionary) -> void:
	if peer_id == 1:
		_rpc_trade_msg(kind, data)
	elif peer_id > 1:
		_rpc_trade_msg.rpc_id(peer_id, kind, data)


## Any pilot: ask the host for a trade with the targeted ship.
func request_trade(s: Node, target: Node) -> void:
	if target == null or not is_instance_valid(target):
		if world != null:
			world.notify(s, "Target a pilot first (T), then press O to trade.")
		return
	trade_cmd("invite", {"target": target.net_id})


## Any pilot's trade action: the host handles it, a client sends it.
func trade_cmd(kind: String, data: Dictionary) -> void:
	if is_host():
		_handle_trade_cmd(1, kind, data)
	elif is_client():
		_rpc_trade_cmd.rpc_id(1, kind, data)


@rpc("any_peer", "reliable")
func _rpc_trade_cmd(kind: String, data: Dictionary) -> void:
	if is_host():
		_handle_trade_cmd(multiplayer.get_remote_sender_id(), kind, data)


func _trade_of(net_id: int) -> Dictionary:
	for t in trades.values():
		if net_id in t["ships"]:
			return t
	return {}


static func _clean_offer(offer: Variant) -> Dictionary:
	var o := {"credits": 0, "parts": [], "blueprints": [], "cargo": {}}
	if offer is Dictionary:
		o["credits"] = maxi(int(offer.get("credits", 0)), 0)
		for i in offer.get("parts", []):
			if int(i) >= 0 and not o["parts"].has(int(i)):
				o["parts"].append(int(i))
		for b in offer.get("blueprints", []):
			if not o["blueprints"].has(str(b)):
				o["blueprints"].append(str(b))
		var cargo: Variant = offer.get("cargo", {})
		if cargo is Dictionary:
			# resources from the cargo hold, whole units
			for k in cargo:
				if Economy.RESOURCES.has(str(k)) and int(cargo[k]) > 0:
					o["cargo"][str(k)] = int(cargo[k])
	return o


## An offer is only as good as the purse behind it.
func _offer_fits(s: Node, o: Dictionary) -> bool:
	if o["credits"] > s.credits:
		return false
	for i in o["parts"]:
		if i >= s.hold.size():
			return false
	for b in o["blueprints"]:
		if not s.blueprints.has(b):
			return false
	for k in o["cargo"]:
		if o["cargo"][k] > s.cargo.get(k, 0.0) + 0.01:
			return false
	return true


func _purse_of(s: Node) -> Dictionary:
	return {"credits": s.credits, "hold": s.hold.duplicate(), "blueprints": s.blueprints.duplicate(), "cargo": s.cargo.duplicate()}


func _park(s: Node, on: bool) -> void:
	s.docked = on
	if on:
		for k in s.inputs.keys():
			s.inputs[k] = false if typeof(s.inputs[k]) == TYPE_BOOL else 0.0
		s.inputs["brake"] = true


func _trade_view(t: Dictionary) -> Dictionary:
	# offers as the other side should read them: part names, not hold indices
	var offers := {}
	for id in t["ships"]:
		var o: Dictionary = t["offers"][id].duplicate(true)
		var s: Node = ships_by_id.get(id, null)
		var names := []
		for i in o["parts"]:
			if s != null and i < s.hold.size():
				names.append(GameData.MODULES[s.hold[i]]["name"] if GameData.MODULES.has(s.hold[i]) else s.hold[i])
		o["part_names"] = names
		offers[id] = o
	return {"id": t["id"], "offers": offers, "accepted": t["accepted"].duplicate()}


func _handle_trade_cmd(peer_id: int, kind: String, data: Dictionary) -> void:
	if world == null:
		return
	var me: Node = ships_by_id.get(seats.get(peer_id, 0), null) if peer_id != 1 else world.player
	if me == null or not is_instance_valid(me) or me.dead:
		return
	if world.duel_rules != null:
		if kind == "invite":
			world.notify(me, "No trading in a duel: everyone here is out to get you.")
		return
	match kind:
		"invite":
			var other: Node = ships_by_id.get(int(data.get("target", 0)), null)
			if other == null or not is_instance_valid(other) or other == me or other.dead:
				world.notify(me, "No one there to trade with.")
			elif _peer_of(other.net_id) < 0:
				world.notify(me, "%s is a computer pilot; only pilots can trade." % other.pilot_name)
			elif me.global_position.distance_to(other.global_position) > TRADE_RANGE:
				world.notify(me, "Too far from %s to trade: close to within %d." % [other.pilot_name, int(TRADE_RANGE)])
			elif not _trade_of(me.net_id).is_empty() or not _trade_of(other.net_id).is_empty() or me.docked or other.docked:
				world.notify(me, "%s is busy." % other.pilot_name)
			else:
				var t := {"id": next_trade_id, "ships": [me.net_id, other.net_id],
					"offers": {me.net_id: _clean_offer({}), other.net_id: _clean_offer({})},
					"accepted": {me.net_id: false, other.net_id: false}}
				next_trade_id += 1
				trades[t["id"]] = t
				_park(me, true)
				_park(other, true)
				for id in t["ships"]:
					var s: Node = ships_by_id[id]
					var partner: Node = ships_by_id[t["ships"][1] if id == t["ships"][0] else t["ships"][0]]
					var d := _purse_of(s)
					d["id"] = t["id"]
					d["me"] = id
					d["partner"] = partner.net_id
					d["partner_name"] = partner.pilot_name
					_to_peer(_peer_of(id), "open", d)
				world.notify_all("%s and %s are trading." % [me.pilot_name, other.pilot_name], Color(0.8, 0.9, 1.0))
		"offer":
			var t := _trade_of(me.net_id)
			if t.is_empty() or t["id"] != int(data.get("id", -1)):
				return
			var o := _clean_offer(data.get("offer", {}))
			if not _offer_fits(me, o):
				_to_peer(peer_id, "update", _trade_view(t))
				return
			t["offers"][me.net_id] = o
			for id in t["ships"]:
				t["accepted"][id] = false
				_to_peer(_peer_of(id), "update", _trade_view(t))
		"accept":
			var t := _trade_of(me.net_id)
			if t.is_empty() or t["id"] != int(data.get("id", -1)):
				return
			if t["accepted"][me.net_id]:
				return   # already said yes; nothing new to tell
			t["accepted"][me.net_id] = true
			var all := true
			for id in t["ships"]:
				all = all and t["accepted"][id]
			if all:
				_settle_trade(t)
			else:
				for id in t["ships"]:
					_to_peer(_peer_of(id), "update", _trade_view(t))
		"cancel":
			var t := _trade_of(me.net_id)
			if not t.is_empty():
				_close_trade(t, "%s called the trade off." % me.pilot_name)


func _settle_trade(t: Dictionary) -> void:
	var a: Node = ships_by_id.get(t["ships"][0], null)
	var b: Node = ships_by_id.get(t["ships"][1], null)
	if a == null or b == null or not is_instance_valid(a) or not is_instance_valid(b):
		_close_trade(t, "A pilot is gone.")
		return
	var oa: Dictionary = t["offers"][a.net_id]
	var ob: Dictionary = t["offers"][b.net_id]
	if not _offer_fits(a, oa) or not _offer_fits(b, ob):
		_close_trade(t, "The goods changed under the table; try again.")
		return
	# gather what each side gives, then hand it over
	var give_a := _take(a, oa)
	var give_b := _take(b, ob)
	_give(b, give_a)
	_give(a, give_b)
	trades.erase(t["id"])
	for s in [a, b]:
		_park(s, false)
		var d := _purse_of(s)
		d["id"] = t["id"]
		d["reason"] = "Deal done."
		d["done"] = true
		_to_peer(_peer_of(s.net_id), "closed", d)
	world.notify_all("%s and %s made a deal." % [a.pilot_name, b.pilot_name], Color(0.8, 1.0, 0.8))


func _take(s: Node, o: Dictionary) -> Dictionary:
	var out := {"credits": o["credits"], "parts": [], "blueprints": o["blueprints"].duplicate(), "cargo": o["cargo"]}
	s.credits -= o["credits"]
	var idx: Array = o["parts"].duplicate()
	idx.sort()
	idx.reverse()
	for i in idx:
		out["parts"].append(s.hold[i])
		s.hold.remove_at(i)
	for k in o["cargo"]:
		s.cargo[k] = s.cargo.get(k, 0.0) - o["cargo"][k]
		if s.cargo[k] < 0.01:
			s.cargo.erase(k)
	return out


func _give(s: Node, goods: Dictionary) -> void:
	s.credits += goods["credits"]
	for id in goods["parts"]:
		if s.hold.size() < Ship.HOLD_SIZE:
			s.hold.append(id)
		else:
			world.spawn_powerup(id, s.global_position + Vector2(randf_range(-30, 30), randf_range(-30, 30)), s.velocity)
			world.notify(s, "Hold full: %s left floating beside you." % GameData.MODULES[id]["name"])
	for b in goods["blueprints"]:
		if not s.blueprints.has(b):
			s.blueprints.append(b)
	# goods for the hold: what does not fit is left floating in crates
	for k in goods.get("cargo", {}):
		var n: float = goods["cargo"][k]
		var fits := minf(n, maxf(s.cargo_space(), 0.0))
		if fits > 0.0:
			s.cargo[k] = s.cargo.get(k, 0.0) + fits
		if n - fits >= 1.0:
			world.spawn_powerup("rs:%s:%d" % [k, int(n - fits)], s.global_position + Vector2(randf_range(-40, 40), randf_range(-40, 40)), s.velocity)
			world.notify(s, "Hold full: %d %s left floating beside you." % [int(n - fits), Economy.NAMES[k]])


func _close_trade(t: Dictionary, reason: String) -> void:
	trades.erase(t["id"])
	for id in t["ships"]:
		var s: Node = ships_by_id.get(id, null)
		if s != null and is_instance_valid(s):
			_park(s, false)
		var d := {"id": t["id"], "reason": reason, "done": false}
		if s != null and is_instance_valid(s):
			d.merge(_purse_of(s))
		_to_peer(_peer_of(id), "closed", d)


## Each tick on the host: trades end when a pilot dies or drifts away.
func _watch_trades() -> void:
	for t in trades.values().duplicate():
		var a: Node = ships_by_id.get(t["ships"][0], null)
		var b: Node = ships_by_id.get(t["ships"][1], null)
		if a == null or b == null or not is_instance_valid(a) or not is_instance_valid(b) or a.dead or b.dead:
			_close_trade(t, "The trade ended: a pilot was lost.")
		elif a.global_position.distance_to(b.global_position) > TRADE_RANGE * 1.5:
			_close_trade(t, "The trade ended: the ships drifted apart.")


## What the host tells a pilot about their trade (also called locally for
## the host's own pilot).
@rpc("authority", "reliable")
func _rpc_trade_msg(kind: String, data: Dictionary) -> void:
	match kind:
		"open":
			trade_opened.emit(data)
		"update":
			trade_updated.emit(data)
		"closed":
			# the purse after the deal (or unchanged) comes with the close
			var s: Node = ships_by_id.get(local_net_id, null) if is_client() else null
			if s != null and is_instance_valid(s) and data.has("credits"):
				s.credits = data["credits"]
				s.hold = data["hold"]
				s.blueprints = data["blueprints"]
			trade_closed.emit(data)


# ------------------------------------------------------------- per tick

func _physics_process(delta: float) -> void:
	if not is_host() or world == null:
		return
	_apply_remote_inputs()
	if not trades.is_empty():
		_watch_trades()
	if seats.is_empty():
		snapshot_timer = 0.0
		return
	snapshot_timer -= delta
	if snapshot_timer <= 0.0:
		snapshot_timer += SNAPSHOT_INTERVAL
		_send_snapshots()
	systems_timer -= delta
	if systems_timer <= 0.0:
		systems_timer = SYSTEMS_EVERY
		systems_sends += 1
		_send_systems(systems_sends % SYSTEMS_FULL_EVERY == 0)
	stats_timer -= delta
	if stats_timer <= 0.0:
		stats_timer = STATS_EVERY
		_rpc_stats.rpc(_stats(), _round_state())
		_rpc_colonies.rpc(_colony_state())
		_rpc_economy.rpc(world.economy.pack())
		_rpc_stations.rpc(_station_state())
		_rpc_convoys.rpc(_convoy_state())
		if world.duel_rules != null:
			_rpc_duel.rpc(world.duel_rules.pack())
		for peer_id in seats:
			var s: Node = ships_by_id.get(seats[peer_id], null)
			if s != null and is_instance_valid(s):
				_rpc_purse.rpc_id(peer_id, s.credits, s.hold, s.blueprints)
				_rpc_cabin.rpc_id(peer_id, s.colonists, s.transfer, s.troops)
				_rpc_cargo.rpc_id(peer_id, s.cargo, s.cargo_orders, s.kit)
				var gov: Governor = world.governors.get(s.race_id, null)
				_rpc_jobs.rpc_id(peer_id, gov.pack_board() if gov != null else [])
