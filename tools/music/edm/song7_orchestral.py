#!/usr/bin/env python3
# song7_orchestral.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 7, Epic Orchestral": a hybrid orchestral / movie-trailer remix of
# sf-music7 (the original game's music by Stardock, from mid/sf-music7.mid),
# produced in code: a sampled orchestra (synth_orch.Sampler) plus synthesized
# hybrid layers (braams, sub booms, pulse ostinato, risers), mixed and
# mastered here.
#
# Samples: VSCO 2 Community Edition (Versilian Studios; Sam Gossner, Simon
# Dalzell, Elan Hickler), CC0 1.0 / public domain,
# https://github.com/sgossner/VSCO-2-CE: violin/viola/cello sections, solo
# contrabass, F horn, trumpet, tenor trombone, tuba, piccolo, flute, oboe,
# clarinet, bassoon, timpani, concert bass drum, giant ethnic drum, tenor
# drums, snare, cymbals, gong.  Everything else is synthesized (synth.py,
# synth_orch.py).
#
# Taken from the original song (the MIDI starts with a two-beat pickup; bar
# numbers below count from the bass's first downbeat):
#   - the 3-3-2 ostinato      0 .75 1.5 1.75 2 2.75 3.5 (3.75) on C2/C3
#                             (fretless bass, horns, trombones, bars 5-62),
#                             here on spiccato celli/basses, staccato horns
#                             and a synth pulse
#   - the intro flourish      piccolo G5 Ab5 G5 Ab5 G5 | G F Eb Db C Ab | G4 |
#                             Ab Bb C Db Eb F, trumpets Eb5/G5 calls, the
#                             timpani C3/Db2 3-3-2 figure (bars 1-4)
#   - the ostinato section    strings C4 | F4 Db4 | C4, piccolo G6 F6 Eb6 F6 G6,
#                             bassoon G F Ab G, piccolo G5 F5 Eb5 Db5 C5 and
#                             the trumpet call G4C5 C5G5 ... C5Eb5 (bars 5-12)
#   - the main fanfare        C C C G~ C C G C' | Bb Ab F# F Eb Db C Bb with
#                             its parallel line a third below, trumpets
#                             C5G5 | F5Ab5 Db5F5 | C5G5 | Db5F5 | G5 | Ab5 and
#                             the horn counter G3 G4 Ab4 | F4 (bars 13-18,
#                             57-62); C to Db "Phrygian planing"
#   - the oboe / bassoon / clarinet C-minor interlude (bars 19-34)
#   - the F minor / Gb section: string figure Ab G F Ab G F, Bb Ab Gb ...,
#                             choir line C4 Eb4 Ab4 Bb4 Gb4, hits, and the
#                             G-pedal turn C5 Bb4 | A4 Ab4 (bars 35-48)
#   - the C-minor coda        strings G5 C5 G5 C5 G5 C5 | Ab5 C5 F5 over the
#                             rising bass C D Eb F, trumpet calls G5C6, the
#                             G Eb D turn to the final C (bars 63-72)
#   - the snare march         3-3-2 / 8ths / fanfare patterns and its rolls
# Fixed: the piccolo G5 against the trumpets' Ab5 over the Db chord (bars 18
# and 62) is Ab5, as the parallel figure in bar 16 implies.
# New: 150 BPM (from 155), a trailer form (mysterious F-minor opening, the
# flourish, the ostinato build, fanfare, interlude, tension build, the full
# fanfare twice as the climax with C-major colour on the second pass, coda,
# a final C-major chord and hit), seating, a designed hall, the hybrid layer.
#
#   python3 tools/music/edm/song7_orchestral.py OUT.wav [FROM_BAR TO_BAR]
#   SONG7_MUTE=taiko,snare,cym,timp,sub,fx  mutes buses (for chroma checks)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
import scipy.signal as ss
from pedalboard import (Pedalboard, Compressor, Limiter, HighpassFilter, LowpassFilter,
                        LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_orch as O  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 150.0
SPB = 60.0 / BPM
NBARS = 92
TAIL = 6.5
N = int((NBARS * 4 * SPB + TAIL) * SR)
FROM_BAR = float(sys.argv[2]) if len(sys.argv) > 3 else 0
TO_BAR = float(sys.argv[3]) if len(sys.argv) > 3 else 999
RNG = np.random.default_rng(155)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def active(bar, beat=0.0):
    b = bar + beat / 4
    return FROM_BAR - 1 <= b < TO_BAR


def notes(spec):
    if isinstance(spec, (int, np.integer)):
        return [int(spec)]
    return [m(x) for x in spec.split('+')]


# ------------------------------------------------------------- harmony

CH = {   # name: (bass root, pad voicing)
    'Fm': (41, ['F3', 'Ab3', 'C4']),
    'Gb': (42, ['Gb3', 'Bb3', 'Db4', 'F4']),
    'G': (43, ['G3', 'D4', 'G4']),
    'C5': (36, ['C4', 'G4', 'C5']),
    'C5/G': (43, ['C4', 'G4', 'C5']),
    'Cm': (36, ['C4', 'Eb4', 'G4']),
    'Cm/G': (43, ['C4', 'Eb4', 'G4']),
    'C': (36, ['C4', 'E4', 'G4', 'C5']),
    'C/G': (43, ['C4', 'E4', 'G4', 'C5']),
    'Db': (37, ['Db4', 'F4', 'Ab4']),
    'Fm/Ab': (44, ['F3', 'Ab3', 'C4']),
    'G7b9': (43, ['G3', 'Bb3', 'Db4', 'F4']),
    'Bb/D': (38, ['D4', 'F4', 'Bb4']),
    'Cm/Eb': (39, ['Eb4', 'G4', 'C5']),
    'Bb/F': (41, ['F4', 'Bb4', 'D5']),
}
FANFARE_PROG = ['C5/G', 'Gb', 'C5', 'Db', 'C5/G', 'Gb', 'C5', 'Db']
HARM = {}   # (bar, beat) -> chord
for b in range(0, 8):
    HARM[(b, 0)] = ['Fm', 'Fm', 'Gb', 'Gb'][b % 4]
for b in range(8, 12):
    HARM[(b, 0)] = 'G'
HARM.update({(12, 0): 'C5', (13, 0): 'Db', (13, 2): 'Fm', (14, 0): 'Cm/G', (15, 0): 'Fm/Ab', (15, 2): 'G7b9'})
for b in range(16, 24):
    HARM[(b, 0)] = 'C5'
HARM[(23, 0)] = 'Cm'
for i, c in enumerate(FANFARE_PROG):
    HARM[(24 + i, 0)] = c
    HARM[(64 + i, 0)] = c
    HARM[(72 + i, 0)] = c.replace('C5', 'C')
for b in range(32, 48):
    HARM[(b, 0)] = 'Cm'
for b in range(48, 60):
    HARM[(b, 0)] = ['Fm', 'Fm', 'Gb', 'Gb'][(b - 48) % 4]
for b in range(60, 64):
    HARM[(b, 0)] = 'G'
for b in range(80, 88):
    HARM[(b, 0)] = ['Cm', 'Bb/D', 'Cm/Eb', 'Bb/F'][(b - 80) % 4]
HARM[(88, 0)] = 'G'
for b in range(89, NBARS):
    HARM[(b, 0)] = 'C'
_HK = sorted(HARM)


def chord_at(bar, beat=0.0):
    key = (bar, beat)
    c = None
    for k in _HK:
        if k <= key:
            c = HARM[k]
        else:
            break
    return c


def root_at(bar, beat=0.0):
    return CH[chord_at(bar, beat)][0]


# ------------------------------------------------------------ themes
# (beat, note(s), length in beats[, accent])

BASS_BAR = [0, .75, 1.5, 1.75, 2, 2.75, 3.5, 3.75]         # the 3-3-2 ostinato (one bar)
BASS_BAR2 = [0, .75, 1.5, 1.75, 2, 2.75]                   # ... second bar, then an 8th on 3.5
SN_A = [0, .75, 1.5, 1.75, 2, 2.75, 3.5, 3.75]
SN_B = [0, .75, 1.5, 1.75, 2, 2.5, 3, 3.5]
SN_F = [0, 1, 1.25, 1.5, 2.5, 2.75, 3, 3.5]

# intro flourish (orig bars 1-4), 16 beats
FL_PICC = [(2, 'G5', .25), (2.25, 'Ab5', .25), (2.5, 'G5', .25), (2.75, 'Ab5', .25), (3, 'G5', 1),
           (4, 'G5', .75), (4.75, 'F5', .75), (5.5, 'Eb5', .5), (6, 'Db5', .75), (6.75, 'C5', .75),
           (7.5, 'Ab4', .5), (8, 'G4', 4), (12, 'Ab4', .75), (12.75, 'Bb4', .75), (13.5, 'C5', .5),
           (14, 'Db5', .75), (14.75, 'Eb5', .75), (15.5, 'F5', .5), (16, 'G5', 4)]
FL_TPT = [(9, 'Eb5+G5', .25), (9.5, 'Eb5+G5', .25), (9.75, 'Eb5+G5', .25), (10, 'Eb5+G5', 2),
          (14, 'Db5+F5', .75), (14.75, 'Eb5+G5', .75), (15.5, 'F5+Bb5', .5), (16, 'C5+G5+C6', 1.5)]
FL_HORN = [(1, 'G3', 1), (2, 'G4', 2)]
FL_HI = [(0, 'C4', 4), (4, 'Db4', 2), (6, 'F4', 2), (8, 'C4', 4), (12, 'Ab4', 2), (14, 'Bb4', 2)]
FL_MID = [(0, 'G3', 4), (4, 'Ab3', 2), (6, 'Db4', 2), (8, 'G3', 4), (12, 'F4', 2), (14, 'F4', 2)]
FL_PED = [(0, 'C6', 4), (4, 'Db6', 4), (8, 'C6', 4), (12, 'Db6', 4)]
FL_TIMP = [(0, 'C3'), (.75, 'C3'), (1.5, 'C3'), (2, 'C3'), (2.75, 'C3'), (3.5, 'G2'), (3.75, 'G2'),
           (4, 'Db2'), (4.75, 'Db2'), (5.5, 'Db2'), (6, 'Db2'), (6.75, 'Db2'), (7.5, 'Db2')]

# ostinato section (orig bars 5-12), 32 beats
OS_STR = [(0, 'C4', 4), (4, 'F4', 2), (6, 'Db4', 2), (8, 'C4', 8), (16, 'C4', 4), (20, 'Db4', 2),
          (22, 'F4', 2), (24, 'G4', 8)]
OS_FLUTE = [(0, 'G5', 8), (18, 'G5', 1), (19, 'F5', 1), (20, 'Eb5', 2), (22, 'Db5', 2), (24, 'C5', 8)]
PICC_RUN = [(0, 'G6', .25), (.25, 'F6', .25), (.5, 'Eb6', .25), (.75, 'F6', .25), (1, 'G6', 2)]
BSN_FIG = [(0, 'G3', 1), (1, 'F3', 1), (2, 'Ab3', 1), (3, 'G3', 4)]
OS_TPT = [(24.5, 'G4+C5', .5), (25, 'C5+G5', .25), (25.5, 'G4+C5', .5), (26, 'C5+G5', 1.5),
          (27.5, 'Db5+F5', .5), (28, 'C5+Eb5', 4)]
PICKUP = [(30, 'G3+G4', 1), (31, 'Bb3+Bb4', 1)]

# the fanfare (orig bars 13-18); one bar per entry, keyed by original bar
FAN_MEL = {
    13: [(0, 'C4', .25), (.5, 'C4', .25), (.75, 'C4', .25), (1, 'G4', 1.5), (2.5, 'C4', .25),
         (2.75, 'C4', .25), (3, 'G4', .5), (3.5, 'C5', .5)],
    14: [(0, 'Bb4', .5), (.5, 'Ab4', .5), (1, 'F#4', .25), (1.5, 'F4', .25), (2, 'Eb4', .5),
         (2.5, 'Db4', .5), (3, 'C4', .25), (3.5, 'Bb3', .25)],
    15: [(0, 'C4', 4)], 16: [(0, 'Db4', 4)], 17: [(0, 'C4', 4)], 18: [(0, 'Db4', 4)],
}
FAN_HARM = {
    13: [(0, 'G3', .25), (.5, 'G3', .25), (.75, 'G3', .25), (1, 'C4', 1.5), (2.5, 'G3', .25),
         (2.75, 'G3', .25), (3, 'C4', .5), (3.5, 'G4', .5)],
    14: [(0, 'F#4', .5), (.5, 'F4', .5), (1, 'Eb4', .25), (1.5, 'Db4', .25), (2, 'Bb3', .5),
         (2.5, 'Ab3', .5), (3, 'F#3', .25), (3.5, 'F3', .25)],
    15: [(0, 'G3', 4)], 16: [(0, 'Ab3', 4)], 17: [(0, 'G3', 4)], 18: [(0, 'Ab3', 4)],
}
FAN_TPT = {13: [(0, 'C5+G5', 4)], 14: [(0, 'F5+Ab5', 2), (2, 'Db5+F5', 2)], 15: [(0, 'C5+G5', 4)],
           16: [(0, 'Db5+F5', 4)], 17: [(0, 'C5+G5', 4)], 18: [(0, 'Db5+Ab5', 4)]}
FAN_CTR = {13: [(1, 'G3', 1), (2, 'G4', 1), (3, 'Ab4', 1)], 14: [(0, 'F4', 2), (2, 'F4', 2)],
           15: [(0, 'G4', 4)], 16: [(0, 'Db4', 4)], 17: [(0, 'C4', 4)], 18: [(0, 'Db4', 4)]}
# piccolo: pickup G5 F5 -> Db5/Db6 (bar 16); pickup Bb5 Ab5 -> Db5/Ab5 (bar 18, G5 fixed to Ab5)
FAN_PICC = {15: [(3.5, 'G5', .25), (3.75, 'F5', .25)], 16: [(0, 'Db5+Db6', 4)],
            17: [(0, 'C5+C6', 3.5), (3.5, 'Bb5', .25), (3.75, 'Ab5', .25)], 18: [(0, 'Db5+Ab5', 4)]}
FAN_PED = {13: 'C6', 14: 'Db6', 15: 'C6', 16: 'Db6', 17: 'C6', 18: 'Db6'}
FAN_ORDER = [13, 14, 15, 16, 13, 14, 17, 18]
FAN_LOW = [(0, 1.5), (1.5, 1.5), (3, 1)]      # the low strings' 3-3-2 in quarters (bars 13-14)

# interlude (orig bars 19-34)
OBOE = [(0, 'G4', 1), (1, 'F4', 1), (2, 'G4', 5), (7, 'F4', 1)]
CLAR = [(0, 'Eb4', 1), (1, 'D4', 1), (2, 'Eb4', 5), (7, 'D4', 1), (8, 'Eb4', 8)]
CALL = [(3, 'C5+G5', .25), (3.5, 'C5+G5', .25), (3.75, 'C5+G5', .25), (4, 'C5+G5', 3),
        (11, 'C5+G5', .25), (11.5, 'C5+G5', .25), (11.75, 'C5+G5', .25), (12, 'C5+G5', 3.5)]
TAIKO_FIG = [(5.5, .25), (5.75, .25), (6, .5), (6.5, .5), (13.5, .25), (13.75, .25), (14, 1)]
INT_END_BSN = [(1, 'G3', 1), (2, 'C3', 1), (3, 'D3', 1), (4, 'Eb3', 4)]

# F minor / Gb section (orig bars 35-48), per 4 bars
FIG_F = [(0, 'Ab4', .5), (.5, 'G4', .5), (1, 'F4', .5), (1.5, 'Ab4', 1), (2.5, 'G4', .5), (3, 'F4', 1)]
FIG_GB = [(0, 'Bb4', .5), (.5, 'Ab4', .5), (1, 'Gb4', .5), (1.5, 'Bb4', 1), (2.5, 'Ab4', .5), (3, 'Gb4', 1)]
FIG_GB2 = FIG_GB[:5] + [(3, 'Gb4', .5), (3.5, 'Bb4', .5)]
FIG4 = FIG_F + [(b + 4, n, l) for b, n, l in FIG_F] + [(b + 8, n, l) for b, n, l in FIG_GB] + \
    [(b + 12, n, l) for b, n, l in FIG_GB2]
HIGH4 = [(0, 'C6', 6), (6, 'Eb6', 2), (8, 'F6', 8)]
CHOIR_LINE = [(0, 'C4', 4), (4, 'Eb4', 2), (6, 'Ab4', 2), (8, 'Bb4', 4), (12, 'Gb4', 4)]
CHOIR_LINE2 = [(0, 'C4', 4), (4, 'Eb4', 2), (6, 'Ab4', 2), (8, 'Bb4', 4), (12, 'Db4', 4)]
HITS4 = [(0, 'F4+F5+C5', 1.5), (1.5, 'C4+C5+F4', 1.5), (6, 'F4+F5+C5', 1), (7, 'C4+C5+F4', 1),
         (8, 'Gb4+Gb5+Bb4', 3)]
TURN_HI = [(0, 'C5', 1.5), (1.5, 'Bb4', 2.5), (4, 'A4', 1.5), (5.5, 'Ab4', 2.5)]
TURN_LO = [(0, 'G3', 4), (4, 'Eb3', 1.5), (5.5, 'D3', 2.5)]
TURN_MID = [(0, 'G4', 1.5), (1.5, 'G4', 2.5), (4, 'Eb4', 1.5), (5.5, 'D4', 2.5)]

# coda (orig bars 63-72)
CODA_A = [(0, 'G5', 1), (1, 'C5', .5), (1.5, 'G5', 1), (2.5, 'C5', .5), (3, 'G5', .5), (3.5, 'C5', .5),
          (4, 'Ab5', 1), (5, 'C5', .5), (5.5, 'F5', 2.5)]
CODA_B = CODA_A[:8] + [(5.5, 'F5', 1.5), (7, 'Eb5', .5), (7.5, 'F5', .5)]
CODA_IN = [(0, 'Eb4', 4), (4, 'D4+F4', 4), (8, 'Eb4', 4), (12, 'D4+F4', 4)]
CODA_PAD = [(0, 'C4', 4), (4, 'D4', 1.5), (5.5, 'Bb3', 2.5)]
CODA_CALL = [(0, 'G5+C6', .25), (.5, 'C5+G5', .25), (.75, 'C5+G5', .25), (1, 'G5+C6', 7)]
CODA_ACC = {'Cm': [(0, 'C3+G3', .5), (.5, 'C3+G3', .5), (3.5, 'C3+G3', .5)], 'Bb/D': [(0, 'D3+F3', .5)],
            'Cm/Eb': [(0, 'Eb3+G3', .5), (.5, 'Eb3+G3', .5), (3.5, 'Eb3+G3', .5)], 'Bb/F': [(0, 'F3', .5)]}
TURN_END = [(1, 'G4', 1), (2, 'Eb4', 1), (3, 'D4', 1)]

# --------------------------------------------------------------- players

smp = O.Sampler()
# seat: (pan, width); bus
SEAT = {
    'vln1': (-0.62, 0.55, 'vln'), 'vln2': (-0.3, 0.55, 'vln'), 'vla': (0.14, 0.5, 'vla'),
    'vc': (0.42, 0.5, 'lowstr'), 'cb': (0.6, 0.35, 'lowstr'),
    'horn': (-0.34, 0.45, 'horn'), 'horn2': (-0.18, 0.45, 'horn'),
    'tpt': (0.3, 0.35, 'tpt'), 'tpt2': (0.42, 0.35, 'tpt'),
    'tbn': (0.22, 0.4, 'lowbr'), 'tuba': (0.34, 0.3, 'lowbr'),
    'fl': (-0.12, 0.3, 'ww'), 'picc': (-0.2, 0.3, 'ww'), 'ob': (0.06, 0.3, 'ww'),
    'cl': (-0.04, 0.3, 'ww'), 'bsn': (0.16, 0.3, 'ww'),
}
ARTS = {
    'vln1': {'sus': 'vln_sus', 'short': 'vln_spic', 'trem': 'vln_trem', 'pizz': 'vln_pizz'},
    'vln2': {'sus': 'vln_sus', 'short': 'vln_spic', 'trem': 'vln_trem', 'pizz': 'vln_pizz'},
    'vla': {'sus': 'vla_sus', 'short': 'vla_spic', 'pizz': 'vla_pizz'},
    'vc': {'sus': 'vc_sus', 'short': 'vc_spic', 'trem': 'vc_trem', 'pizz': 'vc_pizz'},
    'cb': {'sus': 'cb_sus', 'short': 'cb_spic'},
    'horn': {'sus': 'horn_sus', 'short': 'horn_stac'}, 'horn2': {'sus': 'horn_sus', 'short': 'horn_stac'},
    'tpt': {'sus': 'tpt_sus', 'short': 'tpt_stac', 'mute': 'tpt_mute'},
    'tpt2': {'sus': 'tpt_sus', 'short': 'tpt_stac', 'mute': 'tpt_mute'},
    'tbn': {'sus': 'tbn_sus', 'short': 'tbn_stac'}, 'tuba': {'sus': 'tuba_sus', 'short': 'tuba_stac'},
    'fl': {'sus': 'fl_sus'}, 'picc': {'sus': 'picc_sus'}, 'ob': {'sus': 'ob_sus'},
    'cl': {'sus': 'cl_sus'}, 'bsn': {'sus': 'bsn_sus'},
}

BUSES = ['vln', 'vla', 'lowstr', 'horn', 'tpt', 'lowbr', 'ww', 'timp', 'taiko', 'snare', 'cym',
         'synth', 'pad', 'sub', 'fx']
bus = {k: S.Bus(N) for k in BUSES}
HUM = [0.010, 0.07]      # timing sigma (s), velocity sigma; set per section


def put(name, sig, t, gain=1.0):
    bus[name].add(sig, t, gain)


def play(player, mel, bar0, transpose=0, vel=0.8, art='auto', short_max=0.5, rel=None,
         attack=0.0, legato=True, double=False, swell=None, gap=0.0, maxlen=None, pan_off=0.0):
    """Render a note list on a seated player.  art 'auto' = short articulation
    for notes up to short_max beats, sustain otherwise (legato between
    touching sustained notes)."""
    pan, width, bname = SEAT[player]
    arts = ARTS[player]
    mel = sorted(mel, key=lambda e: e[0])
    for i, ev in enumerate(mel):
        beat, spec, ln = ev[0], ev[1], ev[2]
        acc = ev[3] if len(ev) > 3 else 1.0
        if not active(bar0, beat):
            continue
        a = art
        if a == 'auto':
            a = 'short' if ('short' in arts and ln <= short_max) else 'sus'
        key = arts.get(a, arts['sus'])
        is_short = a in ('short', 'pizz')
        leg = False
        if legato and not is_short and i > 0:
            pb, _, pl = mel[i - 1][:3]
            leg = abs(pb + pl - beat) < 0.05 and pl > short_max
        nxt_leg = False
        if legato and not is_short and i + 1 < len(mel):
            nb = mel[i + 1][0]
            nxt_leg = abs(beat + ln - nb) < 0.05 and mel[i + 1][2] > short_max
        dur = B(ln) - gap * SPB
        if maxlen:
            dur = min(dur, maxlen)
        if nxt_leg:
            dur += 0.06
        r = rel if rel is not None else (0.12 if is_short else (0.07 if nxt_leg else 0.3))
        for md in notes(spec):
            md += transpose
            copies = [(0.0, 0.0, 0.0, 1.0)]
            if double:
                copies.append((0.008 + 0.012 * RNG.random(), (RNG.random() - 0.5) * 0.14,
                               0.12 * (1 if RNG.random() > 0.5 else -1), 0.85))
            for dt, det, dp, g in copies:
                v = float(np.clip(vel * acc * (1 + RNG.normal(0, HUM[1])), 0.02, 1.0))
                sig = smp.note(key, md, max(dur, 0.05), v, rel=r, attack=attack, legato=leg, detune=det)
                if swell:
                    s0 = swell
                    k = min(int(dur * SR), sig.shape[1])
                    u = np.linspace(0, 1, k, dtype=np.float32)
                    sig[:, :k] *= s0 + (1 - s0) * u ** 1.6
                t = T(bar0, beat) + dt + (0 if leg else RNG.normal(0, HUM[0]))
                t = max(t, T(bar0, beat) - 0.02)
                put(bname, O.seat(sig, float(np.clip(pan + dp + pan_off, -1, 1)), width), t, g)


def fan_part(player, bar0, table, order=FAN_ORDER, **kw):
    for i, ob in enumerate(order):
        if ob in table:
            play(player, table[ob], bar0 + i, **kw)


def pads(player, bar0, bar1, vel=0.5, transpose=0, upper=0, attack=0.25, art='sus', double=False, pick=None):
    """Sustained chords from HARM (each chord held until it changes)."""
    pts = [k for k in _HK if bar0 <= k[0] < bar1]
    if not pts or pts[0] != (bar0, 0):
        pts = [(bar0, 0)] + pts
    for i, (b, bt) in enumerate(pts):
        e = pts[i + 1] if i + 1 < len(pts) else (bar1, 0)
        ln = (e[0] - b) * 4 + (e[1] - bt)
        voic = CH[chord_at(b, bt)][1]
        if pick is not None:
            voic = [voic[j] for j in pick if j < len(voic)]
        mel = [(bt, '+'.join(voic), ln)]
        play(player, mel, b, transpose=transpose, vel=vel, art=art, attack=attack, legato=False,
             double=double)


def ostinato(player, bar0, bar1, octave=0, vel=0.8, low8=True, art='short', double=False, accent=1.0):
    """The 3-3-2 ostinato on the bass root of each bar."""
    for b in range(bar0, bar1):
        r = root_at(b) + octave
        second = (b - bar0) % 2 == 1
        hits = BASS_BAR2 if second else BASS_BAR
        mel = []
        for i, bt in enumerate(hits):
            a = accent if bt in (0, .75, 1.5, 2, 2.75) else 1.0
            mel.append((bt, r, .25, (1.0 if bt in (0, 2) else 0.82) * a))
        if second:
            lo = r - 5 if CH[chord_at(b)][0] == 36 else r
            mel.append((3.5, lo, .5, 0.95))
        play(player, mel, b, vel=vel, art=art, legato=False, double=double)


def sixteenths(player, bar0, bar1, octave=0, vel=0.6, art='short'):
    """Straight 16ths on the root with the 3-3-2 accents (drive)."""
    for b in range(bar0, bar1):
        r = root_at(b) + octave
        mel = [(i * .25, r, .2, 1.0 if (i * .25) in BASS_BAR else 0.6) for i in range(16)]
        play(player, mel, b, vel=vel, art=art, legato=False)


# --------------------------------------------------------------- hybrid

_pc = {}


def pulse_ost(bar0, bar1, vel=0.6, octave=0, cut=700.0, env=1800.0):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        r = root_at(b) + octave
        for i in range(16):
            bt = i * .25
            acc = bt in BASS_BAR
            md = r + (12 if (i % 4 == 2 and not acc) else 0)
            key = (md, acc, cut, env)
            if key not in _pc:
                _pc[key] = O.pulse(md, B(.2), 1.0, cut=cut, env=env * (1.4 if acc else 0.8),
                                   decay=0.08 if acc else 0.05)
            put('synth', _pc[key], T(b, bt), vel * (1.0 if acc else 0.55))


def braam_at(bar, beat=0.0, dur_beats=4, vel=1.0, chord=None, brass=True):
    if not active(bar, beat):
        return
    r = CH[chord or chord_at(bar, beat)][0]
    r = r - 12 if r >= 41 else r      # keep the root around C2..E2
    ms = [r, r + 7, r + 12]
    put('synth', O.braam(ms, B(dur_beats), vel), T(bar, beat))
    if brass:
        for md in (r, r + 7):
            sig = smp.note('tbn_sus', md + 12 if md < 40 else md, B(dur_beats), min(1.0, vel), rel=0.4)
            put('lowbr', O.seat(sig, 0.22, 0.4), T(bar, beat), 0.8)
        sig = smp.note('tuba_sus', r, B(dur_beats), min(1.0, vel), rel=0.4)
        put('lowbr', O.seat(sig, 0.3, 0.3), T(bar, beat), 0.8)
        for md in (r + 12, r + 19):
            sig = smp.note('horn_sus', md, B(dur_beats), min(1.0, vel), rel=0.4)
            put('horn', O.seat(sig, -0.3, 0.4), T(bar, beat), 0.7)


def boom_at(bar, beat=0.0, vel=1.0):
    if active(bar, beat):
        put('sub', O.boom(vel), T(bar, beat))


def taiko_ens(bar, beat, vel=0.8, players=3, synth=True, spread=0.7):
    """Taiko ensemble: several sampled drums with a few ms of spread, plus a
    synthesized body layer."""
    if not active(bar, beat):
        return
    t = T(bar, beat)
    for k in range(players):
        dt = abs(RNG.normal(0, 0.006)) if k else 0.0
        p = (k / max(players - 1, 1) * 2 - 1) * spread if players > 1 else 0.0
        v = float(np.clip(vel * (1 + RNG.normal(0, 0.06)), 0, 1))
        src = ('taiko', 'taiko_hand', 'taiko_stick', 'taiko')[k % 4]
        put('taiko', O.seat(O.perc(src, v), p, 0.5), t + dt, 0.9 if src != 'taiko_stick' else 0.5)
    if vel > 0.55:
        put('taiko', O.seat(O.perc('bd', vel * 0.9), 0.0, 0.6), t, 0.7)
    if synth and vel >= 0.6:
        key = ('tk', round(vel, 2))
        if key not in _pc:
            _pc[key] = O.taiko_synth(1.0, 65.41)   # tuned to C2
        put('taiko', _pc[key], t, vel * 0.5)


def bass_drum(bar, beat, vel=0.8):
    if active(bar, beat):
        put('taiko', O.seat(O.perc('bd', vel), 0.0, 0.6), T(bar, beat), 1.0)


def snare_pat(bar0, bar1, pats, vel=0.5, roll_last=False):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        pat = pats[(b - bar0) % len(pats)]
        for i, bt in enumerate(pat):
            v = vel * (1.0 if bt in (0, 2) else (0.85 if bt in (.75, 1.5, 2.75) else 0.7))
            v = float(np.clip(v * (1 + RNG.normal(0, 0.08)), 0, 1))
            put('snare', O.seat(O.perc('snare', v), 0.12, 0.5), T(b, bt) + RNG.normal(0, 0.004))


def snare_roll(bar, beat, beats, v0=0.2, v1=0.9, div=8):
    for i in range(int(beats * div)):
        bt = beat + i / div
        u = i / max(beats * div - 1, 1)
        if not active(bar, bt):
            continue
        v = v0 + (v1 - v0) * u ** 1.4
        put('snare', O.seat(O.perc('snare', v), 0.12 + 0.1 * np.sin(i), 0.5),
            T(bar, bt) + RNG.normal(0, 0.003), 0.9)


def toms(bar, hits, vel=0.8):
    """hits: (beat, 'hi'|'lo')"""
    for bt, w in hits:
        if active(bar, bt):
            src = 'tenor_hi' if w == 'hi' else 'tenor_lo'
            p = -0.25 if w == 'hi' else 0.2
            put('taiko', O.seat(O.perc(src, vel), p, 0.5), T(bar, bt), 0.8)
            if w == 'lo':
                put('taiko', O.seat(O.perc('taiko_stick', vel * 0.9), p, 0.4), T(bar, bt), 0.4)


def crash(bar, beat=0.0, vel=0.9, kind='crash'):
    if active(bar, beat):
        put('cym', O.seat(O.perc(kind, vel), 0.0, 1.0), T(bar, beat))


def gong(bar, beat=0.0, vel=0.9):
    if active(bar, beat):
        put('cym', O.seat(O.perc('gong', vel, maxlen=12.0), -0.15, 0.8), T(bar, beat), 0.8)


def timp(bar, hits, vel=0.8):
    for bt, spec in hits:
        if active(bar, bt):
            v = float(np.clip(vel * (1 + RNG.normal(0, 0.06)), 0, 1))
            put('timp', O.seat(O.timpani(m(spec), v), -0.08, 0.6), T(bar, bt) + RNG.normal(0, 0.004))


ROLL_PITCH = {1: 41.9, 2: 47.2, 3: 49.0, 4: 52.3, 5: 55.0}


def timp_roll(bar, beat, beats, spec, v0=0.1, v1=1.0):
    """Sampled timpani roll on a pitch, faded in over its length (crescendo)."""
    if not active(bar, beat):
        return
    md = m(spec)
    drum = min(ROLL_PITCH, key=lambda k: abs(ROLL_PITCH[k] - md))
    f = f'Percussion/Timpani/Rolls/Timpani{drum}_Roll_v5_rr1_Sum.wav'
    if drum == 5:
        f = 'Percussion/Timpani/Rolls/Timpani5_Roll_v4_rr1_Sum.wav'
    sig = O.perc_file(f, md - ROLL_PITCH[drum], maxlen=B(beats) + 0.3).copy()
    k = sig.shape[1]
    u = np.linspace(0, 1, k, dtype=np.float32)
    sig *= (v0 + (v1 - v0) * u ** 2) * 0.9
    put('timp', O.seat(sig, -0.08, 0.6), T(bar, beat))


def cym_swell(bar, beat=0.0, which='Long', vel=0.8):
    """Suspended-cymbal crescendo (sample) whose peak lands on (bar, beat)."""
    rel = f'Percussion/susCymb1-cresc-{which}_v1.wav'
    sig = O.perc_file(rel)
    env = np.abs(sig).max(0)
    pk = int(np.argmax(env))
    t = T(bar, beat) - pk / SR
    if active(bar, beat) and t >= 0:
        put('cym', O.seat(sig[:, :pk + int(0.25 * SR)], 0.25, 1.0) * 0.6, t, vel)


def rev_crash(bar, beat=0.0, beats=4, vel=0.6):
    """Reversed cymbal into a downbeat."""
    if not active(bar, beat):
        return
    c = O.perc('crash', 0.95)
    L = min(int(B(beats) * SR), c.shape[1])
    r = c[:, :L][:, ::-1].copy()
    r *= np.linspace(0, 1, L, dtype=np.float32) ** 2
    put('fx', r, T(bar, beat) - L / SR, vel)


def riser(bar, beat, beats, vel=0.5, f0=300, f1=9000):
    if active(bar, beat):
        put('fx', S.noise_riser(B(beats), f0, f1, 2.0).astype(np.float32), T(bar, beat), vel)


def whoosh_at(bar, beat, beats=4, vel=0.5):
    """Whoosh centred on (bar, beat)."""
    if active(bar, beat):
        put('fx', O.whoosh(B(beats)), T(bar, beat) - B(beats) / 2, vel)


def choir_pad(bar0, bar1, vel=0.5, transpose=0, voic=None):
    pts = [k for k in _HK if bar0 <= k[0] < bar1]
    for i, (b, bt) in enumerate(pts):
        if not active(b, bt):
            continue
        e = pts[i + 1] if i + 1 < len(pts) else (bar1, 0)
        ln = (e[0] - b) * 4 + (e[1] - bt)
        for nn in (voic or CH[chord_at(b, bt)][1]):
            put('pad', O.choir(m(nn) + transpose, B(ln), 1.0), T(b, bt), vel)


def choir_line(mel, bar0, vel=0.5, transpose=0):
    for bt, spec, ln in mel:
        if active(bar0, bt):
            for md in notes(spec):
                put('pad', O.choir(md + transpose, B(ln) + 0.1, 1.0, attack=0.25), T(bar0, bt), vel)


def piano_fig(mel, bar0, vel=0.5, transpose=0):
    for bt, spec, ln in mel:
        if active(bar0, bt):
            for md in notes(spec):
                put('pad', S.piano(md + transpose, B(ln) + 0.4, vel=0.55).astype(np.float32),
                    T(bar0, bt) + RNG.normal(0, 0.006), vel)


def drone(bar0, bar1, vel=0.5):
    """Low sub-bass drone on the bass root (synthesized)."""
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b
        while e < bar1 and chord_at(e) == c:
            e += 1
        if active(b):
            r = CH[c][0] - 12
            y = S.sub_bass(r, T(e) - T(b) + 0.1, 1.0, a=0.4, r=0.4, drive=1.1).astype(np.float32)
            put('sub', y, T(b), vel)
        b = e


# ---------------------------------------------------------- arrangement

# ===== S0  OPENING, F minor / Gb, mysterious (bars 0-11)
HUM[:] = [0.012, 0.08]
drone(0, 12, 0.22)
for b0 in (0, 4):
    piano_fig(FIG4, b0, vel=0.55 if b0 == 0 else 0.45, transpose=-12)
play('cb', [(0, 'F1', 8), (8, 'Gb1', 8), (16, 'F1', 8), (24, 'Gb1', 8), (32, 'G1', 16)], 0, vel=0.25,
     attack=1.0, legato=True)
play('vc', [(0, 'F2', 8), (8, 'Gb2', 8), (16, 'F2', 8), (24, 'Gb2', 8)], 0, vel=0.2, attack=1.2,
     legato=True)
play('vln1', [(b, n, l) for b, n, l in HIGH4], 0, vel=0.12, art='trem', attack=1.5, legato=False)
play('vln1', [(b, n, l) for b, n, l in HIGH4], 4, vel=0.18, art='trem', attack=1.0, legato=False)
play('vln2', FIG4, 4, vel=0.35, art='pizz')
play('vla', FIG4, 4, vel=0.3, art='pizz', transpose=-12)
play('horn', CHOIR_LINE, 4, vel=0.3, attack=0.4)
choir_pad(4, 8, vel=0.25)
for b in range(4, 8):
    taiko_ens(b, 0, vel=0.35, players=2)
    if b % 2 == 1:
        taiko_ens(b, 2.5, vel=0.25, players=1, synth=False)
# the G turn, twice, growing (orig 47-48)
play('vln1', TURN_HI, 8, vel=0.45)
play('vln2', TURN_HI, 8, vel=0.4, transpose=-12)
play('vc', TURN_LO, 8, vel=0.45)
play('cb', [(0, 'G1', 8)], 8, vel=0.4)
play('horn', TURN_MID, 8, vel=0.4, transpose=-12)
play('vln1', TURN_HI, 10, vel=0.75, transpose=12, double=True)
play('vln2', TURN_HI, 10, vel=0.7, double=True)
play('vla', TURN_MID, 10, vel=0.65, transpose=-12)
play('vc', TURN_LO, 10, vel=0.7)
play('horn', TURN_MID, 10, vel=0.7, transpose=-12, double=True)
play('tbn', TURN_LO, 10, vel=0.6)
choir_line([(0, 'G4', 4), (4, 'Eb4', 1.5), (5.5, 'D4', 2.5)], 10, vel=0.4)
for b in (8, 9, 10, 11):
    ostinato('tbn', b, b + 1, octave=-12 + 12, vel=0.25 + 0.12 * (b - 8))
    taiko_ens(b, 0, vel=0.45 + 0.1 * (b - 8), players=3)
    taiko_ens(b, 2, vel=0.35 + 0.1 * (b - 8), players=2)
timp_roll(10, 0, 8, 'G2', 0.05, 0.9)
snare_roll(11, 0, 4, 0.1, 0.7, div=6)
toms(11, [(2, 'hi'), (2.5, 'hi'), (3, 'lo'), (3.25, 'lo'), (3.5, 'lo'), (3.75, 'lo')], 0.8)
cym_swell(12, 0, 'Median', 0.8)
rev_crash(12, 0, 4, 0.5)
riser(9, 0, 12, 0.35, 200, 8000)

# ===== S1  FLOURISH (orig bars 1-4), tutti (bars 12-15)
HUM[:] = [0.008, 0.06]
boom_at(12, 0, 1.0)
gong(12, 0, 0.9)
crash(12, 0, 0.95, 'clash')
crash(14, 0, 0.7)
braam_at(12, 0, 4, 0.9, chord='C5')
play('vln1', FL_PICC, 12, vel=0.85, short_max=0.3, double=True)
play('vln2', FL_PICC, 12, vel=0.8, transpose=-12, short_max=0.3, double=True)
play('fl', FL_PICC, 12, vel=0.8, short_max=0)
play('picc', [e for e in FL_PICC if e[0] < 16], 12, vel=0.6, transpose=12, short_max=0)
play('vla', FL_PICC, 12, vel=0.7, transpose=-12, short_max=0.3)
play('tpt', FL_TPT, 12, vel=0.85, short_max=0.3)
play('horn', FL_HORN, 12, vel=0.9, double=True)
play('horn', FL_HI, 12, vel=0.85, double=True)
play('horn2', FL_MID, 12, vel=0.8)
play('tbn', FL_MID, 12, vel=0.75, transpose=-12)
play('picc', FL_PED, 12, vel=0.5, transpose=-12)
play('vln1', FL_PED, 12, vel=0.4, art='trem')
ostinato('vc', 12, 13, vel=0.9, double=True)
ostinato('cb', 12, 13, octave=0, vel=0.9)
ostinato('vc', 14, 15, vel=0.9, double=True)
ostinato('cb', 14, 15, vel=0.9)
play('vc', [(0, 'Db2', 2), (2, 'F2', 2)], 13, vel=0.85, double=True)
play('cb', [(0, 'Db2', 2), (2, 'F2', 2)], 13, vel=0.85)
play('vc', [(0, 'Ab2', 2), (2, 'G2', 2)], 15, vel=0.85, double=True)
play('cb', [(0, 'Ab1', 2), (2, 'G1', 2)], 15, vel=0.85)
play('tuba', [(0, 'C2', 4), (4, 'Db2', 2), (6, 'F1', 2), (8, 'G1', 4), (12, 'Ab1', 2), (14, 'G1', 2)], 12,
     vel=0.85)
for b0 in (12, 14):
    timp(b0, [(bt, sp) for bt, sp in FL_TIMP], 0.9)
    for bt in (0, .75, 1.5, 2, 2.75):
        taiko_ens(b0, bt, vel=0.9 if bt in (0, 2) else 0.7, players=3)
    taiko_ens(b0 + 1, 0, vel=0.85, players=3)
    taiko_ens(b0 + 1, 2, vel=0.75, players=3)
    bass_drum(b0, 0, 0.9)
    # orchestra hits on beat 3 (orig ch9): staccato tutti C
    for p, md in (('tpt', 'C5+G5'), ('horn2', 'C4+G4'), ('tbn', 'C3+G3')):
        play(p, [(3, md, .5)], b0, vel=0.85, art='short')
    play('vln2', [(3, 'C5+G5', .25)], b0, vel=0.8, art='short')
snare_pat(12, 16, [SN_A, SN_B], vel=0.45)
toms(15, [(3, 'hi'), (3.25, 'hi'), (3.5, 'lo'), (3.75, 'lo')], 0.85)
pads('vla', 13, 16, vel=0.5, pick=[0, 1], transpose=0)

# ===== S2  OSTINATO (orig bars 5-12), building (bars 16-23)
HUM[:] = [0.009, 0.07]
crash(16, 0, 0.75)
boom_at(16, 0, 0.6)
ostinato('vc', 16, 24, vel=0.7, double=True)
ostinato('cb', 16, 24, vel=0.7)
ostinato('horn2', 20, 24, octave=12, vel=0.45)
ostinato('tbn', 18, 24, octave=12, vel=0.4)
pulse_ost(16, 24, vel=0.4, octave=12, cut=500, env=1400)
play('vla', OS_STR, 16, vel=0.45, attack=0.2)
play('vln2', OS_STR, 16, vel=0.4, transpose=12, attack=0.2)
play('vln1', [(0, 'C6', 16), (16, 'C6', 16)], 16, vel=0.3, attack=0.6, legato=False)
play('fl', OS_FLUTE, 16, vel=0.75)
play('horn', [(18, 'G4', 1), (19, 'F4', 1), (20, 'Eb4', 2), (22, 'Db4', 2), (24, 'C4', 6)], 16, vel=0.6)
play('picc', PICC_RUN, 19, transpose=0, vel=0.75, short_max=0)
play('fl', PICC_RUN, 19, transpose=-12, vel=0.6, short_max=0)
play('bsn', BSN_FIG, 19, vel=0.8)
play('cl', BSN_FIG, 19, vel=0.4, transpose=12)
play('tpt', OS_TPT, 16, vel=0.75, short_max=0.3)
play('tpt2', [(28, 'C5+Eb5', 4)], 16, vel=0.6)
play('horn', PICKUP, 16, vel=0.9, double=True)
play('tbn', PICKUP, 16, vel=0.8, transpose=-12)
timp(23, [(i * .5, 'C3') for i in range(8)], 0.75)
snare_pat(16, 23, [SN_A, SN_B], vel=0.3)
snare_roll(23, 0, 4, 0.25, 0.85, div=8)
for b in range(16, 24):
    taiko_ens(b, 0, vel=0.7, players=3)
    taiko_ens(b, 2, vel=0.5, players=2)
    if b % 2 == 1:
        taiko_ens(b, 3.5, vel=0.4, players=2, synth=False)
        taiko_ens(b, 3.75, vel=0.45, players=2, synth=False)
    if b % 2 == 0:
        bass_drum(b, 0, 0.6)
cym_swell(24, 0, 'Short', 0.7)
riser(21, 0, 12, 0.35, 300, 10000)
rev_crash(24, 0, 2, 0.4)


# ===== S3  FANFARE 1 (orig 13-18, 13-14, 17-18) (bars 24-31)
def fanfare(bar0, big=False, major=False):
    v = 1.0 if big else 0.85
    # melody: horns (doubled), trombones 8vb, violins 8va, violas
    fan_part('horn', bar0, FAN_MEL, vel=v, double=True, short_max=0.3)
    fan_part('tbn', bar0, FAN_MEL, vel=v * 0.9, transpose=-12, short_max=0.3)
    fan_part('vln1', bar0, FAN_MEL, vel=v * 0.9, transpose=12, double=big, short_max=0.3)
    fan_part('vla', bar0, FAN_MEL, vel=v * 0.8, short_max=0.3)
    fan_part('horn2', bar0, FAN_HARM, vel=v * 0.85, short_max=0.3)
    fan_part('vln2', bar0, FAN_HARM, vel=v * 0.8, transpose=12, short_max=0.3)
    if big:
        # trumpets on the tune in pass 2, chords otherwise
        if major:
            fan_part('tpt', bar0, FAN_MEL, vel=1.0, transpose=12, short_max=0.3, double=True)
            fan_part('tpt2', bar0, FAN_TPT, vel=0.8)
            fan_part('picc', bar0, FAN_MEL, vel=0.7, transpose=24, short_max=0)
            fan_part('fl', bar0, FAN_MEL, vel=0.7, transpose=12, short_max=0)
        else:
            fan_part('tpt', bar0, FAN_TPT, vel=0.95, double=True)
            fan_part('fl', bar0, FAN_PICC, vel=0.75, short_max=0)
            fan_part('picc', bar0, FAN_PICC, vel=0.65, transpose=12, short_max=0)
        fan_part('tuba', bar0, {k: [(0, CH[c][0] - (12 if CH[c][0] >= 40 else 0), 4)] for k, c in
                                zip([13, 14, 15, 16, 17, 18], ['C5/G', 'Gb', 'C5', 'Db', 'C5', 'Db'])}, vel=0.8)
    else:
        fan_part('tpt', bar0, FAN_TPT, vel=0.7)
        fan_part('fl', bar0, FAN_PICC, vel=0.6, short_max=0)
    fan_part('vc', bar0, FAN_CTR, vel=v * 0.85, short_max=0.3)
    fan_part('cl', bar0, FAN_CTR, vel=v * 0.6, transpose=12, short_max=0)
    for i, ob in enumerate(FAN_ORDER):
        b = bar0 + i
        play('vln1', [(0, FAN_PED[ob], 4)], b, vel=0.45 if big else 0.35, art='trem', legato=False)
        if ob in (13, 14):
            r = root_at(b)
            low = [(bt, r - 12, ln) for bt, ln in FAN_LOW]
            play('cb', low, b, vel=0.85, art='short', legato=False)
            play('tbn', [(bt, r, ln) for bt, ln in FAN_LOW], b, vel=0.8 if big else 0.65, art='short',
                 legato=False)
            timp(b, [(bt, {43: 'G2', 42: 'Gb2'}.get(r, 'C3')) for bt, _ in FAN_LOW], 0.95 if big else 0.8)
        if major and ob in (15, 17):
            for p, nn in (('horn2', 'E4'), ('vla', 'E4+G4'), ('cl', 'E5')):
                play(p, [(0, nn, 4)], b, vel=0.7, legato=False)
    # bass ostinato, pulse
    ostinato('vc', bar0, bar0 + 8, vel=0.85, double=big)
    ostinato('cb', bar0, bar0 + 8, vel=0.85)
    pulse_ost(bar0, bar0 + 8, vel=0.5 if big else 0.4, octave=12, cut=700, env=2000)
    if big:
        sixteenths('vc', bar0, bar0 + 8, octave=12, vel=0.4)
    # drums
    for i in range(8):
        b = bar0 + i
        ob = FAN_ORDER[i]
        taiko_ens(b, 0, vel=1.0 if big else 0.85, players=4 if big else 3)
        taiko_ens(b, 2, vel=0.85 if big else 0.7, players=4 if big else 3)
        if ob in (13, 14):
            taiko_ens(b, 1.5, vel=0.8, players=3)
            taiko_ens(b, 3, vel=0.75, players=3)
        else:
            taiko_ens(b, 3.5, vel=0.6, players=2, synth=False)
            taiko_ens(b, 3.75, vel=0.65, players=2, synth=False)
        bass_drum(b, 0, 0.85)
        if ob in (15, 16, 17, 18) and (big or ob in (15, 17)):
            braam_at(b, 0, 4, 0.85 if big else 0.6, brass=big)
    snare_pat(bar0, bar0 + 8, [SN_F, SN_F, SN_A, SN_B], vel=0.55 if big else 0.42)
    for b in (bar0, bar0 + 4):
        crash(b, 0, 0.95 if big else 0.8, 'clash' if big else 'crash')
        boom_at(b, 0, 1.0 if big else 0.75)
    snare_roll(bar0 + 7, 2, 2, 0.3, 0.9 if big else 0.7, div=8)
    toms(bar0 + 3, [(3, 'hi'), (3.25, 'hi'), (3.5, 'lo'), (3.75, 'lo')], 0.8)


HUM[:] = [0.008, 0.06]
fanfare(24)

# ===== S4  INTERLUDE, C minor (orig 19-34) (bars 32-47)
HUM[:] = [0.012, 0.08]
crash(32, 0, 0.5, 'sus_hit')
drone(32, 48, 0.06)
pulse_ost(32, 48, vel=0.32, octave=0, cut=350, env=900)
play('cb', [(0, 'C2', 32), (32, 'C2', 32)], 32, vel=0.3, attack=0.8, legato=False)
play('vc', [(0, 'C3+G3', 32), (32, 'C3+G3', 32)], 32, vel=0.25, attack=1.0, legato=False)
play('vla', [(0, 'C4+G4', 16), (16, 'C4+G4', 16), (32, 'C4', 16), (48, 'C4', 16)], 32, vel=0.25, attack=1.0,
     legato=False)
play('vln1', [(0, 'C6', 16), (32, 'C6', 16), (48, 'C6', 16)], 32, vel=0.2, attack=1.5, legato=False)
play('vln2', [(32, 'G4', 16), (48, 'G4', 14)], 32, vel=0.25, attack=1.0, legato=False)
for b0 in (32, 36, 40, 44):
    play('ob', OBOE, b0, vel=0.75)
for b0 in (34, 38):
    play('cl', [(0, 'G4', 8)], b0, vel=0.35, attack=0.4)
    play('bsn', BSN_FIG, b0, transpose=0, vel=0.7, rel=0.4)
for b0 in (40, 44):
    play('cl', CLAR, b0, vel=0.65)
for b0 in (32, 34, 36, 38, 40, 42, 44):
    play('tpt', CALL[:4], b0, vel=0.45 if b0 < 40 else 0.55, art='mute', legato=False)
for b0 in (32, 36, 40, 44):
    for bt, ln in TAIKO_FIG:
        taiko_ens(b0, bt, vel=0.3 if b0 < 40 else 0.45, players=2, synth=False)
ostinato('vc', 40, 48, vel=0.4)
ostinato('horn2', 44, 48, octave=12, vel=0.35)
play('bsn', INT_END_BSN, 46, vel=0.8)
play('ob', [(0, 'G4', 8)], 46, vel=0.7)
play('cl', [(0, 'Eb4', 8)], 46, vel=0.6)
play('vln1', [(6, 'F5', 1), (7, 'G5', 1)], 46, vel=0.7)
play('vln2', [(6, 'F4', 1), (7, 'G4', 1)], 46, vel=0.65)
snare_pat(44, 47, [SN_A, SN_B], vel=0.22)
snare_roll(47, 0, 4, 0.15, 0.8, div=8)
timp_roll(46, 0, 8, 'C3', 0.05, 0.8)
for b in range(44, 48):
    taiko_ens(b, 0, vel=0.55, players=2)
riser(44, 0, 16, 0.4, 200, 9000)
whoosh_at(48, 0, 6, 0.4)
cym_swell(48, 0, 'Median', 0.75)

# ===== S5  TENSION, F minor / Gb (orig 35-48 + 2 bars) (bars 48-63)
HUM[:] = [0.008, 0.07]
for k, b0 in enumerate((48, 52, 56)):
    v = 0.7 + 0.1 * k
    play('vln1', FIG4, b0, vel=v, transpose=12, short_max=0.6, double=True)
    play('vln2', FIG4, b0, vel=v, short_max=0.6, double=True)
    play('vla', FIG4, b0, vel=v * 0.9, short_max=0.6)
    play('fl', HIGH4, b0, vel=0.45 + 0.1 * k)
    play('vln1', HIGH4, b0, vel=0.35 + 0.1 * k, art='trem', legato=False)
    play('horn2', [(0, 'F4', 8), (8, 'Gb4', 8)], b0, vel=0.55 + 0.1 * k, legato=False)
    for bt, spec, ln in HITS4:
        for p in ('tpt', 'horn'):
            play(p, [(bt, spec, min(ln, .5))], b0, vel=0.85 + 0.05 * k, art='short')
        taiko_ens(b0, bt, vel=0.85, players=3)
        timp(b0, [(bt, 'F2' if bt < 8 else 'Gb2')], 0.9)
    ostinato('vc', b0, b0 + 4, vel=0.75 + 0.05 * k, double=True)
    ostinato('cb', b0, b0 + 4, vel=0.75 + 0.05 * k)
    play('cb', [(0, 'F1', 8), (8, 'Gb1', 8)], b0, vel=0.55 + 0.1 * k, legato=False)
    play('vc', [(0, 'F2', 8), (8, 'Gb2', 8)], b0, vel=0.45 + 0.1 * k, legato=False)
    ostinato('tbn', b0, b0 + 4, octave=12, vel=0.45 + 0.1 * k)
    pulse_ost(b0, b0 + 4, vel=0.45, octave=12, cut=600, env=1800)
    crash(b0, 0, 0.7 + 0.1 * k)
    if k:
        line = CHOIR_LINE if k == 1 else CHOIR_LINE2
        play('horn', line, b0, vel=0.8 if k == 1 else 0.9, double=True)
        choir_line(line, b0, vel=0.45 if k == 1 else 0.55)
        if k == 2:
            play('tbn', line, b0, vel=0.85, transpose=-12)
            play('tuba', [(0, 'F1', 8), (8, 'Gb1', 8)], b0, vel=0.7)
    for b in range(b0, b0 + 4):
        taiko_ens(b, 0, vel=0.85, players=3)
        taiko_ens(b, 2, vel=0.7, players=3)
        for bt in (.75, 2.75):
            taiko_ens(b, bt, vel=0.5, players=2, synth=False)
        bass_drum(b, 0, 0.75)
    snare_pat(b0, b0 + 4, [SN_A, SN_B], vel=0.35 + 0.08 * k)
    braam_at(b0, 0, 8, 0.55 + 0.15 * k, chord='Fm', brass=k == 2)
    braam_at(b0 + 2, 0, 8, 0.55 + 0.15 * k, chord='Gb', brass=k == 2)
toms(59, [(2, 'hi'), (2.5, 'hi'), (3, 'lo'), (3.25, 'lo'), (3.5, 'lo'), (3.75, 'lo')], 0.9)
# the G turn (orig 47-48), then a 2-bar build on G
HUM[:] = [0.006, 0.05]
for b0, v in ((60, 0.85), (62, 1.0)):
    play('vln1', TURN_HI, b0, vel=v, transpose=12, double=True)
    play('vln2', TURN_HI, b0, vel=v, double=True)
    play('vla', TURN_MID, b0, vel=v * 0.9)
    play('fl', [(0, 'G5', 1.5), (1.5, 'G5', 2.5), (4, 'Eb5', 1.5), (5.5, 'D5', 2.5)], b0, vel=0.7)
    play('picc', [(0, 'G6', 8)], b0, vel=0.5)
    play('horn', TURN_MID, b0, vel=v, double=True)
    play('horn2', [(0, 'G4', 8)], b0, vel=v * 0.85)
    play('tbn', TURN_LO, b0, vel=v * 0.9)
    play('tuba', [(0, 'G1', 8)], b0, vel=v * 0.8)
    play('vc', TURN_LO, b0, vel=v * 0.9)
    choir_line([(0, 'G3+G4', 4), (4, 'Eb3+Eb4', 1.5), (5.5, 'D3+D4', 2.5)], b0, vel=0.55)
    braam_at(b0, 0, 8, 0.7 if b0 == 60 else 0.9, chord='G', brass=True)
    crash(b0, 0, 0.85)
ostinato('cb', 60, 63, vel=0.85)
ostinato('vc', 60, 63, vel=0.85, double=True)
ostinato('tbn', 60, 64, octave=0, vel=0.7)
sixteenths('vc', 62, 63, octave=12, vel=0.5)
pulse_ost(60, 63, vel=0.5, octave=12, cut=700, env=2400)
for b in range(60, 64):
    taiko_ens(b, 0, vel=0.95, players=4)
    taiko_ens(b, 2, vel=0.85, players=4)
    for bt in (1, 1.5, 3, 3.5):
        taiko_ens(b, bt, vel=0.55 + 0.1 * (b - 60), players=2, synth=False)
snare_pat(60, 62, [SN_A, SN_B], vel=0.5)
snare_roll(62, 0, 7.5, 0.25, 1.0, div=8)
timp_roll(62, 0, 7.5, 'G2', 0.1, 1.0)
toms(63, [(1, 'hi'), (1.5, 'hi'), (2, 'lo'), (2.5, 'lo'), (3, 'lo'), (3.25, 'lo')], 1.0)
cym_swell(63, 3.5, 'Long', 0.9)
rev_crash(63, 3.5, 4, 0.6)
riser(60, 0, 15.5, 0.5, 200, 12000)

# ===== S6  CLIMAX: the full fanfare twice (bars 64-79)
HUM[:] = [0.007, 0.05]
gong(64, 0, 1.0)
gong(72, 0, 0.9)
fanfare(64, big=True)
fanfare(72, big=True, major=True)
choir_pad(64, 72, vel=0.4, transpose=0)
choir_pad(72, 80, vel=0.5, transpose=12)
pads('vla', 64, 80, vel=0.45, pick=[0, 1, 2], attack=0.1)
for b in range(64, 80):
    for bt in (.5, 1.5, 2.5, 3.5):
        toms(b, [(bt, 'hi' if bt < 2 else 'lo')], 0.35)
rev_crash(72, 0, 2, 0.5)
toms(79, [(2, 'hi'), (2.25, 'hi'), (2.5, 'hi'), (2.75, 'lo'), (3, 'lo'), (3.25, 'lo'), (3.5, 'lo'),
          (3.75, 'lo')], 1.0)

# ===== S7  CODA, C minor rising to the final C major (orig 63-72) (bars 80-91)
HUM[:] = [0.007, 0.05]
crash(80, 0, 0.95, 'clash')
boom_at(80, 0, 0.9)
for b0, big in ((80, False), (84, True)):
    for half in (0, 2):
        mel = CODA_A if (b0, half) != (84, 2) else CODA_B
        v = 0.95 if big else 0.85
        play('vln1', mel, b0 + half, vel=v, double=True, short_max=0.5)
        play('vln2', mel, b0 + half, vel=v * 0.95, transpose=-12, short_max=0.5, double=big)
        play('fl', mel, b0 + half, vel=0.6, short_max=0)
        if big:
            play('picc', mel, b0 + half, vel=0.5, transpose=12, short_max=0)
        play('horn', CODA_PAD, b0 + half, vel=0.85 if big else 0.75, double=True)
        play('tpt', CODA_CALL if big else [], b0 + half, vel=1.0, short_max=0.3, double=True)
    play('vla', CODA_IN, b0, vel=0.75)
    play('cl', CODA_IN, b0, vel=0.55)
    for b in range(b0, b0 + 4):
        c = chord_at(b)
        play('tbn', CODA_ACC[c], b, vel=0.95, art='short')
        play('tuba', [(0, CH[c][0] - (12 if CH[c][0] >= 40 else 0), 4)], b, vel=0.8)
        timp(b, [(0, {36: 'C2', 38: 'D2', 39: 'Eb2', 41: 'F2'}[CH[c][0]])], 0.95)
        taiko_ens(b, 0, vel=0.95, players=4)
        taiko_ens(b, 3.5, vel=0.85, players=3)
        taiko_ens(b, 2, vel=0.7, players=3)
        bass_drum(b, 0, 0.85)
    ostinato('horn2', b0, b0 + 4, octave=12, vel=0.8)
    ostinato('vc', b0, b0 + 4, vel=0.9, double=True)
    ostinato('cb', b0, b0 + 4, vel=0.9)
    sixteenths('vc', b0, b0 + 4, octave=12, vel=0.4 if big else 0.3)
    pulse_ost(b0, b0 + 4, vel=0.5, octave=12, cut=800, env=2400)
    snare_pat(b0, b0 + 4, [SN_A, SN_B], vel=0.55)
    braam_at(b0, 0, 8, 0.8, chord='Cm', brass=False)
    braam_at(b0 + 2, 0, 8, 0.8, chord='Cm', brass=False)
crash(84, 0, 1.0, 'clash')
boom_at(84, 0, 1.0)
play('vla', [(3, 'Eb5', .5), (3.5, 'F5', .5)], 87, vel=0.9, art='short')
choir_pad(84, 88, vel=0.45, transpose=12)
# bar 88: G (orig 71): the G Eb D turn
HUM[:] = [0.005, 0.04]
crash(88, 0, 0.9)
play('vln1', TURN_END, 88, vel=1.0, transpose=12, double=True)
play('vln2', TURN_END, 88, vel=1.0, double=True)
play('vla', TURN_END, 88, vel=0.9)
play('fl', TURN_END, 88, vel=0.8, transpose=12)
play('picc', TURN_END, 88, vel=0.7, transpose=24)
play('horn', [(0, 'G4+D4', 4)], 88, vel=1.0, double=True)
play('tbn', [(0, 'G3+D4', 4)], 88, vel=1.0)
play('tuba', [(0, 'G1', 4)], 88, vel=0.9)
play('tpt', [(0, 'G5', 3), (3, 'Bb5', 1)], 88, vel=1.0, double=True)
play('vc', [(0, 'G2', 4)], 88, vel=1.0, double=True)
play('cb', [(0, 'G1', 4)], 88, vel=1.0)
choir_line([(0, 'G3+D4+G4', 4)], 88, vel=0.6)
braam_at(88, 0, 4, 1.0, chord='G', brass=False)
timp_roll(88, 0, 4, 'G2', 0.3, 1.0)
snare_roll(88, 0, 4, 0.4, 1.0, div=8)
for bt in (0, 1, 2, 3):
    taiko_ens(88, bt, vel=0.9, players=4)
toms(88, [(3, 'hi'), (3.25, 'hi'), (3.5, 'lo'), (3.75, 'lo')], 1.0)
rev_crash(89, 0, 2, 0.6)

# ===== FINAL: C major chord (orig 72), held, then the last hit (bars 89-91)
HUM[:] = [0.006, 0.04]
gong(89, 0, 1.0)
crash(89, 0, 1.0, 'clash')
boom_at(89, 0, 1.0)
FIN = 7.5   # beats held (an 8th of air before the last hit)
play('vln1', [(0, 'C6+G5', FIN)], 89, vel=1.0, double=True, legato=False)
play('vln2', [(0, 'E5+C5', FIN)], 89, vel=1.0, double=True, legato=False)
play('vla', [(0, 'G4+E4', FIN)], 89, vel=0.95, legato=False)
play('vc', [(0, 'C3+G2', FIN)], 89, vel=1.0, double=True, legato=False)
play('cb', [(0, 'C2', FIN)], 89, vel=1.0, legato=False)
play('horn', [(0, 'C4+E4+G4', FIN)], 89, vel=1.0, double=True, legato=False)
play('horn2', [(0, 'C5', FIN)], 89, vel=0.9, legato=False)
play('tpt', [(0, 'C6+G5', FIN)], 89, vel=1.0, double=True, legato=False)
play('tpt2', [(0, 'E5+C5', FIN)], 89, vel=0.9, legato=False)
play('tbn', [(0, 'C3+G3+E3', FIN)], 89, vel=1.0, legato=False)
play('tuba', [(0, 'C2', FIN)], 89, vel=1.0, legato=False)
play('fl', [(0, 'G5+C6', FIN)], 89, vel=0.8, legato=False)
play('picc', [(0, 'G6', FIN)], 89, vel=0.65, legato=False)
play('cl', [(0, 'E5', FIN)], 89, vel=0.7, legato=False)
play('bsn', [(0, 'C3', FIN)], 89, vel=0.8, legato=False)
choir_pad(89, 91, vel=0.6, voic=['C4', 'E4', 'G4', 'C5'])
braam_at(89, 0, FIN, 1.0, chord='C', brass=False)
timp_roll(89, 0, FIN, 'C3', 0.4, 1.0)
snare_roll(90, 0, 4, 0.2, 0.9, div=8)
cym_swell(91, 0, 'Short', 0.7)
for bt in (0, 2, 4, 5, 6, 6.5, 7, 7.25, 7.5, 7.75):
    taiko_ens(89 + int(bt // 4), bt % 4, vel=0.8 + 0.02 * bt, players=3)
# the last hit
for p, nn in (('vln1', 'C6+G5'), ('vln2', 'C5+E5'), ('vla', 'G4+C4'), ('vc', 'C3+C2'), ('cb', 'C2'),
              ('horn', 'C4+G4'), ('horn2', 'E4+C5'), ('tpt', 'C5+G5'), ('tpt2', 'C6'), ('tbn', 'C3+G3'),
              ('tuba', 'C2')):
    play(p, [(0, nn, .5)], 91, vel=1.0, art='short', rel=0.25)
timp(91, [(0, 'C2'), (0, 'C3')], 1.0)
taiko_ens(91, 0, vel=1.0, players=5)
bass_drum(91, 0, 1.0)
boom_at(91, 0, 1.0)
crash(91, 0, 1.0, 'clash')
gong(91, 0, 0.8)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


GAIN = {  # dB
    'vln': 1.0, 'vla': 0.0, 'lowstr': 1.5, 'horn': -1.5, 'tpt': -5.0, 'lowbr': 0.5, 'ww': -4.5,
    'timp': 12.0, 'taiko': 6.0, 'snare': 6.0, 'cym': -4.0, 'synth': -6.0, 'pad': -18.0,
    'sub': -5.0, 'fx': -10.0,
}
VERB = {'vln': .35, 'vla': .35, 'lowstr': .25, 'horn': .4, 'tpt': .3, 'lowbr': .3, 'ww': .45,
        'timp': .35, 'taiko': .25, 'snare': .25, 'cym': .25, 'synth': .12, 'pad': .5, 'fx': .3}

bus['vln'].x = stage(bus['vln'].x, HighpassFilter(140), PeakFilter(3200, -2.0, 1.0), HighShelfFilter(9000, 1.0, 0.7))
bus['vla'].x = stage(bus['vla'].x, HighpassFilter(110))
bus['lowstr'].x = stage(bus['lowstr'].x, HighpassFilter(32))
bus['horn'].x = stage(bus['horn'].x, HighpassFilter(70), LowShelfFilter(200, 1.5, 0.7))
bus['tpt'].x = stage(bus['tpt'].x, HighpassFilter(160), PeakFilter(2600, -1.5, 1.0))
bus['lowbr'].x = stage(bus['lowbr'].x, HighpassFilter(35))
bus['ww'].x = stage(bus['ww'].x, HighpassFilter(180))
bus['timp'].x = stage(bus['timp'].x, HighpassFilter(35))
bus['taiko'].x = stage(bus['taiko'].x, HighpassFilter(30),
                       Compressor(threshold_db=-14, ratio=2.5, attack_ms=8, release_ms=120))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(150))
bus['cym'].x = stage(bus['cym'].x, HighpassFilter(300))
bus['synth'].x = stage(bus['synth'].x, HighpassFilter(38), LowpassFilter(6500))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(150), LowpassFilter(7000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(250))

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('SONG7_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in BUSES:
    x = bus[k].x
    del bus[k]
    if k in MUTE:
        continue
    x *= np.float32(10 ** (GAIN[k] / 20))
    nz = np.nonzero(np.max(np.abs(x), 0) > 1e-4)[0]
    if len(nz) > SR:
        seg = x[:, nz[0]:nz[-1]].T.astype(np.float64)
        levels[k] = meter.integrated_loudness(seg)
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    del x

ir = O.hall_ir()
verb = O.convolve(stage(verb_send, HighpassFilter(180), LowpassFilter(9000)), ir)
del verb_send
mix += verb * np.float32(0.55)
del verb

# section dynamics (dB at bar positions)
SECTION_DB = [(0, 4.5), (11.9, 2.0), (12, 0.0), (15.9, 0.0), (16, -1.5), (23.9, -1.0), (24, -1.5),
              (31.9, -1.5), (32, 4.5), (43.9, 3.5), (47.9, 0.5), (48, -2.0), (59.9, -1.0), (60, -1.0),
              (63.9, 0.0), (64, 0.5), (79.9, 1.0), (80, 0.0), (88.9, 0.5), (89, 1.5), (NBARS + 5, 1.5)]
bars_axis = np.arange(N) / SR / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del bars_axis, sec

# ---------------------------------------------------------- master
# mono low end: remove the side signal below 120 Hz (zero-phase complement)
side = (mix[0] - mix[1]) * 0.5
side -= ss.sosfiltfilt(ss.butter(4, 120, 'low', fs=SR, output='sos'), side).astype(np.float32)
mid = (mix[0] + mix[1]) * 0.5
mix = np.stack([mid + side, mid - side]).astype(np.float32)
del side, mid
mix = stage(mix, HighpassFilter(24))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-15.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(110, 1.5, 0.7), PeakFilter(700, -3.0, 0.6), HighShelfFilter(5000, 3.0, 0.7),
            Compressor(threshold_db=-12, ratio=1.8, attack_ms=30, release_ms=200))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=120))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song7_orchestral.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
