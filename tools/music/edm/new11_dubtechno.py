#!/usr/bin/env python3
# new11_dubtechno.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Dead Reckoning": new track 11, dub techno (Basic Channel / Maurizio /
# Deepchord), 118 BPM, C# minor, energy tag CRUISE.  A NEW piece (not a remix):
# a soft round kick, chord stabs drenched in filtered ping-pong delay and a long
# hall, a slowly evolving low-pass, a deep sub and one quiet recognisable motif.
#
# Credits: the source music is Stardock's (the original Stellar Frontier game
# music, mid/sf-music2.mid and mid/sf-music5.mid).  Composed, synthesized and
# mixed in code with tools/music/edm/synth.py, synth_new10.py and
# synth_new11.py: no samples, no SoundFonts.
#
# Built from (bar numbers are the original MIDIs', 1-based):
#   sf-music5.mid (C# minor; its phrases start on beat 3 of the MIDI bars, so
#   every figure below is shifted by two beats onto the true downbeat)
#   - the second string-stab rhythm of bars 17-31 (String Ensemble, insts 18/21:
#     C#6 on beats 3, 3-and, 4-and, with the B5 / F#5 quarter pickup on beat 2
#     of every other bar; song5_edm.py used only the bars 1-16 stab C#6/G#5):
#     shifted, it is  x x . x | . . . (x) | x x . x | . . . .  -- the rhythm
#     of the chord stab all through the track, with its pickup as a soft
#     E/G#/B upper-structure stab on beat 4
#   - the string counter-figure of bars 59-73 (inst 14: C#5 C#5-- with the
#     Ab4 pickup every four bars, under the electric-piano hook; unused by
#     song5_edm.py and new05_goa.py): G#4 C#5 C#5-- is the track's only melody,
#     on a soft FM electric piano drowned in delay (from bar 16; an octave-up
#     double in the peaks)
#   - the timpani answer of bars 7 / 15 (Ab2 E2, Ab2 E2 Ab2) and of bar 39
#     (Ab2 . F#2): the bass fills at the ends of the phrases (G# E -> C#, and
#     G# . F# into the F#m bars)
#   sf-music2.mid (C# major / F# minor section, bars 41-56)
#   - the String Ensemble 2 inner voices (insts 12/13): the pair F4/Ab4 rising
#     to F#4/A4 every two bars (C# -> F#m); new02_proghouse.py used only the
#     bars 1-20 voices.  Here, in C# minor, the pair is E4/G#4 -> F#4/A4 (the
#     F is lowered to E: logged change, the track is minor throughout) and the
#     whole stab chord planes up a step with it: C#m9 (E G# B D#) -> F#m7
#     (F# A C# E).  This two-bar i-iv sway is the B section and the peaks.
#   - the A1 bass under F#m of bar 43 (F#m/A): the drift section's iv/A colour
# New: the form (stab-and-delay intro, groove on C#m only, the i-iv sway,
# beatless breakdown with filter sweep, peak, drift, second peak, outro, a
# final C#m9 hit whose delay rings out and loops into the intro), the chords'
# voicings, all drums (round G#1 kick, soft rounded "pdlap" snap with 4 round
# robins, dark off-beat hats, shaker), the sub bass, the pads, the dub delay
# and reverb chain and the filter automation.
#
#   python3 tools/music/edm/new11_dubtechno.py OUT.wav [bar0 bar1]
#   NEW11_MUTE=kick,snap,hats  (mute buses, for the chroma check)
import os
import sys

import numpy as np
import pyloudnorm as pyln
import soundfile as sf_
from pedalboard import (Pedalboard, Chorus, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new11 as D  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 118.0
SPB = 60.0 / BPM
NBARS = 96
TAIL = 4.0
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 4 <= bar < RB1


# ------------------------------------------------------------- harmony

SWAY = ['i', 'i', 'iv', 'iv']
DRIFT = ['i', 'i', 'iv/A', 'iv/A']
CHORDS = (['i'] * 8 +          # 0-7    intro
          ['i'] * 16 +         # 8-23   groove A (C#m only)
          SWAY * 4 +           # 24-39  groove B (the sway)
          ['i'] * 4 + ['iv'] * 4 +   # 40-47  breakdown
          SWAY * 4 +           # 48-63  peak 1
          DRIFT * 2 +          # 64-71  drift
          SWAY * 4 +           # 72-87  peak 2
          ['i'] * 8)           # 88-95  outro, final hit at 94
assert len(CHORDS) == NBARS

STAB = {'i': ['C#3', 'E4', 'G#4', 'B4', 'D#5'],   # C#m9 (low root + the rootless upper shape)
        'iv': ['F#3', 'F#4', 'A4', 'C#5', 'E5'],  # F#m7: the same shape a step up
        'iv/A': ['A3', 'F#4', 'A4', 'C#5', 'E5']}
PICK = {'i': ['E4', 'G#4', 'B4'], 'iv': ['F#4', 'A4', 'C#5'], 'iv/A': ['E4', 'A4', 'C#5']}
PAD = {'i': ['C#3', 'G#3', 'B3', 'E4'], 'iv': ['F#3', 'C#4', 'E4', 'A4'], 'iv/A': ['A2', 'E3', 'C#4', 'G#4']}
ROOT = {'i': 'C#2', 'iv': 'F#1', 'iv/A': 'A1'}

# song 5's string stab rhythm (bars 17-31), shifted 2 beats: 4-bar cell (bar, beat, accent, pickup?)
STAB_CELL = [(0, 0, 1.0, 0), (0, .5, .8, 0), (0, 1.5, .9, 0), (1, 3, .6, 1),
             (2, 0, 1.0, 0), (2, .5, .8, 0), (2, 1.5, .9, 0)]
# song 5's string counter-figure (bars 59-73), shifted: 4-bar cell (bar, beat, note, beats)
MOTIF = [(0, 2, 'C#5', .5), (0, 2.5, 'C#5', 1.5),
         (2, 1.5, 'G#4', .5), (2, 2, 'C#5', .5), (2, 2.5, 'C#5', 1.5)]

# --------------------------------------------------------------- buses

names = ['kick', 'snap', 'hats', 'sub', 'stab', 'keys', 'pad']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def add(name, sig, t, gain=1.0):
    bus[name].add(sig, t - T0, gain)


rg = np.random.default_rng(11)


def hv(v, spread=0.06):
    return v * (1 + spread * (rg.random() * 2 - 1))


def jit(t, ms=3.0):
    return t + (rg.random() * 2 - 1) * ms / 1000


# ------------------------------------------------------------- drums

def kick_at(bar, beat, vel=1.0):
    if not live(bar):
        return
    t = T(bar, beat)
    add('kick', cached('kick', D.dub_kick), t, vel)
    kicks.append(t - T0)


_snap_i = [0]


def snap_at(bar, beat, vel=1.0, dark=0.35):
    if not live(bar):
        return
    v = _snap_i[0] % 4
    _snap_i[0] += 1
    sig = cached(('snap', v, dark), lambda: D.snap(1.0, v, f0=207.65, dark=dark))
    add('snap', sig, jit(T(bar, beat), 2), hv(vel, 0.08))


def hats_bar(bar, off=0.5, ghost=0.0, shk=0.0, opn=0.0):
    if not live(bar):
        return
    for beat in range(4):
        if off:
            add('hats', cached(('ho', beat % 2), lambda: D.soft_hat(1.0, True, 0.15 if beat % 2 else -0.1)),
                jit(T(bar, beat + .5)), hv(off * (1.0 if beat % 2 else 0.9)))
        if ghost:
            for s16 in (.25, .75):
                add('hats', cached(('hg', s16), lambda: D.soft_hat(1.0, False, -0.3 if s16 < .5 else 0.3)),
                    jit(T(bar, beat + s16 + 0.03)), hv(ghost, 0.25))
        if shk:
            for s16 in (0, .25, .5, .75):
                acc = (.45, .7, .55, .9)[int(s16 * 4)]
                add('hats', cached(('sh', s16), lambda: D.shaker(1.0, 0.35)),
                    jit(T(bar, beat + s16 + (0.025 if s16 in (.25, .75) else 0))), hv(shk * acc, 0.2))
    if opn:
        add('hats', cached('hopen', lambda: D.soft_hat(1.0, True, 0.0)), T(bar, 3.5), opn)


def groove(bar0, bar1, kick=1.0, snap=0.0, off=0.5, ghost=0.0, shk=0.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            if kick:
                kick_at(b, beat, kick * (1.0 if beat % 2 == 0 else 0.94))
        if snap:
            snap_at(b, 1, snap)
            snap_at(b, 3, snap * 0.95)
            if b % 4 == 3:
                snap_at(b, 3.75, snap * 0.25, dark=0.9)
        hats_bar(b, off, ghost, shk)


# ------------------------------------------------------------- tonal parts

def stab_hit(bar, beat, chord, vel, pick=False):
    if not live(bar):
        return
    notes = PICK[chord] if pick else STAB[chord]
    key = ('stab', chord, pick)
    sig = cached(key, lambda: D.dub_stab([m(n) for n in notes], dur=0.15 if not pick else 0.12))
    add('stab', sig, jit(T(bar, beat), 1.5), hv(vel, 0.05))


def stabs(bar0, bar1, vel=1.0, mode='cell'):
    """mode 'cell': song 5's rhythm; 'one': one stab per bar on the downbeat's offbeat;
    'two': one stab per two bars (breakdown / intro)."""
    for b in range(bar0, bar1):
        if mode == 'cell':
            for db, beat, acc, pk in STAB_CELL:
                if (b - bar0) % 4 == db:
                    stab_hit(b, beat, CHORDS[b], vel * acc, pick=bool(pk))
        elif mode == 'one':
            stab_hit(b, 0.5, CHORDS[b], vel)
        elif mode == 'two':
            if (b - bar0) % 2 == 0:
                stab_hit(b, 0, CHORDS[b], vel)


def motif(bar0, bar1, vel=1.0, octave=0.0):
    for b in range(bar0, bar1, 4):
        for db, beat, note, d in MOTIF:
            bar = b + db
            if bar >= bar1 or not live(bar):
                continue
            md = m(note)
            add('keys', cached(('k', md, d), lambda: D.dub_keys(md, B(d), 1.0, pan=-0.15)),
                jit(T(bar, beat), 4), hv(vel, 0.07))
            if octave:
                add('keys', cached(('k8', md, d), lambda: D.dub_keys(md + 12, B(d), 1.0, decay=0.6, pan=0.3)),
                    jit(T(bar, beat), 4), hv(vel * octave, 0.07))


def pads(bar0, bar1, vel=1.0, attack=1.0, cut=900.0):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if live(b) or live(e - 1):
            for note in PAD[ch]:
                add('pad', D.analog_pad(m(note), T(e) - T(b), 1.0, T(b), attack=attack, release=1.6, cut=cut,
                                        voices=3, detune=0.08), T(b), vel * 0.5)
        b = e


# bass: off-beat sub pulses on the root; the phrase-end fills are song 5's timpani answers
BASS_BAR = [(0.5, 1.1, 1.0), (2.5, 0.9, 0.9), (3.5, 0.35, 0.7)]


def bass(bar0, bar1, vel=1.0, fills=True):
    for b in range(bar0, bar1):
        if not live(b):
            continue
        ch = CHORDS[b]
        r = m(ROOT[ch])
        nxt = CHORDS[b + 1] if b + 1 < NBARS else 'i'
        events = BASS_BAR
        if fills and (b - bar0) % 4 == 3:
            if nxt == 'i':     # bars 7 / 15: Ab2 E2 -> home
                events = [(0.5, 1.1, 1.0), (2, 0.9, 0.9, 'G#1'), (3, 0.9, 0.85, 'E1')]
            else:              # bar 39: Ab2 . F#2 into the F#m bars
                events = [(0.5, 1.1, 1.0), (2, 1.2, 0.9, 'G#1'), (3.5, 0.45, 0.85, 'F#1')]
        for ev in events:
            beat, d, v = ev[:3]
            md = m(ev[3]) if len(ev) > 3 else r
            sig = cached(('b', md, d), lambda: D.warm_bass(md, B(d), 1.0, cut=230, env=380, decay=0.08,
                                                          saw_amt=0.5, a=0.008, r=0.07))
            add('sub', sig, T(b, beat), vel * v)


# ---------------------------------------------------------- arrangement

# INTRO 0-7: one stab every two bars into the delay, pad, sub drone; kick from bar 4
stabs(0, 8, 0.9, mode='two')
stabs(6, 8, 0.6, mode='one')
pads(0, 8, vel=0.8, attack=2.5, cut=700)
for b in range(4, 8):
    for beat in range(4):
        kick_at(b, beat, 0.55 + 0.08 * (b - 4))
for b in (0, 2, 4, 6):
    if live(b):
        add('sub', D.warm_bass(m('C#2'), B(7.5), 1.0, cut=120, env=40, saw_amt=0.1, a=0.6, r=0.5), T(b), 0.55)

# GROOVE A 8-23: C#m only, the stab rhythm; the motif from 16
groove(8, 24, kick=1.0, snap=0.0, off=0.45)
for b in range(16, 24):
    if b % 2 == 1:
        snap_at(b, 3, 0.45, dark=0.6)
stabs(8, 24, 1.0)
bass(8, 24, 0.95)
pads(8, 24, vel=0.55, attack=1.5, cut=800)
motif(16, 24, 0.55)

# GROOVE B 24-39: the i-iv sway (song 2's rising string pair)
groove(24, 40, kick=1.0, snap=0.55, off=0.5, ghost=0.1, shk=0.12)
stabs(24, 40, 1.0)
bass(24, 40, 1.0)
pads(24, 40, vel=0.6, attack=1.0, cut=950)
motif(24, 40, 0.65)

# BREAKDOWN 40-47: beatless, stab echoes, the motif alone in the delay
stabs(40, 48, 0.85, mode='two')
pads(40, 48, vel=0.85, attack=1.8, cut=1000)
motif(40, 48, 0.7)
for b in (40, 44):
    if live(b):
        ch = CHORDS[b]
        add('sub', D.warm_bass(m(ROOT[ch]), B(15.5), 1.0, cut=120, env=40, saw_amt=0.1, a=0.8, r=0.6),
            T(b), 0.6)
for b in (46, 47):
    for beat in (0, 1, 2, 3):
        kick_at(b, beat, 0.35 + 0.15 * (b - 46) + 0.05 * beat)

# PEAK 1 48-63
groove(48, 64, kick=1.0, snap=0.6, off=0.55, ghost=0.16, shk=0.18)
stabs(48, 64, 1.05)
stabs(48, 64, 0.35, mode='one')
bass(48, 64, 1.0)
pads(48, 64, vel=0.6, attack=0.8, cut=1200)
motif(48, 64, 0.7, octave=0.35)

# DRIFT 64-71: thinner, iv over A, the stab one per bar, no motif
groove(64, 72, kick=1.0, snap=0.0, off=0.45, ghost=0.08)
for b in range(64, 72):
    if b % 2 == 1:
        snap_at(b, 3, 0.45, dark=0.6)
stabs(64, 72, 0.95, mode='one')
stabs(64, 72, 0.7)
bass(64, 72, 0.95)
pads(64, 72, vel=0.7, attack=1.2, cut=900)

# PEAK 2 72-87
groove(72, 88, kick=1.0, snap=0.62, off=0.55, ghost=0.18, shk=0.2)
stabs(72, 88, 1.08)
stabs(72, 88, 0.35, mode='one')
bass(72, 88, 1.0)
pads(72, 88, vel=0.62, attack=0.8, cut=1300)
motif(72, 88, 0.72, octave=0.45)

# OUTRO 88-95: C#m only, thinning; the final C#m9 hit at bar 94 rings out in the delay
groove(88, 92, kick=1.0, snap=0.45, off=0.45, ghost=0.0, shk=0.0)
groove(92, 94, kick=0.95, snap=0.0, off=0.35)
stabs(88, 94, 0.95)
bass(88, 94, 0.95)
pads(88, 94, vel=0.55, attack=1.0, cut=850)
motif(88, 92, 0.55)
kick_at(94, 0, 1.0)
if live(94):
    stab_hit(94, 0, 'i', 1.15)
    add('sub', D.warm_bass(m('C#2'), B(5), 1.0, cut=150, env=150, saw_amt=0.25, a=0.01, r=0.35), T(94), 1.0)
    for note in PAD['i']:
        add('pad', D.analog_pad(m(note), B(5), 1.0, T(94), attack=0.05, release=1.0, cut=1000, voices=3),
            T(94), 0.4)
    add('keys', D.dub_keys(m('C#5'), B(3), 1.0, decay=1.2), T(94), 0.6)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


tt = np.arange(N) / SR + T0
bars_axis = tt / (4 * SPB)

# the slow evolving stab filter: a cutoff table per bar (Hz) times a 16-bar LFO
CUT = [(0, 380), (7.9, 800), (8, 650), (23.9, 1300), (24, 1100), (39.9, 1700), (40, 500), (47.9, 2200),
       (48, 1500), (63.9, 2300), (64, 800), (71.9, 1400), (72, 1600), (87.9, 2600), (88, 1300), (94, 900),
       (97, 700)]
stab_fc = np.interp(bars_axis, [c[0] for c in CUT], [c[1] for c in CUT]) * \
    2 ** (0.35 * np.sin(2 * np.pi * bars_axis / 16 - 1.0)) * 1.45

duck_bass = S.sidechain(N, kicks, depth=0.6, release=0.3)
duck_pad = S.sidechain(N, kicks, depth=0.3, release=0.4)
duck_dly = S.sidechain(N, kicks, depth=0.35, release=0.35)

GAIN = {'kick': -6.5, 'snap': -5.0, 'hats': 4.5, 'sub': -10.0, 'stab': 2.0, 'keys': 0.5, 'pad': -1.5}
DUCK = {'sub': duck_bass, 'pad': duck_pad, 'stab': duck_pad, 'keys': S.sidechain(N, kicks, 0.15, 0.3)}
VERB = {'snap': .3, 'hats': .05, 'stab': .25, 'keys': .35, 'pad': .35}
DELAY = {'stab': .9, 'keys': .7, 'snap': .12}

FX = {
    'pad': lambda x: stage(x, HighpassFilter(120), Chorus(rate_hz=0.2, depth=0.25, centre_delay_ms=9, mix=0.4),
                           LowpassFilter(3500)),
    'keys': lambda x: stage(x, HighpassFilter(200), LowpassFilter(5000)),
    'snap': lambda x: stage(x, HighpassFilter(150), LowpassFilter(6500)),
    'hats': lambda x: stage(x, HighpassFilter(5000), LowpassFilter(9500)),
    'sub': lambda x: stage(x, LowpassFilter(220)),
    'kick': lambda x: stage(x, HighpassFilter(30)),
}


def evolve(k, x):
    if k == 'stab':
        x = S.svf(x, stab_fc, 1.4)
    return np.asarray(x, np.float32)


mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
MUTE = set(filter(None, os.environ.get('NEW11_MUTE', '').split(',')))
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

# dub delay: dotted 1/8 ping-pong, low-pass in the loop following the stab filter
dly_lp = np.clip(stab_fc * 0.9, 600, 3200)
dly = D.dub_delay(delay_send, B(0.75), feedback=0.62, lp=dly_lp, hp=260, drive=1.4)
del delay_send
dly = stage(dly, HighpassFilter(200), LowpassFilter(4500))
dly *= duck_dly[None, :]
levels['delay'] = meter.integrated_loudness((dly * np.float32(1.2)).T.astype(np.float64))
verb_send += dly * np.float32(0.35)
verb = D.convolve(stage(verb_send, HighpassFilter(280), LowpassFilter(6000)),
                  D.hall_ir(t60=(5.0, 5.5, 3.5, 1.6), length=7.0, seed=11))
del verb_send
levels['verb'] = meter.integrated_loudness((verb * np.float32(0.8)).T.astype(np.float64))
mix += dly * np.float32(1.2) + verb * np.float32(0.8)
del verb, dly

# section dynamics (dB at bar lines)
SECTION_DB = [(0, -5.0), (7.9, -4.0), (8, -2.5), (23.9, -2.0), (24, -1.0), (39.9, -1.0), (40, -6.0),
              (47.9, -4.5), (48, 0.0), (63.9, 0.0), (64, -2.5), (71.9, -2.0), (72, 0.5), (87.9, 0.5),
              (88, -1.5), (94, -1.5), (97, -1.5)]
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(80, -1.5, 0.7), PeakFilter(1400, 1.5, 0.6), HighShelfFilter(7000, -1.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=120))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

if RB1 == NBARS:
    end = int((T(94) + 3.6 - T0) * SR)   # the final hit at bar 94 rings 3.6 s
    mix = mix[:, :end]
    fade = int(1.2 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade, dtype=np.float32)[None, :] ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new11.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, gated):')
ref = levels.get('kick', 0.0)
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}   ({v - ref:+5.1f} vs kick)')
