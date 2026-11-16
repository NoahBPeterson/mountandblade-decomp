// port/include/shlobj.h: portable-build shim (ticket 004): declares only what src/ uses.
#pragma once
#include <windows.h>
#define CSIDL_PERSONAL 0x0005
#define CSIDL_APPDATA 0x001a
#define CSIDL_LOCAL_APPDATA 0x001c
#define CSIDL_FLAG_CREATE 0x8000
#define SHGFP_TYPE_CURRENT 0
extern "C" HRESULT WINAPI SHGetFolderPathA(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath);
#define SHGetFolderPath SHGetFolderPathA
