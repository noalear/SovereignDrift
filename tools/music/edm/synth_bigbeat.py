# synth_bigbeat.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for the big beat / breakbeat track new04_bigbeat.py.  Builds on
# synth.py (imported, not modified).  Everything is synthesized: no samples.
#
#   bb_kick     round, punchy breakbeat kick tuned to G1 (the fifth of C minor)
#   bb_snare    fat, short "pdlap" snare: tuned body at G3 with a fast pitch
#               drop, band-limited 1-4 kHz snap, tiny clap flam, ~110 ms decay,
#               4 round-robin variants; ghost notes are darker
#   bb_hat, shaker, claves, taiko
#   acid        TB-303 style monophonic line: saw, 24 dB resonant low-pass,
#               accent (more env, louder, shorter decay) and slide (legato glide,
#               no envelope retrigger), soft saturation
#   dist_bass   distorted analog riff bass (2 saws + square, filter env, drive)
#               plus a separate clean sine sub for a mono low end
#   stab        short brassy saw-stack chord stab
#   mono_lead   gritty mono synth lead with glide and delayed vibrato
#   strings     string-machine pad (detuned saws; ensemble chorus on the bus)
import numpy as np

import synth as S
from synth import SR, hz


def _t(n):
    return np.arange(n) / SR


def square(freq, n, phase=0.0):
    """Band-limited square as the difference of two polyBLEP saws."""
    return 0.5 * (S.saw(freq, n, phase) - S.saw(freq, n, (phase + 0.5) % 1.0))


# ------------------------------------------------------------------ drums

def bb_kick(vel=1.0, f_end=49.0, f_start=150.0, length=0.36):
    n = int(length * SR)
    t = _t(n)
    f = f_end + (f_start - f_end) * np.exp(-t / 0.035) + 260 * np.exp(-t / 0.004)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.17) * np.minimum(t / 0.0006, 1)
    amp *= np.where(t > length - 0.05, np.clip((length - t) / 0.05, 0, 1), 1)
    knock = S.butter(S.rng().standard_normal(n), [900, 3500], 'band', 2) * np.exp(-t / 0.004) * 0.18
    y = np.tanh(2.6 * body * amp) * 0.92 + knock
    return np.stack([y, y]) * vel


_SN_RR = [(1.000, 1.00, 1.00), (0.982, 0.93, 0.95), (1.017, 1.06, 0.98), (0.994, 0.97, 0.92)]


def bb_snare(vel=1.0, var=0, ghost=False, f0=196.0):
    """Fat, round, short big-beat snare ("pdlap", never "dtschh")."""
    pm, tone, lvl = _SN_RR[var % 4]
    dark = 1.0 if ghost else 0.0
    n = int(0.24 * SR)
    t = _t(n)
    rg = S.rng()
    f = f0 * pm * (1 + 0.5 * np.exp(-t / 0.011))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.055) + 0.4 * np.sin(1.59 * ph) * np.exp(-t / 0.035) +
            0.12 * np.sin(2.3 * ph) * np.exp(-t / 0.02))
    snap = S.butter(rg.standard_normal(n), [1100 * tone, 3800 * tone], 'band', 2)
    snap *= (np.exp(-t / 0.018) + 0.22 * np.exp(-t / 0.055)) * (1 - 0.75 * dark)
    clap = np.zeros(n)
    for off in (0.0, 0.006, 0.013):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0032), 0)
    clap = S.butter(rg.standard_normal(n) * clap, [1100, 2900], 'band', 2) * 0.45 * (1 - 0.9 * dark)
    # a short, dark "room": a few early reflections of the body
    room = np.zeros(n)
    for d_ms, g in ((7.3, 0.18), (11.9, 0.12), (17.1, 0.08), (23.7, 0.05)):
        k = int(d_ms * 1e-3 * SR)
        room[k:] += body[:-k] * g
    y = 1.0 * body + 1.5 * snap + 1.2 * clap + room
    y = np.tanh(1.3 * y) / 1.1 * np.minimum(t / 0.0004, 1)
    y = S.butter(y, 7000 - 4000 * dark, 'low', 2)
    y = S.butter(y, 120, 'high', 1)
    fe = int(0.03 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 11)]) * vel * lvl


def bb_hat(vel=1.0, open_=False, pan=0.15):
    length = 0.32 if open_ else 0.06
    n = int(length * SR)
    t = _t(n)
    src = 0.5 * S._metal(n) + 0.6 * S.rng().standard_normal(n)
    y = S.butter(src, [6500, 12500], 'band', 2) * np.exp(-t / (0.11 if open_ else 0.018))
    y *= np.minimum(t / 0.0005, 1)
    fe = int(0.01 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel, pan)


def shaker(vel=1.0, pan=-0.3):
    n = int(0.09 * SR)
    t = _t(n)
    e = np.minimum(t / 0.007, 1) * np.exp(-np.maximum(t - 0.007, 0) / 0.026)
    y = S.butter(S.rng().standard_normal(n), [3500, 8500], 'band', 2) * e
    return S.pan2(y * vel * 0.8, pan)


def claves(vel=1.0, pan=0.35, f=1760.0):
    n = int(0.12 * SR)
    t = _t(n)
    y = np.sin(2 * np.pi * f * t) * np.exp(-t / 0.022) + 0.25 * np.sin(2 * np.pi * 2.71 * f * t) * np.exp(-t / 0.008)
    return S.pan2(y * vel * np.minimum(t / 0.0004, 1), pan)


def taiko(midi, vel=1.0, decay=0.35):
    n = int((decay * 4 + 0.1) * SR)
    t = _t(n)
    f = hz(midi) * (1 + 0.45 * np.exp(-t / 0.025))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / decay)
    y += 0.4 * np.sin(2 * np.pi * np.cumsum(f * 1.52) / SR) * np.exp(-t / (decay * 0.4))
    y += S.butter(S.rng().standard_normal(n), 1400, 'low', 2) * np.exp(-t / 0.025) * 0.6
    y = np.tanh(1.6 * y) * np.minimum(t / 0.0008, 1)
    return np.stack([y, y]) * vel


# -------------------------------------------------------------- the 303

def acid(notes, length, cut, reso=3.2, oct_mod=2.2, decay=0.16, glide=0.04, drive=1.8, square_mix=0.0):
    """TB-303 style line.

    notes   list of (start_s, dur_s, midi, accent, slide); slide = glide into the NEXT note
            without retriggering the envelopes (the gate stays open)
    length  seconds to render
    cut     base cutoff (Hz): scalar or per-sample array of the rendered length
    """
    n = int(length * SR)
    t = _t(n)
    pitch = np.full(n, float(notes[0][2]) if notes else 48.0)
    amp = np.zeros(n)
    fenv = np.zeros(n)
    acc_amp = np.ones(n)
    prev_slide = False
    prev_pitch = None
    for i, (st, du, md, acc, sl) in enumerate(notes):
        i0 = int(st * SR)
        if i0 >= n:
            break
        nxt = int(notes[i + 1][0] * SR) if i + 1 < len(notes) else n
        nxt = min(nxt, n)
        gate_end = nxt if sl else min(i0 + int(du * SR), nxt)
        seg = np.arange(nxt - i0) / SR
        if prev_slide and prev_pitch is not None:
            pitch[i0:nxt] = md + (prev_pitch - md) * np.exp(-seg / glide)
        else:
            pitch[i0:nxt] = md
        if not prev_slide:
            # retrigger: filter envelope and amplitude attack
            d = decay * (0.55 if acc else 1.0)
            fenv[i0:nxt] = np.exp(-seg / d) * (1.45 if acc else 1.0)
            a0 = amp[i0 - 1] if i0 > 0 else 0.0
            a = a0 + (1.0 - a0) * np.minimum(seg / 0.002, 1.0)
        else:
            fenv[i0:nxt] = fenv[i0 - 1] * np.exp(-seg / decay) if i0 > 0 else 0.0
            a = np.ones_like(seg)
        g = gate_end - i0
        e = a.copy()
        if g < len(seg):
            e[g:] = e[g] * np.exp(-(seg[g:] - seg[g]) / 0.008)
        amp[i0:nxt] = e
        acc_amp[i0:nxt] = 1.3 if acc else 1.0
        prev_slide = sl
        prev_pitch = md
    f = hz(pitch)
    osc = S.saw(f, n) * (1 - square_mix) + square(f, n) * square_mix
    base = np.broadcast_to(np.asarray(cut, float), (n,))
    fc = np.minimum(base * 2.0 ** (oct_mod * fenv), 5000.0)
    y = S.svf(osc, fc, reso)
    y = S.svf(y, fc * 1.15, 0.6)
    y = y * amp * acc_amp
    y = np.tanh(drive * y) / np.tanh(drive)
    y = S.butter(y, 70, 'high', 2)
    return y.astype(np.float32)


# ----------------------------------------------------------------- bass

def dist_bass(midi, dur, vel=1.0, cut=260.0, env=1900.0, decay=0.09, drive=3.5, r=0.025):
    """Distorted analog riff bass (mono), above ~90 Hz; pair with sub()."""
    n = int((dur + 6 * r) * SR)
    t = _t(n)
    f = hz(midi)
    w = 0.5 * S.saw(f, n) + 0.45 * S.saw(f * 1.006, n, 0.31) + 0.45 * square(f * 0.997, n, 0.6)
    fc = cut + env * np.exp(-t / decay)
    y = S.svf(w, fc, 1.5)
    e = S.env_adsr(n, dur, 0.002, 0.25, 0.75, r)
    y = np.tanh(drive * y * e) * 0.8
    y = S.butter(y, 3200, 'low', 2)
    y = S.butter(y, 85, 'high', 2)
    return np.stack([y, y]) * vel


def sub(midi, dur, vel=1.0, r=0.025):
    n = int((dur + 6 * r) * SR)
    y = S.sine(hz(midi), n)
    y = np.tanh(1.3 * y) / np.tanh(1.3)
    y = y * S.env_adsr(n, dur, 0.003, 0.4, 0.9, r) * vel
    return np.stack([y, y])


# ---------------------------------------------------------- synths

def stab(midis, dur, vel=1.0, cut=550.0, env=4200.0, decay=0.055, width=0.7):
    """Short brassy saw-stack chord stab (stereo)."""
    n = int((dur + 0.4) * SR)
    t = _t(n)
    L = np.zeros(n)
    R = np.zeros(n)
    for md in midis:
        f = hz(md)
        c = S.saw(f, n, S.rng().random())
        l_ = S.saw(f * 2 ** (-11 / 1200), n, S.rng().random())
        r_ = S.saw(f * 2 ** (11 / 1200), n, S.rng().random())
        sq = square(f * 0.5, n) * 0.12
        L += c * 0.6 + l_ * (0.5 + 0.3 * width) + r_ * (0.5 - 0.3 * width) + sq
        R += c * 0.6 + r_ * (0.5 + 0.3 * width) + l_ * (0.5 - 0.3 * width) + sq
    fc = cut + env * np.exp(-t / decay)
    y = S.svf(np.stack([L, R]), fc, 1.1)
    e = S.env_adsr(n, dur, 0.0015, 0.12, 0.35, 0.07)
    y = np.tanh(1.3 * y * e / np.sqrt(len(midis))) * vel
    return y


def mono_lead(notes, length, cut=1700.0, env=2300.0, decay=0.22, glide=0.03, vib=0.14, drive=1.6):
    """Gritty mono lead.  notes: (start_s, dur_s, midi, legato_into_next)."""
    n = int(length * SR)
    t = _t(n)
    pitch = np.full(n, float(notes[0][2]))
    amp = np.zeros(n)
    fenv = np.zeros(n)
    vibr = np.zeros(n)
    prev_leg = False
    prev = None
    for i, (st, du, md, leg) in enumerate(notes):
        i0 = int(st * SR)
        if i0 >= n:
            break
        nxt = int(notes[i + 1][0] * SR) if i + 1 < len(notes) else n
        nxt = min(nxt, n)
        tail = min(n, max(nxt, i0 + int((du + 0.25) * SR)))
        seg = np.arange(tail - i0) / SR
        if prev is not None and prev_leg:
            pitch[i0:tail] = md + (prev - md) * np.exp(-seg / glide)
        else:
            pitch[i0:tail] = md
        if not prev_leg:
            fenv[i0:tail] = np.exp(-seg / decay)
            a0 = amp[i0 - 1] if i0 > 0 else 0.0
            a = a0 + (1.0 - a0) * np.minimum(seg / 0.006, 1.0)
        else:
            fenv[i0:tail] = np.maximum(fenv[i0 - 1] * np.exp(-seg / 0.4), 0.0)
            a = np.ones_like(seg)
        g = int(du * SR)
        e = a * (0.82 + 0.18 * np.exp(-seg / 0.15))
        if g < len(seg):
            e[g:] = e[g] * np.exp(-(seg[g:] - seg[g]) / 0.06)
        amp[i0:tail] = e
        if du >= 0.5:
            vibr[i0:tail] = np.clip((seg - 0.25) / 0.35, 0, 1) * vib * np.sin(2 * np.pi * 5.4 * seg)
        prev_leg = leg
        prev = md
    p = pitch + vibr
    f = hz(p)
    center = S.saw(f, n) * 0.55 + square(f * 0.5, n) * 0.2
    L = center + S.saw(f * 2 ** (-7 / 1200), n, 0.21) * 0.5
    R = center + S.saw(f * 2 ** (7 / 1200), n, 0.67) * 0.5
    fc = np.minimum(cut + env * fenv, 6500.0)
    y = S.svf(np.stack([L, R]), fc, 1.0)
    y = np.tanh(drive * y * amp[None, :]) / np.tanh(drive)
    y = S.butter(y, 180, 'high', 2)
    return y.astype(np.float32)


def strings(midi, dur, vel=1.0, a=0.6, r=0.9, cut=2400.0):
    n = int((dur + 5 * r) * SR)
    L = np.zeros(n)
    R = np.zeros(n)
    f = hz(midi)
    for k, c in enumerate((-14, -6, 5, 13)):
        w = S.saw(f * 2 ** (c / 1200), n, S.rng().random())
        if k % 2:
            L += w
        else:
            R += w
        (R if k % 2 else L)[:] += 0.35 * w
    y = S.svf(np.stack([L, R]), cut, 0.7)
    y = S.butter(y, 160, 'high', 1)
    e = S.env_adsr(n, dur, a, 1.0, 0.9, r)
    return y * e * vel * 0.3


def glass(midi, dur, vel=1.0, r=0.6):
    """Harmonic two-operator FM bell (ratio 2): clean pitch, for the pings and the bell line."""
    f = float(hz(midi))
    n = int((dur + 5 * r) * SR)
    t = _t(n)
    idx = 1.6 * np.exp(-t / 0.12) + 0.25
    y = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * 2.0 * f * t))
    y += 0.25 * np.sin(2 * np.pi * 2 * f * t) * np.exp(-t / 0.3)
    e = np.exp(-t / (0.8 * r)) * np.minimum(t / 0.0015, 1)
    e *= np.where(t < dur + 0.5 * r, 1, np.exp(-(t - dur - 0.5 * r) / 0.25))
    return S.pan2(y * e * vel * 0.5, 0.0)
