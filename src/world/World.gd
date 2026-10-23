## World -- the Sol system scenario: bodies, ships, missiles, pickups,
## collisions, scoring, respawns and planet capture.
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

var planets: Array[Planet] = []
var ships: Array[Ship] = []
var missiles: Array = []
var powerups: Array = []
var camera: Camera2D
var fx: Fx
var starfield: Starfield
var player: Ship = null
var paused := false
var elapsed := 0.0
var respawn_queue: Array = []   # [ship, time]
var planet_by_name := {}
var bodies_layer: Node2D
var ships_layer: Node2D
var missiles_layer: Node2D
var pickups_layer: Node2D
var planet_counts := {}
var game_over := false
var target_zoom := 1.0
var shake := 0.0                 # camera shake left, in pixels
var slowmo := 0.0                # seconds of kill slow-motion left
var intensity_timer := 0.0
var last_player_damage := -100.0
var last_player_fire := -100.0
var income_timer := 60.0         # planets pay their empire every minute
var round_rules: Round = null     # the Conquest rules; the host runs them
const START_CREDITS := 250
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
	camera.position_smoothing_enabled = true
	camera.position_smoothing_speed = 6.0
	add_child(camera)
	starfield.camera = camera
	target_zoom = Settings.camera_zoom
	camera.zoom = Vector2.ONE * target_zoom * BASE_ZOOM
	_build_system()
	if Net.is_client():
		Net.client_attach(self)   # the ships come from the host
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


func _spawn_pilots() -> void:
	# the player
	player = _make_ship(Settings.pilot_name, Settings.race_id, _starting_design(Settings.race_id, Settings.career), Settings.pilot_name + "'s ship", true)
	player.score = Settings.career["score"]
	player.kills = Settings.career["kills"]
	player.rank = GameData.rank_for(player.score, player.kills)
	player.credits = START_CREDITS
	var pc := PlayerController.new()
	pc.name = "Controller"
	player.add_child(pc)
	pc.setup(player, self)
	_place_at_home(player)
	# starbases for the three empires
	for race_id in GameData.STARBASES:
		var home: Planet = planet_by_name[GameData.STARBASES[race_id]]
		var base := _make_ship("%s Command" % GameData.race(race_id)["name"], race_id, "Starbase", GameData.race(race_id)["name"] + " Starbase", false)
		base.global_position = home.global_position + Vector2(home.radius + 420.0, 0).rotated(randf() * TAU)
		base.velocity = home.velocity
		var ai := AIPilot.new()
		base.add_child(ai)
		ai.setup(base, self, home, 14)
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
		_place_at_home(s)
		# spread the pilots out so the fight does not start on top of the player
		s.global_position += Vector2(randf_range(-1400, 1400), randf_range(-1400, 1400))
		per_race[race_id] += 1


func _starting_design(race_id: int, career: Dictionary) -> String:
	var level := GameData.rank_for(career.get("score", 0), career.get("kills", 0))
	return _design_for_rank(level, race_id)


func _design_for_rank(level: int, race_id: int) -> String:
	var options := GameData.ship_names_for_rank(level, false)
	if options.is_empty():
		return "Scout"
	# computer pilots prefer the best hull they may fly
	return options[options.size() - 1]


func _home_planet(race_id: int) -> Planet:
	var name: String = GameData.STARBASES.get(race_id, GameData.race(race_id)["home"])
	return planet_by_name.get(name, planets[0])


func _make_ship(pilot: String, race_id: int, design: String, ship_name: String, is_player: bool) -> Ship:
	var s := Ship.new()
	s.name = pilot
	s.is_player = is_player
	ships_layer.add_child(s)
	s.setup(self, design, race_id, pilot, ship_name)
	s.died.connect(_on_ship_died)
	s.damaged.connect(_on_ship_damaged)
	s.rank_changed.connect(_on_rank_changed)
	ships.append(s)
	return s


func _place_at_home(s: Ship) -> void:
	var home := _home_planet(s.race_id)
	var a := randf() * TAU
	s.global_position = home.global_position + Vector2(cos(a), sin(a)) * (home.radius + 260.0)
	s.velocity = home.velocity + Vector2(cos(a), sin(a)) * 20.0
	s.rotation = a                  # nose away from the planet, so thrust is safe


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
	if player != null and not player.dead:
		# look a little ahead of the ship's motion
		camera.global_position = player.global_position + player.velocity * 0.35
	elif player != null:
		camera.global_position = player.global_position
	# shake decays quickly; the offset is in screen pixels so divide by zoom
	if shake > 0.0:
		shake = maxf(shake - delta * 18.0, 0.0)
		var a := randf() * TAU
		camera.offset = Vector2(cos(a), sin(a)) * shake * Settings.screen_shake / camera.zoom.x
	else:
		camera.offset = Vector2.ZERO
	if slowmo > 0.0:
		slowmo -= delta / maxf(Engine.time_scale, 0.01)
		if slowmo <= 0.0:
			Engine.time_scale = 1.0
	intensity_timer -= delta
	if intensity_timer <= 0.0:
		intensity_timer = 0.5
		Music.set_intensity(_combat_intensity())


func _physics_process(delta: float) -> void:
	if paused:
		return
	elapsed += delta
	missiles = missiles.filter(func(m): return is_instance_valid(m))
	powerups = powerups.filter(func(p): return is_instance_valid(p))
	for p in planets:
		p.update_orbit(delta)
	if Net.is_client():
		Net.client_tick(delta)   # the host collides, scores and respawns
		return
	sun_heat = 0.0
	_collide_ships(delta)
	_sun_warning(delta)
	_collide_missiles()
	_collect_powerups()
	_respawns()
	_score_planets(delta)


func set_paused(p: bool) -> void:
	paused = p
	get_tree().paused = p
	if p:
		Engine.time_scale = 1.0
		slowmo = 0.0


func add_shake(amount: float) -> void:
	shake = minf(shake + amount, 28.0)


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

func gravity_at(pos: Vector2, _mass: float) -> Vector2:
	var g := Vector2.ZERO
	for p in planets:
		var to := p.global_position - pos
		var d := to.length()
		if d < 1.0:
			continue
		var strength := 0.0
		if p.look == "Sun":
			# the sun pulls across the inner system
			strength = 2.2e8 / maxf(d * d, 1.0)
			if d < p.radius * 3.0:
				strength *= 2.0
		else:
			var reach := p.radius * 7.0
			if d > reach:
				continue
			strength = p.radius * 55.0 / maxf(d, p.radius) * (1.0 - d / reach)
		g += to.normalized() * minf(strength, 400.0)
	return g


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
		if s.dead or s.race_id == race_id:
			continue
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
		if s.dead or s.race_id == viewer.race_id or s.is_station:
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
		if s.dead:
			continue
		var d := pos.distance_to(s.global_position)
		if d < s.size + tolerance and d < best_d:
			best_d = d
			best = s
	return best


func enemy_planets(race_id: int) -> Array:
	var out := []
	for p in planets:
		if p.is_colonised() and p.owner_race != race_id:
			out.append(p)
	return out


func _collide_ships(delta: float) -> void:
	for i in ships.size():
		var a := ships[i]
		if a.dead:
			continue
		# planets and the sun
		for p in planets:
			var to := a.global_position - p.global_position
			var d := to.length()
			var min_d := p.radius + a.size * 0.7
			if p.look == "Sun":
				# heat ramps up over two radii above the surface: a warning
				# first, then real damage, then a scorching bounce off the
				# corona rather than an instant death
				var heat := clampf(1.0 - (d - p.radius) / (p.radius * 2.2), 0.0, 1.0)
				if a == player:
					sun_heat = heat
				if heat > 0.25:
					var t := (heat - 0.25) / 0.75
					a.take_damage((30.0 + 1100.0 * t * t) * delta, p)
				if d < min_d:
					var n := to.normalized() if d > 0.001 else Vector2.RIGHT
					a.global_position = p.global_position + n * min_d
					var vn := a.velocity.dot(n)
					if vn < 0.0:
						a.velocity -= n * vn * 1.6
					a.take_damage(900.0 * delta + 120.0, p)
				continue
			if d < min_d:
				var n := to.normalized()
				a.global_position = p.global_position + n * min_d
				var vn := (a.velocity - p.velocity).dot(n)
				if vn < 0.0:
					a.velocity -= n * vn * 1.4
					var impact := -vn
					if impact > 60.0:
						a.take_damage((impact - 60.0) * 0.6, p, a.global_position)
						Sfx.play("collide", a.global_position, clampf(impact / 200.0, 0.4, 1.0))
		# other ships
		for j in range(i + 1, ships.size()):
			var b := ships[j]
			if b.dead:
				continue
			var to := b.global_position - a.global_position
			var d := to.length()
			var min_d := (a.size + b.size) * 0.75
			if d < min_d and d > 0.001:
				var n := to.normalized()
				var rel := (b.velocity - a.velocity).dot(n)
				var total := a.mass + b.mass
				var push := (min_d - d)
				a.global_position -= n * push * (b.mass / total)
				b.global_position += n * push * (a.mass / total)
				if rel < 0.0:
					var impulse := -rel * 1.1
					a.velocity -= n * impulse * (b.mass / total)
					b.velocity += n * impulse * (a.mass / total)
					if impulse > 90.0:
						var dmg := (impulse - 90.0) * 0.5
						# a ram prow turns a collision into a weapon when it hits nose first
						var a_ram := 1.0
						var b_ram := 1.0
						if a.ram_power > 0.0 and a.forward().dot(n) > 0.7:
							a_ram = a.ram_power
						if b.ram_power > 0.0 and b.forward().dot(-n) > 0.7:
							b_ram = b.ram_power
						a.take_damage(dmg * (b.mass / total) * 2.0 * b_ram / a_ram, b, a.global_position + n * a.size)
						b.take_damage(dmg * (a.mass / total) * 2.0 * a_ram / b_ram, a, b.global_position - n * b.size)
						if a_ram > 1.0 or b_ram > 1.0:
							add_shake(6.0 if a.is_player or b.is_player else 0.0)


## Beep and say so while the player is in the Sun's heat.
func _sun_warning(delta: float) -> void:
	if player == null or player.dead or sun_heat <= 0.0:
		_heat_alarm = 0.0
		return
	_heat_alarm -= delta
	if _heat_alarm <= 0.0:
		_heat_alarm = lerpf(1.6, 0.35, sun_heat)
		Sfx.play_ui("alarm", 0.35 + 0.5 * sun_heat)
		if sun_heat < 0.3:
			notify(player, "Warning: close to the Sun. Turn away.")


func _collide_missiles() -> void:
	for m: Missile in missiles:
		if not is_instance_valid(m) or m.exploded:
			continue
		var hit := false
		for s in ships:
			if s.dead or s == m.owner_ship:
				continue
			if m.armed > 0.0 and s.race_id == m.owner_race:
				continue
			if s.race_id == m.owner_race and m.missile_id != "002":
				continue
			if m.check_hit(s, s.size * 0.8):
				m.explode(s)
				hit = true
				break
		if hit:
			continue
		for p in planets:
			var d := m.global_position.distance_to(p.global_position)
			if d < p.radius + m.radius:
				if p.is_colonised() and p.owner_race != m.owner_race:
					m.explode(p)
				else:
					m.exploded = true
					fx.spark(m.global_position, 3, m.color)
					m.queue_free()
				break


func raycast_beam(shooter: Node, origin: Vector2, dir: Vector2, range_px: float) -> Dictionary:
	var best_t := range_px
	var best: Node2D = null
	var shooter_race: int = shooter.race_id if "race_id" in shooter else shooter.owner_race
	for s in ships:
		if s.dead or s == shooter or s.race_id == shooter_race:
			continue
		var t := _ray_circle(origin, dir, s.global_position, s.size * 0.8)
		if t >= 0.0 and t < best_t:
			best_t = t
			best = s
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
			hit.take_damage(m.warhead, attacker if attacker is Ship or attacker is Planet else m.owner_ship, pos)
			if hit is Ship and m.data.has("drain"):
				hit.energy = maxf(hit.energy - m.data["drain"], 0.0)
				if hit.cloaked:
					hit.set_cloak(false)
		fx.explosion(pos, 6.0 + m.radius * 2.0, m.color, false, "impact")
		return
	# area damage falls off with distance
	for s in ships:
		if s.dead:
			continue
		var d := pos.distance_to(s.global_position) - s.size * 0.5
		if d < splash:
			var f := 1.0 - maxf(d, 0.0) / splash
			s.take_damage(m.warhead * f, attacker if attacker is Ship else null, s.global_position)
	for p in planets:
		var d := pos.distance_to(p.global_position) - p.radius
		if d < splash and p.is_colonised():
			var f := 1.0 - maxf(d, 0.0) / splash
			p.take_damage(m.warhead * f * 0.5, attacker if attacker is Ship else null)
	fx.explosion(pos, splash * 0.5, m.color, m.missile_id == "002", "explode")


func spawn_sparks(pos: Vector2, n: int, color: Color) -> void:
	fx.spark(pos, n, color)


func spawn_powerup(id: String, pos: Vector2, vel: Vector2) -> void:
	var p := Powerup.new()
	pickups_layer.add_child(p)
	p.setup(id, pos, vel)
	powerups.append(p)
	Net.host_powerup_spawned(p)


func _collect_powerups() -> void:
	for p in powerups:
		if not is_instance_valid(p):
			continue
		for s in ships:
			if s.dead or s.is_station:
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
					p.queue_free()
				break


# ----------------------------------------------------------- game events

func _on_ship_damaged(s: Ship, amount: float) -> void:
	if s.is_player:
		last_player_damage = elapsed
		add_shake(clampf(amount * 0.04, 1.0, 10.0))


func _on_ship_died(s: Ship, killer: Node) -> void:
	var big := s.size > 40.0
	fx.explosion(s.global_position, s.size * 1.6, s.race_color(), big)
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
		spawn_powerup(id, s.global_position + Vector2(cos(a), sin(a)) * s.size * 0.5, s.velocity * 0.4 + Vector2(cos(a), sin(a)) * randf_range(30.0, 90.0))
	# the purse spills: a credit chip, and now and then a blueprint of an
	# empire part the wreck carried
	var purse := 20 + s.rank * 6 + (200 if s.is_station else 0)
	spawn_powerup("cr:%d" % purse, s.global_position + Vector2(randf_range(-20, 20), randf_range(-20, 20)), s.velocity * 0.3 + Vector2(randf_range(-40, 40), randf_range(-40, 40)))
	var flavour := []
	for id in s.design["modules"]:
		if GameData.part_race(id) == s.race_id:
			flavour.append(id)
	for w in s.design["weapons"]:
		if GameData.part_race(w[0]) == s.race_id:
			flavour.append(w[0])
	if not flavour.is_empty() and randf() < (0.35 if big else 0.2):
		var a := randf() * TAU
		spawn_powerup("bp:" + flavour[randi() % flavour.size()], s.global_position, s.velocity * 0.3 + Vector2(cos(a), sin(a)) * 50.0)
	var killer_ship: Ship = killer as Ship
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
		Settings.career["deaths"] = Settings.career.get("deaths", 0) + 1
		Settings.save_settings()
		player_died.emit()
	respawn_queue.append([s, elapsed + RESPAWN_DELAY + (6.0 if s.is_station else 0.0)])


func _respawns() -> void:
	var keep := []
	for entry in respawn_queue:
		var s: Ship = entry[0]
		if elapsed >= entry[1]:
			var home := _home_planet(s.race_id)
			if s.is_station:
				s.respawn(home.global_position + Vector2(home.radius + 420.0, 0).rotated(randf() * TAU), home.velocity)
			else:
				_place_at_home(s)
				s.respawn(s.global_position, s.velocity)
			Net.host_ship_respawned(s)
			if s.is_player:
				player_respawned.emit()
				notify(s, "Launched from %s." % home.body_name)
		else:
			keep.append(entry)
	respawn_queue = keep


func _on_rank_changed(s: Ship, level: int) -> void:
	if s.is_player:
		notify_all("%s promoted to %s!" % [s.pilot_name, GameData.rank_name(level)], Color(1, 0.9, 0.5))
		Sfx.play_ui("rankup", 0.8)
		Settings.career["score"] = s.score
		Settings.career["kills"] = s.kills
		Settings.save_settings()


func planet_captured(p: Planet, winner: int, attacker: Node) -> void:
	var old := p.owner_race
	p.surrender_to(winner)
	Net.host_planet_captured(p, winner)
	var mult := round_rules.bonus_multiplier() if round_rules != null else 1.0
	if attacker is Ship:
		attacker.add_score(50, false)
		attacker.captures += 1
		if attacker.is_player:
			var bonus := int(200 * mult)
			attacker.earn(bonus)
			notify(attacker, "Capture bonus: %d credits." % bonus)
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
	if player != null:
		Settings.career["rounds"] = Settings.career.get("rounds", 0) + 1
		if winner == player.race_id:
			Settings.career["wins"] = Settings.career.get("wins", 0) + 1
		Settings.career["score"] = player.score
		Settings.career["kills"] = player.kills
		Settings.save_settings()
	Sfx.play_ui("rankup", 1.0)
	round_over.emit(result)


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
	var pos := s.global_position
	var vel := s.velocity
	var rot := s.rotation
	s.refit(design)
	s.global_position = pos
	s.velocity = vel
	s.rotation = rot
	s.invulnerable = 2.0
	Sfx.play("dock", pos, 0.8)
	notify(s, "Launched: %s." % design["name"])


func notify(s: Ship, text: String) -> void:
	if s != null and s.is_player:
		message.emit(text, Color(0.85, 0.9, 1.0))
	else:
		Net.relay_notify(s, text)


func notify_all(text: String, color: Color) -> void:
	message.emit(text, color)
	Net.relay_notify_all(text, color)


func zoom_by(factor: float) -> void:
	target_zoom = clampf(target_zoom * factor, 0.35, 1.6)
	Settings.camera_zoom = target_zoom
