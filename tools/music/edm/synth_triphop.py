# synth_triphop.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments and effects for new01_triphop.py ("Graveyard Orbit"), a late-90s
# Bristol-style trip-hop track.  Everything here is synthesized; the sampled
# instruments (VSCO-2 CE, CC0) are played with synth_ambient.Sampler.
#
#   drums     round dusty kick (tuned), dark "pdlap" snare with round-robin
#             variants, ghost snares, dusty hats, shaker, wood rim
#   tonal     FM Rhodes/Wurli, theremin-like sine lead (legato, glides,
#             delayed vibrato), dub sub bass
#   texture   vinyl crackle + hiss (subtle), tape wow/flutter
#   effects   dub delay (filtered, saturated feedback), small plate IR
import numpy as np
import scipy.signal as ss

import synth as S
from synth import SR


def _t(n):
    return np.arange(n) / SR


def _r():
    return S.rng()


# ------------------------------------------------------------------ drums

def kick(vel=1.0, f_end=49.0, f_start=118.0, length=0.55, var=0):
    """Round, boomy breakbeat kick; the body settles on f_end (tuned to the key)."""
    n = int(length * SR)
    t = _t(n)
    f = f_end + (f_start - f_end) * np.exp(-t / 0.032)
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * np.exp(-t / (0.24 + 0.02 * var))
    knock = np.sin(ph * 2.0) * np.exp(-t / 0.035) * 0.3
    click = S.butter(_r().standard_normal(n) * np.exp(-t / 0.0025), [700, 3500], 'band', 1) * 0.25
    x = body + knock + click
    x = np.tanh(1.6 * x) / np.tanh(1.6)
    x *= np.minimum(t / 0.0015, 1.0)
    x[-int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    x = S.butter(x, 6000, 'low', 2)
    return S.pan2(x * vel * 0.8, 0.0).astype(np.float32)


SNARE_VARS = [  # (tone Hz, snap centre Hz, level, room)
    (198.0, 2100.0, 1.00, 0.28),
    (205.0, 2350.0, 0.93, 0.25),
    (192.0, 1900.0, 0.97, 0.30),
    (210.0, 2250.0, 0.90, 0.27),
]


def snare(vel=1.0, var=0, ghost=False):
    """Short rounded "pdlap": tuned body with a fast pitch drop, band-limited
    snap at 1-4 kHz, a tiny clap/rim layer and a short room; ~110 ms decay,
    very little energy above 8 kHz."""
    tone, snap_c, lev, room = SNARE_VARS[var % len(SNARE_VARS)]
    n = int(0.42 * SR)
    t = _t(n)
    r = _r()
    f = tone * (1.0 + 0.55 * np.exp(-t / 0.010))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.062) + 0.45 * np.sin(ph * 1.62) * np.exp(-t / 0.038)
            + 0.18 * np.sin(ph * 2.31) * np.exp(-t / 0.03))
    noise = r.standard_normal(n)
    snap = S.butter(noise, [snap_c * 0.55, snap_c * 1.8], 'band', 2) * np.exp(-t / 0.028) * 2.4
    # tiny clap/rim layer: three micro-bursts
    clap = np.zeros(n)
    for k, d in enumerate((0.0, 0.006, 0.013)):
        i = int(d * SR)
        m_ = min(n - i, int(0.012 * SR))
        clap[i:i + m_] += np.exp(-np.arange(m_) / SR / 0.003) * (0.7 ** k)
    clap = S.butter(clap * r.standard_normal(n), [900, 2600], 'band', 2) * 0.9
    x = body * 0.9 + snap * (0.55 if not ghost else 0.35) + clap * (0.5 if not ghost else 0.2)
    x *= np.minimum(t / 0.0012, 1.0)
    # short room
    irn = int(0.16 * SR)
    ir = S.butter(r.standard_normal(irn), [300, 3500], 'band', 1) * np.exp(-_t(irn) / 0.035)
    ir /= np.sqrt(np.sum(ir ** 2)) + 1e-9
    wet = ss.oaconvolve(x, ir)[:n]
    x = x + room * wet * 0.5
    x = S.butter(x, 7000, 'low', 2)
    x = S.butter(x, 120, 'high', 2)
    x = np.tanh(1.3 * x) / np.tanh(1.3)
    x *= np.exp(-np.maximum(t - 0.12, 0) / 0.04)
    if ghost:
        x = S.butter(x, 4000, 'low', 1)
    st = S.pan2(x * vel * lev * 0.55, 0.04)
    st[:, -int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    return st.astype(np.float32)


def hat(vel=1.0, open_=False, var=0, pan=0.18):
    """Dusty hat: metallic noise, band-limited (no fizz above ~11 kHz)."""
    n = int((0.35 if open_ else 0.09) * SR)
    t = _t(n)
    r = np.random.default_rng(300 + var * 7 + (100 if open_ else 0))
    x = r.standard_normal(n)
    fr = np.array([317, 461, 599, 811, 1043, 1327]) * (1.0 + 0.012 * var)
    met = sum(np.sign(np.sin(2 * np.pi * f * t)) for f in fr) / 6
    x = 0.6 * x + 0.4 * met
    x = S.butter(x, [6000 + 300 * var, 11000], 'band', 2)
    dec = 0.11 if open_ else (0.022 + 0.004 * var)
    x *= np.exp(-t / dec) * np.minimum(t / 0.0007, 1)
    x[-int(0.006 * SR):] *= np.linspace(1, 0, int(0.006 * SR))
    return S.pan2(x * vel * 0.35, pan).astype(np.float32)


def shaker(vel=1.0, var=0, pan=-0.3):
    n = int(0.12 * SR)
    t = _t(n)
    r = np.random.default_rng(500 + var)
    x = S.butter(r.standard_normal(n), [3500, 9000], 'band', 2)
    env = np.minimum(t / 0.012, 1) * np.exp(-np.maximum(t - 0.012, 0) / 0.035)
    return S.pan2(x * env * vel * 0.25, pan).astype(np.float32)


def rim(vel=1.0, pan=0.25):
    """Soft woody rim/wood-block click."""
    n = int(0.12 * SR)
    t = _t(n)
    x = (np.sin(2 * np.pi * 1520 * t) * np.exp(-t / 0.012) + 0.6 * np.sin(2 * np.pi * 820 * t) * np.exp(-t / 0.02))
    x += S.butter(_r().standard_normal(n), [1500, 4000], 'band', 1) * np.exp(-t / 0.003) * 0.5
    x *= np.minimum(t / 0.0008, 1)
    return S.pan2(x * vel * 0.35, pan).astype(np.float32)


# ------------------------------------------------------------------ tonal

def rhodes(midi, dur, vel=0.7, release=0.25, pan=0.0):
    """FM electric piano: 1:1 FM 'bark' rising with velocity, a tine ping, slow decay."""
    f = S.hz(midi)
    n = int((dur + 5 * release) * SR)
    t = _t(n)
    idx = (0.8 + 2.6 * vel ** 1.5) * np.exp(-t / 0.35) + 0.6
    mod = np.sin(2 * np.pi * f * t)
    car = np.sin(2 * np.pi * f * t + idx * mod)
    tine = np.sin(2 * np.pi * f * 7.02 * t) * np.exp(-t / 0.035) * 0.10 * vel
    sub = np.sin(2 * np.pi * f * t) * 0.35
    dec = 2.2 if midi < 64 else 1.5
    amp = np.exp(-t / dec) * np.minimum(t / 0.002, 1)
    amp = np.where(t < dur, amp, amp * np.exp(-(t - dur) / release))
    x = (car * 0.6 + sub + tine) * amp
    x[-int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    return S.pan2(x * vel * 0.45, pan).astype(np.float32)


def theremin(notes, total, vel=1.0, glide=0.075, vib_rate=5.6, vib_depth=0.22, attack=0.07,
             release=0.35):
    """Legato theremin-like line. notes: [(t_sec, midi, dur_sec, vel)], times relative to
    the returned buffer start (buffer is `total` seconds).  Continuous phase, exponential
    portamento between connected notes, delayed vibrato per note, soft swells."""
    n = int(total * SR)
    pitch = np.full(n, np.nan)
    amp = np.zeros(n)
    vibamt = np.zeros(n)
    notes = sorted(notes)
    for k, (ts, md, d, v) in enumerate(notes):
        i0 = int(ts * SR)
        i1 = min(n, int((ts + d) * SR))
        pitch[i0:i1] = md
        connected = k > 0 and ts - (notes[k - 1][0] + notes[k - 1][2]) < 0.06
        L = i1 - i0
        tt = np.arange(L) / SR
        a = np.minimum(tt / (0.03 if connected else attack), 1.0)
        a = (0.85 + 0.15 * a) if connected else a
        swell = 1.0 + 0.08 * np.minimum(tt / 0.6, 1.0)
        amp[i0:i1] = np.maximum(amp[i0:i1], a * swell * v)
        vibamt[i0:i1] = np.clip((tt - 0.22) / 0.5, 0, 1)
        nxt = notes[k + 1][0] if k + 1 < len(notes) else None
        if nxt is None or nxt - (ts + d) > 0.06:
            R = int(release * 6 * SR)
            j1 = min(n, i1 + R)
            rel = np.exp(-np.arange(j1 - i1) / SR / release)
            last = amp[i1 - 1] if i1 > i0 else 0
            amp[i1:j1] = np.maximum(amp[i1:j1], last * rel)
            pitch[i1:j1] = np.where(np.isnan(pitch[i1:j1]), md, pitch[i1:j1])
            vibamt[i1:j1] = vibamt[i1 - 1] if i1 > i0 else 0
    # fill gaps in pitch with the previous value
    idx = np.where(~np.isnan(pitch), np.arange(n), 0)
    np.maximum.accumulate(idx, out=idx)
    pitch = pitch[idx]
    pitch = np.nan_to_num(pitch, nan=float(notes[0][1]))
    # portamento: one-pole smoothing of pitch
    a = np.exp(-1.0 / (glide * SR))
    pitch = ss.lfilter([1 - a], [1, -a], pitch, zi=[pitch[0] * a])[0]
    t = _t(n)
    vib = vib_depth * vibamt * np.sin(2 * np.pi * vib_rate * t + 0.3 * np.sin(2 * np.pi * 0.23 * t))
    f = S.hz(pitch + vib)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = np.sin(ph) + 0.10 * np.sin(2 * ph) + 0.06 * np.sin(3 * ph) + 0.02 * np.sin(5 * ph)
    # smooth amp (no zipper)
    b_ = np.exp(-1.0 / (0.008 * SR))
    amp = ss.lfilter([1 - b_], [1, -b_], amp)
    x = np.tanh(1.2 * x * amp) / np.tanh(1.2)
    x = S.butter(x, 4500, 'low', 2)
    return (x * vel * 0.5).astype(np.float32)


def dub_bass(midi, dur, vel=1.0, release=0.07):
    """Deep dub sub: sine + soft 2nd harmonic, a small pitch 'thump' at the onset."""
    f0 = S.hz(midi)
    n = int((dur + 6 * release) * SR)
    t = _t(n)
    f = f0 * 2 ** ((0.35 * np.exp(-t / 0.025)) / 12)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = np.sin(ph) + 0.22 * np.sin(2 * ph + 0.4) + 0.06 * np.sin(3 * ph)
    amp = np.minimum(t / 0.006, 1) * (0.82 + 0.18 * np.exp(-t / 0.12))
    amp = np.where(t < dur, amp, amp * np.exp(-(t - dur) / release))
    x = np.tanh(1.4 * x * amp) / np.tanh(1.4)
    x = S.butter(x, 420, 'low', 2)
    x[-int(0.008 * SR):] *= np.linspace(1, 0, int(0.008 * SR))
    return S.pan2(x * vel * 0.7, 0.0).astype(np.float32)


# ---------------------------------------------------------------- texture

def vinyl(n, crackle=1.0, hiss=1.0, seed=78):
    """Subtle record surface: soft band-limited crackle and a little hiss."""
    r = np.random.default_rng(seed)
    out = np.zeros((2, n), np.float32)
    h = r.standard_normal((2, n)).astype(np.float32)
    h = S.butter(h, [900, 7000], 'band', 1).astype(np.float32) * 0.004 * hiss
    out += h
    dur = n / SR
    for rate, lvl in ((10.0, 0.010), (1.2, 0.030)):
        k = r.poisson(rate * dur)
        times = r.uniform(0, dur, k)
        for tc in times:
            i = int(tc * SR)
            L = int(r.uniform(0.0004, 0.0016) * SR)
            if i + L >= n:
                continue
            burst = r.standard_normal(L) * np.exp(-np.arange(L) / (L / 3.0)) * lvl * r.uniform(0.3, 1.0)
            ch = r.integers(0, 3)
            if ch == 2:
                out[:, i:i + L] += burst
            else:
                out[ch, i:i + L] += burst
    out = S.butter(out, 4500, 'low', 2).astype(np.float32)
    out = S.butter(out, 500, 'high', 1).astype(np.float32)
    return out * crackle


def wow(x, depth_ms=1.2, rate=0.55, flutter_ms=0.12, flutter_rate=6.3, seed=3):
    """Tape wow/flutter: slowly modulated fractional delay (per channel slightly offset)."""
    n = x.shape[1]
    t = _t(n)
    out = np.empty_like(x)
    for c in range(x.shape[0]):
        ph = 0.7 * c + seed
        d = (depth_ms * (1 + np.sin(2 * np.pi * rate * t + ph)) / 2
             + flutter_ms * (1 + np.sin(2 * np.pi * flutter_rate * t + 2 * ph)) / 2) / 1000 * SR + 2
        pos = np.arange(n) - d
        out[c] = np.interp(pos, np.arange(n), x[c], left=0.0).astype(np.float32)
    return out


# ---------------------------------------------------------------- effects

def dub_delay(x, delay_sec, feedback=0.5, repeats=9, hp=350.0, lp=2800.0, drive=1.3, spread=0.35):
    """Dub-style echo (wet only): every repeat is band-narrowed and saturated, and the
    echoes walk from side to side."""
    mono = (x[0] + x[1]) * 0.5
    sig = S.butter(S.butter(mono, hp, 'high', 2), lp, 'low', 2)
    n = x.shape[1]
    out = np.zeros((2, n), np.float32)
    d = int(delay_sec * SR)
    for i in range(1, repeats + 1):
        off = d * i
        if off >= n:
            break
        g = feedback ** (i - 1)
        p = spread * (1 if i % 2 else -1)
        L = np.sqrt((1 - p) / 2)
        R = np.sqrt((1 + p) / 2)
        out[0, off:] += (sig[: n - off] * g * L).astype(np.float32)
        out[1, off:] += (sig[: n - off] * g * R).astype(np.float32)
        sig = np.tanh(drive * sig) / drive
        sig = S.butter(S.butter(sig, hp * 1.12 ** i, 'high', 1), lp * 0.9 ** i, 'low', 1)
    return out


def plate_ir(t60=(1.6, 2.2, 1.6, 0.9), length=3.2, predelay=0.018, seed=21):
    r = np.random.default_rng(seed)
    n = int(length * SR)
    t = _t(n)
    bands = [(None, 300), (300, 2500), (2500, 7000), (7000, None)]
    ir = np.zeros((2, n))
    for (lo, hi), T60 in zip(bands, t60):
        x = r.standard_normal((2, n))
        if lo is None:
            x = S.butter(x, hi, 'low', 2)
        elif hi is None:
            x = S.butter(x, lo, 'high', 2)
        else:
            x = S.butter(x, [lo, hi], 'band', 2)
        ir += x * np.exp(-6.9 * t / T60)
    ir *= np.minimum(t / 0.02, 1)
    pd = int(predelay * SR)
    ir = np.concatenate([np.zeros((2, pd)), ir[:, : n - pd]], axis=1)
    ir /= np.sqrt(np.sum(ir ** 2) / 2)
    return ir.astype(np.float32)


def convolve(x, ir):
    out = np.empty((2, x.shape[1]), np.float32)
    for c in range(2):
        out[c] = ss.oaconvolve(x[c], ir[c], mode='full')[: x.shape[1]].astype(np.float32)
    return out


def autopan(x, rate=2.2, depth=0.35, phase=0.0):
    """Stereo tremolo (Rhodes 'vibrato' knob)."""
    t = _t(x.shape[1])
    p = depth * np.sin(2 * np.pi * rate * t + phase)
    gl = np.sqrt(np.clip(1 - p, 0, 2) / 2) * np.sqrt(2)
    gr = np.sqrt(np.clip(1 + p, 0, 2) / 2) * np.sqrt(2)
    return np.stack([x[0] * gl, x[1] * gr]).astype(np.float32)
