## NetCodec -- packs inputs and world snapshots into small byte arrays.
## Positions are float32, angles int16 (1/10000 rad), gauges one byte each.
class_name NetCodec

const BTN_BOOST := 1
const BTN_BRAKE := 2
const BTN_PRIMARY := 4
const BTN_SECONDARY := 8
const BTN_MISSILE := 16
const BTN_MINE := 32
const BTN_BEAM := 64
const BTN_AIM := 128          # aim_point is set

const FLAG_DEAD := 1
const FLAG_JAMMING := 2       # its jammer is on
const FLAG_WARPING := 4
const FLAG_STATION := 8
const FLAG_JAMMED := 16       # someone's jammer is on it
const FLAG_SILENT := 32       # silent running

## Signatures go as one byte each, in steps of this.
const SIG_STEP := 4.0


static func _axis(v: float) -> int:
	return int(clampf(v, -1.0, 1.0) * 127.0)


static func _gauge(v: float, top: float) -> int:
	return int(clampf(v / maxf(top, 0.001), 0.0, 1.0) * 255.0)


## One tick of a pilot's controls.
static func pack_input(seq: int, inputs: Dictionary, aim_point: Vector2, pos: Vector2) -> PackedByteArray:
	var b := StreamPeerBuffer.new()
	b.put_u32(seq)
	b.put_8(_axis(inputs["thrust"]))
	b.put_8(_axis(inputs["turn"]))
	b.put_8(_axis(inputs["strafe"]))
	var btn := 0
	if inputs["boost"]: btn |= BTN_BOOST
	if inputs["brake"]: btn |= BTN_BRAKE
	if inputs["fire_primary"]: btn |= BTN_PRIMARY
	if inputs["fire_secondary"]: btn |= BTN_SECONDARY
	if inputs["fire_missile"]: btn |= BTN_MISSILE
	if inputs["fire_mine"]: btn |= BTN_MINE
	if inputs["fire_beam"]: btn |= BTN_BEAM
	var aim := 0.0
	if aim_point != Vector2.INF:
		btn |= BTN_AIM
		aim = (aim_point - pos).angle()
	b.put_u8(btn)
	b.put_16(int(aim * 10000.0))
	return b.data_array


## Returns {seq, inputs, aim_angle (or INF)}.
static func unpack_input(data: PackedByteArray) -> Dictionary:
	var b := StreamPeerBuffer.new()
	b.data_array = data
	var out := {"seq": b.get_u32(), "inputs": {}, "aim_angle": INF}
	var i: Dictionary = out["inputs"]
	i["thrust"] = b.get_8() / 127.0
	i["turn"] = b.get_8() / 127.0
	i["strafe"] = b.get_8() / 127.0
	var btn := b.get_u8()
	i["boost"] = (btn & BTN_BOOST) != 0
	i["brake"] = (btn & BTN_BRAKE) != 0
	i["fire_primary"] = (btn & BTN_PRIMARY) != 0
	i["fire_secondary"] = (btn & BTN_SECONDARY) != 0
	i["fire_missile"] = (btn & BTN_MISSILE) != 0
	i["fire_mine"] = (btn & BTN_MINE) != 0
	i["fire_beam"] = (btn & BTN_BEAM) != 0
	var aim := b.get_16() / 10000.0
	if (btn & BTN_AIM) != 0:
		out["aim_angle"] = aim
	return out


## The state of every ship, 30 bytes each, after a 12-byte header: heat,
## EM and temporal signatures ride along so clients can work out their
## own contacts (radar return they work out themselves).
static func pack_snapshot(elapsed: float, ack_seq: int, ships: Array) -> PackedByteArray:
	var b := StreamPeerBuffer.new()
	b.put_float(elapsed)
	b.put_u32(ack_seq)
	b.put_u16(ships.size())
	b.put_u16(0)
	for s in ships:
		b.put_u16(s.net_id)
		b.put_float(s.global_position.x)
		b.put_float(s.global_position.y)
		b.put_float(s.velocity.x)
		b.put_float(s.velocity.y)
		b.put_16(int(wrapf(s.rotation, -PI, PI) * 10000.0))
		b.put_u8(_gauge(s.hull, s.hull_max))
		b.put_u8(_gauge(s.shield, s.shield_max))
		b.put_u8(_gauge(s.energy, s.energy_max))
		b.put_u8(int(clampf(s.thrust_level, 0.0, 1.0) * 255.0))
		var flags := 0
		if s.dead: flags |= FLAG_DEAD
		if s.jamming: flags |= FLAG_JAMMING
		if s.warping: flags |= FLAG_WARPING
		if s.is_station: flags |= FLAG_STATION
		if s.sensors.is_jammed(): flags |= FLAG_JAMMED
		if s.sensors.emcon: flags |= FLAG_SILENT
		b.put_u8(flags)
		for ch in ["heat", "em", "temporal"]:
			b.put_u8(int(clampf(s.sensors.signature(ch) / SIG_STEP, 0.0, 255.0)))
	return b.data_array


## Returns {elapsed, ack_seq, ships: [{net_id, pos, vel, rot, hull, shield, energy, thrust, flags, sig}]}.
static func unpack_snapshot(data: PackedByteArray) -> Dictionary:
	var b := StreamPeerBuffer.new()
	b.data_array = data
	var out := {"elapsed": b.get_float(), "ack_seq": b.get_u32(), "ships": []}
	var n := b.get_u16()
	b.get_u16()
	for k in n:
		var e := {}
		e["net_id"] = b.get_u16()
		e["pos"] = Vector2(b.get_float(), b.get_float())
		e["vel"] = Vector2(b.get_float(), b.get_float())
		e["rot"] = b.get_16() / 10000.0
		e["hull"] = b.get_u8() / 255.0
		e["shield"] = b.get_u8() / 255.0
		e["energy"] = b.get_u8() / 255.0
		e["thrust"] = b.get_u8() / 255.0
		e["flags"] = b.get_u8()
		e["sig"] = [0.0, b.get_u8() * SIG_STEP, b.get_u8() * SIG_STEP, b.get_u8() * SIG_STEP]
		out["ships"].append(e)
	return out
