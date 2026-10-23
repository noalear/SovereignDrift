#!/usr/bin/env python3
# new17_hybrid.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# "Iron Armada": a NEW piece of hybrid orchestral / industrial battle music
# (Homeworld / Freespace battle cues meets Junkie XL / Mick Gordon), 140 BPM,
# C minor, for the remake's BATTLE tag: sampled brass and string ostinati over
# a distorted synth bass and industrial drums (taikos, deep toms, metal hits, a
# round "taiko snare" -- no noisy snares).  Heroic but dark: a fleet engagement.
#
# Credits: the original music is Stardock's (Stellar Frontier, 1997,
# mid/sf-music4.mid and mid/sf-music7.mid).  Samples: VSCO-2 Community
# Edition by Versilian Studios (Sam Gossner, Simon Dalzell, Elan Hickler),
# CC0 1.0 / public domain, https://github.com/sgossner/VSCO-2-CE (violin,
# viola and cello sections, solo contrabass, F horn, trumpet, tenor trombone,
# tuba, giant ethnic drum, anvil, brake drum, gong, crash), played by
# synth_orch.Sampler.  All electronic sounds are synthesized (synth.py,
# synth_orch.py, synth_new06.py, synth_new17.py).
#
# It is not a remix.  It is built from fragments that the existing scripts for
# these songs (song4_chiptune.py, new04_bigbeat.py, song7_orchestral.py,
# new07_cinematic.py, new09_spacesynth.py) do not use, or use only as
# background, developed into new roles.  Bar numbers are each MIDI's own
# (sf-music4: 1-indexed bars, no pickup; sf-music7: counted from the bass's
# first downbeat after the two-beat pickup, as in song7_orchestral.py).
#
# From sf-music4.mid (C minor, 145 BPM) -- the main source:
#   F1  the pad layer's inner descent  C5 (6 beats) G#4 (2) | F4 (6) D#4 (2) |
#       G4 (16)   (prog 94 "pad layer", bars 33-36; song4 only has its octave
#       doubling in the strings as a background counter-line).  Here it is the
#       skeleton of the whole piece: its notes are the long notes of the brass
#       theme (C5 Ab4 F4 G4, then Eb5 C5 Ab4 B4 a third higher) and it implies
#       the new progression Cm - Ab - Fm - G; it is quoted verbatim by the solo
#       horn in the breakdown.
#   F2  the Voice Oohs rhythm cycle  (.25 1.25 1.75 | 0 .25 .75 1.25 1.75 2.25
#       3.25 3.75 | 2 2.25 2.75 3.25 3.75 | .25 1.25 1.75; ch 5, bars 5-64;
#       song4 and new04 use it as high pings).  Here, as a rhythm only: the
#       accent map of the 16th string ostinato and the pattern of the taiko
#       sticks; in the build it is played by the big taikos.
#   F3  the snare pickup into each phrase  (16ths on 3.5 3.75 -> crash on 1,
#       bars 19, 23, 27, 31, ...) -> the low-tom pickups and metal hits that
#       mark every phrase.
# From sf-music7.mid (C minor, 155 BPM) -- brass / fanfare material:
#   F4  the interlude's open-fifth brass call  C5+G5: 8th, 16th, 16th on beat 3,
#       4-7 beats held from the downbeat (ch 4 brass section, bars 19-33: the
#       quiet brass behind the woodwind interlude, not used by the remixes).
#       Its rhythm is the pickup of every cell of the brass theme; verbatim
#       (distant horns) in the intro; sequenced on open fifths Ab-Eb, Bb-F,
#       C-G, D-G in the bridge.
#   F5  the trombones' semitone climb  F4 | F4 | F#4 F#4 | F4 ... | G4 G4 (ch 6
#       trombone, bars 35-48, behind the F-minor/Gb section) -> the trombone
#       line of the "clash" section and of the build (Fm - Gb - Fm - G).
#   F6  the coda's trombone stabs  on 1, 1-and and 4-and (C3+G3 | D3+F3 |
#       Eb3+G3 ...; ch 4 trombones, bars 63-69) -> the stab rhythm of the low
#       brass, distorted bass and grind stabs in the clash and bridge.
#   F7  the final turn  D4 (1.5) -> Eb5 F5 | G5  (ch 3 brass, bar 70) -> the
#       trumpets' last lift into the final C-minor chord.
# New: everything else -- the form (intro, ostinato, theme, clash, breakdown,
# build, peak, half-time bridge, climax, outro, a final C-minor hit that rings
# out and loops back to the intro), the theme's moving notes and second
# phrase, the harmony beyond F1, the orchestration, the industrial drum
# programming, the distorted bass, sound design and mix.
#
#   python3 tools/music/edm/new17_hybrid.py OUT.wav [BAR0 BAR1]
#   NEW17_MUTE=kick,snare,taiko,metal,fx  mutes buses (chroma check)
#   NEW17_TONAL=path.wav also writes a dry mix of the tonal buses
#   VSCO_DIR / VSCO_ROOT = the VSCO-2-CE folder; NEW17_CACHE = sample index dir
import gc
import os
import sys
import tempfile

_here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _here)
if 'VSCO_ROOT' not in os.environ and 'VSCO_DIR' in os.environ:
    os.environ['VSCO_ROOT'] = os.environ['VSCO_DIR']
if 'VSCO_DIR' not in os.environ and 'VSCO_ROOT' in os.environ:
    os.environ['VSCO_DIR'] = os.environ['VSCO_ROOT']
if os.environ.get('NEW17_CACHE'):
    os.makedirs(os.environ['NEW17_CACHE'], exist_ok=True)
    tempfile.tempdir = os.environ['NEW17_CACHE']

import numpy as np  # noqa: E402
import soundfile as sf_  # noqa: E402
import pyloudnorm as pyln  # noqa: E402
import scipy.signal as ss  # noqa: E402
from pedalboard import (Pedalboard, Reverb, Compressor, Limiter, HighpassFilter,  # noqa: E402
                        LowpassFilter, LowShelfFilter, HighShelfFilter, PeakFilter)

import synth as S  # noqa: E402
import synth_orch as O  # noqa: E402
import synth_new17 as X  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 140.0
SPB = 60.0 / BPM
NBARS = 101           # bar 100 is the final hit
TAIL = 3.6
BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
OFFSET = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)
MUTE = set(filter(None, os.environ.get('NEW17_MUTE', '').split(',')))
RNG = np.random.default_rng(1717)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - OFFSET


def B(beats):
    return beats * SPB


def active(bar, beat=0.0):
    b = bar + beat / 4
    return BAR0 - 1.5 <= b < BAR1 + 0.5


def notes(spec):
    if isinstance(spec, (int, np.integer)):
        return [int(spec)]
    return [m(x) for x in spec.split('+')]


# ------------------------------------------------------------- harmony

CH = {   # name: (bass root midi, chord tones (pitch classes), string voicing)
    'Cm': (36, (0, 3, 7), ['C3', 'G3', 'C4', 'Eb4', 'G4']),
    'Ab': (32, (8, 0, 3), ['Ab2', 'Eb3', 'Ab3', 'C4', 'Eb4']),
    'Fm': (41, (5, 8, 0), ['F2', 'C3', 'F3', 'Ab3', 'C4']),
    'G': (31, (7, 11, 2), ['G2', 'D3', 'G3', 'B3', 'D4']),
    'Bb': (34, (10, 2, 5), ['Bb2', 'F3', 'Bb3', 'D4', 'F4']),
    'Gb': (42, (6, 10, 1), ['Gb2', 'Db3', 'Gb3', 'Bb3', 'Db4']),
}
PROG = ['Cm', 'Cm', 'Ab', 'Ab', 'Fm', 'Fm', 'G', 'G']
PROG2 = ['Cm', 'Cm', 'Ab', 'Ab', 'Fm', 'Bb', 'G', 'G']
CHORDS = (['Cm'] * 4 + ['Ab', 'Ab', 'G', 'G'] +             # 0 intro
          PROG +                                            # 8 ostinato
          PROG + PROG2 +                                    # 16 theme
          ['Fm', 'Fm', 'Gb', 'Gb', 'Fm', 'Fm', 'G', 'G'] +  # 32 clash
          PROG +                                            # 40 breakdown
          ['Fm', 'Fm', 'Gb', 'Gb', 'Fm', 'Gb', 'G', 'G'] +  # 48 build
          PROG + PROG2 +                                    # 56 peak
          ['Ab', 'Ab', 'Bb', 'Bb', 'Cm', 'Cm', 'G', 'G'] +  # 72 bridge
          PROG + PROG2 +                                    # 80 climax
          ['Cm', 'Cm', 'Ab', 'G'] + ['Cm'])                 # 96 outro, 100 final
assert len(CHORDS) == NBARS, len(CHORDS)


def chord_at(bar):
    return CHORDS[int(np.clip(bar, 0, NBARS - 1))]


def root_at(bar):
    return CH[chord_at(bar)][0]


def below(md, ch, gap=2):
    """Highest chord tone at least `gap` semitones under md (brass harmony)."""
    pcs = CH[ch][1]
    for k in range(gap, 13):
        if (md - k) % 12 in pcs:
            return md - k
    return md - 12


# ------------------------------------------------------------- themes
# (beat, note, length in beats); beats may be negative (pickups)

CALL = lambda b, n: [(b, n, 0.5), (b + 0.5, n, 0.25), (b + 0.75, n, 0.25)]   # F4 rhythm

THEME = (CALL(-1, 'G4') +
         [(0, 'C5', 4.5), (4.5, 'Bb4', 0.5), (5, 'C5', 1), (6, 'Eb5', 1)] + CALL(7, 'Ab4') +
         [(8, 'Ab4', 4.5), (12.5, 'G4', 0.5), (13, 'Ab4', 1), (14, 'C5', 1)] + CALL(15, 'F4') +
         [(16, 'F4', 4), (20, 'G4', 1), (21, 'Ab4', 1), (22, 'Eb4', 2)] +
         [(24, 'G4', 6)] + CALL(31, 'Eb5') +
         [(32, 'Eb5', 4.5), (36.5, 'D5', 0.5), (37, 'Eb5', 1), (38, 'G5', 1)] + CALL(39, 'C5') +
         [(40, 'C5', 4.5), (44.5, 'Bb4', 0.5), (45, 'C5', 1), (46, 'Eb5', 1)] + CALL(47, 'Ab4') +
         [(48, 'Ab4', 4), (52, 'Bb4', 2), (54, 'D5', 2)] +
         [(56, 'B4', 4), (60, 'D5', 3)])
# F1 verbatim (sf-music4 bars 33-36): the horn's quote in the breakdown
F1 = [(0, 'C5', 6), (6, 'Ab4', 2), (8, 'F4', 6), (14, 'Eb4', 2), (16, 'G4', 16)]
# F5: the trombones' semitone climb (sf-music7 bars 35-48), 2 bars per note
CLIMB = [(0, 'F4', 8), (8, 'F#4', 8), (16, 'F4', 8), (24, 'G4', 8)]
CLIMB_BUILD = [(0, 'F4', 8), (8, 'F#4', 8), (16, 'F4', 4), (20, 'F#4', 4), (24, 'G4', 8)]
# climax descant (violins)
DESCANT = [(0, 'G5', 8), (8, 'Ab5', 8), (16, 'Ab5', 8), (24, 'G5', 8),
           (32, 'G5', 8), (40, 'Ab5', 8), (48, 'Ab5', 4), (52, 'F5', 4), (56, 'G5', 8)]

# F2: the Voice Oohs rhythm cycle (sf-music4 ch 5), 4 bars
PING = [[.25, 1.25, 1.75], [0, .25, .75, 1.25, 1.75, 2.25, 3.25, 3.75],
        [2, 2.25, 2.75, 3.25, 3.75], [.25, 1.25, 1.75]]

# --------------------------------------------------------------- players

O._RAW_BUDGET = 300e6
O._HIT_BUDGET = 120e6
_KEEP = {'horn_sus', 'horn_stac', 'tbn_sus', 'tbn_stac', 'tpt_sus', 'tpt_stac', 'tuba_sus', 'tuba_stac',
         'vln_sus', 'vln_spic', 'vln_trem', 'vla_sus', 'vla_spic', 'vc_sus', 'vc_spic', 'vc_trem',
         'cb_sus', 'cb_spic'}
for _k in list(O.ARTS):
    if _k not in _KEEP:
        del O.ARTS[_k]
smp = O.Sampler(seed=17)

SEAT = {   # pan, width, bus
    'vln1': (-0.55, 0.6, 'strhi'), 'vln2': (-0.3, 0.6, 'strhi'), 'vla': (0.1, 0.55, 'strhi'),
    'vc': (0.4, 0.5, 'strlo'), 'cb': (0.55, 0.35, 'strlo'),
    'horn': (-0.3, 0.5, 'horn'), 'horn2': (-0.12, 0.5, 'horn'),
    'tpt': (0.28, 0.35, 'tpt'), 'tbn': (0.18, 0.4, 'lowbr'), 'tuba': (0.32, 0.3, 'lowbr'),
}
ARTS = {
    'vln1': {'sus': 'vln_sus', 'short': 'vln_spic', 'trem': 'vln_trem'},
    'vln2': {'sus': 'vln_sus', 'short': 'vln_spic', 'trem': 'vln_trem'},
    'vla': {'sus': 'vla_sus', 'short': 'vla_spic'},
    'vc': {'sus': 'vc_sus', 'short': 'vc_spic', 'trem': 'vc_trem'},
    'cb': {'sus': 'cb_sus', 'short': 'cb_spic'},
    'horn': {'sus': 'horn_sus', 'short': 'horn_stac'}, 'horn2': {'sus': 'horn_sus', 'short': 'horn_stac'},
    'tpt': {'sus': 'tpt_sus', 'short': 'tpt_stac'},
    'tbn': {'sus': 'tbn_sus', 'short': 'tbn_stac'}, 'tuba': {'sus': 'tuba_sus', 'short': 'tuba_stac'},
}
BUSES = ['strhi', 'strlo', 'horn', 'tpt', 'lowbr', 'choir', 'bass', 'sub', 'synth',
         'kick', 'snare', 'taiko', 'metal', 'fx']
bus = {k: S.Bus(N) for k in BUSES}
KICKS = []
HUM = [0.008, 0.06]
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def put(name, sig, t, gain=1.0):
    if t + sig.shape[1] / SR < 0 or t > N / SR:
        return
    bus[name].add(sig, t, gain)


def play(player, mel, bar0, transpose=0, vel=0.7, art='auto', short_max=0.5, rel=None,
         attack=0.0, legato=True, harm=False, swell=None, gain=1.0):
    """Note list on a seated (sampled) player.  art 'auto': short articulation
    up to short_max beats, sustain otherwise.  harm: add the next chord tone
    below on notes of a beat or longer (brass harmony)."""
    pan, width, bname = SEAT[player]
    arts = ARTS[player]
    mel = sorted(mel, key=lambda e: e[0])
    for i, ev in enumerate(mel):
        beat, spec, ln = ev[0], ev[1], ev[2]
        acc = ev[3] if len(ev) > 3 else 1.0
        bar = bar0 + int(np.floor(beat / 4))
        if not (active(bar0, beat) or active(bar0, beat + ln)):
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
        dur = B(ln) * (0.8 if is_short else 1.0) + (0.06 if nxt_leg else 0.0)
        r = rel if rel is not None else (0.1 if is_short else (0.07 if nxt_leg else 0.4))
        mds = [md + transpose for md in notes(spec)]
        if harm and ln >= 1:
            mds.append(below(mds[0], chord_at(bar)))
        for j, md in enumerate(mds):
            v = float(np.clip(vel * acc * (1 + RNG.normal(0, HUM[1])) * (0.85 if j else 1.0), 0.02, 1.0))
            sig = smp.note(key, md, max(dur, 0.05), v, rel=r, attack=attack, legato=leg)
            if swell:
                k = min(int(dur * SR), sig.shape[1])
                u = np.linspace(0, 1, k, dtype=np.float32)
                sig[:, :k] *= swell + (1 - swell) * u ** 1.5
            t = T(bar0, beat) + (0 if leg else RNG.normal(0, HUM[0]))
            t = max(t, T(bar0, beat) - 0.015)
            put(bname, O.seat(sig, pan, width), t, gain * (0.8 if j else 1.0))


def chords(player, bar0, bar1, vel=0.4, pick=None, transpose=0, attack=0.3, art='sus', swell=None, gain=1.0):
    """Sustained chords, held while the chord stays."""
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        v = CH[c][2]
        if pick is not None:
            v = [v[j] for j in pick if j < len(v)]
        play(player, [(0, '+'.join(v), (e - b) * 4)], b, transpose, vel, art, attack=attack, legato=False,
             swell=swell, gain=gain)
        b = e


# string ostinato: 16ths on chord tones, accents from the F2 rhythm cycle
OST_SHAPE = [2, 1, 3, 1]          # indices into (root, fifth, octave, tenth)


def ost_tones(ch):
    r = CH[ch][0]
    pcs = CH[ch][1]
    third = (pcs[1] - pcs[0]) % 12
    base = r % 12 + 60                  # G3 .. F#4
    if base > 66:
        base -= 12
    return [base, base + 7, base + 12, base + 12 + third]


def strings_ost(bar0, bar1, vel=0.6, sixteenths=True, accent=0.35, octave=0, low=True, low_vel=0.6):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        ch = chord_at(b)
        tn = ost_tones(ch)
        acc = set(PING[b % 4])
        steps = 16 if sixteenths else 8
        for i in range(steps):
            beat = i * (4 / steps)
            a = 1.0 if beat in acc else (1 - accent)
            md = tn[OST_SHAPE[i % 4]] + 12 * octave
            play('vln1', [(beat, md, 0.25)], b, vel=vel * a, art='short', legato=False)
            if i % 2 == 0:
                vl = tn[0] - 12 if tn[0] - 12 >= 48 else tn[0]
                play('vla', [(beat, vl + (7 if i % 4 == 2 else 0), 0.5)], b, vel=vel * 0.8 * a,
                     art='short', legato=False)
        if low:
            lo = CH[ch][0] % 12 + 36
            cbn = lo - 12 if lo - 12 >= 28 else lo
            for i in range(8):
                beat = i * 0.5
                a = 1.0 if (beat in acc or i % 4 == 0) else 0.7
                play('vc', [(beat, lo, 0.5)], b, vel=low_vel * a, art='short', legato=False)
                if i % 2 == 0:
                    play('cb', [(beat, cbn, 0.5)], b, vel=low_vel * 0.9 * a, art='short', legato=False)


# ------------------------------------------------------------------ bass

GALLOP = [0, None, 0, 0, 0, None, 0, 0, 0, None, 0, 0, 12, None, 0, 0]
DRIVE16 = [0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 12, 0, 12]
STAB = [(0, 0.4), (0.5, 0.4), (3.5, 0.4)]          # F6 rhythm


def sub_span(bar0, bar1, vel=0.8):
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        if active(b) or active(e - 1):
            r = CH[c][0]
            sr = r if r <= 39 else r - 12
            put('sub', X.sub(sr, B((e - b) * 4) - 0.03), T(b), vel)
        b = e


def bass16(bar0, bar1, pattern=GALLOP, vel=1.0, sub_vel=0.8):
    sub_span(bar0, bar1, sub_vel)
    for b in range(bar0, bar1):
        if not active(b):
            continue
        r = CH[chord_at(b)][0]
        for i, st in enumerate(pattern):
            if st is None:
                continue
            md = r + st
            acc = 1.0 if i % 4 == 0 else 0.82
            put('bass', cached(('db', md), lambda: X.dist_bass(md, B(0.25) * 0.85)), T(b, i * 0.25), vel * acc)


def stabs(bar0, bar1, vel=1.0, brass=True, sub_vel=0.8, last_bar_full=True):
    """F6: stabs on 1, 1-and, 4-and: low brass + distorted bass + grind."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        ch = chord_at(b)
        r = CH[ch][0]
        v = CH[ch][2]
        for beat, ln in STAB:
            put('bass', cached(('db2', r), lambda: X.dist_bass(r, B(ln))), T(b, beat), vel)
            put('sub', cached(('sb', r), lambda: X.sub(r if r <= 39 else r - 12, B(ln))), T(b, beat), sub_vel)
            put('synth', cached(('gs', ch), lambda: X.grind_stab([m(v[1]), m(v[2]), m(v[3])], B(ln))),
                T(b, beat), 0.3 * vel)
            if brass:
                play('tbn', [(beat, '+'.join(v[1:3]), ln)], b, vel=0.85 * vel, art='short', legato=False)
                play('tuba', [(beat, m(v[0]), ln)], b, vel=0.8 * vel, art='short', legato=False)
                play('horn2', [(beat, '+'.join(v[2:4]), ln)], b, vel=0.75 * vel, art='short', legato=False)


def synth_pulse(bar0, bar1, vel=0.5):
    """Filtered saw pulse doubling the root in 16ths (the hybrid layer)."""
    for b in range(bar0, bar1):
        if not active(b):
            continue
        r = CH[chord_at(b)][0] + 12
        while r > 50:
            r -= 12
        for i in range(16):
            md = r + (12 if i % 4 == 2 else 0)
            put('synth', cached(('pl', md), lambda: O.pulse(md, B(0.22), 1.0, cut=500, env=1400, decay=0.05)),
                T(b, i * 0.25), vel * (1.0 if i % 4 == 0 else 0.75))


# ---------------------------------------------------------------- drums

_rr = {'k': 0, 's': 0}


def kick_at(bar, beat, vel=1.0):
    if not active(bar, beat):
        return
    v = _rr['k'] % 3
    _rr['k'] += 1
    put('kick', cached(('k', v), lambda: X.kick(1.0, v)), T(bar, beat), vel)
    KICKS.append(T(bar, beat))


def snare_at(bar, beat, vel=1.0):
    if not active(bar, beat):
        return
    v = _rr['s'] % 4
    _rr['s'] += 1
    put('snare', cached(('s', v), lambda: X.taiko_snare(1.0, v)), T(bar, beat) + RNG.normal(0, 0.002),
        vel * (0.95 + 0.05 * RNG.random()))


def taiko_at(bar, beat, vel=0.9, src='taiko', pan=0.0):
    if not active(bar, beat):
        return
    f0 = 65.41 if src == 'taiko' else 98.0
    put('taiko', O.seat(X.taiko(vel, f0, src), pan, 0.5), T(bar, beat) + RNG.normal(0, 0.003), 1.0)


def tom_at(bar, beat, md, vel=0.8, pan=0.0):
    if not active(bar, beat):
        return
    s = int(RNG.integers(0, 4))
    put('taiko', S.pan2(cached(('tom', md, s), lambda: X.tom(md, 1.0, s)).mean(0), pan), T(bar, beat), vel)


def metal_at(bar, beat, kind, vel=0.6, pan=0.0, var=None):
    if not active(bar, beat):
        return
    v = int(RNG.integers(0, 3)) if var is None else var
    sig = cached(('mt', kind, v), lambda: X.metal(kind, 1.0, v))
    put('metal', S.pan2(sig.mean(0), pan), T(bar, beat), vel)


def ticks(bar0, bar1, vel=0.5, sixteenth=False):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        steps = 16 if sixteenth else 8
        for i in range(steps):
            beat = i * 4 / steps
            a = [0.5, 0.25, 1.0, 0.3][i % 4] if sixteenth else (1.0 if i % 2 else 0.45)
            s = i % 3
            put('metal', cached(('t', s), lambda: X.tick(1.0, 0.3 * (s - 1), s)), T(b, beat), vel * a)


def pickup(bar, vel=0.8, crash=True):
    """F3: 16ths on 3.5 3.75 -> crash on the next 1 (here: deep toms + metal)."""
    tom_at(bar, 3.5, 43, vel * 0.8, -0.3)
    tom_at(bar, 3.75, 38, vel, 0.3)
    if crash:
        cym(bar + 1, 0, 0.5 * vel)


def tom_fill(bar, start=2.0, seq=(45, 45, 43, 43, 41, 41, 38, 36), step=0.25, vel=0.85):
    for i, md in enumerate(seq):
        tom_at(bar, start + i * step, md, vel * (0.75 + 0.25 * i / len(seq)), 0.5 - i / len(seq))


def cym(bar, beat=0.0, vel=0.5):
    if active(bar, beat):
        put('fx', O.seat(O.perc('crash', vel), 0.1, 1.0), T(bar, beat), 0.8)


def gong(bar, vel=0.7):
    if active(bar):
        put('fx', O.seat(O.perc('gong', vel, maxlen=8.0), -0.1, 0.8), T(bar), 0.7)


def hit(bar, vel=1.0, braam=True, ch=None):
    """Section downbeat: taiko + boom + braam (+ crash)."""
    if not active(bar):
        return
    taiko_at(bar, 0, min(1.0, vel), pan=-0.1)
    put('fx', O.boom(1.0), T(bar), 0.55 * vel)
    if braam:
        c = ch or chord_at(bar)
        r = CH[c][0]
        put('synth', O.braam([r, r + 7, r + 12], B(3)), T(bar), 0.45 * vel)
    cym(bar, 0, 0.6 * vel)


def groove_a(bar0, bar1, vel=1.0, sticks=True, metal=True):
    """Main battle groove: kick 1, 2-and-a.. ; taiko snare on 2 and 4; big taiko
    on 1 and 3-and; taiko sticks on the F2 cycle; metal on the offbeats."""
    for b in range(bar0, bar1):
        kick_at(b, 0, vel)
        kick_at(b, 1.75, 0.75 * vel)
        kick_at(b, 2.5, 0.9 * vel)
        if b % 2 == 1:
            kick_at(b, 3.75, 0.7 * vel)
        snare_at(b, 1, 0.95 * vel)
        snare_at(b, 3, vel)
        taiko_at(b, 0, 0.85 * vel, pan=-0.15)
        taiko_at(b, 2.5, 0.7 * vel, pan=0.15)
        if sticks:
            for bt in PING[b % 4]:
                taiko_at(b, bt, 0.35 * vel, 'taiko_stick', pan=0.35 if bt < 2 else -0.35)
        ticks(b, b + 1, 0.3 * vel)
        if metal:
            metal_at(b, 0.75, 'brake', 0.5 * vel, -0.4)
            metal_at(b, 2.75, 'brake', 0.45 * vel, 0.4)
            if b % 4 == 3:
                metal_at(b, 3.5, 'anvil', 0.55 * vel, 0.0)


def groove_b(bar0, bar1, vel=1.0):
    """Peak: industrial four-on-the-floor under the taikos."""
    for b in range(bar0, bar1):
        for bt in range(4):
            kick_at(b, bt, vel * (1.0 if bt % 2 == 0 else 0.9))
        kick_at(b, 2.75, 0.6 * vel)
        snare_at(b, 1, 0.95 * vel)
        snare_at(b, 3, vel)
        if b % 2 == 1:
            snare_at(b, 3.75, 0.3 * vel)
        taiko_at(b, 0, 0.9 * vel, pan=-0.15)
        taiko_at(b, 1.5, 0.55 * vel, pan=0.2)
        taiko_at(b, 2.5, 0.75 * vel, pan=0.15)
        for bt in PING[b % 4]:
            taiko_at(b, bt, 0.38 * vel, 'taiko_stick', pan=0.35 if bt < 2 else -0.35)
        metal_at(b, 0.75, 'brake', 0.5 * vel, -0.4)
        metal_at(b, 2.25, 'stamp', 0.45 * vel, 0.3)
        ticks(b, b + 1, 0.35 * vel, sixteenth=True)
        metal_at(b, 2.75, 'brake', 0.45 * vel, 0.4)
        if b % 2 == 1:
            metal_at(b, 3.5, 'anvil', 0.6 * vel, 0.0)


def halftime(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        kick_at(b, 0, vel)
        kick_at(b, 0.75, 0.6 * vel)
        kick_at(b, 2.5, 0.8 * vel)
        snare_at(b, 2, vel)
        taiko_at(b, 0, 0.9 * vel)
        if b % 2 == 1:
            taiko_at(b, 3, 0.6 * vel, pan=0.2)
            taiko_at(b, 3.5, 0.7 * vel, pan=-0.2)
        metal_at(b, 1.5, 'brake', 0.45 * vel, -0.4)
        metal_at(b, 3.5, 'anvil', 0.5 * vel, 0.3)


def choir_span(bar0, bar1, vel=0.4, attack=0.8):
    b = bar0
    while b < bar1:
        c = chord_at(b)
        e = b + 1
        while e < bar1 and chord_at(e) == c:
            e += 1
        if active(b) or active(e - 1):
            for nt in CH[c][2][1:4]:
                md = m(nt) + 12
                put('choir', O.choir(md, B((e - b) * 4), 1.0, attack=attack), T(b), vel)
        b = e


# ---------------------------------------------------------- arrangement

# INTRO 0-7: low strings tremolo on C, distant horns play F4 verbatim,
# taikos, metal, the distorted bass creeps in
hit(0, 0.7)
gong(0, 0.5)
play('vc', [(0, 'C3', 16)], 0, vel=0.45, art='trem', legato=False, attack=1.2, swell=0.2)
play('cb', [(0, 'C2', 16)], 0, vel=0.5, legato=False, attack=1.0)
chords('vc', 4, 8, vel=0.4, pick=[0, 1], art='trem', attack=0.4)
chords('cb', 4, 8, vel=0.45, pick=[0], transpose=-12, attack=0.3)
play('horn', CALL(7, 'C5') + [(8, 'C5', 7)], 0, transpose=-12, vel=0.5)
play('horn2', CALL(7, 'G4') + [(8, 'G4', 7)], 0, transpose=-12, vel=0.45)
play('horn', CALL(15, 'Eb5') + [(16, 'Eb5', 8), (24, 'D5', 8)], 0, transpose=-12, vel=0.55)
play('horn2', CALL(15, 'Ab4') + [(16, 'Ab4', 8), (24, 'B4', 8)], 0, transpose=-12, vel=0.5)
sub_span(0, 8, 0.55)
for b in range(4, 8):
    for i in range(0, 16, 2):
        r = CH[chord_at(b)][0]
        put('bass', cached(('db', r), lambda: X.dist_bass(r, B(0.25) * 0.85)), T(b, i * 0.25), 0.55 + 0.05 * (b - 4))
for b in range(0, 8):
    taiko_at(b, 0, 0.55 + 0.04 * b)
    if b >= 2:
        taiko_at(b, 2.5, 0.45 + 0.04 * b, pan=0.2)
    if b % 2 == 1:
        metal_at(b, 3.5, 'anvil', 0.4, 0.0)
for b in range(4, 8):
    for bt in PING[b % 4]:
        taiko_at(b, bt, 0.3, 'taiko_stick', pan=0.3)
tom_fill(7, 2.0)
if active(7):
    put('fx', X.riser(B(4)), T(7), 0.35)

# OSTINATO 8-15: strings 16ths (F2 accents), bass gallop, main groove
hit(8, 0.85)
strings_ost(8, 16, vel=0.62)
bass16(8, 16, GALLOP, 0.95, 0.8)
synth_pulse(12, 16, 0.35)
groove_a(8, 16, 0.92)
chords('horn', 8, 16, vel=0.35, pick=[1, 2], attack=0.5)
chords('tbn', 12, 16, vel=0.35, pick=[0, 1], attack=0.3)
pickup(11, 0.7, crash=False)
tom_fill(15, 2.0)

# THEME 16-31: the brass theme (F1 + F4) in horns, trombones harmonize
hit(16, 1.0)
strings_ost(16, 32, vel=0.58)
bass16(16, 32, GALLOP, 1.0, 0.85)
synth_pulse(16, 32, 0.35)
groove_a(16, 32, 1.0)
play('horn', THEME, 16, vel=0.82, harm=True)
play('tbn', THEME, 16, transpose=-12, vel=0.55)
chords('tbn', 16, 32, vel=0.3, pick=[0], attack=0.2)
for b in (19, 23, 27):
    pickup(b, 0.75)
tom_fill(31, 2.0)

# CLASH 32-39: F5 trombone climb, F6 stabs, the strings drive on
hit(32, 1.0)
strings_ost(32, 40, vel=0.62, accent=0.45)
stabs(32, 40, 1.0)
for b in range(32, 40):
    if active(b):
        r = CH[chord_at(b)][0]
        for i in (4, 6, 8, 10, 12):
            put('bass', cached(('db', r), lambda: X.dist_bass(r, B(0.25) * 0.85)), T(b, i * 0.25), 0.7)
play('tbn', CLIMB, 32, vel=0.9, harm=False)
play('tbn', CLIMB, 32, transpose=-12, vel=0.75)
play('horn', CLIMB, 32, vel=0.7)
play('tpt', [(16 - 1, 'C5', 0.5), (15.5, 'C5', 0.25), (15.75, 'C5', 0.25), (16, 'C5', 7),
             (24 - 1, 'D5', 0.5), (23.5, 'D5', 0.25), (23.75, 'D5', 0.25), (24, 'D5', 7)], 32, vel=0.7)
groove_a(32, 40, 1.0, sticks=True)
for b in (33, 35, 37):
    pickup(b, 0.8, crash=False)
tom_fill(39, 1.0, (45, 45, 45, 43, 43, 43, 41, 41, 38, 38, 36, 36), 0.25, 0.9)

# BREAKDOWN 40-47: no beat; solo horn quotes F1, strings sustain, choir
hit(40, 0.6, braam=False)
gong(40, 0.6)
play('horn', F1, 40, vel=0.6)
chords('vc', 40, 48, vel=0.4, pick=[0, 1], attack=0.6)
chords('cb', 40, 48, vel=0.4, pick=[0], transpose=-12, attack=0.6)
chords('vln2', 40, 48, vel=0.3, pick=[3, 4], transpose=12, attack=1.0)
choir_span(40, 48, 0.35, attack=1.2)
sub_span(40, 48, 0.5)
for b in range(40, 48):
    taiko_at(b, 0, 0.35, pan=-0.1)
    if b % 2 == 1:
        metal_at(b, 2.5, 'anvil', 0.3, 0.2)

# BUILD 48-55: F5 climb again (faster), taikos play the F2 cycle, riser, gap
taiko_at(48, 0, 0.8)
strings_ost(48, 52, vel=0.45, low=False)
strings_ost(52, 56, vel=0.58, low=True, low_vel=0.5)
play('tbn', CLIMB_BUILD, 48, vel=0.75)
play('tbn', CLIMB_BUILD, 48, transpose=-12, vel=0.6)
play('horn', CLIMB_BUILD, 48, vel=0.6, swell=0.5)
play('vln2', [(16, CH['Fm'][2][4], 8, 1.0)], 48, transpose=12, vel=0.4, art='trem')
play('vln2', [(24, 'D5', 7.5)], 48, vel=0.5, art='trem', swell=0.3)
sub_span(48, 56, 0.6)
for b in range(48, 55):
    lvl = 0.45 + 0.07 * (b - 48)
    for bt in PING[b % 4]:
        taiko_at(b, bt, lvl, 'taiko' if bt in (0, 2) else 'taiko_hand', pan=0.25 if bt < 2 else -0.25)
    kick_at(b, 0, 0.6 + 0.05 * (b - 48))
    if b >= 52:
        kick_at(b, 2, 0.6 + 0.05 * (b - 48))
        snare_at(b, 1, 0.5 + 0.08 * (b - 52))
        snare_at(b, 3, 0.55 + 0.08 * (b - 52))
for i in range(12):                       # bar 55: toms build, stop at 3.5
    tom_at(55, i * 0.25, [45, 43, 41, 38][i // 3], 0.6 + 0.03 * i, 0.4 - 0.07 * i)
if active(52):
    put('fx', X.riser(B(14)), T(52), 0.5)
for b in range(52, 55):
    for i in range(0, 16, 2):
        r = CH[chord_at(b)][0]
        put('bass', cached(('db', r), lambda: X.dist_bass(r, B(0.25) * 0.85)), T(b, i * 0.25), 0.7)

# PEAK 56-71: the theme in trumpets + horns, four-on-the-floor groove
hit(56, 1.15)
gong(56, 0.5)
strings_ost(56, 72, vel=0.62)
bass16(56, 72, DRIVE16, 1.0, 0.85)
synth_pulse(56, 72, 0.42)
groove_b(56, 72, 1.0)
play('tpt', THEME, 56, vel=0.8)
play('horn', THEME, 56, transpose=-12, vel=0.8, harm=True)
play('tbn', THEME, 56, transpose=-12, vel=0.45)
chords('tbn', 56, 72, vel=0.35, pick=[0, 1], attack=0.2)
chords('tuba', 56, 72, vel=0.4, pick=[0], transpose=-12, attack=0.2)
choir_span(56, 72, 0.22)
for b in (59, 63, 67):
    pickup(b, 0.8)
tom_fill(71, 2.0)

# BRIDGE 72-79: half time; F4 call sequenced on open fifths, F6 stabs
hit(72, 0.9)
halftime(72, 80, 1.0)
stabs(72, 80, 0.95)
CALLS = (CALL(-1, 'Eb5') + [(0, 'Eb5', 7)] + CALL(7, 'F5') + [(8, 'F5', 7)] +
         CALL(15, 'G5') + [(16, 'G5', 7)] + [(24, 'G5', 6)])
play('tpt', CALLS, 72, vel=0.8)
play('tpt', [(b, n.replace('Eb5', 'Ab4').replace('F5', 'Bb4').replace('G5', 'C5'), ln)
             for b, n, ln in CALLS[:12]] + [(24, 'D5', 6)], 72, vel=0.7)
play('horn', [(0, 'Ab4', 8), (8, 'Bb4', 8), (16, 'C5', 8), (24, 'B4', 8)], 72, transpose=-12, vel=0.6)
chords('vc', 72, 80, vel=0.5, pick=[0, 1], art='trem', attack=0.2)
chords('vln1', 72, 80, vel=0.45, pick=[3, 4], transpose=12, art='trem', attack=0.3)
sub_span(72, 80, 0.4)
tom_fill(79, 1.0, (45, 45, 45, 45, 43, 43, 43, 43, 41, 41, 38, 36), 0.25, 1.0)
if active(78):
    put('fx', X.riser(B(8)), T(78), 0.45)

# CLIMAX 80-95: the theme, full: trumpets, horns, violin descant, choir
hit(80, 1.2)
gong(80, 0.55)
strings_ost(80, 96, vel=0.6, octave=0)
bass16(80, 96, DRIVE16, 1.0, 0.85)
synth_pulse(80, 96, 0.45)
groove_b(80, 96, 1.05)
play('tpt', THEME, 80, vel=0.88, harm=True)
play('horn', THEME, 80, transpose=-12, vel=0.85, harm=True)
play('tbn', THEME, 80, transpose=-12, vel=0.5)
play('vln2', DESCANT, 80, vel=0.5, attack=0.15)
chords('tuba', 80, 96, vel=0.45, pick=[0], transpose=-12, attack=0.2)
chords('tbn', 80, 96, vel=0.35, pick=[0, 1], attack=0.2)
choir_span(80, 96, 0.3)
for b in (83, 87, 91):
    pickup(b, 0.85)
hit(88, 0.8, braam=False)
tom_fill(95, 2.0)

# OUTRO 96-99 and the final hit (bar 100): F4 call, F7 lift into C minor
hit(96, 0.8, braam=False)
strings_ost(96, 100, vel=0.55)
bass16(96, 100, GALLOP, 0.9, 0.8)
groove_a(96, 99, 0.9)
kick_at(99, 0, 0.9)
snare_at(99, 1, 0.8)
taiko_at(99, 0, 0.8)
play('tpt', CALL(-1, 'C5') + [(0, 'C5', 7)] + CALL(7, 'Ab4') + [(8, 'Ab4', 4)] +
     [(13.5, 'D5', 1.5), (15, 'Eb5', 0.5), (15.5, 'F5', 0.5)], 96, vel=0.78)
play('horn', CALL(-1, 'G4') + [(0, 'G4', 7)] + CALL(7, 'Eb4') + [(8, 'Eb4', 4), (12, 'D4', 4)], 96, vel=0.7)
play('tbn', [(0, 'C4', 8), (8, 'Ab3', 4), (12, 'G3', 4)], 96, transpose=-12, vel=0.6)
for i, md in enumerate((45, 45, 43, 43, 41, 41, 38, 36)):
    tom_at(99, 2 + i * 0.25, md, 0.7 + 0.04 * i, 0.4 - 0.1 * i)
FIN = 2.0
if active(100):
    kick_at(100, 0, 1.0)
    taiko_at(100, 0, 1.0)
    taiko_at(100, 0.02, 0.9, 'taiko_hand', pan=0.3)
    put('fx', O.boom(1.0), T(100), 0.8)
    put('synth', O.braam([36, 43, 48], B(FIN)), T(100), 0.55)
    gong(100, 0.8)
    cym(100, 0, 0.6)
    metal_at(100, 0, 'anvil', 0.55, 0.0)
    put('sub', X.sub(36, B(FIN), 1.0, r=0.7), T(100), 0.9)
    put('bass', X.dist_bass(36, B(FIN), 1.0), T(100), 0.6)
play('tpt', [(0, 'G5', FIN), (0, 'Eb5', FIN)], 100, vel=0.85, rel=0.9, legato=False)
play('horn', [(0, 'C5+G4+Eb4+C4', FIN)], 100, vel=0.85, rel=1.0, legato=False)
play('tbn', [(0, 'C3+G3+C4', FIN)], 100, vel=0.85, rel=1.0, legato=False)
play('tuba', [(0, 'C2', FIN)], 100, vel=0.85, rel=1.0, legato=False)
play('vln1', [(0, 'C5+G5', FIN)], 100, vel=0.7, rel=1.1, legato=False)
play('vla', [(0, 'Eb4+G4', FIN)], 100, vel=0.6, rel=1.1, legato=False)
play('vc', [(0, 'C3+G3', FIN)], 100, vel=0.7, rel=1.1, legato=False)
play('cb', [(0, 'C2', FIN)], 100, vel=0.7, rel=1.1, legato=False)

_cache.clear()
gc.collect()


# ------------------------------------------------------------------ mix

def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


GAIN = {  # dB
    'strhi': 2.5, 'strlo': 0.0, 'horn': 0.0, 'tpt': 0.0, 'lowbr': -3.0, 'choir': -14.0,
    'bass': -9.0, 'sub': -7.0, 'synth': -10.0,
    'kick': -5.0, 'snare': -4.0, 'taiko': 1.0, 'metal': 2.0, 'fx': -8.0,
}
HALL = {'strhi': .35, 'strlo': .22, 'horn': .4, 'tpt': .35, 'lowbr': .3, 'choir': .6, 'snare': .12,
        'taiko': .18, 'metal': .12, 'fx': .25, 'synth': .08}
ROOM = {'snare': .35, 'taiko': .25, 'metal': .3, 'kick': .05}
DUCK = {'bass': 0.5, 'sub': 0.6, 'synth': 0.35, 'choir': 0.15, 'strlo': 0.12}

bus['strhi'].x = stage(bus['strhi'].x, HighpassFilter(150), PeakFilter(3200, -1.5, 1.0),
                       HighShelfFilter(8000, -0.5, 0.7))
bus['strlo'].x = stage(bus['strlo'].x, HighpassFilter(35))
bus['horn'].x = stage(bus['horn'].x, HighpassFilter(70))
bus['tpt'].x = stage(bus['tpt'].x, HighpassFilter(150), HighShelfFilter(7000, -2.0, 0.7))
bus['lowbr'].x = stage(bus['lowbr'].x, HighpassFilter(40))
bus['choir'].x = stage(bus['choir'].x, HighpassFilter(180), LowpassFilter(6000))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(50), PeakFilter(250, -2.0, 1.0), LowpassFilter(4000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(120))
bus['synth'].x = stage(bus['synth'].x, HighpassFilter(80), LowpassFilter(5000))
bus['kick'].x = stage(bus['kick'].x, HighpassFilter(30), LowpassFilter(8000))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(100), LowpassFilter(7000),
                       Compressor(threshold_db=-18, ratio=2.0, attack_ms=5, release_ms=80))
bus['taiko'].x = stage(bus['taiko'].x, HighpassFilter(38), LowpassFilter(7000),
                       Compressor(threshold_db=-16, ratio=2.0, attack_ms=8, release_ms=120))
bus['metal'].x = stage(bus['metal'].x, HighpassFilter(200), LowpassFilter(8500))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(25), LowpassFilter(10000))

KICKS = sorted(k for k in KICKS if 0 <= k < N / SR)
mix = np.zeros((2, N), np.float32)
TONAL_OUT = os.environ.get('NEW17_TONAL')
DRUMS = {'kick', 'snare', 'taiko', 'metal', 'fx'}
tonal = np.zeros((2, N), np.float32) if TONAL_OUT else None
hall_send = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
grind_src = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
for k in BUSES:
    x = bus[k].x
    del bus[k]
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= S.sidechain(N, KICKS, depth=DUCK[k], release=0.2, attack=0.004)[None, :]
    if k == 'bass':
        grind_src += x
    if k in MUTE:
        continue
    nz = np.nonzero(np.max(np.abs(x), 0) > 1e-4)[0]
    if len(nz) > SR:
        seg = x[:, nz[0]:nz[-1]].T.astype(np.float64)
        act = np.max(np.abs(seg), axis=1) > 1e-4
        levels[k] = meter.integrated_loudness(seg[act]) if act.sum() > SR else -99.0
        del seg
    mix += x
    if tonal is not None and k not in DRUMS:
        tonal += x
    if k in HALL:
        hall_send += x * np.float32(HALL[k])
    if k in ROOM:
        room_send += x * np.float32(ROOM[k])
    del x
gc.collect()

# industrial grind: parallel distortion of the bass (band-passed), louder in the peaks
bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
GRIND_DB = [(0, -60), (3.9, -60), (4, -12), (8, -8), (16, -5), (32, -2), (39.9, -2), (40, -60), (51.9, -60),
            (52, -10), (56, -3), (72, -1), (80, 0), (96, -5), (100, -6), (102, -60)]
gr = np.interp(bars_axis, [p[0] for p in GRIND_DB], [p[1] for p in GRIND_DB])
grind = np.tanh(7.0 * S.butter(grind_src, 250, 'high', 2)).astype(np.float32)
del grind_src
grind = stage(grind, HighpassFilter(400), LowpassFilter(4000), PeakFilter(1500, 2.0, 0.8))
grind *= (10 ** ((gr - 15.0) / 20)).astype(np.float32)[None, :]
if 'grind' not in MUTE and 'bass' not in MUTE:
    seg = grind.T.astype(np.float64)
    act = np.max(np.abs(seg), axis=1) > 1e-4
    levels['grind'] = meter.integrated_loudness(seg[act]) if act.sum() > SR else -99.0
    del seg
    mix += grind
    if tonal is not None:
        tonal += grind
del grind, gr
gc.collect()

if tonal is not None:
    sf_.write(TONAL_OUT, (tonal / (np.abs(tonal).max() + 1e-9) * 0.5).T, SR, subtype='FLOAT')
    del tonal

ir = O.hall_ir(length=3.6, rt_low=2.6, rt_mid=2.2, rt_high=1.0)
hall = O.convolve(stage(hall_send, HighpassFilter(220), LowpassFilter(7000)), ir)
del hall_send, ir
mix += hall * np.float32(0.5)
del hall
room = stage(room_send, HighpassFilter(200), LowpassFilter(6500),
             Reverb(room_size=0.3, damping=0.6, wet_level=1.0, dry_level=0.0, width=0.9))
del room_send
mix += room * np.float32(0.6)
del room
gc.collect()

# section dynamics (dB at bar positions)
SECTION_DB = [(0, -4.5), (7.9, -3.5), (8, -2.5), (15.9, -2.5), (16, -1.0), (31.9, -1.0), (32, -0.5),
              (39.9, -0.5), (40, -6.5), (47.9, -6.0), (48, -5.0), (55.9, -1.5), (56, 0.0), (71.9, 0.0),
              (72, -2.0), (79.9, -1.5), (80, 0.5), (95.9, 0.5), (96, -1.5), (99.9, -1.0), (100, -0.5),
              (104, -0.5)]
sec = np.interp(bars_axis, [p[0] for p in SECTION_DB], [p[1] for p in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, bars_axis

# ---------------------------------------------------------------- master
side = (mix[0] - mix[1]) * 0.5
side -= ss.sosfiltfilt(ss.butter(4, 120, 'low', fs=SR, output='sos'), side).astype(np.float32)
mid = (mix[0] + mix[1]) * 0.5
mix = np.stack([mid + side, mid - side]).astype(np.float32)
del side, mid
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix, LowShelfFilter(90, 0.5, 0.7), PeakFilter(500, -1.5, 0.7), HighShelfFilter(7000, 1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=100))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

if BAR1 == NBARS:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-70 / 20))[0][-1]
    end = min(last + 1, int((T(NBARS - 1) + 3.2) * SR))
    mix = mix[:, :end]
fade = int(1.2 * SR)
mix[:, -fade:] *= (np.linspace(1, 0, fade) ** 1.5)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new17_hybrid.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
