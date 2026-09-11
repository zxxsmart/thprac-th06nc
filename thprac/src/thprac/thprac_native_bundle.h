#pragma once
#include <Windows.h>
#include <shlobj.h>
#include <algorithm>
#include <cstring>
#include <string>

namespace THPrac::NativeBundle {
// Resource IDs and relative names also appear in tools/package_nc_single.py.
inline constexpr int BundleId = 100;
inline constexpr const wchar_t* Files[] = {
    L"thprac_bridge64.exe", L"thprac_th06nc.dll", L"freetype.dll",
    L"README_NC.md", L"LICENCE",
    L"licenses\\FreeType-LICENSE.txt", L"licenses\\FreeType-FTL.txt",
    L"licenses\\MinHook.txt", L"licenses\\ImGui.txt", L"THIRD_PARTY_NOTICES.txt"
};
struct Resource {
    const unsigned char* data = nullptr;
    DWORD size = 0;
};
inline Resource Read(int id) {
    auto module = GetModuleHandleW(nullptr);
    auto resource = FindResourceW(module, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource) return {};
    return {static_cast<const unsigned char*>(LockResource(LoadResource(module, resource))),
        SizeofResource(module, resource)};
}
inline bool Present() {
    auto id = Read(BundleId);
    if (!id.data || id.size != 64) return false;
    for (DWORD i = 0; i < id.size; ++i)
        if (!((id.data[i] >= '0' && id.data[i] <= '9') || (id.data[i] >= 'a' && id.data[i] <= 'f'))) return false;
    return true;
}
inline bool Matches(const std::wstring& path, Resource resource) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart == resource.size;
    unsigned char buffer[65536];
    for (DWORD offset = 0; ok && offset < resource.size;) {
        DWORD count = (std::min)(DWORD(sizeof(buffer)), resource.size - offset), got = 0;
        ok = ReadFile(file, buffer, count, &got, nullptr) && got == count &&
            std::memcmp(buffer, resource.data + offset, count) == 0;
        offset += count;
    }
    CloseHandle(file);
    return ok;
}
inline bool Directory(const std::wstring& path) {
    if (CreateDirectoryW(path.c_str(), nullptr)) return true;
    auto attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY);
}
inline bool Extract(const std::wstring& folder, const std::wstring& path, Resource resource) {
    if (!resource.data || !resource.size) return false;
    if (Matches(path, resource)) return true;
    // Publish complete files atomically. Existing matching DLLs remain untouched
    // while games use them; simultaneous launchers can reuse the winner's file.
    wchar_t temp[MAX_PATH]{};
    if (!GetTempFileNameW(folder.c_str(), L"tnc", 0, temp)) return false;
    HANDLE file = CreateFileW(temp, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
    DWORD written = 0;
    bool ok = file != INVALID_HANDLE_VALUE;
    if (ok) {
        ok = WriteFile(file, resource.data, resource.size, &written, nullptr) && written == resource.size;
        if (ok) ok = FlushFileBuffers(file) != 0;
        CloseHandle(file);
    }
    if (ok) ok = MoveFileExW(temp, path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    DeleteFileW(temp);
    return ok || Matches(path, resource);
}
inline bool Prepare(std::wstring& helper) {
    auto id = Read(BundleId);
    if (!id.data || id.size != 64) return false;
    std::wstring version;
    for (DWORD i = 0; i < id.size; ++i) {
        char c = char(id.data[i]);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        version += wchar_t(c);
    }
    wchar_t local[MAX_PATH]{};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, local))) return false;
    std::wstring folder(local);
    for (const auto& component : {std::wstring(L"thprac"), std::wstring(L"th06nc"), version}) {
        folder += L"\\" + component;
        if (!Directory(folder)) return false;
    }
    if (!Directory(folder + L"\\licenses")) return false;
    for (int i = 0; i < int(sizeof(Files) / sizeof(Files[0])); ++i)
        if (!Extract(folder, folder + L"\\" + Files[i], Read(BundleId + 1 + i))) return false;
    helper = folder + L"\\" + Files[0];
    return true;
}
}
