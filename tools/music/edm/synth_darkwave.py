# synth_darkwave.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Cold-wave / darkwave instruments for the alternative song-8 remix
# (song8_darkwave.py), built on the shared toolkit synth.py (which is not
# modified here).  Everything is synthesized; no samples.
#
#   dmx_kick      dry, tight drum-machine kick, pitch-tuned (B1 by default)
#   dark_snare    round, tonal "pdlap" snare: tuned body with a fast pitch drop,
#                 a short 0.9-3.5 kHz snap, a woody shell band, a little wire and
#                 a tiny clap layer; ~110 ms, almost nothing above 7 kHz; seeds
#                 0..3 are round-robin variants (pitch / snap / level)
#   cold_hat      short metallic closed / open hat (square-partial cluster)
#   dmx_tom       pitched drum-machine tom, dry
#   seq_bass      muted, slightly detuned saw + pulse sequencer bass with a sub
#   string_machine  Solina-style divide-down strings (ensemble added on the bus),
#                 optional tempo-synced 16th tremolo
#   cold_pad      two saws + pulse, dark low-passed pad
#   vox           dark static-formant choir pad ("oo" / "oh" / "ah"; no sweeps)
#   pulse_lead    detuned PWM pulse lead with delayed vibrato
#   guitar        Karplus-Strong "clean chorused guitar" pluck (numba)
#   stab          short dark detuned-saw chord stab (for the orchestra hits)
#   ensemble      3-phase string-machine / Juno-style stereo chorus
import numpy as np
import numba

from synth import SR, hz, saw, sine, svf, butter, env_adsr


def _t(n):
    return np.arange(n) / SR


def pulse(freq, n, pw=0.5, phase=0.0):
    """Band-limited pulse: the difference of two polyBLEP saws."""
    if np.ndim(pw):
        # PWM: second saw phase-offset per sample
        f = np.broadcast_to(np.asarray(freq, float), (n,))
        dt = f / SR
        ph = (phase + np.cumsum(dt) - dt[0]) % 1.0
        a = _blep_saw(ph, dt)
        b = _blep_saw((ph + pw) % 1.0, dt)
        return 0.5 * (a - b)
    return 0.5 * (saw(freq, n, phase) - saw(freq, n, (phase + pw) % 1.0))


def _blep_saw(ph, dt):
    y = 2.0 * ph - 1.0
    mk = ph < dt
    x = ph[mk] / dt[mk]
    y[mk] -= x + x - x * x - 1.0
    mk = ph > 1.0 - dt
    x = (ph[mk] - 1.0) / dt[mk]
    y[mk] -= x * x + x + x + 1.0
    return y


# ------------------------------------------------------------------ drums

def dmx_kick(vel=1.0, f_end=61.74, length=0.34, seed=0):
    """Dry, tight kick; f_end 61.74 Hz = B1 (the tonic of B minor)."""
    g = np.random.default_rng(10 + seed)
    n = int(length * SR)
    t = _t(n)
    f = f_end + (180.0 - f_end) * np.exp(-t / 0.026) + 520 * np.exp(-t / 0.0018)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.12) * np.minimum(t / 0.0006, 1)
    amp *= np.clip((length - t) / 0.06, 0, 1)
    click = butter(g.standard_normal(n), [1500, 6000], 'band', 1) * np.exp(-t / 0.0016) * 0.22
    y = np.tanh(2.0 * body * amp) * 0.95 + click
    return np.stack([y, y]).astype(np.float32) * vel


def dark_snare(seed=0, vel=1.0, tone=185.0, clap=0.12):
    """Round, dark drum-machine snare (tone 185 Hz = F#3, the fifth of B)."""
    g = np.random.default_rng(700 + seed)
    pt = 1.0 + (g.random() - 0.5) * 0.04          # +-2 % pitch per variant
    lvl = 1.0 + (g.random() - 0.5) * 0.18          # +-0.8 dB
    snap_fc = 1800.0 * (1 + (g.random() - 0.5) * 0.3)
    n = int(0.22 * SR)
    t = _t(n)
    f0 = tone * pt
    f = f0 * (1 + 0.55 * np.exp(-t / 0.008))
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.075)
    body += 0.32 * np.sin(2 * np.pi * np.cumsum(f * 1.6) / SR) * np.exp(-t / 0.04)
    body = np.tanh(1.5 * body) / np.tanh(1.5)
    nz = g.standard_normal(n)
    v = float(np.clip(vel, 0.2, 1.2))
    snap = butter(nz, [snap_fc * 0.5, min(snap_fc * 1.9, 3500.0)], 'band', 2) * np.exp(-t / 0.016)
    wire = butter(butter(nz, 1400, 'high', 2), 5500, 'low', 2) * np.exp(-t / 0.045)
    shell = butter(nz, [380, 1100], 'band', 2) * np.exp(-t / 0.026)
    y = 0.9 * body + (0.85 * v ** 0.6) * snap * 2.0 + 0.22 * v * wire * 1.6 + 0.85 * shell
    if clap:
        e = np.zeros(n)
        for k, off in enumerate((0.0, 0.007, 0.015)):
            u = t - off
            e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0035), 0)
        y += clap * butter(g.standard_normal(n), [1000, 3000], 'band', 2) * e * 2.0
    y *= np.minimum(t / 0.0005, 1) * np.clip((0.22 - t) / 0.04, 0, 1)
    y = butter(y, 7000, 'low', 2) * lvl
    y = butter(y, 110, 'high', 2)
    y = y / (np.max(np.abs(y)) + 1e-9) * 0.9
    return np.stack([y, y]).astype(np.float32)


def soft_clap(seed=0):
    """Short, band-limited clap (used sparingly, layered on accents)."""
    g = np.random.default_rng(900 + seed)
    n = int(0.25 * SR)
    t = _t(n)
    e = np.zeros(n)
    for k, off in enumerate((0.0, 0.009, 0.019)):
        u = t - off
        e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.004), 0) * (0.8 + 0.1 * k)
    u = t - 0.026
    e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.06), 0) * 0.45
    y = butter(g.standard_normal(n), [900, 3800], 'band', 2) * e
    y *= np.clip((0.25 - t) / 0.03, 0, 1)
    y = y / (np.max(np.abs(y)) + 1e-9) * 0.8
    return np.stack([y, np.roll(y, 29)]).astype(np.float32)


_HAT_F = np.array([205.3, 304.4, 369.6, 522.7, 540.0, 800.0]) * 1.55


def cold_hat(vel=1.0, seed=0, open_=False):
    g = np.random.default_rng(1200 + seed)
    length = 0.32 if open_ else 0.07
    n = int(length * SR)
    t = _t(n)
    y = np.zeros(n)
    for f in _HAT_F * (1 + 0.004 * (seed - 1.5)):
        y += np.sign(np.sin(2 * np.pi * f * t + g.random() * 6.28))
    y = y / len(_HAT_F) * 0.75 + 0.35 * g.standard_normal(n)
    tau = 0.09 if open_ else 0.013
    y = butter(y, [6800, 12500], 'band', 2) * np.exp(-t / tau) * np.minimum(t / 0.0004, 1)
    y *= np.clip((length - t) / 0.01, 0, 1)
    return np.stack([y, y]).astype(np.float32) * vel


def dmx_tom(midi, seed=0):
    g = np.random.default_rng(1500 + seed)
    n = int(0.45 * SR)
    t = _t(n)
    f = hz(midi) * (1 + 0.45 * np.exp(-t / 0.03))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.17)
    y += butter(g.standard_normal(n), [300, 2500], 'band', 1) * np.exp(-t / 0.012) * 0.3
    y = np.tanh(1.5 * y) * np.minimum(t / 0.0005, 1) * np.clip((0.45 - t) / 0.05, 0, 1)
    return np.stack([y, y]).astype(np.float32)


def dark_crash(seed=0, length=2.6):
    g = np.random.default_rng(1700 + seed)
    n = int(length * SR)
    t = _t(n)
    out = []
    for c in range(2):
        y = np.zeros(n)
        for f in _HAT_F * (0.71 + 0.03 * c):
            y += np.sign(np.sin(2 * np.pi * f * t + g.random() * 6.28))
        y = y / len(_HAT_F) * 0.4 + g.standard_normal(n)
        y = butter(butter(y, 2800, 'high', 2), 8500, 'low', 2)
        y *= (0.4 * np.exp(-t / 0.04) + 0.6 * np.exp(-t / 0.75)) * np.minimum(t / 0.002, 1)
        y *= np.clip((length - t) / 0.3, 0, 1)
        out.append(y)
    return np.stack(out).astype(np.float32) * 0.6


# ------------------------------------------------------------------- bass

def seq_bass(midi, dur, vel=1.0, cut=260.0, env=900.0, decay=0.06, q=1.0, sub=0.55):
    """Muted sequencer bass: two saws +-6 cents and a narrow pulse, a short
    filter pluck, a sine sub.  Mono."""
    n = int((dur + 0.1) * SR)
    t = _t(n)
    f = float(hz(midi))
    w = 0.55 * saw(f * 2 ** (0.06 / 12), n, 0.13) + 0.45 * saw(f * 2 ** (-0.06 / 12), n, 0.71)
    w += 0.35 * pulse(f, n, 0.3, 0.4)
    fc = (cut + env * vel * np.exp(-t / decay)) * (f / 61.7) ** 0.3
    y = svf(svf(w, fc, 0.6), fc, q)
    y = y + sub * sine(f, n) * (1.0 if f < 100 else 0.5)
    y = butter(y, 33, 'high', 2)
    y = np.tanh(1.6 * y * env_adsr(n, dur, 0.002, 0.14, 0.55, 0.02)) * vel
    return np.stack([y, y]).astype(np.float32)


def sub_sine(midi, dur, vel=1.0, a=0.01, r=0.08):
    n = int((dur + 6 * r) * SR)
    y = sine(hz(midi), n)
    y = np.tanh(1.3 * y) / np.tanh(1.3) * env_adsr(n, dur, a, 0.5, 1.0, r) * vel
    return np.stack([y, y]).astype(np.float32)


# ---------------------------------------------------------- pads / strings

def string_machine(midi, dur, vel=1.0, a=0.35, r=0.9, cut=3600.0, trem=0.0, trem_hz=8.27,
                   trem_phase=0.0):
    """Solina-ish strings.  trem > 0: a soft amplitude tremolo (tempo-synced
    16ths by default) for the original's tremolo strings."""
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    g = np.random.default_rng(int(midi * 7 + dur * 100) % 9999)
    L = np.zeros(n)
    R = np.zeros(n)
    for o, p, gg in ((-0.11, -0.7, 1.0), (0.0, 0.0, 0.9), (0.10, 0.7, 1.0), (12.03, 0.3, 0.3)):
        vib = 0.04 * np.sin(2 * np.pi * (5.4 + g.random()) * t + g.random() * 6)
        w = saw(hz(midi + o + vib), n, g.random()) * gg
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = svf(np.stack([L, R]), cut, 0.6)
    x = svf(x, cut * 1.3, 0.55)
    x = butter(x, 220, 'high', 2)
    e = env_adsr(n, dur, a, 1.0, 1.0, r)
    if trem:
        ph = 2 * np.pi * trem_hz * t + trem_phase
        e = e * (1 - trem * (0.5 * (1 - np.cos(ph))) ** 1.5)
    return (x * e * vel * 0.32).astype(np.float32)


def cold_pad(midi, dur, vel=1.0, a=0.8, r=1.4, cut=900.0, cut_open=700.0, open_t=2.0):
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    g = np.random.default_rng(int(midi * 13 + dur * 10) % 9999)
    drift = 0.04 * np.sin(2 * np.pi * (0.17 + 0.1 * g.random()) * t + g.random() * 6.28)
    f = hz(midi + drift)
    o1 = saw(f * 2 ** (0.1 / 12), n, g.random())
    o2 = saw(f * 2 ** (-0.1 / 12), n, g.random())
    o3 = pulse(f * 0.5, n, 0.4, g.random())
    L = 0.7 * o1 + 0.3 * o2 + 0.35 * o3
    R = 0.7 * o2 + 0.3 * o1 + 0.35 * o3
    fc = (cut + cut_open * (1 - np.exp(-t / open_t))) * (float(hz(midi)) / 330.0) ** 0.25
    x = svf(np.stack([L, R]), fc, 0.6)
    x = svf(x, fc, 0.7)
    x = butter(x, 110, 'high', 2)
    return (x * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 0.45).astype(np.float32)


_VOWELS = {
    'oo': ((320, 1.0, 5.0), (800, 0.32, 6.0), (2300, 0.06, 9.0)),
    'oh': ((430, 1.0, 5.5), (820, 0.5, 6.5), (2600, 0.08, 9.0)),
    'ah': ((680, 1.0, 6.0), (1080, 0.45, 7.0), (2500, 0.12, 9.0)),
}


def vox(midi, dur, vel=1.0, vowel='oo', a=0.45, r=1.1, voices=5):
    """Dark choir pad with static formants (no vowel movement)."""
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    g = np.random.default_rng(int(midi * 31 + dur * 10) % 9999)
    L = np.zeros(n)
    R = np.zeros(n)
    for i, o in enumerate(np.linspace(-0.16, 0.16, voices)):
        vb = np.clip((t - 0.25) / 0.6, 0, 1) * 0.08 * np.sin(2 * np.pi * (4.8 + 0.35 * i) * t + i)
        w = saw(hz(midi + o + vb), n, g.random())
        p = o / 0.16 * 0.75
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = np.stack([L, R]) / np.sqrt(voices)
    y = np.zeros_like(x)
    for f, gg, q in _VOWELS[vowel]:
        y += svf(x, f, q, 'bp') * gg
    y += 0.12 * svf(x, 700, 0.7)
    y = butter(y, 150, 'high', 2)
    return (y * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 1.5).astype(np.float32)


# ------------------------------------------------------------------- leads

def pulse_lead(midi, dur, vel=1.0, cut=2600.0, a=0.012, r=0.18, vib=0.13, detune=0.07, sub=0.18,
               glide_from=None):
    """Detuned PWM pulse lead: two pulses (L/R-weighted), slow PWM, a quiet
    square an octave down, 2x2-pole low-pass with a small attack envelope."""
    n = int((dur + 5 * r) * SR) + 1
    t = _t(n)
    pm = np.full(n, float(midi))
    if glide_from is not None:
        pm = midi + (glide_from - midi) * np.exp(-t / 0.03)
    if vib:
        pm = pm + np.clip((t - 0.35) / 0.5, 0, 1) * vib * np.sin(2 * np.pi * 5.2 * t)
    pw1 = 0.36 + 0.1 * np.sin(2 * np.pi * 0.6 * t)
    pw2 = 0.40 + 0.1 * np.sin(2 * np.pi * 0.45 * t + 1.7)
    o1 = pulse(hz(pm + detune), n, pw1, 0.1)
    o2 = pulse(hz(pm - detune), n, pw2, 0.6)
    s = pulse(hz(pm - 12), n, 0.5, 0.3) * sub
    L = 0.75 * o1 + 0.35 * o2 + s
    R = 0.35 * o1 + 0.75 * o2 + s
    fc = cut * (1 + 0.6 * np.exp(-t / 0.08)) * (float(hz(midi)) / 700.0) ** 0.3
    x = svf(np.stack([L, R]), fc, 0.6)
    x = svf(x, fc, 0.8)
    x = butter(x, 160, 'high', 2)
    return (x * env_adsr(n, dur, a, 0.5, 0.85, r) * vel * 0.8).astype(np.float32)


@numba.njit(cache=True)
def _ks(exc, n, period, loss, bright):
    size = 8192
    buf = np.zeros(size)
    y = np.zeros(n)
    w = 0
    prev = 0.0
    ne = exc.shape[0]
    for i in range(n):
        rp = w - period
        while rp < 0:
            rp += size
        i0 = int(rp)
        fr = rp - i0
        d = buf[i0 % size] * (1 - fr) + buf[(i0 + 1) % size] * fr
        lp = bright * d + (1 - bright) * prev
        prev = lp
        x = exc[i] if i < ne else 0.0
        v = x + loss[i] * lp
        buf[w] = v
        y[i] = v
        w = (w + 1) % size
    return y


def _loop_coef(f, T):
    """One-pole loop low-pass coefficient b so that the fundamental loses
    1/e of its level in T seconds from the loop filter alone."""
    w = 2 * np.pi * f / SR
    target = np.exp(-2.0 / (T * f))          # |H|^2 per pass
    lo, hi = 0.02, 0.9999
    for _ in range(50):
        b = 0.5 * (lo + hi)
        h2 = b * b / (1 - 2 * (1 - b) * np.cos(w) + (1 - b) ** 2)
        if h2 < target:
            lo = b
        else:
            hi = b
    return 0.5 * (lo + hi)


def guitar(midi, dur, vel=0.8, tau=1.6, bright=1.0, pick=0.13, mute=False, seed=0, r=0.07):
    """Karplus-Strong clean guitar string: a filtered pick burst (with a
    pick-position comb), a one-pole loop low-pass (bright: how long the
    fundamental survives the loop filter, ~seconds), a fast damp after
    note-off.  mute=True gives a palm-muted pluck."""
    g = np.random.default_rng(2100 + seed + int(midi) * 3)
    f = float(hz(midi))
    if mute:
        tau, bright = 0.18, 0.35
    b = _loop_coef(f, bright)
    period = SR / f - (1 - b) / b
    total = dur + 6 * r + 0.02
    n = int(total * SR)
    t = _t(n)
    pg = np.exp(-period / (tau * SR))
    pg_off = np.exp(-period / (r * SR))
    loss = np.where(t < dur, pg, pg_off)
    m = int(period)
    exc = g.standard_normal(m + 1)
    exc = butter(exc, 1200 + 4500 * vel, 'low', 1)
    k = max(1, int(pick * m))
    exc[k:] -= exc[:-k].copy()
    y = _ks(exc, n, period, loss, b)
    y = butter(y, 85, 'high', 2)
    y *= np.clip((total - t) / 0.02, 0, 1)
    y = y / (np.max(np.abs(y)) + 1e-9) * vel * 0.6
    return np.stack([y, y]).astype(np.float32)


def stab(midi, dur, vel=1.0, cut=500.0, env=2600.0, decay=0.07):
    """Short dark detuned-saw stab (the orchestra-hit replacement)."""
    n = int((dur + 0.4) * SR)
    t = _t(n)
    L = np.zeros(n)
    R = np.zeros(n)
    for o, p in ((-0.12, -0.6), (0.0, 0.0), (0.12, 0.6)):
        w = saw(hz(midi + o), n, (o + 0.3) % 1)
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    fc = cut + env * np.exp(-t / decay)
    x = svf(np.stack([L, R]), fc, 0.9)
    x = butter(x, 150, 'high', 2)
    return (x * env_adsr(n, dur, 0.002, 0.12, 0.35, 0.09) * vel * 0.45).astype(np.float32)


def swell(length, lp=2500.0, seed=0):
    """Dark reverse-cymbal-like swell (noise through a fixed low-pass)."""
    g = np.random.default_rng(3000 + seed)
    n = int(length * SR)
    u = np.linspace(0, 1, n)
    x = butter(butter(g.standard_normal((2, n)), lp, 'low', 2), 300, 'high', 1)
    return (x * u ** 2.2 * 0.5).astype(np.float32)


# ------------------------------------------------------------------ effects

def ensemble(x, mix=0.5, depth_ms=1.6, center_ms=8.0, rate=0.55, fast=6.0, fast_ms=0.12, chunk=1 << 20):
    """3-phase ensemble chorus (string-machine style): three modulated delays
    with LFOs 120 degrees apart, plus a little fast vibrato.  Processed in
    chunks to keep memory low.  float32 in / out."""
    n = x.shape[1]
    out = np.empty_like(x)
    pad = int(0.05 * SR)
    for s0 in range(0, n, chunk):
        s1 = min(n, s0 + chunk)
        a0 = max(0, s0 - pad)
        base = np.arange(s0, s1, dtype=np.float64)
        tt = base / SR
        for c in range(2):
            src = x[c, a0:s1].astype(np.float64)
            sb = np.arange(a0, s1, dtype=np.float64)
            wet = np.zeros(s1 - s0)
            for k in range(3):
                ph = 2 * np.pi * k / 3 + c * np.pi / 3
                d = (center_ms + depth_ms * np.sin(2 * np.pi * rate * tt + ph)
                     + fast_ms * np.sin(2 * np.pi * fast * tt + ph * 1.3)) * 1e-3 * SR
                wet += np.interp(base - d, sb, src)
            out[c, s0:s1] = ((1 - mix) * x[c, s0:s1] + mix * wet / 3).astype(np.float32)
    return out
