import sys, numpy as np, soundfile as sf, scipy.signal as ss
x, sr = sf.read(sys.argv[1]); x = x.T
mono = x.mean(0)
Y = np.abs(np.fft.rfft(mono))**2; f = np.fft.rfftfreq(len(mono), 1/sr)
lo = [40,80,160,315,630,1250,2500,5000,10000]
b = [10*np.log10(Y[(f>=l)&(f<2*l)].sum()+1e-12) for l in lo]
print('tilt ', ' '.join(f'{v-b[0]:6.1f}' for v in b), '(ref 0 -2 -6 -9 -11 -13 -15 -18 -25)')
sos = ss.butter(4, 8000, 'high', fs=sr, output='sos'); h = ss.sosfilt(sos, mono)
d = np.abs(np.diff(h)); thr = 3*np.percentile(d, 99.99); print('clicks', int((d > thr).sum()), 'peak', 20*np.log10(np.abs(x).max()))
sl = ss.sosfilt(ss.butter(4,150,'low',fs=sr,output='sos'), x)
print('corr all %.3f  <150 %.3f' % (np.corrcoef(x[0],x[1])[0,1], np.corrcoef(sl[0],sl[1])[0,1]))
