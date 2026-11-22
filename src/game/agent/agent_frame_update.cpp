//-----------------------------------------------------------------------------
// File: agent_frame_update.cpp  (0x45c220..0x460160)
//
// Desc: update_attack, move_step, update, tick
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
inline __declspec(noinline) bool ItemKind::is_ranged()
{
    unsigned t = type_flags & 0xff;
    return t == itp_type_bow || t == itp_type_crossbow || t == itp_type_thrown || t == itp_type_pistol || t == itp_type_musket;
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
inline __declspec(noinline) int box_overlap3(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return 1;
    return 0;
}
inline __declspec(noinline) int box_overlap2(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y)
        return 1;
    return 0;
}
inline __declspec(noinline) void rgl::cell::collect(const vector4* bmin, const vector4* bmax, int* use3d, unsigned mask, shape_base** out, int* count)
{
    shape_base* buf[0x2000];
    int n = 0;
    unsigned i = 0;                 // lives in memory in the original (reloaded every iteration)
    int k = 0;
    while (i < a.size()) {
        shape_base* sh = a[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];                  // both query temporaries live in one block (min below max)
        const vector4* smax = sh->get_max(&tv[1]);
        const vector4* smin = sh->get_min(&tv[0]);
        char hit;
        if (*use3d == 0)
            hit = box_overlap2(bmin, bmax, smin, smax);
        else
            hit = box_overlap3(bmin, bmax, smin, smax);
        if (!hit) {
            i++;
            continue;
        }
        if (k < *count) {
            shape_base* o = out[k];
            shape_base* cur = a[i];
            if (cur >= o) {
                k++;
                if (cur == o) {
                    i++;
                    buf[n++] = cur;
                } else {
                    buf[n++] = o;
                }
                continue;
            }
        }
        shape_base* cur2 = a[i];
        i++;
        buf[n++] = cur2;
    }
    if (k < *count) {
        int m = *count - k;
        memcpy(&buf[n], &out[k], m * sizeof(shape_base*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}
inline __declspec(noinline) void rgl::world::query_box(const vector4* bmin, const vector4* bmax, unsigned mask, shape_base** out, int* count)
{
    int lo[2], hi[2];
    to_cell(&bmin->x, lo);
    to_cell(&bmax->x, hi);
    *count = 0;
    int2 p;
    for (p.x = lo[0]; p.x <= hi[0]; p.x++) {
        if (lo[1] <= hi[1]) {
            int flat = 1;
            for (p.y = lo[1]; p.y <= hi[1]; p.y++)
                cells[p.x][p.y].collect(bmin, bmax, &flat, mask, out, count);
        }
    }
}
// @ 0x43b1f0
inline Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}
// Which of the four attack directions (1 left, 2 right, 3 up?, 0 thrust) the agent picks; derived from the
// mouse movement (mode > 0) or from where the target stands relative to the view (mode 0).
// rgl::matrix::transform_point_to_local (0x407f70, src/engine/physics/collision_shapes.cpp) with its body visible here (inline noinline
// duplicate, as map_panel.cpp): its argument does not escape
inline __declspec(noinline) vector4 rgl::matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
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
// The original agent translation unit (0x4353a0..0x45fxxx) saw these bodies: a local whose address reaches only them does not
// escape, so it shares stack slots (codegen 7). game/agent/agent_queries.cpp and game/agent/agent_stats.cpp own the rows.
inline __declspec(noinline) int ItemRef::max_amount() const     // 0x434b70
{
    if (item < 0) return 0;
    int v = g_items[item].max_ammo;
    if (v != 0 && (m4 & 0xff000000) == 0x2a000000) {
        float f = v * 0.13f;
        int d = round_float(&f);
        if (d < 1) d = 1;
        v += d;
    }
    return v;
}
inline __declspec(noinline) void ItemRef::add_amount(int n)     // 0x434be0
{
    int v = (m4 & 0xff) + n;
    if (v > max_amount()) {
        v = max_amount();
        b.amount = 0;
        m4 |= v;
        return;
    }
    b.amount = 0;
    m4 |= v;
}
inline __declspec(noinline) ItemRef Agent::best_ammo(int idx, int* missing, int* total, int* slot_out)      // 0x436880
{
    ItemRef r;
    *slot_out = -1;
    r.item = -1;
    if (idx == -1) idx = slots[0];
    *missing = -1;
    *total = -1;
    if (idx >= 0 && pairs[idx].item >= 0) {
        unsigned wt = g_items[pairs[idx].item].type_flags & 0xff;
        if (wt == 8 || wt == 9 || wt == 10 || wt == 0x11 || wt == 0x10) {
            *missing = 0;
            *total = 0;
            for (int i = 0; i < 4; i++) {
                ItemRef* cur = &pairs[i];
                if (cur->item >= 0) {
                    int w = pairs[idx].item;
                    unsigned t = g_items[w].type_flags & 0xff;
                    if ((t == 8 && (char)g_items[cur->item].type_flags == 5) ||
                        (t == 9 && (char)g_items[cur->item].type_flags == 6) ||
                        (t == 0x10 && (char)g_items[cur->item].type_flags == 0x12) ||
                        (t == 0x11 && (char)g_items[cur->item].type_flags == 0x12) ||
                        (t == 10 && cur->item == w)) {
                        if (*slot_out == -1) {
                            r = *cur;
                            *slot_out = i;
                        }
                        if (*missing == 0) {
                            unsigned b = cur->m4;
                            if (cur->max_amount() != (int)(b & 0xff)) {
                                r = *cur;
                                *slot_out = i;
                            }
                        }
                        unsigned b2 = cur->m4;
                        *missing += cur->max_amount() - (b2 & 0xff);
                        *total += cur->max_amount();
                    }
                }
            }
        }
    }
    return r;
}
inline __declspec(noinline) void rgl::mesh_list::cap_all(const float* limit)     // 0x438f90
{
    for (std::vector<rgl::mesh*>::iterator it = begin(); it < end(); ++it) {
        rgl::mesh* n = *it;
        keyframes* l = n->local_points ? &n->kf : &n->parent->get_kf();
        if (0 < l->count) {
            if (*limit > (float)l->times[l->count - 1])
                n->anim_time = (float)l->times[l->count - 1];
            else
                n->anim_time = *limit;
        }
    }
}
inline __declspec(noinline) void rgl::lod_mesh::cap_all(const float* limit)   // 0x439000
{
    for (int i = 0; i < count; ++i)
        lists[i].cap_all(limit);
}
inline __declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)     // 0x40e3d0
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
float frand();                                                          // 0x40e400
struct a_v2 { float x, y; a_v2() {} a_v2(const float& a, const float& b) : x(a), y(b) {} a_v2 operator*(const float& k) const { return a_v2(k * x, k * y); } void operator*=(const float& k) { x *= k; y *= k; } };
static __forceinline void a_dec(float* v, int j, const float& dt) { v[j] = v[j] - (v[j] + 1.0f) * dt; }
static __forceinline void a_add2(float* o, const a_v2& u) { o[0] = u.x + o[0]; o[1] = u.y + o[1]; }
static inline float a_len2(const vec2& v) { return (float)sqrt(v.x * v.x + v.y * v.y); }
using rgl::mesh;
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
// rgl::matrix::transform_point (0x408460, src/engine/physics/collision_shapes.cpp) with its body visible from here on (inline noinline
// duplicate): its hidden return temporary does not escape, so get_label_box's third result shares p2's dead slot (frame 0x20)
inline __declspec(noinline) vector4 rgl::matrix::transform_point(const vector4& p) const
{
    return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                   s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                   s.z * p.x + f.z * p.y + u.z * p.z + o.z);
}
inline __declspec(noinline) capsule::capsule(float r, const vector4* a, const vector4* b) : shape_base(inline_init())
{
    radius = r;
    l0 = *a;
    l1 = *b;
    p0 = l0;
    p1 = l1;
}
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
// ---- Agent::process_blow: the handler of a pending blow ---------------------------------------------------------------------------









#include "engine/fx/anim_set.h"   // AnimSet g_anim_set (0xa76628): .data is the array of 0x30-byte animation definitions
#define itcf_throw_stone 0x0000000000010000i64
// 0x407c80 (engine/physics/collision_shapes.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
namespace rgl {
inline __declspec(noinline) float vector4::out_length_squared() const { return length_squared(); }      // 0x407c80
}


// ---- Agent::update_attack: the per-tick attack / reload state machine -------------------------------------------------------------
#define itp_cant_reload_on_horseback 0x00100000
#define itcf_throw_knife   0x0000000000020000i64
#define itcf_throw_axe     0x0000000000030000i64
#define itcf_throw_javelin 0x0000000000040000i64
#define itcf_shoot_musket  0x0000000000080000i64
#define itcf_shoot_mask    0x00000000000ff000i64
#define itcf_reload_pistol 0x0000007000000000i64
#define itcf_reload_musket 0x0000008000000000i64
#define itcf_reload_mask   0x000000f000000000i64
int __cdecl ftoi_helper(int x);                     // 0x435440
static __forceinline ItemRef* cur_ptr(Agent* a) { ItemRef* p = 0; if (a->wield[0] >= 0) p = &a->pairs[a->wield[0]]; return p; }
#define ACT anim[1].cur          // +0x100 current animation id
#define NEXTACT anim[1].next     // +0x104
#define PRIO1 anim[1].events     // +0x114
#define PROG anim[1].scale       // +0x118 animation progress
#define TYPE (ik->type_flags & 0xff)
static __forceinline void log_timed(const char* s) { tstring msg(s); g_mission->log_message(g_strtable.lookup(msg), 20.0f); }
#define LOG_TIMED(str) log_timed(str);
// @ 0x45c220
void Agent::update_attack(float dt)
{
    int slot;
    Blow hit;
    hit.init();
    if (((int)(anim[0].events & 0x1ffff) >= 0x20 && anim[0].cur >= 0 && (defs->data[anim[0].cur].flags & 0x400)) ||
        (int)(PRIO1 & 0x1ffff) >= 0x40) {
        active = 0;
        f5610 = 0;
        return;
    }
    ItemRef cur = slot_pair_fi(0);
    FxDef* sk = body->fx;
    if (!sk->pose_valid) {
        if (sk->pose_src)
            sk->update_poses();
        else
            sk->update_poses2();
    }

    fr1 = body->local * sk->elems[19].m84;
    if (this->kind != 0)
        goto done;
    {
    int ci = cur.item;
    if (ci < 0) ci = 0;
    ItemKind* ik = &g_items[ci];
    unsigned t0 = TYPE;
    if (t0 == itp_type_one_handed_wpn || t0 == itp_type_two_handed_wpn || t0 == itp_type_polearm || cur.item < 0) {
        // ------------------------------------------------------------------ melee / bare hands
        if (ACT == -1)
            active = 0;
        int S;
        if (active == 0 && f5610 == 0) {
            float t = dt + charge_ramp;
            charge_ramp = t;
            if (t > 1.0f)
                charge_ramp = 1.0f;
            if (in_now.bits & 0x100) {
                bool found = false;
                int i = 0;
                do {
                    if (i >= 10) break;
                    start_shoot(0);
                    if (snd_a != -1) {
                        found = true;
                        active = 1;
                        set_order(1, snd_a, 0x0240000a, damage_factor(0));
                        this->cur = 0.0f;
                        f5610 = 0;
                        classify(0);
                        charge = charge_ramp;
                    }
                    i++;
                } while (!found);
            }
        } else if (active == 1) {
            if (PROG > 0.99f) {
                float u = dt * 0.8f + charge;
                charge = u;
                if (u < 1.0f)
                    charge = dt * 0.6f + u;
                if (charge > 1.3f)
                    charge = 1.3f;
            }
            if (ACT == snd_a && !(in_now.bits & 0x200)) {
                if (pool_id == g_mission->player_id) {
                    if (in_now.bits & 0x100) {
                        if (PROG < 0.3f && g_options.ctl_mode > 0) {
                            if (pick_attack_dir(0) != mode1) {
                                start_shoot(0);
                                if (snd_a != -1) {
                                    set_order(1, snd_a, 0x0240000a, damage_factor(0));
                                    classify(0);
                                }
                            }
                            goto melee_tail;
                        }
                        goto melee_hold;
                    }
                } else {
                melee_hold:
                    if (in_now.bits & 0x100)
                        goto melee_tail;
                }
                if (PROG > 0.99f) {
                    if (snd_b == -1) {
                        active = 0;
                    } else {
                        set_order(1, snd_b, 0x0140000a, damage_factor(0));
                        active = 2;
                        { int kind = 3; play_hit_sound(&kind); }
                        if (horse == -1 && frand() < (charge - 1.0f) * 3.5f) {
                            if (pool_id == g_mission->player_id || frand() < 0.3f) {
                                int kind = 2;
                                play_class_sound(&kind);
                            } else {
                                int kind = 3;
                                play_class_sound(&kind);
                            }
                        }
                        fr1.o.x = fr2.o.x = -20000000.0f;      // 0xcb989680 (0x45d14d)
                        hit_once = 0;
                        prop_done = 0;
                        classify(0);
                    }
                }
            } else {
                charge_ramp = 0.8f;
                active = 6;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
        }
    melee_tail:
        S = active;
        if ((S == 2 && ACT == NEXTACT && ACT == snd_b && (mode1 == 0 || PROG > 0.1f)) || f100_is_0x48_and_charged()) {
            if (f100_is_0x48_and_charged()) {
                mode1 = 0;
                snd_c = 0x44;
                if (frand() < 0.05f) {
                    hit_once = 0;
                    prop_done = 0;
                }
            }
            if (in_now.bits & 0x200) {
                active = 6;
                charge_ramp = 0.8f;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
            if (!f100_is_0x48_and_charged() && (!(PROG < 0.85f) || (mode1 != 0 && !(PROG > 0.2f))))
                goto chk99;
            if (f100_is_0x48_and_charged())
                charge = 1.0f;
            int r = swing_trace(0, 0, &hit, &charge);
            if (r != 0 && f100_is_0x48_and_charged())
                order1(this, 0x44, 0x0100007d);
            if (r >= 2) {
                set_order(1, snd_c, 0x1400023, damage_factor(0));
                active = 5;
            }
            if ((in_now.bits & 0x100) && pool_id != g_mission->player_id) {
                active = 6;
                charge_ramp = 0.8f;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
            if (r == 1) {
                if (hit.dmg_out < 5.0f && cur.item >= 0) {
                    set_order(r, snd_c, 0x1400023, damage_factor(0));
                    active = 5;
                } else {
                    active = 3;
                }
                goto tail;
            }
            if (r == 2)
                goto clear_state;
            goto chk99;
        } else {
            if (S == 3)
                goto chk99;
            if (S != 5 && S != 6)
                goto tail;
            if (ACT == -1)
                goto clear_state;
        chk99:
            if (!(PROG > 0.99f))
                goto tail;
        clear_state:
            active = 0;
        }
    } else if (ik->is_ranged()) {
        // ------------------------------------------------------------------ bow / crossbow / thrown / guns
        int missing, total;
        slot = 0;
        best_ammo(-1, &missing, &total, &slot);
        if (TYPE == itp_type_crossbow) {
            if (cur.max_amount() - (cur.m4 & 0xff) != 0) {
                float var2 = 100.0f;
                links[wield[0]]->cap_all(&var2);
                if (slot >= 0)
                    links[slot]->mask = 0xffff;
            } else {
                float var2 = 0.0f;
                links[wield[0]]->cap_all(&var2);
                if (slot >= 0)
                    links[slot]->mask = 0;
            }
        } else if (TYPE == itp_type_bow && links[wield[0]]) {
            float var2 = 0.0f;
            links[wield[0]]->cap_all(&var2);
        }
        int S = active;
        if (S == 0) {
            if (!(in_now.bits & 0x400))
                goto tail;
            if ((TYPE == itp_type_crossbow || TYPE == itp_type_pistol || TYPE == itp_type_musket) &&
                cur.max_amount() - (cur.m4 & 0xff) == 0) {
                if (missing > 0) {
                    reload_anim = -1;
                    if (horse == -1) {
                        reload_anim = 0x7b;
                    } else if (ik->type_flags & itp_cant_reload_on_horseback) {
                        if (pool_id == g_mission->player_id) {
                            tstring msg("ui_cant_reload_this_weapon_on_horseback");
                            g_gui.add(*g_strtable.lookup(msg), -1);
                        }
                    } else {
                        reload_anim = 0x7c;
                    }
                    unsigned __int64 rl = ik->flags & itcf_reload_mask;
                    if (rl != 0) {
                        if (rl == itcf_reload_pistol)
                            reload_anim = 0x87;
                        else if (rl == itcf_reload_musket)
                            reload_anim = 0x8a;
                    }
                    if (reload_anim >= 0) {
                        set_order(1, reload_anim, 0x0140003c, damage_factor(0));
                        update_quiver();
                        active = 2;
                    }
                } else if (pool_id == g_mission->player_id) {
                    if (total > 0) {
                        LOG_TIMED("ui_no_more_bolts")
                    } else {
                        LOG_TIMED("ui_you_are_not_carrying_any_bolts")
                        goto no_ammo;
                    }
                }
            } else if (missing <= 0 && TYPE != itp_type_crossbow && TYPE != itp_type_pistol && TYPE != itp_type_musket) {
                if (pool_id == g_mission->player_id) {
                    if (total > 0) {
                        if (TYPE == itp_type_bow)
                            LOG_TIMED("ui_no_more_arrows")
                    } else {
                        if (TYPE == itp_type_bow)
                            LOG_TIMED("ui_you_are_not_carrying_any_arrows")
                        else
                            LOG_TIMED("ui_you_are_not_carrying_any_bolts")
                    }
                }
            no_ammo:
                goto tail;
            } else {
                hit_once = 0;
                prop_done = 0;
                active = 1;
                timer.restamp();
                start_shoot(0);
                if (snd_a != -1) {
                    set_order(1, snd_a, 0x02400002, damage_factor(0));
                    unsigned __int64 sm = ik->flags & itcf_shoot_mask;
                    if (sm == itcf_throw_axe || sm == itcf_throw_knife || sm == itcf_throw_stone ||
                        sm == itcf_shoot_musket || TYPE == itp_type_thrown || TYPE == itp_type_crossbow) {
                        variant = 4;
                    } else {
                        variant = ((TYPE != itp_type_pistol) - 1) & 5;
                    }
                    this->cur = 0.0f;
                }
            }
        } else if (S == 1) {
            if ((unsigned char)ik->type_flags == itp_type_bow && PROG > 0.3f) {
                if (PROG > 0.46f && links[wield[0]]) {
                    float x = (PROG - 0.46f) * 2.5f;
                    float hi = 1.0f;
                    float lo = 0.0f;
                    float var2 = clamp(&x, &lo, &hi) * 100.0f;
                    links[wield[0]]->cap_all(&var2);
                }
                links[slot]->mask = 0xffff;
                update_quiver();
            }
            if (in_now.bits & 0x200) {
                order1(this, -1, 2);
                if ((unsigned char)ik->type_flags == itp_type_bow && links[slot]) {
                    links[slot]->mask = 0;
                    update_quiver();
                }
                goto clear_state;
            } else if (!(in_now.bits & 0x100) &&
                       (PROG > 0.99f || ((TYPE == itp_type_crossbow || TYPE == itp_type_musket) && PROG > 0.2f))) {
            {
                unsigned rel = 0;
                if (TYPE != itp_type_bow) {
                    if (TYPE == itp_type_thrown) {
                        rel = 0x40001b;
                    } else if (TYPE == itp_type_crossbow || TYPE == itp_type_pistol || TYPE == itp_type_musket) {
                        ItemRef* rp = cur_ptr(this);
                        if (rp)
                            rp->add_amount(1);
                    }
                }
                set_order(1, snd_b, rel | 0x1000002, damage_factor(0));
            }
            if ((unsigned char)ik->type_flags != itp_type_thrown) {
                shoot();
                if ((unsigned char)ik->type_flags == itp_type_bow)
                    top_up_ammo(1);
                if (slot >= 0 && links[slot])
                    links[slot]->mask = 0;
                active = 0;
            } else {
                active = 3;
            }
            if (TYPE == itp_type_bow) {
                if (pool_id == g_mission->player_id) {
                    float var = 0.1f;
                    int kind = 0x25;
                    play_sound_var(&kind, &var);
                } else {
                    float var = 0.1f;
                    int kind = 0x25;
                    play_sound_var(&kind, &var);
                }
            } else if (TYPE == itp_type_crossbow) {
                float var = 0.1f;
                int kind = 0x26;
                play_sound_var(&kind, &var);
            } else if (TYPE == itp_type_thrown) {
                unsigned __int64 sm = ik->flags & itcf_shoot_mask;
                if (sm == itcf_throw_javelin) {
                    int kind = 0x27;
                    float var = 0.1f;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_axe) {
                    float var = 0.1f;
                    int kind = 0x28;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_knife) {
                    float var = 0.1f;
                    int kind = 0x29;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_stone) {
                    int kind = 0x2a;
                    float var = 0.1f;
                    play_sound_var(&kind, &var);
                }
            }
            g_game->event_ctx.agent = pool_id;
            g_game->event_ctx.troop = item.item;
            g_game->event_ctx.slot = wield[0];
            {
                GameData* g = g_game;
                g->event_ctx.key_a = slot_mesh[ftoi_helper(g->event_ctx.slot)];
            }
            g_script.pos[1] = body->local * *body->fx->rec(ik->ammo_skill());
            ik->sound.update(-0x33);
            }
        } else if (S == 2) {
            if (ACT != reload_anim) {
                active = 0;
            } else if ((in_now.bits & 0x130200) ||
                       (ACT == 0x7b && PROG < 0.8f && pool_id == g_mission->player_id && (in_now.bits & 0xf))) {
                order1(this, -1, 0x3d);
                active = 0;
            } else if (PROG > 0.99f) {
                ItemRef* rp = cur_ptr(this);
                if (rp) {
                    unsigned u = rp->m4 & 0xff;
                    int n = top_up_ammo(u);
                    ItemRef* rq = cur_ptr(this);
                    rq->b.amount = 0;
                    rq->m4 |= u - n;
                }
                update_quiver();
                active = 0;
            } else if (links[wield[0]]) {
                float lo2 = 0.4f, hi2 = 0.66f;
                if (horse != -1) {
                    lo2 = 0.16f;
                    hi2 = 0.5f;
                }
                float x = (PROG - lo2) / (hi2 - lo2);
                float hi = 1.0f;
                float lo = 0.0f;
                float var2 = clamp(&x, &lo, &hi) * 100.0f;
                links[wield[0]]->cap_all(&var2);
            }
        } else if (S == 3) {
            if (ACT == 0x84 || ACT == 0x80 || ACT == 0x82 || ACT == 0x7e) {
                if (PROG > 0.25f) {
                    shoot();
                    top_up_ammo(1);
                    active = 4;
                    if (wield[0] >= 0)
                        links[wield[0]]->mask = 0;
                }
            } else {
                active = 0;
            }
        } else if (S == 4) {
            if (ACT == 0x84 || ACT == 0x80 || ACT == 0x82 || ACT == 0x7e) {
                if (!(PROG > 0.98f))
                    goto tail;
                if (wield[0] >= 0)
                    links[wield[0]]->mask = 0xffff;
                update_quiver();
            }
            goto clear_state;
        }
    }
    }
tail:
    if (in_now.bits & 0x200) {
        if (f5610 == 0) {
            pick_sound_ids(&block_anim_a, &block_anim_b, &f561c);
            if (block_anim_a != -1 && (int)(PRIO1 & 0x1ffff) <= 0xf) {
                f5610 = 1;
                active = 0;
                float v = damage_factor(0);
                if (slot_pair(1).item >= 0)
                    v = damage_factor(1);
                set_order(1, block_anim_a, 0x0240000e, v);
                this->cur = 0.0f;
                classify(1);
            }
            goto done;
        }
        if (ACT == block_anim_a || ACT == f561c || ACT == block_anim_b) {
            if (ACT == block_anim_a && PROG < 0.99f) {
                if (PROG < 0.5f && g_options.stick_mode != 0) {
                    int a, b, c;
                    pick_sound_ids(&a, &b, &c);
                    if (a != block_anim_a) {
                        block_anim_b = b;
                        block_anim_a = a;
                        f561c = c;
                        float v = damage_factor(0);
                        if (slot_pair(1).item >= 0)
                            v = damage_factor(1);
                        float p = PROG;
                        set_order(1, block_anim_a, 0x0240000e, v);
                        { AnimSlot& a1 = anim[1]; a1.phase = p * 0.85f; }
                        active = 0;
                        goto done;
                    }
                }
            } else {
                set_order(1, block_anim_b, 0xe, 1.0f);
            }
            active = 0;
        } else {
            order1(this, -1, 0xf);
        }
    } else {
        f5610 = 0;
        if (block_anim_a != -1) {
            if (ACT == block_anim_a || (ACT == f561c && PROG > 0.2f))
                order1(this, -1, 0xf);
        }
    }
done:
    fr2 = fr1;
}

// ---- Agent::move_step: the movement / collision step ----------------------------------------------------------------------------------
void __cdecl bounce(vector4* out, vector4* v, const vector4* n);                    // 0x432e30 (the result is *out)
static __forceinline float elapsed_inl(const Timer& t) { return (float)(g_ticks[t.clock] - t.start) * 1e-5f; }
static __forceinline void mv_vadd_to(vector4& a, const vector4& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
struct MV3 { float x, y, z; MV3() {} MV3(float a, float b, float c) : x(a), y(b), z(c) {} };
static __forceinline MV3 mv_vneg(const vector4& a) { return MV3(-a.x, -a.y, -a.z); }
static __forceinline void mv_vadd3(vector4& a, const MV3& b) { a.x = a.x + b.x; a.y = a.y + b.y; a.z = a.z + b.z; }
static __forceinline vector4 mv_vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 mv_vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 mv_unit_up(const vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.z * v.z + v.y * v.y);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        return vector4(v.x * inv, inv * v.y, inv * v.z);
    }
    return vector4(0.0f, 1.0f, 0.0f);
}
static __forceinline vector4 mv_vscale(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
// 0x4353a0 (game/agent/agent_queries.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) Blow* Blow::init()      // 0x4353a0
{
    flags = 0;
    attacker = -1;
    pos = vector4(0.0f, 0.0f, 0.0f);
    dir = vector4(1.0f, 0.0f, 0.0f);
    dir2 = dir;
    bone = 0;
    dmg_in = 0;
    kind = 0;
    item = -1;
    missile = 0;
    dmg_out = 0;
    absorbed = 0;
    return this;
}

// @ 0x45d820
void Agent::move_step(float dt)
{
    vector4 push(0.0f, 0.0f, 0.0f);
    vector4 v(0.0f, 0.0f, 0.0f);
    vector4 gv(0.0f, 0.0f, 0.0f);
    vector4 np;
    bool hit_ok;
    vector4 hits[16];
    shape_base* shapes[8188];
    int nh;
    {
    ::pos m = *(::pos*)&body->local;
    if ((flags3c & 1) && elapsed_inl(t40) >= 0.0f) {
        vector4 a(m.s.x * vel_xy.x, m.s.y * vel_xy.x, m.s.z * vel_xy.x);
        vector4 b(m.f.x * vel_xy.y, m.f.y * vel_xy.y, m.f.z * vel_xy.y);
        push = vector4(a.x + b.x, a.y + b.y, a.z + b.z);
        int fid = floor_face->mc;
        if (fid >= 0) {
            rgl::entity* ent = g_mission->scene->find_entity(fid);
            if (ent && ent->is_dirty()) {                  // the floor moved this frame: carry the agent with it
                vector4 loc = ent->prev.transform_point_to_local(pos);
                vector4 wp = ((::pos*)&ent->local)->transform_point(loc);
                vector4 d = mv_vsub(wp, pos);
                if (dt > 0.0f) {
                    float inv = 1.0f / dt;
                    vector4 t = mv_vscale(inv, d);
                    mv_vadd_to(push, t);
                }
            }
        }
    } else {
        v = v56a4;
        impulse.z = impulse.z - dt * 9.8f;
        mv_vadd_to(v, impulse);
    }
    float k = 1.0f - ((flags3c & 1) ? dt * 0.4f : dt * 0.1f);
    v.x = v.x * k;
    v.y = v.y * k;
    v.z = k * v.z;
    if (body && anim[0].cur != -1 && anim[0].sub != -1) {
        AnimDef* r = &defs->data[anim[0].cur];
        if (r->flags & 0x4000) {
            SubAnim* p = &r->subs[anim[0].sub];
            float inv = 1.0f / p->scale;
            gv = mv_vscale(inv, ((const ::pos&)body->local).rot(p->displacement));
        }
    }
    vector4 nv(v.x + push.x, v.y + push.y, v.z + push.z);
    v56a4 = nv;
    push.x = 0.0f;
    push.y = 0.0f;
    push.z = 0.0f;
    gv = vector4(nv.x + gv.x, nv.y + gv.y, nv.z + gv.z);
    np.x = gv.x * dt;
    np.y = gv.y * dt;
    np.z = gv.z * dt;
    capsule cap;
    camera_params(&cap);
    m.o = vector4(np.x + pos.x, np.y + pos.y, np.z + pos.z);
    cap.transform(&m);
    vector4 t;
    vector4 bmin, bmax;
    {
        vector4* q = cap.get_min(&t);
        bmin.x = q->x - 0.1f;
        bmin.y = q->y - 0.1f;
        bmin.z = q->z - 0.1f;
    }
    {
        vector4* q = cap.get_max(&t);
        bmax.x = q->x + 0.1f;
        bmax.y = q->y + 0.1f;
        bmax.z = q->z + 0.1f;
    }
    hit_ok = true;
    contact contact;
    if ((float)sqrt(np.y * np.y + np.x * np.x + np.z * np.z) > 0.0f) {
        std::vector<rgl::entity*> ents;
        g_mission->scene->collect_boxed(&bmin, &bmax, &ents);
        for (std::vector<rgl::entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
            rgl::entity* e = *it;
            unsigned eid = e->game_id;
            if (e->shape != 0 && e->game_kind == 0xb && eid != pool_id && eid != (unsigned)horse && eid != (unsigned)rider) {
                Agent* o = &g_mission->agents.pages[eid >> 4][eid & 0xf];
                if (o->state == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                    vector4 old = np;
                    bounce(&t, &np, &contact.normal);
                    vector4 rv = t;
                    if (this->kind == 1 && vel_xy.y > 4.0f && o->kind == 0 && o->horse == -1) {
                        int st = item.horse_charge();
                        float f = (float)st * 0.03f + 0.1f;
                        if (rider >= 0)
                            f = (float)g_game->skill_level(g_mission->agents.get(rider)->troop, 0x18) * 0.03f + f;
                        if (f < 0.0f)
                            f = 0.0f;
                        else if (f > 0.96f)
                            f = 0.96f;
                        float g = 1.0f - f;
                        rv.x = rv.x * g;
                        rv.y = rv.y * g;
                        rv.z = rv.z * g;
                        np = (np - old) * g + old;
                    } else if (this->kind == 1 && vel_xy.y > 3.0f && o->kind == 1) {
                        hit_ok = !(rgl::dot(body->local.f, o->body->local.f) > 0.0f) || !(o->vel_xy.y > 0.2f);
                    }
                    if (this->kind == 0 && o->kind == 0 && o->horse == -1) {
                        vec2 p2 = {np.x + pos.x, np.y + pos.y};
                        vec2 d2;
                        d2.x = o->pos.x - p2.x;
                        d2.y = o->pos.y - p2.y;
                        float len = d2.normalize();
                        if (len < 0.7f) {
                            float pen = (0.7f - len) * -0.6f;
                            a_v2 tt = ((a_v2&)d2) * pen;
                            np.x = tt.x + np.x;
                            np.y = tt.y + np.y;
                        }
                    }
                    m.o = vector4(np.x + pos.x, np.y + pos.y, np.z + pos.z);
                    cap.transform(&m);
                    mv_vadd3(push, mv_vneg(rv));
                    if (this->kind == 1 && vel_xy.y > 3.5f && o->kind == 0 && o->horse == -1 && (o->flags3c & 1) &&
                        o->t40.elapsed() >= 0.0f) {
                        if (rider == -1 || g_mission->agents.get(rider)->is_enemy(o)) {
                            Blow b;
                            b.init();
                            vector4 nn(-contact.normal.x, -contact.normal.y, -contact.normal.z);
                            b.bone = 7;
                            int s2 = item.horse_charge() + 5;
                            float dmg = (((nn.z * v56a4.z + nn.y * v56a4.y + nn.x * v56a4.x) + 0.1f) * (float)s2 * g_module_settings.horse_charge_damage_multiplier) * vel_xy.y * 0.015f;
                            if (dmg < 0.0f)
                                dmg = 0.0f;
                            b.dmg_in = dmg;
                            b.dir = nn;
                            b.dir2 = nn;
                            b.attacker = pool_id;
                            b.pos = contact.point;
                            b.kind = 2;
                            b.flags = 0x10;
                            o->note_blow(&b);
                        }
                    }
                }
            } else if (e->shape != 0 && e->game_kind == 0) {
                if (shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                    vector4 fv;
                    fv.x = contact.normal.x * contact.depth;
                    fv.y = contact.depth * contact.normal.y;
                    fv.z = contact.depth * contact.normal.z;
                    np.x = fv.x + np.x;
                    np.y = np.y + fv.y;
                    np.z = np.z + fv.z;
                    MV3 nn = mv_vneg(contact.normal);
                    vector4 mir;
                    mir.x = nn.x + nn.x;
                    mir.y = nn.y + nn.y;
                    mir.z = nn.z + nn.z;
                    // the rigid body's impulse slot (26); an agent's body is a strategic_entity, whose slot 26 is
                    // clear_emitters: the original calls it with these two arguments too
                    static_cast<rgl::collision_entity*>(e)->apply_impulse(&contact.point, &mir);
                    cap.p0.x = cap.p0.x + fv.x;
                    cap.p0.y = cap.p0.y + fv.y;
                    cap.p0.z = cap.p0.z + fv.z;
                    cap.p1.x = cap.p1.x + fv.x;
                    cap.p1.y = cap.p1.y + fv.y;
                    cap.p1.z = cap.p1.z + fv.z;
                    push.x = fv.x + push.x;
                    push.y = fv.y + push.y;
                    push.z = fv.z + push.z;
                }
            }
        }
    }
    unsigned mask = 0x8002;
    if (pool_id == (unsigned)g_mission->player_id || !(flags3c & 1))
        mask = 0x8102;
    int count;
    g_mission->scene->query_box(&bmin, &bmax, mask, shapes, &count);
    nh = 0;
    float best = 0.0f;
    obstacle = 0;
    vector4* hp = hits;
    for (int i = 0; i < count; i++) {
        shape_base* sh = shapes[i];
        if (shape_contact_count(&cap, sh, 1, &contact) == 0)
            continue;
        bool ok = true;
        float d = np.y * contact.normal.y + contact.normal.x * np.x + contact.normal.z * np.z;
        if (d > 0.0f) {
            ok = false;
        } else if (shapes[i]->shape_type() == 2 && static_cast<face*>(sh)->normal.z < 0.05f && d < best) {
            obstacle = static_cast<face*>(sh);
            best = d;
        }
        if (!(flags3c & 1)) {
            if (contact.point.z < pos.z + 0.4f && np.z <= 0.0f && shapes[i]->shape_type() == 2 && static_cast<face*>(sh)->normal.z > 0.1f) {
                flags3c |= 1;
                if (this->kind == 1)
                    set_order(0, 0x19e, 0x1000021, 1.0f);
                else
                    set_order(0, 3, 0x1000021, 1.0f);
            } else if (pos.z + 0.4f < contact.point.z && contact.normal.z > 0.95f && np.z <= 0.0f) {
                continue;
            }
        }
        if (ok && nh < 16) {
            *hp = contact.normal;
            nh++;
            hp++;
        }
    }
    }
    if (nh != 0) {
        int it = 0;
        bool ok2;
        do {
            ok2 = true;
            int jj = nh;
            vector4* hq = hits;
            if (jj > 0) do {
                vector4& n = *hq;
                vector4 nn(-n.x, -n.y, -n.z);
                float tq = np.y * nn.y + np.z * nn.z + nn.x * np.x;
                if (tq > 1e-6f) {
                    ok2 = false;
                    float tx = tq * n.x;
                    float ty = tq * n.y;
                    float tz = tq * n.z;
                    np.x = tx + np.x;
                    np.y = np.y + ty;
                    np.z = np.z + tz;
                    push.x = push.x + tx;
                    push.y = push.y + ty;
                    push.z = push.z + tz;
                }
                hq++;
            } while (--jj != 0);
            it++;
            if (it > 16) {
                if (!ok2) {
                    np.x = 0.0f;
                    np.y = 0.0f;
                    np.z = 0.0f;
                }
                break;
            }
        } while (!ok2);
    }
    face* nf;
    int tries;
    float fz;
    rgl::world* scene = g_mission->scene;
    if (gv.y * gv.y + gv.z * gv.z + gv.x * gv.x == 0.0f) {
        np = pos;
        nf = 0;
        tries = 0;
    } else {
        np = vector4(np.x + pos.x, np.y + pos.y, np.z + pos.z);
        vector4 raw = np;
        if (np.x < g_mission->min_x)
            np.x = g_mission->min_x;
        if (np.y < g_mission->min_y)
            np.y = g_mission->min_y;
        if (np.x > g_mission->max_x)
            np.x = g_mission->max_x;
        if (np.y > g_mission->max_y)
            np.y = g_mission->max_y;
        push.x = (np.x - raw.x) + push.x;
        push.y = (np.y - raw.y) + push.y;
        nf = 0;
        tries = 0;
    }
    do {
        ((rgl::world*)scene)->floor_at(&np, &fz, &nf);
        face* cur = floor_face;
        if (cur == 0) {
            np = mv_vadd(mv_vscale(0.95f, mv_vsub(np, pos)), pos);
        }
        tries++;
        if (tries > 30) {
            if (nf != 0)
                break;
            nf = cur;
            if (nf == 0)
                nf = (face*)scene->objs[0];
            np = pos;
            fz = pos.z;
        }
    } while (nf == 0);
    if (flags3c & 1) {
        float dz = pos.z - fz;
        if (dt * vel_xy.y + 0.4f < dz || nf->normal.z < 0.1f) {
            vector4 probe = mv_vadd(mv_vscale(0.4f, mv_unit_up(mv_vsub(np, pos))), pos);
            float fz2;
            face* tf;
            ((rgl::world*)scene)->floor_at(&probe, &fz2, &tf);
            float dz2 = pos.z - fz2;
            bool up = false;
            if (nh > 0) {
                int jj = nh;
                vector4* hq = hits;
                do {
                    if (hq->z > 0.0f)
                        up = true;
                    hq++;
                } while (--jj != 0);
            }
            if (pool_id == (unsigned)g_mission->player_id || (!(dz2 > 2.5f) && !(fz < g_mission->base_z - 1.5f))) {
                if (!up)
                    flags3c &= ~1u;
            } else {
                goto reset_pos;
            }
        } else if (!(dz < -1.5f)) {
            np.z = fz;
            if (dz < -0.06f || 0.06f < dz)
                v98.z = dz + v98.z;
        } else {
reset_pos:
            nf = floor_face;
            np = pos;
        }
    } else if (np.z < fz) {
        np.z = fz;
        flags3c |= 1;
        int snd;
        if (this->kind == 1) {
            if (defs->count > 0x19e && ((anim[0].events & 0x1ffff) <= 0x21 || (anim[0].cur == -1 && anim[0].next == -1))) {
                anim[0].next = 0x19e;
                anim[0].events = 0x1000021;
                anim[0].rate = 1.0f;
                if ((defs->data[0x19e].flags & 0x400) && anim[1].cur != -1 && (anim[1].events & 0x1ffff) < 0x21)
                    anim[1].set(-1, 0x21);
            }
            float var = 0.1f;
            if (fz < g_mission->base_z) {
                snd = 0x24;
                play_sound_var(&snd, &var);
            } else {
                snd = 0x22;
                play_sound_var(&snd, &var);
            }
        } else {
            if (defs->count > 3 && ((anim[0].events & 0x1ffff) <= 0x21 || (anim[0].cur == -1 && anim[0].next == -1))) {
                anim[0].next = 3;
                anim[0].events = 0x1000021;
                anim[0].rate = 1.0f;
                if ((defs->data[3].flags & 0x400) && anim[1].cur != -1 && (anim[1].events & 0x1ffff) < 0x21)
                    anim[1].set(-1, 0x21);
            }
            float var = 0.1f;
            if (fz < g_mission->base_z) {
                snd = 0x20;
                play_sound_var(&snd, &var);
            } else {
                snd = 0x1e;
                play_sound_var(&snd, &var);
            }
        }
        vector4 vn = v56a4;
        float len2 = (float)sqrt(vn.x * vn.x + vn.z * vn.z + vn.y * vn.y);
        if (len2 > 1e-10f) {
            float inv = 1.0f / len2;
            vn.x = vn.x * inv;
            vn.y = vn.y * inv;
            vn.z = vn.z * inv;
        } else {
            vn.x = 0.0f;
            vn.y = 1.0f;
            vn.z = 0.0f;
        }
        if ((this->kind == 1 && len2 > 11.0f) || len2 > 13.0f) {
            vector4 nrm = nf->normal;
            vector4 nn(-nrm.x, -nrm.y, -nrm.z);
            Blow b;
            b.init();
            vector4 hpos = vector4(np.x, np.y, fz + 0.01f);
            b.pos = hpos;
            b.dir = nf->normal;
            b.dir2 = nf->normal;
            b.dmg_in = nn.z * vn.z + nn.y * vn.y + vn.x * nn.x;
            b.dmg_in = b.dmg_in * len2;
            b.dmg_in = b.dmg_in + b.dmg_in;
            b.flags = 0;
            b.attacker = -1;
            b.bone = (this->kind != 1) ? 2 : 9;
            b.kind = 2;
            int kind = 5;
            play_hit_sound(&kind);
            if ((this->kind != 0 || horse != -1) && this->kind == 1)
                b.dmg_in = b.dmg_in + b.dmg_in;
            reduce_blow(&b);
            if (has_blow == 0 || !(b.dmg_out < blow.dmg_out)) {
                blow = b;
                has_blow = 1;
            }
        }
    }
    gv.x = np.x - pos.x;
    gv.y = np.y - pos.y;
    gv.z = np.z - pos.z;
    if (dt > 1e-5f) {
        float k5 = dt * 5.0f;
        float inv = 1.0f / dt;
        v = vector4(gv.x * inv, gv.y * inv, gv.z * inv);
        float om = 1.0f - k5;
        v56b4 = vector4(om * v56b4.x + v.x * k5, om * v56b4.y + v.y * k5, om * v56b4.z + v.z * k5);
        vec2 pp;
        pp.x = push.x;
        pp.y = push.y;
        if (pp.y * pp.y + pp.x * pp.x > 0.0f) {
            float sp = (float)sqrt(gv.z * gv.z + gv.y * gv.y + gv.x * gv.x) * inv;
            if (sp < vel_xy.y) {
                if (this->kind == 1 && vel_xy.y > 3.0f && sp < 0.5f && hit_ok) {
                    sp = 0.0f;
                    set_order(0, 0x19c, 0x1000049, 1.0f);
                    int kind = 4;
                    play_hit_sound(&kind);
                }
                vel_xy.y = vel_xy.y - (vel_xy.y - sp) * dt * 10.0f;
            }
        }
    }
    if (np.z > -9000.0f && np.z < 9000.0f) {
        pos = np;
        floor_face = nf;
    }
    impulse = vector4(0.0f, 0.0f, 0.0f);
}

// ---- Agent::update, Agent::tick: the per-frame agent update ---------------------------------------------------------------------------
// @ 0x45f660
void Agent::update(float dt)
{
    if (f5830 == 5)
        return;
    if (need_name) {
        draw_name();
        need_name = 0;
    }
    {
    float old = anim[1].scale;
    for (int i = 0; i < 2; i++) {
        EnvRec* c = &body->fx->env[i];
        float p = c->phase();
        if (c->shape_mode != 1) {
            if (c->shape_mode == 2)
                p = (float)sin(p * 1.5707964f);
            else if (c->shape_mode == 5)
                p = (float)sqrt(p);
            else if (c->shape_mode == 4)
                p = p * p;
            else
                p = 0.0f;
        }
        anim[i].scale = p;
    }
    float now = anim[1].scale;
    if (anim[1].cur >= 0 && anim[1].sub >= 0 && (defs->data[anim[1].cur].subs[anim[1].sub].flags & 0x200)) {
        for (int k = 0; k < 4; k++) {
            if (old <= g_anim_set.data[anim[1].cur].subs[anim[1].sub].events[k] &&
                g_anim_set.data[anim[1].cur].subs[anim[1].sub].events[k] < now) {
                if (k == 0) {
                    float pitch;
                    switch (anim[1].cur) {
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_sound_mgr.slot[43], 0, &body->local.o, pitch, 0);
                        break;
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_sound_mgr.slot[46], 0, &body->local.o, pitch, 0);
                        break;
                    }
                } else if (k == 1) {
                    float pitch;
                    switch (anim[1].cur) {
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.4f + 0.8f;
                        g_sound_mgr.play(g_sound_mgr.slot[44], 0, &body->local.o, pitch, 0);
                        break;
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_sound_mgr.slot[45], 0, &body->local.o, pitch, 0);
                        break;
                    }
                }
            }
        }
    }
    }
    prev_scale1 = prev_scale0;
    prev_scale0 = anim[0].scale;
    float speed = a_len2(vel_xy);
    if (anim[0].cur >= 0) {
        unsigned flags = defs->data[anim[0].cur].subs[anim[0].sub].flags;
        if (disabled == 0 && (flags & 0x300)) {
            int kind = 0;
            int k = 0;
            float pitch;
            do {
                if (prev_scale1 <= g_anim_set.data[anim[0].cur].subs[anim[0].sub].events[k] &&
                    g_anim_set.data[anim[0].cur].subs[anim[0].sub].events[k] < prev_scale0) {
                    if ((flags & 0x200) && k == 0) {
                        switch (anim[0].cur) {
                        case 0x11a:
                        case 0x11b:
                        case 0x11c:
                        case 0x11d:
                        case 0x11e:
                        case 0x11f:
                        case 0x120:
                        case 0x121:
                        case 0x122:
                        case 0x123:
                        case 0x125:
                        case 0x1a2:
                        case 0x1a3:
                        case 0x1a4: {
                            pitch = frand() * 0.4f + 0.8f;
                            g_sound_mgr.play(g_sound_mgr.slot[90], 0, &body->local.o, pitch, 0);
                        }
                        }
                    }
                    if (flags & 0x100) {
                        if (pos.z < g_mission->base_z) {
                            if (this->kind == 0 || k == 3 || k == 4)
                                play_pain_sound(g_mission->base_z - pos.z);
                        } else {
                            play_hit_sound(&kind);
                        }
                        if (fx1b0 && speed > 0.0f) {
                            if (pos.z < g_mission->base_z) {
                                float s = 0.2f;
                                if (this->kind == 1)
                                    s = 0.9f;
                                ::pos m;
                                identity_pos((::pos*)&m);
                                vec2 d;
                                d.x = vel_xy.x;
                                d.y = vel_xy.y;
                                d.normalize();
                                m.o.x = s * d.x;
                                m.o.y = s * d.y;
                                m.f.x = vel_xy.x;
                                m.f.y = vel_xy.y;
                                m.f.z = 0.0f;
                                m.f.normalize();
                                m.orthonormalize();
                                fx1b0->local = m;
                                fx1b4->local = m;
                                float v;
                                if (this->kind == 1)
                                    v = speed * 10.0f + 20.0f;
                                else
                                    v = speed * 3.0f + 5.0f;
                                fx1b0->reset_pending((int)(v * 0.5f));
                                fx1b4->reset_pending((int)(v * 0.4f));
                                a_v2 u = a_v2(d.x, d.y) * 0.7f;
                                a_add2(&m.o.x, u);
                                fx1b8->local = m;
                                fx1b8->reset_pending(1);
                            } else if (fx1ac) {
                                float v = 0.0f;
                                if (this->kind == 1) {
                                    if (speed > 5.5f && floor_face && (floor_face->m8 & 0x4000))
                                        v = speed + 10.0f;
                                    if (fx1ac->tag != g_emit_defs.used[4])
                                        v = v * 3.0f;
                                    fx1ac->reset_pending((int)(v * 0.15f));
                                }
                            }
                        }
                    }
                }
                k++;
                kind = k;
            } while (k < 4);
        }
    }
    float dt0 = dt;
    smooth_turn(dt0);
    if (rider == -1) {
        if (f7c != 2) {
            ai_idle_tick(&dt);
            poll_input(dt0, &in_now);
        }
        if (horse >= 0) {
            unsigned id = horse;
            Agent* o = g_mission->agents.pages[id >> 4] + (id & 0xf);
            o->in_now = in_now;
        }
    }
    if (dt > 0.0f) {
        process_blow(dt0);
        update_controls(dt0);
        update_attack(dt0);
        if (horse == -1)
            move_step(dt0);
    }
    update_pitch(dt0);
    int j = 0;
    do {
        if ((&v98.x)[j] < -0.3f)
            (&v98.x)[j] = -0.3f;
        else if ((&v98.x)[j] > 0.3f)
            (&v98.x)[j] = 0.3f;
        if ((&v98.x)[j] < -0.001f) {
            if (j == 2)
                (&v98.x)[2] = ((((&v98.x)[2] - 0.7f) * dt) * 5.0f + 1.0f) * (&v98.x)[2];
            else
                (&v98.x)[j] = (1.0f - (&v98.x)[j]) * dt + (&v98.x)[j];
            if ((&v98.x)[j] > 0.0f)
                (&v98.x)[j] = 0.0f;
        } else if ((&v98.x)[j] > 0.001f) {
            a_dec(&v98.x, j, dt);
            if ((&v98.x)[j] < 0.0f)
                (&v98.x)[j] = 0.0f;
        } else {
            (&v98.x)[j] = 0.0f;
        }
        j++;
    } while (j < 3);
    update_aim_error();
}
// @ 0x45ff30
void Agent::tick()
{
    if (body) {
        update_anims(*(int*)&g_step_tbl[2]);
        if (horse >= 0) {
            unsigned id = horse;
            Agent* o = g_mission->agents.pages[id >> 4] + (id & 0xf);
            pos = o->pos;
            vel = o->vel;
            rot[0] = o->rot[0];
            v56b4 = o->v56b4;
            v56a4 = o->v56a4;
            floor_face = o->floor_face;
            vel_xy.x = o->vel_xy.x;
            vel_xy.y = o->vel_xy.y;
            flags3c = o->flags3c;
            v98 = o->v98;
        }
        if (dismount_state && f5830 == 2 && rider == -1 &&
            (pos.x - g_mission->min_x < 1.0f || pos.y - g_mission->min_y < 1.0f ||
             g_mission->max_x - pos.x < 1.0f || g_mission->max_y - pos.y < 1.0f)) {
            f5830 = 3;
            restart_inl(tm);
            draw_name();
        }
        tick_death();
        ragdoll_update();
        if (channel >= 0 && snd_length < ((float)(g_ticks[snd_stamp.clock] - snd_stamp.start) * 1e-05f))
            channel = -1;
    }
}
