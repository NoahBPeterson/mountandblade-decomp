// port/include/malloc.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <stdlib.h>
#include <alloca.h>
#define _alloca alloca
