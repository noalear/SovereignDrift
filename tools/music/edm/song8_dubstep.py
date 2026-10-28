#!/usr/bin/env python3
# song8_dubstep.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 8, Dubstep": a melodic / cinematic dubstep remix of sf-music8 (the
# original game's music by Stardock, from mid/sf-music8.mid), produced in code
# with tools/music/edm/synth.py and synth_dubstep.py: synthesized growl basses,
# sub, supersaws, choir, square lead, drums and effects, plus orchestral
# samples from VSCO 2 Community Edition (Versilian Studios,
# https://github.com/sgossner/VSCO-2-CE, CC0 1.0 Universal / public domain):
# violin, viola and cello sections (sustain, tremolo, spiccato), French horns
# (sustain, staccato), trumpets and trombones filling the horn set's gaps,
# a snare drum, the large ethnic drum (as taiko) and
# crash / suspended cymbals.
#
# Taken from the original (B minor, 200 BPM; bar numbers are the MIDI's):
#   - the intro (bars 1-4): the F#4 square-lead rhythm, the tremolo F#6, the
#     chromatic bass rise B C D-E F#-A under an F# pedal, the taiko 8ths and
#     the orchestra-hit F# 8ths;
#   - the riff (bars 5-20, 37-52, 89-92): bass B1 F#2 B2 | C3 G2 C2 in 3+3+2,
#     the strings F#-A-F# over D, the choir B C B A-G and the orchestra hits
#     B | C F# A; it is the growl-bass rhythm of the drops;
#   - the main melody (bars 13-20) F#5 B4 F#5 G5 C6 ... F#5 G5 E5 D5 (D5 E5);
#   - the tension section (bars 29-36): F/E/F# with tritone 8ths F5/B4,
#     E5/A#4, F#5/C5 (the build);
#   - the descending harp lines and the doubled F#3-A3-G3-F#3-G3-F#3 line
#     (bars 45-56), the Em descent (53-56);
#   - the F#5 G5 E5 sequence doubled in octaves and its F#7b9 (bars 57-60),
#     kept as written (the G against F# is intentional);
#   - the chant (bars 61-68) B4 B4 F#4 F#4 B4 C#5 B4 A#4 over
#     Bm | F#m/A | G/B | F#/A# and its counter line (69-76);
#   - the Em6 and F#7b9 ending (77-88) with the bass run F#3 G3 F#3 E3 ... C#3.
# New: 150 BPM half-time (one original bar = one dubstep bar, snare on 3),
# a dubstep form (intro, theme, build, drop, switch, breakdown, build,
# melodic drop, finale, outro), heavy bass on the riff rhythm, a mono sub
# with sub drops, a kick tuned to B1, a round layered snare, sidechain,
# supersaw chords and lead in the second drop, risers and impacts.
# v2 (2026-10-14, after the user's listening): the formant "yoi" growls and
# LFO filter-sweep wobbles of the drops (heard as a distracting "wipe") are
# replaced by tight non-vowel bass: fixed-tone FM stabs, distorted saw octave
# stabs, a fixed-cutoff reese, 1/8 and 1/16 gated chops on the riff rhythm and
# sub drops at the phrase ends; the noisy snare became a short, rounded
# round-robin snare; the breakdown (the part the user liked best) is unchanged.
#
#   python3 tools/music/edm/song8_dubstep.py OUT.wav
#   env SONG8_MUTE=kick,snare,...  mutes buses (for chroma checks)
#   env SONG8_BARS=a,b             renders only bars a..b-1 (while iterating)
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
import synth_dubstep as D  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 150.0
SPB = 60.0 / BPM
NBARS = 104
TAIL = 8.0
_bars = os.environ.get('SONG8_BARS')
BAR0, BAR1 = (int(v) for v in _bars.split(',')) if _bars else (0, NBARS)
OFF = (BAR0 * 4) * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(t, length=8.0):
    """Is an event at time t (absolute seconds) inside the render window?"""
    return t + length > OFF and t < OFF + N / SR


rng = np.random.default_rng(8)


def hv(v, amt=0.06):
    """Humanised velocity."""
    return float(np.clip(v * (1 + amt * (rng.random() * 2 - 1)), 0.05, 1.0))


# ------------------------------------------------------------- harmony
# Per-bar chord (for the chroma check) and supersaw voicings.
PCS = {
    'B5': 'B F#', 'Cx': 'C E F#', 'DE': 'D E F#', 'F#A': 'F# A E',
    'Bm': 'B D F#', 'C': 'C E G', 'Bdim/F': 'F B D', 'E7b5': 'E A# D', 'F#7b5': 'F# C E',
    'F7': 'F B D#', 'Em': 'E G B', 'F#7b9': 'F# A# G', 'F#m/A': 'A C# F#', 'G/B': 'G B D',
    'F#/A#': 'F# A# C#', 'Em6': 'E G C#', 'F#7': 'F# A# E', 'SEQ': 'F# G E',
}
VOICE = {
    'Bm': ['B3', 'D4', 'F#4', 'B4'], 'C': ['C4', 'E4', 'G4', 'C5'],
    'F#m/A': ['A3', 'C#4', 'F#4', 'A4'], 'G/B': ['B3', 'D4', 'G4', 'B4'],
    'F#/A#': ['A#3', 'C#4', 'F#4', 'A#4'], 'Em6': ['B3', 'C#4', 'E4', 'G4'],
    'F#7b9': ['A#3', 'C#4', 'E4', 'G4'], 'F#7': ['A#3', 'C#4', 'E4', 'F#4'], 'Em': ['B3', 'E4', 'G4', 'B4'], 'SEQ': ['B3', 'E4', 'G4', 'B4'],
}

RIFF_CH = ['Bm', 'C'] * 2
TENS_CH = ['Bdim/F', 'E7b5', 'Bdim/F', 'E7b5', 'F#7b5', 'F7', 'F#7b5', 'F#7b5']
CHANT_CH = ['Bm', 'F#m/A', 'G/B', 'F#/A#'] * 2
CHORDS = (['B5', 'Cx', 'DE', 'F#A'] * 2 +          # 0-7 intro (orig 1-4 twice)
          RIFF_CH * 4 +                              # 8-23 theme (orig 5-20)
          TENS_CH +                                  # 24-31 build (orig 29-36)
          RIFF_CH * 4 +                              # 32-47 drop 1 (orig 37-52)
          ['Em'] * 4 + ['SEQ'] * 3 + ['F#7b9'] +     # 48-55 switch (orig 53-60)
          CHANT_CH * 2 +                             # 56-71 breakdown (orig 61-76)
          RIFF_CH * 2 + CHANT_CH +                   # 72-87 drop 2 (orig 13-20, 61-68)
          ['Em6'] * 4 + ['F#7'] * 4 +                # 88-95 finale (orig 77-80, 85-88)
          RIFF_CH + ['Bm'] * 4)                      # 96-103 outro (orig 89-92, end)
assert len(CHORDS) == NBARS, len(CHORDS)

# Bass hits of each bar in 3+3+2 (beat, length, note)
def h332(a, b, c, last=1.0):
    return [(0, 1.5, a), (1.5, 1.5, b), (3, last, c)]


RIFF_BASS = [h332('B1', 'F#2', 'B2'), h332('C3', 'G2', 'C2')]
CHANT_BASS = [h332('B1', 'D2', 'F#2'), h332('A2', 'A2', 'A2'), h332('B1', 'D2', 'F#2'), h332('A#2', 'A#2', 'A#2')]
EM_BASS = h332('E2', 'B1', 'D2')
SEQ_BASS = h332('F#2', 'G2', 'E2')
F7_BASS = h332('F#2', 'C#3', 'E3')
TENS_BASS = ['F2', 'E2', 'F2', 'E2', 'F#2', 'F2', 'F#2', 'F#2']

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
SEQ_MEL = [(0, 'F#5', 1.5), (1.5, 'G5', 1.5), (3, 'E5', 1)] * 1
SEQ_MEL = [(b * 4 + x, n, l) for b in range(3) for x, n, l in SEQ_MEL] + [(12, 'F#5', 1.5), (13.5, 'G5', 1.5), (15, 'A#5', 1)]
CHANT = [(0, 'B4', 3), (3, 'B4', 1), (4, 'F#4', 3), (7, 'F#4', 1), (8, 'B4', 1.5), (9.5, 'C#5', 1.5), (11, 'B4', 1),
         (12, 'A#4', 4),
         (16, 'B4', 3), (19, 'B4', 1), (20, 'F#4', 3), (23, 'F#4', 1), (24, 'B4', 1.5), (25.5, 'C#5', 1.5),
         (27, 'B4', 1), (28, 'C#5', 4)]
CHANT_DYADS = [['D5', 'B5'], ['C#5', 'F#5'], ['D5', 'B5'], ['C#5', 'C#6']] * 2
COUNTER = [(0, 'F#5', 4), (4, 'F#5', 2), (6, 'E5', 2), (8, 'D5', 2), (10, 'F#5', 2), (12, 'F#5', 4),
           (16, 'F#5', 4), (20, 'F#5', 2), (22, 'E5', 2), (24, 'D5', 2), (26, 'G5', 2), (28, 'F#5', 3)]
COUNTER_LO = [(0, 'D5', 4), (4, 'C#5', 4), (8, 'B4', 4), (12, 'C#5', 4)] * 1
COUNTER_LO = COUNTER_LO + [(b + 16, n, l) for b, n, l in COUNTER_LO]
EM6_MEL = [(0, 'E5', 2), (2, 'C#5', 2), (4, 'E5', 2), (6, 'B4', 2)] * 1
EM6_MEL = EM6_MEL + [(b + 8, n, l) for b, n, l in EM6_MEL]
EM6_RISE = [(0, 'B3', 6), (6, 'C#4', 2), (8, 'D4', 4), (12, 'E4', 4)]
F7_MEL = [(0, 'B4', 2), (2, 'C#5', 2), (4, 'D5', 2), (6, 'B4', 2), (8, 'F#5', 7.5)]
F7_RUN = ['F#3', 'G3', 'F#3', 'E3', 'F#3', 'E3', 'D3', 'C#3']

# ---------------------------------------------------------------- buses
names = ['kick', 'snare', 'perc', 'hats', 'cym', 'sub', 'growl', 'strings', 'spic', 'brass',
         'choir', 'lead', 'pad', 'pluck', 'fx', 'impact']
bus = {k: S.Bus(N) for k in names}
kicks = []
snares = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, t, fn, gain=1.0, length=8.0, key=None):
    """Render fn() into bus `name` at absolute time t (skipped outside the window)."""
    if not live(t, length):
        return
    sig = cached(key, fn) if key is not None else fn()
    bus[name].add(sig, t - OFF, gain)


# instruments
VLN = D.Sampler('Strings/Violin Section/susVib/*.wav', gain=1.6, release=0.35)
VLA = D.Sampler('Strings/Viola Section/susvib/*.wav', gain=1.6, release=0.35)
VC = D.Sampler('Strings/Cello Section/susvib/*.wav', gain=1.4, release=0.3)
VLN_TR = D.Sampler('Strings/Violin Section/Trem/*.wav', gain=1.5, release=0.5)
VLA_TR = D.Sampler('Strings/Viola Section/trem/*.wav', gain=1.5, release=0.5)
VC_TR = D.Sampler('Strings/Cello Section/trem/*.wav', gain=1.2, release=0.4)
VLN_SP = D.Sampler('Strings/Violin Section/Spic/*.wav', gain=1.5, release=0.12)
VC_SP = D.Sampler('Strings/Cello Section/spic/*.wav', gain=1.3, release=0.12)
# brass sections: the VSCO horn set has no samples between C4 and D5, so
# trumpets and trombones fill in (each note goes to the closest sample)
HN = D.Section([(D.Sampler('Brass/F Horn/sus/*.wav', gain=2.2, release=0.3), 1.0),
                (D.Sampler('Brass/Trumpet/sus/*.wav', gain=2.2, release=0.3), 0.4),
                (D.Sampler('Brass/OldTrombone/Sustain/*.wav', gain=2.2, release=0.3), 0.55)])
HN_ST = D.Section([(D.Sampler('Brass/F Horn/stac/*.wav', gain=2.2, release=0.12), 1.0),
                   (D.Sampler('Brass/Trumpet/stac/*.wav', gain=2.2, release=0.12), 3.0),
                   (D.Sampler('Brass/Tenor Trombone/stac/*.wav', gain=2.2, release=0.12), 4.0)])


def samp(name, inst, bar, notes, vel=0.7, transpose=0, gain=1.0, attack=None, release=None, legato=0.0):
    for beat, note, dur in notes:
        t = T(bar, beat)
        md = m(note) + transpose
        d = B(dur) + legato
        put(name, t, lambda: inst.note(md, d, hv(vel), attack=attack, release=release), gain, length=d + 3)


def choir_notes(bar, notes, vel=1.0, vowel='a', gain=1.0, transpose=0, a=0.35):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur)
        put('choir', T(bar, beat), lambda: D.choir(md, d, 1.0, vowel, a=a), gain * vel, length=d + 4,
            key=('ch', md, round(d, 3), vowel, a))


def orch_hit(bar, beat, notes, dur, vel=0.85, gain=1.0):
    """Orchestra hit: horn staccato + violin / cello spiccato on an octave pair."""
    t = T(bar, beat)
    for note in notes:
        md = m(note)
        put('spic', t, lambda: HN_ST.note(md, B(dur) * 0.8, hv(vel)), gain * 0.9, length=3)
        if md >= 55:
            put('spic', t, lambda: VLN_SP.note(md, B(dur) * 0.6, hv(vel)), gain * 0.7, length=3)
        else:
            put('spic', t, lambda: VC_SP.note(md, B(dur) * 0.6, hv(vel)), gain * 0.7, length=3)


def sub_note(note):
    md = m(note)
    while md > 39:
        md -= 12
    while md < 28:
        md += 12
    return md


def sub_phrase(bar, hits, vel=1.0):
    """Sub under a drop bar; the last bar of each 8-bar phrase ends in a sub drop."""
    if bar % 8 == 7:
        sub_hits(bar, hits[:2], vel)
        sub_drop_at(bar, hits[2][0], hits[2][2], 4 - hits[2][0], vel)
    else:
        sub_hits(bar, hits, vel)


def sub_hits(bar, hits, vel=1.0, gate=0.94):
    for beat, dur, note in hits:
        md = sub_note(note)
        d = B(dur) * gate
        put('sub', T(bar, beat), lambda: S.sub_bass(md, d, 1.0, a=0.004, r=0.03, drive=1.3), vel, length=d + 1,
            key=('sub', md, round(d, 3)))


# ------------------------------------------------------------- growl bass
# Each pattern chops one bar of the 3+3+2 riff into bass hits:
# (beat, length, which riff note 0/1/2, style, settings); the snare on beat 3
# (beat index 2) is always left in a hole.  v2 (after the user's listening):
# no vowel / formant sweeps and no LFO filter sweeps -- only tight stabs with
# fixed tone: FM stabs, distorted saw octave stabs, a fixed-cutoff reese, and
# 1/8 and 1/16 gated chops; sub drops end the phrases.
STAB = dict(period=0, index=(1.5, 4.0), cut=(90, 2600))
STAB2 = dict(period=0, index=(1.0, 5.0), ratio=2.0, cut=(90, 3000))
SAW8 = dict(period=0, cut=(90, 1800))
REESE = dict(period=0, cut=(300, 2500))


def chop(step, base, kw, octave_last=False):
    return dict(step=step, base=base, kw=kw, octave_last=octave_last)


HEAVY = [
    [(0, .9, 0, 'saw8', SAW8), (1.5, .45, 1, 'stab', STAB), (2.5, .5, 1, 'chop', chop(.25, 'stab', STAB)),
     (3, 1.0, 2, 'chop', chop(.5, 'saw8', SAW8))],
    [(0, 1.25, 0, 'chop', chop(.5, 'stab', STAB2)), (1.5, .45, 1, 'saw8', SAW8), (2.5, .45, 1, 'stab', STAB),
     (3, 1.0, 2, 'chop', chop(.25, 'stab', STAB, True))],
    [(0, 1.4, 0, 'reese', REESE), (1.5, .45, 1, 'stab', STAB2), (2.5, .22, 1, 'stab', STAB), (2.75, .22, 1, 'saw8', SAW8),
     (3, .9, 2, 'saw8', SAW8)],
    [(0, 1.0, 0, 'saw8', SAW8), (1.5, .45, 1, 'stab', STAB2), (2.5, .5, 1, 'chop', chop(.25, 'saw8', SAW8)),
     (3, 1.0, 2, 'chop', chop(.25, 'stab', STAB, True))],
]
MELODIC = [
    [(0, 1.4, 0, 'reese', REESE), (1.5, .45, 1, 'saw8', SAW8), (2.5, .45, 1, 'stab', STAB), (3, .9, 2, 'reese', REESE)],
    [(0, 1.4, 0, 'reese', REESE), (1.5, .45, 1, 'stab', STAB2), (2.5, .45, 1, 'chop', chop(.25, 'stab', STAB)),
     (3, .9, 2, 'chop', chop(.5, 'saw8', SAW8))],
]


def gstab(bar, beat, md, d, style, kw, vel):
    put('growl', T(bar, beat), lambda: D.growl(md, d, style, vel=1.0, seed=bar % 3, **kw), vel, length=d + 1,
        key=('g', style, md, round(d, 3), str(sorted(kw.items()))))


def growl_bar(bar, hits, pattern, vel=1.0):
    notes = [n for _, _, n in hits]
    for beat, dur, idx, style, kw in pattern:
        md = m(notes[idx])
        if style == 'chop':
            k = int(round(dur / kw['step']))
            for i in range(k):
                up = 12 if (kw['octave_last'] and i == k - 1) else 0
                gstab(bar, beat + i * kw['step'], md + up, B(kw['step'] * 0.6), kw['base'], kw['kw'],
                      vel * (1.0 if i % 2 == 0 else 0.85))
            continue
        gstab(bar, beat, md, B(dur), style, kw, vel)


def sub_drop_at(bar, beat, note, beats, vel=1.0):
    md = sub_note(note)
    put('sub', T(bar, beat), lambda: D.sub_drop(md, B(beats)), vel, length=B(beats) + 1, key=('sdrop', md, beats))


# --------------------------------------------------------------- drums

def place_kick(bar, beat, vel=1.0):
    t = T(bar, beat)
    put('kick', t, lambda: S.kick(1.0, 0.5, f_end=61.74, f_start=230.0), vel, length=1, key=('kick',))
    if OFF <= t < OFF + N / SR:
        kicks.append(t - OFF)


def place_snare(bar, beat, vel=1.0, big=True):
    t = T(bar, beat)
    seed = int(rng.integers(4))   # round robin of 4 slightly different hits
    put('snare', t, lambda: D.round_snare(1.0, seed), hv(vel, 0.04), length=1.5, key=('sn', seed))
    if big and OFF <= t < OFF + N / SR:
        snares.append(t - OFF)


def half_time(bar0, bar1, vel=1.0, hats16=False, fills=True, kick_var=True):
    for b in range(bar0, bar1):
        place_kick(b, 0, vel)
        place_snare(b, 2, vel)
        if kick_var:
            if b % 2 == 0:
                place_kick(b, 1.5, 0.75 * vel)
                place_kick(b, 3, 0.8 * vel)
            else:
                place_kick(b, 2.75, 0.65 * vel)
                place_kick(b, 3.5, 0.8 * vel)
        if fills and b % 8 == 7:
            for i, bb in enumerate((3, 3.25, 3.5, 3.75)):
                place_snare(b, bb, (0.45 + 0.15 * i) * vel, big=False)
        elif fills and b % 4 == 3:
            place_snare(b, 3.75, 0.4 * vel, big=False)
        for i in range(8 if not hats16 else 16):
            step = 0.5 if not hats16 else 0.25
            bb = i * step
            acc = 0.8 if (bb % 1) == 0.5 else 0.45
            if hats16 and (bb % 0.5) == 0.25:
                acc = 0.3
            put('hats', T(b, bb), lambda: S.hat(1.0, False, -0.2 + 0.4 * rng.random()), hv(acc, 0.15) * vel, length=1)
        if b % 2 == 1:
            put('hats', T(b, 3.5), lambda: S.hat(1.0, True, 0.2), 0.45 * vel, length=1)


def marching(bar0, bar1, vel=0.4):
    """The original's marching-snare 8ths (D2 on every 8th, 16th pickups)."""
    for b in range(bar0, bar1):
        for i in range(8):
            acc = 1.0 if i % 2 == 0 else 0.7
            put('perc', T(b, i * .5), lambda: D.round_snare(1.0, i % 4), hv(vel * acc * 0.4, 0.2), length=1,
                key=('sn', i % 4))
        if b % 2 == 0:
            put('perc', T(b, 3.75), lambda: D.round_snare(1.0, 3), vel * 0.25, length=1, key=('sn', 3))


def taiko_bar(b, vel=0.8, kind=None):
    """The original's taiko 8ths on F#3 (ch 7), with its 16th pickups."""
    k = b % 4 if kind is None else kind
    if k == 3:
        beats = [0, .5, 1.5, 2, 2.5, 3, 3.5]
    else:
        beats = [i * .5 for i in range(8)] + ([3.75] if k in (0, 2) else [])
    for bb in beats:
        acc = 1.0 if bb in (0, 2) else (0.6 if bb % 1 == 0 else 0.45)
        sd = int(bb * 4) % 3
        put('perc', T(b, bb), lambda: D.taiko(1.0, -3.0, sd), hv(vel * acc, 0.1), length=2, key=('tk', sd))


def crash_at(bar, vel=1.0, beat=0.0):
    seed = bar % 2
    put('cym', T(bar, beat), lambda: D.crash_s(1.0, seed), vel, length=12, key=('cr', seed))


def impact_at(bar, vel=1.0, beat=0.0):
    put('impact', T(bar, beat), lambda: S.impact(1.0), vel, length=3, key=('imp',))


def snare_roll(bar0, segments, vel0=0.2, vel1=1.0, stop_beat=None):
    total = sum(b for b, _ in segments) * 4
    beat = 0.0
    for bars, div in segments:
        for i in range(int(bars * 4 * div)):
            bb = beat + i / div
            if stop_beat is not None and bb >= stop_beat:
                break
            u = bb / total
            tone = 190 + 140 * u
            put('snare', T(bar0) + B(bb), lambda: D.round_snare(1.0, i % 4), (vel0 + (vel1 - vel0) * u ** 1.3) * 0.85,
                length=1, key=('sn', i % 4))
        beat += bars * 4


def riser(bar0, bars, vel=0.7, f0=300.0, f1=10000.0):
    put('fx', T(bar0), lambda: S.noise_riser(B(bars * 4), f0, f1, 2.5), vel, length=B(bars * 4) + 1)


def rev_cym(bar, beats=4, vel=0.8):
    put('cym', T(bar) - B(beats), lambda: D.crash_s(1.0, 0, reverse=True, length=B(beats)), vel,
        length=B(beats) + 1)


# ------------------------------------------------------------- melodic parts

def lead(name, bar, notes, vel=1.0, transpose=0, cut=7500, cut_env=4000, voices=9, detune=0.2):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur) - 0.01
        put(name, T(bar, beat), lambda: S.supersaw(md, d, 1.0, voices=voices, detune=detune, a=0.004, d=0.6, s=0.8,
                                                    r=0.15, cut=cut, cut_env=cut_env, cut_decay=0.15,
                                                    vib=0.10 if dur >= 1.5 else 0.0), vel, length=d + 2)


def chords(bar0, bar1, rhythm=None, vel=1.0, cut=3500.0, attack=0.01):
    """Supersaw chords: sustained per bar, or stabs on (beat, len) rhythm."""
    for b in range(bar0, bar1):
        ch = CHORDS[b]
        hits = rhythm or [(0, 4)]
        for beat, dur in hits:
            for note in VOICE[ch]:
                md = m(note)
                d = B(dur) * 0.95
                put('pad', T(b, beat), lambda: S.supersaw(md, d, 1.0, a=attack, d=0.5, s=0.75, r=0.2, cut=cut,
                                                           cut_env=2500, cut_decay=0.2, detune=0.25),
                    vel * 0.5, length=d + 2, key=('pad', md, round(d, 3), cut, attack))


def plucks(bar, notes, vel=1.0, transpose=0):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(min(dur, 1.0))
        put('pluck', T(bar, beat), lambda: S.pluck(md, d, decay=0.25, bright=1.4), vel, length=d + 2,
            key=('pl', md, round(d, 3)))


# riff elements (4 bars, orig 5-8)
def riff_strings(bar, vel=0.65, gain=1.0, low=True):
    for k in range(4):
        b = bar + k
        if k % 2 == 0:
            samp('strings', VLA, b, [(0, 'D4', 4)], vel, gain=gain * 0.8)
            for beat, note, dur in ((0, 'F#4', 1.5), (1.5, 'A4', 1.5), (3, 'F#4', 1)):
                samp('strings', VLN, b, [(beat, note, dur)], vel, transpose=12, gain=gain)
                samp('strings', VLA, b, [(beat, note, dur)], vel, gain=gain * 0.7)
        elif k == 1:
            samp('strings', VLN, b, [(0, 'E5', 3), (3, 'D5', 1)], vel, gain=gain)
            samp('strings', VLA, b, [(0, 'E4', 3), (3, 'D4', 1)], vel, gain=gain * 0.8)
        else:
            samp('strings', VLN, b, [(0, 'G5', 4)], vel, gain=gain)
            samp('strings', VLA, b, [(0, 'E4', 4), (0, 'G4', 4)], vel, gain=gain * 0.7)
    if low:
        samp('strings', VLA, bar, [(0, 'B3', 16)], vel * 0.8, gain=gain * 0.6)


def riff_choir(bar, gain=1.0, vowel='a'):
    choir_notes(bar, [(0, 'B4', 4), (4, 'C5', 4), (8, 'B4', 4), (12, 'A4', 2), (14, 'G4', 2)], gain=gain, vowel=vowel)


def riff_hits(bar, vel=0.85, gain=1.0):
    for k in range(4):
        if k % 2 == 0:
            orch_hit(bar + k, 0, ['B3', 'B4'], 4, vel, gain)
        else:
            orch_hit(bar + k, 0, ['C4', 'C5'], 1.5, vel, gain)
            orch_hit(bar + k, 1.5, ['F#3', 'F#4'], 1.5, vel, gain)
            orch_hit(bar + k, 3, ['A3', 'A4'], 1, vel, gain)


def riff_cello(bar, vel=0.75, gain=1.0):
    for k in range(4):
        for beat, dur, note in RIFF_BASS[k % 2]:
            samp('strings', VC, bar + k, [(beat, note, dur)], vel, transpose=12, gain=gain, release=0.15)


def riff_trem(bar, gain=1.0):
    samp('strings', VLN_TR, bar, [(0, 'F#6', 16)], 0.6, gain=gain, attack=0.4)


# ---------------------------------------------------------- arrangement
# INTRO 0-7 (orig 1-4, twice)
for cyc in range(2):
    b0 = cyc * 4
    samp('strings', VLN_TR, b0, [(0, 'F#6', 16)], 0.5 + 0.2 * cyc, gain=0.9, attack=1.5 if cyc == 0 else 0.3)
    samp('strings', VC_TR, b0, [(0, 'B1', 4), (4, 'C2', 4), (8, 'D2', 2), (10, 'E2', 2), (12, 'F#2', 2), (14, 'A2', 2)],
         0.6 + 0.3 * cyc, transpose=12, gain=1.0, attack=0.15)
    samp('strings', VLN, b0, [(0, 'F#4', 4), (4, 'E4', 4), (8, 'F#4', 2), (10, 'E4', 2), (12, 'F#4', 2), (14, 'A4', 2)],
         0.5 + 0.2 * cyc, gain=0.8, attack=0.3)
    choir_notes(b0, [(0, 'B3', 4), (4, 'C4', 4), (8, 'D4', 2), (10, 'E4', 2), (12, 'F#4', 2), (14, 'E4', 2)],
                gain=0.7 + 0.3 * cyc, vowel='o')
    choir_notes(b0, [(0, 'B3', 4), (4, 'C4', 4), (8, 'B3', 2), (10, 'C4', 2), (12, 'B3', 4)],
                gain=0.5 + 0.3 * cyc, vowel='o', transpose=12)
    for b in range(b0, b0 + 4):
        taiko_bar(b, 0.35 + 0.15 * (b - b0) / 3 + 0.3 * cyc)
for bb in (4, 6):
    for beat in (0, .5, 1.5, 2, 3, 3.5):
        put('lead', T(bb, beat), lambda: D.square_lead(m('F#4'), B(.45), 1.0, cut=3000), 0.55, length=1,
            key=('sq', 'F#4'))
for cyc in range(2):
    for beat in (2, 2.5, 3, 3.5):
        orch_hit(3 + 4 * cyc, beat, ['F#4', 'F#5'], .5, 0.7 + 0.2 * cyc, 0.8)
put('sub', T(4), lambda: S.sub_bass(35, B(4) - .05), 0.5, length=3)
impact_at(4, 0.6)
marching(4, 8, 0.25)
for i, md in enumerate((52, 50, 47, 45)):
    put('perc', T(7, 2 + i * .5), lambda: S.tom(md), 0.6, length=1)
riser(6, 2, 0.4)
rev_cym(8, 4, 0.6)

# THEME 8-23 (orig 5-20): the riff, then the melody
for b4 in range(8, 24, 4):
    riff_strings(b4, 0.6 if b4 < 16 else 0.7)
    riff_choir(b4, 0.65 if b4 < 16 else 0.8)
    riff_hits(b4, 0.8, 0.9)
    riff_cello(b4, 0.8)
    riff_trem(b4, 0.6)
for b in range(8, 24):
    taiko_bar(b, 0.7 if b < 16 else 0.55)
    sub_hits(b, RIFF_BASS[b % 2], 0.55 if b < 16 else 0.75)
crash_at(8, 0.7)
impact_at(8, 0.7)
marching(8, 16, 0.3)
half_time(16, 24, 0.75, fills=True)
crash_at(16, 0.8)
# the melody (orig 13-20): violins at pitch, horns an octave below
samp('brass', HN, 16, MEL, 0.75, transpose=-12, gain=1.0, attack=0.04)
samp('strings', VLN, 16, MEL, 0.8, gain=1.1, attack=0.03, legato=0.05)
samp('brass', HN, 16, [(14, 'F#4', 2)], 0.7, gain=0.7)
samp('strings', VLA, 16, MEL_HARM, 0.6, gain=0.8)
lead('lead', 16, MEL, 0.25, cut=3000, cut_env=1500, voices=5)
rev_cym(24, 2, 0.6)

# BUILD 24-31 (orig 29-36): tritone 8ths, snare roll, risers
TENS_TOP = ['F5', 'E5', 'F5', 'E5', 'F#5', 'F5', 'F#5', 'F#5']
TENS_LOW = ['B4', 'A#4', 'B4', 'A#4', 'C5', 'B4', 'C5', 'C5']
TENS_SUS = [('B3', 'B5'), ('D4', 'D6'), ('B3', 'B5'), ('D4', 'D6'), ('C4', 'C6'), ('F4', 'D#6'), ('C4', 'C6'), ('F#4', 'C6')]
TENS_STR = [('F5', 'F6'), ('E5', 'E6'), ('F5', 'F6'), ('E5', 'E6'), ('F#5', 'F#6'), ('F5', 'F6'), ('F#5', 'F#6'), ('F#5', 'F#6')]
for k in range(8):
    b = 24 + k
    for i in range(8):
        if k == 7 and i >= 6:
            break
        acc = 0.95 if i % 2 == 0 else 0.7
        u = k / 8
        put('spic', T(b, i * .5), lambda: HN_ST.note(m(TENS_LOW[k]) - 12, B(.4), hv(0.6 + 0.3 * u)),
            acc * (0.6 + 0.4 * u), length=2)
        put('spic', T(b, i * .5), lambda: VLN_SP.note(m(TENS_TOP[k]), B(.4), hv(0.6 + 0.3 * u)),
            acc * (0.55 + 0.45 * u), length=2)
        put('spic', T(b, i * .5), lambda: VLN_SP.note(m(TENS_LOW[k]), B(.4), hv(0.6 + 0.3 * u)),
            acc * (0.45 + 0.4 * u), length=2)
    lo, hi = TENS_SUS[k]
    samp('strings', VLA_TR, b, [(0, lo, 4)], 0.7, gain=0.7)
    samp('strings', VLN_TR, b, [(0, hi, 4 if k < 7 else 3)], 0.7, gain=0.6)
    s1, s2 = TENS_STR[k]
    samp('strings', VLN, b, [(0, s1, 2), (2, s2, 2 if k < 7 else 1)], 0.7, gain=0.6)
    r = TENS_BASS[k]
    hits = h332(r, r, r) if k < 7 else [(i * .5, .45, r) for i in range(6)]
    for beat, dur, note in hits:
        samp('strings', VC_SP if k == 7 else VC, b, [(beat, note, dur)], 0.85, transpose=12, gain=1.0, release=0.12)
    sub_hits(b, hits, 0.6 + 0.3 * k / 7)
choir_notes(24, [(0, 'F5', 8), (8, 'F5', 8), (16, 'F#5', 8), (24, 'F#5', 7)], gain=0.5, vowel='a')
choir_notes(24, [(0, 'B4', 4), (4, 'A#4', 4), (8, 'B4', 4), (12, 'A#4', 4), (16, 'C5', 4), (20, 'B4', 4),
                 (24, 'C5', 7)], gain=0.5, vowel='a')
half_time(24, 28, 0.8, fills=False, kick_var=False)
for b in range(28, 31):
    for beat in range(4):
        place_kick(b, beat, 0.75 + 0.05 * (b - 28))
snare_roll(28, [(1, 2), (1, 4), (1, 4), (1, 8)], vel0=0.25, vel1=0.95, stop_beat=14.5)
riser(24, 8, 0.75, 250, 11000)
put('fx', T(28), lambda: S.pitch_riser(B(14.5), 47, 71), 0.55, length=7)
put('cym', T(24), lambda: D.swell(B(30)), 0.5, length=13)
crash_at(24, 0.7)
# pre-drop: a "yoi" pickup in the gap
put('growl', T(31, 3.5), lambda: D.growl(m('F#2'), B(.4), 'saw8', period=0, cut=(90, 1400)), 0.7, length=1)

# DROP 1 32-47 (orig 37-52)
for b in range(32, 48):
    growl_bar(b, RIFF_BASS[b % 2], HEAVY[(b - 32) % 4], vel=1.0)
    sub_phrase(b, RIFF_BASS[b % 2], 1.0)
half_time(32, 48, 1.0, hats16=False)
for b in (32, 40):
    crash_at(b, 1.0)
impact_at(32, 1.0)
for b4 in (32, 36, 40, 44):
    riff_hits(b4, 0.9, 0.85)
    riff_choir(b4, 0.45)
    riff_trem(b4, 0.5)
marching(40, 48, 0.25)
plucks(40, DESC, 0.8)
samp('strings', VLN_SP, 40, [x for x in DESC if x[2] <= 0.5], 0.75, gain=0.5)
samp('brass', HN, 40, DOUBLE_LINE, 0.8, transpose=12, gain=0.9)
samp('strings', VC, 40, DOUBLE_LINE, 0.8, gain=0.8)

# SWITCH 48-55 (orig 53-60): Em descent, F# G E sequence, F#7b9
for b in range(48, 52):
    growl_bar(b, EM_BASS, HEAVY[(b - 48) % 4], 1.0)
    sub_phrase(b, EM_BASS, 1.0)
for b in range(52, 55):
    growl_bar(b, SEQ_BASS, HEAVY[(b - 52 + 1) % 4], 1.0)
    sub_phrase(b, SEQ_BASS, 1.0)
growl_bar(55, h332('F#2', 'G2', 'F#2'), HEAVY[3], 1.0)
sub_hits(55, h332('F#2', 'G2', 'F#2'), 1.0)
half_time(48, 56, 1.0, hats16=True)
crash_at(48, 0.9)
crash_at(52, 0.9)
plucks(48, EM_DESC, 0.85)
samp('strings', VLN_SP, 48, [x for x in EM_DESC if x[2] <= 0.5], 0.75, gain=0.5)
samp('brass', HN, 48, EM_BRASS, 0.8, transpose=-12, gain=1.0)
samp('strings', VLN, 48, EM_BRASS, 0.75, gain=0.9)
choir_notes(48, [(0, 'E4', 4), (4, 'G4', 2), (6, 'B4', 2), (8, 'E5', 4), (12, 'D5', 2), (14, 'E5', 2)], gain=0.6)
for k in range(4):
    orch_hit(48 + k, 0, ['E4', 'E5'] if k < 3 else ['B3', 'B4'], 2, 0.85, 0.7)
# the doubled F# G E line (orig 57-60): violins + horns + lead in octaves
samp('strings', VLN, 52, SEQ_MEL, 0.85, gain=1.0)
samp('brass', HN, 52, SEQ_MEL, 0.85, transpose=-12, gain=1.0)
lead('lead', 52, SEQ_MEL, 0.55, cut=6000)
lead('lead', 52, SEQ_MEL, 0.3, transpose=-12, cut=4000)
samp('strings', VLA, 52, [(0, 'G3', 4), (0, 'B3', 4), (4, 'B3', 2), (4, 'D4', 2), (6, 'B3', 2), (6, 'E4', 2),
                          (8, 'D4', 4), (8, 'F#4', 4), (12, 'F#4', 2), (12, 'A#4', 2), (14, 'C#5', 2)], 0.7, gain=0.7)
samp('strings', VLN_TR, 52, [(0, 'B4', 4), (4, 'D5', 2), (6, 'E5', 2), (8, 'F#5', 4), (12, 'A#5', 4)], 0.7, gain=0.6)
for k in range(3):
    orch_hit(52 + k, 0, ['E4', 'E5'], 2, 0.85, 0.7)
for beat in (0, 1, 2, 2.5, 3, 3.5):
    orch_hit(55, beat, ['F#4', 'F#5'], .5, 0.95, 0.9)
choir_notes(55, [(0, 'G4', 4), (0, 'A#4', 4), (0, 'E5', 4)], gain=0.6)
riser(54, 2, 0.6, 400, 9000)

# BREAKDOWN 56-71 (orig 61-76): the chant with the choir, then the counter line
crash_at(56, 0.8)
impact_at(56, 0.8)
put('fx', T(56), lambda: S.downlifter(B(8)), 0.5, length=4)
CH_CHOIR = ['B3', 'F#4', 'G4', 'F#4']
for k in range(16):
    b = 56 + k
    c = CH_CHOIR[k % 4]
    ln = 4 if k < 15 else 3   # the last bar leaves a gap before the drop
    choir_notes(b, [(0, c, ln)], gain=0.85, vowel='a')
    choir_notes(b, [(0, c, ln)], gain=0.6, vowel='o', transpose=-12)
    dy = CHANT_DYADS[k % 8]
    samp('strings', VLN, b, [(0, dy[0], ln), (0, dy[1], ln)], 0.55, gain=0.6, attack=0.2)
    bh = CHANT_BASS[k % 4] if k < 15 else CHANT_BASS[3][:2]
    for beat, dur, note in bh:
        samp('strings', VC, b, [(beat, note, dur)], 0.6, transpose=12, gain=0.8, release=0.3)
    sub_hits(b, bh, 0.35 if k < 8 else 0.5)
samp('brass', HN, 56, CHANT, 0.7, gain=1.0, attack=0.05)
choir_notes(56, CHANT, gain=0.55, vowel='o', transpose=12)
samp('strings', VLN_TR, 56, [(0, 'F#6', 16), (16, 'F#6', 16)], 0.6, gain=0.75, attack=1.0)
# a quiet bell arpeggio on the chant chords (second half of the breakdown)
for i in range(32):
    ch = CHORDS[60 + i // 8]
    note = VOICE[ch][[1, 2, 3, 2][i % 4]]
    md = m(note) + 12
    put('pluck', T(60, i * .5), lambda: S.bell(md, B(.5), 1.0, r=0.8), 0.18 + 0.1 * (i / 32), length=4,
        key=('bell', md))
for k in range(8):
    b = 56 + k
    taiko_bar(b, 0.35, kind=1) if k % 2 == 1 else None
put('perc', T(56), lambda: D.taiko(1.0, -5.0, 0), 0.8, length=2)
# counter line (orig 69-76) on a filtered supersaw lead that opens up
lead('lead', 64, COUNTER, 0.8, cut=2500, cut_env=2000)
samp('strings', VLA, 64, COUNTER_LO, 0.6, gain=0.7)
samp('brass', HN, 64, CHANT, 0.75, gain=0.8)
plucks(64, [(i * .5, VOICE[CHORDS[64 + i // 8]][(i % 4)], .5) for i in range(64)], 0.35, transpose=12)
for b in range(64, 68):
    place_kick(b, 0, 0.7)
    place_kick(b, 2, 0.6)
for b in range(68, 71):
    for beat in range(4):
        place_kick(b, beat, 0.75)
place_kick(71, 0, 0.8)
place_kick(71, 1, 0.8)
snare_roll(68, [(1, 2), (1, 4), (1, 4), (1, 8)], vel0=0.25, vel1=1.0, stop_beat=14.5)
riser(64, 8, 0.75, 250, 12000)
put('fx', T(68), lambda: S.pitch_riser(B(14.5), 47, 71), 0.6, length=7)
rev_cym(72, 4, 0.8)
put('growl', T(71, 3.5), lambda: D.growl(m('F#2'), B(.4), 'saw8', period=0, cut=(90, 1400)), 0.7, length=1)

# DROP 2 72-87: melodic - supersaws carry the melody over the bass
for b in range(72, 80):
    growl_bar(b, RIFF_BASS[b % 2], MELODIC[b % 2], vel=0.85)
    sub_phrase(b, RIFF_BASS[b % 2], 1.0)
for k in range(8):
    b = 80 + k
    growl_bar(b, CHANT_BASS[k % 4], MELODIC[k % 2] if k % 4 != 3 else HEAVY[k % 4], vel=0.85)
    sub_phrase(b, CHANT_BASS[k % 4], 1.0)
half_time(72, 88, 1.0, hats16=True)
for b in (72, 80):
    crash_at(b, 1.0)
    impact_at(b, 1.0)
chords(72, 88, rhythm=[(0, 1.5), (1.5, 1.5), (3, 1)], vel=0.9, cut=4000)
lead('lead', 72, MEL, 1.0)
lead('lead', 72, MEL, 0.5, transpose=-12, cut=4500)
lead('lead', 76, MEL[9:], 0.3, transpose=12, cut=6000)
samp('strings', VLN, 72, MEL, 0.85, gain=0.6)
CHANT_HI = [(b, n, l) for b, n, l in CHANT]
lead('lead', 80, CHANT_HI, 1.0, transpose=12)
lead('lead', 80, CHANT_HI, 0.5, cut=4500)
samp('brass', HN, 80, CHANT, 0.85, gain=0.6)
for k in range(8):
    choir_notes(80 + k, [(0, CH_CHOIR[k % 4], 4)], gain=0.4, vowel='a', transpose=12)
samp('strings', VLN_TR, 80, [(0, 'F#6', 16), (16, 'F#6', 16)], 0.7, gain=0.5)
for b4 in (72, 76):
    riff_hits(b4, 0.85, 0.6)

# FINALE 88-95 (orig 77-80, 85-88): Em6, then F#7b9 and the bass run
for b in range(88, 92):
    growl_bar(b, EM_BASS, HEAVY[(b - 88) % 4], 1.0)
    sub_phrase(b, EM_BASS, 1.0)
for b in range(92, 95):
    growl_bar(b, F7_BASS, HEAVY[(b - 92) % 4], 1.0)
    sub_phrase(b, F7_BASS, 1.0)
for i, note in enumerate(F7_RUN):
    md = m(note)
    put('growl', T(95, i * .5), lambda: D.growl(md, B(.4), 'saw8' if i % 2 == 0 else 'stab', period=0,
                                                index=(1.5, 4.0), cut=(90, 1800)), 1.0, length=1)
    sub_hits(95, [(i * .5, .5, note)], 1.0)
    samp('brass', HN, 95, [(i * .5, note, .5)], 0.9, transpose=12, gain=0.9)
    samp('strings', VC_SP, 95, [(i * .5, note, .5)], 0.9, transpose=12, gain=1.0)
half_time(88, 95, 1.0, hats16=True)
place_kick(95, 0, 1.0)
for i in range(4):
    place_snare(95, 2 + i * .5, 0.45 + 0.1 * i, big=False)
crash_at(88, 1.0)
crash_at(92, 0.9)
lead('lead', 88, EM6_MEL, 0.9)
lead('lead', 88, EM6_MEL, 0.45, transpose=-12, cut=4500)
samp('brass', HN, 88, EM6_MEL, 0.8, transpose=-12, gain=0.9)
samp('strings', VLN, 88, [(0, 'G5', 16)], 0.7, gain=0.6)
samp('strings', VLA, 88, EM6_RISE, 0.75, gain=0.8)
chords(88, 95, rhythm=[(0, 1.5), (1.5, 1.5), (3, 1)], vel=0.75, cut=3500)
lead('lead', 92, F7_MEL, 1.0)
lead('lead', 92, F7_MEL, 0.45, transpose=-12, cut=4500)
samp('brass', HN, 92, [(6, 'E5', 2), (8, 'F#5', 7.5)], 0.8, transpose=-12, gain=0.9)
choir_notes(92, [(0, 'A#4', 4), (4, 'G4', 4), (8, 'F#4', 7.5)], gain=0.7)
choir_notes(92, [(0, 'C#4', 4), (4, 'B3', 4), (8, 'C#4', 7.5)], gain=0.6)
samp('strings', VLN_TR, 92, [(0, 'F#5', 15.5)], 0.7, gain=0.6)
orch_hit(93, 2, ['C#5'], 2, 0.8, 0.7)
riser(94, 2, 0.6, 400, 9000)

# OUTRO 96-103 (orig 89-92, then the end on B minor)
crash_at(96, 0.9)
impact_at(96, 0.9)
put('growl', T(96), lambda: D.growl(m('B1'), B(1.4), 'saw8', period=0, cut=(90, 1200)), 0.8, length=3)
sub_drop_at(96, 0, 'B1', 3, 0.8)
riff_strings(96, 0.6)
riff_choir(96, 0.75)
riff_hits(96, 0.75, 0.8)
riff_cello(96, 0.7)
riff_trem(96, 0.6)
for b in range(96, 100):
    taiko_bar(b, 0.6 - 0.08 * (b - 96))
    sub_hits(b, RIFF_BASS[b % 2], 0.6)
put('kick', T(100), lambda: S.kick(1.0, 0.5, f_end=61.74, f_start=230.0), 0.8, length=1)
impact_at(100, 0.6)
samp('strings', VLN_TR, 100, [(0, 'F#6', 14)], 0.6, gain=0.7)
samp('strings', VC_TR, 100, [(0, 'B1', 14)], 0.6, transpose=12, gain=0.9)
samp('strings', VLA, 100, [(0, 'D4', 14), (0, 'F#4', 14)], 0.5, gain=0.6, attack=0.4)
choir_notes(100, [(0, 'B3', 14), (0, 'D4', 14), (0, 'F#4', 14)], gain=0.6, vowel='o')
choir_notes(100, [(0, 'D5', 14), (0, 'B4', 14)], gain=0.5, vowel='a')
samp('strings', VLN, 100, [(0, 'D5', 14), (0, 'F#5', 14), (0, 'B5', 14)], 0.5, gain=0.6, attack=0.5)
samp('brass', HN, 100, [(0, 'B3', 10), (0, 'D4', 10), (0, 'F#4', 10)], 0.45, gain=0.7, attack=0.3)
put('sub', T(100), lambda: S.sub_bass(35, B(12)), 0.4, length=6)
for beat in (0, .5, 1.5, 2, 3, 3.5):
    put('lead', T(101, beat), lambda: D.square_lead(m('F#4'), B(.45), 1.0, cut=2500), 0.45, length=1, key=('sq2',))
put('perc', T(100), lambda: D.taiko(1.0, -5.0, 1), 0.7, length=2)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_sub = S.sidechain(N, kicks, depth=0.9, release=0.22)
duck_hard = S.sidechain(N, kicks + snares, depth=0.75, release=0.18, power=1.5)
duck_mid = S.sidechain(N, kicks + snares, depth=0.5, release=0.25)
duck_soft = S.sidechain(N, kicks + snares, depth=0.3, release=0.22)

GAIN = {  # dB
    'kick': -2.0, 'snare': 0.5, 'perc': -6.0, 'hats': -6.5, 'cym': -12.0, 'sub': -12.5, 'growl': 12.0,
    'strings': -6.0, 'spic': -10.5, 'brass': -7.0, 'choir': -5.0, 'lead': -6.5, 'pad': -8.5, 'pluck': -7.5,
    'fx': -18.0, 'impact': -8.0,
}
DUCK = {'sub': duck_sub, 'growl': duck_hard, 'pad': duck_mid, 'pluck': duck_soft, 'lead': duck_soft,
        'strings': duck_soft, 'choir': duck_soft, 'brass': duck_soft, 'spic': duck_soft, 'fx': duck_soft}
HALL = {'strings': .3, 'spic': .3, 'brass': .3, 'choir': .45, 'lead': .2, 'pad': .2, 'pluck': .3, 'perc': .25,
        'fx': .3, 'cym': .1}
PLATE = {'snare': .22}
DELAY = {'lead': .14, 'pluck': .3}

bus['growl'].x = stage(bus['growl'].x, HighpassFilter(90),
                       Compressor(threshold_db=-24, ratio=4, attack_ms=2, release_ms=60),
                       PeakFilter(3500, -2.0, 1.0), LowpassFilter(12000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(120))
bus['kick'].x = stage(bus['kick'].x, Compressor(threshold_db=-10, ratio=3, attack_ms=5, release_ms=80))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(150), PeakFilter(220, 1.5, 1.2), LowpassFilter(9000),
                       Compressor(threshold_db=-14, ratio=3, attack_ms=3, release_ms=120))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(6000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(45))
bus['strings'].x = stage(bus['strings'].x, HighpassFilter(55))
bus['spic'].x = stage(bus['spic'].x, HighpassFilter(150), PeakFilter(2500, 1.5, 1.0))
bus['brass'].x = stage(bus['brass'].x, HighpassFilter(120))
bus['choir'].x = stage(bus['choir'].x, HighpassFilter(180), Chorus(rate_hz=0.3, depth=0.3, centre_delay_ms=9, mix=0.4))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(250), PeakFilter(2800, 1.5, 0.8))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(200), Chorus(rate_hz=0.4, depth=0.25, centre_delay_ms=8, mix=0.3))
bus['pluck'].x = stage(bus['pluck'].x, HighpassFilter(300))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(250))
bus['cym'].x = stage(bus['cym'].x, HighpassFilter(400))
bus['impact'].x = stage(bus['impact'].x, HighpassFilter(38))

mix = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
plate_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
drop_lv = {}
MUTE = set(filter(None, os.environ.get('SONG8_MUTE', '').split(',')))
meter = pyln.Meter(SR)
# pre-drop gaps: everything but the growl pickup and the risers drops out
GAPS = [(31.75, 32.0), (71.75, 72.0)]
gap = np.ones(N, np.float32)
for g0, g1 in GAPS:
    i0, i1 = int((T(0, g0 * 4) - OFF) * SR), int((T(0, g1 * 4) - OFF) * SR)
    rmp = int(0.012 * SR)
    if 0 <= i0 and i1 <= N:
        gap[i0:i1] = 0.06
        gap[i0:i0 + rmp] = np.linspace(1, 0.06, rmp)
        gap[i1 - rmp // 3:i1] = np.linspace(0.06, 1, rmp // 3)
NO_GAP = {'growl', 'fx', 'kick', 'snare', 'impact', 'cym'}
for k in names:
    if k in MUTE:
        bus[k] = None
        continue
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    if k not in NO_GAP:
        x = x * gap[None, :]
    if BAR0 == 0 and BAR1 == NBARS:
        w = x[:, int(T(32) * SR):int(T(48) * SR)]
        if np.any(w):
            drop_lv[k] = meter.integrated_loudness(w.T.astype(np.float64))
    act = np.max(np.abs(x), axis=0) > 1e-4
    if act.sum() > SR:
        levels[k] = meter.integrated_loudness(x[:, act].T.astype(np.float64))
    mix += x
    if k in HALL:
        hall_send += x * HALL[k]
    if k in PLATE:
        plate_send += x * PLATE[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    bus[k] = None
    del x

pre = int(0.025 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
hall = stage(hs, HighpassFilter(300), LowpassFilter(8000),
             Reverb(room_size=0.92, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del hs, hall_send
plate = stage(plate_send, HighpassFilter(250), LowpassFilter(9000),
              Reverb(room_size=0.8, damping=0.35, wet_level=1.0, dry_level=0.0, width=1.0))
del plate_send
dly = S.pingpong(delay_send, B(0.75), feedback=0.4, repeats=6)
del delay_send
mix = mix + (hall * duck_soft[None, :] * 0.85 + plate * 0.8 + dly * 0.5) * gap[None, :]
del hall, plate, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -5.5), (8, -4.5), (16, -4), (23.9, -3.5), (24, -7), (31.7, -1.5), (31.75, -1.5), (32, 0), (47.9, 0),
              (48, 0), (55.9, 0), (56, -5.5), (63.9, -4.5), (64, -4.5), (71.7, -1.5), (71.75, -1.5), (72, 0.5),
              (87.9, 0.5), (88, 0.3), (95.9, 0.3), (96, -3.5), (100, -5), (104, -6), (110, -6)]
bars_axis = (np.arange(N) / SR + OFF) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(80, 0.5, 0.7), HighShelfFilter(10000, -1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=15, release_ms=150))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song8_dubstep.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}   drop 1: {drop_lv.get(k, -99):6.1f}')
