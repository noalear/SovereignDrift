import sys, numpy as np, librosa, scipy.signal as ss
sys.path.insert(0,'/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm')
y, sr = librosa.load(sys.argv[1], sr=22050, mono=True)
y = ss.sosfilt(ss.butter(4,130,'high',fs=sr,output='sos'), y)
bar = 240/130; off = int(sys.argv[2]) if len(sys.argv)>2 else 0
C = librosa.feature.chroma_cqt(y=y, sr=sr, hop_length=512)
names = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']
fr = sr/512
for i in range(int(len(y)/sr/bar)):
    c = C[:, int(i*bar*fr):int((i+1)*bar*fr)].mean(1)
    top = np.argsort(c)[::-1][:3]
    print(off+i, ' '.join(names[t] for t in top))
