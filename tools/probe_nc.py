"""Local runtime inspection for the practice adapter. Does not scan unrelated processes."""
import ctypes as c, sys, json, mmap, time
from pathlib import Path
from ctypes import wintypes as w
k=c.WinDLL('kernel32',use_last_error=True)
u=c.WinDLL('user32',use_last_error=True)
k.OpenProcess.argtypes=[w.DWORD,w.BOOL,w.DWORD];k.OpenProcess.restype=w.HANDLE
k.ReadProcessMemory.argtypes=[w.HANDLE,c.c_void_p,c.c_void_p,c.c_size_t,c.POINTER(c.c_size_t)]
k.CloseHandle.argtypes=[w.HANDLE]
class Settings(c.Structure):
    _fields_=[(n,c.c_int32) for n in ['enabled','stage','difficulty','shot','spell','frame','lives','bombs','power','graze','point','rank']]+[('score',c.c_uint64),('flags',c.c_uint32),('restart',c.c_uint32)]+[(n,c.c_int32)for n in ['fakeShot','section','phase','dialogue','fps']]+[('reserved',c.c_uint32)]
class Status(c.Structure):
    _fields_=[(n,c.c_int32) for n in ['ready','error','stage','difficulty','frame','lives','bombs','power','rank']]+[('starts',c.c_uint32),('score',c.c_uint64),('flags',c.c_uint32),('exports',c.c_uint32),('misses',c.c_int32),('bombsUsed',c.c_int32),('message',c.c_wchar*160)]
class Shared(c.Structure):
    _fields_=[('magic',c.c_uint32),('version',c.c_uint32),('settings',Settings),('status',Status)]
def connection(pid):
    mm=mmap.mmap(-1,c.sizeof(Shared),tagname=f'Local\\Th06NcPractice_{pid}')
    data=Shared.from_buffer(mm)
    if data.magic!=0x4e435031 or data.version!=7:raise RuntimeError('No compatible practice connection')
    return mm,data
def inspect(pid,patch=None):
    mm,data=connection(pid)
    k.OpenMutexW.argtypes=[w.DWORD,w.BOOL,w.LPCWSTR];k.OpenMutexW.restype=w.HANDLE
    k.WaitForSingleObject.argtypes=[w.HANDLE,w.DWORD];k.ReleaseMutex.argtypes=[w.HANDLE]
    lock=k.OpenMutexW(0x100001,False,f'Local\\Th06NcPractice_{pid}_lock')
    if k.WaitForSingleObject(lock,1000) not in (0,0x80):raise RuntimeError('Mutex timeout')
    try:
        if patch:
            for key,value in patch.items():setattr(data.settings,key,value)
        result={part:{n:getattr(getattr(data,part),n)for n,_ in getattr(data,part)._fields_}for part in ['settings','status']}
    finally:k.ReleaseMutex(lock);k.CloseHandle(lock)
    return result
def memory(pid):
    process=k.OpenProcess(0x410,False,pid)
    if not process:raise c.WinError(c.get_last_error())
    k.K32EnumProcessModules.argtypes=[w.HANDLE,c.POINTER(c.c_void_p),w.DWORD,c.POINTER(w.DWORD)]
    modules=(c.c_void_p*1024)();needed=w.DWORD()
    if not k.K32EnumProcessModules(process,modules,c.sizeof(modules),c.byref(needed)):raise c.WinError(c.get_last_error())
    base=modules[0]
    def read(offset,fmt):
        import struct
        buffer=c.create_string_buffer(struct.calcsize(fmt));n=c.c_size_t()
        if not k.ReadProcessMemory(process,base+offset,buffer,len(buffer),c.byref(n)):raise c.WinError(c.get_last_error())
        return struct.unpack(fmt,buffer.raw)[0]
    try:
        enemies=[{'index':i,'hp':read(0xaa1e98+i*0x10b0+0x234,'<i'),'sub':read(0xaa1e98+i*0x10b0+0x80,'<h'),
                  'age':read(0xaa1e98+i*0x10b0+4,'<i'),'boss':bool(read(0xaa1e98+i*0x10b0+0xbd,'<B')&8),
                  'var5':read(0xaa1e98+i*0x10b0+0x74,'<i'),'float3':read(0xaa1e98+i*0x10b0+0x64,'<f'),
                  'lifeCallback':read(0xaa1e98+i*0x10b0+0xa4,'<i')}
                 for i in range(256) if read(0xaa1e98+i*0x10b0+0xbc,'<B')&0x80]
        return {'state':read(0xc21d9c,'<i'),'practice':read(0x4f27b4,'<B'),'spellPractice':read(0x4f27b5,'<B'),'enemies':enemies,'playerState':read(0x506c38,'<B'),'bombActive':read(0x509268,'<B'),
                'bgmPath':read(0xc21c0c,'256s').split(b'\0')[0].decode('ascii',errors='replace'),
                'graze':read(0x4ff0cc,'<i'),'point':read(0x4f27ba,'<H'),'timer':read(0xa6ec1c,'<i'),
                'misses':read(0x4f1e60,'<i'),'bombsUsed':read(0x4f1e64,'<i'),'fps':read(0xc22108,'<d'),
                'replay':read(0x4f278c,'<B'),'bgmHandle':read(0x50966c,'<i'),'deathWindow':read(0x506adc,'<i'),
                'dialogue':read(read(0xa6ec08,'<Q')-base+14000,'<i') if read(0xa6ec08,'<Q') else -1,'shot':read(0x4f1e80,'<B')*2+read(0x4f1e81,'<B'),
                'invulnerabilityTimer':read(0x506bf8,'<i'), 'gameOver':read(0x4f27b1,'<B'),
                'stageTitleScript':read(read(0xa6ec08,'<Q')-base+0xa58,'<Q') if read(0xa6ec08,'<Q') else 0,
                'stageTitleVisible':read(read(0xa6ec08,'<Q')-base+0xa24,'<I')&1 if read(0xa6ec08,'<Q') else 0}
    finally:k.CloseHandle(process)
def windows(pid):
    result=[]
    @c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
    def visit(hwnd,_):
        owner=w.DWORD();u.GetWindowThreadProcessId(hwnd,c.byref(owner))
        if owner.value==pid:
            title=c.create_unicode_buffer(512);u.GetWindowTextW(hwnd,title,512)
            result.append((hwnd,title.value))
        return True
    u.EnumWindows(visit,0);return result
def key(pid,vk):
    wins=windows(pid)
    hwnd=next(h for h,title in wins if 'Embodiment' in title)
    u.ShowWindow(hwnd,9);u.SetForegroundWindow(hwnd)
    scan=u.MapVirtualKeyW(vk,0)
    extended=1 if vk in [0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2d,0x2e] else 0
    u.keybd_event(vk,scan,extended,0);time.sleep(.08);u.keybd_event(vk,scan,extended|2,0);time.sleep(.18)
if __name__=='__main__':
    pid=int(sys.argv[1]);action=sys.argv[2] if len(sys.argv)>2 else 'status'
    if action=='key':
        for arg in sys.argv[3:]:key(pid,int(arg,0))
    elif action=='windows':print(windows(pid))
    elif action=='memory':print(json.dumps(memory(pid),ensure_ascii=False))
    elif action=='dialogs':
        @c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
        def child(hwnd,_):
            text=c.create_unicode_buffer(2048);u.GetWindowTextW(hwnd,text,2048);print(text.value);return True
        for hwnd,title in windows(pid):
            print(title);u.EnumChildWindows(hwnd,child,0)
    elif action=='shot':
        from PIL import ImageGrab
        hwnd=next(h for h,title in windows(pid) if 'Embodiment' in title)
        ImageGrab.grab(window=hwnd).save(Path(__file__).resolve().parents[1]/'test-screen.png')
    else:print(json.dumps(inspect(pid,json.loads(sys.argv[3]) if action=='set' else None),ensure_ascii=False,indent=2))
