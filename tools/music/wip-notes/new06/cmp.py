import sys, numpy as np, soundfile as sf, pyloudnorm as pyln, subprocess, scipy.signal as ss
SR=48000; SPB=60/128
def load(p):
    if p.endswith('.ogg'):
        raw=subprocess.run(['ffmpeg','-v','quiet','-i',p,'-f','f32le','-ac','2','-ar','48000','-'],capture_output=True).stdout
        return np.frombuffer(raw,np.float32).reshape(-1,2).T
    x,_=sf.read(p,dtype='float32',always_2d=True); return x.T
files=sys.argv[1:]
X={f:load(f) for f in files}
m=pyln.Meter(SR)
bands=[40,80,160,315,630,1250,2500,5000,10000,20000]
secs=[(0,8,'intro'),(8,16,'groove'),(16,32,'theme'),(32,40,'sigh'),(40,48,'brkdn'),(48,56,'build'),(56,72,'peak'),(72,80,'bridge'),(80,88,'climax'),(88,94,'outro')]
def T(b,beat=0): return (b*4+beat)*SPB
hp=ss.butter(2,[250,4000],'band',fs=SR,output='sos')
for f,x in X.items():
    print('==',f, 'len %.1f'%(x.shape[1]/SR),'LUFS %.2f'%m.integrated_loudness(x.T.astype(np.float64)))
    mono=x.mean(0); h=ss.sosfilt(hp,mono)
    def e(t,w=0.08):
        i=int(t*SR); return 10*np.log10(np.mean(h[i:i+int(w*SR)]**2)+1e-12)
    # transient energy at beat 3.5 in bars b%4==3 vs b%4!=3 (groove sections), and vs beat 3.0
    a=[e(T(b,3.5)) for b in range(8,40) if b%4==3]+[e(T(b,3.5)) for b in range(56,72) if b%4==3]
    o=[e(T(b,3.5)) for b in range(8,40) if b%4!=3]+[e(T(b,3.5)) for b in range(56,72) if b%4!=3]
    q=[e(T(b,3.0)) for b in list(range(8,40))+list(range(56,72))]
    print('  250-4k energy, beat 4.5 in every-4th bar %.1f dB | other bars %.1f | beat 4 %.1f'%(np.mean(a),np.mean(o),np.mean(q)))
    br=[e(T(b,2.5),0.3) for b in (41,43,45,47)]; br2=[e(T(b,2.0),0.3) for b in (41,43,45,47)]
    print('  breakdown odd bars beat3.5 %.1f, beat3 %.1f'%(np.mean(br),np.mean(br2)))
    for b0,b1,nm in secs:
        seg=x[:,int(T(b0)*SR):int(T(b1)*SR)]
        F,P=ss.welch(seg.mean(0),SR,nperseg=8192)
        ob=[10*np.log10(P[(F>=bands[i])&(F<bands[i+1])].sum()+1e-20) for i in range(9)]
        rms=20*np.log10(np.sqrt(np.mean(seg**2)))
        lo=ss.sosfilt(ss.butter(4,150,'low',fs=SR,output='sos'),seg)
        c=np.corrcoef(seg)[0,1]; cl=np.corrcoef(lo)[0,1]
        print('  %-7s rms %6.1f corr %.2f lo %.2f | '%(nm,rms,c,cl)+' '.join('%5.1f'%(v-ob[0]) for v in ob))
