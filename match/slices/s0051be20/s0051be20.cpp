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
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
    void assign(const char* s);                         // 0x42bcb0
    const char* c_str() const { return buf ? buf : (const char*)&buf; }
};

struct StrTable {
    tstring* lookup(const tstring* key);                // 0x43a1e0 (never null)
    tstring* find(const tstring* key);                  // 0x4c6010 (null when absent)
};
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
struct Stack {                                          // 0x20 bytes, array at party+0xa8
    int count;                                          // +0x00
    int troop;                                          // +0x04
    int flags;                                          // +0x08
    int spawned;                                        // +0x0c
    int wounded;                                        // +0x10
    int f14, f18, f1c;
};
struct Party {                                          // 0x5ac8 bytes
    char pad0[4];
    int state;                                          // +0x04 (1 = live)
    char pad8[0xa8 - 8];
    Stack stacks[64];                                   // +0xa8
    int nstacks;                                        // +0x8a8
    int skill(int n);                                   // 0x4b2370 (thiscall, ret 4)
};
struct PartyMgr {                                       // the manager at game+0xf8
    char pad0[0x138d0];
    char** pages;                                       // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    int next_id;                                        // +0x138dc
    int index_of(int id);                               // 0x48ad00 (thiscall, ret 4)
    Party* get(int id) { int i = index_of(id); return (Party*)(pages[i >> 4] + (i & 0xf) * 0x5ac8); }
};
struct Troop {                                          // 0x508 bytes, table at game+0x13a00
    char pad0[0x30];
    __int64 flags;                                      // +0x30 (0x10 = hero)
    char pad38[0x508 - 0x38];
};
struct PartyInfo {                                      // 0x5c bytes, vector at game+0x13a64
    char pad0[0x14];
    float f14;                                          // +0x14
    int flags;                                          // +0x18 (0x10000000 = battle, low byte = size)
    char pad1c[0x5c - 0x1c];
};
struct SlotRec {                                        // 0xb0 bytes at 0x5e29f8
    char pad0[0x10];
    tstring name;                                       // +0x10
    char pad20[0x2c - 0x20];
    int f2c;                                            // +0x2c
    char pad30[0xb0 - 0x30];
};
extern SlotRec* g_slotrecs;                             // 0x5e29f8
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
    if (id >= (unsigned)mgr.next_id)
        return;
    int idx = mgr.index_of(id);
    if (idx < 0)
        return;
    Party* p = (Party*)(mgr.pages[(unsigned)idx >> 4] + (idx & 0xf) * 0x5ac8);
    if (p->state != 1)
        return;
    for (int i = 0; i < mgr.get(id)->nstacks; i++) {
        if (mgr.get(id)->stacks[i].flags & 1) {
            *wounded += mgr.get(id)->stacks[i].count;
        } else {
            int t = mgr.get(id)->stacks[i].troop;
            if (troops[t].flags & 0x10) {
                (*num)++;
                if (agent_low(mgr.get(id)->stacks[i].troop))
                    (*num2)++;
            } else {
                *num += mgr.get(id)->stacks[i].count;
                *num2 += mgr.get(id)->stacks[i].wounded;
            }
        }
    }
}

// @ 0x51c090
int __stdcall draw_party_line(int index)
{
    int skill = g_game->mgr.get(g_game->main_party)->skill(0x10);
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
                    SlotRec* r = &g_slotrecs[n];
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

// ---- the panel of 0x51c5a0 ---------------------------------------------------------------------------
struct PanelText {                                      // vtable slot +0xa4 = set_text(text, s)
    virtual void t00(); virtual void t01(); virtual void t02(); virtual void t03(); virtual void t04();
    virtual void t05(); virtual void t06(); virtual void t07(); virtual void t08(); virtual void t09();
    virtual void t10(); virtual void t11(); virtual void t12(); virtual void t13(); virtual void t14();
    virtual void t15(); virtual void t16(); virtual void t17(); virtual void t18(); virtual void t19();
    virtual void t20(); virtual void t21(); virtual void t22(); virtual void t23(); virtual void t24();
    virtual void t25(); virtual void t26(); virtual void t27(); virtual void t28(); virtual void t29();
    virtual void t30(); virtual void t31(); virtual void t32(); virtual void t33(); virtual void t34();
    virtual void t35(); virtual void t36(); virtual void t37(); virtual void t38(); virtual void t39();
    virtual void t40();
    virtual void set_text(const char* text, const tstring& s);  // +0xa4
};
struct Panel {
    char pad0[4];
    int mode;                                           // +0x04
    char pad8[0x30 - 8];
    PanelText* text;                                    // +0x30
    char pad34[0x3c - 0x34];
    int counter;                                        // +0x3c
    tstring str;                                        // +0x40
    bool sub(int* counter, tstring* str, tstring* out); // 0x51c720
    void update(int arg);                               // 0x51c5a0
};

// @ 0x51c5a0
void Panel::update(int arg)
{
    char tmp;
    tstring s;
    char buf[0x100];

    bool ok = sub(&counter, &str, &s);
    _snprintf(buf, 0x100, "%d", counter);
    text->set_text(buf, s);

    if (ok) {
        int m = mode;
        if (m == 0) {
            ((LogStream*)0x5a9f60)->w_cstr("Switching to menu window...\n");
            *(int*)((char*)g_game + 0x13aec) = 0;
            *(int*)(*(int*)0xa739bc + 0x14) = 0;
            *(int*)(*(int*)0xa739dc + 0x18) = 0xf;
            ((MainMenu*)0xa73980)->choose(0x17);
            return;
        }
        if (m == 1) {
            ((MainMenu*)0xa73980)->choose(7);
            return;
        }
        if (m == 2) {
            *(int*)((char*)g_game + 0x13aec) = 2;
            *(int*)0x5e2924 = *(int*)((char*)g_game + 0x13aec);
            *(char*)0x5e291c = 1;
        } else {
            *(int*)((char*)g_game + 0x13aec) = 3;
            *(int*)0x5e2924 = *(int*)((char*)g_game + 0x13aec);
            *(char*)0x5e291c = 1;
        }
        *(char*)0x5e291e = 0;
        *(char*)0x5e291d = 0;
        *(int*)0x5e2920 = -1;
        ((FlagBlock*)0x5e291c)->apply(&tmp);
    }
    (void)arg;
}
