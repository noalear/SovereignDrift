#!/usr/bin/env python3
# new16_neurofunk.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Kill Vector": new track 16 (energy tag BATTLE), a late-90s techstep /
# neurofunk drum & bass piece (Ed Rush & Optical, Bad Company, Konflict),
# 172 BPM, F# minor (dorian colour), produced in code with
# tools/music/edm/synth.py and synth_new16.py.
#
# Credits: built from fragments of the original game music by Stardock
# (mid/sf-music2.mid and mid/sf-music8.mid).  Crash-cymbal samples from the
# VSCO-2 Community Edition by Versilian Studios (CC0 1.0 Universal, public
# domain dedication).  Everything else (reese, sub, stabs, pads, lead, the
# whole drum kit, FX) is synthesized.
#
# A NEW piece, not a remix.  Fragments (bar numbers are the MIDIs', 1-based,
# neither file has a pickup), chosen from what song2_synthwave.py,
# new02_proghouse.py, song8_dubstep.py, song8_darkwave.py and new08_detroit.py
# did not use:
#   F1  sf-music2, the String Ensemble 2 inner voices, ch 6 + ch 7, bars 25-56
#       (two voices moving by step every two bars): F#4/A4 <-> Ab4/B4 over the
#       F#m / Abm bass of bars 25-40, and F4/Ab4 <-> F#4/A4 over C# / F#m(A) of
#       bars 41-56.  With F# as the new tonic this is the track's harmony and
#       its dyad cell: F#m F#m G#m G#m (prog A, orig 25-28), C# C# F#m/A F#m/A
#       (prog B, orig 41-44).  The dyads are the pad voicings, the chopped
#       offbeat "neuro" dyad stabs in the drops, and their top voice
#       (A4 B4 / G#4 A4) is the eerie lead's held line in the second drop.
#   F2  sf-music8 (second half), the orchestra-hit punctuation of the chant
#       section, ch 8 + ch 13, bars 61-64 / 65-68: B (beat 3) | D (beat 3) |
#       F# F# F# (3+3+2) | F#  -- moved down a fourth to F# | A | C# C# C# | C#.
#       This is the track's sharp stab motif ("kill vector" hook).  It is
#       re-pitched over prog B (C# | E# | F# F# F# | F#) and prog C (F# | G# |
#       A A A | G#), and the breakdown lead develops it (augmented, extended to
#       D#6 / B5 and resolving to F#5).
#   F3  sf-music8 (second half), the rising closing line of ch 2 (program change
#       to contrabass at bar 84), bars 84-86: F#4 A4 (beats 4, 4.5) | B4 C#5 |
#       D5 B4 | F#5, moved down a fourth: C# E | F# G# | A F# | C#.  It is the
#       lead's answer phrase over prog B (C# C# F#m/A F#m/A: a V -> i line, the
#       E natural pickup a blue note against C#) in the breakdown and drop 2,
#       varied the second time (A B A G# | F#) so it lands on the tonic.
# New: everything else -- 172 BPM, prog C (F#m C# D C#, i V bVI V, used in the
# second half of the drops and the outro, ending on C# so the final F#m hit
# closes it), the form (intro, build, drop 1, breakdown,
# drop 2, outro, final F#m hit that rings out and loops back to the F#m
# intro), the two-step breaks (kick tuned to F#1, a round tuned A3 snare with
# round robins and ghost notes, 16th hats, shaker, ride, tom fills instead of
# snare rolls), the reese bass with a FIXED low-pass (one cutoff per section,
# no sweeps, no vowel / wobble), octave jumps, 1/16 chops and sub drops over a
# clean mono sine sub, dark pad, FX and the mix.
#
#   python3 tools/music/edm/new16_neurofunk.py OUT.wav [bar0 bar1]
#   NEW16_MUTE=kick,snare,...  mutes buses (for the chroma check)
import gc
import glob
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Chorus, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new16 as K  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 172.0
SPB = 60.0 / BPM
FINAL = 128
NBARS = FINAL + 1
RING = 3.0
TAIL = 4.0
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)
VSCO = os.environ.get('VSCO_DIR', '/tmp/claude-0/samples/VSCO-2-CE')


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - T0


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 2 <= bar < RB1


# ------------------------------------------------------------- harmony

PROG = {'A': ['F#m', 'F#m', 'G#m', 'G#m'],        # F1, orig sf-music2 bars 25-28
        'B': ['C#', 'C#', 'F#m/A', 'F#m/A'],      # F1, orig sf-music2 bars 41-44
        'C': ['F#m', 'C#', 'D', 'C#']}            # new (i V bVI V)
SUBROOT = {'F#m': 'F#1', 'G#m': 'G#1', 'C#': 'C#2', 'F#m/A': 'A1', 'D': 'D2'}
DYAD = {'F#m': ['F#4', 'A4'], 'G#m': ['G#4', 'B4'], 'C#': ['F4', 'G#4'],     # F4 = E#4
        'F#m/A': ['F#4', 'A4'], 'D': ['F#4', 'A4']}
PADV = {'F#m': ['F#3', 'C#4', 'F#4', 'A4'], 'G#m': ['G#3', 'D#4', 'G#4', 'B4'],
        'C#': ['C#3', 'C#4', 'F4', 'G#4'], 'F#m/A': ['A2', 'C#4', 'F#4', 'A4'],
        'D': ['D3', 'D4', 'F#4', 'A4']}

FORM = [  # (first bar, prog letter, number of 4-bar cycles)
    (0, 'A', 4), (16, 'A', 2), (24, 'B', 2),                 # intro, build
    (32, 'A', 4), (48, 'B', 2), (56, 'C', 2),                # drop 1
    (64, 'A', 2), (72, 'B', 2),                              # breakdown
    (80, 'A', 2), (88, 'B', 2), (96, 'C', 2), (104, 'A', 2),  # drop 2
    (112, 'A', 2), (120, 'C', 2)]                            # outro
CHORDS = []
PROGBAR = []
for _b0, _p, _c in FORM:
    for _i in range(4 * _c):
        CHORDS.append(PROG[_p][_i % 4])
        PROGBAR.append((_p, _i % 4))
CHORDS.append('F#m')
PROGBAR.append(('end', 0))
assert len(CHORDS) == NBARS

# F2: the stab motif (beat offsets within a 4-bar cycle, note) per progression
MOTIF_RHY = [(2.0, 0), (6.0, 1), (8.0, 2), (9.5, 2), (11.0, 2), (12.0, 3)]
MOTIF_NOTES = {'A': ['F#4', 'A4', 'C#5', 'C#5'],        # orig B D F# F# (sf-music8 61-64)
               'B': ['C#4', 'F4', 'F#4', 'F#4'],         # F4 = E#4
               'C': ['F#4', 'G#4', 'A4', 'G#4']}
FIFTH_OK = {'A': True, 'B': False, 'C': False}

# reese / sub patterns: (step, len_steps, interval, drop)
BASS_PAT = {
    'a': [(0, 6, 0, 0), (6, 4, 0, 0), (10, 6, 0, 0)],
    'b': [(0, 3, 0, 0), (3, 3, 12, 0), (6, 4, 0, 0), (10, 3, 0, 0), (13, 3, 12, 0)],
    'c': [(0, 6, 0, 0), (6, 2, 0, 0), (8, 2, 12, 0), (10, 6, 0, 0)],
    'd': [(0, 6, 0, 0), (6, 4, 0, 0), (10, 2, 0, 0), (12, 4, 0, 12)],              # sub drop
    'g': [(0, 4, 0, 0), (6, 2, 0, 0), (10, 6, 0, 0)],
    'x': [(0, 1, 0, 0), (1, 1, 0, 0), (2, 2, 12, 0), (4, 1, 0, 0), (6, 2, 0, 0), (8, 1, 12, 0),
          (9, 1, 0, 0), (10, 2, 0, 0), (12, 1, 0, 0), (13, 1, 12, 0), (14, 2, 0, 0)],  # 1/16 chops
    'h': [(0, 16, 0, 0)],
    'q': [(0, 8, 0, 0), (10, 6, 0, 0)],
}

# ------------------------------------------------------------- buses

names = ['kick', 'snare', 'hats', 'ride', 'perc', 'sub', 'reese', 'stab', 'dyad', 'pad', 'lead',
         'atmos', 'fx', 'crash']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}
rg = S.rng()


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def hv(v, spread=0.08):
    return v * (1 - spread / 2 + spread * rg.random())


def load_wav(path):
    x, sr = sf_.read(path, dtype='float32', always_2d=True)
    x = x.T[:2]
    if x.shape[0] == 1:
        x = np.vstack([x, x])
    if sr != SR:
        n_out = int(x.shape[1] * SR / sr)
        pos = np.arange(n_out) * (sr / SR)
        x = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x]).astype(np.float32)
    on = int(np.argmax(np.abs(x).max(axis=0) > 0.02 * np.abs(x).max()))
    x = x[:, max(0, on - 48):]
    return x / np.abs(x).max()


CRASH = [load_wav(p) for p in sorted(glob.glob(os.path.join(VSCO, 'Percussion', 'cymbal-crash1_mf_rr*.wav')))]
CRASH += [load_wav(p) for p in sorted(glob.glob(os.path.join(VSCO, 'Percussion', 'cymbal-crash1_mp_rr*.wav')))]
assert CRASH, 'VSCO crash samples not found (set VSCO_DIR)'

# ------------------------------------------------------------- drums

PAT = {
    'A': dict(k=[(0, 1.0), (10, .9)], s=[(4, 1.0), (12, 1.0)], g=[(7, .26), (9, .2), (15, .3)]),
    'B': dict(k=[(0, 1.0), (2, .6), (10, .9)], s=[(4, 1.0), (12, 1.0)], g=[(6, .22), (14, .3)]),
    'C': dict(k=[(0, 1.0), (10, .85), (13, .6)], s=[(4, 1.0), (12, 1.0)], g=[(1, .18), (7, .3), (15, .34)]),
    'D': dict(k=[(0, 1.0), (10, .85)], s=[(4, 1.0), (12, 1.0)], g=[(7, .3), (9, .28), (14, .35)]),
    'E': dict(k=[(0, 1.0), (6, .55), (10, .9)], s=[(4, 1.0), (12, 1.0)], g=[(3, .2), (11, .26), (15, .3)]),
    'H': dict(k=[(0, 1.0), (10, .7)], s=[(8, 1.0)], g=[(14, .25)]),          # half-time
    'S': dict(k=[(0, 1.0)], s=[(4, 1.0)], g=[]),                             # stop after beat 2
}
_rr = 0


def place_kick(d, step, vel=1.0):
    t = T(d, step / 4)
    bus['kick'].add(cached(('kick',), K.kick), t, hv(vel, 0.04))
    kicks.append(t)


def snare_hit(d, step, vel=1.0):
    global _rr
    _rr += 1
    v = _rr % 4
    if vel >= 0.45:
        sig = cached(('sn', v), lambda: K.snare(1.0, v))
    else:
        sig = cached(('gh', v), lambda: K.snare(1.0, v, dark=0.85))
    bus['snare'].add(sig, T(d, step / 4), hv(vel, 0.08))


def beat_bar(d, pat='A', hats='16', ride=False, shaker=False, kick=True, snare=True, ghosts=True, vel=1.0):
    if not live(d):
        return
    p = PAT[pat]
    stop = 8 if pat == 'S' else 16
    if kick:
        for s, v in p['k']:
            place_kick(d, s, v * vel)
    if snare:
        for s, v in p['s']:
            snare_hit(d, s, v * vel)
    if ghosts:
        for s, v in p['g']:
            snare_hit(d, s, v * 0.8 * vel)
    if hats:
        for s in range(stop):
            if hats == '8' and s % 2:
                continue
            if hats == 'off' and s % 4 != 2:
                continue
            acc = [.32, .2, .55, .24][s % 4]
            if s in (6,) and hats == '16' and pat in ('A', 'E'):
                bus['hats'].add(cached(('oh',), lambda: K.hat(1.0, 0.15, open_=True)), T(d, s / 4), hv(.3))
                continue
            bus['hats'].add(K.hat(1.0, -0.3 + 0.6 * rg.random()), T(d, s / 4), hv(acc * vel, 0.25))
    if shaker:
        for s in range(stop):
            bus['perc'].add(K.shaker(1.0, 0.35), T(d, s / 4), hv(0.14 if s % 2 else 0.22, 0.3) * vel)
    if ride:
        for s, v in ((0, .5), (4, .38), (8, .48), (12, .38)):
            if s < stop:
                bus['ride'].add(cached(('ride',), K.ride), T(d, s / 4), hv(v * vel, 0.1))


def drums(d0, d1, plan, **kw):
    for d in range(d0, d1):
        beat_bar(d, plan[(d - d0) % len(plan)], **kw)


def tom_fill(d, beat0=2.0, vel=0.7):
    """Tuned tom fill on the last two beats (instead of a snare roll): C#3 A2 F#2."""
    if not live(d):
        return
    for i, (b, nt) in enumerate(((0, 'C#3'), (0.5, 'C#3'), (0.75, 'A2'), (1.25, 'F#2'), (1.5, 'F#2'))):
        bus['perc'].add(cached(('tom', nt), lambda: S.tom(m(nt), 1.0)), T(d, beat0 + b),
                        hv(vel * (0.85 + 0.05 * i), 0.06))


def crash_at(d, vel=0.5, k=0):
    if live(d):
        bus['crash'].add(CRASH[k % len(CRASH)], T(d), vel)


# ------------------------------------------------------------- bass

def bass(d0, d1, plan, sub=1.0, reese=1.0, cut=750.0, octave=0.0, drive=2.2, last_drop=True):
    """reese + sub over bars d0..d1-1 with the pattern string cycled per bar."""
    a, b = max(d0, RB0), min(d1, RB1)
    if a >= b:
        return
    notes = []
    for d in range(a, b):
        root = m(SUBROOT[CHORDS[d]])
        if d == FINAL:
            notes.append((T(d) + T0, root, 0, RING * 0.9, 0))
            continue
        pat = BASS_PAT[plan[(d - d0) % len(plan)]]
        for st, ln, iv, drop in pat:
            notes.append((T(d, st / 4) + T0, root, iv, B(ln / 4), drop))
    notes.sort()
    t_start = T(a) + T0
    n = int((T(b) - T(a) + (RING if b > FINAL else 0.25)) * SR)
    if sub:        # the sub ignores the octave jumps
        p, e = K.traj([(ts - t_start, r, du, dr) for ts, r, iv, du, dr in notes], n)
        bus['sub'].add(K.sub(p, e), T(a), sub)
    if reese:
        p, e = K.traj([(ts - t_start, r + iv + 12, du, dr) for ts, r, iv, du, dr in notes], n)
        bus['reese'].add(K.reese(p, e, cut=cut, octave=octave, drive=drive), T(a), reese)


# ------------------------------------------------------------- stabs / pads / lead

def stab_notes(note, prog):
    md = m(note)
    out = [md - 12, md]
    if FIFTH_OK.get(prog, False):
        out.append(md + 7)
    return out


def motif(d0, vel=0.8, cut=3000.0, octave=0, decay=0.13):
    """F2: the stab motif over the 4-bar cycle starting at bar d0."""
    prog = PROGBAR[d0][0]
    notes = MOTIF_NOTES[prog]
    for bt, k in MOTIF_RHY:
        d = d0 + int(bt // 4)
        if not live(d):
            continue
        md = stab_notes(notes[k], prog)
        md = [x + 12 * octave for x in md]
        dur = 0.32 if k == 3 else 0.2
        key = ('stab', tuple(md), dur, cut, decay)
        v = vel * (1.08 if bt == 8.0 else 1.0)
        bus['stab'].add(cached(key, lambda: K.stab(md, dur, 1.0, cut=cut, decay=decay)), T(d, bt % 4), hv(v, 0.05))


def dyad_chops(d0, d1, steps=(6, 14), vel=0.45, cut=1700.0):
    """F1 dyads as short offbeat stabs."""
    for d in range(d0, d1):
        if not live(d):
            continue
        dy = tuple(m(x) for x in DYAD[CHORDS[d]])
        for s in steps:
            key = ('dy', dy, cut)
            bus['dyad'].add(cached(key, lambda: K.stab(list(dy), 0.09, 1.0, cut=cut, decay=0.05, width=0.8)),
                            T(d, s / 4), hv(vel * (1.0 if s % 8 == 6 else 0.8), 0.1))


def pad_bars(d0, d1, vel=1.0, attack=1.0, cut=1100.0):
    d = max(d0, RB0 - 2)
    d1 = min(d1, RB1, FINAL)
    while d < d1:
        ch = CHORDS[d]
        e = d
        while e < d1 and CHORDS[e] == ch:
            e += 1
        dur = T(e) - T(d)
        for note in PADV[ch]:
            bus['pad'].add(K.pad(m(note), dur, 1.0, a=attack, r=0.9, cut=cut), T(d), vel * 0.3)
        d = e


def lead(notes, d0, vel=0.8, transpose=0, cut=2600.0):
    for beat, note, dur in notes:
        bb = d0 * 4 + beat
        d, bt = int(bb // 4), bb % 4
        if live(d):
            bus['lead'].add(K.ghostlead(m(note) + transpose, B(dur), vel, cut=cut), T(d, bt), 1.0)


# breakdown lead: F2 augmented and developed (prog A), then F3 (prog B)
LEAD_BD = [(2, 'F#5', 3), (5, 'A5', 3), (8, 'B5', 4), (12, 'C#6', 1.5), (13.5, 'C#6', 1.5), (15, 'B5', 3),
           (18, 'F#5', 4), (22, 'A5', 2), (24, 'C#6', 1.5), (25.5, 'D#6', 1.5), (27, 'C#6', 1), (28, 'B5', 3.5)]
# F3 (beats from the first bar of prog B)
F3_LINE = [(0, 'F5', 3), (3, 'C#5', .5), (3.5, 'E5', .5), (4, 'F#5', 2), (6, 'G#5', 2), (8, 'A5', 2),
           (10, 'F#5', 2), (12, 'C#6', 4)]
F3_VAR = [(0, 'G#5', 3), (3, 'C#5', .5), (3.5, 'E5', .5), (4, 'F#5', 2), (6, 'G#5', 2), (8, 'A5', 1.5),
          (9.5, 'B5', .5), (10, 'A5', 1), (11, 'G#5', 1), (12, 'F#5', 2.5)]
# F1 top voice, one per bar (drop 2, prog C): A G# A G#
LEAD_F1 = [(0, 'A5', 3.8), (4, 'G#5', 3.8), (8, 'A5', 3.8), (12, 'G#5', 3.8)]

# ---------------------------------------------------------- arrangement

print('arranging', flush=True)
bus['atmos'].add(K.drone(N, T0), 0.0, 1.0)

# INTRO 0-15: dark pad (F1), sub drone, the motif teased far away; break fades in from 8
pad_bars(0, 16, vel=0.95, attack=2.0, cut=900)
bass(0, 16, 'h', sub=0.7, reese=0.0)
for d in (4, 12):
    motif(d, vel=0.45, cut=1300, decay=0.1)
drums(8, 15, 'AAAB', hats='16', kick=True)
beat_bar(15, 'S', hats='16')
bus['fx'].add(S.reverse_cymbal(B(6)), T(15, -2), 0.25)

# BUILD 16-31: full break, sub + a dark (low fixed cutoff) reese from 24, motif closer
crash_at(16, 0.45)
bus['fx'].add(S.impact(), T(16), 0.35)
drums(16, 31, 'AABCAABD', hats='16', shaker=True)
pad_bars(16, 32, vel=0.6, attack=0.8, cut=1000)
bass(16, 24, 'q', sub=0.9, reese=0.0)
bass(24, 31, 'aaag', sub=1.0, reese=0.75, cut=420)
motif(16, vel=0.6, cut=1700)
motif(24, vel=0.65, cut=1900)
dyad_chops(20, 31, steps=(6,), vel=0.3, cut=1300)
bus['fx'].add(S.noise_riser(B(14), 300, 6000, 1.5), T(28), 0.10)
if live(31):
    beat_bar(31, 'S', hats='8')
    bus['fx'].add(S.reverse_cymbal(B(4)), T(31, 0), 0.3)
    place_kick(31, 0, 0.9)

# DROP 1 32-63 ---------------------------------------------------------------
crash_at(32, 0.6)
bus['fx'].add(S.impact(), T(32), 0.5)
drums(32, 47, 'AABCAEBD', hats='16')
tom_fill(47, 2.0, 0.65)
beat_bar(47, 'D', hats='16')
bass(32, 48, 'abacabad', cut=760)
for d in (32, 40):
    motif(d, vel=0.85)
for d in (36, 44):
    motif(d, vel=0.55, octave=-1, cut=1600)
dyad_chops(32, 48, vel=0.4)
pad_bars(32, 48, vel=0.32, attack=0.4, cut=1000)
crash_at(40, 0.35, 1)

crash_at(48, 0.45, 2)
drums(48, 55, 'AABCAABC', hats='16', shaker=True)
beat_bar(55, 'D', hats='16', shaker=True)
bass(48, 56, 'cacd', cut=760)
motif(48, vel=0.85)
motif(52, vel=0.7)
dyad_chops(48, 56, vel=0.42)
pad_bars(48, 56, vel=0.32, attack=0.4, cut=1000)

crash_at(56, 0.45, 3)
drums(56, 63, 'AEBCAEBC', hats='16', shaker=True)
tom_fill(63, 2.0, 0.7)
beat_bar(63, 'S', hats='16', shaker=True)
bass(56, 63, 'acag', cut=760)
bass(63, 64, 'g', cut=760)
motif(56, vel=0.85)
motif(60, vel=0.75)
dyad_chops(56, 63, vel=0.42)
pad_bars(56, 64, vel=0.35, attack=0.4, cut=1000)
bus['fx'].add(S.downlifter(B(8)), T(63, 2), 0.18)

# BREAKDOWN 64-79: pad + lead (F2 developed), sub long; half-time pulse from 72
crash_at(64, 0.35, 1)
bus['fx'].add(S.impact(), T(64), 0.3)
pad_bars(64, 80, vel=1.0, attack=1.2, cut=1200)
bass(64, 72, 'h', sub=0.75, reese=0.0)
bass(72, 79, 'q', sub=0.85, reese=0.0)
lead(LEAD_BD, 64, vel=0.85)
lead(F3_LINE, 72, vel=0.85)
lead(F3_VAR, 76, vel=0.85)
drums(64, 72, 'AAAAAAAA', hats='off', kick=False, snare=False, ghosts=False, ride=True, vel=0.6)
drums(72, 76, 'HHHH', hats='8', ghosts=True, vel=0.75)
drums(76, 79, 'AAB', hats='16', vel=0.85)
dyad_chops(72, 79, steps=(6,), vel=0.3, cut=1300)
motif(76, vel=0.55, cut=1600)
bus['fx'].add(S.noise_riser(B(11), 300, 6500, 1.5), T(76), 0.12)
bus['fx'].add(S.reverse_cymbal(B(4)), T(79), 0.3)
if live(79):
    place_kick(79, 0, 0.9)
    snare_hit(79, 4, 0.8)

# DROP 2 80-111: heavier (octave layer on the reese, 1/16 chops, ride) --------
crash_at(80, 0.65)
bus['fx'].add(S.impact(), T(80), 0.55)
drums(80, 87, 'AABCAEBC', hats='16', ride=True)
tom_fill(87, 2.0, 0.65)
beat_bar(87, 'D', hats='16', ride=True)
bass(80, 88, 'abcdabxd', cut=950, octave=0.35, drive=2.6)
motif(80, vel=0.9)
motif(84, vel=0.65, octave=-1, cut=1700)
dyad_chops(80, 88, steps=(3, 6, 14), vel=0.42, cut=1900)
pad_bars(80, 88, vel=0.3, attack=0.4, cut=1000)

crash_at(88, 0.5, 2)
drums(88, 95, 'AEBCAEBC', hats='16', ride=True, shaker=True)
beat_bar(95, 'D', hats='16', ride=True, shaker=True)
bass(88, 96, 'bcbd', cut=950, octave=0.35, drive=2.6)
motif(88, vel=0.9)
motif(92, vel=0.8)
lead(F3_LINE, 88, vel=0.6)
lead(F3_VAR, 92, vel=0.65)
dyad_chops(88, 96, steps=(3, 6, 14), vel=0.42, cut=1900)
pad_bars(88, 96, vel=0.3, attack=0.4, cut=1000)

crash_at(96, 0.5, 3)
drums(96, 103, 'AABCAEBC', hats='16', ride=True, shaker=True)
tom_fill(103, 2.0, 0.7)
beat_bar(103, 'D', hats='16', ride=True, shaker=True)
bass(96, 104, 'acxd', cut=950, octave=0.35, drive=2.6)
motif(96, vel=0.9)
motif(100, vel=0.85)
lead(LEAD_F1, 96, vel=0.6)
lead(LEAD_F1, 100, vel=0.65)
dyad_chops(96, 104, steps=(6, 14), vel=0.4, cut=1900)
pad_bars(96, 104, vel=0.3, attack=0.4, cut=1000)

crash_at(104, 0.55)
drums(104, 111, 'AEBCAEBC', hats='16', ride=True, shaker=True)
beat_bar(111, 'S', hats='16', ride=True)
bass(104, 111, 'xbxbxbx', cut=950, octave=0.35, drive=2.6)
bass(111, 112, 'g', cut=950, octave=0.35, drive=2.6)
motif(104, vel=0.95)
motif(108, vel=0.9)
dyad_chops(104, 111, steps=(3, 6, 14), vel=0.44, cut=1900)
pad_bars(104, 112, vel=0.3, attack=0.4, cut=1000)

# OUTRO 112-127: the drop thins out; prog C ends on C# -> final F#m hit
crash_at(112, 0.5, 1)
drums(112, 119, 'AABCAABC', hats='16')
beat_bar(119, 'D', hats='16')
bass(112, 120, 'aaad', cut=760)
motif(112, vel=0.8)
motif(116, vel=0.6, octave=-1, cut=1600)
dyad_chops(112, 120, vel=0.38)
pad_bars(112, 120, vel=0.4, attack=0.5, cut=1000)

drums(120, 127, 'AAAB', hats='8', ghosts=False, vel=0.85)
beat_bar(127, 'A', hats='8', ghosts=False, vel=0.85)
bass(120, 128, 'qqqq', sub=0.95, reese=0.0)
motif(120, vel=0.7, cut=2000)
motif(124, vel=0.75, cut=2000)
pad_bars(120, 128, vel=0.7, attack=0.8, cut=1100)
lead([(0, 'A5', 3.8), (4, 'G#5', 3.8), (8, 'A5', 3.8), (12, 'G#5', 3.8)], 124, vel=0.45)

# FINAL HIT (bar 128): F#m -- kick, sub, stab, pad, crash; rings out
if live(FINAL):
    place_kick(FINAL, 0, 1.0)
    snare_hit(FINAL, 0, 0.6)
    crash_at(FINAL, 0.55)
    bus['fx'].add(S.impact(), T(FINAL), 0.45)
    bass(FINAL, NBARS, 'h', sub=1.0, reese=0.8, cut=760)
    bus['stab'].add(K.stab([m('F#3'), m('F#4'), m('C#5')], 0.5, 1.0, cut=2200, decay=0.35), T(FINAL), 0.9)
    for note in PADV['F#m']:
        bus['pad'].add(K.pad(m(note), 1.2, 1.0, a=0.01, r=0.9, cut=1200), T(FINAL), 0.32)
    bus['lead'].add(K.ghostlead(m('A5'), 1.6, 0.5, r=0.6), T(FINAL), 1.0)

# ------------------------------------------------------------- mix

print('mixing', flush=True)
del CRASH
_cache.clear()
gc.collect()


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


def sweep(k, d0, d1, f0, f1, shape=1.6, q=0.7):
    """time-varying low-pass on bus k between bars d0..d1 (the intro's drum fade-in only)."""
    i0, i1 = int(max(0, T(d0)) * SR), int(max(0, T(d1)) * SR)
    i1 = min(i1, N)
    if i1 - i0 < 100:
        return
    u = np.linspace(0, 1, i1 - i0)
    fc = f0 * (f1 / f0) ** (u ** shape)
    bus[k].x[:, i0:i1] = S.svf(bus[k].x[:, i0:i1], fc, q).astype(np.float32)


for k in ('snare', 'hats', 'kick'):
    sweep(k, 8, 16, 400, 16000, 2.0)

duck_bass = S.sidechain(N, kicks, depth=0.55, release=0.13, power=1.5)
duck_sub = S.sidechain(N, kicks, depth=0.75, release=0.16, power=1.5)
duck_mid = S.sidechain(N, kicks, depth=0.25, release=0.15)

GAIN = {  # dB
    'kick': 0.0, 'snare': 3.5, 'hats': 9.0, 'ride': 4.0, 'perc': -10.0,
    'sub': -9.5, 'reese': -1.0, 'stab': 6.0, 'dyad': 0.0, 'pad': -4.0, 'lead': 2.0,
    'atmos': -20.0, 'fx': -6.0, 'crash': -6.0,
}
DUCK = {'sub': duck_sub, 'reese': duck_bass, 'pad': duck_mid, 'dyad': duck_mid, 'stab': duck_mid}
ROOM = {'snare': .18, 'hats': .06, 'perc': .12, 'stab': .1}
HALL = {'snare': .04, 'stab': .22, 'dyad': .25, 'pad': .3, 'lead': .38, 'fx': .25, 'ride': .06, 'atmos': .3}
DELAY = {'stab': .18, 'lead': .2, 'dyad': .12}

bus['stab'].x = stage(bus['stab'].x, HighpassFilter(160), PeakFilter(300, -2.5, 0.8), LowpassFilter(6500),
                      Compressor(threshold_db=-18, ratio=2.5, attack_ms=3, release_ms=90))
bus['dyad'].x = stage(bus['dyad'].x, HighpassFilter(250), LowpassFilter(5000))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(180), PeakFilter(400, -2.0, 1.0),
                     Chorus(rate_hz=0.2, depth=0.35, centre_delay_ms=9, mix=0.45))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(300), LowpassFilter(7000),
                      Compressor(threshold_db=-22, ratio=2.5, attack_ms=10, release_ms=150))
bus['reese'].x = stage(bus['reese'].x, HighpassFilter(80), PeakFilter(250, 1.5, 0.9), PeakFilter(2800, 3.0, 0.8), LowpassFilter(5500),
                       Compressor(threshold_db=-16, ratio=3, attack_ms=8, release_ms=80))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(140), LowpassFilter(8000),
                       Compressor(threshold_db=-16, ratio=3, attack_ms=3, release_ms=60))
bus['kick'].x = stage(bus['kick'].x, HighpassFilter(30))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(5500), LowpassFilter(11500))
bus['ride'].x = stage(bus['ride'].x, HighpassFilter(2500), LowpassFilter(10000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(70), LowpassFilter(11000))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(40), LowpassFilter(9000))
bus['crash'].x = stage(bus['crash'].x, HighpassFilter(400), LowpassFilter(9000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['atmos'].x = stage(bus['atmos'].x, HighpassFilter(150), LowpassFilter(3000))
_bars = (np.arange(N) / SR + T0) / (4 * SPB)
_at = np.interp(_bars, [0, 16, 16.01, 64, 64.01, 80, 80.01, 120, 128, 132],
                [1.0, 0.8, 0.25, 0.25, 0.9, 0.7, 0.25, 0.25, 0.6, 0.0])
bus['atmos'].x *= _at.astype(np.float32)[None, :]
del _at

mix = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('NEW16_MUTE', '').split(',')))
STEMS = os.environ.get('NEW16_STEMS')
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        continue
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= DUCK[k][None, :]
    if np.any(x):
        levels[k] = pyln.Meter(SR).integrated_loudness(x.T.astype(np.float64))
    if STEMS:
        np.save(os.path.join(STEMS, k + '.npy'), x)
    mix += x
    if k in ROOM:
        room_send += x * ROOM[k]
    if k in HALL:
        hall_send += x * HALL[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

room = stage(room_send, HighpassFilter(250), LowpassFilter(6500),
             Reverb(room_size=0.22, damping=0.6, wet_level=1.0, dry_level=0.0, width=0.8))
del room_send
pre = int(0.02 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
del hall_send
hall = stage(hs, HighpassFilter(350), LowpassFilter(7000),
             Reverb(room_size=0.88, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del hs
hall *= duck_mid[None, :]
dly = S.pingpong(delay_send, B(0.75), feedback=0.42, repeats=7, lp=4000, hp=400)
del delay_send
mix += room * 0.8 + hall * 0.75 + dly * 0.5
del room, hall, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -6.5), (8, -6.0), (15.9, -5.0), (16, -4.0), (31.9, -3.0), (32, 0.0), (63.9, 0.0),
              (64, -7.5), (71.9, -7.0), (72, -6.0), (79.9, -4.0), (80, 1.0), (111.9, 1.0),
              (112, -1.0), (119.9, -1.5), (120, -3.0), (127.9, -3.0), (128, -1.0), (140, -1.0)]
sec = np.interp(_bars, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(60, -1.0, 0.7), PeakFilter(350, -1.5, 0.8), PeakFilter(3200, 2.0, 0.7), HighShelfFilter(9000, -1.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=60))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
if RB1 == NBARS:
    last = min(last, int((T(FINAL) + RING) * SR))
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new16_neurofunk.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
