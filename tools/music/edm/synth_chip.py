# synth_chip.py -- NEW FILE (2026-10-13), Stellar Frontier Enhanced Edition.
#
# Chiptune voices for the song-4 remix (tools/music/edm/song4_chiptune.py),
# built on synth.py (which this file imports and does not change).  These
# model the NES 2A03 sound chip, band-limited so nothing aliases:
#
#   pulse     band-limited pulse wave (difference of two alias-free saw
#             wavetables), 12.5 / 25 / 50 % duty, per-sample duty
#             sweeps, 4-bit volume envelopes stepped at the 60 Hz frame rate,
#             delayed vibrato, pitch slides and 50 Hz chord arpeggios
#   triangle  the 2A03's 32-step (4-bit) triangle, rebuilt per pitch from its
#             exact Fourier series up to Nyquist (a band-limited wavetable)
#   noise     the 15-bit LFSR noise channel, long (32767-step) and short
#             (93-step, metallic) modes, at the real NTSC noise periods,
#             rendered 4x oversampled and decimated
#   drums     chip hats / snare / china / triangle-drop kick and toms
import numpy as np
import scipy.signal as ss

from synth import SR, hz, butter, rng

FRAME = 60.0          # NES frame rate: volume envelopes step at this rate
ARP_RATE = 50.0       # arpeggio step rate (Hz): the classic 50 Hz chord arp
CPU = 1789773.0       # NTSC 2A03 clock
NOISE_PERIODS = [4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068]


# ------------------------------------------------------------ envelopes

def chip_env(n, gate, a=0.0, d=0.15, s=0.6, r=0.03, steps=15):
    """ADSR quantised to 4-bit volume (16 levels) and held per 60 Hz frame,
    then smoothed by ~1 ms so the frame steps do not click."""
    t = np.arange(n) / SR
    tf = np.floor(t * FRAME) / FRAME           # the frame each sample sits in
    if a > 0:
        att = np.minimum(tf / a, 1.0)
    else:
        att = np.ones(n)
    dec = s + (1 - s) * np.exp(-np.maximum(tf - a, 0) / max(d, 1e-4))
    e = np.where(tf < a, att, dec)
    g_end = (s + (1 - s) * np.exp(-max(gate - a, 0) / max(d, 1e-4))) if gate >= a else gate / a
    rel = tf >= gate
    e[rel] = g_end * np.exp(-(tf[rel] - gate) / max(r, 1e-4))
    e = np.round(e * steps) / steps
    # one-pole smoothing (~1 ms) plus a 2 ms attack ramp
    e = ss.lfilter([1 - np.exp(-1 / (0.0010 * SR))], [1, -np.exp(-1 / (0.0010 * SR))], e)
    e *= np.minimum(t / 0.002, 1.0)
    # guarantee a clean end
    k = min(n, int(0.004 * SR))
    e[-k:] *= np.linspace(1, 0, k)
    return e


def pitch_curve(n, midi, arp=None, vib=0.0, vib_rate=6.0, vib_delay=0.18, slide=0.0,
                slide_tau=0.025, bend=0.0, bend_time=0.0):
    """Per-sample MIDI pitch: base + arpeggio steps + delayed vibrato + slide-in."""
    t = np.arange(n) / SR
    p = np.full(n, float(midi))
    if arp:
        step = (np.floor(t * ARP_RATE).astype(int)) % len(arp)
        p += np.asarray(arp, float)[step]
    if vib:
        amt = np.clip((t - vib_delay) / 0.25, 0, 1) * vib
        p += amt * np.sin(2 * np.pi * vib_rate * t)
    if slide:
        p += slide * np.exp(-t / slide_tau)
    if bend:
        p += bend * np.clip(t / max(bend_time, 1e-3), 0, 1) ** 2
    return p


# ---------------------------------------------------------------- pulse

_saw_tables = {}
STBL = 8192


def _saw_table(kmax):
    """One period of a band-limited saw (kmax harmonics, exact Fourier series)."""
    if kmax not in _saw_tables:
        spec = np.zeros(STBL // 2 + 1, complex)
        k = np.arange(1, min(kmax, STBL // 2 - 1) + 1)
        spec[k] = -1j * STBL / (np.pi * k)
        _saw_tables[kmax] = np.fft.irfft(spec, STBL)
    return _saw_tables[kmax]


def _read(tbl, ph):
    x = ph * len(tbl)
    i0 = x.astype(np.int64) % len(tbl)
    fr = x - np.floor(x)
    return tbl[i0] * (1 - fr) + tbl[(i0 + 1) % len(tbl)] * fr


def pulse_wave(midi_curve, duty, phase=0.0):
    """Band-limited pulse = saw(ph) - saw(ph - duty), both from an alias-free
    wavetable, so per-sample duty sweeps stay band-limited too.  Zero-mean."""
    n = len(midi_curve)
    f = hz(midi_curve)
    kmax = max(1, int(0.45 * SR / max(float(f.max()), 20.0)))
    tbl = _saw_table(kmax)
    d = np.broadcast_to(np.asarray(duty, float), (n,))
    ph = (phase + np.cumsum(f / SR) - f[0] / SR) % 1.0
    return 0.5 * (_read(tbl, ph) - _read(tbl, (ph - d) % 1.0))


def pulse(midi, dur, duty=0.25, duty_end=None, duty_time=0.25, a=0.0, d=0.15, s=0.6, r=0.03,
          arp=None, vib=0.0, vib_rate=6.0, vib_delay=0.18, slide=0.0, slide_tau=0.025,
          bend=0.0, bend_time=0.0, vel=1.0, cents=0.0):
    """One pulse-channel note (mono). duty_end: duty sweep target over duty_time s."""
    n = int((dur + 6 * r + 0.01) * SR)
    pc = pitch_curve(n, midi + cents / 100.0, arp, vib, vib_rate, vib_delay, slide, slide_tau,
                     bend, bend_time)
    if duty_end is not None:
        t = np.arange(n) / SR
        # duty changes per frame (as a tracker's duty macro does)
        u = np.clip(np.floor(t * FRAME) / FRAME / max(duty_time, 1e-3), 0, 1)
        dc = duty + (duty_end - duty) * u
    else:
        dc = duty
    y = pulse_wave(pc, dc, rng().random())
    return (y * chip_env(n, dur, a, d, s, r) * vel * 0.5).astype(np.float32)


# ------------------------------------------------------------- triangle

_TRI32 = np.concatenate([np.arange(15, -1, -1), np.arange(0, 16)]).astype(float)
_TRI32 = (_TRI32 - 7.5) / 7.5
_TRI_DFT = np.fft.fft(_TRI32) / 32.0
_tables = {}
TBL = 4096


def _tri_table(kmax):
    """One period of the 32-step triangle, band-limited to kmax harmonics."""
    if kmax not in _tables:
        spec = np.zeros(TBL // 2 + 1, complex)
        for k in range(1, min(kmax, TBL // 2 - 1) + 1):
            # zero-order hold of the 32 steps: DFT bin (aliased) x sinc x half-step delay
            c = _TRI_DFT[k % 32] * np.sinc(k / 32.0) * np.exp(-1j * np.pi * k / 32.0)
            spec[k] = c * TBL
        _tables[kmax] = np.fft.irfft(spec, TBL)
    return _tables[kmax]


def triangle(midi, dur, r=0.008, slide=0.0, slide_tau=0.02, bend=0.0, bend_time=0.0, vib=0.0,
             vel=1.0):
    """NES triangle note (mono): no volume control on the chip, so only a gate."""
    n = int((dur + 6 * r + 0.005) * SR)
    pc = pitch_curve(n, midi, None, vib, 5.5, 0.25, slide, slide_tau, bend, bend_time)
    f = hz(pc)
    kmax = int(0.45 * SR / max(f.max(), 20.0))
    tbl = _tri_table(kmax)
    ph = (0.25 + np.cumsum(f / SR) - f[0] / SR) % 1.0     # start at a zero crossing
    x = ph * TBL
    i0 = x.astype(int) % TBL
    fr = x - np.floor(x)
    y = tbl[i0] * (1 - fr) + tbl[(i0 + 1) % TBL] * fr
    t = np.arange(n) / SR
    g = np.where(t < dur, 1.0, np.exp(-(t - dur) / r)) * np.minimum(t / 0.0015, 1)
    k = min(n, int(0.003 * SR))
    g[-k:] *= np.linspace(1, 0, k)
    return (y * g * vel * 0.8).astype(np.float32)


# ---------------------------------------------------------------- noise

def _lfsr(short):
    reg = 1
    tap = 6 if short else 1
    out = np.empty(32767, np.float32)
    for i in range(32767):
        fb = (reg ^ (reg >> tap)) & 1
        reg = (reg >> 1) | (fb << 14)
        out[i] = 1.0 if (reg & 1) == 0 else -1.0
    if short:
        # the short mode cycles in 93 steps; keep one clean cycle
        for p in (93, 31):
            if np.array_equal(out[100:100 + p], out[100 + p:100 + 2 * p]):
                return out[100:100 + p].copy()
    return out


_SEQ = {False: _lfsr(False), True: _lfsr(True)}
OS = 4


def noise(dur, period=3, short=False, a=0.0, d=0.08, s=0.0, r=0.03, period_end=None, sweep_time=0.1,
          vel=1.0, steps=15):
    """NES noise channel hit (mono). period: index 0-15 into the NTSC noise period table
    (0 = brightest); period_end sweeps the index (like a tracker pitch macro)."""
    n = int((dur + 6 * r + 0.01) * SR)
    no = n * OS
    t = np.arange(no) / (SR * OS)
    if period_end is None:
        rate = np.full(no, CPU / NOISE_PERIODS[period])
    else:
        idx = period + (period_end - period) * np.clip(np.floor(t * FRAME) / FRAME / sweep_time, 0, 1)
        idx = np.clip(np.round(idx).astype(int), 0, 15)
        rate = CPU / np.asarray(NOISE_PERIODS, float)[idx]
    seq = _SEQ[short]
    pos = np.floor(np.cumsum(rate / (SR * OS))).astype(np.int64) % len(seq)
    x = seq[pos]
    y = ss.resample_poly(x, 1, OS)[:n]
    return (y * chip_env(n, dur, a, d, s, r, steps) * vel * 0.5).astype(np.float32)


# ---------------------------------------------------------------- drums

def chip_hat(vel=1.0, open_=False):
    if open_:
        return noise(0.12, 0, False, d=0.09, s=0.0, r=0.05, vel=vel)
    return noise(0.03, 1, False, d=0.018, s=0.0, r=0.01, vel=vel)


def chip_shaker(vel=1.0):
    """The original's cabasa, as a short bright noise tick."""
    return noise(0.04, 2, False, a=0.008, d=0.02, s=0.0, r=0.01, vel=vel)


def chip_snare(vel=1.0):
    """Noise snare: bright attack sweeping down, plus a short triangle 'body' drop."""
    nz = noise(0.16, 4, False, d=0.06, s=0.0, r=0.04, period_end=8, sweep_time=0.08, vel=vel)
    body = triangle(55, 0.04, r=0.01, bend=-14, bend_time=0.04, vel=0.7 * vel)
    out = nz
    out[:len(body)] += body
    return out


def chip_china(vel=1.0):
    """The original's china cymbal: long-mode noise with a slow 4-bit decay."""
    a = noise(1.2, 2, False, d=0.45, s=0.0, r=0.3, vel=vel)
    b = noise(1.2, 0, True, d=0.12, s=0.0, r=0.1, vel=vel * 0.35)
    a[:len(b)] += b
    return a


def chip_kick(vel=1.0, midi=43):
    """Triangle pitch-drop kick (the NES trick) with a noise click."""
    body = triangle(midi, 0.09, r=0.02, bend=-24, bend_time=0.07, vel=vel)
    click = noise(0.012, 6, False, d=0.006, s=0.0, r=0.004, vel=0.6 * vel)
    body[:len(click)] += click
    return body


def chip_tom(midi, vel=1.0):
    """Triangle tom: a quick downward slide."""
    return triangle(midi, 0.14, r=0.03, bend=-7, bend_time=0.14, vel=vel)


def blip(midi, vel=1.0, duty=0.125):
    """Very short pulse blip (the original's claves)."""
    return pulse(midi, 0.025, duty, d=0.015, s=0.0, r=0.01, vel=vel)
