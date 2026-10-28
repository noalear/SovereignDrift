# synth_new18.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments and effects for new track 18, "Quiet Drive"
# (tools/music/edm/new18_ambient_breaks.py), a late-90s downtempo / ambient
# breakbeat piece.  Builds on synth.py, synth_new10.py and synth_triphop.py,
# which are not modified here.  Everything is synthesized: no samples.
#
#   drums     dusty round kick tuned to A1 (the fifth of D minor), the round
#             tuned "pdlap" snare (A3 body, 1.6x second mode, 1-4 kHz snap,
#             tiny clap layer, short dark room, 4 round-robins, ghost version)
#   effects   sampler "dust" (12-bit grit + 24 kHz sample-and-hold, low-passed:
#             an SP-1200 / S950 flavour on the break bus), tape-style
#             pitch drift for the "seasick" detuned pads (a slowly modulated
#             delay, Boards-of-Canada style)
#   tonal     boc_pad: two detuned analog-pad layers a few cents apart with a
#             slow vibrato of their own; soft sine "flute" lead for the
#             breakdown (sine + a little 2nd/3rd harmonic, breathy attack)
import numpy as np
import scipy.signal as ss

import synth as S
import synth_new10 as X
from synth import SR

_r = np.random.default_rng(1818)


def _t(n):
    return np.arange(n) / SR


# ------------------------------------------------------------------ drums

def kick(vel=1.0, f_end=55.0, f_start=120.0, length=0.5, var=0):
    """Round, soft breakbeat kick settling on A1 (55 Hz); a little knock, no click."""
    n = int(length * SR)
    t = _t(n)
    f = f_end + (f_start - f_end) * np.exp(-t / 0.034)
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * np.exp(-t / (0.22 + 0.015 * var))
    knock = np.sin(ph * 2.0) * np.exp(-t / 0.03) * 0.25
    r = np.random.default_rng(40 + var)
    beat = S.butter(r.standard_normal(n) * np.exp(-t / 0.003), [500, 2500], 'band', 1) * 0.18
    x = body + knock + beat
    x = np.tanh(1.5 * x) / np.tanh(1.5)
    x *= np.minimum(t / 0.0015, 1.0)
    x[-int(0.02 * SR):] *= np.linspace(1, 0, int(0.02 * SR))
    x = S.butter(x, 4500, 'low', 2)
    return S.pan2(x * vel * 0.8, 0.0).astype(np.float32)


# (pitch factor, snap centre Hz, level, room)
SNARE_RR = [(1.000, 2000.0, 1.00, 0.30),
            (1.014, 2250.0, 0.92, 0.27),
            (0.987, 1850.0, 0.97, 0.32),
            (1.006, 2400.0, 0.90, 0.28)]


def snare(vel=1.0, var=0, ghost=False, f0=220.0):
    """Round tuned "pdlap": A3 body with a fast pitch drop and a 1.6x mode, a
    band-limited snap at 1-4 kHz, a tiny clap layer, a short dark room; ~120 ms
    to -20 dB and almost nothing above 7 kHz.  Softer (ghost) hits: less snap."""
    pf, snap_c, lev, room = SNARE_RR[var % 4]
    n = int(0.4 * SR)
    t = _t(n)
    r = np.random.default_rng(700 + var + (50 if ghost else 0))
    f = f0 * pf * (1.0 + 0.5 * np.exp(-t / 0.010))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.065) + 0.42 * np.sin(ph * 1.6) * np.exp(-t / 0.04)
            + 0.15 * np.sin(ph * 2.33) * np.exp(-t / 0.025))
    snp = S.butter(r.standard_normal(n), [snap_c * 0.55, snap_c * 1.8], 'band', 2) * np.exp(-t / 0.025) * 2.2
    clap = np.zeros(n)
    for k, d in enumerate((0.0, 0.007, 0.015)):
        i = int(d * SR)
        m_ = min(n - i, int(0.012 * SR))
        clap[i:i + m_] += np.exp(-np.arange(m_) / SR / 0.003) * (0.7 ** k)
    clap = S.butter(clap * r.standard_normal(n), [900, 2800], 'band', 2) * 0.8
    x = body * 0.95 + snp * (0.5 if not ghost else 0.18) + clap * (0.45 if not ghost else 0.1)
    x *= np.minimum(t / 0.0012, 1.0)
    irn = int(0.18 * SR)
    ir = S.butter(r.standard_normal(irn), [300, 3000], 'band', 1) * np.exp(-_t(irn) / 0.04)
    ir /= np.sqrt(np.sum(ir ** 2)) + 1e-9
    x = x + room * ss.oaconvolve(x, ir)[:n] * 0.5
    x = S.butter(x, 6800, 'low', 2)
    x = S.butter(x, 120, 'high', 2)
    x = np.tanh(1.25 * x) / np.tanh(1.25)
    x *= np.exp(-np.maximum(t - 0.12, 0) / 0.045)
    if ghost:
        x = S.butter(x, 3500, 'low', 1)
    st = S.pan2(x * vel * lev * 0.55, 0.05)
    st[:, -int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    return st.astype(np.float32)


def dust(x, bits=12, hold=2, lp=7800.0, drive=1.4):
    """Sampler grit for the break bus: quantize to `bits` (relative to the bus
    peak), sample-and-hold by `hold` samples, low-pass and soft-saturate."""
    pk = float(np.max(np.abs(x))) + 1e-9
    q = 2 ** (bits - 1)
    y = np.round(x / pk * q) / q * pk
    if hold > 1:
        n = y.shape[1]
        idx = (np.arange(n) // hold) * hold
        y = y[:, idx]
    y = S.butter(y, lp, 'low', 4)
    y = np.tanh(drive * y / pk) / np.tanh(drive) * pk
    return y.astype(np.float32)


def seasick(x, depth_ms=3.0, rate=0.23, rate2=0.071, seed=5):
    """Slow tape pitch drift: a modulated delay; both channels move together
    (so it detunes against the dry Rhodes and bass, not left vs right)."""
    n = x.shape[1]
    t = _t(n)
    d = (depth_ms * (1 + 0.7 * np.sin(2 * np.pi * rate * t + seed) + 0.3 * np.sin(2 * np.pi * rate2 * t)) / 2
         ) / 1000 * SR + 2
    pos = np.arange(n) - d
    out = np.empty_like(x)
    for c in range(x.shape[0]):
        out[c] = np.interp(pos, np.arange(n), x[c], left=0.0).astype(np.float32)
    return out


# ------------------------------------------------------------------ tonal

def boc_pad(midi, dur, vel=1.0, t0=0.0, attack=1.0, release=1.6, cut=1300.0):
    """Warm detuned pad: two analog-pad layers 9 cents apart (one with an octave-down saw)."""
    a = X.analog_pad(midi + 0.045, dur, vel, t0, attack=attack, release=release, cut=cut, voices=3,
                     detune=0.10, q=0.7, octave=0.0, width=0.9)
    b = X.analog_pad(midi - 0.045, dur, vel * 0.8, t0 + 0.37, attack=attack * 1.2, release=release,
                     cut=cut * 0.8, voices=2, detune=0.07, q=0.7, octave=0.35, width=0.6)
    k = min(a.shape[1], b.shape[1])
    return a[:, :k] + b[:, :k]


def sine_flute(midi, dur, vel=1.0, attack=0.09, release=0.35, pan=0.0):
    """Soft sine lead with a breathy onset (for the breakdown)."""
    f = float(S.hz(midi))
    n = int((dur + 4 * release) * SR)
    t = _t(n)
    vib = 0.10 * np.clip((t - 0.35) / 0.5, 0, 1) * np.sin(2 * np.pi * 4.6 * t)
    ff = f * 2 ** (vib / 12)
    ph = 2 * np.pi * np.cumsum(ff) / SR
    y = np.sin(ph) + 0.12 * np.sin(2 * ph) + 0.05 * np.sin(3 * ph)
    br = S.butter(_r.standard_normal(n), [f * 1.5, f * 5], 'band', 1) * np.exp(-t / 0.08) * 0.15
    e = S.env_adsr(n, dur, attack, 0.6, 0.85, release / 4)
    y = (y + br) * e
    k = int(0.01 * SR)
    y[-k:] *= np.linspace(1, 0, k)
    return S.pan2(y * vel * 0.4, pan).astype(np.float32)
