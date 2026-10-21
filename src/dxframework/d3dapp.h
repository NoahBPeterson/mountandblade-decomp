//-----------------------------------------------------------------------------
// File: D3DApp.h
//
// Desc: Application class for the Direct3D samples framework library
//       (DirectX 9 SDK), as modified by TaleWorlds for Mount & Blade 1.011.
//
// M&B changes visible in the layout / vtable (see MIGRATION.md):
//   - FindBestFullscreenMode is virtual (vtable slot 0)
//   - two members after the vptr: a quit request flag (+4) and a minimum
//     frame time (+8) used as a frame limiter by Render3DEnvironment
//   - Create() only creates the window; device creation moved to a new
//     virtual (slot 13, provisional name Create3DEnvironment)
//   - an extra dword at +0x334 ahead of the overridable variables
//-----------------------------------------------------------------------------
#ifndef D3DAPP_H
#define D3DAPP_H

#include <windows.h>
#include <tchar.h>
#include <d3d9.h>
#include "d3denumeration.h"
#include "d3dsettings.h"

//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------
enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHEDTOREF };

#define D3DAPPERR_NODIRECT3D          0x82000001
#define D3DAPPERR_NOWINDOW            0x82000002
#define D3DAPPERR_NOCOMPATIBLEDEVICES 0x82000003
#define D3DAPPERR_NOWINDOWABLEDEVICES 0x82000004
#define D3DAPPERR_NOHARDWAREDEVICE    0x82000005
#define D3DAPPERR_HALNOTCOMPATIBLE    0x82000006
#define D3DAPPERR_NOWINDOWEDHAL       0x82000007
#define D3DAPPERR_NODESKTOPHAL        0x82000008
#define D3DAPPERR_NOHALTHISMODE       0x82000009
#define D3DAPPERR_NONZEROREFCOUNT     0x8200000a
#define D3DAPPERR_MEDIANOTFOUND       0x8200000b
#define D3DAPPERR_RESETFAILED         0x8200000c
#define D3DAPPERR_NULLREFDEVICE       0x8200000d

//-----------------------------------------------------------------------------
// Resource ids (resource.h of the framework / the game)
//-----------------------------------------------------------------------------
#define IDI_MAIN_ICON           206     // M&B icon id (SDK: 101)
#define IDR_MAIN_ACCEL          113
#define IDR_MENU                141
#define IDD_SELECTDEVICE        144
#define IDM_CHANGEDEVICE        40002
#define IDM_TOGGLEFULLSCREEN    40003
#define IDM_TOGGLESTART         40004
#define IDM_SINGLESTEP          40005
#define IDM_EXIT                40006

//-----------------------------------------------------------------------------
// Name: class CD3DApplication
// Desc: A base class for creating sample D3D9 applications. To create a simple
//       Direct3D application, simply derive this class into a class (such as
//       class CMyD3DApplication) and override the following functions, as
//       needed:
//          OneTimeSceneInit()    - To initialize app data (alloc mem, etc.)
//          InitDeviceObjects()   - To initialize the 3D scene objects
//          FrameMove()           - To animate the scene
//          Render()              - To render the scene
//          DeleteDeviceObjects() - To cleanup the 3D scene objects
//          FinalCleanup()        - To cleanup app data (for exitting the app)
//          MsgProc()             - To handle Windows messages
//       vtable 0x549c60; size 0x348
//-----------------------------------------------------------------------------
class CD3DApplication
{
    friend bool ConfirmDeviceHelper( D3DCAPS9* pCaps, VertexProcessingType vertexProcessingType,
                                     D3DFORMAT adapterFormat, D3DFORMAT backBufferFormat );   // 0x401010
protected:
    // M&B additions (names provisional)
    bool              m_bQuitRequested;     // +0x004  Run() closes the window when set
    FLOAT             m_fMinFrameTime;      // +0x008  Render3DEnvironment waits until this much time elapsed

    CD3DEnumeration   m_d3dEnumeration;     // +0x00c
    D3DSettings       m_d3dSettings;        // +0x038

    // Internal variables for the state of the app
    bool              m_bWindowed;          // +0x0a4
    bool              m_bActive;            // +0x0a5
    bool              m_bDeviceLost;        // +0x0a6
    bool              m_bMinimized;         // +0x0a7
    bool              m_bMaximized;         // +0x0a8
    bool              m_bIgnoreSizeChange;  // +0x0a9
    bool              m_bDeviceObjectsInited;   // +0x0aa
    bool              m_bDeviceObjectsRestored; // +0x0ab

    // Internal variables used for timing
    bool              m_bFrameMoving;       // +0x0ac
    bool              m_bSingleStep;        // +0x0ad

    // Internal error handling function
    HRESULT DisplayErrorMsg( HRESULT hr, DWORD dwType );           // 0x4014f0

    // Internal functions to manage and render the 3D scene
    void    BuildPresentParamsFromSettings();                      // 0x402280
    bool    FindBestWindowedMode( bool bRequireHAL, bool bRequireREF );    // 0x401a60
    virtual bool FindBestFullscreenMode( bool bRequireHAL, bool bRequireREF );  // 0x401d40 (vtable 0)
    HRESULT ChooseInitialD3DSettings();                            // 0x402100
    HRESULT Initialize3DEnvironment();                             // 0x4027c0
    HRESULT HandlePossibleSizeChange();                            // 0x402150
    HRESULT Reset3DEnvironment();                                  // 0x401290
    HRESULT ToggleFullscreen();                                    // 0x402be0
    HRESULT UserSelectNewDevice();                                 // 0x402d40
    void    Cleanup3DEnvironment();                                // 0x4026e0
    HRESULT Render3DEnvironment();                                 // 0x402430
    virtual HRESULT AdjustWindowForChange();                       // 0x4013e0 (vtable 1)
    virtual void UpdateStats();                                    // 0x402580 (vtable 2)

protected:
    // Main objects used for creating and rendering the 3D scene
    D3DPRESENT_PARAMETERS m_d3dpp;          // +0x0b0  Parameters for CreateDevice/Reset
    HWND              m_hWnd;               // +0x0e8  The main app window
    HWND              m_hWndFocus;          // +0x0ec  The D3D focus window (usually same as m_hWnd)
    HMENU             m_hMenu;              // +0x0f0  App menu bar (stored here when fullscreen)
    LPDIRECT3D9       m_pD3D;               // +0x0f4  The main D3D object
    LPDIRECT3DDEVICE9 m_pd3dDevice;         // +0x0f8  The D3D rendering device
    D3DCAPS9          m_d3dCaps;            // +0x0fc  Caps for the device
    D3DSURFACE_DESC   m_d3dsdBackBuffer;    // +0x22c  Surface desc of the backbuffer
    DWORD             m_dwCreateFlags;      // +0x24c  Indicate sw or hw vertex processing
    DWORD             m_dwWindowStyle;      // +0x250  Saved window style for mode switches
    RECT              m_rcWindowBounds;     // +0x254  Saved window bounds for mode switches
    RECT              m_rcWindowClient;     // +0x264  Saved client area size for mode switches

    // Variables for timing
    FLOAT             m_fTime;              // +0x274  Current time in seconds
    FLOAT             m_fElapsedTime;       // +0x278  Time elapsed since last frame
    FLOAT             m_fFPS;               // +0x27c  Instanteous frame rate
    TCHAR             m_strDeviceStats[90]; // +0x280  String to hold D3D device stats
    TCHAR             m_strFrameStats[90];  // +0x2da  String to hold frame stats
    DWORD             m_dw334;              // +0x334  M&B addition, zeroed by the constructor (provisional)

    // Overridable variables for the app
    TCHAR*            m_strWindowTitle;     // +0x338  Title for the app's window
    DWORD             m_dwCreationWidth;    // +0x33c  Width used to create window
    DWORD             m_dwCreationHeight;   // +0x340  Height used to create window
    bool              m_bShowCursorWhenFullscreen;  // +0x344  Whether to show cursor when fullscreen
    bool              m_bClipCursorWhenFullscreen;  // +0x345  Whether to limit cursor pos when fullscreen
    bool              m_bStartFullscreen;   // +0x346  Whether to start up the app in fullscreen mode

    // Overridable functions for the 3D scene created by the app
    virtual HRESULT ConfirmDevice(D3DCAPS9*,DWORD,D3DFORMAT,D3DFORMAT);   // 0x401a20 (vtable 3)
    virtual HRESULT OneTimeSceneInit()                         { return S_OK; }   // vtable 4
    virtual HRESULT InitDeviceObjects()                        { return S_OK; }   // vtable 5
    virtual HRESULT RestoreDeviceObjects()                     { return S_OK; }   // vtable 6
    virtual HRESULT FrameMove()                                { return S_OK; }   // vtable 7
    virtual HRESULT Render()                                   { return S_OK; }   // vtable 8
    virtual HRESULT InvalidateDeviceObjects()                  { return S_OK; }   // vtable 9
    virtual HRESULT DeleteDeviceObjects()                      { return S_OK; }   // vtable 10
    virtual HRESULT FinalCleanup()                             { return S_OK; }   // vtable 11
    // (slots 4-11 are all the ICF-folded `return S_OK` at 0x408020)

public:
    // Functions to create, run, pause, and clean up the application
    virtual HRESULT Create( HINSTANCE hInstance );                 // 0x401070 (vtable 12)
    virtual HRESULT Create3DEnvironment( int );                    // 0x403010 (vtable 13, M&B; provisional)
    virtual INT     Run();                                         // 0x402ee0 (vtable 14)
    virtual LRESULT MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );  // 0x403120 (vtable 15)
    virtual void    Pause( bool bPause );                          // 0x401480 (vtable 16)
    virtual         ~CD3DApplication();                            // 0x401a30 (vtable 17, deleting)

    // Internal constructor
    CD3DApplication();                                             // 0x401910
};

#endif
