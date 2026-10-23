//-----------------------------------------------------------------------------
// File: config.cpp  (0x4b9e70..0x4be730)
//
// Desc: the create-game page refresh, the user folders, the Direct3D start-up probes of the application object
//       (view rectangle, D3D creation, the configured fullscreen mode), the launcher's list/progress helpers, the
//       screenshot writer, the option block (rgl_config.txt) reader and setters, the software cursor, the
//       DirectInput device set, the render batch's buffer and declaration tables, the ambient particle ring, the
//       camera ray, the ini-file reader and the module-file record readers (triggers, particle systems,
//       presentations, sounds, the music player, menu items, script and clip records).
//       Names provisional. Functions in ascending address order (cl's bottom-up emission).
//       Flags: /O2 /MT /Gy /TP.
//
// Boundary: no call leaves 0x4b9e70..0x4be730 forward (re-checked on work/migrate/facts.json: the only forward
// calls out of 0x4b8f40..0x4be730 are the FMOD / d3d9 import thunks at 0x5342e2.., 0x534342), so the range is
// one object as far as the calls can tell; the map's class changes inside (Core | Config | Cursor | renderer |
// ini_file | records) have no call support. See MIGRATION.md. The end is the WinLicense marker function at
// 0x4be730 (next object, meta_mission::init).
//-----------------------------------------------------------------------------
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include "./config.h"
#include "../../rgl/resource.h"

// ---- imports and library functions the slices declared by hand ------------------------------------------------------
extern "C" __declspec(dllimport) long __stdcall SHGetFolderPathA(HWND hwnd, int csidl, HANDLE token, DWORD flags, char* path);   // 0x5492c0
extern "C" int _atoi(const char* str);                      // 0x53734d (atoi: jmp _atol)
extern "C" IDirect3D9* __stdcall Direct3DCreate9(UINT);     // 0x534342
typedef void* FMOD_STREAM;
extern "C" short __stdcall FSOUND_Stream_Close(FMOD_STREAM s);                                              // 0x5342e2
extern "C" FMOD_STREAM __stdcall FSOUND_Stream_Open(const char* name, unsigned mode, int offset, int length);   // 0x5342e8
rgl::matrix identity_pos();                                 // 0x40cd00 (cdecl, hidden return pointer)
void __fastcall tstring_ctor(tstring* p);                   // 0x42bbd0 (tstring::tstring, called on raw slots)
void __fastcall tstring_dtor(tstring* p);                   // 0x42bc40
bool ConfirmDeviceHelper(D3DCAPS9* pCaps, VertexProcessingType vp, D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat);   // 0x401010

// ---- 0x4b9e70 ----------------------------------------------------------------------------------------------------------
// The original is a straight run of macro expansions: ShowWindow(GetDlgItem(hDlg, id), test ? SW_SHOW : SW_HIDE)
// with the condition spelled differently per group, which shows in the codegen (mode 1: single ==; modes 1..3:
// cmp 1 / cmp 2 / cmp 3; the other groups `!= X ? SW_HIDE : SW_SHOW`, the setne + dec/and 5 sequence).
// The sub-option buttons are enabled/disabled afterwards (the first three disable themselves once selected).
enum GameMode {
    MODE_CAMPAIGN = 0,          // single-player campaign page
    MODE_QUICK = 1,             // quick battle
    MODE_CUSTOM = 2,            // custom battle
    MODE_MULTIPLAYER = 3,       // multiplayer client (join)
    MODE_MULTIPLAYER_HOST = 4,  // multiplayer host (create server)
};

// @ 0x4b9e70
void CreateGameOptionsPage::refresh()
{
    // Quick-battle only.
    ShowWindow(GetDlgItem(hDlg, 0x46e), g_config->game_mode == MODE_QUICK ? SW_SHOW : SW_HIDE);

    // Custom battle only.
    ShowWindow(GetDlgItem(hDlg, 0x474), g_config->game_mode != MODE_CUSTOM ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x475), g_config->game_mode != MODE_CUSTOM ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x476), g_config->game_mode != MODE_CUSTOM ? SW_HIDE : SW_SHOW);

    // Shared by quick battle, custom battle and the multiplayer client.
    ShowWindow(GetDlgItem(hDlg, 0x472), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x47d), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x47f), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x473), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x47e), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x480), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x477), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x481), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x482), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, 0x4af), g_config->game_mode == MODE_QUICK || g_config->game_mode == MODE_CUSTOM || g_config->game_mode == MODE_MULTIPLAYER ? SW_SHOW : SW_HIDE);

    // Multiplayer client (join) only.
    ShowWindow(GetDlgItem(hDlg, 0x4cd), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4c8), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4cf), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4c9), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ce), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ca), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4cb), g_config->game_mode != MODE_MULTIPLAYER ? SW_HIDE : SW_SHOW);

    // Single-player campaign only.
    ShowWindow(GetDlgItem(hDlg, 0x439), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x43a), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x43b), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x44e), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x44f), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x435), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x436), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x437), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x478), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x479), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x47a), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48b), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48c), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48d), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48e), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x48f), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x490), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x491), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x492), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x493), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x438), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x431), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x434), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x433), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x47b), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x47c), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x453), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x454), g_config->game_mode != MODE_CAMPAIGN ? SW_HIDE : SW_SHOW);

    // Multiplayer host (create server) only.
    ShowWindow(GetDlgItem(hDlg, 0x494), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ae), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x497), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x498), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x499), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a5), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a3), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a4), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x49b), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4aa), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a7), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a6), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4a8), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x49d), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ac), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, 0x4ad), g_config->game_mode != MODE_MULTIPLAYER_HOST ? SW_HIDE : SW_SHOW);

    // Sub-option buttons of the host page. The first three are the radio buttons: the selected one is disabled
    // (you cannot re-select it), every other one enabled.
    EnableWindow(GetDlgItem(hDlg, 0x4a5), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 0 ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, 0x4a3), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hDlg, 0x4a4), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 2 ? FALSE : TRUE);
    // The rest are only usable with the matching sub-option selected.
    EnableWindow(GetDlgItem(hDlg, 0x49b), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4aa), g_config->game_mode == MODE_MULTIPLAYER_HOST && (g_config->sub_option == 0 || g_config->sub_option == 1) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4a7), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 0 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4a6), !(g_config->game_mode != MODE_MULTIPLAYER_HOST));
    EnableWindow(GetDlgItem(hDlg, 0x4a8), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 2 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x49d), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4ac), g_config->game_mode == MODE_MULTIPLAYER_HOST && (g_config->sub_option == 0 || g_config->sub_option == 1) ? TRUE : FALSE);
    EnableWindow(GetDlgItem(hDlg, 0x4ad), g_config->game_mode == MODE_MULTIPLAYER_HOST && g_config->sub_option == 1 ? TRUE : FALSE);
}

// ---- 0x4bab70 user folders ---------------------------------------------------------------------------------------------
// The probes only read the low dword of `name`, so the original keeps a 0x200-byte scratch buffer at [esp].
// @ 0x4bab70
static void user_path(char* out)
{
    char path[0x104];
    char name[0x104];

    SHGetFolderPathA(0, 0x1a, 0, 0, path);              // CSIDL_APPDATA
    sprintf(name, "%s\\Mount&Blade", path);
    CreateDirectoryA(name, 0);
    sprintf(name, "%s", out);
    sprintf(out, "%s\\Mount&Blade\\%s", path, name);
}

// @ 0x4babf0
static void screenshots_path(char* out)
{
    char path[0x104];
    char name[0x104];

    SHGetFolderPathA(0, 0x05, 0, 0, path);              // CSIDL_PERSONAL
    sprintf(name, "%s\\Mount&Blade Screenshots", path);
    CreateDirectoryA(name, 0);
    sprintf(name, "%s", out);
    sprintf(out, "%s\\Mount&Blade Screenshots\\%s", path, name);
}

// Both are external functions in the binary (called from other objects) but take their argument in ESI, a
// register convention cl only picks for internal-linkage functions: they are `static` here and this keeper
// (not in the binary; never linked into a relink) references them so the compiler emits them.
void keep_paths(char* a, char* b) { user_path(a); screenshots_path(b); }

// @ 0x4bac70
// Decodes the "\n" and "\r" escapes of a config/translation line in place, shortening the string.
void __fastcall unescape(tstring* s)
{
    int dst = 0;
    int src = 0;
    while (dst < s->len - 1) {
        char c = s->buf[src];
        if (c == '\\' && s->buf[src + 1] == 'n') {
            s->buf[dst] = '\n';
            src++;
            s->len--;
        } else if (s->buf[src] == '\\' && s->buf[src + 1] == 'r') {
            s->buf[dst] = '\r';
            src++;
            s->len--;
        } else {
            s->buf[dst] = c;
        }
        dst++;
        src++;
    }
    s->buf[dst] = s->buf[src];
    s->buf[dst + 1] = 0;
}

// ---- the application object's start-up probes (Core, ./core.h) -------------------------------------------------------
// @ 0x4bace0
void Core::update_view_rect()
{
    RECT r;
    GetWindowRect(m_hWnd, &r);
    if (m_bWindowed) {
        g_view_x = r.left + 4;
        g_view_y = (r.bottom - m_d3dsdBackBuffer.Height) - 4;
    } else {
        g_view_x = 0;
        g_view_y = 0;
    }
    g_view_w = m_d3dsdBackBuffer.Width;
    g_view_h = m_d3dsdBackBuffer.Height;
}

// @ 0x4bad70
int Core::init_d3d()
{
    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D) {
        return DisplayErrorMsg(0x82000001, MSGERR_APPMUSTEXIT);
    }
    m_d3dEnumeration.SetD3D(m_pD3D);
    m_d3dEnumeration.ConfirmDeviceCallback = ConfirmDeviceHelper;
    m_pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode);
    HRESULT hr = m_d3dEnumeration.Enumerate();
    if (FAILED(hr)) {
        if (m_pD3D) {
            m_pD3D->Release();
            m_pD3D = 0;
        }
        DisplayErrorMsg(hr, MSGERR_APPMUSTEXIT);
    }
    return hr;
}

// The depth/stencil format preference of FindBestFullscreenMode (the same ranking is inlined in d3dapp.cpp).
static inline int DepthStencilFormatRank(D3DFORMAT fmt)
{
    int r = 0;
    if (fmt == D3DFMT_D32) r = 10;
    else if (fmt == D3DFMT_D24X8) r = 9;
    else if (fmt == D3DFMT_D24S8) r = 8;
    else if (fmt == D3DFMT_D24X4S4) r = 7;
    else if (fmt == D3DFMT_D16) r = 6;
    else if (fmt == D3DFMT_D16_LOCKABLE) r = 5;
    else if (fmt == D3DFMT_D15S1) r = 4;
    return r;
}

// @ 0x4badf0
// The SDK's FindBestFullscreenMode with the configured display (width, height, bits) standing in for the desktop
// mode: picks the device combo, the closest listed mode, the depth format, the multisample type from the option
// block and the present interval (vsync) and stores them into the fullscreen half of the D3D settings.
bool Core::choose_display_mode(bool bRequireHAL, bool bRequireREF)
{
    D3DDISPLAYMODE want;
    D3DDISPLAYMODE best_want = { 0, 0, 0, (D3DFORMAT)0 };
    D3DAdapterInfo* pBestAdapterInfo = NULL;
    D3DDeviceInfo* pBestDeviceInfo = NULL;
    D3DDeviceCombo* pBestDeviceCombo = NULL;

    if (g_cfg.display_width > 0) {
        want.Width = g_cfg.display_width;
        want.Height = g_cfg.display_height;
        want.Format = (D3DFORMAT)((g_cfg.display_bits == 16) + 0x16);   // R5G6B5 (23) or X8R8G8B8 (22)
    } else {
        want.Width = mode.Width;
        want.Height = mode.Height;
        want.Format = mode.Format;
    }
    want.RefreshRate = mode.RefreshRate;

    for (UINT iai = 0; iai < m_d3dEnumeration.m_pAdapterInfoList->Count(); iai++) {
        D3DAdapterInfo* pAdapterInfo = (D3DAdapterInfo*)m_d3dEnumeration.m_pAdapterInfoList->GetPtr(iai);
        for (UINT idi = 0; idi < pAdapterInfo->pDeviceInfoList->Count(); idi++) {
            D3DDeviceInfo* pDeviceInfo = (D3DDeviceInfo*)pAdapterInfo->pDeviceInfoList->GetPtr(idi);
            if (bRequireHAL && pDeviceInfo->DevType != D3DDEVTYPE_HAL)
                continue;
            if (bRequireREF && pDeviceInfo->DevType != D3DDEVTYPE_REF)
                continue;
            for (UINT idc = 0; idc < pDeviceInfo->pDeviceComboList->Count(); idc++) {
                D3DDeviceCombo* pDeviceCombo = (D3DDeviceCombo*)pDeviceInfo->pDeviceComboList->GetPtr(idc);
                bool bAdapterMatchesBB = (pDeviceCombo->BackBufferFormat == pDeviceCombo->AdapterFormat);
                bool bAdapterMatchesDesktop = (pDeviceCombo->AdapterFormat == want.Format);
                if (pDeviceCombo->IsWindowed)
                    continue;
                if (pBestDeviceCombo == NULL ||
                    pBestDeviceCombo->DevType != D3DDEVTYPE_HAL && pDeviceInfo->DevType == D3DDEVTYPE_HAL ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && pBestDeviceCombo->AdapterFormat != want.Format && bAdapterMatchesDesktop ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesDesktop && bAdapterMatchesBB) {
                    best_want = want;
                    pBestAdapterInfo = pAdapterInfo;
                    pBestDeviceInfo = pDeviceInfo;
                    pBestDeviceCombo = pDeviceCombo;
                    if (pDeviceInfo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesDesktop && bAdapterMatchesBB)
                        goto EndFullscreenDeviceComboSearch;
                }
            }
        }
    }
EndFullscreenDeviceComboSearch:
    if (pBestDeviceCombo == NULL)
        return false;

    D3DDISPLAYMODE bestDisplayMode;
    bestDisplayMode.Width = 0;
    bestDisplayMode.Height = 0;
    bestDisplayMode.Format = D3DFMT_UNKNOWN;
    bestDisplayMode.RefreshRate = 0;
    for (UINT idm = 0; idm < pBestAdapterInfo->pDisplayModeList->Count(); idm++) {
        D3DDISPLAYMODE* pdm = (D3DDISPLAYMODE*)pBestAdapterInfo->pDisplayModeList->GetPtr(idm);
        if (pdm->Format != pBestDeviceCombo->AdapterFormat)
            continue;
        if (pdm->Width == best_want.Width &&
            pdm->Height == best_want.Height &&
            pdm->RefreshRate == best_want.RefreshRate) {
            bestDisplayMode = *pdm;
            break;
        } else if (pdm->Width == best_want.Width &&
                   pdm->Height == best_want.Height &&
                   pdm->RefreshRate > bestDisplayMode.RefreshRate) {
            bestDisplayMode = *pdm;
        } else if (pdm->Width == best_want.Width) {
            bestDisplayMode = *pdm;
        } else if (bestDisplayMode.Width == 0) {
            bestDisplayMode = *pdm;
        }
    }

    m_d3dSettings.pFullscreen_AdapterInfo = pBestAdapterInfo;
    m_d3dSettings.pFullscreen_DeviceInfo = pBestDeviceInfo;
    m_d3dSettings.Fullscreen_DisplayMode = bestDisplayMode;
    m_d3dSettings.pFullscreen_DeviceCombo = pBestDeviceCombo;
    m_d3dSettings.IsWindowed = false;
    if (m_d3dEnumeration.AppUsesDepthBuffer) {
        D3DFORMAT bestFmt = *(D3DFORMAT*)pBestDeviceCombo->pDepthStencilFormatList->GetPtr(0);
        for (UINT i = 1; i < pBestDeviceCombo->pDepthStencilFormatList->Count(); i++) {
            D3DFORMAT fmt = *(D3DFORMAT*)pBestDeviceCombo->pDepthStencilFormatList->GetPtr(i);
            if (DepthStencilFormatRank(fmt) > DepthStencilFormatRank(bestFmt))
                bestFmt = fmt;
        }
        m_d3dSettings.Fullscreen_DepthStencilBufferFormat = bestFmt;
    }
    if (g_cfg.antialiasing == 2) {
        m_d3dSettings.Fullscreen_MultisampleType = (D3DMULTISAMPLE_TYPE)g_cfg.antialiasing;
        m_d3dSettings.Fullscreen_MultisampleQuality = g_cfg.sample_quality;
    } else if (g_cfg.antialiasing == 4) {
        m_d3dSettings.Fullscreen_MultisampleType = (D3DMULTISAMPLE_TYPE)g_cfg.antialiasing;
        m_d3dSettings.Fullscreen_MultisampleQuality = g_cfg.sample_quality;
    } else {
        m_d3dSettings.Fullscreen_MultisampleType = *(D3DMULTISAMPLE_TYPE*)pBestDeviceCombo->pMultiSampleTypeList->GetPtr(0);
        m_d3dSettings.Fullscreen_MultisampleQuality = 0;
    }
    m_d3dSettings.Fullscreen_VertexProcessingType = *(VertexProcessingType*)pBestDeviceCombo->pVertexProcessingTypeList->GetPtr(0);
    if (g_cfg.force_vsync) {
        m_d3dSettings.Fullscreen_PresentInterval = 1;
        m_d3dSettings.Windowed_PresentInterval = 1;
    } else {
        m_d3dSettings.Fullscreen_PresentInterval = 0x80000000;
        m_d3dSettings.Windowed_PresentInterval = 0x80000000;
    }
    return true;
}

// ---- 0x4bb260 launcher list helpers --------------------------------------------------------------------------------
// @ 0x4bb260
// Moves the list box selection of the options page by `delta` entries (clamped) and remembers the new
// top-visible and selected entry in the game data object.
void set_list_scroll(int delta)
{
    HWND list = GetDlgItem(g_hwnd_page, 0x431);
    HWND combo = GetDlgItem(g_hwnd_page, 0x433);
    g_config->list_top = SendMessageA(list, 0x150, SendMessageA(list, 0x147, 0, 0), 0);
    int cur = SendMessageA(combo, 0x188, 0, 0);             // CB_GETCOUNT
    int count = SendMessageA(combo, 0x18b, 0, 0);
    cur += delta;
    if (cur < 0) cur = 0;
    else if (cur > count) cur = count;
    SendMessageA(combo, 0x186, (WPARAM)cur, 0);             // CB_SETCURSEL
    g_config->list_sel = SendMessageA(combo, 0x199, SendMessageA(combo, 0x188, 0, 0), 0);
}

// @ 0x4bb310
// Pumps the message queue until WM_QUIT, translating accelerators is not done here.
void pump_messages()
{
    MSG msg;
    HACCEL accel = LoadAcceleratorsA(0, MAKEINTRESOURCEA(0x71));
    msg.message = 0;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) break;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (accel) DestroyAcceleratorTable(accel);
}

// @ 0x4bb390
void set_debug_label()
{
    SetDlgItemTextA(g_hwnd_dlg, 0x4b3, "Debug");
    return;
}

// @ 0x4bb3b0
// Sets the two-dword progress range of the dialog's progress bar (PBM_SETRANGE32).
void set_progress_range(int low, int high)
{
    int range[2];
    range[0] = low;
    range[1] = high;
    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)range);
}

// @ 0x4bb3f0
// Re-renders the settings page `page` in the list box and sends the review message after a settings change.
// The list box lines are built as `name = value,value`; a quoted part is shown in red (green once the value
// differs from the stored setting). Afterwards the list height is compared against the window height.
void __cdecl refresh_settings_list(const tstring* page, char detailed)
{
    char text[0x1000];
    RECT item;
    int color;
    int key_end;
    int lineheight;
    int item_rgb[3];
    RECT window;
    char quote;

    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x434, 0, (LPARAM)&item);
    g_config->settings_text.erase_range(item.left, item.right);
    g_config->settings_text.insert(item.left, *page);

    tstring sel_text;
    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x43e, 0, (LPARAM)text);
    {
        tstring line(text);
        sel_text.append(line);
    }

    item_rgb[0] = 2;
    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x461, (WPARAM)item_rgb, (LPARAM)page->c_str());

    item.right = item.left + page->len;
    quote = '"';
    int first = page->find(quote, 0);
    int separated = 0;
    item_rgb[1] = 0x3c;
    item_rgb[2] = 0x40000000;
    color = detailed ? 0x00c80000 : 0;

    for (;;) {
        item.left = separated + item.right;
        item.top = first < 0 ? item.right : first + item.right;
        SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
        item_rgb[0] = color;
        SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);
        if (first < 0) {
            break;
        }
        separated = page->find(quote, first + 1) + 1;
        if (separated < 1) {
            first = -1;
        } else {
            item.left = item.right + first;
            item.top = item.right + separated;
            SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
            item_rgb[0] = (detailed ? 0x00c80000 : 0) + 0xc800;
            SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);
            first = page->find(quote, separated);
        }
    }

    if (!detailed) {
        first = page->find(tstring(g_eq_sep), 0);
        key_end = page->find(tstring(g_eq_sep), 0) + 3;
        while ((first = first + 3, first > 2)) {
            int k = page->find((const char)',', first);
            if (k < 0) {
                k = page->len;
            }
            if (key_end > 2) {
                int e = page->find((const char)',', key_end);
                if (e < 0) {
                    e = page->len;
                }
                long l = _atoi(page->substr(first, k).c_str());
                long r = _atoi(page->substr(key_end, e).c_str());
                if (l != r) {
                    goto mark;
                }
            } else {
            mark:
                item.left = item.right + first;
                item.top = item.right + k;
                SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
                item_rgb[0] = 0xff;
                SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);
            }
            first = page->find(tstring(g_eq_sep), first) + 3;
            key_end = page->find(tstring(g_eq_sep), key_end) + 3;
        }
    }

    WPARAM top = SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x436, 0, (LPARAM)item.left);
    GetWindowRect(GetDlgItem(g_hwnd_dlg, 0x4b0), &window);
    int count = window.bottom - window.top;
    lineheight = (top - 5) * 0x11;
    item_rgb[0] = 0;
    if ((float)count * g_line_pitch_ratio < (float)abs(lineheight - g_edit_value)) {
        SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x4de, 0, (LPARAM)item_rgb);
        g_edit_key = item_rgb[0];
        g_edit_value = lineheight;
    }

    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
    item_rgb[2] = detailed != 0;
    item_rgb[0] = 0x3c;
    item_rgb[1] = 1;
    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x444, 1, (LPARAM)item_rgb);

    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
    item.right = item.left;
    SendMessageA(GetDlgItem(g_hwnd_dlg, 0x4b0), 0x437, 0, (LPARAM)&item);
    sel_text.~tstring();
}

// @ 0x4bb9c0
// Grabs a front-buffer copy into a system-memory surface and writes it as an image file, optionally cropped to
// the view rectangle. Returns false when the grab or the save failed.
bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format)
{
    D3DDISPLAYMODE dmode;
    RECT rect;
    IDirect3DSurface9* surface = 0;

    g_d3d_device->GetDisplayMode(0, &dmode);
    g_d3d_device->CreateOffscreenPlainSurface(dmode.Width, dmode.Height, D3DFMT_A8R8G8B8,
                                              D3DPOOL_SYSTEMMEM, &surface, 0);
    bool ok = false;
    if (SUCCEEDED(g_d3d_device->GetFrontBufferData(0, surface))) {
        if (g_screenshot_cropped) {
            rect.left = g_view_x;
            rect.right = g_view_x + g_view_w;
            rect.top = g_view_y;
            rect.bottom = g_view_y + g_view_h;
            ok = SUCCEEDED(D3DXSaveSurfaceToFileA(file_name, format, surface, 0, &rect));
        } else {
            ok = SUCCEEDED(D3DXSaveSurfaceToFileA(file_name, format, surface, 0, 0));
        }
    }
    if (surface) {
        surface->Release();
    }
    return ok;
}

// @ 0x4bba90
// Splits a line into its runs of identifier characters: the leading run goes to `first`, the run after the first
// separator to `second`, anything later is dropped. The state machine walks 0 -> 1 (in a run) -> 2 (between
// runs) -> 3 (in the second run) -> 4 (after the second run).
void Config::split_line(const char* line, tstring* first, tstring* second)
{
    int i = 0;
    char ch;
    first->clear();
    second->clear();

    const char* p = line;
    for (;;) {
        ch = *p;
        p++;
        if (ch == 0) {
            break;
        }
    }
    int count = (int)(p - (line + 1));
    int state = 0;

    for (; i < count; i++) {
        ch = line[i];
        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '_') {
            if (state == 0) {
                state = 1;
                first->push_back(ch);
            } else if (state == 2) {
                state = 3;
                second->push_back(ch);
            } else if (state == 1) {
                first->push_back(ch);
            } else if (state == 3) {
                second->push_back(ch);
            }
        } else {
            if (state == 1) state = 2;
            if (state == 3) state = 4;
        }
    }
}

// @ 0x4bbb70
// The value of a "name = value" line, 1 when the value is empty.
int Config::value_as_int(const tstring& s)
{
    int r = 1;
    if (s.len != 0) {
        r = _atoi(s.c_str());
    }
    return r;
}

// ---- the option block -----------------------------------------------------------------------------------------------
// @ 0x4bbba0
// Applies one "name = value" line. Both strings are lowered first, then every known name stores the value
// parsed from the right-hand string. Names with no handler leave the configuration untouched.
void Config::set(tstring& key, tstring& value)
{
    key.to_lower();
    value.to_lower();

    if (key.equals("start_windowed")) { start_windowed = value_as_int(value); return; }
    if (key.equals("skip_tutorial")) { skip_tutorial = value_as_int(value); return; }
    if (key.equals("enable_edit_mode")) { enable_edit_mode = value_as_int(value); return; }
    if (key.equals("force_single_threading")) { force_single_threading = value_as_int(value); return; }
    if (key.equals("debug_mode")) { debug_mode = value_as_int(value); return; }
    if (key.equals("cheat_mode")) { cheat_mode = value_as_int(value); return; }
    if (key.equals("enable_blood")) { enable_blood = value_as_int(value); return; }
    if (key.equals("disable_music")) { disable_music = value_as_int(value); return; }
    if (key.equals("disable_sound")) { disable_sound = value_as_int(value); return; }
    if (key.equals("show_framerate")) { show_framerate = value_as_int(value); return; }
    if (key.equals("max_framerate")) { max_framerate = value_as_int(value); return; }
    if (key.equals("use_pixel_shaders")) { use_pixel_shaders = value_as_int(value); return; }
    if (key.equals("use_vertex_shaders")) { use_vertex_shaders = value_as_int(value); return; }
    if (key.equals("fake_reflections")) { fake_reflections = value_as_int(value); return; }
    if (key.equals("disable_frequency_variation")) { disable_frequency_variation = value_as_int(value); return; }
    if (key.equals("texture_detail")) {
        int d = value_as_int(value);
        texture_detail = d;
        if (d < 0) texture_detail = 0;
        else if (d > 101) texture_detail = 101;
        return;
    }
    if (key.equals("render_buffer_size")) { render_buffer_size = value_as_int(value); return; }
    if (key.equals("first_time")) { first_time = value_as_int(value); return; }
    if (key.equals("display_width")) { display_width = value_as_int(value); return; }
    if (key.equals("display_height")) { display_height = value_as_int(value); return; }
    if (key.equals("display_bits")) { display_bits = value_as_int(value); return; }
    if (key.equals("antialiasing")) { antialiasing = value_as_int(value); return; }
    if (key.equals("sample_quality")) { sample_quality = value_as_int(value); return; }
    if (key.equals("force_vsync")) { force_vsync = value_as_int(value); return; }
    if (key.equals("shadowmap_quality")) {
        int q = value_as_int(value);
        shadowmap_quality = q;
        unk6c = q;
    }
}

// @ 0x4bbf70
// Reads the file line by line; lines starting with '#' are comments. Each line is read with getc into a
// 256-byte buffer (at most 250 characters).
void Config::load_file(FILE* f)
{
    tstring first;
    tstring second;
    char line[0x100];

    while (!(f->_flag & _IOEOF)) {
        int n = 0;
        do {
            char c = (char)getc(f);
            line[n] = c;
            n++;
            if ((f->_flag & _IOEOF) || c == '\n') break;
        } while (n < 250);
        line[n] = 0;
        if (line[0] != '#') {
            split_line(line, &first, &second);
            set(first, second);
        }
    }
}

// @ 0x4bc020
// Configuration defaults (a void member, so the original keeps ECX as `this` and EAX as the zero register).
void Config::init_defaults()
{
    first_time = 1;
    show_framerate = 0;
    max_framerate = 100;
    texture_detail = 50;
    use_pixel_shaders = 1;
    use_vertex_shaders = 1;
    disable_sound = 0;
    disable_music = 0;
    enable_edit_mode = 0;
    force_single_threading = 0;
    debug_mode = 0;
    cheat_mode = 0;
    enable_blood = 1;
    unk44 = 0;
    disable_frequency_variation = 0;
    render_buffer_size = 15;
    fake_reflections = 1;
    start_windowed = 0;
    display_width = 0;
    display_height = 0;
    display_bits = 32;
    antialiasing = 1;
    sample_quality = 0;
    force_vsync = 0;
    unk64 = 1;
    shadowmap_quality = 1;
    unk6c = 1;
}

// @ 0x4bc090
// The triple-buffering choice also picks the render buffer size.
void Config::set_triple_buffering(int t)
{
    int v = t;

    first_time = t;
    switch (v) {
    case 5: disable_music = 9999999; break;
    case 4: disable_music = 40; break;
    case 3: disable_music = 20; break;
    case 2: disable_music = 10; break;
    case 1: disable_music = 5; break;
    case 0: disable_music = 0; break;
    }
}

// @ 0x4bc100
// The texture-quality choice picks the anisotropic level.
void Config::set_texture_quality(int q)
{
    int v = q;

    texture_detail = q;
    switch (v) {
    case 5: disable_sound = 9999999; break;
    case 4: disable_sound = 10; break;
    case 3: disable_sound = 5; break;
    case 2: disable_sound = 3; break;
    case 1: disable_sound = 1; break;
    case 0: disable_sound = 0; break;
    }
}

// ---- the software cursor ----------------------------------------------------------------------------------------------
// @ 0x4bc170
// The cursor starts at the origin with the fixed radius 0.05 and the two state bytes cleared.
Cursor::Cursor()
{
    *(Vec4*)&pos_x = Vec4(0.0f, 0.0f, 0.0f);
    *(Vec4*)&pos2_x = Vec4(0.0f, 0.0f, 0.0f);
    radius = 0.05f;
    unk25 = 0;
    moving = 0;
    unk28 = 0;
}

// @ 0x4bc1f0
// Reads the cursor position, the target position and the radius from the settings, keeping any missing value.
void Cursor::read(Settings* src)
{
    double v;
    if (src->get_float("radius", &v) == 0) radius = (float)v;
    if (src->get_float("pos_x", &v) == 0) pos_x = (float)v;
    if (src->get_float("pos_y", &v) == 0) pos_y = (float)v;
    if (src->get_float("pos_z", &v) == 0) pos_z = (float)v;
    if (src->get_float("pos2_x", &v) == 0) pos2_x = (float)v;
    if (src->get_float("pos2_y", &v) == 0) pos2_y = (float)v;
    if (src->get_float("pos2_z", &v) == 0) pos2_z = (float)v;
}

// @ 0x4bc2d0
// Clears the input state and rebuilds both view matrices with a unit last column.
DisplayState::DisplayState()
{
    Matrix tmp;
    Matrix* m;
    unk618 = 0;
    unk74c = 0;
    mat1 = *make_matrix(&tmp);
    m = &mat1;
    m->m[3] = 0;
    m->m[7] = 0;
    m->m[11] = 0;
    m->m[15] = 1.0f;
    mat2 = *make_matrix(&tmp);
    m = &mat2;
    m->m[3] = 0;
    m->m[7] = 0;
    m->m[11] = 0;
    m->m[15] = 1.0f;
}

// ---- the DirectInput device set -------------------------------------------------------------------------------------
// @ 0x4bc350
// Re-applies the exclusive, background cooperative level to the keyboard and records which level took.
void __fastcall apply_input_settings(InputSet* self)
{
    typedef int (__stdcall *SetupFn)(void*, int, int, void*, int, int, int);
    self->exclusive_ok = 0;
    self->background_ok = 0;
    if (g_dinput_enabled != 0) {
        HRESULT hr = (*(SetupFn*)(*(int*)self->unk74c + 0x28))((void*)self->unk74c, 0, 1, (void*)self->unk750, 2, 3, 0x4b);
        if (hr >= 0) {
            self->exclusive_ok = 1;
            return;
        }
        hr = (*(SetupFn*)(*(int*)self->unk74c + 0x28))((void*)self->unk74c, 0, 1, (void*)self->unk750, 2, 3, 0x34324644);
        if (hr >= 0) {
            self->background_ok = 1;
        }
    }
}

// @ 0x4bc3d0
// Unacquires and releases the keyboard and the mouse, then the DirectInput object itself.
void __fastcall release_input_devices(InputSet* self)
{
    if (self->keyboard != 0) self->keyboard->Unacquire();
    if (self->mouse != 0) self->mouse->Unacquire();
    if (self->keyboard != 0) {
        self->keyboard->Release();
        self->keyboard = 0;
    }
    if (self->mouse != 0) {
        self->mouse->Release();
        self->mouse = 0;
    }
    if (self->dinput != 0) {
        self->dinput->Release();
        self->dinput = 0;
    }
}

// @ 0x4bc430
// Creates DirectInput, the keyboard and the buffered mouse, then starts them.
int InputSet::init_devices(HWND hwnd)
{
    DIPROPDWORD buffer_size;
    HRESULT hr;

    release_input_devices(this);
    hr = DirectInput8Create(GetModuleHandleA(0), 0x0800, IID_IDirectInput8A, (void**)&dinput, 0);
    if (hr >= 0) {
        hr = dinput->CreateDevice(GUID_SysKeyboard, &keyboard, 0);
        if (hr >= 0) {
            hr = keyboard->SetDataFormat(&c_dfDIKeyboard);
            if (hr >= 0) {
                hr = keyboard->SetCooperativeLevel(hwnd, 0x16);
                if (hr >= 0) {
                    keyboard->Acquire();
                    hr = dinput->CreateDevice(GUID_SysMouse, &mouse, 0);
                    if (hr >= 0) {
                        hr = mouse->SetDataFormat(&c_dfDIMouse);
                        if (hr >= 0) {
                            hr = mouse->SetCooperativeLevel(hwnd, 6);
                            if (hr >= 0) {
                                buffer_size.diph.dwSize = 0x14;
                                buffer_size.diph.dwHeaderSize = 0x10;
                                buffer_size.diph.dwObj = 0;
                                buffer_size.diph.dwHow = 0;
                                buffer_size.dwData = 0x10;
                                hr = mouse->SetProperty(DIPROP_BUFFERSIZE, &buffer_size.diph);
                                if (hr >= 0) {
                                    mouse->Acquire();
                                    hr = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

// ---- the render batch's buffer table ----------------------------------------------------------------------------------
// @ 0x4bc540
void buffer_pair::release()
{
    dev_buffer& a = bufs[0];
    if (a.type == 1)
        a.ib->Release();
    else if (a.count != 0 && a.vb)
        a.vb->Release();
    a.generation++;
    a.loaded = 0;
    dev_buffer& b = bufs[1];
    if (b.type == 1)
        b.ib->Release();
    else if (b.count != 0 && b.vb)
        b.vb->Release();
    b.generation++;
    b.loaded = 0;
}

// @ 0x4bc5b0
void render_batch::init_formats()
{
    cur_vf_a = -1;
    cur_ib = -1;
    cur_vf_b = -1;
    int n = (g_cfg.render_buffer_size << 18) / 0x44;
    for (int i = 0; i < 10; i++)
        vf[i / 2].bufs[i % 2].type = 0;
    dev_buffer* p = &vf[0].bufs[0];
    p[0].fvf = 0x02000013; p[0].elem_size = 0x24;
    p[1].fvf = 0x02000013; p[1].elem_size = 0x24;
    p[2].fvf = 0x02000053; p[2].elem_size = 0x28;
    p[3].fvf = 0x02000053; p[3].elem_size = 0x28;
    p[4].fvf = 0x02000153; p[4].elem_size = 0x40;
    p[5].fvf = 0x02000153; p[5].elem_size = 0x40;
    p[6].fvf = 0x01001013; p[6].elem_size = 0x44;
    p[7].fvf = 0x01001013; p[7].elem_size = 0x44;
    p[8].fvf = 0x01001113; p[8].elem_size = 0x5c;
    p[9].fvf = 0x01001113; p[9].elem_size = 0x5c;
    {
        tstring s("vdt_regular_ffp");
        p[0].name = s;
        p[1].name = s;
    }
    {
        tstring s("vdt_regular");
        p[2].name = s;
        p[3].name = s;
    }
    {
        tstring s("vdt_normal_map");
        p[4].name = s;
        p[5].name = s;
    }
    {
        tstring s("vdt_skinning");
        p[6].name = s;
        p[7].name = s;
    }
    {
        tstring s("vdt_normal_map_skinning");
        p[8].name = s;
        p[9].name = s;
    }
    ib.bufs[0].type = 1;
    ib.bufs[1].type = 1;
    ib.bufs[0].count = n * 3 * 8;
    ib.bufs[1].count = n / 2 * 8;
}

// @ 0x4bc830
int render_batch::init_decls(int* a0, int* a1, int* a2, int* a3, int* a4,
                             int* a5, int* a6, int* a7, int* a8, int* a9)
{
    if (g_dinput_enabled) {
        int c1 = *a1 ? *a1 : 16000;
        int c0 = *a0;
        vf[0].bufs[1].count = c1;
        vf[0].bufs[0].count = c0;
        int c3 = *a3 ? *a3 : 35000;
        int c2 = *a2 ? *a2 : 290000;
        vf[1].bufs[1].count = c3;
        vf[1].bufs[0].count = c2;
        int c5 = *a5;
        int c4 = *a4 ? *a4 : 90000;
        vf[3].bufs[1].count = c5;
        vf[3].bufs[0].count = c4;
        int c7 = *a7 ? *a7 : 12000;
        int c6 = *a6 ? *a6 : 120000;
        vf[2].bufs[0].count = c6;
        vf[2].bufs[1].count = c7;
    } else {
        int c9 = *a9 ? *a9 : 30000;
        int c8 = *a8 ? *a8 : 290000;
        vf[0].bufs[0].count = c8;
        vf[0].bufs[1].count = c9;
    }

    D3DVERTEXELEMENT9 ffp[5];
    D3DVERTEXELEMENT9 regular[6];
    D3DVERTEXELEMENT9 skinning[7];
    D3DVERTEXELEMENT9 normalmap[8];
    D3DVERTEXELEMENT9 normalmap_skin[9];

    ffp[0].Stream = 0; ffp[0].Offset = 0; ffp[0].Type = D3DDECLTYPE_FLOAT3;
    ffp[0].Method = 0; ffp[0].Usage = D3DDECLUSAGE_POSITION; ffp[0].UsageIndex = 0;
    ffp[1].Stream = 0; ffp[1].Offset = 0xc; ffp[1].Type = D3DDECLTYPE_FLOAT3;
    ffp[1].Method = 0; ffp[1].Usage = D3DDECLUSAGE_NORMAL; ffp[1].UsageIndex = 0;
    ffp[2].Stream = 0; ffp[2].Offset = 0x18; ffp[2].Type = D3DDECLTYPE_D3DCOLOR;
    ffp[2].Method = 0; ffp[2].Usage = D3DDECLUSAGE_COLOR; ffp[2].UsageIndex = 0;
    ffp[3].Stream = 0; ffp[3].Offset = 0x1c; ffp[3].Type = D3DDECLTYPE_FLOAT2;
    ffp[3].Method = 0; ffp[3].Usage = D3DDECLUSAGE_TEXCOORD; ffp[3].UsageIndex = 0;
    ffp[4].Stream = 0xff; ffp[4].Offset = 0; ffp[4].Type = D3DDECLTYPE_UNUSED;
    ffp[4].Method = 0; ffp[4].Usage = 0; ffp[4].UsageIndex = 0;

    regular[0].Stream = 0; regular[0].Offset = 0; regular[0].Type = D3DDECLTYPE_FLOAT3;
    regular[0].Method = 0; regular[0].Usage = D3DDECLUSAGE_POSITION; regular[0].UsageIndex = 0;
    regular[1].Stream = 0; regular[1].Offset = 0xc; regular[1].Type = D3DDECLTYPE_FLOAT3;
    regular[1].Method = 0; regular[1].Usage = D3DDECLUSAGE_NORMAL; regular[1].UsageIndex = 0;
    regular[2].Stream = 0; regular[2].Offset = 0x18; regular[2].Type = D3DDECLTYPE_D3DCOLOR;
    regular[2].Method = 0; regular[2].Usage = D3DDECLUSAGE_COLOR; regular[2].UsageIndex = 0;
    regular[3].Stream = 0; regular[3].Offset = 0x1c; regular[3].Type = D3DDECLTYPE_D3DCOLOR;
    regular[3].Method = 0; regular[3].Usage = D3DDECLUSAGE_COLOR; regular[3].UsageIndex = 1;
    regular[4].Stream = 0; regular[4].Offset = 0x20; regular[4].Type = D3DDECLTYPE_FLOAT2;
    regular[4].Method = 0; regular[4].Usage = D3DDECLUSAGE_TEXCOORD; regular[4].UsageIndex = 0;
    regular[5].Stream = 0xff; regular[5].Offset = 0; regular[5].Type = D3DDECLTYPE_UNUSED;
    regular[5].Method = 0; regular[5].Usage = 0; regular[5].UsageIndex = 0;

    normalmap[0].Stream = 0; normalmap[0].Offset = 0; normalmap[0].Type = D3DDECLTYPE_FLOAT3;
    normalmap[0].Method = 0; normalmap[0].Usage = D3DDECLUSAGE_POSITION; normalmap[0].UsageIndex = 0;
    normalmap[1].Stream = 0; normalmap[1].Offset = 0xc; normalmap[1].Type = D3DDECLTYPE_FLOAT3;
    normalmap[1].Method = 0; normalmap[1].Usage = D3DDECLUSAGE_NORMAL; normalmap[1].UsageIndex = 0;
    normalmap[2].Stream = 0; normalmap[2].Offset = 0x18; normalmap[2].Type = D3DDECLTYPE_D3DCOLOR;
    normalmap[2].Method = 0; normalmap[2].Usage = D3DDECLUSAGE_COLOR; normalmap[2].UsageIndex = 0;
    normalmap[3].Stream = 0; normalmap[3].Offset = 0x1c; normalmap[3].Type = D3DDECLTYPE_D3DCOLOR;
    normalmap[3].Method = 0; normalmap[3].Usage = D3DDECLUSAGE_COLOR; normalmap[3].UsageIndex = 1;
    normalmap[4].Stream = 0; normalmap[4].Offset = 0x20; normalmap[4].Type = D3DDECLTYPE_FLOAT2;
    normalmap[4].Method = 0; normalmap[4].Usage = D3DDECLUSAGE_TEXCOORD; normalmap[4].UsageIndex = 0;
    normalmap[5].Stream = 0; normalmap[5].Offset = 0x28; normalmap[5].Type = D3DDECLTYPE_FLOAT3;
    normalmap[5].Method = 0; normalmap[5].Usage = D3DDECLUSAGE_TANGENT; normalmap[5].UsageIndex = 0;
    normalmap[6].Stream = 0; normalmap[6].Offset = 0x34; normalmap[6].Type = D3DDECLTYPE_FLOAT3;
    normalmap[6].Method = 0; normalmap[6].Usage = D3DDECLUSAGE_BINORMAL; normalmap[6].UsageIndex = 0;
    normalmap[7].Stream = 0xff; normalmap[7].Offset = 0; normalmap[7].Type = D3DDECLTYPE_UNUSED;
    normalmap[7].Method = 0; normalmap[7].Usage = 0; normalmap[7].UsageIndex = 0;

    skinning[0].Stream = 0; skinning[0].Offset = 0; skinning[0].Type = D3DDECLTYPE_FLOAT3;
    skinning[0].Method = 0; skinning[0].Usage = D3DDECLUSAGE_POSITION; skinning[0].UsageIndex = 0;
    skinning[1].Stream = 0; skinning[1].Offset = 0xc; skinning[1].Type = D3DDECLTYPE_FLOAT3;
    skinning[1].Method = 0; skinning[1].Usage = D3DDECLUSAGE_NORMAL; skinning[1].UsageIndex = 0;
    skinning[2].Stream = 0; skinning[2].Offset = 0x18; skinning[2].Type = D3DDECLTYPE_D3DCOLOR;
    skinning[2].Method = 0; skinning[2].Usage = D3DDECLUSAGE_COLOR; skinning[2].UsageIndex = 0;
    skinning[3].Stream = 0; skinning[3].Offset = 0x1c; skinning[3].Type = D3DDECLTYPE_FLOAT2;
    skinning[3].Method = 0; skinning[3].Usage = D3DDECLUSAGE_TEXCOORD; skinning[3].UsageIndex = 0;
    skinning[4].Stream = 0; skinning[4].Offset = 0x24; skinning[4].Type = D3DDECLTYPE_UBYTE4;
    skinning[4].Method = 0; skinning[4].Usage = D3DDECLUSAGE_BLENDWEIGHT; skinning[4].UsageIndex = 0;
    skinning[5].Stream = 0; skinning[5].Offset = 0x34; skinning[5].Type = D3DDECLTYPE_UBYTE4;
    skinning[5].Method = 0; skinning[5].Usage = D3DDECLUSAGE_BLENDINDICES; skinning[5].UsageIndex = 0;
    skinning[6].Stream = 0xff; skinning[6].Offset = 0; skinning[6].Type = D3DDECLTYPE_UNUSED;
    skinning[6].Method = 0; skinning[6].Usage = 0; skinning[6].UsageIndex = 0;

    normalmap_skin[0].Stream = 0; normalmap_skin[0].Offset = 0; normalmap_skin[0].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[0].Method = 0; normalmap_skin[0].Usage = D3DDECLUSAGE_POSITION; normalmap_skin[0].UsageIndex = 0;
    normalmap_skin[1].Stream = 0; normalmap_skin[1].Offset = 0xc; normalmap_skin[1].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[1].Method = 0; normalmap_skin[1].Usage = D3DDECLUSAGE_NORMAL; normalmap_skin[1].UsageIndex = 0;
    normalmap_skin[2].Stream = 0; normalmap_skin[2].Offset = 0x18; normalmap_skin[2].Type = D3DDECLTYPE_D3DCOLOR;
    normalmap_skin[2].Method = 0; normalmap_skin[2].Usage = D3DDECLUSAGE_COLOR; normalmap_skin[2].UsageIndex = 0;
    normalmap_skin[3].Stream = 0; normalmap_skin[3].Offset = 0x1c; normalmap_skin[3].Type = D3DDECLTYPE_FLOAT2;
    normalmap_skin[3].Method = 0; normalmap_skin[3].Usage = D3DDECLUSAGE_TEXCOORD; normalmap_skin[3].UsageIndex = 0;
    normalmap_skin[4].Stream = 0; normalmap_skin[4].Offset = 0x24; normalmap_skin[4].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[4].Method = 0; normalmap_skin[4].Usage = D3DDECLUSAGE_TANGENT; normalmap_skin[4].UsageIndex = 0;
    normalmap_skin[5].Stream = 0; normalmap_skin[5].Offset = 0x30; normalmap_skin[5].Type = D3DDECLTYPE_FLOAT3;
    normalmap_skin[5].Method = 0; normalmap_skin[5].Usage = D3DDECLUSAGE_BINORMAL; normalmap_skin[5].UsageIndex = 0;
    normalmap_skin[6].Stream = 0; normalmap_skin[6].Offset = 0x3c; normalmap_skin[6].Type = D3DDECLTYPE_UBYTE4;
    normalmap_skin[6].Method = 0; normalmap_skin[6].Usage = D3DDECLUSAGE_BLENDWEIGHT; normalmap_skin[6].UsageIndex = 0;
    normalmap_skin[7].Stream = 0; normalmap_skin[7].Offset = 0x4c; normalmap_skin[7].Type = D3DDECLTYPE_UBYTE4;
    normalmap_skin[7].Method = 0; normalmap_skin[7].Usage = D3DDECLUSAGE_BLENDINDICES; normalmap_skin[7].UsageIndex = 0;
    normalmap_skin[8].Stream = 0xff; normalmap_skin[8].Offset = 0; normalmap_skin[8].Type = D3DDECLTYPE_UNUSED;
    normalmap_skin[8].Method = 0; normalmap_skin[8].Usage = 0; normalmap_skin[8].UsageIndex = 0;

    if (g_d3d_device->CreateVertexDeclaration(ffp, &decls[0]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_d3d_device->CreateVertexDeclaration(regular, &decls[1]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_d3d_device->CreateVertexDeclaration(normalmap, &decls[2]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_d3d_device->CreateVertexDeclaration(skinning, &decls[3]) < 0)
        mb_error("Unable to create vertex declaration.");
    if (g_d3d_device->CreateVertexDeclaration(normalmap_skin, &decls[4]) < 0)
        mb_error("Unable to create vertex declaration.");

    vf[0].bufs[0].decl = decls[0];
    vf[0].bufs[1].decl = decls[0];
    vf[1].bufs[0].decl = decls[1];
    vf[1].bufs[1].decl = decls[1];
    vf[2].bufs[0].decl = decls[2];
    vf[2].bufs[1].decl = decls[2];
    vf[3].bufs[0].decl = decls[3];
    vf[3].bufs[1].decl = decls[3];
    vf[4].bufs[0].decl = decls[4];
    vf[4].bufs[1].decl = decls[4];

    return 0;
}

// @ 0x4bcfb0
void render_batch::init_shaders()
{
    buffer_pair* p = vf;
    for (int i = 0; i < 5; i++, p++)
        p->restore();
    ib.restore();

    counter++;
    {
        tstring name("render_to_depth_shader");
        sh0 = g_resmgr.get_shader((rgl::string&)name);
    }
    {
        tstring name("render_to_depth_skin_shader");
        sh1 = g_resmgr.get_shader((rgl::string&)name);
    }
    {
        tstring name("render_character_shadow");
        sh2 = g_resmgr.get_shader((rgl::string&)name);
    }
    {
        tstring name("render_character_shadow_with_skin");
        sh3 = g_resmgr.get_shader((rgl::string&)name);
    }
}

// @ 0x4bd0c0
int render_batch::release_decls()
{
    if (decls[0]) { decls[0]->Release(); decls[0] = 0; }
    if (decls[1]) { decls[1]->Release(); decls[1] = 0; }
    if (decls[2]) { decls[2]->Release(); decls[2] = 0; }
    if (decls[3]) { decls[3]->Release(); decls[3] = 0; }
    if (decls[4]) { decls[4]->Release(); decls[4] = 0; }
    return 0;
}

// @ 0x4bd120
font_details::font_details()
{
    a = 0; b = 0; c = 0; d = 0; e = 0; f = 0; g = 0;
}

// ---- ambient particle ring ----------------------------------------------------------------------------------------------
// @ 0x4bd170
void weather::update(void* pass, const float* depth)
{
    rgl::matrix m = identity_pos();
    float alpha = 0.2f - fade;
    m.o.x = 0.002f;
    m.o.z = *depth;
    for (int i = 0; i < 32; i++) {
        int idx = (i + cur + 32) % 32;
        if (life[idx] < 10.0f) {
            alpha += delta[idx];
            m.o.y = alpha;
            unsigned col = colors[idx];
            int a = (int)((1.0f - life[idx] * 0.1f) * 256.0f);
            if (a < 0)
                a = 0;
            else if (a >= 255)
                a = 254;
            int b = (int)((unsigned)(255 - a) * 2 / 3);
            if (b < 0)
                b = 0;
            else if (b >= 255)
                b = 254;
            first[idx].color = (a << 24) | (col & 0xffffff);
            second[idx].color = ((b | 0xffffff00) << 8 | b) << 8 | b;
            first[idx].render(pass, &m);
            second[idx].render(pass, &m);
        }
    }
}

// ---- screen ray ---------------------------------------------------------------------------------------------------------------
// @ 0x4bd2f0
// The original reads stack slots it never writes: the far point's ox/oy when the camera is not in the ortho mode
// and the w of the position vector (both points copy it).  The locals are gathered in one 0x28-byte record so the
// uninitialised reads hit the same frame offsets as the original (frame 0x30: w at +0x1c, ox at +0x20, oy at +0x24);
// the equivalence harness then sees the same stack garbage on both sides.
void camera_ray::screen_to_world(vector4* near_pt, vector4* far_pt)
{
    struct Frame {
        float px, hp, z, hz;
        float v[4];
        float ox;
    } f;
    float oy;
    struct V2 { float x, y; } d = { g_mouse_x - vx, g_mouse_y - vy };
    f.px = d.x / vw;
    float sy = d.y / vh;
    f.px = (zb8 - zb4) * f.px + zb4;
    float py = (zbc - zc0) * sy + zc0;
    f.z = -zc4;
    if (ortho) {
        float k = 1.0f / zc4;
        f.ox = k * zc8 * f.px;
        oy = k * zc8 * py;
    }
    float z2 = -zc8;
    f.v[0] = f.z * view.u.x + py * view.f.x + f.px * view.s.x + view.o.x;
    f.v[1] = f.z * view.u.y + py * view.f.y + f.px * view.s.y + view.o.y;
    f.v[2] = f.z * view.u.z + py * view.f.z + f.px * view.s.z + view.o.z;
    near_pt->x = f.v[0];
    near_pt->y = f.v[1];
    near_pt->z = f.v[2];
    near_pt->w = f.v[3];
    f.v[0] = z2 * view.u.x + oy * view.f.x + f.ox * view.s.x + view.o.x;
    f.v[1] = z2 * view.u.y + oy * view.f.y + f.ox * view.s.y + view.o.y;
    f.v[2] = z2 * view.u.z + oy * view.f.z + f.ox * view.s.z + view.o.z;
    far_pt->x = f.v[0];
    far_pt->y = f.v[1];
    far_pt->z = f.v[2];
    far_pt->w = f.v[3];
}

// ---- ini file -------------------------------------------------------------------------------------------------------------------
// @ 0x4bd4d0
void ini_file::parse_line(const char* line, tstring& key, tstring& value)
{
    int i = 0;
    key.clear();
    value.clear();
    int len = (int)strlen(line);
    int state = 0;
    for (; i < len; i++) {
        char c = line[i];
        if (c <= ' ' || c == '=') {
            if (state == 1) {
                state = 2;
            } else if (state == 3) {
                state = 4;
            }
        } else if (state == 0) {
            state = 1;
            key.push_back(c);
        } else if (state == 2) {
            state = 3;
            value.push_back(c);
        } else if (state == 1) {
            key.push_back(c);
        } else if (state == 3) {
            value.push_back(c);
        }
    }
}

// @ 0x4bd5a0
int ini_file::find(const tstring& key)
{
    int found = -1;
    int i = 0;
    do {
        if (i >= count)
            break;
        if (entries[i].key.equals(key))
            found = i;
        i++;
    } while (found == -1);
    return found;
}

// @ 0x4bd5e0
float ini_file::get_float(const tstring& key)
{
    float ret = 0.0f;
    int idx = find(key);
    if (idx >= 0)
        ret = (float)atof(entries[idx].value.c_str());
    return ret;
}

// @ 0x4bd630
void ini_file::load(FILE* f)
{
    tstring key;
    tstring value;
    count = 0;
    while (!feof(f)) {
        char line[256];
        int i = 0;
        do {
            char c = (char)fgetc(f);
            line[i] = c;
            i++;
            if (feof(f) || c == '\n')
                break;
        } while (i < 250);
        line[i] = '\0';
        if (line[0] != '#') {
            if (count < 0x1000) {
                parse_line(line, key, value);
                key.to_lower();
                entries[count].key = key;
                entries[count].value = value;
                count++;
            } else {
                mb_warning("Too many lines in Ini file. Ignoring...");
            }
        }
    }
}

// @ 0x4bd740
void ini_file::write(FILE* f, const tstring& key, const tstring& value)
{
    fputs(key.c_str(), f);
    fputs("|", f);
    fputs(value.c_str(), f);
    fputs("\n", f);
}

// ---- script blocks and trigger records ----------------------------------------------------------------------------------
// @ 0x4bd7a0
Block::Block(const Block& o) : name()
{
    count = o.count;
    data = 0;
    loaded = 0;
    if (count) {
        data = new Elem[count];
        for (int i = 0; i < count; i++)
            data[i] = o.data[i];
    }
}

// @ 0x4bd830
void trigger_rec::read(stream_base& s, const char* name, int idx)
{
    s.r_float(&f0);
    s.r_float(&f4);
    s.r_float(&f8);
    cond.read(&s);
    cons.read(&s);
    char buf[512];
    _snprintf(buf, 0x200, "%s trigger no: %d conditions", name, idx);
    {
        tstring tmp(buf);
        cond.name = tmp;
    }
    _snprintf(buf, 0x200, "%s trigger no: %d consequences", name, idx);
    {
        tstring tmp(buf);
        cons.name = tmp;
    }
}

// @ 0x4bd910
void trigger_entry::read(stream_base& s, const char* name, int idx)
{
    s.r_float(&f0);
    block.read(&s);
    char buf[512];
    _snprintf(buf, 0x200, "%s trigger no: %d", name, idx);
    {
        tstring tmp(buf);
        block.name = tmp;
    }
}

// ---- particle system / presentation / sound records -----------------------------------------------------------------------
// Slot offsets of the stream (see ../../rgl/stream.h): +0x38 r_float, +0x3c r_bool, +0x40 r_uint, +0x44 r_u64,
// +0x48 r_i64, +0x4c r_int, +0x50 r_byte, +0x54 r_tstr.
#define RF(a, p) (a).r_float((float*)(p))
#define RI(a, p) (a).r_int((int*)(p))
#define RS(a, p) (a).r_tstr((rgl::string*)(p))

// @ 0x4bd990
void psys_record::load(stream_base& a)
{
    char* o = (char*)this;
    RS(a, &name);
    RI(a, &f10);
    RS(a, &tex);
    RF(a, o + 0x24);
    RF(a, o + 0x30);
    RF(a, o + 0x34);
    RF(a, o + 0x38);
    RF(a, o + 0x3c);
    RF(a, o + 0x40);
    int* p = (int*)(o + 0x44);
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(a, p), p + 1); p += 2;
    RF(RF(RF(a, p), p + 1), p + 2); p += 3;
    RF(RF(RF(a, p), p + 1), p + 2);
    RF(a, o + 0xb4);
    RF(a, o + 0xb8);
    *(float*)(o + 0xb8) *= 0.017453292f;
    RF(a, o + 0xbc);
}

// @ 0x4bdb60
void loaded_psys_table::load_used()
{
    extern const char* g_psys_names[];                  // 0x5a9c60 (11 names to 0x5a9c8c)
    const char* const* nm = g_psys_names;
    int* out = used;
    for (; nm < g_psys_names + 11; ++nm) {
        int i = 0;
        int found = -1;
        int off = 0;
        while (found == -1) {
            if (i >= count)
                break;
            if (((tstring*)((char*)records + off))->equals(*nm))
                found = i;
            ++i;
            off += 0xc0;
        }
        if (found == -1)
            ((stream_base&)g_log).w_cstr("WARNING: UNABLE TO MAP Particle System CODE: ").w_cstr(*nm).w_cstr(g_eol);
        *out++ = found;
    }
}

// @ 0x4bdc00
void* psys_record::read(stream_base& a, char* o)
{
    RF(a, o + 0x0c);
    tstring tmp;
    RS(a, &tmp);
    *(void**)o = g_resmgr.get_skeleton_anim((rgl::string&)tmp);
    stream_base& s = RI(RI(a, o + 0x10), o + 0x14).r_uint((unsigned*)(o + 0x08)).r_uint((unsigned*)(o + 0x18));
    int* q = (int*)(o + 0x2c);
    RF(RF(RF(s, q), q + 1), q + 2);
    RF(s, o + 0x3c);
    *(float*)(o + 0x1c) = (*(unsigned*)(o + 0x18) & 0xff) ? (float)(*(unsigned*)(o + 0x18) & 0xff) * 0.003921568859f : -1.0f;
    *(float*)(o + 0x20) = *(unsigned char*)(o + 0x19) ? (float)*(unsigned char*)(o + 0x19) * 0.003921568859f : -1.0f;
    *(float*)(o + 0x24) = *(unsigned char*)(o + 0x1a) ? (float)*(unsigned char*)(o + 0x1a) * 0.003921568859f : -1.0f;
    *(float*)(o + 0x28) = *(unsigned char*)(o + 0x1b) ? (float)*(unsigned char*)(o + 0x1b) * 0.003921568859f : -1.0f;
    return &a;
}

// @ 0x4bdd50
void psys_record::load_extras(stream_base& a)
{
    int i;
    RI(a, (char*)this + 0x00);
    if (*(int*)((char*)this + 0x00) < 0)
        *(int*)((char*)this + 0x00) = 0;
    else if (*(int*)((char*)this + 0x00) > 0x40)
        *(int*)((char*)this + 0x00) = 0x40;
    RI(a, (char*)this + 0x04);
    RI(a, (char*)this + 0x08);
    RI(a, (char*)this + 0x20);
    RI(a, (char*)this + 0x14);
    int* n = (int*)((char*)this + 0x24);
    RI(a, n);
    if (*n > 8)
        mb_error("Too many item overrides: %d (max: %d)", *n, 8);
    for (i = 0; i < *n; i++)
        RI(a, (char*)this + 0x28 + 4 * i);
}

// @ 0x4bddf0
void prsnt_record::load_extras(stream_base& a)
{
    char* o = (char*)this;
    int tmp[4];
    RS(a, o);
    RI(a, o + 0x10);
    RF(a, o + 0x14);
    RF(a, o + 0x18);
    RF(a, o + 0x1c);
    int* p = (int*)(o + 0x20);
    RF(RF(RF(a, p), p + 1), p + 2);
    RF(RF(RF(a, &tmp[0]), &tmp[1]), &tmp[2]);
    p = (int*)(o + 0x30);
    RF(RF(RF(a, p), p + 1), p + 2);
    RF(a, o + 0x40).r_uint((unsigned*)(o + 0x44));
}

// @ 0x4bdec0
void loaded_prsnt_table::load_used()
{
    extern const char* g_prsnt_names[];                 // 0x5a9c8c (3 names to 0x5a9c98)
    for (int k = 0; k < 3; ++k) {
        int found = -1;
        int i = 0;
        const char* const* nm = g_prsnt_names + k;
        char* rec = (char*)records;
        do {
            if (i >= count)
                break;
            if (((tstring*)rec)->equals(*nm))
                found = i;
            rec += 0x24;
            ++i;
        } while (found == -1);
        if (found == -1)
            ((stream_base&)g_log).w_cstr("WARNING: UNABLE TO MAP GAME PRESENTATION CODE: ").w_cstr(*nm).w_cstr(g_eol);
        used[k] = found;
    }
}

// @ 0x4bdf60
void prsnt_record::save(stream_base& a)
{
    char* o = (char*)this;
    RS(a, &name);
    RI(a, &f10);
    RS(a, &tex);
    RF(a, o + 0x24);
    RF(a, o + 0x28);
    RF(a, o + 0x2c);
    RF(a, o + 0x30);
    RF(a, o + 0x34);
    RF(a, o + 0x38);
    RF(a, o + 0x3c);
    RF(a, o + 0x40);
    RF(a, o + 0x44);
}

// @ 0x4bdff0
void sound_def::save(stream_base& a)
{
    RS(a, &name);
    RS(a, &tex);
    tex.underscores_to_spaces();
    a.r_u64((unsigned __int64*)((char*)this + 0x20));
    RI(a, (char*)this + 0x28);
    RI(a, (char*)this + 0x2c);
    RI(a, (char*)this + 0x30);
    int* p = (int*)((char*)this + 0x48);
    for (int i = 0; i < 6; i++, p += 4) {
        RI(a, p - 2);
        if (p[-2] >= 0) {
            RI(a, p - 1);
            RI(a, p);
            RI(a, p + 1);
        }
    }
    f34 = 0;
    *(int*)((char*)this + 0x38) = 0;
    *(int*)((char*)this + 0x3c) = 0;
}

// @ 0x4be0a0
void music_player::stop(int i)
{
    FSOUND_Stream_Close(*(FMOD_STREAM*)&w[8 + i]);
    *(FMOD_STREAM*)&w[8 + i] = 0;
    w[i] = 0;
    w[12 + i] = -1;
}

// @ 0x4be0e0
void music_player::play(int rec, int* slot)
{
    char path[1024];
    int index = *slot;
    w[10 + index] = rec;
    music_res* r = (music_res*)((char*)this + 0x20 * w[10 + index]);
    const char* s = r->name;
    if (r->flags & 0x10000000) {
        const char* root = *(const char**)((char*)this + 0x2480);
        if (!root) root = (const char*)this + 0x2480;
        if (!s) s = (const char*)r + 0x1c;
        sprintf(path, "%sMusic\\%s", root, s);
    } else {
        if (!s) s = (const char*)r + 0x1c;
        sprintf(path, "music\\%s", s);
    }
    *(FMOD_STREAM*)&w[8 + index] = FSOUND_Stream_Open(path, (r->flags & 0x40) ? 0x1000002 : 0x1000000, 0, 0);
    w[index] = 1;
    w[2 + index] = 0;
    w[14 + index] = 10000;
    w[26 + index] = 0;
    start[w[10 + index]] = (float)g_ticks[0] * 1e-05f;
}

// @ 0x4be200
void sound_record::load(stream_base& a)
{
    RS(a, this);
    RI(a, (char*)this + 0x10);
    RI(a, &nsamples);
    if (nsamples > 0x20)
        mb_error("more than %d samples per sound.", 0x20);
    for (int i = 0; i < nsamples; i++)
        RI(a, (char*)this + 0x14 + 4 * i);
}

// @ 0x4be270
void loaded_snd_table::load_used()
{
    extern const char* g_snd_names[];                   // 0x5a9ca8 (0x7d names to 0x5a9e9c)
    for (int k = 0; k < 0x7d; ++k) {
        int found = -1;
        int i = 0;
        const char* const* nm = g_snd_names + k;
        char* rec = (char*)records;
        do {
            if (i >= count)
                break;
            if (((tstring*)rec)->equals(*nm))
                found = i;
            rec += 0x98;
            ++i;
        } while (found == -1);
        if (found == -1)
            ((stream_base&)g_log).w_cstr("WARNING: UNABLE TO MAP SOUND CODE: ").w_cstr(*nm).w_cstr(g_eol);
        used[k] = found;
    }
}

// ---- menu items and script records ------------------------------------------------------------------------------------------
// @ 0x4be310
void menu_item::read(stream_base* s, const tstring* menu)
{
    char buf[0x200];
    RS(*s, &f20);
    cond.read(s);
    _snprintf(buf, 0x200, "menu %s item %s condition", menu->c_str(), f20.c_str());
    {
        tstring tmp(buf);
        cond.name = tmp;
    }
    RS(*s, &name);
    name.underscores_to_spaces();
    conseq.read(s);
    _snprintf(buf, 0x200, "menu %s item %s consequence", menu->c_str(), f20.c_str());
    {
        tstring tmp(buf);
        conseq.name = tmp;
    }
    RS(*s, &f10);
    f10.underscores_to_spaces();
}

// @ 0x4be420
menu_item::menu_item()
{
}

// @ 0x4be470
int rec34::read(stream_base* s)
{
    char buf[0x200];
    RS(*s, &name);
    s->r_float(&f14);
    body.read(s);
    _snprintf(buf, 0x200, "script: %s", name.c_str());
    body.name = buf;
    return f14 >= 0.0f;
}

// @ 0x4be520
// A real constructor, not string data: 0x4be520..0x4be553 is `mov eax,ecx; mov [eax+4],0; mov [eax],0; ret` with a
// WinLicense marker pair around the two stores (jmp +0x10 over a 16-byte 'WL  ' record, twice); the 'WL  '
// bytes at 0x4be524/0x4be530/0x4be543/0x4be54e are those records. Called by the static initializers at
// 0x547f10, 0x547f30 and 0x548010 for the tables at 0xa75eb0, 0xa75f2c and 0x8ed4f58.
rec_table::rec_table()
{
    count = 0;
    records = 0;
}

// @ 0x4be560
void rec34_table::load_used()
{
    extern const char* const g_script_names[];          // 0x5a9ea0 (the 28 script-code names)
    for (int k = 0; k < 28; ++k) {
        int found = -1;
        int i = 0;
        for (; found == -1 && i < count; ++i) {
            if (records[i].name.equals(g_script_names[k]))
                found = i;
        }
        if (found == -1)
            ((stream_base&)g_log).w_cstr("WARNING: UNABLE TO MAP GAME SCRIPT CODE: ").w_cstr(g_script_names[k]).w_cstr(g_eol);
        used[k] = found;
    }
}

// @ 0x4be600
void rec20::read(stream_base* s)
{
    RS(*s, &name);
    RI(*s, &f1c);
    tstring tmp;
    RS(*s, &tmp);
    mat = g_resmgr.get_material((rgl::string&)tmp);
    s->r_float(&f18);
    mat2 = 0;
    tstring tmp2;
    RS(*s, &tmp2);
    if (tmp2.differs("none"))
        mat2 = g_resmgr.get_material((rgl::string&)tmp2);
}

// @ 0x4be6b0
clip_table::clip_table()
{
    clip_slot_raw* p = slots;
    int n = 13;
    do {
        tstring_ctor((tstring*)&p->name);                       // 0x42bbd0
        p->v14 = 0;
        ++p;
    } while (--n);
    v04 = -100;
    v08 = -100;
    v00 = 0;
    v144 = 0;
    v148 = 0;
}

// @ 0x4be6f0
clip_table::~clip_table()
{
    clip_slot_raw* p = slots + 13;
    int n = 13;
    do {
        --p;
        if (p->v14)
            operator delete[](p->v14);
        tstring_dtor((tstring*)&p->name);                       // 0x42bc40
    } while (--n);
}

// @ 0x4be720
clip_slot::clip_slot()
{
    tstring_ctor((tstring*)&name);                              // 0x42bbd0
}
