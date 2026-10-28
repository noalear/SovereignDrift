# synth_new03.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for new track 03, an atmospheric jungle / intelligent drum & bass
# piece (new03_jungle.py).  Builds on synth.py and synth_liquid.py (imported,
# not modified) and adds:
#
#   vibes        additive vibraphone: bar partials 1 : 3.98 : 9.9, soft mallet,
#                motor tremolo, damper release
#   jpad         warm Juno-style string pad (detuned saws, gentle LP, slow attack)
#   airpad       breathy "air" layer: noise band-passed at each chord tone
#   jkick        soft, round break kick tuned to A1 (the fifth of D minor)
#   jsnare       short, rounded, woody "pdlap" snare tuned to A3 (4 round robins)
#   jghost       dark ghost-note version of jsnare
#   jhat         crisp-but-soft closed hat (band-limited, nothing harsh above 11 kHz)
#   jride        soft jazz ride (band-limited, short wash)
#   rimstick     woody side-stick (the original song's side-stick part)
#   wind         slow, quiet space-wind bed (background only)
#   warm_bass    warm mid layer for the sub (rounded triangle, LP'd) so the bass
#                line is audible on small speakers
import numpy as np

import synth as S
from synth import SR, hz


# ------------------------------------------------------------- keys

def vibes(midi, dur, vel=0.7, r=0.35, trem_hz=4.8, trem=0.22, phase=0.0):
    """Vibraphone: (2, n).  dur = how long the damper is up."""
    f = float(hz(midi))
    ring = 2.6 * (523.0 / f) ** 0.35          # natural decay (s), longer for low bars
    n = int((min(dur, ring * 1.5) + 5 * r + 0.05) * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    y = np.sin(2 * np.pi * f * t + rg.random()) * np.exp(-t / ring)
    y += 0.30 * vel * np.sin(2 * np.pi * 3.98 * f * t) * np.exp(-t / (ring * 0.25))
    if 9.9 * f < 16000:
        y += 0.06 * vel * np.sin(2 * np.pi * 9.9 * f * t) * np.exp(-t / 0.05)
    mallet = S.butter(rg.standard_normal(n), [700, 3500], 'band', 2) * np.exp(-t / 0.0025) * 0.10 * vel
    y = y + mallet
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / r))
    y *= gate * np.minimum(t / 0.0012, 1) * vel * 0.5
    ph = 2 * np.pi * trem_hz * t + phase
    L = y * (1 - trem * (0.5 + 0.5 * np.sin(ph)))
    R = y * (1 - trem * (0.5 + 0.5 * np.sin(ph + 1.3)))
    p = float(np.clip((midi - 72) / 20, -0.5, 0.5))
    return np.stack([L * np.sqrt(1 - p), R * np.sqrt(1 + p)]).astype(np.float32)


def jpad(midi, dur, vel=1.0, a=1.2, r=1.2, cut=1300.0, voices=4, detune=0.12):
    """Warm string-machine pad voice: (2, n)."""
    n = int((dur + 4 * r) * SR)
    rg = S.rng()
    L = np.zeros(n)
    R = np.zeros(n)
    t = np.arange(n) / SR
    for i in range(voices):
        det = (i - (voices - 1) / 2) * detune / max(1, voices - 1) * 2
        drift = 0.03 * np.sin(2 * np.pi * (0.15 + 0.1 * rg.random()) * t + rg.random() * 6.3)
        w = S.saw(hz(midi + det + drift), n, rg.random())
        p = -0.7 + 1.4 * i / max(1, voices - 1)
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = np.stack([L, R]) / np.sqrt(voices)
    x = S.svf(x, cut, 0.6)
    x = S.butter(x, 120, 'high', 1)
    e = S.env_adsr(n, dur, a, 1.0, 0.95, r)
    return (x * e * vel).astype(np.float32)


def airpad(midi, dur, vel=1.0, a=1.5, r=1.5, q=40.0):
    """Breathy noise tuned to a note: (2, n)."""
    n = int((dur + 4 * r) * SR)
    rg = S.rng()
    f = float(hz(midi))
    x = rg.standard_normal((2, n))
    # the SVF band-pass output peaks at gain q: normalise it
    y = (S.svf(x, f, q, 'bp') + 0.5 * S.svf(x, 2 * f, q, 'bp')) / q
    e = S.env_adsr(n, dur, a, 1.0, 1.0, r)
    return (y * e * vel * 3.0).astype(np.float32)


# ------------------------------------------------------------- bass

def warm_bass(pitch, env, cut=320.0):
    """Rounded-triangle mid layer an octave above the sub line, low-passed (mono)."""
    n = len(pitch)
    f = hz(pitch + 12)
    ph = (np.cumsum(f) / SR) % 1.0
    tri = 4 * np.abs(ph - 0.5) - 1
    y = S.svf(tri, cut, 0.7)
    y = S.butter(y, 90, 'high', 2)
    return (np.tanh(1.3 * y) * env).astype(np.float32)


# ------------------------------------------------------------- drums

def jkick(vel=1.0, f_end=55.0, f_start=150.0, length=0.32):
    """Soft, round break kick (body ends on A1)."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_start - f_end) * np.exp(-t / 0.028) + 300 * np.exp(-t / 0.002)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.13) * np.minimum(t / 0.0008, 1)
    amp *= np.where(t > length - 0.05, np.clip((length - t) / 0.05, 0, 1), 1)
    beater = S.butter(S.rng().standard_normal(n), [1200, 4500], 'band', 2) * np.exp(-t / 0.003) * 0.22
    y = np.tanh(2.0 * body * amp) * 0.9 + beater
    return np.stack([y, y]).astype(np.float32) * vel


# round-robin variants: (pitch mult, snap tone mult, level, body decay mult)
_SN_RR = [(1.000, 1.00, 1.00, 1.00), (0.988, 0.93, 0.95, 1.05), (1.012, 1.06, 0.98, 0.95),
          (0.996, 0.97, 0.92, 1.02)]


def jsnare(vel=1.0, var=0, dark=0.0, f0=220.0):
    """Short, rounded, woody snare ("pdlap"): body at A3 with a fast pitch drop and
    membrane modes, a short 1-3.5 kHz snap, a small rim click and clap flam,
    ~90 ms decay (-30 dB at ~150 ms), low-passed at 7 kHz.  dark 0..1 for ghost notes."""
    pm, tone, lvl, dm = _SN_RR[var % 4]
    n = int(0.20 * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    f = f0 * pm * (1 + 0.5 * np.exp(-t / 0.009))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / (0.045 * dm)) + 0.32 * np.sin(1.59 * ph) * np.exp(-t / 0.035) +
            0.14 * np.sin(2.14 * ph) * np.exp(-t / 0.020))
    snap = S.butter(rg.standard_normal(n), [1100 * tone, 3500 * tone], 'band', 2)
    snap *= (np.exp(-t / 0.013) + 0.2 * np.exp(-t / 0.045)) * (1 - 0.85 * dark)
    rim = np.sin(2 * np.pi * 1500 * tone * t) * np.exp(-t / 0.0035) * 0.18 * (1 - 0.7 * dark)
    clap = np.zeros(n)
    for off in (0.0, 0.006, 0.013):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0028), 0)
    clap = S.butter(rg.standard_normal(n) * clap, [1100, 3000], 'band', 2) * 0.28 * (1 - 0.9 * dark)
    y = 0.95 * body + 1.0 * snap + rim + 1.4 * clap
    y = np.tanh(1.3 * y) / 1.05 * np.minimum(t / 0.0004, 1)
    y = S.butter(y, 7000 - 4000 * dark, 'low', 2)
    fe = int(0.025 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 11)]).astype(np.float32) * vel * lvl


def jghost(vel=0.15, var=0):
    return jsnare(vel, var, dark=1.0)


def jhat(vel=1.0, pan=0.0, length=0.07, tau=0.016, open_=False):
    if open_:
        length, tau = 0.28, 0.07
    n = int(length * SR)
    t = np.arange(n) / SR
    src = 0.5 * S._metal(n) + 0.6 * S.rng().standard_normal(n)
    y = S.butter(src, [6500, 11000], 'band', 2) * np.exp(-t / tau)
    y *= np.minimum(t / 0.0006, 1)
    fe = min(int(0.01 * SR), n // 3)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.9, pan).astype(np.float32)


def jride(vel=1.0, pan=0.3, length=0.9):
    n = int(length * SR)
    t = np.arange(n) / SR
    src = S._metal(n) * 0.7 + 0.35 * S.rng().standard_normal(n)
    y = S.butter(src, [3000, 9000], 'band', 2) * (0.5 * np.exp(-t / 0.012) + 0.5 * np.exp(-t / 0.30))
    ping = np.sin(2 * np.pi * 2637.0 * t) * np.exp(-t / 0.25) * 0.08     # E7 bell, quiet
    y = y + ping
    fe = int(0.05 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.7, pan).astype(np.float32)


def rimstick(vel=1.0, pan=-0.15):
    n = int(0.07 * SR)
    t = np.arange(n) / SR
    y = (np.sin(2 * np.pi * 1180 * t) * np.exp(-t / 0.007) + 0.5 * np.sin(2 * np.pi * 520 * t) * np.exp(-t / 0.012) +
         S.butter(S.rng().standard_normal(n), [1500, 5000], 'band', 2) * np.exp(-t / 0.003) * 0.5)
    fe = int(0.01 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.6, pan).astype(np.float32)


# ------------------------------------------------------------- atmos

def wind(n, t0=0.0, seed_rate=0.07):
    """Quiet space wind: noise through a slowly wandering low band (no fast sweeps)."""
    rg = S.rng()
    x = rg.standard_normal((2, n))
    t = t0 + np.arange(n) / SR
    fc = 650 * 2 ** (0.9 * np.sin(2 * np.pi * seed_rate * t) + 0.4 * np.sin(2 * np.pi * 0.031 * t + 1.0))
    y = S.svf(x, fc, 1.2, 'bp')
    amp = 0.6 + 0.4 * np.sin(2 * np.pi * 0.045 * t + 2.0)
    return (y * amp * 0.5).astype(np.float32)
