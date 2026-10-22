// Slice s00401010: Direct3D application framework (window creation, device reset, error messages).
#include <string.h>
#include <new>             // declares operator delete[]: without it cl frees arrays with plain operator delete
#include "s00401010.h"

Application* g_app;     // 0x5d6f40
bool g_errorShown;      // 0x5d6f48
extern int g_defWidth;  // 0x5a9fe4
extern int g_defHeight; // 0x5a9fe8
LRESULT CALLBACK MBWndProc(HWND, UINT, WPARAM, LPARAM);   // 0x401000

// @ 0x401010
bool app_confirm_device(int a, int mode, int b, int c)
{
    int flag;
    if (mode == 0) flag = 0x20;
    else if (mode == 1) flag = 0x80;
    else if (mode == 2) flag = 0x40;
    else flag = (mode != 3) - 1 & 0x50;
    return g_app->ConfirmDevice(a, flag, b, c) >= 0;
}

// @ 0x401070
int Application::Create(HINSTANCE hInst)
{
    if (hInst) {
        if (m_hWnd == 0) {
            m_dwWindowStyle = m_bStartFullscreen ? 0x80000000 : 0xcf0000;
            RECT rc;
            if (g_defWidth > 0) {
                SetRect(&rc, 0, 0, g_defWidth, g_defHeight);
            } else {
                int fx = GetSystemMetrics(0x20) * 4;
                int fy = GetSystemMetrics(0x21);
                int t = GetSystemMetrics(0xf) + fy * 4;
                t += GetSystemMetrics(4);
                int w = GetSystemMetrics(0x3d) - fx;
                int h = GetSystemMetrics(0x3e) - t;
                SetRect(&rc, 0, 0, w, h);
            }
            AdjustWindowRect(&rc, m_dwWindowStyle, 1);
            WNDCLASSEXW wc;
            memset(&wc, 0, sizeof(wc));
            wc.cbSize = sizeof(wc);
            wc.hbrBackground = (HBRUSH)GetStockObject(4);
            wc.style = 3;
            wc.lpfnWndProc = MBWndProc;
            wc.hIcon = LoadIconA(hInst, (LPCSTR)0xce);
            wc.hIconSm = LoadIconA(hInst, (LPCSTR)0xce);
            wc.hInstance = hInst;
            wc.hCursor = LoadCursorA(0, (LPCSTR)0x7f00);
            wc.lpszClassName = L"MB Window";
            RegisterClassExW(&wc);
            int sx = GetSystemMetrics(0x3d);
            int sy = GetSystemMetrics(0x3e);
            int x = (rc.left - rc.right + sx) / 2;
            int y = (rc.top - rc.bottom + sy) / 2;
            m_hWnd = CreateWindowExW(0, L"MB Window", (LPCWSTR)m_strWindowTitle, m_dwWindowStyle, x, y,
                                     rc.right - rc.left, rc.bottom - rc.top, 0, LoadMenuA(hInst, (LPCSTR)0x8d), hInst, 0);
            UpdateWindow(m_hWnd);
        }
        if (m_hWndFocus == 0) m_hWndFocus = m_hWnd;
        m_dwWindowStyle = GetWindowLongA(m_hWnd, -16);
        GetWindowRect(m_hWnd, &m_rcWindowBounds);
        GetClientRect(m_hWnd, &m_rcWindowClient);
    }
    return 0;
}

// @ 0x401290
int Application::Reset3DEnvironment()
{
    if (m_bDeviceObjectsRestored) {
        m_bDeviceObjectsRestored = false;
        InvalidateDeviceObjects();
    }
    if (m_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICELOST) {
        m_bDeviceLost = true;
        return 0;
    }
    int hr = m_pd3dDevice->Reset(&m_d3dpp);
    if (hr < 0) return hr;
    IDirect3DSurface9* back;
    m_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back);
    back->GetDesc(&m_d3dsdBackBuffer);
    back->Release();
    GetClassLongA(m_hWnd, -12);
    if (m_bShowCursorWhenFullscreen && !m_bWindowed) m_pd3dDevice->ShowCursor(1);
    else m_pd3dDevice->ShowCursor(0);
    if (m_bClipCursorWhenFullscreen) {
        RECT rc;
        if (!m_bWindowed) {
            GetWindowRect(m_hWnd, &rc);
            ClipCursor(&rc);
        } else
            ClipCursor(0);
    }
    hr = RestoreDeviceObjects();
    if (hr < 0) {
        InvalidateDeviceObjects();
        return hr;
    }
    m_bDeviceObjectsRestored = true;
    if (!m_bFrameMoving) {
        m_bSingleStep = true;
        GetTimer(1);
        GetTimer(2);
    }
    return 0;
}

// @ 0x4013e0
int Application::AdjustWindowForChange()
{
    if (m_bWindowed) {
        m_dwWindowStyle = 0xcf0000;
        SetWindowLongA(m_hWnd, -16, 0xcf0000);
        if (m_hMenu) {
            SetMenu(m_hWnd, m_hMenu);
            m_hMenu = 0;
            return 0;
        }
    } else {
        m_dwWindowStyle = 0x80000000;
        SetWindowLongA(m_hWnd, -16, 0x80000000);
        if (m_hMenu == 0) {
            m_hMenu = GetMenu(m_hWnd);
            SetMenu(m_hWnd, 0);
        }
    }
    return 0;
}

// @ 0x401480
void Application::Pause(bool bPause)
{
    static DWORD dwAppPausedCount = 0L;     // 0x5d6f44
    dwAppPausedCount += (bPause ? +1 : -1);
    m_bActive = (dwAppPausedCount ? false : true);
    if (bPause && (1 == dwAppPausedCount)) {
        if (!m_bFrameMoving) return;
        GetTimer(2);
    }
    if (0 == dwAppPausedCount) {
        if (m_bFrameMoving) GetTimer(1);
    }
}

// @ 0x4014f0
int Application::DisplayErrorMsg(int hr, int type)
{
    char msg[512];
    if (g_errorShown) return hr;
    switch (hr) {
    case 0x8007000e:
        strcpy(msg, "Not enough memory.");
        break;
    case 0x82000001:
        strcpy(msg, "Could not initialize Direct3D. You may\nwant to check that the latest version of\nDirectX is correctly installed on your\nsystem.  Also make sure that this program\nwas compiled with header files that match\nthe installed DirectX DLLs.");
        break;
    case 0x82000003:
        strcpy(msg, "Could not find any compatible Direct3D\ndevices.");
        break;
    case 0x82000004:
        strcpy(msg, "This sample cannot run in a desktop\nwindow with the current display settings.\nPlease change your desktop settings to a\n16- or 32-bit display mode and re-run this\nsample.");
        break;
    case 0x82000007:
        strcpy(msg, "Your Direct3D hardware accelerator cannot\nrender into a window.\nPress F2 while the app is running to see a\nlist of available devices and modes.");
        break;
    case 0x82000006:
        strcpy(msg, "This sample requires functionality that is\nnot available on your Direct3D hardware\naccelerator.");
        break;
    case 0x82000005:
        strcpy(msg, "No hardware-accelerated Direct3D devices\nwere found.");
        break;
    case 0x82000008:
        strcpy(msg, "Your Direct3D hardware accelerator cannot\nrender into a window with the current\ndesktop display settings.\nPress F2 while the app is running to see a\nlist of available devices and modes.");
        break;
    case 0x82000009:
        strcpy(msg, "This sample requires functionality that is\nnot available on your Direct3D hardware\naccelerator with the current desktop display\nsettings.\nPress F2 while the app is running to see a\nlist of available devices and modes.");
        break;
    case 0x8200000b:
    case 0x80070002:
        strcpy(msg, "Could not load required media.");
        break;
    case 0x8200000c:
        strcpy(msg, "Could not reset the Direct3D device.");
        break;
    case 0x8200000a:
        strcpy(msg, "A D3D object has a non-zero reference\ncount (meaning things were not properly\ncleaned up).");
        break;
    case 0x8200000d:
        strcpy(msg, "Warning: Nothing will be rendered.\nThe reference rendering device was selected, but your\ncomputer only has a reduced-functionality reference device\ninstalled.  Install the DirectX SDK to get the full\nreference device.\n");
        break;
    case 0x8876017c:
        strcpy(msg, "Not enough video memory.");
        break;
    default:
        strcpy(msg, "Generic application error. Enable\ndebug output for detailed information.");
        break;
    case 0x88760827:
        strcpy(msg, "A serious problem occured inside the display driver.");
        type = 1;
        break;
    }
    if (type == 1) {
        g_errorShown = true;
        strcat(msg, "\n\nThis sample will now exit.");
        MessageBoxA(0, msg, m_strWindowTitle, 0x10);
        if (m_hWnd) SendMessageA(m_hWnd, 0x10, 0, 0);
    } else {
        if (type == 2)
            strcat(msg, "\n\nSwitching to the reference rasterizer,\na software device that implements the entire\nDirect3D feature set, but runs very slowly.");
        MessageBoxA(0, msg, m_strWindowTitle, 0x30);
    }
    return hr;
}

// @ 0x401830
void __stdcall vec_dtor(void* p, unsigned size, int n, void (__fastcall* f)(void*))
{
    p = (char*)p + size * n;
    while (--n >= 0) {
        p = (char*)p - size;
        f(p);
    }
}

// 0x401870: compiler-generated vector deleting destructor of a 0x34-byte record (two strings and an
// array pointer), called with flags 3 from 0x4d3731.
struct StrRec {                 // 0x10 bytes, dtor frees p (operator delete)
    int pad0;
    char* p;
    int pad8[2];
    ~StrRec();                  // 0x42bc40
};
struct Rec34 {
    StrRec a;                   // 0
    StrRec b;                   // 0x10
    int gap[4];                 // 0x20
    char* arr;                  // 0x30
    ~Rec34() { if (arr) delete[] arr; }
};
// @ 0x401870
void destroy_rec_array(Rec34* r) { delete[] r; }

// @ 0x401800
void __stdcall vec_ctor(void* p, unsigned size, int n, void (__fastcall* f)(void*))
{
    while (--n >= 0) {
        f(p);
        p = (char*)p + size;
    }
}
