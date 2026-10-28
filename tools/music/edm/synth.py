# synth.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# A small software synthesizer and mixing toolkit used to produce the
# electronic remixes of the game's music (tools/music/edm/).  Everything is
# synthesized here from oscillators and noise: no samples, no SoundFonts.
#
#   oscillators   band-limited saw (polyBLEP), sine, additive plucks, FM bell,
#                 additive piano
#   instruments   supersaw (lead / pad / stab), pluck, sub bass, rolling bass,
#                 kick, clap, snare, hats, ride, crash, toms, risers, impacts
#   effects       TPT state-variable filter (numba), ping-pong delay,
#                 sidechain ducking; reverb, chorus, EQ, compression and
#                 limiting come from Spotify's pedalboard library
import numpy as np
import numba
import scipy.signal as ss

SR = 48000
NOTE = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5,
        'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}


def m(name):
    """'C#5' -> MIDI note number."""
    if isinstance(name, (int, np.integer)):
        return int(name)
    return 12 * (int(name[-1]) + 1) + NOTE[name[:-1]]


def hz(midi):
    return 440.0 * 2.0 ** ((np.asarray(midi, float) - 69.0) / 12.0)


_rng = np.random.default_rng(1997)


def rng():
    return _rng


# ---------------------------------------------------------------- filters

@numba.njit(cache=True)
def _svf(x, fc, q, mode):
    """Zavalishin TPT state-variable filter; fc per sample; mode 0 LP, 1 BP, 2 HP."""
    n = x.shape[0]
    y = np.empty(n)
    ic1 = 0.0
    ic2 = 0.0
    k = 1.0 / q
    for i in range(n):
        f = fc[i]
        if f > 0.49 * SR:
            f = 0.49 * SR
        if f < 10.0:
            f = 10.0
        g = np.tan(np.pi * f / SR)
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
            y[i] = v1
        else:
            y[i] = x[i] - k * v1 - v2
    return y


def svf(x, fc, q=0.707, mode='lp'):
    """Filter mono (n,) or stereo (2, n) audio; fc scalar or per-sample array."""
    md = {'lp': 0, 'bp': 1, 'hp': 2}[mode]
    x = np.asarray(x, float)
    n = x.shape[-1]
    f = np.broadcast_to(np.asarray(fc, float), (n,)).astype(float).copy()
    if x.ndim == 1:
        return _svf(x, f, float(q), md)
    return np.stack([_svf(np.ascontiguousarray(c), f, float(q), md) for c in x])


def butter(x, fc, kind='low', order=2):
    sos = ss.butter(order, fc, kind, fs=SR, output='sos')
    return ss.sosfilt(sos, x, axis=-1)


# ------------------------------------------------------------ oscillators

def saw(freq, n, phase=0.0):
    """polyBLEP saw, -1..1; freq scalar or per-sample array."""
    f = np.broadcast_to(np.asarray(freq, float), (n,))
    dt = f / SR
    ph = (phase + np.cumsum(dt) - dt[0]) % 1.0
    y = 2.0 * ph - 1.0
    mk = ph < dt
    x = ph[mk] / dt[mk]
    y[mk] -= x + x - x * x - 1.0
    mk = ph > 1.0 - dt
    x = (ph[mk] - 1.0) / dt[mk]
    y[mk] -= x * x + x + x + 1.0
    return y


def sine(freq, n, phase=0.0):
    f = np.broadcast_to(np.asarray(freq, float), (n,))
    return np.sin(2 * np.pi * (phase + np.cumsum(f) / SR))


def env_adsr(n, gate, a=0.005, d=0.2, s=0.7, r=0.15):
    """ADSR; gate in seconds; r is the release time constant."""
    t = np.arange(n) / SR

    def held(tt):
        att = np.minimum(tt / a, 1.0) if a > 0 else np.ones_like(tt)
        dec = s + (1 - s) * np.exp(-np.maximum(tt - a, 0) / max(d, 1e-4))
        return np.where(tt < a, att, dec)

    e = held(t)
    g = held(np.array([gate]))[0]
    rel = t >= gate
    e[rel] = g * np.exp(-(t[rel] - gate) / max(r, 1e-4))
    return e


def pan2(x, p):
    """Equal-power pan of mono x; p -1 (left) .. 1 (right)."""
    return np.stack([x * np.sqrt((1 - p) / 2), x * np.sqrt((1 + p) / 2)])


def vibrato(n, onset=0.3, rate=5.3, depth=0.12):
    """Semitone offsets: delayed vibrato for held notes."""
    t = np.arange(n) / SR
    amt = np.clip((t - onset) / 0.4, 0, 1) * depth
    return amt * np.sin(2 * np.pi * rate * t)


# ----------------------------------------------------------- instruments

def supersaw(midi, dur, vel=1.0, voices=7, detune=0.22, width=0.9,
             a=0.004, d=0.4, s=0.75, r=0.18,
             cut=7000.0, cut_env=0.0, cut_decay=0.25, q=0.8,
             vib=0.0, sub=0.0, center=1.0):
    """JP-8000 style supersaw with a filter envelope; returns (2, n)."""
    n = int((dur + 6 * r) * SR) + 1
    offs = np.linspace(-1, 1, voices)
    shape = np.sign(offs) * np.abs(offs) ** 1.4
    vb = vibrato(n, depth=vib) if vib else 0.0
    L = np.zeros(n)
    R = np.zeros(n)
    for i, o in enumerate(shape):
        f = hz(midi + o * detune + vb)
        w = saw(f, n, rng().random())
        g = center if o == 0 else 0.62
        p = offs[i] * width
        L += w * g * np.sqrt((1 - p) / 2)
        R += w * g * np.sqrt((1 + p) / 2)
    if sub:
        sw = sine(hz(midi - 12), n) * sub
        L += sw
        R += sw
    t = np.arange(n) / SR
    fc = cut + cut_env * np.exp(-t / cut_decay)
    out = svf(np.stack([L, R]), fc, q)
    out = butter(out, max(25.0, hz(midi) * 0.5), 'high', 1)
    e = env_adsr(n, dur, a, d, s, r)
    return out * e * vel / np.sqrt(voices)


def pluck(midi, dur, vel=1.0, decay=0.22, bright=1.0, r=0.06, detune=0.08, width=0.6):
    """Additive saw pluck: upper harmonics die first (a filter-envelope sound)."""
    f0 = float(hz(midi))
    n = int((dur + 6 * r + 0.05) * SR)
    t = np.arange(n) / SR
    K = int(min(48, 15000 / f0))
    out = np.zeros((2, n))
    for side, dv in ((0, -detune), (1, detune)):
        f = f0 * 2 ** (dv / 12)
        acc = np.zeros(n)
        ph = rng().random(K) * 2 * np.pi
        for k in range(1, K + 1):
            tau = decay / (1 + 0.55 * (k - 1) / bright)
            acc += np.sin(2 * np.pi * k * f * t + ph[k - 1]) * np.exp(-t / tau) / k
        out[side] = acc
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / r))
    att = np.minimum(t / 0.002, 1)
    c = (out[0] + out[1]) / 2
    sdiff = (out[0] - out[1]) / 2 * width
    return np.stack([c + sdiff, c - sdiff]) * gate * att * vel * 0.6


def bell(midi, dur, vel=1.0, r=1.2):
    """Two-operator FM bell."""
    f = float(hz(midi))
    n = int((dur + 5 * r) * SR)
    t = np.arange(n) / SR
    idx = 2.2 * np.exp(-t / 0.5) + 0.3
    y = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * 3.5 * f * t))
    y += 0.5 * np.sin(2 * np.pi * f * t + 0.8 * np.exp(-t / 0.3) * np.sin(2 * np.pi * f * t))
    e = np.exp(-t / (0.9 * r)) * np.minimum(t / 0.002, 1)
    e *= np.where(t < dur + 0.5 * r, 1, np.exp(-(t - dur - 0.5 * r) / 0.3))
    return pan2(y * e * vel * 0.5, 0.0)


def piano(midi, dur, vel=0.8, r=0.35):
    """Additive piano-like tone: stretched partials, two detuned strings, hammer."""
    f0 = float(hz(midi))
    n = int((dur + 5 * r + 0.1) * SR)
    t = np.arange(n) / SR
    B = 0.00035
    K = int(min(28, 12000 / f0))
    base_t60 = 3.5 * (261.6 / f0) ** 0.5 + 0.6
    out = np.zeros((2, n))
    for side, cents in ((0, -0.9), (1, 0.9)):
        acc = np.zeros(n)
        for k in range(1, K + 1):
            fk = k * f0 * np.sqrt(1 + B * k * k) * 2 ** (cents / 1200)
            amp = (1.0 / k ** 1.15) * (vel ** (0.4 + 0.12 * k))
            tau = base_t60 / 6.9 / (1 + 0.28 * (k - 1))
            acc += amp * np.sin(2 * np.pi * fk * t) * np.exp(-t / tau)
        out[side] = acc
    hammer = butter(rng().standard_normal(n) * np.exp(-t / 0.004), [800, 5000], 'band') * 0.15 * vel
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / r))
    p = np.clip((midi - 66) / 30, -0.5, 0.5)
    mono = (out[0] + out[1]) / 2
    side = (out[0] - out[1]) / 2
    st = np.stack([mono + side, mono - side]) * gate * np.minimum(t / 0.001, 1)
    st += hammer
    return np.stack([st[0] * np.sqrt(1 - p), st[1] * np.sqrt(1 + p)]) * 0.35


def sub_bass(midi, dur, vel=1.0, a=0.004, r=0.03, drive=1.4):
    n = int((dur + 6 * r) * SR)
    y = sine(hz(midi), n)
    y = np.tanh(drive * y) / np.tanh(drive)
    e = env_adsr(n, dur, a, 0.5, 1.0, r)
    y = y * e * vel
    return np.stack([y, y])


def mid_bass(midi, dur, vel=1.0, cut=350.0, cut_env=1500.0, decay=0.07, r=0.03):
    """Short, punchy filtered saw bass (mono) for the rolling bass line."""
    n = int((dur + 6 * r) * SR)
    w = saw(hz(midi), n) * 0.6 + saw(hz(midi + 0.08), n, 0.37) * 0.4
    t = np.arange(n) / SR
    fc = cut + cut_env * np.exp(-t / decay)
    y = svf(w, fc, 1.1)
    y = butter(y, 45, 'high', 2)
    e = env_adsr(n, dur, 0.002, 0.12, 0.6, r)
    y = np.tanh(1.6 * y * e) * vel
    return np.stack([y, y])


# ----------------------------------------------------------------- drums

def kick(vel=1.0, length=0.42, f_end=51.9, f_start=165.0):
    # f_end: G#1, the fifth of C# minor (the kick's pitch is audible)
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_start - f_end) * np.exp(-t / 0.032) + 420 * np.exp(-t / 0.0035)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.20) * np.minimum(t / 0.0008, 1)
    amp *= np.where(t > length - 0.06, np.clip((length - t) / 0.06, 0, 1), 1)
    click = butter(rng().standard_normal(n), 3000, 'high', 2) * np.exp(-t / 0.0025) * 0.35
    y = np.tanh(2.2 * body * amp) * 0.9 + click
    return np.stack([y, y]) * vel


def clap(vel=1.0):
    n = int(0.5 * SR)
    t = np.arange(n) / SR
    noise = rng().standard_normal(n)
    e = np.zeros(n)
    for k, off in enumerate((0.0, 0.010, 0.021)):
        u = t - off
        e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0045), 0) * (0.8 + 0.1 * k)
    u = t - 0.03
    e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.11), 0) * 0.7
    y = butter(noise * e, [900, 6500], 'band', 2)
    L = y
    R = np.roll(y, 37)
    return np.stack([L, R]) * vel * 1.4


def snare(vel=1.0, tone=200.0):
    n = int(0.3 * SR)
    t = np.arange(n) / SR
    body = np.sin(2 * np.pi * tone * t * (1 + 0.3 * np.exp(-t / 0.01))) * np.exp(-t / 0.05)
    nz = butter(rng().standard_normal(n), 1200, 'high', 2) * np.exp(-t / 0.09)
    y = (0.6 * body + 0.8 * nz) * np.minimum(t / 0.0005, 1)
    return np.stack([y, y]) * vel


_HAT_F = np.array([205.3, 304.4, 369.6, 522.7, 540.0, 800.0]) * 1.7


def _metal(n):
    t = np.arange(n) / SR
    y = np.zeros(n)
    for f in _HAT_F:
        y += np.sign(np.sin(2 * np.pi * f * t + rng().random() * 6.28))
    return y / len(_HAT_F)


def hat(vel=1.0, open_=False, pan=0.0):
    length = 0.45 if open_ else 0.09
    n = int(length * SR)
    t = np.arange(n) / SR
    tau = 0.13 if open_ else 0.022
    src = 0.55 * _metal(n) + 0.6 * rng().standard_normal(n)
    y = butter(src, 7500 if not open_ else 6500, 'high', 4) * np.exp(-t / tau)
    return pan2(y * vel * 0.9, pan)


def ride(vel=1.0, pan=0.25):
    n = int(1.2 * SR)
    t = np.arange(n) / SR
    src = _metal(n) * 0.7 + 0.4 * rng().standard_normal(n)
    y = butter(src, [3500, 12000], 'band', 2) * (0.6 * np.exp(-t / 0.02) + 0.4 * np.exp(-t / 0.45))
    return pan2(y * vel * 0.8, pan)


def crash(vel=1.0, length=3.0):
    n = int(length * SR)
    t = np.arange(n) / SR
    out = []
    for _ in range(2):
        src = 0.5 * _metal(n) + rng().standard_normal(n)
        y = butter(src, 3800, 'high', 2) * (0.5 * np.exp(-t / 0.03) + 0.5 * np.exp(-t / 0.9))
        out.append(y)
    return np.stack(out) * vel * 0.7


def tom(midi, vel=1.0):
    n = int(0.6 * SR)
    t = np.arange(n) / SR
    f = hz(midi) * (1 + 0.6 * np.exp(-t / 0.03))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.18)
    y += butter(rng().standard_normal(n), 2500, 'low', 2) * np.exp(-t / 0.02) * 0.4
    y = np.tanh(1.5 * y)
    return np.stack([y, y]) * vel


def reverse_cymbal(length):
    c = crash(1.0, length + 0.3)[:, : int(length * SR)]
    return c[:, ::-1].copy()


def noise_riser(length, f0=300.0, f1=9000.0, q=2.0):
    n = int(length * SR)
    x = rng().standard_normal((2, n))
    u = np.linspace(0, 1, n)
    fc = f0 * (f1 / f0) ** (u ** 1.5)
    y = svf(x, fc, q, 'bp')
    return y * (u ** 1.7) * 0.6


def downlifter(length, f0=8000.0, f1=200.0):
    n = int(length * SR)
    x = rng().standard_normal((2, n))
    u = np.linspace(0, 1, n)
    fc = f0 * (f1 / f0) ** (u ** 0.6)
    y = svf(x, fc, 1.5, 'bp')
    return y * np.exp(-u * 4) * 0.6


def impact(vel=1.0):
    n = int(2.5 * SR)
    t = np.arange(n) / SR
    f = 34 + 40 * np.exp(-t / 0.08)
    boom = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.7)
    boom = np.tanh(1.8 * boom)
    return np.stack([boom, boom]) * vel


def pitch_riser(length, midi0, midi1, vel=1.0):
    n = int(length * SR)
    u = np.linspace(0, 1, n)
    mid = midi0 + (midi1 - midi0) * u ** 1.6
    L = np.zeros(n)
    R = np.zeros(n)
    for o, p in ((-0.25, -0.7), (0.0, 0.0), (0.25, 0.7)):
        w = saw(hz(mid + o), n, rng().random())
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    y = svf(np.stack([L, R]), 400 + 7000 * u ** 2, 0.9)
    return y * u ** 2 * vel * 0.35


# ----------------------------------------------------------------- mixing

class Bus:
    def __init__(self, n):
        self.x = np.zeros((2, n), np.float32)

    def add(self, sig, t_sec, gain=1.0):
        i = int(round(t_sec * SR))
        if i >= self.x.shape[1]:
            return
        sig = np.asarray(sig)
        k = min(sig.shape[1], self.x.shape[1] - i)
        if i < 0:
            sig = sig[:, -i:]
            k = min(sig.shape[1], self.x.shape[1])
            i = 0
        self.x[:, i:i + k] += (sig[:, :k] * gain).astype(np.float32)


def sidechain(n, kick_times, depth=0.7, release=0.32, attack=0.004, power=2.0):
    """Gain curve ducking after each kick (seconds)."""
    g = np.ones(n, np.float32)
    L = int(release * SR)
    u = np.arange(L) / SR
    shape = np.minimum(u / attack, 1.0) * (1 - u / release) ** power
    curve = (1 - depth * shape).astype(np.float32)
    for tk in kick_times:
        i = int(round(tk * SR))
        if i >= n:
            continue
        k = min(L, n - i)
        g[i:i + k] = np.minimum(g[i:i + k], curve[:k])
    return g


def pingpong(x, delay_sec, feedback=0.45, repeats=8, lp=5000.0, hp=350.0):
    """Ping-pong echoes of a stereo signal (wet only)."""
    mono = (x[0] + x[1]) * 0.5
    mono = butter(butter(mono, hp, 'high', 2), lp, 'low', 2)
    n = x.shape[1]
    out = np.zeros((2, n), np.float32)
    d = int(delay_sec * SR)
    sig = mono
    for i in range(1, repeats + 1):
        off = d * i
        if off >= n:
            break
        g = feedback ** (i - 1)
        ch = (i - 1) % 2
        out[ch, off:] += (sig[: n - off] * g).astype(np.float32)
        if i % 2 == 0:
            sig = butter(sig, lp * 0.85 ** i, 'low', 1)
    return out


def rms_db(x):
    return 20 * np.log10(np.sqrt(np.mean(np.square(x, dtype=np.float64))) + 1e-12)
