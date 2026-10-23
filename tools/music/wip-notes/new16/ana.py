import sys, numpy as np, soundfile as sf, scipy.signal as ss, pyloudnorm as pyln
x, sr = sf.read(sys.argv[1], dtype='float64'); x = x.T
bar0 = float(sys.argv[2]) if len(sys.argv) > 2 else 0
SPB = 60/172; BAR = 4*SPB
mono = x.mean(0)
print('len %.1fs peak %.2f dB' % (x.shape[1]/sr, 20*np.log10(np.abs(x).max())))
print('LUFS %.2f' % pyln.Meter(sr).integrated_loudness(x.T))
# clicks
sos = ss.butter(4, 8000, 'high', fs=sr, output='sos'); h = ss.sosfilt(sos, mono); d = np.abs(np.diff(h))
thr = 3*np.percentile(d, 99.99); print('clicks', int((d > thr).sum()), 'thr', thr)
# octave bands per 8-bar chunk
edges = [40,80,160,315,630,1250,2500,5000,10000,20000]
nb = int(x.shape[1]/(BAR*sr))
for c0 in range(0, nb, 8):
    i0, i1 = int(c0*BAR*sr), int(min(nb, c0+8)*BAR*sr)
    seg = mono[i0:i1]
    if len(seg) < sr: continue
    f, P = ss.welch(seg, sr, nperseg=8192)
    bands = [10*np.log10(P[(f>=lo)&(f<hi)].sum()+1e-20) for lo, hi in zip(edges[:-1], edges[1:])]
    ref = bands[0]
    rms = 20*np.log10(np.sqrt(np.mean(seg**2)))
    L, R = x[0, i0:i1], x[1, i0:i1]
    corr = np.corrcoef(L, R)[0, 1]
    lo = ss.sosfilt(ss.butter(4, 150, 'low', fs=sr, output='sos'), np.stack([L, R]))
    corrl = np.corrcoef(lo[0], lo[1])[0, 1]
    print(f'bar {bar0+c0:5.0f} rms {rms:6.1f} corr {corr:.2f} lo {corrl:.3f} | ' + ' '.join(f'{b-ref:5.1f}' for b in bands))
