//-----------------------------------------------------------------------------
// File: D3DEnumeration.h
//
// Desc: Enumerates D3D adapters, devices, modes, etc.
//       (DirectX 9 SDK sample framework; layouts verified against Mount & Blade 1.011)
//-----------------------------------------------------------------------------
#ifndef D3DENUM_H
#define D3DENUM_H

#include <windows.h>
#include <d3d9.h>
#include "dxutil.h"

//-----------------------------------------------------------------------------
// Name: enum VertexProcessingType
// Desc: Enumeration of all possible D3D vertex processing types.
//-----------------------------------------------------------------------------
enum VertexProcessingType
{
    SOFTWARE_VP,
    MIXED_VP,
    HARDWARE_VP,
    PURE_HARDWARE_VP
};

//-----------------------------------------------------------------------------
// Name: struct D3DAdapterInfo
// Desc: Info about a display adapter. (0x458 bytes)
//-----------------------------------------------------------------------------
struct D3DAdapterInfo
{
    int AdapterOrdinal;                         // +0x000
    D3DADAPTER_IDENTIFIER9 AdapterIdentifier;   // +0x004
    CArrayList* pDisplayModeList;               // +0x450  List of D3DDISPLAYMODEs
    CArrayList* pDeviceInfoList;                // +0x454  List of D3DDeviceInfo pointers
    ~D3DAdapterInfo( void );                    // 0x4040c0
};

//-----------------------------------------------------------------------------
// Name: struct D3DDeviceInfo
// Desc: Info about a D3D device, including a list of D3DDeviceCombos (see below)
//       that work with the device. (0x13c bytes)
//-----------------------------------------------------------------------------
struct D3DDeviceInfo
{
    int AdapterOrdinal;                         // +0x000
    D3DDEVTYPE DevType;                         // +0x004
    D3DCAPS9 Caps;                              // +0x008
    CArrayList* pDeviceComboList;               // +0x138  List of D3DDeviceCombo pointers
    ~D3DDeviceInfo( void );                     // 0x403bf0
};

//-----------------------------------------------------------------------------
// Name: struct D3DDSMSConflict
// Desc: A depth/stencil buffer format that is incompatible with a
//       multisample type.
//-----------------------------------------------------------------------------
struct D3DDSMSConflict
{
    D3DFORMAT DSFormat;
    D3DMULTISAMPLE_TYPE MSType;
};

//-----------------------------------------------------------------------------
// Name: struct D3DDeviceCombo
// Desc: A combination of adapter format, back buffer format, and windowed/fullscreen
//       that is compatible with a particular D3D device (and the app). (0x2c bytes:
//       operator new(0x2c) in EnumerateDeviceCombos)
//-----------------------------------------------------------------------------
struct D3DDeviceCombo
{
    int AdapterOrdinal;                         // +0x00
    D3DDEVTYPE DevType;                         // +0x04
    D3DFORMAT AdapterFormat;                    // +0x08
    D3DFORMAT BackBufferFormat;                 // +0x0c
    bool IsWindowed;                            // +0x10
    CArrayList* pDepthStencilFormatList;        // +0x14  List of D3DFORMATs
    CArrayList* pMultiSampleTypeList;           // +0x18  List of D3DMULTISAMPLE_TYPEs
    CArrayList* pMultiSampleQualityList;        // +0x1c  List of DWORDs (number of quality levels for each multisample type)
    CArrayList* pDSMSConflictList;              // +0x20  List of D3DDSMSConflicts
    CArrayList* pVertexProcessingTypeList;      // +0x24  List of VertexProcessingTypes
    CArrayList* pPresentIntervalList;           // +0x28  List of D3DPRESENT_INTERVALs

    ~D3DDeviceCombo( void );                    // 0x403640
};

typedef bool(* CONFIRMDEVICECALLBACK)( D3DCAPS9* pCaps, VertexProcessingType vertexProcessingType,
                                       D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat );

//-----------------------------------------------------------------------------
// Name: class CD3DEnumeration
// Desc: Enumerates available D3D adapters, devices, modes, etc. (0x2c bytes,
//       embedded in CD3DApplication at +0xc)
//-----------------------------------------------------------------------------
class CD3DEnumeration
{
private:
    IDirect3D9* m_pD3D;                         // +0x00

private:
    HRESULT EnumerateDevices( D3DAdapterInfo* pAdapterInfo, CArrayList* pAdapterFormatList );        // 0x403f90
    HRESULT EnumerateDeviceCombos( D3DDeviceInfo* pDeviceInfo, CArrayList* pAdapterFormatList );     // 0x403c50
    void BuildDepthStencilFormatList( D3DDeviceCombo* pDeviceCombo );                                // 0x403770
    void BuildMultiSampleTypeList( D3DDeviceCombo* pDeviceCombo );                                   // 0x4038c0
    void BuildDSMSConflictList( D3DDeviceCombo* pDeviceCombo );                                      // 0x403b40
    void BuildVertexProcessingTypeList( D3DDeviceInfo* pDeviceInfo, D3DDeviceCombo* pDeviceCombo );  // 0x4039b0
    void BuildPresentIntervalList( D3DDeviceInfo* pDeviceInfo, D3DDeviceCombo* pDeviceCombo );       // 0x403ab0

public:
    CArrayList* m_pAdapterInfoList;             // +0x04
    // The following variables can be used to limit what modes, formats,
    // etc. are enumerated.  Set them to the values you want before calling
    // Enumerate().
    CONFIRMDEVICECALLBACK ConfirmDeviceCallback;// +0x08
    UINT AppMinFullscreenWidth;                 // +0x0c
    UINT AppMinFullscreenHeight;                // +0x10
    UINT AppMinColorChannelBits;                // +0x14  min color bits per channel in adapter format
    UINT AppMinAlphaChannelBits;                // +0x18  min alpha bits per pixel in back buffer format
    UINT AppMinDepthBits;                       // +0x1c
    UINT AppMinStencilBits;                     // +0x20
    bool AppUsesDepthBuffer;                    // +0x24
    bool AppUsesMixedVP;                        // +0x25  whether app can take advantage of mixed vp mode
    bool AppRequiresWindowed;                   // +0x26
    bool AppRequiresFullscreen;                 // +0x27
    CArrayList* m_pAllowedAdapterFormatList;    // +0x28  list of D3DFORMATs

    CD3DEnumeration();                          // 0x4036e0
    ~CD3DEnumeration();                         // 0x404480
    void SetD3D(IDirect3D9* pD3D) { m_pD3D = pD3D; }
    HRESULT Enumerate();                        // 0x404140
};

#endif
