"""Keyboard smoke entry for the disposable NC copy with its default menu config."""
import sys,time
import probe_nc as p
pid=int(sys.argv[1])
hwnd=next(h for h,title in p.windows(pid) if 'Embodiment' in title)
p.u.ShowWindow(hwnd,9);p.u.SetForegroundWindow(hwnd);time.sleep(1)
p.key(pid,90);time.sleep(2)
for key in [40,40]+[90]*6:
    p.key(pid,key);time.sleep(1)
for key in [40,39,39,39]:
    p.key(pid,key);time.sleep(.2)
p.key(pid,90);time.sleep(3)
p.key(pid,112)
assert p.inspect(pid)['status']['starts']>0, 'The native menu did not enter custom practice; inspect its current screen.'
print(p.inspect(pid));print(p.memory(pid))
