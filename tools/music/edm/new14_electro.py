#!/usr/bin/env python3
# new14_electro.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Cold Signal": new track 14 (dynamic-music tag TENSION), Detroit electro in
# the manner of Drexciya, Aux 88 and Dopplereffekt: 125 BPM, C minor, a TR-808
# broken electro beat (long tuned boom kick, a short round snare, rimshot
# ticks, toms), a squelchy 303-style bassline over a clean sine sub, cold
# minor-key 16th arpeggios, open-fifth "signal" stabs, sonar pings, a dark
# string-machine bed and a cold mono lead.  Spy-like suspense: Phrygian Db,
# a dominant G with b9/b13, a tritone leap.  A NEW piece (not a remix).
#
# Credits: the source music is Stardock's (the original Stellar Frontier game
# music, 1997: mid/sf-music7.mid and mid/sf-music1.mid).  Composed,
# synthesized and mixed in code with tools/music/edm/synth.py, synth_goa.py,
# synth_new08.py (all unchanged) and synth_new14.py: no samples, no SoundFonts.
#
# Built from fragments that song7_orchestral.py, song1_ambient.py,
# new07_cinematic.py, new01_triphop.py, new09_spacesynth.py and
# new10_ambient_techno.py do not use (bar numbers: sf-music7 counts from the
# bass's first downbeat after the two-beat pickup, as in song7_orchestral.py;
# sf-music1 is 1-based):
#   F1 sf-music7, the brass-section "call" of the C-minor interlude, bars
#      19-33 (ch 4, Brass Section): open fifths C5+G5 struck on beats 3, 3.5,
#      3.75 and then held from the next downbeat.  It is the "signal": the
#      open-fifth chord stab of the whole track (root-fifth-octave on every
#      chord, struck in that rhythm every fourth bar), and the three-note
#      pickup rhythm that leads into each phrase of both lead themes.
#   F2 sf-music7, the taiko figure of bars 20-32 (ch 2, even bars only):
#      16th 16th 8th 8th on beats 1.5, 1.75, 2, 2.5.  It is the rhythmic cell
#      of the squelch bassline (root, root, octave, seventh) and the 808 tom
#      figure of the breakdown and the bridge.
#   F3 sf-music1, the choir's opening sigh G4 -> Eb4 (ch 9, bars 3-4; it
#      returns in bars 75-76): the first gesture of theme A (G5 Eb5) and the
#      sonar pings (G6 / Eb6) of the intro, breakdown and outro.
#   F4 sf-music1, the flute's variant tail Ab5 Db6 | C6 (ch 7, bars 16-17: the
#      rising fourth to the Phrygian Db and the fall to C, unlike the
#      Ab Db F Ab | G motif of bars 6-9 that song1_ambient.py quotes).  It is
#      the answer of theme A (over Db -> Ab), and theme B develops it by
#      sequence (Ab Db | C, F Bb | Ab) and by a tritone (Db G) over Db.
# New: tempo, the form (beatless intro, groove, theme A, breakdown, build,
# theme B peak, bridge, peak with both themes, outro, a final C-minor hit
# that rings out and loops back to the intro), all harmony (Cm Cm Db Ab |
# Cm Cm Fm G, and Db Ab Bbm Fm | Db Ab G G), the bass and arp patterns,
# the drum programming and all sound design.
#
#   python3 tools/music/edm/new14_electro.py OUT.wav
#   env NEW14_BARS=a,b     renders only bars a..b-1 (while iterating)
#   env NEW14_MUTE=kick,snare,...   mutes buses
#   env NEW14_TONAL=path.wav  also writes the dry tonal buses (chroma check)
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
import synth_goa as G  # noqa: E402
import synth_new08 as X8  # noqa: E402
import synth_new14 as X  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 125.0
SPB = 60.0 / BPM
NBARS = 101          # bar 100 is the final hit
TAIL = 5.0
_bars = os.environ.get('NEW14_BARS')
BAR0, BAR1 = (int(v) for v in _bars.split(',')) if _bars else (0, NBARS)
OFF = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def live(t, length=8.0):
    return t + length > OFF and t < OFF + N / SR


rng = np.random.default_rng(2014)


def hv(v, amt=0.06):
    return float(np.clip(v * (1 + amt * (rng.random() * 2 - 1)), 0.02, 1.3))


# ------------------------------------------------------------- harmony

ROOT = {'Cm': 'C', 'Db': 'Db', 'Ab': 'Ab', 'Fm': 'F', 'G': 'G', 'Bbm': 'Bb'}
MINOR = {'Cm', 'Fm', 'Bbm'}
BASS_OCT = {'Cm': 'C2', 'Db': 'Db2', 'Ab': 'Ab1', 'Fm': 'F1', 'G': 'G1', 'Bbm': 'Bb1'}
PADV = {'Cm': ['C3', 'G3', 'Eb4'], 'Db': ['Db3', 'Ab3', 'F4'], 'Ab': ['Ab2', 'Eb3', 'C4'],
        'Fm': ['F3', 'C4', 'Ab4'], 'G': ['G2', 'D3', 'B3'], 'Bbm': ['Bb2', 'F3', 'Db4']}
STABV = {'Cm': ['C4', 'G4', 'C5'], 'Db': ['Db4', 'Ab4', 'Db5'], 'Ab': ['Ab3', 'Eb4', 'Ab4'],
         'Fm': ['F3', 'C4', 'F4'], 'G': ['G3', 'D4', 'G4'], 'Bbm': ['Bb3', 'F4', 'Bb4']}
ARPV = {'Cm': ['C4', 'G4', 'C5', 'Eb5', 'G5'], 'Db': ['Db4', 'Ab4', 'Db5', 'F5', 'Ab5'],
        'Ab': ['Ab3', 'Eb4', 'Ab4', 'C5', 'Eb5'], 'Fm': ['F3', 'C4', 'F4', 'Ab4', 'C5'],
        'G': ['G3', 'D4', 'G4', 'B4', 'D5'], 'Bbm': ['Bb3', 'F4', 'Bb4', 'Db5', 'F5']}

VAMP = ['Cm', 'Cm', 'Db', 'Cm']
CH_A = ['Cm', 'Cm', 'Db', 'Ab', 'Cm', 'Cm', 'Fm', 'G']
CH_B = ['Db', 'Ab', 'Bbm', 'Fm', 'Db', 'Ab', 'G', 'G']
CHORDS = (['Cm'] * 4 + ['Cm', 'Cm', 'Db', 'Cm'] +      # 0-7    intro
          VAMP * 4 +                                    # 8-23   groove
          CH_A * 2 +                                    # 24-39  theme A
          ['Db', 'Db', 'Cm', 'Cm', 'Ab', 'Ab', 'G', 'G'] +   # 40-47 breakdown
          VAMP * 2 +                                    # 48-55  build
          CH_B * 2 +                                    # 56-71  peak 1: theme B
          ['Cm', 'Cm', 'Db', 'Db', 'Cm', 'Cm', 'G', 'G'] +   # 72-79 bridge
          CH_A + CH_B +                                 # 80-95  peak 2
          ['Cm', 'Cm', 'Db', 'Cm'] +                    # 96-99  outro
          ['Cm'])                                       # 100    final hit
assert len(CHORDS) == NBARS, len(CHORDS)

# theme A (F3 sigh, F1 pickup rhythm, F4 flute tail) over CH_A: (beat, note, len)
THEME_A = [(0, 'G5', 1.5), (1.5, 'Eb5', 2.5),
           (7, 'C5', .25), (7.5, 'C5', .25), (7.75, 'Eb5', .25),
           (8, 'Ab5', 2), (10, 'Db6', 2),
           (12, 'C6', 2.5), (15, 'C6', .25), (15.5, 'Bb5', .25), (15.75, 'Ab5', .25),
           (16, 'G5', 1.5), (17.5, 'Eb5', 2.5),
           (23, 'D5', .25), (23.5, 'Eb5', .25), (23.75, 'F5', .25),
           (24, 'Ab5', 2), (26, 'G5', 2),
           (28, 'F5', 1), (29, 'Eb5', 1), (30, 'D5', 2)]
THEME_A2 = THEME_A[:-3] + [(28, 'F5', .5), (28.5, 'Eb5', .5), (29, 'D5', 1), (30, 'B4', 2)]
# theme B (F4 developed by sequence and a tritone) over CH_B
THEME_B = [(0, 'Ab5', 2), (2, 'Db6', 2),
           (4, 'C6', 3), (7, 'C6', .25), (7.5, 'C6', .25), (7.75, 'Bb5', .25),
           (8, 'F5', 2), (10, 'Bb5', 2),
           (12, 'Ab5', 3), (15, 'F5', .25), (15.5, 'F5', .25), (15.75, 'Eb5', .25),
           (16, 'Db5', 2), (18, 'G5', 2),
           (20, 'F5', 2), (22, 'Eb5', 2),
           (24, 'D5', 1.5), (25.5, 'F5', 1), (26.5, 'Ab5', 1.5),
           (28, 'G5', 3.5)]

# --------------------------------------------------------------- buses
names = ['kick', 'snare', 'hats', 'rim', 'perc', 'cym', 'bass', 'sub', 'arp', 'stab', 'pad',
         'lead', 'ping', 'fx']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}
BASS_EV = []    # mono lines: (t0, t1, midi, accent, slide) in absolute seconds
ARP_EV = []
LEAD_EV = []


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

KICK_A = [0, 6, 10]            # 16th steps
KICK_B = [0, 3, 10, 13]
SNARE_ST = [4, 12]


def place_kick(bar, beat, vel=1.0):
    t = T(bar, beat)
    put('kick', t, lambda: X.kick808(1.0), vel, length=1, key=('kick',))
    if OFF - 1 <= t < OFF + N / SR:
        kicks.append(t - OFF)


def beat(bar0, bar1, kick=1.0, snare=0.9, hats=0.3, rims=0.0, ghost=True, alt=True, stop=None):
    """The electro break: kick pattern A / B alternating, snare on 2 and 4,
    16th closed hats with accents, an open hat on the last off-beat of B bars.
    stop: absolute beat (in bar1-1) after which nothing is placed."""
    for b in range(bar0, bar1):
        pat = KICK_B if (alt and b % 2 == 1) else KICK_A
        for st in range(16):
            bt = st / 4
            if stop is not None and b == bar1 - 1 and bt >= stop:
                break
            if kick and st in pat:
                place_kick(b, bt, kick * (1.0 if st == 0 else 0.92))
            if snare and st in SNARE_ST:
                v = int(rng.integers(4))
                put('snare', T(b, bt), lambda: X.snare(1.0, v), hv(snare, 0.05), length=1, key=('sn', v))
            if snare and ghost and b % 4 == 3 and st == 15:
                put('snare', T(b, bt), lambda: X.snare(1.0, 3, soft=1.0), hv(snare * 0.28, 0.05), length=1,
                    key=('sng',))
            if hats:
                acc = (1.0 if st % 4 == 2 else 0.55 if st % 2 == 0 else 0.38)
                if b % 2 == 1 and st == 14:
                    v = int(rng.integers(3))
                    put('hats', T(b, bt), lambda: X8.hat909(1.0, True, v, 0.15), hv(hats * 0.7, 0.06), length=1,
                        key=('oh', v))
                    continue
                v = int(rng.integers(4))
                pan = -0.15 if st % 2 else 0.1
                put('hats', T(b, bt), lambda: X8.hat909(1.0, False, v, pan), hv(hats * acc, 0.1), length=1,
                    key=('ch', v, pan))
            if rims and st in (3, 7, 11) and not (st == 7 and b % 2 == 0):
                v = int(rng.integers(3))
                put('rim', T(b, bt), lambda: X.rim(1.0, v), hv(rims * (1.0 if st == 3 else 0.75), 0.08), length=1,
                    key=('rim', v))


TOM_FIG = [(1.5, 'C4', 0.8, 0.35), (1.75, 'C4', 0.6, 0.35), (2.0, 'G3', 1.0, -0.2), (2.5, 'C3', 0.9, -0.4)]


def toms(bar0, bar1, vel=0.6, every=2):
    """F2: the taiko figure 16th 16th 8th 8th on beats 1.5-2.5 on 808 toms."""
    for b in range(bar0, bar1):
        if (b - bar0) % every != every - 1:
            continue
        for bt, note, acc, pan in TOM_FIG:
            md = m(note)
            put('perc', T(b, bt), lambda: X.tom808(md, 1.0, pan), hv(vel * acc, 0.06), length=1,
                key=('tom', md, pan))


def tom_fill(bar, vel=0.7):
    for bt, note in ((2.0, 'C4'), (2.5, 'G3'), (3.0, 'Eb3'), (3.5, 'C3')):
        md = m(note)
        put('perc', T(bar, bt), lambda: X.tom808(md, 1.0, 0.0), hv(vel, 0.05), length=1, key=('tomf', md))


def crash_at(bar, vel=0.4):
    put('cym', T(bar), lambda: X8.crash909(1.0, seed=bar % 2), vel, length=3, key=('cr', bar % 2))


# ------------------------------------------------------------- bass

# (beat, interval, length, accent, slide).  F2's taiko cell is beats 1.5-2.5.
BASS_PAT = [(0, 0, .5, 1.0, False), (0.75, 12, .25, 0.0, False),
            (1.5, 0, .25, 0.3, False), (1.75, 0, .25, 0.0, False), (2.0, 12, .5, 1.0, False),
            (2.5, 'b7', .5, 0.0, True),
            (3.25, 0, .25, 0.0, False), (3.5, 7, .25, 0.6, False), (3.75, 12, .2, 0.0, True)]
BASS_TURN = [(3.0, 0, .25, 0.3, False), (3.25, 3, .25, 0.0, False), (3.5, 5, .25, 0.6, False),
             (3.75, 7, .2, 0.0, True)]


def bass(bar0, bar1, vel=1.0, sub=0.8, turn=4, stop=None):
    for b in range(bar0, bar1):
        ch = CHORDS[b]
        r = m(BASS_OCT[ch])
        pat = BASS_PAT
        if turn and b % turn == turn - 1 and ch in MINOR:
            pat = [p for p in BASS_PAT if p[0] < 3.0] + BASS_TURN
        for bt, iv, ln, ac, sl in pat:
            if stop is not None and b == bar1 - 1 and bt >= stop:
                break
            if iv == 'b7':
                iv = 10 if ch in MINOR else 7
            t0 = T(b, bt)
            BASS_EV.append((t0, t0 + B(ln) * 0.92, r + iv, ac * vel, sl))
            if sub and iv in (0, 12) and ac >= 0.3:
                md = r if r >= 31 else r + 12
                d = round(B(ln) * 0.9, 4)
                put('sub', t0, lambda: S.sub_bass(md, d, 1.0, a=0.003, r=0.04, drive=1.2), sub * vel,
                    length=1, key=('sub', md, d))


def bass_hold(bar, beats, vel=0.7, sub=0.7):
    ch = CHORDS[bar]
    r = m(BASS_OCT[ch])
    t0 = T(bar)
    BASS_EV.append((t0, t0 + B(beats) - 0.05, r, 0.0, False))
    if sub:
        md = r if r >= 31 else r + 12
        d = B(beats) - 0.1
        put('sub', t0, lambda: S.sub_bass(md, d, 1.0, a=0.2, r=0.4, drive=1.1), sub, length=d + 3)


# ------------------------------------------------------------- arp / stabs / pad

ARP_IDX = [0, 2, 1, 3, 2, 4, 3, 1, 0, 2, 1, 3, 4, 2, 3, 1]
ARP_ACC = {0, 3, 6, 10, 13}


def arp(bar0, bar1, vel=1.0, oct_=0, gate=0.55, every=1):
    for b in range(bar0, bar1):
        tones = [m(n) + oct_ for n in ARPV[CHORDS[b]]]
        for st in range(0, 16, every):
            t0 = T(b, st / 4)
            ac = 0.8 if st in ARP_ACC else 0.0
            ARP_EV.append((t0, t0 + B(0.25 * every) * gate, tones[ARP_IDX[st]], ac * vel, False))


def stab_hit(bar, bt, chord, dur, vel, cut=1400.0):
    notes = tuple(m(n) for n in STABV[chord])
    cq = int(round(np.log(cut) / np.log(1.05)))
    c = 1.05 ** cq
    put('stab', T(bar, bt), lambda: X8.stab(list(notes), B(dur), 1.0, cut=c, env=2.6, decay=0.05, q=1.3,
                                            amp_tau=0.22), vel, length=3, key=('stab', notes, round(dur, 3), cq))


def signal(bar0, bar1, vel=1.0, cut=1400.0, every=4):
    """F1: the open-fifth call, struck on 3, 3.5, 3.75 and held into the next downbeat."""
    for b in range(bar0, bar1):
        if (b - bar0) % every != every - 1:
            continue
        nxt = CHORDS[b + 1] if b + 1 < NBARS else CHORDS[b]
        for bt, ln, ac in ((3.0, .3, 0.85), (3.5, .2, 0.7), (3.75, .2, 0.75)):
            stab_hit(b, bt, CHORDS[b], ln, hv(vel * ac, 0.04), cut)
        stab_hit(b + 1, 0, nxt, 1.5, hv(vel, 0.04), cut * 1.2)


def pad(bar0, bar1, vel=0.5, a=1.0, cut=1500.0, end=None):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        t1 = T(e) if (end is None or e < bar1) else end
        d = t1 - T(b)
        for note in PADV[ch]:
            md = m(note)
            put('pad', T(b), lambda: X8.strings(md, d, 1.0, a=a, r=1.0, cut=cut, octave=0.12), vel,
                length=d + 6)
        b = e


def lead(bar, notes, vel=1.0, transpose=0):
    for i, (bt, note, ln) in enumerate(notes):
        t0 = T(bar, bt)
        nxt = notes[i + 1][0] if i + 1 < len(notes) else None
        legato = nxt is not None and abs(nxt - (bt + ln)) < 1e-6 and ln <= 0.25
        t1 = t0 + B(ln) * (1.0 if legato else 0.9)
        prev_end = notes[i - 1][0] + notes[i - 1][2] if i else -9
        slide = i > 0 and abs(prev_end - bt) < 1e-6 and notes[i - 1][2] <= 0.25
        LEAD_EV.append((t0, t1, m(note) + transpose, 0.6 * vel if ln <= 0.25 else 0.25 * vel, slide))


def pings(bar0, bar1, vel=0.5, every=2, pitches=('G6', 'Eb6')):
    """F3: the sigh as sonar pings, G then Eb a beat and a half later."""
    for b in range(bar0, bar1):
        if (b - bar0) % every:
            continue
        for k, (bt, note) in enumerate(((0, pitches[0]), (1.5, pitches[1]))):
            md = m(note)
            pn = 0.45 if k == 0 else -0.45
            put('ping', T(b, bt), lambda: X.ping(md, 1.0, 1.0, pn), vel * (1.0 if k == 0 else 0.8), length=6,
                key=('ping', md, pn))


# ---------------------------------------------------------- arrangement

# INTRO 0-7: string bed, sonar pings, the arp fades in under a closed filter,
# rim ticks and hats, the signal at bar 7
pad(0, 8, vel=0.5, a=3.0, cut=1000)
pings(0, 8, 0.55)
arp(2, 8, vel=0.8)
beat(4, 8, kick=0, snare=0, hats=0.18, rims=0.5, alt=True)
signal(4, 8, vel=0.6, cut=700)
put('fx', T(6), lambda: X8.noise_swell(T(8) - T(6), 300, 3000), 0.3, length=6)

# GROOVE 8-23: the electro break, the squelch bass, the arp, the signal
crash_at(8, 0.35)
beat(8, 24, kick=1.0, snare=0.9, hats=0.3, rims=0.45)
bass(8, 24)
arp(8, 24, vel=0.9)
signal(8, 24, vel=0.85, cut=1200)
pad(16, 24, vel=0.32, a=1.5, cut=1300)
toms(20, 24, 0.5)

# THEME A 24-39
crash_at(24, 0.4)
beat(24, 40, kick=1.0, snare=0.95, hats=0.32, rims=0.4)
bass(24, 40)
arp(24, 40, vel=0.85)
signal(24, 40, vel=0.85, cut=1500)
pad(24, 40, vel=0.34, a=0.8, cut=1500)
lead(24, THEME_A)
lead(32, THEME_A2)
tom_fill(39, 0.6)

# BREAKDOWN 40-47: no kick, the toms' taiko figure, pings, held bass
pad(40, 48, vel=0.5, a=1.2, cut=1300)
pings(40, 48, 0.6)
toms(40, 48, 0.55)
beat(44, 48, kick=0, snare=0, hats=0.16, rims=0.4)
for b in (40, 42, 44, 46):
    bass_hold(b, 8, vel=0.55, sub=0.6)
arp(44, 48, vel=0.6, every=2, gate=0.4)
stab_hit(40, 0, 'Db', 1.5, 0.6, 900)
stab_hit(44, 0, 'Ab', 1.5, 0.6, 900)

# BUILD 48-55: kick and bass back, the arp opens, a short silence before the drop
beat(48, 52, kick=1.0, snare=0, hats=0.22, rims=0.4)
beat(52, 56, kick=1.0, snare=0.85, hats=0.3, rims=0.4, stop=2.0)
bass(48, 56, vel=0.9, stop=2.0)
arp(48, 55, vel=0.9)
signal(48, 52, vel=0.7, cut=900)
pad(48, 56, vel=0.32, a=1.0, cut=1300, end=T(55, 2))
toms(48, 54, 0.4)
put('fx', T(52), lambda: X8.noise_swell(T(55, 2) - T(52), 300, 5000), 0.45, length=10)

# PEAK 1 56-71: theme B
crash_at(56, 0.5)
beat(56, 72, kick=1.0, snare=1.0, hats=0.34, rims=0.4)
bass(56, 72)
arp(56, 72, vel=1.0)
signal(56, 72, vel=0.9, cut=1700)
pad(56, 72, vel=0.38, a=0.6, cut=1700)
lead(56, THEME_B)
lead(64, THEME_B)
crash_at(64, 0.3)
tom_fill(71, 0.65)

# BRIDGE 72-79: the spy section; kick, rim and bass, the squelch opens
beat(72, 76, kick=1.0, snare=0, hats=0.24, rims=0.55)
beat(76, 80, kick=1.0, snare=0.85, hats=0.3, rims=0.45)
bass(72, 80, vel=1.0)
toms(72, 80, 0.45)
arp(76, 80, vel=0.8, every=2, gate=0.4)
pings(72, 80, 0.4, every=4)
pad(72, 80, vel=0.3, a=1.5, cut=1100)
signal(72, 80, vel=0.8, cut=1100)
tom_fill(79, 0.7)

# PEAK 2 80-95: theme A and theme B, everything
crash_at(80, 0.5)
beat(80, 96, kick=1.0, snare=1.0, hats=0.35, rims=0.42)
bass(80, 96)
arp(80, 96, vel=1.0)
signal(80, 96, vel=0.95, cut=1800)
pad(80, 96, vel=0.4, a=0.6, cut=1800)
lead(80, THEME_A2)
lead(88, THEME_B)
crash_at(88, 0.35)

# OUTRO 96-99: back to the intro's world; pings, arp, bass, kick drops out
beat(96, 98, kick=1.0, snare=0.8, hats=0.26, rims=0.45)
beat(98, 100, kick=0, snare=0, hats=0.2, rims=0.45)
bass(96, 100, vel=0.85)
arp(96, 100, vel=0.75)
pad(96, 100, vel=0.45, a=1.0, cut=1200)
pings(96, 100, 0.5)
signal(96, 100, vel=0.8, cut=1100)     # its held chord lands on the final hit

# FINAL HIT 100: C minor, rings out
place_kick(100, 0, 1.0)
put('sub', T(100), lambda: X.boom(1.0, 32.7 * 2), 0.6, length=4)
BASS_EV.append((T(100), T(100) + B(2.0), m('C2'), 1.0, False))
for note in ('C3', 'G3', 'Eb4', 'C4'):
    md = m(note)
    put('pad', T(100), lambda: X8.strings(md, B(2.5), 1.0, a=0.02, r=1.0, cut=1600, octave=0.12), 0.55, length=8)
put('ping', T(100), lambda: X.ping(m('G6'), 1.0, 1.2, 0.3), 0.5, length=6)
put('cym', T(100), lambda: X8.crash909(1.0, seed=3), 0.3, length=3)


# ------------------------------------------------------------- mono lines

bars_axis = (np.arange(N) / SR + OFF) / (4 * SPB)


def curve(pts):
    return np.exp(np.interp(bars_axis, [p[0] for p in pts], [np.log(p[1]) for p in pts]))


def rel_events(evs):
    out = []
    for t0, t1, md, ac, sl in sorted(evs, key=lambda e: e[0]):
        a, b = t0 - OFF, t1 - OFF
        if b <= 0 or a >= N / SR:
            continue
        out.append((max(a, 0.0), min(b, N / SR - 1e-3), md, ac, sl))
    return out


def render_lines():
    # squelch bass: 303-style saw through a resonant ladder, cutoff automated
    ev = rel_events(BASS_EV)
    if ev:
        cut = curve([(0, 160), (8, 170), (24, 210), (40, 120), (48, 130), (55, 260), (56, 230), (72, 230),
                     (76, 360), (80, 250), (96, 190), (101, 150)])
        res = np.interp(bars_axis, [0, 72, 76, 80, 101], [2.6, 2.6, 3.05, 2.7, 2.6])
        y = G.acid(ev, N, cut, res, envmod=2.9, decay=0.16, drive=1.7, wave='saw')
        bus['bass'].x += np.stack([y, y]).astype(np.float32)
    # cold arp: square through the ladder, low resonance, the filter opening per section
    ev = rel_events(ARP_EV)
    if ev:
        cut = curve([(0, 350), (2, 350), (8, 850), (24, 1100), (40, 700), (48, 600), (55, 2000), (56, 1400),
                     (72, 1400), (80, 1600), (96, 1000), (101, 550)])
        cut = cut * (1 + 0.25 * np.sin(2 * np.pi * bars_axis / 8))
        y = G.acid(ev, N, cut, 1.6, envmod=1.6, decay=0.07, drive=1.2, wave='square')
        bus['arp'].x += S.pan2(y, 0.0).astype(np.float32) * 1.2
    # cold lead: two detuned saw/pulse voices, light vibrato, fixed filter
    ev = rel_events(LEAD_EV)
    if ev:
        y = G.goa_lead(ev, N, 2500.0, 0.6, envmod=0.9, decay=0.25, detune=0.07, vib=0.05, width=0.4,
                       pulse=0.55)
        bus['lead'].x += y.astype(np.float32)


render_lines()

# ------------------------------------------------------------- mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_bass = S.sidechain(N, kicks, depth=0.8, release=0.2)
duck_mid = S.sidechain(N, kicks, depth=0.4, release=0.22)
duck_soft = S.sidechain(N, kicks, depth=0.2, release=0.25)

GAIN = {  # dB
    'kick': -1.0, 'snare': 1.0, 'hats': 15.0, 'rim': 8.0, 'perc': -2.0, 'cym': -10.0,
    'bass': -3.0, 'sub': -4.0, 'arp': -7.0, 'stab': 2.0, 'pad': 1.0, 'lead': 5.0, 'ping': -8.0,
    'fx': -12.0,
}
DUCK = {'bass': duck_bass, 'sub': duck_bass, 'arp': duck_mid, 'stab': duck_mid, 'pad': duck_soft,
        'lead': duck_soft, 'perc': duck_soft}
VERB = {'snare': .1, 'perc': .2, 'stab': .25, 'pad': .4, 'lead': .25, 'ping': .5, 'fx': .4, 'rim': .15,
        'arp': .12}
DELAY = {'stab': .5, 'arp': .25, 'lead': .2, 'ping': .7, 'rim': .2, 'perc': .12}

bus['stab'].x = stage(bus['stab'].x, HighpassFilter(200), PeakFilter(450, -3.0, 0.9), LowpassFilter(5000))
bus['pad'].x = X8.chorus(stage(bus['pad'].x, HighpassFilter(200), PeakFilter(500, -2.0, 0.8)), rate=0.35, depth_ms=2.4, mix=0.5)
bus['arp'].x = stage(bus['arp'].x, HighpassFilter(330), PeakFilter(520, -3.0, 0.8), PeakFilter(3000, 2.0, 0.9),
                      LowpassFilter(7000))
# arp: slow auto-pan so the 16ths move across the field
tt = (np.arange(N) / SR + OFF)
pl = (0.45 * np.sin(2 * np.pi * tt / (SPB * 8))).astype(np.float32)
bus['arp'].x[0] *= (1 - pl)
bus['arp'].x[1] *= (1 + pl)
del pl, tt
bus['lead'].x = X8.chorus(stage(bus['lead'].x, HighpassFilter(300), PeakFilter(650, -2.5, 0.8),
                                PeakFilter(3000, 2.5, 0.9),
                                LowpassFilter(7000)), rate=0.5, depth_ms=1.5, mix=0.3)
bus['hats'].x = S.butter(stage(bus['hats'].x, HighpassFilter(5000), HighShelfFilter(10000, -4.0, 0.7)), 10500,
                         'low', 4).astype(np.float32)
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(150))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(40), LowpassFilter(3500))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(160))
bus['kick'].x = stage(bus['kick'].x, HighpassFilter(30))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(100))
bus['ping'].x = stage(bus['ping'].x, LowpassFilter(5000))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(300))

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
bands = {}
TONAL = np.zeros((2, N), np.float32) if os.environ.get('NEW14_TONAL') else None
MUTE = set(filter(None, os.environ.get('NEW14_MUTE', '').split(',')))
BUSDIR = os.environ.get('NEW14_BUSDIR')
for k in names:
    x = bus[k].x
    bus[k] = None
    if k in MUTE:
        continue
    x = x * np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
    seg = x.T.astype(np.float64)
    if np.any(seg):
        # loudness while the bus plays (blocks of 0.4 s above -70 dBFS)
        blk = int(0.4 * SR)
        nb = seg.shape[0] // blk
        e = np.sqrt(np.mean(seg[:nb * blk].reshape(nb, blk, 2) ** 2, axis=(1, 2)))
        on = np.repeat(e > 10 ** (-60 / 20), blk)
        segp = seg[:nb * blk][on]
        levels[k] = pyln.Meter(SR).integrated_loudness(segp) if segp.shape[0] > SR else -99.0
    if os.environ.get('NEW14_BANDS') and np.any(seg):
        Y = np.abs(np.fft.rfft(seg[:, 0] + seg[:, 1])) ** 2
        f = np.fft.rfftfreq(seg.shape[0], 1 / SR)
        bands[k] = [10 * np.log10(Y[(f >= lo) & (f < 2 * lo)].sum() + 1e-9) for lo in
                    (20, 40, 80, 160, 315, 630, 1250, 2500, 5000, 10000)]
        del Y, f
    del seg
    mix += x
    if TONAL is not None and k not in ('kick', 'snare', 'hats', 'rim', 'perc', 'cym', 'fx', 'sub'):
        TONAL += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x

pre = int(0.02 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(350), LowpassFilter(7000),
             Reverb(room_size=0.85, damping=0.6, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
dly = X8.dub_delay(delay_send, B(0.75), feedback=0.48, repeats=8, lp=3000, hp=500)
del delay_send
mix = mix + verb * duck_soft[None, :] * 0.8 + dly * duck_mid[None, :] * 0.5
del verb, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -6.5), (7.9, -5.0), (8, -2.5), (23.9, -2.0), (24, -0.8), (39.9, -0.8),
              (40, -7.5), (47.9, -7.0), (48, -5.0), (55.5, -2.0), (56, 0.0), (71.9, 0.0),
              (72, -3.0), (79.9, -2.0), (80, 0.5), (95.9, 0.5), (96, -3.0), (99.9, -3.5),
              (100, -2.0), (110, -2.0)]
sec = np.interp(bars_axis, [p[0] for p in SECTION_DB], [p[1] for p in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec

# ---------------------------------------------------------- master
meter = pyln.Meter(SR)
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, PeakFilter(550, -1.5, 0.7), PeakFilter(3500, 1.5, 0.7), HighShelfFilter(9000, -1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

if not _bars:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
    end = min(last + 1, int((T(100) + 3.6) * SR))   # the final hit rings ~3.5 s
    mix = mix[:, :end]
    fade = int(1.2 * SR)
    mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32) ** 1.5

out = sys.argv[1] if len(sys.argv) > 1 else 'new14.wav'
if TONAL is not None:
    TONAL = TONAL[:, :mix.shape[1]]
    sf_.write(os.environ['NEW14_TONAL'], (TONAL / (np.abs(TONAL).max() + 1e-9) * 0.5).T, SR, subtype='FLOAT')
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS, before section gain):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
if bands:
    ref = max(v[1] for v in bands.values())
    print('bands     ' + ' '.join(f'{lo:>6d}' for lo in (20, 40, 80, 160, 315, 630, 1250, 2500, 5000, 10000)))
    for k, v in bands.items():
        print(f'  {k:8s}' + ' '.join(f'{x - ref:6.1f}' for x in v))
