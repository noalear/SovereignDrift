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
var dynamic_music := true     # the music follows the fight
var mouse_aim := true         # turrets follow the mouse
var screen_shake := 1.0
var last_join_ip := ""       # the address typed last on the Play online screen
var round_minutes := 30       # Conquest round length; 0 plays until the system falls
var flight_assist := true     # the RCS holds the turn rate and stops a tumble
var alt_fire_group := 2       # the weapon group the Alt-fire button shoots

# Persistent career, kept between games like the original profile.
var career := {"score": 0, "kills": 0, "deaths": 0, "rounds": 0, "wins": 0}


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
	dynamic_music = cfg.get_value("audio", "dynamic_music", dynamic_music)
	mouse_aim = cfg.get_value("controls", "mouse_aim", mouse_aim)
	flight_assist = cfg.get_value("controls", "flight_assist", flight_assist)
	alt_fire_group = cfg.get_value("controls", "alt_fire_group", alt_fire_group)
	screen_shake = cfg.get_value("video", "screen_shake", screen_shake)
	fullscreen = cfg.get_value("video", "fullscreen", fullscreen)
	camera_zoom = cfg.get_value("video", "camera_zoom", camera_zoom)
	pilot_name = cfg.get_value("pilot", "name", pilot_name)
	race_id = cfg.get_value("pilot", "race", race_id)
	career = cfg.get_value("pilot", "career", career)
	last_join_ip = cfg.get_value("online", "last_join_ip", last_join_ip)
	round_minutes = cfg.get_value("game", "round_minutes", round_minutes)


func save_settings() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("audio", "music_mode", music_mode)
	cfg.set_value("audio", "music_volume", music_volume)
	cfg.set_value("audio", "sfx_volume", sfx_volume)
	cfg.set_value("audio", "song8", song8)
	cfg.set_value("audio", "dynamic_music", dynamic_music)
	cfg.set_value("controls", "mouse_aim", mouse_aim)
	cfg.set_value("controls", "flight_assist", flight_assist)
	cfg.set_value("controls", "alt_fire_group", alt_fire_group)
	cfg.set_value("video", "screen_shake", screen_shake)
	cfg.set_value("video", "fullscreen", fullscreen)
	cfg.set_value("video", "camera_zoom", camera_zoom)
	cfg.set_value("pilot", "name", pilot_name)
	cfg.set_value("pilot", "race", race_id)
	cfg.set_value("pilot", "career", career)
	cfg.set_value("online", "last_join_ip", last_join_ip)
	cfg.set_value("game", "round_minutes", round_minutes)
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
