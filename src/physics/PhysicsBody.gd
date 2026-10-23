## PhysicsBody -- one rigid body in the game's own 2D physics space
## (see PhysicsSpace).  A body is a convex polygon or a circle with mass,
## rotational inertia, linear and angular velocity, and force and torque
## accumulators that the owning node fills in once per frame.
##
## Positions are world pixels, angles are radians, masses are the module
## masses from GameData (a Scout is about 4500).  The body is plain data:
## the node that owns it copies its state in before the step and out after.
class_name PhysicsBody
extends RefCounted

enum Kind { SHIP, PROJECTILE, DEBRIS, PICKUP, ROCK, PLANET }

var id := 0
var node: Node2D = null
var kind := Kind.SHIP
var enabled := true

# state
var pos := Vector2.ZERO
var vel := Vector2.ZERO
var angle := 0.0
var ang_vel := 0.0
var prev_pos := Vector2.ZERO        # where the body was before the last sub-step

# mass
var mass := 1.0
var inv_mass := 1.0
var inertia := 1.0
var inv_inertia := 1.0
var inertia_per_mass := 1.0         # second moment of the shape per unit mass

# shape
var radius := 10.0                  # bounding (or actual, for a circle) radius
var hull := PackedVector2Array()    # local convex polygon, CCW; empty for a circle
var restitution := 0.2
var friction := 0.3

# per-frame accumulators and flags
var force := Vector2.ZERO
var torque := 0.0
var gravity := Vector2.ZERO         # acceleration from planets, filled by the space
var gravity_scale := 1.0
var sun_gravity := true             # feels the sun's well
var belt_gravity := false           # rocks: the centripetal pull that keeps the belt turning
var ang_damping := 0.0              # 1/s, lets debris settle its tumble
var bullet := false                 # swept against hulls instead of resolved as a contact
var phasing := false                # slips through other bodies (quantum drive)
var kinematic := false              # moved by its node, infinite mass (planets)
var is_sun := false                 # the star
var gm := 0.0                       # planets: the GM their gravity uses
var accel := Vector2.ZERO           # planets: the acceleration of their rail

var _world_hull := PackedVector2Array()
var _world_normals := PackedVector2Array()
var _cache_pos := Vector2.INF
var _cache_angle := INF


func set_mass(m: float) -> void:
	mass = maxf(m, 0.001)
	inv_mass = 0.0 if kinematic else 1.0 / mass
	inertia = mass * inertia_per_mass
	inv_inertia = 0.0 if kinematic else 1.0 / maxf(inertia, 0.001)


func set_kinematic(on: bool) -> void:
	kinematic = on
	set_mass(mass)


## Use a convex polygon (any winding; it is hulled and made CCW).
func set_hull(points: PackedVector2Array) -> void:
	var h := Geometry2D.convex_hull(points)
	# convex_hull closes the loop with a repeat of the first point
	if h.size() > 1 and h[0].is_equal_approx(h[h.size() - 1]):
		h.remove_at(h.size() - 1)
	if _signed_area(h) < 0.0:
		h.reverse()
	hull = h
	radius = 0.0
	for p in hull:
		radius = maxf(radius, p.length())
	inertia_per_mass = _polygon_inertia_per_mass(hull)
	_cache_pos = Vector2.INF
	set_mass(mass)


## Use a circle.
func set_circle(r: float) -> void:
	hull = PackedVector2Array()
	radius = r
	inertia_per_mass = 0.5 * r * r
	set_mass(mass)


func is_circle() -> bool:
	return hull.is_empty()


## The hull in world space, cached while the body does not move.
func world_hull() -> PackedVector2Array:
	_refresh_world()
	return _world_hull


## Outward unit normal of each world-hull edge i (from vertex i to i + 1).
func world_normals() -> PackedVector2Array:
	_refresh_world()
	return _world_normals


func _refresh_world() -> void:
	if _cache_pos == pos and _cache_angle == angle:
		return
	_cache_pos = pos
	_cache_angle = angle
	var n := hull.size()
	_world_hull.resize(n)
	_world_normals.resize(n)
	var c := cos(angle)
	var s := sin(angle)
	for i in n:
		var p := hull[i]
		_world_hull[i] = Vector2(p.x * c - p.y * s, p.x * s + p.y * c) + pos
	for i in n:
		var e := _world_hull[(i + 1) % n] - _world_hull[i]
		_world_normals[i] = Vector2(e.y, -e.x).normalized()


func apply_force(f: Vector2) -> void:
	force += f


func apply_force_at(f: Vector2, world_point: Vector2) -> void:
	force += f
	torque += (world_point - pos).cross(f)


func apply_torque(t: float) -> void:
	torque += t


## An instantaneous change of momentum at a world point: pushes and spins.
func apply_impulse(j: Vector2, world_point: Vector2) -> void:
	if kinematic:
		return
	vel += j * inv_mass
	ang_vel += (world_point - pos).cross(j) * inv_inertia


func velocity_at(world_point: Vector2) -> Vector2:
	var r := world_point - pos
	return vel + Vector2(-ang_vel * r.y, ang_vel * r.x)


func kinetic_energy() -> float:
	return 0.5 * mass * vel.length_squared() + 0.5 * inertia * ang_vel * ang_vel


## Distance from a world point to the body's surface (0 inside).
func surface_distance(p: Vector2) -> float:
	if is_circle():
		return maxf(p.distance_to(pos) - radius, 0.0)
	var wh := world_hull()
	var nm := world_normals()
	var inside := true
	var best := INF
	for i in wh.size():
		var a := wh[i]
		var b := wh[(i + 1) % wh.size()]
		if nm[i].dot(p - a) > 0.0:
			inside = false
		best = minf(best, Geometry2D.get_closest_point_to_segment(p, a, b).distance_to(p))
	return 0.0 if inside else best


## Nearest point on the body's surface to a world point.
func closest_surface_point(p: Vector2) -> Vector2:
	if is_circle():
		return pos + (p - pos).normalized() * radius
	var wh := world_hull()
	var best := Vector2.ZERO
	var best_d := INF
	for i in wh.size():
		var q := Geometry2D.get_closest_point_to_segment(p, wh[i], wh[(i + 1) % wh.size()])
		var d := q.distance_squared_to(p)
		if d < best_d:
			best_d = d
			best = q
	return best


static func _signed_area(poly: PackedVector2Array) -> float:
	var a := 0.0
	for i in poly.size():
		var p := poly[i]
		var q := poly[(i + 1) % poly.size()]
		a += p.x * q.y - q.x * p.y
	return a * 0.5


## Second moment of area about the origin divided by the area, so that
## inertia = mass * this for a uniform plate rotating about the node pivot.
static func _polygon_inertia_per_mass(poly: PackedVector2Array) -> float:
	var num := 0.0
	var area := 0.0
	for i in poly.size():
		var p := poly[i]
		var q := poly[(i + 1) % poly.size()]
		var cr := p.cross(q)
		num += cr * (p.dot(p) + p.dot(q) + q.dot(q))
		area += cr
	if absf(area) < 1e-6:
		return 1.0
	return absf(num / 12.0) / absf(area * 0.5)
