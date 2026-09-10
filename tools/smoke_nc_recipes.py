"""Exercise recipes only after a user-visible Practice Start session is active.

Run against a disposable local game copy. The shared diagnostic channel requests
the same native retry path as Esc+R; it never writes into the game's code.
"""
import argparse
import json
import re
import time
from pathlib import Path
import probe_nc as probe

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('pid',type=int)
    parser.add_argument('--start',type=int,default=1)
    parser.add_argument('--limit',type=int,default=70)
    parser.add_argument('--output',type=Path,default=Path('build/nc-recipe-smoke.json'))
    parser.add_argument('--portions',action='store_true')
    args=parser.parse_args()
    core=Path(__file__).resolve().parents[1]/'thprac/src/thprac'
    header=(core/'thprac_locale_def.h').read_text(encoding='utf-8-sig').split('namespace TH06 {')[1].split('};')[0]
    ids={name:i for i,name in enumerate(re.findall(r'^\s*(A0000ERROR|TH06_\w+)\s*,',header,re.M))}
    names=re.findall(r'case THPrac::TH06::(TH06_\w+):',(core/'th06nc/practice_patches.inl').read_text())
    if args.portions:
        names=[f'TH06_ST{stage}_PORTION{portion}'for stage,count in enumerate([6,4,7,9,5,2,7],1)for portion in range(1,count+1)]
        ids={name:10000+int(re.search(r'ST(\d)',name)[1])*100+int(re.search(r'PORTION(\d+)',name)[1])for name in names}
    state=probe.inspect(args.pid)
    assert state['status']['starts']>0 and probe.memory(args.pid)['practice']==1, 'Enter custom Practice Start first'
    rows=[]
    for name in names[args.start-1:args.start-1+args.limit]:
        stage=int(re.search(r'ST(\d)',name)[1]);before=probe.inspect(args.pid)
        patch=dict(stage=stage,difficulty=4 if stage==7 else 2,shot=0,section=ids[name],frame=0,
                   spell=-1,flags=1,fps=240,lives=8,bombs=8,power=128,rank=32,score=0,graze=0,point=0,
                   dialogue=0,fakeShot=-1,phase=0,restart=before['settings']['restart']+1)
        probe.inspect(args.pid,patch)
        deadline=time.monotonic()+12
        while time.monotonic()<deadline:
            current=probe.inspect(args.pid)
            if current['status']['starts']>before['status']['starts']:break
            time.sleep(.05)
        else: raise RuntimeError(f'Retry did not start: {name}')
        seenEnemies=False
        if args.portions:
            deadline=time.monotonic()+3
            while time.monotonic()<deadline:
                seenEnemies=seenEnemies or bool(probe.memory(args.pid)['enemies'])
                time.sleep(.1)
        else:time.sleep(2.5)
        status=probe.inspect(args.pid);memory=probe.memory(args.pid)
        row=dict(name=name,status=status,memory=memory,seenEnemies=seenEnemies or bool(memory['enemies']))
        row['ok']=status['status']['error']==0 and status['status']['stage']==stage and memory['practice']==1 and memory['spellPractice']==0 and memory['state']==2 and row['seenEnemies']
        if not args.portions:
            # These are dialogue/entry subs, never the selected attacks.
            forbidden={5:{12,23},6:{8,9},7:{16}}
            if memory['enemies'] and all(e['sub'] in forbidden.get(stage,set()) for e in memory['enemies']):row['ok']=False
        rows.append(row);args.output.parent.mkdir(exist_ok=True,parents=True)
        args.output.write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf-8')
        print(name,'PASS' if row['ok'] else 'FAIL',[(e['sub'],e['hp'])for e in memory['enemies']],flush=True)
        if not row['ok']:raise RuntimeError(f'Recipe failed: {name}')

if __name__=='__main__':main()
