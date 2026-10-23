# Records the in-game mix for a listening/measurement check: an audition of
# every effect through the real buses, then a scripted dogfight.
#   xvfb-run -a godot --path . --write-movie /tmp/mix.avi --fixed-fps 30 -s tools/sfx_record.gd [-- --no-music]
extends SceneTree

var frames := 0
var main: Node
var gallery: Array = []


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 20:
		main._start_game()
		if "--no-music" in OS.get_cmdline_user_args():
			root.get_node("Music").set_volume(0.0)
	if main.world == null or main.world.player == null:
		return false
	var w = main.world
	var p = w.player
	var sfx = root.get_node("Sfx")
	# 1) gallery: one of each, 1.2 s apart, from just off the player's nose
	if frames == 40:
		gallery = sfx.banks.keys().filter(func(n): return not n.rstrip("0123456789") in sfx.LOOPS)
		print("gallery order: ", gallery)
	var gi := (frames - 40) / 36
	if frames >= 40 and (frames - 40) % 36 == 0 and gi < gallery.size():
		var n: String = gallery[gi]
		if n in sfx.FLAT:
			sfx.play_ui(n)
		else:
			sfx.play(n, p.global_position + Vector2(200, 0))
	var t0 := 40 + 36 * maxi(gallery.size(), 1)
	# 2) flight and combat
	if frames > t0 and frames > 60:
		var f := frames - t0
		p.inputs["thrust"] = 1.0 if f < 300 else 0.3
		p.inputs["boost"] = f > 90 and f < 160
		p.inputs["turn"] = sin(f * 0.03) * 0.5
		p.inputs["fire_primary"] = f > 180 and f % 20 < 8
		p.inputs["fire_beam"] = f > 260 and f % 90 < 40
		p.inputs["fire_missile"] = f % 120 == 0
		if f == 170:
			for i in 3:
				var e = w.nearest_enemy_ship(p.global_position, p.race_id, 1e9)
				if e != null:
					e.global_position = p.global_position + Vector2(400 + 150 * i, 80 * i)
					e.velocity = p.velocity
					e.inputs["fire_primary"] = true
					p.target = e
		if f == 420 and p.target != null and is_instance_valid(p.target):
			p.target.take_damage(1e6, p)
		if f == 520:
			p.start_warp()
		if f == 640:
			p.end_warp()
		if f == 760:
			quit()
	return false
