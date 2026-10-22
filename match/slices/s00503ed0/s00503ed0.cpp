// Slice s00503ed0: Mission::refresh_party_members (provisional name) -- rebuild one half of the
// party/troop exchange screen: clear the 2x64 slot widgets of that half, mark the rows that exist
// with an empty label, walk the selected party's stacks, build each row's label ("<name> <n>/<m>",
// hero percentages, ...), pick the per-row colour out of the settings table and push everything into
// the widgets, then refresh the "company" / "prisoners" totals of the footer.
//
// Every offset comes from the disassembly of 0x503ed0; the engine layouts (tstring, Widget vtable
// slots, ini table, Party/Stack/PartyMgr, GameData, Mission) follow the verified neighbouring slices
// (s005063e0, s00531800, s004bd170, s004893b0, s0043a280).
#include <windows.h>
#include <stdio.h>

// ---- engine string (16 bytes) ------------------------------------------------------------------------
struct tstring {
    int pos;                                                // +0x00
    char* buf;                                              // +0x04
    int cap;                                                // +0x08
    int len;                                                // +0x0c
    tstring();                                              // 0x42bbd0
    tstring(const char* s);                                 // 0x42bbe0
    ~tstring();                                             // 0x42bc40
    void assign(const char* s);                             // 0x42bcb0
    void append(const tstring& o);                          // 0x42c1e0
    char equals(const tstring& o) const;                    // 0x42bdd0 (returns a char)
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* find(tstring* key);                            // 0x4c6010
    tstring* lookup(tstring* key);                          // 0x43a1e0
};
extern StrTable g_strtable;                                 // 0x5e2a18

// ---- settings table at 0xa44240 (0x2c-byte entries, value tstring at +0x1c) --------------------------
struct ini_entry {
    tstring key;                                            // +0x00
    char    pad10[0x0c];                                    // +0x10
    tstring value;                                          // +0x1c
};
struct ini_file {
    ini_entry entries[0x1000];                              // +0x00000
    int       count;                                        // +0x2c000 (0xa70240)
};
extern ini_file g_config;                                   // 0xa44240

// ---- the 16-byte scratch the widget's set_text slot writes the text extents into --------------------
struct vector4 { float x, y, z, w; };

// ---- UI widget --------------------------------------------------------------------------------------
struct Widget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34();
    virtual void d35(); virtual void d36(); virtual void d37();
    virtual void set_colour(int* colour);                   // +0x98
    virtual void d39(); virtual void d40();
    virtual void set_text(vector4* out, const tstring* s);  // +0xa4
    virtual void d42(); virtual void d43();
    virtual void clear();                                   // +0xb0
    virtual void act_b4();                                  // +0xb4
    virtual void enable();                                  // +0xb8
    virtual void act_bc();                                  // +0xbc
};

// ---- troops / parties -------------------------------------------------------------------------------
struct TroopRec {                                           // 0x508 bytes
    tstring id;                                             // +0x00
    char    pad10[0x10];                                    // +0x10
    tstring name;                                           // +0x20
    int     flags;                                          // +0x30
    char    pad34[0x58 - 0x34];
    int     f58;                                            // +0x58
    char    pad5c[0x7a - 0x5c];
    unsigned char f7a;                                      // +0x7a
    char    pad7b[0xa0 - 0x7b];
    float   fa0;                                            // +0xa0
    int     fa4;                                            // +0xa4
    int     fa8;                                            // +0xa8
    char    padac[0x3b4 - 0xac];
    int     f3b4;                                           // +0x3b4
    char    pad3b8[0x508 - 0x3b8];
};

struct Stack {                                              // 0x20 bytes, array at Party+0xa8
    int count;                                              // +0x00
    int troop;                                              // +0x04
    int flags;                                              // +0x08
    int f0c;                                                // +0x0c
    int f10;                                                // +0x10
    int f14;                                                // +0x14
    int f18;                                                // +0x18
    int f1c;                                                // +0x1c
};

struct Party {                                              // 0x5ac8 bytes
    char  pad0[0xa8];
    Stack stacks[64];                                       // +0xa8
    int   nstacks;                                          // +0x8a8
    char  pad8ac[0x5ac8 - 0x8ac];

    int ext_4b1700();                                       // 0x4b1700 (rows this half can show)
    int sum_head();                                         // 0x4894b0 (sum of the unflagged prefix)
    int sum_flagged();                                      // 0x4894e0 (sum of the flagged stacks)
    int regular_limit();                                    // 0x4b22d0
    int prisoner_limit();                                   // 0x4b2330
};

struct PartyMgr {
    int index_of(int id);                                   // 0x48ad00 (packed index >>4 / &0xf)
    char pad4[0x138d0];
    Party** pages;                                          // +0x138d0
};

struct GameData {
    char     pad0[0x78];
    int      player_party;                                  // +0x78
    char     pad7c[0x8c - 0x7c];
    int      player;                                        // +0x8c
    char     pad90[0xf8 - 0x90];
    PartyMgr party_mgr;                                     // +0xf8
    char     padp[0x13a00 - 0x139cc];
    TroopRec* troops;                                       // +0x13a00

    int weekly_cost();                                      // 0x504e20
    tstring string_value(int value);                        // 0x48e9e0
    bool agent_low(int troop);                              // 0x4638f0
};
extern GameData* g_game;                                    // 0x5e2930

// clamped level value used by the hero percentage (lives outside the loaded image)
#define g_clamp_max (*(int*)0x8ee3ac4)

// ---- mission ----------------------------------------------------------------------------------------
struct Mission {
    char pad0[4];
    int  f04;                                               // +0x04
    char pad8[0x10 - 0x8];
    int  f10;                                               // +0x10 (party id of the second half)
    char pad14[0x64 - 0x14];
    Widget* w64;                                            // +0x64 (weekly cost label)
    Widget* w68;                                            // +0x68 (company label)
    Widget* w6c;                                            // +0x6c (prisoners label)
    char pad70[0xb8 - 0x70];
    Widget* grid[2][2][64];                                 // +0xb8 [type][row][slot]
    char pad4b8[0x4c8 - 0x4b8];
    int slots[2][2][64];                                    // +0x4c8
    int f8c8;                                               // +0x8c8
    char pad8cc[0x8d8 - 0x8cc];
    char f8d8[2];                                           // +0x8d8 (show the healthy rows)
    char f8da[2];                                           // +0x8da (show the wounded rows)
    int f8dc;                                               // +0x8dc
    int f8e0;                                               // +0x8e0
    int f8e4;                                               // +0x8e4

    void fn_504ba0(int type, int kind, Party* party, int count);  // 0x504ba0
    void fn_504d50(int type, int troop);                          // 0x504d50
    void refresh_party_members(int type);                         // 0x503ed0
};

// ---- helpers ----------------------------------------------------------------------------------------
static Party* get_party(int id)
{
    PartyMgr* m = &g_game->party_mgr;
    int i = m->index_of(id);
    return &m->pages[i >> 4][i & 0xf];
}

// the troop's localised display name: its name record when the string table knows it, else its id
static const char* display_name(TroopRec* rec)
{
    tstring* nm = g_strtable.find(&rec->name);
    if (!nm)
        nm = (tstring*)rec;
    return nm->c_str();
}

// push a label into a widget (slot 0xa4 writes the text extents into a scratch vector4)
static void push_text(Widget* w, const tstring* s)
{
    vector4 out;
    w->set_text(&out, s);
}

static void push_empty(Widget* w)
{
    tstring s("");
    push_text(w, &s);
}

static void append_plus(tstring& s)
{
    tstring plus(" +");
    s.append(plus);
}

// read a "%x" colour out of the settings table entry named by key and hand it to the widget
static __forceinline void push_colour(Widget* w, const char* key)
{
    tstring k(key);
    int colour = 0;
    int found = -1;
    for (int i = 0; found == -1 && i < g_config.count; i++) {
        if (g_config.entries[i].key.equals(k))
            found = i;
    }
    if (found >= 0)
        sscanf(g_config.entries[found].value.c_str(), "%x", &colour);
    w->set_colour(&colour);
}

// @ 0x503ed0
void Mission::refresh_party_members(int type)
{
    if ((f04 == 0 || f04 == 5) && type == 1)
        return;

    int party_id = g_game->player_party;
    if (type != 0)
        party_id = f10;

    // wipe the two rows of this half
    for (int i = 0; i < 64; i++) {
        grid[type][0][i]->clear();
        grid[type][1][i]->clear();
    }
    for (int i = 0; i < 64; i++) {
        slots[type][0][i] = -1;
        slots[type][1][i] = -1;
    }

    // first wounded stack: the wounded rows are shown compacted from there on
    int first_wounded = 0;
    {
        Party* p = get_party(party_id);
        for (int i = 0; i < p->nstacks; i++) {
            if (p->stacks[i].flags & 1)
                break;
            first_wounded++;
        }
    }

    int healthy = 0;
    int wounded = 0;
    char buf[512];

    if (party_id != -1) {
        // label every row that exists with an empty string
        for (int i = 0; i < get_party(party_id)->ext_4b1700(); i++) {
            push_empty(grid[type][0][i]);
            grid[type][0][i]->enable();
        }
        for (int i = 0; i < get_party(party_id)->ext_4b1700(); i++) {
            push_empty(grid[type][1][i]);
            grid[type][1][i]->enable();
        }

        int negfirst = -first_wounded;

        for (int i = 0; i < get_party(party_id)->nstacks; i++, negfirst++) {
            Stack* st = &get_party(party_id)->stacks[i];
            bool is_wounded = (st->flags & 1) != 0;

            bool include = (f8d8[type] && !is_wounded) || (f8da[type] && is_wounded);
            if (!include)
                continue;

            if (is_wounded)
                wounded++;
            else
                healthy++;

            tstring label;
            TroopRec* rec = &g_game->troops[st->troop];

            if (rec->flags & 0x10) {
                // hero: percentage of the (clamped) level denominator
                if (g_game->agent_low(st->troop)) {
                    {
                        tstring key("ui_button_party_member_hero_percentage_wounded");
                        int v = rec->f7a & 0xf;
                        if (v < 0)
                            v = 0;
                        else if (v > g_clamp_max)
                            v = g_clamp_max;
                        int pct = (int)(rec->fa0 * 100.0f / (float)(rec->f58 + v * 2 + 0x23) + 0.0001f);
                        sprintf(buf, g_strtable.lookup(&key)->c_str(), display_name(rec), pct);
                    }
                    label.assign(buf);
                    if (rec->fa8 != 0)
                        append_plus(label);
                } else {
                    {
                        tstring key("ui_button_party_member_hero_percentage");
                        int v = rec->f7a & 0xf;
                        if (v < 0)
                            v = 0;
                        else if (v > g_clamp_max)
                            v = g_clamp_max;
                        int pct = (int)(rec->fa0 * 100.0f / (float)(rec->f58 + v * 2 + 0x23) + 0.0001f);
                        sprintf(buf, g_strtable.lookup(&key)->c_str(), display_name(rec), pct);
                    }
                    label.assign(buf);
                    if (rec->fa8 != 0)
                        append_plus(label);
                }
            } else {
                if (st->count > 0 && !is_wounded) {
                    {
                        tstring key("ui_button_party_member_healthy_total");
                        sprintf(buf, g_strtable.lookup(&key)->c_str(), display_name(rec),
                                st->count - st->f10, st->count);
                    }
                    label.assign(buf);
                    if (st->f14 > 0)
                        append_plus(label);
                } else {
                    {
                        tstring key("ui_button_party_member_total");
                        sprintf(buf, g_strtable.lookup(&key)->c_str(), display_name(rec),
                                st->count);
                    }
                    label.assign(buf);
                    if (st->f14 > 0)
                        append_plus(label);
                }
            }

            int row = is_wounded ? 1 : 0;
            int col = is_wounded ? negfirst : i;

            push_text(grid[type][row][col], &label);
            if ((rec->flags & 0x10) && g_game->agent_low(st->troop))
                push_colour(grid[type][row][col], "party_b_member_t_c_wounded");
            else
                push_colour(grid[type][row][col], "party_b_member_t_c");
            grid[type][row][col]->act_b4();
            grid[type][row][col]->act_bc();
            slots[type][row][col] = i;
        }

        fn_504ba0(type, 0, get_party(party_id), healthy);
        fn_504ba0(type, 1, get_party(party_id), wounded);
    }

    if (type == 0)
        fn_504d50(0, g_game->troops[g_game->player].f3b4);

    if (type == 0) {
        int cost = g_game->weekly_cost();
        if (cost > 0) {
            {
                tstring key("ui_weekly_cost_value");
                tstring val = g_game->string_value(cost);
                sprintf(buf, g_strtable.lookup(&key)->c_str(), val.c_str());
            }
            tstring text(buf);
            push_text(w64, &text);
        } else {
            tstring text("");
            push_text(w64, &text);
        }

        {
            tstring key("ui_company");
            tstring* fmt = g_strtable.lookup(&key);
            sprintf(buf, fmt->c_str(), get_party(g_game->player_party)->sum_head(),
                    get_party(g_game->player_party)->regular_limit());
        }
        {
            tstring text(buf);
            push_text(w68, &text);
        }

        {
            tstring key("ui_prisoners_equal_value");
            tstring* fmt = g_strtable.lookup(&key);
            sprintf(buf, fmt->c_str(), get_party(g_game->player_party)->sum_flagged(),
                    get_party(g_game->player_party)->prisoner_limit());
        }
        {
            tstring text(buf);
            push_text(w6c, &text);
        }
    }
}
