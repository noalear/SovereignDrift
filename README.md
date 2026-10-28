# Stellar Frontier — Remake

A ground-up, modern remake of Stellar Frontier (Stardock, 1997), built in
Godot 4.5. The gameplay is reimagined rather than ported rule for rule: mouse
and right-stick aimed turrets, full gamepad support, dynamic music that
follows combat intensity, and a camera that reacts to speed, warp and hits.
None of the original code or art is carried over; all ship, planet and effect
art is procedural, and the only assets brought across are the new music made
for the Enhanced Edition.

## Branches

| Branch | What it holds |
|---|---|
| `remake` | This remake (Godot 4.5, GDScript). |
| `master` (tag `original-1.0`) | The original edition, an exact copy of the 1997 game running on Godot 4. |
| `improved` | The Enhanced Edition: launcher, any resolution, Ogg music modes, LAN multiplayer in progress. |

## Running it

1. Install [Godot 4.5](https://godotengine.org/download) (standard build, not .NET).
2. Open `project.godot` in the editor and press F5, or run
   `godot --path .` from the repository root.

The project uses the GL Compatibility renderer, so it runs on older GPUs too.
Press H or F1 in game for the controls card.

### Controls (keyboard and mouse)

| Key | Action |
|---|---|
| W / S, A / D, Q / E | Thrust / reverse, turn, strafe |
| Shift, X, J | Boost, brake, quantum drive (warp) |
| Mouse | Aim turrets |
| Space / LMB, Ctrl / RMB | Torpedoes, secondary weapon |
| F, M, L, C | Seeking missiles, mine, laser, cloak |
| T / Tab, R, Shift+click | Next target, nearest enemy, target the ship under the cursor (or open the info card of a planet or moon: people, owner, defences, growth, income) |
| N, G, U | Next nav planet, autopilot to it, refit at your starbase |
| B, 1, 2 | Land on the world below / lift off; take colonists on; put them down (join, settle, invade). In flight 1 and 2 select weapon groups |
| K | Board a disabled target (shoot out its engines or reactor first) |
| V | Flight assist on/off (the thrusters hold your turn and stop a tumble) |
| F9, Alt+Enter, Esc | Next song, full screen, menu |

On a gamepad the left stick flies, the right stick aims, RT/LT fire.

### Ship systems, crews and capture

Every ship's engines, reactor, shield generators, sensors, quantum drive,
cloak and weapon mounts sit where they are drawn on the hull, and hits damage
the systems near the impact: shoot a ship in the stern to knock its engines
out, broadside it to take its shields. Damaged systems work at reduced
strength; offline ones stop until damage control restarts them, which goes
as fast as the crew allows. Hits also kill crew. A ship with its engines or
reactor out is dead in space, and you can board it (K): the tractor holds it
alongside while your crew fights its crew, and a ship taken from a computer
pilot flies with you under a prize crew. The balance numbers live at the top
of `src/ships/ShipSystems.gd` and `src/world/Capture.gd`;
`tools/capture_test.gd` reports how much hull is left when ships are
disabled, and `tools/battle_sim.gd` runs a whole round of computer pilots.

### Tutorial

**Tutorial (voiced, Terran)** on the title screen: Commander Hale of the
Terran Naval Academy talks a cadet through the whole game in a calm Sol (no
enemy fleets, no round clock, colonies hold their fire, nothing reaches the
career). Seven chapters, 46 lessons: the cockpit, flight, navigation and the
quantum drive, the map and info cards, targeting, weapon groups, disabling
and boarding a drone, cloaking, a fight with a rookie pirate, loot, the
starbase and shipyard, landing, colonists, cargo, the ground war tabs, the
ledger and job board, stations, the free port, invasions and how to win.
Every line is spoken and shown as subtitles, with the objective and its live
keys under it. Enter skips a lesson, Shift+Enter a chapter, Backspace repeats
the line. The lessons live in `src/ui/Tutorial.gd`, the words in
`src/ui/TutorialLines.gd`; `tools/tutorial_test.gd` plays it through.

## Voices

Spoken lines are made with [Kokoro-82M](https://github.com/hexgrad/kokoro),
a local text-to-speech model under Apache 2.0 (free to use in games, runs
offline, 54 voices), through `tools/voice/generate.py`. It reads the lines
from `src/ui/TutorialLines.gd` (speakers: voice, speed, clean or radio
effect), renders `assets/voice/<set>/<id>.ogg` at 48 kHz, compresses and
levels them to -17 LUFS, and the game ducks the music while a line plays
(Settings > Voice volume). See the script's header for setup; the same tool
can voice campaign characters later.

## Layout

```
project.godot          project settings and input map
src/Main.gd/.tscn      entry scene
src/autoload/          Settings, GameData (ship/module data), Music, Sfx
src/ships/             Ship, ShipArt, AIPilot, PlayerController
src/world/             World, Planet (+ planet.gdshader), Starfield, Fx, Powerup
src/weapons/           Missile
src/ui/                MainMenu, HUD
src/audio/             EngineSound (per-ship drive and booster loops)
tools/                 smoke test and screenshot scripts
tools/sfx/             sound-effect generator (Python: numpy, scipy, soundfile)
assets/music/          Ogg soundtrack (see below)
assets/sfx/            rendered sound effects, <name>_<NN>.ogg
assets/voice/          spoken lines (tools/voice/generate.py)
```

## Sound effects

Every effect is designed in code in `tools/sfx/generate.py` and rendered to
`assets/sfx/` (48 kHz Ogg, several variants each so repeats never sound the
same).  Modules have a voice per level: Engine I chemical rocket, II ion
drive, III fusion torch, IV antimatter; Booster I-III; Laser I-III (laser,
phaser, particle lance); Shield I-VI and the station shield; Armor I-III;
Quantum drive I-III.  To change a sound, edit its function and run
`python3 tools/sfx/generate.py <name>`, then open the editor (or run
`godot --headless --path . --import`) to re-import.
`tools/sfx_record.gd` records the in-game mix with Movie Maker for checks.

## Testing

```sh
# 100-second scripted battle, headless; fails on script errors
godot --headless --path . -s tools/smoke.gd

# Screenshots of the menu and game screens (needs a display or xvfb-run)
godot --path . --rendering-driver opengl3 -s tools/screenshot.gd
```

## Building

Export templates for Godot 4.5 must be installed. Then:

```sh
godot --headless --path . --export-release "Windows Desktop" build/windows/StellarFrontier.exe
```

The executable has the game data embedded. `build/` is ignored by git.

## Music

`assets/music/remix/`: genre remixes of the eight original songs.

| File | Song | Style |
|---|---|---|
| remix1-space-ambient.ogg | 1 | Space ambient |
| remix2-synthwave.ogg | 2 | Synthwave |
| remix3-liquid-dnb.ogg | 3 | Liquid drum & bass |
| remix4-chiptune.ogg | 4 | Chiptune |
| remix5-edm-trance.ogg | 5 | EDM / trance |
| remix6-symphonic-metal.ogg | 6 | Symphonic metal |
| remix7-epic-orchestral.ogg | 7 | Epic orchestral |
| remix8-dubstep.ogg | 8 | Dubstep |
| remix8-darkwave.ogg | 8 | Darkwave (alternative) |

`assets/music/new/`: eighteen new tracks built from unused fragments of the
original songs.

| File | Style |
|---|---|
| new01-graveyard-orbit.ogg | Trip-hop |
| new02-across-the-heliopause.ogg | Progressive house |
| new03-lagrange-drift.ogg | Atmospheric jungle |
| new04-tritone-run.ogg | Big beat |
| new05-nebula-caravan.ogg | Goa trance |
| new06-black-shipyards.ogg | Industrial |
| new07-rimward-patrol.ogg | Cinematic electronica |
| new08-long-haul-to-kepler-gate.ogg | Detroit techno |
| new09-far-sun-meridian.ogg | Jarre-style space synth |
| new10-lantern-orbit.ogg | Ambient techno |
| new11-dead-reckoning.ogg | Dub techno |
| new12-solar-wind-archive.ogg | Berlin-school sequencer |
| new13-reactor-hall.ogg | Industrial techno |
| new14-cold-signal.ogg | Detroit electro |
| new15-hull-breach.ogg | Hard industrial techno |
| new16-kill-vector.ogg | Neurofunk drum & bass |
| new17-iron-armada.ogg | Orchestral industrial |
| new18-quiet-drive.ogg | Ambient breaks |

All tracks are Ogg Vorbis at about -16 LUFS and end on a strong final chord
so they loop cleanly. `src/autoload/Music.gd` tags each one cruise, tension or
battle for the dynamic music. The scripts that generated them are in
`tools/music/edm/` (shared toolkit `synth.py`, one script per track; the
original MIDIs are in `tools/music/enhanced_mid/`). Some need the VSCO-2 CE
samples (CC0): clone https://github.com/sgossner/VSCO-2-CE and set `VSCO_DIR`.

Open music notes: new06 v2 (shipyard hammer subdued and locked to the groove)
is in `tools/music/candidates/` waiting for a listen; song 8 still needs a
pick between dubstep and darkwave.

## License

The music is derived from the original Stellar Frontier songs, so it falls
under the Stardock Shared Source Stellar Frontier License
(`stellar_frontier_shared_source_eula.txt`): non-commercial use only.
