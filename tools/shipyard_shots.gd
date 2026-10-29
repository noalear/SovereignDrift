# Shipyard pictures for a visual check: the yard as it opens, a Cruiser on
# the slip with a mount picked, and a bay picked.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/shipyard_shots.gd
# SF_SHOTS=<dir> picks the folder (default /tmp/claude-0/sf/shipyard).
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/shipyard"


func _init() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	call_deferred("_start")


func _start() -> void:
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _shot(name: String) -> void:
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out_dir + "/" + name + ".png")
	print("saved ", name)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 30:
		main._start_free_flight()
	var w = main.world
	if w == null or w.player == null:
		return false
	var p: Ship = w.player
	if frames == 60:
		var base: Ship = w.starbase_of(p.race_id)
		p.global_position = base.global_position + Vector2(base.size + 80, 0)
		p.velocity = base.velocity
		p.credits = 3200
		p.hold = ["5502", "4003", "9202"]
		p.score = 1200
		p.kills = 160
		p.rank = 7
		w.try_refit(p)
	if main.builder == null:
		return frames > 400
	var b = main.builder
	if frames == 80:
		_shot("yard_open")
		b._switch_hull("Cruiser")
		b._select("hp", 2)
	if frames == 95:
		_shot("yard_cruiser_mount")
		b._select("bay", 1)
	if frames == 110:
		_shot("yard_cruiser_bay")
		b._select("hold", 0)
	if frames == 125:
		_shot("yard_hold")
		# Add a part: the part types as buttons
		Builder.pad_mode = true
		b.sel_kind = ""
		b.sel_index = -1
		b.focus_key = "bay:%d" % b.modules.size()
		b._command("accept")      # shows the focus
		b._command("accept")      # picks the slot; the list shows the types
	if frames == 140:
		_shot("yard_types")
		b.focus_key = "type:Shields"
		b._command("accept")      # opens Shields: families with their marks
		b._command("down")
		b._command("down")
	if frames == 155:
		_shot("yard_shields")
		# a weapon mount, switching out: weapon types, then Guns
		Builder.pad_mode = false
		b.focus_key = "hp:2"
		b._command("accept")
		b.focus_key = "type:Guns"
		b._command("accept")
		b._command("down")
	if frames == 170:
		_shot("yard_guns")
		# buy a few things and press Accept: the bill
		b._command("accept")
		b._select("bay", b.modules.size())
		b._mount("9203")
		b._sell_hold(0)
		b._command("launch")
	if frames == 185:
		_shot("yard_bill")
		b._answer(1)
		# a full hold: taking a part off asks first
		while b.hold.size() < 16:
			b.hold.append("9202")
		b.focus_key = "bay:0"
		b._command("remove")
	if frames == 200:
		_shot("yard_hold_full")
		b._answer(1)
		b.hold = ["9202", "4022"]
		b.hold_stock = {}
		b._switch_hull("Destroyer")
		# the booster picked: its sockets on the hull
		for i in b.modules.size():
			if b.modules[i]["id"] == "9201":
				b._select("bay", i)
	if frames == 215:
		_shot("yard_booster_sockets")
		# equip a second booster from the hold: which bay?
		b.focus_key = "hold:0"
		b._equip_hold(0)
	if frames == 230:
		_shot("yard_equip_where")
		b._place_at(b.placing["targets"][-1])     # a new bay
		b._select("bay", b.modules.size() - 1)
		b._command("place")
	if frames == 245:
		_shot("yard_move_socket")
		b._command("accept")
		# swap the bow gun with a flank mount, then turn a flank turret
		b._select("hp", 0)
		b.focus_key = "hp:0"
		b._command("place")
	if frames == 260:
		_shot("yard_move_weapon")
		b._command("accept")
		b._select("hp", 2)
		b.focus_key = "hp:2"
		for n in 3:
			b._command("turn_l")
	if frames == 275:
		_shot("yard_turned")
		# a booster picked: its other marks, lower ones too, with what each adds
		for i in b.modules.size():
			if GameData.MODULES[b.modules[i]["id"]]["func"] == "boost":
				b._select("bay", i)
				b.focus_key = "bay:%d" % i
				break
		var rows: Array = b._catalogue_rows()
		if not rows.is_empty():
			b.hover = -1
			b.kb_nav = true
			b.focus_key = "part:" + rows[-1]["id"]
	if frames == 290:
		_shot("yard_booster_marks")
		for i in b.modules.size():
			if GameData.MODULES[b.modules[i]["id"]]["func"] == "qdrv":
				b._select("bay", i)
				b.focus_key = "bay:%d" % i
				break
	if frames == 305:
		_shot("yard_qdrv_marks")
		# a disruptor on the rear rack: it fires all round, and turns
		b.mounts[b.hardpoints.size() - 1] = {"id": "4020"}
		b._select("hp", b.hardpoints.size() - 1)
		for n in 6:
			b._command("turn_r")
	if frames == 320:
		_shot("yard_disruptor")
		return true
	return false
