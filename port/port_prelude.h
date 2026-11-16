// patches/port_sketch/port_prelude.h: SKETCH, UNTESTED, NOT WIRED INTO ANY BUILD. Ticket 004 (docs/tickets/004-portable-build-config-cmake-clang.md).
//
// Force-included (`clang -include port_prelude.h`) into every src/ translation unit of the PORTABLE configuration only,
// so src/ itself carries no #ifdef (docs/patches.md: "No #ifdef PATCH in src/"). It papers over the MSVC-isms that
// ticket 005 counted in src/ on 2026-11-02:
//   __cdecl 556, __stdcall 204, __fastcall 160, __thiscall 11   (931 uses, 158 files)
//   __int64 712 (117 files), __declspec(noinline) 266, (dllimport) 54, (noreturn) 7, (naked) 1
//   #pragma comment(linker, "/alternatename:...") 60 (needs a real fix, see 005), __asm 20 (WL_MARKER, 2 files)
#pragma once

#if defined(__clang__) || defined(__GNUC__)

// --- types -------------------------------------------------------------------------------------------------------
#define __int64 long long
#define __int32 int
#define __int16 short
#define __forceinline inline __attribute__((always_inline))
#define __inline inline

// --- calling conventions -----------------------------------------------------------------------------------------
// 32-bit x86 only: the conventions are real there (the vtable calls into the game's own code never cross a DLL, so
// ANY consistent choice works inside the portable build; keep them only where a callback is handed to C code).
// 64-bit (x86-64, arm64): all of them are the platform ABI and the keywords must vanish.
#if defined(__i386__)
#  define __cdecl     __attribute__((cdecl))
#  define __stdcall   __attribute__((stdcall))
#  define __fastcall  __attribute__((fastcall))
#  define __thiscall  __attribute__((thiscall))
#else
#  define __cdecl
#  define __stdcall
#  define __fastcall
#  define __thiscall
#endif

// --- __declspec --------------------------------------------------------------------------------------------------
// Function-like object macros cannot express __declspec(x) -> attribute, so src/ would need one mechanical rewrite
// (sed 's/__declspec(noinline)/MB_NOINLINE/') OR the prelude can use clang's -fms-extensions:
//   clang -fms-extensions -fms-compatibility   accepts __declspec(noinline|noreturn|naked|dllimport) natively
// Prefer that and leave this block empty; it is listed to show what has to be true.

// --- MSVC integer-literal suffix ---------------------------------------------------------------------------------
// 1i64 / 0xffi64 -> compile with -fms-extensions (accepted) or sed to LL.

// --- CRT names ---------------------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#define _stricmp  strcasecmp
#define stricmp   strcasecmp
#define _strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#define _itoa(v, b, r) mb_itoa((v), (b), (r))
extern "C" char* mb_itoa(int value, char* buf, int radix);
#define _stat   stat
#define _atoi64 atoll

#endif
