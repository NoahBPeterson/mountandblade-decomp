// port/runtime/crt_extra.cpp: the Microsoft CRT functions src/ calls that glibc lacks (ticket 004 milestone 2), declared by
// port/include/io.h, mb_crt_extra.h and process.h. Real POSIX implementations.
//
// Compiled WITHOUT the src/ flags (no prelude, no -malign-double: port/CMakeLists.txt mb_runtime), because these call
// glibc with glibc's own structs (struct stat, struct dirent). The records src/ sees (_finddata_t, struct _stat) have no
// 64-bit members, so their layout is the same under both alignment rules.
//
// Paths: src/ writes '\\' separators; they are turned into '/' here. Case-insensitive lookup (the game names
// "Data\\item_kinds1.txt" and "data\\..." alike) is ticket 007's job, not done here.
#include <io.h>
#include <process.h>
#include <mb_crt_extra.h>
#include "../platform/mb_paths.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

namespace {

char* to_base(unsigned long long v, bool neg, char* buf, int radix)
{
    if (radix < 2 || radix > 36) {
        buf[0] = 0;
        return buf;
    }
    char tmp[72];
    int n = 0;
    do {
        int d = (int)(v % (unsigned)radix);
        tmp[n++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned)radix;
    } while (v);
    char* p = buf;
    if (neg)
        *p++ = '-';
    while (n)
        *p++ = tmp[--n];
    *p = 0;
    return buf;
}

struct ThreadStart {
    void (__cdecl* fn)(void*);
    void* arg;
};

void* thread_main(void* p)
{
    ThreadStart s = *(ThreadStart*)p;
    free(p);
    s.fn(s.arg);
    return 0;
}

} // namespace

extern "C" {

// ---- _findfirst / _findnext / _findclose (io.h): the platform layer's directory enumeration (case-insensitive,
// backslash paths, sorted, mbp_find_open in port/platform/fs.cpp) ----------------------------------------------------------
static void fill_finddata(const struct mbp_dirent& d, struct _finddata_t* fi)
{
    memset(fi, 0, sizeof(*fi));
    fi->attrib = (d.attrib & 0x10) ? _A_SUBDIR : _A_NORMAL;
    if (d.attrib & 1)
        fi->attrib |= _A_RDONLY;
    fi->time_create = (time_t)d.ctime;
    fi->time_access = (time_t)d.atime;
    fi->time_write = (time_t)d.mtime;
    fi->size = (unsigned long)d.size;
    strncpy(fi->name, d.name, sizeof(fi->name) - 1);
}
intptr_t mb_findfirst(const char* filespec, struct _finddata_t* fileinfo)
{
    struct mbp_dirent d;
    void* h = mbp_find_open(filespec, &d);
    if (!h) {
        errno = ENOENT;
        return -1;
    }
    fill_finddata(d, fileinfo);
    return (intptr_t)h;
}

int mb_findnext(intptr_t handle, struct _finddata_t* fileinfo)
{
    if (handle == -1 || !handle)
        return -1;
    struct mbp_dirent d;
    if (mbp_find_next((void*)handle, &d) != 0) {
        errno = ENOENT;
        return -1;
    }
    fill_finddata(d, fileinfo);
    return 0;
}

int mb_findclose(intptr_t handle)
{
    if (handle == -1 || !handle)
        return -1;
    mbp_find_close((void*)handle);
    return 0;
}

// ---- _itoa & co., _strupr / _strlwr (mb_crt_extra.h) ----------------------------------------------------------------
// MSVC semantics: only radix 10 prints a sign; other radixes print the two's complement bits.
char* mb_itoa(int value, char* buf, int radix)
{
    if (radix == 10 && value < 0)
        return to_base(0ull - (unsigned long long)(long long)value, true, buf, radix);
    return to_base((unsigned)value, false, buf, radix);
}
char* mb_ltoa(long value, char* buf, int radix) { return mb_itoa((int)value, buf, radix); }
char* mb_ultoa(unsigned long value, char* buf, int radix) { return to_base(value, false, buf, radix); }
char* mb_i64toa(long long value, char* buf, int radix)
{
    if (radix == 10 && value < 0)
        return to_base(0ull - (unsigned long long)value, true, buf, radix);
    return to_base((unsigned long long)value, false, buf, radix);
}
char* mb_strupr(char* s)
{
    for (char* p = s; *p; ++p)
        *p = (char)toupper((unsigned char)*p);
    return s;
}
char* mb_strlwr(char* s)
{
    for (char* p = s; *p; ++p)
        *p = (char)tolower((unsigned char)*p);
    return s;
}

// ---- _stat (mb_crt_extra.h): MSVC's 32-bit record from glibc's stat ---------------------------------------------------
int mb_stat(const char* path, struct _stat* st)
{
    char p[PATH_MAX];
    mbp_resolve_path(path, p, sizeof(p));
    struct stat sb;
    if (stat(p, &sb) != 0)
        return -1;
    memset(st, 0, sizeof(*st));
    st->st_dev = (unsigned)sb.st_dev;
    st->st_ino = (unsigned short)sb.st_ino;
    st->st_mode = (unsigned short)sb.st_mode;
    st->st_nlink = (short)sb.st_nlink;
    st->st_uid = (short)sb.st_uid;
    st->st_gid = (short)sb.st_gid;
    st->st_rdev = (unsigned)sb.st_rdev;
    st->st_size = (long)sb.st_size;
#if defined(__APPLE__)
    st->st_atimespec = sb.st_atimespec;
    st->st_mtimespec = sb.st_mtimespec;
    st->st_ctimespec = sb.st_ctimespec;
#else
    st->st_atim = sb.st_atim;
    st->st_mtim = sb.st_mtim;
    st->st_ctim = sb.st_ctim;
#endif
    return 0;
}

// ---- _beginthread / _endthread (process.h) ------------------------------------------------------------------------
uintptr_t _beginthread(void (__cdecl* start_address)(void*), unsigned stack_size, void* arglist)
{
    ThreadStart* s = (ThreadStart*)malloc(sizeof(ThreadStart));
    if (!s)
        return (uintptr_t)-1;
    s->fn = start_address;
    s->arg = arglist;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (stack_size)
        pthread_attr_setstacksize(&attr, stack_size < PTHREAD_STACK_MIN ? PTHREAD_STACK_MIN : stack_size);
    pthread_t t;
    int rc = pthread_create(&t, &attr, thread_main, s);
    pthread_attr_destroy(&attr);
    if (rc != 0) {
        free(s);
        return (uintptr_t)-1;
    }
    return (uintptr_t)t;
}

void _endthread(void) { pthread_exit(0); }

} // extern "C"
