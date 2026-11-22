//-----------------------------------------------------------------------------
// File: agent_ai.cpp  (0x444c50..0x44e970)
//
// Desc: nav-mesh path search (find_path, reachable), goal and path planning, ammo top-up, weapon mode, attack start, update_ai, slot_class_impl (static, EAX), ai_update_control, cycle_slot, spawn_missile, spawn_prop, choose_action, ai_idle_tick, shoot
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
// vector4::out_length 0x407c50: a header inline in the original, left out of line by cl. fire_check's g_mission survives the call, so the body was visible.
inline __declspec(noinline) float rgl::vector4::out_length() const { return length(); }
//=============================================================================
// 0x43a280..0x43b100, from agent_stats.cpp
// the weapon class names, the std::vector / std::_Uninit_* instantiations, the level queries, the agent's quiver and the light accumulator
//=============================================================================
// inline vector helpers of the original: rgl's operators are out-of-line in other objects
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
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
// @ 0x43b1f0
inline Agent* AgentTable::get(unsigned id)
{
    return &pages[id >> 4][id & 0xf];
}
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
// 0x440b30 vector<int>::push_back (agent_combat.cpp) is kept out of line


#include "engine/core/std_vec_insert_n.h"      // vector<rgl::mesh*>::_Insert_n 0x410610 (declared only: the call is out of line)

//=============================================================================
// 0x444c50..0x460160
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
extern int g_propkind_count;                                // 0x5e29e0

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----

unsigned g_input_mask;                                      // 0x5e2474


// ---- canonical classes / forwarders (see agent_logic.h) ----------------------------------------------------------------------------

// inline vector helpers of the original object
static inline vector4 a_vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static inline vector4 a_vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vector4 a_vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

#define itp_two_handed 0x200000

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
// @ 0x444c50
bool PolyMesh::find_path(int from, int to, int* out_faces, vector4* out_pos, int* out_count, int* out_steps,
                        const float* w0, const float* w1, const float* w2)
{
    int chain[0x2004];
    while (!open.empty())
        open.pop_inl();
    next_stamp();
    faces[from].stamp = stamp + 1;
    faces[from].parent = from;
    hnode n;
    n.g = 0.0f;
    n.f = 0.0f;
    n.id = from;
    open.push(n);
    *out_steps = 0;
    float maxg = 0.0f;
    while (faces[to].stamp <= stamp) {
        if (open.empty() || !(n.f < 1e+06f))
            break;
        (*out_steps)++;
        n = open.top();
        open.pop_inl();
        int cur = n.id;
        for (int k = 0; k < faces[cur].n; k++) {
            int nb = faces[cur].nbr[k];
            if (nb != -1 && faces[nb].stamp <= stamp) {
                float cost = 1.0f;
                faces[nb].stamp = stamp + 1;
                faces[nb].parent = cur;
                MEdge* e = &edges[faces[cur].eid[k]];
                float w = e->face[0] == nb ? e->crease[0] : e->crease[1];
                if (w < 0.65f)
                    cost = (0.65f - w) * 20.0f + 1.0f;
                if (w <= *w0)
                    cost = (1.0f - w) * 50.0f + cost;
                vector4 dg = a_vsub(faces[to].centroid, faces[nb].centroid);
                vector4 ds = a_vsub(faces[from].centroid, faces[nb].centroid);
                float ls = ds.length();
                float lg = dg.length();
                cost = cost + n.g;
                if (n.g > maxg)
                    maxg = n.g;
                open.push(hnode(nb, cost, (lg / ls + 1.0f) * cost));
            }
        }
        faces[cur].stamp = stamp + 2;
    }
    if (faces[to].stamp <= stamp) {
        *out_count = 0;
        return false;
    }
    int len = 0;
    int f = to;
    while (f != from && len < 0x2000) {
        chain[len++] = f;
        f = faces[f].parent;
    }
    chain[len++] = from;
    *out_count = len < 0x400 ? len : 0x400;
    int prev = from;
    for (int i = 0; i < *out_count; i++) {
        int cur = chain[len - 1 - i];
        out_faces[i] = cur;
        int k = 0;
        MPoly& pr = faces[prev];
        for (int j = 0; j < pr.n; j++) {
            if (pr.nbr[j] == cur) {
                k = j;
                break;
            }
        }
        out_pos[i] = edges[faces[prev].eid[k]].mid;
        prev = out_faces[i];
    }
    for (int i = 1; i < *out_count; i++)
        out_pos[i - 1] = out_pos[i];
    out_pos[*out_count - 1] = faces[out_faces[*out_count - 1]].centroid;
    return true;
}


// ---- PolyMesh::reachable: can the goal face be reached within `max_steps` hops? -----------------------------------------------
// @ 0x4452f0
bool PolyMesh::reachable(const int* from, const int* to, const int* max_steps)
{
    while (!open.empty())
        open.pop_inl();
    next_stamp();
    faces[*from].stamp = stamp + 1;
    faces[*from].parent = *from;
    hnode n;
    n.g = 0.0f;
    n.f = 0.0f;
    n.id = *from;
    open.push(n);
    while (faces[*to].stamp <= stamp) {
        if (open.empty() || !(n.f < 1e+06f))
            break;
        n = open.top();
        open.pop_inl();
        if ((int)n.g <= *max_steps) {
            int cur = n.id;
            for (int k = 0; k < faces[cur].n; k++) {
                int nb = faces[cur].nbr[k];
                if (nb != -1 && faces[nb].stamp <= stamp) {
                    faces[nb].stamp = stamp + 1;
                    faces[nb].parent = cur;
                    vector4 dg = a_vsub(faces[*to].centroid, faces[nb].centroid);
                    vector4 ds = a_vsub(faces[*from].centroid, faces[nb].centroid);
                    float g = n.g + 1.0f;
                    float h = ((float)sqrt(dg.y * dg.y + dg.z * dg.z + dg.x * dg.x) / ds.length()) * 0.7f + 1.0f;
                    open.push(hnode(nb, g, h * g));
                }
            }
        }
        faces[n.id].stamp = stamp + 2;
    }
    return faces[*to].stamp > stamp;
}

// ---- std::vector<PhysRec> helpers (0x445700 _Ucopy; 0x445e70 _Ufill and 0x445ea0 _Insert_n further down) ----------------------------
// @ 0x445700
template std::vector<PhysRec>::pointer std::vector<PhysRec>::_Ucopy<PhysRec*>(PhysRec*, PhysRec*, PhysRec*);

// ---- Agent helpers --------------------------------------------------------------------------------------------------
// @ 0x445720
void Agent::pick_reachable_goal(const vector4* center, const float* radius, const int* face, const int* max_steps)
{
    vector4 c = *center;
    int tries = 0;
    do {
        set_goal_near(&c, radius);
        if (*face < 0)
            return;
        if (ai.goal_face < 0)
            return;
        if (g_mission->nav.reachable(face, &ai.goal_face, max_steps))
            return;
        const MPoly& f = g_mission->nav.faces[*face];
        float k = (float)(rand() % 0x3dc9) * 6.3223115e-06f + 0.7f;
        c = a_vadd(a_vscale(k, a_vsub(c, f.centroid)), f.centroid);
        tries++;
    } while (tries < 8);
}

// @ 0x445850
void Agent::goal_near_for_class(const vector4* center, const float* radius)
{
    int t = team;
    if (t < 0 || t >= 8)
        t = 7;
    int cls = 0;
    if (troop >= 0) {
        if (horse >= 0)
            cls = 2;
        else if ((g_game->troops[troop].flags30 & 0x4000000) != 0)
            cls = 1;
        else
            cls = 0;
    }
    ClassInfo* r = &g_mission->teams[t].cls[cls];
    int max_steps = 7;
    pick_reachable_goal(center, radius, &r->nav, &max_steps);
    ai.goal_dir = r->target;
}

// @ 0x445910
bool Agent::plan_path()
{
    ai.path_count = 0;
    float width, w1, w2;      // w1 / w2 are never written and find_path (same unit) never reads them: they take width's frame slot
    if (kind == 0 && horse == -1)
        width = 0.45f;
    else
        width = 0.6f;
    int face = update_nav_face();
    if (face >= 0 && ai.goal_face >= 0) {
        g_mission->nav.find_path(face, ai.goal_face, ai.pface(), ai.path_pos, &ai.path_count, &ai.path_steps,
                                 &width, &w1, &w2);
        ai.path_cur = 0;
        ai.path_flag = ai.path_count <= 0;
        ai.goal_face_done = ai.goal_face;
    }
    return ai.path_count > 0;
}

// @ 0x4459c0
void Agent::play_class_sound(const int* kind)
{
    unsigned cls = (unsigned)g_game->troops[troop].flags30 & 0xf;
    int snd = g_anim.voice(cls, *kind);
    if (snd >= 0)
        play_sound(&snd);
}

// @ 0x445a20
void Agent::on_wield_reset()
{
    both_hands = 0;
    if (slots[0] >= 0) {
        int item = pairs[slots[0]].item;
        if (item >= 0 && (g_items[item].type_flags & itp_two_handed) != 0)
            both_hands = 1;
    }
    update_equipment_display();
    on_wield_changed();
    fr2.o.x = -20000000.0f;
    fr1.o.x = -20000000.0f;
}

// ---- std::vector<PhysRec> helpers 0x445e70 / 0x445ea0 (the whole _Insert_n COMDAT is 0x33e bytes: 0x445ea0..0x4461dd, including
// the two catch(...) funclets 0x44608f / 0x44614e) ------------------------------------------------------------------------------
// @ 0x445e70
template std::vector<PhysRec>::pointer std::vector<PhysRec>::_Ufill(PhysRec*, size_t, const PhysRec&);
// @ 0x445ea0
template void std::vector<PhysRec>::_Insert_n(std::vector<PhysRec>::iterator, size_t, const PhysRec&);

// ---- Agent: top up the ammunition of the held ranged weapon ----------------------------------------------------
// 0x434b70 / 0x434be0 / 0x434c30 (ItemRef::max_amount / add_amount / room_left) are header-inline in the original: the bodies are
// visible here, so cl inlines some uses and keeps a call for the others (the slice's local copies did the same)
static inline int round_float_inl(const float* p)       // 0x432540 (inlined here; game/agent/agent_queries.h declares the out-of-line round_float)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}
static int ir_max_amount(ItemRef* r)                      // 0x434b70
{
    if (r->item < 0) return 0;
    int v = g_items[r->item].max_ammo;
    if (v != 0 && (r->m4 & 0xff000000) == 0x2a000000) {
        float f = v * 0.13f;
        int d = round_float_inl(&f);
        if (d < 1) d = 1;
        v += d;
    }
    return v;
}
static void ir_add_amount(ItemRef* r, int n)             // 0x434be0
{
    int v = (r->m4 & 0xff) + n;
    if (v > ir_max_amount(r))
        v = ir_max_amount(r);
    r->b.amount = 0;
    r->m4 = r->m4 | v;
}
static int ir_room_left(ItemRef* r)                      // 0x434c30
{
    return ir_max_amount(r) - (int)(r->m4 & 0xff);
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
inline __declspec(noinline) int rgl::mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)vertices.size();
    vertices.resize(n + 1);
    (&*vertices.begin())[n].index = index;
    (&*vertices.begin())[n].uv = *uv;
    (&*vertices.begin())[n].color = *color;
    (&*vertices.begin())[n].f5c = *f5c;
    vertex60* vv = &vertices[n];
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}
inline __declspec(noinline) int rgl::mesh::add_point(const vector4* p)
{
    ensure_local();
    int n = (int)points.size();
    points.resize(n + 1);
    points[n].a = *p;
    points[n].b = *p;
    return n;
}
static inline unsigned itype(const ItemRef* p) { return g_items[p->item].type_flags & 0xff; }

// @ 0x445a90
int Agent::top_up_ammo(int count)
{
    ItemRef* cur = 0;
    if (slots[0] >= 0)
        cur = &pairs[slots[0]];
    int added = 0;
    if (cur != 0) {
        unsigned wt = g_items[cur->item].type_flags & 0xff;
        if (wt == 8 || wt == 9 || wt == 0x10 || wt == 0x11 || wt == 10) {
            bool done = false;
            for (int i = 0; i < 4 && !done; i++) {
                ItemRef* s = &pairs[i];
                if (s->item >= 0) {
                    if ((itype(s) == 5 && itype(cur) == 8) ||
                        (itype(s) == 6 && itype(cur) == 9) ||
                        (itype(s) == 0x12 && itype(cur) == 0x10) ||
                        (itype(s) == 0x12 && itype(cur) == 0x11) ||
                        (itype(cur) == 10 && s->item == cur->item)) {
                        while (added < count) {
                            if (ir_room_left(s) <= 0)
                                break;
                            added++;
                            ir_add_amount(s, 1);
                        }
                        if (added >= count)
                            done = true;
                    }
                }
            }
            equip_value = equipment_value();
            if ((char)g_items[cur->item].type_flags == 10) {
                int missing, total, slot;
                best_ammo(-1, &missing, &total, &slot);
                if (missing <= 0) {
                    slots[0] = -1;
                    on_wield_reset();
                    float rate = damage_factor(0);
                    int anim_id = snd_b;
                    if (anim_id >= -1 && anim_id < defs->count && this->anim[1].set(anim_id, 0x11400020)) {
                        this->anim[1].rate = rate < 0.1f ? 0.1f : (rate >= 10.0f ? 10.0f : rate);
                        if (slots[0] >= 0 && pairs[slots[0]].item >= 0)
                            links[slots[0]]->mask = 0;
                    }
                }
            }
        }
    }
    return added;
}

// @ 0x4461e0
void Agent::set_weapon_mode(int mode)
{
    if (weapon_mode != mode) {
        int hit = -1;
        int hit2 = -1;
        int t1 = team;
        if (t1 < 0 || t1 >= 8) t1 = 7;
        int tr = troop;
        int k1 = 0;
        if (tr >= 0) {
            if (horse >= 0)
                k1 = 2;
            else
                k1 = (g_game->troops[tr].flags30 & 0x4000000) ? 1 : 0;
        }
        int ra = g_mission->teams[t1].cls[k1].a;
        int t2 = team;
        if (t2 < 0 || t2 >= 8) t2 = 7;
        int k2 = 0;
        if (tr >= 0) {
            if (horse >= 0)
                k2 = 2;
            else
                k2 = (g_game->troops[tr].flags30 & 0x4000000) ? 1 : 0;
        }
        int rb = g_mission->teams[t2].cls[k2].b;
        for (int i = 3; i >= 0; i--) {
            ItemRef* p = &pairs[i];
            if (p->item >= 0) {
                ItemKind* ik = &g_items[p->item];
                bool ok = ((ik->swing_damage & 0x300) == 0x200 || p->damage(0) <= 1) &&
                          ((ik->thrust_damage & 0x300) == 0x200 || p->damage(1) <= 1);
                if ((ra == 0 || ok) &&
                    (rb == 0 || ((ik->type_flags & 0xff) != 8 && (ik->type_flags & 0xff) != 9 &&
                                 (ik->type_flags & 0xff) != 10 && (ik->type_flags & 0xff) != 0x10 &&
                                 (ik->type_flags & 0xff) != 0x11)) &&
                    (horse < 0 || !(ik->type_flags & 0x10000000))) {
                    if (mode == 1) {
                        unsigned fl = ik->type_flags;
                        unsigned type = fl & 0xff;
                        if ((type == 2 || type == 3 || type == 4) && (fl & 0x400000))
                            hit = i;
                    } else if (mode == 2) {
                        if (ik->is_ranged()) {
                            int missing, total, slot_out;
                            best_ammo(i, &missing, &total, &slot_out);
                            if (missing > 0) {
                                if (!(ik->type_flags & 0x100000) || horse == -1)
                                    hit = i;
                            }
                        }
                    } else if (mode == 3) {
                        if ((char)ik->type_flags == 4)
                            hit = i;
                    }
                }
                if ((char)ik->type_flags == 7 && (int)(p->m4 & 0xffff) > 1)
                    hit2 = i;
            }
        }
        if (hit != -1)
            slots[0] = hit;
        else if (mode == 1)
            slots[0] = -1;
        if (hit2 != -1)
            slots[1] = hit2;
        weapon_mode = mode;
        ai.combat_state = 0;
        ai.input |= 0x200;
        on_wield_reset();
    }
}

// @ 0x4464d0
void Agent::start_attack(int kind)
{
    ai.simple_behavior = kind;
    float scale = 5.0f;
    switch (kind) {
    case 4:
        if (!has_weapon_for(1)) {
            if (has_weapon_for(3)) {
                set_weapon_mode(3);
                scale = 8.0f;
            } else {
                set_weapon_mode(1);
                scale = 8.0f;
            }
        } else {
            set_weapon_mode(1);
            scale = 8.0f;
        }
        break;
    case 7:
        if (has_weapon_for(3)) {
            set_weapon_mode(3);
            scale = 9.0f;
        } else {
            set_weapon_mode(1);
            scale = 9.0f;
        }
        break;
    case 5:
    case 6:
        set_weapon_mode(2);
        // fall through
    case 11:
    case 13:
    case 14:
        scale = 10.0f;
        break;
    case 8:
        scale = 2.0f;
        break;
    // cases 9, 10 and 12 need their own (empty after optimisation) targets for cl to emit a plain dword jump table
    case 9:
        scale = scale;
        break;
    case 10:
        scale = scale;
        break;
    case 12:
        scale = scale;
        break;
    }
    ai.ta_interval = ((float)(rand() % 0x3dc9) * 3.161156e-05f + 0.75f) * scale;
}

// ---- Agent::update_ai: the per-tick order executor ------------------------------------------------------------------
#define AG(i) (*agent_by_id(i))
#define tf_guarantee_ranged 0x4000000i64
float __cdecl hit_chance(int* who, Agent* a);                          // 0x438cb0 (game/agent/agent_stats.cpp)
static __forceinline float hit_chance_ref(const ItemRef& who, Agent* a) { return hit_chance((int*)&who, a); }
int __cdecl team_leader(const Agent* a);                                // 0x435b60
float frand();                                                          // 0x40e400
float __cdecl fmin_ref(const float* a, const float* b);                 // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
float g_dt_scale = 1.0f;                                    // 0x5a9bf0
unsigned g_input_flags;                                     // 0x5e2470


__forceinline ClassInfo* Agent::class_rec()
{
    int cls = team;
    if (cls < 0 || cls >= 8) cls = 7;
    int kind = 0;
    if (troop >= 0) {
        if (horse >= 0)
            kind = 2;
        else
            kind = (g_game->troops[troop].flags30 & tf_guarantee_ranged) ? 1 : 0;
    }
    return &g_mission->teams[cls].cls[kind];
}


// 1 + the cosine of v (normalised in its by-value copy: loads run ahead of the copy's stores) with w
static inline float a_cos1(vector4 v, const vector4& w) { v.normalize(); return v.x * w.x + v.z * w.z + v.y * w.y + 1.0f; }
static inline float a_dot3r(const vector4& b, const vector4& a) { return a.z * b.z + a.y * b.y + a.x * b.x; }
static inline void a_sub3(vector4& r, const vector4& a, const vector4& b) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; }
// @ 0x4465e0
void Agent::update_ai()
{
    ClassInfo* cr = class_rec();

    int prev = -1;
    int near_ = ai.last_attacker;
    if (near_ >= 0 && g_mission->agents[near_].state == 1) {
        Agent* o = &g_mission->agents[ai.last_attacker];   // re-read, not near_: the original reloads it here
        vector4 d;
        a_sub3(d, o->pos, pos);
        float len = d.normalize();
        bool far_ = true;
        if (ai.target >= 0) {
            const vector4& cp = g_mission->agents[ai.target].pos;
            vector4 e = cp - pos;
            far_ = !(sqrt(e.y * e.y + (e.z * e.z + e.x * e.x)) <= len + 1.0f);
        }
        if (far_ && is_enemy(o))
            ai.target = near_;
        prev = near_;
    }

    bool flag_a = false;        // local_81
    bool flag_b = false;        // local_5d
    int mode_ = ai.simple_behavior;

    if (mode_ == 1) {
        if (!(ai.alarm & 1)) {
            ai.move_mode = 0;
            ai.path_state |= 0x20;
        } else {
            unsigned t = ai.target;
            if (t == 0xffffffff || AG(t).state != 1 || frand() < 0.1f)
                flag_a = true;
            t = ai.target;
            if ((int)t >= 0) {
                bool ranged = false;
                const vector4& op = AG(ai.target).pos;
                vector4 dd = a_vsub(pos, op);
                float dist = (float)sqrt(dd.y * dd.y + (dd.z * dd.z + dd.x * dd.x));
                ItemRef p = AG(ai.target).cur_pair_fi();
                if (p.item >= 0 && g_items[p.item].is_ranged())
                    ranged = true;
                if (horse >= 0 || v56b4.length_squared() < 0.7f || dist < 15.0f ||
                    (dist < 100.0f && weapon_mode == 2) || ranged) {
                    ai.move_mode = 2;
                    ItemRef c = cur_pair_fi();
                    if (c.item >= 0 && g_items[c.item].is_ranged())
                        ai.move_mode = 0x22;
                }
            }
            flag_b = true;
            ai.path_state |= 0x20;
        }
    } else if (mode_ == 4) {
        if (prev == -1 || ai.target == -1 || AG(ai.target).state != 1)
            ai.target = pick_shout_target(0x26, 300.0f);
        if (ai.target == -1) {
            reset_if_enemy_charged();
            ai.move_mode = 0;
        } else if (AG(ai.target).state != 1) {
            ai.ta_interval = 0.0f;
        } else {
            vector4 q;
            const vector4& op = AG(ai.target).pos;
            vector4 w = op - pos;
            float dist = w.normalize();
            ItemRef sp = AG(ai.target).slot_pair(0);
            int ranged = 0;
            if (sp.item >= 0 && g_items[sp.item].is_ranged())
                ranged = 1;
            ai.move_mode = 2;
            if (!ranged && dist > 15.0f && horse < 0)
                ai.move_mode = 0;
            if (ai.melee_charge < 0.0f)
                ai.melee_charge = 0.0f;
            else if (ai.melee_charge > 30.0f)
                ai.melee_charge = 30.0f;
            vector4 acc[2];
            float wsum[2];
            q = vector4(0.0f, 0.0f, 0.0f);
            acc[0] = q;
            q = vector4(0.0f, 0.0f, 0.0f);
            acc[1] = q;
            wsum[0] = wsum[1] = 0.0f;
            float t98 = 0.0f;                       // cnt, later k and the clamp argument (one address-taken scratch)
            int best = -1;
            Mission* m = g_mission;
            unsigned it = m->agents.first_used();
            if (it < (unsigned)m->agents.count) {
                do {
                    Agent* a = &m->agents.pages[it >> 4][it & 0xf];
                    if (a->kind == 0 && a->pool_id != ai.target && a->state == 1 && a->pool_id != pool_id) {
                        vector4 dv = a->pos - pos;
                        float len = dv.normalize();
                        float k = 2.0f / (2.0f + len);
                        float w2 = k * k;
                        int idx = 0;
                        if (is_enemy(a)) {
                            idx = 1;
                            if (len < 5.0f) {
                                t98 += 1.0f;
                                best = it;
                            }
                        }
                        wsum[idx] = w2 + wsum[idx];
                        float* ap = &acc[idx].x;
                        vector4 t = dv * w2;
                        ap[0] = t.x + ap[0];
                        ap[1] = t.y + ap[1];
                        ap[2] = t.z + ap[2];
                    }
                    m = g_mission;
                    m->agents.next_used(&it);
                } while (it < (unsigned)m->agents.count);
            }
            if (dist > 10.0f && best >= 0) {
                const vector4& bp = m->agents.pages[(unsigned)best >> 4][(unsigned)best & 0xf].pos;
                q.x = bp.x - pos.x;
                q.y = bp.y - pos.y;
                q.z = bp.z - pos.z;
                q.normalize();
                if (q.x * ai.move_dir.x + (q.y * ai.move_dir.y + q.z * ai.move_dir.z) > -0.3f) {
                    ai.target = best;
                    m = g_mission;
                }
            }
            bool close_ = false;
            if (cr->kind == 1) {
                hand_pos(&q);
                vector4 d = pos - q;
                float len = d.normalize();
                if (len > 5.0f) {
                    ai.simple_behavior = 1;
                    float r = 0.2f;
                    goal_near_for_class(&q, &r);
                    close_ = true;
                }
                m = g_mission;
            }
            if (cr->kind == 2) {
                int t2 = team;
                if (t2 < 0 || t2 >= 8) t2 = 7;
                if (m->teams[t2].leader != pool_id) {
                    hand_pos(&q);
                    vector4 d = pos - q;
                    float len = d.normalize();
                    if (len > 8.0f) {
                        ai.simple_behavior = 9;
                        goto L_tail4;
                    }
                }
            }
            if (!close_) {
                if (ai.circle_mode == 0) {
                    float d0 = wsum[1] - wsum[0];      // stays on the x87 stack across both tests
                    float diff;
                    if (d0 < 0.05f)
                        diff = 0.05f;
                    else if (d0 >= 0.3f)
                        diff = 0.3f;
                    else
                        diff = d0;
                    int r = rand() % 0x3dc9;
                    int hf = ai.always_attack_in_melee;
                    float push = (((float)r * (1.0f / 15817.0f)) * diff * g_dt_scale) * 20.0f;
                    if (hf == 0)
                        push = ((float)g_options.difficulty * 0.5f + (40.0f - (float)g_game->troops[troop].level) * 0.012f) + push;
                    push = push + ai.melee_charge;
                    ai.melee_charge = push;
                    if (push < 0.0f)
                        ai.melee_charge = 0.0f;
                    else if (push > 11.0f)
                        ai.melee_charge = 11.0f;
                    if (ai.scripted_mode == 1)
                        ai.melee_charge = 0.0f;
                    {
                        ItemRef cp = AG(ai.target).cur_pair_fi();
                        if ((cp.item < 0 || !g_items[cp.item].is_ranged()) && ai.melee_charge >= 10.0f) {
                            ai.melee_charge = 10.0f;
                            if (hf == 0)
                                ai.circle_mode = 1;
                        }
                    }
                    if (ai.circle_mode_prev != 0 || frand() < 0.3f - t98 * 0.05f) {
                        ai.circle_mode_prev = ai.circle_mode;
                        if (ai.scripted_mode == 1) {
                            float r2 = 0.2f;
                            set_goal_near(&ai.scripted_destination.o, &r2);
                        } else {
                            int fid = -1;
                            if (g_mission->scene->hf)
                                g_mission->scene->hf->sample(pos.x, pos.y, wsum, &fid);
                            if (dist < 4.0f ||
                                (g_mission->scene->hf && (floor_face == 0 || floor_face->m4 != fid))) {
                                set_goal_agent(ai.target);
                            } else {
                                float r30 = 30.0f;
                                float m = fmin_ref(&dist, &r30);
                                float fr = frand();
                                AgentTable* tb = &g_mission->agents;
                                int u = ai.target;
                                t98 = ((fr + 2.0f) * m) * 0.2f;
                                vector4 d = t98 * tb->get(u)->v56b4;
                                float f = a_cos1(tb->get(u)->v56b4, w);
                                w.x = d.x * f;
                                w.y = d.y * f;
                                w.z = d.z * f;
                                const vector4* pp = &tb->get(u)->pos;
                                q.x = w.x + pp->x;
                                q.y = w.y + pp->y;
                                q.z = w.z + pp->z;
                                d = q;
                                t98 = dist * 0.3f - 0.6f;
                                float hi = 20.0f;
                                float lo = 0.0f;
                                float c = clamp(&t98, &lo, &hi);
                                if (ai.always_attack_in_melee != 0 && c > 0.5f)
                                    c = 0.5f;
                                int ms = (int)(c * 0.2f + 4.0f);
                                int face = tb->get(u)->update_nav_face();
                                pick_reachable_goal(&d, &c, &face, &ms);
                            }
                        }
                    }
                } else if (ai.circle_mode == 1) {
                    float val = wsum[0] * 1.3f - wsum[1];
                    float hi = 0.3f;
                    float lo = 0.01f;
                    float nv = ai.melee_charge - ((clamp(&val, &lo, &hi) * frand()) * g_dt_scale) * 20.0f;   // clamp before frand: a named c would be sunk below frand (clamp is pure here)
                    ai.melee_charge = nv;
                    if (nv < 0.0f)
                        ai.melee_charge = 0.0f;
                    else if (nv > 11.0f)
                        ai.melee_charge = 11.0f;
                    bool rg = false;
                    {
                        ItemRef sp2 = AG(ai.target).slot_pair(0);
                        if (sp2.item >= 0 && g_items[sp2.item].is_ranged())
                            rg = true;
                    }
                    if (ai.melee_charge < 0.0f || rg) {
                        ai.melee_charge = 0.0f;
                        ai.circle_mode = 0;
                    }
                    if (ai.circle_mode_prev != 1) {
                        ai.circle_mode_prev = ai.circle_mode;
                        a_sub3(q, acc[0] * 4.0f, acc[1]);
                        q.normalize();
                        float r16 = 16.0f;
                        vector4 d = q * 15.0f + pos;
                        set_goal_near(&d, &r16);
                    }
                }
            }
        }
    L_tail4:
        {
            ai.path_state |= 0x20;
            float s = 0.5f;
            ItemRef cp = cur_pair_fi();
            if (cp.item != -1)
                s = (float)g_items[cp.item].weapon_length * 0.01f + 0.9f;
            if (ai.path_state & 2) {
                ai.melee_standoff = s;
                ai.melee_standoff_low = s - 0.4f;
            } else {
                ai.melee_standoff = 0.5f;
                ai.melee_standoff_low = 0.2f;
            }
        }
    } else if (mode_ == 5 && ai.combat_state != 1 && ai.combat_state != 3 && ai.combat_state != 4) {
        float chance = hit_chance((int*)&(const ItemRef&)slot_pair(0), this);
        vector4 q, w;
        float t80 = 500.0f;
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            vector4 d = g_mission->agents.get(e)->pos - pos;
            near_ = d.normalize();
        }
        if (ai.target >= 0 && g_mission->agents.get(ai.target)->state != 1) {
            ai.target = -1;
        }
        if (ai.target == -1) {
            int c = ai.action_target;
            if (c >= 0 && g_mission->agents.get(c)->state == 1) {
                ai.target = c;
            }
        }
        int tg = ai.target;
        float cur;
        if (tg >= 0) {
            vector4 d = g_mission->agents.get(tg)->pos - pos;
            cur = d.normalize();
        } else {
            cur = t80;
        }
        if ((near_ < 8.0f && cur > near_ + near_) || tg == -1)
            ai.target = e;
        float wsum[2];
        if (frand() < 0.2f || ai.target == -1 || g_mission->agents.get(ai.target)->state != 1) {
            collect_enemies(wsum);
            if (ai.nthreat > 0) {
                int c = (ai.threats[0].dist < 3.5f && g_mission->agents.get(ai.threats[0].id)->state == 1) ? ai.threats[0].id : pick_threat();
                if (c >= 0) {
                    ai.action_target = c;
                    ai.target = c;
                }
            }
        }
        int t3 = ai.target;
        if (t3 >= 0 && g_mission->agents.get(t3)->state == 1) {
            Agent* ta = g_mission->agents.get(t3);
            ai.move_mode = 0x22;
            vector4 d = ta->pos - pos;
            t80 = d.normalize();
            int bestid = -1;
            bool close99 = false;
            if (frand() + 1.5f > t80)
                ai.ta_interval = 0.0f;
            t80 = (ta->pos.z - pos.z) * 0.7f + t80;
            float one = 1.0f;
            float dd = fmax_ref(&t80, &one);
            if (cr->kind == 1 || (cr->kind == 2 && team_leader(this) != pool_id)) {
                close99 = true;
                hand_pos(&w);
                a_sub3(q, pos, w);
                wsum[0] = q.normalize();
            }
            t80 = chance * 0.7f;
            one = 30.0f;
            if ((fmin_ref(&one, &t80) < dd && !close99 && cr->kind != 3) ||
                (ai.always_attack_in_melee != 0 && !close99 && cr->kind != 3)) {
                q = ta->pos;
                bestid = ta->pool_id;
            } else {
                if (line_of_sight(ta->pool_id, 0) && fire_check(ta->pool_id, (int*)&t80) == 1 && (!close99 || wsum[0] < 3.0f)) {
                    reset_path();      // 0x432080 (the path state only; 93d4415c had rebound this to the spawn reset 0x472f90)
                    goto L_a79;
                }
                if (close99) {
                    q = w;
                } else {
                    w = d * 0.7f + pos;
                    q = w;
                }
            }
            if (frand() < 0.3f) {
                if (close99) {
                    wsum[0] = 2.0f;
                    goal_near_for_class(&q, &wsum[0]);
                } else if (bestid >= 0) {
                    int two = 2;
                    int face = g_mission->agents.get(bestid)->update_nav_face();
                    one = 1.0f;
                    pick_reachable_goal(&q, &one, &face, &two);
                } else {
                    wsum[0] = 1.8f;
                    set_goal_near(&q, &wsum[0]);
                }
            }
        }
    L_a79:
        ai.path_state |= 0x20;
        ai.melee_standoff = 1.5f;
    } else if (mode_ == 6) {
        hit_chance_ref(cur_pair_fi(), this);
        float t80 = 500.0f;
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            vector4 d = AG(e).pos - pos;
            near_ = d.normalize();
        }
        float cur;
        if (ai.target >= 0) {
            vector4 d = AG(ai.target).pos - pos;
            cur = d.normalize();
        } else {
            cur = t80;
        }
        if (near_ < 8.0f && cur > near_ + near_)
            ai.target = e;
        int r = rand() % 0x3dc9;
        int c;
        if (((float)r * (1.0f / 15817.0f) < 0.2f || ai.target == -1 ||
             g_mission->agents.get(ai.target)->state != 1) &&
            (collect_enemies(&t80), ai.nthreat > 0 && (c = (int)pick_threat()) >= 0)) {
            ai.action_target = c;
            ai.target = c;
        }
        if (ai.target == -1)
            ai.target = ai.action_target;
        if (horse == -1) {
            ai.ta_interval = 0.0f;
            return;
        }
        if (AG(ai.target).state != 1) {
            ai.target = -1;
            ai.ta_interval = 0.0f;
        } else {
            ai.move_mode = 0x22;
            int c3 = cr->kind;
            if (c3 != 3) {
                if (c3 == 1) {
                    if (frand() < 0.2f) {
                        vector4 h;
                        hand_pos(&h);
                        float r2 = 2.0f;
                        goal_near_for_class(&h, &r2);
                    }
                } else if (c3 == 2) {
                    if (frand() < 0.2f) {
                        vector4 h;
                        hand_pos(&h);
                        float r2 = 2.0f;
                        goal_near_for_class(&h, &r2);
                    }
                } else {
                    if (frand() < 0.5f) {
                        Agent* ta = g_mission->agents.get(ai.target);
                        vec2 v;
                        v.x = ta->pos.x - pos.x;
                        v.y = ta->pos.y - pos.y;
                        v.normalize();
                        vector4 gp;
                        float a = v.y * 13.0f;
                        float b = -v.x * 13.0f;
                        float cc = v.x * -20.0f;
                        float dd = v.y * -20.0f + ta->pos.y;
                        float r10 = 10.0f;
                        gp.x = (cc + ta->pos.x) + a;
                        gp.y = dd + b;
                        gp.z = pos.z + 100.0f;
                        set_goal_near(&gp, &r10);
                    }
                }
            }
        }
        ai.path_state |= 0x20;
        ai.melee_standoff = 1.5f;
    } else if (mode_ == 8) {
        flag_a = true;
        if (horse == -1) {
            ai.ta_interval = 0.0f;
            return;
        }
        if (cr->kind != 3 &&
            (!(ai.path_state & 1) || g_mission->agents.get(horse)->vel_xy.y < 1.0f || (ai.goal_face != ai.goal_face_done && ai.goal_face >= 0))) {
            if (g_mission->agents.get(horse)->vel_xy.y > 2.0f) {
                float rr = 15.0f;
                vector4 gp = vel * 25.0f + pos;
                set_goal_near(&gp, &rr);
            } else {
                float rr = 30.0f;
                set_goal_near(&pos, &rr);
            }
        }
        if (ai.path_flag != 0) {
            float r50 = 50.0f;
            set_goal_near(&pos, &r50);
        }
    } else if (mode_ == 7) {
        if (horse == -1) {
            ai.ta_interval = 0.0f;
            return;
        }
        ai.target = pick_shout_target(0x1026, 150.0f);
        if (ai.target == -1)
            ai.target = ai.action_target;
        unsigned u = ai.target;
        Agent* ta = g_mission->agents.get(u);
        vec2 v;
        v.x = ta->pos.x - pos.x;
        v.y = ta->pos.y - pos.y;
        float dist = v.normalize();
        float dotv = v.x * vel.x + v.y * vel.y;
        if (((dist < 10.0f && dotv < 0.0f) &&
             (g_mission->agents.get(u)->horse == -1 || frand() < 0.2f)) ||
            (dist < 2.0f && vel_xy.y < 1.0f)) {
            ai.simple_behavior = 8;
            ai.ta_interval = (((float)(rand() % 0x3dc9)) * 3.161156e-05f + 0.75f) * 2.0f;
        } else if (weapon_mode == 3 && dotv < 0.3f && dist < 10.0f) {
            ai.simple_behavior = 8;
            ai.ta_interval = (((float)(rand() % 0x3dc9)) * 3.161156e-05f + 0.75f) * 2.0f;
        }
        ai.goal_agent = ai.target;
        set_goal_agent(ai.target);
        if (cr->kind != 3) {
            if ((vel_xy.y > 2.0f && g_mission->agents.get(ai.target)->horse == -1 && dist < 11.0f) ||
                (f100_is_0x48_and_charged() && dotv > 0.2f)) {
                set_goal_agent(ai.target);
                ai.circle_mode = 0;
            } else if (ai.circle_mode != 1 || frand() < 0.5f) {
                float s = dist + 10.0f;
                ai.circle_mode = 1;
                float r30 = 30.0f;
                float m = fmin_ref(&s, &r30);
                float fr = frand();
                unsigned u2 = ai.target;
                float k = ((fr + 2.0f) * m) * 0.2f;
                AgentTable* tb = &g_mission->agents;
                vector4 q = k * tb->get(u2)->v56b4;
                vector4 w = q + tb->get(u2)->pos;
                vector4 r4 = tb->get(u2)->v56b4;
                r4.normalize();
                float f = r4.x * v.x + r4.y * v.y;
                if (f < 0.0f) {
                    f = f + 1.0f;
                    vector4 d = q * f;
                    q = d + tb->get(u2)->pos;
                    w = q;
                }
                float val = dist * 0.35f - 0.7f;
                float hi = 25.0f;
                float lo = 0.0f;
                float c = clamp(&val, &lo, &hi);
                set_goal_near(&w, &c);
            }
        }
        ai.path_state |= 0x20;
        if (ai.path_state & 2)
            ai.path_state |= 0x80;
        if (dist < 35.0f)
            ai.path_state |= 0x200;
        ai.move_mode = 2;
    } else if (mode_ == 9) {
        flag_b = true;
        vector4 q, d;
        hand_pos(&q);
        ai.action_point = q;
        ItemRef sp = slot_pair(0);
        a_sub3(d, q, pos);
        float dist = d.normalize();
        float m = 1000.0f;
        bool b99 = false;
        if (sp.item < 0 || !g_items[sp.item].is_ranged()) {
            if (ai.target == -1 || g_mission->agents.get(ai.target)->state != 1 || frand() < 0.2f)
                ai.target = pick_threat();
            b99 = true;
        } else if (ai.target == -1 || g_mission->agents.get(ai.target)->state != 1 || frand() < 0.2f) {
            ai.target = pick_random();
        }
        int tg = ai.target;
        if (tg >= 0) {
            vector4 d = g_mission->agents.get(tg)->pos - pos;
            m = d.normalize();
        }
        if (!b99 && tg >= 0 && dist < 7.0f && m < 4.0f) {
            set_goal_agent(tg);
        } else {
            float r2 = 0.2f;
            goal_near_for_class(&ai.action_point, &r2);
            ai.path_state |= 0x20;
            if (horse >= 0)
                ai.melee_standoff = 3.5f;
            else
                ai.melee_standoff = 2.5f;
        }
    } else if (mode_ == 0xb) {
        int p = g_mission->player_id;
        if (p >= 0) {
            Agent* pa = g_mission->agents.get(p);
            if (rgl::length(pos - pa->pos) < 1.2f) {
                ai.target = p;
                ai.move_mode = 2;
                goto done;
            }
        }
        ai.move_mode = 0;
        ai.target = -1;
    } else if (mode_ == 0xd) {
        ai.move_mode = 0;
        ai.target = -1;
        ai.melee_standoff = 3.5f;
        if (frand() < 0.0005f || prev >= 0) {
            float r15 = 15.0f;
            vector4 v = vel * 10.0f + pos;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 0xe) {
        ai.move_mode = 0;
        ai.target = -1;
        ai.melee_standoff = 3.0f;
        if (frand() < 0.3f) {
            float r15 = 15.0f;
            vector4 v = vel * 20.0f + pos;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 3) {
        reset_path();      // 0x432080
        if (vel_xy.y < 0.5f && horse != -1)
            g_input_flags |= 0x10000;
    } else if (mode_ == 2) {
        if (horse != -1) {
            if (ai.ta_interval > 1.0f)
                ai.ta_interval = 1.0f;
        } else {
            int c = ai.action_target;
            if (c != -1 && g_mission->agents.get(c)->state == 1) {
                Agent* ca = g_mission->agents.get(c);
                vector4 d = ca->pos - pos;
                float l = d.out_length();
                if (l > 0.99f) {
                    flag_a = true;
                    rgl::strategic_entity* b = ca->body;
                    vector4 q;
                    if (l > 2.0f) {
                        vector4 s(b->local.s.x, b->local.s.y, b->local.s.z);
                        q = a_vsub(ca->pos, s);
                        ai.path_state = 0x21;
                    } else {
                        vector4 s = b->local.s * 0.2f;
                        q = a_vsub(ca->pos, s);
                        ai.path_state = 1;
                    }
                    float r0 = 0.0f;
                    set_goal_near(&q, &r0);
                } else {
                    g_input_flags |= 0x20000;
                    flag_a = false;
                    ai.target = c;
                    ai.move_mode = 2;
                }
                ai.melee_standoff = 0.15f;
            }
        }
    }

done:
    if (g_mission->f6554 != 0 && pool_id == g_dialogs.partner_agent) {
        ai.simple_behavior = 0;
        flag_a = false;
        ai.target = g_mission->player_id;
        ai.move_mode = 2;
    }
    if (ai.simple_behavior == 0) {
        ai.path_state = 0;
        ai.path_flag = 0;
        ai.path_count = 0;
        ai.path_active = 0;
        ai.goal_changed = 0;
        ai.move_mode = 0;
    } else if (ai.simple_behavior == 0xc) {
        ai.path_state = 0;
        ai.path_flag = 0;
        ai.path_count = 0;
        ai.path_active = 0;
        ai.goal_changed = 0;
        ai.move_mode = 0;
        ai.target = -1;
        if ((float)(rand() % 0x3dc9) * (1.0f / 15817.0f) < 0.3f) {
            ai.target = pick_shout_target(0x26, 300.0f);
            if (ai.target >= 0)
                ai.ta_interval = 0.0f;
        }
        if ((float)(rand() % 0x3dc9) * (1.0f / 15817.0f) < 0.6f) {
            g_input_flags = g_input_flags & 0xffffff;
            g_input_flags = g_input_flags | 0x1000000;
        }
    }

    ItemRef cp = cur_pair_fi();
    if (flag_a && kind != 1) {
        float range = 30.0f;
        if (cp.item >= 0 && g_items[cp.item].is_ranged())
            range = hit_chance_ref(cp, this);
        int n = ai.nthreat;
        float best = 0.0f;
        int bestid = -1;
        if (n > 0)
            bestid = ai.threats[0].id;
        if (cp.item >= 0 && g_items[cp.item].is_ranged()) {
            bestid = pick_threat();
        } else {
            for (int i = 0; i < ai.nthreat; i++) {
                Threat* t = &ai.threats[i];
                float score = -0.01f;
                Agent* ta = &AG((unsigned)t->id);
                if (ta->state == 1) {
                    vector4 d = ta->pos - pos;
                    t->dir = d;
                    float len = t->dir.normalize();
                    t->dist = len;
                    if (len < range) {
                        score = 10.0f / (len + 1.0f) + a_dot3r(t->dir, body->local.f) * 0.1f;
                    } else {
                        if (horse == -1) {
                            ItemRef p1 = slot_pair_fi(1);
                            if (p1.item >= 0) {
                                ItemKind& k1 = g_items[slot_pair(1).item];
                                if ((char)k1.type_flags == itp_type_shield) {     // cmp cl, 7 (0x448bf6): the reference leaves the dead add eax,edx
                                    ItemRef p0 = ta->slot_pair(0);
                                    if (p0.item >= 0) {
                                        ItemKind* ik = &g_items[p0.item];
                                        if (ik->is_ranged() && (float)ik->shoot_speed * 2.1f > t->dist) {
                                            float inv = 1.0f / (t->dist + 10.0f);
                                            float one = 1.0f;
                                            score = fmin_ref(&one, &inv) * 2.5f;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (t->id == ai.target)
                        score = score * 1.5f;
                    if (t->id == (int)prev)
                        score = score + score;
                }
                if (score > best) {
                    best = score;
                    bestid = t->id;
                }
            }
        }
        if (bestid >= 0)
            ai.target = bestid;
        else
            ai.target = -1;
        ai.move_mode = (ai.target == -1) ? 0 : 2;
    }
    if (flag_b && kind == 0 && ai.target >= 0) {
        const vector4& tv = AG(ai.target).v56b4;
        vector4 ta_ = a_vadd(vector4(tv.x, tv.y, tv.z), AG(ai.target).pos);
        vector4 tb_ = a_vadd(vector4(v56b4.x, v56b4.y, v56b4.z), pos);
        vector4 d = a_vsub(ta_, tb_);
        float dist = d.normalize();
        float range = 3.0f;
        if (horse >= 0 && g_game->troops[troop].nibble(0x17) < 2)
            range = 15.0f;
        if (dist > range && has_weapon_for(2)) {
            set_weapon_mode(2);
        } else if (horse < 0 || !(v56b4.length() > 0.1f) ||
                   !has_weapon_for(3)) {
            if (has_weapon_for(1))
                set_weapon_mode(1);
        } else {
            set_weapon_mode(3);
        }
    }
    ai.tc_interval = 0;
}


// @ 0x4350c0
static __declspec(noinline) int slot_class_impl(int item, int flag)
{
    unsigned t = g_items[item].type_flags & 0xff;
    if (t == itp_type_bow) return 3;
    if (t == itp_type_crossbow) return 4;
    if (t == itp_type_thrown) return 5;
    if (t == itp_type_pistol || t == itp_type_musket) return 6;
    if (t == itp_type_polearm) return 2;
    if (t == itp_type_one_handed_wpn) return 0;
    return flag == 0;
}

struct a_v2 { float x, y; a_v2() {} a_v2(const float& a, const float& b) : x(a), y(b) {} a_v2 operator*(const float& k) const { return a_v2(k * x, k * y); } void operator*=(const float& k) { x *= k; y *= k; } };
static inline float a_clampr(const float& x, float lo, const float& hi) { return clamp(&x, &lo, &hi); }
static inline float a_len(float y, float x) { return (float)sqrt(x * x + y * y); }
static __forceinline void a_dec(float* v, int j, const float& dt) { v[j] = v[j] - (v[j] + 1.0f) * dt; }
static __forceinline void a_add2(float* o, const a_v2& u) { o[0] = u.x + o[0]; o[1] = u.y + o[1]; }
static inline float a_len2(const vec2& v) { return (float)sqrt(v.x * v.x + v.y * v.y); }
static inline bool a_bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }
static inline const vector4& a_pos(const Agent* a) { return a->pos; }
static inline void a_sub2(vec2& r, const vector4& a, const vector4& b) { r.x = a.x - b.x; r.y = a.y - b.y; }
static inline void a_clamp(float& v, float lo, float hi) { if (v < lo) v = lo; else if (v > hi) v = hi; }

// 0x4354d0 (game/agent/agent_queries.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) bool Agent::flag_0x400_or_0x114() const      // 0x4354d0
{
    bool r = false;
    if ((int)(anim[0].events & 0x1ffff) >= 100 && anim[0].cur >= 0 && (defs->data[anim[0].cur].flags & 0x400) != 0 || (int)(anim[1].events & 0x1ffff) >= 100)
        r = true;
    return r;
}

// @ 0x448eb0
void Agent::ai_update_control()
{
    float strafe;
    int mid;
    float angle;
    unsigned fid;

    {
    vector4 tgt = a_pos(this);                                   // movement target
    if (floor_face == 0)
        return;

    // ---- 1. keep the path / goal up to date ----------------------------------------------------------------------
    if (ai.path_active != 0) {
        if (ai.path_state & 2) {
            const vector4* tp = &agent_by_id(ai.goal_agent)->pos;
            vector4 dd(tp->x - ai.goal.x, tp->y - ai.goal.y, tp->z - ai.goal.z);
            if ((float)sqrt(dd.y * dd.y + (dd.z * dd.z + dd.x * dd.x)) > 4.0f &&
                ai.goal_face != agent_by_id(ai.goal_agent)->update_nav_face()) {
                fid = ai.goal_agent;
                ai.goal = (&g_mission->agents)->get(fid)->pos;
                ai.goal_face = (&g_mission->agents)->get(fid)->update_nav_face();
                ai.goal_changed = 1;
                ai.path_active = 0;
            }
        }
        if (ai.path_active != 0)
            advance_path();
    }

    // ---- 2. pick the movement target -------------------------------------------------------------------------------
    if (ai.path_active != 0)
        tgt = ai.path_pos[ai.path_cur];
    if (ai.path_active == 0) {
        unsigned fl = ai.path_state;
        tgt = a_pos(this);
        if (fl != 0) {
            if ((fl & 2) != 0) {
                vector4 lv;
                const vector4& v = *agent_by_id(ai.goal_agent)->local_velocity(&lv);
                tgt = v * 0.8f + agent_by_id(ai.goal_agent)->pos;
                if ((char)fl < 0) {
                    float m = -1.0f;
                    float k = 0.4f;
                    if (f100_is_0x48_and_charged())
                        k = 0.25f;
                    if (anim[1].cur == 0xb9 || anim[1].cur == 0xb8)
                        m = 1.0f;
                    float s = m * k;
                    vec2 perp = {vel.y, -vel.x};
                    vec2 off = {perp.x * s, perp.y * s};
                    tgt.x += off.x;
                    tgt.y += off.y;
                }
                ai.goal = agent_by_id(ai.goal_agent)->pos;
                ai.goal_face = agent_by_id(ai.goal_agent)->update_nav_face();
            } else if ((fl & 1) != 0) {
                tgt = ai.goal;
            }
        }
        bool ok = false;
        if (ai.goal_changed != 0 && ai.goal_face != update_nav_face()) {
            if (plan_path()) {
                ai.path_active = 1;
                ok = true;
            } else {
                ai.path_active = 0;
            }
            ai.goal_changed = 0;
            if (ok) {
                advance_path();
                if (ai.path_active != 0)
                    tgt = ai.path_pos[ai.path_cur];
            }
        }
    }

    // ---- 3. heading jitter and obstacle avoidance -----------------------------------------------------------------------
    {
    {
        float base;
        if (this->kind == 0 && horse < 0) {
            int r = rand() % 0x3dc9;
            base = (float)r * 6.3223115e-06f + 0.88f;
        } else {
            int r = rand() % 0x3dc9;
            base = (float)r * 6.3223115e-06f + 0.85f;
        }
        ai.veer = base * ai.veer;
    }
    vector4 d = tgt - a_pos(this);                               // direction to the target
    float dlen = d.normalize();

    if ((flags3c & 1) && dlen > 0.1f) {
        if (obstacle == 0) {
            capsule cap;
            float a = 0.3f;
            float b = 1.0f;
            if (this->kind != 0 || horse != -1) {
                a = 0.5f;
                b = 2.0f;
            }
            if (b < dlen)
                b = dlen;
            if (b > 5.0f)
                b = 5.0f;
            cap.radius = 0.25f;
            cap.p0 = d * a + a_pos(this);
            cap.p1 = d * b + a_pos(this);
            cap.p0.z = cap.p0.z + 1.3f;
            cap.p1.z = cap.p1.z + 1.3f;
            vector4 bmin, bmax;
            cap.get_min(&bmin);
            cap.get_max(&bmax);
            shape_base* shapes[8193];
            int count;
            g_mission->scene->query_box(&bmin, &bmax, 2, shapes, &count);
            int n = count;
            shape_base* hit;
            int found = 0;
            contact contact;
            vector4 nrm;
            for (int i = 0; i < n; i++) {
                if (found != 0)
                    break;
                shape_base* sh = shapes[i];
                found = shape_contact_count(&cap, sh, 1, &contact);
                if (found != 0) {
                    nrm = contact.normal;
                    hit = sh;
                }
            }
            if (found != 0 && hit->shape_type() != 2) {
                vec2 nn = {-nrm.x, -nrm.y};
                if (d.x * nn.y - d.y * nn.x > 0.0f)
                    ai.veer = (-0.7f - ai.veer) * ((frand() + 1.0f) * 0.5f) + ai.veer;
                else
                    ai.veer = (0.7f - ai.veer) * ((frand() + 1.0f) * 0.5f) + ai.veer;
            }
            if (horse >= 0) {
                std::vector<rgl::entity*> ents;
                g_mission->scene->collect_boxed(&bmin, &bmax, &ents);
                std::vector<rgl::entity*>::iterator pe = ents.begin();
                if (pe < ents.end()) {
                    do {
                        rgl::entity* e = *pe;
                        if (e->shape != 0 && e->game_kind == 0xb && (unsigned)e->game_id != pool_id &&
                            (unsigned)e->game_id != (unsigned)horse && (unsigned)e->game_id != (unsigned)rider) {
                            Agent* o = agent_by_id(e->game_id);
                            if (o->kind == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                                rgl::strategic_entity* mb = body;
                                rgl::matrix m = mb->local;
                                int flag = 1;
                                float ang = 0.01f;
                                m.rotate_about_u(&ang);
                                const vector4& of = o->body->local.f;
                                float lhs = (float)fabs(m.f.y * of.y + m.f.z * of.z + m.f.x * of.x);
                                float rhs = (float)fabs(rgl::dot(mb->local.f, of));
                                if (rhs > lhs)
                                    flag = -1;
                                ai.veer = (float)flag * 0.1f + ai.veer;
                            }
                        }
                        ++pe;
                    } while (pe < ents.end());
                }
            }
        }
        // ---- speed-dependent weave ---------------------------------------------------------------------------------
        if (sqrt(v56b4.x * v56b4.x + (v56b4.z * v56b4.z + v56b4.y * v56b4.y)) < 0.3f) {
            if (horse >= 0) {
                if (!(ai.speed > 0.9f))
                    goto weave_up;
                if (ai.weave < -0.95f) {
                    ai.simple_behavior = 3;
                    ai.weave = ai.weave - 0.03f;
                } else {
                    ai.weave = ai.weave - 0.03f;
                }
            } else {
                if (this->kind != 0 || (ai.strafe_state & 1) == 0)
                    goto weave_up;
                ai.weave = ai.weave - 0.03f;
            }
        } else {
weave_up:
            ai.weave = ai.weave + 0.2f;
        }
        a_clamp(ai.weave, -1.0f, 1.0f);
        if (ai.weave < -0.1f) {
            if ((float)(g_ticks[weave_timer.clock] - weave_timer.start) * 1e-5f >= 0.0f) {
                ai.weave = 1.0f;
                ai.path_active = 0;
                ai.goal_changed = 1;
                weave_timer.start = g_ticks[weave_timer.clock] + 6000000;
            } else {
                if (ai.veer < 0.0f) {
                    int r = rand() % 0x3dc9;
                    ai.veer = ai.veer - (0.3f - ai.weave) * ((float)r * 6.3223117e-05f + 0.5f);
                } else {
                    int r = rand() % 0x3dc9;
                    ai.veer = (0.3f - ai.weave) * ((float)r * 6.3223117e-05f + 0.5f) + ai.veer;
                }
                int r = rand() % 0x3dc9;
                if ((float)r * 6.3223117e-05f < 0.01f)
                    ai.veer = -ai.veer;
            }
        }
    }
    if (ai.veer < -0.03f || 0.03f < ai.veer) {
        float va = ai.veer;
        float ca = (float)cos(va);
        float sa = (float)sin(va);
        float rx = ca * d.x - d.y * sa;
        float ry = sa * d.x + d.y * ca;
        tgt = vector4(rx, ry, d.z) * dlen + pos;
    }

    }

    // ---- 4. eye position and move direction ----------------------------------------------------------------------------
    {
    vector4 eye = pos;
    {
        float h;
        if (this->kind == 0 && horse != -1)
            h = 2.4f;
        else
            h = 1.5f;
        eye.z = h + eye.z;
    }
    float att = attack_speed();
    unsigned fl2 = ai.move_mode;
    if ((fl2 & 1) != 0) {
        ai.move_dir = ai.wp - eye;
    } else if ((fl2 & 2) == 0 || ai.target == 0xffffffff) {
        ai.move_dir = tgt - a_pos(this);
        if (ai.move_dir.length() < 0.001f)
            ai.move_dir = vel;
    } else {
        Agent* o = agent_by_id(ai.target);
        vector4 op = o->pos;
        float h;
        if (o->kind == 0 && o->horse != -1)
            h = 2.4f;
        else
            h = 1.5f;
        op.z = (h - 0.2f) + op.z;
        if (o->horse < 0 && o->f5610 == 0)
            goto lead;
        if ((fl2 & 0x20) == 0)
            goto lead_done;
        op.z = op.z - (float)(int)(ai.rand_bits & 2) * 0.8f;
lead:
        if ((fl2 & 0x20) != 0 && att > 0.1f) {
            vector4 dv = op - a_pos(this);          // from the agent's position, not the eye (0x449c44: fsub [edi], edi = &pos)
            float dxz = dv.x * dv.x + dv.z * dv.z;   // dy^2 is added last (0x449c69..0x449c7f); in one expression cl summed dz, dy, dx
            float dist = (float)sqrt(dxz + dv.y * dv.y);
            float s = 40.0f - (float)g_game->troops[troop].level;
            if (0.0f > s)
                s = 0.0f;
            float j = (frand() - 0.5f) * s * 0.01f + 1.0f;
            float k = (float)g_game->troops[troop].level * 0.02f + 0.6f;
            if (!(k < 1.0f))
                k = 1.0f;
            k = (dist / att) * k;
            vector4 off = k * o->v56b4 * j;
            op.x = off.x + op.x;
            op.y = off.y + op.y;
            op.z = off.z + op.z;
        }
lead_done:
        ai.move_dir = op - eye;
    }
    float mz = ai.move_dir.z;
    float mlen = ai.move_dir.normalize();
    if ((ai.move_mode & 0x20) != 0) {
        float s = 0.2f - (float)g_game->troops[troop].level * 0.005f;
        if (!(s > 0.0f))
            s = 0.0f;
        int r = rand() % 0x3dc9;
        float jit = ((float)r * 6.3223117e-05f - 0.5f) * s + 1.0f;
        float af = accel_factor(&att, &mlen, &mz);
        ai.move_dir.z = af * jit + ai.move_dir.z;
        ai.move_dir.normalize();
    }

    }

    // ---- 5. movement input bits ---------------------------------------------------------------------------------------
    mid = horse;
    {
    vector4 d = tgt - a_pos(this);
    strafe = 0.0f;
    ai.strafe.x = d.x;
    ai.strafe.y = d.y;
    angle = 0.0f;
    if (mid == -1 && this->kind == 0) {
        vec2 m;
        m.x = ai.move_dir.x;
        m.y = ai.move_dir.y;
        float ml2 = (float)sqrt(m.x * m.x + m.y * m.y);
        float nx, ny;
        if (ml2 > 0.0f) {
            float inv = 1.0f / ml2;
            nx = m.x * inv;
            ny = m.y * inv;
        } else {
            nx = 0.0f;
            ny = 1.0f;
        }
        vector4 dd = d;
        vec2 l = {dd.x * ny - dd.y * nx, dd.x * nx + dd.y * ny};
        float fa, fb;
        if (ai.path_active == 0) {
            fa = ai.melee_standoff * 1.4f;
            fb = ai.melee_standoff * 0.9f;
        } else {
            fa = 0.0f;
            fb = 0.0f;
        }
        float l2 = (float)sqrt(l.x * l.x + l.y * l.y);
        if (l2 > 0.0f) {
            float inv = 1.0f / l2;
            l.x = l.x * inv;
            l.y = l.y * inv;
        } else {
            l.x = 0.0f;
            l.y = 1.0f;
        }
        unsigned f = ai.strafe_state;
        vec2 mv;
        mv.x = 0.0f;
        mv.y = 0.0f;
        if (((f & 1) != 0 && l2 > fb) || ((f & 1) == 0 && l2 > fa)) {
            if (((f & 2) != 0 && l.y > 0.35f) || ((f & 2) == 0 && l.y > 0.45f)) {
                f |= 2;
                ai.in_prev.bits |= 1;
                mv.y = 1.0f;
            } else if (((f & 4) != 0 && l.y < -0.35f) || ((f & 4) == 0 && l.y < -0.45f)) {
                f |= 4;
                ai.in_prev.bits |= 2;
                mv.y = -1.0f;
            } else {
                f &= 0xfffffff9;
            }
            ai.strafe_state = f;
            if (((f & 8) != 0 && l.x > 0.35f) || ((f & 8) == 0 && l.x > 0.45f)) {
                f |= 8;
                ai.in_prev.bits |= 4;
                mv.x = 1.0f;
            } else if (((f & 0x10) != 0 && l.x < -0.35f) || ((f & 0x10) == 0 && l.x < -0.45f)) {
                f |= 0x10;
                ai.in_prev.bits |= 8;
                mv.x = -1.0f;
            } else {
                f &= 0xffffffe7;
            }
            ai.strafe_state = f;
            ai.strafe_state = f | 1;
            mv.normalize();
            strafe = l.x * mv.y - mv.x * l.y;
        } else {
            ai.strafe_state = 0;
        }
        if ((ai.path_state & 0x20) != 0 && ai.speed_limit > 2.6f)
            ai.in_prev.bits = ai.in_prev.bits | 0x200000;
    }
    if (mid != -1 || this->kind != 0) {
        // ---- follow / mount case ----
        vector4 gp = tgt;
        if (ai.path_active != 0)
            gp = ai.goal;
        d = gp - a_pos(this);
        float gdist = (float)sqrt(d.x * d.x + d.z * d.z + d.y * d.y);
        float hi = 0.8f, lo = 0.5f;
        if (ai.path_active == 0) {
            hi = ai.melee_standoff * 1.2f;
            lo = ai.melee_standoff * 0.8f;
        }
        if ((ai.strafe_state != 0 && gdist > lo) || (ai.strafe_state == 0 && gdist > hi)) {
            ai.strafe_state = 1;
            vector4 tt = tgt - a_pos(this);
            float h_y = vel.y;
            float h_x = vel.x;
            float hl = (float)sqrt(h_x * h_x + h_y * h_y);
            float hx, hy;
            if (hl > 0.0f) {
                float inv = 1.0f / hl;
                hx = h_x * inv;
                hy = h_y * inv;
            } else {
                hx = 0.0f;
                hy = 1.0f;
            }
            float tl = (float)sqrt(tt.x * tt.x + tt.y * tt.y);
            float tx, ty;
            if (tl > 0.0f) {
                float inv = 1.0f / tl;
                tx = tt.x * inv;
                ty = tt.y * inv;
            } else {
                tx = 0.0f;
                ty = 1.0f;
            }
            float a0 = (float)atan2(-(hy * tx - ty * hx), hy * ty + hx * tx);
            float ang = a0;
            float sp = gdist * 0.7f;
            if (sp < 1.0f)
                sp = 1.0f;
            else if (ai.speed_limit <= sp)
                sp = ai.speed_limit;
            ai.speed = sp;
            float gv = ai.goal_dir.length();
            if (gv > sp && d.z * ai.goal_dir.z + d.y * ai.goal_dir.y + d.x * ai.goal_dir.x > 0.1f)
                ai.speed = sp * 0.5f + gv;
            float other = turn_rate;
            if (mid != -1)
                other = agent_by_id(mid)->turn_rate;
            if (ang - other > 0.1f && ang > 0.1f)
                ai.in_prev.bits |= 0x80;
            else if (ang - other < -0.1f && ang < -0.1f)
                ai.in_prev.bits |= 0x40;
            if ((ai.turn_sign != 0 && ang > 0.1f) || (ai.turn_sign == 0 && ang > 0.2f)) {
                ai.turn_sign = 1;
                ai.speed = (0.9f - ang * 0.45f) * ai.speed;
            } else if ((ai.turn_sign != 0 && ang < -0.1f) || (ai.turn_sign == 0 && ang < -0.2f)) {
                ai.turn_sign = 1;
                ai.speed = (ang * 0.45f + 0.9f) * ai.speed;
            } else {
                ai.turn_sign = 0;
            }
            angle = ang;
            if (ai.speed < 1.2f)
                ai.speed = 1.2f;
            else if (ai.speed > ai.speed_limit)
                ai.speed = ai.speed_limit;
        } else {
            ai.speed = 0.0f;
            ai.strafe_state = 0;
        }
        unsigned who = pool_id;
        if (this->kind == 0)
            who = mid;
        if (agent_by_id(who)->vel_xy.y < ai.speed - 0.2f) {
            ai.in_prev.bits = ai.in_prev.bits | 0x10;
        } else if (ai.speed + 0.2f < agent_by_id(who)->vel_xy.y && (ai.path_state & 0x200) == 0) {
            ai.in_prev.bits = ai.in_prev.bits | 0x20;
        }
    }
    }
    }

    // ---- 6. combat state machine -----------------------------------------------------------------------------------------
    if (g_mission->f6554 == 0 && this->kind == 0 && ai.target != 0xffffffff && (ai.move_mode & 2) != 0) {
        if (!is_enemy(agent_by_id(ai.target)) || agent_by_id(ai.target)->state != 1)
            goto no_target;
        Agent* o = g_mission->agents.get(ai.target);
        vec2 tv;
        a_sub2(tv, a_pos(o), a_pos(this));
        float dist = tv.normalize();
        ItemRef p = slot_pair(0);
        ItemKind* wk;
        if (p.item == -1 || (wk = &g_items[p.item], !wk->is_ranged()))
            goto melee;
        {
        float hc, fm, zt, one;
        unsigned itype;

        // ---- ranged weapon ------------------------------------------------------------------------------------------------
        itype = wk->type_flags & 0xff;
        hc = hit_chance_ref(p, this);
        one = 1.0f;
        zt = (o->pos.z - a_pos(this).z) * 0.7f + dist;
        fm = fmax_ref(&zt, &one);
        if (ai.combat_state == 0) {
            if ((itype == itp_type_crossbow || itype == itp_type_pistol || itype == itp_type_musket) &&
                p.max_amount() == (int)(p.m4 & 0xff)) {
                ai.timer.start = g_ticks[ai.timer.clock];
                ai.combat_state = 4;
                goto bb72;
            }
            if (fm >= hc) {
                ai.combat_state = 7;
                goto bb72;
            }
            if (fm < hc && frand() < ((float)g_game->troops[troop].level + 1.0f) * 0.2f) {
                int blocker;
                int r = fire_check(ai.target, &blocker);
                ai.fire_result = r;
                if (r == 1) {
                    ai.combat_state = 1;
                    float x = (dist + 70.0f) * 0.01f;
                    float lo = 0.7f;
                    float hi = 1.2f;
                    float cl = clamp(&x, &lo, &hi);
                    float delay = (((frand() + 2.5f) * cl) * 35.0f) / (float)(g_game->troops[troop].level + 0x23);
                    if (itype == itp_type_thrown)
                        delay = delay * 0.5f;
                    ((Timer*)&ai.timer)->set_delay(-delay);
                    goto bb72;
                }
                if (r == 0) {
                    ai.last_hit = g_hit_shape;
                    int t = (int)pick_threat();
                    if (t < 0) {
                        ai.combat_state = 7;
                        goto bb72;
                    }
                    ai.target = t;
                    goto bb72;
                }
                if (r == 3) {
                    ai.combat_state = 8;
                } else {
                    ai.shot_failed = 2;
                    ai.ta_interval = 0;
                }
            }
            goto bb72;
        }
        if (ai.combat_state == 1) {
            // state 1
            ai.input = ai.input | 0x100;
            int m = horse;
            if (m == -1 && dist < 2.5f && (o->horse == m || o->v56b4.out_length() < 1.5f)) {
                ai.ta_interval = 0;
                ai.combat_state = 3;
            }
            if (m == -1 && (itype == itp_type_crossbow || itype == itp_type_bow) && ai.timer.elapsed() > -0.7f)
                ai.in_prev.bits = ai.in_prev.bits & 0xfffffff0;
            if (!(ai.timer.elapsed() > 0.0f))
                goto bb72;
            {
                int blocker;
                if (fire_check(ai.target, &blocker) == 1) {
                    ((Timer*)&ai.timer)->set_delay(-0.3f);
                    ai.combat_state = 3;
                    goto bb72;
                }
            }
            ai.input = ai.input | 0x200;
            goto bb68;
        }
        if (ai.combat_state == 3) {
            int m = horse;
            if (m == -1 && dist < 4.0f && (o->horse == m || o->v56b4.out_length() < 1.0f)) {
                ai.ta_interval = 0;
                ai.combat_state = 0;
            }
            if (m == -1 && (itype == itp_type_crossbow || itype == itp_type_bow))
                ai.in_prev.bits = ai.in_prev.bits & 0xfffffff0;
            if (ai.timer.elapsed() >= 0.0f)
                ai.combat_state = 0;
            goto bb72;
        }
        if (ai.combat_state == 4) {
            int m = horse;
            if (m == -1 && dist < 2.5f && (o->horse == m || o->v56b4.out_length() < 1.5f)) {
                ai.ta_interval = 0;
                ai.combat_state = 0;
            }
            ai.input = ai.input | 0x100;
            if (m == -1)
                ai.in_prev.bits = ai.in_prev.bits & 0xfffffff0;
            if (p.max_amount() != (int)(p.m4 & 0xff))
                ai.combat_state = 0;
            if (anim[1].cur != 0x7b && ai.timer.elapsed() > 1.0f)
                ai.combat_state = 0;
            goto bb72;
        }
        if (ai.combat_state == 7) {
            if (!(fm < hc))
                goto bb72;
            if (frand() < 0.03f) {
                int t = (int)pick_threat();
                if (t < 0)
                    goto bb72;
                ai.target = t;
            } else {
                if (!line_of_sight(ai.target, ai.last_hit)) {
                    ai.last_hit = g_hit_shape;
                    goto bb72;
                }
            }
            goto bb68;
        }
        if (ai.combat_state == 8) {
            AgentTable* at = &g_mission->agents;
            vec2 tv2;
            a_sub2(tv2, at->get(ai.target)->pos, a_pos(this));
            tv2.normalize();
            vec2 bf;
            bf.x = body->local.f.x;
            bf.y = body->local.f.y;
            float fx = bf.x;
            float fy = bf.y;
            if (horse == -1 || at->get(horse)->vel_xy.y > 0.5f)
                goto bb68;
            unsigned bits;
            if (fy * tv2.y + tv2.x * fx > 0.0f)
                bits = ai.in_prev.bits | 0x40;
            else
                bits = ai.in_prev.bits | 0x80;
            ai.in_prev.bits = bits;
            if (!(tv2.y * fx - fy * tv2.x > 0.3f))
                goto bb72;
            goto bb68;
        }
        goto bb72;
        }

melee:
        float reach0, facing, ofacing, wprof, r2f, distL;
        int idx;
        bool b3e, b3f, b58, b59, b5a, b5b;
        // ---- melee / no ranged weapon -----------------------------------------------------------------------------------
        reach0 = (frand() - 0.5f) * (float)(g_options.difficulty + 1) * 0.15f + 0.9f;
        facing = tv.y * vel.y + tv.x * vel.x;
        ofacing = -tv.x * o->vel.x + -tv.y * o->vel.y;
        {
            float r1 = frand() * 0.2f + 1.0f;
            const vector4& V = v56a4;
            const vector4& OP = o->pos;
            float outlen = (o->v56a4 * r1 + OP - (V * r1 + a_pos(this))).out_length();
            idx = p.item;
            if (idx < 0)
                idx = 0;
            wk = &g_items[idx];
            int mc;
            {
            ItemRef q = slot_pair(1);
            b3f = false;
            wprof = 50.0f;
            if (idx != 0) {
                int cls = slot_class_impl(p.item, slot_pair(1).item >= 0);
                if (cls >= 0 && cls < 7)
                    wprof = g_game->troops[troop].stat[cls];
                reach0 = (100.0f / (float)p.speed_rating()) * reach0;
            }
            if (ai.combat_state == 3 && active == 1)
                reach0 = reach0 * 0.5f;
            vector4 l2 = o->v56a4 * reach0 + OP;
            float dl = rgl::length(l2 - a_pos(this));
            float reach2 = rgl::length(V) * reach0;
            mc = horse;
            float distv;
            if (mc >= 0 && !(rgl::length(l2 - (V * reach0 + a_pos(this))) < dl - reach2 * 0.5f))
                distv = rgl::length(OP - a_pos(this)) + 0.2f;
            else
                distv = dl - reach2;
            distL = distv;
            if (q.item >= 0 && (g_items[q.item].type_flags & 0xff) == itp_type_shield)
                b3f = true;
            }
            float r2base = 2.5f;
            float reachf = 2.5f;
            if (p.item != -1)
                reachf = (float)g_items[p.item].weapon_length * 0.01f + 1.1f;
            if (mc >= 0)
                reachf = reachf - 0.3f;
            if (o->horse >= 0)
                reachf = reachf + 0.7f;
            ItemRef op0 = o->slot_pair(0);
            if (op0.item != -1)
                r2base = (float)g_items[op0.item].weapon_length * 0.01f + 1.9f;
            r2f = r2base * 1.3f;
            b5b = false;
            {
            ItemRef op1 = o->slot_pair(1);
            b3e = false;
            b5a = distL < reachf;
            b58 = (ofacing > 0.0f) && (distL < r2f);
            b59 = false;
            if ((char)wk->type_flags == 4 && facing > 0.4f && anim[1].cur == 0x47 && outlen < 19.0f)
                b59 = true;
            if (f100_is_0x48_and_charged() && facing > 0.4f)
                b3e = true;
            if (op1.item >= 0 && (g_items[op1.item].type_flags & 0xff) == itp_type_shield)
                b5b = true;
            }
            int st = ai.combat_state;
            if (st <= 1) {
                if (b3f && op0.item != -1 && !b3e && !b59) {
                    ai.input = ai.input | 0x200;
                } else if (outlen < 2.8f) {
                    if (g_game->troops[troop].level >= 0x14 && (b3f || p.item == -1) && !b59 && !b3e)
                        ai.input = ai.input | 0x200;
                }
                if (flag_0x400_or_0x114()) {
                    if (this->kind == 0 && horse == -1)
                        ai.in_prev.bits = (ai.in_prev.bits & 0xfffffffe) | 2;
                    if (b3f) {
                        {
                            float fr = frand();      // one rand call, doubled (0x44b4a0 fadd st0,st0)
                            ((Timer*)&ai.timer2)->set_delay(-((fr + fr) + 1.3f));
                        }
                        ai.combat_state = 7;
                    }
                    goto bb72;
                }
                if (b5a && !b3e && (frand() < wprof * 0.004f + 0.5f || horse >= 0)) {
                    ((Timer*)&ai.timer)->set_delay(0.0f);
                    ai.combat_state = 3;
                    goto bb72;
                }
                if (b3f) {
                    if (op0.item == -1)
                        goto b53e;
                    if (!b3e && g_items[op0.item].is_melee() && ai.timer2.elapsed() > 0.0f &&
                        ((o->active == 1 && frand() < 0.3f - (float)g_game->troops[troop].level * 0.01f) ||
                         (o->active == 2 && frand() < (float)g_game->troops[troop].level * 0.05f + 0.5f)) &&
                        b58) {
b508:
                        {
                            float fr = frand();      // one rand call, doubled (0x44b4a0 fadd st0,st0)
                            ((Timer*)&ai.timer2)->set_delay(-((fr + fr) + 1.3f));
                        }
                        ai.combat_state = 7;
                        goto bb72;
                    }
                }
                if (op0.item == -1) {
b53e:
                    if (p.item == -1 && ai.timer2.elapsed() > 0.0f && ofacing > 0.0f &&
                        ((o->active == 1 && frand() < (float)g_game->troops[troop].level * 0.01f + 0.5f) ||
                         (o->active == 2 && frand() < (float)g_game->troops[troop].level * 0.05f + 0.5f)) &&
                        outlen < r2f)
                        goto b508;
                }
                if (!b3f && !b3e && wk->is_melee() && (wk->type_flags & 0x4000) == 0 && ai.timer2.elapsed() > 0.0f &&
                    op0.item != -1 && g_items[op0.item].is_melee() && o->active != 0 && b58 &&
                    frand() < wprof * 0.01f + 0.1f) {
                    ((Timer*)&ai.timer2)->set_delay(-(frand() * 0.5f + 0.6f));
                    ai.combat_state = 2;
                    goto bb72;
                }
                if (dist < 15.0f && frand() < 0.005f && horse == -1 && (g_mission->flags & 1) == 0 &&
                    (!(frand() < 0.2f) || (g_mission->flags & 1) != 0)) {
                    int kind = 4;
                    play_class_sound(&kind);
                }
                goto bb72;
            }
            if (st == 2) {
                ai.input = ai.input | 0x200;
                if (ai.timer2.elapsed() > 0.0f)
                    ai.combat_state = 0;
                if (o->active != 0 || !(frand() < 0.9f))
                    goto bb72;
                if (frand() < 0.4f) {
                    ((Timer*)&ai.timer)->set_delay(-0.2f);
                    ai.combat_state = 3;
                    goto bb72;
                }
                goto bb68;
            }
            if (st == 3) {
                if (frand() < 0.02f && horse == -1 && (g_mission->flags & 1) == 0) {
                    int kind = 4;
                    play_class_sound(&kind);
                }
                unsigned fl = ai.input;
                int fa = active;
                ai.input = fl | 0x100;
                if (fa <= 4 && ai.timer.elapsed() > 0.0f && fa == 1) {
                    if (b5a) {
                        int sv = p.speed_rating();
                        int sk = g_game->troops[troop].level + 0x28;
                        ((Timer*)&ai.timer)->set_delay(-((4.9f / (float)sk + (float)g_options.difficulty * 0.25f + 0.6f) * (100.0f / (float)sv)));
                        ai.combat_state = 4;
                    } else if (ai.timer.elapsed() > 8.0f || reach0 * 4.0f < distL) {
                        ai.input = fl | 0x300;
                        ai.combat_state = 0;
                    }
                }
                if (flag_0x400_or_0x114())
                    goto bb68;
                goto bb72;
            }
            if (st == 4) {
                if (ai.timer.elapsed() > 0.0f) {
                    ai.combat_state = 0;
                } else if (in_id_set(anim[1].cur)) {
                    ai.combat_state = 0;
                } else {
                    if (b5b && o->f5610 != 0 && ofacing > 0.0f) {
                        if (frand() < 0.03f)
                            ai.combat_state = 0;
                    } else if (g_items[op0.item].is_melee() && !b5b && ofacing > 0.0f) {
                        float zero[3];
                        zero[0] = 0.0f;
                        zero[1] = 0.0f;
                        zero[2] = 0.0f;
                        if (can_hit(pool_id, ai.target, 0, 0, zero) == 0) {
                            if (wprof > 100.0f && frand() < 0.03f)
                                ai.combat_state = 0;
                        } else if (frand() < ((4.0f - (float)g_options.difficulty) * wprof) * 0.0005f) {
                            ai.combat_state = 0;
                        }
                    }
                }
                if (flag_0x400_or_0x114())
                    goto bb68;
                goto bb72;
            }
            if (st == 7) {
                ai.input = ai.input | 0x200;
                if ((op0.item == -1 || g_items[op0.item].is_melee()) && o->active == 0) {
                    if (frand() < 0.4f) {
                        ((Timer*)&ai.timer2)->set_delay(0.0f);
                        ((Timer*)&ai.timer)->set_delay(-0.2f);
                        ai.combat_state = 3;
                    } else {
                        ((Timer*)&ai.timer2)->set_delay(0.0f);
                        ai.combat_state = 0;
                    }
                }
                if (ai.timer2.elapsed() > 0.0f)
                    goto bb68;
                goto bb72;
            }
            goto bb68;
        }
    } else {
no_target:
        if ((g_input_mask | 0x100) != 0 && (g_input_mask | 0x200) == 0)
            ai.input = ai.input | 0x300;
    }
bb68:
    ai.combat_state = 0;
bb72:
    ai.in_prev.move = ai.move_dir;
    if ((!(strafe < -0.001f) || !(strafe < 0.001f)) && (ai.in_prev.bits & 0xf) != 0 &&
        ((ai.input | g_input_mask) & 0x100) == 0 && ai.combat_state != 4) {
        double a = -asin((double)strafe);
        float c = (float)cos(a);
        double s = sin(a);
        float py = ai.in_prev.move.y;
        ai.in_prev.move.y = (float)(s * ai.in_prev.move.x + c * ai.in_prev.move.y);
        ai.in_prev.move.x = (float)(c * ai.in_prev.move.x - s * py);
    }
    ai.in_prev.move.normalize();
}


// ---- Agent::cycle_slot: advance slots[which] to the next usable held item -------------------------------------------------
// @ 0x44bc30
void Agent::cycle_slot(int which)
{
    bool found = false;
    int old_item = -1;
    int old = slots[which];
    if (old >= 0)
        old_item = pairs[old].item;
    if (which == 0 || slots[0] < 0 || slots[1] < 0 || slots[4] == 0) {
        int n = 0;
        do {
            if (n >= 4)
                break;
            int v = (slots[which] + 1) % 4;
            slots[which] = v;
            if (which == 0 || slots[1] != slots[0]) {
                int it = pairs[v].item;
                if (it >= 0) {
                    ItemKind* ik = &g_items[it];
                    if (which == 0) {
                        unsigned fl = ik->type_flags;
                        if ((char)fl == 10) {
                            ItemRef* p = &pairs[v];
                            int cnt = p->m4 & 0xff;
                            int val = p->max_amount();
                            if (val - cnt > 0 && pairs[slots[0]].item != old_item) {
                                if (horse < 0 || !(ik->type_flags & 0x10000000))
                                    found = true;
                            }
                        } else if (fl & 0x400000) {
                            if (horse < 0 || !(fl & 0x10000000))
                                found = true;
                        }
                    } else {
                        unsigned fl = ik->type_flags;
                        unsigned t = fl & 0xff;
                        // stand-in: the original keeps the second t == 7 compare (cmp eax,7); written plainly cl folds it into
                        // the first, (t ^ 7) == 0 survives to codegen as xor eax,7 (one byte off), t - 7 == 0 as add eax,-7
                        if ((t == 7 || ((fl & 0x800000) && (t ^ 7) == 0)) &&
                            (horse < 0 || !(fl & 0x10000000)) && (int)(pairs[v].m4 & 0xffff) > 0)
                            found = true;
                    }
                }
            }
            n++;
        } while (!found);
        if (which != 0 && old >= slots[which])
            slots[which] = -1;
        if (!found)
            slots[which] = -1;
    } else {
        slots[0] = -1;
    }
    slots[4] = 0;
    if (slots[0] >= 0) {
        int it = pairs[slots[0]].item;
        if (it >= 0 && (g_items[it].type_flags & 0x200000))
            slots[4] = 1;
    }
    update_equipment_display();
    on_wield_changed();
    fr1.o.x = fr2.o.x = -20000000.0f;
}


// ---- Mission::spawn_missile ---------------------------------------------------------------------------------
// the missile record of an id: pages of 16 records of 0xb0 bytes at Mission+0x30 (Pool<Missile>, engine/core/pool.h)
static __forceinline Agent& mag(Mission* m, unsigned id) { return m->agents.pages[id >> 4][id & 0xf]; }
static __forceinline Missile& mrec(Pool<Missile>* p, unsigned id)
{
    return p->pages[id >> 4][id & 0xf];   // the real operator[] per use (its dead add reg,edi stays)
}
#define MS(id) mrec(&poolJ, id)
static __forceinline void set_fbc_ref(rgl::entity* e, const float& v) { e->set_fbc(&v); }
// @ 0x44be50
int Mission::spawn_missile(const vector4* pos, const vector4* dir, ItemRef* launcher, ItemRef* ammo,
                           unsigned shooter_id)
{
    Agent* shooter = &(mag(this, shooter_id));
    float mount_f;
    if (shooter->horse >= 0)
        mount_f = (float)cos(shooter->cur) * (mag(this, shooter->horse)).vel_xy.y;
    else
        mount_f = shooter->vel_xy.y;
    int id = poolJ.alloc();
    rgl::entity* e = scene->create_entity(9);
    rgl::matrix ident;
    identity_pos(&ident);
    rgl::mesh* m;
    // a launcher of type 10 names the flying mesh itself (0x44bed6..0x44befd read the third argument); otherwise
    // the ammo item's own mesh (0x44bf04.. read the fourth)
    if ((g_items[launcher->item].type_flags & 0xff) == 10) {
        tstring tn = launcher->mesh_name();
        m = (rgl::mesh*)g_resmgr.get_object(&tn);
    } else {
        m = 0;
        if (ammo->item >= 0 && ammo->item < g_item_kind_count)
            m = g_items[ammo->item].mesh_for(0x2000000000000000ui64, ammo->b.mod);
        if (!m) {
            tstring name("flying_missile");
            m = (rgl::mesh*)g_resmgr.get_object(&name);
        }
    }
    if (m) {
        rgl::mesh* c = m->clone();
        c->local_frame = ident;
        e->add_mesh(c);
    }
    e->local.f = *dir;
    e->local.u = vector4(0.0f, 0.0f, 1.0f);
    e->local.orthonormalize();
    e->local.o = *pos;
    set_fbc_ref(e, 0.5f);                              // a const float& temporary, stored after the push
    e->center = vector4(0.0f, e->get_radius(), 0.0f);
    e->visible = 0;
    MS(id).ent = e;
    MS(id).pos = *pos;
    MS(id).prev = *pos;
    MS(id).vel = *dir;
    MS(id).f64 = (float)launcher->damage(1);
    unsigned lt = g_items[launcher->item].type_flags & 0xff;
    if (lt == 8 || lt == 9)
        MS(id).f64 += (float)ammo->damage(1);
    MS(id).f60 = shooter->attack_speed();
    MS(id).speed = MS(id).f60;
    MS(id).speed += mount_f;
    MS(id).shooter = shooter_id;
    MS(id).launcher = *launcher;
    MS(id).ammo = *ammo;
    MS(id).init();
    return id;
}

// ---- Mission::spawn_prop: instantiate scene-prop record props[*idx] as a world entity (meshes, collision shape, 3D sounds) ----
// views of tables of other objects (offsets read here): g_ptypes entries 0x48 bytes at +0x1c, count at +0x18; g_propkinds records
// flags +0x10, meshes +0x40, shape +0x50, sounds +0x6c; ItemKind sounds +0x228
static inline SimpleTriggerVec& isnd(ItemKind* k) { return k->sound; }

// mesh of an item for the prop display class, null for an invalid item index (bounds check inlined)
static inline rgl::mesh* item_mesh(int item, int packed)
{
    if (item < 0 || item >= g_item_kind_count)
        return 0;
    return g_items[item].mesh_for(0x3000000000000000ui64, packed >> 24);
}
int find_item(const tstring* name);                     // 0x434ad0 (cdecl)

// 0x4285e0 (engine/scene/shape_load.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) shape_base* body_part::clone_shape()      // 0x4285e0
{
    shape_base* r;
    if (shape->shape_type() == 0) {
        r = new sphere(*(sphere*)shape);
    } else if (shape->shape_type() == 1) {
        r = new capsule(*(capsule*)shape);
    } else if (shape->shape_type() == 2) {
        r = new face(*(face*)shape);
    } else if (shape->shape_type() == 3) {
        r = new composite(*(composite*)shape);
    } else if (shape->shape_type() == 4) {
        return ((manifold*)shape)->build_composite();
    }
    return r;
}

// @ 0x44c210
void Mission::spawn_prop(int* idx, int* id)
{
    PlacedObj* r = (PlacedObj*)&props[*idx];
    r->frame = r->start_frame;
    std::vector<rgl::mesh*> meshes;
    body_part* shape_res = 0;
    tstring name;
    tstring s2("0");
    int tbl = -1;
    int t = r->kind;
    if (t == 2) {
        int ii = find_item(&r->name);
        name = g_items[ii].mesh_name_for(0);
    } else if (t == 6) {
        int item = find_item(&r->name);
        int param = r->value;
        unsigned k = g_items[item].type_flags & 0xff;
        ItemRef ref;
        ref.item = item;
        ref.m4 = param;
        rgl::mesh* m;
        if (!((k == 2 || k == 3 || k == 4 || k == 6 || k == 5 || k == 0x12) && (m = item_mesh(ref.item, (int)ref.m4)) != 0))
            name = ref.mesh_name();
        else
            meshes.push_back(m);
    } else if (t == 4) {
        int ti = find_in_table20(&r->name);
        r->value = r->value % g_ptypes[ti].nframes;
        name = g_ptypes[ti].boxes[r->value].name;
        body_part* rs = g_ptypes[ti].boxes[r->value].res;
        if (rs)
            shape_res = rs;
    } else if (t == 7) {
        int ri = r->id;
        name = g_items[ri].mesh_name_for(r->value);
        s2.assign("boshield");
    } else if (t == 0) {
        tbl = find_in_table78(&r->name);
        if (tbl < 0)
            tbl = r->id;
        if (tbl < 0 || tbl >= g_propkind_count) {
            tbl = -1;
        } else {
            MissionObj* rec = &g_propkinds[tbl];
            if (g_cfg.enable_edit_mode != 0 || !(rec->flags & 0x4000))
                meshes = rec->meshes;
            shape_res = rec->shape;
        }
    } else if (t != 3) {
        if (t == 1) {
            if (g_cfg.enable_edit_mode)
                name.assign("entry_arrow");
            else
                name.assign("");
        } else if (t == 5) {
            if (g_cfg.enable_edit_mode)
                name.assign("_mesh_passage");
            else
                name.assign("");
            s2.assign("bo_mesh_passage");
        }
    }
after:
    if (name.len > 0)
        meshes = g_resmgr.find_meshes(&name);
    if (s2.differs("0") && s2.len != 0)
        shape_res = (body_part*)g_resmgr.get_object60(s2);

    if (r->kind == 7) {
        r->id = find_item(&r->name);
        rgl::dynamic_entity* e = static_cast<rgl::dynamic_entity*>(scene->create_entity(7));   // type 7: the rigid body
        for (unsigned i = 0; i < meshes.size(); i++)
            e->add_mesh(meshes[i]->clone());
        if (shape_res)
            e->shape = shape_res->clone_shape();
        static_cast<rgl::matrix&>(e->local) = r->start_frame;
        e->game_kind = 0xc;
        e->game_id = *idx;
        r->ent2 = e;
        e->mass = 2.0f;
        float a = (float)(rand() % 0x3dc9) * (float)(1.0 / 15817);
        float b = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817) + 1.0f;
        float c = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817);
        e->vel = a_vadd(a_vadd(vmul(c, e->local.f), vector4(0.0f, 0.0f, b)), vmul(a, e->local.u));
        e->restitution = 0.4f;
        float a2 = (float)(rand() % 0x3dc9) * (float)(9.0 / 15817);
        float c2 = (float)(rand() % 0x3dc9) * (float)(4.0 / 15817);
        float d = (float)(rand() % 0x3dc9) * (float)(6.0 / 15817);
        e->grounded = 0;
        e->angvel = a_vadd(a_vadd(vmul(d, r->frame.s), vmul(c2, r->frame.f)), vmul(a2, r->frame.u));
    } else {
        rgl::strategic_entity* e = static_cast<rgl::strategic_entity*>(scene->create_entity(9));   // type 9
        for (unsigned i = 0; i < meshes.size(); i++) {
            rgl::mesh* m = meshes[i]->clone();
            if (r->kind == 4) {
                vector4 dir, col;
                ((rgl::world*)scene)->light_at(r->frame.o, vector4(0.0f, 0.0f, 1.0f), &dir, &col, 1, 1.0f);
            }
            if (r->kind == 0 && tbl >= 0 && tbl < g_propkind_count && (g_propkinds[tbl].flags & 0x12000))
                m->mask = 0xffff;
            else
                m->mask = 0x1ffff;
            if (r->scale.z * r->scale.y * r->scale.x < 0.0f)
                m->mirrored = 1;
            e->add_mesh(m);
        }
        if (shape_res) {
            if (r->kind == 3 || r->kind == 5 ||
                (r->kind == 0 && tbl >= 0 && ((g_propkinds[tbl].flags & 0xff) == 5 || (g_propkinds[tbl].flags & 0xff) == 10)))
                e->shape = shape_res->copy_shape();
            shape_base* sh = shape_res->clone_shape();
            int v = 8;
            if (r->kind == 0 && tbl >= 0) {
                unsigned fl = g_propkinds[tbl].flags;
                if ((char)fl == 8)
                    v = 0x10c;
                else if (fl & 0x4000)
                    v = 0xc;
                else if (fl & 0x8000)
                    v = 0;
            }
            sh->set_m8(v);
            sh->scale(r->scale);
            sh->transform((const pos*)&r->frame);
            e->add_shape(sh);
        } else if (r->kind == 2) {
            r->id = find_item(&r->name);
            unsigned k = g_items[r->id].type_flags & 0xff;
            shape_base* sh;
            if (k == 1) {
                sh = (shape_base*)new capsule(0.4f, &vector4(0.0f, -0.3f, 1.05f), &vector4(0.0f, 0.67f, 1.15f));
            } else if (k == 0xb) {
                sh = (shape_base*)new sphere(vector4(0.0f, 0.0f, 0.0f), &(const float&)0.3f);
            } else {
                goto done_shape;
            }
            if (sh) {
                sh->scale(r->scale);
                sh->transform((const pos*)&r->frame);
                ((rgl::world*)scene)->reg_shape((shape_base*)sh);
            }
        } else if (r->kind == 6) {
            unsigned k = g_items[r->id].type_flags & 0xff;
            ItemKind* ik = &g_items[r->id];
            shape_base* sh;
            if (k == 1) {
                sh = (shape_base*)new capsule(0.4f, &vector4(0.0f, -0.3f, 1.05f), &vector4(0.0f, 0.67f, 1.15f));
            } else if (k == 0xb) {
                sh = (shape_base*)new sphere(vector4(0.0f, 0.0f, 0.0f), &(const float&)0.3f);
            } else if (ik->is_melee() || ik->is_ranged() || k == 7 || k == 5 || k == 6 || k == 0x12) {
                sh = (shape_base*)new sphere(vector4(0.0f, 0.0f, 0.0f), &(const float&)0.5f);
            } else {
                goto done_shape;
            }
            if (sh) {
                sh->scale(r->scale);
                e->shape = sh;
            }
        }
done_shape:
        e->set_local_scaled(&r->frame, &r->scale);
        r->m58 = r->frame;
        e->game_kind = 0xc;
        e->game_id = *idx;
        r->ent = e;
        prop_ents.push_back(e);
    }

    int ty = r->kind;
    if (ty == 0) {
        int ri = r->id;
        if (ri >= 0 && ri < g_propkind_count && g_propkinds[ri].triggers.count > 0) {
            SimpleTriggerVec* ss = &g_propkinds[ri].triggers;
            g_game->set_matrix(&r->frame);
            g_game->event_ctx.prop = *idx;
            rgl::entity* en = r->ent;
            if (!(en->meshes.size() > 0)) { g_game->event_ctx.mesh = 0; } else { g_game->event_ctx.mesh = en->meshes[0]; }
            g_game->event_ctx.key_a = 0;
            g_script.param1 = *idx;
            ss->update(-0x28);
        }
    } else if (ty == 2 || ty == 6 || ty == 7) {
        int ri = r->id;
        if (ri >= 0 && ri < g_item_kind_count && isnd(&g_items[ri]).count > 0) {
            SimpleTriggerVec* ss = &isnd(&g_items[ri]);
            int who = *id;
            int aid = -1;
            if (who >= 0)
                aid = mag(this, who).troop;
            g_game->event_ctx.agent = who;
            g_game->event_ctx.troop = aid;
            g_game->event_ctx.slot = -1000;
            rgl::entity* en = r->ent;
            if (en && en->meshes.size() > 0) {
                g_game->event_ctx.mesh = en->meshes[0];
            } else {
                rgl::entity* e2 = r->ent2;
                if (e2 && e2->meshes.size() > 0)
                    g_game->event_ctx.mesh = e2->meshes[0];
                else
                    g_game->event_ctx.mesh = 0;
            }
            g_game->event_ctx.key_a = 0;
            g_script.param1 = *id;
            g_script.param2 = aid;
            ss->update(-0x32);
        }
    }
    r->init_hp();
}

// ---- std::vector<PhysRec>::insert (0x44cf90) ----------------------------------------------------------------------------------
// @ 0x44cf90
template std::vector<PhysRec>::iterator std::vector<PhysRec>::insert(std::vector<PhysRec>::iterator, const PhysRec&);


// ---- Agent::choose_action: the AI decision (15 scored candidate actions) ------------------------------------------------------
#undef AG
#define AG(i) (g_mission->agents.pages[(unsigned)(i) >> 4][(unsigned)(i) & 0xf])   // unsigned index: shr, as the original
#define ZERO_S for (int zi = 0; zi < 15; zi++) s[zi] = 0.0f;
#define tf_hero 0x00000010i64
#define tf_guarantee_horse 0x01000000i64
static __forceinline char mode_flag1_i(const Mission* m) { return ((g_mission_templates[m->mode].flags >> 1) & 1) != 0; }
static __forceinline int team_leader_i(const Agent* a)     // team_leader 0x435b60, inlined in choose_action
{
    int t = a->team;
    if (t < 0 || t >= 8) t = 7;
    return g_mission->teams[t].leader;
}
static __forceinline float ca_dot(const vector4& a, const vector4& b) { return a.x * b.x + a.z * b.z + b.y * a.y; }   // the threat-loop dot product (the dir reference anchors the loop pointer at dir.y, as the original)
// @ 0x44d000
void Agent::choose_action(int unused)
{
    vector4 pv[15];                             // target position per kind
    int ids[15];                                // target id per kind
    ClassInfo* cr;
    float s[15];                                // score per kind
    unsigned cand;
    float dist;
    int cls = team;
    if (cls < 0 || cls >= 8) cls = 7;
    int kind = 0;
    if (troop >= 0) {
        if (horse >= 0)
            kind = 2;
        else
            kind = (g_game->troops[troop].flags30 & tf_guarantee_ranged) ? 1 : 0;
    }
    cr = &g_mission->teams[cls].cls[kind];

    ai.last_attacker = -1;
    ai.melee_standoff_low = 0;
    ai.melee_standoff = 1.5f;
    ZERO_S

    if (this->kind == 1) {
        if (ai.scripted_mode == 1) {
            ZERO_S
            s[1] = 10000.0f;
            pv[1] = ai.scripted_destination.o;
        } else if (dismount_state) {
            s[14] = 10000.0f;
        } else {
            s[13] = 10000.0f;
        }
    } else if (!(ai.alarm & 1)) {
        if (ai.scripted_mode == 1) {
            s[1] = 10.0f;
            pv[1] = ai.scripted_destination.o;
        }
        s[11] = (float)(rand() % 15817) * 9.4834677e-06f + 0.3f;
    } else {
        {
            vector4 hp;
            collect_enemies(&hp.x);
        }
        cand = pick_random();
        Mission* m = g_mission;
        bool target_mounted = false;
        int held_id = -1;
        dist = 1000.0f;
        if (cand != (unsigned)-1) {
            float dx = m->agents.pages[cand >> 4][cand & 0xf].pos.x - pos.x;
            float dy = m->agents.pages[cand >> 4][cand & 0xf].pos.y - pos.y;
            float dz = m->agents.pages[cand >> 4][cand & 0xf].pos.z - pos.z;
            dist = (float)sqrt(dx * dx + (dy * dy + dz * dz));
            ItemRef r = m->agents.pages[cand >> 4][cand & 0xf].cur_pair();
            held_id = r.item;
            target_mounted = m->agents.pages[cand >> 4][cand & 0xf].horse >= 0;
        }
        if (ai.scripted_mode == 1) {
            ZERO_S
            s[1] = 10.0f;
            pv[1] = ai.scripted_destination.o;
        } else {
            if (ai.nthreat) {
                float t = 5.0f / dist;
                if (t < 0.0f)
                    s[4] = 0.0f;
                else if (t >= 5.0f)
                    s[4] = 5.0f;
                else
                    s[4] = t;
                ids[4] = cand;
                if (!has_weapon_for(1) && !has_weapon_for(3)) s[4] = s[4] * 0.1f;
                if (has_weapon_for(2)) {
                    float t5 = dist * 0.06666667f;
                    if (t5 < 0.0f)
                        s[5] = 0.0f;
                    else if (t5 >= 2.5f)
                        s[5] = 2.5f;
                    else
                        s[5] = t5;
                    float t6 = dist * 0.07692308f;
                    if (!(t6 < 2.0f)) t6 = 2.0f;
                    s[6] = t6 * 1.5f;
                    if (g_game->troops[troop].flags30 & tf_guarantee_ranged) {
                        s[5] = s[5] + s[5];
                        s[6] = s[6] * 3.0f;
                    }
                    if (horse != -1) {
                        if (weapon_mode == 2)
                            s[6] = (frand() * 2.0f + 3.5f) * s[6];
                        else if (ai.weave < -0.5f)
                            s[6] = (frand() * 5.5f + 6.0f) * s[6];
                    } else {
                        s[6] = 0.0f;
                        if (weapon_mode == 2) s[5] = (frand() + 2.0f) * s[5];
                        if (target_mounted && held_id >= -1 && g_items[held_id].is_ranged())
                            s[5] = s[5] * 2.5f;
                    }
                    ids[5] = cand;
                    ids[6] = cand;
                }
                if (horse != -1) {
                    s[5] = s[5] * 0.2f;
                    s[4] = 0.0f;
                    if (has_weapon_for(3) || has_weapon_for(1)) {
                        int best = -1;
                        float bests = -100.0f;
                        for (int i = 0; i < ai.nthreat; i++) {
                            float w = 50.0f / (ai.threats[i].dist + 25.0f);
                            if (!(w < 1.0f)) w = 1.0f;
                            int n = agent_by_id(ai.threats[i].id)->targeted_by - 1;
                            w = w / ((float)(n < 0 ? 0 : n) + 1.0f);
                            float d = ca_dot(vel, ai.threats[i].dir);
                            if (d > 0.0f) w = (d * d * 20.0f + 1.0f) * w;
                            float sc = ((float)(rand() % 15817) * 6.3223117e-05f + 0.25f) * w * 0.5f;
                            if (agent_by_id(ai.threats[i].id)->weapon_flags & 0x10)
                                sc = sc * 1.5f;
                            else if (agent_by_id(ai.threats[i].id)->weapon_flags & 0x20)
                                sc = sc * 1.5f;
                            if (sc > bests) {
                                best = i;
                                bests = sc;
                            }
                        }
                        ids[7] = ai.threats[best].id;
                        s[7] = bests;
                    }
                }
            } else {
                if (mode_flag1_i(g_mission)) {
                    for (int i = 0; i < 8; i++) {
                        if (g_mission->teams[team].relation[i] == -1 &&
                            (g_mission->counts[i][5] > 0 || g_mission->counts[i][4] > 0)) {
                            s[12] = 1000.0f;
                            break;
                        }
                    }
                }
            }
            cls = team;
            if (cls < 0 || cls >= 8) cls = 7;
            int leader = g_mission->teams[cls].leader;
            if (this->kind == 0) {
                int k = cr->kind;
                if (k == 1 || (k == 2 && leader != pool_id)) {
                    if ((int)cand >= 0) {
                        vector4 d = vsub(AG(cand).pos, cr->pos);
                        if ((float)sqrt(d.z * d.z + d.x * d.x + d.y * d.y) > cr->half + cr->half && dist > 10.0f)
                            s[7] = s[7] * 0.0f;
                    }
                    if (k == 2 && leader != pool_id) {
                        vector4 hp;
                        hand_pos(&hp);
                        vector4 d = vsub(hp, pos);
                        float l = (float)sqrt(d.y * d.y + d.z * d.z + d.x * d.x) * 0.25f;
                        if (l < 0.15f)
                            l = 0.15f;
                        else if (l >= 4.0f)
                            l = 4.0f;
                        s[9] = (l + 1.0f);
                        pv[1] = AG(leader).pos;
                        pv[9] = AG(leader).pos;
                    } else if (k == 1) {
                        vector4 hp;
                        hand_pos(&hp);
                        vector4 d = vsub(hp, pos);
                        float l = (float)sqrt(d.y * d.y + d.z * d.z + d.x * d.x) * 0.25f;
                        if (l < 0.15f)
                            l = 0.15f;
                        else if (l >= 4.0f)
                            l = 4.0f;
                        s[1] = (l + 1.0f);
                        pv[1] = hp;
                    }
                    ids[9] = cand;
                    ids[1] = cand;
                } else if (cr->kind == 3) {
                    s[1] = 1.5f;
                    pv[1] = pos;
                    ids[1] = cand;
                }
                if (cr->k4 == 1 && horse == -1) {
                    Troop* T = &g_game->troops[troop];
                    unsigned __int64 fl = T->flags30;
                    if ((fl & tf_hero) || (fl & tf_guarantee_horse)) {
                        AgentTable* tbl = &g_mission->agents;
                        float best = 10000.0f;
                        int bestid = -1;
                        unsigned it = tbl->first_used();
                        unsigned cnt = tbl->count;
                        if (it < cnt) {
                            Agent* const* pg = &g_mission->agents.pages[0];
                            do {
                                Agent* a = &pg[it >> 4][it & 0xf];
                                if (a->kind == 1 && a->rider == -1 && a->f5830 == 2) {
                                    int v = a->item.difficulty();
                                    if (v <= T->nibble(0x18)) {
                                        vector4 d = vsub(a->pos, pos);
                                        float dd = (float)sqrt(d.y * d.y + d.z * d.z + d.x * d.x);
                                        if (dd < best) {
                                            best = dd;
                                            bestid = it;
                                        }
                                    }
                                }
                                tbl->next_used(&it);
                            } while (it < cnt);
                            if (bestid != -1 && best < 60.0f) {
                                s[2] = 2.2f;
                                ids[2] = bestid;
                            }
                        }
                        goto randomise;
                    }
                }
                if (cr->k4 == 2 && horse != -1 && dist > 5.0f) s[3] = 50.0f;
            }
        }
    }
randomise:
    for (int i = 0; i < 15; i++) s[i] = ((float)(rand() % 15817) * 1.2644623e-05f + 0.9f) * s[i];
    if (ai.shot_failed == 2) {
        s[5] = s[5] * 0.1f;
        s[6] = s[6] * 0.5f;
    }
    int best = 0;
    for (int i = 1; i < 15; i++)
        if (s[i] > s[best]) best = i;

    int saved = ai.simple_behavior;
    start_attack(best);
    ai.action_target = ids[best];
    ai.action_point = pv[best];
    ai.circle_mode = 0;
    ai.circle_mode_prev = -1;
    if (best == 11 || best != 1) {
        if (best == 5 || best == 6) {
            int t = pick_threat();
            if (t >= 0) ai.action_target = t;
        }
    } else {
        int k = cr->kind;
        if (k == 1 || (k == 2 && team_leader_i(this) != pool_id)) {
            float zero = 0.0f;
            goal_near_for_class(&ai.action_point, &zero);
        } else if (cr->kind == 3 && ai.scripted_mode != 1) {
            if (ai.fire_result == 0) {
                const vector4& f = body->local.f;
                vector4 p = a_vadd(vector4(f.x, f.y, f.z), pos);
                int fkind = 2;
                int face = update_nav_face();
                float speed = 2.0f;
                pick_reachable_goal(&p, &speed, &face, &fkind);
            }
        } else {
            float zero = 0.0f;
            set_goal_near(&ai.action_point, &zero);
        }
        ai.path_state |= 0x20;
        if (horse < 0 && this->kind != 1) {
            if (ai.scripted_mode != 0)
                ai.melee_standoff = 1.75f;
            else
                ai.melee_standoff = 1.0f;
        } else {
            ai.melee_standoff = 3.0f;
        }
    }
    ai.shot_failed = 0;
    if (ai.simple_behavior != saved) ai.tb_interval = 0;
}


// ---- Agent::ai_idle_tick: the per-frame timer block of the AI controller ----------------------------------------------------
// The three timer tests compare against Timer::elapsed() (0x432c10) inlined: the tick is re-read for the
// restart below, so the inlined form must not be commoned with it.
static __forceinline float tick_elapsed(const Timer& t)
{
    return (float)(g_ticks[t.clock] - t.start) * 1e-05f;
}

// @ 0x44df30
void Agent::ai_idle_tick(const float* dt)
{
    int alive = state;
    g_input_flags = 0;
    if (alive != 1) {
        ai.in_prev.bits = 0;
        return;
    }
    float v = this->ai.alertness - *dt * 0.002f;
    this->ai.alertness = v;
    if (v > 1.0f)
        this->ai.alertness = v - *dt * 0.05f;
    if (this->ai.alertness < 0.5f)
        this->ai.alertness = 0.5f;
    if (ai.action_countdown < 9.0f)
        ai.action_countdown = ai.action_countdown + *dt;
    if (ai.ta_interval < tick_elapsed(ai.ta)) {
        ai.ta.start = g_ticks[ai.ta.clock];
        ai.ta_interval = (float)(rand() % 0x3dc9) * 0.00012644623f + 5.0f;
        choose_action(0);
    }
    if (ai.tb_interval < tick_elapsed(ai.tb)) {
        ai.rand_bits = rand();
        update_ai();
        ai.tb.start = g_ticks[ai.tb.clock];
        ai.tb_interval = ((float)(rand() % 0x3dc9) * 6.322312e-05f + 0.5f) * g_dt_scale;
    }
    if (ai.tc_interval <= tick_elapsed(ai.tc)) {
        update_alarm();
        ai.in_prev.bits = 0;
        g_input_mask = ai.input;
        ai.input = 0;
        ai_update_control();
        ai.tc.start = g_ticks[ai.tc.clock];
        float r1 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        float hi = (float)g_options.difficulty * 0.02f + 0.2f / (float)(g_game->troops[troop].level + 10);
        ai.tc_interval = (r1 * 0.03f + hi) + 0.05f;
        InputRec e;
        e.bits = ai.input;
        float r2 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        float ga = (float)g_options.difficulty * 0.1f;
        float t = ga + (r2 + 1.0f) * 0.1f;
        if (troop >= 0)
            t = t / ((float)g_game->troops[troop].level + 30.0f) * 40.0f;
        e.time = (__int64)(t * 100000.0f + g_ticks[2]);
        ai.in_prev.bits = ai.in_prev.bits | g_input_flags;
        ai.input_q.push_back(e);
    }
}


// ---- Agent::shoot: fire a missile from the agent's hand ---------------------------------------------------------------------
// @ 0x44e2b0
void Agent::shoot()
{
    ItemRef curp = cur_pair_fi();
    ItemKind* ik = &g_items[curp.item];

    FxDef* an = body->fx;
    if (!an->pose_valid) {
        if (an->pose_src) an->update_poses(); else an->update_poses2();
    }
    const rgl::matrix* W = &an->elems[9].m84;
    vector4 P(W->o.x * 0.65f, W->o.y * 0.65f, W->o.z * 0.65f);
    an = body->fx;
    if (!an->pose_valid) {
        if (an->pose_src) an->update_poses(); else an->update_poses2();
    }
    const vector4* o19 = &an->elems[19].m84.o;
    const rgl::matrix* m = &body->local;
    float kx = o19->x * 0.35f + P.x;
    float ky = o19->y * 0.35f + P.y;
    float kz = o19->z * 0.35f + P.z;
    P.x = kz * m->u.x + ky * m->f.x + kx * m->s.x + m->o.x;
    P.y = kz * m->u.y + ky * m->f.y + kx * m->s.y + m->o.y;
    P.z = kz * m->u.z + ky * m->f.z + kx * m->s.z + m->o.z;
    P.x = W->f.x * 0.15f + P.x;
    P.y = W->f.y * 0.15f + P.y;
    P.z = W->f.z * 0.15f + P.z;

    vector4 d0 = vel;
    if (horse >= 0) {
        vector4 tmp;
        rotate_scale_xy(&tmp, &vel, &cur, &in_now.move);
        d0 = tmp;
    }
    float spr = aim_error;
    Mission* ms = g_mission;
    int tid = ai.target;
    if (ms->player_id != pool_id && tid >= 0) {
        ItemRef c2 = slot_pair_fi(1);
        float cls = (float)slot_class_impl(curp.item, c2.item >= 0);
        Agent* tg = &ms->agents.pages[tid >> 4][tid & 0xf];
        vector4 rel = a_vsub(tg->v56b4, v56b4);
        vector4 diff = a_vsub(tg->pos, pos);
        vector4 dn = diff;
        float dist = dn.normalize();
        vector4 q = a_vadd(diff, rel);
        q.normalize();
        float err = cls + 20.0f;
        float cx = dn.y * q.z - dn.z * q.y;
        float cy = dn.z * q.x - q.z * dn.x;
        float cz = q.y * dn.x - dn.y * q.x;
        float g5 = (float)g_options.difficulty * 0.5f;
        float cm = (float)sqrt(cx * cx + cz * cz + cy * cy);
        float rm = (float)sqrt(rel.x * rel.x + rel.z * rel.z + rel.y * rel.y);
        float A2 = err;
        float t2 = (g5 + 0.5f) * (((10.0f / (dist + 10.0f)) * (cm * rm)) * 0.3f / err);
        float t1 = (g5 + 1.0f) * (0.65f / A2);
        err = t1 + t2;
        if (err > 0.2f) err = 0.2f;
        if (ms->fog_far < 200.0f) err = dist / ms->fog_far * 0.05f + err;
        if (::is_night()) err = dist * 0.001f + err;
        d0.x = (0.5f - (float)(rand() % 0x3dc9) * 6.322312e-05f) * err + d0.x;
        d0.y = (0.5f - (float)(rand() % 0x3dc9) * 6.322312e-05f) * err + d0.y;
        d0.z = (0.5f - (float)(rand() % 0x3dc9) * 6.322312e-05f) * err + d0.z;
        d0.normalize();
    }
    unsigned mid;
    {
    int missing, total, slot_out;
    ItemRef ammo = best_ammo(-1, &missing, &total, &slot_out);
    vector4 shot;
    rand_cone_dir(&shot, d0, spr);
    ItemRef launcher = cur_pair_fi();
    mid = g_mission->spawn_missile(&P, &shot, &launcher, &ammo, pool_id);
    }
    unsigned __int64 shoot_bits = ik->flags & 0xff000ui64;
    if (shoot_bits == 0x30000ui64) {
        mrec(&g_mission->poolJ, mid).tumble = 6.0f;
        return;
    }
    if (shoot_bits == 0x20000ui64) {
        mrec(&g_mission->poolJ, mid).tumble = 12.0f;
        return;
    }
    mrec(&g_mission->poolJ, mid).tumble = 0.0f;
}
