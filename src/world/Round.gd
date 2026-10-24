## Round -- the rules of a game of Conquest.  Runs on the host: keeps the
## clock, decides the phase, counts planets, ends the round and hands the
## scoreboard everything it needs.  Clients only show what they are told.
class_name Round
extends RefCounted

signal phase_changed(phase: String)
signal finished(result: Dictionary)

const FINAL_MINUTES := 5.0         # the last stretch: income and captures doubled
const HOLD_SECONDS := 45.0         # holding the whole system this long ends the round

var world: Node
var length := 0.0                  # seconds, 0 means "until the system falls"
var elapsed := 0.0
var phase := "opening"             # opening | war | final | sudden_death | over
var hold_timer := 0.0
var hold_race := -1
var over := false
var result := {}
var events: Array = []             # [time, text, race] for the scoreboard timeline
var captures := {0: 0, 1: 0, 2: 0, 3: 0}
var kills := {0: 0, 1: 0, 2: 0, 3: 0}
var baseline := {}                 # ship -> [score, kills] when the round began


func setup(p_world: Node, minutes: int) -> void:
	world = p_world
	length = minutes * 60.0
	for s in world.ships:
		baseline[s] = [s.score, s.kills]


## What a pilot did this round (careers carry their old score and kills).
func round_score(s: Node) -> int:
	return maxi(s.score - int(baseline.get(s, [0, 0])[0]), 0)


func round_kills(s: Node) -> int:
	return maxi(s.kills - int(baseline.get(s, [0, 0])[1]), 0)


func time_left() -> float:
	return maxf(length - elapsed, 0.0) if length > 0.0 else -1.0


func bonus_multiplier() -> float:
	return 2.0 if phase in ["final", "sudden_death"] else 1.0


func clock_text() -> String:
	if length <= 0.0:
		return _fmt(elapsed)
	return _fmt(time_left())


static func _fmt(t: float) -> String:
	var s := int(t)
	return "%d:%02d" % [s / 60, s % 60]


## Who holds what; pirates count as a holder but can never win.
func planet_counts() -> Dictionary:
	var counts := {0: 0, 1: 0, 2: 0, 3: 0}
	for p in world.planets:
		if p.is_colonised():
			counts[p.owner_race] = counts.get(p.owner_race, 0) + 1
	return counts


func empire_score(race: int) -> int:
	var total := 0
	for s in world.ships:
		if s.race_id == race:
			total += round_score(s)
	return total


## Empires ordered by planets held, then by their pilots' score.
func standings() -> Array:
	var counts := planet_counts()
	var rows := []
	for r in [0, 1, 2]:
		rows.append({"race": r, "planets": counts.get(r, 0), "score": empire_score(r), "kills": kills.get(r, 0), "captures": captures.get(r, 0)})
	rows.sort_custom(func(a, b): return a["planets"] > b["planets"] if a["planets"] != b["planets"] else a["score"] > b["score"])
	return rows


func note(text: String, race: int = -1) -> void:
	events.append([elapsed, text, race])
	if events.size() > 40:
		events.pop_front()


func on_kill(killer_race: int) -> void:
	if killer_race >= 0:
		kills[killer_race] = kills.get(killer_race, 0) + 1


func on_capture(race: int, planet_name: String, from_race: int) -> void:
	captures[race] = captures.get(race, 0) + 1
	note("%s took %s%s" % [GameData.race(race)["name"], planet_name, (" from the %s" % GameData.race(from_race)["name"]) if from_race >= 0 else ""], race)


## A new colony on an empty world counts with the captures.
func on_settle(race: int, planet_name: String) -> void:
	captures[race] = captures.get(race, 0) + 1
	note("%s settled %s" % [GameData.race(race)["name"], planet_name], race)


func tick(delta: float) -> void:
	if over:
		return
	elapsed += delta
	var counts := planet_counts()
	# phases by the clock
	var new_phase := phase
	if phase == "sudden_death":
		pass
	elif elapsed < 90.0:
		new_phase = "opening"
	elif length > 0.0 and time_left() <= FINAL_MINUTES * 60.0:
		new_phase = "final"
	else:
		new_phase = "war"
	if new_phase != phase:
		phase = new_phase
		match phase:
			"war":
				world.notify_all("The system is at war. Capture planets: each one pays its empire every minute.", Color(1, 0.95, 0.8))
			"final":
				world.notify_all("Final %d minutes: planet income and capture bonuses are doubled." % int(FINAL_MINUTES), Color(1, 0.8, 0.4))
				note("Final phase", -1)
		phase_changed.emit(phase)
	# conquest: one empire holds every colony for a while
	var holders := 0
	var who := -1
	for r in counts:
		if counts[r] > 0:
			holders += 1
			who = r
	if holders == 1 and who >= 0 and who <= 2:
		if hold_race != who:
			hold_race = who
			hold_timer = 0.0
			world.notify_all("The %s hold the whole system. %d seconds to victory." % [GameData.race(who)["name"], int(HOLD_SECONDS)], GameData.race(who)["color"])
		hold_timer += delta
		if hold_timer >= HOLD_SECONDS:
			_finish(who, "conquest")
			return
	else:
		hold_race = -1
		hold_timer = 0.0
	# the clock runs out: most planets wins, score breaks ties, a dead heat
	# goes to sudden death until the next capture
	if length > 0.0 and elapsed >= length:
		var rows := standings()
		var tied: bool = rows[0]["planets"] == rows[1]["planets"] and rows[0]["score"] == rows[1]["score"]
		if tied:
			if phase != "sudden_death":
				phase = "sudden_death"
				world.notify_all("Time is up and the empires are level: sudden death, the next capture decides it.", Color(1, 0.6, 0.4))
				phase_changed.emit(phase)
		else:
			_finish(rows[0]["race"], "time")


## Sudden death settled by a capture.
func decide(winner: int) -> void:
	if not over:
		_finish(winner, "time")


func _finish(winner: int, how: String) -> void:
	over = true
	phase = "over"
	var pilots := []
	for s in world.ships:
		if s.is_station or s.is_prize:
			continue
		pilots.append({"name": s.pilot_name, "race": s.race_id, "rank": s.rank, "kills": round_kills(s), "deaths": s.deaths,
			"captures": s.captures, "boarded": s.ships_captured, "score": round_score(s), "is_player": s.is_player, "hull": s.design_name, "credits_earned": s.credits_earned})
	pilots.sort_custom(func(a, b): return a["score"] > b["score"])
	result = {"winner": winner, "how": how, "elapsed": elapsed, "standings": standings(), "pilots": pilots, "events": events.duplicate()}
	note("%s win the system by %s" % [GameData.race(winner)["name"], "conquest" if how == "conquest" else "planets held"], winner)
	finished.emit(result)
