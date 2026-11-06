//-----------------------------------------------------------------------------
// File: game_tick.cpp  (0x516ac0..0x5191e0)
//
// Desc: the campaign map's frame tick of the game object: the clock step and its fan-out (sound, party and settlement
//       updates, the second / weather / minute tickers), the resume countdown, the marker lists, the daily colour blend
//       of sun / fog / sky, the map-record pool cursor, party and settlement visibility, the weather drift, the weekly
//       desertion report, the party-track ageing, the track ribbons and the town training camp.
//       "T regime" region (docs/tu_map.md): the file keeps the binary order; helpers that follow their caller are
//       __declspec(noinline). The game object's fields are reached by offset through the macros below (the slices'
//       member names). Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./game_tick.h"
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"
#include "../../rgl/mesh.h"
#include "../rec5c.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../config/config.h"
#include "../battle_rec.h"      // BattleRec (update_nav_anim 0x4b71e0)
#include "../shared_vars.h"
#include "../key_names.h"
#include "../game_globals.h"
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../party2/party_model.h"   // PartyModel::set_fade 0x4b1800 (Party::model)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
float g_step_tbl[5] = {0.01f, 0.01f, 0.01f, 0.01f, 0.01f};  // 0x5a9c20 per-clock step (seconds); [0] is the frame step


// ---- the game object's fields, by offset (members of GameG's `this`) --------------------------------------------------
#define FLD(T, off) (*(T*)((char*)this + (off)))
#define main_party f78
#define cur_agent f78
#define f94 FLD(char, 0x94)
#define f95 FLD(char, 0x95)
#define pool FLD(GamePool, 0xcc)
#define f13a70 FLD(tstring, 0x13a70)
#define resume_old FLD(tstring, 0x13a70)
#define resume_timer FLD(Timer, 0x13a80)
#define f13aa0 FLD(char, 0x13aa0)
#define f13aa4 FLD(int, 0x13aa4)
#define f13aa8 FLD(tstring, 0x13aa8)
#define f13ab8 FLD(tstring, 0x13ab8)
#define report_text FLD(tstring, 0x13aa8)
#define report_title FLD(tstring, 0x13ab8)
#define f13ac8 FLD(char, 0x13ac8)
#define report_pending FLD(char, 0x13ac8)
#define f13ad0 FLD(float, 0x13ad0)
#define f13adc FLD(int, 0x13adc)
#define f13af8 FLD(int, 0x13af8)
#define f13b18 FLD(int, 0x13b18)
#define f13b20 FLD(int, 0x13b20)
#define f13b24 FLD(int, 0x13b24)
#define f13b20i64 FLD(__int64, 0x13b20)
#define f13b28 FLD(int, 0x13b28)
#define f13b2c FLD(int, 0x13b2c)
#define f13b30 FLD(int, 0x13b30)
#define f13b34 FLD(int, 0x13b34)
#define f13b38 FLD(int, 0x13b38)
#define f13b3c FLD(float, 0x13b3c)
#define f13b40 FLD(float, 0x13b40)
#define f13b60 FLD(int, 0x13b60)
#define speed FLD(float, 0x14058)
#define msg_timer FLD(Timer, 0x1a298)
#define game_ents FLD(ColorEnt**, 0x38)
#define game_ents_end FLD(ColorEnt**, 0x3c)
#define sun_ent FLD(ColorEnt*, 0x44)
#define color_base FLD(rgl::vector4, 0x48)
#define color_fog FLD(rgl::vector4, 0x58)
#define color_sky FLD(rgl::vector4, 0x68)
#define target_obj FLD(ColorTarget*, 0x139dc)
#define tracks FLD(Vec5c, 0x13a60)
#define events FLD(Vec5c, 0x13a60)
#define f2c FLD(rgl::mesh*, 0x2c)
#define f30 FLD(Cfg30*, 0x30)

struct Cfg30 {                                      // the object at game+0x30
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04(); virtual void v05();
    virtual void v06(const float* d);               // +0x18
};

// ---- globals and callees of other translation units ----------------------------------------------------------------
extern const float g_k002;       // 0x590cd0  marker decay per second

#include "../game_globals.h"      // g_module_settings.disable_food_slot, g_module_settings.player_wounded_treshold (canonical)
void set_clock_step(int* clock, float step);            // 0x461420
bool is_daytime();                                          // 0x435700 (true in the 4..20 h window; game/agent/agent.cpp)
// GameW (inv_common.h): remove_party 0x4915d0 (game/world_data, GameData::remove_party)
#include "../res_mgr.h"                         // g_resmgr: get_object 0x419540 (rgl/core/resmgr.h)
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
#define GAMMA (0.45454543828964233f)                    // 0x5a9f1c  1 / 2.2 (the compiler's rounding)

// lerp: dst = dst*s + src*t (the 4th lane rides along uninitialised)
static void blend_color(rgl::vector4& dst, const rgl::vector4& src, float t, float s)
{
    rgl::vector4 r;
    r.x = src.x * t + dst.x * s;
    r.y = src.y * t + dst.y * s;
    r.z = src.z * t + dst.z * s;
    dst = r;
}

// the iterator helper on a bare base pointer
static Rec5c* rec_at(Rec5c* base, int idx)
{
    ItemArray a;
    a.base = base;
    ItemIter r = a.at(idx);
    return r.p;
}

// @ 0x516ac0
void GameG::tick(float dt)
{
    int flags = 1;
    if (f94) {
        speed = 0.0f;
        return;
    }

    if (f13aa0 || f13ad0 > 0.0f) {
        int n = f13adc;
        float v = (n ? (float)n : 10.0f) * g_module_settings.time_multiplier * dt;
        flags = 0x10001;
        speed = v;
        f13ad0 -= v;
    } else if (f13aa4 == 0) {
        speed = 0.0f;
    } else if (f13aa4 == 2) {
        float v = g_module_settings.time_multiplier * dt;
        speed = v;
        if (g_keyboard.down[0x1d] || g_keyboard.down[0x9d])
            speed = v * 10.0f;
    } else {
        Party* a = parties.get(cur_agent);
        Party* b = parties.get(cur_agent);
        float dx = a->pos.x - b->dest_x;
        float dy = a->pos.y - b->dest_y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d == 0.0f)
            speed = 0.0f;
        else
            speed = g_module_settings.time_multiplier * dt;
    }

    static const int map_clock = 1;                 // 0x5937b4 (the map clock's index in g_ticks)
    set_clock_step(const_cast<int*>(&map_clock), speed);
    f13af8 = 0;
    ((TimerList*)((char*)this + 0x139f4))->update(flags);   // 0x48add0
    ((snd_bank*)((char*)this + 0x139e8))->update_all(flags);   // 0x462760

    int target = -1;

    if (g_module_settings.disable_food_slot == 0 &&
        ((Troop*)((char*)troops + player * 0x508))->equip[9].item == -1) {
        float d = (g_ticks[msg_timer.clock] - msg_timer.start) * 1e-5f;
        if (d > 1.0f) {
            msg_timer.set_delay(-2.0f);
            tstring msg("ui_party_has_nothing_to_eat");
            g_gui.add(*g_strtable.lookup(msg), 0xffdd4444);
        }
    }

    party_at(&parties, parties.lookup(cur_agent))->dirty = 1;

    {
        int v = (int)((g_ticks[f13b18] - f13b20i64) * 1e-5f);
        if (v > f13b28) {
            update_weather();
            f13b28 = v;
        }
    }
    {
        int v = (int)((g_ticks[f13b18] - f13b20i64) * 1e-5f * (1.0f / 24));
        if (v > f13b2c) {
            train_troops();
            f13b2c = v;
        }
    }
    {
        int v = (int)((g_ticks[f13b18] - f13b20i64) * 1e-5f * (1.0f / 168));
        if (v > f13b30)
            f13b30 = v;
    }
    {
        int v = (int)((g_ticks[f13b18] - f13b20i64) * 1e-5f * (1.0f / 720));
        if (v > f13b34)
            f13b34 = v;
    }
    {
        int v = (int)((g_ticks[f13b18] - f13b20i64) * 1e-5f * (1.0f / 8640));
        if (v > f13b38)
            f13b38 = v;
    }

    if (speed > 1e-8f) {
        track_parties(speed);
        update_tracks();
    }
    update_cursor(speed);

    int id = parties.first_party_id();
    while ((unsigned)id < parties.n_parties) {
        Party* p = party_at(&parties, parties.lookup(id));
        p->tick(speed);
        if (p->action == 8 && (*(__int64*)&p->flags_lo & 0x20000))
            target = id;
        parties.next_party_id(&id);
    }
    if (target >= 0)
        ((GameW*)this)->remove_party(target);   // 0x4915d0 (game/world_data)

    {
        GamePool* gp = &pool;
        int idx = gp->first();
        while ((unsigned)idx < (unsigned)gp->count) {
            ((BattleTable*)gp)->get(idx)->update_nav_anim(*(int*)&speed);   // 0x4b71e0 (battle_rec.h; the argument is unused by the callee)
            gp->next(&idx);
        }
    }

    ((rgl::world*)g_script.level)->step(g_step_tbl[0]);

    if (g_cfg.cheat_mode && g_keyboard.down[0x14] && !g_keyboard.prev[0x14] && g_keyboard.down[0x1d])
        f95 = !f95;

    update_colors();
    update_markers();

    if (f13aa0) {
        float d = (g_ticks[resume_timer.clock] - resume_timer.start) * 1e-5f;
        if (d >= 0.0f) {
            f13aa0 = 0;
            Party* p = party_at(&parties, parties.lookup(cur_agent));
            *(__int64*)&p->flags_lo &= ~(__int64)0x100;
            tstring s("");
            resume_strings(&s, &resume_old);
        }
    }
}

// @ 0x517110
void GameG::resume_strings(tstring* a, tstring* b)
{
    f13ab8 = *a;
    f13aa8 = *b;
    f13ac8 = 1;
}

// @ 0x517140
void GameG::update_markers()
{
    for (unsigned i = 0; i < (unsigned)g_script.mat_type0.count(); i++) {
        g_script.mat_type0.at<Marker>(i)->f1f8 -= g_step_tbl[0] * 0.02f;
        g_script.mat_type0.at<Marker>(i)->f208 = g_script.mat_type0.at<Marker>(i)->f1f8;
    }
    for (unsigned i = 0; i < (unsigned)g_script.mat_sea.count(); i++) {
        g_script.mat_sea.at<Marker>(i)->f1f4 += g_step_tbl[0] * g_module_settings.map_sea_speed_x * 3.0f;
        g_script.mat_sea.at<Marker>(i)->f1f8 += g_step_tbl[0] * g_module_settings.map_sea_speed_y * 3.0f;
        g_script.mat_sea.at<Marker>(i)->f204 = g_script.mat_sea.at<Marker>(i)->f1f4;
        g_script.mat_sea.at<Marker>(i)->f208 = g_script.mat_sea.at<Marker>(i)->f1f8;
    }
    for (unsigned i = 0; i < (unsigned)g_script.mat_sea2.count(); i++) {
        g_script.mat_sea2.at<Marker>(i)->f1f4 -= g_step_tbl[0] * g_module_settings.map_sea_speed_x * 2.0f;
        g_script.mat_sea2.at<Marker>(i)->f1f8 -= g_step_tbl[0] * g_module_settings.map_sea_speed_y * 2.0f;
        g_script.mat_sea2.at<Marker>(i)->f204 = g_script.mat_sea2.at<Marker>(i)->f1f4;
        g_script.mat_sea2.at<Marker>(i)->f208 = g_script.mat_sea2.at<Marker>(i)->f1f8;
    }
    for (unsigned i = 0; i < (unsigned)g_script.mat_river.count(); i++) {
        g_script.mat_river.at<Marker>(i)->f1f4 += g_step_tbl[0] * g_module_settings.map_river_speed_x;
        g_script.mat_river.at<Marker>(i)->f1f8 += g_step_tbl[0] * g_module_settings.map_river_speed_y;
        g_script.mat_river.at<Marker>(i)->f204 = g_script.mat_river.at<Marker>(i)->f1f4;
        g_script.mat_river.at<Marker>(i)->f208 = g_script.mat_river.at<Marker>(i)->f1f8;
    }
}

// @ 0x517350
void GameG::update_colors()
{
    float t = g_step_tbl[0] * 0.9f;
    float s = 1.0f - t;

    rgl::vector4 c;
    sun_color(&c);
    blend_color(color_base, c, t, s);

    fog_color(&c);
    blend_color(color_fog, c, t, s);

    sky_color(&c);
    blend_color(color_sky, c, t, s);

    rgl::vector4 sun;
    sun.x = color_base.x + color_sky.x;
    sun.y = color_base.y + color_sky.y;
    sun.z = color_base.z + color_sky.z;
    sun.w = 1.0f;

    if (g_use_shaders == 0) {
        sun.x = (float)pow(sun.x, GAMMA);
        sun.y = (float)pow(sun.y, GAMMA);
        sun.z = (float)pow(sun.z, GAMMA);
        unsigned col = rgl::pack_color(&sun);
        set_grid_color(&col);
        sun_ent->color = col;
    } else {
        unsigned col = rgl::pack_color(&sun);
        ColorEnt** e = game_ents;
        while (e < game_ents_end) {
            (*e)->color = col;
            e++;
        }
        sun_ent->color = col;
    }

    if (g_use_shaders == 0) {
        target_obj->v = rgl::vector4((float)pow(color_base.x, GAMMA),
                            (float)pow(color_base.y, GAMMA),
                            (float)pow(color_base.z, GAMMA));
        target_obj->v.w = 1.0f;                      // the original copies the sun's w lane here
        ((rgl::world*)g_script.level)->ambient = rgl::vector4((float)pow(color_sky.x, GAMMA),
                                 (float)pow(color_sky.y, GAMMA),
                                 (float)pow(color_sky.z, GAMMA));
    } else {
        target_obj->v = color_base;
        ((rgl::world*)g_script.level)->ambient = color_sky;
    }
    ((rgl::world*)g_script.level)->ambient.w = 1.0f;                         // +0x74 is the scene alpha
}

// @ 0x5176e0
rgl::vector4* GameG::sun_color(rgl::vector4* out)
{
    rgl::vector4 c;
    if (!is_daytime()) {
        c = rgl::vector4(0.095f, 0.116f, 0.24f);
    } else {
        float d = 3.3f - (f13b3c + f13b3c) - f13b40;
        c = rgl::vector4(d * 0.33900001645088196f, d * 0.3f, d * 0.1980000138282776f);
    }
    *out = c;
    return out;
}

// @ 0x517770
rgl::vector4* GameG::sky_color(rgl::vector4* out)
{
    rgl::vector4 c;
    if (!is_daytime()) {
        c = rgl::vector4(0.035f, 0.04f, 0.14f);
    } else {
        float f = f13b40 + f13b3c;
        // the tint is one expression repeated per component: cl commons it but keeps y and z in the temporary's slots
        rgl::vector4 v((f + f + 1.0f) * 0.08f, (f + f + 1.0f) * 0.08f, (f + f + 1.0f) * 0.08f);
        c = v + rgl::vector4(0.24f, 0.28f, 0.44f);
    }
    *out = c;
    return out;
}

// @ 0x517820
rgl::vector4* GameG::fog_color(rgl::vector4* out)
{
    rgl::vector4 c(0.65f, 0.68f, 0.71f);
    if (!is_daytime())
        c = rgl::vector4(0.11f, 0.18f, 0.25f);
    if (g_use_shaders == 0) {
        c.x = (float)sqrt(c.x);
        c.y = (float)sqrt(c.y);
        c.z = (float)sqrt(c.z);
    }
    *out = c;
    return out;
}

// @ 0x5178d0
void GameG::set_grid_color(unsigned* c)
{
    int i = 0;
    if (g_script.page_rows > 0) {
        do {
            int j = 0;
            if (g_script.page_cols > 0) {
                do {
                    GridModel** pp = (GridModel**)&g_script.map_cell[j * 8 + i];
                    for (ColorEnt** e = (*pp)->ents; e < (*pp)->ents_end; e++)
                        (*e)->color = *c;
                    j++;
                } while (j < g_script.page_cols);
            }
            i++;
        } while (i < g_script.page_rows);
    }
}

struct Rec98 { int f0; int alive; char pad[0x90]; };
static inline Rec98& pool_at(GamePool* p, int i) { return (*(std::vector<Rec98*>*)((char*)p + 0x14))[(unsigned)i >> 4][i & 0xf]; }
// @ 0x517960
void GamePool::next(int* index)
{
    ++*index;
    while (*index < count && !pool_at(this, *index).alive)
        ++*index;
}

// @ 0x5179b0
int GamePool::first()
{
    int i = 0;
    if (count > 0) {
        do {
            char* rec = pages[(unsigned)i >> 4] + (i & 0xf) * 0x98;
            if (*(int*)(rec + 4) != 0)
                break;
            i++;
        } while (i < count);
    }
    return i;
}

// @ 0x5179f0
void GameG::update_cursor(float dt)
{
    PartyMgr* mgr = &parties;

    party_at(mgr, mgr->lookup(main_party))->f6c = 1;
    float radius = party_at(mgr, mgr->lookup(main_party))->map_speed();

    unsigned id = (unsigned)mgr->first_party_id();
    while (id < mgr->n_parties) {
        unsigned cur = id;
        Party* p = party_at(mgr, mgr->lookup((int)id));
        if (cur != (unsigned)main_party && p->f8c8 < 0) {
            unsigned __int64 flags = *(unsigned __int64*)&p->flags_lo;
            if ((flags & 0x1000100) == 0 && p->model != 0) {
                Party* mp = party_at(mgr, mgr->lookup(main_party));
                float dx = p->pos.x - mp->pos.x;
                float dy = p->pos.y - mp->pos.y;
                float dist = (float)sqrt(dx * dx + dy * dy);
                if ((flags & 0x4000) == 0) {
                    if (dist <= radius && dist <= p->strength() * radius) {
                        g_script.trigger_result.store1 = 0;
                        g_script.trigger_result.store0 = 1;
                        emit_event(0x19, main_party, (int)cur);
                        if ((g_script.trigger_result.store0 | g_script.trigger_result.store1) != 0) {
                            float f = p->alpha + dt * 30.0f;
                            p->alpha = f;
                            if (f > 1.0f || f < -100.0f)
                                p->alpha = 1.0f;
                            if (p->f6c == 0) {
                                p->f6c = 1;
                                emit_event(0xc, (int)cur, 0);
                            }
                        } else {
                            float f = p->alpha - dt * 5.0f;
                            p->alpha = f;
                            if (f < 0.0f || f < -100.0f)
                                p->alpha = 0.0f;
                            if (p->f6c != 0) {
                                p->f6c = 0;
                                emit_event(0xd, (int)cur, 0);
                            }
                        }
                    } else {
                        float f = p->alpha - dt * 5.0f;
                        p->alpha = f;
                        if (f < 0.0f || f < -100.0f)
                            p->alpha = 0.0f;
                        if (p->f6c != 0) {
                            p->f6c = 0;
                            emit_event(0xd, (int)cur, 0);
                        }
                    }
                } else {
                    p->alpha = 1.0f;
                    if (dist <= radius * 0.5f && p->f6c == 0) {
                        p->f6c = 1;
                        p->dirty = 1;
                        emit_event(0xc, (int)cur, 0);
                    } else if (radius + radius < dist && p->f6c != 0) {
                        p->f6c = 0;
                        p->dirty = 1;
                        emit_event(0xd, (int)cur, 0);
                    }
                }
                if (f95 == 0) {
                    float v = (p->alpha < 0.01f) ? 0.0f : 1.0f;
                    p->model->set_fade(&v);
                } else {
                    p->alpha = 1.0f;
                    p->f6c = 1;
                }
            }
        }
        mgr->next_party_id((int*)&id);
    }

    int i = pool.first();
    while ((unsigned)i < (unsigned)pool.count) {
        MapRec* r = (MapRec*)(pool.pages[i >> 4] + (i & 0xf) * 0x98);
        if (f95 != 0) {
            r->opacity = 1.0f;
        } else {
            Party* mp = party_at(mgr, mgr->lookup(main_party));
            float dx = r->x - mp->pos.x;
            float dy = r->y - mp->pos.y;
            float dist = (float)sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                float f = dt * 30.0f + r->opacity;
                r->opacity = f;
                if (f > 1.0f)
                    r->opacity = 1.0f;
                else if (f < -100.0f)
                    r->opacity = 1.0f;
            } else {
                float f = r->opacity - dt * 5.0f;
                r->opacity = f;
                if (f < 0.0f || f < -100.0f)
                    r->opacity = 0.0f;
            }
        }
        pool.next(&i);
    }
}

// @ 0x517e70
void GameG::update_weather()
{
    PartyMgr* mgr = &parties;

    unsigned id = (unsigned)mgr->first_party_id();
    while (id < mgr->n_parties) {
        if (id == (unsigned)main_party
            || (*(unsigned __int64*)&party_at(mgr, mgr->lookup((int)id))->flags_lo & 0x80100) == 0)
            party_at(mgr, mgr->lookup((int)id))->tick_stats();
        mgr->next_party_id((int*)&id);
    }

    __int64 cur = g_ticks[f13b18];
    __int64 ref;
    ((int*)&ref)[0] = f13b20;
    ((int*)&ref)[1] = f13b24;
    int hour = (int)((float)(cur - ref) * 1e-5f) % 24;

    float v = ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.5f) * 0.1f + f13b3c;
    f13b3c = v;
    if (hour > 0 && hour < 8 && f13b40 < 0.9f) {
        float k = 1.0f;
        if (v > 0.4f)
            k = 1.5f;
        f13b40 += ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.1f) * k * 0.05f;
    } else if (hour > 10 && hour < 18 && f13b40 > 0.1f) {
        float k = 1.0f;
        if (v < 0.6f)
            k = 1.5f;
        f13b40 -= ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.1f) * k * 0.05f;
    }

    float u = (float)(rand() % 0x3dc9) * 6.3223117e-5f;
    f13b40 += ((f13b3c - 0.55f) * u) * 0.01f;

    float f = ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.5f) * 0.1f + f13b40;
    f13b40 = f;
    if (f < 0.0f)
        f13b40 = 0.0f;
    else if (f > 1.0f)
        f13b40 = 1.0f;

    if (f13b3c < 0.0f)
        f13b3c = 0.0f;
    else if (f13b3c > 1.0f)
        f13b3c = 1.0f;

    if (hour >= 18) {
        if ((float)(rand() % 0x3dc9) * 6.3223117e-5f < 0.25f && f13b2c > f13b60)
            goto fire;
    }
    if (hour >= 22 && f13b2c > f13b60) {
fire:
        weekly_report((f13b2c & 7) == 0);
        f13b60 = f13b2c;
    }
}

// @ 0x5181c0
void GameG::weekly_report(bool)
{
    Party* p = player_party();
    float factor1 = -0.01f;
    float factor2;
    Troop* troop = &troops[player];
    tstring body;

    int total = 0;
    for (int i = 0; i < p->nstacks; i++) {
        if (p->stacks[i].flags & 1)
            break;
        total += p->stacks[i].count;
    }
    int limit = p->max_members();
    if (total > limit) {
        float f = (1.0f - (float)limit / (float)total) * 0.5f;
        if (f > -0.01f)
            factor1 = f;
        tstring key("ui_number_of_companions_exceeds_leadership_limit");
        body.append(*g_strtable.lookup(key));
    }

    total = 0;
    for (int i = 0; i < p->nstacks; i++)
        if (p->stacks[i].flags & 1)
            total += p->stacks[i].count;
    limit = p->max_prisoners();
    factor2 = -0.01f;
    if (total > limit) {
        factor2 = (1.0f - (float)limit / (float)total) * 0.5f;
        tstring key("ui_number_of_prisoners_exceeds_prisoner_management_limit");
        body.append(*g_strtable.lookup(key));
    }

    if (p->f7c < 0.2f) {
        tstring key("ui_party_morale_is_low");
        body.append(*g_strtable.lookup(key));
        float f = 0.3f - p->f7c;
        if (f < factor1)
            f = factor1;
        factor1 = f * 0.2f;
    }

    if (troop->f3b4 < 0)
        troop->f3b4 = 0;

    int counts[32];
    int ids[32];
    int nsel = 0;
    int i = p->nstacks - 1;
    Stack* s = &p->stacks[i];
    for (; i > 0; i--, s--) {
        int hits = 0;
        float thr = factor1;
        if (s->flags & 1)
            thr = factor2;
        if ((troops[s->troop].flags32 & 0x10) == 0) {
            counts[nsel] = 0;
            for (int k = 0; k < s->count; k++) {
                if ((float)(rand() % 0x3dc9) * 6.3223117e-5f < thr) {
                    counts[nsel]++;
                    hits++;
                }
            }
            if (counts[nsel] != 0) {
                ids[nsel] = s->troop;
                nsel++;
            }
            if (hits != 0)
                p->consume_stack(i, hits, (int*)((char*)this + 0x1405c), 0);
        }
    }

    if (nsel != 0) {
        int before_last = nsel - 2;
        for (int j = 0; j < nsel; j++) {
            body.push_back(' ');
            Troop* tr = &troops[ids[j]];
            if ((tr->flags32 & 0x10) != 0) {
                tstring* name = g_strtable.find(&tr->key);
                if (name == 0)
                    name = (tstring*)tr;
                body.append(*name);
            } else {
                tstring* name = g_strtable.find(&tr->key);
                if (name == 0)
                    name = (tstring*)tr;
                char buf[4092];
                sprintf(buf, "%d %s ", counts[j], name->c_str());
                tstring part(buf);
                body.append(part);
            }
            if (j < before_last)
                body.push_back(',');
            else if (j == before_last) {
                tstring key("ui_and_one_space");
                body.append(*g_strtable.lookup(key));
            }
        }
        {
            tstring key(nsel == 1 && counts[0] == 1 ? "ui_has_deserted_the_party"
                                                    : "ui_have_deserted_the_party");
            body.append(*g_strtable.lookup(key));
        }
        {
            tstring key("ui_weekly_report");
            report_title = *g_strtable.lookup(key);
            report_text = body;
            report_pending = 1;
        }
    }
    // body's destructor
}

// @ 0x518680
void GameG::track_parties(float dt)
{
    PartyV4 pos = player_party()->pos;
    int skill = player_party()->skill_level(0x10);
    float radius = ((float)skill * 0.125f + 1.0f) * g_module_settings.track_spotting;
    if (!is_daytime())
        radius *= 0.3f;
    float threshold = radius * radius;
    if (player_party()->skill_level(0x10) == 0)
        threshold = -0.1f;

    const int SEEN = 0x20000000;
    Rec5c* rec = tracks._Myfirst;
    while (rec < tracks._Mylast) {
        rec->f14 += dt;
        if ((rec->flags & SEEN) == 0) {
            float dx = rec->x - pos.x;
            float dy = rec->y - pos.y;
            float dz = rec->z - pos.z;
            if (dx * dx + dz * dz + dy * dy < threshold)
                rec->flags |= SEEN;
        }
        rec++;
    }

    float sf = skill_plus_8();
    int last = tracks.size() - 1;
    int i = 0;
    while (i <= last) {
        Rec5c* cur = rec_at(tracks._Myfirst, i);
        unsigned char b = cur->f1a;
        float t = (float)((b & 0xf) + 0x10) * 0.03125f;
        t *= sf;
        if (rec_at(tracks._Myfirst, i)->f14 > t) {
            *rec_at(tracks._Myfirst, i) = *rec_at(tracks._Myfirst, last);
            last--;
        } else {
            i++;
        }
    }
    if (last < tracks.size() - 1) {
        Rec5c* dummy;
        tracks.shift_over(&dummy, tracks._Myfirst + (last + 1), tracks._Mylast);
    }
}

// @ 0x518940
float GameG::skill_plus_8()
{
    int s = player_party()->skill_level(0x10);
    return (float)s + 8.0f;
}

// @ 0x518990
Rec5c** Vec5c::shift_over(Rec5c** out, Rec5c* dst, Rec5c* src)
{
    if (dst != src) {
        Rec5c* end = _Mylast;
        Rec5c* p = dst;
        while (src != end) {
            *p = *src;
            ++p;
            ++src;
        }
        _Mylast = p;
    }
    *out = dst;
    return out;
}

// @ 0x5189e0
void GameG::update_tracks()
{
    f2c->clear_geometry();

    rgl::mesh* track;
    {
        tstring n("track");
        track = g_resmgr.get_object(&n);
    }
    rgl::mesh* battle;
    {
        tstring n("battle_track");
        battle = g_resmgr.get_object(&n);
    }

    float denom = (float)party_at(&parties, parties.lookup(f78))->skill_level(0x10) + 8.0f;

    for (Rec5c* p = events._Myfirst; p < events._Mylast; ++p) {
        if (!f95 && !(p->flags & 0x20000000))
            continue;

        int first = (int)f2c->v50.size();
        rgl::mesh* m = track;
        if (p->flags & 0x10000000)
            m = battle;
        f2c->append_skinned(m, (rgl::matrix*)((char*)p + 0x1c), -1, -1, 0);

        float t = p->f14 / denom;
        t = (t < 1.0f) ? t : 1.0f;
        t *= 3.0f;

        rgl::vector4 c;
        if (t < 1.0f) {
            float u = 1.0f - t;
            // 0x518b53: (0.9t + 1.2u, 0.8t + 0u, 0t + 0u)
            c = rgl::vector4(0.9f * t + 1.2f * u,
                             0.8f * t + 0.0f * u,
                             0.0f * t + 0.0f * u);
        } else {
            float u = t - 1.0f;
            if (u > 1.0f)
                u = 1.0f;
            float v = 1.0f - u;
            // 0x518bee: (0u + 0.9v, 0.6u + 0.8v, 0.8u + 0v)
            c = rgl::vector4(0.0f * u + 0.9f * v,
                             0.6f * u + 0.8f * v,
                             0.8f * u + 0.0f * v);
        }

        unsigned col = rgl::pack_color(&c);
        if (party_at(&parties, parties.lookup(f78))->skill_level(0x10) < 2)
            col = 0xffa8a8a8;
        col |= 0xff000000;

        for (int i = first; i < f2c->v50.size(); i++)
            f2c->v50[i].color = col;
    }

    float d = 1000000.0f;
    f30->v06(&d);
}

// @ 0x518d70
void GameG::train_troops()
{
    for (unsigned id = parties.first_party_id(); id < parties.n_parties; parties.next_party_id((int*)&id))
        party_at(&parties, parties.lookup(id))->dirty = 1;

    Party* p = party_at(&parties, parties.lookup(f78));

    int gain[64];
    for (int i = 0; i < p->nstacks; i++)
        gain[i] = 0;

    int limit[11] = {0, 4, 10, 0x10, 0x17, 0x1e, 0x26, 0x2e, 0x37, 0x41, 0x50};
    int total = 0;

    for (int i = 0; i < p->nstacks; i++) {
        Stack& s = p->stacks[i];
        if (s.flags & 1)
            continue;
        if (!(troops[s.troop].flags30 & 0x10))
            continue;
        if (s.troop == player ? troops[s.troop].hp < g_module_settings.player_wounded_treshold : troops[s.troop].hp < g_module_settings.hero_wounded_treshold)
            continue;
        int lvl = skill_level(s.troop, 0x11);
        if (lvl > 10)
            lvl = 10;
        int cost = limit[lvl];
        if (!cost)
            continue;

        int n = p->nstacks;
        int j = 0;
        if (n > 0) do {
            Stack& q = p->stacks[j];
            if (j == i)
                continue;
            if (q.flags & 1)
                continue;
            if (troops[q.troop].level >= troops[s.troop].level)
                continue;
            if (troops[q.troop].flags30 & 0x10) {
                if (q.troop == player ? troops[q.troop].hp < g_module_settings.player_wounded_treshold : troops[q.troop].hp < g_module_settings.hero_wounded_treshold)
                    continue;
                gain[j] += cost;
                total += cost;
            } else {
                if (!troops[q.troop].f50)
                    continue;
                int ready = q.count - q.wounded;
                gain[j] += cost * (ready > 0 ? ready : 0);
                total += cost * (ready > 0 ? ready : 0);
            }
        } while (++j < n);
    }

    if (total != 0) {
        g_gui.add(*g_strtable.lookup(tstring("ui_days_training_is_complete")), -1);
        char buf[512];
        sprintf(buf, g_strtable.lookup(tstring("ui_total_experience_gained_through_training_number"))->c_str(), total);
        g_gui.add(tstring(buf), -1);
    }

    bool changed = false;
    for (int i = 0; i < p->nstacks; i++) {
        Stack& q = p->stacks[i];
        if (troops[q.troop].flags30 & 0x10) {
            add_experience(q.troop, gain[i]);
        } else {
            int old = q.f14;
            p->add_xp_to_stack(i, gain[i]);
            if (q.f14 > old)
                changed = true;
        }
    }
    if (changed)
        g_gui.add(*g_strtable.lookup(tstring("ui_some_soldiers_are_ready_to_upgrade")), -1);
}
