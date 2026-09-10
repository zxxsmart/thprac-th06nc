"""Runtime feature regression against an already opened custom practice session."""
import sys,time,json
from pathlib import Path
import probe_nc as p
pid=int(sys.argv[1]);game=Path(sys.argv[2])
def restart(**settings):
    before=p.inspect(pid)
    p.inspect(pid,dict(settings,restart=before['settings']['restart']+1))
    end=time.monotonic()+10
    while time.monotonic()<end:
        now=p.inspect(pid)
        if now['status']['error']:raise RuntimeError(now)
        if now['status']['starts']>before['status']['starts']:return now['status']['starts']
        time.sleep(.05)
    raise RuntimeError('Practice retry timed out')
rows=[]
for stage,section in enumerate([1,7,13,22,31,39,50],1):
    restart(stage=stage,section=section,difficulty=4 if stage==7 else 2,flags=1,fps=240)
    time.sleep(1.5)
    row=dict(status=p.inspect(pid),memory=p.memory(pid))
    assert row['memory']['practice']==1 and row['memory']['spellPractice']==0 and row['status']['status']['stage']==stage
    rows.append(row);print('script guard',stage,'PASS',flush=True)
before=set((game/'replay').glob('th6_??.rpy'))
number=restart(stage=1,section=4,difficulty=1,shot=0,flags=65,fps=60,score=1234560,graze=345,point=123,lives=4,bombs=3,power=96,rank=12)
time.sleep(1)
resources=p.inspect(pid)['status']
assert all(resources[k]==v for k,v in dict(lives=4,bombs=3,power=96,rank=12).items()),resources
assert resources['score']>=1234560
p.inspect(pid,dict(flags=81));time.sleep(.5)
locked1=p.memory(pid);time.sleep(1);locked2=p.memory(pid)
assert locked1['enemies'][0]['age']==locked2['enemies'][0]['age']
p.inspect(pid,dict(flags=65));time.sleep(2)
saved=p.inspect(pid);p.inspect(pid,dict(saveReplay=saved['settings']['saveReplay']+1));time.sleep(.5)
new=(set((game/'replay').glob('th6_??.rpy'))-before).pop()
assert new.with_name(new.name+'.thprac-nc').exists()
report=dict(guards=rows,resources=resources,lock=[locked1,locked2],recordNumber=number,replay=new.name)
Path('build/nc-features.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('resources, rank, time lock and replay export PASS',new.name,number,flush=True)
p.key(pid,27);time.sleep(.4);p.key(pid,81);time.sleep(2)
print('exit FPS',p.memory(pid)['fps'])
