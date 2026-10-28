# Sensors and stealth test: the ranges from the design doc, checked on
# ships put down in deep space (no worlds in the way, no allies to share
# a data link) unless a check needs them.  Radar, lidar's cone, infrared,
# EM and temporal grades, the jammer, a ping, silent running, line of
# sight and the data link.
#   godot --headless --path . -s tools/sensor_test.gd
extends SceneTree

var frames := 0
var world_frames := 0
var main: Node
var failed := false
var made: Array = []
var gd: Node                 # the GameData autoload (not an identifier in a tool script)
const FAR := Vector2(250000.0, 250000.0)


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	gd = root.get_node("/root/GameData")
	main = load("res://src/Main.tscn").instantiate()
	if not "world" in main:
		# Main.gd failed to compile: usually a stale class cache after new
		# class_name files were pulled.  Fix: godot --headless --path . --import
		print("FAILED: src/Main.gd did not compile. Run `godot --headless --path . --import` and try again.")
		quit(1)
		return
	root.add_child(main)
	await process_frame
	await process_frame
	var b: Array = main.menu.buttons[0]
	for mb in main.menu.buttons:
		if mb[1] == "Practice duel":
			b = mb
	var ev := InputEventMouseButton.new()
	ev.button_index = MOUSE_BUTTON_LEFT
	ev.position = (b[0] as Rect2).get_center()
	ev.global_position = ev.position
	ev.pressed = true
	root.push_input(ev, true)
	var up := ev.duplicate()
	up.pressed = false
	root.push_input(up, true)


func _check(ok: bool, what: String) -> void:
	print(("ok      " if ok else "FAILED  ") + what)
	if not ok:
		failed = true


## A ship of `hull` with only the sensors in `fit` (and the built-in optics).
func _ship(w: World, race: int, hull: String, fit: Array, at: Vector2) -> Ship:
	var s := w.spawn_ship("Test %d" % made.size(), race, hull, "Probe", at, Vector2.ZERO)
	var d: Dictionary = gd.stock_design(hull)
	var mods := []
	for id in d["modules"]:
		if gd.MODULES[id]["func"] != "sensor":
			mods.append(id)
	d["modules"] = mods + fit
	s.refit(d)
	s.shield = s.shield_max
	s.energy = s.energy_max
	made.append(s)
	return s


func _put(s: Ship, at: Vector2) -> void:
	s.global_position = at
	s.body.pos = at
	s.reset_physics_interpolation()


## The grade with fresh caches (all of this runs inside one frame).
func _q(v: Ship, t: Ship) -> int:
	for s in made:
		s.sensors._own.clear()
		s.sensors._seen.clear()
	return v.contact_quality(t)


func _process(_delta: float) -> bool:
	frames += 1
	if main == null or main.world == null:
		return false
	var w: World = main.world
	world_frames += 1
	if world_frames != 20:
		return false
	var east := Vector2.RIGHT
	# --- radar: seen at rated x signature / 100
	var v := _ship(w, 0, "Battleship", ["7003"], FAR)
	var bb := _ship(w, 2, "Battleship", [], FAR + east * 2550.0)
	_check(is_equal_approx(bb.signature("radar"), 100.0), "a Battleship's radar return is 100 (%.1f)" % bb.signature("radar"))
	_check(_q(v, bb) == 3, "Radar III tracks a Battleship at 2550")
	_put(bb, FAR + east * 2700.0)
	_check(_q(v, bb) == 2, "and has only a contact at 2700 (out to 1.6x)")
	_put(bb, FAR + east * 4300.0)
	_check(_q(v, bb) == 0, "and nothing at 4300")
	var sc := _ship(w, 2, "Scout", [], FAR + east * 1250.0)
	_check(_q(v, sc) == 3, "Radar III tracks a Scout (return 50) at 1250")
	_put(sc, FAR + east * 1350.0)
	_check(_q(v, sc) == 2, "but not at 1350")
	_put(sc, FAR + east * 9000.0)
	# --- a Shroud coat (armour thread): return x0.4
	_put(bb, FAR + east * 1000.0)
	bb.stats["armor_sig"] = 0.4
	_check(_q(v, bb) == 3, "a Shrouded Battleship is tracked at 1000")
	_put(bb, FAR + east * 1100.0)
	_check(_q(v, bb) == 2, "but not at 1100 (1040)")
	bb.stats["armor_sig"] = 1.0
	# --- jammed: our ranges x0.4
	v.sensors.jammed_until = v.sensors.now() + 30.0
	_put(bb, FAR + east * 1000.0)
	_check(_q(v, bb) == 3, "jammed, Radar III still tracks a Battleship at 1000")
	_put(bb, FAR + east * 1100.0)
	_check(_q(v, bb) != 3, "but not at 1100 (1040)")
	v.sensors.jammed_until = -1.0
	# --- silent running: our radar is off, optics alone (700)
	_put(bb, FAR + east * 2000.0)
	v.set_silent(true)
	_check(_q(v, bb) == 0, "silent running, the radar is off: nothing at 2000")
	_put(bb, FAR + east * 650.0)
	_check(_q(v, bb) == 3, "optics still track at 650")
	v.set_silent(false)
	v.sensors.emcon_wake = 0.0
	# --- lidar: 3200 in a 15 degree cone off the nose
	var lv := _ship(w, 0, "Battleship", ["7201"], FAR + Vector2(0, 20000.0))
	lv.rotation = 0.0
	_put(bb, lv.global_position + east * 3100.0)
	_check(_q(lv, bb) == 3, "Lidar I tracks a Battleship dead ahead at 3100")
	_put(bb, lv.global_position - east * 3100.0)
	_check(_q(lv, bb) < 3, "but not one astern")
	_put(bb, lv.global_position + east.rotated(0.4) * 3100.0)
	_check(_q(lv, bb) < 3, "or 23 degrees off the nose")
	# --- infrared: heat 100 seen at 1200
	var iv := _ship(w, 0, "Battleship", ["7301"], FAR + Vector2(0, 40000.0))
	_put(bb, iv.global_position + east * 1150.0)
	bb.sensors.heat = 100.0
	_check(_q(iv, bb) == 3 and iv.has_track(bb, "heat"), "Infrared I holds a heat track on heat 100 at 1150")
	_check(not iv.has_track(bb, "radar"), "which is not a radar track")
	_put(bb, iv.global_position + east * 1250.0)
	_check(_q(iv, bb) < 3, "and loses it at 1250")
	# --- EM: a contact or a bearing, never a lock
	var ev := _ship(w, 0, "Battleship", ["7401"], FAR + Vector2(0, 60000.0))
	var loud := _ship(w, 2, "Battleship", ["7003"], ev.global_position + east * 5000.0)
	_check(_q(ev, loud) == 2, "EM receiver I hears Radar III (em %d) as a contact at 5000" % int(loud.signature("em")))
	_put(loud, ev.global_position + east * 9000.0)
	_check(_q(ev, loud) == 1, "a bearing at 9000")
	_put(loud, ev.global_position + east * 12000.0)
	_check(_q(ev, loud) == 0, "nothing at 12000")
	_put(loud, ev.global_position + east * 5000.0)
	loud.set_silent(true)
	_check(_q(ev, loud) == 0, "nothing at 5000 when it runs silent (em %d)" % int(loud.signature("em")))
	loud.set_silent(false)
	# --- temporal: jumps only
	var tv := _ship(w, 0, "Battleship", ["7502"], FAR + Vector2(0, 80000.0))
	_put(loud, tv.global_position + east * 9000.0)
	_check(_q(tv, loud) == 0, "Temporal II hears nothing from a ship not jumping")
	loud.warping = true
	loud.warp_charge = Ship.WARP_CHARGE_TIME + 1.0
	_check(_q(tv, loud) == 2, "a ship in the jump (temporal %d) is a contact at 9000" % int(loud.signature("temporal")))
	loud.warping = false
	loud.warp_charge = 0.0
	loud.add_signature("temporal", Sensors.JUMP_TEMPORAL, Sensors.RIPPLE_TIME)
	_check(_q(tv, loud) == 2, "and so is the ripple where it dropped out")
	# --- line of sight: a world blocks radar, not temporal
	var jup: Planet = w.planet_by_name["Jupiter"]
	var near_side := jup.global_position + Vector2(jup.radius + 200.0, 0)
	var far_side := jup.global_position - Vector2(jup.radius + 200.0, 0)
	_check(near_side.distance_to(far_side) < 2600.0, "Jupiter's far side is within Radar III reach (%d)" % int(near_side.distance_to(far_side)))
	_put(v, near_side)
	_put(bb, far_side)
	_check(_q(v, bb) == 0, "Radar III cannot see through Jupiter")
	_put(tv, near_side + Vector2(0, 30))
	bb.add_signature("temporal", Sensors.JUMP_TEMPORAL, Sensors.RIPPLE_TIME)
	_check(_q(tv, bb) >= 1, "a temporal sensor hears a jump behind it")
	_put(v, FAR)
	_put(tv, FAR + Vector2(0, 80000.0))
	# --- data link: an ally within 4000 shares its track
	var blind := _ship(w, 0, "Scout", [], FAR + Vector2(0, 100000.0))
	var spotter := _ship(w, 0, "Battleship", ["7003"], blind.global_position + east * 3000.0)
	_put(bb, spotter.global_position + east * 2000.0)
	w._update_sensor_links()
	_check(_q(blind, bb) == 3, "a blind Scout gets its ally's track by data link")
	_put(spotter, blind.global_position + east * 5000.0)
	_put(bb, spotter.global_position + east * 2000.0)
	_check(_q(blind, bb) == 0, "but not from an ally 5000 away")
	# --- the jammer: on a locked target, ranges x0.4 after it bites
	var jv := _ship(w, 0, "Scout", ["7003"], FAR + Vector2(0, 120000.0))
	var victim := _ship(w, 2, "Battleship", ["7003"], jv.global_position + east * 1200.0)
	jv.target = victim
	_check(jv.can_jam, "a Scout with a Jammer can jam")
	jv.set_jam(true)
	_check(jv.jamming, "the jammer goes on against a locked target")
	jv.sensors._jam_tick(Sensors.JAM_BITE + 0.1)
	_check(victim.sensors.is_jammed(), "and bites after %.0f s" % Sensors.JAM_BITE)
	_check(_q(victim, jv) < 3, "the jammed Battleship loses its lock on the Scout at 1200")
	jv.sensors.tick(0.01)
	_check(jv.sensors.heat_raw >= Sensors.JAM_HEAT, "a running jammer runs hot (heat heading for %d)" % int(jv.sensors.heat_raw))
	jv.jam_off(3.0)
	_check(not jv.jamming and jv.sensors.jam_out > 0.0, "a disruptor hit knocks the jammer out")
	victim.sensors.jammed_until = -1.0
	var lonely := _ship(w, 0, "Scout", ["9101"], FAR + Vector2(0, 140000.0))
	lonely.target = null
	lonely.set_jam(true)
	_check(not lonely.jamming, "the jammer needs a locked target")
	# --- a ping: tracks what it finds, even through silent running
	var pv := _ship(w, 0, "Battleship", ["7101"], FAR + Vector2(0, 160000.0))
	var quiet := _ship(w, 2, "Scout", [], pv.global_position + east * 1250.0)
	_check(_q(pv, quiet) < 3, "a pulse radar alone has no track before it pings")
	pv.ping()
	_check(_q(pv, quiet) == 3, "a ping tracks a Scout at 1250")
	_check(pv.sensors.ping_ready > 0.0, "and the pulse radar recharges")
	_check(quiet.contact_quality(pv) >= 0 and pv.signature("em") > 100.0, "the ping is loud on EM (%d)" % int(pv.signature("em")))
	# --- shipyard numbers
	var sigs: Dictionary = Sensors.fit_signatures(gd.stock_design("Battleship"), gd.stats_for(gd.stock_design("Battleship")))
	_check(is_equal_approx(sigs["radar"], 100.0) and sigs["burn"] > sigs["heat"], "the shipyard rows: return %d, heat %d / %d, emissions %d" % [sigs["radar"], sigs["heat"], sigs["burn"], sigs["em"]])
	for s in made:
		if is_instance_valid(s):
			w.remove_ship(s)
	print("sensor test %s" % ("FAILED" if failed else "passed"))
	quit(1 if failed else 0)
	return true

