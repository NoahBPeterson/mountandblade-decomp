//-----------------------------------------------------------------------------
// File: agent_effects_sound.cpp  (0x441ff0..0x444c50)
//
// Desc: int lists, record copy helpers, entities-in-box query, effect element/definition lists, emitter setup, SoundMgr::play, experience, effect spawn, dead queue, target scans, sound pickers, equipment display, wield change
//       One part of what was game/agent4/agent_logic.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define ANIM_SLOT_SET_OUT_OF_LINE       // 0x4378a0 is a real call here (the body is game/agent/agent_stats.cpp's), written out at two sites of on_wield_changed
#include <vector>
#include <deque>
#include "engine/math/rgl_math.h"
#include "engine/core/std_vec_out_of_line.h"
#include "game/agent/ai/agent_types.h"
#include "game/agent/combat/agent_types.h"
#include "game/agent/ai/agent_ai.h"
#include "game/agent/agent_logic.h"
#include "game/common/game_globals.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <algorithm>
#include <stdexcept>
#include "game/dialog/dialog_manager.h"      // g_script 0xa70440 (g_ev_a / g_ev_b live in it; listed directly: the resolution audit reads only the source's own includes)
#include "game/agent/agent_stats.h"
#include "game/agent/anim_slot.h"      // (named here too for the resolution audit)
#include "engine/fx/env_curve.h"      // Curve: the canonical class of the Skeleton record (Curve::recs +0x40 is Skeleton::chans)
#include "engine/math/mat4_include.h"
#include "engine/core/ptr_tag.h"
#include "engine/core/timer.h"
#include "engine/math/vec.h"
#include "engine/physics/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/physics/phys_set.h"
#include "engine/physics/height_field.h"
#include "engine/scene/world.h"
#include "engine/physics/rigid_body_rgl_dynamic.h"      // rgl::dynamic_entity: a kind-7 prop's rigid body (spawn_prop 0x44c210)
#include "engine/render/light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/mission/placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/resource/skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "game/data/item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/agent/agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/mission/mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/terrain/poly_mesh.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/skill_cap.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/data/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/terrain/scene_file.h"     // SceneFile: GameData::scenes' record (flags)
#include "game/data/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/party/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include <vector>
#include "game/data/str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/config/options.h"   // g_options 0x5aa008
#include "game/data/item_kind_include.h"   // g_items 0x5e29d0
#include "game/input/slot.h"
#include "game/agent/combat/agent_combat.h"
#include "engine/core/tstring.h"
#include "engine/render/font.h"
#include "game/common/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/agent/physics/phys_set.h"
#include "engine/math/quaternion.h"
#include "game/data/env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "engine/resource/res_mgr.h"
#include "game/config/config_block.h"
#include "engine/fx/fx.h"
#include "game/data/emit.h"
#include "game/ui/gui_manager.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "engine/audio/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "engine/render/mesh.h"
#include "game/dialog/dialog_mgr.h"
#include <memory>
#include "game/agent/agent_queries.h"
#include "game/input/key_names.h"      // g_keyboard (0xa170c8): pick_sound_ids
#include "game/data/mode_rec.h"
#include "engine/render/mesh_parts.h"
#include "engine/render/mesh_prims.h"
#include "engine/core/hnode.h"
#include "game/agent/blow.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "game/script/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "game/mission/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)

// ---- definitions from the old file's prelude and earlier sections that this part's code uses (copies; each is a static
// or inline helper, a type or a macro: none of them emits code or data of its own)
// vector4::out_length 0x407c50: a header inline in the original, left out of line by cl. fire_check's g_mission survives the call, so the body was visible.
inline __declspec(noinline) float rgl::vector4::out_length() const { return length(); }
// 0x433770 (rgl::psys::set_placement, game/agent/agent_item_pick.cpp owns the row): the body
// visible, so the frame EmitTable::setup (0x4424c0) passes does not escape and its origin stores are independent of the loads through d.
inline __declspec(noinline) void rgl::psys::set_placement(const matrix* m, const vector4* a, const vector4* b, float c)
{
    unsigned f = flags & ~1u;
    flags = f;
    local = *m;
    box = *a;
    vel_base = *b;
    vel_scale = c;
    if (f & 2) {
        spawn_left = spawn_total = 1000000000;
        spawn_accum = 0;
    }
}
// 0x40cd60 / 0x40cd00 (rgl::identity3 / rgl::identity_pos, engine/physics/collision_shapes.cpp owns the rows): the bodies visible, so a frame passed to
// identity_pos does not escape and its later stores are independent of pointer loads (EmitTable::setup 0x4424c0).
inline __declspec(noinline) rgl::mat3* __cdecl rgl::identity3(rgl::mat3* out)
{
    vector4 a(1.0f, 0.0f, 0.0f), b(0.0f, 1.0f, 0.0f), c(0.0f, 0.0f, 1.0f);
    out->s = a;
    out->f = b;
    out->u = c;
    return out;
}
inline __declspec(noinline) rgl::matrix* __cdecl rgl::identity_pos(rgl::matrix* out)
{
    vector4 o(0.0f, 0.0f, 0.0f);
    rgl::mat3 m;
    *(rgl::mat3*)out = *identity3(&m);
    out->o = o;
    return out;
}
// Bodies the original's object saw for the box query chain (0x42aa50 strat_entity, 0x426500 / 0x426690 / 0x426700
// world_collide): the shape array and the count passed to query_box do not escape
// Body the original's object saw (row owner engine/scene/frustum_grid.cpp 0x41ed00): with it visible lo/hi do not escape,
// so hi[1] is not reloaded after each cell walk and, with no callee-saved register left, stays in memory
inline __declspec(noinline) void rgl::world::to_cell(const float* p, int* out)
{
    for (int i = 0; i < 2; i++) {
        float f = (p[i] - lo[i]) / cell_size[i];
        if (f <= 0.0f)
            out[i] = 0;
        else if ((float)(n[i] - 1) <= f)
            out[i] = n[i] - 1;
        else
            out[i] = (int)f + 1;
    }
}
// ---- the agent table (agents are 0x5928-byte records in pages of 16) --------------------------------------------------------------------------------------------
// @ 0x43b160
inline void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].in_use != 0) break;
        ++*i;
    }
}
// @ 0x43b1b0
inline int AgentTable::first_used()
{
    unsigned v = 0;
    if (0 < count) {
        Agent** pg = &pages[0];
        do {
            if (pg[v >> 4][v & 0xf].in_use != 0) break;
            v++;
        } while ((int)v < count);
    }
    return v;
}
// itcf_shoot_* (Module System header_items): the ranged-attack capability bits of an item.
enum {
    itcf_thrust_polearm = 0x100,
    itcf_shoot_bow      = 0x1000,
    itcf_shoot_crossbow = 0x4000,
    itcf_throw_stone    = 0x10000,
    itcf_throw_knife    = 0x20000,
    itcf_throw_axe      = 0x30000,
    itcf_throw_javelin  = 0x40000,
    itcf_shoot_pistol   = 0x70000,
    itcf_shoot_musket   = 0x80000,
    itcf_shoot_mask     = 0xff000,
};
typedef matrix frame;   // a 4-row frame as the original builds it inline: three rows from identity3, origin zero

//=============================================================================
// 0x441ff0..0x444c50, from agent_fx_sound.cpp
// Desc: an object that keeps int lists, the 0x38-byte record copy / fill helpers, the world's "entities fully
//       touching a box" query, the effect element / definition lists, the emitter setup from the definition
//       table, the FMOD sound player, game-data experience (level-up bookkeeping and its messages), the mission's
//       effect spawn and dead-agent queue, the agent's target scans, sound pickers and players, the equipment
//       display and the wield-change animation / sound selection. Functions in ascending address order (cl's
//       bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
#include "game/agent/combat/agent_fx_sound.h"   // after the 0x43e190.. section: its itcf_* macros are i64, that section's enum is int
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


const int g_xp_table[63] = {                             // 0x590974
    0, 600, 1360, 2296, 3426, 4768, 6345, 8179, 10297, 13010, 16161, 19806, 24007, 28832, 34362, 40682, 47892, 56103, 65441, 77233,
    90809, 106425, 124371, 144981, 168636, 195769, 226879, 262533, 303381, 350164, 412091, 484440, 568947, 667638, 782877, 917424,
    1074494, 1257843, 1471851, 1721626, 2070551, 2489361, 2992033, 3595340, 4319408, 5188389, 6231267, 7482821, 8984785, 11236531,
    14051314, 17569892, 21968215, 27466219, 34338823, 42929679, 53668349, 67091786, 83871183, 160204600, 320304600, 644046000, 2050460000 };


// 0x440b30 vector<int>::push_back (agent_combat.cpp) is kept out of line


#include "engine/core/std_vec_insert_n.h"      // vector<rgl::mesh*>::_Insert_n 0x410610 (declared only: the call is out of line)
struct named_object;
template<> void std::vector<named_object*, std::allocator<named_object*> >::_Insert_n(std::vector<named_object*, std::allocator<named_object*> >::iterator, size_t, named_object* const&);   // 0x41a2c0 (engine/resource/resource_manager.cpp)

// @ 0x441ff0
void rgl::lod_mesh::add_mesh(int idx, rgl::mesh* val)
{
    lists[idx].push_back(val);
    if (idx + 1 > count)
        count = idx + 1;
}

// ---- std::_Uninit_copy of the 0x38-byte PhysRec ------------------------------------------------------------
// @ 0x442060
template void std::allocator<PhysRec>::construct(PhysRec*, const PhysRec&);
// @ 0x442080 (+ catch-all funclet 0x4420ea: destroys [next, dest), then `throw;`)
template PhysRec* std::_Uninit_copy(PhysRec*, PhysRec*, PhysRec*, std::allocator<PhysRec>&, std::_Nonscalar_ptr_iterator_tag);

// 0x442110 rgl::entity::add_mesh: the header's inline member (engine/scene/scene_entity.h), emitted out of line here

// ---- the world's box query --------------------------------------------------------------------------------------------
static __forceinline bool box3_inl(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return true;
    return false;
}

// @ 0x442180
void rgl::world::collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<entity*>* out)
{
    out->clear();
    int lo[2];
    int hi[2];
    int c[2];
    to_cell(&bmin->x, lo);
    to_cell(&bmax->x, hi);
    for (c[0] = lo[0]; c[0] <= hi[0]; c[0]++) {
        for (c[1] = lo[1]; c[1] <= hi[1]; c[1]++) {
            for (cellnode* nd = cells[c[0]][c[1]].head; nd != 0; nd = nd->next) {
                entity** pe = &nd->obj;
                if (box3_inl(bmin, bmax, (*pe)->get_v9c(), (*pe)->get_vac()))
                    out->push_back(*pe);
            }
        }
    }
    sort_range(out->begin(), out->end(), (int)(out->end() - out->begin()));
    std::vector<entity*>::iterator e = std::unique(out->begin(), out->end());
    out->erase(e, out->end());
}

// ---- effect elements / definitions ----------------------------------------------------------------------------------------
// @ 0x442390
void FxElem::add_psys(rgl::mesh* p) { psys.push_back(p); }   // the list holds the particle meshes (Psys* here): vector<mesh*> instance 0x410610
// @ 0x4423f0
void FxElem::add_group(rgl::lod_mesh* g)
{
    groups.push_back(g);         // the slow path is vector<lod_mesh*>::_Insert_n 0x41a2c0 (resource_manager.cpp's instance)
    ngroups++;
}
// @ 0x442460
void FxDef::attach_psys(int idx, rgl::mesh* p, const mat4* m)
{
    p->local_frame = *m;
    elems[idx].add_psys(p);
}
// @ 0x4424a0
void FxDef::attach_group(int idx, rgl::lod_mesh* g)
{
    elems[idx].add_group(g);
}

// ---- emitter setup from a definition table ----------------------------------------------------------------------------------
// @ 0x4424c0
void EmitTable::setup(rgl::psys* e, int idx, const vector4* pos)
{
    if (idx < 0 || idx >= count)
        return;
    EmitDef* d = &defs[idx];
    rgl::mesh* r = (rgl::mesh*)g_resmgr.get_object(&d->mesh_name);
    if (r == 0)
        return;
    e->init();
    e->tag = idx;
    e->src = r;
    e->res = r->res;
    e->flags = d->flags;
    e->spawn_rate = d->spawn_rate;
    e->lifetime = d->life;
    e->drag = d->drag;
    vector4 g(0.0f, 0.0f, -9.8f);
    e->accel = g;
    e->accel_scale = d->accel_scale;
    e->noise_scale = d->noise_scale;
    e->noise_amount = d->noise_amount;
    for (int k = 0; k < 5; k++)
        e->curve[k] = d->t[k];
    e->spin_scale = d->rot_speed;
    e->spin_drag = d->rot_damping;
    mat4 m;
    rgl::identity_pos(&m);
    m.o = *pos;
    e->set_placement(&m, &d->a, &d->b, d->c);
}

// ---- the FMOD sound player ---------------------------------------------------------------------------------------------------
static __forceinline int round_i(float v) { return round_float(&v); }
// 0x432510 (game/common/game_util.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) int __cdecl clamp_index(const int* v, const int* lo, const int* hi)      // 0x432510
{
    if (*v < *lo) return *lo;
    if (*v >= *hi) return *hi - 1;
    return *v;
}

// @ 0x442650
int SoundMgr::play(int idx, int flags, const vector4* pos, float freq, float* out_duration)
{
    int ret = -1;
    if (g_cfg.disable_sound == 0 && idx >= 0 && idx < ngroups) {
        int* pn = &groups[idx].nvar;
        int n = *pn;
        if (n > 0) {
            int k;
            if (groups[idx].nvar <= 1)
                k = 0;
            else
                k = rand() % *pn;
            int dn = groups[idx].var[k];
            int ch = FSOUND_PlaySoundEx(-1, defs[dn].sample, 0, 1);
            ret = ch;
            if (out_duration)
                *out_duration = defs[dn].ratio;
            int t = ch;                     // the handle pushed for a looping sound: address-taken, stored before the calls
            if (ch >= 0) {
                int hi = 0x100;
                int vol = 0;
                int r = round_i(groups[idx].volume_scale() * g_options.master_volume * 255.0f);
                vol = clamp_index(&r, &vol, &hi);
                int looping = groups[idx].flags & 2;
                int prio;
                if (flags & 1) {
                    FSOUND_3D_SetAttributes(ch, 0, 0);
                    prio = 0xff;
                } else {
                    FSOUND_3D_SetAttributes(ch, &pos->x, 0);
                    int lo = 0;
                    int hi2 = 0x100;
                    float dist = (frame.o - *pos).out_length();
                    float f = 3.0f / (dist + 3.0f);
                    int pr = round_i(defs[dn].range_scale() * f * 254.0f);
                    prio = clamp_index(&pr, &lo, &hi2);
                }
                FSOUND_SetVolume(ch, vol);
                FSOUND_SetLoopMode(ch, (looping != 0) + 1);
                if (groups[idx].flags & 4)
                    FSOUND_SetCurrentPosition(ch, rand_below(&defs[dn].length));
                if (looping)
                    channels.push_back(t);
                if (freq != 1.0f && g_cfg.disable_frequency_variation == 0)
                    FSOUND_SetFrequency(ch, (int)((float)defs[dn].freq * freq));
                FSOUND_SetPriority(ch, prio);
                FSOUND_SetPaused(ch, 0);
            }
        }
    }
    return ret;
}
// @ 0x4428a0
int SoundMgr::play_slot(int n, int flags, const vector4* pos, float freq, float* out_duration)
{
    n = slot[n];
    return play(n, flags, pos, freq, out_duration);
}

// ---- experience and its messages ------------------------------------------------------------------------------------------------
// the ring-buffer append that this TU inlines (colour 0xfff000)
static __forceinline void log_add(const tstring& s)
{
    if (g_gui.enabled && g_gui.ring.count < 32) {
        int idx = (g_gui.ring.head + g_gui.ring.count) % 32;
        g_gui.ring.names[idx] = s;
        g_gui.ring.vals[idx] = 0xfff000;
        g_gui.ring.count++;
    }
}

// @ 0x4428c0
int GameData::add_experience(int troop, int amount)
{
    char buf1[512], buf2[512], buf3[512];
    if (amount < 0)
        amount = 0;
    else if (amount >= 30000)
        amount = 29999;
    if (troop == player && amount > 0 && f13a34 != 0 && g_options.show_messages != 0) {
        {
            tstring key("ui_you_got_number_experience");
            sprintf(buf1, g_strtable.lookup(key)->c_str(), amount);
        }
        {
            tstring msg(buf1);
            g_gui.push(msg, -1);
        }
    }
    Troop* c = &chars[troop];
    c->exp += amount;
    while (c->exp >= g_xp_table[c->level]) {
        c->skill_points++;
        c->attr_points++;
        c->prof_points += 10;
        c->level++;
        if (troop == player) {
            g_sound_mgr.play(g_sound_mgr.slot[1], 1, &g_sound_mgr.frame.o, 1.0f, 0);
            {
                tstring key("ui_you_have_advanced_to_level_number");
                sprintf(buf2, g_strtable.lookup(key)->c_str(), c->level);
            }
            {
                tstring msg(buf2);
                log_add(msg);
            }
        } else {
            PartyMgr* pm = &parties;
            int i = 0;
            int p = pm->lookup(main_party);
            if (pm->pages[p >> 4][p & 0xf].count > 0) {
                do {
                    int q = pm->lookup(main_party);
                    if (pm->pages[q >> 4][q & 0xf].stacks[i].troop == troop) {
                        {
                            tstring key("ui_value_has_advanced_to_level_number");
                            tstring* nm = g_strtable.find(&c->key);
                            if (nm == 0)
                                nm = (tstring*)c;
                            sprintf(buf3, g_strtable.lookup(key)->c_str(), nm->c_str(), c->level);
                        }
                        {
                            tstring msg(buf3);
                            log_add(msg);
                        }
                    }
                    i++;
                    int r = pm->lookup(main_party);
                    if (!(i < pm->pages[r >> 4][r & 0xf].count))
                        break;
                } while (1);
            }
        }
    }
    return c->exp;
}

// ---- spawning an effect emitter ---------------------------------------------------------------------------------------------------
// @ 0x442ca0
rgl::psys* Mission::spawn_effect(int idx, const mat4* m, int duration)
{
    rgl::psys* e = ((rgl::world*)scene)->alloc_emitter();
    vector4 v(0.0f, 0.0f, 0.0f);
    g_emit_defs.setup(e, idx, &v);
    e->world = *m * e->local;
    if (e->res != 0 && (e->res->flags & 0x40)) {
        vector4 t;
        t.x = g_env_rows[env_row].v20[0] * 0.3f;
        t.y = g_env_rows[env_row].v20[1] * 0.3f;
        t.z = g_env_rows[env_row].v20[2] * 0.3f;
        EnvRow* c = &g_env_rows[env_row];
        v.x = t.x + c->base[0];
        v.y = t.y + c->base[1];
        v.z = t.z + c->base[2];
        vector4* d = &e->tint;
        *d = v;
    }
    e->flags &= ~1u;
    e->spawn_left = e->spawn_total = duration;
    e->spawn_accum = 0;
    return e;
}

// @ 0x442dc0
void Mission::queue_dead(Agent* a)
{
    dead.push_back(a);
    while (dead.size() > g_options.max_dead) {
        dead[0]->f5830 = 4;
        Agent* o = dead[0];
        Timer* tm = &o->tm;
        tm->start = g_ticks[tm->clock];      // Timer::restart inlined
        for (unsigned i = 1; i < dead.size(); i++)
            dead[i - 1] = dead[i];
        dead.pop_back();
    }
}

// ---- agent scans -------------------------------------------------------------------------------------------------------------------
// @ 0x442ed0
bool Agent::can_target(unsigned aid, unsigned flags, float maxdist, float* out_dist)
{
    Agent* o = &g_mission->agents.pages[aid >> 4][aid & 0xf];
    if (o->state == 1
        && (!(flags & 1) || !is_enemy(o))
        && (!(flags & 2) || !is_friend(o))
        && (!(flags & 4) || !(char)is_neutral(o))
        && (!(flags & 0x10) || o->kind == 1)
        && (!(flags & 0x20) || o->kind != 1)
        && (!(flags & 0x200) || o->pool_id > pool_id)
        && o->pool_id != pool_id
        && (!(flags & 0x2000) || line_of_sight(aid, 0))
        && o->pool_id != horse) {
        vector4 d = o->pos - pos;
        float len = d.normalize();
        *out_dist = len;
        if (maxdist >= 0.0f && len > maxdist)
            return false;
        if (flags & 0x1000) {
            float dot = d.x * in_now.move.x + (d.z * in_now.move.z + d.y * in_now.move.y);
            if (dot < -0.1f)
                return false;
        }
        return true;
    }
    return false;
}

// @ 0x443020
bool Agent::any_hostile_near()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned i = t->first_used();
    while (i < (unsigned)t->count) {
        Agent* o = &m->agents.pages[i >> 4][i & 0xf];
        if (o->state == 1) {
            bool friendly = false;
            if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
                friendly = g_mission->teams[team].relation[o->team] == 1;
            if (!friendly && !(char)is_neutral(o) && o->kind != 1 && o->pool_id != pool_id && o->pool_id != horse) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                if (!(300.0f < len) && line_of_sight(i, 0))
                    return true;
                m = g_mission;
                t = &m->agents;
            }
        }
        t->next_used(&i);
    }
    return false;
}

// @ 0x443140
void Agent::update_alarm()
{
    if (ai.alarm & 1)
        return;
    bool hit = false;
    AgentTable* t = &g_mission->agents;
    unsigned i = t->first_used();
    unsigned n = t->count;
    while (i < n) {
        if (hit)
            break;
        Agent** pg = &(&*g_mission->agents.pages.begin())[0];
        Agent* o = pg[i >> 4] + (i & 0xf);
        if (o->state == 1 && !is_enemy(o) && !(char)is_neutral(o) && o->kind != 1 && o->pool_id != pool_id && o->pool_id != horse) {
            vector4 d = o->pos - pos;
            if (!(10.0f < d.normalize()))
                hit = (pg[i >> 4][i & 0xf].ai.alarm & 1) != 0;
        }
        g_mission->agents.next_used(&i);
    }
    if (hit || any_hostile_near()) {
        ai.alarm |= 1;
        ai.ta_interval = 0;
    }
}

// ---- agent sound pickers and players ---------------------------------------------------------------------------------------------------
// @ 0x443270
unsigned Agent::pick_shout_target(int flags, float maxdist)
{
    unsigned best = (unsigned)-1;
    float bestv = 0.0f;
    AgentTable* tbl = &g_mission->agents;
    unsigned i = tbl->first_used();
    if (i < (unsigned)tbl->count) {
        do {
            unsigned id = i;
            float dist;
            bool ok = can_target(id, flags, maxdist, &dist);
            Mission* m = g_mission;
            if (ok) {
                float x = 3.0f / (dist + 2.0f);
                float t = (x < 1.0f) ? x : 1.0f;
                float w = 1.0f / ((float)m->agents[id].targeted_by + 7.0f);
                float k = 1.0f;
                if (horse >= 0 && dist < 40.0f) {
                    vector4 d;
                    const vector4& op = m->agents.pages[id >> 4][id & 0xf].pos;
                    d.x = op.x - pos.x;
                    d.y = op.y - pos.y;
                    d.z = op.z - pos.z;
                    dist = d.normalize();
                    const vector4& fw = body->local.f;
                    float dp = d.y * fw.y;
                    dp += d.z * fw.z;
                    dp += d.x * fw.x;
                    if (dp > 0.0f)
                        k = ((40.0f - dist) * dp) * dp + 1.0f;
                }
                float s = (k * w) * t;
                if (s > bestv) { bestv = s; best = id; }
            }
            tbl = &m->agents;
            tbl->next_used(&i);
        } while (i < (unsigned)tbl->count);
    }
    return best;
}

static inline bool fire_ok(Agent* a, unsigned id) { int fc; return a->fire_check(id, &fc) == 1; }
// @ 0x443430
unsigned Agent::pick_threat()
{
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned id = (unsigned)-1;
    int n = 0;
    if (0 < ai.nthreat) {
        Threat* r = ai.threats;
        do {
            id = r->id;
            Agent* o = &g_mission->agents.pages[id >> 4][id & 0xf];
            if (o->state == 1 && line_of_sight(id, 0)) {
                if (fire_ok(this, id)) {
                    const vector4& op = g_mission->agents.pages[id >> 4][id & 0xf].pos;
                    float dx = op.x - pos.x;
                    float dy = op.y - pos.y;
                    float dz = op.z - pos.z;
                    float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                    if (!(v < 4.0f)) v = 4.0f;
                    float score = v / ((float)g_mission->agents.pages[(unsigned)r->id >> 4][r->id & 0xf].targeted_by + 7.0f);
                    float s = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * score;
                    if (s > bestv) { bestv = s; best = r->id; }
                }
            }
            n++;
            r++;
            id = best;
        } while (n < ai.nthreat);
    }
    return id;
}

// @ 0x4435c0
void Agent::play_hit_sound(int* kind)
{
    Mission* m = g_mission;
    if (m->muted)
        return;
    int snd;
    if (this->kind == 0) {
        int k = *kind;
        if (k < 0)
            return;
        if (k > 1) {
            if (k != 3)
                return;
            snd = 0xe;
        } else if (!(pos.z < m->base_z)) {
            bool b = (g_game->scenes[m->scene_idx].flags & 1) != 0;
            if (b)
                snd = 0x10;
            else
                snd = 0xf;
        } else {
            snd = 0x11;
        }
    } else {
        int k = *kind;
        unsigned c = gait & 7;
        if (k == 4) {
            snd = 0x61;
        } else if (pos.z > m->base_z) {
            if (c == 2 || c == 0) {
                if (k != 0 && k != 2) return;
                snd = 0x67;
            } else if (c == 3) {
                if (k != 0 && k != 2) return;
                snd = 0x68;
            } else if (c == 4) {
                if (k != 0) return;
                snd = 0x69;
            } else {
                if (c != 5) return;
                if (k != 0) return;
                snd = 0x6a;
            }
        } else {
            if (k != 2 && k != 3) return;
            snd = 0x11;
        }
    }
    int r = rand();
    float pitch = (float)(r % 0x3dc9) * 2.5289246e-05f + 0.8f;
    g_sound_mgr.play(g_sound_mgr.slot[snd], 0, &body->local.o, pitch, 0);
}

// @ 0x443730
void Agent::play_sound_var(int* kind, float* var)
{
    if (*kind != -1 && body != 0) {
        float pitch = 1.0f;
        if (*var > 0.001f) {
            float v = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * *var;
            v = v + v;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f)
                { float nv = -v; pitch = 1.0f / (nv + 1.0f); }
        }
        int own = 0;
        if (pool_id == g_mission->player_id)
            own = 1;
        vector4 p = body->local.o;
        float h = (this->kind == 0 && horse != -1) ? 2.4f : 1.5f;
        p.z += h - 0.3f;
        g_sound_mgr.play(g_sound_mgr.slot[*kind], own, &p, pitch, &pitch);
    }
}

// @ 0x443870
void Agent::play_sound(int* snd)
{
    if (*snd != -1 && body != 0) {
        if (channel >= 0) {
            FSOUND_StopSound(channel);
            channel = -1;
        }
        float pitch = 1.0f;
        Mission* m = g_mission;
        if (kind == 0 && (g_game->chars[troop].flags32 & 0xf) != 1 && pool_id != m->player_id) {
            float v = ((float)(((party * 0x244ad + (pool_id + 0x1d) * 0x14fd + troop * 0x76ef) % 0x1b41) * 0x51f % 0x367)
                       * 0.0011481056f - 0.5f) * 0.3f;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f) {
                float nv = -v;
                pitch = 1.0f / (nv + 1.0f);
            }
        }
        int own = 0;
        if (pool_id == m->player_id)
            own = 1;
        int ch = g_sound_mgr.play(*snd, own, &body->local.o, pitch, &pitch);
        if (ch >= 0) {
            channel = ch;
            snd_stamp.start = g_ticks[snd_stamp.clock];
            snd_length = pitch;
        }
    }
}

// @ 0x443a00
void Agent::play_pain_sound(float health)
{
    health = (1.2f - health) * 0.3f;
    float v = (health < 0.0f) ? 0.0f : ((health >= 0.5f) ? 0.5f : health);
    int r = rand();
    float pitch = (float)(r % 0x3dc9) * 6.3223115e-06f + (v + 0.95f);
    g_sound_mgr.play(g_sound_mgr.slot[0x11], 0, &body->local.o, pitch, 0);
}

// ---- the equipment display ----------------------------------------------------------------------------------------------------------------
static inline unsigned char itype(const ItemKind* k) { return k->type(); }

// shows `m` on the carry bone record `r` with local frame `f`
static __forceinline void attach(Agent* a, rgl::lod_mesh* m, int r, const rgl::matrix& f)
{
    FxDef* g = a->body->fx;
    g->remove_group(m);
    m->frame = f;
    FxElem& reg = g->elems[r];
    reg.groups.push_back(m);
    reg.ngroups++;
}

// @ 0x443aa0
void Agent::update_equipment_display()
{
    char slot_used[4];
    char bone_used[11];
    int i;
    for (int q = 0; q < 11; q++)       // (an explicit loop: the `= {0}` form gives a different frame)
        bone_used[q] = 0;
    for (i = 3; i >= 0; i--) {
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            if (A[i] != 0) {
                A[i]->mask = 0;
                if (itype(ik) == itp_type_shield) {
                    rgl::lod_mesh* m = A[i];
                    int k = 0;
                    if (0 < m->count) {
                        float limit = 300.0f;
                        do {
                            m->lists[k].cap_all(&limit);
                            k++;
                        } while (k < m->count);
                        k = 0;
                    }
                }
            }
            if (B[i] != 0)
                B[i]->mask = 0;
            if (C[i] != 0) {
                C[i]->mask = 0;
                rgl::lod_mesh* m = C[i];
                int k = 0;
                if (0 < m->count) {
                    float limit = 0.0f;
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->count);
                    k = 0;
                }
            }
            slot_used[i] = 0;
        }
    }
    int j;
    for (j = 0; j < 2; j++) {
        int s = wield[j];
        if (s >= 0 && pairs[s].item >= 0 && (j == 0 || both_hands == 0)) {
            ItemKind* ik = &g_items[pairs[wield[j]].item];
            unsigned t = ik->type_flags & 0xff;
            int region = -1;
            if (t == 2 || t == 3 || t == 4 || t == 9 || t == 10 || t == 0x10 || t == 0x11)
                region = 0x13;
            if (t == 8)
                region = 0xe;
            else if (t == 7)
                region = 0xc;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t3;
            mat3* ip = identity3(&t3);
            rgl::matrix M;
            static_cast<mat3&>(M) = *ip;
            M.o = z0;
            if ((g_items[pairs[s].item].flags & itcf_shoot_mask) == itcf_throw_javelin) {
                M.f = vector4(-M.f.x, -M.f.y, -M.f.z);
                M.s = vector4(-M.s.x, -M.s.y, -M.s.z);
            }
            if (A[wield[j]] != 0) {
                A[wield[j]]->mask = 0xffff;
                rgl::lod_mesh* m = A[wield[j]];
                float limit = 0.0f;
                int k = 0;
                if (0 < m->count) {
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->count);
                }
                attach(this, A[wield[j]], region, M);
            }
            if (B[wield[j]] != 0) {
                B[wield[j]]->mask = 0xffff;
                attach(this, B[wield[j]], region, M);
            }
            if ((ik->flags & itcf_carry_mask) != 0 && (ik->flags & itcf_show_holster_when_drawn) == 0)
                slot_used[wield[j]] = 1;
            rgl::lod_mesh* h = C[wield[j]];
            if (h != 0) {
                if ((ik->flags & itcf_show_holster_when_drawn) == 0) {
                    h->mask = 0;
                } else {
                    float limit = 10.0f;
                    int k = 0;
                    if (0 < h->count) {
                        do {
                            h->lists[k].cap_all(&limit);
                            k++;
                        } while (k < h->count);
                    }
                }
            }
        }
    }
    for (i = 3; i >= 0; i--) {
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            if ((ik->flags & itcf_carry_mask) != 0) {
                rgl::matrix M;
                int region, bone;
                ik->get_carry_pos(&M, &region, &bone);
                if (!bone_used[bone] && !slot_used[i] && (itype(ik) != itp_type_shield || (pairs[i].m4 & 0xffff) > 10)) {
                    bone_used[bone] = 1;
                    if (C[i] == 0) {
                        if (A[i] != 0) {
                            A[i]->mask = 0xffff;
                            attach(this, A[i], region, M);
                        }
                        if (B[i] != 0) {
                            B[i]->mask = 0xffff;
                            attach(this, B[i], region, M);
                        }
                    } else {
                        C[i]->mask = 0xffff;
                    }
                }
            }
        }
    }
    if (pool_id == g_mission->player_id && g_game->f139e4 != 0) {
        for (j = 0; j < 4; j++) {
            if (pairs[j].item >= 0) {
                ItemKind* ik = &g_items[pairs[j].item];
                if ((ik->flags & itcf_carry_mask) != 0) {
                    rgl::matrix M;
                    int bone, region;
                    ik->get_carry_pos(&M, &bone, &region);
                    if (bone == 9 || bone == 7 || bone == 8) {
                        if (wield[0] != j && (wield[1] != j || both_hands != 0)) {
                            if (A[j] != 0)
                                A[j]->mask &= 0x2000;
                            if (B[j] != 0)
                                B[j]->mask &= 0x2000;
                        }
                        if (C[j] != 0)
                            C[j]->mask &= 0x2000;
                    }
                }
            }
        }
    }
    update_quiver();
}

// ---- the wield change ----------------------------------------------------------------------------------------------------------------------------
// AnimSlot::set 0x4378a0 written out at these two sites only (every other caller in this file calls it)
static __forceinline bool aset_fx(AnimSlot& s, int id, unsigned prio)
{
    if ((prio & 0x1ffff) < (s.events & 0x1ffff) && (s.cur != -1 || s.next != -1))
        return false;
    s.next = id;
    s.events = prio;
    if (id == -1)
        s.events = 0;
    return true;
}
// Agent::set_order 0x4379c0 (agent_stats.cpp) expanded for slot 1; INL picks the inline AnimSlot::set (two of the three sites)
template <bool INL>
static __forceinline void set_order_1(Agent* a, int id, unsigned prio, float rate)
{
    if (id >= -1 && id < a->defs->count) {
        if (INL ? aset_fx(a->anim[1], id, prio) : a->anim[1].set(id, prio)) {
            a->anim[1].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
            if (prio & 0x4000000)
                a->body->fx->set_rec_phase(1, 0.0f);   // 0x433b20
            int k = a->wield[0];
            if (k >= 0 && a->pairs[k].item >= 0) {
                if (prio & 0x10000000)
                    a->A[k]->mask = 0;
                else
                    a->A[k]->mask = 0xffff;
            }
        }
    }
}
// 0x433b20 (engine/fx/env_curve.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) void FxDef::set_rec_phase(int i, float p)      // 0x433b20
{
    if (p < 0.0f) p = 0.0f;
    else if (p > 1.0f) p = 1.0f;
    EnvRec* r = &env[i];
    int ticks = (int)(r->unshape(p) * r->period * 100000.0f);
    r->tm.start = g_ticks[r->tm.clock] - ticks;
}

// @ 0x444370
void Agent::on_wield_changed()
{
    int anim_ = -1;
    int anim2 = -1;
    int snd2 = -1;
    int snd1 = -1;
    for (int j = 0; j < 2; j++) {
        int c = wield[j];
        int p = wield_prev[j];
        if (c != p) {
            int item;
            if (c < 0 || (item = pairs[c].item) < 0 || (j != 0 && both_hands != 0)) {
                if (j != 0) {
                    if (p >= 0 && c < 0 && both_hands == 0) {
                        int pi = pairs[p].item;
                        anim2 = 0x6d;
                        if (pi >= 0) {
                            ItemKind* ik = &g_items[pi];
                            unsigned t = ik->type_flags & 0xff;
                            if (t == itp_type_bow) {
                                anim2 = ((ik->flags & itcf_carry_mask) == itcf_carry_bowcase_left) ? 0x71 : 0x6f;
                                snd1 = 0x55;
                            } else if (t == itp_type_shield) {
                                anim2 = 0x6d;
                                snd1 = 0x57;
                            }
                        }
                    }
                } else {
                    if (p >= 0 && c < 0) {
                        int pi = pairs[p].item;
                        anim_ = 0x53;
                        if (pi >= 0) {
                            switch (g_items[pi].flags & itcf_carry_mask) {
                            case itcf_carry_mace_left_hip:
                            case itcf_carry_axe_left_hip:        anim_ = 0x59; snd2 = 0x49; break;
                            case itcf_carry_dagger_front_right:  anim_ = 0x61; snd2 = 0x53; break;
                            case itcf_carry_sword_left_hip:      anim_ = 0x55; snd2 = 0x45; break;
                            case itcf_carry_dagger_front_left:   anim_ = 0x5f; snd2 = 0x53; break;
                            case itcf_carry_revolver_right:      anim_ = 0x65; snd2 = 0x51; break;
                            case itcf_carry_pistol_front_left:   anim_ = 0x67; snd2 = 0x51; break;
                            case itcf_carry_spear:               anim_ = 0x5d; snd2 = 0x4d; break;
                            case itcf_carry_axe_back:            anim_ = 0x63; snd2 = 0x4b; break;
                            case itcf_carry_sword_back:          anim_ = 0x57; snd2 = 0x47; break;
                            case itcf_carry_crossbow_back:       anim_ = 0x5b; snd2 = 0x4f; break;
                            case itcf_carry_katana:              anim_ = 0x69; snd2 = 0x45; break;
                            case itcf_carry_wakizashi:           anim_ = 0x6b; snd2 = 0x53; break;
                            default:                             anim_ = 0x53; break;
                            }
                        }
                    }
                }
            } else {
                ItemKind* ik = &g_items[item];
                unsigned __int64 carry = ik->flags & itcf_carry_mask;
                unsigned t = ik->type_flags & 0xff;
                if (t == itp_type_bow) {
                    anim2 = (carry == itcf_carry_bowcase_left) ? 0x70 : 0x6e;
                    snd1 = 0x54;
                } else if (t == itp_type_shield) {
                    anim2 = 0x6c;
                    snd1 = 0x56;
                } else if (j == 0) {
                    switch (carry) {
                    case itcf_carry_mace_left_hip:
                    case itcf_carry_axe_left_hip:        anim_ = 0x58; snd2 = 0x48; break;
                    case itcf_carry_dagger_front_right:  anim_ = 0x60; snd2 = 0x52; break;
                    case itcf_carry_sword_left_hip:      anim_ = 0x54; snd2 = 0x44; break;
                    case itcf_carry_dagger_front_left:   anim_ = 0x5e; snd2 = 0x52; break;
                    case itcf_carry_revolver_right:      anim_ = 100;  snd2 = 0x50; break;
                    case itcf_carry_pistol_front_left:   anim_ = 0x66; snd2 = 0x50; break;
                    case itcf_carry_spear:               anim_ = 0x5c; snd2 = 0x4c; break;
                    case itcf_carry_axe_back:            anim_ = 0x62; snd2 = 0x4a; break;
                    case itcf_carry_sword_back:          anim_ = 0x56; snd2 = 0x46; break;
                    case itcf_carry_crossbow_back:       anim_ = 0x5a; snd2 = 0x4e; break;
                    case itcf_carry_katana:              anim_ = 0x68; snd2 = 0x44; break;
                    case itcf_carry_wakizashi:           anim_ = 0x6a; snd2 = 0x52; break;
                    default:
                        if (t == itp_type_polearm) { anim_ = 0x5c; snd2 = 0x4c; }
                        else { anim_ = 0x52; snd2 = 0x58; }
                        break;
                    }
                } else {
                    anim2 = 0x6c;
                }
            }
        }
        wield_prev[j] = c;
    }
    if (both_hands != 0)
        wield_prev[1] = -1;
    if (anim[1].cur == 0x7b)
        set_order_1<true>(this, -1, 0x46, 1.0f);
    if (anim_ != -1)
        set_order_1<true>(this, anim_, 0x5000046, 1.0f);
    else if (anim2 != -1)
        set_order_1<false>(this, anim2, 0x5000046, 1.0f);
    if (snd1 != -1 && (float)(g_ticks[g_mission->setup_timer.clock] - g_mission->setup_timer.start) * 1e-05f > 1.0f)
        g_sound_mgr.play(g_sound_mgr.slot[snd1], 0, &pos, 1.0f, 0);
    if (snd2 != -1 && (float)(g_ticks[g_mission->setup_timer.clock] - g_mission->setup_timer.start) * 1e-05f > 1.0f)
        g_sound_mgr.play(g_sound_mgr.slot[snd2], 0, &pos, 1.0f, 0);
    weapon_flags = 0;
    for (int j = 0; j < 2; j++) {
        int s = wield[j];
        if (s >= 0 && pairs[s].item >= 0 && (j == 0 || both_hands == 0)) {
            ItemKind* ik = &g_items[pairs[s].item];
            unsigned t = ik->type_flags & 0xff;
            if (t == itp_type_bow) {
                weapon_flags = 0x10;
                variant = 0;
            } else if (t == itp_type_crossbow || t == itp_type_thrown) {
                weapon_flags = 0x20;
                variant = 4;
            } else if (t == itp_type_two_handed_wpn && (ik->type_flags & itp_two_handed)) {
                weapon_flags = 4;
            } else if (t == itp_type_shield) {
                weapon_flags |= 0x1000;
            } else if (t == itp_type_polearm) {
                weapon_flags |= 8;
            } else if (t == itp_type_one_handed_wpn || t == itp_type_two_handed_wpn) {
                weapon_flags |= 2;
            }
        }
    }
    active = 0;
    f5610 = 0;
}

// ---- std::_Uninit_fill_n of the 0x38-byte PhysRec ---------------------------------------------------------------------------------------------
// @ 0x444bc0 (+ catch-all funclet 0x444c28: destroys [next, first), then `throw;`)
template void std::_Uninit_fill_n(PhysRec*, unsigned, const PhysRec&, std::allocator<PhysRec>&, std::_Nonscalar_ptr_iterator_tag);
