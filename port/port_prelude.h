// port/port_prelude.h: force-included (`-include port_prelude.h`, port/CMakeLists.txt) into every src/ translation unit of
// the PORTABLE configuration only (ticket 004). src/ itself carries no #ifdef (docs/patches.md), so everything the MSVC 7.1
// build gets from cl and its headers, the portable build gets from here, port/include (Windows / D3D9 / D3DX / DirectInput /
// CRT-extra shims) and port/stl (the VC7.1-shaped standard library).
//
// clang with -fms-extensions already accepts most MSVC syntax natively: __int64, __forceinline, __declspec(noinline|
// noreturn|naked|selectany|novtable|align), the i64 literal suffix, and on i686 the __cdecl/__stdcall/__fastcall/__thiscall
// keywords. This file covers the rest.
#ifndef MB_PORT_PRELUDE_H
#define MB_PORT_PRELUDE_H

#if defined(__APPLE__)
#define _DONT_USE_CTYPE_INLINE_ 1   // macOS <ctype.h>: isalpha & co. as extern "C" functions, as src/ declares them
#endif

// src/engine/core/tstring.h: raw storage of hand-built tstrings is sized with MB_TSTRING_SIZE (int, char*, int, int);
// port/main.cpp checks it against sizeof(tstring).
#if defined(__LP64__)
#define MB_TSTRING_SIZE 24
#else
#define MB_TSTRING_SIZE 16
#endif

#if !defined(__clang__)
#error "port_prelude.h is for the clang portable build only (the MSVC flow never includes it)"
#endif

// --- calling conventions -------------------------------------------------------------------------------------------
// i686: real keywords (clang, -fms-extensions), kept so callbacks handed to C code keep their convention.
// 64-bit (x86-64, arm64): one platform ABI, the keywords vanish (ticket 006 targets).
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

// --- WinLicense markers (src/platform/wl_marker.h) ---------------------------------------------------------------------
// 18 bytes of _emit that the protector's SDK left inline; no runtime effect. Pre-empt the header (its guard) so the
// portable build sees an empty block instead of an MS asm block (src/ writes WL_MARKER(12) without a ';').
#define WL_MARKER_H
#define WL_MARKER(id) {}
#define WL_MARKER_LONG(id) {}

// --- _fltused: the static CRT's floating-point marker, which several src/ TUs define (`extern "C" int _fltused = 0;`) for the
// MSVC link. Nothing reads it; each copy is a weak definition here (ld64 rejects a weakened zero-fill duplicate).
#define _fltused _fltused __attribute__((weak))

// --- MSVC-only intrinsics the source names ------------------------------------------------------------------------------
// _ReadWriteBarrier is a clang builtin under -fms-compatibility (src/ declares it itself, as for cl).
#define _AddressOfReturnAddress() ((void*)((char*)__builtin_frame_address(0) + sizeof(void*)))

// --- the vector deleting destructor called by name (src/game/data/record_destructors.cpp: `return __vecDelDtor(flags);`) --------
// MSVC's compiler-generated `??_E` member: bit 1 = an array from new[], bit 0 = free the storage.
template<class T> inline void* mb_vec_del_dtor(T* p, unsigned flags)
{
    if (flags & 2) {
        delete[] p;
    } else {
        p->T::~T();
        if (flags & 1)
            ::operator delete((void*)p);
    }
    return p;
}
#define __vecDelDtor(flags) mb_vec_del_dtor(this, (flags))

// --- the CRT surface the MSVC headers add on top of ISO C (port/include/mb_crt_extra.h) -------------------------------
#include <mb_crt_extra.h>

#endif // MB_PORT_PRELUDE_H
