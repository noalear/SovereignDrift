import pretty_midi, sys
pm = pretty_midi.PrettyMIDI(sys.argv[1])
spb = 60/pm.get_tempo_changes()[1][0]
names=['C','C#','D','Eb','E','F','F#','G','Ab','A','Bb','B']
nm=lambda p: names[p%12]+str(p//12-1)
chs = sys.argv[2].split(',') if len(sys.argv)>2 else None
b0,b1 = (int(sys.argv[3]),int(sys.argv[4])) if len(sys.argv)>4 else (1,999)
for i,ins in enumerate(pm.instruments):
    if ins.is_drum: continue
    if chs and str(i) not in chs: continue
    bars={}
    for n in ins.notes:
        beat=n.start/spb; bar=int(beat//4)+1
        bars.setdefault(bar,[]).append((beat%4, nm(n.pitch), (n.end-n.start)/spb))
    print(f'### inst {i} prog {ins.program} {pretty_midi.program_to_instrument_name(ins.program)} n={len(ins.notes)}')
    for b in sorted(bars):
        if b0<=b<=b1:
            print(f'{b:3d}: '+' '.join(f'{st:g}:{p}/{d:.2g}' for st,p,d in sorted(bars[b])))
