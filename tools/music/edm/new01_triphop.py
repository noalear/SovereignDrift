#!/usr/bin/env python3
# new01_triphop.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Graveyard Orbit": a NEW trip-hop / downtempo-breakbeat piece (late-90s Bristol
# sound: dusty swung breaks, dub sub bass, Rhodes, scratchy strings, a theremin
# lead, a harmon-muted trumpet and a spacious dub delay), 88 BPM, C minor.
#
# Credits: the source material is Stardock's (the original game's music,
# mid/sf-music1.mid, 1997); this is a new composition developed from fragments of
# it that the existing remix (song1_ambient.py) does not use or barely uses.
# Produced entirely in code with tools/music/edm/synth.py and synth_triphop.py.
# Samples: VSCO-2 Community Edition by Versilian Studios, CC0 1.0 (public domain),
# https://github.com/sgossner/VSCO-2-CE -- harmon-muted trumpet (sustain), violin,
# viola and cello sections (sustain vibrato).
#
# Built from these fragments of sf-music1.mid (bar numbers are the original's,
# 1-based, at 140 BPM):
#   F1  the trombone call (ch 6/7, bars 12-13 and 18-19): the two-voice swell
#       Eb4+G4 -> F4+Ab4 and the pickup figure C4 . C4 C4 | C4 (beats 3, 4, 4+ | 1),
#       not used in song 1's remix.  Here it becomes the main theme: the theremin
#       melody (theme A) opens with the pickup rhythm and climbs G -> Ab over the
#       i - bII sway; the muted trumpet plays the call itself with dub echoes; the
#       strings play the swell (intro, theme A, peak, outro); the trumpet's
#       breakdown solo develops it in inversion (Ab -> G) over new chords.
#   F2  the string inner voices of bars 21-28 (ch 4/6/9 overflow): F5 and Db5 held
#       over Db, then Eb5 and C5 re-attacked on syncopated off-beats over Cm
#       (C5 on beats 1, 4, 4+4...; Eb5 on 1, 3, 3+4...).  The syncopation becomes
#       the Rhodes "push" comping pattern of the whole track (two interlocking
#       voices); the parallel descent F->Eb / Db->C becomes theme B (theremin) and
#       the string lines of sections B and peak B, over a new progression
#       Bbm9 - Db - Ab - Fm - G7(b9).
#   F3  the taiko ostinato of bars 1-4 (ch 8): its two-bar figure, with each
#       8th of the original turned into a 16th, is the breakbeat's kick/snare/ghost
#       skeleton (K . K g | S . . g | . . K . | S . g K).
#   F4  the tuba's re-attack figure of bars 57-72 (6 beats held, 2-beat
#       re-attack) as the dub bass of the breakdown; and the original's i - bII
#       (C / Db) sway of bars 1-40 as the home progression, now two bars per chord.
# New: tempo, groove, form (intro - groove - theme A - theme B - breakdown -
# peak A - peak B - outro - final hit), the chords of the B section and the
# breakdown (Ab - Bbm - Db - Gsus - G7), all melodic development and sound
# design.  The track ends on a C minor hit that rings out and loops to the intro.
#
#   python3 tools/music/edm/new01_triphop.py OUT.wav
#   NEW01_MUTE=kick,snare,hats,perc,drumlo,vinyl   (mute buses, for chroma checks)
import os
import sys

import numpy as np
import pyloudnorm as pyln
import soundfile as sf_
from pedalboard import (Pedalboard, Compressor, Limiter, HighpassFilter, LowpassFilter,
                        LowShelfFilter, HighShelfFilter, PeakFilter, Chorus)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_ambient as A  # noqa: E402
import synth_triphop as TH  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 88.0
SPB = 60.0 / BPM
NBARS = 61
TAIL = 4.5
N = int((NBARS * 4 * SPB + TAIL) * SR)
SWING = 0.60          # 16th swing (MPC-style)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def sw(beat):
    """Swing the odd 16ths."""
    q = beat * 4
    if abs(q - round(q)) < 1e-6 and int(round(q)) % 2 == 1:
        return beat + (SWING - 0.5) * 0.5
    return beat


# ------------------------------------------------------------- harmony

PA = ['Cm', 'Cm', 'Db', 'Db']
PB = ['Bbm', 'Bbm', 'Db', 'Db', 'Ab', 'Fm', 'G7', 'G7']
PBD = ['Ab', 'Ab', 'Bbm', 'Bbm', 'Db', 'Db', 'Gsus', 'G7']
SECTIONS = [  # name, first bar, bars
    ('intro', 0, 4), ('groove', 4, 8), ('themeA', 12, 8), ('themeB', 20, 8),
    ('breakdown', 28, 8), ('peakA', 36, 8), ('peakB', 44, 8), ('outro', 52, 8), ('end', 60, 1)]
CHORDS = PA + PA * 2 + PA * 2 + PB + PBD + PA * 2 + PB + PA * 2 + ['Cm']
assert len(CHORDS) == NBARS

# pitch classes for the chroma check (incl. the bass root)
CHORD_PC = {'Cm': 'C Eb G Bb D', 'Db': 'Db F Ab C Eb', 'Bbm': 'Bb Db F Ab C', 'Ab': 'Ab C Eb G',
            'Fm': 'F Ab C Eb G', 'G7': 'G B D F Ab', 'Gsus': 'G C D F'}
ROOT = {'Cm': 'C2', 'Db': 'Db2', 'Bbm': 'Bb1', 'Ab': 'Ab1', 'Fm': 'F1', 'G7': 'G1', 'Gsus': 'G1'}
THIRD = {'Cm': 3, 'Db': 4, 'Bbm': 3, 'Ab': 4, 'Fm': 3, 'G7': 4, 'Gsus': 5}
# Rhodes: two interlocking voices (lower pair, upper pair), rootless voicings
RH = {'Cm': (['Eb3', 'Bb3'], ['D4', 'G4']), 'Db': (['F3', 'C4'], ['Eb4', 'Ab4']),
      'Bbm': (['Db3', 'Ab3'], ['C4', 'F4']), 'Ab': (['Eb3', 'G3'], ['C4', 'Ab4']),
      'Fm': (['Ab3', 'C4'], ['Eb4', 'G4']), 'G7': (['F3', 'B3'], ['D4', 'Ab4']),
      'Gsus': (['F3', 'C4'], ['D4', 'G4'])}
# string beds for the breakdown (viola pair + cello root)
SBED = {'Ab': ['C4', 'Eb4', 'Ab2'], 'Bbm': ['Db4', 'F4', 'Bb2'], 'Db': ['F4', 'Ab4', 'Db3'],
        'Gsus': ['C4', 'D4', 'G2'], 'G7': ['B3', 'F4', 'G2'], 'Cm': ['Eb4', 'G4', 'C3']}
KICK_HZ = 49.0     # G1, the fifth of C minor


def runs(bar0, bar1, key=lambda c: c):
    b = bar0
    while b < bar1:
        k = key(CHORDS[b])
        e = b
        while e < bar1 and key(CHORDS[e]) == k:
            e += 1
        yield CHORDS[b], b, e
        b = e


# ------------------------------------------------------------- themes (beats; note; length)

# F1 developed: theremin theme A (8 bars over Cm Cm Db Db x2)
THEME_A = [(2, 'C5', .75), (3, 'C5', .5), (3.5, 'C5', .5),
           (4, 'G5', 3.5), (7.5, 'F5', .25), (7.75, 'Eb5', .25),
           (8, 'F5', 2.5), (10.5, 'Ab5', 1.5),
           (12, 'Ab5', 2), (14, 'G5', 1), (15, 'F5', 1),
           (16, 'Eb5', 2), (18, 'C5', .75), (19, 'C5', .5), (19.5, 'Eb5', .5),
           (20, 'G5', 2), (22, 'Bb5', 1), (23, 'Ab5', 1),
           (24, 'Ab5', 3), (27, 'F5', 1),
           (28, 'Eb5', 1.5), (29.5, 'Db5', .5), (30, 'C5', 2)]
# F2 developed: theremin theme B (Bbm Bbm Db Db Ab Fm G7 G7)
THEME_B = [(0, 'F5', 3), (3, 'Db5', .5), (3.5, 'Eb5', .5),
           (4, 'F5', 3), (7, 'Ab5', 1),
           (8, 'F5', 4),
           (12, 'Eb5', 2), (14, 'Db5', 1), (15, 'C5', 1),
           (16, 'Eb5', 3), (19, 'C5', 1),
           (20, 'C5', 2), (22, 'Ab4', 1), (23, 'C5', 1),
           (24, 'B4', 4),
           (28, 'D5', 1.5), (29.5, 'F5', .5), (30, 'Ab5', 1.5), (31.5, 'G5', .5)]
# F1: the trombone call on the muted trumpet
CALL = [(2, 'C5', .75), (3, 'C5', .45), (3.5, 'C5', .45), (4, 'G4', 3)]
CALL_INTRO = [(6, 'C5', .75), (7, 'C5', .45), (7.5, 'C5', .45), (8, 'Eb5', 4), (12, 'F5', 3.5)]
# breakdown solo: F1 in inversion over Ab Ab Bbm Bbm Db Db Gsus G7
TPT_SOLO = [(2, 'Eb5', .75), (3, 'Eb5', .45), (3.5, 'Eb5', .45),
            (4, 'C5', 3), (7, 'Bb4', .5), (7.5, 'Ab4', .5),
            (8, 'Bb4', 2), (10, 'Db5', 2),
            (12, 'F5', 3), (15, 'Eb5', 1),
            (18, 'F5', .75), (19, 'F5', .45), (19.5, 'F5', .45),
            (20, 'Eb5', 2), (22, 'Db5', 2),
            (24, 'C5', 2), (26, 'D5', 2),
            (28, 'B4', 2), (30, 'D5', 1), (31, 'F5', 1)]
# F2: string inner voices (two voices) for the B progression; the syncopated
# re-attacks of the original bars 25-28 over Ab / Fm
STR_B_HI = [(0, 'F5', 8), (8, 'F5', 8), (16, 'Eb5', 2), (18, 'Eb5', 4), (22, 'Eb5', 2),
            (24, 'D5', 4), (28, 'F5', 4)]
STR_B_LO = [(0, 'Db5', 8), (8, 'Db5', 8), (16, 'C5', 3), (19, 'C5', 4), (23, 'C5', 1),
            (24, 'B4', 8)]
# F1: the trombone swell on strings, over Cm -> Db
SWELL = [((0, 8), ['Eb4', 'G4']), ((8, 8), ['F4', 'Ab4'])]

# ------------------------------------------------------------- instruments

tpt_s = A.Sampler('Brass/Trumpet/harmonM-sus/*.wav', offset=12)
vln_s = A.Sampler('Strings/Violin Section/susVib/*.wav', offset=12)
vla_s = A.Sampler('Strings/Viola Section/susvib/*.wav', offset=12)
vc_s = A.Sampler('Strings/Cello Section/susvib/*.wav', offset=12)

names = ['kick', 'snare', 'hats', 'perc', 'drumlo', 'bass', 'rhodes', 'strings', 'lead', 'tpt',
         'pad', 'vinyl']
MUTE = set(filter(None, os.environ.get('NEW01_MUTE', '').split(',')))
bus = {k: S.Bus(N) for k in names}
throw = S.Bus(N)            # dub-delay throws (snare / rim hits sent only to the delay)
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def jit(ms=4.0):
    return (S.rng().random() - 0.5) * 2 * ms / 1000.0


def hv(v, amt=0.08):
    return v * (1 + (S.rng().random() - 0.5) * 2 * amt)


# ---- drums.  16th grid positions per bar; derived from the taiko ostinato (F3)
BREAK = [
    {'k': [(0, 1.0), (2, .8), (10, .9), (15, .55)], 's': [4, 12], 'g': [(7, .22), (9, .16), (14, .2)]},
    {'k': [(0, 1.0), (6, .75), (10, .85)], 's': [4, 12], 'g': [(3, .18), (9, .2), (13, .16), (15, .25)]},
]
FILL = {'k': [(0, 1.0), (2, .8), (10, .9)], 's': [4, 12, 15], 'g': [(7, .2), (11, .25), (13, .3), (14, .4)]}
_snr = [0]


def drum_bar(bar, pat, target='main', hat_mode='8', open_at=None, vel=1.0, shaker=False):
    kb = 'kick' if target == 'main' else 'drumlo'
    sb = 'snare' if target == 'main' else 'drumlo'
    hb = 'hats' if target == 'main' else 'drumlo'
    for pos, v in pat['k']:
        bt = sw(pos / 4)
        var = int(S.rng().integers(0, 3))
        sig = cached(('k', var), lambda: TH.kick(1.0, f_end=KICK_HZ, var=var))
        bus[kb].add(sig, T(bar, bt) + jit(3), hv(v) * vel)
    for pos in pat['s']:
        bt = sw(pos / 4)
        _snr[0] += 1
        var = _snr[0] % 4
        sig = cached(('s', var), lambda: TH.snare(1.0, var=var))
        v = 1.0 if pos in (4, 12) else 0.55
        bus[sb].add(sig, T(bar, bt) + 0.010 + jit(4), hv(v, 0.06) * vel)
    for pos, v in pat['g']:
        bt = sw(pos / 4)
        var = int(S.rng().integers(0, 4))
        sig = cached(('g', var), lambda: TH.snare(1.0, var=var, ghost=True))
        bus[sb].add(sig, T(bar, bt) + 0.012 + jit(6), hv(v, 0.2) * vel)
    if hat_mode:
        step = 2 if hat_mode == '8' else 1
        for pos in range(0, 16, step):
            if open_at is not None and pos == open_at:
                sig = cached(('ho',), lambda: TH.hat(1.0, open_=True))
                bus[hb].add(sig, T(bar, sw(pos / 4)) + jit(3), 0.55 * vel)
                continue
            if open_at is not None and open_at < pos < open_at + 2:
                continue
            acc = 1.0 if pos % 4 == 0 else (0.7 if pos % 2 == 0 else 0.4)
            var = int(S.rng().integers(0, 4))
            sig = cached(('h', var), lambda: TH.hat(1.0, var=var))
            bus[hb].add(sig, T(bar, sw(pos / 4)) + jit(4), hv(acc, 0.15) * 0.8 * vel)
    if shaker:
        for pos in range(16):
            acc = 0.9 if pos % 4 == 2 else (0.5 if pos % 2 == 0 else 0.35)
            var = int(S.rng().integers(0, 4))
            sig = cached(('sh', var), lambda: TH.shaker(1.0, var=var))
            bus['perc'].add(sig, T(bar, sw(pos / 4)) + jit(5), hv(acc, 0.15) * vel)


def drums(bar0, bar1, target='main', vel=1.0, shaker=False, fills=True):
    for b in range(bar0, bar1):
        k = b - bar0
        last = fills and (k % 8 == 7 or b == bar1 - 1)
        pat = FILL if last else BREAK[k % 2]
        open_at = 14 if (k % 4 == 1) else None
        drum_bar(b, pat, target, open_at=open_at, vel=vel, shaker=shaker)


def snare_throw(bar, beat=3.0, vel=0.9):
    """A backbeat snare sent to the dub delay only."""
    throw.add(cached(('s', 0), lambda: TH.snare(1.0, var=0)), T(bar, beat) + 0.01, vel)


def rims(bar0, bar1, positions=(6, 11), vel=0.6):
    for b in range(bar0, bar1):
        for p in positions:
            sig = cached(('rim',), lambda: TH.rim(1.0))
            bus['perc'].add(sig, T(b, sw(p / 4)) + jit(4), hv(vel))
            throw.add(sig, T(b, sw(p / 4)), vel * 0.8)


def kick_hit(bar, beat=0.0, vel=1.0):
    bus['kick'].add(cached(('k', 0), lambda: TH.kick(1.0, f_end=KICK_HZ, var=0)), T(bar, beat), vel)


# ---- bass

def bass(bar0, bar1, vel=1.0, style='dub'):
    if style == 'tuba':   # F4: held 6 beats, re-attack for 2
        for ch, b, e in runs(bar0, bar1, key=lambda c: ROOT[c]):
            r = m(ROOT[ch])
            for k in range(b, e, 2):
                span = min(2, e - k)
                if span == 2:
                    bus['bass'].add(TH.dub_bass(r, B(5.7), 1.0), T(k), vel)
                    bus['bass'].add(TH.dub_bass(r, B(1.85), 0.9), T(k, 6), vel * 0.85)
                else:
                    bus['bass'].add(TH.dub_bass(r, B(3.8), 1.0), T(k), vel)
        return
    for ch, b, e in runs(bar0, bar1):
        r = m(ROOT[ch])
        third = r + THIRD[ch]
        for k in range(b, e):
            first = (k - b) % 2 == 0
            if first:
                notes = [(0, r, 1.5, 1.0), (2.5, r, .4, .8), (3, r - 5, .85, .9)]
            else:
                low = r - 2 if ch in ('Cm', 'Bbm', 'Fm', 'G7') else r - 5
                notes = [(0, r, 1.5, 1.0), (1.75, r, .2, .6), (2.5, third, .45, .85), (3.25, low, .6, .8)]
            if k == e - 1 and e - b == 1:     # single-bar chords: simpler
                notes = [(0, r, 1.5, 1.0), (2.5, r, .4, .8), (3, r - 5, .85, .9)]
            for bt, md, d, v in notes:
                bus['bass'].add(TH.dub_bass(md, B(d), 1.0), T(k, sw(bt)) + jit(2), v * vel)


# ---- Rhodes: F2 "push" comping (two interlocking voices)

PUSH_LO = [(0, 3), (3, 4), (7, 1)]          # C5-voice of bars 25-28: 1, 4, 4+4 ...
PUSH_HI = [(0, 2), (2, 4), (6, 2)]          # Eb5-voice: 1, 3, 3+4 ...


def rhodes(bar0, bar1, vel=0.7, mode='push'):
    if mode == 'hold':
        for ch, b, e in runs(bar0, bar1):
            for voice in (0, 1):
                for j, n_ in enumerate(RH[ch][voice]):
                    sig = TH.rhodes(m(n_), T(e) - T(b) - 0.08, hv(vel, 0.05), release=0.4,
                                    pan=-0.2 if voice == 0 else 0.2)
                    bus['rhodes'].add(sig, T(b) + 0.01 * (2 * voice + j) + jit(3), 1.0)
        return
    for b in range(bar0, bar1, 2):
        for pat, voice in ((PUSH_LO, 0), (PUSH_HI, 1)):
            for bt, d in pat:
                bar = b + int(bt // 4)
                if bar >= bar1:
                    continue
                ch = CHORDS[bar]
                end_bar = b + int((bt + d - 0.01) // 4)
                if end_bar != bar and end_bar < NBARS and CHORDS[end_bar] != ch:
                    d = (bar - b + 1) * 4 - bt - 0.05     # don't ring over a chord change
                v = hv(vel * (1.0 if bt % 4 == 0 else 0.82), 0.07)
                for j, n_ in enumerate(RH[ch][voice]):
                    sig = TH.rhodes(m(n_), B(d) - 0.04, v, pan=-0.2 if voice == 0 else 0.2)
                    bus['rhodes'].add(sig, T(b, bt) + 0.006 * j + jit(3), 1.0)


def rhodes_chord(bar, notes, dur_beats, vel=0.75, spread=0.012):
    for j, n_ in enumerate(notes):
        bus['rhodes'].add(TH.rhodes(m(n_), B(dur_beats), vel, release=0.6, pan=(j - 2) * 0.12),
                          T(bar) + spread * j, 1.0)


# ---- melodic voices

def lead(bar0, notes, vel=1.0, transpose=0):
    t0 = T(bar0, notes[0][0])
    end = max(b + l for b, n, l in notes)
    total = T(bar0, end) - t0 + 3.0
    ph = [(T(bar0, b) - t0, m(n) + transpose, B(l) - 0.02, hv(1.0, 0.05)) for b, n, l in notes]
    bus['lead'].add(S.pan2(TH.theremin(ph, total, 1.0), 0.05), t0, vel)


def tpt(bar0, notes, vel=0.8, transpose=0, gap=0.04):
    for b, n_, l in notes:
        sig = tpt_s.note(m(n_) + transpose, max(B(l) - gap, 0.1), 0.85, attack=0.02, release=0.18)
        bus['tpt'].add(sig, T(bar0, b) + jit(6), hv(vel, 0.06))


def strings(bar0, notes, vel=0.6, smp=None, attack=0.45, release=1.0, transpose=0):
    for b, n_, l in notes:
        md = m(n_) + transpose
        s_ = smp or (vc_s if md < 55 else (vla_s if md < 67 else vln_s))
        sig = s_.note(md, B(l) - 0.05, 0.8, attack=attack, release=release)
        bus['strings'].add(sig, T(bar0, b) + jit(8), vel)


def swell(bar0, cycles=1, octave=0, vel=0.55, cello=True):
    """F1 trombone swell (Eb+G over Cm -> F+Ab over Db) on strings."""
    for c in range(cycles):
        b0 = bar0 + 4 * c
        for (st, ln), pair in SWELL:
            strings(b0, [(st, p, ln) for p in pair], vel=vel, transpose=12 * octave, attack=1.2, release=1.4)
        if cello:
            strings(b0, [(0, 'C3', 8), (8, 'Db3', 8)], vel=vel * 0.8, smp=vc_s, attack=1.0)


def string_bed(bar0, bar1, vel=0.5):
    for ch, b, e in runs(bar0, bar1):
        notes = SBED[ch]
        strings(b, [(0, n_, (e - b) * 4) for n_ in notes], vel=vel, attack=1.0, release=1.4)


def pad(bar0, bar1, vel=0.5):
    for ch, b, e in runs(bar0, bar1):
        dur = T(e) - T(b)
        lo, hi = RH[ch]
        for n_ in lo + hi:
            bus['pad'].add(A.pad_voice(m(n_) + 12, dur, 1.0, attack=1.5, release=2.0, cut=1600), T(b), vel * 0.3)


# ================================================================ arrangement

# intro (0-3): Rhodes + strings swell + trumpet call + the break through a wall
rhodes(0, 4, vel=0.55)
swell(0, 1, octave=0, vel=0.5)
pad(0, 4, vel=0.6)
tpt(0, CALL_INTRO, vel=0.7)
drums(2, 4, target='lo', fills=False)
bass(0, 4, vel=0.55, style='tuba')

# groove (4-11)
drums(4, 12)
bass(4, 12)
rhodes(4, 12, vel=0.62)
tpt(5, CALL, vel=0.7)
tpt(9, CALL, vel=0.65)
snare_throw(7)
snare_throw(11)

# theme A (12-19)
drums(12, 20)
bass(12, 20)
rhodes(12, 20, vel=0.6)
lead(12, THEME_A, vel=1.0)
swell(12, 2, octave=1, vel=0.32, cello=False)
snare_throw(15)
snare_throw(19, 3.5, 0.7)

# theme B (20-27)
drums(20, 28)
bass(20, 28)
rhodes(20, 28, vel=0.6)
lead(20, THEME_B, vel=1.0)
strings(20, STR_B_HI, vel=0.42)
strings(20, STR_B_LO, vel=0.42)
strings(20, [(0, 'Bb2', 8), (8, 'Db3', 8), (16, 'Ab2', 4), (20, 'F2', 4), (24, 'G2', 8)], vel=0.4, smp=vc_s)
snare_throw(23)

# breakdown (28-35): no kit; rims + dub; strings and Rhodes hold; trumpet solo
rhodes(28, 36, vel=0.5, mode='hold')
string_bed(28, 36, vel=0.5)
pad(28, 36, vel=0.8)
bass(28, 36, vel=0.8, style='tuba')
tpt(28, TPT_SOLO, vel=0.8)
rims(30, 35, positions=(6, 11), vel=0.5)
drum_bar(35, {'k': [], 's': [12], 'g': [(9, .2), (11, .3), (13, .35), (14, .45), (15, .55)]},
         hat_mode=None, vel=0.9)

# peak A (36-43)
drums(36, 44, shaker=True)
bass(36, 44)
rhodes(36, 44, vel=0.66)
lead(36, THEME_A, vel=1.0)
swell(36, 2, octave=1, vel=0.42, cello=True)
strings(36, [(b + 4 * c, n_, l) for c in (0, 1) for b, n_, l in
             [(0, 'G5', 3), (3, 'G5', 4), (7, 'G5', 1), (8, 'Ab5', 3), (11, 'Ab5', 4), (15, 'Ab5', 1)]],
        vel=0.3, attack=0.15, release=0.6)
tpt(37, [(b, n_, l) for b, n_, l in CALL[:3]], vel=0.55, transpose=-12)
snare_throw(39)
snare_throw(43, 3.5, 0.7)

# peak B (44-51)
drums(44, 52, shaker=True)
bass(44, 52)
rhodes(44, 52, vel=0.66)
lead(44, THEME_B, vel=1.0)
tpt(44, THEME_B, vel=0.5, transpose=-12)
strings(44, STR_B_HI, vel=0.5)
strings(44, STR_B_LO, vel=0.5)
strings(44, STR_B_HI, vel=0.3, transpose=12, smp=vln_s)
strings(44, [(0, 'Bb2', 8), (8, 'Db3', 8), (16, 'Ab2', 4), (20, 'F2', 4), (24, 'G2', 8)], vel=0.45, smp=vc_s)
snare_throw(47)

# outro (52-59): groove, theme A's first half, the call; break filtered at the end
drums(52, 58)
drums(58, 60, target='lo', fills=False)
bass(52, 60)
rhodes(52, 60, vel=0.58)
lead(52, THEME_A[:11], vel=0.9)
swell(52, 2, octave=0, vel=0.45)
tpt(57, CALL, vel=0.65)
snare_throw(55)

# end (60): C minor hit, rings out (loops back to the intro's Cm)
kick_hit(60, 0, 1.0)
bus['bass'].add(TH.dub_bass(m('C2'), 2.2, 1.0, release=0.4), T(60), 1.0)
rhodes_chord(60, ['C3', 'G3', 'Eb4', 'Bb4', 'D5'], 4.5, vel=0.7)
strings(60, [(0, 'C3', 4), (0, 'G3', 4), (0, 'Eb4', 4), (0, 'G4', 4)], vel=0.5, attack=0.05, release=0.9)
tpt(60, [(0, 'C5', 3.0)], vel=0.65)

# vinyl surface (very subtle), a little more on the intro/outro
vin = TH.vinyl(N)
vgain = np.interp(np.arange(N) / SR, [0, T(4), T(5), T(56), T(60), N / SR], [1.3, 1.3, 0.8, 0.8, 1.2, 1.0])
bus['vinyl'].x = vin * vgain[None, :].astype(np.float32)
del vin

# ================================================================ mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


# bus processing
bus['drumlo'].x = stage(bus['drumlo'].x, LowpassFilter(520), HighpassFilter(45))
bus['strings'].x = stage(bus['strings'].x, HighpassFilter(220), LowpassFilter(5200),
                         PeakFilter(2400, 1.5, 1.0), PeakFilter(450, -2.0, 0.8))
bus['strings'].x = TH.wow(bus['strings'].x, depth_ms=1.3, rate=0.45)
bus['strings'].x = (np.tanh(bus['strings'].x * 2.0) / 2.0).astype(np.float32)
bus['tpt'].x = stage(bus['tpt'].x, HighpassFilter(300), LowpassFilter(6500), PeakFilter(1600, 2.0, 0.9))
bus['rhodes'].x = TH.autopan(bus['rhodes'].x, rate=2.4, depth=0.3)
bus['rhodes'].x = stage(bus['rhodes'].x, HighpassFilter(90), LowpassFilter(5500),
                        Chorus(rate_hz=0.5, depth=0.15, centre_delay_ms=6, mix=0.25))
bus['rhodes'].x = (np.tanh(bus['rhodes'].x * 1.5) / 1.5).astype(np.float32)
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(200))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(180), LowpassFilter(3000))
bus['bass'].x = stage(bus['bass'].x, LowpassFilter(350), HighpassFilter(30))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(4500), LowpassFilter(11000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(400), LowpassFilter(10000))

# target loudness per bus while playing (LUFS); gains are derived from these
TARGET = {'kick': -19.5, 'snare': -22.5, 'hats': -30.5, 'perc': -32.0, 'drumlo': -24.0,
          'bass': -20.5, 'rhodes': -22.0, 'strings': -24.5, 'lead': -21.5, 'tpt': -22.5,
          'pad': -32.0, 'vinyl': -46.0}
VERB = {'snare': .10, 'rhodes': .22, 'strings': .40, 'lead': .30, 'tpt': .35, 'pad': .4, 'perc': .2,
        'drumlo': .1}
DELAY = {'lead': .22, 'tpt': .45, 'rhodes': .08, 'perc': .0}
meter = pyln.Meter(SR)
levels = {}
gains = {}
mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
for k in names:
    x = bus[k].x
    if not np.any(x):
        continue
    lv = meter.integrated_loudness(x.T.astype(np.float64))
    g = np.float32(10 ** ((TARGET[k] - lv) / 20))
    gains[k] = TARGET[k] - lv
    if k == 'snare':
        throw.x *= g
    x = x * g
    levels[k] = TARGET[k]
    bus[k].x = None
    if k in MUTE:
        continue
    mix += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x
if 'snare' not in MUTE:
    delay_send += throw.x * 0.9
del throw

ir = TH.plate_ir()
verb = TH.convolve(stage(verb_send, HighpassFilter(300), LowpassFilter(7000)), ir)
del verb_send
dly = TH.dub_delay(delay_send, B(0.75), feedback=0.52, repeats=9, hp=380, lp=2600)
dly = dly + TH.convolve(dly, ir) * 0.3
del delay_send
mix = mix + verb * 0.55 + dly * 0.7
del verb, dly

# drum-bus glue is done on the full mix with a gentle compressor below
SECTION_DB = [(0, -5), (3.9, -5), (4, -2.0), (11.9, -2.0), (12, -1.0), (19.9, -1.0), (20, -0.5),
              (27.9, -0.5), (28, -4.0), (35.9, -3.5), (36, 0.5), (51.9, 1.0), (52, -1.0), (60, -1.5),
              (NBARS + 3, -1.5)]
bars_axis = np.arange(N) / SR / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(26))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(75, -2.0, 0.7), PeakFilter(1800, 1.0, 0.7), HighShelfFilter(10000, -2.0, 0.7),
            Compressor(threshold_db=-15, ratio=2.0, attack_ms=25, release_ms=180))
mix = (np.tanh(mix * 1.1) / 1.1).astype(np.float32)
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=100))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-66 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.4 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new01_triphop.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus gains applied (dB):', {k: round(v, 1) for k, v in gains.items()})
