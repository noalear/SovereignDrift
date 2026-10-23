# synth_new09.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for new09_spacesynth.py: a late-70s / 90s "space synth" kit in
# the manner of Jean-Michel Jarre (Oxygene, Equinoxe) and Vangelis (Blade
# Runner), synthesized from oscillators and noise only (no samples):
#
#   string_voice   Eminent / ARP Solina string-machine note (divide-down saws,
#                  8' + 4'); ensemble() and phaser() are applied on the bus
#   moog_note      Mini-Moog-like sequencer bass note (saw + square, 24 dB
#                  low-pass with filter envelope)
#   mono_lead      monophonic portamento lead for a whole phrase (legato glide,
#                  delayed vibrato, filter envelope); 'odyssey' (Jarre's
#                  nasal lead) or 'theremin' (sine-like, wide vibrato)
#   cs80_brass     Yamaha CS-80 style brass (detuned saws, slow filter swell,
#                  pitch scoop, slow vibrato)
#   glass          soft glassy pluck for the high sequence
#   rhythm box     Korg Minipops-style soft kick, round snare ("pdlap"), soft
#                  hats, shaker, bongos
#   wind           band-passed noise whoosh (used sparingly)
# It imports the oscillators/filters of synth.py (which it does not modify).
import numpy as np
import numba
import synth as S
from synth import SR, hz, saw, svf, butter

_rng = np.random.default_rng(909)


# ------------------------------------------------------------- utilities

@numba.njit(cache=True)
def _glide(target, reset, coef):
    """One-pole portamento on a pitch curve (semitones); reset marks jumps."""
    n = target.shape[0]
    y = np.empty(n)
    p = target[0]
    for i in range(n):
        if reset[i]:
            p = target[i]
        else:
            p += coef * (target[i] - p)
        y[i] = p
    return y


@numba.njit(cache=True)
def _follow(gate, ca, cr):
    n = gate.shape[0]
    y = np.empty(n)
    e = 0.0
    for i in range(n):
        g = gate[i]
        if g > e:
            e += ca * (g - e)
        else:
            e += cr * (g - e)
        y[i] = e
    return y


@numba.njit(cache=True)
def _phaser(x, lfo_hz, fmin, fmax, stages, fb, phase):
    n = x.shape[0]
    y = np.empty(n)
    z = np.zeros(stages)
    last = 0.0
    lr = np.log(fmax / fmin)
    for i in range(n):
        u = 0.5 + 0.5 * np.sin(2 * np.pi * lfo_hz * i / SR + phase)
        f = fmin * np.exp(lr * u)
        t = np.tan(np.pi * f / SR)
        a = (t - 1.0) / (t + 1.0)
        s = x[i] + fb * last
        for k in range(stages):
            o = a * s + z[k]
            z[k] = s - a * o
            s = o
        last = s
        y[i] = s
    return y


def phaser(x, rate=0.11, fmin=180.0, fmax=2600.0, stages=6, fb=0.45, mix=0.5):
    """Small-Stone style stereo phaser on a (2, n) signal (L/R LFO offset)."""
    out = np.empty_like(x)
    for c in range(2):
        xc = np.ascontiguousarray(x[c], dtype=np.float64)
        wet = _phaser(xc, rate, fmin, fmax, stages, fb, c * np.pi / 2)
        out[c] = (1 - mix) * xc + mix * wet
    return out


def ensemble(mono, depth_ms=1.6, center_ms=7.5, chunk=1 << 20):
    """String-machine ensemble: three delay taps modulated by a slow (0.63 Hz)
    and a fast (6.1 Hz) LFO 120 degrees apart; mono in, (2, n) float32 out."""
    n = mono.shape[0]
    out = np.zeros((2, n), np.float32)
    x = np.concatenate([np.zeros(2048), mono.astype(np.float64)])
    wl = (0.55, 0.30, 0.15)
    wr = (0.15, 0.30, 0.55)
    for s0 in range(0, n, chunk):
        s1 = min(n, s0 + chunk)
        t = np.arange(s0, s1) / SR
        accL = np.zeros(s1 - s0)
        accR = np.zeros(s1 - s0)
        for k in range(3):
            ph = k * 2 * np.pi / 3
            d = (center_ms + depth_ms * np.sin(2 * np.pi * 0.63 * t + ph)
                 + 0.25 * np.sin(2 * np.pi * 6.1 * t + ph)) * SR / 1000.0
            pos = np.arange(s0, s1) + 2048 - d
            i0 = np.floor(pos).astype(np.int64)
            fr = pos - i0
            v = x[i0] * (1 - fr) + x[i0 + 1] * fr
            accL += v * wl[k]
            accR += v * wr[k]
        dry = mono[s0:s1]
        out[0, s0:s1] = (0.45 * dry + accL).astype(np.float32)
        out[1, s0:s1] = (0.45 * dry + accR).astype(np.float32)
    return out


def pulse(freq, n, pw=0.5, phase=0.0):
    return 0.5 * (saw(freq, n, phase) - saw(freq, n, (phase + pw) % 1.0))


# ----------------------------------------------------------- instruments

def string_voice(midi, dur, vel=1.0, a=0.55, r=1.3, bright=1.0):
    """Mono string-machine note (divide-down 8' + 4' saws, a hint of 16')."""
    n = int((dur + 5 * r) * SR)
    f = hz(midi)
    ph = _rng.random()
    y = saw(f, n, ph) + 0.55 * saw(2 * f, n, ph) + 0.18 * saw(f / 2, n, ph)
    t = np.arange(n) / SR
    y = svf(y, 5000.0 * bright, 0.6)
    y = butter(y, 220, 'high', 2)
    e = S.env_adsr(n, dur, a, 0.5, 1.0, r)
    return (y * e * vel * 0.25).astype(np.float32)


def moog_note(midi, dur, vel=1.0, cut=320.0, env=2600.0, decay=0.09, q=1.6, r=0.035, sub=0.35):
    """Mini-Moog-like sequencer note (mono): saw + square + sub, 24 dB ladder-ish LP."""
    n = int((dur + 6 * r) * SR)
    f = float(hz(midi))
    ph = _rng.random()
    w = 0.7 * saw(f, n, ph) + 0.45 * pulse(f * 1.002, n, 0.5, ph) + sub * pulse(f / 2, n, 0.5, ph)
    t = np.arange(n) / SR
    fc = cut + env * vel * np.exp(-t / decay) + 0.6 * f
    y = svf(w, fc, q)
    y = svf(y, fc * 1.15, 0.6)
    e = S.env_adsr(n, dur, 0.002, 0.25, 0.75, r)
    y = np.tanh(1.5 * y * e) * vel
    return y.astype(np.float32)


def mono_lead(notes, spb, kind='odyssey', glide=0.065, vib=0.16, cut=2600.0, env=2200.0,
              q=1.3, rel=0.25, legato_gap=0.06, tail=1.2):
    """A monophonic lead phrase.  notes: (beat, midi, beats) relative to the
    phrase start; consecutive notes closer than legato_gap seconds glide
    (portamento) without re-triggering.  Returns mono float32 audio."""
    notes = sorted(notes)
    end = max(b + d for b, _, d in notes) * spb
    n = int((end + tail) * SR)
    target = np.zeros(n)
    reset = np.zeros(n, np.bool_)
    gate = np.zeros(n)
    since = np.full(n, 10.0)       # seconds since the last (re)articulation
    held = np.zeros(n)             # seconds since this pitch began (for vibrato)
    prev_end = -1.0
    first_pitch = notes[0][1]
    target[:] = first_pitch
    for b, md, d in notes:
        i0 = int(b * spb * SR)
        i1 = min(n, int((b + d) * spb * SR))
        target[i0:] = md
        gate[i0:i1] = 1.0
        held[i0:] = np.arange(n - i0) / SR
        legato = prev_end >= 0 and (b * spb - prev_end) < legato_gap
        if not legato:
            reset[i0] = True
            since[i0:] = np.arange(n - i0) / SR
        prev_end = (b + d) * spb
    coef = 1.0 - np.exp(-1.0 / (glide * SR))
    pitch = _glide(target, reset, coef)
    vibd = np.clip((held - 0.32) / 0.45, 0, 1) * vib
    t = np.arange(n) / SR
    pitch = pitch + vibd * np.sin(2 * np.pi * 5.4 * t) + 0.03 * np.sin(2 * np.pi * 0.37 * t)
    f = hz(pitch)
    if kind == 'theremin':
        ph = 2 * np.pi * np.cumsum(f) / SR
        y = np.sin(ph) + 0.22 * np.sin(2 * ph + 0.4) + 0.07 * np.sin(3 * ph)
        y = np.tanh(1.3 * y)
        amp = _follow(gate, 1 - np.exp(-1 / (0.06 * SR)), 1 - np.exp(-1 / (rel * SR)))
        return (y * amp * 0.5).astype(np.float32)
    ph0 = _rng.random()
    y = 0.75 * saw(f, n, ph0) + 0.55 * pulse(f * 1.0035, n, 0.38, ph0) + 0.1 * saw(f * 0.5, n, ph0)
    fenv = np.exp(-since / 0.35)
    fc = cut + env * fenv + 0.9 * f
    y = svf(y, fc, q)
    y = svf(y, fc * 1.6, 0.55)
    amp = _follow(gate, 1 - np.exp(-1 / (0.008 * SR)), 1 - np.exp(-1 / (rel * SR)))
    y = np.tanh(1.2 * y) * amp
    return (y * 0.42).astype(np.float32)


def cs80_brass(midi, dur, vel=1.0, a=0.12, swell=0.35, r=0.55, cut=2300.0, width=0.6, scoop=0.35):
    """CS-80 style brass note, stereo (2 detuned saw pairs, slow filter swell)."""
    n = int((dur + 4 * r) * SR)
    t = np.arange(n) / SR
    bend = -scoop * np.exp(-t / 0.07)
    vibr = np.clip((t - 0.5) / 0.6, 0, 1) * 0.07 * np.sin(2 * np.pi * 4.8 * t)
    fenv = 1 - np.exp(-t / swell)
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / (r * 0.6)))
    fc = 350 + (cut * vel - 350) * fenv * (0.75 + 0.25 * gate)
    out = []
    for side, det in ((0, -0.07), (1, 0.07)):
        ff = hz(midi + det + bend + vibr)
        ph = _rng.random()
        w = saw(ff, n, ph) + 0.6 * saw(ff * 1.004, n, (ph + 0.3) % 1) + 0.35 * pulse(ff, n, 0.3, ph)
        y = svf(w, fc, 1.1)
        y = svf(y, fc * 1.4, 0.6)
        out.append(y)
    L, R = out
    mid = (L + R) / 2
    sd = (L - R) / 2 * width
    e = S.env_adsr(n, dur, a, 0.8, 0.85, r)
    y = np.stack([mid + sd, mid - sd]) * e * vel * 0.3
    y = butter(y, 120, 'high', 2)
    return y.astype(np.float32)


def glass(midi, dur, vel=1.0, decay=0.25):
    """Soft glassy pluck (square-ish, filter-decaying) for the high sequence."""
    n = int((dur + 0.6) * SR)
    t = np.arange(n) / SR
    f = float(hz(midi))
    w = pulse(f, n, 0.25, _rng.random()) + 0.4 * np.sin(2 * np.pi * 2 * f * t)
    fc = 600 + 5200 * np.exp(-t / 0.06)
    y = svf(w, fc, 1.2)
    e = np.exp(-t / decay) * np.minimum(t / 0.002, 1) * np.where(t < dur, 1, np.exp(-(t - dur) / 0.08))
    return (y * e * vel * 0.4).astype(np.float32)


# ---------------------------------------------------------- rhythm box

def kick(vel=1.0, f_end=49.0):
    """Soft, round rhythm-box kick (no click), tuned to G1 (the fifth of C)."""
    n = int(0.42 * SR)
    t = np.arange(n) / SR
    f = f_end + 70 * np.exp(-t / 0.028)
    y = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.16) * np.minimum(t / 0.0015, 1)
    amp *= np.clip((0.42 - t) / 0.05, 0, 1)
    y = np.tanh(1.3 * y * amp) / np.tanh(1.3)
    y += butter(_rng.standard_normal(n), 900, 'low', 2) * np.exp(-t / 0.004) * 0.12
    return (y * vel).astype(np.float32)


def snare(variant=0, vel=1.0):
    """Round, short "pdlap": tuned body with a fast pitch drop, a band-limited
    snap, a tiny clap layer; ~110 ms; little energy above 8 kHz.
    variant 0-3 = round-robin (slight pitch/tone/level differences)."""
    rr = np.random.default_rng(4400 + variant)
    n = int(0.2 * SR)
    t = np.arange(n) / SR
    tone = (196.0, 203.0, 190.0, 208.0)[variant % 4]
    f = tone * (1 + 0.45 * np.exp(-t / 0.009))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) + 0.35 * np.sin(1.58 * ph)) * np.exp(-t / 0.034)
    nz = rr.standard_normal(n)
    snap = butter(nz, [1300, 4200], 'band', 2) * np.exp(-t / (0.02 + 0.004 * variant))
    clap = np.zeros(n)
    for k, off in enumerate((0.0, 0.007, 0.0145)):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.004), 0)
    clap = butter(nz * clap, [900, 3500], 'band', 2) * 0.35
    y = 0.9 * body + (0.8 + 0.06 * variant) * snap + clap
    y = butter(y, 7000, 'low', 2) * np.minimum(t / 0.0007, 1)
    room = butter(rr.standard_normal(n), [500, 3000], 'band', 2) * np.exp(-t / 0.03) * 0.06
    y = y + room
    y *= np.clip((0.2 - t) / 0.03, 0, 1)
    return (y * vel * 0.8).astype(np.float32)


def hat(vel=1.0, variant=0):
    rr = np.random.default_rng(5500 + variant)
    n = int(0.08 * SR)
    t = np.arange(n) / SR
    y = butter(rr.standard_normal(n), [6500, 11000], 'band', 2) * np.exp(-t / 0.016)
    return (y * vel * 0.6).astype(np.float32)


def shaker(vel=1.0, variant=0):
    rr = np.random.default_rng(6600 + variant)
    n = int(0.09 * SR)
    t = np.arange(n) / SR
    e = np.minimum(t / 0.012, 1) * np.exp(-np.maximum(t - 0.012, 0) / 0.022)
    y = butter(rr.standard_normal(n), [3500, 8500], 'band', 2) * e
    return (y * vel * 0.5).astype(np.float32)


def bongo(midi, vel=1.0):
    n = int(0.3 * SR)
    t = np.arange(n) / SR
    f = hz(midi) * (1 + 0.25 * np.exp(-t / 0.012))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.07) * np.minimum(t / 0.001, 1)
    y += butter(_rng.standard_normal(n), [1500, 4000], 'band', 2) * np.exp(-t / 0.004) * 0.15
    return (y * vel * 0.8).astype(np.float32)


def tom(midi, vel=1.0):
    n = int(0.45 * SR)
    t = np.arange(n) / SR
    f = hz(midi) * (1 + 0.35 * np.exp(-t / 0.02))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.13) * np.minimum(t / 0.001, 1)
    y = np.tanh(1.2 * y)
    return (y * vel * 0.8).astype(np.float32)


def wind(length, f0=350.0, f1=1400.0, q=1.6, seed=0):
    """A soft stereo wind / whoosh: noise through a slowly swept band-pass,
    swelling in and out (rises to f1 at 60 % of the length, then falls)."""
    rr = np.random.default_rng(7700 + seed)
    n = int(length * SR)
    u = np.linspace(0, 1, n)
    shape = np.where(u < 0.6, (u / 0.6), 1 - (u - 0.6) / 0.4)
    fc = f0 * (f1 / f0) ** np.clip(shape, 0, 1)
    out = []
    for c in range(2):
        x = rr.standard_normal(n)
        y = svf(x, fc * (1 + 0.08 * c), q, 'bp')
        out.append(y)
    amp = np.sin(np.pi * u) ** 1.5
    return (np.stack(out) * amp * 0.5).astype(np.float32)
