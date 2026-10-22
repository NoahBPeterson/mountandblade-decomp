// Slice s00512620: the item-price / item-name helpers of the inventory code (0x512620..0x5129d0).
// Names and layouts are provisional; every offset is taken from this slice's disassembly. The
// emitter call (0x512620) is the 3x__int64 variant of SoundCtl::emit_event at 0x435790 (slice
// s00435490), whose source shape is reused here.
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---------------------------------------------------------------------------------------------------------
// tstring (layout from slice s0042a1f0; the inline c_str body is what the callers inline)
struct tstring {
    int pos;                                            // +0
    char* buf;                                          // +4
    int cap;                                            // +8
    int len;                                            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    char equals(const tstring& o) const;                // 0x42bdd0
    void underscores_to_spaces();                       // 0x42bf80
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// translation table at 0x5e2a18 (hash buckets of 16-byte strings)
struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0
    tstring* find(const tstring& key) MB_NOTHROW;       // 0x4c6010 (null when absent)
};
extern StrTable g_strtable;                             // 0x5e2a18

// ---------------------------------------------------------------------------------------------------------
// item kind record, 0x238 bytes (array at 0x5e29d0)
struct ItemKind {
    tstring id;                                         // +0  (fallback display name)
    char pad10[0x20 - 0x10];
    tstring name;                                       // +0x20 (translation key)
    char pad30[0x1c8 - 0x30];
    union {
        unsigned flags;                                 // +0x1c8 (0x2000000 tested below)
        struct { unsigned char type; char tpad[3]; };
    };
    char pad1cc[0x1d8 - 0x1cc];
    int value;                                          // +0x1d8
    char pad1dc[0x210 - 0x1dc];
    int count;                                          // +0x210
    char pad214[0x21c - 0x214];
    float f21c;                                         // +0x21c
    char pad220[0x238 - 0x220];
};
extern ItemKind* g_items;                               // 0x5e29d0

// item specifier (a 2-dword ref plus a signed modifier byte at +7)
struct ItemRef {
    int id;                                             // +0
    int f4;                                             // +4 (low byte = count, +7 = modifier)
    void name(char* out);                               // 0x512730
    int max_amount();                                   // 0x434b70 (also seen as 0x434b70 in s00434ad0)
};

// modifier record, 0x28 bytes (array at 0xa75f70): format string, translation key, value weight
struct Modifier {
    tstring fmt;                                        // +0
    tstring key;                                        // +0x10
    float weight;                                       // +0x20
    char pad24[0x4];
};
extern Modifier g_modifiers[];                          // 0xa75f70
extern float g_sort_val[];                              // 0xa75f90 (weight slot of the same records)

// ---------------------------------------------------------------------------------------------------------
// script emitter record (0x34 bytes, array pointer at 0xa75eb0, parallel index array at 0xa75ebc)
struct EmitSub {
    char emit(int flags, int zero, int* count, __int64* vals, unsigned tag);   // 0x4b09e0
};
struct Emitter {
    char pad0[4];
    tstring name;                                       // +4
    char pad14[0x18 - 0x14];
    EmitSub sub;                                        // +0x18
    char pad19[0x34 - 0x19];
};
extern Emitter* g_emitters;                             // 0xa75eb0
extern int g_emitter_idx[];                             // 0xa75ebc

// 0x5c8-byte party entry of the party pool (0x5ac8 bytes)
struct Party {
    char pad0[0x5ac8];
    int skill(int idx);                                 // 0x4b2370
};

// the party container at game+0xf8
struct PartyPool {
    char pad0[0x138d0];
    Party** pages;                                      // +0x138d0
    int  find_index(int id);                            // 0x48ad00
    Party* find(int id);                                // 0x51ee50
    Party* at(int id)
    {
        int i = find_index(id);
        return (Party*)pages[i >> 4] + (i & 0xf);
    }
};

// game data singleton at 0x5e2930 (only the fields this slice touches)
struct GameData {
    char pad0[0x78];
    int f78;                                            // +0x78
    char pad7c[0xf8 - 0x7c];
    PartyPool parties;                                  // +0xf8
    char pad13a00[0x13a00 - (0xf8 + sizeof(PartyPool))];
    char* chars;                                        // +0x13a00 (0x508-byte rows)
    char pad13a04[0x13a48 - 0x13a04];
    char* char_data;                                    // +0x13a48
    char pad13a4c[0x1a068 - 0x13a4c];
    int f1a068;                                         // +0x1a068
    tstring name;                                       // +0x1a06c
    int f1a07c;                                         // +0x1a07c

    char emit_event(int id, int a, int b, int c);                    // 0x512620
    int  price(ItemRef* item, int idx, char adjust, char plain, char scaled);   // 0x5129d0
    char pick_event(int mode, int id, int arg);                      // 0x435790
};
extern GameData* g_game;                                // 0x5e2930

// 4-byte out slot pair shared with the script query (0xa70458 contains both dwords); read as an
// __int64 by the value scaling below (the original uses fild qword).
extern __int64 g_result;                                // 0xa70458

// inventory window (only the fields this slice touches)
struct InvWindow {
    char pad0[4];
    int f4;                                             // +4
    char pad8[0x14 - 0x8];
    int troop;                                          // +0x14
    char pad18[0xabc - 0x18];
    char fabc;                                          // +0xabc
    void compute(int* item, int mode);                  // 0x512960
};

// page/stash slot-flag table
struct SlotWindow {
    char pad0[0x364];
    unsigned page[2][0x60];                             // +0x364 (0x180 bytes per page)
    char pad664[0x970 - 0x664];
    unsigned stash[1];                                  // +0x970
    char slot_flag(int page_idx, int slot, unsigned mask);   // 0x5126e0
};

int round_value(float* v);                              // 0x432540

// ---------------------------------------------------------------------------------------------------------
// @ 0x512620
char GameData::emit_event(int id, int a, int b, int c)
{
    __int64 vals[16];
    vals[0] = a;
    vals[1] = b;
    vals[2] = c;
    unsigned tag;
    if (f1a068 && g_emitters[id].name.equals(name))
        tag = (id << 16) | f1a07c;
    else
        tag = (unsigned)-1;
    char r = 0;
    int count = 3;
    if (g_emitter_idx[id] >= 0)
        r = g_emitters[g_emitter_idx[id]].sub.emit(0x200000, 0, &count, vals, tag);
    return r;
}

// @ 0x5126e0
char SlotWindow::slot_flag(int page_idx, int slot, unsigned mask)
{
    if (page_idx < 2)
        return (page[page_idx][slot] & mask) != 0;
    return (stash[page_idx * 10 + slot] & mask) != 0;
}

// @ 0x512730
void ItemRef::name(char* out)
{
    if (id < 0) {
        strcpy(out, "ERROR: INVALID ITEM");
        return;
    }
    int mod = ((signed char*)this)[7];
    ItemKind* it = &g_items[id];
    tstring* t = g_strtable.find(it->name);
    tstring nm(*(t ? t : (tstring*)it));
    nm.underscores_to_spaces();
    if (mod != 0) {
        const char* s = nm.c_str();
        Modifier* m = &g_modifiers[mod];
        tstring* mk = g_strtable.find(m->key);
        const char* fmt = (mk ? mk : (tstring*)m)->c_str();
        _snprintf(out, 0x80, fmt, s);
    } else {
        _snprintf(out, 0x80, "%s", nm.c_str());
    }
}

// @ 0x512840
void attribute_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_strength");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_agility");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_intelligence");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else {
        tstring k("ui_charisma");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

// @ 0x512960
void InvWindow::compute(int* item, int mode)
{
    char trade = 0;
    char grid = 0;
    if (fabc != 0 || f4 == 1 || f4 == 3 || f4 == 4)
        trade = 1;
    if (f4 == 2)
        grid = 1;
    char plain;
    if (mode != 1)
        plain = 1;
    else
        plain = 0;
    g_game->price((ItemRef*)item, troop, plain, trade, grid);
}

// @ 0x5129d0
int GameData::price(ItemRef* item, int idx, char adjust, char plain, char scaled)
{
    if (item->id < 0)
        return 0;
    if (g_items[item->id].value <= 0)
        return 0;

    PartyPool* pool = &parties;
    int pi = pool->find_index(f78);
    Party* p = (Party*)pool->pages[pi >> 4] + (pi & 0xf);
    int skill = p->skill(0);
    float f = (1.0f - (float)skill * 0.06f) * 1.25f;
    if (adjust)
        f = f * 1.9f;
    if (g_items[item->id].type == 0xb) {
        int skill2 = pool->find(f78)->skill(0);
        f = 0.25f - (float)skill2 * 0.01f;
    }

    float value = (float)g_items[item->id].value;
    if (g_items[item->id].flags & 0x2000000) {
        int n = item->f4 & 0xff;
        int c = item->max_amount();
        value = (float)(c - n) / (float)item->max_amount() * value;
    }
    int mod = ((signed char*)item)[7];
    if (mod > 0)
        value = value * g_sort_val[mod * 10];

    int result;
    if (plain) {
        result = (int)value;
    } else {
        g_result = 100;
        int mode = adjust ? 4 : 3;
        if (pick_event(mode, item->id, (int)value)) {
            result = (int)((float)g_result * value * 0.01f);
        } else {
            if (scaled)
                value = value * g_items[item->id].f21c;
            int row = *(int*)(chars + idx * 0x508 + 0x4c);
            float tf = *(float*)(char_data + row * 0x300 + 0x2c);
            if (tf < -1.0f)
                tf = -1.0f * 3.0f;
            else {
                if (tf > 1.0f)
                    tf = 1.0f;
                else if (tf < 0.0f)
                    tf *= 3.0f;
                tf *= 0.5f;
            }
            tf = (1.0f - tf) * f + 1.0f;
            if (adjust)
                tf = 1.0f / tf;
            value = tf * value;
            result = round_value(&value);
        }
    }
    if (result < 1)
        result = 1;
    return result;
}
