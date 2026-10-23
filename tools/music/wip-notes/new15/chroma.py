import sys, numpy as np, librosa, scipy.signal as ss
path=sys.argv[1]; bar0=int(sys.argv[2])
y,sr=librosa.load(path,sr=22050,mono=True)
y=ss.sosfilt(ss.butter(4,130,'high',fs=sr,output='sos'),y)
C=librosa.feature.chroma_cqt(y=y,sr=sr,hop_length=512)
bl=4*60/140; fps=sr/512
names=['C','C#','D','Eb','E','F','F#','G','Ab','A','Bb','B']
for b in range(int(C.shape[1]/fps/bl)):
    c=C[:,int(b*bl*fps):int((b+1)*bl*fps)].mean(1)
    top=np.argsort(c)[::-1][:4]
    print(bar0+b, ' '.join('%s%.2f'%(names[i],c[i]) for i in top))
