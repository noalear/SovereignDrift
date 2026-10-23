# Music work in progress (paused 2026-10-23)

Kyle paused the music work on 2026-10-23 ("we've got enough for the moment"). This branch saves where it stopped.
Nothing here is in the game yet: `src/autoload/Music.gd` and `assets/music/` are unchanged from `remake`.

## Toolkit
- `edm/` holds the shared toolkit (`synth.py`) and the scripts for all existing remixes and new tracks, copied
  from the `improved` branch, plus the new drafts below. `enhanced_mid/` holds the original MIDIs.
- `BRIEF.md` is the brief the track drafts were written against (paths in it point at a session scratch dir:
  set `S` to your own work dir). The process is the `midi-genre-remix` skill.
- Renders need VSCO-2 CE (CC0) samples: `git clone --depth 1 https://github.com/sgossner/VSCO-2-CE` and set
  `VSCO_DIR`. Some drafts still hard-code a scratch path for it; switch those to `$VSCO_DIR`.
- `wip-notes/newNN/` holds each draft's MIDI dumps and analysis helpers.

## State of each track
| # | Title | Tag | Genre | State |
|---|---|---|---|---|
| 06 v2 | Black Shipyards | battle | industrial | **Done.** Hammer quieter (about 8 dB), on the grid, blended with machinery stamps and hydraulics; rest unchanged. Verified by measurement, -16 LUFS. Candidate in `candidates/`; not yet swapped into `assets/music/new/` because Kyle has not heard it. |
| 11 | Dead Reckoning | cruise | dub techno | Script drafted, one full render done, not yet verified or mastered to spec. |
| 12 | Solar Wind Archive | cruise | Berlin-school sequencer | Script drafted, test renders done, not verified. |
| 13 | Reactor Hall | tension | dark industrial techno | Script drafted, full render done (3:13), bus balance measured, not yet verified. |
| 14 | Cold Signal | tension | Detroit electro | Script drafted, one section rendered. |
| 15 | Hull Breach | battle | hard industrial techno | Script drafted, full render done, not verified. |
| 16 | Kill Vector | battle | neurofunk DnB | Script drafted, one section rendered; reese, stabs, snare and hats were far too low and were being rebalanced. |
| 17 | Iron Armada | battle | orchestral-industrial | Script drafted, one section rendered. |
| 18 | Quiet Drive | cruise | ambient breaks | Script drafted, one section rendered; first full render was interrupted. |

## To finish later
1. Per draft: full render, verify (skill section 6), encode to `assets/music/new/newNN-<title>.ogg`.
2. Add each finished track to `NEW_TRACKS` in `src/autoload/Music.gd` with its tag, and let Godot import it.
3. Swap in Black Shipyards v2 once Kyle has listened to it.
