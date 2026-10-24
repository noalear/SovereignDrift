## World -- the Sol system scenario: bodies, ships, missiles, pickups,
## asteroids, the physics step, scoring, respawns and planet
## capture.
class_name World
extends Node2D

signal message(text: String, color: Color)
signal player_died()
signal player_respawned()
signal planet_owner_changed(planet: Planet)
signal shipyard_requested(ship: Ship, base: Ship)
signal round_over(result: Dictionary)

const MISSILE_SCRIPT := preload("res://src/weapons/Missile.gd")
const RESPAWN_DELAY := 4.0
## Computer pilots take longer to relaunch, pirates longer still, and none
## of them ever appears inside this distance of the player: the next one
## arrives from out of view, not on top of you.
const AI_RESPAWN_DELAY := 15.0
## Pirates are a rare visitor (Kyle: "super rare"): one raider, first seen
## 6 to 12 minutes in, back 10 to 18 minutes after each loss.
const PIRATE_FIRST := Vector2(360.0, 720.0)
const PIRATE_RETURN := Vector2(600.0, 1080.0)
const SPAWN_CLEAR := 2600.0
## Momentum carried per point of weapon damage (mass units * px/s): a
## torpedo nudges a Scout by about 6 px/s, a mine throws it at 70.
const IMPULSE_PER_DAMAGE := 180.0
## Closing speed below which a bump does no damage.
const IMPACT_THRESHOLD := 60.0

var space: PhysicsSpace
var planets: Array[Planet] = []
var ships: Array[Ship] = []
var missiles: Array = []
var powerups: Array = []
var debris: Array = []           # Debris nodes: the belt's rocks
var sun: Planet = null
var camera: Camera2D
var fx: Fx
var starfield: Starfield
var player: Ship = null
var paused := false
var elapsed := 0.0
var respawn_queue: Array = []   # [ship, time]
var planet_by_name := {}
var inspected_planet: Planet = null   # the world on the player's info card (shift-click)
var bodies_layer: Node2D
var debris_layer: Node2D
var ships_layer: Node2D
var missiles_layer: Node2D
var pickups_layer: Node2D
var planet_counts := {}
var game_over := false
var target_zoom := 1.0
var shake := 0.0                 # camera shake left, in pixels
# The camera moves on physics ticks like everything it watches, and the
# engine's physics interpolation draws both between ticks; the last two
# tick positions are kept for the starfield, which follows it by hand.
var cam_prev := Vector2.ZERO
var cam_cur := Vector2.ZERO
var slowmo := 0.0                # seconds of kill slow-motion left
var intensity_timer := 0.0
var last_player_damage := -100.0
var last_player_fire := -100.0
var income_timer := 60.0         # planets pay their empire every minute
var round_rules: Round = null     # the Conquest rules; the host runs them
var capture: Capture = null       # boarding and prizes; the host runs them
var practice := false            # Practice duel: you, your starbase, one pirate at a time
var practice_opponent := true    # false: Free flight, practice with no pirate at all
var duel_score := [0, 0]         # practice: [your kills, their kills]
const START_CREDITS := 250
const SUN_HEAT_RADII := 4.0       # the heat zone reaches this many Sun radii above the surface
const BASE_ZOOM := 1.3            # "Normal" camera: ships read clearly at 1080p and 4K
var sun_heat := 0.0              # 0..1, how close the player is to burning in the Sun
var _heat_alarm := 0.0


func _ready() -> void:
	bodies_layer = Node2D.new()
	bodies_layer.name = "Bodies"
	add_child(bodies_layer)
	pickups_layer = Node2D.new()
	pickups_layer.name = "Pickups"
	add_child(pickups_layer)
	debris_layer = Node2D.new()
	debris_layer.name = "Debris"
	add_child(debris_layer)
	ships_layer = Node2D.new()
	ships_layer.name = "Ships"
	add_child(ships_layer)
	missiles_layer = Node2D.new()
	missiles_layer.name = "Missiles"
	add_child(missiles_layer)
	fx = Fx.new()
	fx.name = "Fx"
	fx.z_index = 5
	add_child(fx)
	starfield = Starfield.new()
	add_child(starfield)
	camera = Camera2D.new()
	camera.name = "Camera"
	camera.process_callback = Camera2D.CAMERA2D_PROCESS_PHYSICS
	add_child(camera)
	starfield.camera = camera
	starfield.world = self
	target_zoom = Settings.camera_zoom
	camera.zoom = Vector2.ONE * target_zoom * BASE_ZOOM
	space = PhysicsSpace.new()
	space.filter = Callable(self, "_can_collide")
	capture = Capture.new(self)
	_build_system()
	_spawn_rocks()
	if Net.is_client():
		# the ships come from the host, and so does the round: this Round
		# never ticks, Net fills its fields for the HUD and the map
		round_rules = Round.new()
		round_rules.world = self
		Net.client_attach(self)
	elif practice:
		_spawn_practice()
	else:
		_spawn_pilots()
		round_rules = Round.new()
		round_rules.setup(self, Settings.round_minutes)
		round_rules.finished.connect(_on_round_finished)
		if Net.is_host():
			Net.host_attach(self)
	camera.make_current()


# --------------------------------------------------------------- set-up

func _build_system() -> void:
	var seed_base := 1997
	for i in GameData.SOL.size():
		var row: Array = GameData.SOL[i]
		var parent: Planet = planet_by_name.get(row[3], null)
		var p := Planet.new()
		p.name = row[1]
		bodies_layer.add_child(p)
		p.setup(self, row, parent, seed_base + i * 13)
		planets.append(p)
		planet_by_name[row[1]] = p
	for name in GameData.COLONIES:
		var c: Array = GameData.COLONIES[name]
		var p: Planet = planet_by_name[name]
		p.set_colony(c[0], c[1], c[2])
	# settle orbits so moons sit in place
	for p in planets:
		p.update_orbit(0.0)
		if p.look == "Sun":
			sun = p
		var b := PhysicsBody.new()
		b.kind = PhysicsBody.Kind.PLANET
		b.node = p
		b.set_circle(p.radius)
		b.is_sun = p.look == "Sun"
		b.gm = PhysicsSpace.gm_for(p.look, p.body_class, p.radius)
		b.restitution = 0.3
		b.friction = 0.5
		b.pos = p.global_position
		b.vel = p.velocity
		b.accel = p.accel
		p.body = b
		space.add_body(b)
	var belt: Planet = planet_by_name.get("Ceres", null)
	if belt != null:
		space.belt_omega = belt.orbit_speed
	space.sun_pos = sun.global_position


## The asteroid belt: clumps of rock in a ring between Mars's and
## Jupiter's gravity, turning with the belt's minor bodies.
func _spawn_rocks() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 4242
	var band_lo := 6340.0
	var band_hi := 6520.0
	var centres := []
	for p in planets:
		if p.body_class == "Minor" and p.orbit_radius >= band_lo - 150.0 and p.orbit_radius <= band_hi + 150.0:
			centres.append(p.global_position.normalized() * (band_lo + band_hi) * 0.5)
	while centres.size() < 12:
		var a := rng.randf() * TAU
		centres.append(Vector2(cos(a), sin(a)) * (band_lo + band_hi) * 0.5)
	var placed := []
	for c in centres:
		var n := 7 + rng.randi() % 4
		for i in n:
			var r := rng.randf_range(6.0, 22.0)
			var pos := Vector2.ZERO
			var ok := false
			for attempt in 12:
				var a := rng.randf() * TAU
				pos = c + Vector2(cos(a), sin(a)) * rng.randf_range(0.0, 520.0)
				pos = pos.normalized() * clampf(pos.length(), band_lo, band_hi)
				ok = true
				for q in placed:
					if pos.distance_to(q[0]) < r + q[1] + 12.0:
						ok = false
						break
				for p in planets:
					if p.look != "Sun" and pos.distance_to(p.global_position) < PhysicsSpace.reach_of(p.body) + p.radius + 60.0:
						ok = false
						break
				if ok:
					break
			if not ok:
				continue
			placed.append([pos, r])
			var vel := space.belt_velocity(pos) + Vector2(rng.randf_range(-1.0, 1.0), rng.randf_range(-1.0, 1.0))
			spawn_rock(pos, vel, r, rng.randi())


func spawn_rock(pos: Vector2, vel: Vector2, r: float, seed_value: int) -> Debris:
	var d := Debris.new()
	debris_layer.add_child(d)
	d.setup_rock(self, pos, vel, r, seed_value)
	debris.append(d)
	space.add_body(d.body)
	return d


func _spawn_player() -> void:
	player = _make_ship(Settings.pilot_name, Settings.race_id, _starting_design(Settings.race_id, Settings.career), Settings.pilot_name + "'s ship", true)
	player.score = Settings.career["score"]
	player.kills = Settings.career["kills"]
	player.rank = GameData.rank_for(player.score, player.kills)
	player.credits = START_CREDITS
	var pc := PlayerController.new()
	pc.name = "Controller"
	player.add_child(pc)
	pc.setup(player, self)


func _spawn_pilots() -> void:
	_spawn_player()
	# starbases for the three empires (the player launches from one, below)
	for race_id in GameData.STARBASES:
		var home: Planet = planet_by_name[GameData.STARBASES[race_id]]
		var base := _make_ship("%s Command" % GameData.race(race_id)["name"], race_id, "Starbase", GameData.race(race_id)["name"] + " Starbase", false)
		base.global_position = _station_spot(home)
		base.velocity = space.circular_orbit_velocity(home.body, base.global_position)
		base.reset_physics_interpolation()
		var ai := AIPilot.new()
		base.add_child(ai)
		ai.setup(base, self, home, 14)
	_keep_station()
	_place_at_home(player)
	# computer pilots
	var per_race := {0: 0, 1: 0, 2: 0}
	var robots: Array = GameData.ROBOTS.duplicate()
	robots.shuffle()
	for r in robots:
		var race_id: int = r[1]
		if per_race[race_id] >= 6:
			continue
		var rank: int = r[2]
		var design := _design_for_rank(rank, race_id)
		var s := _make_ship(r[0], race_id, design, r[3], false)
		s.rank = rank
		s.score = GameData.RANKS[rank]["score"]
		s.kills = GameData.RANKS[rank]["kills"]
		var ai := AIPilot.new()
		ai.name = "AI"
		s.add_child(ai)
		var home: Planet = _home_planet(race_id)
		ai.setup(s, self, home, rank)
		# spread the pilots out so the fight does not start on top of the player
		_place_at_home(s, 1400.0)
		per_race[race_id] += 1
	_spawn_pirate_scouts()


## Practice duel: no round clock and no fleets, the colonies hold their
## fire, and one pirate Scout at a time comes at you from close by.  Your
## own starbase stays for repairs and the shipyard.
func _spawn_practice() -> void:
	_spawn_player()
	var home: Planet = planet_by_name[GameData.STARBASES[player.race_id]]
	var base := _make_ship("%s Command" % GameData.race(player.race_id)["name"], player.race_id, "Starbase", GameData.race(player.race_id)["name"] + " Starbase", false)
	base.global_position = _station_spot(home)
	base.velocity = space.circular_orbit_velocity(home.body, base.global_position)
	base.reset_physics_interpolation()
	# no AI for the starbase: it holds its fire, so every duel is yours
	_keep_station()
	_place_at_home(player)
	if not practice_opponent:
		return
	var s := _make_ship("Pirate Vex", 3, "Scout", "Rust Jackal", false)
	var ai := AIPilot.new()
	ai.name = "AI"
	s.add_child(ai)
	ai.setup(s, self, home, 2)
	_place_duelist(s)


## Put the practice opponent inside sensor range, facing the player.
func _place_duelist(s: Ship) -> void:
	var a := player.rotation + randf_range(-0.6, 0.6)
	s.global_position = player.global_position + Vector2(cos(a), sin(a)) * minf(1200.0, player.sensor_range * 0.75)
	s.velocity = player.velocity
	s.reset_physics_interpolation()
	s.rotation = a + PI


## One pirate raider, held back for a rare visit: it sits benched (dead,
## out of the world) until its time comes, then launches from Ceres, out
## of the player's view.  It is in the roster from the first frame, so
## online joiners have it too.
func _spawn_pirate_scouts() -> void:
	if player == null:
		return
	var home := _home_planet(player.race_id)
	var s := _make_ship("Pirate Vex", 3, "Scout", "Rust Jackal", false)
	s.rank = 0
	var ai := AIPilot.new()
	ai.name = "AI"
	s.add_child(ai)
	ai.setup(s, self, home, 1)
	_bench(s, randf_range(PIRATE_FIRST.x, PIRATE_FIRST.y))


## Take a ship out of play until `delay` seconds from now.
func _bench(s: Ship, delay: float) -> void:
	s.dead = true
	s.hull = 0.0
	if s.body != null:
		s.body.enabled = false
	s.hide()
	s.set_physics_process(false)
	respawn_queue.append([s, elapsed + delay])


func _starting_design(race_id: int, career: Dictionary) -> String:
	var level := GameData.rank_for(career.get("score", 0), career.get("kills", 0))
	return _design_for_rank(level, race_id)


func _design_for_rank(level: int, race_id: int) -> String:
	var options := GameData.ship_names_for_rank(level, false)
	options.erase("Starbase")       # each empire has exactly one starbase
	if options.is_empty():
		return "Scout"
	# computer pilots prefer the best hull they may fly
	return options[options.size() - 1]


## Where a starbase sits: off its planet, on the side away from the
## planet's parent so a moon's base is never inside the planet.
func _station_spot(home: Planet) -> Vector2:
	var a := randf() * TAU
	if home.parent_body != null:
		a = (home.global_position - home.parent_body.global_position).angle() + randf_range(-0.6, 0.6)
	return home.global_position + Vector2(cos(a), sin(a)) * (home.radius + 420.0)


func _home_planet(race_id: int) -> Planet:
	var name: String = GameData.STARBASES.get(race_id, GameData.race(race_id)["home"])
	return planet_by_name.get(name, planets[0])


## `hull_race` is whose shipyard built the hull, when it is not the
## pilot's own (a captured prize keeps its painting).
func _make_ship(pilot: String, race_id: int, design: String, ship_name: String, is_player: bool, hull_race: int = -1) -> Ship:
	var s := Ship.new()
	s.name = pilot
	s.is_player = is_player
	s.hull_race = hull_race
	ships_layer.add_child(s)
	s.setup(self, design, race_id, pilot, ship_name)
	s.died.connect(_on_ship_died)
	s.damaged.connect(_on_ship_damaged)
	s.rank_changed.connect(_on_rank_changed)
	s.system_failed.connect(_on_system_failed)
	s.touched_down.connect(_on_touched_down)
	s.lifted_off.connect(_on_lifted_off)
	ships.append(s)
	space.add_body(s.body)
	return s


## Put a ship beside its home world, optionally scattered up to `spread`
## px away, moving with the planet (in a parking orbit when close enough).
func _place_at_home(s: Ship, spread: float = 0.0) -> void:
	var home := _home_planet(s.race_id)
	var a := randf() * TAU
	# pilots launch from their starbase when it stands
	var base := starbase_of(s.race_id)
	if base != null and not s.is_station:
		var off := Vector2(cos(a), sin(a)) * (base.size * 1.4 + 120.0)
		if spread > 0.0:
			off += Vector2(randf_range(-spread, spread), randf_range(-spread, spread))
			if off.length() < base.size * 1.4 + 120.0:
				off = off.normalized() * (base.size * 1.4 + 120.0)
		s.global_position = base.global_position + off
		s.velocity = base.velocity + off.normalized() * 30.0
		s.reset_physics_interpolation()
		s.rotation = off.angle()
		return
	# otherwise a parking orbit well inside the planet's gravity
	var alt := minf(home.radius + 260.0, PhysicsSpace.reach_of(home.body) * 0.6)
	s.global_position = home.global_position + Vector2(cos(a), sin(a)) * maxf(alt, home.radius + 60.0)
	if spread > 0.0:
		s.global_position += Vector2(randf_range(-spread, spread), randf_range(-spread, spread))
		if s.global_position.distance_to(home.global_position) < home.radius + 80.0:
			s.global_position = home.global_position + (s.global_position - home.global_position).normalized() * (home.radius + 80.0)
	s.velocity = space.circular_orbit_velocity(home.body, s.global_position)
	s.reset_physics_interpolation()
	s.rotation = (s.velocity - home.velocity).angle() if s.velocity != home.velocity else a


func starbase_of(race_id: int) -> Ship:
	for s in ships:
		if s.is_station and s.race_id == race_id and not s.dead:
			return s
	return null


## Starbases hold a fixed spot beside their home world instead of falling
## free around the Sun (with only the planet's velocity they drifted
## sunward, which put the Terran base deep in the heat).  Their bodies are
## kinematic, so ships bounce off them and nothing moves them.
func _keep_station() -> void:
	for s in ships:
		if not s.is_station or s.dead:
			continue
		var home: Planet = planet_by_name.get(GameData.STARBASES.get(s.race_id, ""), null)
		if home == null:
			continue
		if not s.has_meta("station_angle"):
			# on the side of the planet away from the Sun
			s.set_meta("station_angle", home.global_position.angle() + randf_range(-0.6, 0.6))
		var ang: float = s.get_meta("station_angle")
		var r := home.radius + 420.0
		s.global_position = home.global_position + Vector2(cos(ang), sin(ang)) * r
		s.velocity = home.velocity


# ------------------------------------------------------------ main loop

func _process(delta: float) -> void:
	if paused:
		return
	# pull the camera out as the ship speeds up, further still in warp
	var speed_mul := 1.0
	if player != null and not player.dead:
		if player.warping:
			speed_mul = 0.5
		else:
			speed_mul = lerpf(1.0, 0.75, clampf(player.velocity.length() / GameData.LIGHT_SPEED / 1.2, 0.0, 1.0))
	camera.zoom = camera.zoom.lerp(Vector2.ONE * target_zoom * BASE_ZOOM * speed_mul, 2.5 * delta)
	if slowmo > 0.0:
		slowmo -= delta / maxf(Engine.time_scale, 0.01)
		if slowmo <= 0.0:
			Engine.time_scale = 1.0
	intensity_timer -= delta
	if intensity_timer <= 0.0:
		intensity_timer = 0.5
		Music.set_intensity(_combat_intensity())


## Follow the player on the physics tick, after the ships have moved, so
## camera and ship move in the same steps and the interpolation between
## ticks keeps the ship still on screen.  Smoothing as Camera2D's own
## (speed 6); a jump of thousands of units (respawn, warp, a teleport)
## snaps instead of sliding across the system.
func _follow_camera(delta: float) -> void:
	var target := cam_cur
	if player != null and not player.dead:
		# look a little ahead of the ship's motion
		target = player.global_position + player.velocity * 0.35
	elif player != null:
		target = player.global_position
	cam_prev = cam_cur
	if cam_cur.distance_to(target) > 4000.0 or cam_cur == Vector2.ZERO:
		cam_cur = target
		cam_prev = target
		camera.reset_physics_interpolation()
	else:
		cam_cur = cam_cur.lerp(target, minf(6.0 * delta, 1.0))
	camera.global_position = cam_cur
	# shake decays quickly; the offset is in screen pixels so divide by zoom
	if shake > 0.0:
		shake = maxf(shake - delta * 18.0 * SHAKE_SCALE, 0.0)
		var a := randf() * TAU
		camera.offset = Vector2(cos(a), sin(a)) * shake * Settings.screen_shake / camera.zoom.x
	else:
		camera.offset = Vector2.ZERO


## Where the camera is drawn this frame, between its last two ticks.
func camera_centre() -> Vector2:
	return cam_prev.lerp(cam_cur, Engine.get_physics_interpolation_fraction()) + camera.offset


## Where a moving body is drawn this frame: its tick position eased back
## along its velocity by the part of the tick not yet shown.
func drawn_position(n: Node2D) -> Vector2:
	var v = n.get("velocity")
	if not (v is Vector2):
		return n.global_position
	return n.global_position - v * (1.0 - Engine.get_physics_interpolation_fraction()) / Engine.physics_ticks_per_second


func _physics_process(delta: float) -> void:
	if paused:
		return
	elapsed += delta
	missiles = missiles.filter(func(m): return is_instance_valid(m))
	powerups = powerups.filter(func(p): return is_instance_valid(p))
	for p in planets:
		p.update_orbit(delta)
		p.sync_body()
	if Net.is_client():
		Net.client_tick(delta)   # the host collides, scores and respawns
		_follow_camera(delta)
		return
	sun_heat = 0.0
	_keep_station()
	# everything that flies fills in its forces, the space moves it, and
	# then each node takes its new state back
	for s in ships:
		s.pre_step(delta)
	for m in missiles:
		m.pre_step(delta)
	for p in powerups:
		p.pre_step(delta)
	for d in debris:
		d.pre_step(delta)
	space.step(delta)
	for s in ships:
		s.post_step(delta)
	for m in missiles:
		m.post_step(delta)
	for p in powerups.duplicate():
		p.post_step(delta)
	for d in debris.duplicate():
		d.post_step(delta)
	_handle_contacts()
	_sun_hazard(delta)
	_sun_warning(delta)
	_collide_missiles()
	_collect_powerups()
	_landed_ships(delta)
	_refuel(delta)
	capture.tick(delta)
	_respawns()
	_score_planets(delta)
	_follow_camera(delta)


func set_paused(p: bool) -> void:
	paused = p
	get_tree().paused = p
	if p:
		Engine.time_scale = 1.0
		slowmo = 0.0


## Kyle: shake at a tenth of its first strength.  The Settings choice
## (Off / Light / Full) scales on top of this.
const SHAKE_SCALE := 0.1

func add_shake(amount: float) -> void:
	shake = minf(shake + amount * SHAKE_SCALE, 28.0 * SHAKE_SCALE)


## What the music should feel like right now.
func _combat_intensity() -> int:
	if player == null or player.dead:
		return Music.Intensity.TENSION
	var enemy := nearest_enemy_ship(player.global_position, player.race_id, player.sensor_range * 1.5, player)
	var enemy_dist := 1e9
	if enemy != null:
		enemy_dist = player.global_position.distance_to(enemy.global_position)
	var hurt := elapsed - last_player_damage
	var fired := elapsed - last_player_fire
	if hurt < 6.0 or (fired < 4.0 and enemy_dist < 1400.0) or enemy_dist < 800.0:
		return Music.Intensity.BATTLE
	if enemy != null or hurt < 15.0:
		return Music.Intensity.TENSION
	return Music.Intensity.CRUISE


# ---------------------------------------------------------------- physics

func gravity_at(pos: Vector2, _mass: float = 0.0) -> Vector2:
	return space.gravity_at(pos)


## Which pairs of bodies may touch.  Planets are solid to everything but
## shots (the world handles those, since colonies take damage); shots skip
## their owner and their own side; floating modules are never solid.
func _can_collide(a: PhysicsBody, b: PhysicsBody) -> bool:
	var ka := a.kind
	var kb := b.kind
	if ka == PhysicsBody.Kind.PICKUP or kb == PhysicsBody.Kind.PICKUP:
		return false
	if ka == PhysicsBody.Kind.PROJECTILE or kb == PhysicsBody.Kind.PROJECTILE:
		var shot := a if ka == PhysicsBody.Kind.PROJECTILE else b
		var other := b if ka == PhysicsBody.Kind.PROJECTILE else a
		if not shot.bullet:
			return false
		if other.kind != PhysicsBody.Kind.SHIP and other.kind != PhysicsBody.Kind.ROCK:
			return false
		var m: Missile = shot.node
		if m == null or m.exploded or m.armed > 0.0:
			return false
		if other.kind == PhysicsBody.Kind.SHIP:
			var t: Ship = other.node
			if t == m.owner_ship or t.race_id == m.owner_race:
				return false
		return true
	if ka == PhysicsBody.Kind.PLANET or kb == PhysicsBody.Kind.PLANET:
		var p: Planet = (a if ka == PhysicsBody.Kind.PLANET else b).node
		return p.look != "Sun"
	return true


func nearest_body(pos: Vector2) -> Planet:
	var best: Planet = null
	var best_d := INF
	for p in planets:
		var d := pos.distance_to(p.global_position) - p.radius
		if d < best_d:
			best_d = d
			best = p
	return best


func nearest_enemy_ship(pos: Vector2, race_id: int, range_px: float, viewer: Ship = null) -> Ship:
	var best: Ship = null
	var best_d := range_px
	for s in ships:
		if s.dead or s.race_id == race_id or s.is_landing():
			continue
		if s.spoken_for(race_id):
			continue      # a friend is taking that one alive
		if viewer != null and not s.visible_to(viewer):
			continue
		elif viewer == null and s.cloaked:
			continue
		var d := pos.distance_to(s.global_position)
		if d < best_d:
			best_d = d
			best = s
	return best


## The nearest enemy any ship of the viewer's empire can see: what the
## fleet's shared sensors know, out to a long range.
func nearest_known_enemy(viewer: Ship, range_px: float = 16000.0) -> Ship:
	var best: Ship = null
	var best_d := range_px
	for s in ships:
		if s.dead or s.race_id == viewer.race_id or s.is_station or s.is_landing():
			continue
		var d := viewer.global_position.distance_to(s.global_position)
		if d >= best_d:
			continue
		var seen := s.visible_to(viewer) and d <= viewer.sensor_range * 1.5
		if not seen:
			for f in ships:
				if f.dead or f.race_id != viewer.race_id:
					continue
				if s.visible_to(f) and f.global_position.distance_to(s.global_position) <= f.sensor_range * 1.5:
					seen = true
					break
		if seen:
			best_d = d
			best = s
	return best


func ship_at(pos: Vector2, tolerance: float) -> Ship:
	var best: Ship = null
	var best_d := INF
	for s in ships:
		if s.dead or s.is_landed():
			continue
		var d := pos.distance_to(s.global_position)
		if d < s.size + tolerance and d < best_d:
			best_d = d
			best = s
	return best


## The world under a point, if any: the nearest whose drawn disc (plus
## the tolerance) holds it.  Not the Sun.
func planet_at(pos: Vector2, tolerance: float) -> Planet:
	var best: Planet = null
	var best_d := INF
	for p in planets:
		if p.look == "Sun":
			continue
		var d := pos.distance_to(p.global_position) - p.drawn_radius()
		if d < tolerance and d < best_d:
			best_d = d
			best = p
	return best


func enemy_planets(race_id: int) -> Array:
	var out := []
	for p in planets:
		if p.is_colonised() and p.owner_race != race_id:
			out.append(p)
	return out


## Turn the step's contacts into damage, sound and shake.
func _handle_contacts() -> void:
	for c in space.contacts:
		var v: float = c["speed"]
		if v < IMPACT_THRESHOLD:
			continue
		var a: PhysicsBody = c["a"]
		var b: PhysicsBody = c["b"]
		var point: Vector2 = c["point"]
		_impact(a, b, v, point)
		_impact(b, a, v, point)
		var loud := clampf((v - IMPACT_THRESHOLD) / 300.0, 0.25, 0.8)
		if a.kind == PhysicsBody.Kind.SHIP or b.kind == PhysicsBody.Kind.SHIP:
			Sfx.play("hit", point, loud, 0.7)
		if player != null and (a.node == player or b.node == player):
			add_shake(clampf((v - IMPACT_THRESHOLD) * 0.05, 2.0, 14.0))


func _impact(victim: PhysicsBody, other: PhysicsBody, v: float, point: Vector2) -> void:
	var node := victim.node
	if node == null or not is_instance_valid(node):
		return
	# the lighter party takes the bigger share; a planet takes none
	var share := 1.0
	if other.inv_mass > 0.0:
		share = 2.0 * other.mass / (victim.mass + other.mass)
	var dmg := (v - IMPACT_THRESHOLD) * 0.6 * share
	if node is Ship:
		var attacker: Node = other.node
		if other.node is Ship:
			# a ram prow turns a collision into a weapon when it hits nose first
			var o: Ship = other.node
			var n := (node.global_position - o.global_position).normalized()
			if o.ram_power > 0.0 and o.forward().dot(n) > 0.7:
				dmg *= o.ram_power
			if node.ram_power > 0.0 and node.forward().dot(-n) > 0.7:
				dmg /= node.ram_power
			# wingmates bump without credit for the crash
			if o.race_id == node.race_id:
				dmg *= 0.5
				attacker = null
		node.take_damage(dmg, attacker, point)
	elif node is Debris:
		node.take_damage(dmg * 0.5, other.node, point)


## The sun burns anything that comes close and swallows what touches it.
func _sun_hazard(delta: float) -> void:
	if sun == null:
		return
	for a in ships:
		if a.dead:
			continue
		var d := a.global_position.distance_to(sun.global_position)
		# heat ramps up over the radii above the surface: a warning first,
		# then real damage, then a scorching bounce off the corona rather
		# than an instant death
		var heat := clampf(1.0 - (d - sun.radius) / (sun.radius * SUN_HEAT_RADII), 0.0, 1.0)
		if a == player:
			sun_heat = heat
		# the outer third only warns; inside it heat builds slowly enough
		# that turning away in a few seconds always saves you
		if heat > 0.35:
			var t := (heat - 0.35) / 0.65
			a.take_damage((15.0 + 320.0 * t * t) * delta, sun)
		var min_d := sun.radius + a.size * 0.7
		if d < min_d:
			var n := (a.global_position - sun.global_position).normalized() if d > 0.001 else Vector2.RIGHT
			a.global_position = sun.global_position + n * min_d
			var vn := a.velocity.dot(n)
			if vn < 0.0:
				a.velocity -= n * vn * 1.6
			a.take_damage(400.0 * delta, sun)
	for d in debris.duplicate():
		if d.global_position.distance_to(sun.global_position) < sun.radius:
			remove_debris(d)
	for m in missiles:
		if not m.exploded and m.global_position.distance_to(sun.global_position) < sun.radius:
			m._free()


## Beep and say so while the player is in the Sun's heat.
func _sun_warning(delta: float) -> void:
	if player == null or player.dead or sun_heat <= 0.0:
		_heat_alarm = 0.0
		return
	_heat_alarm -= delta
	if _heat_alarm <= 0.0:
		_heat_alarm = lerpf(1.6, 0.35, sun_heat)
		Sfx.play_ui("alarm", 0.35 + 0.5 * sun_heat)
		if sun_heat < 0.35:
			notify(player, "Warning: the Sun's heat. Turn away before it burns through the hull.")


func _collide_missiles() -> void:
	# fast shots: whatever the sweep touched first
	for hit in space.bullet_hits:
		var m: Missile = hit["bullet"].node
		if m == null or not is_instance_valid(m) or m.exploded:
			continue
		var target: Node2D = hit["body"].node
		if target == null or not is_instance_valid(target):
			continue
		m.hit_point = hit["point"]
		m.hit_normal = hit["normal"]
		m.explode(target)
	# proximity fuses: grenades and mines go off near any enemy hull
	for m: Missile in missiles:
		if not is_instance_valid(m) or m.exploded or m.data["trigger"] != "prox":
			continue
		var hit := false
		for s in ships:
			if s.dead or s == m.owner_ship or s.is_landing():
				continue
			if s.race_id == m.owner_race:
				continue      # no friendly fire, mines included
			if m.check_hit(s.body):
				m.hit_point = Vector2.INF
				m.explode(s)
				hit = true
				break
		if hit:
			continue
	for m: Missile in missiles:
		if not is_instance_valid(m) or m.exploded:
			continue
		for p in planets:
			var d := m.global_position.distance_to(p.global_position)
			if d < p.radius + m.radius:
				if p.is_colonised() and p.owner_race != m.owner_race:
					m.hit_point = m.global_position
					m.explode(p)
				else:
					fx.spark(m.global_position, 3, m.color)
					m._free()
				break


func raycast_beam(shooter: Node, origin: Vector2, dir: Vector2, range_px: float) -> Dictionary:
	var best_t := range_px
	var best: Node2D = null
	var shooter_race: int = shooter.race_id if "race_id" in shooter else shooter.owner_race
	for s in ships:
		if s.dead or s == shooter or s.race_id == shooter_race or s.warping or s.is_landing():
			continue
		var t := space.ray_body(origin, dir, s.body, best_t)
		if t >= 0.0 and t < best_t:
			best_t = t
			best = s
	for d in debris:
		var t := space.ray_body(origin, dir, d.body, best_t)
		if t >= 0.0 and t < best_t:
			best_t = t
			best = d
	for p in planets:
		var t := _ray_circle(origin, dir, p.global_position, p.radius)
		if t >= 0.0 and t < best_t:
			best_t = t
			best = p if (p.is_colonised() and p.owner_race != shooter_race) else null
	return {"body": best, "point": origin + dir * best_t}


func _ray_circle(o: Vector2, d: Vector2, c: Vector2, r: float) -> float:
	var oc := o - c
	var b := oc.dot(d)
	var cc := oc.dot(oc) - r * r
	var disc := b * b - cc
	if disc < 0.0:
		return -1.0
	var t := -b - sqrt(disc)
	return t if t >= 0.0 else -1.0


func add_beam(a: Vector2, b: Vector2, color: Color) -> void:
	fx.beam(a, b, color)


# ---------------------------------------------------------------- spawns

func spawn_missile(owner: Node, missile_id: String, pos: Vector2, vel: Vector2, angle: float, target: Node2D) -> void:
	var m: Missile = MISSILE_SCRIPT.new()
	missiles_layer.add_child(m)
	var race: int = owner.race_id if owner is Ship else owner.owner_race
	m.setup(self, owner, race, missile_id, pos, vel, angle, target)
	missiles.append(m)
	Net.host_shot(m, owner, missile_id, pos, vel, angle)
	space.add_body(m.body)


func release_body(b: PhysicsBody) -> void:
	if b != null:
		space.remove_body(b)


## Shove a node's body and keep the node's copy of the state in step.
func push_node(node: Node2D, j: Vector2, at: Vector2) -> void:
	if node is Ship:
		node.apply_hit_impulse(j, at)
	elif node is Debris:
		node.body.apply_impulse(j, at)
	elif node is Powerup or node is Missile:
		node.body.apply_impulse(j, at)
		node.velocity = node.body.vel


func missile_detonated(m: Missile, hit: Node2D, splash: float) -> void:
	var pos := m.global_position
	if Net.is_client():
		# only looks; the host decides damage and tells us
		fx.explosion(pos, (splash * 0.5) if splash > 0.0 else (6.0 + m.radius * 2.0), m.color, m.missile_id == "002")
		return
	Net.host_boom(m, pos, splash)
	var attacker: Node = m.owner_ship if m.owner_ship != null and is_instance_valid(m.owner_ship) else null
	if attacker == null:
		attacker = m
		m.set_meta("race_id", m.owner_race)
	if splash <= 0.0:
		if hit != null:
			var at := m.hit_point if m.hit_point != Vector2.INF else pos
			if hit is Ship:
				hit.take_damage(m.warhead, attacker if attacker is Ship else m.owner_ship, at, m.data.get("sys", 1.0), "engines" if m.data.get("heat", false) else "")
			else:
				hit.take_damage(m.warhead, attacker if attacker is Ship or attacker is Planet else m.owner_ship, at)
			if hit is Ship and m.data.has("drain"):
				hit.energy = maxf(hit.energy - m.data["drain"], 0.0)
				if hit.cloaked:
					hit.set_cloak(false)
			# the shot's momentum and the blast kick the target
			var dir := m.velocity.normalized() if m.velocity.length_squared() > 1.0 else -m.hit_normal
			push_node(hit, dir * m.warhead * IMPULSE_PER_DAMAGE + m.momentum(), at)
		fx.explosion(pos, 6.0 + m.radius * 2.0, m.color, false, "impact")
		return
	# area damage falls off with distance
	for s in ships:
		if s.dead or s.race_id == m.owner_race:
			continue
		var d := pos.distance_to(s.global_position) - s.size * 0.5
		if d < splash:
			var f := 1.0 - maxf(d, 0.0) / splash
			# the blast hits the side of the hull that faces it
			s.take_damage(m.warhead * f, attacker if attacker is Ship else null, s.body.closest_surface_point(pos), m.data.get("sys", 1.0))
	for p in planets:
		var d := pos.distance_to(p.global_position) - p.radius
		if d < splash and p.is_colonised():
			var f := 1.0 - maxf(d, 0.0) / splash
			p.take_damage(m.warhead * f * 0.5, attacker if attacker is Ship else null)
	for d: Debris in debris:
		var dd: float = pos.distance_to(d.global_position) - d.rock_radius
		if dd < splash:
			d.take_damage(m.warhead * (1.0 - maxf(dd, 0.0) / splash) * 0.5, attacker, d.global_position)
	blast(pos, splash, m.warhead * IMPULSE_PER_DAMAGE * 0.6)
	fx.explosion(pos, splash * 0.5, m.color, m.missile_id == "002", "explode")


## A pressure wave that throws ships, rocks and loot outward.  Nothing
## light is thrown faster than BLAST_MAX_DV, so loot stays where it fell.
const BLAST_MAX_DV := 90.0
func blast(pos: Vector2, range_px: float, impulse: float, kinds: Array = [PhysicsBody.Kind.SHIP, PhysicsBody.Kind.ROCK, PhysicsBody.Kind.PICKUP]) -> void:
	for b: PhysicsBody in space.bodies_near(pos, range_px, kinds):
		var surf: Vector2 = b.closest_surface_point(pos)
		var f := 1.0 - clampf(surf.distance_to(pos) / range_px, 0.0, 1.0)
		if f <= 0.0:
			continue
		var dir := b.pos - pos
		dir = dir.normalized() if dir.length_squared() > 1e-6 else Vector2.RIGHT
		var j := minf(impulse * f, BLAST_MAX_DV * b.mass)
		push_node(b.node, dir * j, surf.lerp(b.pos, 0.5))


func spawn_sparks(pos: Vector2, n: int, color: Color) -> void:
	fx.spark(pos, n, color)


func spawn_powerup(id: String, pos: Vector2, vel: Vector2) -> void:
	var p := Powerup.new()
	pickups_layer.add_child(p)
	p.setup(id, pos, vel)
	p.world = self
	powerups.append(p)
	Net.host_powerup_spawned(p)
	space.add_body(p.body)


# -------------------------------------------------------------- debris

func remove_debris(d: Debris) -> void:
	if not is_instance_valid(d):
		return
	space.remove_body(d.body)
	debris.erase(d)
	d.queue_free()


## A rock shot to bits: big ones split, small ones turn to dust.
func break_rock(d: Debris) -> void:
	var pos := d.global_position
	var vel: Vector2 = d.body.vel
	var r := d.rock_radius
	fx.rock_burst(pos, vel, r, d.albedo)
	remove_debris(d)
	if r >= 9.0:
		var pieces := 2 + (1 if r > 15.0 else 0)
		for i in pieces:
			var a := TAU * i / pieces + randf() * 0.5
			var off := Vector2(cos(a), sin(a))
			var child := spawn_rock(pos + off * r * 0.6, vel + off * randf_range(15.0, 45.0), r * 0.58, randi())
			child.body.ang_vel = randf_range(-2.0, 2.0)
			child.match_material(d)


## Friendly bases and colonies top up a ship's tanks when it sits close.
## Friendly bases and colonies also make up a thinned crew.
const CREW_FROM_BASE := 2.0       # hands a second from a starbase
const CREW_FROM_COLONY := 0.5     # hands a second from a colony
func _refuel(delta: float) -> void:
	for s in ships:
		s.refuelling = false
		if s.dead or (s.fuel >= s.fuel_max and s.crew >= s.crew_max):
			continue
		var rate := 0.0
		var hands := 0.0
		for o in ships:
			if o.is_station and not o.dead and o.race_id == s.race_id and o != s and o.global_position.distance_to(s.global_position) < o.size + 260.0:
				rate = maxf(rate, 0.12)
				hands = CREW_FROM_BASE
		for p in planets:
			if p.is_colonised() and p.owner_race == s.race_id and p.global_position.distance_to(s.global_position) < p.radius + 220.0:
				rate = maxf(rate, 0.04)
				hands = maxf(hands, CREW_FROM_COLONY)
		if hands > 0.0 and s.crew < s.crew_max and s.boarded_by == null:
			var was_crew := s.crew
			s.crew = minf(s.crew + hands * delta, s.crew_max)
			if s.is_player and was_crew < s.crew_max and s.crew >= s.crew_max:
				notify(s, "Crew at full strength (%d)." % s.crew_max)
		if s.is_landed() and s.land_body.is_colonised() and s.land_body.owner_race == s.race_id:
			rate = maxf(rate, 0.15)       # the colony's spaceport fills the tanks
		if rate > 0.0 and s.fuel < s.fuel_max:
			s.refuelling = true
			var was := s.fuel
			s.refuel(s.fuel_max * rate * delta)
			if s.is_player and was < s.fuel_max * 0.25 and s.fuel >= s.fuel_max * 0.25:
				notify(s, "Refuelling.")


func _collect_powerups() -> void:
	for p in powerups:
		if not is_instance_valid(p):
			continue
		for s in ships:
			if s.dead or s.is_station or s.is_landing():
				continue
			if s.global_position.distance_to(p.global_position) < s.size * 0.8 + 10.0:
				var taken := false
				match p.kind:
					"credits":
						if s.is_player:
							s.earn(p.amount)
							notify(s, "Salvaged %d credits." % p.amount)
							taken = true
					"blueprint":
						if s.is_player:
							if not s.blueprints.has(p.module_id):
								s.blueprints.append(p.module_id)
								notify(s, "Blueprint: %s can now be bought at your shipyard." % GameData.MODULES[p.module_id]["name"])
							else:
								s.earn(60)
								notify(s, "Duplicate blueprint sold for 60 credits.")
							taken = true
					_:
						if s.try_pickup(p.module_id):
							if s.is_player:
								notify(s, "Salvaged %s (hold %d/%d)." % [p.label, s.hold.size(), Ship.HOLD_SIZE])
							taken = true
						elif s.is_player:
							notify(s, "Hold full: %s left behind." % p.label)
				if taken:
					Sfx.play("pickup", s.global_position, 0.7)
					Net.host_powerup_taken(p, s)
					release_body(p.body)
					p.queue_free()
				break


# ----------------------------------------------------------- game events

func _on_ship_damaged(s: Ship, amount: float) -> void:
	if s.is_player:
		last_player_damage = elapsed
		add_shake(clampf(amount * 0.04, 1.0, 10.0))


func _on_ship_died(s: Ship, killer: Node) -> void:
	if s.captured:
		_on_ship_captured(s, killer)
		return
	var big := s.size > 40.0
	fx.ship_destroyed(s)
	# the wave rocks nearby ships and rocks; the loot it leaves is spared
	blast(s.global_position, s.size * 5.0, s.mass * 25.0, [PhysicsBody.Kind.SHIP, PhysicsBody.Kind.ROCK])
	if player != null:
		# the blast is felt by distance: shake, a dip in the music, and a
		# beat of slow motion when it was the player's kill
		var near := 1.0 - clampf(player.global_position.distance_to(s.global_position) / 1800.0, 0.0, 1.0)
		if near > 0.0:
			add_shake((16.0 if big else 8.0) * near)
			Music.duck_for((0.8 if big else 0.5) * near)
		if killer == player and not s.is_player:
			Engine.time_scale = 0.25
			slowmo = 0.12
	if Net.is_client():
		if s.is_player:
			player_died.emit()
		return   # drops, score and respawn are the host's
	Net.host_ship_died(s, killer)
	# the wreck leaves modules floating
	var drops := 1 + (2 if big else 0) + (1 if randf() < 0.5 else 0)
	var candidates := []
	for id in s.design["modules"]:
		var f: String = GameData.MODULES[id]["func"]
		if f in ["shield", "eng", "sensor", "power", "stealth", "boost", "armor"]:
			candidates.append(id)
	for i in drops:
		if candidates.is_empty():
			break
		var id: String = candidates[randi() % candidates.size()]
		var a := randf() * TAU
		spawn_powerup(id, s.global_position + Vector2(cos(a), sin(a)) * s.size * 0.5, s.velocity * 0.5 + Vector2(cos(a), sin(a)) * randf_range(10.0, 30.0))
	# the purse spills: a credit chip, and now and then a blueprint of an
	# empire part the wreck carried
	var purse := 20 + s.rank * 6 + (200 if s.is_station else 0)
	spawn_powerup("cr:%d" % purse, s.global_position + Vector2(randf_range(-20, 20), randf_range(-20, 20)), s.velocity * 0.5 + Vector2(randf_range(-15, 15), randf_range(-15, 15)))
	var flavour := []
	for id in s.design["modules"]:
		if GameData.part_race(id) == s.race_id:
			flavour.append(id)
	for w in s.design["weapons"]:
		if GameData.part_race(w[0]) == s.race_id:
			flavour.append(w[0])
	if not flavour.is_empty() and randf() < (0.35 if big else 0.2):
		var a := randf() * TAU
		spawn_powerup("bp:" + flavour[randi() % flavour.size()], s.global_position, s.velocity * 0.5 + Vector2(cos(a), sin(a)) * 20.0)
	var killer_ship: Ship = killer as Ship
	if killer_ship != null and killer_ship.race_id == s.race_id:
		killer_ship = null        # never credit a kill on your own side
	if round_rules != null and killer_ship != null and killer_ship != s:
		round_rules.on_kill(killer_ship.race_id)
	if killer_ship != null and killer_ship != s and not killer_ship.dead:
		var points := 10 + s.rank * 2 + (40 if s.is_station else 0)
		killer_ship.add_score(points, true)
		if killer_ship.is_player:
			var bounty := 30 + s.rank * 8 + (300 if s.is_station else 0)
			killer_ship.earn(bounty)
			notify(killer_ship, "Bounty: %d credits." % bounty)
		notify_all("%s (%s) destroyed %s (%s)." % [killer_ship.pilot_name, GameData.race(killer_ship.race_id)["name"], s.pilot_name, GameData.race(s.race_id)["name"]],
			killer_ship.race_color() if killer_ship.is_player or s.is_player else Color(0.8, 0.8, 0.8))
	else:
		notify_all("%s was lost." % s.pilot_name, Color(0.75, 0.75, 0.75))
	if s.is_player:
		if records_career():
			Settings.career["deaths"] = Settings.career.get("deaths", 0) + 1
		Settings.save_settings()
		player_died.emit()
	if practice and not s.is_station:
		# only a duel's real result counts: you killed it, or it killed you
		if s.is_player and killer_ship != null:
			duel_score[1] += 1
		elif not s.is_player and killer_ship == player:
			duel_score[0] += 1
		notify_all("Duel: you %d, pirates %d.%s" % [duel_score[0], duel_score[1], "" if s.is_player else "  Another pirate comes at you in a few seconds."], Color(1.0, 0.9, 0.6))
	respawn_queue.append([s, elapsed + _respawn_delay(s)])


func _respawn_delay(s: Ship) -> float:
	if s.is_station:
		return RESPAWN_DELAY + 6.0
	if s.is_player or practice or s.net_id in Net.seats.values():
		return RESPAWN_DELAY
	return randf_range(PIRATE_RETURN.x, PIRATE_RETURN.y) if s.race_id == 3 else AI_RESPAWN_DELAY


## Put a computer pilot back at home, out of the player's view.  False when
## every spot tried is too close (the player is camped at that home), so
## the relaunch waits.
func _place_clear_of_player(s: Ship) -> bool:
	for i in 8:
		_place_at_home(s, 400.0 * i)
		if player == null or player.dead or s.global_position.distance_to(player.global_position) >= SPAWN_CLEAR:
			return true
	if starbase_of(s.race_id) == null:
		# no starbase to launch from: come in from the far side of home
		var home := _home_planet(s.race_id)
		var away := (home.global_position - player.global_position).normalized()
		var at := home.global_position + away * maxf(SPAWN_CLEAR - home.global_position.distance_to(player.global_position) + 200.0, home.radius + 400.0)
		if at.distance_to(player.global_position) >= SPAWN_CLEAR:
			s.global_position = at
			s.velocity = space.circular_orbit_velocity(home.body, at)
			s.reset_physics_interpolation()
			return true
	return false


## A boarded ship leaves the fight quietly: no blast and no loot (the
## captor has had both), and the pilot relaunches at home.  Capture.take
## did the scoring.
func _on_ship_captured(s: Ship, _captor: Node) -> void:
	fx.spark(s.global_position, 14, Color(0.6, 0.85, 1.0))
	if Net.is_client():
		if s.is_player:
			player_died.emit()
		return
	Net.host_ship_died(s, _captor)
	if s.is_player:
		if records_career():
			Settings.career["deaths"] = Settings.career.get("deaths", 0) + 1
		Settings.save_settings()
		player_died.emit()
	respawn_queue.append([s, elapsed + _respawn_delay(s)])


## A system shot out: tell the pilot who lost it and the one who did it.
func _on_system_failed(s: Ship, e: Dictionary, attacker: Node) -> void:
	var vital: bool = e["kind"] in ShipSystems.VITAL
	if s.is_player or s.net_id in Net.seats.values():
		notify(s, "%s offline!%s" % [e["label"], "  We're dead in space: damage control is on it." if vital and s.is_disabled() else ""])
		if s.is_player:
			Sfx.play_ui("alarm", 0.6)
	if attacker is Ship and attacker != s:
		if vital and s.is_disabled():
			notify(attacker, "%s's %s is disabled: %s out. Close in and press K to board." % [s.pilot_name, s.design_name, e["label"].to_lower()])
		else:
			notify(attacker, "Hit: %s's %s is out." % [s.pilot_name, e["label"].to_lower()])


## The player (or a seated online pilot) asks to board its target.
func try_board(s: Ship) -> void:
	if Net.is_client():
		Net.client_command("board:%d" % (s.target.net_id if s.target is Ship and is_instance_valid(s.target) else 0))
		return
	capture.try_board(s, s.target)


## Take a ship out of the world for good (a prize that is lost or sold).
func remove_ship(s: Ship) -> void:
	if not ships.has(s):
		return
	for o in ships:
		if o.boarding == s:
			capture.cancel(o, "")
		if o.target == s:
			o.target = null
		if o.prize_of == s:
			o.prize_of = null
	ships.erase(s)
	space.remove_body(s.body)
	Net.host_ship_removed(s)
	s.queue_free()


func _respawns() -> void:
	var keep := []
	for entry in respawn_queue:
		var s: Ship = entry[0]
		if not is_instance_valid(s):
			continue
		if elapsed >= entry[1] and s.is_prize:
			remove_ship(s)        # a prize's crew does not come back
		elif elapsed >= entry[1]:
			var home := _home_planet(s.race_id)
			if s.is_station:
				var at := _station_spot(home)
				s.respawn(at, space.circular_orbit_velocity(home.body, at))
			elif practice and not s.is_player and player != null and not player.dead:
				_place_duelist(s)
				s.respawn(s.global_position, s.velocity)
			elif s.is_player or s.net_id in Net.seats.values():
				_place_at_home(s)
				s.respawn(s.global_position, s.velocity)
			elif _place_clear_of_player(s):
				s.respawn(s.global_position, s.velocity)
			else:
				entry[1] = elapsed + 5.0      # the player sits on its home: try again later
				keep.append(entry)
				continue
			Net.host_ship_respawned(s)
			if s.is_player:
				player_respawned.emit()
				var base := starbase_of(s.race_id)
				notify(s, ("Launched from the %s starbase at %s." % [GameData.race(s.race_id)["name"], home.body_name]) if base != null else ("Launched from %s." % home.body_name))
		else:
			keep.append(entry)
	respawn_queue = keep


## Practice duels and free flight are off the record: nothing earned there
## reaches the pilot's profile.
func records_career() -> bool:
	return not practice


func _on_rank_changed(s: Ship, level: int) -> void:
	if s.is_player:
		notify_all("%s promoted to %s!" % [s.pilot_name, GameData.rank_name(level)], Color(1, 0.9, 0.5))
		Sfx.play_ui("rankup", 0.8)
		if not records_career():
			return
		Settings.career["score"] = s.score
		Settings.career["kills"] = s.kills
		Settings.save_settings()


func planet_captured(p: Planet, winner: int, attacker: Node, people: float = -1.0) -> void:
	var old := p.owner_race
	p.surrender_to(winner, people)
	Net.host_planet_captured(p, winner)
	var mult := round_rules.bonus_multiplier() if round_rules != null else 1.0
	if attacker is Ship:
		attacker.add_score(50, false)
		attacker.captures += 1
		if attacker.is_player:
			var bonus := int(200 * mult)
			attacker.earn(bonus)
			notify(attacker, "Capture bonus: %d credits." % bonus)
	Net.host_colony_changed(p)
	if round_rules != null:
		round_rules.on_capture(winner, p.body_name, old)
	notify_all("%s surrendered to the %s%s." % [p.body_name, GameData.race(winner)["name"],
		(" (was %s)" % GameData.race(old)["name"]) if old >= 0 else ""], GameData.race(winner)["color"])
	Sfx.play_ui("captured", 0.7)
	planet_owner_changed.emit(p)
	if round_rules != null and round_rules.phase == "sudden_death" and winner <= 2:
		round_rules.decide(winner)


func _score_planets(delta: float) -> void:
	planet_counts = {0: 0, 1: 0, 2: 0, 3: 0}
	for p in planets:
		if p.is_colonised():
			planet_counts[p.owner_race] = planet_counts.get(p.owner_race, 0) + 1
	if game_over:
		return
	# held planets pay their empire's pilots every minute, double in the
	# final phase
	income_timer -= delta
	if income_timer <= 0.0:
		income_timer = 60.0
		var mult := round_rules.bonus_multiplier() if round_rules != null else 1.0
		for s in ships:
			if s.is_station or s.race_id > 2:
				continue
			var n: int = planet_counts.get(s.race_id, 0)
			if n <= 0:
				continue
			var pay := int(n * 10 * mult)
			s.earn(pay)
			if s.is_player:
				notify(s, "Planet income: %d credits from %d worlds." % [pay, n])
	if round_rules != null:
		round_rules.tick(delta)


## The round is decided: freeze the fight in slow motion, record the
## career, and let Main show the scoreboard.
func _on_round_finished(result: Dictionary) -> void:
	game_over = true
	var winner: int = result["winner"]
	notify_all("The %s win the Sol system!" % GameData.race(winner)["name"], GameData.race(winner)["color"])
	Engine.time_scale = 0.35
	slowmo = 1.2
	if player != null and records_career():
		Settings.career["rounds"] = Settings.career.get("rounds", 0) + 1
		if winner == player.race_id:
			Settings.career["wins"] = Settings.career.get("wins", 0) + 1
		Settings.career["score"] = player.score
		Settings.career["kills"] = player.kills
		Settings.save_settings()
	Sfx.play_ui("rankup", 1.0)
	Net.host_round_over(result)
	round_over.emit(result)


# ------------------------------------------------- landing and colonists

## The world a ship would land on: the nearest body's surface.
func landing_body(s: Ship) -> Planet:
	if s.is_landing():
		return s.land_body
	return nearest_body(s.global_position)


## B: land on the world below, or lift off from it.  Computer pilots name
## the world they mean (a moon can sit closer to its planet than the ship).
func try_land(s: Ship, onto: Planet = null) -> void:
	if s.dead:
		return
	if Net.is_client():
		Net.client_command("land")
		return
	if s.is_landed():
		s.start_takeoff()
		notify(s, "Lifting off from %s." % s.land_body.body_name)
		return
	if s.is_landing():
		return
	var p := onto if onto != null else landing_body(s)
	var why := s.landing_refusal(p)
	if not why.is_empty():
		notify(s, why)
		return
	s.start_landing(p)
	Net.host_landing(s)
	notify(s, "Landing on %s." % p.body_name)


## 1 / 2 on the ground: take colonists aboard, or put them down.
func colonist_command(s: Ship, boarding: bool) -> void:
	if Net.is_client():
		Net.client_command("colonists_board" if boarding else "colonists_unload")
		return
	if not s.is_landed():
		notify(s, "Land on a world first (B).")
		return
	var p := s.land_body
	var friendly := p.is_colonised() and p.owner_race == s.race_id
	if boarding:
		if not friendly:
			notify(s, "Colonists only board from your own colonies.")
		elif s.colonists >= s.cabin - 1.0:
			notify(s, "The cabin is full: %s aboard." % GameData.fmt_pop(s.colonists))
		elif spare_colonists(p) <= 0.0:
			notify(s, "%s cannot spare any colonists (it needs %s to keep going)." % [p.body_name, GameData.fmt_pop(_colony_reserve(p))])
		else:
			s.transfer = 1
			notify(s, "Colonists boarding from %s." % p.body_name)
	else:
		if s.colonists <= 0.0:
			notify(s, "No colonists aboard. Take some on at one of your colonies (land, then 1).")
		elif friendly:
			s.transfer = -1
			notify(s, "Colonists going ashore to join %s." % p.body_name)
		else:
			deliver_colonists(s, p)


## What a colony keeps for itself: a quarter of what the world could hold.
func _colony_reserve(p: Planet) -> float:
	return maxf(10e6, p.optimal_population(p.owner_race) * 0.25)


func spare_colonists(p: Planet) -> float:
	if not p.is_colonised():
		return 0.0
	return maxf(p.population - _colony_reserve(p), 0.0)


func _on_touched_down(s: Ship, p: Planet) -> void:
	Net.host_landing(s)
	Sfx.play("dock", s.global_position, 0.5, 0.7)
	var friendly := p.is_colonised() and p.owner_race == s.race_id
	if friendly:
		if s.colonists <= 0.0 and s.cabin > 0.0 and spare_colonists(p) > 0.0:
			s.transfer = 1
			notify(s, "Landed on %s. Colonists are boarding (2 puts them back, B lifts off)." % p.body_name)
		elif s.colonists > 0.0:
			notify(s, "Landed on %s with %s colonists aboard: 2 puts them down here, 1 takes more on." % [p.body_name, GameData.fmt_pop(s.colonists)])
		else:
			notify(s, "Landed on %s: refuelling and repairing." % p.body_name)
	elif s.colonists > 0.0:
		deliver_colonists(s, p)
	elif p.is_colonised():
		notify(s, "Landed on the %s colony %s (%s people). Bring colonists to invade it." % [GameData.race(p.owner_race)["adj"], p.body_name, GameData.fmt_pop(p.population)])
	else:
		notify(s, "Landed on %s. Nobody lives here: bring colonists to settle it (it can support %s)." % [p.body_name, GameData.fmt_pop(p.optimal_population(s.race_id))])


func _on_lifted_off(s: Ship, _p: Planet) -> void:
	Net.host_landing(s)


## Landed ships: colonists board or go ashore, and a friendly spaceport
## patches up the hull.
func _landed_ships(delta: float) -> void:
	for s in ships:
		if s.dead or not s.is_landed():
			continue
		var p := s.land_body
		var friendly := p.is_colonised() and p.owner_race == s.race_id
		if friendly and s.hull < s.hull_max:
			s.hull = minf(s.hull + s.hull_max * 0.03 * delta, s.hull_max)
		if s.transfer == 0:
			continue
		if not friendly:
			s.transfer = 0
			continue
		var step := s.cabin / 3.0 * delta
		if s.transfer > 0:
			var n := minf(minf(step, s.cabin - s.colonists), spare_colonists(p))
			s.colonists += n
			p.population -= n
			if n > 0.0:
				s.colonist_origin = p
			if s.colonists >= s.cabin - 1.0 or spare_colonists(p) <= 0.0:
				s.transfer = 0
				notify(s, "%s colonists aboard%s. Land them on an empty world to settle it, or on an enemy colony to invade." % [GameData.fmt_pop(s.colonists),
					"" if s.colonists >= s.cabin - 1.0 else " (all %s can spare)" % p.body_name])
				Net.host_colony_changed(p)
		else:
			var n := minf(step, s.colonists)
			s.colonists -= n
			p.population += n
			s.set_meta("delivered", float(s.get_meta("delivered", 0.0)) + n)
			if s.colonists <= 0.0:
				s.colonists = 0.0
				s.transfer = 0
				var total: float = s.get_meta("delivered", 0.0)
				s.remove_meta("delivered")
				# pay only for people brought from another colony into room
				# this one had: unloading where you boarded earns nothing
				var room := maxf(p.optimal_population(p.owner_race) - (p.population - total), 0.0)
				var useful := 0.0 if s.colonist_origin == p else minf(total, room)
				s.colonist_origin = null
				var pay := int(useful / 1e6)
				if pay > 0:
					s.earn(pay)
					s.add_score(maxi(int(useful / 20e6), 1), false)
					notify(s, "%s colonists joined %s (now %s). %d credits." % [GameData.fmt_pop(total), p.body_name, GameData.fmt_pop(p.population), pay])
				else:
					notify(s, "%s colonists went back ashore on %s (now %s)." % [GameData.fmt_pop(total), p.body_name, GameData.fmt_pop(p.population)])
				Net.host_colony_changed(p)


## The colonists aboard land all at once on a world that is not ours: they
## settle it if nobody lives there, or fight the people who do.
func deliver_colonists(s: Ship, p: Planet) -> void:
	var people := s.colonists
	if people <= 0.0:
		return
	var mult := round_rules.bonus_multiplier() if round_rules != null else 1.0
	var race_name: String = GameData.race(s.race_id)["name"]
	s.colonists = 0.0
	s.transfer = 0
	s.colonist_origin = null
	if not p.is_colonised():
		p.found_colony(s.race_id, people)
		s.add_score(40, false)
		s.captures += 1
		var bonus := int(150 * mult)
		s.earn(bonus)
		notify(s, "Colony founded on %s: %s settlers. %d credits." % [p.body_name, GameData.fmt_pop(people), bonus])
		notify_all("%s settled %s for the %s." % [s.pilot_name, p.body_name, race_name], GameData.race(s.race_id)["color"])
		Sfx.play_ui("captured", 0.6)
		if round_rules != null:
			round_rules.on_settle(s.race_id, p.body_name)
		Net.host_colony_changed(p)
		planet_owner_changed.emit(p)
		return
	# invasion: the landing force fights at the offensive factor once the
	# defences are down, on even terms against shields and guns
	var factor := GameData.OFFENSIVE_FACTOR if p.is_defenceless() else 1.0
	var attack := people * factor
	if attack > p.population:
		var survivors := maxf(people - p.population / factor, GameData.MIN_COLONY)
		notify(s, "Invasion of %s succeeded: %s colonists hold it." % [p.body_name, GameData.fmt_pop(survivors)])
		planet_captured(p, s.race_id, s, survivors)
		return
	p.population -= attack
	var defenders: String = GameData.race(p.owner_race)["adj"]
	if p.population < GameData.MIN_COLONY:
		notify(s, "The invasion of %s failed, but it took the %s colony with it." % [p.body_name, defenders])
		colony_wiped_out(p, s)
		return
	notify(s, "The invasion of %s was beaten back: %s defenders are left.%s" % [p.body_name, GameData.fmt_pop(p.population),
		"" if p.is_defenceless() else " Knock its defences out first: invaders fight twice as hard then."])
	Net.host_colony_changed(p)


## A colony's hull fell: its guns go quiet and it can be invaded.
func colony_knocked_out(p: Planet, attacker: Node) -> void:
	if attacker is Ship:
		attacker.add_score(10, false)
	notify_all("%s's defences are down: land colonists on it to take it." % p.body_name, GameData.race(attacker.race_id)["color"] if attacker is Ship else Color(0.9, 0.9, 0.9))
	Sfx.play("hit", p.global_position, 0.6, 0.6)
	Net.host_colony_changed(p)


## Bombarded or out-fought down to nobody: the world is empty again.
func colony_wiped_out(p: Planet, by: Node) -> void:
	var old := p.owner_race
	p.clear_colony()
	notify_all("The %s colony on %s is gone%s." % [GameData.race(old)["adj"], p.body_name,
		(" (%s)" % by.pilot_name) if by is Ship else ""], Color(0.9, 0.75, 0.6))
	if round_rules != null:
		round_rules.note("%s colony on %s wiped out" % [GameData.race(old)["adj"], p.body_name], old)
	Net.host_colony_changed(p)
	planet_owner_changed.emit(p)


# ----------------------------------------------------------- player aids

func cycle_target(s: Ship) -> void:
	var candidates := []
	for o in ships:
		if o == s or o.dead or not o.visible_to(s):
			continue
		candidates.append(o)
	candidates.sort_custom(func(a, b): return a.global_position.distance_to(s.global_position) < b.global_position.distance_to(s.global_position))
	if candidates.is_empty():
		s.target = null
		return
	var idx := candidates.find(s.target)
	s.target = candidates[(idx + 1) % candidates.size()]


func cycle_nav(s: Ship) -> void:
	var list := planets.filter(func(p): return p.look != "Sun")
	list.sort_custom(func(a, b): return a.global_position.distance_to(s.global_position) < b.global_position.distance_to(s.global_position))
	var idx := list.find(s.nav_target)
	s.nav_target = list[(idx + 1) % list.size()]
	notify(s, "Navigation: %s." % s.nav_target.body_name)


## Open the shipyard: needs a friendly starbase within docking range.
func try_refit(s: Ship) -> void:
	var base := nearest_base(s)
	if base == null:
		notify(s, "The shipyard is at your starbase (fly within docking range and press U).")
		return
	if s.speed() > 220.0:
		notify(s, "Too fast to dock: brake first (X).")
		return
	shipyard_requested.emit(s, base)


## The friendly starbase this ship could dock at, if any.
func nearest_base(s: Ship) -> Ship:
	for o in ships:
		if o.is_station and not o.dead and o.race_id == s.race_id and o.global_position.distance_to(s.global_position) < o.size + 320.0:
			return o
	return null


## Apply a shipyard build to a ship where it sits.
func apply_build(s: Ship, design: Dictionary) -> void:
	if Net.is_client():
		Net.client_build(s, design)   # the host fits it and tells everyone
		return
	var pos := s.global_position
	var vel := s.velocity
	var rot := s.rotation
	s.refit(design)
	s.global_position = pos
	s.velocity = vel
	s.rotation = rot
	s.invulnerable = 2.0
	Net.host_refit(s)
	Sfx.play("dock", pos, 0.8)
	notify(s, "Launched: %s." % design["name"])


var _last_notes := {}           # text -> time, so one event never floods the log


func notify(s: Ship, text: String) -> void:
	if s != null and s.is_player:
		var now := Time.get_ticks_msec() / 1000.0
		if now - float(_last_notes.get(text, -100.0)) < 5.0:
			return
		_last_notes[text] = now
		message.emit(text, Color(0.85, 0.9, 1.0))
	else:
		Net.relay_notify(s, text)


func notify_all(text: String, color: Color) -> void:
	message.emit(text, color)
	Net.relay_notify_all(text, color)


func zoom_by(factor: float) -> void:
	target_zoom = clampf(target_zoom * factor, 0.35, 1.6)
	Settings.camera_zoom = target_zoom
