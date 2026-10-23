# synth_ambient.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Instruments and effects for the space-ambient / downtempo remix of
# sf-music1 (tools/music/edm/song1_ambient.py).  Builds on synth.py, which is
# not modified here.
#
#   Sampler        a small multisample player for the VSCO-2 Community Edition
#                  WAVs (CC0 1.0, Versilian Studios): nearest sampled note,
#                  resampled to pitch, velocity layer by dynamic, attack and
#                  release envelope, crossfade loop for notes longer than the
#                  sample.  Octave offsets of the file names were checked by
#                  pitch detection (most VSCO folders name middle C "C3"; the
#                  harp names it "C4").
#   synths         glass (soft FM bell for the harp-figure arpeggio), legato
#                  mono lead with glide and delayed vibrato, formant "aah"
#                  choir, warm pad voice, soft tuned kick, sub bass
#   effects        convolution hall with a synthesized impulse response
#                  (frequency-dependent decay), shimmer (reverb -> +12 st
#                  pitch shift -> reverb), slow filter LFO
import glob
import os
import re

import numpy as np
import scipy.signal as ss
import soundfile as sf

import synth as S
from synth import SR, hz

VSCO = os.environ.get('VSCO_DIR', '/tmp/claude-0/samples/VSCO-2-CE') + '/'
_NOTE = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7,
         'G#': 8, 'A': 9, 'A#': 10, 'B': 11}


def _load_wav(path):
    """Stereo float32 at SR, leading silence trimmed."""
    x, sr = sf.read(path, dtype='float32', always_2d=True)
    x = x.T
    if x.shape[0] == 1:
        x = np.vstack([x, x])
    if sr != SR:
        g = np.gcd(SR, sr)
        x = ss.resample_poly(x, SR // g, sr // g, axis=1).astype(np.float32)
    env = np.max(np.abs(x), axis=0)
    pk = env.max() + 1e-9
    i0 = int(np.argmax(env > pk * 0.01))
    i0 = max(0, i0 - int(0.002 * SR))
    return np.ascontiguousarray(x[:, i0:])


def _norm_rms(x, seconds=1.5, target_db=-18.0):
    seg = x[:, : int(seconds * SR)]
    r = np.sqrt(np.mean(seg.astype(np.float64) ** 2)) + 1e-9
    return x * np.float32(10 ** (target_db / 20) / r)


class Sampler:
    """Multisample player for one VSCO articulation folder."""

    def __init__(self, pattern, offset=12, normalize=True, loop_from=0.35, loop_to=0.85):
        self.zones = {}
        for f in sorted(glob.glob(VSCO + pattern)):
            name = os.path.basename(f)
            mm = re.search(r'_([A-G]#?)(-?\d)(?=[_.])', name)
            if not mm:
                continue
            midi = 12 * (int(mm.group(2)) + 1) + _NOTE[mm.group(1)] + offset
            lv = re.search(r'_v(\d+)', name)
            lv = int(lv.group(1)) if lv else 1
            self.zones.setdefault(midi, {}).setdefault(lv, []).append(f)
        assert self.zones, pattern
        self.roots = np.array(sorted(self.zones))
        self.normalize = normalize
        self.loop = (loop_from, loop_to)
        self._wav = {}
        self._rr = 0

    def _get(self, f):
        if f not in self._wav:
            x = _load_wav(f)
            self._wav[f] = _norm_rms(x) if self.normalize else x
        return self._wav[f]

    def note(self, midi, dur, vel=0.8, layer=None, attack=0.0, release=0.35, tune=0.0):
        """Return (2, n) float32: dur seconds of gate, then an exponential release."""
        root = int(self.roots[np.argmin(np.abs(self.roots - midi) + 0.01 * (self.roots > midi))])
        lays = sorted(self.zones[root])
        if layer is None:
            layer = lays[min(len(lays) - 1, int(vel * len(lays)))]
        else:
            layer = lays[min(range(len(lays)), key=lambda i: abs(lays[i] - layer))]
        files = self.zones[root][layer]
        self._rr += 1
        x = self._get(files[self._rr % len(files)])
        ratio = 2 ** ((midi + tune - root) / 12.0)
        n_out = int((dur + 5 * release) * SR) + 1
        need = int(n_out * ratio) + 2
        if need > x.shape[1]:
            x = self._looped(x, need)
        pos = np.arange(n_out) * ratio
        i = pos.astype(np.int64)
        fr = (pos - i).astype(np.float32)
        out = x[:, i] * (1 - fr) + x[:, i + 1] * fr
        t = np.arange(n_out) / SR
        env = np.where(t < dur, 1.0, np.exp(-(t - dur) / max(release, 1e-3)))
        if attack > 0:
            env = env * np.minimum(t / attack, 1.0)
        env[-int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
        return (out * (env * vel).astype(np.float32)).astype(np.float32)

    def _looped(self, x, need):
        n = x.shape[1]
        a, b = int(n * self.loop[0]), int(n * self.loop[1])
        xf = int(0.25 * SR)
        seg = x[:, a:b]
        out = [x[:, :b]]
        total = b
        ramp = np.linspace(0, 1, xf, dtype=np.float32)
        while total < need + xf:
            prev = out[-1]
            head = seg[:, :xf] * ramp + prev[:, -xf:] * (1 - ramp)
            out[-1] = prev[:, :-xf]
            out.append(np.concatenate([head, seg[:, xf:]], axis=1))
            total += seg.shape[1] - xf
        y = np.concatenate(out, axis=1)
        return np.concatenate([y, np.zeros((2, 4), np.float32)], axis=1)


class OneShot:
    """Unpitched or fixed-pitch one-shots (with round robin), optionally re-pitched."""

    def __init__(self, patterns, normalize=True):
        if isinstance(patterns, str):
            patterns = [patterns]
        self.files = []
        for p in patterns:
            self.files += sorted(glob.glob(VSCO + p))
        assert self.files, patterns
        self.normalize = normalize
        self._wav = {}
        self._rr = 0

    def get(self, idx=None, semis=0.0, max_len=None):
        if idx is None:
            self._rr += 1
            idx = self._rr
        f = self.files[idx % len(self.files)]
        if f not in self._wav:
            x = _load_wav(f)
            self._wav[f] = _norm_rms(x, 0.3) if self.normalize else x
        x = self._wav[f]
        if semis:
            ratio = 2 ** (semis / 12)
            n_out = int((x.shape[1] - 2) / ratio)
            pos = np.arange(n_out) * ratio
            i = pos.astype(np.int64)
            fr = (pos - i).astype(np.float32)
            x = x[:, i] * (1 - fr) + x[:, i + 1] * fr
        if max_len is not None and x.shape[1] > int(max_len * SR):
            k = int(max_len * SR)
            x = x[:, :k].copy()
            f_ = min(int(0.05 * SR), k)
            x[:, -f_:] *= np.linspace(1, 0, f_, dtype=np.float32)
        return x


# ------------------------------------------------------------ synths

def glass(midi, dur, vel=1.0, decay=1.4, bright=1.0):
    """Glassy FM bell/pluck for the harp figure: soft attack transient, long sine tail."""
    f = float(hz(midi))
    n = int((dur + 4 * decay) * SR)
    t = np.arange(n) / SR
    idx = bright * (1.6 * np.exp(-t / 0.08) + 0.25 * np.exp(-t / 0.6))
    y = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * 2.0 * f * t))
    y += 0.25 * np.sin(2 * np.pi * 3.01 * f * t) * np.exp(-t / (0.25 * decay))
    y += 0.12 * bright * np.sin(2 * np.pi * 5.43 * f * t) * np.exp(-t / 0.05)
    e = np.exp(-t / decay) * np.minimum(t / 0.0015, 1)
    e *= np.where(t < dur, 1.0, np.exp(-(t - dur) / (0.6 * decay)))
    e[-int(0.01 * SR):] *= np.linspace(1, 0, int(0.01 * SR))
    y = y * e * vel * 0.4
    d = int(0.0007 * SR)  # tiny inter-channel offset for width
    L = y
    R = np.concatenate([np.zeros(d), y[:-d]])
    return np.stack([L, R]).astype(np.float32)


def legato_line(notes, vel=1.0, glide=0.07, cut=1700.0, cut_env=1200.0, env_decay=0.35, q=0.9,
                vib_depth=0.14, vib_rate=5.0, attack=0.06, release=0.7, detune=0.07, breath=0.04,
                tail=None):
    """Mono legato synth lead.  notes: [(t_sec, midi, dur_sec)] sorted, relative to the
    first note (t of first note = 0).  Pitch glides between touching notes; gaps
    re-articulate.  Returns (2, n) float32."""
    t_end = max(t + d for t, _, d in notes)
    n = int((t_end + 5 * release) * SR) + 1
    t = np.arange(n) / SR
    pitch = np.full(n, float(notes[0][1]))
    gate = np.zeros(n)
    since = np.full(n, 10.0)    # seconds since the last note onset (filter env, vibrato)
    prev_end = None
    for k, (t0, md, d) in enumerate(notes):
        i0, i1 = int(t0 * SR), int((t0 + d) * SR)
        legato = prev_end is not None and abs(prev_end - t0) < 0.03
        if legato:
            g = int(glide * SR)
            p0 = pitch[i0 - 1]
            ramp = p0 + (md - p0) * (0.5 - 0.5 * np.cos(np.pi * np.minimum(np.arange(i1 - i0) / max(g, 1), 1)))
            pitch[i0:i1] = ramp
        else:
            pitch[i0:i1] = md
        pitch[i1:] = md
        gate[i0:i1] = 1.0
        since[i0:] = t[i0:] - t0
        prev_end = t0 + d
    # amplitude envelope: one-pole attack/release on the gate
    a_c = np.exp(-1 / (attack * SR))
    r_c = np.exp(-1 / (release * SR / 4.6))
    amp = np.empty(n)
    _env_follow(gate, amp, a_c, r_c)
    vib = vib_depth * np.clip((since - 0.35) / 0.5, 0, 1) * np.sin(2 * np.pi * vib_rate * t)
    vib += 0.03 * np.sin(2 * np.pi * 0.23 * t)
    f = hz(pitch + vib)
    w1 = S.saw(f, n, 0.1)
    w2 = S.saw(f * 2 ** (detune / 12), n, 0.6)
    w3 = S.sine(f * 0.5, n)
    x = 0.5 * w1 + 0.4 * w2 + 0.35 * w3
    x += breath * S.butter(S.rng().standard_normal(n), [1500, 5000], 'band', 1)
    fc = cut * (f / 500.0) ** 0.3 + cut_env * np.exp(-since / env_decay)
    y = S.svf(x, fc, q)
    y = y * amp * vel * 0.5
    d = int(0.011 * SR)
    R = np.concatenate([np.zeros(d), y[:-d]])
    return np.stack([y, 0.85 * y + 0.15 * R]).astype(np.float32)


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


FORMANTS = {'a': [(800, 1.0, 6), (1150, 0.6, 7), (2900, 0.22, 9), (3900, 0.12, 10)],
            'o': [(450, 1.0, 6), (800, 0.5, 7), (2830, 0.12, 9)],
            'u': [(325, 1.0, 6), (700, 0.3, 7), (2530, 0.08, 9)]}


def choir(midi, dur, vel=1.0, vowel='a', attack=0.45, release=1.1, voices=5, detune=0.14, breath=0.08):
    """Formant-filtered detuned saws: a soft synthetic 'aah' choir; (2, n)."""
    n = int((dur + 5 * release) * SR) + 1
    t = np.arange(n) / SR
    out = np.zeros((2, n))
    offs = np.linspace(-1, 1, voices)
    for i, o in enumerate(offs):
        ph = S.rng().random() * 6.28
        vib = 0.09 * np.sin(2 * np.pi * (5.2 + 0.4 * o) * t + ph) * np.clip((t - 0.2) / 0.6, 0, 1)
        w = S.saw(hz(midi + o * detune + vib), n, S.rng().random())
        p = o * 0.8
        out[0] += w * np.sqrt((1 - p) / 2)
        out[1] += w * np.sqrt((1 + p) / 2)
    out += breath * S.rng().standard_normal((2, n))
    y = np.zeros((2, n))
    for fc, g, q in FORMANTS[vowel]:
        y += g * S.svf(out, fc, q, 'bp')
    e = S.env_adsr(n, dur, attack, 0.5, 0.9, release)
    return (y * e * vel * 0.35 / np.sqrt(voices)).astype(np.float32)


def pad_voice(midi, dur, vel=1.0, attack=1.5, release=2.5, cut=1400.0, voices=7, detune=0.2):
    """Wide, soft supersaw pad voice (filter LFO is applied on the bus)."""
    return S.supersaw(midi, dur, vel, voices=voices, detune=detune, width=1.0, a=attack, d=1.5, s=0.9,
                      r=release / 4.6 * 1.5, cut=cut, q=0.6).astype(np.float32)


def soft_kick(f_end=49.0, vel=1.0, length=0.55):
    """Muffled downtempo kick tuned to f_end (Hz); no click."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + (f_end * 2.4 - f_end) * np.exp(-t / 0.045)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.24) * np.minimum(t / 0.003, 1)
    amp *= np.where(t > length - 0.08, np.clip((length - t) / 0.08, 0, 1), 1)
    y = np.tanh(1.6 * body * amp)
    knock = S.butter(S.rng().standard_normal(n), [150, 900], 'band', 1) * np.exp(-t / 0.012) * 0.25
    y = S.butter(y + knock, 1800, 'low', 2)
    return np.stack([y, y]).astype(np.float32) * vel


def sub(midi, dur, vel=1.0, attack=0.08, release=0.5):
    n = int((dur + 5 * release) * SR)
    y = S.sine(hz(midi), n)
    y = np.tanh(1.2 * y) / np.tanh(1.2)
    y = y + 0.08 * S.sine(hz(midi) * 2, n)
    e = S.env_adsr(n, dur, attack, 0.5, 1.0, release)
    y = (y * e * vel).astype(np.float32)
    return np.stack([y, y])


def warm_bass(midi, dur, vel=1.0, cut=380.0, attack=0.15, release=0.6):
    """Soft filtered saw an octave above the sub: the tuba / choir-bass warmth."""
    n = int((dur + 5 * release) * SR)
    w = 0.6 * S.saw(hz(midi), n) + 0.4 * S.saw(hz(midi + 0.07), n, 0.4)
    y = S.svf(w, cut, 0.8)
    e = S.env_adsr(n, dur, attack, 0.8, 0.85, release)
    y = (y * e * vel).astype(np.float32)
    return np.stack([y, y])


# ------------------------------------------------------------ effects

def hall_ir(t60=(5.5, 6.0, 4.2, 2.2), length=7.0, predelay=0.025, seed=11):
    """Stereo impulse response: decorrelated noise with frequency-dependent decay
    (bands <400, 400-2.5k, 2.5k-7k, >7k Hz) and a smeared early part."""
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
    ir *= np.minimum(t / 0.06, 1) ** 1.5    # soft build-up, no hard early slap
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
