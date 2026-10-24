// Slice s0051be20: the party tooltip helpers of the game object at 0x5e2930.
//   0x51be20  bool dist_is_zero(o)         -- the 2D distance between two points of `o` is 0
//   0x51be60  Game::counts(id, a, b, c)    -- walk a party's stacks and tally regular / hero / wounded
//   0x51c090  draw_party_line(index)       -- the map tooltip for party record `index` (hours, size, ...)
//   0x51c5a0  Panel::update(arg)           -- format the panel's counter, refresh its text, switch screens
// The party manager is at game+0xf8 (pages at +0x138d0, next id at +0x138dc); the party records are
// 0x5ac8 bytes, stacks of 0x20 at +0xa8 (count, troop, flags, spawned, wounded).  Names are provisional.
#include <windows.h>
#include <stdio.h>
#include <math.h>

// ---- rgl string (16 bytes) ---------------------------------------------------------------------------
#include "../tstring.h"

#include "../str_table.h"
extern StrTable g_strtable;                             // 0x5e2a18

struct Gui {
    int draw_string(const tstring* s, float y, int color, char b);   // 0x4e43e0
};
extern Gui g_gui;                                       // 0x5d7080

struct LogStream {                                      // the engine text log at 0x5a9f60 (slot 10 = w_cstr)
    virtual void l00(); virtual void l01(); virtual void l02(); virtual void l03(); virtual void l04();
    virtual void l05(); virtual void l06(); virtual void l07(); virtual void l08(); virtual void l09();
    virtual void w_cstr(const char* s);                 // +0x28
};
struct MainMenu { void choose(int option); };           // 0x495700 (thiscall, ret 4)
struct FlagBlock { void apply(void* p); };              // 0x513710 (thiscall on 0x5e291c, ret 4)

// ---- party records -----------------------------------------------------------------------------------
#include "../../include/party.h"
#include "../../include/troop.h"


struct PartyInfo {                                      // 0x5c bytes, vector at game+0x13a64
    char pad0[0x14];
    float f14;                                          // +0x14
    int flags;                                          // +0x18 (0x10000000 = battle, low byte = size)
    char pad1c[0x5c - 0x1c];
};
struct PartyCellRec {                                        // 0xb0 bytes at 0x5e29f8
    char pad0[0x10];
    tstring name;                                       // +0x10
    char pad20[0x2c - 0x20];
    int f2c;                                            // +0x2c
    char pad30[0xb0 - 0x30];
};
extern PartyCellRec* g_slotrecs;                             // 0x5e29f8
extern int g_flag_a181b0;                               // 0xa181b0

struct Game {                                           // the game object (0x5e2930)
    char pad0[0x78];
    int main_party;                                     // +0x78
    char pad7c[0xf8 - 0x7c];
    PartyMgr mgr;                                       // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                                      // +0x13a00
    char pad13a04[0x13a48 - 0x13a04];
    float* rel;                                         // +0x13a48 (0xc0 floats per troop)
    char pad13a4c[0x13a64 - 0x13a4c];
    PartyInfo* party_info;                              // +0x13a64
    bool agent_low(int idx);                            // 0x4638f0 (thiscall, ret 4)
    void counts(unsigned id, int* a, int* b, int* c);   // 0x51be60
};
extern Game* g_game;                                    // 0x5e2930

unsigned __stdcall tint_color(float t, int flag);       // 0x4b1930

// ---- the two points the distance test reads -----------------------------------------------------------
struct DistObj {
    char pad0[0x30];
    float x, y;                                         // +0x30, +0x34
    char pad38[0x964 - 0x38];
    float px, py;                                       // +0x964, +0x968
};

// @ 0x51be20
bool __fastcall dist_is_zero(DistObj* o)
{
    float dx = o->x - o->px;
    float dy = o->y - o->py;
    float d = dx * dx;
    d += dy * dy;
    return sqrt(d) == 0.0f;
}

// @ 0x51be60
void Game::counts(unsigned id, int* num, int* num2, int* wounded)
{
    *num = 0;
    *num2 = 0;
    *wounded = 0;
    if (id >= (unsigned)mgr.n_parties)
        return;
    int idx = mgr.index_of(id);
    if (idx < 0)
        return;
    Party* p = (Party*)((char*)mgr.pages._Myfirst[(unsigned)idx >> 4] + (idx & 0xf) * 0x5ac8);
    if (p->type != 1)
        return;
    for (int i = 0; i < mgr.get_party(id)->nstacks; i++) {
        if (mgr.get_party(id)->stacks[i].flags & 1) {
            *wounded += mgr.get_party(id)->stacks[i].count;
        } else {
            int t = mgr.get_party(id)->stacks[i].troop;
            if (troops[t].flags30s & 0x10) {
                (*num)++;
                if (agent_low(mgr.get_party(id)->stacks[i].troop))
                    (*num2)++;
            } else {
                *num += mgr.get_party(id)->stacks[i].count;
                *num2 += mgr.get_party(id)->stacks[i].wounded;
            }
        }
    }
}

// @ 0x51c090
int __stdcall draw_party_line(int index)
{
    int skill = g_game->mgr.get_party(g_game->main_party)->skill_level(0x10);
    PartyInfo* info = &g_game->party_info[index];
    int flags = info->flags;
    if (flags & 0x10000000) {
        tstring k("ui_battleground");
        g_gui.draw_string(g_strtable.lookup(&k), 0.02f, 0xfff0e0d0, 1);
    } else {
        tstring k("ui_tracks");
        g_gui.draw_string(g_strtable.lookup(&k), 0.02f, 0xfff0e0d0, 1);
    }

    char buf[512];
    if (skill > 1) {
        int hours = (int)info->f14;
        int step = (2 - skill) * 2;
        int div = hours / 3 + step;
        if (div < 1)
            div = 1;
        else if (div > 10)
            div = 10;
        int lo = (hours / div) * div;
        int hi = div - 1 + lo;
        if (lo < 1)
            lo = 1;
        else if (lo > 0x20)
            lo = 0x20;
        if (hi < 1)
            hi = 1;
        else if (hi > 0x20)
            hi = 0x20;
        if (lo == hi) {
            if (lo < 2) {
                tstring k("ui_1_hour");
                sprintf(buf, g_strtable.lookup(&k)->c_str());
            } else {
                tstring k("ui_n_hours");
                sprintf(buf, g_strtable.lookup(&k)->c_str(), lo);
            }
        } else {
            tstring k("ui_between_hours");
            sprintf(buf, g_strtable.lookup(&k)->c_str(), lo, hi);
        }
        {
            tstring s(buf);
            g_gui.draw_string(&s, 0.0249f, 0xfff0f0f0, 1);
        }

        int size = info->flags & 0xff;
        int div2 = size / 3 + step;
        if (div2 < 1)
            div2 = 1;
        else if (div2 > 10)
            div2 = 10;
        int lo2 = (size / div2) * div2;
        int hi2 = div2 - 1 + lo2;
        if (lo2 <= 0)
            lo2 = 1;
        if (hi2 <= 0)
            hi2 = 1;
        if (info->flags & 0x10000000) {
            tstring k("ui_combatants");
            sprintf(buf, g_strtable.lookup(&k)->c_str(), size);
        } else if (lo2 == hi2) {
            tstring k("ui_party_size");
            sprintf(buf, g_strtable.lookup(&k)->c_str(), size);
        } else {
            tstring k("ui_party_size_between");
            sprintf(buf, g_strtable.lookup(&k)->c_str(), lo2, hi2);
        }
        {
            tstring s(buf);
            g_gui.draw_string(&s, 0.0249f, 0xfff0f0f0, 1);
        }
    }

    if (skill > 3) {
        unsigned f = info->flags;
        if (!(f & 0x10000000)) {
            int n = (f >> 8) & 0xff;
            if (n != 0xff && n > 0) {
                if (skill == 4) {
                    if (n % 7 != 0)
                        return n / 7;
                } else if (skill == 5) {
                    if (n % 7 > 1)
                        return n / 7;
                } else if (skill == 6) {
                    if (n % 7 > 2)
                        return n / 7;
                } else if (skill == 7) {
                    if (n % 7 > 3)
                        return n / 7;
                } else if (skill == 8) {
                    if (n % 7 > 4)
                        return n / 7;
                } else if (skill == 9) {
                    if (n % 7 > 5)
                        return n / 7;
                } else if (skill >= 10) {
                    PartyCellRec* r = &g_slotrecs[n];
                    unsigned color;
                    if (g_flag_a181b0 != 0)
                        color = *(unsigned*)&g_game->rel[r->f2c * 0xc0 + 0xa];
                    else
                        color = tint_color(g_game->rel[r->f2c * 0xc0 + 0xb], 1);
                    const tstring* name = g_strtable.find((const tstring*)((char*)r + 0x10));
                    if (name == 0)
                        name = (const tstring*)r;
                    return g_gui.draw_string(name, 0.025f, color, 1);
                }
            }
        }
    }
}

// (0x51c5a0 Panel::update, the last function of the slice, belongs to the next object: see MIGRATION.md)
