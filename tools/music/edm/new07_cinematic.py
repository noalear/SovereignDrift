#!/usr/bin/env python3
# new07_cinematic.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Rimward Patrol": a NEW piece of cinematic electronica (orchestral /
# electronic hybrid, late-90s space-game style), 108 BPM, C minor, built from
# material of the original game's song 7 (Stardock, mid/sf-music7.mid) that
# its remix (song7_orchestral.py) did not use or used only in passing.
# Mood: a mission briefing / deep-space patrol: tension and wonder, no bombast.
#
# Credits: the original music is Stardock's (Stellar Frontier, 1997,
# mid/sf-music7.mid).  Samples: VSCO 2 Community Edition (Versilian Studios;
# Sam Gossner, Simon Dalzell, Elan Hickler), CC0 1.0 / public domain,
# https://github.com/sgossner/VSCO-2-CE (violin/viola/cello sections, solo
# contrabass, F horn, tenor trombone, flute, oboe, clarinet, bassoon,
# timpani, suspended cymbal, crash), played by synth_orch.Sampler.  All
# electronic sounds are synthesized (synth.py, synth_orch.py, synth_new07.py).
#
# Fragments of sf-music7.mid (bar numbers count from the bass's first
# downbeat, after the two-beat pickup, as in song7_orchestral.py):
#   F1  the clarinet lament    C4(3) Eb4(1) | Db4(2) Bb3(2) | G3(6), bars 53-55
#       (prog 71, a counter-line the remix never used).  It becomes theme A:
#       sequenced up through F Ab | G Eb | C and Ab C | Bb G | Eb, with a
#       Phrygian Db cadence, reharmonized over a moving bass
#       (Cm Db Cm | Fm Eb Ab | Ab Eb/G Cm | Fm Db G); quoted verbatim by the
#       distant horn (intro, outro) and the solo clarinet (breakdown).
#   F2  the choir-aah line     C4 | Eb4 Ab4 | Bb4 | Gb4 || C4 | Eb4 Ab4 | Bb4 |
#       Db5 | G4 | Eb4 D4, bars 39-48 (prog 52 and its octave below, prog 0
#       ch. 63/64; the remix only used its first five notes on horns).  It
#       becomes theme B, one note per bar, reharmonized
#       Ab Fm Gb Ebm | Ab Fm Gb Db | G G; its G | Eb D tail is also the
#       cadence of theme A.
#   F3  the bassoon figure     (beat 2) G3 C3 D3 | Eb3, bar 33 (prog 70, one
#       transition bar in the original).  It becomes the "response" that
#       answers each phrase of theme A (bassoon, then trombones), and, as
#       5-1-2-b3 of every chord, the cell of the synth arpeggio and the
#       violins' spiccato ostinato.
# New: everything else: the form (intro, A, B, breakdown, A tutti, B tutti,
# outro that cadences back to the intro), the harmony, the electronic pulse,
# a restrained breakbeat with a short round snare, choir and pads.
#
#   python3 tools/music/edm/new07_cinematic.py OUT.wav [FROM_BAR TO_BAR]
#   NEW07_MUTE=kick,snare,hats,cym,timp,fx   mutes buses (for chroma checks)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
import scipy.signal as ss
from pedalboard import (Pedalboard, Compressor, Limiter, HighpassFilter, LowpassFilter,
                        LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_orch as O  # noqa: E402
import synth_new07 as X  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 108.0
SPB = 60.0 / BPM
NBARS = 74
TAIL = 4.5
N = int((NBARS * 4 * SPB + TAIL) * SR)
FROM_BAR = float(sys.argv[2]) if len(sys.argv) > 3 else 0
TO_BAR = float(sys.argv[3]) if len(sys.argv) > 3 else 999
RNG = np.random.default_rng(707)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def active(bar, beat=0.0):
    b = bar + beat / 4
    return FROM_BAR - 1 <= b < TO_BAR


def notes(spec):
    if isinstance(spec, (int, np.integer)):
        return [int(spec)]
    return [m(x) for x in spec.split('+')]


# ------------------------------------------------------------- harmony

CH = {   # name: (bass root midi, pad voicing)
    'Cm': (36, ['G3', 'C4', 'Eb4', 'G4']),
    'Db/C': (36, ['Ab3', 'Db4', 'F4', 'Ab4']),
    'Db': (37, ['Ab3', 'Db4', 'F4', 'Ab4']),
    'Fm': (41, ['Ab3', 'C4', 'F4', 'Ab4']),
    'Eb': (39, ['G3', 'Bb3', 'Eb4', 'G4']),
    'Eb/G': (43, ['G3', 'Bb3', 'Eb4', 'G4']),
    'Eb/F': (41, ['G3', 'Bb3', 'Eb4', 'G4']),
    'Ab': (44, ['Ab3', 'C4', 'Eb4', 'Ab4']),
    'Ab/C': (36, ['Ab3', 'C4', 'Eb4', 'Ab4']),
    'Gb': (42, ['Gb3', 'Bb3', 'Db4', 'Gb4']),
    'Ebm': (39, ['Gb3', 'Bb3', 'Eb4', 'Gb4']),
    'G': (43, ['G3', 'B3', 'D4', 'G4']),
    'Gx': (43, ['G3', 'B3', 'G4']),        # under the Eb -> D appoggiatura
}
# the F3 cell (5 1 2 b3 of C minor) on every chord
CELL = {
    'Cm': ['G3', 'C4', 'D4', 'Eb4'], 'Db/C': ['Ab3', 'Db4', 'Eb4', 'F4'], 'Db': ['Ab3', 'Db4', 'Eb4', 'F4'],
    'Fm': ['C4', 'F4', 'G4', 'Ab4'], 'Eb': ['Bb3', 'Eb4', 'F4', 'G4'], 'Eb/G': ['Bb3', 'Eb4', 'F4', 'G4'],
    'Eb/F': ['Bb3', 'Eb4', 'F4', 'G4'], 'Ab': ['Eb3', 'Ab3', 'Bb3', 'C4'], 'Ab/C': ['Eb3', 'Ab3', 'Bb3', 'C4'],
    'Gb': ['Db4', 'Gb4', 'Ab4', 'Bb4'], 'Ebm': ['Bb3', 'Eb4', 'F4', 'Gb4'], 'G': ['D4', 'G4', 'Ab4', 'B4'],
    'Gx': ['D4', 'G4', 'Ab4', 'B4'],
}
for _k, _v in CELL.items():      # keep the cell centred around D4
    _mm = [m(x) for x in _v]
    _sh = int(round((62 - np.mean(_mm)) / 12)) * 12
    CELL[_k] = [x + _sh for x in _mm]

INTRO = ['Cm', 'Cm', 'Db/C', 'Cm', 'Cm', 'Db/C', 'Cm', 'Cm']
A_CH = ['Cm', 'Db', 'Cm', 'Cm', 'Fm', 'Eb', 'Ab', 'Ab', 'Ab', 'Eb/G', 'Cm', 'Cm', 'Fm', 'Db', 'G', 'Gx']
B_CH = ['Ab', 'Fm', 'Gb', 'Ebm', 'Ab', 'Fm', 'Gb', 'Db', 'G', 'Gx']
BRK = ['Cm', 'Db/C', 'Cm', 'Cm', 'Fm', 'Eb/F', 'Ab/C', 'G']
OUT = ['Cm', 'Db/C', 'Cm', 'Cm']
CHORDS = INTRO + A_CH + B_CH + ['Cm', 'Cm'] + BRK + A_CH + B_CH + OUT
assert len(CHORDS) == NBARS, len(CHORDS)
S_INTRO, S_A1, S_B1, S_TR, S_BRK, S_A2, S_B2, S_OUT = 0, 8, 24, 34, 36, 44, 60, 70


def chord_at(bar):
    return CHORDS[int(np.clip(bar, 0, NBARS - 1))]


def root_at(bar):
    return CH[chord_at(bar)][0]


# ------------------------------------------------------------ themes
# (beat, note(s), length in beats[, accent])

F1 = [(0, 'C4', 3), (3, 'Eb4', 1), (4, 'Db4', 2), (6, 'Bb3', 2), (8, 'G3', 6)]   # orig bars 53-55
THEME_A = [
    (0, 'C4', 3), (3, 'Eb4', 1), (4, 'Db4', 2), (6, 'Bb3', 2), (8, 'G3', 6),
    (16, 'F4', 3), (19, 'Ab4', 1), (20, 'G4', 2), (22, 'Eb4', 2), (24, 'C4', 6),
    (32, 'Ab4', 3), (35, 'C5', 1), (36, 'Bb4', 2), (38, 'G4', 2), (40, 'Eb4', 6),
    (48, 'Ab4', 3), (51, 'G4', 1), (52, 'F4', 2), (54, 'Db4', 2), (56, 'G4', 4),
    (60, 'Eb4', 1.5), (61.5, 'D4', 2.5),
]
# F3 (orig bar 33) answering phrases a, b, c (theme bars 3, 7, 11)
RESP = {3: [(1, 'G3', 1), (2, 'C3', 1), (3, 'D3', 1), (4, 'Eb3', 2)],
        7: [(1, 'Eb3', 1), (2, 'Ab2', 1), (3, 'Bb2', 1), (4, 'C3', 2)],
        11: [(1, 'C4', 1), (2, 'F3', 1), (3, 'G3', 1), (4, 'Ab3', 2)]}
THEME_B = [   # orig bars 39-48, one note per bar
    (0, 'C4', 4), (4, 'Eb4', 2), (6, 'Ab4', 2), (8, 'Bb4', 4), (12, 'Gb4', 4),
    (16, 'C4', 4), (20, 'Eb4', 2), (22, 'Ab4', 2), (24, 'Bb4', 4), (28, 'Db5', 4),
    (32, 'G4', 4), (36, 'Eb4', 1.5), (37.5, 'D4', 2.5),
]

# --------------------------------------------------------------- players

O._RAW_BUDGET = 300e6      # smaller sample caches: keeps the peak memory under ~2.3 GB
O._HIT_BUDGET = 120e6
smp = O.Sampler()
SEAT = {   # pan, width, bus
    'vln1': (-0.55, 0.6, 'vln'), 'vln2': (-0.25, 0.6, 'vln'), 'vla': (0.12, 0.55, 'vla'),
    'vc': (0.38, 0.5, 'lowstr'), 'cb': (0.55, 0.35, 'lowstr'),
    'horn': (-0.3, 0.5, 'horn'), 'horn2': (-0.15, 0.5, 'horn'),
    'tbn': (0.25, 0.4, 'lowbr'), 'tpt': (0.3, 0.35, 'lowbr'),
    'fl': (-0.1, 0.3, 'ww'), 'ob': (0.05, 0.3, 'ww'), 'cl': (-0.05, 0.3, 'ww'), 'bsn': (0.18, 0.3, 'ww'),
}
ARTS = {
    'vln1': {'sus': 'vln_sus', 'short': 'vln_spic', 'trem': 'vln_trem'},
    'vln2': {'sus': 'vln_sus', 'short': 'vln_spic', 'trem': 'vln_trem'},
    'vla': {'sus': 'vla_sus', 'short': 'vla_spic'},
    'vc': {'sus': 'vc_sus', 'short': 'vc_spic', 'trem': 'vc_trem'},
    'cb': {'sus': 'cb_sus', 'short': 'cb_spic'},
    'horn': {'sus': 'horn_sus', 'short': 'horn_stac'}, 'horn2': {'sus': 'horn_sus', 'short': 'horn_stac'},
    'tbn': {'sus': 'tbn_sus', 'short': 'tbn_stac'}, 'tpt': {'sus': 'tpt_sus'},
    'fl': {'sus': 'fl_sus'}, 'ob': {'sus': 'ob_sus'}, 'cl': {'sus': 'cl_sus'}, 'bsn': {'sus': 'bsn_sus'},
}
BUSES = ['vln', 'lowstr', 'horn', 'lowbr', 'ww', 'choir', 'pad', 'arp', 'pulse',
         'kick', 'snare', 'hats', 'timp', 'cym', 'echo']
ALIAS = {'vla': ('vln', -1.5), 'sub': ('pulse', -3.0), 'fx': ('cym', -3.0)}   # shared buses (memory)
bus = {k: S.Bus(N) for k in BUSES}
KICKS = []
HUM = [0.010, 0.07]


def put(name, sig, t, gain=1.0):
    if name in ALIAS:
        name, db = ALIAS[name]
        gain *= 10 ** (db / 20)
    bus[name].add(sig, t, gain)


def play(player, mel, bar0, transpose=0, vel=0.7, art='auto', short_max=0.5, rel=None,
         attack=0.0, legato=True, double=False, swell=None, echo=0.0):
    """Note list on a seated player (sampled).  art 'auto': short articulation
    up to short_max beats, sustain otherwise; legato between touching notes."""
    pan, width, bname = SEAT[player]
    arts = ARTS[player]
    mel = sorted(mel, key=lambda e: e[0])
    for i, ev in enumerate(mel):
        beat, spec, ln = ev[0], ev[1], ev[2]
        acc = ev[3] if len(ev) > 3 else 1.0
        if not active(bar0, beat):
            continue
        a = art
        if a == 'auto':
            a = 'short' if ('short' in arts and ln <= short_max) else 'sus'
        key = arts.get(a, arts['sus'])
        is_short = a == 'short'
        leg = nxt_leg = False
        if legato and not is_short and i > 0:
            pb, _, pl = mel[i - 1][:3]
            leg = abs(pb + pl - beat) < 0.05 and pl > short_max
        if legato and not is_short and i + 1 < len(mel):
            nxt_leg = abs(beat + ln - mel[i + 1][0]) < 0.05 and mel[i + 1][2] > short_max
        dur = B(ln) + (0.06 if nxt_leg else 0.0)
        r = rel if rel is not None else (0.12 if is_short else (0.07 if nxt_leg else 0.45))
        for md in notes(spec):
            md += transpose
            copies = [(0.0, 0.0, 0.0, 1.0)]
            if double:
                copies.append((0.010 + 0.012 * RNG.random(), (RNG.random() - 0.5) * 0.12,
                               0.12 * (1 if RNG.random() > 0.5 else -1), 0.8))
            for dt, det, dp, g in copies:
                v = float(np.clip(vel * acc * (1 + RNG.normal(0, HUM[1])), 0.02, 1.0))
                sig = smp.note(key, md, max(dur, 0.05), v, rel=r, attack=attack, legato=leg, detune=det)
                if swell:
                    k = min(int(dur * SR), sig.shape[1])
                    u = np.linspace(0, 1, k, dtype=np.float32)
                    sig[:, :k] *= swell + (1 - swell) * u ** 1.5
                t = T(bar0, beat) + dt + (0 if leg else RNG.normal(0, HUM[0]))
                t = max(t, T(bar0, beat) - 0.02)
                st = O.seat(sig, float(np.clip(pan + dp, -1, 1)), width)
                put(bname, st, t, g)
                if echo:
                    put('echo', st, t, g * echo)


def pads(player, bar0, bar1, vel=0.4, transpose=0, pick=None, attack=0.4, art='sus', double=False,
         swell=None):
    """Sustained chords from CHORDS (held while the chord stays)."""
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        voic = CH[c][1]
        if pick is not None:
            voic = [voic[j] for j in pick if j < len(voic)]
        play(player, [(0, '+'.join(voic), (e - b) * 4)], b, transpose=transpose, vel=vel, art=art,
             attack=attack, legato=False, double=double, swell=swell)
        b = e


def roots(player, bar0, bar1, vel=0.5, octave=0, attack=0.3, low=28):
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        r = CH[c][0] + octave
        if r < low:
            r += 12
        play(player, [(0, r, (e - b) * 4)], b, vel=vel, attack=attack, legato=False)
        b = e


def ostinato_8ths(player, bar0, bar1, vel=0.4, transpose=12, pat=(0, 1, 2, 3, 2, 1, 0, 1)):
    """F3 cell in spiccato 8ths."""
    for b in range(bar0, bar1):
        cell = CELL[chord_at(b)]
        mel = [(i * .5, cell[j] + transpose, .4, 1.0 if i % 2 == 0 else 0.8) for i, j in enumerate(pat)]
        play(player, mel, b, vel=vel, art='short', legato=False)


# ------------------------------------------------------------ synths

from collections import OrderedDict
_cache = OrderedDict()
_CACHE_BUDGET = 120e6


def cached(key, fn):
    """LRU cache of rendered synth notes (bounded, to keep the peak memory down)."""
    if key in _cache:
        _cache.move_to_end(key)
        return _cache[key]
    y = fn()
    _cache[key] = y
    while sum(v.nbytes for v in _cache.values()) > _CACHE_BUDGET and len(_cache) > 1:
        _cache.popitem(last=False)
    return y


def vox_pads(bar0, bar1, vel=0.4, transpose=0, pick=None, vowel='oh', attack=1.2):
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        if active(b):
            voic = CH[c][1]
            if pick is not None:
                voic = [voic[j] for j in pick if j < len(voic)]
            ln = B((e - b) * 4)
            for nn in voic:
                md = m(nn) + transpose
                sig = cached(('vox', md, round(ln, 2), vowel, attack),
                             lambda: X.vox(md, ln, 1.0, attack=attack, vowel=vowel, seed=md))
                put('choir', sig, T(b), vel)
        b = e


def vox_line(mel, bar0, vel=0.4, transpose=0, vowel='oh'):
    for bt, spec, ln in mel:
        if active(bar0, bt):
            for md in notes(spec):
                md += transpose
                sig = X.vox(md, B(ln) + 0.15, 1.0, attack=0.45, rel=0.8, vowel=vowel, seed=3)
                put('choir', sig, T(bar0, bt), vel)


def synth_pads(bar0, bar1, vel=0.4, cut=1300.0, transpose=0):
    """Warm JP/Juno string-machine pad."""
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        if active(b):
            ln = B((e - b) * 4)
            for nn in CH[c][1]:
                md = m(nn) + transpose
                sig = cached(('pad', md, round(ln, 2), cut),
                             lambda: S.supersaw(md, ln, 1.0, voices=5, detune=0.14, width=0.95, a=0.9, d=1.5,
                                                s=0.85, r=0.9, cut=cut, q=0.7).astype(np.float32))
                put('pad', sig, T(b), vel)
        b = e


def arp(bar0, bar1, vel=0.4, bright=0.6, pat=(0, 1, 2, 3, 2, 1, 0, 1), octave=12, ramp=None):
    """The F3 cell as an 8th-note pluck arpeggio (ping-pong delayed on its bus)."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        cell = CELL[chord_at(b)]
        br = bright if ramp is None else ramp[0] + (ramp[1] - ramp[0]) * (b - bar0) / max(bar1 - bar0 - 1, 1)
        brq = round(br, 1)
        for i, j in enumerate(pat):
            md = cell[j] + octave
            sig = cached(('arp', md, brq), lambda: S.pluck(md, B(.4), 1.0, decay=0.16 + 0.1 * brq, bright=brq,
                                                             r=0.08, detune=0.07, width=0.7).astype(np.float32))
            v = vel * (1.0 if i % 2 == 0 else 0.75) * (1 + RNG.normal(0, 0.05))
            put('arp', sig, T(b, i * .5), v)


def pulse(bar0, bar1, vel=0.5, div=2, cut=380.0, env=1300.0):
    """Steady electronic pulse on the bass root (filtered saw + sub octave)."""
    step = 1.0 / div
    for b in range(bar0, bar1):
        if not active(b):
            continue
        r = root_at(b) + 12
        for i in range(4 * div):
            bt = i * step
            acc = abs(bt - round(bt)) < 1e-6
            ln = 0.42 if div == 2 else 0.2
            key = ('pulse', r, ln, acc, cut, env)
            sig = cached(key, lambda: O.pulse(r, B(ln), 1.0, cut=cut, env=env * (1.3 if acc else 0.8),
                                              decay=0.07 if acc else 0.05))
            put('pulse', sig, T(b, bt), vel * (1.0 if acc else 0.7))


def drone(bar0, bar1, vel=0.4):
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and CH[chord_at(e)][0] == CH[c][0]:
            e += 1
        if active(b):
            y = S.sub_bass(CH[c][0], T(e) - T(b) + 0.2, 1.0, a=0.8, r=0.6, drive=1.1).astype(np.float32)
            put('sub', y, T(b), vel)
        b = e


# ---------------------------------------------------------------- drums

_rr = {'sn': 0, 'hat': 0, 'shk': 0}


def kick(bar, beat, vel=0.8):
    if active(bar, beat):
        t = T(bar, beat) + RNG.normal(0, 0.002)
        put('kick', X.deep_kick(vel), t)
        KICKS.append(t)


def snare(bar, beat, vel=0.7, ghost=False):
    if active(bar, beat):
        _rr['sn'] += 1
        v = float(np.clip(vel * (1 + RNG.normal(0, 0.07)), 0, 1))
        sig = X.round_snare(v, _rr['sn'], dark=1.0 if ghost else 0.0)
        put('snare', O.seat(sig, 0.06, 0.4), T(bar, beat) + RNG.normal(0, 0.003))


def hat(bar, beat, vel=0.4, open_=False, pan=0.25):
    if active(bar, beat):
        _rr['hat'] += 1
        sig = X.soft_hat(float(np.clip(vel * (1 + RNG.normal(0, 0.1)), 0, 1)), _rr['hat'], open_)
        put('hats', O.seat(sig, pan, 0.3), T(bar, beat) + RNG.normal(0, 0.003))


def shk(bar, beat, vel=0.3):
    if active(bar, beat):
        _rr['shk'] += 1
        put('hats', O.seat(X.shaker(vel, _rr['shk']), -0.35, 0.4), T(bar, beat) + RNG.normal(0, 0.004))


GROOVE = [   # (kicks, snares, ghost snares) per bar, in beats: a restrained break
    ([0, 1.75, 2.5], [1, 3], [3.75]),
    ([0, 0.75, 2.5, 2.75], [1, 3], [1.75, 3.5]),
]


def groove(bar0, bar1, level=1.0, ghosts=True, hats16=False, shaker16=False, kick_only=False, fill_last=False):
    for b in range(bar0, bar1):
        ks, ss_, gs = GROOVE[(b - bar0) % 2]
        last = fill_last and b == bar1 - 1
        for k in ks:
            if last and k > 2.6:
                continue
            kick(b, k, 0.85 * level * (1.0 if k == 0 else 0.8))
        if kick_only:
            continue
        for s in ss_:
            snare(b, s, 0.75 * level * (1.0 if s == 3 else 0.93))
        if ghosts and not last:
            for g in gs:
                snare(b, g, 0.16 * level, ghost=True)
        for i in range(8):
            bt = i * .5
            hat(b, bt, (0.32 if i % 2 else 0.22) * level)
            if hats16:
                hat(b, bt + .25, 0.1 * level)
        if shaker16:
            for i in range(16):
                shk(b, i * .25, (0.22 if i % 4 == 2 else 0.12) * level)
        if last:
            for bt, v in ((3, 0.35), (3.25, 0.45), (3.5, 0.55), (3.75, 0.68)):
                snare(b, bt, v * level)


def toms(bar, hits, vel=0.6):
    for bt, md in hits:
        if active(bar, bt):
            put('timp', O.seat(S.tom(m(md), vel).astype(np.float32) * 0.6, 0.2 if m(md) < 48 else -0.2, 0.5),
                T(bar, bt))


def timp(bar, beat, spec, vel=0.6):
    if active(bar, beat):
        put('timp', O.seat(O.timpani(m(spec), vel), -0.08, 0.6), T(bar, beat) + RNG.normal(0, 0.003))


ROLL_PITCH = {1: 41.9, 2: 47.2, 3: 49.0, 4: 52.3, 5: 55.0}


def timp_roll(bar, beat, beats, spec, v0=0.05, v1=0.8):
    if not active(bar, beat):
        return
    md = m(spec)
    drum = min(ROLL_PITCH, key=lambda k: abs(ROLL_PITCH[k] - md))
    f = f'Percussion/Timpani/Rolls/Timpani{drum}_Roll_v5_rr1_Sum.wav'
    if drum == 5:
        f = 'Percussion/Timpani/Rolls/Timpani5_Roll_v4_rr1_Sum.wav'
    sig = O.perc_file(f, md - ROLL_PITCH[drum], maxlen=B(beats) + 0.3).copy()
    u = np.linspace(0, 1, sig.shape[1], dtype=np.float32)
    sig *= (v0 + (v1 - v0) * u ** 2) * 0.9
    put('timp', O.seat(sig, -0.08, 0.6), T(bar, beat))


def cym_swell(bar, beat=0.0, which='Long', vel=0.6):
    """Suspended-cymbal crescendo whose peak lands on (bar, beat)."""
    sig = O.perc_file(f'Percussion/susCymb1-cresc-{which}_v1.wav')
    pk = int(np.argmax(np.abs(sig).max(0)))
    t = T(bar, beat) - pk / SR
    if active(bar, beat) and t >= 0:
        put('cym', O.seat(sig[:, :pk + int(0.3 * SR)], 0.25, 1.0) * 0.6, t, vel)


def cym_hit(bar, beat=0.0, vel=0.5):
    if active(bar, beat):
        put('cym', O.seat(O.perc('sus_hit', vel), -0.2, 0.9), T(bar, beat))


def rev_cym(bar, beat=0.0, beats=4, vel=0.4):
    if not active(bar, beat):
        return
    c = O.perc('crash', 0.8)
    L = min(int(B(beats) * SR), c.shape[1])
    r = c[:, :L][:, ::-1].copy()
    r *= np.linspace(0, 1, L, dtype=np.float32) ** 2
    put('fx', r, T(bar, beat) - L / SR, vel)


# ---------------------------------------------------------- arrangement

# ===== INTRO (bars 0-7): deep space; the distant horn quotes F1
HUM[:] = [0.012, 0.06]
drone(0, 8, 0.25)
vox_pads(0, 8, vel=0.4, vowel='oo', attack=2.5)
synth_pads(2, 8, vel=0.22, cut=900)
play('vln1', [(0, 'C6', 16), (16, 'C6', 16)], 0, vel=0.12, attack=3.0, legato=False)
play('cb', [(0, 'C2', 32)], 0, vel=0.2, attack=2.0, legato=False)
arp(2, 8, vel=0.32, ramp=(0.2, 0.6))
pulse(4, 8, vel=0.32, div=2, cut=260, env=700)
play('horn', F1, 4, vel=0.32, attack=0.15, echo=0.6)
play('vc', [(0, 'C3+G3', 16)], 4, vel=0.18, attack=2.0, legato=False)
play('bsn', RESP[3], 7, vel=0.45)
timp_roll(7, 0, 4, 'C3', 0.02, 0.45)
rev_cym(8, 0, 4, 0.35)
kick(7, 3.5, 0.45)

# ===== A1 (bars 8-23): theme A on celli and violas, the pulse and break enter
HUM[:] = [0.010, 0.06]
a0 = S_A1
cym_hit(a0, 0, 0.35)
play('vc', THEME_A, a0, vel=0.62, attack=0.08, double=True)
play('vla', THEME_A, a0, vel=0.55, attack=0.08)
for rb, mel in RESP.items():
    play('bsn', mel, a0 + rb, vel=0.55)
    play('cl', mel, a0 + rb, vel=0.3, transpose=12)
pads('vln2', a0, a0 + 16, vel=0.26, pick=[2, 3], transpose=12, attack=0.8)
pads('horn2', a0 + 8, a0 + 16, vel=0.3, pick=[0, 1], attack=0.6)
roots('cb', a0, a0 + 16, vel=0.4, octave=-12)
vox_pads(a0, a0 + 16, vel=0.3, vowel='oo')
synth_pads(a0, a0 + 16, vel=0.2, cut=1100)
arp(a0, a0 + 16, vel=0.32, bright=0.6)
pulse(a0, a0 + 16, vel=0.45, div=2, cut=320, env=1100)
groove(a0, a0 + 8, level=0.8, ghosts=False)
groove(a0 + 8, a0 + 16, level=0.9, ghosts=True, shaker16=True, fill_last=True)
timp(a0 + 14, 0, 'G2', 0.45)
cym_swell(S_B1, 0, 'Short', 0.4)

# ===== B1 (bars 24-33): theme B on violins and choir (wonder)
HUM[:] = [0.010, 0.06]
b0 = S_B1
cym_hit(b0, 0, 0.45)
play('vln1', THEME_B, b0, vel=0.62, transpose=12, attack=0.15, double=True)
play('vln2', THEME_B, b0, vel=0.5, attack=0.15)
vox_line(THEME_B, b0, vel=0.32, vowel='oh')
pads('horn', b0, b0 + 10, vel=0.36, pick=[1, 2], attack=0.5)
pads('vla', b0, b0 + 10, vel=0.3, pick=[0, 1], attack=0.6)
roots('vc', b0, b0 + 10, vel=0.45, attack=0.4)
roots('cb', b0, b0 + 10, vel=0.45, octave=-12)
roots('tbn', b0 + 4, b0 + 10, vel=0.3, attack=0.5)
vox_pads(b0, b0 + 10, vel=0.3, vowel='oo')
synth_pads(b0, b0 + 10, vel=0.22, cut=1400)
arp(b0, b0 + 10, vel=0.34, bright=0.7)
pulse(b0, b0 + 10, vel=0.48, div=2, cut=340, env=1300)
groove(b0, b0 + 10, level=0.95, ghosts=True, shaker16=True, fill_last=False)
toms(b0 + 9, [(2.5, 'C3'), (3, 'G2'), (3.5, 'Eb2')], 0.5)
timp(b0 + 8, 0, 'G2', 0.55)

# ===== TRANSITION (bars 34-35): the band thins out
t0 = S_TR
pads('vla', t0, t0 + 2, vel=0.32, pick=[0, 1, 2], attack=0.3, swell=1.0)
play('vc', [(0, 'C3', 8)], t0, vel=0.4, legato=False)
play('horn', [(0, 'C4+G4', 8)], t0, vel=0.32, attack=0.3, legato=False)
vox_pads(t0, t0 + 2, vel=0.3, vowel='oo')
arp(t0, t0 + 2, vel=0.3, bright=0.6)
pulse(t0, t0 + 2, vel=0.42, div=2, cut=300, env=1000)
groove(t0, t0 + 1, level=0.7, ghosts=False)
kick(t0 + 1, 0, 0.55)
play('bsn', RESP[3], t0 + 1, vel=0.55)
drone(t0 + 1, t0 + 2, 0.3)

# ===== BREAKDOWN (bars 36-43): the solo clarinet quotes F1, the oboe answers
HUM[:] = [0.012, 0.07]
k0 = S_BRK
drone(k0, k0 + 7, 0.24)
play('cl', F1, k0, vel=0.62, attack=0.05)
play('ob', [(0, 'F4', 3), (3, 'Ab4', 1), (4, 'G4', 2), (6, 'Eb4', 2), (8, 'C4', 4)], k0 + 4, vel=0.6)
play('vc', [(0, 'C3', 16), (16, 'F2', 8), (24, 'C3', 4)], k0, vel=0.2, art='trem', attack=1.0, legato=False)
play('cb', [(0, 'C2', 16), (16, 'F1', 8), (24, 'C2', 4)], k0, vel=0.28, attack=1.0, legato=False)
play('vln1', [(0, 'C6', 16), (16, 'C6', 12)], k0, vel=0.16, art='trem', attack=2.0, legato=False)
pads('vla', k0, k0 + 7, vel=0.22, pick=[1, 2], attack=1.0)
vox_pads(k0, k0 + 7, vel=0.4, vowel='oo', attack=1.8)
synth_pads(k0, k0 + 7, vel=0.2, cut=800)
arp(k0, k0 + 7, vel=0.3, bright=0.4)
pulse(k0 + 4, k0 + 7, vel=0.3, div=2, cut=250, env=600)
play('bsn', RESP[3], k0 + 3, vel=0.45)
# bar 43, G: the build back in (no snare roll: kicks, a timpani roll, a swell)
g = k0 + 7
pads('horn', g, g + 1, vel=0.5, pick=[0, 1, 2], attack=0.3, swell=0.3)
pads('vla', g, g + 1, vel=0.45, pick=[1, 2, 3], attack=0.3, swell=0.3)
play('vln1', [(0, 'G4+D5', 4)], g, vel=0.5, swell=0.25, legato=False)
play('vc', [(0, 'G2', 4)], g, vel=0.55, swell=0.3, legato=False)
play('cb', [(0, 'G1', 4)], g, vel=0.55, legato=False)
play('tbn', [(1, 'G2', 1), (2, 'C3', 1), (3, 'D3', 1)], g, vel=0.5)
vox_pads(g, g + 1, vel=0.45, vowel='ah', attack=1.0)
arp(g, g + 1, vel=0.36, bright=0.7)
pulse(g, g + 1, vel=0.45, div=4, cut=330, env=1300)
timp_roll(g, 0, 4, 'G2', 0.05, 0.75)
for bt in (0, 1, 2, 3):
    kick(g, bt, 0.45 + 0.1 * bt)
snare(g, 3.5, 0.5)
snare(g, 3.75, 0.62)
cym_swell(S_A2, 0, 'Median', 0.55)

# ===== A2 (bars 44-59): theme A tutti (horns, violins 8va, celli 8vb)
HUM[:] = [0.008, 0.05]
a0 = S_A2
cym_hit(a0, 0, 0.6)
cym_hit(a0 + 8, 0, 0.5)
play('horn', THEME_A, a0, vel=0.72, attack=0.06, double=True)
play('vln1', THEME_A, a0, vel=0.66, transpose=12, attack=0.08, double=True)
play('vc', THEME_A, a0, vel=0.6, transpose=-12, attack=0.08, double=True)
play('fl', THEME_A[10:15], a0, vel=0.35, transpose=12)
for rb, mel in RESP.items():
    play('tbn', mel, a0 + rb, vel=0.6)
    play('bsn', mel, a0 + rb, vel=0.5)
pads('vla', a0, a0 + 16, vel=0.4, pick=[0, 1, 2], attack=0.4)
pads('horn2', a0, a0 + 16, vel=0.32, pick=[0, 1], attack=0.5)
ostinato_8ths('vln2', a0, a0 + 16, vel=0.34, transpose=12)
roots('cb', a0, a0 + 16, vel=0.55, octave=-12)
roots('tbn', a0 + 8, a0 + 16, vel=0.3, attack=0.5)
vox_pads(a0, a0 + 16, vel=0.38, vowel='ah')
synth_pads(a0, a0 + 16, vel=0.22, cut=1600)
arp(a0, a0 + 16, vel=0.34, bright=0.8)
pulse(a0, a0 + 16, vel=0.5, div=4, cut=360, env=1500)
groove(a0, a0 + 8, level=1.0, ghosts=True, shaker16=True, fill_last=True)
groove(a0 + 8, a0 + 16, level=1.0, ghosts=True, shaker16=True, hats16=True, fill_last=True)
for rb, sp in ((0, 'C3'), (4, 'F2'), (8, 'Ab2'), (12, 'F2'), (14, 'G2')):
    timp(a0 + rb, 0, sp, 0.6)
timp_roll(a0 + 15, 0, 4, 'G2', 0.05, 0.6)
cym_swell(S_B2, 0, 'Short', 0.5)

# ===== B2 (bars 60-69): theme B tutti, the high point (wonder, not bombast)
HUM[:] = [0.008, 0.05]
b0 = S_B2
cym_hit(b0, 0, 0.65)
cym_hit(b0 + 4, 0, 0.45)
play('vln1', THEME_B, b0, vel=0.74, transpose=12, attack=0.1, double=True)
play('vln2', THEME_B, b0, vel=0.62, attack=0.1, double=True)
play('horn', THEME_B, b0, vel=0.66, attack=0.1, double=True)
play('fl', THEME_B, b0, vel=0.32, transpose=12)
vox_line(THEME_B, b0, vel=0.38, vowel='ah')
vox_line(THEME_B, b0, vel=0.2, transpose=12, vowel='oo')
pads('vla', b0, b0 + 10, vel=0.4, pick=[0, 1, 2], attack=0.4)
pads('horn2', b0, b0 + 10, vel=0.35, pick=[0, 1], attack=0.5)
roots('vc', b0, b0 + 10, vel=0.58, attack=0.3)
roots('cb', b0, b0 + 10, vel=0.58, octave=-12)
roots('tbn', b0, b0 + 10, vel=0.4, attack=0.4)
for rb in (1, 5):     # the F3 response answers inside theme B too
    play('tbn', [(1, 'C3', 1), (2, 'F2', 1), (3, 'G2', 1)], b0 + rb, vel=0.5)
vox_pads(b0, b0 + 10, vel=0.38, vowel='oo')
synth_pads(b0, b0 + 10, vel=0.24, cut=1800)
arp(b0, b0 + 10, vel=0.36, bright=0.9, pat=(0, 1, 2, 3, 3, 2, 1, 0))
pulse(b0, b0 + 10, vel=0.52, div=4, cut=380, env=1600)
groove(b0, b0 + 8, level=1.0, ghosts=True, shaker16=True, hats16=True, fill_last=True)
groove(b0 + 8, b0 + 10, level=1.0, ghosts=True, shaker16=True, fill_last=False)
for rb, sp in ((0, 'Ab2'), (2, 'Gb2'), (4, 'Ab2'), (6, 'Gb2'), (7, 'Db3'), (8, 'G2')):
    timp(b0 + rb, 0, sp, 0.62)
timp_roll(b0 + 9, 0, 4, 'G2', 0.1, 0.7)
toms(b0 + 9, [(2, 'C3'), (2.5, 'C3'), (3, 'G2'), (3.5, 'Eb2')], 0.55)
cym_swell(S_OUT, 0, 'Short', 0.45)

# ===== OUTRO (bars 70-73): F1 once more, back home to C minor (loops to bar 0)
HUM[:] = [0.010, 0.06]
o0 = S_OUT
cym_hit(o0, 0, 0.5)
timp(o0, 0, 'C3', 0.55)
play('horn', F1[:4] + [(8, 'G3', 4)], o0, vel=0.5, attack=0.08, double=True, echo=0.35)
play('vc', F1[:4] + [(8, 'G3', 4)], o0, vel=0.45, attack=0.08)
pads('vla', o0, o0 + 3, vel=0.32, pick=[0, 1, 2], attack=0.5)
pads('vln2', o0, o0 + 3, vel=0.22, pick=[2, 3], transpose=12, attack=0.8)
roots('cb', o0, o0 + 3, vel=0.45, octave=-12)
vox_pads(o0, o0 + 3, vel=0.34, vowel='oo')
synth_pads(o0, o0 + 3, vel=0.2, cut=1100)
arp(o0, o0 + 3, vel=0.32, bright=0.6)
pulse(o0, o0 + 3, vel=0.42, div=2, cut=300, env=1000)
groove(o0, o0 + 2, level=0.75, ghosts=False)
for i in range(8):
    hat(o0 + 2, i * .5, 0.2 if i % 2 else 0.14)
kick(o0 + 2, 0, 0.55)
drone(o0 + 2, o0 + 4, 0.3)
# bar 73: the final C minor chord, rung out
f = o0 + 3
FIN = 4.0
play('vln1', [(0, 'G4+C5+Eb5', FIN)], f, vel=0.4, legato=False, rel=0.9)
play('vla', [(0, 'G3+C4', FIN)], f, vel=0.42, legato=False, rel=0.9)
play('vc', [(0, 'C3+G3', FIN)], f, vel=0.45, legato=False, rel=0.9)
play('cb', [(0, 'C2', FIN)], f, vel=0.5, legato=False, rel=0.9)
play('horn', [(0, 'C4+Eb4+G4', FIN)], f, vel=0.42, legato=False, rel=0.9, double=True)
play('tbn', [(0, 'C3', FIN)], f, vel=0.32, legato=False, rel=0.9)
vox_pads(f, f + 1, vel=0.38, vowel='oo', attack=0.4)
put('pulse', O.pulse(48, B(1.5), 1.0, cut=300, env=900, decay=0.12), T(f), 0.5) if active(f) else None
timp(f, 0, 'C2', 0.5)
kick(f, 0, 0.5)
cym_hit(f, 0, 0.4)

# ------------------------------------------------------------- mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


GAIN = {  # dB
    'vln': 1.0, 'lowstr': 1.0, 'horn': 1.0, 'lowbr': -2.0, 'ww': -3.0, 'choir': -17.0,
    'pad': -7.0, 'arp': -4.0, 'pulse': -1.5, 'kick': -6.0, 'snare': 1.5, 'hats': -1.0,
    'timp': 0.0, 'cym': -6.0, 'echo': -10.0,
}
VERB = {'vln': .45, 'lowstr': .3, 'horn': .5, 'lowbr': .4, 'ww': .55, 'choir': .7, 'pad': .45,
        'arp': .4, 'pulse': .05, 'snare': .18, 'hats': .1, 'timp': .35, 'cym': .3, 'echo': .8}
ROOM = {'kick': .08, 'snare': .5, 'hats': .3}
SIDECHAIN = {'pulse': 0.45, 'arp': 0.25, 'pad': 0.3, 'choir': 0.12}

bus['vln'].x = stage(bus['vln'].x, HighpassFilter(140), PeakFilter(3200, -2.0, 1.0), HighShelfFilter(9000, -1.5, 0.7))
bus['lowstr'].x = stage(bus['lowstr'].x, HighpassFilter(32))
bus['horn'].x = stage(bus['horn'].x, HighpassFilter(70), LowShelfFilter(200, 1.0, 0.7))
bus['lowbr'].x = stage(bus['lowbr'].x, HighpassFilter(45))
bus['ww'].x = stage(bus['ww'].x, HighpassFilter(150))
bus['choir'].x = stage(bus['choir'].x, HighpassFilter(160), LowpassFilter(6500))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(150), LowpassFilter(5000))
bus['arp'].x = stage(bus['arp'].x, HighpassFilter(200), LowpassFilter(6500))
bus['arp'].x += S.pingpong(bus['arp'].x, 0.75 * SPB, feedback=0.42, repeats=7, lp=4200, hp=400).astype(np.float32) * 0.55
bus['pulse'].x = stage(bus['pulse'].x, HighpassFilter(38), LowpassFilter(3500))
bus['kick'].x = stage(bus['kick'].x, HighpassFilter(30), LowpassFilter(7000))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(120), LowpassFilter(8000),
                       Compressor(threshold_db=-18, ratio=2.0, attack_ms=5, release_ms=80))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(2000), LowpassFilter(11000))
bus['timp'].x = stage(bus['timp'].x, HighpassFilter(35))
bus['cym'].x = stage(bus['cym'].x, HighpassFilter(300), LowpassFilter(12000))
bus['echo'].x = S.pingpong(stage(bus['echo'].x, HighpassFilter(250)), 0.75 * SPB, feedback=0.5, repeats=8,
                           lp=3500, hp=300).astype(np.float32)

KICKS.sort()

mix = np.zeros((2, N), np.float32)
TONAL_OUT = os.environ.get('NEW07_TONAL')        # also write a dry, drums-muted mix (chroma check)
DRUMS = {'kick', 'snare', 'hats', 'timp', 'cym', 'echo'}
tonal = np.zeros((2, N), np.float32) if TONAL_OUT else None
verb_send = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('NEW07_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in BUSES:
    x = bus[k].x
    del bus[k]
    if k in MUTE:
        continue
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in SIDECHAIN:
        x *= S.sidechain(N, KICKS, depth=SIDECHAIN[k], release=0.28, attack=0.004)[None, :]
    nz = np.nonzero(np.max(np.abs(x), 0) > 1e-4)[0]
    if len(nz) > SR:
        seg = x[:, nz[0]:nz[-1]].T.astype(np.float64)
        levels[k] = meter.integrated_loudness(seg)
    mix += x
    if tonal is not None and k not in DRUMS:
        tonal += x
    if k in VERB:
        verb_send += x * np.float32(VERB[k])
    if k in ROOM:
        room_send += x * np.float32(ROOM[k])
    del x

verb = O.convolve(stage(verb_send, HighpassFilter(200), LowpassFilter(7500)), X.space_ir())
del verb_send
if tonal is not None:
    sf_.write(TONAL_OUT, (tonal / (np.abs(tonal).max() + 1e-9) * 0.5).T, SR, subtype='FLOAT')
    del tonal
mix += verb * np.float32(0.6)
del verb
room = O.convolve(stage(room_send, HighpassFilter(250), LowpassFilter(7000)), X.room_ir())
del room_send
mix += room * np.float32(0.35)
del room

# section dynamics (dB at bar positions)
SECTION_DB = [(0, -2.0), (7.9, -1.0), (8, -1.0), (23.9, -0.5), (24, -0.5), (33.9, -0.5), (34, -0.5),
              (35.9, -1.5), (36, -2.0), (42.9, -1.5), (43.9, -0.5), (44, 0.0), (59.9, 0.0), (60, 0.5),
              (69.9, 0.5), (70, -0.5), (NBARS + 5, -0.5)]
bars_axis = np.arange(N) / SR / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del bars_axis, sec

# ---------------------------------------------------------- master
side = (mix[0] - mix[1]) * 0.5
side -= ss.sosfiltfilt(ss.butter(4, 120, 'low', fs=SR, output='sos'), side).astype(np.float32)
mid = (mix[0] + mix[1]) * 0.5
mix = np.stack([mid + side, mid - side]).astype(np.float32)
del side, mid
mix = stage(mix, HighpassFilter(26))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-15.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(100, 1.0, 0.7), PeakFilter(450, -1.5, 0.7), HighShelfFilter(7000, 2.0, 0.7),
            Compressor(threshold_db=-14, ratio=1.7, attack_ms=40, release_ms=250))
mix = np.tanh(mix * 1.05) / 1.05
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=150))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
end = min(last + 1, int((T(NBARS) + 3.0) * SR))
mix = mix[:, :end]
fade = int(0.8 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new07_cinematic.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
