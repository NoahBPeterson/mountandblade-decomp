// port/platform/mbp_prelude.h: force-included into the platform library's TUs. The same calling-convention macros
// port/port_prelude.h gives src/ (so port/include/windows.h declares the API with the identical signatures), without
// the CRT-extra macro layer (the platform code calls libc directly).
#pragma once
#if !defined(__i386__)
#  define __cdecl
#  define __stdcall
#  define __fastcall
#  define __thiscall
#  define _cdecl
#  define _stdcall
#endif
#define WINAPI __stdcall
#define APIENTRY __stdcall
#define CALLBACK __stdcall
#define STDMETHODCALLTYPE __stdcall
#define CDECL __cdecl
#define WINAPIV __cdecl
