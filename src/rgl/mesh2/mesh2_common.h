// mesh2_common.h: engine entry points and the translation table shared by the three modules of 0x4c2c50..0x4ce720
// (rec_tables.cpp, gui_edit.cpp, mesh_2.cpp). The runtime functions are the static CRT's (operator new/delete and
// their array forms, sprintf); LangTables is the translation table (the UI string table at 0x5e2a18; find 0x4c6010 is defined in gui_edit.cpp, the ctor and loader in mesh_2.cpp).
#ifndef MESH2_COMMON_H
#define MESH2_COMMON_H
#include <stddef.h>
#include <stdio.h>
#include "../../game/tstring.h"

#include "../../include/crt.h"   // canonical: operator new (0x53545e)

// The UI string table (global at 0x5e2a18): 0xdf3 hash buckets of tstring vectors, keys in one half and the
// translations in the other (find 0x4c6010 returns the translation of `key`, or null).
// Constructs a tstring in place on raw storage. The standard placement new puts a null test in front of the
// call that the original does not have, so the constructor is called through this declaration (0x42bbd0,
// thiscall, no arguments; the resolver reads the address from the comment).
struct tstring_ctor { void ctor(); };   // 0x42bbd0
#define CONSTRUCT_TSTRING(p) (((tstring_ctor*)(p))->ctor())

// One language hash bucket. The bucket is a 16-byte record whose FIRST dword the original never touches: every access
// (0x4c6010, the insert paths of 0x4cda90, the zero loops of 0x4cda20) addresses a std::vector<tstring> at +4..+0xf,
// so the leading dword is kept as padding and the vector is described by hand.
struct lang_bucket {
    int pad;                                                    // +0x00
    tstring* myfirst;                                           // +0x04
    tstring* mylast;                                            // +0x08
    tstring* myend;                                             // +0x0c
    unsigned size() const { return myfirst ? (unsigned)(mylast - myfirst) : 0u; }
};

// The two hash tables of the translation table (0xdf3 buckets each: the keys, then the values). 0x4cda20 emits the
// 0x42bbd0 calls for the two trailing strings *after* the bucket zero loops, but a ctor's member construction always
// precedes its body -- so the tables live in a base whose inlined ctor clears them first, and the strings stay members
// of the derived class.
struct LangTablesBase {
    lang_bucket keys[0xdf3];                                    // +0x0000
    lang_bucket vals[0xdf3];                                    // +0xdf30
    __forceinline LangTablesBase()
    {
        int* k = (int*)keys + 2;
        int n = 0xdf3;
        do {
            k[-1] = 0; k[0] = 0; k[1] = 0;
            k += 4;
            n--;
        } while (n != 0);
        int* v = (int*)vals + 2;
        n = 0xdf3;
        do {
            v[-1] = 0; v[0] = 0; v[1] = 0;
            v += 4;
            n--;
        } while (n != 0);
    }
};

unsigned __stdcall hash_string(const tstring* key);   // 0x433bb0 (game/agent/curve.h): a free stdcall function

// The translation table: the UI string table (global at 0x5e2a18) and the language tables (0x5a8e68) are instances of it.
struct LangTables : LangTablesBase {
    tstring notfound;                                           // +0x1be60 "STR NOT FOUND"
    tstring language;                                           // +0x1be70
    LangTables();                                               // 0x4cda20
    tstring* find(const tstring* key) throw();                  // 0x4c6010 (null when absent)
    tstring* lookup(const tstring& key) throw();                // 0x43a1e0 (never null: a shared "ui_unknown..." entry on a miss)
    void write(FILE* f, const tstring& key, const tstring& value);   // 0x4bd740 ("key|value\n" row of a language file)
    void load(const tstring* root);                             // 0x4cda90 (the path prefix is a tstring: buf, else its inline bytes at +4)
    void release_strings();                                     // 0x4ca720
};
extern LangTables g_strtable;                         // 0x5e2a18

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
