# synth_new06.py -- NEW FILE (2026-10-14), Stellar Frontier Enhanced Edition.
#
# Instruments for new06_industrial.py (an industrial / EBM track): a hard,
# tuned EBM kick, a round, heavy, metallic snare (round-robin), clanks from
# sampled anvil and brake drum, synthesized modal metal hits, a sequenced
# saw/pulse bass with a resonant filter envelope, a cold pulse lead, a
# distorted "grind" brass, a dark string machine, tubular-bell pings and a
# distorted machine drone.  Built on synth.py (which is not modified).
#
# Samples: VSCO-2 Community Edition by Versilian Studios, CC0 1.0 / public
# domain, https://github.com/sgossner/VSCO-2-CE -- Percussion/Anvil_Hit1,
# Percussion/BrakeDrum1_Hammer, Percussion/TB_hit (tubular bells),
# Percussion/gongscrape, VSCO 1 Percussion/varMetal/Gong (gong_smack).
# They are read as data with soundfile.  Everything else is synthesized.
#
# v2 (2026-10-23): hammer(), hydraulic() and press() for the groove-locked shipyard layer
# that replaces v1's loud anvil; samples are found via $VSCO_DIR.
import glob
import os
import numpy as np
import soundfile as sf

import synth as S
from synth import SR, hz, saw, sine, svf, butter

VSCO = os.environ.get('VSCO_DIR', '/tmp/claude-0/samples/VSCO-2-CE').rstrip('/') + '/'
_wav = {}


def load(path):
    """Stereo float32 (2, n) resampled to SR, onset-trimmed, peak-normalized."""
    if path not in _wav:
        x, sr = sf.read(path, dtype='float32', always_2d=True)
        x = x.T
        if x.shape[0] == 1:
            x = np.vstack([x, x])
        x = x[:2]
        a = np.max(np.abs(x), axis=0)
        i0 = max(0, int(np.argmax(a > a.max() * 0.02)) - int(0.002 * sr))
        x = x[:, i0:]
        if sr != SR:
            pos = np.arange(int(x.shape[1] * SR / sr)) * sr / SR
            x = np.stack([np.interp(pos, np.arange(x.shape[1]), c) for c in x]).astype(np.float32)
        _wav[path] = x / (np.max(np.abs(x)) + 1e-9)
    return _wav[path]


def repitch(x, semis, maxlen=None, fade=0.03):
    """Resample by 2**(semis/12) (lower = slower/darker); optional length cap with fade."""
    ratio = 2 ** (semis / 12)
    n = int((x.shape[1] - 2) / ratio)
    if maxlen:
        n = min(n, int(maxlen * SR))
    pos = np.arange(n) * ratio
    i = pos.astype(np.int64)
    fr = (pos - i).astype(np.float32)
    y = x[:, i] * (1 - fr) + x[:, i + 1] * fr
    k = min(n, int(fade * SR))
    y[:, n - k:] *= np.linspace(1, 0, k, dtype=np.float32)
    return y


# ---------------------------------------------------------------- drums

def ebm_kick(vel=1.0, f_end=58.27, var=0):
    """Hard, short EBM kick tuned to Bb1 (the fifth of Eb minor), saturated."""
    n = int(0.36 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(10 + var)
    f = f_end + (190.0 - f_end) * np.exp(-t / 0.024) + 500 * np.exp(-t / 0.0025)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR)
    amp = np.exp(-t / 0.11) * np.minimum(t / 0.0006, 1)
    amp *= np.where(t > 0.30, np.clip((0.36 - t) / 0.06, 0, 1), 1)
    y = np.tanh(2.8 * body * amp) * 0.85
    thud = butter(g.standard_normal(n), [90, 300], 'band', 2) * np.exp(-t / 0.012) * 0.5
    click = butter(g.standard_normal(n), [2000, 6000], 'band', 2) * np.exp(-t / 0.0018) * 0.25
    y = y + thud + click
    return np.stack([y, y]).astype(np.float32) * vel


_SN_RR = [(1.000, 1.00, 1.00, 0), (0.982, 0.93, 0.95, 1), (1.018, 1.06, 0.98, 2), (0.994, 0.97, 0.92, 0)]
_brake = None


def _brakes():
    global _brake
    if _brake is None:
        _brake = [load(p) for p in sorted(glob.glob(VSCO + 'Percussion/BrakeDrum1_Hammer_v*_Sum.wav'))]
    return _brake


def heavy_snare(vel=1.0, var=0, f0=185.0):
    """Round, heavy industrial snare ("pdlap" with a metal edge, not "tschh").

    A tuned body at F#3 (185 Hz, the minor third of Eb minor) with a fast pitch
    drop and two inharmonic shell modes, a short 1-4 kHz snap, a small clap
    flam, a trace of a pitched-down brake-drum hit for the metallic edge;
    ~120 ms decay, low-passed at 7 kHz.  var 0..3 are round-robin variants.
    """
    pm, tone, lvl, bk = _SN_RR[var % 4]
    n = int(0.28 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(40 + var)
    f = f0 * pm * (1 + 0.65 * np.exp(-t / 0.009))
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = (np.sin(ph) * np.exp(-t / 0.075) + 0.4 * np.sin(1.47 * ph) * np.exp(-t / 0.045) +
            0.18 * np.sin(2.21 * ph) * np.exp(-t / 0.02))
    snap = butter(g.standard_normal(n), [1000 * tone, 3800 * tone], 'band', 2)
    snap *= np.exp(-t / 0.017) + 0.22 * np.exp(-t / 0.055)
    clap = np.zeros(n)
    for off in (0.0, 0.006, 0.013):
        u = t - off
        clap += np.where(u >= 0, np.exp(-np.maximum(u, 0) / 0.0035), 0)
    clap = butter(g.standard_normal(n) * clap, [1100, 3000], 'band', 2) * 0.3
    y = 1.0 * body + 0.62 * snap + 1.1 * clap
    y = np.tanh(1.5 * y) / 1.1 * np.minimum(t / 0.0004, 1)
    y = butter(y, 7000, 'low', 2)
    out = np.stack([y, np.roll(y, 11)])
    br = repitch(_brakes()[bk], -7 + 0.3 * var, maxlen=0.28)
    br = butter(br, [500, 5000], 'band', 2) * np.exp(-t[:br.shape[1]] / 0.05)
    out[:, :br.shape[1]] += br * 0.22
    fe = int(0.03 * SR)
    out[:, -fe:] *= np.linspace(1, 0, fe)
    return (out * vel * lvl).astype(np.float32)


def clank(kind='brake', semis=0.0, vel=1.0, var=0, maxlen=0.6, lp=6000.0, decay=0.18):
    """Sampled metal: 'brake' (brake drum, 1-3.5 kHz) or 'anvil' (pitched way down)."""
    if kind == 'brake':
        x = _brakes()[var % 3]
    else:
        x = load(sorted(glob.glob(VSCO + 'Percussion/Anvil_Hit1_v*_Sum.wav'))[var % 3])
    y = repitch(x, semis, maxlen=maxlen)
    t = np.arange(y.shape[1]) / SR
    y = y * (0.35 * np.exp(-t / 0.012) + 0.65 * np.exp(-t / decay))
    y = butter(y, lp, 'low', 2)
    y = butter(y, 180, 'high', 2)
    return (y * vel).astype(np.float32)


def hammer(var=0, semis=-17.0):
    """v2: the shipyard hammer, tamed.  The anvil pitched down less far, cut to
    ~0.4 s with a fast decay (no long ring), band-limited to 250-3500 Hz, with a
    small brake-drum strike on top so it reads as a hammer on steel, not a gong."""
    x = load(sorted(glob.glob(VSCO + 'Percussion/Anvil_Hit1_v*_Sum.wav'))[var % 3])
    y = repitch(x, semis + 0.35 * (var % 3), maxlen=0.42, fade=0.06)
    t = np.arange(y.shape[1]) / SR
    y = y * (0.45 * np.exp(-t / 0.01) + 0.55 * np.exp(-t / 0.085))
    br = repitch(_brakes()[(var + 1) % 3], -9.0, maxlen=0.2)
    tb = np.arange(br.shape[1]) / SR
    y[:, :br.shape[1]] += br * np.exp(-tb / 0.025) * 0.35
    y = butter(butter(y, 3500, 'low', 2), 250, 'high', 2)
    return (y / (np.max(np.abs(y)) + 1e-9)).astype(np.float32)


def hydraulic(dur=0.42, seed=0):
    """v2: a hydraulic press letting off pressure: a short "chunk" (valve) then a
    hiss whose band-pass falls 3.2 kHz -> 700 Hz; dark (no 'tschh' above 5 kHz)."""
    n = int((dur + 0.05) * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(300 + seed)
    fc = 700 + 2500 * np.exp(-t / (dur * 0.35))
    amp = np.minimum(t / 0.012, 1) * np.exp(-t / (dur * 0.38))
    L = svf(g.standard_normal(n), fc, 1.8, 'bp') * amp
    R = svf(g.standard_normal(n), fc * 1.04, 1.8, 'bp') * amp
    chunk = butter(g.standard_normal(n), [150, 900], 'band', 2) * np.exp(-t / 0.012)
    hiss = np.stack([L + 0.8 * chunk, R + 0.8 * chunk])
    hiss = butter(hiss, 5000, 'low', 2)
    fe = int(0.04 * SR)
    hiss[:, -fe:] *= np.linspace(1, 0, fe)
    return (hiss / (np.max(np.abs(hiss)) + 1e-9)).astype(np.float32)


def press(midi=51, seed=0):
    """v2: machinery stamp: a short, dull tuned metal clunk (modal hit, low
    brightness) plus a 400-2500 Hz noise thump; the stamping machine under the kick."""
    y = metal_hit(midi, 1.0, decay=0.07, bright=0.55, seed=seed)
    n = y.shape[1]
    t = np.arange(n) / SR
    g = np.random.default_rng(500 + seed)
    th = butter(g.standard_normal(n), [400, 2500], 'band', 2) * np.exp(-t / 0.008)
    y = y + th[None, :] * 0.5
    y = butter(y, 4500, 'low', 2)
    return (y / (np.max(np.abs(y)) + 1e-9)).astype(np.float32)


_MODES = np.array([1.0, 2.32, 3.87, 5.21, 6.73, 8.66])


def metal_hit(midi, vel=1.0, decay=0.35, bright=1.0, seed=0):
    """Modal (struck bar / pipe) metal hit tuned to midi: inharmonic partials."""
    f0 = float(hz(midi))
    n = int((decay * 5 + 0.05) * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(seed)
    y = np.zeros(n)
    for k, r in enumerate(_MODES):
        fk = f0 * r * (1 + 0.003 * g.standard_normal())
        if fk > 12000:
            break
        amp = (0.8 ** k) * (bright ** k)
        y += amp * np.sin(2 * np.pi * fk * t + g.random() * 6.28) * np.exp(-t / (decay / (1 + 0.5 * k)))
    strike = butter(g.standard_normal(n), [f0 * 2, min(f0 * 12, 9000)], 'band', 2) * np.exp(-t / 0.004) * 0.6
    y = (y + strike) * np.minimum(t / 0.0005, 1)
    fe = int(0.02 * SR)
    y[-fe:] *= np.linspace(1, 0, fe)
    return np.stack([y, np.roll(y, 9)]).astype(np.float32) * vel * 0.5


def tick(vel=1.0, pan=0.0, seed=0):
    """Short, dark metallic tick (the industrial hat): ring-modulated squares, band-passed."""
    n = int(0.06 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(seed)
    src = 0.6 * S._metal(n) + 0.4 * g.standard_normal(n)
    y = butter(src, [5000, 10000], 'band', 2) * np.exp(-t / 0.014)
    return S.pan2(y * vel, pan).astype(np.float32)


# ----------------------------------------------------------------- bass

def seq_bass(midi, dur, vel=1.0, cut=500.0, env=1800.0, decay=0.06, q=1.6, drive=1.8):
    """Sequenced EBM bass: saw + square sub-octave, resonant 4-pole LP with a
    fast envelope, saturation.  Mono (2, n)."""
    r = 0.012
    n = int((dur + 6 * r) * SR)
    f = hz(midi)
    w = 0.65 * saw(f, n) + 0.35 * np.sign(sine(f / 2, n))
    t = np.arange(n) / SR
    fc = cut + env * np.exp(-t / decay)
    y = svf(svf(w, fc, q), fc, 0.6)
    e = np.where(t < dur, 1.0, np.exp(-(t - dur) / r)) * np.minimum(t / 0.0015, 1)
    y = np.tanh(drive * y * e) / np.tanh(drive) * vel
    y = butter(y, 38, 'high', 2)
    return np.stack([y, y]).astype(np.float32)


def sub(midi, dur, vel=1.0, a=0.003, r=0.03):
    return S.sub_bass(midi, dur, vel, a=a, r=r, drive=1.2).astype(np.float32)


# --------------------------------------------------------- tonal synths

def pulse(freq, n, width=0.5, phase=0.0):
    """Band-limited pulse from two polyBLEP saws."""
    return 0.5 * (saw(freq, n, phase) - saw(freq, n, (phase + width) % 1.0))


def cold_lead(midi, dur, vel=1.0, cut=2300.0, cut_env=1500.0, r=0.18, vib=0.10):
    """Cold, hollow pulse lead (two detuned pulses + a quiet saw), slight scoop and vibrato."""
    n = int((dur + 6 * r) * SR)
    t = np.arange(n) / SR
    vb = S.vibrato(n, onset=0.35, rate=4.8, depth=vib) if dur > 0.5 else 0.0
    scoop = -0.35 * np.exp(-t / 0.03)
    L = pulse(hz(midi - 0.07 + vb + scoop), n, 0.32, 0.1)
    R = pulse(hz(midi + 0.07 + vb + scoop), n, 0.36, 0.6)
    s = saw(hz(midi + vb + scoop), n, 0.3) * 0.35
    fc = cut + cut_env * np.exp(-t / 0.18)
    x = svf(np.stack([L + s, R + s]), fc, 0.9)
    e = S.env_adsr(n, dur, 0.012, 0.5, 0.8, r)
    x = np.tanh(1.4 * x * e) / 1.4
    return (x * vel).astype(np.float32)


def grind_brass(midi, dur, vel=1.0, cut=1300.0, drive=3.0, a=0.05, r=0.25):
    """Distorted synth brass: detuned saws, filter swell, tanh drive, dark LP."""
    n = int((dur + 6 * r) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    for o, p in ((-0.11, -0.6), (-0.04, -0.2), (0.04, 0.2), (0.11, 0.6)):
        w = saw(hz(midi + o), n, S.rng().random())
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    fc = cut * (0.55 + 0.45 * np.minimum(t / 0.15, 1)) + 600 * np.exp(-t / 0.3)
    x = svf(np.stack([L, R]) * 0.5, fc, 1.0)
    e = S.env_adsr(n, dur, a, 0.6, 0.85, r)
    x = np.tanh(drive * x * e) / np.tanh(drive)
    x = butter(x, 2600, 'low', 2)
    x = butter(x, 120, 'high', 2)
    return (x * vel).astype(np.float32)


def string_machine(midi, dur, vel=1.0, cut=2200.0, a=0.6, r=1.2):
    """Solina-like string machine voice: detuned saws, gentle LP, slow attack."""
    n = int((dur + 5 * r) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    vb = 0.05 * np.sin(2 * np.pi * 5.5 * t)
    for o, p in ((-0.09, -0.8), (0.0, 0.0), (0.09, 0.8)):
        w = saw(hz(midi + o + vb), n, S.rng().random())
        L += w * np.sqrt((1 - p) / 2)
        R += w * np.sqrt((1 + p) / 2)
    x = svf(np.stack([L, R]) * 0.45, cut, 0.7)
    e = S.env_adsr(n, dur, a, 1.0, 0.9, r)
    return (x * e * vel).astype(np.float32)


def dark_pad(midi, dur, vel=1.0, cut=900.0, a=1.2, r=1.6):
    """Dark detuned-saw pad voice (supersaw, closed filter, slow)."""
    return S.supersaw(midi, dur, vel, voices=5, detune=0.18, width=0.85, a=a, d=1.5, s=0.9, r=r,
                      cut=cut, q=0.75).astype(np.float32)


_tb = None


def tubular(midi, dur=2.5, vel=1.0):
    """Tubular bell (VSCO TB hits, nearest sample repitched), darkened, plus a soft FM tail."""
    global _tb
    if _tb is None:
        _tb = {}
        for p in glob.glob(VSCO + 'Percussion/TB_hit_*.wav'):
            nm = os.path.basename(p).split('_')[2]
            _tb[S.m(nm)] = load(p)
    keys = np.array(sorted(_tb))
    k = int(keys[np.argmin(np.abs(keys - midi))])
    y = repitch(_tb[k], midi - k, maxlen=dur + 1.0, fade=0.4)
    y = butter(y, 5500, 'low', 2)
    fm = S.bell(midi, dur * 0.5, r=dur * 0.4)[:, :y.shape[1]] * 0.25
    y[:, :fm.shape[1]] += fm
    return (y * vel).astype(np.float32)


def drone(midi, n, vel=1.0, seed=0):
    """Distorted machine drone: saws at midi and an octave up, driven, band-passed,
    with a slowly wandering filter (for long textures)."""
    g = np.random.default_rng(seed)
    t = np.arange(n) / SR
    w = saw(hz(midi), n, g.random()) + 0.6 * saw(hz(midi + 12.03), n, g.random()) + \
        0.3 * saw(hz(midi + 19.0), n, g.random())
    w = np.tanh(3.0 * w)
    fc = 520 * 2 ** (0.9 * np.sin(2 * np.pi * t / 11.0 + g.random() * 6) +
                     0.4 * np.sin(2 * np.pi * t / 4.3 + g.random() * 6))
    y = svf(w, fc, 2.2, 'bp')
    L = y
    R = np.roll(y, int(0.013 * SR))
    return (np.stack([L, R]) * vel * 0.5).astype(np.float32)


def gong_scrape(semis=-5.0, vel=1.0):
    x = load(VSCO + 'Percussion/gongscrape_mf.wav')
    y = repitch(x, semis, fade=0.5)
    y = butter(butter(y, 4500, 'low', 2), 120, 'high', 2)
    return (y * vel).astype(np.float32)


def gong_smack(semis=-3.0, vel=1.0, maxlen=4.0):
    x = load(VSCO + 'VSCO 1 Percussion/varMetal/Gong/gong_smack_low_1.wav')
    y = repitch(x, semis, maxlen=maxlen, fade=1.0)
    y = butter(y, 3500, 'low', 2)
    return (y * vel).astype(np.float32)


def rev_swell(length, seed=0):
    """Reverse cymbal swell, darkened (no bright 'wipe')."""
    c = S.reverse_cymbal(length)
    return butter(c, 7000, 'low', 2).astype(np.float32)
