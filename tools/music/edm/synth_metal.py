# synth_metal.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Instruments for the symphonic-metal remix (song6_symphonic_metal.py), built
# on top of synth.py (which is shared and left untouched):
#
#   guitar        Karplus-Strong string model (numba) with pick-position comb,
#                 attack pitch glide, palm-mute damping and fret-hand muting;
#                 power chords strummed string by string
#   amp           high-gain amp simulation: tight pre-EQ with a mid boost
#                 (tube-screamer style), two oversampled asymmetric tube-like
#                 gain stages, tone stack, and a designed minimum-phase 4x12
#                 cabinet impulse response
#   lead guitar   one continuous monophonic phrase per call (legato glides,
#                 bends, delayed wide vibrato, pick transients) into the amp
#   bass guitar   Karplus-Strong picked bass + sine sub + parallel grit
#   drums         metal kit: triggered kick tuned to the key, layered snare,
#                 pitched toms, china, hats, ride
#   choir         formant-filtered ensemble of detuned saws ("aah"/"ooh")
#   Sampler       a small sampler for the VSCO-2 Community Edition orchestral
#                 samples (CC0 1.0): nearest sample, resampled to pitch,
#                 dynamic layers, round robins, release envelope
import os
import re
import glob
import numpy as np
import numba
import scipy.signal as ss
import soundfile as sf

import synth as S
from synth import SR, hz, butter

VSCO = '/tmp/claude-0/samples/VSCO-2-CE'


def _rng():
    return S.rng()


# ------------------------------------------------------------ biquads

def peq(x, f0, gain_db, q=0.8):
    """RBJ peaking EQ."""
    A = 10 ** (gain_db / 40)
    w = 2 * np.pi * f0 / SR
    al = np.sin(w) / (2 * q)
    b = [1 + al * A, -2 * np.cos(w), 1 - al * A]
    a = [1 + al / A, -2 * np.cos(w), 1 - al / A]
    return ss.lfilter(np.array(b) / a[0], np.array(a) / a[0], x, axis=-1)


def shelf(x, f0, gain_db, kind='low', s=0.8):
    A = 10 ** (gain_db / 40)
    w = 2 * np.pi * f0 / SR
    cw, sw = np.cos(w), np.sin(w)
    al = sw / 2 * np.sqrt((A + 1 / A) * (1 / s - 1) + 2)
    sa = 2 * np.sqrt(A) * al
    if kind == 'low':
        b = [A * ((A + 1) - (A - 1) * cw + sa), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - sa)]
        a = [(A + 1) + (A - 1) * cw + sa, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - sa]
    else:
        b = [A * ((A + 1) + (A - 1) * cw + sa), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - sa)]
        a = [(A + 1) - (A - 1) * cw + sa, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - sa]
    return ss.lfilter(np.array(b) / a[0], np.array(a) / a[0], x, axis=-1)


# -------------------------------------------------------- Karplus-Strong

@numba.njit(cache=True)
def _ks(n, exc, delay, rho, damp):
    """Karplus-Strong string.  delay/rho/damp are per-sample arrays.
    y[i] = exc[i] + rho * LP(y[i - delay]) with a one-pole loop lowpass."""
    y = np.zeros(n)
    s = 0.0
    m = exc.shape[0]
    for i in range(n):
        d = delay[i]
        p = i - d
        v = 0.0
        if p >= 1.0:
            k = int(p)
            fr = p - k
            v = y[k] * (1.0 - fr) + y[k + 1] * fr if k + 1 < i else y[k]
        c = damp[i]
        s = (1.0 - c) * v + c * s
        e = exc[i] if i < m else 0.0
        y[i] = e + rho[i] * s
    return y


def ks_string(midi, dur, vel=1.0, t60=3.0, bright=0.6, pick_pos=0.13, mute=0.0,
              glide=0.004, release=0.025, cents=0.0, seed_rng=None):
    """One plucked string.  mute 0 (open) .. 1 (hard palm mute).
    Returns mono float64 of length dur + release tail."""
    r = seed_rng if seed_rng is not None else _rng()
    f = float(hz(midi)) * 2 ** (cents / 1200)
    n = int((dur + release * 6) * SR) + 1
    t = np.arange(n) / SR
    # loop lowpass coefficient: more damping when muted
    c = 0.08 + 0.55 * (1 - bright) + 0.32 * mute
    c = min(c, 0.93)
    D0 = SR / f - c / (1 - c)
    # pitch glide: string goes sharp on a hard attack and settles
    delay = D0 * (1 - glide * vel * np.exp(-t / 0.035))
    # decay per period from t60 (palm mute shortens drastically)
    t60e = t60 * (1 - mute) + 0.12 * mute
    rho_val = 10 ** (-3.0 / (t60e * f))
    rho = np.full(n, rho_val)
    damp = np.full(n, c)
    # fret-hand mute at note end: loop gain falls fast, loop goes dark
    g = int(dur * SR)
    if g < n:
        u = np.minimum((t[g:] - dur) / 0.012, 1.0)
        rho[g:] = rho_val * (1 - u) + 0.86 * u
        damp[g:] = c * (1 - u) + 0.85 * u
    # excitation: one period of noise, lowpassed for brightness, pick comb
    L = int(D0) + 1
    e = r.standard_normal(L)
    a = 0.15 + 0.6 * (1 - bright) + 0.2 * mute
    e = ss.lfilter([1 - a], [1, -a], e)
    pp = max(1, int(pick_pos * L))
    e = e - 0.9 * np.concatenate([np.zeros(pp), e[:-pp]])
    e = e / (np.max(np.abs(e)) + 1e-9)
    y = _ks(n, e * vel, delay, rho, damp)
    y = ss.lfilter([1, -1], [1, -0.995], y)          # DC block
    env = np.ones(n)
    if g < n:
        env[g:] = np.exp(-(t[g:] - dur) / release)
    y *= env
    return y


def pick_click(n, vel=1.0, mute=0.0):
    """Short pick-on-string transient added to the DI signal."""
    k = int(0.004 * SR)
    t = np.arange(k) / SR
    z = butter(_rng().standard_normal(k), 1800 + 1500 * (1 - mute), 'high', 2) * np.exp(-t / 0.0009)
    out = np.zeros(n)
    out[:k] = z * 0.25 * vel
    return out


def power_chord(root, dur, vel=1.0, kind='open', strum=0.006, take=0, r=None):
    """Guitar DI for a power chord (root, fifth, octave) or a palm-muted chug.
    kind: 'open' (root 5th 8va, rings), 'mute' (palm-muted root+5th),
          'chug' (palm-muted root only, tight), 'single' (one open note)."""
    r = r if r is not None else _rng()
    if kind == 'open':
        notes, mute, t60, br = [root, root + 7, root + 12], 0.0, 3.5, 0.62
    elif kind == 'mute':
        notes, mute, t60, br = [root, root + 7], 0.75, 0.3, 0.45
    elif kind == 'chug':
        notes, mute, t60, br = [root, root + 7], 0.92, 0.15, 0.4
    elif kind == 'fifth':
        notes, mute, t60, br = [root, root + 7], 0.0, 3.5, 0.62
    else:
        notes, mute, t60, br = [root], 0.0, 3.0, 0.65
    rel = 0.02 if kind in ('mute', 'chug') else 0.03
    n = int((dur + rel * 6 + strum * 3 + 0.01) * SR) + 1
    out = np.zeros(n)
    for i, md in enumerate(notes):
        off = int((i * strum + abs(r.normal(0, 0.0012))) * SR)
        sv = vel * (1.0 - 0.08 * i) * (0.92 + 0.16 * r.random())
        y = ks_string(md, max(0.02, dur - i * strum), sv, t60=t60, bright=br + 0.06 * r.standard_normal(),
                      pick_pos=0.10 + 0.06 * r.random(), mute=mute, release=rel,
                      cents=r.normal(0, 2.0) + (1.5 if i == 1 else 0.0), seed_rng=r)
        k = min(len(y), n - off)
        out[off:off + k] += y[:k]
    out += pick_click(n, vel, mute)
    return out


# --------------------------------------------------------------- amp sim

def _min_phase_ir(freqs, mags_db, ntaps=2048):
    """Minimum-phase FIR from a magnitude response (cepstral method)."""
    nfft = 16384
    f = np.fft.rfftfreq(nfft, 1 / SR)
    mag = 10 ** (np.interp(f, freqs, mags_db) / 20)
    logm = np.log(np.maximum(mag, 1e-6))
    full = np.concatenate([logm, logm[-2:0:-1]])
    cep = np.fft.ifft(full).real
    w = np.zeros(nfft)
    w[0] = 1
    w[1:nfft // 2] = 2
    w[nfft // 2] = 1
    h = np.fft.ifft(np.exp(np.fft.fft(cep * w))).real[:ntaps]
    h *= np.hanning(2 * ntaps)[ntaps:]
    return h


def cab_ir(variant=0):
    """A designed 4x12 closed-back cabinet + SM57 response."""
    r = np.random.default_rng(57 + variant)
    f = np.geomspace(20, SR / 2, 400)
    db = np.zeros_like(f)
    db += -40 * np.log10(1 + (75 / f) ** 2) / 2              # low cut (2nd order around 75 Hz)
    db += 4.5 * np.exp(-0.5 * (np.log2(f / 115) / 0.35) ** 2)   # cabinet thump
    db += -3.0 * np.exp(-0.5 * (np.log2(f / 420) / 0.45) ** 2)  # boxy mids dip
    db += 5.0 * np.exp(-0.5 * (np.log2(f / (2500 + 250 * variant)) / 0.4) ** 2)  # speaker presence
    db += 2.0 * np.exp(-0.5 * (np.log2(f / 1200) / 0.3) ** 2)
    db += -60 * np.log10(1 + (f / 5600) ** 4) / 2            # speaker roll-off, steep
    # cone break-up irregularities above 1.5 kHz
    ripple = np.interp(np.log2(f), np.linspace(np.log2(20), np.log2(SR / 2), 60), r.normal(0, 1.8, 60))
    db += ripple * np.clip((f - 1500) / 1500, 0, 1)
    h = _min_phase_ir(f, db)
    return h / np.sqrt(np.sum(h ** 2))


def _shaper(x, drive, bias):
    """Asymmetric tube-like waveshaper."""
    y = np.tanh(drive * x + bias) - np.tanh(bias)
    return np.where(y > 0, y, y * 0.85 + 0.15 * np.tanh(2 * y) / 2)


def amp(di, gain=0.7, variant=0, os_factor=4, chunk=1 << 18, tight=100.0, mid_boost=7.0,
        scoop=-3.0, presence=2.0, low=7.0):
    """High-gain amp + cabinet on a mono DI track (any length). Returns mono float32."""
    x = np.asarray(di, np.float64)
    x = butter(x, tight, 'high', 2)
    x = peq(x, 850, mid_boost, 0.7)
    x = butter(x, 6500, 'low', 1)
    n = len(x)
    out = np.zeros(n, np.float32)
    pad = 2048
    sos_i_hp = ss.butter(1, 140, 'high', fs=SR * os_factor, output='sos')
    sos_i_lp = ss.butter(1, 9000, 'low', fs=SR * os_factor, output='sos')
    sos_o_lp = ss.butter(2, 11000, 'low', fs=SR * os_factor, output='sos')
    for s in range(0, n, chunk):
        a = max(0, s - pad)
        b = min(n, s + chunk + pad)
        seg = x[a:b]
        if not np.any(seg):
            continue
        u = ss.resample_poly(seg, os_factor, 1)
        u = _shaper(u, 14.0 * gain, 0.25)
        u = ss.sosfilt(sos_i_hp, u)
        u = ss.sosfilt(sos_i_lp, u)
        u = _shaper(u, 4.0 * gain, -0.15)
        u = ss.sosfilt(sos_o_lp, u)
        y = ss.resample_poly(u, 1, os_factor)
        out[s:min(n, s + chunk)] = y[s - a:s - a + min(chunk, n - s)]
    y = out.astype(np.float64)
    y = butter(y, 25, 'high', 1)
    # tone stack
    y = shelf(y, 220, low, 'low', 0.6)
    y = peq(y, 480, scoop, 0.9)
    y = peq(y, 2300, presence, 0.9)
    y = ss.oaconvolve(y, cab_ir(variant))[:n]
    return y.astype(np.float32)


# ----------------------------------------------------------- lead guitar

def lead_phrase(notes, spb, vel=1.0, r=None, bend_prob=0.35, vib_depth=0.32):
    """A monophonic lead line as one continuous DI signal.
    notes: list of (beat, midi, len_beats[, flags]) with flags containing
    'b' (bend up into the note), 's' (slide), 'l' (legato, no pick).
    Returns (mono DI, start time in seconds)."""
    r = r if r is not None else _rng()
    notes = sorted(notes, key=lambda z: z[0])
    t0 = notes[0][0] * spb
    t_end = (notes[-1][0] + notes[-1][2]) * spb + 0.6
    n = int((t_end - t0) * SR) + 1
    t = np.arange(n) / SR + t0
    pitch = np.zeros(n)
    amp_ = np.zeros(n)
    picks = []
    for i, nt in enumerate(notes):
        b, md, ln = nt[0], nt[1], nt[2]
        flags = nt[3] if len(nt) > 3 else ''
        s = int((b * spb - t0) * SR)
        e = int(((b + ln) * spb - t0) * SR)
        nxt = int((notes[i + 1][0] * spb - t0) * SR) if i + 1 < len(notes) else n
        legato = nxt <= e + int(0.01 * SR)
        stop = min(n, nxt if legato else e)
        k = stop - s
        tt = np.arange(k) / SR
        p = np.full(k, float(md))
        dur = ln * spb
        if 'b' in flags or ('l' not in flags and dur > 0.45 and r.random() < bend_prob):
            depth = 2.0 if r.random() < 0.6 else 1.0
            p -= depth * np.exp(-tt / 0.045) * (tt < 0.25)
        if i > 0 and ('s' in flags or 'l' in flags):
            prev = notes[i - 1][1]
            p += (prev - md) * np.exp(-tt / 0.018)
        if dur > 0.35:
            onset = min(0.18, dur * 0.35)
            dep = np.clip((tt - onset) / 0.25, 0, 1) * vib_depth
            rate = 5.6 + 0.4 * r.standard_normal()
            # guitar vibrato bends up from the note
            p += dep * (0.5 - 0.5 * np.cos(2 * np.pi * rate * np.maximum(tt - onset, 0)))
        pitch[s:stop] = p
        a = np.ones(k) * (0.75 + 0.25 * np.exp(-tt / 0.4))
        if not legato:
            rel = np.clip((e - s - np.arange(k)) / (0.02 * SR), 0, 1)
            a *= rel
        amp_[s:stop] = a * vel
        if 'l' not in flags:
            picks.append(s)
    # fill pitch in rests with the last pitch (silent anyway)
    last = pitch[0] if pitch[0] else notes[0][1]
    for i in range(n):
        if pitch[i] == 0:
            pitch[i] = last
        else:
            last = pitch[i]
    f = hz(pitch)
    osc = S.saw(f, n) * 0.6 + (S.saw(f, n) - S.saw(f, n, 0.42)) * 0.35
    osc = butter(osc, 3500, 'low', 1)
    amp_ = butter(amp_, 120, 'low', 1)
    di = osc * amp_ * 0.5
    for s in picks:
        k = min(int(0.004 * SR), n - s)
        di[s:s + k] += butter(r.standard_normal(k), 2500, 'high', 2) * np.exp(-np.arange(k) / (0.0008 * SR)) * 0.2
    return di, t0


# ----------------------------------------------------------- bass guitar

def bass_note(midi, dur, vel=1.0, r=None):
    """Picked metal bass: KS string + sine sub, with parallel grit. Mono."""
    r = r if r is not None else _rng()
    y = ks_string(midi, dur, vel, t60=2.5, bright=0.7, pick_pos=0.12, mute=0.15, glide=0.002,
                  release=0.02, seed_rng=r)
    n = len(y)
    t = np.arange(n) / SR
    env = np.where(t < dur, np.exp(-t / 1.5), np.exp(-dur / 1.5) * np.exp(-(t - dur) / 0.02))
    sub = np.sin(2 * np.pi * hz(midi) * t) * env * np.minimum(t / 0.003, 1) * vel
    clean = y * 0.8 + sub * 0.4
    grit = np.tanh(6 * butter(y, 250, 'high', 2))
    grit = butter(grit, 3200, 'low', 2)
    return clean + 0.4 * grit


# ------------------------------------------------------------------ drums

def metal_kick(vel=1.0, f_end=58.27, r=None):
    """Triggered metal kick: tight tuned body, beater click. Mono."""
    r = r if r is not None else _rng()
    length = 0.32
    n = int(length * SR)
    t = np.arange(n) / SR
    f = f_end + 110 * np.exp(-t / 0.018) + 900 * np.exp(-t / 0.0018)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.13) * np.minimum(t / 0.0005, 1)
    body *= np.clip((length - t) / 0.05, 0, 1)
    click = butter(r.standard_normal(n), [2500, 7000], 'band', 2) * np.exp(-t / 0.0022) * 0.9
    beater = np.sin(2 * np.pi * 1600 * t) * np.exp(-t / 0.0015) * 0.5
    y = np.tanh(1.8 * body) * 0.95 + click + beater
    return y * vel


class Snare:
    """Metal snare: synthesized body + wires layered with VSCO acoustic snare hits."""

    def __init__(self):
        self.samples = []
        for nm in ('snare1_fff_1', 'snare1_fff_2', 'snare1_ff_1', 'snare1_ff_2'):
            p = f'{VSCO}/VSCO 1 Percussion/drums/snare/drum1/{nm}.wav'
            if os.path.exists(p):
                x, sr = sf.read(p)
                x = x.mean(1) if x.ndim > 1 else x
                x = ss.resample_poly(x, SR, sr) if sr != SR else x
                pk = np.argmax(np.abs(x))
                x = x[max(0, pk - int(0.002 * SR)):]
                self.samples.append(x / (np.max(np.abs(x)) + 1e-9))
        self.rim = []
        p = f'{VSCO}/VSCO 1 Percussion/drums/snare/drum1/snare1_rimshot_fff_1.wav'
        if os.path.exists(p):
            x, sr = sf.read(p)
            x = x.mean(1) if x.ndim > 1 else x
            x = ss.resample_poly(x, SR, sr) if sr != SR else x
            pk = np.argmax(np.abs(x))
            self.rim = x[max(0, pk - int(0.002 * SR)):] / (np.max(np.abs(x)) + 1e-9)

    def hit(self, vel=1.0, r=None, ghost=False):
        r = r if r is not None else _rng()
        n = int(0.45 * SR)
        t = np.arange(n) / SR
        body = (np.sin(2 * np.pi * 185 * t * (1 + 0.25 * np.exp(-t / 0.006))) * np.exp(-t / 0.06) +
                0.5 * np.sin(2 * np.pi * 330 * t) * np.exp(-t / 0.035))
        wires = butter(r.standard_normal(n), [1800, 9500], 'band', 2) * (np.exp(-t / 0.11) * 0.8 + 0.4 * np.exp(-t / 0.02))
        crack = butter(r.standard_normal(n), 3000, 'high', 2) * np.exp(-t / 0.003) * 1.2
        syn = np.tanh(1.5 * (0.7 * body + 0.9 * wires + crack)) * np.minimum(t / 0.0004, 1)
        y = syn * 0.55
        if self.samples:
            smp = self.samples[r.integers(len(self.samples))]
            k = min(n, len(smp))
            y[:k] += smp[:k] * 0.9
            if not ghost and len(self.rim):
                k = min(n, len(self.rim))
                y[:k] += self.rim[:k] * 0.35
        y *= np.clip((0.45 - t) / 0.05, 0, 1)
        return y * vel


def metal_tom(midi, vel=1.0, r=None):
    r = r if r is not None else _rng()
    n = int(0.9 * SR)
    t = np.arange(n) / SR
    f0 = float(hz(midi))
    f = f0 * (1 + 0.45 * np.exp(-t / 0.04))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.28)
    y += 0.35 * np.sin(2 * np.pi * np.cumsum(f * 1.59) / SR) * np.exp(-t / 0.12)
    y += butter(r.standard_normal(n), [1500, 6000], 'band', 2) * np.exp(-t / 0.006) * 0.6
    y = np.tanh(1.6 * y) * np.minimum(t / 0.0005, 1)
    y *= np.clip((0.9 - t) / 0.1, 0, 1)
    return y * vel


def china(vel=1.0, length=2.2, r=None):
    """Trashy china cymbal: inharmonic square partials, band-limited, fast attack."""
    r = r if r is not None else _rng()
    n = int(length * SR)
    t = np.arange(n) / SR
    out = []
    for _ in range(2):
        src = np.zeros(n)
        for fr in r.uniform(400, 4200, 14):
            src += np.sign(np.sin(2 * np.pi * fr * t + r.random() * 6.28))
        src = src / 14 * 0.8 + r.standard_normal(n) * 0.6
        src = src * (1 + 0.6 * np.sin(2 * np.pi * 333 * t))
        y = butter(src, [1200, 11000], 'band', 2)
        y = peq(y, 2800, 6.0, 1.2)
        y *= 0.6 * np.exp(-t / 0.05) + 0.4 * np.exp(-t / 0.55)
        out.append(y)
    return np.stack(out) * vel * 0.6


# ----------------------------------------------------------------- choir

_FORMANTS = {
    'a': [(800, 1.0, 80), (1150, 0.5, 90), (2900, 0.18, 120), (3900, 0.1, 130)],
    'o': [(450, 1.0, 70), (800, 0.4, 80), (2830, 0.08, 100), (3800, 0.05, 120)],
    'u': [(325, 1.0, 50), (700, 0.25, 60), (2700, 0.04, 170), (3800, 0.03, 180)],
}


def choir(midi, dur, vel=1.0, vowel='a', voices=6, a=0.25, r_=0.6, female=None, rr=None):
    """Synthesized choir section: detuned saws with jitter and vibrato, through
    vowel formant band-passes. Returns (2, n)."""
    rr = rr if rr is not None else _rng()
    n = int((dur + 4 * r_) * SR) + 1
    t = np.arange(n) / SR
    if female is None:
        female = midi >= 60
    fm = _FORMANTS[vowel]
    if female:
        fm = [(f * 1.12, g, bw * 1.1) for f, g, bw in fm]
    L = np.zeros(n)
    R = np.zeros(n)
    for v in range(voices):
        det = rr.normal(0, 0.09)
        rate = 4.8 + rr.random() * 1.2
        jit = butter(rr.standard_normal(n), 6, 'low', 1) * 3.0
        p = midi + det + 0.13 * np.sin(2 * np.pi * rate * t + rr.random() * 6.28) * np.clip(t / 0.6, 0, 1) + jit * 0.05
        w = S.saw(hz(p), n, rr.random())
        w = w + 0.12 * butter(rr.standard_normal(n), 3000, 'high', 1)   # breath
        pan = (v / max(1, voices - 1)) * 1.4 - 0.7
        L += w * np.sqrt((1 - pan) / 2)
        R += w * np.sqrt((1 + pan) / 2)
    src = np.stack([L, R])
    y = np.zeros_like(src)
    for f, g, bw in fm:
        q = f / (bw * 2.0)
        y += g * S.svf(src, f, q, 'bp')
    y = butter(y, 150, 'high', 2)
    e = S.env_adsr(n, dur, a, 0.6, 0.9, r_)
    return y * e * vel / np.sqrt(voices) * 1.6


# --------------------------------------------------------------- sampler

_NOTE = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}


class Sampler:
    """Nearest-sample instrument over a VSCO-2 folder.
    VSCO names put middle C at C3, so real MIDI = named + 12 (verified by f0
    detection on every folder used here)."""

    def __init__(self, folder, release=0.25, offset=12, gain=1.0, max_len=None, layers=None):
        self.release = release
        self.gain = gain
        self.items = []   # (midi, layer, rr, data)
        files = sorted(glob.glob(f'{VSCO}/{folder}/*.wav'))
        for f in files:
            b = os.path.basename(f)
            mm = re.search(r'_([A-G]#?)(-?\d)(?=[_.])', b)
            if not mm:
                continue
            midi = 12 * (int(mm.group(2)) + 1) + _NOTE[mm.group(1)] + offset
            lv = re.search(r'_v(\d)', b)
            layer = int(lv.group(1)) if lv else 1
            if layers is not None and layer not in layers:
                continue
            x, sr = sf.read(f, dtype='float32')
            if x.ndim == 1:
                x = np.stack([x, x], 1)
            x = x.T[:2]
            if max_len:
                x = x[:, :int(max_len * sr)]
            if sr != SR:
                x = ss.resample_poly(x, SR, sr, axis=1).astype(np.float32)
            # trim leading silence
            env = np.max(np.abs(x), axis=0)
            th = np.max(env) * 0.01
            i0 = max(0, int(np.argmax(env > th)) - int(0.003 * SR))
            x = x[:, i0:]
            self.items.append((midi, layer, x))
        self.layers = sorted(set(it[1] for it in self.items))
        # loudness-normalise each layer group so velocity does the dynamics
        peak = max(np.max(np.abs(it[2])) for it in self.items)
        self.norm = 1.0 / peak
        self._cache = {}

    def note(self, midi, dur, vel=1.0, layer=None, r=None, attack=0.0):
        """Returns (2, n) float32."""
        r = r if r is not None else _rng()
        if layer is None:
            # map vel 0..1 onto available layers
            li = min(len(self.layers) - 1, int(vel * len(self.layers) * 0.999))
            layer = self.layers[li]
        cands = [it for it in self.items if it[1] == layer] or self.items
        best = min(abs(it[0] - midi) for it in cands)
        pool = [it for it in cands if abs(it[0] - midi) == best]
        smidi, _, x = pool[r.integers(len(pool))]
        ratio = 2 ** ((midi - smidi) / 12)
        n_out = int((dur + 5 * self.release) * SR)
        n_src_needed = int(n_out * ratio) + 2
        src = x[:, :min(x.shape[1], n_src_needed)]
        pos = np.arange(n_out) * ratio
        pos = pos[pos < src.shape[1] - 1]
        i = pos.astype(np.int64)
        fr = (pos - i).astype(np.float32)
        y = src[:, i] * (1 - fr) + src[:, i + 1] * fr
        m = y.shape[1]
        t = np.arange(m) / SR
        env = np.where(t < dur, 1.0, np.exp(-(t - dur) / self.release)).astype(np.float32)
        if attack > 0:
            env *= np.minimum(t / attack, 1).astype(np.float32)
        tail = int(0.01 * SR)
        if m > tail:
            env[-tail:] *= np.linspace(1, 0, tail, dtype=np.float32)
        return y * env * (self.norm * self.gain * (0.35 + 0.65 * vel))


def load_one(path, offset=0):
    x, sr = sf.read(path, dtype='float32')
    if x.ndim == 1:
        x = np.stack([x, x], 1)
    x = x.T[:2]
    if sr != SR:
        x = ss.resample_poly(x, SR, sr, axis=1).astype(np.float32)
    return x / (np.max(np.abs(x)) + 1e-9)


def repitch(x, semis):
    ratio = 2 ** (semis / 12)
    pos = np.arange(int(x.shape[1] / ratio) - 1) * ratio
    i = pos.astype(np.int64)
    fr = (pos - i).astype(np.float32)
    return x[:, i] * (1 - fr) + x[:, i + 1] * fr
