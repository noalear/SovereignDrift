import sys, pretty_midi as pm
f=sys.argv[1]; p=pm.PrettyMIDI(f); bpm=p.get_tempo_changes()[1][0]; spb=60/bpm
off=float(sys.argv[2]) if len(sys.argv)>2 else 0  # beats offset
for i,ins in enumerate(p.instruments):
    if ins.is_drum: continue
    print(f'## {i} {ins.name} prog{ins.program}')
    bars={}
    for n in sorted(ins.notes,key=lambda n:n.start):
        b=n.start/spb-off; bar=int(b//4); 
        bars.setdefault(bar,[]).append(f'{b-bar*4:.2f}:{pm.note_number_to_name(n.pitch)}/{(n.end-n.start)/spb:.2f}v{n.velocity}')
    for bar in sorted(bars): print(f'  {bar+1:3d} | '+' '.join(bars[bar]))
