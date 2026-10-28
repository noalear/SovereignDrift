# synth_orch.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Orchestral sampler and hybrid ("trailer") sound design for the epic
# orchestral remix of song 7 (song7_orchestral.py).  Builds on synth.py
# (oscillators, SVF filter, Bus) without changing it.
#
# Samples: VSCO 2 Community Edition by Versilian Studios (Sam Gossner, Simon
# Dalzell, Elan Hickler), CC0 1.0 Universal / public domain,
# https://github.com/sgossner/VSCO-2-CE.  Read from VSCO_ROOT (default
# /tmp/claude-0/samples/VSCO-2-CE) as plain data with soundfile.  The Upright
# Piano folder carries its own redistribution note, so it is not used.
#
#   Sampler        multi-sample instruments: nearest sample per note, pitch
#                  measured once from the audio (index cached in the temp dir),
#                  resampled with soxr, equal-power velocity crossfade between
#                  the two nearest dynamic layers, round robins, crossfade
#                  looping for long notes, legato (attack skipped), release
#   perc()         one-shot percussion groups by dynamic (bass drum, giant
#                  ethnic drum, tenor drums, snare, cymbals, gong, timpani)
#   braam()        detuned saw/pulse "brass" low hit with filter bloom
#   boom()         sub drop tuned to C1
#   taiko_synth()  synthesized drum body layered under the sampled taikos
#   pulse()        filtered saw pulse for the synth ostinato
#   choir()        formant "aah" pad (subtle hybrid layer)
#   whoosh()       swept noise transition
#   hall_ir()      designed stereo hall impulse response (early reflections +
#                  frequency-dependent exponential tail)
import os
import re
import json
import glob
import tempfile
import numpy as np
import soundfile as sf
import soxr
import scipy.signal as ss

import synth as S
from synth import SR, hz

ROOT = os.environ.get('VSCO_ROOT', '/tmp/claude-0/samples/VSCO-2-CE')
SRC_SR = 44100

ARTS = {
    'horn_sus': 'Brass/F Horn/sus', 'horn_stac': 'Brass/F Horn/stac',
    'tbn_sus': 'Brass/Tenor Trombone/sus', 'tbn_stac': 'Brass/Tenor Trombone/stac',
    'tpt_sus': 'Brass/Trumpet/sus', 'tpt_stac': 'Brass/Trumpet/stac', 'tpt_vib': 'Brass/Trumpet/susvib',
    'tuba_sus': 'Brass/Tuba/sus', 'tuba_stac': 'Brass/Tuba/stac',
    'vln_sus': 'Strings/Violin Section/susVib', 'vln_spic': 'Strings/Violin Section/Spic',
    'vln_trem': 'Strings/Violin Section/Trem',
    'vla_sus': 'Strings/Viola Section/susvib', 'vla_spic': 'Strings/Viola Section/spic',
    'vc_sus': 'Strings/Cello Section/susvib', 'vc_spic': 'Strings/Cello Section/spic',
    'vc_trem': 'Strings/Cello Section/trem',
    'cb_sus': 'Strings/Solo Contrabass/SusNV', 'cb_spic': 'Strings/Solo Contrabass/Spic',
    'picc_sus': 'Woodwinds/Piccolo/Sus', 'fl_sus': 'Woodwinds/Flute/susvib', 'ob_sus': 'Woodwinds/Oboe/Vib',
    'cl_sus': 'Woodwinds/Clarinet/susLong', 'bsn_sus': 'Woodwinds/Bassoon/sus',
    'vln_pizz': 'Strings/Violin Section/Pizz', 'vla_pizz': 'Strings/Viola Section/pizz',
    'vc_pizz': 'Strings/Cello Section/pizzT', 'tpt_mute': 'Brass/Trumpet/straightM-sus',
}
SUSTAINED = {k for k in ARTS if k.endswith(('_sus', '_vib', '_trem', '_mute'))}

_NOTE = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
_PAT = re.compile(r'_([A-G])(#?)(-?\d)(?:_|$)')


def _parse(fn):
    b = os.path.splitext(fn)[0]
    mm = _PAT.search(b + '_')
    if not mm:
        return None
    midi = 12 * (int(mm.group(3)) + 1) + _NOTE[mm.group(1)] + (1 if mm.group(2) else 0)
    v = re.search(r'_v(\d+)', b)
    rr = re.search(r'_(?:rr|RR|r)(\d+)', b)
    return midi, int(v.group(1)) if v else 1, int(rr.group(1)) if rr else 1


def _detect(path, named):
    import librosa
    x, sr = sf.read(path, dtype='float32', always_2d=True)
    y = x.mean(1)
    a, b = int(0.08 * sr), min(len(y), int(1.6 * sr))
    seg = y[a:b] if b - a > 4096 else y
    f0n = librosa.midi_to_hz(named)
    fmin, fmax = max(25, f0n / 4.5), min(sr / 4, f0n * 4.5)
    fl = 4096 if fmin < 60 else 2048
    f = librosa.yin(seg, fmin=fmin, fmax=fmax, sr=sr, frame_length=fl)
    rms = librosa.feature.rms(y=seg, frame_length=fl, hop_length=512)[0]
    k = min(len(f), len(rms))
    good = rms[:k] > 0.3 * rms[:k].max()
    det = float(np.median(librosa.hz_to_midi(f[:k][good]))) if good.sum() >= 3 else None
    return det, float(np.sqrt(np.mean(x ** 2)))


def _fine_cents(path, midi, short):
    """Fine tuning in cents from the harmonic peaks (parabolic interpolation
    on a zero-padded FFT of the steady part)."""
    x, sr = sf.read(path, dtype='float32', always_2d=True)
    y = x.mean(1)
    env = np.abs(y)
    i0 = int(np.argmax(env > env.max() * 0.004))
    a, b = (i0 + int(0.04 * sr), i0 + int(0.22 * sr)) if short else (i0 + int(0.3 * sr), i0 + int(1.5 * sr))
    s = y[a:b]
    if len(s) < 2048:
        return None
    n = 1 << int(np.ceil(np.log2(len(s) * 8)))
    Y = np.abs(np.fft.rfft(s * np.hanning(len(s)), n))
    f = np.fft.rfftfreq(n, 1 / sr)
    f0 = 440 * 2 ** ((midi - 69) / 12)
    est, w = [], []
    for k in range(1, 9):
        fk = f0 * k
        if fk > sr / 2 - 500:
            break
        lo, hi = np.searchsorted(f, [fk * 2 ** (-0.6 / 12), fk * 2 ** (0.6 / 12)])
        if hi - lo < 3:
            continue
        j = lo + int(np.argmax(Y[lo:hi]))
        if 0 < j < len(Y) - 1:
            a1, b1, c1 = (np.log(Y[j + d] + 1e-12) for d in (-1, 0, 1))
            p = 0.5 * (a1 - c1) / (a1 - 2 * b1 + c1)
            est.append(1200 * np.log2((j + p) * (f[1] - f[0]) / fk))
            w.append(Y[j])
    if not est or sum(w) <= 0:
        return None
    est, w = np.array(est), np.array(w)
    keep = np.abs(est - np.median(est)) < 25
    if w[keep].sum() <= 0:
        return None
    return float(np.average(est[keep], weights=w[keep]))


def build_index(keys=None):
    """Scan the sample folders and measure each sample's pitch.  The VSCO
    file names are an octave low (C3 = middle C); the offset is voted per
    articulation from the measured pitches, and the measured cents are kept
    as a fine-tuning correction."""
    out = {}
    for key, d in ARTS.items():
        if keys is not None and key not in keys:
            continue
        rows = []
        for fn in sorted(os.listdir(os.path.join(ROOT, d))):
            p = _parse(fn) if fn.lower().endswith('.wav') else None
            if not p:
                continue
            det, rms = _detect(os.path.join(ROOT, d, fn), p[0])
            rows.append(dict(f=os.path.join(d, fn), named=p[0], v=p[1], rr=p[2], det=det, rms=rms))
        votes = {}
        for r in rows:
            if r['det'] is not None:
                o = int(round((r['det'] - r['named']) / 12)) * 12
                votes[o] = votes.get(o, 0) + 1
        off = max(votes, key=votes.get)
        for r in rows:
            r['midi'] = r['named'] + off
            r['cents'] = 0.0
            if r['det'] is not None and abs(r['det'] - r['midi']) < 0.5:
                r['cents'] = round((r['det'] - r['midi']) * 100, 1)
            c2 = _fine_cents(os.path.join(ROOT, r['f']), r['midi'], key not in SUSTAINED)
            r['cents2'] = round(c2, 1) if c2 is not None else r['cents']
        out[key] = rows
    return out


def load_index():
    """Index of all articulations, measured once and cached in the temp dir."""
    path = os.path.join(tempfile.gettempdir(), 'sf_vsco_orch_index.json')
    idx = {}
    if os.path.exists(path):
        with open(path) as fh:
            idx = json.load(fh)
    missing = [k for k in ARTS if k not in idx or 'cents2' not in idx[k][0]]
    if missing:
        idx.update(build_index(missing))
        with open(path, 'w') as fh:
            json.dump(idx, fh)
    return idx


from collections import OrderedDict

_raw = OrderedDict()
_RAW_BUDGET = 450e6     # bytes kept in the sample cache (LRU)


def load_raw(rel):
    """Sample as float32 (2, n) at 44.1 kHz, leading silence trimmed."""
    if rel in _raw:
        _raw.move_to_end(rel)
    else:
        x, sr = sf.read(os.path.join(ROOT, rel), dtype='float32', always_2d=True)
        assert sr == SRC_SR, (rel, sr)
        if x.shape[1] == 1:
            x = np.repeat(x, 2, 1)
        x = x[:, :2].T
        env = np.abs(x).max(0)
        pk = env.max() + 1e-9
        i0 = int(np.argmax(env > pk * 0.004))
        i0 = max(0, i0 - int(0.002 * SRC_SR))
        _raw[rel] = np.ascontiguousarray(x[:, i0:])
        while sum(v.nbytes for v in _raw.values()) > _RAW_BUDGET and len(_raw) > 1:
            _raw.popitem(last=False)
    return _raw[rel]


def _extend(x, need, rng):
    """Crossfade-loop the steady part of a sustained sample to `need` frames."""
    n = x.shape[1]
    if n >= need:
        return x
    a = int(min(1.2 * SRC_SR, n * 0.3))
    b = n - int(min(1.6 * SRC_SR, n * 0.25))
    xf = int(min(0.5 * SRC_SR, (b - a) * 0.4))
    res = x[:, :b]
    while res.shape[1] < need:
        st = a + int(rng.random() * (b - a - xf) * 0.3)
        piece = x[:, st:b]
        u = np.linspace(0, np.pi / 2, xf, dtype=np.float32)
        head = res[:, -xf:] * np.cos(u) + piece[:, :xf] * np.sin(u)
        res = np.concatenate([res[:, :-xf], head, piece[:, xf:]], 1)
    return res


def _resample(seg, ratio):
    """Pitch by `ratio` (>1 = up) and convert 44.1 kHz -> SR."""
    y = soxr.resample(np.ascontiguousarray(seg.T), SRC_SR * ratio, SR, quality='HQ')
    return np.ascontiguousarray(y.T, dtype=np.float32)


class Sampler:
    def __init__(self, seed=7):
        # drop samples whose measured pitch disagrees with their name by more
        # than a quarter tone (a few VSCO files are mislabelled, e.g. the
        # clarinet "F#5" sounds F5, some oboe/pizzicato files an octave off)
        self.idx = {a: [r for r in rows if r['det'] is None or abs(r['det'] - r['midi']) <= 0.5]
                    for a, rows in load_index().items()}
        self.rng = np.random.default_rng(seed)
        self.rrpos = {}
        self.norm = {}

    def _norm(self, art):
        """Gain that puts the loudest layer's typical 100 ms peak RMS at -14 dBFS."""
        if art not in self.norm:
            rows = self.idx[art]
            top = max(r['v'] for r in rows)
            lv = []
            for r in rows:
                if r['v'] != top:
                    continue
                x = load_raw(r['f']).mean(0)
                w = int(0.1 * SRC_SR)
                k = len(x) // w
                if k < 1:
                    continue
                e = np.sqrt(np.mean(x[:k * w].reshape(k, w) ** 2, 1))
                lv.append(e.max())
            self.norm[art] = float(10 ** (-14 / 20) / (np.median(lv) + 1e-9))
        return self.norm[art]

    def layers(self, art):
        return sorted({r['v'] for r in self.idx[art]})

    def _pick(self, art, v, midi):
        rows = [r for r in self.idx[art] if r['v'] == v]
        d = [abs(r['midi'] + r['cents2'] / 100 - midi) for r in rows]
        best = min(d)
        cands = [r for r, dd in zip(rows, d) if dd - best < 0.01]
        k = (art, v, cands[0]['midi'])
        i = self.rrpos.get(k, 0)
        self.rrpos[k] = i + 1
        return cands[i % len(cands)]

    def note(self, art, midi, dur, vel=0.8, rel=0.25, attack=0.0, legato=False, detune=0.0):
        """One note -> float32 (2, n).  vel 0..1 crossfades the dynamic layers."""
        L = self.layers(art)
        vel = float(np.clip(vel, 0.0, 1.0))
        pos = vel * (len(L) - 1)
        i0 = int(np.floor(pos))
        i1 = min(i0 + 1, len(L) - 1)
        w = pos - i0
        parts = [(L[i0], np.cos(w * np.pi / 2))]
        if i1 != i0 and w > 0.02:
            parts.append((L[i1], np.sin(w * np.pi / 2)))
        n_out = int((dur + 5 * rel + 0.03) * SR)
        out = np.zeros((2, n_out), np.float32)
        skip = 0.07 if legato else 0.0
        for v, wt in parts:
            r = self._pick(art, v, midi)
            x = load_raw(r['f'])
            ratio = 2 ** ((midi + detune - r['midi'] - r['cents2'] / 100) / 12)
            off = int(skip * SRC_SR * ratio)
            need = int(n_out * ratio * SRC_SR / SR) + 64 + off
            if art in SUSTAINED and x.shape[1] < need:
                x = _extend(x, need, self.rng)
            y = _resample(x[:, off:need], ratio)
            k = min(n_out, y.shape[1])
            out[:, :k] += y[:, :k] * np.float32(wt)
        t = np.arange(n_out, dtype=np.float32) / SR
        env = np.ones(n_out, np.float32)
        a = max(attack, 0.03 if legato else 0.002)
        env *= np.minimum(t / a, 1.0)
        rl = t > dur
        env[rl] *= np.exp(-(t[rl] - dur) / rel)
        nf = min(int(0.01 * SR), n_out)
        env[-nf:] *= np.linspace(1, 0, nf, dtype=np.float32)
        g = self._norm(art) * (0.55 + 0.45 * vel if len(L) > 1 else vel ** 1.1)
        return out * env * np.float32(g)


# ------------------------------------------------------------ percussion

_DYN = {'pppp': 0, 'ppp': 1, 'pp': 2, 'p': 3, 'mp': 4, 'mf': 5, 'f': 6, 'ff': 7, 'fff': 8}
PERC = {
    'bd': ('Percussion/BDrumNewhit_v*_rr*_Sum.wav', r'_v(\d)_'),
    'taiko': ('VSCO 1 Percussion/drums/other/ethnic/giant/mallet/EthnicLargeMallet_hit_*.wav', 'dyn'),
    'taiko_hand': ('VSCO 1 Percussion/drums/other/ethnic/giant/hand/EthnicLargeHand_hit_*.wav', 'dyn'),
    'taiko_stick': ('VSCO 1 Percussion/drums/other/ethnic/giant/sticks/EthnicLargeSticks_hit_*.wav', 'dyn'),
    'tenor_lo': ('VSCO 1 Percussion/drums/tenor/tenor_lower/tenor_*.wav', 'dyn'),
    'tenor_hi': ('VSCO 1 Percussion/drums/tenor/tenor_higher/tenorH_*.wav', 'dyn'),
    'snare': ('Percussion/Snare2-HitSN_v*_rr*_Sum.wav', r'_v(\d)_'),
    'crash': ('Percussion/cymbal-crash1_*_rr*.wav', 'dyn'),
    'clash': ('VSCO 1 Percussion/varMetal/Cymbals/clash/crash_hit_ff_*.wav', None),
    'sus_hit': ('Percussion/susCymb1-hit_*_rr*.wav', 'dyn'),
    'gong': ('Percussion/gongHit_*.wav', 'dyn'),
    'timp_roll': ('Percussion/Timpani/Rolls/Timpani*_Roll_v*_rr1_Sum.wav', None),
}
TIMP_PITCH = {1: 41.45, 2: 46.75, 3: 49.5, 4: 52.45, 5: 54.55}   # measured principal modes
_perc_groups = {}
_perc_rr = {}


def _group(name):
    if name not in _perc_groups:
        pat, how = PERC[name]
        files = sorted(glob.glob(os.path.join(ROOT, pat)))
        lv = {}
        for f in files:
            b = os.path.basename(f)
            if how == 'dyn':
                mm = re.search(r'_(pppp|ppp|pp|p|mp|mf|fff|ff|f)(?:_|\.wav)', b)
                key = _DYN[mm.group(1)]
            elif how:
                key = int(re.search(how, b).group(1))
            else:
                key = 0
            lv.setdefault(key, []).append(os.path.relpath(f, ROOT))
        _perc_groups[name] = [lv[k] for k in sorted(lv)]
    return _perc_groups[name]


_hit_cache = OrderedDict()
_HIT_BUDGET = 200e6


def _hit_file(rel, semis=0.0, maxlen=None):
    key = (rel, round(semis, 3), maxlen)
    if key in _hit_cache:
        _hit_cache.move_to_end(key)
    else:
        x = load_raw(rel)
        ratio = 2 ** (semis / 12)
        if maxlen:
            x = x[:, :int(maxlen * SRC_SR * ratio)]
        y = _resample(x, ratio)
        if maxlen:
            nf = min(int(0.05 * SR), y.shape[1])
            y[:, -nf:] *= np.linspace(1, 0, nf, dtype=np.float32)
        _hit_cache[key] = y
        while sum(v.nbytes for v in _hit_cache.values()) > _HIT_BUDGET and len(_hit_cache) > 1:
            _hit_cache.popitem(last=False)
    return _hit_cache[key]


_gain = {}


def _group_gain(name, files):
    """Scale a whole group so its loudest level peaks at 0.7 (keeps dynamics)."""
    if name not in _gain:
        pk = max(float(np.abs(load_raw(f)).max()) for f in files)
        _gain[name] = 0.7 / (pk + 1e-9)
    return _gain[name]


def perc(name, vel=0.8, semis=0.0, maxlen=None):
    """One-shot from a percussion group; vel 0..1 picks the dynamic level."""
    g = _group(name)
    gain = _group_gain(name, g[-1])
    lvl = int(round(np.clip(vel, 0, 1) * (len(g) - 1)))
    files = g[lvl]
    i = _perc_rr.get((name, lvl), 0)
    _perc_rr[(name, lvl)] = i + 1
    return _hit_file(files[i % len(files)], semis, maxlen) * np.float32(gain)


def perc_file(rel, semis=0.0, maxlen=None):
    return _hit_file(rel, semis, maxlen)


def timpani(midi, vel=0.8):
    """Pitched timpani hit: nearest drum, shifted to the note."""
    drum = min(TIMP_PITCH, key=lambda k: abs(TIMP_PITCH[k] - midi) + (0.5 if TIMP_PITCH[k] < midi else 0))
    layer = 'v4' if vel > 0.7 else ('v3' if vel > 0.35 else 'v1')
    if drum == 1 and layer == 'v4':
        layer = 'v3'
    files = sorted(glob.glob(os.path.join(ROOT, f'Percussion/Timpani/Timpani{drum}_Hit_{layer}_rr*_Sum.wav')))
    i = _perc_rr.get(('timp', drum, layer), 0)
    _perc_rr[('timp', drum, layer)] = i + 1
    rel = os.path.relpath(files[i % len(files)], ROOT)
    gain = _group_gain('timp', [os.path.relpath(f, ROOT) for f in
                                glob.glob(os.path.join(ROOT, 'Percussion/Timpani/Timpani*_Hit_v4_*.wav'))])
    return _hit_file(rel, midi - TIMP_PITCH[drum], maxlen=4.0) * np.float32(gain)


# ------------------------------------------------------- hybrid synths

def _rng():
    return S.rng()


def braam(midis, dur, vel=1.0, bloom=0.18, cut_max=2600.0):
    """Trailer 'braam': detuned saws + pulses per note, slow filter bloom,
    pitch scoop at the onset, saturation.  Returns (2, n)."""
    n = int((dur + 1.2) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    scoop = -0.7 * np.exp(-t / 0.09)
    for md in midis:
        for det, p in ((-0.13, -0.8), (-0.05, -0.3), (0.05, 0.3), (0.13, 0.8)):
            f = hz(md + det + scoop)
            w = S.saw(f, n, _rng().random()) * 0.7
            w += (S.saw(f, n, 0.0) - S.saw(f, n, 0.42)) * 0.35      # pulse
            L += w * np.sqrt((1 - p) / 2)
            R += w * np.sqrt((1 + p) / 2)
    env_f = (1 - np.exp(-t / bloom)) * np.exp(-np.maximum(t - 0.25, 0) / 1.4)
    fc = 120 + cut_max * env_f
    y = S.svf(np.stack([L, R]), fc, 1.3)
    y = np.tanh(2.2 * y / np.sqrt(len(midis) * 4)) * 0.8
    e = S.env_adsr(n, dur, 0.02, 0.6, 0.75, 0.45)
    return (y * e * vel).astype(np.float32)


def boom(vel=1.0, f_end=32.70, length=3.2):
    """Sub drop that settles on C1 (the key's root)."""
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + 85 * np.exp(-t / 0.07) + 300 * np.exp(-t / 0.006)
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.8) * np.minimum(t / 0.0015, 1)
    y = np.tanh(1.6 * y)
    click = S.butter(_rng().standard_normal(n), 900, 'low', 2) * np.exp(-t / 0.012) * 0.25
    y = y + click
    y[-int(0.1 * SR):] *= np.linspace(1, 0, int(0.1 * SR))
    return np.stack([y, y]).astype(np.float32) * vel


def taiko_synth(vel=1.0, f0=65.41):
    n = int(1.4 * SR)
    t = np.arange(n) / SR
    f = f0 * (1 + 0.9 * np.exp(-t / 0.025))
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.12)
    body += 0.35 * np.sin(2 * np.pi * np.cumsum(f * 1.59) / SR) * np.exp(-t / 0.15)
    skin = S.butter(_rng().standard_normal(n), [180, 1600], 'band', 2) * np.exp(-t / 0.045) * 0.8
    y = np.tanh(1.8 * (body + skin) * np.minimum(t / 0.001, 1))
    return np.stack([y, y]).astype(np.float32) * vel * 0.6


def pulse(midi, dur, vel=1.0, cut=900.0, env=2200.0, decay=0.07):
    """Filtered saw/pulse for the 16th ostinato; stereo by L/R detune."""
    n = int((dur + 0.08) * SR)
    t = np.arange(n) / SR
    out = []
    for det in (-0.06, 0.06):
        f = hz(midi + det)
        w = S.saw(f, n, _rng().random()) * 0.6 + (S.saw(f, n, 0.0) - S.saw(f, n, 0.3)) * 0.3
        if midi >= 48:
            w += S.sine(hz(midi - 12), n) * 0.35      # sub octave only above C3
        out.append(w)
    fc = cut + env * np.exp(-t / decay)
    y = S.svf(np.stack(out), fc, 1.4)
    e = np.minimum(t / 0.002, 1) * np.where(t < dur, 1.0, np.exp(-(t - dur) / 0.02))
    return (y * e * vel * 0.5).astype(np.float32)


_FORM = [(730, 1.0, 80), (1090, 0.5, 90), (2440, 0.25, 120), (3300, 0.12, 150)]   # 'aah'


def choir(midi, dur, vel=1.0, attack=0.6, voices=5):
    n = int((dur + 1.2) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for i in range(voices):
        p = -0.8 + 1.6 * i / max(voices - 1, 1)
        vib = 0.12 * np.sin(2 * np.pi * (5.0 + 0.4 * i) * t + _rng().random() * 6.28)
        w = S.saw(hz(midi + (i - voices // 2) * 0.07 + vib), n, _rng().random())
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    src = np.stack([L, R]) / voices
    y = np.zeros_like(src)
    for fc, g, bw in _FORM:
        y += S.svf(src, fc, fc / bw, 'bp') * g
    y += S.butter(_rng().standard_normal((2, n)), [3000, 7000], 'band', 2) * 0.01   # breath
    e = S.env_adsr(n, dur, attack, 1.0, 0.95, 0.7)
    return (y * e * vel * 1.6).astype(np.float32)


def whoosh(length, f0=250.0, f1=6000.0):
    """Noise swept up then down with a bell-shaped level and a L->R pan."""
    n = int(length * SR)
    u = np.linspace(0, 1, n)
    shape = np.sin(np.pi * u ** 0.7) ** 2
    fc = f0 * (f1 / f0) ** np.sin(np.pi * u ** 0.8)
    x = _rng().standard_normal(n)
    y = S.svf(x, fc, 1.2, 'bp') * shape
    p = -0.8 + 1.6 * u
    return np.stack([y * np.sqrt((1 - p) / 2), y * np.sqrt((1 + p) / 2)]).astype(np.float32)


# --------------------------------------------------------------- space

def seat(y, pan, width=0.6):
    """Place a stereo sample on the stage: narrow it, then equal-power pan."""
    m = (y[0] + y[1]) * 0.5
    s = (y[0] - y[1]) * 0.5 * width
    a = (pan + 1) * np.pi / 4
    return np.stack([(m + s) * np.cos(a) * np.sqrt(2), (m - s) * np.sin(a) * np.sqrt(2)]).astype(np.float32)


def hall_ir(length=4.2, rt_low=3.3, rt_mid=2.7, rt_high=1.3, predelay=0.022, seed=3):
    """Designed concert-hall impulse response (2, n): sparse early reflections
    and a decorrelated noise tail whose decay time falls with frequency."""
    rng = np.random.default_rng(seed)
    n = int(length * SR)
    t = np.arange(n) / SR
    ir = np.zeros((2, n))
    bands = [(20, 250, rt_low), (250, 1000, (rt_low + rt_mid) / 2), (1000, 3000, rt_mid),
             (3000, 7000, (rt_mid + rt_high) / 2), (7000, 16000, rt_high * 0.8)]
    for ch in range(2):
        tail = np.zeros(n)
        for lo, hi, rt in bands:
            nz = S.butter(rng.standard_normal(n), [lo, hi], 'band', 2)
            tail += nz * np.exp(-6.91 * t / rt)
        tail *= np.clip((t - predelay) / 0.05, 0, 1) ** 1.5
        ir[ch] = tail
    for k in range(18):
        dt = predelay * 0.4 + rng.random() * 0.075
        g = 0.9 * (1 - dt / 0.12) * (0.5 + 0.5 * rng.random())
        ch = k % 2
        i = int(dt * SR)
        ir[ch, i] += g * (1 if rng.random() > 0.3 else -1)
        ir[1 - ch, i + int(0.0007 * SR)] += g * 0.5
    ir /= np.sqrt(np.sum(ir ** 2) / 2)
    return ir.astype(np.float32)


def convolve(x, ir):
    """Stereo convolution, channel by channel (float32 out, length of x)."""
    out = np.zeros_like(x, dtype=np.float32)
    for ch in range(2):
        out[ch] = ss.oaconvolve(x[ch], ir[ch], mode='full')[: x.shape[1]].astype(np.float32)
    return out
