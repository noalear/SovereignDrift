import sys, numpy as np, soundfile as sf, matplotlib
matplotlib.use('Agg'); import matplotlib.pyplot as plt, scipy.signal as ss
x, sr = sf.read(sys.argv[1]); mono = x.mean(1)
f, t, Z = ss.spectrogram(mono, sr, nperseg=4096, noverlap=2048)
fig, (a1, a2) = plt.subplots(2, 1, figsize=(16, 8), gridspec_kw={'height_ratios': [4, 1]}, sharex=True)
a1.pcolormesh(t, f, 10*np.log10(Z+1e-14), shading='auto', vmin=-130, vmax=-40, cmap='magma')
a1.set_yscale('symlog', linthresh=200); a1.set_ylim(30, 20000)
BAR = 4*60/92
secs = [0, 8, 16, 24, 32, 40, 48, 56, 64, 70]
for s in secs:
    a1.axvline(s*BAR, color='c', lw=0.8); a2.axvline(s*BAR, color='c', lw=0.8)
    a1.text(s*BAR, 16000, str(s), color='w')
hop = int(0.25*sr); r = [20*np.log10(np.sqrt(np.mean(mono[i:i+hop]**2))+1e-9) for i in range(0, len(mono)-hop, hop)]
a2.plot(np.arange(len(r))*0.25, r); a2.set_ylim(-45, -5); a2.grid()
plt.tight_layout(); plt.savefig(sys.argv[2], dpi=70)
