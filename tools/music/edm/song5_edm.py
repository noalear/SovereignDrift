#!/usr/bin/env python3
# song5_edm.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 5, Electronic": an EDM / trance remix of sf-music5 (the original
# game's music by Stardock, from mid/sf-music5.mid), produced entirely in
# code with tools/music/edm/synth.py: synthesized supersaws, plucks, piano,
# sub bass, drums and effects, mixed and mastered here.
#
# Taken from the original song (C# minor):
#   - the arpeggio ostinato  . C# G# B  (Fantasia pad, all through the song)
#   - the hook               C#6 A5 B5 G#5 | A5 F#5 G#5 E5 | F#5 D#5 D#5 F#5 | G#5 D#5
#                            (electric piano, bars 57-72) and its harmony line
#   - the brass line         G#4 ... G#F#G#F# G#F#E F# G# C# | C# B C# E (bars 9-16)
#   - the string stabs       C#6/G#5 (bars 1-16) and the motif G#5 F#5 E5 F#5 G#5
#   - the E / F# section with its choir line (bars 17-32)
#   - the C# / D brass section (bars 41-48) and the D section of the ending
# New: 138 BPM (from 180), a i-VI-III-IV progression under the hook
# (C#m7 Amaj7 Emaj7 F#6/9), an EDM form (intro, groove, breakdown, build,
# drop, bridge, drop, outro), drums, bass, risers and impacts.
#
#   python3 tools/music/edm/song5_edm.py OUT.wav
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
from synth import m, SR  # noqa: E402

BPM = 138.0
SPB = 60.0 / BPM
NBARS = 88
TAIL = 7.0
N = int((NBARS * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


# ------------------------------------------------------------- harmony

PAD = {
    'C#m':  ['C#4', 'E4', 'G#4', 'B4'],
    'A':    ['A3', 'C#4', 'E4', 'G#4'],
    'E':    ['E4', 'G#4', 'B4', 'D#5'],
    'F#':   ['F#3', 'C#4', 'D#4', 'G#4'],
    'E6':   ['E4', 'G#4', 'B4', 'C#5'],
    'F#m':  ['F#4', 'A4', 'C#5', 'E5'],
    'C#5':  ['C#4', 'G#4', 'C#5'],
    'D':    ['D4', 'F#4', 'A4', 'C#5'],
}
ROOT = {'C#m': 37, 'A': 33, 'E': 28, 'F#': 30, 'E6': 28, 'F#m': 30, 'C#5': 37, 'D': 38}

HOOK_PROG = ['C#m', 'A', 'E', 'F#']
CHORDS = (['C#m'] * 16 +                       # intro, groove A
          ['E6', 'E6', 'F#m', 'F#m'] * 2 +       # groove B
          HOOK_PROG * 4 +                        # breakdown + build
          HOOK_PROG * 4 +                        # drop 1
          ['C#5', 'C#5', 'D', 'D'] * 2 +         # bridge
          HOOK_PROG * 4 +                        # drop 2
          ['D'] * 4 + ['C#m'] * 4)               # outro
assert len(CHORDS) == NBARS

# The hook, 8 bars (beats; note; length)
HOOK_A = [(0, 'C#6', 1.5), (1.5, 'A5', .5), (2, 'B5', 1.5), (3.5, 'G#5', .5),
          (4, 'A5', 1.5), (5.5, 'F#5', .5), (6, 'G#5', 1.5), (7.5, 'E5', .5),
          (8, 'F#5', 1), (9, 'D#5', 1), (10, 'D#5', 1), (11, 'F#5', 1),
          (12, 'G#5', 2)]
HOOK = HOOK_A + [(14, 'D#5', 2)] + [(b + 16, n, l) for b, n, l in HOOK_A] + [(30, 'C#6', 2)]
HOOK_HARM = [(8, 'G#4', 1), (9, 'B4', 1), (10, 'B4', 1), (11, 'D#5', 1), (12, 'D#5', 2), (14, 'G#4', 2)]
HOOK_HARM = HOOK_HARM + [(b + 16, n, l) for b, n, l in HOOK_HARM[:-1]] + [(30, 'E5', 2)]

# The brass line, 4 bars
BRASS = [(0, 'G#4', 4.9), (5, 'G#4', .25), (5.25, 'F#4', .25), (5.5, 'G#4', .25), (5.75, 'F#4', .25),
         (6, 'G#4', .5), (6.5, 'F#4', .5), (7, 'E4', .5), (7.5, 'F#4', .5),
         (8, 'G#4', 1), (9, 'C#4', 1), (10, 'C#4', 1), (11, 'B3', .5), (11.5, 'C#4', 1), (12.5, 'E4', .5)]
MOTIF = [(0, 'G#5', .5), (.5, 'F#5', .5), (1, 'E5', .5), (1.5, 'F#5', .5), (2, 'G#5', 3.5)]
CHOIR = [(0, 'C#5', 4), (4, 'D5', 2), (6, 'F#5', 2), (8, 'G#5', 4), (12, 'F#5', 4)]


def arp_notes(chord):
    return ['C#5', 'F#5', 'A5'] if chord == 'F#m' else ['C#5', 'G#5', 'B5']


# --------------------------------------------------------------- buses

names = ['kick', 'clap', 'snare', 'hats', 'cym', 'toms', 'sub', 'bass', 'pad', 'arp', 'arp16',
         'lead', 'lead_lo', 'lead_hi', 'harm', 'brass', 'choir', 'stab', 'bell', 'piano', 'fx', 'impact']
bus = {k: S.Bus(N) for k in names}
kicks = []

_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def place_kick(t, vel=1.0):
    bus['kick'].add(cached(('kick',), S.kick), t, vel)
    kicks.append(t)


def four_floor(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            place_kick(T(b, beat), vel)


def claps(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        for beat in (1, 3):
            bus['clap'].add(S.clap(), T(b, beat), vel)


def hats(bar0, bar1, closed=0.35, opn=0.6, ride=0.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            if opn:
                bus['hats'].add(S.hat(1.0, True, 0.15), T(b, beat + .5), opn)
            if closed:
                for s16, acc in ((0, .7), (.25, .45), (.5, .0), (.75, .55)):
                    if acc:
                        bus['hats'].add(S.hat(1.0, False, -0.25), T(b, beat + s16),
                                        closed * acc * (0.9 + 0.2 * S.rng().random()))
            if ride:
                bus['hats'].add(S.ride(), T(b, beat), ride * (1.0 if beat % 2 == 0 else 0.8))


def crash_at(t, vel=1.0):
    bus['cym'].add(S.crash(), t, vel)


def snare_roll(bar0, bars_by_div, vel0=0.25, vel1=1.0, stop_beat=None):
    """bars_by_div: list of (bars, notes per beat)."""
    t0 = T(bar0)
    total = sum(b for b, _ in bars_by_div) * 4
    beat = 0.0
    for bars, div in bars_by_div:
        for i in range(int(bars * 4 * div)):
            bb = beat + i / div
            if stop_beat is not None and bb >= stop_beat:
                break
            u = bb / total
            bus['snare'].add(S.snare(1.0, 190 + 120 * u), t0 + B(bb), vel0 + (vel1 - vel0) * u ** 1.3)
        beat += bars * 4


def bass_line(bar0, bar1, style):
    for b in range(bar0, bar1):
        r = ROOT[CHORDS[b]]
        if style == 'sustain':
            bus['sub'].add(S.sub_bass(r, B(4) - .02), T(b), 1.0)
        elif style == 'offbeat':
            bus['sub'].add(S.sub_bass(r, B(4) - .02), T(b), 0.9)
            for beat in range(4):
                bus['bass'].add(cached(('mb', r + 12, .45), lambda: S.mid_bass(r + 12, B(.45))),
                                T(b, beat + .5), 0.9)
        elif style == 'rolling':
            bus['sub'].add(S.sub_bass(r, B(4) - .02), T(b), 1.0)
            for beat in range(4):
                for s16 in (.25, .5, .75):
                    v = 0.95 if s16 == .5 else 0.75
                    bus['bass'].add(cached(('mb', r + 12, .22), lambda: S.mid_bass(r + 12, B(.22))),
                                    T(b, beat + s16), v)


def pads(bar0, bar1, vel=1.0, attack=0.25, cut=3200.0, target='pad'):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        dur = T(e) - T(b)
        for note in PAD[ch]:
            bus[target].add(S.supersaw(m(note), dur, 1.0, a=attack, d=0.8, s=0.85, r=0.5,
                                       cut=cut, detune=0.25), T(b), vel * 0.5)
        b = e


def arp(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        notes = arp_notes(CHORDS[b])
        for half in (0, 2):
            for off, note, dur in ((.5, notes[0], .5), (1.0, notes[1], .5), (1.5, notes[2], 1.0)):
                bus['arp'].add(cached(('pl', note, dur), lambda: S.pluck(m(note), B(dur), decay=0.18)),
                               T(b, half + off), vel)


ARP16 = {'C#m': ['C#5', 'E5', 'G#5', 'C#6'], 'A': ['C#5', 'E5', 'A5', 'C#6'],
         'E': ['B4', 'E5', 'G#5', 'B5'], 'F#': ['C#5', 'F#5', 'G#5', 'C#6']}


def arp16(bar0, bar1, vel=1.0):
    pat = [0, 1, 2, 3, 2, 1, 2, 3]
    for b in range(bar0, bar1):
        tones = ARP16[CHORDS[b]]
        for i in range(16):
            note = tones[pat[i % 8]]
            acc = 1.0 if i % 4 == 0 else 0.7
            bus['arp16'].add(cached(('p16', note), lambda: S.pluck(m(note), B(.2), decay=0.07, bright=1.6)),
                             T(b, i * .25), vel * acc)


def melody(target, bar0, notes, transpose=0, inst='lead', vel=1.0, **kw):
    for beat, note, dur in notes:
        md = m(note) + transpose
        t = T(bar0, beat)
        if inst == 'lead':
            sig = S.supersaw(md, B(dur) - 0.01, 1.0, voices=9, detune=0.2, a=0.004, d=0.6, s=0.8, r=0.12,
                             cut=kw.get('cut', 7500), cut_env=kw.get('cut_env', 4000), cut_decay=0.15,
                             vib=0.10 if dur >= 1.5 else 0.0)
        elif inst == 'brass':
            sig = S.supersaw(md, B(dur) - 0.02, 1.0, voices=5, detune=0.12, width=0.5, a=0.02, d=0.3, s=0.8,
                             r=0.08, cut=2200, cut_env=2500, cut_decay=0.12, q=1.0,
                             vib=0.08 if dur >= 1.5 else 0.0)
        elif inst == 'choir':
            sig = S.supersaw(md, B(dur), 1.0, voices=7, detune=0.3, a=0.35, d=1.0, s=0.9, r=0.6,
                             cut=1800, vib=0.06)
        elif inst == 'bell':
            sig = S.bell(md, B(dur))
        elif inst == 'piano':
            sig = S.piano(md, B(dur), vel=0.85)
        else:
            raise ValueError(inst)
        bus[target].add(sig, t, vel)


def stabs(bar0, bar1, vel=1.0):
    hits = [(0, ['C#5', 'G#5', 'C#6'], .25), (.5, ['C#5', 'G#5', 'C#6'], .25),
            (7.5, ['B4', 'F#5', 'B5'], .4), (8, ['C#5', 'G#5', 'C#6'], .25), (8.5, ['C#5', 'G#5', 'C#6'], .25)]
    for b in range(bar0, bar1, 4):
        for beat, chord, dur in hits:
            for note in chord:
                bus['stab'].add(cached(('st', note, dur), lambda: S.supersaw(
                    m(note), B(dur), 1.0, a=0.002, d=0.15, s=0.3, r=0.12, cut=4000, cut_env=6000,
                    cut_decay=0.08)), T(b, beat), vel * 0.6)


def bridge_stabs(bar0, vel=1.0):
    seq = [(2, ['C#4', 'G#4'], .25), (3, ['C#4', 'G#4'], .4), (3.5, ['C#4', 'G#4'], .4),
           (4, ['C#4', 'G#4', 'C#5'], 1.9),
           (6, ['G#4', 'C#5'], .25), (7, ['G#4', 'C#5'], .4), (7.5, ['G#4', 'C#5'], .4),
           (8, ['G#4', 'D5', 'D4'], 6.0)]
    for cycle in range(2):
        b = bar0 + 4 * cycle
        for beat, chord, dur in seq:
            for note in chord:
                bus['stab'].add(S.supersaw(m(note), B(dur), 1.0, voices=7, detune=0.18, a=0.003, d=0.5,
                                           s=0.55, r=0.25, cut=2600, cut_env=5000, cut_decay=0.1),
                                T(b, beat), vel * 0.55)
        for beat, md in ((13.5, 55), (14, 52), (14.5, 50), (15, 47), (15.5, 45)):
            bus['toms'].add(S.tom(md), T(b, beat), 0.9)


# ---------------------------------------------------------- arrangement

# INTRO 0-7
pads(0, 8, vel=0.55, attack=1.2, cut=1600)
arp(0, 8, vel=0.8)
melody('bell', 2, MOTIF, vel=0.55, inst='bell')
melody('bell', 6, MOTIF, vel=0.65, inst='bell')
hats(4, 8, closed=0.22, opn=0.0)
bus['fx'].add(S.noise_riser(T(8) - T(6), 400, 7000), T(6), 0.5)
bus['cym'].add(S.reverse_cymbal(B(4)), T(7), 0.7)
bus['sub'].add(S.sub_bass(37, T(8) - T(4)), T(4), 0.45)

# GROOVE A 8-15 (brass line, stabs)
crash_at(T(8))
four_floor(8, 24)
claps(12, 24, 0.85)
hats(8, 24, closed=0.33, opn=0.5)
bass_line(8, 24, 'offbeat')
arp(8, 24, vel=0.85)
pads(8, 16, vel=0.45, attack=0.4, cut=2200)
melody('brass', 8, BRASS, vel=0.8, inst='brass')
melody('brass', 12, BRASS, vel=0.85, inst='brass')
melody('brass', 8, BRASS, transpose=12, vel=0.35, inst='brass')
melody('brass', 12, BRASS, transpose=12, vel=0.4, inst='brass')
stabs(8, 16, vel=0.8)

# GROOVE B 16-23 (E / F# section, choir line, motif)
crash_at(T(16), 0.8)
pads(16, 24, vel=0.6, attack=0.3, cut=2600)
melody('choir', 16, CHOIR, vel=0.8, inst='choir')
melody('choir', 20, CHOIR, vel=0.85, inst='choir')
for bb in (17, 19, 21):
    melody('bell', bb, MOTIF, vel=0.45, inst='bell', transpose=0)
hats(20, 24, closed=0.0, opn=0.0, ride=0.35)
for i in range(8):
    bus['snare'].add(S.snare(1.0, 200 + 10 * i), T(23, 2 + i * .25), 0.3 + 0.07 * i)

# BREAKDOWN 24-31 (piano hook, no drums)
crash_at(T(24), 0.7)
bus['impact'].add(S.impact(), T(24), 0.7)
bus['fx'].add(S.downlifter(B(8)), T(24), 0.5)
pads(24, 32, vel=0.6, attack=0.8, cut=1800)
bass_line(24, 32, 'sustain')
melody('piano', 24, HOOK, vel=1.0, inst='piano')
melody('piano', 24, [(b, n, l) for b, n, l in HOOK_HARM], vel=0.55, inst='piano')
arp(24, 32, vel=0.45)

# BUILD 32-39
pads(32, 40, vel=0.55, attack=0.3, cut=2400)
melody('lead', 32, HOOK, vel=0.9, inst='lead')
four_floor(32, 36, 0.9)
bass_line(32, 36, 'offbeat')
bass_line(36, 40, 'sustain')
arp(32, 39, vel=0.7)
snare_roll(32, [(2, 1), (2, 2), (2, 4), (2, 8)], vel0=0.18, vel1=0.95, stop_beat=31.0)
bus['fx'].add(S.noise_riser(T(40) - T(32), 300, 11000, 2.5), T(32), 0.8)
bus['fx'].add(S.pitch_riser(T(39, 3) - T(36), 49, 73), T(36), 0.6)
bus['cym'].add(S.reverse_cymbal(B(3)), T(39, 0), 0.8)

# DROP 1 40-55
for t in (T(40), T(48)):
    crash_at(t, 1.0)
bus['impact'].add(S.impact(), T(40), 1.0)
four_floor(40, 56)
claps(40, 56)
hats(40, 48, closed=0.4, opn=0.65)
hats(48, 56, closed=0.4, opn=0.65, ride=0.3)
bass_line(40, 56, 'rolling')
pads(40, 56, vel=0.7, attack=0.02, cut=3500)
arp(40, 56, vel=0.8)
melody('lead', 40, HOOK, vel=1.0, inst='lead')
melody('lead', 48, HOOK, vel=1.0, inst='lead')
melody('lead_lo', 40, HOOK, transpose=-12, vel=0.55, inst='lead', cut=4500)
melody('lead_lo', 48, HOOK, transpose=-12, vel=0.55, inst='lead', cut=4500)
for i in range(4):
    bus['snare'].add(S.snare(), T(55, 2 + i * .5), 0.5 + 0.1 * i)

# BRIDGE 56-63 (C# / D)
crash_at(T(56), 0.9)
bus['impact'].add(S.impact(), T(56), 0.6)
for b in range(56, 62):
    for beat in (0, 2.5):
        place_kick(T(b, beat), 0.95)
    bus['clap'].add(S.clap(), T(b, 2), 0.9)
    for beat in range(4):
        bus['hats'].add(S.hat(1.0, False, -0.2), T(b, beat + .5), 0.3)
four_floor(62, 64, 0.85)
bass_line(56, 64, 'sustain')
pads(56, 64, vel=0.45, attack=0.5, cut=2000)
arp(56, 64, vel=0.75)
bridge_stabs(56, vel=1.0)
snare_roll(62, [(1, 4), (1, 8)], vel0=0.3, vel1=1.0, stop_beat=7.5)
bus['fx'].add(S.noise_riser(T(64) - T(60), 500, 10000, 2.5), T(60), 0.7)
bus['fx'].add(S.pitch_riser(T(63, 3.5) - T(62), 49, 73), T(62), 0.5)

# DROP 2 64-79
for t in (T(64), T(72)):
    crash_at(t, 1.0)
bus['impact'].add(S.impact(), T(64), 1.0)
four_floor(64, 80)
claps(64, 80)
hats(64, 80, closed=0.42, opn=0.65, ride=0.3)
bass_line(64, 80, 'rolling')
pads(64, 80, vel=0.7, attack=0.02, cut=3800)
arp(64, 80, vel=0.75)
arp16(64, 80, vel=0.55)
for bb in (64, 72):
    melody('lead', bb, HOOK, vel=1.0, inst='lead')
    melody('lead_lo', bb, HOOK, transpose=-12, vel=0.55, inst='lead', cut=4500)
    melody('lead_hi', bb, HOOK, transpose=12, vel=0.4, inst='bell')
melody('harm', 72, HOOK_HARM, transpose=12, vel=0.6, inst='lead', cut=5000)
for i in range(8):
    bus['snare'].add(S.snare(1.0, 200 + 12 * i), T(79, 2 + i * .25), 0.35 + 0.08 * i)

# OUTRO 80-87 (D, then C#m)
crash_at(T(80), 0.9)
four_floor(80, 84, 0.9)
hats(80, 84, closed=0.3, opn=0.45)
claps(80, 84, 0.7)
bass_line(80, 88, 'sustain')
pads(80, 88, vel=0.6, attack=0.3, cut=2600)
arp(80, 88, vel=0.7)
melody('bell', 80, [(b + 2, n, l) for b, n, l in MOTIF], vel=0.6, inst='bell')
melody('bell', 82, [(b + 2, n, l) for b, n, l in MOTIF], vel=0.6, inst='bell')
crash_at(T(84), 0.8)
bus['impact'].add(S.impact(), T(84), 0.7)
melody('piano', 84, [(0, 'C#4', 12), (0, 'G#4', 12), (0, 'C#5', 12), (0, 'E5', 12), (0, 'G#5', 12)],
       vel=0.8, inst='piano')

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


t = np.arange(N) / SR
duck_hard = S.sidechain(N, kicks, depth=0.85, release=0.30)
duck_mid = S.sidechain(N, kicks, depth=0.6, release=0.34)
duck_soft = S.sidechain(N, kicks, depth=0.35, release=0.30)

GAIN = {  # dB
    'kick': -3.0, 'clap': -5.5, 'snare': -13.0, 'hats': -8.0, 'cym': -15.0, 'toms': -10.0,
    'sub': -12.0, 'bass': -4.0, 'pad': 1.0, 'arp': -6.0, 'arp16': -4.0,
    'lead': -5.0, 'lead_lo': -8.5, 'lead_hi': -12.0, 'harm': -9.0, 'brass': -4.0, 'choir': -3.0,
    'stab': -6.0, 'bell': -9.0, 'piano': -4.0, 'fx': -14.0, 'impact': -8.0,
}
DUCK = {'sub': duck_hard, 'bass': duck_hard, 'pad': duck_mid, 'arp': duck_mid, 'arp16': duck_mid,
        'lead': duck_soft, 'lead_lo': duck_soft, 'lead_hi': duck_soft, 'harm': duck_soft,
        'brass': duck_soft, 'choir': duck_mid, 'stab': duck_soft, 'fx': duck_soft}
VERB = {'clap': .18, 'snare': .15, 'toms': .25, 'pad': .35, 'arp': .3, 'arp16': .2, 'lead': .22,
        'lead_lo': .1, 'lead_hi': .35, 'harm': .25, 'brass': .2, 'choir': .4, 'stab': .45, 'bell': .45,
        'piano': .35, 'fx': .3}
DELAY = {'arp': .3, 'lead': .16, 'lead_hi': .3, 'bell': .3, 'stab': .25, 'piano': .12, 'brass': .1}

# bus processing
bus['arp'].x = stage(bus['arp'].x, HighpassFilter(250))
bus['arp16'].x = stage(bus['arp16'].x, HighpassFilter(350))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(150), Chorus(rate_hz=0.4, depth=0.25, centre_delay_ms=8, mix=0.35))
bus['choir'].x = stage(bus['choir'].x, HighpassFilter(200), Chorus(rate_hz=0.3, depth=0.3, centre_delay_ms=9, mix=0.4))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(220), PeakFilter(2800, 2.0, 0.8))
bus['lead_lo'].x = stage(bus['lead_lo'].x, HighpassFilter(180))
bus['clap'].x = stage(bus['clap'].x, HighpassFilter(300))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(5000))
bus['piano'].x = stage(bus['piano'].x, HighpassFilter(80))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(250))
bus['stab'].x = stage(bus['stab'].x, HighpassFilter(200))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(60), LowpassFilter(3000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(160))

# build: filter sweep on the lead and arp (bars 32-40)
sweep = np.full(N, 20000.0)
i0, i1 = int(T(32) * SR), int(T(40) * SR)
u = np.linspace(0, 1, i1 - i0)
sweep[i0:i1] = 700 * (16000 / 700) ** (u ** 1.6)
bus['lead'].x = S.svf(bus['lead'].x, sweep, 0.9).astype(np.float32)
# intro: arp opens up over 8 bars
sw2 = np.full(N, 20000.0)
i1 = int(T(8) * SR)
sw2[:i1] = 500 * (14000 / 500) ** (np.linspace(0, 1, i1) ** 1.3)
bus['arp'].x = S.svf(bus['arp'].x, sw2, 0.9).astype(np.float32)

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('SONG5_MUTE', '').split(',')))
for k in names:
    if k in MUTE:
        continue
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    seg = x.T.astype(np.float64)
    levels[k] = pyln.Meter(SR).integrated_loudness(seg) if np.any(seg) else -99.0
    mix += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]

pre = int(0.02 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
verb = stage(vs, HighpassFilter(350), LowpassFilter(9000),
             Reverb(room_size=0.88, damping=0.45, wet_level=1.0, dry_level=0.0, width=1.0))
verb = verb * duck_soft[None, :]
dly = S.pingpong(delay_send, B(0.75), feedback=0.42, repeats=8)
dly = stage(dly, Reverb(room_size=0.5, damping=0.6, wet_level=0.25, dry_level=1.0, width=1.0))
mix = mix + verb * 0.9 + dly * 0.6

# section dynamics (dB at bar lines, linear in between): quiet breakdown,
# a crescendo through the build, the drops loudest, drop 2 a touch above drop 1
SECTION_DB = [(0, -4), (8, -2.5), (24, -2.5), (24.01, -7), (32, -7), (39.75, -1.5),
              (40, 0), (55.9, 0), (56, -3), (63.9, -2), (64, 0.5), (79.9, 0.5), (80, -2),
              (84, -3), (88, -3)]
bars_axis = np.arange(N) / SR / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(90, 0.0, 0.7), HighShelfFilter(9000, -1.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.5 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song5_edm.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
