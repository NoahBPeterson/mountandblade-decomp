//-----------------------------------------------------------------------------
// File: launcher_options_dialog.cpp  (0x4b96d0..0x4b9e70)
//
// Desc: two registry helpers for the key file, the launcher's options dialog (tab switching, the DXDiag
//       video-memory probe, paint), the start-up display probe and the version check (the HTTP client it calls
//       is game/config/http_client.cpp, its own object in the original). Names provisional. Ascending address order.
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: the map puts 0x4b9b90 between this object and the next (class change OptionsDlg|Core, low). The
// function at 0x4b9b90 is itself an OptionsDlg method (choose_dx_settings), so the object runs on to 0x4b9e70
// (CreateGameOptionsPage::refresh, the first function of the next class); see MIGRATION.md.
//-----------------------------------------------------------------------------
#include "platform/image_addrs.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "game/config/launcher_options_dialog.h"
#include "engine/core/crt.h"      // canonical CRT declarations (listed directly: the resolution audit reads only the source's own includes)
#include "game/config/core.h"
#include "game/config/config.h"
#include "game/ui/gui_manager.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "engine/render/dev_buffer.h"
#include "engine/scene/frustum.h"
#include "engine/math/mat4_include.h"
#include "engine/render/render_batch.h"      // (listed directly: the resolution audit scans only the source's own includes)

// @ 0x4b96d0
// (the log object is reached through a stream_base reference: cl would devirtualise a call on the extern object itself)
void __cdecl open_keys_key(PHKEY out)
{
    DWORD disp;
    RegCreateKeyExA((HKEY)0x80000001, "Software\\MountAndBladeKeys", 0, 0, 0, 0x20007, 0, out, &disp);
    if (disp != 1) {
        return;
    }
    ((stream_base&)g_log).w_cstr("Creating new Registry Key");
}

// @ 0x4b9720
void __cdecl read_key_string(HKEY* key, LPCSTR name, tstring* out)
{
    DWORD type;
    DWORD size = 0x100;
    char buf[256];
    buf[255] = '\0';
    out->assign("");
    if (*key) {
        if (RegQueryValueExA(*key, name, 0, &type, (LPBYTE)buf, &size) == 0)
            out->assign(buf);
    }
}

// ---- DirectX diagnostic COM interfaces (dxdiag.h is not on the toolchain) --------------------------------------
// DXDiag bases its objects directly on IUnknown; only the slots this object calls are named
// (Initialize +0xc, GetRootContainer +0x10; GetChildContainer +0x14, EnumChildContainerName +0x10, GetProp +0x20).
struct DXDIAG_INIT_PARAMS {
    DWORD dwSize;
    DWORD dwDxDiagHeaderVersion;
    BOOL bAllowWHQLChecks;
    void* pReserved;
};
struct IDxDiagContainer : public IUnknown {
    virtual HRESULT __stdcall GetNumberOfChildContainers(DWORD* n) = 0;
    virtual HRESULT __stdcall EnumChildContainerName(DWORD idx, WCHAR* name, DWORD cap) = 0;
    virtual HRESULT __stdcall GetChildContainer(const WCHAR* name, IDxDiagContainer** c) = 0;
    virtual HRESULT __stdcall GetNumberOfProps(DWORD* n) = 0;
    virtual HRESULT __stdcall EnumPropName(DWORD idx, WCHAR* name, DWORD cap) = 0;
    virtual HRESULT __stdcall GetProp(WCHAR* name, VARIANT* v) = 0;
    virtual HRESULT __stdcall GetPropByName(const WCHAR* prop, const WCHAR* name, VARIANT* v) = 0;
    virtual HRESULT __stdcall GetContainerByIndex(DWORD idx, IDxDiagContainer** c) = 0;
};
struct IDxDiagProvider : public IUnknown {
    virtual HRESULT __stdcall Initialize(void* params) = 0;
    virtual HRESULT __stdcall GetRootContainer(IDxDiagContainer** root) = 0;
};
// ---- data this module owns (dxdiag.h's GUIDs; values read from the image once by tools/relink/gen_data.py) ----
extern const GUID IID_IDxDiagProvider = { 0x9c6b4cb0, 0x23f8, 0x49cc, { 0xa3, 0xed, 0x45, 0xa5, 0x50, 0x00, 0xa6, 0xd2 } };       // 0x59c8e4
extern const GUID CLSID_DxDiagProvider = { 0xa65b8071, 0x3bfe, 0x4213, { 0x9a, 0x5b, 0x49, 0x1d, 0xa4, 0x46, 0x1c, 0xa7 } };      // 0x59c8f4





// @ 0x4b9aa0
long __cdecl get_latest_version()
{
    http_result reply;
    reply.url = 0;
    reply.headers = 0;
    reply.body = 0;
    if (http_fetch("http://www.taleworlds.com/mb_version.txt", 0, 0, 0, &reply) > 0)
        return -1;
    return atoi(reply.body);
}

// @ 0x4b9790
void GameApp::set_active_tab(int tab)
{
    int show = (tab != 0) - 1 & 5;
    HWND h;
    ShowWindow(GetDlgItem(options_hwnd, 0x43f), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x449), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x447), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x450), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x43e), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x441), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x493), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x468), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x444), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x448), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x445), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x487), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x460), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x45f), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x462), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x465), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x492), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x467), show);
    show = (tab != 1) - 1 & 5;
    ShowWindow(GetDlgItem(options_hwnd, 0x452), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x446), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x44d), show);
    show = (tab != 2) - 1 & 5;
    ShowWindow(GetDlgItem(options_hwnd, 0x443), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x442), show);
    show = (tab != 3) - 1 & 5;
    ShowWindow(GetDlgItem(options_hwnd, 0x44c), show);
    ShowWindow(GetDlgItem(options_hwnd, 0x44a), show);
}

// @ 0x4b99c0
void run_settings_probe()
{
    if (g_app_obj.m_bWindowed)
        return;
    g_app_obj.Pause(true);
    g_app_obj.m_bWindowed = 1;
    if (g_app_obj.ChooseInitialD3DSettings() < 0)
        g_app_obj.DisplayErrorMsg(0x8200000c, (DWORD)1);
    if (g_app_obj.ToggleFullscreen() < 0)
        g_app_obj.DisplayErrorMsg(0x8200000c, (DWORD)1);
    g_app_obj.Pause(false);
}

// @ 0x4b9a30
// Reads a DXDiag container property (a BSTR) into a char buffer. A GameApp member that never reads this (the
// caller passes it in ecx, `mov ecx,esi`); four stack arguments (ret 0x10). A failed GetProp returns its code, a
// non-BSTR value E_INVALIDARG (neither clears the variant).
__declspec(noinline) long GameApp::prop_string(IDxDiagContainer* obj, WCHAR* name, char* buf, unsigned cap)
{
    VARIANT result;
    VariantInit(&result);
    HRESULT hr = obj->GetProp(name, &result);
    if (hr < 0)
        return hr;
    if (result.vt != VT_BSTR)
        return 0x80070057;
    wcstombs(buf, result.bstrVal, cap);
    buf[cap - 1] = 0;
    VariantClear(&result);
    return 0;
}

// @ 0x4b9af0
void GameApp::paint()
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hwnd, &ps);
    HDC mem = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(mem, hbmp);
    BITMAP bm;
    GetObjectA(hbmp, sizeof(BITMAP), &bm);
    BitBlt(dc, 0x108, 0x3f, 0x154, 0x113, mem, 0, 0, 0xcc0020);
    SelectObject(mem, old);
    DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

// @ 0x4b9b90
// Picks the texture detail / shader defaults from the video memory DXDiag reports ("32.0 MB" ...); the options block
// (a Config copy on the caller's stack) is only written when `overwrite` is set, except for the low-memory case.
void GameApp::choose_dx_settings(Config* cfg, bool overwrite)
{
    MEMORYSTATUS mem;                           // zeroed whole by memset (one xor eax,eax, eight stores); the result is never read
    memset(&mem, 0, sizeof(mem));
    GlobalMemoryStatus(&mem);

    IDxDiagContainer* display = 0;
    IDxDiagContainer* devices = 0;
    IDxDiagProvider* provider = 0;
    IDxDiagContainer* root = 0;
    DXDIAG_INIT_PARAMS params;
    WCHAR name[0x100];
    char mem_text[0x100];

    do {
        if (CoInitialize(0) < 0)                // a failed CoInitialize skips straight to the releases
            break;
        if (CoCreateInstance(CLSID_DxDiagProvider, 0, 1, IID_IDxDiagProvider, (void**)&provider) < 0)
            break;
        if (!provider)
            break;
        memset(&params, 0, sizeof(params));
        params.dwSize = 0x10;
        params.dwDxDiagHeaderVersion = 0x6f;
        params.bAllowWHQLChecks = 0;
        params.pReserved = 0;
        if (provider->Initialize(&params) < 0)
            break;
        if (provider->GetRootContainer(&root) < 0)
            break;
        if (root->GetChildContainer(L"DxDiag_DisplayDevices", &devices) < 0)
            break;
        if (devices->EnumChildContainerName(0, name, 0x100) < 0)
            break;
        if (devices->GetChildContainer(name, &display) < 0)
            break;
        if (prop_string(display, L"szDisplayMemoryEnglish", mem_text, 0x100) < 0)
            break;
        tstring s(mem_text);
        s.to_upper();
        if (s.equals("4.0 MB") || s.equals("8.0 MB") || s.equals("16.0 MB")) {
            cfg->texture_detail = 0;
            cfg->antialiasing = 0;
            cfg->use_vertex_shaders = 0;
            cfg->shadowmap_quality = 0;
        } else if (s.equals("32.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x14;
                cfg->use_vertex_shaders = 0;
                cfg->shadowmap_quality = 0;
            }
        } else if (s.equals("64.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x3c;
                cfg->use_vertex_shaders = 0;
                cfg->shadowmap_quality = 0;
            }
        } else if (s.equals("128.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x50;
                cfg->shadowmap_quality = 1;
            }
        } else if (s.equals("256.0 MB")) {
            if (overwrite) {
                cfg->texture_detail = 0x64;
                cfg->shadowmap_quality = 1;
            }
        } else {                                // unknown sizes as 256 MB: both arms merge, the last equals result is unused
            if (overwrite) {
                cfg->texture_detail = 0x64;
                cfg->shadowmap_quality = 1;
            }
        }
    } while (0);
    if (display) {
        display->Release();
        display = 0;
    }
    if (devices) {
        devices->Release();
        devices = 0;
    }
    if (root) {
        root->Release();
        root = 0;
    }
    if (provider)
        provider->Release();
}
