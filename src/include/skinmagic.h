// skinmagic.h: the part of the SkinMagic API (SkinMagic.dll, linked through an import library the glue builds from the
// DLL's export table) the game calls. The exports are stdcall C functions: _InitSkinMagicLib@16, ... ONE declaration for
// every caller.
#ifndef MB_SKINMAGIC_H
#define MB_SKINMAGIC_H
#include <windows.h>

extern "C" {
void __stdcall InitSkinMagicLib(HINSTANCE inst, const char* app, int a, int b);   // 0x535452 (ordinal 1)
void __stdcall ExitSkinMagicLib();                                                // 0x535458 (ordinal 2)
void __stdcall LoadSkinFile(const char* path);                                    // 0x53544c (ordinal 3)
int  __stdcall SetSingleDialogSkin(HWND dialog, const char* skin_class);          // 0x535446 (ordinal 9)
}

#endif
