import sys, pretty_midi as pm
p = pm.PrettyMIDI(sys.argv[1]); shift = float(sys.argv[2]) if len(sys.argv)>2 else 0
spb = 60/p.get_tempo_changes()[1][0]
names='C C# D Eb E F F# G Ab A Bb B'.split()
nn=lambda n: names[n%12]+str(n//12-1)
for i,ins in enumerate(p.instruments):
    if ins.is_drum: continue
    print(f'### inst {i} ch? prog {ins.program} {pm.program_to_instrument_name(ins.program)} n={len(ins.notes)}')
    bars={}
    for n in sorted(ins.notes,key=lambda n:(n.start,n.pitch)):
        b=n.start/spb - shift; bar=int(b//4)+1; beat=b-4*(bar-1)
        bars.setdefault(bar,[]).append(f'{beat:g}:{nn(n.pitch)}/{(n.end-n.start)/spb:.2g}')
    for bar in sorted(bars): print(f'{bar:3d} | '+' '.join(bars[bar]))
