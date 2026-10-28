#!/usr/bin/env python3
# new03_jungle.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Lagrange Drift": new track 03, an atmospheric jungle / intelligent drum &
# bass piece (1995-98 Good Looking / Logical Progression style) at 168 BPM in
# D minor (dorian), produced in code with tools/music/edm/synth.py,
# synth_liquid.py and synth_new03.py.
#
# Credits: built from fragments of the original game music by Stardock
# (mid/sf-music3.mid, Stardock Shared Source license).  Flute, clarinet,
# triangle and cymbal samples from the VSCO-2 Community Edition by Versilian
# Studios (CC0 1.0 Universal, public domain dedication).  Everything else
# (Rhodes, vibraphone, pads, sub, the breakbeat kit, wind) is synthesized.
#
# A NEW piece, not a remix of song 3: its themes are the parts of sf-music3
# that the song-3 remix (song3_liquid.py) only used in the background:
#   F1  the inner string voice, ch 8 (String Ensemble 1), orig bars 1-8:
#       C5 D5 F5 D5 | C5 D5 F5 E5 (one note per bar).  Here re-set as a
#       syncopated two-bar vibraphone hook and sequenced down the scale over
#       each chord (C D F D / B C E C / A B D B / G A C A, tails D E C D ...);
#       in the climax the flute plays the original line augmented (2 bars a note).
#   F2  the French horn counter-line, ch 9, orig bars 26-31:
#       E3 G3 | A3 E3 | G3 F3 | E3 .. C3 | D3, two octaves up as the main
#       flute / "sax" (clarinet) lead, re-harmonized Fmaj9 G13 Cmaj9 Am9 Dm9
#       (it now resolves to the new tonic D), with a varied second statement.
#   F3  the whistle counter-line, ch 2, orig bars 67-76:
#       A3 B3 | C4 D4 | C4 | C4 D4 | E4 | D4 C4 | B3 A3 | B3 | E4, compressed
#       to 8 bars as the answering theme (breakdown and section C).
#   F4  the C-section bass line, ch 5 (fretless), orig bars 49-64:
#       D .. C .. B .. F | A E A C with its walk figures D2 A1 D2 F2 (bar 52)
#       and C2 D2 E2 C2 (bar 56): the main progression (Dm9 Cmaj9 Bm7b5 Am9,
#       the original Dm Am/C Bm7b5 Am heard with D as the tonic) and the rolling
#       sub; the bass figure E1 B1 D2 E2 E1 (bar 4) moved to A for the dominant.
#   F5  the original kit's open triangle (bars 18, 20, 34, 36) and side-stick
#       pickup figure (beat 1.5 -> 2): triangle accents, breakdown rim clicks.
# New: 168 BPM breakbeats (round, short tuned snare with round robins,
# ghost notes, ride), rolling sub, Rhodes, vibes, pads, a long airy intro,
# floating breakdown, and the ending A7sus -> A7b9 -> a final Dm9 hit with a
# short ring-out (the track starts on Dm9, so it loops cleanly).
#
#   python3 tools/music/edm/new03_jungle.py OUT.wav [bar0 bar1]
#   NEW03_MUTE=kick,snare,... mutes buses (for the chroma check)
import os
import sys
import gc
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Chorus, Compressor, Limiter, Distortion,
                        HighpassFilter, LowpassFilter, LowShelfFilter,
                        HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_liquid as L  # noqa: E402
import synth_new03 as J  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 168.0
SPB = 60.0 / BPM
NBARS = 113           # bars 0-111 + the final hit (bar 112)
FINAL = 112
RING = 3.0
TAIL = 4.5
RB0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
RB1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
T0 = RB0 * 4 * SPB
N = int(((RB1 - RB0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - T0


def B(beats):
    return beats * SPB


def live(bar):
    return RB0 - 3 <= bar < RB1


# ------------------------------------------------------------- harmony

PROG_A = ['Dm9', 'Dm9', 'Cmaj9', 'Cmaj9', 'Bm7b5', 'Bm7b5', 'Am9', 'Am9']          # F4
PROG_B = ['Fmaj9', 'G13', 'Cmaj9', 'Am9', 'Dm9', 'Bbmaj9', 'Dm9', 'A7sus']        # under F2
PROG_C = ['Dm9', 'Bbmaj9', 'Fmaj9', 'G9sus', 'Am9', 'Dm9', 'G13', 'A9sus']        # under F3

FORM = [('intro', 0, 'A'), ('intro', 8, 'A'), ('A1', 16, 'A'), ('A1', 24, 'A'),
        ('B1', 32, 'B'), ('B1', 40, 'B'), ('brk', 48, 'A'), ('brk', 56, 'C'),
        ('B2', 64, 'B'), ('B2', 72, 'B'), ('C', 80, 'C'), ('A3', 88, 'A'), ('A3', 96, 'A'),
        ('out', 104, 'A')]
PROGS = {'A': PROG_A, 'B': PROG_B, 'C': PROG_C}
CHORDS = []
PROG_OF = []          # (prog letter, index in prog) per bar
for _name, _b0, _p in FORM:
    for _i in range(8):
        CHORDS.append(PROGS[_p][_i])
        PROG_OF.append((_p, _i))
CHORDS[111] = 'A7sus'           # the final cadence: Am9 | A7sus -> A7b9 | Dm9
PROG_OF[111] = ('B', 7)
CHORDS.append('Dm9')
PROG_OF.append(('end', 0))
assert len(CHORDS) == NBARS

# rootless jazz voicings (the bass plays the roots)
VOICING = {
    'Dm9': ['F3', 'A3', 'C4', 'E4'],
    'Cmaj9': ['E3', 'G3', 'B3', 'D4'],
    'Bm7b5': ['A3', 'B3', 'D4', 'F4'],
    'Am9': ['G3', 'B3', 'C4', 'E4'],
    'Fmaj9': ['A3', 'C4', 'E4', 'G4'],
    'G13': ['F3', 'B3', 'E4', 'A4'],
    'Bbmaj9': ['A3', 'C4', 'D4', 'F4'],
    'A7sus': ['G3', 'A3', 'D4', 'E4'],
    'G9sus': ['F3', 'A3', 'C4', 'D4'],
    'A9sus': ['G3', 'B3', 'D4', 'E4'],
    'A7b9': ['G3', 'C#4', 'E4', 'Bb4'],
}
ROOT = {'Dm9': 'D', 'Cmaj9': 'C', 'Bm7b5': 'B', 'Am9': 'A', 'Fmaj9': 'F', 'G13': 'G', 'Bbmaj9': 'Bb',
        'A7sus': 'A', 'G9sus': 'G', 'A9sus': 'A', 'A7b9': 'A'}

# rolling sub, per bar of each progression: (beat, note, beats)
_AFIG = [(0, 'A1', .75), (.75, 'E2', .75), (1.5, 'G2', .5), (2, 'A2', 1), (3, 'A1', 1)]   # orig bar 4, in A
BASS_A = [[(0, 'D2', 2.75), (2.75, 'A1', 1.25)],
          [(0, 'D2', 1), (1, 'A1', 1), (2, 'D2', 1), (3, 'F2', 1)],          # orig bar 52
          [(0, 'C2', 2.75), (2.75, 'G1', 1.25)],
          [(0, 'C2', 1), (1, 'D2', 1), (2, 'E2', 1), (3, 'C2', 1)],          # orig bar 56
          [(0, 'B1', 2.75), (2.75, 'F1', 1.25)],
          [(0, 'B1', 2), (2, 'F1', 2)],                                      # orig bars 59-60
          [(0, 'A1', 3), (3, 'E1', 1)],                                      # orig bars 61-62
          [(0, 'A1', 2), (2, 'C2', 2)]]                                      # orig bars 63-64
BASS_B = [[(0, 'F1', 2.75), (2.75, 'C2', 1.25)],
          [(0, 'G1', 2.75), (2.75, 'D2', 1.25)],
          [(0, 'C2', 2.75), (2.75, 'G1', 1.25)],
          [(0, 'A1', 3), (3, 'E1', 1)],
          [(0, 'D2', 2.75), (2.75, 'A1', 1.25)],
          [(0, 'Bb1', 2.75), (2.75, 'F1', 1.25)],
          [(0, 'D2', 1), (1, 'A1', 1), (2, 'D2', 1), (3, 'F2', 1)],
          _AFIG]
BASS_C = [[(0, 'D2', 2.75), (2.75, 'A1', 1.25)],
          [(0, 'Bb1', 2.75), (2.75, 'F1', 1.25)],
          [(0, 'F1', 2.75), (2.75, 'C2', 1.25)],
          [(0, 'G1', 2.75), (2.75, 'D2', 1.25)],
          [(0, 'A1', 3), (3, 'E1', 1)],
          [(0, 'D2', 1), (1, 'A1', 1), (2, 'D2', 1), (3, 'F2', 1)],
          [(0, 'G1', 2), (2, 'B1', 1), (3, 'D2', 1)],
          _AFIG]
BASS = {'A': BASS_A, 'B': BASS_B, 'C': BASS_C}

# F1: vibraphone hook, two bars per chord pair: (x, y, z, tail1, tail2)
HOOK_NOTES = {'Dm9': ('C5', 'D5', 'F5', 'D5', 'E5'), 'Cmaj9': ('B4', 'C5', 'E5', 'C5', 'D5'),
              'Bm7b5': ('A4', 'B4', 'D5', 'B4', 'E5'), 'Am9': ('G4', 'A4', 'C5', 'A4', 'B4')}
HOOK_THIRD = {'F5': 'D5', 'E5': 'C5', 'D5': 'B4', 'C5': 'A4'}
# F1 augmented (orig line C5 D5 F5 D5 | C5 D5 F5 E5, two bars a note), per Prog A pair
F1_LONG = [['C5', 'D5', 'F5', 'D5'], ['C5', 'D5', 'F5', 'E5']]

# F2: the lead (beats from the first bar of Prog B; negative = pickup)
F2 = [(-2, 'E5', 1), (-1, 'G5', 1), (0, 'A5', 2), (2, 'E5', 2), (4, 'G5', 2), (6, 'F5', 2), (8, 'E5', 6),
      (14, 'C5', 2), (16, 'D5', 8)]
F2B = [(-2, 'E5', 1), (-1, 'G5', 1), (0, 'A5', 1.5), (1.5, 'G5', .5), (2, 'E5', 2), (4, 'G5', 2), (6, 'F5', 1),
       (7, 'A5', 1), (8, 'E5', 4.5), (12.5, 'F5', .5), (13, 'E5', .5), (13.5, 'D5', .5), (14, 'C5', 2),
       (16, 'D5', 7)]
# vibes answer after the lead (bar 6 of Prog B: Dm9)
F2_ANS = [(24, 'C5', .75), (24.75, 'D5', .75), (25.5, 'F5', 1), (26.5, 'E5', 1.5), (28.5, 'A4', .5), (29, 'D5', 1)]
# F3: the answer theme (beats from the first bar of Prog C)
F3 = [(0, 'A4', 2), (2, 'B4', 2), (4, 'C5', 2), (6, 'D5', 2), (8, 'C5', 4), (12, 'C5', 2), (14, 'D5', 2),
      (16, 'E5', 4), (20, 'D5', 2), (22, 'C5', 2), (24, 'B4', 2), (26, 'A4', 2), (28, 'B4', 2), (30, 'E5', 2)]

# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'hats', 'ride', 'perc', 'sub', 'wbass', 'pad', 'rhodes', 'vibes', 'flute', 'sax',
         'atmos', 'fx']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def load_wav(path, gain=1.0):
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
    x /= np.abs(x).max()
    return x * np.float32(gain)


print('loading samples', flush=True)
FLUTE_S = L.Sampler('Woodwinds/Flute/expvib/*.wav', octave=12, layer=None)
CLAR_S = L.Sampler('Woodwinds/Clarinet/susLong/*.wav', octave=12)
PERC = L.VSCO + '/Percussion/'
TRI = [load_wav(PERC + f) for f in ('Triangle3-HitM_v1_rr1_Sum.wav', 'Triangle3-HitM_v1_rr2_Sum.wav',
                                     'Triangle3-HitM_v2_rr1_Sum.wav')]
CRASH = [load_wav(PERC + f) for f in ('cymbal-crash1_mp_rr1.wav', 'cymbal-crash1_pp_rr1.wav')]
rg = S.rng()


def hv(v, spread=0.08):
    return v * (1 - spread / 2 + spread * rg.random())


# ------------------------------------------------------------- drums

SWING = 0.07


def st(step):
    return step / 4 + (SWING / 4 if step % 2 else 0.0)


PAT = {
    'A': dict(k=[(0, 1.0), (10, .85)], s=[(4, 1.0), (12, 1.0)], g=[(7, .30), (9, .26), (14, .22)]),
    'B': dict(k=[(0, 1.0), (2, .55), (10, .85)], s=[(4, 1.0), (12, 1.0)], g=[(6, .24), (9, .30), (11, .2), (15, .3)]),
    'C': dict(k=[(0, 1.0), (2, .7), (10, .85), (11, .55)], s=[(4, 1.0), (12, 1.0)],
              g=[(7, .42), (9, .38), (15, .42)]),
    'D': dict(k=[(0, 1.0), (8, .6), (10, .8)], s=[(4, 1.0), (12, .95)], g=[(5, .22), (7, .34), (13, .3), (14, .45),
                                                                          (15, .38)]),
    'F': dict(k=[(0, 1.0), (10, .8)], s=[(4, 1.0), (12, 1.0)], g=[(7, .32), (9, .3), (11, .4), (13, .45),
                                                                   (14, .5), (15, .58)]),
    'H': dict(k=[(0, 1.0)], s=[(8, .85)], g=[(14, .25)]),                      # half-time
    'S': dict(k=[(0, 1.0)], s=[(4, 1.0)], g=[]),                               # stop after beat 2
}
_rr = 0


def place_kick(d, step, vel=1.0):
    t = T(d, st(step))
    bus['kick'].add(cached(('kick',), J.jkick), t, hv(vel, 0.05))
    kicks.append(t)


def snare_hit(d, step, vel=1.0):
    global _rr
    _rr += 1
    v = _rr % 4
    if vel >= 0.4:
        sig = cached(('sn', v), lambda: J.jsnare(1.0, v))
    else:
        sig = cached(('gh', v), lambda: J.jsnare(1.0, v, dark=0.85))
    bus['snare'].add(sig, T(d, st(step)), hv(vel, 0.1))


def beat_bar(d, pat='A', hats='8', ride=False, shaker=False, kick=True, snare=True, ghosts=True, vel=1.0):
    if not live(d):
        return
    p = PAT[pat]
    stop = 8 if pat == 'S' else 16
    if kick:
        for s, v in p['k']:
            place_kick(d, s, v * vel)
    if snare:
        for s, v in p['s']:
            snare_hit(d, s, (0.9 if s == 4 else 1.0) * v * vel)
    if ghosts:
        for s, v in p['g']:
            snare_hit(d, s, v * 0.75 * vel)
    if hats:
        for s in range(stop):
            if hats == '8' and s % 2:
                continue
            if hats == '4' and s % 4:
                continue
            acc = [.55, .22, .4, .26][s % 4]
            if s == 14 and hats == '16' and pat in ('A', 'C'):
                bus['hats'].add(cached(('oh',), lambda: J.jhat(1.0, 0.1, open_=True)), T(d, st(s)), hv(.35))
                continue
            bus['hats'].add(J.jhat(1.0, -0.25 + 0.5 * rg.random()), T(d, st(s)), hv(acc * vel, 0.25))
    if shaker:
        for s in range(stop):
            bus['perc'].add(L.shaker(1.0, 0.4), T(d, st(s)), hv(0.13 if s % 2 else 0.2, 0.3) * vel)
    if ride:
        for s, v in ((0, .5), (4, .4), (6, .28), (8, .46), (12, .4), (14, .28)):
            if s < stop:
                bus['ride'].add(cached(('ride',), J.jride), T(d, st(s)), hv(v * vel, 0.12))


def drums(d0, d1, plan, **kw):
    """plan: 8-character pattern string cycled over the bars."""
    for d in range(d0, d1):
        beat_bar(d, plan[(d - d0) % len(plan)], **kw)


def crash_at(d, vel=0.5, k=0):
    if live(d):
        bus['fx'].add(CRASH[k % 2], T(d), vel)


def triangle(d, beat=0.0, vel=0.3):
    if live(d):
        bus['perc'].add(TRI[int(rg.integers(3))], T(d, beat), hv(vel, 0.15))


# ------------------------------------------------------------- bass

def bass_notes(d0, d1, mode='roll'):
    out = []
    for d in range(d0, d1):
        pg, i = PROG_OF[d]
        if pg == 'end':
            out.append((T(d) + T0, m('D2'), RING * 0.8))
            continue
        if mode == 'long':
            out.append((T(d) + T0, m(ROOT[CHORDS[d]] + ('2' if ROOT[CHORDS[d]] in 'CD' else '1')), B(4)))
            continue
        for beat, note, dur in BASS[pg][i]:
            out.append((T(d, beat) + T0, m(note), B(dur)))
    out.sort()
    res = []
    for k, (ts, md, du) in enumerate(out):
        if k + 1 < len(out):
            du = min(du, out[k + 1][0] - ts)
        res.append((ts, md, du))
    return res


def bass(d0, d1, sub=1.0, warm=1.0, mode='roll'):
    a, b = max(d0, RB0), min(d1, RB1)
    if a >= b:
        return
    notes = bass_notes(a, b, mode)
    t_start = T(a) + T0
    n = int((T(b) - T(a) + (RING if b > FINAL else 0.3)) * SR)
    rel = [(ts - t_start, md, du) for ts, md, du in notes]
    pitch, env = L.line_traj(rel, n, glide=0.03)
    if sub:
        y = L.sub_from(pitch, env, drive=1.25)
        bus['sub'].add(np.stack([y, y]), T(a), sub)
    if warm:
        y = J.warm_bass(pitch, env)
        bus['wbass'].add(np.stack([y, y]), T(a), warm)


# ------------------------------------------------------------- harmony

def chord_at(d, beat=0.0):
    ch = CHORDS[d]
    if d == 111 and beat >= 2:
        return 'A7b9'
    return ch


def rhodes_bar(d, style='comp', vel=0.55):
    if not live(d) or d >= FINAL:
        return
    if style == 'pad':
        hits = [(0, 3.9, 1.0)]
    elif style == 'comp':
        hits = [(0, 2.4, 1.0), (2.5, 1.4, 0.7)]
    elif style == 'push':
        hits = [(0, 1.4, 1.0), (1.5, 0.9, 0.6), (2.5, 1.4, 0.8)]
    else:  # 'stab': one short syncopated chord (sparse)
        hits = [(0, 1.0, 1.0), (2.75, 1.2, 0.6)]
    for beat, dur, v in hits:
        ch = chord_at(d, beat)
        for i, note in enumerate(VOICING[ch]):
            vv = round(vel * v * (0.9 + 0.12 * (i == len(VOICING[ch]) - 1)), 2)
            bus['rhodes'].add(cached(('rh', note, dur, vv), lambda: L.rhodes(m(note), B(dur), vv)),
                              T(d, beat + 0.015 * i), 1.0)


def pad_bars(d0, d1, vel=1.0, attack=1.0, cut=1300.0, air=0.0):
    d = max(d0, RB0 - 3)
    d1 = min(d1, RB1, FINAL)
    while d < d1:
        ch = CHORDS[d]
        e = d
        while e < d1 and CHORDS[e] == ch and not (e == 111):
            e += 1
        if e == d:
            e = d + 1
        dur = T(e) - T(d)
        notes = VOICING[ch] + [VOICING[ch][-1][:-1] + str(int(VOICING[ch][-1][-1]) + 1)]
        for note in notes:
            bus['pad'].add(J.jpad(m(note), dur, 1.0, a=attack, r=1.0, cut=cut * 1.5), T(d), vel * 0.3)
        if air:
            for note in VOICING[ch][-2:]:
                bus['pad'].add(J.airpad(m(note) + 12, dur, 1.0, a=attack, r=1.2), T(d), air)
        d = e


def vib(d, beat, note, dur, vel=0.6, harm=False):
    if not live(d):
        return
    md = m(note)
    vel = round(vel * 20) / 20
    bus['vibes'].add(cached(('vb', md, round(dur, 2), round(vel, 2)), lambda: J.vibes(md, B(dur), vel)),
                     T(d, beat), 1.0)
    if harm and note in HOOK_THIRD:
        h = m(HOOK_THIRD[note])
        bus['vibes'].add(cached(('vb', h, round(dur, 2), round(vel * .7, 2)), lambda: J.vibes(h, B(dur), vel * .7)),
                         T(d, beat), 1.0)


def hook_bars(d0, d1, vel=0.6, sparse=False, harm=True):
    """F1 hook: per Prog A chord pair, bar a and bar b."""
    for d in range(d0, d1):
        ch = CHORDS[d]
        x, y, z, t1, t2 = HOOK_NOTES[ch]
        first = d == 0 or CHORDS[d - 1] != ch
        if sparse and not first:
            continue
        tail = t1 if first else t2
        v = hv(vel, 0.1)
        vib(d, 0, x, .75, v * .9)
        vib(d, .75, y, .75, v * .8)
        vib(d, 1.5, z, 1.0, v, harm=harm)
        vib(d, 2.5, tail, 1.5, v * .85)


def lead(notes, d0, fn, transpose=0, octave_shift=0):
    for beat, note, dur in notes:
        bb = d0 * 4 + beat
        d, bt = int(bb // 4), bb % 4
        if live(d):
            fn(d, bt, m(note) + transpose + 12 * octave_shift, dur)


def flute_fn(vel=0.8, pan=0.0):
    def fn(d, beat, md, dur):
        sig = FLUTE_S.note(md, B(dur) - 0.02, vel, r=0.14, a=0.02)
        if pan:
            sig = sig * np.array([[np.sqrt(1 - pan)], [np.sqrt(1 + pan)]], np.float32)
        bus['flute'].add(sig, T(d, beat), 1.0)
    return fn


def sax_fn(vel=0.5):
    def fn(d, beat, md, dur):
        sig = CLAR_S.note(md, B(dur) - 0.02, vel, r=0.12, a=0.035)
        bus['sax'].add(sig, T(d, beat), 1.0)
    return fn


def vibes_fn(vel=0.55):
    def fn(d, beat, md, dur):
        bus['vibes'].add(J.vibes(md, B(dur), vel), T(d, beat), 1.0)
    return fn


def f1_long(d0, d1, fn, transpose=0):
    """F1 augmented: the original inner-voice line, one note per Prog A chord pair."""
    for d in range(d0, d1, 2):
        cyc = ((d - d0) // 8) % 2
        k = ((d - d0) % 8) // 2
        if live(d):
            fn(d, 0, m(F1_LONG[cyc][k]) + transpose, 7.8)


# ---------------------------------------------------------- arrangement

print('arranging', flush=True)
if True:
    bus['atmos'].add(J.wind(N, T0), 0.0, 1.0)

# INTRO 0-15 ---------------------------------------------------------------
pad_bars(0, 16, vel=0.9, attack=2.5, cut=1000, air=0.5)
for d in range(4, 16):
    rhodes_bar(d, 'pad', vel=0.4 if d < 8 else 0.45)
hook_bars(0, 8, vel=0.42, sparse=True, harm=False)
hook_bars(8, 16, vel=0.5)
for d in (0, 4, 8, 12):
    triangle(d, 0, 0.22)
# a distant call: the head of F2 on the flute, far back
lead([(0, 'E5', 1), (1, 'G5', 1), (2, 'A5', 2), (4, 'E5', 4)], 6, flute_fn(0.35, pan=-0.3))
lead([(0, 'E5', 1), (1, 'G5', 1), (2, 'A5', 2), (4, 'G5', 2), (6, 'F5', 2)], 13, flute_fn(0.4, pan=0.3))
# the break fades in from bar 8 (low-passed, see the sweep in the mix)
drums(8, 15, 'AAAB', hats='16', kick=True)
beat_bar(15, 'S', hats='16')
drums(12, 16, 'AAAA', hats=False, kick=False, snare=False, ghosts=False, ride=True)
bass(12, 16, sub=0.7, warm=0.0, mode='long')
bus['fx'].add(S.reverse_cymbal(B(6)), T(15, -2), 0.25)

# A1 16-31: the hook over the rolling break -------------------------------
crash_at(16, 0.45)
bus['fx'].add(S.impact(), T(16), 0.35)
drums(16, 31, 'AABAABCD', hats='16', shaker=True)
beat_bar(31, 'F', hats='16', shaker=True)
crash_at(24, 0.3, 1)
bass(16, 32)
for d in range(16, 32):
    rhodes_bar(d, 'comp', vel=0.5)
pad_bars(16, 32, vel=0.55, attack=0.8, cut=1300)
hook_bars(16, 32, vel=0.6)
for d in (18, 20, 26, 28):
    triangle(d, 0, 0.2)

# B1 32-47: the lead (F2) -------------------------------------------------
crash_at(32, 0.5)
drums(32, 47, 'AABCAABD', hats='16', ride=True)
beat_bar(47, 'F', hats='16', ride=True)
crash_at(40, 0.3, 1)
bass(32, 48)
for d in range(32, 48):
    rhodes_bar(d, 'comp', vel=0.5)
pad_bars(32, 48, vel=0.6, attack=0.6, cut=1400)
lead(F2, 32, flute_fn(0.85))
lead(F2B, 40, flute_fn(0.88))
lead(F2_ANS, 32, vibes_fn(0.55))
lead(F2_ANS, 40, vibes_fn(0.6))
for d in (34, 36, 42, 44):
    triangle(d, 0, 0.2)

# BREAKDOWN 48-63: floating, then F3 over a half-time pulse --------------
crash_at(48, 0.35, 1)
bus['fx'].add(S.impact(), T(48), 0.3)
bass(48, 56, sub=0.75, warm=0.0, mode='long')
bass(56, 64, sub=0.85, warm=0.5)
for d in range(48, 64):
    rhodes_bar(d, 'pad', vel=0.45)
pad_bars(48, 64, vel=0.85, attack=1.5, cut=1100, air=0.6)
hook_bars(48, 56, vel=0.5, sparse=False)
drums(48, 56, 'AAAAAAAA', hats=False, kick=False, snare=False, ghosts=False, ride=True, vel=0.7)
# half-time pulse: kick on 1, the original side-stick figure on 2.5/3
for d in range(56, 60):
    if live(d):
        place_kick(d, 0, 0.75)
        bus['perc'].add(J.rimstick(1.0), T(d, 2), hv(0.5))
        if d % 2 == 1:
            bus['perc'].add(J.rimstick(1.0, 0.15), T(d, 1.5), hv(0.32))
drums(56, 60, 'HHHH', hats='8', kick=False, snare=False, ghosts=False, vel=0.6)
drums(60, 63, 'AABA', hats='16')
beat_bar(63, 'S', hats='16')
lead(F3, 56, flute_fn(0.75))
lead(F3, 56, vibes_fn(0.3), transpose=-12)
bus['fx'].add(S.noise_riser(T(63, 2) - T(60), 300, 6000, 1.5), T(60), 0.12)
bus['fx'].add(S.reverse_cymbal(B(6)), T(63, -2), 0.3)

# B2 64-79: the peak, lead on flute + "sax" -------------------------------
crash_at(64, 0.55)
bus['fx'].add(S.impact(), T(64), 0.45)
drums(64, 79, 'AABCAABD', hats='16', ride=True, shaker=True)
beat_bar(79, 'F', hats='16', ride=True, shaker=True)
crash_at(72, 0.35, 1)
bass(64, 80)
for d in range(64, 80):
    rhodes_bar(d, 'push', vel=0.52)
pad_bars(64, 80, vel=0.65, attack=0.5, cut=1500)
lead(F2, 64, flute_fn(0.9))
lead(F2, 64, sax_fn(0.55), octave_shift=-1)
lead(F2B, 72, flute_fn(0.92))
lead(F2B, 72, sax_fn(0.6), octave_shift=-1)
lead(F2_ANS, 64, vibes_fn(0.6))
lead(F2_ANS, 72, vibes_fn(0.65))
for d in (66, 68, 74, 76):
    triangle(d, 0, 0.22)

# C 80-87: the answer theme with the full break ---------------------------
crash_at(80, 0.45)
drums(80, 87, 'CABCCABD', hats='16', ride=True, shaker=True)
beat_bar(87, 'F', hats='16', ride=True, shaker=True)
bass(80, 88)
for d in range(80, 88):
    rhodes_bar(d, 'comp', vel=0.52)
pad_bars(80, 88, vel=0.65, attack=0.5, cut=1500)
lead(F3, 80, flute_fn(0.88))
lead(F3, 80, sax_fn(0.5), octave_shift=-1)

# A3 88-103: climax, the hook with F1 augmented on top --------------------
crash_at(88, 0.55)
bus['fx'].add(S.impact(), T(88), 0.4)
drums(88, 103, 'AABCAABD', hats='16', ride=True, shaker=True)
beat_bar(103, 'F', hats='16', ride=True, shaker=True)
crash_at(96, 0.35, 1)
bass(88, 104)
for d in range(88, 104):
    rhodes_bar(d, 'comp', vel=0.52)
pad_bars(88, 104, vel=0.7, attack=0.5, cut=1600)
hook_bars(88, 104, vel=0.62)
f1_long(88, 104, flute_fn(0.75))
f1_long(96, 104, sax_fn(0.45), transpose=-12)
for d in (90, 92, 98, 100):
    triangle(d, 0, 0.22)

# OUTRO 104-112: thinning out, the cadence and a final hit ----------------
crash_at(104, 0.4, 1)
drums(104, 108, 'AABA', hats='16', shaker=True)
drums(108, 111, 'AAB', hats='8')
beat_bar(111, 'A', hats='8', ghosts=False)
bass(104, NBARS)
for d in range(104, 112):
    rhodes_bar(d, 'comp' if d < 108 else 'pad', vel=0.5)
pad_bars(104, 112, vel=0.65, attack=0.7, cut=1300, air=0.3)
hook_bars(104, 110, vel=0.55)
# bar 110 Am9 (hook bar a), bar 111 A7sus -> A7b9: the F2 pickup turned into a cadence
for bt, nt, du, v in ((0, 'G4', .75, .5), (.75, 'A4', .75, .45), (1.5, 'C5', 1.0, .55), (2.5, 'A4', 1.5, .45)):
    vib(110, bt, nt, du, v)
lead([(0, 'E5', 1), (1, 'G5', 1), (2, 'A5', 1), (3, 'E5', 1)], 111, flute_fn(0.7))
lead([(2, 'C#5', 1), (3, 'E5', 1)], 111, vibes_fn(0.5))
if live(FINAL):
    place_kick(FINAL, 0, 1.0)
    snare_hit(FINAL, 0, 0.7)
    crash_at(FINAL, 0.4)
    for note in ['D3', 'A3', 'C4', 'E4', 'F4']:
        bus['rhodes'].add(L.rhodes(m(note), 1.2, 0.55, r=0.6), T(FINAL), 1.0)
    for note in ['F3', 'A3', 'C4', 'E4', 'A4']:
        bus['pad'].add(J.jpad(m(note), 1.0, 1.0, a=0.02, r=0.8, cut=1500), T(FINAL), 0.2)
    vib(FINAL, 0, 'D5', 6, 0.6)
    vib(FINAL, 0, 'A4', 6, 0.45)
    flute_fn(0.7)(FINAL, 0, m('D5'), 3.5)
    sax_fn(0.45)(FINAL, 0, m('D4'), 3.5)
    triangle(FINAL, 0, 0.3)

# ------------------------------------------------------------- mix

print('mixing', flush=True)
del FLUTE_S, CLAR_S, TRI, CRASH
_cache.clear()
gc.collect()


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


def sweep(k, d0, d1, f0, f1, shape=1.6, q=0.7):
    """time-varying low-pass on bus k between bars d0..d1 (f0 -> f1)."""
    i0, i1 = int(max(0, T(d0)) * SR), int(max(0, T(d1)) * SR)
    i1 = min(i1, N)
    if i1 - i0 < 100:
        return
    u = np.linspace(0, 1, i1 - i0)
    fc = f0 * (f1 / f0) ** (u ** shape)
    bus[k].x[:, i0:i1] = S.svf(bus[k].x[:, i0:i1], fc, q).astype(np.float32)


# intro: the break opens from a dark low-pass; the breakdown's break returns the same way
for k in ('snare', 'hats', 'kick', 'ride'):
    sweep(k, 8, 16, 300, 16000, 2.0)
    sweep(k, 60, 64, 500, 16000, 1.6)
for k in ('vibes', 'pad'):
    sweep(k, 0, 8, 1200, 16000, 1.2)

duck_bass = S.sidechain(N, kicks, depth=0.4, release=0.12, power=1.5)
duck_mid = S.sidechain(N, kicks, depth=0.15, release=0.16)

GAIN = {  # dB
    'kick': -3.0, 'snare': 4.0, 'hats': 4.0, 'ride': 2.0, 'perc': -3.0,
    'sub': -11.5, 'wbass': -21.0, 'pad': -3.5, 'rhodes': -1.5, 'vibes': -2.0, 'flute': 6.0, 'sax': 0.0,
    'atmos': -26.0, 'fx': -8.0,
}
DUCK = {'sub': duck_bass, 'wbass': duck_bass, 'pad': duck_mid, 'rhodes': duck_mid}
ROOM = {'snare': .22, 'hats': .1, 'kick': .05, 'perc': .15}
HALL = {'snare': .06, 'rhodes': .3, 'vibes': .4, 'flute': .38, 'sax': .3, 'pad': .35, 'perc': .3, 'fx': .25,
        'ride': .08, 'atmos': .3}
DELAY = {'flute': .2, 'vibes': .22, 'rhodes': .06, 'sax': .1}

# bus processing
bus['rhodes'].x = stage(bus['rhodes'].x, HighpassFilter(120), PeakFilter(280, -4.0, 0.8),
                        LowpassFilter(7000), Chorus(rate_hz=0.5, depth=0.2, centre_delay_ms=6, mix=0.35))
_t = (np.arange(N) / SR + T0)
_trem = 0.5 + 0.5 * np.sin(2 * np.pi * _t / (3 * SPB))          # slow auto-pan, dotted half
bus['rhodes'].x[0] *= (0.7 + 0.3 * _trem).astype(np.float32)
bus['rhodes'].x[1] *= (0.7 + 0.3 * (1 - _trem)).astype(np.float32)
del _trem, _t
bus['vibes'].x = stage(bus['vibes'].x, HighpassFilter(200), LowpassFilter(9000),
                       Compressor(threshold_db=-20, ratio=2, attack_ms=5, release_ms=120))
bus['flute'].x = stage(bus['flute'].x, HighpassFilter(280), PeakFilter(3500, -1.5, 1.0), LowpassFilter(9500),
                       Compressor(threshold_db=-24, ratio=2.5, attack_ms=15, release_ms=150))
bus['sax'].x = stage(bus['sax'].x, HighpassFilter(160), PeakFilter(1300, 3.0, 0.9), Distortion(drive_db=5),
                     LowpassFilter(5500), Compressor(threshold_db=-22, ratio=3, attack_ms=10, release_ms=120))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(250), PeakFilter(400, -2.0, 1.0), Chorus(rate_hz=0.2, depth=0.35, centre_delay_ms=9, mix=0.45))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(140), LowpassFilter(8000),
                       Compressor(threshold_db=-18, ratio=3, attack_ms=4, release_ms=70))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(5000), LowpassFilter(11500))
bus['ride'].x = stage(bus['ride'].x, HighpassFilter(2500), LowpassFilter(10000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(400), LowpassFilter(12000))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(40), LowpassFilter(10000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(160))
bus['wbass'].x = stage(bus['wbass'].x, HighpassFilter(90), LowpassFilter(900))
bus['atmos'].x = stage(bus['atmos'].x, HighpassFilter(200), LowpassFilter(4000))
# the wind comes forward in the quiet parts
_bars = (np.arange(N) / SR + T0) / (4 * SPB)
_at = np.interp(_bars, [0, 8, 16, 16.01, 48, 48.01, 56, 64, 104, 112, 116],
                [1.0, 0.8, 0.6, 0.3, 0.3, 0.9, 0.8, 0.3, 0.3, 0.6, 0.0])
bus['atmos'].x *= _at.astype(np.float32)[None, :]
del _at

mix = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('NEW03_MUTE', '').split(',')))
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
    mix += x
    if k in ROOM:
        room_send += x * ROOM[k]
    if k in HALL:
        hall_send += x * HALL[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

room = stage(room_send, HighpassFilter(250), LowpassFilter(7000),
             Reverb(room_size=0.25, damping=0.55, wet_level=1.0, dry_level=0.0, width=0.8))
del room_send
pre = int(0.04 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
del hall_send
hall = stage(hs, HighpassFilter(300), LowpassFilter(7500),
             Reverb(room_size=0.93, damping=0.45, wet_level=1.0, dry_level=0.0, width=1.0))
del hs
hall *= duck_mid[None, :]
dly = S.pingpong(delay_send, B(0.75), feedback=0.45, repeats=8, lp=4000, hp=400)
del delay_send
dly = stage(dly, Reverb(room_size=0.6, damping=0.6, wet_level=0.35, dry_level=1.0, width=1.0))
mix += room * 0.8 + hall * 0.85 + dly * 0.55
del room, hall, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -5.0), (8, -4.5), (15.9, -3.5), (16, -2.5), (31.9, -2.5), (32, -1.5), (47.9, -1.5),
              (48, -6.5), (55.9, -6.5), (56, -5.5), (63.9, -2.5), (64, 0.8), (87.9, 0.8), (88, 1.0),
              (103.9, 1.0), (104, -0.5), (111.9, -1.5), (112, -1.0), (130, -1.0)]
sec = np.interp(_bars, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.5 - loud) / 20))
mix = stage(mix, LowShelfFilter(70, 0.5, 0.7), PeakFilter(350, -2.0, 0.8), PeakFilter(2200, 1.5, 0.7),
            HighShelfFilter(2500, 2.0, 0.6),
            Compressor(threshold_db=-16, ratio=2.0, attack_ms=25, release_ms=200))
mix = np.tanh(mix * 1.05) / 1.05
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=60))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

# trim: the final hit rings ~RING s, then a short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
if RB1 == NBARS:
    last = min(last, int((T(FINAL) + RING) * SR))
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new03_jungle.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
