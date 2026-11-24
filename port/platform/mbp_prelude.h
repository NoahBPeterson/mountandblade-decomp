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

// --- environment reads see what SDL sets (Windows only) -------------------------------------------------------------
// SDL's SDL_setenv_unsafe writes only the Win32 process block (SetEnvironmentVariableA, SDL_getenv.c's
// HAVE_WIN32_ENVIRONMENT branch), which the mingw CRT's getenv never reads (probed work/winprobe2/env2: _putenv is
// visible to both, SetEnvironmentVariableA only to Win32). The platform lane reads the environment with getenv (fs.cpp's
// MB_USER_DIR sandbox, gdi.cpp's MB_FONT/WINDIR, window.cpp's MBP_HIGHDPI ...), so a value SDL sets at run time would be
// invisible. Redirect to mbp_getenv (defined in fs.cpp, in that file's extern "C" block): the CRT value first, then the
// Win32 block. On POSIX SDL calls setenv, so plain getenv already agrees, and gfx/audio/test TUs that do not link
// mb_platform keep the CRT getenv.
#if defined(_WIN32)
#include <stdlib.h>
extern "C" const char* mbp_getenv(const char* name);
#define getenv(name) mbp_getenv(name)
#endif
