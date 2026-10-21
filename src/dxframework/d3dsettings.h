//-----------------------------------------------------------------------------
// File: D3DSettings.h
//
// Desc: Settings class and change-settings dialog class for the Direct3D
//       samples framework library. (DirectX 9 SDK; layouts verified against
//       Mount & Blade 1.011)
//-----------------------------------------------------------------------------
#ifndef D3DSETTINGS_H
#define D3DSETTINGS_H

#include <windows.h>
#include <d3d9.h>
#include "d3denumeration.h"

//-----------------------------------------------------------------------------
// Name: class D3DSettings
// Desc: Current D3D settings: adapter, device, mode, formats, etc. (0x6c bytes)
//-----------------------------------------------------------------------------
class D3DSettings
{
public:
    bool IsWindowed;                                    // +0x00

    D3DAdapterInfo* pWindowed_AdapterInfo;              // +0x04
    D3DDeviceInfo* pWindowed_DeviceInfo;                // +0x08
    D3DDeviceCombo* pWindowed_DeviceCombo;              // +0x0c

    D3DDISPLAYMODE Windowed_DisplayMode;                // +0x10  not changable by the user
    D3DFORMAT Windowed_DepthStencilBufferFormat;        // +0x20
    D3DMULTISAMPLE_TYPE Windowed_MultisampleType;       // +0x24
    DWORD Windowed_MultisampleQuality;                  // +0x28
    VertexProcessingType Windowed_VertexProcessingType; // +0x2c
    UINT Windowed_PresentInterval;                      // +0x30
    int Windowed_Width;                                 // +0x34
    int Windowed_Height;                                // +0x38

    D3DAdapterInfo* pFullscreen_AdapterInfo;            // +0x3c
    D3DDeviceInfo* pFullscreen_DeviceInfo;              // +0x40
    D3DDeviceCombo* pFullscreen_DeviceCombo;            // +0x44

    D3DDISPLAYMODE Fullscreen_DisplayMode;              // +0x48  changable by the user
    D3DFORMAT Fullscreen_DepthStencilBufferFormat;      // +0x58
    D3DMULTISAMPLE_TYPE Fullscreen_MultisampleType;     // +0x5c
    DWORD Fullscreen_MultisampleQuality;                // +0x60
    VertexProcessingType Fullscreen_VertexProcessingType;   // +0x64
    UINT Fullscreen_PresentInterval;                    // +0x68

    D3DAdapterInfo* PAdapterInfo() { return IsWindowed ? pWindowed_AdapterInfo : pFullscreen_AdapterInfo; }
    D3DDeviceInfo* PDeviceInfo() { return IsWindowed ? pWindowed_DeviceInfo : pFullscreen_DeviceInfo; }
    D3DDeviceCombo* PDeviceCombo() { return IsWindowed ? pWindowed_DeviceCombo : pFullscreen_DeviceCombo; }

    int AdapterOrdinal() { return PDeviceCombo()->AdapterOrdinal; }
    D3DDEVTYPE DevType() { return PDeviceCombo()->DevType; }
    D3DFORMAT BackBufferFormat() { return PDeviceCombo()->BackBufferFormat; }

    D3DDISPLAYMODE DisplayMode() { return IsWindowed ? Windowed_DisplayMode : Fullscreen_DisplayMode; }
    void SetDisplayMode(D3DDISPLAYMODE value) { if (IsWindowed) Windowed_DisplayMode = value; else Fullscreen_DisplayMode = value; }

    D3DFORMAT DepthStencilBufferFormat() { return IsWindowed ? Windowed_DepthStencilBufferFormat : Fullscreen_DepthStencilBufferFormat; }
    void SetDepthStencilBufferFormat(D3DFORMAT value) { if (IsWindowed) Windowed_DepthStencilBufferFormat = value; else Fullscreen_DepthStencilBufferFormat = value; }

    D3DMULTISAMPLE_TYPE MultisampleType() { return IsWindowed ? Windowed_MultisampleType : Fullscreen_MultisampleType; }
    void SetMultisampleType(D3DMULTISAMPLE_TYPE value) { if (IsWindowed) Windowed_MultisampleType = value; else Fullscreen_MultisampleType = value; }

    DWORD MultisampleQuality() { return IsWindowed ? Windowed_MultisampleQuality : Fullscreen_MultisampleQuality; }
    void SetMultisampleQuality(DWORD value) { if (IsWindowed) Windowed_MultisampleQuality = value; else Fullscreen_MultisampleQuality = value; }

    VertexProcessingType GetVertexProcessingType() { return IsWindowed ? Windowed_VertexProcessingType : Fullscreen_VertexProcessingType; }
    void SetVertexProcessingType(VertexProcessingType value) { if (IsWindowed) Windowed_VertexProcessingType = value; else Fullscreen_VertexProcessingType = value; }

    UINT PresentInterval() { return IsWindowed ? Windowed_PresentInterval : Fullscreen_PresentInterval; }
    void SetPresentInterval(UINT value) { if (IsWindowed) Windowed_PresentInterval = value; else Fullscreen_PresentInterval = value; }
};

//-----------------------------------------------------------------------------
// Name: class CD3DSettingsDialog
// Desc: Dialog box to allow the user to change the D3D settings (0x74 bytes)
//-----------------------------------------------------------------------------
class CD3DSettingsDialog
{
private:
    HWND m_hDlg;                                // +0x00
    CD3DEnumeration* m_pEnumeration;            // +0x04
    D3DSettings m_d3dSettings;                  // +0x08

private:
    // ComboBox helper functions
    void ComboBoxAdd( int id, void* pData, LPCTSTR pstrDesc );     // 0x4056d0
    void ComboBoxSelect( int id, void* pData );                    // 0x405890
    void* ComboBoxSelected( int id );                              // 0x405760
    bool ComboBoxSomethingSelected( int id );                      // (inlined)
    UINT ComboBoxCount( int id );                                  // (inlined)
    void ComboBoxSelectIndex( int id, int index );                 // 0x405710
    void ComboBoxClear( int id );                                  // (inlined)
    bool ComboBoxContainsText( int id, LPCTSTR pstrText );         // 0x4057a0

    void AdapterChanged( void );                                   // 0x405920
    void DeviceChanged( void );                                    // 0x406d30
    void WindowedFullscreenChanged( void );                        // 0x405ac0
    void AdapterFormatChanged( void );                             // 0x405e00
    void ResolutionChanged( void );                                // 0x4061c0
    void RefreshRateChanged( void );                               // 0x405840
    void BackBufferFormatChanged( void );                          // 0x406370
    void DepthStencilBufferFormatChanged( void );                  // 0x4068c0
    void MultisampleTypeChanged( void );                           // 0x406a90
    void MultisampleQualityChanged( void );                        // 0x406c40
    void VertexProcessingChanged( void );                          // 0x406c90
    void PresentIntervalChanged( void );                           // 0x406ce0

public:
    CD3DSettingsDialog( CD3DEnumeration* pEnumeration, D3DSettings* pSettings);   // 0x4056a0
    INT_PTR ShowDialog( HWND hwndParent );                                         // 0x407230
    INT_PTR DialogProc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam );       // 0x406e50
    void GetFinalSettings( D3DSettings* pSettings ) { *pSettings = m_d3dSettings; }
};

#endif
