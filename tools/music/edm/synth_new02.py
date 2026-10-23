# synth_new02.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Late-1990s progressive-house instruments for new02_proghouse.py, built on the
# shared toolkit synth.py (not modified here).  All synthesized; no samples.
#
#   kick909     TR-909 style kick: sine body with a pitch sweep, tuned end pitch,
#               short knock, soft saturation
#   clap_soft   a soft, round 909-ish clap: three short band-limited bursts, a
#               short tonal body, a dark tail; nothing above ~6 kHz; round-robin
#   hat909      closed / open metallic hats (six square partials + noise)
#   shaker      soft 16th shaker
#   ride909     quiet ride
#   crash_soft  a darker, shorter crash for section changes
#   tom909      tuned 909-style tom (for the timpani pickups of the original)
#   sub, bass_mid   the rolling bass: sine sub + filtered saw/square roll
#   pluck       the arpeggio pluck (saw + pulse through a filter envelope)
#   warm_pad    JP-8000/Juno-style warm pad (detuned saws, slow filter opening)
#   lead        warm detuned-saw lead (theme), filter envelope, delayed vibrato
#   reed        soft filtered saw/pulse "reed pad" voice for long counter-lines
#   shimmer     high sine/saw shimmer for the string pedal
#   chorus      stereo chorus (modulated delays)
import numpy as np
import scipy.signal as ss

from synth import SR, hz, saw, sine, svf, butter, env_adsr, rng


def _t(n):
    return np.arange(n) / SR


def pulse(freq, n, pw=0.5, phase=0.0):
    return 0.5 * (saw(freq, n, phase) - saw(freq, n, (phase + pw) % 1.0))


# ------------------------------------------------------------------ drums

def kick909(vel=1.0, f_end=51.9, length=0.46):
    n = int(length * SR)
    t = _t(n)
    f = f_end + (185.0 - f_end) * np.exp(-t / 0.030) + 380.0 * np.exp(-t / 0.003)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.27) * np.minimum(t / 0.0007, 1)
    amp *= np.clip((length - t) / 0.07, 0, 1)
    g = np.random.default_rng(909)
    knock = butter(g.standard_normal(n), [900, 4000], 'band', 1) * np.exp(-t / 0.0035) * 0.22
    y = np.tanh(1.7 * body * amp) / np.tanh(1.7) * 0.92 + knock
    return np.stack([y, y]) * vel


def _room(dry, decay=0.12, lp=3500.0, wet_db=-12.0, seed=0, length=0.25):
    g = np.random.default_rng(seed)
    L = int(length * SR)
    tt = _t(L)
    ir = g.standard_normal((2, L)) * np.exp(-tt / decay) * np.minimum(tt / 0.005, 1)
    ir[:, -int(0.02 * SR):] *= np.linspace(1, 0, int(0.02 * SR))
    ir = butter(butter(ir, lp, 'low', 2), 300, 'high', 2)
    ir /= np.sqrt(np.sum(ir[0] ** 2))
    wet = np.stack([ss.fftconvolve(dry, ir[c]) for c in range(2)])
    wet *= 10 ** (wet_db / 20) * np.sqrt(np.sum(dry ** 2)) / (np.sqrt(np.sum(wet[0] ** 2)) + 1e-9)
    out = wet
    out[:, :dry.shape[0]] += dry
    k = int(0.01 * SR)
    out[:, -k:] *= np.linspace(1, 0, k)
    return out


def clap_soft(seed=0, vel=1.0):
    """Soft, round clap: short bursts band-limited to ~0.9-3.2 kHz, a 220 Hz body,
    a 45 ms dark tail, a small room.  `seed` = round-robin variant."""
    g = np.random.default_rng(700 + seed)
    n = int(0.22 * SR)
    t = _t(n)
    sp = 1.0 + (g.random() - 0.5) * 0.25          # burst spacing varies per variant
    fc = 1500.0 * (1 + (g.random() - 0.5) * 0.2)
    nz = g.standard_normal(n)
    e = np.zeros(n)
    for k, off in enumerate((0.0, 0.0085 * sp, 0.0175 * sp)):
        u = t - off
        e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0032), 0) * (0.75 + 0.12 * k)
    u = t - 0.024 * sp
    e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.045), 0) * 0.55
    burst = butter(nz, [fc * 0.6, fc * 2.1], 'band', 2) * e
    f = 222.0 * (1 + (g.random() - 0.5) * 0.04) * (1 + 0.35 * np.exp(-t / 0.008))
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.032) * 0.45
    y = (burst * 1.6 * vel ** 0.4 + body) * np.minimum(t / 0.0006, 1)
    y *= np.clip((0.22 - t) / 0.03, 0, 1)
    y = butter(y, 6000, 'low', 2) * (1.0 + (g.random() - 0.5) * 0.15)
    out = _room(y, decay=0.075, wet_db=-12.0, seed=seed + 31)
    out[1] = np.roll(out[1], 23)
    return out * vel


_HAT_F = np.array([205.3, 304.4, 369.6, 522.7, 540.0, 800.0]) * 1.72


def _metal(n, g):
    t = _t(n)
    y = np.zeros(n)
    for f in _HAT_F:
        y += np.sign(np.sin(2 * np.pi * f * t + g.random() * 6.28))
    return y / len(_HAT_F)


def hat909(seed=0, open_=False):
    g = np.random.default_rng(800 + seed + (50 if open_ else 0))
    length = 0.36 if open_ else 0.08
    n = int(length * SR)
    t = _t(n)
    tau = 0.10 if open_ else 0.020 * (1 + (g.random() - 0.5) * 0.3)
    src = 0.6 * _metal(n, g) + 0.5 * g.standard_normal(n)
    y = butter(butter(src, 7200, 'high', 4), 13500, 'low', 2) * np.exp(-t / tau)
    y *= np.minimum(t / 0.0005, 1) * np.clip((length - t) / 0.02, 0, 1)
    return np.stack([y, y]) * 0.8


def shaker(seed=0):
    g = np.random.default_rng(850 + seed)
    n = int(0.09 * SR)
    t = _t(n)
    e = np.minimum(t / 0.007, 1) * np.exp(-np.maximum(t - 0.007, 0) / 0.028)
    y = butter(g.standard_normal(n), [4500, 10500], 'band', 2) * e
    return np.stack([y, np.roll(y, 11)]) * 0.7


def ride909(seed=0):
    g = np.random.default_rng(870 + seed)
    n = int(1.1 * SR)
    t = _t(n)
    src = 0.7 * _metal(n, g) + 0.35 * g.standard_normal(n)
    y = butter(src, [4200, 11000], 'band', 2) * (0.55 * np.exp(-t / 0.018) + 0.45 * np.exp(-t / 0.42))
    y *= np.clip((1.1 - t) / 0.1, 0, 1)
    return np.stack([y * 0.9, y * 1.05]) * 0.7


def crash_soft(length=2.6):
    g = np.random.default_rng(880)
    n = int(length * SR)
    t = _t(n)
    out = []
    for _ in range(2):
        src = 0.45 * _metal(n, g) + g.standard_normal(n)
        y = butter(butter(src, 3200, 'high', 2), 10500, 'low', 2)
        y *= (0.4 * np.exp(-t / 0.025) + 0.6 * np.exp(-t / 0.75)) * np.clip((length - t) / 0.3, 0, 1)
        out.append(y)
    return np.stack(out) * 0.6


def tom909(midi, vel=1.0):
    n = int(0.45 * SR)
    t = _t(n)
    f = hz(midi) * (1 + 0.45 * np.exp(-t / 0.025))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.16)
    y += butter(np.random.default_rng(int(midi)).standard_normal(n), 1800, 'low', 2) * np.exp(-t / 0.012) * 0.25
    y = np.tanh(1.3 * y) * np.minimum(t / 0.0006, 1) * np.clip((0.45 - t) / 0.05, 0, 1)
    return np.stack([y, y]) * vel


# ------------------------------------------------------------------- bass

def sub(midi, dur, vel=1.0, a=0.006, r=0.04):
    n = int((dur + 6 * r) * SR)
    y = sine(hz(midi), n)
    y = np.tanh(1.3 * y) / np.tanh(1.3)
    y = y * env_adsr(n, dur, a, 0.5, 1.0, r) * vel
    return np.stack([y, y])


def bass_mid(midi, dur, vel=1.0, cut=280.0, env=1100.0, decay=0.07, q=1.0):
    """Round rolling-bass note: saw + square an octave below, filter blip."""
    n = int((dur + 0.1) * SR)
    t = _t(n)
    f = float(hz(midi))
    w = 0.7 * saw(f * 2 ** (0.04 / 12), n, 0.2) + 0.3 * saw(f * 2 ** (-0.04 / 12), n, 0.7)
    w += 0.35 * pulse(f / 2, n, 0.5, 0.1)
    fc = cut + env * np.exp(-t / decay)
    y = svf(svf(w, fc, 0.6), fc, q)
    y = butter(y, 40, 'high', 2)
    y = np.tanh(1.5 * y * env_adsr(n, dur, 0.002, 0.10, 0.55, 0.02)) * vel
    return np.stack([y, y])


# -------------------------------------------------------------- melodic

def pluck(midi, dur, vel=1.0, cut=700.0, env=3800.0, decay=0.085, q=1.3, adecay=0.32):
    n = int((dur + 0.35) * SR)
    t = _t(n)
    f = float(hz(midi))
    y = 0.7 * saw(f * 1.002, n, 0.3) + 0.5 * pulse(f * 0.998, n, 0.4, 0.6)
    fc = cut + env * np.exp(-t / decay)
    y = svf(y, fc, q)
    y = butter(y, 150, 'high', 1)
    amp = np.exp(-t / adecay) * np.minimum(t / 0.0015, 1)
    amp *= np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.06))
    y = y * amp * vel * 0.7
    return np.stack([y, y])


def warm_pad(midi, dur, vel=1.0, a=0.8, r=1.2, cut=1300.0, cut_open=1200.0, open_t=2.0, detune=0.11):
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    drift = 0.03 * np.sin(2 * np.pi * (0.17 + 0.1 * rng().random()) * t + rng().random() * 6.28)
    f = hz(midi + drift)
    o1 = saw(f * 2 ** (detune / 12), n, rng().random())
    o2 = saw(f * 2 ** (-detune / 12), n, rng().random())
    o3 = saw(f * 2 ** (detune * 0.4 / 12), n, rng().random())
    o4 = pulse(f, n, 0.45, rng().random())
    L = 0.7 * o1 + 0.35 * o3 + 0.35 * o4 + 0.2 * o2
    R = 0.7 * o2 + 0.35 * o3 + 0.35 * o4 + 0.2 * o1
    fc = (cut + cut_open * (1 - np.exp(-t / open_t))) * (float(hz(midi)) / 330.0) ** 0.2
    x = svf(np.stack([L, R]), fc, 0.55)
    x = svf(x, fc, 0.7)
    x = butter(x, 100, 'high', 2)
    return x * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 0.45


def lead(midi, dur, vel=1.0, voices=5, detune=0.13, width=0.7, a=0.012, d=0.5, s=0.8, r=0.25,
         cut=2400.0, env=2600.0, f_dec=0.25, q=0.9, vib=0.11, vib_onset=0.35, glide_from=None):
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    pm = np.full(n, float(midi))
    if glide_from is not None:
        pm = midi + (glide_from - midi) * np.exp(-t / 0.035)
    if vib:
        pm = pm + np.clip((t - vib_onset) / 0.6, 0, 1) * vib * np.sin(2 * np.pi * 5.0 * t + rng().random() * 6)
    offs = np.linspace(-1, 1, voices)
    L = np.zeros(n)
    R = np.zeros(n)
    for o in offs:
        w = saw(hz(pm + o * detune), n, rng().random())
        p = o * width
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    sw = 0.35 * sine(hz(pm), n)
    L += sw
    R += sw
    fc = (cut + env * np.exp(-t / f_dec)) * (float(hz(midi)) / 660.0) ** 0.3
    x = svf(np.stack([L, R]) / np.sqrt(voices), fc, 0.55)
    x = svf(x, fc, q)
    x = butter(x, max(40.0, float(hz(midi)) * 0.5), 'high', 1)
    return x * env_adsr(n, dur, a, d, s, r) * vel


def reed(midi, dur, vel=1.0, a=0.18, r=0.7, cut=1500.0):
    """Soft filtered saw+pulse duo voice with slow vibrato (the clarinet lines)."""
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    pm = midi + np.clip((t - 0.5) / 0.8, 0, 1) * 0.08 * np.sin(2 * np.pi * 4.6 * t + rng().random() * 6)
    f = hz(pm)
    y1 = 0.6 * saw(f * 1.0015, n, rng().random()) + 0.6 * pulse(f, n, 0.38, rng().random())
    y2 = 0.6 * saw(f * 0.9985, n, rng().random()) + 0.6 * pulse(f, n, 0.62, rng().random())
    fc = cut * (1 + 0.4 * np.minimum(t / 0.6, 1))
    x = svf(svf(np.stack([y1, y2]), fc, 0.6), fc, 0.75)
    x = butter(x, 160, 'high', 2)
    return x * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 0.6


def shimmer(midi, dur, vel=1.0, a=1.2, r=1.6):
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    f = hz(midi + 0.04 * np.sin(2 * np.pi * 0.3 * t))
    y = sine(f, n) + 0.25 * sine(2 * f, n, 0.3)
    s2 = svf(saw(f * 1.004, n, 0.5), 3200, 0.6) * 0.25
    trem = 1 + 0.12 * np.sin(2 * np.pi * 0.21 * t)
    e = env_adsr(n, dur, a, 1.0, 1.0, r) * trem * vel * 0.35
    return np.stack([(y + s2) * e, (y - s2) * e])


def chorus(x, rate=0.45, depth_ms=1.6, center_ms=8.0, mix=0.45, spread=np.pi / 2):
    n = x.shape[1]
    base = np.arange(n, dtype=np.float64)
    out = np.empty_like(x)
    for c in range(2):
        lfo = np.sin(2 * np.pi * rate * base / SR + c * spread)
        idx = base - (center_ms + depth_ms * lfo) * 1e-3 * SR
        del lfo
        wet = np.interp(idx, base, x[c].astype(np.float64))
        del idx
        out[c] = ((1 - mix) * x[c] + mix * wet).astype(np.float32)
        del wet
    return out
