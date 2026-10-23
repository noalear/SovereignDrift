import numpy as np, soundfile as sf, glob, os
SR=48000; SPB=60/128
def st(d,k): x,_=sf.read(f'{d}/{k}.wav',dtype='float32'); return x.T
for d in ('st1','st2'):
    met=st(d,'metal'); sn=st(d,'snare'); allm=sum(st(d,os.path.basename(p)[:-4]) for p in glob.glob(d+'/*.wav'))
    w=int(0.05*SR)
    def env(x):
        p=np.mean(x**2,0); c=np.convolve(p,np.ones(w)/w,'same'); return 10*np.log10(c+1e-12)
    em,es,ea=env(met),env(sn),env(allm)
    # onsets of metal stem: local maxima of 50ms env above (max-?); group by grid position (16ths)
    hop=int(SPB/4*SR)
    n16=len(em)//hop
    pk=np.array([em[i*hop:i*hop+int(0.06*SR)].max() for i in range(n16)])
    pos=np.arange(n16)%16
    bars=np.arange(n16)//16
    grv=(bars>=8)&(bars<40)|(bars>=56)&(bars<72)
    print('==',d,'metal stem: loudest 50ms level per 16th slot in grooves (median over bars, max over bars)')
    for s in range(16):
        sel=grv&(pos==s); print('  slot %2d (beat %.2f): med %6.1f  max %6.1f'%(s,s/4+1,np.median(pk[sel]),pk[sel].max()))
    ps=np.array([es[i*hop:i*hop+int(0.06*SR)].max() for i in range(n16)])
    pa=np.array([ea[i*hop:i*hop+int(0.06*SR)].max() for i in range(n16)])
    print('  snare at beats 2/4 median %.1f ; full mix (all stems) median per slot %.1f'%(np.median(ps[grv&((pos==4)|(pos==12))]),np.median(pa[grv])))
    # loudest metal events overall relative to full mix
    i=np.argsort(pk)[-6:]
    for j in i: print('   loud metal: bar %d beat %.2f metal %.1f mix %.1f'%(bars[j],pos[j]/4+1,pk[j],pa[j]))
