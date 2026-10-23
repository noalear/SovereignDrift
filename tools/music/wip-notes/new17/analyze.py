import sys, numpy as np, soundfile as sf, pyloudnorm as pyln, scipy.signal as ss
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
path=sys.argv[1]; bar0=float(sys.argv[2]) if len(sys.argv)>2 else 0
SPB=60/140; BAR=4*SPB
x,sr=sf.read(path,always_2d=True); x=x.T
print('len %.1fs peak %.2f dBFS'%(x.shape[1]/sr, 20*np.log10(np.abs(x).max())))
m=pyln.Meter(sr); print('LUFS %.2f'%m.integrated_loudness(x.T))
SECT=[(0,'intro'),(8,'ost'),(16,'theme'),(32,'clash'),(40,'brk'),(48,'build'),(56,'peak'),(72,'bridge'),(80,'climax'),(96,'outro'),(100,'final'),(101,'end')]
bands=[40,80,160,315,630,1250,2500,5000,10000,20000]
def band_db(seg):
    f,P=ss.welch(seg.mean(0),sr,nperseg=8192); out=[]
    for lo,hi in zip(bands[:-1],bands[1:]):
        k=(f>=lo)&(f<hi); out.append(10*np.log10(P[k].sum()+1e-20))
    return np.array(out)
ref=None
for (a,nm),(b,_) in zip(SECT[:-1],SECT[1:]):
    i0=int((a-bar0)*BAR*sr); i1=int((b-bar0)*BAR*sr)
    if i1<=0 or i0>=x.shape[1]: continue
    seg=x[:,max(i0,0):min(i1,x.shape[1])]
    if seg.shape[1]<sr: continue
    rms=20*np.log10(np.sqrt((seg**2).mean())+1e-12)
    bd=band_db(seg); 
    if ref is None: ref=bd[0]
    lo=ss.sosfilt(ss.butter(4,150,'low',fs=sr,output='sos'),seg)
    c_lo=np.corrcoef(lo)[0,1]; c=np.corrcoef(seg)[0,1]
    print(f'{nm:7s} rms {rms:6.1f}  corr {c:.2f} lo {c_lo:.2f}  bands '+' '.join(f'{v-bd[0]:5.0f}' for v in bd))
hp=ss.sosfilt(ss.butter(4,8000,'high',fs=sr,output='sos'),x.mean(0))
d=np.abs(np.diff(hp)); thr=3*np.percentile(d,99.99); print('clicks', int((d>thr).sum()), 'thr', thr)
if len(sys.argv)>3:
    f,t,Z=ss.spectrogram(x.mean(0),sr,nperseg=4096,noverlap=2048)
    fig,ax=plt.subplots(2,1,figsize=(18,8),gridspec_kw={'height_ratios':[4,1]},sharex=True)
    ax[0].pcolormesh(t,f,10*np.log10(Z+1e-12),vmin=-120,vmax=-30,shading='auto'); ax[0].set_yscale('symlog',linthresh=200); ax[0].set_ylim(30,20000)
    hop=int(0.1*sr); r=[20*np.log10(np.sqrt((x[:,i:i+hop]**2).mean())+1e-9) for i in range(0,x.shape[1]-hop,hop)]
    ax[1].plot(np.arange(len(r))*0.1,r); ax[1].set_ylim(-50,0)
    for a,nm in SECT:
        tt=(a-bar0)*BAR
        for k in ax: k.axvline(tt,color='w' if k is ax[0] else 'k',lw=0.8)
        ax[0].text(tt,15000,nm,color='w')
    plt.tight_layout(); plt.savefig(sys.argv[3],dpi=70)
