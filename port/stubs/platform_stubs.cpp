// port/stubs/platform_stubs.cpp: PLACEHOLDERS for the Win32 / D3D9 / D3DX / DirectInput / COM / Winsock / SkinMagic names
// src/ calls, so the portable `mb` links (ticket 004 milestone 2). Nothing here is the port's platform layer:
//   ticket 007 (SDL3 platform layer)  replaces the USER / GDI / KERNEL / registry / clipboard / shell / Winsock section,
//   ticket 008 (renderer)             replaces Direct3DCreate9 (dxvk-native first, then SDL_GPU),
//   ticket 009 (D3DX)                 replaces the 12 D3DX imports with port/ code,
//   the launcher / SkinMagic paths are dropped, not ported.
//
// Rule for a placeholder: report failure the way the real API does (FALSE / NULL / E_FAIL / an error code), never fake
// success. The exceptions are marked "real": pure functions with no platform state (lstr*, MulDiv, SetRect, wsprintfA,
// htons) and the few KERNEL services the reconstruction cannot get past without (time, Sleep, critical sections and
// auto-reset events for the loader thread _beginthread now really starts), done minimally over POSIX.
//
// The signatures are the shims' (port/include): WINAPI is __stdcall on i686, so the callee pops and every parameter
// list must be exact. Compiled with the src/ flags (port_prelude.h, -malign-double), as the shims' structs require.
#include <windows.h>
#include <mmsystem.h>
#include <shlobj.h>
#include <shellapi.h>
#include <oleauto.h>
#include <winsock2.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <dinput.h>
#include <skinmagic.h>

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

namespace {
// auto-/manual-reset event behind a HANDLE (CreateEventA / SetEvent / WaitForSingleObject)
struct PortEvent {
    unsigned magic;
    pthread_mutex_t mu;
    pthread_cond_t cv;
    bool manual, signalled;
};
const unsigned EVENT_MAGIC = 0x45564e54;   // 'EVNT'

void say(const char* what, const char* detail)
{
    fprintf(stderr, "mb (platform placeholder): %s%s%s\n", what, detail ? ": " : "", detail ? detail : "");
}

unsigned long long now_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long long)ts.tv_sec * 1000000000ull + (unsigned long long)ts.tv_nsec;
}
} // namespace

extern "C" {

// =====================================================================================================================
// KERNEL (ticket 007)
// =====================================================================================================================
// real: time
DWORD WINAPI timeGetTime() { return (DWORD)(now_ns() / 1000000ull); }
BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* lpFrequency) { lpFrequency->QuadPart = 1000000000ll; return TRUE; }
BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) { lpPerformanceCount->QuadPart = (LONGLONG)now_ns(); return TRUE; }
void WINAPI Sleep(DWORD dwMilliseconds) { usleep((useconds_t)dwMilliseconds * 1000u); }

// real: critical sections (recursive, as Win32's are); the pthread mutex lives behind DebugInfo
void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION cs)
{
    memset(cs, 0, sizeof(*cs));
    pthread_mutex_t* m = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &a);
    pthread_mutexattr_destroy(&a);
    cs->DebugInfo = m;
}
void WINAPI EnterCriticalSection(LPCRITICAL_SECTION cs) { pthread_mutex_lock((pthread_mutex_t*)cs->DebugInfo); }
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION cs) { pthread_mutex_unlock((pthread_mutex_t*)cs->DebugInfo); }

// real: events (the loader thread's g_mt_event). WaitForSingleObject on anything else (a process handle) fails.
HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName)
{
    PortEvent* e = (PortEvent*)calloc(1, sizeof(PortEvent));
    e->magic = EVENT_MAGIC;
    pthread_mutex_init(&e->mu, 0);
    pthread_cond_init(&e->cv, 0);
    e->manual = bManualReset != 0;
    e->signalled = bInitialState != 0;
    return (HANDLE)e;
}
BOOL WINAPI SetEvent(HANDLE hEvent)
{
    PortEvent* e = (PortEvent*)hEvent;
    if (!e || e->magic != EVENT_MAGIC)
        return FALSE;
    pthread_mutex_lock(&e->mu);
    e->signalled = true;
    pthread_cond_broadcast(&e->cv);
    pthread_mutex_unlock(&e->mu);
    return TRUE;
}
DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
    PortEvent* e = (PortEvent*)hHandle;
    if (!e || e->magic != EVENT_MAGIC)
        return 0xFFFFFFFF;                                  // WAIT_FAILED
    pthread_mutex_lock(&e->mu);
    int rc = 0;
    if (dwMilliseconds == INFINITE) {
        while (!e->signalled)
            pthread_cond_wait(&e->cv, &e->mu);
    } else {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned long long ns = (unsigned long long)ts.tv_nsec + (unsigned long long)dwMilliseconds * 1000000ull;
        ts.tv_sec += (time_t)(ns / 1000000000ull);
        ts.tv_nsec = (long)(ns % 1000000000ull);
        while (!e->signalled && rc != ETIMEDOUT)
            rc = pthread_cond_timedwait(&e->cv, &e->mu, &ts);
    }
    bool got = e->signalled;
    if (got && !e->manual)
        e->signalled = false;
    pthread_mutex_unlock(&e->mu);
    return got ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}
BOOL WINAPI CloseHandle(HANDLE hObject) { return FALSE; }

HANDLE WINAPI GetCurrentProcess() { return 0; }
BOOL WINAPI GetProcessAffinityMask(HANDLE hProcess, DWORD_PTR* lpProcessAffinityMask, DWORD_PTR* lpSystemAffinityMask) { return FALSE; }
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer) { memset(lpBuffer, 0, sizeof(*lpBuffer)); }
HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName) { return 0; }
HMODULE WINAPI LoadLibraryA(LPCSTR lpLibFileName) { return 0; }
BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags,
                           LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo,
                           LPPROCESS_INFORMATION lpProcessInformation) { return FALSE; }
BOOL WINAPI CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) { return FALSE; }
HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) { return (HANDLE)(LONG_PTR)-1; }   // INVALID_HANDLE_VALUE
BOOL WINAPI FindClose(HANDLE hFindFile) { return FALSE; }
HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes) { return 0; }
LPVOID WINAPI GlobalLock(HGLOBAL hMem) { return 0; }
BOOL WINAPI GlobalUnlock(HGLOBAL hMem) { return FALSE; }
int WINAPI MulDiv(int nNumber, int nNumerator, int nDenominator)                      // real
{
    if (nDenominator == 0)
        return -1;
    long long v = (long long)nNumber * nNumerator;
    long long h = (nDenominator < 0 ? -nDenominator : nDenominator) / 2;
    v += ((v < 0) != (nDenominator < 0)) ? -h : h;                                     // round half away from zero
    return (int)(v / nDenominator);
}
// real: the lstr* family
LPSTR WINAPI lstrcatA(LPSTR lpString1, LPCSTR lpString2) { return strcat(lpString1, lpString2); }
int WINAPI lstrcmpA(LPCSTR lpString1, LPCSTR lpString2) { return strcmp(lpString1, lpString2); }
LPSTR WINAPI lstrcpyA(LPSTR lpString1, LPCSTR lpString2) { return strcpy(lpString1, lpString2); }
LPSTR WINAPI lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength)
{
    if (iMaxLength <= 0)
        return lpString1;
    strncpy(lpString1, lpString2, (size_t)iMaxLength - 1);
    lpString1[iMaxLength - 1] = 0;
    return lpString1;
}
int WINAPI lstrlenA(LPCSTR lpString) { return lpString ? (int)strlen(lpString) : 0; }
int WINAPIV wsprintfA(LPSTR out, LPCSTR fmt, ...)                                      // real (wsprintf has no floats)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, 1024, fmt, ap);                                             // wsprintf's documented 1024 limit
    va_end(ap);
    return n;
}

// ---- registry: no keys exist ------------------------------------------------------------------------------------
LONG WINAPI RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass, DWORD dwOptions, REGSAM samDesired,
                            LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition)
{ return ERROR_FILE_NOT_FOUND; }
LONG WINAPI RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData,
                             LPDWORD lpcbData) { return ERROR_FILE_NOT_FOUND; }
LONG WINAPI RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE* lpData, DWORD cbData)
{ return ERROR_FILE_NOT_FOUND; }
LONG WINAPI RegCloseKey(HKEY hKey) { return ERROR_SUCCESS; }

// ---- shell / COM ------------------------------------------------------------------------------------------------
HRESULT WINAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath) { return E_FAIL; }
HINSTANCE WINAPI ShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory,
                               INT nShowCmd) { return 0; }
HRESULT WINAPI CoInitialize(LPVOID pvReserved) { return E_FAIL; }
HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    if (ppv)
        *ppv = 0;
    return E_FAIL;
}
void WINAPI VariantInit(VARIANTARG* pvarg) { memset(pvarg, 0, sizeof(*pvarg)); }
HRESULT WINAPI VariantClear(VARIANTARG* pvarg) { memset(pvarg, 0, sizeof(*pvarg)); return S_OK; }

// =====================================================================================================================
// USER / GDI (ticket 007; the launcher dialogs are dropped)
// =====================================================================================================================
int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    say(lpCaption ? lpCaption : "MessageBoxA", lpText);
    return 0;
}
ATOM WINAPI RegisterClassExW(const WNDCLASSEXW* lpWndClass) { return 0; }
HWND WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y,
                            int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{ return 0; }
BOOL WINAPI DestroyWindow(HWND hWnd) { return FALSE; }
LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) { return 0; }
BOOL WINAPI GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) { return 0; }   // 0 = WM_QUIT
BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) { return FALSE; }
BOOL WINAPI PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) { return FALSE; }
BOOL WINAPI TranslateMessage(const MSG* lpMsg) { return FALSE; }
LRESULT WINAPI DispatchMessageA(const MSG* lpMsg) { return 0; }
LRESULT WINAPI DispatchMessageW(const MSG* lpMsg) { return 0; }
int WINAPI TranslateAcceleratorA(HWND hWnd, HACCEL hAccTable, LPMSG lpMsg) { return 0; }
BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) { return FALSE; }
void WINAPI PostQuitMessage(int nExitCode) {}
LRESULT WINAPI SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) { return 0; }
BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow) { return FALSE; }
BOOL WINAPI UpdateWindow(HWND hWnd) { return FALSE; }
BOOL WINAPI RedrawWindow(HWND hWnd, const RECT* lprcUpdate, HRGN hrgnUpdate, UINT flags) { return FALSE; }
BOOL WINAPI EnableWindow(HWND hWnd, BOOL bEnable) { return FALSE; }
HWND WINAPI SetActiveWindow(HWND hWnd) { return 0; }
HWND WINAPI SetFocus(HWND hWnd) { return 0; }
BOOL WINAPI SetForegroundWindow(HWND hWnd) { return FALSE; }
BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) { return FALSE; }
BOOL WINAPI AdjustWindowRect(LPRECT lpRect, DWORD dwStyle, BOOL bMenu) { return FALSE; }
BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect) { memset(lpRect, 0, sizeof(*lpRect)); return FALSE; }
BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect) { memset(lpRect, 0, sizeof(*lpRect)); return FALSE; }
LONG WINAPI GetWindowLongA(HWND hWnd, int nIndex) { return 0; }
LONG WINAPI SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong) { return 0; }
DWORD WINAPI GetClassLongA(HWND hWnd, int nIndex) { return 0; }
int WINAPI GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount) { if (nMaxCount > 0) lpString[0] = 0; return 0; }
BOOL WINAPI SetWindowTextA(HWND hWnd, LPCSTR lpString) { return FALSE; }
BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint) { return FALSE; }
int WINAPI GetSystemMetrics(int nIndex) { return 0; }
HMENU WINAPI LoadMenuA(HINSTANCE hInstance, LPCSTR lpMenuName) { return 0; }
HMENU WINAPI GetMenu(HWND hWnd) { return 0; }
BOOL WINAPI SetMenu(HWND hWnd, HMENU hMenu) { return FALSE; }
BOOL WINAPI DestroyMenu(HMENU hMenu) { return FALSE; }
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE hInstance, LPCSTR lpTableName) { return 0; }
BOOL WINAPI DestroyAcceleratorTable(HACCEL hAccel) { return FALSE; }
HICON WINAPI LoadIconA(HINSTANCE hInstance, LPCSTR lpIconName) { return 0; }
HCURSOR WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName) { return 0; }
HANDLE WINAPI LoadImageA(HINSTANCE hInst, LPCSTR name, UINT type, int cx, int cy, UINT fuLoad) { return 0; }
HCURSOR WINAPI SetCursor(HCURSOR hCursor) { return 0; }
BOOL WINAPI GetCursorPos(LPPOINT lpPoint) { lpPoint->x = lpPoint->y = 0; return FALSE; }
BOOL WINAPI SetCursorPos(int X, int Y) { return FALSE; }
BOOL WINAPI ClipCursor(const RECT* lpRect) { return FALSE; }
BOOL WINAPI SetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)        // real
{
    if (!lprc)
        return FALSE;
    lprc->left = xLeft; lprc->top = yTop; lprc->right = xRight; lprc->bottom = yBottom;
    return TRUE;
}

// ---- clipboard --------------------------------------------------------------------------------------------------
BOOL WINAPI OpenClipboard(HWND hWndNewOwner) { return FALSE; }
BOOL WINAPI CloseClipboard() { return FALSE; }
BOOL WINAPI EmptyClipboard() { return FALSE; }
BOOL WINAPI IsClipboardFormatAvailable(UINT format) { return FALSE; }
HANDLE WINAPI GetClipboardData(UINT uFormat) { return 0; }
HANDLE WINAPI SetClipboardData(UINT uFormat, HANDLE hMem) { return 0; }

// ---- dialogs (the launcher; dropped, not ported) -----------------------------------------------------------------
INT_PTR WINAPI DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc,
                               LPARAM dwInitParam) { return -1; }
HWND WINAPI CreateDialogParamA(HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc,
                               LPARAM dwInitParam) { return 0; }
BOOL WINAPI EndDialog(HWND hDlg, INT_PTR nResult) { return FALSE; }
HWND WINAPI GetDlgItem(HWND hDlg, int nIDDlgItem) { return 0; }
UINT WINAPI GetDlgItemInt(HWND hDlg, int nIDDlgItem, BOOL* lpTranslated, BOOL bSigned)
{
    if (lpTranslated)
        *lpTranslated = FALSE;
    return 0;
}
BOOL WINAPI SetDlgItemInt(HWND hDlg, int nIDDlgItem, UINT uValue, BOOL bSigned) { return FALSE; }
UINT WINAPI GetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPSTR lpString, int cchMax) { if (cchMax > 0) lpString[0] = 0; return 0; }
BOOL WINAPI SetDlgItemTextA(HWND hDlg, int nIDDlgItem, LPCSTR lpString) { return FALSE; }
BOOL WINAPI CheckRadioButton(HWND hDlg, int nIDFirstButton, int nIDLastButton, int nIDCheckButton) { return FALSE; }
UINT WINAPI IsDlgButtonChecked(HWND hDlg, int nIDButton) { return 0; }

// ---- GDI (the font atlas and the splash bitmap) --------------------------------------------------------------------
HDC WINAPI BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint) { memset(lpPaint, 0, sizeof(*lpPaint)); return 0; }
BOOL WINAPI EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint) { return FALSE; }
HDC WINAPI CreateCompatibleDC(HDC hdc) { return 0; }
BOOL WINAPI DeleteDC(HDC hdc) { return FALSE; }
HBITMAP WINAPI CreateDIBSection(HDC hdc, const BITMAPINFO* pbmi, UINT usage, void** ppvBits, HANDLE hSection, DWORD offset)
{
    if (ppvBits)
        *ppvBits = 0;
    return 0;
}
HFONT WINAPI CreateFontA(int cHeight, int cWidth, int cEscapement, int cOrientation, int cWeight, DWORD bItalic,
                         DWORD bUnderline, DWORD bStrikeOut, DWORD iCharSet, DWORD iOutPrecision, DWORD iClipPrecision,
                         DWORD iQuality, DWORD iPitchAndFamily, LPCSTR pszFaceName) { return 0; }
HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h) { return 0; }
BOOL WINAPI DeleteObject(HGDIOBJ ho) { return FALSE; }
HGDIOBJ WINAPI GetStockObject(int i) { return 0; }
int WINAPI GetObjectA(HANDLE h, int c, LPVOID pv) { return 0; }
int WINAPI GetDeviceCaps(HDC hdc, int index) { return 0; }
int WINAPI SetMapMode(HDC hdc, int iMode) { return 0; }
COLORREF WINAPI SetTextColor(HDC hdc, COLORREF color) { return 0xFFFFFFFF; }          // CLR_INVALID
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF color) { return 0xFFFFFFFF; }
UINT WINAPI SetTextAlign(HDC hdc, UINT align) { return 0xFFFFFFFF; }                  // GDI_ERROR
BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE psizl) { psizl->cx = psizl->cy = 0; return FALSE; }
BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, const RECT* lprect, LPCSTR lpString, UINT c, const INT* lpDx)
{ return FALSE; }
BOOL WINAPI BitBlt(HDC hdc, int x, int y, int cx, int cy, HDC hdcSrc, int x1, int y1, DWORD rop) { return FALSE; }

// ---- SkinMagic (the launcher's skin library; dropped) ---------------------------------------------------------------
void __stdcall InitSkinMagicLib(HINSTANCE inst, const char* app, int a, int b) {}
void __stdcall ExitSkinMagicLib() {}
void __stdcall LoadSkinFile(const char* path) {}
int  __stdcall SetSingleDialogSkin(HWND dialog, const char* skin_class) { return 0; }

// ---- Winsock (the version check). These names shadow glibc's socket API inside this executable: the game calls them
// __stdcall with Winsock types, glibc's are cdecl. Ticket 007 renames the seam (mbp::) before anything else links them.
int WINAPI WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData) { return 10091; }  // WSASYSNOTREADY
SOCKET WINAPI socket(int af, int type, int protocol) { return INVALID_SOCKET; }
int WINAPI connect(SOCKET s, const struct sockaddr* name, int namelen) { return SOCKET_ERROR; }
int WINAPI send(SOCKET s, const char* buf, int len, int flags) { return SOCKET_ERROR; }
int WINAPI recv(SOCKET s, char* buf, int len, int flags) { return SOCKET_ERROR; }
int WINAPI closesocket(SOCKET s) { return SOCKET_ERROR; }
struct hostent* WINAPI gethostbyname(const char* name) { return 0; }
unsigned short WINAPI htons(unsigned short hostshort) { return (unsigned short)((hostshort >> 8) | (hostshort << 8)); }   // real

// =====================================================================================================================
// Direct3D 9 (ticket 008): no device. Direct3DCreate9 returning NULL is the documented "D3D not available" result.
// =====================================================================================================================
IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion) { return 0; }

// =====================================================================================================================
// D3DX 9 (ticket 009): creators fail; the math helpers return pOut unchanged (placeholders, NOT the math).
// =====================================================================================================================
HRESULT WINAPI D3DXAssembleShaderFromFileA(LPCSTR pSrcFile, CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags,
                                           LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs) { return E_FAIL; }
HRESULT WINAPI D3DXCreateEffectFromFileA(LPDIRECT3DDEVICE9 pDevice, LPCSTR pSrcFile, CONST D3DXMACRO* pDefines,
                                         LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXEFFECTPOOL pPool,
                                         LPD3DXEFFECT* ppEffect, LPD3DXBUFFER* ppCompilationErrors) { return E_FAIL; }
HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT MipLevels, DWORD Usage,
                                 D3DFORMAT Format, D3DPOOL Pool, LPDIRECT3DTEXTURE9* ppTexture) { return E_FAIL; }
HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE9 pDevice, LPCSTR pSrcFile, UINT Width, UINT Height,
                                            UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter,
                                            DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo,
                                            PALETTEENTRY* pPalette, LPDIRECT3DTEXTURE9* ppTexture) { return E_FAIL; }
HRESULT WINAPI D3DXSaveSurfaceToFileA(LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPDIRECT3DSURFACE9 pSrcSurface,
                                      CONST PALETTEENTRY* pSrcPalette, CONST RECT* pSrcRect) { return E_FAIL; }
D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX* pOut, FLOAT* pDeterminant, const D3DXMATRIX* pM) { return pOut; }
D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2) { return pOut; }
D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM) { return pOut; }
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf) { return pOut; }
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH(D3DXMATRIX* pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf) { return pOut; }
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX* pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf)
{ return pOut; }
D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM) { return pOut; }

// =====================================================================================================================
// DirectInput 8 (ticket 007: SDL3 input through the DIK table)
// =====================================================================================================================
HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
    if (ppvOut)
        *ppvOut = 0;
    return E_FAIL;
}
// The real GUIDs (dinput.h); the data formats are empty: no device ever reads them while DirectInput8Create fails.
const GUID GUID_SysKeyboard = {0x6F1D2B61, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID GUID_SysMouse = {0x6F1D2B60, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};
const GUID IID_IDirectInput8A = {0xBF798030, 0x483A, 0x4DA2, {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00}};
const DIDATAFORMAT c_dfDIKeyboard = {};
const DIDATAFORMAT c_dfDIMouse2 = {};

} // extern "C"
