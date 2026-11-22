//-----------------------------------------------------------------------------
// File: agent_corpse_and_controls.cpp  (0x4554c0..0x457b40)
//
// Desc: spawn_corpse, tick_death, note_blow, update_controls
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
// visible bodies of callees another object defines (raises the match of the rows that call them; the calls stay calls)
inline __declspec(noinline) ItemRef Agent::slot_pair(int idx) const
{
    const ItemRef* p = 0;
    int i = slots[idx];
    if (i >= 0 && (idx == 0 || slots[4] == 0)) p = &pairs[i];
    ItemRef r;
    r.item = -1;
    if (p) r = *p;
    return r;
}
// @ 0x43b1f0
inline Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}
// Weapon-kind groups of the movement animation table (kind 9; 0 and 7; 1..6; everything else). The kind is the last blow's
// hit location (Blow::bone, +0x5674), left in the record by the blow that interrupted the move.
#define PICK(k9, k07, k16, kx, m9, m07, m16, mx, shared) \
    if (blow.bone == 9) { \
        if (mode == 2) return m9; \
        return (mode != 1 ? -16 : 0) + k9; \
    } \
    if (blow.bone == 0 || blow.bone == 7) { \
        if (mode == 2) return m07; \
        return (mode != 1 ? -16 : 0) + k07; \
    } \
    if (blow.bone == 1 || blow.bone == 4 || blow.bone == 2 || blow.bone == 5 || blow.bone == 3 || blow.bone == 6) { \
        if (mode == 2) return shared; \
        return (mode != 1 ? -16 : 0) + k16; \
    } \
    if (mode == 2) return mx; \
    return (mode != 1 ? -16 : 0) + kx;
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
// 0x440b30 vector<int>::push_back (agent_combat.cpp) is kept out of line


#include "engine/core/std_vec_insert_n.h"      // vector<rgl::mesh*>::_Insert_n 0x410610 (declared only: the call is out of line)
// 0x433b20 (engine/fx/env_curve.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) void FxDef::set_rec_phase(int i, float p)      // 0x433b20
{
    if (p < 0.0f) p = 0.0f;
    else if (p > 1.0f) p = 1.0f;
    EnvRec* r = &env[i];
    int ticks = (int)(r->unshape(p) * r->period * 100000.0f);
    r->tm.start = g_ticks[r->tm.clock] - ticks;
}
// ---- PolyMesh::find_path: A*-style search over the navigation faces ---------------------------------------------------------

// More bodies the original's object saw (rgl header inlines left out of line; collision_shapes.cpp and game_util.cpp own the rows).
inline __declspec(noinline) float rgl::vector4::normalize()
{
    float len = (float)sqrt(x * x + y * y + z * z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
inline __declspec(noinline) vector4 rgl::matrix::rot(const vector4& p) const
{
    return vector4(p.x * s.x + (f.x * p.y + u.x * p.z),
                   f.y * p.y + (s.y * p.x + u.y * p.z),
                   f.z * p.y + (s.z * p.x + u.z * p.z));
}
inline __declspec(noinline) vector4 rgl::cross(const vector4& a, const vector4& b) { return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline __declspec(noinline) float __cdecl fmax_ref(const float* a, const float* b) { return *a > *b ? *a : *b; }
inline __declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)     // 0x40e3d0
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
float frand();                                                          // 0x40e400
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
struct a_v2 { float x, y; a_v2() {} a_v2(const float& a, const float& b) : x(a), y(b) {} a_v2 operator*(const float& k) const { return a_v2(k * x, k * y); } void operator*=(const float& k) { x *= k; y *= k; } };
static inline float a_clampr(const float& x, float lo, const float& hi) { return clamp(&x, &lo, &hi); }
static inline float a_len2(const vec2& v) { return (float)sqrt(v.x * v.x + v.y * v.y); }
static inline bool a_bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }
// ---- Mission::spawn_missile ---------------------------------------------------------------------------------
// the missile record of an id: pages of 16 records of 0xb0 bytes at Mission+0x30 (Pool<Missile>, engine/core/pool.h)
static __forceinline Agent& mag(Mission* m, unsigned id) { return m->agents.pages[id >> 4][id & 0xf]; }
namespace rgl { static inline unsigned min255(unsigned v) { return v < 255 ? v : 255; } }
// 0x40e440 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
namespace rgl {
inline __declspec(noinline) unsigned pack_color(const vector4* c)      // 0x40e440
{
    unsigned a = (unsigned)(c->w * 256.0f);
    unsigned r = (unsigned)(c->x * 256.0f);
    unsigned g = (unsigned)(c->y * 256.0f);
    unsigned b = (unsigned)(c->z * 256.0f);
    return (((min255(a) << 8 | min255(r)) << 8) | min255(g)) << 8 | min255(b);
}
}
#include "game/mission/placed_obj.h"      // PlacedObj (Mission::props is a std::vector<PlacedObj>)

// @ 0x4554c0
void Agent::spawn_corpse()
{
    floor_face = 0;
    vector4 saved = pos;
    turn_rate = 0;
    int i = 0;
    {
        float z;
        face** pf = &floor_face;
        do {
            if (i >= 100)
                break;
            i++;
            if (i == 30 || i == 60)
                pos = saved;
            g_mission->scene->floor_at(&pos, &z, pf);
            if (*pf != 0)
                break;
            pos.x = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 0.01f + pos.x;
            pos.y = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * 0.01f + pos.y;
        } while (*pf == 0);
        pos.z = z;
    }
    int r;
    float fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&ai.hand_off.x)[0] = fv + fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&ai.hand_off.x)[1] = fv + fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&ai.hand_off.x)[2] = fv + fv;
    r = rand() % 0x3dc9;
    ai.action_countdown = 5.0f;
    spawn = pos;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&ai.hand_off.x)[3] = fv + fv;
    ragdoll_update();
    stand_animation = 1;
    if ((g_game->chars[troop].flags32 & 0xf) == 1)
        stand_animation = 0;
    walk_forward_animation = 9;
}

// ---- Agent::tick_death, Agent::note_blow -------------------------------------------------------------------------------------------
#define TICKS_SINCE(t) ((float)(g_ticks[(t).clock] - (t).start) * 1e-5f)
static void restart_timer(Timer& t) { t.start = g_ticks[t.clock]; }
// @ 0x457710
void Agent::tick_death()
{
    if (dismount_state == 1 && rider >= 0 && kind == 1 && TICKS_SINCE(tm) > 3.0f) {
        g_mission->agents.pages[(unsigned)rider >> 4][(unsigned)rider & 0xf].horse = -1;
        rider = -1;
        dismount_state = 2;
    }
    if (f5830 == 3) {
        if (dismount_state != 0) {
            if (!(TICKS_SINCE(tm) > 3.0f))
                return;
            f5830 = 4;
            restart_timer(tm);
            return;
        }
        if (disabled == 0 && queued == 0 && tm.elapsed() > 3.0f) {
            if (state != 1) {
                body->fx->set_visible(true);
                if (pool_id != g_mission->player_id)
                    drop_held();
            }
            if (pool_id != g_mission->player_id)
                g_mission->queue_dead(this);
            queued = 1;
        }
        if (g_options.max_ragdolls > g_phys.nworlds && TICKS_SINCE(tm) > 0.0f && t5918.elapsed() > 0.0f) {
            if (disabled == 0 && queued == 0)
                start_ragdoll();
        }
        if (disabled == 0)
            return;
        if ((floor_sound_done == 0 || !(t5918.elapsed() > 10.0f)) && !(t5918.elapsed() > 30.0f) &&
            (!(t5918.elapsed() > 1.0f) || !g_phys.none_sleeping(phys_idx)))
            return;
        on_removed();
    } else if (f5830 == 4) {
        if (queued == 0 && state != 1)
            return;
        float k = 1.0f - TICKS_SINCE(tm) * 0.33333334f;
        if (k < 0.0f)
            k = 0.0f;
        else if (k > 1.0f)
            k = 1.0f;
        vector4 c;
        c.x = 1.0f; c.y = 1.0f; c.z = 1.0f; c.w = k;
        body->set_color(rgl::pack_color(&c));
        body->shadow_fade = k;
        if (TICKS_SINCE(tm) > 3.0f) {
            f5830 = 5;
            release_body();
            if (state == 1)
                state = 2;
        }
    }
}
// @ 0x457af0
void Agent::note_blow(Blow* b)
{
    reduce_blow(b);
    if (has_blow == 0 || !(b->dmg_out < blow.dmg_out)) {
        blow = *b;
        has_blow = 1;
    }
}

// ---- Agent::update_controls: the movement update of an agent ------------------------------------------------------------------------
struct GaitBand { float lo, hi; };
extern const GaitBand g_gait_walk[3];
extern const GaitBand g_gait_mount[6];
const GaitBand g_gait_walk[3] = { { -10000.0f, 0.3f }, { 0.27f, 2.32f }, { 2.27f, 10000.08f } };       // 0x590940 {lo, hi} per speed band
const GaitBand g_gait_mount[6] = { { -3.0f, -0.01f }, { -0.01f, 0.01f }, { 0.2f, 2.1f }, { 2.0f, 3.9f }, { 3.8f, 6.3f }, { 6.2f, 200.5f } };   // 0x590910
float g_move_fwd;                                           // 0x5e2478
float g_move_side;                                          // 0x5e247c
#define PICK(m2, m3, m1, m0) (mode == 2 ? (m2) : mode == 3 ? (m3) : mode == 1 ? (m1) : (m0))
static __forceinline bool aset_inl(AnimSlot& s, int id, unsigned prio)         // 0x4378a0 inlined (AnimSlot::set is a call elsewhere in this object)
{
    if ((prio & 0x1ffff) < (s.events & 0x1ffff) && (s.cur != -1 || s.next != -1))
        return false;
    s.next = id;
    s.events = prio;
    if (id == -1)
        s.events = 0;
    return true;
}
static __forceinline void restart_inl(Timer& t) { t.start = g_ticks[t.clock]; }     // Timer::restart as the slice's callers inline it
static __forceinline void a_vadd(vector4& a, const vector4& b) { a.x += b.x; a.y += b.y; a.z += b.z; }      // vector4 += (x + 0.0f folds to a copy that stays)
// 0x4379c0 as cl inlines it (the call sites that pass a constant slot 1)
static __forceinline void set_order_inl(Agent* a, int slot, int id, unsigned prio, float rate)
{
    if (id >= -1 && id < a->defs->count) {
        if (aset_inl(a->anim[slot], id, prio)) {
            a->anim[slot].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
            if (prio & 0x4000000)
                a->body->fx->set_rec_phase(slot, 0.0f);
            if (slot == 0) {
                if (id >= 0 && (a->defs->data[id].flags & 0x400) && a->anim[1].cur != -1 &&
                    (prio & 0x1ffff) > (a->anim[1].events & 0x1ffff))
                    aset_inl(a->anim[1], -1, prio & 0x1ffff);
            } else if (slot == 1) {
                int k = a->wield[0];
                if (k >= 0 && a->pairs[k].item >= 0) {
                    if (prio & 0x10000000)
                        a->links[k]->mask = 0;
                    else
                        a->links[k]->mask = 0xffff;
                }
            }
        }
    }
}
static __forceinline void order1(Agent* a, int id, unsigned prio) { set_order_inl(a, 1, id, prio, 1.0f); }
// @ 0x4556d0
void Agent::update_controls(float dt)
{
    g_move_fwd = 0.0f;
    g_move_side = 0.0f;
    turn_rate2 = (1.0f - dt * 10.0f) * turn_rate2;
    turn_rate = (1.0f - dt * 5.0f) * turn_rate;
    float t74 = (1.0f - dt * 4.0f) * (rot[1] - dt * rot[0] * 0.2f) * 0.9f;
    rot[1] = t74;
    rot[0] = t74 + rot[0];
    if ((anim[1].events & 0x1ffff) == 0)
        order1(this, -1, 0);
    vec2 inp;
    if ((int)(anim[0].events & 0x1ffff) >= 0x40) {
        inp.x = 0.0f;
        inp.y = 0.0f;
        vel_xy = inp;
        return;
    }
    float speed_mul = 1.0f;
    if (flags3c & 1) {
        if ((float)(g_ticks[t40.clock] - t40.start) * 1e-05f >= 0.0f) {
            // ---- terrain / night speed multiplier ------------------------------------------------------------------
            Mission* ms = g_mission;
            if (this->kind == 0) {
                if (pos.z < ms->base_z) {
                    float lo = 0.4f;
                    float v = 1.0f - (ms->base_z - pos.z) * 0.5f;
                    speed_mul = fmax_ref(&lo, &v);
                } else if (ms->precip_mode == 1) {
                    speed_mul = 0.9f;
                }
            } else {
                if (pos.z < ms->base_z) {
                    float lo = 0.3f;
                    float v = 1.0f - (ms->base_z - pos.z) * 0.6f;
                    speed_mul = fmax_ref(&lo, &v);
                } else {
                    if (ms->precip_mode == 1)
                        speed_mul = 0.9f;
                    if (g_game->is_night())
                        speed_mul = speed_mul * 0.9f;
                }
            }
            vector4 up(0.0f, 0.0f, 1.0f);
            if (floor_face)
                up = floor_face->normal;

            // ---- input bits -> the two input axes ------------------------------------------------------------------
            unsigned fl = in_now.bits;
            float side = 0.0f;
            inp.y = 0.0f;
            if (fl & 1) {
                inp.y = 1.0f;
                if (fl & 0x200000)
                    g_move_fwd = 1.0f;
                else
                    g_move_fwd = 0.6f;
            } else if (fl & 2) {
                inp.y = -1.0f;
                g_move_fwd = -0.7f;
                if (!(fl & 0x200000))
                    g_move_fwd = -0.5f;
            }
            if (in_now.bits & 4) {
                side = 1.0f;
                g_move_fwd = g_move_fwd * 0.7f;
                if (in_now.bits & 0x200000)
                    g_move_side = 0.9f;
                else
                    g_move_side = 0.55f;
            } else if (in_now.bits & 8) {
                side = -1.0f;
                g_move_fwd = g_move_fwd * 0.7f;
                if (in_now.bits & 0x200000)
                    g_move_side = -0.9f;
                else
                    g_move_side = -0.55f;
            }
            float len2 = side * side + inp.y * inp.y;
            if (len2 > 0.0f) {
                float inv = 1.0f / (float)sqrt(len2);
                float ms = inv * side;
                float mf = inv * inp.y;
                if (mf < -0.1f) {
                    ms = ms * 0.7f;
                    mf = mf * 0.7f;
                    if (!(in_now.bits & 0x200000)) {
                        ms = ms * 0.7f;
                        mf = mf * 0.7f;
                    }
                } else if (!(in_now.bits & 0x200000)) {
                    ms = ms * 0.6f;
                    mf = mf * 0.6f;
                }
                g_move_fwd = mf;
                g_move_side = ms;
            }

            int mode = 0;
            if (this->kind == 0) {
                // ---- on foot: weapon mode, walk animation ----------------------------------------------------------
                {
                    ItemRef a = slot_pair(0);
                    ItemRef b = slot_pair(1);
                    if (a.item >= 0) {
                        ItemKind* ik = &g_items[a.item];
                        mode = 1;
                        unsigned fl;
                        if ((b.item == -1 && (ik->type_flags & 0xff) == 4) || (fl = ik->type_flags, (fl & 0xff) == 9)) {
                            mode = 2;
                        } else if (ik->is_melee() && (fl & 0x200000)) {
                            mode = 3;
                        }
                    }
                }
                if ((anim[0].events & 0x1ffff) == 0 && horse == -1) {
                    int anim;
                    switch (mode) {
                    case 1:
                        anim = 5;
                        break;
                    case 2:
                        { ItemKind* ik; if (slot_pair(0).item < 0 || (ik = &g_items[slot_pair(0).item], (ik->type_flags & 0xff) != 9))
                            anim = 7;
                        else
                            anim = 8; }
                        break;
                    case 3:
                        anim = 6;
                        break;
                    default:
                        if (!(ai.alarm & 1) && (pool_id != (unsigned)g_mission->player_id ||
                                (!a_bit(g_mission_templates[g_mission->mode].flags, 1) && !(g_mission->flags & 1))))
                            anim = stand_animation;
                        else
                            anim = 4;
                        break;
                    }
                    set_order(0, anim, 0, 1.0f);
                }
                float divisor = 1.0f;
                a_v2 mv(g_move_side, g_move_fwd);
                float slope = 1.0f;
                {
                    vector4 v(g_move_side, g_move_fwd, 0.0f);
                    v.normalize();
                    v = ((const ::pos&)body->local).rot(v);
                    vector4 c = cross(v, up);
                    v = cross(up, c);
                    v.normalize();
                    if (v.z > 0.01f)
                        slope = 1.0f - v.z * 0.5f;
                }
                if (v98.z > 0.0001f) {
                    slope = (1.0f - v98.z * 0.2f) * slope;
                } else if (v98.z < -0.0001f) {
                    float lo = 0.0f;
                    float vv = v98.z * 0.5f + 0.7f;
                    slope = fmax_ref(&lo, &vv) * slope;
                }
                float mag = (float)sqrt(g_move_fwd * g_move_fwd + g_move_side * g_move_side);
                if (mag > 0.0f) {
                    mv.x = g_move_side * (1.0f / mag);
                    mv.y = (1.0f / mag) * g_move_fwd;
                }
                float lim = stamina_scale() * slope;
                if (mag > speed_mul)
                    mag = speed_mul;
                if (mag < speed_mul * -0.8f)
                    mag = speed_mul * -0.8f;
                if (mag > lim)
                    mag = lim;
                if (mag < lim * -0.8f)
                    mag = lim * -0.8f;
                int off = troop;
                const int& agi = g_game->chars[off].attributes[1];
                float skill = ((60.0f / (equip_value + 60.0f)) * (g_game->chars[off].nibble(0x19) * 5.0f + agi) + 100.0f) * 0.01f;
                float nvx, nvy;
                {
                    a_v2 dv = mv * mag * 2.8f * skill;
                    dv.x = dv.x * 0.95f;
                    if (dv.y < 0.0f)
                        dv.y = dv.y * 0.8f;
                    dv.x = dv.x - vel_xy.x;
                    dv.y = dv.y - vel_xy.y;
                    const int& agi2 = g_game->chars[off].attributes[1];
                    float step = (((70.0f / (equip_value + 70.0f)) * ((25.0f / (equip_value + 25.0f)) * (((float)g_game->chars[off].nibble(0x19) * 0.2f) + ((float)agi2 * 0.06666667f)) + 1.0f)) * dt) * 10.0f;
                    float lo_ = -step;
                    nvx = a_clampr(dv.x, lo_, step) + vel_xy.x;
                    vel_xy.x = nvx;
                    nvy = a_clampr(dv.y, lo_, step) + vel_xy.y;
                    vel_xy.y = nvy;
                }
                unsigned band = gait & 7;
                float spd = a_len2(vel_xy);
                for (;;) {
                    if (spd > g_gait_walk[band].hi)
                        band++;
                    else if (spd < g_gait_walk[band].lo)
                        band--;
                    else
                        break;
                }
                if (band == 2 && nvy < -0.1f)
                    band = 1;
                if (band == 2) {
                    int ath3 = g_game->chars[off].nibble(0x19);
                    divisor = ((float)ath3 + 30.0f) * 0.07f;
                    if (fabs(nvy) * 0.5f > fabs(nvx)) {
                        if (nvy > 0.0f)
                            set_order(0, PICK(0x13, 0x14, 0x12, 0x11), 0x100000, 1.0f);
                        else
                            set_order(0, PICK(0x1f, 0x20, 0x1e, 0x1d), 0x100000, 1.0f);
                    } else {
                        float h = (float)fabs(nvx) * 0.5f;
                        if (nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x17, 0x18, 0x16, 0x15), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x1b, 0x1c, 0x1a, 0x19), 0x100000, 1.0f);
                        } else if (-nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x38, 0x37, 0x36, 0x35), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x34, 0x33, 0x32, 0x31), 0x100000, 1.0f);
                        } else if (nvx > 0.0f) {
                            set_order(0, PICK(0x3c, 0x3b, 0x3a, 0x39), 0x100000, 1.0f);
                        } else {
                            set_order(0, PICK(0x40, 0x3f, 0x3e, 0x3d), 0x100000, 1.0f);
                        }
                    }
                } else if (band == 1) {
                    divisor = 1.5f;
                    if (fabs(nvy) * 0.5f > fabs(nvx)) {
                        if (nvy > 0.0f)
                            set_order(0, PICK(0xb, 0xc, 10, walk_forward_animation), 0x100000, 1.0f);
                        else
                            set_order(0, PICK(0xf, 0x10, 0xe, 0xd), 0x100000, 1.0f);
                    } else {
                        float h = (float)fabs(nvx) * 0.5f;
                        if (nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x2c, 0x2b, 0x2a, 0x29), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x30, 0x2f, 0x2e, 0x2d), 0x100000, 1.0f);
                        } else if (-nvy > h) {
                            if (nvx > 0.0f)
                                set_order(0, PICK(0x38, 0x37, 0x36, 0x35), 0x100000, 1.0f);
                            else
                                set_order(0, PICK(0x34, 0x33, 0x32, 0x31), 0x100000, 1.0f);
                        } else if (nvx > 0.0f) {
                            set_order(0, PICK(0x24, 0x23, 0x22, 0x21), 0x100000, 1.0f);
                        } else {
                            set_order(0, PICK(0x28, 0x27, 0x26, 0x25), 0x100000, 1.0f);
                        }
                    }
                }
                gait_ratio = spd / divisor;
                if (gait_ratio > 0.001f)
                    anim[0].rate_scale = 1.0f / gait_ratio;
                else
                    anim[0].rate_scale = 1.0f;
                gait = (gait & 0xfffffff8) | band;
            }
            // ---- mount: rider skill, acceleration, gait band ------------------------------------------------------
            int rid = rider;
            int rskill = 5;
            float rmul = 1.0f;
            if (rid != -1) {
                Agent* pg = g_mission->agents.pages[(unsigned)rid >> 4];
                int rt = pg[rid & 0xf].troop;
                rskill = g_game->chars[rt].nibble(0x18);
                rmul = ((float)rskill + 25.0f) * 0.037037037f;
            }
            float sgn = 0.0f;
            if (this->kind == 1) {
                unsigned fl2 = in_now.bits;
                if (fl2 & 0x40) {
                    sgn = -1.0f;
                } else if (fl2 & 0x80) {
                    sgn = 1.0f;
                }
                if (rid >= 0) {
                    Mission* m7 = g_mission;
                    if (m7->agents.get(rid)->f100_is_0x48_and_charged()) {
                        if ((int)pool_id == m7->player_id)       // both arms the same: only the two loads are left (0x456503)
                            sgn = sgn * 0.7f;
                        else
                            sgn = sgn * 0.7f;
                    }
                }
                float lim;
                if (vel_xy.y > 4.0f)
                    lim = vel_xy.y;
                else
                    lim = 4.0f;
                int s1 = item.horse_maneuver() + 1;
                float acc = ((((float)s1 / lim) * sgn) * rmul) * dt + turn_rate;
                turn_rate = acc;
                rot[1] = rot[1] - ((acc * vel_xy.y) * dt) * 0.007f;
                vel.rotate_xy(acc * dt);
                if (in_now.bits & 0x10) {
                    if (this->kind == 1) {
                        unsigned bb = gait;
                        if ((bb & 0x300) == 0) {
                            gait = bb | 0x100;
                            int s2 = item.horse_maneuver() - 1;
                            vel_xy.y = ((float)s2 * rmul) * 0.04f;
                        } else if (bb & 0x100) {
                            vel_xy.y = (((18.0f - vel_xy.y) * ((float)item.horse_maneuver() * ((float)rskill + 3.0f))) * dt) * 0.0006f + vel_xy.y;
                        } else if (bb & 0x200) {
                            float nf = dt + vel_xy.y;
                            vel_xy.y = nf;
                            if (nf > -0.9f)
                                vel_xy.y = 0.0f;
                        }
                    }
                } else if (in_now.bits & 0x20) {
                    if (this->kind != 0)
                        order1(this, 0x1a1, 0);
                    unsigned bb = gait;
                    if ((bb & 0x300) == 0) {
                        bb = bb | 0x200;
                        gait = bb;
                        vel_xy.y = -0.9f;
                    } else if (bb & 0x200) {
                        vel_xy.y = vel_xy.y - dt * 0.5f;
                    } else if (bb & 0x100) {
                        int s2 = item.horse_maneuver();
                        float nf = vel_xy.y - ((float)s2 * dt) * 0.08f;
                        vel_xy.y = nf;
                        if (nf < 0.9f)
                            vel_xy.y = 0.0f;
                    }
                } else {
                    float nf = (1.0f - dt * 0.01f) * vel_xy.y;
                    vel_xy.y = nf;
                    if (fabs(nf) < 0.9f)
                        vel_xy.y = 0.0f;
                    if (vel_xy.y == 0.0f)
                        gait = gait & 0xfffffcff;
                }
                vel.normalize();
                int s3 = item.horse_speed() + 1;
                vector4* fr = &body->local.f;
                float cap = ((float)rskill * 0.02f + 1.0f) * ((float)s3 * speed_mul * 0.19f);
                vector4 v;
                v.x = vel_xy.y * fr->x;
                v.y = fr->y * vel_xy.y;
                v.z = fr->z * vel_xy.y;
                vector4 c = cross(v, up);
                v = cross(up, c);
                v.normalize();
                float lim2;
                if (v.z > 0.01f)
                    lim2 = (1.0f - v.z * 0.9f) * cap;
                else if (v.z < -0.27f)
                    lim2 = (1.0f - (-0.27f - v.z) * 1.3f) * cap;
                else
                    lim2 = cap;
                if (vel_xy.y < -1.0f)
                    vel_xy.y = -1.0f;
                else if (lim2 < vel_xy.y)
                    vel_xy.y = lim2;
                unsigned band = gait & 7;
                float div2 = 1.0f;
                if (vel_xy.y > g_gait_mount[band].hi)
                    band++;
                else if (vel_xy.y < g_gait_mount[band].lo)
                    band--;
                if ((int)band < 0)
                    band = 0;
                else if ((int)band > 5)
                    band = 5;
                if (band == 5) {
                    set_order(0, 0x19a, 0x100000, 1.0f);
                    div2 = 5.0f;
                    if (rider == g_mission->player_id) {
                        if (t5810.elapsed() > 1.2f)
                            restart_inl(t5810);
                    }
                } else if (band == 4) {
                    set_order(0, 0x199, 0x100000, 1.0f);
                    div2 = 3.2f;
                } else if (band == 3) {
                    set_order(0, 0x198, 0x100000, 1.0f);
                    div2 = 2.0f;
                } else if (band == 2) {
                    set_order(0, 0x197, 0x100000, 1.0f);
                    div2 = 1.7f;
                } else if (band == 0) {
                    set_order(0, 0x19b, 0, 1.0f);
                    div2 = 1.0f;
                } else {
                    set_order(0, 0x196, 0, 1.0f);
                    if (t5810.elapsed() > 1.2f) {
                        if (t5810.elapsed() > 30.2f || frand() < 0.05f) {
                            float var = 0.0f;
                            int kind = 0x6c;
                            play_sound_var(&kind, &var);
                        }
                        restart_inl(t5810);
                    }
                }
                float r = vel_xy.y / div2;
                gait_ratio = r;
                if (r != 0.0f)
                    anim[0].rate_scale = 1.0f / r;
                else
                    anim[0].rate_scale = 1.0f;
                gait = (gait & 0xfffffff8) | band;
                if (turn_rate < -0.1f) {
                    if (band == 1)
                        set_order(0, 0x19f, 0, 1.0f);
                    order1(this, 0x19f, 0);
                } else if (turn_rate > 0.1f) {
                    if (band == 1)
                        set_order(0, 0x1a0, 0, 1.0f);
                    order1(this, 0x1a0, 0);
                }
            }
            // ---- jump / rear-up ------------------------------------------------------------------------------------
            if ((in_now.bits & 0x100000) && t5800.elapsed() >= 0.0f) {
                if (this->kind == 0) {
                    if (horse == -1) {
                        flags3c = flags3c & 0xfffffffe;
                        a_vadd(impulse, vector4(0.0f, 0.0f, 5.0f));
                        set_order(0, 2, 0x1000002, 1.0f);
                        if (pos.z < g_mission->base_z) {
                            float var = 0.1f;
                            int kind = 0x1f;
                            play_sound_var(&kind, &var);
                        } else {
                            float var = 0.1f;
                            int kind = 0x1d;
                            play_sound_var(&kind, &var);
                        }
                        ((Timer*)&t5800)->set_delay(-2.0f);
                    }
                } else if (vel_xy.y > 3.5f) {
                    flags3c = flags3c & 0xfffffffe;
                    a_vadd(impulse, vector4(0.0f, 0.0f, 6.0f));
                    set_order(0, 0x19d, 0x1000002, 1.0f);
                    if (pos.z < g_mission->base_z) {
                        float var = 0.1f;
                        int kind = 0x23;
                        play_sound_var(&kind, &var);
                    } else {
                        float var = 0.1f;
                        int kind = 0x21;
                        play_sound_var(&kind, &var);
                    }
                    ((Timer*)&t5800)->set_delay(-4.0f);
                }
            }
            if ((in_now.bits & 0x40000) && this->kind != 0) {
                int kind = 4;
                play_hit_sound(&kind);
                set_order(0, 0x19c, 0x100004a, 1.0f);
            }
        }
    }
    // ---- heading ------------------------------------------------------------------------------------------------
    if ((int)pool_id == g_mission->player_id && horse == -1) {
        if (in_now.f4 != 0) {
            vel = in_now.move;
        } else {
            if (active != 0 || f5610 != 0 || (in_now.bits & 0xffdfffff) != 0)
                steer(dt);
        }
    } else if (this->kind != 1 && horse == -1) {
        steer(dt);
    }
    // ---- riding: mount reactions, mount / dismount --------------------------------------------------------------
    unsigned mid = horse;
    if ((int)mid >= 0) {
        Agent* h = agent_by_id(mid);
        switch (h->anim[0].cur) {
        case 0x197: set_order(0, 0x42, 0, 1.0f); break;
        case 0x198: set_order(0, 0x45, 0, 1.0f); break;
        case 0x199: set_order(0, 0x46, 0, 1.0f); break;
        case 0x19a: set_order(0, 0x47, 0, 1.0f); break;
        case 0x19c: set_order(0, 0x49, 0x1000040, 1.0f); break;
        case 0x19d: set_order(0, 0x4b, 0, 1.0f); break;
        case 0x19e: set_order(0, 0x4c, 0, 1.0f); break;
        case 0x1a0: set_order(0, 0x4e, 0, 1.0f); break;
        case 0x19f: set_order(0, 0x4d, 0, 1.0f); break;
        default:
            aset_inl(anim[0], 0x41, 0);
            break;
        }
        if (weapon_flags & 8) {
            if (agent_by_id(horse)->vel_xy.y > 5.5f) {
                order1(this, 0x48, 0);
            } else if (agent_by_id(horse)->anim[0].cur == 0x196) {
                order1(this, 0x51, 0);
            } else {
                order1(this, 0x43, 0);
            }
        }
    }
    if ((in_now.bits & 0x80000) && (gait & 0x100) && this->kind == 0 && horse >= 0 &&
        g_mission->agents.get(horse)->vel_xy.y < 5.2f)
        set_order(0, 0x4a, 0x1000002, 1.0f);
    unsigned fl = in_now.bits;
    if ((fl & 0x10000) && horse >= 0 && mount_timer.active == 0) {
        ((ActiveTimer*)&mount_timer)->begin(2.0f);
        set_order(0, 0x50, 0x1000040, 1.0f);
        order1(this, -1, 0x1f);
        active = 0;
        f5610 = 0;
        agent_by_id(horse)->rider = -1;
        agent_by_id(horse)->ai.simple_behavior = 0;
        agent_by_id(horse)->ai.ta_interval = 10.0f;     // +0x2c0 as a float (0x457367: 0x41200000)
        restart_inl(agent_by_id(horse)->ai.ta);
        horse = -1;
        draw_name();
    } else if (!(fl & 0x20000) || horse != -1) {
        if ((fl >> 24) && (fl & 0xff000000) == 0x1000000 && this->kind == 0 && anim[1].cur != 0x127 && mount_attempts < 3) {
            order1(this, 0x127, 0x1000040);
            mount_attempts++;
            int kind = 6;
            play_class_sound(&kind);
        }
    } else {
        unsigned tg;
        if (f7c == 2)
            tg = ai.target;
        else
            tg = find_talk_target();
        if ((int)tg >= 0 && agent_by_id(tg)->rider == -1) {
            g_mission->agents.get(tg)->f5830 = 2;
            g_mission->agents.get(tg)->rider = pool_id;
            horse = tg;
            set_order(0, 0x4f, 0x1000040, 1.0f);
            order1(this, -1, 0x1f);
            active = 0;
            f5610 = 0;
            draw_name();
            if (wield[0] >= 0 && pairs[wield[0]].item >= 0 && (g_items[pairs[wield[0]].item].type_flags & 0x10000000)) {
                wield[0] = -1;
                on_wield_reset();
            }
            if (wield[1] >= 0 && pairs[wield[1]].item >= 0) {
                unsigned tf = g_items[pairs[wield[1]].item].type_flags;
                if ((tf & 0xff) == 7 && (tf & 0x10000000)) {
                    wield[1] = -1;
                    on_wield_reset();
                    order1(this, 0x6d, 0x5000046);
                }
            }
        }
    }
    ((ActiveTimer*)&mount_timer)->expired();
}

// ---- Agent::process_blow: the handler of a pending blow ---------------------------------------------------------------------------









#include "engine/fx/anim_set.h"   // AnimSet g_anim_set (0xa76628): .data is the array of 0x30-byte animation definitions
#define itp_wooden_attack 0x00020000
#define itcf_throw_stone 0x0000000000010000i64
#define tf_allways_fall_dead 0x80
#define imod_lame 30
#define PAGE_AGENT(m, i) ((m)->agents.pages[(unsigned)(i) >> 4] + ((i) & 0xf))
// 0x43a280 (the register-convention helper: type in EAX, destination buffer in ESI)
#define PB_IMPULSE() if (flags3c & 1) { \
        flags3c &= ~1u; \
        ((Timer*)&t40)->set_delay(-0.6f); \
        impulse.x = imp.x + impulse.x; \
        impulse.y = imp.y + impulse.y; \
        impulse.z = imp.z + impulse.z; \
        if ((int)blow.attacker >= 0 && g_mission->agents.get(blow.attacker)->kind == 1) { \
            impulse.x = impulse.x; \
            impulse.y = impulse.y; \
            impulse.z = impulse.z + 1.0f; \
        } \
    }
// 0x407c80 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
namespace rgl {
inline __declspec(noinline) float vector4::out_length_squared() const { return length_squared(); }      // 0x407c80
}

// 0x435130 (game/agent/agent_queries.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) bool Troop::is_hero()      // 0x435130
{
    if (flags30 & 0x10) return true;
    return false;
}
