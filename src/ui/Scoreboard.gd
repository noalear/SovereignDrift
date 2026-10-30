## Scoreboard -- the end of a round: who won and how, the planets held,
## every pilot's line, your own summary, and the way to the next round.
class_name Scoreboard
extends Control

signal play_again()
signal to_menu()

var result := {}
var online := ""            # "host": Next round keeps the friends, "client": the host starts it
var player_race := 1
var font: Font
var time := 0.0
var buttons: Array = []     # [rect, callback]
var focus := 0
var hover := -1

const TEXT := Color(0.9, 0.95, 1.0)
const DIM := Color(0.55, 0.65, 0.8)


func open(p_result: Dictionary, p_player_race: int) -> void:
	result = p_result
	player_race = p_player_race
	mouse_filter = Control.MOUSE_FILTER_STOP
	process_mode = Node.PROCESS_MODE_ALWAYS
	font = ThemeDB.fallback_font


func _ready() -> void:
	# fill the screen; anchors alone leave a Control under a CanvasLayer at
	# size zero, which no click can reach
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)


func _process(delta: float) -> void:
	time += delta
	queue_redraw()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		hover = -1
		for i in buttons.size():
			if buttons[i][0].has_point(event.position):
				hover = i
				focus = i
	elif event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		for i in buttons.size():
			if buttons[i][0].has_point(event.position):
				Sfx.play_ui("click", 0.5)
				buttons[i][1].call()
		accept_event()


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		if focus < buttons.size() and online != "client":
			buttons[focus][1].call()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("ui_cancel") or event.is_action_pressed("pause_menu"):
		to_menu.emit()
		get_viewport().set_input_as_handled()
	elif event.is_action_pressed("ui_left") or event.is_action_pressed("ui_right") or event.is_action_pressed("ui_up") or event.is_action_pressed("ui_down"):
		focus = (focus + 1) % maxi(buttons.size(), 1)
		get_viewport().set_input_as_handled()


func _button(rect: Rect2, label: String, cb: Callable) -> void:
	buttons.append([rect, cb])
	var idx := buttons.size() - 1
	var lit := hover == idx or focus == idx
	draw_rect(rect, Color(0.18, 0.3, 0.55, 0.95) if lit else Color(0.08, 0.14, 0.28, 0.85))
	draw_rect(rect, Color(0.5, 0.7, 1.0, 0.7), false, 1.5)
	if focus == idx:
		draw_rect(rect.grow(3), Color(1.0, 0.95, 0.7, 0.6 + 0.3 * sin(time * 6.0)), false, 2.0)
	draw_string(font, rect.position + Vector2(0, rect.size.y * 0.5 + 7), label, HORIZONTAL_ALIGNMENT_CENTER, rect.size.x, 19, Color.WHITE)


func _draw() -> void:
	buttons.clear()
	var vp := get_viewport_rect().size
	draw_rect(Rect2(Vector2.ZERO, vp), Color(0.0, 0.01, 0.03, 0.9))
	var w := minf(1100.0, vp.x - 60.0)
	var x := (vp.x - w) * 0.5
	var y := 50.0
	var winner: int = result.get("winner", -1)
	var wcol: Color = GameData.race(winner)["color"] if winner >= 0 else Color.WHITE
	var won := winner == player_race
	# banner
	var headline := "%s %s" % [GameData.race(winner).get("plural", GameData.race(winner)["name"]), "win the Sol system"] if winner >= 0 else "The round is over"
	draw_string(font, Vector2(x, y + 40), headline.to_upper(), HORIZONTAL_ALIGNMENT_CENTER, w, 40, wcol)
	var how: String = result.get("how", "")
	var sub := "Victory" if won else "Defeat"
	sub += "  ·  " + ("every colony held" if how == "conquest" else "most planets when time ran out") + "  ·  round time %s" % Round._fmt(result.get("elapsed", 0.0))
	draw_string(font, Vector2(x, y + 70), sub, HORIZONTAL_ALIGNMENT_CENTER, w, 17, Color(1, 0.95, 0.8) if won else DIM)
	y += 100
	# standings
	var sx := x
	var sw := w * 0.42
	draw_string(font, Vector2(sx, y), "EMPIRES", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	draw_string(font, Vector2(sx + 170, y), "planets", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(sx + 250, y), "captures", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(sx + 330, y), "kills", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(sx + 390, y), "score", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	var ry := y
	for row in result.get("standings", []):
		ry += 26
		var col: Color = GameData.race(row["race"])["color"]
		draw_string(font, Vector2(sx, ry), GameData.race(row["race"])["name"], HORIZONTAL_ALIGNMENT_LEFT, -1, 16, col)
		draw_string(font, Vector2(sx + 170, ry), str(row["planets"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
		draw_string(font, Vector2(sx + 250, ry), str(row["captures"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
		draw_string(font, Vector2(sx + 330, ry), str(row["kills"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
		draw_string(font, Vector2(sx + 390, ry), str(row["score"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, TEXT)
	# your round
	ry += 44
	draw_string(font, Vector2(sx, ry), "YOUR ROUND", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	for pl in result.get("pilots", []):
		if pl["is_player"]:
			var lines := [
				"%s, %s of the %s" % [pl["name"], GameData.rank_name(pl["rank"]), GameData.race(pl["race"])["name"]],
				"Kills %d   Deaths %d   Planets captured %d" % [pl["kills"], pl["deaths"], pl["captures"]],
				"Score %d   Credits earned %d   Last hull: %s" % [pl["score"], pl["credits_earned"], pl["hull"]],
			]
			for l in lines:
				ry += 22
				draw_string(font, Vector2(sx, ry), l, HORIZONTAL_ALIGNMENT_LEFT, sw, 14, TEXT)
	# the timeline
	ry += 40
	draw_string(font, Vector2(sx, ry), "HOW IT WENT", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	var ev: Array = result.get("events", [])
	var start := maxi(ev.size() - 10, 0)
	for i in range(start, ev.size()):
		ry += 18
		if ry > vp.y - 110:
			break
		var e: Array = ev[i]
		var col: Color = GameData.race(e[2])["color"] if e[2] >= 0 else DIM
		draw_string(font, Vector2(sx, ry), "%s  %s" % [Round._fmt(e[0]), e[1]], HORIZONTAL_ALIGNMENT_LEFT, sw + 40, 12, col)
	# pilots table
	var px := x + w * 0.5
	var pw := w * 0.5
	draw_string(font, Vector2(px, y), "PILOTS", HORIZONTAL_ALIGNMENT_LEFT, -1, 15, Color(0.6, 0.8, 1.0))
	draw_string(font, Vector2(px + 230, y), "rank", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(px + 340, y), "kills", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(px + 395, y), "deaths", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(px + 460, y), "caps", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	draw_string(font, Vector2(px + 510, y), "score", HORIZONTAL_ALIGNMENT_LEFT, -1, 13, DIM)
	var py := y
	for pl in result.get("pilots", []):
		py += 20
		if py > vp.y - 110:
			break
		var col: Color = GameData.race(pl["race"])["color"]
		var tcol := Color(1, 0.95, 0.7) if pl["is_player"] else TEXT
		draw_string(font, Vector2(px, py), pl["name"], HORIZONTAL_ALIGNMENT_LEFT, 220, 14, col if not pl["is_player"] else tcol)
		draw_string(font, Vector2(px + 230, py), GameData.rank_name(pl["rank"]), HORIZONTAL_ALIGNMENT_LEFT, 105, 13, tcol)
		draw_string(font, Vector2(px + 340, py), str(pl["kills"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, tcol)
		draw_string(font, Vector2(px + 395, py), str(pl["deaths"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, tcol)
		draw_string(font, Vector2(px + 460, py), str(pl["captures"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, tcol)
		draw_string(font, Vector2(px + 510, py), str(pl["score"]), HORIZONTAL_ALIGNMENT_LEFT, -1, 14, tcol)
	# career line and buttons
	var c: Dictionary = Settings.career
	draw_string(font, Vector2(x, vp.y - 72), "Career: %d rounds, %d won   ·   %d kills, %d deaths" % [c.get("rounds", 0), c.get("wins", 0), c.get("kills", 0), c.get("deaths", 0)], HORIZONTAL_ALIGNMENT_CENTER, w, 14, DIM)
	if online == "client":
		# the host decides when the next round starts; Enter does nothing here
		draw_string(font, Vector2(x, vp.y - 92), "Waiting for the host to start the next round" + ".".repeat(1 + int(time * 2.0) % 3), HORIZONTAL_ALIGNMENT_CENTER, w, 16, Color(1, 0.95, 0.7))
		_button(Rect2(vp.x * 0.5 - 115, vp.y - 56, 230, 44), "Leave the game  (Esc)", func(): to_menu.emit())
		return
	_button(Rect2(vp.x * 0.5 - 250, vp.y - 56, 230, 44), "Next round  (Enter)" if online == "host" else "Play again  (Enter)", func(): play_again.emit())
	_button(Rect2(vp.x * 0.5 + 20, vp.y - 56, 230, 44), "Main menu  (Esc)", func(): to_menu.emit())
