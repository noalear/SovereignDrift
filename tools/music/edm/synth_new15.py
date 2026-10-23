# synth_new15.py -- NEW FILE (2026-10-23), Stellar Frontier remake.
#
# Instruments for new15_hard_industrial.py ("Hull Breach", peak-time hard
# industrial techno): a pounding, distorted kick tuned to B-flat (the fifth
# of E-flat minor), a tuned offbeat rumble on E-flat, a sharp distorted riff
# synth, gated chord stabs, dark hats, tuned metal clangs and an alarm-free
# set of FX.  Built on synth.py and synth_new06.py (neither is modified).
#
# Samples: VSCO-2 Community Edition by Versilian Studios, CC0 1.0 / public
# domain, https://github.com/sgossner/VSCO-2-CE -- Percussion/Anvil_Hit1,
# Percussion/BrakeDrum1_Hammer, Percussion/gongscrape, VSCO 1 Percussion/
# varMetal/Gong/gong_hit_f.  Read as data with soundfile (via synth_new06.load).
import os
import numpy as np

import synth as S
import synth_new06 as I
from synth import SR, hz, saw, sine, svf, butter

VSCO = os.environ.get('VSCO_DIR', '/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/vsco')
I.VSCO = VSCO.rstrip('/') + '/'


# ---------------------------------------------------------------- drums

def hard_kick(vel=1.0, var=0, f_end=58.27):
    """Pounding distorted kick, fundamental B-flat 1 (58.3 Hz), ~0.45 s.

    A pitch-swept sine driven hard into tanh (odd harmonics give the
    'distorted' hard-techno punch), a band-passed knock and a short click;
    the distorted part is low-passed so the top end stays dark."""
    n = int(0.46 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(150 + var)
    f = f_end * (1 + 0.004 * var) + (240.0 - f_end) * np.exp(-t / 0.020) + 900 * np.exp(-t / 0.0018)
    ph = 2 * np.pi * np.cumsum(f) / SR
    amp = np.exp(-t / 0.17) * np.minimum(t / 0.0005, 1)
    amp *= np.where(t > 0.38, np.clip((0.46 - t) / 0.08, 0, 1), 1)
    body = np.sin(ph) * amp
    dist = np.tanh(4.5 * body) / np.tanh(4.5)
    dist = butter(dist, 5000, 'low', 2)
    clean = np.sin(ph) * amp
    y = 0.75 * dist + 0.35 * clean
    knock = butter(g.standard_normal(n), [180, 900], 'band', 2) * np.exp(-t / 0.010) * 0.35
    click = butter(g.standard_normal(n), [1800, 5000], 'band', 2) * np.exp(-t / 0.0015) * 0.28
    y = np.tanh(1.4 * (y + knock + click)) / np.tanh(1.4)
    return np.stack([y, y]).astype(np.float32) * vel


def rumble_note(midi=27, dur=0.3, vel=1.0, drive=3.0, cut=260.0, seed=0):
    """Tuned rumble: E-flat sub + octave + a little dark noise, driven, low-passed.
    Mono (2, n).  midi 27 = Eb1."""
    n = int((dur + 0.03) * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(300 + seed)
    f0 = float(hz(midi))
    x = (0.7 * sine(f0, n, g.random()) + 0.9 * sine(2 * f0, n, g.random()) + 0.4 * sine(3 * f0, n, g.random()))
    nz = butter(g.standard_normal(n), [60, 400], 'band', 2) * 0.5
    x = np.tanh(drive * (x + nz)) / np.tanh(drive)
    x = svf(x, cut, 1.2)
    e = np.minimum(t / 0.006, 1) * np.where(t < dur, np.exp(-t / (dur * 1.2)), np.exp(-dur / (dur * 1.2)) *
                                             np.exp(-(t - dur) / 0.01))
    y = butter(x * e, 32, 'high', 2) * vel
    return np.stack([y, y]).astype(np.float32)


def dark_hat(vel=1.0, open_=False, pan=0.0, seed=0):
    """Closed / open hat, low-passed so nothing harsh sits above 10 kHz."""
    length = 0.32 if open_ else 0.07
    n = int(length * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(500 + seed)
    tau = 0.09 if open_ else 0.018
    src = 0.55 * S._metal(n) + 0.5 * g.standard_normal(n)
    y = butter(src, [6000, 11000] if not open_ else [5000, 10000], 'band', 2) * np.exp(-t / tau)
    y *= np.minimum(t / 0.0006, 1)
    return S.pan2(y * vel, pan).astype(np.float32)


def ride(vel=1.0, pan=0.3, seed=0):
    n = int(0.9 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(600 + seed)
    src = S._metal(n) * 0.7 + 0.35 * g.standard_normal(n)
    y = butter(src, [3000, 9000], 'band', 2) * (0.55 * np.exp(-t / 0.015) + 0.45 * np.exp(-t / 0.35))
    return S.pan2(y * vel * 0.8, pan).astype(np.float32)


def crash(vel=1.0, seed=0):
    n = int(2.4 * SR)
    t = np.arange(n) / SR
    g = np.random.default_rng(700 + seed)
    out = []
    for _ in range(2):
        src = 0.5 * S._metal(n) + g.standard_normal(n)
        y = butter(src, [3000, 9000], 'band', 2) * (0.5 * np.exp(-t / 0.03) + 0.5 * np.exp(-t / 0.8))
        out.append(y)
    return (np.stack(out) * vel * 0.7).astype(np.float32)


def clang(midi, vel=1.0, var=0, decay=0.22):
    """Tuned metal clang: a modal pipe hit at `midi` layered with a pitched brake-drum hit."""
    mh = I.metal_hit(midi, 1.0, decay=decay, bright=0.85, seed=midi * 3 + var)
    br = I.clank('brake', (midi - 78) * 0.5 - 2 + 0.4 * var, 1.0, var, maxlen=0.4, lp=7000, decay=0.12)
    n = max(mh.shape[1], br.shape[1])
    y = np.zeros((2, n), np.float32)
    y[:, :mh.shape[1]] += mh * 0.9
    y[:, :br.shape[1]] += br * 0.45
    y = butter(y, 7500, 'low', 2)
    return (y * vel).astype(np.float32)


def anvil(vel=1.0, semis=-14.0, var=0, maxlen=0.8):
    return I.clank('anvil', semis, vel, var, maxlen=maxlen, lp=4500, decay=0.25)


def tom(midi, vel=1.0, pan=0.0):
    """Distorted industrial tom (pitched), dark."""
    n = int(0.45 * SR)
    t = np.arange(n) / SR
    f = hz(midi) * (1 + 0.5 * np.exp(-t / 0.025))
    y = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.15)
    y += butter(np.random.default_rng(midi).standard_normal(n), [300, 2500], 'band', 2) * np.exp(-t / 0.012) * 0.4
    y = np.tanh(2.2 * y) / np.tanh(2.2)
    return S.pan2(y * vel, pan).astype(np.float32)


# ---------------------------------------------------------- tonal synths

def riff_synth(midi, dur, vel=1.0, cut=700.0, env=3800.0, decay=0.055, q=2.4, drive=3.2, r=0.02, width=0.25):
    """Sharp, distorted riff voice: two detuned saws + a square an octave down,
    resonant low-pass with a fast envelope, hard tanh drive.  (2, n)"""
    n = int((dur + 6 * r) * SR)
    t = np.arange(n) / SR
    f = hz(midi)
    a = saw(f * 2 ** (-0.07 / 12), n, 0.1)
    b = saw(f * 2 ** (0.07 / 12), n, 0.6)
    sq = np.sign(sine(f / 2, n)) * 0.3
    fc = cut + env * np.exp(-t / decay)
    xa = svf(a + sq, fc, q)
    xb = svf(b + sq, fc, q)
    e = np.where(t < dur, 1.0, np.exp(-(t - dur) / r)) * np.minimum(t / 0.0015, 1)
    xa = np.tanh(drive * xa * e) / np.tanh(drive)
    xb = np.tanh(drive * xb * e) / np.tanh(drive)
    c = (xa + xb) / 2
    s = (xa - xb) / 2 * width
    y = np.stack([c + s, c - s])
    y = butter(y, 120, 'high', 2)
    y = butter(y, 6500, 'low', 2)
    return (y * vel).astype(np.float32)


def gated_stab(midis, dur, vel=1.0, cut=1100.0, env=3200.0, decay=0.07, drive=2.2, rel=0.012):
    """Chord stab with a hard gate: detuned saws per note, filter envelope,
    saturation, flat sustain and a fast (gate) release.  (2, n)"""
    n = int((dur + 6 * rel) * SR)
    t = np.arange(n) / SR
    L = np.zeros(n)
    R = np.zeros(n)
    g = np.random.default_rng(int(sum(midis)) + int(dur * 1000))
    for md in midis:
        for o, p in ((-0.12, -0.7), (0.0, 0.0), (0.12, 0.7)):
            w = saw(hz(md + o), n, g.random())
            L += w * np.sqrt((1 - p) / 2)
            R += w * np.sqrt((1 + p) / 2)
    x = np.stack([L, R]) / (len(midis) * 1.7)
    fc = cut + env * np.exp(-t / decay)
    x = svf(x, fc, 1.3)
    gate = np.where(t < dur, 1.0, np.exp(-(t - dur) / rel)) * np.minimum(t / 0.002, 1)
    x = np.tanh(drive * x * gate) / np.tanh(drive)
    x = butter(x, 160, 'high', 2)
    return (x * vel).astype(np.float32)


def dark_pad(midi, dur, vel=1.0, cut=800.0, a=1.0, r=1.5):
    return I.dark_pad(midi, dur, vel, cut=cut, a=a, r=r)


def drone(midi, n, vel=1.0, seed=0):
    return I.drone(midi, n, vel, seed=seed)


def bell(midi, dur=2.0, vel=1.0):
    """Cold FM bell for the breakdown riff."""
    return S.bell(midi, dur * 0.4, vel, r=dur * 0.5).astype(np.float32)


def sub(midi, dur, vel=1.0):
    return S.sub_bass(midi, dur, vel, a=0.004, r=0.04, drive=1.3).astype(np.float32)


# ------------------------------------------------------------------- FX

def gong_hit(semis=-3.0, vel=1.0, maxlen=4.0):
    x = I.load(I.VSCO + 'VSCO 1 Percussion/varMetal/Gong/gong_hit_f.wav')
    y = I.repitch(x, semis, maxlen=maxlen, fade=1.0)
    y = butter(y, 3500, 'low', 2)
    return (y * vel).astype(np.float32)


def gong_scrape(semis=-5.0, vel=1.0):
    return I.gong_scrape(semis, vel)


def riser(length, f0=300.0, f1=6000.0):
    y = S.noise_riser(length, f0, f1, q=2.0)
    return butter(y, 7000, 'low', 2).astype(np.float32)


def rev_swell(length):
    return I.rev_swell(length)


def impact(vel=1.0):
    return S.impact(vel).astype(np.float32)
