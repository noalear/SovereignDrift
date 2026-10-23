# synth_liquid.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Instruments for the liquid drum & bass remix of sf-music3 (song3_liquid.py).
# Builds on synth.py (imported, not modified) and adds:
#
#   Sampler        a small multi-sample player for the VSCO-2 Community Edition
#                  orchestral samples (CC0 1.0 / public domain): nearest sampled
#                  note, resampled to pitch, dynamic layer by velocity, gate and
#                  release envelope, round robin.  VSCO names most instruments
#                  one octave low (a "C4" flute file sounds C5); `octave` fixes it.
#   rhodes         two-operator FM electric piano with a tine transient
#   choir          formant-filtered detuned saws ("aah" choir)
#   whistle        breathy sine whistle
#   bass_line      mono legato line (glide) -> sub sine and a warm reese
#   dnb_kick, dnb_snare, shaker, open_hat
#   break_loop     a synthesized two-bar breakbeat, processed like a sampled
#                  break, plus chop() to re-sequence its eighth-note slices
#   crackle        vinyl crackle and hiss bed
import glob
import os
import re

import numpy as np
import soundfile as sf

import synth as S
from synth import SR, hz, m

VSCO = '/tmp/claude-0/samples/VSCO-2-CE'
_NOTE_RE = re.compile(r'_([A-G]#?)(\d)_')


# ------------------------------------------------------------- sampler

class Sampler:
    """Multi-sample player.

    pattern  glob below VSCO
    octave   semitone correction for VSCO's naming (+12 for most instruments)
    layer    regex group giving the dynamic layer (1 = softest), or None
    norm_s   window (s) after the onset used to normalise each file's RMS
    """

    def __init__(self, pattern, octave=0, layer=r'_v(\d)', norm_s=1.0, target_db=-18.0):
        self.zones = []  # (midi, layer, rr, data(2,n) float32)
        for f in sorted(glob.glob(os.path.join(VSCO, pattern))):
            name = os.path.basename(f)
            mm = _NOTE_RE.search(name.replace('.wav', '_'))
            if not mm:
                continue
            midi = m(mm.group(1) + mm.group(2)) + octave
            lay = 1
            if layer:
                ml = re.search(layer, name)
                lay = int(ml.group(1)) if ml else 1
            x, sr = sf.read(f, dtype='float32', always_2d=True)
            x = x.T
            if x.shape[0] == 1:
                x = np.vstack([x, x])
            x = x[:2]
            # resample the file to 48 kHz once (linear interpolation)
            if sr != SR:
                n_out = int(x.shape[1] * SR / sr)
                pos = np.arange(n_out) * (sr / SR)
                idx = np.arange(x.shape[1])
                x = np.stack([np.interp(pos, idx, c) for c in x]).astype(np.float32)
            mono = np.abs(x).max(axis=0)
            on = int(np.argmax(mono > 0.02 * mono.max()))
            on = max(0, on - int(0.004 * SR))
            x = x[:, on:]
            w = x[:, : int(norm_s * SR)]
            r = np.sqrt(np.mean(np.square(w, dtype=np.float64))) + 1e-9
            x = x * np.float32(10 ** (target_db / 20) / r)
            # fade the very end of the file
            fe = min(int(0.05 * SR), x.shape[1] // 4)
            x[:, -fe:] *= np.linspace(1, 0, fe, dtype=np.float32)
            self.zones.append((midi, lay, len(self.zones), x))
        if not self.zones:
            raise FileNotFoundError(pattern)
        self.layers = sorted(set(z[1] for z in self.zones))
        self._rr = 0
        self._cache = {}

    def _pick(self, midi, vel):
        li = min(len(self.layers) - 1, int(vel * len(self.layers)))
        lay = self.layers[li]
        cand = [z for z in self.zones if z[1] == lay]
        best = min(abs(z[0] - midi) for z in cand)
        near = [z for z in cand if abs(z[0] - midi) == best]
        self._rr += 1
        return near[self._rr % len(near)]

    def note(self, midi, dur, vel=0.8, r=0.25, a=0.0, offset=0.0, max_len=None):
        """(2, n) float32: note of `dur` s, release time constant r, optional attack a."""
        z = self._pick(midi, vel)
        key = (z[2], midi)
        if key not in self._cache:
            ratio = 2 ** ((midi - z[0]) / 12)
            src = z[3]
            n_out = int(src.shape[1] / ratio) - 2
            pos = np.arange(n_out) * ratio
            idx = np.arange(src.shape[1])
            self._cache[key] = np.stack([np.interp(pos, idx, c) for c in src]).astype(np.float32)
        x = self._cache[key]
        o = int(offset * SR)
        need = int((dur + 5 * r) * SR)
        if max_len:
            need = min(need, int(max_len * SR))
        y = x[:, o:o + need].copy()
        n = y.shape[1]
        t = np.arange(n, dtype=np.float32) / SR
        env = np.where(t < dur, 1.0, np.exp(-(t - dur) / max(r, 1e-3))).astype(np.float32)
        if a > 0:
            env *= np.minimum(t / a, 1.0)
        else:
            env *= np.minimum(t / 0.0015, 1.0)
        fe = min(int(0.01 * SR), n)
        env[-fe:] *= np.linspace(1, 0, fe, dtype=np.float32)
        return y * env * np.float32(vel)


# ------------------------------------------------------------- keys / voices

def rhodes(midi, dur, vel=0.7, r=0.3):
    """FM electric piano: 1:1 body with a decaying index (bark) plus a 14:1 tine."""
    f = float(hz(midi))
    n = int((dur + 5 * r + 0.05) * SR)
    t = np.arange(n) / SR
    i1 = (0.4 + 1.4 * vel) * np.exp(-t / 0.4) + 0.25 * vel
    body = np.sin(2 * np.pi * f * t + i1 * np.sin(2 * np.pi * f * t))
    i2 = (0.8 + 1.5 * vel) * np.exp(-t / 0.018)
    tine = np.sin(2 * np.pi * f * t + i2 * np.sin(2 * np.pi * 14.0 * f * t)) * np.exp(-t / 0.5)
    tau = 1.7 * (261.6 / f) ** 0.45
    amp = np.exp(-t / tau) * 0.85 + 0.15 * np.exp(-t / (tau * 4))
    y = (body + 0.35 * tine) * amp
    y = np.tanh((1.0 + 1.2 * vel) * y) / np.tanh(1.0 + 1.2 * vel)
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / r))
    y = y * gate * np.minimum(t / 0.001, 1) * vel * 0.4
    p = float(np.clip((midi - 62) / 24, -0.4, 0.4))
    return S.pan2(y, p)


_FORMANTS = {'a': ((800, 1150, 2900), (80, 90, 120), (1.0, 0.5, 0.18)),
             'o': ((450, 800, 2830), (70, 80, 100), (1.0, 0.35, 0.1)),
             'u': ((350, 640, 2400), (60, 70, 90), (1.0, 0.25, 0.06))}


def choir(midi, dur, vel=0.8, a=0.45, r=0.7, vowel='a', voices=6):
    """Ensemble 'aah': detuned saws with their own vibrato through three formants."""
    n = int((dur + 5 * r) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    rg = S.rng()
    for i in range(voices):
        det = (i - (voices - 1) / 2) * 0.07 + rg.normal(0, 0.02)
        vib = 0.12 * np.sin(2 * np.pi * (4.6 + rg.random()) * t + rg.random() * 6.3)
        w = S.saw(hz(midi + det + vib), n, rg.random())
        p = -0.8 + 1.6 * i / max(1, voices - 1)
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    src = np.stack([L, R]) / np.sqrt(voices)
    fs, bws, gs = _FORMANTS[vowel]
    out = np.zeros_like(src)
    for fc, bw, g in zip(fs, bws, gs):
        out += S.svf(src, fc, fc / bw, 'bp') * g
    breath = S.butter(rg.standard_normal((2, n)), [2500, 7000], 'band', 2) * 0.03
    out = out + breath
    e = S.env_adsr(n, dur, a, 1.0, 0.9, r)
    return out * e * vel * 1.6


def whistle(midi, dur, vel=0.8, a=0.05, r=0.15):
    n = int((dur + 5 * r) * SR)
    t = np.arange(n) / SR
    scoop = -0.35 * np.exp(-t / 0.04)
    vb = S.vibrato(n, onset=0.25, rate=5.6, depth=0.18)
    f = hz(midi + scoop + vb)
    ph = 2 * np.pi * np.cumsum(f) / SR
    y = np.sin(ph) + 0.06 * np.sin(2 * ph) + 0.02 * np.sin(3 * ph)
    br = S.svf(S.rng().standard_normal(n), f * 1.0, 6.0, 'bp') * 0.12
    br += S.butter(S.rng().standard_normal(n), 4000, 'high', 2) * 0.012
    e = S.env_adsr(n, dur, a, 0.3, 0.85, r)
    y = (y + br) * e * vel * 0.5
    return S.pan2(y, 0.1)


# ------------------------------------------------------------- bass

def line_traj(notes, n, glide=0.035, gap=0.012):
    """notes: (t_start_s, midi, dur_s) sorted; returns per-sample midi and gate env."""
    pitch = np.full(n, np.nan)
    env = np.zeros(n)
    prev = None
    for k, (ts, md, d) in enumerate(notes):
        i0 = max(0, int(ts * SR))
        i1 = min(n, int((ts + d) * SR))
        if i1 <= i0:
            continue
        L = i1 - i0
        u = np.arange(L) / SR
        if prev is not None and abs(prev[0] + prev[1] - ts) < 0.02:
            p0 = prev[2]
            pitch[i0:i1] = md + (p0 - md) * np.exp(-u / glide)
        else:
            pitch[i0:i1] = md
        e = np.ones(L)
        a = int(0.004 * SR)
        e[:a] = np.linspace(0, 1, a) if (prev is None or abs(prev[0] + prev[1] - ts) >= 0.02) else \
            np.linspace(0.75, 1, a)
        g = min(int(gap * SR), L // 4)
        nxt = notes[k + 1][0] if k + 1 < len(notes) else None
        if nxt is None or abs(nxt - (ts + d)) >= 0.02:
            e[-g:] *= np.linspace(1, 0, g)
        env[i0:i1] = e
        prev = (ts, d, md)
    # fill pitch gaps with the last pitch so oscillators stay continuous
    idx = np.where(~np.isnan(pitch), np.arange(n), 0)
    np.maximum.accumulate(idx, out=idx)
    first = np.argmax(~np.isnan(pitch))
    pitch[:first] = pitch[first]
    pitch = pitch[idx]
    return pitch, env


def sub_from(pitch, env, drive=1.3):
    y = S.sine(hz(pitch), len(pitch))
    y = np.tanh(drive * y) / np.tanh(drive)
    return (y * env).astype(np.float32)


def reese_from(pitch, env, t0, spb, cut=420.0, lfo_beats=2.0, depth=0.55, q=1.2):
    """Warm reese an octave above the sub: three detuned saws, LP with a slow synced LFO."""
    n = len(pitch)
    out = np.zeros((2, n))
    for side, dets in ((0, (-0.13, 0.04)), (1, (0.13, -0.05))):
        acc = np.zeros(n)
        for dt in dets:
            acc += S.saw(hz(pitch + 12 + dt), n, S.rng().random())
        acc += 0.6 * S.saw(hz(pitch + 12), n, 0.3 + side * 0.2)
        out[side] = acc
    t = t0 + np.arange(n) / SR
    lfo = 0.5 - 0.5 * np.cos(2 * np.pi * t / (lfo_beats * spb))
    fc = cut * (1 + depth * 2.5 * lfo) * (hz(pitch) / 55.0) ** 0.5
    y = S.svf(out, fc, q)
    y = S.butter(y, 70, 'high', 2)
    y = np.tanh(1.5 * y) * env
    return y.astype(np.float32) * 0.35


# ------------------------------------------------------------- drums

def dnb_kick(vel=1.0, f_end=55.0, f_start=240.0, length=0.30):
    """Tight DnB kick; f_end 55 Hz = A1, the tonic."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_start - f_end) * np.exp(-t / 0.022) + 700 * np.exp(-t / 0.0018)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.11) * np.minimum(t / 0.0006, 1)
    amp *= np.where(t > length - 0.04, np.clip((length - t) / 0.04, 0, 1), 1)
    click = S.butter(S.rng().standard_normal(n), [3000, 9000], 'band', 2) * np.exp(-t / 0.0022) * 0.45
    knock = np.sin(2 * np.pi * 1300 * t) * np.exp(-t / 0.004) * 0.25
    y = np.tanh(2.6 * body * amp) * 0.9 + click + knock
    return np.stack([y, y]) * vel


def dnb_snare(vel=1.0, tone=174.6, bright=1.0, decay=0.13):
    """Crisp snare: tuned body (F3) + second mode + bright noise + crack."""
    n = int(0.45 * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    ph = 2 * np.pi * np.cumsum(tone * (1 + 0.45 * np.exp(-t / 0.007))) / SR
    body = np.sin(ph) * np.exp(-t / 0.055) + 0.45 * np.sin(1.61 * ph) * np.exp(-t / 0.035)
    nz = rg.standard_normal(n)
    nz = S.butter(nz, [1300 * bright, 8500], 'band', 2) * (0.75 * np.exp(-t / decay) + 0.6 * np.exp(-t / 0.006))
    nz += S.butter(rg.standard_normal(n), [2500, 6500], 'band', 2) * np.exp(-t / (decay * 0.6)) * 0.55 * bright
    y = (0.75 * body + 0.9 * nz) * np.minimum(t / 0.0004, 1)
    y = np.tanh(1.8 * y) / 1.2
    L = y
    Rr = np.roll(y, 23)
    return np.stack([L, Rr]) * vel


def ghost(vel=0.25):
    s = dnb_snare(1.0, tone=180.0, bright=0.8, decay=0.06)
    return S.butter(s, 7000, 'low', 2) * vel


_PDLAP_RR = [(1.000, 1.00, 1.00), (0.985, 0.92, 0.96), (1.015, 1.07, 1.00), (0.995, 0.96, 0.93)]


def pdlap(vel=1.0, var=0, dark=0.0, f0=220.0):
    """Short, rounded, woody/tonal snare ("pdlap", not "dtschh").

    Tuned body at A3 (220 Hz, the tonic) with a fast pitch drop and a fifth mode,
    a short band-limited snap at 1-4 kHz, a tiny rim click and clap flam, ~100 ms
    decay, low-passed at 7 kHz.  var: one of 4 round-robin variants (pitch,
    snap tone, level).  dark 0..1 removes snap/clap and closes the LP (ghosts).
    """
    pm, tone, lvl = _PDLAP_RR[var % 4]
    n = int(0.22 * SR)
    t = np.arange(n) / SR
    rg = S.rng()
    f = f0 * pm * (1 + 0.55 * np.exp(-t / 0.010))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.065) + 0.35 * np.sin(1.5 * ph) * np.exp(-t / 0.040) +
            0.15 * np.sin(2.32 * ph) * np.exp(-t / 0.018))
    snap = S.butter(rg.standard_normal(n), [1000 * tone, 4000 * tone], 'band', 2)
    snap *= (np.exp(-t / 0.016) + 0.25 * np.exp(-t / 0.05)) * (1 - 0.8 * dark)
    rim = np.sin(2 * np.pi * 1650 * tone * t) * np.exp(-t / 0.004) * 0.22 * (1 - 0.7 * dark)
    clap = np.zeros(n)
    for off in (0.0, 0.007, 0.015):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.003), 0)
    clap = S.butter(rg.standard_normal(n) * clap, [1200, 3200], 'band', 2) * 0.35 * (1 - 0.9 * dark)
    y = 0.9 * body + 0.8 * snap + rim + 1.3 * clap
    y = np.tanh(1.4 * y) / 1.1 * np.minimum(t / 0.0004, 1)
    y = S.butter(y, 7000 - 4500 * dark, 'low', 2)
    fe = int(0.02 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 14)]) * vel * lvl


def pdlap_ghost(vel=0.15, var=0):
    return pdlap(vel, var, dark=1.0)


def shaker(vel=1.0, pan=0.3):
    n = int(0.09 * SR)
    t = np.arange(n) / SR
    e = np.minimum(t / 0.012, 1) * np.exp(-np.maximum(t - 0.012, 0) / 0.02)
    y = S.butter(S.rng().standard_normal(n), [5000, 12000], 'band', 2) * e
    return S.pan2(y * vel * 0.8, pan)


def open_hat(vel=1.0, pan=0.15, length=0.3):
    n = int(length * SR)
    t = np.arange(n) / SR
    src = 0.55 * S._metal(n) + 0.6 * S.rng().standard_normal(n)
    y = S.butter(src, 7000, 'high', 4) * np.exp(-t / 0.09)
    y *= np.where(t > length - 0.03, np.clip((length - t) / 0.03, 0, 1), 1)
    return S.pan2(y * vel * 0.8, pan)


def rim(vel=1.0):
    n = int(0.08 * SR)
    t = np.arange(n) / SR
    y = np.sin(2 * np.pi * 1750 * t) * np.exp(-t / 0.006) + \
        S.butter(S.rng().standard_normal(n), 2500, 'high', 2) * np.exp(-t / 0.004) * 0.6
    return S.pan2(y * vel * 0.6, -0.2)


# "break" kit: rounder kick, higher, rattly snare, hats on eighths
BREAK_A = [('k', 0, 1.0), ('h', 0, .6), ('h', 2, .5), ('k', 2, .75), ('s', 4, 1.0), ('h', 4, .55), ('h', 6, .5),
           ('g', 7, .35), ('h', 8, .55), ('g', 9, .3), ('k', 10, .85), ('h', 10, .5), ('k', 11, .5),
           ('s', 12, 1.0), ('h', 12, .55), ('h', 14, .5), ('g', 15, .35)]
BREAK_B = [('k', 0, 1.0), ('h', 0, .6), ('h', 2, .5), ('k', 2, .7), ('s', 4, 1.0), ('h', 4, .55), ('g', 6, .3),
           ('h', 6, .5), ('g', 7, .35), ('h', 8, .55), ('g', 9, .3), ('k', 10, .85), ('h', 10, .5),
           ('s', 12, 1.0), ('h', 12, .55), ('g', 14, .35), ('h', 14, .6), ('s', 15, .45)]


def break_loop(spb):
    """Two bars of a synthesized break, saturated, band-limited and roomy: (2, n) float32."""
    from pedalboard import Pedalboard, Compressor, Reverb, HighpassFilter, LowpassFilter, Distortion
    bar = 4 * spb
    n = int((2 * bar + 0.5) * SR)
    x = np.zeros((2, n))
    for bi, pat in enumerate((BREAK_A, BREAK_B)):
        for kind, step, v in pat:
            t0 = bi * bar + step * spb / 4
            if kind == 'k':
                sig = dnb_kick(v, f_end=62.0, f_start=180.0, length=0.25)
            elif kind == 's':
                sig = pdlap(v, var=step // 4, dark=0.3, f0=196.0)
            elif kind == 'g':
                sig = pdlap_ghost(v * 0.6, var=step)
            else:
                sig = S.hat(v, False, 0.0)
            i = int(t0 * SR)
            k = min(sig.shape[1], n - i)
            x[:, i:i + k] += sig[:, :k]
    board = Pedalboard([HighpassFilter(110), Compressor(threshold_db=-18, ratio=4, attack_ms=2, release_ms=60),
                        Distortion(drive_db=3), LowpassFilter(7000),
                        Reverb(room_size=0.18, damping=0.6, wet_level=0.18, dry_level=0.9, width=0.6)])
    y = board(x.astype(np.float32), SR)
    return y[:, : int(2 * bar * SR) + int(0.02 * SR)] * 0.5


def chop(loop, spb, order):
    """Re-sequence a two-bar loop by eighth-note slices; order: 8 slice indices (0..15) for one bar."""
    sl = spb / 2
    L = int(round(sl * SR))
    out = np.zeros((2, int(4 * spb * SR) + L), np.float32)
    f = int(0.0015 * SR)
    ramp_in = np.linspace(0, 1, f, dtype=np.float32)
    ramp_out = ramp_in[::-1]
    for j, s in enumerate(order):
        if s is None:
            continue
        a = int(round(s * sl * SR))
        seg = loop[:, a:a + L].copy()
        seg[:, :f] *= ramp_in
        seg[:, -f:] *= ramp_out
        i = int(round(j * sl * SR))
        out[:, i:i + seg.shape[1]] += seg
    return out


# ------------------------------------------------------------- atmos

def crackle(n, density=25.0, hiss=0.004):
    rg = S.rng()
    out = np.zeros((2, n), np.float32)
    k = int(n / SR * density)
    pos = rg.integers(0, n - 200, k)
    amp = rg.pareto(3.0, k) * 0.05
    side = rg.integers(0, 2, k)
    for p, a, s in zip(pos, amp, side):
        out[s, p] += a
        out[1 - s, p + 1] += a * 0.5
    out = S.butter(out, [900, 9000], 'band', 2).astype(np.float32)
    h = S.butter(rg.standard_normal((2, n)), 5000, 'low', 1) * hiss
    return (out + h.astype(np.float32))
