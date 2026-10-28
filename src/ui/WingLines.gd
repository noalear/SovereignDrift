## WingLines -- what wingmen say on the radio when they take an order.
##
## Voiced like the tutorial (tools/voice/generate.py renders
## assets/voice/wing/<voice>_<id>.ogg for every voice below, with the radio
## band-pass), so each wingman keeps one voice: the pilot's name picks it.
class_name WingLines
extends RefCounted

const SET := "wing"

## Kokoro voices for wingmen; a pilot's name picks one.
const VOICES := {
	"wm1": {"voice": "am_michael", "speed": 1.05, "fx": "radio"},
	"wm2": {"voice": "bm_george", "speed": 1.04, "fx": "radio"},
	"wf1": {"voice": "af_bella", "speed": 1.05, "fx": "radio"},
	"wf2": {"voice": "bf_emma", "speed": 1.04, "fx": "radio"},
}

## [id, text]: the order lines (Wing.WHEEL ids) and a few reports.
const LINES := [
	["form", "Copy. Forming up on you."],
	["attack", "Copy. Engaging your target."],
	["cover", "On your wing. I've got your back."],
	["hold", "Copy. Holding here."],
	["engage", "Weapons free. Going hunting."],
	["home", "Heading home for repairs."],
	["join", "Reporting in. I'm on your wing."],
	["dismiss", "Understood. Back to the fleet."],
	["kill", "Target's down. Back on your wing."],
	["bingo", "Fuel's low. Heading home, back soon."],
	["back", "Repaired, and back on your wing."],
]


static func text(id: String) -> String:
	for row in LINES:
		if row[0] == id:
			return row[1]
	return ""


## The same pilot always has the same voice.
static func voice_for(pilot: String) -> String:
	var keys := VOICES.keys()
	return keys[absi(pilot.hash()) % keys.size()]


static func audio_path(voice: String, id: String) -> String:
	return "res://assets/voice/%s/%s_%s.ogg" % [SET, voice, id]
