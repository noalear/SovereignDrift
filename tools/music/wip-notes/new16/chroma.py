import sys, numpy as np, soundfile as sf, librosa, scipy.signal as ss
sys.path.insert(0,'/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm')
x, sr = sf.read(sys.argv[1], dtype='float32'); y = x.mean(1)
bar0 = int(sys.argv[2])
y = ss.sosfilt(ss.butter(4, 130, 'high', fs=sr, output='sos'), y)
y = librosa.resample(y, orig_sr=sr, target_sr=22050)
C = librosa.feature.chroma_cqt(y=y, sr=22050, hop_length=512)
BAR = 4*60/172; fps = 22050/512
names=['C','C#','D','D#','E','F','F#','G','G#','A','A#','B']
# chords from script
import re
src=open('/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm/new16_neurofunk.py').read()
PROG={'A': ['F#m', 'F#m', 'G#m', 'G#m'],'B': ['C#', 'C#', 'F#m/A', 'F#m/A'],'C': ['F#m', 'C#', 'D', 'C#']}
FORM=[(0, 'A', 4), (16, 'A', 2), (24, 'B', 2),(32, 'A', 4), (48, 'B', 2), (56, 'C', 2),(64, 'A', 2), (72, 'B', 2),(80, 'A', 2), (88, 'B', 2), (96, 'C', 2), (104, 'A', 2),(112, 'A', 2), (120, 'C', 2)]
CH=[]
for b0,p,c in FORM:
    for i in range(4*c): CH.append(PROG[p][i%4])
CH.append('F#m')
TONES={'F#m':{'F#','A','C#'},'G#m':{'G#','B','D#'},'C#':{'C#','F','G#'},'F#m/A':{'F#','A','C#'},'D':{'D','F#','A'}}
ok=0;tot=0
for b in range(int(C.shape[1]/fps/BAR)):
    seg=C[:, int(b*BAR*fps):int((b+1)*BAR*fps)].mean(1)
    top=[names[i] for i in np.argsort(seg)[::-1][:3]]
    ch=CH[bar0+b]; hit=len(set(top)&TONES[ch])
    tot+=1; ok+= hit>=2
    print(bar0+b, ch, top, hit)
print('bars with >=2 of top3 chord tones: %d/%d'%(ok,tot))
