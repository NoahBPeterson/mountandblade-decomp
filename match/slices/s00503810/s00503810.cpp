// Slice s00503810 (batch ds1): the party-window object's "read" half and its morale panel.
//   0x503810  PartyWindow::read(int which)  -- load one half of the exchange screen back from the
//             game state (owner char, party 0x7c/0x80 values, stack list + random-seed table).
//   0x503a50  PartyWindow::refresh_morale() -- morale label ("ui_terrible".."ui_excellent") plus the
//             starvation warning, pushed into the info widget.
// The object layout follows the verified neighbours s005324d0 (apply/swap_rows) and s00503ed0/s005063e0
// (party window). Names are provisional.
#include <windows.h>
#include <stdio.h>
#include <math.h>

// ---- engine string (16 bytes) ------------------------------------------------------------------------
struct tstring {
    int pos;                                                // +0x00
    char* buf;                                              // +0x04
    int cap;                                                // +0x08
    int len;                                                // +0x0c
    tstring();                                              // 0x42bbd0
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
    void append(const tstring& o);                          // 0x42c1e0
    void push_back(char c);                                 // 0x42be70
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(const tstring* key);                    // 0x43a1e0
};
extern StrTable g_strtable;                                 // 0x5e2a18

// ---- UI widget --------------------------------------------------------------------------------------
struct vector4 { float x, y, z, w; };

struct Widget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34();
    virtual void d35(); virtual void d36();
    virtual void get_xy(float* v);                          // +0x94
    virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_text(vector4* out, const tstring* s);  // +0xa4
    char pad0[0x124];
    tstring name;                                           // +0x128
};

// ---- troops / parties -------------------------------------------------------------------------------
struct Stack {                                              // 0x20 bytes, array at Party::stacks
    int count;                                              // +0x00
    int troop;                                              // +0x04
    int flags;                                              // +0x08
    int f0c;                                                // +0x0c
    int f10;                                                // +0x10
    int f14;                                                // +0x14
    int f18;                                                // +0x18
    int f1c;                                                // +0x1c
};
typedef char assert_stack[sizeof(Stack) == 0x20 ? 1 : -1];

struct Party {                                              // 0x5ac8-byte party record
    char  pad0[0xa8];
    Stack stacks[64];                                       // +0xa8
    int   nstacks;                                          // +0x8a8
    char  pad8ac[0x5ac8 - 0x8ac];
};

struct PartyMgr {
    int index_of(unsigned id);                              // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);                                     // 0x51ee50 (thiscall, ret 4)
    char pad4[0x138d0];
    Party** pages;                                          // +0x138d0
};
struct PartyMgrGet {                                        // 0x51ee50 returns a Party*
    char pad0[0xac];
    int fac;                                                // +0xac
};

struct CharRec {                                            // 0x508-byte troop record
    char pad0[0x3b4];
    int f3b4;                                               // +0x3b4
    char pad3b8[0x508 - 0x3b8];
};

struct GameData {
    char     pad0[0x78];
    int      main_party;                                    // +0x78
    char     pad7c[0x8c - 0x7c];
    int      player;                                        // +0x8c
    char     pad90[0xf8 - 0x90];
    PartyMgr party_mgr;                                     // +0xf8
    char     padp[0x13a00 - 0x139cc];
    CharRec* chars;                                         // +0x13a00
};
extern GameData* g_game;                                    // 0x5e2930

inline PartyMgr* party_mgr() { return (PartyMgr*)((char*)g_game + 0xf8); }

inline Party* party_at(PartyMgr* m, int idx)
{
    Party* page = m->pages[idx >> 4];
    return &page[idx & 0xf];
}

inline Party* party_index(int id)
{
    PartyMgr* m = party_mgr();
    return party_at(m, m->index_of(id));
}

// ---- the party window object ------------------------------------------------------------------------
struct Row { int v[8]; };
typedef char assert_row[sizeof(Row) == 0x20 ? 1 : -1];

struct PartyWindow {
    char pad0[4];
    int  f04;                                               // +0x04 command (2/3 = exchange with a char)
    char pad8[0x10 - 0x8];
    int  f10;                                               // +0x10 party id of the second half
    int  f14;                                               // +0x14 owner char of the second half
    char pad18[0x80 - 0x18];
    Widget* info;                                           // +0x80 info widget
    char pad84[0x8e8 - 0x84];
    Row  rows[2][64];                                       // +0x8e8
    int  nstacks[2];                                        // +0x18e8
    int  owner_troop[2];                                    // +0x18f0
    int  party_7c[2];                                       // +0x18f8
    int  party_80[2];                                       // +0x1900
    int  table[64][32];                                     // +0x1908

    void read(int which);                                   // 0x503810
    void refresh_morale();                                  // 0x503a50
};

// @ 0x503810
void PartyWindow::read(int which)
{
    int mainp = g_game->main_party;
    int owner = g_game->player;
    if (which != 0) {
        mainp = f10;
        if (f04 == 3 || f04 == 2)
            owner = f14;
        else
            owner = *(int*)((char*)party_mgr()->get(mainp) + 0xac);
    }
    if (owner >= 0)
        owner_troop[which] = g_game->chars[owner].f3b4;
    party_7c[which] = *(int*)((char*)party_index(mainp) + 0x7c);
    party_80[which] = *(int*)((char*)party_index(mainp) + 0x80);
    nstacks[which] = *(int*)((char*)party_index(mainp) + 0x8a8);
    if (nstacks[which] > 0) {
        int i = 0;
        Row* dst = rows[which];
        do {
            *(int*)((char*)party_index(mainp) + 0xb4 + i * 0x20) = 0;
            *dst = *(Row*)((char*)party_index(mainp) + 0xa8 + i * 0x20);
            i++;
            dst++;
        } while (i < nstacks[which]);
    }
    if (which == 0) {
        for (int i = 0; i < 64; i++)
            for (int j = 0; j < 32; j++)
                table[i][j] = ((int*)((char*)g_game + 0x1405c))[i * 32 + j];
    }
}

// @ 0x503a50
void PartyWindow::refresh_morale()
{
    float morale = 0.0f;
    morale = *(float*)((char*)party_index(g_game->main_party) + 0x7c);

    char text[0x1f0];

    sprintf(text,
            g_strtable.lookup(&tstring("ui_morale"))->c_str(),
            morale < 0.1f ? g_strtable.lookup(&tstring("ui_terrible"))->c_str()
          : morale < 0.2f ? g_strtable.lookup(&tstring("ui_very_low"))->c_str()
          : morale < 0.3f ? g_strtable.lookup(&tstring("ui_low"))->c_str()
          : morale < 0.4f ? g_strtable.lookup(&tstring("ui_below_average"))->c_str()
          : morale < 0.6f ? g_strtable.lookup(&tstring("ui_average"))->c_str()
          : morale < 0.7f ? g_strtable.lookup(&tstring("ui_above_average"))->c_str()
          : morale < 0.8f ? g_strtable.lookup(&tstring("ui_high"))->c_str()
          : morale < 0.9f ? g_strtable.lookup(&tstring("ui_very_high"))->c_str()
          : g_strtable.lookup(&tstring("ui_excellent"))->c_str());

    {
        tstring line(text);
        vector4 extents;
        info->set_text(&extents, &line);
    }

    float value = *(float*)((char*)party_index(g_game->main_party) + 0x80);
    if (!(value >= 0.1f))
        return;
    value *= 100.0f;

    {
        tstring fmt("ui_starving");
        float whole = floor(value);
        int n = (int)whole;
        if (!(value - whole <= 0.5f))
            n++;
        sprintf(text, g_strtable.lookup(&fmt)->c_str(), n);
    }

    {
        tstring line(" (");
        info->name.append(line);
    }
    {
        tstring line(text);
        info->name.append(line);
    }
    info->name.push_back(')');

    float pos[4];
    info->get_xy(pos);
}
