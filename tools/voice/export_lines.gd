## Writes every voiced line (TutorialLines, StoryLines, and WingLines in
## each wingman voice) to JSON for tools/voice/generate.py.
##   godot --headless --path . -s tools/voice/export_lines.gd -- <out.json>
extends SceneTree


func _init() -> void:
	var args := OS.get_cmdline_user_args()
	var out := args[0] if not args.is_empty() else "user://voice_lines.json"
	var lines := []
	for row in TutorialLines.LINES:
		lines.append({"set": TutorialLines.SET, "id": row[0], "speaker": row[1], "text": Names.fill(row[2])})
	var speakers := {}
	for k in TutorialLines.SPEAKERS:
		var s: Dictionary = TutorialLines.SPEAKERS[k]
		speakers[k] = {"voice": s["voice"], "speed": s["speed"], "fx": s["fx"]}
	# the story campaign: its own cast, and each line's sound (comms or face to face)
	for row in StoryLines.LINES:
		lines.append({"set": StoryLines.SET, "id": row[0], "speaker": StoryLines.SET + ":" + row[1], "text": Names.fill(row[3]), "fx": StoryLines.fx(row[0])})
	for k in StoryLines.SPEAKERS:
		var s: Dictionary = StoryLines.SPEAKERS[k]
		speakers[StoryLines.SET + ":" + k] = {"voice": s["voice"], "speed": s["speed"], "fx": s["fx"]}
	for v in WingLines.VOICES:
		speakers[v] = WingLines.VOICES[v].duplicate()
		for row in WingLines.LINES:
			lines.append({"set": WingLines.SET, "id": "%s_%s" % [v, row[0]], "speaker": v, "text": Names.fill(row[1])})
	var f := FileAccess.open(out, FileAccess.WRITE)
	f.store_string(JSON.stringify({"speakers": speakers, "lines": lines, "say": Names.SAY}, "  "))
	f.close()
	print("wrote %d lines to %s" % [lines.size(), out])
	quit(0)
