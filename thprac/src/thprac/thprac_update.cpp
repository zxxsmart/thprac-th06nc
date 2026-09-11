#include "thprac_update.h"
#include "thprac_fork.h"
#include "thprac_gui_locale.h"
#include "thprac_utils.h"
#include <shellapi.h>
#include <string>
namespace THPrac {
void OpenReleasePage(HWND window) {
    if (reinterpret_cast<INT_PTR>(ShellExecuteA(window, "open", THPRAC_FORK_RELEASES,
        nullptr, nullptr, SW_SHOWNORMAL)) <= 32) {
        std::string message = std::string(S(TH06NC_OPEN_LINK_ERROR)) + "\n" THPRAC_FORK_RELEASES;
        MessageBoxW(window, utf8_to_utf16(message.c_str()).c_str(), L"thprac-th06nc", MB_ICONERROR);
    }
}
}
