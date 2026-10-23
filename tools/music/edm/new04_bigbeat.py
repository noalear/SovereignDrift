#!/usr/bin/env python3
# new04_bigbeat.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Tritone Run": a NEW big beat / breakbeat track (late-90s Chemical Brothers /
# Crystal Method / Prodigy style) for Stellar Frontier Enhanced.  It is not a
# remix: it is a new piece built from fragments of the original game's song 4
# (music by Stardock, mid/sf-music4.mid) that the chiptune remix
# (song4_chiptune.py) left in the background or used only in passing.
# Everything is synthesized in code with tools/music/edm/synth.py and
# tools/music/edm/synth_bigbeat.py: no samples, no SoundFonts.
#
# Fragments taken from sf-music4.mid (1-indexed bars of the MIDI, 145 BPM):
#   1. the fretless-bass tritone riff  C2 G2 C3 G2 D#2 | F#2 C2 F#2 (ch 6, bars
#      45-56, with the F#1 "dive" of bars 48/56 and the pickup C2 G1 A#1 of bar
#      44).  Here it is the backbone: a distorted analog bass riff all through
#      the track, sequenced onto Ab and Bb (diatonic) for the breakdown and bridge.
#   2. the honky-tonk piano tritone figure  G4 C#4 F#4 C4  (ch 7, bars 15-16 and
#      39-40).  Developed two ways: in diminution as the TB-303 acid line
#      (16ths around the C / F# roots, with accents and slides), and in
#      augmentation on a 3-3-2 rhythm as the lead hook (G4 C#4 F#4 | C4 ...),
#      extended with G#4 and a climb to C5 A#4.
#   3. the piano dyads of the dark middle  C5 over G#4 / F#4  (ch 7/8, bars
#      41-56) and their accent pattern (velocities 114 80 80 102 69 69 91 69:
#      accents on 1, 2-and, 4 = a 3-3-2 tresillo).  Developed as the stabby
#      synth riff (C4 G#4 C5 / C4 F#4 C5 on the tresillo), re-voiced to C minor
#      (C4 D#4 G4 C5) in the second drop.
#   4. transitions: the choir pedal's move C -> F# (bar 13) as the tension
#      pedal of the build; the horn theme's under-used ending D4 D#4 -> G4
#      (bars 31-33) as the bridge melody (D5 D#5 over Bb, G5 over C); the taiko
#      crescendo C4 8ths -> C3 hit (bars 12-13, 16-17) as the build-up and drop
#      hit; the claves rhythm (bars 39-40) in the build and bridge; the snare
#      fill 2-and 2-a 3 3-and (bars 44, 48) as the break fills; the Voice Oohs
#      C7 ping rhythm (bars 17-20) as a quiet sonar ping in the intro/breakdown.
# New: 128 BPM, the big beat form (filtered-break intro, grooves, hook,
# breakdown with an F# tension pedal, drop, bridge, second drop, outro, a
# final C minor hit that loops back to the start), the Ab/Bb harmony, the
# synthesized breakbeat (round-robin "pdlap" snares, ghost notes, swing,
# parallel compression), the 303, distorted bass, stabs, strings and effects.
#
#   python3 tools/music/edm/new04_bigbeat.py OUT.wav [bar0 bar1]
#   NEW04_MUTE=kick,snare,hats,perc,cym,fx   (mute buses, e.g. for the chroma check)
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
import synth_bigbeat as BB  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 128.0
SPB = 60.0 / BPM
NBARS = 88          # plus the final hit on bar 88, beat 1
TAIL = 4.0

BAR0, BAR1 = 0, NBARS + 1
if len(sys.argv) > 3:
    BAR0, BAR1 = int(sys.argv[2]), int(sys.argv[3])
OFF = BAR0 * 4 * SPB
N = int(((min(BAR1, NBARS + 1) - BAR0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


SECTIONS = {'intro': (0, 8), 'grooveA': (8, 16), 'grooveB': (16, 24), 'hook1': (24, 32),
            'breakdown': (32, 36), 'build': (36, 40), 'drop1': (40, 56), 'bridge': (56, 64),
            'drop2': (64, 80), 'outro': (80, 88)}

# ------------------------------------------------------------- harmony
CHORDS = (['Cp'] * 8 +                                   # intro: C pedal
          ['C', 'F#'] * 12 +                             # grooves, hook 1
          ['Ab', 'Ab', 'Bb', 'Bb'] + ['F#p'] * 4 +       # breakdown, build (F# pedal)
          ['C', 'F#'] * 8 +                              # drop 1
          ['Ab', 'Ab', 'Bb', 'Bb', 'C', 'F#', 'C', 'F#'] +   # bridge
          ['C', 'F#'] * 8 +                              # drop 2
          ['C', 'F#'] * 4)                               # outro
assert len(CHORDS) == NBARS, len(CHORDS)
# pitch classes expected in the chroma (for the check script)
PCS = {'Cp': 'C G', 'C': 'C G D# G#', 'F#': 'F# C', 'Ab': 'G# C D#', 'Bb': 'A# D F',
       'F#p': 'F# C G G#', 'Cm': 'C D# G'}

PAD = {'Cp': ['C3', 'G3', 'C4'], 'C': ['C4', 'D#4', 'G4'], 'F#': ['F#3', 'C4', 'F#4'],
       'Ab': ['G#3', 'C4', 'D#4'], 'Bb': ['A#3', 'D4', 'F4'], 'F#p': ['F#3', 'C4', 'F#4']}
SUBROOT = {'Cp': 'C2', 'C': 'C2', 'F#': 'F#1', 'Ab': 'G#1', 'Bb': 'A#1', 'F#p': 'F#1'}

# fragment 1: the fretless tritone riff (beats, note, length)
RIFF = {
    'C':     [(0, 'C2', 1), (1, 'G2', .5), (1.5, 'C3', 1), (2.5, 'G2', 1), (3.5, 'D#2', .5)],
    'F#':    [(0, 'F#2', 1), (1, 'C2', .5), (1.5, 'F#2', 2.5)],
    'F#dive': [(0, 'F#2', 1), (1, 'C2', .5), (1.5, 'F#1', 2.5)],
    'F#pick': [(0, 'F#2', 1), (1, 'C2', .5), (1.5, 'F#2', 1), (2.5, 'C2', .5), (3, 'G1', .5), (3.5, 'A#1', .5)],
    'Ab':    [(0, 'G#1', 1), (1, 'D#2', .5), (1.5, 'G#2', 1), (2.5, 'D#2', 1), (3.5, 'C2', .5)],
    'Bb':    [(0, 'A#1', 1), (1, 'F2', .5), (1.5, 'A#2', 1), (2.5, 'F2', 1), (3.5, 'D2', .5)],
}
PICKUP = [(2.5, 'C2', .5), (3, 'G1', .5), (3.5, 'A#1', .5)]

# fragment 2a: the tritone figure in diminution -> 303 line (16 steps: note, accent, slide)
ACID = {
    'C': [('C2', 1, 0), ('C2', 0, 0), ('C3', 0, 1), ('G2', 0, 0),
          (None, 0, 0), ('C#3', 1, 0), ('C2', 0, 0), ('F#2', 0, 1),
          ('C3', 0, 0), ('C2', 0, 0), ('G2', 1, 0), ('C#3', 0, 1),
          ('F#2', 0, 0), ('C3', 1, 0), ('C2', 0, 0), ('D#2', 0, 1)],
    'F#': [('F#1', 1, 0), ('F#1', 0, 0), ('F#2', 0, 1), ('C2', 0, 0),
           (None, 0, 0), ('C#3', 1, 0), ('F#1', 0, 0), ('G2', 0, 1),
           ('F#2', 0, 0), ('F#1', 0, 0), ('C3', 1, 0), ('C#3', 0, 1),
           ('C2', 0, 0), ('F#2', 1, 0), ('G2', 0, 0), ('A#1', 0, 1)],
}
ACID['Cp'] = ACID['C']
ACID['F#p'] = ACID['F#']
# the diatonic variant for the Ab / Bb bars (C#->D, F#->F, D#->D before transposing)
ACID['Ab'] = ACID['Bb'] = [({'C#3': 'D3', 'F#2': 'F2', 'D#2': 'D2'}.get(n, n), a, s_) for n, a, s_ in ACID['C']]
ACID_TR = {'Cp': 0, 'C': 0, 'F#': 0, 'F#p': 0, 'Ab': -4, 'Bb': -2}

# fragment 2b: the tritone figure augmented on 3-3-2 -> the lead hook (8 bars over C / F#)
HOOK = [(0, 'G4', 1.5), (1.5, 'C#4', 1.5), (3, 'F#4', 1),
        (4, 'C4', 3), (7, 'D#4', .5), (7.5, 'F#4', .5),
        (8, 'G4', 1.5), (9.5, 'G#4', 1.5), (11, 'G4', 1),
        (12, 'F#4', 1.5), (13.5, 'C4', 2.5),
        (16, 'G4', 1.5), (17.5, 'C#4', 1.5), (19, 'F#4', 1),
        (20, 'C4', 3), (23, 'D#4', .5), (23.5, 'F#4', .5),
        (24, 'G4', 1.5), (25.5, 'C5', 1.5), (27, 'A#4', 1),
        (28, 'F#4', 2.5), (30.5, 'D#4', .5), (31, 'C4', 1)]
# fragment 4: the horn theme's ending D-Eb-G, as the bridge line (Ab Ab Bb Bb C F# C F#)
BRIDGE = [(0, 'C5', 6), (6, 'D#5', 1), (7, 'C5', 1), (8, 'D5', 4), (12, 'D#5', 4),
          (16, 'G5', 4), (20, 'F#5', 2), (22, 'C5', 2),
          (24, 'G4', 1.5), (25.5, 'C#4', 1.5), (27, 'F#4', 1),
          (28, 'C4', 2.5), (30.5, 'D#4', .5), (31, 'F#4', 1)]
# fragment 3: the piano dyads on their 3-3-2 accents -> stabs
STAB = {'C': ['C4', 'G#4', 'C5'], 'F#': ['C4', 'F#4', 'C5'], 'Cm': ['C4', 'D#4', 'G4', 'C5'],
        'Ab': ['C4', 'D#4', 'G#4'], 'Bb': ['D4', 'F4', 'A#4']}
TRESILLO = [(0, 1.0), (1.5, 0.82), (3, 0.9)]
# fragment 4: claves (orig bars 39-40, 2 bars) and the Voice Oohs ping cycle
CLAVES = [0, .5, .75, 1, 2.5, 3, 4 + 1.5, 4 + 1.75, 4 + 2, 4 + 2.5, 4 + 3, 4 + 3.5]
PING_A = [0, .25, .75, 1.25, 1.75, 2.25, 3.25, 3.75]
PING_B = [2, 2.25, 2.75, 3.25, 3.75]
PING_C = [.25, 1.25, 1.75]
PING_CYCLE = [PING_C, PING_A, PING_B, PING_C]

# --------------------------------------------------------------- buses
names = ['kick', 'snare', 'hats', 'perc', 'cym', 'fx', 'sub', 'bass', 'acid', 'stab', 'lead', 'pad', 'bell']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}
RNG = np.random.default_rng(404)


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, sig, t, gain=1.0, pan=0.0):
    t = t - OFF
    if t < -20 or t > N / SR:
        return
    sig = np.asarray(sig)
    if sig.ndim == 1:
        sig = S.pan2(sig, pan)
    bus[name].add(sig, t, gain)


def in_range(bar):
    return BAR0 - 2 <= bar < BAR1 + 1


# ---------------------------------------------------------------- drums
_sn = [0]


def snare_hit(t, vel, ghost=False):
    v = _sn[0] % 4
    _sn[0] += 1
    sig = cached(('sn', v, ghost), lambda: BB.bb_snare(1.0, v, ghost))
    put('snare', sig, t + RNG.normal(0, 0.0015), vel * (0.93 + 0.14 * RNG.random()))


def kick_hit(t, vel=1.0):
    put('kick', cached(('kick',), BB.bb_kick), t, vel)
    kicks.append(t - OFF)


PAT = {
    'A0': dict(K=[0, .75, 2.5], S=[1, 3], G=[2.25, 3.75]),
    'A1': dict(K=[0, 1.75, 2.5], S=[1, 3], G=[.5, 3.25, 3.5]),
    'B0': dict(K=[0, .75, 1.5, 2.5], S=[1, 3], G=[1.75, 2.25, 3.75]),
    'B1': dict(K=[0, .5, 1.75, 2.5], S=[1, 3], G=[2.75, 3.25]),
}
SWING = 0.03   # beats of delay on the off 16ths


def sw(beat):
    return beat + (SWING if abs((beat * 4) % 2 - 1) < 1e-6 else 0.0)


def drum_bar(bar, style='A', fill=None, hats=1.0, shaker=0.0, open_hat=0.0, vel=1.0, ghosts=1.0):
    if not in_range(bar):
        return
    pat = PAT[style + str(bar % 2)]
    for b in pat['K']:
        kick_hit(T(bar, sw(b)), vel * (1.0 if b == 0 else 0.88))
    snares = list(pat['S'])
    gh = list(pat['G'])
    if fill == 4:      # orig bar 44: 2-and 2-a 3 3-and
        snares = [1] + []
        gh = [g for g in gh if g < 2.5]
        for b, v in ((2.5, .55), (2.75, .62), (3, .9), (3.5, 1.0)):
            snare_hit(T(bar, sw(b)), vel * v)
    elif fill == 8:    # orig bar 48: louder, with an open hat lift
        snares = [1]
        gh = [g for g in gh if g < 2.5]
        for b, v in ((2.5, .7), (2.75, .75), (3, 1.0), (3.25, .6), (3.5, 1.05)):
            snare_hit(T(bar, sw(b)), vel * v)
    for b in snares:
        snare_hit(T(bar, b), vel * (1.0 if b == 3 else 0.95))
    for b in gh:
        snare_hit(T(bar, sw(b)), vel * 0.22 * ghosts, ghost=True)
    if hats:
        for i in range(16):
            b = i * 0.25
            acc = (0.75, 0.32, 0.55, 0.38)[i % 4]
            if open_hat and b == 3.5 and bar % 2 == 1:
                continue
            put('hats', cached(('hat', i % 3), lambda: BB.bb_hat(1.0, False, 0.15)),
                T(bar, sw(b)) + RNG.normal(0, 0.0015), hats * acc * (0.9 + 0.2 * RNG.random()))
    if open_hat and bar % 2 == 1:
        put('hats', cached(('ohat',), lambda: BB.bb_hat(1.0, True, -0.1)), T(bar, 3.5), open_hat)
    if shaker:   # the cabasa: 8ths, plus a 16th pair at 3.5 / 3.75
        for b in (0, .5, 1, 1.5, 2, 2.5, 3, 3.5, 3.75):
            acc = 1.0 if b in (0, 2) else (0.75 if b % 1 == 0 else 0.6)
            put('perc', cached(('shk', int(b * 4) % 2), lambda: BB.shaker(1.0, -0.35)),
                T(bar, sw(b)), shaker * acc * (0.9 + 0.2 * RNG.random()))


def drums(bar0, bar1, style='A', **kw):
    for b in range(bar0, bar1):
        fill = None
        if (b + 1) % 8 == 0:
            fill = 8
        elif (b + 1) % 4 == 0:
            fill = 4
        drum_bar(b, style, fill if kw.get('fills', True) else None,
                 **{k: v for k, v in kw.items() if k != 'fills'})


def crash_at(bar, vel=1.0):
    put('cym', cached(('crash',), lambda: S.crash(1.0, 2.6)), T(bar), vel)


def claves(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1, 2):
        for c in CLAVES:
            if b + c / 4 < bar1:
                put('perc', cached(('clv',), BB.claves), T(b, sw(c)), vel * (1.0 if c % 1 == 0 else 0.75))


def taiko_hit(t, midi, vel=1.0, decay=0.35):
    put('perc', cached(('tk', midi, decay), lambda: BB.taiko(midi, 1.0, decay)), t, vel)


# ---------------------------------------------------------------- bass
def bass_riff(bar0, bar1, vel=1.0, dive_every=0, pickup_end=True):
    for b in range(bar0, bar1):
        if not in_range(b):
            continue
        ch = CHORDS[b]
        key = 'C' if ch in ('C', 'Cp') else ('F#' if ch in ('F#', 'F#p') else ch)
        if key == 'F#':
            if pickup_end and (b + 1) % 8 == 0:
                key = 'F#pick'
            elif dive_every and (b + 1) % dive_every == 0:
                key = 'F#dive'
        for beat, n, dur in RIFF[key]:
            md = m(n)
            d = B(dur) * 0.9
            put('bass', cached(('db', md, round(dur, 2)), lambda: BB.dist_bass(md, d)), T(b, beat), vel)
            put('sub', cached(('sb', md, round(dur, 2)), lambda: BB.sub(md, d)), T(b, beat), vel)


def bass_hold(bar0, bar1, vel=1.0, dist=0.4):
    """Whole-bar roots (breakdown)."""
    for b in range(bar0, bar1):
        md = m(SUBROOT[CHORDS[b]])
        put('sub', BB.sub(md, B(4) - 0.03), T(b), vel)
        if dist:
            put('bass', BB.dist_bass(md + 12, B(3.5), env=900, decay=0.3, drive=2.2), T(b), vel * dist)


# ---------------------------------------------------------------- acid
ACID_CUT = [(0, 200), (4, 220), (8, 380), (16, 600), (24, 800), (31.9, 950), (32, 520), (36, 480),
            (39.75, 1900), (40, 820), (55.9, 1100), (56, 950), (63.9, 1300), (64, 900), (79.9, 1500),
            (80, 1000), (88, 380)]


def acid_cut_at(bars):
    xs = [p[0] for p in ACID_CUT]
    ys = np.log([p[1] for p in ACID_CUT])
    return np.exp(np.interp(bars, xs, ys))


def acid_block(bar0, bar1, gain=1.0, oct_mod=2.2, reso=3.2, drive=1.8, transpose=12, stop_beat=4.0):
    b0, b1 = max(bar0, BAR0 - 1), min(bar1, BAR1)
    if b0 >= b1:
        return
    step = B(0.25)
    notes = []
    for b in range(b0, b1):
        ch = CHORDS[b]
        for i, (n, acc, sl) in enumerate(ACID[ch]):
            if n is None:
                continue
            if b == bar1 - 1 and i * 0.25 >= stop_beat:
                break
            last = (b == b1 - 1 and i == 15) or (b == bar1 - 1 and (i + 1) * 0.25 >= stop_beat)
            sl = bool(sl) and not last
            md = m(n) + ACID_TR[ch] + transpose
            notes.append((T(b, i * 0.25) - T(b0), step if sl else step * 0.55, md, bool(acc), sl))
    length = T(b1) - T(b0) + 0.5
    nn = int(length * SR)
    bars_axis = b0 + np.arange(nn) / SR / (4 * SPB)
    cut = acid_cut_at(bars_axis)
    y = BB.acid(notes, length, cut, reso=reso, oct_mod=oct_mod, drive=drive)
    put('acid', np.stack([y, y]), T(b0), gain)


# ---------------------------------------------------------------- synths
def lead_phrase(bar0, notes, transpose=0, gain=1.0, cut=1700.0, env=2300.0, target='lead'):
    if not (BAR0 - 8 <= bar0 < BAR1):
        return
    ev = []
    for i, (beat, n, dur) in enumerate(notes):
        nxt = notes[i + 1][0] if i + 1 < len(notes) else None
        leg = nxt is not None and abs(beat + dur - nxt) < 1e-6 and dur <= 0.5
        ev.append((B(beat), B(dur) * (1.0 if leg else 0.9), m(n) + transpose, leg))
    length = B(notes[-1][0] + notes[-1][2]) + 1.0
    y = BB.mono_lead(ev, length, cut=cut, env=env)
    put(target, y, T(bar0), gain)


def stabs(bar0, bar1, vel=1.0, extra=False, cm_every=0):
    for b in range(bar0, bar1):
        if not in_range(b):
            continue
        ch = CHORDS[b]
        key = 'C' if ch in ('C', 'Cp') else ('F#' if ch in ('F#', 'F#p') else ch)
        if key == 'C' and cm_every and (b // 2) % cm_every == 1:
            key = 'Cm'
        hits = TRESILLO + ([(3.5, 0.55)] if extra else [])
        for beat, v in hits:
            mids = tuple(m(n) for n in STAB[key])
            sig = cached(('stab', mids), lambda: BB.stab(list(mids), B(0.3)))
            put('stab', sig, T(b, beat), vel * v)


def pads(bar0, bar1, vel=1.0, a=0.6, cut=2400.0, extra=None):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if in_range(b) or in_range(e - 1):
            dur = T(e) - T(b)
            for n in PAD[ch] + (extra or []):
                put('pad', BB.strings(m(n), dur, 1.0, a=a, cut=cut), T(b), vel)
        b = e


def pings(bar0, bar1, vel=1.0, note='C6'):
    for b in range(bar0, bar1):
        if not in_range(b):
            continue
        for k, beat in enumerate(PING_CYCLE[b % 4]):
            put('bell', cached(('ping', note), lambda: BB.glass(m(note), B(0.25), r=0.2)), T(b, beat),
                vel * (0.8 + 0.2 * (k == 0)))


def bell_line(bar0, notes, vel=1.0, transpose=0):
    for beat, n, dur in notes:
        md = m(n) + transpose
        put('bell', BB.glass(md, B(dur), r=0.8), T(bar0, beat), vel)


# ---------------------------------------------------------- arrangement

# INTRO 0-7: the C pedal, filtered break (opens at 8), sonar pings, the 303 creeping in
pads(0, 8, vel=0.5, a=2.5, cut=1500, extra=['C6'])
drums(0, 8, 'A', hats=0.7, fills=False)
pings(0, 8, vel=0.5)
acid_block(4, 8, gain=0.8)
for i, b in enumerate((2, 2.5, 3, 3.5)):          # orig bar 12: taiko C 8ths, crescendo
    taiko_hit(T(7, b), m('C3'), 0.35 + 0.18 * i)
for beat, n, dur in PICKUP:
    md = m(n)
    put('bass', BB.dist_bass(md, B(dur) * 0.9), T(7, beat), 0.9)
    put('sub', BB.sub(md, B(dur) * 0.9), T(7, beat), 0.9)
put('fx', S.noise_riser(T(8) - T(6), 400, 6000, 2.0), T(6), 0.35)

# GROOVE A 8-15: the break opens, the riff, the 303
crash_at(8)
taiko_hit(T(8), m('C2'), 0.9, 0.5)
drums(8, 16, 'A', hats=0.8)
bass_riff(8, 16)
acid_block(8, 24, gain=0.9)

# GROOVE B 16-23: + stabs (the piano dyads' tresillo), shaker
crash_at(16, 0.7)
drums(16, 24, 'A', hats=0.85, shaker=0.6)
bass_riff(16, 24)
stabs(16, 24, vel=0.85)

# HOOK 1 24-31: the lead hook (tritone figure augmented), driving break
crash_at(24, 0.9)
drums(24, 32, 'B', hats=0.9, shaker=0.6, open_hat=0.35)
bass_riff(24, 32, dive_every=4)
acid_block(24, 32, gain=0.85)
lead_phrase(24, HOOK, gain=0.9, cut=1400, env=2000)
stabs(24, 32, vel=0.6)

# BREAKDOWN 32-35: no drums; Ab - Bb strings, the bridge line foreshadowed on bells
crash_at(32, 0.6)
put('fx', S.downlifter(B(8)), T(32), 0.35)
pads(32, 36, vel=0.75, a=0.8, cut=2000)
bass_hold(32, 36, vel=0.85, dist=0.35)
acid_block(32, 40, gain=0.75, oct_mod=2.4, stop_beat=3.0)
bell_line(32, BRIDGE[:5], vel=0.65)
pings(32, 36, vel=0.35)

# BUILD 36-39: the F# tension pedal, filtered break returning, claves, the
# original's G-G# ostinato as stabs, taiko crescendo, then a beat of air
pads(36, 40, vel=0.7, a=1.5, cut=2200)
for b in range(36, 39):
    drum_bar(b, 'A', None, hats=0.6, ghosts=0.6)
if in_range(39):                                     # bar 39: the break stops on beat 3
    for b in (0, .75, 2):
        kick_hit(T(39, sw(b)), 0.9)
    for b, v in ((1, .95), (1.75, .5), (2.5, .8), (2.75, .85)):
        snare_hit(T(39, sw(b)), v)
claves(36, 40, vel=0.7)
for b in range(36, 40):
    for i in range(8):
        beat = i * 0.5
        if b == 39 and beat >= 3:
            break
        u = ((b - 36) * 8 + i) / 32
        md = m('G4') if i % 2 == 0 else m('G#4')
        put('stab', cached(('ost', md), lambda: BB.stab([md, md - 12], B(0.22), cut=700, env=2500)),
            T(b, beat), 0.25 + 0.45 * u)
    put('sub', BB.sub(m('F#1'), B(4) - 0.03), T(b), 0.8 if b < 39 else 0.0)
for b in range(36, 39):
    for i in range(8):
        put('bass', cached(('dbF8',), lambda: BB.dist_bass(m('F#1'), B(0.4), env=1200)), T(b, i * 0.5),
            0.4 + 0.5 * ((b - 36) * 8 + i) / 24)
for i in range(8):                                   # taiko 8ths crescendo (orig bars 12, 16)
    taiko_hit(T(38, i * 0.5), m('C3'), 0.3 + 0.06 * i)
for i in range(6):
    taiko_hit(T(39, i * 0.5), m('C3') if i % 2 == 0 else m('G2'), 0.75 + 0.05 * i)
put('fx', S.noise_riser(T(39, 3) - T(36), 300, 8000, 2.0), T(36), 0.45)
put('cym', S.reverse_cymbal(B(3)), T(39) , 0.55)

# DROP 1 40-55
crash_at(40, 1.0)
crash_at(48, 0.8)
taiko_hit(T(40), m('C2'), 1.0, 0.6)
put('fx', S.impact(), T(40), 0.7)
drums(40, 56, 'B', hats=1.0, shaker=0.7, open_hat=0.4)
bass_riff(40, 56, dive_every=4)
acid_block(40, 56, gain=0.85, drive=2.0)
stabs(40, 56, vel=0.85)
lead_phrase(40, HOOK, gain=1.0)
lead_phrase(48, HOOK, gain=1.0)

# BRIDGE 56-63: Ab Ab Bb Bb C F# C F#, the horn ending D-Eb-G as the lead, claves
crash_at(56, 0.85)
drums(56, 64, 'A', hats=0.75, open_hat=0.3)
claves(56, 64, vel=0.6)
bass_riff(56, 64)
pads(56, 64, vel=0.5, a=0.4, cut=2200)
acid_block(56, 64, gain=0.75, drive=2.0)
stabs(56, 64, vel=0.55)
lead_phrase(56, BRIDGE, gain=0.95)
put('fx', S.noise_riser(T(64) - T(62), 500, 7000, 2.0), T(62), 0.35)

# DROP 2 64-79: hook doubled an octave up, stabs re-voiced (Cm) every other C
crash_at(64, 1.0)
crash_at(72, 0.85)
taiko_hit(T(64), m('C2'), 1.0, 0.6)
put('fx', S.impact(), T(64), 0.75)
drums(64, 80, 'B', hats=1.0, shaker=0.75, open_hat=0.45)
bass_riff(64, 80, dive_every=4)
acid_block(64, 80, gain=0.9, drive=2.2, oct_mod=2.4)
stabs(64, 80, vel=0.85, extra=True, cm_every=2)
for b in (64, 72):
    lead_phrase(b, HOOK, gain=1.0)
    lead_phrase(b, HOOK, transpose=12, gain=0.38, cut=2200, env=1800)
for b in range(65, 80, 2):                           # taiko answers on the F# bars
    taiko_hit(T(b, 3.5), m('F#2'), 0.45)

# OUTRO 80-87: riff, break and 303 thinning; a final C minor hit on bar 88
crash_at(80, 0.85)
drums(80, 88, 'A', hats=0.8, shaker=0.5)
bass_riff(80, 88)
acid_block(80, 88, gain=0.8)
stabs(80, 84, vel=0.6)
pings(84, 88, vel=0.35)
# final hit (bar 88, beat 1): kick + snare, C2 bass, C minor stab, crash, taiko, impact
if in_range(88):
    kick_hit(T(88), 1.0)
    snare_hit(T(88), 0.9)
    crash_at(88, 0.9)
    taiko_hit(T(88), m('C2'), 1.0, 0.7)
    put('fx', S.impact(), T(88), 0.6)
    put('bass', BB.dist_bass(m('C2'), B(2), env=1500, decay=0.25), T(88), 0.6)
    put('sub', BB.sub(m('C2'), B(3)), T(88), 1.0)
    put('stab', BB.stab([m(n) for n in STAB['Cm']], B(1.5), env=3000, decay=0.12), T(88), 1.0)
    for n in ('C4', 'D#4', 'G4', 'C5'):
        put('pad', BB.strings(m(n), B(2.5), a=0.01, r=0.7), T(88), 0.9)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


def filt_region(name, bar0, bar1, f0, f1, curve=1.6):
    """Low-pass sweep f0 -> f1 on a bus between two bars (filtered-break effect)."""
    i0 = max(0, int((T(bar0) - OFF) * SR))
    i1 = min(N, int((T(bar1) - OFF) * SR))
    if i1 <= i0:
        return
    full = int((T(bar1) - T(bar0)) * SR)
    u = (np.arange(i0, i1) - int((T(bar0) - OFF) * SR)) / full
    fc = f0 * (f1 / f0) ** (u ** curve)
    seg = bus[name].x[:, i0:i1]
    bus[name].x[:, i0:i1] = S.svf(seg, fc, 0.9).astype(np.float32)


for k in ('kick', 'snare', 'hats', 'perc'):
    filt_region(k, 0, 8, 380, 2400, 1.0)
    filt_region(k, 36, 40, 500, 9000, 1.8)

duck = S.sidechain(N, kicks, depth=0.35, release=0.18)
duck_soft = S.sidechain(N, kicks, depth=0.18, release=0.16)

GAIN = {  # dB
    'kick': -2.0, 'snare': 0.5, 'hats': 1.0, 'perc': -5.0, 'cym': -17.0, 'fx': -15.0,
    'sub': -6.5, 'bass': -3.5, 'acid': -9.0, 'stab': -2.5, 'lead': -6.0, 'pad': -8.0, 'bell': -7.0,
}
DUCK = {'sub': duck, 'bass': duck, 'pad': duck, 'acid': duck_soft, 'stab': duck_soft}
VERB = {'snare': .08, 'perc': .15, 'stab': .25, 'lead': .16, 'pad': .4, 'bell': .45, 'fx': .3, 'acid': .06}
DELAY = {'lead': .14, 'bell': .35, 'acid': .12, 'stab': .1, 'perc': .05}

bus['hats'].x = stage(bus['hats'].x, HighpassFilter(4000), LowpassFilter(11000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(60))
bus['bass'].x = stage(bus['bass'].x, PeakFilter(300, -3.5, 0.8), PeakFilter(700, -1.5, 1.0), LowpassFilter(4000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['acid'].x = stage(bus['acid'].x, HighpassFilter(190), PeakFilter(280, -3.0, 0.9), LowpassFilter(5500))
bus['stab'].x = stage(bus['stab'].x, HighpassFilter(250), PeakFilter(350, -2.0, 1.0), LowpassFilter(6500))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(200), PeakFilter(2200, 1.5, 0.9), LowpassFilter(7500))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(150), Chorus(rate_hz=0.6, depth=0.35, centre_delay_ms=7, mix=0.5))
bus['bell'].x = stage(bus['bell'].x, HighpassFilter(400), LowpassFilter(8000))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(60))

mix = np.zeros((2, N), np.float32)
drum_sum = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('NEW04_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in names:
    if k in MUTE:
        bus[k].x = None
        continue
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    bus[k].x = None
    if k in DUCK:
        x = x * DUCK[k][None, :]
    nz = np.nonzero(np.max(np.abs(x), axis=0) > 1e-4)[0]
    levels[k] = meter.integrated_loudness(x[:, nz].T.astype(np.float64)) if len(nz) > SR else -99.0
    if k in ('kick', 'snare', 'hats', 'perc'):
        drum_sum += x
    else:
        mix += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

# the "phat" break: parallel compression and a little saturation
squash = stage(drum_sum, Compressor(threshold_db=-24, ratio=6.0, attack_ms=6, release_ms=70))
squash = np.tanh(squash * 2.2) / 2.2
drum_sum = stage(drum_sum, Compressor(threshold_db=-12, ratio=2.5, attack_ms=12, release_ms=90))
drum_sum = drum_sum + squash * np.float32(0.9)
del squash
mix += drum_sum
del drum_sum

pre = int(0.015 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(300), LowpassFilter(7500),
             Reverb(room_size=0.7, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
dly = S.pingpong(delay_send, B(0.75), feedback=0.38, repeats=6, lp=4000)
del delay_send
mix = mix + verb * 0.8 + dly * 0.55
del verb, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -3.5), (7.9, -2.5), (8, -3.0), (16, -2.5), (23.9, -2.5), (24, -1.5), (31.9, -1.5),
              (32, -4.5), (35.9, -4.0), (36, -5.0), (39.9, -1.5), (40, 0), (55.9, 0), (56, -2.0),
              (63.9, -1.5), (64, 0.3), (79.9, 0.3), (80, -1.5), (87.9, -2.0), (88, -0.5), (96, -0.5)]
bars_axis = BAR0 + np.arange(N) / SR / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(30))
# mono below ~120 Hz
lo = S.butter(mix, 120, 'low', 2).astype(np.float32)
lo_m = lo.mean(axis=0, keepdims=True)
mix = mix - lo + lo_m
del lo, lo_m
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(80, 0.5, 0.7), PeakFilter(260, -2.0, 0.7), HighShelfFilter(6000, 1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=25, release_ms=160))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.3 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new04_bigbeat.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, relative to kick):')
ref = levels.get('kick', 0.0)
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}  {v - ref:+5.1f}')
