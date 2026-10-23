import sys, numpy as np
sys.path.insert(0, sys.argv[1])
import synth_new06 as I, synth as S
from synth import SR
def lvl(x, g_db, v):
    x = x * 10**(g_db/20) * v
    n = int(0.4*SR); y = np.zeros((2, n)); y[:, :min(n, x.shape[1])] = x[:, :n]
    import pyloudnorm as pyln
    rms = 10*np.log10(np.mean(y**2)+1e-12); pk = 20*np.log10(np.abs(y).max())
    return rms, pk
print('snare', lvl(I.heavy_snare(1.0, 0), -4.5, 1.0))
print('kick', lvl(I.ebm_kick(), -3, 1.0))
print('anvil', lvl(I.clank('anvil', -19, 1.0, 1, maxlen=0.9, lp=4000, decay=0.3), 6, 0.5))
print('brake', lvl(I.clank('brake', -3, 1.0, 0), 6, 0.55))
print('brake-5', lvl(S.pan2(I.clank('brake', -5, 1.0, 1).mean(0), .45), 6, 0.5))
print('pipe', lvl(I.metal_hit(58, 1.0, decay=0.12, bright=0.7, seed=58), 6, 0.22))
for v in range(3): print('hammer', v, lvl(I.hammer(v), 6, 1.0))
print('hyd', lvl(I.hydraulic(), 6, 1.0))
print('press', lvl(I.press(), 6, 1.0))
