// patches/devfast/iat_hook.h: point one of our own import-table slots at a replacement.
//
// The loader fills the import address table before the C runtime runs static constructors, so a static constructor
// can swap a slot: every call the game makes through that import (directly, `call [__imp__X]`, or through the
// linker's `jmp [__imp__X]` thunk) then reaches the replacement. Returns the previous target, 0 if the import is absent.
#pragma once
#include <windows.h>
#include <string.h>

inline void* iat_hook(const char* dll, const char* func, void* repl)
{
    char* base = (char*)GetModuleHandleA(0);
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (IMAGE_IMPORT_DESCRIPTOR* d = (IMAGE_IMPORT_DESCRIPTOR*)(base + dir.VirtualAddress); d->Name; d++) {
        if (_stricmp(base + d->Name, dll) != 0)
            continue;
        IMAGE_THUNK_DATA* names = (IMAGE_THUNK_DATA*)(base + (d->OriginalFirstThunk ? d->OriginalFirstThunk : d->FirstThunk));
        IMAGE_THUNK_DATA* slots = (IMAGE_THUNK_DATA*)(base + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, slots++) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                continue;
            if (strcmp((char*)((IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData))->Name, func) != 0)
                continue;
            DWORD old;
            void* prev = (void*)slots->u1.Function;
            VirtualProtect(&slots->u1.Function, 4, PAGE_READWRITE, &old);
            slots->u1.Function = (DWORD)repl;
            VirtualProtect(&slots->u1.Function, 4, old, &old);
            return prev;
        }
    }
    return 0;
}
