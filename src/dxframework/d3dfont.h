//-----------------------------------------------------------------------------
// File: D3DFont.h
//
// Desc: Texture-based font class (DirectX 9 SDK sample framework, as linked
//       into Mount & Blade 1.011: the 3D-text and scaled-text methods are not
//       linked).
//-----------------------------------------------------------------------------
#ifndef D3DFONT_H
#define D3DFONT_H
#include <windows.h>
#include <tchar.h>
#include <d3d9.h>

// Font creation flags
#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define D3DFONT_ZENABLE     0x0004

// Font rendering flags
#define D3DFONT_CENTERED_X  0x0001
#define D3DFONT_CENTERED_Y  0x0002
#define D3DFONT_TWOSIDED    0x0004
#define D3DFONT_FILTERED    0x0008

//-----------------------------------------------------------------------------
// Name: class CD3DFont
// Desc: Texture-based font class for doing text in a 3D scene. (0x67c bytes)
//-----------------------------------------------------------------------------
class CD3DFont
{
    TCHAR   m_strFontName[80];            // +0x000  Font properties
    DWORD   m_dwFontHeight;               // +0x050
    DWORD   m_dwFontFlags;                // +0x054

    LPDIRECT3DDEVICE9       m_pd3dDevice; // +0x058  A D3DDevice used for rendering
    LPDIRECT3DTEXTURE9      m_pTexture;   // +0x05c  The d3d texture for this font
    LPDIRECT3DVERTEXBUFFER9 m_pVB;        // +0x060  VertexBuffer for rendering text
    DWORD   m_dwTexWidth;                 // +0x064  Texture dimensions
    DWORD   m_dwTexHeight;                // +0x068
    FLOAT   m_fTextScale;                 // +0x06c
    FLOAT   m_fTexCoords[128-32][4];      // +0x070
    DWORD   m_dwSpacing;                  // +0x670  Character pixel spacing per side

    // Stateblocks for setting and restoring render states
    LPDIRECT3DSTATEBLOCK9 m_pStateBlockSaved;       // +0x674
    LPDIRECT3DSTATEBLOCK9 m_pStateBlockDrawText;    // +0x678

    HRESULT   CreateGDIFont( HDC hDC, HFONT* pFont );                       // 0x404500
    HRESULT   PaintAlphabet( HDC hDC, BOOL bMeasureOnly=FALSE );           // 0x404a00

public:
    // 2D and 3D text drawing functions
    HRESULT DrawText( FLOAT x, FLOAT y, DWORD dwColor,
                      const TCHAR* strText, DWORD dwFlags=0L );            // 0x404f00

    // Initializing and destroying device-dependent objects
    HRESULT InitDeviceObjects( LPDIRECT3DDEVICE9 pd3dDevice );             // 0x404c00
    HRESULT RestoreDeviceObjects();                                        // 0x4045b0
    HRESULT InvalidateDeviceObjects();                                     // 0x404900
    HRESULT DeleteDeviceObjects();                                         // 0x404950

    // Constructor / destructor
    CD3DFont( const TCHAR* strFontName, DWORD dwHeight, DWORD dwFlags=0L ); // 0x404980
    ~CD3DFont();                                                           // 0x4049d0
};

#endif
