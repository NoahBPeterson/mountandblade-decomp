// mesh2_common.h: engine entry points and the translation table shared by the three modules of 0x4c2c50..0x4ce720
// (module_option_records.cpp, editor_selection_and_translation.cpp, poly_mesh_edge_face_ops.cpp). The runtime functions are the static CRT's (operator new/delete and
// their array forms, sprintf); StrTable (game/data/str_table.h) is the translation table (the UI string table at 0x5e2a18; find 0x4c6010 is defined in editor_selection_and_translation.cpp, the ctor, loader and the global in poly_mesh_edge_face_ops.cpp).
#ifndef MESH2_COMMON_H
#define MESH2_COMMON_H
#include <stddef.h>
#include <stdio.h>
#include "engine/core/tstring.h"

#include "engine/core/crt.h"   // canonical: operator new (0x53545e)

// The UI string table (global at 0x5e2a18): 0xdf3 hash buckets of tstring vectors, keys in one half and the
// translations in the other (find 0x4c6010 returns the translation of `key`, or null).
// Constructs a tstring in place on raw storage. The standard placement new puts a null test in front of the
// call that the original does not have, so the constructor (0x42bbd0) is called explicitly.
#define CONSTRUCT_TSTRING(p) (((tstring*)(p))->tstring::tstring())

#include "game/data/str_table.h"   // StrTable (the translation table, was LangTables here): g_strtable 0x5e2a18, ctor 0x4cda20, load 0x4cda90, find 0x4c6010

unsigned __stdcall hash_string(const tstring* key);   // 0x433bb0 (engine/fx/env_curve.h): a free stdcall function

// one "key|value" row of a language csv, as 0x4c8ca0 writes it inline
inline void write_row(FILE* f, const tstring& key, const tstring& value)
{
    fputs(key.c_str(), f);                              // 0x537ef4
    fputs("|", f);                                      // 0x5946f8
    fputs(value.c_str(), f);                            // 0x537ef4
    fputs("\n", f);                                     // 0x54b118
}

// key lookup with fallback, used for the untranslated rows
inline const tstring* find_or(const tstring& key, const tstring& fallback)
{
    const tstring* p = g_strtable.find(&key);
    return p ? p : &fallback;
}

// "key" + suffix lookup with fallback (the plural / text / desc variants)
inline const tstring* suffix_or(const tstring& key, const char* suffix, const tstring& fallback)
{
    // one full expression: the lookup runs while the suffix and the sum are both alive, and they die after it
    const tstring* p = g_strtable.find(&(key + tstring(suffix)));
    return p ? p : &fallback;
}

#endif
