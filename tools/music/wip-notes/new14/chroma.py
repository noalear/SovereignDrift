# chroma per bar of the dry tonal stem vs the intended chords
import sys, numpy as np, soundfile as sf, librosa
sys.path.insert(0, '/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm')
x, sr = sf.read(sys.argv[1]); y = x.mean(1); bar0 = int(sys.argv[2]) if len(sys.argv) > 2 else 0
SPB = 60/125; BL = 4*SPB
import scipy.signal as ss
y = ss.sosfilt(ss.butter(4, 130, 'high', fs=sr, output='sos'), y)
PCS = {'Cm':'C Eb G','Db':'Db F Ab','Ab':'Ab C Eb','Fm':'F Ab C','G':'G B D','Bbm':'Bb Db F'}
NM = ['C','Db','D','Eb','E','F','Gb','G','Ab','A','Bb','B']
# read CHORDS from the script without running it
src = open('/tmp/claude-0/-home-claude/bdcf4380-7c02-523f-bd93-ae04cd7c35ec/scratchpad/work/tools/music/edm/new14_electro.py').read()
ns = {}
exec(src[src.index("VAMP = "):src.index("assert len(CHORDS)")], ns)
CH = ns['CHORDS']
C = librosa.feature.chroma_cqt(y=y.astype(np.float32), sr=sr, hop_length=1024, fmin=65)
fr = librosa.frames_to_time(np.arange(C.shape[1]), sr=sr, hop_length=1024)
good = 0; tot = 0
for b in range(int(len(y)/sr/BL)):
    sel = (fr >= b*BL) & (fr < (b+1)*BL)
    if not sel.any(): continue
    v = C[:, sel].mean(1); top = [NM[i] for i in np.argsort(v)[::-1][:3]]
    ch = CH[bar0+b] if bar0+b < len(CH) else '?'
    exp = PCS.get(ch, '').split()
    hit = sum(t in exp for t in top); tot += 1; good += hit >= 2
    print(f'bar {bar0+b:3d} {ch:4s} top3 {" ".join(top):12s} match {hit}/3')
print(f'bars with >=2/3 chord tones in top 3: {good}/{tot}')
