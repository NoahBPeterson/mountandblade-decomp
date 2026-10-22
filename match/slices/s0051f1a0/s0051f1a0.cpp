// Slice s0051f1a0 (batch ds1).
//   0x51f1a0  GameState::start_new_game -- the battle start-up: reset the global spawn/slider block, resync
//             the two tick clocks, randomize the sound bank and the gui timers, clear the party/troop/item
//             tables, read parties.txt into the party manager, reset the player character (name, skills,
//             attributes, equipment and weapon points), seed the 64x32 random table, then run 0x51f800.
//   0x51f7c0  DelayList::arm -- re-arm the gui timers of a 0x30-byte delay list (frand() * delay each).
// Names are provisional; every offset below is read off this slice's disassembly (the class layout
// agrees with s00463790's GameState and s004902a0's party manager).
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "rgl.h"

// ---- tstring (0x42bbd0 / 0x42bbe0 / 0x42bc40 / 0x42bc50) ---------------------------------------------
struct tstring {
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();                                      // 0x42bbd0 (ret 0)
    tstring(const char* s);                         // 0x42bbe0 (ret 4)
    ~tstring();                                     // 0x42bc40 (ret 0)
    tstring& operator=(const tstring& o);           // 0x42bc50 (ret 4)
};

void mb_error(const char* fmt, ...);                // 0x42cc80 (cdecl)

// ---- text streams (layout from s00416ef0 / s004d0c20 mbbin.h) ----------------------------------------
struct stream_root {
    virtual void s00();                             // 0
    virtual void s01();
    virtual stream_root& s02(stream_root* o);
    virtual stream_root& s03(float* v);
    virtual stream_root& s04(char* v);
    virtual stream_root& s05(unsigned __int64* v);
    virtual stream_root& s06(__int64* v);
    virtual stream_root& s07(unsigned* v);
    virtual stream_root& s08(int* v);
    virtual stream_root& s09(unsigned char* v);
    virtual stream_root& s10(const char* s);
    virtual stream_root& s11(const tstring* s);
    virtual stream_root& s12(void* v);
    virtual stream_root& s13(stream_root* o);
    virtual stream_root& r_float(float* v);         // 14
    virtual stream_root& r_bool(char* v);           // 15
    virtual stream_root& r_uint(unsigned* v);       // 16
    virtual stream_root& r_u64(unsigned __int64* v);  // 17
    virtual stream_root& r_i64(__int64* v);         // 18
    virtual stream_root& r_int(int* v);             // 19 (slot 0x4c)
    virtual stream_root& r_byte(char* v);           // 20 (slot 0x50)
    virtual stream_root& r_tstr(tstring* v);        // 21 (slot 0x54)
    virtual stream_root& s22(void* v);
    virtual bool at_end();
    virtual void set_autoflush(char f);
    virtual char get_autoflush();
    virtual int flush();
    tstring name;                                   // +4
    int state;                                      // +0x14
    char autoflush;                                 // +0x18
    int f1c;                                        // +0x1c
    tstring buf;                                    // +0x20
};

struct tstream3 : stream_root {                     // vtable 0x54ac88
    FILE* fp;                                       // +0x30
    char failed;                                    // +0x34
    tstream3()                                      // 0x4172d0
    {
        state = 0;
        autoflush = 0;
        f1c = 0;
        fp = 0;
        failed = 1;
    }
    bool open_read_text(const char* path);          // 0x415d60 (ret 4)
    stream_root& r_int(int* v);                     // 0x415930 (slot 19, ICF)
    stream_root& r_tstr(tstring* v);                // 0x415880 (slot 21, ICF)
};

// ---- game records ------------------------------------------------------------------------------------
struct TroopRec {                                   // 0x508 bytes
    tstring name;                                   // +0x00
    char   pad10[0x30 - 0x10];
    int    flags;                                   // +0x30 (0x8000 = has a body)
    char   pad34[0x58 - 0x34];
    int    level_at;                                // +0x58
    char   pad5c[0x68 - 0x5c];
    unsigned n0 : 4;                                // +0x68
    unsigned n1 : 4;
    unsigned n2 : 4;
    unsigned n3 : 4;
    unsigned n4 : 4;
    unsigned n5 : 4;
    unsigned n6 : 4;
    unsigned n7 : 4;
    char   pad6c[0x7a - 0x6c];
    unsigned char rank;                             // +0x7a (low nibble)
    char   pad7b[0x9c - 0x7b];
    int    f9c;                                     // +0x9c
    float  weapon_pts;                              // +0xa0
    int    hp;                                      // +0xa4
    int    hp_max;                                  // +0xa8
    int    xp_next;                                 // +0xac
    char   padb0[0x3b4 - 0xb0];
    int    f3b4;                                    // +0x3b4
    char   pad3b8[0x508 - 0x3b8];
    void   auto_equip(int a, int b, int c, int d);  // 0x4687e0 (ret 0x10)
    void   set_skill(int idx, int val);             // 0x48bf90 (ret 8)
    void   reset_anim();                            // 0x520270 (ret 0)
};

struct PartyRec {                                   // 0x5ac8 bytes
    char pad0[0x74];
    int  f74;                                       // +0x74
};

struct SkillCap {                                   // 0x44 bytes, table at 0x8ee3100
    char pad0[0x34];
    int  cap;                                       // +0x34
    char pad38[0x44 - 0x38];
};

// ---- gui delay list (0x51f7c0) -----------------------------------------------------------------------
struct gui_timer {                                  // 16 bytes (s0042c9b0/gui.h)
    int clock;                                      // +0
    int pad4;
    __int64 t;                                      // +8
    void set_delay(float seconds);                  // 0x42cd20 (thiscall, ret 4)
};
struct DelayItem { float delay; char pad[0x1c]; gui_timer timer; };   // 0x30 bytes
struct DelayList { int count; DelayItem* items; void arm(); };        // arm = 0x51f7c0

// std::vector<...>::clear (0x41f4b0), shared out-of-line instance.
struct Vec { void* v0; void* v1; void* v2; void clear(); };
struct TroopKind {                                  // 0xb0 bytes, the table at 0x5e29f8
    char pad0[0x34];
    int  f34;                                       // +0x34
    int  f38;                                       // +0x38
    int  f3c;                                       // +0x3c
    char pad40[0xa0 - 0x40];
    Vec  list;                                      // +0xa0
};
struct ItemKind {                                   // 0x238 bytes, the table at 0x5e29d0
    char pad0[0x30];
    Vec  list;                                      // +0x30
};

// 0xa70484: the free list of the spawn/slider block
struct Mgr {
    char  pad0[0x138d0];
    char** pages;                                   // +0x138d0 (page of 16 0x5ac8-byte parties)
    char  pad1[0x138f0 - 0x138d4];
    struct { int d[3]; void randomize(); } snd;     // +0x138f0 (snd_bank, 0x462790)
    DelayList delays;                               // +0x138fc
    int   f13904;                                   // +0x13904
    int  index_of(unsigned id);                     // 0x48ad00 (ret 4)
    void load(stream_root* s, int flag);            // 0x51eb90 (ret 8)
};

struct GameState {
    char      pad0[4];
    int       f4;                                   // +0x04 (seeded from rand())
    char      pad8[0x1c - 0x8];
    tstring   module;                               // +0x1c (see s00463790)
    char      pad2c[0x78 - 0x2c];
    int       party;                                // +0x78 (party handle being started)
    char      pad7c[0x8c - 0x7c];
    int       player;                               // +0x8c (the player's troop index)
    char      pad90[0xf8 - 0x90];
    Mgr       mgr;                                  // +0xf8
    TroopRec* troops;                               // +0x13a00
    int       troop_count;                          // +0x13a04
    char      pad13a08[0x13a34 - 0x13a08];
    int       f13a34;                               // +0x13a34
    char      pad13a38[0x13b18 - 0x13a38];
    int       tick_idx;                             // +0x13b18
    __int64   clock;                                // +0x13b20
    int       f13b28;                               // +0x13b28
    int       f13b2c;                               // +0x13b2c
    int       f13b30;                               // +0x13b30
    int       f13b34;                               // +0x13b34
    int       f13b38;                               // +0x13b38
    char      pad13b3c[0x13b60 - 0x13b3c];
    int       f13b60;                               // +0x13b60
    char      pad13b64[0x1405c - 0x13b64];
    int       rnd[0x40][0x20];                      // +0x1405c
    char      pad1605c[0x1a298 - 0x1605c];
    int       tick_idx2;                            // +0x1a298
    __int64   clock2;                               // +0x1a2a0
    const char* module_path(const char* sub);       // 0x4639d0 (ret 4)
    void start_new_game();                          // 0x51f1a0
    void more_start();                              // 0x51f800 (slice s0051f800)
};
extern GameState* g_game;                           // 0x5e2930

extern __int64 g_time[];                            // 0x5e2948 (tick table, 8 bytes per entry)
extern float   g_start_time;                        // 0x59bffc
extern int     g_mode;                              // 0x5a9fac
extern void*   g_main_menu[];                       // 0xa73980 (30 menu pointers)
extern char*   g_troop_tbl;                         // 0x5e29f8 (0xb0-byte records)
extern int     g_troop_tbl_n;                       // 0x5e29fc
extern char*   g_items;                             // 0x5e29d0 (0x238-byte item kinds)
extern int     g_item_count;                        // 0x5e29d4
extern SkillCap g_skill_tbl[];                      // 0x8ee3100 (s004375b0)
extern int     g_skin_cap;                          // 0x8ee3ac4 (rank/weapon-point clamp)
struct Animator {                                   // 0x8ed4f60 (s004f49d0)
    void reset(void* a, void* b);                   // 0x520520 (thiscall, ret 8)
};
extern Animator g_anim;                             // 0x8ed4f60

extern int  g_a70440;                               // 0xa70440
extern int  g_spawn_lo;                             // 0xa70448
extern int  g_spawn_hi;                             // 0xa7044c
extern int  g_a70450;                               // 0xa70450
extern int  g_a70454;                               // 0xa70454
extern int  g_result;                               // 0xa70458
extern int  g_store1;                               // 0xa7045c
extern int  g_a70460;                               // 0xa70460
extern int  g_a70474;                               // 0xa70474
extern int  g_a70478;                               // 0xa70478
extern int  g_a7047c;                               // 0xa7047c
extern void* g_a70484;                              // 0xa70484
extern void* g_a70488;                              // 0xa70488
extern void* g_a7048c;                              // 0xa7048c

float frand();                                      // 0x40e400 (returns st0)
int   rand();                                       // 0x536a9f (_rand)
void  operator delete(void* p);                     // 0x535712 (??3)

inline int clamp_rank(int v)
{
    return v < 0 ? 0 : (v > g_skin_cap ? g_skin_cap : v);
}

// @ 0x51f1a0 -----------------------------------------------------------------------------------------
void GameState::start_new_game()
{
    tstring a;                                      // [esp+0x28] -- first parties.txt string
    tstring b;                                      // [esp+0x18] -- second parties.txt string

    g_spawn_lo = -100000;                           // 0xa70448
    g_a70450 = -100000;                             // 0xa70450
    g_a70440 = 0;                                   // 0xa70440
    g_a70474 = -1;                                  // 0xa70474
    g_a70478 = 0;                                   // 0xa70478
    g_a7047c = -1;                                  // 0xa7047c
    g_spawn_hi = -1;                                // 0xa7044c
    g_a70454 = -1;                                  // 0xa70454
    g_result = 0;                                   // 0xa70458
    g_store1 = 0;                                   // 0xa7045c
    g_a70460 = 100;                                 // 0xa70460
    if (g_a70484)
        operator delete(g_a70484);
    g_a70484 = 0;
    g_a70488 = 0;
    g_a7048c = 0;

    g_time[1] = (__int64)g_start_time;

    mgr.snd.randomize();                            // 0x462790
    mgr.delays.arm();                               // 0x51f7c0

    clock = g_time[tick_idx] - 600000;
    f4 = rand();

    clock2 = g_time[tick_idx2];

    if (g_mode == 0) {
        for (int i = 0; i < 30; i++)
            *(int*)((char*)g_main_menu[i] + 8) = 0;
    }

    f13b28 = -1;
    f13b2c = -1;
    f13b30 = -1;
    f13b34 = -1;
    f13b38 = -1;
    f13b60 = -1;
    f13a34 = 0;

    tstream3 stream;
    {
        const char* path = module_path("parties.txt");
        if (!stream.open_read_text(path))
            mb_error("Unable to open %s", path);
        stream.r_tstr(&a);
        stream.r_tstr(&b);
        int version;
        stream.r_int(&version);
        stream.failed = 1;
        mgr.load(&stream, 0);
    }

    int slot = mgr.index_of(party);
    char* pr = mgr.pages[slot >> 4] + (slot & 0xf) * 0x5ac8;
    *(int*)(pr + 0x74) = 0x80;

    if (g_troop_tbl_n > 0) {
        for (int i = 0; i < g_troop_tbl_n; i++) {
            TroopKind* r = (TroopKind*)(g_troop_tbl + i * 0xb0);
            r->f34 = 0;
            r->f38 = 0;
            r->f3c = 0;
            r->list.clear();
        }
    }
    if (g_item_count > 0) {
        for (int i = 0; i < g_item_count; i++)
            ((ItemKind*)(g_items + i * 0x238))->list.clear();
    }

    if (troop_count > 0) {
        TroopRec* t = troops;
        for (int i = 0; i < troop_count; i++, t++) {
            if (t != troops)
                t->reset_anim();
            t->auto_equip(-1, 0, 0, 0);
            if (t->flags & 0x8000)
                g_anim.reset((char*)t + 0x3c0, (char*)t + 0x3e0);
        }
    }

    for (int i = 0; i < 0x40; i++) {
        for (int j = 0; j < 0x20; j++)
            rnd[i][j] = rand();
    }

    troops[player].name = "";

    if (g_mode != 0) {
        int a1 = g_skill_tbl[1].cap < 3 ? g_skill_tbl[1].cap : 3;
        troops[player].n1 = a1;
        int y = troops[player].level_at + 0x23 + clamp_rank(troops[player].rank & 0xf) * 2;
        if ((float)y < troops[player].weapon_pts)
            troops[player].weapon_pts =
                (float)(troops[player].level_at + 0x23 + clamp_rank(troops[player].rank & 0xf) * 2);
        int a2 = g_skill_tbl[2].cap < 2 ? g_skill_tbl[2].cap : 2;
        troops[player].n2 = a2;
        int z = troops[player].level_at + 0x23 + clamp_rank(troops[player].rank & 0xf) * 2;
        if ((float)z < troops[player].weapon_pts)
            troops[player].weapon_pts =
                (float)(troops[player].level_at + 0x23 + clamp_rank(troops[player].rank & 0xf) * 2);
    }

    troops[player].f9c = rand() % 50 + 0x6b;
    troops[player].f3b4 = 0;
    troops[player].weapon_pts = (float)(troops[player].level_at + 0x23 + clamp_rank(troops[player].rank & 0xf) * 2);
    troops[player].hp_max = 4;
    troops[player].hp = 4;
    troops[player].xp_next = 10;
    troops[player].xp_next = 10;

    for (int i = 0; i < 0x2a; i++)
        troops[player].set_skill(i, 0);

    more_start();                                   // 0x51f800
}

// @ 0x51f7c0 -----------------------------------------------------------------------------------------
void DelayList::arm()
{
    for (int i = 0; i < count; i++) {
        DelayItem* it = &items[i];
        it->timer.set_delay(frand() * it->delay);
    }
}
