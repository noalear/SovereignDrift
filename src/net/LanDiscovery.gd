## LanDiscovery -- hosts shout on the local network once a second; menus
## listen and list them.  One UDP broadcast on port 7778, JSON inside.
class_name LanDiscovery
extends Node

const PORT := 7778
const ANNOUNCE_EVERY := 1.0
const EXPIRE_AFTER := 4.0

var announcing := false
var listening := false
var games := {}                  # ip -> {name, players, max, port, protocol, seen}
var _sock: PacketPeerUDP = null
var _timer := 0.0
var _info := {}


func announce(info: Dictionary) -> void:
	_info = info
	if announcing:
		return
	stop()
	_sock = PacketPeerUDP.new()
	_sock.set_broadcast_enabled(true)
	_sock.set_dest_address("255.255.255.255", PORT)
	announcing = true
	_timer = 0.0


func listen() -> void:
	if listening:
		return
	stop()
	_sock = PacketPeerUDP.new()
	if _sock.bind(PORT, "0.0.0.0") != OK:
		_sock = null
		return
	listening = true
	games.clear()


func stop() -> void:
	if _sock != null:
		_sock.close()
	_sock = null
	announcing = false
	listening = false


func _process(delta: float) -> void:
	if _sock == null:
		return
	var now := Time.get_ticks_msec() / 1000.0
	if announcing:
		_timer -= delta
		if _timer <= 0.0:
			_timer = ANNOUNCE_EVERY
			_sock.put_packet(JSON.stringify(_info).to_utf8_buffer())
	elif listening:
		while _sock.get_available_packet_count() > 0:
			var data := _sock.get_packet()
			var ip := _sock.get_packet_ip()
			var parsed = JSON.parse_string(data.get_string_from_utf8())
			if parsed is Dictionary and parsed.has("name"):
				parsed["seen"] = now
				parsed["ip"] = ip
				games[ip] = parsed
		for ip in games.keys():
			if now - games[ip]["seen"] > EXPIRE_AFTER:
				games.erase(ip)


## The games heard recently, newest first.
func list() -> Array:
	var out := games.values()
	out.sort_custom(func(a, b): return a["seen"] > b["seen"])
	return out
