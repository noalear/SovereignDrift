import numpy as np, soundfile as sf, subprocess, scipy.signal as ss, librosa
SR=48000; SPB=60/128
def load(p):
    raw=subprocess.run(['ffmpeg','-v','quiet','-i',p,'-f','f32le','-ac','2','-ar','48000','-'],capture_output=True).stdout
    return np.frombuffer(raw,np.float32).reshape(-1,2).T
import sys
old=load(sys.argv[1]); new=load(sys.argv[2])
for nm,x in (('old',old),('new',new)):
    h=ss.sosfilt(ss.butter(4,8000,'high',fs=SR,output='sos'),x.mean(0))
    d=np.abs(np.diff(h)); thr=3*np.percentile(d,99.99)
    print(nm,'clicks(>3x p99.99 jumps):',int((d>thr).sum()),'peak %.2f dBFS'%(20*np.log10(np.abs(x).max())))
# chroma per bar (above 130 Hz) top-3, old vs new
def chroma(x):
    y=librosa.resample(x.mean(0).astype(np.float32),orig_sr=SR,target_sr=22050)
    y=librosa.effects.harmonic(y)
    c=librosa.feature.chroma_cqt(y=y,sr=22050,hop_length=512,fmin=130.8,n_octaves=5)
    fr=librosa.frames_to_time(np.arange(c.shape[1]),sr=22050,hop_length=512)
    bars=(fr/(4*SPB)).astype(int)
    return np.array([c[:,bars==b].mean(1) for b in range(95)])
co,cn=chroma(old),chroma(new)
same=sum(set(np.argsort(co[b])[-3:])==set(np.argsort(cn[b])[-3:]) for b in range(95))
cos=np.mean([co[b]@cn[b]/np.linalg.norm(co[b])/np.linalg.norm(cn[b]) for b in range(95)])
print('chroma top-3 identical in %d/95 bars; mean cosine %.4f'%(same,cos))
