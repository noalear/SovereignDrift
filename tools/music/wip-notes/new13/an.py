import sys, numpy as np, soundfile as sf, scipy.signal as ss
x, sr = sf.read(sys.argv[1], always_2d=True); x = x.T
bpm = 130; bar = 240/bpm
off = float(sys.argv[2]) if len(sys.argv)>2 else 0
def bands(seg):
    f, P = ss.welch(seg.mean(0), sr, nperseg=8192)
    edges = [40,80,160,315,630,1250,2500,5000,10000,20000]
    e = [10*np.log10(P[(f>=a)&(f<b)].sum()+1e-20) for a,b in zip(edges[:-1],edges[1:])]
    return np.array(e)-e[0]
print('bands', ' '.join(f'{v:6.1f}' for v in bands(x)))
lo = ss.sosfilt(ss.butter(4,150,'low',fs=sr,output='sos'), x)
print('corr all %.3f  <150 %.3f' % (np.corrcoef(x)[0,1], np.corrcoef(lo)[0,1]))
hp = ss.sosfilt(ss.butter(4,8000,'high',fs=sr,output='sos'), x.mean(0))
d = np.abs(np.diff(hp)); thr = 3*np.percentile(d, 99.99); print('clicks', int((d>thr).sum()), 'peak', 20*np.log10(np.abs(x).max()))
nb = int(x.shape[1]/sr/bar)
r = [20*np.log10(np.sqrt(np.mean(x[:, int(i*bar*sr):int((i+1)*bar*sr)]**2))+1e-9) for i in range(nb)]
print('bar rms:', ' '.join(f'{int(off)+i}:{v:.1f}' for i,v in enumerate(r)))
