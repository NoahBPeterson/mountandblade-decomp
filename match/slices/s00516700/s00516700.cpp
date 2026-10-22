// Slice s00516700: the per-frame agent-view reset (0x516700), the game's two id-building helpers
// (0x5168b0 clears the 0x60 trade ids, 0x5168e0 rebuilds a party's id grid; 0x5169e0 is the per-slot
// reset used by 0x516700) and the "quick saving" message (0x516a70).
// Layouts are provisional; every offset comes from this slice's disassembly.
#include <windows.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- shared .rdata constants ------------------------------------------------------------------------
extern const float g_half;          // 0x549d70  0.5f
extern const float g_three;         // 0x54a710  3.0f
extern int        g_max_level;      // 0x8ee3ac4

// ---- records -----------------------------------------------------------------------------------------
struct InvSlot {                    // 0x20 bytes, array at party+0xa8
    int count;                      // +0x00
    int item;                       // +0x04
    int f08;                        // +0x08
    int f0c;                        // +0x0c
    int f10;                        // +0x10
    int f14;                        // +0x14
    int f18;                        // +0x18
    int f1c;                        // +0x1c
};

struct Party {                      // 0x5ac8 bytes
    char pad0[0xa8];
    InvSlot inv[0x40];              // +0xa8  (0x20 each)
    int count;                      // +0x8a8
};

struct TradeSlot {                  // 8-byte slot at troop+0xb4
    int id;                         // +0x00
    int pad;                        // +0x04
};

struct Troop {                      // 0x508 bytes, array at g_game+0x13a00
    char pad0[0x30];
    __int64 flags;                  // +0x30
    char pad38[0x58 - 0x38];
    int  f58;                       // +0x58
    char pad5c[0x7a - 0x5c];
    char f7a;                       // +0x7a
    char pad7b[0xa0 - 0x7b];
    float f_a0;                     // +0xa0
    char pad_a4[0xb4 - 0xa4];
    TradeSlot trade[0x60];          // +0xb4
    char pad_3b4[0x508 - 0x3b4];
};

struct PartyMgr {                   // embedded in GameData at +0xf8
    char pad0[0x138d0];
    Party** pages;                  // +0x138d0 (absolute 0x139c8)
    int  index_of(int id);          // 0x48ad00
    Party* get(int id);             // 0x51ee50 (index_of + paging, inlined)
};

struct GameData {
    char pad0[0x78];
    int  cur_agent;                 // +0x78
    char pad7c[0x90 - 0x7c];
    int  f90;                       // +0x90
    char pad94[0xf8 - 0x94];
    PartyMgr parties;               // +0xf8
    char pad_a00[0x13a00 - 0xf8 - sizeof(PartyMgr)];
    Troop* troops;                  // +0x13a00
    char pad_a04[0x1605c - 0x13a04];
    int  ids_lo[0x800];             // +0x1605c
    int  ids_hi[0x800];             // +0x1805c
    void fill_ids(int* out, int id);            // 0x5168e0
    __declspec(noinline) void reset_trade_ids(); // 0x5168b0 (called, not inlined, by 0x516700)
};
extern GameData* g_game;            // 0x5e2930

int count_hero_items();             // 0x513e10

static Party* party_at(PartyMgr* m, int idx)
{
    char* page = *(char**)((char*)m->pages + (idx >> 4) * 4);
    return (Party*)(page + (idx & 0xf) * 0x5ac8);
}

// ======================================================================================================
struct Unit {
    int  f00;               // +0x00
    int  f04;               // +0x04
    char pad08[0x18 - 0x08];
    int  f18;               // +0x18
    int  f1c;               // +0x1c
    char f20;               // +0x20
    char f21;               // +0x21
    char pad22[0x34 - 0x22];
    char f34;               // +0x34
    char f35;               // +0x35
    char pad36[2];
    int  f38;               // +0x38
    char pad3c[0x138 - 0x3c];
    int  f138;              // +0x138
    char pad13c[0x164 - 0x13c];
    int  f164;              // +0x164
    int  f168;              // +0x168
    int  f16c;              // +0x16c
    int  f170;              // +0x170
    char pad174[0x274 - 0x174];
    int  f274;              // +0x274
    int  f278;              // +0x278
    char pad27c[0x2c0 - 0x27c];
    int  f2c0;              // +0x2c0
    void update_anim();             // 0x516700
    void reset_ids(int id);         // 0x5169e0
};

// @ 0x5169e0
void Unit::reset_ids(int id)
{
    if (id == -1)
        return;
    PartyMgr* m = &g_game->parties;
    Party* rec = party_at(m, m->index_of(id));
    for (int i = 0; i < rec->count; i++) {
        int slot = *(int*)((char*)rec + 0xac + i * 0x20);
        Troop* t = (Troop*)((char*)g_game->troops + slot * 0x508);
        if ((t->flags & 0x10) != 0) {
            *(int*)((char*)rec + 0xb8 + i * 0x20) = 0;
            *(int*)((char*)rec + 0xa8 + i * 0x20) = 1;
        }
    }
}

// @ 0x516700
void Unit::update_anim()
{
    f2c0 = 0;
    reset_ids(g_game->cur_agent);
    reset_ids(f00);
    reset_ids(f04);
    {
        int* ids = &f00;
        if (ids[f138] >= 0)
            g_game->fill_ids(g_game->ids_lo, ids[f138]);
        if (ids[1 - f138] >= 0)
            g_game->fill_ids(g_game->ids_hi, ids[1 - f138]);
    }
    if (f00 >= 0) {
        GameData* g = g_game;
        PartyMgr* m = &g->parties;
        Party* p = party_at(m, m->index_of(f00));
        int slot = p->inv[0].item;
        if (slot >= 0) {
            Troop* t = &g->troops[slot];
            if ((t->flags & 0x10) != 0) {
                int k = t->f7a & 0xf;
                if (k < 0)
                    k = 0;
                else if (k > g_max_level)
                    k = g_max_level;
                int n = t->f58 + k * 2 + 0x23;
                float g = (float)(int)((float)n * g_half + g_three);
                if (!(g <= t->f_a0))
                    t->f_a0 = g;
            }
        }
    }
    f168 = count_hero_items();
    f34 = 0;
    f35 = 0;
    f38 = 0;
    f18 = 0;
    f164 = 0;
    f20 = 0;
    f21 = 0;
    f16c = 0;
    f170 = 0;
    f274 = 0;
    f278 = 0;
    g_game->reset_trade_ids();
    f2c0 = 0;               // mirrors the original's trailing inline marker (blocks the tail call)
}

// ======================================================================================================
// @ 0x5168b0  clear the 0x60 (stride 8) trade ids of the record selected by f90
void GameData::reset_trade_ids()
{
    TradeSlot* p = &troops[f90].trade[0];
    for (int i = 0; i < 0x60; i++) {
        p->id = -1;
        p++;
    }
}

// @ 0x5168e0  rebuild a party's id grid (the 0x20-slot rows at party+0xa8)
void GameData::fill_ids(int* out, int id)
{
    PartyMgr* m = &parties;
    Party* rec = party_at(m, m->index_of(id));
    if (rec->count <= 0)
        return;
    int* base = out;
    for (int row = 0; row < rec->count; row++, base += 0x20) {
        int* o = base;
        for (int j = 0; j < rec->inv[row].count; j++, o++) {
            if (j >= 0x20)
                break;
            int v = -1;
            if (id >= 0)
                v = m->get(id)->inv[row].item;
            *o = v * 0x1bf + (id * 0x7bb7 + j) + 0x45db;
        }
    }
}

// ======================================================================================================
// The string table / message log used by 0x516a70 (16-byte tstring layout as in slice s0043a280).
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring(const char* s) throw();     // 0x42bbe0
    tstring(const tstring& s) throw();  // 0x42c110
    ~tstring() throw();                 // 0x42bc40
};
struct StrTable { tstring* lookup(const tstring& key); };            // 0x43a1e0 (ret 4), table 0x5e2a18
struct MsgLog { void add(tstring msg, int color); };                 // 0x452c00 (ret 0x14), object 0x5d7080
extern StrTable g_strtable;                                          // 0x5e2a18
int save_game(int arg, char* name);                                  // 0x4e8980 (cdecl)

// @ 0x516a70
void quick_save_msg()
{
    ((MsgLog*)0x5d7080)->add(*g_strtable.lookup(tstring("ui_quick_saving")), -1);
    save_game(-1, 0);
}
