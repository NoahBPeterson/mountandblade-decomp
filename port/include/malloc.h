// port/include/malloc.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <stdlib.h>
#if defined(_WIN32)
#include_next <malloc.h>       // mingw's own: __mingw_aligned_malloc / __mingw_aligned_free (clang's mm_malloc.h) and _alloca
#else
#include <alloca.h>
#define _alloca alloca
#endif
