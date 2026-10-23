"""Small DSP toolkit for the Stellar Frontier sound-effect generator.

Everything works on float64 numpy arrays at RATE Hz.  Mono signals are 1-D,
stereo signals are (n, 2).  numba is used for the per-sample filters when it
is installed; without it the same code runs (slowly) as plain Python.
"""
import math

import numpy as np
from scipy.signal import fftconvolve, lfilter

try:
    from numba import njit
except ImportError:  # pragma: no cover
    def njit(*a, **k):
        if a and callable(a[0]):
            return a[0]
        return lambda f: f

RATE = 48000


# ------------------------------------------------------------------ basics

def n_of(seconds):
    return max(1, int(round(seconds * RATE)))


def time(seconds):
    return np.arange(n_of(seconds)) / RATE


def ramp(n, a, b, curve="exp"):
    """A sweep from a to b over n samples (exp is perceptually even for pitch)."""
    x = np.linspace(0.0, 1.0, n)
    if curve == "exp" and a > 0 and b > 0:
        return a * (b / a) ** x
    if curve == "log":
        x = 1.0 - (1.0 - x) ** 3
    if curve == "quad":
        x = x * x
    return a + (b - a) * x


def env_points(n, points, curve=1.0):
    """Piecewise envelope through (time_s, value) points.  curve > 1 bows
    segments toward their start value (faster decays)."""
    t = np.arange(n) / RATE
    out = np.zeros(n)
    pts = sorted(points)
    for (t0, v0), (t1, v1) in zip(pts, pts[1:]):
        m = (t >= t0) & (t < t1)
        if not m.any():
            continue
        x = (t[m] - t0) / max(t1 - t0, 1e-9)
        if v1 < v0:
            x = 1.0 - (1.0 - x) ** curve
        else:
            x = x ** (1.0 / curve) if curve != 1.0 else x
        out[m] = v0 + (v1 - v0) * x
    out[t >= pts[-1][0]] = pts[-1][1]
    out[t < pts[0][0]] = pts[0][1]
    return out


def env_exp(n, tau, attack=0.001):
    t = np.arange(n) / RATE
    a = np.clip(t / max(attack, 1e-6), 0.0, 1.0)
    return a * np.exp(-t / tau)


def fade(x, fin=0.001, fout=0.01):
    x = x.copy()
    a, b = n_of(fin), n_of(fout)
    w = np.ones(len(x))
    if a > 1:
        w[:a] = np.sin(np.linspace(0, math.pi / 2, a)) ** 2
    if b > 1:
        w[-b:] = np.cos(np.linspace(0, math.pi / 2, b)) ** 2
    return x * (w[:, None] if x.ndim == 2 else w)


def pad(x, n):
    if len(x) >= n:
        return x[:n]
    shape = (n - len(x),) + x.shape[1:]
    return np.concatenate([x, np.zeros(shape)])


def mix(*parts):
    """Sum signals of differing length (and mono/stereo)."""
    n = max(len(p) for p in parts)
    st = any(p.ndim == 2 for p in parts)
    out = np.zeros((n, 2)) if st else np.zeros(n)
    for p in parts:
        if st and p.ndim == 1:
            p = np.stack([p, p], axis=1)
        out[:len(p)] += p
    return out


def place(dst, src, at_s, gain=1.0):
    i = n_of(at_s) if at_s > 0 else 0
    if i >= len(dst):
        return dst
    m = min(len(src), len(dst) - i)
    if dst.ndim == 2 and src.ndim == 1:
        src = np.stack([src, src], axis=1)
    dst[i:i + m] += src[:m] * gain
    return dst


def stereo(x, pan=0.0):
    """Mono to stereo with equal-power pan (-1 left .. 1 right)."""
    a = (pan + 1.0) * math.pi / 4
    return np.stack([x * math.cos(a), x * math.sin(a)], axis=1) * math.sqrt(2)


def to_mono(x):
    return x.mean(axis=1) if x.ndim == 2 else x


# ------------------------------------------------------------- oscillators

def phase_of(freq, n=None, phase0=0.0):
    if np.isscalar(freq):
        freq = np.full(n, float(freq))
    return (phase0 + np.cumsum(freq) / RATE) % 1.0, freq


def sine(freq, n=None, phase0=0.0, fm=None):
    ph, _ = phase_of(freq, n, phase0)
    if fm is not None:
        return np.sin(2 * math.pi * ph + fm)
    return np.sin(2 * math.pi * ph)


def _blep(t, dt):
    out = np.zeros_like(t)
    m = t < dt
    x = t[m] / dt[m]
    out[m] = x + x - x * x - 1.0
    m2 = t > 1.0 - dt
    x = (t[m2] - 1.0) / dt[m2]
    out[m2] = x * x + x + x + 1.0
    return out


def saw(freq, n=None, phase0=0.0):
    ph, f = phase_of(freq, n, phase0)
    dt = np.clip(np.abs(f) / RATE, 1e-9, 0.5)
    return 2.0 * ph - 1.0 - _blep(ph, dt)


def square(freq, n=None, phase0=0.0, pw=0.5):
    ph, f = phase_of(freq, n, phase0)
    dt = np.clip(np.abs(f) / RATE, 1e-9, 0.5)
    s = np.where(ph < pw, 1.0, -1.0)
    s += _blep(ph, dt)
    s -= _blep((ph + 1.0 - pw) % 1.0, dt)
    return s


def supersaw(freq, n, voices=7, detune=0.012, rng=None):
    rng = rng or np.random.default_rng(0)
    out = np.zeros(n)
    for i in range(voices):
        d = 1.0 + detune * (i - (voices - 1) / 2) / max((voices - 1) / 2, 1)
        out += saw(np.asarray(freq) * d if not np.isscalar(freq) else freq * d, n, rng.random())
    return out / math.sqrt(voices)


def fm_op(carrier, ratio, index, n, phase0=0.0):
    """Two-operator FM: index may be an envelope array."""
    mod = sine(np.asarray(carrier) * ratio if not np.isscalar(carrier) else carrier * ratio, n)
    return sine(carrier, n, phase0, fm=np.asarray(index) * mod)


# ------------------------------------------------------------------ noise

def white(n, rng):
    return rng.standard_normal(n) * 0.5


def pink(n, rng):
    x = rng.standard_normal(n)
    X = np.fft.rfft(x)
    f = np.arange(len(X), dtype=float)
    f[0] = 1.0
    X /= np.sqrt(f)
    y = np.fft.irfft(X, n)
    return y / (np.std(y) + 1e-12) * 0.5


def brown(n, rng):
    x = np.cumsum(rng.standard_normal(n))
    x = lfilter([1, -1], [1, -0.9995], x)
    return x / (np.std(x) + 1e-12) * 0.5


def crackle(n, rng, density, decay=0.0006):
    """Sparse random impulses; density is events/s (scalar or array)."""
    d = np.full(n, density) if np.isscalar(density) else density
    hits = rng.random(n) < d / RATE
    imp = np.zeros(n)
    imp[hits] = rng.uniform(-1, 1, hits.sum())
    k = np.exp(-np.arange(n_of(decay * 8)) / (decay * RATE))
    return fftconvolve(imp, k)[:n]


# ---------------------------------------------------------------- filters

@njit(cache=True)
def _svf(x, cutoff, q, mode):
    # Cytomic/Simper trapezoidal state-variable filter, cutoff per sample.
    n = x.shape[0]
    y = np.empty(n)
    ic1 = 0.0
    ic2 = 0.0
    k = 1.0 / q
    for i in range(n):
        fc = cutoff[i]
        if fc > 0.49 * 48000.0:
            fc = 0.49 * 48000.0
        if fc < 5.0:
            fc = 5.0
        g = math.tan(math.pi * fc / 48000.0)
        a1 = 1.0 / (1.0 + g * (g + k))
        a2 = g * a1
        a3 = g * a2
        v3 = x[i] - ic2
        v1 = a1 * ic1 + a2 * v3
        v2 = ic2 + a2 * ic1 + a3 * v3
        ic1 = 2.0 * v1 - ic1
        ic2 = 2.0 * v2 - ic2
        if mode == 0:
            y[i] = v2
        elif mode == 1:
            y[i] = x[i] - k * v1 - v2
        elif mode == 2:
            y[i] = v1 * k
        else:
            y[i] = x[i] - k * v1
    return y


_MODES = {"lp": 0, "hp": 1, "bp": 2, "notch": 3}


def svf(x, cutoff, q=0.707, mode="lp"):
    if x.ndim == 2:
        return np.stack([svf(x[:, c], cutoff, q, mode) for c in range(2)], axis=1)
    c = np.full(len(x), float(cutoff)) if np.isscalar(cutoff) else np.asarray(cutoff, dtype=float)
    return _svf(np.ascontiguousarray(x, dtype=float), np.ascontiguousarray(c), float(q), _MODES[mode])


def lp(x, f, q=0.707):
    return svf(x, f, q, "lp")


def hp(x, f, q=0.707):
    return svf(x, f, q, "hp")


def bp(x, f, q=1.0):
    return svf(x, f, q, "bp")


def _biquad(kind, f, q, gain_db):
    A = 10 ** (gain_db / 40)
    w = 2 * math.pi * f / RATE
    cw, sw = math.cos(w), math.sin(w)
    al = sw / (2 * q)
    if kind == "peak":
        b = [1 + al * A, -2 * cw, 1 - al * A]
        a = [1 + al / A, -2 * cw, 1 - al / A]
    elif kind == "lowshelf":
        s = 2 * math.sqrt(A) * al
        b = [A * ((A + 1) - (A - 1) * cw + s), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - s)]
        a = [(A + 1) + (A - 1) * cw + s, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - s]
    else:  # highshelf
        s = 2 * math.sqrt(A) * al
        b = [A * ((A + 1) + (A - 1) * cw + s), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - s)]
        a = [(A + 1) - (A - 1) * cw + s, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - s]
    return np.array(b) / a[0], np.array(a) / a[0]


def eq(x, kind, f, gain_db, q=0.707):
    b, a = _biquad(kind, f, q, gain_db)
    return lfilter(b, a, x, axis=0)


def dc_block(x):
    return lfilter([1, -1], [1, -0.999], x, axis=0)


def modal(excite, modes, rng=None):
    """Bank of resonators: modes = [(freq, decay_s, gain), ...]."""
    out = np.zeros(len(excite))
    for f, d, g in modes:
        if f >= RATE * 0.45:
            continue
        r = math.exp(-1.0 / (d * RATE))
        w = 2 * math.pi * f / RATE
        b = [1 - r]
        a = [1, -2 * r * math.cos(w), r * r]
        out += lfilter(b, a, excite) * g
    return out


def metal_modes(base, count, rng, decay=(0.08, 0.6), spread=4.5):
    """Inharmonic partials of a struck plate/hull panel."""
    modes = []
    for i in range(count):
        f = base * (1.0 + spread * (rng.random() ** 1.6) * (i + 1) / count) * rng.uniform(0.97, 1.03)
        modes.append((f, rng.uniform(*decay) * (1.0 - 0.6 * i / count), rng.uniform(0.4, 1.0) / (1 + i * 0.25)))
    return modes


# ------------------------------------------------------------- non-linear

def drive(x, amount):
    """tanh saturation, gain compensated so quiet signals keep their level."""
    return np.tanh(x * amount) / math.tanh(amount) if amount > 0 else x


def fold(x, amount):
    return np.sin(x * amount * math.pi / 2)


def crush(x, bits=8, hold=1):
    q = 2 ** (bits - 1)
    y = np.round(x * q) / q
    if hold > 1:
        y = np.repeat(y[::hold], hold, axis=0)[:len(x)]
    return y


# ---------------------------------------------------------------- spatial

def _ir(seconds, decay, rng, damp=6000.0, predelay=0.012, width=1.0):
    n = n_of(seconds)
    t = np.arange(n) / RATE
    chans = []
    for c in range(2):
        nz = rng.standard_normal(n)
        # highs die faster than lows: blend a bright and a dark tail
        dark = lp(nz, damp * 0.35)
        bright = lp(nz, damp)
        tail = bright * np.exp(-t / (decay * 0.45)) + dark * np.exp(-t / decay) * 1.4
        # sparse early reflections
        er = np.zeros(n)
        for _ in range(14):
            i = int(rng.uniform(0.002, 0.06) * RATE)
            if i < n:
                er[i] += rng.uniform(-1, 1) * 0.8
        ir = tail * 0.25 + er
        chans.append(ir)
    L, R = chans
    mid, side = (L + R) / 2, (L - R) / 2
    L, R = mid + side * width, mid - side * width
    ir = np.stack([L, R], axis=1)
    ir = np.concatenate([np.zeros((n_of(predelay), 2)), ir])
    ir /= np.sqrt((ir ** 2).sum(axis=0).mean()) + 1e-12
    return ir


def reverb(x, wet=0.25, decay=0.6, seconds=None, damp=6000.0, predelay=0.012, width=1.0, seed=99, hp_in=180.0):
    """Convolution with a synthetic decaying-noise room.  Returns stereo,
    longer than the input by the tail.  The send is high-passed so sub
    energy stays dry and tight."""
    rng = np.random.default_rng(seed)
    seconds = seconds or decay * 2.5
    ir = _ir(seconds, decay, rng, damp, predelay, width)
    if x.ndim == 1:
        x = np.stack([x, x], axis=1)
    send = hp(x, hp_in) if hp_in else x
    tail = np.stack([fftconvolve(send[:, c], ir[:, c]) for c in range(2)], axis=1)
    dry = pad(x, len(tail))
    return dry * (1.0 - wet * 0.5) + tail * wet


def delay(x, seconds, feedback=0.35, mix_=0.3, damp=5000.0, spread=0.0):
    if x.ndim == 1:
        x = np.stack([x, x], axis=1)
    d = n_of(seconds)
    reps = 1
    while feedback ** reps > 0.01 and reps < 30:
        reps += 1
    out = pad(x, len(x) + d * reps + n_of(spread))
    tap = x.copy()
    for r in range(1, reps + 1):
        tap = lp(tap, damp) * feedback
        off = d * r
        for c in range(2):
            o = off + (n_of(spread) if (c == 1 and r % 2) else 0)
            out[o:o + len(tap), c] += tap[:, c] * mix_ / feedback
    return out


def widen(x, amount=0.5, seconds=0.011):
    """Decorrelate a mono-ish signal into stereo (Haas plus a little comb)."""
    m = to_mono(x)
    d = n_of(seconds)
    late = np.concatenate([np.zeros(d), m])[:len(m)]
    side = (m - late) * 0.5 * amount
    return np.stack([m + side, m - side], axis=1)


def autopan(x, rate, depth=0.6, phase=0.0):
    m = to_mono(x)
    t = np.arange(len(m)) / RATE
    p = depth * np.sin(2 * math.pi * rate * t + phase)
    a = (p + 1) * math.pi / 4
    return np.stack([m * np.cos(a), m * np.sin(a)], axis=1) * math.sqrt(2)


# --------------------------------------------------------------- dynamics

def limit(x, ceiling=0.95, release=0.08, look=0.002):
    """Look-ahead peak limiter."""
    from scipy.ndimage import maximum_filter1d
    pk = np.abs(x).max(axis=1) if x.ndim == 2 else np.abs(x)
    w = n_of(look) * 2 + 1
    pk = maximum_filter1d(pk, w)
    g = np.minimum(1.0, ceiling / np.maximum(pk, 1e-9))
    # instant attack, smooth release
    r = math.exp(-1.0 / (release * RATE))
    g = _release(g, r)
    g = np.convolve(g, np.ones(w) / w, mode="same")
    return x * (g[:, None] if x.ndim == 2 else g)


@njit(cache=True)
def _release(g, r):
    out = np.empty_like(g)
    s = 1.0
    for i in range(g.shape[0]):
        v = g[i]
        if v < s:
            s = v
        else:
            s = v + (s - v) * r
        out[i] = s
    return out


def compress(x, threshold_db=-18.0, ratio=4.0, attack=0.003, release=0.12, makeup_db=0.0):
    lvl = np.abs(x).max(axis=1) if x.ndim == 2 else np.abs(x)
    a = math.exp(-1.0 / (attack * RATE))
    r = math.exp(-1.0 / (release * RATE))
    env = _follow(lvl, a, r)
    db = 20 * np.log10(np.maximum(env, 1e-9))
    over = np.maximum(db - threshold_db, 0.0)
    gain = 10 ** ((-over * (1 - 1 / ratio) + makeup_db) / 20)
    return x * (gain[:, None] if x.ndim == 2 else gain)


@njit(cache=True)
def _follow(x, a, r):
    out = np.empty_like(x)
    s = 0.0
    for i in range(x.shape[0]):
        v = x[i]
        c = a if v > s else r
        s = v + (s - v) * c
        out[i] = s
    return out


def normalize(x, peak_db=-1.0):
    pk = np.abs(x).max()
    return x * (10 ** (peak_db / 20) / pk) if pk > 0 else x


def loudness(x):
    """Rough momentary loudness in dB: K-weighting-like shelf, then the
    loudest 150 ms window (so tails and silence do not dilute it)."""
    m = to_mono(x)
    k = eq(hp(m, 60.0), "highshelf", 1500.0, 4.0)
    w = n_of(0.15)
    if len(k) <= w:
        return 10 * math.log10(np.mean(k ** 2) + 1e-12)
    c = np.cumsum(np.concatenate([[0.0], k ** 2]))
    ms = (c[w:] - c[:-w]) / w
    return 10 * math.log10(ms.max() + 1e-12)


def set_loudness(x, target_db, peak_ceiling_db=-0.8):
    g = 10 ** ((target_db - loudness(x)) / 20)
    y = x * g
    ceil = 10 ** (peak_ceiling_db / 20)
    if np.abs(y).max() > ceil:
        y = limit(y, ceil)
    return y


def trim_tail(x, floor_db=-54.0, keep=0.02):
    """Cut trailing near-silence, then fade the last few ms."""
    m = np.abs(x).max(axis=1) if x.ndim == 2 else np.abs(x)
    thr = m.max() * 10 ** (floor_db / 20)
    idx = np.nonzero(m > thr)[0]
    end = min(len(x), (idx[-1] if len(idx) else len(x)) + n_of(keep))
    return fade(x[:end], 0.0005, min(0.08, end / RATE * 0.25))


def make_loop(x, xfade=0.25, preroll=0.5):
    """Seamless loop: drop the filters' warm-up, then crossfade the tail
    into the head (equal power, as the layers are noise-like)."""
    x = x[n_of(preroll):]
    n = n_of(xfade)
    body = x[:-n].copy()
    head = x[:n]
    tail = x[-n:]
    a = np.sin(np.linspace(0, math.pi / 2, n))
    b = np.cos(np.linspace(0, math.pi / 2, n))
    if x.ndim == 2:
        a, b = a[:, None], b[:, None]
    body[:n] = head * a + tail * b
    return body
