#!/usr/bin/env python3
# song1_ambient.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# "Song 1, Space ambient": a space-ambient / downtempo remix of sf-music1.
# The music is Stardock's (the original game's music, mid/sf-music1.mid),
# remixed here; produced entirely in code with tools/music/edm/synth.py and
# synth_ambient.py: synthesized pads, glass bells, lead, choir, bass and
# drums, plus real orchestral samples, mixed and mastered in this file.
#
# Samples: VSCO-2 Community Edition by Versilian Studios, CC0 1.0 (public
# domain), https://github.com/sgossner/VSCO-2-CE -- harp, oboe (vibrato),
# flute (non-vibrato sustain), F horn (sustain), violin and viola sections
# (sustain vibrato), concert bass drum, snare, shaker, wood click, timpani
# (hits and rolls), suspended cymbal (crescendo and soft hits), gong.
#
# Taken from the original song (C minor / Db: the i - bII Phrygian sway;
# bar numbers are the original's, 1-based):
#   - the harp arpeggio  G3 C4 D4 Eb4 G4 Eb4 D4 C4 | Ab3 Db4 Eb4 F4 Ab4 ...
#     and its Fm7 / Gm7 forms (bars 9-76), as harp + glass bells
#   - the bass C / Db every 4 bars (bars 1-40), F / G (41-56), C Db Eb Db (57-72)
#   - the flute motif    Ab5 Db5 F5 Ab5 | G5   (bars 6-9, 14-17, 78-80), VSCO flute
#   - the string line    C6 ... Bb5 Ab5 (bars 1-8, 73-80) and C6 D6 G6 (41-56)
#   - the whistle line   G3 C4 Eb4 G4 / Ab3 Db4 F4 Db4, then G F Ab .. Bb Db F Ab | G
#     (bars 9-25), as a soft legato synth lead
#   - the oboe melody    G4 F4 Db4 Ab4 .. Bb4 Ab4 G4 F4 G4 .. F4 (bars 30-40), VSCO oboe
#   - the main melody    G4 | G5 F5 Eb5 D5 C5 D5 C5 Bb4 (synth strings, bars 40-56)
#     and the French-horn counter-line G3 Bb3 G3 G3 G3 C4 D4 Eb4 D4 Bb3 Ab3 ...
#     (bars 48-56), lead synth + violins, VSCO horn
#   - the choir phrase   G4 C4 D4 Eb4 Db4 and strings G4 .. F4 Eb4 (bars 57-72),
#     the late oboe line Bb4 Ab4 F4 G4 (bars 67-72)
#   - the taiko rhythm (bars 1-56, 73-80) as distant concert bass drum;
#     the timpani rolls G2 -> C3 (bar 8) and C#3 -> G3 (bar 32)
# New: a half-time 70 BPM feel on the original 140 BPM grid (so the melodies
# keep their real-time speed), an ambient form (beatless intro, downtempo
# groove, first theme, beatless middle, peak with the main theme and
# everything in, outro on the flute motif ending on C minor), a soft
# downtempo beat (tuned muffled kick, brushed snare, shaker), sub bass, wide
# pads through slow filters, shimmer reverb and long ping-pong delays.
#
#   python3 tools/music/edm/song1_ambient.py OUT.wav
#   SONG1_MUTE=kick,snare,perc,taiko  (mute buses, for chroma checks)
import os
import sys

import numpy as np
import pyloudnorm as pyln
import soundfile as sf_
from pedalboard import (Pedalboard, Chorus, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter,
                        PitchShift, Reverb)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_ambient as A  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 140.0                 # the original grid; the groove is half-time (70)
SPB = 60.0 / BPM
NBARS = 108
TAIL = 12.0
N = int((NBARS * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


# ------------------------------------------------------------- harmony

CHORDS = []
CHORDS += ['Cm' if (b // 4) % 2 == 0 else 'Db' for b in range(40)]        # 0-39 intro, groove
CHORDS += ['Fm7' if (b // 2) % 2 == 0 else 'Gm7' for b in range(16)]      # 40-55 theme
CHORDS += ['Cm', 'Cm', 'Db', 'Db', 'Eb', 'Eb', 'Db', 'Db'] * 2             # 56-71 middle
CHORDS += ['Fm7' if (b // 2) % 2 == 0 else 'Gm7' for b in range(16)]      # 72-87 peak
CHORDS += ['Cm'] * 4 + ['Db'] * 4 + ['Cm'] * 4 + ['Db'] * 4                # 88-103 outro
CHORDS += ['Cmf'] * 4                                                      # 104-107 end
assert len(CHORDS) == NBARS

PAD = {'Cm': ['C3', 'G3', 'Eb4', 'G4', 'D5'], 'Db': ['Db3', 'Ab3', 'F4', 'Ab4', 'Eb5'],
       'Eb': ['Eb3', 'Bb3', 'G4', 'Bb4', 'D5'], 'Fm7': ['F3', 'C4', 'Eb4', 'Ab4', 'C5'],
       'Gm7': ['G3', 'D4', 'F4', 'Bb4', 'D5'], 'Cmf': ['C3', 'G3', 'Eb4', 'G4', 'C5', 'D5']}
ROOT = {'Cm': 36, 'Db': 37, 'Eb': 39, 'Fm7': 29, 'Gm7': 31, 'Cmf': 36}        # sub octave
KICK_HZ = {'Cm': 49.0, 'Db': 51.9, 'Eb': 58.3, 'Fm7': 43.65, 'Gm7': 49.0, 'Cmf': 49.0}
HARP = {'Cm': ['G3', 'C4', 'D4', 'Eb4', 'G4', 'Eb4', 'D4', 'C4'],
        'Db': ['Ab3', 'Db4', 'Eb4', 'F4', 'Ab4', 'F4', 'Eb4', 'Db4'],
        'Fm7': ['F3', 'Ab3', 'C4', 'Eb4', 'F4', 'Eb4', 'C4', 'Ab3'],
        'Gm7': ['G3', 'Bb3', 'D4', 'F4', 'G4', 'F4', 'D4', 'Bb3']}
HARP['Eb'] = HARP['Cm']        # as in the original (bars 61-62, 69-70)
HARP['Cmf'] = HARP['Cm']

# ------------------------------------------------------------- themes (beats; note; length)

FLUTE_A = [(2, 'Ab5', 1), (3, 'Db5', 1), (4, 'F5', 4), (8, 'Ab5', 4), (12, 'G5', 8)]
FLUTE_B = [(2, 'Ab5', 1), (3, 'Db5', 1), (4, 'F5', 4), (8, 'Ab5', 2), (10, 'Db6', 2), (12, 'C6', 14)]
STR_A = [(0, 'C6', 16), (16, 'Bb5', 4), (20, 'Ab5', 12)]
STR_B = [(0, 'C6', 16), (16, 'Bb5', 4), (20, 'Ab5', 4), (24, 'F5', 4), (28, 'Ab5', 4)]
STR_C = [(0, 'C6', 16), (16, 'Bb5', 4), (20, 'Ab5', 2), (22, 'G5', 2), (24, 'Ab5', 8), (32, 'G5', 16)]
CHOIR_IN = [(2, 'G4', 2), (4, 'Eb4', 4)]
_W1 = [(0, 'G3', 2), (2, 'C4', 2), (4, 'Eb4', 2), (6, 'G4', 2)]
_W2 = [(0, 'Ab3', 2), (2, 'Db4', 2), (4, 'F4', 2), (6, 'Db4', 2)]
WHISTLE_ARP = [(b + off, n, l) for off, w in ((0, _W1), (8, _W1), (16, _W2), (24, _W2)) for b, n, l in w]
WHISTLE_MEL = [(0, 'G3', 4), (4, 'F3', 2), (6, 'Ab3', 2), (8, 'G3', 4), (12, 'F3', 2), (14, 'Ab3', 2),
               (16, 'Bb3', 4), (20, 'Db4', 2), (22, 'F4', 2), (24, 'Ab4', 8), (32, 'G4', 16)]
OBOE_A = [(5, 'G4', 1), (6, 'F4', 1), (7, 'Db4', 1), (8, 'Ab4', 7), (15, 'G4', .5), (15.5, 'Ab4', .5),
          (16, 'Bb4', 2), (18, 'Ab4', 2), (20, 'G4', 2), (22, 'F4', 2), (24, 'G4', 4), (28, 'G4', 4),
          (32, 'F4', 14)]
VIOLA_A = [(0, 'G4', 2), (2, 'F4', 2), (4, 'Eb4', 2), (6, 'D4', 2), (8, 'Db4', 8), (16, 'Db4', 8)]
MAIN = [(0, 'G5', 6), (6, 'F5', 2), (8, 'Eb5', 4), (12, 'D5', 2), (14, 'C5', 2), (16, 'D5', 8),
        (24, 'C5', 4), (28, 'Bb4', 4)]
MAIN2 = [(-2, 'G4', 2)] + MAIN + [(b + 32, n, l) for b, n, l in MAIN]
HIGH_MAIN = [(0, 'C6', 8), (8, 'D6', 4), (12, 'G6', 4), (16, 'C6', 8), (24, 'D6', 8)]
HIGH_MAIN = HIGH_MAIN + [(b + 32, n, l) for b, n, l in HIGH_MAIN]
HORN = [(28, 'G3', 2), (30, 'Bb3', 2), (32, 'G3', .5), (33, 'G3', .5), (33.5, 'G3', .5), (34, 'C4', 1),
        (35, 'D4', 1), (36, 'Eb4', 2), (38, 'D4', 2), (40, 'Bb3', 3), (43, 'Ab3', 1), (44, 'G3', 2),
        (46, 'F3', 2), (48, 'G3', 6), (54, 'Eb3', 2), (56, 'F3', 8)]
CHOIR_MID = [(2, 'G4', 2), (4, 'C4', 2), (6, 'D4', 2), (8, 'Eb4', 4), (12, 'Db4', 4)]
STR_MID = [(0, 'G4', 12), (12, 'F4', 2), (14, 'Eb4', 2)]
HIGH_MID = [(0, 'C6', 8), (8, 'Db6', 8), (16, 'C6', 8), (24, 'Db6', 8)]
OBOE_B = [(42, 'Bb4', 2), (44, 'Ab4', 2), (46, 'F4', 2), (48, 'G4', 8), (56, 'G4', 4), (60, 'F4', 2.5)]

# --------------------------------------------------------------- instruments

harp_s = A.Sampler('Strings/Harp/KSHarp_*.wav', offset=0)
oboe_s = A.Sampler('Woodwinds/Oboe/Vib/*.wav', offset=12)
flute_s = A.Sampler('Woodwinds/Flute/susNV/*.wav', offset=12)
horn_s = A.Sampler('Brass/F Horn/sus/*.wav', offset=12)
vln_s = A.Sampler('Strings/Violin Section/susVib/*.wav', offset=12)
vla_s = A.Sampler('Strings/Viola Section/susvib/*.wav', offset=12)
snare_s = A.OneShot('Percussion/Snare2-HitSN_v1_*.wav')
snare2_s = A.OneShot('Percussion/Snare2-HitSN_v3_*.wav')
rim_s = A.OneShot('VSCO 1 Percussion/varWood/wood_click_pp_*.wav')
shaker_s = A.OneShot("VSCO 1 Percussion/varWood/Camo's Shaker/shake*.wav")
taiko_s = {k: A.OneShot('Percussion/BDrumNewhit_v%d_*.wav' % k) for k in (3, 5, 7)}
timp_roll = {'G2': (A.OneShot('Percussion/Timpani/Rolls/Timpani1_Roll_v5_*.wav'), 43 - 41.7),
             'C3': (A.OneShot('Percussion/Timpani/Rolls/Timpani2_Roll_v5_*.wav'), 48 - 47.5),
             'C#3': (A.OneShot('Percussion/Timpani/Rolls/Timpani3_Roll_v5_*.wav'), 49 - 49.1)}
timp_hit = {'C3': (A.OneShot('Percussion/Timpani/Timpani2_Hit_v4_*.wav'), 48 - 46.8),
            'F2': (A.OneShot('Percussion/Timpani/Timpani1_Hit_v3_*.wav'), 41 - 41.5),
            'G3': (A.OneShot('Percussion/Timpani/Timpani5_Hit_v4_*.wav'), 55 - 54.6)}
cym_cresc = A.OneShot('Percussion/susCymb1-cresc-Long_v1.wav', normalize=False)
cym_cresc_s = A.OneShot('Percussion/susCymb1-cresc-Median_v1.wav', normalize=False)
cym_soft = A.OneShot('Percussion/susCymb1-hit_pp_*.wav', normalize=False)
gong = A.OneShot('Percussion/gongHit_p.wav', normalize=False)

# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'perc', 'taiko', 'fx', 'sub', 'bass', 'pad', 'choirpad', 'harp', 'glass',
         'strings', 'lead', 'oboe', 'flute', 'horn', 'choir']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def runs(bar0, bar1):
    """(chord, start bar, end bar) runs of equal chords."""
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        yield ch, b, e
        b = e


def jitter(ms=4.0):
    return (S.rng().random() - 0.5) * 2 * ms / 1000.0


# ---- harmony beds

def pads(bar0, bar1, vel=1.0, attack=2.0, release=3.0):
    for ch, b, e in runs(bar0, bar1):
        dur = T(e) - T(b)
        for i, note in enumerate(PAD[ch]):
            sig = A.pad_voice(m(note), dur, 1.0, attack=attack, release=release, cut=5000)
            bus['pad'].add(sig, T(b), vel * 0.4)


def choir_pad(bar0, bar1, vel=1.0, vowel='o'):
    for ch, b, e in runs(bar0, bar1):
        r = ROOT[ch] + 12 if ROOT[ch] >= 36 else ROOT[ch] + 24
        for md in (r, r + 7, r + 12):
            bus['choirpad'].add(A.choir(md, T(e) - T(b), 1.0, vowel=vowel, attack=1.2, release=1.8),
                                T(b), vel * (0.8 if md == r else 0.55))


def bass(bar0, bar1, vel=1.0, warm=1.0, release=0.35):
    for ch, b, e in runs(bar0, bar1):
        dur = T(e) - T(b) - 0.03
        bus['sub'].add(A.sub(ROOT[ch], dur, 1.0, attack=0.12, release=release), T(b), vel)
        if warm:
            bus['bass'].add(A.warm_bass(ROOT[ch] + 12, dur, 1.0), T(b), vel * warm)


def harp(bar0, bar1, vel=1.0, glass=0.6, every=1):
    for b in range(bar0, bar1):
        pat = HARP[CHORDS[b]]
        for i, note in enumerate(pat):
            if i % every:
                continue
            acc = (1.0 if i % 4 == 0 else 0.8) * (0.92 + 0.16 * S.rng().random())
            t = T(b, i * 0.5) + jitter(3)
            bus['harp'].add(cached(('harp', note), lambda: harp_s.note(m(note), B(1.5), 0.8, release=0.6)),
                            t, vel * acc)
            if glass:
                bus['glass'].add(cached(('gl', note), lambda: A.glass(m(note) + 12, B(0.5), decay=0.9)),
                                 t, vel * glass * acc)


def harp_roll(bar, chord_notes, vel=1.0, spread=0.09):
    for i, note in enumerate(chord_notes):
        bus['harp'].add(harp_s.note(m(note), 6.0, 0.8, release=1.5), T(bar) + i * spread, vel)
        bus['glass'].add(A.glass(m(note) + 12, 4.0, decay=2.0), T(bar) + i * spread, vel * 0.5)


# ---- melodies

def sampled(target, smp, bar0, notes, vel=1.0, transpose=0, attack=0.03, release=0.45, legato=0.08,
            layer=None):
    for beat, note, dur in notes:
        md = m(note) + transpose
        sig = smp.note(md, B(dur) + legato, vel=1.0, layer=layer, attack=attack, release=release)
        bus[target].add(sig, T(bar0, beat), vel)


def strings(bar0, notes, vel=1.0, transpose=0, smp=None, attack=0.6, release=1.2):
    sampled('strings', smp or vln_s, bar0, notes, vel, transpose, attack=attack, release=release,
            legato=0.15, layer=1)


def lead(bar0, notes, vel=1.0, transpose=0, **kw):
    t0 = T(bar0, notes[0][0])
    ph = [(T(bar0, b) - t0, m(n) + transpose, B(l)) for b, n, l in notes]
    bus['lead'].add(A.legato_line(ph, **kw), t0, vel)


def choir_line(bar0, notes, vel=1.0, vowel='a', transpose=0):
    for beat, note, dur in notes:
        bus['choir'].add(A.choir(m(note) + transpose, B(dur) + 0.1, 1.0, vowel=vowel, attack=0.3,
                                 release=0.9), T(bar0, beat), vel)


# ---- drums (half-time: one groove bar = 8 beats of the 140 grid; 16th = 0.5 beat)

SWING = 0.06     # beats: late odd 16ths


def place_kick(t, ch, vel):
    k = cached(('kick', ch), lambda: A.soft_kick(KICK_HZ[ch]))
    bus['kick'].add(k, t, vel)
    kicks.append(t)


def groove(bar0, bar1, kick=1.0, snare=1.0, shaker=1.0, rim=1.0, hat=0.0, fill_last=False):
    for b in range(bar0, bar1, 2):
        ch = CHORDS[b]
        cyc = (b - bar0) // 2
        for p in range(16):
            beat = p * 0.5 + (SWING if p % 2 else 0.0)
            t = T(b, beat) + jitter(4)
            hv = 0.9 + 0.2 * S.rng().random()
            if kick:
                if p == 0:
                    place_kick(t, ch, kick)
                elif p == 10:
                    place_kick(t, CHORDS[b + 1], kick * 0.72)
                elif p == 7 and cyc % 2 == 1:
                    place_kick(t, ch, kick * 0.45)
            if snare and p == 8:
                bus['snare'].add(snare_s.get(), t, snare * hv)
                bus['snare'].add(cached(('brush',), brush), t - 0.012, snare * 0.5)
            if rim and p in (3, 13) or (rim and p == 15 and cyc % 2 == 0):
                bus['perc'].add(rim_s.get(), t, rim * 0.22 * hv)
            if shaker:
                acc = [0.55, 0.3, 0.85, 0.35][p % 4]
                bus['perc'].add(shaker_s.get(), t, shaker * 0.32 * acc * hv)
            if hat and p in (2, 6, 10, 14):
                bus['perc'].add(cached(('hat',), lambda: S.hat(1.0, False, 0.3)), t, hat * 0.18 * hv)
            if hat and p == 14 and cyc % 2 == 1:
                bus['perc'].add(cached(('ohat',), lambda: S.hat(1.0, True, -0.3)), t, hat * 0.12)
        if fill_last and b + 2 >= bar1:
            for i, bt in enumerate((5.5, 6.0, 6.5, 7.0, 7.25, 7.5, 7.75)):
                bus['snare'].add(snare2_s.get(), T(b, bt) + jitter(3), 0.18 + 0.07 * i)


def brush():
    n = int(0.35 * SR)
    t = np.arange(n) / SR
    x = S.rng().standard_normal((2, n))
    env = np.minimum(t / 0.02, 1) * np.exp(-t / 0.09)
    return S.butter(x * env, [1800, 9000], 'band', 1).astype(np.float32) * 0.35


# taiko rhythm from the original (2-bar cycles; beats; MIDI velocity)
TAIKO_A = [[(0, 127), (.5, 69), (1, 69), (1.5, 69), (2, 69), (3.5, 69), (3.75, 69), (4, 69), (6, 69), (7.5, 91)],
           [(0, 127), (.5, 58), (1, 58), (1.5, 58), (2, 58), (3.5, 58), (3.75, 58), (4, 58), (5.5, 58),
            (5.75, 58), (6, 69), (6.5, 69), (7, 80), (7.5, 91), (7.75, 91)]]
TAIKO_B = [[(0, 127)] + [(x * .5, 58) for x in range(1, 12)] + [(6, 69), (6.5, 69), (7, 80), (7.5, 91), (7.75, 91)]]


def taiko(bar0, bar1, pattern, vel=1.0, semis=-3.0):
    for i, b in enumerate(range(bar0, bar1, 2)):
        for beat, v in pattern[i % len(pattern)]:
            u = (v / 127.0) ** 2
            layer = 7 if v >= 110 else (5 if v >= 85 else 3)
            sig = cached(('tk', layer, i % 2), lambda: taiko_s[layer].get(i % 2, semis=semis, max_len=3.0))
            bus['taiko'].add(sig, max(0.0, T(b, beat) + jitter(5)), vel * u)


def timp(roll, hit, bar_hit, roll_beats=2.0, vel=1.0):
    smp, st = timp_roll[roll]
    x = smp.get(0, semis=st)
    k = int(B(roll_beats) * SR)
    seg = x[:, int(1.0 * SR): int(1.0 * SR) + k].copy()
    seg *= (np.linspace(0.15, 1.0, k) ** 1.5).astype(np.float32)
    seg[:, -64:] *= np.linspace(1, 0, 64, dtype=np.float32)
    bus['fx'].add(seg, T(bar_hit) - B(roll_beats), vel * 0.5)
    smp, st = timp_hit[hit]
    bus['fx'].add(smp.get(0, semis=st, max_len=6.0), T(bar_hit), vel * 0.8)


def swell(bar_peak, which=None, vel=1.0):
    """Suspended-cymbal crescendo whose loudest point lands on bar_peak."""
    x = (which or cym_cresc).get(0)
    env = np.convolve(np.max(np.abs(x), axis=0), np.ones(2048) / 2048, 'same')
    pk = int(np.argmax(env))
    bus['fx'].add(x, T(bar_peak) - pk / SR, vel * 0.9)


# ---------------------------------------------------------- arrangement

# INTRO I (0-7): drones, high strings, distant taiko, the flute motif
pads(0, 16, vel=0.75, attack=4.0)
choir_pad(0, 16, vel=0.6)
bass(0, 16, vel=0.55, warm=0.5)
strings(0, STR_A, vel=0.7, attack=2.5)
choir_line(0, CHOIR_IN, vel=0.6, vowel='o')
taiko(0, 8, TAIKO_A, vel=0.55)
sampled('flute', flute_s, 5, FLUTE_A, vel=0.85, attack=0.06, release=0.8)
timp('G2', 'C3', 8, roll_beats=2.0, vel=0.7)

# INTRO II (8-15): the harp figure enters, whistle line as a soft lead
harp(8, 16, vel=0.75, glass=0.45)
strings(8, STR_B, vel=0.7, attack=1.2)
lead(8, WHISTLE_ARP, vel=0.45, cut=900, cut_env=500, vib_depth=0.08, attack=0.25,
     release=0.9)
taiko(8, 16, TAIKO_B, vel=0.45)
sampled('flute', flute_s, 13, FLUTE_B, vel=0.9, attack=0.06, release=0.9)
swell(16, cym_cresc_s, vel=0.45)

# GROOVE (16-39): the beat comes in; whistle melody, then the oboe
pads(16, 40, vel=0.7, attack=1.5)
choir_pad(16, 40, vel=0.45)
bass(16, 40, vel=0.9, warm=0.7)
harp(16, 40, vel=0.8, glass=0.55)
groove(16, 20, kick=0.85, snare=0.0, shaker=0.7, rim=0.0)
groove(20, 24, kick=0.9, snare=0.75, shaker=0.85, rim=0.6)
groove(24, 32, kick=1.0, snare=0.9, shaker=1.0, rim=1.0)
groove(32, 40, kick=1.0, snare=0.9, shaker=1.0, rim=1.0, hat=0.5, fill_last=True)
taiko(16, 40, TAIKO_B, vel=0.4)
strings(16, STR_C, vel=0.65, attack=1.0)
lead(16, [(b, n, l) for b, n, l in WHISTLE_MEL], vel=0.8, transpose=12, cut=1500, cut_env=900)
sampled('oboe', oboe_s, 28, OBOE_A, vel=0.95, attack=0.04, release=0.5)
strings(34, VIOLA_A, vel=0.5, smp=vla_s, attack=0.4)
strings(28, [(0, 'Db6', 16), (16, 'C6', 16), (32, 'Db6', 16)], vel=0.35, attack=2.0)
timp('C#3', 'G3', 32, roll_beats=4.0, vel=0.55)
swell(40, cym_cresc_s, vel=0.5)

# THEME (40-55): the main melody, first time; horn counter-line in the second half
pads(40, 56, vel=0.75, attack=0.8)
choir_pad(40, 56, vel=0.5, vowel='a')
bass(40, 56, vel=1.0, warm=0.8)
harp(40, 56, vel=0.85, glass=0.6)
groove(40, 56, kick=1.0, snare=0.95, shaker=1.0, rim=1.0, hat=0.8, fill_last=True)
taiko(40, 56, TAIKO_B, vel=0.45)
bus['fx'].add(cym_soft.get(0), T(40), 0.35)
lead(39, [(b + 4, n, l) for b, n, l in MAIN2], vel=1.0, cut=1900, cut_env=1400)
strings(40, MAIN, vel=0.35, transpose=-12, attack=0.3)
strings(48, MAIN, vel=0.35, transpose=-12, attack=0.3)
strings(40, HIGH_MAIN, vel=0.4, attack=1.5)
sampled('horn', horn_s, 40, HORN, vel=0.9, attack=0.05, release=0.6)
swell(56, cym_cresc_s, vel=0.35)

# MIDDLE (56-71): beatless; choir phrase, strings, the late oboe line
pads(56, 72, vel=0.85, attack=2.5, release=4.0)
choir_pad(56, 72, vel=0.55)
bass(56, 72, vel=0.6, warm=0.5)
harp(56, 72, vel=0.65, glass=0.7, every=1)
for k in range(4):
    choir_line(56 + 4 * k, CHOIR_MID, vel=0.9 - 0.08 * (k == 3))
    strings(56 + 4 * k, STR_MID if k < 3 else [(0, 'G4', 12), (12, 'F4', 4)], vel=0.5, smp=vla_s,
            attack=0.8)
strings(56, HIGH_MID + [(b + 32, n, l) for b, n, l in HIGH_MID], vel=0.35, attack=2.0)
sampled('oboe', oboe_s, 56, OBOE_B, vel=0.9, attack=0.05, release=0.6)
bus['fx'].add(gong.get(0, semis=-5, max_len=20.0), T(56), 0.25)
timp('C3', 'F2', 72, roll_beats=6.0, vel=0.8)
swell(72, cym_cresc, vel=0.7)

# PEAK (72-87): the main melody returns with everything in
pads(72, 88, vel=0.9, attack=0.5)
choir_pad(72, 88, vel=0.75, vowel='a')
bass(72, 88, vel=1.0, warm=0.9)
harp(72, 88, vel=0.9, glass=0.75)
groove(72, 88, kick=1.0, snare=1.0, shaker=1.0, rim=1.0, hat=1.0)
taiko(72, 88, TAIKO_B, vel=0.55)
bus['fx'].add(gong.get(0, semis=-3, max_len=20.0), T(72), 0.5)
lead(71, [(b + 4, n, l) for b, n, l in MAIN2], vel=1.0, cut=2400, cut_env=1800)
sampled('flute', flute_s, 72, MAIN + [(b + 32, n, l) for b, n, l in MAIN], vel=0.45, transpose=12,
        attack=0.08, release=0.6)
strings(72, MAIN, vel=0.55, attack=0.25)
strings(80, MAIN, vel=0.6, attack=0.25)
strings(72, MAIN, vel=0.45, transpose=-12, attack=0.25)
strings(80, MAIN, vel=0.45, transpose=-12, attack=0.25)
strings(72, HIGH_MAIN, vel=0.5, attack=1.0)
sampled('horn', horn_s, 72, [(0, 'C4', 8), (8, 'D4', 8), (16, 'C4', 8), (24, 'Bb3', 4)], vel=0.6,
        attack=0.3, release=0.8)
sampled('horn', horn_s, 72, HORN, vel=1.0, attack=0.05, release=0.6)
choir_line(72, [(0, 'C5', 8), (8, 'Bb4', 8), (16, 'Ab4', 8), (24, 'Bb4', 8)] * 1, vel=0.5)
choir_line(80, [(0, 'C5', 8), (8, 'Bb4', 8), (16, 'Ab4', 8), (24, 'Bb4', 8)], vel=0.55)
swell(88, cym_cresc_s, vel=0.4)

# OUTRO (88-103): back to C / Db; the beat thins and leaves; the flute motif closes on C minor
pads(88, 104, vel=0.75, attack=1.5, release=5.0)
pads(104, 108, vel=0.75, attack=0.8, release=14.0)
choir_pad(88, 108, vel=0.5)
bass(88, 104, vel=0.85, warm=0.6)
harp(88, 104, vel=0.75, glass=0.6)
groove(88, 92, kick=0.9, snare=0.8, shaker=0.9, rim=0.8, hat=0.4)
groove(92, 96, kick=0.75, snare=0.0, shaker=0.7, rim=0.4)
taiko(88, 104, TAIKO_A, vel=0.45)
bus['fx'].add(cym_soft.get(1), T(88), 0.3)
strings(88, STR_A, vel=0.6, attack=1.5)
choir_line(88, CHOIR_IN, vel=0.55, vowel='o')
sampled('flute', flute_s, 93, FLUTE_A, vel=0.85, attack=0.06, release=0.9)
strings(96, STR_A, vel=0.5, attack=2.0)
sampled('flute', flute_s, 101, FLUTE_A[:-1] + [(12, 'G5', 14)], vel=0.8, attack=0.06, release=1.5)
timp('G2', 'C3', 104, roll_beats=4.0, vel=0.55)
# END (104-107): C minor (add 9), a rolled harp chord
bass(104, 108, vel=0.7, warm=0.4, release=2.5)
harp_roll(104, ['C3', 'G3', 'C4', 'D4', 'Eb4', 'G4', 'C5'], vel=0.7)
bus['fx'].add(gong.get(0, semis=-5, max_len=20.0), T(104), 0.3)

# ------------------------------------------------------------- mix

del _cache


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


bars_axis = np.arange(N, dtype=np.float32) / np.float32(SR * 4 * SPB)


def curve(points):
    return np.interp(bars_axis, [p[0] for p in points], [p[1] for p in points]).astype(np.float32)


duck_sub = S.sidechain(N, kicks, depth=0.5, release=0.28)
duck_pad = S.sidechain(N, kicks, depth=0.18, release=0.4)

GAIN = {  # dB
    'kick': -10.0, 'snare': -9.0, 'perc': -0.5, 'taiko': 1.0, 'fx': 0.0,
    'sub': -14.5, 'bass': -7.0, 'pad': -0.5, 'choirpad': -5.5, 'harp': -7.0, 'glass': -9.0,
    'strings': -8.0, 'lead': -3.0, 'oboe': -2.7, 'flute': -3.0, 'horn': -4.5, 'choir': -5.0,
}
DUCK = {'sub': duck_sub, 'bass': duck_sub, 'pad': duck_pad, 'choirpad': duck_pad}
VERB = {'snare': .15, 'perc': .12, 'taiko': .8, 'fx': .5, 'pad': .5, 'choirpad': .6, 'harp': .45,
        'glass': .6, 'strings': .55, 'lead': .4, 'oboe': .45, 'flute': .55, 'horn': .45, 'choir': .6}
SHIMMER = {'pad': .15, 'glass': .35, 'strings': .25, 'flute': .25, 'harp': .1, 'choir': .15}
DELAY = {'harp': .35, 'glass': .45, 'flute': .35, 'lead': .25, 'oboe': .2}
LONG_DELAY = {'lead': .2, 'flute': .15, 'horn': .1}

# pad: a slow filter whose centre follows the sections (dark intro and middle, open peak)
pad_cut = curve([(0, 700), (16, 1100), (40, 1600), (56, 1000), (71, 1400), (72, 2600), (88, 1600),
                 (108, 700)])
bus['pad'].x = A.lfo_filter(bus['pad'].x, pad_cut, depth_oct=0.7, rate=0.045, q=0.9)
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(140), PeakFilter(450, -3.0, 0.8),
                     Chorus(rate_hz=0.25, depth=0.35, centre_delay_ms=12, mix=0.5))
bus['choirpad'].x = A.lfo_filter(bus['choirpad'].x, 2200, depth_oct=0.5, rate=0.031, q=0.7, phase=1.0)
bus['choirpad'].x = stage(bus['choirpad'].x, HighpassFilter(160), PeakFilter(500, -3.0, 0.8))
# the harp figure opens up over the second intro phrase
harp_cut = curve([(0, 600), (8, 600), (16, 5000), (56, 5000), (57, 2500), (71, 4000), (72, 7000),
                  (88, 5000), (108, 2000)])
bus['harp'].x = S.svf(bus['harp'].x, harp_cut, 0.8).astype(np.float32)
bus['glass'].x = S.svf(bus['glass'].x, harp_cut * 1.6, 0.8).astype(np.float32)
bus['harp'].x = stage(bus['harp'].x, HighpassFilter(180))
bus['glass'].x = stage(bus['glass'].x, HighpassFilter(400))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(160), Chorus(rate_hz=0.6, depth=0.15, centre_delay_ms=7,
                                                                  mix=0.3))
bus['strings'].x = stage(bus['strings'].x, HighpassFilter(150), HighShelfFilter(6000, -1.5, 0.7))
bus['oboe'].x = stage(bus['oboe'].x, HighpassFilter(200), PeakFilter(1400, -2.0, 1.0))
bus['flute'].x = stage(bus['flute'].x, HighpassFilter(250))
bus['horn'].x = stage(bus['horn'].x, HighpassFilter(90))
bus['choir'].x = stage(bus['choir'].x, HighpassFilter(180), Chorus(rate_hz=0.3, depth=0.3, centre_delay_ms=9,
                                                                    mix=0.4))
bus['kick'].x = stage(bus['kick'].x, LowpassFilter(1500))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(150), LowpassFilter(7000),
                       Reverb(room_size=0.3, damping=0.6, wet_level=0.18, dry_level=1.0, width=0.8))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(500), LowpassFilter(12000))
bus['taiko'].x = stage(bus['taiko'].x, HighpassFilter(35), LowpassFilter(1400))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(40))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(70), LowpassFilter(700))
PAN = {'harp': -0.3, 'glass': 0.35, 'oboe': 0.2, 'horn': -0.25, 'flute': 0.25, 'taiko': -0.1}
for k, p in PAN.items():
    x = bus[k].x
    mid = (x[0] + x[1]) * 0.5
    side = (x[0] - x[1]) * 0.5
    bus[k].x = np.stack([mid * np.float32(np.sqrt(1 - p)) + side, mid * np.float32(np.sqrt(1 + p)) - side])

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
shim_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
long_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('SONG1_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        continue
    x = x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    if np.any(x):
        act = np.max(np.abs(x), axis=0) > 1e-4
        levels[k] = meter.integrated_loudness(x[:, act].T.astype(np.float64)) if act.sum() > SR else -99.0
    mix += x
    for d_, s_ in ((VERB, verb_send), (SHIMMER, shim_send), (DELAY, delay_send), (LONG_DELAY, long_send)):
        if k in d_:
            s_ += x * np.float32(d_[k])
    del x

IR = A.hall_ir()
verb = A.convolve(stage(verb_send, HighpassFilter(220), LowpassFilter(10000)), IR)
del verb_send
shim = stage(shim_send, HighpassFilter(500), PitchShift(semitones=12.0), LowpassFilter(9000))
shim = A.convolve(shim, A.hall_ir(t60=(7, 8, 6, 3.5), length=9.0, seed=5))
shim = stage(shim, PitchShift(semitones=12.0), HighpassFilter(1500)) * 0.35 + shim
del shim_send
dly = S.pingpong(delay_send, B(1.5), feedback=0.5, repeats=8, lp=4500, hp=400)
del delay_send
dly2 = S.pingpong(long_send, B(3.0), feedback=0.45, repeats=6, lp=3000, hp=500)
del long_send
echo = A.convolve(stage(dly + dly2, HighpassFilter(300)), IR) * 0.5 + (dly + dly2) * 0.6
del dly, dly2
mix += verb * np.float32(1.0) + shim * np.float32(0.35) + echo * np.float32(0.7)
del verb, shim, echo

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -7), (8, -5.5), (15.9, -4.5), (16, -3.5), (24, -3), (39.9, -2), (40, -2.5), (55.9, -2),
              (56, -7.5), (64, -7), (68, -6), (71.9, -2.5), (72, 0.5), (87.9, 0.5), (88, -2.5), (96, -4.5),
              (104, -5.5), (108, -6), (115, -6)]
mix = mix * (10 ** (curve(SECTION_DB) / 20))[None, :]

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(25))
# keep the low end mono below ~120 Hz
lo = S.butter(mix, 120, 'low', 2).astype(np.float32)
lo_m = (lo[0] + lo[1]) * 0.5
mix = mix - lo + np.stack([lo_m, lo_m])
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-17.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(70, 0.5, 0.7), HighShelfFilter(9000, 1.0, 0.7),
            Compressor(threshold_db=-18, ratio=1.8, attack_ms=40, release_ms=300))
mix = np.tanh(mix * 1.05) / 1.05
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=150))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-66 / 20))[0][-1]
mix = mix[:, : last + 1]
fin = int(0.005 * SR)
mix[:, :fin] *= np.linspace(0, 1, fin)[None, :].astype(np.float32)
fade = int(1.5 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 2

out = sys.argv[1] if len(sys.argv) > 1 else 'song1_ambient.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, after bus gain):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:9s} {v:6.1f}')
