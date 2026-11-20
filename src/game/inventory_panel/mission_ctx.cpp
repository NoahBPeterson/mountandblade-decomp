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
namespace rgl { struct strategic_entity; }
// 0x43f310 (Mission::prop_ents; agent4/agent_logic.cpp instantiates it and ICF folds it with vector<int>::resize there).
// The element type is the strategic entity: vector<rgl::entity*>::resize is the rgl objects' 0x41fa00 (entity.cpp's
// vector<Obj*> copy), and a symbol names one COMDAT, so prop_ents cannot be that instantiation.
template<> void std::vector<rgl::strategic_entity*, std::allocator<rgl::strategic_entity*> >::resize(size_t, rgl::strategic_entity*);
template<> void std::vector<PlacedObj, std::allocator<PlacedObj> >::resize(size_t, PlacedObj);   // 0x4777c0 (terrain_b/terrain_b.cpp; ret 0x1cc: the 0x1c8 object by value, then the count)
int save_game(int id, tstring* name);                  // 0x4e8980
#include "../dialog_mgr.h"                              // DialogMgr g_dialogs (0x8ed50c8)


static inline tstring* lookup_ref(const tstring& k) { return g_strtable.lookup(k); }
#include "../event.h"                                  // Event: the 0x98-byte record of GameData::events (pages at game+0xe4)
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
#include "../../rgl/scene/scene_entity.h"   // rgl::entity: GameData::compass (set_local_dirty)
#include "../window_mgr.h"      // slot_as<T>(i)
#include "../../include/render_flags.h"
#include "../shadow_map.h"
#include "../ui_globals.h"      // UI / core globals
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../../include/sound_mgr.h"   // g_sound_mgr 0x5aa128
// the party accessors: visible here too, as header inlines in the original (the same definitions as
// inventory_panel/note_window.cpp; the row owners win the link). cl then knows the calls write no memory and keeps
// the page vector in a register across them (count_hero_items, setup, start_mission). clear_hero_ammo 0x513eb0, which
// reloads it after the call, was compiled without them: it is in its own unit (hero_ammo.cpp).
inline int Map10007::lookup(unsigned id)
{
    if (id < 0x2717)
        return tab[id].value;
    int h = id % 0x2717;
    int n = 0;
    if (tab[h].key != id) {
        do {
            if (n >= 1000)
                return -1;
            h = (h + 0x4f) % 0x2717;
            n++;
        } while (tab[h].key != id);
    }
    if (n < 1000)
        return tab[h].value;
    return -1;
}
__declspec(noinline) Party* PartyMgr::get(int id)
{
    int k = this->lookup(id);
    return pages._Myfirst[k >> 4] + (k & 0xf);
}
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
        if (k & key_mask) {
            key_mask &= ~k;
            return k;
        }
        p += 2;
    } while ((intptr_t)p < (intptr_t)(g_bit + 32));
    return 0;
}

// @ 0x513b40
void MissionCtx::update_parties()
{
    WL_MARKER(12)
    if (dirty != 0) {
        dirty = 0;
        GV->clear_hero_ammo(g_game->main_party);

        if (party[side] >= 0 && g_game->parties.is_live(party[side])) {
            int a, b, c, d;

            GV->clear_hero_ammo(party[side]);
            g_game->parties.get(party[side])->dirty = 1;
            a = 0;
            b = 0;
            c = 0;
            d = 0;
            g_game->count_units(party[side], &a, &b, &c, &d);
        }
        if (party_battle_mode != 0) {
            if (party[1 - side] >= 0 && g_game->parties.is_live(party[1 - side])) {
                int a, b, c, d;

                GV->clear_hero_ammo(party[1 - side]);
                g_game->parties.get(party[1 - side])->dirty = 1;
                a = 0;
                b = 0;
                c = 0;
                d = 0;
                g_game->count_units(party[1 - side], &a, &b, &c, &d);
            }
        }

        if (mode == 3 || (battle_flags & 1)) {
            unsigned h = battle_event;
            Event* page = g_game->events.pages._Myfirst[h >> 4];

            (page + (h & 0xf))->detach();                   // 0x4b6180 (party2/party_2.cpp)
        }

        PartyMgr* pm = &g_game->parties;

        party_at(pm, pm->lookup(g_game->main_party))->battle = -1;
        party_at(pm, pm->lookup(g_game->main_party))->action = 0;

        if (count_hero_items() > upgradable) {
            tstring msg("ui_some_soldiers_are_ready_to_upgrade");

            g_gui.push(*g_strtable.lookup(msg), 0xffbbffbb);
        }

        party_at(pm, pm->lookup(g_game->main_party))->dirty = 1;
    }
    WL_MARKER(13)
}

// @ 0x513e10
int MissionCtx::count_hero_items()
{
    GameData* g = g_game;
    PartyMgr& m = g->parties;
    Party* p = m.at(m.lookup(g_game->main_party));
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

// (0x513eb0 GameDataV::clear_hero_ammo: its own unit, hero_ammo.cpp)

// @ 0x513f80
void MissionCtx::setup(unsigned flags, int delta)
{
    WL_MARKER(12)
    int old = advantage;
    if (flags & 1)
        f20 = 1;
    if (flags & 2)
        f20 = 1;
    if (flags & 4)
        key_mask |= 1;
    if (flags & 0x10) {
        select_side(0);
        key_mask |= 0x400;
    }
    if (flags & 0x20) {
        select_side(1);
        key_mask |= 0x400;
    }
    if (flags & 0x1000)
        mission_flags = mission_flags & ~4;
    else if (flags & 0x2000)
        mission_flags = mission_flags | 4;
    if (flags & 0x4000)
        mission_flags = mission_flags & ~8;
    else if (flags & 0x8000)
        mission_flags = mission_flags | 8;
    if (flags & 0x40) {
        mission_flags = mission_flags | 0xa0;
        party_battle_mode = 1;
        key_mask |= 0x400;
    }
    if ((mode == 3 || mode == 4) && (mission_flags & 0x10) == 0) {
        int n = (int)flags >> 8 & 0xf;
        if (n > 8)
            n = 8 - n;
        advantage = g_game->gate_rating(g_game->main_party, party[side], party[1 - side]) + n;
        PartyMgr* m = &g_game->parties;
        party_at(m, m->lookup(g_game->main_party))->sum_stat(0x110);
        if (g_game->parties.is_live(party[side]))
            party_index(party[side])->sum_stat(0x110);
        if (mode == 4 && g_game->parties.is_live(party[1 - side]))
            g_game->parties.get(party[1 - side])->sum_stat(0x110);
        if (advantage > 6)
            advantage = 6;
        advantage_delta = advantage - old;
        int i = 0;
        while (i < party_index(g_game->main_party)->nstacks)
            i++;
        int l[8];
        unsigned ps = party[side];
        l[6] = 0;
        l[4] = 0;
        l[2] = 0;
        l[0] = 0;
        g_game->count_units(ps, &l[6], &l[4], &l[2], &l[0]);
        int d = -l[0];
        if (mode == 4) {
            unsigned po = party[1 - side];
            l[7] = 0;
            l[5] = 0;
            l[3] = 0;
            l[1] = 0;
            g_game->count_units(po, &l[7], &l[5], &l[3], &l[1]);
        }
        if (((mission_flags & 0x80) != 0 || party_battle_mode != 0) &&
            ((flags & 0x2000000) || (d <= 0 && passage_menu <= 0))) {
            battle_flags = 0x143;
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
        int v = key_mask | 0x200;
        f21 = 1;
        key_mask = v;
        g_game->f13a18 = 0;
    }
    WL_MARKER(13)
}

// a random index below n (the original inlined a helper of this form: its two zero returns stay separate blocks)
static inline int rand_below(int n) { if (n <= 1) return 0; return rand() % n; }
// @ 0x514360
void MissionCtx::pick_template()
{
    int kind = type;
    template_type = kind;
    int found[32];
    int n = 0;
    for (int i = 0; i < g_mission_template_count; i++) {
        if (g_mission_templates[i].type == kind)
            found[n++] = i;
    }
    if (n == 0)
        mb_error("Unable to find matching mission template for mission type %d.", kind);
    int idx = rand_below(n);
    SCENE->init(found[idx], 0, 0, advantage, this, player_side, side);
}

// @ 0x5143f0
void MissionV::init(int tmpl, int a2, int a3, int a4, MissionCtx* ctx, int a6, int a7)
{
    mode = tmpl;
    advantage = a4;
    scene_idx = a2;
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
    g_game->list_members(g_game->main_party, list[1 - a7], &cnt[1 - a7]);
    init_scene();
}

// @ 0x5144d0
void MissionV::init_scene()
{
    f6339 = 0;
    props.resize(0, PlacedObj());
    flags = g_mission_templates[mode].flags;
    f6554 = 0;
    muted = true;
    g_game->screen_troop = -1;
    g_game->jump_scene = -1;
    g_game->jump_entry = 0;
    g_game->jump_mission = 0;
    g_game->talk_enabled = 0;
    g_game->talk_disabled = 0;
    conversation = -1;
    dismount_count = 0;
    player_id = -1;
    f60 = -1;
    int i = 0;
    Team* tm = (Team*)teams;                            // mission_ctx.h's Team is TeamRec with its reset
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
    (FramePlacement&)view = out;
    g_terrain.reset();
    g_sound_mgr.stop_all();
    prop_ents.resize(0, 0);
    announce_reinforcements = 0;
    announce_enemies = 0;
    ordered_blunt = 0;
    ordered_hold_fire = 0;
    ordered_dismount = 0;
    str.assign("");
}

// @ 0x514670
void Team::init(int idx)
{
    leader = -1;
    f0 = 0;
    for (int j = 0; j < 3; j++)
        cls[j].init();
    int i = 0;
    int* p = relation;
    do {
        if (idx == 7)
            *p = 0;
        else
            *p = (i == idx) * 2 - 1;
        i++;
        p++;
    } while (i < 8);
    relation[7] = 0;
}

// @ 0x5146f0
void Sub3::init()
{
    kind = 0;
    k4 = 0;
    a = 0;
    b = 0;
    Lane4 v(20.0f, 20.0f, 20.0f);
    pos = v;
    use = 1;
    saved_use = 1;
    dir = Lane4(0.0f, 0.001f, 0.0f);
    sum = v;
    half = 10.0f;
    mode = 2;
    nav = -1;
    target = Lane4(0.0f, 0.0f, 0.0f);
}

// @ 0x5147e0
void MissionCtx::start_scene(int a1, int a2, int a3)
{
    mission_template = a1;
    f158 = a2;
    f15c = a3;
    SCENE->init(a1, a2, a3, advantage, this, player_side, side);
}

// @ 0x514830
void autosave_notice()
{
    if (g_game->fb4 != 0)
        return;
    {
        tstring s("ui_autosaving");
        g_gui.add(*g_strtable.lookup(s), -1);
    }
    save_game(-1, 0);
}

// The hero table is scene 1's visitor slots (troop and dna) and visitor counts, reached through g_game for every store
// (the stores alias g_game, so it is reloaded after each).
#include "../terrain_b/scene_file.h"    // SceneFile: the scene record (visitors, visitor_counts)
static inline SceneFile* hero_scene() { return &g_game->scenes[1]; }

// @ 0x514890
void MissionCtx::build_hero_table(int a1, int a2, int a3, int a4)
{
    g_game->f13a08 = a1;
    for (int i = 0; i < 0x40; i++)
        hero_scene()->visitors[i].troop = -1;
    hero_scene()->visitors[0].troop = g_game->player;
    hero_scene()->visitor_counts[0] = 1;
    hero_scene()->visitors[1].troop = a1;
    hero_scene()->visitors[1].dna = a2;
    hero_scene()->visitor_counts[1] = 1;

    Party* p = party_index(g_game->main_party);
    int k = 2;
    for (int n = 1; n < p->nstacks; n++) {
        if ((g_game->troops[p->stacks[n].troop].flags30 & 0x10) != 0 && p->stacks[n].troop != a1) {
            hero_scene()->visitors[k].troop = p->stacks[n].troop;
            hero_scene()->visitor_counts[k] = 1;
            k++;
        }
    }

    if (a3 > 0 && a3 != g_game->main_party) {
        int n = 1;
        if (n < party_index(a3)->nstacks) {
            int k = 17;
            do {
                int item = party_index(a3)->stacks[n].troop;
                if ((g_game->troops[item].flags30 & 0x10) != 0) {
                    hero_scene()->visitors[k].troop = party_index(a3)->stacks[n].troop;
                    hero_scene()->visitor_counts[k] = 1;
                    k++;
                }
                n++;
            } while (n < party_index(a3)->nstacks);
        }
    }

    SCENE->init(1, 1, 0, 0, this, player_side, 0);
    g_mission->talk_troop = a1;
    g_mission->scene_counter = scene_counter;
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
    g_game->f13a08 = a1;
    mission_template = a2;
    f158 = a3;
    f15c = a4;
    SCENE->init(a2, a3, a4, advantage, this, player_side, side);
    g_mission->scene_counter = scene_counter;
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
    build_hero_table(p->stacks[0].troop, g->token_lookup(party[side], 0, 0, 0), party[side], 1);
}

static inline Event* event_at(std::vector<Event*>::iterator pages, unsigned idx)
{
    Event* page = *(pages + (idx >> 4));
    return page + (idx & 0xf);
}
static inline bool shadow_stale(skin* target) { return target->tex[0]->last_frame != g_batch.counter; }   // the shadow skin's texture was not drawn this frame
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

    bool stale = false;
    if (g_shadow_map.target != 0)
        stale = shadow_stale(g_shadow_map.target);
    if (stale && (void*)g_shadow_map.cam == (void*)g_script.level && g_use_shaders != 0)
        g_shadow_map.render();

    set_map_view(cam, g_gui.view_a * -0.1f, g_gui.view_a * 0.1f, g_gui.view_b * -0.095f, g_gui.view_b * 0.095f);
    cam.build_planes();
    f30->scene3d.fr = cam;

    __int64 cur = g_ticks[g_game->f13b18];
    __int64 ref = g_game->clock;
    t = (float)(cur - ref) * 1e-5f;

    float hour = (float)fmod((double)t, 24.0);
    {
        f5c->set_text(*(
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
                       g_strtable.lookup("ui_midnight")));
    }

    int hi = (int)t;
    GameData* ge = g_game;
    g_script.result_string.assign("");
    ge->emit_event(0xf, hi / 24, hi);
    {
        tstring tmp(g_script.result_string);
        f60->set_text(tmp);
    }

    flag = g_game->rest_period <= 0.0f;
    if (g_game->flag18 == 0 && g_game_keys.clicked(0x24))
        quick_save_msg();
    if (g_game->question_answer >= 0) {
        g_script.param1 = g_game->question_answer;
        ((TimerList&)g_game->simple).update(-8);           // the simple triggers (TimerList's update 0x48add0)
        g_game->question_answer = -1;
    }

    int cmd = g_game->mode;
    if (cmd == 0x12) {
        g_game->mode = 0;
        slot_as<head_rec>(23)->w18 = 7;
        g_windows.set_state(0x17);
        return;
    }
    if (cmd == 0xf) {
        g_game->mode = 0;
        slot_as<head_rec>(27)->w18 = 7;
        g_windows.set_state(0x1b);
        return;
    }
    if (cmd == 0x10) {
        g_game->mode = 0;
        slot_as<head_rec>(26)->w18 = 7;
        g_windows.set_state(0x1a);
        return;
    }
    if (g_keyboard.down[0x1] != 0 && g_keyboard.prev[0x1] == 0) {
        slot_as<head_rec>(5)->w18 = 7;
        g_windows.set_state(5);
        return;
    }
    if (f48 != 0 && f48->clear_mask(0x10000)) {
        slot_as<head_rec>(22)->w18 = 7;
        slot_as<head_rec>(22)->w04 = 0;
        g_windows.set_state(0x16);
        return;
    }
    if (flag) {
        if (f50->clear_mask(0x10000) || g_game_keys.clicked(0x12)) {
            if (g_module_settings.show_quest_notes != 0) {
                slot_as<head_rec>(26)->w18 = 7;
                g_game->notes_type = 4;
                g_game->notes_object = 0;
                g_windows.set_state(0x1a);
                return;
            }
            slot_as<head_rec>(13)->w18 = 7;
            slot_as<head_rec>(13)->w04 = 0;
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
    if (g->f13ac8 != 0) {
        g_windows.box()->setup(&g->t13ab8, &g->t13aa8);
        g_windows.open(0);
        g_game->f13ac8 = 0;
        return;
    }
    if (g->start_map_conversation != 0) {
        g->start_map_conversation = 0;
        g_mission_ctx.build_hero_table(g_game->map_conversation_troop, -1, -1, g_game->map_conversation_mode);
        slot_as<head_rec>(16)->w18 = 7;
        g_windows.set_state(0x10);
        return;
    }

    {
    int fpid = g->main_party;
    PartyMgr* pm = &g->parties;
    if (pm->get(fpid)->battle >= 0) {
    unsigned idx1 = pm->get(fpid)->battle;
    std::vector<Event*>::iterator pages = g->events.pages.begin();
    if (event_at(pages, idx1)->type == 1) {
        Event* node = event_at(pages, pm->get(fpid)->battle);
        WL_MARKER(12)
        start = 1;
        if (node->party_a == g_game->main_party) {
            g_game->attach_party = node->party_b;
            g_game->attach_scene = -100000;
        } else if (node->party_b == g_game->main_party) {
            g_game->attach_party = node->party_a;
            g_game->attach_scene = -100000;
        } else {
            g_game->attach_party = node->party_b;
            g_game->attach_scene = node->party_a;
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

    g_mission_ctx.init_battle(PARTIES->get(g_game->main_party)->battle);
    g_game->current_menu = 0;
    g_game->jump_entry = 0;
    ((TimerList&)g_game->simple).update(-6);
    g_game->emit_event(1, g_game->attach_party, g_game->attach_scene);
    int id;
    if (g_game->current_menu > 0) {
        g_mission_ctx.frame(g_game->current_menu);
        int n = g_game->current_menu;
        id = PARTIES->get(g_game->main_party)->battle;
        g_map_flags.f8 = n;
    } else {
        g_mission_ctx.frame(-1);
        g_mission_ctx.party_battle_mode = 1;
        id = PARTIES->get(g_game->main_party)->battle;
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
        bool c_f40 = f40->clear_mask(0x10000) || g_game_keys.clicked(0x10);
        if (c_f40) {
            WL_MARKER(4)
            slot_as<head_rec>(8)->w18 = 7;
            slot_as<head_rec>(8)->w04 = 0;
            g_windows.set_state(8);
            WL_MARKER(5)
            return;
        }
        bool c_f3c = f3c->clear_mask(0x10000) || g_game_keys.clicked(0x11);
        if (c_f3c) {
            slot_as<head_rec>(12)->w18 = 7;
            slot_as<head_rec>(12)->w04 = 0;
            g_windows.set_state(0xc);
            return;
        }
        bool c_f44 = f44->clear_mask(0x10000) || g_game_keys.clicked(0xf);
        if (c_f44) {
            slot_as<head_rec>(15)->w18 = 7;
            slot_as<head_rec>(15)->w04 = 0;
            slot_as<head_rec>(15)->w14 = g_game->player;
            g_windows.set_state(0xf);
            return;
        }
        if (g_game_keys.clicked(0x13)) {
            slot_as<head_rec>(25)->w18 = 7;
            slot_as<head_rec>(25)->w04 = 0;
            g_windows.set_state(0x19);
            return;
        }
        if (f4c != 0 && f4c->clear_mask(0x10000)) {
            GameDataV* gv = GV;
            gv->attach_party_center(&gv->parties.get(gv->main_party)->pos.x, gv->main_party, gv->f80);
            return;
        }
    }
    if (f38 != 0 && f38->clear_mask(0x10000)) {
        g_game->current_menu = 4;
        slot_as<head_rec>(23)->w18 = 7;
        g_windows.set_state(0x17);
        return;
    }
    MissionUi* o = f54;
    if (g_game->f14058 == 0.0f)
        o->show();
    else
        o->hide();
    g_gui.flush();
    if (g_game->rest_period > 0.0f && g_game->rest_interactive != 0) {
        f58->show();
        if (g_keyboard.down[0xe0] == 0 && g_keyboard.prev[0xe0] != 0)
            g_game->rest_period = 0.0f;
    } else {
        f58->hide();
    }
    if (!(g_game->rest_period > 0.0f && g_game->rest_interactive == 0))   // skipped only while paused with 13ad8 clear (0x515ad3: je past it)
        camera_move(dt);
    camera_update(dt);
    if (g_game->rest_period <= 0.0f) {
        ((MapPanel*)this)->update_target();
        f2cc += dt;
        f2d0 += dt;
        g_game->f13aa4 = 0;
        {
            GameData* gb = g_game;
            int bpid = gb->main_party;
            PartyMgr* bpm = &gb->parties;
            if (bpm->get(bpid)->action == 4 || bpm->get(bpid)->action == 1
                || bpm->get(bpid)->action == 6 || bpm->get(bpid)->action == 9)
                gb->f13aa4 = 1;
        }
        if (g_keyboard.down[57] != 0) {
            GameData* gk = g_game;
            gk->parties.get(gk->main_party)->action = 0;
            PARTIES->get(g_game->main_party)->reissue_order();
            g_game->f13aa4 = 2;
        }
    }

    GameG* tg = (GameG*)GV;
    tg->tick(dt);
    {
    rgl::matrix m;
    rgl::identity_pos(&m);
    {
        float vx = 0.0175f - g_gui.view_a * 0.1975f;
        float vy = 0.015f - g_gui.view_b * 0.18f;
        rgl::vector4 v(vx, vy, -0.4f);
        m.o = ((pos*)&cam.m)->transform_point(v);
    }
    g_game->compass->set_local_dirty(&m);                 // the compass entity: slot 21 (+0x54), entity::set_local_dirty 0x41fe70
    }

    if (g_cfg.enable_edit_mode != 0 && (g_keyboard.down[0x1d] != 0 || g_keyboard.down[0x9d] != 0) && (char)g_keyboard.is_ready(0x12))
        g_game->editor.active = (g_game->editor.active == 0);

    if (g_game->editor.active != 0) {
        char buf[0x200];
        int bpid = g_game->main_party;
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
        g_game->map_live = 0;
        g_game->sel_party = g_game->main_party;
    }
    b = false; if (g_game_keys.entry[3][0] >= 0 && g_keyboard.down[g_game_keys.entry[3][0]] != 0) b = true;
    if ((g_game_keys.entry[3][1] >= 0 && g_keyboard.down[g_game_keys.entry[3][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[0] * (height + 10.0f) * dt;
        x = d.x + x;
        z = d.y + z;
        g_game->map_live = 0;
        g_game->sel_party = g_game->main_party;
    }
    b = false; if (g_game_keys.entry[0][0] >= 0 && g_keyboard.down[g_game_keys.entry[0][0]] != 0) b = true;
    if ((g_game_keys.entry[0][1] >= 0 && g_keyboard.down[g_game_keys.entry[0][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[1] * (height + 10.0f) * dt;
        x = d.x + x;
        z = d.y + z;
        g_game->map_live = 0;
        g_game->sel_party = g_game->main_party;
    }
    b = false; if (g_game_keys.entry[1][0] >= 0 && g_keyboard.down[g_game_keys.entry[1][0]] != 0) b = true;
    if ((g_game_keys.entry[1][1] >= 0 && g_keyboard.down[g_game_keys.entry[1][1]] != 0) || b) {
        Vec2f d = ((Vec2f*)m.m)[1] * (height + 10.0f) * dt;
        x -= d.x;
        z -= d.y;
        g_game->map_live = 0;
        g_game->sel_party = g_game->main_party;
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
static __forceinline Event& node_of(unsigned id)
{
    std::vector<Event*>& pages = g_game->events.pages;
    return pages[id >> 4][id & 0xf];
}
// the same with the page in a local: the two party_a (+0x54) reads have the index first in the SIB ([eax+ecx+0x54], 0x5162ef, 0x516329)
static __forceinline Event& node_of2(unsigned id)
{
    std::vector<Event*>& pages = g_game->events.pages;
    Event* page = pages[id >> 4];
    return page[id & 0xf];
}
// @ 0x516250
void MissionCtx::init_battle(int id_)
{
    WL_MARKER(12)
    init();           // 0x4be742 (called through its WinLicense entry 0x4be730)
    battle_event = id_;
    dirty = 1;
    side = 0;
    side_chosen = 1;
    mission_flags = 1;
    mode = 3;

    if (g_game->main_party == node_of(id_).party_a) {
        party[0] = node_of(id_).party_b;
        party[1] = -1;
        player_side = 1;
        side_chosen = 1;
    } else if (g_game->main_party == node_of(id_).party_b) {
        party[0] = node_of2(id_).party_a;
        party[1] = -1;
        player_side = 0;
        side_chosen = 1;
    } else if (node_of(id_).type == 1) {
        party[0] = node_of(id_).party_b;
        party[1] = node_of2(id_).party_a;
        mission_flags = 0x11;
        side_chosen = 0;
        player_side = 2;
        mode = 4;
    }
    g_game->f13a18 = 1;
    WL_MARKER(13)
}

// _Vector_iterator<T*>::operator+ left out of line in this function (the original calls the COMDAT, 0x516393; map_editor/exchange_screens.cpp)
struct VecIter {
    int* p;
    __declspec(noinline) int** advance(int** out, int n);   // 0x534160 (ret 8)
};
static __forceinline Event& node_call(unsigned id)
{
    VecIter it;
    it.p = (int*)g_game->events.pages._Myfirst;
    int* slot;
    int** r = it.advance(&slot, id >> 4);
    Event* page = *(Event**)*r;
    return page[id & 0xf];
}
// @ 0x516370
void MissionCtx::frame(int tick)
{
    WL_MARKER(12)
    passage_menu = tick;
    if (player_side == 2 && tick <= 0) {
        party[0] = node_call(battle_event).party_b;
        party[1] = node_call(battle_event).party_a;

        // the record is looked up again for each test (0x516447, 0x5164cf), not read from party[]
        int a = 0;
        int b = 0;
        int ida = node_call(battle_event).party_b;
        GameData* ga = g_game;
        PartyMgr* pa = &ga->parties;
        Party* qa = party_at(pa, pa->lookup(ida));
        if (ga->factions[qa->faction].relations[0] < 0.0f)
            a = 1;
        int idb = node_call(battle_event).party_a;
        GameData* gb = g_game;
        Party* qb = party_index(idb);
        if (gb->factions[qb->faction].relations[0] < 0.0f)
            b = 1;
        if (a != 0 && b == 0) {
            side = 0;
            mission_flags &= ~0x10;
            side_chosen = 1;
        } else if (a == 0 && b != 0) {
            side = 1;
            mission_flags &= ~0x10;
            side_chosen = 1;
        } else {
            side = 0;
            mission_flags |= 0x10;
            side_chosen = 0;
        }
    }
    if (side_chosen != 0) {
        refresh_rating();
        if (passage_menu <= 0)
            key_mask |= 1;
    }
    PartyMgr* pc = &g_game->parties;
    Party* p = party_at(pc, pc->lookup(g_game->main_party));
    if ((char)((PartyV*)p)->data_ready())
        mission_flags |= 2;

    if (g_game->main_party == node_call(battle_event).party_b)
        mission_flags |= 4;
    else if (g_game->main_party == node_call(battle_event).party_a)
        mission_flags |= 8;

    WL_MARKER(13)
    update_anim();
}

// @ 0x5166a0  Party::data_ready
int PartyV::data_ready()
{
    int clk = clk59b0.clock;
    __int64 start = clk59b0.start;
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
    reset_ids(g_game->main_party);
    reset_ids(party[0]);
    reset_ids(party[1]);
    {
        int* ids = &party[0];
        if (ids[side] >= 0)
            GV->fill_ids(g_game->stack_seeds_side[0][0], ids[side]);
        if (ids[1 - side] >= 0)
            GV->fill_ids(g_game->stack_seeds_side[1][0], ids[1 - side]);
    }
    if (party[0] >= 0) {
        GameData* g = g_game;
        PartyMgr* m = &g->parties;
        Party* p = m->at(m->lookup(party[0]));
        int slot = p->stacks[0].troop;
        if (slot >= 0) {
            if ((g->troops[slot].flags30 & 0x10) != 0) {
                Troop* t = &g->troops[slot];
                int f = t->attributes[0];
                float g = (float)(int)((float)(f + hero_rank(t) * 2 + 0x23) * 0.5f + 3.0f);
                if (g > t->hp)          // fcom; test ah,0x41; jne skip (0x516834): a NaN hp skips
                    t->hp = g;
            }
        }
    }
    upgradable = count_hero_items();
    f34 = 0;
    f35 = 0;
    f38 = 0;
    f18 = 0;
    battle_flags = 0;
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
    Troop* t = &troops[hero_troop];
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
