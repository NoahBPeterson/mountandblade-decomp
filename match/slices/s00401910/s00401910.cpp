// Slice s00401910: Application constructor/destructor and the device-selection helpers.
#include "s00401910.h"

// Depth/stencil format preference (higher is better): D32, D24X8, D24S8, D24X4S4, D16, D16_LOCKABLE, D15S1.
static inline int depth_rank(int fmt)
{
    int r = 0;
    if (fmt == 0x47) r = 10;
    else if (fmt == 0x4d) r = 9;
    else if (fmt == 0x4b) r = 8;
    else if (fmt == 0x4f) r = 7;
    else if (fmt == 0x50) r = 6;
    else if (fmt == 0x46) r = 5;
    else if (fmt == 0x49) r = 4;
    return r;
}

// 0x401480 (defined out of line in s00401010; repeated here so the constructor inlines it)
void Application::Pause(bool bPause)
{
    static DWORD dwAppPausedCount = 0L;     // 0x5d6f44
    dwAppPausedCount += (bPause ? +1 : -1);
    m_bActive = (dwAppPausedCount ? false : true);
    if (bPause && (1 == dwAppPausedCount)) {
        if (m_bFrameMoving) GetTimer(2);
    }
    if (0 == dwAppPausedCount) {
        if (m_bFrameMoving) GetTimer(1);
    }
}

// @ 0x401910
Application::Application()
{
    g_app = this;
    b4 = false;
    fTime8 = 0.0f;
    m_pD3D = 0;
    m_pd3dDevice = 0;
    m_hWnd = 0;
    m_hWndFocus = 0;
    m_hMenu = 0;
    m_bWindowed = true;
    m_bDeviceLost = false;
    m_bMinimized = false;
    m_bMaximized = false;
    m_bIgnoreSizeChange = false;
    m_bDeviceObjectsInited = false;
    m_bDeviceObjectsRestored = false;
    m_dwCreateFlags = 0;
    m_bFrameMoving = true;
    m_bSingleStep = false;
    m_fTime = 0.0f;
    m_fElapsedTime = 0.0f;
    m_fFPS = 0.0f;
    m_strDeviceStats[0] = 0;
    m_strFrameStats[0] = 0;
    i334 = 0;
    m_dwCreationWidth = 400;
    m_dwCreationHeight = 300;
    m_bShowCursorWhenFullscreen = 0;
    m_bStartFullscreen = 0;
    Pause(true);
    m_bClipCursorWhenFullscreen = 1;
}

// @ 0x401a20
int Application::ConfirmDevice(int, int, int, int) { return 0; }

// @ 0x401a30
Application::~Application() {}

// @ 0x401a60
bool Application::FindBestWindowedMode(bool requireHAL, bool requireREF)
{
    D3DDISPLAYMODE mode;
    m_pD3D->GetAdapterDisplayMode(0, &mode);
    D3DAdapterInfo* bestAdapter = 0;
    D3DDeviceInfo* bestDevice = 0;
    D3DDeviceCombo* best = 0;
    for (unsigned ia = 0; ia < m_d3dEnum.m_pAdapterInfoList->count; ia++) {
        D3DAdapterInfo* ai = (D3DAdapterInfo*)m_d3dEnum.m_pAdapterInfoList->get(ia);
        for (unsigned id = 0; id < ai->pDeviceInfoList->count; id++) {
            D3DDeviceInfo* di = (D3DDeviceInfo*)ai->pDeviceInfoList->get(id);
            if (requireHAL && di->DeviceType != 1) continue;
            if (requireREF && di->DeviceType != 2) continue;
            for (unsigned ic = 0; ic < di->pDeviceComboList->count; ic++) {
                D3DDeviceCombo* dc = (D3DDeviceCombo*)di->pDeviceComboList->get(ic);
                bool bbMatches = dc->BackBufferFormat == dc->AdapterFormat;
                if (dc->IsWindowed && dc->AdapterFormat == mode.Format) {
                    if (best == 0 || (best->DeviceType != 1 && dc->DeviceType == 1) ||
                        (dc->DeviceType == 1 && bbMatches)) {
                        bestAdapter = ai;
                        bestDevice = di;
                        best = dc;
                        if (dc->DeviceType == 1 && bbMatches) goto done;
                    }
                }
            }
        }
    }
done:
    if (best == 0) return false;
    m_d3dSettings.pWindowed_AdapterInfo = bestAdapter;
    m_d3dSettings.pWindowed_DeviceInfo = bestDevice;
    m_d3dSettings.Windowed_DisplayMode = mode;
    m_d3dSettings.Window_Width = m_rcWindowClient.right - m_rcWindowClient.left;
    m_d3dSettings.pWindowed_DeviceCombo = best;
    m_d3dSettings.IsWindowed = true;
    m_d3dSettings.Window_Height = m_rcWindowClient.bottom - m_rcWindowClient.top;
    if (m_d3dEnum.AppUsesDepthBuffer) {
        int bestFmt = *(int*)best->pDepthStencilFormatList->get(0);
        for (unsigned i = 1; i < best->pDepthStencilFormatList->count; i++) {
            int f = *(int*)best->pDepthStencilFormatList->get(i);
            if (depth_rank(f) > depth_rank(bestFmt)) bestFmt = f;
        }
        m_d3dSettings.Windowed_DepthStencilBufferFormat = bestFmt;
    }
    m_d3dSettings.Windowed_MultisampleType = *(int*)best->pMultiSampleTypeList->get(0);
    m_d3dSettings.Windowed_MultisampleQuality = 0;
    m_d3dSettings.Windowed_VertexProcessingType = *(int*)best->pVertexProcessingTypeList->get(0);
    m_d3dSettings.Windowed_PresentInterval = *(int*)best->pPresentIntervalList->get(0);
    return true;
}

// @ 0x401d40
bool Application::FindBestFullscreenMode(bool requireHAL, bool requireREF)
{
    D3DDISPLAYMODE desk;
    D3DDISPLAYMODE bestDesk = {0, 0, 0, (D3DFORMAT)0};
    D3DAdapterInfo* bestAdapter = 0;
    D3DDeviceInfo* bestDevice = 0;
    D3DDeviceCombo* best = 0;
    for (unsigned ia = 0; ia < m_d3dEnum.m_pAdapterInfoList->count; ia++) {
        D3DAdapterInfo* ai = (D3DAdapterInfo*)m_d3dEnum.m_pAdapterInfoList->get(ia);
        m_pD3D->GetAdapterDisplayMode(ai->AdapterOrdinal, &desk);
        for (unsigned id = 0; id < ai->pDeviceInfoList->count; id++) {
            D3DDeviceInfo* di = (D3DDeviceInfo*)ai->pDeviceInfoList->get(id);
            if (requireHAL && di->DeviceType != 1) continue;
            if (requireREF && di->DeviceType != 2) continue;
            for (unsigned ic = 0; ic < di->pDeviceComboList->count; ic++) {
                D3DDeviceCombo* dc = (D3DDeviceCombo*)di->pDeviceComboList->get(ic);
                bool bbMatches = dc->BackBufferFormat == dc->AdapterFormat;
                bool deskMatches = dc->AdapterFormat == desk.Format;
                if (!dc->IsWindowed) {
                    if (best == 0 || (best->DeviceType != 1 && di->DeviceType == 1) ||
                        (dc->DeviceType == 1 && best->AdapterFormat != desk.Format && deskMatches) ||
                        (dc->DeviceType == 1 && deskMatches && bbMatches)) {
                        bestDesk = desk;
                        bestAdapter = ai;
                        bestDevice = di;
                        best = dc;
                        if (di->DeviceType == 1 && deskMatches && bbMatches) goto done;
                    }
                }
            }
        }
    }
done:
    if (best == 0) return false;
    D3DDISPLAYMODE bm;
    bm.Width = 0; bm.Height = 0; bm.Format = D3DFMT_UNKNOWN; bm.RefreshRate = 0;
    for (unsigned i = 0; i < bestAdapter->pDisplayModeList->count; i++) {
        D3DDISPLAYMODE* pdm = (D3DDISPLAYMODE*)bestAdapter->pDisplayModeList->get(i);
        if (pdm->Format != best->AdapterFormat) continue;
        if (pdm->Width == bestDesk.Width && pdm->Height == bestDesk.Height && pdm->RefreshRate == bestDesk.RefreshRate) {
            bm = *pdm;
            break;
        } else if (pdm->Width == bestDesk.Width && pdm->Height == bestDesk.Height && pdm->RefreshRate > bm.RefreshRate) {
            bm = *pdm;
        } else if (pdm->Width == bestDesk.Width) {
            bm = *pdm;
        } else if (bm.Width == 0) {
            bm = *pdm;
        }
    }
    m_d3dSettings.pFullscreen_AdapterInfo = bestAdapter;
    m_d3dSettings.pFullscreen_DeviceInfo = bestDevice;
    m_d3dSettings.Fullscreen_DisplayMode = bm;
    m_d3dSettings.pFullscreen_DeviceCombo = best;
    m_d3dSettings.IsWindowed = false;
    if (m_d3dEnum.AppUsesDepthBuffer) {
        int bestFmt = *(int*)best->pDepthStencilFormatList->get(0);
        for (unsigned i = 1; i < best->pDepthStencilFormatList->count; i++) {
            int f = *(int*)best->pDepthStencilFormatList->get(i);
            if (depth_rank(f) > depth_rank(bestFmt)) bestFmt = f;
        }
        m_d3dSettings.Fullscreen_DepthStencilBufferFormat = bestFmt;
    }
    m_d3dSettings.Fullscreen_MultisampleType = *(int*)best->pMultiSampleTypeList->get(0);
    m_d3dSettings.Fullscreen_MultisampleQuality = 0;
    m_d3dSettings.Fullscreen_VertexProcessingType = *(int*)best->pVertexProcessingTypeList->get(0);
    m_d3dSettings.Fullscreen_PresentInterval = 0;
    return true;
}

// @ 0x402100
int Application::ChooseInitialD3DSettings()
{
    bool fs = FindBestFullscreenMode(false, false);
    bool win = FindBestWindowedMode(false, false);
    if (m_bStartFullscreen && fs) m_d3dSettings.IsWindowed = false;
    if (!win) {
        if (!fs) return 0x82000003;
        m_d3dSettings.IsWindowed = win;
    }
    if (!fs && !win) return 0x82000003;
    return 0;
}

// @ 0x402150
int Application::HandlePossibleSizeChange()
{
    HRESULT hr = S_OK;
    RECT rcClientOld;
    rcClientOld = m_rcWindowClient;
    if (m_bIgnoreSizeChange) return S_OK;
    GetWindowRect(m_hWnd, &m_rcWindowBounds);
    GetClientRect(m_hWnd, &m_rcWindowClient);
    if (rcClientOld.right - rcClientOld.left != m_rcWindowClient.right - m_rcWindowClient.left ||
        rcClientOld.bottom - rcClientOld.top != m_rcWindowClient.bottom - m_rcWindowClient.top) {
        Pause(true);
        m_d3dpp.BackBufferWidth = m_rcWindowClient.right - m_rcWindowClient.left;
        m_d3dpp.BackBufferHeight = m_rcWindowClient.bottom - m_rcWindowClient.top;
        if (m_pd3dDevice != NULL) {
            if (FAILED(hr = Reset3DEnvironment())) {
                if (hr != D3DERR_OUTOFVIDEOMEMORY)
                    hr = 0x8200000c;
                DisplayErrorMsg(hr, 1);
            }
        }
        Pause(false);
    }
    return hr;
}
