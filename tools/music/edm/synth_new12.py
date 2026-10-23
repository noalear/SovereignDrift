# synth_new12.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments for new track 12, the Berlin-school sequencer piece
# (tools/music/edm/new12_berlinschool.py).  Builds on synth.py and
# synth_new10.py (neither is modified).  Everything is synthesized: no samples.
#
#   seq_note     Moog-style sequencer voice: saw + pulse + detuned saw through a
#                24 dB low-pass (two cascaded state-variable stages, resonance on
#                the second), short filter envelope, a little drive.  The base
#                cutoff is a parameter, so the arrangement can "open the filter"
#                slowly over minutes, as on a modular.
#   mello_flute  Mellotron-like flute: a few harmonics, breath noise, a chiff at
#                the start, tape wow and a slow tape-head attack.
#   tape_strings Mellotron/Solina-ish string voice with deeper tape wow, dark.
#   tape_echo    a stereo tape echo (Revox-style): each repeat darker, a little
#                wow on the delay time, alternating sides.
import numpy as np

import synth as S
import synth_new10 as X
from synth import SR, hz

_r = np.random.default_rng(1212)


def _t(n):
    return np.arange(n) / SR


def ladder(x, fc, q=2.0):
    """24 dB/oct low-pass: two SVF stages, the resonance on the second."""
    return S.svf(S.svf(x, fc, 0.62), fc, q)


def seq_note(midi, dur, vel=1.0, cut=600.0, env=1.4, decay=0.11, q=2.2, r=0.035,
             pulse_amt=0.35, sub=0.0, drive=1.4):
    """One sequencer step (mono float32).  fc = cut * (1 + env * exp(-t/decay))."""
    f = float(hz(midi))
    n = int((dur + 6 * r + 0.01) * SR)
    t = _t(n)
    ph = _r.random()
    x = 0.55 * S.saw(f, n, ph)
    x += 0.35 * S.saw(f * 2 ** (0.07 / 12), n, (ph + 0.31) % 1.0)
    if pulse_amt:
        p2 = (ph + 0.5) % 1.0
        x += pulse_amt * 0.5 * (S.saw(f, n, ph) - S.saw(f, n, p2))
    if sub:
        x += sub * S.sine(f * 0.5, n)
    fc = np.clip(cut * (1 + env * np.exp(-t / decay)), 30, 16000)
    y = ladder(x, fc, q)
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / r))
    y = y * gate * np.minimum(t / 0.0015, 1)
    y = np.tanh(drive * y) / np.tanh(drive)
    return (y * vel).astype(np.float32)


def mello_flute(midi, dur, vel=1.0, t0=0.0, pan=0.0, attack=0.09, release=0.35):
    """Mellotron flute note, (2, n) float32."""
    f0 = float(hz(midi))
    n = int((dur + 5 * release) * SR)
    t = _t(n)
    vib = 0.07 * np.clip((t - 0.35) / 0.5, 0, 1) * np.sin(2 * np.pi * 4.7 * t + _r.random() * 6)
    w = X.wow(n, t0, depth=0.07, rate=0.45)
    f = f0 * 2 ** ((vib + w) / 12)
    ph = 2 * np.pi * np.cumsum(f) / SR
    y = np.sin(ph) + 0.16 * np.sin(2 * ph + 0.4) + 0.07 * np.sin(3 * ph + 1.1) + 0.025 * np.sin(4 * ph)
    breath = S.butter(_r.standard_normal(n), [min(f0 * 1.5, 6000), min(f0 * 4.5, 12000)], 'band', 2)
    y += breath * (0.05 + 0.25 * np.exp(-t / 0.05))
    e = S.env_adsr(n, dur, attack, 0.6, 0.85, release / 4.6 * 1.5)
    y = np.tanh(1.2 * y * e) / np.tanh(1.2)
    y = S.butter(y, 5200, 'low', 2)
    return S.pan2(y * vel * 0.45, pan).astype(np.float32)


def tape_strings(midi, dur, vel=1.0, t0=0.0, attack=0.5, release=1.2, cut=2100.0):
    """Mellotron-ish strings: string machine with a deeper tape wow, darker."""
    a = X.analog_pad(midi, dur, vel, t0, attack=attack, release=release, cut=cut, voices=3,
                     detune=0.11, q=0.6, bright_env=0.4, width=1.0)
    b = X.analog_pad(midi + 12, dur, vel * 0.3, t0 + 0.11, attack=attack * 1.3, release=release,
                     cut=cut * 1.2, voices=2, detune=0.1, q=0.6, bright_env=0.3, width=1.0)
    k = min(a.shape[1], b.shape[1])
    return a[:, :k] + b[:, :k]


def tape_echo(x, delay_sec, feedback=0.4, repeats=7, lp=4200.0, hp=300.0, wow_ms=0.6):
    """Stereo tape echo (wet only): alternating sides, each repeat darker,
    a little wow on the delay time."""
    mono = (x[0] + x[1]) * 0.5
    sig = S.butter(S.butter(mono, hp, 'high', 2), lp, 'low', 2)
    n = x.shape[1]
    out = np.zeros((2, n), np.float32)
    idx = np.arange(n, dtype=np.float64)
    for i in range(1, repeats + 1):
        d = delay_sec * i * SR
        wobble = wow_ms * SR / 1000.0 * np.sin(2 * np.pi * 0.37 * idx / SR + i)
        pos = idx - d - wobble
        ok = pos >= 0
        p = pos[ok]
        i0 = np.floor(p).astype(np.int64)
        fr = p - i0
        i1 = np.minimum(i0 + 1, n - 1)
        v = sig[i0] * (1 - fr) + sig[i1] * fr
        g = feedback ** (i - 1)
        out[(i - 1) % 2, ok] += (v * g).astype(np.float32)
        sig = S.butter(sig, lp * 0.88 ** i, 'low', 1)
    return out
