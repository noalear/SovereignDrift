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
	"marrow": {"name": "Ilsa Marrow", "title": "Broker, Jupiter free port", "voice": "bf_alice", "speed": 0.98, "fx": "radio", "fx_near": "radio",
		"color": Color(0.85, 0.7, 1.0), "race": 4, "look": {"skin": Color(0.9, 0.76, 0.66), "hair": Color(0.55, 0.2, 0.12), "uniform": Color(0.18, 0.14, 0.2), "long_hair": true}},
	"kesh": {"name": "Overlord Kesh Varr", "title": "Drengin Empire, Mars", "voice": "am_onyx", "speed": 0.9, "fx": "drengin", "fx_near": "drengin",
		"color": Color(1.0, 0.45, 0.35), "race": 2, "look": {"alien": "drengin", "skin": Color(0.42, 0.48, 0.36), "uniform": Color(0.35, 0.1, 0.08)}},
	"rhee": {"name": "Major Rhee", "title": "Third Drop Battalion", "voice": "am_eric", "speed": 1.0, "fx": "radio", "fx_near": "radio",
		"color": Color(0.75, 0.9, 0.55), "race": 1, "look": {"skin": Color(0.8, 0.64, 0.5), "hair": Color(0.1, 0.08, 0.07), "uniform": Color(0.3, 0.34, 0.22), "helmet": true}},
	"tenn": {"name": "Speaker Tenn", "title": "Envoy of the Arcean Union", "voice": "af_nicole", "speed": 0.95, "fx": "arcean", "fx_near": "arcean",
		"color": Color(0.7, 0.9, 1.0), "race": 0, "look": {"skin": Color(0.62, 0.72, 0.86), "hair": Color(0.85, 0.9, 0.95), "uniform": Color(0.2, 0.3, 0.45), "long_hair": true}},
	"corrigan": {"name": "Mags Corrigan", "title": "Pirate captain", "voice": "bf_isabella", "speed": 1.02, "fx": "radio", "fx_near": "radio",
		"color": Color(1.0, 0.7, 0.4), "race": 3, "look": {"skin": Color(0.88, 0.7, 0.58), "hair": Color(0.7, 0.25, 0.1), "uniform": Color(0.3, 0.22, 0.16), "long_hair": true}},
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
	"port_backroom": {"name": "Jupiter free port, back room"},
	"bridge_drengin": {"name": "Drengin flagship, Mars"},
	"dropship_bay": {"name": "Drop ship troop bay"},
	"bridge_arcean": {"name": "Arcean envoy ship"},
}

## [id, speaker, place, text]
const LINES := [
	# --- calls to a briefing
	["call_first_watch", "hale", "ops_room", "Lantern One, this is Hale. Welcome to the squadron. I have your first job waiting. Open the briefing when you're ready."],
	["call_dark_convoy", "hale", "ops_room", "Lantern One, Hale. Something's come up in the belt, and I want you on it. The briefing is ready when you are."],
	["call_free_port", "hale", "ops_room", "Lantern One, Hale. That raider's log gave us a name at the free port. The briefing is up when you're ready."],
	["call_picket_line", "hale", "ops_room", "Lantern One, Hale. We're putting a gun platform over Luna, and I want you on it. Open the briefing."],
	["call_accord_breaks", "hale", "ops_room", "Lantern One, scramble. Something is happening at Mars. Briefing, now."],
	["call_shields_down", "hale", "ops_room", "Lantern One, Hale. We're taking something back from Mars, and you're opening the door. The briefing is ready."],
	["call_wolf_pack", "hale", "ops_room", "Lantern One, Hale. You've earned a wing of your own. Open the briefing."],
	["call_envoy", "hale", "ops_room", "Lantern One, Hale. Something delicate has come up. The Arceans are sending someone to talk. Open the briefing."],
	["call_silent_running", "hale", "ops_room", "Lantern One, Hale. Intelligence has a rumour, and I need you to turn it into a picture. Briefing's up."],
	["call_burn_yards", "hale", "ops_room", "Lantern One, scramble. We're hitting the Hammer before it launches. Briefing, now."],
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
	# --- Operation 3: Free Port
	["port_brief_1", "hale", "ops_room", "The raider's log names a broker at the Jupiter free port, Ilsa Marrow. She sells everyone's secrets, including the transponder codes those pirates are flying under."],
	["port_brief_2", "hale", "ops_room", "Go and get those records. The port is neutral ground, so no weapons and no trouble. Fleet has put three hundred credits in your account for expenses."],
	["port_brief_3", "hale", "ops_room", "How you get them is your call, Lantern. Just don't start a war at the free port. We have one coming already."],
	["port_start", "brandt", "cockpit_terran", "I'm sitting this one out. Apparently my face is too well known at the port. Bring me back something nice."],
	["port_marrow_1", "marrow", "port_backroom", "A Terran fighter, all on its own. You must be Hale's new pilot. Yes, I have what you came for."],
	["port_marrow_2", "marrow", "port_backroom", "Transponder records, every code sold this year. My price is eight units of Exotics. Not credits, Exotics."],
	["port_choice", "hale", "ops_room", "Your call, Lantern. Pay her, or take the records quietly. Her data relay sits behind the port."],
	["port_pay_go", "hale", "ops_room", "Buy eight Exotics at the port's market, then stay alongside the port while she takes them."],
	["port_paid", "marrow", "port_backroom", "A pleasure doing business. The records are on your ship. Do come again."],
	["port_steal_go", "hale", "ops_room", "Go silent, slip round to the relay behind the port, and sit on it until the copy is done. If port security gets a contact on you, it's over."],
	["port_copied", "hale", "ops_room", "Copy's complete. Get clear before anyone looks too hard."],
	["port_found", "marrow", "port_backroom", "Someone has been in my records. Terran, by the smell of it. My prices just went up, for every one of you."],
	["port_records_1", "hale", "ops_room", "Here it is. Every raider code was bought by Varr Shipping. That is a Drengin front company."],
	["port_records_2", "hale", "ops_room", "Varr, as in Kesh Varr, the Overlord at Mars. Come home, Lantern."],
	["port_done", "hale", "ops_room", "Good work. Now we know who. We still don't know why."],
	["port_fail_seen", "hale", "ops_room", "Port security has you. Abort, Lantern, and come home before this gets ugly."],
	["port_fail_shot", "hale", "ops_room", "You fired on port security! Cease fire and get out of there."],
	# --- Operation 4: Picket Line
	["pk_brief_1", "hale", "ops_room", "Luna has no guns worth the name. The engineers have put up a defense station frame in Luna orbit, and it's waiting on eight units of Alloys."],
	["pk_brief_2", "hale", "ops_room", "Land on Earth, load the Alloys, and bring them up to the frame. Then stay with it until it's built. Brandt flies with you."],
	["pk_brief_3", "hale", "ops_room", "Pirates have been seen round Luna all week. If they come, that frame does not fall. Pirates are fair game."],
	["pk_start", "brandt", "cockpit_terran", "Kettle with you. Hauling building supplies. This is what I trained for."],
	["pk_loaded", "hale", "ops_room", "That's the Alloys. Take them up to the frame at Luna and come alongside to unload."],
	["pk_delivered", "hale", "ops_room", "Delivered. The crews are bolting it together now. Two minutes."],
	["pk_raid", "brandt", "cockpit_terran", "Contacts! Three of them, coming round Luna. Pirate transponders again."],
	["pk_raid2", "hale", "ops_room", "Protect that frame, Lantern. Weapons free on the raiders."],
	["pk_formation", "brandt", "cockpit_terran", "Look at them. Tight wedge, perfect spacing. Pirates don't fly like that. That's Drengin drill."],
	["pk_built", "hale", "ops_room", "The station is up and its guns are live. Luna has teeth now."],
	["pk_done", "hale", "ops_room", "Well flown. And Brandt is right. I've sent all of it to the Admiral."],
	["pk_done2", "okafor", "admiralty", "Three raids in three weeks, all of them Drengin under false colours. I'm putting the fleet on alert."],
	["pk_fail_frame", "hale", "ops_room", "The frame is gone. Pull back, Lantern. We'll have to build it again."],
	# --- Operation 5: The Accord Breaks
	["ab_brief_1", "hale", "ops_room", "Mars has gone quiet on every channel, and the Drengin fleet is moving. Overlord Kesh Varr wants to speak to the whole system."],
	["ab_brief_2", "hale", "ops_room", "Whatever he says, Luna is the first thing in his way. Get out there with Brandt and hold it until the fleet arrives. Six minutes."],
	["ab_brief_3", "hale", "ops_room", "If Luna can't be held, you fall back to the starbase. I need you alive more than I need that rock."],
	["ab_kesh_1", "kesh", "bridge_drengin", "People of Sol. For a generation the Accord has kept you weak, and kept you waiting."],
	["ab_kesh_2", "kesh", "bridge_drengin", "Terra starves itself on pirate stories. Today the Accord ends. Luna belongs to the Drengin."],
	["ab_kesh_3", "kesh", "bridge_drengin", "Whoever stands in our way will be remembered as a warning."],
	["ab_hot", "hale", "ops_room", "That's it. The Accord is over. All Terran ships, weapons free on Drengin targets."],
	["ab_brandt", "brandt", "cockpit_terran", "Here they come. A lot of them."],
	["ab_wave2", "hale", "ops_room", "Second wave inbound from Mars. Heavier this time."],
	["ab_wave3", "hale", "ops_room", "Last push, Lantern. Ninety seconds and the fleet is there."],
	["ab_luna_weak", "hale", "ops_room", "Luna's guns are down. If their troops land, we lose it."],
	["ab_fleet", "okafor", "admiralty", "This is Admiral Okafor. The fleet is here. Lantern Squadron, well held."],
	["ab_held", "hale", "ops_room", "Luna holds. You held it, Lantern. Come home."],
	["ab_lost", "hale", "ops_room", "Luna has fallen. Drengin troops are on the ground. Fall back to the starbase, now."],
	["ab_lost2", "okafor", "admiralty", "We lost Luna today. We will take it back. Lantern Squadron, regroup at Earth."],
	["ab_done", "hale", "ops_room", "That was the end of the peace. Get some sleep, Lantern. Tomorrow we go to war."],
	# --- Operation 6: Shields Down
	["sd_brief_1", "hale", "ops_room", "The Drengin have dug in on Deimos, the outer moon of Mars. It's the base for every raid on the inner system, and it sits behind a planetary shield."],
	["sd_brief_2", "hale", "ops_room", "Major Rhee's battalion is aboard two drop ships, the Anvil and the Bastion. Nothing lands while that shield is up. Your job is to shoot it down."],
	["sd_brief_3", "hale", "ops_room", "Once it falls it stays down for less than a minute. The drop ships wait out of range until it's down, then you cover them all the way to the ground."],
	["sd_start", "brandt", "cockpit_terran", "Lantern Two with you. Two drop ships into a shielded fortress. Somebody upstairs really likes us."],
	["sd_rhee", "rhee", "dropship_bay", "Anvil and Bastion on your wing, Lantern. Twelve units of Third Drop in the bays. Just get us a door."],
	["sd_contact", "brandt", "cockpit_terran", "Deimos ahead. Their guard is coming up to meet us."],
	["sd_shield", "hale", "ops_room", "Drop ships, hold at the marker. Lantern, put everything you have into that shield."],
	["sd_down", "hale", "ops_room", "The shield is down! Anvil, Bastion, go, go, go!"],
	["sd_drop", "rhee", "dropship_bay", "Going in. Keep them off our backs until we're on the ground."],
	["sd_again", "hale", "ops_room", "The shield is back up. Drop ships, pull back. Lantern, knock it down again."],
	["sd_landed", "rhee", "dropship_bay", "Third Drop is on the ground. Fighting toward the shield generator."],
	["sd_battle", "rhee", "dropship_bay", "Heavy fire down here. Stay low over us, Lantern. Every gun in orbit helps."],
	["sd_reinforce", "brandt", "cockpit_terran", "More of them, coming round from Mars. They want this rock back."],
	["sd_lost_one", "hale", "ops_room", "We've lost a drop ship. The other one has to make it."],
	["sd_taken", "rhee", "dropship_bay", "Deimos is ours. The garrison has surrendered. Raising the flag."],
	["sd_done", "hale", "ops_room", "First ground we've taken back. Their raids start from further away now. Well done, Lantern."],
	["sd_fail_droppers", "hale", "ops_room", "Both drop ships are down. Abort, Lantern. There's nobody left to land."],
	["sd_fail_beaten", "rhee", "dropship_bay", "They're pushing us off the moon. Third Drop is pulling out. I'm sorry, Lantern."],
	# --- Operation 7: Wolf Pack
	["wp_brief_1", "hale", "ops_room", "The Drengin are feeding their Mars fleet from Venus. Supply convoys run the lane between the two, every few hours."],
	["wp_brief_2", "hale", "ops_room", "Lantern Three and Lantern Four fly with you today. They're your wing. Give them orders: send them at a target and they'll take it apart."],
	["wp_brief_3", "hale", "ops_room", "Three supply ships and a light escort. Every ship that reaches Mars is another week of fuel for Kesh Varr's fleet. Don't let them through."],
	["wp_start", "hale", "ops_room", "Lantern Three and Four are forming on you. They take your orders, so give some."],
	["wp_sighted", "hale", "ops_room", "Contact. Drengin freighters on the lane, with escorts. That's your convoy."],
	["wp_orders", "hale", "ops_room", "Send your wing at the escorts and go for the freighters yourself. Or the other way round. Your call, flight lead."],
	["wp_one", "hale", "ops_room", "One freighter down."],
	["wp_two", "hale", "ops_room", "That's two. Don't let the last one run."],
	["wp_escaped", "hale", "ops_room", "One of them made it to Mars. Stop the rest."],
	["wp_wing_lost", "hale", "ops_room", "We've lost one of your wing. Keep the other one close."],
	["wp_kesh", "kesh", "bridge_drengin", "Terran wolves, picking at my supply lines. Enjoy it. The Hammer does not need fuel from Venus."],
	["wp_done", "okafor", "admiralty", "Mars will be short of fuel this week because of you, Lantern. That's how wars are won, one convoy at a time."],
	["wp_done2", "hale", "ops_room", "Good flying, flight lead. And who is the Hammer? Find out what you can. I'll do the same."],
	["wp_fail", "hale", "ops_room", "Too many of them got through. Break off, Lantern. We'll catch the next one."],
	# --- Operation 8: The Envoy
	["env_brief_1", "hale", "ops_room", "An Arcean envoy, Speaker Tenn, is on the way from Saturn to Earth in one unarmed ship. The Arceans have stayed out of this war, and Fleet would very much like that to change."],
	["env_brief_2", "hale", "ops_room", "Meet the envoy's ship in the belt and bring it the rest of the way to our starbase. Nothing touches that ship."],
	["env_brief_3", "hale", "ops_room", "Pirates are working that stretch again. Real ones, this time. Be ready."],
	["env_start", "brandt", "cockpit_terran", "An actual diplomat. I'll be on my best behaviour. Mostly."],
	["env_tenn_1", "tenn", "bridge_arcean", "Terran pilots. I am Speaker Tenn of the Arcean Union. Thank you for the escort. My people would like to know who we are dealing with."],
	["env_tenn_2", "tenn", "bridge_arcean", "Lead the way, Lantern. We will keep pace."],
	["env_corrigan_1", "corrigan", "cockpit_pirate", "Well now. An Arcean yacht and a few Terran fighters. Mags Corrigan, at your service. Hand over the yacht and you can all go home."],
	["env_corrigan_2", "corrigan", "cockpit_pirate", "No? Pity. Take it, lads, and try not to scratch the paint."],
	["env_weapons", "hale", "ops_room", "Those are real pirates, Lantern. Weapons free. Keep them off the envoy."],
	["env_tenn_hit", "tenn", "bridge_arcean", "We are taking fire. Our ship was not built for this."],
	["env_corrigan_flee", "corrigan", "cockpit_pirate", "That's enough. It's not worth the hull. Mags out."],
	["env_clear", "brandt", "cockpit_terran", "Scope's clear. A real pirate. I'd almost forgotten what one looked like."],
	["env_arrive", "tenn", "bridge_arcean", "We are at your starbase. Before I speak to your Admiral, I want to hear it from you, Lantern. Why should the Arceans join a war between Terra and Mars?"],
	["env_choice", "hale", "ops_room", "Your call, Lantern. You carry the Ceres log and the free port records. Show them to Tenn, and the Arceans learn everything we know. Or keep them close, and Intelligence pays for your discretion."],
	["env_share_1", "tenn", "bridge_arcean", "Drengin weapons on pirate ships. Drengin money behind the raids. This is not a border war. Kesh Varr means to own the whole system."],
	["env_share_2", "tenn", "bridge_arcean", "I will tell the Union what I have seen. Arcean ships will stand with Terra. Thank you, Lantern."],
	["env_keep_1", "tenn", "bridge_arcean", "I see. Terra keeps its secrets. Then the Arceans will keep to themselves, as before."],
	["env_keep_2", "okafor", "admiralty", "Intelligence thanks you for your discretion, Lantern. The bonus is in your account. Some things are better kept close."],
	["env_fail_lost", "hale", "ops_room", "The envoy's ship is gone. Abort, Lantern. This is a disaster."],
	["env_fail_breach", "hale", "ops_room", "You fired on the envoy! Cease fire, Lantern, now!"],
	# --- Operation 9: Silent Running
	["sr_brief_1", "hale", "ops_room", "The Drengin are building something in a shipyard over Mars. Something big. Intelligence calls it the Hammer, and nobody has seen it."],
	["sr_brief_2", "hale", "ops_room", "The yard sits inside a picket net. The pickets run passive arrays: they listen for drive heat and emissions. Run silent and they won't hear you."],
	["sr_brief_3", "hale", "ops_room", "If a picket gets too close, jam it. Get within scanning range of the Hammer, take your look, and get out. If they hear you, abort."],
	["sr_start", "brandt", "cockpit_terran", "Kettle is sitting this one out. Two ships make twice the noise. Cold coffee and a dark cockpit, all yours."],
	["sr_net", "hale", "ops_room", "You're at the edge of the net. Engines low, emissions off. Slow and quiet."],
	["sr_warn", "brandt", "cockpit_terran", "A picket's listening our way. Jam it, or get out of its way."],
	["sr_scan", "hale", "ops_room", "You're in range. Hold steady and let your sensors drink it in."],
	["sr_seen", "hale", "ops_room", "There it is. A dreadnought. A spinal gun, and armour like a moon. And it's almost finished."],
	["sr_seen2", "okafor", "admiralty", "If that ship launches, nothing we have can stop it. Get out of there with that scan, Lantern."],
	["sr_out", "hale", "ops_room", "Ease out the way you came. Nobody heard you come in. Nobody hears you leave."],
	["sr_done", "hale", "ops_room", "Clean. They never knew you were there. Now we know what we're up against."],
	["sr_fail_seen", "hale", "ops_room", "They've heard you! The net's lit up. Abort, Lantern, get out now!"],
	# --- Operation 10: Burn the Yards
	["by_brief_1", "hale", "ops_room", "Your scan gave us everything. The Hammer is days from launch, and once it's out, nothing in our fleet can stand against it."],
	["by_brief_2", "hale", "ops_room", "So we hit the yard. The Resolute leads a strike group straight at Mars. You and Brandt go in with them and burn that shipyard down."],
	["by_brief_3", "hale", "ops_room", "If the Hammer gets out of its cradle, don't fight it. Fall back. I need you alive for what comes next."],
	["by_start", "okafor", "admiralty", "Strike group, this is Okafor. Target the yard, nothing else. For Luna, and for everyone the raids took."],
	["by_start2", "brandt", "cockpit_terran", "Kettle with you. Let's go break something big."],
	["by_kesh", "kesh", "bridge_drengin", "Terrans at my yards. Brave, and foolish. The Hammer wakes soon, and it remembers faces."],
	["by_halfway", "hale", "ops_room", "The yard is burning. Keep at it."],
	["by_wave", "brandt", "cockpit_terran", "More of them, coming up off Mars!"],
	["by_minute", "hale", "ops_room", "One minute until the Hammer's cradle opens. Burn it now, Lantern!"],
	["by_burned", "hale", "ops_room", "The yard is going up! The Hammer's still in its cradle, and it's hurt!"],
	["by_burned2", "okafor", "admiralty", "Well done. That ship won't fly for months. Strike group, withdraw."],
	["by_launched", "hale", "ops_room", "The Hammer is out of its cradle. Break off, Lantern. All ships, fall back now!"],
	["by_launched2", "kesh", "bridge_drengin", "Now you see it. Run, little Terrans. The Hammer will find you."],
	["by_clear", "hale", "ops_room", "Get clear of Mars, Lantern. Don't look back."],
	["by_done_burned", "hale", "ops_room", "We hurt them today. Badly. Get some rest, Lantern. This isn't over."],
	["by_done_launched", "okafor", "admiralty", "We didn't stop it. But we know its name, and we know its face. Lantern Squadron, regroup at Earth. The war has changed."],
	# --- any time
	["breach_warn", "hale", "ops_room", "Lantern One, that's an Accord ship. Hold your fire."],
	["breach_warn_arcean", "hale", "ops_room", "Lantern One, that's an Arcean ship. They're staying out of this war. Hold your fire."],
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
