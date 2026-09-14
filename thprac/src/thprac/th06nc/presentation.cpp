#include "presentation.h"
#include "module.h"
#include "MinHook.h"
#include <d3d11.h>
#include <dxgi1_5.h>
#include <wrl/client.h>
#include <atomic>

namespace THPrac::TH06NC {
namespace {
static_assert(sizeof(void*) == 8 && sizeof(DXGI_SWAP_CHAIN_DESC) == 72 && sizeof(D3D11_TEXTURE2D_DESC) == 44);
using Microsoft::WRL::ComPtr;
using ResizeCallback = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
ResizeCallback originalResize{};
bool enabled{}, submitted{};
std::atomic<DisplayState> state{DisplayState::Off};
std::atomic<HRESULT> lastError{S_OK};
IDXGISwapChain* observed{}; // Borrowed: the game owns the chain.
ComPtr<ID3D11DeviceContext> context;
HANDLE frameAvailable{};

template<class T> T& data(uintptr_t rva) { return *reinterpret_cast<T*>(imageBase + rva); }
void closeWaitHandle()
{
    if (frameAvailable) CloseHandle(frameAvailable);
    frameAvailable = nullptr;
    submitted = false;
}
void unavailable(HRESULT error)
{
    closeWaitHandle();
    context.Reset();
    lastError = error;
    state = DisplayState::Unavailable;
}

HRESULT STDMETHODCALLTYPE resizeBuffers(IDXGISwapChain* swap, UINT count, UINT width, UINT height,
    DXGI_FORMAT format, UINT flags)
{
    // NC normally passes flags=0. DXGI requires the tearing and waitable-object
    // creation flags to survive ResizeBuffers. Do not alter other swap chains.
    if (enabled && swap == data<IDXGISwapChain*>(Rva::SwapChain)) {
        DXGI_SWAP_CHAIN_DESC desc{};
        if (SUCCEEDED(swap->GetDesc(&desc)))
            flags |= desc.Flags & (DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
    }
    return originalResize(swap, count, width, height, format, flags);
}

HRESULT convert(IDXGISwapChain* oldSwap, const DXGI_SWAP_CHAIN_DESC& oldDesc)
{
    // The tested NC renderer draws into an offscreen target, then into this
    // window backbuffer. Exclusive fullscreen and multisampled swap chains
    // retain the native path; changing their display mode would be intrusive.
    if (!oldDesc.Windowed || oldDesc.SampleDesc.Count != 1 ||
        oldDesc.SwapEffect != DXGI_SWAP_EFFECT_DISCARD ||
        oldDesc.BufferDesc.Format != DXGI_FORMAT_R8G8B8A8_UNORM ||
        data<int>(Rva::AlternatePresent1) || data<int>(Rva::AlternatePresent2))
        return DXGI_ERROR_UNSUPPORTED;

    ComPtr<IDXGIFactory2> factory;
    ComPtr<IDXGIFactory5> features;
    ComPtr<ID3D11Device> device;
    HRESULT hr = oldSwap->GetParent(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) return hr;
    hr = factory.As(&features);
    if (FAILED(hr)) return hr;
    BOOL tearing = FALSE;
    hr = features->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing));
    if (FAILED(hr) || !tearing) return FAILED(hr) ? hr : DXGI_ERROR_UNSUPPORTED;
    hr = oldSwap->GetDevice(IID_PPV_ARGS(&device));
    if (FAILED(hr)) return hr;

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = oldDesc.BufferDesc.Width;
    desc.Height = oldDesc.BufferDesc.Height;
    desc.Format = oldDesc.BufferDesc.Format;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = oldDesc.BufferUsage;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    ComPtr<IDXGISwapChain1> replacement;
    // Prepare all objects before changing any game-owned pointer. A failure
    // here leaves the original chain, backbuffer and rendering state intact.
    hr = factory->CreateSwapChainForHwnd(device.Get(), oldDesc.OutputWindow, &desc, nullptr, nullptr, &replacement);
    if (FAILED(hr)) return hr;
    ComPtr<IDXGISwapChain2> queue;
    hr = replacement.As(&queue);
    if (FAILED(hr)) return hr;
    hr = queue->SetMaximumFrameLatency(1);
    if (FAILED(hr)) return hr;
    ComPtr<ID3D11Texture2D> buffer;
    ComPtr<ID3D11ShaderResourceView> source;
    ComPtr<ID3D11RenderTargetView> target;
    ComPtr<IDXGIOutput> output;
    ComPtr<IDXGIOutput1> output1;
    DXGI_SWAP_CHAIN_DESC actual{};
    if (FAILED(hr = replacement->GetBuffer(0, IID_PPV_ARGS(&buffer))) ||
        FAILED(hr = device->CreateShaderResourceView(buffer.Get(), nullptr, &source)) ||
        FAILED(hr = device->CreateRenderTargetView(buffer.Get(), nullptr, &target)) ||
        FAILED(hr = replacement->GetContainingOutput(&output)) ||
        FAILED(hr = output.As(&output1)) ||
        FAILED(hr = replacement->GetDesc(&actual))) return hr;
    HANDLE available = queue->GetFrameLatencyWaitableObject();
    if (!available) return E_FAIL;

    // Mirror NC's native ResizeBackbuffer (0x253a60): flush/unbind its cached
    // render target slots, release the old backbuffer views, then bind the new
    // views through the game's own cache-aware setter. No ClearState, which
    // would desynchronize the rest of the game's D3D11 state cache.
    auto setTarget = reinterpret_cast<int64_t(*)(void*, void*, unsigned)>(imageBase + Rva::SetRenderTarget);
    auto releaseBuffers = reinterpret_cast<int64_t(*)(unsigned)>(imageBase + Rva::ReleaseBackbuffer);
    auto oldBuffer = data<ID3D11Texture2D*>(Rva::Backbuffer);
    bool bound[8]{};
    for (unsigned i = 0; i < 8; ++i) {
        bound[i] = data<ID3D11Texture2D*>(Rva::CachedRenderTargets + i * sizeof(void*)) == oldBuffer;
        if (bound[i]) setTarget(nullptr, nullptr, i);
    }
    releaseBuffers(0);
    data<IDXGISwapChain*>(Rva::SwapChain) = replacement.Detach();
    data<DXGI_SWAP_CHAIN_DESC>(Rva::SwapChainDesc) = actual;
    data<ID3D11Texture2D*>(Rva::Backbuffer) = buffer.Detach();
    data<ID3D11Texture2D*>(Rva::Backbuffer)->GetDesc(&data<D3D11_TEXTURE2D_DESC>(Rva::BackbufferDesc));
    data<ID3D11ShaderResourceView*>(Rva::BackbufferSource) = source.Detach();
    data<ID3D11RenderTargetView*>(Rva::BackbufferTarget) = target.Detach();
    if (auto old = data<IDXGIOutput*>(Rva::Output)) old->Release();
    if (auto old = data<IDXGIOutput1*>(Rva::Output1)) old->Release();
    data<IDXGIOutput*>(Rva::Output) = output.Detach();
    data<IDXGIOutput1*>(Rva::Output1) = output1.Detach();
    for (unsigned i = 0; i < 8; ++i)
        if (bound[i]) setTarget(data<void*>(Rva::Backbuffer), data<void*>(Rva::BackbufferTarget), i);
    oldSwap->Release();

    closeWaitHandle();
    frameAvailable = available;
    device->GetImmediateContext(context.ReleaseAndGetAddressOf());
    observed = data<IDXGISwapChain*>(Rva::SwapChain);
    submitted = true;
    lastError = S_OK;
    state = DisplayState::Active;
    return S_OK;
}
}

bool InstallLowLatency(void* resizeBuffersEntry)
{
    if (MH_CreateHook(resizeBuffersEntry, reinterpret_cast<void*>(resizeBuffers),
        reinterpret_cast<void**>(&originalResize)) != MH_OK) return false;
    enabled = true;
    state = DisplayState::Pending;
    return true;
}

void LowLatencyBeforeFrame()
{
    if (!enabled) return;
    auto swap = data<IDXGISwapChain*>(Rva::SwapChain);
    if (!swap || !data<ID3D11Texture2D*>(Rva::Backbuffer)) return;
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swap->GetDesc(&desc))) return;
    // Graphics options can destroy/recreate a chain (even at the same address).
    if (swap != observed || (state == DisplayState::Active && desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD)) {
        closeWaitHandle();
        context.Reset();
        observed = swap;
        if (IsIconic(desc.OutputWindow) || !desc.BufferDesc.Width || !desc.BufferDesc.Height) {
            observed = nullptr;
            return;
        }
        HRESULT hr = convert(swap, desc);
        if (FAILED(hr)) unavailable(hr);
    }
    if (state == DisplayState::Active && submitted && frameAvailable) {
        submitted = false;
        // Once per rendered frame, BEFORE the calc chain samples input. Replay
        // fast-forward may run this chain several times without a Present.
        // Wake for window messages and bound the wait to keep Alt-Tab/minimize
        // responsive; the game's normal message pump still dispatches them.
        if (!IsIconic(desc.OutputWindow) && GetForegroundWindow() == desc.OutputWindow)
            MsgWaitForMultipleObjectsEx(1, &frameAvailable, 100, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }
}

HRESULT LowLatencyPresent(IDXGISwapChain* swap, UINT interval, UINT flags, PresentCallback original)
{
    if (state != DisplayState::Active || swap != observed || !context || (flags & DXGI_PRESENT_TEST))
        return original(swap, interval, flags);
    BOOL fullscreen = FALSE;
    if (FAILED(swap->GetFullscreenState(&fullscreen, nullptr))) return original(swap, interval, flags);
    if (!fullscreen) flags |= DXGI_PRESENT_ALLOW_TEARING;
    else flags &= ~DXGI_PRESENT_ALLOW_TEARING;

    // Flip Present unbinds the backbuffer RTV. Restore the actual bindings so
    // NC's cached setters remain valid on the next frame, including its UI.
    ID3D11RenderTargetView* targets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
    ID3D11DepthStencilView* depth{};
    context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, &depth);
    HRESULT result = original(swap, 0, flags);
    context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, targets, depth);
    for (auto target : targets) if (target) target->Release();
    if (depth) depth->Release();
    submitted = SUCCEEDED(result);
    return result;
}

DisplayState LowLatencyState() { return state.load(); }
uint32_t LowLatencyError() { return static_cast<uint32_t>(lastError.load()); }
}
