## Names -- what players see the game and its peoples called (Kyle,
## 2026-10-30).  The code keeps the old names (arcean, terran, drengin in
## ids, art folders, save keys and the protocol); every player-facing word
## comes from here, so a rename is one edit here plus a voice regen
## (tools/voice/generate.py) for the lines that say a name.
##
## Voiced and long text writes a name as a token, {terran}, {terrans},
## {arcean}, {arceans}, {drengin} or {game}, and goes through fill().
class_name Names
extends RefCounted

const GAME := "Sovereign Drift"
## The game this one remakes, for the credits (the Stardock license asks
## that its notices stay).
const ORIGINAL := "Stellar Frontier"

const ARCEAN := "Ephyri"         # jellyfish-like aquanoids
const TERRAN := "Solborn"        # humans, born under Sol
const DRENGIN := "Skethra"       # spider-like arachnoids
const PIRATE := "Pirate"

## One of a people, by race id.  Each new name is its own plural.
const RACE := [ARCEAN, TERRAN, DRENGIN, PIRATE]
const PLURAL := [ARCEAN, TERRAN, DRENGIN, "Pirates"]

## How the voice generator says a name, in Kokoro phonemes, where its own
## guess is off (it says "SKEE-thra"; we want "SKETH-ra").  Check a new
## name with tools/voice/generate.py --sample af_heart "The Skethra".
const SAY := {DRENGIN: "skˈɛθɹə"}

const TOKENS := {
	"game": GAME,
	"arcean": ARCEAN, "arceans": ARCEAN,
	"terran": TERRAN, "terrans": TERRAN,
	"drengin": DRENGIN,
}


## Text with its name tokens filled in.
static func fill(text: String) -> String:
	return text.format(TOKENS) if "{" in text else text
