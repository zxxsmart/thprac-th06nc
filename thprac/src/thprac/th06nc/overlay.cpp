#include "module.h"
#include "thprac_gui_components.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "MinHook.h"
#include <d3d11.h>
#include <memory>

namespace THPrac {
constinit THPracSettings gSettings{.language = LOCALE_ZH_CN};
constinit HotkeyChords hotkeys{};
}
namespace THPrac::TH06NC {
using namespace TH06;
THPracParam thPracParam{};
#include "practice_ui.inl"
THGuiPrac& PracticeUi();
#include "pause_ui.inl"
class THOverlay : public Gui::GameGuiWnd {
public:
    THOverlay() {
        SetTitle("Mod Menu");SetFade(0.5f,0.5f);SetPos(10,10);SetSize(0,0);
        SetWndFlag(ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
            ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoNav);
    }
    void OnContentUpdate() override {
        const th_glossary_t labels[]={TH_MUTEKI,TH_INFLIVES,TH_INFBOMBS,TH_INFPOWER,TH_TIMELOCK,TH_AUTOBOMB,TH_EL_BGM};
        const uint32_t masks[]={1,2,4,8,16,32,128};
        for(int i=0;i<7;++i) {
            auto cursor=ImGui::GetCursorPos();
            if(PracticeFlags()&masks[i])ImGui::TextColored({0,1,0,1},"[F%d: %s]",i+1,S(labels[i]));
            else ImGui::Text("F%d: %s",i+1,S(labels[i]));
            ImGui::SetCursorPos(cursor);ImGui::PushID(i);
            if(ImGui::InvisibleButton("toggle",{230,ImGui::GetTextLineHeight()}))TogglePracticeFlag(masks[i]);
            ImGui::PopID();
        }
        ImGui::Separator();
        if(ImGui::Button(S(TH_RESTART)))RequestPracticeRestart();
        ImGui::SameLine();
        const char* save[]={"保存录像","Save replay","リプレイ保存"};
        if(ImGui::Button(save[Gui::LocaleGet()]))RequestReplaySave();
    }
};
class THAdvanced : public Gui::GameGuiWnd {
    Gui::GuiSlider<int,ImGuiDataType_S32> fps{"FPS",30,240};
public:
    THAdvanced(){SetTitle("Advanced Options");SetFade(0.8f,0.8f);SetPos(260,65);SetSize(330,115);SetItemWidth(-60);}
    void OnContentUpdate() override {
        ImGui::TextUnformatted(S(TH_ADV_OPT));ImGui::Separator();
        ImGui::TextUnformatted(S(TH_GAME_SPEED));
        *fps=PracticeFps();if(fps())SetPracticeFps(*fps);
    }
};
namespace {
using PresentFn = HRESULT (STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
PresentFn originalPresent{};
ID3D11Device* device{};
ID3D11DeviceContext* context{};
ImGuiContext* guiContext{};
std::unique_ptr<THGuiPrac> practice;
std::unique_ptr<THOverlay> overlay;
std::unique_ptr<THAdvanced> advanced;
std::unique_ptr<THPauseMenu> pause;
bool pauseWanted{};
int pauseAction{};
bool wanted{}, opened{};
bool quickWanted{}, advancedWanted{}, inputHooked{};
int action{}, openingFrames{};
uint16_t uiInput{}, uiPrevious{}, uiRepeat{};

HRESULT STDMETHODCALLTYPE Present(IDXGISwapChain* swap, UINT interval, UINT flags)
{
    if(!PracticeActive()){quickWanted=false;advancedWanted=false;pauseWanted=false;}
    if (!wanted && !opened && !quickWanted && !advancedWanted && !pauseWanted && !inputHooked) return originalPresent(swap, interval, flags);
    DXGI_SWAP_CHAIN_DESC desc{};
    if (FAILED(swap->GetDesc(&desc))) return originalPresent(swap, interval, flags);
    auto previousContext = ImGui::GetCurrentContext();
    if (!guiContext) {
        if (FAILED(swap->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device))))
            return originalPresent(swap, interval, flags);
        device->GetImmediateContext(&context);
        guiContext = ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        ImGui::StyleColorsDark();
        auto language = gSettings.language;
        Gui::LocaleCreateFont(16.0f);
        Gui::LocaleSet(language);
        Gui::ImplWin32Init(desc.OutputWindow);
        ImGui_ImplDX11_Init(device, context);
        Gui::InGameInputInit(Gui::INGAGME_INPUT_GEN1,
            reinterpret_cast<uintptr_t>(&uiInput), reinterpret_cast<uintptr_t>(&uiPrevious),
            reinterpret_cast<uintptr_t>(&uiRepeat));
        practice = std::make_unique<THGuiPrac>();
        overlay = std::make_unique<THOverlay>();
        advanced = std::make_unique<THAdvanced>();
        pause = std::make_unique<THPauseMenu>();
    }
    ImGui::SetCurrentContext(guiContext);
    if (wanted && !opened) {
        practice->State(1);
        opened = true;
        openingFrames = 0;
    }
    if (!wanted && opened) {
        practice->State(action == 1 ? 3 : 4);
        opened = false;
    }
    bool anyVisible=wanted||quickWanted||advancedWanted||pauseWanted;
    if(anyVisible&&!inputHooked){Gui::ImplWin32HookWndProc();inputHooked=true;}
    if(!anyVisible) {
        if(inputHooked){Gui::ImplWin32UnHookWndProc();inputHooked=false;}
        ImGui::SetCurrentContext(previousContext);
        return originalPresent(swap, interval, flags);
    }
    auto native = *reinterpret_cast<uint16_t*>(imageBase + Rva::Input);
    auto prev = *reinterpret_cast<uint16_t*>(imageBase + Rva::PreviousInput);
    uiInput = (native & 0xf4) | ((native & 0x100) ? 1 : 0);
    uiPrevious = (prev & 0xf4) | ((prev & 0x100) ? 1 : 0);
    uiRepeat = *reinterpret_cast<uint16_t*>(imageBase + Rva::InputRepeat);
    auto& io = ImGui::GetIO();
    RECT client{};
    GetClientRect(desc.OutputWindow, &client);
    float screenScale = std::max(1.0f, float(client.bottom)) / 480.0f;
    io.DisplaySize = {float(client.right) / screenScale, 480};
    ImGui_ImplDX11_NewFrame();
    Gui::ImplWin32NewFrame();
    Gui::KeyboardInputUpdate();
    // ImGui consumes navigation inputs in NewFrame, before widgets are drawn.
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    io.NavInputs[ImGuiNavInput_DpadUp] = Gui::InGameInputGet(VK_UP);
    io.NavInputs[ImGuiNavInput_DpadDown] = Gui::InGameInputGet(VK_DOWN);
    io.NavInputs[ImGuiNavInput_DpadLeft] = Gui::InGameInputGet(VK_LEFT);
    io.NavInputs[ImGuiNavInput_DpadRight] = Gui::InGameInputGet(VK_RIGHT);
    ImGui::NewFrame();
    if(wanted)practice->Update();
    if(pauseWanted){
        pauseAction=pause->PMState();
        pause->Update();
        if(pauseAction)pauseWanted=false;
    }
    if(quickWanted){overlay->Open();overlay->Update();}else overlay->Close();
    if(advancedWanted){advanced->Open();advanced->Update();}else advanced->Close();
    if (wanted && ++openingFrames > 12 && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel) && !ImGui::IsAnyItemActive()) {
        if ((native & 0x100) && !(prev & 0x100)) {
            practice->State(3);
            action = 1;
            wanted = false;
        } else if ((native & 0x200) && !(prev & 0x200)) {
            action = 2;
            wanted = false;
        }
    }
    ImGui::Render();
    ID3D11Texture2D* buffer{};
    ID3D11RenderTargetView* target{};
    if (SUCCEEDED(swap->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&buffer)))) {
        D3D11_TEXTURE2D_DESC surface{};
        buffer->GetDesc(&surface);
        device->CreateRenderTargetView(buffer, nullptr, &target);
        buffer->Release();
        if (target) {
            auto data = ImGui::GetDrawData();
            ImVec2 scale{surface.Width / io.DisplaySize.x, surface.Height / io.DisplaySize.y};
            data->ScaleClipRects(scale);
            for (int n = 0; n < data->CmdListsCount; ++n)
                for (auto& vertex : data->CmdLists[n]->VtxBuffer) {
                    vertex.pos.x *= scale.x;
                    vertex.pos.y *= scale.y;
                }
            data->DisplaySize = {float(surface.Width), float(surface.Height)};
            ID3D11RenderTargetView* saved[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
            ID3D11DepthStencilView* depth{};
            context->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, saved, &depth);
            context->OMSetRenderTargets(1, &target, nullptr);
            ImGui_ImplDX11_RenderDrawData(data);
            context->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, saved, depth);
            for (auto view : saved) if (view) view->Release();
            if (depth) depth->Release();
            target->Release();
        }
    }
    ImGui::SetCurrentContext(previousContext);
    return originalPresent(swap, interval, flags);
}
}
THGuiPrac& PracticeUi(){return *practice;}
void OpenPause(){pauseWanted=true;pauseAction=0;}
void ClosePause(){pauseWanted=false;}
int TakePauseAction(){int result=pauseAction;pauseAction=0;return result;}
void OpenPractice() { wanted = true; action = 0; }
void ClosePractice() { wanted = false; }
bool PracticeIsOpen() { return wanted || opened; }
int TakePracticeAction() { int result = action; action = 0; return result; }
void ToggleQuickMenu(){if(PracticeActive())quickWanted=!quickWanted;}
void ToggleAdvancedMenu(){if(PracticeActive())advancedWanted=!advancedWanted;}
bool InstallOverlay()
{
    // A hidden temporary swap chain gives the system's actual Present entry point.
    auto instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpfnWndProc = DefWindowProcW;
    wc.lpszClassName = L"thprac.D3D11.discovery";
    RegisterClassW(&wc);
    HWND window = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPED, 0, 0, 32, 32,
        nullptr, nullptr, instance, nullptr);
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount = 1;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = window;
    desc.SampleDesc.Count = 1;
    desc.Windowed = TRUE;
    IDXGISwapChain* swap{};
    ID3D11Device* discoveryDevice{};
    ID3D11DeviceContext* discoveryContext{};
    bool ok = false;
    if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, nullptr, 0, D3D11_SDK_VERSION, &desc, &swap, &discoveryDevice, nullptr, &discoveryContext))) {
        auto entry = (*reinterpret_cast<void***>(swap))[8];
        ok = MH_CreateHook(entry, reinterpret_cast<void*>(Present), reinterpret_cast<void**>(&originalPresent)) == MH_OK;
        discoveryContext->Release();
        discoveryDevice->Release();
        swap->Release();
    }
    DestroyWindow(window);
    UnregisterClassW(wc.lpszClassName, instance);
    return ok;
}
}
