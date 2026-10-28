#!/usr/bin/env python3
# new18_ambient_breaks.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Quiet Drive": new track 18 (energy tag CRUISE), late-90s downtempo /
# ambient breakbeat at 92 BPM in D minor, in the manner of Boards of Canada,
# Global Communication (76:14) and The Future Sound of London (Lifeforms):
# dusty soft breaks with a round tuned snare, warm detuned pads, Rhodes and a
# gentle sighing motif.  A NEW piece (not a remix), built from two lyrical
# fragments of the game's songs that the existing scripts do not feature.
#
# Credits: the source music is Stardock's (the original Stellar Frontier game
# music, mid/sf-music6.mid and mid/sf-music3.mid, Stardock Shared Source
# license).  Composed, synthesized and mixed in code with tools/music/edm/
# synth.py, synth_new10.py, synth_triphop.py and synth_new18.py: no samples,
# no SoundFonts.
#
# Built from (bar numbers are the original MIDIs'; sf-music6 counted from the
# first downbeat after its two-beat pickup, as in song6_symphonic_metal.py;
# sf-music3 1-based, as in song3_liquid.py):
#   sf-music6.mid (E-flat minor), transposed down a semitone to D minor
#   - the piccolo's sighing line over the "ascent" (bars 81-88; song 6's remix
#     uses only the ascent's chords, bass and choir, not this line):
#     Bb4 . . F#4 | Ab4 . . E4 | F#4 . . Eb4 | Ab4 . B4 .  (3 + 1 beats, falling
#     thirds) -> A4 F4 | G4 Eb4 | F4 D4 | G4 Bb4: motif A, the main theme,
#     with a new answering phrase (F4 . . D4 | Bb4 . A4 . ., ending on the 9th)
#   - its harmony, the ascent Ebm E F# Abm -> Dm9 Ebmaj9 F6/9 Gm9: the main
#     chord loop of the track (re-voiced as extended chords)
#   sf-music3.mid (A minor), transposed up a fourth to D minor
#   - the choir's counter-line of the B section (Choir Aahs, bars 33-41; heard
#     only far in the background, at -23 dB, in song3_liquid.py):
#     D4 F4 | E4 | A4 B4 | C5 | D5 | D5 C5 | B4 . . | . . C5 | A4
#     -> G4 Bb4 | A4 | D5 E5 | F5 | G5 | G5 F5 | E5 | . . F5 (D5): theme B,
#     re-harmonized Gm9 Bbmaj9 A7sus4 Dm9 Em7b5 Bb6/9 A7sus4 A7(b13)
#     (the original Dm E E Am Bm7b5 Bm7b5 E E Am, with the bar-2 A and bar-6
#     G5 re-set); the original's augmented C5-over-E colour is kept as the
#     F5 over A7(b13)
# New: the form (beatless intro on the chord loop, groove A, theme A, theme B,
# beatless breakdown with motif A in augmentation on a sine flute and glass,
# rebuild, peak B with a string-machine harmony, peak A with a third below,
# outro with Gm9 - A7sus4 and a final Dm9 hit that rings out and
# loops back into the intro), all chords beyond the fragments, the dusty
# 92 BPM breakbeat (round kick tuned to A1, round tuned A3 "pdlap" snare with
# 4 round-robins and ghost notes, swung hats, sampler grit), the warm round
# bass, detuned pads with tape drift, the FM Rhodes, the vinyl/hiss bed, hall
# reverb and dub delay.
#
#   python3 tools/music/edm/new18_ambient_breaks.py OUT.wav [bar0 bar1]
#   NEW18_MUTE=kick,snare,hats,perc,vinyl  (mute buses, for the chroma check)
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
import synth_triphop as TH  # noqa: E402
import synth_new18 as Q  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 92.0
SPB = 60.0 / BPM
NBARS = 72
TAIL = 4.0
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)
SWING = 0.07  # beats of delay on the off 16ths


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 3 <= bar < RB1


# ------------------------------------------------------------- harmony

LOOP = ['Dm9', 'Ebmaj9', 'F69', 'Gm9']                  # song 6's ascent, in D minor
THB = ['Gm9', 'Bbmaj9', 'A7sus4', 'Dm9', 'Em7b5', 'Bb69', 'A7sus4', 'A7b13']
BRK = ['Dm9', 'Dm9', 'Ebmaj9', 'Ebmaj9', 'F69', 'F69', 'Gm9', 'Gm9']
OUT = LOOP + ['Gm9', 'A7sus4', 'Dm9', 'Dm9']
CHORDS = (LOOP * 2 +       # 0-7    intro (beatless)
          LOOP * 2 +       # 8-15   groove A (motif A on the Rhodes)
          LOOP * 2 +       # 16-23  theme A (lead)
          THB +            # 24-31  theme B
          BRK +            # 32-39  breakdown (beatless)
          LOOP * 2 +       # 40-47  rebuild
          THB +            # 48-55  peak B
          LOOP * 2 +       # 56-63  peak A
          OUT)             # 64-71  outro, final Dm9 at 70
CHORDS = CHORDS[:NBARS]
assert len(CHORDS) == NBARS, len(CHORDS)

PAD = {
    'Dm9': ['F3', 'A3', 'C4', 'E4'],
    'Ebmaj9': ['G3', 'Bb3', 'D4', 'F4'],
    'F69': ['A3', 'C4', 'D4', 'G4'],
    'Gm9': ['Bb3', 'D4', 'F4', 'A4'],
    'Bbmaj9': ['A3', 'C4', 'D4', 'F4'],
    'Bb69': ['F3', 'C4', 'D4', 'G4'],
    'A7sus4': ['G3', 'D4', 'E4', 'A4'],
    'A7b13': ['G3', 'C#4', 'F4', 'A4'],
    'Em7b5': ['G3', 'Bb3', 'D4', 'E4'],
}
ROOT = {'Dm9': 'D2', 'Ebmaj9': 'Eb2', 'F69': 'F1', 'Gm9': 'G1', 'Bbmaj9': 'Bb1', 'Bb69': 'Bb1',
        'A7sus4': 'A1', 'A7b13': 'A1', 'Em7b5': 'E2'}
FIFTH = {'Dm9': 'A1', 'Ebmaj9': 'Bb1', 'F69': 'C2', 'Gm9': 'D2', 'Bbmaj9': 'F1', 'Bb69': 'F1',
         'A7sus4': 'E2', 'A7b13': 'E2', 'Em7b5': 'Bb1'}

# ------------------------------------------------------------- themes (beat, note, length)
# motif A: song 6's piccolo sigh (bars 81-84), in D minor, + a new answer
MOT_A1 = [(0, 'A4', 3), (3, 'F4', 1), (4, 'G4', 3), (7, 'Eb4', 1), (8, 'F4', 3), (11, 'D4', 1),
          (12, 'G4', 2), (14, 'Bb4', 2)]
MOT_A2 = [(0, 'A4', 3), (3, 'F4', 1), (4, 'G4', 3), (7, 'Eb4', 1), (8, 'F4', 3), (11, 'D4', 1),
          (12, 'Bb4', 1.5), (13.5, 'A4', 2.5)]
MOT_A = MOT_A1 + [(b + 16, n, d) for b, n, d in MOT_A2]
# the diatonic third below (peak A harmony)
HARM_A = [(0, 'F4', 3), (3, 'D4', 1), (4, 'Eb4', 3), (7, 'C4', 1), (8, 'D4', 3), (11, 'Bb3', 1),
          (12, 'D4', 2), (14, 'G4', 2),
          (16, 'F4', 3), (19, 'D4', 1), (20, 'Eb4', 3), (23, 'C4', 1), (24, 'D4', 3), (27, 'Bb3', 1),
          (28, 'G4', 1.5), (29.5, 'F4', 2.5)]
# theme B: song 3's choir counter-line (bars 33-41), up a fourth
THEME_B = [(0, 'G4', 2), (2, 'Bb4', 2), (4, 'A4', 4), (8, 'D5', 2), (10, 'E5', 2), (12, 'F5', 4),
           (16, 'G5', 4), (20, 'G5', 2), (22, 'F5', 2), (24, 'E5', 6), (30, 'F5', 2)]
# string-machine harmony for theme B (mostly a third/sixth below)
HARM_B = [(0, 'D4', 2), (2, 'F4', 2), (4, 'F4', 4), (8, 'A4', 2), (10, 'A4', 2), (12, 'C5', 4),
          (16, 'D5', 4), (20, 'D5', 2), (22, 'D5', 2), (24, 'C#5', 6), (30, 'C#5', 2)]

# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'hats', 'perc', 'sub', 'bass', 'pad', 'rhodes', 'lead', 'strings',
         'glass', 'flute', 'vinyl', 'fx']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def add(name, sig, t, gain=1.0):
    bus[name].add(sig, t - T0, gain)


rg = np.random.default_rng(18)


def hv(v, spread=0.08):
    return v * (1 + spread * (rg.random() * 2 - 1))


def jit(t, ms=4.0):
    return t + (rg.random() * 2 - 1) * ms / 1000


def sw(beat):
    """Swing the off 16ths."""
    frac = round((beat % 1) * 4) % 4
    return beat + (SWING if frac in (1, 3) else 0.0)


# ------------------------------------------------------------- drums

def kick_at(bar, beat, vel=1.0):
    if not live(bar):
        return
    t = jit(T(bar, sw(beat)), 3)
    v = int(rg.integers(0, 2))
    add('kick', cached(('k', v), lambda: Q.kick(1.0, var=v)), t, hv(vel, 0.05))
    kicks.append(t - T0)


_sn = [0]


def snare_at(bar, beat, vel=1.0, ghost=False):
    if not live(bar):
        return
    v = _sn[0] % 4
    _sn[0] += 1
    sig = cached(('s', v, ghost), lambda: Q.snare(1.0, v, ghost=ghost))
    add('snare', sig, jit(T(bar, sw(beat)), 4 if ghost else 2.5), hv(vel, 0.07))


def hats_bar(bar, vel=0.5, ghost=0.25, opn=0.0):
    if not live(bar):
        return
    for i in range(16):
        beat = i * 0.25
        if i % 2 == 0:
            acc = 1.0 if i % 4 == 2 else 0.72
            v = vel * acc
        else:
            if not ghost:
                continue
            v = ghost * (0.8 if i % 4 == 3 else 0.55)
        var = int(rg.integers(0, 3))
        add('hats', cached(('h', var), lambda: TH.hat(1.0, False, var, pan=0.2)),
            jit(T(bar, sw(beat)), 5), hv(v, 0.15))
    if opn:
        add('hats', cached('ho', lambda: TH.hat(1.0, True, 1, pan=0.1)), T(bar, sw(3.5)), opn)


# two-bar dusty break: (beat, vel) for kick; snare on 2 and 4 with ghosts
K1 = [(0, 1.0), (1.75, 0.55), (2.5, 0.9)]
K2 = [(0, 1.0), (0.75, 0.6), (2.25, 0.75), (2.75, 0.85)]
G1 = [(2.75, 0.17), (3.75, 0.13)]
G2 = [(1.5, 0.12), (3.25, 0.16), (3.75, 0.2)]


def brk(bar0, bar1, kick=1.0, snare=0.85, hat=0.45, ghost=0.22, opn=0.0, ghosts=True, fill_last=False):
    for b in range(bar0, bar1):
        two = (b - bar0) % 2
        for beat, v in (K2 if two else K1):
            kick_at(b, beat, kick * v)
        snare_at(b, 1, snare)
        snare_at(b, 3, snare * 0.96)
        if ghosts:
            for beat, v in (G2 if two else G1):
                snare_at(b, beat, snare * v * 1.6, ghost=True)
        hats_bar(b, hat, ghost, opn if two else 0.0)
    if fill_last and live(bar1 - 1):   # a soft rim/tom-ish pickup, no snare roll
        for beat, v in ((3.25, 0.5), (3.5, 0.6), (3.75, 0.7)):
            add('perc', cached('rim', lambda: TH.rim(1.0, 0.2)), T(bar1 - 1, sw(beat)), v)


def shaker(bar0, bar1, vel=0.3):
    for b in range(bar0, bar1):
        if not live(b):
            continue
        for i in range(8):
            acc = (0.6, 1.0)[i % 2]
            add('perc', cached(('sh', i % 2), lambda: TH.shaker(1.0, i % 2, pan=-0.35)),
                jit(T(b, i * 0.5 + 0.25 + SWING * 0.5), 6), hv(vel * acc, 0.15))


def swell(bar, beats=4, vel=1.0):
    if not live(bar):
        return
    sig = cached(('sw', beats), lambda: S.butter(S.reverse_cymbal(B(beats)), 4500, 'low', 2).astype(np.float32))
    add('fx', sig, T(bar) - B(beats), vel)


# ------------------------------------------------------------- tonal parts

def pads(bar0, bar1, vel=1.0, attack=0.8, release=1.6, cut=1300.0):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if live(b) or live(e - 1):
            dur = T(e) - T(b)
            for note in PAD[ch]:
                add('pad', Q.boc_pad(m(note), dur, 1.0, T(b), attack=attack, release=release, cut=cut),
                    T(b), vel * 0.45)
        b = e


def bass(bar0, bar1, vel=1.0, style='groove'):
    """Round warm bass: root on 1 (held), root again on the and-of-2, the fifth as a pickup."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        ch = CHORDS[b]
        r = m(ROOT[ch])
        f5 = m(FIFTH[ch])
        if style == 'hold':
            add('bass', X.warm_bass(r, B(3.8), 1.0, cut=180, env=150, saw_amt=0.25, a=0.03, r=0.12), T(b), vel)
            continue
        for beat, note, dur, v in ((0, r, 1.4, 1.0), (2.5, r, 0.9, 0.85), (3.5, f5, 0.45, 0.7)):
            sig = cached(('b', note, dur), lambda: X.warm_bass(note, B(dur), 1.0, cut=240, env=450, decay=0.1,
                                                              saw_amt=0.6, r=0.05))
            add('bass', sig, jit(T(b, sw(beat)), 3), hv(vel * v, 0.04))


def rhodes_comp(bar0, bar1, vel=0.6, sparse=False):
    """Lazy Rhodes chords: on 1 (held) and a lighter push on the and-of-2."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        ch = CHORDS[b]
        notes = [m(n) for n in PAD[ch]]
        hits = ((0, 2.3, 1.0),) if sparse else ((0, 2.3, 1.0), (2.5, 1.3, 0.7))
        for beat, dur, v in hits:
            for k, md in enumerate(notes):
                t = T(b, beat) + 0.012 * k          # a slight strum
                add('rhodes', cached(('rh', md, dur), lambda: TH.rhodes(md, B(dur), 0.55, 0.3,
                                                                       pan=-0.25 + 0.17 * k)),
                    t, hv(vel * v, 0.06))


def rhodes_line(bar0, notes, vel=0.8, transpose=0):
    for b, n, d in notes:
        bar = bar0 + int(b // 4)
        if not live(bar):
            continue
        md = m(n) + transpose
        add('rhodes', cached(('rl', md, d), lambda: TH.rhodes(md, B(d) - 0.05, 0.8, 0.35, pan=0.1)),
            jit(T(bar0, b), 6), hv(vel, 0.05))


def lead(bar0, notes, vel=1.0, transpose=0, **kw):
    end_bar = bar0 + int(max(b + d for b, _, d in notes) / 4)
    if not (live(bar0) or live(end_bar)):
        return
    t0 = T(bar0, notes[0][0])
    ev = [(T(bar0, b) - t0, m(n) + transpose, B(d) - 0.02) for b, n, d in notes]
    kw.setdefault('cut', 1100.0)
    kw.setdefault('cut_env', 600.0)
    kw.setdefault('vib_depth', 0.09)
    add('lead', X.warm_lead(ev, 1.0, t_abs=t0, glide=0.07, attack=0.06, release=0.6, **kw), t0, vel)


def strings(bar0, notes, vel=1.0, transpose=0):
    for b, n, d in notes:
        bar = bar0 + int(b // 4)
        if not live(bar):
            continue
        add('strings', X.string_machine(m(n) + transpose, B(d) - 0.03, 1.0, T(bar0, b), attack=0.5, release=1.0,
                                        cut=2200), T(bar0, b), vel)


def glass(bar0, notes, vel=1.0, transpose=0, decay=1.4):
    for b, n, d in notes:
        if not live(bar0 + int(b // 4)):
            continue
        md = m(n) + transpose
        add('glass', cached(('gl', md, decay), lambda: X.glass(md, B(.6), 1.0, decay=decay, bright=0.7)),
            T(bar0, b), vel)


def flute(bar0, notes, vel=1.0, transpose=0):
    for b, n, d in notes:
        if not live(bar0 + int(b // 4)):
            continue
        add('flute', Q.sine_flute(m(n) + transpose, B(d) - 0.05, 1.0), T(bar0, b), vel)


def glass_arp(bar0, bar1, vel=0.5):
    """Slow 8th-note glass arpeggio on the pad tones, 5-step cycle against the bar."""
    pat = [0, 2, 3, 1, 3]
    g = 0
    for b in range(bar0, bar1):
        if not live(b):
            g += 8
            continue
        tones = [m(n) + 12 for n in PAD[CHORDS[b]]]
        for i in range(8):
            md = tones[pat[g % 5]]
            g += 1
            if i % 4 == 3:
                continue
            add('glass', cached(('ga', md), lambda: X.glass(md, B(.4), 1.0, decay=0.8, bright=0.6,
                                                            pan=0.3 * np.sin(md))),
                T(b, sw(i * 0.5)), hv(vel * (1.0 if i % 2 == 0 else 0.75), 0.08))


def aug(notes, k=2.0):
    return [(b * k, n, d * k) for b, n, d in notes]


# ---------------------------------------------------------- arrangement

# continuous: vinyl crackle + hiss (stronger in the beatless parts)
vin = TH.vinyl(N, crackle=1.0, hiss=1.0, seed=1818)
lvl = np.interp(np.arange(N) / SR + T0,
                [0, T(8), T(9), T(32), T(33), T(40), T(64), T(70), T(72) + 4],
                [1.0, 1.0, 0.6, 0.6, 1.0, 0.7, 0.6, 0.9, 0.9]).astype(np.float32)
bus['vinyl'].x += vin * lvl[None, :]
del vin, lvl

# INTRO 0-7: beatless; the chord loop on pads + Rhodes, motif A's first phrase on the Rhodes
pads(0, 8, vel=0.85, attack=2.0, release=2.0, cut=900)
rhodes_comp(2, 8, vel=0.45, sparse=True)
rhodes_line(4, MOT_A1, vel=0.5)
bass(4, 8, vel=0.55, style='hold')
for b in (6, 7):
    hats_bar(b, 0.22 + 0.06 * (b - 6), 0.0)
swell(8, 4, 0.35)

# GROOVE A 8-15: the break comes in (filtered open on the bus), bass, motif A on the Rhodes
brk(8, 16, kick=0.95, snare=0.8, hat=0.42, ghost=0.18, ghosts=True, fill_last=True)
pads(8, 16, vel=0.7, attack=0.8, cut=1100)
bass(8, 16, vel=0.9)
rhodes_comp(8, 16, vel=0.42)
rhodes_line(8, MOT_A, vel=0.75)

# THEME A 16-23: the lead sings motif A, glass answers
brk(16, 24, kick=1.0, snare=0.85, hat=0.45, ghost=0.22, opn=0.18, fill_last=True)
pads(16, 24, vel=0.65, attack=0.6, cut=1300)
bass(16, 24, vel=1.0)
rhodes_comp(16, 24, vel=0.45)
lead(16, MOT_A, vel=1.0)
glass(16, [(b + 3, n, d) for b, n, d in MOT_A if d >= 3][:4], vel=0.3, transpose=12)
swell(24, 4, 0.35)

# THEME B 24-31: song 3's choir counter-line on the lead, strings below
brk(24, 32, kick=1.0, snare=0.85, hat=0.45, ghost=0.2, opn=0.15)
shaker(24, 32, 0.28)
pads(24, 32, vel=0.6, attack=0.5, cut=1400)
bass(24, 32, vel=1.0)
rhodes_comp(24, 32, vel=0.42)
lead(24, THEME_B, vel=0.95)
strings(24, HARM_B, vel=0.45, transpose=-12)
glass_arp(28, 32, vel=0.3)

# BREAKDOWN 32-39: beatless; motif A in augmentation on the sine flute, glass halo
pads(32, 40, vel=0.9, attack=1.6, release=2.2, cut=950)
bass(32, 40, vel=0.55, style='hold')
flute(32, aug(MOT_A1), vel=0.8)
glass(32, aug(MOT_A1), vel=0.22, transpose=12, decay=2.0)
rhodes_comp(32, 40, vel=0.3, sparse=True)
for b in (38, 39):
    hats_bar(b, 0.2 + 0.05 * (b - 38), 0.0)
swell(40, 8, 0.4)

# REBUILD 40-47: break back, motif A on the Rhodes and the lead an octave down
brk(40, 48, kick=0.95, snare=0.8, hat=0.42, ghost=0.2, fill_last=True)
pads(40, 48, vel=0.65, attack=0.6, cut=1200)
bass(40, 48, vel=0.95)
rhodes_comp(40, 48, vel=0.4)
rhodes_line(40, MOT_A, vel=0.7)
lead(44, MOT_A2, vel=0.75)
glass_arp(40, 48, vel=0.28)
swell(48, 4, 0.4)

# PEAK B 48-55: theme B with the string-machine harmony, shaker, open hats
brk(48, 56, kick=1.0, snare=0.88, hat=0.48, ghost=0.22, opn=0.2, fill_last=True)
shaker(48, 56, 0.32)
pads(48, 56, vel=0.62, attack=0.4, cut=1600)
bass(48, 56, vel=1.0)
rhodes_comp(48, 56, vel=0.45)
lead(48, THEME_B, vel=1.0)
strings(48, HARM_B, vel=0.6)
strings(48, THEME_B, vel=0.35, transpose=-12)
glass_arp(48, 56, vel=0.32)
swell(56, 4, 0.4)

# PEAK A 56-63: motif A with the third below, strings, glass
brk(56, 64, kick=1.0, snare=0.88, hat=0.5, ghost=0.22, opn=0.2, fill_last=True)
shaker(56, 64, 0.3)
pads(56, 64, vel=0.62, attack=0.4, cut=1700)
bass(56, 64, vel=1.0)
rhodes_comp(56, 64, vel=0.45)
lead(56, MOT_A, vel=1.0)
strings(56, HARM_A, vel=0.5)
glass(56, MOT_A, vel=0.18, transpose=12)
glass_arp(60, 64, vel=0.28)

# OUTRO 64-71: the beat thins, motif A's first phrase once more, Gm9 A7sus4, final Dm9 hit at 70
brk(64, 68, kick=0.95, snare=0.75, hat=0.38, ghost=0.15, ghosts=False)
for b in (68, 69):
    for beat, v in K1:
        kick_at(b, beat, 0.85 * v)
    hats_bar(b, 0.3, 0.0)
snare_at(68, 1, 0.65)
snare_at(68, 3, 0.62)
snare_at(69, 1, 0.6)
pads(64, 70, vel=0.7, attack=0.6, cut=1200)
bass(64, 70, vel=0.9)
rhodes_comp(64, 70, vel=0.4)
rhodes_line(64, MOT_A1, vel=0.6)
lead(64, MOT_A1, vel=0.75)
lead(68, [(0, 'Bb4', 2), (2, 'A4', 2), (4, 'G4', 2), (6, 'E4', 2)], vel=0.7)
# final hit (bar 70): kick, deep D, Dm9 on pad + Rhodes + glass, the lead lands on A4
kick_at(70, 0, 1.0)
if live(70):
    add('bass', X.warm_bass(m('D2'), B(5), 1.0, cut=180, env=250, saw_amt=0.35, a=0.01, r=0.4), T(70), 1.0)
    for note in PAD['Dm9'] + ['D3']:
        add('pad', Q.boc_pad(m(note), B(5.5), 1.0, T(70), attack=0.05, release=1.3, cut=1300), T(70), 0.4)
        add('rhodes', TH.rhodes(m(note) + 12 if note == 'D3' else m(note), B(4), 0.7, 0.6), T(70), 0.55)
    for note in ('D5', 'A5', 'E6'):
        add('glass', X.glass(m(note), B(3), 1.0, decay=1.6, bright=0.7), T(70), 0.28)
    add('lead', X.warm_lead([(0.0, m('A4'), B(4.5))], 1.0, t_abs=T(70), cut=1100, cut_env=500,
                            release=0.7), T(70), 0.75)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


kicks = [k for k in kicks if k >= 0]
duck_bass = S.sidechain(N, kicks, depth=0.45, release=0.25)
duck_pad = S.sidechain(N, kicks, depth=0.18, release=0.35)

GAIN = {  # dB
    'kick': -6.0, 'snare': 0.0, 'hats': 12.0, 'perc': 2.0, 'bass': -10.0, 'sub': -6.0,
    'pad': -2.0, 'rhodes': 3.0, 'lead': -4.0, 'strings': -2.0, 'glass': -4.0, 'flute': -2.0,
    'vinyl': 12.0, 'fx': -6.0,
}
DUCK = {'bass': duck_bass, 'pad': duck_pad, 'strings': duck_pad, 'rhodes': duck_pad}
VERB = {'snare': .14, 'hats': .05, 'perc': .2, 'pad': .3, 'rhodes': .22, 'lead': .3, 'strings': .4,
        'glass': .5, 'flute': .45, 'fx': .3}
DELAY = {'rhodes': .16, 'lead': .2, 'glass': .25, 'flute': .25, 'perc': .2}

tt_abs = None


def drums_filter(x):
    """Low-pass automation on the break: it opens over the first groove, and
    closes a little in the outro."""
    tt = np.arange(N) / SR + T0
    fc = np.interp(tt, [T(6), T(8), T(12), T(64), T(70), T(72)], [900, 1500, 9000, 9000, 4500, 4500])
    return S.svf(x, fc, 0.6).astype(np.float32)


FX = {
    'pad': lambda x: Q.seasick(stage(x, HighpassFilter(150), Chorus(rate_hz=0.2, depth=0.25, centre_delay_ms=9,
                                                                   mix=0.4)), depth_ms=3.5),
    'rhodes': lambda x: TH.autopan(stage(x, HighpassFilter(110), LowpassFilter(6000)), rate=2.6, depth=0.3),
    'strings': lambda x: stage(x, HighpassFilter(160), Chorus(rate_hz=0.5, depth=0.35, centre_delay_ms=7, mix=0.5),
                               LowpassFilter(6000)),
    'lead': lambda x: Q.seasick(stage(x, HighpassFilter(200), PeakFilter(2500, 1.0, 0.8), LowpassFilter(7000)),
                                depth_ms=1.6, rate=0.31),
    'glass': lambda x: stage(x, HighpassFilter(300), LowpassFilter(9000)),
    'flute': lambda x: Q.seasick(stage(x, HighpassFilter(200)), depth_ms=1.5, rate=0.27),
    'kick': lambda x: drums_filter(Q.dust(x, bits=12, hold=2, lp=7000, drive=1.3)),
    'snare': lambda x: drums_filter(Q.dust(stage(x, HighpassFilter(130)), bits=12, hold=2, lp=7000, drive=1.3)),
    'hats': lambda x: drums_filter(Q.dust(stage(x, HighpassFilter(5000)), bits=12, hold=2, lp=9500, drive=1.2)),
    'perc': lambda x: stage(x, HighpassFilter(200), LowpassFilter(8000)),
    'bass': lambda x: stage(x, HighpassFilter(32), LowpassFilter(1800)),
    'vinyl': lambda x: x,
    'fx': lambda x: stage(x, HighpassFilter(300)),
}

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
MUTE = set(filter(None, os.environ.get('NEW18_MUTE', '').split(',')))
STEMS = os.environ.get('NEW18_STEMS')   # directory: write per-bus stems (for balance checks)
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE or not np.any(x):
        continue
    if k in FX:
        x = FX[k](x)
    x = np.asarray(x, np.float32) * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= DUCK[k][None, :]
    levels[k] = meter.integrated_loudness(x.T.astype(np.float64))
    if STEMS:
        sf_.write(os.path.join(STEMS, k + '.wav'), x.T, SR, subtype='FLOAT')
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    if k in DELAY:
        delay_send += x * np.float32(DELAY[k])
    del x

verb = X.convolve(stage(verb_send, HighpassFilter(300), LowpassFilter(7000)), X.hall_ir(t60=(3.6, 4.0, 2.8, 1.4)))
del verb_send
dly = TH.dub_delay(delay_send, B(0.75), feedback=0.42, repeats=7, hp=380, lp=3200, drive=1.2)
del delay_send
mix += verb * np.float32(0.5) + dly * np.float32(0.45)
del verb, dly

# section dynamics (dB at bar lines)
SECTION_DB = [(0, -4.5), (7.9, -4.0), (8, -3.5), (15.9, -3.0), (16, -1.8), (23.9, -1.8), (24, -1.2),
              (31.9, -1.2), (32, -4.5), (39.9, -4.0), (40, -3.0), (47.9, -2.5), (48, 0.0), (55.9, 0.0),
              (56, 0.4), (63.9, 0.4), (64, -1.5), (69.9, -2.0), (70, -1.5), (73, -1.5)]
bars_axis = (np.arange(N) / SR + T0) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(110, -2.5, 0.7), PeakFilter(3000, 2.0, 0.7), HighShelfFilter(7000, -1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=120))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

# trim: the song ends at bar 72 (the final hit at bar 70 rings ~3 s), short fade
if RB1 == NBARS:
    end = int((T(70) + 4.2 - T0) * SR)
    mix = mix[:, :end]
    fade = int(1.2 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade, dtype=np.float32)[None, :] ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new18.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, gated):')
ref = levels.get('kick', 0.0)
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}   ({v - ref:+5.1f} vs kick)')
