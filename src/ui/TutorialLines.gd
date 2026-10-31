## TutorialLines -- what the tutorial's instructor says, word for word.
##
## A people or the game is named by a token ({terran}, {drengin}, ...),
## filled in from Names.  Each line is voiced: tools/voice/generate.py reads this file (through
## tools/voice/export_lines.gd) and renders assets/voice/<set>/<id>.ogg with
## the speaker's voice.  The subtitles show the same words, so the spoken
## text never names a key: the objective line under it shows the live keys.
## Change a line here, then run the generator again.
class_name TutorialLines
extends RefCounted

## Who speaks.  voice: a Kokoro voice; speed: its pace; fx: "clean" or
## "radio" (a comm-channel band-pass).  New campaign voices go here too.
const SPEAKERS := {
	"hale": {"name": "Commander Hale", "title": Names.TERRAN + " Naval Academy", "voice": "af_heart", "speed": 0.96, "fx": "clean",
		"color": Color(0.55, 0.8, 1.0)},
}

## [id, speaker, text]
const LINES := [
	# --- welcome and the cockpit
	["intro", "hale", "Welcome to the {terran} Naval Academy, cadet. I'm Commander Hale, and I'll be with you on comms for this flight. Nobody out here is shooting at us today, so take your time."],
	["intro2", "hale", "By the end of this flight you'll know how to fly, fight, capture ships, command a wing, move colonists and cargo, and run a war for the Sol system. Let's start with your cockpit."],
	["hud_status", "hale", "Bottom left are your shields, hull, energy and fuel. Shields soak up hits and recharge on their own. The hull does not, so protect it. The row of lights above shows your ship's systems."],
	["hud_weapons", "hale", "Beside them is your weapons panel, with your four weapon groups. Bottom right is the radar. Ships show as little darts pointing the way they're flying, in their empire's colour. {terran} blue is us, and enemies get a red ring."],
	["hud_top", "hale", "Along the top are your rank, score and credits, and how many planets each empire holds. Planets are what this war is about."],
	# --- flight school
	["thrust", "hale", "Let's fly. Hold thrust to push forward, and turn to steer. There's no friction in space, so when you let go, you keep drifting. Take her out a little way from the starbase."],
	["strafe", "hale", "Strafe thrusters slide you sideways without turning, and reverse thrust slows you down or backs you up. When you reverse or brake, you'll see the retro jets fire from the bow. Give them both a try."],
	["brake", "hale", "To stop, use the brake. It fires against your drift until you're holding still beside whatever world you're near."],
	["boost", "hale", "Boost gives you a burst of speed while you thrust. Like your engines, it runs on fuel, not energy, and it burns through the tank fast. Save it for a chase, or for getting out of one."],
	["assist", "hale", "Flight assist fires small thrusters to stop you spinning when you let go of the turn. Switch it off and you tumble freely, which some pilots prefer. Turn it off, then back on."],
	["beacon", "hale", "I've dropped a nav beacon ahead of you. Fly to it using what you've just learned. Follow the arrow if it's off screen."],
	["gravity", "hale", "Feel that pull? Every planet and moon has gravity, and if you coast near one you'll swing into orbit. Keep well clear of the Sun. Its heat will cook your hull."],
	# --- navigation
	["nav", "hale", "Space is big, so let the computer help. Cycle your navigation target until Luna, Earth's moon, is selected."],
	["autopilot", "hale", "Now engage the autopilot. It steers around planets and the Sun, and brakes for you when you arrive. Touch any flight control to take back manual control."],
	["warp", "hale", "For long trips, you have a warp drive. It throws you across the system at many times light speed, but turning or reversing drops you out. Point at open space, jump, then drop out again."],
	["map", "hale", "Open the system map. It shows every world and who owns it, and every ship your sensors can see. Names of worlds and stations are coloured by who holds them: green is ours, yellow a friendly empire, red the enemy, and grey belongs to nobody. Tap the key to leave it open, or hold it for a quick look."],
	["info", "hale", "Select any planet or moon to open its info card: who lives there, its defences, its resources, what it earns, and a picture of the world itself. Try it on Luna or Earth."],
	["zoom", "hale", "The mouse wheel zooms the camera, and the radar has its own zoom. Zoom out when you want to see trouble coming."],
	# --- combat
	["target", "hale", "Time for target practice. I've put an unmanned drone off your bow. Lock it as your target. You can cycle targets, pick the nearest enemy, or select the ship itself. A picture of your target sits under its readout, so you can see what you're up against, and the damage you've done."],
	["fire", "hale", "Your guns follow the mouse. Each one aims from its own mount, so their shots meet right where you point. Mass driver bolts fly a long way, so lead your target. Open fire on the drone. Shields absorb the first hits. Once they're down, you're hitting hull and systems."],
	["groups", "hale", "Your weapons are sorted into four groups by type. Guns are group one, lasers and disruptors two, launchers three, and mines four. Lasers and disruptors aim themselves at the ship you've locked, or at the mouse when nothing is locked. On your scout, the mass driver is group one, the seeker rack is group three, and the mine layer is group four. The number keys pick what your trigger fires, and alternate fire shoots a second group without switching. Pick group four, then back to group one."],
	["seeker", "hale", "Group three is your seeker rack. A seeker homes on engine heat and steers itself all the way in, so a ship can't outrun it or turn inside it. What stops one is point defence, or a lucky hit from a gun. A single seeker won't get past a point defence turret, so against bigger ships, fire them in pairs. Pick group three, put a seeker into the drone, then go back to your guns."],
	["disable", "hale", "Now the mark of a good captain: don't just blow ships apart. Every hit damages the systems nearest to it. Damaged engines push and turn slowly. Knock them out and the ship can't steer, brake or stop a spin. It just drifts the way it was going. Hit the drone in the stern to knock out its engines, and it's dead in space."],
	["drone_lost", "hale", "That one's scrap. Easy on the trigger, cadet. Here's another. Aim for the engines at the back."],
	["board", "hale", "Its engines are out. Come alongside, slow down, and board it. Your crew fights theirs, and if you win, the ship is yours. While you have a disabled ship targeted, your side holds its fire and leaves it to you. A captured prize follows you as a wingman."],
	["wing", "hale", "That ship flies on your wing now, and wingmen take your orders. Hold the orders key for the wheel, point at an order and let go, or tap the key and pick one. Form up keeps them tight on you, firing only when fired on. Cover me has them fight anything that comes close, and that's what they do until told otherwise. The bar by each wingman's name is its hull. A battered one can go home, land and get patched up. Order your wing to form up."],
	["wing_call", "hale", "You won't always have a prize. Call wingmen from the wheel, and our free pilots join you: one for a cadet, more as you rise in rank. Senior captains only fly for their equals. Your wing can also hold a spot, hunt at will, or go home for repairs, and Dismiss sends called pilots back to their duties. When you warp, your wing jumps with you if they have the energy, unless they're holding a spot or on an attack. Watch for enemy admirals. They lead wings of their own."],
	["shield_focus", "hale", "Your shield has a front half and a rear half, and a hit only drains the half it lands on. After a hit, that half waits a moment before it recharges. You can swing the charge forward when you attack, or aft when you run. Swing it forward, then back to balanced."],
	["shields_down", "hale", "You can also lower your shields. Lowered, they hold nothing, but they stop drawing power and your ship runs quieter. Raising them takes a couple of seconds, and they come back weak, so pick your moment. Lower them, then raise them again."],
	["stealth", "hale", "Every ship gives itself away. Your hull shows on radar, your engines and guns glow on infrared, and your own radar and shields broadcast for anyone listening. The bars by your radar show how loud you are. Enemy sensors lock on to whatever stands out. Silent running switches your radar off, holds your shields and turns the reactor down. You're much harder to find, but half blind too. Go silent, then come back up."],
	["pdc", "hale", "Missiles, torpedoes, drones and mines can all be shot down. I've had the yard crew fit a point defence cannon in your bays. It's a small gatling turret that fires on its own at anything coming for you. A launcher off your bow will send a few missiles your way. Just watch the turret work. Its stream of fire shows up on heat sensors, so you can switch it off when you want to stay hidden. After that, switch it back on."],
	["jammer", "hale", "Ready for a live opponent? A rookie pirate in a scout is coming in, and this one shoots back. Your scout carries a jammer. Lock the pirate as your target and switch the jammer on. Its sensors lose most of their range, so it struggles to keep a lock on you. Jamming burns energy and runs hot, so keep an eye on your meters."],
	["wing_attack", "hale", "Before you engage, put your wing to work. Lock the pirate as your target and order the attack. Every wingman goes for whatever you've locked, a ship or an enemy colony's guns, and comes back to cover you once it's down."],
	["spar", "hale", "Now finish the fight. Destroy the pirate, or disable and board it. If you lose your ship, you'll relaunch at the starbase."],
	["spar_died", "hale", "You'll relaunch from the starbase in a moment. Happens to all of us. Out in the war it costs you, though: half the credits you carry spill with the wreck, so fly back for them if you can. Get back out there."],
	["loot", "hale", "Every wreck spills a credit chip and one of the parts it carried, and now and then a blueprint for enemy technology. Fly over the pickups to scoop them up."],
	# --- the starbase
	["dock", "hale", "Head back to the starbase at Earth. Near a friendly starbase or colony, your fuel, crew and hull are topped up, and your launchers and mine layer are reloaded. Follow the green arrow."],
	["shipyard", "hale", "Slow down and open the shipyard. This is where you change hulls, fit engines, shields and weapons, and set your weapon groups. Better hulls open up as you rise in rank."],
	["armor", "hale", "Your hull takes one coat of armour. Plate is solid all round, but heavy. Ablative burns away under fire, Reflective turns lasers, Reactive breaks the biggest hits, a Shroud hides you from radar, and Bulkheads shelter your crew and systems. Reflective is {arcean} work and Reactive is {drengin}, so those take a blueprint from their wrecks."],
	["shield_types", "hale", "Shields come in families too. Standard shields are the steady all rounder. Barriers are big and slow to recharge. Flickers are small and recharge in a blink, but let some hits slip through. The {drengin} fly Deflectors. They bounce gun rounds and laser beams off like a pool shot, and the shot is theirs from then on, so a careless burst can come right back at you. Missiles, torpedoes and mines still get through. Every shield draws on your reactor while it's up, and a Deflector draws the most."],
	["shipyard_tour", "hale", "Pick a mount or a bay, choose a part type, then a part. Nothing is paid until you accept the order. Weapons can swap mounts, drag a turret or scroll over it to turn it, and boosters, shields and point defence can move around the hull, all for free. Anything you take off goes to your hold. Wingmen flying close to the yard get tabs along the top, so you can outfit their ships too. A pilot you called pays from their own purse first and your credits cover the rest, and a prize is yours to pay for."],
	["base_cargo", "hale", "The starbase also sells station kits and takes orders for new ships. Open the cargo panel here to see them. Kits and orders unlock at Lieutenant Commander."],
	# --- worlds, colonists and cargo
	["land", "hale", "Now let's go down to Earth. Get low over the planet, slow down, and land."],
	["colonists", "hale", "On your own worlds you refuel and repair, and you can take colonists aboard. Your cabin is small, but every million counts. Take some on."],
	["cargo", "hale", "Every world produces resources: metals, ice, fuel, food and exotics, and refined alloys. Open your cargo hold and load what Luna is short of."],
	["ground_tabs", "hale", "The other tabs run the ground war. Recruit troops at the barracks, and raise planetary shields, gun batteries, farms and industry. Have a look at them."],
	["to_luna", "hale", "Lift off and fly to Luna. Use the autopilot if you like, then land there."],
	["unload", "hale", "Put your colonists down to grow the colony, and unload your cargo. Goods delivered where they're short pay you credits."],
	["ledger", "hale", "All of this feeds your empire. Open the empire ledger to see every world's stockpile, and the job board, where our governor posts paid hauling and escort work."],
	# --- the empire, and how to win
	["haulers", "hale", "You're not alone out here. {terran} supply ships haul goods between our worlds, and trade ships sell our surplus. Escort ours, and hunt the enemy's. A starving colony can't build defences."],
	["stations", "hale", "As you rise in rank you can buy station kits: mining stations, gas skimmers, refineries, supply depots, sensor posts, defence platforms, trade stations and shipyards. Haul a kit to a site, deploy it, then deliver the materials to finish it."],
	["station_capture", "hale", "Enemy stations can be taken, just like ships. Shoot their shields down and break the hull to under half, then come alongside and board. Station crews fight hard from behind their bulkheads, so bring a full crew, or marines. Take one and it works for us, hold and all. Home starbases and the free port can't be boarded. A home starbase defends itself with turrets that pick their own targets, so come with a fleet. Destroy one, though, and its empire needs a full minute to rebuild it, and none of its pilots can launch until it's back. That goes for ours too."],
	["freeport", "hale", "At Jupiter's leading Trojan point sits the free port, a neutral market every empire can use. Buy where it's cheap, and sell where it's dear."],
	["invasion", "hale", "To take an enemy world, first silence its guns. Home worlds also sit under a planetary shield, and nothing lands until that shield is down. Then land troops. Armour breaks garrisons, and commandos sabotage from the ground."],
	["invasion2", "hale", "Drop ships carry whole armies. Colonists alone can only take a world with no defenders, and only twenty to one. The {drengin} hold Mars and Venus. The {arceans} hold Titan and Jupiter's moons."],
	["strategy", "hale", "So here's how to win. Early on, carry colonists and goods to grow your worlds, and settle empty moons. Defend the worlds that matter. Pick off enemy supply ships to starve them, and disable rather than destroy, so you keep the prizes."],
	["strategy2", "hale", "When an enemy world is weak, knock out its shield and guns and send in the troops. In a Conquest round, the empire holding the most planets when time runs out wins, or the first to hold every colony for forty five seconds."],
	["modes", "hale", "From the main menu you can fly the story, The Frontier War, where you'll hear from me again. Single Player has the rest: a campaign that saves as it goes, a Conquest round, Survival, where you hold your starbase against waves of raiders, and Skirmish, two empires four a side in ready built ships. Online, host any of these for friends, in co-op on your side or competitive against you, or a free for all duel. The story is co-op only, and the enemy brings more ships for each of you. Practice has duels and free flight, off the record. Pirates are rare, but take them seriously."],
	["outro", "hale", "Last thing. Help shows every control, Settings lets you change any key, and the menu pauses the game. That's everything, cadet. The system is yours. Fly safe."],
	# --- short replies when an objective is done
	["good_1", "hale", "Good."],
	["good_2", "hale", "Nicely done."],
	["good_3", "hale", "That's it."],
	["good_4", "hale", "Well flown."],
	["good_5", "hale", "Perfect."],
	["good_6", "hale", "Exactly right."],
]

const SET := "tutorial"


static func text(id: String) -> String:
	for row in LINES:
		if row[0] == id:
			return Names.fill(row[2])
	return ""


static func speaker(id: String) -> Dictionary:
	for row in LINES:
		if row[0] == id:
			return SPEAKERS.get(row[1], {})
	return {}


static func audio_path(id: String) -> String:
	return "res://assets/voice/%s/%s.ogg" % [SET, id]
