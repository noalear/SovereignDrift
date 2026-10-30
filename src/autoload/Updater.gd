## Updater -- keeps an installed copy up to date.  The game data that came
## with the installer is the base; an update is a small patch pack holding
## only the files that changed since that base, loaded over it at start.
## At launch it reads the manifest, downloads a newer patch in the
## background (in parts when it is too big for one file on GitHub, joined
## here), and the main menu offers a restart.  Builds without
## res://build.json (editor and test runs) never check.
##
## This must stay the first autoload: the patch has to be in place before
## any other script loads.  A change to this file, project.godot or the
## engine needs a new installer, since those load before any patch.
extends Node

const MANIFEST_URL := "https://raw.githubusercontent.com/noalear/SFUpdater/main/updates.json"
const DIR := "user://updates/"
const INFO := DIR + "patch.json"     # {"base", "build", "file"}: the patch to load at start

signal changed()

var base := ""          # the build the installer put in
var build := ""         # the build running now: the base, or the patch over it
var version := ""
var state := "off"      # off | checking | current | downloading | ready | installer | failed
var note := ""          # one line for the main menu
var installer_url := ""
var latest := ""
var _want := {}         # the manifest's patch for our base
var _http: HTTPRequest
var _file := ""         # where the download goes
var _parts: Array = []  # [{url, size, sha256}] still to fetch, in order
var _part := 0          # which part is downloading
var _done := 0          # bytes of the parts already down


func _init() -> void:
	var info := _read_json("res://build.json")
	if info.is_empty():
		return
	base = str(info.get("build", ""))
	build = base
	version = str(info.get("version", ""))
	var patch := _read_json(INFO)
	var keep := ""
	if str(patch.get("base", "")) == base and FileAccess.file_exists(DIR + str(patch.get("file", ""))):
		keep = str(patch["file"])
		if ProjectSettings.load_resource_pack(DIR + keep):
			var now := _read_json("res://build.json")
			build = str(now.get("build", base))
			version = str(now.get("version", version))
		else:
			keep = ""
	# patches for another base (a newer installer went in) or half downloads
	var d := DirAccess.open(DIR)
	if d != null:
		for f in d.get_files():
			if f != keep and f != "patch.json":
				d.remove(f)
	if keep.is_empty() and FileAccess.file_exists(INFO):
		DirAccess.remove_absolute(INFO)


func _ready() -> void:
	if base.is_empty():
		return
	print("Updater: build %s (installed %s)" % [build, base])
	_http = HTTPRequest.new()
	_http.use_threads = true
	_http.timeout = 30.0
	add_child(_http)
	_http.request_completed.connect(_on_manifest)
	_to("checking", "Checking for updates...")
	var url := OS.get_environment("SF_UPDATE_URL")
	if url.is_empty():
		url = MANIFEST_URL
	if _http.request(url + "?t=%d" % Time.get_unix_time_from_system()) != OK:
		_to("failed", "Couldn't check for updates.")


func _process(_delta: float) -> void:
	if state == "downloading" and _http != null:
		var size := maxi(int(_want.get("size", 1)), 1)
		note = "Downloading update: %d%% of %.1f MB" % [mini(100, (_done + _http.get_downloaded_bytes()) * 100 / size), size / 1048576.0]


## Restart into the downloaded update with the same command line.
func restart() -> void:
	OS.set_restart_on_exit(true, OS.get_cmdline_args())
	get_tree().quit()


func _on_manifest(result: int, code: int, _headers: PackedStringArray, body: PackedByteArray) -> void:
	_http.request_completed.disconnect(_on_manifest)
	var m: Variant = JSON.parse_string(body.get_string_from_utf8()) if result == HTTPRequest.RESULT_SUCCESS and code == 200 else null
	if typeof(m) != TYPE_DICTIONARY:
		_to("failed", "Couldn't check for updates.")
		return
	latest = str(m.get("latest", ""))
	installer_url = str(m.get("installer", ""))
	var patches: Dictionary = m.get("patches", {})
	if latest.is_empty() or latest == build:
		_to("current", "Up to date.")
	elif latest == base:
		# the newest build is the installed one: drop the patch at next start
		DirAccess.remove_absolute(INFO)
		_to("ready", "Update ready: restart to finish.")
	elif patches.has(base):
		_want = patches[base]
		var have := _read_json(INFO)
		if str(have.get("build", "")) == latest:
			_to("ready", "Update ready: restart to play build %s." % latest)
			return
		DirAccess.make_dir_recursive_absolute(DIR)
		_file = "%s.pck" % latest
		# one file, or parts that are joined once they are all down
		_parts = _want.get("parts", [{"url": _want.get("url", ""), "size": _want.get("size", 0), "sha256": _want.get("sha256", "")}])
		_part = 0
		_done = 0
		_http.request_completed.connect(_on_patch)
		_to("downloading", "Downloading update...")
		_fetch_part()
	else:
		_to("installer", "A new version needs the full installer.")


func _part_path(i: int) -> String:
	return DIR + _file + ".part%d" % i


func _fetch_part() -> void:
	_http.download_file = _part_path(_part)
	if _http.request(str(_parts[_part].get("url", ""))) != OK:
		_http.request_completed.disconnect(_on_patch)
		_to("failed", "The update download didn't start.")


func _on_patch(result: int, code: int, _headers: PackedStringArray, _body: PackedByteArray) -> void:
	var got := _part_path(_part)
	if result != HTTPRequest.RESULT_SUCCESS or code != 200:
		_drop_parts()
		_to("failed", "The update download failed. It will try again next start.")
		return
	if FileAccess.get_sha256(got) != str(_parts[_part].get("sha256", "")):
		_drop_parts()
		_to("failed", "The update download was damaged. It will try again next start.")
		return
	_done += int(_parts[_part].get("size", 0))
	_part += 1
	if _part < _parts.size():
		_fetch_part()
		return
	_http.request_completed.disconnect(_on_patch)
	var whole := DIR + _file + ".part"
	if _parts.size() == 1:
		DirAccess.rename_absolute(_part_path(0), whole)
	else:
		var out := FileAccess.open(whole, FileAccess.WRITE)
		for i in _parts.size():
			out.store_buffer(FileAccess.get_file_as_bytes(_part_path(i)))
			DirAccess.remove_absolute(_part_path(i))
		out.close()
	if FileAccess.get_sha256(whole) != str(_want.get("sha256", "")):
		DirAccess.remove_absolute(whole)
		_to("failed", "The update download was damaged. It will try again next start.")
		return
	DirAccess.rename_absolute(whole, DIR + _file)
	var f := FileAccess.open(INFO, FileAccess.WRITE)
	f.store_string(JSON.stringify({"base": base, "build": latest, "file": _file}))
	f.close()
	_to("ready", "Update ready: restart to play build %s." % latest)


func _drop_parts() -> void:
	_http.request_completed.disconnect(_on_patch)
	for i in _parts.size():
		DirAccess.remove_absolute(_part_path(i))


func _to(s: String, line: String) -> void:
	state = s
	note = line
	print("Updater: %s: %s" % [s, line])
	changed.emit()


static func _read_json(path: String) -> Dictionary:
	if not FileAccess.file_exists(path):
		return {}
	var v: Variant = JSON.parse_string(FileAccess.get_file_as_string(path))
	return v if typeof(v) == TYPE_DICTIONARY else {}
