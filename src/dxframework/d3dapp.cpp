//-----------------------------------------------------------------------------
// File: D3DApp.cpp
//
// Desc: Application class for the Direct3D samples framework library
//       (DirectX 9 SDK), as modified by TaleWorlds for Mount & Blade 1.011.
//       Region 0x401000-0x4035cf.
//-----------------------------------------------------------------------------
#define _MBCS   // project setting: multi-byte character set (TCHAR = char, _tcs* = _mbs*)
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <new>             // declares operator delete[]: without it cl 13.10 frees arrays with plain operator delete (the original calls 0x53570d)
#include <mbstring.h>
#include <d3d9.h>
#include "./dxutil.h"
#include "./d3denumeration.h"
#include "./d3dsettings.h"
#include "./d3dapp.h"
#include "./d3dutil.h"

//-----------------------------------------------------------------------------
// Global access to the app (needed for the global WndProc())
//-----------------------------------------------------------------------------
static CD3DApplication* g_pD3DApp = NULL;   // 0x5d6f40
static bool s_bFatalErrorReported = false;  // 0x5d6f48 (DisplayErrorMsg's latch; also read by the inlined copies)

// Default window size, set by the game before Create() (M&B; provisional names)
extern int g_iWindowWidth;                  // 0x5a9fe4
extern int g_iWindowHeight;                 // 0x5a9fe8

//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Static msg handler which passes messages to the application class.
//-----------------------------------------------------------------------------
// @ 0x401000
LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return g_pD3DApp->MsgProc( hWnd, uMsg, wParam, lParam );
}

//-----------------------------------------------------------------------------
// Name: ConfirmDeviceHelper()
// Desc: Static function used by D3DEnumeration
//       (M&B: external linkage, the game stores it in ConfirmDeviceCallback)
//-----------------------------------------------------------------------------
// @ 0x401010
bool ConfirmDeviceHelper( D3DCAPS9* pCaps, VertexProcessingType vertexProcessingType,
                          D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat )
{
    DWORD dwBehavior;

    if (vertexProcessingType == SOFTWARE_VP)
        dwBehavior = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    else if (vertexProcessingType == MIXED_VP)
        dwBehavior = D3DCREATE_MIXED_VERTEXPROCESSING;
    else if (vertexProcessingType == HARDWARE_VP)
        dwBehavior = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else if (vertexProcessingType == PURE_HARDWARE_VP)
        dwBehavior = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
    else
        dwBehavior = 0; // TODO: throw exception

    return SUCCEEDED( g_pD3DApp->ConfirmDevice( pCaps, dwBehavior, adapterFormat, backBufferFormat ) );
}

//-----------------------------------------------------------------------------
// Name: Create()
// Desc: M&B: only registers the window class and creates the window; the
//       device part of the SDK's Create() is Create3DEnvironment() (0x403010).
//-----------------------------------------------------------------------------
// @ 0x401070
HRESULT CD3DApplication::Create( HINSTANCE hInstance )
{
    if (hInstance) {
        // Create the app's window if one was not already created by the derived class
        if (m_hWnd == NULL) {
            m_dwWindowStyle = m_bStartFullscreen ? WS_POPUP : WS_OVERLAPPEDWINDOW;
            RECT rc;
            if (g_iWindowWidth > 0) {
                SetRect(&rc, 0, 0, g_iWindowWidth, g_iWindowHeight);
            } else {
                // Default: the largest window that fits the work area
                int fx = GetSystemMetrics(SM_CXSIZEFRAME) * 4;
                int fy = GetSystemMetrics(SM_CYMENU) + GetSystemMetrics(SM_CYSIZEFRAME) * 4 + GetSystemMetrics(SM_CYCAPTION);
                int w = GetSystemMetrics(SM_CXMAXIMIZED) - fx;
                SetRect(&rc, 0, 0, w, GetSystemMetrics(SM_CYMAXIMIZED) - fy);
            }
            AdjustWindowRect(&rc, m_dwWindowStyle, TRUE);

            // Register the windows class
            WNDCLASSEXW wc;
            memset(&wc, 0, sizeof(wc));
            wc.cbSize = sizeof(wc);
            wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WndProc;
            wc.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_MAIN_ICON));
            wc.hIconSm = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_MAIN_ICON));
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
            wc.lpszClassName = L"MB Window";
            RegisterClassExW(&wc);

            // Create the render window, centred on the screen
            int sx = GetSystemMetrics(SM_CXMAXIMIZED);
            int sy = GetSystemMetrics(SM_CYMAXIMIZED);
            int x = (rc.left - rc.right + sx) / 2;
            int y = (rc.top - rc.bottom + sy) / 2;
            m_hWnd = CreateWindowExW(0, L"MB Window", (LPCWSTR)m_strWindowTitle, m_dwWindowStyle, x, y,
                                     rc.right - rc.left, rc.bottom - rc.top, NULL,
                                     LoadMenuA(hInstance, MAKEINTRESOURCEA(IDR_MENU)), hInstance, 0);
            UpdateWindow(m_hWnd);
        }

        // The focus window can be a specified to be a different window than the
        // device window.  If not, use the device window as the focus window.
        if (m_hWndFocus == NULL)
            m_hWndFocus = m_hWnd;

        // Save window properties
        m_dwWindowStyle = GetWindowLongA(m_hWnd, GWL_STYLE);
        GetWindowRect(m_hWnd, &m_rcWindowBounds);
        GetClientRect(m_hWnd, &m_rcWindowClient);
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Reset3DEnvironment()
// Desc: Reset the 3D environment
//-----------------------------------------------------------------------------
// @ 0x401290
HRESULT CD3DApplication::Reset3DEnvironment()
{
    HRESULT hr;

    // Release all vidmem objects
    if (m_bDeviceObjectsRestored) {
        m_bDeviceObjectsRestored = false;
        InvalidateDeviceObjects();
    }
    // M&B: bail out while the device is lost
    if (m_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICELOST) {
        m_bDeviceLost = true;
        return S_OK;
    }
    // Reset the device
    if (FAILED(hr = m_pd3dDevice->Reset(&m_d3dpp)))
        return hr;

    // Store render target surface desc
    LPDIRECT3DSURFACE9 pBackBuffer;
    m_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
    pBackBuffer->GetDesc(&m_d3dsdBackBuffer);
    pBackBuffer->Release();

    // Set up the fullscreen cursor
    GetClassLongA(m_hWnd, GCL_HCURSOR);     // (the SDK's HCURSOR use is compiled out)
    if (m_bShowCursorWhenFullscreen && !m_bWindowed)
        m_pd3dDevice->ShowCursor(TRUE);
    else
        m_pd3dDevice->ShowCursor(FALSE);

    // Confine cursor to fullscreen window
    if (m_bClipCursorWhenFullscreen) {
        RECT rcWindow;
        if (!m_bWindowed) {
            GetWindowRect(m_hWnd, &rcWindow);
            ClipCursor(&rcWindow);
        } else
            ClipCursor(NULL);
    }

    // Initialize the app's device-dependent objects
    hr = RestoreDeviceObjects();
    if (FAILED(hr)) {
        InvalidateDeviceObjects();
        return hr;
    }
    m_bDeviceObjectsRestored = true;

    // If the app is paused, trigger the rendering of the current frame
    if (false == m_bFrameMoving) {
        m_bSingleStep = true;
        DXUtil_Timer(TIMER_START);
        DXUtil_Timer(TIMER_STOP);
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: AdjustWindowForChange()
// Desc: Prepare the window for a possible change between windowed mode and
//       fullscreen mode.
//-----------------------------------------------------------------------------
// @ 0x4013e0
HRESULT CD3DApplication::AdjustWindowForChange()
{
    if (m_bWindowed) {
        // Set windowed-mode style
        m_dwWindowStyle = WS_OVERLAPPEDWINDOW;
        SetWindowLongA(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
        if (m_hMenu != NULL) {
            SetMenu(m_hWnd, m_hMenu);
            m_hMenu = NULL;
            return S_OK;
        }
    } else {
        // Set fullscreen-mode style
        m_dwWindowStyle = WS_POPUP;
        SetWindowLongA(m_hWnd, GWL_STYLE, WS_POPUP);
        if (m_hMenu == NULL) {
            m_hMenu = GetMenu(m_hWnd);
            SetMenu(m_hWnd, NULL);
        }
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Pause()
// Desc: Called in to toggle the pause state of the app.
//-----------------------------------------------------------------------------
// @ 0x401480
void CD3DApplication::Pause( bool bPause )
{
    static DWORD dwAppPausedCount = 0L;     // 0x5d6f44

    dwAppPausedCount += ( bPause ? +1 : -1 );
    m_bActive         = ( dwAppPausedCount ? false : true );

    // Handle the first pause request (of many, nestable pause requests)
    if( bPause && ( 1 == dwAppPausedCount ) )
    {
        // Stop the scene from animating
        if( !m_bFrameMoving )
            return;
        DXUtil_Timer( TIMER_STOP );
    }

    if( 0 == dwAppPausedCount )
    {
        // Restart the timers
        if( m_bFrameMoving )
            DXUtil_Timer( TIMER_START );
    }
}

//-----------------------------------------------------------------------------
// Name: DisplayErrorMsg()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
// @ 0x4014f0
HRESULT CD3DApplication::DisplayErrorMsg( HRESULT hr, DWORD dwType )
{
    TCHAR strMsg[512];

    // If a fatal error message has already been reported, the app
    // is already shutting down, so don't show more error messages.
    if (s_bFatalErrorReported)
        return hr;

    switch (hr) {
    case E_OUTOFMEMORY:
        _tcscpy(strMsg, _T("Not enough memory."));
        break;
    case D3DAPPERR_NODIRECT3D:
        _tcscpy(strMsg, _T("Could not initialize Direct3D. You may\n")
                        _T("want to check that the latest version of\n")
                        _T("DirectX is correctly installed on your\n")
                        _T("system.  Also make sure that this program\n")
                        _T("was compiled with header files that match\n")
                        _T("the installed DirectX DLLs."));
        break;
    case D3DAPPERR_NOCOMPATIBLEDEVICES:
        _tcscpy(strMsg, _T("Could not find any compatible Direct3D\n")
                        _T("devices."));
        break;
    case D3DAPPERR_NOWINDOWABLEDEVICES:
        _tcscpy(strMsg, _T("This sample cannot run in a desktop\n")
                        _T("window with the current display settings.\n")
                        _T("Please change your desktop settings to a\n")
                        _T("16- or 32-bit display mode and re-run this\n")
                        _T("sample."));
        break;
    case D3DAPPERR_NOWINDOWEDHAL:
        _tcscpy(strMsg, _T("Your Direct3D hardware accelerator cannot\n")
                        _T("render into a window.\n")
                        _T("Press F2 while the app is running to see a\n")
                        _T("list of available devices and modes."));
        break;
    case D3DAPPERR_HALNOTCOMPATIBLE:
        _tcscpy(strMsg, _T("This sample requires functionality that is\n")
                        _T("not available on your Direct3D hardware\n")
                        _T("accelerator."));
        break;
    case D3DAPPERR_NOHARDWAREDEVICE:
        _tcscpy(strMsg, _T("No hardware-accelerated Direct3D devices\n")
                        _T("were found."));
        break;
    case D3DAPPERR_NODESKTOPHAL:
        _tcscpy(strMsg, _T("Your Direct3D hardware accelerator cannot\n")
                        _T("render into a window with the current\n")
                        _T("desktop display settings.\n")
                        _T("Press F2 while the app is running to see a\n")
                        _T("list of available devices and modes."));
        break;
    case D3DAPPERR_NOHALTHISMODE:
        _tcscpy(strMsg, _T("This sample requires functionality that is\n")
                        _T("not available on your Direct3D hardware\n")
                        _T("accelerator with the current desktop display\n")
                        _T("settings.\n")
                        _T("Press F2 while the app is running to see a\n")
                        _T("list of available devices and modes."));
        break;
    case D3DAPPERR_MEDIANOTFOUND:
    case __HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ):
        _tcscpy(strMsg, _T("Could not load required media."));
        break;
    case D3DAPPERR_RESETFAILED:
        _tcscpy(strMsg, _T("Could not reset the Direct3D device."));
        break;
    case D3DAPPERR_NONZEROREFCOUNT:
        _tcscpy(strMsg, _T("A D3D object has a non-zero reference\n")
                        _T("count (meaning things were not properly\n")
                        _T("cleaned up)."));
        break;
    case D3DAPPERR_NULLREFDEVICE:
        _tcscpy(strMsg, _T("Warning: Nothing will be rendered.\n")
                        _T("The reference rendering device was selected, but your\n")
                        _T("computer only has a reduced-functionality reference device\n")
                        _T("installed.  Install the DirectX SDK to get the full\n")
                        _T("reference device.\n"));
        break;
    case D3DERR_OUTOFVIDEOMEMORY:
        _tcscpy(strMsg, _T("Not enough video memory."));
        break;
    default:
        _tcscpy(strMsg, _T("Generic application error. Enable\n")
                        _T("debug output for detailed information."));
        break;
    case D3DERR_DRIVERINTERNALERROR:    // M&B addition: always fatal
        _tcscpy(strMsg, _T("A serious problem occured inside the display driver."));
        dwType = MSGERR_APPMUSTEXIT;
        break;
    }

    if (MSGERR_APPMUSTEXIT == dwType) {
        s_bFatalErrorReported = true;
        _tcscat(strMsg, _T("\n\nThis sample will now exit."));
        MessageBoxA(NULL, strMsg, m_strWindowTitle, MB_ICONERROR | MB_OK);

        // Close the window, which shuts down the app
        if (m_hWnd)
            SendMessageA(m_hWnd, WM_CLOSE, 0, 0);
    } else {
        if (MSGWARN_SWITCHEDTOREF == dwType)
            _tcscat(strMsg, _T("\n\nSwitching to the reference rasterizer,\n")
                            _T("a software device that implements the entire\n")
                            _T("Direct3D feature set, but runs very slowly."));
        MessageBoxA(NULL, strMsg, m_strWindowTitle, MB_ICONWARNING | MB_OK);
    }
    return hr;
}

//-----------------------------------------------------------------------------
// 0x401800, 0x401830, 0x401870: compiler-generated helpers that the linker placed in
// this object's code (COMDATs; see MIGRATION.md). They are reproduced here so that
// the bytes stay verified; none of them is part of the SDK source.
//-----------------------------------------------------------------------------

// `vector constructor iterator' (??_H): calls a thiscall constructor on n elements.
// @ 0x401800
void __stdcall vector_constructor_iterator( void* p, unsigned size, int n, void (__fastcall* ctor)(void*) )
{
    while (--n >= 0) {
        ctor(p);
        p = (char*)p + size;
    }
}

// `vector destructor iterator' (??_I): calls a thiscall destructor on n elements, last first.
// @ 0x401830
void __stdcall vector_destructor_iterator( void* p, unsigned size, int n, void (__fastcall* dtor)(void*) )
{
    p = (char*)p + size * n;
    while (--n >= 0) {
        p = (char*)p - size;
        dtor(p);
    }
}

// `vector deleting destructor' (??_E) of a 0x34-byte game record (two strings and an
// array pointer); its only caller is 0x4d3731. The record type and the delete[] that
// instantiates the helper belong to game code; this TU only has to contain the COMDAT.
struct MBStringRec {                // 0x10 bytes (provisional)
    int pad0;
    char* p;
    int pad8[2];
    ~MBStringRec();                 // 0x42bc40
};
struct MBRecord34 {                 // 0x34 bytes (provisional)
    MBStringRec a;                  // +0x00
    MBStringRec b;                  // +0x10
    int gap[4];                     // +0x20
    char* arr;                      // +0x30
    ~MBRecord34() { if (arr) delete[] arr; }
};
// @ 0x401870
void MBRecord34_DeleteArray( MBRecord34* r ) { delete[] r; }   // instantiates ??_EMBRecord34

//-----------------------------------------------------------------------------
// Name: CD3DApplication()
// Desc: Constructor
//-----------------------------------------------------------------------------
// @ 0x401910
CD3DApplication::CD3DApplication()
{
    g_pD3DApp           = this;

    m_bQuitRequested    = false;
    m_fMinFrameTime     = 0.0f;
    m_pD3D              = NULL;
    m_pd3dDevice        = NULL;
    m_hWnd              = NULL;
    m_hWndFocus         = NULL;
    m_hMenu             = NULL;
    m_bWindowed         = true;
    m_bDeviceLost       = false;
    m_bMinimized        = false;
    m_bMaximized        = false;
    m_bIgnoreSizeChange = false;
    m_bDeviceObjectsInited = false;
    m_bDeviceObjectsRestored = false;
    m_dwCreateFlags     = 0;

    m_bFrameMoving      = true;
    m_bSingleStep       = false;
    m_fTime             = 0.0f;
    m_fElapsedTime      = 0.0f;
    m_fFPS              = 0.0f;
    m_strDeviceStats[0] = _T('\0');
    m_strFrameStats[0]  = _T('\0');
    m_dw334             = 0;

    m_dwCreationWidth   = 400;
    m_dwCreationHeight  = 300;
    m_bShowCursorWhenFullscreen = false;
    m_bStartFullscreen  = false;

    Pause( true ); // Pause until we're ready to render

    // When m_bClipCursorWhenFullscreen is true, the cursor is limited to
    // the device window when the app goes fullscreen.
    m_bClipCursorWhenFullscreen = true;
}

//-----------------------------------------------------------------------------
// Name: ConfirmDevice()
// Desc: Default: every device is acceptable (inline in the SDK header)
//-----------------------------------------------------------------------------
// @ 0x401a20
HRESULT CD3DApplication::ConfirmDevice( D3DCAPS9*, DWORD, D3DFORMAT, D3DFORMAT )
{
    return S_OK;
}

// (0x401a30 is the scalar deleting destructor generated for this destructor)
// @ 0x401a30
CD3DApplication::~CD3DApplication()
{
}

//-----------------------------------------------------------------------------
// Name: FindBestWindowedMode()
// Desc: Sets up m_d3dSettings with best available windowed mode, subject to
//       the bRequireHAL and bRequireREF constraints.  Returns false if no such
//       mode can be found.
//-----------------------------------------------------------------------------
// M&B ranks the depth/stencil formats instead of taking the first one.
static inline int DepthStencilFormatRank( D3DFORMAT fmt )
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

// @ 0x401a60
bool CD3DApplication::FindBestWindowedMode( bool bRequireHAL, bool bRequireREF )
{
    // Get display mode of primary adapter (which is assumed to be where the window
    // will appear)
    D3DDISPLAYMODE primaryDesktopDisplayMode;
    m_pD3D->GetAdapterDisplayMode(0, &primaryDesktopDisplayMode);

    D3DAdapterInfo* pBestAdapterInfo = NULL;
    D3DDeviceInfo* pBestDeviceInfo = NULL;
    D3DDeviceCombo* pBestDeviceCombo = NULL;

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
                if (!pDeviceCombo->IsWindowed)
                    continue;
                if (pDeviceCombo->AdapterFormat != primaryDesktopDisplayMode.Format)
                    continue;
                // If we haven't found a compatible DeviceCombo yet, or if this set
                // is better (because it's a HAL, and/or because formats match better),
                // save it
                if (pBestDeviceCombo == NULL ||
                    pBestDeviceCombo->DevType != D3DDEVTYPE_HAL && pDeviceCombo->DevType == D3DDEVTYPE_HAL ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesBB) {
                    pBestAdapterInfo = pAdapterInfo;
                    pBestDeviceInfo = pDeviceInfo;
                    pBestDeviceCombo = pDeviceCombo;
                    if (pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesBB) {
                        // This windowed device combo looks great -- take it
                        goto EndWindowedDeviceComboSearch;
                    }
                    // Otherwise keep looking for a better windowed device combo
                }
            }
        }
    }
EndWindowedDeviceComboSearch:
    if (pBestDeviceCombo == NULL)
        return false;

    m_d3dSettings.pWindowed_AdapterInfo = pBestAdapterInfo;
    m_d3dSettings.pWindowed_DeviceInfo = pBestDeviceInfo;
    m_d3dSettings.Windowed_DisplayMode = primaryDesktopDisplayMode;
    m_d3dSettings.Windowed_Width = m_rcWindowClient.right - m_rcWindowClient.left;
    m_d3dSettings.pWindowed_DeviceCombo = pBestDeviceCombo;
    m_d3dSettings.IsWindowed = true;
    m_d3dSettings.Windowed_Height = m_rcWindowClient.bottom - m_rcWindowClient.top;
    if (m_d3dEnumeration.AppUsesDepthBuffer) {
        D3DFORMAT bestFmt = *(D3DFORMAT*)pBestDeviceCombo->pDepthStencilFormatList->GetPtr(0);
        for (UINT i = 1; i < pBestDeviceCombo->pDepthStencilFormatList->Count(); i++) {
            D3DFORMAT fmt = *(D3DFORMAT*)pBestDeviceCombo->pDepthStencilFormatList->GetPtr(i);
            if (DepthStencilFormatRank(fmt) > DepthStencilFormatRank(bestFmt))
                bestFmt = fmt;
        }
        m_d3dSettings.Windowed_DepthStencilBufferFormat = bestFmt;
    }
    m_d3dSettings.Windowed_MultisampleType = *(D3DMULTISAMPLE_TYPE*)pBestDeviceCombo->pMultiSampleTypeList->GetPtr(0);
    m_d3dSettings.Windowed_MultisampleQuality = 0;
    m_d3dSettings.Windowed_VertexProcessingType = *(VertexProcessingType*)pBestDeviceCombo->pVertexProcessingTypeList->GetPtr(0);
    m_d3dSettings.Windowed_PresentInterval = *(UINT*)pBestDeviceCombo->pPresentIntervalList->GetPtr(0);
    return true;
}

//-----------------------------------------------------------------------------
// Name: FindBestFullscreenMode()
// Desc: Sets up m_d3dSettings with best available fullscreen mode, subject to
//       the bRequireHAL and bRequireREF constraints.  Returns false if no such
//       mode can be found.
//-----------------------------------------------------------------------------
// @ 0x401d40
bool CD3DApplication::FindBestFullscreenMode( bool bRequireHAL, bool bRequireREF )
{
    // For fullscreen, default to first HAL DeviceCombo that supports the current desktop
    // display mode, or any display mode if HAL is not compatible with the desktop mode, or
    // non-HAL if no HAL is available
    D3DDISPLAYMODE adapterDesktopDisplayMode;
    D3DDISPLAYMODE bestAdapterDesktopDisplayMode = { 0, 0, 0, (D3DFORMAT)0 };
    D3DAdapterInfo* pBestAdapterInfo = NULL;
    D3DDeviceInfo* pBestDeviceInfo = NULL;
    D3DDeviceCombo* pBestDeviceCombo = NULL;

    for (UINT iai = 0; iai < m_d3dEnumeration.m_pAdapterInfoList->Count(); iai++) {
        D3DAdapterInfo* pAdapterInfo = (D3DAdapterInfo*)m_d3dEnumeration.m_pAdapterInfoList->GetPtr(iai);
        m_pD3D->GetAdapterDisplayMode(pAdapterInfo->AdapterOrdinal, &adapterDesktopDisplayMode);
        for (UINT idi = 0; idi < pAdapterInfo->pDeviceInfoList->Count(); idi++) {
            D3DDeviceInfo* pDeviceInfo = (D3DDeviceInfo*)pAdapterInfo->pDeviceInfoList->GetPtr(idi);
            if (bRequireHAL && pDeviceInfo->DevType != D3DDEVTYPE_HAL)
                continue;
            if (bRequireREF && pDeviceInfo->DevType != D3DDEVTYPE_REF)
                continue;
            for (UINT idc = 0; idc < pDeviceInfo->pDeviceComboList->Count(); idc++) {
                D3DDeviceCombo* pDeviceCombo = (D3DDeviceCombo*)pDeviceInfo->pDeviceComboList->GetPtr(idc);
                bool bAdapterMatchesBB = (pDeviceCombo->BackBufferFormat == pDeviceCombo->AdapterFormat);
                bool bAdapterMatchesDesktop = (pDeviceCombo->AdapterFormat == adapterDesktopDisplayMode.Format);
                if (pDeviceCombo->IsWindowed)
                    continue;
                // If we haven't found a compatible set yet, or if this set
                // is better (because it's a HAL, and/or because formats match better),
                // save it
                if (pBestDeviceCombo == NULL ||
                    pBestDeviceCombo->DevType != D3DDEVTYPE_HAL && pDeviceInfo->DevType == D3DDEVTYPE_HAL ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && pBestDeviceCombo->AdapterFormat != adapterDesktopDisplayMode.Format && bAdapterMatchesDesktop ||
                    pDeviceCombo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesDesktop && bAdapterMatchesBB) {
                    bestAdapterDesktopDisplayMode = adapterDesktopDisplayMode;
                    pBestAdapterInfo = pAdapterInfo;
                    pBestDeviceInfo = pDeviceInfo;
                    pBestDeviceCombo = pDeviceCombo;
                    if (pDeviceInfo->DevType == D3DDEVTYPE_HAL && bAdapterMatchesDesktop && bAdapterMatchesBB) {
                        // This fullscreen device combo looks great -- take it
                        goto EndFullscreenDeviceComboSearch;
                    }
                    // Otherwise keep looking for a better fullscreen device combo
                }
            }
        }
    }
EndFullscreenDeviceComboSearch:
    if (pBestDeviceCombo == NULL)
        return false;

    // Need to find a display mode on the best adapter that uses pBestDeviceCombo->AdapterFormat
    // and is as close to bestAdapterDesktopDisplayMode's res as possible
    D3DDISPLAYMODE bestDisplayMode;
    bestDisplayMode.Width = 0;
    bestDisplayMode.Height = 0;
    bestDisplayMode.Format = D3DFMT_UNKNOWN;
    bestDisplayMode.RefreshRate = 0;
    for (UINT idm = 0; idm < pBestAdapterInfo->pDisplayModeList->Count(); idm++) {
        D3DDISPLAYMODE* pdm = (D3DDISPLAYMODE*)pBestAdapterInfo->pDisplayModeList->GetPtr(idm);
        if (pdm->Format != pBestDeviceCombo->AdapterFormat)
            continue;
        if (pdm->Width == bestAdapterDesktopDisplayMode.Width &&
            pdm->Height == bestAdapterDesktopDisplayMode.Height &&
            pdm->RefreshRate == bestAdapterDesktopDisplayMode.RefreshRate) {
            // found a perfect match, so stop
            bestDisplayMode = *pdm;
            break;
        } else if (pdm->Width == bestAdapterDesktopDisplayMode.Width &&
                   pdm->Height == bestAdapterDesktopDisplayMode.Height &&
                   pdm->RefreshRate > bestDisplayMode.RefreshRate) {
            // refresh rate doesn't match, but width/height match, so keep this
            // and keep looking
            bestDisplayMode = *pdm;
        } else if (pdm->Width == bestAdapterDesktopDisplayMode.Width) {
            // width matches, so keep this and keep looking
            bestDisplayMode = *pdm;
        } else if (bestDisplayMode.Width == 0) {
            // we don't have anything better yet, so keep this and keep looking
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
    m_d3dSettings.Fullscreen_MultisampleType = *(D3DMULTISAMPLE_TYPE*)pBestDeviceCombo->pMultiSampleTypeList->GetPtr(0);
    m_d3dSettings.Fullscreen_MultisampleQuality = 0;
    m_d3dSettings.Fullscreen_VertexProcessingType = *(VertexProcessingType*)pBestDeviceCombo->pVertexProcessingTypeList->GetPtr(0);
    m_d3dSettings.Fullscreen_PresentInterval = D3DPRESENT_INTERVAL_DEFAULT;
    return true;
}

//-----------------------------------------------------------------------------
// Name: ChooseInitialD3DSettings()
// Desc:
//-----------------------------------------------------------------------------
// @ 0x402100
HRESULT CD3DApplication::ChooseInitialD3DSettings()
{
    bool bFoundFullscreen = FindBestFullscreenMode( false, false );
    bool bFoundWindowed = FindBestWindowedMode( false, false );

    if( m_bStartFullscreen && bFoundFullscreen )
        m_d3dSettings.IsWindowed = false;
    if( !bFoundWindowed && bFoundFullscreen )
        m_d3dSettings.IsWindowed = false;

    if( !bFoundFullscreen && !bFoundWindowed )
        return D3DAPPERR_NOCOMPATIBLEDEVICES;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: HandlePossibleSizeChange()
// Desc: Reset the device if the client area size has changed.
//-----------------------------------------------------------------------------
// @ 0x402150
HRESULT CD3DApplication::HandlePossibleSizeChange()
{
    HRESULT hr = S_OK;
    RECT rcClientOld;
    rcClientOld = m_rcWindowClient;

    if( m_bIgnoreSizeChange )
        return S_OK;

    // Update window properties
    GetWindowRect( m_hWnd, &m_rcWindowBounds );
    GetClientRect( m_hWnd, &m_rcWindowClient );

    if( rcClientOld.right - rcClientOld.left !=
        m_rcWindowClient.right - m_rcWindowClient.left ||
        rcClientOld.bottom - rcClientOld.top !=
        m_rcWindowClient.bottom - m_rcWindowClient.top)
    {
        // A new window size will require a new backbuffer
        // size, so the 3D structures must be changed accordingly.
        Pause( true );

        m_d3dpp.BackBufferWidth  = m_rcWindowClient.right - m_rcWindowClient.left;
        m_d3dpp.BackBufferHeight = m_rcWindowClient.bottom - m_rcWindowClient.top;

        if( m_pd3dDevice != NULL )
        {
            // Reset the 3D environment
            if( FAILED( hr = Reset3DEnvironment() ) )
            {
                if( hr != D3DERR_OUTOFVIDEOMEMORY )
                    hr = D3DAPPERR_RESETFAILED;
                DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
            }
        }
        Pause( false );
    }
    return hr;
}

//-----------------------------------------------------------------------------
// Name: BuildPresentParamsFromSettings()
// Desc:
//-----------------------------------------------------------------------------
// @ 0x402280
void CD3DApplication::BuildPresentParamsFromSettings()
{
    m_d3dpp.Windowed               = m_d3dSettings.IsWindowed;
    m_d3dpp.MultiSampleType        = m_d3dSettings.MultisampleType();
    m_d3dpp.MultiSampleQuality     = m_d3dSettings.MultisampleQuality();
    m_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = m_d3dEnumeration.AppUsesDepthBuffer;
    m_d3dpp.hDeviceWindow          = m_hWnd;
    if( m_d3dEnumeration.AppUsesDepthBuffer )
    {
        m_d3dpp.Flags              = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
        m_d3dpp.AutoDepthStencilFormat = m_d3dSettings.DepthStencilBufferFormat();
    }
    else
    {
        m_d3dpp.Flags              = 0;
    }

    if( m_bWindowed )
    {
        m_d3dpp.BackBufferWidth  = m_rcWindowClient.right - m_rcWindowClient.left;
        m_d3dpp.BackBufferHeight = m_rcWindowClient.bottom - m_rcWindowClient.top;
        m_d3dpp.BackBufferFormat = m_d3dSettings.PDeviceCombo()->BackBufferFormat;
        m_d3dpp.FullScreen_RefreshRateInHz = 0;
        m_d3dpp.PresentationInterval = m_d3dSettings.PresentInterval();
    }
    else
    {
        m_d3dpp.BackBufferWidth  = m_d3dSettings.DisplayMode().Width;
        m_d3dpp.BackBufferHeight = m_d3dSettings.DisplayMode().Height;
        m_d3dpp.BackBufferFormat = m_d3dSettings.PDeviceCombo()->BackBufferFormat;
        m_d3dpp.FullScreen_RefreshRateInHz = m_d3dSettings.Fullscreen_DisplayMode.RefreshRate;
        m_d3dpp.PresentationInterval = m_d3dSettings.PresentInterval();
    }
    // M&B: triple buffering unless presenting immediately
    m_d3dpp.BackBufferCount = (m_d3dpp.PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE) + 1;
}

//-----------------------------------------------------------------------------
// Name: Render3DEnvironment()
// Desc: Here's what this function does:
//       - Checks to make sure app is still active (if fullscreen, etc)
//       - Checks to see if it is time to draw with DXUtil_Timer function
//       - Calls FrameMove() to recalculate new positions
//       - Calls Render() to draw the new frame
//       - Updates some frame count statistics
//       - Calls m_pd3dDevice->Present() to display the rendered frame.
//-----------------------------------------------------------------------------
// @ 0x402430
HRESULT CD3DApplication::Render3DEnvironment()
{
    HRESULT hr;

    if( m_bDeviceLost )
    {
        // Test the cooperative level to see if it's okay to render
        if( FAILED( hr = m_pd3dDevice->TestCooperativeLevel() ) )
        {
            // If the device was lost, do not render until we get it back
            if( D3DERR_DEVICELOST == hr )
                return S_OK;

            // Check if the device needs to be reset.
            if( D3DERR_DEVICENOTRESET == hr )
            {
                // If we are windowed, read the desktop mode and use the same format for
                // the back buffer
                if( m_bWindowed )
                {
                    D3DAdapterInfo* pAdapterInfo = m_d3dSettings.PAdapterInfo();
                    m_pD3D->GetAdapterDisplayMode( pAdapterInfo->AdapterOrdinal, &m_d3dSettings.Windowed_DisplayMode );
                    m_d3dpp.BackBufferFormat = m_d3dSettings.Windowed_DisplayMode.Format;
                }

                if( FAILED( hr = Reset3DEnvironment() ) )
                    return hr;
            }
            return hr;
        }
        m_bDeviceLost = false;
    }

    // Get the app's time, in seconds. Skip rendering if no time elapsed
    FLOAT fAppTime        = DXUtil_Timer( TIMER_GETAPPTIME );
    FLOAT fElapsedAppTime = DXUtil_Timer( TIMER_GETELAPSEDTIME );
    if( ( 0.0f == fElapsedAppTime ) && m_bFrameMoving )
        return S_OK;

    // M&B: frame limiter
    while( fElapsedAppTime < m_fMinFrameTime )
        fElapsedAppTime += DXUtil_Timer( TIMER_GETELAPSEDTIME );

    // FrameMove (animate) the scene
    if( m_bFrameMoving || m_bSingleStep )
    {
        // Store the time for the app
        m_fTime        = fAppTime;
        m_fElapsedTime = fElapsedAppTime;

        // Frame move the scene
        if( FAILED( hr = FrameMove() ) )
            return hr;

        m_bSingleStep = false;
    }

    // Render the scene as normal
    if( FAILED( hr = Render() ) )
        return hr;

    UpdateStats();

    // Show the frame on the primary surface.
    hr = m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
    if( D3DERR_DEVICELOST == hr )
        m_bDeviceLost = true;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: UpdateStats()
// Desc: M&B: the frame-stats string is only "%.02f fps" (the format strings are
//       still built, as in the SDK).
//-----------------------------------------------------------------------------
// @ 0x402580
void CD3DApplication::UpdateStats()
{
    // Keep track of the frame count
    static FLOAT fLastTime = 0.0f;      // 0x5d6f50
    static DWORD dwFrames  = 0;         // 0x5d6f4c
    FLOAT fTime = DXUtil_Timer( TIMER_GETABSOLUTETIME );
    ++dwFrames;

    // Update the scene stats once per second
    if( fTime - fLastTime > 1.0f )
    {
        m_fFPS    = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames  = 0;

        TCHAR strFmt[100];
        D3DFORMAT fmtAdapter = m_d3dSettings.DisplayMode().Format;
        if( fmtAdapter == m_d3dsdBackBuffer.Format )
        {
            lstrcpynA( strFmt, D3DUtil_D3DFormatToString( fmtAdapter, false ), 100 );
        }
        else
        {
            _sntprintf( strFmt, 100, _T("backbuf %s, adapter %s"),
                D3DUtil_D3DFormatToString( m_d3dsdBackBuffer.Format, false ),
                D3DUtil_D3DFormatToString( fmtAdapter, false ) );
        }
        strFmt[99] = _T('\0');

        TCHAR strDepthFmt[100];
        if( m_d3dEnumeration.AppUsesDepthBuffer )
        {
            _sntprintf( strDepthFmt, 100, _T(" (%s)"),
                D3DUtil_D3DFormatToString( m_d3dSettings.DepthStencilBufferFormat(), false ) );
            strDepthFmt[99] = _T('\0');
        }
        else
        {
            // No depth buffer
            strDepthFmt[0] = _T('\0');
        }

        _sntprintf( m_strFrameStats, 90, _T("%.02f fps"), m_fFPS );
        m_strFrameStats[89] = _T('\0');
    }
}

//-----------------------------------------------------------------------------
// Name: Cleanup3DEnvironment()
// Desc: Cleanup scene objects
//-----------------------------------------------------------------------------
// @ 0x4026e0
void CD3DApplication::Cleanup3DEnvironment()
{
    if( m_pd3dDevice != NULL )
    {
        if( m_bDeviceObjectsRestored )
        {
            m_bDeviceObjectsRestored = false;
            InvalidateDeviceObjects();
        }
        if( m_bDeviceObjectsInited )
        {
            m_bDeviceObjectsInited = false;
            DeleteDeviceObjects();
        }

        if( m_pd3dDevice->Release() > 0 )
            DisplayErrorMsg( D3DAPPERR_NONZEROREFCOUNT, MSGERR_APPMUSTEXIT );
        m_pd3dDevice = NULL;
    }
}

//-----------------------------------------------------------------------------
// Name: Initialize3DEnvironment()
// Desc: Usually this function is not overridden.  Here's what this function does:
//       - Sets the windowed flag to be either windowed or fullscreen mode
//       - Sets parameters for z-buffer depth and back buffer
//       - Creates the D3D device
//       - Sets the window position (if windowed, that is)
//       - Makes some determinations as to the abilites of the driver (HAL, etc)
//       - Sets up some cursor stuff
//       - Calls InitDeviceObjects()
//       - Calls RestoreDeviceObjects()
//       - If all goes well, app is ready to render
//       (UpdateDeviceStats is inlined.)
//-----------------------------------------------------------------------------
// @ 0x4027c0
HRESULT CD3DApplication::Initialize3DEnvironment()
{
    HRESULT hr;

    D3DAdapterInfo* pAdapterInfo = m_d3dSettings.PAdapterInfo();
    D3DDeviceInfo* pDeviceInfo = m_d3dSettings.PDeviceInfo();

    m_bWindowed = m_d3dSettings.IsWindowed;

    // Prepare window for possible windowed/fullscreen change
    AdjustWindowForChange();

    // Set up the presentation parameters
    BuildPresentParamsFromSettings();

    if( pDeviceInfo->Caps.PrimitiveMiscCaps & D3DPMISCCAPS_NULLREFERENCE )
    {
        // Warn user about null ref device that can't render anything
        DisplayErrorMsg( D3DAPPERR_NULLREFDEVICE, 0 );
    }

    DWORD behaviorFlags;
    if (m_d3dSettings.GetVertexProcessingType() == SOFTWARE_VP)
        behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == MIXED_VP)
        behaviorFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == HARDWARE_VP)
        behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == PURE_HARDWARE_VP)
        behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
    else
        behaviorFlags = 0; // TODO: throw exception

    // Create the device
    hr = m_pD3D->CreateDevice( m_d3dSettings.AdapterOrdinal(), pDeviceInfo->DevType,
                               m_hWndFocus, behaviorFlags, &m_d3dpp,
                               &m_pd3dDevice );

    if( SUCCEEDED(hr) )
    {
        // When moving from fullscreen to windowed mode, it is important to
        // adjust the window size after recreating the device rather than
        // beforehand to ensure that you get the window size you want.
        if( m_bWindowed )
        {
            SetWindowPos( m_hWnd, HWND_NOTOPMOST,
                          m_rcWindowBounds.left, m_rcWindowBounds.top,
                          ( m_rcWindowBounds.right - m_rcWindowBounds.left ),
                          ( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
                          SWP_SHOWWINDOW );
        }

        // Store device Caps
        m_pd3dDevice->GetDeviceCaps( &m_d3dCaps );
        m_dwCreateFlags = behaviorFlags;

        // Store device description
        if( pDeviceInfo->DevType == D3DDEVTYPE_REF )
            lstrcpyA( m_strDeviceStats, "REF" );
        else if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
            lstrcpyA( m_strDeviceStats, "HAL" );
        else if( pDeviceInfo->DevType == D3DDEVTYPE_SW )
            lstrcpyA( m_strDeviceStats, "SW" );

        if( behaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING &&
            behaviorFlags & D3DCREATE_PUREDEVICE )
        {
            if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
                lstrcatA( m_strDeviceStats, " (pure hw vp)" );
            else
                lstrcatA( m_strDeviceStats, " (simulated pure hw vp)" );
        }
        else if( behaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING )
        {
            if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
                lstrcatA( m_strDeviceStats, " (hw vp)" );
            else
                lstrcatA( m_strDeviceStats, " (simulated hw vp)" );
        }
        else if( behaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING )
        {
            if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
                lstrcatA( m_strDeviceStats, " (mixed vp)" );
            else
                lstrcatA( m_strDeviceStats, " (simulated mixed vp)" );
        }
        else if( behaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING )
        {
            lstrcatA( m_strDeviceStats, " (sw vp)" );
        }

        if( pDeviceInfo->DevType == D3DDEVTYPE_HAL )
        {
            // Be sure not to overflow m_strDeviceStats when appending the adapter
            // description, since it can be long.  Note that the adapter description
            // is initially CHAR and must be converted to TCHAR.
            lstrcatA( m_strDeviceStats, ": " );
            const int cchDesc = sizeof(pAdapterInfo->AdapterIdentifier.Description);
            TCHAR szDescription[cchDesc];
            DXUtil_ConvertAnsiStringToGenericCch( szDescription,
                pAdapterInfo->AdapterIdentifier.Description, cchDesc );
            int maxAppend = sizeof(m_strDeviceStats) / sizeof(TCHAR) -
                lstrlenA( m_strDeviceStats ) - 1;
            _tcsncat( m_strDeviceStats, szDescription, maxAppend );
        }

        // Store render target surface desc
        LPDIRECT3DSURFACE9 pBackBuffer = NULL;
        m_pd3dDevice->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer );
        pBackBuffer->GetDesc( &m_d3dsdBackBuffer );
        pBackBuffer->Release();

        // Set up the fullscreen cursor
        GetClassLongA( m_hWnd, GCL_HCURSOR );   // (the SDK's HCURSOR use is compiled out)
        if( m_bShowCursorWhenFullscreen && !m_bWindowed )
            m_pd3dDevice->ShowCursor( TRUE );
        else
            m_pd3dDevice->ShowCursor( FALSE );

        // Confine cursor to fullscreen window
        if( m_bClipCursorWhenFullscreen )
        {
            if (!m_bWindowed )
            {
                RECT rcWindow;
                GetWindowRect( m_hWnd, &rcWindow );
                ClipCursor( &rcWindow );
            }
            else
            {
                ClipCursor( NULL );
            }
        }

        // Initialize the app's device-dependent objects
        hr = InitDeviceObjects();
        if( FAILED(hr) )
        {
            DeleteDeviceObjects();
        }
        else
        {
            m_bDeviceObjectsInited = true;
            hr = RestoreDeviceObjects();
            if( FAILED(hr) )
            {
                InvalidateDeviceObjects();
            }
            else
            {
                m_bDeviceObjectsRestored = true;
                return S_OK;
            }
        }

        // Cleanup before we try again
        Cleanup3DEnvironment();
    }

    // If that failed, fall back to the reference rasterizer
    if( hr != D3DAPPERR_MEDIANOTFOUND &&
        hr != __HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND ) &&
        pDeviceInfo->DevType == D3DDEVTYPE_HAL )
    {
        if (FindBestWindowedMode(false, true))
        {
            m_bWindowed = true;
            AdjustWindowForChange();
            // Make sure main window isn't topmost, so error message is visible
            SetWindowPos( m_hWnd, HWND_NOTOPMOST,
                          m_rcWindowBounds.left, m_rcWindowBounds.top,
                          ( m_rcWindowBounds.right - m_rcWindowBounds.left ),
                          ( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
                          SWP_SHOWWINDOW );

            // Let the user know we are switching from HAL to the reference rasterizer
            DisplayErrorMsg( hr, MSGWARN_SWITCHEDTOREF );

            hr = Initialize3DEnvironment();
        }
    }
    return hr;
}

//-----------------------------------------------------------------------------
// Name: ToggleFullScreen()
// Desc: Called when user toggles between fullscreen mode and windowed mode
//-----------------------------------------------------------------------------
// @ 0x402be0
HRESULT CD3DApplication::ToggleFullscreen()
{
    HRESULT hr;
    int AdapterOrdinalOld = m_d3dSettings.AdapterOrdinal();
    D3DDEVTYPE DevTypeOld = m_d3dSettings.DevType();

    Pause( true );
    m_bIgnoreSizeChange = true;

    // Toggle the windowed state
    m_bWindowed = !m_bWindowed;
    m_d3dSettings.IsWindowed = m_bWindowed;

    // Prepare window for windowed/fullscreen change
    AdjustWindowForChange();

    // If AdapterOrdinal and DevType are the same, we can just do a Reset().
    // If they've changed, we need to do a complete device teardown/rebuild.
    if (m_d3dSettings.AdapterOrdinal() == AdapterOrdinalOld &&
        m_d3dSettings.DevType() == DevTypeOld)
    {
        // Reset the 3D device
        BuildPresentParamsFromSettings();
        hr = Reset3DEnvironment();
    }
    else
    {
        Cleanup3DEnvironment();
        hr = Initialize3DEnvironment();
    }
    if( FAILED( hr ) )
    {
        if( hr != D3DERR_OUTOFVIDEOMEMORY )
            hr = D3DAPPERR_RESETFAILED;
        m_bIgnoreSizeChange = false;
        if( !m_bWindowed )
        {
            // Restore window type to windowed mode
            m_bWindowed = !m_bWindowed;
            m_d3dSettings.IsWindowed = m_bWindowed;
            AdjustWindowForChange();
            SetWindowPos( m_hWnd, HWND_NOTOPMOST,
                        m_rcWindowBounds.left, m_rcWindowBounds.top,
                        ( m_rcWindowBounds.right - m_rcWindowBounds.left ),
                        ( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
                        SWP_SHOWWINDOW );
        }
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    m_bIgnoreSizeChange = false;

    // When moving from fullscreen to windowed mode, it is important to
    // adjust the window size after resetting the device rather than
    // beforehand to ensure that you get the window size you want.
    if( m_bWindowed )
    {
        SetWindowPos( m_hWnd, HWND_NOTOPMOST,
                      m_rcWindowBounds.left, m_rcWindowBounds.top,
                      ( m_rcWindowBounds.right - m_rcWindowBounds.left ),
                      ( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
                      SWP_SHOWWINDOW );
    }

    Pause( false );
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: UserSelectNewDevice()
// Desc: Displays a dialog so the user can select a new adapter, device, or
//       display mode, and then recreates the 3D environment if needed
//-----------------------------------------------------------------------------
// @ 0x402d40
HRESULT CD3DApplication::UserSelectNewDevice()
{
    HRESULT hr;

    // Can't display dialogs in fullscreen mode
    if( m_bWindowed == false )
    {
        if( FAILED( ToggleFullscreen() ) )
        {
            DisplayErrorMsg( D3DAPPERR_RESETFAILED, MSGERR_APPMUSTEXIT );
            return E_FAIL;
        }
    }

    CD3DSettingsDialog settingsDialog( &m_d3dEnumeration, &m_d3dSettings);
    if( settingsDialog.ShowDialog( m_hWnd ) != IDOK )
        return S_OK;
    settingsDialog.GetFinalSettings( &m_d3dSettings );

    m_bWindowed = m_d3dSettings.IsWindowed;

    // Release display objects, so a new device can be created
    Cleanup3DEnvironment();

    // Inform the display class of the change. It will internally
    // re-create valid surfaces, a d3ddevice, etc.
    if( FAILED( hr = Initialize3DEnvironment() ) )
    {
        if( hr != D3DERR_OUTOFVIDEOMEMORY )
            hr = D3DAPPERR_RESETFAILED;
        if( !m_bWindowed )
        {
            // Restore window type to windowed mode
            m_bWindowed = !m_bWindowed;
            m_d3dSettings.IsWindowed = m_bWindowed;
            AdjustWindowForChange();
            SetWindowPos( m_hWnd, HWND_NOTOPMOST,
                        m_rcWindowBounds.left, m_rcWindowBounds.top,
                        ( m_rcWindowBounds.right - m_rcWindowBounds.left ),
                        ( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
                        SWP_SHOWWINDOW );
        }
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    // If the app is paused, trigger the rendering of the current frame
    if( false == m_bFrameMoving )
    {
        m_bSingleStep = true;
        DXUtil_Timer( TIMER_START );
        DXUtil_Timer( TIMER_STOP );
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Run()
// Desc: Message-processing loop. Idle time is used to render the scene.
//       (M&B: Unicode message pump, quit request flag)
//-----------------------------------------------------------------------------
// @ 0x402ee0
INT CD3DApplication::Run()
{
    // Load keyboard accelerators
    HACCEL hAccel = LoadAcceleratorsA( NULL, MAKEINTRESOURCEA(IDR_MAIN_ACCEL) );

    // Now we're ready to recieve and process Windows messages.
    bool bGotMsg;
    MSG  msg;
    msg.message = WM_NULL;
    PeekMessageA( &msg, NULL, 0U, 0U, PM_NOREMOVE );

    while( WM_QUIT != msg.message  )
    {
        // Use PeekMessage() if the app is active, so we can use idle time to
        // render the scene. Else, use GetMessage() to avoid eating CPU time.
        if( m_bActive )
            bGotMsg = ( PeekMessageW( &msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );
        else
            bGotMsg = ( GetMessageW( &msg, NULL, 0U, 0U ) != 0 );

        if( bGotMsg )
        {
            // Translate and dispatch the message
            if( hAccel == NULL || m_hWnd == NULL ||
                0 == TranslateAcceleratorA( m_hWnd, hAccel, &msg ) )
            {
                TranslateMessage( &msg );
                DispatchMessageW( &msg );
            }
            if( m_bQuitRequested )
                SendMessageA( m_hWnd, WM_CLOSE, 0, 0 );
        }
        else
        {
            if( m_bDeviceLost )
            {
                // Yield some CPU time to other processes
                Sleep( 100 ); // 100 milliseconds
            }
            // Render a frame during idle time (no messages are waiting)
            if( m_bActive )
            {
                if( FAILED( Render3DEnvironment() ) )
                    SendMessageA( m_hWnd, WM_CLOSE, 0, 0 );
            }
        }
    }
    if( hAccel != NULL )
        DestroyAcceleratorTable( hAccel );

    return (INT)msg.wParam;
}

//-----------------------------------------------------------------------------
// Name: Create3DEnvironment()   (M&B; provisional name)
// Desc: The device half of the SDK's Create(): picks the initial settings,
//       initializes the scene and the 3D environment and shows the window.
//-----------------------------------------------------------------------------
// @ 0x403010
HRESULT CD3DApplication::Create3DEnvironment( int )
{
    HRESULT hr;

    if( FAILED( hr = ChooseInitialD3DSettings() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    // Initialize the application timer
    DXUtil_Timer( TIMER_START );

    // Initialize the app's custom scene stuff
    if( FAILED( hr = OneTimeSceneInit() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    // Initialize the 3D environment for the app
    if( FAILED( hr = Initialize3DEnvironment() ) )
    {
        SAFE_RELEASE( m_pD3D );
        return DisplayErrorMsg( hr, MSGERR_APPMUSTEXIT );
    }

    // The app is ready to go
    Pause( false );

    ShowWindow( m_hWnd, SW_SHOWNORMAL );
    UpdateWindow( m_hWnd );
    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: Message handling function.
//-----------------------------------------------------------------------------
// @ 0x403120
LRESULT CD3DApplication::MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_PAINT:
            // Handle paint messages when the app is paused
            if( m_pd3dDevice && !m_bActive && m_bWindowed &&
                m_bDeviceObjectsInited && m_bDeviceObjectsRestored )
            {
                Render();
                m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
            }
            break;

        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 100;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 100;
            break;

        case WM_ENTERSIZEMOVE:
            // Halt frame movement while the app is sizing or moving
            Pause( true );
            break;

        case WM_SIZE:
            // Pick up possible changes to window style due to maximize, etc.
            if( m_bWindowed && m_hWnd != NULL )
                m_dwWindowStyle = GetWindowLongA( m_hWnd, GWL_STYLE );

            if( SIZE_MINIMIZED == wParam )
            {
                if( m_bClipCursorWhenFullscreen && !m_bWindowed )
                    ClipCursor( NULL );
                Pause( true ); // Pause while we're minimized
                m_bMinimized = true;
                m_bMaximized = false;
            }
            else if( SIZE_MAXIMIZED == wParam )
            {
                if( m_bMinimized )
                    Pause( false ); // Unpause since we're no longer minimized
                m_bMinimized = false;
                m_bMaximized = true;
                HandlePossibleSizeChange();
            }
            else if( SIZE_RESTORED == wParam )
            {
                if( m_bMaximized )
                {
                    m_bMaximized = false;
                    HandlePossibleSizeChange();
                }
                else if( m_bMinimized)
                {
                    Pause( false ); // Unpause since we're no longer minimized
                    m_bMinimized = false;
                    HandlePossibleSizeChange();
                }
                else
                {
                    // If we're neither maximized nor minimized, the window size
                    // is changing by the user dragging the window edges.  In this
                    // case, we don't reset the device yet -- we wait until the
                    // user stops dragging, and a WM_EXITSIZEMOVE message comes.
                }
            }
            break;

        case WM_EXITSIZEMOVE:
            Pause( false );
            HandlePossibleSizeChange();
            break;

        case WM_SETCURSOR:
            // Turn off Windows cursor in fullscreen mode
            if( m_bActive && !m_bWindowed )
            {
                SetCursor( NULL );
                if( m_bShowCursorWhenFullscreen )
                    m_pd3dDevice->ShowCursor( TRUE );
                return TRUE; // prevent Windows from setting cursor to window class cursor
            }
            break;

        case WM_MOUSEMOVE:
            if( m_bActive && m_pd3dDevice != NULL )
            {
                POINT ptCursor;
                GetCursorPos( &ptCursor );
                if( !m_bWindowed )
                    ScreenToClient( m_hWnd, &ptCursor );
                m_pd3dDevice->SetCursorPosition( ptCursor.x, ptCursor.y, 0 );
            }
            break;

        case WM_ENTERMENULOOP:
            // Pause the app when menus are displayed
            Pause(true);
            break;

        case WM_EXITMENULOOP:
            Pause(false);
            break;

        case WM_NCHITTEST:
            // Prevent the user from selecting the menu in fullscreen mode
            if( !m_bWindowed )
                return HTCLIENT;
            break;

        case WM_POWERBROADCAST:
            switch( wParam )
            {
                #ifndef PBT_APMQUERYSUSPEND
                    #define PBT_APMQUERYSUSPEND 0x0000
                #endif
                case PBT_APMQUERYSUSPEND:
                    // At this point, the app should save any data for open
                    // network connections, files, etc., and prepare to go into
                    // a suspended mode.
                    return TRUE;

                #ifndef PBT_APMRESUMESUSPEND
                    #define PBT_APMRESUMESUSPEND 0x0007
                #endif
                case PBT_APMRESUMESUSPEND:
                    // At this point, the app should recover any data, network
                    // connections, files, etc., and resume running from when
                    // the app was suspended.
                    return TRUE;
            }
            break;

        case WM_SYSCOMMAND:
            // Prevent moving/sizing and power loss in fullscreen mode
            switch( wParam )
            {
                case SC_MOVE:
                case SC_SIZE:
                case SC_MAXIMIZE:
                case SC_KEYMENU:
                case SC_MONITORPOWER:
                    if( false == m_bWindowed )
                        return 1;
                    break;
            }
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
                case IDM_TOGGLESTART:
                    // Toggle frame movement
                    m_bFrameMoving = !m_bFrameMoving;
                    DXUtil_Timer( m_bFrameMoving ? TIMER_START : TIMER_STOP );
                    break;

                case IDM_SINGLESTEP:
                    // Single-step frame movement
                    if( false == m_bFrameMoving )
                        DXUtil_Timer( TIMER_ADVANCE );
                    else
                        DXUtil_Timer( TIMER_STOP );
                    m_bFrameMoving = false;
                    m_bSingleStep  = true;
                    break;

                case IDM_CHANGEDEVICE:
                    // Prompt the user to select a new device or mode
                    Pause(true);
                    UserSelectNewDevice();
                    Pause(false);
                    return 0;

                case IDM_TOGGLEFULLSCREEN:
                    // Toggle the fullscreen/window mode
                    Pause( true );
                    if( FAILED( ToggleFullscreen() ) )
                        DisplayErrorMsg( D3DAPPERR_RESETFAILED, MSGERR_APPMUSTEXIT );
                    Pause( false );
                    return 0;

                case IDM_EXIT:
                    // Received key/menu command to exit app
                    SendMessageA( hWnd, WM_CLOSE, 0, 0 );
                    return 0;
            }
            break;

        case WM_CLOSE:
            // M&B: FinalCleanup runs first
            FinalCleanup();
            Cleanup3DEnvironment();
            SAFE_RELEASE( m_pD3D );
            HMENU hMenu;
            hMenu = GetMenu(hWnd);
            if( hMenu != NULL )
                DestroyMenu( hMenu );
            DestroyWindow( hWnd );
            PostQuitMessage(0);
            m_hWnd = NULL;
            return 0;
    }

    return DefWindowProcA( hWnd, uMsg, wParam, lParam );
}
