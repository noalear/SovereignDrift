# synth_goa.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for new05_goa.py (a 1990s goa / psytrance track): a TB-303 style
# acid voice and a monophonic "twisting" goa lead (both on a zero-delay-feedback
# 4-pole ladder filter, numba), a short psytrance kick, the rolling off-kick
# bass note, a round woody snare (no white-noise tail), congas, and a soft
# cosmic "swirl" texture.  Everything is synthesized; it builds on synth.py.
import numpy as np
import numba

import synth as S
from synth import SR, hz


# ------------------------------------------------------------ ladder filter

@numba.njit(cache=True)
def _ladder(x, fc, k, drive):
    """ZDF 4-pole ladder (Zavalishin TPT one-poles, feedback solved linearly,
    tanh on the stage input).  fc and k (0..4, 4 = self-oscillation) per sample."""
    n = x.shape[0]
    y = np.empty(n)
    s1 = 0.0
    s2 = 0.0
    s3 = 0.0
    s4 = 0.0
    for i in range(n):
        f = fc[i]
        if f > 0.45 * SR:
            f = 0.45 * SR
        if f < 15.0:
            f = 15.0
        g = np.tan(np.pi * f / SR)
        G = g / (1.0 + g)
        a = 1.0 / (1.0 + g)
        S_ = G * G * G * s1 * a + G * G * s2 * a + G * s3 * a + s4 * a
        kk = k[i]
        u = (x[i] * drive - kk * S_) / (1.0 + kk * G * G * G * G)
        u = np.tanh(u)
        v = (u - s1) * G
        y1 = v + s1
        s1 = y1 + v
        v = (y1 - s2) * G
        y2 = v + s2
        s2 = y2 + v
        v = (y2 - s3) * G
        y3 = v + s3
        s3 = y3 + v
        v = (y3 - s4) * G
        y4 = v + s4
        s4 = y4 + v
        y[i] = y4 * (1.0 + 0.45 * kk) / drive
    return y


def ladder(x, fc, k, drive=1.0):
    n = x.shape[-1]
    f = np.broadcast_to(np.asarray(fc, float), (n,)).astype(float).copy()
    kk = np.broadcast_to(np.asarray(k, float), (n,)).astype(float).copy()
    if x.ndim == 1:
        return _ladder(np.ascontiguousarray(x, float), f, kk, float(drive))
    return np.stack([_ladder(np.ascontiguousarray(c, float), f, kk, float(drive)) for c in x])


@numba.njit(cache=True)
def _glide(target, tau):
    """One-pole glide of a per-sample pitch target with per-sample time constant."""
    n = target.shape[0]
    y = np.empty(n)
    p = target[0]
    for i in range(n):
        c = 1.0 - np.exp(-1.0 / (tau[i] * SR))
        p += (target[i] - p) * c
        y[i] = p
    return y


# ---------------------------------------------------------- mono synth core

def mono_events(events, n, decay=0.18, acc_decay=0.09, gate_frac=None, glide=0.045, jump=0.004):
    """events: list of (t0_s, t1_s, midi, accent 0..1, slide_in bool), time-sorted,
    all inside [0, n).  Returns per-sample pitch, amp env and filter env (0..1)."""
    tgt = np.full(n, np.nan)
    tau = np.full(n, jump)
    amp = np.zeros(n)
    fenv = np.zeros(n)
    acc = np.zeros(n)
    for j, (t0, t1, md, ac, slide) in enumerate(events):
        i0 = max(0, int(round(t0 * SR)))
        i1 = min(n, int(round(t1 * SR)))
        if i1 <= i0:
            continue
        L = i1 - i0
        tgt[i0:i1] = md
        if slide:
            tau[i0:i1] = glide
        u = np.arange(L) / SR
        a = np.minimum(u / 0.003, 1.0) if not slide else np.ones(L)
        rel = int(0.006 * SR)
        nxt_slide = j + 1 < len(events) and events[j + 1][4] and abs(events[j + 1][0] - t1) < 1e-3
        e = a * (1.0 + 0.35 * ac)
        if slide and i0 > 0:
            # legato: glide the level from the previous note's (accent) level
            r5 = min(L, int(0.008 * SR))
            e[:r5] = np.linspace(amp[i0 - 1], e[r5 - 1] if r5 < L else e[-1], r5)
        if not nxt_slide and L > rel:
            e[-rel:] *= np.linspace(1, 0, rel)
        amp[i0:i1] = np.maximum(amp[i0:i1], e)
        if not slide:
            d = acc_decay if ac > 0.5 else decay
            fe = np.exp(-u / d) * (1.0 + 0.6 * ac)
            fenv[i0:i1] = fe
            acc[i0:i1] = ac
        else:
            # a slide keeps the running filter envelope of the previous note
            prev = fenv[i0 - 1] if i0 > 0 else 0.0
            d = decay
            fenv[i0:i1] = prev * np.exp(-u / d)
            acc[i0:i1] = acc[i0 - 1] if i0 > 0 else 0.0
    # hold pitch through rests so the oscillator phase/pitch stay continuous
    valid = ~np.isnan(tgt)
    if not valid.any():
        return np.full(n, 60.0), amp, fenv, acc
    idx = np.where(valid, np.arange(n), 0)
    np.maximum.accumulate(idx, out=idx)
    first = int(np.argmax(valid))
    idx[:first] = first
    tgt = tgt[idx]
    pitch = _glide(tgt, tau)
    return pitch, amp, fenv, acc


def acid(events, n, cut, res, envmod=2.6, decay=0.2, drive=1.6, wave='saw'):
    """TB-303 style voice.  cut (Hz) and res (k 0..4) scalar or per-sample.
    Returns mono float64."""
    pitch, amp, fenv, acc = mono_events(events, n, decay=decay, acc_decay=0.085)
    f = hz(pitch)
    osc = S.saw(f, n)
    if wave == 'square':
        osc = 0.5 * (osc - S.saw(f, n, 0.5))
    cut = np.broadcast_to(np.asarray(cut, float), (n,))
    fc = np.minimum(cut * 2.0 ** ((envmod + 0.8 * acc) * fenv), 6000.0)
    k = np.broadcast_to(np.asarray(res, float), (n,)) + 0.4 * acc
    y = ladder(osc * 0.8, fc, k, drive)
    y = np.tanh(1.8 * y * amp) / 1.2
    y = S.butter(y, 70, 'high', 2)
    return y


def goa_lead(events, n, cut, res, envmod=2.2, decay=0.16, detune=0.12, lfo_rate=0.0, lfo_oct=0.0,
             vib=0.0, width=0.55, pulse=0.35):
    """Monophonic twisting goa lead: two detuned saws + a pulse, ladder LP with
    a per-note envelope and an optional slow cutoff LFO; stereo (2, n)."""
    pitch, amp, fenv, acc = mono_events(events, n, decay=decay, acc_decay=0.1, glide=0.05)
    t = np.arange(n) / SR
    if vib:
        pitch = pitch + vib * np.sin(2 * np.pi * 5.2 * t)
    cut = np.broadcast_to(np.asarray(cut, float), (n,))
    lfo = lfo_oct * np.sin(2 * np.pi * lfo_rate * t) if lfo_rate else 0.0
    fc = np.minimum(cut * 2.0 ** (envmod * fenv + lfo), 8000.0)
    k = np.broadcast_to(np.asarray(res, float), (n,))
    out = []
    for side, dt in ((0, -detune), (1, detune)):
        f = hz(pitch + dt)
        w = S.saw(f, n, S.rng().random())
        if pulse:
            fp = hz(pitch - dt * 0.5)
            ph = S.rng().random()
            w = w * (1 - pulse) + pulse * 0.5 * (S.saw(fp, n, ph) - S.saw(fp, n, (ph + 0.5) % 1.0))
        out.append(ladder(w * 0.7, fc, k, 1.3))
    L, R = out
    c = (L + R) / 2
    d = (L - R) / 2 * width
    y = np.stack([c + d, c - d]) * amp
    return S.butter(y, 140, 'high', 2)


# ----------------------------------------------------------------- drums

def psy_kick(vel=1.0, f_end=51.91, length=0.25):
    """Short, tight psytrance kick, tuned to G#1 (the fifth of C#);
    it is gone by the first off-kick bass 16th."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + 190.0 * np.exp(-t / 0.022) + 600 * np.exp(-t / 0.0025)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.085) * np.minimum(t / 0.0006, 1)
    amp *= np.clip((length - t) / 0.05, 0, 1)
    click = S.butter(S.rng().standard_normal(n), [2500, 9000], 'band', 2) * np.exp(-t / 0.0018) * 0.35
    y = np.tanh(2.6 * body * amp) * 0.92 + click
    return np.stack([y, y]) * vel


def roll_bass(midi, vel=1.0, length=0.098, phase=0.0):
    """One note of the rolling psytrance bass (K-b-b-b): filtered saw/square
    with a fast filter envelope plus the sine fundamental; mono.  phase: the
    start phase of a free-running oscillator (0..1), so that the note train
    is not a phase-locked comb."""
    n = int((length + 0.01) * SR)
    t = np.arange(n) / SR
    f0 = float(hz(midi))
    w = S.saw(f0, n, phase) * 0.6 + 0.4 * 0.5 * (S.saw(f0, n, phase) - S.saw(f0, n, (phase + 0.5) % 1.0))
    fc = 120 + 1300 * np.exp(-t / 0.025)
    y = ladder(w, fc, 1.0, 1.2)
    y += 0.9 * np.sin(2 * np.pi * (f0 * t + phase))
    e = np.minimum(t / 0.0015, 1) * np.exp(-t / 0.075) * np.clip((length + 0.01 - t) / 0.012, 0, 1)
    y = np.tanh(1.5 * y * e) * vel
    y = S.butter(y, 38, 'high', 2)
    return np.stack([y, y])


_SN_RR = [(1.000, 1.00, 1.00), (0.988, 0.93, 0.95), (1.012, 1.06, 0.98), (0.996, 0.97, 0.93)]


def round_snare(vel=1.0, var=0, f0=207.65):
    """Short, rounded, woody snare/clap ("pdlap"): a body tuned to G#3 with a fast
    pitch drop, a short 1-4 kHz snap, a tiny clap flam, ~100 ms decay, LP 7 kHz.
    var: one of 4 round-robin variants."""
    pm, tone, lvl = _SN_RR[var % 4]
    n = int(0.2 * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    f = f0 * pm * (1 + 0.5 * np.exp(-t / 0.010))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * np.exp(-t / 0.06) + 0.3 * np.sin(1.5 * ph) * np.exp(-t / 0.035)
    snap = S.butter(rg.standard_normal(n), [1000 * tone, 4000 * tone], 'band', 2)
    snap *= np.exp(-t / 0.014) + 0.2 * np.exp(-t / 0.045)
    clap = np.zeros(n)
    for off in (0.0, 0.008, 0.016):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.003), 0)
    clap = S.butter(rg.standard_normal(n) * clap, [1100, 3200], 'band', 2) * 0.4
    y = 0.9 * body + 0.75 * snap + 1.3 * clap
    y = np.tanh(1.3 * y) / 1.1 * np.minimum(t / 0.0004, 1)
    y = S.butter(y, 7000, 'low', 2)
    fe = int(0.02 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 16)]) * vel * lvl


def conga(midi, vel=1.0, slap=False, pan=0.0):
    """Hand-drum: a tuned membrane with a small pitch drop and a skin slap."""
    n = int(0.35 * SR)
    t = np.arange(n) / SR
    f = float(hz(midi)) * (1 + 0.12 * np.exp(-t / 0.012))
    ph = 2 * np.pi * np.cumsum(f) / SR
    y = np.sin(ph) * np.exp(-t / (0.09 if not slap else 0.05)) + 0.25 * np.sin(2.4 * ph) * np.exp(-t / 0.02)
    sl = S.butter(S.rng().standard_normal(n), [700, 3500], 'band', 2) * np.exp(-t / (0.006 if not slap else 0.012))
    y = np.tanh(1.2 * (y + (0.25 if not slap else 0.6) * sl))
    return S.pan2(y * vel * 0.8, pan)


def shaker(vel=1.0, pan=0.2):
    n = int(0.07 * SR)
    t = np.arange(n) / SR
    y = S.butter(S.rng().standard_normal(n), [5000, 11000], 'band', 2)
    e = np.minimum(t / 0.012, 1) * np.exp(-t / 0.018)
    return S.pan2(y * e * vel * 0.8, pan)


# ----------------------------------------------------------------- FX

def swirl(length, f_lo=500.0, f_hi=2600.0, rate=0.11, q=5.0, pan_rate=0.07, seed_phase=0.0):
    """Soft cosmic swirl: noise through a slowly wandering resonant band-pass,
    circling in the stereo field.  Fades in and out; meant for the background."""
    n = int(length * SR)
    t = np.arange(n) / SR
    x = S.rng().standard_normal(n)
    wob = 0.5 + 0.5 * np.sin(2 * np.pi * rate * t + seed_phase) * np.cos(2 * np.pi * rate * 0.37 * t)
    fc = f_lo * (f_hi / f_lo) ** wob
    y = S.svf(x, fc, q, 'bp')
    y2 = S.svf(x[::-1].copy(), fc * 1.5, q, 'bp')
    env = np.sin(np.pi * np.clip(t / length, 0, 1)) ** 1.5
    p = np.sin(2 * np.pi * pan_rate * t + seed_phase)
    L = y * np.sqrt((1 - p) / 2) + y2 * np.sqrt((1 + p) / 2)
    R = y * np.sqrt((1 + p) / 2) + y2 * np.sqrt((1 - p) / 2)
    return np.stack([L, R]) * env * 0.25


def space_drone(midi_list, length, cut=900.0, rate=0.05):
    """A slowly beating, filtered saw drone (for the intro / breakdown floor)."""
    n = int(length * SR)
    t = np.arange(n) / SR
    out = np.zeros((2, n))
    for j, md in enumerate(midi_list):
        for side in (0, 1):
            dt = (0.07 if side else -0.07) * (1 + 0.3 * j)
            out[side] += S.saw(hz(md + dt + 0.03 * np.sin(2 * np.pi * rate * t + j)), n, S.rng().random())
    fc = cut * 2 ** (0.6 * np.sin(2 * np.pi * rate * 0.5 * t))
    out = S.svf(out, fc, 0.9)
    env = np.minimum(t / 2.5, 1) * np.clip((length - t) / 1.5, 0, 1)
    return out * env / len(midi_list) * 0.5


def psy_hat(vel=1.0, open_=False, pan=0.0):
    """Crisp 909-ish psytrance hat with body in the 4-10 kHz range (S.hat sits
    above 7.5 kHz only): metallic square partials + noise, short decay."""
    length = 0.32 if open_ else 0.06
    n = int(length * SR)
    t = np.arange(n) / SR
    src = 0.6 * S._metal(n) + 0.5 * S.rng().standard_normal(n)
    y = S.butter(S.butter(src, 4200, 'high', 4), 12500, 'low', 2)
    tau = 0.085 if open_ else 0.014
    y = y * np.exp(-t / tau) * np.minimum(t / 0.0005, 1)
    fe = int(0.004 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.9, pan)
