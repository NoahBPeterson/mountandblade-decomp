// port/include/process.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <stdint.h>
extern "C" uintptr_t _beginthread(void (__cdecl* start_address)(void*), unsigned stack_size, void* arglist);
extern "C" void _endthread(void);
