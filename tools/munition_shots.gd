# Screenshots of every munition: a gallery of each shot in flight, one
# lane per kind (zoomed in and at play zoom), the mass driver slug of each
# empire, muzzle flashes, and a volley of everything landing on a ship.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/munition_shots.gd
# SF_SHOTS=<dir> picks the folder (default /tmp/claude-0/sf/shots).
extends SceneTree

const LANES := [
	["001", "Mass driver slug"], ["010", "Rapid bolt"], ["018", "Cannon shell"],
	["022", "PDC round"], ["020", "Disruptor bolt"], ["003", "Plasma (stations)"],
	["008", "Missile"], ["009", "Seeker"], ["004", "Torpedo"], ["021", "Drone"], ["002", "Mine"],
]
const LANE_GAP := 25.0
const GALLERY_T := 0.55

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shots"
var stubs := {}
var lanes: Array = []        # [{m, start, v, t, x, y}]
var headers: Array = []      # column titles [{l, at}]
var pending := ""            # gallery shot to take once the lanes reach the middle
var pending_at := 0.0        # volley shots: seconds after the volley starts
var imp_t := -1.0            # seconds into the one-at-a-time impact pictures
var imp_queue := []          # [missile id, fire at]
var imp_snaps := []          # [picture name, at]
var volley_t := -1.0         # seconds since the volleys started (physics time)
var volley_shots := [["munitions_muzzle", 0.04], ["munitions_volley", 0.2], ["munitions_impacts", 0.42], ["munitions_impacts_late", 0.75]]
var labels: CanvasLayer
var origin := Vector2.ZERO
var target: Ship = null
var race := 1


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	if name.begins_with("impact_") and target != null:
		# just the struck flank
		var at: Vector2 = main.world.get_viewport().get_canvas_transform() * (target.global_position - Vector2(target.size * 0.5, 0))
		img = img.get_region(Rect2i(Vector2i(at) - Vector2i(320, 210), Vector2i(640, 420)))
	img.save_png(out_dir + "/" + name + ".png")
	print("saved " + name)


func _owner(r: int) -> Node:
	if not stubs.has(r):
		var n := Node.new()
		n.set_script(load("res://src/net/ShotOwner.gd"))
		n.owner_race = r
		root.add_child(n)
		stubs[r] = n
	return stubs[r]


func _clear() -> void:
	var w: World = main.world
	for m in w.missiles.duplicate():
		if is_instance_valid(m):
			m._free()
	w.missiles.clear()
	lanes.clear()
	headers.clear()
	for c in labels.get_children():
		c.queue_free()


## One lane per munition, each flying right at its own launch speed, all
## timed to cross the middle of the screen together at the shot.
func _gallery(ids: Array, races: Array, names: Array, cols: int = 1, col_gap: float = 160.0) -> void:
	var w: World = main.world
	_clear()
	var n := ids.size()
	var rows := ceili(float(n) / cols)
	for i in n:
		var col := i % cols
		var row := i / cols
		var md: Dictionary = _gd().MISSILES[ids[i]]
		var speed: float = maxf(md["fire_v"] * _gd().LIGHT_SPEED, 40.0)
		if ids[i] == "002":
			speed = 0.0
		var y := (row - (rows - 1) * 0.5) * LANE_GAP
		var start := Vector2(60.0 + (col - (cols - 1) * 0.5) * col_gap - speed * GALLERY_T, y)
		w.spawn_missile(_owner(races[i]), ids[i], origin + start, w.player.velocity + Vector2.RIGHT * speed, 0.0, null)
		var m: Missile = w.missiles[-1]
		if ids[i] == "002":
			m.armed = 0.0
		lanes.append({"m": m, "start": start, "v": speed, "t": 0.0, "y": y, "x": 60.0 + (col - (cols - 1) * 0.5) * col_gap - 150.0})
		var l := Label.new()
		l.text = names[i] if col == 0 else ""
		l.add_theme_font_size_override("font_size", 22)
		l.add_theme_color_override("font_outline_color", Color.BLACK)
		l.add_theme_constant_override("outline_size", 6)
		labels.add_child(l)
	headers.clear()
	if cols > 1:
		for c in cols:
			var h := Label.new()
			h.text = ["Arcean", "Terran", "Drengin", "Pirate"][c]
			h.add_theme_font_size_override("font_size", 22)
			h.add_theme_color_override("font_outline_color", Color.BLACK)
			h.add_theme_constant_override("outline_size", 6)
			labels.add_child(h)
			headers.append({"l": h, "at": Vector2(60.0 + (c - (cols - 1) * 0.5) * col_gap, -(rows * 0.5 + 0.6) * LANE_GAP)})


## Pin each lane's shot to its straight run (no gravity, no seeking), so
## the gallery is tidy.
func _physics_process(delta: float) -> bool:
	if main == null or main.world == null or main.world.player == null:
		return false
	var w: World = main.world
	if volley_t >= 0.0 and imp_t < 0.0 and target != null:
		var before := volley_t
		volley_t += delta
		if int(before / 1.0) != int(volley_t / 1.0) or before == 0.0:
			_volley()
	if imp_t >= 0.0 and target != null:
		imp_t += delta
		while not imp_queue.is_empty() and imp_t >= imp_queue[0][1]:
			_one_shot(imp_queue.pop_front()[0])
	for ln in lanes:
		if not is_instance_valid(ln["m"]):
			continue
		var m: Missile = ln["m"]
		if m.exploded:
			continue
		ln["t"] += delta
		m.age = minf(m.age, m.life - 1.0)
		var v: Vector2 = w.player.velocity + Vector2.RIGHT * ln["v"]
		var at: Vector2 = origin + ln["start"] + Vector2.RIGHT * ln["v"] * ln["t"]
		m.velocity = v
		m.body.vel = v
		m.global_position = at
		m.body.pos = at
		m.target = null
	return false


func _hold_lanes() -> void:
	var w: World = main.world
	var xf := w.get_viewport().get_canvas_transform()
	var k := 0
	for ln in lanes:
		var lab: Label = labels.get_child(k)
		lab.position = xf * (origin + Vector2(ln["x"], ln["y"])) - Vector2(lab.size.x, lab.size.y * 0.5)
		k += 1
	for h in headers:
		var lab: Label = h["l"]
		lab.position = xf * (origin + h["at"]) - lab.size * 0.5


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_free_flight()
	if frames <= 30 or main.world == null or main.world.player == null:
		return false
	if pending != "" and volley_t < 0.0 and not lanes.is_empty() and lanes[0]["t"] >= GALLERY_T:
		_shot(pending)
		pending = ""
	var w: World = main.world
	var p: Ship = w.player
	p.invulnerable = 1e9
	if frames == 40:
		labels = CanvasLayer.new()
		labels.layer = 50
		root.add_child(labels)
		p.refit("Destroyer")
		var at: Vector2 = p.global_position + Vector2(9000, 0)
		p.global_position = at
		p.body.pos = at
		p.rotation = 0.0
		p.body.angle = 0.0
		w.target_zoom = 2.6
	if frames > 40:
		for k in p.inputs.keys():
			p.inputs[k] = false if typeof(p.inputs[k]) == TYPE_BOOL else 0.0
		p.body.ang_vel = 0.0
		p.energy = 1e6
		p.velocity = Vector2.ZERO
		p.body.vel = Vector2.ZERO
		origin = p.global_position + (Vector2(0, 25) if frames < 290 else Vector2.ZERO)
		p.visible = frames >= 450
		_hold_lanes()
	# 1. every munition, close up
	if frames == 120:
		var ids := []
		var names := []
		var rs := []
		for l in LANES:
			ids.append(l[0])
			names.append(l[1])
			rs.append(race)
		_gallery(ids, rs, names)
	if frames == 121:
		pending = "munitions_close"

	# 2. the same at play zoom
	if frames == 160:
		w.target_zoom = 1.0
	if frames == 250:
		var ids := []
		var names := []
		var rs := []
		for l in LANES:
			ids.append(l[0])
			names.append(l[1])
			rs.append(race)
		_gallery(ids, rs, names)
	if frames == 251:
		pending = "munitions_play_zoom"
	# 3. each empire's slug and rapid bolt
	if frames == 290:
		w.target_zoom = 1.9
	if frames == 380:
		var ids := []
		var names := []
		var rs := []
		var kinds := ["001", "010", "020", "008", "009", "004", "018", "021", "002"]
		for id in kinds:
			for r in 4:
				ids.append(id)
				rs.append(r)
				var nm: String = ""
				for l in LANES:
					if l[0] == id:
						nm = l[1]
				names.append(nm if r == 0 else ["Arcean", "Terran", "Drengin", "Pirate"][r])
		_gallery(ids, rs, names, 4, 170.0)
	if frames == 381:
		pending = "munitions_empires"
	# 4. muzzle flashes: the destroyer fires everything it carries
	if frames == 450:
		_clear()
		w.target_zoom = 2.2
		p.design = _gd().stock_design("Destroyer", p.race_id)
		target = w.spawn_ship("Drone", 2, "Cruiser", "Target", p.global_position + Vector2(240, 0), p.velocity, -1)
		target.invulnerable = 1e9
	if frames > 450 and target != null:
		target.velocity = p.velocity
		target.body.vel = p.velocity
		root.warp_mouse(w.get_viewport().get_canvas_transform() * target.global_position)
	if frames > 460 and volley_t < 0.0:
		volley_t = 0.0
		# slow motion, so each picture lands on its moment however slowly
		# the frames render
		Engine.time_scale = 0.12
	if pending != "" and volley_t >= 0.0 and volley_t >= float(pending_at):
		_shot(pending)
		pending = ""
	if volley_t >= 0.0 and pending == "" and not volley_shots.is_empty():
		var next: Array = volley_shots.pop_front()
		pending = next[0]
		pending_at = next[1]
	if volley_t > 3.0 and imp_t < 0.0:
		imp_t = 0.0
		for m in w.missiles.duplicate():
			if is_instance_valid(m):
				m._free()
		w.missiles.clear()
		_plan_impacts()
	if imp_t >= 0.0 and pending == "" and imp_snaps.size() > 0 and imp_t >= imp_snaps[0][1]:
		var sn: Array = imp_snaps.pop_front()
		_shot(sn[0])
	if imp_t >= 0.0 and imp_snaps.is_empty():
		quit(0)
	return false


## One shot of each kind into the target's flank, a picture just after
## each lands.
const IMPACTS := [["001", "slug"], ["018", "shell"], ["022", "pdc"], ["020", "disruptor"], ["008", "missile"], ["004", "torpedo"], ["002", "mine"]]
const IMPACT_GAP := 0.9
const IMPACT_FROM := 70.0

func _plan_impacts() -> void:
	var t := 0.1
	for e in IMPACTS:
		var md: Dictionary = _gd().MISSILES[e[0]]
		var speed: float = maxf(md["fire_v"] * _gd().LIGHT_SPEED, 1.0)
		var land := IMPACT_FROM / speed if e[0] != "002" else 0.35
		imp_queue.append([e[0], t])
		imp_snaps.append(["impact_" + e[1], t + land + 0.06])
		t += IMPACT_GAP


func _one_shot(id: String) -> void:
	var w: World = main.world
	var p: Ship = w.player
	var md: Dictionary = _gd().MISSILES[id]
	var aim: Vector2 = target.global_position + Vector2(0, -10)
	var from := aim + Vector2(-target.size * 0.5 - IMPACT_FROM, 0)
	var speed: float = md["fire_v"] * _gd().LIGHT_SPEED
	if id == "002":
		from = aim + Vector2(-target.size * 0.5 - 30.0, 0)
		speed = 0.0
	var n := 6 if id == "022" else 1
	for k in n:
		w.spawn_missile(p, id, from + Vector2(-k * 14.0, randf_range(-3, 3)), p.velocity + Vector2.RIGHT * speed, 0.0, target)
		if id == "002":
			w.missiles[-1].armed = 0.0


## Shots of every kind from the destroyer's nose at the target.
func _volley() -> void:
	var w: World = main.world
	var p: Ship = w.player
	var to := (target.global_position - p.global_position).normalized()
	var ids := ["001", "010", "018", "020", "008", "009", "004", "021", "022"]
	for i in ids.size():
		var md: Dictionary = _gd().MISSILES[ids[i]]
		var side := to.orthogonal() * (i - ids.size() * 0.5) * 12.0
		var spd: float = md["fire_v"] * _gd().LIGHT_SPEED
		w.spawn_missile(p, ids[i], p.global_position + to * p.size * 0.6 + side, p.velocity + to * spd, to.angle(), target)


func _gd() -> Node:
	return root.get_node("GameData")
