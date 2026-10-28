# synth_new11.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments and effects for new track 11, the dub-techno piece
# (tools/music/edm/new11_dubtechno.py).  Builds on synth.py and borrows a few
# voices from synth_new10.py (snap, soft_hat, shaker, warm_bass, analog_pad,
# hall_ir, convolve); neither file is modified.  Everything is synthesized:
# no samples, no SoundFonts.
#
#   dub_kick    soft, round, long-bodied kick tuned to G#1 (the fifth of C#m)
#   dub_stab    the Basic Channel chord stab: per note two detuned saws and a
#               soft pulse, short amplitude envelope, low-passed (the evolving
#               filter is applied on the bus)
#   dub_keys    a soft FM electric-piano voice for the motif (round, no bark)
#   dub_delay   tape-style ping-pong feedback delay with a low-pass and a
#               high-pass INSIDE the feedback loop (cutoff per sample, so the
#               echoes darken and follow the filter automation) and a soft
#               saturator in the loop (numba)
import numpy as np
import numba

import synth as S
import synth_new10 as X10
from synth import SR, hz

# re-exported voices (unchanged, from synth_new10.py)
snap = X10.snap
soft_hat = X10.soft_hat
shaker = X10.shaker
warm_bass = X10.warm_bass
analog_pad = X10.analog_pad
hall_ir = X10.hall_ir
convolve = X10.convolve

_r = np.random.default_rng(1111)


def dub_kick(vel=1.0, f_end=51.91, f_start=118.0, length=0.55):
    """Soft round kick, G#1 body (the fifth of C# minor), slow pitch drop, no click."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_start - f_end) * np.exp(-t / 0.045) + 60 * np.exp(-t / 0.007)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.26) * np.minimum(t / 0.002, 1)
    amp *= np.where(t > length - 0.08, np.clip((length - t) / 0.08, 0, 1), 1)
    y = np.tanh(1.6 * body * amp) / np.tanh(1.6)
    knock = S.butter(_r.standard_normal(n), [150, 900], 'band', 1) * np.exp(-t / 0.005) * 0.12
    y = S.butter(y + knock, 1600, 'low', 2)
    return np.stack([y, y]).astype(np.float32) * vel


def dub_stab(notes, dur=0.16, vel=1.0, cut=2400.0, env=1800.0, decay=0.05, q=1.1, width=0.5):
    """Chord stab: notes = MIDI list.  Returns (2, n) float32, ~dur + short release."""
    rel = 0.06
    n = int((dur + 6 * rel) * SR) + 1
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for j, md in enumerate(notes):
        f = float(hz(md))
        p = (j / max(len(notes) - 1, 1) * 2 - 1) * width
        for dv, side in ((-0.07, -1), (0.07, 1)):
            ph = _r.random()
            w = S.saw(f * 2 ** (dv / 12), n, ph)
            pl = w - S.saw(f * 2 ** (dv / 12), n, (ph + 0.42) % 1.0)  # a pulse (two offset saws)
            x = 0.7 * w + 0.25 * pl
            pp = np.clip(p + 0.25 * side, -1, 1)
            L += x * np.sqrt((1 - pp) / 2)
            R += x * np.sqrt((1 + pp) / 2)
    fc = cut + env * np.exp(-t / decay)
    y = S.svf(np.stack([L, R]), fc, q)
    y = S.butter(y, 160, 'high', 2)
    e = np.minimum(t / 0.003, 1) * (0.25 + 0.75 * np.exp(-t / 0.07))
    e *= np.where(t < dur, 1.0, np.exp(-(t - dur) / rel))
    k = int(0.006 * SR)
    e[-k:] *= np.linspace(1, 0, k)
    return (y * e * vel / np.sqrt(2 * len(notes))).astype(np.float32)


def dub_keys(midi, dur, vel=1.0, decay=0.9, pan=0.0):
    """Soft FM electric piano: 1:1 FM with a decaying index, a faint tine, slow decay."""
    f = float(hz(midi))
    rel = 0.25
    n = int((dur + 5 * rel) * SR) + 1
    t = np.arange(n) / SR
    idx = 1.1 * np.exp(-t / 0.12) + 0.25
    y = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * f * t))
    y += 0.08 * np.sin(2 * np.pi * 7.0 * f * t) * np.exp(-t / 0.03)
    y += 0.3 * np.sin(2 * np.pi * 0.5 * f * t) * np.exp(-t / (1.5 * decay))
    e = np.exp(-t / decay) * np.minimum(t / 0.003, 1)
    e *= np.where(t < dur, 1.0, np.exp(-(t - dur) / rel))
    k = int(0.008 * SR)
    e[-k:] *= np.linspace(1, 0, k)
    y = S.butter(y * e * vel * 0.45, 5000, 'low', 2)
    return S.pan2(y, pan).astype(np.float32)


@numba.njit(cache=False)
def _dub_delay(x, dl, dr, fb, lp, hp_c, drive, out):
    n = x.shape[0]
    size = max(dl, dr) + 2
    bl = np.zeros(size)
    br = np.zeros(size)
    w = 0
    lpl = 0.0
    lpr = 0.0
    hpl = 0.0
    hpr = 0.0
    for i in range(n):
        rl = w - dl
        if rl < 0:
            rl += size
        rr = w - dr
        if rr < 0:
            rr += size
        yl = bl[rl]
        yr = br[rr]
        out[0, i] = yl
        out[1, i] = yr
        a = 1.0 - np.exp(-2.0 * np.pi * lp[i] / 48000.0)
        lpl += a * (yl - lpl)
        lpr += a * (yr - lpr)
        hpl += hp_c * (lpl - hpl)
        hpr += hp_c * (lpr - hpr)
        fl = lpl - hpl
        fr = lpr - hpr
        # ping-pong: input -> left line; left echoes feed the right line and back
        vl = x[i] + fb * fr
        vr = fb * fl
        bl[w] = np.tanh(drive * vl) / drive
        br[w] = np.tanh(drive * vr) / drive
        w += 1
        if w >= size:
            w = 0


def dub_delay(x, delay_sec, feedback=0.6, lp=2500.0, hp=220.0, drive=1.3, right_sec=None):
    """Stereo ping-pong delay (wet only); x stereo (2, n); lp scalar or per-sample array."""
    mono = np.ascontiguousarray((x[0] + x[1]) * 0.5, dtype=np.float64)
    n = mono.shape[0]
    lpa = np.ascontiguousarray(np.broadcast_to(np.asarray(lp, float), (n,)), dtype=np.float64)
    out = np.zeros((2, n))
    hp_c = 1.0 - np.exp(-2.0 * np.pi * hp / SR)
    dl = int(round(delay_sec * SR))
    dr = int(round((right_sec or delay_sec) * SR))
    _dub_delay(mono, dl, dr, float(feedback), lpa, float(hp_c), float(drive), out)
    return out.astype(np.float32)
