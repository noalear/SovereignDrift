#!/usr/bin/env python3
# song2_synthwave.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 2, Synthwave": a 1980s synthwave / outrun remix of sf-music2 (the
# original game's music by Stardock, from mid/sf-music2.mid), produced
# entirely in code with tools/music/edm/synth.py and synth_synthwave.py:
# synthesized analog-style brass, Juno pads, string machine, choir, arps,
# octave bass and a LinnDrum-style kit with gated reverb, mixed, run through
# a tape stage and mastered here.  No samples are used.
#
# Taken from the original song (C# major, Ab minor middle; bar numbers are
# the MIDI's, 1-based; it has no pickup, bar 1 is the downbeat):
#   - the march ostinato rhythm  1 . trip-trip-trip . 3 . 4  (fretless bass on
#     C#2 and French horn on the fifth Ab3, bars 1-40 and 57-60): on a muted
#     synth-brass ostinato all through, and on a fretless bass in the breakdown
#   - the C# C# A B progression (bVI bVII I) of bars 1-24, and the string pedal C#6
#   - the synth-brass hook  Ab4 C#5 Ab5 | F#5 . . F#5E5Eb5 | E5 . . E5Eb5C#5 | F5
#     with its harmony line (bars 6-9 and 18-21), now the chorus on a big
#     detuned-saw brass lead; and the harp run of bar 18 as its answer
#   - the flute lines (bars 2-3, 11-13, 16-17, 58-59), the oboe calls (bars 5-6,
#     22-23) and the trombone fanfare C#4 C#4 Eb4 F4 (bar 14)
#   - the Ab minor section, F#m / Abm (bars 25-40): the string melody
#     Ab5 | F#5 E5 Eb5 C#5 | Eb5 B4 | Ab4 with its run to B5 and the climb to C#6
#   - the C# / F#m section (bars 41-56): the harp 8th arpeggio (as the Juno arp),
#     the clarinet two-voice line, the rising choir line C#4 ... A5, the
#     timpani pickups (as tuned toms)
#   - the timpani figure C#3 . . C#3-Ab2-C#3 (as toms), the march snare
#     patterns (as fills and in the build), the timpani/cymbal roll of bar 40
#   - the coda (bars 57-60): the ostinato and the flute line in C#
# New: 110 BPM (from 135), straight 8th octave bass, LinnDrum-style kit
# (tuned kick, gated-reverb snare, claps, 16th hats, tom fills), Juno pads,
# arpeggios, a synthwave form (intro, verse, chorus, breakdown, build, anthem,
# key-lift final chorus in D, outro back home in C#), B7sus4 under the hook's
# second chord, tape wow and saturation, a darker top end.
#
# v2 (2026-10-14, after the user's listening): the gated-noise snare and the
# bright clap are replaced by SW.snare2, a rounded tuned "pdlap" (Ab3 body with a
# fast pitch drop, 1-4 kHz snap, woody shell, small clap layer inside the hit, a
# short dark gated room), 4 round-robin variants with velocity-dependent snap,
# sitting about 1.5 LU under the lead; the march snare uses the same voice (Bb3,
# dry), the bar-47 roll is 16ths, and the outro has no march snare.
#
#   python3 tools/music/edm/song2_synthwave.py OUT.wav [bar0 bar1]
#   SONG2_MUTE=kick,snare,...   mutes buses (for the chroma check)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_synthwave as SW  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 110.0
SPB = 60.0 / BPM
NBARS = 89          # 88 bars + the final hit
TAIL = 6.0


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
    'C#':   ['Ab3', 'C#4', 'F4', 'C#5'],
    'A':    ['A3', 'C#4', 'E4', 'A4'],
    'B':    ['B3', 'Eb4', 'F#4', 'B4'],
    'Bsus': ['B3', 'E4', 'F#4', 'A4'],
    'F#m':  ['A3', 'C#4', 'F#4', 'A4'],
    'Abm':  ['B3', 'Eb4', 'Ab4', 'B4'],
    'Bb':   ['Bb3', 'D4', 'F4', 'Bb4'],
    'Csus': ['C4', 'F4', 'G4', 'Bb4'],
    'D':    ['A3', 'D4', 'F#4', 'A4'],
}
ROOT = {'C#': 37, 'A': 33, 'B': 35, 'Bsus': 35, 'F#m': 30, 'Abm': 32, 'Bb': 34, 'Csus': 36, 'D': 38}
ROOT_FRETLESS = {'C#': 37, 'A': 33, 'B': 35, 'Bsus': 35, 'F#m': 42, 'Abm': 44}   # the original's octave
OST = {'C#': 'Ab3', 'A': 'A3', 'B': 'F#3', 'Bsus': 'F#3', 'F#m': 'F#3', 'Abm': 'Ab3',
       'Bb': 'Bb3', 'Csus': 'G3', 'D': 'A3'}
ARP = {  # the harp figure of bars 41-44, carried onto every chord (8 eighths)
    'C#':   ['Ab3', 'C#4', 'Eb4', 'F4', 'Ab4', 'F4', 'Eb4', 'C#4'],
    'F#m':  ['A3', 'C#4', 'F#4', 'A4', 'C#5', 'A4', 'F#4', 'C#4'],
    'A':    ['A3', 'C#4', 'E4', 'A4', 'C#5', 'A4', 'E4', 'C#4'],
    'B':    ['B3', 'Eb4', 'F#4', 'B4', 'Eb5', 'B4', 'F#4', 'Eb4'],
    'Bsus': ['B3', 'E4', 'F#4', 'A4', 'B4', 'A4', 'F#4', 'E4'],
    'Abm':  ['Ab3', 'B3', 'Eb4', 'Ab4', 'B4', 'Ab4', 'Eb4', 'B3'],
    'Bb':   ['Bb3', 'D4', 'F4', 'Bb4', 'D5', 'Bb4', 'F4', 'D4'],
    'Csus': ['C4', 'F4', 'G4', 'Bb4', 'C5', 'Bb4', 'G4', 'F4'],
    'D':    ['A3', 'D4', 'E4', 'F#4', 'A4', 'F#4', 'E4', 'D4'],
}

CHORDS = (['C#'] * 6 + ['A', 'B'] +                       # 0-7    intro      (orig 1-8)
          ['C#', 'C#', 'A', 'B'] * 3 + ['C#'] * 4 +       # 8-23   verse      (orig 9-24)
          ['A', 'Bsus', 'C#', 'C#'] * 2 +                 # 24-31  chorus 1   (hook, orig 6-9 / 18-21)
          ['F#m', 'F#m', 'Abm', 'Abm'] * 2 +              # 32-39  breakdown  (orig 25-32)
          ['F#m', 'F#m', 'Abm', 'Abm'] * 2 +              # 40-47  build      (orig 33-40)
          ['C#', 'C#', 'F#m', 'F#m'] * 3 + ['C#', 'C#', 'F#m', 'A'] +   # 48-63 anthem (orig 41-56)
          ['Bb', 'Csus', 'D', 'D'] * 4 +                  # 64-79  final chorus, lifted to D
          ['A', 'Bsus'] + ['C#'] * 6 +                    # 80-87  outro      (orig 57-60)
          ['C#'])                                         # 88     final hit
assert len(CHORDS) == NBARS

# --------------------------------------------------------------- themes
# (beat, note, length in beats), relative to a bar

TRIP = 1.0 / 3.0
MARCH_RHY = [(0, .5, 1.0), (1, TRIP, .62), (1 + TRIP, TRIP, .55), (1 + 2 * TRIP, TRIP, .7),
             (2, .5, .9), (3, .5, .8)]                     # bass + horn, every bar of 1-40

INTRO_FLUTE = [(6, 'C#5', 1), (7, 'F5', 1), (8, 'Ab5', 7.5)]           # bars 2-3 (+8va)
OBOE_1 = [(3, 'Eb5', 1), (4, 'F5', 4)]                                 # bars 5-6 (+8va)
VERSE_FLUTE = [(3, 'C#5', 1), (4, 'B4', 1), (5, 'A4', 1), (6, 'Ab4', 1), (7, 'F#4', 1),
               (8, 'F4', .25), (8.5, 'F4', .25), (8.75, 'F#4', .25), (9, 'Ab4', 6.5)]  # bars 11-13
TROMBONE = [(0, 'C#4', .25), (.5, 'C#4', .25), (.75, 'Eb4', .25), (1, 'F4', 2.8)]     # bar 14
FLUTE_2 = [(2, 'Ab4', 1), (3, 'F#4', 1), (4, 'Ab4', 7.5)]                            # bars 16-17
HARP_RUN = [(0, 'C#5', .25), (.25, 'Ab4', .25), (.5, 'F4', .25), (.75, 'C#4', .25),
            (1, 'Ab4', .25), (1.25, 'F4', .25), (1.5, 'C#4', .25), (1.75, 'Ab3', .25),
            (2, 'C#4', .5)]                                                          # bar 18
OBOE_2 = [(3, 'Eb5', 1), (4, 'F5', 5.8)]                                             # bars 22-23

# the hook, relative to the A bar (pickup in the bar before)
HOOK_PICK = [(-1.5, 'Ab4', .45), (-1, 'C#5', .4), (-.5, 'Ab5', .48)]
HOOK_BODY = [(0, 'F#5', 3), (3, 'F#5', TRIP), (3 + TRIP, 'E5', TRIP), (3 + 2 * TRIP, 'Eb5', TRIP),
             (4, 'E5', 3), (7, 'E5', TRIP), (7 + TRIP, 'Eb5', TRIP), (7 + 2 * TRIP, 'C#5', TRIP)]
HARM_PICK = [(-1.5, 'C#4', .45), (-1, 'Ab4', .4), (-.5, 'C#5', .48)]
HARM_BODY = [(0, 'B4', 3), (3, 'B4', TRIP), (3 + TRIP, 'A4', TRIP), (3 + 2 * TRIP, 'Ab4', TRIP),
             (4, 'A4', 3), (7, 'A4', TRIP), (7 + TRIP, 'F#4', TRIP), (7 + 2 * TRIP, 'E4', TRIP)]

# Ab minor section, 4-bar phrases (bars 25-40)
PH = [(3, 'Ab5', 1), (4, 'F#5', 1), (5, 'E5', 1), (6, 'Eb5', 1), (7, 'C#5', 1)]
P1 = PH + [(8, 'Eb5', 3), (11, 'B4', 1), (12, 'Ab4', 4)]
P2 = PH + [(8, 'Eb5', 3), (11, 'F#5', 1), (12, 'Ab5', 4)]
P3 = PH + [(8, 'Eb5', 2.5), (10.5, 'E5', .125), (10.625, 'F#5', .125), (10.75, 'Ab5', .125),
           (10.875, 'A5', .125), (11, 'B5', 1), (12, 'Ab5', 4)]
P4 = PH + [(8, 'Eb5', 3), (11, 'F#5', 1), (12, 'Ab5', 2), (14, 'B5', 2)]

# C# / F#m section (bars 41-56)
CLAR_TOP = [(0, 'F4', 4), (4, 'F4', 2), (6, 'Ab4', 2), (8, 'F#4', 4), (12, 'A4', 2), (14, 'F#4', 2)]
CLAR_LOW = [(0, 'Ab3', 4), (4, 'Ab3', 2), (6, 'B3', 2), (8, 'A3', 4), (12, 'C#4', 2), (14, 'A3', 2)]
CHOIR_LINE = [(30, 'C#4', 2), (32, 'F4', 3), (35, 'F#4', 1), (36, 'Ab4', 2), (38, 'F4', 2),
              (40, 'F#4', 3), (43, 'Ab4', 1), (44, 'A4', 2), (46, 'C#5', 2), (48, 'F5', 3),
              (51, 'F#5', 1), (52, 'Ab5', 2), (54, 'F5', 2), (56, 'A5', 8)]

FLUTE_CODA = [(6, 'C#5', 1), (7, 'F5', 1), (8, 'Ab5', 12)]          # bars 58-59 (+8va)

# march snare figures from the drum part: (beat, velocity), 4 bars each
MARCH_A = [[(0, 1), (1.5, .6), (1.75, .6), (2, .9), (2.5, .9)],
           [(0, 1), (1.5, .8), (1.75, .8), (2, .9)],
           [(0, 1), (1.5, .8), (1.75, .8), (2, .9), (3.5, .8), (3.75, .85)],
           [(0, 1), (1, .6), (1.5, .6), (2, .65), (3, .7), (3 + TRIP, .85), (3 + 2 * TRIP, 1.0)]]
MARCH_B = [[(0, 1), (1.5, .5), (1.75, .5), (2, .9), (2.5, .8), (3, .9), (3.5, .8)],
           [(0, 1), (1, .8), (1.25, .7), (1.5, .8), (1.75, .7), (2, .9), (3, .9)],
           [(0, 1), (1, .8), (1 + TRIP, .7), (1 + 2 * TRIP, .8), (2, .9), (2.5, .8), (3, .9), (3.5, .8)],
           [(0, 1), (1, .8), (1.25, .7), (1.5, .8), (1.75, .7), (2, .9)]]

# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'march', 'hats', 'toms', 'fx', 'bass', 'pad', 'strings', 'vox',
         'arp', 'ost', 'lead', 'lead2', 'solo', 'bell']
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


INST = {
    'brass': SW.brass,
    'solo': SW.solo,
    'vox': SW.vox,
    'solina': SW.solina,
    'pluck': lambda md, d, **kw: S.pluck(md, d, decay=0.32, bright=0.7, r=0.25, **kw),
}


def play(name, bar, notes, inst, transpose=0, vel=1.0, legato=0.012, **kw):
    for beat, note, dur in notes:
        md = m(note) + transpose
        put(name, T(bar, beat),
            lambda md=md, dur=dur: INST[inst](md, max(0.05, B(dur) - legato), **kw),
            vel, B(dur) + 4.0)


# ---------------------------------------------------------- drum helpers

def kick_f(bar):
    return 55.0 if CHORDS[bar] in ('Bb', 'Csus', 'D') else 51.9   # A1 in D, G#1 elsewhere


def kick_at(bar, beat, vel=1.0):
    t = T(bar, beat)
    f = kick_f(bar)
    put('kick', t, lambda: cached(('k', f), lambda: SW.linn_kick(1.0, f)), vel, 1.0)
    if START <= t < END:
        kicks.append(t - START)


def kicks_pat(bar0, bar1, beats, vel=1.0):
    for b in range(bar0, bar1):
        for bt in beats:
            kick_at(b, bt, vel * (1.0 if bt == int(bt) else 0.85))


def backbeat(bar0, bar1, vel=1.0, clap=0.0, beats=(1, 3)):
    for b in range(bar0, bar1):
        for i, bt in enumerate(beats):
            seed = (b * 2 + i) % 4                       # 4 round-robin variants
            vq = round(min(1.2, vel) * 10) / 10          # velocity shapes the snap and wire
            cl = round(clap * 0.6, 2)                    # the clap is a small layer inside the hit
            put('snare', T(b, bt), lambda seed=seed, vq=vq, cl=cl: cached(
                ('sn2', seed, vq, cl), lambda: SW.snare2(seed, vq, clap=cl)),
                vel * (0.95 + 0.1 * S.rng().random()), 1.0)


def hats(bar0, bar1, level, opn=0.0, sixteenths=True):
    acc16 = {0: .55, .25: .3, .5: .85, .75: .35}
    for b in range(bar0, bar1):
        for beat in range(4):
            for s16, a in acc16.items():
                if not sixteenths and s16 in (.25, .75):
                    continue
                if opn and s16 == .5:
                    v = S.rng().integers(4)
                    put('hats', T(b, beat + s16), lambda v=v: cached(('ho', v), lambda: S.hat(1.0, True, 0.2)),
                        opn, 1.0)
                    continue
                v = S.rng().integers(6)
                put('hats', T(b, beat + s16), lambda v=v: cached(('hc', v), lambda: S.hat(1.0, False, -0.2)),
                    level * a * (0.88 + 0.24 * S.rng().random()), 1.0)


def tom(bar, beat, note, vel=1.0):
    md = m(note)
    put('toms', T(bar, beat), lambda: cached(('tom', md), lambda: SW.linn_tom(md, md % 5)), vel, 1.0)


def march(bar0, pattern, vel=1.0, bars=None):
    for i in range(bars or len(pattern)):
        for beat, v in pattern[i % len(pattern)]:
            seed = int(beat * 3) % 4
            vq = round(min(1.2, vel * v) * 10) / 10
            put('march', T(bar0 + i, beat), lambda seed=seed, vq=vq: cached(
                ('ms2', seed, vq), lambda: SW.snare2(seed + 4, max(0.3, vq), tone=233.1, room=False)),
                vel * v * (0.92 + 0.16 * S.rng().random()), 1.0)


def timp_figure(bar, low='Ab2', vel=1.0):
    """C#3 (3 beats) then the triplet C#3 low C#3 -- the timpani of bars 1-24."""
    tom(bar, 0, 'C#3', vel)
    tom(bar, 3, 'C#3', vel * .7)
    tom(bar, 3 + TRIP, low, vel * .8)
    tom(bar, 3 + 2 * TRIP, 'C#3', vel * .9)


def tom_fill(bar, notes, beat0=2.0, step=.25, vel=1.0):
    for i, nt in enumerate(notes):
        tom(bar, beat0 + i * step, nt, vel * (0.75 + 0.25 * i / max(1, len(notes) - 1)))


def crash(bar, vel=1.0):
    put('fx', T(bar), lambda: S.crash(1.0, 3.0), vel * 0.8, 3.0)


def impact(bar, vel=1.0):
    put('fx', T(bar), lambda: S.impact(), vel * 0.7, 3.0)


# ----------------------------------------------------------- instruments

def pads(bar0, bar1, vel=1.0, cut=2000.0, a=0.5, r=1.0, target='pad'):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        dur = T(e) - T(b)
        for note in PAD[ch]:
            put(target, T(b), lambda note=note, dur=dur: SW.juno_pad(m(note), dur, cut=cut, a=a, r=r),
                vel * 0.5, dur + 6.0)
        b = e


def pedal(bar, bars, note, vel=1.0, a=0.6):
    put('strings', T(bar), lambda: SW.solina(m(note), T(bar + bars) - T(bar) - 0.02, a=a), vel, B(4 * bars) + 6)


def bass(bar0, bar1, style='oct', vel=1.0):
    for b in range(bar0, bar1):
        ch = CHORDS[b]
        r = ROOT[ch]
        if style in ('oct', 'root'):
            for i in range(8):
                md = r + (12 if (style == 'oct' and i % 2) else 0)
                v = (1.0 if i % 2 == 0 else 0.82) * vel
                put('bass', T(b, i * .5), lambda md=md: cached(('b8', md), lambda: SW.bass8(md, B(.42))), v, 1.0)
        elif style == 'fretless':
            rf = ROOT_FRETLESS.get(ch, r)
            for beat, d, acc in MARCH_RHY:
                put('bass', T(b, beat), lambda rf=rf, d=d: cached(('fl', rf, d), lambda: SW.fretless(rf, B(d) * .9)),
                    vel * acc, 1.0)
        elif style == 'hold':
            put('bass', T(b), lambda r=r: SW.fretless(r, B(4) - .03), vel, 4.0)


def ost(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        md = m(OST[CHORDS[b]])
        for beat, d, acc in MARCH_RHY:
            put('ost', T(b, beat), lambda md=md, d=d: cached(('os', md, d), lambda: SW.brass_stab(md, B(d) * .85)),
                vel * acc, 1.0)


def arp8(bar0, bar1, vel=1.0, transpose=0):
    for b in range(bar0, bar1):
        for i, nt in enumerate(ARP[CHORDS[b]]):
            md = m(nt) + transpose
            put('arp', T(b, i * .5), lambda md=md: cached(('a8', md), lambda: SW.arp_note(md, B(.4), decay=0.16)),
                vel * (1.0 if i % 2 == 0 else 0.8), 1.0)


def arp16(bar0, bar1, vel=1.0, transpose=12):
    for b in range(bar0, bar1):
        tones = ARP[CHORDS[b]]
        for i in range(16):
            md = m(tones[i % 8]) + transpose
            put('arp', T(b, i * .25), lambda md=md: cached(('a16', md), lambda: SW.arp_note(
                md, B(.2), decay=0.06, env=5000.0, q=2.6)), vel * (1.0 if i % 4 == 0 else 0.68), 1.0)


LEAD_KW = dict()
HARM_KW = dict(cut_hi=3200.0, cut_sus=1500.0, voices=5, vib=0.08)
LOW_BRASS = dict(cut_hi=3000.0, cut_sus=1300.0, voices=5, detune=0.07, vib=0.06, a=0.04)


def hook(bar, end_note, end_len, transpose=0, pickup=True, vel=1.0, harm=0.55, octave=0.0,
         harm_end='Ab4'):
    notes = (HOOK_PICK if pickup else []) + HOOK_BODY + [(8, end_note, end_len)]
    play('lead', bar, notes, 'brass', transpose, vel)
    if harm:
        hn = (HARM_PICK if pickup else []) + HARM_BODY + [(8, harm_end, end_len)]
        play('lead2', bar, hn, 'brass', transpose, vel * harm, **HARM_KW)
    if octave:
        play('lead2', bar, notes, 'brass', transpose + 12, vel * octave, cut_hi=6000.0, cut_sus=3000.0,
             voices=5, detune=0.11)


# ---------------------------------------------------------- arrangement

# INTRO 0-7 (orig 1-8): timpani figure on toms, pad and arp opening up, flute + oboe
pads(0, 8, vel=0.9, cut=1100, a=2.0)
pedal(0, 8, 'C#6', vel=0.5, a=3.0)
ost(2, 8, vel=0.7)
arp8(4, 8, vel=0.55)
for b in range(0, 8, 2):
    timp_figure(b, 'A2' if b == 6 else 'Ab2', vel=0.55 + 0.05 * b)
for b in range(1, 8, 2):
    tom(b, 0, 'B2' if b == 7 else 'C#3', 0.5 + 0.05 * b)
play('solo', 1, INTRO_FLUTE, 'solo', vel=0.8)
play('solo', 4, OBOE_1, 'solo', vel=0.75)
bass(4, 8, 'hold', vel=0.55)
march(4, MARCH_A, vel=0.35)
hats(6, 8, 0.25, sixteenths=False)
put('fx', T(6), lambda: S.noise_riser(T(8) - T(6), 400, 8000), 0.45, 9)
put('fx', T(7), lambda: S.reverse_cymbal(B(4)), 0.6, 3)

# VERSE 8-23 (orig 9-24): the groove arrives; flute lines, trombone call, harp run
crash(8)
kicks_pat(8, 16, (0, 2, 2.5))
kicks_pat(16, 23, (0, 1, 2, 2.5, 3))
kicks_pat(23, 24, (0, 1, 2))
backbeat(8, 16, vel=0.9)
backbeat(16, 24, vel=0.95, clap=0.45)
hats(8, 16, 0.32)
hats(16, 24, 0.36, opn=0.22)
bass(8, 16, 'root', vel=0.95)
bass(16, 24, 'oct')
pads(8, 24, vel=0.75, cut=1700, a=0.4)
pedal(8, 8, 'C#6', vel=0.45)
pedal(16, 8, 'C#6', vel=0.5)
ost(8, 24, vel=0.75)
arp8(12, 24, vel=0.6)
play('solo', 10, VERSE_FLUTE, 'solo', vel=0.95)
play('lead', 13, TROMBONE, 'brass', vel=0.75, **LOW_BRASS)
play('lead2', 13, TROMBONE, 'brass', transpose=12, vel=0.3, **HARM_KW)
play('solo', 15, FLUTE_2, 'solo', vel=0.9)
play('bell', 17, HARP_RUN, 'pluck', transpose=12, vel=0.8)
play('solo', 18, VERSE_FLUTE, 'solo', vel=0.95)
play('solo', 18, VERSE_FLUTE, 'solo', transpose=12, vel=0.4)
play('vox', 20, [(2, 'Ab3', 2), (4, 'Ab4', 11.5)], 'vox', vel=1.0)
play('solo', 21, OBOE_2, 'solo', vel=0.85)
for b in (10, 14, 18):
    timp_figure(b, 'A2' if CHORDS[b] == 'A' else 'Ab2', vel=0.45)
march(23, [MARCH_A[3]], vel=0.8)
tom_fill(23, ['Ab3', 'F3', 'C#3', 'Ab2'], beat0=3.0, step=.25, vel=0.7)
put('fx', T(21), lambda: S.noise_riser(T(24) - T(21), 300, 9000, 2.2), 0.55, 9)
put('fx', T(23), lambda: S.reverse_cymbal(B(4)), 0.7, 3)

# CHORUS 1 24-31: the hook on the big brass, A Bsus C# C#
crash(24, 1.0)
crash(28, 0.85)
impact(24, 0.7)
kicks_pat(24, 32, (0, 1, 2, 3))
backbeat(24, 32, vel=1.0, clap=0.55)
hats(24, 32, 0.4, opn=0.32)
bass(24, 32, 'oct')
pads(24, 32, vel=0.85, cut=2500, a=0.15)
pedal(24, 8, 'C#6', vel=0.55)
ost(24, 32, vel=0.7)
arp16(24, 32, vel=0.5)
hook(24, 'F5', 6.4)
hook(28, 'C#5', 7.6, harm_end='F4')
play('bell', 27, HARP_RUN, 'pluck', transpose=12, vel=0.75)
play('bell', 31, HARP_RUN, 'pluck', transpose=12, vel=0.75)
play('vox', 26, [(0, 'F4', 7.5)], 'vox', vel=0.8)
play('vox', 30, [(0, 'Ab4', 7.5)], 'vox', vel=0.8)
tom_fill(31, ['C#4', 'Ab3', 'F3', 'C#3', 'Ab2', 'F2'], beat0=2.5, step=.25, vel=0.75)

# BREAKDOWN 32-39 (orig 25-32): Ab minor, no drums; fretless bass plays the march rhythm
crash(32, 0.7)
impact(32, 0.6)
put('fx', T(32), lambda: S.downlifter(B(8)), 0.45, 8)
pads(32, 40, vel=0.8, cut=1300, a=0.8, r=1.5)
for b in range(32, 40, 2):
    pedal(b, 2, 'C#6' if CHORDS[b] == 'F#m' else 'Eb6', vel=0.5, a=0.8)
bass(32, 40, 'fretless', vel=0.7)
ost(36, 40, vel=0.45)
play('solo', 32, P1, 'solo', vel=1.0)
play('solo', 36, P2, 'solo', vel=1.0)
play('bell', 35, [(0, 'Ab5', .5), (.5, 'Eb5', .5), (1, 'B4', .5), (1.5, 'Ab4', .5), (2, 'Eb4', 1)], 'pluck',
     vel=0.5)
hats(36, 40, 0.18, sixteenths=False)
put('fx', T(39), lambda: S.reverse_cymbal(B(4)), 0.5, 3)

# BUILD 40-47 (orig 33-40): the melody on brass, the march snare returns, roll into bar 48
pads(40, 48, vel=0.8, cut=1700, a=0.3)
for b in range(40, 48, 2):
    pedal(b, 2, 'C#6' if CHORDS[b] == 'F#m' else 'Eb6', vel=0.5, a=0.5)
kicks_pat(40, 44, (0, 2))
kicks_pat(44, 47, (0, 1, 2, 3))
kicks_pat(47, 48, (0, 1))
backbeat(40, 44, vel=0.85, beats=(2,))
backbeat(44, 47, vel=0.95, clap=0.4)
march(40, MARCH_B, vel=0.45)
march(44, MARCH_B[:3], vel=0.7)
hats(40, 44, 0.25, sixteenths=False)
hats(44, 47, 0.33)
bass(40, 44, 'fretless', vel=0.8)
bass(44, 48, 'oct', vel=0.95)
ost(40, 48, vel=0.6)
arp8(40, 48, vel=0.55, transpose=12)
play('lead', 40, P3, 'brass', vel=0.9)
play('lead', 44, P4, 'brass', vel=1.0)
play('lead2', 44, P4, 'brass', transpose=-12, vel=0.4, **HARM_KW)
# bar 40's roll: timpani (toms) and snare 16ths from beat 2, cymbal swell
for i in range(8):
    tom(47, 2 + i * .25, 'C#3', 0.4 + 0.08 * i)
for i in range(8):
    vq = round(0.4 + 0.08 * i, 1)
    put('march', T(47, 2 + i * .25), lambda vq=vq, i=i: cached(('ms2', i % 4 + 4, vq), lambda: SW.snare2(
        i % 4 + 4, vq, tone=233.1, room=False)), 0.35 + 0.08 * i, 1)
put('fx', T(44), lambda: S.noise_riser(T(48) - T(44), 300, 11000, 2.5), 0.7, 12)
put('fx', T(46), lambda: S.pitch_riser(T(47, 3.5) - T(46), 49, 73), 0.45, 6)
put('fx', T(47), lambda: S.reverse_cymbal(B(4)), 0.8, 3)

# ANTHEM 48-63 (orig 41-56): C# / F#m, harp arp, clarinet line, the rising choir line
for b in range(48, 64, 4):
    crash(b, 1.0 if b == 48 else 0.8)
impact(48, 0.9)
kicks_pat(48, 64, (0, 1, 2, 3))
backbeat(48, 64, vel=1.0, clap=0.55)
hats(48, 64, 0.4, opn=0.3)
bass(48, 64, 'oct')
pads(48, 64, vel=0.85, cut=2600, a=0.2)
pedal(52, 12, 'C#6', vel=0.55)
ost(48, 64, vel=0.55)
arp8(48, 64, vel=0.75)
arp16(56, 64, vel=0.42)
for b in range(48, 64, 2):
    lo = 'A2' if CHORDS[b] == 'F#m' else 'Ab2'
    tom(b, 3.5, lo, 0.6)
    tom(b, 3.75, lo, 0.7)
play('lead', 48, [(0, 'C#6', 15.8)], 'brass', vel=0.85, a=0.25, vib=0.16)
play('lead2', 48, CLAR_TOP, 'brass', vel=0.6, **LOW_BRASS)
play('lead2', 48, CLAR_LOW, 'brass', vel=0.5, **LOW_BRASS)
play('lead2', 52, CLAR_TOP, 'brass', vel=0.6, **LOW_BRASS)
play('lead2', 52, CLAR_LOW, 'brass', vel=0.5, **LOW_BRASS)
play('lead', 52, [(b, n, l) for b, n, l in CLAR_TOP], 'brass', transpose=12, vel=0.9)
play('vox', 48, CHOIR_LINE, 'vox', vel=0.9)
play('lead2', 48, [x for x in CHOIR_LINE if x[0] < 48], 'brass', vel=0.55, **LOW_BRASS)
choir_hi = [x for x in CHOIR_LINE if x[0] >= 48]
play('lead', 48, choir_hi[:-1] + [(56, 'A5', 6.4)], 'brass', vel=1.0)
play('lead2', 48, choir_hi[:-1] + [(56, 'A5', 6.4)], 'brass', transpose=12, vel=0.32, cut_hi=6000.0,
     cut_sus=3000.0, voices=5)
tom_fill(63, ['A3', 'E3', 'C#3', 'A2'], beat0=3.0, step=.25, vel=0.8)
put('fx', T(61), lambda: S.noise_riser(T(64) - T(61), 400, 11000, 2.5), 0.6, 9)
put('fx', T(62), lambda: S.pitch_riser(T(63, 3.5) - T(62), 50, 74), 0.4, 6)

# FINAL CHORUS 64-79: the hook lifted to D (Bb C D D), octave-doubled in the second half
for b in range(64, 80, 4):
    crash(b, 1.0)
impact(64, 1.0)
kicks_pat(64, 80, (0, 1, 2, 3))
backbeat(64, 80, vel=1.0, clap=0.6)
hats(64, 80, 0.42, opn=0.34)
bass(64, 80, 'oct')
pads(64, 80, vel=0.9, cut=2900, a=0.1)
pedal(64, 16, 'D6', vel=0.55)
ost(64, 80, vel=0.6)
arp16(64, 80, vel=0.5)
arp8(72, 80, vel=0.5)
hook(64, 'F5', 6.4, transpose=1)
hook(68, 'C#5', 6.4, transpose=1, harm_end='F4')
hook(72, 'F5', 6.4, transpose=1, octave=0.42)
hook(76, 'F5', 10.9, transpose=1, octave=0.42)        # F#5 held into the outro's A bar
for b in (67, 71, 75):
    play('bell', b, HARP_RUN, 'pluck', transpose=13, vel=0.7)
for b, ch in ((64, 'Bb'), (65, 'Csus'), (66, 'D'), (68, 'Bb'), (69, 'Csus'), (70, 'D'),
              (72, 'Bb'), (73, 'Csus'), (74, 'D'), (76, 'Bb'), (77, 'Csus'), (78, 'D')):
    d = 8 if ch == 'D' else 4
    for nt in PAD[ch][1:]:
        play('vox', b, [(0, nt, d - .1)], 'vox', vel=0.45)
march(71, [MARCH_A[3]], vel=0.7)
march(79, [MARCH_A[3]], vel=0.85)
tom_fill(79, ['D4', 'A3', 'F#3', 'D3', 'A2', 'F#2'], beat0=2.5, step=.25, vel=0.8)

# OUTRO 80-87 (orig 57-60): back home to C#, last hook tail, the coda flute
crash(80, 0.9)
kicks_pat(80, 84, (0, 2, 2.5))
backbeat(80, 84, vel=0.9, clap=0.4)
hats(80, 84, 0.3, sixteenths=False)
bass(80, 84, 'oct', vel=0.9)
bass(84, 88, 'fretless', vel=0.75)
pads(80, 88, vel=0.8, cut=2000, a=0.3, r=1.5)
pedal(80, 8, 'C#6', vel=0.5)
ost(80, 88, vel=0.6)
arp8(80, 86, vel=0.5)
play('lead', 80, HOOK_BODY[1:] + [(8, 'F5', 7.8)], 'brass', vel=0.95)
play('lead2', 80, HARM_BODY[1:] + [(8, 'Ab4', 7.8)], 'brass', vel=0.5, **HARM_KW)
play('solo', 84, FLUTE_CODA, 'solo', vel=0.85)
for b in (84, 86):
    timp_figure(b, 'Ab2', vel=0.6)
    tom(b + 1, 0, 'C#3', 0.55)
put('fx', T(87), lambda: S.reverse_cymbal(B(4)), 0.5, 3)

# FINAL HIT 88
crash(88, 1.0)
impact(88, 0.8)
kick_at(88, 0, 1.0)
put('snare', T(88), lambda: SW.snare2(2, 1.0, clap=0.3), 0.9, 1)
pads(88, 89, vel=0.8, cut=2200, a=0.01, r=2.0)
for nt in ('C#3', 'Ab3', 'C#4', 'F4', 'Ab4'):
    play('lead2', 88, [(0, nt, 6)], 'brass', vel=0.45, **LOW_BRASS)
play('lead', 88, [(0, 'C#5', 6)], 'brass', vel=0.7)
put('bass', T(88), lambda: SW.fretless(37, B(6)), 0.8, 5)
pedal(88, 1, 'C#6', vel=0.5, a=0.05)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


abs_bars = (START + np.arange(N) / SR) / (4 * SPB)


def bar_curve(points):
    """Piecewise-linear automation over absolute bars."""
    return np.interp(abs_bars, [p[0] for p in points], [p[1] for p in points]).astype(np.float32)


duck_bass = S.sidechain(N, kicks, depth=0.5, release=0.22)
duck_pad = S.sidechain(N, kicks, depth=0.3, release=0.28)
duck_soft = S.sidechain(N, kicks, depth=0.15, release=0.25)

GAIN = {  # dB
    'kick': -4.0, 'snare': -6.5, 'march': -8.0, 'hats': 5.0, 'toms': -9.0, 'fx': -14.0,
    'bass': -2.5, 'pad': 0.0, 'strings': -6.0, 'vox': -18.0, 'arp': -6.0, 'ost': -2.0,
    'lead': -8.0, 'lead2': -8.5, 'solo': -0.5, 'bell': -10.0,
}
DUCK = {'bass': duck_bass, 'pad': duck_pad, 'strings': duck_soft, 'arp': duck_pad, 'ost': duck_soft,
        'vox': duck_soft}
VERB = {'snare': .08, 'march': .15, 'toms': .1, 'pad': .25, 'strings': .4, 'vox': .4, 'arp': .22,
        'ost': .12, 'lead': .22, 'lead2': .22, 'solo': .4, 'bell': .4, 'fx': .25, 'hats': .03}
DELAY = {'lead': .13, 'solo': .3, 'arp': .22, 'bell': .3, 'lead2': .06}
PROC = {
    'snare': (HighpassFilter(110), LowpassFilter(8500)),
    'march': (HighpassFilter(150), LowpassFilter(8000)),
    'hats': (HighpassFilter(5500), LowpassFilter(13000)),
    'fx': (HighpassFilter(200), LowpassFilter(10000)),
    'toms': (HighpassFilter(55),),
    'bass': (HighpassFilter(30), LowpassFilter(2600)),
    'pad': (HighpassFilter(160),),
    'strings': (HighpassFilter(320), LowpassFilter(7000)),
    'vox': (HighpassFilter(200),),
    'arp': (HighpassFilter(220),),
    'ost': (HighpassFilter(160),),
    'lead': (HighpassFilter(180),),
    'lead2': (HighpassFilter(180),),
    'solo': (HighpassFilter(200),),
    'bell': (HighpassFilter(250),),
}
CHORUS = {'pad': dict(rate=0.5, depth_ms=2.2, center_ms=7.5, mix=0.5),
          'strings': dict(rate=0.75, depth_ms=3.0, center_ms=9.0, mix=0.55),
          'vox': dict(rate=0.4, depth_ms=2.5, center_ms=10.0, mix=0.45),
          'lead': dict(rate=0.35, depth_ms=1.2, center_ms=6.0, mix=0.22),
          'lead2': dict(rate=0.45, depth_ms=1.6, center_ms=8.0, mix=0.35),
          'solo': dict(rate=0.6, depth_ms=1.5, center_ms=7.0, mix=0.3),
          'arp': dict(rate=0.55, depth_ms=1.8, center_ms=7.0, mix=0.4),
          'ost': dict(rate=0.5, depth_ms=1.5, center_ms=7.0, mix=0.35)}
# filter sweeps: the intro opens, the build opens, bars as (bar, Hz)
SWEEP = {
    'pad': [(0, 900), (8, 9000), (8.01, 20000), (200, 20000)],
    'arp': [(0, 600), (8, 6000), (24, 9000), (32, 9000), (40, 900), (48, 12000), (48.01, 20000), (200, 20000)],
    'ost': [(0, 500), (8, 5000), (8.01, 20000), (200, 20000)],
    'lead': [(0, 20000), (40, 20000), (40.01, 1100), (47.9, 14000), (48, 20000), (200, 20000)],
}

DIAG = os.environ.get('SONG2_DIAG')
DIAG_SECS = [('intro', 0, 8), ('verse', 8, 24), ('chor1', 24, 32), ('break', 32, 40), ('build', 40, 48),
             ('anthem', 48, 64), ('final', 64, 80), ('outro', 80, 88)]
DIAG_BANDS = [(40, 160), (160, 630), (630, 2500), (2500, 20000)]
diag = {}


def diag_bus(k, x):
    """RMS (dB) of the bus per section, and its share per band in the loudest section."""
    import scipy.signal as ss_
    row = []
    for _, b0, b1 in DIAG_SECS:
        i0, i1 = int((T(b0) - START) * SR), int((T(b1) - START) * SR)
        if i1 <= 0 or i0 >= N:
            row.append(None)
            continue
        y = x[:, max(0, i0):i1].mean(axis=0)
        f, P = ss_.welch(y, SR, nperseg=4096)
        tot = 10 * np.log10(np.mean(y.astype(np.float64) ** 2) + 1e-20)
        bands = [10 * np.log10(P[(f >= a) & (f < b)].sum() / (P.sum() + 1e-30) + 1e-20) for a, b in DIAG_BANDS]
        hi = 10 * np.log10(P[(f >= 5000) & (f < 10000)].sum() * (f[1] - f[0]) + 1e-20)
        row.append((tot, bands, hi))
    diag[k] = row


mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('SONG2_MUTE', '').split(',')))
for k in names:
    x = bus.pop(k).x
    if k in MUTE or not np.any(x):
        del x
        continue
    if k in PROC:
        x = stage(x, *PROC[k])
    if k in SWEEP:
        x = S.svf(x, bar_curve(SWEEP[k]), 0.8).astype(np.float32)
    if k in CHORUS:
        x = SW.chorus(x, **CHORUS[k])
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= DUCK[k][None, :]
    levels[k] = pyln.Meter(SR).integrated_loudness(x.T.astype(np.float64))
    if DIAG:
        diag_bus(k, x)
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    if k in DELAY:
        delay_send += x * np.float32(DELAY[k])
    del x

pre = int(0.025 * SR)
verb_send[:, pre:] = verb_send[:, :-pre].copy()
verb_send[:, :pre] = 0
verb = stage(verb_send, HighpassFilter(300), LowpassFilter(7500),
             Reverb(room_size=0.9, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del verb_send
verb *= duck_soft[None, :]
mix += verb * np.float32(0.85)
del verb
dly = S.pingpong(delay_send, B(0.75), feedback=0.4, repeats=7, lp=4200.0, hp=400.0)
del delay_send
dly = stage(dly, Reverb(room_size=0.5, damping=0.6, wet_level=0.25, dry_level=1.0, width=1.0))
mix += dly * np.float32(0.55)
del dly

# section dynamics (dB at bar lines): quiet intro and breakdown, the build
# climbing, the anthem and the final chorus loudest
SECTION_DB = [(0, -3), (7.9, -2.5), (8, -4.5), (23.9, -4), (24, 0), (31.9, 0), (32, -3.5), (39.9, -3.5),
              (40, -5), (47.9, -1.5), (48, -1.5), (55.9, -1.2), (56, -0.5), (63.9, 0), (64, 1.8), (79.9, 1.8),
              (80, -1.5), (84, -2.5), (88, -3.5), (88.01, -1.5), (95, -1.5)]
mix *= (10 ** (bar_curve(SECTION_DB) / 20))[None, :]
del abs_bars

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-15.0 - loud) / 20))
mix = SW.tape(mix)
mix = stage(mix, HighpassFilter(25),
            LowShelfFilter(75, 1.0, 0.7), PeakFilter(1000, -2.0, 0.7), HighShelfFilter(6500, -2.5, 0.7),
            LowpassFilter(15500),
            Compressor(threshold_db=-12, ratio=2.0, attack_ms=25, release_ms=180),
            Limiter(threshold_db=-1.8, release_ms=90))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song2_synthwave.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s', 'peak', float(np.max(np.abs(mix))))
if DIAG:
    print('bus RMS dB per section (band shares 40-160/160-630/630-2.5k/2.5k+ in brackets)')
    print('         ' + ''.join(f'{nm:>22s}' for nm, _, _ in DIAG_SECS))
    for k, row in diag.items():
        cells = []
        for c in row:
            if c is None or c[0] < -90:
                cells.append(' ' * 22)
            else:
                cells.append(f'{c[0]:6.1f} [' + ' '.join(f'{int(round(v)):3d}' for v in c[1]) + ']')
        print(f'{k:8s} ' + ''.join(f'{c:>22s}' for c in cells))
if DIAG:
    print('5-10 kHz level per bus (dB) in chorus 1 / final:')
    for k, row in diag.items():
        print(f'  {k:8s} ' + '  '.join(f'{row[j][2]:6.1f}' if row[j] else '   -  ' for j in (2, 6)))
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
