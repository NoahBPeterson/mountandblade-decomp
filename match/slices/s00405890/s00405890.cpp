// Slice s00405890: Direct3D settings dialog handlers (part 1).
#include <stdio.h>
#include "d3ddlg.h"

// 0x407ba0
// void* ArrayList::GetPtr(unsigned i);
// 0x4056d0 ComboAdd, 0x405710 ComboSelectIndex, 0x4057a0 ComboContains (declared in d3ddlg.h)
// 0x4055e0
// const char* MultisampleTypeToString(int);
// 0x407250
// const char* FormatToString(int, int);

// @ 0x405890
void D3DSettingsDlg::ComboSelectData(int id, int data)
{
    HWND hCombo = GetDlgItem(hDlg, id);
    HWND hCombo2 = GetDlgItem(hDlg, id);
    unsigned count = SendMessage(hCombo2, CB_GETCOUNT, 0, 0);
    for (unsigned i = 0; i < count; i++) {
        if (SendMessage(hCombo, CB_GETITEMDATA, i, 0) == data) {
            SendMessage(hCombo, CB_SETCURSEL, i, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(id, CBN_SELCHANGE), (LPARAM)hCombo);
            return;
        }
    }
}

// @ 0x405920
void D3DSettingsDlg::OnAdapterChanged()
{
    HWND hAdapter = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
    int sel = (int)SendMessage(hAdapter, CB_GETCURSEL, 0, 0);
    if (sel < 0)
        return;
    AdapterInfo* adapter = (AdapterInfo*)SendMessage(hAdapter, CB_GETITEMDATA, sel, 0);
    if (adapter == 0)
        return;
    if (windowed)
        win.adapter = adapter;
    else
        fs.adapter = adapter;

    HWND hReset = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
    SendMessage(hReset, CB_RESETCONTENT, 0, 0);
    for (unsigned i = 0; i < adapter->devices->count; i++) {
        DeviceInfo* dev = (DeviceInfo*)adapter->devices->GetPtr(i);
        const char* name;
        switch (dev->deviceType) {
        case 1: name = "D3DDEVTYPE_HAL"; break;
        case 2: name = "D3DDEVTYPE_REF"; break;
        case 3: name = "D3DDEVTYPE_SW"; break;
        default: name = "Unknown devType";
        }
        HWND hDevice = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
        int idx = (int)SendMessage(hDevice, CB_ADDSTRING, 0, (LPARAM)name);
        SendMessage(hDevice, CB_SETITEMDATA, idx, (LPARAM)dev);
        DeviceCombo* cur = windowed ? win.combo : fs.combo;
        if (dev->deviceType == cur->deviceType)
            ComboSelectData(IDC_DEVICE_COMBO, (int)dev);
    }
    HWND hCur = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
    if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
            HWND h = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
            SendMessage(h, CB_SETCURSEL, 0, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DEVICE_COMBO, CBN_SELCHANGE), (LPARAM)h);
        }
    }
}

// @ 0x405ac0
void D3DSettingsDlg::OnWindowedFullscreenChanged()
{
    HWND hAdapter = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
    int sel = (int)SendMessage(hAdapter, CB_GETCURSEL, 0, 0);
    AdapterInfo* adapter;
    if (sel < 0)
        adapter = 0;
    else
        adapter = (AdapterInfo*)SendMessage(hAdapter, CB_GETITEMDATA, sel, 0);
    HWND hDevice = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
    sel = (int)SendMessage(hDevice, CB_GETCURSEL, 0, 0);
    DeviceInfo* device;
    if (sel < 0)
        device = 0;
    else
        device = (DeviceInfo*)SendMessage(hDevice, CB_GETITEMDATA, sel, 0);
    if (adapter == 0 || device == 0)
        return;

    if (IsDlgButtonChecked(hDlg, IDC_WINDOWED)) {
        char rate[50];
        char res[50];
        windowed = 1;
        win.adapter = adapter;
        win.device = device;
        HWND hFmt = GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO);
        SendMessage(hFmt, CB_RESETCONTENT, 0, 0);
        ComboAdd(IDC_ADAPTERFORMAT_COMBO, win.mode.format, FormatToString(win.mode.format, 1));
        ComboSelectIndex(IDC_ADAPTERFORMAT_COMBO, 0);
        EnableWindow(GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO), FALSE);

        unsigned packed = MAKELONG(win.mode.width, win.mode.height);
        _snprintf(res, 50, "%d by %d", win.mode.width, win.mode.height);
        res[49] = 0;
        HWND hRes = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
        SendMessage(hRes, CB_RESETCONTENT, 0, 0);
        ComboAdd(IDC_RESOLUTION_COMBO, packed, res);
        ComboSelectIndex(IDC_RESOLUTION_COMBO, 0);
        EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_COMBO), FALSE);

        if (win.mode.refreshRate == 0)
            lstrcpyA(rate, "Default Rate");
        else
            _snprintf(rate, 50, "%d Hz", win.mode.refreshRate);
        rate[49] = 0;
        HWND hRate = GetDlgItem(hDlg, IDC_REFRESHRATE_COMBO);
        SendMessage(hRate, CB_RESETCONTENT, 0, 0);
        ComboAdd(IDC_REFRESHRATE_COMBO, win.mode.refreshRate, rate);
        ComboSelectIndex(IDC_REFRESHRATE_COMBO, 0);
        EnableWindow(GetDlgItem(hDlg, IDC_REFRESHRATE_COMBO), FALSE);
        return;
    }

    windowed = 0;
    fs.adapter = adapter;
    fs.device = device;
    HWND hFmt = GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO);
    SendMessage(hFmt, CB_RESETCONTENT, 0, 0);
    for (unsigned i = 0; i < device->combos->count; i++) {
        DeviceCombo* combo = (DeviceCombo*)device->combos->GetPtr(i);
        if (combo->windowed == 0) {
            int format = combo->adapterFormat;
            if (!ComboContains(IDC_ADAPTERFORMAT_COMBO, FormatToString(format, 1))) {
                ComboAdd(IDC_ADAPTERFORMAT_COMBO, format, FormatToString(format, 1));
                if (format == fs.mode.format)
                    ComboSelectData(IDC_ADAPTERFORMAT_COMBO, format);
            }
        }
    }
    HWND hCur = GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO);
    if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0)
            ComboSelectIndex(IDC_ADAPTERFORMAT_COMBO, 0);
    }
    EnableWindow(GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_RESOLUTION_COMBO), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_REFRESHRATE_COMBO), TRUE);
}

// @ 0x405e00
void D3DSettingsDlg::OnAdapterFormatChanged()
{
    if (!IsDlgButtonChecked(hDlg, IDC_WINDOWED)) {
        char buf[50];
        HWND hAdapter = GetDlgItem(hDlg, IDC_ADAPTER_COMBO);
        int sel = (int)SendMessage(hAdapter, CB_GETCURSEL, 0, 0);
        AdapterInfo* adapter;
        if (sel < 0)
            adapter = 0;
        else
            adapter = (AdapterInfo*)SendMessage(hAdapter, CB_GETITEMDATA, sel, 0);
        HWND hFormat = GetDlgItem(hDlg, IDC_ADAPTERFORMAT_COMBO);
        sel = (int)SendMessage(hFormat, CB_GETCURSEL, 0, 0);
        int adapterFormat;
        if (sel < 0)
            adapterFormat = 0;
        else
            adapterFormat = (int)SendMessage(hFormat, CB_GETITEMDATA, sel, 0);
        fs.mode.format = adapterFormat;

        HWND hReset = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
        SendMessage(hReset, CB_RESETCONTENT, 0, 0);
        for (unsigned i = 0; i < adapter->displayModes->count; i++) {
            DisplayMode m = *(DisplayMode*)adapter->displayModes->GetPtr(i);
            if (m.format == adapterFormat) {
                unsigned packed = MAKELONG(m.width, m.height);
                _snprintf(buf, 50, "%d by %d", m.width, m.height);
                buf[49] = 0;
                if (!ComboContains(IDC_RESOLUTION_COMBO, buf)) {
                    HWND hRes = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
                    int idx = (int)SendMessage(hRes, CB_ADDSTRING, 0, (LPARAM)buf);
                    SendMessage(hRes, CB_SETITEMDATA, idx, packed);
                    if (fs.mode.width == m.width && fs.mode.height == m.height)
                        ComboSelectData(IDC_RESOLUTION_COMBO, packed);
                }
            }
        }
        HWND hCur = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
        if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
            HWND hCount = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
            if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
                HWND h = GetDlgItem(hDlg, IDC_RESOLUTION_COMBO);
                SendMessage(h, CB_SETCURSEL, 0, 0);
                PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_RESOLUTION_COMBO, CBN_SELCHANGE), (LPARAM)h);
            }
        }
    }

    HWND hDevice = GetDlgItem(hDlg, IDC_DEVICE_COMBO);
    int sel = (int)SendMessage(hDevice, CB_GETCURSEL, 0, 0);
    DeviceInfo* device;
    if (sel < 0)
        return;
    device = (DeviceInfo*)SendMessage(hDevice, CB_GETITEMDATA, sel, 0);
    if (device == 0)
        return;

    HWND hReset = GetDlgItem(hDlg, IDC_BACKBUFFERFORMAT_COMBO);
    SendMessage(hReset, CB_RESETCONTENT, 0, 0);
    for (unsigned i = 0; i < device->combos->count; i++) {
        DeviceCombo* combo = (DeviceCombo*)device->combos->GetPtr(i);
        if (combo->windowed == windowed) {
            DisplayMode* pm = &win.mode;
            if (windowed == 0)
                pm = &fs.mode;
            DisplayMode mode = *pm;
            if (combo->adapterFormat == mode.format) {
                if (!ComboContains(IDC_BACKBUFFERFORMAT_COMBO, FormatToString(combo->backBufferFormat, 1))) {
                    const char* text = FormatToString(combo->backBufferFormat, 1);
                    int bbFormat = combo->backBufferFormat;
                    HWND hBB = GetDlgItem(hDlg, IDC_BACKBUFFERFORMAT_COMBO);
                    int idx = (int)SendMessage(hBB, CB_ADDSTRING, 0, (LPARAM)text);
                    SendMessage(hBB, CB_SETITEMDATA, idx, bbFormat);
                    DeviceCombo* cur = windowed ? win.combo : fs.combo;
                    if (combo->backBufferFormat == cur->backBufferFormat)
                        ComboSelectData(IDC_BACKBUFFERFORMAT_COMBO, combo->backBufferFormat);
                }
            }
        }
    }
    HWND hCur = GetDlgItem(hDlg, IDC_BACKBUFFERFORMAT_COMBO);
    if ((int)SendMessage(hCur, CB_GETCURSEL, 0, 0) < 0) {
        HWND hCount = GetDlgItem(hDlg, IDC_BACKBUFFERFORMAT_COMBO);
        if ((unsigned)SendMessage(hCount, CB_GETCOUNT, 0, 0) > 0) {
            HWND h = GetDlgItem(hDlg, IDC_BACKBUFFERFORMAT_COMBO);
            SendMessage(h, CB_SETCURSEL, 0, 0);
            PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_BACKBUFFERFORMAT_COMBO, CBN_SELCHANGE), (LPARAM)h);
        }
    }
}
