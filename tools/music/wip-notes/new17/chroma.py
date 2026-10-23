import sys, numpy as np, librosa
path=sys.argv[1]; bar0=int(sys.argv[2]); expect=sys.argv[3].split(',')
SPB=60/140; BAR=4*SPB
y,sr=librosa.load(path,sr=22050,mono=True)
y=librosa.effects.preemphasis(y,coef=0.0)
import scipy.signal as ss
y=ss.sosfilt(ss.butter(4,130,'high',fs=sr,output='sos'),y)
C=librosa.feature.chroma_cqt(y=y,sr=sr,hop_length=512,fmin=librosa.note_to_hz('C3'),n_octaves=5)
names='C C# D Eb E F F# G Ab A Bb B'.split()
TONES={'Cm':{0,3,7},'Ab':{8,0,3},'Fm':{5,8,0},'G':{7,11,2},'Bb':{10,2,5},'Gb':{6,10,1}}
ok=0; tot=0
for i,ch in enumerate(expect):
    a=int((i*BAR+0.05)*sr/512); b=int(((i+1)*BAR-0.05)*sr/512)
    if b>C.shape[1]: break
    v=C[:,a:b].mean(1); top=np.argsort(v)[::-1][:3]
    hit=len(set(top)&TONES[ch]); ok+=hit>=2; tot+=1
    print(bar0+i, ch, [names[k] for k in top], 'OK' if hit>=2 else ('weak' if hit==1 else 'BAD'))
print('bars with >=2 of top3 in chord: %d/%d'%(ok,tot))
