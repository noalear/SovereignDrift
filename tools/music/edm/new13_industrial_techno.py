#!/usr/bin/env python3
# new13_industrial_techno.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Reactor Hall": new track 13 (tag TENSION), dark rolling industrial techno
# in the manner of Surgeon, Regis, Blawan and Ancient Methods: 130 BPM,
# B minor, a distorted but tight kick tuned to B1 with a rolling rumble tail
# that follows the root, metallic percussion, hydraulics, and one sparse motif
# from the originals over it.  Hypnotic, not peak-time: something approaching.
#
# Credits: the source music is Stardock's (the original Stellar Frontier game
# music, mid/sf-music8.mid and mid/sf-music3.mid).  Synthesized and mixed in
# code with tools/music/edm/synth.py, synth_new06.py and synth_new13.py, plus
# VSCO-2 Community Edition samples by Versilian Studios (CC0 1.0 / public
# domain, https://github.com/sgossner/VSCO-2-CE): anvil, brake drum, ratchet,
# tubular bells, gong scrape and gong smack.
#
# A NEW piece, not a remix.  Fragments taken (bar numbers are the original
# MIDIs', 1-based), all left out of song8_dubstep.py, song8_darkwave.py,
# new08_detroit.py, song3_liquid.py (except E, see there), new03_jungle.py and
# new10_ambient_techno.py:
#   A. sf-music8, drum channel, bars 21-36 (the tension passage): the hand
#      percussion, low conga on 1 and 2.5, open high conga on 2, cabasa 8ths
#      ("x...h.x........." per bar, first half only).  It is the metal
#      percussion engine of the whole track: the low conga is a pitched-down
#      anvil on a B2 pipe, the high conga a brake drum on an F#3 pipe, the
#      cabasa 8ths become hydraulic hisses.  Developed in the peaks with an
#      answer in the empty second half of the bar (beats 3.75 and 4.5).
#   B. sf-music8, drum channel, bar 28: the tom fill, high tom 16ths, hi-mid
#      16ths, low-mid 16ths, low tom 8ths (also bar 4 in sparser form) -> the
#      descending tuned-pipe fill (B4 A4 F#4 D4 ... B3) at phrase ends.
#   C. sf-music8, low choir (ch 6, Choir Aahs), bars 61-76: B2 | F#3 | G3 | F#3,
#      one note per bar -> the root movement of the rumble and drone in the
#      tension passages (B B F# F# G G F# F#, two bars a note) and the
#      breakdown (G -> F#, the b6 -> 5 pull).
#   D. sf-music3, flute (ch 2), bars 14-15: C5 . G5 . . F#5 F5 | E5, the
#      chromatic tail of the main melody, transposed to B minor:
#      D5 A5 G#5 G5 | F#5 (rhythm 2 + 1.5 + 1/4 + 1/4 | 3.5 beats) -> the
#      motif, an octave down on the steel lead, with an answer D A G F#
#      (natural sixth) over the G and F# bars, and augmented on tubular bells
#      in the breakdown and the drop-down.
#   E. sf-music3, strings (ch 5), bars 17-23: the repeated 8ths F5 x8 -> E5,
#      D5 x8 -> C5 (in song3_liquid.py only as a background pizzicato) ->
#      transposed to B: G x8 -> F#, E x8 -> D, as distorted B-minor dyad stabs
#      (B3+G4 ... B3+F#4) that rise out of the breakdown and drive the build.
# New: everything else -- the form (intro, groove, motif, breakdown, build,
# peak, drop-down, final peak, outro on a final B-minor hit that rings out and
# loops back into the B drone of the intro), the kick and rumble design,
# the 16th ticks, the round heavy snare on 2 and 4 (F#3 body, a trace of brake
# drum, from synth_new06.py), sound design and mix.
#
#   python3 tools/music/edm/new13_industrial_techno.py OUT.wav [BAR0 BAR1]
#   NEW13_MUTE=kick,rumble,metal,...  mutes buses (for the chroma check)
#   VSCO_DIR=/path/to/VSCO-2-CE       sample folder
import gc
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Chorus, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new13 as X  # noqa: E402  (sets synth_new06's sample folder)
import synth_new06 as I  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 130.0
SPB = 60.0 / BPM
NBARS = 104
FINAL = 102
TAIL = 0.6
BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
OFFSET = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)
MUTE = set(filter(None, os.environ.get('NEW13_MUTE', '').split(',')))


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - OFFSET


def B(beats):
    return beats * SPB


def active(bar):
    return BAR0 - 2 <= bar < BAR1 + 1


# ------------------------------------------------------------- harmony
# root per bar (fragment C gives the B / F# / G / F# movement)
TURN = ['B', 'B', 'F#', 'F#', 'G', 'G', 'F#', 'F#']
ROOTS = (['B'] * 8 +            # 0 intro
         ['B'] * 16 +           # 8 groove
         ['B'] * 8 + TURN +     # 24 motif
         ['G'] * 4 + ['F#'] * 4 +  # 40 breakdown
         ['B'] * 4 + ['G', 'G', 'F#', 'F#'] +  # 48 build
         ['B'] * 8 + TURN + ['B'] * 8 +        # 56 peak
         ['B'] * 8 +            # 80 drop-down
         TURN + ['B'] * 4 +     # 88 final peak
         ['B'] * 4)             # 100 outro, final hit at 102
assert len(ROOTS) == NBARS, len(ROOTS)
RUMBLE_MIDI = {'B': 35, 'G': 31, 'F#': 30}  # B1 G1 F#1
DRONE_MIDI = {'B': 47, 'G': 43, 'F#': 42}

# fragment D: the motif (2 bars) and its answer over G / F#
MOTIF = [(0, 'D5', 2), (2, 'A5', 1.5), (3.5, 'G#5', 0.25), (3.75, 'G5', 0.25), (4, 'F#5', 3.5)]
ANSWER = [(0, 'D5', 2), (2, 'A5', 1.5), (3.5, 'G5', 0.5), (4, 'F#5', 3.5)]
# fragment E: repeated 8ths falling a semitone / a tone (2 bars each)
PULSE_A = [(i * 0.5, ['B3', 'G4'], 0.22) for i in range(8)] + [(4, ['B3', 'F#4'], 3.0)]
PULSE_B = [(i * 0.5, ['B3', 'E4'], 0.22) for i in range(8)] + [(4, ['B3', 'D4'], 3.0)]

# --------------------------------------------------------------- buses

names = ['kick', 'rumble', 'metal', 'hyd', 'ticks', 'snare', 'toms', 'fx', 'lead', 'bell', 'stab',
         'drone', 'tex']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, sig, t, gain=1.0):
    if t + sig.shape[1] / SR < 0 or t > N / SR:
        return
    bus[name].add(sig, t, gain)


# ---------------------------------------------------------------- drums

_kv = [0]


def kick_at(bar, beat, vel=1.0):
    if not active(bar):
        return
    v = _kv[0] % 3
    _kv[0] += 1
    put('kick', cached(('k', v), lambda: X.kick(1.0, v)), T(bar, beat), vel)
    kicks.append(T(bar, beat))


def rumble_at(bar, beat, vel=1.0, beats=1.0, grit=1.0, decay=0.17):
    if not active(bar):
        return
    md = RUMBLE_MIDI[ROOTS[bar]]
    key = ('r', md, beats, round(grit, 2), decay, int(beat * 4) % 3)
    sig = cached(key, lambda: X.rumble(md, B(beats), 1.0, seed=md * 7 + int(beat * 4) % 3, grit=grit,
                                        decay=decay))
    put('rumble', sig, T(bar, beat), vel)


def four(bar0, bar1, vel=1.0, rumble=1.0, grit=1.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            kick_at(b, beat, vel * (1.0 if beat % 2 == 0 else 0.95))
            if rumble:
                rumble_at(b, beat, rumble * (1.0 if beat % 2 == 0 else 0.9), grit=grit)


_sn = [0]


def snare_at(bar, beat, vel=1.0):
    if not active(bar):
        return
    v = _sn[0] % 4
    _sn[0] += 1
    put('snare', cached(('s', v), lambda: I.heavy_snare(1.0, v)), T(bar, beat),
        vel * (0.94 + 0.06 * S.rng().random()))


def snares(bar0, bar1, vel=1.0, two=True):
    for b in range(bar0, bar1):
        if two:
            snare_at(b, 1, vel)
        snare_at(b, 3, vel * 1.03)


def ticks(bar0, bar1, vel=1.0):
    acc = [0.0, 0.35, 0.8, 0.4]
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for beat in range(4):
            for i, a in enumerate(acc):
                if a:
                    s = (b * 16 + beat * 4 + i) % 3
                    put('ticks', cached(('t', s), lambda: I.tick(1.0, 0.3 * (s - 1), s)),
                        T(b, beat + i * .25), vel * a)


def hyd(bar0, bar1, vel=1.0, onbeats=0.0):
    """Fragment A's cabasa 8ths as hydraulic hisses (offbeats strong)."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for k in range(8):
            off = k % 2 == 1
            a = (0.9 if k % 4 == 3 else 0.75) if off else onbeats
            if not a:
                continue
            v = (b * 8 + k) % 4
            pan = -0.35 if k % 4 == 1 else 0.35 if k % 4 == 3 else 0.0
            put('hyd', cached(('h', v, pan), lambda: X.hydraulic(1.0, v, pan)), T(b, k * 0.5), vel * a)


def low_clank(var):
    def fn():
        a = I.clank('anvil', -19, 1.0, var % 3, maxlen=0.5, lp=3000, decay=0.12)
        p = I.metal_hit(m('B2'), 1.0, decay=0.16, bright=0.5, seed=var)
        n = max(a.shape[1], p.shape[1])
        y = np.zeros((2, n), np.float32)
        y[:, :a.shape[1]] += a
        y[:, :p.shape[1]] += p * 0.8
        return y
    return cached(('lc', var), fn)


def hi_clank(var):
    def fn():
        a = I.clank('brake', -4, 1.0, var % 3, maxlen=0.4, lp=5500, decay=0.07)
        p = I.metal_hit(m('F#3'), 1.0, decay=0.1, bright=0.6, seed=10 + var)
        n = max(a.shape[1], p.shape[1])
        y = np.zeros((2, n), np.float32)
        y[:, :a.shape[1]] += a
        y[:, :p.shape[1]] += p * 0.6
        return y
    return cached(('hc', var), fn)


def metal(bar0, bar1, vel=1.0, answer=False):
    """Fragment A: low conga 1 & 2.5, high conga 2 (first half of the bar)."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        put('metal', S.pan2(low_clank(b % 3).mean(0), -0.2), T(b, 0.0), vel * 0.5)
        put('metal', S.pan2(hi_clank(b % 3).mean(0), 0.4), T(b, 1.0), vel * 0.75)
        put('metal', S.pan2(low_clank((b + 1) % 3).mean(0), -0.3), T(b, 1.5), vel * 0.8)
        if answer:
            put('metal', S.pan2(hi_clank((b + 2) % 3).mean(0), -0.45), T(b, 2.75), vel * 0.5)
            put('metal', S.pan2(low_clank((b + 2) % 3).mean(0), 0.35), T(b, 3.5), vel * 0.55)


# fragment B: the bar-28 fill, high/hi-mid/low-mid 16ths, low 8ths
FILL = [(0, 71), (.25, 71), (.5, 71), (.75, 71), (1, 69), (1.25, 69), (1.5, 69), (1.75, 69),
        (2, 66), (2.25, 66), (2.5, 66), (2.75, 66), (3, 62), (3.5, 59)]


def tom_fill(bar, vel=1.0, half=False):
    if not active(bar):
        return
    for i, (beat, md) in enumerate(FILL):
        if half and beat < 2:
            continue
        sig = cached(('pt', md), lambda: X.pipe_tom(md, 1.0, seed=md))
        put('toms', S.pan2(sig.mean(0), 0.4 - 0.06 * i), T(bar, beat), vel * (0.7 + 0.3 * i / len(FILL)))


def hit(bar, vel=1.0, gong=True):
    if not active(bar):
        return
    put('fx', S.impact(1.0), T(bar), 0.6 * vel)
    if gong:
        put('fx', cached(('gs',), lambda: I.gong_smack(-5.0, 1.0)), T(bar), 0.5 * vel)


def swell_into(bar, beats=4.0, vel=0.6):
    if active(bar):
        put('fx', I.rev_swell(B(beats)), T(bar) - B(beats), vel)


# ---------------------------------------------------------- tonal parts

def drone_span(bar0, bar1, vel=1.0):
    b = bar0
    while b < bar1:
        r = ROOTS[b]
        e = b
        while e < bar1 and ROOTS[e] == r:
            e += 1
        if active(b) or active(e - 1):
            n = int((T(e) - T(b) + 0.6) * SR)
            d = I.drone(DRONE_MIDI[r] - 12, n, 1.0, seed=b)
            t = np.arange(n) / SR
            fade = np.minimum(np.minimum(t / 0.4, 1), np.clip((n / SR - t) / 0.6, 0, 1))
            put('drone', d * fade[None, :].astype(np.float32), T(b), vel)
        b = e


def line(target, bar0, notes, transpose=0, vel=1.0, inst='lead', **kw):
    for beat, note, dur in notes:
        bar = bar0 + int(beat // 4)
        if not (active(bar) or active(bar + int(dur // 4))):
            continue
        md = m(note) + transpose
        if inst == 'lead':
            sig = X.steel_lead(md, B(dur) - 0.02, 1.0, **kw)
        elif inst == 'bell':
            sig = I.tubular(md, max(2.5, B(dur)), 1.0)
        else:
            raise ValueError(inst)
        put(target, sig, T(bar0, beat), vel)


def motif(bar, vel=1.0, transpose=-12, octave=0.0, **kw):
    """MOTIF over B, ANSWER over G / F#."""
    notes = MOTIF if ROOTS[bar] == 'B' else ANSWER
    line('lead', bar, notes, transpose, vel, **kw)
    if octave:
        line('lead', bar, notes, transpose + 12, vel * octave, cut=1800, env=2200)


def augmented(bar, vel=0.5, transpose=-12):
    notes = MOTIF if ROOTS[bar] == 'B' else ANSWER
    line('bell', bar, [(b * 2, n, d * 2) for b, n, d in notes], transpose, vel, 'bell')


def pulses(bar, pat, vel=1.0, cut=900.0, transpose=0):
    if not (active(bar) or active(bar + 1)):
        return
    for beat, notes, dur in pat:
        mids = tuple(m(x) + transpose for x in notes)
        sig = cached(('st', mids, dur, cut), lambda: X.stab(mids, B(dur), 1.0, cut=cut))
        put('stab', sig, T(bar, beat), vel * (1.0 if dur > 1 else (0.85 if beat % 1 else 1.0)))


# ---------------------------------------------------------- arrangement

# INTRO 0-7: B drone, gong scrape, the machine winds up; percussion, then the kick
drone_span(0, 8, 0.7)
if active(0):
    put('tex', I.gong_scrape(-6.0, 1.0), T(0), 0.4)
    put('tex', X.ratchet(-9.0, 1.0), T(2), 0.3)
metal(0, 8, 0.55)
hyd(2, 8, 0.45)
for b in range(4, 8):
    for beat in range(4):
        kick_at(b, beat, 0.8)
        rumble_at(b, beat, 0.55 + 0.08 * (b - 4), grit=0.6)
tom_fill(7, 0.5, half=True)

# GROOVE 8-23: the full machine, B pedal
hit(8, 0.5, gong=False)
four(8, 24, 1.0, 0.9, grit=0.8)
metal(8, 24, 0.85)
hyd(8, 24, 0.75)
ticks(12, 24, 0.7)
drone_span(8, 24, 0.3)
tom_fill(15, 0.6, half=True)
tom_fill(23, 0.8)
if active(16):
    put('tex', X.ratchet(-12.0, 1.0), T(16), 0.22)

# MOTIF 24-39: the steel lead enters (fragment D), snare on 4 then 2 and 4,
# the tension turn B B F# F# G G F# F# in the second half (fragment C)
hit(24, 0.6)
four(24, 40, 1.0, 1.0)
metal(24, 40, 0.9)
hyd(24, 40, 0.8)
ticks(24, 40, 0.8)
snares(24, 32, 0.8, two=False)
snares(32, 40, 0.85)
drone_span(24, 40, 0.3)
for b in (24, 28, 32, 36):
    motif(b, 0.95)
tom_fill(31, 0.8)
tom_fill(39, 1.0)

# BREAKDOWN 40-47: no kick; the rumble swells alone on G then F#, the motif
# augmented on bells, the pulse stabs (fragment E) rise from far away
hit(40, 0.6)
if active(40):
    put('tex', I.gong_scrape(-8.0, 1.0), T(40), 0.45)
    put('tex', X.ratchet(-10.0, 1.0), T(44), 0.3)
drone_span(40, 48, 0.75)
for b in range(40, 48):
    rumble_at(b, 0, 0.5, beats=2.0, grit=0.5, decay=0.5)
    rumble_at(b, 2, 0.4, beats=2.0, grit=0.5, decay=0.5)
augmented(40, 0.5)
metal(44, 48, 0.4)
hyd(40, 48, 0.4)
pulses(44, PULSE_A, 0.35, cut=450)
pulses(46, PULSE_A, 0.5, cut=600)

# BUILD 48-55: the kick returns (1 and 3, then four), the stabs drive
for b in range(48, 52):
    kick_at(b, 0, 0.9)
    kick_at(b, 2, 0.85)
    rumble_at(b, 0, 0.75, beats=2.0, decay=0.3)
    rumble_at(b, 2, 0.7, beats=2.0, decay=0.3)
four(52, 56, 0.95, 0.9)
metal(48, 56, 0.85)
hyd(48, 56, 0.6)
ticks(52, 56, 0.6)
drone_span(48, 56, 0.4)
pulses(48, PULSE_A, 0.6, cut=700)
pulses(50, PULSE_B, 0.65, cut=750)
pulses(52, PULSE_A, 0.75, cut=850)
pulses(54, PULSE_A, 0.85, cut=1000)
for b in range(54, 56):
    for beat in (1, 3):
        snare_at(b, beat, 0.6 + 0.1 * (b - 54))
tom_fill(55, 1.0)
swell_into(56, 4, 0.5)

# PEAK 56-79: everything; fragment A with its answer; motif with octave
hit(56, 1.0)
four(56, 80, 1.0, 1.0, grit=1.1)
metal(56, 80, 1.0, answer=True)
hyd(56, 80, 0.85, onbeats=0.3)
ticks(56, 80, 0.9)
snares(56, 80, 0.95)
drone_span(56, 80, 0.3)
for b in (56, 60, 64, 68, 72, 76):
    motif(b, 1.0, octave=0.3 if b >= 64 else 0.0)
for b in (62, 70, 78):
    pulses(b, PULSE_A if b != 70 else PULSE_B, 0.6, cut=1000)
hit(64, 0.6)
hit(72, 0.5, gong=False)
tom_fill(63, 0.9)
tom_fill(71, 1.0)
tom_fill(79, 1.0)

# DROP-DOWN 80-87: kick, rumble and metal; the motif augmented on bells
hit(80, 0.6, gong=True)
four(80, 88, 0.95, 0.85, grit=0.7)
metal(80, 88, 0.8)
hyd(80, 88, 0.55)
snares(84, 88, 0.7, two=False)
drone_span(80, 88, 0.5)
augmented(80, 0.45)
augmented(84, 0.4)
tom_fill(87, 1.0)
swell_into(88, 4, 0.5)

# FINAL PEAK 88-99: the tension turn under the motif and the pulses
hit(88, 1.0)
four(88, 100, 1.0, 1.0, grit=1.2)
metal(88, 100, 1.05, answer=True)
hyd(88, 100, 0.9, onbeats=0.35)
ticks(88, 100, 0.95)
snares(88, 100, 1.0)
drone_span(88, 100, 0.35)
for b in (88, 92, 96):
    motif(b, 1.0, octave=0.35)
pulses(90, PULSE_A, 0.65, cut=1100)
pulses(94, PULSE_A, 0.7, cut=1150)
pulses(98, PULSE_B, 0.65, cut=1100)
tom_fill(95, 1.0)
tom_fill(99, 0.9, half=True)

# OUTRO 100-101, final hit at 102
four(100, 102, 0.95, 0.9)
metal(100, 102, 0.8)
hyd(100, 102, 0.6)
snares(100, 102, 0.8, two=False)
drone_span(100, 102, 0.4)
if active(FINAL):
    kick_at(FINAL, 0, 1.0)
    hit(FINAL, 1.0)
    put('rumble', X.rumble(35, 3.0, 1.0, seed=99, grit=0.8, decay=0.9), T(FINAL), 1.0)
    put('metal', I.clank('anvil', -19, 1.0, 0, maxlen=2.0, lp=3500, decay=0.8), T(FINAL), 0.6)
    put('stab', X.stab((m('B2'), m('F#3'), m('B3'), m('D4')), B(2.0), 1.0, cut=1000), T(FINAL), 0.6)
    put('lead', X.steel_lead(m('F#4'), B(3.0), 1.0, r=0.8), T(FINAL), 0.6)
    put('bell', I.tubular(m('B4'), 3.5, 1.0), T(FINAL), 0.4)
    put('bell', I.tubular(m('F#4'), 3.5, 1.0), T(FINAL, 0.02), 0.3)
    n = int(4.0 * SR)
    d = I.drone(35, n, 1.0, seed=FINAL)
    t = np.arange(n) / SR
    put('drone', d * (np.exp(-t / 1.0) * np.minimum(t / 0.01, 1))[None, :].astype(np.float32), T(FINAL), 0.6)

_cache.clear()
gc.collect()

# ------------------------------------------------------------- mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_rumble = S.sidechain(N, kicks, depth=0.9, release=0.10)
duck_soft = S.sidechain(N, kicks, depth=0.3, release=0.2)

GAIN = {  # dB
    'kick': -3.0, 'rumble': -8.5, 'metal': 10.0, 'hyd': -1.0, 'ticks': 6.0, 'snare': -6.0, 'toms': -8.0,
    'fx': -12.0, 'lead': -9.5, 'bell': -6.0, 'stab': -10.0, 'drone': -4.0, 'tex': -8.0,
}
DUCK = {'rumble': duck_rumble, 'drone': duck_soft, 'stab': duck_soft, 'tex': duck_soft}
ROOM = {'snare': .25, 'metal': .3, 'toms': .25, 'hyd': .2, 'ticks': .1, 'kick': .03}
HALL = {'lead': .3, 'bell': .45, 'stab': .3, 'metal': .15, 'fx': .25, 'tex': .35, 'snare': .08,
        'drone': .2, 'toms': .15}
DELAY = {'lead': .3, 'stab': .25, 'metal': .08, 'bell': .15}

bus['rumble'].x = stage(bus['rumble'].x, HighpassFilter(30))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(200), PeakFilter(2200, 1.0, 0.8))
bus['ticks'].x = stage(bus['ticks'].x, LowpassFilter(10000))
bus['metal'].x = stage(bus['metal'].x, HighpassFilter(150), LowpassFilter(8500))
bus['hyd'].x = stage(bus['hyd'].x, HighpassFilter(300))
bus['drone'].x = stage(bus['drone'].x, HighpassFilter(100), LowpassFilter(4000))
bus['tex'].x = stage(bus['tex'].x, HighpassFilter(150), LowpassFilter(5000))
bus['fx'].x = stage(bus['fx'].x, LowpassFilter(8000))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(100))
bus['stab'].x = stage(bus['stab'].x, HighpassFilter(180), LowpassFilter(5000))

mix = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
kick_src = None
levels = {}
meter = pyln.Meter(SR)
for k in names:
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    bus[k].x = None
    if k in DUCK:
        x = x * DUCK[k][None, :]
    if k == 'kick':
        kick_src = x.mean(0).copy()
    if k in MUTE:
        continue
    seg = x.T.astype(np.float64)
    act = np.max(np.abs(seg), axis=1) > 1e-4
    levels[k] = meter.integrated_loudness(seg[act]) if act.sum() > SR else -99.0
    del seg
    mix += x
    if k in ROOM:
        room_send += x * ROOM[k]
    if k in HALL:
        hall_send += x * HALL[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    del x
gc.collect()

# reverb rumble: the kick through a dark, long room, saturated, low-passed, mono, ducked
if 'rumble' not in MUTE and 'kick' not in MUTE:
    kv = stage(np.stack([kick_src, kick_src]), HighpassFilter(35), LowpassFilter(400),
               Reverb(room_size=0.93, damping=0.35, wet_level=1.0, dry_level=0.0, width=0.0))
    kv = np.tanh(4.0 * kv.mean(0))
    kv = S.butter(S.butter(kv, 150, 'low', 4), 32, 'high', 2).astype(np.float32)
    kv *= duck_rumble * np.float32(10 ** (-13.0 / 20))
    seg = np.stack([kv, kv]).T.astype(np.float64)
    act = np.abs(seg[:, 0]) > 1e-4
    levels['krev'] = meter.integrated_loudness(seg[act]) if act.sum() > SR else -99.0
    del seg
    mix += kv[None, :]
    del kv
del kick_src

room = stage(room_send, HighpassFilter(200), LowpassFilter(6500),
             Reverb(room_size=0.35, damping=0.6, wet_level=1.0, dry_level=0.0, width=0.9))
del room_send
pre = int(0.025 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
del hall_send
hall = stage(hs, HighpassFilter(350), LowpassFilter(6000),
             Reverb(room_size=0.9, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del hs
dly = S.pingpong(delay_send, B(0.75), feedback=0.42, repeats=7, lp=3500, hp=400)
del delay_send
mix = mix + room * 0.8 + hall * 0.7 + dly * 0.5
del room, hall, dly
gc.collect()

# section dynamics (dB at bar lines, linear in between)
bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
SECTION_DB = [(0, -4), (7.9, -3), (8, -2.5), (23.9, -2.5), (24, -1.5), (39.9, -1.5),
              (40, -5.5), (47.9, -5), (48, -4), (55.9, -1.5), (56, 0), (79.9, 0), (80, -2.5), (87.9, -2.5),
              (88, 0.5), (99.9, 0.5), (100, -1.5), (101.9, -1.5), (102, 0), (NBARS, 0)]
sec = np.interp(bars_axis, [p[0] for p in SECTION_DB], [p[1] for p in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, PeakFilter(3200, 1.0, 0.7), HighShelfFilter(7000, -0.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=15, release_ms=120))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

if BAR1 == NBARS:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
    mix = mix[:, : last + 1]
fade = int(0.5 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new13_industrial_techno.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
