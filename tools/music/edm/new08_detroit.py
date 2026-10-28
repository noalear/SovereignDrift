#!/usr/bin/env python3
# new08_detroit.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Long Haul to Kepler Gate": a Detroit / space techno track (1993-99
# style: a hypnotic TR-909 groove, dub chord stabs with echo, a deep bass,
# a soaring string machine and an FM electric piano, filter automation),
# a NEW composition built from under-used material of the original game's
# song 8 (music by Stardock, mid/sf-music8.mid; B minor, 200 BPM).  The
# existing remix (song8_dubstep.py) features the riff, the main melody, the
# chant and the descents; this track is built from fragments it left out or
# buried (bar numbers are the MIDI's):
#   A. the fretless-bass fill of the tension passage, bars 23/27/31/35:
#      F2 B2 F3 B2 B1 / F#2 C#3 F#3 C#3 C#2 in the rhythm 1 . 1/2 . 1 . 1 . 1/2
#      (root, fifth, octave, fifth, low fifth).  It becomes the deep bassline
#      of the whole track, re-rooted per chord (B1 F#2 B2 F#2 F#1 ...); over
#      F#7 it is played as written (bar 27); its tritone form (bar 23,
#      B1 F2 B2 F2 F1) returns once under a Bm7b5 stab, the original's
#      Bdim/F tension harmony (bars 21-24).
#   B. the piano inner voice of the riff, bars 5-20 (ch 35): D4 | E4 E4 | D4 |
#      E4.  It is the moving bottom voice of the dub stabs (Bm9 -> Em9: D4 ->
#      E4, with the Em bars re-struck on beat 3 as in the original's "E4 E4").
#   C. the rising tremolo-string line of bars 57-60 (ch 18/13): B4 | D5 E5 |
#      F#5 | A#5, with its harmony (F# to A#: the leading tone).  It becomes the
#      soaring string theme over a new i-iv-VI-V progression (Bm9 Em9 Gmaj9
#      F#7), developed by inversion (B5 G5 F#5 D5 C#5 A#4), by extension
#      (... F#5 A5 A#5) and to a peak (B5 C#6 D6 C#6 A#5).
#   D. the ending's rising inner voices over the E pedal, bars 81-84: strings
#      G3 . A3 | B3 | C#4 and choir B3 . C#4 | D4 | E4 (parallel thirds), under
#      the piano's G5 pedal (ch 36, bars 77-88).  They are the electric-piano
#      chords of the breakdown (Em -> Em6, Dorian), with the G5 pedal on top.
# New: 132 BPM, the 909 kit (kick tuned to B1, crisp hats, a soft round clap,
# ride, 808 congas), the form (intro, groove A, groove B with the theme,
# breakdown, build, peak, dark turn, outro, a final Bm9 hit that rings out and
# loops back to the start), filter automation on the stabs and strings.
# Everything is synthesized (synth.py + synth_new08.py); no samples.
#
#   python3 tools/music/edm/new08_detroit.py OUT.wav
#   env NEW08_MUTE=kick,clap,...  mutes buses (for chroma checks)
#   env NEW08_BARS=a,b            renders only bars a..b-1 (while iterating)
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
import synth_new08 as X  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 132.0
SPB = 60.0 / BPM
NBARS = 104
TAIL = 6.0
_bars = os.environ.get('NEW08_BARS')
BAR0, BAR1 = (int(v) for v in _bars.split(',')) if _bars else (0, NBARS)
OFF = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(t, length=8.0):
    return t + length > OFF and t < OFF + N / SR


rng = np.random.default_rng(1993)


def hv(v, amt=0.06):
    return float(np.clip(v * (1 + amt * (rng.random() * 2 - 1)), 0.03, 1.2))


# ------------------------------------------------------------- harmony

VOICE = {   # dub-stab / e-piano voicings (rootless; the bass has the root)
    'Bm9':   ['D4', 'F#4', 'A4', 'C#5'],
    'Em9':   ['E4', 'F#4', 'G4', 'B4'],      # fragment B: bottom voice D4 -> E4
    'Gmaj9': ['D4', 'F#4', 'A4', 'B4'],
    'F#7':   ['E4', 'F#4', 'A#4', 'C#5'],
    'Bm7b5': ['D4', 'F4', 'A4', 'B4'],       # the original's Bdim/F tension colour
}
PADV = {    # string-machine pad voicings (open, below the stabs)
    'Bm9': ['B2', 'F#3', 'D4'], 'Em9': ['E3', 'B3', 'G4'], 'Gmaj9': ['D3', 'B3', 'F#4'],
    'F#7': ['C#3', 'A#3', 'E4'], 'Bm7b5': ['B2', 'F3', 'D4'],
}
PCS = {     # expected pitch classes per bar (chroma check)
    'Bm9': 'B D F# A C#', 'Em9': 'E G B D F#', 'Gmaj9': 'G B D F# A', 'F#7': 'F# A# C# E',
    'Bm7b5': 'B D F A', 'E1': 'E G B', 'E2': 'E G B A C#', 'E3': 'E B D G', 'E4': 'E C# G',
}
BASS_ROOT = {'Bm9': 'B1', 'Em9': 'E2', 'Gmaj9': 'G1', 'F#7': 'F#2', 'Bm7b5': 'B1'}

VAMP = ['Bm9', 'Em9'] * 2
PROG = ['Bm9', 'Em9', 'Gmaj9', 'F#7']
RISE = ['E1', 'E2', 'E3', 'E4']
CHORDS = (['Bm9'] * 8 +            # 0-7    intro: drums, drone
          VAMP * 2 +               # 8-15   intro: bass and stabs enter
          VAMP * 4 +               # 16-31  groove A
          PROG * 4 +               # 32-47  groove B: the string theme
          RISE * 2 +               # 48-55  breakdown: the E-pedal rise
          ['Gmaj9', 'Gmaj9', 'F#7', 'F#7'] +   # 56-59 breakdown: theme returns
          PROG +                   # 60-63  build
          PROG * 4 +               # 64-79  peak
          VAMP + ['Bm9', 'Em9', 'Bm9', 'Bm7b5'] +   # 80-87 dark turn
          VAMP * 2 +               # 88-95  outro
          PROG +                   # 96-99  last statement
          ['Bm9', 'Em9', 'F#7', 'Bm9'])            # 100-103 cadence, final hit
assert len(CHORDS) == NBARS, len(CHORDS)

# fragment A: the bass fill rhythm (beat, length) and contour per chord
FILL_RHY = [(0, 1.0), (1, 0.5), (1.5, 1.0), (2.5, 1.0), (3.5, 0.5)]
FILL = {
    'Bm9':   ['B1', 'F#2', 'B2', 'F#2', 'F#1'],
    'Em9':   ['E2', 'B2', 'E3', 'B2', 'B1'],
    'Gmaj9': ['G1', 'D2', 'G2', 'D2', 'F#2'],     # last note leads into F#
    'F#7':   ['F#2', 'C#3', 'F#3', 'C#3', 'C#2'],  # bar 27, as written
    'Bm7b5': ['B1', 'F2', 'B2', 'F2', 'F1'],       # bar 23's tritone form
}

# fragment C: the string theme (beat, note, length) over PROG x4
THEME_P1 = [(0, 'B4', 4), (4, 'D5', 2), (6, 'E5', 2), (8, 'F#5', 4), (12, 'A#5', 4)]
THEME_P2 = [(0, 'B5', 4), (4, 'G5', 2), (6, 'F#5', 2), (8, 'D5', 4), (12, 'C#5', 2), (14, 'A#4', 2)]
THEME_P3 = [(0, 'B4', 4), (4, 'D5', 2), (6, 'E5', 2), (8, 'F#5', 2), (10, 'A5', 2), (12, 'A#5', 4)]
THEME_P4 = [(0, 'B5', 6), (6, 'C#6', 2), (8, 'D6', 4), (12, 'C#6', 2), (14, 'A#5', 2)]
THEME = THEME_P1 + [(b + 16, n, l) for b, n, l in THEME_P2] + \
    [(b + 32, n, l) for b, n, l in THEME_P3] + [(b + 48, n, l) for b, n, l in THEME_P4]
# a lower harmony line for the peak (thirds / sixths under the theme)
THEME_HARM = [(0, 'F#4', 4), (4, 'B4', 2), (6, 'B4', 2), (8, 'D5', 4), (12, 'E5', 2), (14, 'F#5', 2),
              (16, 'F#5', 4), (20, 'E5', 2), (22, 'D5', 2), (24, 'B4', 4), (28, 'A#4', 2), (30, 'F#4', 2),
              (32, 'F#4', 4), (36, 'B4', 2), (38, 'B4', 2), (40, 'D5', 2), (42, 'F#5', 2), (44, 'F#5', 4),
              (48, 'F#5', 6), (54, 'G5', 2), (56, 'A5', 4), (60, 'A#5', 2), (62, 'F#5', 2)]

# fragment D: rising inner thirds over the E pedal (beat, notes, length), G5 pedal on top
RISE_CH = [(0, ['E3', 'G3', 'B3'], 6), (6, ['E3', 'A3', 'C#4'], 2), (8, ['E3', 'B3', 'D4'], 4),
           (12, ['E3', 'C#4', 'E4'], 4)]

# --------------------------------------------------------------- buses
names = ['kick', 'clap', 'hats', 'ride', 'perc', 'cym', 'bass', 'sub', 'stab', 'strings', 'theme',
         'rhodes', 'lead', 'fx']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def put(name, t, fn, gain=1.0, length=8.0, key=None):
    if not live(t, length):
        return
    if key is not None:
        if key not in _cache:
            _cache[key] = fn()
        sig = _cache[key]
    else:
        sig = fn()
    bus[name].add(sig, t - OFF, gain)


# ------------------------------------------------------------- drums

def place_kick(bar, beat, vel=1.0):
    t = T(bar, beat)
    put('kick', t, lambda: X.kick909(1.0), vel, length=1, key=('kick',))
    if OFF <= t < OFF + N / SR:
        kicks.append(t - OFF)


def four(bar0, bar1, vel=1.0, drop_last=None):
    """Four on the floor; drop_last: bars (by index % 8 == 7 etc.) whose beat 4 is left out."""
    for b in range(bar0, bar1):
        for beat in range(4):
            if drop_last and b % drop_last == drop_last - 1 and beat == 3:
                continue
            place_kick(b, beat, vel * (1.0 if beat in (0, 2) else 0.95))


def claps(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        for beat in (1, 3):
            v = int(rng.integers(4))
            put('clap', T(b, beat), lambda: X.clap909(1.0, v), hv(vel, 0.05), length=1, key=('clap', v))


def hats(bar0, bar1, closed=0.3, opn=0.5, swing=0.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            if opn:
                v = int(rng.integers(3))
                put('hats', T(b, beat + .5), lambda: X.hat909(1.0, True, v, 0.12), hv(opn, 0.06), length=1,
                    key=('oh', v))
            if closed:
                for s16, acc in ((0, .45), (.25, .75), (.75, .9)):
                    v = int(rng.integers(4))
                    sw = swing if s16 in (.25, .75) else 0
                    put('hats', T(b, beat + s16 + sw), lambda: X.hat909(1.0, False, v, -0.2), hv(closed * acc, 0.12),
                        length=1, key=('ch', v))


def rides(bar0, bar1, vel=0.4):
    for b in range(bar0, bar1):
        for beat in range(4):
            v = int(rng.integers(3))
            put('ride', T(b, beat), lambda: X.ride909(1.0, v), hv(vel * (1.0 if beat % 2 == 0 else 0.8), 0.08),
                length=2, key=('ride', v))


CONGA_PAT = [(0.75, 'F#4', .6, .4), (1.5, 'B3', .8, -.3), (2.25, 'F#4', .5, .4), (2.75, 'C#5', .35, .5),
             (3.5, 'B3', .7, -.3)]


def congas(bar0, bar1, vel=0.5):
    for b in range(bar0, bar1):
        for beat, note, acc, pan in CONGA_PAT:
            if b % 4 == 3 and beat == 2.75:
                continue
            put('perc', T(b, beat), lambda: X.conga(m(note), 1.0, pan), hv(vel * acc, 0.1), length=1,
                key=('cg', note))


def crash_at(bar, vel=0.6):
    put('cym', T(bar), lambda: X.crash909(1.0, seed=bar % 2), vel, length=3, key=('cr', bar % 2))


# ------------------------------------------------------------- bass

def bass_bar(bar, vel=1.0, cut=260.0, figure=None):
    ch = CHORDS[bar]
    notes = figure or FILL[ch]
    for (beat, dur), note in zip(FILL_RHY, notes):
        md = m(note)
        acc = 1.0 if beat == 0 else (0.85 if beat in (1.5, 2.5) else 0.75)
        d = B(dur) * 0.88
        cq = int(round(cut / 20) * 20)
        put('bass', T(bar, beat), lambda: X.deep_bass(md, d, 1.0, cut=cq, env=650 * cq / 260), vel * acc,
            length=d + 1, key=('db', md, round(d, 3), cq))


def bass_sustain(bar, note, beats, vel=0.8, cut=200.0):
    md = m(note)
    put('bass', T(bar), lambda: X.deep_bass(md, B(beats) - 0.05, 1.0, cut=cut, env=150, sub=0.8, r=0.3), vel,
        length=B(beats) + 2)


def sub_sustain(bar, note, beats, vel=0.8):
    md = m(note)
    put('sub', T(bar), lambda: S.sub_bass(md, B(beats) - 0.05, 1.0, a=0.3, r=0.4, drive=1.1), vel,
        length=B(beats) + 3)


# ------------------------------------------------------------- stabs

STAB_PAT = [(0.5, .25, 1.0), (1.75, .25, 0.7), (2.5, .25, 0.85)]


def cut_curve(bar):
    """Stab filter automation: piecewise (bar, Hz), plus a slow 16-bar LFO."""
    pts = [(8, 380), (16, 900), (32, 1900), (40, 1700), (48, 2000), (60, 650), (64, 2100), (72, 2600),
           (80, 2400), (88, 2200), (96, 1100), (104, 1500)]
    xs = [p[0] for p in pts]
    ys = [np.log(p[1]) for p in pts]
    c = np.exp(np.interp(bar, xs, ys))
    return c * (1 + 0.18 * np.sin(2 * np.pi * (bar - 8) / 16))


def stab_hit(bar, beat, chord, dur, vel):
    c = cut_curve(bar + beat / 4)
    cq = int(round(np.log(c) / np.log(1.04)))     # 4 % steps, cached
    cut = 1.04 ** cq
    notes = tuple(m(n) for n in VOICE[chord])
    put('stab', T(bar, beat), lambda: X.stab(list(notes), B(dur), 1.0, cut=cut), vel, length=2,
        key=('stab', notes, round(dur, 3), cq))


def stabs(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        ch = CHORDS[b]
        if ch not in VOICE:
            continue
        pat = list(STAB_PAT)
        if ch == 'Em9':                 # fragment B: the "E4 E4" re-strike
            pat = pat + [(3.0, .25, 0.6)]
        if b % 4 == 3:
            pat = pat + [(3.75, .2, 0.5)]
        for beat, dur, acc in pat:
            stab_hit(b, beat, ch, dur, hv(vel * acc, 0.05))


# ------------------------------------------------------------- keys / strings

def rhodes_chord(bar, beat, notes, dur, vel=0.6, gain=1.0, spread=0.0):
    for i, note in enumerate(notes):
        md = m(note)
        vv = round(vel * (0.9 + 0.2 * rng.random()), 2)
        put('rhodes', T(bar, beat + i * spread), lambda: X.rhodes(md, B(dur), vv), gain, length=B(dur) + 3)


RH_PAT = [(0, 1.4, 0.7), (1.5, 0.4, 0.45), (2.5, 1.2, 0.6)]


def rhodes_comp(bar0, bar1, vel=0.6, sparse=False):
    for b in range(bar0, bar1):
        ch = CHORDS[b]
        if ch not in VOICE:
            continue
        notes = [n for n in VOICE[ch]]
        pat = [(0, 3.5, 0.65)] if sparse else RH_PAT
        for beat, dur, acc in pat:
            rhodes_chord(b, beat, notes, dur, vel * acc, spread=0.012 if dur > 1 else 0.0)
        if not sparse and b % 2 == 1:   # anticipate the next chord on the "and" of 4
            nxt = CHORDS[b + 1] if b + 1 < NBARS else ch
            if nxt in VOICE:
                rhodes_chord(b, 3.5, VOICE[nxt][1:], 0.45, vel * 0.4)


def string_notes(target, bar, notes, vel=1.0, transpose=0, a=0.35, r=0.7, cut=3600.0, octave=0.3, legato=0.08):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur) + legato
        put(target, T(bar, beat), lambda: X.strings(md, d, 1.0, a=a, r=r, cut=cut, octave=octave), vel,
            length=d + 6)


def pad(bar0, bar1, vel=0.6, a=0.8, cut=2200.0, low=True):
    """String-machine chord pad: one sustained chord per run of equal chords."""
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        notes = PADV.get(ch)
        if notes:
            for note in notes:
                md = m(note) if low else m(note) + 12
                d = T(e) - T(b)
                put('strings', T(b), lambda: X.strings(md, d, 1.0, a=a, r=1.2, cut=cut, octave=0.15), vel,
                    length=d + 6)
        b = e


def lead_notes(bar, notes, vel=1.0, transpose=0, cut=2200.0):
    for beat, note, dur in notes:
        md = m(note) + transpose
        d = B(dur) - 0.02
        put('lead', T(bar, beat), lambda: X.pulse_lead(md, d, 1.0, cut=cut), vel, length=d + 2)


# ---------------------------------------------------------- arrangement

# INTRO 0-15: kick, then hats; a dark string drone; bass and stabs at 8
four(0, 16, 1.0, drop_last=8)
hats(4, 8, closed=0.22, opn=0.0)
hats(8, 12, closed=0.28, opn=0.28)
hats(12, 16, closed=0.3, opn=0.42)
claps(12, 16, 0.8)
for note in ('B2', 'F#3', 'D4'):
    md = m(note)
    put('strings', T(0), lambda: X.strings(md, T(8) - 0.3, 1.0, a=3.0, r=1.5, cut=900, octave=0.1), 0.55,
        length=T(8) + 6)
for b in range(8, 16):
    bass_bar(b, 0.9, cut=150 + 110 * (b - 8) / 7)
stabs(8, 16, 0.75)
put('fx', T(12), lambda: X.noise_swell(T(16) - T(12)), 0.35, length=8)

# GROOVE A 16-31: the full groove, stabs opening
crash_at(16, 0.5)
four(16, 32, 1.0, drop_last=16)
claps(16, 32, 0.9)
hats(16, 32, closed=0.32, opn=0.5)
for b in range(16, 32):
    bass_bar(b, 1.0, cut=260)
stabs(16, 32, 1.0)
pad(24, 32, vel=0.35, a=1.2, cut=1600)
rhodes_comp(24, 32, vel=0.55, sparse=True)
congas(28, 32, 0.35)

# GROOVE B 32-47: the progression and the string theme (fragment C)
crash_at(32, 0.6)
four(32, 48, 1.0)
claps(32, 48, 0.9)
hats(32, 48, closed=0.34, opn=0.52)
rides(40, 48, 0.3)
congas(32, 48, 0.45)
for b in range(32, 48):
    bass_bar(b, 1.0, cut=280)
stabs(32, 48, 0.9)
rhodes_comp(32, 48, vel=0.6)
pad(32, 48, vel=0.3, a=0.6, cut=1800)
string_notes('theme', 32, THEME, vel=0.95, cut=3400)
put('fx', T(46), lambda: X.noise_swell(T(48) - T(46), 400, 4000), 0.3, length=6)

# BREAKDOWN 48-59: no kick; the E-pedal rise (fragment D) on the e-piano
crash_at(48, 0.45)
for cyc in range(2):
    b0 = 48 + 4 * cyc
    for beat, notes, dur in RISE_CH:
        rhodes_chord(b0, beat, notes, dur, 0.55 + 0.1 * cyc, spread=0.03)
        # gentle repeated pulse on the off-beats, like a slow tremolo comp
        for k in range(int(dur)):
            if k % 2 == 1:
                rhodes_chord(b0, beat + k + 0.5, notes[1:], 0.4, 0.25 + 0.05 * cyc)
    put('rhodes', T(b0), lambda: X.rhodes(m('G5'), B(16), 0.5), 0.55, length=B(16) + 3)   # the G5 pedal
    sub_sustain(b0, 'E1', 16, 0.75)
    bass_sustain(b0, 'E2', 16, 0.45)
string_notes('strings', 48, [(0, 'G5', 16), (16, 'G5', 16)], vel=0.4, a=2.0, r=1.5, cut=2400, octave=0.0)
string_notes('strings', 52, [(0, 'B3', 6), (6, 'C#4', 2), (8, 'D4', 4), (12, 'E4', 4)], vel=0.45, a=0.6, cut=2000)
hats(52, 56, closed=0.16, opn=0.0)
for b in (48, 52):
    stab_hit(b, 0.5, 'Em9', .25, 0.7)      # a lone dub stab echoing into the space
# 56-59: the theme's rise returns over G and F#
string_notes('theme', 56, THEME_P1, vel=0.85, cut=3000)
pad(56, 60, vel=0.35, a=0.8, cut=1600)
rhodes_comp(56, 60, vel=0.5, sparse=True)
sub_sustain(56, 'G1', 8, 0.75)
bass_sustain(56, 'G1', 8, 0.5)
sub_sustain(58, 'F#1', 8, 0.8)
bass_sustain(58, 'F#1', 8, 0.5)
hats(56, 60, closed=0.2, opn=0.0)
put('fx', T(56), lambda: X.noise_swell(T(60) - T(56), 250, 4500), 0.4, length=10)

# BUILD 60-63: the kick and the bass return with the filters opening
four(60, 64, 0.95)
for b in range(60, 64):
    bass_bar(b, 0.95, cut=140 + 120 * (b - 60) / 3)
stabs(60, 64, 0.85)
hats(60, 64, closed=0.3, opn=0.3)
claps(62, 64, 0.75)
string_notes('theme', 60, THEME_P1, vel=0.9, cut=3200)
rhodes_comp(60, 64, vel=0.5)
for beat, md in ((3.0, 54), (3.25, 50), (3.5, 47)):
    put('perc', T(63, beat), lambda: X.tom909(md, 1.0), 0.45, length=1)

# PEAK 64-79: everything; the theme doubled by a soft pulse lead
crash_at(64, 0.7)
crash_at(72, 0.5)
four(64, 80, 1.0)
claps(64, 80, 0.95)
hats(64, 80, closed=0.36, opn=0.55)
rides(64, 80, 0.32)
congas(64, 80, 0.5)
for b in range(64, 80):
    bass_bar(b, 1.0, cut=300)
stabs(64, 80, 0.9)
rhodes_comp(64, 80, vel=0.62)
pad(64, 80, vel=0.32, a=0.5, cut=2000)
string_notes('theme', 64, THEME, vel=1.0, cut=3800)
string_notes('theme', 64, THEME_HARM, vel=0.45, cut=3000, octave=0.0)
lead_notes(64, THEME, vel=0.35, transpose=0, cut=2600)

# DARK TURN 80-87: back to the vamp, more hypnotic; the Bm7b5 / tritone bar
crash_at(80, 0.5)
four(80, 88, 1.0)
claps(80, 88, 0.9)
hats(80, 88, closed=0.36, opn=0.55)
rides(80, 87, 0.28)
congas(80, 88, 0.5)
for b in range(80, 88):
    bass_bar(b, 1.0, cut=300)
stabs(80, 88, 1.0)
pad(80, 88, vel=0.3, a=1.0, cut=1500)
string_notes('theme', 80, [(0, 'F#5', 8), (8, 'E5', 8), (16, 'D5', 4), (20, 'C#5', 4), (24, 'D5', 4),
                            (28, 'F5', 4), (32, 'F#5', 4)],
             vel=0.55, a=1.0, cut=2800)
rhodes_comp(80, 88, vel=0.5, sparse=True)

# OUTRO 88-103: strip back, the filters close, a last theme statement and the
# cadence F#7 -> Bm9 with a final hit ringing out
crash_at(88, 0.45)
four(88, 103, 1.0, drop_last=8)
place_kick(103, 0, 1.0)
claps(88, 103, 0.85)
hats(88, 96, closed=0.32, opn=0.4)
hats(96, 103, closed=0.3, opn=0.45)
congas(96, 102, 0.4)
for b in range(88, 103):
    bass_bar(b, 1.0, cut=270)
stabs(88, 103, 0.9)
rhodes_comp(96, 103, vel=0.55)
string_notes('theme', 96, THEME_P1, vel=0.85, cut=3200)
string_notes('theme', 100, [(0, 'B4', 4), (4, 'E5', 4), (8, 'C#5', 2), (10, 'A#4', 2)], vel=0.8, cut=3000)
pad(96, 103, vel=0.3, a=0.6, cut=1700)
# final hit, bar 103
stab_hit(103, 0, 'Bm9', 1.0, 1.0)
rhodes_chord(103, 0, ['B3', 'D4', 'F#4', 'A4', 'C#5'], 6, 0.7, spread=0.015)
string_notes('theme', 103, [(0, 'B4', 4), (0, 'F#4', 4), (0, 'D5', 4)], vel=0.6, a=0.05, r=1.0, cut=2800)
put('bass', T(103), lambda: X.deep_bass(m('B1'), B(3), 1.0, cut=260, env=500, r=0.5), 1.0, length=4)
crash_at(103, 0.4)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_bass = S.sidechain(N, kicks, depth=0.7, release=0.22)
duck_mid = S.sidechain(N, kicks, depth=0.35, release=0.25)
duck_soft = S.sidechain(N, kicks, depth=0.2, release=0.25)

GAIN = {  # dB
    'kick': -2.0, 'clap': -1.0, 'hats': 7.0, 'ride': 6.0, 'perc': 4.0, 'cym': -12.0,
    'bass': -0.5, 'sub': -6.0, 'stab': 5.5, 'strings': 2.0, 'theme': 3.5, 'rhodes': -6.0,
    'lead': 2.0, 'fx': -12.0,
}
DUCK = {'bass': duck_bass, 'sub': duck_bass, 'stab': duck_mid, 'strings': duck_soft, 'theme': duck_soft,
        'rhodes': duck_soft, 'lead': duck_soft, 'perc': duck_soft}
VERB = {'clap': .12, 'perc': .2, 'stab': .25, 'strings': .4, 'theme': .35, 'rhodes': .3, 'lead': .3,
        'fx': .4, 'ride': .1}
DELAY = {'stab': .55, 'rhodes': .12, 'lead': .2, 'perc': .1, 'theme': .06}

bus['stab'].x = stage(bus['stab'].x, HighpassFilter(180), PeakFilter(400, -3.0, 0.9))
bus['strings'].x = X.chorus(stage(bus['strings'].x, HighpassFilter(180)), rate=0.45, depth_ms=2.2, mix=0.5)
bus['theme'].x = X.chorus(stage(bus['theme'].x, HighpassFilter(220), PeakFilter(600, -2.0, 0.8), HighShelfFilter(3000, 1.5, 0.7)), rate=0.6,
                          depth_ms=2.0, mix=0.45)
bus['rhodes'].x = stage(bus['rhodes'].x, HighpassFilter(120), PeakFilter(420, -2.5, 1.0), PeakFilter(3200, -2.0, 0.9))
# e-piano: slow stereo auto-pan (tremolo), as on a Suitcase
tt = (np.arange(N) / SR + OFF).astype(np.float32)
pan_lfo = 0.35 * np.sin(2 * np.pi * tt / (SPB * 2))
bus['rhodes'].x[0] *= (1 - pan_lfo)
bus['rhodes'].x[1] *= (1 + pan_lfo)
del pan_lfo
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(300))
bus['hats'].x = S.butter(stage(bus['hats'].x, HighpassFilter(4500), HighShelfFilter(10000, -4.0, 0.7)), 11000,
                         'low', 4).astype(np.float32)
bus['ride'].x = stage(bus['ride'].x, HighpassFilter(3000))
bus['clap'].x = stage(bus['clap'].x, HighpassFilter(250))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(32), LowpassFilter(2200))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(150))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(300))

# theme strings: a slow low-pass evolution (dark at the start of each phrase,
# opening as the line climbs), on top of the per-note filter
bars_axis = (np.arange(N) / SR + OFF) / (4 * SPB)
evo = 2400 * (8000 / 2400) ** (0.5 + 0.5 * np.sin(2 * np.pi * (bars_axis - 32) / 32 - np.pi / 2))
bus['theme'].x = S.svf(bus['theme'].x, evo, 0.7).astype(np.float32)
del evo

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
bands = {}
TONAL = np.zeros((2, N), np.float32) if os.environ.get('NEW08_TONAL') else None
MUTE = set(filter(None, os.environ.get('NEW08_MUTE', '').split(',')))
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        continue
    x = x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    seg = x.T.astype(np.float64)
    levels[k] = pyln.Meter(SR).integrated_loudness(seg) if np.any(seg) else -99.0
    if os.environ.get('NEW08_BANDS') and np.any(seg):
        Y = np.abs(np.fft.rfft(seg[:, 0] + seg[:, 1])) ** 2
        f = np.fft.rfftfreq(seg.shape[0], 1 / SR)
        bands[k] = [10 * np.log10(Y[(f >= lo) & (f < 2 * lo)].sum() + 1e-9) for lo in
                    (20, 40, 80, 160, 315, 630, 1250, 2500, 5000, 10000)]
        del Y, f
    del seg
    mix += x
    if TONAL is not None and k not in ('kick', 'clap', 'hats', 'ride', 'perc', 'cym', 'fx'):
        TONAL += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

pre = int(0.015 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(300), LowpassFilter(7500),
             Reverb(room_size=0.82, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
dly = X.dub_delay(delay_send, B(0.75), feedback=0.52, repeats=9, lp=2600, hp=450)
del delay_send
dly = stage(dly, Reverb(room_size=0.6, damping=0.6, wet_level=0.3, dry_level=1.0, width=1.0))
mix = mix + verb * duck_soft[None, :] * 0.85 + dly * duck_mid[None, :] * 0.55
del verb, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -4.5), (8, -3.5), (15.9, -2.5), (16, -1.5), (31.9, -1.0), (32, -0.5), (47.9, -0.5),
              (48, -6.5), (55.9, -6.0), (56, -5.5), (59.9, -4.0), (60, -3.5), (63.9, -1.0),
              (64, 0.0), (79.9, 0.0), (80, -0.3), (87.9, -0.3), (88, -2.0), (95.9, -2.5), (96, -2.0),
              (104, -2.0), (110, -2.0)]
sec = np.interp(bars_axis, [p[0] for p in SECTION_DB], [p[1] for p in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(90, 0.5, 0.7), HighShelfFilter(10000, -1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=25, release_ms=160))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

if not _bars:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
    end = min(last + 1, int((T(103) + 4.0) * SR))   # the final hit rings ~4 s
    mix = mix[:, :end]
    fade = int(1.2 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new08.wav'
if TONAL is not None:   # dry tonal buses only (no drums), for the chroma check
    TONAL = TONAL[:, :mix.shape[1]]
    sf_.write(os.environ['NEW08_TONAL'], (TONAL / (np.abs(TONAL).max() + 1e-9) * 0.5).T, SR, subtype='FLOAT')
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
if bands:
    ref = max(v[1] for v in bands.values())
    print('bands     ' + ' '.join(f'{lo:>6d}' for lo in (20, 40, 80, 160, 315, 630, 1250, 2500, 5000, 10000)))
    for k, v in bands.items():
        print(f'  {k:8s}' + ' '.join(f'{x - ref:6.1f}' for x in v))
