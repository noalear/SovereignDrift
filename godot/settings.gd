# settings.gd -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
# Player settings (autoload "Settings"), saved in user://settings.cfg.
extends Node

const PATH := "user://settings.cfg"

# Desktop sizes the original program can run at.  Heights below 768 cut off
# the 1024x768 menu art, so none are offered.
const RESOLUTIONS := [
	"1024x768", "1280x960", "1600x1200",                  # 4:3
	"1280x800", "1440x900", "1680x1050", "1920x1200",     # 16:10
	"1366x768", "1600x900", "1920x1080", "2560x1440",     # 16:9
	"2560x1080", "3440x1440",                              # 21:9
]

var resolution := "auto"        # "auto" or one of RESOLUTIONS
var display := "fullscreen"     # "fullscreen" or "window"
var music := "mix"              # "mix", "remix", "hifi", "midi" or "off"
var song8 := "dubstep"          # remix of song 8: "dubstep" or "darkwave"
var music_volume_db := 0.0
var effects_volume_db := 0.0


func _ready() -> void:
	load_settings()


func load_settings() -> void:
	var cf := ConfigFile.new()
	if cf.load(PATH) != OK:
		return
	resolution = cf.get_value("display", "resolution", resolution)
	display = cf.get_value("display", "mode", display)
	music = cf.get_value("audio", "music", music)
	if not ["mix", "remix", "hifi", "midi", "off"].has(music):
		music = "mix"
	song8 = cf.get_value("audio", "song8", song8)
	music_volume_db = cf.get_value("audio", "music_volume_db", music_volume_db)
	effects_volume_db = cf.get_value("audio", "effects_volume_db", effects_volume_db)


func save_settings() -> void:
	var cf := ConfigFile.new()
	cf.set_value("display", "resolution", resolution)
	cf.set_value("display", "mode", display)
	cf.set_value("audio", "music", music)
	cf.set_value("audio", "song8", song8)
	cf.set_value("audio", "music_volume_db", music_volume_db)
	cf.set_value("audio", "effects_volume_db", effects_volume_db)
	cf.save(PATH)


static func parse(res: String) -> Vector2i:
	var p := res.split("x")
	return Vector2i(int(p[0]), int(p[1]))


# The monitor's own aspect ratio, at the largest size no taller than 1080
# lines (the game's text is drawn in pixels; taller desktops make it tiny).
func auto_resolution() -> Vector2i:
	var screen := DisplayServer.screen_get_size(DisplayServer.window_get_current_screen())
	if screen.x <= 0 or screen.y <= 0:
		return Vector2i(1024, 768)
	var aspect := float(screen.x) / float(screen.y)
	var best := Vector2i(1024, 768)
	var best_err := 1e9
	for r in RESOLUTIONS:
		var v := parse(r)
		if v.y > 1080 or v.x > screen.x or v.y > screen.y:
			continue
		var err := absf(float(v.x) / float(v.y) - aspect) * 10000.0 - v.y
		if err < best_err:
			best_err = err
			best = v
	return best


func desktop_size() -> Vector2i:
	if resolution == "auto" or not RESOLUTIONS.has(resolution):
		return auto_resolution()
	return parse(resolution)
