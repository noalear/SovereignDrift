# synth_new08.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for new08_detroit.py (Detroit / space techno): a TR-909 style
# kit (tuned kick, crisp hats, a soft round clap, ride, crash, toms), 808
# style congas, a deep filtered bass, a dub chord stab, a string machine
# ensemble, an FM electric piano and a soft pulse lead.  Everything is
# synthesized; no samples.  Builds on tools/music/edm/synth.py (unchanged).
import numpy as np
import scipy.signal as ss

import synth as S
from synth import SR, hz, saw, sine, svf, butter, env_adsr

_g = np.random.default_rng(808)


def _t(n):
    return np.arange(n) / SR


def square(freq, n, phase=0.0, pw=0.5):
    """Band-limited pulse from two polyBLEP saws."""
    a = saw(freq, n, phase)
    b = saw(freq, n, (phase + pw) % 1.0)
    return (a - b) * 0.5


# ----------------------------------------------------------------- drums

def kick909(vel=1.0, f_end=61.74, f_start=230.0, decay=0.30, length=0.5):
    """909-style kick: fast pitch sweep to a tuned body (B1 by default), a short
    attack click, gentle saturation."""
    n = int(length * SR)
    t = _t(n)
    f = f_end + (f_start - f_end) * np.exp(-t / 0.028) + 300 * np.exp(-t / 0.003)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / decay) * np.minimum(t / 0.0006, 1)
    amp *= np.where(t > length - 0.05, np.clip((length - t) / 0.05, 0, 1), 1)
    y = np.tanh(1.8 * body * amp) / np.tanh(1.8)
    click = butter(_g.standard_normal(n), [1500, 6000], 'band', 2) * np.exp(-t / 0.002) * 0.25
    y = y + click
    return np.stack([y, y]) * vel


_HAT = np.array([263.0, 400.0, 421.0, 474.0, 587.0, 845.0]) * 1.95


def _metal(n, scale=1.0, seed=0):
    g = np.random.default_rng(seed)
    t = _t(n)
    y = np.zeros(n)
    for f in _HAT * scale:
        y += np.sign(np.sin(2 * np.pi * f * t + g.random() * 6.28))
    return y / len(_HAT)


def hat909(vel=1.0, open_=False, var=0, pan=0.0):
    """Crisp 909-like hat; `var` is a round-robin variant (tone, decay, level)."""
    g = np.random.default_rng(900 + var + (50 if open_ else 0))
    length = 0.42 if open_ else 0.10
    n = int(length * SR)
    t = _t(n)
    tau = (0.15 if open_ else 0.026) * (1 + 0.12 * (g.random() - 0.5))
    src = 0.6 * _metal(n, 1 + 0.02 * (g.random() - 0.5), var) + 0.45 * g.standard_normal(n)
    y = butter(src, 7200 if not open_ else 6400, 'high', 4)
    y = butter(y, 14500, 'low', 2)
    e = np.exp(-t / tau) * np.minimum(t / 0.0004, 1)
    if open_:
        e *= np.where(t > length - 0.04, np.clip((length - t) / 0.04, 0, 1), 1)
    lvl = 1 + 0.12 * (g.random() - 0.5)
    return S.pan2(y * e * vel * lvl * 0.9, pan)


def clap909(vel=1.0, var=0):
    """Soft, round clap: three short bursts and a short dark tail, band-limited
    to ~0.8-4.5 kHz, a little tonal body and a short room. Variants differ in
    burst spacing, colour and level."""
    g = np.random.default_rng(300 + var)
    n = int(0.32 * SR)
    t = _t(n)
    noise = g.standard_normal(n)
    e = np.zeros(n)
    sp = 0.0085 * (1 + 0.15 * (g.random() - 0.5))
    for k in range(3):
        u = t - k * sp
        e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.004), 0) * (0.75 + 0.1 * k)
    u = t - 3 * sp
    e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.055), 0) * 0.65
    fc = 1250 * (1 + 0.12 * (g.random() - 0.5))
    y = butter(noise * e, [fc * 0.65, fc * 3.4], 'band', 2)
    y = butter(y, 4800, 'low', 2)
    body = np.sin(2 * np.pi * 330 * t * (1 + 0.15 * np.exp(-t / 0.01))) * np.exp(-t / 0.025) * 0.25
    y = y + body * np.minimum(t / 0.001, 1)
    # short dark room
    L = int(0.12 * SR)
    tt = _t(L)
    ir = g.standard_normal((2, L)) * np.exp(-tt / 0.045)
    ir = butter(butter(ir, 3500, 'low', 2), 400, 'high', 2)
    ir /= np.sqrt(np.sum(ir[0] ** 2))
    wet = np.stack([ss.fftconvolve(y, ir[c])[:n] for c in range(2)]) * 0.28
    out = np.stack([y, np.roll(y, 23)]) + wet
    out[:, -int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    return out * vel * (1 + 0.1 * (g.random() - 0.5)) * 1.6


def ride909(vel=1.0, var=0, pan=0.3):
    g = np.random.default_rng(700 + var)
    n = int(1.1 * SR)
    t = _t(n)
    src = _metal(n, 1.55, var) * 0.7 + 0.35 * g.standard_normal(n)
    y = butter(src, [3800, 11000], 'band', 2)
    y = y * (0.55 * np.exp(-t / 0.015) + 0.45 * np.exp(-t / 0.42))
    y *= np.where(t > 1.0, np.clip((1.1 - t) / 0.1, 0, 1), 1)
    return S.pan2(y * vel * 0.7, pan)


def crash909(vel=1.0, length=2.4, seed=0):
    g = np.random.default_rng(600 + seed)
    n = int(length * SR)
    t = _t(n)
    out = []
    for c in range(2):
        src = 0.5 * _metal(n, 1.3 + 0.05 * c, seed + c) + g.standard_normal(n)
        y = butter(src, 3500, 'high', 2)
        y = butter(y, 12000, 'low', 2)
        y = y * (0.4 * np.exp(-t / 0.03) + 0.6 * np.exp(-t / 0.8))
        y *= np.where(t > length - 0.3, np.clip((length - t) / 0.3, 0, 1), 1)
        out.append(y)
    return np.stack(out) * vel * 0.6


def conga(midi, vel=1.0, pan=0.0):
    """808-style conga: pitched sine with a quick downward blip, short decay."""
    n = int(0.3 * SR)
    t = _t(n)
    f = hz(midi) * (1 + 0.25 * np.exp(-t / 0.006))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.085) * np.minimum(t / 0.0008, 1)
    y += butter(_g.standard_normal(n), 2500, 'low', 2) * np.exp(-t / 0.003) * 0.15
    return S.pan2(y * vel * 0.8, pan)


def tom909(midi, vel=1.0, pan=0.0):
    n = int(0.5 * SR)
    t = _t(n)
    f = hz(midi) * (1 + 0.45 * np.exp(-t / 0.035))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.16)
    y += butter(_g.standard_normal(n), 1800, 'low', 2) * np.exp(-t / 0.012) * 0.25
    y = np.tanh(1.3 * y)
    return S.pan2(y * vel, pan)


# ------------------------------------------------------------ bass / keys

def deep_bass(midi, dur, vel=1.0, cut=260.0, env=650.0, decay=0.09, q=1.1, sub=0.55, r=0.035):
    """Deep mono bass: saw + square through a low-pass with a short envelope,
    a sine sub at the fundamental, gentle drive."""
    n = int((dur + 6 * r) * SR)
    f = hz(midi)
    w = 0.55 * saw(f, n, 0.1) + 0.45 * square(f * 1.002, n, 0.35, 0.42)
    t = _t(n)
    fc = cut + env * np.exp(-t / decay)
    y = svf(w, fc, q)
    y = svf(y, fc * 1.3, 0.7)
    y = y + sub * sine(f, n)
    y = butter(y, 30, 'high', 2)
    e = env_adsr(n, dur, 0.003, 0.25, 0.8, r)
    y = np.tanh(1.5 * y * e) / np.tanh(1.5) * vel
    return np.stack([y, y])


def stab(midis, dur, vel=1.0, cut=1500.0, env=2.2, decay=0.06, q=1.6, amp_tau=0.16):
    """Dub chord stab: two detuned saws and a pulse per note, a resonant
    low-pass with a fast envelope (cut * (1 + env) -> cut), a short decay."""
    n = int((dur + 0.5) * SR)
    t = _t(n)
    L = np.zeros(n)
    R = np.zeros(n)
    k = len(midis)
    for i, md in enumerate(midis):
        f = float(hz(md))
        p = (i / max(k - 1, 1) - 0.5) * 0.6
        a = saw(f * 2 ** (-0.07 / 12), n, _g.random())
        b = saw(f * 2 ** (0.07 / 12), n, _g.random())
        c = square(f, n, _g.random(), 0.35) * 0.5
        L += (a * 0.8 + b * 0.4 + c) * np.sqrt((1 - p) / 2)
        R += (a * 0.4 + b * 0.8 + c) * np.sqrt((1 + p) / 2)
    fc = cut * (1 + env * np.exp(-t / decay))
    x = svf(np.stack([L, R]), fc, q)
    x = butter(x, 140, 'high', 2)
    e = np.exp(-t / amp_tau) * np.minimum(t / 0.0015, 1)
    e *= np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.04))
    return x * e * vel / np.sqrt(k) * 0.6


def strings(midi, dur, vel=1.0, a=0.5, r=1.2, cut=3600.0, octave=0.3):
    """String machine voice (Solina-ish): three detuned saws with slow
    independent vibrato plus a quiet octave, low-passed; ensemble chorus is
    applied on the bus."""
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    L = np.zeros(n)
    R = np.zeros(n)
    for o, p, gg in ((-0.11, -0.7, 1.0), (0.0, 0.0, 0.9), (0.10, 0.7, 1.0), (12.03, 0.25, octave)):
        vib = 0.06 * np.sin(2 * np.pi * (5.2 + _g.random() * 0.8) * t + _g.random() * 6)
        w = saw(hz(midi + o + vib), n, _g.random()) * gg
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = svf(np.stack([L, R]), cut, 0.6)
    x = butter(x, 200, 'high', 2)
    return x * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 0.3


def rhodes(midi, dur, vel=0.7, r=0.35):
    """FM electric piano: 1:1 body with a decaying index plus a 14:1 tine."""
    f = float(hz(midi))
    n = int((dur + 5 * r + 0.05) * SR)
    t = _t(n)
    i1 = (0.35 + 1.2 * vel) * np.exp(-t / 0.45) + 0.2 * vel
    body = np.sin(2 * np.pi * f * t + i1 * np.sin(2 * np.pi * f * t))
    i2 = (0.7 + 1.3 * vel) * np.exp(-t / 0.02)
    tine = np.sin(2 * np.pi * f * t + i2 * np.sin(2 * np.pi * 14.0 * f * t)) * np.exp(-t / 0.45)
    tau = 1.8 * (261.6 / f) ** 0.45
    amp = np.exp(-t / tau) * 0.8 + 0.2 * np.exp(-t / (tau * 4))
    y = (body + 0.3 * tine) * amp
    y = np.tanh((1.0 + vel) * y) / np.tanh(1.0 + vel)
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / r))
    y = y * gate * np.minimum(t / 0.001, 1) * vel * 0.4
    return np.stack([y, y])


def pulse_lead(midi, dur, vel=1.0, cut=2200.0, a=0.03, r=0.25, vib=0.12):
    """Soft pulse lead with delayed vibrato (doubles the string theme)."""
    n = int((dur + 5 * r) * SR)
    t = _t(n)
    v = S.vibrato(n, onset=0.25, rate=5.0, depth=vib)
    f = hz(midi + v)
    w = square(f, n, 0.0, 0.3) * 0.7 + saw(f * 1.003, n, 0.5) * 0.3
    x = svf(w, cut * (1 + 0.6 * np.exp(-t / 0.15)), 0.8)
    x = butter(x, 250, 'high', 2)
    y = x * env_adsr(n, dur, a, 0.4, 0.85, r) * vel * 0.5
    return S.pan2(y, 0.0)


# ------------------------------------------------------------------ fx

def chorus(x, rate=0.5, depth_ms=2.0, center_ms=8.0, mix=0.5, spread=np.pi / 2):
    """Ensemble chorus: one modulated delay per channel, LFOs offset."""
    n = x.shape[1]
    base = np.arange(n, dtype=np.float64)
    out = np.empty_like(x)
    for c in range(2):
        lfo = np.sin(2 * np.pi * rate * base / SR + c * spread)
        lfo2 = 0.4 * np.sin(2 * np.pi * rate * 2.7 * base / SR + c * 1.3)
        idx = base - (center_ms + depth_ms * (lfo + lfo2)) * 1e-3 * SR
        del lfo, lfo2
        wet = np.interp(idx, base, x[c].astype(np.float64))
        del idx
        out[c] = ((1 - mix) * x[c] + mix * wet).astype(np.float32)
        del wet
    return out


def dub_delay(x, delay_sec, feedback=0.55, repeats=10, lp=2800.0, hp=450.0, spread=0.8):
    """Dub echo: each repeat darker than the last, alternating sides."""
    mono = (x[0] + x[1]) * 0.5
    sig = butter(butter(mono, hp, 'high', 2), lp, 'low', 2)
    n = x.shape[1]
    out = np.zeros((2, n), np.float32)
    d = int(delay_sec * SR)
    for i in range(1, repeats + 1):
        off = d * i
        if off >= n:
            break
        g = feedback ** (i - 1)
        side = 1 if i % 2 else -1
        gl = np.sqrt((1 - side * spread) / 2)
        gr = np.sqrt((1 + side * spread) / 2)
        seg = sig[: n - off] * g
        out[0, off:] += (seg * gl).astype(np.float32)
        out[1, off:] += (seg * gr).astype(np.float32)
        sig = np.tanh(butter(sig, lp * 0.8 ** i, 'low', 1) * 1.05)
    return out


def noise_swell(length, f0=300.0, f1=5000.0):
    """Soft filtered-noise swell for builds (low, dark, never in front)."""
    n = int(length * SR)
    x = _g.standard_normal((2, n))
    u = np.linspace(0, 1, n)
    fc = f0 * (f1 / f0) ** (u ** 1.5)
    y = svf(x, fc, 0.9, 'bp')
    y = butter(y, 9000, 'low', 2)
    return y * (u ** 2) * 0.4
