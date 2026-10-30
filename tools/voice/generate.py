"""Stellar Frontier remake -- voice-line generator.

Speaks every line in src/ui/TutorialLines.gd and src/campaign/StoryLines.gd
with Kokoro-82M, a local
text-to-speech model (Apache 2.0: free for games, no account, no network
once the model is on disk), and writes assets/voice/<set>/<id>.ogg
(48 kHz mono Ogg Vorbis, loudness-matched).  The same tool voices any
future cast: add a speaker and lines, pick one of Kokoro's 54 voices.

    python3 -m venv ~/tts && ~/tts/bin/pip install kokoro-onnx soundfile numpy scipy
    # model files (about 350 MB), from the kokoro-onnx GitHub release
    #   kokoro-v1.0.onnx and voices-v1.0.bin -> tools/voice/models/ (or KOKORO_DIR)
    ~/tts/bin/python tools/voice/generate.py              # every line
    ~/tts/bin/python tools/voice/generate.py intro good_  # ids starting so
    ~/tts/bin/python tools/voice/generate.py --report     # lengths and levels only
    ~/tts/bin/python tools/voice/generate.py --sample am_michael "Text"  # try a voice

Reads the lines through Godot (GODOT=path to the editor binary, default
"godot") so the game's file stays the one source.  Needs ffmpeg with
libvorbis.  Kokoro is deterministic: the same text and voice give the same
audio.  After rendering, run `godot --headless --path . --import`.
"""
import json
import os
import subprocess
import sys
import tempfile

import numpy as np
import soundfile as sf
from scipy import signal

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MODELS = os.environ.get("KOKORO_DIR", os.path.join(ROOT, "tools", "voice", "models"))
GODOT = os.environ.get("GODOT", "godot")
RATE = 48000
TARGET_LUFS = -17.0     # a touch over the music (about -16 LUFS before its volume) once ducked
PEAK = 0.84             # -1.5 dBFS: Vorbis overshoots a little


def read_lines():
    out = os.path.join(tempfile.gettempdir(), "sf_voice_lines.json")
    subprocess.run([GODOT, "--headless", "--path", ROOT, "-s", "tools/voice/export_lines.gd", "--", out],
                   check=True, stdout=subprocess.DEVNULL)
    with open(out, encoding="utf-8") as f:
        return json.load(f)


def kokoro():
    from kokoro_onnx import Kokoro
    return Kokoro(os.path.join(MODELS, "kokoro-v1.0.onnx"), os.path.join(MODELS, "voices-v1.0.bin"))


def loudness(x, sr):
    """Integrated loudness, ITU-R BS.1770 K-weighting (48 kHz coefficients)
    with the -70 LUFS absolute gate; lines are short and dense, so no
    relative gate."""
    assert sr == 48000
    pre = signal.lfilter([1.53512485958697, -2.69169618940638, 1.19839281085285],
                         [1.0, -1.69065929318241, 0.73248077421585], x)
    k = signal.lfilter([1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621], pre)
    block = int(0.4 * sr)
    hop = int(0.1 * sr)
    powers = np.array([np.mean(k[i:i + block] ** 2) for i in range(0, max(len(k) - block, 1), hop)])
    powers = powers[powers > 0]
    if powers.size == 0:
        return -100.0
    gated = powers[-0.691 + 10 * np.log10(powers) > -70.0]
    return -0.691 + 10 * np.log10(np.mean(gated)) if gated.size else -100.0


def trim(x, sr, floor=0.004, pad=0.08):
    """Cut the model's leading and trailing silence, leaving a short pad."""
    idx = np.nonzero(np.abs(x) > floor)[0]
    if idx.size == 0:
        return x
    a = max(idx[0] - int(0.02 * sr), 0)
    b = min(idx[-1] + int(pad * sr), len(x))
    out = x[a:b].copy()
    fade = int(0.01 * sr)
    out[:fade] *= np.linspace(0, 1, fade)
    out[-fade:] *= np.linspace(1, 0, fade)
    return out


def fx_chain(x, sr, fx):
    # a gentle presence lift and a low cut keep the voice clear over the music
    x = signal.sosfilt(signal.butter(2, 90.0, "highpass", fs=sr, output="sos"), x)
    if fx == "radio":
        x = signal.sosfilt(signal.butter(4, [320.0, 3400.0], "bandpass", fs=sr, output="sos"), x)
        x = np.tanh(x * 2.2) / np.tanh(2.2)
    elif fx == "drengin":
        # a Drengin over comms: lower and heavier, a growl of grit, still clear words
        x = pitch_shift(x, sr, -3.0)
        x = signal.sosfilt(signal.butter(4, [180.0, 3800.0], "bandpass", fs=sr, output="sos"), x)
        x = np.tanh(x * 3.5) / np.tanh(3.5)
    elif fx == "arcean":
        # an Arcean: a faint shimmer, a doubled voice a few cents apart
        y = pitch_shift(x, sr, 0.12)
        n = min(len(x), len(y))
        x = x[:n] * 0.75 + np.roll(y[:n], int(0.012 * sr)) * 0.45
        x = signal.sosfilt(signal.butter(4, [250.0, 6000.0], "bandpass", fs=sr, output="sos"), x)
    else:
        b, a = _peaking(3200.0, 1.5, 0.9, sr)
        x = signal.lfilter(b, a, x)
    return x


def pitch_shift(x, sr, semitones):
    """Shift the pitch, keeping the length: resample, then stretch back
    with overlap-added windows."""
    k = 2 ** (semitones / 12.0)
    y = signal.resample_poly(x, 1000, int(round(1000 * k)))
    win = int(0.04 * sr)
    hop_out = win // 2
    hop_in = hop_out * len(y) / max(len(x), 1)
    out = np.zeros(len(x) + win)
    norm = np.zeros(len(x) + win)
    w = np.hanning(win)
    i = 0
    while i * hop_out + win < len(out):
        a = int(i * hop_in)
        seg = y[a:a + win]
        if len(seg) < win:
            break
        out[i * hop_out:i * hop_out + win] += seg * w
        norm[i * hop_out:i * hop_out + win] += w
        i += 1
    norm[norm < 1e-3] = 1.0
    return (out / norm)[:len(x)]


def _peaking(f0, gain_db, q, sr):
    A = 10 ** (gain_db / 40)
    w = 2 * np.pi * f0 / sr
    al = np.sin(w) / (2 * q)
    b = [1 + al * A, -2 * np.cos(w), 1 - al * A]
    a = [1 + al / A, -2 * np.cos(w), 1 - al / A]
    return np.array(b) / a[0], np.array(a) / a[0]


def say_names(k, text, lang, say):
    """The text as phonemes with each name in `say` (Names.SAY) spoken our
    way, or None when the text has none of them."""
    words = [w for w in say if w in text]
    if not words:
        return None
    ph = k.tokenizer.phonemize(text, lang)
    for w in words:
        ph = ph.replace(k.tokenizer.phonemize(w, lang).strip(" .,!?"), say[w])
    return ph


def render(k, text, spk, say=None):
    lang = spk.get("lang", "en-us")
    ph = say_names(k, text, lang, say or {})
    audio, sr = k.create(ph if ph else text, voice=spk["voice"], speed=float(spk.get("speed", 1.0)), lang=lang,
                         is_phonemes=bool(ph))
    x = np.asarray(audio, dtype=np.float64)
    x = signal.resample_poly(x, RATE, sr)
    x = trim(x, RATE)
    x = fx_chain(x, RATE, spk.get("fx", "clean"))
    x *= 10 ** ((TARGET_LUFS - 3.0 - loudness(x, RATE)) / 20)
    x = compress(x, RATE)
    for _ in range(3):
        x *= 10 ** ((TARGET_LUFS - loudness(x, RATE)) / 20)
        x = limit(x, RATE)
    return x


def _follow(level, sr, attack, release):
    """One-pole envelope follower: fast up, slow down."""
    a = np.exp(-1.0 / (attack * sr))
    r = np.exp(-1.0 / (release * sr))
    out = np.empty_like(level)
    e = 0.0
    for i, v in enumerate(level):
        e = a * e + (1 - a) * v if v > e else r * e + (1 - r) * v
        out[i] = e
    return out


def compress(x, sr, threshold_db=-24.0, ratio=2.5):
    """A gentle voice compressor, so quiet words and loud ones sit closer
    and the lines reach their loudness without hitting the peak limit."""
    env = _follow(np.abs(x), sr, 0.005, 0.12)
    over = 20 * np.log10(np.maximum(env, 1e-6)) - threshold_db
    gain_db = np.where(over > 0, -over * (1 - 1 / ratio), 0.0)
    return x * 10 ** (gain_db / 20)


def limit(x, sr, look=0.004):
    """Lookahead peak limiter at PEAK: the gain dips just before a peak
    and recovers smoothly, instead of the whole line being turned down."""
    from scipy.ndimage import minimum_filter1d, uniform_filter1d
    g = np.minimum(1.0, PEAK / np.maximum(np.abs(x), 1e-9))
    n = max(int(look * sr), 1)
    g = minimum_filter1d(g, size=2 * n + 1)
    g = uniform_filter1d(g, size=2 * n + 1)
    return x * np.minimum(g, 1.0)


def write_ogg(x, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as t:
        wav = t.name
    sf.write(wav, x.astype(np.float32), RATE, subtype="FLOAT")
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", wav, "-c:a", "libvorbis", "-q:a", "5", path], check=True)
    os.unlink(wav)


def main(argv):
    if "--sample" in argv:
        i = argv.index("--sample")
        voice, text = argv[i + 1], argv[i + 2]
        x = render(kokoro(), text, {"voice": voice}, read_lines().get("say", {}))
        out = os.path.join(tempfile.gettempdir(), "sf_voice_sample_%s.wav" % voice)
        sf.write(out, x.astype(np.float32), RATE)
        print(out)
        return
    data = read_lines()
    speakers = data["speakers"]
    picks = [a for a in argv if not a.startswith("--")]
    report_only = "--report" in argv
    k = None if report_only else kokoro()
    for line in data["lines"]:
        if picks and not any(line["id"].startswith(p) for p in picks):
            continue
        path = os.path.join(ROOT, "assets", "voice", line["set"], line["id"] + ".ogg")
        if not report_only:
            spk = dict(speakers[line["speaker"]])
            if line.get("fx"):
                spk["fx"] = line["fx"]
            write_ogg(render(k, line["text"], spk, data.get("say", {})), path)
        if os.path.exists(path):
            x, sr = sf.read(path)
            words = len(line["text"].split())
            print("%-14s %5.1f s  %4.1f words/s  %6.1f LUFS  peak %5.1f dB" % (
                line["id"], len(x) / sr, words / (len(x) / sr), loudness(x, sr), 20 * np.log10(np.max(np.abs(x)) + 1e-9)))
        else:
            print("%-14s missing" % line["id"])


if __name__ == "__main__":
    main(sys.argv[1:])
