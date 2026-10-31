## ShipArt -- ship models built from the painted sprite sheets.
##
## Every hull is a top-down painting in its empire's design language
## (assets/ships/<empire>/<Hull>.png): Terran plated grey-blue hulls,
## Arcean organic teal shells with green bioluminescence, Drengin crimson
## blades, Pirate patched-up scrap.  Each hull class has its own outline
## and the paintings carry no guns: every hardpoint is an empty socket in
## the empire's style (assets/parts/socket_<empire>.png), and the fitted
## parts are drawn on top from the parts sheet (assets/parts/*.png): a
## turret model per weapon, seated in its socket, that turns with the
## pilot's aim, one nozzle per engine, shield
## domes, the sensor dish, the reactor core, jammer spine, booster pods,
## warp fins and so on, so a loadout can be read from the ship itself.
## Engine flames, hit flashes and battle damage are live effects
## drawn around the model each frame.  Ships point along +X.
##
## An armor coat restyles the whole painting (coat_texture): plate panels,
## mirror chrome, ceramic tiles, reactive bricks, a dark shroud or heavy
## ribs, worked out from each empire's own painting and laid out in its
## style, so every hull, empire and coat looks different.
class_name ShipArt
extends RefCounted

## Hull sprite length in multiples of the design's size (a radius-like
## number used by the rules).
const LENGTH := 3.0

## Where each hardpoint's socket sits on a station painting, in hardpoint
## order, as fractions of the painting (x from the left edge, y from the
## top).  Ship hulls keep theirs, with every other part's spot, in
## ShipPlaces.SPOTS ("w").  Paintings listed in neither get their sockets
## snapped onto the hull from the original game's mount radii.
## Hardpoint order per hull (GameData.SHIPS weapons): Scout bow, bow, aft
## rack; Destroyer bow, starboard (+y), port, aft; Raider bow, starboard,
## port; Cruiser and BattleCruiser bow, bow, starboard, port, aft;
## Battleship bow, heavy bow, starboard, port, aft; Dreadnought bow,
## starboard, port, dorsal, aft; Freighter and Drop ship one bow gun.
const MOUNTS := {
	# Defensive station: main gun on the hub, the two flak guns in opposite
	# gun pits (all three turn the full circle, so where they sit is looks)
	"stations/DefenseStation_terran": [[0.5, 0.45], [0.27, 0.64], [0.744, 0.245]],
	"stations/DefenseStation_arcean": [[0.504, 0.49], [0.297, 0.645], [0.718, 0.319]],
	"stations/DefenseStation_drengin": [[0.5, 0.5], [0.29, 0.655], [0.709, 0.333]],
	# Starbase: the two torpedo tubes, battery and laser by the hub, then the
	# four defence turrets in the painted gun emplacements between the arms
	# (down-right, down-left, up-left, up-right)
	"stations/terran@Starbase": [[0.644, 0.5], [0.356, 0.5], [0.5, 0.64], [0.5, 0.272], [0.775, 0.76], [0.225, 0.76], [0.225, 0.215], [0.775, 0.215]],
	"stations/arcean@Starbase": [[0.644, 0.5], [0.356, 0.5], [0.5, 0.643], [0.5, 0.268], [0.775, 0.73], [0.225, 0.73], [0.225, 0.215], [0.775, 0.215]],
	"stations/drengin@Starbase": [[0.644, 0.5], [0.356, 0.5], [0.5, 0.64], [0.5, 0.272], [0.765, 0.76], [0.235, 0.76], [0.235, 0.245], [0.765, 0.245]],
}

static var _textures := {}
static var _masks := {}
static var _spots := {}
static var _models := {}
static var _engines := {}
static var _sockets_cache := {}
static var _coats := {}            # "<painting path>|<coat id>" -> Texture2D, or the Image a worker baked
static var _coat_jobs := {}        # the same keys -> WorkerThreadPool task, while it bakes
static var _coat_lock := Mutex.new()
## Bake coats on the spot instead of on a worker (screenshot tools).
static var bake_now := false
## Draw the high-resolution paintings (the shipyard), resampled to
## `hd_width` pixels across when that is set (0: as they are).
static var hd := false
static var hd_width := 0
static var _hd_sized := {}


# ---------------------------------------------------------------- public

## Draw a ship.  `design` is a hull name or a design dictionary.  `xform`
## places the model (Ship draws at identity; menus and the shipyard pass a
## position and scale).  `aims` holds a live angle per weapon for turrets
## that track a target; `flash` whitens the hull for a hit.
static func draw_ship(c: CanvasItem, design: Variant, race_id: int, size: float, base: Color, thrusting: float, alpha: float, damage: float,
		xform: Transform2D = Transform2D.IDENTITY, aims: Array = [], flash: float = 0.0) -> void:
	var model := build_model(design, race_id, size)
	draw_model(c, model, base, thrusting, alpha, damage, xform, aims, flash)


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
	if GameData.is_station_hull(hull_name):
		var yard: String = ["arcean", "terran", "drengin", "drengin", "terran"][clampi(race_id, 0, 4)]
		# a built station's own painting when there is one, else the starbase's
		var own := texture("res://assets/ships/stations/%s_%s.png" % [hull_name, yard]) if hull_name != "Starbase" else null
		return own if own != null else texture("res://assets/ships/stations/%s.png" % yard)
	if race_id == 3:
		match hull_name:
			"Destroyer": file = "Raider"
			"Dreadnought": file = "Battleship"
			"Freighter", "Trader", "DropShip": file = "Cruiser"
	else:
		match hull_name:
			"Raider": file = "Destroyer"
	var t := texture("res://assets/ships/%s/%s.png" % [folder, file])
	if t == null and hull_name == "Trader":
		return hull_texture("Freighter", race_id)   # until the trade ship has its own painting
	if t == null and hull_name == "DropShip":
		return hull_texture("Cruiser", race_id)     # until the drop ship has its own painting
	return t


## The high-resolution copy of a hull painting (assets/ships/hd/, the same
## picture upscaled from its source sheet, for the shipyard), or null.
static func hd_source(tex: Texture2D) -> Texture2D:
	if tex == null:
		return null
	return texture(tex.resource_path.replace("res://assets/ships/", "res://assets/ships/hd/").get_basename() + ".webp")


## The high-resolution painting resampled to `hd_width` (so it stays sharp
## without mipmaps); the painting itself when there is no copy.
static func hd_texture(tex: Texture2D) -> Texture2D:
	var big := hd_source(tex)
	return tex if big == null else _sized(big, big.resource_path)


## `t` resampled once to `hd_width` pixels across (cached under `key`).
static func _sized(t: Texture2D, key: String) -> Texture2D:
	if hd_width <= 0 or hd_width >= t.get_width():
		return t
	key += "|%d" % hd_width
	if not _hd_sized.has(key):
		if _hd_sized.size() > 12:
			_hd_sized.clear()
		var img := t.get_image()
		if img.is_compressed():
			img.decompress()
		img.resize(hd_width, maxi(int(round(img.get_height() * float(hd_width) / img.get_width())), 1), Image.INTERPOLATE_LANCZOS)
		_hd_sized[key] = ImageTexture.create_from_image(img)
	return _hd_sized[key]


static func part_texture(name: String) -> Texture2D:
	return texture("res://assets/parts/%s.png" % name)


static func _painting_key(tex: Texture2D) -> String:
	if tex == null:
		return ""
	var path := tex.resource_path
	return path.get_base_dir().get_file() + "/" + path.get_file().get_basename()


## The name of a part's picture for an empire: "<icon>_<empire>" when that
## empire has its own painting (the mass driver does), else the shared one.
static func race_part_icon(icon: String, race_id: int) -> String:
	var own: String = icon + "_" + ["arcean", "terran", "drengin", "pirate"][clampi(race_id, 0, 3)]
	return own if part_texture(own) != null else icon


## The empty weapon socket in an empire's style.  Its feed conduits point
## left; the ring itself is the square at the right end of the picture.
static func socket_texture(race_id: int) -> Texture2D:
	return part_texture("socket_" + ["arcean", "terran", "drengin", "pirate"][clampi(race_id, 0, 3)])


## The parts-sheet icon for a weapon module.
static func weapon_icon(id: String) -> String:
	var mod: Dictionary = GameData.MODULES[id]
	if mod["func"] == "beam":
		return "laser"
	match mod["p"][3]:
		"001": return "massdriver"
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
		"jammer": return "cloak"
		"repair": return "repair"
		"regen": return "regen"
		"ram": return "ram"
		"boost": return "booster"
		"qdrv": return "qdrv"
		"pdc": return "rapid"
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


## The drawn turret and socket radius: no wider than a slim hull can seat.
static func seat_radius(m: Dictionary, cls: int) -> float:
	var rect: Rect2 = m["rect"]
	return minf(turret_radius(m["size"], cls), maxf(rect.size.y * 0.3, 2.5))


## Where each of a hull's hardpoints sits on its painting.  The original
## game's mount radii were made for its own hull shapes, so each mount is
## moved to the nearest spot on the painted hull deep enough for its
## turret, clear of the other turrets, and mirrored mounts stay mirrored.
static func hardpoint_spots(hull_name: String, race_id: int, size: float) -> Array:
	var key := "%s|%d|%.2f" % [hull_name, race_id, size]
	if _spots.has(key):
		return _spots[key]
	var tex := hull_texture(hull_name, race_id)
	var rect := _hull_rect(tex, size, GameData.is_station_hull(hull_name))
	var mask := hull_mask(tex)
	var raws := []
	var out := []
	# hand-placed sockets on the decks each painting leaves free
	var placed: Array = places(tex, hull_name).get("w", MOUNTS.get(_painting_key(tex) + "@" + hull_name, MOUNTS.get(_painting_key(tex), [])))
	var hps := GameData.hull_hardpoints(hull_name)
	if placed.size() == hps.size():
		for p in placed:
			out.append(rect.position + Vector2(p[0] * rect.size.x, p[1] * rect.size.y))
		_spots[key] = out
		return out
	for hp in hps:
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


## The hand-placed part spots for a hull's painting (ShipPlaces), or {}.
static func places(tex: Texture2D, hull_name: String) -> Dictionary:
	var key := _painting_key(tex)
	return ShipPlaces.SPOTS.get(key + "@" + hull_name, ShipPlaces.SPOTS.get(key, {}))


## A painting fraction ([x from the stern edge, y from the top]) as a
## model-space point.
static func _at(rect: Rect2, p: Array) -> Vector2:
	return rect.position + Vector2(float(p[0]) * rect.size.x, float(p[1]) * rect.size.y)


## Hand-placed sockets ("boost" or "ext" in ShipPlaces) as model points:
## a point off the centre line is a mirrored pair, [x, y, 0] a single
## socket off the centre line, four numbers a pair as given.
static func _placed_sockets(rect: Rect2, list: Array) -> Array:
	var out := []
	for p in list:
		if p.size() >= 4:
			out.append(_at(rect, [p[0], p[1]]))
			out.append(_at(rect, [p[2], p[3]]))
		else:
			out.append(_at(rect, p))
			if p.size() == 2 and absf(float(p[1]) - 0.5) >= 0.005:
				out.append(_at(rect, [p[0], 1.0 - float(p[1])]))
	return out


## A mount position as the original game stored it.
static func raw_mount(angle_deg: float, dist: float, size: float) -> Vector2:
	return Vector2.RIGHT.rotated(deg_to_rad(angle_deg)) * (dist + size * 0.5) * (0.75 if dist == 0.0 else 1.0)


## Where each of a design's weapons ([id, angle, radius, ...]) sits on the
## hull: on the first free hardpoint with the same mount, so two guns
## sharing a mount angle (a Cruiser's two bow guns) get a socket each.
static func weapon_spots(hull_name: String, race_id: int, size: float, weapons: Array) -> Array:
	var hps := GameData.hull_hardpoints(hull_name)
	var spots := hardpoint_spots(hull_name, race_id, size)
	var used := {}
	var out := []
	for w in weapons:
		var spot := Vector2.INF
		for i in hps.size():
			if not used.has(i) and float(hps[i][0]) == float(w[1]) and float(hps[i][1]) == float(w[2]):
				used[i] = true
				spot = spots[i]
				break
		if spot == Vector2.INF:
			var tex := hull_texture(hull_name, race_id)
			var r := turret_radius(size, GameData.weapon_class(w[0]))
			spot = _snap(hull_mask(tex), _hull_rect(tex, size, GameData.is_station_hull(hull_name)), raw_mount(float(w[1]), float(w[2]), size), r * 0.8)
		out.append(spot)
	return out


## Sockets for the systems that show on a hull, as [x from the stern,
## spread from the keel to the edge] fractions: "boost" for booster pods
## (mirrored pairs on the aft flanks, beside the engines), "ext" for point
## defence turrets and shield emitters (mirrored flank pairs, bow, stern).
## Listed in the order empty sockets are filled.
const SOCKET_PLAN := {
	"boost": [[0.12, 0.9], [0.24, 0.85]],
	"ext": [[0.5, 0.62], [0.82, 0.0], [0.66, 0.58], [0.34, 0.62], [0.2, 0.0]],
}
## Which socket set each placed system uses.
const SOCKET_KIND := {"pdc": "ext", "shield": "ext", "boost": "boost"}


## A hull's system sockets of one kind (SOCKET_PLAN), on the painting and
## clear of the weapon mounts and of each other.  A mirrored pair stays
## mirrored when the painting allows it.
static func system_sockets(hull_name: String, race_id: int, size: float, kind: String) -> Array:
	var key := "%s|%d|%.2f" % [hull_name, race_id, size]
	if not _sockets_cache.has(key):
		_sockets_cache[key] = _make_sockets(hull_name, race_id, size)
	return _sockets_cache[key][kind]


static func _make_sockets(hull_name: String, race_id: int, size: float) -> Dictionary:
	var tex := hull_texture(hull_name, race_id)
	var rect := _hull_rect(tex, size, GameData.is_station_hull(hull_name))
	var mask := hull_mask(tex)
	var hand := places(tex, hull_name)
	if hand.has("boost") and hand.has("ext"):
		return {"boost": _placed_sockets(rect, hand["boost"]), "ext": _placed_sockets(rect, hand["ext"])}
	var taken: Array = hardpoint_spots(hull_name, race_id, size).duplicate()
	# boosters ride beside the engines, not out on a wing tip
	var engine_reach := rect.size.y * 0.28
	var painted := painted_engines(tex)
	if not painted.is_empty():
		engine_reach = 0.0
		for e in painted:
			engine_reach = maxf(engine_reach, (absf(e["y"]) + e["h"] * 0.5) * rect.size.y)
	var out := {}
	for kind in ["boost", "ext"]:
		var r := size * (0.09 if kind == "ext" else 0.1)
		var list := []
		for plan in SOCKET_PLAN[kind]:
			var x: float = rect.position.x + float(plan[0]) * rect.size.x
			# how far the hull reaches either side of the keel at this station
			var reach := rect.size.y * 0.25
			if not mask.is_empty():
				var gw: int = mask["gw"]
				var gh: int = mask["gh"]
				var gx := clampi(int((x - rect.position.x) / rect.size.x * gw), 0, gw - 1)
				var top := gh
				var bottom := -1
				for gy in gh:
					if mask["depth"][gy * gw + gx] > 0.0:
						top = mini(top, gy)
						bottom = maxi(bottom, gy)
				if bottom >= 0:
					var up := -(rect.position.y + top * rect.size.y / gh)
					var down := rect.position.y + (bottom + 1) * rect.size.y / gh
					reach = maxf(minf(up, down) - r * 0.6, 0.0)
			if kind == "boost":
				reach = minf(reach, engine_reach + r * 1.6)
			var spread: float = plan[1]
			if spread <= 0.0:
				var p := _snap(mask, rect, Vector2(x, 0.0), r * 0.5, taken, r * 1.7)
				taken.append(p)
				list.append(p)
				continue
			var a := _snap(mask, rect, Vector2(x, -reach * spread), r * 0.5, taken, r * 1.7)
			taken.append(a)
			var b := Vector2(a.x, -a.y)
			if absf(a.y) < r or (not mask.is_empty() and (_depth_at(mask, rect, b) < r * 0.5 or not _clear_of(b, taken, r * 1.7))):
				b = _snap(mask, rect, Vector2(x, reach * spread), r * 0.5, taken, r * 1.7)
			taken.append(b)
			list.append(a)
			list.append(b)
		out[kind] = list
	return out


## Which socket of its kind each placed system on a fit sits in:
## func ("pdc", "shield", "boost") -> one socket index per such module in
## fit order (-1 when every socket is full).  A design's own "places"
## (the shipyard's picks, the same shape) are kept when they are valid
## and free; the rest fill the first empty sockets.
static func socket_choice(fit: Dictionary, hull_name: String, race_id: int, size: float) -> Dictionary:
	var counts := {}
	for id in fit.get("modules", []):
		var f: String = GameData.MODULES[id]["func"]
		if SOCKET_KIND.has(f):
			counts[f] = counts.get(f, 0) + 1
	var out := {}
	var used := {"ext": {}, "boost": {}}
	var wanted: Dictionary = fit.get("places", {})
	for f in counts:
		var n_sockets := system_sockets(hull_name, race_id, size, SOCKET_KIND[f]).size()
		var picks := []
		var asked: Array = wanted.get(f, [])
		for k in counts[f]:
			var want := int(asked[k]) if k < asked.size() else -1
			if want >= 0 and want < n_sockets and not used[SOCKET_KIND[f]].has(want):
				used[SOCKET_KIND[f]][want] = true
				picks.append(want)
			else:
				picks.append(-1)
		out[f] = picks
	for f in out:
		var free: Dictionary = used[SOCKET_KIND[f]]
		var n_sockets := system_sockets(hull_name, race_id, size, SOCKET_KIND[f]).size()
		for k in out[f].size():
			if out[f][k] >= 0:
				continue
			for i in n_sockets:
				if not free.has(i):
					free[i] = true
					out[f][k] = i
					break
	return out


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
		"tex": hull_texture(hull_name, race_id), "rect": Rect2(), "nozzles": [], "retro": [], "overlays": [], "turrets": [], "sockets": [], "pdc": [],
		"glow": glow_color(race_id), "tint": part_tint(race_id), "station": GameData.is_station_hull(hull_name)}
	var tex: Texture2D = m["tex"]
	m["coat"] = _coat_of(fit)
	m["rect"] = _hull_rect(tex, size, m["station"])
	m["mask"] = hull_mask(tex)
	# every hardpoint spot is kept clear of system parts, fitted or not
	m["taken"] = hardpoint_spots(hull_name, race_id, size).duplicate()
	var counts := _count_funcs(fit)
	if not m["station"]:
		var n_eng: int = counts.get("eng", 2 if hull_name != "Scout" else 1)
		var eng_tier: int = counts.get("eng_tier", 2)
		m["nozzles"] = _nozzle_points(m, hull_name, size, n_eng, eng_tier)
		m["retro"] = _retro_points(m, size)
		# systems in sockets first, so the rest keep clear of them
		var choice := socket_choice(fit, hull_name, race_id, size)
		_placed_systems(m, size, choice)
		_pdc_mounts(m, size, counts.get("pdc", 0), choice.get("pdc", []))
		# the other parts keep clear of every socket, so any can be used
		m["taken"] += system_sockets(hull_name, race_id, size, "boost") + system_sockets(hull_name, race_id, size, "ext")
		_system_overlays(m, size, counts)
	else:
		_pdc_mounts(m, size, counts.get("pdc", 0))
	if not m["station"]:
		_sockets(m, size)
	var weapons: Array = fit["weapons"] if not fit.is_empty() else GameData.SHIPS[hull_name]["weapons"]
	var spots := weapon_spots(hull_name, race_id, size, weapons)
	for i in weapons.size():
		_turret_overlay(m, weapons[i], i, spots[i])
	_models[key] = m
	return m


static func _coat_of(fit: Dictionary) -> String:
	if fit.is_empty():
		return ""
	for id in fit.get("modules", []):
		if GameData.ARMOR.has(id):
			return id
	return ""


# ------------------------------------------------------------ armor coats

## The painting `tex` wearing armor coat `coat_id`, or null with no coat
## (or while it is still being baked: the bare painting shows until then).
## Baked once per painting and coat on a worker thread.
static func coat_texture(tex: Texture2D, coat_id: String, race_id: int, base: Texture2D = null) -> Texture2D:
	if tex == null or coat_id == "" or not GameData.ARMOR.has(coat_id):
		return null
	var key := "%s|%s" % [tex.resource_path, coat_id]
	_coat_lock.lock()
	var got: Variant = _coats.get(key)
	_coat_lock.unlock()
	if got is Texture2D:
		return got
	if got is Image:
		var t := ImageTexture.create_from_image(got)
		_coat_lock.lock()
		_coats[key] = t
		_coat_lock.unlock()
		_coat_jobs.erase(key)
		return t
	if _coat_jobs.has(key) or (DisplayServer.get_name() == "headless" and not bake_now):
		return null                    # (a headless server draws nothing)
	var img := tex.get_image()
	if img == null:
		return null
	if img.is_compressed():
		img.decompress()
	img.convert(Image.FORMAT_RGBA8)
	# a high-resolution copy is laid out like its game painting `base`
	var mask := hull_mask(tex if base == null else base)
	var px := 1.0 if base == null else img.get_width() / float(base.get_width())
	var coat: String = GameData.ARMOR[coat_id]["coat"]
	var tier := GameData.tier(coat_id)
	if bake_now:
		_coats[key] = ImageTexture.create_from_image(bake_coat(img, coat, tier, race_id, mask, px))
		return _coats[key]
	_coat_jobs[key] = WorkerThreadPool.add_task(_bake_job.bind(key, img, coat, tier, race_id, mask, px), false, "coat " + key)
	return null


## (on a worker: touches nothing but its own arguments)
static func _bake_job(key: String, img: Image, coat: String, tier: int, race_id: int, mask: Dictionary, px: float) -> void:
	var out := bake_coat(img, coat, tier, race_id, mask, px)
	_coat_lock.lock()
	_coats[key] = out
	_coat_lock.unlock()


static func _hash2(x: float, y: float) -> float:
	var h := sin(x * 127.1 + y * 311.7) * 43758.5453
	return h - floorf(h)


## Repaint `img` (RGBA8, a hull painting) in armor coat `coat` (a
## GameData.ARMOR "coat"; `tier` for plate).  `px` is how many pixels of
## `img` make one pixel of the game painting (the shipyard's high-resolution
## copy), so the pattern comes out the same size on the hull.  Each coat has
## its own finish and pattern; each empire lays the pattern out its own
## way: Terran straight and square, Arcean flowing in curves, Drengin
## raked at an angle, pirates patchy, with cells missing and mismatched.
##  Plate: steel panels, smaller with each tier, rivets from Plate II.
##  Reflective: mirror chrome with raking highlights, bright edges.
##  Ablative: pale ceramic tiles, mottled, the rims heat-stained.
##  Reactive: a matte hull with rounded explosive cassettes bolted round
##   the rim.
##  Shroud: near black, with faint radar-scattering facets.
##  Bulkheads: barely shows outside (it is inside work): a duller skin,
##   faint frame lines and a thicker rim.
static func bake_coat(img: Image, coat: String, tier: int, race_id: int, mask: Dictionary, px: float = 1.0) -> Image:
	var w := img.get_width()
	var h := img.get_height()
	var data := img.get_data()
	var tint := part_tint(race_id)
	var glow := glow_color(race_id)
	var s := clampf(h / px / 6.0, 10.0, 34.0)  # pattern cell, in game painting pixels
	var gw: int = mask.get("gw", 0)
	var gh: int = mask.get("gh", 0)
	var depth: PackedFloat32Array = mask.get("depth", PackedFloat32Array())
	var cx := float(gw) / w
	var cy := float(gh) / h
	var rake := 0.5                           # Drengin pattern angle
	var cr := cos(rake)
	var sr := sin(rake)
	for y in h:
		for x in w:
			var i := (y * w + x) * 4
			var al := data[i + 3]
			if al == 0:
				continue
			var r := data[i] / 255.0
			var g := data[i + 1] / 255.0
			var b := data[i + 2] / 255.0
			var lum := 0.3 * r + 0.59 * g + 0.11 * b
			# depth into the hull, in mask cells (0 at the edge)
			var dep := 0.0
			if gw > 0:
				dep = depth[mini(int(y * cy), gh - 1) * gw + mini(int(x * cx), gw - 1)]
			# the pattern's coordinates, laid out in the empire's style
			var u := x / px
			var v := y / px
			match race_id:
				0:
					u += sin(v * 0.06) * s * 0.35
					v += sin(u * 0.045) * s * 0.3
				2:
					var uu := u * cr - v * sr
					v = u * sr + v * cr
					u = uu
			var patchy := race_id == 3 and _hash2(floorf(u / (s * 2.0)), floorf(v / (s * 2.0))) < 0.22
			var nr := r
			var ng := g
			var nb := b
			match coat:
				"plate":
					# steel grey at the painting's own brightness
					var k := 0.18 + 0.1 * tier
					var st := lum * 1.08 + 0.02
					nr = lerpf(r, st * 0.95 * tint.r, k)
					ng = lerpf(g, st * 1.0 * tint.g, k)
					nb = lerpf(b, st * 1.1 * tint.b, k)
					var pw := s * (1.25 - 0.15 * tier)
					var ph := pw * 0.72
					var col := floorf(u / pw)
					var fu := u / pw - col
					var fv := v / ph + 0.5 * fmod(absf(col), 2.0)
					fv -= floorf(fv)
					var du := minf(fu, 1.0 - fu) * pw
					var dv := minf(fv, 1.0 - fv) * ph
					var seam := 0.9 + 0.4 * tier
					var f := 1.0
					if minf(du, dv) < seam:
						f *= 0.72
					elif dv < seam + 1.2 and fv < 0.5:
						f *= 1.12
					if tier >= 2 and du < 3.0 + tier and dv < 3.0 + tier and du > seam and dv > seam:
						f *= 1.35         # rivets at the panel corners
					if patchy:
						f *= 0.85
					nr *= f
					ng *= f
					nb *= f
				"reflective":
					var cv := minf(lum * 1.25, 1.0)
					nr = lerpf(r, cv * (0.85 + 0.15 * glow.r), 0.6)
					ng = lerpf(g, cv * (0.85 + 0.15 * glow.g), 0.6)
					nb = lerpf(b, cv * (0.85 + 0.15 * glow.b), 0.6)
					nr = (nr - 0.5) * 1.2 + 0.47
					ng = (ng - 0.5) * 1.2 + 0.47
					nb = (nb - 0.5) * 1.2 + 0.47
					var t := (u * 0.55 + v) / s
					var streak := pow(0.5 + 0.5 * sin(t * 2.1), 18.0) * 0.4 + pow(0.5 + 0.5 * sin(t * 0.63 + 1.3), 40.0) * 0.3
					if dep > 0.0 and dep < 1.6:
						streak += 0.22
					if patchy:
						streak *= 0.3
					nr += streak
					ng += streak
					nb += streak
				"ablative":
					var lv := 0.55 + 0.6 * lum
					nr = lerpf(r, 0.82 * lv, 0.62)
					ng = lerpf(g, 0.75 * lv, 0.62)
					nb = lerpf(b, 0.64 * lv, 0.62)
					var tw := s * 0.48
					var row := floorf(v / tw)
					var tu := u / tw + 0.5 * fmod(absf(row), 2.0)
					var col := floorf(tu)
					var fu := tu - col
					var fv := v / tw - row
					var f := 0.94 + 0.12 * _hash2(col, row) + 0.04 * _hash2(u, v)
					if minf(minf(fu, 1.0 - fu), minf(fv, 1.0 - fv)) * tw < 0.9:
						f *= 0.62
					if dep > 0.0 and dep < 2.2:
						f *= 0.62 + 0.17 * dep   # heat-stained rims
					if patchy:
						f *= 0.8
					nr *= f
					ng *= f
					nb *= f
				"reactive":
					# a matte, darker hull with explosive cassettes bolted on
					# round the rim: separate rounded pads, hull between them
					var lv := 0.6 + 0.55 * lum
					nr = lerpf(r, 0.5 * lv * tint.r, 0.3)
					ng = lerpf(g, 0.48 * lv * tint.g, 0.3)
					nb = lerpf(b, 0.42 * lv * tint.b, 0.3)
					if dep > 0.3 and dep < 2.0 and not patchy:
						var sp := s * 0.95
						var row := floorf(v / sp)
						var pu := u / sp + 0.5 * fmod(absf(row), 2.0)
						var qx := (pu - floorf(pu) - 0.5) * 2.0       # -1..1 across the pad's cell
						var qy := (v / sp - row - 0.5) * 2.0
						var d := pow(pow(absf(qx), 4.0) + pow(absf(qy), 4.0), 0.25)   # rounded square
						if d < 0.62:
							var lit := clampf(1.0 - 0.45 * (qx + qy), 0.55, 1.4)     # lit from the upper left
							var f := lit * (0.75 if d > 0.52 else 1.0)
							var face := 0.5 * lv
							nr = lerpf(nr, face * 0.92, 0.75) * f
							ng = lerpf(ng, face * 0.9, 0.75) * f
							nb = lerpf(nb, face * 0.78, 0.75) * f
						elif d < 0.72:
							nr *= 0.55                                    # its shadow on the hull
							ng *= 0.55
							nb *= 0.55
				"shroud":
					var dk := 0.09 + 0.24 * lum
					nr = lerpf(r, dk * (0.9 + 0.1 * tint.r), 0.84)
					ng = lerpf(g, dk * (0.92 + 0.1 * tint.g), 0.84)
					nb = lerpf(b, dk * (1.0 + 0.12 * tint.b), 0.84)
					var a1 := (u + v) / (s * 1.4)
					var a2 := (u - v * 0.6) / (s * 1.7)
					var e := minf(absf(a1 - roundf(a1)) * s * 1.4, absf(a2 - roundf(a2)) * s * 1.7)
					if e < 0.8:
						nr += 0.05
						ng += 0.055
						nb += 0.065
					if patchy:
						nr += 0.04
						ng += 0.03
				"bulkheads":
					# inside work, so the hull barely shows it: a heavier,
					# slightly duller skin with faint frame lines and a
					# thickened rim
					var st := lum * 1.05
					nr = lerpf(r, st * 0.85 * tint.r, 0.12)
					ng = lerpf(g, st * 0.87 * tint.g, 0.12)
					nb = lerpf(b, st * 0.9 * tint.b, 0.12)
					var pitch := s * 2.2
					var fu := u / pitch - floorf(u / pitch)
					var f := 0.93 if fu < 0.05 else 1.0
					if dep > 0.0 and dep < 1.0:
						f *= 0.88
					nr *= f
					ng *= f
					nb *= f
			data[i] = clampi(int(nr * 255.0), 0, 255)
			data[i + 1] = clampi(int(ng * 255.0), 0, 255)
			data[i + 2] = clampi(int(nb * 255.0), 0, 255)
	return Image.create_from_data(w, h, false, Image.FORMAT_RGBA8, data)


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


## Where the retro jets fire from when the ship reverses or brakes: a pair
## on the bow's shoulders, either side of the keel, each set on the hull's
## forward edge and angled out so the flame clears the nose.
const RETRO_SPLAY := 28.0

static func _retro_points(m: Dictionary, size: float) -> Array:
	var rect: Rect2 = m["rect"]
	var mask: Dictionary = m["mask"]
	var r := size * 0.09
	var out := []
	for side in [-1.0, 1.0]:
		var y: float = side * rect.size.y * 0.2
		var x := rect.end.x - r
		if not mask.is_empty():
			var gw: int = mask["gw"]
			var gh: int = mask["gh"]
			var row := clampi(int((y - rect.position.y) / rect.size.y * gh), 0, gh - 1)
			# walk in from the bow to the first solid cell of this row,
			# pulling the row toward the keel if it misses the hull
			var found := false
			for tries in gh:
				for gx in range(gw - 1, -1, -1):
					if mask["depth"][row * gw + gx] > 0.0:
						x = rect.position.x + (gx + 0.5) * rect.size.x / gw - r * 0.3
						y = rect.position.y + (row + 0.5) * rect.size.y / gh
						found = true
						break
				if found:
					break
				row += 1 if row < gh / 2 else -1
		out.append({"pos": Vector2(x, y), "r": r, "angle": deg_to_rad(RETRO_SPLAY) * signf(y if absf(y) > 0.01 else side)})
	return out


## Add a part overlay, kept on the hull and clear of other parts unless
## `free` (parts that hang off an edge on purpose, like the ram prow).
static func _overlay(m: Dictionary, icon: String, pos: Vector2, width: float, rot: float = 0.0, alpha: float = 1.0, anchor: float = 0.5, free: bool = false) -> void:
	var t := part_texture(race_part_icon(icon, m["race"]))
	if t == null:
		return
	if not free:
		pos = _snap(m["mask"], m["rect"], pos, width * 0.4, m["taken"], width * 0.85)
		m["taken"].append(pos)
	m["overlays"].append({"tex": t, "icon": icon, "pos": pos, "w": width, "h": width * t.get_height() / t.get_width(), "rot": rot, "a": alpha, "anchor": anchor})


## A system part's width, no wider across the hull than a slim painting
## can seat (`rot` turns the picture: the jammer spine lies along the hull).
static func _fit_width(m: Dictionary, icon: String, width: float, rot: float) -> float:
	var t := part_texture(race_part_icon(icon, m["race"]))
	var across := 1.0
	if absf(rot) < 0.1 and t != null:
		across = t.get_height() / float(t.get_width())
	return minf(width, m["rect"].size.y * 0.42 / across)


## Fitted systems, placed where they read best on a top-down hull.
static func _system_overlays(m: Dictionary, size: float, counts: Dictionary) -> void:
	var rect: Rect2 = m["rect"]
	var hw := rect.size.y * 0.5          # half height of the hull sprite
	var s := size
	var hand := places(m["tex"], m["hull"])
	# a hand-placed spot (ShipPlaces) when the painting has one, else the
	# spot given here, kept on the hull and clear of other parts
	var put := func(icon: String, spot: Vector2, width: float, rot: float = 0.0, alpha: float = 0.95) -> void:
		if hand.has(icon):
			var p := _at(rect, hand[icon])
			m["taken"].append(p)
			_overlay(m, icon, p, _fit_width(m, icon, width, rot), rot, alpha, 0.5, true)
		else:
			_overlay(m, icon, spot, width, rot, alpha)
	# (engines are part of each painting; boosters and shield emitters sit
	# in sockets: _placed_systems)
	# quantum drive over the stern, its drive bell aft (Kyle: it was drawn
	# backwards with the bell forward)
	var qt: int = counts.get("qdrv_tier", 0)
	if qt > 0:
		put.call("qdrv", Vector2(rect.position.x + s * 0.7, 0), s * (0.22 + 0.04 * qt), 0.0, 0.9)
	# reactor core at the heart, brighter window with tier
	var pt: int = counts.get("power_tier", 0)
	if pt > 0:
		put.call("reactor", Vector2(-s * 0.15, 0), s * (0.22 + 0.03 * pt))
	# (armor is a coat over the whole painting: coat_texture)
	# sensor dish forward, bigger by tier
	var st: int = counts.get("sensor_tier", 0)
	if st > 0:
		put.call("sensor", Vector2(s * 0.4, -hw * 0.35), s * (0.11 + 0.03 * st))
	# the jammer's emitter spine down the middle
	if counts.get("jammer", 0) > 0:
		put.call("cloak", Vector2(-s * 0.55, 0), s * 0.18, PI * 0.5, 0.9)
	# repair hatch, regenerator node, ram prow
	if counts.get("repair", 0) > 0:
		put.call("repair", Vector2(-s * 0.45, hw * 0.35), s * 0.2)
	if counts.get("regen", 0) > 0:
		put.call("regen", Vector2(s * 0.3, hw * 0.3), s * 0.2)
	if counts.get("ram", 0) > 0:
		_overlay(m, "ram", Vector2(rect.end.x + s * 0.05, 0), s * 0.5, 0.0, 1.0, 0.25, true)


## Booster pods and shield emitter domes, one per module, each in its
## socket (socket_choice); any past the last socket find room nearby.
static func _placed_systems(m: Dictionary, size: float, choice: Dictionary) -> void:
	var s := size
	for f in ["boost", "shield"]:
		var sockets := system_sockets(m["hull"], m["race"], size, SOCKET_KIND[f])
		var icon := "booster" if f == "boost" else "shield"
		var width := s * (0.24 if f == "boost" else 0.16)
		for k in choice.get(f, []):
			if k >= 0 and k < sockets.size():
				m["taken"].append(sockets[k])
				_overlay(m, icon, sockets[k], width, 0.0, 0.95, 0.5, true)
			else:
				_overlay(m, icon, Vector2(-s * 0.3, 0.0), width, 0.0, 0.95)


## Where each PDC turret sits: in its socket when the hull has sockets
## (`picks`, from socket_choice), else the flanks amidships first, then
## the bow and the stern, then the aft flanks, kept on the hull and clear
## of the other parts.  PointDefence fires from these spots.
static func _pdc_mounts(m: Dictionary, size: float, n: int, picks: Array = []) -> void:
	if n <= 0:
		return
	var hw: float = m["rect"].size.y * 0.5
	var s := size
	var spots := [Vector2(s * 0.05, -hw * 0.55), Vector2(s * 0.05, hw * 0.55), Vector2(s * 0.5, 0),
		Vector2(-s * 0.55, 0), Vector2(-s * 0.3, -hw * 0.5), Vector2(-s * 0.3, hw * 0.5)]
	var sockets := system_sockets(m["hull"], m["race"], size, "ext") if not picks.is_empty() else []
	var pic := race_part_icon("rapid", m["race"])
	var t := part_texture(pic)
	# a small gatling, well under the lightest gun turret (Kyle: PDCs were
	# drawn bigger than the main guns)
	var w := seat_radius(m, 0) * 1.4
	for i in n:
		var p: Vector2
		var k: int = picks[i] if i < picks.size() else -1
		if k >= 0 and k < sockets.size():
			p = sockets[k]
		else:
			p = spots[i % spots.size()] * (1.0 - 0.15 * int(i / spots.size()))
			p = _snap(m["mask"], m["rect"], p, w * 0.3, m["taken"], w * 0.8)
		m["taken"].append(p)
		m["pdc"].append({"tex": t, "pos": p, "w": w, "h": w * (t.get_height() / float(t.get_width()) if t != null else 0.5),
			"rest": p.angle() if p.length() > 0.5 else 0.0, "anchor": TURRET_PIVOT.get(pic, 0.36)})


## The PDC turrets' places on a model (local to the ship).
static func pdc_spots(m: Dictionary) -> Array:
	var out := []
	for p in m.get("pdc", []):
		out.append(p["pos"])
	return out


## An empty socket on every hardpoint, fitted or not, with its feed
## conduits running aft and in toward the keel.
static func _sockets(m: Dictionary, size: float) -> void:
	var t := socket_texture(m["race"])
	if t == null:
		return
	var rect: Rect2 = m["rect"]
	var hps := GameData.hull_hardpoints(m["hull"])
	var spots := hardpoint_spots(m["hull"], m["race"], size)
	var ring := float(t.get_height())
	for i in hps.size():
		var p: Vector2 = spots[i]
		var d := seat_radius(m, hps[i][5]) * 2.2
		var side := clampf(p.y / maxf(rect.size.y * 0.25, 0.001), -1.5, 1.5)
		var feed := Vector2(-1.0, -side)
		if float(hps[i][0]) == 180.0:
			feed = Vector2(1.0, 0.0)        # the rear rack feeds from forward
		var w := d * t.get_width() / ring
		m["sockets"].append({"tex": t, "pos": p, "w": w, "h": d, "rot": (-feed).angle(), "anchor": (w - d * 0.5) / w})


## Long-barrelled turrets: drawn length in turret radii, and where the
## ring's centre (the pivot) sits along the picture (per empire painting
## where they differ).
## Lengths are 1.44 x the picture's width over its height, so every
## turret's ring comes out the size of the mass driver's (cut_turrets.py
## prints both numbers for a new sheet).
const TURRET_LENGTH := {"massdriver": 3.2, "massdriver_arcean": 2.65, "massdriver_drengin": 2.35, "massdriver_pirate": 2.85,
	"laser": 2.11, "rapid": 1.97, "cannon": 2.77, "grenade": 1.92, "missile": 1.86, "drone": 1.49, "disruptor": 2.24, "mine": 2.0,
	"laser_arcean": 1.77, "rapid_arcean": 1.73, "cannon_arcean": 2.14, "grenade_arcean": 1.65, "missile_arcean": 1.52, "drone_arcean": 1.55, "disruptor_arcean": 1.93, "mine_arcean": 1.69,
	"laser_drengin": 1.64, "rapid_drengin": 1.58, "cannon_drengin": 1.91, "grenade_drengin": 1.63, "missile_drengin": 1.47, "drone_drengin": 1.31, "disruptor_drengin": 1.6, "mine_drengin": 1.53,
	"laser_pirate": 1.87, "rapid_pirate": 1.76, "cannon_pirate": 2.32, "grenade_pirate": 1.78, "missile_pirate": 1.65, "drone_pirate": 1.5, "disruptor_pirate": 2.0, "mine_pirate": 1.74}
const TURRET_PIVOT := {"massdriver": 0.223, "massdriver_arcean": 0.366, "massdriver_drengin": 0.318, "massdriver_pirate": 0.305,
	"laser": 0.332, "rapid": 0.36, "cannon": 0.261, "grenade": 0.38, "missile": 0.432, "drone": 0.498, "disruptor": 0.268, "mine": 0.35,
	"laser_arcean": 0.405, "rapid_arcean": 0.43, "cannon_arcean": 0.262, "grenade_arcean": 0.395, "missile_arcean": 0.431, "drone_arcean": 0.506, "disruptor_arcean": 0.293, "mine_arcean": 0.403,
	"laser_drengin": 0.372, "rapid_drengin": 0.408, "cannon_drengin": 0.28, "grenade_drengin": 0.378, "missile_drengin": 0.414, "drone_drengin": 0.495, "disruptor_drengin": 0.253, "mine_drengin": 0.404,
	"laser_pirate": 0.361, "rapid_pirate": 0.368, "cannon_pirate": 0.237, "grenade_pirate": 0.396, "missile_pirate": 0.417, "drone_pirate": 0.534, "disruptor_pirate": 0.25, "mine_pirate": 0.318}


static func _turret_overlay(m: Dictionary, w: Array, index: int, mount: Vector2) -> void:
	var id: String = w[0]
	var cls := GameData.weapon_class(id)
	var r := seat_radius(m, cls)
	var icon := weapon_icon(id)
	var pic := race_part_icon(icon, m["race"])
	var t := part_texture(pic)
	if t == null:
		return
	var width: float = r * TURRET_LENGTH.get(pic, TURRET_LENGTH.get(icon, 2.6 if icon in ["cannon", "disruptor", "torpedo", "mine"] else 2.0))
	m["turrets"].append({"tex": t, "pos": mount, "w": width, "h": width * t.get_height() / t.get_width(),
		"fire": deg_to_rad(float(w[3])), "index": index, "turret": w[5] - w[4] >= 150,
		"anchor": TURRET_PIVOT.get(pic, TURRET_PIVOT.get(icon, 0.42 if icon != "mine" else 0.5))})


# ------------------------------------------------------------------ draw

## `broken` maps a part icon, "engines" or "t<weapon index>" to how badly
## that system is damaged (0..1, 1 offline): those parts are drawn dark.
static func draw_model(c: CanvasItem, m: Dictionary, base: Color, thrusting: float, alpha: float, damage: float,
		xform: Transform2D = Transform2D.IDENTITY, aims: Array = [], flash: float = 0.0, broken: Dictionary = {}, coat_wear: float = 1.0, pdc_aims: Array = [], retro: float = 0.0) -> void:
	var a := alpha
	var size: float = m["size"]
	var rect: Rect2 = m["rect"]
	c.draw_set_transform_matrix(xform)
	# engine flames under everything
	if thrusting > 0.01 and a > 0.05:
		# soft plumes: a wide glow in the empire's colour, a hotter plume
		# inside it and a short white core, each fading out along its length
		var glow: Color = m["glow"]
		var plume := plume_texture()
		var hot := glow.lerp(Color.WHITE, 0.45)
		for nz in m["nozzles"]:
			var p: Vector2 = nz["pos"]
			var r: float = nz["r"]
			var l := r * (3.0 + 5.5 * thrusting) * randf_range(0.9, 1.1)
			var root := p.x + r * 0.7
			c.draw_texture_rect(plume, Rect2(root - l * 1.3, p.y - r * 1.6, l * 1.3, r * 3.2), false, Color(glow.r, glow.g, glow.b, 0.5 * a))
			c.draw_texture_rect(plume, Rect2(root - l * 0.85, p.y - r * 1.0, l * 0.85, r * 2.0), false, Color(hot.r, hot.g, hot.b, 0.85 * a))
			c.draw_texture_rect(plume, Rect2(root - l * 0.42, p.y - r * 0.55, l * 0.42, r * 1.1), false, Color(1, 1, 1, 0.9 * a))
	# reversing or braking: the retro jets on the bow fire forward
	if retro > 0.01 and a > 0.05:
		var glow: Color = m["glow"]
		var plume := plume_texture()
		var hot := glow.lerp(Color.WHITE, 0.45)
		for nz in m.get("retro", []):
			var r: float = nz["r"]
			var l := r * (3.0 + 6.0 * retro) * randf_range(0.9, 1.1)
			# turned to face forward: the plume picture runs out to its left
			c.draw_set_transform_matrix(xform * Transform2D(PI + float(nz["angle"]), nz["pos"]))
			var root := r * 0.3
			c.draw_texture_rect(plume, Rect2(root - l * 1.3, -r * 1.6, l * 1.3, r * 3.2), false, Color(glow.r, glow.g, glow.b, 0.5 * a))
			c.draw_texture_rect(plume, Rect2(root - l * 0.85, -r * 1.0, l * 0.85, r * 2.0), false, Color(hot.r, hot.g, hot.b, 0.85 * a))
			c.draw_texture_rect(plume, Rect2(root - l * 0.42, -r * 0.55, l * 0.42, r * 1.1), false, Color(1, 1, 1, 0.9 * a))
		c.draw_set_transform_matrix(xform)
	# the hull painting
	var bright := 1.0 + flash * 2.0
	var dark := 1.0 - 0.3 * clampf(damage, 0.0, 1.0)
	var mod := Color(bright * dark, bright * dark, bright * dark, a)
	var tex: Texture2D = m["tex"]
	if tex != null:
		# (the shipyard draws the high-resolution painting, and its coat)
		var big := hd_source(tex) if hd else null
		c.draw_texture_rect(tex if big == null else _sized(big, big.resource_path), rect, false, mod)
		# the armor coat over it, thinning as an ablative layer or the
		# reactive tiles are spent
		var coat_tex := coat_texture(tex if big == null else big, m.get("coat", ""), m["race"], tex)
		if coat_tex != null and big != null:
			coat_tex = _sized(coat_tex, "%s|%s" % [big.resource_path, m.get("coat", "")])
		if coat_tex != null and coat_wear > 0.0:
			c.draw_texture_rect(coat_tex, rect, false, Color(mod.r, mod.g, mod.b, a * clampf(coat_wear, 0.0, 1.0)))
	else:
		c.draw_rect(rect, Color(base.r, base.g, base.b, a))
	# the weapon sockets, part of the hull
	for so in m["sockets"]:
		_draw_icon(c, xform, so["tex"], so["pos"], so["w"], so["h"], so["rot"], mod, so["anchor"])
	# fitted systems, washed in the empire's palette
	var tint: Color = m["tint"]
	var pm := Color(tint.r * bright * dark, tint.g * bright * dark, tint.b * bright * dark, a)
	for o in m["overlays"]:
		var k := 1.0 - 0.6 * float(broken.get(o.get("icon", ""), 0.0))
		_draw_icon(c, xform, o["tex"], o["pos"], o["w"], o["h"], o["rot"], Color(pm.r * k, pm.g * k, pm.b * k, a * o["a"]), o["anchor"])
	# weapons, turrets turned to their live aim
	for t in m["turrets"]:
		var ang: float = t["fire"]
		if t["index"] < aims.size():
			ang = aims[t["index"]]
		var k := 1.0 - 0.6 * float(broken.get("t%d" % t["index"], 0.0))
		_draw_icon(c, xform, t["tex"], t["pos"], t["w"], t["h"], ang, Color(pm.r * k, pm.g * k, pm.b * k, pm.a), t["anchor"])
	# point defence turrets: a dark ring under a small gatling at its live aim
	var kp := 1.0 - 0.6 * float(broken.get("pdc", 0.0))
	for i in m["pdc"].size():
		var p: Dictionary = m["pdc"][i]
		# seated in the ring of the empire's own socket picture
		var ring := socket_texture(m["race"])
		if ring != null:
			var rh := float(ring.get_height())
			var d: float = p["w"] * 0.8
			c.draw_set_transform_matrix(xform)
			c.draw_texture_rect_region(ring, Rect2(p["pos"] - Vector2(d, d) * 0.5, Vector2(d, d)), Rect2(ring.get_width() - rh, 0.0, rh, rh), mod)
		if p["tex"] != null:
			var ang: float = pdc_aims[i] if i < pdc_aims.size() else p["rest"]
			_draw_icon(c, xform, p["tex"], p["pos"], p["w"], p["h"], ang, Color(pm.r * kp, pm.g * kp, pm.b * kp, pm.a), p["anchor"])
	c.draw_set_transform_matrix(xform)
	# idle nozzle glow
	if a > 0.05 and not m["station"]:
		var glow: Color = m["glow"]
		var live := 1.0 - float(broken.get("engines", 0.0))
		var soft := Fx.soft_texture()
		for nz in m["nozzles"]:
			var gr: float = nz["r"] * (1.1 + 0.4 * thrusting)
			c.draw_texture_rect(soft, Rect2(nz["pos"] - Vector2(gr, gr), Vector2(gr, gr) * 2.0), false, Color(glow.r, glow.g, glow.b, (0.35 + 0.55 * thrusting) * a * live))
	# damage: scorch marks and sparks of exposed frame
	if damage > 0.25:
		var rng := RandomNumberGenerator.new()
		rng.seed = m["seed"]
		var marks := int(damage * 9.0)
		var scorch := scorch_texture()
		var soft := Fx.soft_texture()
		for i in marks:
			var p := Vector2(rng.randf_range(rect.position.x * 0.8, rect.end.x * 0.8), rng.randf_range(rect.position.y * 0.6, rect.end.y * 0.6))
			# only where there is hull under the mark
			if not m["mask"].is_empty() and _depth_at(m["mask"], rect, p) <= 0.0:
				continue
			var sr := size * rng.randf_range(0.1, 0.22)
			c.draw_set_transform_matrix(xform * Transform2D(rng.randf() * TAU, p))
			c.draw_texture_rect(scorch, Rect2(-sr, -sr * 0.8, sr * 2.0, sr * 1.6), false, Color(1, 1, 1, 0.9 * a))
			if damage > 0.6 and i % 2 == 0:
				# embers glowing in the burnt plating
				var er := sr * 0.35 * randf_range(0.6, 1.0)
				c.draw_texture_rect(soft, Rect2(-er, -er, er * 2.0, er * 2.0), false, Color(1.0, 0.5, 0.18, 0.75 * a * randf_range(0.4, 1.0)))
	c.draw_set_transform_matrix(Transform2D.IDENTITY)


static func _draw_icon(c: CanvasItem, xform: Transform2D, tex: Texture2D, pos: Vector2, w: float, h: float, rot: float, mod: Color, anchor: float) -> void:
	c.draw_set_transform_matrix(xform * Transform2D(rot, pos))
	c.draw_texture_rect(tex, Rect2(-w * anchor, -h * 0.5, w, h), false, mod)


static var _plume: Texture2D = null
static var _scorch: Texture2D = null


## An engine plume: hot at the right edge (the nozzle), widening a little
## and then thinning to nothing at the left, soft across, with no edge
## anywhere, so flames read as light rather than shapes.
static func plume_texture() -> Texture2D:
	if _plume == null:
		var w := 128
		var h := 48
		var img := Image.create(w, h, false, Image.FORMAT_RGBA8)
		for y in h:
			var v := (y + 0.5) / h * 2.0 - 1.0
			for x in w:
				var u := 1.0 - (x + 0.5) / w          # 0 at the nozzle, 1 at the tip
				var width := (0.6 + 0.4 * sin(minf(u * 4.0, 1.0) * PI * 0.5)) * pow(1.0 - u, 0.5)
				var across := exp(-pow(v / maxf(width, 0.02), 2.0) * 3.5)
				var along := pow(1.0 - u, 1.5) * smoothstep(0.0, 0.08, u)
				img.set_pixel(x, y, Color(1, 1, 1, clampf(across * along, 0.0, 1.0)))
		_plume = ImageTexture.create_from_image(img)
	return _plume


## A burn mark on the plating: a blotchy dark stain with a ragged, soft
## edge and a brown rim.
static func scorch_texture() -> Texture2D:
	if _scorch == null:
		var n := 64
		var img := Image.create(n, n, false, Image.FORMAT_RGBA8)
		var noise := FastNoiseLite.new()
		noise.seed = 7
		noise.frequency = 0.09
		for y in n:
			for x in n:
				var d := Vector2(x + 0.5 - n * 0.5, y + 0.5 - n * 0.5).length() / (n * 0.5)
				var ragged := d + noise.get_noise_2d(x, y) * 0.35
				var a := clampf(1.0 - ragged, 0.0, 1.0)
				a = a * a * (3.0 - 2.0 * a)
				var core := clampf(1.0 - ragged * 1.6, 0.0, 1.0)
				var col := Color(0.16, 0.1, 0.07).lerp(Color(0.02, 0.018, 0.016), core)
				img.set_pixel(x, y, Color(col.r, col.g, col.b, a * 0.92))
		_scorch = ImageTexture.create_from_image(img)
	return _scorch
