#ifndef MB_D3DFONT_H
#define MB_D3DFONT_H
#include "d3dmin.h"

// ---- CD3DFont (d3dfont.cpp of the DX9 SDK samples)
#define D3DFONT_BOLD        0x0001
#define D3DFONT_ITALIC      0x0002
#define D3DFONT_ZENABLE     0x0004
#define D3DFONT_FILTERED    0x0008      // used by DrawText (0x404f00)

class CD3DFont {
public:
    char m_strFontName[80];                    // +0x00
    DWORD m_dwFontHeight;                      // +0x50
    DWORD m_dwFontFlags;                       // +0x54
    IDirect3DDevice9* m_pd3dDevice;            // +0x58
    IDirect3DTexture9* m_pTexture;             // +0x5c
    IDirect3DVertexBuffer9* m_pVB;             // +0x60
    DWORD m_dwTexWidth;                        // +0x64
    DWORD m_dwTexHeight;                       // +0x68
    float m_fTextScale;                        // +0x6c
    float m_fTexCoords[128 - 32][4];           // +0x70
    DWORD m_dwSpacing;                         // +0x670
    IDirect3DStateBlock9* m_pStateBlockSaved;  // +0x674
    IDirect3DStateBlock9* m_pStateBlockDrawText; // +0x678

    CD3DFont(const char* strFontName, DWORD dwHeight, DWORD dwFlags);      // 0x404980
    ~CD3DFont();                                                           // 0x4049d0
    HRESULT CreateGDIFont(HDC hDC, HFONT* pFont);                          // 0x404500
    HRESULT PaintAlphabet(HDC hDC, BOOL bMeasureOnly = FALSE);             // 0x404a00
    HRESULT InitDeviceObjects(IDirect3DDevice9* pd3dDevice);               // 0x404c00
    HRESULT RestoreDeviceObjects();                                        // 0x4045b0
    HRESULT InvalidateDeviceObjects();                                     // 0x404900
    HRESULT DeleteDeviceObjects();                                         // 0x404950
    HRESULT DrawText(float sx, float sy, DWORD dwColor, const char* strText, DWORD dwFlags = 0); // 0x404f00
};

#endif
