import sys, numpy as np, soundfile as sf, scipy.signal as ss
import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
path=sys.argv[1]; bar0=float(sys.argv[2]) if len(sys.argv)>2 else 0
x,sr=sf.read(path,always_2d=True); x=x.T
spb=60/140; bl=4*spb
mono=x.mean(0)
nb=int(len(mono)/sr/bl)
print('len %.1fs peak %.2f dB'%(len(mono)/sr,20*np.log10(np.abs(x).max())))
# section RMS per 4 bars
for i in range(0,nb,4):
    s=mono[int(i*bl*sr):int((i+4)*bl*sr)]
    if len(s)==0: break
    print('bars %3d-%3d rms %6.1f dB'%(bar0+i,bar0+i+3,20*np.log10(np.sqrt(np.mean(s**2))+1e-12)),end='')
    seg=x[:,int(i*bl*sr):int((i+4)*bl*sr)]
    lo=ss.sosfilt(ss.butter(4,150,'low',fs=sr,output='sos'),seg)
    c=np.corrcoef(seg)[0,1]; cl=np.corrcoef(lo)[0,1]
    f,P=ss.welch(seg.mean(0),sr,nperseg=8192)
    edges=[40,80,160,315,630,1250,2500,5000,10000,20000]
    bands=[10*np.log10(P[(f>=edges[j])&(f<edges[j+1])].sum()+1e-20) for j in range(9)]
    print('  corr %.2f low %.2f | '%(c,cl)+' '.join('%5.1f'%(b-bands[0]) for b in bands))
# clicks
hp=ss.sosfilt(ss.butter(4,8000,'high',fs=sr,output='sos'),mono)
d=np.abs(np.diff(hp)); thr=3*np.percentile(d,99.99)
print('clicks >3x p99.99:',int((d>thr).sum()))
if len(sys.argv)>3:
    f,t,Z=ss.spectrogram(mono,sr,nperseg=4096,noverlap=2048)
    fig,ax=plt.subplots(2,1,figsize=(18,8),gridspec_kw={'height_ratios':[4,1]})
    ax[0].pcolormesh(t,f,10*np.log10(Z+1e-14),vmin=-120,vmax=-30,shading='auto',cmap='magma'); ax[0].set_yscale('symlog',linthresh=200); ax[0].set_ylim(30,20000)
    for b in [8,16,32,48,56,64,80,88,104,110]:
        ax[0].axvline(b*bl,color='c',lw=0.6); ax[1].axvline(b*bl,color='c',lw=0.6)
    w=int(0.2*sr); r=[20*np.log10(np.sqrt(np.mean(mono[i:i+w]**2))+1e-9) for i in range(0,len(mono)-w,w)]
    ax[1].plot(np.arange(len(r))*0.2,r); ax[1].set_ylim(-50,0)
    plt.tight_layout(); plt.savefig(sys.argv[3],dpi=70)
