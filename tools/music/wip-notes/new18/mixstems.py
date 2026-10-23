import sys, os, numpy as np, soundfile as sf
d = sys.argv[1]; out = sys.argv[2]; keep = sys.argv[3].split(',')
acc = None
for k in keep:
    p = os.path.join(d, k + '.wav')
    if not os.path.exists(p): continue
    x, sr = sf.read(p, dtype='float32')
    acc = x if acc is None else acc + x
sf.write(out, acc, sr, subtype='FLOAT')
