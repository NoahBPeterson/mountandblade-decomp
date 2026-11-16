// port/include/direct.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <unistd.h>
#include <sys/stat.h>
extern "C" int mb_mkdir(const char* path);
#define _mkdir mb_mkdir
#define _rmdir rmdir
#define _chdir chdir
#define _getcwd getcwd
