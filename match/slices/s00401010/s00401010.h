// Shared declarations for slices s00401010 / s00401910 / s00402280: the Direct3D application base class
// (a modified DirectX 9 SDK "CD3DApplication" framework; names provisional).
#ifndef MB_APP_H
#define MB_APP_H
// <sal.h> is not on the toolchain include path (papercut): alias it to the VC9 copy shipped with the SDK.
#pragma include_alias(<sal.h>, <Z:/Users/noahpeterson/Documents/Projects/mountandblade-decomp/work/toolchain/vc9sp1/VC/include/sal.h>)
#include <windows.h>
#include <d3d9.h>

// DirectX 9 SDK common framework (d3dapp.h, d3denum.h, d3dsettings.h), as modified for Mount & Blade.
// CArrayList: type==0 -> inline records of `bytesPerEntry` bytes at data; else array of dwords.
struct List {
    int type;                   // 0
    char* data;                 // 4
    int bytesPerEntry;          // 8
    unsigned count;             // 0xc
    void* get(unsigned i);      // 0x407ba0
};

struct D3DDeviceCombo {
    int AdapterOrdinal;         // 0
    int DeviceType;             // 4
    int AdapterFormat;          // 8
    int BackBufferFormat;       // 0xc
    bool IsWindowed;            // 0x10
    List* pDepthStencilFormatList;      // 0x14
    List* pMultiSampleTypeList;         // 0x18
    List* pMultiSampleQualityList;      // 0x1c
    List* pDSMSConflictList;            // 0x20
    List* pVertexProcessingTypeList;    // 0x24
    List* pPresentIntervalList;         // 0x28
    void* pAdapterInfo;                 // 0x2c
    void* pDeviceInfo;                  // 0x30
};
struct D3DDeviceInfo {
    int AdapterOrdinal;         // 0
    int DeviceType;             // 4
    D3DCAPS9 Caps;              // 8
    List* pDeviceComboList;     // 0x138
};
struct D3DAdapterInfo {
    int AdapterOrdinal;         // 0
    D3DADAPTER_IDENTIFIER9 AdapterIdentifier;   // 4
    List* pDisplayModeList;     // 0x450
    List* pDeviceInfoList;      // 0x454
};

struct Enumeration {            // CD3DEnumeration, 0x2c bytes, embedded at Application+0xc
    IDirect3D9* m_pD3D;         // 0
    List* m_pAdapterInfoList;   // 4
    void* ConfirmDeviceCallback;// 8
    UINT AppMinFullscreenWidth; // 0xc  (ctor: 640)
    UINT AppMinFullscreenHeight;// 0x10 (480)
    UINT AppMinColorChannelBits;// 0x14 (5)
    UINT AppMinAlphaChannelBits;// 0x18
    UINT AppMinDepthBits;       // 0x1c
    UINT AppMinStencilBits;     // 0x20
    bool AppUsesDepthBuffer;    // 0x24
    bool AppUsesMixedVP;        // 0x25
    bool AppRequiresWindowed;   // 0x26
    bool AppRequiresFullscreen; // 0x27
    List* m_pAllowedAdapterFormatList;  // 0x28
    Enumeration();              // 0x4036e0
    ~Enumeration();             // 0x404480
};

struct D3DSettings {            // 0x3c bytes at Application+0x38
    bool IsWindowed;                                // 0
    D3DAdapterInfo* pWindowed_AdapterInfo;          // 4
    D3DDeviceInfo* pWindowed_DeviceInfo;            // 8
    D3DDeviceCombo* pWindowed_DeviceCombo;          // 0xc
    D3DDISPLAYMODE Windowed_DisplayMode;            // 0x10
    int Windowed_DepthStencilBufferFormat;          // 0x20
    int Windowed_MultisampleType;                   // 0x24
    int Windowed_MultisampleQuality;                // 0x28
    int Windowed_VertexProcessingType;              // 0x2c
    int Windowed_PresentInterval;                   // 0x30
    int Window_Width, Window_Height;                // 0x34 0x38
    D3DAdapterInfo* pFullscreen_AdapterInfo;        // 0x3c
    D3DDeviceInfo* pFullscreen_DeviceInfo;          // 0x40
    D3DDeviceCombo* pFullscreen_DeviceCombo;        // 0x44
    D3DDISPLAYMODE Fullscreen_DisplayMode;          // 0x48
    int Fullscreen_DepthStencilBufferFormat;        // 0x58
    int Fullscreen_MultisampleType;                 // 0x5c
    int Fullscreen_MultisampleQuality;              // 0x60
    int Fullscreen_VertexProcessingType;            // 0x64
    int Fullscreen_PresentInterval;                 // 0x68
    D3DAdapterInfo* PAdapterInfo() { return IsWindowed ? pWindowed_AdapterInfo : pFullscreen_AdapterInfo; }
    D3DDeviceInfo* PDeviceInfo() { return IsWindowed ? pWindowed_DeviceInfo : pFullscreen_DeviceInfo; }
    D3DDeviceCombo* PDeviceCombo() { return IsWindowed ? pWindowed_DeviceCombo : pFullscreen_DeviceCombo; }
    D3DDISPLAYMODE DisplayMode() { return IsWindowed ? Windowed_DisplayMode : Fullscreen_DisplayMode; }
    int DepthStencilBufferFormat() { return IsWindowed ? Windowed_DepthStencilBufferFormat : Fullscreen_DepthStencilBufferFormat; }
    int MultisampleType() { return IsWindowed ? Windowed_MultisampleType : Fullscreen_MultisampleType; }
    int MultisampleQuality() { return IsWindowed ? Windowed_MultisampleQuality : Fullscreen_MultisampleQuality; }
    int GetVertexProcessingType() { return IsWindowed ? Windowed_VertexProcessingType : Fullscreen_VertexProcessingType; }
    int PresentInterval() { return IsWindowed ? Windowed_PresentInterval : Fullscreen_PresentInterval; }
};

struct Application {            // CD3DApplication; size >= 0x347
    virtual bool FindBestFullscreenMode(bool requireHAL, bool requireREF);   // 0 0x401d40
    virtual int  AdjustWindowForChange();                                        // 1 0x4013e0
    virtual void UpdateStats();                                              // 2 0x402580
    virtual int  ConfirmDevice(int a, int b, int c, int d);                  // 3 0x401a20
    virtual int  v4();
    virtual int  InitDeviceObjects();                                        // 5
    virtual int  RestoreDeviceObjects();                                     // 6
    virtual int  FrameMove();                                                // 7
    virtual int  Render();                                                   // 8
    virtual int  InvalidateDeviceObjects();                                  // 9
    virtual int  DeleteDeviceObjects();                                      // 10
    virtual int  v11();
    virtual int  Create(HINSTANCE h);                                        // 12 0x401070
    virtual int  v13();
    virtual int  v14();
    virtual int  v15();
    virtual void Pause(bool p);                                              // 16 0x401480
    virtual ~Application();                                                  // 17 0x401a30 (deleting)

    bool  b4;                   // 4
    float fTime8;               // 8
    Enumeration m_d3dEnum;      // 0xc .. 0x38
    D3DSettings m_d3dSettings;  // 0x38 .. 0xa4
    bool  m_bWindowed;          // 0xa4
    bool  m_bActive;            // 0xa5
    bool  m_bDeviceLost;        // 0xa6
    bool  m_bMinimized;         // 0xa7
    bool  m_bMaximized;         // 0xa8
    bool  m_bIgnoreSizeChange;  // 0xa9
    bool  m_bDeviceObjectsInited;   // 0xaa
    bool  m_bDeviceObjectsRestored; // 0xab
    bool  m_bFrameMoving;       // 0xac
    bool  m_bSingleStep;        // 0xad
    char padAE[2];
    D3DPRESENT_PARAMETERS m_d3dpp;  // 0xb0 .. 0xe8
    HWND m_hWnd;                // 0xe8
    HWND m_hWndFocus;           // 0xec
    HMENU m_hMenu;              // 0xf0
    IDirect3D9* m_pD3D;         // 0xf4
    IDirect3DDevice9* m_pd3dDevice; // 0xf8
    D3DCAPS9 m_d3dCaps;         // 0xfc
    D3DSURFACE_DESC m_d3dsdBackBuffer;  // 0x22c
    DWORD m_dwCreateFlags;      // 0x24c
    DWORD m_dwWindowStyle;      // 0x250
    RECT m_rcWindowBounds;      // 0x254
    RECT m_rcWindowClient;      // 0x264
    float m_fTime;              // 0x274
    float m_fElapsedTime;       // 0x278
    float m_fFPS;               // 0x27c
    char m_strDeviceStats[90];  // 0x280
    char m_strFrameStats[90];   // 0x2da
    int i334;                   // 0x334 (zeroed by the constructor)
    const char* m_strWindowTitle;   // 0x338
    int m_dwCreationWidth;      // 0x33c (400)
    int m_dwCreationHeight;     // 0x340 (300)
    bool m_bShowCursorWhenFullscreen;   // 0x344
    bool m_bClipCursorWhenFullscreen;   // 0x345
    bool m_bStartFullscreen;    // 0x346
    char pad347;

    Application();                                                    // 0x401910
    bool FindBestWindowedMode(bool requireHAL, bool requireREF);      // 0x401a60
    int  Initialize3DEnvironment();                                   // 0x4027c0
    int  Reset3DEnvironment();                                        // 0x401290
    int  DisplayErrorMsg(int hr, int type);                           // 0x4014f0
    void BuildPresentParamsFromSettings();                            // 0x402280
    int  Render3DEnvironment();                                       // 0x402430
    void Cleanup3DEnvironment();                                      // 0x4026e0
    int  ChooseInitialD3DSettings();                                  // 0x402100
    int  HandlePossibleSizeChange();                                  // 0x402150
};
extern Application* g_app;      // 0x5d6f40
float __stdcall GetTimer(int which);   // 0x407660 (DXUtil_Timer: 4 abs time, 5 app time, 6 elapsed, 1 start, 2 stop)
const char* D3DUtil_D3DFormatToString(int fmt, bool prefix);   // 0x407250 (cdecl)
void DXUtil_ConvertAnsiStringToGenericCch(char* dst, const char* src, int cch);   // 0x407a30 (cdecl)

#endif
