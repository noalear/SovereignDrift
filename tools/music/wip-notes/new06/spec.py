import numpy as np, subprocess, matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt, scipy.signal as ss, sys
SR=48000; SPB=60/128
def load(p):
    raw=subprocess.run(['ffmpeg','-v','quiet','-i',p,'-f','f32le','-ac','2','-ar','48000','-'],capture_output=True).stdout
    return np.frombuffer(raw,np.float32).reshape(-1,2).T.mean(0)
old,new=load(sys.argv[1]),load(sys.argv[2])
fig,ax=plt.subplots(4,1,figsize=(16,14),gridspec_kw={'height_ratios':[3,1,2,2]})
f,t,P=ss.spectrogram(new,SR,nperseg=4096,noverlap=2048)
ax[0].pcolormesh(t,f,10*np.log10(P+1e-14),vmin=-120,vmax=-40,shading='auto',cmap='magma'); ax[0].set_yscale('symlog',linthresh=200); ax[0].set_ylim(30,20000)
for b in (8,16,32,40,48,56,72,80,88,94): ax[0].axvline(b*4*SPB,color='c',lw=0.8)
w=int(0.2*SR); r=[20*np.log10(np.sqrt(np.mean(new[i:i+w]**2))+1e-9) for i in range(0,len(new)-w,w)]
ax[1].plot(np.arange(len(r))*0.2,r); ax[1].set_ylabel('RMS dB'); ax[1].set_xlim(0,t[-1])
# zoom: bars 60-64 (v1 anvil at bar 63 beat 4.5), 250-4k band envelope old vs new
a,b=int(60*4*SPB*SR),int(64*4*SPB*SR)
sos=ss.butter(2,[250,4000],'band',fs=SR,output='sos')
for k,(nm,x) in enumerate((('v1 (old)',old),('v2 (new)',new))):
    f2,t2,P2=ss.spectrogram(x[a:b],SR,nperseg=1024,noverlap=768)
    ax[2+k].pcolormesh(t2,f2,10*np.log10(P2+1e-14),vmin=-120,vmax=-40,shading='auto',cmap='magma'); ax[2+k].set_ylim(100,8000); ax[2+k].set_yscale('log')
    for q in range(17): ax[2+k].axvline(q*SPB,color='w' if q%4==0 else 'gray',lw=0.6)
    ax[2+k].set_title(nm+' bars 61-64 (beat lines)')
plt.tight_layout(); plt.savefig('spec_v2.png',dpi=70)
