// Slice s00504ba0: the party-bar member fill (0x504ba0), the money value label (0x504d50) and the
// weekly-cost total (0x504e20).
// Names and layouts are provisional; every offset comes from the disassembly of this slice.
//   0x504ba0  party_menu::fill_member(int page, int type, int unused, int count)       (ret 0x10)
//   0x504d50  Mission::money_label(int type, int troop)                              (ret 8)
//   0x504e20  GameData::weekly_cost()                                                (ret)
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

struct Party;

// ---------------------------------------------------------------------------------------------------------
// tstring (16 bytes): read cursor, heap buffer, capacity, length.
struct tstring {
    int pos;                                            // +0
    char* buf;                                          // +4
    int cap;                                            // +8
    int len;                                            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    // c_str() is deliberately NOT out of line: the original inlines "buf ? buf : (char*)&buf" into every
    // caller (the address of the object's own buf field is the empty string).
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

// ---------------------------------------------------------------------------------------------------------
// Ini-file table (0x1000 entries of 0x2c bytes, key tstring +0, value tstring +0x1c, count at +0x2c000).
struct ini_entry {
    tstring key;                                        // +0
    char    pad10[0xc];                                 // +0x10
    tstring value;                                      // +0x1c
};

struct ini_file {
    ini_entry entries[0x1000];                          // +0
    int       count;                                    // +0x2c000
    int   find(const tstring& key);                     // 0x4bd5a0
    float get_float(const tstring& key)                 // 0x4bd5e0 (inlined into callers)
    {
        int i = find(key);
        if (i < 0)
            return 0.0f;
        return (float)atof(entries[i].value.c_str());
    }
};

ini_file g_ini;                                         // 0xa44240
struct string_table {
    char pad0[0x10];
    tstring* find(const tstring* key);                  // 0x43a1e0 (StrTable::lookup)
};
string_table g_strings;                                 // 0x5e2a18

// ---------------------------------------------------------------------------------------------------------
// A widget: 58 virtuals (data starts at +0xe8); the two floats a bar hands to set_pos/set_size are read
// straight out of the object with a plain mov, so they are integers here.
// Slot +0x58 = set_pos(const float*), +0x64 = set_size(const float*), +0xa4 = set_text(out, in).
struct WRet { int a, b; WRet(); };                       // 8-byte non-POD hidden-pointer return, never destroyed
struct widget {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void v58(const float* a);                   // +0x58
    virtual void v5c(); virtual void v60();
    virtual void v64(const float* ab);                  // +0x64
    virtual void v68(); virtual void v6c(); virtual void v70();
    virtual void v74(const int* v);
    virtual void v78();
    virtual void v7c(void* p);
    virtual void v80(int n, void* p);
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void v94();
    virtual void v98(const int* v);
    virtual void v9c(); virtual void va0();
    virtual WRet va4(const tstring& s);                 // +0xa4 set_text
    virtual void va8();
    virtual void vac(tstring* out, const float* pos);
    virtual void vb0(); virtual void vb4(); virtual void vb8(); virtual void vbc();
    virtual void vc0(); virtual void vc4(); virtual void vc8(void* p);
    virtual void vcc(); virtual void vd0(); virtual void vd4(); virtual void vd8();
    virtual void vdc(); virtual void ve0(); virtual void ve4();
    void quarter_update();                              // 0x488d50 (thiscall, no stack args)
    char   pad4[0xe8 - 4];
    int    f_e8;                                        // +0xe8 (first float the bar reports)
    int    f_ec;                                        // +0xec
    int    f_f0;                                        // +0xf0 (second float: the bar's height)
};

// ---------------------------------------------------------------------------------------------------------
// The party menu. It is a sub-object of the game data at g_game + 0xf8, so every offset below is relative
// to that. +0xb8 is a 2x0x40 grid of rows (page * 0x100 + row * 0xc); +0x4b8 = the four per-type bar
// totals, +0x4c0 = their offsets, +0x4c8 = the next type's totals.
struct party_menu {
    char    pad0[0x98];
    widget* head[4];                                    // +0x98  indexed by page * 2 + type
    widget* tail[4];                                    // +0xa8  indexed by page * 2 + type
    widget* rows[4][0x40];                              // +0xb8  the bar rows, one 0x40-entry group per page * 2 + type
    float   amount[2];                                  // +0x4b8 per-type bar total (indexed by type only)
    float   start[2];                                   // +0x4c0 per-type bar offset
    void fill_member(int page, int type, int unused, int count);   // 0x504ba0
};

// ---------------------------------------------------------------------------------------------------------
// GameData (g_game at 0x5e2930): party list index +0x78, PartyMgr +0xf8, page table +0x139c8,
// troop records +0x13a00 (0x508 bytes each, flags at +0x30).
struct party_rec {
    char pad0[0x30];
    int  flags;                                         // +0x30
    char pad34[0x508 - 0x34];
};

struct party_mgr {
    int index_of(int id);                               // 0x48ad00
};

struct GameData {
    char       pad0[0x78];
    int        player_party;                            // +0x78
    char       pad7c[0xf8 - 0x7c];
    party_menu menu;                                    // +0xf8
    char       pad4d0[0x139c8 - 0xf8 - sizeof(party_menu)];
    int**      pages;                                   // +0x139c8
    char       pad13cc[0x13a00 - 0x139c8 - 4];
    party_rec* troops;                                  // +0x13a00
    int  sl_query(int a, int b, int c);                 // 0x435790
    int  sl_party_value(int member);                    // 0x48a290
    tstring string_value(int v);                        // 0x48e9e0 (hidden return)
    int  weekly_cost();                                 // 0x504e20
};

// ---------------------------------------------------------------------------------------------------------
// The Mission party window: only the label this slice touches (+0x5c) is named.
struct Mission {
    char    pad0[0x5c];
    widget* footer;                                     // +0x5c
    void money_label(int type, int troop);              // 0x504d50
};

extern GameData* g_game;                                // 0x5e2930
extern int g_result;                                    // 0xa70458
extern int g_result2;                                   // 0xa7045c
extern float g_grow_frac;                               // 0x54a6e4 (0.2f)


// @ 0x504ba0
void party_menu::fill_member(int page, int type, int unused, int count)
{
    (void)unused;
    float unit;
    {
        tstring key("party_b_member_h");
        unit = 0.0f;
        int n = g_ini.find(key);
        if (n >= 0)
            unit = (float)atof(g_ini.entries[n].value.c_str());
    }
    float total = (float)count * unit;
    if (total < amount[type]) {
        total += unit * g_grow_frac;
        if (!(total < amount[type]))
            total = amount[type];
    }

    float p[2];                     // the one float pair every set_pos / set_size call below reuses
    widget** row = rows[type + page * 2];
    for (int i = 0; i < count; i++, row++) {
        widget* bar = *row;
        *(int*)&p[0] = bar->f_e8;
        p[1] = total - (float)(i + 1) * unit;
        bar->v58(p);
    }

    widget* a = tail[type + page * 2];
    *(int*)&p[0] = a->f_f0;
    p[1] = total;
    a->v64(p);

    float x = (amount[type] < total) ? amount[type] : total;
    widget** pb = &head[type + page * 2];
    *(int*)&p[0] = (*pb)->f_f0;
    p[1] = x;
    (*pb)->v64(p);

    *(int*)&p[0] = (*pb)->f_e8;
    p[1] = start[type] + amount[type] - x;
    (*pb)->v58(p);

    (*pb)->quarter_update();
}

// @ 0x504d50
void Mission::money_label(int type, int troop)
{
    if (type == 2 || type != 0)
        return;

    char buf[512];
    {
        tstring key("ui_money_value");
        sprintf(buf, g_strings.find(&key)->c_str(), g_game->string_value(troop).c_str());
    }
    {
        tstring text(buf);
        footer->va4(text);
    }
}

// ---------------------------------------------------------------------------------------------------------
// The Mission thunk 0x504ba0 forwards into the party menu (see the note above the marker):
//     mov ecx, [ecx + 0x5c]   ; page = the Mission's widget/anchor at +0x5c
//     push edx                ; buffer
//     push eax                ; count
//     jmp 0x5966c0            ; party_menu::fill_member(??)
// Our reconstruction below is the menu method itself; when the menu object is laid out with the widget
// anchor as its first field, Mission's ecx is exactly this pointer, which is why the original keeps ESI
// (= ecx) alive across the whole body instead of reloading `this`.

// @ 0x504e20
int GameData::weekly_cost()
{
    g_result = 0;
    g_result2 = 0;
    int total = 0;
    if (sl_query(8, 0, 0))
        return g_result;

    party_mgr* pm = (party_mgr*)((char*)this + 0xf8);
    int id = pm->index_of(player_party);
    int* blk = pages[id >> 4];
    char* rec = (char*)blk + (id & 0xf) * 0x5ac8;
    int cnt = *(int*)(rec + 0x8a8);

    int* row = (int*)(rec + 0xcc);
    for (int i = 1; i < cnt; i++) {
        if (row[1] & 1)
            break;
        int member = row[0];
        int v = sl_party_value(member);
        if ((troops[member].flags & 0x10) == 0)
            v *= row[-1];
        total += v;
        row += 8;
    }
    return total;
}
