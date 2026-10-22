// Slice s00513e10: mission-party bookkeeping helpers on the game data (0x5e2930) and on a mission
// context, plus the mission-template selection that starts a scene.
//   0x513e10  count_hero_items()                      (no args; main party at g_game+0x78)
//   0x513eb0  clear_hero_ammo(party)                   (this = g_game; manager at +0xf8)
//   0x513f80  setup(side flags, count)                 (mission context)
//   0x514360  pick_template()                          (mission context -> 0x5143f0 of g_scene)
//   0x5143f0  g_scene->init(tmpl, a, b, c, ctx, d, n)
//   0x5144d0  g_scene->init_scene()
//   0x514670  team_slot_init(idx)                      (8 team records at +0x527c, 0x16c each)
// Layouts are copied from the neighbouring verified slices (s004b1b20 parties/inventory, s004b4470 /
// s004b5c30 GameData at 0x5e2930, s004902a0 party manager).
#include <windows.h>
#include <stdlib.h>

// ---- records ----------------------------------------------------------------------------------------------
struct InvSlot {                                 // 0x20 bytes, array at party+0xa8 (s004b1b20)
    int count;                                   // +0x00
    int item;                                    // +0x04
    int flags;                                   // +0x08 (bit 0)
    int f0c;                                     // +0x0c
    int f10;                                     // +0x10 (zeroed for hero items by 0x513eb0)
    int f14;                                     // +0x14 (summed by 0x513e10)
    int f18;                                     // +0x18
    int f1c;                                     // +0x1c
};

struct Party {                                   // 0x5ac8 bytes
    int f0;                                      // +0x00
    int type;                                    // +0x04 (== 1 tests the party kind used here)
    char pad8[0xa0 - 0x08];
    char f_a0;                                   // +0xa0
    char padA1[0xa8 - 0xa1];
    InvSlot inv[0x40];                           // +0xa8
    int inv_count;                               // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
    int total_value(unsigned flags) const;        // 0x4b1cf0 (ret 4)
};

struct PartyMgr {                                // g_game+0xf8
    char pad0[0x138d0];
    Party** pages;                               // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                              // +0x138dc
    int index_of(int id);                        // 0x48ad00 (thiscall, ret 4)
    int is_slot(unsigned id);                    // 0x4903e0 (thiscall, ret 4)
    Party* get(int id);                          // 0x51ee50 (thiscall, ret 4)
};
inline Party* party_at(PartyMgr* m, int i) { return &m->pages[i >> 4][i & 0xf]; }
__forceinline Party* party_get(PartyMgr* m, int i) { return m->pages[i >> 4] + (i & 0xf); }
__forceinline Party* party_get_u(PartyMgr* m, unsigned i) { return m->pages[i >> 4] + (i & 0xf); }

struct Troop {                                   // 0x508-byte troop record, array at g_game+0x13a00
    char pad0[0x30];
    unsigned flags;                              // +0x30 (bit 0x10 = hero)
    char pad34[0x508 - 0x34];
};

struct GameData {                                // 0x5e2930
    char pad0[0x78];
    int main_party;                              // +0x78
    char pad7c[0xf8 - 0x7c];
    PartyMgr parties;                            // +0xf8
    char pad139d8[0x13a00 - 0x139d8];
    Troop* troops;                               // +0x13a00 (0x508-byte troop records)
    char pad13a04[0x13a18 - 0x13a04];
    char f13a18;                                 // +0x13a18
    char pad13a19[0x13af0 - 0x13a19];
    int f13af0;                                  // +0x13af0
    int f13af4;                                  // +0x13af4
    int f13af8;                                  // +0x13af8
    int f13afc;                                  // +0x13afc
    char pad13b00[0x13b10 - 0x13b00];
    char f13b10;                                 // +0x13b10
    char f13b11;                                 // +0x13b11
    void clear_hero_ammo(unsigned id);            // 0x513eb0
    void collect(int id, int* out, int* count);   // 0x490fa0 (thiscall, ret 0xc)
    int gate_rating(unsigned p1, unsigned p2, int p3);                    // 0x491b20 (ret 0xc)
    void count_party(unsigned id, int* a, int* b, int* c, int* d);       // 0x490ce0 (ret 0x14)
    void refresh_party(unsigned id);              // 0x4b4700 (ret 4; this = the game data)
};
extern GameData* g_game;                         // 0x5e2930

// @ 0x513e10
int count_hero_items()
{
    GameData* g = g_game;
    PartyMgr* m = &g->parties;
    Party* p = party_at(m, m->index_of(g->main_party));
    int n = 0;
    int i = 0;
    InvSlot* s = p->inv;
    while (i < p->inv_count) {
        if (s->flags & 1)
            break;
        n++;
        i++;
        s++;
    }
    int total = 0;
    s = p->inv;
    while (n-- > 0) {
        if (!(g->troops[s->item].flags & 0x10))
            total += s->f14;
        s++;
    }
    return total;
}

// @ 0x513eb0
void GameData::clear_hero_ammo(unsigned id)
{
    GameData* g = this;
    PartyMgr* m = &g->parties;
    if (id < m->count) {
        int idx = m->index_of(id);
        if (idx >= 0 && party_get_u(m, idx)->type == 1) {
            Party* p = party_get(m, m->index_of(id));
            for (int i = 0; i < p->inv_count; i++) {
                if ((g->troops[p->inv[i].item].flags & 0x10) != 0 && p->inv[i].f10 != 0)
                    p->inv[i].f10 = 0;
            }
        }
    }
}

// ---- the mission context and the scene ---------------------------------------------------------------------
struct MissionTemplate {                         // 0x58 bytes, array at 0x5e2a00
    char pad0[0x3c];
    int type;                                    // +0x3c
    char pad40[0x58 - 0x40];
};
extern MissionTemplate* g_templates;             // 0x5e2a00
extern int g_template_count;                     // 0x5e2a04

struct Scene;
extern Scene* g_scene;                           // 0x5e292c (the active scene object)

void mb_error(const char* fmt, ...);             // 0x42cc80 (cdecl)

struct Team;                                     // 8 team slots at ctx+0x527c, 0x16c each

struct MissionCtx {
    int party[2];                                // +0x00 (the two sides' party ids)
    char pad8[0x10 - 0x08];
    int f10;                                     // +0x10 (mission flags)
    int mode;                                    // +0x14
    int f18;                                     // +0x18
    int f1c;                                     // +0x1c
    char f20;                                    // +0x20
    char f21;                                    // +0x21
    char pad22[0x24 - 0x22];
    int f24;                                     // +0x24
    char pad28[0x2c - 0x28];
    int type;                                    // +0x2c (mission type -> template kind)
    char pad30[0x3c - 0x30];
    int f3c;                                     // +0x3c
    char pad40[0x44 - 0x40];
    int f44;                                     // +0x44
    char pad48[0x138 - 0x48];
    int side;                                    // +0x138 (selected side)
    char pad13c[0x144 - 0x13c];
    int f144;                                    // +0x144
    char pad148[0x164 - 0x148];
    int f164;                                    // +0x164
    char pad168[0x27c - 0x168];
    int f27c;                                    // +0x27c
    char pad280[0x2c4 - 0x280];
    int f2c4;                                    // +0x2c4
    char f2c8;                                   // +0x2c8
    char pad2c9[0x527c - 0x2c9];
    Team* team(int i) { return (Team*)((char*)this + 0x527c + i * 0x16c); }
    void pick_template();                        // 0x514360
    void select_side(int side);                  // 0x492750 (ret 4)
    void setup(unsigned flags, int delta);       // 0x513f80 (ret 8)
};

struct Row { float x, y, z, w; };
struct mat3 { Row s, f, u; };
mat3* identity3(mat3* m);                        // 0x40cd60 (cdecl)
struct Placement { mat3 m; int a, b, c, d; };    // 0x40 bytes (rotation + position)
struct SceneTmp {                                // 0x1c8 bytes, ctor 0x437830
    char pad0[0x10];
    int f10, f14, f18, f1c;                      // +0x10
    mat3 rot;                                    // +0x20
    Placement out;                               // +0x50
    char pad90[0x1c8 - 0x90];
    SceneTmp();                                  // 0x437830
};
struct IVector {                                 // std::vector<int>-like, +0x63c0
    void resize(unsigned n, int v);              // 0x43f310 (ret 8)
};
struct TStr {                                    // tstring, +0x6814
    void assign(const char* s);                  // 0x42bcb0
};
struct SceneSub {                                // object at +0x6688
    int dummy;
    void f(int v);                               // 0x4777c0 (ret 4)
};
struct GenA {                                    // reset object, this = 0xa76630
    void reset();                                // 0x4647e0
};
struct GenB {                                    // UI root, this = 0x5aa128
    void f();                                    // 0x494310
};

struct Scene {
    char pad0[0x5c];
    int f5c;                                     // +0x5c
    int f60;                                     // +0x60
    char pad64[0x5df0 - 0x64];
    int f5df0;                                   // +0x5df0 (template index)
    int f5df4;                                   // +0x5df4
    int f5df8;                                   // +0x5df8
    char pad5dfc[0x5e0c - 0x5dfc];
    int a5e0c[0x80];                             // +0x5e0c
    int a600c[0x80];                             // +0x600c (main party's list)
    int f620c;                                   // +0x620c
    int f6210;                                   // +0x6210
    int f6214;                                   // +0x6214
    int f6218;                                   // +0x6218
    char pad621c[0x62e0 - 0x621c];
    int f62e0;                                   // +0x62e0
    char pad62e4[0x6339 - 0x62e4];
    char f6339;                                  // +0x6339
    char pad633a[0x63c0 - 0x633a];
    IVector vec;                                 // +0x63c0
    char pad63c4[0x6520 - 0x63c4];
    int f6520;                                   // +0x6520
    char pad6524[0x6528 - 0x6524];
    int f6528;                                   // +0x6528
    char pad652c[0x6554 - 0x652c];
    char f6554;                                  // +0x6554
    char f6555;                                  // +0x6555
    char pad6556[0x6620 - 0x6556];
    int f6620;                                   // +0x6620
    Placement f6624;                             // +0x6624
    char pad6664[0x6668 - 0x6664];
    char f6668;                                  // +0x6668
    char f6669;                                  // +0x6669
    char pad666a[0x666c - 0x666a];
    int f666c;                                   // +0x666c
    int f6670;                                   // +0x6670
    int f6674;                                   // +0x6674
    char pad6678[0x6688 - 0x6678];
    SceneSub sub;                                // +0x6688
    char pad668c[0x6814 - 0x668c];
    TStr str;                                    // +0x6814
    void init(int tmpl, int a2, int a3, int a4, MissionCtx* ctx, int a6, int a7);   // 0x5143f0
    void init_scene();                           // 0x5144d0
};

// @ 0x513f80
void MissionCtx::setup(unsigned flags, int delta)
{
    int old = f1c;
    if (flags & 1)
        f20 = 1;
    if (flags & 2)
        f20 = 1;
    if (flags & 4)
        f3c |= 1;
    if (flags & 0x10) {
        select_side(0);
        f3c |= 0x400;
    }
    if (flags & 0x20) {
        select_side(1);
        f3c |= 0x400;
    }
    if (flags & 0x1000)
        f10 = f10 & ~4;
    else if (flags & 0x2000)
        f10 = f10 | 4;
    if (flags & 0x4000)
        f10 = f10 & ~8;
    else if (flags & 0x8000)
        f10 = f10 | 8;
    if (flags & 0x40) {
        f10 = f10 | 0xa0;
        f2c8 = 1;
        f3c |= 0x400;
    }
    if ((mode == 3 || mode == 4) && (f10 & 0x10) == 0) {
        int n = (int)flags >> 8 & 0xf;
        if (n > 8)
            n = 8 - n;
        f1c = g_game->gate_rating(g_game->main_party, party[side], party[1 - side]) + n;
        PartyMgr* m = &g_game->parties;
        party_at(m, m->index_of(g_game->main_party))->total_value(0x110);
        if (g_game->parties.is_slot(party[side]))
            party_at(&g_game->parties, g_game->parties.index_of(party[side]))->total_value(0x110);
        if (mode == 4 && g_game->parties.is_slot(party[1 - side]))
            g_game->parties.get(party[1 - side])->total_value(0x110);
        if (f1c > 6)
            f1c = 6;
        f27c = f1c - old;
        int i = 0;
        while (i < party_at(&g_game->parties, g_game->parties.index_of(g_game->main_party))->inv_count)
            i++;
        int l[8];
        l[0] = 0;
        l[2] = 0;
        l[4] = 0;
        l[6] = 0;
        g_game->count_party(party[side], &l[6], &l[4], &l[2], &l[0]);
        int d = -l[0];
        if (mode == 4) {
            l[1] = 0;
            l[3] = 0;
            l[5] = 0;
            l[7] = 0;
            g_game->count_party(party[1 - side], &l[7], &l[5], &l[3], &l[1]);
        }
        if (((signed char)f10 < 0 || f2c8 != 0) &&
            ((flags & 0x2000000) || (d <= 0 && f2c4 <= 0))) {
            f164 = 0x143;
            f18 = 1;
            f20 = 1;
        }
    }
    g_game->refresh_party(g_game->main_party);
    if (party[0] >= 0)
        g_game->refresh_party(party[0]);
    if (party[1] >= 0)
        g_game->refresh_party(party[1]);
    if (f20 && !f21) {
        int v = f3c | 0x200;
        f21 = 1;
        f3c = v;
        g_game->f13a18 = 0;
    }
}

// @ 0x514360
void MissionCtx::pick_template()
{
    int kind = type;
    f44 = kind;
    int found[32];
    int n = 0;
    for (int i = 0; i < g_template_count; i++) {
        if (g_templates[i].type == kind)
            found[n++] = i;
    }
    int idx;
    if (n == 0) {
        mb_error("Unable to find matching mission template for mission type %d.", kind);
        idx = 0;
    } else if (n <= 1) {
        idx = 0;
    } else {
        idx = rand() % n;
    }
    g_scene->init(found[idx], 0, 0, f1c, this, f24, side);
}

// @ 0x5143f0
void Scene::init(int tmpl, int a2, int a3, int a4, MissionCtx* ctx, int a6, int a7)
{
    f5df0 = tmpl;
    f6520 = a4;
    f5df8 = a3;
    f5df4 = a2;
    f6214 = a6;
    f620c = 0;
    f6210 = 0;
    int p0 = ctx->party[0];
    if (p0 >= 0) {
        f620c = 0;
        g_game->collect(p0, a5e0c, &f620c);
    }
    int p1 = ctx->party[1];
    if (p1 >= 0) {
        f6210 = 0;
        g_game->collect(p1, a600c, &f6210);
    }
    f6218 = a7;
    g_game->collect(g_game->main_party, a600c - a7 * 0x80, (int*)((char*)this + (0x1884 - a7) * 4));
    init_scene();
}

// ---- teams -------------------------------------------------------------------------------------------------
struct Sub3 {
    char pad[0x6c];
    void init();                                 // 0x5146f0
};
struct Team {
    int f0;                                      // +0x00
    int f4;                                      // +0x04
    Sub3 subs[3];                                // +0x08 (0x6c each)
    int sel[8];                                  // +0x14c
    void init(int idx);                          // 0x514670
};

// @ 0x514670
void Team::init(int idx)
{
    f4 = -1;
    f0 = 0;
    for (int j = 0; j < 3; j++)
        subs[j].init();
    int i = 0;
    int* p = sel;
    do {
        if (idx == 7)
            *p = 0;
        else
            *p = (i == idx) * 2 - 1;
        i++;
        p++;
    } while (i < 8);
    sel[7] = 0;
}

// @ 0x5144d0
void Scene::init_scene()
{
    f6339 = 0;
    SceneTmp t;
    sub.f(0);
    f6528 = *(int*)((char*)g_templates + f5df0 * 0x58 + 0x20);
    f6554 = 0;
    f6555 = 1;
    g_game->f13af0 = -1;
    g_game->f13af4 = -1;
    g_game->f13af8 = 0;
    g_game->f13afc = 0;
    g_game->f13b10 = 0;
    g_game->f13b11 = 0;
    f6620 = -1;
    f62e0 = 0;
    f5c = -1;
    f60 = -1;
    Team* tm = (Team*)((char*)this + 0x527c);
    for (int i = 0; i < 8; i++, tm++)
        tm->init(i);
    t.f10 = 0;
    t.f14 = 0;
    t.f18 = 0;
    t.out.m = *identity3(&t.rot);
    t.out.a = t.f10;
    t.out.b = t.f14;
    t.out.c = t.f18;
    t.out.d = t.f1c;
    f6624 = t.out;
    ((GenA*)0xa76630)->reset();
    ((GenB*)0x5aa128)->f();
    vec.resize(0, 0);
    f6668 = 0;
    f6669 = 0;
    f666c = 0;
    f6670 = 0;
    f6674 = 0;
    str.assign("");
}
