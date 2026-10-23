#!/usr/bin/env python3
# song4_chiptune.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 4, Chiptune": a modern-produced chiptune remix of sf-music4 (the
# original game's music by Stardock, from mid/sf-music4.mid, remixed here).
# The chip voices are NES 2A03 models from tools/music/edm/synth_chip.py
# (band-limited pulse / 4-bit triangle / LFSR noise); the modern layer (kick,
# snare, clap, crash, risers, impacts) comes from tools/music/edm/synth.py.
# Everything is synthesized in code: no samples, no SoundFonts.
#
# Taken from the original song (C minor over C / F# tritone pedals, 145 BPM).
# Bars below are the original's 1-indexed bars; this remix's bars 0-59
# follow original bars 1-60 one for one:
#   - the square-lead figure   G5 C5 G5 C5 G5 C5 C5 C5 | G5 C5 G5 C5 G5 C5
#                              (ch 1, every 4 bars, all through the song)
#   - the brass ostinato       G3 G#3 in 8ths (ch 3, 544 notes; G#3 A3 at 57-60)
#   - the choir / strings pedals: C2 + C6, F#2 at 13-16 (the tritone), A#1 at 31,
#     C#2 + C#6 at 57-60
#   - the Voice Oohs C7 pings and their 4-bar rhythm cycle (ch 5)
#   - the taiko fill (bar 12-13) and the china / snare accents of every phrase
#   - the honky-tonk piano tritone figure G4 C#4 F#4 C4 (bars 15-16, 39-40),
#     kept exactly: it is intentional
#   - the French-horn theme    G3 G4 F4 C#4 | G3 G4 F4 G4 | ... D4 D#4 (17-32)
#     and the strings' counter-line G#5 F5 (D#5 G5) (bars 26-38)
#   - the claves rhythm (bars 39-40)
#   - the dark middle (41-56): piano G#4/F#4 against C5, the fretless tritone
#     riff C2 G2 C3 G2 D#2 | F#2 C2 F#2, the cabasa 8ths, the whistle line
#     G3 G#3 C4 F#4 G#4, and its semitone-up restatement A3 C#4 G4 A4 G#4 (57-60)
# New: 160 BPM, a chiptune form (intro, tension, theme, dark middle, a "boss
# fight" in C# minor with double-time drums: the original's own semitone lift
# at bar 57, extended into a climax), a short C-major "stage clear" statement
# of the theme, and an outro back on the C ostinato (it loops in the game).
# Chords under the theme (Cm, Db, Abmaj7, Bb, Eb) follow the original's bass and
# melody notes; the chip arpeggios, triangle octave bass, bitcrushed bass layer,
# drums, chip echo (repeated notes at lower volume) and stereo-spread duplicate
# channels are new.
#
#   python3 tools/music/edm/song4_chiptune.py OUT.wav [bar0 bar1]
#   SONG4_MUTE=kick,snare,noise,cym  (mute buses, e.g. for the chroma check)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter, Bitcrush,
                        HighpassFilter, LowpassFilter, LowShelfFilter,
                        HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_chip as C  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 160.0
SPB = 60.0 / BPM
NBARS = 104
TAIL = 5.0

BAR0, BAR1 = 0, NBARS
if len(sys.argv) > 3:
    BAR0, BAR1 = int(sys.argv[2]), int(sys.argv[3])
OFF = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


SECTIONS = {'intro': (0, 8), 'tension': (8, 16), 'theme': (16, 32), 'themeB': (32, 40),
            'dark': (40, 56), 'build': (56, 60), 'boss1': (60, 76), 'boss2': (76, 88),
            'victory': (88, 96), 'outro': (96, 104)}

# ------------------------------------------------------------- harmony
# chord per bar: arp (semitones over a base note), triangle root, chord tones
ARP = {
    'Cped':  ('C4', [0, 7, 12]),   'F#ped': ('F#4', [0, 6, 12]),
    'Cm':    ('C4', [0, 3, 7]),    'Db':    ('C#4', [0, 4, 7]),
    'Abmaj7':  ('G#3', [0, 4, 7, 11]), 'Bb':  ('A#3', [0, 4, 7]),
    'Eb':    ('D#4', [0, 4, 7]),   'Ctri':  ('C4', [0, 6, 7]),
    'CAb':   ('G#4', [0, 4]),      'F#C':   ('F#4', [0, 6]),
    'C#ped': ('C#4', [0, 7, 12]),  'Gtri':  ('G4', [0, 6]),
    'C#A':   ('A4', [0, 4]),       'C#m':   ('C#4', [0, 3, 7]),
    'D':     ('D4', [0, 4, 7]),    'Amaj7':   ('A3', [0, 4, 7, 11]),
    'B':     ('B3', [0, 4, 7]),    'E':     ('E4', [0, 4, 7]),
    'C#tri': ('C#4', [0, 6, 7]),
    'C':     ('C4', [0, 4, 7]),    'Am':    ('A3', [0, 3, 7]),
    'F':     ('F3', [0, 4, 7]),    'G':     ('G3', [0, 4, 7]),
}
ROOT = {'Cped': 'C2', 'F#ped': 'F#2', 'Cm': 'C2', 'Db': 'C#2', 'Abmaj7': 'G#1', 'Bb': 'A#1', 'Eb': 'D#2',
        'Ctri': 'C2', 'CAb': 'C2', 'F#C': 'F#2', 'C#ped': 'C#2', 'Gtri': 'G2', 'C#A': 'C#2',
        'C#m': 'C#2', 'D': 'D2', 'Amaj7': 'A1', 'B': 'B1', 'E': 'E2', 'C#tri': 'C#2',
        'C': 'C2', 'Am': 'A1', 'F': 'F2', 'G': 'G1'}
# pitch classes expected in the chroma (for the check script)
PCS = {'Cped': 'C G G#', 'F#ped': 'F# C G', 'Cm': 'C D# G', 'Db': 'C# F G#', 'Abmaj7': 'G# C D# G',
       'Bb': 'A# D F', 'Eb': 'D# G A#', 'Ctri': 'C F# G C#', 'CAb': 'C G# G D#', 'F#C': 'F# C G',
       'C#ped': 'C# G# A', 'Gtri': 'G C# G# A', 'C#A': 'C# A G# E', 'C#m': 'C# E G#', 'D': 'D F# A',
       'Amaj7': 'A C# E G#', 'B': 'B D# F#', 'E': 'E G# B', 'C#tri': 'C# G G# D',
       'C': 'C E G', 'Am': 'A C E', 'F': 'F A C', 'G': 'G B D'}

THEME_PROG = ['Cm', 'Cm', 'Db', 'Db', 'Cm', 'Cm', 'Abmaj7', 'Abmaj7',
              'Cm', 'Cm', 'Db', 'Db', 'Cm', 'Cm', 'Bb', 'Eb']
BOSS_PROG = ['C#m', 'C#m', 'D', 'D', 'C#m', 'C#m', 'Amaj7', 'Amaj7',
             'C#m', 'C#m', 'D', 'D', 'C#m', 'C#m', 'B', 'E']
CHORDS = (['Cped'] * 12 + ['F#ped'] * 4 +              # intro, tension (orig 1-16)
          THEME_PROG +                                 # theme (orig 17-32)
          ['Cm', 'Cm', 'Db', 'Db', 'Cm', 'Cm', 'Ctri', 'Ctri'] +   # theme B (orig 33-40)
          ['CAb', 'F#C'] * 8 +                         # dark middle (orig 41-56)
          ['C#ped', 'Gtri', 'C#ped', 'Gtri'] +         # build, in C# (orig 57-60)
          BOSS_PROG +                                  # boss 1
          ['C#A', 'Gtri'] * 4 + ['C#tri'] * 4 +        # boss 2
          ['C', 'C', 'Am', 'Am', 'F', 'F', 'G', 'G'] + # stage clear
          ['Cped'] * 8)                                # outro
assert len(CHORDS) == NBARS, len(CHORDS)

# ------------------------------------------------------------- themes (beats, note, length)
LEADPAT = [(0, 'G5', .25), (.25, 'C5', .75), (1, 'G5', .25), (1.25, 'C5', .75), (2, 'G5', .25),
           (2.25, 'C5', .75), (3, 'C5', .25), (3.25, 'C5', .75),
           (4, 'G5', .25), (4.25, 'C5', .75), (5, 'G5', .25), (5.25, 'C5', .75), (6, 'G5', .25),
           (6.25, 'C5', .75)]
PING_A = [0, .25, .75, 1.25, 1.75, 2.25, 3.25, 3.75]
PING_B = [2, 2.25, 2.75, 3.25, 3.75]
PING_C = [.25, 1.25, 1.75]
PING_CYCLE = [PING_C, PING_A, PING_B, PING_C]     # orig ch 5, from bar 17 on
HORN1 = [(1, 'G3', 1), (2, 'G4', 4), (6, 'F4', 2), (8, 'C#4', 8)]
HORN2 = [(1, 'G3', 1), (2, 'G4', 4), (6, 'F4', 2), (8, 'G4', 8)]
HORN4 = [(1, 'G3', 1), (2, 'G4', 4), (6, 'F4', 2), (8, 'D4', 4), (12, 'D#4', 4)]
STRINGS = [(6, 'G#5', 2), (8, 'F5', 8)]
THEMEB_STR = [(6, 'G#5', 2), (8, 'F5', 6), (14, 'D#5', 2), (16, 'G5', 16)]
TRIFIG = [(0, 'G4', .5), (.5, 'C#4', .5), (1, 'F#4', .5), (1.5, 'C4', .5),
          (2, 'G4', .5), (2.5, 'C#4', .5), (3, 'F#4', .5), (3.5, 'C4', .5)]
CLAVES = [0, .5, .75, 1, 2.5, 3, 5.5, 5.75, 6, 6.5, 7, 7.5]
RIFF_C = [(0, 'C2', 1), (1, 'G2', .5), (1.5, 'C3', 1), (2.5, 'G2', 1), (3.5, 'D#2', .5)]
RIFF_F = [(0, 'F#2', 1), (1, 'C2', .5), (1.5, 'F#2', 2.5)]
RIFF_F2 = [(0, 'F#2', 1), (1, 'C2', .5), (1.5, 'F#1', 2.5)]
RIFF_PICKUP = [(2.5, 'C2', .5), (3, 'G1', .5), (3.5, 'A#1', .5)]
WHISTLE1 = [(0, 'G3', 2), (2, 'G#3', 4), (6, 'C4', 2), (8, 'F#4', 2), (10, 'G#4', 8)]
WHISTLE2 = [(0, 'A3', 4), (4, 'C#4', 2), (6, 'G4', 2), (8, 'A4', 4), (12, 'G#4', 4)]
# stage clear: the horn theme's rhythm, resolved into C major
VICTORY = [(1, 'G3', 1), (2, 'G4', 4), (6, 'F4', 2), (8, 'E4', 8),
           (17, 'G3', 1), (18, 'A4', 4), (22, 'F4', 2), (24, 'G4', 8)]


def tr(notes, k):
    return [(b, m(n) + k, l) for b, n, l in notes]


# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'noise', 'cym', 'tri', 'crush', 'lead', 'echo', 'sq', 'ost', 'arp', 'ping',
         'counter', 'fig', 'fx']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, sig, t, gain=1.0, pan=0.0):
    t = t - OFF
    if t < -12 or t > N / SR:
        return
    sig = np.asarray(sig)
    if sig.ndim == 1:
        sig = S.pan2(sig, pan)
    bus[name].add(sig, t, gain)


def chip_echo(name, sig, t, delay_beats=.75, gain=.38, repeats=2, pan=0.0, decay=.55):
    """The chip echo trick: the same note again, quieter, on the other side."""
    for i in range(1, repeats + 1):
        put(name, sig, t + B(delay_beats * i), gain * decay ** (i - 1), -pan if i % 2 else pan)


# ------------------------------------------------------- note helpers

def note_m(n):
    return m(n) if isinstance(n, str) else int(n)


def lead_line(bar0, notes, transpose=0, gain=1.0, octave_up=0.0, echo=True, width=1.0,
              duty0=.5, duty1=.25):
    """Main pulse lead: 50->25 % duty sweep, slides between notes, delayed vibrato,
    a stereo pair of 12.5 % duplicates, and the chip echo."""
    prev = None
    for beat, n, dur in notes:
        md = note_m(n) + transpose
        slide = 0.0 if prev is None or abs(prev - md) > 12 else float(prev - md)
        if prev is not None and abs(prev - md) <= 12 and dur < 1.0:
            slide *= 0.5
        long_ = dur >= 1.5
        d = B(dur) - 0.012
        sig = C.pulse(md, d, duty0, duty_end=duty1, duty_time=0.35, d=0.5, s=0.72, r=0.05,
                      vib=0.28 if long_ else 0.0, vib_rate=6.2, vib_delay=0.22,
                      slide=slide, slide_tau=0.022)
        t = T(bar0, beat)
        put('lead', sig, t, gain)
        if width:
            for side, c in ((-1, -7.0), (1, 7.0)):
                dup = C.pulse(md, d, .125, d=0.5, s=0.6, r=0.05, vib=0.22 if long_ else 0.0,
                              vib_rate=5.7, vib_delay=0.26, slide=slide, slide_tau=0.03, cents=c)
                put('lead', dup, t + 0.006 * (side + 1), gain * 0.45 * width, 0.8 * side)
        if octave_up:
            up = C.pulse(md + 12, d, .125, d=0.4, s=0.55, r=0.05, vib=0.2 if long_ else 0.0,
                         slide=slide, slide_tau=0.022)
            put('lead', up, t, gain * octave_up)
        if echo:
            chip_echo('echo', sig, t, .75, .36, 2, 0.6)
        prev = md


def counter_line(bar0, notes, transpose=0, gain=1.0, pan=0.35, duty=.5, echo=False):
    for beat, n, dur in notes:
        md = note_m(n) + transpose
        sig = C.pulse(md, B(dur) - 0.02, duty, a=0.05, d=0.8, s=0.7, r=0.08,
                      vib=0.18 if dur >= 2 else 0.0, vib_rate=5.4, vib_delay=0.3)
        put('counter', sig, T(bar0, beat), gain, pan)
        if echo:
            chip_echo('echo', sig, T(bar0, beat), .75, .3, 1, pan)


def string_pad(bar0, bars, note='C6', gain=1.0, pan=-0.4):
    """The original's held string note, as a soft 12.5 % pulse with tremolo-ish vibrato."""
    sig = C.pulse(m(note), B(4 * bars) - 0.05, .125, a=0.6, d=2.0, s=0.8, r=0.3, vib=0.12,
                  vib_rate=4.8, vib_delay=0.4)
    put('counter', sig, T(bar0), gain, pan)


def sq_pattern(bar0, bar1, transpose=0, gain=1.0, pan=-0.3, duty=.25, echo=True):
    """The original's square-lead figure (G5 C5 ...), every 4 bars."""
    for b in range(bar0, bar1, 4):
        for beat, n, dur in LEADPAT:
            if b + beat / 4 >= bar1:
                break
            md = m(n) + transpose
            acc = 1.0 if beat % 1 == 0 else 0.8
            sig = cached(('sq', md, dur, duty), lambda: C.pulse(md, B(dur) - .02, duty, d=0.14,
                                                                  s=0.45, r=0.03))
            put('sq', sig, T(b, beat), gain * acc, pan)
            if echo:
                chip_echo('echo', sig, T(b, beat), .75, .3, 1, pan)


def ostinato(bar0, bar1, lo='G3', hi='G#3', div=2, gain=1.0, transpose=0):
    """The brass ostinato G3-G#3 (div=2: 8ths as written; div=4: double time)."""
    a, b_ = m(lo) + transpose, m(hi) + transpose
    L = 1.0 / div
    for b in range(bar0, bar1):
        for i in range(4 * div):
            md = a if i % 2 == 0 else b_
            acc = 1.0 if i % div == 0 else 0.82
            sig = cached(('ost', md, L), lambda: C.pulse(md, B(L * .8), .25, d=0.09, s=0.35,
                                                         r=0.025))
            put('ost', sig, T(b, i * L), gain * acc, -0.25)
            dup = cached(('ostd', md, L), lambda: C.pulse(md, B(L * .7), .125, d=0.07,
                                                          s=0.3, r=0.02, cents=8))
            put('ost', dup, T(b, i * L) + 0.012, gain * acc * 0.6, 0.65)
            oc = cached(('osto', md, L), lambda: C.pulse(md + 12, B(L * .5), .125, d=0.04,
                                                         s=0.15, r=0.02, cents=-5))
            put('ost', oc, T(b, i * L) + 0.004, gain * acc * 0.25, -0.7)


def pings(bar0, bar1, note='C7', gain=1.0, cycle_start=16):
    for b in range(bar0, bar1):
        for k, beat in enumerate(PING_CYCLE[(b - cycle_start) % 4]):
            md = m(note)
            sig = cached(('ping', md), lambda: C.pulse(md, .03, .25, d=0.035, s=0.0, r=0.02))
            pan = 0.45 if k % 2 else -0.45
            put('ping', sig, T(b, beat), gain, pan)
            chip_echo('ping', sig, T(b, beat), .75, .4, 2, pan)


def arps(bar0, bar1, div=1, gain=1.0, gate=.85, duty=.25, transpose=0, spread=0.55):
    """50 Hz chord arpeggios, retriggered every 1/div beat, on two chip channels
    panned apart: the second one at 12.5 % duty, starting one arp step later."""
    for b in range(bar0, bar1):
        base, steps = ARP[CHORDS[b]]
        md = m(base) + transpose
        L = 1.0 / div
        rot = steps[1:] + steps[:1]
        for i in range(4 * div):
            acc = 1.0 if i % div == 0 else 0.78
            for side, st, du in ((-1, steps, duty), (1, rot, .125)):
                sig = cached(('arp', md, tuple(st), L, du),
                             lambda: C.pulse(md, B(L * gate), du, arp=st, d=0.12 if div > 1 else 0.22,
                                             s=0.45, r=0.03))
                put('arp', sig, T(b, i * L), gain * acc * (0.8 if side > 0 else 0.75), spread * side)


def tri_bass(bar0, bar1, style='oct8', gain=1.0, crush=0.0):
    for b in range(bar0, bar1):
        r = m(ROOT[CHORDS[b]])
        if style == 'pedal':
            notes = [(0, r, 4)]
        elif style == 'oct8':
            notes = [(i * .5, r + (12 if i % 2 else 0), .5) for i in range(8)]
        elif style == 'drive':
            notes = [(i * .5, r + (12 if i in (3, 6) else 0), .5) for i in range(8)]
        elif style == 'gallop':
            pat = [0, 0, 12, 0, 0, 12, 0, 12, 0, 0, 12, 0, 0, 12, 7, 12]
            notes = [(i * .25, r + pat[i], .25) for i in range(16)]
        else:
            raise ValueError(style)
        for beat, md, dur in notes:
            g = 0.94 if dur < 4 else 1.0
            sig = cached(('tri', md, dur), lambda: C.triangle(md, B(dur) - .015, r=0.006))
            put('tri', sig, T(b, beat), gain * g)
            if crush:
                cs = cached(('cr', md, dur), lambda: C.pulse(md + 12, B(min(dur, 1)) - .02, .5,
                                                             d=0.1, s=0.5, r=0.02))
                put('crush', cs, T(b, beat), crush)


def tri_line(bar0, notes, transpose=0, gain=1.0, crush=0.0):
    for beat, n, dur in notes:
        md = note_m(n) + transpose
        sig = C.triangle(md, B(dur) - .015, r=0.006)
        put('tri', sig, T(bar0, beat), gain)
        if crush:
            put('crush', C.pulse(md + 12, B(dur) - .02, .5, d=0.15, s=0.55, r=0.02), T(bar0, beat), crush)


def tri_fig(bar0, bar1, transpose=0, div=2, gain=1.0, echo=True):
    """The honky-tonk piano tritone figure G4 C#4 F#4 C4, as a 12.5 % pulse."""
    for b in range(bar0, bar1):
        for i in range(4 * div):
            n = TRIFIG[i % 4][1]
            md = m(n) + transpose
            L = 1.0 / div
            sig = cached(('fig', md, L), lambda: C.pulse(md, B(L) * .7, .125, d=0.1, s=0.3, r=0.02))
            pan = -0.35 if i % 2 else 0.35
            put('fig', sig, T(b, i * L), gain * (1.0 if i % 2 == 0 else .85), pan)
            if echo:
                chip_echo('echo', sig, T(b, i * L), .75, .3, 1, pan)


def dyads(bar0, bar1, gain=1.0):
    """The original's piano G#4/F#4 against C5 in 8ths: 2-note 50 Hz arps."""
    for b in range(bar0, bar1):
        base, steps = ARP[CHORDS[b]]
        md = m(base)
        for i in range(8):
            sig = cached(('dy', md, tuple(steps)), lambda: C.pulse(md, B(.38), .125, arp=steps, d=0.1,
                                                                   s=0.35, r=0.02))
            put('fig', sig, T(b, i * .5), gain * (1.0 if i % 2 == 0 else .8), 0.25 if i % 2 else -0.25)


# --------------------------------------------------------------- drums

KICK_F = {'C': 49.0, 'C#': 51.9}     # G1 / G#1: the fifth of the key


def kick(t, vel=1.0, key='C'):
    put('kick', cached(('kick', key), lambda: S.kick(1.0, 0.38, KICK_F[key], 150.0)), t, vel)
    put('noise', cached(('ck',), lambda: C.chip_kick(1.0)), t, 0.35 * vel)
    if BAR0 * 4 * SPB - 1 <= t <= BAR1 * 4 * SPB + 1:
        kicks.append(t - OFF)


def snare(t, vel=1.0, chip=True):
    put('snare', cached(('sn',), lambda: S.snare(1.0, 196.0)), t, vel)
    put('snare', cached(('cl',), lambda: S.clap(1.0)), t, 0.45 * vel)
    if chip:
        put('noise', cached(('cs',), lambda: C.chip_snare(1.0)), t, 0.8 * vel)


def chip_sn(t, vel=1.0, pan=0.0):
    put('noise', cached(('cs',), lambda: C.chip_snare(1.0)), t, vel, pan)


def hat(t, vel=1.0, open_=False, pan=0.15):
    put('noise', cached(('h', open_), lambda: C.chip_hat(1.0, open_)), t, vel, pan)


def shaker(t, vel=1.0, pan=-0.2):
    put('noise', cached(('sh',), lambda: C.chip_shaker(1.0)), t, vel, pan)


def china(t, vel=1.0):
    put('noise', cached(('ch',), lambda: C.chip_china(1.0)), t, 0.8 * vel, 0.2)
    put('cym', cached(('cr',), lambda: S.crash(1.0, 2.5)), t, 0.55 * vel)


def crash(t, vel=1.0):
    put('cym', cached(('cr',), lambda: S.crash(1.0, 2.5)), t, vel)
    put('noise', cached(('chs',), lambda: C.noise(0.9, 1, False, d=0.35, s=0.0, r=0.2)), t, 0.6 * vel, -0.2)


def groove(bar0, bar1, style, key='C', hats_on=True, vel=1.0):
    for b in range(bar0, bar1):
        if style == 'half':
            kick(T(b, 0), vel, key)
            if b % 2 == 1:
                kick(T(b, 2.5), 0.85 * vel, key)
            snare(T(b, 2), vel)
            if hats_on:
                for i in range(8):
                    hat(T(b, i * .5), 0.55 if i % 2 == 0 else 0.8, pan=0.3 if i % 2 else -0.1)
        elif style == 'main':
            for bt, v in ((0, 1.0), (2, .9), (2.5, .8)):
                kick(T(b, bt), v * vel, key)
            if b % 4 == 3:
                kick(T(b, 3.75), .7 * vel, key)
            snare(T(b, 1), vel)
            snare(T(b, 3), vel)
            if hats_on:
                for i in range(16):
                    if i % 2 == 0:
                        hat(T(b, i * .25), 0.75 if i % 4 == 2 else 0.5, pan=0.3 if i % 4 == 2 else -0.1)
                    elif i in (7, 15):
                        hat(T(b, i * .25), 0.35, pan=-0.4)
                hat(T(b, 3.5), 0.55, True)
        elif style == 'double':
            for bt in range(4):
                kick(T(b, bt), (1.0 if bt % 2 == 0 else 0.85) * vel, key)
            snare(T(b, 1), vel)
            snare(T(b, 3), vel)
            for bt in (.5, 1.5, 2.5, 3.5):
                chip_sn(T(b, bt), 0.42 * vel, 0.25)
            if hats_on:
                for i in range(16):
                    hat(T(b, i * .25), (0.7 if i % 2 == 0 else 0.42) * vel, pan=0.3 if i % 2 else -0.15)
        else:
            raise ValueError(style)


def phrase_accents(bar0, bar1, vel=1.0):
    """The original's accent: two 16th snares at the end of the third bar of each
    phrase, and a china on the fourth bar's downbeat."""
    for b in range(bar0, bar1, 4):
        chip_sn(T(b + 2, 3.5), 0.55 * vel, -0.2)
        chip_sn(T(b + 2, 3.75), 0.7 * vel, 0.2)
        china(T(b + 3), 0.7 * vel)


def snare_roll(bar0, bars_by_div, vel0=.25, vel1=1.0):
    t0 = T(bar0)
    total = sum(b for b, _ in bars_by_div) * 4
    beat = 0.0
    for bars, div in bars_by_div:
        for i in range(int(bars * 4 * div)):
            bb = beat + i / div
            u = bb / total
            v = vel0 + (vel1 - vel0) * u ** 1.3
            chip_sn(t0 + B(bb), v, 0.3 * np.sin(bb * 2.1))
            if div <= 4:
                put('snare', cached(('sn',), lambda: S.snare(1.0, 196.0)), t0 + B(bb), v * 0.5)
        beat += bars * 4


def toms(bar, beats, notes, vel=1.0):
    for bt, n in zip(beats, notes):
        md = note_m(n)
        put('tri', cached(('tom', md), lambda: C.chip_tom(md)), T(bar, bt), vel)
        put('snare', cached(('stom', md), lambda: S.tom(md - 12)), T(bar, bt), 0.35 * vel)


def chip_riser(bar0, bars, lo='C4', hi='C6', arp=(0, 7, 12), gain=1.0):
    d = B(4 * bars)
    sig = C.pulse(m(lo), d, .25, duty_end=.125, duty_time=d, a=d * 0.7, d=1.0, s=1.0, r=0.02,
                  arp=list(arp), bend=m(hi) - m(lo), bend_time=d)
    put('fx', sig, T(bar0), gain)


# ---------------------------------------------------------- arrangement

# INTRO 0-7 (orig 1-8): square figure, ostinato, pedals, then the pings
ostinato(0, 8, gain=0.85)
sq_pattern(0, 8, gain=0.9, pan=0.0, duty=.25)
tri_bass(0, 8, 'pedal', gain=0.75)
string_pad(0, 4, 'C6', 0.55)
string_pad(4, 4, 'C6', 0.65)
pings(4, 8, gain=0.75)
for b in range(4, 8):
    for i in range(8):
        shaker(T(b, i * .5), 0.35 if i % 2 == 0 else 0.5)
snare_roll(7, [(.5, 4), (.5, 8)], 0.2, 0.75)
put('fx', S.noise_riser(B(8), 500, 9000), T(6), 0.35)

# TENSION 8-15 (orig 9-16): drums in, the F# tritone pedal at 12, piano figure at 14
crash(T(8), 0.8)
groove(8, 12, 'half')
groove(12, 16, 'half')
ostinato(8, 16, gain=0.95)
sq_pattern(8, 16, gain=0.85, pan=-0.25)
pings(8, 16, gain=0.7)
tri_bass(8, 12, 'drive', gain=0.95, crush=0.5)
tri_bass(12, 16, 'drive', gain=1.0, crush=0.65)
string_pad(8, 4, 'C6', 0.6)
string_pad(12, 4, 'C6', 0.7, pan=0.4)
arps(8, 16, div=1, gain=0.55)
toms(11, [2, 2.5, 3, 3.5], ['C4', 'C4', 'G3', 'D#3'], 0.9)   # the taiko fill (orig bar 12)
china(T(12), 1.0)
tri_fig(14, 16, gain=0.95)
snare_roll(15, [(.5, 4), (.5, 8)], 0.3, 0.9)
put('fx', S.noise_riser(B(8), 400, 10000, 2.5), T(14), 0.45)

# THEME 16-31 (orig 17-32): the horn theme on the pulse lead
crash(T(16), 1.0)
put('fx', S.impact(), T(16), 0.6)
groove(16, 32, 'main')
phrase_accents(16, 32)
ostinato(16, 32, gain=0.75)
sq_pattern(16, 32, gain=0.55, pan=-0.45, duty=.125, echo=False)
pings(16, 32, gain=0.6)
tri_bass(16, 32, 'oct8', gain=1.0, crush=0.55)
arps(16, 32, div=2, gain=0.6)
lead_line(16, HORN1, transpose=12)
lead_line(20, HORN2, transpose=12)
lead_line(24, HORN1, transpose=12)
lead_line(28, HORN4, transpose=12)
counter_line(24, STRINGS, gain=0.6, pan=0.4)
counter_line(28, STRINGS, gain=0.65, pan=0.4)

# THEME B 32-39 (orig 33-40): the strings' line leads, horn G holds; tritone figure + claves
crash(T(32), 0.9)
groove(32, 38, 'main')
phrase_accents(32, 36)
ostinato(32, 40, gain=0.8)
tri_bass(32, 38, 'oct8', gain=1.0, crush=0.55)
tri_bass(38, 40, 'pedal', gain=1.0)
arps(32, 38, div=2, gain=0.55)
lead_line(32, THEMEB_STR, gain=0.95)
counter_line(32, THEMEB_STR, transpose=-12, gain=0.55, pan=-0.4, duty=.25)
counter_line(32, [(0, 'G4', 8), (16, 'C5', 8)], gain=0.45, pan=0.45, duty=.5)
pings(32, 38, gain=0.6)
tri_fig(38, 40, gain=1.0)
for bt in CLAVES:
    put('ping', cached(('clv',), lambda: C.blip(m('D#6'))), T(38, bt), 0.9, 0.3)
kick(T(38, 0), 1.0)
china(T(38), 0.9)
for b in (38, 39):
    for i in range(8):
        hat(T(b, i * .5), 0.5)
snare_roll(39, [(.5, 4), (.5, 8)], 0.3, 0.9)

# DARK MIDDLE 40-55 (orig 41-56): piano dyads, the fretless tritone riff, whistle
put('fx', S.impact(), T(40), 0.7)
china(T(40), 0.9)
put('fx', S.downlifter(B(8)), T(40), 0.4)
groove(40, 56, 'half', hats_on=False, vel=0.85)
for b in range(40, 56):
    for i in range(8):
        hat(T(b, i * .5), 0.35 if i % 2 == 0 else 0.5)
    if b % 4 == 1:
        chip_sn(T(b, 3), 0.6)
        chip_sn(T(b, 3.5), 0.6)
    if b % 4 == 3:
        for bt in (2.5, 2.75, 3, 3.5):
            chip_sn(T(b, bt), 0.65)
    if b % 4 == 0:
        china(T(b), 0.6)
for b in range(52, 56):            # the cabasa 8ths (orig 53-56)
    for i in range(8):
        shaker(T(b, i * .5), 0.6 if i % 2 == 0 else 0.45, 0.35)
ostinato(40, 56, gain=0.7)
dyads(40, 56, gain=0.8)
tri_bass(40, 43, 'pedal', gain=1.0)
tri_line(43, [(0, 'C2', 2.5)] + RIFF_PICKUP, crush=0.6)
for b in range(44, 56, 2):
    tri_line(b, RIFF_C, crush=0.6)
    if b == 50:
        tri_line(b + 1, [(0, 'F#2', 4)], crush=0.6)     # orig bar 52: F#2 held
    else:
        tri_line(b + 1, RIFF_F2 if b % 4 == 2 else RIFF_F, crush=0.6)
tri_fig(46, 48, gain=0.6)
string_pad(40, 4, 'C6', 0.55)
string_pad(48, 4, 'C6', 0.6, pan=0.4)
counter_line(47, [(b + 2, n, l) for b, n, l in WHISTLE1], gain=0.7, pan=-0.3, duty=.25, echo=True)
lead_line(51, [(b + 2, n, l) for b, n, l in WHISTLE1], transpose=12, gain=0.85, duty0=.5, duty1=.5)
pings(48, 56, gain=0.5)

# BUILD 56-59 (orig 57-60): the semitone lift to C#, riser, roll
crash(T(56), 0.9)
groove(56, 58, 'half', key='C#')
for b in (58, 59):
    for bt in range(4):
        kick(T(b, bt), 0.9, 'C#')
ostinato(56, 60, 'G#3', 'A3', gain=0.85)
dyads(56, 60, gain=0.75)
tri_line(56, RIFF_C, transpose=1, crush=0.6)
tri_line(57, [(0, 'G2', 1), (1, 'C#2', .5), (1.5, 'G2', 2.5)], crush=0.6)
tri_line(58, RIFF_C, transpose=1, crush=0.6)
tri_line(59, [(0, 'G2', 1), (1, 'C#2', .5), (1.5, 'G1', 2.5)], crush=0.6)
lead_line(56, WHISTLE2, transpose=12, gain=0.85, duty0=.5, duty1=.5)
string_pad(56, 4, 'C#6', 0.7)
snare_roll(58, [(1, 2), (.5, 4), (.5, 8)], 0.2, 1.0)
put('fx', S.noise_riser(T(60) - T(56), 300, 11000, 2.5), T(56), 0.7)
chip_riser(58, 2, 'C#4', 'C#6', (0, 7, 12), 0.5)
put('cym', S.reverse_cymbal(B(4)), T(59), 0.6)
for i in range(8):                 # boss warning siren: the tritone C#6 / G6
    sig = C.pulse(m('G6') if i % 2 else m('C#6'), B(.95), .5, d=0.5, s=0.8, r=0.03, vib=0.35,
                  vib_rate=9.0, vib_delay=0.0)
    put('fx', sig, T(58, i), 0.35 * (0.6 + 0.4 * i / 7), -0.5 if i % 2 else 0.5)

# BOSS FIGHT 1 60-75: the theme in C# minor, double-time drums
for bb in (60, 64, 68, 72):
    crash(T(bb), 1.0)
put('fx', S.impact(), T(60), 1.0)
groove(60, 76, 'double', key='C#')
phrase_accents(60, 76, 0.9)
ostinato(60, 76, 'G#3', 'A3', div=4, gain=0.6)
tri_bass(60, 76, 'gallop', gain=1.0, crush=0.6)
arps(60, 76, div=4, gain=0.55, gate=.7)
lead_line(60, HORN1, transpose=13, gain=1.0, octave_up=0.3)
lead_line(64, HORN2, transpose=13, gain=1.0, octave_up=0.3)
lead_line(68, HORN1, transpose=13, gain=1.0, octave_up=0.35)
lead_line(72, HORN4, transpose=13, gain=1.0, octave_up=0.35)
counter_line(64, [(6, 'G#5', 2), (8, 'G#5', 8)], transpose=1, gain=0.5, pan=0.45)
counter_line(68, STRINGS, transpose=1, gain=0.6, pan=0.45)
counter_line(72, STRINGS, transpose=1, gain=0.65, pan=0.45)
sq_pattern(60, 76, transpose=1, gain=0.5, pan=-0.5, duty=.125, echo=False)
pings(60, 76, 'C#7', gain=0.55)

# BOSS FIGHT 2 76-87: the tritone riff in C#, the whistle line on top, then the
# piano tritone figure at double speed into the stage-clear hit
for bb in (76, 80):
    crash(T(bb), 1.0)
groove(76, 84, 'double', key='C#')
groove(84, 86, 'main', key='C#')
ostinato(76, 84, 'G#3', 'A3', div=4, gain=0.6)
dyads(76, 84, gain=0.7)
for b in range(76, 84, 2):
    tri_line(b, RIFF_C, transpose=1, crush=0.65)
    tri_line(b + 1, [(0, 'G2', 1), (1, 'C#2', .5), (1.5, 'G2', 1), (2.5, 'G1', 1), (3.5, 'G2', .5)],
             crush=0.65)
lead_line(76, WHISTLE2, transpose=12, gain=0.95, octave_up=0.0)
lead_line(80, WHISTLE2, transpose=12, gain=1.0, octave_up=0.4)
counter_line(80, WHISTLE2, transpose=7, gain=0.45, pan=-0.45, duty=.25)
pings(76, 84, 'C#7', gain=0.55)
tri_fig(84, 88, transpose=1, div=4, gain=1.0)
lead_line(84, [(0, 'G#4', 2), (2, 'D5', 2), (4, 'G5', 2), (6, 'C#5', 2),
               (8, 'G#5', 2), (10, 'D6', 2), (12, 'G6', 1), (13, 'C#6', 1)], gain=0.8, echo=True)
tri_bass(84, 88, 'oct8', gain=1.0, crush=0.5)
string_pad(84, 4, 'C#6', 0.7)
snare_roll(86, [(1, 4), (.75, 8)], 0.3, 1.0)
put('fx', S.noise_riser(T(88) - T(84), 400, 12000, 2.5), T(84), 0.7)
chip_riser(86, 2, 'G3', 'G5', (0, 6, 12), 0.5)
toms(87, [3, 3.25, 3.5, 3.75], ['C#4', 'G#3', 'D3', 'C#3'], 1.0)

# STAGE CLEAR 88-95: the theme resolved into C major
crash(T(88), 1.0)
china(T(88), 0.8)
put('fx', S.impact(), T(88), 1.0)
crash(T(92), 0.8)
groove(88, 96, 'main')
tri_bass(88, 96, 'oct8', gain=1.0, crush=0.5)
arps(88, 96, div=2, gain=0.65)
lead_line(88, VICTORY, transpose=12, gain=1.0, octave_up=0.3)
sq_pattern(88, 96, gain=0.55, pan=-0.45, duty=.125, echo=False)
pings(88, 96, gain=0.55)
counter_line(88, [(0, 'E5', 8), (8, 'E5', 8), (16, 'C5', 8), (24, 'D5', 8)], gain=0.45, pan=0.45)

# OUTRO 96-103: back on the C ostinato (the game loops it), thinning out
crash(T(96), 0.8)
groove(96, 100, 'half', vel=0.85)
for b in (100, 101):
    for i in range(8):
        shaker(T(b, i * .5), 0.4)
ostinato(96, 102, gain=0.85)
sq_pattern(96, 102, gain=0.9, pan=0.0, duty=.25)
tri_bass(96, 102, 'pedal', gain=0.85)
pings(96, 102, gain=0.6)
string_pad(96, 4, 'C6', 0.6)
string_pad(100, 2, 'C6', 0.5, pan=0.4)
# final hit: C power chord blip, triangle C, china, kick
kick(T(102), 1.0)
china(T(102), 1.0)
for n, g in (('C4', .7), ('G4', .6), ('C5', .6)):
    put('arp', C.pulse(m(n), B(4), .25, d=0.9, s=0.0, r=0.4), T(102), g)
put('tri', C.triangle(m('C2'), B(3), r=0.3), T(102), 1.0)
sig = C.pulse(m('G5'), B(.25), .25, d=0.14, s=0.45, r=0.03)
put('sq', sig, T(102), 0.9)
chip_echo('echo', sig, T(102), .75, .5, 6, 0.6, decay=0.7)
sig = C.pulse(m('C5'), B(1.5), .25, d=0.3, s=0.45, r=0.1)
put('sq', sig, T(102, .25), 0.9)
chip_echo('echo', sig, T(102, .25), .75, .5, 6, 0.6, decay=0.7)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_hard = S.sidechain(N, kicks, depth=0.7, release=0.22)
duck_mid = S.sidechain(N, kicks, depth=0.45, release=0.24)
duck_soft = S.sidechain(N, kicks, depth=0.25, release=0.2)

GAIN = {  # dB
    'kick': -5.5, 'snare': -6.5, 'noise': -9.0, 'cym': -17.0, 'tri': -7.0, 'crush': -2.0,
    'lead': -1.5, 'echo': -3.0, 'sq': 0.0, 'ost': -0.5, 'arp': -2.0, 'ping': -5.0,
    'counter': -6.0, 'fig': -2.0, 'fx': -15.0,
}
DUCK = {'tri': duck_soft, 'crush': duck_hard, 'arp': duck_mid, 'ost': duck_mid, 'ping': duck_soft,
        'counter': duck_soft, 'fig': duck_soft, 'echo': duck_mid, 'sq': duck_soft, 'lead': duck_soft,
        'fx': duck_soft}
VERB = {'snare': .12, 'noise': .05, 'lead': .12, 'echo': .2, 'sq': .12, 'ost': .1, 'arp': .15,
        'ping': .3, 'counter': .3, 'fig': .2, 'fx': .25}

# bus processing
bus['crush'].x = stage(bus['crush'].x, Bitcrush(bit_depth=5), HighpassFilter(90), LowpassFilter(2600))
bus['tri'].x = stage(bus['tri'].x, LowpassFilter(9000))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(150), PeakFilter(2500, 1.5, 0.9),
                      LowpassFilter(11000))
bus['echo'].x = stage(bus['echo'].x, HighpassFilter(300), LowpassFilter(6500))
bus['sq'].x = stage(bus['sq'].x, HighpassFilter(250), LowpassFilter(10000))
bus['ost'].x = stage(bus['ost'].x, HighpassFilter(140), LowpassFilter(9000))
bus['arp'].x = stage(bus['arp'].x, HighpassFilter(200), LowpassFilter(10000))
bus['ping'].x = stage(bus['ping'].x, HighpassFilter(800), LowpassFilter(9000))
bus['counter'].x = stage(bus['counter'].x, HighpassFilter(250), LowpassFilter(11000))
bus['fig'].x = stage(bus['fig'].x, HighpassFilter(200), LowpassFilter(10000))
bus['noise'].x = stage(bus['noise'].x, HighpassFilter(400), LowpassFilter(11000),
                       HighShelfFilter(7000, -3.0, 0.7))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(120), PeakFilter(200, 2.0, 1.0), LowpassFilter(10000),
                       HighShelfFilter(6000, -4.0, 0.7))
bus['kick'].x = stage(bus['kick'].x, LowpassFilter(9000))
bus['cym'].x = stage(bus['cym'].x, HighpassFilter(3000), LowpassFilter(13000))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(40))

# section filter: the intro opens up from a dull, muffled chip
bars_axis = (np.arange(N) / SR + OFF) / (4 * SPB)
sweep = np.interp(bars_axis, [0, 7.9, 8], [900, 9000, 20000]).astype(float)
sweep = np.minimum(sweep, 20000)
for k in ('sq', 'ost', 'counter', 'ping'):
    if BAR0 < 8:
        bus[k].x = S.svf(bus[k].x, sweep, 0.8).astype(np.float32)

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('SONG4_MUTE', '').split(',')))
for k in names:
    if k in MUTE:
        bus[k] = None
        continue
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    bus[k] = None
    if k in DUCK:
        x = x * DUCK[k][None, :]
    seg = x.T.astype(np.float64)
    act = np.abs(seg).max(axis=1) > 1e-4
    levels[k] = pyln.Meter(SR).integrated_loudness(seg) if act.any() else -99.0
    del seg
    if os.environ.get('SONG4_STEMS'):
        sf_.write(os.path.join(os.environ['SONG4_STEMS'], k + '.wav'), x.T, SR, subtype='FLOAT')
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    del x

pre = int(0.015 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(400), LowpassFilter(8000),
             Reverb(room_size=0.6, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
verb = verb * duck_soft[None, :]
mix = mix + verb * np.float32(0.8)
del verb

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -6), (7.9, -4), (8, -3), (15.9, -2), (16, -0.5), (31.9, -0.5), (32, -1), (39.9, -1.5),
              (40, -5.5), (55.9, -4.5), (56, -4), (59.9, -1), (60, 1.5), (75.9, 1.5), (76, 1.5), (87.9, 2),
              (88, 0.5), (95.9, 0.5), (96, -3), (101.9, -4), (102, -3), (104, -3)]
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# mono low end: below 120 Hz both sides get the mid signal
lo = S.butter(mix, 120, 'low', 4).astype(np.float32)
mid_lo = lo.mean(axis=0, keepdims=True)
mix = mix - lo + mid_lo
del lo, mid_lo

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(80, 0.5, 0.7), HighShelfFilter(10000, -1.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=15, release_ms=120))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=60))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.5 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'song4_chiptune.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
