// port/include/mmsystem.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
extern "C" DWORD WINAPI timeGetTime(void);
extern "C" UINT WINAPI timeBeginPeriod(UINT uPeriod);
extern "C" UINT WINAPI timeEndPeriod(UINT uPeriod);
