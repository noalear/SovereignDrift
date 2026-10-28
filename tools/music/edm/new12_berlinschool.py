#!/usr/bin/env python3
# new12_berlinschool.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Solar Wind Archive": new track 12, Berlin-school sequencer music (108 BPM,
# B minor / Aeolian with Lydian and Dorian colours and one Phrygian turn to C),
# in the 1974-80 manner of Tangerine Dream (Phaedra, Rubycon, Stratosfear),
# Klaus Schulze and early Steve Roach.  Cruise tag: calm and wondrous.
# A NEW piece (not a remix), built from buried fragments of two of the game's
# songs.
#
# Credits: the source music is Stardock's (the original Stellar Frontier game
# music, sf-music4.mid and sf-music8.mid, Stardock Shared Source license).
# Composed, synthesized and mixed in code with tools/music/edm/synth.py,
# synth_new10.py and synth_new12.py: no samples, no SoundFonts.
#
# Built from (bar numbers are the original MIDIs', 1-based):
#   sf-music8.mid (B minor)
#   - the choir's rising line E4 | G4 B4 | E5 | D5 E5 F#5 (Choir Aahs, bars
#     53-56, buried under the harp descent and never featured by
#     song8_dubstep.py, song8_darkwave.py or new08_detroit.py).  Its seven
#     notes, as scale steps (0 2 4 7 6 7 8), are the shape of the 16-step
#     sequence (around a pedal on 0, 3, 6, 8, 11, 14: the song's 3+3+2 riff
#     grouping); over E minor the sequence plays the original notes.  The same
#     seven notes in 8ths form the second, high sequence: a 7-step loop against
#     the 8-step bar, so the two sequences phase against each other.
#   - the contrabass ascent F#4 A4 | B4 C#5 | D5 B4 | F#5 (Contrabass, bars
#     84-86, played once before the closing riff): the opening phrase of the
#     soft lead theme, over Bm | Bm | Gmaj7 | Gmaj7 (the F#5 is the major 7th).
#   - the strings' sway E5 | C#5 E5 | B4 | E5 C#5 | E5 B4 (String Ensemble 1,
#     bars 76-80, an inner voice of the ending's Em6 colour): the answering
#     phrase of the lead theme (over A) and the string-machine line of the
#     outro (over Em6 -> A).
#   sf-music4.mid (C minor, transposed down a semitone)
#   - the whistle line G3 | G#3 C4 | F#4 | G#4 ... A3 | C#4 G4 | A4 | G#4 | G4
#     (Whistle, here Flute in the enhanced MIDI; bars 52-61, heard only in
#     passing in song4_chiptune.py's dark middle, unused by new04_bigbeat.py):
#     the Mellotron-flute melody of the drift section, one original bar per new
#     bar, an octave up: F#4 | G4 | B4 F5 | G5 ... G#4 | C5 F#5 | G#5 | G5 | F#5.
#     Its semitone lift (C -> C# pedal at bar 57) becomes the Phrygian turn
#     B -> C, re-harmonized G/B, G7/B, Cmaj7#5, D/C, Cmaj7#5, Cmaj7, Bm.
# New: the form (drone intro, the sequence fading in, sequence + theme, a
# beatless drift with the Mellotron flute and the Phrygian turn, the peak with
# both sequences, a gentle kick pulse and the theme again, an outro that closes
# the filter and a final Bm9 hit that rings out and loops back to the intro),
# the harmony (Bm, Gmaj7 (Lydian), Em6 (Dorian), Aadd9), the Moog-style
# sequencer with a filter that opens over minutes, the 8th-note bass pulse,
# analog and tape-string pads with a phaser, the soft glide lead, the glass
# second sequence, tape echo, a long hall, a solar-wind noise bed and a soft
# kick tuned to B1 with a shaker (no snare at all).
#
#   python3 tools/music/edm/new12_berlinschool.py OUT.wav [bar0 bar1]
#   NEW12_MUTE=kick,perc  (mute buses, e.g. for the chroma check)
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
import synth_new12 as Y  # noqa: E402
import synth_new09 as Z  # noqa: E402  (phaser)
from synth import m, SR  # noqa: E402

BPM = 108.0
SPB = 60.0 / BPM
NBARS = 82
TAIL = 3.5
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 3 <= bar < RB1


# ------------------------------------------------------------- harmony

THEME = ['Bm', 'Bm', 'G', 'G', 'Em', 'Em', 'A', 'A'] * 2
CHORDS = (['Bm'] * 8 +                                   # 0-7    intro (drone)
          ['Bm'] * 8 + ['G'] * 4 + ['Em'] * 2 + ['A'] * 2 +   # 8-23   sequence
          THEME +                                        # 24-39  theme
          ['Bm', 'Bm', 'Bm', 'GB', 'G7B', 'GB', 'GB',    # 40-46  drift
           'Caug', 'DC', 'Caug', 'Cmaj7',                # 47-50  Phrygian turn
           'Bm', 'Bm', 'Bm', 'Bm', 'Bm'] +               # 51-55
          THEME +                                        # 56-71  peak
          ['Bm', 'Bm', 'G', 'G', 'Em', 'Em', 'A', 'A', 'Bm', 'Bm'])  # 72-81 outro, hit at 80
assert len(CHORDS) == NBARS

PAD = {
    'Bm': ['B3', 'D4', 'F#4', 'C#5'],
    'G': ['G3', 'B3', 'D4', 'F#4'],
    'Em': ['E3', 'G3', 'B3', 'C#4'],
    'A': ['A3', 'C#4', 'E4', 'B4'],
    'GB': ['B2', 'G3', 'B3', 'D4'],
    'G7B': ['B2', 'F3', 'G3', 'D4'],
    'Caug': ['C3', 'E3', 'G#3', 'B3'],
    'DC': ['C3', 'D3', 'F#3', 'A3'],
    'Cmaj7': ['C3', 'E3', 'G3', 'B3'],
}
ROOT = {'Bm': 'B1', 'G': 'G1', 'Em': 'E2', 'A': 'A1', 'GB': 'B1', 'G7B': 'B1',
        'Caug': 'C2', 'DC': 'C2', 'Cmaj7': 'C2'}
# 7-note scale per chord, ascending from the sequence's pedal note
_SC = {
    'Bm': ('B2', [0, 2, 3, 5, 7, 8, 10]),        # B Aeolian
    'G': ('G2', [0, 2, 4, 6, 7, 9, 11]),         # G Lydian (the C# of B minor)
    'Em': ('E2', [0, 2, 3, 5, 7, 9, 10]),        # E Dorian (the C# again)
    'A': ('A2', [0, 2, 4, 5, 7, 9, 10]),         # A Mixolydian
    'GB': ('B2', [0, 2, 3, 5, 7, 8, 10]),
    'G7B': ('B2', [0, 1, 3, 5, 6, 8, 10]),
    'Caug': ('C3', [0, 2, 4, 6, 8, 9, 11]),      # C Lydian augmented
    'DC': ('C3', [0, 2, 4, 6, 7, 9, 11]),        # C Lydian
    'Cmaj7': ('C3', [0, 2, 4, 6, 7, 9, 11]),
}


def deg(ch, d, octave=0):
    root, sc = _SC[ch]
    return m(root) + sc[d % 7] + 12 * (d // 7) + 12 * octave


# song 8's choir rise E G B E D E F# (bars 53-56) as scale steps
RISE = [0, 2, 4, 7, 6, 7, 8]
P = None   # the pedal
SEQ16 = [P, 2, 4, P, 7, 6, P, 7, P, 8, 7, P, 4, 2, P, 4]
ACC16 = [1.0, .62, .7, .9, .78, .66, .92, .7, .97, .74, .66, .9, .7, .62, .9, .68]

# ------------------------------------------------------------- themes (beats; note; length)

# contrabass ascent (song 8, bars 84-86) -> theme phrase 1
PH1 = [(2, 'F#4', 1), (3, 'A4', 1), (4, 'B4', 2), (6, 'C#5', 2), (8, 'D5', 2), (10, 'B4', 2), (12, 'F#5', 4)]
# the strings' sway E5 C#5 E5 B4 (song 8, bars 76-80) closes phrase 2
PH2 = [(0, 'E5', 2), (2, 'D5', 1), (3, 'E5', 1), (4, 'G5', 2), (6, 'F#5', 2),
       (8, 'E5', 2), (10, 'C#5', 2), (12, 'E5', 2), (14, 'B4', 2)]
PH4 = PH2[:-2] + [(12, 'B4', 4)]
# song 4's whistle line (bars 52-61), down a semitone and up an octave
FLUTE = [(2, 'F#4', 2), (4, 'G4', 4), (8, 'B4', 2), (10, 'F5', 2), (12, 'G5', 8),
         (20, 'G#4', 4), (24, 'C5', 2), (26, 'F#5', 2), (28, 'G#5', 4), (32, 'G5', 4), (36, 'F#5', 10)]
SWAY = [(0, 'E5', 2), (2, 'C#5', 2), (4, 'E5', 2), (6, 'B4', 2), (8, 'E5', 2), (10, 'C#5', 2), (12, 'E5', 2),
        (14, 'B4', 2)]

# --------------------------------------------------------------- buses

names = ['kick', 'perc', 'sub', 'bass', 'seq', 'seq2', 'pad', 'strings', 'flute', 'lead', 'bells', 'wind']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def add(name, sig, t, gain=1.0):
    bus[name].add(sig, t - T0, gain)


rg = np.random.default_rng(12)


def hv(v, spread=0.06):
    return v * (1 + spread * (rg.random() * 2 - 1))


def keyframes(points):
    xs = [p[0] for p in points]
    ys = [np.log(p[1]) for p in points]
    return lambda bar: float(np.exp(np.interp(bar, xs, ys)))


# the sequencer's filter knob over the whole piece (bar -> base cutoff Hz)
SEQ_CUT = keyframes([(4, 140), (8, 280), (16, 650), (24, 1200), (32, 1800), (40, 1500), (43, 520),
                     (47, 700), (51, 450), (56, 1200), (64, 2300), (70, 3200), (72, 2000), (76, 800),
                     (80, 330)])
SEQ_ENV = keyframes([(4, 1.0), (24, 1.6), (40, 1.8), (44, 2.4), (56, 1.6), (70, 2.0), (80, 1.2)])

# ------------------------------------------------------------- parts


def seq(bar0, bar1, vel=1.0, mode='full', fade_in=0):
    """The 16-step sequence (16ths), shaped by song 8's choir rise around a pedal."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        ch = CHORDS[b]
        for i in range(16):
            st = SEQ16[i]
            if mode == 'pedal' and st not in (P, 7):
                continue
            if mode == 'sparse' and st not in (P, 7, 4) and i % 4 != 1:
                continue
            md = deg(ch, 0) if st is P else deg(ch, st)
            pos = b + i / 16
            g = vel * ACC16[i]
            if fade_in:
                g *= min(1.0, 0.25 + 0.75 * (pos - bar0) / fade_in)
            cut = SEQ_CUT(pos) * (1.15 if st is P else 1.0)
            sig = Y.seq_note(md, B(.2), 1.0, cut=cut, env=SEQ_ENV(pos), decay=0.1, q=2.4)
            add('seq', sig[None, :].repeat(2, 0), T(b, i * .25), hv(g, 0.05))


def seq2(bar0, bar1, vel=1.0):
    """High glass sequence: the choir rise's 7 notes in 8ths, phasing against the 8-step bar."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        ch = CHORDS[b]
        for i in range(8):
            g = b * 8 + i
            st = RISE[g % 7]
            md = deg(ch, st, octave=2)
            if md > m('D6'):
                md -= 12
            add('seq2', cached(('g2', md), lambda: X.glass(md, B(.4), 1.0, decay=0.55, bright=0.7)),
                T(b, i * .5), hv(vel * (1.0 if g % 7 == 0 else 0.72), 0.08))


def bass_pulse(bar0, bar1, vel=1.0, pat=(0, 0, 12, 0, 0, 0, 12, 7)):
    """Moog bass in 8ths on the chord root (octave and fifth on the off-steps)."""
    for b in range(bar0, bar1):
        if not live(b):
            continue
        r = m(ROOT[CHORDS[b]])
        for i, iv in enumerate(pat):
            md = r + iv
            sig = cached(('bp', md), lambda: Y.seq_note(md, B(.42), 1.0, cut=170, env=2.2, decay=0.07,
                                                        q=1.3, pulse_amt=0.2, sub=0.0, drive=1.6))
            add('bass', sig[None, :].repeat(2, 0), T(b, i * .5), hv(vel * (1.0 if i % 2 == 0 else 0.78), 0.04))


def sub_drone(bar0, bar1, vel=1.0):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and ROOT[CHORDS[e]] == ROOT[ch]:
            e += 1
        if live(b) or live(e - 1):
            add('sub', X.warm_bass(m(ROOT[ch]), T(e) - T(b) - 0.05, 1.0, cut=140, env=60, saw_amt=0.15,
                                   a=0.25, r=0.25), T(b), vel)
        b = e


def pads(bar0, bar1, vel=1.0, attack=1.0, release=1.6, cut=1300.0):
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
                                        voices=3), T(b), vel * 0.5)
        b = e


def tstrings(bar0, bar1, vel=1.0, top_only=False, attack=0.8):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if live(b) or live(e - 1):
            notes = PAD[ch][-2:] if top_only else PAD[ch]
            for note in notes:
                add('strings', Y.tape_strings(m(note) + 12, T(e) - T(b), 1.0, T(b), attack=attack),
                    T(b), vel * 0.4)
        b = e


def lead(bar0, notes, vel=1.0, transpose=0, **kw):
    t0 = T(bar0, notes[0][0])
    if not (live(bar0) or live(bar0 + int(max(b + d for b, _, d in notes) / 4))):
        return
    ev = [(T(bar0, b) - t0, m(n) + transpose, B(d) - 0.01) for b, n, d in notes]
    # touching notes glide: shorten nothing, keep them contiguous
    ev = [(t, md, d + 0.01) for t, md, d in ev]
    add('lead', X.warm_lead(ev, 1.0, t_abs=t0, glide=0.09, cut=1100.0, cut_env=500.0, vib_depth=0.1,
                            attack=0.07, release=0.6, **kw), t0, vel)


def flute(bar0, notes, vel=1.0, transpose=0):
    for b, n, d in notes:
        bar = bar0 + int(b // 4)
        if not live(bar):
            continue
        add('flute', Y.mello_flute(m(n) + transpose, B(d) - 0.05, 1.0, T(bar0, b), pan=-0.15),
            T(bar0, b), vel)


def string_line(bar0, notes, vel=1.0, transpose=0):
    for b, n, d in notes:
        if not live(bar0 + int(b // 4)):
            continue
        add('strings', Y.tape_strings(m(n) + transpose, B(d) - 0.04, 1.0, T(bar0, b), attack=0.3,
                                      release=0.9, cut=2400), T(bar0, b), vel)


def bells(bar0, notes, vel=1.0, transpose=0, decay=1.4):
    for b, n, d in notes:
        if not live(bar0 + int(b // 4)):
            continue
        md = m(n) + transpose
        add('bells', cached(('bl', md, decay), lambda: X.glass(md, B(.6), 1.0, decay=decay, bright=0.7)),
            T(bar0, b), vel)


def kick_at(bar, beat, vel=1.0):
    if not live(bar):
        return
    t = T(bar, beat)
    add('kick', cached('kick', lambda: X.round_kick(1.0, f_end=61.74, f_start=118.0, length=0.45)), t, vel)
    kicks.append(t - T0)


def pulse_beat(bar0, bar1, kick=1.0, quarters=True, shaker=0.0, hat=0.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            if quarters or beat in (0, 2):
                kick_at(b, beat, kick * (1.0 if beat in (0, 2) else 0.85))
        if not live(b):
            continue
        if shaker:
            for s in range(16):
                acc = (.55, .3, .75, .35)[s % 4]
                add('perc', cached(('sh', s % 2), lambda: X.shaker(1.0, 0.3 if s % 2 else -0.2)),
                    T(b, s * .25 + (0.02 if s % 2 else 0)), hv(shaker * acc, 0.15))
        if hat:
            for beat in range(4):
                add('perc', cached('hat', lambda: X.soft_hat(1.0, False, 0.15)), T(b, beat + .5), hv(hat, 0.08))


def swell(bar, beats=4, vel=1.0):
    if not live(bar):
        return
    sig = cached(('sw', beats), lambda: S.butter(S.reverse_cymbal(B(beats)), 4500, 'low', 2).astype(np.float32))
    add('wind', sig, T(bar) - B(beats), vel)


# ---------------------------------------------------------- arrangement

# continuous: the solar-wind bed (louder in the beatless parts)
if True:
    wind = X.wind(N, f_lo=300.0, f_hi=1800.0, rate=0.027, seed=12)
    lvl = np.interp(np.arange(N) / SR + T0,
                    [0, T(8), T(24), T(40), T(44), T(56), T(72), T(80), T(82) + 4],
                    [1.0, 0.7, 0.45, 0.6, 0.95, 0.45, 0.6, 0.8, 0.4]).astype(np.float32)
    bus['wind'].x += wind * lvl[None, :]
    del wind, lvl

# INTRO 0-7: the B drone, the pad swells in, the sequence fades in with a closed filter
pads(0, 8, vel=0.85, attack=3.5, release=2.0, cut=900)
sub_drone(2, 8, 0.6)
tstrings(4, 8, vel=0.45, top_only=True, attack=2.5)
seq(4, 8, vel=0.8, mode='pedal', fade_in=4)
bells(6, [(0, 'B4', 4), (4, 'F#5', 4)], vel=0.35, decay=2.2)

# SEQUENCE 8-23: the full pattern, the filter starts to open, the bass pulse joins
pads(8, 24, vel=0.75, attack=1.5, cut=1200)
sub_drone(8, 24, 0.75)
seq(8, 24, vel=1.0)
bass_pulse(12, 24, vel=0.7)
tstrings(16, 24, vel=0.5, attack=1.2)
swell(24, 4, 0.5)

# THEME 24-39: the soft lead (song 8's contrabass ascent + the strings' sway)
pads(24, 40, vel=0.7, attack=0.8, cut=1400)
sub_drone(24, 40, 0.8)
seq(24, 40, vel=1.0)
bass_pulse(24, 40, vel=0.8)
lead(24, PH1)
lead(28, PH2)
lead(32, PH1)
lead(36, PH4)
pulse_beat(28, 40, kick=0.55, quarters=False)
seq2(32, 40, vel=0.35)

# DRIFT 40-55: beatless; the Mellotron flute plays song 4's whistle line; the B -> C turn
pads(40, 56, vel=0.8, attack=2.0, release=2.0, cut=1000)
tstrings(40, 56, vel=0.55, attack=2.0)
sub_drone(40, 56, 0.65)
seq(40, 43, vel=0.85, mode='sparse')
seq(43, 47, vel=0.7, mode='pedal')
seq(47, 51, vel=0.75, mode='sparse')
seq(51, 56, vel=0.85, mode='sparse')
flute(42, FLUTE, vel=1.0)
bells(53, [(0, 'B4', 4), (4, 'C#5', 4), (8, 'D5', 4), (12, 'F#5', 4)], vel=0.3, decay=2.0)
swell(56, 8, 0.6)

# PEAK 56-71: both sequences, the gentle pulse, the theme with string harmony
pads(56, 72, vel=0.7, attack=0.6, cut=1600)
tstrings(56, 72, vel=0.45, top_only=True, attack=0.9)
sub_drone(56, 72, 0.85)
seq(56, 72, vel=1.0)
seq2(56, 72, vel=0.5)
bass_pulse(56, 72, vel=0.9)
pulse_beat(56, 72, kick=0.7, quarters=True, shaker=0.35, hat=0.22)
lead(56, PH1)
lead(60, PH2)
lead(64, PH1)
lead(68, PH4)
flute(56, [(b, n, d) for b, n, d in PH1], vel=0.35, transpose=-12)
flute(64, [(b, n, d) for b, n, d in PH1], vel=0.45)
string_line(60, SWAY, vel=0.3, transpose=-12)

# OUTRO 72-81: the filter closes, the lead once more, the sway on strings, final Bm9 at 80
pads(72, 80, vel=0.75, attack=1.0, cut=1300)
sub_drone(72, 80, 0.8)
seq(72, 80, vel=0.95)
seq2(72, 76, vel=0.3)
bass_pulse(72, 78, vel=0.65)
pulse_beat(72, 76, kick=0.55, quarters=False)
lead(72, PH1, vel=0.85)
string_line(76, SWAY, vel=0.55, transpose=-12)
# final hit (bar 80): kick, deep B, Bm9 on pads + strings + bells, held two bars
kick_at(80, 0, 0.9)
if live(80):
    add('seq', Y.seq_note(m('B2'), B(1.5), 1.0, cut=500, env=2.5, decay=0.25, q=2.0, r=0.5)[None, :].repeat(2, 0),
        T(80), 1.0)
    add('sub', X.warm_bass(m('B1'), B(6), 1.0, cut=150, env=200, saw_amt=0.3, a=0.01, r=0.4), T(80), 1.0)
    for note in ['B2', 'F#3', 'D4', 'A4', 'C#5']:
        add('pad', X.analog_pad(m(note), B(6.5), 1.0, T(80), attack=0.05, release=1.3, cut=1700, voices=3),
            T(80), 0.45)
        add('strings', Y.tape_strings(m(note) + 12, B(6), 1.0, T(80), attack=0.1, release=1.0), T(80), 0.3)
    for note in ('B4', 'F#5', 'C#6'):
        add('bells', X.glass(m(note), B(4), 1.0, decay=1.6, bright=0.8), T(80), 0.35)

# ------------------------------------------------------------- mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_bass = S.sidechain(N, kicks, depth=0.4, release=0.25)
duck_soft = S.sidechain(N, kicks, depth=0.15, release=0.3)

GAIN = {  # dB
    'kick': -9.0, 'perc': 0.0, 'sub': -8.0, 'bass': -9.0, 'seq': -8.5, 'seq2': -2.0,
    'pad': 1.0, 'strings': 1.0, 'flute': -2.0, 'lead': -1.0, 'bells': -5.0, 'wind': -13.0,
}
DUCK = {'sub': duck_bass, 'bass': duck_bass, 'pad': duck_soft, 'seq': duck_soft}
VERB = {'kick': .05, 'perc': .15, 'seq': .22, 'seq2': .45, 'pad': .35, 'strings': .4, 'flute': .4,
        'lead': .35, 'bells': .55, 'wind': .3, 'bass': .04}
DELAY = {'seq': .28, 'seq2': .3, 'lead': .22, 'flute': .25, 'bells': .3}

FX = {
    'pad': lambda x: stage(x, HighpassFilter(180), Chorus(rate_hz=0.2, depth=0.25, centre_delay_ms=9, mix=0.4)),
    'strings': lambda x: stage(Z.phaser(stage(x, HighpassFilter(170)), rate=0.07, fmin=250, fmax=2800, mix=0.4),
                               Chorus(rate_hz=0.55, depth=0.3, centre_delay_ms=7, mix=0.45), LowpassFilter(6500)),
    'lead': lambda x: stage(x, HighpassFilter(180), PeakFilter(2600, 0.5, 0.8)),
    'flute': lambda x: stage(x, HighpassFilter(220)),
    'seq': lambda x: stage(x, HighpassFilter(70)),
    'seq2': lambda x: stage(x, HighpassFilter(400), LowpassFilter(9000)),
    'bells': lambda x: stage(x, HighpassFilter(300), LowpassFilter(10000)),
    'perc': lambda x: stage(x, HighpassFilter(3000), LowpassFilter(10000)),
    'bass': lambda x: stage(x, HighpassFilter(38), LowpassFilter(1800)),
    'sub': lambda x: stage(x, LowpassFilter(180)),
    'wind': lambda x: stage(x, HighpassFilter(200)),
}


def evolve(k, x):
    """Sequence auto-pan (slow, TD style); pads breathe over 16 bars."""
    tt = np.arange(N) / SR + T0
    if k == 'seq':
        p = 0.3 * np.sin(2 * np.pi * tt / T(8))
        x = np.stack([x[0] * np.sqrt((1 - p)), x[1] * np.sqrt((1 + p))])
    elif k == 'pad':
        x = S.svf(x, 2400 * 2 ** (0.6 * np.sin(2 * np.pi * tt / T(16) - 1.0)), 0.7)
    return np.asarray(x, np.float32)


mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
MUTE = set(filter(None, os.environ.get('NEW12_MUTE', '').split(',')))
STEMS = os.environ.get('NEW12_STEMS')
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
    if STEMS:
        sf_.write(os.path.join(STEMS, f'{k}.wav'), x.T, SR, subtype='FLOAT')
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    if k in DELAY:
        delay_send += x * np.float32(DELAY[k])
    del x

verb = X.convolve(stage(verb_send, HighpassFilter(250), LowpassFilter(7500)), X.hall_ir(t60=(5.5, 5.8, 4.0, 2.0)))
del verb_send
dly = Y.tape_echo(delay_send, B(0.75), feedback=0.42, repeats=7, lp=4000, hp=350)
del delay_send
mix += verb * np.float32(0.6) + dly * np.float32(0.5)
del verb, dly

# section dynamics (dB at bar lines)
SECTION_DB = [(0, -6.0), (7.9, -5.0), (8, -4.5), (23.9, -3.0), (24, -2.0), (39.9, -1.5), (40, -6.5),
              (55.9, -5.5), (56, 0.0), (71.9, 0.5), (72, -1.0), (79.9, -2.0), (80, -1.0), (83, -1.0)]
bars_axis = (np.arange(N) / SR + T0) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(110, 0.3, 0.7), PeakFilter(320, -2.5, 0.8), HighShelfFilter(5000, 2.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=120))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

if RB1 == NBARS:
    end = int((T(80) + 3.9 - T0) * SR)
    mix = mix[:, :end]
    fade = int(1.2 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade, dtype=np.float32)[None, :]

out = sys.argv[1] if len(sys.argv) > 1 else 'new12.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, gated):')
ref = levels.get('pad', 0.0)
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}   ({v - ref:+5.1f} vs pad)')
