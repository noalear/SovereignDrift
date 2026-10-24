## Keybinds -- the pilot's own keys, mouse buttons and gamepad controls.
##
## Every rebindable action has two keyboard-or-mouse slots and one gamepad
## slot.  The defaults are whatever project.godot binds; only the actions
## the pilot changed are saved, to user://keybinds.cfg, and they are put
## into the InputMap at start-up.
extends Node

const PATH := "user://keybinds.cfg"
const KB_SLOTS := 2

## [action, label, section].  Actions this build does not have are skipped.
const ACTIONS := [
	["thrust", "Thrust", "Flight"],
	["reverse", "Reverse thrust", "Flight"],
	["turn_left", "Turn left", "Flight"],
	["turn_right", "Turn right", "Flight"],
	["strafe_left", "Strafe left", "Flight"],
	["strafe_right", "Strafe right", "Flight"],
	["boost", "Boost", "Flight"],
	["brake", "Brake", "Flight"],
	["flight_assist", "Flight assist on / off", "Flight"],
	["nav_next", "Next nav planet", "Flight"],
	["autopilot", "Autopilot", "Flight"],
	["warp", "Quantum drive", "Flight"],
	["fire_primary", "Fire selected group", "Weapons"],
	["fire_secondary", "Alt-fire", "Weapons"],
	["weapon_group_1", "Select group 1", "Weapons"],
	["weapon_group_2", "Select group 2", "Weapons"],
	["weapon_group_3", "Select group 3", "Weapons"],
	["weapon_group_4", "Select group 4", "Weapons"],
	["weapon_group_next", "Next weapon group", "Weapons"],
	["cloak", "Cloak", "Weapons"],
	["target_next", "Next target", "Weapons"],
	["target_nearest", "Nearest enemy", "Weapons"],
	["land", "Land / lift off", "Ship"],
	["colonists_board", "Take colonists on", "Ship"],
	["colonists_unload", "Put colonists down", "Ship"],
	["board", "Board a disabled target", "Ship"],
	["refit", "Shipyard (at a starbase)", "Ship"],
	["trade", "Trade (online)", "Ship"],
	["tactical_map", "System map", "Interface"],
	["radar_zoom_in", "Radar zoom in", "Interface"],
	["radar_zoom_out", "Radar zoom out", "Interface"],
	["help", "Help", "Interface"],
	["music_next", "Next song", "Interface"],
	["pause_menu", "Menu / back", "Interface"],
	["fullscreen", "Full screen", "Interface"],
]

## Pairs that may share a control because they never apply at the same
## time: colonist transfer works only while landed, where guns are silent.
const SHARED := [
	["weapon_group_1", "colonists_board"],
	["weapon_group_2", "colonists_unload"],
]

const PAD_BUTTONS := {
	0: "A", 1: "B", 2: "X", 3: "Y", 4: "Back", 5: "Guide", 6: "Start",
	7: "Left stick", 8: "Right stick", 9: "LB", 10: "RB",
	11: "D-pad up", 12: "D-pad down", 13: "D-pad left", 14: "D-pad right", 15: "Share",
}
const PAD_AXES := {
	0: ["Left stick left", "Left stick right"], 1: ["Left stick up", "Left stick down"],
	2: ["Right stick left", "Right stick right"], 3: ["Right stick up", "Right stick down"],
	4: ["LT", "LT"], 5: ["RT", "RT"],
}
const MOUSE_BUTTONS := {
	1: "Mouse left", 2: "Mouse right", 3: "Mouse middle", 4: "Wheel up", 5: "Wheel down",
	6: "Wheel left", 7: "Wheel right", 8: "Mouse back", 9: "Mouse forward",
}

## Friendlier names than Godot's for a few keys.
const KEY_NAMES := {
	KEY_QUOTELEFT: "`", KEY_APOSTROPHE: "'", KEY_COMMA: ",", KEY_PERIOD: ".", KEY_SLASH: "/",
	KEY_BACKSLASH: "\\", KEY_SEMICOLON: ";", KEY_BRACKETLEFT: "[", KEY_BRACKETRIGHT: "]",
	KEY_EQUAL: "=", KEY_MINUS: "-",
	KEY_PAGEUP: "Page Up", KEY_PAGEDOWN: "Page Down",
}

var defaults := {}   # action -> {"kb": [d|null, d|null], "pad": d|null}
var custom := {}     # only what the pilot changed


func _ready() -> void:
	for row in ACTIONS:
		if InputMap.has_action(row[0]):
			defaults[row[0]] = _read_map(row[0])
	load_binds()
	apply_all()


func actions() -> Array:
	return ACTIONS.filter(func(r): return InputMap.has_action(r[0]))


func binding(action: String) -> Dictionary:
	return custom.get(action, defaults.get(action, {"kb": [null, null], "pad": null}))


# ------------------------------------------------------------ changing

## Put `ev` in a slot ("kb0", "kb1" or "pad").  Another action using the same
## control loses it, unless the two may share; returns that action's label.
func assign(action: String, slot: String, ev: InputEvent) -> String:
	var d := _ser(ev)
	if d.is_empty():
		return ""
	var moved_from := ""
	for row in actions():
		var other: String = row[0]
		if other == action or _may_share(action, other):
			continue
		var b := _copy(binding(other))
		var changed := false
		for i in KB_SLOTS:
			if b["kb"][i] != null and _same(b["kb"][i], d):
				b["kb"][i] = null
				changed = true
		if b["pad"] != null and _same(b["pad"], d):
			b["pad"] = null
			changed = true
		if changed:
			custom[other] = b
			moved_from = row[1]
	var mine := _copy(binding(action))
	if slot == "pad":
		mine["pad"] = d
	else:
		mine["kb"][int(slot.substr(2))] = d
	custom[action] = mine
	apply_all()
	save_binds()
	return moved_from


func clear(action: String, slot: String) -> void:
	var mine := _copy(binding(action))
	if slot == "pad":
		mine["pad"] = null
	else:
		mine["kb"][int(slot.substr(2))] = null
	custom[action] = mine
	apply_all()
	save_binds()


func reset_all() -> void:
	custom.clear()
	apply_all()
	save_binds()


func apply_all() -> void:
	for action in defaults:
		var b := binding(action)
		InputMap.action_erase_events(action)
		for d in b["kb"]:
			if d != null:
				InputMap.action_add_event(action, _deser(d))
		if b["pad"] != null:
			InputMap.action_add_event(action, _deser(b["pad"]))


func load_binds() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(PATH) != OK:
		return
	for action in cfg.get_section_keys("keys") if cfg.has_section("keys") else []:
		if defaults.has(action):
			var v: Variant = cfg.get_value("keys", action)
			if v is Dictionary and v.has("kb") and v.has("pad"):
				custom[action] = v


func save_binds() -> void:
	var cfg := ConfigFile.new()
	for action in custom:
		cfg.set_value("keys", action, custom[action])
	cfg.save(PATH)


# ------------------------------------------------------------ text

## What a slot shows: "Space", "Mouse left", "RT", or "" when empty.
func slot_text(action: String, slot: String) -> String:
	var b := binding(action)
	var d: Variant = b["pad"] if slot == "pad" else b["kb"][int(slot.substr(2))]
	return describe(d)


## The first control on an action, for hints in the HUD ("Space", "1").
func short_text(action: String) -> String:
	if not defaults.has(action):
		return "?"
	var b := binding(action)
	for d in b["kb"]:
		if d != null:
			return describe(d)
	return describe(b["pad"]) if b["pad"] != null else "unbound"


func describe(d: Variant) -> String:
	if d == null or not d is Dictionary:
		return ""
	if d.has("k"):
		var code: int = d["k"]
		var label: int = int(DisplayServer.keyboard_get_label_from_physical(code)) if DisplayServer.get_name() != "headless" else code
		if code == KEY_KP_ADD or code == KEY_KP_SUBTRACT:
			return "Num +" if code == KEY_KP_ADD else "Num -"
		var s: String = KEY_NAMES.get(label if label != 0 else code, OS.get_keycode_string(label if label != 0 else code))
		return s if not s.is_empty() else "Key %d" % code
	if d.has("m"):
		return MOUSE_BUTTONS.get(int(d["m"]), "Mouse %d" % int(d["m"]))
	if d.has("jb"):
		return PAD_BUTTONS.get(int(d["jb"]), "Pad %d" % int(d["jb"]))
	if d.has("ja"):
		var names: Array = PAD_AXES.get(int(d["ja"]), ["Axis %d-" % int(d["ja"]), "Axis %d+" % int(d["ja"])])
		return names[1] if float(d["v"]) > 0.0 else names[0]
	return ""


## Can this event go in this slot?  Keyboard slots take keys and mouse
## buttons, the gamepad slot takes pad buttons and sticks or triggers.
static func fits(slot: String, ev: InputEvent) -> bool:
	if slot == "pad":
		return ev is InputEventJoypadButton or (ev is InputEventJoypadMotion and absf(ev.axis_value) > 0.6)
	return ev is InputEventKey or ev is InputEventMouseButton


# ------------------------------------------------------------ plumbing

func _may_share(a: String, b: String) -> bool:
	for p in SHARED:
		if (p[0] == a and p[1] == b) or (p[0] == b and p[1] == a):
			return true
	return false


func _read_map(action: String) -> Dictionary:
	var out := {"kb": [], "pad": null}
	for ev in InputMap.action_get_events(action):
		var d := _ser(ev)
		if d.is_empty():
			continue
		if d.has("k") or d.has("m"):
			if out["kb"].size() < KB_SLOTS:
				out["kb"].append(d)
		elif out["pad"] == null:
			out["pad"] = d
	while out["kb"].size() < KB_SLOTS:
		out["kb"].append(null)
	return out


static func _copy(b: Dictionary) -> Dictionary:
	return {"kb": (b["kb"] as Array).duplicate(), "pad": b["pad"]}


static func _same(a: Dictionary, b: Dictionary) -> bool:
	if a.has("ja") and b.has("ja"):
		return int(a["ja"]) == int(b["ja"]) and signf(float(a["v"])) == signf(float(b["v"]))
	return a == b


static func _ser(ev: InputEvent) -> Dictionary:
	if ev is InputEventKey:
		var code: int = ev.physical_keycode if ev.physical_keycode != 0 else ev.keycode
		return {"k": code} if code != 0 else {}
	if ev is InputEventMouseButton:
		return {"m": ev.button_index}
	if ev is InputEventJoypadButton:
		return {"jb": ev.button_index}
	if ev is InputEventJoypadMotion:
		return {"ja": ev.axis, "v": 1.0 if ev.axis_value > 0.0 else -1.0}
	return {}


static func _deser(d: Dictionary) -> InputEvent:
	if d.has("k"):
		var e := InputEventKey.new()
		e.physical_keycode = int(d["k"])
		return e
	if d.has("m"):
		var e := InputEventMouseButton.new()
		e.button_index = int(d["m"])
		return e
	if d.has("jb"):
		var e := InputEventJoypadButton.new()
		e.button_index = int(d["jb"])
		return e
	var e := InputEventJoypadMotion.new()
	e.axis = int(d["ja"])
	e.axis_value = float(d["v"])
	return e
