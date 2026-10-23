# paths.gd -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
# Where the original program and its data live (autoload "Paths").
extends Node


func game_dir() -> String:
	var env := OS.get_environment("SF_GAME_DIR")
	if env != "":
		return env
	return OS.get_executable_path().get_base_dir().path_join("game")


func game_exe(base: String) -> String:
	var ext := ".exe" if OS.get_name() == "Windows" else ""
	return game_dir().path_join(base + ext)
