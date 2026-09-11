// Compile as a Win32 console EXE, then embed the normal payload with package_nc_single.py.
#include "thprac_native_bundle.h"
#include <cstdio>

int wmain() {
    using namespace THPrac::NativeBundle;
    std::wstring helper;
    if (!Present() || !Prepare(helper)) return 1;
    WIN32_FILE_ATTRIBUTE_DATA before{}, after{};
    if (!GetFileAttributesExW(helper.c_str(), GetFileExInfoStandard, &before)) return 2;
    if (!Prepare(helper) || !GetFileAttributesExW(helper.c_str(), GetFileExInfoStandard, &after)) return 3;
    if (CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime)) return 4;
    auto resource = Read(BundleId + 1);
    HANDLE file = CreateFileW(helper.c_str(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 5;
    DWORD written;
    WriteFile(file, "damaged test cache", 18, &written, nullptr);
    CloseHandle(file);
    if (Matches(helper, resource)) return 6;
    if (!Prepare(helper) || !Matches(helper, resource)) return 7;
    // A cached DLL held open for execution must be reused, not overwritten.
    auto dll = helper.substr(0, helper.find_last_of(L'\\') + 1) + Files[1];
    file = CreateFileW(dll.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return 8;
    bool reused = Prepare(helper);
    CloseHandle(file);
    if (!reused) return 9;
    wprintf(L"PASS: extraction, unchanged reuse, corrupt cache repair, locked DLL reuse\n%ls\n", helper.c_str());
    return 0;
}
