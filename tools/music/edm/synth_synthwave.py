# synth_synthwave.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# 1980s analog-style instruments for the synthwave remix (song2_synthwave.py),
# built on the shared toolkit synth.py (which is not modified here).  All
# synthesized; no samples.
#
#   brass       Jupiter/OB-style detuned-saw brass: 4-pole filter with an
#               attack-then-decay envelope, pitch scoop, slow delayed vibrato
#   juno_pad    two saws + pulse, slow filter opening (chorus is added on the bus)
#   solina      string-machine ensemble strings
#   vox         formant-filtered saw choir ("aah")
#   solo        soft pulse lead (for the flute / oboe motifs)
#   bass8       Juno-style saw + sub-square octave bass with a filter pluck
#   fretless    round sine/saw bass with a fretless "mwah" scoop
#   arp_note    resonant pulse pluck for the arpeggiator
#   linn_kick, gated_snare, march_snare, linn_tom: LinnDrum-style kit with
#               a convolution gated-reverb tail on snare and toms
#   tape        wow/flutter (modulated delay) + soft asymmetric saturation
import numpy as np
import scipy.signal as ss

from synth import SR, hz, saw, sine, svf, butter, env_adsr, rng


def pulse(freq, n, pw=0.5, phase=0.0):
    """Band-limited pulse: difference of two polyBLEP saws (DC removed later by HP)."""
    return 0.5 * (saw(freq, n, phase) - saw(freq, n, (phase + pw) % 1.0))


def _time(n):
    return np.arange(n) / SR


# --------------------------------------------------------------- brass lead

def brass(midi, dur, vel=1.0, voices=6, detune=0.09, width=0.8,
          a=0.03, d=0.6, s=0.82, r=0.22,
          cut_lo=450.0, cut_hi=5200.0, cut_sus=2300.0, f_att=0.08, f_dec=0.45,
          q=0.9, vib=0.13, vib_rate=4.7, vib_onset=0.5, scoop=0.35, drive=1.4,
          keytrack=0.45):
    n = int((dur + 5 * r) * SR) + 1
    t = _time(n)
    pm = midi - scoop * np.exp(-t / 0.04)
    if vib:
        amt = np.clip((t - vib_onset) / 0.7, 0, 1) * vib
        pm = pm + amt * np.sin(2 * np.pi * vib_rate * t + rng().random() * 6.28)
    offs = np.linspace(-1, 1, voices)
    L = np.zeros(n)
    R = np.zeros(n)
    for o in offs:
        w = saw(hz(pm + o * detune), n, rng().random())
        p = o * width
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    kt = (float(hz(midi)) / 440.0) ** keytrack
    fc = np.where(t < f_att, cut_lo + (cut_hi - cut_lo) * (t / f_att),
                  cut_sus + (cut_hi - cut_sus) * np.exp(-np.maximum(t - f_att, 0) / f_dec))
    fc = fc * np.where(t > dur, np.exp(-(t - dur) / (2.5 * r)), 1.0)
    fc = fc * kt * (0.65 + 0.35 * vel)
    x = np.stack([L, R]) / np.sqrt(voices)
    x = svf(x, fc, 0.54)
    x = svf(x, fc, q)
    x = butter(x, max(30.0, float(hz(midi)) * 0.5), 'high', 1)
    e = env_adsr(n, dur, a, d, s, r)
    y = np.tanh(drive * x * e) / np.tanh(drive)
    return y * vel


def brass_stab(midi, dur, vel=1.0):
    """Short muted brass for the horn ostinato."""
    return brass(midi, dur, vel, voices=3, detune=0.06, width=0.5, a=0.004, d=0.12, s=0.45, r=0.06,
                 cut_lo=1800, cut_hi=3400, cut_sus=900, f_att=0.004, f_dec=0.09, q=1.1,
                 vib=0.0, scoop=0.0, drive=1.2, keytrack=0.3)


# --------------------------------------------------------------- pads

def juno_pad(midi, dur, vel=1.0, a=0.6, r=1.0, cut=1500.0, cut_open=1400.0, open_t=1.5,
             q=0.75, detune=0.09, pw=0.42):
    n = int((dur + 5 * r) * SR) + 1
    t = _time(n)
    drift = 0.03 * np.sin(2 * np.pi * (0.21 + 0.1 * rng().random()) * t + rng().random() * 6.28)
    f = hz(midi + drift)
    o1 = saw(f * 2 ** (detune / 12), n, rng().random())
    o2 = saw(f * 2 ** (-detune / 12), n, rng().random())
    o3 = pulse(f, n, pw, rng().random())
    L = 0.75 * o1 + 0.5 * o3 + 0.25 * o2
    R = 0.75 * o2 + 0.5 * o3 + 0.25 * o1
    fc = (cut + cut_open * (1 - np.exp(-t / open_t))) * (float(hz(midi)) / 330.0) ** 0.25
    x = svf(np.stack([L, R]), fc, 0.55)
    x = svf(x, fc, q)
    x = butter(x, 90, 'high', 2)
    return x * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 0.5


def solina(midi, dur, vel=1.0, a=0.45, r=1.1, cut=4200.0):
    n = int((dur + 5 * r) * SR) + 1
    t = _time(n)
    L = np.zeros(n)
    R = np.zeros(n)
    for o, p in ((-0.13, -0.6), (0.0, 0.0), (0.12, 0.6), (12.04, 0.2)):
        g = 0.35 if o > 6 else 1.0
        vib = 0.05 * np.sin(2 * np.pi * (5.6 + rng().random()) * t + rng().random() * 6)
        w = saw(hz(midi + o + vib), n, rng().random()) * g
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = svf(np.stack([L, R]), cut, 0.6)
    x = butter(x, 250, 'high', 2)
    return x * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 0.35


_FORMANTS = ((650, 1.0, 6.0), (1050, 0.45, 7.0), (2450, 0.22, 9.0), (3300, 0.1, 10.0))


def vox(midi, dur, vel=1.0, a=0.35, r=0.9, voices=5):
    n = int((dur + 5 * r) * SR) + 1
    t = _time(n)
    L = np.zeros(n)
    R = np.zeros(n)
    for i, o in enumerate(np.linspace(-0.18, 0.18, voices)):
        vb = np.clip((t - 0.3) / 0.6, 0, 1) * 0.09 * np.sin(2 * np.pi * (5.1 + 0.3 * i) * t + i)
        w = saw(hz(midi + o + vb), n, rng().random())
        p = o / 0.18 * 0.7
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = np.stack([L, R]) / np.sqrt(voices)
    y = np.zeros_like(x)
    for f, g, q in _FORMANTS:
        y += svf(x, f, q, 'bp') * g
    y += 0.15 * svf(x, 900, 0.7)
    return y * env_adsr(n, dur, a, 1.0, 1.0, r) * vel * 1.4


def solo(midi, dur, vel=1.0, a=0.035, r=0.28, cut=2100.0, vib=0.16):
    """Soft PWM-square lead with breath; for the flute and oboe motifs."""
    n = int((dur + 5 * r) * SR) + 1
    t = _time(n)
    pm = midi - 0.2 * np.exp(-t / 0.03)
    pm = pm + np.clip((t - 0.28) / 0.5, 0, 1) * vib * np.sin(2 * np.pi * 5.4 * t)
    f = hz(pm)
    y = pulse(f, n, 0.5, 0.0) * 0.55 + 0.75 * sine(f, n) + 0.12 * sine(2 * f, n)
    fc = cut * (1 + 0.8 * np.exp(-t / 0.15))
    y = svf(svf(y, fc, 0.6), fc, 0.7)
    br = butter(rng().standard_normal(n), [1500, 4500], 'band', 1) * 0.05
    y = (y + br) * env_adsr(n, dur, a, 0.4, 0.85, r)
    return np.stack([y, y]) * vel * 0.7


# --------------------------------------------------------------- bass

def bass8(midi, dur, vel=1.0, cut=220.0, env=1500.0, decay=0.075, q=1.05):
    n = int((dur + 0.12) * SR)
    t = _time(n)
    f = float(hz(midi))
    w = 0.65 * saw(f * 2 ** (0.05 / 12), n, 0.1) + 0.35 * saw(f * 2 ** (-0.05 / 12), n, 0.6)
    w += 0.45 * pulse(f / 2, n, 0.5, 0.3)
    fc = cut + env * np.exp(-t / decay)
    y = svf(svf(w, fc, 0.6), fc, q)
    y = y + 0.55 * sine(f, n) * (1 if f < 90 else 0.6)
    y = butter(y, 32, 'high', 2)
    y = np.tanh(1.7 * y * env_adsr(n, dur, 0.002, 0.18, 0.7, 0.025)) * vel
    return np.stack([y, y])


def fretless(midi, dur, vel=1.0):
    n = int((dur + 0.2) * SR)
    t = _time(n)
    f = hz(midi - 0.45 * np.exp(-t / 0.05))
    y = sine(f, n) + 0.3 * saw(f, n)
    y = svf(y, 420 + 500 * np.exp(-t / 0.12), 0.8)
    y = np.tanh(1.3 * y * env_adsr(n, dur, 0.01, 0.3, 0.75, 0.06)) * vel
    return np.stack([y, y])


# --------------------------------------------------------------- arp

def arp_note(midi, dur, vel=1.0, decay=0.11, cut=500.0, env=4200.0, q=2.2, pw=0.32):
    n = int((dur + 0.15) * SR)
    t = _time(n)
    f = float(hz(midi))
    y = pulse(f, n, pw, 0.0) + 0.35 * saw(f * 1.003, n, 0.4)
    fc = cut + env * np.exp(-t / decay)
    y = svf(y, fc, q)
    y = butter(y, 120, 'high', 1)
    y = y * env_adsr(n, dur, 0.001, 0.2, 0.35, 0.05)
    return np.stack([y, y]) * vel * 0.8


# --------------------------------------------------------------- drums

def linn_kick(vel=1.0, f_end=51.9):
    n = int(0.48 * SR)
    t = _time(n)
    f = f_end + (150.0 - f_end) * np.exp(-t / 0.035) + 650 * np.exp(-t / 0.0022)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.24) * np.minimum(t / 0.0006, 1)
    amp *= np.clip((0.48 - t) / 0.08, 0, 1)
    knock = butter(rng().standard_normal(n), [1200, 5000], 'band', 1) * np.exp(-t / 0.004) * 0.28
    y = np.tanh(1.9 * body * amp) * 0.95 + knock
    return np.stack([y, y]) * vel


def _gate_ir(hold, release=0.035, decay=0.9, lp=7000.0, hp=300.0, seed=0):
    g = np.random.default_rng(seed)
    L = int((hold + release) * SR)
    tt = _time(L)
    e = np.exp(-tt / decay)
    e *= np.where(tt < hold, 1.0, 0.5 * (1 + np.cos(np.pi * np.clip((tt - hold) / release, 0, 1))))
    e *= np.minimum(tt / 0.004, 1)
    ir = g.standard_normal((2, L)) * e
    ir = butter(butter(ir, lp, 'low', 2), hp, 'high', 2)
    return ir / np.sqrt(np.sum(ir[0] ** 2))


def _gated(dry, hold, wet_db, seed):
    ir = _gate_ir(hold, seed=seed)
    wet = np.stack([ss.fftconvolve(dry, ir[c]) for c in range(2)])
    wet *= 10 ** (wet_db / 20) * np.max(np.abs(dry)) / (np.max(np.abs(wet)) + 1e-9)
    out = np.zeros_like(wet)
    out[:, :dry.shape[0]] += dry
    out += wet
    k = out.shape[1]
    out[:, -int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    return out[:, :k]


def gated_snare(seed=0, hold=0.26, wet_db=-3.0, tone=190.0):
    g = np.random.default_rng(100 + seed)
    n = int(0.28 * SR)
    t = _time(n)
    body = (0.6 * np.sin(2 * np.pi * tone * t * (1 + 0.25 * np.exp(-t / 0.008))) * np.exp(-t / 0.055)
            + 0.3 * np.sin(2 * np.pi * tone * 1.72 * t) * np.exp(-t / 0.035))
    nz = butter(butter(g.standard_normal(n), 900, 'high', 2), 9500, 'low', 2) * np.exp(-t / 0.10)
    crack = butter(g.standard_normal(n), 2500, 'high', 2) * np.exp(-t / 0.006) * 0.6
    dry = (body + 0.95 * nz + crack) * np.minimum(t / 0.0004, 1)
    return _gated(dry, hold, wet_db, seed)


def march_snare(seed=0, tone=245.0):
    """Tight, dry field-snare for the march figures from the original drum part."""
    g = np.random.default_rng(200 + seed)
    n = int(0.22 * SR)
    t = _time(n)
    body = 0.45 * np.sin(2 * np.pi * tone * t * (1 + 0.2 * np.exp(-t / 0.006))) * np.exp(-t / 0.03)
    nz = butter(butter(g.standard_normal(n), 1800, 'high', 2), 11000, 'low', 1) * np.exp(-t / 0.07)
    y = (body + 0.85 * nz) * np.minimum(t / 0.0004, 1)
    return np.stack([y, y])


def linn_tom(midi, seed=0, gated=True):
    n = int(0.5 * SR)
    t = _time(n)
    f = hz(midi) * (1 + 0.5 * np.exp(-t / 0.035))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.2)
    y += butter(np.random.default_rng(300 + seed).standard_normal(n), 2500, 'low', 2) * np.exp(-t / 0.015) * 0.35
    y = np.tanh(1.6 * y) * np.minimum(t / 0.0005, 1)
    if not gated:
        return np.stack([y, y])
    return _gated(y, 0.22, -5.0, 50 + seed)


# --------------------------------------------------------------- tape

def tape(x, wow_ms=0.45, wow_hz=0.55, flutter_ms=0.035, flutter_hz=6.3, drive=1.25, asym=0.06):
    """Modulated-delay wow/flutter plus soft asymmetric saturation (float32 in/out)."""
    n = x.shape[1]
    t = np.arange(n, dtype=np.float64) / SR
    d = (wow_ms * (np.sin(2 * np.pi * wow_hz * t) + 0.45 * np.sin(2 * np.pi * 0.23 * wow_hz * 4.1 * t + 1.3))
         + flutter_ms * np.sin(2 * np.pi * flutter_hz * t)) * 1e-3 * SR
    d -= d.min() - 2.0
    idx = np.arange(n, dtype=np.float64) - d
    del t, d
    base = np.arange(n, dtype=np.float64)
    out = np.empty_like(x)
    for c in range(x.shape[0]):
        out[c] = np.interp(idx, base, x[c].astype(np.float64)).astype(np.float32)
    del idx, base
    y = out + np.float32(asym) * out * out
    y = np.tanh(np.float32(drive) * y) / np.float32(np.tanh(drive))
    return y.astype(np.float32)


def chorus(x, rate=0.5, depth_ms=1.8, center_ms=7.0, mix=0.5, spread=np.pi / 2):
    """Juno-style stereo chorus: one modulated delay per channel, LFOs offset by `spread`."""
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


# --------------------------------------------------------------- v2 snare (2026-10-14)
# After the user's listening: a rounded, tonal "pdlap" instead of a gated noise burst.
# Tuned body (Ab3 by default, the fifth of C#) with a fast pitch drop, a short snap
# band-limited to 1-4 kHz, a little wire, and a short, dark gated room. Almost
# nothing above 8 kHz.  `seed` picks a round-robin variant (pitch/tone/level).

def _dark_room(dry, hold=0.11, release=0.05, decay=0.16, lp=3800.0, wet_db=-11.0, seed=0):
    g = np.random.default_rng(seed)
    L = int((hold + release) * SR)
    tt = _time(L)
    e = np.exp(-tt / decay) * np.minimum(tt / 0.006, 1)
    e *= np.where(tt < hold, 1.0, 0.5 * (1 + np.cos(np.pi * np.clip((tt - hold) / release, 0, 1))))
    ir = g.standard_normal((2, L)) * e
    ir = butter(butter(ir, lp, 'low', 2), 280, 'high', 2)
    ir /= np.sqrt(np.sum(ir[0] ** 2))
    wet = np.stack([ss.fftconvolve(dry, ir[c]) for c in range(2)])
    wet *= 10 ** (wet_db / 20) * np.sqrt(np.sum(dry ** 2)) / (np.sqrt(np.sum(wet[0] ** 2)) + 1e-9)
    out = wet.copy()
    out[:, :dry.shape[0]] += dry
    k = int(0.01 * SR)
    out[:, -k:] *= np.linspace(1, 0, k)
    return out


def snare2(seed=0, vel=1.0, tone=207.7, room=True, clap=0.0):
    g = np.random.default_rng(500 + seed)
    pt = 1.0 + (g.random() - 0.5) * 0.035          # +-1.75 % pitch per variant
    lvl = 1.0 + (g.random() - 0.5) * 0.18           # +-0.8 dB
    snap_fc = 2000.0 * (1 + (g.random() - 0.5) * 0.25)
    n = int(0.24 * SR)
    t = _time(n)
    f0 = tone * pt
    f = f0 * (1 + 0.45 * np.exp(-t / 0.010))
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.085)
    body += 0.35 * np.sin(2 * np.pi * np.cumsum(f * 1.59) / SR) * np.exp(-t / 0.045)
    body = np.tanh(1.4 * body) / np.tanh(1.4)
    nz = g.standard_normal(n)
    v = float(np.clip(vel, 0.2, 1.2))
    snap = butter(nz, [snap_fc * 0.5, min(snap_fc * 2.0, 4200.0)], 'band', 2) * np.exp(-t / 0.018)
    wire = butter(butter(nz, 1500, 'high', 2), 6500, 'low', 2) * np.exp(-t / 0.055)
    y = (0.85 * body + (0.9 * v ** 0.6) * snap * 2.2 + 0.32 * v * wire * 1.6)
    y += 0.9 * butter(nz, [420, 1150], 'band', 2) * np.exp(-t / 0.028)    # woody shell
    if clap:
        e = np.zeros(n)
        for k, off in enumerate((0.0, 0.008, 0.017)):
            u = t - off
            e += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.004), 0)
        y += clap * butter(g.standard_normal(n), [1100, 3400], 'band', 2) * e * 2.0
    y *= np.minimum(t / 0.0005, 1) * np.clip((0.24 - t) / 0.03, 0, 1)
    y = butter(y, 7500, 'low', 2) * lvl
    if not room:
        return np.stack([y, y])
    return _dark_room(y, seed=seed + 7)
