#!/usr/bin/env python3
# song8_darkwave.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Song 8, Darkwave": an alternative (darkwave / cold-wave) remix of sf-music8
# (the original game's music by Stardock, from mid/sf-music8.mid), produced
# entirely in code with tools/music/edm/synth.py and synth_darkwave.py: drum
# machine, sequencer bass, string machine, cold pads, a static-formant vox pad,
# a detuned pulse lead and a Karplus-Strong chorused "clean guitar".  No
# samples, no SoundFonts.  (The dubstep version is song8_dubstep.py; the
# thematic analysis below follows its header.)
#
# Taken from the original (B minor, 200 BPM; bar numbers are the MIDI's):
#   - the intro (bars 1-4): the chromatic bass rise B C D-E F#-A under the
#     F#3 taiko 8ths (here a palm-muted guitar F#3 8th pedal that runs through
#     the whole song, as in the original), the F#4 square-lead rhythm, the
#     tremolo F#6 (a 16th-synced string-machine tremolo), the choir line
#     B3 C4 D4-E4 F#4-E4 and the orchestra-hit F# 8ths;
#   - the riff (bars 5-12, 37-52, 89-92): bass B1 F#2 B2 | C3 G2 C2 in 3+3+2
#     (the sequencer bass's 8ths keep the grouping, and the kick follows it),
#     the strings F#-A-F# | E-D | F#-A-F# | G over D, the choir B C B A-G and
#     the orchestra hits B | C F# A (short dark stabs);
#   - the main melody (bars 13-20) F#5 B4 F#5 G5 C6 ... F#5 G5 E5 D5 (D5 E5);
#   - the tension section (bars 21-28): F / E / F# / F with the tritone 8ths
#     F5/B4, E5/A#4, F#5/C5, the syncopated bass of bars 23 and 27, the high
#     B5 D6 C6 D#6 sustains;
#   - the descending harp lines and the doubled F#3-A3-G3-F#3 line
#     (bars 45-56), the Em descent (53-56), the F#5 G5 E5 sequence and its
#     F#7b9 (57-60);
#   - the chant (bars 61-68) B4 B4 F#4 F#4 B4 C#5 B4 A#4 over
#     Bm | F#m/A | G/B | F#/A# and its counter line (69-76);
#   - the Em6 and F#7 ending (77-88) with the bass run F#3 G3 F#3 E3 ... C#3,
#     and the closing riff (89-92).
# New: 124 BPM (one original bar = one bar), a darkwave form (intro, riff,
# melody, tension, peak, switch, breakdown, peak, finale, outro), a dry
# drum-machine kit (kick tuned to B1, a round tonal snare on a short plate,
# a cold 16th hat, claps only on a few accents, DMX tom fills), a muted
# 8th/16th sequencer bass, cold chorused pads and a big hall on the pads,
# a dotted-8th delay on the lead and guitar, and a B-minor final hit with a
# short ring-out (no fade, no roll).
#
#   python3 tools/music/edm/song8_darkwave.py OUT.wav
#   env SONG8DW_MUTE=kick,snare,...  mutes buses (for chroma checks)
#   env SONG8DW_BARS=a,b             renders only bars a..b-1 (while iterating)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter,
                        HighpassFilter, LowpassFilter, LowShelfFilter,
                        HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_darkwave as W  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 124.0
SPB = 60.0 / BPM
NBARS = 101
TAIL = 6.0
_bars = os.environ.get('SONG8DW_BARS')
BAR0, BAR1 = (int(v) for v in _bars.split(',')) if _bars else (0, NBARS)
OFF = (BAR0 * 4) * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)
TREM_HZ = BPM / 60.0 * 4          # 16ths


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(t, length=8.0):
    return t + length > OFF and t < OFF + N / SR


rng = np.random.default_rng(88)


def hv(v, amt=0.06):
    return float(np.clip(v * (1 + amt * (rng.random() * 2 - 1)), 0.03, 1.2))


# ------------------------------------------------------------- harmony
PCS = {
    'B5': 'B F#', 'Cx': 'C F#', 'DE': 'D E F#', 'F#A': 'F# A E',
    'Bm': 'B D F#', 'C': 'C E G', 'Bdim/F': 'F B D', 'E7b5': 'E A# D', 'F#7b5': 'F# C E',
    'F7': 'F B D#', 'Em': 'E G B', 'F#7b9': 'F# A# G', 'F#m/A': 'A C# F#', 'G/B': 'G B D',
    'F#/A#': 'F# A# C#', 'Em6': 'E G C#', 'F#7': 'F# A# E', 'SEQ': 'F# G E',
}
VOICE = {   # cold-pad voicings (low and close)
    'Bm': ['B2', 'F#3', 'B3', 'D4'], 'C': ['C3', 'G3', 'C4', 'E4'],
    'F#m/A': ['A2', 'F#3', 'A3', 'C#4'], 'G/B': ['B2', 'G3', 'B3', 'D4'],
    'F#/A#': ['A#2', 'F#3', 'A#3', 'C#4'], 'Em6': ['E3', 'B3', 'C#4', 'G4'],
    'F#7b9': ['F#3', 'A#3', 'E4', 'G4'], 'F#7': ['F#3', 'A#3', 'C#4', 'E4'], 'Em': ['E3', 'B3', 'E4', 'G4'],
    'SEQ': ['E3', 'B3', 'E4', 'G4'], 'Bdim/F': ['F3', 'B3', 'D4'], 'E7b5': ['E3', 'A#3', 'D4'],
    'F#7b5': ['F#3', 'C4', 'E4'], 'F7': ['F3', 'B3', 'D#4'],
}

RIFF_CH = ['Bm', 'C'] * 2
TENS_CH = ['Bdim/F', 'E7b5', 'Bdim/F', 'E7b5', 'F#7b5', 'F7', 'F#7b5', 'F7']
CHANT_CH = ['Bm', 'F#m/A', 'G/B', 'F#/A#'] * 2
CHORDS = (['B5', 'Cx', 'DE', 'F#A'] * 2 +          # 0-7 intro (orig 1-4 twice)
          RIFF_CH * 2 +                              # 8-15 riff (orig 5-12)
          RIFF_CH * 2 +                              # 16-23 melody (orig 13-20)
          TENS_CH +                                  # 24-31 tension (orig 21-28)
          RIFF_CH * 4 +                              # 32-47 peak 1 (orig 37-52)
          ['Em'] * 4 + ['SEQ'] * 3 + ['F#7b9'] +     # 48-55 switch (orig 53-60)
          CHANT_CH * 2 +                             # 56-71 breakdown (orig 61-76)
          RIFF_CH * 2 + CHANT_CH +                   # 72-87 peak 2 (orig 13-20, 61-68)
          ['Em6'] * 4 + ['F#7'] * 4 +                # 88-95 finale (orig 77-80, 85-88)
          RIFF_CH + ['Bm'])                          # 96-100 outro + final hit (orig 89-92)
assert len(CHORDS) == NBARS, len(CHORDS)


def h332(a, b, c):
    return [(0, 1.5, a), (1.5, 1.5, b), (3, 1.0, c)]


RIFF_BASS = [h332('B1', 'F#2', 'B2'), h332('C3', 'G2', 'C2')]
CHANT_BASS = [h332('B1', 'D2', 'F#2'), [(0, 4, 'A2')], h332('B1', 'D2', 'F#2'), [(0, 4, 'A#2')]]
EM_BASS = h332('E2', 'B1', 'D2')
SEQ_BASS = h332('F#2', 'G2', 'E2')
F7_BASS = h332('F#2', 'C#3', 'E3')
INTRO_BASS = [[(0, 4, 'B1')], [(0, 4, 'C2')], [(0, 2, 'D2'), (2, 2, 'E2')], [(0, 2, 'F#2'), (2, 2, 'A2')]]
TENS_BASS = [h332('F2', 'F2', 'F2'), h332('E2', 'E2', 'E2'),
             [(0, 1, 'F2'), (1, .5, 'B2'), (1.5, 1, 'F3'), (2.5, 1, 'B2'), (3.5, .5, 'B1')],
             [(0, 1.5, 'E2'), (1.5, 2.5, 'E2')],
             h332('F#2', 'F#2', 'F#2'), h332('F2', 'F2', 'F2'),
             [(0, 1, 'F#2'), (1, .5, 'C#3'), (1.5, 1, 'F#3'), (2.5, 1, 'C#3'), (3.5, .5, 'C#2')],
             h332('F2', 'F2', 'F2')]

# ---------------------------------------------------------------- themes
# (beat, note, length), beats from the start of the section
MEL = [(0, 'F#5', 1), (1, 'B4', .5), (1.5, 'F#5', 2.5), (4, 'G5', 2), (6, 'C6', 2),
       (8, 'F#5', 1), (9, 'B4', .5), (9.5, 'F#5', 2.5), (12, 'G5', 4),
       (16, 'F#5', 4), (20, 'G5', 2), (22, 'E5', 2), (24, 'D5', 6), (30, 'D5', 1), (31, 'E5', 1)]
MEL_HARM = [(30, 'G4', 1), (31, 'A4', 1)]
DESC_A = ['F#5', 'E5', 'D5', 'C#5', 'D5', 'C#5', 'B4', 'A4']
DESC_B = ['B4', 'A4', 'G4', 'F#4', 'G4', 'F#4', 'E4', 'D4']
DESC = ([(i * .5, n, .5) for i, n in enumerate(DESC_A)] + [(4 + i * .5, n, .5) for i, n in enumerate(DESC_B)] +
        [(8, 'F#4', 8)] +
        [(16 + i * .5, n, .5) for i, n in enumerate(DESC_A)] + [(20 + i * .5, n, .5) for i, n in enumerate(DESC_B)] +
        [(24, 'F#4', 7), (31, 'G5', .5), (31.5, 'A5', .5)])
DOUBLE_LINE = [(9, 'F#3', 1), (10, 'F#3', 1), (11, 'A3', 1), (12, 'G3', 4), (16, 'F#3', 4), (20, 'G3', 4), (24, 'F#3', 8)]
EM_DESC = ([(i * .5, n, .5) for i, n in enumerate(['B5', 'A5', 'G5', 'F#5', 'G5', 'F#5', 'E5', 'D5'])] +
           [(4 + i * .5, n, .5) for i, n in enumerate(['E5', 'D5', 'C#5', 'B4', 'A4', 'G4', 'F#4', 'E4'])] +
           [(8, 'G4', 4)])
EM_BRASS = [(8, 'E5', 4), (12, 'D5', 2), (14, 'E5', 2)]
SEQ_MEL = [(b * 4 + x, n, l) for b in range(3) for x, n, l in [(0, 'F#5', 1.5), (1.5, 'G5', 1.5), (3, 'E5', 1)]]
SEQ_MEL += [(12, 'F#5', 1.5), (13.5, 'G5', 1.5), (15, 'A#5', 1)]
CHANT = [(0, 'B4', 3), (3, 'B4', 1), (4, 'F#4', 3), (7, 'F#4', 1), (8, 'B4', 1.5), (9.5, 'C#5', 1.5), (11, 'B4', 1),
         (12, 'A#4', 4),
         (16, 'B4', 3), (19, 'B4', 1), (20, 'F#4', 3), (23, 'F#4', 1), (24, 'B4', 1.5), (25.5, 'C#5', 1.5),
         (27, 'B4', 1), (28, 'C#5', 4)]
CHANT_DYADS = [['D5', 'B5'], ['C#5', 'F#5'], ['D5', 'B5'], ['C#5', 'C#6']] * 2
CH_LOW = ['B3', 'F#4', 'G4', 'F#4']
COUNTER = [(0, 'F#5', 4), (4, 'F#5', 2), (6, 'E5', 2), (8, 'D5', 2), (10, 'F#5', 2), (12, 'F#5', 4),
           (16, 'F#5', 4), (20, 'F#5', 2), (22, 'E5', 2), (24, 'D5', 2), (26, 'G5', 2), (28, 'F#5', 3)]
COUNTER_LO = [(0, 'D5', 4), (4, 'C#5', 4), (8, 'B4', 4), (12, 'C#5', 4)]
COUNTER_LO = COUNTER_LO + [(b + 16, n, l) for b, n, l in COUNTER_LO]
EM6_MEL = [(0, 'E5', 2), (2, 'C#5', 2), (4, 'E5', 2), (6, 'B4', 2)]
EM6_MEL = EM6_MEL + [(b + 8, n, l) for b, n, l in EM6_MEL]
EM6_RISE = [(0, 'B3', 6), (6, 'C#4', 2), (8, 'D4', 4), (12, 'E4', 4)]
F7_MEL = [(0, 'B4', 2), (2, 'C#5', 2), (4, 'D5', 2), (6, 'B4', 2), (8, 'F#5', 7.5)]
F7_RUN = ['F#3', 'G3', 'F#3', 'E3', 'F#3', 'E3', 'D3', 'C#3']
TENS_TOP = ['F5', 'E5', 'F5', 'E5', 'F#5', 'F5', 'F#5', 'F5']
TENS_LOW = ['B4', 'A#4', 'B4', 'A#4', 'C5', 'B4', 'C5', 'B4']
TENS_HI = ['B5', 'D6', 'B5', 'D6', 'C6', 'D#6', 'C6', 'D#6']
TENS_STR = [('F4', 'F5'), ('E5', 'E5'), ('F4', 'F5'), ('E5', 'E5'), ('F#4', 'F#5'), ('F5', 'F5'), ('F#4', 'F#5'),
            ('F5', 'F5')]
SQ_RHYTHM = [0, .5, 1.5, 2, 3, 3.5]

# ---------------------------------------------------------------- buses
names = ['kick', 'snare', 'hats', 'perc', 'bass', 'pulse8', 'guitar', 'lead', 'strings', 'pad', 'vox', 'stab']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, t, fn, gain=1.0, length=8.0, key=None, pan=0.0):
    if not live(t, length):
        return
    sig = cached(key, fn) if key is not None else fn()
    if pan:
        g = np.array([[np.sqrt(1 - pan)], [np.sqrt(1 + pan)]], np.float32)
        sig = sig * g
    bus[name].add(sig, t - OFF, gain)


# --------------------------------------------------------------- drums

def kick(bar, beat, vel=1.0):
    t = T(bar, beat)
    put('kick', t, lambda: W.dmx_kick(1.0), hv(vel, 0.03), length=1, key=('kick',))
    if OFF <= t < OFF + N / SR:
        kicks.append(t - OFF)


def snare(bar, beat, vel=1.0, clap=0.0):
    seed = int(rng.integers(4))
    lv = 0 if vel < 0.6 else (1 if vel < 0.9 else 2)
    sv = (0.55, 0.8, 1.0)[lv]
    put('snare', T(bar, beat), lambda: W.dark_snare(seed, sv), hv(vel, 0.05), length=1, key=('sn', seed, lv))
    if clap:
        cs = int(rng.integers(3))
        put('snare', T(bar, beat) + 0.004, lambda: W.soft_clap(cs), clap, length=1, key=('clap', cs))


def hat(bar, beat, vel, open_=False):
    seed = int(rng.integers(4))
    put('hats', T(bar, beat), lambda: W.cold_hat(1.0, seed, open_), hv(vel, 0.12), length=1,
        key=('hat', seed, open_), pan=0.25)


def hats16(b, vel=1.0, step=0.25, open_every=0):
    for i in range(int(4 / step)):
        bb = i * step
        if bb % 1 == 0:
            acc = 0.6
        elif bb % 0.5 == 0:
            acc = 0.85
        else:
            acc = 0.38
        hat(b, bb, acc * vel)
    if open_every and b % open_every == open_every - 1:
        hat(b, 3.5, 0.55 * vel, True)


def tom(bar, beat, note, vel=0.8):
    md = m(note)
    put('perc', T(bar, beat), lambda: W.dmx_tom(md), hv(vel, 0.04), length=1, key=('tom', md))


def tom_fill(bar, start=2.0, notes=('D3', 'B2', 'G2', 'E2'), vel=0.75, step=0.5):
    for i, n in enumerate(notes):
        tom(bar, start + i * step, n, vel * (0.85 + 0.15 * i / max(1, len(notes) - 1)))


def crash(bar, vel=0.8, beat=0.0):
    seed = bar % 2
    put('perc', T(bar, beat), lambda: W.dark_crash(seed), vel * 0.6, length=4, key=('cr', seed))


def beat_riff(b0, b1, vel=1.0, hat_vel=1.0, step=0.25, snares=True, clap_every=0, fills=True):
    """Kick on the riff's 3+3+2 (1, the dotted-quarter, 3), snare on 2 and 4."""
    for b in range(b0, b1):
        kick(b, 0, vel)
        kick(b, 1.5, 0.8 * vel)
        kick(b, 2, 0.9 * vel)
        if b % 4 == 3:
            kick(b, 3.5, 0.6 * vel)
        if snares:
            snare(b, 1, 0.85 * vel)
            cl = 0.35 if (clap_every and b % clap_every == clap_every - 1) else 0.0
            snare(b, 3, 0.95 * vel, clap=cl)
        hats16(b, hat_vel, step)
        if fills and b % 8 == 7 and snares:
            snare(b, 3.75, 0.45 * vel)


def beat_four(b0, b1, vel=1.0, hat_vel=1.0, step=0.25, snares=True, clap_every=0, open_every=2):
    for b in range(b0, b1):
        for beat in range(4):
            kick(b, beat, vel * (1.0 if beat % 2 == 0 else 0.88))
        if snares:
            snare(b, 1, 0.85 * vel)
            cl = 0.35 if (clap_every and b % clap_every == clap_every - 1) else 0.0
            snare(b, 3, 0.95 * vel, clap=cl)
        hats16(b, hat_vel, step, open_every)


# ---------------------------------------------------------------- bass

def bass_bar(bar, hits, style='8', vel=1.0, cut=260.0, env=900.0):
    """Sequencer bass over one bar of (beat, len, note) hits.
    '8': 8ths on each hit's note (accent on the hit), '16': 16ths with
    accents on the 8ths, 'long': one muted-but-held note per hit."""
    for beat, dur, note in hits:
        md = m(note)
        if style == 'long':
            d = B(dur) * 0.95
            put('bass', T(bar, beat), lambda: W.seq_bass(md, d, 0.9, cut=cut * 0.8, env=env * 0.5, decay=0.25),
                vel, length=d + 1, key=('bl', md, round(d, 3), cut, env))
            continue
        step = 0.5 if style == '8' else 0.25
        k = int(round(dur / step))
        for i in range(k):
            bb = beat + i * step
            acc = 1.0 if i == 0 else (0.82 if (bb % 0.5) == 0 else 0.62)
            d = B(step) * 0.78
            av = round(acc, 2)
            put('bass', T(bar, bb), lambda: W.seq_bass(md, d, av, cut=cut, env=env), vel * (0.85 + 0.15 * acc),
                length=1, key=('b', md, round(d, 3), av, cut, env))


def pedal8(bar, vel=0.5, kind='mute', note='F#3'):
    """The original's F#3 8ths (taiko, ch 7) with the 16th pickup on odd bars."""
    md = m(note)
    beats = [i * .5 for i in range(8)]
    if bar % 2 == 0:
        beats = beats[:-1] + [3.5, 3.75]
    for bb in beats:
        acc = 1.0 if bb % 1 == 0 else 0.75
        if bb == 3.75:
            acc = 0.6
        if kind == 'mute':
            sd = int(bb * 4) % 3
            put('pulse8', T(bar, bb), lambda: W.guitar(md, B(.4), 0.75, mute=True, seed=sd), hv(vel * acc, 0.08),
                length=1, key=('pm', md, sd), pan=-0.35)
        else:   # a short resonant blip (tone variation for other sections)
            put('pulse8', T(bar, bb), lambda: W.stab(md, B(.3), 1.0, cut=600, env=2200, decay=0.04),
                hv(vel * acc, 0.08), length=1, key=('blip', md), pan=-0.35)


# ------------------------------------------------------------ melodic parts

def lead(bar, notes, vel=1.0, transpose=0, cut=3300.0, name='lead', pan=0.0, legato=0.02):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur) - 0.03 + legato
        put(name, T(bar, beat), lambda: W.pulse_lead(md, d, 1.0, cut=cut), vel, length=d + 2,
            key=('ld', md, round(d, 3), cut), pan=pan)


def gtr(bar, notes, vel=0.8, transpose=0, pan=0.2, mute=False, tau=1.6, name='guitar', gain=1.0):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur)
        sd = int(rng.integers(3))
        put(name, T(bar, beat) + 0.002 * rng.random(), lambda: W.guitar(md, d, vel, tau=tau, mute=mute, seed=sd),
            gain, length=d + 1, key=('gt', md, round(d, 3), vel, tau, mute, sd), pan=pan)


def strings(bar, notes, vel=1.0, transpose=0, trem=0.0, a=0.35, r=0.9, cut=3600.0):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur)
        put('strings', T(bar, beat), lambda: W.string_machine(md, d, 1.0, a=a, r=r, cut=cut, trem=trem,
                                                               trem_hz=TREM_HZ),
            vel, length=d + 5, key=('sm', md, round(d, 3), trem, a, r, cut))


def vox(bar, notes, vel=1.0, vowel='oo', transpose=0, a=0.45, name='vox'):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur)
        put(name, T(bar, beat), lambda: W.vox(md, d, 1.0, vowel, a=a), vel, length=d + 6,
            key=('vx', md, round(d, 3), vowel, a))


def pads(b0, b1, vel=1.0, a=0.8, cut=900.0, rhythm=None):
    b = b0
    while b < b1:
        ch = CHORDS[b]
        e = b
        while e < b1 and CHORDS[e] == ch and rhythm is None:
            e += 1
        if rhythm is None:
            hits = [(0, (e - b) * 4)]
        else:
            hits = rhythm
            e = b + 1
        for beat, dur in hits:
            d = B(dur) * 0.97
            for note in VOICE[ch]:
                md = m(note)
                put('pad', T(b, beat), lambda: W.cold_pad(md, d, 1.0, a=a, cut=cut), vel, length=d + 7,
                    key=('pd', md, round(d, 3), a, cut))
        b = e


def stabs(bar, beat, notes, dur=0.5, vel=0.7):
    for note in notes:
        md = m(note)
        d = B(dur) * 0.8
        put('stab', T(bar, beat), lambda: W.stab(md, d, 1.0), vel, length=2, key=('st', md, round(d, 3)))


def riff_hits(bar, vel=0.6):
    """The orchestra-hit motif B | C F# A (orig ch 8 / 22)."""
    for k in range(4):
        if k % 2 == 0:
            stabs(bar + k, 0, ['B3', 'B4'], 1.5, vel)
        else:
            stabs(bar + k, 0, ['C4', 'C5'], 1.0, vel)
            stabs(bar + k, 1.5, ['F#3', 'F#4'], 1.0, vel * 0.9)
            stabs(bar + k, 3, ['A3', 'A4'], 0.8, vel * 0.9)


def riff_strings(bar, vel=0.7, octave=True, low=True):
    for k in range(4):
        b = bar + k
        if k % 2 == 0:
            line = [(0, 'F#4', 1.5), (1.5, 'A4', 1.5), (3, 'F#4', 1)]
            if low:
                strings(b, [(0, 'D4', 4)], vel * 0.55)
        elif k == 1:
            line = [(0, 'E4', 3), (3, 'D4', 1)]
            if low:
                strings(b, [(0, 'E4', 4)], vel * 0.45, transpose=-12)
        else:
            line = [(0, 'G4', 4)]
            if low:
                strings(b, [(0, 'E4', 4)], vel * 0.45)
        strings(b, line, vel, a=0.08, r=0.4)
        if octave:
            strings(b, line, vel * 0.55, transpose=12, a=0.08, r=0.4)


def riff_vox(bar, vel=0.8, vowel='oo'):
    vox(bar, [(0, 'B4', 4), (4, 'C5', 4), (8, 'B4', 4), (12, 'A4', 2), (14, 'G4', 2)], vel, vowel)
    vox(bar, [(0, 'B3', 16)], vel * 0.6, 'oo')


def trem_hi(bar, beats, vel=0.4, note='F#6'):
    strings(bar, [(0, note, beats)], vel, trem=0.75, a=1.0, r=1.2, cut=4200)


# ---------------------------------------------------------- arrangement
# INTRO 0-7 (orig 1-4 twice)
for cyc in range(2):
    b0 = cyc * 4
    for k in range(4):
        b = b0 + k
        pedal8(b, 0.45 + 0.1 * cyc + 0.05 * k)
        if cyc == 1:
            bass_bar(b, INTRO_BASS[k], '8', 0.9)
        else:
            bass_bar(b, INTRO_BASS[k], 'long', 0.75)
    trem_hi(b0, 16, 0.25 + 0.12 * cyc)
    vox(b0, [(0, 'B3', 4), (4, 'C4', 4), (8, 'D4', 2), (10, 'E4', 2), (12, 'F#4', 2), (14, 'E4', 2)],
        0.6 + 0.25 * cyc, 'oo', a=0.8 if cyc == 0 else 0.4)
    strings(b0, [(0, 'F#4', 4), (4, 'E4', 4), (8, 'F#4', 2), (10, 'E4', 2), (12, 'F#4', 2), (14, 'A4', 2)],
            0.35 + 0.25 * cyc, a=0.8 if cyc == 0 else 0.3)
for b in (2, 3):
    hats16(b, 0.5 + 0.2 * (b - 2), 0.5)
    kick(b, 0, 0.6)
    kick(b, 2, 0.6)
beat_four(4, 8, 0.9, 0.85, open_every=0)
for bb in (4, 6):
    lead(bb, [(beat, 'F#4', .45) for beat in SQ_RHYTHM], 0.45, cut=1800)
for beat in (2, 2.5, 3, 3.5):
    stabs(7, beat, ['F#3', 'F#4'], 0.5, 0.55 + 0.1 * (beat - 2))
tom_fill(7, 3.0, ('D3', 'B2'), 0.6)
put('perc', T(7), lambda: W.swell(B(4)), 0.35, length=3)

# RIFF A 8-15 (orig 5-12)
crash(8, 0.7)
for b in range(8, 16):
    bass_bar(b, RIFF_BASS[b % 2], '8', 1.0)
    pedal8(b, 0.5)
beat_riff(8, 16, 1.0, 1.0)
riff_strings(8, 0.55, octave=False)
riff_strings(12, 0.6, octave=True)
riff_vox(8, 0.6)
riff_vox(12, 0.7)
riff_hits(8, 0.4)
riff_hits(12, 0.45)
trem_hi(8, 32, 0.25)
pads(8, 16, 0.45, a=1.2, cut=700)

# MELODY A 16-23 (orig 13-20): the melody on the chorused guitar + a soft lead
crash(16, 0.6)
for b in range(16, 24):
    bass_bar(b, RIFF_BASS[b % 2], '8', 1.0)
    pedal8(b, 0.42)
beat_riff(16, 24, 1.0, 0.9, clap_every=4)
riff_strings(16, 0.45, octave=False)
riff_strings(20, 0.5, octave=False)
riff_vox(16, 0.5)
riff_vox(20, 0.55)
pads(16, 24, 0.45, a=0.6, cut=800)
gtr(16, MEL, 0.85, pan=0.1, gain=1.5)
gtr(16, MEL_HARM, 0.6, pan=0.3)
lead(16, MEL, 0.35, transpose=-12, cut=1600)
tom_fill(23, 2.0, ('D3', 'B2', 'G2', 'E2'), 0.7)

# TENSION 24-31 (orig 21-28): tritone 8ths, F / E / F# / F, building
crash(24, 0.6)
for k in range(8):
    b = 24 + k
    bass_bar(b, TENS_BASS[k], '8', 0.95)
    pedal8(b, 0.35 + 0.15 * k / 7, kind='blip')
    u = k / 7
    for i in range(8):
        acc = 0.95 if i % 2 == 0 else 0.7
        stabs(b, i * .5, [TENS_TOP[k]], 0.42, (0.35 + 0.3 * u) * acc)
        stabs(b, i * .5, [TENS_LOW[k]], 0.42, (0.28 + 0.25 * u) * acc)
    s1, s2 = TENS_STR[k]
    strings(b, [(0, s1, 2), (2, s2, 2)], 0.5 + 0.2 * u, a=0.1, r=0.4)
    trem_hi(b, 4, 0.18 + 0.12 * u, TENS_HI[k])
    vox(b, [(0, VOICE[TENS_CH[k]][1], 4), (0, VOICE[TENS_CH[k]][2], 4)], 0.5 + 0.2 * u, 'ah')
pads(24, 32, 0.4, a=0.5, cut=750)
beat_four(24, 28, 0.85, 0.7, snares=False, open_every=0)
beat_four(28, 31, 0.92, 0.85, open_every=0)
kick(31, 0, 0.95)
kick(31, 1, 0.9)
snare(31, 1, 0.85)
tom_fill(31, 2.0, ('F#3', 'D3', 'B2', 'G2', 'F#2', 'D2'), 0.8, 0.333)
for i in range(4):
    hat(31, i * .5, 0.5)
put('perc', T(30), lambda: W.swell(B(8)), 0.5, length=5)

# PEAK 1 32-47 (orig 37-52): the riff, then the descending guitar lines
crash(32, 0.9)
crash(40, 0.8)
for b in range(32, 48):
    bass_bar(b, RIFF_BASS[b % 2], '8', 1.0)
    pedal8(b, 0.4)
beat_four(32, 48, 1.0, 1.0, clap_every=8)
for b in (39, 47):
    snare(b, 3.5, 0.5)
    snare(b, 3.75, 0.6)
riff_strings(32, 0.65)
riff_strings(36, 0.65)
riff_vox(32, 0.75, 'oh')
riff_vox(36, 0.75, 'oh')
riff_hits(32, 0.55)
riff_hits(36, 0.55)
pads(32, 48, 0.4, a=0.3, cut=900)
trem_hi(32, 32, 0.25)
# 40-47: the descending harp lines on the chorused guitar, the doubled line on the lead
gtr(40, DESC, 0.9, pan=0.25)
gtr(40, [x for x in DESC if x[2] <= 0.5], 0.55, transpose=-12, pan=-0.25)
lead(40, DOUBLE_LINE, 0.75, transpose=12, cut=2400)
strings(40, DOUBLE_LINE, 0.5, a=0.1)
riff_strings(40, 0.45, octave=False)
riff_strings(44, 0.45, octave=False)
riff_vox(40, 0.55)
riff_vox(44, 0.55)

# SWITCH 48-55 (orig 53-60): Em descent, the F# G E sequence, F#7b9
crash(48, 0.8)
for b in range(48, 52):
    bass_bar(b, EM_BASS, '8', 1.0)
for b in range(52, 55):
    bass_bar(b, SEQ_BASS, '8', 1.0)
bass_bar(55, [(i * .5, .5, 'F#2') for i in range(8)], '8', 1.0)
for b in range(48, 56):
    pedal8(b, 0.38)
beat_four(48, 55, 1.0, 1.0, open_every=2)
kick(55, 0, 1.0)
kick(55, 1, 0.9)
snare(55, 1, 0.9)
for i, beat in enumerate((2, 2.5, 3, 3.5)):
    stabs(55, beat, ['F#3', 'F#4'], 0.5, 0.55 + 0.08 * i)
    kick(55, beat, 0.8)
tom_fill(55, 2.25, ('D3', 'B2', 'G2'), 0.6, 0.5)
gtr(48, EM_DESC, 0.9, pan=0.25)
lead(48, EM_BRASS, 0.85, cut=2400)
lead(48, EM_BRASS, 0.35, transpose=-12, cut=1500)
vox(48, [(0, 'E4', 4), (4, 'G4', 2), (6, 'B4', 2), (8, 'E5', 4), (12, 'D5', 2), (14, 'E5', 2)], 0.6, 'oh')
pads(48, 56, 0.45, a=0.3, cut=900)
lead(52, SEQ_MEL, 0.9, cut=3400)
lead(52, SEQ_MEL, 0.4, transpose=-12, cut=1600)
gtr(52, SEQ_MEL, 0.7, transpose=12, pan=0.3, tau=1.0)
strings(52, [(0, 'G3', 4), (0, 'B3', 4), (4, 'B3', 2), (4, 'D4', 2), (6, 'B3', 2), (6, 'E4', 2),
             (8, 'D4', 4), (8, 'F#4', 4), (12, 'F#4', 2), (12, 'A#4', 2), (14, 'C#5', 2)], 0.6)
vox(55, [(0, 'G4', 4), (0, 'A#4', 4), (0, 'E5', 4)], 0.55, 'ah')
put('perc', T(54), lambda: W.swell(B(8)), 0.4, length=5)

# BREAKDOWN 56-71 (orig 61-76): the chant as a dark vox pad, then the counter line
crash(56, 0.6)
put('kick', T(56), lambda: W.dmx_kick(1.0), 0.9, length=1)
for k in range(16):
    b = 56 + k
    ln = 4 if k < 15 else 3.5
    vox(b, [(0, CH_LOW[k % 4], ln)], 0.75, 'oo')
    vox(b, [(0, CH_LOW[k % 4], ln)], 0.45, 'oo', transpose=-12)
    dy = CHANT_DYADS[k % 8]
    strings(b, [(0, dy[0], ln), (0, dy[1], ln)], 0.35 + (0.1 if k >= 8 else 0), a=0.6, r=1.0)
    bh = CHANT_BASS[k % 4]
    bass_bar(b, bh, 'long' if k < 8 else '8', 0.75 if k < 8 else 0.85, cut=220 if k < 8 else 250)
    pedal8(b, 0.25 + 0.15 * k / 15, kind='mute' if k < 8 else 'blip')
pads(56, 72, 0.5, a=1.0, cut=750)
vox(56, CHANT, 0.85, 'oh', name='vox')
gtr(56, CHANT, 0.6, pan=0.15, tau=2.0)
trem_hi(56, 32, 0.2)
for b in range(56, 60):
    hats16(b, 0.35, 0.5)
for b in range(60, 64):
    kick(b, 0, 0.75)
    kick(b, 2, 0.65)
    hats16(b, 0.45, 0.5)
# guitar arpeggio on the chant chords (second half)
for i in range(64):
    ch = CHORDS[64 + i // 8]
    note = VOICE[ch][[1, 2, 3, 2][i % 4]]
    gtr(64, [(i * .5, note, .5)], 0.45 + 0.2 * (i / 64), transpose=12, pan=0.35, tau=0.9)
lead(64, COUNTER, 0.7, cut=2000)
strings(64, COUNTER_LO, 0.4, a=0.3)
vox(64, CHANT, 0.6, 'oh')
beat_four(64, 68, 0.8, 0.7, snares=False, open_every=0)
for b in range(64, 68):
    snare(b, 3, 0.7)
beat_four(68, 71, 0.92, 0.9, open_every=0)
kick(71, 0, 0.95)
snare(71, 1, 0.9)
tom_fill(71, 2.0, ('F#3', 'D3', 'B2', 'G2', 'F#2', 'D2'), 0.85, 0.333)
put('perc', T(70), lambda: W.swell(B(8)), 0.55, length=5)

# PEAK 2 72-87: the melody on the lead + guitar, then the chant on the lead
crash(72, 1.0)
crash(80, 0.9)
for b in range(72, 80):
    bass_bar(b, RIFF_BASS[b % 2], '16' if b % 2 == 0 else '8', 1.0)
    pedal8(b, 0.35)
for k in range(8):
    bass_bar(80 + k, CHANT_BASS[k % 4] if k % 2 == 0 else [(0, 4, CHANT_BASS[k % 4][0][2])], '8', 1.0)
    pedal8(80 + k, 0.35, kind='blip')
beat_four(72, 88, 1.05, 1.05, clap_every=4)
for b in (79, 87):
    snare(b, 3.5, 0.55)
    snare(b, 3.75, 0.65)
lead(72, MEL, 1.0, cut=3600)
lead(72, MEL, 0.4, transpose=-12, cut=1600)
gtr(72, MEL, 0.8, pan=0.3, tau=1.2, gain=0.8)
lead(72, MEL_HARM, 0.5, cut=2200)
riff_strings(72, 0.6)
riff_strings(76, 0.65)
riff_vox(72, 0.6, 'oh')
riff_vox(76, 0.65, 'oh')
riff_hits(72, 0.45)
riff_hits(76, 0.45)
pads(72, 80, 0.4, a=0.2, cut=1000)
trem_hi(72, 32, 0.28)
lead(80, CHANT, 1.0, transpose=12, cut=3600)
lead(80, CHANT, 0.45, cut=1800)
strings(80, COUNTER, 0.55, a=0.15)
for k in range(8):
    vox(80 + k, [(0, CH_LOW[k % 4], 4)], 0.7, 'oh')
    vox(80 + k, [(0, CHANT_DYADS[k % 8][0], 4)], 0.45, 'oo')
pads(80, 88, 0.45, a=0.2, cut=1000)
for i in range(128):
    ch = CHORDS[80 + i // 16]
    note = VOICE[ch][[1, 2, 3, 2][i % 4]]
    gtr(80, [(i * .25, note, .25)], 0.4 + (0.12 if i % 4 == 0 else 0), transpose=12, pan=0.4, tau=0.6)

# FINALE 88-95 (orig 77-80, 85-88): Em6, then F#7 and the bass run
crash(88, 0.9)
for b in range(88, 92):
    bass_bar(b, EM_BASS, '8', 1.0)
    pedal8(b, 0.35)
for b in range(92, 95):
    bass_bar(b, F7_BASS, '8', 1.0)
    pedal8(b, 0.35)
beat_four(88, 95, 1.0, 1.0, clap_every=4)
lead(88, EM6_MEL, 0.95, cut=3400)
lead(88, EM6_MEL, 0.4, transpose=-12, cut=1600)
gtr(88, EM6_MEL, 0.6, pan=0.3, tau=1.2, gain=0.7)
strings(88, [(0, 'G5', 16)], 0.4, a=0.6)
strings(88, EM6_RISE, 0.6, a=0.3)
vox(88, [(0, 'E4', 16), (0, 'B3', 16)], 0.55, 'oh')
pads(88, 95, 0.45, a=0.3, cut=950)
crash(92, 0.8)
lead(92, F7_MEL, 1.0, cut=3600)
lead(92, F7_MEL, 0.4, transpose=-12, cut=1600)
vox(92, [(0, 'A#4', 4), (4, 'G4', 4), (8, 'F#4', 7.5)], 0.7, 'oh')
vox(92, [(0, 'C#4', 4), (4, 'B3', 4), (8, 'C#4', 7.5)], 0.6, 'oo')
trem_hi(92, 15.5, 0.3, 'F#5')
strings(92, [(0, 'A#4', 8), (0, 'E4', 8)], 0.35, a=0.4)
stabs(93, 2, ['C#4', 'C#5'], 1.5, 0.45)
# the run (orig 88) on bass, guitar and lead
for i, note in enumerate(F7_RUN):
    bass_bar(95, [(i * .5, .5, note)], '8', 1.0)
gtr(95, [(i * .5, n, .5) for i, n in enumerate(F7_RUN)], 0.8, transpose=12, pan=0.2, tau=0.8)
lead(95, [(i * .5, n, .45) for i, n in enumerate(F7_RUN)], 0.55, transpose=12, cut=2400)
for beat in range(4):
    kick(95, beat, 0.95)
snare(95, 1, 0.9)
snare(95, 3, 0.95, clap=0.3)
for i in range(16):
    hat(95, i * .25, 0.35 + (0.3 if i % 2 == 1 else 0.15))

# OUTRO 96-99 (orig 89-92): the riff thins out, then the final B-minor hit
crash(96, 0.8)
for b in range(96, 100):
    bass_bar(b, RIFF_BASS[b % 2], '8', 0.95)
    pedal8(b, 0.4)
beat_riff(96, 99, 0.95, 0.8, step=0.5, fills=False)
kick(99, 0, 0.9)
kick(99, 1.5, 0.75)
kick(99, 2, 0.85)
snare(99, 1, 0.8)
for i in range(4):
    hat(99, i * .5, 0.4)
tom_fill(99, 3.0, ('D3', 'B2'), 0.65)
riff_strings(96, 0.55, octave=False)
riff_vox(96, 0.65)
riff_hits(96, 0.45)
trem_hi(96, 16, 0.25)
pads(96, 100, 0.4, a=0.3, cut=850)
lead(97, [(beat, 'F#4', .45) for beat in SQ_RHYTHM], 0.4, cut=1800)
gtr(98, [(0, 'F#5', 1), (1, 'B4', .5), (1.5, 'F#5', 2.5)], 0.55, pan=0.2, tau=1.6)
# the final hit (bar 100): kick, snare + clap, crash, Bm everywhere, ~3.5 s ring
kick(100, 0, 1.0)
snare(100, 0, 0.9, clap=0.4)
crash(100, 0.9)
FIN = B(4.5)
put('bass', T(100), lambda: W.seq_bass(m('B1'), FIN, 1.0, cut=200, env=600, decay=0.3), 0.9, length=6)
put('bass', T(100), lambda: W.sub_sine(m('B1'), FIN, 1.0, r=0.4), 0.45, length=6)
for note in ['B2', 'F#3', 'B3', 'D4', 'F#4']:
    md = m(note)
    put('pad', T(100), lambda: W.cold_pad(md, FIN, 1.0, a=0.01, r=1.0, cut=1100), 0.5, length=10)
strings(100, [(0, 'B3', 4.5), (0, 'D4', 4.5), (0, 'F#4', 4.5), (0, 'B4', 4.5)], 0.55, a=0.02, r=0.8)
vox(100, [(0, 'B3', 4.5), (0, 'D4', 4.5), (0, 'F#4', 4.5)], 0.6, 'oo', a=0.05)
for i, note in enumerate(['B2', 'F#3', 'B3', 'D4', 'F#4', 'B4']):
    md = m(note)
    put('guitar', T(100) + 0.018 * i, lambda: W.guitar(md, B(4.5), 0.85, tau=2.0, seed=i), 0.6, length=8,
        pan=-0.2 + 0.08 * i)
stabs(100, 0, ['B3', 'B4'], 1.0, 0.5)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_bass = S.sidechain(N, kicks, depth=0.45, release=0.16, power=1.5)
duck_soft = S.sidechain(N, kicks, depth=0.18, release=0.25)

GAIN = {  # dB
    'kick': -3.0, 'snare': -1.0, 'hats': -2.0, 'perc': -11.0, 'bass': -2.0, 'pulse8': 8.0,
    'guitar': 2.0, 'lead': -7.5, 'strings': -9.0, 'pad': -7.5, 'vox': -23.5, 'stab': -10.0,
}
DUCK = {'bass': duck_bass, 'pad': duck_soft, 'strings': duck_soft, 'vox': duck_soft, 'stab': duck_soft,
        'pulse8': duck_soft}
HALL = {'pad': .45, 'vox': .5, 'strings': .4, 'lead': .2, 'guitar': .3, 'stab': .25, 'pulse8': .12,
        'perc': .15}
PLATE = {'snare': .2, 'perc': .1}
DELAY = {'lead': .2, 'guitar': .3, 'pulse8': .05}

bus['kick'].x = stage(bus['kick'].x, Compressor(threshold_db=-10, ratio=3, attack_ms=5, release_ms=80))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(120), LowpassFilter(8000),
                       Compressor(threshold_db=-14, ratio=2.5, attack_ms=4, release_ms=100))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(6000), LowpassFilter(13000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(60))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(32), LowpassFilter(2800),
                      Compressor(threshold_db=-16, ratio=3, attack_ms=3, release_ms=80))
bus['pulse8'].x = stage(bus['pulse8'].x, HighpassFilter(150), LowpassFilter(5000))
bus['guitar'].x = W.ensemble(stage(bus['guitar'].x, HighpassFilter(140), PeakFilter(3000, 3.0, 0.8)),
                             mix=0.5, depth_ms=2.2, center_ms=9, rate=0.7)
bus['pulse8'].x = W.ensemble(bus['pulse8'].x, mix=0.35, depth_ms=1.5, center_ms=7, rate=0.5)
bus['lead'].x = W.ensemble(stage(bus['lead'].x, HighpassFilter(200), PeakFilter(2400, 1.5, 0.8)),
                           mix=0.3, depth_ms=1.2, center_ms=7, rate=0.45)
bus['strings'].x = W.ensemble(stage(bus['strings'].x, HighpassFilter(160)), mix=0.6, depth_ms=2.0,
                              center_ms=9, rate=0.6, fast=6.2, fast_ms=0.25)
bus['pad'].x = W.ensemble(stage(bus['pad'].x, HighpassFilter(150), PeakFilter(450, -2.0, 1.0), LowpassFilter(5000)), mix=0.5)
bus['vox'].x = W.ensemble(stage(bus['vox'].x, HighpassFilter(200), PeakFilter(450, -2.0, 1.0)), mix=0.4, depth_ms=1.5, rate=0.35)
bus['stab'].x = stage(bus['stab'].x, HighpassFilter(180))

mix = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
plate_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
peak_lv = {}
WINDOWS = {'melA': (16, 24), 'break': (56, 64), 'peak2': (72, 80)}
_cache.clear()
MUTE = set(filter(None, os.environ.get('SONG8DW_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in names:
    if k in MUTE:
        bus[k] = None
        continue
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    if BAR0 == 0 and BAR1 == NBARS:
        for wn, (w0, w1) in WINDOWS.items():
            w = x[:, int(T(w0) * SR):int(T(w1) * SR)]
            peak_lv[(k, wn)] = meter.integrated_loudness(w.T.astype(np.float64)) if np.any(w) else -99.0
    act = np.max(np.abs(x), axis=0) > 1e-4
    if act.sum() > SR:
        levels[k] = meter.integrated_loudness(x[:, act].T.astype(np.float64))
    del act
    mix += x
    if k in HALL:
        hall_send += x * HALL[k]
    if k in PLATE:
        plate_send += x * PLATE[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    bus[k] = None
    del x

pre = int(0.03 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
hall = stage(hs, HighpassFilter(250), LowpassFilter(6500),
             Reverb(room_size=0.93, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del hs, hall_send
plate = stage(plate_send, HighpassFilter(300), LowpassFilter(7000),
              Reverb(room_size=0.42, damping=0.6, wet_level=1.0, dry_level=0.0, width=0.9))
del plate_send
dly = S.pingpong(delay_send, B(0.75), feedback=0.38, repeats=6, lp=3800.0, hp=400.0)
del delay_send
mix = mix + hall * 0.9 + plate * 0.8 + dly * 0.55
del hall, plate, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -7), (3.9, -6), (4, -5), (7.9, -4.5), (8, -4), (15.9, -3.5), (16, -2.5), (23.9, -2.5),
              (24, -4.5), (31.9, -1.5), (32, 0), (47.9, 0), (48, -0.5), (55.9, -0.5),
              (56, -7.5), (63.9, -6.5), (64, -7), (71.9, -2.5), (72, 0.5), (87.9, 0.5),
              (88, 0), (95.9, 0), (96, -2.5), (99.9, -3), (100, -1.5), (110, -1.5)]
bars_axis = (np.arange(N) / SR + OFF) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(80, 0.5, 0.7), HighShelfFilter(9000, -1.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=15, release_ms=150))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
if BAR1 == NBARS:   # the final hit rings for ~4 s, then a short fade
    last = min(last, int((T(100) + 4.6 - OFF) * SR))
mix = mix[:, : last + 1]
fade = int(1.0 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song8_darkwave.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}  ' + ' '.join(f'{wn} {peak_lv.get((k, wn), -99):6.1f}' for wn in WINDOWS))
