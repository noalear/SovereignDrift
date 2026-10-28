# synth_new16.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments for new track 16, "Kill Vector", a late-90s techstep /
# neurofunk drum & bass piece (new16_neurofunk.py).  Builds on synth.py
# (imported, not modified) and adds:
#
#   traj         per-sample pitch + gate trajectory for a bass line (glides,
#                re-triggers with a tiny gap, sub drops)
#   reese        reese bass: three detuned saws through ONE fixed low-pass
#                (no filter movement at all), driven, mono low end
#   sub          clean sine sub (gentle tanh) following the same trajectory
#   stab         dark techstep stab: detuned saws + a hollow pulse through a
#                fixed low-pass, fast decay, a short body transient
#   pad          dark string-machine pad (slow attack, low-passed)
#   ghostlead    thin, hollow "eerie" lead (detuned pulse pair, late vibrato)
#   kick         short punchy DnB kick, body tuned to F#1 (the tonic)
#   snare        round, tuned "pdlap" snare (A3 body, 1.6x mode, 1.2-4 kHz snap,
#                rim + clap inside the hit, ~110 ms to -20 dB, LP 7 kHz),
#                4 round-robin variants, softer hits get less snap
#   hat / shaker / ride   band-limited metal (nothing harsh above ~11 kHz)
#   drone        quiet low noise-band bed for the intro and breakdown
import numpy as np
import numba

import synth as S
from synth import SR, hz


# ------------------------------------------------------------- bass

def traj(notes, n, glide=0.010, gap=0.006):
    """notes: sorted (t_start_s, midi, dur_s, drop_semitones).  Returns per-sample
    pitch (midi) and gate env (0..1).  Consecutive notes re-trigger with a tiny gap."""
    pitch = np.full(n, np.nan)
    env = np.zeros(n)
    prev_end, prev_p = None, None
    a = int(0.002 * SR)
    g = int(gap * SR)
    for ts, md, d, drop in notes:
        i0 = max(0, int(ts * SR))
        i1 = min(n, int((ts + d) * SR))
        if i1 - i0 < 4 * g:
            continue
        L = i1 - i0
        u = np.arange(L) / SR
        p = np.full(L, float(md))
        if drop:
            p -= drop * (u / (L / SR)) ** 1.6
        if prev_p is not None and prev_end is not None and abs(prev_end - ts) < 0.01 and prev_p != md:
            p += (prev_p - md) * np.exp(-u / glide)
        pitch[i0:i1] = p
        e = np.ones(L)
        e[:a] = np.linspace(0, 1, a)
        e[-g:] = np.linspace(1, 0, g)
        if drop:
            e *= np.clip(1.15 - u / (L / SR), 0, 1) ** 0.5
        env[i0:i1] = e
        prev_end, prev_p = ts + d, p[-1]
    idx = np.where(~np.isnan(pitch), np.arange(n), 0)
    np.maximum.accumulate(idx, out=idx)
    first = int(np.argmax(~np.isnan(pitch)))
    pitch[:first] = pitch[first]
    pitch = pitch[idx]
    return pitch, env


def reese(pitch, env, cut=750.0, drive=2.2, width=0.35, octave=0.0, q=0.9):
    """Reese: 3 detuned saws -> fixed LP (constant cutoff) -> drive.  (2, n)."""
    n = len(pitch)
    rg = S.rng()
    f = hz(pitch)
    v1 = S.saw(f * 2 ** (-0.13 / 12), n, rg.random())
    v2 = S.saw(f * 2 ** (0.13 / 12), n, rg.random())
    v3 = S.saw(f * 2 ** (0.02 / 12), n, rg.random())
    mono = 0.5 * (v1 + v2) + 0.55 * v3
    if octave:
        mono = mono + octave * S.saw(f * 2.0 * 2 ** (0.05 / 12), n, rg.random())
    side = (v1 - v2) * 0.5
    mono = S.svf(mono, cut, q)
    side = S.svf(side, cut * 1.3, q)
    side = S.butter(side, 280, 'high', 2) * width
    mono = np.tanh(drive * mono * env) / np.tanh(drive)
    side = np.tanh(drive * side * env) / np.tanh(drive)
    y = np.stack([mono + side, mono - side])
    y = S.butter(y, 4500, 'low', 2)
    y = S.butter(y, 70, 'high', 2)
    return y.astype(np.float32)


def sub(pitch, env, drive=1.3):
    y = S.sine(hz(pitch), len(pitch))
    y = np.tanh(drive * y) / np.tanh(drive)
    # smooth the gate a little more for the sub (no clicks)
    k = int(0.003 * SR)
    e = np.convolve(env, np.ones(k) / k, mode='same')
    y = (y * e).astype(np.float32)
    return np.stack([y, y])


# ------------------------------------------------------------- stabs / pads / lead

def stab(midis, dur=0.22, vel=1.0, cut=2400.0, decay=0.13, width=0.5):
    """Dark techstep stab (2, n): fixed tone, no sweep."""
    n = int((dur + 0.35) * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    L = np.zeros(n)
    R = np.zeros(n)
    for md in midis:
        f = float(hz(md))
        for o, p in ((-0.11, -width), (0.0, 0.0), (0.11, width)):
            w = S.saw(f * 2 ** (o / 12), n, rg.random())
            L += w * np.sqrt((1 - p) / 2)
            R += w * np.sqrt((1 + p) / 2)
        ph = (f * 1.003 * t + rg.random()) % 1.0
        pul = np.where(ph < 0.32, 1.0, -0.47)
        L += 0.35 * pul
        R += 0.35 * pul
    y = np.stack([L, R]) / np.sqrt(3 * len(midis))
    y = S.svf(y, cut, 1.1)
    y = S.butter(y, 140, 'high', 2)
    amp = (0.75 * np.exp(-t / decay) + 0.25 * np.exp(-t / (decay * 3))) * np.minimum(t / 0.0012, 1)
    amp *= np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.04))
    body = S.butter(rg.standard_normal(n), [500, 2600], 'band', 2) * np.exp(-t / 0.006) * 0.25
    y = np.tanh(1.6 * (y * amp + body)) / 1.2
    return (y * vel).astype(np.float32)


def pad(midi, dur, vel=1.0, a=1.0, r=1.0, cut=1100.0, voices=4, detune=0.14):
    n = int((dur + 4 * r) * SR)
    rg = S.rng()
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for i in range(voices):
        det = (i - (voices - 1) / 2) * detune / max(1, voices - 1) * 2
        drift = 0.025 * np.sin(2 * np.pi * (0.13 + 0.1 * rg.random()) * t + rg.random() * 6.3)
        w = S.saw(hz(midi + det + drift), n, rg.random())
        p = -0.75 + 1.5 * i / max(1, voices - 1)
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = np.stack([L, R]) / np.sqrt(voices)
    x = S.svf(x, cut, 0.6)
    x = S.butter(x, 110, 'high', 1)
    e = S.env_adsr(n, dur, a, 1.0, 0.95, r)
    return (x * e * vel).astype(np.float32)


def ghostlead(midi, dur, vel=1.0, a=0.06, r=0.35, cut=2600.0, vib=0.10):
    """Thin hollow lead: two detuned pulses (~40% duty), late vibrato, fixed LP."""
    n = int((dur + 5 * r) * SR)
    rg = S.rng()
    vb = S.vibrato(n, onset=0.35, rate=5.0, depth=vib)
    out = []
    for det in (-0.06, 0.06):
        f = hz(midi + det + vb)
        p0 = rg.random()
        w = S.saw(f, n, p0) - S.saw(f, n, (p0 + 0.4) % 1.0)      # 40 % pulse
        out.append(0.5 * w + 0.25 * S.sine(f, n, p0))
    y = np.stack([out[0] * 0.85 + out[1] * 0.15, out[1] * 0.85 + out[0] * 0.15])
    y = S.svf(y, cut, 0.8)
    y = S.butter(y, 250, 'high', 2)
    e = S.env_adsr(n, dur, a, 0.6, 0.85, r)
    return (y * e * vel * 0.6).astype(np.float32)


# ------------------------------------------------------------- drums

def kick(vel=1.0, f_end=46.25, f_start=190.0, length=0.30):
    """Short punchy DnB kick, body ends on F#1."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_start - f_end) * np.exp(-t / 0.022) + 520 * np.exp(-t / 0.0018)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.095) * np.minimum(t / 0.0006, 1)
    amp *= np.where(t > length - 0.05, np.clip((length - t) / 0.05, 0, 1), 1)
    beater = S.butter(S.rng().standard_normal(n), [1800, 5500], 'band', 2) * np.exp(-t / 0.0022) * 0.28
    y = np.tanh(2.6 * body * amp) * 0.9 + beater
    return np.stack([y, y]).astype(np.float32) * vel


# (pitch mult, snap tone mult, level, body decay mult)
_SN_RR = [(1.000, 1.00, 1.00, 1.00), (0.986, 0.92, 0.94, 1.05), (1.014, 1.07, 0.97, 0.95),
          (0.995, 0.97, 0.91, 1.02)]


def snare(vel=1.0, var=0, dark=0.0, f0=220.0):
    """Round tuned techstep snare ("pdlap"): A3 body with a fast pitch drop and a
    1.59x membrane mode, short 1.2-4 kHz snap, rim, small clap flam, LP 7 kHz.
    About 110 ms to -20 dB.  dark 0..1 for ghost notes (less snap)."""
    pm, tone, lvl, dm = _SN_RR[var % 4]
    n = int(0.22 * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    f = f0 * pm * (1 + 0.55 * np.exp(-t / 0.008))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / (0.050 * dm)) + 0.34 * np.sin(1.59 * ph) * np.exp(-t / 0.032) +
            0.12 * np.sin(2.14 * ph) * np.exp(-t / 0.018))
    snap = S.butter(rg.standard_normal(n), [1200 * tone, 4000 * tone], 'band', 2)
    snap *= (np.exp(-t / 0.012) + 0.22 * np.exp(-t / 0.045)) * (1 - 0.85 * dark)
    rim = np.sin(2 * np.pi * 1750 * tone * t) * np.exp(-t / 0.003) * 0.2 * (1 - 0.7 * dark)
    cl = np.zeros(n)
    for off in (0.0, 0.007, 0.014):
        u = t - off
        cl += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0026), 0)
    cl = S.butter(rg.standard_normal(n) * cl, [1100, 3200], 'band', 2) * 0.3 * (1 - 0.9 * dark)
    y = 1.0 * body + 1.05 * snap + rim + 1.4 * cl
    y = np.tanh(1.4 * y) / 1.05 * np.minimum(t / 0.0004, 1)
    y = S.butter(y, 7000 - 3500 * dark, 'low', 2)
    fe = int(0.03 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 9)]).astype(np.float32) * vel * lvl


def hat(vel=1.0, pan=0.0, open_=False):
    length, tau = (0.30, 0.075) if open_ else (0.07, 0.014)
    n = int(length * SR)
    t = np.arange(n) / SR
    src = 0.5 * S._metal(n) + 0.6 * S.rng().standard_normal(n)
    y = S.butter(src, [6500, 11000], 'band', 2) * np.exp(-t / tau)
    y *= np.minimum(t / 0.0005, 1)
    fe = min(int(0.01 * SR), n // 3)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.9, pan).astype(np.float32)


def shaker(vel=1.0, pan=0.0):
    n = int(0.08 * SR)
    t = np.arange(n) / SR
    y = S.butter(S.rng().standard_normal(n), [3500, 8500], 'band', 2)
    y *= np.minimum(t / 0.006, 1) * np.exp(-t / 0.022)
    fe = int(0.01 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.8, pan).astype(np.float32)


def ride(vel=1.0, pan=0.3, length=0.8):
    n = int(length * SR)
    t = np.arange(n) / SR
    src = S._metal(n) * 0.7 + 0.35 * S.rng().standard_normal(n)
    y = S.butter(src, [3000, 9500], 'band', 2) * (0.5 * np.exp(-t / 0.012) + 0.5 * np.exp(-t / 0.28))
    y = y + np.sin(2 * np.pi * 2960.0 * t) * np.exp(-t / 0.2) * 0.06       # F#7-ish ping, quiet
    fe = int(0.05 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return S.pan2(y * vel * 0.7, pan).astype(np.float32)


# ------------------------------------------------------------- atmos

def drone(n, t0=0.0):
    """Quiet dark bed: noise in a slowly wandering low band (no fast sweeps)."""
    rg = S.rng()
    x = rg.standard_normal((2, n))
    t = t0 + np.arange(n) / SR
    fc = 420 * 2 ** (0.7 * np.sin(2 * np.pi * 0.05 * t) + 0.3 * np.sin(2 * np.pi * 0.021 * t + 1.0))
    y = S.svf(x, fc, 1.4, 'bp')
    return (y * 0.5).astype(np.float32)
