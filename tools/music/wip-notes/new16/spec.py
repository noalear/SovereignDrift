import sys, numpy as np, soundfile as sf, matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt, scipy.signal as ss
x, sr = sf.read(sys.argv[1], dtype='float32'); y = x.mean(1)
f, t, Z = ss.spectrogram(y, sr, nperseg=4096, noverlap=2048)
BAR = 4*60/172
fig, ax = plt.subplots(2, 1, figsize=(18, 8), gridspec_kw={'height_ratios': [4, 1]}, sharex=True)
ax[0].pcolormesh(t, f, 10*np.log10(Z+1e-12), vmin=-120, vmax=-30, shading='auto', cmap='magma')
ax[0].set_yscale('symlog', linthresh=200); ax[0].set_ylim(30, 20000)
for b in (16, 32, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128):
    for a in ax: a.axvline(b*BAR, color='c', lw=0.8)
w = int(0.4*sr); r = [20*np.log10(np.sqrt(np.mean(y[i:i+w]**2))+1e-9) for i in range(0, len(y)-w, w)]
ax[1].plot(np.arange(len(r))*0.4, r); ax[1].set_ylim(-45, -5); ax[1].grid()
plt.tight_layout(); plt.savefig(sys.argv[2], dpi=70)
