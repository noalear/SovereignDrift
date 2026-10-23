# synth_new13.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments for new13_industrial_techno.py ("Reactor Hall", dark rolling
# industrial techno): a distorted but tight kick tuned to B1, a tuned
# "rumble" voice (the rolling low end that follows the root), hydraulic
# pneumatic hisses, tuned metal-pipe toms, a cold steel lead with a metallic FM
# edge, a short distorted dyad stab, and helpers for sampled machinery.
# Built on synth.py and synth_new06.py (neither is modified).
#
# Samples: VSCO-2 Community Edition by Versilian Studios, CC0 1.0 / public
# domain, https://github.com/sgossner/VSCO-2-CE -- Percussion/Anvil_Hit1,
# Percussion/BrakeDrum1_Hammer, Percussion/Ratchet1-Slow, Percussion/TB_hit
# (tubular bells), Percussion/gongscrape, VSCO 1 Percussion/varMetal/Gong
# (gong smack).  Read as data with soundfile; everything else is synthesized.
import os
import numpy as np

import synth as S
import synth_new06 as I
from synth import SR, hz, saw, sine, svf, butter

VSCO = os.environ.get('VSCO_DIR', '/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/vsco')
I.VSCO = VSCO.rstrip('/') + '/'   # synth_new06 reads its sample folder from this module global

KICK_HZ = 61.735  # B1, the tonic


def _fade_tail(y, sec=0.02):
    k = int(sec * SR)
    y[..., -k:] *= np.linspace(1, 0, k)
    return y


# ---------------------------------------------------------------- drums

def kick(vel=1.0, var=0, f_end=KICK_HZ):
    """Distorted but tight techno kick: fast pitch drop to B1, hard saturation,
    a band-passed crunch layer and a short click; ~0.34 s, no boom tail (the
    tail is the separate tuned rumble)."""
    n = int(0.34 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(130 + var)
    f = f_end + (235.0 - f_end) * np.exp(-t / 0.026) + 650 * np.exp(-t / 0.0018)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.115) * np.minimum(t / 0.0005, 1)
    y = np.tanh(3.2 * body * amp) / np.tanh(3.2)
    crunch = np.clip(4.0 * body * amp, -1, 1)
    crunch = butter(butter(crunch, 250, 'high', 2), 4500, 'low', 2) * 0.22
    click = butter(g.standard_normal(n), [1500, 5500], 'band', 2) * np.exp(-t / 0.0015) * 0.28
    y = (0.9 * y + crunch + click) * (1 + 0.02 * (var - 1))
    y = _fade_tail(y, 0.04)
    return np.stack([y, y]).astype(np.float32) * vel


def rumble(midi, length, vel=1.0, seed=0, grit=1.0, peak=0.07, decay=0.17, delay=0.035):
    """One beat of the rolling rumble, tuned to `midi` (e.g. B1): a sine and its
    octave plus band-passed noise, swelling just after the kick and dying
    before the next one, saturated and low-passed.  Mono low end with a
    little decorrelated grit above 250 Hz for width."""
    n = int((length + 0.06) * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(700 + seed)
    f = float(hz(midi))
    u = np.maximum(t - delay, 0)
    env = (1 - np.exp(-u / (peak * 0.45))) * np.exp(-u / decay) * (t >= delay)
    env /= env.max() + 1e-9
    tone = sine(f, n, g.random()) + 0.45 * sine(2 * f * 1.002, n, g.random())
    nz = svf(g.standard_normal(n), 3 * f, 3.0, 'bp') * 0.5 * grit
    y = np.tanh(2.4 * (tone + nz) * env)
    low = svf(y, 190.0, 0.8)
    low = butter(low, 28, 'high', 2)
    hi = []
    for c in range(2):
        nn = svf(g.standard_normal(n), 2.4 * f * (1.0 + 0.04 * c) * 3, 2.0, 'bp')
        hi.append(butter(np.tanh(3 * (nn * 0.6 + y) * env), [250, 1400], 'band', 2) * 0.12 * grit)
    out = np.stack([low + hi[0], low + hi[1]])
    return _fade_tail(out, 0.03).astype(np.float32) * vel


def hydraulic(vel=1.0, var=0, pan=0.0):
    """Pneumatic 'tss-k': noise through a resonant band-pass falling 2.6 kHz ->
    700 Hz, a small valve thunk, dark (LP 7 kHz)."""
    n = int(0.22 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(300 + var)
    f0 = 2600 * (1 + 0.08 * (var - 1.5))
    fc = 700 + (f0 - 700) * np.exp(-t / 0.035)
    y = svf(g.standard_normal(n), fc, 2.5, 'bp')
    y *= np.minimum(t / 0.003, 1) * (np.exp(-t / 0.035) + 0.15 * np.exp(-t / 0.09))
    th = np.sin(2 * np.pi * np.cumsum(150 - 60 * (1 - np.exp(-t / 0.02))) / SR) * np.exp(-t / 0.018) * 0.35
    y = butter(y + th, 7000, 'low', 2)
    y = _fade_tail(y, 0.02)
    return S.pan2(y * vel, pan).astype(np.float32)


def pipe_tom(midi, vel=1.0, seed=0):
    """Tuned metal pipe struck like a tom: modal metal hit + a pitched tom body."""
    a = I.metal_hit(midi, 1.0, decay=0.22, bright=0.55, seed=seed)
    b = S.tom(midi - 12, 1.0) * 0.45
    n = max(a.shape[1], b.shape[1])
    y = np.zeros((2, n), np.float32)
    y[:, :a.shape[1]] += a
    y[:, :b.shape[1]] += b.astype(np.float32)
    y = butter(y, 6500, 'low', 2)
    return _fade_tail(y).astype(np.float32) * vel


def ratchet(semis=-7.0, vel=1.0, maxlen=6.0):
    """Slow ratchet crank, pitched down: a machine winding up (texture)."""
    x = I.load(I.VSCO + 'Percussion/Ratchet1-Slow_v1_rr1_Sum.wav')
    y = I.repitch(x, semis, maxlen=maxlen, fade=0.5)
    y = butter(butter(y, 3500, 'low', 2), 150, 'high', 2)
    return (y * vel).astype(np.float32)


# --------------------------------------------------------- tonal synths

def steel_lead(midi, dur, vel=1.0, cut=1100.0, env=1600.0, r=0.3):
    """Cold steel lead: pulse + saw through a resonant LP with a short envelope,
    saturated, with a metallic FM strike on the attack."""
    n = int((dur + 6 * r) * SR)
    t = np.arange(n) / SR
    f = hz(midi)
    out = []
    for c, dv in enumerate((-0.06, 0.06)):
        ff = f * 2 ** (dv / 12)
        w = 0.6 * I.pulse(ff, n, 0.3, 0.2 * c) + 0.4 * saw(ff * 1.002, n, 0.5 * c)
        fc = cut + env * np.exp(-t / 0.12)
        y = svf(w, fc, 1.3)
        out.append(y)
    x = np.stack(out)
    fm = np.sin(2 * np.pi * f * t + 2.5 * np.exp(-t / 0.07) * np.sin(2 * np.pi * 3.5 * f * t))
    fm *= np.exp(-t / 0.22) * 0.35
    x = x + fm[None, :]
    e = S.env_adsr(n, dur, 0.004, 0.35, 0.7, r)
    x = np.tanh(2.0 * x * e) / np.tanh(2.0)
    x = butter(x, 130, 'high', 2)
    return _fade_tail(x).astype(np.float32) * vel


def stab(midis, dur, vel=1.0, cut=900.0, env=1400.0, drive=3.0):
    """Short distorted dyad stab (detuned saws, LP envelope, tanh), slightly wide."""
    r = 0.04
    n = int((dur + 6 * r) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for k, md in enumerate(midis):
        for o, p in ((-0.08, -0.5), (0.08, 0.5)):
            w = saw(hz(md + o), n, (0.37 * k + 0.5 * (o > 0)) % 1)
            L += w * np.sqrt((1 - p) / 2)
            R += w * np.sqrt((1 + p) / 2)
    fc = cut + env * np.exp(-t / 0.05)
    x = svf(np.stack([L, R]) / len(midis), fc, 1.2)
    e = np.where(t < dur, 1.0, np.exp(-(t - dur) / r)) * np.minimum(t / 0.002, 1)
    x = np.tanh(drive * x * e) / np.tanh(drive)
    x = butter(x, 160, 'high', 2)
    return _fade_tail(x).astype(np.float32) * vel
