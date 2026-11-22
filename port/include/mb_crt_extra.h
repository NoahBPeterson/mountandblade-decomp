// port/include/mb_crt_extra.h: the Microsoft CRT names src/ uses on top of ISO C, mapped onto POSIX / glibc for the
// portable build (force-included through port/port_prelude.h). Functions the platform lacks are declared here and
// implemented by the port's runtime (port/runtime, a later milestone: linking).
#ifndef MB_PORT_CRT_EXTRA_H
#define MB_PORT_CRT_EXTRA_H

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
#if defined(__APPLE__)
#define _iobuf     __sFILE
#define _flag      _flags
#define _IOEOF     0x20
#else
#define _iobuf     _IO_FILE
#define _flag      _flags
#define _IOEOF     0x10
#endif
#define _MAX_PATH  260
#define MAX_PATH   260

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

// _stat: MSVC's struct _stat (32-bit sizes and times). glibc's struct stat has 64-bit members whose alignment differs
// under -malign-double, so src/ gets its own record and the port runtime fills it (mb_stat).
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

// File names in src/ are Windows names ("Data\\item_kinds1.txt", "data\\ground_specs.txt": backslashes, any case). The stdio
// entry points that take a path go through the platform layer's path resolution (port/platform/fs.cpp: '\\' -> '/',
// case-insensitive component lookup against a cached directory index, CRLF -> LF for text-mode reads, rename that does not
// replace). Object-like macros: every use, including src/engine/core/crt.h's own declarations of these names, is renamed alike.
#include "../platform/mb_paths.h"
#define fopen  mb_fopen
#define remove mb_remove
#define rename mb_rename

#endif
