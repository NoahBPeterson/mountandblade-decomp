// port/include/oleauto.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
typedef WCHAR OLECHAR;
typedef OLECHAR* BSTR;
typedef unsigned short VARTYPE;
enum VARENUM { VT_EMPTY = 0, VT_NULL = 1, VT_I2 = 2, VT_I4 = 3, VT_R4 = 4, VT_R8 = 5, VT_BSTR = 8, VT_BOOL = 11, VT_UI4 = 19 };
typedef struct tagVARIANT {
    VARTYPE vt; WORD wReserved1, wReserved2, wReserved3;
    union { LONG lVal; BYTE bVal; SHORT iVal; FLOAT fltVal; DOUBLE dblVal; short boolVal; BSTR bstrVal; ULONG ulVal; IUnknown* punkVal; void* byref; };
} VARIANT, *LPVARIANT, VARIANTARG;
#define CLSCTX_INPROC_SERVER 0x1
extern "C" {
HRESULT WINAPI CoInitialize(LPVOID pvReserved);
void WINAPI CoUninitialize(void);
HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);
void WINAPI VariantInit(VARIANTARG* pvarg);
HRESULT WINAPI VariantClear(VARIANTARG* pvarg);
void WINAPI SysFreeString(BSTR bstrString);
}
