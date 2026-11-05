# Screenshots of every title-screen page at the window's size, for a
# layout check at different resolutions:
#   SF_SHOTS=/tmp/shots xvfb-run -a -s "-screen 0 1366x768x24" godot --path . \
#       --resolution 1366x768 --rendering-driver opengl3 -s tools/menu_shots.gd
extends SceneTree

var frames := 0
var main: Node
var out_dir := "/tmp/claude-0/sf/pages"
var pages := ["main", "single", "online", "online_versus", "practice", "pilot", "settings", "controls"]
var at := 0


func _initialize() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	DirAccess.make_dir_recursive_absolute(out_dir)
	main = load("res://src/Main.tscn").instantiate()
	root.add_child(main)


func _process(_delta: float) -> bool:
	frames += 1
	if frames < 100:
		return false
	if (frames - 100) % 20 == 0:
		if at > 0:
			var img := root.get_viewport().get_texture().get_image()
			var vp := root.get_viewport().get_visible_rect().size
			img.save_png("%s/%s_%dx%d.png" % [out_dir, pages[at - 1], int(vp.x), int(vp.y)])
			print("saved ", pages[at - 1])
		if at >= pages.size():
			quit(0)
			return true
		main.menu.versus = pages[at].ends_with("_versus")   # the Online page with Competitive picked
		main.menu._set_page(pages[at].trim_suffix("_versus"))
		main.menu.intro = 1.0
		main.menu.page_fade = 0.0
		at += 1
	return false
