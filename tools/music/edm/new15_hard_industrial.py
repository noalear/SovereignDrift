#!/usr/bin/env python3
# new15_hard_industrial.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Hull Breach": a NEW peak-time hard industrial techno track (Perc, Ansome,
# I Hate Models, Rebekah) for the battle music set, 140 BPM, E-flat minor
# (E-flat Phrygian: the E-natural / C-flat colours of the source songs).
# It is not a remix: it is built from fragments of the original game music by
# Stardock (Stellar Frontier, 1997: mid/sf-music5.mid and mid/sf-music6.mid)
# that the existing scripts (song5_edm.py, song6_symphonic_metal.py,
# new05_goa.py, new06_industrial.py, new09_spacesynth.py) do not use.
# Bar numbers count from each MIDI's first downbeat (both files start with a
# two-beat pickup).  sf-music5 material is moved up a whole tone (C# -> Eb).
#
#   A. sf-music5, strings (ch "String Ensemble 1", track 6), bars 16-31: the
#      pickup figure C#4 on the "and" of 4, then G#4 G#4 G#4 on beats 3, 3-and,
#      4 of the next bar ->  Eb | Bb Bb Bb.  It is (1) the cell of the riff
#      ("pickup, then repeated fifth"), and (2) the rhythm of the tuned metal
#      clangs (Eb pipe on 4-and, three Bb pipes on 3, 3-and, 4).
#   B. sf-music5, strings (track 14), bars 57-71: the push under the electric
#      piano hook, G#4 -> C#5 C#5-- (a fourth up, then the repeated note held)
#      -> Bb3 Eb4 Eb4--, the other cell of the riff.  The riff (new) chains
#      B and A in 16ths and ends with the Phrygian E-natural (bar 1) or the
#      C-flat -> B-flat lament (bar 2) of sf-music6.
#   C. sf-music5, string stabs (tracks 18/21), bars 16-30 and 56-70: the
#      2-bar stab rhythm  x x . x | . . . . . . X-  with C#6/F#5 (then
#      C#6/G#4) and B5/F#5 on beat 3 of the second bar -> the gated chord
#      stabs Eb(sus4) / Eb5 and Db5, same rhythm, same voice-leading.
#   D. sf-music6, strings + timpani (tracks 23/32), bar 51: the B-flat octave
#      stab rhythm 0, .5(long), 1.5, 2(long), 3, 3.5 that kicks the second
#      half of the song -> the Bb4/Bb5 octave stab in the 4th bar of every
#      phrase of the second and third peaks.
#   Plus the source's E-major / E-flat-minor alternation (sf-music6) as the
#   bridge, where the whole machine moves up a semitone onto an E pedal for
#   four bars before falling back to E-flat.
# New: everything else -- the riff itself (built from cells A and B), the
# form (intro, groove, peak 1, breakdown, build, peak 2, E-pedal bridge,
# peak 3, outro, a final E-flat-minor hit that rings out), the 140-BPM drum
# programming, the tuned offbeat rumble, the sound design and the mix.
#
# Sounds: synthesized (tools/music/edm/synth.py, synth_new06.py,
# synth_new15.py), plus VSCO-2 Community Edition samples by Versilian Studios
# (CC0 1.0 / public domain, https://github.com/sgossner/VSCO-2-CE): anvil,
# brake drum, gong hit and gong scrape.  Original music (c) Stardock.
#
#   python3 tools/music/edm/new15_hard_industrial.py OUT.wav [BAR0 BAR1]
#   NEW15_MUTE=kick,snare,... mutes buses (for the chroma check)
#   VSCO_DIR=/path/to/VSCO-2-CE
import gc
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Chorus, Compressor, Limiter, HighpassFilter,
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_new15 as H  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 140.0
SPB = 60.0 / BPM
NBARS = 112
FINAL = 110
TAIL = 0.5
BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
OFFSET = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)
MUTE = set(filter(None, os.environ.get('NEW15_MUTE', '').split(',')))
S._rng = np.random.default_rng(2026)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - OFFSET


def B(beats):
    return beats * SPB


def active(bar):
    return BAR0 - 2 <= bar < BAR1 + 1


# ------------------------------------------------------------- material

# pedal per bar: Eb, except the bridge's E-pedal half (bars 80-83)
def pedal(bar):
    return 1 if 80 <= bar < 84 else 0


# the riff (16th steps): cell B (Bb Eb Eb--) + cell A (Eb | Bb Bb Bb) + ending
RIFF_1 = [(0, 'Bb3', 1), (1, 'Eb4', 1), (2, 'Eb4', 2), (5, 'Eb3', 1), (6, 'Bb3', 1), (7, 'Bb3', 1),
          (8, 'Bb3', 2), (11, 'Bb3', 1), (12, 'Eb4', 1), (13, 'Eb4', 1), (14, 'E4', 2)]
RIFF_2 = [(0, 'Bb3', 1), (1, 'Eb4', 1), (2, 'Eb4', 2), (5, 'Eb3', 1), (6, 'Bb3', 1), (7, 'Bb3', 1),
          (8, 'Bb3', 2), (11, 'Bb3', 1), (12, 'B3', 2), (14, 'Bb3', 2)]

# fragment C: stab chords (Eb sus4 / Eb5 alternating every 4 bars; Db5 answer)
STAB_X1 = ['Eb4', 'Ab4', 'Eb5']
STAB_X2 = ['Eb4', 'Bb4', 'Eb5']
STAB_Y = ['Db4', 'Ab4', 'Db5']
STAB_MIN = ['Eb4', 'Gb4', 'Bb4', 'Eb5']

# --------------------------------------------------------------- buses

names = ['kick', 'rumble', 'snare', 'hats', 'ride', 'metal', 'toms', 'fx', 'sub', 'pad',
         'stab', 'riff', 'bell', 'tex']
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

def kick_at(bar, beat, vel=1.0):
    if not active(bar):
        return
    v = int(bar * 4 + beat) % 2
    put('kick', cached(('k', v), lambda: H.hard_kick(1.0, v)), T(bar, beat), vel)
    kicks.append(T(bar, beat))


def four(bar0, bar1, vel=1.0, fills=True):
    for b in range(bar0, bar1):
        for beat in range(4):
            kick_at(b, beat, vel)
        if fills and (b - bar0) % 8 == 7:
            kick_at(b, 3.75, vel * 0.7)


_sn = [0]


def snare_at(bar, beat, vel=1.0):
    if not active(bar):
        return
    v = _sn[0] % 4
    _sn[0] += 1
    put('snare', cached(('s', v), lambda: H.I.heavy_snare(1.0, v, f0=155.56)), T(bar, beat),
        vel * (0.94 + 0.06 * S.rng().random()))


def backbeat(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        snare_at(b, 1, vel)
        snare_at(b, 3, vel)


def hats(bar0, bar1, vel=1.0, closed=True, opened=True):
    acc = [0.0, 0.35, 0.0, 0.45] if not opened else [0.25, 0.4, 0.0, 0.5]
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for beat in range(4):
            if closed:
                for i, a in enumerate(acc):
                    if a:
                        s = (b * 16 + beat * 4 + i) % 4
                        put('hats', cached(('h', s), lambda: H.dark_hat(1.0, False, 0.2 * (s - 1.5), s)),
                            T(b, beat + i * .25), vel * a)
            if opened:
                put('hats', cached(('ho',), lambda: H.dark_hat(1.0, True, -0.1, 9)), T(b, beat + .5), vel * 0.55)


def rides(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for beat in range(4):
            s = (b + beat) % 3
            put('ride', cached(('r', s), lambda: H.ride(1.0, 0.35, s)), T(b, beat + .5), vel * 0.6)
            put('ride', cached(('r', s), lambda: H.ride(1.0, 0.35, s)), T(b, beat), vel * 0.35)


def crash_at(bar, vel=1.0):
    if active(bar):
        put('ride', cached(('cr',), lambda: H.crash(1.0)), T(bar), vel)


# metal locked to the groove: brake clanks on the 16ths before beats 2 and 4
METAL = [(0.75, 0, 0.5), (2.75, 1, 0.45)]
METAL_PLUS = METAL + [(1.5, 2, 0.3), (3.25, 0, 0.28), (3.5, 1, 0.22)]


def metal(bar0, bar1, pattern=METAL, vel=1.0, anvil_every=4):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for beat, var, v in pattern:
            sig = cached(('c', var), lambda: H.I.clank('brake', -4 - 2 * var, 1.0, var, maxlen=0.35, lp=6500,
                                                       decay=0.09))
            pan = -0.5 if beat < 2 else 0.5
            put('metal', S.pan2(sig.mean(0), pan), T(b, beat), vel * v)
        if anvil_every and (b - bar0) % anvil_every == anvil_every - 1:
            put('metal', cached(('anv',), lambda: H.anvil(1.0, -14, 1)), T(b, 3.5), vel * 0.5)


def clangs(bar0, bar1, vel=1.0, shift=0):
    """Fragment A rhythm on tuned pipes: Eb on 4-and | Bb Bb Bb on 3, 3-and, 4."""
    for b in range(bar0, bar1, 2):
        if active(b):
            md = 63 + shift + pedal(b)
            put('metal', cached(('cl', md, 0), lambda: H.clang(md, 1.0, 0)), T(b, 3.5), vel * 0.8)
        if active(b + 1):
            md = 70 + shift + pedal(b + 1)
            for i, beat in enumerate((2, 2.5, 3)):
                put('metal', cached(('cl', md, i), lambda: H.clang(md, 1.0, i)), T(b + 1, beat),
                    vel * (0.6 + 0.12 * i))


def tom_fill(bar, start=2.0, notes=(51, 51, 46, 46, 43, 43, 39, 39), step=0.25, vel=0.9):
    if not active(bar):
        return
    for i, md in enumerate(notes):
        put('toms', H.tom(md + pedal(bar), 1.0, 0.4 - 0.1 * i), T(bar, start + i * step),
            vel * (0.8 + 0.2 * i / len(notes)))


def hit(bar, vel=1.0, gong=True):
    if not active(bar):
        return
    put('fx', cached(('imp',), lambda: H.impact(1.0)), T(bar), 0.7 * vel)
    if gong:
        put('fx', cached(('gong',), lambda: H.gong_hit(-3.0, 1.0)), T(bar), 0.5 * vel)


def riser_into(bar, beats=16, vel=0.5):
    if active(bar) or active(bar - beats // 4):
        put('fx', H.riser(B(beats) - B(1)), T(bar) - B(beats), vel)


def swell_into(bar, beats=4, vel=0.5):
    if active(bar):
        put('fx', H.rev_swell(B(beats)), T(bar) - B(beats), vel)


# ---------------------------------------------------------- the rumble

def rumble(bar0, bar1, mode='roll', vel=1.0, cut=260.0):
    """Tuned offbeat rumble between the kicks.  'off': one note on each offbeat;
    'roll': three 16ths after each kick (accent on the offbeat)."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        md = 27 + pedal(b)
        for beat in range(4):
            if mode == 'off':
                put('rumble', cached(('ru', md, 'o', cut), lambda: H.rumble_note(md, B(0.4), 1.0, cut=cut)),
                    T(b, beat + .5), vel)
            else:
                for i, a in ((1, 0.7), (2, 1.0), (3, 0.8)):
                    put('rumble', cached(('ru', md, i, cut), lambda: H.rumble_note(md, B(0.2), 1.0, cut=cut,
                                                                                   seed=i)),
                        T(b, beat + i * .25), vel * a)


# ---------------------------------------------------------- tonal parts

def riff(bar0, bar1, vel=1.0, transpose=0, cut=700.0, env=3800.0, target='riff', inst='riff'):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        cell = RIFF_1 if (b - bar0) % 2 == 0 else RIFF_2
        for step, note, ln in cell:
            md = m(note) + transpose + pedal(b)
            if inst == 'riff':
                sig = cached(('rf', md, ln, cut, env),
                             lambda: H.riff_synth(md, B(ln * .25) * 0.8, 1.0, cut=cut, env=env))
            put(target, sig, T(b, step * .25), vel * (1.0 if step in (0, 5, 8, 12) else 0.85))


def riff_half(bar0, nbars, vel=1.0, inst='bell'):
    """The riff at half speed (each 16th becomes an 8th) for the breakdown."""
    for k in range(0, nbars, 2):
        b = bar0 + k
        cell = RIFF_1 if (k // 2) % 2 == 0 else RIFF_2
        for step, note, ln in cell:
            bb = b + step // 8
            beat = (step % 8) * .5
            if not active(bb):
                continue
            md = m(note) + 12
            if inst == 'bell':
                sig = cached(('bl', md), lambda: H.bell(md, 1.6, 1.0))
                put('bell', sig, T(bb, beat), vel)
            else:
                sig = cached(('rfh', md, ln), lambda: H.riff_synth(md, B(ln * .5) * 0.9, 1.0, cut=500, env=1600,
                                                                   decay=0.09, drive=2.0))
                put('riff', sig, T(bb, beat), vel)


def stabs(bar0, bar1, vel=1.0, transpose=0, cut=1100.0):
    """Fragment C: x x . x | . . . . . . X-  (2 bars), chord X alternates every 4 bars."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        k = b - bar0
        tr = transpose + pedal(b)
        X = STAB_X1 if (k // 4) % 2 == 0 else STAB_X2
        if k % 2 == 0:
            for beat in (0, .5, 1.5):
                sig = cached(('st', tuple(X), tr, cut, .35),
                             lambda: H.gated_stab([m(n) + tr for n in X], B(.35), 1.0, cut=cut))
                put('stab', sig, T(b, beat), vel)
        else:
            sig = cached(('st', tuple(STAB_Y), tr, cut, .9),
                         lambda: H.gated_stab([m(n) + tr for n in STAB_Y], B(.9), 1.0, cut=cut))
            put('stab', sig, T(b, 3), vel * 0.9)


def octave_stab(bar, vel=1.0):
    """Fragment D: the Bb octave stab rhythm of sf-music6 bar 51."""
    if not active(bar):
        return
    tr = pedal(bar)
    for beat, ln in ((0, .5), (.5, 1), (1.5, .5), (2, 1), (3, .5), (3.5, .5)):
        sig = cached(('os', ln, tr), lambda: H.gated_stab([m('Bb4') + tr, m('Bb5') + tr], B(ln) * 0.8, 1.0,
                                                          cut=1500, env=3000))
        put('stab', sig, T(bar, beat), vel)


def pads(bar0, bar1, notes, vel=1.0, cut=800.0, attack=1.0):
    if not (active(bar0) or active(bar1 - 1)):
        return
    dur = T(bar1) - T(bar0)
    for note in notes:
        put('pad', H.dark_pad(m(note), dur, 1.0, cut=cut, a=attack), T(bar0), vel * 0.5)


def drone_span(bar0, bar1, vel=1.0, root=39):
    if not (active(bar0) or active(bar1 - 1)):
        return
    n = int((T(bar1) - T(bar0) + 0.6) * SR)
    d = H.drone(root - 12, n, 1.0, seed=bar0)
    t = np.arange(n) / SR
    fade = np.minimum(np.minimum(t / 0.4, 1), np.clip((n / SR - t) / 0.6, 0, 1))
    put('tex', d * fade[None, :].astype(np.float32), T(bar0), vel)


def sub_span(bar0, bar1, vel=1.0, md=27):
    if active(bar0) or active(bar1 - 1):
        put('sub', H.sub(md, T(bar1) - T(bar0) - 0.03), T(bar0), vel)


# ---------------------------------------------------------- arrangement

# INTRO A 0-7: kick, offbeat rumble, ticks; metal from bar 4
four(0, 8)
rumble(0, 8, 'off', 0.8, cut=200)
hats(0, 8, 0.6, closed=True, opened=False)
metal(4, 8, METAL[:1], 0.8, anvil_every=0)
drone_span(0, 8, 0.25)

# INTRO B 8-15: snare, rolling rumble, open hats, the pipe clangs (fragment A)
four(8, 16)
backbeat(8, 16, 0.8)
rumble(8, 16, 'roll', 0.9, cut=230)
hats(8, 16, 0.8)
metal(8, 16, METAL, 0.9)
clangs(8, 16, 0.7)
tom_fill(15, 3.0, (51, 46, 43, 39), 0.25, 0.7)

# GROOVE 16-31: gated stabs (fragment C); the riff teased, filtered, from 24
crash_at(16, 0.6)
four(16, 32)
backbeat(16, 32, 0.9)
rumble(16, 32, 'roll', 1.0)
hats(16, 32, 0.9)
metal(16, 32, METAL, 1.0)
clangs(16, 32, 0.6)
stabs(16, 32, 0.8, cut=900)
riff(24, 32, 0.55, cut=350, env=900)
rides(24, 32, 0.7)
swell_into(32, 4, 0.4)
tom_fill(31, 2.0)

# PEAK 1 32-47: the riff
hit(32, 0.8, gong=False)
crash_at(32, 0.9)
four(32, 48)
backbeat(32, 48, 1.0)
rumble(32, 48, 'roll', 1.0)
hats(32, 48, 1.0)
rides(32, 48, 0.9)
metal(32, 48, METAL_PLUS, 1.0)
clangs(32, 48, 0.55)
stabs(32, 48, 0.7, cut=1100)
riff(32, 48, 1.0)
crash_at(40, 0.6)
tom_fill(47, 2.0)

# BREAKDOWN 48-55: no beat; drone, pads on the stab harmony, the riff at half speed on bells
hit(48, 0.6)
if active(48):
    put('tex', H.gong_scrape(-6.0, 1.0), T(48), 0.4)
drone_span(48, 56, 0.5)
sub_span(48, 52, 0.45)
sub_span(52, 56, 0.45)
pads(48, 50, ['Eb3', 'Bb3', 'Gb4'], 0.8, cut=700, attack=1.5)
pads(50, 52, ['Eb3', 'Ab3', 'Eb4'], 0.8, cut=700, attack=0.8)
pads(52, 54, ['Eb3', 'Bb3', 'Gb4'], 0.8, cut=800, attack=0.8)
pads(54, 56, ['Db3', 'Ab3', 'Db4'], 0.8, cut=800, attack=0.8)
riff_half(48, 8, 0.5, 'bell')
for b in (49, 51, 53, 55):
    if active(b):
        put('metal', cached(('anv2',), lambda: H.anvil(1.0, -16, 2, maxlen=1.2)), T(b, 2.5), 0.4)

# BUILD 56-63: kick returns, rumble swells, the riff filtered opening, riser; one beat of silence
for b in range(56, 60):
    kick_at(b, 0, 0.85)
    kick_at(b, 2, 0.8)
four(60, 63, 0.95, fills=False)
for beat in range(3):
    kick_at(63, beat, 1.0)
rumble(56, 60, 'off', 0.7, cut=200)
rumble(60, 63, 'roll', 0.9, cut=240)
riff(56, 63, 0.6, cut=300, env=600)
hats(58, 63, 0.7, opened=False)
pads(56, 63, ['Eb3', 'Bb3', 'Gb4'], 0.6, cut=900, attack=1.0)
for b in range(56, 64, 2):
    clangs(b, b + 2, 0.4 + 0.05 * (b - 56))
riser_into(64, 32, 0.45)
tom_fill(63, 1.0, (51, 51, 46, 46, 43, 43, 39, 39), 0.25, 0.9)
# beat 4 of bar 63: silence before the drop (tom fill ends at 3.0)

# PEAK 2 64-79: the brutal return -- riff doubled, octave stab (fragment D) every 4th bar
hit(64, 1.0)
crash_at(64, 1.0)
four(64, 80)
backbeat(64, 80, 1.0)
rumble(64, 80, 'roll', 1.1, cut=280)
hats(64, 80, 1.0)
rides(64, 80, 1.0)
metal(64, 80, METAL_PLUS, 1.05, anvil_every=2)
clangs(64, 80, 0.5)
stabs(64, 80, 0.65, cut=1300)
riff(64, 80, 1.0, cut=800)
riff(64, 80, 0.35, 12, cut=1200, env=3000)
for b in range(67, 80, 4):
    octave_stab(b, 0.55)
crash_at(72, 0.7)
tom_fill(79, 2.0)

# BRIDGE 80-87: the E-major / E-flat shift -- everything up a semitone on an E pedal, then back
hit(80, 0.7, gong=True)
four(80, 88)
backbeat(80, 88, 0.9)
rumble(80, 88, 'roll', 1.0, cut=240)
hats(80, 88, 0.8, opened=False)
metal(80, 88, METAL_PLUS, 1.0, anvil_every=2)
clangs(80, 88, 0.8)
riff(80, 88, 0.75, -12, cut=500, env=2200)
drone_span(80, 84, 0.35, root=40)
drone_span(84, 88, 0.35, root=39)
stabs(84, 88, 0.6)
tom_fill(83, 2.0, (52, 52, 47, 47, 44, 44, 40, 40))
tom_fill(87, 1.0, (51, 51, 51, 51, 46, 46, 46, 46, 43, 43, 39, 39), 0.25, 1.0)
swell_into(88, 4, 0.5)

# PEAK 3 88-103: everything
hit(88, 1.0)
crash_at(88, 1.0)
four(88, 104)
backbeat(88, 104, 1.0)
rumble(88, 104, 'roll', 1.15, cut=290)
hats(88, 104, 1.0)
rides(88, 104, 1.0)
metal(88, 104, METAL_PLUS, 1.1, anvil_every=2)
clangs(88, 104, 0.55)
stabs(88, 104, 0.7, cut=1400)
riff(88, 104, 1.0, cut=850)
riff(88, 104, 0.38, 12, cut=1300, env=3200)
riff(96, 104, 0.4, -12, cut=450, env=1500)
for b in range(91, 104, 4):
    octave_stab(b, 0.6)
crash_at(96, 0.8)
tom_fill(103, 2.0)

# OUTRO 104-109: the machine strips down; final E-flat-minor hit at 110
crash_at(104, 0.6)
four(104, 110, fills=False)
backbeat(104, 108, 0.85)
rumble(104, 110, 'roll', 1.0, cut=240)
hats(104, 108, 0.8)
metal(104, 110, METAL, 0.9, anvil_every=0)
clangs(104, 110, 0.6)
riff(104, 108, 0.75, cut=500, env=1800)
stabs(104, 108, 0.55, cut=900)
tom_fill(109, 2.0, (51, 51, 46, 46, 43, 43, 39, 39), 0.25, 1.0)
if active(FINAL):
    kick_at(FINAL, 0, 1.0)
    hit(FINAL, 1.0)
    crash_at(FINAL, 0.7)
    put('metal', H.anvil(1.0, -14, 0, maxlen=2.0), T(FINAL), 0.5)
    put('stab', H.gated_stab([m(n) for n in STAB_MIN], B(2.5), 1.0, cut=1300, env=2500, rel=0.6), T(FINAL), 0.8)
    put('riff', H.riff_synth(m('Eb3'), B(2), 1.0, cut=600, env=2500, decay=0.12, r=0.5), T(FINAL), 0.8)
    put('pad', H.dark_pad(m('Eb3'), B(3), 1.0, cut=900, a=0.01, r=1.2), T(FINAL), 0.5)
    put('pad', H.dark_pad(m('Gb3'), B(3), 1.0, cut=900, a=0.01, r=1.2), T(FINAL), 0.35)
    put('pad', H.dark_pad(m('Bb3'), B(3), 1.0, cut=900, a=0.01, r=1.2), T(FINAL), 0.4)
    put('sub', H.sub(27, B(4), 1.0), T(FINAL), 0.9)
    put('rumble', H.rumble_note(27, B(3), 1.0, cut=220), T(FINAL, 0.25), 0.6)

_cache.clear()
gc.collect()

# ------------------------------------------------------------- mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_hard = S.sidechain(N, kicks, depth=0.95, release=0.10, power=1.5)
duck_mid = S.sidechain(N, kicks, depth=0.5, release=0.16)
duck_soft = S.sidechain(N, kicks, depth=0.3, release=0.2)

GAIN = {  # dB
    'kick': -3.0, 'rumble': -4.5, 'snare': -7.0, 'hats': 5.5, 'ride': 1.0, 'metal': 3.0, 'toms': -11.0,
    'fx': -12.0, 'sub': -6.0, 'pad': 2.0, 'stab': -8.0, 'riff': -3.0, 'bell': -1.0, 'tex': -5.0,
}
DUCK = {'rumble': duck_hard, 'sub': duck_hard, 'pad': duck_mid, 'tex': duck_mid, 'stab': duck_mid,
        'riff': duck_soft, 'bell': duck_soft}
ROOM = {'snare': .25, 'metal': .3, 'toms': .2, 'hats': .08, 'kick': .03}
HALL = {'riff': .12, 'bell': .45, 'pad': .3, 'stab': .3, 'metal': .15, 'fx': .25, 'tex': .3, 'snare': .1}
DELAY = {'riff': .08, 'bell': .3, 'metal': .08, 'stab': .15}

# notch the distortion's 5th harmonic of Eb (G, a major third) and soften its 9th (F)
bus['rumble'].x = stage(bus['rumble'].x, LowpassFilter(700), PeakFilter(196, -9.0, 4.0), PeakFilter(392, -6.0, 4.0),
                        PeakFilter(175, -3.0, 4.0))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(120), Chorus(rate_hz=0.3, depth=0.25, centre_delay_ms=9, mix=0.35))
# keep the pads mono below ~200 Hz (side channel high-passed)
_mid = (bus['pad'].x[0] + bus['pad'].x[1]) * 0.5
_side = S.butter((bus['pad'].x[0] - bus['pad'].x[1]) * 0.5, 200, 'high', 2).astype(np.float32)
bus['pad'].x = np.stack([_mid + _side, _mid - _side]).astype(np.float32)
del _mid, _side
bus['riff'].x = stage(bus['riff'].x, HighpassFilter(140), PeakFilter(1500, 1.5, 0.8))
bus['stab'].x = stage(bus['stab'].x, HighpassFilter(220), LowpassFilter(7000))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(4000), LowpassFilter(10000))
bus['ride'].x = stage(bus['ride'].x, HighpassFilter(2500), LowpassFilter(9000))
bus['metal'].x = stage(bus['metal'].x, HighpassFilter(250), LowpassFilter(8000))
bus['tex'].x = stage(bus['tex'].x, HighpassFilter(150), LowpassFilter(4500))
bus['fx'].x = stage(bus['fx'].x, LowpassFilter(8000))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(100))
bus['bell'].x = stage(bus['bell'].x, HighpassFilter(300), LowpassFilter(7000))

# rumble wash: the kick through a dark reverb, low-passed, driven, then ducked hard (between kicks)
kx = bus['kick'].x * np.float32(10 ** (GAIN['kick'] / 20))
wash = stage(kx, Reverb(room_size=0.75, damping=0.7, wet_level=1.0, dry_level=0.0, width=0.3), LowpassFilter(160))
wash = np.tanh(wash * 6.0).astype(np.float32)
wash = stage(wash, LowpassFilter(220), HighpassFilter(35))
del kx

mix = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
for k in names:
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    bus[k].x = None
    if k == 'rumble':
        x = x + wash * np.float32(10 ** (-14 / 20))
    if k in DUCK:
        x = x * DUCK[k][None, :]
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
del wash
gc.collect()

room = stage(room_send, HighpassFilter(200), LowpassFilter(7000),
             Reverb(room_size=0.35, damping=0.6, wet_level=1.0, dry_level=0.0, width=0.9))
del room_send
pre = int(0.02 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
del hall_send
hall = stage(hs, HighpassFilter(350), LowpassFilter(6500),
             Reverb(room_size=0.85, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del hs
dly = S.pingpong(delay_send, B(0.75), feedback=0.38, repeats=6, lp=4000, hp=500)
del delay_send
mix = mix + room * 0.8 + hall * 0.7 + dly * 0.5
del room, hall, dly
gc.collect()

# section dynamics (dB at bar lines, linear in between)
bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
SECTION_DB = [(0, -3), (7.9, -3), (8, -2.5), (15.9, -2.5), (16, -1.5), (31.9, -1.5), (32, 0), (47.9, 0),
              (48, -2.0), (55.9, -1.5), (56, -3.0), (63.9, -3), (64, 0.3), (79.9, 0.3), (80, -1.0), (87.9, -1.0),
              (88, 1.2), (103.9, 1.2), (104, -1.5), (109.9, -2.0), (110, 0.0), (112, 0.0)]
sec = np.interp(bars_axis, [p[0] for p in SECTION_DB], [p[1] for p in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(70, -1.0, 0.7), PeakFilter(55, -2.0, 1.0), PeakFilter(120, 1.5, 0.9), PeakFilter(3000, 1.5, 0.7), HighShelfFilter(9000, 0.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=15, release_ms=120))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

if BAR1 == NBARS:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
    mix = mix[:, : last + 1]
fade = int(0.4 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new15_hard_industrial.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
