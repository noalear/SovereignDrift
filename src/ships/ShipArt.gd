## ShipArt -- ship models built from the painted sprite sheets.
##
## Every hull is a top-down painting in its empire's design language
## (assets/ships/<empire>/<Hull>.png): Terran plated grey-blue hulls,
## Arcean organic teal shells with green bioluminescence, Drengin crimson
## blades, Pirate patched-up scrap.  The fitted parts are drawn on top from
## the parts sheet (assets/parts/*.png): a turret model per weapon that
## turns with the pilot's aim, one nozzle per engine, armour plates, shield
## domes, the sensor dish, the reactor core, cloak spine, booster pods,
## warp fins and so on, so a loadout can be read from the ship itself.
## Engine flames, cloaking, hit flashes and battle damage are live effects
## drawn around the model each frame.  Ships point along +X.
class_name ShipArt
extends RefCounted

## Hull sprite length in multiples of the design's size (a radius-like
## number used by the rules).
const LENGTH := 3.0

static var _textures := {}


# ---------------------------------------------------------------- public

## Draw a ship.  `design` is a hull name or a design dictionary.  `xform`
## places the model (Ship draws at identity; menus and the shipyard pass a
## position and scale).  `aims` holds a live angle per weapon for turrets
## that track a target; `flash` whitens the hull for a hit.
static func draw_ship(c: CanvasItem, design: Variant, race_id: int, size: float, base: Color, thrusting: float, cloak_alpha: float, damage: float,
		xform: Transform2D = Transform2D.IDENTITY, aims: Array = [], flash: float = 0.0) -> void:
	var model := build_model(design, race_id, size)
	draw_model(c, model, base, thrusting, cloak_alpha, damage, xform, aims, flash)


static func texture(path: String) -> Texture2D:
	if _textures.has(path):
		return _textures[path]
	var t: Texture2D = load(path) if ResourceLoader.exists(path) else null
	_textures[path] = t
	return t


## Which painting a hull uses for an empire.
static func hull_texture(hull_name: String, race_id: int) -> Texture2D:
	var folder: String = ["arcean", "terran", "drengin", "pirate"][clampi(race_id, 0, 3)]
	var file := hull_name
	if hull_name == "Starbase":
		return texture("res://assets/ships/stations/%s.png" % ["arcean", "terran", "drengin", "drengin"][clampi(race_id, 0, 3)])
	if race_id == 3:
		match hull_name:
			"Destroyer": file = "Raider"
			"Dreadnought": file = "Battleship"
			"Freighter": file = "Cruiser"
	else:
		match hull_name:
			"Raider": file = "Destroyer"
			"Marauder":
				folder = "pirate"
			"Freighter":
				folder = "terran"
				file = "Cruiser"
	return texture("res://assets/ships/%s/%s.png" % [folder, file])


static func part_texture(name: String) -> Texture2D:
	return texture("res://assets/parts/%s.png" % name)


## The parts-sheet icon for a weapon module.
static func weapon_icon(id: String) -> String:
	var mod: Dictionary = GameData.MODULES[id]
	if mod["func"] == "beam":
		return "laser"
	match mod["p"][3]:
		"010": return "rapid"
		"004": return "grenade"
		"018": return "cannon"
		"008", "009": return "missile"
		"021": return "drone"
		"020": return "disruptor"
		"002": return "mine"
	return "torpedo"


## Engine flame colour per empire.
static func glow_color(race_id: int) -> Color:
	match race_id:
		0: return Color(0.55, 1.0, 0.5)
		2: return Color(1.0, 0.5, 0.15)
		3: return Color(1.0, 0.85, 0.4)
	return Color(0.5, 0.8, 1.0)


# ----------------------------------------------------------------- model

## Build the model: the hull sprite rect, nozzles and the part overlays.
static func build_model(design: Variant, race_id: int, size: float) -> Dictionary:
	var hull_name: String = design if design is String else design["name"]
	var fit: Dictionary = {} if design is String else design
	var m := {"hull": hull_name, "race": race_id, "size": size, "seed": hash(hull_name) + race_id * 31,
		"tex": hull_texture(hull_name, race_id), "rect": Rect2(), "nozzles": [], "overlays": [], "turrets": [],
		"glow": glow_color(race_id), "station": hull_name == "Starbase"}
	var tex: Texture2D = m["tex"]
	var length := size * LENGTH
	if tex != null:
		var aspect := float(tex.get_height()) / float(tex.get_width())
		if m["station"]:
			length = size * 2.6
		m["rect"] = Rect2(-length * 0.5, -length * aspect * 0.5, length, length * aspect)
	else:
		m["rect"] = Rect2(-length * 0.5, -length * 0.25, length, length * 0.5)
	var counts := _count_funcs(fit)
	if not m["station"]:
		var n_eng: int = counts.get("eng", 2 if hull_name != "Scout" else 1)
		var eng_tier: int = counts.get("eng_tier", 2)
		m["nozzles"] = _nozzle_points(hull_name, size, n_eng, eng_tier, m["rect"])
		_system_overlays(m, size, counts)
	var weapons: Array = fit["weapons"] if not fit.is_empty() else GameData.SHIPS[hull_name]["weapons"]
	var i := 0
	for w in weapons:
		_turret_overlay(m, w, i, size)
		i += 1
	return m


static func _count_funcs(fit: Dictionary) -> Dictionary:
	var out := {}
	if fit.is_empty():
		return out
	for id in fit["modules"]:
		var f: String = GameData.MODULES[id]["func"]
		out[f] = out.get(f, 0) + 1
		var t := GameData.tier(id)
		out[f + "_tier"] = maxi(out.get(f + "_tier", 0), t)
	return out


static func _nozzle_points(hull_name: String, size: float, n: int, tier: int, rect: Rect2) -> Array:
	n = clampi(n, 1, 4)
	var spread := rect.size.y * 0.28
	if hull_name == "Scout":
		spread = rect.size.y * 0.18
	var out := []
	var r := size * (0.08 + 0.03 * tier) * (1.0 if n <= 2 else 0.8)
	for i in n:
		var y := 0.0 if n == 1 else lerpf(-spread, spread, float(i) / float(n - 1))
		out.append({"pos": Vector2(rect.position.x + r * 0.6, y), "r": r})
	return out


static func _overlay(m: Dictionary, icon: String, pos: Vector2, width: float, rot: float = 0.0, alpha: float = 1.0, anchor: float = 0.5) -> void:
	var t := part_texture(icon)
	if t == null:
		return
	m["overlays"].append({"tex": t, "pos": pos, "w": width, "h": width * t.get_height() / t.get_width(), "rot": rot, "a": alpha, "anchor": anchor})


## Fitted systems, placed where they read best on a top-down hull.
static func _system_overlays(m: Dictionary, size: float, counts: Dictionary) -> void:
	var rect: Rect2 = m["rect"]
	var hw := rect.size.y * 0.5          # half height of the hull sprite
	var s := size
	# engines: a nozzle block per engine at the stern, booster pods beside them
	for nz in m["nozzles"]:
		var p: Vector2 = nz["pos"]
		var r: float = nz["r"]
		_overlay(m, "nozzle", p + Vector2(r * 0.9, 0), r * 2.1, 0.0, 0.95)
		if counts.get("boost", 0) > 0:
			_overlay(m, "booster", p + Vector2(r * 1.6, -r * 1.5), r * 1.9, 0.0, 0.95)
			_overlay(m, "booster", p + Vector2(r * 1.6, r * 1.5), r * 1.9, 0.0, 0.95)
	# quantum drive fins over the stern
	var qt: int = counts.get("qdrv_tier", 0)
	if qt > 0:
		_overlay(m, "qdrv", Vector2(rect.position.x + s * 0.55, 0), s * (0.36 + 0.06 * qt), PI, 0.9)
	# reactor core at the heart, brighter window with tier
	var pt: int = counts.get("power_tier", 0)
	if pt > 0:
		_overlay(m, "reactor", Vector2(-s * 0.15, 0), s * (0.22 + 0.03 * pt), 0.0, 0.95)
	# shield emitter domes along the flanks, a pair per module
	var shields: int = counts.get("shield", 0)
	for k in shields:
		var x := s * (0.15 - 0.4 * k)
		_overlay(m, "shield", Vector2(x, -hw * 0.6), s * 0.16, 0.0, 0.95)
		_overlay(m, "shield", Vector2(x, hw * 0.6), s * 0.16, 0.0, 0.95)
	# armour plates on the edges, one per plate, bigger by tier
	var plates: int = counts.get("armor", 0)
	var at: int = counts.get("armor_tier", 1)
	for k in plates:
		var x := s * (0.45 - 0.5 * k)
		var w := s * (0.2 + 0.04 * at)
		_overlay(m, "armor", Vector2(x, -hw * 0.82), w, 0.0, 0.95)
		_overlay(m, "armor", Vector2(x, hw * 0.82), w, PI, 0.95)
	# sensor dish forward, bigger by tier
	var st: int = counts.get("sensor_tier", 0)
	if st > 0:
		_overlay(m, "sensor", Vector2(s * 0.4, -hw * 0.35), s * (0.11 + 0.03 * st), 0.0, 0.95)
	# cloaking spine down the middle
	if counts.get("stealth", 0) > 0:
		_overlay(m, "cloak", Vector2(-s * 0.55, 0), s * 0.18, PI * 0.5, 0.9)
	# repair hatch, regenerator node, ram prow
	if counts.get("repair", 0) > 0:
		_overlay(m, "repair", Vector2(-s * 0.45, hw * 0.35), s * 0.2, 0.0, 0.95)
	if counts.get("regen", 0) > 0:
		_overlay(m, "regen", Vector2(s * 0.3, hw * 0.3), s * 0.2, 0.0, 0.95)
	if counts.get("ram", 0) > 0:
		_overlay(m, "ram", Vector2(rect.end.x + s * 0.05, 0), s * 0.5, 0.0, 1.0, 0.25)


static func _turret_overlay(m: Dictionary, w: Array, index: int, size: float) -> void:
	var id: String = w[0]
	var mount := Vector2.RIGHT.rotated(deg_to_rad(float(w[1]))) * (float(w[2]) + size * 0.5) * (0.75 if w[2] == 0 else 1.0)
	var cls := GameData.weapon_class(id)
	var r := maxf(size * (0.09 + 0.045 * cls), 2.5)
	var icon := weapon_icon(id)
	var t := part_texture(icon)
	if t == null:
		return
	var width := r * (2.6 if icon in ["cannon", "disruptor", "torpedo", "mine"] else 2.0)
	m["turrets"].append({"tex": t, "pos": mount, "w": width, "h": width * t.get_height() / t.get_width(),
		"fire": deg_to_rad(float(w[3])), "index": index, "turret": w[5] - w[4] >= 150, "anchor": 0.42 if icon != "mine" else 0.5})


# ------------------------------------------------------------------ draw

static func draw_model(c: CanvasItem, m: Dictionary, base: Color, thrusting: float, cloak_alpha: float, damage: float,
		xform: Transform2D = Transform2D.IDENTITY, aims: Array = [], flash: float = 0.0) -> void:
	var a := cloak_alpha
	var size: float = m["size"]
	var rect: Rect2 = m["rect"]
	c.draw_set_transform_matrix(xform)
	# engine flames under everything
	if thrusting > 0.01 and a > 0.05:
		var glow: Color = m["glow"]
		for nz in m["nozzles"]:
			var p: Vector2 = nz["pos"]
			var r: float = nz["r"]
			var l := r * (3.0 + 5.0 * thrusting) * randf_range(0.85, 1.15)
			c.draw_colored_polygon(PackedVector2Array([p + Vector2(r * 0.5, -r * 1.3), p + Vector2(-l * 1.3, 0), p + Vector2(r * 0.5, r * 1.3)]), Color(glow.r, glow.g, glow.b, 0.3 * a))
			c.draw_colored_polygon(PackedVector2Array([p + Vector2(r * 0.5, -r * 0.9), p + Vector2(-l, 0), p + Vector2(r * 0.5, r * 0.9)]), Color(glow.r, glow.g, glow.b, 0.75 * a))
			c.draw_colored_polygon(PackedVector2Array([p + Vector2(r * 0.5, -r * 0.45), p + Vector2(-l * 0.55, 0), p + Vector2(r * 0.5, r * 0.45)]), Color(1, 1, 1, 0.9 * a))
	# the hull painting
	var bright := 1.0 + flash * 2.0
	var dark := 1.0 - 0.3 * clampf(damage, 0.0, 1.0)
	var mod := Color(bright * dark, bright * dark, bright * dark, a)
	var tex: Texture2D = m["tex"]
	if tex != null:
		c.draw_texture_rect(tex, rect, false, mod)
	else:
		c.draw_rect(rect, Color(base.r, base.g, base.b, a))
	# fitted systems
	for o in m["overlays"]:
		_draw_icon(c, xform, o["tex"], o["pos"], o["w"], o["h"], o["rot"], Color(bright * dark, bright * dark, bright * dark, a * o["a"]), o["anchor"])
	# weapons, turrets turned to their live aim
	for t in m["turrets"]:
		var ang: float = t["fire"]
		if t["turret"] and t["index"] < aims.size():
			ang = aims[t["index"]]
		_draw_icon(c, xform, t["tex"], t["pos"], t["w"], t["h"], ang, Color(bright * dark, bright * dark, bright * dark, a), t["anchor"])
	c.draw_set_transform_matrix(xform)
	# idle nozzle glow
	if a > 0.05 and not m["station"]:
		var glow: Color = m["glow"]
		for nz in m["nozzles"]:
			c.draw_circle(nz["pos"], nz["r"] * 0.6, Color(glow.r, glow.g, glow.b, (0.3 + 0.6 * thrusting) * a))
	# damage: scorch marks and sparks of exposed frame
	if damage > 0.25:
		var rng := RandomNumberGenerator.new()
		rng.seed = m["seed"]
		var marks := int(damage * 9.0)
		for i in marks:
			var p := Vector2(rng.randf_range(rect.position.x * 0.8, rect.end.x * 0.8), rng.randf_range(rect.position.y * 0.6, rect.end.y * 0.6))
			c.draw_circle(p, size * rng.randf_range(0.07, 0.16), Color(0.03, 0.02, 0.02, 0.85 * a))
			if damage > 0.6 and i % 2 == 0:
				c.draw_line(p + Vector2(-size * 0.06, 0), p + Vector2(size * 0.06, 0), Color(1.0, 0.5, 0.2, 0.5 * a * randf()), 1.0)
	c.draw_set_transform_matrix(Transform2D.IDENTITY)


static func _draw_icon(c: CanvasItem, xform: Transform2D, tex: Texture2D, pos: Vector2, w: float, h: float, rot: float, mod: Color, anchor: float) -> void:
	c.draw_set_transform_matrix(xform * Transform2D(rot, pos))
	c.draw_texture_rect(tex, Rect2(-w * anchor, -h * 0.5, w, h), false, mod)
