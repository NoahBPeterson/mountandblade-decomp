// port/include/io.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
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
#define _access access
