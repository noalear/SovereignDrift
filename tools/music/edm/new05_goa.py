#!/usr/bin/env python3
# new05_goa.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# "Nebula Caravan": a NEW goa / psytrance track (1996-99 style: Hallucinogen,
# Astral Projection, Juno Reactor) for the game's music set.  It is not a remix:
# it is built from fragments of the original game music that the song-5
# remix (song5_edm.py) does not feature, or only touches in passing.
# Credits: the source material is Stardock's music for Stellar Frontier
# (mid/sf-music5.mid, Stardock Shared Source license).  Everything here is
# synthesized in code (tools/music/edm/synth.py, synth_goa.py); no samples.
#
# Fragments of sf-music5.mid used (bar numbers are the MIDI's; its phrases
# start on beat 3 of the MIDI bars, so "true" bars are shifted by 2 beats):
#   A. the C# / D "Phrygian call" of bars 41-48 (synth brass + trumpets):
#      x . x x | X---- on C#4/G#4, then on C#5 -> D5 over a G#4 pedal, with
#      the bass alternating C#2 / D2 every two bars.  Song 5 used it only as
#      bridge stabs; here it is (1) the cell of the TB-303 acid riff
#      (C# C# C# D + Phrygian A/B/G# turns), (2) the twisting lead "call"
#      of the C#/D sections, answered by a new Phrygian descent D C# B A G#,
#      and (3) the C#/D bass alternation of the groove B and bridge sections.
#   B. the synth-brass counter-line of bars 20-24 / 28-32 (channel 7):
#      E4 F#4 | G#4 F#4 E4 F#4 | G#4 ---- | F#4 ----, unused in song 5.
#      Developed into the 8-bar squelchy lead theme of groove A (extended
#      with a G#-A-G# turn and a Phrygian D4-C#4 / D5-C#5 cadence).
#   C. the choir counter-melody of bars 65-72 (channel 6): G#4 | C#5 D#5 |
#      E5 | G#5 (2nd time ending on C#5), over the bass C#2 C#2 E2 F#2 of
#      bars 57-72; unused in song 5.  It becomes the euphoric peak melody,
#      extended to 8 bars with a new answering phrase, over C#m C#m E F#sus2 |
#      C#m C#m E G#sus4 (the original bass roots, new voicings).
#   D. the D-bass section of bars 73-80 (C# material over a D pedal), as the
#      first half of the breakdown: D A C# G# (Dmaj7#11 without the third),
#      the Phrygian bII; it also gives the bII -> i cadence of the ending.
#   E. the conga / tom fill of bars 44, 48 and 64 (Open Hi Conga, Low Conga,
#      High Tom, Low-Mid Tom, snare, Floor Tom), re-tuned to the key, as the
#      phrase-end fills.
# New: 142 BPM (from 180), a psytrance form (intro, groove A, groove B,
# breakdown, peak 1, acid bridge, peak 2, outro), the rolling off-kick bass,
# the acid line, drums, pads and the euphoric supersaw lead, harmonies, FX.
# The ending is a Phrygian cadence (D -> C#m) and a final C#m hit with a short
# ring-out, so the loop can return to the C# drone of the intro.
#
#   python3 tools/music/edm/new05_goa.py OUT.wav
#   env NEW05_BARS=b0,b1   render only those bars (while iterating)
#   env NEW05_MUTE=kick,snare,...  mutes buses (for the chroma check)
import os
import sys
import numpy as np
import soundfile as sf_
import pyloudnorm as pyln
from pedalboard import (Pedalboard, Reverb, Chorus, Compressor, Limiter, Phaser,
                        HighpassFilter, LowpassFilter, LowShelfFilter,
                        HighShelfFilter, PeakFilter)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import synth as S  # noqa: E402
import synth_goa as G  # noqa: E402
from synth import m, SR  # noqa: E402

BPM = 142.0
SPB = 60.0 / BPM
NBARS = 104
TAIL = 5.0
N = int((NBARS * 4 * SPB + TAIL) * SR)
_rb = os.environ.get('NEW05_BARS')
RB0, RB1 = (int(v) for v in _rb.split(',')) if _rb else (0, NBARS)


def T(bar, beat=0.0):
    return (bar * 4 + beat) * SPB


def B(beats):
    return beats * SPB


def on(b0, b1):
    """True when bars [b0, b1) overlap the render range."""
    return b0 < RB1 + 1 and b1 > RB0 - 1


# ------------------------------------------------------------- harmony

PAD = {
    'C#5':  ['C#4', 'G#4', 'C#5'],
    'C#m':  ['C#4', 'G#4', 'B4', 'E5'],
    'E':    ['B3', 'E4', 'G#4', 'B4'],
    'F#s':  ['F#3', 'C#4', 'G#4', 'C#5'],
    'G#s':  ['G#3', 'C#4', 'D#4', 'G#4'],
    'Dlyd': ['A3', 'D4', 'G#4', 'C#5'],
}
ROOT = {'C#5': 37, 'C#m': 37, 'E': 40, 'F#s': 42, 'G#s': 44, 'Dlyd': 38}
PEAK_PROG = ['C#m', 'C#m', 'E', 'F#s', 'C#m', 'C#m', 'E', 'G#s']
CALL_PROG = ['C#5', 'C#5', 'Dlyd', 'Dlyd']
CHORDS = (['C#5'] * 8 +            # 0   intro
          ['C#m'] * 16 +           # 8   groove A (counter-line B)
          CALL_PROG * 4 +          # 24  groove B (the Phrygian call)
          ['Dlyd'] * 8 +           # 40  breakdown, D pedal
          PEAK_PROG +              # 48  breakdown 2, melody preview
          PEAK_PROG * 2 +          # 56  peak 1
          CALL_PROG * 2 +          # 72  acid bridge
          PEAK_PROG * 2 +          # 80  peak 2
          ['C#m'] * 4 + ['Dlyd'] * 2 + ['C#m'] * 2)   # 96 outro, final hit at 102
assert len(CHORDS) == NBARS
# pitch-class mode of the acid / lead material per bar: 'phr' C# Phrygian,
# 'aeo' C# Aeolian (D -> D#, in the euphoric sections)
MODE = ['aeo' if CHORDS[b] in ('E', 'F#s', 'G#s') or (48 <= b < 72) or (80 <= b < 96) else 'phr'
        for b in range(NBARS)]

PHR = [1, 2, 4, 6, 8, 9, 11]       # C# D E F# G# A B


def step_up(md):
    """Diatonic step up inside C# Phrygian (= D Lydian): C#->D, D->E, G#->A ..."""
    pc = md % 12
    if pc not in PHR:
        return md + 1
    i = PHR.index(pc)
    nxt = PHR[(i + 1) % 7]
    return md + ((nxt - pc) % 12)


def modal(md, b):
    """Adapt Phrygian material to the bar's chord: D bars step up, Aeolian D -> D#."""
    ch = CHORDS[b]
    if ch == 'Dlyd':
        return step_up(md)
    if MODE[b] == 'aeo' and md % 12 == 2:
        return md + 1
    return md


# Fragment A, the acid riff (2 bars of 16ths).  '!' accent, '~' slide into the next step.
RIFF_A = ("C#3! C#4 . C#3 D3!~ D4 C#4 . G#3! C#4 . C#3~ E3 D3! C#4 B3~ "
          "C#4! . C#3 C#4 E4!~ D4 . C#3 G#3!~ B3 C#4 . C#5! B4~ G#4 D4").split()
# the bridge variant: more twisting, with the Phrygian A (b6) turns
RIFF_B = ("C#3! . C#3 D3~ E3 . C#4! . B3~ A3 G#3 . D4!~ C#4 . G#3 "
          "C#3! C#4 . C#3 D3!~ C#3 . G#3 A3!~ G#3 E3 . D3! . C#4~ D4").split()
# intro teaser: only the skeleton of the cell
RIFF_T = ("C#3! . . C#3 D3! . . . G#3 . . C#3 . D3 . . "
          "C#3! . . C#3 E3! . . . G#3 . . . C#4 . . .").split()

# Fragment B, the brass counter-line, developed (beats; note; length) - 8 bars
CLINE = [(-0.5, 'E4', .25), (-0.25, 'F#4', .25),
         (0, 'G#4', 1), (1, 'F#4', 1), (2, 'E4', 1), (3, 'F#4', 1),
         (4, 'G#4', 3), (7, 'A4', .5), (7.5, 'G#4', .5),
         (8, 'F#4', 3), (11, 'G#4', .5), (11.5, 'F#4', .5),
         (12, 'E4', 2), (14, 'D4', 1), (15, 'C#4', .5),
         (15.5, 'E4', .25), (15.75, 'F#4', .25),
         (16, 'G#4', 1), (17, 'F#4', 1), (18, 'E4', 1), (19, 'F#4', 1),
         (20, 'G#4', 2), (22, 'B4', 1), (23, 'C#5', 1),
         (24, 'D5', 3), (27, 'C#5', .5), (27.5, 'B4', .5),
         (28, 'C#5', 3.5)]
GATE16 = [1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0]

# Fragment A, the call (one 4-bar cycle over C# C# D D) and its answers
CALL = [(2, 'G#4', .25), (3, 'G#4', .5), (3.5, 'G#4', .5), (4, 'G#4', 2),
        (6, 'C#5', .25), (7, 'C#5', .5), (7.5, 'C#5', .5), (8, 'D5', 5)]
ANSWER1 = [(13, 'C#5', .5), (13.5, 'B4', .5), (14, 'A4', .5), (14.5, 'G#4', .5),
           (15, 'A4', .5), (15.5, 'G#4', .5)]
ANSWER2 = [(13, 'E5', .5), (13.5, 'D5', .5), (14, 'C#5', 1), (15, 'D5', .25),
           (15.25, 'C#5', .25), (15.5, 'B4', .5)]
CALL_STAB = [(2, .25), (3, .4), (3.5, .4), (4, 1.9), (6, .25), (7, .4), (7.5, .4), (8, 6.0)]

# Fragment C, the choir counter-melody, developed into the 8-bar peak melody
PEAK = [(0, 'G#4', 3.5), (3.5, 'B4', .5),
        (4, 'C#5', 2), (6, 'D#5', 2),
        (8, 'E5', 3), (11, 'D#5', .5), (11.5, 'E5', .5),
        (12, 'G#5', 4),
        (16, 'F#5', 1.5), (17.5, 'E5', .5), (18, 'D#5', 1), (19, 'E5', 1),
        (20, 'C#5', 2), (22, 'D#5', 2),
        (24, 'E5', 2), (26, 'G#5', 1), (27, 'F#5', 1),
        (28, 'C#5', 3), (31, 'D#5', .5), (31.5, 'B4', .5)]
PEAK_HARM = [(0, 'E4', 3.5), (3.5, 'G#4', .5),
             (4, 'G#4', 2), (6, 'B4', 2),
             (8, 'B4', 3), (11, 'B4', .5), (11.5, 'B4', .5),
             (12, 'C#5', 4),
             (16, 'C#5', 1.5), (17.5, 'B4', .5), (18, 'B4', 1), (19, 'B4', 1),
             (20, 'G#4', 2), (22, 'B4', 2),
             (24, 'B4', 2), (26, 'E5', 1), (27, 'C#5', 1),
             (28, 'G#4', 3), (31, 'G#4', .5), (31.5, 'G#4', .5)]
# the breakdown preview: the original choir rhythm, plain
CHOIR = [(0, 'G#4', 4), (4, 'C#5', 2), (6, 'D#5', 2), (8, 'E5', 4), (12, 'G#5', 4),
         (16, 'G#4', 4), (20, 'C#5', 2), (22, 'D#5', 2), (24, 'E5', 4), (28, 'C#5', 4)]

# twisting 16th arp of the peaks: chord tones low -> high, index pattern
ARPT = {'C#m': ['C#4', 'G#4', 'B4', 'E5'], 'E': ['B3', 'E4', 'G#4', 'B4'],
        'F#s': ['C#4', 'F#4', 'G#4', 'C#5'], 'G#s': ['C#4', 'D#4', 'G#4', 'C#5'],
        'C#5': ['C#4', 'G#4', 'B4', 'C#5'], 'Dlyd': ['D4', 'G#4', 'A4', 'C#5']}
ARPPAT = [0, 2, 1, 3, 2, 1, 0, 2, 1, 3, 2, 3, 1, 2, 0, 3]

# Fragment E, the conga / tom fill of bar 44 (beats; drum)
FILL = [(0, 'chi'), (.5, 'thi'), (1, 'thi'), (1, 'clo'), (1.5, 'thi'), (1.5, 'clo'), (1.75, 'thi'),
        (2, 'tmid'), (2, 'chi'), (2.5, 'tmid'), (3, 'sn'), (3.5, 'tlo')]

# --------------------------------------------------------------- buses

names = ['kick', 'snare', 'hats', 'cym', 'perc', 'bass', 'sub', 'acid', 'lead', 'sup', 'sup_hi', 'harm',
         'call', 'pad', 'bell', 'fx', 'impact']
bus = {k: S.Bus(N) for k in names}
kicks = []
_cache = {}


def cached(key, fn):
    if key not in _cache:
        _cache[key] = fn()
    return _cache[key]


def inr(b):
    return RB0 - 1 <= b < RB1 + 1


def place_kick(t, vel=1.0):
    bus['kick'].add(cached(('kick',), G.psy_kick), t, vel)
    kicks.append(t)


def four_floor(bar0, bar1, vel=1.0, skip_last_beats=0):
    for b in range(bar0, bar1):
        if not inr(b):
            continue
        for beat in range(4):
            if b == bar1 - 1 and beat >= 4 - skip_last_beats:
                continue
            place_kick(T(b, beat), vel)


_snv = [0]


def snare_at(t, vel):
    v = _snv[0] % 4
    _snv[0] += 1
    bus['snare'].add(cached(('sn', v), lambda: G.round_snare(1.0, v)), t, vel * (0.94 + 0.12 * S.rng().random()))


def snares(bar0, bar1, vel=1.0):
    for b in range(bar0, bar1):
        if inr(b):
            for beat in (1, 3):
                snare_at(T(b, beat), vel)


def hats(bar0, bar1, closed=0.3, opn=0.5, ride=0.0, shake=0.0):
    for b in range(bar0, bar1):
        if not inr(b):
            continue
        for beat in range(4):
            if opn:
                bus['hats'].add(G.psy_hat(1.0, True, 0.12), T(b, beat + .5), opn * (0.92 + 0.1 * S.rng().random()))
            if closed:
                for s16, acc in ((0, .55), (.25, .4), (.5, .0), (.75, .65)):
                    if acc:
                        bus['hats'].add(G.psy_hat(1.0, False, -0.22), T(b, beat + s16),
                                        closed * acc * (0.88 + 0.24 * S.rng().random()))
            if ride:
                bus['cym'].add(S.ride(pan=0.3), T(b, beat + .5), ride * (0.9 + 0.1 * S.rng().random()))
            if shake:
                for s16 in (.25, .75):
                    bus['perc'].add(G.shaker(1.0, 0.35), T(b, beat + s16), shake * (0.8 + 0.3 * S.rng().random()))


DRUM = {'chi': ('cg', 68, False, 0.35), 'clo': ('cg', 61, False, -0.3),
        'thi': ('tom', 56), 'tmid': ('tom', 52), 'tlo': ('tom', 49)}


def fill(bar, vel=1.0):
    if not inr(bar):
        return
    for beat, d in FILL:
        t = T(bar, beat)
        if d == 'sn':
            snare_at(t, vel * 0.9)
            continue
        spec = DRUM[d]
        if spec[0] == 'cg':
            bus['perc'].add(cached(('cg', spec[1]), lambda: G.conga(spec[1], 1.0, spec[2], spec[3])), t, vel * 0.8)
        else:
            bus['perc'].add(cached(('tom', spec[1]), lambda: S.tom(spec[1])), t, vel * 0.55)


def congas(bar0, bar1, vel=1.0):
    pat = [(.75, 61, False), (1.5, 68, False), (2.75, 61, False), (3.25, 68, True), (3.5, 61, False)]
    for b in range(bar0, bar1):
        if inr(b):
            for beat, md, slap in pat:
                pan = 0.35 if md == 68 else -0.3
                bus['perc'].add(cached(('cg', md, slap), lambda: G.conga(md, 1.0, slap, pan)), T(b, beat),
                                vel * (0.4 if not slap else 0.5) * (0.85 + 0.3 * S.rng().random()))


def crash_at(t, vel=1.0):
    bus['cym'].add(S.crash(), t, vel)


def rolling_bass(bar0, bar1, vel=1.0):
    """The off-kick K-b-b-b bass; a small octave/fifth turn every 4th bar."""
    for b in range(bar0, bar1):
        if not inr(b):
            continue
        r = ROOT[CHORDS[b]]
        for beat in range(4):
            notes = [r, r, r]
            if b % 4 == 3 and beat == 3:
                notes = [r, r + 12, r + 7]
            for s16, md, v in zip((.25, .5, .75), notes, (0.82, 1.0, 0.9)):
                t = T(b, beat + s16)
                ph = int(((float(S.hz(md)) * t) % 1.0) * 8) / 8.0   # free-running oscillator phase
                bus['bass'].add(cached(('rb', md, ph), lambda: G.roll_bass(md, phase=ph)), t, vel * v)


def sub_hold(bar0, bar1, vel=1.0):
    b = bar0
    while b < bar1:
        r = ROOT[CHORDS[b]]
        e = b
        while e < bar1 and ROOT[CHORDS[e]] == r:
            e += 1
        if on(b, e):
            bus['sub'].add(S.sub_bass(r, T(e) - T(b) - 0.03, 1.0, a=0.3, r=0.2, drive=1.1), T(b), vel)
        b = e


def pads(bar0, bar1, vel=1.0, attack=0.25, cut=2600.0):
    b = bar0
    while b < bar1:
        ch = CHORDS[b]
        e = b
        while e < bar1 and CHORDS[e] == ch:
            e += 1
        if on(b, e):
            dur = T(e) - T(b)
            for note in PAD[ch]:
                bus['pad'].add(S.supersaw(m(note), dur, 1.0, a=attack, d=0.8, s=0.85, r=0.5,
                                          cut=cut, detune=0.25), T(b), vel * 0.5)
        b = e


def parse_riff(tokens):
    out = []
    for tk in tokens:
        if tk == '.':
            out.append(None)
        else:
            out.append((tk.rstrip('!~'), '!' in tk, '~' in tk))
    return out


def acid_part(bar0, bar1, riff, cut_pts, res_pts, vel=1.0, envmod=2.6, decay=0.2, wave='saw', target='acid'):
    """cut_pts / res_pts: [(bar, value)], interpolated (cutoff log-linearly)."""
    if not on(bar0, bar1):
        return
    steps = parse_riff(riff)
    st = SPB / 4
    ev = []
    slide_in = False
    for b in range(bar0, bar1):
        for i in range(16):
            s = steps[((b - bar0) * 16 + i) % len(steps)]
            t0 = T(b, i * .25) - T(bar0)
            if s is None:
                slide_in = False
                continue
            name, accn, slide = s
            md = modal(m(name), b)
            t1 = t0 + (st if slide else 0.55 * st)
            ev.append((t0, t1, md, 1.0 if accn else 0.0, slide_in))
            slide_in = slide
    n = int((T(bar1) - T(bar0) + 0.1) * SR)
    bars = T(bar0) / (4 * SPB) + np.arange(n) / SR / (4 * SPB)
    cut = np.exp(np.interp(bars, [p[0] for p in cut_pts], np.log([p[1] for p in cut_pts])))
    res = np.interp(bars, [p[0] for p in res_pts], [p[1] for p in res_pts])
    y = G.acid(ev, n, cut, res, envmod=envmod, decay=decay, wave=wave)
    bus[target].add(np.stack([y, y]), T(bar0), vel)


def slide_next(ev, beat, dur, md, gate):
    for b2, d2, m2, _ in ev:
        if abs(b2 - (beat + dur)) < 1e-6:
            return d2 < 1.0 and not gate and m2 != md
    return False


def lead_line(bar0, notes, transpose=0, vel=1.0, cut=900.0, res=2.4, envmod=2.4, gate=None, lfo=0.0,
              target='lead', modal_on=True, vib=0.0, decay=0.16):
    """Monophonic twisting goa lead; held notes optionally re-gated in 16ths (gate)."""
    ev = []
    for beat, note, dur in notes:
        md = m(note) + transpose
        bar = bar0 + int((beat + 1e-6) // 4)
        if modal_on and 0 <= bar < NBARS:
            md = modal(md, bar)
        if gate and dur >= 1.0:
            k0 = int(round(beat * 4))
            for k in range(k0, k0 + int(round(dur * 4))):
                if gate[k % 16] or k == k0:
                    ev.append((k / 4, .25 * 0.8, md, 1.0 if k == k0 else 0.0))
        else:
            ev.append((beat, dur, md, 1.0))
    if not ev or not on(bar0 + int(ev[0][0] // 4), bar0 + int(ev[-1][0] // 4) + 1):
        return
    b0 = min(e[0] for e in ev)
    t_off = T(bar0, b0)
    events = []
    prev_end = None
    prev_md = None
    for beat, dur, md, ac in ev:
        t0 = T(bar0, beat) - t_off
        t1 = t0 + B(dur)
        slide = (prev_end is not None and abs(prev_end - t0) < 0.004 and dur < 1.0 and not gate
                 and md != prev_md)
        events.append((t0, t1 - (0.0 if slide_next(ev, beat, dur, md, gate) else 0.004), md, ac * 0.6, slide))
        prev_end = t1
        prev_md = md
    n = int((events[-1][1] + 0.3) * SR)
    y = G.goa_lead(events, n, cut, res, envmod=envmod, lfo_rate=lfo, lfo_oct=0.5 if lfo else 0.0,
                   vib=vib, decay=decay)
    bus[target].add(y, t_off, vel)


def saw_melody(target, bar0, notes, transpose=0, vel=1.0, cut=6500, voices=9, vib=0.11, attack=0.006,
               release=0.14, detune=0.2):
    for beat, note, dur in notes:
        bar = bar0 + int((beat + 1e-6) // 4)
        if not inr(bar):
            continue
        md = m(note) + transpose
        sig = S.supersaw(md, B(dur) - 0.01, 1.0, voices=voices, detune=detune, a=attack, d=0.6, s=0.8,
                         r=release, cut=cut, cut_env=3500, cut_decay=0.15, vib=vib if dur >= 1.5 else 0.0)
        bus[target].add(sig, T(bar0, beat), vel)


def call_stabs(bar0, vel=1.0, cut=2400):
    """The original rhythm of fragment A as soft brass chords (C#4+G#4, then G#4+D5)."""
    if not on(bar0, bar0 + 4):
        return
    for beat, dur in CALL_STAB:
        chord = ['C#4', 'G#4'] if beat < 6 else (['G#4', 'C#5'] if beat < 8 else ['G#4', 'D5'])
        for note in chord:
            bus['call'].add(cached(('cs', note, dur, cut), lambda: S.supersaw(
                m(note), B(dur), 1.0, voices=7, detune=0.16, a=0.004, d=0.5, s=0.55, r=0.25,
                cut=cut, cut_env=3000, cut_decay=0.1)), T(bar0, beat), vel * 0.5)


def arp16(bar0, bar1, vel=1.0, cut=1100.0, res=2.2):
    """Twisting 16th arp lead on the chord tones (peaks)."""
    for blk in range(bar0, bar1, 8):
        e = min(bar1, blk + 8)
        if not on(blk, e):
            continue
        notes = []
        for b in range(blk, e):
            tones = ARPT[CHORDS[b]]
            for i in range(16):
                notes.append(((b - blk) * 4 + i * .25, tones[ARPPAT[i]], .25))
        lead_line(blk, [(bt, nm, d * 0.85) for bt, nm, d in notes], vel=vel, cut=cut, res=res, envmod=2.2,
                  lfo=0.0, target='harm', modal_on=False, decay=0.07)


def bells(bar0, bar1, seq, vel=1.0, every=2.0):
    k = 0
    beat = 0.0
    while bar0 * 4 + beat < bar1 * 4:
        b = bar0 + int(beat // 4)
        if inr(b):
            md = m(seq[k % len(seq)])
            bus['bell'].add(cached(('bl', md), lambda: S.bell(md, B(.5))), T(bar0, beat), vel)
        k += 1
        beat += every


# ---------------------------------------------------------- arrangement

# INTRO 0-7: cosmic swirl, the C# drone, a distant acid teaser; kick + hats at 4
if on(0, 8):
    bus['fx'].add(G.swirl(T(8) + 2.0), 0.0, 0.9)
    bus['pad'].add(G.space_drone([37 + 12, 44 + 12, 49 + 12], T(8) + 1.0, cut=700), 0.0, 1.0)
    bells(0, 8, ['G#5', 'D6', 'C#6', 'A5', 'G#5', 'E5', 'B5', 'C#6'], vel=0.4, every=3.0)
acid_part(2, 8, RIFF_T, [(2, 260), (8, 700)], [(2, 2.6), (8, 3.0)], vel=0.8, envmod=2.4, decay=0.25)
four_floor(4, 8, 0.85)
hats(4, 8, closed=0.22, opn=0.0)
sub_hold(4, 8, 0.35)
if on(6, 8):
    bus['fx'].add(S.noise_riser(T(8) - T(6), 400, 6000, 2.0), T(6), 0.35)
    bus['cym'].add(S.reverse_cymbal(B(4)), T(7), 0.5)

# GROOVE A 8-23: kick + rolling bass + acid; the counter-line (fragment B) from 16
if on(8, 9):
    crash_at(T(8), 0.8)
four_floor(8, 24)
rolling_bass(8, 24)
hats(8, 16, closed=0.3, opn=0.45)
hats(16, 24, closed=0.33, opn=0.5)
snares(12, 24, 0.8)
congas(16, 24, 0.8)
fill(23)
acid_part(8, 24, RIFF_A, [(8, 450), (16, 900), (24, 1500)], [(8, 2.8), (24, 3.15)], vel=1.4)
pads(8, 24, vel=0.32, attack=1.5, cut=1300)
lead_line(16, CLINE, cut=650, res=2.6, envmod=2.6, gate=GATE16, lfo=0.125)
if on(15, 16):
    bus['cym'].add(S.reverse_cymbal(B(4)), T(15), 0.45)
if on(16, 17):
    crash_at(T(16), 0.6)

# GROOVE B 24-39: the C# / D Phrygian call (fragment A), acid follows the bII
if on(24, 25):
    crash_at(T(24), 0.85)
    bus['impact'].add(S.impact(), T(24), 0.5)
four_floor(24, 40, skip_last_beats=2)
rolling_bass(24, 40)
hats(24, 32, closed=0.33, opn=0.5)
hats(32, 40, closed=0.35, opn=0.52, ride=0.22)
snares(24, 40, 0.85)
congas(24, 40, 0.8)
fill(31, 0.9)
acid_part(24, 40, RIFF_A, [(24, 1000), (32, 1300), (40, 2000)], [(24, 3.0), (40, 3.3)], vel=1.2)
for c in range(24, 40, 4):
    call_stabs(c, vel=0.75 if c < 32 else 0.9)
    lead_line(c, CALL + (ANSWER1 if (c // 4) % 2 == 0 else ANSWER2), cut=800, res=2.5, envmod=2.4,
              lfo=0.0, vib=0.0, modal_on=False)
pads(32, 40, vel=0.3, attack=0.6, cut=1500)
if on(38, 40):
    bus['fx'].add(S.noise_riser(T(40) - T(38), 500, 7000, 2.0), T(38), 0.4)
    bus['cym'].add(S.reverse_cymbal(B(2)), T(39, 2), 0.6)

# BREAKDOWN 40-55: the D pedal (fragment D) with the call as a slow, wide
# statement; then the peak melody (fragment C) previewed, building into peak 1
if on(40, 41):
    crash_at(T(40), 0.7)
    bus['impact'].add(S.impact(), T(40), 0.7)
    bus['fx'].add(S.downlifter(B(8)), T(40), 0.3)
if on(40, 56):
    bus['fx'].add(G.swirl(T(56) - T(40), 350, 2200, rate=0.08, seed_phase=1.3), T(40), 0.8)
sub_hold(40, 56, 0.8)
pads(40, 48, vel=0.6, attack=1.5, cut=1700)
for c in (40, 44):
    saw_melody('call', c, CALL + (ANSWER1 if c == 40 else ANSWER2), vel=0.5, cut=2600, voices=7, vib=0.12,
               attack=0.03, release=0.5)
bells(40, 48, ['C#6', 'G#5', 'A5', 'D6', 'C#6', 'E6', 'G#5', 'B5'], vel=0.35, every=1.5)
acid_part(44, 56, RIFF_A, [(44, 220), (52, 450), (56, 1400)], [(44, 2.8), (56, 3.3)], vel=0.75, decay=0.25)
pads(48, 56, vel=0.55, attack=0.6, cut=1900)
saw_melody('sup', 48, CHOIR, vel=0.6, cut=2600, voices=7, vib=0.1, attack=0.08, release=0.4)
rolling_bass(52, 56, 0.6)
hats(52, 56, closed=0.22, opn=0.0)
if on(52, 56):
    bus['fx'].add(S.noise_riser(T(55, 3) - T(52), 300, 9000, 2.2), T(52), 0.55)
    bus['fx'].add(S.pitch_riser(T(55, 3) - T(53), 49, 73), T(53), 0.35)
    bus['cym'].add(S.reverse_cymbal(B(3)), T(55, 0), 0.6)
fill(55, 0.85)

# PEAK 1 56-71: euphoric supersaw melody (fragment C) over C#m C#m E F# ...
for t in (56, 64):
    if on(t, t + 1):
        crash_at(T(t), 1.0)
if on(56, 57):
    bus['impact'].add(S.impact(), T(56), 1.0)
four_floor(56, 72)
rolling_bass(56, 72)
hats(56, 72, closed=0.36, opn=0.55, ride=0.2)
snares(56, 72, 0.9)
congas(56, 72, 0.7)
fill(63, 0.7)
fill(71)
acid_part(56, 72, RIFF_A, [(56, 800), (64, 1000), (72, 1300)], [(56, 3.0), (72, 3.2)], vel=0.62)
pads(56, 72, vel=0.55, attack=0.05, cut=2800)
for bb in (56, 64):
    saw_melody('sup', bb, PEAK, vel=1.0)
    saw_melody('sup_hi', bb, PEAK, transpose=12, vel=0.32, cut=5000, voices=5)

# ACID BRIDGE 72-79: C# / D, the acid takes the lead (RIFF_B), the call answers
if on(72, 73):
    crash_at(T(72), 0.9)
    bus['impact'].add(S.impact(), T(72), 0.6)
four_floor(72, 80)
rolling_bass(72, 80)
hats(72, 80, closed=0.36, opn=0.55)
snares(72, 80, 0.85)
congas(72, 80, 0.9)
acid_part(72, 80, RIFF_B, [(72, 1000), (76, 1800), (80, 2800)], [(72, 3.2), (80, 3.5)], vel=1.45,
          envmod=2.9, decay=0.22)
for c in (72, 76):
    call_stabs(c, vel=0.7, cut=2000)
lead_line(76, CALL + ANSWER2, cut=900, res=2.5, envmod=2.4, modal_on=False, vel=0.8)
pads(76, 80, vel=0.3, attack=1.0, cut=1500)
fill(79)
if on(78, 80):
    bus['fx'].add(S.noise_riser(T(80) - T(78), 500, 9000, 2.2), T(78), 0.45)
    bus['cym'].add(S.reverse_cymbal(B(3)), T(79, 1), 0.6)

# PEAK 2 80-95: melody + octave + harmony + the twisting 16th arp
for t in (80, 88):
    if on(t, t + 1):
        crash_at(T(t), 1.0)
if on(80, 81):
    bus['impact'].add(S.impact(), T(80), 1.0)
four_floor(80, 96)
rolling_bass(80, 96)
hats(80, 96, closed=0.38, opn=0.55, ride=0.25, shake=0.25)
snares(80, 96, 0.9)
congas(80, 96, 0.7)
fill(87, 0.7)
fill(95)
acid_part(80, 96, RIFF_A, [(80, 900), (96, 1500)], [(80, 3.1), (96, 3.3)], vel=0.6)
pads(80, 96, vel=0.58, attack=0.05, cut=3000)
arp16(80, 96, vel=0.55)
for bb in (80, 88):
    saw_melody('sup', bb, PEAK, vel=1.0)
    saw_melody('sup_hi', bb, PEAK, transpose=12, vel=0.4, cut=5500, voices=5)
    saw_melody('harm', bb, PEAK_HARM, vel=0.5, cut=4000, voices=7)

# OUTRO 96-103: back to the groove and the call; D -> C#m (Phrygian cadence), final hit
if on(96, 97):
    crash_at(T(96), 0.85)
four_floor(96, 102)
rolling_bass(96, 102)
hats(96, 102, closed=0.33, opn=0.5)
snares(96, 102, 0.8)
acid_part(96, 102, RIFF_A, [(96, 1400), (102, 800)], [(96, 3.1), (102, 2.8)], vel=1.15)
call_stabs(98, vel=0.8)
lead_line(98, CALL, cut=800, res=2.5, envmod=2.4, modal_on=False, vel=0.85)
pads(98, 102, vel=0.4, attack=0.4, cut=1800)
if on(101, 103):
    bus['cym'].add(S.reverse_cymbal(B(2)), T(101, 2), 0.5)
if on(102, 104):
    place_kick(T(102), 1.0)
    crash_at(T(102), 0.9)
    bus['impact'].add(S.impact(), T(102), 0.8)
    for note in ['C#3', 'G#3', 'C#4', 'E4', 'G#4', 'B4']:
        bus['pad'].add(S.supersaw(m(note), B(4), 1.0, a=0.003, d=1.0, s=0.5, r=0.45, cut=3000, cut_env=3000,
                                  cut_decay=0.3, detune=0.22), T(102), 0.42)
    bus['sup'].add(S.supersaw(m('G#4'), B(4), 1.0, voices=9, a=0.004, d=0.9, s=0.5, r=0.4, cut=5000,
                              cut_env=3000, cut_decay=0.2, vib=0.08), T(102), 0.6)
    bus['sub'].add(S.sub_bass(37, B(4.5), 1.0, a=0.004, r=0.3), T(102), 0.9)

# ------------------------------------------------------------- mix


def stage(x, *plugins):
    return Pedalboard(list(plugins))(np.ascontiguousarray(x, np.float32), SR)


duck_mid = S.sidechain(N, kicks, depth=0.55, release=0.28)
duck_soft = S.sidechain(N, kicks, depth=0.3, release=0.25)

GAIN = {  # dB
    'kick': -3.0, 'snare': -6.0, 'hats': 4.0, 'cym': -14.0, 'perc': -8.0,
    'bass': 3.0, 'sub': -11.0, 'acid': -7.0, 'lead': -1.0, 'sup': -2.5, 'sup_hi': -7.0, 'harm': -8.0,
    'call': -2.5, 'pad': -2.0, 'bell': -10.0, 'fx': -12.0, 'impact': -9.0,
}
DUCK = {'pad': duck_mid, 'call': duck_soft, 'sup': duck_soft, 'sup_hi': duck_soft, 'harm': duck_soft,
        'lead': duck_soft, 'acid': duck_soft, 'fx': duck_soft, 'bell': duck_soft, 'sub': duck_mid}
VERB = {'snare': .12, 'perc': .2, 'pad': .35, 'acid': .1, 'lead': .2, 'sup': .25, 'sup_hi': .4, 'harm': .3,
        'call': .35, 'bell': .5, 'fx': .4}
DELAY = {'acid': .14, 'lead': .28, 'sup': .14, 'sup_hi': .3, 'harm': .25, 'bell': .4, 'call': .15, 'perc': .08}

bus['acid'].x = stage(bus['acid'].x, HighpassFilter(150), PeakFilter(260, -4.0, 0.8), PeakFilter(1200, -1.5, 1.0),
                      LowpassFilter(7000))
bus['lead'].x = stage(bus['lead'].x, HighpassFilter(200), LowpassFilter(8000))
bus['harm'].x = stage(bus['harm'].x, HighpassFilter(250))
bus['pad'].x = stage(bus['pad'].x, HighpassFilter(200), PeakFilter(450, -3.0, 0.9), PeakFilter(1400, -2.0, 0.9),
                     Phaser(rate_hz=0.12, depth=0.4, centre_frequency_hz=900, feedback=0.2, mix=0.3),
                     Chorus(rate_hz=0.35, depth=0.25, centre_delay_ms=8, mix=0.3))
bus['sup'].x = stage(bus['sup'].x, HighpassFilter(250), PeakFilter(500, -2.0, 0.9), PeakFilter(2800, 1.5, 0.8))
bus['sup_hi'].x = stage(bus['sup_hi'].x, HighpassFilter(500), LowpassFilter(9000))
bus['call'].x = stage(bus['call'].x, HighpassFilter(200))
bus['snare'].x = stage(bus['snare'].x, HighpassFilter(120))
bus['hats'].x = stage(bus['hats'].x, HighpassFilter(3500), LowpassFilter(13000))
bus['cym'].x = stage(bus['cym'].x, HighpassFilter(2500), LowpassFilter(10000))
bus['perc'].x = stage(bus['perc'].x, HighpassFilter(90))
bus['fx'].x = stage(bus['fx'].x, HighpassFilter(250), LowpassFilter(9000))
bus['bell'].x = stage(bus['bell'].x, HighpassFilter(400))
bus['bass'].x = stage(bus['bass'].x, HighpassFilter(38), PeakFilter(180, -3.0, 1.0), LowpassFilter(1400))
bus['sub'].x = stage(bus['sub'].x, LowpassFilter(140))

mix = np.zeros((2, N), np.float32)
verb_send = np.zeros((2, N), np.float32)
delay_send = np.zeros((2, N), np.float32)
levels = {}
MUTE = set(filter(None, os.environ.get('NEW05_MUTE', '').split(',')))
meter = pyln.Meter(SR)
for k in names:
    if k in MUTE:
        bus[k] = None
        continue
    x = bus[k].x
    x *= np.float32(10 ** (GAIN[k] / 20))
    if k in DUCK:
        x *= DUCK[k][None, :]
    seg = x.T.astype(np.float64)
    act = np.abs(seg).max(axis=1) > 1e-4
    levels[k] = meter.integrated_loudness(seg[act]) if act.sum() > SR else -99.0
    del seg
    mix += x
    if k in VERB:
        verb_send += x * VERB[k]
    if k in DELAY:
        delay_send += x * DELAY[k]
    bus[k] = None

pre = int(0.02 * SR)
vs = np.zeros_like(verb_send)
vs[:, pre:] = verb_send[:, :-pre]
del verb_send
verb = stage(vs, HighpassFilter(350), LowpassFilter(8500),
             Reverb(room_size=0.9, damping=0.5, wet_level=1.0, dry_level=0.0, width=1.0))
del vs
verb *= duck_soft[None, :]
dly = S.pingpong(delay_send, B(0.75), feedback=0.45, repeats=8)
del delay_send
dly = stage(dly, Reverb(room_size=0.5, damping=0.6, wet_level=0.25, dry_level=1.0, width=1.0))
mix += verb * 0.85 + dly * 0.55
del verb, dly

# section dynamics (dB at bar lines, linear in between)
SECTION_DB = [(0, -5), (8, -3.5), (24, -2.5), (39.9, -1.5), (40, -4.5), (48, -5), (55.5, -2.5),
              (56, 0), (71.9, 0), (72, -1.5), (79.9, -1), (80, 0.5), (95.9, 0.5), (96, -1.5),
              (102, -1.5), (104, -1.5)]
bars_axis = np.arange(N) / SR / (4 * SPB)
sec = np.interp(bars_axis, [p_[0] for p_ in SECTION_DB], [p_[1] for p_ in SECTION_DB])
mix *= (10 ** (sec / 20)).astype(np.float32)[None, :]
del bars_axis, sec

# crop the render range
if _rb:
    i0, i1 = int(T(RB0) * SR), min(N, int((T(RB1) + 2.0) * SR))
    mix = np.ascontiguousarray(mix[:, i0:i1])

# ---------------------------------------------------------- master
mix = stage(mix, HighpassFilter(28))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-14.0 - loud) / 20))
mix = stage(mix,
            LowShelfFilter(90, 0.0, 0.7), HighShelfFilter(9000, -1.5, 0.7),
            Compressor(threshold_db=-14, ratio=2.0, attack_ms=20, release_ms=150))
mix = np.tanh(mix * 1.15) / 1.15
mix = stage(mix, Limiter(threshold_db=-1.5, release_ms=80))
loud = meter.integrated_loudness(mix.T.astype(np.float64))
mix *= np.float32(10 ** ((-16.0 - loud) / 20))

# trim to the last audible sample, short fade
env = np.max(np.abs(mix), axis=0)
last = np.nonzero(env > 10 ** (-48 / 20))[0][-1]
mix = mix[:, : last + 1]
fade = int(0.6 * SR)
mix[:, -fade:] *= np.linspace(1, 0, fade)[None, :].astype(np.float32)

out = sys.argv[1] if len(sys.argv) > 1 else 'new05_goa.wav'
sf_.write(out, mix.T, SR, subtype='FLOAT')
print('wrote', out, f'{mix.shape[1] / SR:.1f} s')
print('bus loudness while playing (LUFS):')
for k, v in sorted(levels.items(), key=lambda kv: -kv[1]):
    print(f'  {k:8s} {v:6.1f}')
