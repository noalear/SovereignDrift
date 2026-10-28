# Music status

All 18 new tracks are in the game (`assets/music/new/`, tagged in `src/autoload/Music.gd`).
new11-new18 were finished on 2026-10-28.

| # | Title | Tag | Genre | BPM / key | Script |
|---|---|---|---|---|---|
| 11 | Dead Reckoning | cruise | dub techno | 118, C# minor | `edm/new11_dubtechno.py` |
| 12 | Solar Wind Archive | cruise | Berlin-school sequencer | 108, B minor | `edm/new12_berlinschool.py` |
| 13 | Reactor Hall | tension | dark industrial techno | 130, B minor | `edm/new13_industrial_techno.py` |
| 14 | Cold Signal | tension | Detroit electro | 125, C minor | `edm/new14_electro.py` |
| 15 | Hull Breach | battle | hard industrial techno | 140, E-flat minor | `edm/new15_hard_industrial.py` |
| 16 | Kill Vector | battle | neurofunk drum & bass | 172, F# minor | `edm/new16_neurofunk.py` |
| 17 | Iron Armada | battle | orchestral industrial | 140, C minor | `edm/new17_hybrid.py` |
| 18 | Quiet Drive | cruise | ambient breaks | 92, D minor | `edm/new18_ambient_breaks.py` |

Each script's header lists the fragments of the original songs it uses (with bars) and its credits.
All were checked by measurement only (-16 LUFS, true peak under -4 dBTP, chroma per bar, no clicks);
Kyle's ear feedback is still to come.

## Waiting on Kyle
- **Black Shipyards v2** (`candidates/new06-black-shipyards-v2.ogg`, script `edm/new06_industrial.py` is
  already v2): hammer about 8 dB quieter, on the grid, blended with machinery stamps and hydraulics.
  Swap it into `assets/music/new/new06-black-shipyards.ogg` once Kyle approves.
- Song 8: dubstep or darkwave.

## Rendering
`python3 tools/music/edm/newNN_<genre>.py OUT.wav`, then master/encode as in the script's footer
(Ogg Vorbis q7, 48 kHz). Needs numpy scipy numba pedalboard pyloudnorm librosa soundfile pretty_midi;
some scripts need VSCO-2 CE (CC0): clone https://github.com/sgossner/VSCO-2-CE and set `VSCO_DIR`.
The process and lessons are in `BRIEF.md` and the `midi-genre-remix` skill.
