// Slice s00404140: Direct3D9 adapter enumeration (CD3DEnumeration) and CD3DFont basics.
// This is the DirectX 9 SDK sample framework (d3denum.cpp / d3dfont.cpp), lightly modified by TaleWorlds.
#include "d3dmin.h"
#include <stdlib.h>
#include <mbstring.h>
#include <math.h>

// ---- CArrayList (d3denum.h): AL_VALUE copies entries of m_BytesPerEntry, AL_REFERENCE stores pointers.
enum ArrayListType { AL_VALUE, AL_REFERENCE };
class CArrayList {
public:
    CArrayList(ArrayListType type, unsigned bytesPerEntry = 0);   // 0x407a70
    ~CArrayList();                                                // 0x407aa0
    HRESULT Add(void* pEntry);                                    // 0x407ab0
    void* Get(unsigned index);                                    // 0x407ba0
    bool Contains(void* pEntryData);                              // 0x407bc0
    unsigned Count() { return m_NumEntries; }
    int m_ArrayListType;
    void* m_pData;
    unsigned m_BytesPerEntry;
    unsigned m_NumEntries;
    unsigned m_NumEntriesAllocated;
};

struct D3DAdapterInfo {
    unsigned AdapterOrdinal;
    D3DADAPTER_IDENTIFIER9 AdapterIdentifier;
    CArrayList* pDisplayModeList;
    CArrayList* pDeviceInfoList;
    ~D3DAdapterInfo();                                            // 0x4040c0
};

// 0x4035d0 (takes the format in EAX: a static function the optimizer gave a register convention).
static __declspec(noinline) int GetColorChannelBits(D3DFORMAT fmt)
{
    switch (fmt) {
    case 20: case 21: case 22: return 8;    // R8G8B8, A8R8G8B8, X8R8G8B8
    case 23: case 24: case 25: return 5;    // R5G6B5, X1R5G5B5, A1R5G5B5
    case 26: case 30: return 4;             // A4R4G4B4, X4R4G4B4
    case 27: case 29: return 2;             // R3G3B2, A8R3G3B2
    case 31: case 35: return 10;            // A2B10G10R10, A2R10G10B10
    }
    return 0;
}
int __cdecl SortModesCallback(const void* a, const void* b);     // 0x403720

class CD3DEnumeration {
public:
    IDirect3D9* m_pD3D;                      // +0x00
    CArrayList* m_pAdapterInfoList;          // +0x04
    unsigned pad8;
    unsigned AppMinFullscreenWidth;          // +0x0c
    unsigned AppMinFullscreenHeight;         // +0x10
    unsigned AppMinColorChannelBits;         // +0x14
    char pad18[0x10];
    CArrayList* m_pAllowedAdapterFormatList; // +0x28
    HRESULT EnumerateDevices(D3DAdapterInfo* pAdapterInfo, CArrayList* pAdapterFormatList); // 0x403f90
    HRESULT Enumerate();                     // 0x404140
    ~CD3DEnumeration();                      // 0x404480
};

// @ 0x404140
HRESULT CD3DEnumeration::Enumerate()
{
    CArrayList adapterFormatList(AL_VALUE, sizeof(D3DFORMAT));
    if (m_pD3D == NULL)
        return E_FAIL;

    m_pAdapterInfoList = new CArrayList(AL_REFERENCE);
    if (m_pAdapterInfoList == NULL)
        return E_OUTOFMEMORY;
    m_pAllowedAdapterFormatList = new CArrayList(AL_VALUE, sizeof(D3DFORMAT));
    if (m_pAllowedAdapterFormatList == NULL)
        return E_OUTOFMEMORY;

    HRESULT hr;
    D3DFORMAT fmt;
    fmt = D3DFMT_X8R8G8B8;
    hr = m_pAllowedAdapterFormatList->Add(&fmt);
    if (FAILED(hr)) return hr;
    fmt = D3DFMT_X1R5G5B5;
    hr = m_pAllowedAdapterFormatList->Add(&fmt);
    if (FAILED(hr)) return hr;
    fmt = D3DFMT_R5G6B5;
    hr = m_pAllowedAdapterFormatList->Add(&fmt);
    if (FAILED(hr)) return hr;
    fmt = D3DFMT_A2R10G10B10;
    hr = m_pAllowedAdapterFormatList->Add(&fmt);
    if (FAILED(hr)) return hr;

    UINT numAdapters = m_pD3D->GetAdapterCount();
    for (UINT adapter = 0; adapter < numAdapters; adapter++) {
        D3DAdapterInfo* pAdapterInfo = new D3DAdapterInfo;
        if (pAdapterInfo == NULL)
            return E_OUTOFMEMORY;
        pAdapterInfo->pDisplayModeList = new CArrayList(AL_VALUE, sizeof(D3DDISPLAYMODE));
        pAdapterInfo->pDeviceInfoList = new CArrayList(AL_REFERENCE);
        if (pAdapterInfo->pDisplayModeList == NULL || pAdapterInfo->pDeviceInfoList == NULL) {
            delete pAdapterInfo;
            return E_OUTOFMEMORY;
        }
        pAdapterInfo->AdapterOrdinal = adapter;
        m_pD3D->GetAdapterIdentifier(adapter, 0, &pAdapterInfo->AdapterIdentifier);

        adapterFormatList.m_NumEntries = 0;
        for (UINT iFmt = 0; iFmt < m_pAllowedAdapterFormatList->Count(); iFmt++) {
            D3DFORMAT format = *(D3DFORMAT*)m_pAllowedAdapterFormatList->Get(iFmt);
            UINT numModes = m_pD3D->GetAdapterModeCount(adapter, format);
            for (UINT mode = 0; mode < numModes; mode++) {
                D3DDISPLAYMODE displayMode;
                m_pD3D->EnumAdapterModes(adapter, format, mode, &displayMode);
                if (displayMode.Width < AppMinFullscreenWidth ||
                    displayMode.Height < AppMinFullscreenHeight ||
                    (UINT)GetColorChannelBits(displayMode.Format) < AppMinColorChannelBits)
                    continue;
                pAdapterInfo->pDisplayModeList->Add(&displayMode);
                if (!adapterFormatList.Contains(&displayMode.Format))
                    adapterFormatList.Add(&displayMode.Format);
            }
        }

        qsort(pAdapterInfo->pDisplayModeList->Get(0), pAdapterInfo->pDisplayModeList->Count(),
              sizeof(D3DDISPLAYMODE), SortModesCallback);

        hr = EnumerateDevices(pAdapterInfo, &adapterFormatList);
        if (FAILED(hr)) {
            delete pAdapterInfo;
            return hr;
        }
        if (pAdapterInfo->pDeviceInfoList->Count() == 0)
            delete pAdapterInfo;
        else
            m_pAdapterInfoList->Add(pAdapterInfo);
    }
    return S_OK;
}

// @ 0x404480
CD3DEnumeration::~CD3DEnumeration()
{
    if (m_pAdapterInfoList) {
        for (UINT i = 0; i < m_pAdapterInfoList->Count(); i++) {
            D3DAdapterInfo* pAdapterInfo = (D3DAdapterInfo*)m_pAdapterInfoList->Get(i);
            delete pAdapterInfo;
        }
        delete m_pAdapterInfoList;
    }
    if (m_pAllowedAdapterFormatList) {
        delete m_pAllowedAdapterFormatList;
        m_pAllowedAdapterFormatList = NULL;
    }
}

#include "d3dfont.h"

// @ 0x404500
HRESULT CD3DFont::CreateGDIFont(HDC hDC, HFONT* pFont)
{
    int nHeight = -MulDiv(m_dwFontHeight, (int)(GetDeviceCaps(hDC, 90 /*LOGPIXELSY*/) * m_fTextScale), 72);
    DWORD dwBold = (m_dwFontFlags & D3DFONT_BOLD) ? 700 : 400;
    DWORD dwItalic = (m_dwFontFlags & D3DFONT_ITALIC) ? TRUE : FALSE;
    *pFont = CreateFontA(nHeight, 0, 0, 0, dwBold, dwItalic, FALSE, FALSE, 1 /*DEFAULT_CHARSET*/,
                         0 /*OUT_DEFAULT_PRECIS*/, 0 /*CLIP_DEFAULT_PRECIS*/, 4 /*ANTIALIASED_QUALITY*/,
                         2 /*VARIABLE_PITCH*/, m_strFontName);
    if (*pFont == NULL)
        return E_FAIL;
    return S_OK;
}

// @ 0x404590
double __cdecl CeilF(float x) { return ceil(x); }

// @ 0x4045b0
HRESULT CD3DFont::RestoreDeviceObjects()
{
    HRESULT hr;
    if (FAILED(hr = m_pd3dDevice->CreateVertexBuffer(9600, 0x208 /*WRITEONLY|DYNAMIC*/, 0, 0 /*D3DPOOL_DEFAULT*/,
                                                     &m_pVB, NULL)))
        return hr;

    bool bSupportsAlphaBlend = true;
    IDirect3D9* pd3d = NULL;
    if (SUCCEEDED(m_pd3dDevice->GetDirect3D(&pd3d))) {
        D3DCAPS9 Caps;
        D3DDISPLAYMODE Mode;
        IDirect3DSurface9* pSurf = NULL;
        D3DSURFACE_DESC Desc;
        m_pd3dDevice->GetDeviceCaps(&Caps);
        m_pd3dDevice->GetDisplayMode(0, &Mode);
        if (SUCCEEDED(m_pd3dDevice->GetRenderTarget(0, &pSurf))) {
            pSurf->GetDesc(&Desc);
            if (FAILED(pd3d->CheckDeviceFormat(Caps.AdapterOrdinal, Caps.DeviceType, Mode.Format,
                                               0x80001 /*RENDERTARGET|QUERY_POSTPIXELSHADER_BLENDING*/,
                                               1 /*D3DRTYPE_SURFACE*/, Desc.Format)))
                bSupportsAlphaBlend = false;
            if (pSurf) { pSurf->Release(); pSurf = NULL; }
        }
        if (pd3d) { pd3d->Release(); pd3d = NULL; }
    }

    for (UINT which = 0; which < 2; which++) {
        m_pd3dDevice->BeginStateBlock();
        m_pd3dDevice->SetTexture(0, m_pTexture);
        if (m_dwFontFlags & D3DFONT_ZENABLE)
            m_pd3dDevice->SetRenderState(7 /*ZENABLE*/, TRUE);
        else
            m_pd3dDevice->SetRenderState(7, FALSE);
        if (bSupportsAlphaBlend) {
            m_pd3dDevice->SetRenderState(27 /*ALPHABLENDENABLE*/, TRUE);
            m_pd3dDevice->SetRenderState(19 /*SRCBLEND*/, 5 /*SRCALPHA*/);
            m_pd3dDevice->SetRenderState(20 /*DESTBLEND*/, 6 /*INVSRCALPHA*/);
        } else {
            m_pd3dDevice->SetRenderState(27, FALSE);
        }
        m_pd3dDevice->SetRenderState(15 /*ALPHATESTENABLE*/, TRUE);
        m_pd3dDevice->SetRenderState(24 /*ALPHAREF*/, 0x08);
        m_pd3dDevice->SetRenderState(25 /*ALPHAFUNC*/, 7 /*GREATEREQUAL*/);
        m_pd3dDevice->SetRenderState(8 /*FILLMODE*/, 3 /*SOLID*/);
        m_pd3dDevice->SetRenderState(22 /*CULLMODE*/, 3 /*CCW*/);
        m_pd3dDevice->SetRenderState(52 /*STENCILENABLE*/, FALSE);
        m_pd3dDevice->SetRenderState(136 /*CLIPPING*/, TRUE);
        m_pd3dDevice->SetRenderState(152 /*EDGEANTIALIAS*/, FALSE);
        m_pd3dDevice->SetRenderState(151 /*VERTEXBLEND*/, FALSE);
        m_pd3dDevice->SetRenderState(167 /*INDEXEDVERTEXBLENDENABLE*/, FALSE);
        m_pd3dDevice->SetRenderState(28 /*FOGENABLE*/, FALSE);
        m_pd3dDevice->SetRenderState(168 /*COLORWRITEENABLE*/,
                                     0x1 | 0x2 | 0x4 | 0x8);
        m_pd3dDevice->SetTextureStageState(0, 1 /*COLOROP*/, 4 /*MODULATE*/);
        m_pd3dDevice->SetTextureStageState(0, 2 /*COLORARG1*/, 2 /*TEXTURE*/);
        m_pd3dDevice->SetTextureStageState(0, 3 /*COLORARG2*/, 0 /*DIFFUSE*/);
        m_pd3dDevice->SetTextureStageState(0, 4 /*ALPHAOP*/, 4 /*MODULATE*/);
        m_pd3dDevice->SetTextureStageState(0, 5 /*ALPHAARG1*/, 2 /*TEXTURE*/);
        m_pd3dDevice->SetTextureStageState(0, 6 /*ALPHAARG2*/, 0 /*DIFFUSE*/);
        m_pd3dDevice->SetTextureStageState(0, 11 /*TEXCOORDINDEX*/, 0);
        m_pd3dDevice->SetTextureStageState(0, 24 /*TEXTURETRANSFORMFLAGS*/, 0);
        m_pd3dDevice->SetTextureStageState(1, 1, 1 /*DISABLE*/);
        m_pd3dDevice->SetTextureStageState(1, 4, 1 /*DISABLE*/);
        m_pd3dDevice->SetSamplerState(0, 6 /*MINFILTER*/, 1 /*POINT*/);
        m_pd3dDevice->SetSamplerState(0, 5 /*MAGFILTER*/, 1 /*POINT*/);
        m_pd3dDevice->SetSamplerState(0, 7 /*MIPFILTER*/, 0 /*NONE*/);
        if (which == 0)
            m_pd3dDevice->EndStateBlock(&m_pStateBlockSaved);
        else
            m_pd3dDevice->EndStateBlock(&m_pStateBlockDrawText);
    }
    return S_OK;
}

// @ 0x404900
HRESULT CD3DFont::InvalidateDeviceObjects()
{
    if (m_pVB) { m_pVB->Release(); m_pVB = NULL; }
    if (m_pStateBlockSaved) { m_pStateBlockSaved->Release(); m_pStateBlockSaved = NULL; }
    if (m_pStateBlockDrawText) { m_pStateBlockDrawText->Release(); m_pStateBlockDrawText = NULL; }
    return S_OK;
}

// @ 0x404950
HRESULT CD3DFont::DeleteDeviceObjects()
{
    if (m_pTexture) { m_pTexture->Release(); m_pTexture = NULL; }
    m_pd3dDevice = NULL;
    return S_OK;
}

// @ 0x404980
CD3DFont::CD3DFont(const char* strFontName, DWORD dwHeight, DWORD dwFlags)
{
    _mbsnbcpy((unsigned char*)m_strFontName, (const unsigned char*)strFontName, sizeof(m_strFontName));
    m_strFontName[sizeof(m_strFontName) - 1] = '\0';
    m_dwSpacing = 0;
    m_pd3dDevice = NULL;
    m_pTexture = NULL;
    m_pVB = NULL;
    m_pStateBlockSaved = NULL;
    m_pStateBlockDrawText = NULL;
    m_dwFontHeight = dwHeight;
    m_dwFontFlags = dwFlags;
}

// @ 0x4049d0
CD3DFont::~CD3DFont()
{
    InvalidateDeviceObjects();
    DeleteDeviceObjects();
}
