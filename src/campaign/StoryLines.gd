## StoryLines -- every spoken line of the story campaign, who says it and
## where they are calling from.  tools/voice/generate.py renders each line
## with the speaker's Kokoro voice into assets/voice/campaign/<id>.ogg; the
## Comms window shows the speaker in that place while it plays.
##
## The words are plain English (Kyle: no spelled-out accents); aliens get
## their sound from the effect chain, not the spelling.
class_name StoryLines
extends RefCounted

const SET := "campaign"

## voice: Kokoro voice; fx: the effect chain over comms ("radio"), and
## fx_near face to face in a briefing; look: how the call window
## draws them until their painting is in (race, hair, skin, uniform).
const SPEAKERS := {
	"hale": {"name": "Commander Hale", "title": "Lantern Squadron", "voice": "af_heart", "speed": 0.96, "fx": "radio", "fx_near": "clean",
		"color": Color(0.55, 0.8, 1.0), "race": 1, "look": {"skin": Color(0.86, 0.68, 0.55), "hair": Color(0.22, 0.14, 0.1), "uniform": Color(0.16, 0.26, 0.42), "long_hair": true}},
	"brandt": {"name": "Lt. Brandt", "title": "Lantern Two, \"Kettle\"", "voice": "am_michael", "speed": 1.02, "fx": "radio", "fx_near": "radio",
		"color": Color(0.5, 0.9, 0.75), "race": 1, "look": {"skin": Color(0.62, 0.45, 0.33), "hair": Color(0.08, 0.06, 0.05), "uniform": Color(0.2, 0.3, 0.38), "helmet": true}},
	"okafor": {"name": "Admiral Okafor", "title": "Terran Fleet Command", "voice": "bm_george", "speed": 0.94, "fx": "radio", "fx_near": "clean",
		"color": Color(0.85, 0.85, 0.6), "race": 1, "look": {"skin": Color(0.36, 0.24, 0.17), "hair": Color(0.75, 0.75, 0.75), "uniform": Color(0.12, 0.16, 0.3), "cap": true}},
	"freighter": {"name": "Captain Ruiz", "title": "TSS Cormorant", "voice": "bm_daniel", "speed": 1.0, "fx": "radio", "fx_near": "radio",
		"color": Color(0.9, 0.75, 0.5), "race": 1, "look": {"skin": Color(0.78, 0.6, 0.47), "hair": Color(0.35, 0.3, 0.28), "uniform": Color(0.4, 0.34, 0.24)}},
	"raider": {"name": "Unknown raider", "title": "Pirate transponder", "voice": "am_fenrir", "speed": 0.95, "fx": "radio", "fx_near": "radio",
		"color": Color(1.0, 0.6, 0.35), "race": 2, "look": {"uniform": Color(0.25, 0.2, 0.18), "helmet": true, "visor": true}},
}

## Where a caller can be.  The window paints the place behind them.
const PLACES := {
	"ops_room": {"name": "Terran starbase, operations"},
	"admiralty": {"name": "Fleet Command, Earth"},
	"cockpit_terran": {"name": "Terran fighter cockpit"},
	"bridge_freighter": {"name": "Freighter bridge"},
	"cockpit_pirate": {"name": "Raider cockpit"},
}

## [id, speaker, place, text]
const LINES := [
	# --- calls to a briefing
	["call_first_watch", "hale", "ops_room", "Lantern One, this is Hale. Welcome to the squadron. I have your first job waiting. Open the briefing when you're ready."],
	["call_dark_convoy", "hale", "ops_room", "Lantern One, Hale. Something's come up in the belt, and I want you on it. The briefing is ready when you are."],
	["call_more", "hale", "ops_room", "That's all I have for you for now, Lantern. Fly patrols, help the supply runs, and keep your eyes open. I'll call."],
	# --- Operation 1: First Watch
	["fw_brief_1", "hale", "ops_room", "Here's the job. The supply ship Cormorant is taking fuel cells out to the free port at Jupiter's Trojan point, to trade for alloys. You and Lieutenant Brandt fly escort."],
	["fw_brief_2", "hale", "ops_room", "Convoys have been going missing on that route, so we're not taking chances. Stay with her until she's safe at the port."],
	["fw_brief_3", "hale", "ops_room", "And remember, we are under the Ceres Accord. Drengin and Arcean ships may come and take a look. Nobody fires first. Nobody. Understood?"],
	["fw_start", "brandt", "cockpit_terran", "Lantern Two here. Kettle, if anyone's asking. I'm sitting on the Cormorant. Come say hello and we'll get her moving."],
	["fw_formed", "brandt", "cockpit_terran", "There you are. Cormorant, Lantern flight is with you. Take her out."],
	["fw_cormorant", "freighter", "bridge_freighter", "Cormorant copies, Lantern. Burning for the free port. Nice and slow, she's heavy today."],
	["fw_scout", "brandt", "cockpit_terran", "Hold on. Contact on our flank. Drengin scout, and it's matching our course."],
	["fw_hold", "hale", "ops_room", "I see it, Lantern. It's in Accord space and it hasn't fired. Let it look. Hold your fire."],
	["fw_banter", "brandt", "cockpit_terran", "Just once, I'd like one of them to wave."],
	["fw_arrived", "freighter", "bridge_freighter", "Cormorant is at the free port. Thanks for the company, Lantern."],
	["fw_jam", "brandt", "cockpit_terran", "My scope just went white. It's jamming us!"],
	["fw_leave", "hale", "ops_room", "It's breaking off toward Mars. Let it go. That was a message, not an attack. Come home to the starbase, both of you."],
	["fw_done", "hale", "ops_room", "Clean first flight. No shots fired, and the cargo delivered. That is exactly how I like them."],
	["fw_done2", "brandt", "cockpit_terran", "She means well done. She just doesn't say it like that."],
	["fw_fail_cargo", "hale", "ops_room", "We've lost the Cormorant. Lantern, abort and come home."],
	["fw_fail_breach", "hale", "ops_room", "Lantern One, you fired on a Drengin ship in Accord space. Cease fire and come home. Now."],
	# --- Operation 2: Dark Convoy
	["dc_brief_1", "hale", "ops_room", "The fuel convoy from Ceres, the Tern and the Petrel, stopped answering two hours ago. Last known position, the edge of the belt near Ceres."],
	["dc_brief_2", "hale", "ops_room", "Fly out, find them, and get your sensors on whatever's left. If there are survivors, they come first."],
	["dc_brief_3", "hale", "ops_room", "Pirates are the obvious guess. I don't like obvious guesses. If you meet whoever did this, I want one of their ships in one piece."],
	["dc_start", "brandt", "cockpit_terran", "Ceres is a long way out. Wake me when we get there. Kidding. Mostly."],
	["dc_arrive", "brandt", "cockpit_terran", "There. Two hulls, both dark. Nothing moving."],
	["dc_scan", "hale", "ops_room", "Close in on each wreck, Lantern One. Your sensors will do the rest."],
	["dc_scan_1", "brandt", "cockpit_terran", "This one's been holed clean through. Whatever cut her was hot. Very hot."],
	["dc_scan_2", "hale", "ops_room", "Those are disruptor burns. That's a Drengin weapon. Pirates don't carry those."],
	["dc_ambush", "brandt", "cockpit_terran", "Two contacts, coming out of the rocks! Pirate transponders."],
	["dc_raider", "raider", "cockpit_pirate", "Terran ships. Power down your engines, and you might live."],
	["dc_weapons", "hale", "ops_room", "Pirates are not under the Accord. Weapons free. Knock out the engines on one of them. I want it boarded."],
	["dc_disabled", "brandt", "cockpit_terran", "Its engines are out! It's dead in space. Go and get it, I'll cover you."],
	["dc_captured", "brandt", "cockpit_terran", "We're in. Commander, you'll want to see this. Their log is in Drengin. They didn't even wipe it."],
	["dc_captured2", "hale", "ops_room", "Pirate colours and a Drengin log. Bring that ship home, and say nothing on open channels."],
	["dc_done", "okafor", "admiralty", "Commander Hale sent me your log. If Mars is behind these raids, then the Accord is already over. We just haven't been told yet."],
	["dc_done2", "hale", "ops_room", "Get some rest, Lantern. Things are about to get busy."],
	["dc_fail_lost", "hale", "ops_room", "Both raiders are gone, and there's no ship to bring home. We'll have to do this again."],
	# --- any time
	["breach_warn", "hale", "ops_room", "Lantern One, that's an Accord ship. Hold your fire."],
	["brandt_down", "hale", "ops_room", "Lantern Two is out of the fight. He ejected, and he's safe. Keep going."],
	["player_down", "hale", "ops_room", "Lantern One, you're down. Take a new ship from the starbase and get back out there."],
]


static func row(id: String) -> Array:
	for r in LINES:
		if r[0] == id:
			return r
	return []


static func text(id: String) -> String:
	var r := row(id)
	return r[3] if not r.is_empty() else ""


static func speaker(id: String) -> Dictionary:
	var r := row(id)
	return SPEAKERS.get(r[1], {}) if not r.is_empty() else {}


static func place(id: String) -> String:
	var r := row(id)
	return r[2] if not r.is_empty() else ""


## The sound of a line: face to face in a briefing, over comms else.
static func fx(id: String) -> String:
	var r := row(id)
	if r.is_empty():
		return "radio"
	var spk: Dictionary = SPEAKERS.get(r[1], {})
	return spk.get("fx_near", "clean") if is_briefing(id) else spk.get("fx", "radio")


static func is_briefing(id: String) -> bool:
	return "_brief_" in id


static func audio_path(id: String) -> String:
	return "res://assets/voice/%s/%s.ogg" % [SET, id]
