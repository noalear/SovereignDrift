# synth_new07.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for new track 07, "Rimward Patrol" (new07_cinematic.py), a
# cinematic orchestral/electronic piece.  Builds on synth.py and
# synth_orch.py without changing them.  Everything here is synthesized.
#
#   round_snare()  short, rounded, tonal "pdlap" snare (tuned body on G3 with a
#                  fast pitch drop, short 1-4 kHz snap, tiny rim and clap flam,
#                  ~100 ms decay, low-passed); 4 round-robin variants, ghost mode
#   soft_hat()     dark, short metallic hat (band-limited, no fizz above 11 kHz)
#   shaker()       soft 16th shaker
#   deep_kick()    round kick tuned to G1 (the fifth of C minor)
#   vox()          choir-like "oh/oo" pad: detuned saws with slow drift and
#                  vibrato through a STATIC formant bank (no vowel sweeps)
#   space_ir()     long, dark hall impulse response ("vast distance")
import numpy as np

import synth as S
import synth_orch as O
from synth import SR, hz

# ------------------------------------------------------------------ drums

_SN_RR = [(1.000, 1.00, 1.00), (0.986, 0.93, 0.95), (1.013, 1.06, 0.97), (0.996, 0.97, 0.92)]
_sn_cache = {}


def _snare_raw(var, dark):
    pm, tone, lvl = _SN_RR[var % 4]
    n = int(0.24 * SR)
    t = np.arange(n) / SR
    rg = np.random.default_rng(700 + var + (50 if dark else 0))
    f = 196.0 * pm * (1 + 0.6 * np.exp(-t / 0.009))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.052) + 0.30 * np.sin(1.47 * ph) * np.exp(-t / 0.032) +
            0.12 * np.sin(2.13 * ph) * np.exp(-t / 0.020))
    snap = S.butter(rg.standard_normal(n), [1000 * tone, 3800 * tone], 'band', 2)
    snap *= (np.exp(-t / 0.017) + 0.2 * np.exp(-t / 0.055)) * (1 - 0.8 * dark)
    rim = np.sin(2 * np.pi * 1520 * tone * t) * np.exp(-t / 0.004) * 0.18 * (1 - 0.7 * dark)
    ce = np.zeros(n)
    for off in (0.0, 0.006, 0.013):
        u = t - off
        ce += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.003), 0)
    clap = S.butter(rg.standard_normal(n) * ce, [1100, 3000], 'band', 2) * 0.3 * (1 - 0.9 * dark)
    y = 0.85 * body + 1.9 * snap + 1.2 * rim + 2.2 * clap
    y = np.tanh(1.3 * y) / 1.1 * np.minimum(t / 0.0005, 1)
    y = S.butter(y, 6500 - 4000 * dark, 'low', 2)
    y = S.butter(y, 110, 'high', 2)
    fe = int(0.02 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    y = y / (np.abs(y).max() + 1e-9) * 0.9 * lvl
    return np.stack([y, np.roll(y, 12)]).astype(np.float32)


def round_snare(vel=1.0, var=0, dark=0.0):
    key = (var % 4, dark)
    if key not in _sn_cache:
        _sn_cache[key] = _snare_raw(var % 4, dark)
    return _sn_cache[key] * np.float32(vel)


_HAT_F = np.array([205.3, 304.4, 369.6, 522.7, 540.0, 800.0]) * 1.7
_hat_cache = {}


def soft_hat(vel=1.0, var=0, open_=False):
    key = (var % 4, open_)
    if key not in _hat_cache:
        n = int((0.32 if open_ else 0.08) * SR)
        t = np.arange(n) / SR
        rg = np.random.default_rng(800 + var + (10 if open_ else 0))
        metal = np.zeros(n)
        for f in _HAT_F * (1 + 0.01 * (var - 1.5)):
            metal += np.sign(np.sin(2 * np.pi * f * t + rg.random() * 6.28))
        src = 0.45 * metal / len(_HAT_F) + 0.7 * rg.standard_normal(n)
        y = S.butter(src, [5800, 10500], 'band', 2) * np.exp(-t / (0.085 if open_ else 0.015))
        y *= np.minimum(t / 0.0008, 1)
        fe = int(0.01 * SR)
        y[-fe:] *= np.linspace(1, 0, fe)
        y /= np.abs(y).max() + 1e-9
        _hat_cache[key] = np.stack([y, np.roll(y, 9)]).astype(np.float32)
    return _hat_cache[key] * np.float32(vel)


_shk_cache = {}


def shaker(vel=1.0, var=0):
    if var % 4 not in _shk_cache:
        n = int(0.09 * SR)
        t = np.arange(n) / SR
        rg = np.random.default_rng(900 + var)
        y = S.butter(rg.standard_normal(n), [3500, 8500], 'band', 2)
        y *= np.minimum(t / 0.008, 1) ** 2 * np.exp(-np.maximum(t - 0.008, 0) / 0.03)
        y /= np.abs(y).max() + 1e-9
        _shk_cache[var % 4] = np.stack([y, np.roll(y, 21)]).astype(np.float32)
    return _shk_cache[var % 4] * np.float32(vel)


_kick = None


def deep_kick(vel=1.0):
    """Round 808/909-style kick, settling on G1 (49 Hz), soft click."""
    global _kick
    if _kick is None:
        n = int(0.42 * SR)
        t = np.arange(n) / SR
        f = 49.0 + 95 * np.exp(-t / 0.035) + 250 * np.exp(-t / 0.004)
        body = np.sin(2 * np.pi * np.cumsum(f) / SR)
        amp = np.exp(-t / 0.22) * np.minimum(t / 0.0008, 1)
        amp *= np.where(t > 0.36, np.clip((0.42 - t) / 0.06, 0, 1), 1)
        rg = np.random.default_rng(1001)
        click = S.butter(rg.standard_normal(n), [1500, 5000], 'band', 2) * np.exp(-t / 0.0025) * 0.22
        y = np.tanh(1.8 * body * amp) * 0.95 + click
        _kick = np.stack([y, y]).astype(np.float32)
    return _kick * np.float32(vel)


# ------------------------------------------------------------------ choir

_VOW = {
    'oh': [(430, 1.00, 70), (780, 0.55, 80), (2550, 0.10, 120), (3200, 0.05, 150)],
    'oo': [(330, 1.00, 60), (680, 0.40, 80), (2400, 0.06, 120), (3100, 0.03, 140)],
    'ah': [(720, 1.00, 80), (1100, 0.50, 90), (2500, 0.16, 120), (3300, 0.07, 150)],
}


def vox(midi, dur, vel=1.0, attack=0.9, rel=1.1, vowel='oh', voices=6, seed=0):
    """Choir-like pad note -> float32 (2, n).  The formants are fixed: the
    vowel never sweeps."""
    n = int((dur + 4 * rel) * SR)
    t = np.arange(n) / SR
    rg = np.random.default_rng(seed * 131 + int(midi))
    L = np.zeros(n)
    R = np.zeros(n)
    # slow drift: low-passed noise, a few cents
    for i in range(voices):
        p = -0.85 + 1.7 * i / max(voices - 1, 1)
        det = (i - (voices - 1) / 2) * 0.045
        dr = S.butter(rg.standard_normal(n // 480 + 2), 0.02, 'low', 1)
        dr = np.interp(np.arange(n) / 480.0, np.arange(len(dr)), dr)
        dr = dr / (np.abs(dr).max() + 1e-9) * 0.05
        vib = 0.09 * np.sin(2 * np.pi * (4.7 + 0.25 * i) * t + rg.random() * 6.28) * np.clip(t / 1.2, 0, 1)
        w = S.saw(hz(midi + det + dr + vib), n, rg.random())
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    src = np.stack([L, R]) / voices
    y = np.zeros_like(src)
    for fc, g, bw in _VOW[vowel]:
        y += S.svf(src, fc, fc / bw, 'bp') * g
    br = S.butter(rg.standard_normal((2, n)), [500, 3000], 'band', 2) * 0.006
    y += br
    y = S.butter(y, 5200, 'low', 2)
    a = np.clip(t / attack, 0, 1)
    env = np.sin(a * np.pi / 2) ** 2
    rl = t > dur
    env[rl] *= np.exp(-(t[rl] - dur) / rel)
    fe = int(0.02 * SR)
    env[-fe:] *= np.linspace(1, 0, fe)
    return (y * env * vel * 2.2).astype(np.float32)


# ------------------------------------------------------------------ space

def space_ir():
    """Long, dark hall: the "vast distance" reverb."""
    return O.hall_ir(length=6.0, rt_low=4.6, rt_mid=3.8, rt_high=1.6, predelay=0.04, seed=7)


def room_ir():
    """Short, soft room for the drums."""
    return O.hall_ir(length=1.0, rt_low=0.5, rt_mid=0.45, rt_high=0.25, predelay=0.008, seed=11)
