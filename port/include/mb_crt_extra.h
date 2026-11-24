// port/include/mb_crt_extra.h: the Microsoft CRT names src/ uses on top of ISO C, mapped onto POSIX / glibc for the
// portable build (force-included through port/port_prelude.h). Functions the platform lacks are declared here and
// implemented by the port's runtime (port/runtime, a later milestone: linking). On Windows (_WIN32) mingw-w64's UCRT
// target provides most of this surface natively; only the name mappings that keep the ports' semantics identical stay.
#ifndef MB_PORT_CRT_EXTRA_H
#define MB_PORT_CRT_EXTRA_H

#if defined(_WIN32)
// zig's mingw targets UCRT, where <stdio.h> declares FILE as an opaque record (void* _Placeholder). src/ reads the
// stream's _flag word (src/game/config/config.cpp's Config::load_file), so declare ucrtbase's real stream ahead of
// stdio.h's _FILE_DEFINED guard. Layout and bits verified under Wine (work/winprobe2/iob.cpp): _ptr@0 _base@8 _cnt@16,
// _flag@20, _file@24, _charbuf@28, _bufsiz@32, _tmpfname@40; EOF sets 0x08 (flags 0x41 -> 0x49, feof() == 8), not
// msvcrt's 0x10. This lane runs on Wine / CrossOver only, so ucrtbase's layout is the one that has to match.
// port/gfx's TUs reach this header after <windows.h> has declared FILE: there the opaque UCRT record stands (none of
// them reads _flag).
#if !defined(_FILE_DEFINED)
struct _iobuf {
    char* _ptr;
    char* _base;
    int _cnt;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char* _tmpfname;
};
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif
#define _IOEOF 0x08
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>

#define _stricmp   strcasecmp
#define stricmp    strcasecmp
#define _strnicmp  strncasecmp
#define strnicmp   strncasecmp
#define _strcmpi   strcasecmp
#define strcmpi    strcasecmp
#define _vsnprintf vsnprintf
#define _snprintf  snprintf
#define _strdup    strdup
#define _atoi64    atoll
#define _finite(x) __builtin_isfinite(x)
#define _isnan(x)  __builtin_isnan(x)
#define _hypot     hypot
// MSVC's FILE is struct _iobuf (src/ declares functions over it) with a _flag word; glibc's is struct _IO_FILE with
// _flags, whose _IO_EOF_SEEN bit has _IOEOF's value (0x10). Only src/game/config/config.cpp reads it.
// macOS: struct __sFILE, _flags (short), __SEOF 0x20.
// Windows: declared above, ahead of stdio.h (UCRT's FILE is otherwise opaque).
#if !defined(_WIN32)
#if defined(__APPLE__)
#define _iobuf     __sFILE
#define _flag      _flags
#define _IOEOF     0x20
#else
#define _iobuf     _IO_FILE
#define _flag      _flags
#define _IOEOF     0x10
#endif
#endif
#define _MAX_PATH  260
#define MAX_PATH   260

#if !defined(_WIN32)
extern "C" {
char* mb_itoa(int value, char* buf, int radix);
char* mb_ltoa(long value, char* buf, int radix);
char* mb_ultoa(unsigned long value, char* buf, int radix);
char* mb_i64toa(long long value, char* buf, int radix);
char* mb_strupr(char* s);
char* mb_strlwr(char* s);
}
#define _itoa   mb_itoa
#define itoa    mb_itoa
#define _ltoa   mb_ltoa
#define _ultoa  mb_ultoa
#define _i64toa mb_i64toa
#define _strupr mb_strupr
#define strupr  mb_strupr
#define _strlwr mb_strlwr
#define strlwr  mb_strlwr
#endif

#if !defined(_WIN32)
// _stat: MSVC's struct _stat (32-bit sizes and times). glibc's struct stat has 64-bit members whose alignment differs
// under -malign-double, so src/ gets its own record and the port runtime fills it (mb_stat).
// Windows: mingw's <sys/stat.h> (included above) provides struct _stat and _stat natively.
struct _stat {
    unsigned st_dev; unsigned short st_ino, st_mode; short st_nlink, st_uid, st_gid; unsigned st_rdev; long st_size;
#if defined(__APPLE__)
    struct timespec st_atimespec, st_mtimespec, st_ctimespec;   // macOS's st_atime/st_mtime/st_ctime macros name these
#else
    struct timespec st_atim, st_mtim, st_ctim;      // glibc's st_atime/st_mtime/st_ctime macros name these
#endif
};
extern "C" int mb_stat(const char* path, struct _stat* st);
#define _stat(path, st) mb_stat((path), (st))
#endif

// File names in src/ are Windows names ("Data\\item_kinds1.txt", "data\\ground_specs.txt": backslashes, any case). The stdio
// entry points that take a path go through the platform layer's path resolution (port/platform/fs.cpp: '\\' -> '/',
// case-insensitive component lookup against a cached directory index, CRLF -> LF for text-mode reads, rename that does not
// replace). Object-like macros: every use, including src/engine/core/crt.h's own declarations of these names, is renamed alike.
// Windows: backslashes and case-insensitivity are native, msvcrt's text mode does CRLF <-> LF and its rename does not
// replace — src/ calls fopen/remove/rename directly.
#include "../platform/mb_paths.h"
#if !defined(_WIN32)
#define fopen  mb_fopen
#define remove mb_remove
#define rename mb_rename
#endif

#endif
