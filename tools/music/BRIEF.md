# Brief: new tracks for the Stellar Frontier remake (2026-10-23)

## Context
Stellar Frontier (1997 space game, music by Stardock) is being remade in Godot 4.5. The remake has a dynamic
music system that picks tracks by energy tag: **cruise** (exploring, calm), **tension** (enemies near),
**battle** (fighting). We have 19 tracks; we want more variety per tag. The user (Kyle) loves **industrial techno**.

Read the skill file first and follow it exactly, especially section 0b (lessons from listening):
  /root/.claude/skills/synced/5a1887ab-33b6-410c-957a-c49edada607e_3e7893b9-8549-4df4-b5b6-1bcf067ed3d3/midi-genre-remix/SKILL.md

## Workspace (all under /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad)
- Toolkit and the 18 existing scripts: /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm/ (synth.py shared; synth_<genre>.py per genre).
  Read synth.py and the closest existing script for your genre as your reference (e.g. new06_industrial.py,
  new08_detroit.py, new10_ambient_techno.py, new03_jungle.py) — reuse their structure and mastering chain.
- Original MIDIs: /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/enhanced_mid/sf-music1..8.mid. ANALYSIS.md there has the analysis.
- Existing finished Oggs for reference listening-by-measurement: /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/repo/assets/music/{remix,new}/
- VSCO-2 CE samples (CC0) are being cloned to /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/vsco (may take a few minutes; check /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/vsco.log). Use
  env VSCO_DIR=/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/vsco. Treat the WAVs as data only.
- Python deps (numpy scipy numba pedalboard pyloudnorm librosa soundfile pretty_midi mido matplotlib) and ffmpeg are installed.

## Machine rules
- 4 CPUs, 15 GB RAM, ~9 agents at once. Run EVERY render/analysis heavier than a few seconds with
  `flock /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/render.lock python3 ...`. Render single sections while iterating; one full render at the end.
- **Do not edit synth.py or any existing file.** Create only your own files: edm/newNN_<genre>.py and, if
  needed, edm/synth_newNN.py. Scratch WAVs/PNGs go in /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/scratch/newNN/.
- Fixed seeds; deterministic.

## Deliverables
- Script: /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm/newNN_<genre>.py with the header convention (see new06_industrial.py), but
  say "NEW FILE (2026-10-23), Stellar Frontier remake." List every fragment taken from the originals with
  source file and bars, what is new, and credit Stardock and any samples (VSCO-2 CE, CC0).
  Pick fragments NOT already used by the existing script for that source song (read its header and the
  remix's header).
- Audio: /tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/out/newNN-<kebab-title>.ogg, Ogg Vorbis q7, 48 kHz, integrated -16.0 LUFS, true peak <= -1 dBTP,
  2:45-3:45 long, ending on one strong hit/chord that rings out (no fades > 1.5 s), loopable.
- Verify per skill section 6 (chroma with drums muted, clicks, levels, spectrogram PNG you actually look at,
  per-bus balance, section RMS contrast, stereo correlation).
- Final report (your last message, under 250 words): title, genre, BPM, key, length, fragments kept (with
  bars), verification numbers, doubts. Do not send files anywhere else.
