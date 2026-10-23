# godot_licenses.gd -- NEW FILE (2026-10-12), Stellar Frontier Godot port.
# Writes the Godot Engine license and the notices of the third-party code
# built into it, for inclusion with the release:
#   godot --headless --script tools/godot_licenses.gd -- OUTPUT_FILE
extends SceneTree


func _init() -> void:
	var out: String = OS.get_cmdline_user_args()[0]
	var f := FileAccess.open(out, FileAccess.WRITE)
	f.store_string("THIRD-PARTY NOTICES\n===================\n\n")
	f.store_string("Godot Engine " + Engine.get_version_info()["string"] + "\n")
	f.store_string(Engine.get_license_text() + "\n\n")
	f.store_string("Components built into Godot Engine\n----------------------------------\n\n")
	for info in Engine.get_copyright_info():
		f.store_string("* " + str(info["name"]) + "\n")
		for part in info["parts"]:
			for c in part["copyright"]:
				f.store_string("    (c) " + str(c) + "\n")
			f.store_string("    License: " + str(part["license"]) + "\n")
		f.store_string("\n")
	f.store_string("\nLicense texts\n-------------\n\n")
	var lic: Dictionary = Engine.get_license_info()
	for k in lic.keys():
		f.store_string("=== " + str(k) + " ===\n" + str(lic[k]) + "\n\n")
	f.close()
	quit()
