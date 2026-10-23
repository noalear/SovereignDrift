import sys, numpy as np, soundfile as sf, librosa, scipy.signal as ss
x, sr = sf.read(sys.argv[1], always_2d=True); y = x.mean(1)
bar0 = int(sys.argv[2]); spb = 60/118; bl = 4*spb
y = ss.sosfilt(ss.butter(4,130,'high',fs=sr,output='sos'), y)
y = librosa.resample(y, orig_sr=sr, target_sr=22050); sr2 = 22050
C = librosa.feature.chroma_cqt(y=y, sr=sr2, hop_length=512)
NN = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']
fr = sr2/512
for i in range(int(len(y)/sr2/bl)):
    seg = C[:, int(i*bl*fr):int((i+1)*bl*fr)].mean(1)
    top = np.argsort(seg)[::-1][:4]
    print(bar0+i, ' '.join(f'{NN[k]}:{seg[k]:.2f}' for k in top))
