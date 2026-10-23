import pretty_midi, sys
pm = pretty_midi.PrettyMIDI(sys.argv[1])
shift = float(sys.argv[2])  # beats of pickup
spb = 60/180
names=['C','C#','D','Eb','E','F','F#','Ab','A','Bb','B','B']
names=['C','C#','D','Eb','E','F','F#','G','Ab','A','Bb','B']
def nn(p): return f"{names[p%12]}{p//12-1}"
for i,ins in enumerate(pm.instruments):
    if ins.is_drum: continue
    print(f"### track {i} prog {ins.program} {pretty_midi.program_to_instrument_name(ins.program)} n={len(ins.notes)}")
    bars={}
    for n in sorted(ins.notes,key=lambda n:(n.start,n.pitch)):
        b=n.start/spb - shift
        bar=int(b//4); beat=b-bar*4
        bars.setdefault(bar,[]).append(f"{beat:g}:{nn(n.pitch)}/{(n.end-n.start)/spb:.2g}")
    for bar in sorted(bars):
        print(f" {bar:3d} | "+" ".join(bars[bar]))
