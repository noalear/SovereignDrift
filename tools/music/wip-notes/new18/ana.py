import sys, numpy as np, soundfile as sf, scipy.signal as ss, pyloudnorm as pyln
x, sr = sf.read(sys.argv[1]); x = x.T
bar0 = float(sys.argv[2]) if len(sys.argv) > 2 else 0
SPB = 60/92; BAR = 4*SPB
mono = x.mean(0)
print('len %.1f s  peak %.2f dBFS' % (x.shape[1]/sr, 20*np.log10(np.abs(x).max())))
m = pyln.Meter(sr); print('LUFS %.2f' % m.integrated_loudness(x.T))
# per-4-bar RMS
nb = int(x.shape[1]/sr/BAR)
for b in range(0, nb, 4):
    seg = mono[int(b*BAR*sr):int(min(b+4, nb)*BAR*sr)]
    sg = x[:, int(b*BAR*sr):int(min(b+4, nb)*BAR*sr)]
    lo = ss.sosfilt(ss.butter(4, 150, 'low', fs=sr, output='sos'), sg)
    cor = np.corrcoef(sg)[0,1]; corlo = np.corrcoef(lo)[0,1]
    # octave bands
    f, P = ss.welch(seg, sr, nperseg=8192)
    edges = [40,80,160,315,630,1250,2500,5000,10000,20000]
    bands = [10*np.log10(P[(f>=a)&(f<b2)].sum()+1e-20) for a,b2 in zip(edges[:-1],edges[1:])]
    ref = bands[0]
    print('bars %3d-%3d rms %6.1f corr %.2f lo %.2f | ' % (bar0+b, bar0+b+3, 20*np.log10(np.sqrt(np.mean(seg**2))+1e-12), cor, corlo) + ' '.join('%5.1f' % (v-ref) for v in bands))
hp = ss.sosfilt(ss.butter(4, 8000, 'high', fs=sr, output='sos'), mono)
d = np.abs(np.diff(hp)); thr = 3*np.percentile(d, 99.99)
print('clicks', int((d > thr).sum()), 'thr', thr)
