"""Regression checks for NC menu, resource and added-section fixes.

Use only a disposable game copy already in custom Practice Start. The deliberate
death/HP writes below exercise the native callback paths in that test process.
"""
import ctypes as c
import atexit,json,struct,sys,time
from pathlib import Path
import probe_nc as p

pid=int(sys.argv[1]);rows=[]
atexit.register(lambda:p.inspect(pid,dict(flags=p.inspect(pid)['settings']['flags']|1)))
def capture(name):
    row=dict(name=name,status=p.inspect(pid),memory=p.memory(pid));rows.append(row)
    Path('build/nc-revision.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    print(name,row['status']['status'],flush=True);return row

def restart(**settings):
    before=p.inspect(pid)
    patch=dict(enabled=1,stage=1,difficulty=1,shot=0,section=3,frame=0,flags=1,fps=240,
               lives=4,bombs=2,power=96,rank=12,graze=0,point=0,score=0,phase=0,fakeShot=-1,dialogue=0)
    patch.update(settings);patch['restart']=before['settings']['restart']+1
    p.inspect(pid,patch);end=time.monotonic()+10
    while time.monotonic()<end:
        now=p.inspect(pid)
        assert not now['status']['error'],now
        if now['status']['starts']>before['status']['starts']:return
        time.sleep(.01)
    raise RuntimeError('Retry timed out')

def write(offset,fmt,value):
    process=p.k.OpenProcess(0x438,False,pid)
    p.k.K32EnumProcessModules.argtypes=[p.w.HANDLE,c.POINTER(c.c_void_p),p.w.DWORD,c.POINTER(p.w.DWORD)]
    modules=(c.c_void_p*1024)();needed=p.w.DWORD()
    assert p.k.K32EnumProcessModules(process,modules,c.sizeof(modules),c.byref(needed))
    p.k.WriteProcessMemory.argtypes=[p.w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
    data=struct.pack(fmt,value);n=c.c_size_t()
    assert p.k.WriteProcessMemory(process,modules[0]+offset,data,len(data),c.byref(n)) and n.value==len(data)
    p.k.CloseHandle(process)

def miss():
    write(0x506bf8,'<i',0);write(0x506adc,'<i',1);write(0x506c38,'<B',2)
    time.sleep(.65)

if '--continuation-only' not in sys.argv:
    restart(section=0,flags=0,fps=60)
    row=capture('stage-start-keeps-introduction')
    assert row['memory']['stageTitleScript'] and row['memory']['playerState'] in (1,3) and row['memory']['invulnerabilityTimer']>0,row
    for name,options in [('boss',dict(section=3)),('portion',dict(stage=3,section=10307)),('specified-frame',dict(section=0,frame=1000))]:
        restart(flags=0,fps=60,**options);row=capture(name+'-skips-introduction')
        assert row['memory']['stageTitleScript']==0 and row['memory']['stageTitleVisible']==0
        # This timer also counts upward in the normal, vulnerable state.
        assert row['memory']['playerState']==0 and row['memory']['invulnerabilityTimer']<10,row
    
    restart();time.sleep(1)
    p.key(pid,88);time.sleep(1.8);row=capture('bomb-unlocked-consumes-one')
    assert row['status']['status']['bombs']==1 and row['memory']['bombsUsed']==1,row
    p.inspect(pid,dict(flags=15));time.sleep(.15);row=capture('locks-capture-current-values')
    assert [row['status']['status'][k]for k in ['lives','bombs','power']]==[4,1,96],row
    p.key(pid,88);time.sleep(1.8);row=capture('locked-bomb-keeps-one')
    assert row['status']['status']['bombs']==1 and row['memory']['bombsUsed']==2,row
    p.inspect(pid,dict(flags=14));time.sleep(.1);miss();row=capture('locked-resources-survive-miss')
    assert [row['status']['status'][k]for k in ['lives','bombs','power']]==[4,1,96] and row['memory']['misses']>=1,row
    p.inspect(pid,dict(flags=0));time.sleep(.1);miss();row=capture('unlock-restores-native-loss')
    assert [row['status']['status'][k]for k in ['lives','bombs']]==[3,3] and 80<=row['status']['status']['power']<96,row
    p.inspect(pid,dict(flags=15));time.sleep(.1);row=capture('relocking-captures-new-values')
    assert [row['status']['status'][k]for k in ['lives','bombs']]==[3,3] and 80<=row['status']['status']['power']<96,row
    time.sleep(.3)
    assert p.inspect(pid)['status']['power']==row['status']['status']['power'],row
    restart(flags=14,lives=0,bombs=0,power=64);miss();row=capture('zero-life-lock-respawns')
    assert row['memory']['gameOver']==0 and [row['status']['status'][k]for k in ['lives','bombs','power']]==[0,0,64],row
    
    restart(stage=3,section=10307)
    end=time.monotonic()+5
    while time.monotonic()<end:
        if any(e['hp']==350 and e['sub']==8 for e in p.memory(pid)['enemies']):break
        time.sleep(.03)
    row=capture('stage3-second-half3-added-wave')
    assert row['memory']['practice']==1 and row['memory']['spellPractice']==0
    assert any(e['hp']==350 and e['sub']==8 for e in row['memory']['enemies']),row
    for section,sub,hp in [(71,104,3900),(72,109,4000),(73,115,6000)]:
        restart(stage=7,difficulty=4,section=section);time.sleep(3)
        row=capture('added-spell-'+str(section))
        assert any(e['boss'] and e['sub']==sub and e['hp']==hp for e in row['memory']['enemies']),row
        assert row['memory']['practice']==1 and row['memory']['spellPractice']==0 and row['memory']['stageTitleScript']==0,row
else:
    rows=json.loads(Path('build/nc-revision.json').read_text(encoding='utf-8'))
    rows=[r for r in rows if not r['name'].startswith('added-spells-')]
restart(stage=7,difficulty=4,section=71);time.sleep(2)
for name,sub,hp in [('first-to-second',109,4000),('second-to-third',115,6000)]:
    write(0xaa1e98+0x234,'<i',0);end=time.monotonic()+8
    while time.monotonic()<end:
        if any(e['boss'] and e['sub']==sub and e['hp']==hp for e in p.memory(pid)['enemies']):break
        time.sleep(.05)
    row=capture('added-spells-'+name)
    assert any(e['boss'] and e['sub']==sub and e['hp']==hp for e in row['memory']['enemies']),row
restart(stage=1,section=3,fps=60);p.key(pid,27)
print('PASS; left paused for visual UI checks',flush=True)
