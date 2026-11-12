//-----------------------------------------------------------------------------
// File: mission_ctx.cpp  (0x513b10..0x516ac0)
//
// Desc: the mission start-up object and the map screen (the commit / hotkey dispatcher 0x513710 now lives in
//       inventory_panel.cpp, whose frame_update saw its body): the key mask and
//       the party flush, the hero-item counters, the mission setup, template pick and scene start (Mission::init,
//       init_scene, the team and class-record resets), the autosave notice, the hero table and battle start, the map
//       screen's per-frame update and camera, the battle record reset and the party "data ready" test.
//       "T regime" region (docs/tu_map.md): the file keeps the binary order; helpers that follow their caller are
//       __declspec(noinline). Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#define PLACEDOBJ_CTOR_OUT_OF_LINE   // init_scene 0x5144d0 calls PlacedObj::PlacedObj (0x437830) for the by-value argument
#include "../../include/image_addrs.h"
#include "./mission_ctx_rec.h"   // MissionCtx g_mission_ctx (0xa73300): listed directly (the resolution audit reads only the source's own includes)
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../game_globals.h"   // g_trial_mode (canonical)
#include "./mission_ctx.h"
#include "../render_batch.h"   // g_batch (0x5fe898): counter 0x418828
#include "../terrain_a/terrain.h"   // Terrain, g_terrain (0xa76630)
#include "../window_mgr.h"      // g_windows slot 11 (0xa739ac)
#include "../game_globals.h"      // g_bit (0x5a8e68)
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../mission.h"
#include "../placed_obj.h"      // PlacedObj: the by-value argument of SceneSub::f (constructor 0x437830)
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../map_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../campaign_screen.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../shared_vars.h"
#include "../game_keys.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


#define SCENE ((MissionV*)g_mission)


void autosave_notice();                                 // 0x514830
void mb_error(const char* fmt, ...);                    // 0x42cc80 (cdecl)

struct BattleRec {                                      // the 0x98-byte object resolved from the handle at ctx+0x08 (game/party2)
    void detach();                                // 0x4b6180
};
#include "../mode_rec.h"                              // MissionTemplate (0x58 bytes, array at 0x5e2a00): the one definition
extern MissionTemplate* g_mission_templates;                    // 0x5e2a00

using rgl::mat3;
struct FramePlacement { mat3 m; int a, b, c, d; };      // 0x40 bytes (rotation + position; was Placement: agent/agent.h's Placement is the game-data view of 0x435650)
struct FrameTemp {                                      // init_scene's 0x80-byte locals: the zero origin, the identity rotation, the placement built from them
    rgl::vector4 pos;                                   // +0x00
    mat3 rot;                                           // +0x10
    FramePlacement out;                                 // +0x40
};
template<> void std::vector<int, std::allocator<int> >::resize(size_t, int);   // 0x43f310 (ret 8; agent3/agent_combat.cpp: the out-of-line instance, only declared)
template<> void std::vector<PlacedObj, std::allocator<PlacedObj> >::resize(size_t, PlacedObj);   // 0x4777c0 (terrain_b/terrain_b.cpp; ret 0x1cc: the 0x1c8 object by value, then the count)
int save_game(int id, tstring* name);                  // 0x4e8980
#include "../dialog_mgr.h"                              // DialogMgr g_dialogs (0x8ed50c8)


static inline tstring* lookup_ref(const tstring& k) { return g_strtable.lookup(k); }
#include "../rec98.h"                                  // Rec98: the 0x98-byte scene node record (pages at game+0xe4)
// ---- objects and constants of other translation units ----------------------------------------------------------------
#include "../ui/named_rec.h"                            // NamedRec: setup 0x48a3a0 (ret 8)
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../slot.h"
#include "../game_keys.h"                              // GameKeys g_game_keys (0xa73730): clicked 0x48ac90
#include "../key_names.h"
#include "../../rgl/stream.h"                           // text_stream g_debug_log (0x5a9f30): slot 10 (+0x28, w_cstr) prints one line
#include "../ui/timer_list.h"                                  // TimerList: the sound holder (update 0x48add0)
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../skill_cap.h"
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers

// ---- data this module owns (.bss, no initialiser; sizes from the code) ----

#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../window_mgr.h"      // slot_as<T>(i)
#include "../../include/render_flags.h"
#include "../shadow_map.h"
#include "../ui_globals.h"      // UI / core globals
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
struct ShadowMapP {
    char pad0[0x48];
    int* f48;                                           // +0x48
    void render();                                       // 0x47fef0 (game/mesh_slot)
};
// the compass entity (game+0x1a2a8, an rgl::entity): slot 21 (+0x54) is entity::set_local_dirty (0x41fe70)
struct FrameRenderer {
    virtual void s00(); virtual void s04(); virtual void s08(); virtual void s0c(); virtual void s10(); virtual void s14();
    virtual void s18(); virtual void s1c(); virtual void s20(); virtual void s24(); virtual void s28(); virtual void s2c();
    virtual void s30(); virtual void s34(); virtual void s38(); virtual void s3c(); virtual void s40(); virtual void s44();
    virtual void s48(); virtual void s4c(); virtual void s50();
    virtual void set_frame(const rgl::matrix* m);   // +0x54
};
struct MapPanel {
    void update_target();   // 0x5191e0 (inventory_panel/map_panel.h)
};
struct GameG {
    void tick(float dt);   // 0x516ac0 (ret 4; inventory_panel/game_tick.h)
};
struct PartyV : Party {
    int data_ready();                                   // 0x5166a0
};
void quick_save_msg();                                  // 0x516a70
extern "C" double __cdecl fmod(double, double);         // 0x536a6a (__CIfmod, x87 args)
#include "../game_globals.h"      // g_module_settings.show_quest_notes (canonical)




extern const float g_ten;           // 0x549d74  10.0f
extern const float g_quarter;       // 0x54b868  0.25f





extern const float g_half;          // 0x549d70  0.5f

// ---- data this module owns (docs/standalone_data.md) ----
MapFlags g_map_flags;                                    // 0x5e291c (.bss) the commit-state block

// @ 0x513b10
unsigned __declspec(noinline) MissionCtx::pop_key()
{
    unsigned* p = (unsigned*)g_bit;

    do {
        unsigned k = *p;
        if (k & f3c) {
            f3c &= ~k;
            return k;
        }
        p += 2;
    } while ((int)p < (int)(g_bit + 32));
    return 0;
}

// @ 0x513b40
void MissionCtx::update_parties()
{
    WL_MARKER(12)
    if (f40 != 0) {
        f40 = 0;
        GV->clear_hero_ammo(g_game->f78);

        if (party[side] >= 0 && g_game->parties.is_live(party[side])) {
            int n = 0;

            GV->clear_hero_ammo(party[side]);
            g_game->parties.get(party[side])->dirty = 1;
            // only 0x490ce0's first counter is observable here, and it zeroes the slot itself, so all four
            // out-parameters alias the original's single stack home (the other three were dead)
            g_game->count_units(party[side], &n, &n, &n, &n);
        }
        if (f2c8 != 0) {
            if (party[1 - side] >= 0 && g_game->parties.is_live(party[1 - side])) {
                int n = 0;

                GV->clear_hero_ammo(party[1 - side]);
                g_game->parties.get(party[1 - side])->dirty = 1;
                g_game->count_units(party[1 - side], &n, &n, &n, &n);
            }
        }

        if (mode == 3 || (f164 & 1)) {
            unsigned h = f08;
            char* blk = (*(char***)((char*)g_game + 0xe4))[h >> 4];

            ((BattleRec*)(blk + (h & 0xf) * 0x98))->detach();
        }

        PartyMgr* pm = &g_game->parties;

        party_at(pm, pm->lookup(g_game->f78))->f88 = -1;
        party_at(pm, pm->lookup(g_game->f78))->action = 0;

        if (count_hero_items() > f168) {
            tstring msg("ui_some_soldiers_are_ready_to_upgrade");

            g_gui.push(*g_strtable.lookup(msg), 0xffbbffbb);
        }

        party_at(pm, pm->lookup(g_game->f78))->dirty = 1;
    }
    WL_MARKER(13)
}

// @ 0x513e10
int MissionCtx::count_hero_items()
{
    GameData* g = g_game;
    PartyMgr& m = g->parties;
    Party* p = m.at(m.lookup(g_game->f78));
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
        if (!(g->troops[s->troop].flags30 & 0x10))
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
        int idx = m->lookup(id);
        if (idx >= 0 && m->atu(idx)->type == 1) {
            Party* p = party_at(m, m->lookup(id));
            for (int i = 0; i < p->nstacks; i++) {
                if ((g->troops[p->stacks[i].troop].flags30 & 0x10) != 0 && p->stacks[i].f10 != 0)
                    p->stacks[i].f10 = 0;
            }
        }
    }
}

// @ 0x513f80
void MissionCtx::setup(unsigned flags, int delta)
{
    WL_MARKER(12)
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
        party_at(m, m->lookup(g_game->f78))->sum_stat(0x110);
        if (g_game->parties.is_live(party[side]))
            party_at(&g_game->parties, g_game->parties.lookup(party[side]))->sum_stat(0x110);
        if (mode == 4 && g_game->parties.is_live(party[1 - side]))
            g_game->parties.get(party[1 - side])->sum_stat(0x110);
        if (f1c > 6)
            f1c = 6;
        f27c = f1c - old;
        int i = 0;
        while (i < party_at(&g_game->parties, g_game->parties.lookup(g_game->f78))->nstacks)
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
            ((flags & 0x2000000) || (d <= 0 && passage_menu <= 0))) {
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
    WL_MARKER(13)
}

// a random index below n (the original inlined a helper of this form: its two zero returns stay separate blocks)
static inline int rand_below(int n) { if (n <= 1) return 0; return rand() % n; }
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
    if (n == 0)
        mb_error("Unable to find matching mission template for mission type %d.", kind);
    int idx = rand_below(n);
    SCENE->init(found[idx], 0, 0, f1c, this, f24, side);
}

// @ 0x5143f0
void MissionV::init(int tmpl, int a2, int a3, int a4, MissionCtx* ctx, int a6, int a7)
{
    mode = tmpl;
    MI(this, 0x6520) = a4;
    f5df4 = a2;
    f5df8 = a3;
    f6214 = a6;
    cnt[0] = 0;
    cnt[1] = 0;
    int p0 = ctx->party[0];
    if (p0 >= 0) {
        int* a = &cnt[0];
        int* b = (int*)list[0];
        GameData* g = g_game;
        *a = 0;
        g->list_members(p0, b, a);
    }
    int p1 = ctx->party[1];
    if (p1 >= 0) {
        int* a = &cnt[1];
        int* b = (int*)list[1];
        GameData* g = g_game;
        *a = 0;
        g->list_members(p1, b, a);
    }
    f6218 = a7;
    g_game->list_members(g_game->f78, (int*)(list[1] - a7 * 0x80), (int*)((char*)this + (0x1884 - a7) * 4));
    init_scene();
}

// @ 0x5144d0
void MissionV::init_scene()
{
    *((char*)this + 0x6339) = 0;
    ((std::vector<PlacedObj>*)((char*)this + 0x6688))->resize(0, PlacedObj());
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
    int i = 0;
    Team* tm = (Team*)((char*)this + 0x527c);
    for (; i < 8; i++, tm++)
        tm->init(i);
    rgl::vector4 pos;
    mat3 rot;
    FramePlacement out;
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;
    out.m = *identity3(&rot);
    *(rgl::vector4*)&out.a = pos;
    *(FramePlacement*)((char*)this + 0x6624) = out;
    g_terrain.reset();
    g_sound_mgr.stop_all();
    ((std::vector<int>*)((char*)this + 0x63c0))->resize(0, 0);
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
        g_gui.add(*g_strtable.lookup(s), -1);
    }
    save_game(-1, 0);
}

// The hero table is level record 1's spawn pairs (+0x114, id and value) and the int list at +0x314, reached through
// g_game for every store (the stores alias g_game, so it is reloaded after each).
struct HeroTable { char pad[0x494 + 0x114]; struct { int id, v; } pairs[0x40]; int flags[0x40]; };
#define HT ((HeroTable*)g_game->levels)

// @ 0x514890
void MissionCtx::build_hero_table(int a1, int a2, int a3, int a4)
{
    GD_I(0x13a08) = a1;
    for (int i = 0; i < 0x40; i++)
        HT->pairs[i].id = -1;
    HT->pairs[0].id = g_game->player;
    HT->flags[0] = 1;
    HT->pairs[1].id = a1;
    HT->pairs[1].v = a2;
    HT->flags[1] = 1;

    PartyMgr* m = &g_game->parties;
    Party* p = party_at(m, m->lookup(g_game->f78));
    int k = 2;
    for (int n = 1; n < p->nstacks; n++) {
        int item = p->stacks[n].troop;
        if ((g_game->troops[item].flags30 & 0x10) != 0 && item != a1) {
            HT->pairs[k].id = item;
            HT->flags[k] = 1;
            k++;
        }
    }

    if (a3 > 0 && a3 != g_game->f78) {
        m = &g_game->parties;
        if (1 < party_at(m, m->lookup(a3))->nstacks) {
            int k = 17;
            int n = 1;
            do {
                int item = party_at(m, m->lookup(a3))->stacks[n].troop;
                if ((g_game->troops[item].flags30 & 0x10) != 0) {
                    HT->pairs[k].id = party_at(m, m->lookup(a3))->stacks[n].troop;
                    HT->flags[k] = 1;
                    k++;
                }
                n++;
            } while (n < party_at(m, m->lookup(a3))->nstacks);
        }
    }

    SCENE->init(1, 1, 0, 0, this, f24, 0);
    MI(g_mission, 0x6558) = a1;
    MI(g_mission, 0x5270) = scene_counter;
    scene_counter++;
    SCENE->start_setup();
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    g_dialogs.start(a1, -1, a3, a4, 1);
}

// @ 0x514c00
void MissionCtx::start_battle(int a1, int a2, int a3, int a4)
{
    GD_I(0x13a08) = a1;
    f154 = a2;
    f158 = a3;
    f15c = a4;
    SCENE->init(a2, a3, a4, f1c, this, f24, side);
    MI(g_mission, 0x5270) = scene_counter;
    scene_counter++;
    SCENE->start_setup();
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    SCENE->frame(0.1f, 1);
    g_dialogs.start(a1, -1, party[side], 0, 1);
}

// @ 0x514cd0
void MissionCtx::start_mission()
{
    PartyMgr* pm = PARTIES;
    GameDataV* g = GV;
    int i = pm->lookup(party[side]);
    Party* p = pm->at(i);
    build_hero_table(p->stacks[0].troop, ((GameW*)g)->token_lookup(party[side], 0, 0, 0), party[side], 1);
}

static inline Rec98* rec98_at(std::vector<Rec98*>::iterator pages, unsigned idx)
{
    Rec98* page = *(pages + (idx >> 4));
    return page + (idx & 0xf);
}
static inline bool shadow_stale(void* target) { return ((ShadowMapP*)target)->f48[0x50 / 4] != g_batch.counter; }
static inline void set_map_view(rgl::frustum& c, float left, float right, float bottom, float top)
{
    c.perspective = 1;
    c.nearz = 0.2f;
    c.farz = 300.0f;
    c.left = left;
    c.right = right;
    c.bottom = bottom;
    c.top = top;
}

// @ 0x514d40 ---------------------------------------------------------------------------------------
void CampaignScreen::frame_update(float dt)
{
    float t = 0.0f;                                 // game hours since the clock's reference tick
    char flag;                                      // 1 while the map is idle
    int start;

    if (g_shadow_map.target != 0 && shadow_stale(g_shadow_map.target)
        && (int)g_shadow_map.cam == (int)g_script.level && g_use_shaders != 0)
        ((ShadowMapP*)&g_shadow_map)->render();

    set_map_view(cam, g_gui.view_a * -0.1f, g_gui.view_a * 0.1f, g_gui.view_b * -0.095f, g_gui.view_b * 0.095f);
    cam.build_planes();
    f30->scene3d.fr = cam;

    __int64 cur = g_ticks[GD_I(0x13b18)];
    __int64 ref = *(__int64*)((char*)g_game + 0x13b20);
    t = (float)(cur - ref) * 1e-5f;

    float hour = (float)fmod((double)t, 24.0);

    float sz[2];
    PSO(f5c)->set_text(sz,
        hour < 1.0f  ? g_strtable.lookup("ui_midnight") :
        hour < 4.0f  ? g_strtable.lookup("ui_late_night") :
        hour < 6.0f  ? g_strtable.lookup("ui_dawn") :
        hour < 8.0f  ? g_strtable.lookup("ui_early_morning") :
        hour < 11.0f ? g_strtable.lookup("ui_morning") :
        hour < 13.0f ? g_strtable.lookup("ui_noon") :
        hour < 16.0f ? g_strtable.lookup("ui_afternoon") :
        hour < 18.0f ? g_strtable.lookup("ui_late_afternoon") :
        hour < 20.0f ? g_strtable.lookup("ui_dusk") :
        hour < 23.0f ? g_strtable.lookup("ui_evening") :
                       g_strtable.lookup("ui_midnight"));

    int hi = (int)t;
    GameData* ge = g_game;
    g_script.result_string.assign("");
    ge->emit_event(0xf, hi / 24, hi);
    {
        tstring tmp(g_script.result_string);
        PSO(f60)->set_text(sz, &tmp);
    }

    flag = 1;
    if (!(GD_F(0x13ad0) <= 0.0f))
        flag = 0;
    if (GD_C(0x18) == 0 && g_game_keys.clicked(0x24))
        quick_save_msg();
    if (GD_I(0x1a208) >= 0) {
        g_script.param1 = GD_I(0x1a208);
        ((TimerList*)((char*)g_game + 0x139f4))->update(-8);
        GD_I(0x1a208) = -1;
    }

    int cmd = GD_I(0xc8);
    if (cmd == 0x12) {
        GD_I(0xc8) = 0;
        slot_as<MenuWidget>(23)->f18 = 7;
        g_windows.set_state(0x17);
        return;
    }
    if (cmd == 0xf) {
        GD_I(0xc8) = 0;
        slot_as<MenuWidget>(27)->f18 = 7;
        g_windows.set_state(0x1b);
        return;
    }
    if (cmd == 0x10) {
        GD_I(0xc8) = 0;
        slot_as<MenuWidget>(26)->f18 = 7;
        g_windows.set_state(0x1a);
        return;
    }
    if (g_keyboard.down[0x1] != 0 && g_keyboard.prev[0x1] == 0) {
        slot_as<MenuWidget>(5)->f18 = 7;
        g_windows.set_state(5);
        return;
    }
    if (f48 != 0 && PSO(f48)->clicked_on(0x10000)) {
        slot_as<MenuWidget>(22)->f18 = 7;
        slot_as<MenuWidget>(22)->f4 = 0;
        g_windows.set_state(0x16);
        return;
    }
    if (flag) {
        if (PSO(f50)->clicked_on(0x10000) || g_game_keys.clicked(0x12)) {
            if (g_module_settings.show_quest_notes != 0) {
                slot_as<MenuWidget>(26)->f18 = 7;
                GD_I(0x13b08) = 4;
                GD_I(0x13b0c) = 0;
                g_windows.set_state(0x1a);
                return;
            }
            slot_as<MenuWidget>(13)->f18 = 7;
            slot_as<MenuWidget>(13)->f4 = 0;
            g_windows.set_state(0xd);
            return;
        }
    }

    GameData* g = g_game;
    if (f64 != 0) {
        WL_MARKER(12)
        if (g_saved == 0)
            save_game(-1, 0);
        g_saved = 1;
        WL_MARKER(13)
        return;
    }
    if (*((char*)g + 0x13ac8) != 0) {
        g_windows.box()->setup((tstring*)((char*)g + 0x13ab8), (tstring*)((char*)g + 0x13aa8));
        g_windows.open(0);
        GD_C(0x13ac8) = 0;
        return;
    }
    if (*((char*)g + 0x13a3c) != 0) {
        *((char*)g + 0x13a3c) = 0;
        g_mission_ctx.build_hero_table(GD_I(0x13a44), -1, -1, GD_I(0x13a40));
        slot_as<MenuWidget>(16)->f18 = 7;
        g_windows.set_state(0x10);
        return;
    }

    {
    int fpid = g->f78;
    PartyMgr* pm = &g->parties;
    if (pm->get(fpid)->f88 >= 0) {
    unsigned idx1 = pm->get(fpid)->f88;
    std::vector<Rec98*>::iterator pages = ((std::vector<Rec98*>*)((char*)g + 0xe0))->begin();
    if (rec98_at(pages, idx1)->f10 == 1) {
        Rec98* node = rec98_at(pages, pm->get(fpid)->f88);
        WL_MARKER(12)
        start = 1;
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
        int trial = g_trial_mode;
        if (g_game->troops[g_game->player].level > 6 && trial == 0) {
            {
            tstring a("ui_explanation_level_limit");
            tstring b("ui_level_limit_reached");
            g_windows.box()->setup(g_strtable.lookup(b), g_strtable.lookup(a));
            }
            g_windows.open(trial);
            f64 = 1;
            start = 0;
        } else if (1 - (int)(t * -0.041666667f) > 30 && trial == 0) {
            {
            tstring a("ui_explanation_time_limit");
            tstring b("ui_time_limit_reached");
            g_windows.box()->setup(g_strtable.lookup(b), g_strtable.lookup(a));
            }
            g_windows.open(trial);
            f64 = 1;
            start = 0;
        }
        WL_MARKER(13)

        if (!start)
            return;

    g_mission_ctx.init_battle(PARTIES->get(GD_I(0x78))->f88);
    GD_I(0x13aec) = 0;
    GD_I(0x13af8) = 0;
    ((TimerList*)((char*)g_game + 0x139f4))->update(-6);
    g_game->emit_event(1, GD_I(0x13a0c), GD_I(0x13a10));
    int id;
    if (GD_I(0x13aec) > 0) {
        g_mission_ctx.frame(GD_I(0x13aec));
        int n = GD_I(0x13aec);
        id = PARTIES->get(GD_I(0x78))->f88;
        g_map_flags.f8 = n;
    } else {
        g_mission_ctx.frame(-1);
        g_mission_ctx.f2c8 = 1;
        id = PARTIES->get(GD_I(0x78))->f88;
        g_map_flags.f8 = -1;
    }
    g_map_flags.f4 = id;
    g_map_flags.f0 = 1;
    g_map_flags.f1 = 0;
    g_map_flags.f2 = 0;
    g_map_flags.commit(&flag);
    return;
    }
    }
    }

    if (flag) {
        bool c_f40 = PSO(f40)->clicked_on(0x10000) || g_game_keys.clicked(0x10);
        if (c_f40) {
            WL_MARKER(4)
            slot_as<MenuWidget>(8)->f18 = 7;
            slot_as<MenuWidget>(8)->f4 = 0;
            g_windows.set_state(8);
            WL_MARKER(5)
            return;
        }
        bool c_f3c = PSO(f3c)->clicked_on(0x10000) || g_game_keys.clicked(0x11);
        if (c_f3c) {
            slot_as<MenuWidget>(12)->f18 = 7;
            slot_as<MenuWidget>(12)->f4 = 0;
            g_windows.set_state(0xc);
            return;
        }
        bool c_f44 = PSO(f44)->clicked_on(0x10000) || g_game_keys.clicked(0xf);
        if (c_f44) {
            slot_as<MenuWidget>(15)->f18 = 7;
            slot_as<MenuWidget>(15)->f4 = 0;
            slot_as<MenuWidget>(15)->f14 = GD_I(0x8c);
            g_windows.set_state(0xf);
            return;
        }
        if (g_game_keys.clicked(0x13)) {
            slot_as<MenuWidget>(25)->f18 = 7;
            slot_as<MenuWidget>(25)->f4 = 0;
            g_windows.set_state(0x19);
            return;
        }
        if (f4c != 0 && PSO(f4c)->clicked_on(0x10000)) {
            GameDataV* gv = GV;
            gv->attach_party_center(&gv->parties.get(gv->f78)->pos.x, gv->f78, *(int*)((char*)gv + 0x80));
            return;
        }
    }
    if (f38 != 0 && PSO(f38)->clicked_on(0x10000)) {
        GD_I(0x13aec) = 4;
        slot_as<MenuWidget>(23)->f18 = 7;
        g_windows.set_state(0x17);
        return;
    }
    if (GD_F(0x14058) == 0.0f)
        PSO(f54)->act_b4();
    else
        PSO(f54)->clear();
    g_gui.flush();
    if (GD_F(0x13ad0) > 0.0f && GD_I(0x13ad8) != 0) {
        PSO(f58)->act_b4();
        if (g_keyboard.down[0xe0] == 0 && g_keyboard.prev[0xe0] != 0)
            GD_F(0x13ad0) = 0.0f;
    } else {
        PSO(f58)->clear();
    }
    if (!(GD_F(0x13ad0) > 0.0f && GD_I(0x13ad8) != 0))
        camera_move(dt);
    camera_update(dt);
    if (GD_F(0x13ad0) <= 0.0f) {
        ((MapPanel*)this)->update_target();
        f2cc += dt;
        f2d0 += dt;
        GD_I(0x13aa4) = 0;
        {
            GameData* gb = g_game;
            int bpid = gb->f78;
            PartyMgr* bpm = &gb->parties;
            if (bpm->get(bpid)->action == 4 || bpm->get(bpid)->action == 1
                || bpm->get(bpid)->action == 6 || bpm->get(bpid)->action == 9)
                *(int*)((char*)gb + 0x13aa4) = 1;
        }
        if (g_keyboard.down[57] != 0) {
            GameData* gk = g_game;
            gk->parties.get(gk->f78)->action = 0;
            PARTIES->get(GD_I(0x78))->reissue_order();
            GD_I(0x13aa4) = 2;
        }
    }

    GameG* tg = (GameG*)GV;
    tg->tick(dt);
    rgl::matrix m;
    rgl::identity_pos(&m);
    {
        float vx = 0.0175f - g_gui.view_a * 0.1975f;
        float vy = 0.015f - g_gui.view_b * 0.18f;
        rgl::vector4 v(vx, vy, -0.4f);
        m.o = ((pos*)&cam.m)->transform_point(v);
    }
    (*(FrameRenderer**)((char*)g_game + 0x1a2a8))->set_frame(&m);

    if (g_cfg.enable_edit_mode != 0 && (g_keyboard.down[0x1d] != 0 || g_keyboard.down[0x9d] != 0) && (char)g_keyboard.is_ready(0x12))
        GD_I(0x1a090) = (GD_I(0x1a090) == 0);

    if (GD_I(0x1a090) != 0) {
        char buf[0x200];
        int bpid = GD_I(0x78);
        PartyMgr* bpm = PARTIES;
        sprintf(buf, "Position: %3.2f, %3.2f ;",
                (double)bpm->get(bpid)->pos.x, (double)bpm->get(bpid)->pos.y);
        text_stream& log = g_debug_log;     // reached through a reference: the calls stay virtual (+0x28), as in the original
        log.w_cstr(";-------------------;");
        log.w_cstr(buf);
        log.w_cstr(";-------------------;");
    }
}

struct Vec2f { float x, y; Vec2f() {} Vec2f(float a, float b) : x(a), y(b) {} const float* ptr() const { return &x; } };
static inline Vec2f operator*(const Vec2f& a, const float& s) { return Vec2f(a.x * s, a.y * s); }
// @ 0x515d80
void CampaignScreen::camera_move(float dt)
{
    mat2 m(Vec2f(1.0f, 0.0f).ptr(), Vec2f(0.0f, 1.0f).ptr());   // the identity: a = {1,0} at [esp+0xc], b = {0,1} at [esp+4] (0x515d94)
    m.rotate_2x2(-yaw);

    bool b = false; if (g_game_keys.entry[2][0] >= 0 && g_keyboard.down[g_game_keys.entry[2][0]] != 0) b = true;
    if ((g_game_keys.entry[2][1] >= 0 && g_keyboard.down[g_game_keys.entry[2][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[0] * (height + 10.0f) * dt;
        x -= d.x;
        z -= d.y;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }
    b = false; if (g_game_keys.entry[3][0] >= 0 && g_keyboard.down[g_game_keys.entry[3][0]] != 0) b = true;
    if ((g_game_keys.entry[3][1] >= 0 && g_keyboard.down[g_game_keys.entry[3][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[0] * (height + 10.0f) * dt;
        x = d.x + x;
        z = d.y + z;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }
    b = false; if (g_game_keys.entry[0][0] >= 0 && g_keyboard.down[g_game_keys.entry[0][0]] != 0) b = true;
    if ((g_game_keys.entry[0][1] >= 0 && g_keyboard.down[g_game_keys.entry[0][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[1] * (height + 10.0f) * dt;
        x = d.x + x;
        z = d.y + z;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }
    b = false; if (g_game_keys.entry[1][0] >= 0 && g_keyboard.down[g_game_keys.entry[1][0]] != 0) b = true;
    if ((g_game_keys.entry[1][1] >= 0 && g_keyboard.down[g_game_keys.entry[1][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[1] * (height + 10.0f) * dt;
        x -= d.x;
        z -= d.y;
        GD_C(0x88) = 0;
        GD_I(0x84) = g_game->f78;
    }

    if (g_keyboard.down[0x4c]) speed += dt * 1.1f;
    if (g_keyboard.down[0x50]) speed -= dt * 1.1f;
    if (g_keyboard.down[0x4f]) vy += dt * 1.1f;
    if (g_keyboard.down[0x51]) vy -= dt * 1.1f;

    if (g_keyboard.down[0xe1] && !g_keyboard.prev[0xe1])
        f2d4 = 0;

    float v = g_gui.mouse_dx * 0.1f;
    if (v < -1.0f) v = -1.0f;
    else if (v > 1.0f) v = 1.0f;

    float w = g_gui.mouse_dy * 0.1f;
    if (w < -2.0f) w = -2.0f;
    else if (w > 2.0f) w = 2.0f;

    if (g_keyboard.axis[0] != 0.0f)
        f2d4 = 1;

    vy = v * dt + vy;
    if (g_keyboard.down[0xe1]) {
        float wheel = expf(4.0f * g_options.exp_arg) * 0.03f;
        vy = wheel * g_keyboard.axis[0] * dt + vy;
    }

    if (w != 0.0f || g_keyboard.axis[2] != 0.0f) {
        y = y - w * height * dt;
        if (g_keyboard.axis[2] > 0.0f)
            y = y - height * 0.2f;
        else if (g_keyboard.axis[2] < 0.0f)
            y = height * 0.2f + y;
    }
}

// @ 0x516220
__declspec(noinline) mat2::mat2(const float* a, const float* b)
{
    m[0] = a[0];
    m[1] = a[1];
    m[2] = b[0];
    m[3] = b[1];
}

// the scene record behind an id, through the vector at game+0xe0 (materialised page address, 0x5162b0)
static __forceinline Rec98& node_of(unsigned id)
{
    std::vector<Rec98*>& pages = *(std::vector<Rec98*>*)((char*)g_game + 0xe0);
    return pages[id >> 4][id & 0xf];
}
// the same with the page in a local: the two f54 reads have the index first in the SIB ([eax+ecx+0x54], 0x5162ef, 0x516329)
static __forceinline Rec98& node_of2(unsigned id)
{
    std::vector<Rec98*>& pages = *(std::vector<Rec98*>*)((char*)g_game + 0xe0);
    Rec98* page = pages[id >> 4];
    return page[id & 0xf];
}
// @ 0x516250
void MissionCtx::init_battle(int id_)
{
    WL_MARKER(12)
    init();           // 0x4be742 (called through its WinLicense entry 0x4be730)
    f08 = id_;
    f40 = 1;
    side = 0;
    f144 = 1;
    f10 = 1;
    mode = 3;

    if (g_game->f78 == node_of(id_).f54) {
        party[0] = node_of(id_).f58;
        party[1] = -1;
        f24 = 1;
        f144 = 1;
    } else if (g_game->f78 == node_of(id_).f58) {
        party[0] = node_of2(id_).f54;
        party[1] = -1;
        f24 = 0;
        f144 = 1;
    } else if (node_of(id_).f10 == 1) {
        party[0] = node_of(id_).f58;
        party[1] = node_of2(id_).f54;
        f10 = 0x11;
        f144 = 0;
        f24 = 2;
        mode = 4;
    }
    GD_C(0x13a18) = 1;
    WL_MARKER(13)
}

// _Vector_iterator<T*>::operator+ left out of line in this function (the original calls the COMDAT, 0x516393; map_editor/exchange_screens.cpp)
struct VecIter {
    int* p;
    __declspec(noinline) int** advance(int** out, int n);   // 0x534160 (ret 8)
};
static __forceinline Rec98& node_call(unsigned id)
{
    VecIter it;
    it.p = *(int**)((char*)g_game + 0xe4);
    int* slot;
    Rec98* page = *(Rec98**)*it.advance(&slot, id >> 4);
    return page[id & 0xf];
}
// @ 0x516370
void MissionCtx::frame(int tick)
{
    WL_MARKER(12)
    passage_menu = tick;
    if (f24 == 2 && tick <= 0) {
        party[0] = node_call(f08).f58;
        party[1] = node_call(f08).f54;

        // the record is looked up again for each test (0x516447, 0x5164cf), not read from party[]
        int ida = node_call(f08).f58;
        int a = 0;
        int b = 0;
        GameData* ga = g_game;
        PartyMgr* pa = &ga->parties;
        Party* qa = party_at(pa, pa->lookup(ida));
        if (*(float*)((char*)ga->factions + qa->f70 * 0x300 + 0x2c) < 0.0f)
            a = 1;
        int idb = node_call(f08).f54;
        GameData* gb = g_game;
        PartyMgr* pb = &gb->parties;
        Party* qb = party_at(pb, pb->lookup(idb));
        if (*(float*)((char*)gb->factions + qb->f70 * 0x300 + 0x2c) < 0.0f)
            b = 1;
        if (a != 0 && b == 0) {
            side = 0;
            f10 &= ~0x10;
            f144 = 1;
        } else if (a == 0 && b != 0) {
            side = 1;
            f10 &= ~0x10;
            f144 = 1;
        } else {
            side = 0;
            f10 |= 0x10;
            f144 = 0;
        }
    }
    if (f144 != 0) {
        refresh_rating();
        if (passage_menu <= 0)
            f3c |= 1;
    }
    GameData* gc = g_game;
    PartyMgr* pc = &gc->parties;
    Party* p = party_at(pc, pc->lookup(gc->f78));
    if ((char)((PartyV*)p)->data_ready())
        f10 |= 2;

    if (g_game->f78 == node_call(f08).f58)
        f10 |= 4;
    else if (g_game->f78 == node_call(f08).f54)
        f10 |= 8;

    WL_MARKER(13)
    update_anim();
}

// @ 0x5166a0  Party::data_ready
int PartyV::data_ready()
{
    int clk = *(int*)((char*)this + 0x59b0);
    __int64 start = *(__int64*)((char*)this + 0x59b8);
    if ((*(__int64*)&flags_lo & 0x1000000) == 0) {
        __int64 d = g_ticks[clk] - start;
        if (!((float)d * 1e-5f < g_quarter))
            return 0;
    }
    return 1;
}

static inline int hero_rank(Troop* t) { int k = ((char*)t)[0x7a] & 0xf; if (k < 0) k = 0; else if (k > (*(int*)&g_skill_tbl[36].max_rank)) k = (*(int*)&g_skill_tbl[36].max_rank); return k; }
// @ 0x516700
void MissionCtx::update_anim()
{
    WL_MARKER(12)                                                   // 0x516705
    scene_counter = 0;
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
        Party* p = m->at(m->lookup(party[0]));
        int slot = p->stacks[0].troop;
        if (slot >= 0) {
            if ((g->troops[slot].flags30 & 0x10) != 0) {
                Troop* t = &g->troops[slot];
                int f = t->f58[0];
                float g = (float)(int)((float)(f + hero_rank(t) * 2 + 0x23) * 0.5f + 3.0f);
                if (g > t->hp)          // fcom; test ah,0x41; jne skip (0x516834): a NaN hp skips
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
    WL_MARKER(13)                                                   // 0x51688d (also blocks the tail call)
}

// @ 0x5168b0  clear the 0x60 (stride 8) trade ids of the record selected by f90
void GameDataV::reset_trade_ids()
{
    Troop* t = &troops[*(int*)((char*)this + 0x90)];
    for (int i = 0; i < 0x60; i++)
        t->inv[i].item = -1;
}

// @ 0x5168e0  rebuild a party's id grid (the 0x20-slot rows at party+0xa8)
void GameDataV::fill_ids(int* out, int id)
{
    PartyMgr* m = &parties;
    Party* rec = m->at(m->lookup(id));
    int row = 0;
    if (rec->nstacks > 0) {
        int* base = out;
        do {
            int* o = base;
            for (int j = 0; j < rec->stacks[row].count; j++, o++) {
                if (j >= 0x20)
                    break;
                int v = -1;
                if (id >= 0)
                    v = m->get(id)->stacks[row].troop;
                *o = v * 0x1bf + (id * 0x7bb7 + j) + 0x45db;
            }
            row++;
            base += 0x20;
        } while (row < rec->nstacks);
    }
}

// @ 0x5169e0
void MissionCtx::reset_ids(int id)
{
    if (id == -1)
        return;
    PartyMgr* m = &g_game->parties;
    Party* rec = m->at(m->lookup(id));
    for (int i = 0; i < rec->nstacks; i++) {
        Stack& s = rec->stacks[i];
        if ((g_game->troops[s.troop].flags30 & 0x10) != 0) {
            s.wounded = 0;
            s.count = 1;
        }
    }
}

// @ 0x516a70
void quick_save_msg()
{
    {
        tstring t("ui_quick_saving");
        g_gui.add(*g_strtable.lookup(t), -1);
    }
    save_game(-1, 0);
}
