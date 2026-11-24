// port/include/direct.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#if defined(_WIN32)
// mingw-w64's real direct.h: _mkdir/_rmdir/_chdir/_getcwd are native (msvcrt).
#include_next <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
extern "C" int mb_mkdir(const char* path);
#define _mkdir mb_mkdir
extern "C" int mb_rmdir(const char* path);
#define _rmdir mb_rmdir
#define _chdir chdir
#define _getcwd getcwd
#endif