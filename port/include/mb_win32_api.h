// port/include/mb_win32_api.h: the Win32 functions, structs and constants src/ calls (USER, GDI, KERNEL, registry, shell,
// clipboard, COM), declared for the portable build (ticket 004; included by port/include/windows.h). ANSI entry points
// only: the game is built _MBCS. Declarations only: ticket 007's SDL3 platform layer implements the ones the portable
// build keeps (cursor, clipboard, timers, events, directories) and the launcher / dialog paths are replaced, not ported.
#pragma once
#ifndef MB_PORT_WIN32_API_H
#define MB_PORT_WIN32_API_H

// ---- pointer / integer helpers ----
#define PtrToUlong(p) ((ULONG)(ULONG_PTR)(p))
#define PtrToLong(p) ((LONG)(LONG_PTR)(p))
#define UlongToPtr(ul) ((void*)(ULONG_PTR)(ul))
#define ULongToPtr(ul) ((void*)(ULONG_PTR)(ul))
#define LongToPtr(l) ((void*)(LONG_PTR)(l))
#define IntToPtr(i) ((void*)(INT_PTR)(i))
#define UNREFERENCED_PARAMETER(p) (void)(p)
#define INFINITE 0xFFFFFFFF
#define WAIT_OBJECT_0 0
#define WAIT_TIMEOUT 258L
#define ERROR_SUCCESS 0L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_ALREADY_EXISTS 183L
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FACILITY_WIN32 7
#define __HRESULT_FROM_WIN32(x) ((HRESULT)(x) <= 0 ? (HRESULT)(x) : (HRESULT)(((x) & 0x0000FFFF) | (FACILITY_WIN32 << 16) | 0x80000000))
#define HRESULT_FROM_WIN32(x) __HRESULT_FROM_WIN32(x)
#define MAKEWPARAM(l, h) ((WPARAM)(DWORD)MAKELONG(l, h))
#define MAKELRESULT(l, h) ((LRESULT)(DWORD)MAKELONG(l, h))

// ---- callbacks ----
typedef LRESULT (CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef INT_PTR (CALLBACK* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef DWORD (WINAPI* LPTHREAD_START_ROUTINE)(LPVOID);

// ---- window classes / messages ----
typedef struct tagWNDCLASSA {
    UINT style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor;
    HBRUSH hbrBackground; LPCSTR lpszMenuName, lpszClassName;
} WNDCLASSA, WNDCLASS;
typedef struct tagWNDCLASSEXW {
    UINT cbSize, style; WNDPROC lpfnWndProc; int cbClsExtra, cbWndExtra; HINSTANCE hInstance; HICON hIcon; HCURSOR hCursor;
    HBRUSH hbrBackground; LPCWSTR lpszMenuName, lpszClassName; HICON hIconSm;
} WNDCLASSEXW;
typedef struct tagPAINTSTRUCT { HDC hdc; BOOL fErase; RECT rcPaint; BOOL fRestore, fIncUpdate; BYTE rgbReserved[32]; } PAINTSTRUCT, *LPPAINTSTRUCT;
typedef struct tagMINMAXINFO { POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize; } MINMAXINFO, *LPMINMAXINFO;
typedef struct tagNMHDR { HWND hwndFrom; UINT_PTR idFrom; UINT code; } NMHDR, *LPNMHDR;
typedef struct tagWINDOWPLACEMENT { UINT length, flags, showCmd; POINT ptMinPosition, ptMaxPosition; RECT rcNormalPosition; } WINDOWPLACEMENT;
typedef struct tagMONITORINFO { DWORD cbSize; RECT rcMonitor, rcWork; DWORD dwFlags; } MONITORINFO, *LPMONITORINFO;

#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_DBLCLKS 0x0008
#define CS_OWNDC 0x0020
#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_CHILD 0x40000000L
#define WS_MINIMIZE 0x20000000L
#define WS_VISIBLE 0x10000000L
#define WS_DISABLED 0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MAXIMIZE 0x01000000L
#define WS_CAPTION 0x00C00000L
#define WS_BORDER 0x00800000L
#define WS_DLGFRAME 0x00400000L
#define WS_VSCROLL 0x00200000L
#define WS_HSCROLL 0x00100000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_MAXIMIZEBOX 0x00010000L
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_EX_TOPMOST 0x00000008L
#define CW_USEDEFAULT ((int)0x80000000)
#define GWL_STYLE (-16)
#define GWL_EXSTYLE (-20)
#define GWL_USERDATA (-21)
#define GCL_HCURSOR (-12)
#define GCLP_HCURSOR (-12)
#define HWND_TOP ((HWND)0)
#define HWND_BOTTOM ((HWND)1)
#define HWND_TOPMOST ((HWND)-1)
#define HWND_NOTOPMOST ((HWND)-2)
#define SWP_NOSIZE 0x0001
#define SWP_NOMOVE 0x0002
#define SWP_NOZORDER 0x0004
#define SWP_NOREDRAW 0x0008
#define SWP_NOACTIVATE 0x0010
#define SWP_FRAMECHANGED 0x0020
#define SWP_SHOWWINDOW 0x0040
#define SWP_HIDEWINDOW 0x0080
#define SWP_NOOWNERZORDER 0x0200
#define SW_HIDE 0
#define SW_SHOWNORMAL 1
#define SW_NORMAL 1
#define SW_SHOWMINIMIZED 2
#define SW_SHOWMAXIMIZED 3
#define SW_MAXIMIZE 3
#define SW_SHOW 5
#define SW_MINIMIZE 6
#define SW_RESTORE 9
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CYCAPTION 4
#define SM_CXFRAME 32
#define SM_CYFRAME 33
#define SM_CXSIZEFRAME SM_CXFRAME
#define SM_CYSIZEFRAME SM_CYFRAME
#define SM_CYMENU 15
#define SM_CXMAXIMIZED 61
#define SM_CYMAXIMIZED 62
#define WM_NULL 0x0000
#define WM_CREATE 0x0001
#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_SETTEXT 0x000C
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_ERASEBKGND 0x0014
#define WM_ACTIVATEAPP 0x001C
#define WM_SETCURSOR 0x0020
#define WM_GETMINMAXINFO 0x0024
#define WM_NOTIFY 0x004E
#define WM_NCHITTEST 0x0084
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_SYSCHAR 0x0106
#define WM_INITDIALOG 0x0110
#define WM_COMMAND 0x0111
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_HSCROLL 0x0114
#define WM_VSCROLL 0x0115
#define WM_ENTERMENULOOP 0x0211
#define WM_EXITMENULOOP 0x0212
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP 0x0202
#define WM_RBUTTONDOWN 0x0204
#define WM_RBUTTONUP 0x0205
#define WM_MOUSEWHEEL 0x020A
#define WM_SIZING 0x0214
#define WM_ENTERSIZEMOVE 0x0231
#define WM_EXITSIZEMOVE 0x0232
#define WM_POWERBROADCAST 0x0218
#define WM_USER 0x0400
#define SC_KEYMENU 0xF100
#define SC_MONITORPOWER 0xF170
#define SC_SCREENSAVE 0xF140
#define SC_MOVE 0xF010
#define SC_SIZE 0xF000
#define SC_MAXIMIZE 0xF030
#define HTCLIENT 1
#define PBT_APMQUERYSUSPEND 0x0000
#define BROADCAST_QUERY_DENY 0x424D5144
#define SIZE_RESTORED 0
#define SIZE_MINIMIZED 1
#define SIZE_MAXIMIZED 2
#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
#define MB_OK 0x00000000L
#define MB_OKCANCEL 0x00000001L
#define MB_YESNO 0x00000004L
#define MB_ICONHAND 0x00000010L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_ICONASTERISK 0x00000040L
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONWARNING MB_ICONEXCLAMATION
#define MB_ICONINFORMATION MB_ICONASTERISK
#define MB_ICONSTOP MB_ICONHAND
#define MB_TOPMOST 0x00040000L
#define IDOK 1
#define IDCANCEL 2
#define IDYES 6
#define IDNO 7
#define IDC_ARROW MAKEINTRESOURCE(32512)
#define IDI_APPLICATION MAKEINTRESOURCE(32512)
#define IMAGE_BITMAP 0
#define IMAGE_ICON 1
#define LR_DEFAULTCOLOR 0x00000000
#define LR_LOADFROMFILE 0x00000010
#define MF_BYCOMMAND 0x00000000L
#define MF_CHECKED 0x00000008L
#define MF_UNCHECKED 0x00000000L
#define MF_ENABLED 0x00000000L
#define MF_GRAYED 0x00000001L
#define RDW_INVALIDATE 0x0001
#define RDW_ERASE 0x0004
#define RDW_UPDATENOW 0x0100
#define RDW_ALLCHILDREN 0x0080
#define RDW_FRAME 0x0400

// controls
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define BM_GETSTATE 0x00F2
#define BM_SETSTATE 0x00F3
#define BST_UNCHECKED 0x0000
#define BST_CHECKED 0x0001
#define BN_CLICKED 0
#define CB_ERR (-1)
#define CB_ADDSTRING 0x0143
#define CB_DELETESTRING 0x0144
#define CB_INSERTSTRING 0x014A
#define CB_GETCOUNT 0x0146
#define CB_GETCURSEL 0x0147
#define CB_GETLBTEXT 0x0148
#define CB_GETLBTEXTLEN 0x0149
#define CB_RESETCONTENT 0x014B
#define CB_SETCURSEL 0x014E
#define CB_GETITEMDATA 0x0150
#define CB_SETITEMDATA 0x0151
#define CBN_SELCHANGE 1
#define LB_ERR (-1)
#define LB_ADDSTRING 0x0180
#define LB_RESETCONTENT 0x0184
#define LB_SETSEL 0x0185
#define LB_GETSEL 0x0187
#define LB_GETCURSEL 0x0188
#define LB_GETCOUNT 0x018B
#define LB_GETSELITEMS 0x0191
#define LB_GETITEMDATA 0x0199
#define LB_SETITEMDATA 0x019A
#define LBN_SELCHANGE 1
#define EM_SETLIMITTEXT 0x00C5

// ---- GDI ----
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage;
    LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;
typedef struct tagRGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct tagBITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; } BITMAPINFO, *LPBITMAPINFO;
typedef struct tagBITMAP { LONG bmType, bmWidth, bmHeight, bmWidthBytes; WORD bmPlanes, bmBitsPixel; LPVOID bmBits; } BITMAP, *LPBITMAP;
#define BI_RGB 0L
#define DIB_RGB_COLORS 0
#define LOGPIXELSY 90
#define FW_NORMAL 400
#define FW_BOLD 700
#define DEFAULT_CHARSET 1
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define ANTIALIASED_QUALITY 4
#define NONANTIALIASED_QUALITY 3
#define DEFAULT_QUALITY 0
#define VARIABLE_PITCH 2
#define FF_DONTCARE (0 << 4)
#define ETO_OPAQUE 0x0002
#define MM_TEXT 1
#define TA_TOP 0
#define TA_LEFT 0
#define BLACK_BRUSH 4
#define WHITE_BRUSH 0
#define SRCCOPY (DWORD)0x00CC0020

// ---- kernel ----
typedef struct _STARTUPINFOA {
    DWORD cb; LPSTR lpReserved, lpDesktop, lpTitle; DWORD dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars;
    DWORD dwFillAttribute, dwFlags; WORD wShowWindow, cbReserved2; LPBYTE lpReserved2; HANDLE hStdInput, hStdOutput, hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;
typedef struct _PROCESS_INFORMATION { HANDLE hProcess, hThread; DWORD dwProcessId, dwThreadId; } PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
typedef struct _MEMORYSTATUS {
    DWORD dwLength, dwMemoryLoad; SIZE_T dwTotalPhys, dwAvailPhys, dwTotalPageFile, dwAvailPageFile, dwTotalVirtual, dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;
typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize, dwMajorVersion, dwMinorVersion, dwBuildNumber, dwPlatformId; CHAR szCSDVersion[128];
} OSVERSIONINFOA, *LPOSVERSIONINFOA;
#define GMEM_MOVEABLE 0x0002
#define GMEM_ZEROINIT 0x0040
#define GMEM_DDESHARE 0x2000
#define CF_TEXT 1
#define STARTF_USESHOWWINDOW 0x00000001
#define NORMAL_PRIORITY_CLASS 0x00000020
#define HKEY_CLASSES_ROOT ((HKEY)(ULONG_PTR)((LONG)0x80000000))
#define HKEY_CURRENT_USER ((HKEY)(ULONG_PTR)((LONG)0x80000001))
#define HKEY_LOCAL_MACHINE ((HKEY)(ULONG_PTR)((LONG)0x80000002))
#define KEY_QUERY_VALUE 0x0001
#define KEY_SET_VALUE 0x0002
#define KEY_READ 0x20019
#define KEY_WRITE 0x20006
#define KEY_ALL_ACCESS 0xF003F
#define REG_OPTION_NON_VOLATILE 0x00000000L
#define REG_SZ 1
#define REG_BINARY 3
#define REG_DWORD 4
typedef DWORD REGSAM;

extern "C" {
// USER
HWND WINAPI GetDlgItem(HWND hDlg, int nIDDlgItem);
BOOL WINAPI SetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString);
UINT WINAPI GetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax);
BOOL WINAPI SetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned);
UINT WINAPI GetDlgItemInt(HWND hDlg, int nIDDlgItem, BOOL* lpTranslated, BOOL bSigned);
BOOL WINAPI CheckRadioButton(HWND hDlg, int nIDFirstButton, int nIDLastButton, int nIDCheckButton);
UINT WINAPI IsDlgButtonChecked(HWND hDlg, int nIDButton);
BOOL WINAPI CheckDlgButton(HWND hDlg, int nIDButton, UINT uCheck);
LRESULT WINAPI SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI SendDlgItemMessageA(HWND hDlg, int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void WINAPI PostQuitMessage(int nExitCode);
BOOL WINAPI EndDialog(HWND hDlg, INT_PTR nResult);
INT_PTR WINAPI DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND WINAPI CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
int WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);
BOOL WINAPI UpdateWindow(HWND hWnd);
BOOL WINAPI EnableWindow(HWND hWnd, BOOL bEnable);
BOOL WINAPI DestroyWindow(HWND hWnd);
BOOL WINAPI IsWindow(HWND hWnd);
BOOL WINAPI IsIconic(HWND hWnd);
BOOL WINAPI IsZoomed(HWND hWnd);
HWND WINAPI SetFocus(HWND hWnd);
HWND WINAPI GetFocus();
HWND WINAPI SetActiveWindow(HWND hWnd);
BOOL WINAPI SetForegroundWindow(HWND hWnd);
HWND WINAPI GetForegroundWindow();
BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
BOOL WINAPI MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu);
BOOL WINAPI AdjustWindowRectEx(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle);
BOOL WINAPI GetWindowPlacement(HWND hWnd, WINDOWPLACEMENT* lpwndpl);
BOOL WINAPI SetWindowPlacement(HWND hWnd, const WINDOWPLACEMENT* lpwndpl);
BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint);
BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint);
LONG WINAPI GetWindowLongA(HWND hWnd, int nIndex);
LONG WINAPI SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
LONG WINAPI GetWindowLongW(HWND hWnd, int nIndex);
LONG WINAPI SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong);
DWORD WINAPI GetClassLongA(HWND hWnd, int nIndex);
DWORD WINAPI SetClassLongA(HWND hWnd, int nIndex, LONG dwNewLong);
int WINAPI GetSystemMetrics(int nIndex);
BOOL WINAPI SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
BOOL WINAPI SetRectEmpty(LPRECT lprc);
BOOL WINAPI PtInRect(const RECT* lprc, POINT pt);
BOOL WINAPI IntersectRect(LPRECT lprcDst, const RECT* lprcSrc1, const RECT* lprcSrc2);
BOOL WINAPI RedrawWindow(HWND hWnd, const RECT* lprcUpdate, HRGN hrgnUpdate, UINT flags);
BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase);
HDC WINAPI BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
BOOL WINAPI EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);
HDC WINAPI GetDC(HWND hWnd);
int WINAPI ReleaseDC(HWND hWnd, HDC hDC);
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName);
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
HCURSOR WINAPI LoadCursorW(HINSTANCE hInstance, LPCWSTR lpCursorName);
HCURSOR WINAPI SetCursor(HCURSOR hCursor);
BOOL WINAPI SetCursorPos(int X, int Y);
BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
int WINAPI ShowCursor(BOOL bShow);
BOOL WINAPI ClipCursor(const RECT* lpRect);
HANDLE WINAPI LoadImageA(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad);
HMENU WINAPI LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName);
HMENU WINAPI GetMenu(HWND hWnd);
BOOL WINAPI SetMenu(HWND hWnd, HMENU hMenu);
DWORD WINAPI CheckMenuItem(HMENU hMenu, UINT uIDCheckItem, UINT uCheck);
BOOL WINAPI EnableMenuItem(HMENU hMenu, UINT uIDEnableItem, UINT uEnable);
BOOL WINAPI DestroyMenu(HMENU hMenu);
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR lpTableName);
BOOL WINAPI DestroyAcceleratorTable(HACCEL hAccel);
int WINAPI TranslateAcceleratorA(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);
int WINAPI TranslateAcceleratorW(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg);
BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI TranslateMessage(const MSG* lpMsg);
LRESULT WINAPI DispatchMessageA(const MSG* lpMsg);
LRESULT WINAPI DispatchMessageW(const MSG* lpMsg);
LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI CallWindowProcA(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
ATOM WINAPI RegisterClassA(const WNDCLASSA* lpWndClass);
ATOM WINAPI RegisterClassExW(const WNDCLASSEXW* lpWndClass);
HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
HWND WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
BOOL WINAPI SetWindowTextA(HWND hWnd, LPCSTR lpString);
BOOL WINAPI SetWindowTextW(HWND hWnd, LPCWSTR lpString);
int WINAPI GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount);
HWND WINAPI GetParent(HWND hWnd);
HMONITOR WINAPI MonitorFromWindow(HWND hwnd, DWORD dwFlags);
BOOL WINAPI GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpmi);
BOOL WINAPI OpenClipboard(HWND hWndNewOwner);
BOOL WINAPI CloseClipboard();
BOOL WINAPI EmptyClipboard();
HANDLE WINAPI GetClipboardData(UINT uFormat);
HANDLE WINAPI SetClipboardData(UINT uFormat, HANDLE hMem);
BOOL WINAPI IsClipboardFormatAvailable(UINT format);
SHORT WINAPI GetAsyncKeyState(int vKey);
SHORT WINAPI GetKeyState(int nVirtKey);
UINT WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, void* lpTimerFunc);
BOOL WINAPI KillTimer(HWND hWnd, UINT_PTR uIDEvent);
BOOL WINAPI SystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
int WINAPIV wsprintfA(LPSTR, LPCSTR, ...);
// GDI
HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h);
BOOL WINAPI DeleteObject(HGDIOBJ ho);
HGDIOBJ WINAPI GetStockObject(int i);
int WINAPI GetObjectA(HANDLE h, int c, LPVOID pv);
HDC WINAPI CreateCompatibleDC(HDC hdc);
BOOL WINAPI DeleteDC(HDC hdc);
HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int cx, int cy);
HBITMAP WINAPI CreateDIBSection(HDC hdc, const BITMAPINFO* pbmi, UINT usage, void** ppvBits, HANDLE hSection, DWORD offset);
HFONT WINAPI CreateFontA(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic, DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision, DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName);
int WINAPI GetDeviceCaps(HDC hdc, int index);
int WINAPI SetMapMode(HDC hdc, int iMode);
COLORREF WINAPI SetTextColor(HDC hdc, COLORREF color);
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF color);
UINT WINAPI SetTextAlign(HDC hdc, UINT align);
BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* lprect, LPCSTR lpString, UINT c, const INT* lpDx);
BOOL WINAPI TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c);
BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE psizl);
BOOL WINAPI BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop);
BOOL WINAPI StretchBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc, DWORD rop);
int WINAPI MulDiv(int nNumber, int nNumerator, int nDenominator);
// KERNEL
HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName);
DWORD WINAPI GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);
HMODULE WINAPI LoadLibraryA(LPCSTR lpLibFileName);
BOOL WINAPI FreeLibrary(HMODULE hLibModule);
void* WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName);
HANDLE WINAPI GetCurrentProcess();
HANDLE WINAPI GetCurrentThread();
DWORD WINAPI GetCurrentThreadId();
BOOL WINAPI GetProcessAffinityMask(HANDLE hProcess, DWORD_PTR* lpProcessAffinityMask, DWORD_PTR* lpSystemAffinityMask);
DWORD_PTR WINAPI SetThreadAffinityMask(HANDLE hThread, DWORD_PTR dwThreadAffinityMask);
BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority);
void WINAPI Sleep(DWORD dwMilliseconds);
DWORD WINAPI GetTickCount();
BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency);
BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
void WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime);
void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime);
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer);
BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA lpVersionInformation);
DWORD WINAPI GetLastError();
void WINAPI SetLastError(DWORD dwErrCode);
void WINAPI OutputDebugStringA(LPCSTR lpOutputString);
void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
BOOL WINAPI SetEvent(HANDLE hEvent);
BOOL WINAPI ResetEvent(HANDLE hEvent);
DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WINAPI WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
BOOL WINAPI CloseHandle(HANDLE hObject);
BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);
BOOL WINAPI TerminateProcess(HANDLE hProcess, UINT uExitCode);
void WINAPI ExitProcess(UINT uExitCode);
BOOL WINAPI CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL WINAPI RemoveDirectoryA(LPCSTR lpPathName);
BOOL WINAPI DeleteFileA(LPCSTR lpFileName);
BOOL WINAPI CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
BOOL WINAPI MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
BOOL WINAPI MoveFileExA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags);
#define MOVEFILE_REPLACE_EXISTING 0x00000001
DWORD WINAPI GetFileAttributesA(LPCSTR lpFileName);
HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
BOOL WINAPI FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
BOOL WINAPI FindClose(HANDLE hFindFile);
DWORD WINAPI GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);
BOOL WINAPI SetCurrentDirectoryA(LPCSTR lpPathName);
DWORD WINAPI GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer);
// port/platform/fs.cpp's mbp_getenv: SDL_setenv_unsafe writes only the Win32 block on Windows, so the port reads it too
DWORD WINAPI GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
BOOL WINAPI SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue);HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes);
LPVOID WINAPI GlobalLock(HGLOBAL hMem);
BOOL WINAPI GlobalUnlock(HGLOBAL hMem);
HGLOBAL WINAPI GlobalFree(HGLOBAL hMem);
LPVOID WINAPI VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
BOOL WINAPI VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType);
int WINAPI MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
int WINAPI WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);
LPSTR WINAPI lstrcpyA(LPSTR lpString1, LPCSTR lpString2);
LPSTR WINAPI lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength);
LPSTR WINAPI lstrcatA(LPSTR lpString1, LPCSTR lpString2);
int WINAPI lstrlenA(LPCSTR lpString);
int WINAPI lstrcmpA(LPCSTR lpString1, LPCSTR lpString2);
int WINAPI lstrcmpiA(LPCSTR lpString1, LPCSTR lpString2);
// registry
LONG WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
LONG WINAPI RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
LONG WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
LONG WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData);
LONG WINAPI RegCloseKey(HKEY hKey);
}

// ANSI aliases (the game is _MBCS)
#define SendMessage SendMessageA
#define PostMessage PostMessageA
#define SetDlgItemText SetDlgItemTextA
#define GetDlgItemText GetDlgItemTextA
#define MessageBox MessageBoxA
#define DialogBoxParam DialogBoxParamA
#define DialogBox(hInstance, lpTemplate, hWndParent, lpDialogFunc) DialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, 0L)
#define LoadIcon LoadIconA
#define LoadCursor LoadCursorA
#define LoadImage LoadImageA
#define LoadMenu LoadMenuA
#define LoadAccelerators LoadAcceleratorsA
#define TranslateAccelerator TranslateAcceleratorA
#define PeekMessage PeekMessageA
#define GetMessage GetMessageA
#define DispatchMessage DispatchMessageA
#define DefWindowProc DefWindowProcA
#define RegisterClass RegisterClassA
#define CreateWindowEx CreateWindowExA
#define SetWindowText SetWindowTextA
#define GetWindowText GetWindowTextA
#define GetWindowLong GetWindowLongA
#define SetWindowLong SetWindowLongA
#define GetClassLong GetClassLongA
#define SetClassLong SetClassLongA
#define GetObject GetObjectA
#define CreateFont CreateFontA
#define ExtTextOut ExtTextOutA
#define TextOut TextOutA
#define GetTextExtentPoint32 GetTextExtentPoint32A
#define GetModuleHandle GetModuleHandleA
#define GetModuleFileName GetModuleFileNameA
#define LoadLibrary LoadLibraryA
#define CreateEvent CreateEventA
#define CreateProcess CreateProcessA
#define CreateDirectory CreateDirectoryA
#define FindFirstFile FindFirstFileA
#define FindNextFile FindNextFileA
#define OutputDebugString OutputDebugStringA
#define GetVersionEx GetVersionExA
#define lstrcpy lstrcpyA
#define lstrcpyn lstrcpynA
#define lstrcat lstrcatA
#define lstrlen lstrlenA
#define lstrcmp lstrcmpA
#define lstrcmpi lstrcmpiA
#define wsprintf wsprintfA
#define RegCreateKeyEx RegCreateKeyExA
#define RegOpenKeyEx RegOpenKeyExA
#define RegQueryValueEx RegQueryValueExA
#define RegSetValueEx RegSetValueExA
#define STARTUPINFO STARTUPINFOA

// MSVC's windows.h reaches the common-control declarations in some configurations (src/game/config/launcher_config_dialog.cpp
// uses TCITEMA with no <commctrl.h>): keep them visible.
#include <commctrl.h>

#endif
