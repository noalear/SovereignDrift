## Writes every voiced line (TutorialLines) to JSON for tools/voice/generate.py.
##   godot --headless --path . -s tools/voice/export_lines.gd -- <out.json>
extends SceneTree


func _init() -> void:
	var args := OS.get_cmdline_user_args()
	var out := args[0] if not args.is_empty() else "user://voice_lines.json"
	var lines := []
	for row in TutorialLines.LINES:
		lines.append({"set": TutorialLines.SET, "id": row[0], "speaker": row[1], "text": row[2]})
	var speakers := {}
	for k in TutorialLines.SPEAKERS:
		var s: Dictionary = TutorialLines.SPEAKERS[k]
		speakers[k] = {"voice": s["voice"], "speed": s["speed"], "fx": s["fx"]}
	var f := FileAccess.open(out, FileAccess.WRITE)
	f.store_string(JSON.stringify({"speakers": speakers, "lines": lines}, "  "))
	f.close()
	print("wrote %d lines to %s" % [lines.size(), out])
	quit(0)
