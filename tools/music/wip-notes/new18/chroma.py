import sys, numpy as np, librosa, scipy.signal as ss
LOOP = ['Dm9', 'Ebmaj9', 'F69', 'Gm9']
THB = ['Gm9', 'Bbmaj9', 'A7sus4', 'Dm9', 'Em7b5', 'Bb69', 'A7sus4', 'A7b13']
BRK = ['Dm9', 'Dm9', 'Ebmaj9', 'Ebmaj9', 'F69', 'F69', 'Gm9', 'A7sus4']
OUT = LOOP + ['Dm9', 'Ebmaj9', 'Gm9', 'A7sus4', 'Dm9', 'Dm9']
CH = LOOP*2+LOOP*2+LOOP*2+THB+BRK+LOOP*2+THB+LOOP*2+OUT
TONES = {'Dm9': 'D F A C E', 'Ebmaj9': 'Eb G Bb D F', 'F69': 'F A C D G', 'Gm9': 'G Bb D F A',
 'Bbmaj9': 'Bb D F A C', 'Bb69': 'Bb D F G C', 'A7sus4': 'A D E G', 'A7b13': 'A C# E G F', 'Em7b5': 'E G Bb D'}
PC = {'C':0,'C#':1,'D':2,'Eb':3,'E':4,'F':5,'F#':6,'G':7,'Ab':8,'A':9,'Bb':10,'B':11}
names = 'C C# D Eb E F F# G Ab A Bb B'.split()
y, sr = librosa.load(sys.argv[1], sr=22050, mono=True)
bar0 = int(sys.argv[2]) if len(sys.argv) > 2 else 0
y = ss.sosfilt(ss.butter(4, 130, 'high', fs=sr, output='sos'), y)
hop = 512
C = librosa.feature.chroma_cqt(y=y, sr=sr, hop_length=hop)
BAR = 4*60/92
ok = 0; tot = 0
for i in range(int(len(y)/sr/BAR)):
    b = bar0+i
    if b >= len(CH): break
    a, e = int(i*BAR*sr/hop), int((i+1)*BAR*sr/hop)
    v = C[:, a:e].mean(1)
    top = np.argsort(v)[::-1][:3]
    tones = {PC[t] for t in TONES[CH[b]].split()}
    hit = sum(t in tones for t in top)
    tot += 1; ok += hit == 3
    print(f'{b:3d} {CH[b]:7s} top3 {" ".join(names[t] for t in top):12s} {hit}/3')
print(f'{ok}/{tot} bars with all top-3 in chord')
