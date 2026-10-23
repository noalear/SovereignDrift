import sys, numpy as np, soundfile as sf, scipy.signal as ss
x, sr = sf.read(sys.argv[1], always_2d=True); x = x.T
bar0 = float(sys.argv[2]) if len(sys.argv) > 2 else 0
spb = 60/118; bl = 4*spb
mono = x.mean(0)
print('peak dBFS %.2f' % (20*np.log10(np.abs(x).max())))
# octave bands
edges = [40,80,160,315,630,1250,2500,5000,10000,20000]
f, P = ss.welch(mono, sr, nperseg=16384)
bands = [10*np.log10(P[(f>=a)&(f<b)].sum()+1e-20) for a,b in zip(edges[:-1],edges[1:])]
print('oct bands rel 40-80:', ' '.join('%5.1f' % (b-bands[0]) for b in bands))
# section rms per 4 bars
nb = int(len(mono)/sr/bl)
rows=[]
for i in range(0, nb, 4):
    seg = x[:, int(i*bl*sr):int((i+4)*bl*sr)]
    if seg.shape[1] < sr: break
    r = 20*np.log10(np.sqrt((seg**2).mean())+1e-12)
    lo = ss.sosfilt(ss.butter(4,150,'low',fs=sr,output='sos'), seg)
    c = np.corrcoef(seg[0],seg[1])[0,1]; cl = np.corrcoef(lo[0],lo[1])[0,1]
    rows.append('bar %3d: rms %6.1f corr %.2f lowcorr %.2f' % (bar0+i, r, c, cl))
print('\n'.join(rows))
# clicks
hp = ss.sosfilt(ss.butter(4,8000,'high',fs=sr,output='sos'), mono)
d = np.abs(np.diff(hp)); thr = 3*np.percentile(d, 99.99)
print('click candidates:', int((d>thr).sum()), 'thr %.4f' % thr)
