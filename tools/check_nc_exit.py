"""Native result-menu checks in the disposable game copy only."""
import ctypes as c
import json
import struct
import sys
import time
from pathlib import Path
from PIL import ImageGrab
import probe_nc as p

pid = int(sys.argv[1])

def read_result():
    process = p.k.OpenProcess(0x410, False, pid)
    modules = (c.c_void_p * 1024)()
    needed = p.w.DWORD()
    p.k.K32EnumProcessModules.argtypes = [p.w.HANDLE, c.POINTER(c.c_void_p), p.w.DWORD, c.POINTER(p.w.DWORD)]
    assert p.k.K32EnumProcessModules(process, modules, c.sizeof(modules), c.byref(needed))
    base = modules[0]
    def read(address, fmt):
        buf = c.create_string_buffer(struct.calcsize(fmt))
        assert p.k.ReadProcessMemory(process, address, buf, len(buf), None)
        return struct.unpack(fmt, buf.raw)[0]
    try:
        node = base + 0x4f1440
        for _ in range(512):
            if not node:
                return None
            if read(node + 8, '<Q') == base + 0x6e3a0:
                obj = read(node + 0x38, '<Q')
                return dict(address=obj, state=read(obj + 0x9e94, '<i'),
                            frame=read(obj + 0x4450, '<i'), selection=read(obj + 0x9e7c, '<i'),
                            slot=read(obj + 0x4438, '<i'), name=read(obj + 0x9e88, '9s').decode('ascii'))
            node = read(node + 0x28, '<Q')
    finally:
        p.k.CloseHandle(process)

def screenshot(name):
    hwnd = next(h for h, title in p.windows(pid) if 'Embodiment' in title)
    rect = p.w.RECT()
    p.u.GetWindowRect(hwnd, c.byref(rect))
    ImageGrab.grab((rect.left, rect.top, rect.right, rect.bottom)).resize((1280, 720)).save(name)

if __name__ == '__main__':
    for key in sys.argv[2:]:
        p.key(pid, int(key)); time.sleep(.5)
    time.sleep(2)
    print(json.dumps(dict(result=read_result(), status=p.inspect(pid), memory=p.memory(pid)), ensure_ascii=False))
    screenshot('nc-exit-current.jpg')
