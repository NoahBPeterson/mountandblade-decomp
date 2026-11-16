// port/include/d3dx9.h: portable-build shim (ticket 004) for the D3DX 9 surface src/ uses: the math types, the effect and
// buffer interfaces, and the dozen helper functions the game imports (ticket 009 replaces them with port/ code).
#pragma once
#ifndef MB_PORT_D3DX9_H
#define MB_PORT_D3DX9_H
#include <d3d9.h>

#define D3DX_DEFAULT ((UINT)-1)
#define D3DX_DEFAULT_NONPOW2 ((UINT)-2)
#define D3DX_FROM_FILE ((UINT)-3)
#define D3DFMT_FROM_FILE ((D3DFORMAT)-3)
#define D3DX_FILTER_NONE (1 << 0)
#define D3DX_FILTER_POINT (2 << 0)
#define D3DX_FILTER_LINEAR (3 << 0)
#define D3DX_FILTER_TRIANGLE (4 << 0)
#define D3DX_FILTER_BOX (5 << 0)
#define D3DX_FILTER_DITHER (1 << 19)
#define D3DX_SKIP_DDS_MIP_LEVELS_MASK 0x1F
#define D3DX_SKIP_DDS_MIP_LEVELS_SHIFT 26
#define D3DX_SKIP_DDS_MIP_LEVELS(levels, filter) \
    ((((levels) & D3DX_SKIP_DDS_MIP_LEVELS_MASK) << D3DX_SKIP_DDS_MIP_LEVELS_SHIFT) | ((filter) == D3DX_DEFAULT ? D3DX_FILTER_BOX : (filter)))
#define D3DXSHADER_DEBUG (1 << 0)

typedef enum _D3DXIMAGE_FILEFORMAT {
    D3DXIFF_BMP = 0, D3DXIFF_JPG = 1, D3DXIFF_TGA = 2, D3DXIFF_PNG = 3, D3DXIFF_DDS = 4, D3DXIFF_PPM = 5, D3DXIFF_DIB = 6,
    D3DXIFF_HDR = 7, D3DXIFF_PFM = 8, D3DXIFF_FORCE_DWORD = 0x7fffffff
} D3DXIMAGE_FILEFORMAT;
typedef struct _D3DXIMAGE_INFO {
    UINT Width, Height, Depth, MipLevels; D3DFORMAT Format; D3DRESOURCETYPE ResourceType; D3DXIMAGE_FILEFORMAT ImageFileFormat;
} D3DXIMAGE_INFO;

// ---- math types ----
struct D3DXVECTOR2 {
    D3DXVECTOR2() {}
    D3DXVECTOR2(FLOAT fx, FLOAT fy) : x(fx), y(fy) {}
    operator FLOAT*() { return &x; }
    operator const FLOAT*() const { return &x; }
    FLOAT x, y;
};
struct D3DXVECTOR3 : public D3DVECTOR {
    D3DXVECTOR3() {}
    D3DXVECTOR3(const D3DVECTOR& v) { x = v.x; y = v.y; z = v.z; }
    D3DXVECTOR3(FLOAT fx, FLOAT fy, FLOAT fz) { x = fx; y = fy; z = fz; }
    operator FLOAT*() { return &x; }
    operator const FLOAT*() const { return &x; }
    D3DXVECTOR3 operator+(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x + v.x, y + v.y, z + v.z); }
    D3DXVECTOR3 operator-(const D3DXVECTOR3& v) const { return D3DXVECTOR3(x - v.x, y - v.y, z - v.z); }
    D3DXVECTOR3 operator*(FLOAT f) const { return D3DXVECTOR3(x * f, y * f, z * f); }
};
struct D3DXVECTOR4 {
    D3DXVECTOR4() {}
    D3DXVECTOR4(const FLOAT* p) : x(p[0]), y(p[1]), z(p[2]), w(p[3]) {}
    D3DXVECTOR4(FLOAT fx, FLOAT fy, FLOAT fz, FLOAT fw) : x(fx), y(fy), z(fz), w(fw) {}
    operator FLOAT*() { return &x; }
    operator const FLOAT*() const { return &x; }
    FLOAT x, y, z, w;
};
struct D3DXMATRIX : public D3DMATRIX {
    D3DXMATRIX() {}
    D3DXMATRIX(const FLOAT* p) { memcpy(&_11, p, sizeof(D3DMATRIX)); }
    D3DXMATRIX(const D3DMATRIX& m) { memcpy(&_11, &m, sizeof(D3DMATRIX)); }
    D3DXMATRIX(FLOAT f11, FLOAT f12, FLOAT f13, FLOAT f14, FLOAT f21, FLOAT f22, FLOAT f23, FLOAT f24,
               FLOAT f31, FLOAT f32, FLOAT f33, FLOAT f34, FLOAT f41, FLOAT f42, FLOAT f43, FLOAT f44)
    {
        _11 = f11; _12 = f12; _13 = f13; _14 = f14; _21 = f21; _22 = f22; _23 = f23; _24 = f24;
        _31 = f31; _32 = f32; _33 = f33; _34 = f34; _41 = f41; _42 = f42; _43 = f43; _44 = f44;
    }
    FLOAT& operator()(UINT row, UINT col) { return m[row][col]; }
    FLOAT operator()(UINT row, UINT col) const { return m[row][col]; }
    operator FLOAT*() { return &_11; }
    operator const FLOAT*() const { return &_11; }
    D3DXMATRIX& operator*=(const D3DXMATRIX& r);
    D3DXMATRIX operator*(const D3DXMATRIX& r) const;
};
typedef struct D3DXMATRIX* LPD3DXMATRIX;
struct D3DXPLANE {
    D3DXPLANE() {}
    D3DXPLANE(FLOAT fa, FLOAT fb, FLOAT fc, FLOAT fd) : a(fa), b(fb), c(fc), d(fd) {}
    operator FLOAT*() { return &a; }
    operator const FLOAT*() const { return &a; }
    FLOAT a, b, c, d;
};
struct D3DXQUATERNION { FLOAT x, y, z, w; };
struct D3DXCOLOR { FLOAT r, g, b, a; };

extern "C" {
D3DXMATRIX* WINAPI D3DXMatrixMultiply(D3DXMATRIX* pOut, const D3DXMATRIX* pM1, const D3DXMATRIX* pM2);
D3DXMATRIX* WINAPI D3DXMatrixTranspose(D3DXMATRIX* pOut, const D3DXMATRIX* pM);
D3DXMATRIX* WINAPI D3DXMatrixInverse(D3DXMATRIX* pOut, FLOAT* pDeterminant, const D3DXMATRIX* pM);
D3DXMATRIX* WINAPI D3DXMatrixPerspectiveOffCenterRH(D3DXMATRIX* pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf);
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterRH(D3DXMATRIX* pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf);
D3DXMATRIX* WINAPI D3DXMatrixOrthoOffCenterLH(D3DXMATRIX* pOut, FLOAT l, FLOAT r, FLOAT b, FLOAT t, FLOAT zn, FLOAT zf);
D3DXPLANE* WINAPI D3DXPlaneTransform(D3DXPLANE* pOut, const D3DXPLANE* pP, const D3DXMATRIX* pM);
}
inline D3DXMATRIX& D3DXMATRIX::operator*=(const D3DXMATRIX& r) { D3DXMatrixMultiply(this, this, &r); return *this; }
inline D3DXMATRIX D3DXMATRIX::operator*(const D3DXMATRIX& r) const { D3DXMATRIX o; D3DXMatrixMultiply(&o, this, &r); return o; }

// ---- buffer / effect ----
typedef const char* D3DXHANDLE;
typedef D3DXHANDLE* LPD3DXHANDLE;
struct ID3DXBuffer : public IUnknown {
    virtual LPVOID STDMETHODCALLTYPE GetBufferPointer() = 0;
    virtual DWORD STDMETHODCALLTYPE GetBufferSize() = 0;
};
typedef ID3DXBuffer* LPD3DXBUFFER;
struct ID3DXInclude;
typedef ID3DXInclude* LPD3DXINCLUDE;
typedef struct _D3DXMACRO { LPCSTR Name; LPCSTR Definition; } D3DXMACRO;
typedef const D3DXMACRO* LPD3DXMACRO_C;
struct ID3DXEffectPool : public IUnknown {};
typedef ID3DXEffectPool* LPD3DXEFFECTPOOL;
struct ID3DXEffectStateManager;
typedef ID3DXEffectStateManager* LPD3DXEFFECTSTATEMANAGER;
struct D3DXEFFECT_DESC; struct D3DXPARAMETER_DESC; struct D3DXTECHNIQUE_DESC; struct D3DXPASS_DESC; struct D3DXFUNCTION_DESC;
#define D3DXFX_DONOTSAVESTATE (1 << 0)
#define D3DXFX_DONOTSAVESHADERSTATE (1 << 1)
#define D3DXFX_DONOTSAVESAMPLERSTATE (1 << 2)
#define D3DXFX_NOT_CLONEABLE (1 << 11)

#define MB_FXM(ret, name) virtual ret STDMETHODCALLTYPE name
struct ID3DXBaseEffect : public IUnknown {
    MB_FXM(HRESULT, GetDesc)(D3DXEFFECT_DESC* pDesc) = 0;
    MB_FXM(HRESULT, GetParameterDesc)(D3DXHANDLE hParameter, D3DXPARAMETER_DESC* pDesc) = 0;
    MB_FXM(HRESULT, GetTechniqueDesc)(D3DXHANDLE hTechnique, D3DXTECHNIQUE_DESC* pDesc) = 0;
    MB_FXM(HRESULT, GetPassDesc)(D3DXHANDLE hPass, D3DXPASS_DESC* pDesc) = 0;
    MB_FXM(HRESULT, GetFunctionDesc)(D3DXHANDLE hShader, D3DXFUNCTION_DESC* pDesc) = 0;
    MB_FXM(D3DXHANDLE, GetParameter)(D3DXHANDLE hParameter, UINT Index) = 0;
    MB_FXM(D3DXHANDLE, GetParameterByName)(D3DXHANDLE hParameter, LPCSTR pName) = 0;
    MB_FXM(D3DXHANDLE, GetParameterBySemantic)(D3DXHANDLE hParameter, LPCSTR pSemantic) = 0;
    MB_FXM(D3DXHANDLE, GetParameterElement)(D3DXHANDLE hParameter, UINT Index) = 0;
    MB_FXM(D3DXHANDLE, GetTechnique)(UINT Index) = 0;
    MB_FXM(D3DXHANDLE, GetTechniqueByName)(LPCSTR pName) = 0;
    MB_FXM(D3DXHANDLE, GetPass)(D3DXHANDLE hTechnique, UINT Index) = 0;
    MB_FXM(D3DXHANDLE, GetPassByName)(D3DXHANDLE hTechnique, LPCSTR pName) = 0;
    MB_FXM(D3DXHANDLE, GetFunction)(UINT Index) = 0;
    MB_FXM(D3DXHANDLE, GetFunctionByName)(LPCSTR pName) = 0;
    MB_FXM(D3DXHANDLE, GetAnnotation)(D3DXHANDLE hObject, UINT Index) = 0;
    MB_FXM(D3DXHANDLE, GetAnnotationByName)(D3DXHANDLE hObject, LPCSTR pName) = 0;
    MB_FXM(HRESULT, SetValue)(D3DXHANDLE hParameter, LPCVOID pData, UINT Bytes) = 0;
    MB_FXM(HRESULT, GetValue)(D3DXHANDLE hParameter, LPVOID pData, UINT Bytes) = 0;
    MB_FXM(HRESULT, SetBool)(D3DXHANDLE hParameter, BOOL b) = 0;
    MB_FXM(HRESULT, GetBool)(D3DXHANDLE hParameter, BOOL* pb) = 0;
    MB_FXM(HRESULT, SetBoolArray)(D3DXHANDLE hParameter, CONST BOOL* pb, UINT Count) = 0;
    MB_FXM(HRESULT, GetBoolArray)(D3DXHANDLE hParameter, BOOL* pb, UINT Count) = 0;
    MB_FXM(HRESULT, SetInt)(D3DXHANDLE hParameter, INT n) = 0;
    MB_FXM(HRESULT, GetInt)(D3DXHANDLE hParameter, INT* pn) = 0;
    MB_FXM(HRESULT, SetIntArray)(D3DXHANDLE hParameter, CONST INT* pn, UINT Count) = 0;
    MB_FXM(HRESULT, GetIntArray)(D3DXHANDLE hParameter, INT* pn, UINT Count) = 0;
    MB_FXM(HRESULT, SetFloat)(D3DXHANDLE hParameter, FLOAT f) = 0;
    MB_FXM(HRESULT, GetFloat)(D3DXHANDLE hParameter, FLOAT* pf) = 0;
    MB_FXM(HRESULT, SetFloatArray)(D3DXHANDLE hParameter, CONST FLOAT* pf, UINT Count) = 0;
    MB_FXM(HRESULT, GetFloatArray)(D3DXHANDLE hParameter, FLOAT* pf, UINT Count) = 0;
    MB_FXM(HRESULT, SetVector)(D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector) = 0;
    MB_FXM(HRESULT, GetVector)(D3DXHANDLE hParameter, D3DXVECTOR4* pVector) = 0;
    MB_FXM(HRESULT, SetVectorArray)(D3DXHANDLE hParameter, CONST D3DXVECTOR4* pVector, UINT Count) = 0;
    MB_FXM(HRESULT, GetVectorArray)(D3DXHANDLE hParameter, D3DXVECTOR4* pVector, UINT Count) = 0;
    MB_FXM(HRESULT, SetMatrix)(D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) = 0;
    MB_FXM(HRESULT, GetMatrix)(D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) = 0;
    MB_FXM(HRESULT, SetMatrixArray)(D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, GetMatrixArray)(D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, SetMatrixPointerArray)(D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, GetMatrixPointerArray)(D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, SetMatrixTranspose)(D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix) = 0;
    MB_FXM(HRESULT, GetMatrixTranspose)(D3DXHANDLE hParameter, D3DXMATRIX* pMatrix) = 0;
    MB_FXM(HRESULT, SetMatrixTransposeArray)(D3DXHANDLE hParameter, CONST D3DXMATRIX* pMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, GetMatrixTransposeArray)(D3DXHANDLE hParameter, D3DXMATRIX* pMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, SetMatrixTransposePointerArray)(D3DXHANDLE hParameter, CONST D3DXMATRIX** ppMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, GetMatrixTransposePointerArray)(D3DXHANDLE hParameter, D3DXMATRIX** ppMatrix, UINT Count) = 0;
    MB_FXM(HRESULT, SetString)(D3DXHANDLE hParameter, LPCSTR pString) = 0;
    MB_FXM(HRESULT, GetString)(D3DXHANDLE hParameter, LPCSTR* ppString) = 0;
    MB_FXM(HRESULT, SetTexture)(D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9 pTexture) = 0;
    MB_FXM(HRESULT, GetTexture)(D3DXHANDLE hParameter, LPDIRECT3DBASETEXTURE9* ppTexture) = 0;
    MB_FXM(HRESULT, GetPixelShader)(D3DXHANDLE hParameter, LPDIRECT3DPIXELSHADER9* ppPShader) = 0;
    MB_FXM(HRESULT, GetVertexShader)(D3DXHANDLE hParameter, LPDIRECT3DVERTEXSHADER9* ppVShader) = 0;
    MB_FXM(HRESULT, SetArrayRange)(D3DXHANDLE hParameter, UINT uStart, UINT uEnd) = 0;
};
struct ID3DXEffect;
typedef ID3DXEffect* LPD3DXEFFECT;
struct ID3DXEffect : public ID3DXBaseEffect {
    MB_FXM(HRESULT, GetPool)(LPD3DXEFFECTPOOL* ppPool) = 0;
    MB_FXM(HRESULT, SetTechnique)(D3DXHANDLE hTechnique) = 0;
    MB_FXM(D3DXHANDLE, GetCurrentTechnique)() = 0;
    MB_FXM(HRESULT, ValidateTechnique)(D3DXHANDLE hTechnique) = 0;
    MB_FXM(HRESULT, FindNextValidTechnique)(D3DXHANDLE hTechnique, D3DXHANDLE* pTechnique) = 0;
    MB_FXM(BOOL, IsParameterUsed)(D3DXHANDLE hParameter, D3DXHANDLE hTechnique) = 0;
    MB_FXM(HRESULT, Begin)(UINT* pPasses, DWORD Flags) = 0;
    MB_FXM(HRESULT, BeginPass)(UINT Pass) = 0;
    MB_FXM(HRESULT, CommitChanges)() = 0;
    MB_FXM(HRESULT, EndPass)() = 0;
    MB_FXM(HRESULT, End)() = 0;
    MB_FXM(HRESULT, GetDevice)(LPDIRECT3DDEVICE9* ppDevice) = 0;
    MB_FXM(HRESULT, OnLostDevice)() = 0;
    MB_FXM(HRESULT, OnResetDevice)() = 0;
    MB_FXM(HRESULT, SetStateManager)(LPD3DXEFFECTSTATEMANAGER pManager) = 0;
    MB_FXM(HRESULT, GetStateManager)(LPD3DXEFFECTSTATEMANAGER* ppManager) = 0;
    MB_FXM(HRESULT, BeginParameterBlock)() = 0;
    MB_FXM(D3DXHANDLE, EndParameterBlock)() = 0;
    MB_FXM(HRESULT, ApplyParameterBlock)(D3DXHANDLE hParameterBlock) = 0;
    MB_FXM(HRESULT, DeleteParameterBlock)(D3DXHANDLE hParameterBlock) = 0;
    MB_FXM(HRESULT, CloneEffect)(LPDIRECT3DDEVICE9 pDevice, LPD3DXEFFECT* ppEffect) = 0;
    MB_FXM(HRESULT, SetRawValue)(D3DXHANDLE hParameter, LPCVOID pData, UINT ByteOffset, UINT Bytes) = 0;
};
#undef MB_FXM

// ---- the helper functions the game imports ----
extern "C" {
HRESULT WINAPI D3DXCreateTexture(LPDIRECT3DDEVICE9 pDevice, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, LPDIRECT3DTEXTURE9* ppTexture);
HRESULT WINAPI D3DXCreateTextureFromFileExA(LPDIRECT3DDEVICE9 pDevice, LPCSTR pSrcFile, UINT Width, UINT Height, UINT MipLevels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey, D3DXIMAGE_INFO* pSrcInfo, PALETTEENTRY* pPalette, LPDIRECT3DTEXTURE9* ppTexture);
HRESULT WINAPI D3DXSaveSurfaceToFileA(LPCSTR pDestFile, D3DXIMAGE_FILEFORMAT DestFormat, LPDIRECT3DSURFACE9 pSrcSurface, CONST PALETTEENTRY* pSrcPalette, CONST RECT* pSrcRect);
HRESULT WINAPI D3DXAssembleShaderFromFileA(LPCSTR pSrcFile, CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs);
HRESULT WINAPI D3DXCreateEffectFromFileA(LPDIRECT3DDEVICE9 pDevice, LPCSTR pSrcFile, CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXEFFECTPOOL pPool, LPD3DXEFFECT* ppEffect, LPD3DXBUFFER* ppCompilationErrors);
}

#endif // MB_PORT_D3DX9_H
