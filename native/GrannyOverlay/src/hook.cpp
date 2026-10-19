// DirectX 11 Present hook + window message hook that host the ImGui menu inside the game.
// Loaded by the C# plugin (Overlay.cs), which calls Overlay_Init() once and then talks
// to us only through the SharedState returned by Overlay_GetState().

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <mutex>
#include <string>
#include <vector>

#include "menu.h"
#include "shared.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx11.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

SharedState g_State = {};

using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using Present1Fn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

static PresentFn g_OrigPresent;
static Present1Fn g_OrigPresent1;
static ResizeBuffersFn g_OrigResizeBuffers;
static void** g_SwapChainVTable;

static IDXGISwapChain* g_SwapChain;
static ID3D11Device* g_Device;
static ID3D11DeviceContext* g_Context;
static ID3D11RenderTargetView* g_RTV;
static HWND g_Window;
static WNDPROC g_OrigWndProc;
static bool g_Ready;

// Window messages arrive on the game's main thread but ImGui runs on the render thread,
// so messages are queued here and fed to ImGui right before each frame.
struct QueuedMsg { HWND hwnd; UINT msg; WPARAM wp; LPARAM lp; };
static std::mutex g_MsgLock;
static std::vector<QueuedMsg> g_Msgs;

static bool IsMouseMsg(UINT msg)
{
    return (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || msg == WM_INPUT;
}

static bool IsKeyMsg(UINT msg)
{
    return msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_CHAR || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP;
}

// Mouse button index for button messages (0 L, 1 R, 2 M, 3 X1, 4 X2), or -1.
static int MouseButton(UINT msg, WPARAM wp, bool* down)
{
    switch (msg)
    {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: *down = true;  return 0;
    case WM_LBUTTONUP:                          *down = false; return 0;
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: *down = true;  return 1;
    case WM_RBUTTONUP:                          *down = false; return 1;
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: *down = true;  return 2;
    case WM_MBUTTONUP:                          *down = false; return 2;
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: *down = true;  return GET_XBUTTON_WPARAM(wp) == XBUTTON1 ? 3 : 4;
    case WM_XBUTTONUP:                          *down = false; return GET_XBUTTON_WPARAM(wp) == XBUTTON1 ? 3 : 4;
    default: return -1;
    }
}

// Whether the game got each button's last press, so it also gets the matching release.
static bool g_GameHasButton[5];

static LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (msg == WM_KEYDOWN && wp == VK_DELETE && !(lp & (1 << 30)))
    {
        g_State.menuOpen = !g_State.menuOpen;
        if (!g_State.menuOpen)
            g_State.actSaveConfig = 1;
        return 0;
    }

    bool buttonDown = false;
    int button = MouseButton(msg, wp, &buttonDown);

    // Button-ups always go to ImGui too, so a press it saw can't stay "held" after the menu closes.
    if (g_Ready && (g_State.menuOpen || g_State.radialOpen || msg == WM_KILLFOCUS || msg == WM_SETFOCUS ||
                    (button >= 0 && !buttonDown)))
    {
        std::lock_guard<std::mutex> lock(g_MsgLock);
        g_Msgs.push_back({ hwnd, msg, wp, lp });
    }

    // While the menu or the enemy wheel is open the game shouldn't look around or walk.
    // With Cinematic Unity Explorer open, only block what our own window is using so the
    // explorer still gets clicks and typing everywhere else.
    bool uiOpen = g_State.menuOpen || g_State.radialOpen;
    bool block = false;
    if (uiOpen && g_Ready)
    {
        const ImGuiIO& io = ImGui::GetIO();
        bool shareInput = g_State.explorerStatus == 2 && !g_State.radialOpen;
        if (IsMouseMsg(msg))
            block = !shareInput || msg == WM_INPUT || io.WantCaptureMouse;
        else if (IsKeyMsg(msg))
            block = g_State.menuOpen && (!shareInput || io.WantCaptureKeyboard);
    }
    // A release follows its press: if the game got the press it gets the release, otherwise not.
    if (button >= 0)
    {
        if (buttonDown)
            g_GameHasButton[button] = !block;
        else
        {
            block = !g_GameHasButton[button];
            g_GameHasButton[button] = false;
        }
    }

    if (block)
    {
        // Let key-ups through so nothing stays held down in the game.
        if (msg != WM_KEYUP && msg != WM_SYSKEYUP)
            return msg == WM_INPUT ? DefWindowProcW(hwnd, msg, wp, lp) : 0;
    }

    return CallWindowProcW(g_OrigWndProc, hwnd, msg, wp, lp);
}

static void CreateRenderTarget(IDXGISwapChain* swapChain)
{
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer)) && backBuffer)
    {
        g_Device->CreateRenderTargetView(backBuffer, nullptr, &g_RTV);
        backBuffer->Release();
    }
}

static bool InitOnFirstPresent(IDXGISwapChain* swapChain)
{
    if (FAILED(swapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_Device)) || !g_Device)
        return false;
    g_Device->GetImmediateContext(&g_Context);

    DXGI_SWAP_CHAIN_DESC desc;
    swapChain->GetDesc(&desc);
    g_Window = desc.OutputWindow;
    g_SwapChain = swapChain;

    CreateRenderTarget(swapChain);
    if (!Menu_Init(g_Window, g_Device, g_Context))
        return false;

    g_OrigWndProc = (WNDPROC)SetWindowLongPtrW(g_Window, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
    g_Ready = true;
    g_State.hooked = 1;
    return true;
}

static void RenderOverlay(IDXGISwapChain* swapChain)
{
    static bool failed = false;
    if (!g_Ready && !failed)
        failed = !InitOnFirstPresent(swapChain);

    // Only draw on the game's own swap chain.
    if (g_Ready && swapChain == g_SwapChain)
    {
        {
            std::lock_guard<std::mutex> lock(g_MsgLock);
            for (const auto& m : g_Msgs)
                ImGui_ImplWin32_WndProcHandler(m.hwnd, m.msg, m.wp, m.lp);
            g_Msgs.clear();
        }

        if (!g_RTV)
            CreateRenderTarget(swapChain);

        Radial_Update(g_Window);
        Menu_Frame();
        if (g_RTV)
        {
            g_Context->OMSetRenderTargets(1, &g_RTV, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    }
}

// Unity may present with either Present or Present1; only draw once if one calls the other.
static thread_local bool t_InPresent;

static HRESULT STDMETHODCALLTYPE HookedPresent(IDXGISwapChain* swapChain, UINT syncInterval, UINT flags)
{
    if (t_InPresent)
        return g_OrigPresent(swapChain, syncInterval, flags);
    t_InPresent = true;
    RenderOverlay(swapChain);
    HRESULT hr = g_OrigPresent(swapChain, syncInterval, flags);
    t_InPresent = false;
    return hr;
}

static HRESULT STDMETHODCALLTYPE HookedPresent1(IDXGISwapChain1* swapChain, UINT syncInterval, UINT flags, const DXGI_PRESENT_PARAMETERS* params)
{
    if (t_InPresent)
        return g_OrigPresent1(swapChain, syncInterval, flags, params);
    t_InPresent = true;
    RenderOverlay(swapChain);
    HRESULT hr = g_OrigPresent1(swapChain, syncInterval, flags, params);
    t_InPresent = false;
    return hr;
}

static HRESULT STDMETHODCALLTYPE HookedResizeBuffers(IDXGISwapChain* swapChain, UINT count, UINT w, UINT h, DXGI_FORMAT fmt, UINT flags)
{
    if (g_RTV && swapChain == g_SwapChain)
    {
        g_RTV->Release();
        g_RTV = nullptr;
    }
    return g_OrigResizeBuffers(swapChain, count, w, h, fmt, flags);
}

static bool PatchVTable(void** vtable, int index, void* hook, void** original)
{
    DWORD old;
    if (!VirtualProtect(&vtable[index], sizeof(void*), PAGE_EXECUTE_READWRITE, &old))
        return false;
    *original = vtable[index];
    vtable[index] = hook;
    VirtualProtect(&vtable[index], sizeof(void*), old, &old);
    return true;
}

// Every DXGI swap chain shares one vtable, so a throwaway swap chain tells us where Present lives.
static bool HookSwapChain()
{
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, DefWindowProcW, 0, 0, GetModuleHandleW(nullptr), nullptr, nullptr, nullptr, nullptr, L"GrannyOverlayDummy", nullptr };
    RegisterClassExW(&wc);
    HWND dummy = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, nullptr, nullptr, wc.hInstance, nullptr);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = dummy;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* swapChain = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL level;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                                               D3D11_SDK_VERSION, &sd, &swapChain, &device, &level, &context);
    if (FAILED(hr))
        hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                           D3D11_SDK_VERSION, &sd, &swapChain, &device, &level, &context);

    bool ok = false;
    if (SUCCEEDED(hr) && swapChain)
    {
        g_SwapChainVTable = *(void***)swapChain;
        ok = PatchVTable(g_SwapChainVTable, 8, (void*)&HookedPresent, (void**)&g_OrigPresent) &&
             PatchVTable(g_SwapChainVTable, 13, (void*)&HookedResizeBuffers, (void**)&g_OrigResizeBuffers);

        // Present1 (slot 22) only exists if the swap chain is an IDXGISwapChain1.
        IDXGISwapChain1* swapChain1 = nullptr;
        if (ok && SUCCEEDED(swapChain->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&swapChain1)) && swapChain1)
        {
            if (*(void***)swapChain1 == g_SwapChainVTable)
                PatchVTable(g_SwapChainVTable, 22, (void*)&HookedPresent1, (void**)&g_OrigPresent1);
            swapChain1->Release();
        }
    }

    if (swapChain) swapChain->Release();
    if (context) context->Release();
    if (device) device->Release();
    DestroyWindow(dummy);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return ok;
}

extern "C" __declspec(dllexport) SharedState* Overlay_GetState()
{
    return &g_State;
}

extern "C" __declspec(dllexport) int Overlay_GetVersion()
{
    return SHARED_VERSION;
}

extern "C" __declspec(dllexport) int Overlay_GetStateSize()
{
    return (int)sizeof(SharedState);
}

// Returns 1 if the Present hook was installed. Safe to call more than once.
extern "C" __declspec(dllexport) int Overlay_Init()
{
    static int result = -1;
    if (result == -1)
    {
        g_State.version = SHARED_VERSION;
        result = HookSwapChain() ? 1 : 0;
    }
    return result;
}

#ifdef STANDALONE
#include "game.h"

// Injected on its own: start the C++ cheats, then the ImGui overlay. Runs on a worker thread so
// DllMain returns straight away (the loader lock must not be held while we wait for the game).
static DWORD WINAPI StandaloneMain(LPVOID module)
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW((HMODULE)module, path, MAX_PATH);
    char utf8[MAX_PATH * 3];
    WideCharToMultiByte(CP_UTF8, 0, path, -1, utf8, sizeof(utf8), nullptr, nullptr);
    std::string dir = utf8;
    dir = dir.substr(0, dir.find_last_of("\\/") + 1);

    if (Game_Start(dir))
    {
        if (Overlay_Init() == 1)
            Game_Log("ImGui overlay hooked DirectX 11.");
        else
            Game_Log("ImGui overlay couldn't hook DirectX 11.");
    }
    return 0;
}
#endif

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
#ifdef STANDALONE
        if (HANDLE t = CreateThread(nullptr, 0, StandaloneMain, instance, 0, nullptr))
            CloseHandle(t);
#endif
    }
    return TRUE;
}
