// Slice s005146f0: mission start-up.  A small "sub" record initializer, three mission-context variants
// that hand the launch arguments to the scene, the autosave notice, and the hero-table builder that
// precedes a battle.
//   0x5146f0  Sub3::init()                       (0x6c-byte record, four uninitialized w lanes)
//   0x5147e0  MissionCtx::start_scene(a,b,c)     (store +0x154/+0x158/+0x15c then Scene::init)
//   0x514830  autosave_notice()                  (gated on g_game+0xb4 == 0)
//   0x514890  MissionCtx::build_hero_table(a,b,c,d)
//   0x514c00  MissionCtx::start_battle(a,b,c,d)  (same scene init + dialog, plus g_game+0x13a08)
//   0x514cd0  MissionCtx::start_mission()        (party leader -> hero table -> start_battle wrapper)
// Layouts are taken from the neighbouring verified slices (s004b1b20 parties, s00513e10 mission start).
#include <windows.h>

// ---- records ----------------------------------------------------------------------------------------------
struct InvSlot {                                 // 0x20 bytes, array at party+0xa8
    int count;                                   // +0x00
    int item;                                    // +0x04
    int flags;                                   // +0x08
    int f0c;                                     // +0x0c
    int f10;                                     // +0x10
    int f14;                                     // +0x14
    int f18;                                     // +0x18
    int f1c;                                     // +0x1c
};

struct Party {                                   // 0x5ac8 bytes
    int f0;                                      // +0x00
    int type;                                    // +0x04
    char pad8[0xa0 - 0x08];
    char f_a0;                                   // +0xa0
    char padA1[0xa8 - 0xa1];
    InvSlot inv[0x40];                           // +0xa8
    int inv_count;                               // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};

struct PartyMgr {                                // g_game+0xf8
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                              // +0x138dc
    int index_of(int id);                        // 0x48ad00 (thiscall, ret 4)
};
inline Party* party_at(PartyMgr* m, int i) { return &m->pages[i >> 4][i & 0xf]; }

struct Troop {                                   // 0x508-byte troop record, array at g_game+0x13a00
    char pad0[0x30];
    unsigned flags;                              // +0x30 (bit 0x10 = hero)
    char pad34[0x508 - 0x34];
};

struct GameData {                                // 0x5e2930
    char pad0[0x78];
    int main_party;                              // +0x78
    char pad7c[0x8c - 0x7c];
    int f8c;                                     // +0x8c
    char pad90[0xb4 - 0x90];
    int f_b4;                                    // +0xb4 (0 while a game is running)
    char padB8[0xf8 - 0xb8];
    PartyMgr parties;                            // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                               // +0x13a00
    char pad13a04[0x13a08 - 0x13a04];
    int f13a08;                                  // +0x13a08
    char pad13a0c[0x13a50 - 0x13a0c];
    char* f13a50;                                // +0x13a50 (battle participant table)
    int f491f20(int party, int a, int b, int c);  // 0x491f20 (thiscall, ret 0x10)
};
extern GameData* g_game;                         // 0x5e2930

struct MissionCtx;
struct Scene {
    char pad0[0x5270];
    int f5270;                                   // +0x5270
    char pad5274[0x6558 - 0x5274];
    int f6558;                                   // +0x6558
    void init(int tmpl, int a2, int a3, int a4, MissionCtx* ctx, int a6, int a7);  // 0x5143f0 (ret 0x18)
    void fade(float a, int b);                   // 0x483f60 (ret 8)
    void start_setup();                          // 0x484590
};
extern Scene* g_scene;                           // 0x5e292c

struct MissionCtx {
    int party[2];                                // +0x00
    char pad8[0x1c - 0x08];
    int f1c;                                     // +0x1c
    char f20;                                    // +0x20
    char f21;                                    // +0x21
    char pad22[0x24 - 0x22];
    int f24;                                     // +0x24
    char pad28[0x138 - 0x28];
    int side;                                    // +0x138
    char pad13c[0x154 - 0x13c];
    int f154;                                    // +0x154
    int f158;                                    // +0x158
    int f15c;                                    // +0x15c
    char pad160[0x2c0 - 0x160];
    int f2c0;                                    // +0x2c0
    void start_scene(int a, int b, int c);       // 0x5147e0 (ret 0xc)
    void build_hero_table(int a, int b, int c, int d);   // 0x514890 (ret 0x10)
    void start_battle(int a, int b, int c, int d);       // 0x514c00 (ret 0x10)
    void start_mission();                        // 0x514cd0
};

// ---- the small 4-lane vector used by Sub3 (the 4th lane is never assigned by the constructor) ------------
struct Vec4 {
    float x, y, z, w;
    Vec4(float a, float b, float c) { x = a; y = b; z = c; }
};

struct Sub3 {                                    // 0x6c bytes (embedded in the team record)
    int f0;                                      // +0x00
    int f4;                                      // +0x04
    int f8;                                      // +0x08
    int fc;                                      // +0x0c
    Vec4 a;                                      // +0x10 (20,20,20,-)
    Vec4 b;                                      // +0x20 (0,0,0,-)
    int f30;                                     // +0x30 (-1)
    Vec4 c;                                      // +0x34 (0,0.001,0,-)
    int f44;                                     // +0x44 (1)
    int f48;                                     // +0x48 (1)
    float f4c;                                   // +0x4c (10)
    int f50;                                     // +0x50
    int f54;                                     // +0x54
    int f58;                                     // +0x58 (2)
    Vec4 d;                                      // +0x5c (20,20,20,-)
    void init();                                 // 0x5146f0
};

// ---- strings / messages -----------------------------------------------------------------------------------
struct tstring {                                 // 16 bytes
    int w[4];
    tstring(const char* s);                      // 0x42bbe0
    tstring(const tstring& o);                   // 0x42c110
    ~tstring();                                  // 0x42bc40
};
struct TStrTable {                               // the resource table at 0x5e2a18
    tstring* get(tstring* key, int id);          // 0x43a1e0 (ret 8)
};
struct MsgLog {                                  // object at 0x5d7080
    void add(tstring s);                         // 0x452c00 (ret 0x10)
};
struct DialogMgr {                               // object at 0x8ed50c8
    void start(int a, int b, int c, int d, int e);   // 0x4610b0 (ret 0x14)
};

void save_game(int id, int mode);                // 0x4e8980

// @ 0x5146f0
void Sub3::init()
{
    f0 = 0;
    f4 = 0;
    f8 = 0;
    fc = 0;
    Vec4 v(20.0f, 20.0f, 20.0f);
    a = v;
    f44 = 1;
    f48 = 1;
    c = Vec4(0.0f, 0.001f, 0.0f);
    d = v;
    f4c = 10.0f;
    f58 = 2;
    f30 = -1;
    b = Vec4(0.0f, 0.0f, 0.0f);
}

// @ 0x5147e0
void MissionCtx::start_scene(int a1, int a2, int a3)
{
    f154 = a1;
    f158 = a2;
    f15c = a3;
    g_scene->init(a1, a2, a3, f1c, this, f24, side);
}

// @ 0x514830
void autosave_notice()
{
    if (g_game->f_b4 != 0)
        return;
    {
        tstring s("ui_autosaving");
        ((MsgLog*)0x5d7080)->add(*((TStrTable*)0x5e2a18)->get(&s, -1));
    }
    save_game(-1, 0);
}

// @ 0x514890
void MissionCtx::build_hero_table(int a1, int a2, int a3, int a4)
{
    g_game->f13a08 = a1;
    char* t = g_game->f13a50;
    int off = 0x5a8;
    do {
        *(int*)(t + off) = -1;
        off += 8;
    } while (off < 0x7a8);
    *(int*)(t + 0x5a8) = g_game->f8c;
    *(int*)(t + 0x7a8) = 1;
    *(int*)(t + 0x5b0) = a1;
    *(int*)(t + 0x5b4) = a2;
    *(int*)(t + 0x7ac) = 1;

    PartyMgr* m = &g_game->parties;
    Party* p = party_at(m, m->index_of(g_game->main_party));
    if (1 < p->inv_count) {
        int n = 1;
        int oi = 0x5b8;
        int of = 0x7b0;
        InvSlot* s = &p->inv[1];
        do {
            int item = s->item;
            if ((g_game->troops[item].flags & 0x10) != 0 && item != a1) {
                *(int*)(t + oi) = item;
                *(int*)(t + of) = 1;
                oi += 8;
                of += 4;
            }
            n++;
            s++;
        } while (n < p->inv_count);
    }

    if (a3 > 0 && a3 != g_game->main_party) {
        if (1 < party_at(m, m->index_of(a3))->inv_count) {
            int oi = 0x630;
            int of = 0x7ec;
            int n = 1;
            for (; n < party_at(m, m->index_of(a3))->inv_count; n++) {
                int item = party_at(m, m->index_of(a3))->inv[n].item;
                if ((g_game->troops[item].flags & 0x10) != 0) {
                    *(int*)(t + oi) = item;
                    *(int*)(t + of) = 1;
                    oi += 8;
                    of += 4;
                }
            }
        }
    }

    g_scene->init(1, 1, 0, 0, this, f24, 0);
    g_scene->f6558 = a1;
    g_scene->f5270 = f2c0;
    f2c0++;
    g_scene->start_setup();
    g_scene->fade(0.1f, 1);
    g_scene->fade(0.1f, 1);
    g_scene->fade(0.1f, 1);
    ((DialogMgr*)0x8ed50c8)->start(a1, -1, a3, a4, 1);
}

// @ 0x514c00
void MissionCtx::start_battle(int a1, int a2, int a3, int a4)
{
    g_game->f13a08 = a1;
    f154 = a2;
    f158 = a3;
    f15c = a4;
    g_scene->init(a2, a3, a4, f1c, this, f24, side);
    g_scene->f5270 = f2c0;
    f2c0++;
    g_scene->start_setup();
    g_scene->fade(0.1f, 1);
    g_scene->fade(0.1f, 1);
    g_scene->fade(0.1f, 1);
    ((DialogMgr*)0x8ed50c8)->start(a1, -1, party[side], 0, 1);
}

// @ 0x514cd0
void MissionCtx::start_mission()
{
    int id = party[side];
    Party* p = party_at(&g_game->parties, g_game->parties.index_of(id));
    int v = g_game->f491f20(id, 0, 0, 0);
    build_hero_table(p->inv[0].item, v, id, 1);
}
