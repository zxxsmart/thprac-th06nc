"""Embed the native NC payload into the existing Win32 launcher (Windows only)."""
import argparse
import ctypes as c
import hashlib
import shutil
import tempfile
from pathlib import Path
from ctypes import wintypes as w

FILES = [
    'thprac_bridge64.exe', 'thprac_th06nc.dll', 'freetype.dll',
    'README_NC.md', 'VALIDATION_NC.md', 'LICENCE',
    'licenses/FreeType-LICENSE.txt', 'licenses/FreeType-FTL.txt',
    'licenses/MinHook.txt', 'licenses/ImGui.txt',
]

def package(launcher, payload_dir, output):
    files = [(name, (payload_dir / name).read_bytes()) for name in FILES]
    digest = hashlib.sha256()
    for name, data in files:
        digest.update(name.encode('utf-8') + b'\0' + hashlib.sha256(data).digest())
    bundle_id = digest.hexdigest()
    kernel = c.WinDLL('kernel32', use_last_error=True)
    kernel.BeginUpdateResourceW.argtypes = [w.LPCWSTR, w.BOOL]
    kernel.BeginUpdateResourceW.restype = w.HANDLE
    kernel.UpdateResourceW.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, w.WORD, c.c_void_p, w.DWORD]
    kernel.EndUpdateResourceW.argtypes = [w.HANDLE, w.BOOL]
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=output.parent, suffix='.exe', delete=False) as tmp:
        temporary = Path(tmp.name)
    update = None
    try:
        shutil.copyfile(launcher, temporary)
        update = kernel.BeginUpdateResourceW(str(temporary.resolve()), False)
        if not update:
            raise c.WinError(c.get_last_error())
        for resource_id, data in enumerate([bundle_id.encode('ascii')] + [data for _, data in files], 100):
            buffer = c.create_string_buffer(data)
            if not kernel.UpdateResourceW(update, 10, resource_id, 0, buffer, len(data)):
                raise c.WinError(c.get_last_error())
        handle, update = update, None
        if not kernel.EndUpdateResourceW(handle, False):
            raise c.WinError(c.get_last_error())
        temporary.replace(output)
    finally:
        if update:
            kernel.EndUpdateResourceW(update, True)
        temporary.unlink(missing_ok=True)
    print(f'Single EXE: {output} ({output.stat().st_size:,} bytes), bundle {bundle_id}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--launcher', type=Path, required=True)
    parser.add_argument('--payload-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    package(args.launcher, args.payload_dir, args.output)
