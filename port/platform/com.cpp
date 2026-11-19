// port/platform/com.cpp: the COM / OLE / SkinMagic names the launcher uses.
//  * CoInitialize succeeds (an apartment is a no-op without a COM runtime); CoCreateInstance reports REGDB_E_CLASSNOTREG, the
//    real answer for a class no server provides here (the DxDiag provider the launcher asks for), which its caller handles.
//  * VARIANT / BSTR: a BSTR is a malloc block with a 4-byte byte-length prefix.
//  * SkinMagic skins the launcher dialog with a .smf file; the dialog is virtual (dialog.cpp), so there is nothing to skin.
#include "mbp_internal.h"
#include <oleauto.h>

extern "C" {
HRESULT WINAPI CoInitialize(LPVOID) { return S_OK; }
void WINAPI CoUninitialize(void) {}
HRESULT WINAPI CoCreateInstance(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID* ppv)
{
    if (ppv) *ppv = nullptr;
    return (HRESULT)0x80040154;           // REGDB_E_CLASSNOTREG
}
void WINAPI VariantInit(VARIANTARG* v) { memset(v, 0, sizeof *v); }
void WINAPI SysFreeString(BSTR s) { if (s) free((char*)s - 4); }
BSTR WINAPI SysAllocString(const OLECHAR* s)
{
    if (!s) return nullptr;
    size_t n = 0;
    while (s[n]) ++n;
    char* p = (char*)malloc(4 + (n + 1) * sizeof(OLECHAR));
    uint32_t bytes = (uint32_t)(n * sizeof(OLECHAR));
    memcpy(p, &bytes, 4);
    memcpy(p + 4, s, (n + 1) * sizeof(OLECHAR));
    return (BSTR)(p + 4);
}
HRESULT WINAPI VariantClear(VARIANTARG* v)
{
    if (v->vt == VT_BSTR) SysFreeString(v->bstrVal);
    else if (v->vt == 13 /*VT_UNKNOWN*/ && v->punkVal) v->punkVal->Release();
    memset(v, 0, sizeof *v);
    return S_OK;
}
void WINAPI InitSkinMagicLib(HINSTANCE, const char*, int, int) {}
void WINAPI ExitSkinMagicLib() {}
void WINAPI LoadSkinFile(const char*) {}
int WINAPI SetSingleDialogSkin(HWND, const char*) { return 1; }
}
