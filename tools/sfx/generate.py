"""Stellar Frontier remake -- sound-effect generator.

Renders every effect the game uses into assets/sfx/<name>_<NN>.ogg (48 kHz,
Ogg Vorbis).  Each effect is layered (transient, body, sub, tail, space) and
rendered several times with different seeds so the game can rotate variants
instead of repeating one sample.

    pip install numpy scipy soundfile numba     # numba optional, just faster
    python3 tools/sfx/generate.py               # all effects
    python3 tools/sfx/generate.py explode hit   # only names starting so
    python3 tools/sfx/generate.py --report      # loudness/peak table only

Needs ffmpeg with libvorbis on PATH.  Deterministic: same code, same files.
"""
import math
import os
import subprocess
import sys
import tempfile

import numpy as np
import soundfile as sf

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dsp import *  # noqa: E402,F403

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "assets", "sfx")

SOUNDS = {}   # name -> (fn, variants, loudness_db, loop, max_seconds)


def sound(name, variants=1, loud=-18.0, loop=False, maxlen=0.0):
    def reg(fn):
        SOUNDS[name] = (fn, variants, loud, loop, maxlen)
        return fn
    return reg


def J(rng, x, amount=0.08):
    """Jitter a parameter by +-amount (relative)."""
    return x * (1.0 + rng.uniform(-amount, amount))


# ==================================================================== weapons

@sound("torpedo", 5, -15.0, maxlen=1.0)
def torpedo(rng, v):
    """Photon torpedo: bright zap that drops into a thump, with a fizzing tail."""
    d = 0.55
    n = n_of(d)
    f0, f1 = J(rng, 1700), J(rng, 150)
    pitch = ramp(n, f0, f1) * (1 + 0.04 * np.sin(2 * math.pi * J(rng, 38) * time(d)))
    idx = env_points(n, [(0, 4.0), (0.08, 1.2), (d, 0.2)], 2.0)
    body = fm_op(pitch, J(rng, 1.48, 0.03), idx, n) * 0.6 + saw(pitch * 0.5, n) * 0.25
    body = lp(body, ramp(n, 9000, 700), 1.2) * env_points(n, [(0, 0), (0.002, 1), (0.05, 0.7), (0.25, 0.15), (d, 0)], 2.2)
    sub = sine(ramp(n, 140, 42), n) * env_points(n, [(0, 0), (0.003, 1), (0.12, 0.3), (0.3, 0)], 2.0) * 0.9
    click = hp(white(n, rng), 3000) * env_exp(n, 0.004) * 0.8
    fizz = bp(white(n, rng), ramp(n, 6000, 1500), 2.0) * env_points(n, [(0, 0), (0.02, 0.35), (d, 0)], 1.5)
    x = drive(body + sub + click + fizz * 0.5, 1.6)
    x = mix(widen(x, 0.35), reverb(x, 0.18, 0.35, seed=v) * 0.6)
    return x


@sound("rapid", 7, -18.0, maxlen=0.38)
def rapid(rng, v):
    """Rapid-fire pulse: tight and percussive so it can repeat 10x a second."""
    d = 0.16
    n = n_of(d)
    pitch = ramp(n, J(rng, 2600, 0.12), J(rng, 520))
    tone = square(pitch, n, pw=J(rng, 0.3, 0.2)) * 0.4 + sine(pitch * 0.5, n) * 0.6
    tone = lp(tone, ramp(n, 12000, 1500), 1.4) * env_points(n, [(0, 0), (0.001, 1), (0.03, 0.35), (d, 0)], 2.5)
    click = hp(white(n, rng), 5000) * env_exp(n, 0.0025)
    thump = sine(ramp(n, 180, 70), n) * env_exp(n, 0.025) * 0.6
    x = drive(tone + click * 0.7 + thump, 1.3)
    return mix(widen(x, 0.25), reverb(x, 0.1, 0.2, seed=v) * 0.5)


@sound("plasma", 5, -14.5)
def plasma(rng, v):
    """Plasma bolt: a fat, wobbling ball of energy with a resonant sweep."""
    d = 0.9
    n = n_of(d)
    t = time(d)
    base = J(rng, 92)
    wob = 1 + 0.07 * np.sin(2 * math.pi * J(rng, 17, 0.2) * t) * env_points(n, [(0, 1), (d, 0.3)])
    pitch = ramp(n, base * 2.6, base) * wob
    ss = supersaw(pitch, n, 7, J(rng, 0.018), rng)
    cut = ramp(n, J(rng, 5200), 260) * (1 + 0.35 * np.sin(2 * math.pi * J(rng, 23) * t))
    body = lp(ss, cut, 4.5) * env_points(n, [(0, 0), (0.006, 1), (0.12, 0.7), (0.45, 0.25), (d, 0)], 1.8)
    fm = fm_op(pitch * 0.5, 2.01, env_points(n, [(0, 6), (0.3, 1), (d, 0.2)]), n) * env_points(n, [(0, 0), (0.004, 0.9), (0.35, 0.2), (d, 0)], 2)
    sub = sine(ramp(n, 110, 38), n) * env_points(n, [(0, 0), (0.004, 1), (0.2, 0.25), (0.5, 0)], 2) * 0.9
    sizzle = bp(white(n, rng), ramp(n, 7000, 2000), 3.0) * crackle(n, rng, ramp(n, 900, 80), 0.002) * 18
    x = drive(body * 0.9 + fm * 0.45 + sub + sizzle * 0.4, 2.2)
    x = mix(autopan(x, J(rng, 3.0), 0.25, rng.random() * 6), reverb(x, 0.25, 0.55, seed=v) * 0.6)
    return x


@sound("grenade", 4, -15.5)
def grenade(rng, v):
    """Grenade launcher: hollow tube 'thoomp', breech clank, round whooshing off."""
    d = 0.8
    n = n_of(d)
    pop = sine(ramp(n, J(rng, 260), 55), n) * env_points(n, [(0, 0), (0.002, 1), (0.06, 0.4), (0.22, 0)], 2)
    tube = bp(pink(n, rng), J(rng, 420), 3.5) * env_exp(n, 0.06) * 2.0
    clank_ex = white(n, rng) * env_exp(n, 0.002)
    clank = modal(clank_ex, metal_modes(J(rng, 820), 7, rng, (0.04, 0.22))) * 6
    whoosh = bp(pink(n, rng), ramp(n, 2400, 500), 1.2) * env_points(n, [(0, 0), (0.05, 0.6), (0.2, 0.3), (d, 0)], 1.4)
    x = drive(pop + tube + whoosh * 0.6, 1.8)
    x = place(x, clank, 0.035, 0.4)
    return mix(widen(x, 0.4), reverb(x, 0.22, 0.5, seed=v) * 0.6)


@sound("missile", 4, -15.0)
def missile(rng, v):
    """Seeker launch: igniter crack, then a roaring rocket that tears away."""
    d = 1.5
    n = n_of(d)
    t = time(d)
    crack = drive(hp(white(n, rng), 1200) * env_exp(n, 0.012) * 3, 3)
    roar_src = mix(pink(n, rng) * 0.7, brown(n, rng) * 0.6)
    cut = ramp(n, 700, J(rng, 3800)) * (1 + 0.25 * np.sin(2 * math.pi * 7.3 * t + rng.random() * 6))
    roar = bp(roar_src, cut, 0.9)
    am = 1 + 0.35 * lp(white(n, rng), 30) * 4
    roar *= am * env_points(n, [(0, 0), (0.03, 1), (0.35, 0.75), (d, 0)], 1.4)
    burble = crackle(n, rng, ramp(n, 300, 40), 0.0012) * 6 * env_points(n, [(0, 1), (d, 0)])
    burble = bp(burble, 2500, 0.8)
    whine = sine(ramp(n, J(rng, 900), J(rng, 2100), "log"), n) * env_points(n, [(0, 0), (0.05, 0.18), (0.6, 0.08), (d, 0)])
    thump = sine(ramp(n, 120, 50), n) * env_exp(n, 0.05) * 0.8
    x = drive(crack * 0.7 + roar + burble * 0.5 + whine + thump, 1.6)
    # the rocket moves away: dull it and pan it off to one side
    x = lp(x, ramp(n, 16000, 2200))
    pan = rng.choice([-1, 1]) * 0.45
    x = stereo_moving(x, pan)
    return mix(x, reverb(to_mono(x), 0.3, 0.7, seed=v) * 0.5)


def stereo_moving(x, end_pan):
    n = len(x)
    p = np.linspace(0, end_pan, n)
    a = (p + 1) * math.pi / 4
    return np.stack([x * np.cos(a), x * np.sin(a)], axis=1) * math.sqrt(2)


@sound("mine", 3, -18.0)
def mine(rng, v):
    """Mine drop: release latch, heavy clunk, then a two-tone arming chirp."""
    d = 1.0
    n = n_of(d)
    ex = white(n, rng) * env_exp(n, 0.0015)
    latch = modal(ex, metal_modes(J(rng, 1500), 8, rng, (0.03, 0.12))) * 4
    clunk = sine(ramp(n, 150, 70), n) * env_exp(n, 0.05) + lp(white(n, rng), 600) * env_exp(n, 0.02) * 2
    clunk2 = modal(ex, metal_modes(J(rng, 380), 6, rng, (0.06, 0.25))) * 3
    x = np.zeros(n)
    x = place(x, latch, 0.0, 0.6)
    x = place(x, clunk + clunk2, 0.06, 0.9)
    beep_n = n_of(0.07)
    for k, (at, f) in enumerate([(0.42, 1900), (0.55, 2500)]):
        b = (sine(f, beep_n) + 0.3 * sine(f * 2, beep_n)) * env_points(beep_n, [(0, 0), (0.004, 1), (0.05, 0.7), (0.07, 0)])
        x = place(x, b, at, 0.22)
    x = drive(x, 1.4)
    return mix(widen(x, 0.3), reverb(x, 0.2, 0.45, seed=v) * 0.6)


@sound("cannon", 4, -13.0)
def cannon(rng, v):
    """Mass-driver cannon: a hard electric crack and a chest-thumping boom."""
    d = 1.8
    n = n_of(d)
    zap_p = ramp(n, J(rng, 4200), 300)
    zap = (saw(zap_p, n) * sine(J(rng, 730), n)) * env_points(n, [(0, 0), (0.001, 1), (0.05, 0.2), (0.12, 0)], 2)
    crack = drive(white(n, rng) * env_exp(n, 0.008) * 4, 4)
    boom = sine(ramp(n, J(rng, 95), 28), n) * env_points(n, [(0, 0), (0.004, 1), (0.25, 0.4), (0.9, 0)], 1.6)
    body = lp(brown(n, rng), ramp(n, 5000, 250)) * env_points(n, [(0, 0), (0.003, 1.2), (0.15, 0.5), (1.2, 0)], 2) * 1.6
    ring = modal(white(n, rng) * env_exp(n, 0.002), metal_modes(J(rng, 240), 6, rng, (0.2, 0.7))) * 2.5
    x = drive(zap * 0.4 + crack * 0.5 + boom * 1.1 + body + ring * 0.3, 2.0)
    x = eq(x, "lowshelf", 90, 3.0)
    return mix(widen(x, 0.3), reverb(x, 0.32, 1.0, seed=v) * 0.7)


def _beam_core(rng, n, base):
    t = np.arange(n) / RATE
    hum = (saw(base, n) + saw(base * 1.006, n, 0.3) + 0.6 * saw(base * 2.003, n, 0.7)) / 2.2
    hum = bp(hum, base * 6 * (1 + 0.15 * np.sin(2 * math.pi * 0.7 * t)), 2.5) * 1.8
    buzz = square(base * 0.5, n) * 0.25
    am = 0.75 + 0.25 * np.sin(2 * math.pi * 31 * t)
    hiss = bp(white(n, rng), 5200, 1.5) * (0.5 + 0.5 * lp(np.abs(white(n, rng)) * 3, 40)) * 0.6
    sparkle = hp(crackle(n, rng, 260, 0.0008), 3000) * 10
    return (hum + buzz) * am + hiss + sparkle * 0.35


def beam_loop(rng, v):
    """Sustained laser beam, seamless loop (pitched by the game per ship)."""
    d = 2.75
    n = n_of(d)
    x = hp(drive(_beam_core(rng, n, 110.0), 1.5), 30)
    x = widen(x, 0.4)
    return make_loop(x, 0.25)


def beam_start(rng, v):
    """Beam ignition: a bright up-chirp that lands on the loop's hum."""
    d = 0.4
    n = n_of(d)
    chirp = saw(ramp(n, 300, J(rng, 2400), "log"), n) * env_points(n, [(0, 0), (0.005, 1), (0.12, 0.5), (d, 0)], 1.5)
    chirp = lp(chirp, 7000)
    core = _beam_core(rng, n, 110.0) * env_points(n, [(0, 0), (0.06, 1), (0.2, 0.6), (d, 0)])
    click = hp(white(n, rng), 3000) * env_exp(n, 0.003)
    x = drive(chirp * 0.5 + core + click * 0.6, 1.5)
    return widen(x, 0.4)


def beam_stop(rng, v):
    """Beam cut-off: the hum winds down and spits."""
    d = 0.45
    n = n_of(d)
    p = ramp(n, 110.0, J(rng, 40))
    hum = saw(p, n) + saw(p * 2.01, n) * 0.5
    hum = lp(hum, ramp(n, 2400, 300), 2.0) * env_points(n, [(0, 1), (d, 0)], 2.5)
    spit = hp(crackle(n, rng, ramp(n, 400, 0), 0.001), 2500) * 8
    return widen(drive(hum * 0.8 + spit * 0.4, 1.3), 0.4)


@sound("beam", 4, -18.0, maxlen=1.0)
def beam(rng, v):
    """One-shot beam zap (planetary defence batteries)."""
    d = 0.5
    n = n_of(d)
    p = ramp(n, J(rng, 1800), J(rng, 400))
    z = saw(p, n) * 0.5 + _beam_core(rng, n, J(rng, 140)) * 0.7
    z = lp(z, ramp(n, 9000, 1200), 1.5) * env_points(n, [(0, 0), (0.003, 1), (0.1, 0.5), (d, 0)], 1.8)
    x = drive(z, 1.6)
    return mix(widen(x, 0.4), reverb(x, 0.2, 0.5, seed=v) * 0.6)


# ==================================================================== impacts

@sound("hit", 7, -17.0, maxlen=1.2)
def hit(rng, v):
    """Hull hit: struck armour plate, crunch, and a dull thud through the frame."""
    d = 0.9
    n = n_of(d)
    ex = white(n, rng) * env_exp(n, J(rng, 0.0025, 0.4))
    plate = modal(ex, metal_modes(J(rng, 520, 0.35), 12, rng, (0.05, 0.45), 6.0)) * 3.5
    crunch = drive(lp(white(n, rng), J(rng, 3500)) * env_points(n, [(0, 0), (0.001, 1), (0.04, 0.4), (0.15, 0)], 2) * 2, 3)
    thud = sine(ramp(n, J(rng, 160), 55), n) * env_points(n, [(0, 0), (0.002, 1), (0.08, 0.3), (0.2, 0)], 2)
    debris = bp(crackle(n, rng, ramp(n, 500, 0), 0.0008), 4000, 1.0) * 6
    x = drive(plate * 0.8 + crunch * 0.6 + thud * 0.9 + debris * 0.3, 1.8)
    return mix(widen(x, 0.35), reverb(x, 0.2, 0.4, seed=v) * 0.6)


def shield(rng, v):
    """Shield deflection: an electric shimmer that rings and ripples away."""
    d = 0.85
    n = n_of(d)
    t = time(d)
    f = J(rng, 620, 0.2)
    idx = env_points(n, [(0, 5.0), (0.1, 2.0), (d, 0.3)])
    bell = (fm_op(f, 1.414, idx, n) + 0.6 * fm_op(f * 1.5, 2.76, idx * 0.6, n)) * env_points(n, [(0, 0), (0.003, 1), (0.15, 0.35), (d, 0)], 1.6)
    ripple = 1 + 0.5 * np.sin(2 * math.pi * ramp(n, 45, 12) * t)
    zap = bp(white(n, rng), ramp(n, 9000, 2500), 4.0) * env_points(n, [(0, 0), (0.002, 1.4), (0.06, 0.3), (0.3, 0)], 2)
    sub = sine(ramp(n, 200, 90), n) * env_exp(n, 0.05) * 0.4
    x = drive(bell * ripple * 0.5 + zap * 0.8 + sub, 1.4)
    x = mix(autopan(x, J(rng, 5.0), 0.35, rng.random() * 6), reverb(x, 0.3, 0.6, seed=v) * 0.6)
    return x


@sound("shield_down", 2, -15.0)
def shield_down(rng, v):
    """Shields collapse: a descending glassy sweep breaking into static."""
    d = 1.3
    n = n_of(d)
    p = ramp(n, J(rng, 1400), 120)
    tone = fm_op(p, 1.5, env_points(n, [(0, 3), (d, 8)]), n) * env_points(n, [(0, 0), (0.01, 1), (0.5, 0.5), (d, 0)], 1.4)
    stat = bp(white(n, rng), ramp(n, 3000, 800), 1.0) * crackle(n, rng, ramp(n, 50, 2500), 0.004) * 25 * env_points(n, [(0, 0), (0.3, 0.5), (d, 0)])
    x = drive(tone * 0.6 + stat * 0.5, 1.6)
    return mix(widen(x, 0.5), reverb(x, 0.3, 0.7, seed=v) * 0.6)


@sound("impact", 6, -16.0)
def impact(rng, v):
    """Torpedo / bolt detonation on a target: a small, crisp blast."""
    d = 1.1
    n = n_of(d)
    crack = drive(white(n, rng) * env_exp(n, 0.006) * 3, 3)
    pop = sine(ramp(n, J(rng, 180), 45), n) * env_points(n, [(0, 0), (0.002, 1), (0.1, 0.25), (0.3, 0)], 2)
    body = lp(brown(n, rng) + pink(n, rng) * 0.5, ramp(n, J(rng, 7000), 300)) * env_points(n, [(0, 0), (0.002, 1.3), (0.08, 0.5), (0.6, 0)], 2.2) * 1.5
    deb = bp(crackle(n, rng, ramp(n, 700, 0), 0.0007), 3500, 0.8) * 8
    x = drive(crack * 0.5 + pop + body + deb * 0.3, 1.8)
    return mix(widen(x, 0.4), reverb(x, 0.25, 0.6, seed=v) * 0.6)


@sound("collide", 4, -16.0)
def collide(rng, v):
    """Hull-on-hull or hull-on-rock: a heavy bump and a short grinding scrape."""
    d = 1.0
    n = n_of(d)
    ex = lp(white(n, rng), 1500) * env_exp(n, 0.01)
    body = modal(ex, metal_modes(J(rng, 140), 10, rng, (0.1, 0.5), 3.0)) * 5
    thud = sine(ramp(n, 90, 35), n) * env_points(n, [(0, 0), (0.004, 1), (0.15, 0.3), (0.4, 0)], 2)
    scrape_src = bp(white(n, rng), ramp(n, J(rng, 1800), 700), 6.0) * (0.6 + 0.4 * lp(np.abs(white(n, rng)) * 4, 60))
    scrape = scrape_src * env_points(n, [(0, 0), (0.03, 0.9), (0.35, 0.4), (0.7, 0)]) * 1.4
    x = drive(body * 0.6 + thud + scrape * 0.6, 1.8)
    return mix(widen(x, 0.3), reverb(x, 0.2, 0.5, seed=v) * 0.6)


# ================================================================= explosions

def _blast(rng, n, size, d):
    """Core of a ship explosion; size ~ 1 (fighter) .. 2.5 (station)."""
    t = np.arange(n) / RATE
    crack = drive(hp(white(n, rng), 400) * env_exp(n, 0.01 * size) * 4, 4)
    fire = lp(mix(brown(n, rng), pink(n, rng) * 0.6), ramp(n, J(rng, 9000) / size, 140, "exp")) * 1.8
    fire *= env_points(n, [(0, 0), (0.004, 1.3), (0.12 * size, 0.85), (0.6 * size, 0.35), (d, 0)], 1.6)
    # turbulent rolling of the fireball
    fire *= 1 + 0.4 * lp(white(n, rng), 9 / size) * 6
    boom = sine(ramp(n, J(rng, 75) / size ** 0.4, 22), n) * env_points(n, [(0, 0), (0.006, 1), (0.3 * size, 0.45), (0.9 * size, 0)], 1.5)
    deb_env = env_points(n, [(0, 0), (0.05, 1), (0.5 * size, 0.4), (d, 0)], 1.3)
    debris = bp(crackle(n, rng, 1800 * deb_env, 0.0006), 3200, 0.7) * 9 * deb_env
    pings = np.zeros(n)
    for _ in range(int(4 * size)):
        at = rng.uniform(0.08, 0.7 * d)
        ex = white(n_of(0.6), rng) * env_exp(n_of(0.6), 0.0012)
        p = modal(ex, metal_modes(rng.uniform(600, 2600), 6, rng, (0.05, 0.3))) * rng.uniform(0.6, 1.4)
        pings = place(pings, p, at, 2.5)
    return crack * 0.5 + fire + boom * 1.2 + debris * 0.35 + pings * 0.25 * np.exp(-t / d)


@sound("explode", 6, -12.5)
def explode(rng, v):
    """A ship breaking up: crack, fireball, sub boom, sprays of debris."""
    d = 2.8
    n = n_of(d)
    x = drive(_blast(rng, n, J(rng, 1.0, 0.15), d), 1.7)
    x = eq(x, "lowshelf", 80, 3.0)
    st = mix(widen(x, 0.6), autopan(lp(x, 3000), J(rng, 0.4), 0.3) * 0.3)
    return mix(st, reverb(x, 0.35, 1.1, seed=v) * 0.7)


@sound("bigexplode", 4, -10.5)
def bigexplode(rng, v):
    """Capital ship or starbase going up: chained blasts and a long rumble."""
    d = 6.0
    n = n_of(d)
    x = _blast(rng, n, J(rng, 2.3, 0.1), d)
    # secondary detonations
    for _ in range(rng.integers(2, 4)):
        at = rng.uniform(0.25, 1.6)
        m = n_of(2.5)
        x = place(x, _blast(rng, m, rng.uniform(0.6, 1.1), 2.5), at, rng.uniform(0.35, 0.6))
    # long rumble with slow tremor
    t = time(d)
    rumble = lp(brown(n, rng), 120) * (1 + 0.5 * np.sin(2 * math.pi * 1.7 * t)) * env_points(n, [(0, 0), (0.2, 1), (2.0, 0.6), (d, 0)], 1.3) * 2.0
    x = drive(x + rumble, 1.6)
    x = eq(x, "lowshelf", 70, 4.0)
    st = mix(widen(x, 0.7), autopan(lp(x, 2500), J(rng, 0.25), 0.4) * 0.35)
    return mix(st, reverb(x, 0.4, 1.8, seed=v) * 0.7)


# ==================================================================== engines

def engine_small(rng, v):
    """Fighter drive: a fast turbine whine over a hissing exhaust."""
    d = 4.75
    n = n_of(d)
    t = time(d)
    exhaust = bp(pink(n, rng), 1100, 0.7)
    rumble = bp(brown(n, rng), 120, 0.6) * 1.6
    wob = 1 + 0.004 * np.sin(2 * math.pi * 0.47 * t)
    whine = (sine(440 * wob, n) * 0.5 + sine(880.5 * wob, n) * 0.22 + sine(1321 * wob, n) * 0.1) * 0.18
    pulse = 1 + 0.12 * np.sin(2 * math.pi * 40 * t)
    x = hp(drive((exhaust * 0.8 + rumble) * pulse + whine, 1.3), 30)
    return make_loop(widen(x, 0.5), 0.25)


@sound("engine_large", 1, -19.0, loop=True)
def engine_large(rng, v):
    """Capital drive: deep throbbing reactor and a heavy roar."""
    d = 5.75
    n = n_of(d)
    t = time(d)
    roar = bp(mix(brown(n, rng), pink(n, rng) * 0.4), 220, 0.5) * 1.6
    hum = (sine(55, n) + 0.5 * sine(110.3, n) + 0.3 * square(55.2, n) * 0.4)
    hum = lp(hum, 400) * (0.75 + 0.25 * np.sin(2 * math.pi * 0.95 * t)) * 0.6
    throb = 1 + 0.2 * np.sin(2 * math.pi * 12 * t)
    whine = sine(330 * (1 + 0.003 * np.sin(2 * math.pi * 0.3 * t)), n) * 0.05
    x = hp(drive(roar * throb + hum + whine, 1.4), 30)
    return make_loop(widen(x, 0.5), 0.25)


def boost_loop(rng, v):
    """Afterburner: hot ragged roar layered on top of the engine."""
    d = 3.75
    n = n_of(d)
    t = time(d)
    src = mix(pink(n, rng), white(n, rng) * 0.3)
    roar = bp(src, 1600 * (1 + 0.2 * lp(white(n, rng), 5) * 8), 0.6) * 1.6
    pops = bp(crackle(n, rng, 160, 0.002), 900, 0.7) * 14
    whine = sine(1250 * (1 + 0.005 * np.sin(2 * math.pi * 0.6 * t)), n) * 0.06
    x = hp(drive(roar + pops * 0.5 + whine, 2.0), 30)
    return make_loop(widen(x, 0.6), 0.25)


@sound("rocket_loop", 1, -19.0, loop=True)
def rocket_loop(rng, v):
    """A seeker in flight: thin, fast rocket burn that the game pans as it passes."""
    d = 2.75
    n = n_of(d)
    t = time(d)
    burn = bp(mix(pink(n, rng), white(n, rng) * 0.4), 2400 * (1 + 0.1 * np.sin(2 * math.pi * 6.1 * t)), 0.8) * 1.6
    pops = bp(crackle(n, rng, 220, 0.0012), 3500, 0.8) * 10
    whine = sine(1900 * (1 + 0.006 * np.sin(2 * math.pi * 0.9 * t)), n) * 0.05
    x = hp(drive(burn + pops * 0.4 + whine, 1.6), 200)
    return make_loop(widen(x, 0.3), 0.25)


# =================================================================== warp etc

def warpin(rng, v):
    """Quantum drive spooling up (1.1 s charge): rising stacked tones and pressure."""
    d = 1.25
    n = n_of(d)
    t = time(d)
    tones = np.zeros(n)
    for i, r in enumerate([1.0, 1.5, 2.0, 3.0]):
        p = ramp(n, J(rng, 70) * r, J(rng, 900) * r, "exp")
        tones += saw(p, n, rng.random()) / (1 + i)
    trem = 0.6 + 0.4 * np.sin(2 * math.pi * np.cumsum(ramp(n, 4, 40)) / RATE)
    tones = lp(tones, ramp(n, 600, 9000), 2.0) * trem
    press = bp(pink(n, rng), ramp(n, 300, 5000), 1.2) * 1.5
    env = env_points(n, [(0, 0), (0.9, 0.8), (1.1, 1.0), (d, 0)], 1.0)
    x = drive((tones * 0.5 + press * 0.6) * env, 1.6)
    return mix(widen(x, 0.6), reverb(x, 0.25, 0.6, seed=v) * 0.5)


def warpout(rng, v):
    """Jump to quantum speed: a thunderclap and a tearing whoosh into the distance."""
    d = 2.6
    n = n_of(d)
    clap = drive(white(n, rng) * env_exp(n, 0.02) * 3, 3)
    boom = sine(ramp(n, 120, 25), n) * env_points(n, [(0, 0), (0.004, 1), (0.4, 0.35), (1.2, 0)], 1.5)
    whoosh = bp(pink(n, rng), ramp(n, 6000, 180), 0.9) * env_points(n, [(0, 0), (0.01, 1.4), (0.4, 0.6), (d, 0)], 1.5) * 1.6
    zap = saw(ramp(n, 2500, 60), n) * env_points(n, [(0, 0), (0.002, 1), (0.25, 0.2), (0.6, 0)], 2) * 0.4
    x = drive(clap * 0.4 + boom + whoosh + lp(zap, 6000), 1.8)
    return mix(widen(x, 0.7), reverb(x, 0.4, 1.3, seed=v) * 0.7)


@sound("warpexit", 2, -14.0)
def warpexit(rng, v):
    """Dropping out of quantum: a reverse-swelled rush that lands with a thump."""
    d = 1.3
    n = n_of(d)
    swell = bp(pink(n, rng), ramp(n, 200, 4500), 1.0) * env_points(n, [(0, 0), (0.45, 1), (0.5, 0.3), (d, 0)], 1.0) * 1.5
    land = np.zeros(n)
    m = n_of(0.8)
    land = place(land, sine(ramp(m, 140, 35), m) * env_points(m, [(0, 0), (0.003, 1), (0.3, 0)], 1.6), 0.45)
    land = place(land, drive(white(m, rng) * env_exp(m, 0.01) * 2, 2), 0.45, 0.4)
    down = saw(ramp(n, 900, 80), n) * env_points(n, [(0, 0), (0.45, 0), (0.47, 1), (d, 0)], 2) * 0.25
    x = drive(swell + land + lp(down, 3000), 1.6)
    return mix(widen(x, 0.6), reverb(x, 0.35, 0.9, seed=v) * 0.6)


def _cloak_core(rng, d):
    n = n_of(d)
    t = time(d)
    src = supersaw(ramp(n, J(rng, 660), 110), n, 5, 0.02, rng) * 0.5 + pink(n, rng) * 0.6
    sweep = 600 + 4500 * (0.5 + 0.5 * np.sin(2 * math.pi * ramp(n, 0.8, 5) * t))
    x = svf(src, sweep, 6.0, "notch")
    x = bp(x, ramp(n, 5000, 400), 1.2) * env_points(n, [(0, 0), (0.05, 1), (0.6 * d, 0.5), (d, 0)], 1.5)
    return drive(x * 1.5, 1.3)


@sound("cloak", 2, -18.0)
def cloak(rng, v):
    """Cloak engaging: phasing shimmer that folds away to nothing."""
    x = _cloak_core(rng, 1.3)
    return mix(autopan(x, 1.5, 0.6), reverb(x, 0.4, 0.8, seed=v) * 0.6)


@sound("uncloak", 2, -17.0)
def uncloak(rng, v):
    """Decloaking: the shimmer unfolds in reverse and snaps into solid form."""
    d = 0.9
    x = _cloak_core(rng, d)[::-1].copy()
    n = len(x)
    m = n_of(0.3)
    snap = drive(hp(white(m, rng), 1500) * env_exp(m, 0.006) * 3, 2) + sine(ramp(m, 180, 60), m) * env_exp(m, 0.05)
    y = place(pad(x, n + m), snap, d - 0.04, 0.6)
    return mix(autopan(y, 1.5, 0.6), reverb(y, 0.3, 0.6, seed=v) * 0.6)


# ========================================================== player feedback / UI

def _bell(f, n, decay=0.6, bright=2.0):
    return fm_op(f, 3.5, env_points(n, [(0, bright), (decay, 0.2)]), n) * env_exp(n, decay * 0.5, 0.002)


@sound("pickup", 3, -17.0, maxlen=1.2)
def pickup(rng, v):
    """Module salvaged: a sparkling rising arpeggio."""
    d = 1.0
    n = n_of(d)
    root = [523.25, 587.33, 493.88][v % 3]
    x = np.zeros(n)
    for i, semi in enumerate([0, 4, 7, 12]):
        m = n_of(0.7)
        f = root * 2 ** (semi / 12)
        note = _bell(f, m, 0.5) * 0.6 + sine(f * 2, m) * env_exp(m, 0.08) * 0.2
        x = place(x, note, i * 0.055)
    glitter = hp(crackle(n, rng, env_points(n, [(0, 0), (0.1, 400), (0.6, 0)]), 0.0006), 6000) * 6
    x = x + glitter * 0.2
    return mix(widen(x, 0.6), delay(x * 0.6, 0.11, 0.35, 0.35, 6000, 0.03))


@sound("rankup", 1, -14.0)
def rankup(rng, v):
    """Promotion fanfare: a brassy synth chord progression with a shimmering crown."""
    d = 2.8
    n = n_of(d)
    x = np.zeros((n, 2))
    chords = [(0.0, [261.63, 329.63, 392.0], 0.35), (0.32, [293.66, 369.99, 440.0], 0.35), (0.64, [392.0, 493.88, 587.33, 783.99], 2.0)]
    for at, freqs, dur in chords:
        m = n_of(dur + 0.4)
        c = np.zeros(m)
        for f in freqs:
            p = f * (1 + 0.003 * np.sin(2 * math.pi * 5.5 * time(dur + 0.4)))
            brass = supersaw(p, m, 3, 0.006, rng)
            c += lp(brass, env_points(m, [(0, 400), (0.06, 4500), (dur, 2000), (dur + 0.4, 600)]), 1.0)
        c *= env_points(m, [(0, 0), (0.03, 1), (dur, 0.8), (dur + 0.4, 0)], 1.4) / len(freqs) ** 0.5
        x = place(x, widen(c, 0.5), at)
    crown = np.zeros(n)
    for i, f in enumerate([1567.98, 1975.53, 2349.32, 3135.96]):
        m = n_of(1.2)
        crown = place(crown, _bell(f, m, 0.8, 1.2) * 0.25, 0.64 + i * 0.07)
    x = mix(x, widen(crown, 0.8))
    timp = sine(ramp(n_of(0.6), 98, 80), n_of(0.6)) * env_exp(n_of(0.6), 0.15)
    x = place(x, timp, 0.64, 0.5)
    return mix(x, reverb(to_mono(x), 0.3, 1.2, seed=v) * 0.6)


@sound("dock", 2, -16.0)
def dock(rng, v):
    """Docking clamps / refit: hydraulic hiss, two clamp clunks, a confirming chime."""
    d = 1.6
    n = n_of(d)
    hiss = hp(white(n, rng), 2500) * env_points(n, [(0, 0), (0.02, 0.6), (0.35, 0.2), (0.5, 0)], 1.5)
    x = hiss * 0.6
    for at in (0.12, 0.3):
        m = n_of(0.5)
        ex = lp(white(m, rng), 3000) * env_exp(m, 0.004)
        cl = modal(ex, metal_modes(J(rng, 300), 8, rng, (0.06, 0.3))) * 5 + sine(ramp(m, 110, 50), m) * env_exp(m, 0.05)
        x = place(x, cl, at, 0.7)
    m = n_of(1.0)
    chime = _bell(880, m, 0.6, 1.0) * 0.25 + _bell(1318.5, m, 0.6, 1.0) * 0.2
    x = place(x, chime, 0.55)
    x = drive(x, 1.3)
    return mix(widen(x, 0.5), reverb(x, 0.25, 0.7, seed=v) * 0.6)


@sound("captured", 1, -15.0)
def captured(rng, v):
    """A planet changes hands: a low swelling chord answered by a bell."""
    d = 2.8
    n = n_of(d)
    c = np.zeros(n)
    for f in (110.0, 164.81, 220.0, 277.18):
        c += lp(supersaw(f, n, 5, 0.01, rng), env_points(n, [(0, 300), (0.6, 2200), (d, 500)]), 1.2)
    c *= env_points(n, [(0, 0), (0.5, 1), (1.6, 0.7), (d, 0)], 1.2) * 0.4
    m = n_of(2.0)
    bell = _bell(440, m, 1.4, 1.5) * 0.4 + _bell(659.25, m, 1.4, 1.2) * 0.25
    x = place(widen(c, 0.6), widen(bell, 0.3), 0.5)
    return mix(x, reverb(to_mono(x), 0.35, 1.4, seed=v) * 0.6)


@sound("victory", 1, -13.0)
def victory(rng, v):
    """Round won: a rising major cadence."""
    d = 4.2
    n = n_of(d)
    x = np.zeros((n, 2))
    seq = [(0.0, [196.0, 246.94, 293.66]), (0.45, [220.0, 277.18, 329.63]), (0.9, [261.63, 329.63, 392.0, 523.25])]
    for i, (at, fr) in enumerate(seq):
        dur = 0.5 if i < 2 else 2.6
        m = n_of(dur + 0.6)
        c = np.zeros(m)
        for f in fr:
            c += lp(supersaw(f, m, 5, 0.008, rng), env_points(m, [(0, 500), (0.08, 5000), (dur + 0.6, 800)]), 1.0)
        c *= env_points(m, [(0, 0), (0.04, 1), (dur, 0.85), (dur + 0.6, 0)], 1.3) / 2
        x = place(x, widen(c, 0.6), at)
    for i in range(6):
        m = n_of(1.5)
        x = place(x, widen(_bell(1046.5 * 2 ** ([0, 4, 7, 12, 16, 19][i] / 12), m, 1.0, 1.0) * 0.2, 0.9), 0.9 + i * 0.09)
    return mix(x, reverb(to_mono(x), 0.35, 1.6, seed=v) * 0.6)


@sound("alarm", 1, -17.0)
def alarm(rng, v):
    """Hull critical klaxon (played on repeat by the cockpit)."""
    d = 0.62
    n = n_of(d)
    x = np.zeros(n)
    for at, f in ((0.0, 880.0), (0.26, 660.0)):
        m = n_of(0.22)
        tone = square(f, m, pw=0.35) * 0.35 + sine(f, m) * 0.5 + sine(f * 2.0, m) * 0.15
        tone = lp(tone, 4000) * env_points(m, [(0, 0), (0.008, 1), (0.19, 0.9), (0.22, 0)])
        x = place(x, tone, at)
    return widen(x, 0.2)


@sound("incoming", 1, -20.0)
def incoming(rng, v):
    """Missile lock warning: a short, urgent high blip."""
    d = 0.12
    n = n_of(d)
    tone = (sine(2350, n) + 0.25 * square(2350, n)) * env_points(n, [(0, 0), (0.003, 1), (0.07, 0.8), (0.1, 0)])
    return widen(lp(tone, 7000), 0.1)


@sound("lock", 2, -21.0)
def lock(rng, v):
    """Target acquired: two quick rising beeps."""
    d = 0.25
    n = n_of(d)
    x = np.zeros(n)
    base = [1320.0, 1480.0][v % 2]
    for i, at in enumerate((0.0, 0.09)):
        m = n_of(0.07)
        f = base * (1.0 if i == 0 else 1.335)
        x = place(x, (sine(f, m) + 0.2 * sine(f * 3, m)) * env_points(m, [(0, 0), (0.003, 1), (0.05, 0.6), (0.07, 0)]), at)
    return widen(x, 0.3)


@sound("power_low", 1, -20.0)
def power_low(rng, v):
    """Capacitors drained: a deflating downward tone."""
    d = 0.7
    n = n_of(d)
    p = ramp(n, 520, 140)
    x = (saw(p, n) * 0.4 + sine(p, n) * 0.6)
    x = lp(x, ramp(n, 3000, 400), 2.0) * env_points(n, [(0, 0), (0.01, 1), (0.5, 0.6), (d, 0)])
    return widen(x, 0.3)


@sound("launch", 2, -16.0)
def launch(rng, v):
    """Leaving the hangar: clamps release, engines light, and away."""
    d = 1.6
    n = n_of(d)
    m = n_of(0.5)
    rel = modal(lp(white(m, rng), 2500) * env_exp(m, 0.003), metal_modes(J(rng, 420), 8, rng, (0.05, 0.25))) * 4
    ign = bp(pink(n, rng), ramp(n, 500, 3500), 0.8) * env_points(n, [(0, 0), (0.15, 0), (0.25, 1.2), (0.8, 0.6), (d, 0)], 1.3)
    whine = sine(ramp(n, 300, 1400, "log"), n) * env_points(n, [(0, 0), (0.2, 0), (0.5, 0.15), (d, 0)])
    x = place(ign + whine, rel, 0.0, 0.8)
    x = drive(x, 1.5)
    return mix(widen(x, 0.6), reverb(x, 0.25, 0.7, seed=v) * 0.6)


@sound("notify", 2, -24.0)
def notify(rng, v):
    """Comm message: a soft two-note blip."""
    d = 0.3
    n = n_of(d)
    x = np.zeros(n)
    f = [987.77, 880.0][v % 2]
    for i, at in enumerate((0.0, 0.07)):
        m = n_of(0.2)
        x = place(x, _bell(f * (1.0 if i == 0 else 1.5), m, 0.15, 0.6), at)
    return widen(x, 0.4)


@sound("ui_hover", 3, -30.0)
def ui_hover(rng, v):
    d = 0.06
    n = n_of(d)
    f = J(rng, 2600, 0.05)
    x = sine(f, n) * env_points(n, [(0, 0), (0.002, 1), (0.03, 0.2), (d, 0)], 2) + hp(white(n, rng), 6000) * env_exp(n, 0.002) * 0.3
    return widen(x, 0.3)


@sound("ui_click", 3, -22.0, maxlen=0.32)
def ui_click(rng, v):
    d = 0.16
    n = n_of(d)
    tick = hp(white(n, rng), 2500) * env_exp(n, 0.0025)
    chirp = sine(ramp(n, J(rng, 900, 0.04), 1800), n) * env_points(n, [(0, 0), (0.002, 1), (0.05, 0.4), (d, 0)], 2)
    thump = sine(ramp(n, 220, 120), n) * env_exp(n, 0.02) * 0.5
    x = tick * 0.6 + chirp * 0.5 + thump
    return mix(widen(x, 0.4), reverb(x, 0.15, 0.25, seed=v) * 0.5)


@sound("ui_back", 2, -23.0, maxlen=0.32)
def ui_back(rng, v):
    d = 0.18
    n = n_of(d)
    tick = hp(white(n, rng), 2500) * env_exp(n, 0.0025)
    chirp = sine(ramp(n, 1500, 700), n) * env_points(n, [(0, 0), (0.002, 1), (0.06, 0.4), (d, 0)], 2)
    x = tick * 0.5 + chirp * 0.6
    return mix(widen(x, 0.4), reverb(x, 0.15, 0.25, seed=v) * 0.5)


@sound("ui_start", 1, -16.0)
def ui_start(rng, v):
    """Launch from the menu: a big confirm swell."""
    d = 1.8
    n = n_of(d)
    sw = bp(pink(n, rng), ramp(n, 300, 6000), 1.0) * env_points(n, [(0, 0), (0.5, 1), (0.55, 0.3), (d, 0)], 1.4)
    tone = supersaw(ramp(n, 110, 220, "log"), n, 5, 0.01, rng)
    tone = lp(tone, ramp(n, 400, 5000)) * env_points(n, [(0, 0), (0.5, 0.8), (0.6, 0.5), (d, 0)], 1.5) * 0.5
    m = n_of(1.2)
    hit_ = sine(ramp(m, 150, 40), m) * env_exp(m, 0.15) + drive(white(m, rng) * env_exp(m, 0.01) * 2, 2) * 0.3
    x = place(sw + tone, hit_, 0.5)
    return mix(widen(x, 0.6), reverb(x, 0.35, 1.0, seed=v) * 0.6)


# ============================================== per-item sounds by level / type
#
# Every fitted module speaks with its own voice: Engine I-IV, Booster I-III,
# Laser I-III, Shield I-VI (+ the station shield), Armor I-III and Quantum
# drive I-III each have their own family, and the game picks the one that
# matches the ship's best module of that kind.

def _family(prefix, levels, variants, loud, loop=False, maxlen=0.0):
    """Register prefix1..prefixN from one function taking (rng, v, level)."""
    def reg(fn):
        for lvl in levels:
            sound(f"{prefix}{lvl}", variants, loud, loop, maxlen)(lambda rng, v, _l=lvl: fn(rng, v, _l))
        return fn
    return reg


# ---- engines: four drive technologies, each a different instrument

def _engine_chemical(rng, n, t):
    """Engine I: a chemical rocket.  Ragged roar, popping combustion, a
    rattling low throb you can feel in the hull."""
    roar = bp(mix(pink(n, rng), brown(n, rng) * 0.8), 700 * (1 + 0.15 * np.sin(2 * math.pi * 0.37 * t)), 0.5) * 1.5
    rumble = bp(brown(n, rng), 75, 0.8) * 1.4
    flutter = 1 + 0.22 * np.sin(2 * math.pi * 17.0 * t) * (0.6 + 0.4 * np.sin(2 * math.pi * 0.61 * t))
    pops = bp(crackle(n, rng, 140, 0.0025), 1300, 0.7) * 16
    return drive((roar + rumble) * flutter + pops * 0.5, 2.2)


def _engine_ion(rng, n, t):
    """Engine II: an ion drive.  A smooth electric hum with a chorus of
    detuned saws, a pulsing grid buzz and a fine spray of ions."""
    base = 98.0
    hum = supersaw(base * (1 + 0.003 * np.sin(2 * math.pi * 0.21 * t)), n, 5, 0.008, rng)
    hum = bp(hum, 520 * (1 + 0.35 * np.sin(2 * math.pi * 0.13 * t)), 1.6) * 1.2
    grid = square(base * 0.5, n, pw=0.2) * (0.5 + 0.5 * np.sin(2 * math.pi * 8.0 * t)) * 0.12
    ions = bp(white(n, rng), 6500, 2.0) * (0.6 + 0.4 * np.sin(2 * math.pi * 3.3 * t)) * 0.5
    sub = sine(49.0, n) * 0.35
    return drive(hum + lp(grid, 2500) + ions + sub, 1.4)


def _engine_fusion(rng, n, t):
    """Engine III: a fusion torch.  Heavy blowtorch roar inside a magnetic
    bottle that whines and swirls, over a deep reactor drone."""
    torch = bp(mix(pink(n, rng), white(n, rng) * 0.3), 1800, 0.45) * 1.3
    torch *= 1 + 0.3 * np.sin(2 * math.pi * 23.0 * t)
    body = bp(brown(n, rng), 160, 0.7) * 1.6
    swirl = fm_op(330 * (1 + 0.02 * np.sin(2 * math.pi * 0.5 * t)), 1.5, 1.5 + np.sin(2 * math.pi * 0.25 * t), n) * 0.1
    drone = (sine(41.2, n) + 0.5 * sine(82.4, n)) * 0.45
    return drive(torch + body + swirl + drone, 1.7)


def _engine_antimatter(rng, n, t):
    """Engine IV: an antimatter drive.  A shimmering, phasing chord of pure
    tones with a vortex of filtered noise and a sub you feel more than hear."""
    chord = np.zeros(n)
    for f, a in ((110.0, 1.0), (164.8, 0.7), (220.3, 0.6), (329.6, 0.4), (440.8, 0.25)):
        chord += (sine(f, n) + sine(f * 1.004, n, 0.5)) * a
    chord *= 0.18 * (0.75 + 0.25 * np.sin(2 * math.pi * 0.7 * t))
    vortex_src = mix(pink(n, rng), white(n, rng) * 0.2)
    sweep = 700 + 5200 * (0.5 + 0.5 * np.sin(2 * math.pi * 0.31 * t))
    vortex = svf(bp(vortex_src, 2200, 0.6), sweep, 4.0, "notch") * 0.9
    sub = sine(36.7, n) * 0.5
    sparkle = hp(crackle(n, rng, 60, 0.0006), 7000) * 6
    return drive(chord + vortex + sub + sparkle * 0.3, 1.3)


ENGINES = {1: _engine_chemical, 2: _engine_ion, 3: _engine_fusion, 4: _engine_antimatter}


@_family("engine", [1, 2, 3, 4], 1, -19.0, loop=True)
def engine(rng, v, lvl):
    d = 5.25
    n = n_of(d)
    x = hp(ENGINES[lvl](rng, n, time(d)), 28)
    return make_loop(widen(x, 0.55), 0.3)


@_family("ignite", [1, 2, 3, 4], 3, -16.0)
def ignite(rng, v, lvl):
    """Throttle up: each drive lights in its own way."""
    d = 0.9
    n = n_of(d)
    t = time(d)
    if lvl == 1:      # igniter spark, then the whoomp of propellant catching
        spark = drive(hp(white(n, rng), 2500) * env_exp(n, 0.006) * 3, 3)
        whoomp = bp(pink(n, rng), ramp(n, 300, 1600), 0.7) * env_points(n, [(0, 0), (0.03, 0), (0.08, 1.4), (0.4, 0.4), (d, 0)], 1.5)
        thump = sine(ramp(n, 110, 45), n) * env_points(n, [(0, 0), (0.04, 0), (0.05, 1), (0.3, 0)], 1.8)
        x = spark * 0.5 + whoomp + thump
    elif lvl == 2:    # capacitor charge chirp into the grid buzz
        chirp = sine(ramp(n, 200, J(rng, 1600), "log"), n) * env_points(n, [(0, 0), (0.01, 0.6), (0.18, 1), (0.3, 0.2), (d, 0)])
        zap = bp(white(n, rng), 6000, 3.0) * env_points(n, [(0, 0), (0.15, 0), (0.18, 1), (0.4, 0)], 2) * 1.2
        hum = _engine_ion(rng, n, t) * env_points(n, [(0, 0), (0.2, 0.9), (d, 0)], 1.4)
        x = chirp * 0.4 + zap + hum
    elif lvl == 3:    # containment field snaps on and the torch blasts
        snap = drive(white(n, rng) * env_exp(n, 0.004) * 3, 3)
        boom = sine(ramp(n, 140, 38), n) * env_points(n, [(0, 0), (0.003, 1), (0.35, 0.3), (d, 0)], 1.6)
        blast = _engine_fusion(rng, n, t) * env_points(n, [(0, 0), (0.02, 1.6), (0.3, 0.7), (d, 0)], 1.4)
        x = snap * 0.4 + boom + blast
    else:             # reaction chamber rises through a chord
        rise = np.zeros(n)
        for f in (110.0, 164.8, 220.0, 329.6):
            rise += sine(ramp(n, f * 0.5, f), n)
        rise *= env_points(n, [(0, 0), (0.25, 0.6), (0.35, 1), (d, 0)], 1.3) * 0.25
        flash = hp(white(n, rng), 4000) * env_points(n, [(0, 0), (0.3, 0), (0.33, 1), (0.6, 0)], 2)
        sub = sine(ramp(n, 80, 36), n) * env_points(n, [(0, 0), (0.3, 0), (0.33, 1), (d, 0)], 1.5)
        x = rise + flash * 0.4 + sub
    x = drive(x, 1.4)
    return mix(widen(x, 0.5), reverb(x, 0.25, 0.6, seed=v) * 0.5)


# ---- boosters

@_family("boost", [1, 2, 3], 1, -17.0, loop=True)
def boost(rng, v, lvl):
    """Booster I: afterburner.  II: plasma burner.  III: hyperburner."""
    d = 4.0
    n = n_of(d)
    t = time(d)
    src = mix(pink(n, rng), white(n, rng) * 0.3)
    if lvl == 1:
        roar = bp(src, 1300 * (1 + 0.15 * np.sin(2 * math.pi * 0.4 * t)), 0.55) * 1.6
        pops = bp(crackle(n, rng, 180, 0.002), 900, 0.7) * 14
        x = drive(roar + pops * 0.5, 2.2)
    elif lvl == 2:
        roar = bp(src, 2600, 0.7) * 1.3 * (1 + 0.25 * np.sin(2 * math.pi * 31 * t))
        crackl = hp(crackle(n, rng, 900, 0.0006), 3000) * 12
        whine = sine(1650 * (1 + 0.006 * np.sin(2 * math.pi * 0.8 * t)), n) * 0.08
        x = drive(roar + crackl * 0.4 + whine + bp(brown(n, rng), 200, 0.7) * 0.8, 1.8)
    else:
        roar = bp(src, 1900, 0.5) * 1.3
        siren = fm_op(880 * (1 + 0.03 * np.sin(2 * math.pi * 1.1 * t)), 2.0, 2.0 + 1.5 * np.sin(2 * math.pi * 0.5 * t), n) * 0.09
        tear = svf(white(n, rng), 1500 + 3000 * (0.5 + 0.5 * np.sin(2 * math.pi * 2.3 * t)), 5.0, "bp") * 0.6
        sub = sine(55.0, n) * 0.35 * (1 + 0.3 * np.sin(2 * math.pi * 6 * t))
        x = drive(roar + siren + tear + sub, 1.8)
    return make_loop(widen(hp(x, 30), 0.65), 0.3)


@_family("boost_kick", [1, 2, 3], 2, -14.5)
def boost_kick(rng, v, lvl):
    """The punch when the booster engages."""
    d = 1.0
    n = n_of(d)
    kick = sine(ramp(n, 160 - 25 * lvl, 40), n) * env_points(n, [(0, 0), (0.004, 1), (0.25, 0.3), (0.6, 0)], 1.6)
    blast = bp(pink(n, rng), ramp(n, 900 + 700 * lvl, 500), 0.6) * env_points(n, [(0, 0), (0.01, 1.4), (0.3, 0.4), (d, 0)], 1.5)
    x = kick + blast
    if lvl >= 2:
        x = x + saw(ramp(n, 300, 1800 + 600 * lvl, "log"), n) * env_points(n, [(0, 0), (0.02, 0.25), (0.4, 0)], 1.5) * 0.5
    if lvl == 3:
        x = x + hp(white(n, rng), 5000) * env_exp(n, 0.03) * 0.6
    x = drive(x, 1.8)
    return mix(widen(x, 0.6), reverb(x, 0.3, 0.7, seed=v) * 0.5)


# ---- lasers: thin laser, warbling phaser, crackling particle lance

def _laser(rng, n, lvl):
    t = np.arange(n) / RATE
    if lvl == 1:
        return _beam_core(rng, n, 130.0)
    if lvl == 2:
        base = 82.0
        hum = supersaw(base, n, 5, 0.01, rng)
        warble = 1 + 0.5 * np.sin(2 * math.pi * 6.5 * t)
        hum = bp(hum, 900 * warble, 3.0) * 1.6
        comb = svf(white(n, rng), 3000 + 2000 * np.sin(2 * math.pi * 0.9 * t), 8.0, "bp") * 0.35
        return hum + comb + sine(base * 0.5, n) * 0.3
    base = 55.0
    core = (saw(base, n) + saw(base * 1.5, n, 0.4) * 0.6 + saw(base * 2.01, n, 0.2) * 0.4)
    core = lp(core, 1400 * (1 + 0.25 * np.sin(2 * math.pi * 1.7 * t)), 2.0) * 1.2
    arcs = bp(crackle(n, rng, 500, 0.0015), 2500, 0.8) * 14 * (0.5 + 0.5 * (np.sin(2 * math.pi * 3.1 * t) > 0))
    sub = sine(27.5, n) * 0.5
    return core + arcs * 0.5 + sub


@_family("beam_loop", [1, 2, 3], 1, -19.0, loop=True)
def beam_loop_l(rng, v, lvl):
    d = 3.0
    n = n_of(d)
    x = hp(drive(_laser(rng, n, lvl), 1.5), 25)
    return make_loop(widen(x, 0.45), 0.25)


@_family("beam_start", [1, 2, 3], 3, -16.0)
def beam_start_l(rng, v, lvl):
    d = 0.35 + 0.1 * lvl
    n = n_of(d)
    top = [2600, 1800, 1100][lvl - 1]
    chirp = saw(ramp(n, 200, J(rng, top), "log"), n) * env_points(n, [(0, 0), (0.005, 1), (0.12, 0.5), (d, 0)], 1.5)
    core = _laser(rng, n, lvl) * env_points(n, [(0, 0), (0.06, 1), (0.2, 0.6), (d, 0)])
    click = hp(white(n, rng), 3000) * env_exp(n, 0.003)
    x = lp(chirp, 7000) * 0.5 + core + click * 0.6
    if lvl == 3:
        x = x + sine(ramp(n, 90, 30), n) * env_exp(n, 0.1)
    return widen(drive(x, 1.5), 0.4)


@_family("beam_stop", [1, 2, 3], 3, -21.0)
def beam_stop_l(rng, v, lvl):
    d = 0.4 + 0.1 * lvl
    n = n_of(d)
    base = [130.0, 82.0, 55.0][lvl - 1]
    p = ramp(n, base, J(rng, base * 0.35))
    hum = saw(p, n) + saw(p * 2.01, n) * 0.5
    hum = lp(hum, ramp(n, 2400, 300), 2.0) * env_points(n, [(0, 1), (d, 0)], 2.5)
    spit = hp(crackle(n, rng, ramp(n, 400 + 300 * lvl, 0), 0.001), 2500) * 8
    return widen(drive(hum * 0.8 + spit * 0.4, 1.3), 0.4)


# ---- shields: the field rings lower, richer and longer as it gets stronger

@_family("shield", [1, 2, 3, 4, 5, 6, 7], 4, -18.0)
def shield_l(rng, v, lvl):
    d = 0.5 + 0.1 * lvl
    n = n_of(d)
    t = time(d)
    f = J(rng, 1000.0 * 0.84 ** (lvl - 1), 0.12)
    idx = env_points(n, [(0, 3.0 + 0.5 * lvl), (0.1, 2.0), (d, 0.3)])
    partials = 1 + min(lvl, 5) // 2
    bell = np.zeros(n)
    for k in range(partials):
        bell += fm_op(f * (1.0, 1.5, 2.01, 2.76)[k], (1.414, 2.76, 1.2, 3.1)[k], idx * (0.8 ** k), n) * 0.7 ** k
    bell *= env_points(n, [(0, 0), (0.003, 1), (0.12, 0.4), (d, 0)], 1.6)
    ripple = 1 + 0.5 * np.sin(2 * math.pi * ramp(n, 50 - 4 * lvl, 10) * t)
    zap = bp(white(n, rng), ramp(n, 10000 - 700 * lvl, 2500), 4.0) * env_points(n, [(0, 0), (0.002, 1.4), (0.06, 0.3), (0.3, 0)], 2)
    sub = sine(ramp(n, 240 - 20 * lvl, 70), n) * env_exp(n, 0.04 + 0.01 * lvl) * (0.2 + 0.07 * lvl)
    x = drive(bell * ripple * 0.5 + zap * (0.9 - 0.06 * lvl) + sub, 1.4)
    return mix(autopan(x, J(rng, 5.0), 0.35, rng.random() * 6), reverb(x, 0.3, 0.4 + 0.08 * lvl, seed=v) * 0.6)


# ---- armour: plating gets heavier, duller and deeper

@_family("hit_armor", [1, 2, 3], 5, -17.0, maxlen=1.2)
def hit_armor(rng, v, lvl):
    d = 0.9
    n = n_of(d)
    ex = lp(white(n, rng), 6000 - 1500 * lvl) * env_exp(n, J(rng, 0.002 + 0.001 * lvl, 0.3))
    plate = modal(ex, metal_modes(J(rng, 420 - 90 * lvl, 0.25), 12, rng, (0.08, 0.3 + 0.15 * lvl), 4.0 - 0.6 * lvl)) * 4
    thud = sine(ramp(n, J(rng, 140 - 20 * lvl), 40), n) * env_points(n, [(0, 0), (0.002, 1), (0.1, 0.35), (0.3, 0)], 2) * (0.9 + 0.25 * lvl)
    crunch = drive(lp(white(n, rng), 2500) * env_points(n, [(0, 0), (0.001, 1), (0.03, 0.3), (0.1, 0)], 2) * 2, 3) * (0.6 - 0.15 * lvl)
    x = drive(plate * 0.8 + thud + crunch, 1.8)
    return mix(widen(x, 0.35), reverb(x, 0.2, 0.45, seed=v) * 0.6)


# ---- quantum drives: bigger drives spool deeper and jump harder

@_family("warpin", [1, 2, 3], 2, -15.0)
def warpin_l(rng, v, lvl):
    d = 1.25
    n = n_of(d)
    tones = np.zeros(n)
    ratios = [[1.0, 2.0], [1.0, 1.5, 2.0], [1.0, 1.25, 1.5, 2.0, 3.0]][lvl - 1]
    lo = [110, 70, 45][lvl - 1]
    hi = [1400, 900, 650][lvl - 1]
    for i, r in enumerate(ratios):
        tones += saw(ramp(n, J(rng, lo) * r, J(rng, hi) * r, "exp"), n, rng.random()) / (1 + i * 0.6)
    trem = 0.6 + 0.4 * np.sin(2 * math.pi * np.cumsum(ramp(n, 4, 30 + 10 * lvl)) / RATE)
    tones = lp(tones, ramp(n, 600, 9000), 2.0) * trem
    press = bp(pink(n, rng), ramp(n, 300, 5000), 1.2) * 1.5
    x = (tones * 0.5 + press * 0.6) * env_points(n, [(0, 0), (0.9, 0.8), (1.1, 1.0), (d, 0)], 1.0)
    if lvl == 3:
        x = x + sine(ramp(n, 30, 60), n) * env_points(n, [(0, 0), (1.1, 0.8), (d, 0)]) * 0.6
    x = drive(x, 1.6)
    return mix(widen(x, 0.6), reverb(x, 0.25, 0.6, seed=v) * 0.5)


@_family("warpout", [1, 2, 3], 2, -12.0)
def warpout_l(rng, v, lvl):
    d = 2.2 + 0.4 * lvl
    n = n_of(d)
    clap = drive(white(n, rng) * env_exp(n, 0.015 + 0.005 * lvl) * 3, 3)
    boom = sine(ramp(n, 140 - 15 * lvl, 22), n) * env_points(n, [(0, 0), (0.004, 1), (0.4, 0.35), (1.0 + 0.3 * lvl, 0)], 1.5)
    whoosh = bp(pink(n, rng), ramp(n, 7000 - 800 * lvl, 160), 0.9) * env_points(n, [(0, 0), (0.01, 1.4), (0.4, 0.6), (d, 0)], 1.5) * 1.6
    zap = saw(ramp(n, 3000 - 500 * lvl, 60), n) * env_points(n, [(0, 0), (0.002, 1), (0.25, 0.2), (0.6, 0)], 2) * 0.4
    x = clap * 0.4 + boom * (0.8 + 0.2 * lvl) + whoosh + lp(zap, 6000)
    if lvl >= 2:
        x = x + _bell(220.0 * (1.5 if lvl == 3 else 1.0), n, 1.2, 2.0) * 0.12
    x = drive(x, 1.8)
    return mix(widen(x, 0.7), reverb(x, 0.4, 1.0 + 0.2 * lvl, seed=v) * 0.7)


# ---- station battery (rapid plasma 011): a heavy pounding emplacement gun

@sound("battery", 5, -14.0, maxlen=1.2)
def battery(rng, v):
    d = 0.9
    n = n_of(d)
    pitch = ramp(n, J(rng, 700), 90)
    tone = (square(pitch, n, pw=0.4) * 0.4 + sine(pitch * 0.5, n) * 0.6)
    tone = lp(tone, ramp(n, 6000, 500), 1.5) * env_points(n, [(0, 0), (0.002, 1), (0.08, 0.4), (0.4, 0)], 2)
    pound = sine(ramp(n, 120, 35), n) * env_points(n, [(0, 0), (0.002, 1), (0.2, 0.3), (0.5, 0)], 1.8) * 1.2
    crack = drive(white(n, rng) * env_exp(n, 0.006) * 3, 3)
    x = drive(tone + pound + crack * 0.4, 1.9)
    return mix(widen(x, 0.3), reverb(x, 0.25, 0.6, seed=v) * 0.6)


# ================================================================== rendering

def render(name):
    fn, variants, loud, loop, maxlen = SOUNDS[name]
    out = []
    for v in range(variants):
        rng = np.random.default_rng(sum(map(ord, name)) * 1009 + v * 7919)
        x = fn(rng, v)
        if x.ndim == 1:
            x = np.stack([x, x], axis=1)
        if not loop:
            x = dc_block(x)
            x = trim_tail(x)
            if maxlen and len(x) > n_of(maxlen):
                x = fade(x[:n_of(maxlen)], 0.0, maxlen * 0.4)
            x = fade(x, 0.0008, 0.0)
        x = set_loudness(x, loud)
        out.append(x)
    return out


def write_ogg(path, x):
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
        sf.write(tmp.name, np.clip(x, -1, 1).astype(np.float32), RATE, subtype="FLOAT")
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", tmp.name, "-c:a", "libvorbis", "-q:a", "6", path], check=True)
    os.unlink(tmp.name)


def main(argv):
    report_only = "--report" in argv
    names = [a for a in argv if not a.startswith("--")]
    os.makedirs(OUT, exist_ok=True)
    print(f"{'name':16} {'var':>3} {'len s':>6} {'peak dB':>8} {'loud dB':>8}")
    for name in SOUNDS:
        if names and not any(name.startswith(p) for p in names):
            continue
        for i, x in enumerate(render(name), 1):
            pk = 20 * math.log10(np.abs(x).max() + 1e-12)
            print(f"{name:16} {i:3d} {len(x) / RATE:6.2f} {pk:8.1f} {loudness(x):8.1f}")
            if not report_only:
                write_ogg(os.path.join(OUT, f"{name}_{i:02d}.ogg"), x)


if __name__ == "__main__":
    main(sys.argv[1:])
