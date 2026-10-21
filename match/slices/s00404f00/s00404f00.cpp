// Slice s00404f00: CD3DFont::DrawText plus the small settings-dialog helpers (combo/list box message wrappers)
// and the multisample-type name table.
#include "../s00404140/d3dfont.h"

// ---- CD3DFont::DrawText (d3dfont.cpp of the DX9 samples)
#define D3DFONT_CENTERED_X  0x0001      // note: these are DrawText flags, not the font-creation flags above
#define D3DFONT_CENTERED_Y  0x0002
#define D3DFONT_FILTERED_DT 0x0008
#define MAX_NUM_VERTICES    (50 * 6)

struct D3DXVECTOR4 {
    float x, y, z, w;
    D3DXVECTOR4() {}
    D3DXVECTOR4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
};
struct FONT2DVERTEX { D3DXVECTOR4 p; DWORD color; float tu, tv; };
#define D3DFVF_FONT2DVERTEX 0x144       // XYZRHW | DIFFUSE | TEX1

inline FONT2DVERTEX InitFont2DVertex(const D3DXVECTOR4& p, DWORD color, float tu, float tv)
{
    FONT2DVERTEX v;
    v.p = p;
    v.color = color;
    v.tu = tu;
    v.tv = tv;
    return v;
}

// @ 0x404f00
HRESULT CD3DFont::DrawText(float sx, float sy, DWORD dwColor, const char* strText, DWORD dwFlags)
{
    if (m_pd3dDevice == NULL)
        return E_FAIL;

    // Setup renderstate
    m_pStateBlockSaved->Capture();
    m_pStateBlockDrawText->Apply();
    m_pd3dDevice->SetFVF(D3DFVF_FONT2DVERTEX);
    m_pd3dDevice->SetPixelShader(NULL);
    m_pd3dDevice->SetStreamSource(0, m_pVB, 0, sizeof(FONT2DVERTEX));

    // Set filter states
    if (dwFlags & D3DFONT_FILTERED_DT) {
        m_pd3dDevice->SetSamplerState(0, 6 /*MINFILTER*/, 2 /*LINEAR*/);
        m_pd3dDevice->SetSamplerState(0, 5 /*MAGFILTER*/, 2 /*LINEAR*/);
    }

    // Center the text block in the viewport
    if (dwFlags & D3DFONT_CENTERED_X) {
        D3DVIEWPORT9 vp;
        m_pd3dDevice->GetViewport(&vp);
        const char* strTextTmp = strText;
        float xFinal = 0.0f;

        while (*strTextTmp) {
            char c = *strTextTmp++;
            if (c == '\n')
                break;
            if ((c - 32) < 0 || (c - 32) >= 128 - 32)
                continue;
            float tx1 = m_fTexCoords[c - 32][0];
            float tx2 = m_fTexCoords[c - 32][2];
            xFinal += ((tx2 - tx1) / m_fTextScale) * m_dwTexWidth - (2 * m_dwSpacing);
        }
        sx = (vp.Width - xFinal) * 0.5f;
    }
    if (dwFlags & D3DFONT_CENTERED_Y) {
        D3DVIEWPORT9 vp;
        m_pd3dDevice->GetViewport(&vp);
        float fLineHeight = (m_fTexCoords[0][3] - m_fTexCoords[0][1]);
        sy = (vp.Height - fLineHeight * m_dwTexHeight) * 0.5f;
    }

    // Adjust for character spacing
    sx -= m_dwSpacing;
    float fStartX = sx;

    // Fill vertex buffer
    FONT2DVERTEX* pVertices = NULL;
    DWORD dwNumTriangles = 0;
    m_pVB->Lock(0, 0, (void**)&pVertices, 0x2000 /*D3DLOCK_DISCARD*/);

    while (*strText) {
        char c = *strText++;

        if (c == '\n') {
            sx = fStartX;
            sy += (m_fTexCoords[0][3] - m_fTexCoords[0][1]) * m_dwTexHeight;
        }

        if ((c - 32) < 0 || (c - 32) >= 128 - 32)
            continue;

        float tx1 = m_fTexCoords[c - 32][0];
        float ty1 = m_fTexCoords[c - 32][1];
        float tx2 = m_fTexCoords[c - 32][2];
        float ty2 = m_fTexCoords[c - 32][3];

        float fInvScale = 1.0f / m_fTextScale;
        float w = m_dwTexWidth * (tx2 - tx1) * fInvScale;
        float h = m_dwTexHeight * (ty2 - ty1) * fInvScale;

        if (c != ' ') {
            *pVertices++ = InitFont2DVertex(D3DXVECTOR4(sx + 0 - 0.5f, sy + h - 0.5f, 0.9f, 1.0f), dwColor, tx1, ty2);
            *pVertices++ = InitFont2DVertex(D3DXVECTOR4(sx + 0 - 0.5f, sy + 0 - 0.5f, 0.9f, 1.0f), dwColor, tx1, ty1);
            *pVertices++ = InitFont2DVertex(D3DXVECTOR4(sx + w - 0.5f, sy + h - 0.5f, 0.9f, 1.0f), dwColor, tx2, ty2);
            *pVertices++ = InitFont2DVertex(D3DXVECTOR4(sx + w - 0.5f, sy + 0 - 0.5f, 0.9f, 1.0f), dwColor, tx2, ty1);
            *pVertices++ = InitFont2DVertex(D3DXVECTOR4(sx + w - 0.5f, sy + h - 0.5f, 0.9f, 1.0f), dwColor, tx2, ty2);
            *pVertices++ = InitFont2DVertex(D3DXVECTOR4(sx + 0 - 0.5f, sy + 0 - 0.5f, 0.9f, 1.0f), dwColor, tx1, ty1);
            dwNumTriangles += 2;

            if (dwNumTriangles * 3 > (MAX_NUM_VERTICES - 6)) {
                // Unlock, render, and relock the vertex buffer
                m_pVB->Unlock();
                m_pd3dDevice->DrawPrimitive(4 /*D3DPT_TRIANGLELIST*/, 0, dwNumTriangles);
                pVertices = NULL;
                m_pVB->Lock(0, 0, (void**)&pVertices, 0x2000);
                dwNumTriangles = 0;
            }
        }

        sx += w - (2 * m_dwSpacing);
    }

    // Unlock and render the vertex buffer
    m_pVB->Unlock();
    if (dwNumTriangles > 0)
        m_pd3dDevice->DrawPrimitive(4 /*D3DPT_TRIANGLELIST*/, 0, dwNumTriangles);

    // Restore the modified renderstates
    m_pStateBlockSaved->Apply();
    return S_OK;
}

// ---- D3DMULTISAMPLE_TYPE -> name (d3dsettings.cpp / d3dapp.cpp of the samples)
// @ 0x4055e0
const char* MultisampleTypeToString(int multiSampleType)
{
    switch (multiSampleType) {
    case 0:  return "D3DMULTISAMPLE_NONE";
    case 1:  return "D3DMULTISAMPLE_NONMASKABLE";
    case 2:  return "D3DMULTISAMPLE_2_SAMPLES";
    case 3:  return "D3DMULTISAMPLE_3_SAMPLES";
    case 4:  return "D3DMULTISAMPLE_4_SAMPLES";
    case 5:  return "D3DMULTISAMPLE_5_SAMPLES";
    case 6:  return "D3DMULTISAMPLE_6_SAMPLES";
    case 7:  return "D3DMULTISAMPLE_7_SAMPLES";
    case 8:  return "D3DMULTISAMPLE_8_SAMPLES";
    case 9:  return "D3DMULTISAMPLE_9_SAMPLES";
    case 10: return "D3DMULTISAMPLE_10_SAMPLES";
    case 11: return "D3DMULTISAMPLE_11_SAMPLES";
    case 12: return "D3DMULTISAMPLE_12_SAMPLES";
    case 13: return "D3DMULTISAMPLE_13_SAMPLES";
    case 14: return "D3DMULTISAMPLE_14_SAMPLES";
    case 15: return "D3DMULTISAMPLE_15_SAMPLES";
    case 16: return "D3DMULTISAMPLE_16_SAMPLES";
    default: return "Unknown Multisample Type";
    }
}

// ---- Direct3D settings dialog helpers (CD3DSettingsDialog of the sample's d3dsettings dialog; provisional names)
#define CB_GETCOUNT    0x146
#define CB_GETCURSEL   0x147
#define CB_GETLBTEXT   0x148
#define CB_GETLBTEXTLEN 0x149
#define CB_GETITEMDATA 0x150
#define CB_SETITEMDATA 0x151
#define CB_ADDSTRING   0x143
#define CB_SETCURSEL   0x14E
#define WM_COMMAND     0x111
#define CBN_SELCHANGE  1
#define MAKEWPARAM_(l, h) ((WPARAM)(((WORD)(l)) | ((DWORD)((WORD)(h))) << 16))

struct D3DSettingsBlock { DWORD d[27]; };               // 0x6c bytes, copied by value into the dialog

class CSettingsDialog;
extern CSettingsDialog* g_pSettingsDialog;              // 0x5d6f54

class CSettingsDialog {
public:
    HWND m_hDlg;                    // +0x00
    int m_unk4;                     // +0x04 (provisional: owner / parent app)
    bool m_bWindowed;               // +0x08 (first byte of the settings block)
    char m_pad9[0x58 - 9];
    void* m_pRefreshRateData;       // +0x58 (provisional: item data of the selected refresh-rate combo entry)
    char m_pad5c[0x74 - 0x5c];

    CSettingsDialog(int a, const D3DSettingsBlock* settings);                  // 0x4056a0
    void ComboBoxAdd(int id, void* pData, const char* strDesc);                // 0x4056d0
    void ComboBoxSelect(int id, int index);                                    // 0x405710
    void* ComboBoxSelected(int id);                                            // 0x405760
    bool ComboBoxContainsText(int id, const char* strText);                    // 0x4057a0
    void UpdateRefreshRate();                                                  // 0x405840
};

// @ 0x4056a0
CSettingsDialog::CSettingsDialog(int a, const D3DSettingsBlock* settings)
{
    g_pSettingsDialog = this;
    m_unk4 = a;
    *(D3DSettingsBlock*)&m_bWindowed = *settings;
}

// @ 0x4056d0
void CSettingsDialog::ComboBoxAdd(int id, void* pData, const char* strDesc)
{
    HWND hwndCtrl = GetDlgItem(m_hDlg, id);
    WPARAM index = SendMessageA(hwndCtrl, CB_ADDSTRING, 0, (LPARAM)strDesc);
    SendMessageA(hwndCtrl, CB_SETITEMDATA, index, (LPARAM)pData);
}

// @ 0x405710
void CSettingsDialog::ComboBoxSelect(int id, int index)
{
    HWND hwndCtrl = GetDlgItem(m_hDlg, id);
    SendMessageA(hwndCtrl, CB_SETCURSEL, index, 0);
    PostMessageA(m_hDlg, WM_COMMAND, MAKEWPARAM_(id, CBN_SELCHANGE), (LPARAM)hwndCtrl);
}

// @ 0x405760
void* CSettingsDialog::ComboBoxSelected(int id)
{
    HWND hwndCtrl = GetDlgItem(m_hDlg, id);
    int index = (int)SendMessageA(hwndCtrl, CB_GETCURSEL, 0, 0);
    if (index < 0)
        return NULL;
    return (void*)SendMessageA(hwndCtrl, CB_GETITEMDATA, index, 0);
}

// @ 0x4057a0
bool CSettingsDialog::ComboBoxContainsText(int id, const char* strText)
{
    HWND hwndCtrl = GetDlgItem(m_hDlg, id);
    HWND hwndCombo = GetDlgItem(m_hDlg, id);
    UINT count = (UINT)SendMessageA(hwndCombo, CB_GETCOUNT, 0, 0);
    for (UINT i = 0; i < count; i++) {
        char strItem[200];
        if (SendMessageA(hwndCtrl, CB_GETLBTEXTLEN, i, 0) < 200) {
            SendMessageA(hwndCtrl, CB_GETLBTEXT, i, (LPARAM)strItem);
            if (lstrcmpA(strItem, strText) == 0)
                return true;
        }
    }
    return false;
}

// @ 0x405840
void CSettingsDialog::UpdateRefreshRate()
{
    if (!m_bWindowed) {
        m_pRefreshRateData = ComboBoxSelected(0x3ee /*IDC_REFRESHRATE_COMBO*/);
    }
}
