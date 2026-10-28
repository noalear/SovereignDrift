<!-- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition. Analysis of the original game music MIDI files (Stardock's music, 1997) made before re-arranging them; see rearrange.py. -->

# Stellar Frontier music: analysis of the original MIDI files

NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition. This is an analysis of the eight
game MIDI files (`mid/sf-music1..8.mid`, Stardock's music from the 1997 game). It
was written before re-arranging them with `tools/music/rearrange.py`. The tables
further down come from `rearrange.py --analyze`. The notes directly below are a
hand-written summary.

## Findings common to all eight songs

- **Source format.** These are DirectMusic-style exports at 384 ticks per beat, in 4/4,
  with one tempo per song and 21 tracks. Channels 1-9 carry the instruments and channel 10
  the drums. Tracks 16-20 are overflow tracks that put more notes on channels 1-9.
  Channels 11-16 are never used for notes, so the Enhanced Edition puts its added layers there.
- **Program changes in mid-song.** One channel often plays 3-10 instruments in turn
  (for example, song 7 channel 6 goes strings, brass, bassoon, piccolo, trumpet, oboe,
  trombone, clarinet, synth brass, then orchestra hit). Every program change is
  repeated at each 4-bar segment boundary (tick 6144k+1). Because of this, roles and
  instrument swaps are decided per (channel, program) segment, not per channel.
- **Timing is machine-exact.** Every note starts 1 tick after a grid line and ends
  1 tick before the next one. All onsets sit on a 16th, 8th-triplet (song 2) or
  32nd/16th-triplet grid, the last of these used for timpani, snare and kick rolls. No
  onset is loose, so quantising is a no-op. Swing exists only as written triplets.
- **Velocities are coarse.** They come in a few fixed levels (127/114/102/91/80/69/58...).
  Many parts, basses especially, play at a flat 127 throughout. Phrasing comes only
  from CC11 expression ramps, which appear in about a third of the parts. This is the
  main reason the faithful renders sound mechanical.
- **Pan.** The files set pan to 75 on every channel. Only a few parts move it, for
  example the auto-panning taiko in song 1 and the square-lead arpeggio in song 4.
- **Harmony.** The writing is modal and chromatic on purpose. It uses tritone ostinati
  and polychords over drones (songs 4 and 8), Phrygian bII-I planing (C to Db, song 7),
  minor b6-5 laments (songs 1 and 6), V7b9 colour (song 8), and E-major/Eb-minor
  alternation (song 6). Most minor-2nd clashes are composed. They are doubled in other
  parts, consonant with the bass, resolve by semitone, or recur as ostinati. The
  real defects are mechanical: notes exactly doubled on the same channel and pitch
  (DirectMusic overflow tracks), overlapping same-pitch notes, and one figure where a
  C-chord pattern was carried onto a Db chord (song 7, piccolo G5 against the trumpet's
  Ab5, bars 18 and 62).
- **Dated patches.** Orchestra Hit appears in songs 5-8, Synth Brass 1/2 in songs 5-8,
  Whistle in songs 1, 3, 4 and 6, and Honky-tonk Piano in song 4. Under GeneralUser GS the
  orchestra-hit sample also carries a strong low-frequency thump, with its spectral peak
  at 10-20 Hz, and this inflates the low end of the faithful renders.

## Per-song summary

| song | tempo / length | character, keys | notable for the re-arrangement |
|---|---|---|---|
| 1 | 140 BPM, 80 bars | slow orchestral; F minor, C minor and Db areas over choir/tuba drones (C2 / C#2) | taiko ostinato with auto-pan; harp 8ths; whistle melody; almost no kit drums (5 crashes, 2 chinas) |
| 2 | 135 BPM, 60 bars | Db major (C#), Ab minor middle (bars 25-40) | 8th-triplet feel (17.6% of onsets); military snare (386 hits) plus cymbals; synth-brass lead with CC11 swells; fretless bass at a flat 127 |
| 3 | 220 BPM, 84 bars | A minor, with E major / E minor sections | harp 8th ostinato (672 notes at a flat 91); fretless pedal bass; kick-2/side-stick/snare groove |
| 4 | 145 BPM, 68 bars | C minor / C over tritone drones (C, F#) | square-lead arpeggio (pan automated); brass G-Ab ostinato; honky-tonk piano tritone figure G-C#-F#-C; Voice Oohs pinging C7 16ths |
| 5 | 180 BPM, 84.5 bars | C# minor, F# minor middle | the only full kit groove (236 kicks, 672 closed hats); fantasia-pad 8ths; orchestra hits and synth brass |
| 6 | 180 BPM, 106.5 bars | Eb minor / B major / E major, chromatic | whistle 8th ostinato Eb-Bb-Eb-E (843 notes); orchestra hits on 4 channels; 874 snares; 15 duplicated notes |
| 7 | 155 BPM, 75.5 bars | C, with Fm/Bbm and C minor sections; C to Db planing | 16th fretless and trombone ostinati; snare-and-timbale march with no kick; the one real pitch error (piccolo) |
| 8 | 200 BPM, 92 bars | B minor (Phrygian C over a B pedal), F#/C tritone passage at bars 25-40 | tremolo-strings and choir pedals; orchestra-hit motif B-C-F#-A; 725+221 snares, cabasa |

## Generated detail (`rearrange.py --analyze`)

Roles: *bass*, *sustain* (long-note harmony), *melodic*, *perc* (pitched percussion
such as taiko and timpani, excluded from harmonic analysis), *drums* (channel 10).
Keys use Krumhansl-Kessler profiles over duration-weighted pitch classes per 8 bars,
and the number in brackets is the correlation, so a low value means chromatic or
ambiguous writing. Section boundaries are 4-bar blocks where the music arrives busier or
re-orchestrated; these drive the crescendos.

## sf-music1

- Tempo 140 BPM, 4/4, 80.0 bars, 137.1 s (tpb 384); 1426 notes; stuck notes: 0
- Timing: 97.9% of onsets on the 16th grid, 0.0% on 8th triplets, 2.1% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | bass | Choir Aahs (1) | 28 | C2-G2 | 127-127 (1 levels) | 8.00 beats |
| 2 | melodic | Orchestral Harp (9) | 512 | F3-Ab4 | 114-114 (1 levels) | 0.50 beats |
| 3 | bass | Tuba (1) | 36 | C2-G2 | 114-127 (2 levels) | 8.00 beats |
| 4 | melodic/sustain | String Ensemble 2 (1); Oboe (30); Orchestral Harp (61) | 97 | G3-C6 | 69-114 (5 levels) | 0.50 beats |
| 5 | sustain/perc | Timpani (8); Trombone (12); String Ensemble 2 (21); Choir Aahs (57) | 56 | G2-F5 | 69-127 (6 levels) | 2.00 beats |
| 6 | sustain | Trombone (12); String Ensemble 2 (22); Synth Strings 1 (73) | 33 | G3-G6 | 69-127 (6 levels) | 8.00 beats |
| 7 | sustain/perc | Flute (6); String Ensemble 2 (23); Timpani (32); French Horn (48); Oboe (67) | 73 | C#3-C#6 | 58-127 (7 levels) | 1.00 beats |
| 8 | perc | Taiko Drum (1) | 508 | C2-C2 | 58-127 (6 levels) | 0.50 beats |
| 9 | sustain/melodic/perc | Choir Aahs (3); Whistle (9); Synth Strings 1 (40); Timpani (80) | 76 | G2-G5 | 80-127 (5 levels) | 2.00 beats |
| 10 | drums | kit (9) | 7 | Crash 1 x5, China x2 | 80-102 (3 levels) | 4.00 beats |

- Key estimate per 8 bars: 1-8 Ab (0.45), 9-16 Fm (0.56), 17-24 Fm (0.59), 25-32 Cm (0.64), 33-40 C# (0.46), 41-48 F (0.62), 49-56 F (0.62), 57-64 Cm (0.45), 65-72 Cm (0.47), 73-80 Cm (0.64), 81-81 C (0.00)
- Section boundaries detected at bars: 5, 9, 21, 29, 65, 73, 77
- Same-channel same-pitch overlaps/duplicates: 0

## sf-music2

- Tempo 135 BPM, 4/4, 60.0 bars, 106.7 s (tpb 384); 1603 notes; stuck notes: 0
- Timing: 81.0% of onsets on the 16th grid, 17.6% on 8th triplets, 1.4% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | perc/melodic/sustain | Synth Brass 1 (6); Timpani (40) | 74 | Ab2-Ab5 | 80-127 (4 levels) | 0.33 beats |
| 2 | melodic/sustain | Synth Brass 1 (6); Trombone (14); Orchestral Harp (18); Oboe (22); String Ensemble 1 (25) | 80 | Ab3-C#6 | 80-127 (4 levels) | 1.00 beats |
| 3 | bass | Fretless Bass (1) | 360 | A1-Ab2 | 127-127 (1 levels) | 0.50 beats |
| 4 | melodic | French Horn (1); Orchestral Harp (41) | 392 | F#3-C#5 | 69-102 (4 levels) | 0.50 beats |
| 5 | sustain | String Ensemble 1 (1); Choir Aahs (48) | 29 | C#4-Eb6 | 80-127 (5 levels) | 8.00 beats |
| 6 | sustain | String Ensemble 2 (1) | 31 | C#4-Ab4 | 102-114 (2 levels) | 8.00 beats |
| 7 | sustain | String Ensemble 2 (1) | 31 | C#4-B4 | 102-114 (2 levels) | 8.00 beats |
| 8 | sustain/melodic | Flute (2); Oboe (5); Choir Aahs (21); Clarinet (25) | 64 | Ab3-C#5 | 80-127 (5 levels) | 2.00 beats |
| 9 | perc/sustain | Timpani (1); Clarinet (25) | 112 | Ab2-Ab4 | 80-127 (4 levels) | 2.00 beats |
| 10 | drums | kit (1) | 430 | Snare 1 x386, Crash 1 x13, China x26, Splash x1, Crash 2 x4 | 69-114 (5 levels) | 0.50 beats |

- Key estimate per 8 bars: 1-8 C# (0.88), 9-16 C# (0.89), 17-24 C# (0.88), 25-32 Abm (0.61), 33-40 Abm (0.61), 41-48 C# (0.74), 49-56 C# (0.70), 57-61 C# (0.80)
- Section boundaries detected at bars: 5, 17, 25
- Same-channel same-pitch overlaps/duplicates: 1

## sf-music3

- Tempo 220 BPM, 4/4, 84.0 bars, 91.6 s (tpb 384); 1514 notes; stuck notes: 0
- Timing: 99.5% of onsets on the 16th grid, 0.0% on 8th triplets, 0.5% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | melodic/sustain | Flute (19); Choir Aahs (29); Whistle (51) | 37 | A3-F5 | 91-102 (2 levels) | 2.00 beats |
| 2 | sustain | Flute (9); Choir Aahs (49) | 97 | D4-A5 | 91-127 (4 levels) | 2.00 beats |
| 3 | bass | Fretless Bass (1) | 128 | D1-F2 | 127-127 (1 levels) | 2.00 beats |
| 4 | melodic | Orchestral Harp (1) | 672 | A3-F4 | 91-91 (1 levels) | 0.50 beats |
| 5 | melodic | String Ensemble 1 (1) | 137 | B4-E6 | 38-127 (8 levels) | 1.00 beats |
| 6 | sustain | String Ensemble 1 (1); French Horn (26); String Ensemble 2 (49) | 26 | C3-F5 | 69-114 (5 levels) | 4.00 beats |
| 7 | sustain | Choir Aahs (33); String Ensemble 2 (49) | 20 | A3-D5 | 80-127 (4 levels) | 6.00 beats |
| 10 | drums | kit (1) | 397 | Kick 2 x84, Side Stick x105, Snare 1 x187, Crash 1 x2, Ride 1 x6, China x9, Open Triangle x4 | 58-127 (6 levels) | 2.00 beats |

- Key estimate per 8 bars: 1-8 Am (0.88), 9-16 Am (0.83), 17-24 E (0.64), 25-32 Am (0.77), 33-40 Em (0.55), 41-48 Am (0.93), 49-56 Am (0.81), 57-64 Am (0.79), 65-72 Am (0.81), 73-80 A (0.78), 81-85 Am (0.84)
- Section boundaries detected at bars: 5, 17, 33, 45, 49, 65, 81
- Same-channel same-pitch overlaps/duplicates: 0

## sf-music4

- Tempo 145 BPM, 4/4, 68.0 bars, 112.6 s (tpb 384); 1795 notes; stuck notes: 0
- Timing: 100.0% of onsets on the 16th grid, 0.0% on 8th triplets, 0.0% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | melodic | Square Lead (1) | 242 | C5-G5 | 28-91 (7 levels) | 0.75 beats |
| 2 | bass | Choir Aahs (1) | 18 | Bb1-F#2 | 127-127 (1 levels) | 16.00 beats |
| 3 | melodic | Brass Section (1) | 544 | G3-A3 | 69-127 (6 levels) | 0.50 beats |
| 4 | sustain | String Ensemble 2 (1) | 21 | G5-C#6 | 102-102 (1 levels) | 16.00 beats |
| 5 | melodic | Voice Oohs (5) | 284 | C7-C7 | 48-102 (5 levels) | 0.25 beats |
| 6 | bass | Fretless Bass (44) | 65 | F#1-C#3 | 114-127 (2 levels) | 1.00 beats |
| 7 | melodic/sustain/perc | Taiko Drum (12); Honky-tonk Piano (15); French Horn (17) | 216 | C3-C#5 | 58-127 (7 levels) | 0.50 beats |
| 8 | melodic/perc | Taiko Drum (16); Honky-tonk Piano (41) | 165 | C3-A4 | 69-127 (6 levels) | 0.50 beats |
| 9 | melodic/sustain | String Ensemble 2 (26); Whistle (52) | 21 | G3-Ab5 | 102-127 (2 levels) | 4.00 beats |
| 10 | drums | kit (13) | 219 | Snare 1 x90, Crash 1 x4, China x22, Splash x1, Cabasa x90, Claves x12 | 28-127 (10 levels) | 0.50 beats |

- Key estimate per 8 bars: 1-8 Cm (0.75), 9-16 Cm (0.65), 17-24 C (0.77), 25-32 Cm (0.78), 33-40 C (0.78), 41-48 Cm (0.74), 49-56 Cm (0.71), 57-64 C (0.47), 65-69 Cm (0.75)
- Section boundaries detected at bars: 5, 13, 37, 41
- Same-channel same-pitch overlaps/duplicates: 0

## sf-music5

- Tempo 180 BPM, 4/4, 84.5 bars, 112.7 s (tpb 384); 2608 notes; stuck notes: 0
- Timing: 99.5% of onsets on the 16th grid, 0.0% on 8th triplets, 0.5% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | bass | Fretless Bass (1) | 32 | C#2-Ab2 | 127-127 (1 levels) | 8.00 beats |
| 2 | bass | Choir Aahs (1) | 34 | C#2-Ab2 | 127-127 (1 levels) | 8.00 beats |
| 3 | melodic | Fantasia Pad (1) | 408 | C#4-B4 | 91-91 (1 levels) | 0.50 beats |
| 4 | melodic/sustain | String Ensemble 1 (1); Choir Aahs (17); Fantasia Pad (61) | 113 | C#4-C#6 | 91-114 (3 levels) | 0.50 beats |
| 5 | melodic | String Ensemble 1 (1); Orchestra Hit (18); Synth Brass 1 (42); Electric Piano 1 (57) | 159 | C#4-C#6 | 80-127 (5 levels) | 0.50 beats |
| 6 | melodic/sustain | Orchestra Hit (1); Fantasia Pad (21); Synth Brass 1 (42); Electric Piano 1 (59); Choir Aahs (65) | 93 | C#4-Ab5 | 80-127 (5 levels) | 1.00 beats |
| 7 | melodic | Orchestra Hit (7); Synth Brass 1 (20); Trumpet (43) | 96 | E4-Ab5 | 91-127 (4 levels) | 0.50 beats |
| 8 | melodic | Synth Brass 1 (9); String Ensemble 1 (17); Trumpet (43) | 93 | B3-C#6 | 91-127 (4 levels) | 0.50 beats |
| 9 | melodic/perc | Taiko Drum (16); String Ensemble 1 (17) | 62 | Ab3-Ab5 | 91-127 (3 levels) | 0.50 beats |
| 10 | drums | kit (1) | 1518 | Kick 1 x236, Side Stick x6, Snare 2 x279, Low Floor Tom x11, Closed HH x672, High Floor Tom x20, Low Tom x12, Open HH x23, Low-Mid Tom x20, Hi-Mid Tom x1, Crash 1 x45, High Tom x49, Splash x33, Crash 2 x39, Ride 2 x2, Open Hi Conga x28, Low Conga x27, Claves x15 | 58-127 (6 levels) | 0.50 beats |

- Key estimate per 8 bars: 1-8 C#m (0.73), 9-16 C#m (0.78), 17-24 F#m (0.65), 25-32 F#m (0.66), 33-40 C# (0.75), 41-48 C# (0.50), 49-56 C#m (0.72), 57-64 C#m (0.86), 65-72 C#m (0.89), 73-80 D (0.64), 81-85 C#m (0.70)
- Section boundaries detected at bars: 9, 33, 41, 49, 57
- Same-channel same-pitch overlaps/duplicates: 0

## sf-music6

- Tempo 180 BPM, 4/4, 106.5 bars, 142.0 s (tpb 384); 3621 notes; stuck notes: 0
- Timing: 98.6% of onsets on the 16th grid, 0.0% on 8th triplets, 1.4% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | melodic | Whistle (1) | 843 | Eb4-F#5 | 80-127 (5 levels) | 0.50 beats |
| 2 | sustain/melodic | String Ensemble 2 (1); String Ensemble 1 (40) | 50 | E5-B6 | 80-127 (4 levels) | 4.00 beats |
| 3 | bass | Tuba (1); Fretless Bass (9) | 242 | Ab1-Ab3 | 114-127 (2 levels) | 1.00 beats |
| 4 | melodic/bass/perc/sustain | Orchestra Hit (3); Piccolo (34); Tuba (41); French Horn (65); Taiko Drum (81) | 129 | Bb1-Eb6 | 80-127 (5 levels) | 1.00 beats |
| 5 | melodic/sustain/bass | Fretless Bass (1); String Ensemble 1 (9); Trombone (89) | 345 | Eb2-Ab6 | 102-127 (3 levels) | 1.00 beats |
| 6 | sustain/perc/melodic | Orchestra Hit (1); Choir Aahs (9); Taiko Drum (41) | 110 | Eb3-C#6 | 58-127 (7 levels) | 2.00 beats |
| 7 | melodic/sustain | Orchestra Hit (1); Trombone (23) | 153 | E3-Eb6 | 69-127 (6 levels) | 1.00 beats |
| 8 | melodic/sustain | Synth Brass 2 (1); Orchestra Hit (19); Trombone (23); Synth Brass 1 (41); String Ensemble 1 (49); French Horn (61) | 139 | Eb3-F#6 | 69-127 (6 levels) | 1.00 beats |
| 9 | melodic/perc/sustain | Synth Brass 2 (1); Orchestra Hit (25); Timpani (40); String Ensemble 1 (60); Orchestral Harp (71); Piccolo (81); Taiko Drum (81); Trombone (99) | 200 | B2-Bb6 | 80-127 (5 levels) | 0.50 beats |
| 10 | drums | kit (1) | 1410 | Kick 1 x134, Snare 1 x874, Snare 2 x183, High Floor Tom x3, Low Tom x11, Open HH x2, Low-Mid Tom x4, Hi-Mid Tom x5, Crash 1 x46, High Tom x29, China x37, Splash x10, Crash 2 x14, High Timbale x10, Low Timbale x8, Cabasa x32, Claves x8 | 48-127 (8 levels) | 0.50 beats |

- Key estimate per 8 bars: 1-8 Eb (0.77), 9-16 Ebm (0.58), 17-24 B (0.61), 25-32 B (0.62), 33-40 B (0.62), 41-48 Ebm (0.76), 49-56 Ebm (0.68), 57-64 E (0.62), 65-72 E (0.57), 73-80 B (0.57), 81-88 B (0.76), 89-96 Ebm (0.73), 97-104 B (0.63), 105-107 E (0.81)
- Section boundaries detected at bars: 9, 25, 41, 61, 81, 89, 101
- Same-channel same-pitch overlaps/duplicates: 15

## sf-music7

- Tempo 155 BPM, 4/4, 75.5 bars, 116.9 s (tpb 384); 3431 notes; stuck notes: 0
- Timing: 97.8% of onsets on the 16th grid, 0.0% on 8th triplets, 2.2% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | bass | Fretless Bass (1) | 546 | G1-Ab2 | 127-127 (1 levels) | 0.25 beats |
| 2 | melodic/perc/sustain | Synth Brass 1 (1); String Ensemble 1 (5); Synth Brass 2 (13); Taiko Drum (20); Bassoon (33) | 278 | C3-C6 | 69-127 (6 levels) | 0.50 beats |
| 3 | melodic/sustain | Brass Section (1); French Horn (5); String Ensemble 1 (63); Synth Brass 1 (71) | 509 | C3-G5 | 127-127 (1 levels) | 0.25 beats |
| 4 | melodic/perc/sustain | French Horn (1); Synth Brass 1 (5); Timpani (12); Brass Section (13); String Ensemble 1 (17); Trombone (63) | 239 | C3-C#6 | 80-127 (5 levels) | 0.50 beats |
| 5 | melodic/sustain/perc | Piccolo (2); French Horn (13); String Ensemble 1 (19); Taiko Drum (63) | 104 | G3-G6 | 80-127 (5 levels) | 1.50 beats |
| 6 | melodic/sustain | String Ensemble 1 (1); Brass Section (5); Bassoon (7); Piccolo (16); Trumpet (17); Oboe (19); Trombone (35); Clarinet (53); Synth Brass 1 (63); Orchestra Hit (72) | 131 | C3-C#6 | 69-127 (5 levels) | 1.50 beats |
| 7 | melodic/perc/sustain | Timpani (1); Trombone (5); Choir Aahs (39); Brass Section (63); Trumpet (67) | 471 | C#2-C6 | 58-127 (5 levels) | 0.25 beats |
| 8 | melodic/sustain | Trumpet (3); String Ensemble 1 (19); Clarinet (27); Orchestra Hit (35); Trombone (47); French Horn (63) | 192 | C3-C6 | 69-127 (6 levels) | 0.50 beats |
| 9 | melodic/sustain | Orchestra Hit (2); Trumpet (5) | 83 | F3-F5 | 80-127 (5 levels) | 0.25 beats |
| 10 | drums | kit (1) | 878 | Snare 1 x700, High Floor Tom x2, Low Tom x12, Open HH x6, Low-Mid Tom x6, Hi-Mid Tom x14, Crash 1 x43, High Tom x8, China x3, Crash 2 x41, High Timbale x3, Low Timbale x40 | 69-127 (6 levels) | 0.25 beats |

- Key estimate per 8 bars: 1-8 C (0.77), 9-16 C (0.73), 17-24 C (0.75), 25-32 C (0.82), 33-40 Fm (0.71), 41-48 Bbm (0.47), 49-56 C (0.77), 57-64 Cm (0.61), 65-72 Cm (0.84), 73-76 C (0.79)
- Section boundaries detected at bars: 5, 9, 13, 21, 33, 53, 57, 65, 69
- Same-channel same-pitch overlaps/duplicates: 10

## sf-music8

- Tempo 200 BPM, 4/4, 92.0 bars, 110.4 s (tpb 384); 3488 notes; stuck notes: 0
- Timing: 100.0% of onsets on the 16th grid, 0.0% on 8th triplets, 0.0% on 32nds / 16th triplets (rolls); 0 loose onsets (> 2 ticks off every grid). Notes start 1 tick after the grid and end 1 tick before the next grid line (DirectMusic export), so there is nothing to quantise.

| ch | role | programs (first use bar) | notes | range | velocities | median len |
|---|---|---|---|---|---|---|
| 1 | melodic/sustain | Contrabass (16); Synth Brass 2 (20); Synth Brass 1 (21); String Ensemble 1 (81); Choir Aahs (86) | 183 | F#3-Bb5 | 80-127 (4 levels) | 0.50 beats |
| 2 | melodic/perc/sustain | Taiko Drum (1); Brightness FX (13); Synth Brass 2 (20); Synth Brass 1 (21); Orchestral Harp (44); Contrabass (84) | 216 | B1-C#6 | 80-127 (4 levels) | 0.50 beats |
| 3 | bass | Fretless Bass (1) | 270 | B1-G3 | 102-127 (3 levels) | 1.50 beats |
| 4 | sustain/melodic | Square Lead (1); String Ensemble 1 (5) | 115 | G3-C#6 | 69-127 (6 levels) | 4.00 beats |
| 5 | sustain/melodic | Choir Aahs (1); String Ensemble 1 (5); Orchestral Harp (45); Synth Brass 2 (60) | 302 | G3-Bb6 | 91-127 (4 levels) | 2.00 beats |
| 6 | sustain | Tremolo Strings (1); Choir Aahs (61) | 54 | B2-F#6 | 69-127 (5 levels) | 4.00 beats |
| 7 | perc/sustain | String Ensemble 1 (1); Taiko Drum (5) | 734 | F#3-A5 | 58-127 (7 levels) | 0.50 beats |
| 8 | melodic/sustain | Orchestra Hit (4); Choir Aahs (86) | 175 | F#3-F#5 | 80-127 (5 levels) | 1.50 beats |
| 9 | sustain | String Ensemble 1 (1); Choir Aahs (5); Synth Brass 2 (61) | 88 | B3-B6 | 80-127 (5 levels) | 4.00 beats |
| 10 | drums | kit (1) | 1351 | Kick 1 x15, Side Stick x1, Snare 1 x725, Snare 2 x221, Low Tom x4, Open HH x8, Low-Mid Tom x6, Hi-Mid Tom x8, Crash 1 x50, High Tom x6, China x12, Crash 2 x119, Open Hi Conga x16, Low Conga x32, Cabasa x128 | 80-127 (5 levels) | 0.50 beats |

- Key estimate per 8 bars: 1-8 Bm (0.66), 9-16 Bm (0.77), 17-24 Bm (0.65), 25-32 F (0.48), 33-40 F# (0.46), 41-48 Bm (0.82), 49-56 Bm (0.85), 57-64 Bm (0.72), 65-72 Bm (0.82), 73-80 Bm (0.92), 81-88 Bm (0.76), 89-93 Bm (0.77)
- Section boundaries detected at bars: 5, 21, 57, 85, 89
- Same-channel same-pitch overlaps/duplicates: 8

