#!/usr/bin/env python3
# new09_spacesynth.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Far Sun Meridian": new track 09, classic space synth in the manner of
# Jean-Michel Jarre (Oxygene / Equinoxe), Vangelis (Blade Runner), Space and
# Tangerine Dream -- string-machine pads through a phaser, a sequenced
# Mini-Moog bass, a portamento lead, CS-80 brass, a soft Minipops-style
# rhythm box, a theremin-like voice and a little wind.  Everything is
# synthesized in code (synth.py, synth_new09.py); no samples.
#
# Credits: the original music is Stardock's (Stellar Frontier, 1997,
# mid/sf-music6.mid and mid/sf-music7.mid, Stardock Shared Source licence).
# This is a NEW composition built from fragments of those two songs that the
# existing remixes (song6_symphonic_metal.py, song7_orchestral.py) do not
# feature.  Bar numbers count from each MIDI's first downbeat (both files
# start with a two-beat pickup).
#
# From sf-music7.mid (C minor / C-Db Phrygian, 155 BPM):
#   - the clarinet phrase   C4 . . Eb4 | Db4 Bb3 | G3      (ch "Clarinet", bars 52-54)
#       -> theme A: sung by the lead, sequenced up a diatonic third
#          (Eb G | F Db | C) and re-harmonised Cm Db Cm Bb | Eb Db Cm G
#   - the coda inner voices C4 | D4(1.5) Bb3(2.5) and Eb4 | D4 over the bass
#       C D Eb F             (synth brass + piano, bars 62-69)
#       -> theme B's CS-80 brass voicings and its 4 | 1.5 + 2.5 rhythm, the
#          rising-third sequence of the lead melody over Cm Bb/D Cm/Eb Bb/F
#          Ab Bb Ab Bb
#   - the opening pad chords C | Db | C | Bbm/Db             (bars 0-3)
#       -> the intro progression (Cm Db Bbm)
# From sf-music6.mid (E-flat minor / E major, 180 BPM):
#   - the bass fill         Eb2 Eb2 Gb2 Ab2 Bb2 Bb1          (bars 49, 65, 73)
#       -> the 16-step Moog sequence (root root 8va root | 3 4 5 5-down ...)
#   - the high string line  Eb6 (14 beats) Gb6 | Ab6 (12) E6  (ch2/ch3, bars 48-55)
#       -> the intro line (transposed to C6 Eb6 F6) and the theremin line of
#          the E-flat-minor episode, over the original's Ebm -> E major shift
#   - the E/B bass          B1 . . E2 | B1 Ab1 | B1 . . Ab1 | E2 B1  (bars 52-55)
#   - the trombone sigh     Eb4 over Ab3(1.5) -> Gb3(5.5)    (bars 64-75)
#   - the rising chime      E5 | Ab5 | B5 | E6 on beat 4     (piano, bars 76-79)
#   - the orchestra-hit arpeggio B4 Ab4 E4 | B3 | . B3 E4 Ab4 (bars 95-97)
#       -> glass bells in the E-major bars
#   - the Eb-minor neighbour line Eb4 | F4 Gb4 | Eb4          (piano, bars 48-50)
# New: 108 BPM, the form (rubato-feeling intro, theme A, theme B, the
# E-flat-minor / E-major episode, the G-major turn back to C minor, the
# climax, theme B as the finale and a final C-minor chord that loops back to
# the intro), all harmonisation beyond the fragments, the sound design and
# the mix.
#
#   python3 tools/music/edm/new09_spacesynth.py OUT.wav [BAR0 BAR1]
#   NEW09_MUTE=kick,snare,hats,perc  mutes buses (for the chroma check)
import gc
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
import scipy.signal as ss
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new09 as V  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 108.0
SPB = 60.0 / BPM
NBARS = 76
TAIL = 3.2
BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
OFFSET = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + (TAIL if BAR1 >= NBARS else 0)) * SR)
MUTE = set(filter(None, os.environ.get('NEW09_MUTE', '').split(',')))


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - OFFSET


def B(beats):
    return beats * SPB


# ------------------------------------------------------------- harmony
# name: (bass note, sequence root, quality, string-machine voicing)
CH = {
    'Cm': ('C2', 'C2', 'm', ['G3', 'C4', 'Eb4', 'G4']),
    'Db': ('Db2', 'Db2', 'M', ['Ab3', 'Db4', 'F4', 'Ab4']),
    'Bb': ('Bb1', 'Bb1', 'M', ['F3', 'Bb3', 'D4', 'F4']),
    'Bbm': ('Bb1', 'Bb1', 'm', ['F3', 'Bb3', 'Db4', 'F4']),
    'Eb': ('Eb2', 'Eb2', 'M', ['G3', 'Bb3', 'Eb4', 'G4']),
    'Ab': ('Ab1', 'Ab1', 'M', ['Ab3', 'C4', 'Eb4', 'Ab4']),
    'G': ('G1', 'G2', 'M', ['G3', 'B3', 'D4', 'G4']),
    'G7': ('G1', 'G2', 'M', ['G3', 'B3', 'D4', 'F4']),
    'Bb/D': ('D2', 'Bb1', 'M', ['F3', 'Bb3', 'D4', 'F4']),
    'Cm/Eb': ('Eb2', 'C2', 'm', ['G3', 'C4', 'Eb4', 'G4']),
    'Bb/F': ('F2', 'Bb1', 'M', ['F3', 'Bb3', 'D4', 'F4']),
    'Ebm': ('Eb2', 'Eb2', 'm', ['Gb3', 'Bb3', 'Eb4', 'Gb4']),
    'E/B': ('B1', 'E2', 'M', ['G#3', 'B3', 'E4', 'G#4']),
    'E': ('E2', 'E2', 'M', ['G#3', 'B3', 'E4', 'G#4']),
}
A_PROG = ['Cm', 'Db', 'Cm', 'Bb', 'Eb', 'Db', 'Cm', 'G']
B_PROG = ['Cm', 'Bb/D', 'Cm/Eb', 'Bb/F', 'Ab', 'Bb', 'Ab', 'Bb']
CHORDS = (['Cm'] * 4 + ['Db', 'Db', 'Bbm', 'Bbm']          # 0-7   intro
          + A_PROG * 2                                     # 8-23  theme A
          + B_PROG                                         # 24-31 theme B
          + ['Ebm'] * 4 + ['E/B'] * 4                      # 32-39 episode
          + ['Ebm'] * 4 + ['E'] * 4                        # 40-47
          + ['G', 'G7']                                    # 48-49 turn
          + A_PROG * 2                                     # 50-65 climax
          + B_PROG                                         # 66-73 finale
          + ['Cm', 'Cm'])                                  # 74-75 final chord
assert len(CHORDS) == NBARS, len(CHORDS)

# ------------------------------------------------------------- themes
# (beat, note, beats), relative to the phrase's first bar
A_THEME = [(0, 'C5', 3), (3, 'Eb5', 1), (4, 'Db5', 2), (6, 'Bb4', 2),       # song 7 clarinet
           (8, 'G4', 5), (13, 'Ab4', 1), (14, 'Bb4', 1), (15, 'D5', 1),
           (16, 'Eb5', 3), (19, 'G5', 1), (20, 'F5', 2), (22, 'Db5', 2),    # ... up a third
           (24, 'C5', 4), (28, 'D5', 2), (30, 'B4', 2)]
A_THEME2 = A_THEME[:-3] + [(24, 'C5', 3), (27, 'Eb5', 1), (28, 'D5', 4)]
A_THEME3 = A_THEME[:-3] + [(24, 'C5', 4), (28, 'B4', 2), (30, 'D5', 2)]
A_HARM = [(0, 'G4', 3), (3, 'C5', 1), (4, 'Ab4', 2), (6, 'F4', 2), (8, 'Eb4', 5), (13, 'F4', 3),
          (16, 'G4', 3), (19, 'Bb4', 1), (20, 'Db5', 2), (22, 'Ab4', 2), (24, 'G4', 4),
          (28, 'B4', 2), (30, 'G4', 2)]
B_THEME = [(0, 'G5', 4), (4, 'F5', 1.5), (5.5, 'D5', 2.5), (8, 'G5', 4), (12, 'Bb5', 1.5),
           (13.5, 'F5', 2.5), (16, 'C6', 4), (20, 'D6', 1.5), (21.5, 'Bb5', 2.5),
           (24, 'Eb6', 1.5), (25.5, 'C6', 2.5), (28, 'D6', 4)]
B_FINAL = B_THEME + [(32, 'C6', 7)]
# song 7 bars 62-69: brass inner voices, rhythm 4 | 1.5 + 2.5
B_BRASS = [(0, ['G3', 'C4', 'Eb4'], 4), (4, ['F3', 'Bb3', 'D4'], 1.5), (5.5, ['D3', 'F3', 'Bb3'], 2.5),
           (8, ['G3', 'C4', 'Eb4'], 4), (12, ['F3', 'Bb3', 'D4'], 1.5), (13.5, ['D3', 'F3', 'Bb3'], 2.5),
           (16, ['Ab3', 'C4', 'Eb4'], 4), (20, ['F3', 'Bb3', 'D4'], 1.5), (21.5, ['D3', 'F3', 'Bb3'], 2.5),
           (24, ['Ab3', 'C4', 'Eb4'], 4), (28, ['F3', 'Bb3', 'D4'], 4)]
# song 6: the high string line, transposed to C for the intro
INTRO_LINE = [(0, 'C6', 14), (14, 'Eb6', 2), (16, 'F6', 15)]
# song 6 bars 48-55 / 48-50: the episode's theremin lines
EP1 = [(0, 'Eb5', 14), (14, 'Gb5', 2), (16, 'G#5', 12), (28, 'E5', 4)]
EP2 = [(2, 'Eb5', 2), (4, 'F5', 2), (6, 'Gb5', 2), (8, 'Eb5', 6), (14, 'Gb5', 2),
       (16, 'G#5', 6), (22, 'F#5', 2), (24, 'E5', 4), (28, 'B4', 4)]
# the turn (G, G7) into the climax
TURN = [(0, 'B4', 2), (2, 'D5', 2), (4, 'G5', 3), (7, 'F5', .5), (7.5, 'D5', .5)]
# song 6 bars 95-97: orchestra-hit arpeggio -> glass bells (E bars)
EP_BELLS = [(0, 'B5', 1.5), (1.5, 'G#5', 1.5), (3, 'E5', 1), (4, 'B4', 4),
            (10.5, 'B4', .5), (11, 'E5', .5), (11.5, 'G#5', .5), (12, 'B5', 4)]
# song 6 bars 76-79: rising chime on beat 4
CHIME = [(3, 'E5', 1), (7, 'G#5', 1), (11, 'B5', 1), (15, 'E6', 1)]
# song 6 bars 49/65: bass fill -> sequence (semitones from the root)
SEQ_MIN = [0, 0, 12, 0, 3, 5, 7, -5, 0, 0, 12, 0, 7, 5, 3, -5]
SEQ_ACC = [1.0, .65, .85, .7, .9, .75, .85, .7, .95, .65, .85, .7, .85, .75, .8, .7]
# song 6 bars 52-55: the E/B bass
EB_BASS = [(0, 'B1', 3), (3, 'E2', 1), (4, 'B1', 2), (6, 'G#1', 2), (8, 'B1', 3), (11, 'G#1', 1),
           (12, 'E2', 2), (14, 'B1', 2)]

# --------------------------------------------------------------- buses
names = ['kick', 'snare', 'hats', 'perc', 'sub', 'seq', 'lead', 'ther', 'brass', 'glass', 'bell', 'wind']
bus = {k: S.Bus(N) for k in names}
pad_mono = np.zeros(N, np.float32)   # string machine (mono, ensemble + phaser later)
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def add_mono(target, sig, t, gain=1.0, pan=0.0):
    sig = np.asarray(sig, np.float32)
    st = np.stack([sig * np.float32(np.sqrt((1 - pan) / 2) * np.sqrt(2)),
                   sig * np.float32(np.sqrt((1 + pan) / 2) * np.sqrt(2))])
    bus[target].add(st, t, gain)


def add_pad(sig, t, gain=1.0):
    i = int(round(t * SR))
    if i >= N:
        return
    sig = np.asarray(sig, np.float32)
    if i < 0:
        sig = sig[-i:]
        i = 0
    k = min(len(sig), N - i)
    pad_mono[i:i + k] += sig[:k] * np.float32(gain)


def in_window(bar0, bar1):
    return bar1 > BAR0 and bar0 < BAR1


# ------------------------------------------------------------ writers

def pads(bar0, bar1, vel=1.0, a=0.55, r=1.3, bright=1.0, octave=0):
    if not in_window(bar0, bar1 + 1):
        return
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        dur = T(e) - T(b)
        for note in CH[ch][3]:
            add_pad(V.string_voice(m(note) + 12 * octave, dur, 1.0, a, r, bright), T(b), vel)
        b = e


def line_strings(bar0, notes, vel=1.0, a=0.8, r=1.5):
    for beat, note, dur in notes:
        add_pad(V.string_voice(m(note), B(dur), 1.0, a, r, 1.1), T(bar0, beat), vel)


def seq_bass(bar0, bar1, vel=1.0, cut=380.0, env=2400.0, ramp=None, gate=0.72):
    """16th-note Moog sequence on the chord of each bar (song 6's bass fill)."""
    for b in range(max(bar0, BAR0 - 1), min(bar1, BAR1)):
        ch = CHORDS[b]
        bass, root, q, _ = CH[ch]
        r0 = m(root)
        while r0 < 32:
            r0 += 12
        while r0 > 43:
            r0 -= 12
        bn = m(bass)
        while bn - r0 > 6:
            bn -= 12
        while r0 - bn > 6:
            bn += 12
        u = 1.0 if ramp is None else np.clip((b - bar0 + 0.5) / ramp, 0.15, 1.0)
        for i in range(16):
            iv = SEQ_MIN[i]
            if q == 'M' and iv == 3:
                iv = 4
            md = (bn if i in (0, 1, 8, 9) else r0) + iv
            if md < 28:
                md += 12
            c = cut * (0.6 + 0.4 * u) * (1 + 0.35 * np.sin(2 * np.pi * (b * 16 + i) / 128.0))
            v = SEQ_ACC[i] * vel * u
            key = ('seq', md, round(c, -1), round(v, 2))
            sig = cached(key, lambda: V.moog_note(md, B(.25) * gate, v, cut=c, env=env * u))
            add_mono('seq', sig, T(b, i * .25), 1.0)


def long_bass(bar0, notes, vel=1.0):
    for beat, note, dur in notes:
        add_mono('seq', V.moog_note(m(note), B(dur) * 0.95, vel, cut=260, env=900, decay=0.25, q=1.2),
                 T(bar0, beat), 1.0)


def sub(bar0, bar1, vel=1.0):
    if not in_window(bar0, bar1):
        return
    b = bar0
    while b < bar1:
        bn = CH[CHORDS[b]][0]
        e = b
        while e < bar1 and CH[CHORDS[e]][0] == bn:
            e += 1
        md = m(bn)
        if md > 40:
            md -= 12
        bus['sub'].add(S.sub_bass(md, T(e) - T(b) - 0.03, 1.0, a=0.02, r=0.08, drive=1.2), T(b), vel)
        b = e


def lead(bar0, notes, vel=1.0, transpose=0, kind='odyssey', target='lead', **kw):
    if not in_window(bar0, bar0 + max(b_ + d_ for b_, _, d_ in notes) / 4 + 1):
        return
    nn = [(b_, m(n_) + transpose, d_) for b_, n_, d_ in notes]
    t0 = nn[0][0]
    sig = V.mono_lead([(b_ - t0, md, d_) for b_, md, d_ in nn], SPB, kind=kind, **kw)
    add_mono(target, sig, T(bar0, t0), vel)


def brass_chords(bar0, hits, vel=1.0, transpose=0, **kw):
    for beat, chord, dur in hits:
        for note in chord:
            bus['brass'].add(V.cs80_brass(m(note) + transpose, B(dur) - 0.03, 1.0, **kw), T(bar0, beat), vel)


def brass_line(bar0, notes, vel=1.0, transpose=0, **kw):
    for beat, note, dur in notes:
        bus['brass'].add(V.cs80_brass(m(note) + transpose, B(dur) - 0.04, 1.0, **kw), T(bar0, beat), vel)


def bells(bar0, notes, vel=1.0, transpose=0):
    for beat, note, dur in notes:
        bus['bell'].add(S.bell(m(note) + transpose, B(dur) * 0.5, r=1.4), T(bar0, beat), vel)


GLASS_PAT = [0, 2, 1, 3, 2, 1, 3, 2]


def glass_seq(bar0, bar1, vel=1.0):
    for b in range(max(bar0, BAR0 - 1), min(bar1, BAR1)):
        tones = sorted(m(v) + 12 for v in CH[CHORDS[b]][3])
        for i in range(16):
            md = tones[GLASS_PAT[i % 8]]
            acc = 1.0 if i % 4 == 0 else (0.75 if i % 2 == 0 else 0.6)
            sig = cached(('gl', md), lambda: V.glass(md, B(.22)))
            add_mono('glass', sig, T(b, i * .25), vel * acc, pan=0.35 * np.sin(i * np.pi / 4))


# rhythm box -------------------------------------------------------------
_sn_rr = [0]


def snare_hit(t, vel):
    k = _sn_rr[0] % 4
    _sn_rr[0] += 1
    sig = cached(('sn', k), lambda: V.snare(k))
    add_mono('snare', sig, t, vel * (0.94 + 0.12 * S.rng().random()), pan=0.05)


def drums(bar0, bar1, style='full', vel=1.0):
    for b in range(max(bar0, BAR0 - 1), min(bar1, BAR1)):
        kbeats = (0, 2) if style in ('light', 'half') else (0, 2, 2.75)
        if style == 'half':
            kbeats = (0,)
        for kb in kbeats:
            add_mono('kick', cached(('k',), V.kick), T(b, kb), vel * (1.0 if kb == 0 else 0.85))
        if style == 'full':
            for sb in (1, 3):
                snare_hit(T(b, sb), vel * (0.9 if sb == 1 else 1.0))
            if b % 4 == 3:
                snare_hit(T(b, 3.75), vel * 0.35)
        for i in range(8):
            hv = (0.55 if i % 2 else 0.85) * vel
            add_mono('hats', cached(('h', i % 3), lambda: V.hat(1.0, i % 3)), T(b, i * .5), hv, pan=-0.3)
        for i in range(16):
            sv = (0.5 if i % 4 == 2 else 0.3) * vel
            add_mono('hats', cached(('sh', i % 4), lambda: V.shaker(1.0, i % 4)), T(b, i * .25), sv, pan=0.35)
        if style in ('full', 'light'):
            for beat, md, v in ((0.75, 'C4', .55), (1.5, 'G3', .6), (3.25, 'C4', .45), (3.5, 'G3', .55)):
                add_mono('perc', cached(('bo', md), lambda: V.bongo(m(md))), T(b, beat), v * vel,
                         pan=0.4 if md == 'C4' else -0.4)


def fill(bar, vel=1.0):
    for beat, md in ((2.5, 'G3'), (2.75, 'Eb3'), (3.0, 'C3'), (3.25, 'C3'), (3.5, 'G2'), (3.75, 'G2')):
        add_mono('perc', cached(('tom', md), lambda: V.tom(m(md))), T(bar, beat), vel * 0.8,
                 pan={'G3': .4, 'Eb3': .2, 'C3': 0, 'G2': -.3}[md])


# ---------------------------------------------------------- arrangement
# INTRO 0-7: wind, string machine (song 7's opening chords), song 6's high
# string line in C; the sequencer fades in with the filter opening.
bus['wind'].add(V.wind(9.0, 260, 1100, seed=1), T(0), 1.0)
pads(0, 8, vel=0.9, a=1.6, r=2.0, bright=0.8)
line_strings(0, INTRO_LINE, vel=0.55, a=1.5)
seq_bass(4, 8, vel=0.75, cut=200, env=1400, ramp=4)
bells(6, [(3, 'G5', 1)], vel=0.35)

# THEME A 8-23: rhythm box, the sequence, the lead sings song 7's clarinet
# phrase; the second pass adds snare, bongos and the glass sequence.
drums(8, 16, 'light', 0.8)
drums(16, 24, 'full', 0.9)
fill(23, 0.8)
seq_bass(8, 24, vel=0.9, cut=380, env=2400)
sub(8, 24, 0.55)
pads(8, 16, vel=0.75, a=0.6, bright=0.9)
pads(16, 24, vel=0.85, a=0.5, bright=1.0)
lead(8, A_THEME, vel=1.0)
lead(16, A_THEME2, vel=1.0)
glass_seq(16, 24, vel=0.45)

# THEME B 24-31: song 7's coda voices on CS-80 brass, the lead climbs in thirds
drums(24, 32, 'full', 0.95)
seq_bass(24, 32, vel=0.95, cut=450, env=2600)
sub(24, 32, 0.6)
pads(24, 32, vel=0.7, a=0.4, bright=1.0)
brass_chords(24, B_BRASS, vel=0.55)
lead(24, B_THEME, vel=0.95, transpose=-12)
bus['wind'].add(V.wind(6.0, 300, 1600, seed=2), T(30, 2), 0.8)

# EPISODE 32-47: E-flat minor -> E major (song 6), drums out, theremin line
pads(32, 40, vel=0.85, a=1.2, r=1.6, bright=0.75)
seq_bass(32, 36, vel=0.6, cut=220, env=900)
long_bass(36, EB_BASS, vel=0.7)
lead(32, EP1, vel=0.9, kind='theremin', target='ther', glide=0.12, vib=0.22)
bells(36, EP_BELLS, vel=0.4)
drums(40, 48, 'half', 0.7)
seq_bass(40, 48, vel=0.8, cut=300, env=1800)
sub(32, 48, 0.4)
pads(40, 48, vel=0.8, a=0.8, bright=0.85)
brass_chords(40, [(0, ['Eb4'], 7.5), (0, ['Ab3'], 1.5), (1.5, ['Gb3'], 5.5),
                  (8, ['Eb4'], 7.5), (8, ['Ab3'], 1.5), (9.5, ['Gb3'], 5.5)], vel=0.45, swell=0.6)
lead(40, EP2, vel=0.9, kind='theremin', target='ther', glide=0.12, vib=0.22)
bells(44, CHIME, vel=0.5)
bells(44, CHIME, vel=0.25, transpose=-12)

# THE TURN 48-49: G -> G7 (B is the common tone with E major), back to C minor
pads(48, 50, vel=0.85, a=0.3, bright=1.0)
seq_bass(48, 50, vel=0.9, cut=420, env=2600)
sub(48, 50, 0.55)
brass_chords(48, [(0, ['G3', 'B3', 'D4'], 4), (4, ['G3', 'B3', 'D4', 'F4'], 4)], vel=0.5, swell=0.8)
lead(48, TURN, vel=1.0)
drums(48, 49, 'light', 0.8)
fill(49, 1.0)
bus['wind'].add(V.wind(5.0, 400, 2200, seed=3), T(47, 3), 0.9)

# CLIMAX 50-65: theme A in full, lead doubled an octave up, CS-80 harmony
drums(50, 66, 'full', 1.0)
fill(57, 0.9)
fill(65, 1.0)
seq_bass(50, 66, vel=1.0, cut=520, env=2800)
sub(50, 66, 0.65)
pads(50, 66, vel=0.9, a=0.3, bright=1.1)
pads(50, 66, vel=0.35, a=0.6, bright=1.0, octave=1)
lead(50, A_THEME, vel=1.0)
lead(58, A_THEME3, vel=1.0)
lead(50, A_THEME, vel=0.4, transpose=12, kind='theremin', target='ther', glide=0.08, vib=0.15)
lead(58, A_THEME3, vel=0.4, transpose=12, kind='theremin', target='ther', glide=0.08, vib=0.15)
brass_line(50, A_HARM, vel=0.55)
brass_line(58, A_HARM, vel=0.6)
glass_seq(50, 66, vel=0.55)

# FINALE 66-73: theme B up high, brass, and the final C-minor chord (74)
drums(66, 74, 'full', 1.0)
fill(73, 0.9)
seq_bass(66, 74, vel=1.0, cut=560, env=3000)
sub(66, 74, 0.65)
pads(66, 74, vel=0.9, a=0.3, bright=1.1)
pads(66, 74, vel=0.35, a=0.6, bright=1.0, octave=1)
brass_chords(66, B_BRASS, vel=0.65)
lead(66, B_FINAL, vel=1.0)
glass_seq(66, 74, vel=0.5)
# final chord: string machine, CS-80, low C, one kick; rings out
add_mono('kick', V.kick(), T(74), 1.0)
for note in ['C3', 'G3', 'C4', 'Eb4', 'G4', 'C5']:
    add_pad(V.string_voice(m(note), B(5), 1.0, 0.05, 1.0, 1.0), T(74), 0.9)
brass_chords(74, [(0, ['C3', 'G3', 'C4', 'Eb4', 'G4'], 5)], vel=0.6, swell=0.25, r=0.8)
bus['sub'].add(S.sub_bass(36, B(5), 1.0, a=0.01, r=0.6, drive=1.2), T(74), 0.7)
bus['seq'].add(np.stack([V.moog_note(36, B(5), 0.9, cut=300, env=1500, decay=0.3, r=0.5)] * 2), T(74), 1.0)
bells(74, [(0, 'C6', 4), (0, 'G5', 4)], vel=0.35)
_cache.clear()
gc.collect()

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


# string machine: ensemble chorus, then the phaser
pad = V.ensemble(pad_mono)
del pad_mono
pad = V.phaser(pad, rate=0.09, fmin=200, fmax=3000, stages=6, fb=0.5, mix=0.5).astype(np.float32)
pad = stage(pad, HighpassFilter(140), LowpassFilter(10000), PeakFilter(420, -3.0, 0.8),
            PeakFilter(900, -1.5, 0.7), HighShelfFilter(4500, 2.5, 0.7))
bus['pad'] = S.Bus(1)
bus['pad'].x = pad
del pad
names.append('pad')
gc.collect()

GAIN = {  # dB
    'kick': -11.0, 'snare': -13.0, 'hats': -14.0, 'perc': -13.5, 'sub': -16.0, 'seq': -11.5,
    'lead': -6.0, 'ther': -9.0, 'brass': -5.5, 'glass': -12.0, 'bell': -11.0, 'wind': -22.0,
    'pad': -1.5,
}
VERB = {'snare': .18, 'perc': .2, 'pad': .3, 'lead': .3, 'ther': .5, 'brass': .35, 'glass': .35,
        'bell': .5, 'wind': .4, 'seq': .06}
DELAY = {'lead': .32, 'ther': .3, 'glass': .35, 'bell': .3}

bus['seq'].x = stage(bus['seq'].x, HighpassFilter(38), LowpassFilter(6000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(150))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(180), PeakFilter(1400, 1.5, 0.9), LowpassFilter(9000))
bus['ther'].x = stage(bus['ther'].x, HighpassFilter(200))
bus['brass'].x = stage(bus['brass'].x, HighpassFilter(130), LowpassFilter(8000))
bus['glass'].x = stage(bus['glass'].x, HighpassFilter(400), LowpassFilter(9000))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(3000), LowpassFilter(12000))
bus['wind'].x = stage(bus['wind'].x, HighpassFilter(150), LowpassFilter(5000))
bus['kick'].x = stage(bus['kick'].x, LowpassFilter(3000))

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
for k in names:
    if k in MUTE:
        bus[k] = None
        continue
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    act = np.max(np.abs(x), axis=0) > 1e-4
    levels[k] = meter.integrated_loudness(x[:, act].T.astype(np.float64)) if act.sum() > SR else -99.0
    mix += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    if k in DELAY:
        delay_send += x * np.float32(DELAY[k])
    bus[k] = None
    del x
    gc.collect()

pre = int(0.03 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(300), LowpassFilter(7000),
             Reverb(room_size=0.93, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
dly = S.pingpong(delay_send, B(0.75), feedback=0.45, repeats=9, lp=4200, hp=400)
del delay_send
dly = stage(dly, Reverb(room_size=0.6, damping=0.6, wet_level=0.3, dry_level=1.0, width=1.0))
mix = mix + verb * np.float32(0.85) + dly * np.float32(0.5)
del verb, dly
gc.collect()

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -3), (7.9, -3), (8, -4), (15.9, -3.5), (16, -3), (23.9, -2.5), (24, -1.5), (31.9, -1),
              (32, -7), (39.9, -7), (40, -5.5), (47.9, -5), (48, -3), (49.9, -0.5),
              (50, 0), (65.9, 0), (66, 0.5), (74, 0.5), (80, 0.5)]
bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(100, 0.5, 0.7), HighShelfFilter(9000, -1.0, 0.7),
            Compressor(threshold_db=-15, ratio=1.8, attack_ms=25, release_ms=200))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=100))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))


def true_peak_db(x):
    up = ss.resample_poly(x, 4, 1, axis=1)
    return 20 * np.log10(np.max(np.abs(up)) + 1e-12)


tp = true_peak_db(mix)
if tp > -1.2:   # tame isolated overs without changing the loudness much
    mix = stage(mix, Limiter(threshold_db=-1.2 - (tp + 1.2) * 0.0, release_ms=60))
    loud = meter.integrated_loudness(mix.T.astype(np.float64))
    mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))
    tp = true_peak_db(mix)

# trim to the last audible sample, short fade
if BAR1 >= NBARS:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
    mix = mix[:, : last + 1]
    fade = int(1.2 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new09.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s', f'LUFS {meter.integrated_loudness(mix.T.astype(np.float64)):.2f}',
      f'TP {tp:.2f} dBTP')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
