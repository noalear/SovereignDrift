#!/usr/bin/env python3
# new10_ambient_techno.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Lantern Orbit": new track 10, ambient techno / IDM (118 BPM, A minor), in the
# 1992-99 manner of Orbital, Global Communication, The Future Sound of London,
# early Aphex Twin and Boards of Canada.  A NEW piece (not a remix), built from
# under-used fragments of two of the game's songs.
#
# Credits: the source music is Stardock's (the original Stellar Frontier game
# music, mid/sf-music1.mid and mid/sf-music3.mid, Stardock Shared Source
# license).  Composed, synthesized and mixed in code with tools/music/edm/synth.py
# and synth_new10.py: no samples, no SoundFonts.
#
# Built from (bar numbers are the original MIDIs', 1-based):
#   sf-music3.mid (A minor)
#   - the inner string-pad line C5 D5 F5 D5 | C5 D5 F5 E5 (inst 8, bars 1-8; one
#     whole note per bar, heard only as a quiet intro pad in song3_liquid.py):
#     it is the skeleton of the main melody (theme A), re-harmonized
#     Am9 Bbmaj7 Dm9 Bb | Am9 Bbmaj7 Dm9 Em7 and filled out with pickups; its
#     bar-7 turn borrows the F5 A5 step of the flute (bar 33)
#   - the C-section bass figures D2 A1 D2 F2 (bar 52) and C2 D2 E2 C2 (bar 56),
#     quarter notes: extended by sequence (Bb1 F1 Bb1 D2, A1 B1 C2 E2) into the
#     bass riff of theme B (Dm9 C Bbmaj7 Am9)
#   - the diminished string fall D6 B5 G#5 F5 E5 (bar 8, played once in the
#     original) as a glass-bell turnaround on the E7b9 bars
#   sf-music1.mid (C minor, transposed down a minor third to A minor)
#   - the string descent G4 F4 | Eb4 D4 | C#4 (bars 35-37, not used by
#     song1_ambient.py) -> E D | C B | Bb..A: the counter-melody of theme B, with
#     its chromatic Phrygian Bb (song 1's i-bII sway) resolving to A; its
#     sequence up a step and a harmony a third above for the second B section
#   - the brass swell Eb4/G4 -> F4/Ab4 (bars 12-13) -> C4/E4 -> D4/F4, as the
#     soft two-voice brass pad over the Am9 / Bbmaj7 sway of the intro,
#     breakdown and outro
#   - the brass pickup rhythm C4 on beats 3, 4, 4.5 -> C4 on the downbeat
#     (bars 18-19) as the chord-stab rhythm of the B sections
#   - the taiko rhythm (bars 1-4: 8ths with accents and the 16th pair on beat
#     4.5) as the soft conga pattern
# New: the form (beatless intro on the Am9-Bbmaj7 sway, build, theme A, theme B,
# beatless breakdown, broken-beat rebuild, peak A + B, theme A, outro, a final
# Am9 hit with a short ring-out that loops back into the intro), the chords,
# a soft 4/4 / broken beat (round tuned kick on A1, short round "pdlap"
# snare/clap with 4 round-robins, gentle hats, shaker, conga), warm analog pads
# with tape wow, glass-bell and marimba arpeggios in 3-against-4, a filtered
# Juno sequence, string machine, deep warm bass, a solar-wind noise bed, hall
# reverb and dotted-eighth ping-pong delay.
#
#   python3 tools/music/edm/new10_ambient_techno.py OUT.wav [bar0 bar1]
#   NEW10_MUTE=kick,snap,hats,perc  (mute buses, for the chroma check)
import os
import sys

import numpy as np
import pyloudnorm as pyln
import soundfile as sf_
from pedalboard import (Pedalboard, Chorus, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new10 as X  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 118.0
SPB = 60.0 / BPM
NBARS = 96
TAIL = 2.5
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    """Absolute time (s) of a bar/beat in the song."""
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 3 <= bar < RB1


# ------------------------------------------------------------- harmony

PROG_A = ['Am9', 'Bbmaj7', 'Dm9', 'Bbadd9', 'Am9', 'Bbmaj7', 'Dm9', 'Em7']
PROG_B = ['Dm9', 'Cadd9', 'Bbmaj7', 'Am9']
SWAY = ['Am9', 'Am9', 'Bbmaj7', 'Bbmaj7', 'Am9', 'Am9', 'Bbmaj7', 'E7b9']
BRK = ['Am9', 'Am9', 'Bbmaj7', 'Bbmaj7', 'Dm9', 'Dm9', 'E7b9', 'E7b9']
OUTRO = ['Am9', 'Am9', 'Bbmaj7', 'Bbmaj7', 'Dm9', 'E7b9', 'Am9', 'Am9']
CHORDS = (SWAY +                 # 0-7    intro (beatless)
          PROG_A +               # 8-15   build
          PROG_A * 2 +           # 16-31  theme A
          PROG_B * 4 +           # 32-47  theme B
          BRK +                  # 48-55  breakdown (beatless)
          PROG_A +               # 56-63  rebuild (broken beat)
          PROG_A +               # 64-71  peak A
          PROG_B * 2 +           # 72-79  peak B
          PROG_A +               # 80-87  theme A
          OUTRO)                 # 88-95  outro, final Am9 at 94
assert len(CHORDS) == NBARS

PAD = {
    'Am9': ['A3', 'C4', 'E4', 'G4', 'B4'],
    'Bbmaj7': ['Bb3', 'D4', 'F4', 'A4'],
    'Dm9': ['F3', 'A3', 'C4', 'E4'],
    'Bbadd9': ['Bb3', 'C4', 'D4', 'F4'],
    'Em7': ['G3', 'B3', 'D4', 'E4'],
    'Cadd9': ['G3', 'C4', 'D4', 'E4'],
    'E7b9': ['G#3', 'D4', 'F4', 'B4'],
    'E7sus': ['A3', 'B3', 'D4', 'E4'],
}
ROOT = {'Am9': 'A1', 'Bbmaj7': 'Bb1', 'Dm9': 'D2', 'Bbadd9': 'Bb1', 'Em7': 'E2', 'Cadd9': 'C2',
        'E7b9': 'E2', 'E7sus': 'E2'}
# glass arpeggio: 3-note cells over eighths (3 against 4)
ARP3 = {'Am9': ['A4', 'E5', 'B5'], 'Bbmaj7': ['Bb4', 'F5', 'A5'], 'Dm9': ['F4', 'C5', 'E5'],
        'Bbadd9': ['Bb4', 'F5', 'C6'], 'Em7': ['G4', 'D5', 'B5'], 'Cadd9': ['G4', 'D5', 'E5'],
        'E7b9': ['G#4', 'D5', 'B5'], 'E7sus': ['A4', 'D5', 'B5']}
# marimba / sequence tones (low register)
LOW4 = {'Am9': ['A3', 'E4', 'C4', 'B4'], 'Bbmaj7': ['Bb3', 'F4', 'D4', 'A4'], 'Dm9': ['D4', 'A4', 'F4', 'C5'],
        'Bbadd9': ['Bb3', 'F4', 'D4', 'C5'], 'Em7': ['E4', 'B4', 'G4', 'D5'], 'Cadd9': ['C4', 'G4', 'E4', 'D5'],
        'E7b9': ['E4', 'B4', 'G#4', 'D5'], 'E7sus': ['E4', 'B4', 'A4', 'D5']}

# ------------------------------------------------------------- themes (beats; note; length)

# theme A: skeleton = song 3's inner string line C5 D5 F5 D5 | C5 D5 F5 E5 (bars 1-8)
MEL_A = [(0, 'C5', 2.5), (2.5, 'B4', .5), (3, 'A4', 1),
         (4, 'D5', 3), (7, 'C5', .5), (7.5, 'D5', .5),
         (8, 'F5', 2), (10, 'E5', 1), (11, 'D5', 1),
         (12, 'D5', 3), (15, 'A4', .5), (15.5, 'Bb4', .5),
         (16, 'C5', 2.5), (18.5, 'B4', .5), (19, 'A4', 1),
         (20, 'D5', 2), (22, 'E5', 1), (23, 'F5', 1),
         (24, 'F5', 2), (26, 'A5', 1.5), (27.5, 'G5', .5),
         (28, 'E5', 4)]
MEL_A2 = MEL_A[:-4] + [(24, 'F5', 2), (26, 'E5', 1), (27, 'D5', 1),
                       (28, 'E5', 3), (31, 'D5', .5), (31.5, 'B4', .5)]
# theme B counter-line: song 1's string descent G4 F4 | Eb4 D4 | C#4 (bars 35-37), in A minor
B_P1 = [(0, 'E5', 2), (2, 'D5', 2), (4, 'C5', 2), (6, 'B4', 2), (8, 'Bb4', 6), (14, 'A4', 2)]
B_P2 = [(0, 'F5', 2), (2, 'E5', 2), (4, 'D5', 2), (6, 'C5', 2), (8, 'D5', 6), (14, 'C5', 2)]
B_H1 = [(0, 'G5', 2), (2, 'F5', 2), (4, 'E5', 2), (6, 'D5', 2), (8, 'D5', 6), (14, 'C5', 2)]
B_H2 = [(0, 'A5', 2), (2, 'G5', 2), (4, 'F5', 2), (6, 'E5', 2), (8, 'F5', 6), (14, 'E5', 2)]
# theme B bass: song 3's C-section figures (bars 52, 56) and their sequences
BASS_B = {'Dm9': ['D2', 'A1', 'D2', 'F2'], 'Cadd9': ['C2', 'D2', 'E2', 'C2'],
          'Bbmaj7': ['Bb1', 'F1', 'Bb1', 'D2'], 'Am9': ['A1', 'B1', 'C2', 'E2']}
# song 3's diminished string fall (bar 8)
FALL = [(0, 'D6', .75), (.75, 'B5', .75), (1.5, 'G#5', .5), (2, 'F5', 1), (3, 'E5', 1)]
# song 1's taiko rhythm (bars 1-4) -> 2-bar conga pattern: (beat, accent, high?)
CONGA = [(0, 1.0, 0), (.5, .45, 1), (1, .5, 1), (1.5, .45, 1), (2, .8, 0), (3.5, .6, 1), (3.75, .55, 1),
         (4, .9, 0), (6, .7, 0), (6.5, .4, 1), (7.5, .75, 1)]

# --------------------------------------------------------------- buses

names = ['kick', 'snap', 'hats', 'perc', 'sub', 'bass', 'pad', 'brass', 'strings', 'lead',
         'glass', 'bells', 'mar', 'seq', 'stab', 'wind']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def add(name, sig, t, gain=1.0):
    bus[name].add(sig, t - T0, gain)


rg = np.random.default_rng(10)


def hv(v, spread=0.08):
    return v * (1 + spread * (rg.random() * 2 - 1))


def jit(t, ms=3.0):
    return t + (rg.random() * 2 - 1) * ms / 1000


# ------------------------------------------------------------- drums

def kick_at(bar, beat, vel=1.0):
    if not live(bar):
        return
    t = T(bar, beat)
    add('kick', cached('kick', X.round_kick), t, vel)
    kicks.append(t - T0)


_snap_i = [0]


def snap_at(bar, beat, vel=1.0, ghost=False):
    if not live(bar):
        return
    v = _snap_i[0] % 4
    _snap_i[0] += 1
    sig = cached(('snap', v, ghost), lambda: X.snap(1.0, v, dark=1.0 if ghost else 0.0))
    add('snap', sig, jit(T(bar, beat), 2), hv(vel, 0.06))


def hats_bar(bar, closed=0.5, ghost=0.18, opn=0.0, swing=0.04, shaker=0.0):
    if not live(bar):
        return
    for beat in range(4):
        add('hats', cached(('hc', beat % 2), lambda: X.soft_hat(1.0, False, 0.2)),
            jit(T(bar, beat + .5)), hv(closed))
        for s16 in (.25, .75):
            if ghost:
                add('hats', cached(('hg', s16), lambda: X.soft_hat(1.0, False, -0.25)),
                    jit(T(bar, beat + s16 + swing)), hv(ghost, 0.25))
        if shaker:
            for s16 in (0, .25, .5, .75):
                acc = (.5, .8, .6, 1.0)[int(s16 * 4)]
                add('hats', cached(('sh', s16), lambda: X.shaker(1.0, 0.35)),
                    jit(T(bar, beat + s16 + (swing if s16 in (.25, .75) else 0))), hv(shaker * acc, 0.2))
    if opn:
        add('hats', cached('ho', lambda: X.soft_hat(1.0, True, 0.1)), T(bar, 3.5), opn)


def beat_44(bar0, bar1, kick=1.0, snap=0.8, closed=0.5, ghost=0.18, opn=0.0, shaker=0.0, ghosts=True):
    for b in range(bar0, bar1):
        for beat in range(4):
            if kick:
                kick_at(b, beat, kick * (1.0 if beat in (0, 2) else 0.92))
        if snap:
            snap_at(b, 1, snap)
            snap_at(b, 3, snap * 0.95)
            if ghosts and b % 2 == 1:
                snap_at(b, 3.75, snap * 0.22, ghost=True)
            if ghosts and b % 4 == 2:
                snap_at(b, 1.75, snap * 0.18, ghost=True)
        hats_bar(b, closed, ghost, opn if b % 2 == 1 else 0.0, shaker=shaker)


def beat_broken(bar0, bar1, kick=1.0, snap=0.75, closed=0.45, ghost=0.22):
    """IDM broken beat: kick 1, 2-and, 3-and-a ... snare 2 and 4, ghosts."""
    pats = [[0, 1.5, 2.75], [0, .75, 2.5]]
    for b in range(bar0, bar1):
        for i, beat in enumerate(pats[b % 2]):
            kick_at(b, beat, kick * (1.0 if i == 0 else 0.8))
        snap_at(b, 1, snap)
        snap_at(b, 3, snap)
        snap_at(b, 2.25 if b % 2 else 3.75, snap * 0.2, ghost=True)
        hats_bar(b, closed, ghost, 0.0)
        if live(b):
            for beat in (1.75, 3.25):
                add('perc', cached('rim', X.rim), jit(T(b, beat)), hv(0.35))


def conga(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1, 2):
        if not live(b):
            continue
        for beat, acc, hi in CONGA:
            md = 'E4' if hi else 'A3'
            add('perc', cached(('cg', hi), lambda: X.conga(m(md), 1.0, 0.35 if hi else -0.25)),
                jit(T(b, beat), 4), hv(vel * acc, 0.1))


def swell(bar, beats=4, vel=1.0):
    """Soft reverse-cymbal swell into a section (low-passed, low level)."""
    if not live(bar):
        return
    sig = cached(('sw', beats), lambda: S.butter(S.reverse_cymbal(B(beats)), 5000, 'low', 2).astype(np.float32))
    add('wind', sig, T(bar) - B(beats), vel * 1.4)


# ------------------------------------------------------------- tonal parts

def pads(bar0, bar1, vel=1.0, attack=0.6, release=1.4, cut=1600.0, voices=3):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if live(b) or live(e - 1):
            dur = T(e) - T(b)
            for note in PAD[ch]:
                add('pad', X.analog_pad(m(note), dur, 1.0, T(b), attack=attack, release=release, cut=cut,
                                        voices=voices), T(b), vel * 0.5)
        b = e


def brass_swell(bar, notes, beats, vel=1.0):
    if not live(bar):
        return
    for note in notes:
        add('brass', X.analog_pad(m(note), B(beats), 1.0, T(bar), attack=1.6, release=1.8, cut=950.0,
                                  voices=2, detune=0.07, q=0.9, bright_env=1.2, width=0.5), T(bar), vel)


def sway_brass(bar0, vel=1.0, outro=False):
    """Song 1's brass swell (bars 12-13), C4/E4 -> D4/F4, over the Am9 / Bbmaj7 sway."""
    if outro:   # 88 Am9 | 90-93 Bbmaj7 Bbmaj7 Dm9 E7b9 (the final C4/E4 comes with the last hit)
        brass_swell(bar0, ['C4', 'E4'], 8, vel)
        brass_swell(bar0 + 2, ['D4', 'F4'], 15.5, vel * 1.05)
        return
    brass_swell(bar0, ['C4', 'E4'], 8, vel)
    brass_swell(bar0 + 2, ['D4', 'F4'], 8, vel * 1.05)
    brass_swell(bar0 + 4, ['C4', 'E4'], 8, vel)
    brass_swell(bar0 + 6, ['D4', 'F4'], 7.5, vel * 1.1)


def bass_a(bar0, bar1, sub=1.0, pulse=0.0, style='bar'):
    """Theme A bass: sustained sub on the root + offbeat warm pulses an octave up."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        r = m(ROOT[CHORDS[b]])
        if sub:
            add('sub', X.warm_bass(r, B(3.9), 1.0, cut=160, env=120, saw_amt=0.25, a=0.02, r=0.08),
                T(b), sub)
        if pulse:
            for beat in range(4):
                sig = cached(('pl', r), lambda: X.warm_bass(r + 12, B(.4), 1.0, cut=300, env=900, decay=0.06,
                                                            saw_amt=0.9, r=0.03))
                add('bass', sig, T(b, beat + .5), hv(pulse * (1.0 if beat % 2 else 0.85), 0.05))


def bass_b(bar0, bar1, vel=1.0):
    """Theme B bass riff: song 3's C-section quarter-note figures."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        for beat, note in enumerate(BASS_B[CHORDS[b]]):
            md = m(note)
            sig = cached(('bb', md), lambda: X.warm_bass(md, B(.82), 1.0, cut=220, env=650, decay=0.1,
                                                         saw_amt=0.6, r=0.05))
            add('bass', sig, T(b, beat), vel * (1.0, 0.82, 0.92, 0.86)[beat])
            # the sub follows the riff's low note
            add('sub', cached(('bs', md), lambda: X.warm_bass(md, B(.9), 1.0, cut=150, env=60,
                                                               saw_amt=0.1, r=0.05)), T(b, beat), vel * 0.8)


def glass_arp(bar0, bar1, vel=1.0, sparse=False):
    """3-note glass cells over eighths: a 3-against-4 cross-rhythm over the bar."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        cell = ARP3[CHORDS[b]]
        for i in range(8):
            g = b * 8 + i
            k = g % 3
            if sparse and k == 1:
                continue
            note = cell[k]
            acc = 1.0 if k == 0 else (0.7 if k == 1 else 0.8)
            add('glass', cached(('gl', note), lambda: X.glass(m(note), B(.45), 1.0, decay=0.9,
                                                             pan=(-0.35, 0.0, 0.35)[k])),
                T(b, i * .5), hv(vel * acc, 0.07))


def marimba_arp(bar0, bar1, vel=1.0):
    pat = [0, 1, 2, 3, 2, 1, 2, 3]
    for b in range(bar0, bar1):
        if not live(b):
            continue
        tones = LOW4[CHORDS[b]]
        for i in range(8):
            note = tones[pat[i]]
            acc = 1.0 if i % 4 == 0 else (0.8 if i % 2 == 0 else 0.65)
            add('mar', cached(('mr', note), lambda: X.marimba(m(note), B(.5), 1.0, pan=0.25 * np.sin(i))),
                jit(T(b, i * .5 + (0.03 if i % 2 else 0)), 2), hv(vel * acc, 0.08))


SEQ16 = [0, 2, 1, 3, 2, 1, 0, 2, 3, 1, 2, 0, 1, 3, 2, 1]


def seq(bar0, bar1, vel=1.0):
    """Filtered Juno sequence in 16ths, one octave below the marimba (the LFO is on the bus)."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        tones = LOW4[CHORDS[b]]
        for i in range(16):
            note = m(tones[SEQ16[i]]) - 12
            acc = 1.0 if i % 4 == 0 else (0.55 if i % 2 else 0.75)
            add('seq', cached(('sq', note), lambda: X.softpluck(note, B(.22), 1.0, decay=0.09, cut=500,
                                                                 env=1800, pan=0.0)),
                T(b, i * .25 + (0.035 if i % 2 else 0)), hv(vel * acc, 0.1))


def stabs(bar0, bar1, vel=1.0):
    """Song 1's brass pickup rhythm (bars 18-19): beats 3, 4, 4.5 -> the next downbeat."""
    for b in range(bar0 + 1, bar1, 2):
        if not live(b):
            continue
        nxt = CHORDS[b + 1] if b + 1 < NBARS else CHORDS[b]
        notes = [m(n) + 12 for n in PAD[nxt][-3:]]
        for bar, beat, dur, v in ((b, 2, .35, .75), (b, 3, .35, .8), (b, 3.5, .35, .85), (b + 1, 0, 1.4, 1.0)):
            if bar >= bar1:
                continue
            for md in notes:
                add('stab', cached(('st', md, dur), lambda: X.softpluck(md, B(dur), 1.0, decay=0.2, cut=700,
                                                                     env=2000)), T(bar, beat), vel * v)


def lead(bar0, notes, vel=1.0, transpose=0, **kw):
    if not (live(bar0) or live(bar0 + int(max(b + d for b, _, d in notes) / 4))):
        return
    t0 = T(bar0, notes[0][0])
    ev = [(T(bar0, b) - t0, m(n) + transpose, B(d)) for b, n, d in notes]
    add('lead', X.warm_lead(ev, 1.0, t_abs=t0, **kw), t0, vel)


def strings(bar0, notes, vel=1.0, transpose=-12):
    for b, n, d in notes:
        bar = bar0 + int(b // 4)
        if not live(bar):
            continue
        add('strings', X.string_machine(m(n) + transpose, B(d) - 0.03, 1.0, T(bar0, b)), T(bar0, b), vel)


def bells(bar0, notes, vel=1.0, transpose=0, decay=1.2):
    for b, n, d in notes:
        if not live(bar0 + int(b // 4)):
            continue
        md = m(n) + transpose
        add('bells', cached(('bl', md, decay), lambda: X.glass(md, B(.6), 1.0, decay=decay, bright=0.8)),
            T(bar0, b), vel)


# ---------------------------------------------------------- arrangement

# continuous: the solar-wind bed (louder in the beatless parts)
if True:
    wind = X.wind(N)
    lvl = np.interp(np.arange(N) / SR + T0,
                    [0, T(8), T(16), T(47), T(48), T(56), T(64), T(88), T(94), T(96) + 3],
                    [1.0, 0.7, 0.45, 0.45, 1.0, 0.75, 0.4, 0.45, 0.9, 0.5]).astype(np.float32)
    bus['wind'].x += wind * lvl[None, :]
    del wind, lvl

# INTRO 0-7: beatless, the Am9 / Bbmaj7 sway, song 1's brass swell
pads(0, 8, vel=0.8, attack=2.2, release=1.8, cut=1000)
sway_brass(0, 0.8)
glass_arp(2, 8, vel=0.5, sparse=True)
bells(7, FALL, vel=0.55)
bass_a(4, 8, sub=0.5)
swell(8, 4, 0.5)

# BUILD 8-15: the beat fades in, the arp fills out
pads(8, 16, vel=0.75, attack=0.5, cut=1300)
glass_arp(8, 16, vel=0.65)
for b in range(8, 16):
    kick_at(b, 0, 0.8)
    kick_at(b, 2, 0.8)
    if b >= 10:
        kick_at(b, 1, 0.75)
        kick_at(b, 3, 0.75)
    hats_bar(b, 0.3 + 0.02 * (b - 8), 0.1 if b >= 12 else 0.0)
    if b >= 12:
        snap_at(b, 1, 0.6)
        snap_at(b, 3, 0.6)
bass_a(8, 16, sub=0.85, pulse=0.0)
bass_a(12, 16, sub=0.0, pulse=0.5)
swell(16, 4, 0.6)

# THEME A 16-31
beat_44(16, 32, kick=1.0, snap=0.8, closed=0.5, ghost=0.18, opn=0.25)
pads(16, 32, vel=0.7, attack=0.35, cut=1700)
bass_a(16, 32, sub=1.0, pulse=0.7)
glass_arp(16, 32, vel=0.6)
lead(16, MEL_A, vel=1.0)
lead(24, MEL_A2, vel=1.0)
bells(24, MEL_A2[-6:], vel=0.25, transpose=12)
swell(32, 4, 0.6)

# THEME B 32-47: song 3's bass riff, song 1's string descent, stab rhythm
beat_44(32, 48, kick=1.0, snap=0.8, closed=0.45, ghost=0.15, shaker=0.22)
conga(32, 48, 0.6)
pads(32, 48, vel=0.6, attack=0.4, cut=1900)
bass_b(32, 48, vel=1.0)
marimba_arp(32, 48, vel=0.7)
glass_arp(40, 48, vel=0.4, sparse=True)
for i, ph in enumerate((B_P1, B_P2, B_P1, B_P2)):
    strings(32 + 4 * i, ph, vel=0.8)
stabs(32, 48, vel=0.55)

# BREAKDOWN 48-55: beatless, theme A's skeleton on bells, the brass sway
pads(48, 56, vel=0.85, attack=1.4, release=1.8, cut=1200)
sway_brass(48, 0.75)
bass_a(48, 56, sub=0.6)
bells(48, [(0, 'C5', 4), (4, 'D5', 4), (8, 'F5', 4), (12, 'D5', 4),
           (16, 'C5', 4), (20, 'D5', 4), (24, 'F5', 2), (26, 'E5', 2)], vel=0.55, decay=2.0)
bells(48, [(b + 2, n, d) for b, n, d in [(0, 'E5', 2), (4, 'F5', 2), (8, 'A5', 2), (12, 'F5', 2),
                                          (16, 'E5', 2), (20, 'F5', 2)]], vel=0.3, decay=1.6)
bells(55, FALL, vel=0.55)
glass_arp(52, 56, vel=0.3, sparse=True)
swell(56, 8, 0.55)

# REBUILD 56-63: broken beat, theme A on glass bells
beat_broken(56, 62)
beat_44(62, 64, kick=0.95, snap=0.75, closed=0.45, ghost=0.18)
conga(56, 64, 0.5)
pads(56, 64, vel=0.7, attack=0.4, cut=1500)
bass_a(56, 64, sub=0.95, pulse=0.0)
bass_a(60, 64, sub=0.0, pulse=0.6)
bells(56, MEL_A, vel=0.6)
bells(56, MEL_A, vel=0.2, transpose=12)
glass_arp(56, 64, vel=0.45, sparse=True)
seq(60, 64, vel=0.5)
swell(64, 4, 0.7)

# PEAK A 64-71
beat_44(64, 72, kick=1.0, snap=0.85, closed=0.52, ghost=0.2, opn=0.3, shaker=0.15)
conga(64, 72, 0.45)
pads(64, 72, vel=0.75, attack=0.3, cut=2200)
bass_a(64, 72, sub=1.0, pulse=0.75)
glass_arp(64, 72, vel=0.6)
seq(64, 72, vel=0.6)
lead(64, MEL_A, vel=1.0)
bells(64, MEL_A, vel=0.28, transpose=12)
swell(72, 4, 0.6)

# PEAK B 72-79: riff, descent on strings, its harmony on the lead, both arps
beat_44(72, 80, kick=1.0, snap=0.85, closed=0.5, ghost=0.18, shaker=0.25, opn=0.2)
conga(72, 80, 0.6)
pads(72, 80, vel=0.65, attack=0.3, cut=2200)
bass_b(72, 80, vel=1.0)
marimba_arp(72, 80, vel=0.65)
glass_arp(72, 80, vel=0.45, sparse=True)
seq(72, 80, vel=0.5)
strings(72, B_P1, vel=0.85)
strings(76, B_P2, vel=0.85)
lead(72, B_H1, vel=0.9)
lead(76, B_H2, vel=0.9)
stabs(72, 80, vel=0.5)
swell(80, 4, 0.5)

# THEME A 80-87 (the variation), then the line lands on C5 over the outro's Am9
beat_44(80, 88, kick=1.0, snap=0.8, closed=0.48, ghost=0.16, opn=0.2)
pads(80, 88, vel=0.72, attack=0.35, cut=1900)
bass_a(80, 88, sub=1.0, pulse=0.65)
glass_arp(80, 88, vel=0.6)
seq(80, 84, vel=0.4)
lead(80, MEL_A2 + [(32, 'C5', 6)], vel=1.0)

# OUTRO 88-95: the sway again, the beat thins, a final Am9 hit at 94 with a ring-out
beat_44(88, 92, kick=0.95, snap=0.7, closed=0.4, ghost=0.12, ghosts=False)
for b in (92, 93):
    for beat in range(4):
        kick_at(b, beat, 0.9)
    hats_bar(b, 0.32, 0.0)
snap_at(93, 1, 0.6)
snap_at(93, 3, 0.65)
pads(88, 94, vel=0.75, attack=0.5, cut=1500)
sway_brass(88, 0.7, outro=True)
bass_a(88, 94, sub=0.95, pulse=0.45)
glass_arp(88, 94, vel=0.5)
bells(93, FALL, vel=0.5)
# final hit (bar 94): kick, deep A, the Am9 chord on pad + bells, held two bars
kick_at(94, 0, 1.0)
if live(94):
    add('sub', X.warm_bass(m('A1'), B(6), 1.0, cut=160, env=200, saw_amt=0.3, a=0.01, r=0.35), T(94), 1.0)
    for note in PAD['Am9']:
        add('pad', X.analog_pad(m(note), B(6.5), 1.0, T(94), attack=0.08, release=1.2, cut=1700, voices=3),
            T(94), 0.42)
    for note in ('A4', 'E5', 'B5', 'C6'):
        add('bells', X.glass(m(note), B(4), 1.0, decay=1.6, bright=0.8), T(94), 0.35)
    brass_swell(94, ['C4', 'E4'], 6, 0.6)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_bass = S.sidechain(N, kicks, depth=0.55, release=0.28)
duck_pad = S.sidechain(N, kicks, depth=0.28, release=0.36)
duck_soft = S.sidechain(N, kicks, depth=0.15, release=0.3)

GAIN = {  # dB
    'kick': -4.0, 'snap': -4.5, 'hats': 6.0, 'perc': -3.0, 'sub': -8.5, 'bass': -8.5,
    'pad': 2.0, 'brass': -4.0, 'strings': -2.0, 'lead': -3.0, 'glass': -4.0, 'bells': -5.0,
    'mar': -5.0, 'seq': 0.0, 'stab': -4.0, 'wind': -14.0,
}
DUCK = {'sub': duck_bass, 'bass': duck_bass, 'pad': duck_pad, 'brass': duck_pad, 'strings': duck_soft,
        'glass': duck_soft, 'mar': duck_soft, 'seq': duck_pad, 'stab': duck_soft}
VERB = {'snap': .16, 'hats': .06, 'perc': .2, 'pad': .3, 'brass': .3, 'strings': .4, 'lead': .28,
        'glass': .45, 'bells': .55, 'mar': .3, 'seq': .2, 'stab': .45, 'wind': .3}
DELAY = {'glass': .3, 'bells': .3, 'lead': .2, 'stab': .3, 'seq': .15, 'mar': .12}

# bus processing (done per bus inside the mix loop, so only one copy is alive)
FX = {
    'pad': lambda x: stage(x, HighpassFilter(140), Chorus(rate_hz=0.25, depth=0.25, centre_delay_ms=9, mix=0.45)),
    'strings': lambda x: stage(x, HighpassFilter(160), Chorus(rate_hz=0.6, depth=0.35, centre_delay_ms=7, mix=0.5),
                               LowpassFilter(7000)),
    'brass': lambda x: stage(x, HighpassFilter(150)),
    'lead': lambda x: stage(x, HighpassFilter(200), PeakFilter(2800, 1.0, 0.8)),
    'glass': lambda x: stage(x, HighpassFilter(300), LowpassFilter(11000)),
    'bells': lambda x: stage(x, HighpassFilter(300), LowpassFilter(11000)),
    'mar': lambda x: stage(x, HighpassFilter(160)),
    'stab': lambda x: stage(x, HighpassFilter(250)),
    'snap': lambda x: stage(x, HighpassFilter(140)),
    'hats': lambda x: stage(x, HighpassFilter(4500), LowpassFilter(11000)),
    'perc': lambda x: stage(x, HighpassFilter(120)),
    'bass': lambda x: stage(x, HighpassFilter(38), LowpassFilter(2500)),
    'sub': lambda x: stage(x, LowpassFilter(200)),
    'wind': lambda x: stage(x, HighpassFilter(200)),
}


def evolve(k, x):
    """Evolving filters: pads breathe over 16 bars, the sequence over 8, the intro glass opens."""
    tt = np.arange(N) / SR + T0
    if k == 'pad':
        x = S.svf(x, 2600 * 2 ** (0.7 * np.sin(2 * np.pi * tt / T(16) - 1.2)), 0.8)
    elif k == 'seq':
        x = S.svf(x, 1100 * 2 ** (1.0 * np.sin(2 * np.pi * tt / T(8) - 1.5)), 1.1)
    elif k == 'glass':
        x = S.svf(x, np.interp(tt, [T(2), T(8), T(9)], [900, 6000, 20000]), 0.8)
    return np.asarray(x, np.float32)


mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
MUTE = set(filter(None, os.environ.get('NEW10_MUTE', '').split(',')))
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        del x
        continue
    if k in FX:
        x = FX[k](x)
    x = evolve(k, x)
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= DUCK[k][None, :]
    levels[k] = meter.integrated_loudness(x.T.astype(np.float64)) if np.any(x) else -99.0
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    if k in DELAY:
        delay_send += x * np.float32(DELAY[k])
    del x

verb = X.convolve(stage(verb_send, HighpassFilter(300), LowpassFilter(8000)), X.hall_ir())
del verb_send
dly = S.pingpong(delay_send, B(0.75), feedback=0.4, repeats=7, lp=4500, hp=400)
del delay_send
dly = stage(dly, LowpassFilter(6000))
mix += verb * np.float32(0.55) + dly * np.float32(0.5)
del verb, dly

# section dynamics (dB at bar lines): quiet intro and breakdown, peaks 64-79
SECTION_DB = [(0, -4.5), (8, -3.5), (15.9, -2.5), (16, -1.5), (31.9, -1.5), (32, -1.0), (47.9, -1.0),
              (48, -5.0), (55.9, -5.0), (56, -3.5), (63.9, -2.0), (64, 0.0), (79.9, 0.3), (80, -1.0),
              (87.9, -1.0), (88, -2.0), (94, -2.5), (97, -2.5)]
bars_axis = (np.arange(N) / SR + T0) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(26))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-15.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(100, 0.5, 0.7), PeakFilter(3200, 2.5, 0.7), HighShelfFilter(6000, 0.5, 0.7),
            Compressor(threshold_db=-16, ratio=1.8, attack_ms=25, release_ms=200))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=120))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

# trim: the song ends at bar 96 plus a short natural ring (full render only)
if RB1 == NBARS:
    end = int((T(96) + 0.9 - T0) * SR)
    mix = mix[:, :end]
    fade = int(0.6 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade, dtype=np.float32)[None, :]

out = sys.argv[1] if len(sys.argv) > 1 else 'new10.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, gated):')
ref = levels.get('kick', 0.0)
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}   ({v - ref:+5.1f} vs kick)')
