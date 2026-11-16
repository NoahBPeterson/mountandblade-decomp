// port/include/shellapi.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
extern "C" HINSTANCE WINAPI ShellExecuteA(HWND hwnd, LPCSTR lpOperation, LPCSTR lpFile, LPCSTR lpParameters, LPCSTR lpDirectory, INT nShowCmd);
#define ShellExecute ShellExecuteA
