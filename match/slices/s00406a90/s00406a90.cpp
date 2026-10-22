// Slice s00406a90: Direct3D settings dialog handlers (part 3), dialog procedure and entry point.
#include <stdio.h>
#include "../s00405890/d3ddlg.h"

// 0x405890 ComboSelectData, 0x405710 ComboSelectIndex, 0x407ba0 ArrayList::GetPtr,
// 0x405920 OnAdapterChanged, 0x405ac0 OnWindowedFullscreenChanged, 0x405840 OnRefreshRateChanged,
// 0x405e00 OnAdapterFormatChanged, 0x4061c0 OnResolutionChanged, 0x406370 OnBackBufferFormatChanged,
// 0x4068c0 OnDepthStencilChanged (declared in d3ddlg.h)
// 0x407a30 CopyString (declared in d3ddlg.h)

// 0x5d6f54: the dialog object used by the static dialog procedure
D3DSettingsDlg* g_settingsDlg; // 0x5d6f54

// @ 0x406a90
void D3DSettingsDlg::OnMultisampleTypeChanged()
{
    char buf[100];
    HWND hMS = GetDlgItem(hDlg, IDC_MULTISAMPLE_TYPE_COMBO);
    int sel = (int)SendMessage(hMS, CB_GETCURSEL, 0, 0);
    int msType;
    if (sel < 0)
        msType = 0;
    else
        msType = (int)SendMessage(hMS, CB_GETITEMDATA, sel, 0);
    if (windowed == 0)
        fs.msType = msType;
    else
        win.msType = msType;

    DeviceCombo* combo = windowed ? win.combo : fs.combo;
    unsigned maxQuality = 0;
    for (unsigned i = 0; i < combo->multisampleTypeList->count; i++) {
        if (*(int*)combo->multisampleTypeList->GetPtr(i) == msType) {
            maxQuality = *(unsigned*)combo->qualityList->GetPtr(i);
            break;
        }
    }
    HWND hReset = GetDlgItem(hDlg, IDC_MULTISAMPLE_QUALITY_COMBO);
    SendMessage(hReset, CB_RESETCONTENT, 0, 0);
    for (unsigned q = 0; q < maxQuality; q++) {
        wsprintfA(buf, "%d", q);
        HWND hItem = GetDlgItem(hDlg, IDC_MULTISAMPLE_QUALITY_COMBO);
        int idx = (int)SendMessage(hItem, CB_ADDSTRING, 0, (LPARAM)buf);
        SendMessage(hItem, CB_SETITEMDATA, idx, q);
        unsigned cur = windowed ? win.msQuality : fs.msQuality;
        if (q == cur)
            ComboSelectData(IDC_MULTISAMPLE_QUALITY_COMBO, q);
    }
    HWND hCur = GetDlgItem(hDlg, IDC_MULTISAMPLE_QUALITY_COMBO);
    if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_MULTISAMPLE_QUALITY_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
            HWND h = GetDlgItem(hDlg, IDC_MULTISAMPLE_QUALITY_COMBO);
            SendMessage(h, CB_SETCURSEL, 0, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MULTISAMPLE_QUALITY_COMBO, CBN_SELCHANGE), (LPARAM)h);
        }
    }
}

// @ 0x406c40
void D3DSettingsDlg::OnMultisampleQualityChanged()
{
    HWND h = GetDlgItem(hDlg, IDC_MULTISAMPLE_QUALITY_COMBO);
    int sel = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
    int q;
    if (sel < 0)
        q = 0;
    else
        q = (int)SendMessage(h, CB_GETITEMDATA, sel, 0);
    if (windowed)
        win.msQuality = q;
    else
        fs.msQuality = q;
}

// @ 0x406c90
void D3DSettingsDlg::OnVertexProcessingChanged()
{
    HWND h = GetDlgItem(hDlg, IDC_VERTEXPROCESSING_COMBO);
    int sel = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
    int vp;
    if (sel < 0)
        vp = 0;
    else
        vp = (int)SendMessage(h, CB_GETITEMDATA, sel, 0);
    if (windowed)
        win.vertexProc = vp;
    else
        fs.vertexProc = vp;
}

// @ 0x406ce0
void D3DSettingsDlg::OnPresentIntervalChanged()
{
    HWND h = GetDlgItem(hDlg, IDC_PRESENTINTERVAL_COMBO);
    int sel = (int)SendMessage(h, CB_GETCURSEL, 0, 0);
    unsigned pi;
    if (sel < 0)
        pi = 0;
    else
        pi = (unsigned)SendMessage(h, CB_GETITEMDATA, sel, 0);
    if (windowed)
        win.presentInterval = pi;
    else
        fs.presentInterval = pi;
}

// @ 0x406d30
void D3DSettingsDlg::OnDeviceChanged()
{
    HWND hDevice = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
    int sel = (int)SendMessage(hDevice, CB_GETCURSEL, 0, 0);
    if (sel < 0)
        return;
    DeviceInfo* device = (DeviceInfo*)SendMessage(hDevice, CB_GETITEMDATA, sel, 0);
    if (device == 0)
        return;
    if (windowed)
        win.device = device;
    else
        fs.device = device;
    bool hasWindowed = false;
    bool hasFullscreen = false;
    for (unsigned i = 0; i < device->combos->count; i++) {
        DeviceCombo* combo = (DeviceCombo*)device->combos->GetPtr(i);
        if (combo->windowed)
            hasWindowed = true;
        else
            hasFullscreen = true;
    }
    EnableWindow(GetDlgItem(hDlg, IDC_WINDOWED), hasWindowed);
    EnableWindow(GetDlgItem(hDlg, IDC_FULLSCREEN), hasFullscreen);
    if (windowed && hasWindowed)
        CheckRadioButton(hDlg, IDC_WINDOWED, IDC_FULLSCREEN, IDC_WINDOWED);
    else
        CheckRadioButton(hDlg, IDC_WINDOWED, IDC_FULLSCREEN, IDC_FULLSCREEN);
    OnWindowedFullscreenChanged();
}

// @ 0x406e50
int D3DSettingsDlg::DlgProc(HWND hwnd, unsigned msg, unsigned wParam, long lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
        char text[512];
        hDlg = hwnd;
        for (unsigned i = 0; i < app->adapters->count; i++) {
            AdapterInfo* adapter = (AdapterInfo*)app->adapters->GetPtr(i);
            CopyString(text, adapter->description, 512);
            HWND hAdapter = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
            int idx = (int)SendMessage(hAdapter, CB_ADDSTRING, 0, (LPARAM)text);
            SendMessage(hAdapter, CB_SETITEMDATA, idx, (LPARAM)adapter);
            DeviceCombo* cur = windowed ? win.combo : fs.combo;
            if (adapter->adapterOrdinal == cur->adapterOrdinal)
                ComboSelectData(IDC_ADAPTER_COMBO, (int)adapter);
        }
        HWND hCur = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
        if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
            HWND hCount = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
            if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
                HWND h = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
                SendMessage(h, CB_SETCURSEL, 0, 0);
                PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_ADAPTER_COMBO, CBN_SELCHANGE), (LPARAM)h);
            }
        }
        break;
    }
    case WM_COMMAND: {
        switch ((unsigned short)wParam) {
        case 1:
            EndDialog(hwnd, 1);
            return 1;
        case 2:
            EndDialog(hwnd, 2);
            return 1;
        case IDC_DEVICE_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnDeviceChanged();
                return 1;
            }
            break;
        case IDC_ADAPTER_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnAdapterChanged();
                return 1;
            }
            break;
        case IDC_ADAPTERFORMAT_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnAdapterFormatChanged();
                return 1;
            }
            break;
        case IDC_RESOLUTION_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnResolutionChanged();
                return 1;
            }
            break;
        case IDC_MULTISAMPLE_TYPE_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnMultisampleTypeChanged();
                return 1;
            }
            break;
        case IDC_REFRESHRATE_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnRefreshRateChanged();
                return 1;
            }
            break;
        case IDC_BACKBUFFERFORMAT_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnBackBufferFormatChanged();
                return 1;
            }
            break;
        case IDC_DEPTHSTENCIL_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnDepthStencilChanged();
                return 1;
            }
            break;
        case IDC_MULTISAMPLE_QUALITY_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnMultisampleQualityChanged();
                return 1;
            }
            break;
        case IDC_VERTEXPROCESSING_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnVertexProcessingChanged();
                return 1;
            }
            break;
        case IDC_PRESENTINTERVAL_COMBO:
            if ((short)(wParam >> 16) == CBN_SELCHANGE) {
                OnPresentIntervalChanged();
                return 1;
            }
            break;
        case IDC_WINDOWED:
        case IDC_FULLSCREEN:
            OnWindowedFullscreenChanged();
            return 1;
        }
        break;
    }
    default:
        return 0;
    }
    return 1;
}

// @ 0x407220
BOOL __stdcall SettingsDlgProcThunk(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return g_settingsDlg->DlgProc(hwnd, msg, wParam, lParam);
}

// @ 0x407230
void __stdcall ShowD3DSettingsDialog(HWND parent)
{
    DialogBoxParamA(0, (const char*)0x90, parent, SettingsDlgProcThunk, 0);
}
