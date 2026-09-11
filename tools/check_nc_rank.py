"""Rank regression for a game already in custom practice. Restarts that session."""
import json
import sys
import time
from pathlib import Path
import probe_nc as p

pid = int(sys.argv[1])
rows = []

def restart(**options):
    before = p.inspect(pid)
    settings = dict(enabled=1, stage=1, section=3, difficulty=0, flags=1,
                    rank=99, fps=60, restart=before['settings']['restart'] + 1)
    settings.update(options)
    p.inspect(pid, settings)
    until = time.monotonic() + 10
    while time.monotonic() < until:
        now = p.inspect(pid)
        assert not now['status']['error'], now
        if now['status']['starts'] > before['status']['starts']:
            time.sleep(.3)
            return
        time.sleep(.02)
    raise RuntimeError('Practice restart timed out')

def check(name, expected):
    state = p.inspect(pid)
    assert state['status']['rank'] == expected, state
    rows.append(dict(name=name, expected=expected, state=state))
    print(name, expected, 'PASS', flush=True)

for difficulty, expected in enumerate([20, 32, 32, 32, 18]):
    restart(difficulty=difficulty, stage=7 if difficulty==4 else 1,
            section=71 if difficulty==4 else 3)
    check('native-difficulty-' + str(difficulty), expected)

for value in [0, 12, 99]:
    restart(flags=65, rank=value)
    check('custom-' + str(value), value)
    time.sleep(.5)
    check('custom-persists-' + str(value), value)

p.inspect(pid, dict(rank=5, flags=1))
time.sleep(.2)
check('pending-edits-do-not-change-current-run', 99)
restart(flags=1, rank=5)
check('disabling-custom-restores-native-on-retry', 20)
restart(flags=65, rank=12)
check('custom-applies-on-retry', 12)
Path('build/nc-rank.json').write_text(json.dumps(rows, ensure_ascii=False, indent=2), encoding='utf-8')
p.key(pid, 27)
