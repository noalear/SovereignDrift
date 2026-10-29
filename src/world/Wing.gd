## Wing -- the pilots who fly with a leader and take its orders: captured
## prizes, escorts ordered from a yard, and the empire's computer pilots
## called up to the leader's wing (more seats as the leader rises in rank).
##
## Orders go to the whole wing (OrdersWheel, or the quick keys) and each
## wingman's AIPilot reads them: form up and only return fire, attack the
## leader's target wherever it goes, cover the leader (the default), hold
## a spot or a world, engage at will round the leader, or go home to
## repair and come back.  Call brings free pilots of the empire to the
## wing; Dismiss sends them back to their duties.  The empires' senior
## computer pilots (AI admirals) call up wings of their own.
##
## The host runs it all; a client sees its own and its friends' wings in
## `mirror`, filled by Net.
class_name Wing
extends RefCounted

## The wheel's slices, clockwise from the top.
const WHEEL := ["form", "attack", "cover", "hold", "engage", "home", "call", "dismiss"]
const LABELS := {
	"form": "Form up", "attack": "Attack my target", "cover": "Cover me", "hold": "Hold here",
	"engage": "Engage at will", "home": "Return to base", "call": "Call wingmen", "dismiss": "Dismiss",
}
const HINTS := {
	"form": "Tight on you, return fire only",
	"attack": "Everyone on your target: a ship, or a colony's guns",
	"cover": "Fight what comes near you, help with your target",
	"hold": "Guard this spot, or the world you have targeted",
	"engage": "Hunt anything in sight round you",
	"home": "Repair at home, then rejoin",
	"call": "Bring the empire's free pilots to your wing",
	"dismiss": "Called pilots go back to their duties",
}
## A new wing covers its leader.
const DEFAULT := "cover"

## Called seats: one for a cadet, one more every three ranks.  Prizes and
## ordered escorts fly on top of these (Capture.MAX_PRIZES_PER_PILOT).
const MAX_SEATS := 4
## Captains and above fly only for a leader of their own rank or higher.
const SENIOR_RANK := 7
## Wingmen on Hold fight what comes this close to the spot they guard.
const HOLD_LEASH := 1800.0
## On Engage at will they hunt this far round their leader.
const ENGAGE_RANGE := 5000.0
## A wingman goes home on its own with this little fuel left.
const BINGO_FUEL := 0.12
## An attack order lapses when nobody in the wing has seen its target for this long.
const LOST_AFTER := 10.0
const TRACK_GRACE := 4.0       # seconds the wing keeps after a target nobody sees, on its last track
## Computer leaders: from this rank one wingman, from ADMIRAL_RANK two.
const COMMODORE_RANK := 9
const ADMIRAL_RANK := 12
## How often computer leaders look for wingmen.
const AI_CALL_EVERY := 15.0

var world: Node
var orders := {}             # leader -> {"order", "target", "at", "body", "lost"}
var mirror := {}             # a client's copy: leader net id -> {"order", "target", "at", "members": [[id, status], ...]}
var _tick := 0.0
var _ai_tick := AI_CALL_EVERY * 0.5
var dirty := false           # something changed that clients should hear about now


func _init(p_world: Node) -> void:
	world = p_world


static func seats(rank: int) -> int:
	return clampi(1 + rank / 3, 1, MAX_SEATS)


## Will this computer pilot fly for `leader`?
static func would_serve(ai_rank: int, leader: Ship) -> bool:
	return ai_rank < SENIOR_RANK or ai_rank <= leader.rank


# ------------------------------------------------------------ the roster

static func _ai(s: Ship) -> AIPilot:
	return s.get_node_or_null("AI") as AIPilot if s != null and is_instance_valid(s) else null


## The ships flying on `leader`'s wing, in a steady order (their slots in
## the formation).  Benched ones are included: they relaunch and rejoin.
func members(leader: Ship) -> Array:
	var out := []
	if leader == null or not is_instance_valid(leader):
		return out
	if Net.is_client():
		var m: Dictionary = mirror.get(leader.net_id, {})
		for row in m.get("members", []):
			var s: Node = Net.ships_by_id.get(row[0], null)
			if s != null and is_instance_valid(s):
				out.append(s)
		return out
	for s in world.ships:
		var ai := _ai(s)
		if ai != null and ai.escort == leader and not ai.convoy_guard:
			out.append(s)
	out.sort_custom(func(a, b): return a.get_instance_id() < b.get_instance_id())
	return out


## Where `s` flies in its leader's formation, 0 first.
func slot_of(s: Ship, leader: Ship) -> int:
	var n := 0
	for m in members(leader):
		if m == s:
			return n
		if not m.dead:
			n += 1
	return n


## Computer pilots called up to this leader's wing (not prizes or escorts).
func called(leader: Ship) -> Array:
	return members(leader).filter(func(s): return _ai(s) != null and _ai(s).called)


func order_of(leader: Ship) -> Dictionary:
	if Net.is_client():
		return mirror.get(leader.net_id, {"order": DEFAULT}) if leader != null else {"order": DEFAULT}
	if not orders.has(leader):
		orders[leader] = {"order": DEFAULT, "target": null, "at": Vector2.ZERO, "body": null, "lost": 0.0}
	return orders[leader]


## The spot a holding wing guards, moving with the world it holds over.
func hold_point(leader: Ship) -> Vector2:
	var o := order_of(leader)
	var b: Node2D = o.get("body", null)
	if b != null and is_instance_valid(b):
		return b.global_position + o["at"]
	return o.get("at", Vector2.ZERO)


## What a wingman is doing, for the HUD and the map.
func status(s: Ship) -> String:
	if Net.is_client():
		for id in mirror:
			for row in mirror[id].get("members", []):
				if row[0] == s.net_id:
					return row[1]
		return ""
	if s.dead:
		return "relaunching" if not s.is_prize and not s.has_meta("ordered") else "lost"
	var ai := _ai(s)
	if ai == null:
		return ""
	if ai.sent_home:
		var hull := ceili(s.hull / maxf(s.hull_max, 1.0) * 100.0)
		if s.is_landed():
			return "repairing %d%%" % hull
		return "going home %d%%" % hull
	if s.is_landing():
		return "landed"
	if s.is_disabled():
		return "disabled!"
	var t: Variant = s.target
	if not is_instance_valid(t):
		t = null
	if ai.state == AIPilot.State.BOARD and t is Ship:
		return "boarding " + t.pilot_name
	if ai.state in [AIPilot.State.ATTACK, AIPilot.State.EVADE] and t != null:
		return "fighting " + (t.pilot_name if t is Ship else t.body_name)
	var lead: Ship = ai.escort
	if lead != null and not lead.dead and s.global_position.distance_to(lead.global_position) > 3000.0 and ai.order() != "hold":
		return "on the way, %d" % int(s.global_position.distance_to(lead.global_position))
	match ai.order():
		"form":
			return "in formation"
		"hold":
			return "holding"
		"engage":
			return "hunting"
		"attack":
			return "attacking"
	return "covering you"


# ------------------------------------------------------------- commands

## The leader's order (the host's side; World.wing_command routes a
## client's here through Net).  `target` is the leader's target when the
## order was given.
func command(leader: Ship, order: String, target: Node2D) -> void:
	if leader == null or leader.dead or not order in WHEEL:
		return
	if order == "call":
		call_up(leader, seats(leader.rank))
		return
	if order == "dismiss":
		dismiss(leader)
		return
	var wing := members(leader).filter(func(s): return not s.dead)
	if wing.is_empty():
		_tell(leader, "No wingmen to order. %s calls the empire's pilots; captured ships fly with you too." % Keybinds.short_text("wing_menu"))
		return
	var o := order_of(leader)
	match order:
		"attack":
			var ok: bool = target != null and is_instance_valid(target) and (
				(target is Ship and not target.dead and target.team() != leader.team() and target.race_id != GameData.NEUTRAL)
				or (target is Planet and target.is_colonised() and target.owner_race != leader.race_id and not target.is_defenceless()))
			if not ok:
				_tell(leader, "Target an enemy ship or an armed enemy colony first, then order the attack.")
				return
			o["target"] = target
			o["lost"] = 0.0
		"hold":
			# over the targeted world, else here (moving with a world close by)
			var body: Planet = target if target is Planet else world.nearest_body(leader.global_position)
			if target is Planet:
				o["body"] = target
				var out: Vector2 = leader.global_position - target.global_position
				o["at"] = (out.normalized() if out.length() > 1.0 else Vector2.RIGHT) * (target.radius + 420.0)
			elif body != null and body != world.sun and leader.global_position.distance_to(body.global_position) < body.radius + 2500.0:
				o["body"] = body
				o["at"] = leader.global_position - body.global_position
			else:
				o["body"] = null
				o["at"] = leader.global_position
		"home":
			for s in wing:
				_ai(s).go_home()
			o["order"] = DEFAULT if o["order"] in ["home", "attack"] else o["order"]
			_ack(leader, wing, "home")
			dirty = true
			return
	o["order"] = order
	for s in wing:
		_ai(s).heard_order()
	_ack(leader, wing, order)
	dirty = true


## Bring free computer pilots of the leader's empire onto its wing, the
## nearest first, up to `want` called seats.  How many came.
func call_up(leader: Ship, want: int) -> int:
	var human := _human(leader)
	if world.is_arena() or (world.practice and not world.tutorial):
		if human:
			_tell(leader, "Nobody to call here: wings fly in Conquest rounds, campaigns and online co-op.")
		return 0
	var room := mini(want, seats(leader.rank)) - called(leader).size()
	if room <= 0:
		if human:
			_tell(leader, "Your wing is full: a %s calls up %d pilot%s." % [GameData.rank_name(leader.rank), seats(leader.rank), "" if seats(leader.rank) == 1 else "s"])
		return 0
	var pool := []
	for s in world.ships:
		if _free_for(s, leader):
			pool.append(s)
	var near := func(a, b): return a.global_position.distance_to(leader.global_position) < b.global_position.distance_to(leader.global_position)
	pool.sort_custom(near)
	if human and pool.size() < room:
		# a player's call comes before a computer admiral's: its called pilots come over
		var borrowed := []
		for s in world.ships:
			var ai := _ai(s)
			if ai == null or not ai.called or ai.escort == null or not is_instance_valid(ai.escort) or _human(ai.escort):
				continue
			var lead: Ship = ai.escort
			ai.escort = null
			var ok := _free_for(s, leader)
			ai.escort = lead
			if ok:
				borrowed.append(s)
		borrowed.sort_custom(near)
		pool.append_array(borrowed)
	var came := []
	for s in pool:
		if came.size() >= room:
			break
		if _ai(s).escort != null:
			_ai(s).leave_wing()
		_ai(s).join(leader, true)
		came.append(s)
	if came.is_empty():
		if human:
			var senior := false
			for s in world.ships:
				if _free_for(s, leader, true):
					senior = true
			_tell(leader, "No free pilot to call: %s." % ("the free ones outrank you (Captains fly only for their equals)" if senior
				else "every %s pilot is on a wing, a convoy or a colonist run" % GameData.race(leader.race_id)["adj"]))
		return 0
	if human:
		_ack(leader, [came[0]], "join")
		var names := came.map(func(s): return s.pilot_name)
		world.notify(leader, "%s join%s your wing (%d / %d called)." % [" and ".join(names), "s" if came.size() == 1 else "", called(leader).size(), seats(leader.rank)])
	dirty = true
	return came.size()


func _human(s: Ship) -> bool:
	return s.is_player or s.net_id in Net.seats.values()


## Can `s` be called to `leader`'s wing?  `ignore_rank`: free in every way but rank.
func _free_for(s: Ship, leader: Ship, ignore_rank: bool = false) -> bool:
	if s == leader or s.dead or s.race_id != leader.race_id or s.support or s.is_station or s.is_prize or s.is_player:
		return false
	if s.net_id in Net.seats.values() or s.has_meta("ordered") or s.has_meta("tutorial") or s.is_landing():
		return false
	var ai := _ai(s)
	if ai == null or ai.escort != null or ai.convoy_guard or ai.hauler or ai.duel or ai.state == AIPilot.State.BOARD:
		return false
	if ai.ferry and s.colonists > 0.0:
		return false               # colonists aboard: that run comes first
	if not members(s).is_empty():
		return false               # leads a wing of its own
	return ignore_rank or (would_serve(s.rank, leader) and (s.rank < leader.rank or leader.is_player or leader.net_id in Net.seats.values()))


## Called pilots go back to the empire.  Prizes and ordered escorts stay.
func dismiss(leader: Ship) -> void:
	var gone := called(leader)
	if gone.is_empty():
		_tell(leader, "No called pilots to dismiss (prizes and escorts you ordered stay with you).")
		return
	for s in gone:
		_ai(s).leave_wing()
	_ack(leader, gone, "dismiss")
	world.notify(leader, "%s back to %s duties." % [", ".join(gone.map(func(s): return s.pilot_name)), "their" if gone.size() > 1 else "its"])
	dirty = true


# ---------------------------------------------------------- every tick

func tick(delta: float) -> void:
	_tick -= delta
	if _tick > 0.0:
		return
	_tick = 0.5
	for leader in orders.keys():
		if not is_instance_valid(leader):
			orders.erase(leader)
			continue
		var o: Dictionary = orders[leader]
		if o["order"] == "attack" and not _target_good(leader, o, 0.5):
			o["order"] = DEFAULT
			o["target"] = null
			var wing := members(leader).filter(func(s): return not s.dead)
			if not wing.is_empty():
				_ack(leader, wing, "kill" if o.get("killed", false) else "cover")
			o.erase("killed")
			dirty = true
	_ai_tick -= 0.5
	if _ai_tick <= 0.0:
		_ai_tick = AI_CALL_EVERY
		_admirals()


## Is the attack order's target still there to fight?
func _target_good(leader: Ship, o: Dictionary, dt: float) -> bool:
	var t: Variant = o["target"]
	if not is_instance_valid(t):
		return false
	if t is Planet:
		if not t.is_colonised() or t.owner_race == leader.race_id or t.is_defenceless():
			o["killed"] = true
			return false
		return true
	if t.dead or t.team() == leader.team() or t.captured:
		o["killed"] = t.dead
		return false
	var seen: bool = t.visible_to(leader)
	if not seen:
		for s in members(leader):
			if not s.dead and t.visible_to(s):
				seen = true
				break
	o["lost"] = 0.0 if seen else float(o["lost"]) + dt
	return o["lost"] < LOST_AFTER


## The empires' senior computer pilots call up wings of their own.
func _admirals() -> void:
	if world.is_arena() or world.practice:
		return
	for s in world.ships:
		if s.dead or s.is_player or s.support or s.is_station or s.is_prize or s.race_id > 2 or s.rank < COMMODORE_RANK:
			continue
		if s.net_id in Net.seats.values():
			continue
		var ai := _ai(s)
		if ai == null or ai.escort != null or ai.convoy_guard:
			continue
		var want := 2 if s.rank >= ADMIRAL_RANK else 1
		if called(s).size() < want:
			call_up(s, want)


# ------------------------------------------------------------ the radio

## A wingman answers: one voice for the wing, on the leader's comms only.
func _ack(leader: Ship, wing: Array, line: String) -> void:
	if not (leader.is_player or leader.net_id in Net.seats.values()) or wing.is_empty():
		return
	var s: Ship = wing[0]
	world.radio(leader, s, line)


func _tell(leader: Ship, text: String) -> void:
	if leader.is_player or leader.net_id in Net.seats.values():
		world.notify(leader, text)


## A wingman speaks up on its own (bingo fuel, back from repairs).
func report(s: Ship, line: String) -> void:
	var ai := _ai(s)
	if ai != null and ai.escort != null and is_instance_valid(ai.escort):
		_ack(ai.escort, [s], line)


# --------------------------------------------------------------- online

## Every wing with someone on it: [leader id, order, target id (or -1 -
## planet index), hold point, [[member id, status], ...]].
func pack() -> Array:
	var leaders := {}
	for s in world.ships:
		var ai := _ai(s)
		if ai != null and ai.escort != null and is_instance_valid(ai.escort) and not ai.convoy_guard:
			leaders[ai.escort] = true
	var rows := []
	for leader in leaders:
		var o := order_of(leader)
		var t: Variant = o.get("target", null)
		var tid := 0
		if is_instance_valid(t) and t is Ship:
			tid = t.net_id
		elif is_instance_valid(t) and t is Planet:
			tid = -1 - world.planets.find(t)
		var ms := []
		for m in members(leader):
			ms.append([m.net_id, status(m)])
		rows.append([leader.net_id, o["order"], tid, hold_point(leader), ms])
	return rows


func unpack(rows: Array) -> void:
	mirror.clear()
	for row in rows:
		var t: Node2D = null
		if row[2] > 0:
			t = Net.ships_by_id.get(row[2], null)
		elif row[2] < 0 and -1 - row[2] < world.planets.size():
			t = world.planets[-1 - row[2]]
		mirror[row[0]] = {"order": row[1], "target": t, "at": row[3], "members": row[4]}


## A wingman went down: its leader hears of it, and the wing answers the
## ship that did it.
func member_lost(s: Ship, killer: Node) -> void:
	var ai := _ai(s)
	if ai == null or ai.escort == null or not is_instance_valid(ai.escort) or ai.convoy_guard:
		return
	var leader: Ship = ai.escort
	var back := not s.is_prize and not s.has_meta("ordered")
	_tell(leader, "Wingman down: %s%s." % [s.pilot_name, " (relaunches at home and rejoins you)" if back else " is lost"])
	if is_instance_valid(killer) and killer is Ship and not killer.dead and killer.team() != leader.team():
		for m in members(leader):
			if m != s and not m.dead and m.global_position.distance_to(killer.global_position) < AIPilot.ESCORT_LEASH * 1.5:
				m.last_attacker = killer
				m.last_attacked_at = Time.get_ticks_msec() / 1000.0
	dirty = true


## The leader's called pilots go back to the empire without a word (the
## person who called them left the game).
func release_called(leader: Node) -> void:
	if leader is Ship:
		for s in called(leader):
			_ai(s).leave_wing()
		orders.erase(leader)
		dirty = true
