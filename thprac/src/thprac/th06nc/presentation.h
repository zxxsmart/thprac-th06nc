#pragma once
#include <dxgi.h>
#include <cstdint>

namespace THPrac::TH06NC {
using PresentCallback = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
// Display state is separate from Settings and is never written into replays.
enum class DisplayState : int { Off, Pending, Active, Unavailable };
bool InstallLowLatency(void* resizeBuffersEntry);
void LowLatencyBeforeFrame();
HRESULT LowLatencyPresent(IDXGISwapChain* swap, UINT interval, UINT flags, PresentCallback original);
DisplayState LowLatencyState();
uint32_t LowLatencyError();
}
