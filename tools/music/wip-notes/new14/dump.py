import pretty_midi, sys, collections
m=pretty_midi.PrettyMIDI(sys.argv[1]); off=float(sys.argv[2]) # pickup beats
bpm=m.get_tempo_changes()[1][0]; bl=60/bpm
for i,ins in enumerate(m.instruments):
    if ins.is_drum: continue
    bars=collections.defaultdict(list)
    for n in sorted(ins.notes,key=lambda n:(n.start,n.pitch)):
        b=n.start/bl-off; bar=int(b//4)+1; bt=b-(bar-1)*4
        bars[bar].append(f"{bt:g}:{pretty_midi.note_number_to_name(n.pitch)}/{(n.end-n.start)/bl:.2g}")
    print(f"### inst {i} ch? prog {ins.program} {pretty_midi.program_to_instrument_name(ins.program)} name={ins.name}")
    for b in sorted(bars): print(f"  {b:3d} | "+" ".join(bars[b]))
