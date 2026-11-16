// port/include/commctrl.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
#define TBM_GETPOS (WM_USER)
#define TBM_GETRANGEMIN (WM_USER + 1)
#define TBM_GETRANGEMAX (WM_USER + 2)
#define TBM_SETPOS (WM_USER + 5)
#define TBM_SETRANGE (WM_USER + 6)
#define TBM_SETRANGEMIN (WM_USER + 7)
#define TBM_SETRANGEMAX (WM_USER + 8)
#define TBM_SETTICFREQ (WM_USER + 20)
#define TCM_FIRST 0x1300
#define TCM_GETCURSEL (TCM_FIRST + 11)
#define TCM_SETCURSEL (TCM_FIRST + 12)
#define TCM_INSERTITEMA (TCM_FIRST + 7)
#define TCIF_TEXT 0x0001
#define TCIF_IMAGE 0x0002
#define TCIF_PARAM 0x0008
#define TCN_FIRST (0U - 550U)
#define TCN_SELCHANGE (TCN_FIRST - 1)
#define TCN_SELCHANGING (TCN_FIRST - 2)
typedef struct tagTCITEMA { UINT mask; DWORD dwState, dwStateMask; LPSTR pszText; int cchTextMax; int iImage; LPARAM lParam; } TCITEMA, *LPTCITEMA;
extern "C" void WINAPI InitCommonControls(void);
