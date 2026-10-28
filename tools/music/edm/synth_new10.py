# synth_new10.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments and effects for new track 10, the ambient-techno / IDM piece
# (tools/music/edm/new10_ambient_techno.py).  Builds on synth.py, which is not
# modified here.  Everything is synthesized: no samples, no SoundFonts.
#
#   pads       analog_pad: a few detuned saws with slow per-voice drift and a
#              shared tape wow (Juno/OB style), soft low-pass; string_machine:
#              brighter ensemble saws with an octave layer (Solina-ish)
#   melodic    warm_lead (mono legato lead with glide, PWM-ish double saw and
#              delayed vibrato), glass (soft FM bell), marimba (modal mallet),
#              softpluck (rounded Juno pluck for the filtered sequence)
#   bass       warm_bass: sine sub + low-passed saw with a short filter envelope
#   drums      round_kick (tuned, no click), snap (short rounded tonal snare /
#              clap, 4 round-robins: "pdlap", not "tschh"), soft_hat, shaker,
#              conga, rim
#   effects    hall impulse response + convolution, LFO filter, wind bed
import numpy as np
import scipy.signal as ss

import synth as S
from synth import SR, hz

try:
    import numba

    @numba.njit(cache=True)
    def _env_follow(gate, amp, a_c, r_c):
        s = 0.0
        for i in range(gate.shape[0]):
            c = a_c if gate[i] > s else r_c
            s = c * s + (1 - c) * gate[i]
            amp[i] = s
except ImportError:  # pragma: no cover
    def _env_follow(gate, amp, a_c, r_c):
        s = 0.0
        for i in range(gate.shape[0]):
            c = a_c if gate[i] > s else r_c
            s = c * s + (1 - c) * gate[i]
            amp[i] = s

_r = np.random.default_rng(1010)


def _drift(n, depth, rates=(0.07, 0.17, 0.41)):
    """Slow pseudo-random pitch drift in semitones (sum of slow sines, random phases)."""
    t = np.arange(n) / SR
    y = np.zeros(n)
    for k, rt in enumerate(rates):
        y += np.sin(2 * np.pi * rt * (0.8 + 0.4 * _r.random()) * t + _r.random() * 6.283) / (k + 1)
    return y * depth / 1.6


def wow(n, t0=0.0, depth=0.05, rate=0.55):
    """Shared tape wow (semitones), phase-locked to absolute time t0 so voices move together."""
    t = t0 + np.arange(n) / SR
    return depth * (np.sin(2 * np.pi * rate * t) + 0.45 * np.sin(2 * np.pi * 0.21 * t + 1.3))


# ------------------------------------------------------------------ pads

def analog_pad(midi, dur, vel=1.0, t0=0.0, attack=1.2, release=1.6, cut=1500.0, voices=3,
               detune=0.09, q=0.75, octave=0.0, bright_env=0.6, width=0.8):
    """Warm detuned-saw pad voice with drift and wow; returns (2, n) float32."""
    n = int((dur + 4.6 * release / 1.5) * SR) + 1
    t = np.arange(n) / SR
    w = wow(n, t0)
    L = np.zeros(n)
    R = np.zeros(n)
    offs = np.linspace(-1, 1, voices) if voices > 1 else np.zeros(1)
    for k, o in enumerate(offs):
        f = hz(midi + o * detune + w + _drift(n, 0.04))
        x = S.saw(f, n, _r.random())
        if octave:
            x = x + octave * S.saw(f * 0.5, n, _r.random())
        p = o * width
        L += x * np.sqrt((1 - p) / 2)
        R += x * np.sqrt((1 + p) / 2)
    # the filter opens a little with the attack (analog pad swell)
    fc = cut * (1 + bright_env * np.minimum(t / max(attack, 1e-3), 1.0) ** 1.5) / (1 + bright_env)
    fc = fc * (hz(midi) / 262.0) ** 0.25
    y = S.svf(np.stack([L, R]), fc, q)
    y = S.butter(y, max(30.0, hz(midi) * 0.45), 'high', 1)
    e = S.env_adsr(n, dur, attack, 1.0, 1.0, release / 4.6 * 1.5)
    return (y * e * vel / np.sqrt(voices) * 0.6).astype(np.float32)


def string_machine(midi, dur, vel=1.0, t0=0.0, attack=0.35, release=0.9, cut=2600.0):
    """Ensemble strings (Solina-like): bright saws + octave, slow attack.  Chorus on the bus."""
    a = analog_pad(midi, dur, vel, t0, attack=attack, release=release, cut=cut, voices=3,
                   detune=0.12, q=0.6, bright_env=0.3, width=1.0)
    b = analog_pad(midi + 12, dur, vel * 0.35, t0, attack=attack * 1.3, release=release, cut=cut * 1.3,
                   voices=2, detune=0.1, q=0.6, bright_env=0.3, width=1.0)
    k = min(a.shape[1], b.shape[1])
    return a[:, :k] + b[:, :k]


# --------------------------------------------------------------- melodic

def warm_lead(notes, vel=1.0, glide=0.06, cut=1500.0, cut_env=900.0, env_decay=0.4, q=0.85,
              vib_depth=0.12, vib_rate=4.8, attack=0.05, release=0.45, t_abs=0.0):
    """Mono legato lead.  notes: [(t_sec, midi, dur_sec)] relative to the first note.
    Touching notes glide; gaps re-articulate.  Two saws (one inverted, offset: a soft
    pulse), a sine an octave down and tape wow.  Returns (2, n) float32."""
    t_end = max(t + d for t, _, d in notes)
    n = int((t_end + 5 * release) * SR) + 1
    t = np.arange(n) / SR
    pitch = np.full(n, float(notes[0][1]))
    gate = np.zeros(n)
    since = np.full(n, 10.0)
    prev_end = None
    for t0, md, d in notes:
        i0, i1 = int(t0 * SR), int((t0 + d) * SR)
        legato = prev_end is not None and abs(prev_end - t0) < 0.03
        if legato:
            g = int(glide * SR)
            p0 = pitch[i0 - 1]
            u = np.minimum(np.arange(i1 - i0) / max(g, 1), 1)
            pitch[i0:i1] = p0 + (md - p0) * (0.5 - 0.5 * np.cos(np.pi * u))
        else:
            pitch[i0:i1] = md
        pitch[i1:] = md
        gate[i0:i1] = 1.0
        since[i0:] = t[i0:] - t0
        prev_end = t0 + d
    a_c = np.exp(-1 / (attack * SR))
    r_c = np.exp(-1 / (release * SR / 4.6))
    amp = np.empty(n)
    _env_follow(gate, amp, a_c, r_c)
    vib = vib_depth * np.clip((since - 0.4) / 0.6, 0, 1) * np.sin(2 * np.pi * vib_rate * t)
    vib += wow(n, t_abs, 0.04)
    f = hz(pitch + vib)
    ph = _r.random()
    w1 = S.saw(f, n, ph)
    w2 = S.saw(f * 2 ** (0.06 / 12), n, (ph + 0.37 + 0.1 * np.sin(2 * np.pi * 0.3 * t)) % 1.0)
    x = 0.55 * w1 - 0.45 * w2 + 0.3 * S.sine(f * 0.5, n)
    fc = cut * (f / 520.0) ** 0.35 + cut_env * np.exp(-since / env_decay)
    y = S.svf(x, fc, q)
    y = y * amp * vel * 0.5
    d = int(0.013 * SR)
    R = np.concatenate([np.zeros(d), y[:-d]])
    return np.stack([y, 0.8 * y + 0.2 * R]).astype(np.float32)


def glass(midi, dur, vel=1.0, decay=1.1, bright=1.0, pan=0.0):
    """Soft glassy FM bell: gentle transient, long sine tail."""
    f = float(hz(midi))
    n = int((dur + 4 * decay) * SR)
    t = np.arange(n) / SR
    idx = bright * (1.4 * np.exp(-t / 0.07) + 0.22 * np.exp(-t / 0.5))
    y = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * 2.0 * f * t))
    y += 0.2 * np.sin(2 * np.pi * 3.0 * f * t) * np.exp(-t / (0.22 * decay))
    y += 0.08 * bright * np.sin(2 * np.pi * 5.4 * f * t) * np.exp(-t / 0.04)
    e = np.exp(-t / decay) * np.minimum(t / 0.0015, 1)
    e *= np.where(t < dur, 1.0, np.exp(-(t - dur) / (0.6 * decay)))
    k = int(0.01 * SR)
    e[-k:] *= np.linspace(1, 0, k)
    y = y * e * vel * 0.4
    return S.pan2(y, pan).astype(np.float32)


def marimba(midi, dur, vel=1.0, pan=0.0, bright=1.0):
    """Modal mallet tone: fundamental, the 3.9x and 9.2x bar modes, soft mallet."""
    f = float(hz(midi))
    tau = float(np.clip(0.45 * (440.0 / f) ** 0.6, 0.12, 0.9))
    n = int((min(dur, tau * 3) + tau * 4) * SR)
    t = np.arange(n) / SR
    y = np.sin(2 * np.pi * f * t) * np.exp(-t / tau)
    y += 0.22 * bright * np.sin(2 * np.pi * 3.93 * f * t) * np.exp(-t / (tau * 0.18))
    y += 0.05 * bright * np.sin(2 * np.pi * 9.2 * f * t) * np.exp(-t / 0.012)
    mallet = S.butter(_r.standard_normal(n), [300, 2500], 'band', 1) * np.exp(-t / 0.0025) * 0.12
    y = (y + mallet) * np.minimum(t / 0.001, 1)
    k = int(0.01 * SR)
    y[-k:] *= np.linspace(1, 0, k)
    return S.pan2(y * vel * 0.5, pan).astype(np.float32)


def softpluck(midi, dur, vel=1.0, decay=0.16, cut=900.0, env=2200.0, pan=0.0):
    """Rounded Juno pluck: two saws through a low-pass with a short envelope."""
    f = float(hz(midi))
    n = int((dur + 0.4) * SR)
    t = np.arange(n) / SR
    x = 0.6 * S.saw(f, n, _r.random()) + 0.4 * S.saw(f * 1.003, n, _r.random())
    fc = cut + env * np.exp(-t / decay)
    y = S.svf(x, fc, 0.9)
    e = np.minimum(t / 0.002, 1) * np.exp(-t / (decay * 2.2))
    e *= np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.05))
    k = int(0.005 * SR)
    e[-k:] *= np.linspace(1, 0, k)
    return S.pan2(y * e * vel * 0.5, pan).astype(np.float32)


# ------------------------------------------------------------------ bass

def warm_bass(midi, dur, vel=1.0, cut=240.0, env=520.0, decay=0.12, a=0.006, r=0.06, saw_amt=0.55):
    """Deep warm bass: sine fundamental + a softly filtered saw (round, no buzz)."""
    n = int((dur + 5 * r) * SR) + 1
    t = np.arange(n) / SR
    f = hz(midi)
    y = S.sine(f, n)
    y = np.tanh(1.3 * y) / np.tanh(1.3)
    sw = S.saw(f, n, 0.25) * 0.6 + S.saw(f * 1.004, n, 0.6) * 0.4
    sw = S.svf(sw, cut + env * np.exp(-t / decay), 0.8)
    y = y + saw_amt * sw
    e = S.env_adsr(n, dur, a, 0.4, 0.85, r)
    y = (y * e * vel * 0.8).astype(np.float32)
    return np.stack([y, y])


# ----------------------------------------------------------------- drums

def round_kick(vel=1.0, f_end=55.0, f_start=125.0, length=0.42):
    """Soft, round kick tuned to f_end (A1 = 55 Hz, the tonic); a little knock, no click."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_start - f_end) * np.exp(-t / 0.038) + 90 * np.exp(-t / 0.006)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.2) * np.minimum(t / 0.0015, 1)
    amp *= np.where(t > length - 0.07, np.clip((length - t) / 0.07, 0, 1), 1)
    y = np.tanh(1.8 * body * amp)
    knock = S.butter(_r.standard_normal(n), [200, 1200], 'band', 1) * np.exp(-t / 0.006) * 0.18
    y = S.butter(y + knock, 2500, 'low', 2)
    return np.stack([y, y]).astype(np.float32) * vel


_SNAP_RR = [(1.000, 1.00, 1.00), (0.985, 0.93, 0.95), (1.012, 1.06, 0.98), (0.995, 0.97, 0.92)]


def snap(vel=1.0, var=0, f0=207.65, dark=0.0):
    """Short, round, slightly woody snare/clap ("pdlap"): tuned body (G#3 ~ 208 Hz,
    the leading tone's octave; it reads as a woody knock) with a fast pitch drop,
    band-limited 1-3.5 kHz snap, a tiny three-hit clap flam, ~110 ms decay,
    low-passed at 6.5 kHz.  var: round-robin variant 0-3; dark: ghost version."""
    pm, tone, lvl = _SNAP_RR[var % 4]
    n = int(0.24 * SR)
    t = np.arange(n) / SR
    f = f0 * pm * (1 + 0.5 * np.exp(-t / 0.011))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.07) + 0.3 * np.sin(1.52 * ph) * np.exp(-t / 0.04) +
            0.12 * np.sin(2.3 * ph) * np.exp(-t / 0.02))
    snp = S.butter(_r.standard_normal(n), [1000 * tone, 3500 * tone], 'band', 2)
    snp *= (np.exp(-t / 0.014) + 0.2 * np.exp(-t / 0.05)) * (1 - 0.85 * dark)
    clap = np.zeros(n)
    for off in (0.0, 0.008, 0.017):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0035), 0)
    clap = S.butter(_r.standard_normal(n) * clap, [1100, 3000], 'band', 2) * 0.4 * (1 - 0.9 * dark)
    y = 1.0 * body + 0.6 * snp + 1.2 * clap
    y = np.tanh(1.3 * y) / 1.1 * np.minimum(t / 0.0005, 1)
    y = S.butter(y, 6500 - 4000 * dark, 'low', 2)
    fe = int(0.02 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 17)]).astype(np.float32) * vel * lvl


def soft_hat(vel=1.0, open_=False, pan=0.0):
    """Gentle hat: metallic + noise, band-limited 6.5-13 kHz, short."""
    length = 0.32 if open_ else 0.07
    n = int(length * SR)
    t = np.arange(n) / SR
    tau = 0.085 if open_ else 0.016
    src = 0.5 * S._metal(n) + 0.6 * _r.standard_normal(n)
    y = S.butter(src, [6500, 13000], 'band', 2) * np.exp(-t / tau) * np.minimum(t / 0.0008, 1)
    y *= np.where(t > length - 0.02, np.clip((length - t) / 0.02, 0, 1), 1)
    return S.pan2(y * vel * 0.8, pan).astype(np.float32)


def shaker(vel=1.0, pan=0.3):
    n = int(0.08 * SR)
    t = np.arange(n) / SR
    e = np.minimum(t / 0.01, 1) * np.exp(-np.maximum(t - 0.01, 0) / 0.018)
    y = S.butter(_r.standard_normal(n), [4500, 10000], 'band', 2) * e
    return S.pan2(y * vel * 0.7, pan).astype(np.float32)


def conga(midi, vel=1.0, pan=0.0, slap=0.3):
    """Soft hand drum: pitched membrane with a small pitch drop and a muted slap."""
    n = int(0.35 * SR)
    t = np.arange(n) / SR
    f = hz(midi) * (1 + 0.25 * np.exp(-t / 0.015))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.11)
    y += 0.25 * np.sin(2 * np.pi * np.cumsum(f * 1.59) / SR) * np.exp(-t / 0.04)
    y += S.butter(_r.standard_normal(n), [700, 2500], 'band', 1) * np.exp(-t / 0.006) * slap
    y = np.tanh(1.2 * y) * np.minimum(t / 0.0008, 1)
    k = int(0.01 * SR)
    y[-k:] *= np.linspace(1, 0, k)
    return S.pan2(y * vel * 0.6, pan).astype(np.float32)


def rim(vel=1.0, pan=-0.3):
    n = int(0.06 * SR)
    t = np.arange(n) / SR
    y = np.sin(2 * np.pi * 1500 * t) * np.exp(-t / 0.007) + np.sin(2 * np.pi * 720 * t) * np.exp(-t / 0.01) * 0.5
    y += S.butter(_r.standard_normal(n), 2000, 'high', 2) * np.exp(-t / 0.003) * 0.3
    y = S.butter(y, 6000, 'low', 2)
    return S.pan2(y * vel * 0.5, pan).astype(np.float32)


# --------------------------------------------------------------- effects

def hall_ir(t60=(4.8, 5.2, 3.6, 1.8), length=6.5, predelay=0.03, seed=10):
    """Stereo impulse response: decorrelated noise with band-dependent decay
    (<400, 400-2.5k, 2.5k-7k, >7k Hz) and a soft build-up."""
    r = np.random.default_rng(seed)
    n = int(length * SR)
    t = np.arange(n) / SR
    bands = [(None, 400), (400, 2500), (2500, 7000), (7000, None)]
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
    ir *= np.minimum(t / 0.07, 1) ** 1.5
    pd = int(predelay * SR)
    ir = np.concatenate([np.zeros((2, pd)), ir[:, : n - pd]], axis=1)
    ir /= np.sqrt(np.sum(ir ** 2) / 2)
    return ir.astype(np.float32)


def convolve(x, ir):
    out = np.empty((2, x.shape[1]), np.float32)
    for c in range(2):
        out[c] = ss.oaconvolve(x[c], ir[c], mode='full')[: x.shape[1]].astype(np.float32)
    return out


def lfo_filter(x, base, depth_oct=0.8, rate=0.06, q=0.7, phase=0.0, mode='lp'):
    """Slowly sweeping filter; base cutoff (Hz) scalar or per-sample array."""
    n = x.shape[1]
    t = np.arange(n) / SR
    fc = np.asarray(base) * 2 ** (depth_oct * np.sin(2 * np.pi * rate * t + phase))
    return S.svf(x, fc, q, mode).astype(np.float32)


def wind(n, f_lo=350.0, f_hi=2200.0, rate=0.031, q=1.6, seed=7):
    """Stereo filtered-noise 'solar wind' bed: band-pass centre drifting slowly."""
    r = np.random.default_rng(seed)
    t = np.arange(n) / SR
    out = []
    for c in range(2):
        x = r.standard_normal(n)
        u = 0.5 + 0.5 * np.sin(2 * np.pi * rate * t + c * 1.7 + 0.4 * np.sin(2 * np.pi * 0.011 * t))
        fc = f_lo * (f_hi / f_lo) ** u
        out.append(S.svf(x, fc, q, 'bp'))
    return (np.stack(out) * 0.25).astype(np.float32)
