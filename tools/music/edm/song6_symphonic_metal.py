#!/usr/bin/env python3
# song6_symphonic_metal.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 6, Symphonic Metal": the battle track.  A symphonic-metal remix of
# sf-music6 (the original game's music by Stardock, from mid/sf-music6.mid),
# produced in code: synthesized guitars (Karplus-Strong strings through a
# high-gain amp and cabinet simulation, double-tracked), bass guitar, a metal
# drum kit and a synthesized choir (tools/music/edm/synth_metal.py and
# synth.py), plus an orchestra played from samples.
#
# Samples: VSCO-2 Community Edition by Versilian Studios (Sam Gossner, Simon
# Dalzell, Elan Hickler), CC0 1.0 / public domain,
# https://github.com/sgossner/VSCO-2-CE -- violin, viola, cello sections,
# solo contrabass, F horn, tenor trombone, tuba, trumpet, glockenspiel,
# timpani, concert bass drum, suspended cymbal, gong, marching and drum-kit
# snares.  Everything else is synthesized.
#
# The original is in E-flat Phrygian (E-flat minor against its flat second, E
# major over a B pedal), 180 BPM.  Its MIDI starts with a two-beat pickup;
# bar numbers below are the original's bars counted from the first downbeat.
# Taken from the original:
#   - the whistle ostinato   Eb4 Bb4 Eb5 E5 / E4 B4 E5 B4 (ch1, the whole song)
#                            -> spiccato violins and violas, glockenspiel
#   - the string riff        Eb4 F#4 F4 Eb4 . Eb4 F#4 Eb4 (3+3+2, ch5 bars 8-39)
#                            -> the main guitar riff, as power chords with chugs
#   - the bass riff          Eb2 . Bb1 Eb2 | B1 . Bb1 B1, its fills (bars 24-39)
#   - the intro melody       Eb ... Eb F# Ab | A | Bb F# Eb | Eb F# Bb C# | Eb (bars 0-8)
#   - the choir melody       Eb Bb B Bb-Ab | E B E B-Ab | Bb Ab F# F | E B E B (bars 8-23)
#   - the brass fanfare      Eb5 Bb4 Bb4 Eb5 Bb4 Bb4 Eb5 Eb5 ... F#5 | E5 (bars 18-20)
#   - the high string line   Eb6 / E6, Eb6 F#6 Ab6 E6 (ch2) and the horn line (bars 32-36)
#   - the stop-time break    hits Eb . Eb ... Bb Eb Ab with tom fills (bars 40-47)
#   - the horn phrase        Bb4 Ab4 | F#4 Ab4 F4 F#4 Eb4 (bars 56-59)
#   - the climb              E5 . E5 F#5 Ab5 . Ab5 Bb5 B5 . B5 C#6 Eb6 E6 and its
#                            canon answer (bars 60-63), the descending line
#                            E5 Eb5 C#5 B4 Bb4 Ab4 F#4 E4 (bars 68-71) and the
#                            E-major arpeggio sweep (bars 70-71)
#   - the ascent             Ebm E F# Abm in 3+3+2 with the D pedal (bars 80-87)
#   - the tritone stop       Bb/E hits and the run E Ab Bb C# (bars 88-89)
#   - the ending on E, resolving here down a semitone to E-flat (Phrygian cadence)
# New: the instrumentation and sound design, a metal form (orchestral intro,
# riff, verse, pre-chorus, break, verse 2, orchestral bridge, climax with a
# lead-guitar solo built from the original's lines, ascent, stop, final riff,
# Phrygian ending), drum grooves (gallop, skank, double-kick), the lead
# guitar's phrasing (bends, vibrato), dynamics and the mix.
#
#   python3 tools/music/edm/song6_symphonic_metal.py OUT.wav [BAR0 BAR1]
#   SONG6_MUTE=kick,snare,... mutes groups (for the chroma check)
import gc
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_metal as M  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 180.0
SPB = 60.0 / BPM
NBARS = 117
TAIL = 1.75     # the final hit at bar 116 rings about 3 s
BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
OFFSET = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)
MUTE = set(filter(None, os.environ.get('SONG6_MUTE', '').split(',')))


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def mm(x):
    return m(x) if isinstance(x, str) else int(x)


# ------------------------------------------------------------ the form
#  0- 7 intro (orchestra)       8-15 main riff           16-31 verse (choir)
# 32-47 pre-chorus (open)      48-55 stop-time break     56-71 verse 2
# 72-79 orchestral bridge      80-95 climax (solo)       96-103 ascent
# 104-105 tritone stop        106-113 final riff        114-116 ending

# harmony per bar (for pads, choir, chroma check)
CHORD_TONES = {
    'Ebm': ['Eb', 'Gb', 'Bb'], 'E': ['E', 'G#', 'B'], 'F#': ['F#', 'A#', 'C#'],
    'Abm': ['Ab', 'B', 'Eb'], 'Bb': ['Bb', 'E'], 'Eb5': ['Eb', 'Bb'],
}
EB_B = ['Ebm'] * 4 + ['E'] * 4
CHORDS = (['Ebm'] * 8 + EB_B + EB_B * 2 + EB_B * 2 + ['Ebm'] * 8 + EB_B * 2 + EB_B +
          EB_B * 2 + ['Ebm', 'E', 'F#', 'Abm'] * 2 + ['Bb'] * 2 + EB_B + ['E'] * 2 + ['Ebm'])
assert len(CHORDS) == NBARS, len(CHORDS)

# ------------------------------------------------------------ themes (beats)
WHISTLE = {'Ebm': ['Eb4', 'Bb4', 'Eb5', 'E5'], 'E': ['E4', 'B4', 'E5', 'B4'],
           'F#': ['Eb4', 'B4', 'Eb5', 'E5'], 'Abm': ['Eb4', 'B4', 'Eb5', 'F#5']}
# the original's string riff (ch5), 2 bars
STR_RIFF = {'Ebm': [(0, 'Eb4', 1.5), (1.5, 'F#4', 1), (2.5, 'F4', 1), (3.5, 'Eb4', 2), (5.5, 'Eb4', .5),
                    (6, 'F#4', 1), (7, 'Eb4', 1)],
            'E': [(0, 'E4', 1.5), (1.5, 'Ab4', 1), (2.5, 'E4', 1), (3.5, 'E4', 2), (5.5, 'E4', .5),
                  (6, 'Ab4', 1), (7, 'E4', 1)]}
# main guitar riff: accents on the string riff's notes, palm-muted chugs between
# (beat, kind, semitones above the chug root, length).  O open, C chug.
GTR_RIFF = {
    'Ebm': (39, [(0, 'O', 0, .5), (.5, 'C', 0, .5), (1, 'C', 0, .25), (1.25, 'C', 0, .25), (1.5, 'O', 3, 1),
                 (2.5, 'O', 2, 1), (3.5, 'O', 0, .5), (4, 'C', 0, .5), (4.5, 'C', 0, .25), (4.75, 'C', 0, .25),
                 (5, 'C', 0, .5), (5.5, 'O', 0, .5), (6, 'O', 3, 1), (7, 'O', 0, .5), (7.5, 'C', 0, .25),
                 (7.75, 'C', 0, .25)]),
    'E': (35, [(0, 'O', 5, .5), (.5, 'C', 0, .5), (1, 'C', 0, .25), (1.25, 'C', 0, .25), (1.5, 'O', 9, 1),
               (2.5, 'O', 5, 1), (3.5, 'O', 5, .5), (4, 'C', 0, .5), (4.5, 'C', 0, .25), (4.75, 'C', 0, .25),
               (5, 'C', 0, .5), (5.5, 'O', 5, .5), (6, 'O', 9, 1), (7, 'O', 5, .5), (7.5, 'C', 0, .25),
               (7.75, 'C', 0, .25)]),
}
# the bass riff (ch3), 2 bars
BASS_RIFF = {'Ebm': [(0, 'Eb2', 2.5), (2.5, 'Bb1', 1), (3.5, 'Eb2', 4.5)],
             'E': [(0, 'B1', 3), (3, 'Bb1', 1), (4, 'B1', 4)]}
# pre-chorus bass line with the original's fills (bars 24-31), 8 bars
BASS_PRE = [(0, 'Eb2', 2.5), (2.5, 'Bb1', 1), (3.5, 'Eb2', 2), (5.5, 'Eb2', .25), (5.75, 'Eb2', .25),
            (6, 'A2', .125), (6.125, 'Bb2', .375), (6.5, 'B2', .5), (7, 'Bb2', .5), (7.5, 'F#2', .5),
            (8, 'Eb2', 2.5), (10.5, 'Bb1', 1), (11.5, 'Eb2', 2), (13.5, 'Eb2', .5), (14, 'Bb2', 1), (15, 'Bb1', 1),
            (16, 'B1', 3), (19, 'E2', 1), (20, 'B1', 2), (22, 'Ab1', 2), (24, 'B1', 3), (27, 'Ab1', 1),
            (28, 'E2', 1.5), (29.5, 'D3', .25), (29.75, 'Eb3', .25), (30, 'E3', .5), (30.5, 'B2', .5),
            (31, 'Ab2', .5), (31.5, 'E2', .5)]
CHOIR_MEL = [(0, 'Eb4', 4), (4, 'Bb4', 4), (8, 'B4', 4), (12, 'Bb4', 2), (14, 'Ab4', 2),
             (16, 'E4', 4), (20, 'B4', 4), (24, 'E5', 4), (28, 'B4', 2), (30, 'Ab4', 2),
             (32, 'Bb4', 4), (36, 'Ab4', 4), (40, 'F#4', 4), (44, 'F4', 4),
             (48, 'E4', 4), (52, 'B4', 4), (56, 'E5', 4), (60, 'B4', 4)]
INTRO_MEL = [(0, 'Eb4', 4), (6.5, 'Eb4', .5), (7, 'F#4', .5), (7.5, 'Ab4', .5), (8, 'A4', 6),
             (16, 'Bb4', 1.5), (17.5, 'F#4', 1.5), (19, 'Eb4', 1), (22, 'Eb4', .5), (22.5, 'F#4', .5),
             (23, 'Bb4', .5), (23.5, 'C#5', .5)]
FANFARE = [(0, 'Eb5', .5), (.5, 'Bb4', .5), (1, 'Bb4', .5), (1.5, 'Eb5', .5), (2, 'Bb4', .5), (2.5, 'Bb4', .5),
           (3, 'Eb5', .5), (3.5, 'Eb5', .5), (7, 'F#5', 1), (8, 'E5', 4)]
HORN_LINE = [(2, 'Bb4', 2), (4, 'Eb5', 2), (6, 'F#5', 2), (8, 'Bb5', 6), (14, 'F#5', 2), (16, 'Ab5', 2),
             (18, 'E5', 6)]
BREAK_HITS = [(0, 1.5, 'Eb2'), (1.5, .5, 'Eb2'), (8, 1.5, 'Eb2'), (9.5, .5, 'Bb1'), (11, 1, 'Eb2'),
              (12, .75, 'Ab1')]
BREAK_MEL = [(0, 'Eb5', 1.5), (1.5, 'Eb5', 2.5), (8, 'Eb5', 1.5), (9.5, 'F#5', 1.5), (11, 'Eb5', 1),
             (12, 'Ab5', 4)]
PHRASE_A = [(0, 'Bb4', 7), (7, 'Ab4', 1), (8, 'F#4', 1), (9, 'Ab4', .5), (9.5, 'F4', 1), (10.5, 'F#4', 1),
            (11.5, 'Eb4', 2), (13.5, 'Eb4', .5), (14, 'F#4', 1), (15, 'Eb4', 1)]
CLIMB = [(0, 'E5', 3), (3, 'E5', .5), (3.5, 'F#5', .5), (4, 'Ab5', 3), (7, 'Ab5', .5), (7.5, 'Bb5', .5),
         (8, 'B5', 2.5), (10.5, 'B5', .5), (11, 'C#6', .5), (11.5, 'Eb6', .5), (12, 'E6', 4)]
CANON = [(1, 'E4', .5), (1.5, 'F#4', .5), (2, 'Ab4', 2), (5, 'Ab4', .5), (5.5, 'Bb4', .5), (6, 'B4', 2),
         (8.5, 'B4', .5), (9, 'C#5', .5), (9.5, 'Eb5', .5), (10, 'E5', 2), (12, 'E5', 1), (13, 'B4', 1),
         (14, 'Ab4', 1), (15, 'E4', 1)]
DESCENT = [(0, 'E5', 2), (2, 'Eb5', 2), (4, 'C#5', 2), (6, 'B4', 2), (8, 'Bb4', 2), (10, 'Ab4', 2),
           (12, 'F#4', 2), (14, 'E4', 2)]
SWEEP = [(i * .5, n, .5) for i, n in enumerate(
    ['B2', 'Eb3', 'E3', 'Ab3', 'B3', 'Eb4', 'E4', 'Ab4', 'B4', 'Eb5', 'E5', 'Ab5', 'B5', 'Eb6', 'E6', 'Ab6'])]
ASCENT = ['Ebm', 'E', 'F#', 'Abm']
ASC_ROOT = {'Ebm': 'Eb2', 'E': 'E2', 'F#': 'F#2', 'Abm': 'Ab2'}
ASC_BRASS = {'Ebm': ['Eb5', 'Bb5'], 'E': ['E5', 'B5'], 'F#': ['F#5', 'C#6'], 'Abm': ['Ab5', 'Eb6']}
ASC_LOW = {'Ebm': ['Eb4', 'F#4', 'Eb4'], 'E': ['E4', 'Ab4', 'E4'], 'F#': ['F#4', 'Bb4', 'F#4'],
           'Abm': ['Ab4', 'B4', 'Ab4']}
ASC_CHOIR = {'Ebm': 'Bb4', 'E': 'B4', 'F#': 'C#5', 'Abm': 'Eb5'}
ASC_TBN = {'Ebm': 'F#3', 'E': 'Ab3', 'F#': 'Bb3', 'Abm': 'B3'}
ASC_STR = {'Ebm': 'Eb6', 'E': 'E6', 'F#': 'F#6', 'Abm': 'Ab6'}

# ------------------------------------------------------------ the score
ev = {k: [] for k in ['gtr', 'bass', 'lead', 'kick', 'snare', 'tom', 'crash', 'china', 'hat', 'ride',
                      'timp', 'timproll', 'taiko', 'gong', 'march', 'swell', 'orch', 'choir', 'glock', 'fx']}


def orch(inst, t, note, dur, vel=0.8, **kw):
    ev['orch'].append((inst, t, mm(note), dur, vel, kw))


def line(inst, bar0, notes, transpose=0, vel=0.8, legato=1.0, **kw):
    for b, n, l in notes:
        orch(inst, T(bar0, b), mm(n) + transpose, B(l) * legato, vel, **kw)


def choir_line(bar0, notes, transpose=0, vel=0.8, vowel='a', **kw):
    for b, n, l in notes:
        ev['choir'].append((T(bar0, b), mm(n) + transpose, B(l), vel, vowel, kw))


# --- guitars and bass
def gtr(t, kind, root, dur, vel=1.0, tone='full', sides='LR'):
    """tone: 'full' (high gain, four takes) or 'crunch' (lower gain, two takes);
    sides: 'LR', 'L' or 'R' (which takes play)."""
    ev['gtr'].append((t, kind, mm(root), dur, vel, tone, sides))


def drop_gtr(bar, beat0, beat1):
    """Guitars drop out for a moment (leave space for the brass/strings)."""
    a, b = T(bar, beat0) - 1e-6, T(bar, beat1) - 1e-6
    ev['gtr'] = [e for e in ev['gtr'] if not (a <= e[0] < b)]


# harmony a diatonic third above the riff's notes (E-flat Phrygian), by offset
HARM3 = {'Ebm': {0: 3, 3: 4, 2: 3}, 'E': {5: 4, 9: 3}}


def bass(t, note, dur, vel=1.0):
    ev['bass'].append((t, mm(note), dur, vel))


def bass_of(root):
    r = mm(root) - 12
    return r if r >= 27 else r + 12


def main_riff(bar0, nbars, vel=1.0, with_bass=True, chugs=True, octave=0, open_kind='open',
              tone='full', sides='LR', harm=None):
    """harm: None, or a side ('L'/'R') on which the accents are played as a
    harmonised two-note line (melody note + diatonic third, an octave up)."""
    for b in range(bar0, bar0 + nbars, 2):
        root, pat = GTR_RIFF[CHORDS[b]]
        for beat, k, off, ln in pat:
            kind = open_kind if k == 'O' else 'chug'
            v = vel * (0.95 if k == 'O' else 0.78)
            if k == 'O' or chugs:
                gtr(T(b, beat), kind, root + off + 12 * octave, B(ln) - 0.012, v, tone, sides)
            if harm and k == 'O':
                n0 = root + off + 12
                gtr(T(b, beat), 'single', n0, B(ln) - 0.012, v * 0.9, tone, harm)
                gtr(T(b, beat), 'single', n0 + HARM3[CHORDS[b]][off], B(ln) - 0.012, v * 0.85, tone, harm)
            if with_bass:
                bass(T(b, beat), bass_of(root + off), B(ln) - 0.015, 0.9 if k == 'O' else 0.75)


def verse_gtr(bar0, nbars, vel=1.0, eighths=True, accent='open', tone='full', sides='LR', step=0.5):
    """Chugs following the original bass riff; accents on its notes."""
    for b in range(bar0, bar0 + nbars, 2):
        for beat, note, ln in BASS_RIFF[CHORDS[b]]:
            gtr(T(b, beat), accent, note, B(min(ln, 1.0)) - 0.012, vel * 0.95, tone, sides)
            bass(T(b, beat), bass_of(note), B(ln) - 0.02, 0.9)
            x = beat + 1.0
            while x < beat + ln - 0.01:
                if eighths:
                    gtr(T(b, x), 'chug', note, B(step) - 0.012, vel * (0.8 if x % 1 == 0 else 0.7), tone, sides)
                x += step


def open_line(bar0, notes, vel=1.0, kind='fifth'):
    for b, n, ln in notes:
        k = kind if ln >= 1 else 'mute'
        gtr(T(bar0, b), k, n, B(ln) - 0.012, vel)
        bass(T(bar0, b), bass_of(n), B(ln) - 0.02, 0.9)


def climax_gtr(bar0, nbars, vel=1.0):
    """8th chugs with open hits on the bass riff accents."""
    for b in range(bar0, bar0 + nbars, 2):
        for beat, note, ln in BASS_RIFF[CHORDS[b]]:
            gtr(T(b, beat), 'open', note, B(.5) - 0.012, vel)
            bass(T(b, beat), bass_of(note), B(ln) - 0.02, 0.95)
            x = beat + 0.5
            while x < beat + ln - 0.01:
                gtr(T(b, x), 'chug', note, B(.25) - 0.01, vel * 0.8)
                gtr(T(b, x + .25), 'chug', note, B(.25) - 0.01, vel * 0.65)
                x += 0.5


# --- drums
def hum(rng_, sd=0.003):
    return rng_.normal(0, sd)


DR = np.random.default_rng(66)
TOMS = [m('Ab3'), m('Eb3'), m('Bb2'), m('F#2')]


def kick(t, vel=1.0):
    ev['kick'].append((t + hum(DR, 0.002), vel * (0.93 + 0.07 * DR.random())))


def snare(t, vel=1.0, ghost=False):
    ev['snare'].append((t + hum(DR, 0.003), vel * (0.92 + 0.08 * DR.random()), ghost))


def tom(t, i, vel=1.0):
    ev['tom'].append((t + hum(DR, 0.003), TOMS[i], vel * (0.9 + 0.1 * DR.random()), i))


def crash(t, vel=1.0, side=0):
    ev['crash'].append((t + hum(DR, 0.002), vel, side))


def china(t, vel=1.0):
    ev['china'].append((t + hum(DR, 0.002), vel))


def hat(t, vel=1.0, open_=False):
    ev['hat'].append((t + hum(DR, 0.003), vel * (0.85 + 0.15 * DR.random()), open_))


def ride(t, vel=1.0):
    ev['ride'].append((t + hum(DR, 0.003), vel * (0.85 + 0.15 * DR.random())))


def groove(bar0, bar1, style, vel=1.0, crash_every=4, fill_every=4, fill=True):
    for b in range(bar0, bar1):
        last = fill and (b - bar0) % fill_every == fill_every - 1
        if (b - bar0) % crash_every == 0:
            crash(T(b), 0.9 * vel, side=(b // crash_every) % 2)
        end = 3.0 if last else 4.0
        if style == 'rock':          # kick 1, & of 2, 3; snare 2 and 4; hats in 8ths
            for x in (0, 1.5, 2.5):
                if x < end:
                    kick(T(b, x), vel)
            for x in (1, 3):
                if x < end:
                    snare(T(b, x), vel)
            for i in range(8):
                if i * .5 < end and not (i == 0 and (b - bar0) % crash_every == 0):
                    hat(T(b, i * .5), vel * (0.7 if i % 2 == 0 else 0.5))
        elif style == 'skank':       # thrash: kick on beats, snare on the offbeats
            for i in range(8):
                x = i * .5
                if x >= end:
                    break
                if i % 2 == 0:
                    kick(T(b, x), vel)
                    ride(T(b, x), vel * 0.55)
                else:
                    snare(T(b, x), vel * 0.92)
        elif style == 'double':      # 16th double kick, snare 2 and 4, china/ride quarters
            for i in range(16):
                x = i * .25
                if x < end:
                    kick(T(b, x), vel * (0.9 if i % 2 == 0 else 0.78))
            for x in (1, 3):
                if x < end:
                    snare(T(b, x), vel)
            for x in (0, 1, 2, 3):
                if x < end and not (x == 0 and (b - bar0) % crash_every == 0):
                    (china if x in (1, 3) else ride)(T(b, x), vel * (0.55 if x in (1, 3) else 0.6))
        elif style == 'gallop':      # kick in 8th + two 16ths, snare 2 and 4
            for x in (0, .5, .75, 1.5, 2, 2.5, 2.75, 3.5):
                if x < end:
                    kick(T(b, x), vel * 0.85)
            for x in (1, 3):
                if x < end:
                    snare(T(b, x), vel)
            for x in (0, 1, 2, 3):
                if x < end and not (x == 0 and (b - bar0) % crash_every == 0):
                    ride(T(b, x), vel * 0.55)
        elif style == 'half':        # half time: kick on 1 and & of 2, snare on 3, china on quarters
            for x in (0, 1.5, 1.75):
                if x < end:
                    kick(T(b, x), vel)
            if 2 < end:
                snare(T(b, 2), vel)
            for x in (0, 1, 2, 3):
                if x < end and not (x == 0 and (b - bar0) % crash_every == 0):
                    china(T(b, x), vel * 0.5) if x in (1, 3) else ride(T(b, x), vel * 0.6)
        if last:
            tom_fill(b, 3.0, vel)


def tom_fill(b, start, vel=1.0, kind=0):
    """16th fill from beat `start` to the bar end, high to low toms, kicks under."""
    n = int(round((4 - start) * 4))
    for i in range(n):
        x = start + i * .25
        ti = min(3, int(i * 4 / n))
        if kind == 1 and i % 4 == 0:
            snare(T(b, x), vel)
        else:
            tom(T(b, x), ti, vel * (0.85 + 0.15 * (i % 2 == 0)))
        if i % 2 == 0:
            kick(T(b, x), vel * 0.8)


def snare_roll(bar0, beats, v0=0.3, v1=1.0, div=4):
    for i in range(int(beats * div)):
        u = i / (beats * div)
        snare(T(bar0, i / div), v0 + (v1 - v0) * u ** 1.4, ghost=u < 0.5)


# --- orchestra helpers
def whistle(bar0, bar1, vel=0.6, inst='vln_spic', octave=0):
    for b in range(bar0, bar1):
        notes = WHISTLE[CHORDS[b] if CHORDS[b] in WHISTLE else 'Ebm']
        for i in range(8):
            v = vel * (1.0 if i % 4 == 0 else 0.8 if i % 2 == 0 else 0.7)
            orch(inst, T(b, i * .5), mm(notes[i % 4]) + 12 * octave, B(.45), v)


def strings_riff(bar0, bar1, inst='vc_spic', transpose=0, vel=0.7):
    for b in range(bar0, bar1, 2):
        ch = CHORDS[b] if CHORDS[b] in STR_RIFF else 'Ebm'
        line(inst, b, STR_RIFF[ch], transpose, vel, legato=0.9)


def pad(bar0, bar1, voicing, insts, vel=0.6):
    """Sustained chord voicings per bar group; voicing: chord -> list of notes."""
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        for inst, notes in zip(insts, voicing[ch]):
            for n in notes:
                orch(inst, T(b), n, T(e) - T(b) - 0.03, vel)
        b = e


PAD_V = {  # (violins, violas, celli, basses)
    'Ebm': (['Bb4', 'Eb5', 'Gb5'], ['Gb4', 'Bb3'], ['Eb3', 'Bb2'], ['Eb2']),
    'E': (['B4', 'E5', 'G#5'], ['G#4', 'B3'], ['E3', 'B2'], ['B1']),
    'F#': (['C#5', 'F#5', 'A#5'], ['A#4', 'C#4'], ['F#3', 'C#3'], ['F#2']),
    'Abm': (['B4', 'Eb5', 'Ab5'], ['Ab4', 'Eb4'], ['Ab3', 'Eb3'], ['Ab1']),
    'Bb': (['Bb4', 'E5'], ['Bb3', 'E4'], ['Bb2', 'E3'], ['Bb1']),
}
CHOIR_PAD = {'Ebm': ['Eb4', 'Gb4', 'Bb4'], 'E': ['E4', 'G#4', 'B4'], 'F#': ['F#4', 'A#4', 'C#5'],
             'Abm': ['Ab4', 'B4', 'Eb5'], 'Bb': ['Bb3', 'E4', 'Bb4']}


def choir_pad(bar0, bar1, vel=0.5, vowel='o', octave=0):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        for n in CHOIR_PAD[ch]:
            ev['choir'].append((T(b), mm(n) + 12 * octave, T(e) - T(b), vel, vowel, {}))
        b = e


def hit(t, chord_notes, vel=1.0, low=True):
    """Orchestral stab: staccato brass + spiccato strings (+ low brass)."""
    for n in chord_notes:
        orch('hn_stac', t, n, B(.5), vel)
        orch('vln_spic', t, mm(n) + 12, B(.5), vel)
    if low:
        orch('tba_stac', t, mm(chord_notes[0]) - 12, B(.5), vel)
        orch('tbn_stac', t, chord_notes[0], B(.5), vel)


# ================================================================ arrangement

# ---- INTRO 0-7: the orchestra alone. Timpani roll, low strings, the ostinato,
#      the intro melody on horns and trombones, marching snare enters.
ev['timproll'].append((T(0), T(2) - 0.05, 'Eb2', 0.2, 0.9))
ev['swell'].append((T(2), 'short', 0.5))
pad(0, 8, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.45)
for b in (0, 4):
    orch('vln_sus', T(b), 'Eb6' if b == 0 else 'D6', B(16 if b == 0 else 2) - 0.05, 0.5)
orch('vln_sus', T(4, 2), 'Bb5', B(14) - 0.05, 0.5)
whistle(0, 8, vel=0.5)
whistle(4, 8, vel=0.35, inst='vla_spic', octave=-1)
line('hn_sus', 0, [(0, 'Eb4', 4)], vel=0.7)
line('hn_sus', 2, INTRO_MEL, vel=0.85)
line('tbn_sus', 2, INTRO_MEL, transpose=-12, vel=0.75)
line('tpt_sus', 6, [(b - 16, n, l) for b, n, l in INTRO_MEL if b >= 16], vel=0.6)
choir_pad(2, 8, vel=0.35, vowel='o')
for b in range(2, 8):
    ev['taiko'].append((T(b), 0.55 + 0.05 * (b - 2)))
    ev['taiko'].append((T(b, 1.5), 0.45 + 0.05 * (b - 2)))
for b in range(4, 8):          # the original's marching snare in 8ths, crescendo
    for i in range(8):
        ev['march'].append((T(b, i * .5), (0.3 + 0.12 * (b - 4)) * (1.0 if i % 2 == 0 else 0.75)))
ev['timp'].append((T(4), 'Eb2', 0.8))
ev['timp'].append((T(6), 'Eb2', 0.9))
ev['timproll'].append((T(7), T(8) - 0.02, 'Bb2', 0.3, 1.0))
ev['swell'].append((T(8), 'short', 0.8))
ev['fx'].append(('riser', T(6), T(8) - T(6), 0.5))
snare_roll(7, 2, 0.2, 0.9)
tom_fill(7, 2.0, 0.9)

# ---- MAIN RIFF 8-15: the band slams in on the intro melody's last note.
ev['gong'].append((T(8), 0.8))
ev['fx'].append(('impact', T(8), 0, 1.0))
crash(T(8), 1.0, 1)
main_riff(8, 4)
main_riff(12, 4, chugs=False, octave=1, open_kind='fifth', with_bass=False)
main_riff(12, 4, chugs=False, with_bass=True, sides='')    # bass only (no guitar takes)
groove(8, 16, 'gallop', fill_every=8)
whistle(8, 16, vel=0.6)
line('hn_sus', 8, [(0, 'Eb5', 4)], vel=1.0)
line('tpt_sus', 8, [(0, 'Eb5', 4)], vel=0.8)
hit(T(8), ['Eb3', 'Bb3', 'Eb4'], 1.0)
for bb in (10,):               # the brass fanfare, answering the riff
    line('hn_stac', bb, FANFARE[:8], vel=0.95)
    line('tbn_stac', bb, FANFARE[:8], transpose=-12, vel=0.9)
    line('tpt_sus', bb, FANFARE[:8], vel=0.6, legato=0.8)
line('hn_sus', 11, [(3, 'F#5', 1), (4, 'E5', 4)], vel=0.95)
line('tbn_sus', 11, [(3, 'F#4', 1), (4, 'E4', 4)], vel=0.9)
line('hn_sus', 14, [(0, 'B4', 6), (6, 'Ab4', 2)], vel=0.85)
line('tbn_sus', 14, [(0, 'E4', 6), (6, 'E4', 2)], vel=0.8)
pad(8, 16, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.5)
ev['fx'].append(('revcym', T(15, 2), B(2), 0.7))

# ---- VERSE 16-31: chugs on the bass riff, the choir sings the melody.
crash(T(16), 1.0)
verse_gtr(16, 4, vel=0.9, tone='crunch', sides='L')
verse_gtr(20, 4, vel=0.9, tone='crunch', sides='R')
verse_gtr(24, 8, vel=0.9, accent='mute', step=1.0)
drop_gtr(27, 2, 4)
drop_gtr(31, 2, 4)
groove(16, 24, 'rock')
groove(24, 32, 'skank', fill_every=8)
whistle(16, 32, vel=0.5)
strings_riff(16, 32, 'vc_spic', 0, vel=0.6)
strings_riff(16, 32, 'cb_spic', -12, vel=0.5)
choir_line(16, CHOIR_MEL, vel=0.85, vowel='a')
choir_line(16, CHOIR_MEL, transpose=-12, vel=0.6, vowel='a')
pad(16, 32, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.35)
for b in range(16, 32, 4):
    orch('vln_sus', T(b), 'Eb6' if CHORDS[b] == 'Ebm' else 'E6', B(16) - 0.05, 0.45)
line('hn_stac', 26, FANFARE[:8], vel=0.85)
line('hn_sus', 27, [(3, 'F#5', 1), (4, 'E5', 4)], vel=0.85)
line('tbn_stac', 26, FANFARE[:8], transpose=-12, vel=0.8)
ev['fx'].append(('revcym', T(31, 2), B(2), 0.7))

# ---- PRE-CHORUS 32-47: open ringing chords on the bass line with its fills,
#      half-time drums, the high strings and the horn line.
crash(T(32), 1.0)
ev['fx'].append(('impact', T(32), 0, 0.6))
for bb in (32, 40):
    open_line(bb, [(b, n, l) for b, n, l in BASS_PRE], vel=0.95)
groove(32, 46, 'half', crash_every=4, fill_every=8)
groove(46, 47, 'double', crash_every=8, fill=False)
snare_roll(47, 4, 0.35, 1.0, div=8)
ev['timproll'].append((T(47), T(48) - 0.02, 'Bb2', 0.4, 1.0))
whistle(32, 48, vel=0.4)
line('vln_sus', 32, [(0, 'Eb6', 14), (14, 'F#6', 2), (16, 'Ab6', 12), (28, 'E6', 4)], vel=0.75)
line('vln_sus', 40, [(0, 'Eb6', 14), (14, 'F#6', 2), (16, 'Ab6', 12), (28, 'E6', 4)], vel=0.8)
line('hn_sus', 32, HORN_LINE, vel=0.85)
line('hn_sus', 40, HORN_LINE, vel=0.9)
line('tbn_sus', 40, HORN_LINE, transpose=-12, vel=0.7)
ev['glock'].extend([(T(32, b), mm(n) + 12, B(l), 0.6) for b, n, l in HORN_LINE])
choir_pad(32, 48, vel=0.5, vowel='o')
pad(32, 48, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.4)
ev['fx'].append(('riser', T(46), T(48) - T(46), 0.6))

# ---- BREAK 48-55: stop-time hits, tom fills in the gaps, horn fanfare.
for bb in (48, 52):
    for beat, ln, root in BREAK_HITS:
        t = T(bb, beat)
        gtr(t, 'open', root, B(ln) - 0.02, 1.0)
        bass(t, bass_of(root), B(ln) - 0.02, 1.0)
        kick(t, 1.0)
        crash(t, 0.85, side=int(beat) % 2)
        hit(t, [root.replace('1', '3').replace('2', '3'), 'Eb4' if root != 'Bb1' else 'F4',
                'Bb4' if root != 'Ab1' else 'Ab4'], 0.9)
        ev['timp'].append((t, 'Eb2' if root != 'Bb1' else 'Bb2', 0.8))
    line('tpt_sus', bb, BREAK_MEL, vel=0.85, legato=0.92)
    line('hn_sus', bb, BREAK_MEL, transpose=-12, vel=0.9, legato=0.92)
    # fills in the gaps
    for x in (2.0, 2.5, 3.0, 3.5):
        tom(T(bb, x), 0, 0.8)
        tom(T(bb, x + .25), 1, 0.7)
    for i in range(16):        # bar 2 of the phrase: double kick + chinas on quarters
        kick(T(bb + 1, i * .25), 0.8)
    for x in (0, 1, 2, 3):
        china(T(bb + 1, x), 0.7)
        snare(T(bb + 1, x + .5), 0.8)
    tom_fill(bb + 3, 1.0, 0.9)
    tom_fill(bb + 1, 3.0, 0.8, kind=1)
line('hn_sus', 55, [(2, 'Ab4', 2)], vel=0.9)
gtr(T(55, 2), 'open', 'Ab1', B(2) - 0.02, 1.0)
bass(T(55, 2), 'Ab1', B(2) - 0.02, 1.0)
whistle(48, 56, vel=0.45)

# ---- VERSE 2 56-71: the riff with the strings up an octave, the horn phrase,
#      the climb with its canon.
crash(T(56), 1.0)
ev['gong'].append((T(56), 0.5))
main_riff(56, 8, vel=1.0, sides='L', harm='R')
verse_gtr(64, 8, vel=0.95, tone='crunch')
drop_gtr(67, 2, 4)
drop_gtr(71, 1, 4)
groove(56, 64, 'skank', fill_every=8)
groove(64, 72, 'gallop', fill_every=8)
whistle(56, 72, vel=0.55)
strings_riff(56, 64, 'vln_spic', 12, vel=0.6)
strings_riff(56, 64, 'vla_spic', 0, vel=0.55)
line('hn_sus', 64, PHRASE_A, vel=0.95)
line('tpt_sus', 64, PHRASE_A, vel=0.65)
line('vln_sus', 68, CLIMB, transpose=0, vel=0.8)
line('hn_sus', 68, CANON, transpose=12, vel=0.85)
line('tbn_sus', 68, CANON, vel=0.8)
choir_line(56, [(0, 'Bb4', 16), (16, 'B4', 16)], vel=0.7, vowel='a')
choir_line(56, [(2, 'Eb4', 2), (4, 'F4', 2), (6, 'F#4', 2), (8, 'Eb4', 8), (16, 'E4', 16)], vel=0.6, vowel='o')
pad(56, 72, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.4)
ev['fx'].append(('revcym', T(71), B(4), 0.8))

# ---- ORCHESTRAL BRIDGE 72-79: guitars out. Strings, choir, glockenspiel
#      ostinato, taiko pulse; the march builds into the climax.
ev['fx'].append(('impact', T(72), 0, 0.7))
crash(T(72), 0.8)
ev['fx'].append(('downlifter', T(72), B(8), 0.5))
pad(72, 80, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.6)
choir_line(72, CHOIR_MEL[10:], vel=0.75, vowel='o')
choir_line(72, CHOIR_MEL[10:], transpose=-12, vel=0.5, vowel='o')
line('hn_sus', 72, CHOIR_MEL[10:], transpose=-12, vel=0.55)
for b in range(72, 80):
    notes = WHISTLE[CHORDS[b]]
    for i in range(8):
        ev['glock'].append((T(b, i * .5), mm(notes[i % 4]) + 12, B(.5), 0.45 if i % 2 else 0.6))
whistle(76, 80, vel=0.4)
for b in range(72, 80):
    ev['taiko'].append((T(b), 0.6))
    ev['taiko'].append((T(b, 1.5), 0.45))
    if b >= 76:
        ev['taiko'].append((T(b, 2.5), 0.4))
for b in range(76, 80):
    for i in range(8):
        ev['march'].append((T(b, i * .5), (0.35 + 0.15 * (b - 76)) * (1.0 if i % 2 == 0 else 0.75)))
strings_riff(76, 80, 'vc_spic', 0, vel=0.7)
snare_roll(79, 4, 0.3, 1.0, div=8)
ev['timproll'].append((T(78), T(80) - 0.02, 'Eb2', 0.2, 1.0))
ev['swell'].append((T(80), 'median', 0.9))
ev['fx'].append(('riser', T(76), T(80) - T(76), 0.7))
ev['fx'].append(('pitchriser', T(78), T(80) - T(78), 0.35))

# ---- CLIMAX 80-95: everything. Double kick, the lead guitar sings the horn
#      phrase, the descending line and the climb; choir and strings on top.
ev['gong'].append((T(80), 1.0))
ev['fx'].append(('impact', T(80), 0, 1.0))
crash(T(80), 1.0, 1)
climax_gtr(80, 16, vel=1.0)
groove(80, 96, 'double', crash_every=4, fill_every=4)
whistle(80, 96, vel=0.55)
strings_riff(80, 96, 'vc_spic', 0, vel=0.6)
ev['lead'].append((80, [(b, m(n) + 12, l) for b, n, l in PHRASE_A], 1.0))
ev['lead'].append((84, [(b, m(n) + 12, l) for b, n, l in DESCENT], 1.0))
ev['lead'].append((88, [(b, m(n) + 12, l) for b, n, l in PHRASE_A[:-3]] +
                   [(13.5, m('Eb5'), .25), (13.75, m('F#5'), .25), (14, m('Ab5'), .5),
                    (14.5, m('Bb5'), .5), (15, m('B5'), .5), (15.5, m('C#6'), .5)], 1.0))
ev['lead'].append((92, [(b, m(n), l) for b, n, l in CLIMB], 1.0))
line('vln_sus', 80, [(0, 'Bb6', 16), (16, 'B6', 16)], transpose=-12, vel=0.7)
line('vln_sus', 88, [(0, 'Bb6', 16)], transpose=-12, vel=0.75)
line('vln_spic', 90, SWEEP, vel=0.7)          # the original's arpeggio sweep
line('vla_spic', 90, SWEEP, transpose=-12, vel=0.6)
ev['glock'].extend([(T(90, b), mm(n) + 12, B(l), 0.5) for b, n, l in SWEEP if mm(n) >= m('B4')])
line('hn_sus', 92, CANON, transpose=12, vel=0.9)
line('tbn_sus', 92, CANON, vel=0.85)
line('vln_sus', 92, CLIMB, vel=0.75)
choir_line(80, [(0, 'Bb4', 16), (16, 'B4', 16), (32, 'Bb4', 16), (48, 'B4', 16)], vel=0.8, vowel='a')
choir_line(80, [(0, 'Eb4', 16), (16, 'E4', 16), (32, 'Eb4', 16), (48, 'E4', 16)], vel=0.65, vowel='a')
choir_line(80, [(0, 'Gb4', 16), (16, 'G#4', 16), (32, 'Gb4', 16), (48, 'G#4', 16)], vel=0.5, vowel='a')
pad(80, 96, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.55)
for bb in (80, 88):
    line('tba_sus', bb, [(0, 'Eb2', 16), (16, 'B1', 16)], vel=0.7)
ev['fx'].append(('revcym', T(95), B(4), 0.8))

# ---- ASCENT 96-103: Ebm E F# Abm in 3+3+2, the D pedal, choir in fifths.
for rep in range(2):
    for i, ch in enumerate(ASCENT):
        b = 96 + rep * 4 + i
        crash(T(b), 1.0, side=i % 2)
        root = ASC_ROOT[ch]
        for beat, ln in ((0, 1.5), (1.5, 1.5), (3, 1)):
            gtr(T(b, beat), 'open', root, B(ln) - 0.02, 1.0)
            bass(T(b, beat), bass_of(root), B(ln) - 0.02, 1.0)
            kick(T(b, beat), 1.0)
        for x in range(16):
            if x * .25 not in (0, 1.5, 3):
                kick(T(b, x * .25), 0.75)
        snare(T(b, 1), 1.0)
        snare(T(b, 3), 1.0)
        china(T(b, 2), 0.6)
        for n in ASC_BRASS[ch]:
            orch('hn_sus', T(b), n, B(4) - 0.05, 0.95)
            orch('tpt_sus', T(b), n, B(4) - 0.05, 0.75)
        for (beat, ln), n in zip(((0, 1.5), (1.5, 1.5), (3, 1)), ASC_LOW[ch]):
            orch('tbn_stac', T(b, beat), n, B(ln) * 0.8, 0.9)
            orch('vc_spic', T(b, beat), mm(n) - 12, B(ln) * 0.8, 0.8)
        orch('tbn_sus', T(b), ASC_TBN[ch], B(4) - 0.05, 0.85)
        orch('vln_sus', T(b), ASC_STR[ch], B(4) - 0.05, 0.85)
        orch('vln_sus', T(b), mm(ASC_STR[ch]) - 12, B(4) - 0.05, 0.75)
        ev['choir'].append((T(b), mm(ASC_CHOIR[ch]), B(4), 0.9, 'a', {}))
        ev['choir'].append((T(b), mm(ASC_CHOIR[ch]) - 7, B(4), 0.75, 'a', {}))
        ev['choir'].append((T(b), mm(ASC_CHOIR[ch]) - 12, B(4), 0.6, 'a', {}))
        ped = 'Eb3' if i < 2 else 'D3'
        for x in range(8):
            orch('vla_spic', T(b, x * .5), ped, B(.45), 0.7)
        whistle(b, b + 1, vel=0.55)
    ev['timp'].append((T(96 + rep * 4), 'Eb2', 1.0))
tom_fill(103, 2.0, 1.0)

# ---- STOP 104-105: the tritone hits on Bb/E and the run into the last riff.
for i, x in enumerate((0, .5, 1.5, 2, 3, 3.5, 4)):
    t = T(104, x)
    gtr(t, 'tritone', 'Bb1', B(.4), 1.0)
    bass(t, 'Bb1', B(.4), 1.0)
    kick(t, 1.0)
    (crash if i % 3 == 0 else china)(t, 0.8)
    for n in ('Bb3', 'E4', 'Bb4'):
        orch('hn_stac', t, n, B(.4), 0.95)
    orch('tbn_stac', t, 'Bb2', B(.4), 0.95)
    orch('tba_stac', t, 'Bb1', B(.4), 0.9)
    orch('vln_spic', t, 'E6', B(.4), 0.8)
ev['timp'].append((T(104), 'Bb2', 1.0))
ev['timp'].append((T(105), 'Bb2', 1.0))
gtr(T(105, 2), 'open', 'Bb1', B(2) - 0.02, 0.9)
bass(T(105, 2), 'Bb1', B(2) - 0.02, 1.0)
for x, n in zip((2, 2.5, 3, 3.5), ('E4', 'Ab4', 'Bb4', 'C#5')):
    for tr in (0, 12):
        orch('hn_stac', T(105, x), mm(n) + tr, B(.45), 1.0)
    orch('tbn_stac', T(105, x), mm(n) - 12, B(.45), 0.9)
    orch('vln_spic', T(105, x), mm(n) + 12, B(.45), 0.9)
    ev['lead'].append((105, [(x, m(n) + 12, .5)], 0.9))
    tom(T(105, x), int((x - 2) * 2), 1.0)
    snare(T(105, x + .25), 0.9)
ev['fx'].append(('revcym', T(105), B(4), 0.9))

# ---- FINAL RIFF 106-113: the main riff, the choir melody, everything.
ev['gong'].append((T(106), 1.0))
ev['fx'].append(('impact', T(106), 0, 1.0))
crash(T(106), 1.0, 1)
main_riff(106, 8, vel=1.0)
groove(106, 114, 'double', crash_every=2, fill_every=8)
whistle(106, 114, vel=0.6)
strings_riff(106, 114, 'vc_spic', 0, vel=0.65)
strings_riff(106, 114, 'vln_spic', 12, vel=0.55)
choir_line(106, CHOIR_MEL[:10], vel=0.95, vowel='a')
choir_line(106, CHOIR_MEL[:10], transpose=-12, vel=0.7, vowel='a')
line('hn_sus', 106, CHOIR_MEL[:10], vel=0.8)
line('tbn_sus', 106, CHOIR_MEL[:10], transpose=-12, vel=0.75)
line('vln_sus', 106, [(0, 'Eb6', 16), (16, 'E6', 16)], vel=0.75)
pad(106, 114, PAD_V, ['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], vel=0.5)
line('tba_sus', 106, [(0, 'Eb2', 16), (16, 'B1', 16)], vel=0.7)

# ---- ENDING 114-116: E (the flat second) for two bars, then one big
#      E-flat minor hit that rings out naturally (about 3 s).
crash(T(114), 1.0)
gtr(T(114), 'open', 'E2', B(7.5), 0.95)
bass(T(114), 'E1', B(7.5), 1.0)
for x in (0, 1.5, 3, 4, 5.5, 7):
    kick(T(114, x), 0.9)
for x in (3, 5.5, 7):
    snare(T(114, x), 0.9)
crash(T(115, 3), 0.8, 1)
for n in ('E4', 'G#4', 'B4'):
    ev['choir'].append((T(114), mm(n), B(8), 0.85, 'a', {}))
for inst, notes in zip(['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], PAD_V['E']):
    for n in notes:
        orch(inst, T(114), n, B(8) - 0.05, 0.75)
orch('hn_sus', T(114), 'E5', B(4), 0.9)
orch('hn_sus', T(114, 4), 'Eb5', B(4) - 0.05, 0.9)   # leaning down into E-flat
orch('tbn_sus', T(114), 'B3', B(8) - 0.05, 0.8)
# the final E-flat minor hit
t_end = T(116)
ev['gong'].append((t_end, 1.0))
ev['fx'].append(('impact', t_end, 0, 1.0))
crash(t_end, 1.0, 0)
crash(t_end, 0.9, 1)
china(t_end, 0.8)
kick(t_end, 1.0)
snare(t_end, 1.0)
gtr(t_end, 'open', 'Eb2', B(7), 1.0)
bass(t_end, 'Eb1', B(7), 1.0)
ev['timp'].append((t_end, 'Eb2', 1.0))
for n in ('Eb4', 'Gb4', 'Bb4', 'Eb5'):
    ev['choir'].append((t_end, mm(n), B(6), 0.9, 'a', {}))
for inst, notes in zip(['vln_sus', 'vla_sus', 'vc_sus', 'cb_sus'], PAD_V['Ebm']):
    for n in notes:
        orch(inst, t_end, n, B(6), 0.85)
orch('vln_sus', t_end, 'Eb6', B(6), 0.8)
for n in ('Eb4', 'Bb4', 'Gb4'):
    orch('hn_sus', t_end, n, B(6), 1.0)
orch('tbn_sus', t_end, 'Eb3', B(6), 0.95)
orch('tba_sus', t_end, 'Eb2', B(6), 0.9)
orch('tpt_sus', t_end, 'Bb4', B(6), 0.8)
hit(t_end, ['Eb3', 'Bb3', 'Eb4'], 1.0)


# ================================================================ rendering
for k in ev:
    ev[k] = [e for e in ev[k] if True]


def tt(t):
    """Absolute song time -> time in the rendered window."""
    return t - OFFSET


def in_window(t, length=8.0):
    return tt(t) > -length and tt(t) < N / SR


meter = pyln.Meter(SR)
mix = np.zeros((2, N), np.float32)
send_hall = np.zeros((2, N), np.float32)
send_room = np.zeros((2, N), np.float32)
send_delay = np.zeros((2, N), np.float32)
levels = {}
sec_levels = {}
SECTIONS = {'intro': (0, 8), 'riff': (8, 16), 'verse': (16, 32), 'pre': (32, 48), 'break': (48, 56),
            'verse2': (56, 72), 'bridge': (72, 80), 'climax': (80, 96), 'ascent': (96, 104),
            'final': (106, 114), 'ending': (114, 117)}


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


GAIN = {  # dB
    'gtr': -21.0, 'lead': -21.0, 'bass': 5.5, 'kick': -3.5, 'snare': 2.5, 'toms': -10.0,
    'cym': -5.0, 'perc': -1.0, 'strings': -6.5, 'spic': -4.5, 'brass': -9.5, 'choir': -22.0,
    'glock': -14.0, 'fx': -16.0,
}
HALL = {'lead': .18, 'strings': .35, 'spic': .25, 'brass': .3, 'choir': .45, 'glock': .4, 'perc': .3,
        'fx': .3, 'gtr': .04}
ROOM = {'snare': .22, 'toms': .25, 'kick': .04, 'cym': .1}
DELAY = {'lead': .25, 'glock': .15}


def commit(name, x):
    """Gain, measure, send and add one finished bus to the mix, then free it."""
    global mix
    if name in MUTE:
        return
    x = x * np.float32(10 ** (GAIN[name] / 20))
    seg = x.T.astype(np.float64)
    active = np.max(np.abs(x), axis=0) > 1e-4
    levels[name] = meter.integrated_loudness(seg[active]) if np.sum(active) > SR else -99.0
    del seg
    if name in ('lead', 'choir', 'strings', 'brass', 'spic', 'kick', 'snare'):
        for sn, (b0, b1) in SECTIONS.items():
            a, b = int((T(b0) - OFFSET) * SR), int((T(b1) - OFFSET) * SR)
            if a >= 0 and b <= N and np.max(np.abs(x[:, a:b])) > 1e-4:
                sec_levels.setdefault(sn, {})[name] = meter.integrated_loudness(x[:, a:b].T.astype(np.float64))
    mix += x
    if name in HALL:
        send_hall[:] += x * HALL[name]
    if name in ROOM:
        send_room[:] += x * ROOM[name]
    if name in DELAY:
        send_delay[:] += x * DELAY[name]
    gc.collect()


def add_mono(buf, sig, t, gain=1.0):
    i = int(round(tt(t) * SR))
    if i >= len(buf) or i + len(sig) <= 0:
        return
    if i < 0:
        sig = sig[-i:]
        i = 0
    k = min(len(sig), len(buf) - i)
    buf[i:i + k] += (sig[:k] * gain).astype(np.float32)


def add_st(bus, sig, t, gain=1.0):
    i = int(round(tt(t) * SR))
    if i >= bus.shape[1] or i + sig.shape[1] <= 0:
        return
    if i < 0:
        sig = sig[:, -i:]
        i = 0
    k = min(sig.shape[1], bus.shape[1] - i)
    bus[:, i:i + k] += (sig[:, :k] * gain).astype(np.float32)


def pan(sig, p):
    return np.stack([sig * np.sqrt((1 - p) / 2), sig * np.sqrt((1 + p) / 2)]).astype(np.float32)


# ---------------------------------------------------------- rhythm guitars
# rhythm-guitar level automation (dB at bar lines): the pre-chorus (bars
# 32-47) is the reference; the repeated riffs before the double-kick climax
# sit lower so the melody, strings and choir lead.
GTR_DB = [(0, -3.5), (16, -3.5), (16.01, -2.0), (24, -2.0), (24.01, -4.0), (31.99, -4.0), (32, 0.0),
          (47.99, 0.0), (48, -2.0), (55.99, -2.0), (56, -3.5), (63.99, -3.5), (64, -2.5), (79.5, -2.5),
          (80, 0.0), (120, 0.0)]
TAKES = {  # take: (pan, level, cabinet variant, amp gain) per tone
    'full': [(-1.0, 1.0, 0, 0.7), (1.0, 1.0, 1, 0.72), (-0.55, 0.5, 2, 0.6), (0.55, 0.5, 3, 0.62)],
    'crunch': [(-0.9, 1.0, 2, 0.22), (0.9, 1.0, 3, 0.24)],
}
TAKE_SIDE = {'full': ['L', 'R', 'L', 'R'], 'crunch': ['L', 'R']}


def render_guitars():
    out = np.zeros((2, N), np.float32)
    for tone, takes in TAKES.items():
        for take, (p, g, var, gain_) in enumerate(takes):
            side = TAKE_SIDE[tone][take]
            evs = [e for e in ev['gtr'] if e[5] == tone and side in e[6] and in_window(e[0], 10)]
            if not evs:
                continue
            r = np.random.default_rng(600 + take + (10 if tone == 'crunch' else 0))
            di = np.zeros(N, np.float32)
            lag = r.normal(0, 0.003)
            for t, kind, root, dur, vel, _, _ in evs:
                tj = t + lag + r.normal(0, 0.0035)
                if kind == 'tritone':
                    y = M.power_chord(root, dur, vel, 'fifth', r=r)
                    y += M.power_chord(root + 6, dur, vel * 0.9, 'single', r=r)
                else:
                    y = M.power_chord(root, dur, vel * (0.95 + 0.08 * r.random()), kind, r=r)
                add_mono(di, y, tj)
            if tone == 'crunch':
                y = M.amp(di, gain=gain_, variant=var, tight=120.0, mid_boost=5.0, scoop=-1.0, presence=1.0,
                          low=4.0)
            else:
                y = M.amp(di, gain=gain_, variant=var)
            del di
            out += pan(y, p) * g * np.float32(3.5 if tone == 'crunch' else 1.0)
            del y
            gc.collect()
    out = stage(out, HighpassFilter(85), LowpassFilter(12000), PeakFilter(220, 2.0, 0.8))
    bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
    out *= (10 ** (np.interp(bars_axis, [q[0] for q in GTR_DB], [q[1] for q in GTR_DB]) / 20)).astype(np.float32)
    # per-section loudness of the guitar bus (for balancing)
    for name, (b0, b1) in SECTIONS.items():
        a, b = int((T(b0) - OFFSET) * SR), int((T(b1) - OFFSET) * SR)
        if a >= 0 and b <= N and np.any(out[:, a:b]):
            sec_levels.setdefault(name, {})['gtr'] = meter.integrated_loudness(
                (out[:, a:b] * np.float32(10 ** (GAIN['gtr'] / 20))).T.astype(np.float64))
    return out


def render_lead():
    di = np.zeros(N, np.float32)
    r = np.random.default_rng(77)
    for bar0, notes, vel in ev['lead']:
        if not in_window(T(bar0), 40):
            continue
        y, t0 = M.lead_phrase(notes, SPB, vel, r=r)
        add_mono(di, y, T(bar0) + t0)
    if not np.any(di):
        return np.zeros((2, N), np.float32)
    y = M.amp(di, gain=0.55, variant=4, tight=250.0, mid_boost=8.0, scoop=-1.0, presence=3.0, low=0.0)
    y = stage(y[None, :], HighpassFilter(180), PeakFilter(1800, 2.0, 0.8), LowpassFilter(9000))[0]
    return pan(y, 0.0)


def render_bass():
    buf = np.zeros(N, np.float32)
    r = np.random.default_rng(55)
    cache = {}
    for t, md, dur, vel in ev['bass']:
        if not in_window(t, 10):
            continue
        key = (md, round(dur, 3), r.integers(3))
        if key not in cache:
            cache[key] = M.bass_note(md, dur, 1.0, r=r).astype(np.float32)
        add_mono(buf, cache[key], t + r.normal(0, 0.002), vel)
    y = stage(buf[None, :], HighpassFilter(42), LowShelfFilter(90, -4.0, 0.7), PeakFilter(700, 3.0, 0.8),
              Compressor(threshold_db=-18, ratio=4, attack_ms=5, release_ms=80), LowpassFilter(5000))[0]
    return pan(y, 0.0)


def render_drums():
    r = np.random.default_rng(99)
    # kick: a few round robins, short variant for fast double-kick passages
    kicks = [M.metal_kick(1.0, r=r).astype(np.float32) for _ in range(4)]
    kbuf = np.zeros(N, np.float32)
    times = sorted(e[0] for e in ev['kick'])
    for t, vel in ev['kick']:
        if not in_window(t, 2):
            continue
        y = kicks[r.integers(4)]
        i = np.searchsorted(times, t)
        nxt = times[i + 1] if i + 1 < len(times) else 1e9
        if nxt - t < 0.2:      # choke the body under fast runs
            k = int((nxt - t + 0.03) * SR)
            y = y.copy()
            y[k:] = 0
            y[max(0, k - 480):k] *= np.linspace(1, 0, min(480, k), dtype=np.float32)
        add_mono(kbuf, y, t, vel)
    kick_st = pan(kbuf, 0.0)
    del kbuf
    kick_st = stage(kick_st, HighpassFilter(40), PeakFilter(58.3, 0.5, 1.2), PeakFilter(400, -4, 1.0),
                    PeakFilter(4000, 3.0, 1.0))
    commit('kick', kick_st)
    del kick_st
    # snare
    sn = M.Snare()
    sbuf = np.zeros(N, np.float32)
    for t, vel, ghost in ev['snare']:
        if not in_window(t, 2):
            continue
        add_mono(sbuf, sn.hit(1.0, r=r, ghost=ghost).astype(np.float32), t, vel * (0.5 if ghost else 1.0))
    s_st = pan(sbuf, 0.05)
    del sbuf
    s_st = stage(s_st, HighpassFilter(90), PeakFilter(200, 2.0, 1.0), PeakFilter(5000, 2.0, 0.8),
                 Compressor(threshold_db=-14, ratio=3, attack_ms=3, release_ms=60))
    commit('snare', s_st)
    del s_st
    # toms
    tb = np.zeros((2, N), np.float32)
    for t, md, vel, i in ev['tom']:
        if not in_window(t, 2):
            continue
        add_st(tb, pan(M.metal_tom(md, 1.0, r=r).astype(np.float32), [-0.5, -0.15, 0.25, 0.55][i]), t, vel)
    commit('toms', stage(tb, HighpassFilter(60)))
    del tb
    # cymbals: VSCO suspended-cymbal stick hits layered with synthesized crashes
    cb = np.zeros((2, N), np.float32)
    cr = [M.load_one(f'{M.VSCO}/Percussion/susCymb1-hitstick_fff_rr{i}.wav')[:, :int(4.5 * SR)] for i in (1, 2)]
    for c in cr:
        c[:, -int(1.0 * SR):] *= np.linspace(1, 0, int(1.0 * SR), dtype=np.float32)
    syn_cr = [S.crash(1.0, 3.0).astype(np.float32) for _ in range(2)]
    for t, vel, side in ev['crash']:
        if not in_window(t, 5):
            continue
        smp = cr[side % 2]
        p = -0.45 if side % 2 == 0 else 0.45
        y = np.stack([smp[0] * np.sqrt((1 - p) / 2), smp[1] * np.sqrt((1 + p) / 2)]) * 0.9
        add_st(cb, y, t, vel)
        sc = syn_cr[side % 2]
        add_st(cb, np.stack([sc[0] * np.sqrt((1 - p) / 2), sc[1] * np.sqrt((1 + p) / 2)]), t, vel * 0.35)
    chs = [M.china(1.0, r=r).astype(np.float32) for _ in range(3)]
    for t, vel in ev['china']:
        if not in_window(t, 3):
            continue
        y = chs[r.integers(3)]
        add_st(cb, np.stack([y[0] * 0.95, y[1] * 0.55]), t, vel * 0.8)
    for t, vel, op in ev['hat']:
        if not in_window(t, 1):
            continue
        add_st(cb, S.hat(1.0, op, 0.3).astype(np.float32), t, vel * 0.6)
    for t, vel in ev['ride']:
        if not in_window(t, 2):
            continue
        add_st(cb, S.ride(1.0, 0.35).astype(np.float32), t, vel * 0.55)
    commit('cym', stage(cb, HighpassFilter(350), HighShelfFilter(9000, -2.0, 0.7)))
    del cb


# --------------------------------------------------------- orchestral perc
def render_perc():
    pb = np.zeros((2, N), np.float32)
    P = M.VSCO + '/Percussion/'
    timp = {'Eb2': M.repitch(M.load_one(P + 'Timpani/Timpani1_Hit_v3_rr2_Sum.wav'), -2.39),
            'Bb2': M.repitch(M.load_one(P + 'Timpani/Timpani2_Hit_v4_rr1_Sum.wav'), -0.51)}
    for t, note, vel in ev['timp']:
        if in_window(t, 8):
            add_st(pb, timp[note][:, :int(6 * SR)], t, vel * 0.9)
    rolls = {'Eb2': M.repitch(M.load_one(P + 'Timpani/Rolls/Timpani1_Roll_v5_rr1_Sum.wav')[:, :int(14 * SR)], -2.9),
             'Bb2': M.repitch(M.load_one(P + 'Timpani/Rolls/Timpani2_Roll_v5_rr1_Sum.wav')[:, :int(14 * SR)], -0.9)}
    for t, length, note, v0, v1 in ev['timproll']:
        if not in_window(t, length + 2):
            continue
        y = rolls[note][:, int(0.5 * SR):int(0.5 * SR) + int((length + 0.3) * SR)].copy()
        k = y.shape[1]
        u = np.linspace(0, 1, k, dtype=np.float32)
        env = v0 + (v1 - v0) * u ** 1.5
        env[-int(0.3 * SR):] *= np.linspace(1, 0, int(0.3 * SR), dtype=np.float32)
        add_st(pb, y * env, t, 0.8)
    taiko = [M.load_one(P + f'BDrumNewhit_v7_rr{i}_Sum.wav')[:, :int(3.5 * SR)] for i in (1, 2)]
    for i, (t, vel) in enumerate(ev['taiko']):
        if in_window(t, 4):
            add_st(pb, taiko[i % 2], t, vel * 0.8)
    gong = M.load_one(P + 'gongHit_fff.wav')[:, :int(6 * SR)]
    gong[:, -2 * SR:] *= np.linspace(1, 0, 2 * SR, dtype=np.float32)
    for t, vel in ev['gong']:
        if in_window(t, 12):
            add_st(pb, gong, t, vel * 0.5)
    mar = [M.load_one(M.VSCO + f'/VSCO 1 Percussion/drums/snare/drum3_marching/snare3_f_{i}.wav') for i in (1, 2)]
    for i, (t, vel) in enumerate(ev['march']):
        if in_window(t, 1):
            add_st(pb, mar[i % 2] * np.float32(0.55), t + DR.normal(0, 0.003), vel)
    sw = {k: M.load_one(P + f'susCymb1-cresc-{k.capitalize()}_v1.wav') for k in ('short', 'median')}
    for t, kind, vel in ev['swell']:
        y = sw[kind]
        env = np.max(np.abs(y), axis=0)
        pk = int(np.argmax(env))
        y = y[:, :pk + int(0.15 * SR)].copy()
        y[:, -int(0.15 * SR):] *= np.linspace(1, 0, int(0.15 * SR), dtype=np.float32)
        add_st(pb, y, t - pk / SR, vel * 0.6)
    return stage(pb, HighpassFilter(30))


# ------------------------------------------------------------- orchestra
ORCH = {
    'vln_sus': ('Strings/Violin Section/susVib', 0.35, 'strings', -0.45, None),
    'vla_sus': ('Strings/Viola Section/susvib', 0.35, 'strings', 0.15, None),
    'vc_sus': ('Strings/Cello Section/susvib', 0.35, 'strings', 0.4, None),
    'cb_sus': ('Strings/Solo Contrabass/SusVib', 0.35, 'strings', 0.55, None),
    'vln_spic': ('Strings/Violin Section/Spic', 0.12, 'spic', -0.5, None),
    'vla_spic': ('Strings/Viola Section/spic', 0.12, 'spic', 0.2, None),
    'vc_spic': ('Strings/Cello Section/spic', 0.12, 'spic', 0.45, None),
    'cb_spic': ('Strings/Solo Contrabass/Spic', 0.12, 'spic', 0.6, None),
    'hn_sus': ('Brass/F Horn/sus', 0.25, 'brass', -0.25, (2, 3)),
    'hn_stac': ('Brass/F Horn/stac', 0.12, 'brass', -0.25, (2, 3)),
    'tbn_sus': ('Brass/Tenor Trombone/sus', 0.2, 'brass', 0.3, (2, 3)),
    'tbn_stac': ('Brass/Tenor Trombone/stac', 0.12, 'brass', 0.3, (3, 4)),
    'tba_sus': ('Brass/Tuba/sus', 0.25, 'brass', 0.4, (2, 3)),
    'tba_stac': ('Brass/Tuba/stac', 0.12, 'brass', 0.4, (2,)),
    'tpt_sus': ('Brass/Trumpet/sus', 0.2, 'brass', -0.1, (3,)),
}


def render_orchestra(group):
    bus = np.zeros((2, N), np.float32)
    r = np.random.default_rng(31)
    for inst, (folder, rel, grp, p, layers) in ORCH.items():
        if grp != group:
            continue
        notes = [e for e in ev['orch'] if e[0] == inst and in_window(e[1], e[3] + 3)]
        if not notes:
            continue
        smp = M.Sampler(folder, release=rel, max_len=7.0 if 'sus' in inst else 3.0,
                        layers=layers)
        part = np.zeros((2, N), np.float32)
        for _, t, md, dur, vel, kw in notes:
            # long notes beyond the sample: chain overlapping re-bows
            seg = 6.0
            k = 0
            while True:
                d = min(dur - k * seg, seg + 0.4)
                y = smp.note(md, d, vel, r=r, attack=0.08 if k else 0.0)
                add_st(part, y, t + k * seg + r.normal(0, 0.004), 1.0 if k == 0 else 0.9)
                k += 1
                if dur - k * seg <= 0.2:
                    break
        mono = part.mean(axis=0)
        width = part - mono[None, :]
        part = pan(mono, p) * np.float32(1.4) + width * np.float32(0.6)
        bus += part
        del part, smp, mono, width
        gc.collect()
    return bus


def render_choir():
    bus = np.zeros((2, N), np.float32)
    r = np.random.default_rng(44)
    cache = {}
    for t, md, dur, vel, vowel, kw in ev['choir']:
        if not in_window(t, dur + 3):
            continue
        key = (md, round(dur, 2), vowel)
        if key not in cache:
            cache[key] = M.choir(md, dur, 1.0, vowel=vowel, voices=6, a=0.18, r_=0.5, rr=r).astype(np.float32)
        add_st(bus, cache[key], t - 0.04, vel)
    return stage(bus, HighpassFilter(160), PeakFilter(2800, -2.0, 1.0), LowpassFilter(9000))


def render_glock():
    bus = np.zeros((2, N), np.float32)
    g = M.Sampler('Percussion/Glock', release=0.6, max_len=3.0)
    r = np.random.default_rng(12)
    for t, md, dur, vel in ev['glock']:
        if in_window(t, 4):
            add_st(bus, g.note(md, max(dur, 0.6), vel, r=r), t, 1.0)
    return bus


def render_fx():
    bus = np.zeros((2, N), np.float32)
    for kind, t, length, vel in ev['fx']:
        if not in_window(t, length + 3):
            continue
        if kind == 'riser':
            add_st(bus, S.noise_riser(length, 300, 9000, 2.0), t, vel)
        elif kind == 'revcym':
            add_st(bus, S.reverse_cymbal(length), t, vel * 0.8)
        elif kind == 'impact':
            add_st(bus, S.impact(vel), t, 1.0)
        elif kind == 'downlifter':
            add_st(bus, S.downlifter(length), t, vel)
        elif kind == 'pitchriser':
            add_st(bus, S.pitch_riser(length, 39, 63), t, vel)
    return stage(bus, HighpassFilter(40))


# ------------------------------------------------------------- render all
if 'gtr' not in MUTE:
    commit('gtr', render_guitars())
if 'lead' not in MUTE:
    commit('lead', render_lead())
if 'bass' not in MUTE:
    commit('bass', render_bass())
if not {'kick', 'snare', 'toms', 'cym'} <= MUTE:
    render_drums()
if 'perc' not in MUTE:
    commit('perc', render_perc())
for grp in ('strings', 'spic', 'brass'):
    if grp not in MUTE:
        commit(grp, render_orchestra(grp))
if 'choir' not in MUTE:
    commit('choir', render_choir())
if 'glock' not in MUTE:
    commit('glock', render_glock())
if 'fx' not in MUTE:
    commit('fx', render_fx())

# ------------------------------------------------------------- returns
pre = int(0.025 * SR)
vs = np.zeros_like(send_hall)
vs[:, pre:] = send_hall[:, :-pre]
hall = stage(vs, HighpassFilter(300), LowpassFilter(8000),
             Reverb(room_size=0.9, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del vs, send_hall
room = stage(send_room, HighpassFilter(250), LowpassFilter(9000),
             Reverb(room_size=0.45, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del send_room
dly = S.pingpong(send_delay, B(0.75), feedback=0.38, repeats=6, lp=4500, hp=500)
del send_delay
mix = mix + hall * np.float32(0.8) + room * np.float32(0.7) + dly * np.float32(0.5)
del hall, room, dly
gc.collect()

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -3.0), (7.5, -1.5), (8, 0), (16, -1.5), (32, -1.5), (47.9, -0.5), (48, 0), (56, -0.5),
              (72, -0.5), (72.01, -3.0), (78, -2.5), (79.9, -1.5), (80, 1.0), (96, 1.0), (104, 1.5), (106, 1.5),
              (114, 0.5), (116, 1.0), (120, 1.0)]
bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-13.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(80, -1.0, 0.7), HighShelfFilter(10000, -1.0, 0.7),
            Compressor(threshold_db=-12, ratio=2.0, attack_ms=25, release_ms=180))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=100))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(1.2 * SR)       # a short fade on the final ring-out
mix[:, -fade:] *= (0.5 + 0.5 * np.cos(np.linspace(0, np.pi, fade)))[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song6_symphonic_metal.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
print('per-section LUFS:')
for sn, d in sec_levels.items():
    print(f'  {sn:8s} ' + '  '.join(f'{k} {v:6.1f}' for k, v in sorted(d.items())))
