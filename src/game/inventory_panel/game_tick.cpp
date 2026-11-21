//-----------------------------------------------------------------------------
// File: game_tick.cpp  (0x516ac0..0x518680)
//
// Desc: the campaign map's frame tick of the game object: the clock step and its fan-out (sound, party and settlement
//       updates, the second / weather / minute tickers), the resume countdown, the marker lists, the daily colour blend
//       of sun / fog / sky, the map-record pool cursor, party and settlement visibility, the weather drift, the weekly
//       desertion report (party_tracks.cpp continues at 0x518680: track ageing, ribbons, training camp).
//       "T regime" region (docs/tu_map.md): the file keeps the binary order; helpers that follow their caller are
//       __declspec(noinline). The game object's fields are reached by offset through the macros below (the slices'
//       member names). Names provisional. Flags: /O2 /MT /Gy /TP.
//-----------------------------------------------------------------------------
#include "./game_tick.h"
#include "./inv_common.h"      // (listed directly: the resolution audit reads only the source's own includes)
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../str_table.h"
#include "../../include/game_data.h"
#include "../trigger_vecs.h"            // TriggerVec (snd_bank): update_all 0x462760
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../strat/gui_manager.h"
#include "../../rgl/mesh.h"
#include "../track_rec.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../config/config.h"
#include "../event.h"           // Event, the record of the pool at +0xcc
#include "../shared_vars.h"
#include "../key_names.h"
#include "../game_globals.h"
#include "../../include/render_flags.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../party2/party_model.h"   // strategic_entity::set_fade 0x4b1800 (Party::model)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
float g_step_tbl[5] = {0.01f, 0.01f, 0.01f, 0.01f, 0.01f};  // 0x5a9c20 per-clock step (seconds); [0] is the frame step
float g_gamma = 0.45454544f;                     // 0x5a9f1c 1 / 2.2: the colour ramp's gamma exponent (update_colors)



// ---- globals and callees of other translation units ----------------------------------------------------------------
extern const float g_k002;       // 0x590cd0  marker decay per second

#include "../game_globals.h"      // g_module_settings.disable_food_slot, g_module_settings.player_wounded_treshold (canonical)
void set_clock_step(int* clock, float step);            // 0x461420
bool is_daytime();                                          // 0x435700 (true in the 4..20 h window; game/agent/agent.cpp)
// GameData::remove_party 0x4915d0 (game/world_data)
#include "../res_mgr.h"                         // g_resmgr: get_object 0x419540 (rgl/core/resmgr.h)
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f
#define GAMMA g_gamma                                   // 0x5a9f1c (a float variable: the original loads it with fld dword)

// party page lookup, direct form: page slot loads the record base
static Party* raw_party(PartyMgr* m, int idx)
{
    return (Party*)m->pages._Myfirst[idx >> 4] + (idx & 0xf);
}

// the iterator helper on a bare base pointer
static TrackRec* rec_at(TrackRec* base, int idx)
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
        f14058 = 0.0f;
        return;
    }

    if (f13aa0 || rest_period > 0.0f) {
        float v = (rest_time_speed == 0 ? 10.0f : (float)rest_time_speed) * g_module_settings.time_multiplier * dt;
        flags = 0x10001;
        f14058 = v;
        rest_period -= v;
    } else if (f13aa4 == 0) {
        f14058 = 0.0f;
    } else if (f13aa4 == 2) {
        float v = g_module_settings.time_multiplier * dt;
        f14058 = v;
        if (g_keyboard.down[0x1d] || g_keyboard.down[0x9d])
            f14058 = v * 10.0f;
    } else {
        int mp = main_party;
        Party* a = parties.get(mp);
        const PartyV4& pa = a->pos;
        Party* b = parties.get(mp);
        Vec2& dst = b->dest;
        float dx = pa.x - dst.x;
        float dy = pa.y - dst.y;
        float d = sqrtf(dx * dx + dy * dy);
        if (d == 0.0f)
            f14058 = 0.0f;
        else
            f14058 = g_module_settings.time_multiplier * dt;
    }

    static const int map_clock = 1;                 // 0x5937b4 (the map clock's index in g_ticks)
    set_clock_step(const_cast<int*>(&map_clock), f14058);
    jump_entry = 0;
    simple.update(flags);   // 0x48add0 (the simple triggers)
    ((snd_bank&)triggers).update_all(flags);   // 0x462760 (the module triggers)

    if (g_module_settings.disable_food_slot == 0 &&
        troops[player].equip[9].item == -1) {
        float d = (g_ticks[timer1a298.clock] - timer1a298.start) * 1e-5f;
        if (d > 1.0f) {
            timer1a298.set_delay(-2.0f);
            tstring msg("ui_party_has_nothing_to_eat");
            g_gui.add(*g_strtable.lookup(msg), 0xffdd4444);
        }
    }

    party_at(&parties, parties.lookup(main_party))->dirty = 1;

    {
        int v = (int)((g_ticks[f13b18] - clock) * 1e-5f);
        if (v > current_hours) {
            update_weather();
            current_hours = v;
        }
    }
    {
        int v = (int)((g_ticks[f13b18] - clock) * 1e-5f * (1.0f / 24));
        if (v > string_count) {
            train_troops();
            string_count = v;
        }
    }
    {
        int v = (int)((g_ticks[f13b18] - clock) * 1e-5f * (1.0f / 168));
        if (v > f13b30)
            f13b30 = v;
    }
    {
        int v = (int)((g_ticks[f13b18] - clock) * 1e-5f * (1.0f / 720));
        if (v > f13b34)
            f13b34 = v;
    }
    {
        int v = (int)((g_ticks[f13b18] - clock) * 1e-5f * (1.0f / 8640));
        if (v > f13b38)
            f13b38 = v;
    }

    if (f14058 > 1e-8f) {
        track_parties(f14058);
        update_tracks();
    }
    update_cursor(f14058);

    int target = -1;
    int id = parties.first_party_id();
    while ((unsigned)id < parties.serial) {
        Party* p = party_at(&parties, parties.lookup(id));
        p->tick(f14058);
        if (p->action == 8 && (p->flags & 0x20000))
            target = id;
        parties.next_party_id(&id);
    }
    if (target >= 0)
        remove_party(target);   // 0x4915d0 (game/world_data)

    {
        EventPool* gp = &event_pool(events);
        int idx = gp->first();
        while ((unsigned)idx < (unsigned)gp->count) {
            gp->get(idx)->update_nav_anim(*(int*)&f14058);   // 0x4b71e0 (party2/party_2.cpp; the argument is unused by the callee)
            gp->next(&idx);
        }
    }

    ((rgl::world*)g_script.level)->step(g_step_tbl[0]);

    if (g_cfg.cheat_mode && g_keyboard.down[0x14] && !g_keyboard.prev[0x14] && g_keyboard.down[0x1d])
        reveal_all = !reveal_all;

    update_colors();
    update_markers();

    if (f13aa0) {
        float d = (g_ticks[timer13a80.clock] - timer13a80.start) * 1e-5f;
        if (d >= 0.0f) {
            f13aa0 = 0;
            Party* p = party_at(&parties, parties.lookup(main_party));
            p->flags &= ~(unsigned __int64)0x100;
            tstring s("");
            resume_strings(&s, &t13a70);
        }
    }
}

// @ 0x517110
void GameG::resume_strings(tstring* a, tstring* b)
{
    t13ab8 = *a;
    t13aa8 = *b;
    f13ac8 = 1;
}

// The map scene's material lists are std::vector<rgl::mesh*>; the drift goes into each mesh's matrix tex_matrix (u.x / u.y at
// +0x1f4, o.x / o.y at +0x204). size() and operator[] through the real vector: the counter in ecx, the list base in
// edx/eax and the element pointer copied before the o.y store, as the original.

// the frame step through an inline getter at every use: spelled as a plain g_step_tbl[0] operand, cl loads the f14058
// first and fmuls the step (either operand order); the original loads 0x5a9c20 first and fmuls the f14058 in all six
static inline float frame_dt() { return g_step_tbl[0]; }

// @ 0x517140
void GameG::update_markers()
{
    for (unsigned i = 0; i < g_script.mat_type0.size(); i++) {
        g_script.mat_type0[i]->tex_matrix.u.y -= frame_dt() * 0.02f;
        g_script.mat_type0[i]->tex_matrix.o.y = g_script.mat_type0[i]->tex_matrix.u.y;
    }
    for (unsigned i = 0; i < g_script.mat_sea.size(); i++) {
        g_script.mat_sea[i]->tex_matrix.u.x += frame_dt() * g_module_settings.map_sea_speed_x * 3.0f;
        g_script.mat_sea[i]->tex_matrix.u.y += frame_dt() * g_module_settings.map_sea_speed_y * 3.0f;
        g_script.mat_sea[i]->tex_matrix.o.x = g_script.mat_sea[i]->tex_matrix.u.x;
        g_script.mat_sea[i]->tex_matrix.o.y = g_script.mat_sea[i]->tex_matrix.u.y;
    }
    for (unsigned i = 0; i < g_script.mat_sea2.size(); i++) {
        g_script.mat_sea2[i]->tex_matrix.u.x -= frame_dt() * g_module_settings.map_sea_speed_x * 2.0f;
        g_script.mat_sea2[i]->tex_matrix.u.y -= frame_dt() * g_module_settings.map_sea_speed_y * 2.0f;
        g_script.mat_sea2[i]->tex_matrix.o.x = g_script.mat_sea2[i]->tex_matrix.u.x;
        g_script.mat_sea2[i]->tex_matrix.o.y = g_script.mat_sea2[i]->tex_matrix.u.y;
    }
    for (unsigned i = 0; i < g_script.mat_river.size(); i++) {
        g_script.mat_river[i]->tex_matrix.u.x += frame_dt() * g_module_settings.map_river_speed_x;
        g_script.mat_river[i]->tex_matrix.u.y += frame_dt() * g_module_settings.map_river_speed_y;
        g_script.mat_river[i]->tex_matrix.o.x = g_script.mat_river[i]->tex_matrix.u.x;
        g_script.mat_river[i]->tex_matrix.o.y = g_script.mat_river[i]->tex_matrix.u.y;
    }
}

// @ 0x517350
void GameG::update_colors()
{
    float t = g_step_tbl[0] * 0.9f;
    float s = 1.0f - t;

    // each blend is the vector4 operators (const float& factors: t and s live in memory), the right operand first:
    // the getter's result * t, then dst * s, then the sum's temporary copied whole (its w lane unset)
    rgl::vector4 c;
    fog1 = fog1 * s + *sun_color(&c) * t;
    fog2 = fog2 * s + *fog_color(&c) * t;
    fog3 = fog3 * s + *sky_color(&c) * t;

    rgl::vector4 sun = fog1 + fog3;
    sun.w = 1.0f;

    if (g_use_shaders == 0) {
        sun.x = (float)pow(sun.x, GAMMA);
        sun.y = (float)pow(sun.y, GAMMA);
        sun.z = (float)pow(sun.z, GAMMA);
        unsigned col = rgl::pack_color(&sun);
        set_grid_color(&col);
        map_obj->color = col;
    } else {
        unsigned col = rgl::pack_color(&sun);
        rgl::mesh** e = ents._Myfirst;
        while (e < ents._Mylast) {
            (*e)->color = col;
            e++;
        }
        map_obj->color = col;
    }

    if (g_use_shaders == 0) {
        rgl::vector4 amb((float)pow(fog3.x, GAMMA), (float)pow(fog3.y, GAMMA), (float)pow(fog3.z, GAMMA));
        wind->color = rgl::vector4((float)pow(fog1.x, GAMMA),
                            (float)pow(fog1.y, GAMMA),
                            (float)pow(fog1.z, GAMMA));
        wind->color.w = 1.0f;                      // the original copies the sun's w lane here
        ((rgl::world*)g_script.level)->ambient = amb;
    } else {
        wind->color = fog1;
        ((rgl::world*)g_script.level)->ambient = fog3;
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
        float d = 3.3f - (cloud_amount + cloud_amount) - haze_amount;
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
        float f = haze_amount + cloud_amount;
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
                    rgl::entity** pp = &g_script.map_cell[j * 8 + i];
                    for (rgl::mesh** e = (*pp)->meshes._Myfirst; e < (*pp)->meshes._Mylast; e++)
                        (*e)->color = *c;
                    j++;
                } while (j < g_script.page_cols);
            }
            i++;
        } while (i < g_script.page_rows);
    }
}

// @ 0x517960
void EventPool::next(int* index)
{
    ++*index;
    while (*index < count && !pages[(unsigned)*index >> 4][*index & 0xf].in_use)
        ++*index;
}

// @ 0x5179b0
int EventPool::first()
{
    int i = 0;
    if (count > 0) {
        do {
            if (pages._Myfirst[(unsigned)i >> 4][i & 0xf].in_use != 0)
                break;
            i++;
        } while (i < count);
    }
    return i;
}

static __forceinline void cursor_fade(rgl::strategic_entity* m, const float& v) { m->set_fade(&v); }   // a const float& temporary per arm
// @ 0x5179f0
void GameG::update_cursor(float dt)
{
    PartyMgr* mgr = &parties;

    mgr->at(mgr->lookup(main_party))->in_sight = 1;
    float radius = mgr->at(mgr->lookup(main_party))->map_speed();

    int id = mgr->first_party_id();
    while ((unsigned)id < mgr->serial) {
        unsigned cur = id;
        Party* p = raw_party(mgr, mgr->lookup(id));
        if (cur != (unsigned)main_party && p->attached_to < 0) {
            unsigned __int64 flags = p->flags;
            if ((flags & 0x1000100) == 0 && p->model != 0) {
                const PartyV4& mp = raw_party(mgr, mgr->lookup(main_party))->pos;
                float dx = p->pos.x - mp.x;
                float dy = p->pos.y - mp.y;
                float dist = (float)sqrt(dx * dx + dy * dy);
                if (!((flags & 0x4000) == 0)) {
                    p->alpha = 1.0f;
                    if (dist <= radius * 0.5f && p->in_sight == 0) {
                        p->in_sight = 1;
                        p->dirty = 1;
                        emit_event(0xc, (int)cur, 0);
                    } else if (radius + radius < dist && p->in_sight != 0) {
                        p->in_sight = 0;
                        p->dirty = 1;
                        emit_event(0xd, (int)cur, 0);
                    }
                } else {
                    if (dist <= radius && dist <= p->strength() * radius) {
                        g_script.trigger_result.store1 = 0;
                        g_script.trigger_result.store0 = 1;
                        emit_event(0x19, main_party, (int)cur);
                        if ((g_script.trigger_result.store0 | g_script.trigger_result.store1) != 0) {
                            float f = p->alpha + dt * 30.0f;
                            p->alpha = f;
                            if (f > 1.0f || f < -100.0f)
                                p->alpha = 1.0f;
                            if (p->in_sight == 0) {
                                p->in_sight = 1;
                                emit_event(0xc, (int)cur, 0);
                            }
                        } else {
                            float f = p->alpha - dt * 5.0f;
                            p->alpha = f;
                            if (f < 0.0f || f < -100.0f)
                                p->alpha = 0.0f;
                            if (p->in_sight != 0) {
                                p->in_sight = 0;
                                emit_event(0xd, (int)cur, 0);
                            }
                        }
                    } else {
                        float f = p->alpha - dt * 5.0f;
                        p->alpha = f;
                        if (f < 0.0f || f < -100.0f)
                            p->alpha = 0.0f;
                        if (p->in_sight != 0) {
                            p->in_sight = 0;
                            emit_event(0xd, (int)cur, 0);
                        }
                    }
                }
                if (!(reveal_all == 0)) {
                    p->alpha = 1.0f;
                    p->in_sight = 1;
                } else {
                    if (p->alpha < 0.01f)
                        cursor_fade(p->model, 0.0f);
                    else
                        cursor_fade(p->model, 1.0f);
                }
            }
        }
        mgr->next_party_id(&id);
    }

    int i = 0;
    if (events.count > 0) {
        do {
            if (events.pages._Myfirst[(unsigned)i >> 4][i & 0xf].in_use != 0)
                break;
            i++;
        } while (i < events.count);
    }
    while ((unsigned)i < (unsigned)events.count) {
        Event* r = events.pages._Myfirst[(unsigned)i >> 4] + (i & 0xf);
        if (reveal_all != 0) {
            r->f44 = 1.0f;
        } else {
            const PartyV4& mp = raw_party(mgr, mgr->lookup(main_party))->pos;
            float dx = r->pos.x - mp.x;
            float dy = r->pos.y - mp.y;
            float dist = (float)sqrt(dx * dx + dy * dy);
            if (dist <= radius) {
                float f = dt * 30.0f + r->f44;
                r->f44 = f;
                if (!(f > 1.0f)) { if (f < -100.0f)
                    r->f44 = 1.0f; } else { r->f44 = 1.0f; }
            } else {
                float f = r->f44 - dt * 5.0f;
                r->f44 = f;
                if (f < 0.0f || f < -100.0f)
                    r->f44 = 0.0f;
            }
        }
        event_pool(events).next(&i);
    }
}

// PartyMgr::next_party_id (the row is owned elsewhere): a visible duplicate, so update_weather's loop counter can
// stay in a register across the call.
__declspec(noinline) void PartyMgr::next_party_id(int* id)
{
    unsigned i = lookup(*id) + 1;
    while ((int)i < count && pages._Myfirst[i >> 4][i & 0xf].type != 1)
        i++;
    if ((int)i >= count)
        *id = serial;
    else
        *id = pages._Myfirst[i >> 4][i & 0xf].id;
}

// @ 0x517e70
void GameG::update_weather()
{
    PartyMgr* mgr = &parties;

    int id = mgr->first_party_id();
    while ((unsigned)id < parties.serial) {
        if (id == main_party
            || (party_at(mgr, mgr->lookup(id))->flags & 0x80100) == 0)
            party_at(mgr, mgr->lookup(id))->tick_stats();
        mgr->next_party_id(&id);
    }

    __int64 cur = g_ticks[f13b18];
    __int64 ref = clock;
    int hour = (int)((float)(cur - ref) * 1e-5f) % 24;

    float v = ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.5f) * 0.1f + cloud_amount;
    cloud_amount = v;
    if (hour > 0 && hour < 8 && haze_amount < 0.9f) {
        float k = 1.0f;
        if (v > 0.4f)
            k = 1.5f;
        haze_amount += ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.1f) * k * 0.05f;
    } else if (hour > 10 && hour < 18 && haze_amount > 0.1f) {
        float k = 1.0f;
        if (v < 0.6f)
            k = 1.5f;
        haze_amount -= ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.1f) * k * 0.05f;
    }

    float u = (float)(rand() % 0x3dc9) * 6.3223117e-5f;
    haze_amount += ((cloud_amount - 0.55f) * u) * 0.01f;

    float f = ((float)(rand() % 0x3dc9) * 6.3223117e-5f - 0.5f) * 0.1f + haze_amount;
    haze_amount = f;
    if (f < 0.0f)
        haze_amount = 0.0f;
    else if (f > 1.0f)
        haze_amount = 1.0f;

    if (cloud_amount < 0.0f)
        cloud_amount = 0.0f;
    else if (cloud_amount > 1.0f)
        cloud_amount = 1.0f;

    if (hour >= 18) {
        if ((float)(rand() % 0x3dc9) * 6.3223117e-5f < 0.25f && string_count > f13b60)
            goto fire;
    }
    if (hour >= 22 && string_count > f13b60) {
fire:
        weekly_report((string_count & 7) == 0);
        f13b60 = string_count;
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

    if (p->morale < 0.2f) {
        tstring key("ui_party_morale_is_low");
        body.append(*g_strtable.lookup(key));
        float f = 0.3f - p->morale;
        if (factor1 > f)
            f = factor1;
        factor1 = f * 0.2f;
    }

    if (troop->gold < 0)
        troop->gold = 0;

    int counts[32];
    int ids[32];
    int nsel = 0;
    int i = p->nstacks - 1;
    for (; i > 0; i--) {
        Stack* s = &p->stacks[i];
        int hits = 0;
        float thr = factor1;
        if (s->flags & 1)
            thr = factor2;
        if ((troops[s->troop].flags30 & 0x10) == 0) {
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
                p->consume_stack(i, hits, &stack_seeds[0][0], 0);
        }
    }

    if (nsel != 0) {
        int before_last = nsel - 2;
        for (int j = 0; j < nsel; j++) {
            body.push_back(' ');
            Troop* tr = &troops[ids[j]];
            if ((tr->flags30 & 0x10) != 0) {
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
            t13ab8 = *g_strtable.lookup(key);
            t13aa8 = body;
            f13ac8 = 1;
        }
    }
    // body's destructor
}
