# synth_new17.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments for new17_hybrid.py ("Iron Armada", hybrid orchestral /
# industrial battle music): a hard industrial kick tuned to G1, a deep round
# "taiko snare" (tuned pdlap body + sampled giant-drum hand hit, no noise
# tail), layered taikos (sampled giant ethnic drum + synthesized body), a
# distorted synth bass with a FIXED filter (no sweeps) and a clean mono sub,
# a dark distorted "grind" stab, a metal-hit kit (anvil / brake drum / modal
# pipes), and a dark riser.  Built on synth.py, synth_orch.py and
# synth_new06.py, which are imported and not modified.
#
# Samples: VSCO-2 Community Edition by Versilian Studios (Sam Gossner, Simon
# Dalzell, Elan Hickler), CC0 1.0 / public domain,
# https://github.com/sgossner/VSCO-2-CE -- giant ethnic drum (mallet, hand,
# sticks), concert bass drum, anvil, brake drum, gong, crash.  Read as data
# with soundfile.  Everything else is synthesized.
import numpy as np

import synth as S
import synth_orch as O
import synth_new06 as I
from synth import SR, hz, saw, sine, svf, butter


# ----------------------------------------------------------------- drums

def kick(vel=1.0, var=0):
    """Industrial kick: hard, short, saturated, tuned to G1 (the fifth of C minor)."""
    return I.ebm_kick(vel, f_end=49.0, var=var)


_SN = [(1.000, 1.00, 0), (0.986, 0.92, 1), (1.014, 1.07, 2), (0.996, 0.96, 3)]


def taiko_snare(vel=1.0, var=0, f0=196.0):
    """Deep, round backbeat: a tuned body at G3 (196 Hz) with a fast pitch drop
    and a 1.6x mode, a short 1-3.5 kHz snap, a sampled giant-drum hand slap for
    the skin, ~130 ms to -20 dB, low-passed at 6.5 kHz.  4 round-robin variants
    (+-1.5 % pitch, +-0.8 dB, a different snap tone)."""
    pm, tone, k = _SN[var % 4]
    n = int(0.32 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(170 + var)
    f = f0 * pm * (1 + 0.7 * np.exp(-t / 0.008))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * np.exp(-t / 0.07) + 0.45 * np.sin(1.6 * ph) * np.exp(-t / 0.04)
    snap = butter(g.standard_normal(n), [1000 * tone, 3500 * tone], 'band', 2)
    snap *= np.exp(-t / 0.014) + 0.15 * np.exp(-t / 0.05)
    y = np.tanh(1.4 * (body + 0.45 * snap)) * np.minimum(t / 0.0004, 1)
    y = np.stack([y, np.roll(y, 13)])
    hand = O.perc('taiko_hand', 0.85)[:, :n]
    th = np.arange(hand.shape[1]) / SR
    hand = butter(hand, 120, 'high', 2) * np.exp(-th / 0.09)
    y[:, :hand.shape[1]] += hand * 0.9
    y = butter(y, 6500, 'low', 2)
    fe = int(0.03 * SR)
    y[:, -fe:] *= np.linspace(1, 0, fe)
    lvl = 10 ** ((0.8 * (var % 3 - 1)) / 20)
    return (y * vel * lvl / (np.max(np.abs(y)) + 1e-9)).astype(np.float32)


def taiko(vel=1.0, f0=65.41, src='taiko'):
    """Big drum: sampled giant ethnic drum (mallet) over a synthesized body tuned to C2."""
    smp = O.perc(src, vel)
    syn = O.taiko_synth(1.0, f0)
    n = max(smp.shape[1], syn.shape[1])
    y = np.zeros((2, n), np.float32)
    y[:, :smp.shape[1]] += smp * 1.2
    y[:, :syn.shape[1]] += syn * (0.6 + 0.4 * vel) * (1.0 if src == 'taiko' else 0.3)
    return y * vel


def tom(midi, vel=1.0, seed=0):
    """Low tom: pitched body with a skin, a bit of drive, dark (taiko-like)."""
    n = int(0.7 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(900 + seed)
    f = hz(midi) * (1 + 0.55 * np.exp(-t / 0.03))
    ph = 2 * np.pi * np.cumsum(f) / SR
    y = np.sin(ph) * np.exp(-t / 0.22) + 0.3 * np.sin(1.5 * ph) * np.exp(-t / 0.1)
    y += butter(g.standard_normal(n), [200, 1800], 'band', 2) * np.exp(-t / 0.018) * 0.5
    y = np.tanh(1.6 * y) * np.minimum(t / 0.0005, 1)
    fe = int(0.05 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 7)]).astype(np.float32) * vel * 0.8


def metal(kind, vel=1.0, var=0, semis=0.0):
    """Industrial metal hits.  'brake': brake drum, dark; 'anvil': anvil pitched
    down a 12th; 'pipe': modal pipe on C; 'stamp': machinery stamp."""
    if kind == 'brake':
        return I.clank('brake', -4 + semis, vel, var, maxlen=0.5, lp=5000, decay=0.12)
    if kind == 'anvil':
        return I.clank('anvil', -19 + semis, vel, var, maxlen=1.0, lp=3800, decay=0.35)
    if kind == 'pipe':
        return I.metal_hit(int(60 + semis), vel, decay=0.18, bright=0.6, seed=var)
    if kind == 'stamp':
        return I.press(int(48 + semis), var) * vel * 0.6
    raise ValueError(kind)


def tick(vel=1.0, pan=0.0, seed=0):
    return I.tick(vel, pan, seed)


# ------------------------------------------------------------------ bass

def dist_bass(midi, dur, vel=1.0, cut=1100.0, drive=4.0):
    """Distorted synth bass: two detuned saws + a square, a FIXED low-pass (no
    sweep), driven hard, a second fixed low-pass at 2.8 kHz to keep it dark.
    Mono (2, n).  The clean sub is a separate voice."""
    r = 0.015
    n = int((dur + 6 * r) * SR)
    f = hz(midi)
    t = np.arange(n) / SR
    w = 0.5 * saw(f * 2 ** (-0.07 / 12), n, 0.1) + 0.5 * saw(f * 2 ** (0.07 / 12), n, 0.6) + \
        0.35 * np.sign(sine(f, n, 0.25))
    w = svf(w, cut, 1.2)
    e = np.where(t < dur, 1.0, np.exp(-(t - dur) / r)) * np.minimum(t / 0.002, 1)
    e *= 0.82 + 0.18 * np.exp(-t / 0.05)                  # small accent, no filter move
    y = np.tanh(drive * w * e) / np.tanh(drive)
    y = butter(butter(y, 2800, 'low', 2), 55, 'high', 2) * vel
    return np.stack([y, y]).astype(np.float32)


def sub(midi, dur, vel=1.0, r=0.04):
    return S.sub_bass(midi, dur, vel, a=0.004, r=r, drive=1.2).astype(np.float32)


def grind_stab(midis, dur, vel=1.0):
    """Short distorted saw stab (fixed tone) for the industrial accents."""
    n = int((dur + 0.12) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for md in midis:
        for o, p in ((-0.12, -0.7), (0.0, 0.0), (0.12, 0.7)):
            w = saw(hz(md + o), n, S.rng().random())
            L += w * np.sqrt((1 - p) / 2)
            R += w * np.sqrt((1 + p) / 2)
    x = svf(np.stack([L, R]) / np.sqrt(3 * len(midis)), 1500.0, 0.9)
    e = np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.03)) * np.minimum(t / 0.002, 1)
    x = np.tanh(3.0 * x * e) / np.tanh(3.0)
    x = butter(butter(x, 3000, 'low', 2), 150, 'high', 2)
    return (x * vel).astype(np.float32)


# ------------------------------------------------------------------- fx

def riser(length, seed=0):
    """Dark tonal riser: a C-G fifth of saws gliding up an octave through a
    closing-opening low-pass, plus a band-passed noise bed that stays under
    5 kHz (no bright 'wipe')."""
    n = int(length * SR)
    u = np.linspace(0, 1, n)
    g = np.random.default_rng(1700 + seed)
    out = np.zeros((2, n))
    for base, p in ((36, -0.5), (43, 0.5), (48, 0.0)):
        md = base + 12 * u ** 1.8
        w = saw(hz(md - 0.08), n, g.random()) + saw(hz(md + 0.08), n, g.random())
        out[0] += w * np.sqrt((1 - p) / 2)
        out[1] += w * np.sqrt((1 + p) / 2)
    out = svf(out, 200 + 2800 * u ** 2, 1.1)
    nz = svf(g.standard_normal((2, n)), 400 * (8 ** u), 2.0, 'bp')
    out = out * 0.25 + nz * 0.4
    out = butter(out, 5000, 'low', 2)
    return (out * u ** 2).astype(np.float32)
