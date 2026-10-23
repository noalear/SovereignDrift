#!/usr/bin/env python3
# new06_industrial.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Black Shipyards": a NEW industrial / EBM track (late-90s: Front Line
# Assembly, the Quake soundtrack, Gravity Kills), 128 BPM, E-flat minor with
# the Phrygian E-major / E-flat alternation of the source.  It is not a remix:
# it is built from material of sf-music6 (the original game's music by
# Stardock, mid/sf-music6.mid) that the existing remix (song6_symphonic_metal.py)
# did not use or barely used.  Bar numbers are the original's, counted from the
# first downbeat (the MIDI starts with a two-beat pickup), as in that script:
#   1. the "sigh" inner voice of the trombones and piano, Ab3 -> F#3 over E-flat
#      and Ab3 -> E3 over E (1.5 + 5.5 beats, with a repeated pickup on beat 4;
#      bars 24-39 and 52-78) -> the distorted-brass counterline, and developed
#      as a falling sequence of 4-3 sighs over a new progression
#      Ebm - B - Abm - Bb (Ab-F#, E-Eb, C#-B, Eb-D)
#   2. the piccolo line Bb3 | Eb4 F#4 | Bb4 . . F#4 | Ab4 E4 (bars 32-36) and its
#      answer, the rising arpeggio Ab3 B3 | E4 | Ab4 B4 | Bb4 (bars 53-56)
#      -> the lead theme, extended (a second, higher phrase to C#5) and ending
#      on the source's Bb-over-E tritone
#   3. the beat-4 pings E4 Ab4 B4 E5 rising one per bar (orchestra hit and piano,
#      bars 60-63) -> tubular-bell / metal signal motif, also mirrored into
#      E-flat minor (Eb Gb Bb Eb)
#   4. the intro's string pad Eb6 | D6 Bb5 over the Eb3/Bb3 synth-brass fifth
#      (bars 0-7) -> the string-machine line of the intro, build and outro
#   plus the bass fill Eb Eb F# Ab Bb Bb1 (bar 65) as the sequencer's turnaround.
# New: everything else -- the form (intro, groove, theme, sigh section,
# breakdown, build, peak, half-time bridge, climax, outro with a Phrygian
# E -> E-flat cadence and a final hit that rings out), the 16th-note sequenced
# bass, the drum programming, the sound design and the mix.
#
# v2 (2026-10-23), Stellar Frontier remake -- after Kyle's listen: "the shipyard
# hammer/anvil hit that strikes about every 15 beats is too loud and feels off-beat"
# ("the rest of the song absolutely rips").  v1 put a long (0.9 s), deeply
# pitched anvil on the "and" of beat 4 of every 4th bar, about 2 dB under the
# snare and 8 dB over every other metal hit.  v2 replaces it with a shipyard
# layer locked to the quarter-note grid and repeating every 1-2 bars (YARD,
# shipyard()): a shorter, band-limited hammer (anvil + brake-drum strike,
# synth_new06.hammer) on beat 4 with the backbeat every bar, ~10 dB quieter
# than v1's anvil and level with the brake clanks; a machinery stamp
# (synth_new06.press, tuned Eb3/Bb2) on beats 1 and 3 under the kick; a
# hydraulic release (synth_new06.hydraulic) on beat 3 of every 2nd bar.  The
# breakdown's anvils moved from beat 3.5 to beat 3 at under half the level, with a
# hydraulic hiss; the final-hit anvil is 3 dB lower.  Nothing else changed: the
# arrangement, the other buses, the mix and the master are as in v1.
#
# Sounds: synthesized (tools/music/edm/synth.py, synth_new06.py), plus
# VSCO-2 Community Edition samples by Versilian Studios (CC0 1.0 / public
# domain, https://github.com/sgossner/VSCO-2-CE): anvil, brake drum, tubular
# bells, gong scrape and gong smack.
#
#   python3 tools/music/edm/new06_industrial.py OUT.wav [BAR0 BAR1]
#   NEW06_MUTE=kick,snare,... mutes buses (for the chroma check)
#   NEW06_STEMS=DIR writes each bus (after gain and ducking, before the sends) to DIR/<bus>.wav
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
import synth_new06 as I  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 128.0
SPB = 60.0 / BPM
NBARS = 96
TAIL = 0.6
BAR0 = int(sys.argv[2]) if len(sys.argv) > 3 else 0
BAR1 = int(sys.argv[3]) if len(sys.argv) > 3 else NBARS
OFFSET = BAR0 * 4 * SPB
N = int(((BAR1 - BAR0) * 4 * SPB + TAIL) * SR)
MUTE = set(filter(None, os.environ.get('NEW06_MUTE', '').split(',')))


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB - OFFSET


def B(beats):
    return beats * SPB


def active(bar):
    return BAR0 - 2 <= bar < BAR1 + 1


# ------------------------------------------------------------- harmony

CHORDS = (['Eb5'] * 8 +                                   # 0 intro (open fifth, Eb pedal)
          ['Ebm'] * 4 + ['E'] * 4 +                       # 8 groove
          (['Ebm'] * 4 + ['E'] * 4) * 2 +                 # 16 theme
          ['Ebm', 'Ebm', 'B', 'B', 'Abm', 'Abm', 'Bb', 'Bb'] +  # 32 sigh section
          ['Ebm'] * 4 + ['E'] * 4 +                       # 40 breakdown
          ['Ebm'] * 4 + ['E'] * 3 + ['Bb'] +              # 48 build
          (['Ebm'] * 4 + ['E'] * 4) * 2 +                 # 56 peak (theme)
          ['E'] * 4 + ['Abm', 'Abm', 'Bb', 'Bb'] +        # 72 half-time bridge
          ['Ebm', 'Ebm', 'B', 'B', 'Abm', 'Abm', 'Bb', 'Bb'] +  # 80 climax
          ['Ebm'] * 4 + ['E'] * 2 + ['Ebm'] * 2)          # 88 outro, final hit at 94
assert len(CHORDS) == NBARS, len(CHORDS)

PADV = {'Eb5': ['Eb3', 'Bb3'], 'Ebm': ['Eb3', 'Bb3', 'F#4'], 'E': ['E3', 'B3', 'G#4'],
        'B': ['B2', 'F#3', 'D#4'], 'Abm': ['Ab2', 'Eb3', 'B3'], 'Bb': ['Bb2', 'F3', 'D4']}
ROOT = {'Eb5': 39, 'Ebm': 39, 'E': 40, 'B': 35, 'Abm': 44, 'Bb': 34}

# fragment 2: the lead theme (16 bars over Ebm x4, E x4, Ebm x4, E x4)
THEME = [(2, 'Bb3', 2), (4, 'Eb4', 2), (6, 'F#4', 2), (8, 'Bb4', 6), (14, 'F#4', 2),
         (16, 'Ab4', 2), (18, 'E4', 10),
         (34, 'Bb3', 2), (36, 'Eb4', 2), (38, 'F#4', 2), (40, 'Bb4', 4), (44, 'C#5', 2), (46, 'Bb4', 2),
         (48, 'Ab3', 2), (50, 'B3', 2), (52, 'E4', 4), (56, 'Ab4', 2), (58, 'B4', 2), (60, 'Bb4', 4)]

# fragment 1: the sigh, as in the source (2 bars over Ebm, 2 bars over E)
SIGH_EB = [(0, 'Ab3', 1.5), (1.5, 'F#3', 5.5), (7, 'F#3', 1)]
SIGH_E = [(0, 'Ab3', 1.5), (1.5, 'E3', 5.5), (7, 'Ab3', 1)]
# ... developed: a falling sequence of 4-3 sighs over Ebm - B - Abm - Bb
SIGH_SEQ = [(0, 'Ab4', 1.5), (1.5, 'F#4', 5.5), (7, 'F#4', 1),
            (8, 'E4', 1.5), (9.5, 'D#4', 5.5), (15, 'D#4', 1),
            (16, 'C#4', 1.5), (17.5, 'B3', 5.5), (23, 'B3', 1),
            (24, 'Eb4', 1.5), (25.5, 'D4', 5.5), (31, 'F4', 1)]
# climax counter-line for the string machine
CLIMAX_STR = [(0, 'Bb4', 8), (8, 'B4', 8), (16, 'B4', 8), (24, 'D5', 8)]

# fragment 3: pings on beat 4, rising one per bar
PING = {'E': ['E4', 'G#4', 'B4', 'E5'], 'Ebm': ['Eb4', 'F#4', 'Bb4', 'Eb5'],
        'Eb5': ['Eb4', 'F#4', 'Bb4', 'Eb5']}

# fragment 4: the intro string line (8 bars)
INTRO_STR = [(0, 'Eb5', 16), (16, 'D5', 2), (18, 'Bb4', 14)]

# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'metal', 'ticks', 'toms', 'fx', 'bass', 'sub', 'pad', 'strings',
         'brass', 'lead', 'bell', 'tex']
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
    put('kick', cached(('k',), lambda: I.ebm_kick()), T(bar, beat), vel)
    kicks.append(T(bar, beat))


_sn = [0]


def snare_at(bar, beat, vel=1.0):
    if not active(bar):
        return
    v = _sn[0] % 4
    _sn[0] += 1
    put('snare', cached(('s', v), lambda: I.heavy_snare(1.0, v)), T(bar, beat),
        vel * (0.94 + 0.06 * S.rng().random()))


def four(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        for beat in range(4):
            kick_at(b, beat, vel * (1.0 if beat % 2 == 0 else 0.93))


def backbeat(bar0, bar1, vel=1.0, ghosts=True):
    for b in range(bar0, bar1):
        snare_at(b, 1, vel)
        snare_at(b, 3, vel * 1.03)
        if ghosts and b % 2 == 1:
            snare_at(b, 3.75, vel * 0.28)


def halftime(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        kick_at(b, 0, vel)
        kick_at(b, 2.5, vel * 0.85)
        if b % 2 == 1:
            kick_at(b, 3.5, vel * 0.7)
        snare_at(b, 2, vel)


def ticks(bar0, bar1, vel=1.0, sixteenth=True):
    acc = [0.55, 0.28, 0.9, 0.32] if sixteenth else [0.0, 0.0, 0.9, 0.0]
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for beat in range(4):
            for i, a in enumerate(acc):
                if a:
                    s = (b * 16 + beat * 4 + i) % 3
                    put('ticks', cached(('t', s), lambda: I.tick(1.0, 0.25 * (s - 1), s)),
                        T(b, beat + i * .25), vel * a)


METAL_A = [(0.75, 'brake', -3, 0.55, 0), (2.75, 'brake', -5, 0.5, 1)]
METAL_B = METAL_A + [(1.5, 'brake', -8, 0.35, 2), (3.25, 'brake', -3, 0.3, 0)]


def metal(bar0, bar1, pattern=METAL_A, vel=1.0, anvil=True, pipes=False):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        for beat, kind, semis, v, var in pattern:
            sig = cached(('c', kind, semis, var), lambda: I.clank(kind, semis, 1.0, var))
            pan = -0.45 if beat < 2 else 0.45
            put('metal', S.pan2(sig.mean(0), pan), T(b, beat), vel * v)
        if anvil:
            shipyard(b, vel)
        if pipes:
            for beat in (0.5, 1.5, 2.5, 3.5):
                md = 58 if beat < 2 else 63
                sig = cached(('p', md), lambda: I.metal_hit(md, 1.0, decay=0.12, bright=0.7, seed=md))
                put('metal', S.pan2(sig.mean(0), 0.6 if beat < 2 else -0.6), T(b, beat), vel * 0.22)


# v2: the shipyard layer (replaces v1's loud, long anvil on beat 4.5 of every 4th bar).
# A 2-bar loop on the quarter grid, under the snare and clanks:
#   machinery stamp on beats 1 and 3 (with the kick), the hammer on beat 4 (with
#   the backbeat) of every bar, a hydraulic release on beat 3 of every 2nd bar.
YARD = [(0.0, 'press', 51, 0.07, -0.25), (2.0, 'press', 46, 0.06, 0.25),
        (3.0, 'hammer', 0, 0.34, 0.2),
        (4.0, 'press', 51, 0.07, -0.25), (6.0, 'hydraulic', 0, 0.075, 0.5), (6.0, 'press', 46, 0.05, 0.25),
        (7.0, 'hammer', 1, 0.30, -0.15)]


def shipyard(bar, vel=1.0):
    half = (bar % 2) * 4
    for beat, kind, p, v, pan in YARD:
        if not half <= beat < half + 4:
            continue
        if kind == 'press':
            sig = cached(('pr', p), lambda: I.press(p, seed=p))
        elif kind == 'hammer':
            sig = cached(('hm', p), lambda: I.hammer(p))
        else:
            sig = cached(('hy', p), lambda: I.hydraulic(B(1.0), seed=p))
        put('metal', S.pan2(sig.mean(0), pan) * 1.41 if kind != 'hydraulic' else sig,
            T(bar, beat - half), vel * v)


def tom_fill(bar, start=2.0, notes=(46, 46, 42, 42, 39, 39, 34, 34), step=0.25, vel=0.9):
    if not active(bar):
        return
    for i, md in enumerate(notes):
        put('toms', S.tom(md, 1.0), T(bar, start + i * step), vel * (0.8 + 0.2 * i / len(notes)))


def swell_into(bar, beats=4.0, vel=0.6):
    if active(bar):
        put('fx', I.rev_swell(B(beats)), T(bar) - B(beats), vel)


def hit(bar, vel=1.0, gong=True):
    if not active(bar):
        return
    put('fx', S.impact(1.0), T(bar), 0.7 * vel)
    if gong:
        put('fx', cached(('gs',), lambda: I.gong_smack(-4.0, 1.0)), T(bar), 0.55 * vel)


# ----------------------------------------------------------------- bass

GALLOP = [None, 0, 0, 12, None, 0, 0, 0, None, 0, 0, 12, None, 0, 12, 0]
DRIVE = [0, 0, 12, 0, 0, 0, 12, 0, 0, 0, 12, 0, 0, 12, 0, 12]
PULSE = [None, None, 0, None] * 4
ACCENT = {3, 7, 11, 14}


def cut_at(bar):
    pts = [(0, 200), (4, 250), (8, 650), (16, 750), (32, 850), (40, 320), (48, 300), (55.9, 1000),
           (56, 950), (72, 650), (80, 1100), (88, 800), (94, 450), (96, 450)]
    return float(np.interp(bar, [p[0] for p in pts], [p[1] for p in pts]))


def bass_note(bar, beat, md, dur_beats, vel, accent):
    c = round(cut_at(bar + beat / 4) / 50) * 50
    env = 2600 if accent else 1500
    sig = cached(('b', md, dur_beats, c, env),
                 lambda: I.seq_bass(md, B(dur_beats) * 0.92, 1.0, cut=c, env=env))
    put('bass', sig, T(bar, beat), vel * (1.0 if accent else 0.82))


def turnaround(bar, chord):
    """Last bar of a phrase: two beats of gallop, then the bar-65 fill."""
    if chord == 'E':
        run = [(1.5, 40, .25), (1.75, 40, .25), (2, 39, .5), (2.5, 42, .5), (3, 44, .5), (3.5, 34, .5)]
    else:
        run = [(1.5, 39, .25), (1.75, 39, .25), (2, 42, .5), (2.5, 44, .5), (3, 46, .5), (3.5, 34, .5)]
    r = ROOT[chord]
    for i in range(6):
        st = GALLOP[i]
        if st is not None:
            bass_note(bar, i * .25, r + st, .25, 0.9, i in ACCENT)
    for beat, md, d in run:
        bass_note(bar, beat, md, d, 1.0, True)


def bass(bar0, bar1, pattern=GALLOP, vel=1.0, sub_vel=0.8, turn_every=8, sustain=False):
    for b in range(bar0, bar1):
        if not active(b):
            continue
        ch = CHORDS[b]
        r = ROOT[ch]
        sr = r - 12 if r - 12 >= 30 else r
        if sub_vel:
            put('sub', cached(('sub', sr), lambda: I.sub(sr, B(4) - 0.03)), T(b), sub_vel)
        if sustain:
            continue
        if turn_every and (b - bar0) % turn_every == turn_every - 1:
            turnaround(b, ch)
            continue
        for i, st in enumerate(pattern):
            if st is None:
                continue
            bass_note(b, i * .25, r + st, .25, vel, i in ACCENT)


# ---------------------------------------------------------- tonal parts

def pads(bar0, bar1, vel=1.0, cut=900.0, attack=1.2):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if active(b) or active(e - 1):
            dur = T(e) - T(b)
            for note in PADV[ch]:
                put('pad', I.dark_pad(m(note), dur, 1.0, cut=cut, a=attack), T(b), vel * 0.5)
        b = e


def line(target, bar0, notes, transpose=0, vel=1.0, inst='lead', **kw):
    for beat, note, dur in notes:
        bar = bar0 + int(beat // 4)
        if not (active(bar) or active(bar + int(dur // 4))):
            continue
        md = m(note) + transpose
        if inst == 'lead':
            sig = I.cold_lead(md, B(dur) - 0.03, 1.0, **kw)
        elif inst == 'brass':
            sig = I.grind_brass(md, B(dur) - 0.02, 1.0, **kw)
        elif inst == 'strings':
            sig = I.string_machine(md, B(dur), 1.0, **kw)
        elif inst == 'bell':
            sig = I.tubular(md, max(2.5, B(dur)), 1.0)
        else:
            raise ValueError(inst)
        put(target, sig, T(bar0, beat), vel)


def sighs(bar0, bar1, vel=1.0, transpose=0, target='brass', inst='brass'):
    """The source's sigh figure, 2 bars per cell, following Ebm / E."""
    for b in range(bar0, bar1, 2):
        cell = SIGH_E if CHORDS[b] == 'E' else SIGH_EB
        line(target, b, cell, transpose, vel, inst)


def pings(bar0, bar1, vel=1.0, beat=3.0):
    for b in range(bar0, bar1):
        ch = CHORDS[b]
        if ch not in PING or not active(b):
            continue
        k = 0
        while bar0 <= b - k - 1 and CHORDS[b - k - 1] == ch:
            k += 1
        note = PING[ch][k % 4]
        put('bell', cached(('tb', note), lambda: I.tubular(m(note), 2.5, 1.0)), T(b, beat), vel)


def drone_span(bar0, bar1, vel=1.0):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if active(b) or active(e - 1):
            n = int((T(e) - T(b) + 0.6) * SR)
            d = I.drone(ROOT[ch] - 12, n, 1.0, seed=b)
            t = np.arange(n) / SR
            fade = np.minimum(np.minimum(t / 0.3, 1), np.clip((n / SR - t) / 0.6, 0, 1))
            put('tex', d * fade[None, :].astype(np.float32), T(b), vel)
        b = e


# ---------------------------------------------------------- arrangement

# INTRO 0-7: open fifth, machine drone, the intro string line, pings, bass fades in
pads(0, 8, vel=0.8, cut=700, attack=2.5)
drone_span(0, 8, 0.6)
if active(0):
    put('tex', I.gong_scrape(-5.0, 1.0), T(0), 0.35)
line('strings', 0, INTRO_STR, 0, 0.55, 'strings', cut=1600, a=1.5)
pings(0, 8, 0.45)
bass(0, 4, sustain=True, sub_vel=0.45)
bass(4, 8, PULSE, vel=0.7, sub_vel=0.55, turn_every=0)
metal(4, 8, [(2.75, 'brake', -5, 0.35, 1)], vel=0.8, anvil=True)
swell_into(8, 4, 0.45)

# GROOVE 8-15: the machine starts
hit(8, 0.6, gong=False)
four(8, 16)
backbeat(8, 16, 0.9)
ticks(8, 16, 0.8)
metal(8, 16, METAL_A, 0.9)
bass(8, 16, GALLOP, 1.0, 0.75)
pads(8, 16, 0.55, cut=800)
drone_span(8, 16, 0.3)
pings(8, 16, 0.4)
tom_fill(15, 3.0, (46, 42, 39, 34), 0.25, 0.7)

# THEME 16-31: the lead enters (fragment 2); the sigh joins quietly in the second half
four(16, 32)
backbeat(16, 32, 0.95)
ticks(16, 32, 0.85)
metal(16, 32, METAL_A, 1.0)
bass(16, 32, GALLOP, 1.0, 0.8)
pads(16, 32, 0.6, cut=950)
drone_span(16, 32, 0.25)
line('lead', 16, THEME, 0, 1.0)
sighs(24, 32, 0.45)
tom_fill(31, 2.0)

# SIGH SECTION 32-39: Ebm B Abm Bb, falling 4-3 sighs in distorted brass
hit(32, 0.7)
four(32, 40)
backbeat(32, 40, 1.0)
ticks(32, 40, 0.9)
metal(32, 40, METAL_B, 1.0)
bass(32, 40, GALLOP, 1.0, 0.8)
pads(32, 40, 0.7, cut=1100, attack=0.6)
line('brass', 32, SIGH_SEQ, 0, 0.9)
line('brass', 32, SIGH_SEQ, -12, 0.55)
pings(32, 40, 0.0)
tom_fill(39, 2.0, (46, 46, 42, 42, 39, 39, 34, 34))

# BREAKDOWN 40-47: no beat; bells recall the theme over the drone
hit(40, 0.5)
if active(40):
    put('tex', I.gong_scrape(-7.0, 1.0), T(40), 0.4)
drone_span(40, 48, 0.55)
pads(40, 48, 0.75, cut=800, attack=1.5)
bass(40, 48, PULSE, 0.75, 0.6, turn_every=0)
line('bell', 40, [(2, 'Bb3', 2), (4, 'Eb4', 2), (6, 'F#4', 2), (8, 'Bb4', 8)], 12, 0.55, 'bell')
line('bell', 44, [(0, 'Ab3', 2), (2, 'B3', 2), (4, 'E4', 4), (8, 'Ab4', 2), (10, 'B4', 2), (12, 'Bb4', 4)],
     12, 0.55, 'bell')
for b in (41, 43, 45, 47):  # v2: distant hammer + hydraulic on beat 3 (was a loud anvil on beat 3.5)
    if active(b):
        put('metal', cached(('anv2',), lambda: I.clank('anvil', -17, 1.0, 2, maxlen=0.8, lp=3000, decay=0.25)),
            T(b, 2.0), 0.2)
        put('metal', cached(('hy', 1), lambda: I.hydraulic(B(1.5), seed=1)), T(b, 2.0), 0.06)

# BUILD 48-55: kick returns, metal accelerates, strings climb (fragment 4, octave up)
for b in range(48, 52):
    kick_at(b, 0, 0.9)
    kick_at(b, 2, 0.8)
four(52, 56, 0.95)
for b in range(52, 56):
    snare_at(b, 1, 0.7 + 0.05 * (b - 52))
    snare_at(b, 3, 0.75 + 0.05 * (b - 52))
bass(48, 56, GALLOP, 0.95, 0.7, turn_every=8)
pads(48, 56, 0.65, cut=1000, attack=0.8)
line('strings', 48, INTRO_STR, 12, 0.45, 'strings', cut=2400, a=0.8)
drone_span(48, 56, 0.4)
metal(48, 52, [(1.75, 'brake', -5, 0.45, 0), (3.75, 'brake', -3, 0.45, 1)], 1.0, anvil=False)
metal(52, 54, METAL_A, 1.0, anvil=False)
metal(54, 56, METAL_B, 1.0, anvil=False)
ticks(52, 56, 0.6)
pings(48, 56, 0.4)
tom_fill(55, 2.0, (46, 46, 46, 42, 42, 42, 39, 39), 0.25, 1.0)
swell_into(56, 4, 0.55)

# PEAK 56-71: the theme with the sigh counterline (fragment 1 as in the source)
hit(56, 1.0)
four(56, 72)
backbeat(56, 72, 1.0)
ticks(56, 72, 0.95)
metal(56, 72, METAL_B, 1.0, pipes=True)
bass(56, 72, GALLOP, 1.0, 0.85)
pads(56, 72, 0.7, cut=1200, attack=0.5)
line('lead', 56, THEME, 0, 1.0)
line('lead', 56, THEME, 12, 0.32, cut=3000)
sighs(56, 72, 0.75)
pings(56, 72, 0.32)
drone_span(56, 72, 0.25)
hit(64, 0.6, gong=True)
tom_fill(71, 2.0)

# BRIDGE 72-79: half time, the E-major pings in full, sighs over E
hit(72, 0.6)
halftime(72, 80, 0.95)
ticks(72, 80, 0.7, sixteenth=False)
metal(72, 80, METAL_A, 0.85)
bass(72, 80, GALLOP, 0.9, 0.8, turn_every=0)
pads(72, 80, 0.7, cut=900, attack=0.6)
sighs(72, 76, 0.8)
line('brass', 76, [(0, 'C#4', 1.5), (1.5, 'B3', 5.5), (7, 'B3', 1), (8, 'Eb4', 1.5), (9.5, 'D4', 6.5)],
     -12, 0.8)
pings(72, 76, 0.75, beat=3.0)
pings(72, 76, 0.4, beat=1.0)
drone_span(72, 80, 0.35)
tom_fill(79, 1.0, (46, 46, 46, 46, 42, 42, 42, 42, 39, 39, 34, 34), 0.25, 1.0)
swell_into(80, 4, 0.6)

# CLIMAX 80-87: the sigh sequence in the lead, brass an octave down, strings on top
hit(80, 1.0)
four(80, 88)
backbeat(80, 88, 1.0)
ticks(80, 88, 1.0)
metal(80, 88, METAL_B, 1.05, pipes=True)
bass(80, 88, DRIVE, 0.9, 0.85)
pads(80, 88, 0.75, cut=1300, attack=0.4)
line('lead', 80, SIGH_SEQ, 0, 1.0)
line('lead', 80, SIGH_SEQ, 12, 0.3, cut=3000)
line('brass', 80, SIGH_SEQ, -12, 0.85)
line('strings', 80, CLIMAX_STR, 0, 0.5, 'strings', cut=2600, a=0.4)
drone_span(80, 88, 0.3)
tom_fill(87, 2.0)

# OUTRO 88-95: the machine winds down; Phrygian cadence E -> Eb; final hit at 94
hit(88, 0.6, gong=False)
four(88, 94, 0.95)
backbeat(88, 94, 0.9, ghosts=False)
ticks(88, 92, 0.6)
metal(88, 94, METAL_A, 0.85)
bass(88, 94, GALLOP, 0.95, 0.8, turn_every=6)
pads(88, 94, 0.65, cut=900)
line('strings', 88, [(0, 'Eb5', 8), (8, 'D5', 2), (10, 'Bb4', 6), (16, 'E5', 7.5)], 0, 0.5, 'strings',
     cut=1600, a=0.8)
pings(88, 94, 0.45)
drone_span(88, 94, 0.4)
if active(94):
    kick_at(94, 0, 1.0)
    hit(94, 1.0)
    put('metal', I.clank('anvil', -19, 1.0, 0, maxlen=2.0, lp=3500, decay=0.8), T(94), 0.42)
    for note in ['Eb2', 'Bb2', 'Eb3', 'F#3', 'Bb3']:
        put('brass', I.grind_brass(m(note), B(1.5), 1.0, cut=1400, a=0.005, r=0.9), T(94), 0.45)
    put('pad', I.dark_pad(m('Eb3'), B(2), 1.0, cut=900, a=0.01, r=1.2), T(94), 0.5)
    put('pad', I.dark_pad(m('Bb3'), B(2), 1.0, cut=900, a=0.01, r=1.2), T(94), 0.4)
    put('sub', I.sub(27, B(3), 1.0, r=0.6), T(94), 0.9)
    put('bell', I.tubular(m('Eb4'), 3.0, 1.0), T(94), 0.5)
    put('bell', I.tubular(m('Bb4'), 3.0, 1.0), T(94, 0.02), 0.3)

_cache.clear()
gc.collect()

# ------------------------------------------------------------- mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_bass = S.sidechain(N, kicks, depth=0.45, release=0.16)
duck_soft = S.sidechain(N, kicks, depth=0.25, release=0.22)

GAIN = {  # dB
    'kick': -3.0, 'snare': -4.5, 'metal': 6.0, 'ticks': 2.0, 'toms': -11.0, 'fx': -12.0,
    'bass': -8.0, 'sub': -10.0, 'pad': -3.0, 'strings': -3.0, 'brass': -12.0, 'lead': -7.0,
    'bell': -5.0, 'tex': -10.0,
}
DUCK = {'bass': duck_bass, 'sub': duck_bass, 'pad': duck_soft, 'tex': duck_soft, 'brass': duck_soft}
ROOM = {'snare': .22, 'metal': .3, 'toms': .2, 'ticks': .1, 'kick': .04}
HALL = {'lead': .25, 'bell': .45, 'strings': .4, 'pad': .3, 'brass': .2, 'metal': .12, 'fx': .25,
        'tex': .3, 'snare': .06}
DELAY = {'lead': .2, 'bell': .25, 'metal': .06}

bus['bass'].x = stage(bus['bass'].x, HighpassFilter(40), LowpassFilter(5000))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(110), Chorus(rate_hz=0.3, depth=0.25, centre_delay_ms=9, mix=0.35))
bus['strings'].x = stage(bus['strings'].x, HighpassFilter(250),
                         Chorus(rate_hz=0.6, depth=0.35, centre_delay_ms=7, mix=0.5))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(180), PeakFilter(2500, 1.5, 0.8))
bus['brass'].x = stage(bus['brass'].x, HighpassFilter(110))
bus['ticks'].x = stage(bus['ticks'].x, LowpassFilter(11000))
bus['metal'].x = stage(bus['metal'].x, HighpassFilter(200), LowpassFilter(9000))
bus['tex'].x = stage(bus['tex'].x, HighpassFilter(150), LowpassFilter(5000))
bus['fx'].x = stage(bus['fx'].x, LowpassFilter(9000))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(90))

mix = np.zeros((2, N), np.float32)
room_send = np.zeros((2, N), np.float32)
hall_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
grind_src = np.zeros((2, N), np.float32)
levels = {}
meter = pyln.Meter(SR)
for k in names:
    x = bus[k].x * np.float32(10 ** (GAIN[k] / 20))
    bus[k].x = None
    if k in DUCK:
        x = x * DUCK[k][None, :]
    if k == 'bass':
        grind_src += x
    if k in MUTE:
        continue
    if os.environ.get('NEW06_STEMS'):
        sf_.write(os.path.join(os.environ['NEW06_STEMS'], k + '.wav'), x.T, SR, subtype='FLOAT')
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

# distorted texture: parallel grind of the sequenced bass (band-passed), only in the louder sections
bars_axis = (np.arange(N) / SR + OFFSET) / (4 * SPB)
GRIND_DB = [(0, -60), (7.9, -60), (8, -9), (16, -6), (32, -3), (39.9, -3), (40, -60), (51.9, -60),
            (52, -9), (56, -2), (72, -6), (80, 0), (88, -6), (93.9, -9), (94, -60), (96, -60)]
gr = np.interp(bars_axis, [p[0] for p in GRIND_DB], [p[1] for p in GRIND_DB])
grind = np.tanh(6.0 * S.butter(grind_src, 300, 'high', 2)).astype(np.float32)
grind = stage(grind, HighpassFilter(500), LowpassFilter(4500), PeakFilter(1800, 2.0, 0.8))
grind *= (10 ** ((gr - 11.0) / 20)).astype(np.float32)[None, :]
del grind_src
if 'grind' not in MUTE:
    seg = grind.T.astype(np.float64)
    act = np.max(np.abs(seg), axis=1) > 1e-4
    levels['grind'] = meter.integrated_loudness(seg[act]) if act.sum() > SR else -99.0
    del seg
    mix += grind
    hall_send += grind * 0.1
del grind

room = stage(room_send, HighpassFilter(200), LowpassFilter(7000),
             Reverb(room_size=0.32, damping=0.6, wet_level=1.0, dry_level=0.0, width=0.9))
del room_send
pre = int(0.025 * SR)
hs = np.zeros_like(hall_send)
hs[:, pre:] = hall_send[:, :-pre]
del hall_send
hall = stage(hs, HighpassFilter(300), LowpassFilter(6500),
             Reverb(room_size=0.9, damping=0.55, wet_level=1.0, dry_level=0.0, width=1.0))
del hs
dly = S.pingpong(delay_send, B(0.75), feedback=0.38, repeats=6, lp=4000, hp=400)
del delay_send
mix = mix + room * 0.8 + hall * 0.75 + dly * 0.5
del room, hall, dly
gc.collect()

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -2.5), (7.9, -2), (8, -3), (15.9, -3), (16, -1.5), (31.9, -1.5), (32, -0.5), (39.9, -0.5),
              (40, -4), (47.9, -3.5), (48, -4), (55.9, -1.5), (56, 0), (71.9, 0), (72, -2.5), (79.9, -2),
              (80, 0.7), (87.9, 0.7), (88, -2), (93.9, -2.5), (94, -1), (96, -1)]
sec = np.interp(bars_axis, [p[0] for p in SECTION_DB], [p[1] for p in SECTION_DB])
mix = mix * (10 ** (sec / 20)).astype(np.float32)[None, :]
del sec, gr, bars_axis

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(80, 0.0, 0.7), PeakFilter(3500, 2.0, 0.7), HighShelfFilter(6000, 1.0, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=15, release_ms=120))
mix = np.tanh(mix * 1.1) / 1.1
mix = stage(mix, Limiter(threshold_db=-2.0, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix = mix * np.float32(10 ** ((-16.0 - loud) / 20))

if BAR1 == NBARS:
    env = np.max(np.abs(mix), axis=0)
    last = np.nonzero(env > 10 ** (-60 / 20))[0][-1]
    mix = mix[:, : last + 1]
fade = int(0.4 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new06_industrial.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
