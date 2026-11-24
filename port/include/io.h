// port/include/io.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#if defined(_WIN32)
// mingw-w64's real io.h: _finddata_t and _A_* attribs are native. The find FUNCTIONS are wrapped (defined in
// port/platform/fs.cpp, next to the other directory enumeration): wine's 64-bit kernel32 hands out
// directory-search handles as raw pointers (FindNextFileW validates them by dereferencing a 0xC0FFEE11-tagged
// struct), but callers store the handles in 32-bit longs — harmless on real Windows, where handles are small,
// yet truncating wine's pointer makes every _findnext fault. The port keeps the real HANDLEs in a table and
// hands out small integer handles that survive the truncation.
#include_next <io.h>
extern "C" {
intptr_t mb_win_findfirst(const char* filespec, struct _finddata_t* fileinfo);
int mb_win_findnext(intptr_t handle, struct _finddata_t* fileinfo);
int mb_win_findclose(intptr_t handle);
}
#define _findfirst mb_win_findfirst
#define _findnext mb_win_findnext
#define _findclose mb_win_findclose
extern "C" int mb_access(const char* path, int mode);
#define _access mb_access
#else
#include <stddef.h>
#include <time.h>
#include <unistd.h>
// _findfirst / _findnext / _findclose: implemented by the port runtime over opendir/fnmatch (ticket 007's mbp::find_first).
struct _finddata_t { unsigned attrib; time_t time_create, time_access, time_write; unsigned long size; char name[260]; };
#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_SUBDIR 0x10
#define _A_ARCH 0x20
extern "C" {
intptr_t mb_findfirst(const char* filespec, struct _finddata_t* fileinfo);
int mb_findnext(intptr_t handle, struct _finddata_t* fileinfo);
int mb_findclose(intptr_t handle);
}
#define _findfirst mb_findfirst
#define _findnext mb_findnext
#define _findclose mb_findclose
extern "C" int mb_access(const char* path, int mode);
#define _access mb_access
#endif