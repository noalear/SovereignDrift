## PhysicsSpace -- the remake's own rigid-body simulation for everything
## that flies: ships, projectiles, wreckage, asteroids and floating modules.
##
## Why not Godot's RigidBody2D?  The game needs a deterministic, replayable
## step (the host-authoritative netcode re-simulates it for prediction), soft
## speed limits and warp that do not fight a black-box solver, planets that
## move on rails, and a few hundred bodies at most.  A hand-written space
## gives all of that in one file and stays easy to tune.
##
## The step is semi-implicit Euler in two sub-steps, a spatial-hash
## broad phase, SAT with face clipping for polygon contacts, sequential
## impulses with restitution and Coulomb friction, Baumgarte position
## correction, and swept segments for fast projectiles.
##
## Gravity is Newtonian but local.  Each planet is a 1/r^2 well that
## reaches a few radii out and then fades, and its moons ride Kepler rails
## computed from the same GM, so a ship that matches its planet's speed is
## truly in orbit and a moon's path is the path a ship would take.  The sun
## is a deep well of its own that fades out well short of Mercury: the
## scenario packs the planets far too close for a real heliocentric field,
## which would drag every idle ship into the sun within minutes.  The
## asteroid belt rides a centripetal field that keeps it turning with its
## minor bodies.
class_name PhysicsSpace
extends RefCounted

const CELL := 320.0
const SUBSTEPS := 2
const SOLVER_ITERATIONS := 6
const SLOP := 0.4
const CORRECTION := 0.35
const RESTITUTION_THRESHOLD := 25.0   # below this closing speed contacts do not bounce
const MAX_GRAVITY := 400.0
## The sun's GM: 283 px/s^2 at its surface, 50 at 1000 px, fading to nothing
## at SUN_REACH.
const SUN_GM := 5.0e7
const SUN_REACH := 2200.0
const PLANET_SURFACE_G := 70.0        # px/s^2 at the surface, plus 0.45 per px of radius
## A planet's gravity reaches this many radii out, fading over the last
## fifth.  Short on purpose: the Sol scenario packs the belt right under
## Jupiter, and a long reach would sweep it clean.
const REACH_RADII := 6.0
const MAX_REACH := 600.0
const FADE := 0.2

var bodies: Array[PhysicsBody] = []
var planet_bodies: Array[PhysicsBody] = []
var belt_omega := 0.0                 # rad/s: the asteroid belt turns as one
var sun_pos := Vector2.ZERO
## (a: PhysicsBody, b: PhysicsBody) -> bool; decides which pairs may touch.
var filter: Callable

## Results of the last step, for the world to turn into damage and sound.
## contacts: {a, b, point, normal, speed, impulse}  speed = closing speed before the solve
var contacts: Array = []
## bullet_hits: {bullet, body, point, normal, t}  first thing each bullet ran into
var bullet_hits: Array = []
var step_time_ms := 0.0

var _next_id := 1
var _grid := {}
var _wells: Array[PhysicsBody] = []    # planets that pull, refreshed each step


func add_body(b: PhysicsBody) -> void:
	if b.id == 0:
		b.id = _next_id
		_next_id += 1
	if b.kind == PhysicsBody.Kind.PLANET:
		b.set_kinematic(true)
		if not planet_bodies.has(b):
			planet_bodies.append(b)
	elif not bodies.has(b):
		bodies.append(b)


func remove_body(b: PhysicsBody) -> void:
	bodies.erase(b)
	planet_bodies.erase(b)


# ------------------------------------------------------------------ step

func step(delta: float) -> void:
	var t0 := Time.get_ticks_usec()
	contacts.clear()
	bullet_hits.clear()
	_wells.clear()
	for p in planet_bodies:
		if p.gm > 0.0:
			_wells.append(p)
	var h := delta / SUBSTEPS
	var pairs: Array = []
	for i in SUBSTEPS:
		_compute_gravity()
		_integrate(h)
		if i == 0:
			# the broad phase runs once per frame, with the frame's motion as margin
			pairs = _broadphase(delta)
		_narrowphase(pairs, h)
	for b in bodies:
		b.force = Vector2.ZERO
		b.torque = 0.0
	step_time_ms = (Time.get_ticks_usec() - t0) / 1000.0


func _compute_gravity() -> void:
	for b in bodies:
		if not b.enabled or b.kinematic:
			continue
		b.gravity = gravity_at(b.pos, b.sun_gravity, b.belt_gravity) * b.gravity_scale


## Acceleration from the planets at a point.
func gravity_at(pos: Vector2, with_sun: bool = true, in_belt: bool = false) -> Vector2:
	var g := Vector2.ZERO
	if in_belt:
		# what a circular path at the belt's angular speed needs
		g += (sun_pos - pos) * (belt_omega * belt_omega)
	var dom: PhysicsBody = null
	var dom_strength := 0.0
	var dom_fade := 0.0
	if _wells.is_empty():
		for p in planet_bodies:
			if p.gm > 0.0:
				_wells.append(p)
	for p in _wells:
		var to := p.pos - pos
		var reach := reach_of(p)
		if absf(to.x) > reach or absf(to.y) > reach:
			continue
		var d2 := to.length_squared()
		if d2 < 1.0 or d2 > reach * reach:
			continue
		if p.is_sun and not with_sun:
			continue
		var d := sqrt(d2)
		var strength := p.gm / maxf(d2, p.radius * p.radius)
		# fade out at the edge of the well so there is no seam
		var fade := clampf((reach - d) / (reach * FADE), 0.0, 1.0)
		strength *= fade
		g += to * (strength / d)
		if strength > dom_strength:
			dom_strength = strength
			dom = p
			dom_fade = fade
	# A planet rides a rail that bends; whatever sits in its well is carried
	# along with it, less any pull from other wells that it feels directly,
	# so an orbit round a moving planet closes instead of unwinding.
	if dom != null and not dom.is_sun:
		g += (dom.accel - _wells_at(dom.pos, dom)) * dom_fade
	if g.length_squared() > MAX_GRAVITY * MAX_GRAVITY:
		g = g.normalized() * MAX_GRAVITY
	return g


## Pull of every well but `skip` at a point, with no frame terms.
func _wells_at(pos: Vector2, skip: PhysicsBody) -> Vector2:
	var g := Vector2.ZERO
	for p in _wells:
		if p == skip:
			continue
		var to := p.pos - pos
		var reach := reach_of(p)
		if absf(to.x) > reach or absf(to.y) > reach:
			continue
		var d2 := to.length_squared()
		if d2 < 1.0 or d2 > reach * reach:
			continue
		var d := sqrt(d2)
		var strength := p.gm / maxf(d2, p.radius * p.radius) * clampf((reach - d) / (reach * FADE), 0.0, 1.0)
		g += to * (strength / d)
	return g


## How far a body's gravity reaches.
static func reach_of(p: PhysicsBody) -> float:
	return SUN_REACH if p.is_sun else minf(p.radius * REACH_RADII, MAX_REACH)


## GM of a star, planet or moon from its look and size.  Minor bodies
## (the belt's asteroids) are too light to pull at all.
static func gm_for(look: String, body_class: String, radius: float) -> float:
	if look == "Sun":
		return SUN_GM
	if body_class == "Minor":
		return 0.0
	return (PLANET_SURFACE_G + 0.45 * radius) * radius * radius


## Speed of a circular orbit around a planet body at distance d.
func orbital_speed(p: PhysicsBody, d: float) -> float:
	return sqrt(p.gm / maxf(d, p.radius))


## The velocity that keeps something at `pos` on a circular orbit around a
## planet body, prograde, including the planet's own motion.  Outside the
## body's well it simply moves with the body.
func circular_orbit_velocity(p: PhysicsBody, pos: Vector2) -> Vector2:
	var r := pos - p.pos
	var d := r.length()
	if d < 1.0 or p.gm <= 0.0 or d > reach_of(p) * (1.0 - FADE):
		return p.vel
	return p.vel + Vector2(-r.y, r.x) / d * orbital_speed(p, d)


## The velocity of the belt's rigid rotation at a point.
func belt_velocity(pos: Vector2) -> Vector2:
	var r := pos - sun_pos
	return Vector2(-r.y, r.x) * belt_omega


## The body whose well a point sits in (the strongest pull), or null in
## open space.  Used as the local rest frame.
func dominant_body(pos: Vector2) -> PhysicsBody:
	var best: PhysicsBody = null
	var best_g := 0.0
	for p in planet_bodies:
		if p.gm <= 0.0:
			continue
		var d2 := pos.distance_squared_to(p.pos)
		var reach := reach_of(p)
		if d2 > reach * reach:
			continue
		var g := p.gm / maxf(d2, 1.0)
		if g > best_g:
			best_g = g
			best = p
	return best


func _integrate(h: float) -> void:
	for b in bodies:
		if not b.enabled:
			continue
		b.prev_pos = b.pos
		if b.kinematic:
			continue
		b.vel += (b.force * b.inv_mass + b.gravity) * h
		b.ang_vel += b.torque * b.inv_inertia * h
		if b.ang_damping > 0.0:
			b.ang_vel *= maxf(1.0 - b.ang_damping * h, 0.0)
		b.pos += b.vel * h
		b.angle += b.ang_vel * h


# ---------------------------------------------------------- broad phase

func _cell_range(lo: Vector2, hi: Vector2) -> Rect2i:
	var x0 := floori(lo.x / CELL)
	var y0 := floori(lo.y / CELL)
	var x1 := floori(hi.x / CELL)
	var y1 := floori(hi.y / CELL)
	return Rect2i(x0, y0, x1 - x0 + 1, y1 - y0 + 1)


func _insert(b: PhysicsBody, lo: Vector2, hi: Vector2) -> void:
	var r := _cell_range(lo, hi)
	for y in range(r.position.y, r.end.y):
		for x in range(r.position.x, r.end.x):
			var key := Vector2i(x, y)
			if _grid.has(key):
				_grid[key].append(b)
			else:
				_grid[key] = [b]


## Candidate pairs from the spatial hash, filtered: [[a, b], ...]
func _broadphase(delta: float) -> Array:
	_grid.clear()
	for p in planet_bodies:
		_insert(p, p.pos - Vector2.ONE * p.radius, p.pos + Vector2.ONE * p.radius)
	for b in bodies:
		if not b.enabled:
			continue
		var r := Vector2.ONE * b.radius
		var lo := b.pos
		var hi := b.pos
		if b.bullet:
			lo = Vector2(minf(b.prev_pos.x, b.pos.x), minf(b.prev_pos.y, b.pos.y))
			hi = Vector2(maxf(b.prev_pos.x, b.pos.x), maxf(b.prev_pos.y, b.pos.y))
			# the rest of the frame's travel
			var ahead := b.pos + b.vel * delta
			lo = Vector2(minf(lo.x, ahead.x), minf(lo.y, ahead.y))
			hi = Vector2(maxf(hi.x, ahead.x), maxf(hi.y, ahead.y))
		else:
			var m := b.vel.abs() * delta
			lo -= m
			hi += m
		_insert(b, lo - r, hi + r)
	var seen := {}
	var pairs: Array = []
	for cell in _grid.values():
		var n: int = cell.size()
		if n < 2:
			continue
		for i in n:
			var a: PhysicsBody = cell[i]
			for j in range(i + 1, n):
				var b: PhysicsBody = cell[j]
				if a.kinematic and b.kinematic:
					continue
				if a.bullet and b.bullet:
					continue
				if a.phasing or b.phasing:
					continue
				var key: int = (a.id << 20) | b.id if a.id < b.id else (b.id << 20) | a.id
				if seen.has(key):
					continue
				seen[key] = true
				if filter.is_valid() and not filter.call(a, b):
					continue
				pairs.append([a, b])
	return pairs


func _narrowphase(pairs: Array, h: float) -> void:
	var manifolds: Array = []
	var best_bullet := {}     # bullet id -> hit
	for pair in pairs:
		var a: PhysicsBody = pair[0]
		var b: PhysicsBody = pair[1]
		if not a.enabled or not b.enabled:
			continue
		if a.bullet or b.bullet:
			var bullet := a if a.bullet else b
			var body := b if a.bullet else a
			var hit := _sweep(bullet, body)
			if not hit.is_empty():
				var prev = best_bullet.get(bullet.id)
				if prev == null or hit["t"] < prev["t"]:
					best_bullet[bullet.id] = hit
			continue
		var d2 := a.pos.distance_squared_to(b.pos)
		var rr := a.radius + b.radius
		if d2 > rr * rr:
			continue
		var m := _manifold(a, b)
		if not m.is_empty():
			manifolds.append(m)
	for hit in best_bullet.values():
		# a bullet reports once per step: the earliest hit of the first sub-step that touched
		var dup := false
		for existing in bullet_hits:
			if existing["bullet"] == hit["bullet"]:
				dup = true
				break
		if not dup:
			bullet_hits.append(hit)
	_solve(manifolds, h)


# ---------------------------------------------------------- narrow phase

## Contact manifold between two non-bullet bodies: {a, b, normal, points: [[p, pen], ...]}
func _manifold(a: PhysicsBody, b: PhysicsBody) -> Dictionary:
	if a.is_circle() and b.is_circle():
		var d := b.pos - a.pos
		var dist := d.length()
		var rr := a.radius + b.radius
		if dist >= rr:
			return {}
		var n := d / dist if dist > 1e-6 else Vector2.RIGHT
		return {"a": a, "b": b, "normal": n, "points": [[a.pos + n * (a.radius - (rr - dist) * 0.5), rr - dist]]}
	if a.is_circle():
		var m := _circle_polygon(a, b)
		return m
	if b.is_circle():
		var m := _circle_polygon(b, a)
		if m.is_empty():
			return m
		m["a"] = a
		m["b"] = b
		m["normal"] = -m["normal"]
		return m
	return _polygon_polygon(a, b)


## Circle c against polygon p; the normal points from the circle to the polygon.
func _circle_polygon(c: PhysicsBody, p: PhysicsBody) -> Dictionary:
	var verts := p.world_hull()
	var norms := p.world_normals()
	var n := verts.size()
	var best_sep := -INF
	var best := 0
	for i in n:
		var sep := norms[i].dot(c.pos - verts[i])
		if sep > c.radius:
			return {}
		if sep > best_sep:
			best_sep = sep
			best = i
	var v1 := verts[best]
	var v2 := verts[(best + 1) % n]
	var face_n := norms[best]
	if best_sep < 0.0:
		# centre inside: push out along the least-penetrated face
		return {"a": c, "b": p, "normal": -face_n, "points": [[c.pos + face_n * best_sep, c.radius - best_sep]]}
	# which Voronoi region of the face is the centre in?
	var edge := v2 - v1
	var t := (c.pos - v1).dot(edge) / maxf(edge.length_squared(), 1e-9)
	if t >= 0.0 and t <= 1.0:
		return {"a": c, "b": p, "normal": -face_n, "points": [[c.pos - face_n * c.radius, c.radius - best_sep]]}
	var v := v1 if t < 0.0 else v2
	var d := c.pos - v
	var dist := d.length()
	if dist >= c.radius:
		return {}
	var nn := d / dist if dist > 1e-6 else face_n
	return {"a": c, "b": p, "normal": -nn, "points": [[v, c.radius - dist]]}


static func _max_separation(va: PackedVector2Array, na: PackedVector2Array, vb: PackedVector2Array) -> Array:
	var best := -INF
	var idx := 0
	for i in va.size():
		var nrm := na[i]
		var v := va[i]
		var sep := INF
		for w in vb:
			var d := nrm.dot(w - v)
			if d < sep:
				sep = d
		if sep > best:
			best = sep
			idx = i
	return [best, idx]


static func _clip(p1: Vector2, p2: Vector2, nn: Vector2, c: float) -> Array:
	var out := []
	var d1 := nn.dot(p1) - c
	var d2 := nn.dot(p2) - c
	if d1 <= 0.0:
		out.append(p1)
	if d2 <= 0.0:
		out.append(p2)
	if d1 * d2 < 0.0:
		out.append(p1 + (p2 - p1) * (d1 / (d1 - d2)))
	return out


## SAT with reference-face clipping (the Box2D-lite scheme).
func _polygon_polygon(a: PhysicsBody, b: PhysicsBody) -> Dictionary:
	var va := a.world_hull()
	var na := a.world_normals()
	var vb := b.world_hull()
	var nb := b.world_normals()
	var sa := _max_separation(va, na, vb)
	if sa[0] > 0.0:
		return {}
	var sb := _max_separation(vb, nb, va)
	if sb[0] > 0.0:
		return {}
	var flip: bool = sb[0] > sa[0] + 0.05
	var rv := vb if flip else va
	var rn := nb if flip else na
	var iv := va if flip else vb
	var inn := na if flip else nb
	var ref_i: int = sb[1] if flip else sa[1]
	var n := rn[ref_i]
	var v1 := rv[ref_i]
	var v2 := rv[(ref_i + 1) % rv.size()]
	# incident face: the one facing the reference normal most directly
	var inc := 0
	var min_dot := INF
	for i in inn.size():
		var d := inn[i].dot(n)
		if d < min_dot:
			min_dot = d
			inc = i
	var i1 := iv[inc]
	var i2 := iv[(inc + 1) % iv.size()]
	var t := (v2 - v1).normalized()
	var clipped := _clip(i1, i2, -t, -t.dot(v1))
	if clipped.size() < 2:
		return {}
	clipped = _clip(clipped[0], clipped[1], t, t.dot(v2))
	if clipped.size() < 2:
		return {}
	var points := []
	for p in clipped:
		var sep: float = n.dot(p - v1)
		if sep <= 0.0:
			points.append([p, -sep])
	if points.is_empty():
		return {}
	return {"a": a, "b": b, "normal": -n if flip else n, "points": points}


## Swept test of a bullet's last sub-step against a body: {bullet, body, point, normal, t}
func _sweep(bullet: PhysicsBody, body: PhysicsBody) -> Dictionary:
	var p0 := bullet.prev_pos
	var p1 := bullet.pos
	var seg := p1 - p0
	var inflate := bullet.radius
	# bounding circle first
	var t_circ := _ray_circle(p0, seg, body.pos, body.radius + inflate)
	if t_circ < 0.0 or t_circ > 1.0:
		return {}
	if body.is_circle():
		var hp := p0 + seg * t_circ
		var nn := (hp - body.pos).normalized()
		return {"bullet": bullet, "body": body, "point": body.pos + nn * body.radius, "normal": nn, "t": t_circ}
	var verts := body.world_hull()
	var norms := body.world_normals()
	var n := verts.size()
	# already inside at the start of the sub-step
	if body.surface_distance(p0) <= inflate:
		var cp := body.closest_surface_point(p0)
		return {"bullet": bullet, "body": body, "point": cp, "normal": (p0 - cp).normalized() if not cp.is_equal_approx(p0) else -seg.normalized(), "t": 0.0}
	var best_t := INF
	var best_i := -1
	var best_p := Vector2.ZERO
	for i in n:
		var a := verts[i] + norms[i] * inflate
		var b := verts[(i + 1) % n] + norms[i] * inflate
		var hit = Geometry2D.segment_intersects_segment(p0, p1, a, b)
		if hit != null:
			var t: float = (hit - p0).length() / maxf(seg.length(), 1e-6)
			if t < best_t:
				best_t = t
				best_i = i
				best_p = hit
	if best_i >= 0:
		return {"bullet": bullet, "body": body, "point": best_p - norms[best_i] * inflate, "normal": norms[best_i], "t": best_t}
	# grazing a corner: end point within the inflation distance
	if body.surface_distance(p1) <= inflate:
		var cp := body.closest_surface_point(p1)
		return {"bullet": bullet, "body": body, "point": cp, "normal": (p1 - cp).normalized(), "t": 1.0}
	return {}


static func _ray_circle(o: Vector2, d: Vector2, c: Vector2, r: float) -> float:
	# returns the parameter along d (0..1 for a segment) of the first touch, or -1
	var oc := o - c
	if oc.length_squared() <= r * r:
		return 0.0
	var a := d.length_squared()
	if a < 1e-9:
		return -1.0
	var b := 2.0 * oc.dot(d)
	var cc := oc.length_squared() - r * r
	var disc := b * b - 4.0 * a * cc
	if disc < 0.0:
		return -1.0
	var t := (-b - sqrt(disc)) / (2.0 * a)
	return t if t >= 0.0 else -1.0


## Distance along a ray (origin, unit dir) to the body's surface, or -1.
func ray_body(origin: Vector2, dir: Vector2, body: PhysicsBody, max_len: float) -> float:
	var t_circ := _ray_circle(origin, dir * max_len, body.pos, body.radius)
	if t_circ < 0.0 or t_circ > 1.0:
		return -1.0
	if body.is_circle():
		return t_circ * max_len
	if body.surface_distance(origin) <= 0.0:
		return 0.0
	var verts := body.world_hull()
	var n := verts.size()
	var best := INF
	var far := origin + dir * max_len
	for i in n:
		var hit = Geometry2D.segment_intersects_segment(origin, far, verts[i], verts[(i + 1) % n])
		if hit != null:
			best = minf(best, (hit - origin).length())
	return best if best < INF else -1.0


# ---------------------------------------------------------------- solver

func _solve(manifolds: Array, h: float) -> void:
	var cs: Array = []
	for m in manifolds:
		var a: PhysicsBody = m["a"]
		var b: PhysicsBody = m["b"]
		var n: Vector2 = m["normal"]
		var e := maxf(a.restitution, b.restitution)
		var mu := sqrt(a.friction * b.friction)
		var pair_speed := 0.0
		for pt in m["points"]:
			var p: Vector2 = pt[0]
			var ra := p - a.pos
			var rb := p - b.pos
			var rna := ra.cross(n)
			var rnb := rb.cross(n)
			var k := a.inv_mass + b.inv_mass + rna * rna * a.inv_inertia + rnb * rnb * b.inv_inertia
			var t := Vector2(-n.y, n.x)
			var rta := ra.cross(t)
			var rtb := rb.cross(t)
			var kt := a.inv_mass + b.inv_mass + rta * rta * a.inv_inertia + rtb * rtb * b.inv_inertia
			var vn := (b.velocity_at(p) - a.velocity_at(p)).dot(n)
			var bounce := 0.0
			if vn < -RESTITUTION_THRESHOLD:
				bounce = -e * vn
			pair_speed = maxf(pair_speed, -vn)
			cs.append({"a": a, "b": b, "n": n, "t": t, "ra": ra, "rb": rb, "mn": 1.0 / maxf(k, 1e-9),
				"mt": 1.0 / maxf(kt, 1e-9), "bounce": bounce, "jn": 0.0, "jt": 0.0, "pen": pt[1], "mu": mu, "p": p})
		m["speed"] = pair_speed
	for _it in SOLVER_ITERATIONS:
		for c in cs:
			var a: PhysicsBody = c["a"]
			var b: PhysicsBody = c["b"]
			var n: Vector2 = c["n"]
			var ra: Vector2 = c["ra"]
			var rb: Vector2 = c["rb"]
			# normal
			var rv := b.velocity_at(b.pos + rb) - a.velocity_at(a.pos + ra)
			var vn := rv.dot(n)
			var lambda: float = -(vn - c["bounce"]) * c["mn"]
			var old: float = c["jn"]
			c["jn"] = maxf(old + lambda, 0.0)
			lambda = c["jn"] - old
			_apply_pair(a, b, n * lambda, ra, rb)
			# friction
			rv = b.velocity_at(b.pos + rb) - a.velocity_at(a.pos + ra)
			var t: Vector2 = c["t"]
			var vt := rv.dot(t)
			var lt: float = -vt * c["mt"]
			var limit: float = c["mu"] * c["jn"]
			var oldt: float = c["jt"]
			c["jt"] = clampf(oldt + lt, -limit, limit)
			lt = c["jt"] - oldt
			_apply_pair(a, b, t * lt, ra, rb)
	# push apart what still overlaps
	for c in cs:
		var a: PhysicsBody = c["a"]
		var b: PhysicsBody = c["b"]
		var total := a.inv_mass + b.inv_mass
		if total <= 0.0:
			continue
		var corr: float = maxf(c["pen"] - SLOP, 0.0) * CORRECTION / total
		var n: Vector2 = c["n"]
		a.pos -= n * corr * a.inv_mass
		b.pos += n * corr * b.inv_mass
	# report one event per pair
	for m in manifolds:
		var jn := 0.0
		var point: Vector2 = m["points"][0][0]
		for c in cs:
			if c["a"] == m["a"] and c["b"] == m["b"]:
				jn += c["jn"]
		contacts.append({"a": m["a"], "b": m["b"], "point": point, "normal": m["normal"], "speed": m["speed"], "impulse": jn})


static func _apply_pair(a: PhysicsBody, b: PhysicsBody, j: Vector2, ra: Vector2, rb: Vector2) -> void:
	if not a.kinematic:
		a.vel -= j * a.inv_mass
		a.ang_vel -= ra.cross(j) * a.inv_inertia
	if not b.kinematic:
		b.vel += j * b.inv_mass
		b.ang_vel += rb.cross(j) * b.inv_inertia


# --------------------------------------------------------------- queries

## Every enabled body of the given kinds whose surface is within range of a point.
func bodies_near(pos: Vector2, range_px: float, kinds: Array) -> Array:
	var out := []
	for b in bodies:
		if not b.enabled or not kinds.has(b.kind):
			continue
		if b.pos.distance_to(pos) - b.radius > range_px:
			continue
		out.append(b)
	return out


## Explosion: a radial impulse that falls off linearly to zero at `range_px`.
func blast(pos: Vector2, range_px: float, impulse: float, kinds: Array) -> void:
	for b: PhysicsBody in bodies_near(pos, range_px, kinds):
		var surf: Vector2 = b.closest_surface_point(pos)
		var d := maxf(surf.distance_to(pos), 0.0)
		var f := 1.0 - clampf(d / range_px, 0.0, 1.0)
		if f <= 0.0:
			continue
		var dir := (b.pos - pos)
		if dir.length_squared() < 1e-6:
			dir = Vector2.RIGHT
		dir = dir.normalized()
		# hit the near face so the blast also spins what it catches off-centre
		b.apply_impulse(dir * impulse * f, surf.lerp(b.pos, 0.5))
