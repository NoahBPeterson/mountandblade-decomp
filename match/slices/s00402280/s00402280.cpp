// Slice s00402280: Application present parameters, frame loop, stats, device creation/cleanup.
#include <stdio.h>
#include <string.h>
#include <mbstring.h>
#include "s00402280.h"

extern bool g_errorShown;       // 0x5d6f48

// @ 0x402280
void Application::BuildPresentParamsFromSettings()
{
    m_d3dpp.Windowed = m_d3dSettings.IsWindowed;
    m_d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE)m_d3dSettings.MultisampleType();
    m_d3dpp.MultiSampleQuality = m_d3dSettings.MultisampleQuality();
    m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.EnableAutoDepthStencil = m_d3dEnum.AppUsesDepthBuffer;
    m_d3dpp.hDeviceWindow = m_hWnd;
    if (m_d3dEnum.AppUsesDepthBuffer) {
        m_d3dpp.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
        m_d3dpp.AutoDepthStencilFormat = (D3DFORMAT)m_d3dSettings.DepthStencilBufferFormat();
    } else {
        m_d3dpp.Flags = 0;
    }
    if (m_bWindowed) {
        m_d3dpp.BackBufferWidth = m_rcWindowClient.right - m_rcWindowClient.left;
        m_d3dpp.BackBufferHeight = m_rcWindowClient.bottom - m_rcWindowClient.top;
        m_d3dpp.BackBufferFormat = (D3DFORMAT)m_d3dSettings.PDeviceCombo()->BackBufferFormat;
        m_d3dpp.FullScreen_RefreshRateInHz = 0;
        m_d3dpp.PresentationInterval = m_d3dSettings.PresentInterval();
    } else {
        m_d3dpp.BackBufferWidth = m_d3dSettings.DisplayMode().Width;
        m_d3dpp.BackBufferHeight = m_d3dSettings.DisplayMode().Height;
        m_d3dpp.BackBufferFormat = (D3DFORMAT)m_d3dSettings.PDeviceCombo()->BackBufferFormat;
        m_d3dpp.FullScreen_RefreshRateInHz = m_d3dSettings.Fullscreen_DisplayMode.RefreshRate;
        m_d3dpp.PresentationInterval = m_d3dSettings.PresentInterval();
    }
    m_d3dpp.BackBufferCount = (m_d3dpp.PresentationInterval != 0x80000000) + 1;
}

// @ 0x402430
int Application::Render3DEnvironment()
{
    HRESULT hr;
    if (m_bDeviceLost) {
        if (FAILED(hr = m_pd3dDevice->TestCooperativeLevel())) {
            if (D3DERR_DEVICELOST == hr) return S_OK;
            if (D3DERR_DEVICENOTRESET == hr) {
                if (m_bWindowed) {
                    D3DAdapterInfo* pAdapterInfo = m_d3dSettings.PAdapterInfo();
                    m_pD3D->GetAdapterDisplayMode(pAdapterInfo->AdapterOrdinal, &m_d3dSettings.Windowed_DisplayMode);
                    m_d3dpp.BackBufferFormat = m_d3dSettings.Windowed_DisplayMode.Format;
                }
                if (FAILED(hr = Reset3DEnvironment())) return hr;
            }
            return hr;
        }
        m_bDeviceLost = false;
    }
    float fAppTime = GetTimer(5);
    float fElapsedAppTime = GetTimer(6);
    if ((0.0f == fElapsedAppTime) && m_bFrameMoving) return S_OK;
    while (fElapsedAppTime < fTime8)
        fElapsedAppTime += GetTimer(6);
    if (m_bFrameMoving || m_bSingleStep) {
        m_fTime = fAppTime;
        m_fElapsedTime = fElapsedAppTime;
        if (FAILED(hr = FrameMove())) return hr;
        m_bSingleStep = false;
    }
    if (FAILED(hr = Render())) return hr;
    UpdateStats();
    hr = m_pd3dDevice->Present(NULL, NULL, NULL, NULL);
    if (D3DERR_DEVICELOST == hr) m_bDeviceLost = true;
    return S_OK;
}

// @ 0x402580
void Application::UpdateStats()
{
    static float fLastTime = 0.0f;      // 0x5d6f50
    static DWORD dwFrames = 0;          // 0x5d6f4c
    float fTime = GetTimer(4);
    ++dwFrames;
    if (fTime - fLastTime > 1.0f) {
        m_fFPS = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames = 0;
        char strFmt[100];
        int fmtAdapter = m_d3dSettings.DisplayMode().Format;
        if (fmtAdapter == m_d3dsdBackBuffer.Format) {
            lstrcpynA(strFmt, D3DUtil_D3DFormatToString(fmtAdapter, false), 100);
        } else {
            _snprintf(strFmt, 100, "backbuf %s, adapter %s", D3DUtil_D3DFormatToString(m_d3dsdBackBuffer.Format, false),
                      D3DUtil_D3DFormatToString(fmtAdapter, false));
        }
        strFmt[99] = 0;
        char strDepthFmt[100];
        if (m_d3dEnum.AppUsesDepthBuffer) {
            _snprintf(strDepthFmt, 100, " (%s)", D3DUtil_D3DFormatToString(m_d3dSettings.DepthStencilBufferFormat(), false));
            strDepthFmt[99] = 0;
        } else {
            strDepthFmt[0] = 0;
        }
        _snprintf(m_strFrameStats, 90, "%.02f fps", m_fFPS);
        m_strFrameStats[89] = 0;
    }
}

// @ 0x4026e0
void Application::Cleanup3DEnvironment()
{
    if (m_pd3dDevice != NULL) {
        if (m_bDeviceObjectsRestored) {
            m_bDeviceObjectsRestored = false;
            InvalidateDeviceObjects();
        }
        if (m_bDeviceObjectsInited) {
            m_bDeviceObjectsInited = false;
            DeleteDeviceObjects();
        }
        if (m_pd3dDevice->Release() > 0 && !g_errorShown) {
            char msg[512];
            strcpy(msg, "A D3D object has a non-zero reference\ncount (meaning things were not properly\ncleaned up).");
            g_errorShown = true;
            strcat(msg, "\n\nThis sample will now exit.");
            MessageBoxA(0, msg, m_strWindowTitle, 0x10);
            if (m_hWnd) SendMessageA(m_hWnd, 0x10, 0, 0);
        }
        m_pd3dDevice = NULL;
    }
}

// @ 0x4027c0
int Application::Initialize3DEnvironment()
{
    HRESULT hr;
    D3DAdapterInfo* pAdapterInfo = m_d3dSettings.PAdapterInfo();
    D3DDeviceInfo* pDeviceInfo = m_d3dSettings.PDeviceInfo();
    m_bWindowed = m_d3dSettings.IsWindowed;
    AdjustWindowForChange();
    BuildPresentParamsFromSettings();
    if (pDeviceInfo->Caps.PrimitiveMiscCaps & D3DPMISCCAPS_NULLREFERENCE)
        DisplayErrorMsg(0x8200000d, 0);
    DWORD behaviorFlags;
    if (m_d3dSettings.GetVertexProcessingType() == 0) behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == 1) behaviorFlags = D3DCREATE_MIXED_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == 2) behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    else if (m_d3dSettings.GetVertexProcessingType() == 3) behaviorFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
    else behaviorFlags = 0;
    hr = m_pD3D->CreateDevice(m_d3dSettings.PDeviceCombo()->AdapterOrdinal, (D3DDEVTYPE)pDeviceInfo->DeviceType, m_hWndFocus,
                              behaviorFlags, &m_d3dpp, &m_pd3dDevice);
    if (SUCCEEDED(hr)) {
        if (m_bWindowed) {
            SetWindowPos(m_hWnd, HWND_NOTOPMOST, m_rcWindowBounds.left, m_rcWindowBounds.top,
                         (m_rcWindowBounds.right - m_rcWindowBounds.left), (m_rcWindowBounds.bottom - m_rcWindowBounds.top),
                         SWP_SHOWWINDOW);
        }
        m_pd3dDevice->GetDeviceCaps(&m_d3dCaps);
        m_dwCreateFlags = behaviorFlags;
        if (pDeviceInfo->DeviceType == 2) lstrcpyA(m_strDeviceStats, "REF");
        else if (pDeviceInfo->DeviceType == 1) lstrcpyA(m_strDeviceStats, "HAL");
        else if (pDeviceInfo->DeviceType == 3) lstrcpyA(m_strDeviceStats, "SW");
        if ((behaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) && (behaviorFlags & D3DCREATE_PUREDEVICE)) {
            if (pDeviceInfo->DeviceType == 1) lstrcatA(m_strDeviceStats, " (pure hw vp)");
            else lstrcatA(m_strDeviceStats, " (simulated pure hw vp)");
        } else if (behaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING) {
            if (pDeviceInfo->DeviceType == 1) lstrcatA(m_strDeviceStats, " (hw vp)");
            else lstrcatA(m_strDeviceStats, " (simulated hw vp)");
        } else if (behaviorFlags & D3DCREATE_MIXED_VERTEXPROCESSING) {
            if (pDeviceInfo->DeviceType == 1) lstrcatA(m_strDeviceStats, " (mixed vp)");
            else lstrcatA(m_strDeviceStats, " (simulated mixed vp)");
        } else if (behaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) {
            lstrcatA(m_strDeviceStats, " (sw vp)");
        }
        if (pDeviceInfo->DeviceType == 1) {
            lstrcatA(m_strDeviceStats, ": ");
            char szDescription[512];
            DXUtil_ConvertAnsiStringToGenericCch(szDescription, pAdapterInfo->AdapterIdentifier.Description, 512);
            int maxAppend = 90 - 1 - lstrlenA(m_strDeviceStats);
            _mbsnbcat((unsigned char*)m_strDeviceStats, (unsigned char*)szDescription, maxAppend);
        }
        IDirect3DSurface9* pBackBuffer = NULL;
        m_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
        pBackBuffer->GetDesc(&m_d3dsdBackBuffer);
        pBackBuffer->Release();
        GetClassLongA(m_hWnd, -12);
        if (m_bShowCursorWhenFullscreen && !m_bWindowed) m_pd3dDevice->ShowCursor(TRUE);
        else m_pd3dDevice->ShowCursor(FALSE);
        if (m_bClipCursorWhenFullscreen) {
            if (!m_bWindowed) {
                RECT rcWindow;
                GetWindowRect(m_hWnd, &rcWindow);
                ClipCursor(&rcWindow);
            } else {
                ClipCursor(NULL);
            }
        }
        hr = InitDeviceObjects();
        if (FAILED(hr)) {
            DeleteDeviceObjects();
        } else {
            m_bDeviceObjectsInited = true;
            hr = RestoreDeviceObjects();
            if (FAILED(hr)) {
                InvalidateDeviceObjects();
            } else {
                m_bDeviceObjectsRestored = true;
                return S_OK;
            }
        }
        Cleanup3DEnvironment();
    }
    if (hr != 0x8200000b && hr != 0x80070002 && pDeviceInfo->DeviceType == 1) {
        if (FindBestWindowedMode(false, true)) {
            m_bWindowed = true;
            AdjustWindowForChange();
            SetWindowPos(m_hWnd, HWND_NOTOPMOST, m_rcWindowBounds.left, m_rcWindowBounds.top,
                         (m_rcWindowBounds.right - m_rcWindowBounds.left), (m_rcWindowBounds.bottom - m_rcWindowBounds.top),
                         SWP_SHOWWINDOW);
            DisplayErrorMsg(hr, 2);
            hr = Initialize3DEnvironment();
        }
    }
    return hr;
}
