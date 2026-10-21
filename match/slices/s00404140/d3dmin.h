// Minimal Win32 / GDI / Direct3D9 declarations for slices s00404140, s00404a00, s00404f00.
// windows.h and d3d9.h from SDK 6.0A do not parse under cl 13.10 (they need sal.h), so the pieces used here
// are declared locally with the real layouts and vtable slot numbers.
#ifndef MB_D3DMIN_H
#define MB_D3DMIN_H

typedef long HRESULT;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef int BOOL;
typedef long LONG;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef void* HANDLE;
typedef struct HDC__* HDC;
typedef struct HWND__* HWND;
typedef struct HFONT__* HFONT;
typedef struct HBITMAP__* HBITMAP;
typedef void* HGDIOBJ;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef long LRESULT;
#define WINAPI __stdcall
#define DLLI extern "C" __declspec(dllimport)
#define TRUE 1
#define FALSE 0
#define NULL 0
#define S_OK ((HRESULT)0)
#define E_FAIL ((HRESULT)0x80004005L)
#define E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define D3DERR_MOREDATA ((HRESULT)0x88760867L)

struct SIZE { LONG cx, cy; };
struct RECT { LONG left, top, right, bottom; };
struct BITMAPINFOHEADER {
    DWORD biSize; LONG biWidth, biHeight; WORD biPlanes, biBitCount; DWORD biCompression, biSizeImage;
    LONG biXPelsPerMeter, biYPelsPerMeter; DWORD biClrUsed, biClrImportant;
};
struct BITMAPINFO { BITMAPINFOHEADER bmiHeader; DWORD bmiColors[1]; };

DLLI HDC WINAPI CreateCompatibleDC(HDC);
DLLI int WINAPI SetMapMode(HDC, int);
DLLI int WINAPI GetDeviceCaps(HDC, int);
DLLI int WINAPI MulDiv(int, int, int);
DLLI HFONT WINAPI CreateFontA(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, const char*);
DLLI HGDIOBJ WINAPI SelectObject(HDC, HGDIOBJ);
DLLI BOOL WINAPI DeleteObject(HGDIOBJ);
DLLI BOOL WINAPI DeleteDC(HDC);
DLLI BOOL WINAPI GetTextExtentPoint32A(HDC, const char*, int, SIZE*);
DLLI BOOL WINAPI ExtTextOutA(HDC, int, int, UINT, const RECT*, const char*, UINT, const int*);
DLLI HBITMAP WINAPI CreateDIBSection(HDC, const BITMAPINFO*, UINT, void**, HANDLE, DWORD);
DLLI DWORD WINAPI SetTextColor(HDC, DWORD);
DLLI DWORD WINAPI SetBkColor(HDC, DWORD);
DLLI UINT WINAPI SetTextAlign(HDC, UINT);
DLLI HWND WINAPI GetDlgItem(HWND, int);
DLLI LRESULT WINAPI SendMessageA(HWND, UINT, WPARAM, LPARAM);
DLLI BOOL WINAPI PostMessageA(HWND, UINT, WPARAM, LPARAM);
DLLI int WINAPI lstrcmpA(const char*, const char*);

// ---- Direct3D 9
enum D3DFORMAT { D3DFMT_UNKNOWN = 0, D3DFMT_X8R8G8B8 = 22, D3DFMT_R5G6B5 = 23, D3DFMT_X1R5G5B5 = 24, D3DFMT_A2R10G10B10 = 35 };
struct D3DDISPLAYMODE { UINT Width, Height, RefreshRate; D3DFORMAT Format; };
#pragma pack(push, 4)
struct D3DADAPTER_IDENTIFIER9 {
    char Driver[512]; char Description[512]; char DeviceName[32];
    LONG DriverVersionLow; LONG DriverVersionHigh;
    DWORD VendorId, DeviceId, SubSysId, Revision; DWORD Guid[4]; DWORD WHQLLevel;
};
#pragma pack(pop)
struct D3DCAPS9 { int DeviceType; UINT AdapterOrdinal; char r1[0x58 - 8]; DWORD MaxTextureWidth; DWORD MaxTextureHeight; char r2[304 - 0x60]; };
struct D3DSURFACE_DESC { D3DFORMAT Format; int Type; DWORD Usage; int Pool; int MultiSampleType; DWORD MultiSampleQuality; UINT Width, Height; };
struct D3DVIEWPORT9 { DWORD X, Y, Width, Height; float MinZ, MaxZ; };
struct D3DLOCKED_RECT { int Pitch; void* pBits; };

struct IUnknown {
    virtual HRESULT __stdcall QueryInterface(const void*, void**) = 0;
    virtual DWORD __stdcall AddRef() = 0;
    virtual DWORD __stdcall Release() = 0;
};
struct IDirect3D9 : IUnknown {
    virtual HRESULT __stdcall RegisterSoftwareDevice(void*) = 0;
    virtual UINT __stdcall GetAdapterCount() = 0;
    virtual HRESULT __stdcall GetAdapterIdentifier(UINT, DWORD, D3DADAPTER_IDENTIFIER9*) = 0;
    virtual UINT __stdcall GetAdapterModeCount(UINT, D3DFORMAT) = 0;
    virtual HRESULT __stdcall EnumAdapterModes(UINT, D3DFORMAT, UINT, D3DDISPLAYMODE*) = 0;
    virtual HRESULT __stdcall GetAdapterDisplayMode(UINT, D3DDISPLAYMODE*) = 0;
    virtual HRESULT __stdcall CheckDeviceType(UINT, int, D3DFORMAT, D3DFORMAT, BOOL) = 0;
    virtual HRESULT __stdcall CheckDeviceFormat(UINT, int, D3DFORMAT, DWORD, int, D3DFORMAT) = 0;
};
struct IDirect3DSurface9 : IUnknown {
    virtual HRESULT __stdcall GetDevice(void**) = 0;
    virtual HRESULT __stdcall SetPrivateData() = 0;
    virtual HRESULT __stdcall GetPrivateData() = 0;
    virtual HRESULT __stdcall FreePrivateData() = 0;
    virtual DWORD __stdcall SetPriority() = 0;
    virtual DWORD __stdcall GetPriority() = 0;
    virtual void __stdcall PreLoad() = 0;
    virtual int __stdcall GetType() = 0;
    virtual HRESULT __stdcall GetContainer() = 0;
    virtual HRESULT __stdcall GetDesc(D3DSURFACE_DESC*) = 0;
};
struct IDirect3DTexture9 : IUnknown {
    virtual HRESULT __stdcall GetDevice(void**) = 0;
    virtual HRESULT __stdcall SetPrivateData() = 0;
    virtual HRESULT __stdcall GetPrivateData() = 0;
    virtual HRESULT __stdcall FreePrivateData() = 0;
    virtual DWORD __stdcall SetPriority() = 0;
    virtual DWORD __stdcall GetPriority() = 0;
    virtual void __stdcall PreLoad() = 0;
    virtual int __stdcall GetType() = 0;
    virtual DWORD __stdcall SetLOD() = 0;
    virtual DWORD __stdcall GetLOD() = 0;
    virtual DWORD __stdcall GetLevelCount() = 0;
    virtual HRESULT __stdcall SetAutoGenFilterType() = 0;
    virtual int __stdcall GetAutoGenFilterType() = 0;
    virtual void __stdcall GenerateMipSubLevels() = 0;
    virtual HRESULT __stdcall GetLevelDesc() = 0;
    virtual HRESULT __stdcall GetSurfaceLevel() = 0;
    virtual HRESULT __stdcall LockRect(UINT level, D3DLOCKED_RECT* pLockedRect, const RECT* pRect, DWORD flags) = 0;
    virtual HRESULT __stdcall UnlockRect(UINT level) = 0;
};
struct IDirect3DVertexBuffer9 : IUnknown {
    virtual HRESULT __stdcall GetDevice(void**) = 0;
    virtual HRESULT __stdcall SetPrivateData() = 0;
    virtual HRESULT __stdcall GetPrivateData() = 0;
    virtual HRESULT __stdcall FreePrivateData() = 0;
    virtual DWORD __stdcall SetPriority() = 0;
    virtual DWORD __stdcall GetPriority() = 0;
    virtual void __stdcall PreLoad() = 0;
    virtual int __stdcall GetType() = 0;
    virtual HRESULT __stdcall Lock(UINT offset, UINT size, void** ppData, DWORD flags) = 0;
    virtual HRESULT __stdcall Unlock() = 0;
};
struct IDirect3DStateBlock9 : IUnknown {
    virtual HRESULT __stdcall GetDevice(void**) = 0;
    virtual HRESULT __stdcall Capture() = 0;
    virtual HRESULT __stdcall Apply() = 0;
};
struct IDirect3DDevice9 : IUnknown {
    virtual HRESULT __stdcall TestCooperativeLevel() = 0;                              // 3
    virtual UINT __stdcall GetAvailableTextureMem() = 0;                               // 4
    virtual HRESULT __stdcall EvictManagedResources() = 0;                             // 5
    virtual HRESULT __stdcall GetDirect3D(IDirect3D9** ppD3D9) = 0;                    // 6
    virtual HRESULT __stdcall GetDeviceCaps(D3DCAPS9* pCaps) = 0;                      // 7
    virtual HRESULT __stdcall GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) = 0; // 8
    virtual HRESULT __stdcall s9() = 0; virtual HRESULT __stdcall s10() = 0; virtual HRESULT __stdcall s11() = 0;
    virtual HRESULT __stdcall s12() = 0; virtual HRESULT __stdcall s13() = 0; virtual HRESULT __stdcall s14() = 0;
    virtual HRESULT __stdcall s15() = 0; virtual HRESULT __stdcall s16() = 0; virtual HRESULT __stdcall s17() = 0;
    virtual HRESULT __stdcall s18() = 0; virtual HRESULT __stdcall s19() = 0; virtual HRESULT __stdcall s20() = 0;
    virtual HRESULT __stdcall s21() = 0; virtual HRESULT __stdcall s22() = 0;
    virtual HRESULT __stdcall CreateTexture(UINT w, UINT h, UINT levels, DWORD usage, D3DFORMAT fmt, int pool,
                                            IDirect3DTexture9** ppTex, HANDLE* pShared) = 0;   // 23
    virtual HRESULT __stdcall s24() = 0; virtual HRESULT __stdcall s25() = 0;
    virtual HRESULT __stdcall CreateVertexBuffer(UINT length, DWORD usage, DWORD fvf, int pool,
                                                 IDirect3DVertexBuffer9** ppVB, HANDLE* pShared) = 0; // 26
    virtual HRESULT __stdcall s27() = 0; virtual HRESULT __stdcall s28() = 0; virtual HRESULT __stdcall s29() = 0;
    virtual HRESULT __stdcall s30() = 0; virtual HRESULT __stdcall s31() = 0; virtual HRESULT __stdcall s32() = 0;
    virtual HRESULT __stdcall s33() = 0; virtual HRESULT __stdcall s34() = 0; virtual HRESULT __stdcall s35() = 0;
    virtual HRESULT __stdcall s36() = 0; virtual HRESULT __stdcall s37() = 0;
    virtual HRESULT __stdcall GetRenderTarget(DWORD index, IDirect3DSurface9** ppSurface) = 0; // 38
    virtual HRESULT __stdcall s39() = 0; virtual HRESULT __stdcall s40() = 0; virtual HRESULT __stdcall s41() = 0;
    virtual HRESULT __stdcall s42() = 0; virtual HRESULT __stdcall s43() = 0; virtual HRESULT __stdcall s44() = 0;
    virtual HRESULT __stdcall s45() = 0; virtual HRESULT __stdcall s46() = 0;
    virtual HRESULT __stdcall s47() = 0;
    virtual HRESULT __stdcall GetViewport(D3DVIEWPORT9* pViewport) = 0;                // 48
    virtual HRESULT __stdcall s49() = 0; virtual HRESULT __stdcall s50() = 0; virtual HRESULT __stdcall s51() = 0;
    virtual HRESULT __stdcall s52() = 0; virtual HRESULT __stdcall s53() = 0; virtual HRESULT __stdcall s54() = 0;
    virtual HRESULT __stdcall s55() = 0; virtual HRESULT __stdcall s56() = 0;
    virtual HRESULT __stdcall SetRenderState(int state, DWORD value) = 0;              // 57
    virtual HRESULT __stdcall s58() = 0; virtual HRESULT __stdcall s59() = 0;
    virtual HRESULT __stdcall BeginStateBlock() = 0;                                   // 60
    virtual HRESULT __stdcall EndStateBlock(IDirect3DStateBlock9** ppSB) = 0;          // 61
    virtual HRESULT __stdcall s62() = 0; virtual HRESULT __stdcall s63() = 0; virtual HRESULT __stdcall s64() = 0;
    virtual HRESULT __stdcall SetTexture(DWORD stage, IDirect3DTexture9* pTexture) = 0; // 65
    virtual HRESULT __stdcall s66() = 0;
    virtual HRESULT __stdcall SetTextureStageState(DWORD stage, int type, DWORD value) = 0; // 67
    virtual HRESULT __stdcall s68() = 0;
    virtual HRESULT __stdcall SetSamplerState(DWORD sampler, int type, DWORD value) = 0; // 69
    virtual HRESULT __stdcall s70() = 0; virtual HRESULT __stdcall s71() = 0; virtual HRESULT __stdcall s72() = 0;
    virtual HRESULT __stdcall s73() = 0; virtual HRESULT __stdcall s74() = 0; virtual HRESULT __stdcall s75() = 0;
    virtual HRESULT __stdcall s76() = 0; virtual HRESULT __stdcall s77() = 0; virtual HRESULT __stdcall s78() = 0;
    virtual HRESULT __stdcall s79() = 0; virtual HRESULT __stdcall s80() = 0;
    virtual HRESULT __stdcall DrawPrimitive(int type, UINT startVertex, UINT primCount) = 0; // 81
    virtual HRESULT __stdcall s82() = 0; virtual HRESULT __stdcall s83() = 0; virtual HRESULT __stdcall s84() = 0;
    virtual HRESULT __stdcall s85() = 0; virtual HRESULT __stdcall s86() = 0; virtual HRESULT __stdcall s87() = 0;
    virtual HRESULT __stdcall s88() = 0;
    virtual HRESULT __stdcall SetFVF(DWORD fvf) = 0;                                   // 89
    virtual HRESULT __stdcall s90() = 0; virtual HRESULT __stdcall s91() = 0; virtual HRESULT __stdcall s92() = 0;
    virtual HRESULT __stdcall s93() = 0; virtual HRESULT __stdcall s94() = 0; virtual HRESULT __stdcall s95() = 0;
    virtual HRESULT __stdcall s96() = 0; virtual HRESULT __stdcall s97() = 0; virtual HRESULT __stdcall s98() = 0;
    virtual HRESULT __stdcall s99() = 0;
    virtual HRESULT __stdcall SetStreamSource(UINT stream, IDirect3DVertexBuffer9* pVB, UINT offset, UINT stride) = 0; // 100
    virtual HRESULT __stdcall s101() = 0; virtual HRESULT __stdcall s102() = 0; virtual HRESULT __stdcall s103() = 0;
    virtual HRESULT __stdcall s104() = 0; virtual HRESULT __stdcall s105() = 0; virtual HRESULT __stdcall s106() = 0;
    virtual HRESULT __stdcall SetPixelShader(void* pShader) = 0;                       // 107
};
#endif
