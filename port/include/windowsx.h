// port/include/windowsx.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
#define ComboBox_GetCount(hwndCtl) ((int)(DWORD)SendMessageA((hwndCtl), CB_GETCOUNT, 0L, 0L))
#define ComboBox_GetCurSel(hwndCtl) ((int)(DWORD)SendMessageA((hwndCtl), CB_GETCURSEL, 0L, 0L))
#define ComboBox_SetCurSel(hwndCtl, index) ((int)(DWORD)SendMessageA((hwndCtl), CB_SETCURSEL, (WPARAM)(int)(index), 0L))
#define ComboBox_AddString(hwndCtl, lpsz) ((int)(DWORD)SendMessageA((hwndCtl), CB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))
#define ComboBox_GetItemData(hwndCtl, index) ((LRESULT)(ULONG_PTR)SendMessageA((hwndCtl), CB_GETITEMDATA, (WPARAM)(int)(index), 0L))
#define ComboBox_SetItemData(hwndCtl, index, data) ((int)(DWORD)SendMessageA((hwndCtl), CB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)))
#define ComboBox_ResetContent(hwndCtl) ((int)(DWORD)SendMessageA((hwndCtl), CB_RESETCONTENT, 0L, 0L))
#define ComboBox_GetLBTextLen(hwndCtl, index) ((int)(DWORD)SendMessageA((hwndCtl), CB_GETLBTEXTLEN, (WPARAM)(int)(index), 0L))
#define ComboBox_GetLBText(hwndCtl, index, lpszBuffer) ((int)(DWORD)SendMessageA((hwndCtl), CB_GETLBTEXT, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpszBuffer)))
#define ComboBox_Enable(hwndCtl, fEnable) EnableWindow((hwndCtl), (fEnable))
#define Button_GetCheck(hwndCtl) ((int)(DWORD)SendMessageA((hwndCtl), BM_GETCHECK, 0L, 0L))
#define Button_SetCheck(hwndCtl, check) ((void)SendMessageA((hwndCtl), BM_SETCHECK, (WPARAM)(int)(check), 0L))
