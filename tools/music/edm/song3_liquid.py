#!/usr/bin/env python3
# song3_liquid.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 3, Liquid": a liquid drum & bass remix of sf-music3 (the original
# game's music by Stardock, from mid/sf-music3.mid, Stardock Shared Source
# license), produced in code with tools/music/edm/synth.py and synth_liquid.py.
#
# Sample credits: harp, flute, French horn, violin/viola/cello sections and
# violin pizzicato from the VSCO-2 Community Edition by Versilian Studios
# (CC0 1.0 Universal, public domain dedication).  Everything else (Rhodes,
# choir, whistle, sub, reese, pads, drums, the breakbeat, FX) is synthesized.
#
# The original is written at 220 BPM with a kick on beat 1 and a side stick on
# beat 3 of every bar: a half-time feel.  Here one original bar = one DnB bar
# at 174 BPM (21% slower), so its two-beat melody notes become half notes over
# a fast two-step: the classic liquid "slow melody over rolling drums".
#
# Taken from the original song (A minor), note for note:
#   - the harp ostinato    A3 E4 D4 E4 / A3 F4 E4 F4 / B3 E4 D4 E4 ... in 8ths,
#                          with all its per-bar changes (bars 1-84)
#   - the fretless bass line, incl. the C2..A1 G1 walk-down and the
#     E1 B1 D2 E2 E1 figure (bars 1-84), as sub + reese
#   - the flute melody     A4 B4 C5 E5 | D5 | B4 | A4 B4 | C5 G5 F#5 F5 | E5 ...
#                          and its second half F5 D5 | E5 | D5 B4 | C5 ... (bars 9-41)
#   - the flute harmony line (bars 19-23, 33-41), the French horn counter-line
#     (bars 26-31), the choir lines (bars 29-31, 33-41)
#   - the string lines: A5 A5 B5 ... with the D6 B5 G#5 F5 E5 fall (bars 1-16),
#     the repeated-eighth figures of bars 17-23 (as pizzicato) and the C-section
#     line (bars 49-80); the low string pads of bars 49-80
#   - the choir melody of the C section F4 A4 | A4 D5 | F5 ... (bars 49-80) and the
#     whistle counter-line (bars 51-56, 67-76)
#   - the snare rhythm of the original kit, kept as the ghost-note pattern
#   - the form: A (Am C F E), B (Dm Em Am Bm7b5 E), C (Dm C Bm7b5 Am) sections
# New: 174 BPM two-step breakbeats with a chopped break layer, a liquid form
# (intro, drop 1, interlude, breakdown/build, drop 2, outro), extended chords
# (Am9, C6/9, Fmaj7, E7sus4 / E7b9, Dm9 ...) on a Rhodes, pads, and an ending
# in A minor: E7b9 -> a final Am9 hit with a short ring-out (loop-friendly).
# v2 (after the user's listening): the snare is a short, woody, tuned "pdlap"
# (A3 body, 1-4 kHz snap, 4 round-robins) instead of a bright noise snare; the
# A major lift and the Fmaj7#11 -> Amaj9 ending are gone.
#
#   python3 tools/music/edm/song3_liquid.py OUT.wav [bar0 bar1]
#   SONG3_MUTE=kick,snare,... mutes buses (for the chroma check)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Chorus, Compressor, Limiter,
                        HighpassFilter, LowpassFilter, LowShelfFilter,
                        HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_liquid as L  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 174.0
SPB = 60.0 / BPM
NBARS = 117
TAIL = 4.0
RING = 3.0   # seconds of ring-out after the final hit (bar 116)
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - T0


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 3 <= bar < RB1


# ------------------------------------------------------------- the original

# DnB bar -> original bar (1-based).  Everything that follows the original
# (bass, harp, chords, melodies) is placed through this map.
SRC = (list(range(1, 17)) +        # 0-15    intro          <- 1-16
       list(range(9, 41)) +        # 16-47   drop 1         <- 9-40
       list(range(41, 49)) +       # 48-55   interlude      <- 41-48
       list(range(49, 65)) +       # 56-71   breakdown/build <- 49-64
       list(range(25, 41)) +       # 72-87   drop 2a        <- 25-40
       list(range(65, 81)) +       # 88-103  drop 2b (C)    <- 65-80
       list(range(81, 85)) +       # 104-107 outro          <- 81-84
       list(range(1, 9)) +         # 108-115                <- 1-8
       [0])                        # 116 final hit (Am9)
assert len(SRC) == NBARS


def _a_cycle(bar):
    return ['Am9', 'C69', 'Fmaj7', 'E7b9' if bar % 8 == 0 else 'E7sus'][(bar - 1) % 4]


B_SEC = ['Dm9', 'Em7', 'Em7', 'Am9', 'Bm7b5', 'Bm7b5', 'E7', 'E7b9']
C_SEC = ['Dm9'] * 4 + ['AmC'] * 4 + ['Bm7b5'] * 3 + ['Dm6F'] + ['Am9'] * 4


def chord_of_orig(b):
    if 17 <= b <= 24 or 33 <= b <= 40:
        return B_SEC[(b - 17) % 8]
    if 49 <= b <= 64:
        return C_SEC[b - 49]
    if 65 <= b <= 80:
        return 'E7sus' if b == 76 else C_SEC[b - 65]
    return _a_cycle(b)


CHORDS = [chord_of_orig(s) if s else 'Am9' for s in SRC]
assert len(CHORDS) == NBARS

VOICING = {
    'Am9': ['A3', 'C4', 'E4', 'G4', 'B4'],
    'C69': ['G3', 'C4', 'E4', 'A4', 'D5'],
    'Fmaj7': ['F3', 'C4', 'E4', 'A4'],
    'Fmaj7#11': ['F3', 'C4', 'E4', 'A4', 'B4'],
    'E7sus': ['E3', 'B3', 'D4', 'A4'],
    'E7b9': ['G#3', 'D4', 'F4', 'B4'],
    'E7': ['G#3', 'D4', 'E4', 'B4'],
    'Dm9': ['D3', 'F3', 'C4', 'E4', 'A4'],
    'Em7': ['E3', 'B3', 'D4', 'G4'],
    'Bm7b5': ['B3', 'D4', 'F4', 'A4'],
    'AmC': ['A3', 'C4', 'E4', 'B4'],
    'Dm6F': ['A3', 'D4', 'F4', 'B4'],
    'Amaj9': ['A3', 'C#4', 'E4', 'G#4', 'B4'],
}

# fretless bass, per original bar: (beat, note, beats)
_A1 = [(0, 'A1', 4)]
_A2 = [(0, 'C2', 3.5), (3.5, 'A1', .25), (3.75, 'G1', .25)]
_A3 = [(0, 'F1', 4)]
_A4X = [(0, 'E1', .75), (.75, 'B1', .75), (1.5, 'D2', .5), (2, 'E2', 1), (3, 'E1', 1)]
_A4 = [(0, 'E2', 2), (2, 'E1', 2)]
BASS = {}
for _b in range(1, 85):
    if _b <= 8:
        BASS[_b] = [_A1, _A2, _A3, _A4X][(_b - 1) % 4]
    elif 17 <= _b <= 24 or 33 <= _b <= 40:
        BASS[_b] = [(0, ['D2', 'E2', 'E1', 'A1', 'B1', 'B1', 'E1', 'E2'][(_b - 17) % 8], 4)]
    elif 49 <= _b <= 80:
        BASS[_b] = []
    else:
        BASS[_b] = [_A1, _A2, _A3, _A4][(_b - 1) % 4]
_C = {49: [(0, 'D2', 6)], 50: [(2, 'D2', 2)], 51: [(0, 'D1', 4)],
      52: [(0, 'D2', 1), (1, 'A1', 1), (2, 'D2', 1), (3, 'F2', 1)],
      53: [(0, 'C2', 6)], 54: [(2, 'C2', 2)], 55: [(0, 'C2', 4)],
      56: [(0, 'C2', 1), (1, 'D2', 1), (2, 'E2', 1), (3, 'C2', 1)],
      57: [(0, 'B1', 6)], 58: [(2, 'B1', 2)], 59: [(0, 'B1', 4)], 60: [(0, 'F1', 4)],
      61: [(0, 'A1', 6)], 62: [(2, 'E1', 2)], 63: [(0, 'A1', 4)], 64: [(0, 'C2', 4)]}
for _b, _v in _C.items():
    BASS[_b] = _v
    BASS[_b + 16] = _v
BASS[76] = [(0, 'E2', 4)]
BASS[77] = [(0, 'A1', 6)]
BASS[78] = [(2, 'E1', 2)]
BASS[79] = [(0, 'A1', 8)]
BASS[80] = []

# harp ostinato cells (one cell = 4 eighths, played twice per bar)
_HC = {'AEDE': ['A3', 'E4', 'D4', 'E4'], 'AFEF': ['A3', 'F4', 'E4', 'F4'], 'BEDE': ['B3', 'E4', 'D4', 'E4'],
       'AFDF': ['A3', 'F4', 'D4', 'F4'], 'BFDF': ['B3', 'F4', 'D4', 'F4'], 'AECE': ['A3', 'E4', 'C4', 'E4'],
       'AEC#E': ['A3', 'E4', 'C#4', 'E4']}
HARP = {}
for _b in range(1, 85):
    if 17 <= _b <= 24 or 33 <= _b <= 40:
        HARP[_b] = ['AFDF', 'BEDE', 'BEDE', 'AEDE', 'BFDF', 'BFDF', 'BEDE', 'BEDE'][(_b - 17) % 8]
    elif 49 <= _b <= 80:
        k = (_b - 49) % 16
        HARP[_b] = (['AFDF', 'AFDF', 'AFEF', 'AFDF'] + ['AECE'] * 4 + ['BFDF'] * 4 + ['AECE'] * 4)[k]
    else:
        HARP[_b] = ['AEDE', 'AEDE', 'AFEF', 'BEDE'][(_b - 1) % 4]
HARP[76] = 'BEDE'

# melodies: (orig bar, beat, note, beats)
FLUTE = [(9, 0, 'A4', 2), (9, 2, 'B4', 2), (10, 0, 'C5', 2), (10, 2, 'E5', 2), (11, 0, 'D5', 4), (12, 0, 'B4', 4),
         (13, 0, 'A4', 2), (13, 2, 'B4', 2), (14, 0, 'C5', 2), (14, 2, 'G5', 1.5), (14, 3.5, 'F#5', .25),
         (14, 3.75, 'F5', .25), (15, 0, 'E5', 7), (16, 3, 'D5', .5), (16, 3.5, 'E5', .5),
         (17, 0, 'F5', 2), (17, 2, 'D5', 2), (18, 0, 'E5', 4), (19, 0, 'D5', 2), (19, 2, 'B4', 2), (20, 0, 'C5', 4),
         (21, 0, 'C5', 2), (21, 2, 'B4', 2), (22, 0, 'A4', 2), (22, 2, 'F4', 2), (23, 0, 'B4', 8),
         (25, 0, 'A4', 2), (25, 2, 'B4', 2), (26, 0, 'C5', 2), (26, 2, 'E5', 2), (27, 0, 'D5', 3), (27, 3, 'E5', .5),
         (27, 3.5, 'D5', .5), (28, 0, 'B4', 4), (29, 0, 'A4', 2), (29, 2, 'B4', 2), (30, 0, 'C5', 2),
         (30, 2, 'G5', 2), (31, 0, 'E5', 7), (32, 3, 'D5', .5), (32, 3.5, 'E5', .5),
         (33, 0, 'F5', 2), (33, 2, 'A5', 2), (34, 0, 'E5', 4), (35, 0, 'D5', 2), (35, 2, 'F5', 2), (36, 0, 'E5', 4),
         (37, 0, 'C5', 2), (37, 2, 'B4', 2), (38, 0, 'A4', 2), (38, 2, 'F4', 2), (39, 0, 'B4', 6), (40, 2, 'C5', 2),
         (41, 0, 'A4', 8),
         (81, 0, 'A4', 2), (81, 2, 'B4', 2), (82, 0, 'C5', 2), (82, 2, 'E5', 2), (83, 0, 'D5', 4), (84, 0, 'B4', 8)]
FLUTE_HARM = [(19, 0, 'B4', 2), (19, 2, 'G4', 2), (20, 0, 'A4', 4), (23, 0, 'G#4', 8),
              (33, 0, 'D5', 2), (33, 2, 'F5', 2), (34, 0, 'D5', 4), (35, 0, 'B4', 2), (35, 2, 'D5', 2),
              (36, 0, 'C5', 4), (39, 0, 'G#4', 6), (40, 2, 'A4', 2)]
HORN = [(26, 2, 'E3', 1), (26, 3, 'G3', 1), (27, 0, 'A3', 2), (27, 2, 'E3', 2), (28, 0, 'G3', 2), (28, 2, 'F3', 2),
        (29, 0, 'E3', 6), (30, 2, 'C3', 2), (31, 0, 'D3', 8)]
CHOIR1 = [(29, 0, 'E4', 4), (30, 0, 'E4', 2), (30, 2, 'C4', 2), (31, 0, 'D4', 8)]
CHOIR_HARM = [(33, 0, 'D4', 2), (33, 2, 'F4', 2), (34, 0, 'E4', 4), (35, 0, 'A4', 2), (35, 2, 'B4', 2),
              (36, 0, 'C5', 4), (37, 0, 'D5', 4), (38, 0, 'D5', 2), (38, 2, 'C5', 2), (39, 0, 'B4', 6),
              (40, 2, 'C5', 2), (41, 0, 'A4', 8)]
_CM = [(0, 0, 'F4', 2), (0, 2, 'A4', 1.5), (1, 0, 'A4', 2), (1, 2, 'D5', 2), (2, 0, 'F5', 8),
       (4, 0, 'E4', 2), (4, 2, 'A4', 2), (5, 0, 'A4', 2), (5, 2, 'C5', 2), (6, 0, 'E5', 8),
       (8, 0, 'D4', 2), (8, 2, 'F4', 2), (9, 0, 'F4', 2), (9, 2, 'A4', 2)]
CHOIR_MEL = ([(49 + b, bt, n, d) for b, bt, n, d in _CM] +
             [(59, 0, 'D5', 4), (60, 0, 'B4', 4), (61, 0, 'C5', 7), (62, 3, 'C5', .5), (62, 3.5, 'B4', .5),
              (63, 0, 'A4', 8)] +
             [(65 + b, bt, n, d) for b, bt, n, d in _CM] +
             [(75, 0, 'D5', 4), (76, 0, 'E4', 4), (77, 0, 'A4', 16)])
WHISTLE = [(51, 0, 'A3', 2), (51, 2, 'B3', 2), (52, 0, 'C4', 4), (55, 0, 'C4', 2), (55, 2, 'D4', 2), (56, 0, 'E4', 4),
           (67, 0, 'A3', 2), (67, 2, 'B3', 2), (68, 0, 'C4', 2), (68, 2, 'D4', 2), (69, 0, 'C4', 8),
           (71, 0, 'C4', 2), (71, 2, 'D4', 2), (72, 0, 'E4', 4), (73, 0, 'D4', 2), (73, 2, 'C4', 2),
           (74, 0, 'B3', 2), (74, 2, 'A3', 2), (75, 0, 'B3', 4), (76, 0, 'E4', 4)]
# high strings (String Ensemble 1); 'p' = the staccato eighths of bars 17-23
STR_HI = []
for _b, _n in {1: 'A5', 2: 'A5', 3: 'B5', 5: 'A5', 6: 'A5', 7: 'B5', 9: 'A5', 10: 'A5', 11: 'B5', 12: 'E6',
               13: 'A5', 14: 'A5', 15: 'B5', 16: 'E6', 18: 'E5', 20: 'C5', 22: 'D5', 24: 'B4',
               41: 'A5', 42: 'A5', 43: 'B5', 44: 'G5', 45: 'A5', 46: 'A5', 47: 'B5', 48: 'E6',
               49: 'A5', 50: 'A5', 51: 'B5', 52: 'C6', 59: 'B5', 60: 'D6',
               65: 'A5', 66: 'A5', 67: 'B5', 68: 'C6', 75: 'B5', 76: 'B5'}.items():
    STR_HI.append((_b, 0, _n, 4))
for _b0 in (25, 81):
    for _k, _n in enumerate(['A5', 'A5', 'B5', 'E6']):
        if _b0 + _k <= 84:
            STR_HI.append((_b0 + _k, 0, _n, 4))
for _k, _n in enumerate(['A5', 'A5', 'B5', 'E6']):
    STR_HI.append((29 + _k, 0, _n, 4))
for _b in (34, 36, 38, 40):
    STR_HI.append((_b, 0, {34: 'E5', 36: 'C5', 38: 'D5', 40: 'B4'}[_b], 4))
STR_HI += [(4, 0, 'A5', 2), (4, 2, 'E5', 2),
           (8, 0, 'D6', .75), (8, .75, 'B5', .75), (8, 1.5, 'G#5', .5), (8, 2, 'F5', 1), (8, 3, 'E5', 1),
           (53, 0, 'B5', 8), (55, 0, 'B5', 3.5), (56, 0, 'B5', 2), (56, 2, 'C6', 2), (57, 0, 'A5', 8),
           (61, 0, 'A5', 16), (69, 0, 'B5', 8), (71, 0, 'B5', 3.5), (72, 0, 'B5', 2), (72, 2, 'C6', 2),
           (73, 0, 'A5', 8), (77, 0, 'A5', 16)]
PIZZ = []
for _b, _n in {17: 'F5', 19: 'D5', 21: 'F5', 23: 'D5', 33: 'F5', 35: 'D5', 37: 'F5', 39: 'D5'}.items():
    for _k in range(8):
        PIZZ.append((_b, _k * .5, _n, .25))
STR_LO = [(1, 0, 'C5', 4), (2, 0, 'D5', 4), (3, 0, 'F5', 4), (4, 0, 'D5', 4),
          (5, 0, 'C5', 4), (6, 0, 'D5', 4), (7, 0, 'F5', 4), (8, 0, 'E5', 4)]
CELLO = [(49, 0, 'F3', 16), (53, 0, 'E3', 12), (56, 0, 'C4', 4), (57, 0, 'D4', 16), (61, 0, 'A3', 16),
         (65, 0, 'F3', 16), (69, 0, 'E3', 16), (73, 0, 'D4', 16), (77, 0, 'E3', 16)]
VIOLA = [(49, 0, 'D4', 16), (53, 0, 'A3', 16), (57, 0, 'B3', 16), (61, 0, 'C4', 16),
         (65, 0, 'D4', 16), (69, 0, 'A3', 16), (73, 0, 'B3', 16), (77, 0, 'A3', 16)]
FALL = [(0, 'D6', .75), (.75, 'B5', .75), (1.5, 'G#5', .5), (2, 'F5', 1), (3, 'E5', 1)]   # orig bar 8

# snare onsets of the original kit, per original bar (beats) -> ghost notes
ORIG_SN = {5: [0, 1.5, 1.75, 2, 2.5, 3, 3.5], 6: [0, 1.5, 1.75, 2], 7: [0, 1.5, 1.75, 2, 2.5, 3, 3.5],
           8: [0, 1, 1.5, 2], 9: [0, 1.5, 2, 2.5, 3, 3.5], 11: [0, 1.5, 1.75, 2, 2.5, 3, 3.5],
           13: [0, 1.5, 1.75, 2, 3], 14: [0, 1, 1.5, 2], 17: [0, 1, 2, 2.5, 3, 3.5], 19: [0, 1.5, 1.75, 2, 3],
           21: [0, 1.5, 1.75, 2, 2.5, 3, 3.5], 22: [0, 2], 23: [0, 1.5, 2, 3], 24: [0, 1, 1.5, 2],
           25: [0, 1.5, 1.75, 2, 2.5, 3, 3.5], 29: [0, 1, 2, 3, 3.5], 31: [0, 3, 3.5], 33: [0, 1, 1.5, 2, 3, 3.5],
           35: [0, 1.5, 1.75, 2, 2.5, 3, 3.5], 37: [0, 3, 3.5], 39: [0, 1.5, 1.75, 2, 2.5, 3, 3.5],
           41: [0, 2, 2.5, 3, 3.5], 42: [0, 2], 43: [0, 1.5, 1.75, 2, 3], 44: [0, 1, 1.5, 2],
           45: [0, 1.5, 1.75, 2, 3], 47: [0, 1.5, 1.75, 2, 3], 48: [0, .5, 1, 1.5, 2],
           83: [0, 1.5, 1.75, 2, 2.5, 3, 3.5]}


def follow(lst, d0, d1, fn, transpose=0, only=None):
    """Place the notes of an (orig bar, beat, note, beats) list wherever SRC[d] is their bar."""
    for d in range(d0, d1):
        if not live(d):
            continue
        for ob, beat, note, dur in lst:
            if ob == SRC[d] and (only is None or only(ob, beat, dur)):
                fn(d, beat, m(note) + transpose, dur)


# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'tops', 'brk', 'fx', 'sub', 'reese', 'rhodes', 'harp', 'flute', 'whistle',
         'choir', 'strings', 'lowstr', 'horn', 'pad', 'atmos']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


print('loading samples', flush=True)
HARP_S = L.Sampler('Strings/Harp/*.wav', octave=0, layer=None, norm_s=0.5)
FLUTE_S = L.Sampler('Woodwinds/Flute/susvib/*.wav', octave=12, layer=None)
FLUTE_ST = L.Sampler('Woodwinds/Flute/stac/*.wav', octave=12, norm_s=0.15)
HORN_S = L.Sampler('Brass/F Horn/sus/*.wav', octave=12)
VLN_S = L.Sampler('Strings/Violin Section/susVib/*.wav', octave=12)
PIZZ_S = L.Sampler('Strings/Violin Section/Pizz/*.wav', octave=12, norm_s=0.2)
VLA_S = L.Sampler('Strings/Viola Section/susvib/*.wav', octave=12)
VC_S = L.Sampler('Strings/Cello Section/susvib/*.wav', octave=12)
rg = S.rng()


def hv(v, spread=0.08):
    """humanised velocity"""
    return v * (1 - spread / 2 + spread * rg.random())


# ------------------------------------------------------------- drums

SWING = 0.10   # fraction of a 16th that the off-16ths are late


def st(step):
    return step / 4 + (SWING / 4 if step % 2 else 0.0)


_rr = 0


def place_kick(d, step, vel=1.0):
    t = T(d, step / 4)
    bus['kick'].add(cached(('kick',), L.dnb_kick), t, vel)
    kicks.append(t)


def snare_hit(d, step, vel=1.0):
    global _rr
    _rr += 1
    v = _rr % 4
    bus['snare'].add(cached(('pd', v), lambda: L.pdlap(1.0, v)), T(d, step / 4), hv(vel, 0.08))


def ghost_hit(d, step, vel=0.22):
    v = int(rg.integers(4))
    bus['snare'].add(cached(('pg', v), lambda: L.pdlap_ghost(1.0, v)), T(d, st(step)), hv(vel * 0.55, 0.3))


def beat_bar(d, var=0, fill=None, hats='16', ride=False, shaker=True, kick=True, ghosts=True):
    if not live(d):
        return
    stop = {'snare': 8, 'roll': 12, 'stop': 8, 'stop2': 14}.get(fill, 16)
    # two-step: kick 1 and the "and" of 3, snares on 2 and 4
    ks = [(0, 1.0), (10, .9)]
    if var == 1:
        ks.append((7, .65))
    if var == 3:
        ks.append((13, .55))
    if kick:
        for s, v in ks:
            if s < stop:
                place_kick(d, s, v)
    for s in (4, 12):
        if s < stop:
            snare_hit(d, s, 0.92 if s == 4 else 1.0)
    if ghosts:
        on = ORIG_SN.get(SRC[d]) if SRC[d] else None
        steps = sorted(set(int(b * 4) for b in on)) if on else [7, 9, 15]
        for s in steps:
            if s in (0, 4, 10, 12) or s >= stop or abs(s - 4) < 1 or abs(s - 12) < 1:
                continue
            ghost_hit(d, s, 0.2 if s % 2 == 0 else 0.26)
        if var == 3 and stop == 16:
            ghost_hit(d, 14, 0.35)
            ghost_hit(d, 15, 0.42)
    if hats:
        for s in range(16):
            if s >= stop:
                break
            if hats == '8' and s % 2:
                continue
            if s in (6, 14) and hats != 'soft':
                bus['tops'].add(cached(('oh',), L.open_hat), T(d, st(s)), hv(0.42))
                continue
            acc = [.55, .25, .42, .3][s % 4]
            if hats == 'soft':
                acc *= 0.6
            bus['tops'].add(S.hat(1.0, False, -0.3 + 0.6 * rg.random()), T(d, st(s)), hv(acc * 0.7, 0.25))
    if shaker:
        for s in range(16):
            if s < stop:
                bus['tops'].add(L.shaker(1.0, 0.45), T(d, st(s)), hv(0.16 if s % 2 else 0.24, 0.3))
    if ride:
        for s in (0, 4, 8, 12):
            if s < stop:
                bus['tops'].add(cached(('ride',), lambda: S.ride(1.0, 0.35)), T(d, s / 4), hv(0.35 if s % 8 else 0.45))
    if fill == 'snare':
        for i in range(8):
            snare_hit(d, 8 + i, 0.35 + 0.6 * (i / 7) ** 1.2)
    elif fill == 'roll':
        for i in range(8):
            snare_hit(d, 12 + i / 2, 0.25 + 0.5 * (i / 7))
        place_kick(d, 12, 0.8)


def tom_fill(d, beat0=2.0):
    for i, md in enumerate((57, 55, 52, 50, 48, 45, 43, 40)):
        bus['snare'].add(S.tom(md, 1.0), T(d, beat0 + i * .25), 0.55)


BRK_ORDERS = [[0, 1, 2, 3, 4, 5, 6, 7], [8, 9, 10, 11, 12, 13, 14, 15],
              [0, 1, 2, 3, 4, 5, 6, 7], [8, 9, 10, 11, 12, 13, 14, 15]]
BRK_CHOPS = [[0, 1, 2, 11, 4, 5, 6, 15], [8, 1, 10, 3, 0, 5, 6, 6], [0, 0, 2, 3, 5, 5, 6, 14],
             [8, 9, 2, 3, 12, 13, 6, 7]]
LOOP = L.break_loop(SPB)


def break_bars(d0, d1, chop_every=4, vel=1.0, fill_last=False):
    for d in range(d0, d1):
        if not live(d):
            continue
        o = BRK_ORDERS[d % 4]
        if (d + 1) % chop_every == 0:
            o = BRK_CHOPS[(d // chop_every) % len(BRK_CHOPS)]
        if fill_last and d == d1 - 1:
            o = [0, 1, 2, 3, 2, 2, 6, 6]
        bus['brk'].add(cached(('brk', tuple(o)), lambda: L.chop(LOOP, SPB, o)), T(d), vel)


def crash_at(t, vel=1.0):
    bus['fx'].add(S.crash(1.0, 3.5), t, vel * 0.5)


# ------------------------------------------------------------- bass

def bass_notes(d0, d1):
    out = []
    for d in range(d0, d1):
        s = SRC[d]
        if s == 0:
            out.append((T(d) + T0, m('A1'), B(2.5)))
            continue
        for beat, note, dur in BASS[s]:
            out.append((T(d, beat) + T0, m(note), B(dur)))
    out.sort()
    # cut notes that would run into the next one
    res = []
    for i, (ts, md, du) in enumerate(out):
        if i + 1 < len(out):
            du = min(du, out[i + 1][0] - ts)
        res.append((ts, md, du))
    return res


def bass(d0, d1, sub=1.0, reese=1.0, cut=420.0):
    a, b = max(d0, RB0), min(d1, RB1)
    if a >= b:
        return
    notes = bass_notes(a, b)
    t_start = T(a) + T0
    n = int((T(b) - T(a) + 0.3) * SR)
    rel = [(ts - t_start, md, du) for ts, md, du in notes]
    pitch, env = L.line_traj(rel, n)
    if sub:
        y = L.sub_from(pitch, env)
        bus['sub'].add(np.stack([y, y]), T(a), sub)
    if reese:
        bus['reese'].add(L.reese_from(pitch, env, t_start, SPB, cut=cut), T(a), reese)


# ------------------------------------------------------------- harmony

def rhodes_bar(d, style='comp', vel=0.6):
    if not live(d):
        return
    ch = CHORDS[d]
    if style == 'pad':
        hits = [(0, 3.9, 1.0)]
    elif style == 'comp':
        hits = [(0, 2.4, 1.0), (2.5, 1.4, 0.7)]
    else:  # 'push': anticipations on the last 16th
        hits = [(0, 1.4, 1.0), (1.5, 0.9, 0.6), (2.5, 1.4, 0.8)]
    for beat, dur, v in hits:
        for i, note in enumerate(VOICING[ch]):
            vv = round(vel * v * (0.9 + 0.1 * (i == len(VOICING[ch]) - 1)), 2)
            bus['rhodes'].add(cached(('rh', note, dur, vv), lambda: L.rhodes(m(note), B(dur), vv)),
                              T(d, beat + 0.012 * i), 1.0)


def pad_bars(d0, d1, vel=1.0, attack=0.6, cut=1400.0):
    d = max(d0, RB0 - 3)
    d1 = min(d1, RB1)
    while d < d1:
        ch = CHORDS[d]
        e = d
        while e < d1 and CHORDS[e] == ch:
            e += 1
        dur = T(e) - T(d)
        for note in VOICING[ch]:
            bus['pad'].add(S.supersaw(m(note), dur, 1.0, voices=5, detune=0.18, a=attack, d=1.0, s=0.9,
                                      r=0.6, cut=cut, q=0.6), T(d), vel * 0.35)
        d = e


def harp_bars(d0, d1, vel=0.8, ring=1.2):
    for d in range(d0, d1):
        if not live(d):
            continue
        s = SRC[d]
        if s == 0:
            continue
        cell = _HC[HARP[s]]
        for half in (0, 2):
            for k, note in enumerate(cell):
                acc = 1.0 if k == 0 else 0.8
                bus['harp'].add(cached(('hp', note), lambda: HARP_S.note(m(note), B(ring), 0.8, r=0.5)),
                                T(d, half + k * .5), hv(vel * acc, 0.12))


def harp_gliss(d, beat, notes, vel=0.7, step=0.125):
    for i, note in enumerate(notes):
        bus['harp'].add(HARP_S.note(m(note), 4.0, 0.8, r=1.5), T(d, beat + i * step), vel * (0.7 + 0.3 * i / len(notes)))


# ------------------------------------------------------------- melodic voices

def flute_fn(target='flute', vel=0.9, pan=0.0):
    def fn(d, beat, md, dur):
        if dur <= 0.5:
            sig = FLUTE_ST.note(md, B(dur), vel, r=0.05)
        else:
            sig = FLUTE_S.note(md, B(dur) - 0.02, vel, r=0.12)
        if pan:
            sig = sig * np.array([[np.sqrt((1 - pan) / 2) * 1.41], [np.sqrt((1 + pan) / 2) * 1.41]], np.float32)
        bus[target].add(sig, T(d, beat), 1.0)
    return fn


def whistle_fn(vel=0.8):
    def fn(d, beat, md, dur):
        bus['whistle'].add(L.whistle(md, B(dur) - 0.02, vel), T(d, beat), 1.0)
    return fn


def choir_fn(vel=0.8, vowel='a', a=0.45):
    def fn(d, beat, md, dur):
        bus['choir'].add(L.choir(md, B(dur), vel, a=a, vowel=vowel), T(d, beat), 1.0)
    return fn


def sampler_fn(sm, target, vel=0.8, a=0.0, r=0.3, pan=0.0):
    def fn(d, beat, md, dur):
        sig = sm.note(md, B(dur), vel, r=r, a=a)
        if pan:
            sig = sig * np.array([[np.sqrt((1 - pan) / 2) * 1.41], [np.sqrt((1 + pan) / 2) * 1.41]], np.float32)
        bus[target].add(sig, T(d, beat), 1.0)
    return fn


def short(ob, beat, dur):
    return dur <= 0.3


def long_(ob, beat, dur):
    return dur > 0.3


def fall(d, fn, transpose=0):
    if live(d):
        for beat, note, dur in FALL:
            fn(d, beat, m(note) + transpose, dur)


# ---------------------------------------------------------- arrangement

print('arranging', flush=True)
# atmosphere bed all through (louder in the quiet parts, via its own gain curve)
if True:
    n_at = N
    bus['atmos'].add(L.crackle(n_at), 0.0, 1.0)

# INTRO 0-15 (orig 1-16) ----------------------------------------------
pad_bars(0, 16, vel=0.8, attack=1.5, cut=900)
for d in range(0, 16):
    rhodes_bar(d, 'pad', vel=0.45)
harp_bars(0, 16, vel=0.7)
follow(STR_HI, 0, 16, sampler_fn(VLN_S, 'strings', 0.45, a=0.35, r=0.6), only=long_)
follow(STR_LO, 0, 8, sampler_fn(VLN_S, 'strings', 0.35, a=0.4, r=0.6, pan=-0.3))
fall(7, flute_fn(vel=0.55))
fall(15, flute_fn(vel=0.65))
# filtered break from bar 8, sub from bar 12
for d in range(8, 16):
    beat_bar(d, var=d % 4, hats='soft', shaker=d >= 12, kick=d >= 12,
             fill='snare' if d == 15 else None)
break_bars(8, 16, vel=0.9)
bass(12, 16, sub=0.6, reese=0.0)
bus['fx'].add(S.noise_riser(T(16) - T(12), 300, 9000, 2.0), T(12), 0.35)
bus['fx'].add(S.reverse_cymbal(B(4)), T(15), 0.45)

# DROP 1 16-47 (orig 9-40) ----------------------------------------------
for d in (16, 24, 32, 40):
    crash_at(T(d), 1.0 if d % 16 == 0 else 0.75)
bus['fx'].add(S.impact(), T(16), 0.8)
for d in range(16, 48):
    k = d - 16
    fill = None
    if d in (23, 39):
        fill = 'roll'
    elif d == 31:
        fill = 'snare'
    elif d == 47:
        fill = 'snare'
    beat_bar(d, var=k % 4, fill=fill, ride=d >= 32)
break_bars(16, 48, vel=1.0)
bass(16, 48)
for d in range(16, 48):
    rhodes_bar(d, 'comp', vel=0.6)
pad_bars(16, 48, vel=0.6, attack=0.4, cut=1500)
harp_bars(16, 48, vel=0.7)
follow(FLUTE, 16, 49, flute_fn(vel=0.95))
follow(FLUTE_HARM, 16, 48, flute_fn(vel=0.6, pan=0.35))
follow(STR_HI, 16, 48, sampler_fn(VLN_S, 'strings', 0.4, a=0.3, r=0.5), only=long_)
follow(PIZZ, 16, 48, sampler_fn(PIZZ_S, 'strings', 0.6, r=0.2, pan=-0.35))
follow(HORN, 16, 48, sampler_fn(HORN_S, 'horn', 0.6, a=0.06, r=0.35, pan=-0.2))
follow(CHOIR1, 16, 48, choir_fn(0.6, 'o'))
follow(CHOIR_HARM, 16, 49, choir_fn(0.65, 'a'))

# INTERLUDE 48-55 (orig 41-48) -------------------------------------------
crash_at(T(48), 0.8)
for d in range(48, 56):
    beat_bar(d, var=(d - 48) % 4, hats='8', ride=True, fill='stop' if d == 55 else None)
break_bars(48, 55, chop_every=2, vel=1.0)
bass(48, 55)
bass(55, 56, sub=0.8, reese=0.6)
for d in range(48, 56):
    rhodes_bar(d, 'push', vel=0.62)
pad_bars(48, 56, vel=0.55, attack=0.4, cut=1600)
harp_bars(48, 56, vel=0.85)
follow(STR_HI, 48, 56, sampler_fn(VLN_S, 'strings', 0.45, a=0.3, r=0.5), only=long_)
fall(55, flute_fn(vel=0.7))
bus['fx'].add(S.downlifter(B(8)), T(56), 0.4)

# BREAKDOWN 56-63, BUILD 64-71 (orig 49-64) -------------------------------
crash_at(T(56), 0.6)
bus['fx'].add(S.impact(), T(56), 0.6)
bass(56, 64, sub=0.55, reese=0.0)
bass(64, 72, sub=0.8, reese=0.6, cut=300)
for d in range(56, 72):
    rhodes_bar(d, 'pad', vel=0.5)
pad_bars(56, 72, vel=0.75, attack=1.0, cut=1300)
harp_bars(56, 72, vel=0.65)
follow(CHOIR_MEL, 56, 72, choir_fn(0.85, 'a', a=0.3))
follow(CHOIR_MEL, 56, 72, sampler_fn(VLN_S, 'strings', 0.55, a=0.25, r=0.6), transpose=12)
follow(STR_HI, 56, 72, sampler_fn(VLN_S, 'strings', 0.35, a=0.5, r=0.7, pan=0.3), only=long_)
follow(CELLO, 56, 72, sampler_fn(VC_S, 'lowstr', 0.6, a=0.5, r=0.8, pan=-0.3))
follow(VIOLA, 56, 72, sampler_fn(VLA_S, 'lowstr', 0.5, a=0.5, r=0.8, pan=0.3))
follow(WHISTLE, 56, 72, whistle_fn(0.6), transpose=12)
# the build: half-time pulse, then the break returns under a rising filter
for d in range(64, 68):
    if live(d):
        place_kick(d, 0, 0.8)
        snare_hit(d, 8, 0.6)
for d in range(68, 72):
    beat_bar(d, var=d % 4, hats='soft', shaker=True, fill='stop2' if d == 71 else None)
break_bars(66, 72, vel=0.9)
for i in range(16):
    snare_hit(70, i, 0.2 + 0.45 * (i / 15) ** 1.5)
for i in range(8):
    snare_hit(71, i, 0.55 + 0.35 * (i / 7))
bus['fx'].add(S.noise_riser(T(72) - T(64), 250, 11000, 2.5), T(64), 0.6)
bus['fx'].add(S.pitch_riser(T(71, 3.5) - T(68), 45, 69), T(68), 0.4)
bus['fx'].add(S.reverse_cymbal(B(4)), T(71), 0.6)

# DROP 2 72-103 (orig 25-40, then the C section 65-80) ------------------
for d in (72, 80, 88, 96):
    crash_at(T(d), 1.0)
bus['fx'].add(S.impact(), T(72), 1.0)
bus['fx'].add(S.impact(), T(88), 0.8)
for d in range(72, 104):
    k = d - 72
    fill = {79: 'roll', 87: 'snare', 95: 'roll', 103: 'snare'}.get(d)
    beat_bar(d, var=k % 4, fill=fill, ride=True)
break_bars(72, 104, vel=1.05)
bass(72, 104, cut=480)
for d in range(72, 104):
    rhodes_bar(d, 'comp' if d < 88 else 'push', vel=0.62)
pad_bars(72, 104, vel=0.7, attack=0.3, cut=1800)
harp_bars(72, 104, vel=0.75)
follow(FLUTE, 72, 88, flute_fn(vel=1.0))
follow(FLUTE, 80, 88, whistle_fn(0.35), transpose=12)
follow(FLUTE_HARM, 72, 88, flute_fn(vel=0.6, pan=0.35))
follow(HORN, 72, 88, sampler_fn(HORN_S, 'horn', 0.7, a=0.06, r=0.35, pan=-0.2))
follow(CHOIR1, 72, 88, choir_fn(0.65, 'o'))
follow(CHOIR_HARM, 72, 88, choir_fn(0.7, 'a'))
follow(STR_HI, 72, 88, sampler_fn(VLN_S, 'strings', 0.45, a=0.3, r=0.5), only=long_)
follow(PIZZ, 72, 88, sampler_fn(PIZZ_S, 'strings', 0.65, r=0.2, pan=-0.35))
# C section climax: the choir melody on the flute an octave up, doubled by the choir
follow(CHOIR_MEL, 88, 104, flute_fn(vel=1.0), transpose=12)
follow(CHOIR_MEL, 88, 104, choir_fn(0.8, 'a', a=0.2))
follow(WHISTLE, 88, 104, whistle_fn(0.55), transpose=12)
follow(STR_HI, 88, 104, sampler_fn(VLN_S, 'strings', 0.5, a=0.3, r=0.6), only=long_)
follow(CELLO, 88, 104, sampler_fn(VC_S, 'lowstr', 0.6, a=0.4, r=0.8, pan=-0.3))
follow(VIOLA, 88, 104, sampler_fn(VLA_S, 'lowstr', 0.5, a=0.4, r=0.8, pan=0.3))

# OUTRO 104-116 (orig 81-84, 1-8, final hit) -----------------------------
crash_at(T(104), 0.8)
for d in range(104, 116):
    beat_bar(d, var=(d - 104) % 4, hats='8' if d >= 112 else '16', ride=d < 108, shaker=True)
break_bars(104, 112, vel=0.8)
bass(104, 116, reese=0.7)
bass(116, 117, sub=0.9, reese=0.5)
for d in range(104, 116):
    rhodes_bar(d, 'comp', vel=0.55)
pad_bars(104, 116, vel=0.65, attack=0.8, cut=1300)
harp_bars(104, 116, vel=0.65)
follow(FLUTE, 104, 108, flute_fn(vel=0.9))
follow(STR_HI, 108, 116, sampler_fn(VLN_S, 'strings', 0.4, a=0.4, r=0.6), only=long_)
follow(STR_LO, 108, 116, sampler_fn(VLN_S, 'strings', 0.3, a=0.4, r=0.6, pan=-0.3))
fall(115, flute_fn(vel=0.75))
# the final hit: the drums stop on beat 1 of bar 116 with an Am9 chord (E7b9 -> Am9),
# the harp ostinato's first notes and the flute landing on A4; it rings ~2-3 s.
if live(116):
    place_kick(116, 0, 1.0)
    snare_hit(116, 0, 0.9)
    bus['fx'].add(S.crash(1.0, 2.5), T(116), 0.45)
    for note in ['A2', 'E3', 'G3', 'C4', 'E4', 'B4']:
        bus['rhodes'].add(L.rhodes(m(note), 1.0, 0.6, r=0.5), T(116), 1.0)
    for k, note in enumerate(['A3', 'E4', 'D4', 'E4']):
        bus['harp'].add(HARP_S.note(m(note), 1.5, 0.8, r=0.5), T(116, k * .5), 0.65 * (1 - 0.15 * k))
    bus['harp'].add(HARP_S.note(m('A2'), 1.5, 0.8, r=0.6), T(116), 0.6)
    flute_fn(vel=0.75)(116, 0, m('A4'), 2)
    sampler_fn(VLN_S, 'strings', 0.35, a=0.05, r=0.4)(116, 0, m('E5'), 2.5)
    sampler_fn(VC_S, 'lowstr', 0.5, a=0.02, r=0.4)(116, 0, m('A2'), 2.5)

# ------------------------------------------------------------- mix

print('mixing', flush=True)
# free the sample data before mixing (memory)
del HARP_S, FLUTE_S, FLUTE_ST, HORN_S, VLN_S, PIZZ_S, VLA_S, VC_S, LOOP
_cache.clear()
import gc  # noqa: E402
gc.collect()


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


def sweep(k, d0, d1, f0, f1, shape=1.6, q=0.8):
    """time-varying low-pass on bus k between bars d0..d1 (f0 -> f1)."""
    i0, i1 = int(max(0, T(d0)) * SR), int(max(0, T(d1)) * SR)
    i1 = min(i1, N)
    if i1 - i0 < 100:
        return
    u = np.linspace(0, 1, i1 - i0)
    full = np.zeros(i1 - i0)
    if T(d0) < 0:
        full = u  # partial render: keep the shape roughly
    fc = f0 * (f1 / f0) ** (np.maximum(u, full) ** shape)
    bus[k].x[:, i0:i1] = S.svf(bus[k].x[:, i0:i1], fc, q).astype(np.float32)


# intro: break and hats open from a dark LP; music opens over the first 8 bars
for k in ('snare', 'tops', 'brk', 'kick'):
    sweep(k, 8, 16, 350, 16000, 2.2)
for k in ('rhodes', 'harp', 'pad'):
    sweep(k, 0, 8, 900, 14000, 1.4)
# build: break and reese open up
for k in ('brk', 'tops', 'snare', 'reese'):
    sweep(k, 66, 72, 400, 16000, 1.8)

duck_bass = S.sidechain(N, kicks, depth=0.55, release=0.14, power=1.5)
duck_mid = S.sidechain(N, kicks, depth=0.25, release=0.18)

GAIN = {  # dB
    'kick': -4.0, 'snare': 1.0, 'tops': -2.0, 'brk': -5.0, 'fx': -12.0,
    'sub': -10.0, 'reese': -2.0, 'rhodes': -3.0, 'harp': -5.0, 'flute': 3.0, 'whistle': -5.0,
    'choir': -23.0, 'strings': -7.0, 'lowstr': -7.0, 'horn': -3.0, 'pad': -6.0, 'atmos': 4.0,
}
DUCK = {'sub': duck_bass, 'reese': duck_bass, 'pad': duck_mid, 'rhodes': duck_mid, 'harp': duck_mid,
        'strings': duck_mid, 'lowstr': duck_mid, 'choir': duck_mid}
ROOM = {'snare': .18, 'tops': .1, 'brk': .12, 'kick': .04}
HALL = {'snare': .08, 'rhodes': .25, 'harp': .3, 'flute': .3, 'whistle': .4, 'choir': .45, 'strings': .4,
        'lowstr': .35, 'horn': .35, 'pad': .3, 'fx': .25}
DELAY = {'flute': .16, 'harp': .2, 'rhodes': .08, 'whistle': .25}

# bus processing
bus['rhodes'].x = stage(bus['rhodes'].x, HighpassFilter(110), PeakFilter(250, -2.5, 0.8),
                        Chorus(rate_hz=0.6, depth=0.18, centre_delay_ms=6, mix=0.35))
# Rhodes tremolo (auto-pan, quarter-note)
_t = np.arange(N) / SR
_trem = 0.5 + 0.5 * np.sin(2 * np.pi * _t / (2 * SPB))
bus['rhodes'].x[0] *= (0.72 + 0.28 * _trem).astype(np.float32)
bus['rhodes'].x[1] *= (0.72 + 0.28 * (1 - _trem)).astype(np.float32)
del _trem, _t
bus['harp'].x = stage(bus['harp'].x, HighpassFilter(180), HighShelfFilter(6000, 2.0, 0.7))
bus['flute'].x = stage(bus['flute'].x, HighpassFilter(250), PeakFilter(2500, 1.5, 1.0),
                       Compressor(threshold_db=-24, ratio=2.5, attack_ms=15, release_ms=150))
bus['whistle'].x = stage(bus['whistle'].x, HighpassFilter(300))
bus['choir'].x = stage(bus['choir'].x, HighpassFilter(220), Chorus(rate_hz=0.3, depth=0.3, centre_delay_ms=9, mix=0.4))
bus['strings'].x = stage(bus['strings'].x, HighpassFilter(200))
bus['lowstr'].x = stage(bus['lowstr'].x, HighpassFilter(90))
bus['horn'].x = stage(bus['horn'].x, HighpassFilter(90))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(200), Chorus(rate_hz=0.25, depth=0.3, centre_delay_ms=8, mix=0.4))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(120), LowpassFilter(8000),
                       Compressor(threshold_db=-16, ratio=3, attack_ms=3, release_ms=80))
bus['tops'].x = stage(bus['tops'].x, HighpassFilter(4000), LowpassFilter(12500))
bus['brk'].x = stage(bus['brk'].x, HighpassFilter(220))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(40))
bus['reese'].x = stage(bus['reese'].x, HighpassFilter(90), LowpassFilter(2500),
                       Chorus(rate_hz=0.2, depth=0.2, centre_delay_ms=5, mix=0.3))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['atmos'].x = stage(bus['atmos'].x, HighpassFilter(300))
# the vinyl bed comes forward in the quiet parts
_bars = (np.arange(N) / SR + T0) / (4 * SPB)
_at = np.interp(_bars, [0, 8, 16, 16.01, 56, 56.01, 64, 72, 104, 116, 117],
                [1.0, 0.7, 0.6, 0.25, 0.25, 0.9, 0.9, 0.3, 0.3, 0.4, 0.0])
bus['atmos'].x *= _at.astype(np.float32)[None, :]
del _at

mix = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('SONG3_MUTE', '').split(',')))
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        continue
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= DUCK[k][None, :]
    if np.any(x):
        levels[k] = pyln.Meter(SR).integrated_loudness(x.T.astype(np.float64))
        if os.environ.get('SONG3_HF'):
            i0, i1 = int(max(0, T(20)) * SR), int(max(0, T(28)) * SR)
            if i1 > i0:
                Y = np.abs(np.fft.rfft(x[0, i0:i1])) ** 2
                f = np.fft.rfftfreq(i1 - i0, 1 / SR)
                print(f'  HF {k:8s} 10k+ {10 * np.log10(Y[f > 10000].sum() + 1e-20):6.1f} dB  '
                      f'2.5-5k {10 * np.log10(Y[(f > 2500) & (f < 5000)].sum() + 1e-20):6.1f} dB')
    mix += x
    if k in ROOM:
        room_send += x * ROOM[k]
    if k in HALL:
        hall_send += x * HALL[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

room = stage(room_send, HighpassFilter(250), LowpassFilter(8000),
             Reverb(room_size=0.3, damping=0.5, wet_level=1.0, dry_level=0.0, width=0.8))
del room_send
pre = int(0.03 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
del hall_send
hall = stage(hs, HighpassFilter(300), LowpassFilter(8500),
             Reverb(room_size=0.9, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del hs
hall *= duck_mid[None, :]
dly = S.pingpong(delay_send, B(0.75), feedback=0.4, repeats=7, lp=4500, hp=400)
del delay_send
dly = stage(dly, Reverb(room_size=0.5, damping=0.6, wet_level=0.3, dry_level=1.0, width=1.0))
mix += room * 0.9 + hall * 0.8 + dly * 0.55
del room, hall, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -1.5), (8, -2.5), (15.9, -2.5), (16, 0), (47.9, 0), (48, -1), (55.9, -1.5), (56, -5.5),
              (63.9, -5), (64, -5), (71.8, -2.5), (72, 0.5), (103.9, 0.8), (104, -1.0), (115.9, -1.5),
              (116, -1.5), (130, -1.5)]
sec = np.interp(_bars, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(25))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.5 - loud) / 20))
mix = stage(mix, LowShelfFilter(80, 0.5, 0.7), HighShelfFilter(5000, 1.5, 0.7),
            Compressor(threshold_db=-16, ratio=2.0, attack_ms=25, release_ms=180))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=60))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
if RB1 == NBARS:
    last = min(last, int((T(116) + RING) * SR))
mix = mix[:, : last + 1]
fade = int(0.8 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song3_liquid.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
