//-----------------------------------------------------------------------------
// File: agent_targeting.cpp  (0x43a280..0x43e190)
//
// Desc: weapon-class names, hand position, body release, quiver, static body rebuild, light accumulator (0x43a280..0x43b100, the tail of the TU the old agent_stats.cpp began); agent-table iteration, nearest-enemy list, target pickers, aim-error update, input poll, nav-face tracking, move animation, animation blend update, path goals (0x43b100..0x43e190)
//       One part of what was game/agent4/agent_logic.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define ANIM_SLOT_SET_OUT_OF_LINE       // 0x4378a0 is a real call here (the body is game/agent/agent_stats.cpp's), written out at two sites of on_wield_changed
#define INT_HASH_LOWER_BOUND_INSTANTIATED_HERE   // glyph_hash.h: the glyph table's lookup 0x43b210 is this file's
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

//=============================================================================
// 0x43a280..0x43b100, from agent_stats.cpp
// the weapon class names, the std::vector / std::_Uninit_* instantiations, the level queries, the agent's quiver and the light accumulator
//=============================================================================
// inline vector helpers of the original: rgl's operators are out-of-line in other objects
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + (a.z * b.z + a.y * b.y); }

// @ 0x43a280
static __declspec(noinline) void ui_weapon_class_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

// the troop record's display name through the translation table (the record itself when absent)
// @ 0x43a480
tstring* Troop::display_name()
{
    tstring* t = g_strtable.find(&key);
    return t ? t : (tstring*)this;
}

// ---- <vector> helpers (VC7.1) --------------------------------------------------------------------------------------------------------------
// @ 0x43a4a0
template E12* std::vector<E12>::_Ufill(E12*, size_t, const E12&);
// @ 0x43a4d0
template tstring* std::vector<tstring>::_Ucopy(tstring*, tstring*, tstring*);
// @ 0x43a4f0
template const NewtonRagDollBone** std::vector<const NewtonRagDollBone*>::_Ucopy(std::vector<const NewtonRagDollBone*>::const_iterator, std::vector<const NewtonRagDollBone*>::const_iterator, const NewtonRagDollBone**);

// ---- the agent's hand position -----------------------------------------------------------------------------------------------------------------
// @ 0x43a510
void Agent::hand_pos(vector4* out)
{
    int c = team;
    if (c < 0 || c >= 8) c = 7;
    int h = 0;
    if (troop >= 0) {
        if (horse >= 0) h = 2;
        else h = (g_game->chars[troop].flags30 & 0x4000000i64) != 0 ? 1 : 0;
    }
    ClassInfo* r = &g_mission->teams[c].cls[h];
    float dy = r->dir.y;
    float ndx = -r->dir.x;
    bool left = false;
    float k = 1.0f;
    if (r->mode == 1) {
        k = 0.7f;
        if (class_rank & 1) left = true;
    } else if (r->mode == 2) {
        k = 1.0f;
    } else if (r->mode == 3) {
        k = 1.65f;
    } else if (r->mode == 4) {
        k = 2.5f;
    }
    if (horse != -1) k *= 1.6f;
    *out = r->pos;
    float px = r->dir.x * ai.hand_off.y;
    float py = r->dir.y * ai.hand_off.y;
    float nx = px * k + out->x;
    float ny = py * k + out->y;
    out->x = nx;
    out->y = ny;
    if (left) {
        out->x = nx - k * r->dir.x;
        out->y = ny - k * r->dir.y;
    }
    float pdy = dy * ai.hand_off.x;
    float pndx = ndx * ai.hand_off.x;
    float t8 = pdy * k;
    float x2 = t8 + out->x;
    out->x = x2;
    float y2 = pndx * k + out->y;
    float kk = k * 0.1f;
    out->y = y2;
    out->x = x2 + kk * ai.hand_off_z;
    float t4 = kk * ai.hand_off_w;
    out->y = y2 + t4;
    float height = x2;
    face* hit;
    g_mission->scene->floor_at(out, &height, &hit);
    out->z = height;
}

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

// remove the entity of the agent's body from the scene
// @ 0x43a6f0
void Agent::release_body()
{
    g_mission->scene->remove_entity(body->id);       // the level is the rgl::world (world.h rows +0x28 = level::cells)
    body = 0;
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

// first shape in the box of s (mask 0x8116, or 0x811e when *flag) that touches it; its contact point goes to *out_point
// @ 0x43a720
int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag)
{
    vector4 mn, mx;
    shape_base* found[0x2000];
    int n;
    contact c;
    s->get_min(&mn);
    s->get_max(&mx);
    unsigned mask = 0x8116;
    if (*flag)
        mask = 0x811e;
    g_mission->scene->query_box(&mn, &mx, mask, found, &n);
    int count = n;
    for (int i = 0; i < count; i++) {
        if (shape_contact_count(s, found[i], 1, &c) != 0) {
            *out_point = c.point;
            *out_shape = found[i];
            return 1;
        }
    }
    return 0;
}

// quiver display: the arrows left in each ammo slot drive the display mesh
// @ 0x43a800
void Agent::update_quiver()
{
    for (int i = 0; i < 4; i++) {
        ItemRef& pr = pairs[i];
        if (pr.item >= 0) {
            ItemKind* ik = &g_items[pr.item];
            if (((ik->type_flags & 0xff) == 5 || (ik->type_flags & 0xff) == 6 || (ik->type_flags & 0xff) == 10) && disp[i] != 0) {
                int used = pr.m4 & 0xff;
                int n = pr.max_amount() - used;
                if (links[i] != 0 && links[i]->mask != 0) n--;
                float v = (float)((n * 5 + 5) * 2);
                disp[i]->cap_all(&v);
            }
        }
    }
}

// ---- ragdoll / static physics bodies --------------------------------------------------------------------------------------------------------------------
// @ 0x43a8a0
bool PhysSet::none_sleeping(int idx)
{
    for (unsigned i = 0; i < recs[idx].bones.size(); i++) {
        if (NewtonBodyGetSleepingState(NewtonRagDollBoneGetBody(recs[idx].bones[i])))
            return false;
    }
    return true;
}
// @ 0x43a900
void PhysSet::rebuild_static_body(int idx)
{
    NewtonTreeCollisionEndBuild(recs[idx].collision, 0);
    recs[idx].body = NewtonCreateBody(recs[idx].world, recs[idx].collision);
    NewtonReleaseCollision(recs[idx].world, recs[idx].collision);
    matrix m;
    float* mp = (float*)identity_pos((pos*)&m);
    PhysRec& r = recs[idx];
    NewtonBodySetMatrix(r.body, mp);
}

// start sub-animation `cmd` on channel idx (phase continuity from the previous sub-animation's flags and offset)
// @ 0x43a980
void Agent::start_channel(int idx, SubAnim* cmd, unsigned* flags_io, float* base)
{
    float cur = body->fx->rec_shaped_phase(idx);
    float scale = cmd->scale;
    float len = 1.5f;
    if (cmd->flags & 0xff)
        len = (float)(int)((cmd->flags & 0xff) - 1) * 0.1f;
    if (anim[idx].events & 0x400000) {
        float inv = 1.0f / anim[idx].rate;
        scale = inv * scale;
        len = inv * len;
    }
    body->fx->env[idx].period = scale;
    body->fx->start_anim(idx, cmd->anim, cmd->a, cmd->b);
    body->fx->env[idx].attack = len + 0.01f;
    body->fx->reset_rec(idx);
    if (cmd->flags & 0x10000000)
        body->fx->env[idx].mode = 0x14;
    else
        body->fx->env[idx].mode = 0x15;
    if (cmd->flags & 0xa0000000) {
        float v = 0.0f;
        unsigned fl = *flags_io;
        if (fl & 0x40000000)
            v = -*base;
        else if (fl & 0x20000000)
            v = cur - *base;
        else if (fl & 0x80000000)
            v = -cur - *base;
        float res;
        if (cmd->flags & 0x20000000)
            res = v + cmd->offset;
        else
            res = cmd->offset - v;
        body->fx->set_rec_phase(idx, res);
    } else if (cmd->flags & 0x40000000) {
        body->fx->set_rec_phase(idx, cmd->offset);
    }
}

// std::_Uninit_fill_n for the 0x1c8-byte elements (catch funclet 0x43abbb: destroys [s, f) then `throw;`)
// @ 0x43ab50
template void std::_Uninit_fill_n(E1c8*, unsigned, const E1c8&, std::allocator<E1c8>&, std::_Nonscalar_ptr_iterator_tag);

// nearest nav face to pos by a depth-limited neighbour search from *hint
// @ 0x43abf0
int rgl::world::locate_near(const vector4* pos, int* hint, int* mode)
{
    int face = -1;
    float dist = 100000.0f;
    nav->next_stamp();
    nav_search(&pos->x, hint, mode, 0, &face, &dist);
    return face;
}

// ---- scene light accumulator ---------------------------------------------------------------------------------------------------------------------------
// @ 0x43ac40
void rgl::world::light_at(vector4 p, vector4 n, vector4* out_dir, vector4* out_col, unsigned flags, float scale)
{
    *out_dir = vector4(0.0f, 0.0f, 0.0f);
    *out_col = vector4(0.0f, 0.0f, 0.0f);
    std::vector<Light*>::iterator it = lights.begin();
    if (it < lights.end()) {
        unsigned skip_dir = flags & 2;
        do {
            if (skip_dir && (*it)->type == 2) goto next;
            if ((flags & 4) && (*it)->type != 2) goto next;
            {
                Light* l = *it;
                int type = l->type;
                double dist;
                vector4 d;
                float k;
                if (type == 2) {
                    dist = 1000.0f;
                    d = l->dir;
                    k = scale;
                } else {
                    d = vsub(p, l->pos);
                    dist = d.normalize();
                    k = 1.0f;
                }
                float dp = vdot(n, vneg(d));
                if (dp > 0.0f && (type == 2 || dist < l->range)) {
                    vector4 hp = l->pos;
                    if (type == 2) {
                        vector4 t = vmul(d, (float)dist);
                        hp = vsub(p, t);
                    } else {
                        k = (float)(1.0 / (dist * dist) * k);
                    }
                    bool nothing = (flags == 0);
                    if (nothing) {
                        vector4 p0 = vsub(p, vmul(d, 0.05f));
                        if (!trace_ray(&p0, &hp, 150.0f, 0, 0)) goto next;
                    }
                    l = *it;
                    if (l->flags & 1) k = k * 0.5f;
                    float t = dp * 0.9f + 0.1f;
                    float s = k * t;
                    *out_col += l->color * s;
                    l = *it;
                    float br = (l->color.z + l->color.y) + l->color.x;
                    *out_dir += d * k * t * 0.33f * br;
                }
            }
        next:
            it++;
        } while (it < lights.end());
    }
    float len = (float)sqrt(out_dir->x * out_dir->x + (out_dir->y * out_dir->y + out_dir->z * out_dir->z));
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        out_dir->x = inv * out_dir->x;
        out_dir->y = inv * out_dir->y;
        out_dir->z = inv * out_dir->z;
    } else {
        out_dir->x = 0.0f; out_dir->y = 1.0f; out_dir->z = 0.0f;
    }
}

//=============================================================================
// 0x43b100..0x43e190, from agent_ai.cpp
// Desc: the record table accessors, the agent table iteration, the int hash lookup, the nearest-enemy list and
//       target pickers, the aim-error update, the input queue, nav-face tracking and the movement animation
//       chooser, the animation blend update and the path goals. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

static __forceinline float clampf(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

enum { skl_horse_archery = 23, skl_riding = 24, skl_power_draw = 33, skl_power_throw = 34 };   // header_skills.py


// ---- the skins' blood effects (Animator g_anim: the skin table) --------------------------------------------------------------------------------------------------------------------
// @ 0x43b100
__declspec(noinline) int Animator::blood1(unsigned skin)
{
    if (skin < (unsigned)char_count) return skin_vec[skin].blood1;
    return 0;
}
// @ 0x43b130
__declspec(noinline) int Animator::blood2(unsigned skin)
{
    if (skin < (unsigned)char_count) return skin_vec[skin].blood2;
    return 0;
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

// stdext::hash_map<int, Glyph>::lower_bound: bucket (key ^ 0xdeadbeef) & mask, the iterator returned through a hidden pointer
// @ 0x43b210
template GlyphHash::iterator GlyphHash::lower_bound(const int&);

// @ 0x43b2b0
template tstring* std::vector<tstring>::_Ufill(tstring*, size_t, const tstring&);
// @ 0x43b2e0
template E1c8* std::vector<E1c8>::_Ucopy(E1c8*, E1c8*, E1c8*);

// ---- the agent's list of the 16 nearest enemies, sorted by distance (0x18-byte entries at agent+0x53ec) --------------------------------------------------------------
static __forceinline bool hostile_to(Mission* m, int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return m->teams[a].relation[b] == -1;
}
static __forceinline bool allied_to(Mission* m, int a, Agent* o)
{
    if (a < 0 || a >= 8) return false;
    int b = o->team;
    if (b < 0 || b >= 8) return false;
    return m->teams[a].relation[b] == 1;
}

// @ 0x43b300
void Agent::collect_enemies(float*)
{
    ai.nthreat = 0;
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned i = t->first_used();
    if (i < (unsigned)t->count) {
        do {
            Agent* o = &m->agents.pages[i >> 4][i & 0xf];
            if (o->kind == 0 && o->state == 1 && (int)i != pool_id && hostile_to(m, team, o)) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                int k = 0;
                int n = ai.nthreat;
                if (0 < n) {
                    while (k < ai.nthreat) {
                        if (!(ai.threats[k].dist < len)) break;
                        k++;
                    }
                    if (k >= 16) goto next;
                }
                if (n < 16) ai.nthreat = n + 1;
                for (int j = ai.nthreat - 1; j > k; j--) ai.threats[j] = ai.threats[j - 1];
                ai.threats[k].id = o->pool_id;
                ai.threats[k].dist = len;
                ai.threats[k].dir = d;
                m = g_mission;
            }
        next:
            t = &m->agents;
            t->next_used(&i);
        } while (i < (unsigned)t->count);
    }
}

// @ 0x43b4d0
unsigned Agent::pick_target()
{
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    int n = ai.nthreat;
    unsigned tid = (unsigned)-1;
    if (0 < n) {
        Threat* r = ai.threats;
        int k = n;
        do {
            tid = r->id;
            if (g_mission->agents.pages[tid >> 4][tid & 0xf].state == 1) {
                const vector4& p = g_mission->agents.pages[tid >> 4][tid & 0xf].pos;
                float dx = p.x - pos.x;
                float dy = p.y - pos.y;
                float dz = p.z - pos.z;
                float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                v = v / ((float)agent_by_id(r->id)->targeted_by + 7.0f);
                if (v > bestv) { bestv = v; best = tid; }
            }
            r++;
            k--;
            tid = best;
        } while (k != 0);
    }
    return tid;
}

// the ranged weapon classes (bow, crossbow, thrown, pistol, musket); a by-value helper parameter, not a named local
static __forceinline bool ranged_type(unsigned t) { return t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11; }
// @ 0x43b5d0
unsigned Agent::pick_random()
{
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned tid = (unsigned)-1;
    int n = 0;
    if (0 < ai.nthreat) {
        Threat* r = ai.threats;
        do {
            tid = r->id;
            if (agent_by_id(tid)->state == 1) {
                const vector4& p = agent_by_id(tid)->pos;
                float dx = p.x - pos.x;
                float dy = p.y - pos.y;
                float dz = p.z - pos.z;
                float v = 3.0f / (float)sqrt(dx * dx + (dy * dy + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                ItemRef cur = agent_by_id(r->id)->cur_pair();
                if (cur.item >= 0 && ranged_type(g_items[cur.item].type_flags & 0xff))
                    v = v * 1.5f;
                if (agent_by_id(r->id)->horse != -1) v = v * 0.85f;
                v = v / ((float)agent_by_id(r->id)->targeted_by + 4.0f);
                v = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * v;
                if (v > bestv) { bestv = v; best = r->id; }
            }
            n++;
            r++;
            tid = best;
        } while (n < ai.nthreat);
    }
    return tid;
}

// the agent's (or its mount's) velocity rotated by the body frame
// @ 0x43b7b0
vector4* Agent::local_velocity(vector4* out)
{
    unsigned idx = horse;
    vector4 v;
    if ((int)idx >= 0)
        *(vec2*)&v = g_mission->agents.pages[idx >> 4][idx & 0xf].vel_xy;
    else
        *(vec2*)&v = vel_xy;
    v.z = 0.0f;
    if (body) {
        const rgl::matrix& m = body->local;
        float rz = (v.y * m.f.z + v.x * m.s.z) + m.u.z * 0.0f;
        float ry = (v.y * m.f.y + v.x * m.s.y) + m.u.y * 0.0f;
        float rx = (v.y * m.f.x + v.x * m.s.x) + m.u.x * 0.0f;
        out->x = rx; out->y = ry; out->z = rz;
        return out;
    }
    *out = v;
    return out;
}

// @ 0x43b8a0
unsigned Agent::find_talk_target()
{
    unsigned best = (unsigned)-1;
    unsigned i = g_mission->agents.first_used();
    if (i < (unsigned)g_mission->agents.count) {
        do {
            Agent* o = &g_mission->agents.pages[i >> 4][i & 0xf];
            if (o->kind == 1 && o->rider == -1 && o->state == 1 && o->f5830 == 2) {
                if (o->item.difficulty() <= g_game->skill_level(troop, 0x18)) {
                    vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                    float len = d.normalize();
                    if (len < 1.0f) {
                        vec2 h = { in_now.move.x, in_now.move.y };
                        if (d.y * h.y + d.x * h.x > 0.1f)
                            best = i;
                    }
                }
            }
            g_mission->agents.next_used(&i);
        } while (i < (unsigned)g_mission->agents.count);
    }
    return best;
}

// @ 0x43b9e0
unsigned Agent::pick_front()
{
    unsigned best = (unsigned)-1;
    float bestv = 0.07f;
    unsigned i = g_mission->agents.first_used();
    unsigned n = g_mission->agents.count;
    if (i < n) {
        int myteam = team;
        do {
            Agent* o = &g_mission->agents.pages[i >> 4][i & 0xf];
            if (!allied_to(g_mission, myteam, o) && o->state == 1 && o->kind != 1) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * in_now.move.x + d.y * in_now.move.y) + 0.5f, 0.0f, 1.2f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                if (b * a > bestv) { best = i; bestv = b * a; }
            }
            g_mission->agents.next_used(&i);
        } while (i < n);
    }
    return best;
}

// @ 0x43bb80
unsigned Agent::pick_duel()
{
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned best = (unsigned)-1;
    float bestv = 0.0f;
    unsigned i = t->first_used();
    unsigned n = t->count;
    if (i < n) {
        std::vector<Agent*>::iterator pg = m->agents.pages.begin();
        int myteam = team;
        do {
            Agent* o = &pg[i >> 4][i & 0xf];
            if (hostile_to(g_mission, myteam, o) && o->state == 1 && o->active != 0) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * in_now.move.x + (d.y * in_now.move.y + d.z * in_now.move.z)) + 0.8f, 0.0f, 1.0f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                float od = -(d.x * o->in_now.move.x + (d.y * o->in_now.move.y + d.z * o->in_now.move.z));
                float c = clampf(od + 0.5f, 0.0f, 1.0f);
                float v = (c * b) * a;
                if (v > bestv) { best = i; bestv = v; }
            }
            g_mission->agents.next_used(&i);
        } while (i < n);
    }
    return best;
}

// ---- the aim error of a ranged agent ----------------------------------------------------------------------------------------------------------------------------------
// Notes on the spelling (all found by diffing against the original; none changes behaviour):
//  - `(0.0f > x) ? 0.0f : x` is the inlined max(0, x) the original used for the shot-error floor; the second
//    `if (acc < 0.0f)` is real source too (the original tests twice).
//  - the speed term is `spd * 0.0f` in the original (a tuning constant that is zero in 1.011).
//  - `sp` (mount-less speed) is a float: the original compares/adds it with single-precision constants.
//  - tm is Timer::elapsed() inlined at its first use; the later call in the crossbow/firearm arm stays a call.
inline __declspec(noinline) int ItemRef::accuracy()      // 0x435090
{
    if (item < 0) return 100;
    int v = g_items[item].leg_armor;
    if (v == 0) v = 99;
    return v;
}
static __forceinline float fmaxr(const float& a, const float& b) { return fmax_ref(&a, &b); }
// @ 0x43bdb0
void Agent::update_aim_error()
{
    vector4 n;
    float v = 0.0f;
    ItemRef cur = slot_pair_fi(0);
    int item = cur.item;
    if (item >= 0) {
        unsigned t = g_items[item].type_flags & 0xff;
        if (t == itp_type_bow || t == itp_type_crossbow || t == itp_type_thrown || t == itp_type_pistol || t == itp_type_musket) {
            float tm = (float)(g_ticks[timer.clock] - timer.start) * 1e-5f;
            float a, b, nz, acc;
            {
                vector4 p; p.x = tm; p.y = a = tm + 3.0f; p.z = b = tm * 1.5f;
                noise_gradient(0.7f, &p.x, &n.x);
                nz = (n.x + n.y) * 1.9f + 1.4f;
            }
            ItemKind* ik = &g_items[item];
            vector4 d = vsub(in_now.move, prev_facing);
            float spd = 0.0f;
            if (g_step_tbl[0] > 0.0f) spd = (float)sqrt(d.y * d.y + d.z * d.z + d.x * d.x) / g_step_tbl[0];
            spd = spd * 0.0f;
            if (spd > 1.0f) spd = 1.0f;
            prev_facing = in_now.move;
            unsigned t2 = ik->type_flags & 0xff;
            if (t2 == itp_type_bow) {
                float sk = (float)g_game->skill_level(troop, skl_power_draw) * 25.0f + (int)g_game->chars[troop].stat[3];
                float p21 = (float)cur.damage(1) * 8.0f;
                float w = (float)cur.difficulty() * 40.0f + p21 + 50.0f;
                int c = g_items[item].leg_armor;
                if (c == 0) c = 99;
                float cf = (float)c * 0.01f;
                float acc0 = ((w + 50.0f) - sk) * 6e-5f;
                acc = (0.0f > acc0) ? 0.0f : acc0;
                if (acc < 0.0f) acc = 0.0f;
                if (active) {
                    int ag = horse;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->agents;
                        if (1.0f < fabs(tb->get(ag)->vel_xy.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel_xy.y) + 3.0f - (sk + 200.0f) * (((float)g_game->skill_level(troop, skl_riding) + 5.0f) * (float)g_game->skill_level(troop, skl_horse_archery)) * 0.0004f) * 0.008f;
                            float zero = 0.0f;
                            float m = 4.0f / ((float)g_game->skill_level(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / (sk + 200.0f)) * nz * 7.0f + acc);
                    }
                    float lvl = 1.25f / damage_factor(0);
                    float g = (sk - w * 0.5f) * 0.01f;
                    g = (0.3f > g) ? 0.3f : g;
                    float thr = g + lvl;
                    v = acc;
                    if (tm < lvl) {
                        float zero = 0.0f;
                        float x = (lvl - tm) / lvl - sk * 0.001f;
                        v = fmax_ref(&zero, &x) + acc;
                    } else if (tm > thr) {
                        vector4 q; q.x = tm; q.y = a; q.z = b;
                        noise_gradient(0.4f, &q.x, &n.x);
                        float e1 = (tm - thr) * 0.4f;
                        float e2 = 1.0f;
                        float mn = fmin_ref(&e1, &e2);
                        b = (float)fabs(((n.x + n.y) + 1.0f) * 0.5f);
                        v = (w * 0.0006f) * (b * mn) + acc;
                    }
                    float r2 = (100.0f / (sk + 100.0f)) * spd - sk * 0.002f;
                    r2 = (0.0f > r2) ? 0.0f : r2;
                    v = (1.0f - cf) * 0.08f + (r2 * 0.2f + v);
                } else {
                    v = 0.0;
                }
            } else if (t2 != itp_type_crossbow && t2 != itp_type_pistol && t2 != itp_type_musket) {
                float sk = (float)g_game->skill_level(troop, skl_power_throw) * 20.0f + (int)g_game->chars[troop].stat[5];
                float cf = (float)cur.accuracy() * 0.01f;
                if (active) {
                    float p21 = (float)cur.damage(1);
                    float fv = (float)cur.difficulty() * 5.0f + p21;
                    acc = fmaxr(0.0f, ((fv + 30.0f) * 8.0f - sk) * 0.00012f);
                    int ag = horse;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->agents;
                        if (1.0f < fabs(tb->get(ag)->vel_xy.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel_xy.y) + 5.0f - (sk + 200.0f) * (((float)g_game->skill_level(troop, skl_riding) + 5.0f) * (float)g_game->skill_level(troop, skl_horse_archery)) * 0.0004f) * 0.004f;
                            float zero2 = 0.0f;
                            float m = 4.0f / ((float)g_game->skill_level(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero2, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / (sk + 50.0f)) * nz + acc);
                    }
                    float zero3 = 0.0f;
                    float y = (100.0f / (sk + 100.0f)) * spd - sk * 0.0007f;
                    v = (1.0f - cf) * 0.08f + (fmax_ref(&zero3, &y) * 0.3f + acc);
                } else {
                    v = 0.0;
                }
            } else {
                float cf = (float)cur.accuracy() * 0.01f;
                float A, B;
                if (t2 == itp_type_crossbow) {
                    A = (float)(int)g_game->chars[troop].stat[4];
                    B = (float)cur.damage(1);
                } else if (t2 == itp_type_pistol || t2 == itp_type_musket) {
                    A = (float)(int)g_game->chars[troop].stat[6];
                    B = (float)cur.damage(1);
                }
                if (active && (cur.max_amount() - (int)(cur.m4 & 0xff)) != 0) {
                    float zero = 0.0f;
                    float p1 = ((B + B + 100.0f) - A) * 0.00015f;
                    float q = A + 100.0f;
                    float p2 = 0.7f - timer.elapsed() * q * 0.01f;
                    float zero2 = 0.0f;
                    float r1 = fmax_ref(&zero, &p1);
                    acc = fmax_ref(&zero2, &p2) + r1;
                    int ag = horse;
                    if (ag >= 0) {
                        AgentTable* tb = &g_mission->agents;
                        if (1.0f < fabs(tb->get(ag)->vel_xy.y)) {
                            float t5 = ((float)fabs(tb->get(ag)->vel_xy.y) + 5.0f - (A + 150.0f) * (((float)g_game->skill_level(troop, skl_riding) + 5.0f) * ((float)g_game->skill_level(troop, skl_horse_archery) + 2.0f)) * 0.0004f) * 0.005f;
                            float zero3 = 0.0f;
                            float m = 4.0f / ((float)g_game->skill_level(troop, skl_horse_archery) + 4.0f);
                            acc = (float)(fmax_ref(&zero3, &t5) * m * nz + acc);
                        }
                    } else {
                        float sp = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
                        if (sp > 0.0f) acc = (((sp + 3.0f) / q) * nz * 3.0f + acc);
                    }
                    float zero4 = 0.0f;
                    float y = (100.0f / q) * spd - A * 0.004f;
                    v = (1.0f - cf) * 0.08f + (fmax_ref(&zero4, &y) * 0.15f + acc);
                } else {
                    v = 0.0;
                }
            }
        }
    }
    float rate = g_step_tbl[0] * 5.0f;
    if (!active) rate = g_step_tbl[0];
    if (aim_error < v) rate = rate + rate;
    if (rate > 1.0) rate = 1.0f;
    aim_error = rate * v + (1.0f - rate) * aim_error;
}

// ---- input queue, nav face tracking and the movement animation chooser -------------------------------------------------------------------------------------------------------
// @ 0x43c9c0
void Agent::poll_input(float unused, InputState* out)
{
    InputState orig = *out;
    *out = ai.in_prev;
    if (!ai.input_q.empty()) {
        __int64 now = g_ticks[2];
        while (!ai.input_q.empty()) {
            if (ai.input_q.front().time >= now)
                break;
            out->bits |= ai.input_q.front().bits;
            ai.in_last = *out;
            ai.input_q.pop_front();
        }
    }
    out->bits |= ai.in_last.bits;
    if ((orig.bits & 0x100) == 0 && (out->bits & 0x100) != 0)
        out->bits |= 0x400;
}

// @ 0x43cb30
int Agent::update_nav_face()
{
    vector4& np = nav_pos;
    vector4& pp = pos;
    vector4 d(np.x - pp.x, np.y - pp.y, np.z - pp.z);
    if (d.y * d.y + (d.z * d.z + d.x * d.x) > 0.001f) {
        nav_pos = pos;
        if (nav_face != -1) {
            if (g_mission->scene->nav->inside(&nav_face, &pos.x))
                return nav_face;
            if (nav_face != -1) {
                int mode = 2;
                nav_face = g_mission->scene->locate_near(&pos, &nav_face, &mode);
                if (nav_face != -1)
                    return nav_face;
                nav_face = g_mission->scene->nearest_face(&pos.x);
                return nav_face;
            }
        }
        nav_face = g_mission->scene->nearest_face(&pos.x);
    }
    return nav_face;
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

// variant for the third block: the 1-6 arm tests mode != 2 first (matches 0x43cf37)
#define PICK3(k9, k07, k16, kx, m9, m07, m16, mx, shared) \
    if (blow.bone == 9) { \
        if (mode == 2) return m9; \
        return (mode != 1 ? -16 : 0) + k9; \
    } \
    if (blow.bone == 0 || blow.bone == 7) { \
        if (mode != 2) return (mode != 1 ? -16 : 0) + k07; \
        return m07; \
    } \
    if (blow.bone == 1 || blow.bone == 4 || blow.bone == 2 || blow.bone == 5 || blow.bone == 3 || blow.bone == 6) { \
        if (mode != 2) return (mode != 1 ? -16 : 0) + k16; \
        return shared; \
    } \
    if (mode == 2) return mx; \
    return (mode != 1 ? -16 : 0) + kx;

// @ 0x43cc30
int Agent::move_anim(int mode)
{
    vector4 t;
    t.x = 0.0f; t.y = 0.0f; t.z = 0.0f;
    if ((int)blow.attacker >= 0) {
        unsigned tid = blow.attacker;
        Agent** pp = &g_mission->agents.pages[tid >> 4];
        t = (*pp + (tid & 0xf))->pos;
    }
    vector4 d(t.x - pos.x, t.y - pos.y, t.z - pos.z);
    if (d.x * vel.x + d.y * vel.y + d.z * vel.z > 0.0f) {
        rgl::strategic_entity* b = body;
        float dot = blow.dir2.z * b->local.s.z;
        dot += blow.dir2.y * b->local.s.y;
        float mx = blow.dir2.x;
        dot += mx * b->local.s.x;
        if (dot > 0.5f) {
            PICK(0xff, 0x107, 0x10b, 0x103, 0x10f, 0x116, 0x116, 0x113, 0x116)
        }
        vector4 ns(-b->local.s.x, -b->local.s.y, -b->local.s.z);
        if (ns.x * blow.dir2.x + ns.y * blow.dir2.y + ns.z * blow.dir2.z > 0.5f) {
            PICK(0xfe, 0x106, 0x10a, 0x102, 0x10e, 0x115, 0x115, 0x112, 0x115)
        }
        {
            PICK3(0x100, 0x108, 0x10c, 0x104, 0x110, 0x117, 0x117, 0x114, 0x117)
        }
    }
    PICK(0x101, 0x109, 0x10d, 0x105, 0x111, 0x118, 0x118, 0x118, 0x118)
}

// ---- animation blend update ---------------------------------------------------------------------------------------------------------------------------------------------------------
static inline Agent* AGENT(unsigned id)
{
    Agent** pp = &g_mission->agents.pages[id >> 4];
    return *pp + (id & 0xf);
}

// per-tick update of the two animation slots: next sub-animation, bone masks, channel weights from the pitch,
// head / torso rotations from the look angles
// @ 0x43d040
void Agent::update_anims(int unused)
{
    int i;
    if (disabled != 0)
        return;

    i = 0;
    do {
        AnimSlot* s = &anim[i];
        int v = s->next;
        if (v == -1 || v == s->cur) {
            if (body->fx->env[i].busy() && s->cur != -1) {
                if (anim[i].sub != -1 && v != -1 && ((defs->data[s->cur].subs[anim[i].sub].flags & 0x10000000) != 0)) {
                    v = pick(i);
                    if (v != anim[i].sub)
                        goto start;
                }
            }
        } else {
            v = pick(i);
        start:
            if (v != -1 && s->next != -1) {
                unsigned f1 = 0;
                float f2 = 0;
                if (s->cur != -1 && anim[i].sub != -1) {
                    SubAnim* o = &defs->data[s->cur].subs[anim[i].sub];
                    f1 = o->flags;
                    f2 = o->offset;
                }
                start_channel(i, &defs->data[s->next].subs[v], &f1, &f2);
                anim[i].sub = v;
                s->f28 = defs->data[s->next].priority;
                if (i == 0)
                    prev_scale1 = 1.0f;
            }
        }
        s->cur = s->next;
        i++;
    } while (i < 2);

    char act[2];
    int a0 = (anim[0].cur == -1 || anim[0].sub == -1) ? 0 : 1;
    act[0] = a0;
    int a1 = (anim[1].cur == -1 || anim[1].sub == -1) ? 0 : 1;
    act[1] = a1;

    unsigned bits[2];
    unsigned m = 0xffffffff;
    if (act[0]) {
        unsigned fl = defs->data[anim[0].cur].flags;
        if (fl & 0x100) { bits[0] = body->fx->res->f28; m = ~bits[0]; }
        else if (fl & 0x200) { bits[0] = body->fx->res->f2c; m = ~bits[0]; }
        else if (fl & 0x400) { bits[0] = body->fx->res->f30; m = ~bits[0]; }
        else { bits[0] = 0; m = ~bits[0]; }
    } else {
        bits[0] = 0;
    }
    if (act[1]) {
        unsigned fl = defs->data[anim[1].cur].flags;
        unsigned x;
        if (fl & 0x100) x = body->fx->res->f28;
        else if (fl & 0x200) x = body->fx->res->f2c;
        else if (fl & 0x400) x = body->fx->res->f30;
        else x = 0;
        bits[1] = (x & m) | (m & ~x);
        m = 0;
    } else {
        bits[1] = 0;
    }
    if (act[0])
        bits[0] |= m;

    i = 0;
    do {
        unsigned nb = bits[i];
        FxDef* sk = body->fx;
        unsigned diff = anim[i].prev_mask ^ nb;
        unsigned bit = 1;
        int off = 0;
        for (int k = 0; k < sk->count; k++) {
            if ((bit & diff) != 0)
                sk->elems[k].blend_weight = 0;
            bit <<= 1;
        }
        EnvRec* c = &body->fx->env[i];
        c->active_mask = nb;
        anim[i].prev_mask = nb;
        if (anim[i].f10 != -1)
            body->fx->env[i].bone = anim[i].f10;
        i++;
    } while (i < 2);

    i = 0;
    do {
        if (act[i]) {
            AnimDef* d = &defs->data[anim[i].cur];
            if ((d->flags & 1) != 0 && horse >= 0)
                body->fx->set_rec_phase(i, AGENT(horse)->anim[0].scale);
            if ((d->flags & 0x1000) == 0) {
                body->fx->env[i].wt.w[1] = 0.0f;
                body->fx->env[i].wt.w[0] = 1.0f;
            } else {
                double v = asin(in_now.move.z);
                if (v > 0.0) {
                    v = v * 1.0504225f;
                    body->fx->env[i].wt.offs[1] = anim[i].f28;
                    body->fx->env[i].wt.w[1] = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->fx->env[i].wt.w[1] = (float)v;
                    body->fx->env[i].wt.w[0] = (float)(1.0 - v);
                } else {
                    v = v * -1.0504225f;
                    body->fx->env[i].wt.offs[1] = -anim[i].f28;
                    body->fx->env[i].wt.w[1] = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->fx->env[i].wt.w[1] = (float)v;
                    body->fx->env[i].wt.w[0] = (float)(1.0 - v);
                }
            }
            if ((d->flags & 0x2000) != 0 && horse != -1) {
                double h = cur * 0.5f * 0.5f;
                quaternion q;
                q.x = 0.0f;
                q.z = 0.0f;
                q.y = (float)sin(h);
                q.w = (float)cos(h);
                body->fx->env[i].ref[7] = q;
                body->fx->env[i].flip_mask |= g_bit[7];
                body->fx->env[i].ref[8] = q;
                body->fx->env[i].flip_mask |= g_bit[8];
            }
            if (kind != 0 && (d->flags & 0x8000) == 0) {
                double h = -pitch * 0.5f;
                quaternion q;
                q.y = 0.0f;
                q.z = 0.0f;
                vector4 t;
                t.x = 0.0f;
                q.x = (float)sin(h);
                q.w = (float)cos(h);
                body->fx->env[i].ref[0] = q;
                body->fx->env[i].flip_mask |= g_bit[0];
                double a = -pitch;
                double c1 = cos(a);
                if (pitch < 0.0f) {
                    t.y = (float)((1.0 - c1) * 1.59f);
                    t.z = (float)(sin(a) * -0.79f);
                } else {
                    t.y = (float)((1.0 - c1) * -0.99f);
                    t.z = (float)(sin(a) * -0.89f);
                }
                body->fx->env[i].add_pos = t;
                body->fx->env[i].flip_mask |= g_bit[0];
                q.x = -q.x;
                body->fx->env[i].ref[8] = q;
                body->fx->env[i].flip_mask |= g_bit[8];
                body->fx->env[i].ref[13] = q;
                body->fx->env[i].flip_mask |= g_bit[13];
                body->fx->env[i].ref[18] = q;
                body->fx->env[i].flip_mask |= g_bit[18];
                body->fx->env[i].ref[22] = q;
                body->fx->env[i].flip_mask |= g_bit[22];
            }
        }
        i++;
    } while (i < 2);

    i = 0;
    do {
        AnimSlot* s = &anim[i];
        if ((s->events & 0x100000) != 0)
            body->fx->env[i].set_phase(s->rate_scale);
        unsigned ev = s->events;
        if ((ev & 0x3000000) == 0 || (body->fx->env[i].busy() && (ev & 0x2000000) == 0))
            s->events = 0;
        if (s->phase >= 0.0f) {                     // false for a NaN phase too (0x43da1f: test ah,1 = C0, set when unordered)
            body->fx->set_rec_phase(i, s->phase);
            s->phase = -1.0f;
        }
        i++;
    } while (i < 2);

    if (kind == 0 && anim[0].cur >= 0) {
        AnimDef* d = &defs->data[anim[0].cur];
        if ((d->flags & 0x4000) != 0) {
            const vector4& p = d->subs[anim[0].sub].displacement;
            float x = p.x, y = p.y, z = p.z, w = p.w;
            EnvRec* c = body->fx->env;
            float sc = anim[0].scale;
            c->add_pos.x = -x * sc;
            c->add_pos.y = -y * sc;
            c->add_pos.z = -z * sc;
            c->add_pos.w = w;
            c->flip_mask |= g_bit[0];
        }
    }
}

// ---- path goals and the path follower ----------------------------------------------------------------------------------------------------------------------------------------------------
// @ 0x43db60
template E1c8* std::vector<E1c8>::_Ufill(E1c8*, size_t, const E1c8&);

// @ 0x43db90
void Agent::set_goal_agent(unsigned aid)
{
    int a = AGENT(aid)->update_nav_face();
    if (update_nav_face() != a) {
        if (ai.path_active != 0 && AGENT(aid)->update_nav_face() == ai.goal_face) {
            ai.goal_changed = 0;
            goto done;
        }
        ai.goal_changed = 1;
    }
    ai.path_active = 0;
done:
    ai.goal_agent = aid;
    ai.goal = AGENT(aid)->pos;
    ai.goal_dir = AGENT(aid)->v56b4;
    ai.goal_face = AGENT(aid)->update_nav_face();
    ai.path_flag = 0;
    ai.path_state = 2;
}

// @ 0x43dcb0
inline void Agent::set_goal_point(vector4 p, int face)
{
    if (update_nav_face() != face) {
        ai.goal_changed = 1;
        if (ai.path_active != 0 && face == ai.goal_face) {
            ai.goal_changed = 0;
            goto done;
        }
    }
    ai.path_active = 0;
done:
    ai.goal = p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    ai.path_state = 1;
    ai.goal_dir = p;
    ai.goal_agent = -1;
    ai.goal_face = face;
    ai.path_flag = 0;
}

// @ 0x43dd70
void Agent::set_goal_near(const vector4* center, const float* radius)
{
    int i = 0;
    float r = *radius;
    vector4 p;
    Mission* m;
    do {
        p = *center;
        p.x = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.x;
        p.y = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.y;
        m = g_mission;
        if (m->min_x <= p.x && p.x < m->max_x && m->min_y <= p.y && p.y < m->max_y)
            break;
        i++;
        r *= 0.9f;
    } while (i < 0x20);
    vector4 d(ai.goal.x - p.x, ai.goal.y - p.y, ai.goal.z - p.z);
    if (d.z * d.z + (d.y * d.y + d.x * d.x) < 1e-06f) {
        ai.path_state = 1;
        return;
    }
    int cur = ai.goal_face;
    int face;
    if (cur >= 0 && m->nav.inside(&ai.goal_face, &p.x)) {
        face = cur;
    } else {
        int mode = 2;
        if (cur >= 0 && (face = m->scene->locate_near(&p, &ai.goal_face, &mode)) != -1) {
        } else {
            face = m->scene->nearest_face(&p.x);
        }
    }
    set_goal_point(p, face);
}

// @ 0x43df60
void Agent::advance_path()
{
    if (ai.path_active != 0 && (ai.path_state & 3) != 0) {
        int face = update_nav_face();
        int n = ai.path_count;
        if (face == ai.path_face[n]) {
            ai.path_active = 0;
            ai.goal_changed = 0;
            return;
        }
        if (face >= 0) {
            PolyMesh* nav = &g_mission->nav;
            if (nav->inside(&face, &pos.x)) {
                int next = ai.path_cur + 1;
                int nface;
                vector4 np;
                if (next < n) {
                    np = ai.path_pos[ai.path_cur + 1];
                    nface = ai.path_face[ai.path_cur + 2];
                } else {
                    np = ai.goal;
                    nface = ai.goal_face;
                }
                int steps = 10;
                vector4 d = pos - np;
                if (d.out_length_squared() < 900.0f) {
                    if (nav->trace(face, pos, nface, &np.x, &steps))
                        ai.path_cur = next;
                }
            }
        }
        if (ai.path_cur >= ai.path_count) {
            ai.path_active = 0;
            ai.goal_changed = 0;
        }
    }
}

// deque<T>::_Xlen
// @ 0x43e0e0
void deque_xlen() { throw std::length_error("deque<T> too long"); }

// std::allocator<PhysRec>::destroy: the record's destructor (bodies, then bones)
// @ 0x43e150
template void std::allocator<PhysRec>::destroy(PhysRec*);

#undef PICK
