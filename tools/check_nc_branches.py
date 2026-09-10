"""Practice branch checks: Patchouli fake shots, Sakuya HP, rage and early timer lock."""
import sys,time,json
from pathlib import Path
import probe_nc as p
pid=int(sys.argv[1]);rows=[]
tail='--tail' in sys.argv
if tail:rows=json.loads(Path('build/nc-branches.json').read_text(encoding='utf-8'))[:32]
def check(name,delay=2,**settings):
    before=p.inspect(pid)
    patch=dict(flags=1,fps=240,shot=0,fakeShot=-1,dialogue=0,phase=0)
    patch.update(settings);patch['restart']=before['settings']['restart']+1;p.inspect(pid,patch)
    end=time.monotonic()+10
    while time.monotonic()<end:
        state=p.inspect(pid)
        if state['status']['error']:raise RuntimeError(state)
        if state['status']['starts']>before['status']['starts']:break
        time.sleep(.05)
    else:raise RuntimeError('Retry timed out')
    time.sleep(delay);row=dict(name=name,status=p.inspect(pid),memory=p.memory(pid))
    assert row['memory']['practice']==1 and row['memory']['spellPractice']==0 and row['memory']['enemies'],row
    rows.append(row);Path('build/nc-branches.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
    print(name,[(e['sub'],e['hp'])for e in row['memory']['enemies']],flush=True)
    return row
for fake in ([] if tail else range(4)):
    for section in range(24,31):
        row=check(f'patchouli-{fake}-{section}',stage=4,difficulty=2,fakeShot=fake,section=section)
        assert row['memory']['shot']==0
        if section!=24:assert any(e['sub']!=27 for e in row['memory']['enemies'])
for shot,hp in ([] if tail else enumerate([750,1000,1100,1100])):
    row=check(f'sakuya-{shot}',stage=6,difficulty=2,shot=shot,section=40)
    assert row['memory']['enemies'][0]['hp']==hp,row
for phase,wait in enumerate([280,40]):
    # var5 is decremented by the ECL wait loop after the native callback.
    # Observe its reset value across cycles instead of sampling it once.
    row=check(f'rage-{phase}',stage=7,difficulty=4,section=70,phase=phase)
    values=[];end=time.monotonic()+2
    while time.monotonic()<end:
        values.append(p.memory(pid)['enemies'][0]['var5']);time.sleep(.01)
    row['waitSamples']=values
    assert wait-20<=max(values)<=wait,row
for stage,section in [(1,1),(2,7),(4,23),(5,31),(1,3)]:
    row=check(f'early-time-lock-{stage}-{section}',stage=stage,difficulty=2,section=section,flags=17)
    assert row['memory']['enemies'][0]['hp']>1,row
row=check('infinite-resources',stage=1,difficulty=2,section=3,flags=15,lives=0,bombs=0,power=0)
assert all(row['status']['status'][k]==v for k,v in dict(lives=8,bombs=8,power=128).items())
row=check('auto-bomb',delay=8,stage=1,difficulty=2,section=3,flags=36,lives=8,bombs=8,power=128)
assert row['memory']['bombsUsed']>0 and row['memory']['misses']==0,row
