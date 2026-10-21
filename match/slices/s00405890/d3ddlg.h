// Shared declarations for the Direct3D settings dialog (DX9 SDK sample "D3DSettings" dialog,
// slices s00405890 / s004061c0 / s00406a90). Layouts recovered from field offsets; names follow the sample.
#ifndef MB_D3DDLG_H
#define MB_D3DDLG_H
// windows.h does not compile in this toolchain (SDK 6.0A needs the VC9 <sal.h>), so declare the Win32 pieces used.
typedef struct HWND__* HWND;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef long LRESULT;
typedef int BOOL;
#define WINAPI __stdcall
#define WINAPIV __cdecl
#define DECLSPEC_IMPORT __declspec(dllimport)
#define TRUE 1
#define FALSE 0
#define WM_INITDIALOG 0x110
#define WM_COMMAND 0x111
#define CB_ADDSTRING 0x143
#define CB_RESETCONTENT 0x14b
#define CB_GETCOUNT 0x146
#define CB_GETCURSEL 0x147
#define CB_SETCURSEL 0x14e
#define CB_GETITEMDATA 0x150
#define CB_SETITEMDATA 0x151
#define CBN_SELCHANGE 1
#define MAKEWORD_(a, b) ((unsigned short)(((unsigned char)(a)) | ((unsigned short)((unsigned char)(b))) << 8))
#define MAKELONG(a, b) ((long)(((unsigned short)(a)) | ((unsigned long)((unsigned short)(b))) << 16))
#define MAKEWPARAM(l, h) ((WPARAM)(unsigned long)MAKELONG(l, h))
extern "C" {
DECLSPEC_IMPORT HWND WINAPI GetDlgItem(HWND, int);
DECLSPEC_IMPORT LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM);
DECLSPEC_IMPORT BOOL WINAPI PostMessageA(HWND, UINT, WPARAM, LPARAM);
DECLSPEC_IMPORT BOOL WINAPI EnableWindow(HWND, BOOL);
DECLSPEC_IMPORT UINT WINAPI IsDlgButtonChecked(HWND, int);
DECLSPEC_IMPORT BOOL WINAPI CheckRadioButton(HWND, int, int, int);
DECLSPEC_IMPORT BOOL WINAPI EndDialog(HWND, int);
DECLSPEC_IMPORT char* WINAPI lstrcpyA(char*, const char*);
DECLSPEC_IMPORT int WINAPIV wsprintfA(char*, const char*, ...);
typedef BOOL (__stdcall* DLGPROC)(HWND, UINT, WPARAM, LPARAM);
DECLSPEC_IMPORT int WINAPI DialogBoxParamA(void*, const char*, HWND, DLGPROC, LPARAM);
}
#define SendMessage SendMessageA
// SendMessage(GetDlgItem(...)) evaluates GetDlgItem first in the original (args pushed after the call).
static inline LRESULT ItemMsg(HWND dlg, int id, UINT msg) { HWND h = GetDlgItem(dlg, id); return SendMessage(h, msg, 0, 0); }
#define PostMessage PostMessageA

enum {
    IDC_DEVICE_COMBO = 1000, IDC_ADAPTER_COMBO = 1002, IDC_ADAPTERFORMAT_COMBO = 1003,
    IDC_RESOLUTION_COMBO = 1004, IDC_MULTISAMPLE_TYPE_COMBO = 1005, IDC_REFRESHRATE_COMBO = 1006,
    IDC_BACKBUFFERFORMAT_COMBO = 1007, IDC_DEPTHSTENCIL_COMBO = 1008, IDC_VERTEXPROCESSING_COMBO = 1009,
    IDC_PRESENTINTERVAL_COMBO = 1010, IDC_MULTISAMPLE_QUALITY_COMBO = 1011,
    IDC_WINDOWED = 1016, IDC_FULLSCREEN = 1018
};

// CArrayList of the sample (type 0: inline entries of bytesPerEntry, else pointer array)
struct ArrayList {
    int type;                 // 0
    char* data;               // 4
    int bytesPerEntry;        // 8
    unsigned count;           // 0xc
    int allocated;            // 0x10
    void* GetPtr(unsigned i); // 0x407ba0
};

struct DisplayMode { unsigned width, height, refreshRate; int format; };   // D3DDISPLAYMODE (16 bytes)

struct DeviceCombo {          // D3DDeviceCombo
    int adapterOrdinal;       // 0
    int deviceType;           // 4
    int adapterFormat;        // 8
    int backBufferFormat;     // 0xc
    char windowed;            // 0x10
    ArrayList* depthStencilList;   // 0x14
    ArrayList* multisampleTypeList;// 0x18
    ArrayList* qualityList;        // 0x1c
    ArrayList* conflictList;       // 0x20  {dsFormat, msType}
    ArrayList* vertexProcList;     // 0x24
    ArrayList* presentIntervalList;// 0x28
};

struct DeviceInfo {           // D3DDeviceInfo
    int adapterOrdinal;       // 0
    int deviceType;           // 4
    char caps[0x130];         // 8  D3DCAPS9
    ArrayList* combos;        // 0x138
};

struct AdapterInfo {          // D3DAdapterInfo
    int adapterOrdinal;       // 0
    char identifier[0x200];   // 4
    char description[0x200];  // 0x204 (copied for the combo text)
    char pad[0x4c];
    ArrayList* displayModes;  // 0x450
    ArrayList* devices;       // 0x454
};

struct D3DEnumeration {       // the app's enumeration object (provisional)
    int pad0;
    ArrayList* adapters;      // 4
    char pad[0x1c];
    char allowVertexProc;     // 0x24  gates the DS-less vertex-processing/present-interval UI
};

struct SettingsSet {          // one half of D3DSettings (windowed @0xc, fullscreen @0x44)
    AdapterInfo* adapter;     // +0
    DeviceInfo* device;       // +4
    DeviceCombo* combo;       // +8
    DisplayMode mode;         // +0xc (width,height,refresh,adapterFormat)
    int depthStencil;         // +0x1c
    int msType;               // +0x20
    int msQuality;            // +0x24
    int vertexProc;           // +0x28
    unsigned presentInterval; // +0x2c
};

struct D3DSettingsDlg {
    HWND hDlg;                // 0
    D3DEnumeration* app;      // 4
    char windowed;            // 8
    SettingsSet win;          // 0xc
    int pad3c[2];             // 0x3c
    SettingsSet fs;           // 0x44

    void ComboAdd(int id, int data, const char* text);       // 0x4056d0
    void ComboSelectIndex(int id, int idx);                  // 0x405710
    int  ComboSelectedData(int id);                          // 0x405760
    char ComboContains(int id, const char* text);            // 0x4057a0
    void ComboSelectData(int id, int data);                  // 0x405890
    void OnRefreshRateChanged();                             // 0x405840
    void OnAdapterChanged();                                 // 0x405920
    void OnWindowedFullscreenChanged();                      // 0x405ac0
    void OnAdapterFormatChanged();                           // 0x405e00
    void OnResolutionChanged();                              // 0x4061c0
    void OnBackBufferFormatChanged();                        // 0x406370
    void OnDepthStencilChanged();                            // 0x4068c0
    void OnMultisampleTypeChanged();                         // 0x406a90
    void OnMultisampleQualityChanged();                      // 0x406c40
    void OnVertexProcessingChanged();                        // 0x406c90
    void OnPresentIntervalChanged();                         // 0x406ce0
    void OnDeviceChanged();                                  // 0x406d30
    int  DlgProc(HWND h, unsigned msg, unsigned wp, long lp);// 0x406e50
};

const char* MultisampleTypeToString(int t);                  // 0x4055e0 (cdecl)
const char* FormatToString(int fmt, int withPrefix);         // 0x407250 (cdecl)
long CopyString(char* dst, const char* src, int cap);        // 0x407a30 (cdecl, strsafe-like)
extern D3DSettingsDlg* g_settingsDlg;                        // 0x5d6f54

#endif
