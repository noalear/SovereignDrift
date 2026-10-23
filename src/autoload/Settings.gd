## Settings -- player preferences, saved to user://settings.cfg.
extends Node

const PATH := "user://settings.cfg"

var music_mode := "all"       # all | remixes | new | off
var music_volume := 0.8
var sfx_volume := 0.8
var fullscreen := true
var pilot_name := "Pilot"
var race_id := 1
var song8 := "dubstep"        # dubstep | darkwave
var camera_zoom := 1.0
var arcade_turn := true       # mouse-free keyboard flight

# Persistent career, kept between games like the original profile.
var career := {"score": 0, "kills": 0, "deaths": 0}


func _ready() -> void:
	load_settings()
	apply()


func load_settings() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(PATH) != OK:
		return
	music_mode = cfg.get_value("audio", "music_mode", music_mode)
	music_volume = cfg.get_value("audio", "music_volume", music_volume)
	sfx_volume = cfg.get_value("audio", "sfx_volume", sfx_volume)
	song8 = cfg.get_value("audio", "song8", song8)
	fullscreen = cfg.get_value("video", "fullscreen", fullscreen)
	camera_zoom = cfg.get_value("video", "camera_zoom", camera_zoom)
	pilot_name = cfg.get_value("pilot", "name", pilot_name)
	race_id = cfg.get_value("pilot", "race", race_id)
	career = cfg.get_value("pilot", "career", career)


func save_settings() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("audio", "music_mode", music_mode)
	cfg.set_value("audio", "music_volume", music_volume)
	cfg.set_value("audio", "sfx_volume", sfx_volume)
	cfg.set_value("audio", "song8", song8)
	cfg.set_value("video", "fullscreen", fullscreen)
	cfg.set_value("video", "camera_zoom", camera_zoom)
	cfg.set_value("pilot", "name", pilot_name)
	cfg.set_value("pilot", "race", race_id)
	cfg.set_value("pilot", "career", career)
	cfg.save(PATH)


func apply() -> void:
	var mode := DisplayServer.WINDOW_MODE_FULLSCREEN if fullscreen else DisplayServer.WINDOW_MODE_WINDOWED
	if DisplayServer.get_name() != "headless" and DisplayServer.window_get_mode() != mode:
		DisplayServer.window_set_mode(mode)
	var master := AudioServer.get_bus_index("Master")
	AudioServer.set_bus_volume_db(master, 0.0)


func toggle_fullscreen() -> void:
	fullscreen = not fullscreen
	apply()
	save_settings()
