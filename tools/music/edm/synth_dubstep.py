# synth_dubstep.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Extra instruments for the dubstep remix of sf-music8 (song8_dubstep.py),
# on top of tools/music/edm/synth.py (which is shared and left untouched):
#
#   Sampler      a small multi-sample player for the VSCO-2 Community Edition
#                orchestral samples (CC0 1.0, public domain): nearest sampled
#                note resampled to pitch, velocity layer by dynamic, round
#                robin, onset trim, crossfade looping for long notes, release
#   choir        synthesized "aah"/"ooh" choir: detuned saw voices with their
#                own vibrato through a vowel formant filter bank
#   square       band-limited square lead (two polyBLEP saws)
#   growl        dubstep bass voices: LFO-synced filter wobble, FM growl,
#                formant "yoi" sweep, reese, screech; multiband saturation;
#                a little width above 500 Hz, the low end mono
#   drums        a snare built from synthesis plus a VSCO snare sample, a
#                taiko built from the VSCO large ethnic drum, VSCO crashes
#
# Sample source: VSCO 2 Community Edition by Versilian Studios,
# https://github.com/sgossner/VSCO-2-CE, CC0 1.0 Universal (public domain).
# The sample files are named one octave below their sounding pitch (checked
# with librosa.pyin: VlnEns_susVib_C4 sounds C5), hence OCT = 12.
import glob
import os
import re

import numpy as np
import soundfile as sf

import synth as S
from synth import SR, hz, saw, sine, svf, butter

VSCO = '/tmp/claude-0/samples/VSCO-2-CE/'
OCT = 12
_NOTE_RE = re.compile(r'_([A-G]#?)(-?\d)_[vV](\d)')

_wav_cache = {}


def load_wav(path):
    """Stereo float32 at the file's rate, onset trimmed (5 ms before -40 dB)."""
    if path not in _wav_cache:
        x, sr = sf.read(path, dtype='float32', always_2d=True)
        x = x.T
        if x.shape[0] == 1:
            x = np.vstack([x, x])
        x = x[:2]
        a = np.max(np.abs(x), axis=0)
        thr = a.max() * 0.01
        i0 = int(np.argmax(a > thr))
        i0 = max(0, i0 - int(0.005 * sr))
        _wav_cache[path] = (np.ascontiguousarray(x[:, i0:]), sr)
    return _wav_cache[path]


def _extend(x, sr, need):
    """Crossfade-loop the sustained middle of x until it is `need` samples long."""
    n = x.shape[1]
    if n >= need:
        return x
    a, b = int(n * 0.35), int(n * 0.85)
    xf = int(min(0.25 * sr, (b - a) * 0.3))
    seg = x[:, a:b]
    out = x[:, :b].copy()
    w = np.linspace(0, 1, xf, dtype=np.float32)
    while out.shape[1] < need:
        tail = out[:, -xf:] * np.sqrt(1 - w) + seg[:, :xf] * np.sqrt(w)
        out = np.concatenate([out[:, :-xf], tail, seg[:, xf:]], axis=1)
    return out


class Sampler:
    """Multi-sample instrument from a folder of VSCO WAVs named ..._<note>_v<layer>..."""

    def __init__(self, pattern, gain=1.0, release=0.25, attack=0.003, oct=OCT):
        self.files = {}
        for p in glob.glob(VSCO + pattern):
            mm = _NOTE_RE.search(os.path.basename(p))
            if not mm:
                continue
            md = 12 * (int(mm.group(2)) + 1) + S.NOTE[mm.group(1)] + oct
            self.files.setdefault(md, {}).setdefault(int(mm.group(3)), []).append(p)
        assert self.files, pattern
        self.keys = np.array(sorted(self.files))
        self.gain = gain
        self.release = release
        self.attack = attack
        self.rr = 0

    def note(self, midi, dur, vel=0.8, release=None, attack=None, tail=True):
        """Render a note (2, n) float32; vel 0..1 picks the dynamic layer."""
        r = self.release if release is None else release
        a = self.attack if attack is None else attack
        k = int(self.keys[np.argmin(np.abs(self.keys - midi))])
        layers = sorted(self.files[k])
        li = layers[int(round(np.clip(vel, 0, 1) * (len(layers) - 1)))]
        cands = sorted(self.files[k][li])
        self.rr += 1
        x, sr = load_wav(cands[self.rr % len(cands)])
        ratio = 2 ** ((midi - k) / 12) * sr / SR
        n_out = int((dur + (5 * r if tail else r)) * SR)
        need = int(n_out * ratio) + 4
        x = _extend(x, sr, need)
        pos = np.arange(n_out) * ratio
        n_out = min(n_out, int((x.shape[1] - 2) / ratio))
        pos = pos[:n_out]
        i = pos.astype(np.int64)
        fr = (pos - i).astype(np.float32)
        y = x[:, i] * (1 - fr) + x[:, i + 1] * fr
        t = np.arange(n_out) / SR
        env = np.where(t < dur, 1.0, np.exp(-(t - dur) / max(r, 1e-3)))
        if a > 0.0035:
            env = env * np.minimum(t / a, 1.0)
        else:
            env = env * np.minimum(t / 0.002, 1.0)
        tail_n = min(n_out, int(0.02 * SR))
        env[-tail_n:] *= np.linspace(1, 0, tail_n)
        dyn = 0.55 + 0.45 * vel
        return (y * env.astype(np.float32) * np.float32(self.gain * dyn)).astype(np.float32)


class Section:
    """Several samplers as one section: each note goes to the member whose
    nearest sample is closest (the first member wins ties within `slack`),
    which fills the gaps in a single instrument's sample set."""

    def __init__(self, members, slack=2):
        self.members = members  # [(Sampler, gain)]
        self.slack = slack

    def note(self, midi, dur, vel=0.8, **kw):
        dist = [int(np.min(np.abs(s.keys - midi))) for s, _ in self.members]
        best = min(dist)
        for (s, g), d in zip(self.members, dist):
            if d <= best + (self.slack if s is self.members[0][0] else 0):
                return s.note(midi, dur, vel, **kw) * np.float32(g)
        s, g = self.members[int(np.argmin(dist))]
        return s.note(midi, dur, vel, **kw) * np.float32(g)


# --------------------------------------------------------------- synth voices

def square(freq, n, phase=0.0):
    return 0.5 * (saw(freq, n, phase) - saw(freq, n, (phase + 0.5) % 1.0))


def square_lead(midi, dur, vel=1.0, cut=3500.0, r=0.05, vib=0.0):
    """The original's Square Lead, kept as a deliberately 'retro' little voice."""
    n = int((dur + 6 * r) * SR)
    vb = S.vibrato(n, onset=0.15, depth=vib) if vib else 0.0
    y = square(hz(midi + vb), n) * 0.7 + square(hz(midi + 0.07 + vb), n, 0.3) * 0.3
    y = svf(y, cut, 0.8)
    y = y * S.env_adsr(n, dur, 0.003, 0.15, 0.7, r) * vel
    return np.stack([y, y]).astype(np.float32)


VOWELS = {  # formant (Hz, bandwidth Hz, gain)
    'a': [(800, 80, 1.0), (1150, 90, 0.55), (2900, 120, 0.16), (3900, 130, 0.08)],
    'o': [(450, 70, 1.0), (800, 80, 0.5), (2830, 100, 0.07), (3800, 120, 0.04)],
    'u': [(325, 50, 1.0), (700, 60, 0.3), (2530, 170, 0.04), (3500, 180, 0.02)],
    'e': [(400, 60, 1.0), (1600, 80, 0.4), (2700, 120, 0.2), (3300, 150, 0.12)],
}


def choir(midi, dur, vel=1.0, vowel='a', voices=6, a=0.35, r=0.7, spread=0.8, seed=None):
    """Formant-filtered ensemble 'choir' (2, n) float32."""
    g = np.random.default_rng(seed if seed is not None else int(midi * 7919 + dur * 13))
    n = int((dur + 5 * r) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for v in range(voices):
        det = g.uniform(-0.14, 0.14)
        rate = g.uniform(4.6, 5.8)
        dep = g.uniform(0.08, 0.16)
        vb = dep * np.sin(2 * np.pi * rate * t + g.random() * 6.28) * np.clip(t / 0.6, 0, 1)
        w = saw(hz(midi + det + vb), n, g.random())
        p = (v / max(voices - 1, 1) * 2 - 1) * spread
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    src = np.stack([L, R]) / np.sqrt(voices)
    out = np.zeros_like(src)
    for fc, bw, gn in VOWELS[vowel]:
        out += svf(src, fc, fc / bw, 'bp') * gn
    breath = butter(g.standard_normal((2, n)), [2500, 7000], 'band', 2) * 0.012
    out = out * 0.2 + breath
    out = butter(out, max(80.0, float(hz(midi)) * 0.7), 'high', 2)
    e = S.env_adsr(n, dur, a, 0.8, 0.9, r)
    return (out * e * vel).astype(np.float32)


# ---------------------------------------------------------------- growl bass

def lfo(n, period, shape='cos', phase=0.0):
    """0..1 LFO; period in seconds; 'cos' starts closed, 'down' starts open."""
    if period <= 0:
        return np.full(n, 0.5)
    u = (np.arange(n) / SR / period + phase) % 1.0
    if shape == 'cos':
        return 0.5 - 0.5 * np.cos(2 * np.pi * u)
    if shape == 'down':
        return (1 - u) ** 1.5
    if shape == 'up':
        return u ** 1.5
    if shape == 'tri':
        return 1 - np.abs(2 * u - 1)
    raise ValueError(shape)


def multiband_sat(y, drives=(1.3, 3.2, 2.0), xo=(250.0, 2500.0)):
    lo = butter(y, xo[0], 'low', 2)
    hi = butter(y, xo[1], 'high', 2)
    mid = y - lo - hi
    out = (np.tanh(drives[0] * lo) / drives[0] ** 0.3 +
           np.tanh(drives[1] * mid) / drives[1] ** 0.5 +
           np.tanh(drives[2] * hi) / drives[2] ** 0.5 * 0.8)
    return out


def growl(midi, dur, style='wob', period=0.2, vel=1.0, shape='cos', drive=2.5,
          ratio=1.0, index=(0.5, 5.0), vowels=None, cut=(90.0, 5000.0), q=3.0,
          width=0.35, hp=110.0, r=0.018, seed=0):
    """One dubstep bass hit (2, n) float32; period = LFO period in seconds."""
    n = int((dur + 6 * r) * SR)
    t = np.arange(n) / SR
    f = float(hz(midi))
    g = np.random.default_rng(seed)
    l = lfo(n, period, shape)
    if style == 'wob':
        src = saw(f, n, g.random()) + 0.7 * saw(f * 1.006, n, g.random()) + 0.45 * square(f * 2.0, n)
        fc = cut[0] * (cut[1] / cut[0]) ** l
        y = svf(src, fc, q, 'lp')
        y = np.tanh(drive * y)
    elif style == 'fm':
        I = index[0] + (index[1] - index[0]) * l
        mod = np.sin(2 * np.pi * f * ratio * t + 0.6 * np.sin(2 * np.pi * f * t))
        y = np.sin(2 * np.pi * f * t + I * mod) + 0.35 * saw(f, n)
        y = np.tanh(drive * y)
        y = svf(y, 250 + 7000 * l ** 1.3, 0.9, 'lp')
    elif style == 'yoi':
        # vowel morph, e.g. y -> o -> i: (position 0..1, F1, F2, F3)
        vw = vowels or [(0.0, 280, 2200, 2900), (0.35, 520, 850, 2600), (1.0, 300, 2300, 3100)]
        u = np.clip(t / max(dur, 1e-3), 0, 1)
        if period > 0:
            u = np.clip(u + 0.25 * (l - 0.5), 0, 1)
        pts = [p[0] for p in vw]
        src = np.tanh(2.0 * (saw(f, n) + 0.6 * square(f, n) + 0.3 * saw(f * 2.003, n)))
        y = np.zeros(n)
        for k, (qq, gn) in enumerate(((5.0, 1.0), (7.0, 0.85), (8.0, 0.45))):
            fk = np.interp(u, pts, [p[k + 1] for p in vw])
            y += svf(src, fk, qq, 'bp') * gn
        y = np.tanh(drive * 1.6 * y) + 0.25 * svf(src, 400, 0.7, 'lp')
    elif style == 'reese':
        src = np.zeros(n)
        for d in (-0.13, -0.05, 0.05, 0.13):
            src += saw(f * 2 ** (d / 12), n, g.random())
        fc = cut[0] * (cut[1] / cut[0]) ** l
        y = svf(src * 0.5, fc, 1.2, 'lp')
        y = np.tanh(drive * y)
    elif style == 'stab':
        # plain FM stab: the index decays once (no LFO, no vowel), fixed low-pass
        I = index[1] * np.exp(-t / 0.07) + index[0]
        y = np.sin(2 * np.pi * f * t + I * np.sin(2 * np.pi * f * ratio * t)) + 0.4 * saw(f, n)
        y = np.tanh(drive * y)
        y = svf(y, cut[1], 0.75, 'lp')
    elif style == 'saw8':
        # distorted saw octave stab: saw + saw an octave up, a short filter thump
        src = saw(f, n, g.random()) + 0.8 * saw(2 * f * 1.002, n, g.random()) + 0.4 * square(f, n)
        y = np.tanh(drive * 1.2 * src)
        y = svf(y, cut[1] + 2500 * np.exp(-t / 0.035), 0.8, 'lp')
    elif style == 'screech':
        I = index[0] + (index[1] - index[0]) * l
        y = np.sin(2 * np.pi * f * t + I * np.sin(2 * np.pi * f * ratio * t))
        y = np.tanh(drive * 2 * y)
        y = svf(y, 1200 + 2500 * l, 1.5, 'bp') * 2.5 + 0.3 * y
    else:
        raise ValueError(style)
    y = multiband_sat(y)
    y = butter(y, hp, 'high', 4)
    y = y / (np.sqrt(np.mean(y[: max(1, int(dur * SR))] ** 2)) + 1e-9) * 0.25
    e = S.env_adsr(n, dur, 0.003, 0.3, 1.0, r)
    y = y * e * vel
    # width: a short Haas spread of the band above 500 Hz only
    hi = butter(y, 500, 'high', 2)
    d = int(0.011 * SR)
    side = np.zeros(n)
    side[d:] = hi[:-d]
    side = (side - hi) * width
    return np.stack([y + side, y - side]).astype(np.float32)


# ---------------------------------------------------------------- drums

_snare_samp = None
_taiko = None
_crash = None


def round_snare(vel=1.0, seed=0):
    """Short, rounded "pdlap" snare: tuned body with a fast pitch drop, a short
    1-4 kHz snap, a tiny clap, a trimmed VSCO snare hit; ~120 ms decay, little
    above 8 kHz.  seed 0..3 gives slightly different round-robin variants."""
    global _snare_samp
    if _snare_samp is None:
        _snare_samp = [load_wav(p) for p in sorted(glob.glob(VSCO + 'Percussion/Snare2-HitSN_v[56]*'))
                       + sorted(glob.glob(VSCO + 'VSCO 1 Percussion/drums/snare/drum1/snare1_ff*'))]
    g = np.random.default_rng(100 + seed)
    n = int(0.45 * SR)
    t = np.arange(n) / SR
    tone = 196.0 * (1 + 0.03 * (seed - 1.5))
    f = tone * (1 + 0.9 * np.exp(-t / 0.006))
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.06)
    body += 0.35 * np.sin(2 * np.pi * np.cumsum(f * 1.52) / SR) * np.exp(-t / 0.04)
    snap = butter(g.standard_normal(n), [1000, 4000], 'band', 2) * np.exp(-t / 0.028) * 0.55
    out = np.stack([body + snap, body + snap]) * 0.9
    cl = S.clap(1.0)[:, :n]
    cl = cl * np.exp(-t[:cl.shape[1]] / 0.03)
    out[:, :cl.shape[1]] += cl * 0.18
    x, sr = _snare_samp[seed % len(_snare_samp)]
    m_ = min(n, int(x.shape[1] * SR / sr))
    pos = np.arange(m_) * sr / SR
    xs = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x])
    xs = xs / (np.max(np.abs(xs)) + 1e-9) * np.exp(-t[:m_] / 0.06)
    out[:, :m_] += xs * 0.5
    out = butter(out, 7500, 'low', 2)
    out = butter(out, 120, 'high', 2)
    k = int(0.01 * SR)
    out[:, -k:] *= np.linspace(1, 0, k)
    out = np.tanh(1.3 * out) / np.tanh(1.3)
    return (out / (np.max(np.abs(out)) + 1e-9) * 0.9 * vel).astype(np.float32)


def sub_drop(midi, dur, vel=1.0, drop=12.0):
    """Sine sub gliding down `drop` semitones (the classic sub drop)."""
    n = int((dur + 0.05) * SR)
    t = np.arange(n) / SR
    u = np.clip(t / dur, 0, 1)
    f = hz(midi - drop * u ** 1.6)
    y = np.sin(2 * np.pi * np.cumsum(f) / SR)
    y = np.tanh(1.3 * y) / np.tanh(1.3)
    e = np.minimum(t / 0.004, 1) * np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.02)) * (1 - 0.5 * u)
    y = y * e * vel
    return np.stack([y, y]).astype(np.float32)


def big_snare(vel=1.0, seed=0):
    """Synth body + clap + VSCO snare sample + long noise tail (2, n)."""
    global _snare_samp
    if _snare_samp is None:
        _snare_samp = [load_wav(p) for p in sorted(glob.glob(VSCO + 'Percussion/Snare2-HitSN_v[56]*'))
                       + sorted(glob.glob(VSCO + 'VSCO 1 Percussion/drums/snare/drum1/snare1_ff*'))]
    n = int(0.8 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(seed)
    out = np.zeros((2, n))
    sn = S.snare(1.0, 185.0)
    out[:, :sn.shape[1]] += sn * 0.9
    cl = S.clap(0.8)
    out[:, :cl.shape[1]] += cl * 0.7
    x, sr = _snare_samp[seed % len(_snare_samp)]
    m_ = min(n, int(x.shape[1] * SR / sr))
    pos = np.arange(m_) * sr / SR
    xs = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x])
    out[:, :m_] += xs / (np.max(np.abs(xs)) + 1e-9) * 0.9
    tail = butter(g.standard_normal((2, n)), [1500, 9000], 'band', 2) * np.exp(-t / 0.16) * 0.35
    out += tail
    body = np.sin(2 * np.pi * 180 * t * (1 + 0.5 * np.exp(-t / 0.008))) * np.exp(-t / 0.07) * 0.6
    out += body
    out = np.tanh(1.4 * out) * vel
    return out.astype(np.float32)


def taiko(vel=1.0, pitch=-3.0, seed=0):
    """VSCO large ethnic drum (hand, ff / f / mp), pitched down for weight."""
    global _taiko
    if _taiko is None:
        base = VSCO + 'VSCO 1 Percussion/drums/other/ethnic/giant/hand/'
        _taiko = {lv: [load_wav(p) for p in sorted(glob.glob(base + f'EthnicLargeHand_hit_{lv}_*'))]
                  for lv in ('mp', 'f', 'ff')}
    lv = 'ff' if vel > 0.8 else ('f' if vel > 0.5 else 'mp')
    x, sr = _taiko[lv][seed % len(_taiko[lv])]
    ratio = 2 ** (pitch / 12) * sr / SR
    m_ = int((x.shape[1] - 2) / ratio)
    pos = np.arange(m_) * ratio
    y = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x])
    y = y / (np.max(np.abs(y)) + 1e-9)
    return (y * vel).astype(np.float32)


def crash_s(vel=1.0, seed=0, reverse=False, length=None):
    global _crash
    if _crash is None:
        _crash = [load_wav(p) for p in sorted(glob.glob(VSCO + 'Percussion/cymbal-crash1_ff_rr*'))]
    x, sr = _crash[seed % len(_crash)]
    m_ = int(x.shape[1] * SR / sr)
    pos = np.arange(m_) * sr / SR
    y = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x])
    y = y / (np.max(np.abs(y)) + 1e-9)
    if length is not None:
        y = y[:, :int(length * SR)]
    if reverse:
        y = y[:, ::-1].copy()
        k = min(y.shape[1], int(0.01 * SR))
        y[:, -k:] *= np.linspace(1, 0, k)
    return (y * vel).astype(np.float32)


def swell(length):
    """VSCO suspended cymbal crescendo, time-stretched (resampled) to length."""
    x, sr = load_wav(VSCO + 'Percussion/susCymb1-cresc-Long_v1.wav')
    m_ = int(length * SR)
    pos = np.linspace(0, x.shape[1] - 2, m_)
    y = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x])
    return (y / (np.max(np.abs(y)) + 1e-9)).astype(np.float32)
