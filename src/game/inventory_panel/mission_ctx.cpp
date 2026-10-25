//-----------------------------------------------------------------------------
// File: mission_ctx.cpp  (0x513710..0x516ac0)
//
// Desc: the mission start-up object and the map screen: the commit / hotkey dispatcher of the map flags, the key mask and
//       the party flush, the hero-item counters, the mission setup, template pick and scene start (Mission::init,
//       init_scene, the team and class-record resets), the autosave notice, the hero table and battle start, the map
//       screen's per-frame update and camera, the battle record reset and the party "data ready" test.
//       "T regime" region (docs/tu_map.md): the file keeps the binary order; helpers that follow their caller are
//       __declspec(noinline). Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./mission_ctx.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../mission.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/msg_log.h"
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../overlay.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)

#define SCENE ((MissionV*)g_mission)

extern MsgLog g_msglog;                                 // 0x5d7080
extern int g_a735c0;                                    // 0xa735c0 (scene counter)
extern char g_a73320;                                   // 0xa73320
extern int g_a73438;                                    // 0xa73438 (page index)
extern int g_a7343c;                                    // 0xa7343c (page 1 party slot)
extern int g_a73440;                                    // 0xa73440 (page 0 party slot)

void autosave_notice();                                 // 0x514830
void mb_error(const char* fmt, ...);                    // 0x42cc80 (cdecl)

struct DetachTarget {                                   // the 0x98-byte object resolved from the handle at ctx+0x08
    void detach();                                // 0x4b6180
};
#include "../mode_rec.h"                              // MissionTemplate (0x58 bytes, array at 0x5e2a00): the one definition
extern MissionTemplate* g_mission_templates;                    // 0x5e2a00
extern int g_mission_template_count;                            // 0x5e2a04

using rgl::mat3;
mat3* identity3(mat3* m);                               // 0x40cd60 (cdecl)
struct FramePlacement { mat3 m; int a, b, c, d; };      // 0x40 bytes (rotation + position; was Placement: agent/agent.h's Placement is the game-data view of 0x435650)
struct SceneTmp {                                       // 0x1c8 bytes, ctor 0x437830
    char pad0[0x10];
    int f10, f14, f18, f1c;                             // +0x10
    mat3 rot;                                           // +0x20
    FramePlacement out;                                    // +0x50
    char pad90[0x1c8 - 0x90];
    SceneTmp();                                         // 0x437830
};
struct IVector { void resize(unsigned n, int v); };     // 0x43f310 (ret 8)
struct SceneSub { int dummy; void f(int v); };          // 0x4777c0 (ret 4)
struct GenA { void reset(); };                          // 0x4647e0 (this = 0xa76630)
struct GenB { void f(); };                              // 0x494310 (this = 0x5aa128)
void save_game(int id, int mode);                       // 0x4e8980
struct DialogMgrV {                                     // the dialog manager at 0x8ed50c8
    void start(int a, int b, int c, int d, int e);      // 0x4610b0 (ret 0x14)
};


int count_hero_items();                                 // 0x513e10
static inline tstring* lookup_ref(const tstring& k) { return g_strtable.lookup(&k); }
#include "../rec98.h"                                  // Rec98: the 0x98-byte scene node record (pages at game+0xe4)
// ---- objects and constants of other translation units ----------------------------------------------------------------
#include "../ui/named_rec.h"                            // NamedRec: setup 0x48a3a0 (ret 8)
struct UiKeyState { char down(int key); };                // 0x48ac90 (ret 4)
#include "../key_names.h"
struct TextLog { virtual void print(const char* s); };  // +0x28 prints one line (slot index 10 in the original; the declaration order is kept)
#include "../item_sound.h"                                    // ItemSound: the one definition (play 0x48add0)
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../skill_cap.h"
struct Psys {
    char pad0[0x48];
    int* f48;                                           // +0x48
    void render();                                       // 0x47fef0
};
struct FrameRenderer { virtual void set_frame(const rgl::matrix* m); };   // +0x54 (the slot index is not reproduced: only the call shape matters here)
struct PosV : rgl::matrix {
    rgl::vector4 transform_point(const rgl::vector4& p) const;   // 0x408460
};
struct PartyV : Party {
    int data_ready();                                   // 0x5166a0
};
void refresh_rating();                                  // 0x492470
void quick_save_msg();                                  // 0x516a70
int save_game(int arg, char* name);                     // 0x4e8980
extern "C" double __cdecl fmod(double, double);         // 0x536a6a (__CIfmod, x87 args)
extern tstring g_scratch;                               // 0xa70464
extern TextLog g_debug_log;                               // 0x5a9f30
extern UiKeyState g_keys;                                 // 0xa73730
extern __int64 g_a70448;                                // 0xa70448 (the 64-bit clock the map last played)
extern Psys* g_shadow_map;                                    // 0xa70248
extern int g_psys_scene;            // 0xa170c0
extern int g_a170d8;            // 0xa170d8
extern int g_a174d8;            // 0xa174d8
extern int g_show_quest_notes;            // 0xa181b4
extern int g_a171b8;            // 0xa171b8
extern int g_a17454;            // 0xa17454
extern int g_a17854;            // 0xa17854
extern int g_a17148;            // 0xa17148
extern int g_a17348;            // 0xa17348
extern int g_a70438;            // 0xa70438
extern int g_use_shaders;       // 0x5e28fc
extern int g_no_cam;            // 0x5a9fb0
extern char g_saved;            // 0x5e2a08
extern int g_trial_mode;        // 0x5e29c4
extern int g_a735c8;            // 0xa735c8
extern float g_view_a;          // 0x5d75d8
extern float g_view_b;          // 0x5d75dc
extern int g_level;         // 0xa732b4
extern const float g_zero;          // 0x549ca8  0.0f
extern const float g_ten;           // 0x549d74  10.0f
extern const float g_oneone;        // 0x54b888  1.1f
extern const float g_pttwo;         // 0x54a6e4  0.2f
extern const float g_three;         // 0x54a710  3.0f
extern const float g_quarter;       // 0x54b868  0.25f
extern const float g_four;          // 0x5935c8  4.0f
extern const float g_003;           // 0x54b8b4  0.03f
extern const float g_axis_x;        // 0xa178d4
extern const float g_axis_y;        // 0xa178dc
extern const float g_mouse_dx;      // 0x5d70a4
extern const float g_mouse_dy;      // 0x5d70a8
extern const float g_exp_arg;       // 0x5aa018
extern int g_key_1730;              // 0xa73730
extern int g_key_1734;              // 0xa73734
extern int g_key_1738;              // 0xa73738
extern int g_key_173c;              // 0xa7373c
extern int g_key_1740;              // 0xa73740
extern int g_key_1744;              // 0xa73744
extern int g_key_1748;              // 0xa73748
extern int g_key_174c;              // 0xa7374c
extern int g_keydown[];             // 0xa170d4
extern int g_in_17204;              // 0xa17204
extern int g_in_17210;              // 0xa17210
extern int g_in_17214;              // 0xa17214
extern int g_in_17218;              // 0xa17218
extern int g_in_17458;              // 0xa17458
extern int g_in_17858;              // 0xa17858
extern const float g_half;          // 0x549d70  0.5f

// the party record behind an id (the page table at +0x139c8)
static Party* party_by_id(int id)
{
    PartyMgr* mgr = &g_game->parties;
    int idx = mgr->index_of(id);
    return (Party*)((char*)mgr->pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8);
}

// @ 0x513710
void MapFlags::commit(char* out)
{
    if (f0 != 0)
        f0 = 0;

    if (f1 != 0) {
        if (f8 <= 0 && g_game->fb0 == 0)
            g_game->fb0 = 1;
        if (g_game->fb0 != 0) {
            g_mission_ctx.setup(g_game->fb0, 0);
            g_game->fb0 = 0;
        }
        f1 = 0;
    }

    if (GD_I(0xa8) != 0) {
        g_mission_ctx.setup(GD_I(0xa8), 0);
        GD_I(0xa8) = 0;
    }
    GD_I(0xa8) = 0;

    int key = g_mission_ctx.pop_key();
    *out = 1;

    if (key == 1) {
        f1 = 1;
        g_mission_ctx.start_mission();
    }
    else if (key == 2) {
        g_mission_ctx.build_hero_table(g_a73440, -1, -1, 3);
    }
    else if (key == 0x40 || key == 0x20) {
        if (key == 0x20) {
            int p = g_a7343c;
            g_mission_ctx.build_hero_table(*(int*)((char*)g_game->parties.get(p) + 0xac), -1, p, 2);
        }
        else if (key == 0x40) {
            int p = *(int*)(0xa73304 - g_a73438 * 4);
            g_mission_ctx.build_hero_table(*(int*)((char*)g_game->parties.get(p) + 0xac), -1, p, 4);
        }
    }
    else if (key == 0x10) {
        g_wAC->f4 = 0;
        if (g_a73320 != 0)
            g_wAC->f4 = 2;
        g_wAC->f18 = 0x64;
        g_wAC->f4 = 0;
        return g_main_menu.reset(0xb);
    }
    else if (key == 0x80) {
        g_wA0->f4 = 1;
        g_wA0->f18 = 0x64;
        return g_main_menu.choose(8);
    }
    else if (key == 0x100) {
        g_wB0->f4 = 4;
        g_wB0->f18 = 0x64;
        return g_main_menu.choose(0xc);
    }
    else if (key == 4) {
        g_mission_ctx.pick_template();
        g_wA8->f18 = 0x64;
        return g_main_menu.choose(0xa);
    }
    else if (key == 0x800) {
        g_mission_ctx.start_battle(GD_I(0x13af0), GD_I(0x13afc), GD_I(0x13af4), GD_I(0x13af8));
        g_wC0->f18 = 0x64;
        return g_main_menu.choose(0x10);
    }
    else if (key == 0x1000) {
        g_wEC->f18 = 0x64;
        return g_main_menu.choose(0x1b);
    }
    else if (key == 0x2000) {
        g_wE8->f18 = 0x64;
        return g_main_menu.choose(0x1a);
    }
    else if (key == 8) {
        g_mission_ctx.start_scene(GD_I(0x13afc), GD_I(0x13af4), GD_I(0x13af8));
        MI(g_mission, 0x5270) = g_a735c0++;
        SCENE->start_setup();
        g_wB8->v4();
        g_wB8->f18 = 0x64;
        return g_main_menu.choose(0xe);
    }
    else if (key == 0x200) {
        g_mission_ctx.update_parties();
        autosave_notice();
        return g_main_menu.choose(7);
    }
    else if (key == 0x400 && f8 <= 0) {
        g_w98->f18 = 0x64;
        return g_main_menu.choose(6);
    }
    else if (g_game->fc8 == 1) {
        g_game->fc8 = 0;
        g_mission_ctx.update_parties();
        autosave_notice();
        return g_main_menu.choose(7);
    }
    else if (f8 <= 0) {
        g_w98->f18 = 0x64;
        return g_main_menu.choose(6);
    }
    else {
        f2 = 1;
        g_wDC->v4();
        g_wDC->f18 = 0x64;
        return g_main_menu.choose(0x17);
    }

    g_wC0->f18 = 0x64;
    return g_main_menu.choose(0x10);
}

// @ 0x513b10
unsigned MissionCtx::pop_key()
{
    unsigned* p = (unsigned*)0x5a8e68;

    do {
        unsigned k = *p;
        if (k & f3c) {
            f3c &= ~k;
            return k;
        }
        p += 2;
    } while ((int)p < 0x5a8f68);
    return 0;
}

// @ 0x513b40
void MissionCtx::update_parties()
{
    if (f40 != 0) {
        f40 = 0;
        GV->clear_hero_ammo(g_game->f78);

        if (party[side] >= 0 && g_game->parties.is_live(party[side])) {
            int a = 0, b = 0, c = 0, d = 0;

            GV->clear_hero_ammo(party[side]);
            g_game->parties.get(party[side])->dirty = 1;
            g_game->count_units(party[side], &a, &b, &c, &d);
        }
        if (f2c8 != 0) {
            if (party[1 - side] >= 0 && g_game->parties.is_live(party[1 - side])) {
                int a = 0, b = 0, c = 0, d = 0;

                GV->clear_hero_ammo(party[1 - side]);
                g_game->parties.get(party[1 - side])->dirty = 1;
                g_game->count_units(party[1 - side], &a, &b, &c, &d);
            }
        }

        if (mode == 3 || (f164 & 1)) {
            unsigned h = f08;
            char* blk = (*(char***)((char*)g_game + 0xe4))[h >> 4];

            ((DetachTarget*)(blk + (h & 0xf) * 0x98))->detach();
        }

        PartyMgr* pm = &g_game->parties;

        party_at(pm, pm->index_of(g_game->f78))->f88 = -1;
        party_at(pm, pm->index_of(g_game->f78))->action = 0;

        if (count_hero_items() > f168) {
            tstring msg("ui_some_soldiers_are_ready_to_upgrade");

            g_msglog.add(*g_strtable.lookup(&msg), 0xffbbffbb);
        }

        party_at(pm, pm->index_of(g_game->f78))->dirty = 1;
    }
}

// @ 0x513e10
int count_hero_items()
{
    GameData* g = g_game;
    PartyMgr* m = &g->parties;
    Party* p = party_at(m, m->index_of(g->f78));
    int n = 0;
    int i = 0;
    Stack* s = p->stacks;
    while (i < p->nstacks) {
        if (s->flags & 1)
            break;
        n++;
        i++;
        s++;
    }
    int total = 0;
    s = p->stacks;
    while (n-- > 0) {
        if (!(g->troops[s->troop].flags32 & 0x10))
            total += s->f14;
        s++;
    }
    return total;
}

// @ 0x513eb0
void GameDataV::clear_hero_ammo(unsigned id)
{
    GameDataV* g = this;
    PartyMgr* m = &g->parties;
    if (id < m->n_parties) {
        int idx = m->index_of(id);
        if (idx >= 0 && party_at(m, idx)->type == 1) {
            Party* p = party_at(m, m->index_of(id));
            for (int i = 0; i < p->nstacks; i++) {
                if ((g->troops[p->stacks[i].troop].flags32 & 0x10) != 0 && p->stacks[i].f10 != 0)
                    p->stacks[i].f10 = 0;
            }
        }
    }
}

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
        f1c = GV->gate_rating(g_game->f78, party[side], party[1 - side]) + n;
        PartyMgr* m = &g_game->parties;
        party_at(m, m->index_of(g_game->f78))->sum_stat(0x110);
        if (g_game->parties.is_live(party[side]))
            party_at(&g_game->parties, g_game->parties.index_of(party[side]))->sum_stat(0x110);
        if (mode == 4 && g_game->parties.is_live(party[1 - side]))
            g_game->parties.get(party[1 - side])->sum_stat(0x110);
        if (f1c > 6)
            f1c = 6;
        f27c = f1c - old;
        int i = 0;
        while (i < party_at(&g_game->parties, g_game->parties.index_of(g_game->f78))->nstacks)
            i++;
        int l[8];
        l[0] = 0;
        l[2] = 0;
        l[4] = 0;
        l[6] = 0;
        g_game->count_units(party[side], &l[6], &l[4], &l[2], &l[0]);
        int d = -l[0];
        if (mode == 4) {
            l[1] = 0;
            l[3] = 0;
            l[5] = 0;
            l[7] = 0;
            g_game->count_units(party[1 - side], &l[7], &l[5], &l[3], &l[1]);
        }
        if (((signed char)f10 < 0 || f2c8 != 0) &&
            ((flags & 0x2000000) || (d <= 0 && f2c4 <= 0))) {
            f164 = 0x143;
            f18 = 1;
            f20 = 1;
        }
    }
    g_game->refresh_party(g_game->f78);
    if (party[0] >= 0)
        g_game->refresh_party(party[0]);
    if (party[1] >= 0)
        g_game->refresh_party(party[1]);
    if (f20 && !f21) {
        int v = f3c | 0x200;
        f21 = 1;
        f3c = v;
        GD_C(0x13a18) = 0;
    }
}

// @ 0x514360
void MissionCtx::pick_template()
{
    int kind = type;
    f44 = kind;
    int found[32];
    int n = 0;
    for (int i = 0; i < g_mission_template_count; i++) {
        if (g_mission_templates[i].type == kind)
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
    SCENE->init(found[idx], 0, 0, f1c, this, f24, side);
}

// @ 0x5143f0
void MissionV::init(int tmpl, int a2, int a3, int a4, MissionCtx* ctx, int a6, int a7)
{
    mode = tmpl;
    MI(this, 0x6520) = a4;
    f5df8 = a3;
    f5df4 = a2;
    f6214 = a6;
    cnt[0] = 0;
    cnt[1] = 0;
    int p0 = ctx->party[0];
    if (p0 >= 0) {
        cnt[0] = 0;
        g_game->list_members(p0, (unsigned*)list[0], (unsigned*)&cnt[0]);
    }
    int p1 = ctx->party[1];
    if (p1 >= 0) {
        cnt[1] = 0;
        g_game->list_members(p1, (unsigned*)list[1], (unsigned*)&cnt[1]);
    }
    f6218 = a7;
    g_game->list_members(g_game->f78, (unsigned*)(list[1] - a7 * 0x80), (unsigned*)((char*)this + (0x1884 - a7) * 4));
    init_scene();
}

// @ 0x5144d0
void MissionV::init_scene()
{
    *((char*)this + 0x6339) = 0;
    SceneTmp t;
    ((SceneSub*)((char*)this + 0x6688))->f(0);
    MI(this, 0x6528) = *(int*)((char*)g_mission_templates + mode * 0x58 + 0x20);
    *((char*)this + 0x6554) = 0;
    *((char*)this + 0x6555) = 1;
    GD_I(0x13af0) = -1;
    GD_I(0x13af4) = -1;
    GD_I(0x13af8) = 0;
    GD_I(0x13afc) = 0;
    GD_C(0x13b10) = 0;
    GD_C(0x13b11) = 0;
    MI(this, 0x6620) = -1;
    dismount_count = 0;
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
    *(FramePlacement*)((char*)this + 0x6624) = t.out;
    ((GenA*)0xa76630)->reset();
    ((GenB*)0x5aa128)->f();
    ((IVector*)((char*)this + 0x63c0))->resize(0, 0);
    *((char*)this + 0x6668) = 0;
    *((char*)this + 0x6669) = 0;
    MI(this, 0x666c) = 0;
    MI(this, 0x6670) = 0;
    MI(this, 0x6674) = 0;
    str.assign("");
}

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

// @ 0x5146f0
void Sub3::init()
{
    f0 = 0;
    f4 = 0;
    f8 = 0;
    fc = 0;
    Lane4 v(20.0f, 20.0f, 20.0f);
    a = v;
    f44 = 1;
    f48 = 1;
    c = Lane4(0.0f, 0.001f, 0.0f);
    d = v;
    f4c = 10.0f;
    f58 = 2;
    f30 = -1;
    b = Lane4(0.0f, 0.0f, 0.0f);
}

// @ 0x5147e0
void MissionCtx::start_scene(int a1, int a2, int a3)
{
    f154 = a1;
    f158 = a2;
    f15c = a3;
    SCENE->init(a1, a2, a3, f1c, this, f24, side);
}

// @ 0x514830
void autosave_notice()
{
    if (GD_I(0xb4) != 0)
        return;
    {
        tstring s("ui_autosaving");
        g_msglog.add(*g_strtable.lookup(&s), -1);
    }
    save_game(-1, 0);
}

// @ 0x514890
void MissionCtx::build_hero_table(int a1, int a2, int a3, int a4)
{
    GD_I(0x13a08) = a1;
    char* t = (char*)GD_I(0x13a50);
    int off = 0x5a8;
    do {
        *(int*)(t + off) = -1;
        off += 8;
    } while (off < 0x7a8);
    *(int*)(t + 0x5a8) = g_game->player;
    *(int*)(t + 0x7a8) = 1;
    *(int*)(t + 0x5b0) = a1;
    *(int*)(t + 0x5b4) = a2;
    *(int*)(t + 0x7ac) = 1;

    PartyMgr* m = &g_game->parties;
    Party* p = party_at(m, m->index_of(g_game->f78));
    if (1 < p->nstacks) {
        int n = 1;
        int oi = 0x5b8;
        int of = 0x7b0;
        Stack* s = &p->stacks[1];
        do {
            int item = s->troop;
            if ((g_game->troops[item].flags32 & 0x10) != 0 && item != a1) {
                *(int*)(t + oi) = item;
                *(int*)(t + of) = 1;
                oi += 8;
                of += 4;
            }
            n++;
            s++;
        } while (n < p->nstacks);
    }

    if (a3 > 0 && a3 != g_game->f78) {
        if (1 < party_at(m, m->index_of(a3))->nstacks) {
            int oi = 0x630;
            int of = 0x7ec;
            int n = 1;
            for (; n < party_at(m, m->index_of(a3))->nstacks; n++) {
                int item = party_at(m, m->index_of(a3))->stacks[n].troop;
                if ((g_game->troops[item].flags32 & 0x10) != 0) {
                    *(int*)(t + oi) = item;
                    *(int*)(t + of) = 1;
                    oi += 8;
                    of += 4;
                }
            }
        }
    }

    SCENE->init(1, 1, 0, 0, this, f24, 0);
    MI(g_mission, 0x6558) = a1;
    MI(g_mission, 0x5270) = f2c0;
    f2c0++;
    SCENE->start_setup();
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    ((DialogMgrV*)0x8ed50c8)->start(a1, -1, a3, a4, 1);
}

// @ 0x514c00
void MissionCtx::start_battle(int a1, int a2, int a3, int a4)
{
    GD_I(0x13a08) = a1;
    f154 = a2;
    f158 = a3;
    f15c = a4;
    SCENE->init(a2, a3, a4, f1c, this, f24, side);
    MI(g_mission, 0x5270) = f2c0;
    f2c0++;
    SCENE->start_setup();
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    ((DialogMgrV*)0x8ed50c8)->start(a1, -1, party[side], 0, 1);
}

// @ 0x514cd0
void MissionCtx::start_mission()
{
    int id = party[side];
    Party* p = party_at(&g_game->parties, g_game->parties.index_of(id));
    int v = GV->token_lookup(id, 0, 0, 0);
    build_hero_table(p->stacks[0].troop, v, id, 1);
}

// @ 0x514d40 ---------------------------------------------------------------------------------------
void Screen::frame_update(float dt)
{
    float t = 0.0f;                                 // game hours since the clock's reference tick
    char flag;                                      // 1 while the map is idle
    bool start;

    if (g_shadow_map != 0 && g_shadow_map->f48[0x50 / 4] != g_psys_scene
        && g_a70438 == g_level && g_use_shaders != 0)
        g_shadow_map->render();

    cam.perspective = 1;
    cam.nearz = 0.2f;
    cam.farz = 300.0f;
    cam.left = g_view_a * -0.1f;
    cam.right = g_view_a * 0.1f;
    cam.bottom = g_view_b * -0.095f;
    cam.top = g_view_b * 0.095f;
    cam.build_planes();
    f30->view = cam;

    __int64 cur = g_ticks[GD_I(0x13b18)];
    __int64 ref = *(__int64*)((char*)g_game + 0x13b20);
    t = (float)(cur - ref) * 1e-5f;

    float hour = (float)fmod((double)t, 24.0);

    float sz[2];
    PSO(f5c)->set_text(sz, lookup_ref(
        hour < 1.0f  ? tstring("ui_midnight") :
        hour < 4.0f  ? tstring("ui_late_night") :
        hour < 6.0f  ? tstring("ui_dawn") :
        hour < 8.0f  ? tstring("ui_early_morning") :
        hour < 11.0f ? tstring("ui_morning") :
        hour < 13.0f ? tstring("ui_noon") :
        hour < 16.0f ? tstring("ui_afternoon") :
        hour < 18.0f ? tstring("ui_late_afternoon") :
        hour < 20.0f ? tstring("ui_dusk") :
        hour < 23.0f ? tstring("ui_evening") :
                       tstring("ui_midnight")));

    int hi = (int)t;
    g_scratch.assign("");
    g_game->emit_event(0xf, hi / 24, hi);
    {
        tstring tmp(g_scratch);
        PSO(f60)->set_text(sz, &tmp);
    }

    flag = 1;
    if (GD_F(0x13ad0) > 0.0f)
        flag = 0;
    if (GD_C(0x18) == 0 && g_keys.down(0x24))
        quick_save_msg();
    if (GD_I(0x1a208) >= 0) {
        g_a70448 = GD_I(0x1a208);
        ((ItemSound*)((char*)g_game + 0x139f4))->play(-8);
        GD_I(0x1a208) = -1;
    }

    int cmd = GD_I(0xc8);
    if (cmd == 0x12) {
        GD_I(0xc8) = 0;
        g_wDC->f18 = 7;
        g_main_menu.choose(0x17);
        return;
    }
    if (cmd == 0xf) {
        GD_I(0xc8) = 0;
        g_wEC->f18 = 7;
        g_main_menu.choose(0x1b);
        return;
    }
    if (cmd == 0x10) {
        GD_I(0xc8) = 0;
        g_wE8->f18 = 7;
        g_main_menu.choose(0x1a);
        return;
    }
    if (g_a170d8 != 0 && g_a174d8 == 0) {
        g_w94->f18 = 7;
        g_main_menu.choose(5);
        return;
    }
    if (f48 != 0 && PSO(f48)->clicked_on(0x10000)) {
        g_wD8->f18 = 7;
        g_wD8->f4 = 0;
        g_main_menu.choose(0x16);
        return;
    }
    if (flag) {
        if (PSO(f50)->clicked_on(0x10000) || g_keys.down(0x12)) {
            if (g_show_quest_notes != 0) {
                g_wE8->f18 = 7;
                GD_I(0x13b08) = 4;
                GD_I(0x13b0c) = 0;
                g_main_menu.choose(0x1a);
                return;
            }
            g_wB4->f18 = 7;
            g_wB4->f4 = 0;
            g_main_menu.choose(0xd);
            return;
        }
    }

    if (f64 != 0) {
        if (g_saved == 0)
            save_game(-1, 0);
        g_saved = 1;
        return;
    }
    if (GD_C(0x13ac8) != 0) {
        ((NamedRec*)g_main_menu.f0)->setup((tstring*)((char*)g_game + 0x13ab8), (tstring*)((char*)g_game + 0x13aa8));
        g_main_menu.reset(0);
        GD_C(0x13ac8) = 0;
        return;
    }
    if (GD_C(0x13a3c) != 0) {
        GD_C(0x13a3c) = 0;
        g_mission_ctx.build_hero_table(GD_I(0x13a44), -1, -1, GD_I(0x13a40));
        g_wC0->f18 = 7;
        g_main_menu.choose(0x10);
        return;
    }

    int fpid = GD_I(0x78);
    Party* party = PARTIES->get(fpid);
    if (party->f88 < 0)
        goto no_party;
    party = PARTIES->get(fpid);
    {
        char** pages = *(char***)((char*)g_game + 0xe4);
        unsigned idx = (unsigned)party->f88;
        Rec98* node = (Rec98*)(pages[idx >> 4] + (idx & 0xf) * 0x98);
        if (node->f10 != 1)
            goto no_party;
    }
    party = PARTIES->get(fpid);
    {
        char** pages = *(char***)((char*)g_game + 0xe4);
        unsigned idx = (unsigned)party->f88;
        Rec98* node = (Rec98*)(pages[idx >> 4] + (idx & 0xf) * 0x98);
        start = true;
        if (node->f54 == GD_I(0x78)) {
            GD_I(0x13a0c) = node->f58;
            GD_I(0x13a10) = -100000;
        } else if (node->f58 == GD_I(0x78)) {
            GD_I(0x13a0c) = node->f54;
            GD_I(0x13a10) = -100000;
        } else {
            GD_I(0x13a0c) = node->f58;
            GD_I(0x13a10) = node->f54;
        }
    }
    if (g_game->troops[g_game->player].level <= 6 || g_trial_mode != 0) {
        if (1 - (int)(t * -0.041666667f) > 30) {
            if (g_trial_mode == 0) {
                tstring a("ui_explanation_time_limit");
                tstring b("ui_time_limit_reached");
                ((NamedRec*)g_main_menu.f0)->setup(g_strtable.lookup(&b), g_strtable.lookup(&a));
                g_main_menu.reset(g_trial_mode);
                f64 = 1;
                start = false;
            }
        }
    } else {
        tstring a("ui_explanation_level_limit");
        tstring b("ui_level_limit_reached");
        ((NamedRec*)g_main_menu.f0)->setup(g_strtable.lookup(&b), g_strtable.lookup(&a));
        g_main_menu.reset(g_trial_mode);
        f64 = 1;
        start = false;
    }

    if (!start)
        return;

    party = PARTIES->get(GD_I(0x78));
    g_mission_ctx.init_battle(party->f88);
    GD_I(0x13aec) = 0;
    GD_I(0x13af8) = 0;
    ((ItemSound*)((char*)g_game + 0x139f4))->play(-6);
    g_game->emit_event(1, GD_I(0x13a0c), GD_I(0x13a10));
    if (GD_I(0x13aec) > 0) {
        g_mission_ctx.frame(GD_I(0x13aec));
        party = PARTIES->get(GD_I(0x78));
        g_map_flags.f8 = GD_I(0x13aec);
        g_map_flags.f4 = party->f88;
    } else {
        g_mission_ctx.frame(-1);
        g_a735c8 = 1;
        party = PARTIES->get(GD_I(0x78));
        g_map_flags.f8 = -1;
        g_map_flags.f4 = party->f88;
    }
    g_map_flags.f0 = 1;
    g_map_flags.f1 = 0;
    g_map_flags.f2 = 0;
    g_map_flags.commit(&flag);
    return;

no_party:
    if (flag) {
        if (PSO(f40)->clicked_on(0x10000) || g_keys.down(0x10)) {
            g_wA0->f18 = 7;
            g_wA0->f4 = 0;
            g_main_menu.choose(8);
            return;
        }
        if (PSO(f3c)->clicked_on(0x10000) || g_keys.down(0x11)) {
            g_wB0->f18 = 7;
            g_wB0->f4 = 0;
            g_main_menu.choose(0xc);
            return;
        }
        if (PSO(f44)->clicked_on(0x10000) || g_keys.down(0xf)) {
            g_wBC->f18 = 7;
            g_wBC->f4 = 0;
            g_wBC->f14 = GD_I(0x8c);
            g_main_menu.choose(0xf);
            return;
        }
        if (g_keys.down(0x13)) {
            g_wE4->f18 = 7;
            g_wE4->f4 = 0;
            g_main_menu.choose(0x19);
            return;
        }
        if (f4c != 0 && PSO(f4c)->clicked_on(0x10000)) {
            Party* p = PARTIES->get(GD_I(0x78));
            GV->attach_party_center(&p->pos.x, GD_I(0x78), GD_I(0x80));
            return;
        }
    }
    if (f38 != 0 && PSO(f38)->clicked_on(0x10000)) {
        GD_I(0x13aec) = 4;
        g_wDC->f18 = 7;
        g_main_menu.choose(0x17);
        return;
    }
    if (GD_F(0x14058) == 0.0f)
        PSO(f54)->act_b4();
    else
        PSO(f54)->clear();
    g_msglog.flush();
    if (GD_F(0x13ad0) > 0.0f && GD_I(0x13ad8) != 0) {
        PSO(f58)->act_b4();
        if (g_a17454 == 0 && g_a17854 != 0)
            GD_F(0x13ad0) = 0.0f;
    } else {
        PSO(f58)->clear();
    }
    if (!(GD_F(0x13ad0) > 0.0f && GD_I(0x13ad8) != 0))
        camera_move(dt);
    camera_update(dt);
    if (GD_F(0x13ad0) <= 0.0f) {
        f5191e0();
        f2cc += dt;
        f2d0 += dt;
        GD_I(0x13aa4) = 0;
        {
            int bpid = GD_I(0x78);
            if (PARTIES->get(bpid)->action == 4 || PARTIES->get(bpid)->action == 1
                || PARTIES->get(bpid)->action == 6 || PARTIES->get(bpid)->action == 9)
                GD_I(0x13aa4) = 1;
        }
        if (g_a171b8 != 0) {
            PARTIES->get(GD_I(0x78))->action = 0;
            PARTIES->get(GD_I(0x78))->reissue_order();
            GD_I(0x13aa4) = 2;
        }
    }

    GV->tick(dt);
    rgl::matrix m;
    rgl::identity_pos(&m);
    {
        float vx = 0.0175f - g_view_a * 0.1975f;
        float vy = 0.015f - g_view_b * 0.18f;
        rgl::vector4 v(vx, vy, -0.4f);
        rgl::vector4 p = ((PosV*)&cam.m)->transform_point(v);
        m.o = p;
    }
    (*(FrameRenderer**)((char*)g_game + 0x1a2a8))->set_frame(&m);

    if (g_no_cam != 0 && (g_a17148 != 0 || g_a17348 != 0) && g_key_names.is_bound(0x12))
        GD_I(0x1a090) = (GD_I(0x1a090) == 0);

    if (GD_I(0x1a090) != 0) {
        char buf[0x200];
        int bpid = GD_I(0x78);
        sprintf(buf, "Position: %3.2f, %3.2f ;",
                (double)PARTIES->get(bpid)->pos.x, (double)PARTIES->get(bpid)->pos.y);
        g_debug_log.print(";-------------------;");
        g_debug_log.print(buf);
        g_debug_log.print(";-------------------;");
    }
}

// @ 0x515d80
void Screen::camera_move(float dt)
{
    float src[2][2] = { { 0.0f, 1.0f }, { 1.0f, 0.0f } };
    mat2 m(&src[0][0], &src[1][0]);
    m.rotate_2x2(-yaw);

    bool b = g_key_1740 >= 0 && g_keydown[g_key_1740] != 0;
    if ((g_key_1744 >= 0 && g_keydown[g_key_1744] != 0) || b) {
        float h = height + g_ten;
        x -= m.m[0] * h * dt;
        z -= m.m[1] * h * dt;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }
    b = g_key_1748 >= 0 && g_keydown[g_key_1748] != 0;
    if ((g_key_174c >= 0 && g_keydown[g_key_174c] != 0) || b) {
        float h = height + g_ten;
        x += m.m[0] * h * dt;
        z += m.m[1] * h * dt;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }
    b = g_key_1730 >= 0 && g_keydown[g_key_1730] != 0;
    if ((g_key_1734 >= 0 && g_keydown[g_key_1734] != 0) || b) {
        float h = height + g_ten;
        x += m.m[2] * h * dt;
        z += m.m[3] * h * dt;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }
    b = g_key_1738 >= 0 && g_keydown[g_key_1738] != 0;
    if ((g_key_173c >= 0 && g_keydown[g_key_173c] != 0) || b) {
        float h = height + g_ten;
        x -= m.m[2] * h * dt;
        z -= m.m[3] * h * dt;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }

    if (g_in_17204) speed += dt * g_oneone;
    if (g_in_17214) speed -= dt * g_oneone;
    if (g_in_17210) vy += dt * g_oneone;
    if (g_in_17218) vy -= dt * g_oneone;

    if (g_in_17458 && !g_in_17858)
        f2d4 = 0;

    float v = g_mouse_dx * 0.1f;
    if (!(v > -1.0f)) v = -1.0f;
    else if (v > 1.0f) v = 1.0f;

    float w = g_mouse_dy * 0.1f;
    if (!(w > -2.0f)) w = -2.0f;
    else if (w > 2.0f) w = 2.0f;

    if (g_axis_x != 0.0f)
        f2d4 = 1;

    vy = v * dt + vy;
    if (g_in_17458) {
        vy = expf(g_four * g_exp_arg) * g_003 * g_axis_x * dt + vy;
    }

    if (w != 0.0f || g_axis_y != 0.0f) {
        y = y - w * height * dt;
        if (g_axis_y > 0.0f)
            y = y - height * g_pttwo;
        else if (g_axis_y < 0.0f)
            y = height * g_pttwo + y;
    }
}

// @ 0x516220
mat2::mat2(const float* a, const float* b)
{
    m[0] = a[0];
    m[1] = a[1];
    m[2] = b[0];
    m[3] = b[1];
}

// @ 0x516250
void MissionCtx::init_battle(int id_)
{
    base_init();
    f08 = id_;
    unsigned urow = (id_ & 0xf) * 0x98;
    f40 = 1;
    side = 0;
    f144 = 1;
    f10 = 1;
    mode = 3;

    char* pages = *(char**)((char*)g_game + 0xe4);
    char* rec = *(char**)(pages + ((unsigned)id_ >> 4) * 4);
    rec = (char*)((char*)rec + urow);
    if (g_game->f78 == *(int*)((char*)rec + 0x54)) {
        party[0] = *(int*)((char*)rec + 0x58);
        party[1] = -1;
        f24 = 1;
        f144 = 1;
    } else if (g_game->f78 == *(int*)((char*)rec + 0x58)) {
        party[0] = *(int*)((char*)rec + 0x54);
        party[1] = -1;
        f24 = 0;
        f144 = 1;
    } else if (*(int*)((char*)rec + 0x10) == 1) {
        party[0] = *(int*)((char*)rec + 0x58);
        party[1] = *(int*)((char*)rec + 0x54);
        f10 = 0x11;
        f144 = 0;
        f24 = 2;
        mode = 4;
    }
    GD_C(0x13a18) = 1;
}

// @ 0x516370
void MissionCtx::frame(int tick)
{
    f2c4 = tick;
    if (f24 == 2 && tick <= 0) {
        int aid = (int)f08;
        char* rec = (char*)((char*)(*(char***)((char*)g_game + 0xe4))[aid >> 4] + (aid & 0xf) * 0x98);
        party[0] = *(int*)((char*)rec + 0x58);
        rec = (char*)((char*)(*(char***)((char*)g_game + 0xe4))[aid >> 4] + (aid & 0xf) * 0x98);
        party[1] = *(int*)((char*)rec + 0x54);

        bool a = !(*(float*)((char*)GD_I(0x13a48) + party_by_id(party[0])->f70 * 0x300 + 0x2c) > 0.0f);
        bool b = *(float*)((char*)GD_I(0x13a48) + party_by_id(party[1])->f70 * 0x300 + 0x2c) < 0.0f;
        if (a != b) {
            side = b ? 1 : 0;
            f10 &= ~0x10;
            f144 = 1;
        } else {
            f10 |= 0x10;
            side = 0;
            f144 = 0;
        }
    }
    if (f144 != 0) {
        refresh_rating();
        if (f2c4 < 1)
            f3c |= 1;
    }
    Party* p = party_by_id(g_game->f78);
    if (((PartyV*)p)->data_ready())
        f10 |= 2;

    char* self = (char*)((char*)(*(char***)((char*)g_game + 0xe4))[(int)f08 >> 4] + ((int)f08 & 0xf) * 0x98);
    if (g_game->f78 == *(int*)((char*)self + 0x58))
        f10 |= 4;
    else if (g_game->f78 == *(int*)((char*)self + 0x54))
        f10 |= 8;

    update_anim();
}

// @ 0x5166a0  Party::data_ready
int PartyV::data_ready()
{
    int clk = *(int*)((char*)this + 0x59b0);
    __int64 start = *(__int64*)((char*)this + 0x59b8);
    if ((*(__int64*)&flags_lo & 0x1000000) == 0) {
        __int64 d = g_ticks[clk] - start;
        if (!((float)d * g_tick_scale < g_quarter))
            return 0;
    }
    return 1;
}

// @ 0x516700
void MissionCtx::update_anim()
{
    f2c0 = 0;
    reset_ids(g_game->f78);
    reset_ids(party[0]);
    reset_ids(party[1]);
    {
        int* ids = &party[0];
        if (ids[side] >= 0)
            GV->fill_ids((int*)((char*)g_game + 0x1605c), ids[side]);
        if (ids[1 - side] >= 0)
            GV->fill_ids((int*)((char*)g_game + 0x1805c), ids[1 - side]);
    }
    if (party[0] >= 0) {
        GameData* g = g_game;
        PartyMgr* m = &g->parties;
        Party* p = party_at(m, m->index_of(party[0]));
        int slot = p->stacks[0].troop;
        if (slot >= 0) {
            Troop* t = &g->troops[slot];
            if ((t->flags32 & 0x10) != 0) {
                int k = ((char*)t)[0x7a] & 0xf;
                if (k < 0)
                    k = 0;
                else if (k > (*(int*)&g_skill_tbl[36].cap))
                    k = (*(int*)&g_skill_tbl[36].cap);
                int n = t->f58[0] + k * 2 + 0x23;
                float g = (float)(int)((float)n * g_half + g_three);
                if (!(g <= t->hp))
                    t->hp = g;
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
    GV->reset_trade_ids();
    f2c0 = 0;               // mirrors the original's trailing inline marker (blocks the tail call)
}

// @ 0x5168b0  clear the 0x60 (stride 8) trade ids of the record selected by f90
void GameDataV::reset_trade_ids()
{
    ItemRef* p = &troops[*(int*)((char*)this + 0x90)].inv[0];
    for (int i = 0; i < 0x60; i++) {
        p->item = -1;
        p++;
    }
}

// @ 0x5168e0  rebuild a party's id grid (the 0x20-slot rows at party+0xa8)
void GameDataV::fill_ids(int* out, int id)
{
    PartyMgr* m = &parties;
    Party* rec = party_at(m, m->index_of(id));
    if (rec->nstacks <= 0)
        return;
    int* base = out;
    for (int row = 0; row < rec->nstacks; row++, base += 0x20) {
        int* o = base;
        for (int j = 0; j < rec->stacks[row].count; j++, o++) {
            if (j >= 0x20)
                break;
            int v = -1;
            if (id >= 0)
                v = m->get(id)->stacks[row].troop;
            *o = v * 0x1bf + (id * 0x7bb7 + j) + 0x45db;
        }
    }
}

// @ 0x5169e0
void MissionCtx::reset_ids(int id)
{
    if (id == -1)
        return;
    PartyMgr* m = &g_game->parties;
    Party* rec = party_at(m, m->index_of(id));
    for (int i = 0; i < rec->nstacks; i++) {
        int slot = *(int*)((char*)rec + 0xac + i * 0x20);
        Troop* t = (Troop*)((char*)g_game->troops + slot * 0x508);
        if ((t->flags32 & 0x10) != 0) {
            *(int*)((char*)rec + 0xb8 + i * 0x20) = 0;
            *(int*)((char*)rec + 0xa8 + i * 0x20) = 1;
        }
    }
}

// @ 0x516a70
void quick_save_msg()
{
    g_msglog.add(*g_strtable.lookup(&tstring("ui_quick_saving")), -1);
    save_game(-1, 0);
}
