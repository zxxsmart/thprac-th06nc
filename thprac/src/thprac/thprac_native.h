#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <string>
#include "thprac_native_bundle.h"
#include "thprac_gui_locale.h"
namespace THPrac {
// Cross-bitness is handled by a companion built from this same source tree.
inline bool LaunchTH06NC(bool practice, DWORD attachPid = 0) {
    if (!practice && !attachPid)
        return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", L"steam://rungameid/4659620", nullptr, nullptr, SW_SHOWNORMAL)) > 32;
    std::wstring helper;
    if (NativeBundle::Present()) {
        if (!NativeBundle::Prepare(helper)) {
            MessageBoxW(nullptr, L"无法准备新典练习模块。请检查用户缓存目录的写入权限及安全软件的拦截记录。", L"thprac", MB_ICONERROR);
            return false;
        }
    } else {
        wchar_t self[32768]{};
        if (!GetModuleFileNameW(nullptr, self, 32768)) return false;
        helper = self;
        helper.resize(helper.find_last_of(L"\\/")+1);
        helper += L"thprac_bridge64.exe";
    }
    std::wstring command = L"\"" + helper + L"\" " +
        (attachPid ? L"--attach " + std::to_wstring(attachPid) : L"--steam") +
        L" --language " + std::to_wstring(Gui::LocaleGet());
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
    bool success = true;
    if (attachPid) {
        DWORD exitCode = 1;
        success = WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0 &&
            GetExitCodeProcess(process.hProcess, &exitCode) && exitCode == 0;
    }
    CloseHandle(process.hProcess);
    return success;
}
}
