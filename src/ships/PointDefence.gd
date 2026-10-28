## PointDefence -- the ship's PDCs: small gatling turrets fitted as an
## internal part (module 4022), not on a weapon mount, that shoot down
## incoming missiles, seekers, torpedoes, drones and mines on their own
## (Kyle, 2026-10-28: "a ship component").
##
## Each fitted PDC adds one turret on the hull with all-round coverage.
## A turret picks the munition that will reach its ship (or a friend)
## soonest, swings onto it at SLEW and fires a stream of rounds led for
## the intercept.  The rounds are gun shots (missile 022), so the host's
## World._shoot_down_munitions does the hits like any other gun's.
## Turrets spread out over a salvo before doubling up, so a salvo of
## more munitions than turrets is what gets through.
##
## The pilot turns them off with the PDC key (P): the stream of fire
## shows up on heat sensors.  Ammunition is a magazine per turret,
## refilled where launchers rearm.
class_name PointDefence
extends RefCounted

# ------------------------------------------------------------- balance

const ROUND := "022"
## How far out a turret engages, px (the round flies about 430 px).
const REACH := 340.0
## Turrets start swinging onto a threat this far out, before it is in reach.
const TRACK := 700.0
## Seconds between rounds from one turret.
const RATE := 0.1
## Turret swing, radians per second.
const SLEW := 6.0
## A turret fires once its barrels are this close to the lead, radians.
const ON_TARGET := 0.12
## Random spread of each round, radians (a stream, not a rifle).
const SPREAD := 0.1
## Energy per round.
const SHOT_ENERGY := 4.0
## Rounds per turret.
const MAGAZINE := 1200
## A munition aimed at nobody counts as a threat when it will pass within
## this of a friendly hull (px beyond the hull's size).
const THREAT_PASS := 140.0
## Scores of munitions another turret already has are multiplied by this,
## so turrets spread over a salvo.
const CLAIMED := 3.0
## While firing, the turrets' heat shows up once per this many seconds...
const HEAT_GAP := 0.5
## ...at this signature, for HEAT_TIME.
const HEAT := 30.0
const HEAT_TIME := 1.0
## The fire sound plays at most this often.
const SOUND_GAP := 0.2

## [{pos: local Vector2, aim: angle from the nose, cooldown, target, flash}]
var mounts: Array = []
var ammo := 0
var ammo_max := 0
var enabled := true
var shot_down := 0            # munitions these turrets have shot down
var firing := 0.0             # seconds since the last round (HUD)
var _heat_t := 0.0
var _sound_t := 0.0
var _ammo_f := 0.0


## One turret per fitted PDC, at `spots` (the art model's places for them).
func build(n: int, spots: Array, is_station: bool) -> void:
	var old := ammo
	var had := ammo_max
	mounts.clear()
	for i in n:
		var p: Vector2 = spots[i] if i < spots.size() else Vector2.ZERO
		mounts.append({"pos": p, "aim": p.angle() if p != Vector2.ZERO else 0.0, "cooldown": 0.0, "target": null, "flash": 0.0})
	ammo_max = -1 if is_station else n * MAGAZINE
	# a refit keeps what was in the magazines; new turrets come loaded
	ammo = ammo_max if had <= 0 or ammo_max < 0 else mini(old + maxi(ammo_max - had, 0), ammo_max)
	firing = 99.0


func fitted() -> bool:
	return not mounts.is_empty()


func needs_rearm() -> bool:
	return ammo_max > 0 and ammo < ammo_max


func rearm(share: float) -> void:
	if not needs_rearm():
		return
	_ammo_f += ammo_max * share
	var whole := int(_ammo_f)
	if whole > 0:
		ammo = mini(ammo + whole, ammo_max)
		_ammo_f -= whole


## Swing and fire.  `fire` is false for a puppet (a client's copy of
## someone else's ship): its turrets still track, for looks.
func tick(s: Node, delta: float, fire: bool) -> void:
	if mounts.is_empty():
		return
	firing += delta
	_heat_t -= delta
	_sound_t -= delta
	var eff: float = s.systems.efficiency("pdc")
	var live: bool = enabled and eff > 0.0 and not s.warping and not s.is_landing() and not s.systems.is_offline("reactor")
	var claimed := {}
	for mt in mounts:
		mt["cooldown"] = maxf(mt["cooldown"] - delta, 0.0)
		mt["flash"] = maxf(mt["flash"] - delta, 0.0)
		var want: float = mt["pos"].angle() if mt["pos"] != Vector2.ZERO else 0.0
		var m: Node = _pick(s, mt, claimed) if live else null
		mt["target"] = m
		var aim_at := Vector2.INF
		if m != null:
			claimed[m] = true
			aim_at = _lead(s, mt, m)
			if aim_at != Vector2.INF:
				want = wrapf(aim_at.angle() - s.rotation, -PI, PI)
		mt["aim"] = wrapf(rotate_toward(mt["aim"], want, SLEW * maxf(eff, 0.3) * delta), -PI, PI)
		if not fire or m == null or aim_at == Vector2.INF:
			continue
		if m.global_position.distance_to(s.global_position + mt["pos"].rotated(s.rotation)) > REACH:
			continue                # tracking it in, not yet in reach
		if absf(angle_difference(mt["aim"], want)) > ON_TARGET or mt["cooldown"] > 0.0:
			continue
		if ammo == 0 or s.energy < SHOT_ENERGY:
			continue
		_shoot(s, mt, eff)


## The munition this turret should engage, or null.  Lower score first:
## the time it needs to reach its ship, halved for one homing on us,
## raised for one another turret has.
func _pick(s: Node, mt: Dictionary, claimed: Dictionary) -> Node:
	var w: Node = s.world
	if w == null:
		return null
	var muzzle: Vector2 = s.global_position + mt["pos"].rotated(s.rotation)
	var best: Node = null
	var best_score := INF
	var side: int = s.team()
	for m in w.missiles:
		if not is_instance_valid(m) or m.exploded or not m.shootable() or m.owner_team == side:
			continue
		var to: Vector2 = m.global_position - muzzle
		var d := to.length()
		if d > TRACK:
			continue
		var score := _threat_time(s, m)
		if score == INF:
			continue
		if m.target == s:
			score *= 0.5
		if claimed.has(m):
			score *= CLAIMED
		# the one already under fire stays first, so the stream holds on it
		if m == mt["target"]:
			score *= 0.8
		if score < best_score:
			best_score = score
			best = m
	return best


## Seconds until munition `m` reaches this ship or a friend it threatens,
## INF when it threatens nobody here (flying away, or past everyone).
func _threat_time(s: Node, m: Node) -> float:
	var t: Node = m.target
	if t != null and is_instance_valid(t) and t is Ship and not t.dead and t.team() == s.team():
		# homing on us or a friend: as long as it needs to get there
		var dt: float = m.global_position.distance_to(t.global_position)
		return dt / maxf(m.velocity.length(), 60.0)
	if m.data.get("trigger", "") == "prox" and not m.data.get("guided", false):
		# a mine: only once it is close, by how soon we would reach it
		var dm: float = m.global_position.distance_to(s.global_position)
		var close: float = maxf((s.velocity - m.velocity).length(), 40.0)
		return dm / close + 0.5
	# anything else: when it passes nearest to us, if it passes close
	var rel: Vector2 = m.global_position - s.global_position
	var rv: Vector2 = m.velocity - s.velocity
	var vv := rv.length_squared()
	if vv < 1.0:
		return INF
	var tc := -rel.dot(rv) / vv
	if tc < 0.0:
		return INF                  # already going away
	var miss := (rel + rv * tc).length()
	if miss > s.size + THREAT_PASS:
		return INF
	return tc


## Where to point to hit munition `m` with a round from this turret: the
## intercept of the round's straight flight with the munition's, in the
## ship's frame (rounds leave at the ship's velocity plus their own),
## relative to the muzzle.
func _lead(s: Node, mt: Dictionary, m: Node) -> Vector2:
	var muzzle: Vector2 = s.global_position + mt["pos"].rotated(s.rotation)
	var p: Vector2 = m.global_position - muzzle
	var v: Vector2 = m.velocity - s.velocity
	var sp: float = GameData.MISSILES[ROUND]["fire_v"] * GameData.LIGHT_SPEED
	# |p + v t| = sp t  ->  (v.v - sp^2) t^2 + 2 p.v t + p.p = 0
	var a := v.dot(v) - sp * sp
	var b := 2.0 * p.dot(v)
	var c := p.dot(p)
	var t := -1.0
	if absf(a) < 0.001:
		t = -c / b if absf(b) > 0.001 else -1.0
	else:
		var disc := b * b - 4.0 * a * c
		if disc >= 0.0:
			var r := sqrt(disc)
			var t1 := (-b - r) / (2.0 * a)
			var t2 := (-b + r) / (2.0 * a)
			t = minf(t1, t2) if minf(t1, t2) > 0.0 else maxf(t1, t2)
	if t <= 0.0:
		return p                    # no intercept (it outruns the round): point at it
	return p + v * t


func _shoot(s: Node, mt: Dictionary, eff: float) -> void:
	mt["cooldown"] = RATE / maxf(eff, 0.25)
	mt["flash"] = 0.05
	firing = 0.0
	s.energy -= SHOT_ENERGY
	if ammo > 0:
		ammo -= 1
	if _heat_t <= 0.0:
		_heat_t = HEAT_GAP
		s.add_signature("heat", HEAT, HEAT_TIME)
	if Net.is_client():
		return                      # the host fires it and tells everyone (Net plays the sound)
	if _sound_t <= 0.0:
		_sound_t = SOUND_GAP
		Sfx.play("rapid", s.global_position, 0.3, randf_range(1.15, 1.3))
	var md: Dictionary = GameData.MISSILES[ROUND]
	var ang: float = s.rotation + mt["aim"] + randf_range(-SPREAD, SPREAD)
	var dir := Vector2.RIGHT.rotated(ang)
	var muzzle: Vector2 = s.global_position + mt["pos"].rotated(s.rotation) + dir * 5.0
	s.world.spawn_missile(s, ROUND, muzzle, s.velocity + dir * md["fire_v"] * GameData.LIGHT_SPEED, ang, null)


## Live aims (angle from the nose) for drawing the turrets.
func aims() -> Array:
	var out := []
	for mt in mounts:
		out.append(mt["aim"])
	return out

