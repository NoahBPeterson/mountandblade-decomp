// crt.h: the statically linked CRT entry points (VC7.1 /MT) under their one canonical name each (sorted by address).
#ifndef INCLUDE_CRT_H
#define INCLUDE_CRT_H

#include <stddef.h>
#include <stdio.h>

// card: ?engine_new@@YAPAXI@Z 36 bytes, the scalar operator new (malloc with new handler);
void* __cdecl operator new(size_t size);                  // 0x53545e
// card: ??_U@YAPAXI@Z, the array operator new thunk;
void* __cdecl operator new[](size_t size);              // 0x535482
// card: ??_V@YAXPAX@Z; operator_delete_array declares a bogus second argument (ret 8) while
void __cdecl operator delete[](void* p);              // 0x53570d
// card: ??3@YAXPAX@Z; game_malloc is a misnomer for the free wrapper;
void __cdecl operator delete(void* p);                // 0x535712
// card: lib_crt::_snprintf (calls __output with a FILE-like struct, null-terminates);
extern "C" int __cdecl _snprintf(char* buf, size_t count, const char* fmt, ...);   // 0x535717
// card: __ftol2; the cdecl float round_top spelling is the same helper;
extern "C" int __stdcall ftol2(double d);                      // 0x535a64
// card: lib_crt::__CxxThrowException@8 -> C name _CxxThrowException;
extern "C" __declspec(noreturn) void __stdcall __CxxThrowException(void* obj, void* info);   // 0x5368e6
// card: lib_crt::rand; header src/include/crt.h is new (CRT/operator declarations are not de
extern "C" int __cdecl rand(void);                             // 0x536a9f
// card: floor (x87, 289 bytes);
extern "C" double __cdecl floor(double v);                    // 0x536b10
// card: lib_crt::_fopen = __fsopen(name, mode, 0x40);
extern "C" FILE* __cdecl fopen(const char* name, const char* mode);   // 0x536f7c
// card: lib_crt::sprintf;
extern "C" int __cdecl sprintf(char* buf, const char* fmt, ...);   // 0x536f8f
// card: lib_crt::_fclose (__lock_file, __fclose_lk);
extern "C" int __cdecl fclose(FILE* f);                      // 0x537168
// card: _atoi, a 5-byte jmp to _atol (0x5372c5);
extern "C" int __cdecl atoi(const char* s);                    // 0x53734d
// card: lib_crt::_remove (DeleteFileA, __dosmaperr, returns 0 / -1);
extern "C" int __cdecl remove(const char* path);                // 0x539023

#endif // INCLUDE_CRT_H
