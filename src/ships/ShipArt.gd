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
static var _masks := {}
static var _spots := {}
static var _models := {}
static var _engines := {}


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


## The parts-sheet picture for any module, for salvage and the hold.
static func module_icon(id: String) -> String:
	var f: String = GameData.MODULES[id]["func"]
	match f:
		"beam", "tube": return weapon_icon(id)
		"shield": return "shield"
		"armor": return "armor"
		"eng": return "nozzle"
		"sensor": return "sensor"
		"stealth": return "cloak"
		"repair": return "repair"
		"regen": return "regen"
		"ram": return "ram"
		"boost": return "booster"
		"qdrv": return "qdrv"
	return "reactor"


## Engine flame colour per empire.
static func glow_color(race_id: int) -> Color:
	match race_id:
		0: return Color(0.55, 1.0, 0.5)
		2: return Color(1.0, 0.5, 0.15)
		3: return Color(1.0, 0.85, 0.4)
	return Color(0.5, 0.8, 1.0)


## A light wash over the parts sheet's grey metal so fitted parts sit in
## the empire's palette instead of looking pasted on.
static func part_tint(race_id: int) -> Color:
	match race_id:
		0: return Color(0.78, 1.0, 0.9)
		2: return Color(1.0, 0.78, 0.74)
		3: return Color(1.0, 0.9, 0.74)
	return Color(0.94, 0.97, 1.0)


# ------------------------------------------------------------ hull mask

## Where the painting is solid: a coarse grid holding, per cell, how far
## it lies inside the hull (in cells; 0 is empty space).  Parts are kept
## on cells deep enough to hold them.
static func hull_mask(tex: Texture2D) -> Dictionary:
	if tex == null:
		return {}
	var key := tex.resource_path
	if _masks.has(key):
		return _masks[key]
	var out := {}
	var img := tex.get_image()
	if img != null:
		if img.is_compressed():
			img.decompress()
		var gw := 96
		var gh := maxi(int(round(gw * float(img.get_height()) / img.get_width())), 8)
		var iw := img.get_width()
		var ih := img.get_height()
		var depth := PackedFloat32Array()
		depth.resize(gw * gh)
		for y in gh:
			for x in gw:
				var px := mini(int((x + 0.5) * iw / gw), iw - 1)
				var py := mini(int((y + 0.5) * ih / gh), ih - 1)
				depth[y * gw + x] = 1e6 if img.get_pixel(px, py).a > 0.6 else 0.0
		# two-pass chamfer distance; the image border counts as space
		for y in gh:
			for x in gw:
				var i := y * gw + x
				if depth[i] <= 0.0:
					continue
				var d := minf(depth[i], (depth[i - 1] if x > 0 else 0.0) + 1.0)
				d = minf(d, (depth[i - gw] if y > 0 else 0.0) + 1.0)
				d = minf(d, (depth[i - gw - 1] if x > 0 and y > 0 else 0.0) + 1.414)
				d = minf(d, (depth[i - gw + 1] if x < gw - 1 and y > 0 else 0.0) + 1.414)
				depth[i] = d
		for y in range(gh - 1, -1, -1):
			for x in range(gw - 1, -1, -1):
				var i := y * gw + x
				if depth[i] <= 0.0:
					continue
				var d := minf(depth[i], (depth[i + 1] if x < gw - 1 else 0.0) + 1.0)
				d = minf(d, (depth[i + gw] if y < gh - 1 else 0.0) + 1.0)
				d = minf(d, (depth[i + gw + 1] if x < gw - 1 and y < gh - 1 else 0.0) + 1.414)
				d = minf(d, (depth[i + gw - 1] if x > 0 and y < gh - 1 else 0.0) + 1.414)
				depth[i] = d
		out = {"gw": gw, "gh": gh, "depth": depth}
	_masks[key] = out
	return out


## The engines painted on a hull: bright, saturated glow at the stern,
## grouped into bands.  Each is {"x", "y", "h"} as fractions of the
## painting (x from the left edge, y from the centre line, h tall).
static func painted_engines(tex: Texture2D) -> Array:
	if tex == null:
		return []
	var key := tex.resource_path
	if _engines.has(key):
		return _engines[key]
	var out := []
	var img := tex.get_image()
	if img != null:
		if img.is_compressed():
			img.decompress()
		var w := img.get_width()
		var h := img.get_height()
		var bands := []          # [y0, y1, pixel count, rearmost x]
		for y in h:
			var n := 0
			var rear := w
			for x in int(w * 0.3):
				var c := img.get_pixel(x, y)
				var hi := maxf(c.r, maxf(c.g, c.b))
				if c.a > 0.5 and hi > 0.84 and hi - minf(c.r, minf(c.g, c.b)) > 0.16:
					n += 1
					rear = mini(rear, x)
			if n == 0:
				continue
			if not bands.is_empty() and y - bands[-1][1] <= 3:
				bands[-1][1] = y
				bands[-1][2] += n
				bands[-1][3] = mini(bands[-1][3], rear)
			else:
				bands.append([y, y, n, rear])
		var most := 0
		for b in bands:
			most = maxi(most, b[2])
		for b in bands:
			if b[2] >= 6 and b[2] >= most * 0.25:
				out.append({"x": float(b[3]) / w, "y": (b[0] + b[1]) * 0.5 / h - 0.5, "h": float(b[1] - b[0] + 1) / h})
		if out.size() > 6:
			out.sort_custom(func(a, b) -> bool: return a["h"] > b["h"])
			out = out.slice(0, 6)
	_engines[key] = out
	return out


## How deep a model-space point lies inside the hull, in model units.
static func _depth_at(mask: Dictionary, rect: Rect2, p: Vector2) -> float:
	var gw: int = mask["gw"]
	var gh: int = mask["gh"]
	var x := int(floor((p.x - rect.position.x) / rect.size.x * gw))
	var y := int(floor((p.y - rect.position.y) / rect.size.y * gh))
	if x < 0 or y < 0 or x >= gw or y >= gh:
		return 0.0
	return mask["depth"][y * gw + x] * rect.size.x / gw


static func _clear_of(p: Vector2, taken: Array, spacing: float) -> bool:
	for q in taken:
		if p.distance_to(q) < spacing:
			return false
	return true


## The nearest point to `p` that sits at least `need` inside the hull and
## `spacing` away from parts already placed.  Falls back to ignoring the
## spacing, then to the deepest point of the hull.
static func _snap(mask: Dictionary, rect: Rect2, p: Vector2, need: float, taken: Array = [], spacing: float = 0.0) -> Vector2:
	if mask.is_empty():
		return p
	if _depth_at(mask, rect, p) >= need and _clear_of(p, taken, spacing):
		return p
	var gw: int = mask["gw"]
	var gh: int = mask["gh"]
	var depth: PackedFloat32Array = mask["depth"]
	var cell := Vector2(rect.size.x / gw, rect.size.y / gh)
	var need_c := need / cell.x
	var deep := p
	var deep_v := -1.0
	for pass_i in 2:
		var best := p
		var best_d := INF
		for y in gh:
			for x in gw:
				var dv := depth[y * gw + x]
				if dv <= 0.0:
					continue
				var q := rect.position + Vector2((x + 0.5) * cell.x, (y + 0.5) * cell.y)
				var dd := q.distance_squared_to(p)
				if pass_i == 0 and (dv > deep_v + 0.01 or (absf(dv - deep_v) <= 0.01 and dd < deep.distance_squared_to(p))):
					deep_v = dv
					deep = q
				if dv < need_c or dd >= best_d:
					continue
				if pass_i == 0 and not _clear_of(q, taken, spacing):
					continue
				best_d = dd
				best = q
		if best_d < INF:
			return best
	return deep


static func _hull_rect(tex: Texture2D, size: float, station: bool) -> Rect2:
	var length := size * LENGTH
	if tex == null:
		return Rect2(-length * 0.5, -length * 0.25, length, length * 0.5)
	var aspect := float(tex.get_height()) / float(tex.get_width())
	if station:
		length = size * 2.6
	return Rect2(-length * 0.5, -length * aspect * 0.5, length, length * aspect)


## The turret radius for a weapon class on a hull of this size.
static func turret_radius(size: float, cls: int) -> float:
	return maxf(size * (0.09 + 0.045 * cls), 2.5)


## Where each of a hull's hardpoints sits on its painting.  The original
## game's mount radii were made for its own hull shapes, so each mount is
## moved to the nearest spot on the painted hull deep enough for its
## turret, clear of the other turrets, and mirrored mounts stay mirrored.
static func hardpoint_spots(hull_name: String, race_id: int, size: float) -> Array:
	var key := "%s|%d|%.2f" % [hull_name, race_id, size]
	if _spots.has(key):
		return _spots[key]
	var tex := hull_texture(hull_name, race_id)
	var rect := _hull_rect(tex, size, hull_name == "Starbase")
	var mask := hull_mask(tex)
	var raws := []
	var out := []
	for hp in GameData.hull_hardpoints(hull_name):
		var raw := raw_mount(float(hp[0]), float(hp[1]), size)
		var r := turret_radius(size, hp[5])
		var spot := Vector2.INF
		# keep a mirrored pair mirrored when the painting allows it
		for k in raws.size():
			var q: Vector2 = raws[k]
			if absf(q.x - raw.x) < 0.5 and absf(q.y + raw.y) < 0.5 and absf(raw.y) > 0.5:
				var m := Vector2(out[k].x, -out[k].y)
				if mask.is_empty() or _depth_at(mask, rect, m) >= r * 0.8:
					spot = m
				break
		if spot == Vector2.INF:
			spot = _snap(mask, rect, raw, r * 0.8, out, r * 1.7)
		raws.append(raw)
		out.append(spot)
	_spots[key] = out
	return out


## A mount position as the original game stored it.
static func raw_mount(angle_deg: float, dist: float, size: float) -> Vector2:
	return Vector2.RIGHT.rotated(deg_to_rad(angle_deg)) * (dist + size * 0.5) * (0.75 if dist == 0.0 else 1.0)


## Where a design's weapon `w` ([id, angle, radius, ...]) sits on the hull.
static func weapon_spot(hull_name: String, race_id: int, size: float, w: Array) -> Vector2:
	var hps := GameData.hull_hardpoints(hull_name)
	var spots := hardpoint_spots(hull_name, race_id, size)
	for i in hps.size():
		if float(hps[i][0]) == float(w[1]) and float(hps[i][1]) == float(w[2]):
			return spots[i]
	var tex := hull_texture(hull_name, race_id)
	var r := turret_radius(size, GameData.weapon_class(w[0]))
	return _snap(hull_mask(tex), _hull_rect(tex, size, hull_name == "Starbase"), raw_mount(float(w[1]), float(w[2]), size), r * 0.8)


# ----------------------------------------------------------------- model

## Build the model: the hull sprite rect, nozzles and the part overlays.
static func build_model(design: Variant, race_id: int, size: float) -> Dictionary:
	var key := "%s|%d|%.2f" % [str(design), race_id, size]
	if _models.has(key):
		return _models[key]
	if _models.size() > 256:
		_models.clear()
	var hull_name: String = design if design is String else design["name"]
	var fit: Dictionary = {} if design is String else design
	var m := {"hull": hull_name, "race": race_id, "size": size, "seed": hash(hull_name) + race_id * 31,
		"tex": hull_texture(hull_name, race_id), "rect": Rect2(), "nozzles": [], "overlays": [], "turrets": [],
		"glow": glow_color(race_id), "tint": part_tint(race_id), "station": hull_name == "Starbase"}
	var tex: Texture2D = m["tex"]
	m["rect"] = _hull_rect(tex, size, m["station"])
	m["mask"] = hull_mask(tex)
	# every hardpoint spot is kept clear of system parts, fitted or not
	m["taken"] = hardpoint_spots(hull_name, race_id, size).duplicate()
	var counts := _count_funcs(fit)
	if not m["station"]:
		var n_eng: int = counts.get("eng", 2 if hull_name != "Scout" else 1)
		var eng_tier: int = counts.get("eng_tier", 2)
		m["nozzles"] = _nozzle_points(m, hull_name, size, n_eng, eng_tier)
		_system_overlays(m, size, counts)
	var weapons: Array = fit["weapons"] if not fit.is_empty() else GameData.SHIPS[hull_name]["weapons"]
	var i := 0
	for w in weapons:
		_turret_overlay(m, w, i, size)
		i += 1
	_models[key] = m
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


## Where the engine flames come out: the engines painted on the hull,
## or, for a painting without visible ones, one nozzle per engine module
## spread across the stern and set on the hull's rear edge.
static func _nozzle_points(m: Dictionary, hull_name: String, size: float, n: int, tier: int) -> Array:
	var rect: Rect2 = m["rect"]
	var mask: Dictionary = m["mask"]
	var out := []
	for e in painted_engines(m["tex"]):
		var r := clampf(e["h"] * rect.size.y * 0.45, size * 0.07, size * 0.2) * (0.9 + 0.05 * tier)
		var x: float = rect.position.x + e["x"] * rect.size.x
		# a glow painted inside the hull still gets its flame at the edge
		if not mask.is_empty():
			var gw: int = mask["gw"]
			var gh: int = mask["gh"]
			var row := clampi(int((e["y"] + 0.5) * gh), 0, gh - 1)
			for gx in gw:
				if mask["depth"][row * gw + gx] > 0.0:
					x = minf(x, rect.position.x + gx * rect.size.x / gw)
					break
		out.append({"pos": Vector2(x + r * 0.4, e["y"] * rect.size.y), "r": r})
	if not out.is_empty():
		return out
	n = clampi(n, 1, 4)
	var spread := rect.size.y * (0.18 if hull_name == "Scout" else 0.28)
	var r := size * (0.08 + 0.03 * tier) * (1.0 if n <= 2 else 0.8)
	for i in n:
		var y := 0.0 if n == 1 else lerpf(-spread, spread, float(i) / float(n - 1))
		var x := rect.position.x + r * 0.6
		if not mask.is_empty():
			var gw: int = mask["gw"]
			var gh: int = mask["gh"]
			var row := clampi(int((y - rect.position.y) / rect.size.y * gh), 0, gh - 1)
			# walk in from the stern to the first solid cell of this row,
			# pulling the row toward the keel if it misses the hull
			var found := false
			for tries in gh:
				for gx in gw:
					if mask["depth"][row * gw + gx] >= 1.0:
						x = rect.position.x + (gx + 0.5) * rect.size.x / gw + r * 0.3
						y = rect.position.y + (row + 0.5) * rect.size.y / gh
						found = true
						break
				if found:
					break
				row += 1 if row < gh / 2 else -1
		out.append({"pos": Vector2(x, y), "r": r})
	return out


## Add a part overlay, kept on the hull and clear of other parts unless
## `free` (parts that hang off an edge on purpose, like the ram prow).
static func _overlay(m: Dictionary, icon: String, pos: Vector2, width: float, rot: float = 0.0, alpha: float = 1.0, anchor: float = 0.5, free: bool = false) -> void:
	var t := part_texture(icon)
	if t == null:
		return
	if not free:
		pos = _snap(m["mask"], m["rect"], pos, width * 0.4, m["taken"], width * 0.85)
		m["taken"].append(pos)
	m["overlays"].append({"tex": t, "icon": icon, "pos": pos, "w": width, "h": width * t.get_height() / t.get_width(), "rot": rot, "a": alpha, "anchor": anchor})


## Fitted systems, placed where they read best on a top-down hull.
static func _system_overlays(m: Dictionary, size: float, counts: Dictionary) -> void:
	var rect: Rect2 = m["rect"]
	var hw := rect.size.y * 0.5          # half height of the hull sprite
	var s := size
	# engines are part of each painting; one pair of booster pods sits on
	# the aft flanks
	if counts.get("boost", 0) > 0:
		var bx := rect.position.x + rect.size.x * 0.24
		_overlay(m, "booster", Vector2(bx, -hw * 0.4), s * 0.24, 0.0, 0.95)
		_overlay(m, "booster", Vector2(bx, hw * 0.4), s * 0.24, 0.0, 0.95)
	# quantum drive fins over the stern
	var qt: int = counts.get("qdrv_tier", 0)
	if qt > 0:
		_overlay(m, "qdrv", Vector2(rect.position.x + s * 0.7, 0), s * (0.22 + 0.04 * qt), PI, 0.9)
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
		_overlay(m, "ram", Vector2(rect.end.x + s * 0.05, 0), s * 0.5, 0.0, 1.0, 0.25, true)


static func _turret_overlay(m: Dictionary, w: Array, index: int, size: float) -> void:
	var id: String = w[0]
	var mount := weapon_spot(m["hull"], m["race"], size, w)
	var cls := GameData.weapon_class(id)
	var r := turret_radius(size, cls)
	var icon := weapon_icon(id)
	var t := part_texture(icon)
	if t == null:
		return
	var width := r * (2.6 if icon in ["cannon", "disruptor", "torpedo", "mine"] else 2.0)
	m["turrets"].append({"tex": t, "pos": mount, "w": width, "h": width * t.get_height() / t.get_width(),
		"fire": deg_to_rad(float(w[3])), "index": index, "turret": w[5] - w[4] >= 150, "anchor": 0.42 if icon != "mine" else 0.5})


# ------------------------------------------------------------------ draw

## `broken` maps a part icon, "engines" or "t<weapon index>" to how badly
## that system is damaged (0..1, 1 offline): those parts are drawn dark.
static func draw_model(c: CanvasItem, m: Dictionary, base: Color, thrusting: float, cloak_alpha: float, damage: float,
		xform: Transform2D = Transform2D.IDENTITY, aims: Array = [], flash: float = 0.0, broken: Dictionary = {}) -> void:
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
			var l := r * (2.6 + 4.2 * thrusting) * randf_range(0.85, 1.15)
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
	# fitted systems, washed in the empire's palette
	var tint: Color = m["tint"]
	var pm := Color(tint.r * bright * dark, tint.g * bright * dark, tint.b * bright * dark, a)
	for o in m["overlays"]:
		var k := 1.0 - 0.6 * float(broken.get(o.get("icon", ""), 0.0))
		_draw_icon(c, xform, o["tex"], o["pos"], o["w"], o["h"], o["rot"], Color(pm.r * k, pm.g * k, pm.b * k, a * o["a"]), o["anchor"])
	# weapons, turrets turned to their live aim
	for t in m["turrets"]:
		var ang: float = t["fire"]
		if t["turret"] and t["index"] < aims.size():
			ang = aims[t["index"]]
		var k := 1.0 - 0.6 * float(broken.get("t%d" % t["index"], 0.0))
		_draw_icon(c, xform, t["tex"], t["pos"], t["w"], t["h"], ang, Color(pm.r * k, pm.g * k, pm.b * k, pm.a), t["anchor"])
	c.draw_set_transform_matrix(xform)
	# idle nozzle glow
	if a > 0.05 and not m["station"]:
		var glow: Color = m["glow"]
		var live := 1.0 - float(broken.get("engines", 0.0))
		for nz in m["nozzles"]:
			c.draw_circle(nz["pos"], nz["r"] * 0.6, Color(glow.r, glow.g, glow.b, (0.3 + 0.6 * thrusting) * a * live))
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
