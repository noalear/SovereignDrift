import sys, pretty_midi as pm, collections
f=sys.argv[1]; b0=int(sys.argv[2]); b1=int(sys.argv[3]); chs=set(map(int,sys.argv[4].split(','))) if len(sys.argv)>4 else None

import mido
mf=mido.MidiFile(f); tpb=mf.ticks_per_beat
# per channel via mido
events=collections.defaultdict(list)
for tr in mf.tracks:
    t=0; on={}
    prog={}
    for msg in tr:
        t+=msg.time
        if msg.type=='note_on' and msg.velocity>0:
            on[(msg.channel,msg.note)]=(t,msg.velocity)
        elif msg.type in('note_off','note_on'):
            k=(msg.channel,msg.note)
            if k in on:
                s,v=on.pop(k); events[msg.channel+1].append((s,msg.note,t-s,v))
names=['C','C#','D','Eb','E','F','F#','G','Ab','A','Bb','B']
def nn(n): return names[n%12]+str(n//12-1)
for ch in sorted(events):
    if chs and ch not in chs: continue
    print('=== ch',ch)
    bars=collections.defaultdict(list)
    for s,n,d,v in sorted(events[ch]):
        bar=s//(4*tpb)+1; beat=(s%(4*tpb))/tpb
        bars[bar].append(f"{beat:.2f}:{nn(n)}/{d/tpb:.2f}" if ch!=10 else f"{beat:.2f}:{n}")
    for b in range(b0,b1+1):
        if bars[b]: print(f"{b:3d} "+' '.join(bars[b]))
