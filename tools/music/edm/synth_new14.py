# synth_new14.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments for new14_electro.py ("Cold Signal", Detroit electro): a TR-808
# style kit (a long tuned boom kick, a short round "pdlap" snare with four
# round-robins, rimshot ticks, tuned toms/congas), a sonar ping, and helpers
# for the monophonic lines.  The squelch bass and the cold arpeggio use the
# 303-style ladder voice of synth_goa.py; the chord stab, string machine,
# chorus and dub delay come from synth_new08.py.  Everything is synthesized;
# no samples.  Builds on tools/music/edm/synth.py (unchanged).
import numpy as np
import scipy.signal as ss

import synth as S
from synth import SR, hz, butter

_g = np.random.default_rng(1414)


def _t(n):
    return np.arange(n) / SR


def kick808(vel=1.0, f_end=49.0, f_start=150.0, decay=0.30, length=0.62):
    """808 boom: sine with a slow-ish pitch sweep down to G1 (the fifth of C
    minor), a long body, a soft click; gentle saturation."""
    n = int(length * SR)
    t = _t(n)
    f = f_end + (f_start - f_end) * np.exp(-t / 0.045) + 260 * np.exp(-t / 0.004)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / decay) * np.minimum(t / 0.0007, 1)
    amp *= np.where(t > length - 0.08, np.clip((length - t) / 0.08, 0, 1), 1)
    y = np.tanh(1.6 * body * amp) / np.tanh(1.6)
    click = butter(_g.standard_normal(n), [1200, 5000], 'band', 2) * np.exp(-t / 0.0018) * 0.18
    return np.stack([y + click, y + click]) * vel


_SN_RR = [(1.000, 1.00, 1.00), (0.986, 0.92, 0.95), (1.013, 1.07, 0.97), (0.995, 0.97, 0.91)]


def snare(vel=1.0, var=0, f0=196.0, soft=0.0):
    """Short round "pdlap": a body tuned to G3 (the key's fifth) with a fast
    pitch drop and a second mode at 1.6x, a 1.2-3.8 kHz snap, a small clap
    flam inside the hit, a short dark room; ~110 ms to -20 dB, LP 7 kHz.
    var: round-robin 0-3; soft (0..1): quieter hits get less snap."""
    pm, tone, lvl = _SN_RR[var % 4]
    g = np.random.default_rng(140 + var)
    n = int(0.26 * SR)
    t = _t(n)
    f = f0 * pm * (1 + 0.45 * np.exp(-t / 0.010))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * np.exp(-t / 0.055) + 0.32 * np.sin(1.6 * ph) * np.exp(-t / 0.032)
    snp = butter(g.standard_normal(n), [1200 * tone, 3800 * tone], 'band', 2)
    snp *= (np.exp(-t / 0.013) + 0.18 * np.exp(-t / 0.04)) * (1 - 0.7 * soft)
    clap = np.zeros(n)
    for off in (0.0, 0.007, 0.015):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.003), 0)
    clap = butter(g.standard_normal(n) * clap, [1000, 3000], 'band', 2) * 0.38 * (1 - 0.8 * soft)
    y = 1.0 * body + 0.6 * snp + 1.2 * clap
    y = np.tanh(1.3 * y) / 1.1 * np.minimum(t / 0.0005, 1)
    y = butter(y, 7000, 'low', 2)
    # short dark room
    L = int(0.1 * SR)
    ir = g.standard_normal((2, L)) * np.exp(-_t(L) / 0.03)
    ir = butter(butter(ir, 3000, 'low', 2), 300, 'high', 2)
    ir /= np.sqrt(np.sum(ir[0] ** 2))
    wet = np.stack([ss.fftconvolve(y, ir[c])[:n] for c in range(2)]) * 0.22
    out = np.stack([y, np.roll(y, 17)]) + wet
    fe = int(0.02 * SR)
    out[:, -fe:] *= np.linspace(1, 0, fe)
    return out * vel * lvl


def rim(vel=1.0, var=0, pan=-0.35):
    """808 rimshot tick: two damped partials (about 1.7 kHz and 480 Hz) plus a
    tiny band-limited click; very short."""
    g = np.random.default_rng(500 + var)
    n = int(0.06 * SR)
    t = _t(n)
    pm = 1 + 0.015 * (g.random() - 0.5)
    y = (np.sin(2 * np.pi * 1690 * pm * t) * np.exp(-t / 0.006) +
         0.6 * np.sin(2 * np.pi * 480 * pm * t) * np.exp(-t / 0.009))
    y += butter(g.standard_normal(n), [1500, 5000], 'band', 2) * np.exp(-t / 0.002) * 0.4
    y = butter(y, 6500, 'low', 2) * np.minimum(t / 0.0003, 1)
    return S.pan2(y * vel * 0.6, pan)


def tom808(midi, vel=1.0, pan=0.0, decay=0.2):
    """808 tom / conga: a tuned sine with a short downward pitch blip."""
    n = int(0.55 * SR)
    t = _t(n)
    f = hz(midi) * (1 + 0.3 * np.exp(-t / 0.012))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / decay) * np.minimum(t / 0.0008, 1)
    y += butter(_g.standard_normal(n), 2200, 'low', 2) * np.exp(-t / 0.003) * 0.1
    y = np.tanh(1.2 * y)
    return S.pan2(y * vel * 0.8, pan)


def ping(midi, vel=1.0, decay=1.1, pan=0.0):
    """Sonar ping: a pure sine with a faint inharmonic shimmer (2.76x) and a
    slow decay; the dub delay on the bus makes the echoes."""
    n = int((decay * 5) * SR)
    t = _t(n)
    f = float(hz(midi))
    y = np.sin(2 * np.pi * f * t) + 0.08 * np.sin(2 * np.pi * 2.76 * f * t) * np.exp(-t / 0.2)
    y *= np.exp(-t / decay) * np.minimum(t / 0.003, 1)
    return S.pan2(y * vel * 0.5, pan)


def boom(vel=1.0, f=32.7):
    """Low sine boom for the final hit (C1, decays over ~2.5 s)."""
    n = int(3.2 * SR)
    t = _t(n)
    fr = f + 40 * np.exp(-t / 0.06)
    y = np.sin(2 * np.pi * np.cumsum(fr) / SR) * np.exp(-t / 0.8) * np.minimum(t / 0.002, 1)
    y = np.tanh(1.4 * y)
    return np.stack([y, y]) * vel
