## StorySaves -- the story campaign's save slots: as many as the player
## likes, each with a screenshot, the operation it stands at and when it
## was saved.  In "save" mode (the pause menu) there is a new slot and
## Overwrite on each; in "load" mode (the title screen or the pause menu)
## Load on each.  Delete asks twice either way.
class_name StorySaves
extends Control

signal closed()
signal load_requested(path: String)

var mode := "load"                 # save | load
var world: World = null            # save mode: the war to save
var shot: Image = null             # save mode: the picture for the slot
var list: VBoxContainer
var status: Label
var _armed := ""                   # the slot whose Delete was pressed once


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_STOP
	var bg := ColorRect.new()
	bg.color = Color(0.01, 0.02, 0.05, 0.92)
	bg.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	add_child(bg)
	var outer := VBoxContainer.new()
	outer.set_anchors_preset(Control.PRESET_FULL_RECT)
	outer.anchor_left = 0.5
	outer.anchor_right = 0.5
	outer.offset_left = -440
	outer.offset_right = 440
	outer.offset_top = 40
	outer.offset_bottom = -40
	outer.add_theme_constant_override("separation", 12)
	add_child(outer)
	var title := Label.new()
	title.text = "Save the story" if mode == "save" else "Load a story save"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 30)
	outer.add_child(title)
	status = Label.new()
	status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	status.add_theme_font_size_override("font_size", 15)
	status.add_theme_color_override("font_color", Color(0.75, 0.85, 1.0))
	outer.add_child(status)
	var scroll := ScrollContainer.new()
	scroll.size_flags_vertical = Control.SIZE_EXPAND_FILL
	scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
	outer.add_child(scroll)
	list = VBoxContainer.new()
	list.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	list.add_theme_constant_override("separation", 10)
	scroll.add_child(list)
	var back := _button("Back", func(): closed.emit())
	back.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	back.custom_minimum_size = Vector2(220, 0)
	outer.add_child(back)
	if mode == "save" and world != null and world.story != null and world.story.in_op():
		status.text = "An operation is under way: a save holds the war as it stood at the operation's start."
	_fill()


func _unhandled_input(event: InputEvent) -> void:
	if visible and (event.is_action_pressed("pause_menu") or event.is_action_pressed("ui_cancel")):
		get_viewport().set_input_as_handled()
		closed.emit()


func _button(text: String, on_press: Callable) -> Button:
	var b := Button.new()
	b.text = text
	b.add_theme_font_size_override("font_size", 18)
	b.process_mode = Node.PROCESS_MODE_ALWAYS
	b.pressed.connect(on_press)
	return b


## (Re)build the list of slots.
func _fill() -> void:
	for c in list.get_children():
		c.queue_free()
	var first: Button = null
	if mode == "save":
		var row := _row(_preview(), "New save", _now_line())
		first = _button("Save here", func(): _save(SaveGame.new_slot_path()))
		row.get_child(2).add_child(first)
		list.add_child(row)
	var all := SaveGame.slots()
	for slot in all:
		var info: Dictionary = slot["info"]
		var row := _row(slot["shot"], _op_line(info), _when_line(info))
		var buttons: VBoxContainer = row.get_child(2)
		var path: String = slot["path"]
		var main: Button
		if mode == "save":
			main = _button("Overwrite", func(): _save(path))
		else:
			main = _button("Load", func(): load_requested.emit(path))
		buttons.add_child(main)
		var del := _button("Delete" if _armed != path else "Delete: sure?", func(): _delete(path))
		buttons.add_child(del)
		list.add_child(row)
		if first == null:
			first = main
	if all.is_empty() and mode == "load":
		var none := Label.new()
		none.text = "No saves yet. Save the story from the pause menu in a game."
		none.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		none.add_theme_font_size_override("font_size", 18)
		list.add_child(none)
	if first != null:
		first.call_deferred("grab_focus")


## One slot: [picture, words, buttons].
func _row(pic: Texture2D, head: String, sub: String) -> HBoxContainer:
	var row := HBoxContainer.new()
	row.add_theme_constant_override("separation", 16)
	var tr := TextureRect.new()
	tr.custom_minimum_size = Vector2(SaveGame.SHOT_SIZE) * 0.6
	tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
	tr.stretch_mode = TextureRect.STRETCH_KEEP_ASPECT_CENTERED
	tr.texture = pic
	if pic == null:
		var blank := ColorRect.new()
		blank.color = Color(0.05, 0.08, 0.14)
		blank.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
		tr.add_child(blank)
	row.add_child(tr)
	var words := VBoxContainer.new()
	words.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	words.alignment = BoxContainer.ALIGNMENT_CENTER
	var a := Label.new()
	a.text = head
	a.add_theme_font_size_override("font_size", 20)
	words.add_child(a)
	var b := Label.new()
	b.text = sub
	b.add_theme_font_size_override("font_size", 14)
	b.add_theme_color_override("font_color", Color(0.7, 0.78, 0.9))
	words.add_child(b)
	row.add_child(words)
	var buttons := VBoxContainer.new()
	buttons.alignment = BoxContainer.ALIGNMENT_CENTER
	buttons.custom_minimum_size = Vector2(170, 0)
	row.add_child(buttons)
	return row


func _preview() -> Texture2D:
	if shot == null or shot.is_empty():
		return null
	return ImageTexture.create_from_image(shot)


func _save(path: String) -> void:
	if world == null:
		return
	var ok := SaveGame.save_slot(world, path, shot)
	status.text = "Saved." if ok else "Could not save."
	Sfx.play_ui("click", 0.6)
	_armed = ""
	_fill()


func _delete(path: String) -> void:
	if _armed != path:
		_armed = path
		_fill()
		return
	_armed = ""
	SaveGame.delete_slot(path)
	status.text = "Deleted."
	_fill()


## "Operation 2: Dark Convoy"
static func _op_line(info: Dictionary) -> String:
	var op := int(info.get("op", 0))
	if op >= Story.OPS.size():
		return "Every operation so far done"
	return "Operation %d: %s" % [op + 1, Story.OPS[maxi(op, 0)]["title"]]


## "0:42:10 into the war   ·   saved 2026-10-28 21:14"
static func _when_line(info: Dictionary) -> String:
	var t := int(info.get("elapsed", 0.0))
	var at := Time.get_datetime_string_from_unix_time(int(info.get("saved_at", 0.0)) + _tz(), true)
	return "%d:%02d:%02d into the war   ·   saved %s" % [t / 3600, (t / 60) % 60, t % 60, at.substr(0, 16)]


func _now_line() -> String:
	if world == null or world.story == null:
		return ""
	var info := {"op": world.story.index, "elapsed": world.elapsed, "saved_at": Time.get_unix_time_from_system()}
	return _op_line(info) + "   ·   " + _when_line(info)


## Seconds east of UTC, so the dates read in local time.
static func _tz() -> int:
	return int(Time.get_time_zone_from_system().get("bias", 0)) * 60
