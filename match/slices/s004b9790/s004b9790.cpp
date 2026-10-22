// Slice s004b9790: options-dialog page switching, the version / GPU-memory probe and the options-window
// paint handler. The dialog object is the one used by slices s004cf7b0 / s004d0240 (hwnd at +0x380,
// a paint target hwnd at +0x364 and a bitmap at +0x354). Names are provisional.
// <windows.h>/<d3d9.h> are not used here: the SDK needs the VC9 <sal.h>, and <dxdiag.h> (the DXDiag COM
// headers) is not on the toolchain at all, so the few Win32 types are declared by hand.
typedef void* HWND; typedef void* HBITMAP; typedef void* HDC; typedef void* HGDIOBJ; typedef void* HINSTANCE;
typedef int BOOL; typedef long HRESULT; typedef unsigned long DWORD; typedef unsigned int UINT;
typedef unsigned short WORD; typedef unsigned short WCHAR; typedef WCHAR* LPOLESTR; typedef int DISPID;
typedef long LCID; typedef struct _GUID { unsigned long a; unsigned short b, c; unsigned char d[8]; } GUID;
typedef const GUID& REFIID;
#define IID_NULL (*(const GUID*)0)
#define DECLSPEC_IMPORT __declspec(dllimport)
#define WINAPI __stdcall
#define WINAPIV __cdecl
// OLE automation core (oaidl / oleauto).
struct IUnknown {
    virtual HRESULT __stdcall QueryInterface(const GUID&, void**) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
};
struct VARIANT { unsigned short vt; unsigned short r1, r2, r3; long l; WCHAR* pwszVal; };
struct PROPERTYKEY { GUID guid; DWORD pid; };
struct IPropertyBag : public IUnknown {
    virtual HRESULT __stdcall Read(const WCHAR* name, PROPERTYKEY* key, VARIANT* v) = 0;
};
struct MEMORYSTATUS { DWORD dwLength, dwMemoryLoad; unsigned long dwTotalPhys, dwAvailPhys,
                      dwTotalPageFile, dwAvailPageFile, dwTotalVirtual, dwAvailVirtual; };
struct BITMAP { long bmType; long bmWidth; long bmHeight; long bmWidthBytes; WORD bmPlanes; WORD bmBitsPixel;
                void* bmBits; };
struct PAINTSTRUCT { HDC hdc; BOOL fErase; long rcPaint[4]; BOOL fRestore; BOOL fIncUpdate; unsigned char rgb[32]; };
extern "C" {
DECLSPEC_IMPORT BOOL WINAPI ShowWindow(HWND, int);
DECLSPEC_IMPORT HDC WINAPI BeginPaint(HWND, PAINTSTRUCT*);
DECLSPEC_IMPORT BOOL WINAPI EndPaint(HWND, const PAINTSTRUCT*);
DECLSPEC_IMPORT HDC WINAPI CreateCompatibleDC(HDC);
DECLSPEC_IMPORT HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
DECLSPEC_IMPORT int WINAPI GetObjectA(HGDIOBJ, int, void*);
DECLSPEC_IMPORT BOOL WINAPI BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD);
DECLSPEC_IMPORT BOOL WINAPI DeleteDC(HDC);
DECLSPEC_IMPORT void WINAPI GlobalMemoryStatus(MEMORYSTATUS*);
DECLSPEC_IMPORT HRESULT WINAPI CoInitialize(void*);
DECLSPEC_IMPORT HRESULT WINAPI CoCreateInstance(const GUID&, void*, DWORD, const GUID&, void**);
// 0x5492ac / 0x5492b0: the original calls plain VariantInit / VariantClear (VARIANT is all this needs,
// and PropVariantInit would print the same two names but VariantClear returns void). Declaring VariantClear
// as void is what keeps the original's "lea/push; call; pop" cleanup shape.
DECLSPEC_IMPORT void WINAPI VariantInit(VARIANT*);
DECLSPEC_IMPORT void WINAPI VariantClear(VARIANT*);
DECLSPEC_IMPORT int __cdecl wcstombs(char*, const WCHAR*, unsigned);
}
// GetDlgItemA: windows.h only provides the A/W-neutral GetDlgItem macro, so declare the A-suffixed import
// (the original reaches it through its import table at 0x54932c).
extern "C" __declspec(dllimport) HWND __stdcall GetDlgItemA(HWND, int);   // 0x54932c

long __cdecl _atol(const char*);                    // 0x53734d
#define atol _atol

// 0x4b9110: HTTP GET of the version file; returns > 0 on failure, gives the body back in *out.
extern int http_fetch(const char* url, const char* post, int post_len, int len, int out);   // 0x4b9110

// 0x8ee3c48: the application object (DirectX sample CD3DApplication).
struct Application {
    virtual bool FindBestFullscreenMode(bool requireHAL, bool requireREF);   // 0
    virtual int  AdjustWindowForChange();                                    // 1
    virtual void UpdateStats();                                              // 2
    virtual int  ConfirmDevice(int a, int b, int c, int d);                  // 3
    virtual int  v4();  virtual int  InitDeviceObjects();  virtual int  RestoreDeviceObjects();
    virtual int  FrameMove();  virtual int  Render();  virtual int  InvalidateDeviceObjects();
    virtual int  DeleteDeviceObjects();  virtual int  v11();  virtual int  Create(HINSTANCE h);
    virtual int  v13();  virtual int  v14();  virtual int  v15();
    virtual void Pause(bool p);                                              // 16 0x401480
    char pad[0x33c];
    int  ChooseInitialD3DSettings();                                         // 0x402100
    int  DisplayErrorMsg(int hr, int type);                                  // 0x4014f0
    int  HandleSizeChange();                                                 // 0x402be0
};
extern Application g_app_obj;   // 0x8ee3c48
extern char g_settings_probed;  // 0x8ee3cec

// 0x5aa0a8..0x5aa110: the options block read from rgl_config.txt (see s004bf170); the GPU-memory
// probe only touches these words.
extern int g_opt_5aa0ac;        // 0x5aa0ac
extern int g_opt_5aa0dc;        // 0x5aa0dc
extern int g_opt_5aa0e0;        // 0x5aa0e0
extern int g_opt_5aa100;        // 0x5aa100
extern int g_opt_5aa110;        // 0x5aa110

// The options dialog object (offset-only knowledge).
struct OptionsDlg {
    char pad0[0x354];
    HBITMAP back_bitmap;        // +0x354
    char pad358[0x364 - 0x358];
    HWND paint_hwnd;            // +0x364
    char pad368[0x380 - 0x368];
    HWND hwnd;                  // +0x380
    void set_active_tab(int tab);           // 0x4b9790
    void paint();                           // 0x4b9af0
    void choose_dx_settings(bool overwrite);// 0x4b9b90
};

// minimal tstring (16 bytes; layout verified in match/include/rgl.h, members 0x42bbe0..0x42bd10)
struct tstring {
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring(const char* s);     // 0x42bbe0
    ~tstring();                 // 0x42bc40
    char equals(const char* s) const;   // 0x42bd10
    void to_upper();                    // 0x42bfe0
};

// DirectX diagnostic COM interfaces (dxdiag.h, not on the toolchain); DXDiag bases its objects directly on
// IUnknown and only the slots this slice calls are listed (Initialize +0xc, GetRootContainer +0x10,
// GetChildContainer +0x14, EnumChildContainerName +0x10, GetProp +0x20).
struct IDxDiagContainer : public IUnknown {
    virtual HRESULT __stdcall GetNumberOfChildContainers(DWORD* n) = 0;
    virtual HRESULT __stdcall EnumChildContainerName(DWORD idx, WCHAR* name, DWORD cap) = 0;
    virtual HRESULT __stdcall GetChildContainer(const WCHAR* name, IDxDiagContainer** c) = 0;
    virtual HRESULT __stdcall GetNumberOfProps(DWORD* n) = 0;
    virtual HRESULT __stdcall EnumPropName(DWORD idx, WCHAR* name, DWORD cap) = 0;
    virtual HRESULT __stdcall GetProp(WCHAR* name, VARIANT* v) = 0;    virtual HRESULT __stdcall GetPropByName(const WCHAR* prop, const WCHAR* name, VARIANT* v) = 0;
    virtual HRESULT __stdcall GetContainerByIndex(DWORD idx, IDxDiagContainer** c) = 0;
};
struct IDxDiagProvider : public IUnknown {
    virtual HRESULT __stdcall Initialize(void* params) = 0;
    virtual HRESULT __stdcall GetRootContainer(IDxDiagContainer** root) = 0;
};
extern const GUID CLSID_DxDiagProvider;     // 0x59c8e4
extern const GUID IID_IDxDiagProvider;      // 0x59c8f4

// 0x593a94: property name "szDisplayMemoryEnglish" (wide).
extern WCHAR g_prop_display_memory[22];             // 0x593a94
// 0x4b9a30: reads a DXDiag container property (as a wide string) into a char buffer. The callee itself
// passes the property name (0x593a94) to GetProp; the original also carries a fourth (unused) stack
// argument, which the mangled symbol records as int.
static long __cdecl container_prop_string_impl(IDxDiagContainer* obj, WCHAR* name, char* buf, unsigned cap);

// @ 0x4b9aa0
long get_latest_version()
{
    int body[4];
    body[0] = 0;
    body[1] = 0;
    body[2] = 0;
    if (http_fetch("http://www.taleworlds.com/mb_version.txt", 0, 0, 0, (int)body) > 0)
        return -1;
    return _atol((const char*)body[2]);
}

// @ 0x4b9790
void OptionsDlg::set_active_tab(int tab)
{
    int show = (tab != 0) - 1 & 5;
    HWND h;
    ShowWindow(GetDlgItemA(hwnd, 0x43f), show);
    ShowWindow(GetDlgItemA(hwnd, 0x449), show);
    ShowWindow(GetDlgItemA(hwnd, 0x447), show);
    ShowWindow(GetDlgItemA(hwnd, 0x450), show);
    ShowWindow(GetDlgItemA(hwnd, 0x43e), show);
    ShowWindow(GetDlgItemA(hwnd, 0x441), show);
    ShowWindow(GetDlgItemA(hwnd, 0x493), show);
    ShowWindow(GetDlgItemA(hwnd, 0x468), show);
    ShowWindow(GetDlgItemA(hwnd, 0x444), show);
    ShowWindow(GetDlgItemA(hwnd, 0x448), show);
    ShowWindow(GetDlgItemA(hwnd, 0x445), show);
    ShowWindow(GetDlgItemA(hwnd, 0x487), show);
    ShowWindow(GetDlgItemA(hwnd, 0x460), show);
    ShowWindow(GetDlgItemA(hwnd, 0x45f), show);
    ShowWindow(GetDlgItemA(hwnd, 0x462), show);
    ShowWindow(GetDlgItemA(hwnd, 0x465), show);
    ShowWindow(GetDlgItemA(hwnd, 0x492), show);
    ShowWindow(GetDlgItemA(hwnd, 0x467), show);
    show = (tab != 1) - 1 & 5;
    ShowWindow(GetDlgItemA(hwnd, 0x452), show);
    ShowWindow(GetDlgItemA(hwnd, 0x446), show);
    ShowWindow(GetDlgItemA(hwnd, 0x44d), show);
    show = (tab != 2) - 1 & 5;
    ShowWindow(GetDlgItemA(hwnd, 0x443), show);
    ShowWindow(GetDlgItemA(hwnd, 0x442), show);
    show = (tab != 3) - 1 & 5;
    ShowWindow(GetDlgItemA(hwnd, 0x44c), show);
    ShowWindow(GetDlgItemA(hwnd, 0x44a), show);
}

// @ 0x4b99c0
void run_settings_probe()
{
    if (g_settings_probed)
        return;
    g_app_obj.Pause(true);
    g_settings_probed = 1;
    if (g_app_obj.ChooseInitialD3DSettings() < 0)
        g_app_obj.DisplayErrorMsg(0x8200000c, 1);
    if (g_app_obj.HandleSizeChange() < 0)
        g_app_obj.DisplayErrorMsg(0x8200000c, 1);
    g_app_obj.Pause(false);
}

// @ 0x4b9a30
// The callee receives the wide property name in its second stack slot and hands that along as GetProp's
// filter argument; the fourth stack slot (the wrapper's `int`) is never read, exactly like the original,
// which still rounds the stack with `ret 0x10`.
static long __cdecl container_prop_string_impl(IDxDiagContainer* obj, WCHAR* name, char* buf, unsigned cap)
{
    VARIANT result;
    VariantInit(&result);
    if (obj->GetProp(name, &result) < 0)
        return -2147024809;     // E_INVALIDARG
    if (result.vt != 8) {
        VariantClear(&result);
        return -2147024809;     // E_INVALIDARG (not a BSTR)
    }
    wcstombs(buf, result.pwszVal, cap);
    buf[cap - 1] = 0;
    VariantClear(&result);
    return 0;
}

// @ 0x4b9af0
void OptionsDlg::paint()
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(paint_hwnd, &ps);
    HDC mem = CreateCompatibleDC(dc);
    HGDIOBJ old = SelectObject(mem, back_bitmap);
    BITMAP bm;
    GetObjectA(back_bitmap, sizeof(BITMAP), &bm);
    BitBlt(dc, 0x108, 0x3f, 0x154, 0x113, mem, 0, 0, 0xcc0020);
    SelectObject(mem, old);
    DeleteDC(mem);
    EndPaint(paint_hwnd, &ps);
}

// @ 0x4b9b90
void OptionsDlg::choose_dx_settings(bool overwrite)
{
    MEMORYSTATUS* mem = (MEMORYSTATUS*)0;
    GlobalMemoryStatus(mem);

    IDxDiagContainer* devices = 0;
    IDxDiagContainer* device = 0;
    IDxDiagContainer* display = 0;
    IDxDiagProvider* provider = 0;
    DWORD params[4];
    WCHAR* text;
    WCHAR name[256];
    char mem_text[256];

    CoInitialize(0);
    if (CoCreateInstance(CLSID_DxDiagProvider, 0, 1, IID_IDxDiagProvider, (void**)&provider) < 0)
        return;
    if (!provider)
        return;
    params[0] = 0x10;
    params[1] = 0x6f;
    params[2] = 0;
    params[3] = 0;
    if (provider->Initialize(params) < 0)
        return;
    if (provider->GetRootContainer(&devices) < 0)
        return;
    if (devices->GetChildContainer(L"DxDiag_DisplayDevices", &device) < 0)
        return;
    if (device->EnumChildContainerName(0, name, 256) < 0)
        return;
    if (device->GetChildContainer(name, &display) < 0)
        return;
    if (container_prop_string_impl(display, L"szDisplayMemoryEnglish", (char*)text, 256) < 0)
        return;
    tstring s((const char*)text);
    s.to_upper();
    if (s.equals("4.0 MB") || s.equals("8.0 MB") || s.equals("16.0 MB")) {
        g_opt_5aa0ac = 0;
        g_opt_5aa100 = 0;
        g_opt_5aa0dc = 0;
        g_opt_5aa110 = 0;
    } else if (s.equals("32.0 MB")) {
        if (overwrite) {
            g_opt_5aa0ac = 0x14;
            g_opt_5aa0dc = 0;
            g_opt_5aa110 = 0;
        }
    } else if (s.equals("64.0 MB")) {
        if (overwrite) {
            g_opt_5aa0ac = 0x3c;
            g_opt_5aa0dc = 0;
            g_opt_5aa110 = 0;
        }
    } else if (s.equals("128.0 MB")) {
        if (overwrite) {
            g_opt_5aa0ac = 0x50;
            g_opt_5aa110 = 1;
        }
    } else if (s.equals("256.0 MB")) {
        if (overwrite) {
            g_opt_5aa0ac = 0x64;
            g_opt_5aa110 = 1;
        }
    }
    if (display) {
        display->Release();
        display = 0;
    }
    if (device) {
        device->Release();
        device = 0;
    }
    if (devices) {
        devices->Release();
        devices = 0;
    }
    if (provider)
        provider->Release();
}
