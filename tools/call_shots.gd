# Screenshots of the story campaign's video calls: every caller in their
# painted place, at rest and talking, the way the call window and the
# briefing (key M) draw them.
#   xvfb-run -a -s "-screen 0 1920x1080x24" godot --path . --rendering-driver opengl3 -s tools/call_shots.gd
extends SceneTree

const CALLS := [["hale", "ops_room"], ["okafor", "admiralty"], ["brandt", "cockpit_terran"], ["freighter", "bridge_freighter"],
	["raider", "cockpit_pirate"], ["marrow", "port_backroom"], ["kesh", "bridge_drengin"]]

var out_dir := "/tmp/claude-0/sf/shots"
var board: Control
var frames := 0


class Board extends Control:
	var talk := 0.0

	func _draw() -> void:
		draw_rect(Rect2(Vector2.ZERO, size), Color(0.02, 0.03, 0.05))
		for i in CALLS.size():
			var r := Rect2(20 + (i % 4) * 470, 20 + (i / 4) * 380, 450, 337)
			CallView.draw_call(self, r, CALLS[i][0], CALLS[i][1], talk, 0.0, 1.0)


func _init() -> void:
	call_deferred("_start")


func _start() -> void:
	if OS.has_environment("SF_SHOTS"):
		out_dir = OS.get_environment("SF_SHOTS")
	DirAccess.make_dir_recursive_absolute(out_dir)
	board = Board.new()
	board.size = Vector2(1900, 780)
	root.add_child(board)


func _process(_delta: float) -> bool:
	frames += 1
	if frames == 10:
		root.get_texture().get_image().save_png(out_dir + "/calls_rest.png")
		board.talk = 1.0
		board.queue_redraw()
	elif frames == 20:
		root.get_texture().get_image().save_png(out_dir + "/calls_talk.png")
		return true
	return false
