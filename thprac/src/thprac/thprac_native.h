#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <string>
namespace THPrac {
// Cross-bitness is handled by a companion built from this same source tree.
inline bool LaunchTH06NC(bool practice, DWORD attachPid = 0) {
    if (!practice && !attachPid)
        return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"steam://rungameid/4659620", nullptr, nullptr, SW_SHOWNORMAL)) > 32;
    wchar_t self[32768]{};
    if (!GetModuleFileNameW(nullptr, self, 32768)) return false;
    std::wstring helper(self);
    helper.resize(helper.find_last_of(L"\\/")+1);
    helper += L"thprac_bridge64.exe";
    std::wstring command = L"\"" + helper + L"\" " +
        (attachPid ? L"--attach " + std::to_wstring(attachPid) : L"--steam");
    STARTUPINFOW startup{sizeof(startup)};
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(helper.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &startup, &process)) {
        MessageBoxW(nullptr, L"无法启动 thprac_bridge64.exe。请将它和 thprac_th06nc.dll 放在 thprac.exe 同一目录。", L"thprac", MB_ICONERROR);
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}
}
