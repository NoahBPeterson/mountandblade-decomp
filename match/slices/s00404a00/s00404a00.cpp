// Slice s00404a00: CD3DFont::PaintAlphabet and CD3DFont::InitDeviceObjects (d3dfont.cpp of the DX9 samples).
#include "../s00404140/d3dfont.h"
#include <math.h>
#include <string.h>

// @ 0x404a00
HRESULT CD3DFont::PaintAlphabet(HDC hDC, BOOL bMeasureOnly)
{
    SIZE size;
    char str[2] = "x";   // Calculate the spacing between characters based on line height
    if (0 == GetTextExtentPoint32A(hDC, str, 1, &size))
        return E_FAIL;
    m_dwSpacing = (DWORD)ceil(size.cy * 0.3f);

    DWORD x = m_dwSpacing;
    DWORD y = 0;
    char c;
    for (c = 32; c < 127; c++) {
        str[0] = c;
        if (0 == GetTextExtentPoint32A(hDC, str, 1, &size))
            return E_FAIL;

        if ((DWORD)(x + size.cx + m_dwSpacing) > m_dwTexWidth) {
            x = m_dwSpacing;
            y += size.cy + 1;
        }

        // Check to see if there's room to write the character here
        if (y + size.cy > m_dwTexHeight)
            return D3DERR_MOREDATA;

        if (!bMeasureOnly) {
            // Perform the actual text rendering
            if (0 == ExtTextOutA(hDC, x, y, 0x2 /*ETO_OPAQUE*/, NULL, str, 1, NULL))
                return E_FAIL;
            m_fTexCoords[c - 32][0] = ((float)(x + 0 - m_dwSpacing)) / m_dwTexWidth;
            m_fTexCoords[c - 32][1] = ((float)(y + 0 + 0)) / m_dwTexHeight;
            m_fTexCoords[c - 32][2] = ((float)(x + size.cx + m_dwSpacing)) / m_dwTexWidth;
            m_fTexCoords[c - 32][3] = ((float)(y + size.cy)) / m_dwTexHeight;
        }

        x += size.cx + (2 * m_dwSpacing);
    }
    return S_OK;
}

// @ 0x404c00
HRESULT CD3DFont::InitDeviceObjects(IDirect3DDevice9* pd3dDevice)
{
    HRESULT hr;
    HFONT hFont = NULL;
    HFONT hFontOld = NULL;
    HBITMAP hbmBitmap = NULL;
    HGDIOBJ hbmOld = NULL;
    DWORD* pBitmapBits;
    HDC hDC;

    // Keep a local copy of the device
    m_pd3dDevice = pd3dDevice;
    m_fTextScale = 1.0f;

    hDC = CreateCompatibleDC(NULL);
    SetMapMode(hDC, 1 /*MM_TEXT*/);
    hr = CreateGDIFont(hDC, &hFont);
    if (FAILED(hr))
        goto done;
    hFontOld = (HFONT)SelectObject(hDC, hFont);

    // Establish the texture size; grow it until the whole alphabet fits
    m_dwTexWidth = m_dwTexHeight = 128;
    hr = PaintAlphabet(hDC, TRUE);
    while (hr == D3DERR_MOREDATA) {
        m_dwTexWidth *= 2;
        m_dwTexHeight *= 2;
        hr = PaintAlphabet(hDC, TRUE);
    }
    if (FAILED(hr))
        goto done;

    // If requested texture is too big, use a smaller texture and smaller font, and scale up when rendering.
    {
        D3DCAPS9 d3dCaps;
        m_pd3dDevice->GetDeviceCaps(&d3dCaps);
        if (m_dwTexWidth > d3dCaps.MaxTextureWidth) {
            m_fTextScale = (float)d3dCaps.MaxTextureWidth / (float)m_dwTexWidth;
            m_dwTexWidth = m_dwTexHeight = d3dCaps.MaxTextureWidth;
            bool bFirstTime = true;
            do {
                // If we have already tried a font size, scale it down
                if (!bFirstTime)
                    m_fTextScale *= 0.9f;
                DeleteObject(SelectObject(hDC, hFontOld));
                hr = CreateGDIFont(hDC, &hFont);
                if (FAILED(hr))
                    goto done;
                hFontOld = (HFONT)SelectObject(hDC, hFont);
                bFirstTime = false;
            } while (PaintAlphabet(hDC, TRUE) == D3DERR_MOREDATA);
        }
    }

    // Create a new texture for the font
    hr = m_pd3dDevice->CreateTexture(m_dwTexWidth, m_dwTexHeight, 1, 0, (D3DFORMAT)26 /*A4R4G4B4*/,
                                     1 /*D3DPOOL_MANAGED*/, &m_pTexture, NULL);
    if (FAILED(hr))
        goto done;

    // Prepare to create a bitmap
    {
        BITMAPINFO bmi;
        memset(&bmi.bmiHeader, 0, sizeof(BITMAPINFOHEADER));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = (int)m_dwTexWidth;
        bmi.bmiHeader.biHeight = -(int)m_dwTexHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biCompression = 0 /*BI_RGB*/;
        bmi.bmiHeader.biBitCount = 32;

        // Create a bitmap for the font
        hbmBitmap = CreateDIBSection(hDC, &bmi, 0 /*DIB_RGB_COLORS*/, (void**)&pBitmapBits, NULL, 0);
        hbmOld = SelectObject(hDC, hbmBitmap);
    }

    // Set text properties
    SetTextColor(hDC, 0x00FFFFFF);
    SetBkColor(hDC, 0x00000000);
    SetTextAlign(hDC, 0 /*TA_TOP*/);

    // Paint the alphabet onto the selected bitmap
    hr = PaintAlphabet(hDC, FALSE);
    if (FAILED(hr))
        goto done;

    // Lock the surface and write the alpha values for the set pixels
    {
        D3DLOCKED_RECT d3dlr;
        m_pTexture->LockRect(0, &d3dlr, 0, 0);
        BYTE* pDstRow = (BYTE*)d3dlr.pBits;
        WORD* pDst16;
        BYTE bAlpha;
        for (DWORD y = 0; y < m_dwTexHeight; y++) {
            pDst16 = (WORD*)pDstRow;
            for (DWORD x = 0; x < m_dwTexWidth; x++) {
                bAlpha = (BYTE)((pBitmapBits[m_dwTexWidth * y + x] & 0xff) >> 4);
                if (bAlpha > 0)
                    *pDst16++ = (WORD)((bAlpha << 12) | 0x0fff);
                else
                    *pDst16++ = 0x0000;
            }
            pDstRow += d3dlr.Pitch;
        }
    }
    hr = S_OK;

done:
    // Done updating texture, so clean up used objects
    if (m_pTexture)
        m_pTexture->UnlockRect(0);
    SelectObject(hDC, hbmOld);
    SelectObject(hDC, hFontOld);
    DeleteObject(hbmBitmap);
    DeleteObject(hFont);
    DeleteDC(hDC);
    return hr;
}
