#!/usr/bin/env python3
# new02_proghouse.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Across the Heliopause": a NEW late-1990s progressive-house piece (Sasha &
# Digweed / BT / Hybrid / Way Out West territory) for the game's soundtrack,
# 128 BPM, C# major with an F# minor / Ab minor middle.  Produced entirely in
# code with tools/music/edm/synth.py and synth_new02.py: 909-style kit with a
# soft round clap, rolling sub + saw bass, 16th pluck arpeggio in 3-against-4,
# warm JP/Juno pads, a detuned-saw lead, a reed-like duo voice, piano, string
# shimmer; filter-sweep builds, sidechain, delay and reverb.  No samples.
#
# Built from material of the original song 2 (mid/sf-music2.mid, Stardock's
# music; bar numbers are the MIDI's, 1-based, no pickup) that the existing
# remix (song2_synthwave.py) did not feature:
#   1. The clarinet two-voice inner line of the Ab-minor section, bars 25-40
#      (ch 8/9 "Clarinet": A3/F#3 | B3/Ab3 | Ab3,Ab3/E3,B3 and the climb of bars
#      33-40: C#4/A3 | Eb4/B3 | Ab3/E3,B3 | C#4/A3 | C#4,C#4/E4,A3 | Eb4/F#4 |
#      E4,Eb4/Ab4,F#4).  Bars 25-32: the "reed" duo of section 32-39.  Bars 33-40:
#      the main theme (+1 octave), first on piano in the breakdown, then on the
#      lead in the peak, with its second voice as the harmony; its harmony
#      F#m F#m Abm E/G# | F#m F#m7 Abm7 E/G#-Abm7 is the track's middle section.
#   2. The string-section inner voices of bars 1-20 (ch 13/14 "String Ensemble
#      2"): the voice exchange F4/Ab4 -> E4/C#4 -> F#4/Eb4 -> Ab4/F4 over the
#      C# A B progression.  Here it is set over a C# pedal (C# A/C# B/C#): it
#      gives the arpeggio its note cells in the intro and outro and becomes the
#      two-voice line of the build (bars 48-55: F5 E5 F#5 Ab5 E5 F#5 over
#      Ab5 C#5 Eb5 F5 C#5 Eb5).
#   3. The fretless-bass / timpani figure of bars 41-44 (C#2 .. Ab2 C#2 | A1 |
#      C#2 .. F#2 F#2; timpani pickups Ab2 Ab2 / A2 A2): the rolling bassline of
#      the groove (C# | C# Ab C# | A | C# F#) with its progression C# C# F#m/A
#      F#m, the 909 tom pickups, and the answer phrase of the second peak.
#   4. The high string pedal of bars 25-40 (ch 11 "String Ensemble 1": C#6 /
#      Eb6 alternating every two bars) as a shimmer over the middle section.
# New: the form (intro on a pedal, groove, theme, breakdown, filter build,
# peak, answer, outro, final hit), the 16th arp, the rhythmic re-strikes of
# the theme, the answer phrase, and the drums and production.
#
#   python3 tools/music/edm/new02_proghouse.py OUT.wav [bar0 bar1]
#   NEW02_MUTE=kick,clap,...   mutes buses (for the chroma check)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new02 as P  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 128.0
SPB = 60.0 / BPM
NBARS = 97          # 96 bars + the final hit
TAIL = 4.5


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
START = T(BAR0)
END = min(T(BAR1) + TAIL, T(NBARS) + TAIL)
N = int((END - START) * SR)

# ------------------------------------------------------------- harmony

PAD = {
    'C#':    ['Ab3', 'C#4', 'F4', 'Ab4'],
    'A/C#':  ['C#4', 'E4', 'A4', 'C#5'],
    'B/C#':  ['C#4', 'Eb4', 'F#4', 'B4'],
    'F#m/A': ['A3', 'C#4', 'F#4', 'A4'],
    'F#m':   ['A3', 'C#4', 'F#4', 'A4'],
    'F#m7':  ['A3', 'C#4', 'E4', 'A4'],
    'Abm':   ['B3', 'Eb4', 'Ab4', 'B4'],
    'Abm7':  ['B3', 'Eb4', 'F#4', 'B4'],
    'E/G#':  ['B3', 'E4', 'Ab4', 'B4'],
}
# arpeggio cells (low, mid, top): the string voice pairs of bars 1-20 plus the root
CELL = {'C#': ['F4', 'Ab4', 'C#5'], 'A/C#': ['E4', 'A4', 'C#5'], 'B/C#': ['Eb4', 'F#4', 'B4'],
        'F#m/A': ['F#4', 'A4', 'C#5'], 'F#m': ['F#4', 'A4', 'C#5'], 'F#m7': ['E4', 'A4', 'C#5'],
        'Abm': ['Eb4', 'Ab4', 'B4'], 'Abm7': ['Eb4', 'F#4', 'B4'], 'E/G#': ['E4', 'Ab4', 'B4']}
# rolling-bass notes (mid octave; the sub plays an octave lower): (beat, midi)
BASSN = {'C#': [(0, 49)], 'C#b': [(0, 49), (2, 44), (3, 49)],      # bar 42: C# .. Ab C#
         'A/C#': [(0, 49)], 'B/C#': [(0, 49)],
         'F#m/A': [(0, 45)],                                       # bar 43: A
         'F#m*': [(0, 49), (2, 42)],                               # bar 44: C# .. F# F#
         'F#m': [(0, 42)], 'F#m7': [(0, 42), (3, 40)], 'Abm': [(0, 44)], 'Abm7': [(0, 44)],
         'E/G#': [(0, 44)], 'E/G#|Abm7': [(0, 44)]}

PEDAL = ['C#', 'C#', 'A/C#', 'B/C#']
GROOVE = ['C#', 'C#b', 'F#m/A', 'F#m*']
MIDDLE = ['F#m', 'F#m', 'Abm', 'E/G#']
CLIMB = ['F#m', 'F#m', 'Abm', 'E/G#', 'F#m', 'F#m7', 'Abm7', 'E/G#|Abm7']

CH = (PEDAL * 4 +            # 0-15   intro on a C# pedal
      GROOVE * 4 +           # 16-31  groove (bass figure of bars 41-44)
      MIDDLE * 2 +           # 32-39  middle: the reed duo (bars 25-32)
      CLIMB +                # 40-47  breakdown: the climb (bars 33-40) on piano
      PEDAL * 2 +            # 48-55  build: the voice-exchange line, filter sweep
      CLIMB * 2 +            # 56-71  peak: the climb on the lead
      GROOVE * 2 +           # 72-79  peak 2: the answer
      GROOVE * 2 +           # 80-87  outro
      PEDAL * 2 +            # 88-95  outro on the pedal
      ['C#'])                # 96     final hit
assert len(CH) == NBARS


def pad_of(label, half=0):
    if label == 'E/G#|Abm7':
        return 'E/G#' if half == 0 else 'Abm7'
    if label in PAD:
        return label
    return label.rstrip('b*')


# --------------------------------------------------------------- themes
# (beat, note, length in beats)

# 1. the clarinet duo, bars 25-28 (+8va)
TA_V1 = [(0, 'A4', 8), (8, 'B4', 4), (12, 'Ab4', 2), (14, 'Ab4', 2)]
TA_V2 = [(0, 'F#4', 8), (8, 'Ab4', 4), (12, 'E4', 2), (14, 'B4', 2)]
# the climb, bars 33-40 (+8va): upper line of the two clarinets, and the lower one
TB_TOP = [(0, 'C#5', 8), (8, 'Eb5', 4), (12, 'Ab4', 2), (14, 'B4', 2), (16, 'C#5', 4),
          (20, 'E5', 2), (22, 'C#5', 2), (24, 'F#5', 4), (28, 'Ab5', 2), (30, 'F#5', 2)]
TB_LOW = [(0, 'A4', 8), (8, 'B4', 4), (12, 'E4', 2), (14, 'Ab4', 2), (16, 'A4', 4),
          (20, 'C#5', 2), (22, 'A4', 2), (24, 'Eb5', 4), (28, 'E5', 2), (30, 'Eb5', 2)]
# the peak version: same line with re-strikes and passing notes
TB_PEAK = [(0, 'C#5', 3), (3, 'E5', 1), (4, 'C#5', 3), (7, 'B4', .5), (7.5, 'C#5', .5),
           (8, 'Eb5', 3), (11, 'Eb5', .5), (11.5, 'E5', .5), (12, 'Ab4', 2), (14, 'B4', 2),
           (16, 'C#5', 3), (19, 'C#5', 1), (20, 'E5', 2), (22, 'C#5', 2),
           (24, 'F#5', 3), (27, 'Eb5', 1), (28, 'Ab5', 2), (30, 'F#5', 2)]
TB_PEAK_LOW = [(0, 'A4', 3), (3, 'C#5', 1), (4, 'A4', 3.5), (7.5, 'A4', .5),
               (8, 'B4', 3.5), (11.5, 'B4', .5), (12, 'E4', 2), (14, 'Ab4', 2),
               (16, 'A4', 3), (19, 'A4', 1), (20, 'C#5', 2), (22, 'A4', 2),
               (24, 'Eb5', 3), (27, 'B4', 1), (28, 'E5', 2), (30, 'Eb5', 2)]
# 2. the string voice exchange of bars 5-12 (+8va), over the C# pedal
BV1 = [(0, 'F5', 8), (8, 'E5', 4), (12, 'F#5', 4), (16, 'Ab5', 8), (24, 'E5', 4), (28, 'F#5', 4)]
BV2 = [(0, 'Ab4', 8), (8, 'C#5', 4), (12, 'Eb5', 4), (16, 'F5', 8), (24, 'C#5', 4), (28, 'Eb5', 4)]
# 3. the answer, from the bass figure of bars 41-44 (C# .. Ab C# | A | C# F#)
ANS = [(0, 'F5', 3), (3, 'Eb5', .5), (3.5, 'F5', .5),
       (4, 'F5', 2), (6, 'Ab5', 1), (7, 'F5', 1),
       (8, 'E5', 3), (11, 'C#5', 1),
       (12, 'C#5', 2), (14, 'F#5', 2),
       (16, 'F5', 3), (19, 'Eb5', .5), (19.5, 'F5', .5),
       (20, 'F5', 2), (22, 'Ab5', 1), (23, 'C#6', 1),
       (24, 'A5', 4),
       (28, 'Ab5', 2), (30, 'F#5', 2)]
ANS_LOW = [(0, 'C#5', 4), (4, 'C#5', 2), (6, 'C#5', 2), (8, 'C#5', 4), (12, 'A4', 4),
           (16, 'C#5', 4), (20, 'C#5', 4), (24, 'E5', 4), (28, 'C#5', 4)]

# --------------------------------------------------------------- buses

names = ['kick', 'clap', 'hats', 'perc', 'cym', 'fx', 'sub', 'bass', 'pad', 'arp',
         'lead', 'harm', 'reed', 'piano', 'shim']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, t, maker, gain=1.0, length=12.0):
    if t > END or t + length < START:
        return
    bus[name].add(maker() if callable(maker) else maker, t - START, gain)


def rnd():
    return S.rng().random()


def panned(sig, p):
    return np.stack([sig[0] * np.sqrt((1 - p) / 2), sig[1] * np.sqrt((1 + p) / 2)]) * np.sqrt(2)


# ------------------------------------------------------------- drums

def kick_at(bar, beat, vel=1.0):
    t = T(bar, beat)
    put('kick', t, lambda: cached('k', P.kick909), vel, 1.0)
    if START <= t < END:
        kicks.append(t - START)


def four(bar0, bar1, vel=1.0, last_beats=4):
    for b in range(bar0, bar1):
        for bt in range(4 if b < bar1 - 1 else last_beats):
            kick_at(b, bt, vel)


def claps(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        for i, bt in enumerate((1, 3)):
            seed = (b * 2 + i * 3 + b // 3) % 4
            acc = 1.0 if bt == 1 else 0.93
            put('clap', T(b, bt), lambda seed=seed: cached(('cl', seed), lambda: P.clap_soft(seed)),
                vel * acc * (0.94 + 0.12 * rnd()), 1.0)


def hats(bar0, bar1, closed=0.5, opn=0.0, shak=0.0, ride=0.0):
    acc = {0: .45, .25: .3, .5: .0, .75: .38}
    for b in range(bar0, bar1):
        for beat in range(4):
            for s16, a in acc.items():
                sw = 0.025 if s16 in (.25, .75) else 0.0          # a touch of 16th swing
                if closed and a:
                    v = S.rng().integers(5)
                    put('hats', T(b, beat + s16 + sw), lambda v=v: cached(('hc', v), lambda: panned(P.hat909(v), -0.25)),
                        closed * a * (0.85 + 0.3 * rnd()), 1.0)
                if shak:
                    v = S.rng().integers(4)
                    sa = (.8 if s16 == .5 else .5 if s16 == 0 else .65)
                    put('perc', T(b, beat + s16 + sw), lambda v=v: cached(('sh', v), lambda: panned(P.shaker(v), 0.4)),
                        shak * sa * (0.85 + 0.3 * rnd()), 1.0)
            if opn:
                v = S.rng().integers(3)
                put('hats', T(b, beat + .5), lambda v=v: cached(('ho', v), lambda: panned(P.hat909(v, True), 0.2)),
                    opn * (0.9 + 0.2 * rnd()), 1.0)
            if ride:
                v = S.rng().integers(3)
                put('cym', T(b, beat), lambda v=v: cached(('rd', v), lambda: panned(P.ride909(v), 0.3)),
                    ride * (1.0 if beat % 2 == 0 else 0.8), 1.5)


def tom(bar, beat, note, vel=1.0):
    md = m(note)
    put('perc', T(bar, beat), lambda: cached(('tom', md), lambda: P.tom909(md)), vel, 1.0)


def pickups(bar0, bar1, vel=1.0):
    """The timpani pickups of bars 41-44: Ab2 Ab2 into C#, A2 A2 into F#m/A."""
    for b in range(bar0, bar1):
        nxt = CH[b + 1] if b + 1 < NBARS else 'C#'
        note = {'C#': 'Ab2', 'F#m/A': 'A2', 'F#m': 'A2'}.get(nxt)
        if note and CH[b] != nxt:
            tom(b, 3.5, note, vel * 0.8)
            tom(b, 3.75, note, vel)


def crash(bar, vel=1.0):
    put('cym', T(bar), lambda: cached('cr', P.crash_soft), vel, 3.0)


def rev_cym(bar, beats, vel=1.0):
    put('fx', T(bar) - B(beats), lambda: butter_lp(S.reverse_cymbal(B(beats)), 7000), vel, B(beats) + 1)


def butter_lp(x, fc):
    return S.butter(x, fc, 'low', 2)


# ----------------------------------------------------------- instruments

def bass_notes(bar):
    return BASSN[CH[bar]] if CH[bar] in BASSN else BASSN[pad_of(CH[bar])]


def bass_at(bar, beat):
    md = None
    for bt, n in bass_notes(bar):
        if beat >= bt:
            md = n
    return md


def bass(bar0, bar1, style='roll', vel=1.0, sub_vel=1.0):
    for b in range(bar0, bar1):
        segs = bass_notes(b)
        for i, (bt, md) in enumerate(segs):
            end = segs[i + 1][0] if i + 1 < len(segs) else 4
            if sub_vel:
                put('sub', T(b, bt), lambda md=md, d=end - bt: cached(('sub', md, d), lambda: P.sub(md - 12, B(d) - .02)),
                    sub_vel, 8.0)
        if style == 'hold':
            continue
        for beat in range(4):
            if style == 'roll':
                pat = ((.25, .62), (.5, 1.0), (.75, .72))
            else:                       # 'off': offbeat eighths
                pat = ((.5, 1.0),)
            for s16, a in pat:
                md = bass_at(b, beat + s16)
                put('bass', T(b, beat + s16), lambda md=md: cached(('bm', md), lambda: P.bass_mid(md, B(.21))),
                    vel * a * (0.95 + 0.1 * rnd()), 1.0)


def pads(bar0, bar1, vel=1.0, a=0.8, cut=1300.0, cut_open=1200.0):
    # split into (time, chord) segments
    segs = []
    for b in range(bar0, bar1):
        for half in (0, 1):
            segs.append((b + half * .5, pad_of(CH[b], half)))
    i = 0
    while i < len(segs):
        j = i
        while j < len(segs) and segs[j][1] == segs[i][1]:
            j += 1
        t0 = T(0, segs[i][0] * 4)
        t1 = T(0, segs[j][0] * 4) if j < len(segs) else T(bar1)
        ch = segs[i][1]
        for note in PAD[ch]:
            put('pad', t0, lambda note=note, d=t1 - t0: P.warm_pad(m(note), d, a=a, cut=cut, cut_open=cut_open),
                vel * 0.5, t1 - t0 + 6.0)
        i = j


def arp(bar0, bar1, vel=1.0, oct_var=True):
    for b in range(bar0, bar1):
        for i in range(16):
            g = b * 16 + i
            half = 0 if i < 8 else 1
            cell = CELL[pad_of(CH[b], half)]
            k = g % 3
            md = m(cell[k])
            if oct_var and k == 2 and (g // 3) % 4 == 3:
                md += 12
            acc = 1.0 if i % 4 == 0 else (0.78 if i % 2 == 0 else 0.62)
            pan = (-0.35, 0.05, 0.4)[k]
            put('arp', T(b, i * .25), lambda md=md, k=k, pan=pan: cached(
                ('pl', md, k), lambda: panned(P.pluck(md, B(.2)), pan)),
                vel * acc * (0.92 + 0.16 * rnd()), 1.0)


INST = {
    'lead': lambda md, d, **kw: P.lead(md, d, **kw),
    'soft': lambda md, d, **kw: P.lead(md, d, a=0.12, cut=1500, env=600, f_dec=0.6, vib=0.09, **kw),
    'reed': lambda md, d, **kw: P.reed(md, d, **kw),
    'piano': lambda md, d, **kw: S.piano(md, d, **kw),
    'bell': lambda md, d, **kw: S.bell(md, d, **kw),
    'shim': lambda md, d, **kw: P.shimmer(md, d, **kw),
}


def play(name, bar, notes, inst, transpose=0, vel=1.0, legato=0.015, **kw):
    for beat, note, dur in notes:
        md = m(note) + transpose
        put(name, T(bar, beat),
            lambda md=md, dur=dur: INST[inst](md, max(0.05, B(dur) - legato), **kw),
            vel, B(dur) + 6.0)


def shimmer_pedal(bar0, bar1, vel=1.0):
    """The high string pedal of bars 25-40: C#6 over F#m, Eb6 over Abm / E."""
    b = bar0
    while b < bar1:
        note = 'C#6' if pad_of(CH[b]).startswith('F#m') else 'Eb6'
        play('shim', b, [(0, note, 8)], 'shim', vel=vel)
        b += 2


def piano_lh(bar0, bar1, vel=0.6):
    LH = {'F#m': ['F#2', 'C#3'], 'F#m7': ['F#2', 'E3'], 'Abm': ['Ab2', 'Eb3'], 'Abm7': ['Ab2', 'F#3'],
          'E/G#': ['Ab2', 'B2'], 'C#': ['C#2', 'Ab2'], 'A/C#': ['C#2', 'A2'], 'B/C#': ['C#2', 'B2'],
          'F#m/A': ['A1', 'E2']}
    for b in range(bar0, bar1):
        ch = pad_of(CH[b])
        for i, note in enumerate(LH[ch]):
            play('piano', b, [(i * .5, note, 4 - i * .5)], 'piano', vel=vel)


# ---------------------------------------------------------- arrangement

# INTRO 0-15: kick, hats, the arp on the C# pedal opening up; bass from 8
four(0, 16)
hats(0, 4, closed=0.0, shak=0.35)
hats(4, 8, closed=0.4, shak=0.4)
hats(8, 12, closed=0.45, opn=0.22, shak=0.4)
hats(12, 16, closed=0.5, opn=0.3, shak=0.45)
claps(8, 16, 0.8)
arp(0, 16, vel=0.9)
bass(8, 16, 'off', vel=0.85, sub_vel=0.85)
pickups(11, 16, 0.6)
pads(8, 16, vel=0.4, a=2.0, cut=900, cut_open=500)
rev_cym(16, 2, 0.5)

# GROOVE 16-31: the bass figure of bars 41-44, rolling
crash(16, 0.7)
four(16, 32, last_beats=2)
claps(16, 31, 1.0)
hats(16, 32, closed=0.55, opn=0.35, shak=0.45)
bass(16, 32, 'roll')
arp(16, 32, vel=1.0)
pickups(16, 32, 0.8)
pads(16, 24, vel=0.4, a=1.0, cut=1000, cut_open=600)
pads(24, 32, vel=0.6, a=0.6, cut=1200, cut_open=900)
rev_cym(32, 2, 0.6)

# MIDDLE 32-39: the clarinet duo of bars 25-32, the string shimmer, ride
crash(32, 0.75)
four(32, 40, last_beats=2)
claps(32, 39, 1.0)
hats(32, 40, closed=0.5, opn=0.32, shak=0.4, ride=0.18)
bass(32, 40, 'roll')
arp(32, 40, vel=0.95)
pads(32, 40, vel=0.6, a=0.5, cut=1300, cut_open=900)
for bb in (32, 36):
    play('reed', bb, TA_V1, 'reed', vel=0.9)
    play('reed', bb, TA_V2, 'reed', vel=0.75)
shimmer_pedal(32, 40, 0.8)
put('fx', T(39, 2), lambda: S.downlifter(B(8), 5000, 300), 0.35, 8.0)

# BREAKDOWN 40-47: the climb of bars 33-40 on piano, pads, shimmer, held sub
put('fx', T(40), lambda: S.impact(), 0.35, 3.0)
pads(40, 48, vel=0.75, a=1.2, cut=1100, cut_open=1400)
bass(40, 48, 'hold', sub_vel=0.55)
arp(40, 48, vel=0.6, oct_var=False)
play('piano', 40, TB_TOP, 'piano', vel=0.85)
play('piano', 40, TB_LOW, 'piano', vel=0.6)
piano_lh(40, 48, 0.35)
play('lead', 40, TB_TOP[4:], 'soft', vel=0.55)
shimmer_pedal(40, 48, 1.0)
hats(44, 48, closed=0.0, shak=0.22)

# BUILD 48-55: the string voice exchange over the C# pedal, everything opening
play('lead', 48, BV1, 'soft', vel=0.85)
play('reed', 48, BV2, 'reed', vel=0.7)
pads(48, 56, vel=0.6, a=0.3, cut=1300, cut_open=1500)
arp(48, 56, vel=1.0)
four(48, 52, vel=0.8)
four(52, 56, last_beats=2)
hats(48, 52, closed=0.3, shak=0.45)
hats(52, 56, closed=0.5, opn=0.3, shak=0.5)
claps(52, 55, 0.9)
bass(48, 52, 'hold', sub_vel=0.6)
bass(52, 56, 'roll')
for i, bt in enumerate((2.5, 3.0, 3.5, 3.75)):
    tom(55, bt, ['C#3', 'Ab2', 'A2', 'A2'][i], 0.55 + 0.12 * i)
put('fx', T(48), lambda: S.butter(S.noise_riser(T(56) - T(48), 250, 6000, 1.5), 6500, 'low', 2), 0.3, 18.0)
rev_cym(56, 4, 0.6)

# PEAK 56-71: the climb on the lead, full groove
for bb in (56, 64):
    crash(bb, 0.9 if bb == 56 else 0.6)
put('fx', T(56), lambda: S.impact(), 0.55, 3.0)
four(56, 72)
claps(56, 72, 1.0)
hats(56, 64, closed=0.55, opn=0.35, shak=0.45)
hats(64, 72, closed=0.55, opn=0.38, shak=0.45, ride=0.22)
bass(56, 72, 'roll')
arp(56, 72, vel=0.95)
pads(56, 72, vel=0.65, a=0.25, cut=1500, cut_open=1000)
play('lead', 56, TB_PEAK, 'lead', vel=1.0)
play('lead', 64, TB_PEAK, 'lead', vel=1.0)
play('harm', 64, TB_PEAK_LOW, 'lead', vel=0.8, cut=1800, env=1500)
play('harm', 64, TB_PEAK, 'lead', transpose=12, vel=0.3, cut=2000, env=1500)
shimmer_pedal(56, 72, 0.55)
pickups(56, 72, 0.7)

# PEAK 2 72-79: the answer over the groove progression
crash(72, 0.8)
four(72, 80)
claps(72, 80, 1.0)
hats(72, 80, closed=0.55, opn=0.38, shak=0.45, ride=0.22)
bass(72, 80, 'roll')
arp(72, 80, vel=0.95)
pads(72, 80, vel=0.65, a=0.25, cut=1500, cut_open=1000)
play('lead', 72, ANS, 'lead', vel=0.95)
play('harm', 72, ANS_LOW, 'reed', vel=0.75)
pickups(72, 80, 0.8)

# OUTRO 80-95: back to the groove, then the pedal; elements leave
crash(80, 0.6)
four(80, 96)
claps(80, 92, 0.95)
hats(80, 88, closed=0.55, opn=0.35, shak=0.45)
hats(88, 92, closed=0.5, opn=0.3, shak=0.4)
hats(92, 96, closed=0.45, shak=0.35)
bass(80, 88, 'roll')
bass(88, 96, 'off', vel=0.9)
arp(80, 96, vel=0.9)
pads(80, 88, vel=0.55, a=0.4, cut=1300, cut_open=800)
pads(88, 96, vel=0.4, a=0.6, cut=1000, cut_open=500)
play('shim', 80, ANS[:6], 'bell', vel=0.5)
play('reed', 88, BV1, 'reed', vel=0.7)
pickups(80, 88, 0.7)

# FINAL HIT 96: C# with a natural ring
kick_at(96, 0, 1.0)
crash(96, 0.7)
put('fx', T(96), lambda: S.impact(), 0.5, 3.0)
for note in ['Ab3', 'C#4', 'F4', 'Ab4', 'C#5']:
    put('pad', T(96), lambda note=note: P.warm_pad(m(note), 1.2, a=0.005, r=0.9, cut=1600, cut_open=600), 0.45, 8)
for note in ['C#2', 'Ab2', 'C#3', 'F3', 'Ab3', 'C#4', 'F4']:
    play('piano', 96, [(0, note, 6)], 'piano', vel=0.6)
put('sub', T(96), lambda: P.sub(37, 1.6, r=0.4), 0.9, 4)
for note in ['F4', 'Ab4', 'C#5']:
    put('arp', T(96), lambda note=note: P.pluck(m(note), 0.4, adecay=0.6), 0.7, 3)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


abs_bars = (START + np.arange(N) / SR) / (4 * SPB)


def curve(points, log=True):
    xs = [p_[0] for p_ in points]
    ys = [p_[1] for p_ in points]
    if log:
        return np.exp(np.interp(abs_bars, xs, np.log(ys)))
    return np.interp(abs_bars, xs, ys)


def sweep(name, points, q=0.8):
    bus[name].x = S.svf(bus[name].x, curve(points), q).astype(np.float32)


duck_hard = S.sidechain(N, kicks, depth=0.85, release=0.30)
duck_mid = S.sidechain(N, kicks, depth=0.5, release=0.34)
duck_soft = S.sidechain(N, kicks, depth=0.28, release=0.30)

GAIN = {  # dB
    'kick': -3.0, 'clap': -1.0, 'hats': 7.5, 'perc': -6.0, 'cym': -16.0, 'fx': -16.0,
    'sub': -11.0, 'bass': -3.5, 'pad': 0.0, 'arp': -7.5,
    'lead': -5.5, 'harm': -10.0, 'reed': -8.0, 'piano': -6.0, 'shim': -14.0,
}
DUCK = {'sub': duck_hard, 'bass': duck_hard, 'pad': duck_mid, 'arp': duck_mid, 'reed': duck_mid,
        'lead': duck_soft, 'harm': duck_soft, 'shim': duck_soft, 'fx': duck_soft}
VERB = {'clap': .12, 'perc': .15, 'pad': .3, 'arp': .22, 'lead': .25, 'harm': .3, 'reed': .3,
        'piano': .35, 'shim': .55, 'fx': .3, 'hats': .04}
DELAY = {'arp': .35, 'lead': .22, 'harm': .15, 'piano': .18, 'reed': .12, 'perc': .1}

# filter automation (bar, Hz): the long sweeps
sweep('arp', [(0, 380), (8, 1700), (16, 5500), (32, 7000), (39.9, 7000), (40, 1300), (47.9, 1600),
              (48, 450), (55.9, 14000), (56, 10000), (79.9, 10000), (88, 5500), (95.9, 2000), (97, 2500)], 0.9)
sweep('pad', [(0, 20000), (47.9, 20000), (48, 1100), (55.9, 16000), (56, 20000)], 0.75)
sweep('lead', [(0, 20000), (47.9, 20000), (48, 1500), (55.9, 12000), (56, 20000)], 0.75)
sweep('reed', [(0, 20000), (47.9, 20000), (48, 1500), (55.9, 9000), (56, 20000)], 0.75)
sweep('bass', [(0, 20000), (51.9, 20000), (52, 260), (55.9, 4000), (56, 20000)], 0.8)
sweep('kick', [(0, 20000), (47.9, 20000), (48, 160), (51.9, 420), (52, 20000)], 0.71)

bus['arp'].x = stage(bus['arp'].x, HighpassFilter(220))
bus['pad'].x = P.chorus(stage(bus['pad'].x, HighpassFilter(140)), mix=0.4)
bus['reed'].x = P.chorus(stage(bus['reed'].x, HighpassFilter(180)), rate=0.3, mix=0.3)
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(200), PeakFilter(3200, -1.5, 1.0))
bus['harm'].x = stage(bus['harm'].x, HighpassFilter(220))
bus['clap'].x = stage(bus['clap'].x, HighpassFilter(250), LowpassFilter(7000))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(6000), LowpassFilter(14000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(70))
bus['piano'].x = stage(bus['piano'].x, HighpassFilter(110))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(150))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(55), LowpassFilter(2500))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['shim'].x = stage(bus['shim'].x, HighpassFilter(500))

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('NEW02_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        continue
    x = x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    blk = SR // 10
    nb = N // blk
    xb = x[:, :nb * blk].reshape(2, nb, blk)
    act = np.max(np.abs(xb), axis=(0, 2)) > 1e-4
    if np.count_nonzero(act) > 10:
        levels[k] = meter.integrated_loudness(xb[:, act, :].reshape(2, -1).T.astype(np.float64))
    del xb, act
    mix += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

pre = int(0.025 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(300), LowpassFilter(8000),
             Reverb(room_size=0.86, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
verb = verb * duck_soft[None, :]
dly = S.pingpong(delay_send, B(0.75), feedback=0.42, repeats=8, lp=4500, hp=400)
del delay_send
dly = stage(dly, Reverb(room_size=0.5, damping=0.6, wet_level=0.25, dry_level=1.0, width=1.0))
mix = mix + verb * 0.85 + dly * 0.5
del verb, dly

# section dynamics (dB at bar positions, linear in between)
SECTION_DB = [(0, -4.5), (8, -3.5), (16, -2.5), (32, -1.5), (39.9, -1.5), (40, -5.0), (47.9, -4.5),
              (48, -6.0), (55.9, -1.5), (56, 0.0), (71.9, 0.0), (72, 0.3), (79.9, 0.3), (80, -1.5),
              (88, -2.5), (96, -2.5), (100, -2.5)]
sec = curve([(a, 10 ** (d / 20)) for a, d in SECTION_DB], log=False)
mix = mix * sec.astype(np.float32)[None, :]

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(70, -1.0, 0.7), PeakFilter(140, 1.0, 0.8), PeakFilter(3000, 1.5, 0.7),
            HighShelfFilter(10000, -2.0, 0.7),
            Compressor(threshold_db=-15, ratio=2.0, attack_ms=25, release_ms=180))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-1.6, release_ms=90))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-66 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new02.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
