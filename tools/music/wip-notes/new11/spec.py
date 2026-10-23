import sys, numpy as np, soundfile as sf, matplotlib; matplotlib.use('Agg')
import matplotlib.pyplot as plt, scipy.signal as ss
x, sr = sf.read(sys.argv[1], always_2d=True); y = x.mean(1)
f, t, Z = ss.spectrogram(y, sr, nperseg=4096, noverlap=2048)
fig, ax = plt.subplots(2, 1, figsize=(16, 8), gridspec_kw={'height_ratios': [4, 1]}, sharex=True)
ax[0].pcolormesh(t, f, 10*np.log10(Z+1e-14), vmin=-130, vmax=-40, shading='auto', cmap='magma')
ax[0].set_yscale('log'); ax[0].set_ylim(30, 20000)
bl = 4*60/118
for b in [8,24,40,48,64,72,88,94]:
    for a in ax: a.axvline(b*bl, color='c', lw=0.8)
w = int(0.1*sr); r = [20*np.log10(np.sqrt(np.mean(y[i:i+w]**2))+1e-9) for i in range(0, len(y)-w, w)]
ax[1].plot(np.arange(len(r))*0.1, r); ax[1].set_ylim(-50, -5)
plt.tight_layout(); plt.savefig(sys.argv[2], dpi=70)
