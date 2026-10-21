// Slice s004061c0: Direct3D settings dialog handlers (part 2).
#include <stdio.h>
#include "../s00405890/d3ddlg.h"

// 0x405760 ComboSelectedData, 0x4057a0 ComboContains, 0x4056d0 ComboAdd, 0x405710 ComboSelectIndex,
// 0x405890 ComboSelectData, 0x407ba0 ArrayList::GetPtr, 0x407250 FormatToString, 0x4055e0 MultisampleTypeToString

// @ 0x4061c0
void D3DSettingsDlg::OnResolutionChanged()
{
    if (windowed == 0) {
        char buf[50];
        HWND hAdapter = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
        int sel = (int)SendMessage(hAdapter, CB_GETCURSEL, 0, 0);
        if (sel >= 0) {
            AdapterInfo* adapter = (AdapterInfo*)SendMessage(hAdapter, CB_GETITEMDATA, sel, 0);
            if (adapter != 0) {
                unsigned res = ComboSelectedData(IDC_RESOLUTION_COMBO);
                unsigned width = (unsigned short)res;
                unsigned height = (unsigned short)(res >> 16);
                fs.mode.width = width;
                fs.mode.height = height;
                unsigned adapterFormat = ComboSelectedData(IDC_ADAPTERFORMAT_COMBO);
                HWND hReset = GetDlgItem(hDlg, IDC_REFRESHRATE_COMBO);
                SendMessage(hReset, CB_RESETCONTENT, 0, 0);
                for (unsigned i = 0; i < adapter->displayModes->count; i++) {
                    DisplayMode m = *(DisplayMode*)adapter->displayModes->GetPtr(i);
                    unsigned hz = m.refreshRate;
                    if (m.format == adapterFormat && m.width == width && m.height == height) {
                        if (hz == 0)
                            lstrcpyA(buf, "Default Rate");
                        else
                            _snprintf(buf, 50, "%d Hz", hz);
                        buf[49] = 0;
                        if (!ComboContains(IDC_REFRESHRATE_COMBO, buf)) {
                            ComboAdd(IDC_REFRESHRATE_COMBO, hz, buf);
                            if (fs.mode.refreshRate == hz)
                                ComboSelectData(IDC_REFRESHRATE_COMBO, hz);
                        }
                    }
                }
                HWND hCur = GetDlgItem(hDlg, IDC_REFRESHRATE_COMBO);
                if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
                    HWND hCount = GetDlgItem(hDlg, IDC_REFRESHRATE_COMBO);
                    if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0)
                        ComboSelectIndex(IDC_REFRESHRATE_COMBO, 0);
                }
            }
        }
    }
}

// @ 0x406370
void D3DSettingsDlg::OnBackBufferFormatChanged()
{
    HWND hDevice = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
    int sel = (int)SendMessage(hDevice, CB_GETCURSEL, 0, 0);
    DeviceInfo* device;
    if (sel < 0)
        device = 0;
    else
        device = (DeviceInfo*)SendMessage(hDevice, CB_GETITEMDATA, sel, 0);
    HWND hFormat = GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO);
    sel = (int)SendMessage(hFormat, CB_GETCURSEL, 0, 0);
    int adapterFormat;
    if (sel < 0)
        adapterFormat = 0;
    else
        adapterFormat = (int)SendMessage(hFormat, CB_GETITEMDATA, sel, 0);
    HWND hBB = GetDlgItem(hDlg, IDC_BACKBUFFERFORMAT_COMBO);
    sel = (int)SendMessage(hBB, CB_GETCURSEL, 0, 0);
    int bbFormat;
    if (sel < 0)
        bbFormat = 0;
    else
        bbFormat = (int)SendMessage(hBB, CB_GETITEMDATA, sel, 0);
    if (device == 0)
        return;

    unsigned i = 0;
    DeviceCombo* combo;
    if (device->combos->count <= 0)
        return;
    while (combo = (DeviceCombo*)device->combos->GetPtr(i),
           combo->windowed != windowed || combo->adapterFormat != adapterFormat || combo->backBufferFormat != bbFormat) {
        i++;
        if (i >= device->combos->count)
            return;
    }
    if (windowed)
        win.combo = combo;
    else
        fs.combo = combo;

    HWND hReset = GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO);
    SendMessage(hReset, CB_RESETCONTENT, 0, 0);
    if (app->allowVertexProc != 0) {
        for (unsigned j = 0; j < combo->depthStencilList->count; j++) {
            int fmt = *(int*)combo->depthStencilList->GetPtr(j);
            const char* text = FormatToString(fmt, 1);
            HWND hDS = GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO);
            int idx = (int)SendMessage(hDS, CB_ADDSTRING, 0, (LPARAM)text);
            SendMessage(hDS, CB_SETITEMDATA, idx, fmt);
            int cur = windowed ? win.depthStencil : fs.depthStencil;
            if (fmt == cur)
                ComboSelectData(IDC_DEPTHSTENCIL_COMBO, fmt);
        }
        HWND hCur = GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO);
        if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
            HWND hCount = GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO);
            if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0)
                ComboSelectIndex(IDC_DEPTHSTENCIL_COMBO, 0);
        }
    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO), FALSE);
        HWND hDS = GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO);
        int idx = (int)SendMessage(hDS, CB_ADDSTRING, 0, (LPARAM)"(not used)");
        SendMessage(hDS, CB_SETITEMDATA, idx, 0);
        ComboSelectIndex(IDC_DEPTHSTENCIL_COMBO, 0);
    }

    HWND hVP = GetDlgItem(hDlg, IDC_VERTEXPROCESSING_COMBO);
    SendMessage(hVP, CB_RESETCONTENT, 0, 0);
    for (unsigned j = 0; j < combo->vertexProcList->count; j++) {
        int vp = *(int*)combo->vertexProcList->GetPtr(j);
        const char* text;
        switch (vp) {
        case 0: text = "SOFTWARE_VP"; break;
        case 1: text = "MIXED_VP"; break;
        case 2: text = "HARDWARE_VP"; break;
        case 3: text = "PURE_HARDWARE_VP"; break;
        default: text = "Unknown VertexProcessingType";
        }
        HWND hItem = GetDlgItem(hDlg, IDC_VERTEXPROCESSING_COMBO);
        int idx = (int)SendMessage(hItem, CB_ADDSTRING, 0, (LPARAM)text);
        SendMessage(hItem, CB_SETITEMDATA, idx, vp);
        int cur = windowed ? win.vertexProc : fs.vertexProc;
        if (vp == cur)
            ComboSelectData(IDC_VERTEXPROCESSING_COMBO, vp);
    }
    HWND hCur = GetDlgItem(hDlg, IDC_VERTEXPROCESSING_COMBO);
    if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_VERTEXPROCESSING_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
            HWND h = GetDlgItem(hDlg, IDC_VERTEXPROCESSING_COMBO);
            SendMessage(h, CB_SETCURSEL, 0, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_VERTEXPROCESSING_COMBO, CBN_SELCHANGE), (LPARAM)h);
        }
    }

    HWND hPI = GetDlgItem(hDlg, IDC_PRESENTINTERVAL_COMBO);
    SendMessage(hPI, CB_RESETCONTENT, 0, 0);
    for (unsigned j = 0; j < combo->presentIntervalList->count; j++) {
        unsigned pi = *(unsigned*)combo->presentIntervalList->GetPtr(j);
        const char* text;
        switch (pi) {
        case 0: text = "D3DPRESENT_INTERVAL_DEFAULT"; break;
        case 1: text = "D3DPRESENT_INTERVAL_ONE"; break;
        case 2: text = "D3DPRESENT_INTERVAL_TWO"; break;
        case 4: text = "D3DPRESENT_INTERVAL_THREE"; break;
        case 8: text = "D3DPRESENT_INTERVAL_FOUR"; break;
        case 0x80000000: text = "D3DPRESENT_INTERVAL_IMMEDIATE"; break;
        default: text = "Unknown PresentInterval";
        }
        HWND hItem = GetDlgItem(hDlg, IDC_PRESENTINTERVAL_COMBO);
        int idx = (int)SendMessage(hItem, CB_ADDSTRING, 0, (LPARAM)text);
        SendMessage(hItem, CB_SETITEMDATA, idx, pi);
        unsigned cur = windowed ? win.presentInterval : fs.presentInterval;
        if (pi == cur)
            ComboSelectData(IDC_PRESENTINTERVAL_COMBO, pi);
    }
    HWND hCur2 = GetDlgItem(hDlg, IDC_PRESENTINTERVAL_COMBO);
    if ((int)SendMessage(hCur2, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_PRESENTINTERVAL_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
            HWND h = GetDlgItem(hDlg, IDC_PRESENTINTERVAL_COMBO);
            SendMessage(h, CB_SETCURSEL, 0, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_PRESENTINTERVAL_COMBO, CBN_SELCHANGE), (LPARAM)h);
        }
    }
}

// @ 0x4068c0
void D3DSettingsDlg::OnDepthStencilChanged()
{
    HWND hDS = GetDlgItem(hDlg, IDC_DEPTHSTENCIL_COMBO);
    int sel = (int)SendMessage(hDS, CB_GETCURSEL, 0, 0);
    int dsFormat;
    if (sel < 0)
        dsFormat = 0;
    else
        dsFormat = (int)SendMessage(hDS, CB_GETITEMDATA, sel, 0);
    if (app->allowVertexProc != 0) {
        if (windowed)
            win.depthStencil = dsFormat;
        else
            fs.depthStencil = dsFormat;
    }
    DeviceCombo* combo = windowed ? win.combo : fs.combo;

    HWND hReset = GetDlgItem(hDlg, IDC_MULTISAMPLE_TYPE_COMBO);
    SendMessage(hReset, CB_RESETCONTENT, 0, 0);
    for (unsigned i = 0; i < combo->multisampleTypeList->count; i++) {
        int msType = *(int*)combo->multisampleTypeList->GetPtr(i);
        bool conflict = false;
        for (unsigned j = 0; j < combo->conflictList->count; j++) {
            int* c = (int*)combo->conflictList->GetPtr(j);
            if (c[0] == dsFormat && c[1] == msType) {
                conflict = true;
                break;
            }
        }
        if (!conflict) {
            HWND hItem = GetDlgItem(hDlg, IDC_MULTISAMPLE_TYPE_COMBO);
            const char* text = MultisampleTypeToString(msType);
            int idx = (int)SendMessage(hItem, CB_ADDSTRING, 0, (LPARAM)text);
            SendMessage(hItem, CB_SETITEMDATA, idx, msType);
            int cur = windowed ? win.msType : fs.msType;
            if (msType == cur)
                ComboSelectData(IDC_MULTISAMPLE_TYPE_COMBO, msType);
        }
    }
    HWND hCur = GetDlgItem(hDlg, IDC_MULTISAMPLE_TYPE_COMBO);
    if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_MULTISAMPLE_TYPE_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
            HWND h = GetDlgItem(hDlg, IDC_MULTISAMPLE_TYPE_COMBO);
            SendMessage(h, CB_SETCURSEL, 0, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MULTISAMPLE_TYPE_COMBO, CBN_SELCHANGE), (LPARAM)h);
        }
    }
}
