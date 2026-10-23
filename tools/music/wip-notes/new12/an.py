"""Analysis for new12: python3 an.py WAV bar0 [png] [chroma]  (bar0 = first bar in the file)."""
import sys
import numpy as np
import soundfile as sf
import scipy.signal as ss
import pyloudnorm as pyln

BPM = 108.0
BAR = 4 * 60 / BPM
x, sr = sf.read(sys.argv[1], always_2d=True)
x = x.T
bar0 = int(sys.argv[2])
png = sys.argv[3] if len(sys.argv) > 3 and sys.argv[3] != '-' else None
do_chroma = len(sys.argv) > 4
n = x.shape[1]
mono = x.mean(0)
meter = pyln.Meter(sr)
print(f'len {n / sr:.1f}s  integrated {meter.integrated_loudness(x.T):.2f} LUFS  peak {20 * np.log10(np.abs(x).max()):.2f} dBFS')

# clicks
sos = ss.butter(4, 8000, 'high', fs=sr, output='sos')
h = ss.sosfilt(sos, mono)
d = np.abs(np.diff(h))
thr = 3 * np.percentile(d, 99.99)
print(f'clicks (jumps > 3x p99.99 in HP8k): {int(np.sum(d > thr))}  (thr {thr:.4f})')

# per-section RMS & correlation & spectrum (8-bar blocks)
bands = [(40, 80), (80, 160), (160, 315), (315, 630), (630, 1250), (1250, 2500), (2500, 5000), (5000, 10000),
         (10000, 20000)]
sos_lo = ss.butter(4, 150, 'low', fs=sr, output='sos')
print('bars     RMSdB  corr  corrLo | octave bands rel 40-80 (dB)')
nb = int(n / sr / BAR)
for b in range(0, nb, 4):
    i0, i1 = int(b * BAR * sr), int(min(nb, b + 4) * BAR * sr)
    seg = x[:, i0:i1]
    if seg.shape[1] < sr:
        continue
    r = 20 * np.log10(np.sqrt(np.mean(seg ** 2)) + 1e-12)
    c = np.corrcoef(seg[0], seg[1])[0, 1]
    lo = ss.sosfilt(sos_lo, seg, axis=-1)
    cl = np.corrcoef(lo[0], lo[1])[0, 1]
    f, p = ss.welch(seg.mean(0), sr, nperseg=8192)
    e = [10 * np.log10(np.sum(p[(f >= a) & (f < bb)]) + 1e-20) for a, bb in bands]
    rel = ' '.join(f'{v - e[0]:6.1f}' for v in e)
    print(f'{bar0 + b:3d}-{bar0 + min(nb, b + 4):3d}  {r:6.1f} {c:5.2f} {cl:6.2f} | {rel}')

if do_chroma:
    import librosa
    y = librosa.resample(mono.astype(np.float32), orig_sr=sr, target_sr=22050)
    y = ss.sosfilt(ss.butter(4, 130, 'high', fs=22050, output='sos'), y)
    ch = librosa.feature.chroma_cqt(y=y, sr=22050, hop_length=512)
    fps = 22050 / 512
    names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
    for b in range(nb):
        a, bb = int(b * BAR * fps), int((b + 1) * BAR * fps)
        v = ch[:, a:bb].mean(1)
        top = np.argsort(v)[::-1][:4]
        print(f'bar {bar0 + b:3d}: ' + ' '.join(f'{names[t]}({v[t]:.2f})' for t in top))

if png:
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(2, 1, figsize=(18, 8), gridspec_kw={'height_ratios': [4, 1]}, sharex=True)
    f, t, Z = ss.spectrogram(mono, sr, nperseg=4096, noverlap=2048)
    ax[0].pcolormesh(t, f, 10 * np.log10(Z + 1e-14), vmin=-130, vmax=-40, shading='auto', cmap='magma')
    ax[0].set_yscale('symlog', linthresh=200)
    ax[0].set_ylim(30, 20000)
    hop = int(0.1 * sr)
    rr = [20 * np.log10(np.sqrt(np.mean(mono[i:i + hop] ** 2)) + 1e-9) for i in range(0, n - hop, hop)]
    ax[1].plot(np.arange(len(rr)) * 0.1, rr)
    ax[1].set_ylim(-50, -5)
    for a in ax:
        for b in range(0, nb + 1, 8):
            a.axvline(b * BAR, color='c', lw=0.6)
            if a is ax[1]:
                a.text(b * BAR, -8, str(bar0 + b), fontsize=8)
    plt.tight_layout()
    plt.savefig(png, dpi=70)
    print('png', png)
