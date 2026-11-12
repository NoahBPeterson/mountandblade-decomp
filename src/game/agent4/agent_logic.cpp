//-----------------------------------------------------------------------------
// File: agent_logic.cpp  (0x43a280..0x460160, and the std::logic_error family at 0x40c2c0..0x40c670)
//
// Desc: the agent logic object, one file in address order. The original compiled this block as one translation unit
//       (docs/original_tus.md A1): 0x43a280 takes register arguments from 0x457b40, and first_used, next_used, vnorm and
//       friends are shared across what used to be agent_stats' tail, agent_ai, agent_combat, phys_set and agent_fx_sound.
// Desc: the agent's logic object: the navigation-mesh path search (PolyMesh::find_path, reachable), goal / path planning and
//       class sounds, ammunition top-up (with the std::vector<E56> helpers the linker kept from this object), weapon-mode choice
//       and attack start, the AI order executor (update_ai) and control update, slot cycling, missile / scene-prop spawns, the AI
//       decision (choose_action), the idle timers and the agent's shot; the mesh editing members of rgl::mesh (add_quad,
//       append_skinned, scale, translate, transform, compute_normals, append_plain), the font glyph quad and text layout, the
//       carried-item mesh, the placed-object append, Newton world creation, ragdoll start / update, blood and dust, the carried
//       item refresh, the message log, label box, shield / blow damage, corpse spawn, death states, blow delivery, swing trace,
//       attack update, movement step, the per-frame update and tick. The register-convention statics 0x4350c0 (slot_class_impl)
//       and 0x43a280 (ui_weapon_class_name) are this object's (callers at 0x4385e0, 0x448eb0, 0x44e2b0, 0x453710, 0x457b40).
//       Functions in ascending address order (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//-----------------------------------------------------------------------------
#define ANIM_SLOT_SET_OUT_OF_LINE       // 0x4378a0 is a real call here (the body is agent2/agent_stats.cpp's), written out at two sites of on_wield_changed
#define STD_VECTOR_INT_INSTANTIATED_HERE   // std_vec_out_of_line.h: vector<int>::resize 0x43f310 and operator= 0x43f0a0 are this file's
#define INT_HASH_LOWER_BOUND_INSTANTIATED_HERE   // glyph_lookup.h: the int hash lookup 0x43b210 is this file's
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../std_vec_out_of_line.h"
#include "../agent2/agent_types.h"
#include "../agent3/agent_types.h"
#include "../agent2/agent_ai.h"
#include "./agent_logic.h"
#include "../glyph_lookup.h"
#include "../game_globals.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include <algorithm>
#include <stdexcept>
#include "../dialog_manager.h"      // g_script 0xa70440 (g_ev_a / g_ev_b live in it; listed directly: the resolution audit reads only the source's own includes)
#include "../agent2/agent_stats.h"
#include "../anim_slot.h"      // (named here too for the resolution audit)
#include "../channel.h"
#include "../agent/curve.h"      // Curve: the canonical class of the Skeleton record (Curve::recs +0x40 is Skeleton::chans)
#include "../../include/mat4.h"
#include "../../include/ptr_tag.h"
#include "../../include/ptr_vec.h"
#include "../../include/timer.h"
#include "../../include/vec.h"
#include "../../include/shape.h"      // (the shared class headers named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/phys_set.h"
#include "../../include/height_field.h"
#include "../../rgl/scene/world.h"
#include "../light.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../placed_obj.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../skeleton.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../item_ref.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../agent_obj.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../mission.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../terrain_b/poly_mesh.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../skill_cap.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/game_data.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/troop.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/party.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../int_vec.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../str_table.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/options.h"   // g_options 0x5aa008
#include "../../include/item_kind.h"   // g_items 0x5e29d0
#include "../slot.h"
#include "../agent3/agent_combat.h"
#include "../tstring.h"
#include "../font.h"
#include "../../include/iface.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../agent/phys_set.h"
#include "../../include/quaternion.h"
#include "../env_row.h"                // EnvRow, g_env_rows (0x5e28f4)
#include "../res_mgr.h"
#include "../config/config_block.h"
#include "../../include/fx.h"
#include "../emit.h"
#include "../strat/gui_manager.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/sound_mgr.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../../rgl/mesh.h"
#include "../dialog_mgr.h"
#include <memory>
#include "../agent/agent.h"
#include "../key_names.h"      // g_keyboard (0xa170c8): pick_sound_ids
#include "../mode_rec.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"
#include "../hnode.h"
#include "../blow.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object
#include "../../include/rec78.h"

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
tstring* TroopRec::display_name()
{
    tstring* t = g_strtable.find(&key);
    return t ? t : (tstring*)this;
}

// ---- <vector> helpers (VC7.1) --------------------------------------------------------------------------------------------------------------
// @ 0x43a4a0
E12* vec12::ufill(E12* f, unsigned n, const E12& v)
{
    ptr_tag t;
    uninit_fill_n12(f, n, v, *this, t);
    return f + n;
}
// @ 0x43a4d0
tstring* vecstr::ucopy(tstring* f, tstring* l, tstring* d) { ptr_tag t; return uninit_copy_str(f, l, d, *this, t); }
// @ 0x43a4f0
int* vecint::ucopy(int* f, int* l, int* d) { ptr_tag t; return uninit_copy_int(f, l, d, *this, t); }

// ---- the agent's hand position -----------------------------------------------------------------------------------------------------------------
// @ 0x43a510
void Agent::hand_pos(vector4* out)
{
    int c = team;
    if (c < 0 || c >= 8) c = 7;
    int h = 0;
    if (troop >= 0) {
        if (f2c >= 0) h = 2;
        else h = (g_game->chars[troop].flags30 & 0x4000000i64) != 0 ? 1 : 0;
    }
    ClassInfo* r = &g_mission->teams[c].cls[h];
    float dy = r->dir.y;
    float ndx = -r->dir.x;
    bool left = false;
    float k = 1.0f;
    if (r->mode == 1) {
        k = 0.7f;
        if (f1cc & 1) left = true;
    } else if (r->mode == 2) {
        k = 1.0f;
    } else if (r->mode == 3) {
        k = 1.65f;
    } else if (r->mode == 4) {
        k = 2.5f;
    }
    if (f2c != -1) k *= 1.6f;
    *out = r->pos;
    float px = r->dir.x * f5344;
    float py = r->dir.y * f5344;
    float nx = px * k + out->x;
    float ny = py * k + out->y;
    out->x = nx;
    out->y = ny;
    if (left) {
        out->x = nx - k * r->dir.x;
        out->y = ny - k * r->dir.y;
    }
    float pdy = dy * f5340;
    float pndx = ndx * f5340;
    float t8 = pdy * k;
    float x2 = t8 + out->x;
    out->x = x2;
    float y2 = pndx * k + out->y;
    float kk = k * 0.1f;
    out->y = y2;
    out->x = x2 + kk * f5348;
    float t4 = kk * f534c;
    out->y = y2 + t4;
    float height = x2;
    face* hit;
    g_mission->scene->floor_at(&out->x, &height, &hit);
    out->z = height;
}

// 0x433770 (Obj3::set_placement, rgl/xmlcfg/agent_pick.cpp owns the row; the emitter's placement block as game_util.h lays it out): the body
// visible, so the frame EmitTable::setup (0x4424c0) passes does not escape and its origin stores are independent of the loads through d.
struct Obj3Place_ {
    char pad0[0xe8];
    unsigned flags;                         // +0xe8
    char padec[0x134 - 0xec];
    rgl::matrix m134;                       // +0x134
    vector4 v174;                           // +0x174
    vector4 v184;                           // +0x184
    int m194;                               // +0x194
    int pad198;
    int m19c, m1a0, m1a4;                   // +0x19c
};
inline __declspec(noinline) void Obj3::set_placement(const matrix* m, const vector4* a, const vector4* b, int c)
{
    Obj3Place_* t = (Obj3Place_*)this;
    unsigned f = t->flags & ~1u;
    t->flags = f;
    t->m134 = *m;
    t->v174 = *a;
    t->v184 = *b;
    t->m194 = c;
    if (f & 2) {
        t->m19c = t->m1a0 = 1000000000;
        t->m1a4 = 0;
    }
}

// 0x40cd60 / 0x40cd00 (rgl::identity3 / rgl::identity_pos, rgl/shapes/shapes.cpp owns the rows): the bodies visible, so a frame passed to
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
    g_mission->scene->query_box(&mn.x, &mx.x, mask, found, &n);
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
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            if (((ik->type_flags & 0xff) == 5 || (ik->type_flags & 0xff) == 6 || (ik->type_flags & 0xff) == 10) && disp[i] != 0) {
                int used = pairs[i].m4 & 0xff;
                int n = pairs[i].max_amount() - used;
                if (links[i] != 0 && links[i]->vis != 0) n--;
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
void Agent::start_channel(int idx, SubAnim* cmd, unsigned* in_flags, float* base)
{
    float cur = ((Curve*)body->skel)->rec_shaped_phase(idx);
    float scale = cmd->scale;
    float len = 1.5f;
    if (cmd->flags & 0xff)
        len = (float)(int)((cmd->flags & 0xff) - 1) * 0.1f;
    if (anim[idx].events & 0x400000) {
        float inv = 1.0f / anim[idx].rate;
        scale = inv * scale;
        len = inv * len;
    }
    body->skel->chans[idx].period = scale;
    body->skel->start_anim(idx, (Anim*)cmd->anim, cmd->a, cmd->b);
    body->skel->chans[idx].blend = len + 0.01f;
    ((Curve*)body->skel)->reset_rec(idx);
    if (cmd->flags & 0x10000000)
        body->skel->chans[idx].mode = 0x14;
    else
        body->skel->chans[idx].mode = 0x15;
    if (cmd->flags & 0xa0000000) {
        float v = 0.0f;
        unsigned fl = *in_flags;
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
        ((Curve*)body->skel)->set_rec_phase(idx, res);
    } else if (cmd->flags & 0x40000000) {
        ((Curve*)body->skel)->set_rec_phase(idx, cmd->offset);
    }
}

// std::_Uninit_fill_n for the 0x1c8-byte elements (catch funclet 0x43abbb: destroys [s, f) then `throw;`)
// @ 0x43ab50
void __cdecl uninit_fill_n1c8(E1c8* f, unsigned n, const E1c8& v, allocE1c8& al, ptr_tag tag)
{
    E1c8* s = f;
    try {
        for (; 0 < n; --n, ++f) al.construct(f, v);
    } catch (...) {
        for (; s != f; ++s) s->~E1c8();
        throw;
    }
}

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
    Light** it = lights._Myfirst;
    if (it < lights._Mylast) {
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
                        if (!trace_ray(&p0.x, &hp.x, 150.0f, 0, 0)) goto next;
                    }
                    l = *it;
                    if (l->flags & 1) k = k * 0.5f;
                    float t = dp * 0.9f + 0.1f;
                    float s = k * t;
                    float cr = s * l->color.x;
                    float cg = s * l->color.y;
                    float cb = s * l->color.z;
                    l = *it;
                    out_col->x = cr + out_col->x;
                    out_col->y = cg + out_col->y;
                    out_col->z = cb + out_col->z;
                    float br = (l->color.z + l->color.y) + l->color.x;
                    float ax = (d.x * k) * t;
                    float ay = (d.y * k) * t;
                    float az = (d.z * k) * t;
                    float bx = ax * 0.33f;
                    float by = ay * 0.33f;
                    float bz = az * 0.33f;
                    out_dir->x = bx * br + out_dir->x;
                    out_dir->y = by * br + out_dir->y;
                    out_dir->z = br * bz + out_dir->z;
                }
            }
        next:
            it++;
        } while (it < lights._Mylast);
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


// ---- 0x1f8-byte record table accessors --------------------------------------------------------------------------------------------------------------------
// @ 0x43b100
__declspec(noinline) int Table1f8::get_f1f0(unsigned i)
{
    if (i < count) return recs[i].f1f0;
    return 0;
}
// @ 0x43b130
__declspec(noinline) int Table1f8::get_f1f4(unsigned i)
{
    if (i < count) return recs[i].f1f4;
    return 0;
}

// ---- the agent table (agents are 0x5928-byte records in pages of 16) --------------------------------------------------------------------------------------------
// @ 0x43b160
inline void AgentTable::next_used(unsigned* i)
{
    ++*i;
    while ((int)*i < count) {
        unsigned v = *i;
        if (pages[v >> 4][v & 0xf].used != 0) break;
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
            if (pg[v >> 4][v & 0xf].used != 0) break;
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

// stdext::hash_map<int, HashVal>::lower_bound: bucket (key ^ 0xdeadbeef) & mask, the iterator returned through a hidden pointer
// @ 0x43b210
template IntHashBase::iterator IntHashBase::lower_bound(const int&);

// @ 0x43b2b0
tstring* vecstr::ufill(tstring* f, unsigned n, const tstring& v)
{
    ptr_tag t;
    uninit_fill_n_str(f, n, v, *this, t);
    return f + n;
}
// @ 0x43b2e0
E1c8* vecE1c8::ucopy(E1c8* f, E1c8* l, E1c8* d) { ptr_tag t; return uninit_copy1c8(f, l, d, *this, t); }

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
void Agent::collect_enemies(int)
{
    nthreat = 0;
    Mission* m = g_mission;
    AgentTable* t = &m->agents;
    unsigned i = t->first_used();
    if (i < (unsigned)t->count) {
        do {
            Agent* o = &m->agents.pages[i >> 4][i & 0xf];
            if (o->f10 == 0 && o->f14 == 1 && (int)i != id && hostile_to(m, team, o)) {
                vector4 d;
                d.x = o->pos.x - pos.x;
                d.y = o->pos.y - pos.y;
                d.z = o->pos.z - pos.z;
                float len = d.normalize();
                int k = 0;
                int n = nthreat;
                if (0 < n) {
                    while (k < nthreat) {
                        if (!(threats[k].dist < len)) break;
                        k++;
                    }
                    if (k >= 16) goto next;
                }
                if (n < 16) nthreat = n + 1;
                for (int j = nthreat - 1; j > k; j--) threats[j] = threats[j - 1];
                threats[k].id = o->id;
                threats[k].dist = len;
                threats[k].dir = d;
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
    int n = nthreat;
    unsigned tid = (unsigned)-1;
    if (0 < n) {
        Threat* r = threats;
        int k = n;
        do {
            tid = r->id;
            if (g_mission->agents.pages[tid >> 4][tid & 0xf].f14 == 1) {
                const vector4& p = g_mission->agents.pages[tid >> 4][tid & 0xf].pos;
                float dx = p.x - pos.x;
                float dy = p.y - pos.y;
                float dz = p.z - pos.z;
                float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                v = v / ((float)agent_by_id(r->id)->f55e8 + 7.0f);
                if (v > bestv) { bestv = v; best = tid; }
            }
            r++;
            k--;
            tid = best;
        } while (k != 0);
    }
    return tid;
}

// @ 0x43b5d0
unsigned Agent::pick_random()
{
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned tid = (unsigned)-1;
    int n = 0;
    if (0 < nthreat) {
        Threat* r = threats;
        do {
            tid = r->id;
            Agent* o = &g_mission->agents.pages[tid >> 4][tid & 0xf];
            if (o->f14 == 1) {
                float dx = o->pos.x - pos.x;
                float dy = o->pos.y - pos.y;
                float dz = o->pos.z - pos.z;
                float v = 3.0f / (float)sqrt(dx * dx + (dy * dy + dz * dz));
                if (!(v < 4.0f)) v = 4.0f;
                Agent* o2 = &g_mission->agents.pages[tid >> 4][tid & 0xf];
                ItemRef cur = o2->cur_pair();       // a by-value copy: its stack stores keep cl from reusing the page lookups below
                int item = cur.item;
                if (item >= 0) {
                    unsigned t = g_items[item].type_flags & 0xff;
                    if (t == 8 || t == 9 || t == 10 || t == 0x10 || t == 0x11) v = v * 1.5f;
                }
                if (g_mission->agents.pages[tid >> 4][tid & 0xf].f2c != -1) v = v * 0.85f;
                int w = g_mission->agents.pages[tid >> 4][tid & 0xf].f55e8;
                v = v / ((float)w + 4.0f);
                v = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * v;
                if (v > bestv) { bestv = v; best = r->id; }
            }
            n++;
            r++;
            tid = best;
        } while (n < nthreat);
    }
    return tid;
}

// the agent's (or its mount's) velocity rotated by the body frame
// @ 0x43b7b0
vector4* Agent::local_velocity(vector4* out)
{
    unsigned idx = f2c;
    vector4 v;
    if ((int)idx >= 0) {
        Agent* a = &g_mission->agents.pages[idx >> 4][idx & 0xf];
        v.x = a->vel_xy.x; v.y = a->vel_xy.y;
    } else {
        v.x = vel_xy.x; v.y = vel_xy.y;
    }
    v.z = 0.0f;
    if (body) {
        const rgl::matrix& m = body->m;
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
            if (o->f10 == 1 && o->f30 == -1 && o->f14 == 1 && o->f5830 == 2) {
                if (o->f8c.stat_1ec() <= g_game->skill_level(troop, 0x18)) {
                    vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                    float len = d.normalize();
                    if (len < 1.0f) {
                        struct V2 { float x, y; } h = { fx, fy };
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
            if (!allied_to(g_mission, myteam, o) && o->f14 == 1 && o->f10 != 1) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * fx + d.y * fy) + 0.5f, 0.0f, 1.2f);
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
            if (hostile_to(g_mission, myteam, o) && o->f14 == 1 && o->active != 0) {
                vector4 d(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                float len = d.normalize();
                float a = clampf((d.x * fx + (d.y * fy + d.z * fz)) + 0.8f, 0.0f, 1.0f);
                float b = clampf(1.0f / (len + 0.5f), 0.0f, 1.0f);
                float od = -(d.x * o->fx + (d.y * o->fy + d.z * o->fz));
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
    int v = g_items[item].f204;
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
            vector4 d = vsub(*(vector4*)&fx, prev_facing);
            float spd = 0.0f;
            if (g_step_tbl[0] > 0.0f) spd = (float)sqrt(d.y * d.y + d.z * d.z + d.x * d.x) / g_step_tbl[0];
            spd = spd * 0.0f;
            if (spd > 1.0f) spd = 1.0f;
            prev_facing = *(vector4*)&fx;
            unsigned t2 = ik->type_flags & 0xff;
            if (t2 == itp_type_bow) {
                float sk = (float)g_game->skill_level(troop, skl_power_draw) * 25.0f + (int)g_game->chars[troop].f80[3];
                float p21 = (float)cur.stat_21x(1) * 8.0f;
                float w = (float)cur.stat_1ec() * 40.0f + p21 + 50.0f;
                int c = g_items[item].f204;
                if (c == 0) c = 99;
                float cf = (float)c * 0.01f;
                float acc0 = ((w + 50.0f) - sk) * 6e-5f;
                acc = (0.0f > acc0) ? 0.0f : acc0;
                if (acc < 0.0f) acc = 0.0f;
                if (active) {
                    int ag = f2c;
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
                float sk = (float)g_game->skill_level(troop, skl_power_throw) * 20.0f + (int)g_game->chars[troop].f80[5];
                float cf = (float)cur.accuracy() * 0.01f;
                if (active) {
                    float p21 = (float)cur.stat_21x(1);
                    float fv = (float)cur.stat_1ec() * 5.0f + p21;
                    acc = fmaxr(0.0f, ((fv + 30.0f) * 8.0f - sk) * 0.00012f);
                    int ag = f2c;
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
                    A = (float)(int)g_game->chars[troop].f80[4];
                    B = (float)cur.stat_21x(1);
                } else if (t2 == itp_type_pistol || t2 == itp_type_musket) {
                    A = (float)(int)g_game->chars[troop].f80[6];
                    B = (float)cur.stat_21x(1);
                }
                if (active && (cur.max_amount() - (int)(cur.m4 & 0xff)) != 0) {
                    float zero = 0.0f;
                    float p1 = ((B + B + 100.0f) - A) * 0.00015f;
                    float q = A + 100.0f;
                    float p2 = 0.7f - timer.elapsed() * q * 0.01f;
                    float zero2 = 0.0f;
                    float r1 = fmax_ref(&zero, &p1);
                    acc = fmax_ref(&zero2, &p2) + r1;
                    int ag = f2c;
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
    *out = in_prev;
    if (q_count != 0) {
        unsigned now_lo = ((unsigned*)g_ticks)[4];
        int now_hi = ((int*)g_ticks)[5];
        while (q_count != 0) {
            unsigned i = q_head;
            if (q_cap <= i)
                i -= q_cap;
            InputRec* e = q_buf[i];
            if (e->hi > now_hi || (e->hi == now_hi && e->lo >= now_lo))
                break;
            i = q_head;
            if (q_cap <= i)
                i -= q_cap;
            out->v[0] |= q_buf[i]->bits;
            in_last = *out;
            if (q_count != 0) {
                q_head++;
                if (q_cap <= q_head)
                    q_head = 0;
                q_count--;
                if (q_count == 0)
                    q_head = 0;
            }
        }
    }
    out->v[0] |= in_last.v[0];
    if ((orig.v[0] & 0x100) == 0 && (out->v[0] & 0x100) != 0)
        out->v[0] |= 0x400;
}

// @ 0x43cb30
int Agent::update_nav_face()
{
    vector4 d(nav_pos.x - pos.x, nav_pos.y - pos.y, nav_pos.z - pos.z);
    if (d.y * d.y + (d.z * d.z + d.x * d.x) > 0.001f) {
        vector4* p = &pos;
        nav_pos = *p;
        int face = nav_face;
        if (face != -1) {
            if (g_mission->scene->nav->inside(&nav_face, &p->x))
                return nav_face;
            if (face != -1) {
                int mode = 2;
                nav_face = g_mission->scene->locate_near(p, &nav_face, &mode);
                if (nav_face != -1)
                    return nav_face;
                nav_face = g_mission->scene->nearest_face(&p->x);
                return nav_face;
            }
        }
        nav_face = g_mission->scene->nearest_face(&p->x);
    }
    return nav_face;
}

// Weapon-kind groups of the movement animation table (kind 9; 0 and 7; 1..6; everything else).
#define PICK(k9, k07, k16, kx, m9, m07, m16, mx, shared) \
    if (weapon_kind == 9) { \
        if (mode == 2) return m9; \
        return (mode != 1 ? -16 : 0) + k9; \
    } \
    if (weapon_kind == 0 || weapon_kind == 7) { \
        if (mode != 2) return (mode != 1 ? -16 : 0) + k07; \
        return m07; \
    } \
    if (weapon_kind == 1 || weapon_kind == 4 || weapon_kind == 2 || weapon_kind == 5 || weapon_kind == 3 || weapon_kind == 6) { \
        if (mode == 2) return shared; \
        return (mode != 1 ? -16 : 0) + k16; \
    } \
    if (mode == 2) return mx; \
    return (mode != 1 ? -16 : 0) + kx;

// @ 0x43cc30
int Agent::move_anim(int mode)
{
    vector4 t;
    t.x = 0.0f; t.y = 0.0f; t.z = 0.0f;
    if (target_id >= 0) {
        unsigned tid = target_id;
        Agent** pp = &g_mission->agents.pages[tid >> 4];
        t = (*pp + (tid & 0xf))->pos;
    }
    vector4 d(t.x - pos.x, t.y - pos.y, t.z - pos.z);
    if (d.x * vel.x + d.y * vel.y + d.z * vel.z > 0.0f) {
        AgentBody* b = body;
        if (move_z * b->m.s.z + move_y * b->m.s.y + move_x * b->m.s.x > 0.5f) {
            PICK(0xff, 0x107, 0x10b, 0x103, 0x10f, 0x116, 0x116, 0x113, 0x116)
        }
        if (-b->m.s.z * move_z + -b->m.s.y * move_y + -b->m.s.x * move_x > 0.5f) {
            PICK(0xfe, 0x106, 0x10a, 0x102, 0x10e, 0x115, 0x115, 0x112, 0x115)
        }
        {
            PICK(0x100, 0x108, 0x10c, 0x104, 0x110, 0x117, 0x117, 0x114, 0x117)
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
            if (body->skel->chans[i].busy() && s->cur != -1) {
                if (anim[i].sub != -1 && v != -1 && ((defs->defs[s->cur].subs[anim[i].sub].flags & 0x10000000) != 0)) {
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
                    SubAnim* o = &defs->defs[s->cur].subs[anim[i].sub];
                    f1 = o->flags;
                    f2 = o->offset;
                }
                start_channel(i, &defs->defs[s->next].subs[v], &f1, &f2);
                anim[i].sub = v;
                s->f28 = defs->defs[s->next].f2c;
                if (i == 0)
                    f56e4 = 1.0f;
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
        unsigned fl = defs->defs[anim[0].cur].flags;
        if (fl & 0x100) { bits[0] = body->skel->sel[0x28 / 4]; m = ~bits[0]; }
        else if (fl & 0x200) { bits[0] = body->skel->sel[0x2c / 4]; m = ~bits[0]; }
        else if (fl & 0x400) { bits[0] = body->skel->sel[0x30 / 4]; m = ~bits[0]; }
        else { bits[0] = 0; m = ~bits[0]; }
    } else {
        bits[0] = 0;
    }
    if (act[1]) {
        unsigned fl = defs->defs[anim[1].cur].flags;
        unsigned x;
        if (fl & 0x100) x = body->skel->sel[0x28 / 4];
        else if (fl & 0x200) x = body->skel->sel[0x2c / 4];
        else if (fl & 0x400) x = body->skel->sel[0x30 / 4];
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
        Skeleton* sk = body->skel;
        unsigned diff = anim[i].prev_mask ^ nb;
        unsigned bit = 1;
        int off = 0;
        for (int k = 0; k < sk->nbones; k++) {
            if ((bit & diff) != 0)
                *(int*)((char*)sk->recs + off + 0x1d0) = 0;
            bit <<= 1;
            off += 0x1dc;
        }
        Channel* c = &body->skel->chans[i];
        c->flags = nb;
        c->fc = 0;
        anim[i].prev_mask = nb;
        if (anim[i].f10 != -1)
            body->skel->chans[i].f3c = anim[i].f10;
        i++;
    } while (i < 2);

    i = 0;
    do {
        if (act[i]) {
            AnimDef* d = &defs->defs[anim[i].cur];
            if ((d->flags & 1) != 0 && f2c >= 0)
                ((Curve*)body->skel)->set_rec_phase(i, AGENT(f2c)->anim[0].scale);
            if ((d->flags & 0x1000) == 0) {
                body->skel->chans[i].w1 = 0.0f;
                body->skel->chans[i].w0 = 1.0f;
            } else {
                double v = asin(fz);
                if (v > 0.0) {
                    v = v * 1.0504225f;
                    body->skel->chans[i].rate = anim[i].f28;
                    body->skel->chans[i].w1 = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->skel->chans[i].w1 = (float)v;
                    body->skel->chans[i].w0 = (float)(1.0 - v);
                } else {
                    v = v * -1.0504225f;
                    body->skel->chans[i].rate = -anim[i].f28;
                    body->skel->chans[i].w1 = 0.0f;
                    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
                    body->skel->chans[i].w1 = (float)v;
                    body->skel->chans[i].w0 = (float)(1.0 - v);
                }
            }
            if ((d->flags & 0x2000) != 0 && f2c != -1) {
                double h = cur * 0.5f * 0.5f;
                vector4 q;
                q.y = 0.0f;
                q.w = 0.0f;
                q.z = (float)sin(h);
                q.x = (float)cos(h);
                *(vector4*)(body->skel->chans[i].xform + 0x70 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)((__int64)g_bit[7]);
                body->skel->chans[i].mask_hi |= (unsigned)(((__int64)g_bit[7]) >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0x80 / 4) = q;
                *(__int64*)&body->skel->chans[i].mask_lo |= g_bit[8];
            }
            if (f10 != 0 && (d->flags & 0x8000) == 0) {
                double h = -f56cc * 0.5f;
                vector4 q;
                q.z = 0.0f;
                q.w = 0.0f;
                vector4 t;
                t.x = 0.0f;
                q.y = (float)sin(h);
                q.x = (float)cos(h);
                *(vector4*)body->skel->chans[i].xform = q;
                body->skel->chans[i].mask_lo |= (unsigned)((__int64)g_bit[0]);
                body->skel->chans[i].mask_hi |= (unsigned)(((__int64)g_bit[0]) >> 32);
                double a = -f56cc;
                double c1 = cos(a);
                if (f56cc < 0.0f) {
                    t.y = (float)((1.0 - c1) * 1.59f);
                    t.z = (float)(sin(a) * -0.79f);
                } else {
                    t.y = (float)((1.0 - c1) * -0.99f);
                    t.z = (float)(sin(a) * -0.89f);
                }
                *(vector4*)body->skel->chans[i].v6c = t;
                body->skel->chans[i].mask_lo |= (unsigned)((__int64)g_bit[0]);
                body->skel->chans[i].mask_hi |= (unsigned)(((__int64)g_bit[0]) >> 32);
                q.y = -q.y;
                *(vector4*)(body->skel->chans[i].xform + 0x80 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_bit[8];
                body->skel->chans[i].mask_hi |= (unsigned)(g_bit[8] >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0xd0 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_bit[13];
                body->skel->chans[i].mask_hi |= (unsigned)(g_bit[13] >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0x120 / 4) = q;
                body->skel->chans[i].mask_lo |= (unsigned)g_bit[18];
                body->skel->chans[i].mask_hi |= (unsigned)(g_bit[18] >> 32);
                *(vector4*)(body->skel->chans[i].xform + 0x160 / 4) = q;
                *(__int64*)&body->skel->chans[i].mask_lo |= g_bit[22];
            }
        }
        i++;
    } while (i < 2);

    i = 0;
    do {
        AnimSlot* s = &anim[i];
        if ((s->events & 0x100000) != 0)
            ((Curve*)&body->skel->chans[i])->set_phase(s->f1c);
        unsigned ev = s->events;
        if ((ev & 0x3000000) == 0 || (body->skel->chans[i].busy() && (ev & 0x2000000) == 0))
            s->events = 0;
        if (s->phase >= 0.0f) {                     // false for a NaN phase too (0x43da1f: test ah,1 = C0, set when unordered)
            ((Curve*)body->skel)->set_rec_phase(i, s->phase);
            s->phase = -1.0f;
        }
        i++;
    } while (i < 2);

    if (f10 == 0 && anim[0].cur >= 0) {
        AnimDef* d = &defs->defs[anim[0].cur];
        if ((d->flags & 0x4000) != 0) {
            float* p = &d->subs[anim[0].sub].v2c;
            float x = p[0], y = p[1], z = p[2], w = p[3];
            Channel* c = body->skel->chans;
            float sc = anim[0].scale;
            c->v6c[0] = -x * sc;
            c->v6c[1] = -y * sc;
            c->v6c[2] = -z * sc;
            c->v6c[3] = w;
            c->mask_lo |= (unsigned)((__int64)g_bit[0]);
            c->mask_hi |= (unsigned)(((__int64)g_bit[0]) >> 32);
        }
    }
}

// ---- path goals and the path follower ----------------------------------------------------------------------------------------------------------------------------------------------------
// @ 0x43db60
E1c8* vecE1c8::ufill(E1c8* f, unsigned n, const E1c8& v)
{
    ptr_tag t;
    uninit_fill_n1c8(f, n, v, *this, t);
    return f + n;
}

// @ 0x43db90
void Agent::set_goal_agent(unsigned aid)
{
    int a = AGENT(aid)->update_nav_face();
    if (update_nav_face() != a) {
        if (path_active != 0 && AGENT(aid)->update_nav_face() == goal_face) {
            goal_changed = 0;
            goto done;
        }
        goal_changed = 1;
    }
    path_active = 0;
done:
    f318 = aid;
    goal = AGENT(aid)->pos;
    goal_dir = AGENT(aid)->v56b4;
    goal_face = AGENT(aid)->update_nav_face();
    path_flag = 0;
    path_state = 2;
}

// @ 0x43dcb0
inline void Agent::set_goal_point(vector4 p, int face)
{
    if (update_nav_face() != face) {
        if (path_active != 0 && face == goal_face) {
            goal_changed = 0;
            goto done;
        }
        goal_changed = 1;
    }
    path_active = 0;
done:
    goal = p;
    p.x = 0.0f; p.y = 0.0f; p.z = 0.0f;
    path_state = 1;
    goal_dir = p;
    f318 = -1;
    goal_face = face;
    path_flag = 0;
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
        m = g_mission;
        p.y = ((rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * r * 2.0f + p.y;
        if (m->terrain.min_x <= p.x && p.x < m->terrain.max_x && m->terrain.min_y <= p.y && p.y < m->terrain.max_y)
            break;
        i++;
        r *= 0.9f;
    } while (i < 0x20);
    vector4 d(goal.x - p.x, goal.y - p.y, goal.z - p.z);
    if (d.z * d.z + (d.y * d.y + d.x * d.x) < 1e-06f) {
        path_state = 1;
        return;
    }
    int cur = goal_face;
    int face;
    if (cur >= 0 && m->nav.inside(&goal_face, &p.x)) {
        face = cur;
    } else {
        int mode = 2;
        if (cur >= 0 && (face = m->scene->locate_near(&p, &goal_face, &mode)) != -1) {
        } else {
            face = g_mission->scene->nearest_face(&p.x);
        }
    }
    set_goal_point(p, face);
}

// @ 0x43df60
void Agent::advance_path()
{
    if (path_active != 0 && (path_state & 3) != 0) {
        int face = update_nav_face();
        int n = path_count;
        if (path_face[n] == face) {
            path_active = 0;
            goal_changed = 0;
            return;
        }
        if (face >= 0) {
            PolyMesh* nav = &g_mission->nav;
            if (nav->inside(&face, &pos.x)) {
                int next = path_cur + 1;
                int nface;
                vector4 np;
                if (next < n) {
                    nface = path_face[path_cur + 2];
                    np = path_pos[path_cur];
                } else {
                    nface = goal_face;
                    np = goal;
                }
                int steps = 10;
                vector4 d;
                d.x = pos.x - np.x;
                d.y = pos.y - np.y;
                d.z = pos.z - np.z;
                if (d.out_length_squared() < 900.0f) {
                    if (nav->trace(face, pos, nface, &np.x, &steps))
                        path_cur = next;
                }
            }
        }
        if (path_cur >= path_count) {
            path_active = 0;
            goal_changed = 0;
        }
    }
}

// deque<T>::_Xlen
// @ 0x43e0e0
void deque_xlen() { throw std::length_error("deque<T> too long"); }

// destroys the two int vectors of an E56 record (std::allocator<E56>::destroy)
// @ 0x43e150
void alloc_e56::destroy(VecPair2* p)
{
    p->b.tidy();
    p->a.tidy();
}

#undef PICK

//=============================================================================
// 0x43e190..0x441360 (+ the std::logic_error family, 0x40c2c0..0x40c670), from agent_combat.cpp
// Desc: the game font's glyph metrics, the std::vector / std::deque instantiations the linker kept from this
//       object (tstring, int, E12, E48, E56, PlacedObj, E32 elements), the skeleton accessors, the agent's aim
//       point and capsule hit test, the attack-direction / shoot / sound-id pickers, the "can this hit" test,
//       the agent-table pool and the line-of-sight / fire-check scans. Functions in ascending address order
//       (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
shape_base* g_hit_shape;                                    // 0x5e246c: last shape hit by trace_ray


// signed_angle reads the x and y words of its two arguments (agent2/agent_types.h declares them vec2*); the agent's vel / fx are
// float records (vector4 or float[]) whose first two words are x and y, so the pointer is the same
static __forceinline const vec2* xy(const void* v) { return (const vec2*)v; }

// inline helpers: the original compiled them into the callers
static __forceinline vector4 smul(const float& k, const vector4& v) { return vector4(k * v.x, k * v.y, k * v.z); }
static inline vector4 vscale(const vector4& v, float k) { return vector4(v.x * k, v.y * k, v.z * k); }
static inline vector4 to_world(const matrix& m, const vector4& t)
{
    return vector4(t.x * m.s.x + (t.y * m.f.x + t.z * m.u.x) + m.o.x,
                   t.x * m.s.y + (t.y * m.f.y + t.z * m.u.y) + m.o.y,
                   t.x * m.s.z + (t.y * m.f.z + t.z * m.u.z) + m.o.z);
}
static inline vector4 rot_world(const matrix& m, const vector4& d)
{
    return vector4(m.s.x * d.x + (m.f.x * d.y + m.u.x * d.z),
                   m.s.y * d.x + (m.f.y * d.y + m.u.y * d.z),
                   m.s.z * d.x + (m.f.z * d.y + m.u.z * d.z));
}

// ---- font glyph metrics ---------------------------------------------------------------------------------------
static __forceinline const Glyph& font_glyph(Font* f, int ch)
{
    int key = ch;
    if (key >= 0x200) {
        IntHashBase::iterator it = ((IntHashBase&)f->map).lower_bound(key);
        GlyphNode* n = (GlyphNode*)it._Mynode();
        if (n != f->map.head)
            return n->val;
        return f->dflt;
    }
    return f->ascii[key];
}

// @ 0x43e190
float Font::advance(int ch)
{
    int key = ch;
    const Glyph& g = font_glyph(this, key);
    float r = g.f18 + g.f10;
    return r;
}

// @ 0x43e200
float Font::measure(const int* text, int len, int* start, int* end)
{
    float sum = 0.0f;
    bool in_tag = false;
    int i = *start;
    if (i < *end) {
        do {
            if (i >= len)
                break;
            if (in_tag) {
                if (text[i] == 0x3e)
                    in_tag = false;
            } else {
                int c = text[i];
                if (c == 0x3c)
                    in_tag = true;
                else if (c == 9)
                    sum = (float)(int)(sum * 4.0f) * 0.25f + 0.4f;
                else
                    sum += advance(c);
            }
            i++;
        } while (i < *end);
    }
    return sum;
}

// @ 0x43e2a0
template void std::vector<int, std::allocator<int> >::_Xlen() const;

// ---- container instantiations (VC7.1 <vector> / <deque>) -----------------------------------------------------------
// Built WITHOUT an EH switch: the STL try/catch alone makes cl emit the frames (with /EHa cl also keeps the ~E56 call
// out of line). The catch(...) funclets and cold tails (0x43e47a, 0x43e514, 0x43e910, 0x43ebc9, 0x43ec91, 0x43edd9,
// 0x43ef97) live inside the COMDAT of the function they belong to.
// @ 0x43e310 (+ 0x43e47a, 0x43e514)
template void std::vector<tstring>::_Insert_n(std::vector<tstring>::iterator, size_t, const tstring&);
// @ 0x43e590
template void std::deque<E32>::_Growmap(size_t);
// @ 0x43e6e0
template void std::_Destroy_range(E56*, E56*, std::allocator<E56>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x43e740
void Skeleton::ensure_updated()
{
    if (!done) {
        if (has_poses)
            ((FxAnim*)this)->update_poses();
        else
            ((FxAnim*)this)->update_poses2();
    }
}

// @ 0x43e760
template bool std::vector<int>::_Buy(size_t);
// (the pointer vectors' _Buy instances are the same code, folded into this one by the original linker: the party manager's and the sprite table's page lists)
template bool std::vector<Party*>::_Buy(size_t);
template bool std::vector<char*>::_Buy(size_t);
// 0x43e7b0 (+ 0x43e910): vector<int>::_Insert_n stays declared only (std_vec_out_of_line.h: a visible body costs 0x44c210 about 190 instructions);
// the instance this object emits at that address is the folded vector<RecJ*> one.
// @ 0x43ea00 (+ 0x43ebc9, 0x43ec91)
template void std::vector<PlacedObj>::_Insert_n(std::vector<PlacedObj>::iterator, size_t, const PlacedObj&);
// @ 0x43ed20 (+ 0x43edd9)
template std::vector<int>::vector(const std::vector<int>&);
// @ 0x43edf0 (+ 0x43ef97)
template void std::vector<E12>::_Insert_n(std::vector<E12>::iterator, size_t, const E12&);
// @ 0x43f0a0
template std::vector<int>& std::vector<int>::operator=(const std::vector<int>&);
// @ 0x43f1f0
template void std::deque<E32>::push_back(const E32&);

// @ 0x43f260
char* Skeleton::rec(int i)
{
    if (!done) {
        if (has_poses)
            ((FxAnim*)this)->update_poses();
        else
            ((FxAnim*)this)->update_poses2();
    }
    return recs + i * 0x1dc + 0x84;
}

// @ 0x43f2a0
template void std::vector<tstring>::push_back(const tstring&);
// @ 0x43f310
template void std::vector<int>::resize(size_t, int);
// @ 0x43f3a0
template std::vector<E48>::iterator std::vector<E48>::insert(std::vector<E48>::iterator, const E48&);
// 0x4108b0 is vector<E48>::_Insert_n (an earlier object)
template void std::vector<E48>::_Insert_n(std::vector<E48>::iterator, size_t, const E48&);
// @ 0x43f410
template std::vector<PlacedObj>::iterator std::vector<PlacedObj>::insert(std::vector<PlacedObj>::iterator, const PlacedObj&);
// @ 0x43f480
template void std::vector<E56>::_Destroy(E56*, E56*);
// @ 0x43f4a0
template std::vector<E12>::iterator std::vector<E12>::insert(std::vector<E12>::iterator, const E12&);

// @ 0x43f510
E56::E56(const E56& o)
    : a(o.a), b(o.b), c(o.c), v1(o.v1), v2(o.v2), d(o.d), e(o.e), f(o.f)
{
}

// ---- aim point and hit tests ------------------------------------------------------------------------------------
// One 0x1dc-byte animator record seen at skeleton->recs + 0x1140 (the head/hand bone used as aim origin).
struct AimRec {
    char pad0[0x10];
    vector4 offs;               // +0x10
    vector4 dir;                // +0x20
    vector4 base;               // +0x30
};
// @ 0x43f560
void Agent::aim_point_and_dir(vector4* pos_out, vector4* dir_out)
{
    Skeleton* sk = body->skel;
    if (!sk->done) {
        if (sk->has_poses)
            ((FxAnim*)sk)->update_poses();
        else
            ((FxAnim*)sk)->update_poses2();
    }
    AimRec* r = (AimRec*)(sk->recs + 0x1140);
    vector4 t = vadd(vscale(r->offs, 0.2f), r->base);
    const matrix& m = body->m;
    *pos_out = to_world(m, t);
    *dir_out = rot_world(body->m, r->dir);
}

// @ 0x43f6d0
bool Agent::capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone)
{
    capsule c;
    SkelData* sd = body->skel->data;
    for (int i = 0; i < sd->count; i++) {
        for (unsigned j = 0; ; j++) {
            if (j >= sd->items[i].caps.size())
                break;
            const CapDef& d = sd->items[i].caps[j];
            c.l0 = vmul(d.a, scale);
            c.l1 = vmul(d.b, scale);
            c.radius = d.r * scale;
            Skeleton* sk = body->skel;
            if (!sk->done) {
                if (sk->has_poses)
                    ((FxAnim*)sk)->update_poses();
                else
                    ((FxAnim*)sk)->update_poses2();
            }
            c.transform((const ::pos*)&(body->m * *(matrix*)(sk->recs + i * 0x1dc + 0x84)));
            contact ct;
            if (shape_contact_count(&c, other, 1, &ct)) {
                *out_point = ct.point;
                *out_a = ct.normal;
                *out_bone = i;
                return true;
            }
        }
    }
    return false;
}

// Which of the four attack directions (1 left, 2 right, 3 up?, 0 thrust) the agent picks; derived from the
// mouse movement (mode > 0) or from where the target stands relative to the view (mode 0).
// rgl::matrix::transform_point_to_local (0x407f70, src/rgl/shapes/shapes.cpp) with its body visible here (inline noinline
// duplicate, as map_panel.cpp): its argument does not escape
inline __declspec(noinline) vector4 rgl::matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
}
// @ 0x43f970
int Agent::pick_attack_dir(unsigned tid)
{
    int alt = 0;
    int dir;
    if (g_options.ctl_mode > 0) {
        float dx = g_keyboard.f818;
        float dy = g_keyboard.f81c;
        if (g_options.invert != 0)
            dy = -dy;
        if (g_options.ctl_mode == 2) {
            dx = -dx;
            dy = -dy;
        }
        if (dx > dy) {
            if (dx > -dy) {
                dir = 2;
            } else {
                dir = 0;
                alt = 3;
            }
        } else {
            dir = 1;
            if (!(dx < -dy))
                dir = 3;
        }
        int f = f2c;
        if (f < 0)
            goto done;
        float v = signed_angle(xy(&vel), xy(&fx));
        if ((&g_mission->agents.pages[(unsigned)f >> 4][(unsigned)f & 0xf])->fac > 1.0f) {
            if (v < -0.2f && dir == 2) {
                dir = 1;
                alt = 0;
                goto done;
            }
            if (!(v > 0.2f) || dir != 1)
                goto done;
            alt = 0;
        } else {
            if (v < -0.2f && dir == 2) {
                dir = 1;
                goto done;
            }
            if (!(v > 0.2f) || dir != 1)
                goto done;
        }
        dir = 2;
    } else {
        Agent* t = &g_mission->agents.pages[tid >> 4][tid & 0xf];
        AgentBody* tb = t->body;
        if (tb == 0)
            return 0;
        rgl::matrix cam = g_mission->view;
        { const float quarter = -1.5707964f; cam.matrix::rotate_about_s(&quarter); }
        vector4 p = t->pos;
        p.z = p.z + 0.7f;
        if (t->f10 == 0)
            p = ((const ::pos&)t->body->m).transform_point(*(vector4*)(tb->skel->rec(9) + 0x30));
        vector4 loc = cam.transform_point_to_local(p);
        float inv = 1.0f / loc.y;
        float x = loc.x * inv;
        float z = inv * loc.z;
        float a = 0.4f * z;
        if (x < a - 0.02f && x < -0.02f - a) {
            dir = 1;
            alt = 0;
        } else if (a + 0.02f < x && 0.02f - a < x) {
            dir = 2;
            alt = 0;
        } else if (z < -0.1f) {
            dir = 3;
            alt = 0;
        } else {
            dir = 0;
            alt = 2;
            if (!(x > 0.0f))
                alt = 1;
        }
        int f = f2c;
        if (f < 0)
            goto done;
        float v = signed_angle(xy(&vel), xy(&fx));
        if ((&g_mission->agents.pages[(unsigned)f >> 4][(unsigned)f & 0xf])->fac > 1.0f) {
            alt = 0;
            dir = (v < 0.0f) ? 1 : 2;
        } else {
            if (v < 0.0f && dir == 2) {
                dir = 1;
                goto done;
            }
            if (!(v > 0.0f) || dir != 1)
                goto done;
            dir = 2;
        }
    }
done:
    int snd_a_, snd_b_, snd_c_, var_;
    pick_sounds(0, dir, &snd_a_, &snd_b_, &snd_c_, &var_);
    if (snd_a_ == -1)
        dir = alt;
    return dir;
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

// Picks the animation and sound ids for starting a ranged attack with the item in the given slot: fixed ids
// for each itcf_shoot_* class, otherwise the pair chosen by pick_attack_dir / pick_sounds.
// @ 0x43fd10
void Agent::start_shoot(int slot)
{
    snd_a = -1;
    snd_b = -1;
    snd_c = -1;
    unsigned __int64 flags;
    ItemRef sp = slot_pair(slot);
    if (sp.item == -1)
        flags = 0;
    else
        flags = g_items[sp.item].flags;
    int dir, alt;
    if (f2c >= 0) {
        float v = signed_angle(xy(&vel), xy(&fx));
        if (!(v < 0.0f)) { dir = 2; } else { dir = 1; }
        alt = 0;
    } else {
        dir = rand() % 4;
        alt = rand() % 4;
        if (dir == alt)
            alt = 3 - dir;
    }
    int tgt = pick_front();
    if (id == g_mission->player_id && (g_options.ctl_mode != 0 || tgt != -1))
        dir = pick_attack_dir(tgt);
    unsigned __int64 kind = flags & itcf_shoot_mask;
    if (kind != 0) {
        if (kind == itcf_shoot_bow) {
            snd_b = 0x76;
            mode1 = 0;
            snd_a = 0x75;
            return;
        }
        if (kind == itcf_shoot_crossbow) {
            snd_b = 0x7a;
            mode1 = 0;
            snd_a = 0x79;
            return;
        }
        if (kind == itcf_shoot_pistol) {
            snd_b = 0x86;
            mode1 = 0;
            snd_a = 0x85;
            return;
        }
        if (kind == itcf_shoot_musket) {
            snd_b = 0x89;
            mode1 = 0;
            snd_a = 0x88;
            return;
        }
        if (kind == itcf_throw_stone) {
            snd_b = 0x84;
            mode1 = 0;
            snd_a = 0x83;
            return;
        }
        if (kind == itcf_throw_knife) {
            snd_b = 0x80;
            mode1 = 0;
            snd_a = 0x7f;
            return;
        }
        if (kind == itcf_throw_axe) {
            snd_b = 0x82;
            mode1 = 0;
            snd_a = 0x81;
            return;
        }
        if (kind == itcf_throw_javelin) {
            snd_a = 0x7d;
            snd_b = 0x7e;
        }
        mode1 = 0;
    } else {
        if (slot == 0) {
            mode1 = dir;
            pick_sounds(0, dir, &snd_a, &snd_b, &snd_c, &variant);
            if (snd_a == -1) {
                mode1 = alt;
                pick_sounds(0, alt, &snd_a, &snd_b, &snd_c, &variant);
            }
        } else {
            mode2 = dir;
            pick_sounds(slot, mode1, &snd2_a, &snd2_b, &snd2_c, &variant);
            if (snd2_a == -1) {
                mode2 = alt;
                pick_sounds(slot, mode1, &snd2_a, &snd2_b, &snd2_c, &variant);
            }
        }
    }
}

// @ 0x440070
// itcf_* capability bits are stored in the high dword of the item flags
#define ITCF(x) (((unsigned __int64)(x)) << 32)
void Agent::pick_sound_ids(int* a, int* b, int* c)
{
    *a = -1;
    *b = -1;
    *c = -1;
    int mode = 0;
    if (id != g_mission->player_id || g_options.stick_mode == 0) {
        int t = pick_duel();
        if (t != -1) {
            Agent* o = agent_by_id(t);
            if (o->active == 2)
                mode = o->mode1;
            else if (o->active == 1)
                mode = o->mode1;
        }
    } else if (g_options.stick_mode == 1) {
        float dx = g_keyboard.f818;
        float dy = g_keyboard.f81c;
        if (g_options.invert != 0)
            dy = -dy;
        if (dx > dy) {
            if (dx > -dy)
                mode = 2;
            else
                mode = 3;
        } else {
            if (dx < -dy)
                mode = 1;
            else
                mode = 0;
        }
    } else {
        mode = 0;
        int t = pick_front();
        if (t >= 0) {
            Agent* o = agent_by_id(t);
            AgentBody* tb = o->body;
            if (tb != 0) {
                rgl::matrix cam = g_mission->view;
                { const float quarter = -1.5707964f; cam.matrix::rotate_about_s(&quarter); }
                vector4 p = o->pos;
                p.z = p.z + 0.7f;
                if (o->f10 == 0)
                    p = ((const ::pos&)o->body->m).transform_point(*(vector4*)(tb->skel->rec(9) + 0x30));
                vector4 q = cam.transform_point_to_local(p);
                float inv = 1.0f / q.y;
                float x = q.x * inv;
                float z = inv * q.z;
                float k = 0.4f * z;
                if (x < k - 0.02f && x < -0.02f - k) mode = 2;
                else if (k + 0.02f < x && 0.02f - k < x) mode = 1;
                else if (z < -0.1f) mode = 3;
                else mode = 0;
            }
        }
    }
    unsigned __int64 flags = 0;
    ItemRef p0 = slot_pair_inl(0);
    if (p0.item != -1)
        flags = g_items[p0.item].flags;
    ItemRef p1 = slot_pair_inl(1);
    if (p1.item > 0) {
        *a = 0xc7; *b = 0xc8; *c = 0xc9;
        return;
    }
    if (p0.item <= 0) {
        *a = 0x97; *b = 0x98; *c = 0x99;
        return;
    }
    unsigned __int64 pole = flags & ITCF(itcf_thrust_polearm);
    if (mode == 0) {
        if (pole) { *a = 0xd6; *b = 0xd7; *c = 0xd8; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xca; *b = 0xcb; *c = 0xcc; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xe2; *b = 0xe3; *c = 0xe4; return; }
    } else if (mode == 2) {
        if (pole) { *a = 0xd0; *b = 0xd1; *c = 0xd2; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xd0; *b = 0xd1; *c = 0xd2; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xe8; *b = 0xe9; *c = 0xea; return; }
    } else if (mode == 1) {
        if (pole) { *a = 0xd3; *b = 0xd4; *c = 0xd5; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xd3; *b = 0xd4; *c = 0xd5; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xeb; *b = 0xec; *c = 0xed; return; }
    } else {
        if (pole) { *a = 0xd9; *b = 0xda; *c = 0xdb; return; }
        if (flags & ITCF(itcf_shoot_bow)) { *a = 0xcd; *b = 0xce; *c = 0xcf; return; }
        if (flags & ITCF(itcf_throw_stone)) { *a = 0xe5; *b = 0xe6; *c = 0xe7; }
    }
}

// 0x4378f0 (Agent::get_dir, agent2/agent_stats.cpp owns the row): the body visible, so the direction pair passed by address does not escape
// in Agent::can_hit (0x4406a0) and the slot_pair temporaries share its frame slot.
inline __declspec(noinline) void Agent::get_dir(vec2* out)
{
    vec2 v = *(vec2*)&vel;
    float l = (float)sqrt(v.x * v.x + v.y * v.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        v.x = inv * v.x;
        v.y = inv * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
    if (f10 == 0 && f2c != -1 && (active != 0 || f5610 != 0)) {
        float t = cur;
        float c = (float)cos((double)t);
        float s = (float)sin((double)t);
        float nx = c * v.x - s * v.y;
        float tt = c * v.y;
        v.y = tt + s * v.x;
        v.x = nx;
    }
    *out = v;
}
struct Word_ { int v; Word_(int i) : v(i) {} };   // a class-typed argument: a fastcall never passes it in a register
// @ 0x4406a0
int Agent::can_hit(unsigned aid, unsigned bid, int alt, int flag, const float* target)
{
    Agent* a = agent_by_id(aid);
    Agent* b = agent_by_id(bid);
    bool strike = flag != 0;
    int mode;
    if (a->f100_is_0x48_and_charged())
        mode = 0;
    else if (alt == 0)
        mode = a->mode1;
    else
        mode = a->mode2;
    int act = b->anim[1].cur;
    int grp = group_base(act);
    int not_base = not_group_base(act);
    float dx = b->pos.x - a->pos.x;
    float dy = b->pos.y - a->pos.y;
    float len = (float)sqrt(dy * dy + dx * dx);
    struct { float x, y; } u, f;
    if (len > 0.0f) {
        float inv = 1.0f / len;
        u.x = inv * dx;
        u.y = inv * dy;
    } else {
        u.x = 0.0f;
        u.y = 1.0f;
    }
    b->get_dir((vec2*)&f.x);
    if (strike) {
        if (grp == 199 && f.y * u.y + f.x * u.x < -0.4f) {
            matrix m = b->body->m * *(const matrix*)b->body->skel->rec(12);
            vector4 hp = vadd(m.o, vmul(m.f, 0.2f));
            float pz = hp.z;
            float lo = 0.0f, hi = 0.0f;
            ItemRef sp = b->slot_pair_inl(1);
            if (sp.item >= 0) {
                lo = (float)g_items[sp.item].weight * 0.01f;
                if (lo == 0.0f)
                    lo = 0.5f;
                hi = lo;
                if (lo > 0.5f)
                    hi = 0.5f;
            }
            if (b->troop >= 0) {
                int k = g_game->skill_level(troop, 0x1a);
                float s = (float)k * 0.02f + 1.0f;
                float u2 = (float)k * 0.1f;
                hi = s * hi + u2;
                lo = s * lo + u2;
            }
            float z = target[2] - pz;
            if (-lo < z && z < hi)
                return 1;
        }
        return 0;
    } else {
        if (mode == 2 || mode == 1) {
            if (0.3f < f.y * u.y + f.x * u.x)
                return 0;
        } else if (-0.2f < f.y * u.y + f.x * u.x)
            return 0;
        if (not_base == 0 && b->anim[1].scale < 0.25f)
            return 0;
        if (grp == 0x97) {
            ItemRef sp = a->slot_pair_inl(0);
            if (sp.item < 0)
                return 1;
        }
        return ((int (__fastcall*)(void*, Word_, Word_))::id_matches_dir)(this, Word_(grp), Word_(mode));   // 0x4326d0: `this` is loaded and ignored
    }
}

// more container instantiations
// @ 0x440a30
template E56* std::_Copy_backward_opt(E56*, E56*, E56*, std::_Nonscalar_ptr_iterator_tag);
// @ 0x440aa0
template void std::vector<E48>::push_back(const E48&);
// @ 0x440b30
template void std::vector<int>::push_back(const int&);
// @ 0x440b80
template void std::vector<PlacedObj>::push_back(const PlacedObj&);
// @ 0x440c10
template void std::vector<E12>::push_back(const E12&);

// ---- id pool ---------------------------------------------------------------------------------------------------
// @ 0x440c90
int Pool::alloc()
{
    if (!(free_ids.size() > 0)) {
        RecJ* blk = new RecJ[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].f4 = 0;
            blk[j].f8 = next_id + j;
            free_ids.push_back(next_id + j);
        }
        next_id += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    RecJ* r = &pages[id >> 4][id & 0xf];
    r->f4 = 1;
    r->fc = serial;
    serial++;
    return id;
}

// @ 0x440dc0
template void std::fill(E56*, E56*, const E56&);

// ---- agent line of sight / fire check ----------------------------------------------------------------------------
// @ 0x440e30
bool Agent::line_of_sight(unsigned tid, shape_base* ignore)
{
    Agent* o = agent_by_id(tid);
    bool r = false;
    vector4 d;
    d.x = o->pos.x - pos.x;
    d.y = o->pos.y - pos.y;
    d.z = o->pos.z - pos.z;
    if (d.normalize() <= 300.0f && body != 0 && o->body != 0) {
        vector4 p, dir;
        aim_point_and_dir(&p, &dir);
        vector4 head = ((const ::pos&)o->body->m).transform_point(*(const vector4*)(o->body->skel->recs + 0x1170));
        head.z += 0.2f;
        vector4 t;
        t.x = head.x - p.x;
        t.y = head.y - p.y;
        t.z = head.z - p.z;
        d = t;
        float len = d.normalize();
        float dp = dir.z * d.z + dir.y * d.y + dir.x * d.x;
        if (ignore) {
            capsule c(0.01f, &p, &head);
            if (shape_contact_count(&c, ignore, 0, 0))
                return false;
        }
        if (dp > -0.3f || len < 2.0f)
            r = g_mission->scene->trace_ray(&p.x, &head.x, 400.0f, &g_hit_shape, 1) != 0;
    }
    return r;
}

static __forceinline bool mission_friends(Mission* m, const int& a, const int& b)
{
    if (a >= 0 && a < 8 && b >= 0 && b < 8)
        return ((const int*)&m->teams[0].relation[0])[a * 0x5b + b] == 1;
    return false;
}
// @ 0x441020
int Agent::fire_check(unsigned tid, int* out_blocker)
{
    g_hit_shape = 0;
    *out_blocker = -1;
    if (f2c != -1) {
        float v = signed_angle(xy(&vel), (const vec2*)blk294);
        if (!(g_range_tbl[variant].lo <= v) || !(v < g_range_tbl[variant].hi))
            return 3;
    }
    if (!line_of_sight(tid, 0))
        return 0;
    const vector4& tp = agent_by_id(tid)->pos;
    float score = 0.0f;
    vector4 a;
    a.x = tp.x - pos.x;
    a.y = tp.y - pos.y;
    float dist;
    {
        a.z = tp.z - pos.z;
        float dz = a.z;
        dist = a.normalize();
        float reach = attack_speed();
        a.z = (accel_factor(&reach, &dist, &dz) + a.z) * 0.1f;
    }
    a.normalize();
    float best = 0.0f;
    unsigned i = g_mission->agents.first_used();
    if (i < (unsigned)g_mission->agents.count) {
        do {
            Agent* o = &g_mission->agents.pages[i >> 4][i & 0xf];
            if (o->f10 == 0 && mission_friends(g_mission, team, o->team) && o->f14 == 1 && o->id != id) {
                vector4 b;
                b.x = o->pos.x - pos.x;
                b.y = o->pos.y - pos.y;
                b.z = (o->pos.z - pos.z) * 0.1f;
                float bl = b.normalize();
                float dp = b.x * a.x;
                dp += b.y * a.y;
                dp += b.z * a.z;
                if (dp > 0.2f) {
                    float s = (float)sqrt(1.0f - dp * dp);
                    float t = s - ((o->v56b4.out_length() + 1.0f) * 0.4f) / (bl + 0.2f);
                    if (t < 0.0f)
                        t = 0.0f;
                    if (bl > dist)
                        t = ((bl - dist) + 25.0f) * t * 0.051999997f;
                    t = (0.3f - t) * 3.3333333f;
                    if (t < 0.01f)
                        t = 0.0f;
                    t = (t * t) * (t * t);
                    if (t > 0.7f) {
                        score += t;
                        if (t > best) {
                            best = t;
                            *out_blocker = i;
                        }
                    }
                }
            }
            g_mission->agents.next_used(&i);
        } while (i < (unsigned)g_mission->agents.count);
        if (score > 0.9f)
            return 2;
    }
    return 1;
}

//=============================================================================
// 0x441360..0x441ff0, from phys_set.cpp
// Desc: the Newton physics set: destroy a world record, add a ragdoll bone, start a static tree-collision
//       build, add a capsule rigid body; and Agent::build_static_collision, which fills the tree collision from
//       the static shapes around the agent and adds a body per capsule. Functions in ascending address order.
//       Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
PhysSet g_phys;                                             // 0xa181f8 the physics set (its constructor and destructor run as dynamic initialisers)

// @ 0x441360
void PhysSet::destroy_world(int idx)
{
    if (recs[idx].world != 0) {
        recs[idx].ragdoll = 0;
        recs[idx].collision = 0;
        recs[idx].body = 0;
        recs[idx].bones.resize(0, 0);
        recs[idx].bodies.resize(0, 0);
        NewtonDestroy(recs[idx].world);
        recs[idx].world = 0;
        nworlds--;
    }
}

// @ 0x4413e0
const NewtonRagDollBone* PhysSet::add_bone(int idx, const NewtonRagDollBone* parent, const float* mass, matrix m0,
                                           const float* radius, const float* height, matrix m1, const float* cone_dir,
                                           const float* lim_min, const float* lim_max, const float* lim_twist)
{
    PhysRec* r;
    m1.s.w = 0.0f; m1.f.w = 0.0f; m1.u.w = 0.0f; m1.o.w = 1.0f;
    m0.s.w = 0.0f; m0.f.w = 0.0f; m0.u.w = 0.0f; m0.o.w = 1.0f;
    r = &recs[idx];
    NewtonCollision* cap = NewtonCreateCapsule(r->world, *radius, *height, &m1.s.x);
    float v[4];
    v[0] = 0.0f; v[1] = 0.0f; v[2] = 0.0f;
    const NewtonRagDollBone* bone = NewtonRagDollAddBone(recs[idx].ragdoll, parent, 0, *mass, &m0.s.x, cap, v);
    NewtonReleaseCollision(recs[idx].world, cap);
    NewtonBody* body = NewtonRagDollBoneGetBody(bone);
    float inertia[4];
    float origin[4];
    NewtonConvexCollisionCalculateInertialMatrix(cap, inertia, origin);
    NewtonBodySetCentreOfMass(body, origin);
    NewtonBodySetMassMatrix(body, *mass, inertia[0], inertia[1], inertia[2]);
    NewtonBodySetMaterialGroupID(body, recs[idx].material);
    v[0] = 0.7f; v[1] = 0.7f; v[2] = 0.7f;
    NewtonBodySetAngularDamping(body, v);
    NewtonBodySetFreezeTreshold(body, 0.25f, 0.7f, 5);
    NewtonRagDollBoneSetLimits(bone, cone_dir, *lim_min * 0.017453292f, *lim_max * 0.017453292f, *lim_twist * 0.017453292f, cone_dir, 0.2f, 0.2f);
    recs[idx].bones.push_back(bone);
    return bone;
}

// @ 0x441610
void PhysSet::begin_static_build(int idx)
{
    if (recs[idx].collision != 0) {
        NewtonDestroyBody(recs[idx].world, recs[idx].body);
        recs[idx].collision = 0;
        recs[idx].body = 0;
        for (unsigned i = 0; i < recs[idx].bodies.size(); i++)
            NewtonDestroyBody(recs[idx].world, recs[idx].bodies[i]);
        recs[idx].bodies.resize(0, 0);
    }
    recs[idx].collision = NewtonCreateTreeCollision(recs[idx].world, 0);
    NewtonTreeCollisionBeginBuild(recs[idx].collision);
}

// @ 0x4416c0
NewtonBody* PhysSet::add_capsule_body(int idx, const float* radius, const float* height, const float* mass, matrix* m0, matrix* m1, bool list)
{
    m0->s.w = 0.0f; m0->f.w = 0.0f; m0->u.w = 0.0f; m0->o.w = 1.0f;
    m1->s.w = 0.0f; m1->f.w = 0.0f; m1->u.w = 0.0f; m1->o.w = 1.0f;
    NewtonCollision* cap = NewtonCreateCapsule(recs[idx].world, *radius, *height, &m1->s.x);
    NewtonBody* body = NewtonCreateBody(recs[idx].world, cap);
    NewtonReleaseCollision(recs[idx].world, cap);
    NewtonBodySetMatrix(body, &m0->s.x);
    float inertia[4];
    float origin[4];
    NewtonConvexCollisionCalculateInertialMatrix(cap, inertia, origin);
    NewtonBodySetCentreOfMass(body, origin);
    NewtonBodySetMassMatrix(body, *mass, inertia[0], inertia[1], inertia[2]);
    NewtonBodySetMaterialGroupID(body, recs[idx].material);
    if (list)
        recs[idx].bodies.push_back(body);
    return body;
}

typedef matrix frame;   // a 4-row frame as the original builds it inline: three rows from identity3, origin zero

// the original inlines vector4::normalize (returns the old length) and the out-parameter cross product
static __forceinline float vnorm(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x *= inv; v.y *= inv; v.z *= inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}
static __forceinline void vcross(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
}

static __forceinline vector4 cross3(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
    return r;
}

// @ 0x4417d0
void Agent::build_static_collision()
{
    anchor = body->m.o;
    g_phys.begin_static_build(phys_idx);
    shape_base* hits[0x2000];
    int cnt;
    {
        vector4 bmin, bmax;
        bmin.x = body->m.o.x - 5.0f;
        bmin.y = body->m.o.y - 5.0f;
        bmin.z = body->m.o.z - 5.0f;
        bmax.x = body->m.o.x + 5.0f;
        bmax.y = body->m.o.y + 5.0f;
        bmax.z = body->m.o.z + 5.0f;
        g_mission->scene->query_box(&bmin.x, &bmax.x, 0x8106, hits, &cnt);   // scene is the scene object Ctx (strat/strat_entity.h)
    }
    int i;
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 2) {
            face* f = (face*)hits[i];
            int tri[12];
            tri[0] = *(int*)&f->w[0].x; tri[1] = *(int*)&f->w[0].y; tri[2] = *(int*)&f->w[0].z;
            tri[3] = *(int*)&f->w[1].x; tri[4] = *(int*)&f->w[1].y; tri[5] = *(int*)&f->w[1].z;
            tri[6] = *(int*)&f->w[2].x; tri[7] = *(int*)&f->w[2].y; tri[8] = *(int*)&f->w[2].z;
            tri[9] = *(int*)&f->w[3].x; tri[10] = *(int*)&f->w[3].y; tri[11] = *(int*)&f->w[3].z;
            NewtonTreeCollisionAddFace(g_phys.recs[phys_idx].collision, ((face*)hits[i])->count, (float*)tri, 12, 0);
        }
    }
    g_phys.rebuild_static_body(phys_idx);
    for (i = 0; i < cnt; i++) {
        if (hits[i]->shape_type() == 1) {
            capsule* c = (capsule*)hits[i];
            vector4 dir;
            int radius = *(int*)&c->radius;
            dir.x = c->p1.x - c->p0.x; dir.y = c->p1.y - c->p0.y; dir.z = c->p1.z - c->p0.z;
            float len = vnorm(dir);
            int zero = 0;
            frame R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            mat3 t;
            mat3* ip = identity3(&t);
            *(mat3*)&R = *ip;
            R.s = dir;
            R.o = z0;
            vector4 c1 = cross3(R.s, R.f);
            if (c1.length() < 0.001f) {
                R.f = cross3(R.u, R.s);
                vnorm(R.f);
                R.u = cross3(R.s, R.f);
                vnorm(R.u);
            } else {
                R.u = c1;
                vnorm(R.u);
                R.f = cross3(R.u, R.s);
                vnorm(R.f);
            }
            vector4 zero2(0.0f, 0.0f, 0.0f);
            vector4 mid(c->p0.x + c->p1.x, c->p0.y + c->p1.y, c->p0.z + c->p1.z);
            mid.x *= 0.5f; mid.y *= 0.5f; mid.z *= 0.5f;
            R.o = mid;
            frame M;
            mat3 t2;
            mat3* ip2 = identity3(&t2);
            *(mat3*)&M = *ip2;
            M.o = zero2;
            g_phys.add_capsule_body(phys_idx, (const float*)&radius, &len, (const float*)&zero, &R, &M, true);
        }
    }
}

//=============================================================================
// 0x441ff0..0x444c50, from agent_fx_sound.cpp
// Desc: an object that keeps int lists, the 0x38-byte record copy / fill helpers, the world's "entities fully
//       touching a box" query, the effect element / definition lists, the emitter setup from the definition
//       table, the FMOD sound player, game-data experience (level-up bookkeeping and its messages), the mission's
//       effect spawn and dead-agent queue, the agent's target scans, sound pickers and players, the equipment
//       display and the wield-change animation / sound selection. Functions in ascending address order (cl's
//       bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
#include "../agent3/agent_fx_sound.h"   // after the 0x43e190.. section: its itcf_* macros are i64, that section's enum is int
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


const int g_xp_table[63] = {                             // 0x590974
    0, 600, 1360, 2296, 3426, 4768, 6345, 8179, 10297, 13010, 16161, 19806, 24007, 28832, 34362, 40682, 47892, 56103, 65441, 77233,
    90809, 106425, 124371, 144981, 168636, 195769, 226879, 262533, 303381, 350164, 412091, 484440, 568947, 667638, 782877, 917424,
    1074494, 1257843, 1471851, 1721626, 2070551, 2489361, 2992033, 3595340, 4319408, 5188389, 6231267, 7482821, 8984785, 11236531,
    14051314, 17569892, 21968215, 27466219, 34338823, 42929679, 53668349, 67091786, 83871183, 160204600, 320304600, 644046000, 2050460000 };


// 0x440b30 vector<int>::push_back (agent_combat.cpp) is kept out of line


template<class T> __forceinline void vpush(std::vector<T>& v, const T& x)
{
    if (v.size() < v.capacity()) {
        T* p = v._Mylast;
        *p = x;
        v._Mylast = p + 1;
    } else {
        v.insert(v.end(), x);
    }
}

// @ 0x441ff0
void Poser::add_to_list(int idx, int val)
{
    vpush(lists[idx], val);
    if (idx + 1 > nlists)
        nlists = idx + 1;
}

// ---- std::_Uninit_copy for the 0x38-byte E56 record ----------------------------------------------------------
// @ 0x442060
void alloc_e56::construct(E56* p, const E56& v) { new ((void*)p) E56(v); }

// @ 0x442080 (+ catch-all funclet 0x4420ea: destroys [next, dest), then `throw;`)
E56* __cdecl uninit_copy_e56(const E56* first, const E56* last, E56* dest, alloc_e56& al)
{
    E56* next = dest;
    try {
        for (; first != last; ++dest, ++first)
            al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next)
            al.destroy((VecPair2*)next);
        throw;
    }
    return dest;
}

// @ 0x442110
void Poser::add_extra(int val)
{
    vpush(extra, val);
    changed();
}

// ---- the world's box query --------------------------------------------------------------------------------------------
static __forceinline bool box3_inl(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return true;
    return false;
}

// @ 0x442180
void rgl::world::collect_boxed(const vector4* bmin, const vector4* bmax, std::vector<unsigned>* out)
{
    if (out->_Myfirst != 0)
        operator delete(out->_Myfirst);
    out->_Myfirst = 0;
    out->_Mylast = 0;
    out->_Myend = 0;
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
                    out->push_back(*(unsigned*)pe);
            }
        }
    }
    std::vector<rgl::entity*>& ents = *(std::vector<rgl::entity*>*)out;      // the grid collects entity pointers
    sort_range(ents.begin(), ents.end(), (int)(out->end() - out->begin()));
    std::vector<unsigned>::iterator e = std::unique(out->begin(), out->end());
    out->erase(e, out->end());
}

// ---- effect elements / definitions ----------------------------------------------------------------------------------------
// @ 0x442390
void FxElem::add_psys(int p) { vpush(*(std::vector<int>*)&psys, p); }   // the list holds Psys* stored as ints
// @ 0x4423f0
void FxElem::add_group(int g)
{
    vpush(*(std::vector<int>*)&groups, g);
    f1ac++;
}
// @ 0x442460
void FxDef::attach_psys(int idx, Psys* p, const mat4* m)
{
    p->frame = *m;
    elems[idx].add_psys((int)p);
}
// @ 0x4424a0
void FxDef::attach_group(int idx, PsysGroup* g)
{
    elems[idx].add_group((int)g);
}

// ---- emitter setup from a definition table ----------------------------------------------------------------------------------
// @ 0x4424c0
void EmitTable::setup(rgl::psys* e, int idx, const vector4* pos)
{
    if (idx < 0 || idx >= count)
        return;
    EmitDef* d = &defs[idx];
    rgl::mesh* r = (rgl::mesh*)g_resmgr.get_object(&d->name);
    if (r == 0)
        return;
    e->init();
    e->tag = idx;
    e->src = r;
    e->res = r->res;
    e->flags = d->f10;
    e->f198 = d->f24;
    e->fa0 = d->f30;
    e->f24 = d->f34;
    vector4 g(0.0f, 0.0f, -9.8f);
    e->vc4 = g;
    e->fd4 = d->f38;
    e->f1c = d->f3c;
    e->f20 = d->f40;
    for (int k = 0; k < 2; k++) {
        e->curve[0 + k] = d->t[0][k];
        e->curve[2 + k] = d->t[1][k];
        e->curve[4 + k] = d->t[2][k];
        e->curve[6 + k] = d->t[3][k];
        e->curve[8 + k] = d->t[4][k];
    }
    e->f1b0 = d->fb8;
    e->f1b4 = d->fbc;
    mat4 m;
    rgl::identity_pos(&m);
    m.o = *pos;
    ((Obj3*)e)->set_placement(&m, &d->a, &d->b, d->c);
}

// ---- the FMOD sound player ---------------------------------------------------------------------------------------------------
static __forceinline int round_i(float v) { return round_float(&v); }
// @ 0x442650
int SoundMgr::play(int idx, int flags, const float* pos, float freq, int* out_id)
{
    int ret = -1;
    if (g_cfg.disable_sound == 0 && idx >= 0 && idx < ngroups) {
        int* pn = &groups[idx].nvar;
        int n = *pn;
        if (n > 0) {
            int k = 0;
            if (n > 1)
                k = rand() % *pn;
            int dn = groups[idx].var[k];
            int ch = FSOUND_PlaySoundEx(-1, defs[dn].sample, 0, 1);
            ret = ch;
            if (out_id)
                *out_id = defs[dn].id;
            if (ch >= 0) {
                int looping = groups[idx].flags & 2;
                int hi = 0x100;
                int r = round_i(((const Agent*)&groups[idx])->volume_scale() * g_options.master_volume * 255.0f);
                int vol = 0;
                vol = clamp_index(&r, &vol, &hi);
                int prio;
                if (flags & 1) {
                    FSOUND_3D_SetAttributes(ch, 0, 0);
                    prio = 0xff;
                } else {
                    FSOUND_3D_SetAttributes(ch, pos, 0);
                    int lo = 0;
                    int hi2 = 0x100;
                    float dist = (frame.o - *(const vector4*)pos).out_length();
                    float f = 3.0f / (dist + 3.0f);
                    int pr = round_i(((const Agent*)&defs[dn])->range_scale() * f * 254.0f);
                    prio = clamp_index(&pr, &lo, &hi2);
                }
                FSOUND_SetVolume(ch, vol);
                FSOUND_SetLoopMode(ch, (looping != 0) + 1);
                if (groups[idx].flags & 4)
                    FSOUND_SetCurrentPosition(ch, rand_below(&defs[dn].length));
                if (looping) {
                    int t = ch;
                    channels.push_back(t);
                }
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
int SoundMgr::play_slot(int n, int flags, const float* pos, float freq, int* out_id)
{
    n = slot[n];
    return play(n, flags, pos, freq, out_id);
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
void GameData::add_experience(int troop, int amount)
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
    TroopRec* c = &chars[troop];
    c->exp += amount;
    while (c->exp >= g_xp_table[c->level]) {
        c->fa4++;
        c->fa8++;
        c->fac += 10;
        c->level++;
        if (troop == player) {
            g_sound_mgr.play(g_sound_mgr.slot[1], 1, ((const float*)&g_sound_mgr.frame.o), 1.0f, 0);
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
            int p = pm->lookup(f78);
            if (pm->pages._Myfirst[p >> 4][p & 0xf].count > 0) {
                do {
                    int q = pm->lookup(f78);
                    if (pm->pages._Myfirst[q >> 4][q & 0xf].stacks[i].troop == troop) {
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
                    int r = pm->lookup(f78);
                    if (!(i < pm->pages._Myfirst[r >> 4][r & 0xf].count))
                        break;
                } while (1);
            }
        }
    }
}

// ---- spawning an effect emitter ---------------------------------------------------------------------------------------------------
// @ 0x442ca0
rgl::psys* Mission::spawn_effect(int idx, const mat4* m, int duration)
{
    rgl::psys* e = ((rgl::world*)scene)->alloc_emitter();
    vector4 v(0.0f, 0.0f, 0.0f);
    g_emit_defs.setup(e, idx, &v);
    e->mf4 = *m * e->m134;
    if (e->res != 0 && (e->res->flags & 0x40)) {
        vector4 t;
        t.x = g_env_rows[cam].v20[0] * 0.3f;
        t.y = g_env_rows[cam].v20[1] * 0.3f;
        t.z = g_env_rows[cam].v20[2] * 0.3f;
        EnvRow* c = &g_env_rows[cam];
        v.x = t.x + c->base[0];
        v.y = t.y + c->base[1];
        v.z = t.z + c->base[2];
        vector4* d = &e->vd8;
        *d = v;
    }
    e->flags &= ~1u;
    e->spawn_left = e->f1a0 = duration;
    e->f1a4 = 0;
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
    if (o->f14 == 1
        && (!(flags & 1) || !is_enemy(o))
        && (!(flags & 2) || !is_friend(o))
        && (!(flags & 4) || !(char)is_neutral(o))
        && (!(flags & 0x10) || o->f10 == 1)
        && (!(flags & 0x20) || o->f10 != 1)
        && (!(flags & 0x200) || o->id > id)
        && o->id != id
        && (!(flags & 0x2000) || line_of_sight(aid, 0))
        && o->id != f2c) {
        vector4 d = o->pos - pos;
        float len = d.normalize();
        *out_dist = len;
        if (maxdist >= 0.0f && len > maxdist)
            return false;
        if (flags & 0x1000) {
            float dot = d.x * fx + (d.z * fz + d.y * fy);
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
        if (o->f14 == 1) {
            bool friendly = false;
            if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
                friendly = m->rel_flat()[team * 0x5b + o->team] == 1;
            if (!friendly && !(char)is_neutral(o) && o->f10 != 1 && o->id != id && o->id != f2c) {
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
    if (f2c4 & 1)
        return;
    bool hit = false;
    AgentTable* t = &g_mission->agents;
    unsigned i = t->first_used();
    unsigned n = t->count;
    while (i < n) {
        if (hit)
            break;
        Agent** pg = g_mission->agents.pages._Myfirst;
        Agent* o = pg[i >> 4] + (i & 0xf);
        if (o->f14 == 1 && !is_enemy(o) && !(char)is_neutral(o) && o->f10 != 1 && o->id != id && o->id != f2c) {
            vector4 d = o->pos - pos;
            if (!(10.0f < d.normalize()))
                hit = (pg[i >> 4][i & 0xf].f2c4 & 1) != 0;
        }
        g_mission->agents.next_used(&i);
    }
    if (hit || any_hostile_near()) {
        f2c4 |= 1;
        f2c0 = 0;
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
                float w = 1.0f / ((float)m->agents[id].f55e8 + 7.0f);
                float k = 1.0f;
                if (f2c >= 0 && dist < 40.0f) {
                    vector4 d;
                    const vector4& op = m->agents.pages[id >> 4][id & 0xf].pos;
                    d.x = op.x - pos.x;
                    d.y = op.y - pos.y;
                    d.z = op.z - pos.z;
                    dist = d.normalize();
                    const vector4& fw = body->m.f;
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
    if (0 < nthreat) {
        Threat* r = threats;
        do {
            id = r->id;
            Agent* o = &g_mission->agents.pages[id >> 4][id & 0xf];
            if (o->f14 == 1 && line_of_sight(id, 0)) {
                if (fire_ok(this, id)) {
                    const vector4& op = g_mission->agents.pages[id >> 4][id & 0xf].pos;
                    float dx = op.x - pos.x;
                    float dy = op.y - pos.y;
                    float dz = op.z - pos.z;
                    float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                    if (!(v < 4.0f)) v = 4.0f;
                    float score = v / ((float)g_mission->agents.pages[(unsigned)r->id >> 4][r->id & 0xf].f55e8 + 7.0f);
                    float s = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * score;
                    if (s > bestv) { bestv = s; best = r->id; }
                }
            }
            n++;
            r++;
            id = best;
        } while (n < nthreat);
    }
    return id;
}

// @ 0x4435c0
void Agent::play_hit_sound(int* kind)
{
    Mission* m = g_mission;
    if (m->terrain.muted)
        return;
    int snd;
    if (f10 == 0) {
        int k = *kind;
        if (k < 0)
            return;
        if (k > 1) {
            if (k != 3)
                return;
            snd = 0xe;
        } else if (!(pos.z < m->terrain.base_z)) {
            bool b = (g_game->slots[m->f5df4].flags8 & 1) != 0;
            if (b)
                snd = 0x10;
            else
                snd = 0xf;
        } else {
            snd = 0x11;
        }
    } else {
        int k = *kind;
        unsigned c = fb8 & 7;
        if (k == 4) {
            snd = 0x61;
        } else if (pos.z > m->terrain.base_z) {
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
    g_sound_mgr.play(g_sound_mgr.slot[snd], 0, &body->m.o.x, pitch, 0);
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
        if (id == g_mission->player_id)
            own = 1;
        vector4 p = body->m.o;
        float h = (f10 == 0 && f2c != -1) ? 2.4f : 1.5f;
        p.z += h - 0.3f;
        g_sound_mgr.play(g_sound_mgr.slot[*kind], own, &p.x, pitch, (int*)&pitch);
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
        if (f10 == 0 && (g_game->chars[troop].flags32 & 0xf) != 1 && id != m->player_id) {
            float v = ((float)(((f1bc * 0x244ad + (id + 0x1d) * 0x14fd + troop * 0x76ef) % 0x1b41) * 0x51f % 0x367)
                       * 0.0011481056f - 0.5f) * 0.3f;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f) {
                float nv = -v;
                pitch = 1.0f / (nv + 1.0f);
            }
        }
        int own = 0;
        if (id == m->player_id)
            own = 1;
        int ch = g_sound_mgr.play(*snd, own, &body->m.o.x, pitch, (int*)&pitch);
        if (ch >= 0) {
            channel = ch;
            stamp = g_ticks[f5860];
            f5870 = pitch;
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
    g_sound_mgr.play(g_sound_mgr.slot[0x11], 0, &body->m.o.x, pitch, 0);
}

// ---- the equipment display ----------------------------------------------------------------------------------------------------------------
static inline unsigned char itype(const ItemKind* k) { return *(const unsigned char*)&k->type_flags; }

// shows `m` on the carry bone record `r` with local frame `f`
static __forceinline void attach(Agent* a, Mesh* m, int r, const pos2& f)
{
    Skeleton* g = a->body->skel;
    g->remove_shape((shape_base*)m);
    m->frame = f;
    Region& reg = ((Region*)g->recs)[r];
    vpush((std::vector<int>&)reg.shapes, (int)m);
    reg.count++;
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
                A[i]->vis = 0;
                if (itype(ik) == itp_type_shield) {
                    Mesh* m = A[i];
                    int k = 0;
                    if (0 < m->nlists) {
                        float limit = 300.0f;
                        do {
                            m->lists[k].cap_all(&limit);
                            k++;
                        } while (k < m->nlists);
                        k = 0;
                    }
                }
            }
            if (B[i] != 0)
                B[i]->vis = 0;
            if (C[i] != 0) {
                C[i]->vis = 0;
                Mesh* m = C[i];
                int k = 0;
                if (0 < m->nlists) {
                    float limit = 0.0f;
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->nlists);
                    k = 0;
                }
            }
            slot_used[i] = 0;
        }
    }
    int j;
    for (j = 0; j < 2; j++) {
        int s = wield[j];
        if (s >= 0 && pairs[s].item >= 0 && (j == 0 || f23c == 0)) {
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
            pos2 M;
            *(mat3*)&M = *ip;
            M.o = z0;
            if ((g_items[pairs[s].item].flags & itcf_shoot_mask) == itcf_throw_javelin) {
                M.f = vector4(-M.f.x, -M.f.y, -M.f.z);
                M.s = vector4(-M.s.x, -M.s.y, -M.s.z);
            }
            if (A[wield[j]] != 0) {
                A[wield[j]]->vis = 0xffff;
                Mesh* m = A[wield[j]];
                float limit = 0.0f;
                int k = 0;
                if (0 < m->nlists) {
                    do {
                        m->lists[k].cap_all(&limit);
                        k++;
                    } while (k < m->nlists);
                }
                attach(this, A[wield[j]], region, M);
            }
            if (B[wield[j]] != 0) {
                B[wield[j]]->vis = 0xffff;
                attach(this, B[wield[j]], region, M);
            }
            if ((ik->flags & itcf_carry_mask) != 0 && (ik->flags & itcf_show_holster_when_drawn) == 0)
                slot_used[wield[j]] = 1;
            Mesh* h = C[wield[j]];
            if (h != 0) {
                if ((ik->flags & itcf_show_holster_when_drawn) == 0) {
                    h->vis = 0;
                } else {
                    float limit = 10.0f;
                    int k = 0;
                    if (0 < h->nlists) {
                        do {
                            h->lists[k].cap_all(&limit);
                            k++;
                        } while (k < h->nlists);
                    }
                }
            }
        }
    }
    for (i = 3; i >= 0; i--) {
        if (pairs[i].item >= 0) {
            ItemKind* ik = &g_items[pairs[i].item];
            if ((ik->flags & itcf_carry_mask) != 0) {
                pos2 M;
                int region, bone;
                ik->get_carry_pos(&M, &region, &bone);
                if (!bone_used[bone] && !slot_used[i] && (itype(ik) != itp_type_shield || (pairs[i].m4 & 0xffff) > 10)) {
                    bone_used[bone] = 1;
                    if (C[i] == 0) {
                        if (A[i] != 0) {
                            A[i]->vis = 0xffff;
                            attach(this, A[i], region, M);
                        }
                        if (B[i] != 0) {
                            B[i]->vis = 0xffff;
                            attach(this, B[i], region, M);
                        }
                    } else {
                        C[i]->vis = 0xffff;
                    }
                }
            }
        }
    }
    if (id == g_mission->player_id && g_game->f139e4 != 0) {
        for (j = 0; j < 4; j++) {
            if (pairs[j].item >= 0) {
                ItemKind* ik = &g_items[pairs[j].item];
                if ((ik->flags & itcf_carry_mask) != 0) {
                    pos2 M;
                    int bone, region;
                    ik->get_carry_pos(&M, &bone, &region);
                    if (bone == 9 || bone == 7 || bone == 8) {
                        if (wield[0] != j && (wield[1] != j || f23c != 0)) {
                            if (A[j] != 0)
                                A[j]->vis &= 0x2000;
                            if (B[j] != 0)
                                B[j]->vis &= 0x2000;
                        }
                        if (C[j] != 0)
                            C[j]->vis &= 0x2000;
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
                ((Curve*)a->body->skel)->set_rec_phase(1, 0.0f);   // 0x433b20
            int k = a->wield[0];
            if (k >= 0 && a->pairs[k].item >= 0) {
                if (prio & 0x10000000)
                    a->A[k]->vis = 0;
                else
                    a->A[k]->vis = 0xffff;
            }
        }
    }
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
            if (c < 0 || (item = pairs[c].item) < 0 || (j != 0 && f23c != 0)) {
                if (j != 0) {
                    if (p >= 0 && c < 0 && f23c == 0) {
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
    if (f23c != 0)
        wield_prev[1] = -1;
    if (anim[1].cur == 0x7b)
        set_order_1<true>(this, -1, 0x46, 1.0f);
    if (anim_ != -1)
        set_order_1<true>(this, anim_, 0x5000046, 1.0f);
    else if (anim2 != -1)
        set_order_1<false>(this, anim2, 0x5000046, 1.0f);
    if (snd1 != -1 && (float)(g_ticks[g_mission->time_idx] - g_mission->time_base) * 1e-05f > 1.0f)
        g_sound_mgr.play(g_sound_mgr.slot[snd1], 0, &pos.x, 1.0f, 0);
    if (snd2 != -1 && (float)(g_ticks[g_mission->time_idx] - g_mission->time_base) * 1e-05f > 1.0f)
        g_sound_mgr.play(g_sound_mgr.slot[snd2], 0, &pos.x, 1.0f, 0);
    f130 = 0;
    for (int j = 0; j < 2; j++) {
        int s = wield[j];
        if (s >= 0 && pairs[s].item >= 0 && (j == 0 || f23c == 0)) {
            ItemKind* ik = &g_items[pairs[s].item];
            unsigned t = ik->type_flags & 0xff;
            if (t == itp_type_bow) {
                f130 = 0x10;
                variant = 0;
            } else if (t == itp_type_crossbow || t == itp_type_thrown) {
                f130 = 0x20;
                variant = 4;
            } else if (t == itp_type_two_handed_wpn && (ik->type_flags & itp_two_handed)) {
                f130 = 4;
            } else if (t == itp_type_shield) {
                f130 |= 0x1000;
            } else if (t == itp_type_polearm) {
                f130 |= 8;
            } else if (t == itp_type_one_handed_wpn || t == itp_type_two_handed_wpn) {
                f130 |= 2;
            }
        }
    }
    active = 0;
    f5610 = 0;
}

// ---- std::_Uninit_fill_n for the 0x38-byte E56 record ---------------------------------------------------------------------------------------------
// @ 0x444bc0 (+ catch-all funclet 0x444c28: destroys [next, first), then `throw;`)
void __cdecl uninit_fill_n_e56(E56* first, unsigned n, const E56& val, alloc_e56& al, int tag)
{
    E56* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy((VecPair2*)next);
        throw;
    }
}

//=============================================================================
// 0x444c50..0x460160
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
extern int g_propkind_count;                                // 0x5e29e0

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----

unsigned g_input_mask;                                      // 0x5e2474


// ---- canonical classes / forwarders (see agent_logic.h) ----------------------------------------------------------------------------
struct vec2x : vec2 { void rotate_b(float ang); };           // 0x4327f0 (rgl/xmlcfg/game_util.h)
__forceinline void SceneEnt::add_extra(rgl::mesh* m) { ((Poser*)this)->add_extra((int)m); }     // 0x442110
namespace std { template<> void vector<rgl::scene*, allocator<rgl::scene*> >::push_back(rgl::scene* const&); }   // 0x4119d0 (mesh_obj.cpp's instance, shared by every pointer vector)
__forceinline MeshVec& MeshVec::operator=(const std::vector<rgl::mesh*>& o) { ((std::vector<int>*)this)->operator=(*(const std::vector<int>*)&o); return *this; }   // 0x43f0a0
__forceinline void MeshVec::push_back(rgl::mesh* const& x) { ((std::vector<rgl::scene*>*)this)->push_back(*(rgl::scene* const*)&x); }                         // 0x4119d0

// inline vector helpers of the original object
static inline vector4 a_vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static inline vector4 a_vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vector4 a_vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

#define itp_two_handed 0x200000

// The std::vector<E56> range helpers at 0x442080 / 0x444bc0 are agent3/agent_fx_sound.cpp's definitions (uninit_copy_e56,
// uninit_fill_n_e56). The specialisations below forward to them by name, called through the five-parameter type so the
// caller still pushes the tag slot as the original does (the linker cannot alias a `??$` template name to them).
E56* __cdecl uninit_copy_e56(const E56* first, const E56* last, E56* dest, alloc_e56& al);          // 0x442080
void __cdecl uninit_fill_n_e56(E56* first, unsigned n, const E56& val, alloc_e56& al, int tag);     // 0x444bc0
namespace std {
// out-of-object helpers of the std::vector<E56> instantiations
template<> inline E56* _Uninit_copy<E56*, E56*, allocator<E56> >(E56* f, E56* l, E56* d, allocator<E56>& al, _Nonscalar_ptr_iterator_tag t)   // 0x442080
{
    return ((E56* (__cdecl*)(E56*, E56*, E56*, allocator<E56>&, _Nonscalar_ptr_iterator_tag))uninit_copy_e56)(f, l, d, al, t);
}
template<> inline void _Uninit_fill_n<E56*, unsigned int, E56, allocator<E56> >(E56* f, unsigned int n, const E56& v, allocator<E56>& al, _Nonscalar_ptr_iterator_tag t)   // 0x444bc0
{
    ((void (__cdecl*)(E56*, unsigned int, const E56&, allocator<E56>&, _Nonscalar_ptr_iterator_tag))uninit_fill_n_e56)(f, n, v, al, t);
}
}

// ---- PolyMesh::find_path: A*-style search over the navigation faces ---------------------------------------------------------

// More bodies the original's object saw (rgl header inlines left out of line; shapes.cpp and game_util.cpp own the rows).
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

// ---- std::vector<E56> helpers (0x445700 _Ucopy; 0x445e70 _Ufill and 0x445ea0 _Insert_n further down) ----------------------------
// @ 0x445700
template std::vector<E56>::pointer std::vector<E56>::_Ucopy<E56*>(E56*, E56*, E56*);

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
        if (goal_face < 0)
            return;
        if (g_mission->nav.reachable(face, &goal_face, max_steps))
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
        if (f2c >= 0)
            cls = 2;
        else if ((g_game->troops[troop].flags30 & 0x4000000) != 0)
            cls = 1;
        else
            cls = 0;
    }
    ClassInfo* r = &g_mission->teams[t].cls[cls];
    int max_steps = 7;
    pick_reachable_goal(center, radius, &r->nav, &max_steps);
    goal_dir = r->target;
}

// @ 0x445910
bool Agent::plan_path()
{
    path_count = 0;
    float width, w1, w2;      // w1 / w2 are never written and find_path (same unit) never reads them: they take width's frame slot
    if (f10 == 0 && f2c == -1)
        width = 0.45f;
    else
        width = 0.6f;
    int face = update_nav_face();
    if (face >= 0 && goal_face >= 0) {
        g_mission->nav.find_path(face, goal_face, pface(), ppos(), &path_count, &path_steps,
                                 &width, &w1, &w2);
        path_cur = 0;
        path_flag = path_count <= 0;
        goal_face_done = goal_face;
    }
    return path_count > 0;
}

// @ 0x4459c0
void Agent::play_class_sound(const int* kind)
{
    unsigned cls = (unsigned)g_game->troops[troop].flags30 & 0xf;
    int snd = g_anim_table.sound(cls, *kind);
    if (snd >= 0)
        play_sound(&snd);
}

// @ 0x445a20
void Agent::on_wield_reset()
{
    f23c = 0;
    if (slots[0] >= 0) {
        int item = pairs[slots[0]].item;
        if (item >= 0 && (g_items[item].type_flags & itp_two_handed) != 0)
            f23c = 1;
    }
    update_equipment_display();
    on_wield_changed();
    fr2.o.x = -20000000.0f;
    fr1.o.x = -20000000.0f;
}

// ---- std::vector<E56> helpers 0x445e70 / 0x445ea0 (the whole _Insert_n COMDAT is 0x33e bytes: 0x445ea0..0x4461dd, including
// the two catch(...) funclets 0x44608f / 0x44614e) ------------------------------------------------------------------------------
// @ 0x445e70
template std::vector<E56>::pointer std::vector<E56>::_Ufill(E56*, size_t, const E56&);
// @ 0x445ea0
template void std::vector<E56>::_Insert_n(std::vector<E56>::iterator, size_t, const E56&);

// ---- Agent: top up the ammunition of the held ranged weapon ----------------------------------------------------
// 0x434b70 / 0x434be0 / 0x434c30 (ItemRef::max_amount / add_amount / room_left) are header-inline in the original: the bodies are
// visible here, so cl inlines some uses and keeps a call for the others (the slice's local copies did the same)
static inline int round_float_inl(const float* p)       // 0x432540 (inlined here; agent/agent.h declares the out-of-line round_float)
{
    float fl = (float)floor((double)*p);
    int i = (int)fl;
    return (*p - fl <= 0.5f) ? i : i + 1;
}
static int ir_max_amount(ItemRef* r)                      // 0x434b70
{
    if (r->item < 0) return 0;
    int v = g_items[r->item].f210;
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
    ((unsigned char*)&r->m4)[0] = 0;
    r->m4 = r->m4 | v;
}
static int ir_room_left(ItemRef* r)                      // 0x434c30
{
    return ir_max_amount(r) - (int)(r->m4 & 0xff);
}
// The original agent translation unit (0x4353a0..0x45fxxx) saw these bodies: a local whose address reaches only them does not
// escape, so it shares stack slots (codegen 7). agent/agent.cpp and agent2/agent_stats.cpp own the rows.
inline __declspec(noinline) int ItemRef::max_amount() const     // 0x434b70
{
    if (item < 0) return 0;
    int v = g_items[item].f210;
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
inline __declspec(noinline) void node_list::cap_all(const float* limit)     // 0x438f90
{
    for (rgl::mesh** it = first; it < last; ++it) {
        rgl::mesh* n = *it;
        struct L { int a0; int* data; int count; };
        L* l = n->local_a1 ? (L*)&n->kf : (L*)&n->parent->get_kf();
        if (0 < l->count) {
            if (*limit > (float)l->data[l->count - 1])
                n->c8 = (float)l->data[l->count - 1];
            else
                n->c8 = *limit;
        }
    }
}
inline __declspec(noinline) void node_groups::cap_all(const float* limit)   // 0x439000
{
    for (int i = 0; i < nlists; ++i)
        lists[i].cap_all(limit);
}
inline __declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)     // 0x40e3d0
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
inline __declspec(noinline) int rgl::mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)v50.size();
    v50.resize(n + 1);
    (&*v50.begin())[n].index = index;
    (&*v50.begin())[n].uv = *uv;
    (&*v50.begin())[n].color = *color;
    (&*v50.begin())[n].f5c = *f5c;
    vertex60* vv = v50._Myfirst + n;
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}
inline __declspec(noinline) int rgl::mesh::add_point(const vector4* p)
{
    ensure_local();
    int n = (int)v70.size();
    v70.resize(n + 1);
    v70[n].a = *p;
    v70[n].b = *p;
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
            f24c = equipment_value();
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
                            links[slots[0]]->vis = 0;
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
            if (f2c >= 0)
                k1 = 2;
            else
                k1 = (g_game->troops[tr].flags30 & 0x4000000) ? 1 : 0;
        }
        int ra = g_mission->teams[t1].cls[k1].a;
        int t2 = team;
        if (t2 < 0 || t2 >= 8) t2 = 7;
        int k2 = 0;
        if (tr >= 0) {
            if (f2c >= 0)
                k2 = 2;
            else
                k2 = (g_game->troops[tr].flags30 & 0x4000000) ? 1 : 0;
        }
        int rb = g_mission->teams[t2].cls[k2].b;
        for (int i = 3; i >= 0; i--) {
            ItemRef* p = &pairs[i];
            if (p->item >= 0) {
                ItemKind* ik = &g_items[p->item];
                bool ok = ((ik->f218 & 0x300) == 0x200 || p->stat_21x(0) <= 1) &&
                          ((ik->f214 & 0x300) == 0x200 || p->stat_21x(1) <= 1);
                if ((ra == 0 || ok) &&
                    (rb == 0 || ((ik->type_flags & 0xff) != 8 && (ik->type_flags & 0xff) != 9 &&
                                 (ik->type_flags & 0xff) != 10 && (ik->type_flags & 0xff) != 0x10 &&
                                 (ik->type_flags & 0xff) != 0x11)) &&
                    (f2c < 0 || !(ik->type_flags & 0x10000000))) {
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
                                if (!(ik->type_flags & 0x100000) || f2c == -1)
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
        combat_state = 0;
        f53bc |= 0x200;
        on_wield_reset();
    }
}

// @ 0x4464d0
void Agent::start_attack(int kind)
{
    f2c8 = kind;
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
    attack_delay() = ((float)(rand() % 0x3dc9) * 3.161156e-05f + 0.75f) * scale;
}

// ---- Agent::update_ai: the per-tick order executor ------------------------------------------------------------------
// Bodies the original object could see (header-inline there, kept out of line): cl uses them for its side-effect analysis
// (a call that cannot write memory does not end a CSE'd pages-pointer load).
// @ 0x432350
__declspec(noinline) bool Agent::is_enemy(const Agent* o) const
{
    if (team >= 0 && team < 8 && o->team >= 0 && o->team < 8)
        return g_mission->teams[team].relation[o->team] == -1;
    return false;
}
#define AG(i) (*agent_by_id(i))
#define tf_guarantee_ranged 0x4000000i64
float __cdecl hit_chance(int* who, Agent* a);                          // 0x438cb0 (agent2/agent_stats.cpp)
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
        if (f2c >= 0)
            kind = 2;
        else
            kind = (g_game->troops[troop].flags30 & tf_guarantee_ranged) ? 1 : 0;
    }
    return &g_mission->teams[cls].cls[kind];
}


static inline float a_dot3r(const vector4& b, const vector4& a) { return a.z * b.z + a.y * b.y + a.x * b.x; }
static inline void a_sub3(vector4& r, const vector4& a, const vector4& b) { r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z; }
// @ 0x4465e0
void Agent::update_ai()
{
    ClassInfo* cr = class_rec();

    int prev = -1;
    int near_ = last_attacker;
    if (near_ >= 0 && g_mission->agents[near_].f14 == 1) {
        Agent* o = &g_mission->agents[last_attacker];   // re-read, not near_: the original reloads it here
        vector4 d;
        a_sub3(d, o->pos, pos);
        float len = d.normalize();
        bool far_ = true;
        if (target >= 0) {
            const vector4& cp = g_mission->agents[target].pos;
            vector4 e = cp - pos;
            far_ = !(sqrt(e.y * e.y + (e.z * e.z + e.x * e.x)) <= len + 1.0f);
        }
        if (far_ && is_enemy(o))
            target = near_;
        prev = near_;
    }

    bool flag_a = false;        // local_81
    bool flag_b = false;        // local_5d
    int mode_ = f2c8;

    if (mode_ == 1) {
        if (!((*(unsigned char*)&f2c4) & 1)) {
            f533c = 0;
            path_state |= 0x20;
        } else {
            unsigned t = target;
            if (t == 0xffffffff || AG(t).f14 != 1 || frand() < 0.1f)
                flag_a = true;
            t = target;
            if ((int)t >= 0) {
                bool ranged = false;
                const vector4& op = AG(target).pos;
                vector4 dd = a_vsub(pos, op);
                float dist = (float)sqrt(dd.y * dd.y + (dd.z * dd.z + dd.x * dd.x));
                ItemRef p = AG(target).cur_pair_fi();
                if (p.item >= 0 && g_items[p.item].is_ranged())
                    ranged = true;
                if (f2c >= 0 || v56b4.length_squared() < 0.7f || dist < 15.0f ||
                    (dist < 100.0f && weapon_mode == 2) || ranged) {
                    f533c = 2;
                    ItemRef c = cur_pair_fi();
                    if (c.item >= 0 && g_items[c.item].is_ranged())
                        f533c = 0x22;
                }
            }
            flag_b = true;
            path_state |= 0x20;
        }
    } else if (mode_ == 4) {
        if (prev == -1 || target == -1 || AG(target).f14 != 1)
            target = pick_shout_target(0x26, 300.0f);
        if (target == -1) {
            reset_if_enemy_charged();
            f533c = 0;
        } else if (AG(target).f14 != 1) {
            attack_delay() = 0.0f;
        } else {
            vector4 q;
            const vector4& op = AG(target).pos;
            vector4 w = op - pos;
            float dist = w.normalize();
            ItemRef sp = AG(target).slot_pair(0);
            int ranged = 0;
            if (sp.item >= 0 && g_items[sp.item].is_ranged())
                ranged = 1;
            f533c = 2;
            if (!ranged && dist > 15.0f && f2c < 0)
                f533c = 0;
            if (f314 < 0.0f)
                f314 = 0.0f;
            else if (f314 > 30.0f)
                f314 = 30.0f;
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
                    if (a->f10 == 0 && a->id != target && a->f14 == 1 && a->id != id) {
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
                if (q.x * f294() + (q.y * f298() + q.z * f29c()) > -0.3f) {
                    target = best;
                    m = g_mission;
                }
            }
            bool close_ = false;
            if (cr->kind == 1) {
                hand_pos(&q);
                vector4 d = pos - q;
                float len = d.normalize();
                if (len > 5.0f) {
                    f2c8 = 1;
                    float r = 0.2f;
                    goal_near_for_class(&q, &r);
                    close_ = true;
                }
                m = g_mission;
            }
            if (cr->kind == 2) {
                int t2 = team;
                if (t2 < 0 || t2 >= 8) t2 = 7;
                if (m->teams[t2].leader != id) {
                    hand_pos(&q);
                    vector4 d = pos - q;
                    float len = d.normalize();
                    if (len > 8.0f) {
                        f2c8 = 9;
                        goto L_tail4;
                    }
                }
            }
            if (!close_) {
                if (f2e4 == 0) {
                    float diff = wsum[1] - wsum[0];
                    if (diff < 0.05f)
                        diff = 0.05f;
                    else if (diff >= 0.3f)
                        diff = 0.3f;
                    int r = rand() % 0x3dc9;
                    int hf = always_attack_in_melee;
                    float push = (((float)r * (1.0f / 15817.0f)) * diff * g_dt_scale) * 20.0f;
                    if (hf == 0)
                        push = ((float)g_options.difficulty * 0.5f + (40.0f - (float)g_game->troops[troop].fb0) * 0.012f) + push;
                    push = push + f314;
                    f314 = push;
                    if (push < 0.0f)
                        f314 = 0.0f;
                    else if (push > 11.0f)
                        f314 = 11.0f;
                    if (scripted_mode == 1)
                        f314 = 0.0f;
                    {
                        ItemRef cp = AG(target).cur_pair_fi();
                        if ((cp.item < 0 || !g_items[cp.item].is_ranged()) && f314 >= 10.0f) {
                            f314 = 10.0f;
                            if (hf == 0)
                                f2e4 = 1;
                        }
                    }
                    if (f2e8 != 0 || frand() < 0.3f - t98 * 0.05f) {
                        f2e8 = f2e4;
                        if (scripted_mode == 1) {
                            float r2 = 0.2f;
                            set_goal_near(&scripted_destination.o, &r2);
                        } else {
                            int fid = -1;
                            if (g_mission->scene->hf)
                                g_mission->scene->hf->sample(pos.x, pos.y, wsum, &fid);
                            if (dist < 4.0f ||
                                (g_mission->scene->hf && (fbc == 0 || ((int*)fbc)[1] != fid))) {
                                set_goal_agent(target);
                            } else {
                                float r30 = 30.0f;
                                float m = fmin_ref(&dist, &r30);
                                float fr = frand();
                                AgentTable* tb = &g_mission->agents;
                                int u = target;
                                t98 = ((fr + 2.0f) * m) * 0.2f;
                                vector4 d = t98 * tb->get(u)->v56b4;
                                q = tb->get(u)->v56b4;
                                q.normalize();
                                float f = q.x * w.x + q.z * w.z + q.y * w.y + 1.0f;
                                w.x = d.x * f;
                                w.y = d.y * f;
                                w.z = d.z * f;
                                const vector4* pp = &tb->get(u)->pos;
                                q.x = w.x + pp->x;
                                q.y = w.y + pp->y;
                                q.z = w.z + pp->z;
                                t98 = dist * 0.3f - 0.6f;
                                float hi = 20.0f;
                                float lo = 0.0f;
                                d = q;
                                float c = clamp(&t98, &lo, &hi);
                                if (always_attack_in_melee != 0 && c > 0.5f)
                                    c = 0.5f;
                                int ms = (int)(c * 0.2f + 4.0f);
                                int face = tb->get(u)->update_nav_face();
                                pick_reachable_goal(&d, &c, &face, &ms);
                            }
                        }
                    }
                } else if (f2e4 == 1) {
                    float val = wsum[0] * 1.3f - wsum[1];
                    float hi = 0.3f;
                    float lo = 0.01f;
                    float c = clamp(&val, &lo, &hi);
                    float fr = frand();
                    float nv = f314 - ((fr * c) * g_dt_scale) * 20.0f;
                    f314 = nv;
                    if (nv < 0.0f)
                        f314 = 0.0f;
                    else if (nv > 11.0f)
                        f314 = 11.0f;
                    bool rg = false;
                    {
                        ItemRef sp2 = AG(target).slot_pair(0);
                        if (sp2.item >= 0 && g_items[sp2.item].is_ranged())
                            rg = true;
                    }
                    if (f314 < 0.0f || rg) {
                        f314 = 0.0f;
                        f2e4 = 0;
                    }
                    if (f2e8 != 1) {
                        f2e8 = f2e4;
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
            path_state |= 0x20;
            float s = 0.5f;
            ItemRef cp = cur_pair_fi();
            if (cp.item != -1)
                s = (float)g_items[cp.item].weight * 0.01f + 0.9f;
            if (path_state & 2) {
                f288 = s;
                f28c = s - 0.4f;
            } else {
                f288 = 0.5f;
                f28c = 0.2f;
            }
        }
    } else if (mode_ == 5 && combat_state != 1 && combat_state != 3 && combat_state != 4) {
        float chance = hit_chance((int*)&(const ItemRef&)slot_pair(0), this);
        vector4 q, w;
        float t80 = 500.0f;
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            vector4 d = g_mission->agents.get(e)->pos - pos;
            near_ = d.normalize();
        }
        if (target >= 0 && g_mission->agents.get(target)->f14 != 1) {
            target = -1;
        }
        if (target == -1) {
            int c = f2cc;
            if (c >= 0 && g_mission->agents.get(c)->f14 == 1) {
                target = c;
            }
        }
        int tg = target;
        float cur;
        if (tg >= 0) {
            vector4 d = g_mission->agents.get(tg)->pos - pos;
            cur = d.normalize();
        } else {
            cur = t80;
        }
        if ((near_ < 8.0f && cur > near_ + near_) || tg == -1)
            target = e;
        float wsum[2];
        if (frand() < 0.2f || target == -1 || g_mission->agents.get(target)->f14 != 1) {
            collect_enemies((int)wsum);
            if (nthreat > 0) {
                int c = (threats[0].dist < 3.5f && g_mission->agents.get(threats[0].id)->f14 == 1) ? threats[0].id : pick_threat();
                if (c >= 0) {
                    f2cc = c;
                    target = c;
                }
            }
        }
        int t3 = target;
        if (t3 >= 0 && g_mission->agents.get(t3)->f14 == 1) {
            Agent* ta = g_mission->agents.get(t3);
            f533c = 0x22;
            vector4 d = ta->pos - pos;
            t80 = d.normalize();
            int bestid = -1;
            bool close99 = false;
            if (frand() + 1.5f > t80)
                attack_delay() = 0.0f;
            float one = 1.0f;
            t80 = (ta->pos.z - pos.z) * 0.7f + t80;
            float dd = fmax_ref(&t80, &one);
            if (cr->kind == 1 || (cr->kind == 2 && team_leader(this) != id)) {
                close99 = true;
                hand_pos(&w);
                a_sub3(q, pos, w);
                wsum[0] = q.normalize();
            }
            t80 = chance * 0.7f;
            one = 30.0f;
            if ((fmin_ref(&one, &t80) < dd && !close99 && cr->kind != 3) ||
                (always_attack_in_melee != 0 && !close99 && cr->kind != 3)) {
                q = ta->pos;
                bestid = ta->id;
            } else {
                if (line_of_sight(ta->id, 0) && fire_check(ta->id, (int*)&t80) == 1 && (!close99 || wsum[0] < 3.0f)) {
                    reset();
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
        path_state |= 0x20;
        f288 = 1.5f;
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
        if (target >= 0) {
            vector4 d = AG(target).pos - pos;
            cur = d.normalize();
        } else {
            cur = t80;
        }
        if (near_ < 8.0f && cur > near_ + near_)
            target = e;
        int r = rand() % 0x3dc9;
        int c;
        if (((float)r * (1.0f / 15817.0f) < 0.2f || target == -1 ||
             g_mission->agents.get(target)->f14 != 1) &&
            (collect_enemies((int)&t80), nthreat > 0 && (c = (int)pick_threat()) >= 0)) {
            f2cc = c;
            target = c;
        }
        if (target == -1)
            target = f2cc;
        if (f2c == -1) {
            attack_delay() = 0.0f;
            return;
        }
        if (AG(target).f14 != 1) {
            target = -1;
            attack_delay() = 0.0f;
        } else {
            f533c = 0x22;
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
                        Agent* ta = g_mission->agents.get(target);
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
        path_state |= 0x20;
        f288 = 1.5f;
    } else if (mode_ == 8) {
        flag_a = true;
        if (f2c == -1) {
            attack_delay() = 0.0f;
            return;
        }
        if (cr->kind != 3 &&
            (!(path_state & 1) || g_mission->agents.get(f2c)->fac < 1.0f || (goal_face != goal_face_done && goal_face >= 0))) {
            if (g_mission->agents.get(f2c)->fac > 2.0f) {
                float rr = 15.0f;
                vector4 gp = vel * 25.0f + pos;
                set_goal_near(&gp, &rr);
            } else {
                float rr = 30.0f;
                set_goal_near(&pos, &rr);
            }
        }
        if (path_flag != 0) {
            float r50 = 50.0f;
            set_goal_near(&pos, &r50);
        }
    } else if (mode_ == 7) {
        if (f2c == -1) {
            attack_delay() = 0.0f;
            return;
        }
        target = pick_shout_target(0x1026, 150.0f);
        if (target == -1)
            target = f2cc;
        unsigned u = target;
        Agent* ta = g_mission->agents.get(u);
        vec2 v;
        v.x = ta->pos.x - pos.x;
        v.y = ta->pos.y - pos.y;
        float dist = v.normalize();
        float dotv = v.x * vel.x + v.y * vel.y;
        if (((dist < 10.0f && dotv < 0.0f) &&
             (g_mission->agents.get(u)->f2c == -1 || frand() < 0.2f)) ||
            (dist < 2.0f && fac < 1.0f)) {
            f2c8 = 8;
            attack_delay() = (((float)(rand() % 0x3dc9)) * 3.161156e-05f + 0.75f) * 2.0f;
        } else if (weapon_mode == 3 && dotv < 0.3f && dist < 10.0f) {
            f2c8 = 8;
            attack_delay() = (((float)(rand() % 0x3dc9)) * 3.161156e-05f + 0.75f) * 2.0f;
        }
        f318 = target;
        set_goal_agent(target);
        if (cr->kind != 3) {
            if ((fac > 2.0f && g_mission->agents.get(target)->f2c == -1 && dist < 11.0f) ||
                (f100_is_0x48_and_charged() && dotv > 0.2f)) {
                set_goal_agent(target);
                f2e4 = 0;
            } else if (f2e4 != 1 || frand() < 0.5f) {
                float s = dist + 10.0f;
                f2e4 = 1;
                float r30 = 30.0f;
                float m = fmin_ref(&s, &r30);
                float fr = frand();
                unsigned u2 = target;
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
        path_state |= 0x20;
        if (path_state & 2)
            path_state |= 0x80;
        if (dist < 35.0f)
            path_state |= 0x200;
        f533c = 2;
    } else if (mode_ == 9) {
        flag_b = true;
        vector4 q, d;
        hand_pos(&q);
        f2d0 = q;
        ItemRef sp = slot_pair(0);
        a_sub3(d, q, pos);
        float dist = d.normalize();
        float m = 1000.0f;
        bool b99 = false;
        if (sp.item < 0 || !g_items[sp.item].is_ranged()) {
            if (target == -1 || g_mission->agents.get(target)->f14 != 1 || frand() < 0.2f)
                target = pick_threat();
            b99 = true;
        } else if (target == -1 || g_mission->agents.get(target)->f14 != 1 || frand() < 0.2f) {
            target = pick_random();
        }
        int tg = target;
        if (tg >= 0) {
            vector4 d = g_mission->agents.get(tg)->pos - pos;
            m = d.normalize();
        }
        if (!b99 && tg >= 0 && dist < 7.0f && m < 4.0f) {
            set_goal_agent(tg);
        } else {
            float r2 = 0.2f;
            goal_near_for_class(&f2d0, &r2);
            path_state |= 0x20;
            if (f2c >= 0)
                f288 = 3.5f;
            else
                f288 = 2.5f;
        }
    } else if (mode_ == 0xb) {
        int p = g_mission->player_id;
        if (p >= 0) {
            Agent* pa = g_mission->agents.get(p);
            if (rgl::length(pos - pa->pos) < 1.2f) {
                target = p;
                f533c = 2;
                goto done;
            }
        }
        f533c = 0;
        target = -1;
    } else if (mode_ == 0xd) {
        f533c = 0;
        target = -1;
        f288 = 3.5f;
        if (frand() < 0.0005f || prev >= 0) {
            float r15 = 15.0f;
            vector4 v = vel * 10.0f + pos;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 0xe) {
        f533c = 0;
        target = -1;
        f288 = 3.0f;
        if (frand() < 0.3f) {
            float r15 = 15.0f;
            vector4 v = vel * 20.0f + pos;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 3) {
        reset();
        if (fac < 0.5f && f2c != -1)
            g_input_flags |= 0x10000;
    } else if (mode_ == 2) {
        if (f2c != -1) {
            if (attack_delay() > 1.0f)
                attack_delay() = 1.0f;
        } else {
            int c = f2cc;
            if (c != -1 && g_mission->agents.get(c)->f14 == 1) {
                Agent* ca = g_mission->agents.get(c);
                vector4 d = ca->pos - pos;
                float l = d.out_length();
                if (l > 0.99f) {
                    flag_a = true;
                    AgentBody* b = ca->body;
                    vector4 q;
                    if (l > 2.0f) {
                        vector4 s(b->m.s.x, b->m.s.y, b->m.s.z);
                        q = a_vsub(ca->pos, s);
                        path_state = 0x21;
                    } else {
                        vector4 s = b->m.s * 0.2f;
                        q = a_vsub(ca->pos, s);
                        path_state = 1;
                    }
                    float r0 = 0.0f;
                    set_goal_near(&q, &r0);
                } else {
                    g_input_flags |= 0x20000;
                    flag_a = false;
                    target = c;
                    f533c = 2;
                }
                f288 = 0.15f;
            }
        }
    }

done:
    if (g_mission->terrain.f184 != 0 && id == g_dialogs.partner_agent) {
        f2c8 = 0;
        flag_a = false;
        target = g_mission->player_id;
        f533c = 2;
    }
    if (f2c8 == 0) {
        path_state = 0;
        path_flag = 0;
        path_count = 0;
        path_active = 0;
        goal_changed = 0;
        f533c = 0;
    } else if (f2c8 == 0xc) {
        path_state = 0;
        path_flag = 0;
        path_count = 0;
        path_active = 0;
        goal_changed = 0;
        f533c = 0;
        target = -1;
        if ((float)(rand() % 0x3dc9) * (1.0f / 15817.0f) < 0.3f) {
            target = pick_shout_target(0x26, 300.0f);
            if (target >= 0)
                attack_delay() = 0.0f;
        }
        if ((float)(rand() % 0x3dc9) * (1.0f / 15817.0f) < 0.6f) {
            g_input_flags = g_input_flags & 0xffffff;
            g_input_flags = g_input_flags | 0x1000000;
        }
    }

    ItemRef cp = cur_pair_fi();
    if (flag_a && f10 != 1) {
        float range = 30.0f;
        if (cp.item >= 0 && g_items[cp.item].is_ranged())
            range = hit_chance_ref(cp, this);
        int n = nthreat;
        float best = 0.0f;
        int bestid = -1;
        if (n > 0)
            bestid = threats[0].id;
        if (cp.item >= 0 && g_items[cp.item].is_ranged()) {
            bestid = pick_threat();
        } else if (n > 0) {
            for (int i = 0; i < nthreat; i++) {
                Threat* t = &threats[i];
                float score = -0.01f;
                Agent* ta = &AG((unsigned)t->id);
                if (ta->f14 == 1) {
                    vector4 d = ta->pos - pos;
                    t->dir = d;
                    float len = t->dir.normalize();
                    t->dist = len;
                    if (len < range) {
                        score = 10.0f / (len + 1.0f) + a_dot3r(t->dir, body->m.f) * 0.1f;
                    } else {
                        if (f2c == -1) {
                            ItemRef p1 = slot_pair_fi(1);
                            if (p1.item >= 0) {
                                if ((char)g_items[slot_pair(1).item].type_flags == itp_type_shield) {     // cmp cl, 7 (0x448bf6)
                                    ItemRef p0 = ta->slot_pair(0);
                                    if (p0.item >= 0) {
                                        ItemKind* ik = &g_items[p0.item];
                                        if (ik->is_ranged() && (float)ik->f1f4 * 2.1f > t->dist) {
                                            float inv = 1.0f / (t->dist + 10.0f);
                                            float one = 1.0f;
                                            score = fmin_ref(&one, &inv) * 2.5f;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (t->id == target)
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
            target = bestid;
        else
            target = -1;
        f533c = (target == -1) ? 0 : 2;
    }
    if (flag_b && f10 == 0 && target >= 0) {
        const vector4& tv = AG(target).v56b4;
        vector4 ta_ = a_vadd(vector4(tv.x, tv.y, tv.z), AG(target).pos);
        vector4 tb_ = a_vadd(vector4(v56b4.x, v56b4.y, v56b4.z), pos);
        vector4 d = a_vsub(ta_, tb_);
        float dist = d.normalize();
        float range = 3.0f;
        if (f2c >= 0 && g_game->troops[troop].nibble(0x17) < 2)
            range = 15.0f;
        if (dist > range && has_weapon_for(2)) {
            set_weapon_mode(2);
        } else if (f2c < 0 || !(v56b4.length() > 0.1f) ||
                   !has_weapon_for(3)) {
            if (has_weapon_for(1))
                set_weapon_mode(1);
        } else {
            set_weapon_mode(3);
        }
    }
    f310 = 0;
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

// @ 0x448eb0
void Agent::ai_update_control()
{
    float strafe;
    int mid;
    float angle;
    unsigned fid;

    {
    vector4 tgt = a_pos(this);                                   // movement target
    if (fbc == 0)
        return;

    // ---- 1. keep the path / goal up to date ----------------------------------------------------------------------
    if (path_active != 0) {
        if (path_state & 2) {
            const vector4* tp = &agent_by_id(f318)->pos;
            vector4 dd(tp->x - goal.x, tp->y - goal.y, tp->z - goal.z);
            if ((float)sqrt(dd.y * dd.y + (dd.z * dd.z + dd.x * dd.x)) > 4.0f &&
                goal_face != agent_by_id(f318)->update_nav_face()) {
                fid = f318;
                goal = ((AgentTable*)((char*)g_mission + 4))->get(fid)->pos;
                goal_face = ((AgentTable*)((char*)g_mission + 4))->get(fid)->update_nav_face();
                goal_changed = 1;
                path_active = 0;
            }
        }
        if (path_active != 0)
            advance_path();
    }

    // ---- 2. pick the movement target -------------------------------------------------------------------------------
    if (path_active != 0)
        tgt = ppos()[path_cur];
    if (path_active == 0) {
        unsigned fl = path_state;
        tgt = a_pos(this);
        if (fl != 0) {
            if ((fl & 2) != 0) {
                vector4 lv;
                const vector4& v = *agent_by_id(f318)->local_velocity(&lv);
                tgt = v * 0.8f + agent_by_id(f318)->pos;
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
                goal = agent_by_id(f318)->pos;
                goal_face = agent_by_id(f318)->update_nav_face();
            } else if ((fl & 1) != 0) {
                tgt = goal;
            }
        }
        bool ok = false;
        if (goal_changed != 0 && goal_face != update_nav_face()) {
            if (plan_path()) {
                path_active = 1;
                ok = true;
            } else {
                path_active = 0;
            }
            goal_changed = 0;
            if (ok) {
                advance_path();
                if (path_active != 0)
                    tgt = ppos()[path_cur];
            }
        }
    }

    // ---- 3. heading jitter and obstacle avoidance -----------------------------------------------------------------------
    {
    {
        float base;
        if (f10 == 0 && f2c < 0) {
            int r = rand() % 0x3dc9;
            base = (float)r * 6.3223115e-06f + 0.88f;
        } else {
            int r = rand() % 0x3dc9;
            base = (float)r * 6.3223115e-06f + 0.85f;
        }
        veer = base * veer;
    }
    vector4 d = tgt - a_pos(this);                               // direction to the target
    float dlen = d.normalize();

    if ((flags3c & 1) && dlen > 0.1f) {
        if (fc0 == 0) {
            capsule cap;
            float a = 0.3f;
            float b = 1.0f;
            if (f10 != 0 || f2c != -1) {
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
            g_mission->scene->query_box(&bmin.x, &bmax.x, 2, shapes, &count);
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
                    veer = (-0.7f - veer) * ((frand() + 1.0f) * 0.5f) + veer;
                else
                    veer = (0.7f - veer) * ((frand() + 1.0f) * 0.5f) + veer;
            }
            if (f2c >= 0) {
                EntVec ents;
                ents.first = 0;
                ents.last = 0;
                ents.end = 0;
                g_mission->scene->collect_boxed(&bmin, &bmax, (std::vector<unsigned>*)&ents);
                SceneEnt** pe = ents.first;
                if (pe < ents.last) {
                    do {
                        SceneEnt* e = *pe;
                        if (e->shape != 0 && e->type == 0xb && e->id != id && e->id != (unsigned)f2c &&
                            e->id != (unsigned)f30) {
                            Agent* o = agent_by_id(e->id);
                            if (o->f10 == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                                AgentBody* mb = body;
                                rgl::matrix m = mb->m;
                                int flag = 1;
                                float ang = 0.01f;
                                ((rgl::matrix&)m).rotate_about_u(&ang);
                                const vector4& of = o->body->m.f;
                                float lhs = (float)fabs(m.f.y * of.y + m.f.z * of.z + m.f.x * of.x);
                                float rhs = (float)fabs(rgl::dot(mb->m.f, of));
                                if (rhs > lhs)
                                    flag = -1;
                                veer = (float)flag * 0.1f + veer;
                            }
                        }
                        ++pe;
                    } while (pe < ents.last);
                }
                if (ents.first != 0)
                    operator delete(ents.first);
            }
        }
        // ---- speed-dependent weave ---------------------------------------------------------------------------------
        if (sqrt(v56b4.x * v56b4.x + (v56b4.z * v56b4.z + v56b4.y * v56b4.y)) < 0.3f) {
            if (f2c >= 0) {
                if (!(speed > 0.9f))
                    goto weave_up;
                if (f5570 < -0.95f) {
                    f2c8 = 3;
                    f5570 = f5570 - 0.03f;
                } else {
                    f5570 = f5570 - 0.03f;
                }
            } else {
                if (f10 != 0 || (f5390 & 1) == 0)
                    goto weave_up;
                f5570 = f5570 - 0.03f;
            }
        } else {
weave_up:
            f5570 = f5570 + 0.2f;
        }
        a_clamp(f5570, -1.0f, 1.0f);
        if (f5570 < -0.1f) {
            if ((float)(g_ticks[clock] - deadline) * 1e-5f >= 0.0f) {
                f5570 = 1.0f;
                path_active = 0;
                goal_changed = 1;
                deadline = g_ticks[clock] + 6000000;
            } else {
                if (veer < 0.0f) {
                    int r = rand() % 0x3dc9;
                    veer = veer - (0.3f - f5570) * ((float)r * 6.3223117e-05f + 0.5f);
                } else {
                    int r = rand() % 0x3dc9;
                    veer = (0.3f - f5570) * ((float)r * 6.3223117e-05f + 0.5f) + veer;
                }
                int r = rand() % 0x3dc9;
                if ((float)r * 6.3223117e-05f < 0.01f)
                    veer = -veer;
            }
        }
    }
    if (veer < -0.03f || 0.03f < veer) {
        float va = veer;
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
        if (f10 == 0 && f2c != -1)
            h = 2.4f;
        else
            h = 1.5f;
        eye.z = h + eye.z;
    }
    float att = attack_speed();
    unsigned fl2 = f533c;
    if ((fl2 & 1) != 0) {
        move_dir() = *(const vector4*)&wp_x - eye;
    } else if ((fl2 & 2) == 0 || target == 0xffffffff) {
        move_dir() = tgt - a_pos(this);
        if (move_dir().length() < 0.001f)
            move_dir() = vel;
    } else {
        Agent* o = agent_by_id(target);
        vector4 op = o->pos;
        float h;
        if (o->f10 == 0 && o->f2c != -1)
            h = 2.4f;
        else
            h = 1.5f;
        op.z = (h - 0.2f) + op.z;
        if (o->f2c < 0 && o->f5610 == 0)
            goto lead;
        if ((fl2 & 0x20) == 0)
            goto lead_done;
        op.z = op.z - (float)(int)(f55c4 & 2) * 0.8f;
lead:
        if ((fl2 & 0x20) != 0 && att > 0.1f) {
            vector4 dv = op - a_pos(this);          // from the agent's position, not the eye (0x449c44: fsub [edi], edi = &pos)
            float dxz = dv.x * dv.x + dv.z * dv.z;   // dy^2 is added last (0x449c69..0x449c7f); in one expression cl summed dz, dy, dx
            float dist = (float)sqrt(dxz + dv.y * dv.y);
            float s = 40.0f - (float)g_game->troops[troop].fb0;
            if (0.0f > s)
                s = 0.0f;
            float j = (frand() - 0.5f) * s * 0.01f + 1.0f;
            float k = (float)g_game->troops[troop].fb0 * 0.02f + 0.6f;
            if (!(k < 1.0f))
                k = 1.0f;
            k = (dist / att) * k;
            vector4 off = k * o->v56b4 * j;
            op.x = off.x + op.x;
            op.y = off.y + op.y;
            op.z = off.z + op.z;
        }
lead_done:
        move_dir() = op - eye;
    }
    float mz = move_dir().z;
    float mlen = move_dir().normalize();
    if ((f533c & 0x20) != 0) {
        float s = 0.2f - (float)g_game->troops[troop].fb0 * 0.005f;
        if (!(s > 0.0f))
            s = 0.0f;
        int r = rand() % 0x3dc9;
        float jit = ((float)r * 6.3223117e-05f - 0.5f) * s + 1.0f;
        float af = accel_factor(&att, &mlen, &mz);
        move_dir().z = af * jit + move_dir().z;
        move_dir().normalize();
    }

    }

    // ---- 5. movement input bits ---------------------------------------------------------------------------------------
    mid = f2c;
    {
    vector4 d = tgt - a_pos(this);
    strafe = 0.0f;
    *(vec2*)&f5368 = *(vec2*)&d;
    angle = 0.0f;
    if (mid == -1 && f10 == 0) {
        vec2 m = *(vec2*)&move_dir();
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
        if (path_active == 0) {
            fa = f288 * 1.4f;
            fb = f288 * 0.9f;
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
        unsigned f = f5390;
        vec2 mv;
        mv.x = 0.0f;
        mv.y = 0.0f;
        if (((f & 1) != 0 && l2 > fb) || ((f & 1) == 0 && l2 > fa)) {
            if (((f & 2) != 0 && l.y > 0.35f) || ((f & 2) == 0 && l.y > 0.45f)) {
                f |= 2;
                in_prev.v[0] |= 1;
                mv.y = 1.0f;
            } else if (((f & 4) != 0 && l.y < -0.35f) || ((f & 4) == 0 && l.y < -0.45f)) {
                f |= 4;
                in_prev.v[0] |= 2;
                mv.y = -1.0f;
            } else {
                f &= 0xfffffff9;
            }
            f5390 = f;
            if (((f & 8) != 0 && l.x > 0.35f) || ((f & 8) == 0 && l.x > 0.45f)) {
                f |= 8;
                in_prev.v[0] |= 4;
                mv.x = 1.0f;
            } else if (((f & 0x10) != 0 && l.x < -0.35f) || ((f & 0x10) == 0 && l.x < -0.45f)) {
                f |= 0x10;
                in_prev.v[0] |= 8;
                mv.x = -1.0f;
            } else {
                f &= 0xffffffe7;
            }
            f5390 = f;
            f5390 = f | 1;
            mv.normalize();
            strafe = l.x * mv.y - mv.x * l.y;
        } else {
            f5390 = 0;
        }
        if ((path_state & 0x20) != 0 && speed_limit > 2.6f)
            in_prev.v[0] = in_prev.v[0] | 0x200000;
    }
    if (mid != -1 || f10 != 0) {
        // ---- follow / mount case ----
        vector4 gp = tgt;
        if (path_active != 0)
            gp = goal;
        d = gp - a_pos(this);
        float gdist = (float)sqrt(d.x * d.x + d.z * d.z + d.y * d.y);
        float hi = 0.8f, lo = 0.5f;
        if (path_active == 0) {
            hi = f288 * 1.2f;
            lo = f288 * 0.8f;
        }
        if ((f5390 != 0 && gdist > lo) || (f5390 == 0 && gdist > hi)) {
            f5390 = 1;
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
            else if (speed_limit <= sp)
                sp = speed_limit;
            speed = sp;
            float gv = goal_dir.length();
            if (gv > sp && d.z * goal_dir.z + d.y * goal_dir.y + d.x * goal_dir.x > 0.1f)
                speed = sp * 0.5f + gv;
            float other = f56e8;
            if (mid != -1)
                other = agent_by_id(mid)->f56e8;
            if (ang - other > 0.1f && ang > 0.1f)
                in_prev.v[0] |= 0x80;
            else if (ang - other < -0.1f && ang < -0.1f)
                in_prev.v[0] |= 0x40;
            if ((f5394 != 0 && ang > 0.1f) || (f5394 == 0 && ang > 0.2f)) {
                f5394 = 1;
                speed = (0.9f - ang * 0.45f) * speed;
            } else if ((f5394 != 0 && ang < -0.1f) || (f5394 == 0 && ang < -0.2f)) {
                f5394 = 1;
                speed = (ang * 0.45f + 0.9f) * speed;
            } else {
                f5394 = 0;
            }
            angle = ang;
            if (speed < 1.2f)
                speed = 1.2f;
            else if (speed > speed_limit)
                speed = speed_limit;
        } else {
            speed = 0.0f;
            f5390 = 0;
        }
        unsigned who = id;
        if (f10 == 0)
            who = mid;
        if (agent_by_id(who)->fac < speed - 0.2f) {
            in_prev.v[0] = in_prev.v[0] | 0x10;
        } else if (speed + 0.2f < agent_by_id(who)->fac && (path_state & 0x200) == 0) {
            in_prev.v[0] = in_prev.v[0] | 0x20;
        }
    }
    }
    }

    // ---- 6. combat state machine -----------------------------------------------------------------------------------------
    if (g_mission->terrain.f184 == 0 && f10 == 0 && target != 0xffffffff && (f533c & 2) != 0) {
        if (!is_enemy(agent_by_id(target)) || agent_by_id(target)->f14 != 1)
            goto no_target;
        Agent* o = g_mission->agents.get(target);
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
        if (combat_state == 0) {
            if ((itype == itp_type_crossbow || itype == itp_type_pistol || itype == itp_type_musket) &&
                p.max_amount() == (int)(p.m4 & 0xff)) {
                ai_timer.start = g_ticks[ai_timer.clock];
                combat_state = 4;
                goto bb72;
            }
            if (fm >= hc) {
                combat_state = 7;
                goto bb72;
            }
            if (fm < hc && frand() < ((float)g_game->troops[troop].fb0 + 1.0f) * 0.2f) {
                int blocker;
                int r = fire_check(target, &blocker);
                f53a0 = r;
                if (r == 1) {
                    combat_state = 1;
                    float x = (dist + 70.0f) * 0.01f;
                    float lo = 0.7f;
                    float hi = 1.2f;
                    float cl = clamp(&x, &lo, &hi);
                    float delay = (((frand() + 2.5f) * cl) * 35.0f) / (float)(g_game->troops[troop].fb0 + 0x23);
                    if (itype == itp_type_thrown)
                        delay = delay * 0.5f;
                    ((Timer*)&ai_timer)->set_delay(-delay);
                    goto bb72;
                }
                if (r == 0) {
                    last_hit = g_hit_shape;
                    int t = (int)pick_threat();
                    if (t < 0) {
                        combat_state = 7;
                        goto bb72;
                    }
                    target = t;
                    goto bb72;
                }
                if (r == 3) {
                    combat_state = 8;
                } else {
                    f2e0 = 2;
                    f2c0 = 0;
                }
            }
            goto bb72;
        }
        if (combat_state == 1) {
            // state 1
            f53bc = f53bc | 0x100;
            int m = f2c;
            if (m == -1 && dist < 2.5f && (o->f2c == m || o->v56b4.out_length() < 1.5f)) {
                f2c0 = 0;
                combat_state = 3;
            }
            if (m == -1 && (itype == itp_type_crossbow || itype == itp_type_bow) && ai_timer.elapsed() > -0.7f)
                in_prev.v[0] = in_prev.v[0] & 0xfffffff0;
            if (!(ai_timer.elapsed() > 0.0f))
                goto bb72;
            {
                int blocker;
                if (fire_check(target, &blocker) == 1) {
                    ((Timer*)&ai_timer)->set_delay(-0.3f);
                    combat_state = 3;
                    goto bb72;
                }
            }
            f53bc = f53bc | 0x200;
            goto bb68;
        }
        if (combat_state == 3) {
            int m = f2c;
            if (m == -1 && dist < 4.0f && (o->f2c == m || o->v56b4.out_length() < 1.0f)) {
                f2c0 = 0;
                combat_state = 0;
            }
            if (m == -1 && (itype == itp_type_crossbow || itype == itp_type_bow))
                in_prev.v[0] = in_prev.v[0] & 0xfffffff0;
            if (ai_timer.elapsed() >= 0.0f)
                combat_state = 0;
            goto bb72;
        }
        if (combat_state == 4) {
            int m = f2c;
            if (m == -1 && dist < 2.5f && (o->f2c == m || o->v56b4.out_length() < 1.5f)) {
                f2c0 = 0;
                combat_state = 0;
            }
            f53bc = f53bc | 0x100;
            if (m == -1)
                in_prev.v[0] = in_prev.v[0] & 0xfffffff0;
            if (p.max_amount() != (int)(p.m4 & 0xff))
                combat_state = 0;
            if (anim[1].cur != 0x7b && ai_timer.elapsed() > 1.0f)
                combat_state = 0;
            goto bb72;
        }
        if (combat_state == 7) {
            if (!(fm < hc))
                goto bb72;
            if (frand() < 0.03f) {
                int t = (int)pick_threat();
                if (t < 0)
                    goto bb72;
                target = t;
            } else {
                if (!line_of_sight(target, last_hit)) {
                    last_hit = g_hit_shape;
                    goto bb72;
                }
            }
            goto bb68;
        }
        if (combat_state == 8) {
            AgentTable* at = &g_mission->agents;
            vec2 tv2;
            a_sub2(tv2, at->get(target)->pos, a_pos(this));
            tv2.normalize();
            vec2 bf = *(vec2*)&body->m.f;
            float fx = bf.x;
            float fy = bf.y;
            if (f2c == -1 || at->get(f2c)->fac > 0.5f)
                goto bb68;
            unsigned bits;
            if (fy * tv2.y + tv2.x * fx > 0.0f)
                bits = in_prev.v[0] | 0x40;
            else
                bits = in_prev.v[0] | 0x80;
            in_prev.v[0] = bits;
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
                    wprof = g_game->troops[troop].f80[cls];
                reach0 = (100.0f / (float)p.stat_1f0a()) * reach0;
            }
            if (combat_state == 3 && active == 1)
                reach0 = reach0 * 0.5f;
            vector4 l2 = o->v56a4 * reach0 + OP;
            float dl = rgl::length(l2 - a_pos(this));
            float reach2 = rgl::length(V) * reach0;
            mc = f2c;
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
                reachf = (float)g_items[p.item].weight * 0.01f + 1.1f;
            if (mc >= 0)
                reachf = reachf - 0.3f;
            if (o->f2c >= 0)
                reachf = reachf + 0.7f;
            ItemRef op0 = o->slot_pair(0);
            if (op0.item != -1)
                r2base = (float)g_items[op0.item].weight * 0.01f + 1.9f;
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
            int st = combat_state;
            if (st <= 1) {
                if (b3f && op0.item != -1 && !b3e && !b59) {
                    f53bc = f53bc | 0x200;
                } else if (outlen < 2.8f) {
                    if (g_game->troops[troop].fb0 >= 0x14 && (b3f || p.item == -1) && !b59 && !b3e)
                        f53bc = f53bc | 0x200;
                }
                if (flag_0x400_or_0x114()) {
                    if (f10 == 0 && f2c == -1)
                        in_prev.v[0] = (in_prev.v[0] & 0xfffffffe) | 2;
                    if (b3f) {
                        {
                            float fr = frand();      // one rand call, doubled (0x44b4a0 fadd st0,st0)
                            ((Timer*)&ai_timer2)->set_delay(-((fr + fr) + 1.3f));
                        }
                        combat_state = 7;
                    }
                    goto bb72;
                }
                if (b5a && !b3e && (frand() < wprof * 0.004f + 0.5f || f2c >= 0)) {
                    ((Timer*)&ai_timer)->set_delay(0.0f);
                    combat_state = 3;
                    goto bb72;
                }
                if (b3f) {
                    if (op0.item == -1)
                        goto b53e;
                    if (!b3e && g_items[op0.item].is_melee() && ai_timer2.elapsed() > 0.0f &&
                        ((o->active == 1 && frand() < 0.3f - (float)g_game->troops[troop].fb0 * 0.01f) ||
                         (o->active == 2 && frand() < (float)g_game->troops[troop].fb0 * 0.05f + 0.5f)) &&
                        b58) {
b508:
                        {
                            float fr = frand();      // one rand call, doubled (0x44b4a0 fadd st0,st0)
                            ((Timer*)&ai_timer2)->set_delay(-((fr + fr) + 1.3f));
                        }
                        combat_state = 7;
                        goto bb72;
                    }
                }
                if (op0.item == -1) {
b53e:
                    if (p.item == -1 && ai_timer2.elapsed() > 0.0f && ofacing > 0.0f &&
                        ((o->active == 1 && frand() < (float)g_game->troops[troop].fb0 * 0.01f + 0.5f) ||
                         (o->active == 2 && frand() < (float)g_game->troops[troop].fb0 * 0.05f + 0.5f)) &&
                        outlen < r2f)
                        goto b508;
                }
                if (!b3f && !b3e && wk->is_melee() && (wk->type_flags & 0x4000) == 0 && ai_timer2.elapsed() > 0.0f &&
                    op0.item != -1 && g_items[op0.item].is_melee() && o->active != 0 && b58 &&
                    frand() < wprof * 0.01f + 0.1f) {
                    ((Timer*)&ai_timer2)->set_delay(-(frand() * 0.5f + 0.6f));
                    combat_state = 2;
                    goto bb72;
                }
                if (dist < 15.0f && frand() < 0.005f && f2c == -1 && (g_mission->terrain.f158 & 1) == 0 &&
                    (!(frand() < 0.2f) || (g_mission->terrain.f158 & 1) != 0)) {
                    int kind = 4;
                    play_class_sound(&kind);
                }
                goto bb72;
            }
            if (st == 2) {
                f53bc = f53bc | 0x200;
                if (ai_timer2.elapsed() > 0.0f)
                    combat_state = 0;
                if (o->active != 0 || !(frand() < 0.9f))
                    goto bb72;
                if (frand() < 0.4f) {
                    ((Timer*)&ai_timer)->set_delay(-0.2f);
                    combat_state = 3;
                    goto bb72;
                }
                goto bb68;
            }
            if (st == 3) {
                if (frand() < 0.02f && f2c == -1 && (g_mission->terrain.f158 & 1) == 0) {
                    int kind = 4;
                    play_class_sound(&kind);
                }
                unsigned fl = f53bc;
                int fa = active;
                f53bc = fl | 0x100;
                if (fa <= 4 && ai_timer.elapsed() > 0.0f && fa == 1) {
                    if (b5a) {
                        int sv = p.stat_1f0a();
                        int sk = g_game->troops[troop].fb0 + 0x28;
                        ((Timer*)&ai_timer)->set_delay(-((4.9f / (float)sk + (float)g_options.difficulty * 0.25f + 0.6f) * (100.0f / (float)sv)));
                        combat_state = 4;
                    } else if (ai_timer.elapsed() > 8.0f || reach0 * 4.0f < distL) {
                        f53bc = fl | 0x300;
                        combat_state = 0;
                    }
                }
                if (flag_0x400_or_0x114())
                    goto bb68;
                goto bb72;
            }
            if (st == 4) {
                if (ai_timer.elapsed() > 0.0f) {
                    combat_state = 0;
                } else if (in_id_set(anim[1].cur)) {
                    combat_state = 0;
                } else {
                    if (b5b && o->f5610 != 0 && ofacing > 0.0f) {
                        if (frand() < 0.03f)
                            combat_state = 0;
                    } else if (g_items[op0.item].is_melee() && !b5b && ofacing > 0.0f) {
                        float zero[3];
                        zero[0] = 0.0f;
                        zero[1] = 0.0f;
                        zero[2] = 0.0f;
                        if (can_hit(id, target, 0, 0, zero) == 0) {
                            if (wprof > 100.0f && frand() < 0.03f)
                                combat_state = 0;
                        } else if (frand() < ((4.0f - (float)g_options.difficulty) * wprof) * 0.0005f) {
                            combat_state = 0;
                        }
                    }
                }
                if (flag_0x400_or_0x114())
                    goto bb68;
                goto bb72;
            }
            if (st == 7) {
                f53bc = f53bc | 0x200;
                if ((op0.item == -1 || g_items[op0.item].is_melee()) && o->active == 0) {
                    if (frand() < 0.4f) {
                        ((Timer*)&ai_timer2)->set_delay(0.0f);
                        ((Timer*)&ai_timer)->set_delay(-0.2f);
                        combat_state = 3;
                    } else {
                        ((Timer*)&ai_timer2)->set_delay(0.0f);
                        combat_state = 0;
                    }
                }
                if (ai_timer2.elapsed() > 0.0f)
                    goto bb68;
                goto bb72;
            }
            goto bb68;
        }
    } else {
no_target:
        if ((g_input_mask | 0x100) != 0 && (g_input_mask | 0x200) == 0)
            f53bc = f53bc | 0x300;
    }
bb68:
    combat_state = 0;
bb72:
    prev_move() = move_dir();
    if ((!(strafe < -0.001f) || !(strafe < 0.001f)) && (in_prev.v[0] & 0xf) != 0 &&
        ((f53bc | g_input_mask) & 0x100) == 0 && combat_state != 4) {
        double a = -asin((double)strafe);
        float c = (float)cos(a);
        double s = sin(a);
        float py = prev_move().y;
        prev_move().y = (float)(s * prev_move().x + c * prev_move().y);
        prev_move().x = (float)(c * prev_move().x - s * py);
    }
    prev_move().normalize();
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
                                if (f2c < 0 || !(ik->type_flags & 0x10000000))
                                    found = true;
                            }
                        } else if (fl & 0x400000) {
                            if (f2c < 0 || !(fl & 0x10000000))
                                found = true;
                        }
                    } else {
                        unsigned fl = ik->type_flags;
                        unsigned t = fl & 0xff;
                        // stand-in: the original keeps the second t == 7 compare (cmp eax,7); written plainly cl folds it into
                        // the first, (t ^ 7) == 0 survives to codegen as xor eax,7 (one byte off), t - 7 == 0 as add eax,-7
                        if ((t == 7 || ((fl & 0x800000) && (t ^ 7) == 0)) &&
                            (f2c < 0 || !(fl & 0x10000000)) && (int)(pairs[v].m4 & 0xffff) > 0)
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
// the missile record of an id: pages of 16 records of 0xb0 bytes at Mission+0x30 (agent3 Pool)
static __forceinline Agent& mag(Mission* m, unsigned id) { return m->agents.pages[id >> 4][id & 0xf]; }
static __forceinline Missile& mrec(Pool* p, unsigned id)
{
    PoolRec** pp = &p->pages[id >> 4];
    return *(Missile*)(*pp + (id & 0xf));
}
#define MS(id) mrec((Pool*)poolJ, id)
// @ 0x44be50
int Mission::spawn_missile(const vector4* pos, const vector4* dir, ItemRef* launcher, ItemRef* ammo,
                           unsigned shooter_id)
{
    Agent* shooter = &(mag(this, shooter_id));
    float mount_f;
    if (shooter->f2c >= 0)
        mount_f = (float)cos(shooter->cur) * (mag(this, shooter->f2c)).fac;
    else
        mount_f = shooter->fac;
    int id = ((Pool*)poolJ)->alloc();
    SceneEnt* e = (SceneEnt*)((rgl::world*)scene)->create_entity(9);
    pos2 ident;
    identity_pos((::pos*)&ident);
    rgl::mesh* m;
    // a launcher of type 10 names the flying mesh itself (0x44bed6..0x44befd read the third argument); otherwise
    // the ammo item's own mesh (0x44bf04.. read the fourth)
    if ((g_items[launcher->item].type_flags & 0xff) == 10) {
        tstring tn = launcher->mesh_name();
        m = (rgl::mesh*)g_resmgr.get_object(&tn);
    } else {
        m = 0;
        if (ammo->item >= 0 && ammo->item < g_item_kind_count)
            m = g_items[ammo->item].mesh_for(0x2000000000000000ui64, ((signed char*)ammo)[7]);
        if (!m) {
            tstring name("flying_missile");
            m = (rgl::mesh*)g_resmgr.get_object(&name);
        }
    }
    if (m) {
        rgl::mesh* c = m->clone();
        c->m_cc = ident;
        e->add_extra(c);
    }
    e->local.f = *dir;
    e->local.u = vector4(0.0f, 0.0f, 1.0f);
    e->local.orthonormalize();
    float half = 0.5f;
    e->local.o = *pos;
    e->set_fbc(&half);
    e->vc4 = vector4(0.0f, e->get_fbc(), 0.0f);
    e->f140 = 0;
    MS(id).ent = e;
    MS(id).p20 = *pos;
    MS(id).p10 = *pos;
    MS(id).p40 = *dir;
    MS(id).f64 = (float)launcher->stat_21x(1);
    unsigned lt = g_items[launcher->item].type_flags & 0xff;
    if (lt == 8 || lt == 9)
        MS(id).f64 += (float)ammo->stat_21x(1);
    MS(id).f60 = shooter->attack_speed();
    MS(id).f5c = MS(id).f60;
    MS(id).f5c = mount_f + MS(id).f5c;
    MS(id).shooter = shooter_id;
    MS(id).launcher = *launcher;
    MS(id).ammo = *ammo;
    MS(id).init();
    return id;
}

// ---- Mission::spawn_prop: instantiate scene-prop record props[*idx] as a world entity (meshes, collision shape, 3D sounds) ----
// views of tables of other objects (offsets read here): g_ptypes entries 0x48 bytes at +0x1c, count at +0x18; g_propkinds records
// flags +0x10, meshes +0x40, shape +0x50, sounds +0x6c; ItemKind sounds +0x228
struct Rec20Entry { tstring name; char pad10[0x20 - 0x10]; body_part* shape; char pad24[0x48 - 0x24]; };   // +0x20 the body resource
struct Rec20V { char pad0[0x18]; int count; Rec20Entry* entries; };
static inline Rec20V& r20(int i) { return *(Rec20V*)&g_ptypes[i]; }
static inline unsigned r78_flags(Rec78* r) { return *(unsigned*)((char*)r + 0x10); }
static inline std::vector<rgl::mesh*>& r78_meshes(Rec78* r) { return *(std::vector<rgl::mesh*>*)((char*)r + 0x40); }
static inline body_part* r78_shape(Rec78* r) { return *(body_part**)((char*)r + 0x50); }
static inline TimerList& rsnd(Rec78* r) { return *(TimerList*)((char*)r + 0x6c); }
static inline TimerList& isnd(ItemKind* k) { return *(TimerList*)((char*)k + 0x228); }

#define P_ENT(r) (*(SceneEnt**)&(r)->ent)              // PlacedObj +0x1b4 / +0x1b8: the scene entities of the prop
#define P_ENT2(r) (*(SceneEnt**)&(r)->m1b8)
#define ents (*(EntVec*)((char*)this + 0x63c0))         // Mission +0x63c0: the entity list of the props
// mesh of an item for the prop display class, null for an invalid item index (bounds check inlined)
static inline rgl::mesh* item_mesh(int item, int packed)
{
    if (item < 0 || item >= g_item_kind_count)
        return 0;
    return g_items[item].mesh_for(0x3000000000000000ui64, packed >> 24);
}
int find_item(const tstring* name);                     // 0x434ad0 (cdecl)

// @ 0x44c210
void Mission::spawn_prop(int* idx, int* id)
{
    PlacedObj* r = (PlacedObj*)&props[*idx];
    r->m18 = r->m98;
    MeshVec meshes;
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
        r->value = r->value % r20(ti).count;
        name = r20(ti).entries[r->value].name;
        body_part* rs = r20(ti).entries[r->value].shape;
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
            Rec78* rec = &g_propkinds[tbl];
            if (g_cfg.enable_edit_mode != 0 || !(r78_flags(rec) & 0x4000))
                meshes = r78_meshes(rec);
            shape_res = r78_shape(rec);
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
        {
            std::vector<rgl::mesh*> found = g_resmgr.find_meshes(&name);
            meshes = *(const std::vector<rgl::mesh*>*)&found;
        }
    if (s2.differs("0") && s2.len != 0)
        shape_res = (body_part*)g_resmgr.get_object60(s2);

    if (r->kind == 7) {
        r->id = find_item(&r->name);
        SceneEnt* e = (SceneEnt*)((rgl::world*)scene)->create_entity(7);
        rgl::mesh** first = meshes._Myfirst;
        for (unsigned i = 0; first != 0 && i < (unsigned)(meshes._Mylast - first); i++)
            e->add_extra(first[i]->clone());
        if (shape_res)
            e->shape = shape_res->clone_shape();
        e->local = r->m98;
        e->type = 0xc;
        e->id = *idx;
        P_ENT2(r) = e;
        e->f170 = 2.0f;
        float a = (float)(rand() % 0x3dc9) * (float)(1.0 / 15817);
        float b = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817) + 1.0f;
        float c = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817);
        e->v19c = a_vadd(a_vadd(vmul(c, e->local.f), vector4(0.0f, 0.0f, b)), vmul(a, e->local.u));
        e->f15c = 0.4f;
        float a2 = (float)(rand() % 0x3dc9) * (float)(9.0 / 15817);
        float c2 = (float)(rand() % 0x3dc9) * (float)(4.0 / 15817);
        float d = (float)(rand() % 0x3dc9) * (float)(6.0 / 15817);
        e->f1d4 = 0;
        e->v1ac = a_vadd(a_vadd(vmul(d, r->m18.s), vmul(c2, r->m18.f)), vmul(a2, r->m18.u));
    } else {
        SceneEnt* e = (SceneEnt*)((rgl::world*)scene)->create_entity(9);
        for (unsigned i = 0; i < meshes.size(); i++) {
            rgl::mesh* m = meshes[i]->clone();
            if (r->kind == 4) {
                vector4 dir, col;
                ((rgl::world*)scene)->light_at(r->m18.o, vector4(0.0f, 0.0f, 1.0f), &dir, &col, 1, 1.0f);
            }
            if (r->kind == 0 && tbl >= 0 && tbl < g_propkind_count && (r78_flags(&g_propkinds[tbl]) & 0x12000))
                m->mask = 0xffff;
            else
                m->mask = 0x1ffff;
            if (r->v158.z * r->v158.y * r->v158.x < 0.0f)
                m->f4c = 1;
            e->add_extra(m);
        }
        if (shape_res) {
            if (r->kind == 3 || r->kind == 5 ||
                (r->kind == 0 && tbl >= 0 && ((r78_flags(&g_propkinds[tbl]) & 0xff) == 5 || (r78_flags(&g_propkinds[tbl]) & 0xff) == 10)))
                e->shape = shape_res->copy_shape();
            shape_base* sh = shape_res->clone_shape();
            int v = 8;
            if (r->kind == 0 && tbl >= 0) {
                unsigned fl = r78_flags(&g_propkinds[tbl]);
                if ((char)fl == 8)
                    v = 0x10c;
                else if (fl & 0x4000)
                    v = 0xc;
                else if (fl & 0x8000)
                    v = 0;
            }
            sh->set_m8(v);
            sh->scale((const float*)&r->v158);
            sh->transform((const pos*)&r->m18);
            e->set_shape(sh);
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
                sh->scale((const float*)&r->v158);
                sh->transform((const pos*)&r->m18);
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
                sh->scale((const float*)&r->v158);
                e->shape = sh;
            }
        }
done_shape:
        e->set_frame(&r->m18, &r->v158);
        r->m58 = r->m18;
        e->type = 0xc;
        e->id = *idx;
        P_ENT(r) = e;
        ents.push_back(e);
    }

    int ty = r->kind;
    if (ty == 0) {
        int ri = r->id;
        if (ri >= 0 && ri < g_propkind_count && rsnd(&g_propkinds[ri]).count > 0) {
            TimerList* ss = &rsnd(&g_propkinds[ri]);
            ((Placement*)g_game)->set_matrix(&r->m18);
            g_game->f1a1f0 = *idx;
            SceneEnt* en = P_ENT(r);
            if (!(en->meshes.size() > 0)) { g_game->f1a200 = 0; } else { g_game->f1a200 = (int)en->meshes[0]; }
            g_game->f1a1fc = 0;
            g_script.param1 = *idx;
            ss->update(-0x28);
        }
    } else if (ty == 2 || ty == 6 || ty == 7) {
        int ri = r->id;
        if (ri >= 0 && ri < g_item_kind_count && isnd(&g_items[ri]).count > 0) {
            TimerList* ss = &isnd(&g_items[ri]);
            int who = *id;
            int aid = -1;
            if (who >= 0)
                aid = mag(this, who).troop;
            g_game->f1a1f4 = who;
            g_game->f1a1f8 = aid;
            g_game->f1a204 = -1000;
            SceneEnt* en = P_ENT(r);
            if (en && en->meshes.size() > 0) {
                g_game->f1a200 = (int)en->meshes[0];
            } else {
                SceneEnt* e2 = P_ENT2(r);
                if (e2 && e2->meshes.size() > 0)
                    g_game->f1a200 = (int)e2->meshes[0];
                else
                    g_game->f1a200 = 0;
            }
            g_game->f1a1fc = 0;
            g_script.param1 = *id;
            g_script.param2 = aid;
            ss->update(-0x32);
        }
    }
    ((Agent*)r)->init_max_hp();
}
#undef ents

// ---- std::vector<E56>::insert (0x44cf90) ----------------------------------------------------------------------------------
// @ 0x44cf90
template std::vector<E56>::iterator std::vector<E56>::insert(std::vector<E56>::iterator, const E56&);


// ---- Agent::choose_action: the AI decision (15 scored candidate actions) ------------------------------------------------------
#undef AG
#define AG(i) (g_mission->agents.pages[(unsigned)(i) >> 4][(unsigned)(i) & 0xf])   // unsigned index: shr, as the original
#define ZERO_S for (int zi = 0; zi < 15; zi++) s[zi] = 0.0f;
#define tf_hero 0x00000010i64
#define tf_guarantee_horse 0x01000000i64
// @ 0x44d000
void Agent::choose_action(int unused)
{
    vector4 pv[15];                             // target position per kind
    int ids[15];                                // target id per kind
    ClassInfo* cr;
    float s[15];                                // score per kind
    vector4 hp;
    unsigned cand;
    float dist;
    int cls = team;
    if (cls < 0 || cls >= 8) cls = 7;
    int kind = 0;
    if (troop >= 0) {
        if (f2c >= 0)
            kind = 2;
        else
            kind = (g_game->troops[troop].flags30 & tf_guarantee_ranged) ? 1 : 0;
    }
    cr = &g_mission->teams[cls].cls[kind];

    last_attacker = -1;
    f28c = 0;
    f288 = 1.5f;
    ZERO_S

    if (f10 == 1) {
        if (scripted_mode == 1) {
            ZERO_S
            s[1] = 10000.0f;
            pv[1] = scripted_destination.o;
        } else if (f5844) {
            s[14] = 10000.0f;
        } else {
            s[13] = 10000.0f;
        }
    } else if (!((*(unsigned char*)&f2c4) & 1)) {
        if (scripted_mode == 1) {
            s[1] = 10.0f;
            pv[1] = scripted_destination.o;
        }
        s[11] = (float)(rand() % 15817) * 9.4834677e-06f + 0.3f;
    } else {
        collect_enemies((int)&hp);
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
            target_mounted = m->agents.pages[cand >> 4][cand & 0xf].f2c >= 0;
        }
        if (scripted_mode == 1) {
            ZERO_S
            s[1] = 10.0f;
            pv[1] = scripted_destination.o;
        } else {
            if (nthreat) {
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
                    if (f2c != -1) {
                        if (weapon_mode == 2)
                            s[6] = (frand() * 2.0f + 3.5f) * s[6];
                        else if (f5570 < -0.5f)
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
                if (f2c != -1) {
                    s[5] = s[5] * 0.2f;
                    s[4] = 0.0f;
                    if (has_weapon_for(3) || has_weapon_for(1)) {
                        int best = -1;
                        float bests = -100.0f;
                        for (int i = 0; i < nthreat; i++) {
                            float w = 50.0f / (threats[i].dist + 25.0f);
                            if (!(w < 1.0f)) w = 1.0f;
                            int n = AG(threats[i].id).f55e8 - 1;
                            w = w / ((float)(n < 0 ? 0 : n) + 1.0f);
                            float d = threats[i].dir.x * vel.x + threats[i].dir.z * vel.z + vel.y * threats[i].dir.y;
                            if (d > 0.0f) w = (d * d * 20.0f + 1.0f) * w;
                            float sc = ((float)(rand() % 15817) * 6.3223117e-05f + 0.25f) * w * 0.5f;
                            if (AG(threats[i].id).f130 & 0x10)
                                sc = sc * 1.5f;
                            else if (AG(threats[i].id).f130 & 0x20)
                                sc = sc * 1.5f;
                            if (sc > bests) {
                                best = i;
                                bests = sc;
                            }
                        }
                        ids[7] = threats[best].id;
                        s[7] = bests;
                    }
                }
            } else {
                if ((g_mission_templates[g_mission->mode].flags >> 1) & 1) {
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
            if (f10 == 0) {
                int k = cr->kind;
                if (k == 1 || (k == 2 && leader != id)) {
                    if ((int)cand >= 0) {
                        Agent* o = &AG(cand);
                        vector4 d = vsub(o->pos, cr->pos);
                        if ((float)sqrt(d.z * d.z + d.x * d.x + d.y * d.y) > cr->half + cr->half && dist > 10.0f)
                            s[7] = s[7] * 0.0f;
                    }
                    if (k == 2 && leader != id) {
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
                if (cr->k4 == 1 && f2c == -1) {
                    Troop* T = &g_game->troops[troop];
                    if ((T->flags30 & tf_hero) || (T->flags30 & tf_guarantee_horse)) {
                        AgentTable* tbl = &g_mission->agents;
                        float best = 10000.0f;
                        int bestid = -1;
                        unsigned it = tbl->first_used();
                        unsigned cnt = tbl->count;
                        if (it < cnt) {
                            do {
                                Agent* a = &AG(it);
                                if (a->f10 == 1 && a->f30 == -1 && a->f5830 == 2) {
                                    int v = a->f8c.stat_1ec();
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
                if (cr->k4 == 2 && f2c != -1 && dist > 5.0f) s[3] = 50.0f;
            }
        }
    }
randomise:
    for (int i = 0; i < 15; i++) s[i] = ((float)(rand() % 15817) * 1.2644623e-05f + 0.9f) * s[i];
    if (f2e0 == 2) {
        s[5] = s[5] * 0.1f;
        s[6] = s[6] * 0.5f;
    }
    int best = 0;
    for (int i = 1; i < 15; i++)
        if (s[i] > s[best]) best = i;

    int saved = f2c8;
    start_attack(best);
    f2cc = ids[best];
    f2d0 = pv[best];
    f2e4 = 0;
    f2e8 = -1;
    if (best == 11 || best != 1) {
        if (best == 5 || best == 6) {
            int t = pick_threat();
            if (t >= 0) f2cc = t;
        }
    } else {
        int k = cr->kind;
        float zero;
        if (k == 1) {
            zero = 0.0f;
            goal_near_for_class(&f2d0, &zero);
        } else {
            bool other = false;
            if (k == 2) {
                int c2 = team;
                if (c2 < 0 || c2 >= 8) c2 = 7;
                if (g_mission->teams[c2].leader != id) {
                    zero = 0.0f;
                    goal_near_for_class(&f2d0, &zero);
                    other = true;
                }
            }
            if (!other) {
                if (k == 3 && scripted_mode != 1) {
                    if (f53a0 == 0) {
                        vector4 p;
                        p.x = body->m.f.x + pos.x;
                        p.y = body->m.f.y + pos.y;
                        p.z = body->m.f.z + pos.z;
                        int fkind = 2;
                        int face = update_nav_face();
                        float speed = 2.0f;
                        pick_reachable_goal(&p, &speed, &face, &fkind);
                    }
                } else {
                    zero = 0.0f;
                    set_goal_near(&f2d0, &zero);
                }
            }
        }
        path_state |= 0x20;
        if (f2c < 0 && f10 != 1) {
            if (scripted_mode == 0)
                f288 = 1.0f;
            else
                f288 = 1.75f;
        } else {
            f288 = 3.0f;
        }
    }
    f2e0 = 0;
    if (f2c8 != saved) f2ec = 0;
}


// ---- Agent::ai_idle_tick: the per-frame timer block of the AI controller ----------------------------------------------------
// @ 0x44df30
void Agent::ai_idle_tick(const float* dt)
{
    int alive = f14;
    g_input_flags = 0;
    if (alive != 1) {
        in_prev.v[0] = 0;
        return;
    }
    float v = this->f2a4() - *dt * 0.002f;
    this->f2a4() = v;
    if (v > 1.0f)
        this->f2a4() = v - *dt * 0.05f;
    if (this->f2a4() < 0.5f)
        this->f2a4() = 0.5f;
    if (f290 < 9.0f)
        f290 = f290 + *dt;
    if (attack_delay() < (float)(g_ticks[ai_ta().clock] - ai_ta().start) * 1e-05f) {
        ai_ta().start = g_ticks[ai_ta().clock];
        attack_delay() = (float)(rand() % 0x3dc9) * 0.00012644623f + 5.0f;
        choose_action(0);
    }
    if (f2ec_f() < (float)(g_ticks[ai_tb.clock] - ai_tb.start) * 1e-05f) {
        f55c4 = rand();
        update_ai();
        ai_tb.start = g_ticks[ai_tb.clock];
        f2ec_f() = ((float)(rand() % 0x3dc9) * 6.322312e-05f + 0.5f) * g_dt_scale;
    }
    if (f310_f() <= (float)(g_ticks[ai_tc.clock] - ai_tc.start) * 1e-05f) {
        update_alarm();
        in_prev.v[0] = 0;
        g_input_mask = f53bc;
        f53bc = 0;
        ai_update_control();
        ai_tc.start = g_ticks[ai_tc.clock];
        E32 e;
        float r1 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        e.v[0] = f53bc;
        float hi = (float)g_options.difficulty * 0.02f + 0.2f / (float)(g_game->troops[troop].fb0 + 10);
        f310_f() = (r1 * 0.03f + hi) + 0.05f;
        float r2 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        float ga = (float)g_options.difficulty * 0.1f;
        float rb = (r2 + 1.0f) * 0.1f;
        float t = ga + rb;
        if (troop >= 0)
            t = t / ((float)g_game->troops[troop].fb0 + 30.0f) * 40.0f;
        *(__int64*)&e.v[6] = (__int64)(t * 100000.0f + g_ticks[2]);
        in_prev.v[0] = in_prev.v[0] | g_input_flags;
        ((std::deque<E32>*)((char*)this + 0x53c0))->push_back(e);
    }
}


// ---- Agent::shoot: fire a missile from the agent's hand ---------------------------------------------------------------------
// @ 0x44e2b0
void Agent::shoot()
{
    ItemRef curp = cur_pair_fi();
    ItemKind* ik = &g_items[curp.item];

    Skeleton* an = body->skel;
    if (!an->done) {
        if (an->has_poses) ((FxAnim*)an)->update_poses(); else ((FxAnim*)an)->update_poses2();
    }
    BoneW* bones = (BoneW*)an->recs;
    const rgl::matrix* W = &bones[9].world;
    vector4 P(W->o.x * 0.65f, W->o.y * 0.65f, W->o.z * 0.65f);
    an = body->skel;
    if (!an->done) {
        if (an->has_poses) ((FxAnim*)an)->update_poses(); else ((FxAnim*)an)->update_poses2();
    }
    const vector4* o19 = &((BoneW*)an->recs)[19].world.o;
    const rgl::matrix* m = &body->m;
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
    if (f2c >= 0) {
        vector4 tmp;
        rotate_scale_xy(&tmp, &vel, &cur, (vector4*)&fx);
        d0 = tmp;
    }
    float spr = aim_error;
    Mission* ms = g_mission;
    int tid = target;
    if (ms->player_id != id && tid >= 0) {
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
        if (ms->f6610 < 200.0f) err = dist / ms->f6610 * 0.05f + err;
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
    mid = g_mission->spawn_missile(&P, &shot, &launcher, &ammo, id);
    }
    unsigned __int64 shoot_bits = ik->flags & 0xff000ui64;
    if (shoot_bits == 0x30000ui64) {
        mrec((Pool*)g_mission->poolJ, mid).f98 = 6.0f;
        return;
    }
    if (shoot_bits == 0x20000ui64) {
        mrec((Pool*)g_mission->poolJ, mid).f98 = 12.0f;
        return;
    }
    mrec((Pool*)g_mission->poolJ, mid).f98 = 0.0f;
}


using rgl::mesh;
using rgl::mat4;

#define itp_two_handed 0x200000
#define itcf_carry_mask 0x00000007f0000000i64


// ---- rgl::mesh members --------------------------------------------------------------------------------------------------
// @ 0x44e970
void rgl::mesh::add_quad(const vector4* p0, const vector4* p1, const vec2* uv0, const vec2* uv1, const unsigned* color)
{
    int pts[4];
    int vts[4];
    pts[0] = add_point(p0);
    { vector4 q; q.x = p1->x; q.y = p0->y; q.z = p0->z; pts[1] = add_point(&q); }
    { vector4 q; q.x = p1->x; q.y = p1->y; q.z = p0->z; pts[2] = add_point(&q); }
    { vector4 q; q.x = p0->x; q.y = p1->y; q.z = p0->z; pts[3] = add_point(&q); }
    vector4 n(0.0f, 0.0f, 1.0f);
    int f5c = 0;
    vts[0] = add_vertex(pts[0], uv0, color, &n, &f5c);
    { vec2 u; u.x = uv1->x; u.y = uv0->y; f5c = 0; vts[1] = add_vertex(pts[1], &u, color, &n, &f5c); }
    f5c = 0;
    vts[2] = add_vertex(pts[2], uv1, color, &n, &f5c);
    { vec2 u; u.x = uv0->x; u.y = uv1->y; f5c = 0; vts[3] = add_vertex(pts[3], &u, color, &n, &f5c); }
    add_face(vts[0], vts[1], vts[2]);
    add_face(vts[0], vts[2], vts[3]);
    slots[0].f28 = 1;
    slots[1].f28 = 1;
}
// @ 0x44eb00
void rgl::mesh::append_skinned(mesh* other, const mat4* m, int bone, unsigned col, char flip)
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    int npts = (int)v70.size();
    int nverts = (int)v50.size();
    if (other->f228 > f228) f228 = other->f228;
    if (bone + 1 > f228) f228 = bone + 1;
    std::vector<point20>& pts = other->get70_rec();
    std::vector<vertex60>& vs = other->get50_rec();
    std::vector<skin_vert>& sk = other->get60_rec();
    vector4 p;
    for (unsigned i = 0; i < pts.size(); i++) {
        const vector4& q = pts[i].a;
        p.x = m->f.x * q.y + m->u.x * q.z + q.x * m->s.x + m->o.x;
        p.y = m->s.y * q.x + m->f.y * q.y + m->u.y * q.z + m->o.y;
        p.z = m->s.z * q.x + m->f.z * q.y + m->u.z * q.z + m->o.z;
        add_point(&p);
    }
    if (sk.size() != 0) {
        if (v60.size() < v50.size())
            build_skin();
        for (unsigned i = 0; i < sk.size(); i++) {
            skin_vert e = sk[i];
            v60.push_back(e);
        }
    } else {
        if (bone >= 0) {
            if (v60.size() < v50.size())
                build_skin();
            for (unsigned i = 0; i < vs.size(); i++) {
                skin_vert e;
                e.w[0] = 1.0f; e.w[3] = 0.0f; e.w[2] = 0.0f; e.w[1] = 0.0f;
                e.bone[0] = bone; e.bone[3] = 0; e.bone[2] = 0; e.bone[1] = 0;
                e.bonef[0] = (float)bone; e.bonef[3] = 0; e.bonef[2] = 0; e.bonef[1] = 0;
                v60.push_back(e);
            }
        }
    }
    vector4& tint = p;
    new ((void*)&tint) vector4(rgl::unpack_color(&col));   // constructed in place: the hidden return pointer is &tint
    for (unsigned i = 0; i < vs.size(); i++) {
        const vector4& n = vs[i].pos;
        vertex60 v = vs[i];
        vector4 t;
        t.x = n.x * m->s.x + (m->f.x * n.y + m->u.x * n.z);
        t.y = m->f.y * n.y + (m->u.y * n.z + n.x * m->s.y);
        t.z = m->f.z * n.y + (m->u.z * n.z + n.x * m->s.z);
        v.pos = t;
        v.normal = t;
        v.index += npts;
        vector4 tmp;
        vector4 c = rgl::unpack_color(&v.color);
        c.x = c.x * tint.x; c.y = c.y * tint.y; c.z = c.z * tint.z; c.w = c.w * tint.w;
        v.color = rgl::pack_color(&c);
        v.f58 = v.color;
        v50.push_back(v);
    }
    std::vector<face20>& fs = other->get90_rec();
    for (unsigned i = 0; i < fs.size(); i++) {
        if (!(flip)) { add_face(fs[i].a + nverts, fs[i].b + nverts, fs[i].c + nverts); } else { add_face(fs[i].a + nverts, fs[i].c + nverts, fs[i].b + nverts); }
    }
    slots[0].f28 = 1;
    slots[1].f28 = 1;
    bounds.extend_box(&other->bounds, m);
}
// @ 0x44f0c0
void rgl::mesh::scale(float s)
{
    ensure_local();
    for (unsigned i = 0; i < v70.size(); i++) {
        point20* p = &(&*v70.begin())[i];
        p->a.x = s * p->a.x;
        p->a.y = s * p->a.y;
        p->a.z = s * p->a.z;
        p = &(&*v70.begin())[i];
        p->b.x = s * p->b.x;
        p->b.y = s * p->b.y;
        p->b.z = s * p->b.z;
    }
    for (unsigned j = 0; j < (unsigned)kf.count; j++) {
        for (vector4* q = kf.frames[j].verts._Myfirst; q != kf.frames[j].verts._Mylast; q++) {
            q->x = s * q->x;
            q->y = s * q->y;
            q->z = s * q->z;
        }
    }
    char* dirty = &slots[0].f28;
    *dirty = 1;
    bounds.min.x = s * bounds.min.x;
    bounds.min.y = s * bounds.min.y;
    bounds.min.z = s * bounds.min.z;
    bounds.max.x = s * bounds.max.x;
    bounds.max.y = s * bounds.max.y;
    bounds.max.z = s * bounds.max.z;
    bounds.center.x = s * bounds.center.x;
    bounds.center.y = s * bounds.center.y;
    bounds.center.z = s * bounds.center.z;
    bounds.radius = s * bounds.radius;
}
// @ 0x44f240
void rgl::mesh::translate(const vector4* d)
{
    ensure_local();
    for (unsigned i = 0; i < v70.size(); i++) {
        v70[i].a += *d;
        v70[i].b += *d;
    }
    for (int j = 0; j < kf.count; j++) {
        for (unsigned k = 0; k < kf.frames[j].verts.size(); k++) {
            kf.frames[j].verts[k] += *d;
        }
    }
    char* dirty = &slots[0].f28;
    *dirty = 1;
    bounds.min += *d;                   // through vector4::operator+= (o.x + x order and its expansion's keys: min.z, max.x orientation)
    bounds.max += *d;
    bounds.center.x = bounds.center.x + d->x;
    bounds.center.y = bounds.center.y + d->y;
    bounds.center.z = bounds.center.z + d->z;
}

// ---- Font::draw_glyph, the carried ranged item mesh, add_placed, the Newton world ---------------------------------------------------
// the font glyph record: the slices read it as {u0, v0, u1, v1, xoff, yoff, xadv}; font.h's Glyph has the same seven words
// (key, f4, f8, fc, f10, f14, f18), the key word being the left u of an ascii entry
static __forceinline const Glyph* font_get(Font* f, int c)
{
    if (c >= 0x200) {
        IntHashBase::iterator it = ((IntHashBase&)f->map).lower_bound(c);   // the returned iterator is the local itself
        GlyphNode* n = *(GlyphNode**)&it;
        if (n != f->map.head)
            return &n->val;
        return &f->dflt;
    }
    return &f->ascii[c];
}
struct vec2f { float x, y; };

// @ 0x44f3c0
void rgl::mesh::transform(mat4 m)
{
    ensure_local();
    unsigned i;
    for (i = 0; i < v70.size(); i++) {
        vector4 ta, tb;
        point20* p = &(&*v70.begin())[i];
        ta.x = m.u.x * p->a.z + m.f.x * p->a.y + m.s.x * p->a.x + m.o.x;
        ta.y = m.u.y * p->a.z + m.f.y * p->a.y + m.s.y * p->a.x + m.o.y;
        ta.z = m.u.z * p->a.z + m.f.z * p->a.y + m.s.z * p->a.x + m.o.z;
        p->a = ta;
        p = &(&*v70.begin())[i];
        tb.x = m.u.x * p->b.z + m.f.x * p->b.y + m.s.x * p->b.x + m.o.x;
        // the original sums y and z as (f*by + u*bz) + s*bx (0x44f44a..0x44f4e8); MSVC reorders a commutative
        // chain written in one expression, so the order is pinned with a running sum
        float ty = m.f.y * p->b.y;
        ty = ty + m.u.y * p->b.z;
        ty = ty + m.s.y * p->b.x;
        tb.y = ty + m.o.y;
        float tz = m.f.z * p->b.y;
        tz = tz + m.u.z * p->b.z;
        tz = tz + m.s.z * p->b.x;
        tb.z = tz + m.o.z;
        p->b = tb;
    }
    for (i = 0; i < v50.size(); i++) {
        vector4 tb;
        vertex60* v = &(&*v50.begin())[i];
        // the original sums (s*nx + f*ny) + u*nz (0x44f538..0x44f5a1); pinned with a running sum, see above
        float nx = m.s.x * v->normal.x;
        nx = nx + m.f.x * v->normal.y;
        tb.x = nx + m.u.x * v->normal.z;
        float ny = m.s.y * v->normal.x;
        ny = ny + m.f.y * v->normal.y;
        tb.y = ny + m.u.y * v->normal.z;
        float nz = m.s.z * v->normal.x;
        nz = nz + m.f.z * v->normal.y;
        tb.z = nz + m.u.z * v->normal.z;
        v->normal = tb;
    }
    slots[0].f28 = 1;
    compute_bounds();
}
// @ 0x44f5d0
void Font::draw_glyph(int ch, const vector4* pos, const float* scale_, mesh* m, int arg)
{
    const vec2f* scale = (const vec2f*)scale_;
    const Glyph* g = font_get(this, ch);
    float dv = g->fc - g->f4;
    vector4 a = *pos;
    vec2f s;                            // the glyph size as a two-float struct: its lanes are field views, not substituted temporaries
    s.y = dv * scale->y;
    a.y = (g->f14 * scale->y - s.y) + a.y;
    s.x = (g->f8 - (*(float*)&g->key)) * scale->x;
    vector4 b(a.x + s.x, a.y + s.y, a.z);
    m->add_quad(&a, &b, (const vec2*)&g->key, (const vec2*)&g->f8, (const unsigned*)arg);
}

// item flag field at ItemKind::flags: (flags & 0xff000) selects a visual class (module-system itp_* bits 12..19)
enum { ITEM_CLASS_MASK = 0xff000, ITEM_CLASS_A = 0x10000, ITEM_CLASS_B = 0x20000, ITEM_CLASS_C = 0x30000 };

static inline tstring item_mesh_name(ItemKind* items, const ItemRef& r)
{
    if (r.item >= 0 && r.item < g_item_kind_count)
        return items[r.item].mesh_name_for((int)r.m4 >> 24);
    return tstring("Error: Invalid item");
}
static __forceinline rgl::mesh* get_obj(const tstring& n) { return (rgl::mesh*)g_resmgr.get_object(&n); }
static __forceinline mesh* build_ranged(int i, ItemRef r)
{
    mesh* result = 0;
    if (i >= 0) {
        mesh* obj = (mesh*)get_obj(item_mesh_name(g_items, r));
        float s = (float)g_items[i].weight * 0.01f;
        if (obj) {
            result = obj->clone();
            vector4 d;
            d.y = -s;
            d.x = 0.0f;
            d.z = 0.0f;
            result->translate(&d);
            pos2 rot;
            identity_pos((::pos*)&rot);
            float angle = 1.5707964f;
            ((rgl::matrix&)rot).rotate_about_f(&angle);
            if ((char)g_items[i].type_flags == 10) {
                if ((g_items[i].flags & ITEM_CLASS_MASK) == ITEM_CLASS_C) {
                    angle = frand() * 0.4f + 0.5f;
                    ((rgl::matrix&)rot).rotate_about_u(&angle);
                }
                if ((g_items[i].flags & ITEM_CLASS_MASK) == ITEM_CLASS_B) {
                    angle = frand() * 0.4f + 3.2415926f;
                    ((rgl::matrix&)rot).rotate_about_u(&angle);
                }
            }
            result->transform(*(mat4*)&rot);
        }
    }
    return result;
}

// @ 0x44f6a0
mesh* Wielder::make_ranged_mesh()
{
    ItemKind* items = g_items;
    int i = -1;
    int a = this->a.item;
    if (a >= 0) {
        i = a;
        if ((char)items[a].type_flags != 10)
            i = this->b.item;
    }
    return build_ranged(i, ranged_ref());
}

#include "../prop_vec.h"      // PropVec: the one definition (agent4, map_editor, script)

// @ 0x44f870
int Mission::add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn_id)
{
    int cnt = 0;
    int prev = -1;
    int idx = (int)props.size();
    {
        PlacedObj rec;                                            // its inlined constructor calls reset (0x4352f0, defined as PlacedObj::reset in agent.cpp): shared rec1c8.h
        props.push_back(rec);       // 0x440b80 (the explicit instantiation of agent_combat.cpp)
        init_placed(&props[idx], kind, id, value, m);
        if (*kind == 7 || *kind == 6)
            props[idx].m10 = 2;
        if (*kind == 1) {
            cnt = 0;
            prev = -1;
            for (int k = 0; k < 0x28; k++) {
                if (prev >= cnt)
                    break;
                prev = cnt;
                for (unsigned j = 0; j < props.size(); j++) {
                    if (props[j].kind == 1 && props[j].value == cnt)
                        cnt++;
                }
            }
            if (cnt < 0x40)
                props[idx].value = cnt;
        }
        spawn_prop(&idx, spawn_id);
    }
    return idx;
}

// the freshly created record as the original builds it: the two vectors are zeroed in place and no cleanup follows
#include "../int_vec.h"                      // RawVec: allocator dword + first / last / end
struct PhysRecInit {
    int f0;
    const NewtonWorld* world;
    const NewtonRagDoll* ragdoll;
    RawVec bones;
    RawVec bodies;
    const NewtonCollision* collision;
    const NewtonBody* body;
    int material;
};

// @ 0x44fa40
template void std::vector<E56>::push_back(const E56&);

// @ 0x44fad0
unsigned PhysSet::create_world()
{
    unsigned idx = 0;
    PhysRec* p = &recs[0];
    while (idx < recs.size() && p->world != 0) {
        idx++;
        p++;
    }
    NewtonWorld* w = NewtonCreate(0, 0);
    NewtonSetSolverModel(w, 6);
    {
        vector4 mx, mn;
        mn.x = -1000.0f; mn.y = -1000.0f; mn.z = -100.0f;
        mx.x = 1000.0f; mx.y = 1000.0f; mx.z = 1000.0f;
        NewtonSetWorldSize(w, &mn.x, &mx.x);
    }
    if (idx == recs.size()) {
        PhysRecInit r;
        r.ragdoll = 0;
        r.collision = 0;
        r.body = 0;
        r.world = w;
        ((std::vector<E56>&)recs).push_back(*(const E56*)&r);        // 0x44fa40 (PhysRec is the 0x38-byte E56 record)
    } else {
        recs[idx].world = w;
    }
    recs[idx].material = NewtonMaterialGetDefaultGroupID(w);
    recs[idx].f0 = 0;
    NewtonMaterialSetDefaultSoftness(w, recs[idx].material, recs[idx].material, 0.1f);
    NewtonMaterialSetDefaultElasticity(w, recs[idx].material, recs[idx].material, 0.5f);
    NewtonMaterialSetDefaultFriction(w, recs[idx].material, recs[idx].material, 1.1f, 0.9f);
    nworlds++;
    return idx;
}

// ---- Agent::start_ragdoll: turn the animated skeleton into a Newton ragdoll -----------------------------------------------------
// inlined helpers (see s004417d0)
// frame whose side axis is `dir`, f/u completed with the cross products (the inline code of s004417d0)
static __forceinline void make_frame(matrix& R, const vector4& dir)
{
    R.s = dir;
    vector4 c1 = cross3(R.s, R.f);
    if ((float)sqrt(c1.x * c1.x + c1.z * c1.z + c1.y * c1.y) < 0.001f) {      // 0x450778: (x^2 + z^2) + y^2
        R.f = cross3(R.u, R.s);
        vnorm(R.f);
        R.u = cross3(R.s, R.f);
        vnorm(R.u);
    } else {
        R.u = c1;
        vnorm(R.u);
        R.f = cross3(R.u, R.s);
        vnorm(R.f);
    }
}

// transform a point by a frame
static __forceinline void tpoint(vector4& r, const matrix& m, const vector4& p)
{
    r.x = p.y * m.f.x + p.z * m.u.x + p.x * m.s.x + m.o.x;
    r.y = m.f.y * p.y + m.u.y * p.z + m.s.y * p.x + m.o.y;
    r.z = m.f.z * p.y + m.u.z * p.z + m.s.z * p.x + m.o.z;
}
// rotate a vector by the 3x3 part of a frame
static __forceinline void rot3v(vector4& r, const matrix& m, float x, float y, float z)
{
    r.x = m.s.x * x + m.u.x * z + m.f.x * y;
    r.y = m.s.y * x + m.u.y * z + m.f.y * y;
    r.z = m.s.z * x + m.u.z * z + m.f.z * y;
}
// the same product in the three summation orders the original uses (float sums are not associative)
// 0x450e65 (skeleton velocity) and 0x450f40 (the bone's own angular velocity): x is (u*z + f*y) + s*x
static __forceinline void rot3v_x(vector4& r, const matrix& m, float x, float y, float z)
{
    r.x = m.u.x * z + m.f.x * y + m.s.x * x;
    r.y = m.s.y * x + m.u.y * z + m.f.y * y;
    r.z = m.s.z * x + m.u.z * z + m.f.z * y;
}
// 0x450bf1 (the cone limit axis): x is (u*z + s*x) + f*y, y and z are (f*y + s*x) + u*z
static __forceinline void rot3v_cone(vector4& r, const matrix& m, float x, float y, float z)
{
    r.x = m.u.x * z + m.s.x * x + m.f.x * y;
    r.y = m.f.y * y + m.s.y * x + m.u.y * z;
    r.z = m.f.z * y + m.s.z * x + m.u.z * z;
}
// 0x450fbb (the angular velocity into the world): every component is (z*u + y*f) + x*s
static __forceinline void rot3v_w(vector4& r, const matrix& m, float x, float y, float z)
{
    r.x = z * m.u.x + y * m.f.x + x * m.s.x;
    r.y = z * m.u.y + y * m.f.y + x * m.s.y;
    r.z = z * m.u.z + y * m.f.z + x * m.s.z;
}

// @ 0x44fc60
void Agent::start_ragdoll()
{
    int idx = (int)g_phys.create_world();
    phys_idx = idx;
    g_phys.recs[idx].ragdoll = NewtonCreateRagDoll(g_phys.recs[idx].world);
    NewtonRagDollBegin(g_phys.recs[idx].ragdoll);

    AgentBody* b = body;
    SkelData* sd = b->skel->data;
    int n = sd->count;
    vector4 wacc[32], vacc[32];
    M4 Wr[32], Ar[32];
    matrix* W = (matrix*)Wr;
    matrix* A = (matrix*)Ar;
    int i;

    // zero the accumulators
    for (i = 0; i < n; i++) {
        wacc[i] = vector4(0.0f, 0.0f, 0.0f);
        vacc[i] = vector4(0.0f, 0.0f, 0.0f);
    }
    vector4 v, w;                                           // velocity / angular velocity handed to Newton

    // sum the ancestors' angular velocities into every bone
    Region* regs = b->skel->regs;
    for (i = 0; i < n; i++) {
        float px, py, pz;
        {
            const vector4& o = regs[i].m84.o;
            px = b->m.f.x * o.y + b->m.u.x * o.z + o.x * b->m.s.x + b->m.o.x;
            py = b->m.f.y * o.y + b->m.u.y * o.z + b->m.s.y * o.x + b->m.o.y;
            pz = b->m.f.z * o.y + b->m.u.z * o.z + b->m.s.z * o.x + b->m.o.z;
        }
        int j = sd->items[i].parent;
        if (j >= 0) {
            Region* regs2 = b->skel->regs;
            ChanRec* items = sd->items;
            ChanRec* jr = &items[j];
            do {
                float qx = b->m.f.x * regs[j].m84.o.y + b->m.u.x * regs[j].m84.o.z + b->m.s.x * regs[j].m84.o.x + b->m.o.x;
                float qy = b->m.f.y * regs[j].m84.o.y + b->m.s.y * regs[j].m84.o.x + b->m.u.y * regs[j].m84.o.z + b->m.o.y;
                float qz = b->m.f.z * regs[j].m84.o.y + b->m.s.z * regs[j].m84.o.x + b->m.u.z * regs[j].m84.o.z + b->m.o.z;
                float rx = px - qx;
                float ry = py - qy;
                float rz = pz - qz;
                float lx = regs2[j].m84.f.x * regs2[j].wy + regs2[j].m84.u.x * regs2[j].wz + regs2[j].m84.s.x * regs2[j].wx;     // 0x4500ac: (f*wy + u*wz) + s*wx
                float ly = regs2[j].m84.f.y * regs2[j].wy + regs2[j].m84.u.y * regs2[j].wz + regs2[j].m84.s.y * regs2[j].wx;
                float lz = regs2[j].m84.f.z * regs2[j].wy + regs2[j].m84.u.z * regs2[j].wz + regs2[j].m84.s.z * regs2[j].wx;
                float vx = lx * b->m.s.x + lz * b->m.u.x + ly * b->m.f.x;
                float vy = lx * b->m.s.y + lz * b->m.u.y + ly * b->m.f.y;
                float vz = lx * b->m.s.z + lz * b->m.u.z + ly * b->m.f.z;
                float cx = vy * rz - vz * ry;
                float cz = vx * ry - rx * vy;
                vacc[i].x = cx + vacc[i].x;
                vacc[i].y = (rx * vz - vx * rz) + vacc[i].y;
                vacc[i].z = cz + vacc[i].z;
                wacc[i].x = vx + wacc[i].x;
                wacc[i].y = vy + wacc[i].y;
                wacc[i].z = vz + wacc[i].z;
                do {
                    j = jr->parent;
                    jr = &items[j];
                } while (jr->caps.size() == 0);
            } while (j >= 0);
        }
    }

    // create the bones
    for (i = 0; i < n; i++) {
        ChanRec* rec = &sd->items[i];
        int p = rec->parent;
        matrix T;
        if (p < 0) {
            W[i] = b->m * b->skel->regs[i].m84;
            T = b->m * rec->m;
            A[i] = T;
        } else {
            W[i] = ((rgl::matrix&)W[p]) * b->skel->regs[i].m44;
            matrix cur = rec->m;
            int j = p;
            for (;;) {
                if (sd->items[j].caps.size() != 0)
                    break;
                cur = sd->items[j].m * cur;
                j = sd->items[j].parent;
            }
            T = cur;
            float a0 = rec->rot_x * 0.017453292f;
            ((rgl::matrix&)T).rotate_about_s(&a0);
            float a1 = rec->rot_y * 0.017453292f;
            ((rgl::matrix&)T).rotate_about_f(&a1);
            float a2 = rec->rot_z * 0.017453292f;
            ((rgl::matrix&)T).rotate_about_u(&a2);
            A[i] = ((rgl::matrix&)A[j]) * T;
        }
        if (rec->caps.size() != 0) {
            float radius = 0.0f, height = 0.01f;
            matrix R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            rgl::mat3 t;
            rgl::mat3* ip = identity3(&t);
            *(rgl::mat3*)&R = *ip;
            R.o = z0;
            for (unsigned k = 0; k < rec->caps.size(); k++) {
                const CapDef& d = rec->caps[k];
                radius = d.r * scale;
                vector4 dir;
                dir.x = d.b.x - d.a.x; dir.y = d.b.y - d.a.y; dir.z = d.b.z - d.a.z;
                float len = vnorm(dir);
                height = len * scale;
                make_frame(R, dir);
                vector4 mid(d.b.x + d.a.x, d.b.y + d.a.y, d.b.z + d.a.z);
                R.o = vector4(mid.x * 0.5f * scale, mid.y * 0.5f * scale, mid.z * 0.5f * scale);
            }
            // physics parent: nearest ancestor that already has a bone
            const NewtonRagDollBone* pbone = 0;
            int q = p;
            if (q >= 0) {
                if (b->skel->regs[q].bone == 0) {
                    do {
                        q = sd->items[q].parent;
                    } while (b->skel->regs[q].bone == 0);
                }
                pbone = b->skel->regs[q].bone;
            }
            vector4 cone;
            rot3v_cone(cone, A[i], rec->cone.x, rec->cone.y, rec->cone.z);
            b->skel->regs[i].bone = g_phys.add_bone(phys_idx, pbone, &rec->mass, T, &radius, &height, R, &cone.x,
                                                     &rec->lim_min, &rec->lim_max, &rec->lim_twist);
            v = v56a4;
            link_vel = vector4(0.0f, 0.0f, 0.0f);
            if ((unsigned)f2c != (unsigned)-1) {
                const Agent* other = &g_mission->agents.pages[(unsigned)f2c >> 4][(unsigned)f2c & 0xf];
                link_vel = other->v56a4;
                v = vector4(0.0f, 0.0f, 0.0f);
            }
            vector4 rv;
            rot3v_x(rv, b->m, b->skel->rvx, b->skel->rvy, b->skel->rvz);
            v.x = rv.x + v.x;
            v.y = rv.y + v.y;
            v.z = v.z + rv.z;
            v.x = v.x + vacc[i].x;
            v.y = v.y + vacc[i].y;
            v.z = v.z + vacc[i].z;
            NewtonBody* nb = NewtonRagDollBoneGetBody(b->skel->regs[i].bone);
            Region* e = &b->skel->regs[i];
            vector4 wl;
            rot3v_x(wl, e->m84, e->wx, e->wy, e->wz);
            rot3v_w(w, b->m, wl.x, wl.y, wl.z);
            w.x = w.x + wacc[i].x;
            w.y = w.y + wacc[i].y;
            w.z = w.z + wacc[i].z;
            NewtonBodySetVelocity(nb, &v.x);
            NewtonBodySetOmega(nb, &w.x);
        }
    }

    NewtonRagDollSetForceAndTorqueCallback(g_phys.recs[phys_idx].ragdoll, apply_force_and_torque);
    NewtonRagDollEnd(g_phys.recs[phys_idx].ragdoll);
    for (i = 0; i < sd->count; i++) {
        if (body->skel->regs[i].bone != 0)
            NewtonBodySetMatrix(NewtonRagDollBoneGetBody(body->skel->regs[i].bone), &W[i].s.x);
    }
    build_static_collision();

    // the mount: a capsule body for the horse
    unsigned mid = mount_id;
    if ((int)mid >= 0 && g_mission->agents.pages[mid >> 4][mid & 0xf].body != 0) {
        const Agent* h = &g_mission->agents.pages[mid >> 4][mid & 0xf];
        vector4 z0(0.0f, 0.0f, 0.0f);
        rgl::mat3 t;
        matrix R;
        rgl::mat3* ip = identity3(&t);
        *(rgl::mat3*)&R = *ip;
        float radius = 0.2114f * h->scale;
        vector4 dir(0.0f, 1.0f, 0.0f);                       // 0x45117c: the original folds the unit axis and the 1.2886 length
        float height = 1.2886f * h->scale;
        make_frame(R, dir);
        R.o = vector4(0.0f * h->scale, 0.5443f * h->scale, -0.0629f * h->scale);
        matrix m0 = h->body->m * h->body->skel->regs->m84;
        float zero = 0.0f;
        mount_body = g_phys.add_capsule_body(phys_idx, &radius, &height, &zero, &m0, &R, false);
    }
    disabled = 1;
    f58d8 = 0;
    f58c4 = 1;
    f58c5 = 1;
    body->skel->has_poses = 1;
}

// ---- blood and dust on the meshes of an agent ------------------------------------------------------------------------------
static inline float dust_clamp01(float v) { return v < 0.0f ? 0.0f : (v >= 1.0f ? 1.0f : v); }

// @ 0x451710
void dust_vertices(mesh* m, const mat4* xf, const float* center, const float* radius, const float* strength,
                   const vector4* tint)
{
    bool dirty = false;
    std::vector<rgl::vertex60>& vs = m->get50_rec();
    std::vector<rgl::point20>& ps = m->get70_rec();
    for (unsigned i = 0; i < vs.size(); i++) {
        rgl::point20* pt = &ps[vs[i].index];
        vector4 w;
        w.x = xf->s.x * pt->b.x + xf->f.x * pt->b.y + xf->u.x * pt->b.z + xf->o.x;
        w.y = xf->s.y * pt->b.x + xf->f.y * pt->b.y + xf->u.y * pt->b.z + xf->o.y;
        w.z = xf->s.z * pt->b.x + xf->f.z * pt->b.y + xf->u.z * pt->b.z + xf->o.z;
        vector4 d;
        d.x = w.x - center[0];
        d.y = w.y - center[1];
        d.z = w.z - center[2];
        float dist = d.length();
        if (dist < *radius && (float)(rand() % 15817) * (1.0f / 15817.0f) < 0.6f) {
            float a = (1.0f - dist / *radius) * 1.5f;
            float e = ((float)(rand() % 15817) * (0.6f / 15817.0f) + 0.4f) * a;
            float s = dust_clamp01(e * *strength);
            if (s > 0.02f) {
                m->ensure_local();
                vector4 c = rgl::unpack_color(&vs[i].color);
                float omt = 1.0f - s;
                vector4 o = a_vadd(a_vscale(omt, c), a_vscale(s, *tint));
                o.w = c.w;
                m->v50[i].color = rgl::pack_color(&o);
                dirty = true;
            }
        }
    }
    if (dirty)
        m->slots[0].f28 = 1;
}
// @ 0x4519f0
void Agent::apply_blood(const float* center, float amount)
{
    float radius;
    float t = amount * 0.06f;
    if (t < 0.0f)
        radius = 0.0f;
    else if (t >= 0.7f)
        radius = 0.7f;
    else
        radius = t;
    float strength;
    float u = amount * 0.07f;
    if (u < 0.0f)
        strength = 0.0f;
    else if (u >= 1.0f)
        strength = 1.0f;
    else
        strength = u;
    vector4 tint;
    tint.x = 0.4f;
    tint.y = 0.01f;
    tint.z = 0.01f;
    tint.w = 1.0f;
    if (body) {
        rgl::lod_mesh* shapes[32];
        int n;
        body->skel->all_shapes((shape_base**)shapes, &n);
        for (int s = 0; s < n; s++) {
            rgl::lod_mesh* L = shapes[s];
            if (L && L->mask && L->ready()) {
                int cur = L->cur;
                for (int k = 0; k < L->count; k++) {
                    std::vector<mesh*>& list = L->lists[k].v;
                    for (unsigned j = 0; j < list.size(); j++) {
                        mesh** slot = &(&*list.begin())[j];
                        if (*slot) {
                            rgl::matrix xf = body->m;
                            if (cur == k) {
                                xf = (&*L->lists[cur].v.begin())[j]->m194;
                            } else {
                                std::vector<mesh*>& cl = L->lists[cur].v;
                                if (cl.size() != 0)
                                    xf = (*cl.begin())->m194;
                            }
                            dust_vertices(*slot, (const mat4*)&xf, center, &radius, &strength, &tint);
                        }
                    }
                }
            }
        }
    }
}
static inline void cross_inl(vector4& r, const vector4& a, const vector4& b)
{
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
}
// @ 0x451c50
void rgl::mesh::compute_normals()
{
    typedef struct {
        void operator=(const std::vector<point20>&); // 0x411630
    } _v90_op;
    struct vec4n : vector4 {
        __forceinline float normalize_inl()
        {
            float len = length();
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                x *= inv;
                y *= inv;
                z *= inv;
            } else {
                x = 0.0f;
                y = 1.0f;
                z = 0.0f;
            }
            return len;
        }
        __forceinline void add_inl(const vector4& o)
        {
            x += o.x;
            y += o.y;
            z = z + o.z;
        }
    };
    if (!local_a0) {
        *(std::vector<point20>*)&v90 = *(const std::vector<point20>*)&parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    std::vector<vector4> fnorm;
    fnorm.resize(v90.size());
    unsigned i;
    for (i = 0; i < v90.size(); i++) {
        vector4 e1 = v70[v50[v90[i].b].index].a - v70[v50[v90[i].a].index].a;
        vector4 e2 = v70[v50[v90[i].c].index].a - v70[v50[v90[i].a].index].a;
        cross_inl(fnorm[i], e1, e2);
        ((vec4n&)fnorm[i]).normalize_inl();
    }
    for (i = 0; i < v50.size(); i++) {
        v50[i].normal = vector4(0.0f, 0.0f, 0.0f);
    }
    for (i = 0; i < v90.size(); i++) {
        for (int j = 0; j < 3; j++) {
            ((vec4n&)v50[(&v90[i].a)[j]].normal).add_inl(fnorm[i]);
        }
    }
    for (i = 0; i < v50.size(); i++) {
        if (v50[i].normal.length_squared() > 0.0001f) {
            ((vec4n&)v50[i].normal).normalize_inl();
        } else {
            v50[i].normal = vector4(0.0f, -1.0f, 0.0f);
        }
    }
    slots[0].f28 = 1;
}
// @ 0x4520b0
void rgl::mesh::append_plain(mesh* other, const mat4* m)
{
    append_skinned(other, m, -1, -1, 0);
}

// ---- Font::layout_text, FxDef::skin_lods, Mission::blood_burst, Agent::refresh_carried ---------------------------------------------
enum {
    TXT_NOWRAP2 = 0x2, TXT_ALIGN_L = 0x4, TXT_ALIGN_R = 0x8, TXT_ALIGN_C = 0x10, TXT_NOSHRINK = 0x100,
    TXT_ALIGN_L2 = 0x200, TXT_START_X = 0x400, TXT_TALL_LINES = 0x800
};

// UTF-8 -> int code points (0x487b00; custom register convention in the original: text in edi, count out in edx)
const int g_max_chars = 8192;                        // 0x59095c
static __declspec(noinline) void decode_utf8(int* out_len, const tstring* s, int* buf, const int* maxn)   // 0x487b00
{
    *out_len = 0;
    int n = s->len;
    int i = 0;
    if (n > 0) {
        do {
            if (*out_len >= *maxn)
                break;
            unsigned char c = s->buf[i];
            int ch;
            if (c >= 0xf0 && c < 0xf5) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((((q[0] & 7) << 6) | (q[1] & 0x3f)) << 6 | (q[2] & 0x3f)) << 6) | (q[3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = ((((q[0] & 0xf) << 6) | (q[1] & 0x3f)) << 6) | (q[2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                const unsigned char* q = (const unsigned char*)s->buf + i;
                ch = (((q[0] & 0x1f) << 6) | (q[1] & 0x3f));
                i += 2;
            } else {
                i++;
                if (c >= 0x80)
                    continue;
                ch = (signed char)c;
            }
            buf[*out_len] = ch;
            (*out_len)++;
        } while (i < n);
    }
}

// @ 0x4520d0
void Font::layout_text(float* out, const tstring* text, const vector4* pos_in, const float* scale_in, void* m_,
                       int arg, const float* xmin, const float* xmax, unsigned flags)
{
    mesh* m = (mesh*)m_;
    int buf[8192];
    int len;
    vector4 pos = *pos_in;
    if (flags & TXT_START_X)
        pos.x = *xmin;
    vec2f scale;
    scale.x = (font_scale / x_ratio) * scale_in[0];
    scale.y = (scale_in[1] / y_ratio) * font_scale;
    float y0 = pos.y;
    decode_utf8(&len, text, buf, &g_max_chars);
    unsigned noshrink = flags & TXT_NOSHRINK;
    if (noshrink == 0 && (flags & TXT_NOWRAP2) == 0) {
        int start = 0;
        float w = measure(buf, len, &start, &len) * scale.x * 1.001f;
        float avail = *xmax - *xmin;
        if (w > avail && w > 0.0f) {
            float f = avail / w;
            scale.x = scale.x * f;
            scale.y = f * scale.y;
        }
    }
    int cur = 0;
    float maxx = *xmin;
    if (len > 0) {
        float inv = 1.0f / scale.x;
        unsigned f4 = flags & TXT_ALIGN_L;
        unsigned f800 = flags & TXT_TALL_LINES;
        do {
            int line_end = cur;
            float linew = 0.0f;
            bool first = true;
            bool brk = false;
            int p = cur;
            do {
                brk = false;
                int s = p;
                if (p < len) {
                    for (; s < len; s++) {
                        int c = buf[s];
                        if (c == 0x20 || c == 0xa || c == 9) {
                            if (s < len) {
                                if (buf[s] == 0xa)
                                    brk = true;
                                s++;
                            }
                            break;
                        }
                    }
                }
                int e = s;
                float w = measure(buf, len, &cur, &e) * scale.x;
                if (first || w <= *xmax - *xmin) {
                    linew = w;
                    line_end = s;
                    p = s;
                } else {
                    brk = true;
                }
                if (p >= len)
                    brk = true;
                first = false;
            } while (!brk);
            if (f4 != 0 || ((flags & TXT_ALIGN_L2) && noshrink == 0)) {
                pos.x = *xmin;
            } else if (flags & TXT_ALIGN_C) {
                pos.x = (*xmin + *xmax) * 0.5f - linew * 0.5f;
            } else if (flags & TXT_ALIGN_R) {
                pos.x = *xmin + *xmax - linew;
            }
            float ux = inv * pos.x;
            for (int k = cur; k < line_end; k++) {
                int ch = buf[k];
                const Glyph* g = font_get(this, ch);
                if (ch != 0xa) {
                    if (ch == 9) {
                        int n = (int)((inv * pos.x - ux) * 4.0f);
                        pos.x = (((float)n + 1.0f) * 0.25f + ux) * scale.x;
                    } else {
                        pos.x = scale.x * (*(float*)&g->f10) + pos.x;
                        draw_glyph(ch, &pos, (const float*)&scale, m, arg);
                        pos.x = scale.x * (*(float*)&g->f18) + pos.x;
                    }
                }
            }
            if (maxx < pos.x)
                maxx = pos.x;
            pos.x = *xmin;
            pos.y = pos.y - (((f800 ? 1.2f : 1.0f) * font_scale * scale_in[1]) * line_spacing) * 0.9f;
            cur = line_end;
        } while (cur < len);
    }
    m->compute_normals();
    if ((flags & TXT_ALIGN_C) && noshrink != 0)
        out[0] = *xmax - *xmin;
    else
        out[0] = (maxx - *xmin) + 0.0001f;
    out[1] = (y0 - pos.y) + 0.001f;
}
// @ 0x452530
void FxDef::skin_lods(rgl::lod_mesh* g, int flag)
{
    for (int k = 0; k < g->count; k++) {
        std::vector<rgl::mesh*>& v = g->lists[k].v;
        for (rgl::mesh** it = v._Myfirst; it < v._Mylast; ++it)
            skin(*it, flag);
    }
}
// @ 0x4525a0
void Mission::blood_burst(float x, float y, float z, int unused, float amount)
{
    if (g_cfg.enable_blood) {
        unsigned i = agents.first_used();
        if (i < (unsigned)agents.count) {
            do {
                Agent* a = &agents.pages[i >> 4][i & 0xf];
                float dx = a->pos.x - x;
                float dy = a->pos.y - y;
                float dz = a->pos.z - z;
                if ((float)sqrt(dx * dx + dy * dy + dz * dz) < 3.5f)
                    a->apply_blood(&x, amount);
                agents.next_used(&i);
            } while (i < (unsigned)agents.count);
        }
    }
}
#define LODS(s) (*(rgl::lod_mesh**)&B[s])        // the display lod meshes of the agent (+0x15c)
// @ 0x452650
void Agent::refresh_carried(Missile* other)
{
    int slot = wield[1];
    if (slot < 0)
        return;
    int idx = other->launcher.item;
    if (idx < 0)
        return;
    if ((char)g_items[idx].type_flags != 10)
        idx = other->ammo.item;
    if (idx < 0)
        return;
    if ((g_items[idx].flags & 0xff000) == 0x10000)
        return;
    mesh* m = ((Wielder*)other)->make_ranged_mesh();
    if (m == 0)
        return;
    if (LODS(slot) == 0) {
        LODS(slot) = (rgl::lod_mesh*)new lod_group;
        mesh* nm = new rgl::mesh;
        nm->res = m->res;
        ((Poser*)LODS(slot))->add_to_list(0, (int)nm);
        ((FxDef*)body->skel)->attach_group(0x13, (PsysGroup*)LODS(slot));      // FxDef is the animator at body+0x304 (fx.h attach_group 0x4424a0)
        update_equipment_display();
    }
    rgl::lod_mesh* L = LODS(slot);
    if (L) {
        if ((char)g_items[other->launcher.item].type_flags == 8)
            m->scale(0.87f);
        mat4 M;
        M = body->m * *(mat4*)body->skel->rec(0xc);
        // the frame block is its own scope (fr then shares the operator* return temporary's slot: frame 0xe0)
        {
            rgl::mat4 fr;
            fr.o = other->p20;
            fr.f = other->p40;
            fr.u = rgl::vector4(0.0f, 0.0f, 1.0f);
            fr.orthonormalize();
            rgl::mat4 rel = M.relative(fr);
            vector4 d;
            d.x = rel.o.x;
            d.z = 0.0f;
            d.y = rel.o.y - 0.23f;
            float n = d.normalize();
            if (n < 0.0f)
                n = 0.0f;
            else if (n > 0.19f)
                n = 0.19f;
            {
                vector4 t = vscale(d, n);
                d.x = t.x;
                d.y = t.y + 0.23f;
                d.z = t.z;
            }
            rel.o = d;
            rel.o.z = 0.05f - (frand() * 0.01f + other->f5c * 0.0005f);
            if (rel.o.x < -0.45f)
                rel.o.x = -0.45f;
            else if (rel.o.x > 0.2f)
                rel.o.x = 0.2f;
            rel.o.x = (frand() - 0.5f) * 0.02f + rel.o.x;
            rel.o.y = (frand() - 0.5f) * 0.02f + rel.o.y;
            L->lists[0].v[0]->append_plain(m, (const mat4*)&rel);
        }
    }
    delete m;
}

// ---- Agent::drop_held, the text layout wrapper, the message log, the animator visibility switch, Agent::get_label_box -----------
extern Font* g_font;                                    // 0x5e2934
// the animator's inline accessor of a bone record's world frame (brings the poses up to date first)
static __forceinline char* skel_rec_inl(Skeleton* sk, int i)
{
    if (!sk->done) {
        if (sk->has_poses)
            ((FxAnim*)sk)->update_poses();
        else
            ((FxAnim*)sk)->update_poses2();
    }
    return sk->recs + i * 0x1dc + 0x84;
}
static __forceinline tstring* lk(const tstring& k) { return g_strtable.lookup(k); }
static __forceinline skin* get_mat(const tstring& n) { return g_resmgr.get_material(n); }

// @ 0x4529e0
void Agent::drop_held()
{
    int i = 0;
    ItemRef* it = pairs;
    node_groups** lp = B;
    for (; i < 4; i++, lp++, it++) {
        if (it->item >= 0) {
            int v = rand() % 15817;
            if ((float)v * (1.0f / 15817.0f) < 0.25f) {
                ItemKind* kind = &g_items[it->item];
                pos2 M;
                identity_pos((::pos*)&M);
                int bone;
                int bflags;
                v = -1;
                if (wield[0] == i || wield[1] == i)
                    bone = kind->ammo_skill();
                else if ((kind->flags & itcf_carry_mask) != 0) {
                    kind->get_carry_pos(&M, &v, &bflags);
                    bone = v;
                } else
                    bone = 0x13;
                Skeleton* sk = body->skel;
                mat4 T1 = body->m * *(mat4*)skel_rec_inl(sk, bone);
                mat4 T2 = T1 * *(mat4*)&M;
                T2.o.z += 0.1f;
                if (lp[-4])
                    lp[-4]->vis = 0;
                if (lp[0])
                    lp[0]->vis = 0;
                if (lp[4])
                    lp[4]->vis = 0;
                v = 6;
                g_mission->add_placed(&v, &it->item, (int*)&it->m4, (rgl::matrix*)&T2, &id);
                it->item = -1;
            }
        }
    }
}
// @ 0x452bb0
rgl::vector4* Font::layout_clear(rgl::vector4* out_extent, const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m,
                               const int* a, const float* b, const float* c, unsigned flags)
{
    ((rgl::mesh*)m)->clear_geometry();
    layout_text((float*)out_extent, text, pos, scale, m, (int)a, b, c, flags);
    return out_extent;
}
// @ 0x452c00
void gui_manager::add(tstring msg, int color)
{
    pos2 M;
    tstring S = msg.strip_tags();
    while (history.size() > 100)
        history.erase(history.begin());
    if (enabled) {
        if (S.len > 0x50) {
            int a = 0, b = 0;
            while (b >= 0 && b < 0x50) {
                a = b;
                char c = ' ';
                b = msg.find(c, b + 1);
            }
            if (a > 0) {
                tstring first = msg.substr(0, a);
                msg = msg.substr(a + 1, msg.len);
                add(first, color);
                add(msg, color);
                return;
            }
        }
        {
        vector4 ext;
        {
            vec2 scale;
            float zero;
            float f08;
            int minus1;
            f08 = 0.8f;
            scale.x = 0.021f;                                  // 0x3cac0831 (0x452d72)
            scale.y = 0.021f;                                  // 0x3cac0831 (0x452d7a)
            cur = (cur + 31) % 32;
            ((rgl::mesh*)&meshA[cur])->clear_geometry();
            ((rgl::mesh*)&meshB[cur])->clear_geometry();
            zero = 0.0f;
            minus1 = -1;
            vector4 p(0.02f, -0.03f, 0.0f);
            g_font->layout_clear(&ext, &S, &p, (const float*)&scale, &meshA[cur], &minus1, &zero, &f08, 0);
            mat4* id = (mat4*)identity_pos((::pos*)&M);
            meshB[cur].append_skinned(&meshA[cur], id, -1, -1, 0);
            this->color[cur] = color;
            age[cur] = 0;
            {
                vector4 hi;                                 // never written here: it shares p's slot
                meshA[cur].calc_bounds(&ext, &hi);       // 0x438d80
                height[cur] = hi.y - ext.y;
            }
            total_h += height[cur];
        }
        {
            tstring n("ui_font");
            meshA[cur].res = get_mat(n);
        }
        {
            tstring n("ui_font_background");
            meshB[cur].res = get_mat(n);
        }
        }
        history.push_back(msg);
    }
}

// @ 0x452f80
void Skeleton::set_visible(char on)
{
    if (!frozen) {
        if (on) {
            if (!done) {
                if (has_poses)
                    ((FxAnim*)this)->update_poses();
                else
                    ((FxAnim*)this)->update_poses2();
            }
            for (unsigned i = 0; i < psys.size(); i++)
                ((FxDef*)this)->skin(psys[i], 1);
            for (unsigned j = 0; j < shapes.size(); j++)
                ((FxDef*)this)->skin_lods((rgl::lod_mesh*)shapes[j], 1);
        }
    }
    frozen = on;
}

// Mission::log_message: the caller loads g_mission into ecx and the body ignores it (ret 8). agent_obj/mission.h is shared, so a stub class names the member here.
struct MissionLog { __declspec(noinline) void log_message(const tstring* s, float seconds); };
// @ 0x453020
void MissionLog::log_message(const tstring* s, float seconds)
{
    g_gui.add(*s, -261);
}

// box above the agent's head (two corner vectors and a scale), for its label
struct LabelBox {
    char pad0[0x20];
    vector4 a;                  // +0x20
    vector4 b;                  // +0x30
    char pad40[0x60 - 0x40];
    float scale;                // +0x60
};

// @ 0x453050
void Agent::get_label_box(void* out_)
{
    LabelBox* out = (LabelBox*)out_;
    if (f10 != 0) {
        out->a = vector4(pos.x, pos.y - 0.3f, pos.z + 0.8f);
        out->b = vector4(pos.x, pos.y + 0.8f, pos.z + 0.85f);
        out->scale = 0.5f;
    } else if (body != 0) {
        vector4 p1 = body->m.transform_point(*(vector4*)(body->skel->recs + 0x648));
        vector4 p2 = body->m.transform_point(*(vector4*)(body->skel->recs + 0xbdc));
        p1.x = p1.x + p2.x;
        p1.y = p1.y + p2.y;
        p2.z = p1.z + p2.z;
        p1.x = p1.x * 0.5f;
        p1.y = p1.y * 0.5f;
        p1.z = p2.z * 0.5f;
        out->a = p1;
        out->b = body->m.transform_point(*(vector4*)(body->skel->recs + 0x1170));
        out->scale = f80 * 0.33f;
    } else {
        g_gui.add(tstring("WARNING:CHECK INVALID ENTITY"), -1);
        float z;
        if (f2c == -1)
            z = pos.z + 0.8f;
        else
            z = pos.z + 1.5f;
        vector4 lo(pos.x, pos.y, z - 0.5f);
        out->a = lo;
        vector4 hi(pos.x, pos.y, z + 0.5f);
        out->b = hi;
        out->scale = 0.4f;
    }
}

// ---- Agent::shield_hit, Agent::on_removed ---------------------------------------------------------------------------------------------
#define itp_type_shield 7
// the stored copy of troop t's slot i: the 0x508-byte troop records are indexed as a flat array of ItemRef
#define STORED(t, i) (g_game->chars->equip[(t) * 0xa1 + (i)])
#define B220(i) (((char*)this)[0x220 + (i)])            // the byte array at +0x220 (b220 of the slices: overlaps pairs[10])

// @ 0x4532f0
void Agent::shield_hit(int damage)
{
    int cur = wield[1];
    if (cur >= 0 && f23c == 0) {
        ItemRef* it = &pairs[cur];
        if (it && (g_items[it->item].type_flags & 0xff) == itp_type_shield) {
            damage -= it->stat_200();
            float f = 0.0f > (float)damage ? 0.0f : (float)damage;
            int skill = g_game->skill_level(troop, 0x1a);
            int d = (int)((1.0f - (float)skill * 0.08f) * (float)(int)f);

            if ((float)d > 0.0f) {
                unsigned v = (it->m4 & 0xffff) - d;
                *(short*)&it->m4 = 0;
                it->m4 |= ((int)v < 0) - 1 & v;
                if ((it->m4 & 0xffff) == 0) {
                    int old = wield[1];
                    wield[1] = -1;
                    on_wield_reset();
                    set_order(1, move_anim(0), 0x1000010, 1.0f);
                    f = 0.0f;
                    int kind = 0x5e;
                    play_sound_var(&kind, &f);
                    mat4 M = body->m * *(mat4*)body->skel->rec(0xc);
                    int mod = it->b.mod;
                    kind = 7;
                    g_mission->add_placed(&kind, &it->item, &mod, (rgl::matrix*)&M, &id);
                    if (((g_mission_templates[g_mission->mode].flags >> 1) & 1) && old >= 0 &&
                        g_mission->player_id == id && old < 10 && B220(old)) {
                        ItemRef* s = &pairs[old];
                        if (s->item >= 0 && (g_items[s->item].type_flags & 0xff) == itp_type_shield) {
                            unsigned bits;
                            int m = s->b.mod;
                            if (m == 1) {
                                if (frand() < 0.2f) {
                                    { tstring key("ui_shield_broken"); g_gui.add(*g_strtable.lookup(key), 0xddbbbb); }
                                    
                                    if (s->item == STORED(g_game->player, old).item && pairs[old].m4 == STORED(g_game->player, old).m4)
                                        STORED(g_game->player, old).item = -1;
                                }
                                return;
                            } else if (m == 5) {
                                if (!(frand() < 0.2f))
                                    return;
                                { tstring key("ui_shield_cracked"); g_gui.add(*g_strtable.lookup(key), 0xddbbbb); }
                                bits = 0x1000000;
                            } else {
                                if (!(frand() < 0.2f))
                                    return;
                                { tstring key("ui_shield_deformed"); g_gui.add(*g_strtable.lookup(key), 0xddbbbb); }
                                bits = 0x5000000;
                            }
                            
                            if (s->item == STORED(g_game->player, old).item && pairs[old].m4 == STORED(g_game->player, old).m4) {
                                ItemRef* r2 = &STORED(troop, old);
                                r2->b.mod = 0;
                                r2->m4 |= bits;
                                r2->reduce(0);
                            }
                            s->b.mod = 0;
                            s->m4 |= bits;
                            s->reduce(0);
                        }
                    }
                }
            }
        }
    }
}
// @ 0x454320
void Agent::on_removed()
{
    g_phys.destroy_world(phys_idx);
    disabled = 0;
    if (f14 != 1) {
        body->skel->set_visible(1);
        if (id != g_mission->player_id)
            drop_held();
    }
    if (!queued) {
        if (id != g_mission->player_id)
            g_mission->queue_dead(this);
        queued = 1;
    }
}

// ---- Agent::hit_damage: the damage of a melee blow or missile hit ------------------------------------------------------------------
// 0x434c50 / 0x434cb0 / 0x434d70 (ItemRef::stat_1ec / stat_21x / stat_1f0a, agent/agent.cpp owns the rows): bodies that only read *this, so
// Agent::hit_damage (0x453710) keeps sp.item in a register across the calls (no `item` local) and sp does not escape.
inline __declspec(noinline) int ItemRef::stat_1ec()      // 0x434c50
{
    if (item < 0) return 0;
    int v = g_items[item].f1ec;
    if (v != 0) {
        int m = b.mod;
        if ((char)g_items[item].type_flags == itp_type_horse) {
            if (m == imod_timid) return v - 1;
            if (m == imod_stubborn) return v + 1;
            if (m == imod_champion) return v + 2;
        } else {
            if (m == imod_heavy) return v + 1;
            if (m == imod_strong) return v + 2;
            if (m == imod_masterwork) v += 4;
        }
    }
    return v;
}
inline __declspec(noinline) int ItemRef::stat_21x(int which)      // 0x434cb0
{
    if (item < 0) return 0;
    unsigned v;
    if (which == 0) v = g_items[item].f218 & 0xff;
    else v = g_items[item].f214 & 0xff;
    if (v != 0) {
        switch (b.mod) {
        case imod_cracked: v -= 5; break;
        case imod_rusty:
        case imod_bent: v -= 3; break;
        case imod_chipped: v -= 1; break;
        case imod_crude: v -= 2; break;
        case imod_fine: v += 1; break;
        case imod_heavy: v += 2; break;
        case imod_balanced:
        case imod_strong: v += 3; break;
        case imod_tempered: v += 4; break;
        case imod_masterwork: v += 5; break;
        }
    }
    return v;
}
inline __declspec(noinline) int ItemRef::stat_1f0a()      // 0x434d70
{
    if (item < 0) return 100;
    int v = g_items[item].f1f0;
    if (v != 0) {
        switch (b.mod) {
        case imod_balanced: v += 3; break;
        case imod_tempered: return v;
        case imod_masterwork: v += 1; break;
        case imod_heavy: v -= 2; break;
        case imod_bent:
        case imod_strong: v -= 3; break;
        }
    }
    return v;
}

static __forceinline vector4 vmul_sv(const float& s, const vector4& v)
{
    vector4 r;
    r.x = s * v.x; r.y = s * v.y; r.z = s * v.z;
    return r;
}
static __forceinline vector4 rot_xy(const rgl::matrix& M, const vector4& p)     // p.z taken as 0
{
    vector4 t;
    t.x = p.x * M.s.x + (0.0f * M.u.x + p.y * M.f.x);
    t.y = 0.0f * M.u.y + p.x * M.s.y + p.y * M.f.y;
    t.z = 0.0f * M.u.z + p.x * M.s.z + p.y * M.f.z;
    return t;
}
static __forceinline vector4 rot_inl(const rgl::matrix& M, const vector4& p)
{
    vector4 t;
    t.x = p.x * M.s.x + p.z * M.u.x + p.y * M.f.x;
    t.y = p.z * M.u.y + p.x * M.s.y + p.y * M.f.y;
    t.z = p.z * M.u.z + p.x * M.s.z + p.y * M.f.z;
    return t;
}
// @ 0x453710
void Agent::hit_damage(int hand, Missile* missile, float* out_dmg, unsigned* out_flag, unsigned victim, float* scale)
{
    ItemRef sp = slot_pair(hand);
    *out_dmg = 0.0f;
    Mission* ms = g_mission;
    vector4 A;
    A.x = 0.0f; A.y = 0.0f; A.z = 0.0f;
    if ((int)victim >= 0) {
        Agent& v = ms->agents[victim];
        if (v.f2c >= 0) {
            A = vmul_sv(ms->agents[v.f2c].vel_xy.y, ms->agents[v.f2c].body->m.f);
        } else {
            vector4 lv;
            lv.x = v.vel_xy.x; lv.y = v.vel_xy.y; lv.z = 0.0f;
            A = ((const ::pos&)v.body->m).rot(lv);
        }
    }
    vector4 B;
    if (f2c >= 0) {
        B = vmul_sv(ms->agents[f2c].vel_xy.y, ms->agents[f2c].body->m.f);
    } else {
        vector4 lv;
        *(vec2*)&lv = vel_xy;                       // x, y as dwords; z is never stored (rot_xy uses 0)
        B = rot_xy(body->m, lv);
    }
    if (missile) {
        *out_flag = 1;
        float spd = missile->f5c;
        if ((int)victim >= 0) {
            vector4 rel = A - missile->p40 * missile->f5c;
            spd = rel.length();        // 0x4539d7: (dz^2 + dy^2) + dx^2
        }
        *out_dmg = ((float)pow((double)(spd / missile->f60), (double)g_module_settings.missile_damage_speed_power)) * missile->f64 * *scale;
    } else {
        int st = hand ? mode2 : mode1;
        unsigned fl = (st == 0);
        *out_flag = fl;
        if (sp.item >= 0)
            *out_dmg = (float)(int)sp.stat_21x(fl) * *scale;
        else
            *out_dmg = (float)g_game->chars[troop].lvl58 * 0.3f + 2.0f;
        vector4 d;
        d = A - B;
        float len = d.normalize();
        if (f100_is_0x48_and_charged()) {
            *out_dmg = (*out_dmg + 5.0f) * (float)pow((double)len, (double)1.8f);
            vector4 q = ((const ::pos&)body->m).rot(*(vector4*)(skel_rec_inl(body->skel, 19) + 0x10));
            q.normalize();
            *out_dmg = ((0.2f - (q.x * d.x + d.y * q.y + d.z * q.z)) * *out_dmg) * g_module_settings.couched_lance_damage_multiplier * 0.2f;
            if (g_options.damage_msgs) {
                if (id == g_mission->player_id)
                    { tstring key("ui_delivered_couched_lance_damage"); g_gui.add(*g_strtable.lookup(key), 0xffd03030); }
                else if (victim == (unsigned)g_mission->player_id)
                    { tstring key("ui_received_couched_lance_damage"); g_gui.add(*g_strtable.lookup(key), 0xffd03030); }
            }
        } else {
            if (mode1 == 3)
                *out_dmg = *out_dmg * 1.05f;
            vector4 q = ((const ::pos&)body->m).rot(*(vector4*)(skel_rec_inl(body->skel, 19) + 0x10));
            q.normalize();
            int m = mode1;
            vector4 dir;
            vector4 up;
            up.x = 0.0f; up.y = 0.0f; up.z = 1.0f;
            if (m == 0) {
                dir = q;
            } else if (m == 2) {
                dir = rgl::cross(q, up);
            } else if (m == 1) {
                dir = vneg(rgl::cross(q, up));
            } else {
                vector4 s = rgl::cross(q, up);
                s.normalize();
                dir = rgl::cross(q, s);
            }
            dir.normalize();
            float t = 1.0f;
            float h = anim[1].scale;
            if (h < 0.33f) {
                float k = h * 3.030303f;
                if (k < 0.0f)
                    k = 0.0f;
                else if (k >= 1.0f)
                    k = 1.0f;
                if (m != 0)
                    k = k * 0.7f + 0.3f;
                t = k;
            } else if (h > 0.61f) {
                float x = (h - 0.65f) * -25.000025f;
                t = 1.0f;
                float lo = 0.0f;
                t = clamp(&x, &lo, &t);
            }
            float w = (float)sp.stat_1f0a() * 0.154f;
            if (m == 0)
                w = w * 0.93f;
            if (t < 1e-06f) {
                *out_dmg = 0.0f;
            } else {
                float c = t < 0.1f ? 0.1f : (t >= 1.2f ? 1.2f : t);
                t = (c * w - (dir.x * d.x + (dir.y * d.y + dir.z * d.z)) * len) / w;
                if (t < 0.3f)
                    t = 0.3f;
                else if (t > 3.0f)
                    t = 3.0f;
                float p = (float)pow((double)t, (double)g_module_settings.melee_damage_speed_power);
                *out_dmg = p * *out_dmg;
                float bonus = (p - 1.0f) * 100.0f;
                int n = round_float(&bonus);
                if (n != 0 && g_options.damage_msgs && id == g_mission->player_id) {
                    char msg[512];
                    if (n > 0) {
                        tstring key("ui_speed_bonus_plus");
                        sprintf(msg, g_strtable.lookup(key)->c_str(), n);
                    } else {
                        tstring key("ui_speed_bonus");
                        sprintf(msg, g_strtable.lookup(key)->c_str(), n);
                    }
                    g_gui.add(tstring(msg), -1);
                }
            }
        }
    }
    if (sp.item >= 0) {
        ItemRef w = slot_pair_fi(1);
        int cls = slot_class_impl(sp.item, w.item >= 0);
        int prof = (int)g_game->chars[troop].f80[cls];
        if ((g_items[sp.item].type_flags & 0xff) == itp_type_crossbow) {
            if (g_mission->fog_dense == 1)
                *out_dmg = *out_dmg * 0.75f;
        } else {
            float pct = (float)prof * 0.01f;
            *out_dmg = (pct * 0.15f + 0.85f) * *out_dmg;
            unsigned t = g_items[sp.item].type_flags & 0xff;
            if (t == itp_type_bow) {
                int sk = g_game->skill_level(troop, 0x21);
                int lim = sp.stat_1ec() + 4;
                if (!(lim < sk)) lim = sk;
                *out_dmg = ((float)lim * 0.14f + 1.0f) * *out_dmg;
                if (f2c != -1) {
                    int hs = g_game->skill_level(troop, 0x17);
                    *out_dmg = ((float)hs * 0.019f + 0.8f) * *out_dmg;
                }
                if (g_mission->fog_dense == 1)
                    *out_dmg = *out_dmg * 0.9f;
            } else if (t == itp_type_thrown) {
                int sk = g_game->skill_level(troop, 0x22);
                *out_dmg = ((float)sk * 0.1f + 1.0f) * *out_dmg;
                if (f2c != -1) {
                    int hs = g_game->skill_level(troop, 0x17);
                    *out_dmg = ((float)hs * 0.019f + 0.8f) * *out_dmg;
                }
            } else if (g_items[sp.item].is_melee()) {
                int sk = g_game->skill_level(troop, 0x23);
                *out_dmg = ((float)sk * 0.08f + 1.0f) * *out_dmg;
            }
            *out_dmg = (float)g_game->chars[troop].lvl58 * 0.2f + *out_dmg;
        }
        unsigned t2 = g_items[sp.item].type_flags & 0xff;
        if (t2 == itp_type_two_handed_wpn || t2 == itp_type_polearm) {
            ItemRef w2 = slot_pair_fi(1);
            if (w2.item >= 0 || f2c >= 0)
                *out_dmg = *out_dmg * (0.75f * 0.9f);
        }
    }
    if (*out_dmg < 0.0f)
        *out_dmg = 0.0f;
    else if (*out_dmg > 500.0f)
        *out_dmg = 500.0f;
}

// ---- Agent::ragdoll_update: per-frame update of the agent's body ----------------------------------------------------------------
// @ 0x4543a0
void Agent::ragdoll_update()
{
    int i = 0;
    if (body == 0)
        return;
    if (disabled != 0) {
        SkelData* data = body->skel->data;
        rgl::matrix world[0x20];
        if (0 < body->skel->nbones) {
            do {
                if (i == 0) {
                    rgl::mat4 g;
                    NewtonRagDollBoneGetGlobalMatrix(body->skel->regs[0].bone, (float*)&g);
                    rgl::mat4 chk = g;
                    if (!(-1e6f <= chk.o.x) || !(chk.o.x < 1e6f)) {
                        on_removed();
                        body->skel->has_poses = 0;
                        break;
                    }
                    (rgl::mat4&)body->m = g;
                    rgl::mat4 id0;
                    { pos2 idp; identity_pos((::pos*)&idp); body->skel->regs[0].m44 = *(rgl::mat4*)&idp; }
                    *(rgl::mat4*)&world[0] = g;
                } else {
                    Region* e = &body->skel->regs[i];
                    rgl::mat4 res;
                    mat4* dst;
                    if (e->bone == 0) {
                        res = *(rgl::mat4*)&world[data->items[i].parent] * *(rgl::mat4*)&e->m44;
                        dst = (rgl::mat4*)&world[i];
                    } else {
                        rgl::mat4 g;
                        NewtonRagDollBoneGetGlobalMatrix(e->bone, (float*)&g);
                        mat4* w = (rgl::mat4*)&world[i];
                        *w = g;
                        if ((f10 == 0 && (i == 8 || i == 10 || i == 15)) || (f10 == 1 && i == 2)) {
                            float lim = 0.15f;
                            if (f10 == 1)
                                lim = 0.4f;
                            else if (i == 8)
                                lim = 0.35f;
                            if (!f58c4 && !f58d8 && hold.o.z - w->o.z < 0.0f &&
                                w->o.z - g_mission->scene->floor_z(&w->o.x) < lim) {
                                g_sound_mgr.play(g_sound_mgr.slot[74], 0, (const float*)&body->m.o, frand() * 0.4f + 0.8f, 0);
                                f58d8 = 1;
                            }
                            hold = *w;
                        }
                        res = *(rgl::mat4*)&((rgl::mat4&)world[data->items[i].parent]).relative((const rgl::mat4&)world[i]);
                        dst = (rgl::mat4*)&body->skel->regs[i].m44;
                    }
                    *dst = res;
                }
                i++;
            } while (i < body->skel->nbones);
        }
        float dx = body->m.o.x - anchor.x;
        float dy = body->m.o.y - anchor.y;
        float dz = body->m.o.z - anchor.z;
        if (9.0f < dx * dx + (dy * dy + dz * dz))
            build_static_collision();
        if ((int)mount_id >= 0) {
            unsigned mid = mount_id;
            if (g_mission->agents.pages[mid >> 4][mid & 0xf].body != 0) {
                rgl::mat4 m1, m2, mm;
                NewtonBodyGetMatrix(mount_body, (float*)&m1);
                NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(body->skel->regs[0].bone), (float*)&m2);
                AgentBody* mb = g_mission->agents.pages[mount_id >> 4][mount_id & 0xf].body;
                mm = (rgl::mat4&)mb->m * *(rgl::mat4*)&mb->skel->regs[0].m84;
                NewtonBodySetMatrix(mount_body, (float*)&mm);
                if (0.0f < m2.o.z - m1.o.z) {
                    int j = 0;
                    if (0 < body->skel->nbones) {
                        int off = 0;
                        do {
                            NewtonRagDollBone* b = *(NewtonRagDollBone**)((char*)body->skel->regs + 0x1d8 + off);
                            if (b != 0) {
                                NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                                NewtonBodyGetMatrix(nb, (float*)&m2);
                                float z = mm.o.z + m2.o.z;
                                float x = (mm.o.x + m2.o.x) - m1.o.x;
                                float y = (mm.o.y + m2.o.y) - m1.o.y;
                                float zz = z - m1.o.z;
                                *(vector4*)&m2.o = vector4(x, y, zz);
                                NewtonBodySetMatrix(nb, (float*)&m2);
                            }
                            off += 0x1dc;
                            j++;
                        } while (j < body->skel->nbones);
                    }
                    f58c4 = 0;
                } else if (f58c5 != 0) {
                    f58c5 = 0;
                    float* pv = &g_mission->agents.pages[mount_id >> 4][mount_id & 0xf].v56a4.x;
                    int j = 0;
                    if (0 < body->skel->nbones) {
                        int off = 0;
                        do {
                            NewtonRagDollBone* b = *(NewtonRagDollBone**)((char*)body->skel->regs + 0x1d8 + off);
                            if (b != 0) {
                                NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                                float v[4];
                                NewtonBodyGetVelocity(nb, v);
                                v[0] = v[0] + pv[0];
                                v[1] = v[1] + pv[1];
                                NewtonBodySetVelocity(nb, v);
                            }
                            off += 0x1dc;
                            j++;
                        } while (j < body->skel->nbones);
                    }
                    f58c4 = 0;
                } else
                    f58c4 = 0;
            } else if ((int)mount_id >= 0) {
                int j = 0;
                if (0 < body->skel->nbones) {
                    int off = 0;
                    do {
                        NewtonRagDollBone* b = *(NewtonRagDollBone**)((char*)body->skel->regs + 0x1d8 + off);
                        if (b != 0) {
                            NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                            float v[4];
                            NewtonBodyGetVelocity(nb, v);
                            v[0] = v[0] + link_vel.x;
                            v[1] = v[1] + link_vel.y;
                            NewtonBodySetVelocity(nb, v);
                        }
                        off += 0x1dc;
                        j++;
                    } while (j < body->skel->nbones);
                }
                f58c4 = 0;
            } else
                f58c4 = 0;
        } else
            f58c4 = 0;
    } else if (!queued) {
        matrix* bm = (matrix*)&body->m;
        bm->f = vel;
        bm->f.z = 0.0f;
        bm->u = vector4(0.0f, 0.0f, 1.0f);
        bm->orthonormalize();
        if (f14 != 1) {
            f84 = g_step_tbl[2] + f84;
            if (f84 < 0.0f)
                f84 = 0.0f;
            else if (1.0f < f84)
                f84 = 1.0f;
            float* d = &((face*)fbc)->normal.x;
            float k = 1.0f - f84;
            float a = d[0] * f84;
            float b = d[1] * f84;
            float c = d[2] * f84;
            float zk = k * 0.0f;
            bm->u = vector4(zk + a, zk + b, k + c);
            bm->u.normalize();
            bm->s = rgl::cross(bm->f, bm->u);
            bm->s.normalize();
            bm->f = rgl::cross(bm->u, bm->s);
        }
        ((rgl::matrix*)bm)->rotate_about_f(rot);
        bm->o = vector4(v98.x + pos.x, v98.y + pos.y, v98.z + pos.z);
    }
    camera_params(((AgentBodyV*)body)->camp);
    if (f2c < 0) {
        if (f5830 == 3 || f5830 == 4) {
            ((AgentBodyV*)body)->set_mode(0);
        } else {
            ((AgentBodyV*)body)->set_mode(2);
            if (f30 == -1)
                ((AgentBodyV*)body)->f298 = -1;
        }
    } else {
        ((AgentBodyV*)body)->set_mode(0);
        AgentBody* mb = g_mission->agents.pages[(unsigned)f2c >> 4][(unsigned)f2c & 0xf].body;
        if (mb != 0)
            ((AgentBodyV*)mb)->f298 = ((AgentBodyV*)body)->f8;
    }
    fxp->visible = 0;
    if (g_options.fx_on != 0)
        fxp->visible = 1;
}

#define itcf_carry_slot1 0x100000000i64
#define itcf_carry_slot2 0x200000000i64
#define itcf_carry_quiver_back_right 0x60000000i64
#define itcf_carry_quiver_back 0x80000000i64
// get_carry_pos (0x433e40, src/game/agent/item_kind.cpp) and the two shape constructors (0x4332a0 / 0x433310,
// src/rgl/xmlcfg/agent_pick.cpp) with their bodies visible here (inline noinline duplicates): reduce_blow's carry frame,
// bone / aux, lo and hi then do not escape, so the packer shares their slots with the later locals as the original does
inline __declspec(noinline) void ItemKind::get_carry_pos(pos2* out, int* bone_flags, int* bone)
{
    vector4 v;
    float a;
    *bone = 0;
    pos2 ident;
    *out = *(pos2*)identity_pos((pos*)&ident);
    __int64 carry = flags & itcf_carry_mask;
    *bone_flags = 0;
    if (carry == itcf_carry_sword_left_hip) {
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.24f; v.y = 0.1f; v.z = 0.1f;
        out->o = v;
        a = 1.5393804f; out->rotate_su(&a);
        a = -2.2f; out->rotate_sf(&a);
        a = 0.12f; out->rotate_fu(&a);
        a = -0.2f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_axe_left_hip) {
        vector4& o = out->o;
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.21f; v.y = 0.12f; v.z = 0.14f;
        o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = 1.0f; out->rotate_sf(&a);
        a = 3.3415928f; out->rotate_su(&a);
        float s = (float)weight * 0.01f - 0.3f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == itcf_carry_mace_left_hip) {
        vector4& o = out->o;
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.21f; v.y = 0.12f; v.z = 0.14f;
        o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = 1.0f; out->rotate_sf(&a);
        a = 3.3415928f; out->rotate_su(&a);
        float s = (float)weight * 0.01f - 0.1f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == itcf_carry_dagger_front_left) {
        *bone = 7;
        *bone_flags = 0;
        v.x = 0.04f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = 0.15707964f; out->rotate_su(&a);
        a = -2.5f; out->rotate_sf(&a);
        a = -0.05f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_dagger_front_right) {
        *bone = 8;
        *bone_flags = 0;
        v.x = -0.04f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = -3.2986722f; out->rotate_su(&a);
        a = -2.5f; out->rotate_sf(&a);
        a = 0.05f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_slot1) {
        vector4& o = out->o;
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        o = v;
        a = 0.5654867f; out->rotate_sf(&a);
        a = 3.1415927f; out->rotate_su(&a);
        float s = (float)weight * 0.01f - 0.3f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == (itcf_carry_sword_left_hip | itcf_carry_slot1)) {
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        out->o = v;
        a = -2.576106f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_quiver_right_vertical | itcf_carry_slot1)) {
        vector4& o = out->o;
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        o = v;
        a = 0.5654867f; out->rotate_sf(&a);
        vector4 t(out->f.x * 0.67f, out->f.y * 0.67f, out->f.z * 0.67f);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == (itcf_carry_axe_left_hip | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.04f; v.z = -0.18f;
        out->o = v;
        a = 3.2986722f; out->rotate_su(&a);
        a = 1.0995574f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_quiver_back | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.06f; v.y = 0.18f; v.z = -0.2f;
        out->o = v;
        a = -3.2986722f; out->rotate_su(&a);
        a = 1.5707964f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_dagger_front_left | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.22f; v.y = -0.06f; v.z = -0.2f;
        out->o = v;
        a = 3.138451f; out->rotate_su(&a);
        a = 1.0995574f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_quiver_front_right | itcf_carry_slot1)) {
        vector4& o = out->o;
        *bone = 6;
        *bone_flags = 8;
        v.x = 0.19f; v.y = 0.34f; v.z = -0.155f;
        o = v;
        a = -0.5654867f; out->rotate_sf(&a);
        a = 1.4137167f; out->rotate_su(&a);
        a = 0.1f; out->rotate_sf(&a);
        float s = (float)weight * 0.01f + 0.5f;
        vector4 t(s * out->f.x, s * out->f.y, s * out->f.z);
        o.x = o.x - t.x;
        o.y = o.y - t.y;
        o.z = o.z - t.z;
    }
    else if (carry == (itcf_carry_quiver_back_right | itcf_carry_slot1)) {
        *bone = 6;
        *bone_flags = 8;
        v.x = -0.05f; v.y = 0.0f; v.z = -0.19f;
        out->o = v;
        a = -0.5654867f; out->rotate_sf(&a);
        a = -1.5707964f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_quiver_front_right) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.21f; v.y = 0.15f; v.z = 0.0f;
        out->o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = -2.5f; out->rotate_sf(&a);
        a = -0.2f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_quiver_back_right) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.05f; v.y = 0.03f; v.z = -0.19f;
        out->o = v;
        a = 0.15707964f; out->rotate_su(&a);
        a = 1.1f; out->rotate_sf(&a);
        a = 3.1415927f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_quiver_right_vertical) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.23f; v.y = 0.05f; v.z = 0.0f;
        out->o = v;
        a = 1.4451327f; out->rotate_su(&a);
        a = 0.5f; out->rotate_sf(&a);
        a = -0.05f; out->rotate_su(&a);
        a = 0.2f; out->rotate_fu(&a);
    }
    else if (carry == itcf_carry_quiver_back) {
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.05f; v.y = 0.02f; v.z = -0.17f;
        out->o = v;
        a = 0.37699112f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_dagger_front_right | itcf_carry_slot1)) {
        *bone = 3;
        *bone_flags = 0;
        v.x = 0.18f; v.y = -0.1f; v.z = -0.1f;
        out->o = v;
        a = 2.1991148f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_revolver_right) {
        *bone = 9;
        *bone_flags = 4;
        v.x = -0.1f; v.y = 0.0f; v.z = 0.05f;
        out->o = v;
        a = -1.5707964f; out->rotate_su(&a);
        a = 0.9424779f; out->rotate_sf(&a);
        a = -0.1f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_pistol_front_left) {
        *bone = 7;
        *bone_flags = 0;
        v.x = -0.07f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = 0.15707964f; out->rotate_su(&a);
        a = -1.5f; out->rotate_sf(&a);
        a = -0.05f; out->rotate_su(&a);
    }
    else if (carry == itcf_carry_bowcase_left) {
        *bone = 10;
        *bone_flags = 0;
        v.x = 0.26f; v.y = 0.03f; v.z = -0.1f;
        out->o = v;
        a = -4.3982296f; out->rotate_su(&a);
        a = -0.25f; out->rotate_fu(&a);
        a = -0.84823006f; out->rotate_sf(&a);
    }
    else if (carry == (itcf_carry_sword_left_hip | itcf_carry_slot2)) {
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.23f; v.y = 0.15f; v.z = 0.23f;
        out->o = v;
        a = -1.4451327f; out->rotate_su(&a);
        a = 1.5f; out->rotate_sf(&a);
        a = -0.2f; out->rotate_su(&a);
    }
    else if (carry == (itcf_carry_axe_left_hip | itcf_carry_slot2)) {
        *bone = 2;
        *bone_flags = 0;
        v.x = 0.05f; v.y = 0.19f; v.z = 0.24f;
        out->o = v;
        a = -2.1991148f; out->rotate_su(&a);
        a = 1.8f; out->rotate_sf(&a);
        a = -0.2f; out->rotate_su(&a);
    }
}
inline __declspec(noinline) sphere::sphere(vector4 c, const float* r) : shape_base(inline_init())
{
    local_center = c;
    center = c;
    radius = *r;
}
inline __declspec(noinline) capsule::capsule(float r, const vector4* a, const vector4* b) : shape_base(inline_init())
{
    radius = r;
    l0 = *a;
    l1 = *b;
    p0 = l0;
    p1 = l1;
}
// ---- Agent::reduce_blow, draw_name, spawn_corpse --------------------------------------------------------------------------------------
// @ 0x454e30
void Agent::reduce_blow(Blow* b)
{
    int bone, aux;
    float dmg = b->dmg_in;
    int idx;
    if (f10 == 1)
        idx = 0;
    else {
        int t = b->bone;
        if (t == 9)
            idx = 0;
        else if (t == 1 || t == 4 || t == 2 || t == 5 || t == 3 || t == 6)
            idx = 2;
        else
            idx = 1;
    }
    float armor_v = (float)armor[idx];
    if (f10 == 0) {
        ItemRef w = best_weapon_slot();
        if (w.item >= 0 && b->hit) {
            ItemKind* ik = &g_items[w.item];
            pos2 carry;
            ik->get_carry_pos(&carry, &bone, &aux);
            rgl::mat4 r = *(rgl::mat4*)body->skel->rec(bone);
            rgl::mat4 u = (rgl::mat4&)body->m * (r * *(rgl::mat4*)&carry);
            HitInfo* h = b->hit;
            if (u.u.y * h->dy + u.u.z * h->dz + u.u.x * h->dx < -0.5f) {
                vector4 hi = *(vector4*)&h->dx * 4.0f + *(vector4*)&h->px;
                vector4 lo = *(vector4*)&h->px - *(vector4*)&h->dx * 4.0f;
                capsule cap(0.0f, &lo, &hi);
                float zero = 0.0f;
                sphere sph(*(vector4*)&u.o, &zero);
                float gap = capsule_sphere_gap(&cap, &sph);
                if (gap < (float)ik->weight * 0.01f) {
                    int bonus = w.stat_200() + 10;
                    armor_v = (float)bonus + armor_v;
                    if (g_mission->player_id == id) {
                        tstring key("ui_hit_shield_on_back");
                        g_gui.add(*g_strtable.lookup(key), -1);
                    }
                }
            }
        }
    }
    float x, y;
    if (b->kind == 0) {
        x = g_module_settings.armor_soak_factor[0] * armor_v;
        armor_v = g_module_settings.armor_reduction_factor[0] * armor_v;
    } else if (b->kind == 1) {
        x = g_module_settings.armor_soak_factor[1] * armor_v;
        armor_v = g_module_settings.armor_reduction_factor[1] * armor_v;
    } else {
        x = g_module_settings.armor_soak_factor[2] * armor_v;
        armor_v = g_module_settings.armor_reduction_factor[2] * armor_v;
    }
    y = ((float)(rand() % 0x3dc9) * 3.4772715e-05f + 0.45f) * x;
    float d = dmg - y;
    if (0.0f > d)
        x = 0.0f;
    else
        x = d;
    int iv = rand() % 0x3dc9;
    float part = (1.0f - 1.0f / (float)exp((double)(((float)iv * 3.4772715e-05f + 0.45f) * (armor_v * 0.014f)))) * x;
    if (armor_v < 0.0001f)
        part = 0.0f;
    y = part + y;
    double fl = ::floor((double)y);
    int n = (int)fl;
    b->absorbed = (y - fl <= 0.5f) ? n : n + 1;
    x = ((float)(rand() % 0x3dc9) * 2.5289246e-05f + 0.6f) * dmg - b->absorbed;
    b->dmg_out = x;
    if (f10 != 1) {
        if (b->bone == 9)
            b->dmg_out = x * 1.2f;
        else if (b->bone == 1 || b->bone == 4)
            b->dmg_out = x * 0.9f;
    }
    if (b->dmg_out < 0.0f)
        b->dmg_out = 0.0f;
}
// @ 0x4552c0
void Agent::draw_name()
{
    Mission* ms = g_mission;
    unsigned local = ms->player_id;
    unsigned col = 0xffffffff;
    float origin[3];
    origin[0] = 0.0f; origin[1] = 0.0f; origin[2] = 0.0f;
    vec2 scale;
    scale.x = 0.14f; scale.y = 0.14f;
    float a = -1.0f;
    float bb = 1.0f;
    if (local < 0x80000000u) {
        if (ms->agents.pages[local >> 4][local & 0xf].team == team)
            col = 0xff44ff44;
        else if ((int)local >= 0 && ms->agents.pages[local >> 4][local & 0xf].is_friend(this))
            col = 0xff4444ff;
    }
    if (f10 == 0 && id != (int)local) {
        unsigned fl = g_mission_templates[ms->mode].flags;
        unsigned f1 = (fl >> 1) % 2;
        if (f1 && (int)local >= 0 && ms->agents.pages[local >> 4][local & 0xf].is_friend(this)) {
            char buf[512];
            tstring* nm = g_game->chars[troop].display_name();
            sprintf(buf, "%s", nm->c_str());
            if (f14 != 1) {
                fxp->scale = 0.3f;
                if (f5830 == 3)
                    buf[0] = 0;
            } else if (f2c >= 0)
                fxp->scale = 2.65f;
            else
                fxp->scale = 1.88f;
            unsigned colp = col;
            vector4 ext;
            tstring text(buf);
            g_font->layout_clear(&ext, &text, (vector4*)origin, (const float*)&scale, *fxp->pmesh, (const int*)&colp, &a, &bb, 0x10);
        }
    }
}
// @ 0x4554c0
void Agent::spawn_corpse()
{
    fbc = 0;
    vector4 saved = pos;
    f56e8 = 0;
    int i = 0;
    {
        float z;
        face** pf = (face**)&fbc;
        do {
            if (i >= 100)
                break;
            i++;
            if (i == 30 || i == 60)
                pos = saved;
            g_mission->scene->floor_at(&pos.x, &z, pf);
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
    (&f5340)[0] = fv + fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&f5340)[1] = fv + fv;
    r = rand() % 0x3dc9;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&f5340)[2] = fv + fv;
    r = rand() % 0x3dc9;
    f290 = 5.0f;
    spawn = pos;
    fv = (float)r * 6.322312e-05f - 0.5f;
    (&f5340)[3] = fv + fv;
    ragdoll_update();
    stand_animation = 1;
    if ((g_game->chars[troop].flags32 & 0xf) == 1)
        stand_animation = 0;
    walk_forward_animation = 9;
}

// ---- Agent::tick_death, Agent::note_blow -------------------------------------------------------------------------------------------
#define TICKS_SINCE(t) ((float)(g_ticks[(t).clock] - (t).start) * 1e-5f)
// @ 0x457710
void Agent::tick_death()
{
    if (f5844 == 1 && f30 >= 0 && f10 == 1 && TICKS_SINCE(tm) > 3.0f) {
        g_mission->agents.pages[(unsigned)f30 >> 4][(unsigned)f30 & 0xf].f2c = -1;
        f30 = -1;
        f5844 = 2;
    }
    if (f5830 == 3) {
        if (f5844 != 0) {
            if (!(TICKS_SINCE(tm) > 3.0f))
                return;
            tm.start = g_ticks[tm.clock];
            f5830 = 4;
            return;
        }
        if (disabled == 0 && queued == 0 && tm.elapsed() > 3.0f) {
            if (f14 != 1) {
                body->skel->set_visible(1);
                if (g_mission->player_id != id)
                    drop_held();
            }
            if (id != g_mission->player_id)
                g_mission->queue_dead(this);
            queued = 1;
        }
        if (g_options.i40 > g_phys.nworlds && TICKS_SINCE(tm) > 0.0f && t5918.elapsed() > 0.0f) {
            if (disabled == 0 && queued == 0)
                start_ragdoll();
        }
        if (disabled == 0)
            return;
        if ((f58d8 == 0 || !(t5918.elapsed() > 10.0f)) && !(t5918.elapsed() > 30.0f) &&
            (!(t5918.elapsed() > 1.0f) || !g_phys.none_sleeping(phys_idx)))
            return;
        on_removed();
    } else if (f5830 == 4) {
        if (queued == 0 && f14 != 1)
            return;
        float k = 1.0f - TICKS_SINCE(tm) * 0.33333334f;
        if (k < 0.0f)
            k = 0.0f;
        else if (k > 1.0f)
            k = 1.0f;
        float c[4];
        c[0] = 1.0f; c[1] = 1.0f; c[2] = 1.0f; c[3] = k;
        ((AgentBodyV*)body)->set_color(rgl::pack_color((const vector4*)c));
        ((AgentBodyV*)body)->f294 = k;
        if (TICKS_SINCE(tm) > 3.0f) {
            f5830 = 5;
            release_body();
            if (f14 == 1)
                f14 = 2;
        }
    }
}
// @ 0x457af0
void Agent::note_blow(Blow* b)
{
    reduce_blow(b);
    if (has_blow == 0 || !(b->dmg_out < last_blow().dmg_out)) {
        last_blow() = *b;
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
                ((Curve*)a->body->skel)->set_rec_phase(slot, 0.0f);
            if (slot == 0) {
                if (id >= 0 && (a->defs->defs[id].flags & 0x400) && a->anim[1].cur != -1 &&
                    (prio & 0x1ffff) > (a->anim[1].events & 0x1ffff))
                    aset_inl(a->anim[1], -1, prio & 0x1ffff);
            } else if (slot == 1) {
                int k = a->wield[0];
                if (k >= 0 && a->pairs[k].item >= 0) {
                    if (prio & 0x10000000)
                        a->links[k]->vis = 0;
                    else
                        a->links[k]->vis = 0xffff;
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
    f56ec = (1.0f - dt * 10.0f) * f56ec;
    f56e8 = (1.0f - dt * 5.0f) * f56e8;
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
            if (f10 == 0) {
                if (pos.z < ms->terrain.base_z) {
                    float lo = 0.4f;
                    float v = 1.0f - (ms->terrain.base_z - pos.z) * 0.5f;
                    speed_mul = fmax_ref(&lo, &v);
                } else if (ms->fog_dense == 1) {
                    speed_mul = 0.9f;
                }
            } else {
                if (pos.z < ms->terrain.base_z) {
                    float lo = 0.3f;
                    float v = 1.0f - (ms->terrain.base_z - pos.z) * 0.6f;
                    speed_mul = fmax_ref(&lo, &v);
                } else {
                    if (ms->fog_dense == 1)
                        speed_mul = 0.9f;
                    if (g_game->is_night())
                        speed_mul = speed_mul * 0.9f;
                }
            }
            vector4 up(0.0f, 0.0f, 1.0f);
            if (fbc)
                up = fbc->d;

            // ---- input bits -> the two input axes ------------------------------------------------------------------
            unsigned fl = in_flags;
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
            if (in_flags & 4) {
                side = 1.0f;
                g_move_fwd = g_move_fwd * 0.7f;
                if (in_flags & 0x200000)
                    g_move_side = 0.9f;
                else
                    g_move_side = 0.55f;
            } else if (in_flags & 8) {
                side = -1.0f;
                g_move_fwd = g_move_fwd * 0.7f;
                if (in_flags & 0x200000)
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
                    if (!(in_flags & 0x200000)) {
                        ms = ms * 0.7f;
                        mf = mf * 0.7f;
                    }
                } else if (!(in_flags & 0x200000)) {
                    ms = ms * 0.6f;
                    mf = mf * 0.6f;
                }
                g_move_fwd = mf;
                g_move_side = ms;
            }

            int mode = 0;
            if (f10 == 0) {
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
                if ((anim[0].events & 0x1ffff) == 0 && f2c == -1) {
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
                        if (!(f2c4 & 1) && (id != (unsigned)g_mission->player_id ||
                                (!a_bit(g_mission_templates[g_mission->mode].flags, 1) && !(g_mission->terrain.f158 & 1))))
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
                    v = ((const ::pos&)body->m).rot(v);
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
                const int& agi = g_game->chars[off].f58[1];
                float skill = ((60.0f / (f24c + 60.0f)) * (g_game->chars[off].nibble(0x19) * 5.0f + agi) + 100.0f) * 0.01f;
                float nvx, nvy;
                {
                    a_v2 dv = mv * mag * 2.8f * skill;
                    dv.x = dv.x * 0.95f;
                    if (dv.y < 0.0f)
                        dv.y = dv.y * 0.8f;
                    dv.x = dv.x - vel_xy.x;
                    dv.y = dv.y - vel_xy.y;
                    const int& agi2 = g_game->chars[off].f58[1];
                    float step = (((70.0f / (f24c + 70.0f)) * ((25.0f / (f24c + 25.0f)) * (((float)g_game->chars[off].nibble(0x19) * 0.2f) + ((float)agi2 * 0.06666667f)) + 1.0f)) * dt) * 10.0f;
                    float lo_ = -step;
                    nvx = a_clampr(dv.x, lo_, step) + vel_xy.x;
                    vel_xy.x = nvx;
                    nvy = a_clampr(dv.y, lo_, step) + vel_xy.y;
                    vel_xy.y = nvy;
                }
                unsigned band = fb8 & 7;
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
                fb4 = spd / divisor;
                if (fb4 > 0.001f)
                    anim[0].f1c = 1.0f / fb4;
                else
                    anim[0].f1c = 1.0f;
                fb8 = (fb8 & 0xfffffff8) | band;
            }
            // ---- mount: rider skill, acceleration, gait band ------------------------------------------------------
            int rid = f30;
            int rskill = 5;
            float rmul = 1.0f;
            if (rid != -1) {
                Agent* pg = g_mission->agents.pages[(unsigned)rid >> 4];
                int rt = pg[rid & 0xf].troop;
                rskill = g_game->chars[rt].nibble(0x18);
                rmul = ((float)rskill + 25.0f) * 0.037037037f;
            }
            float sgn = 0.0f;
            if (f10 == 1) {
                unsigned fl2 = in_flags;
                if (fl2 & 0x40) {
                    sgn = -1.0f;
                } else if (fl2 & 0x80) {
                    sgn = 1.0f;
                }
                if (rid >= 0) {
                    Mission* m7 = g_mission;
                    if (m7->agents.get(rid)->f100_is_0x48_and_charged()) {
                        if ((int)id == m7->player_id)       // both arms the same: only the two loads are left (0x456503)
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
                int s1 = f8c.stat_1f0b() + 1;
                float acc = ((((float)s1 / lim) * sgn) * rmul) * dt + f56e8;
                f56e8 = acc;
                rot[1] = rot[1] - ((acc * vel_xy.y) * dt) * 0.007f;
                ((vec2x*)&vel)->rotate_b(acc * dt);
                if (in_flags & 0x10) {
                    if (f10 == 1) {
                        unsigned bb = fb8;
                        if ((bb & 0x300) == 0) {
                            fb8 = bb | 0x100;
                            int s2 = f8c.stat_1f0b() - 1;
                            vel_xy.y = ((float)s2 * rmul) * 0.04f;
                        } else if (bb & 0x100) {
                            vel_xy.y = (((18.0f - vel_xy.y) * ((float)f8c.stat_1f0b() * ((float)rskill + 3.0f))) * dt) * 0.0006f + vel_xy.y;
                        } else if (bb & 0x200) {
                            float nf = dt + vel_xy.y;
                            vel_xy.y = nf;
                            if (nf > -0.9f)
                                vel_xy.y = 0.0f;
                        }
                    }
                } else if (in_flags & 0x20) {
                    if (f10 != 0)
                        order1(this, 0x1a1, 0);
                    unsigned bb = fb8;
                    if ((bb & 0x300) == 0) {
                        bb = bb | 0x200;
                        fb8 = bb;
                        vel_xy.y = -0.9f;
                    } else if (bb & 0x200) {
                        vel_xy.y = vel_xy.y - dt * 0.5f;
                    } else if (bb & 0x100) {
                        int s2 = f8c.stat_1f0b();
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
                        fb8 = fb8 & 0xfffffcff;
                }
                vel.normalize();
                int s3 = f8c.stat_1f4() + 1;
                vector4* fr = &body->m.f;
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
                unsigned band = fb8 & 7;
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
                    if (f30 == g_mission->player_id) {
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
                fb4 = r;
                if (r != 0.0f)
                    anim[0].f1c = 1.0f / r;
                else
                    anim[0].f1c = 1.0f;
                fb8 = (fb8 & 0xfffffff8) | band;
                if (f56e8 < -0.1f) {
                    if (band == 1)
                        set_order(0, 0x19f, 0, 1.0f);
                    order1(this, 0x19f, 0);
                } else if (f56e8 > 0.1f) {
                    if (band == 1)
                        set_order(0, 0x1a0, 0, 1.0f);
                    order1(this, 0x1a0, 0);
                }
            }
            // ---- jump / rear-up ------------------------------------------------------------------------------------
            if ((in_flags & 0x100000) && t5800.elapsed() >= 0.0f) {
                if (f10 == 0) {
                    if (f2c == -1) {
                        flags3c = flags3c & 0xfffffffe;
                        a_vadd(impulse, vector4(0.0f, 0.0f, 5.0f));
                        set_order(0, 2, 0x1000002, 1.0f);
                        if (pos.z < g_mission->terrain.base_z) {
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
                    if (pos.z < g_mission->terrain.base_z) {
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
            if ((in_flags & 0x40000) && f10 != 0) {
                int kind = 4;
                play_hit_sound(&kind);
                set_order(0, 0x19c, 0x100004a, 1.0f);
            }
        }
    }
    // ---- heading ------------------------------------------------------------------------------------------------
    if ((int)id == g_mission->player_id && f2c == -1) {
        if (f55d4 != 0) {
            vel = (*(vector4*)&fx);
        } else {
            if (active != 0 || f5610 != 0 || (in_flags & 0xffdfffff) != 0)
                steer(dt);
        }
    } else if (f10 != 1 && f2c == -1) {
        steer(dt);
    }
    // ---- riding: mount reactions, mount / dismount --------------------------------------------------------------
    unsigned mid = f2c;
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
        if (f130 & 8) {
            if (agent_by_id(f2c)->vel_xy.y > 5.5f) {
                order1(this, 0x48, 0);
            } else if (agent_by_id(f2c)->anim[0].cur == 0x196) {
                order1(this, 0x51, 0);
            } else {
                order1(this, 0x43, 0);
            }
        }
    }
    if ((in_flags & 0x80000) && (fb8 & 0x100) && f10 == 0 && f2c >= 0 &&
        g_mission->agents.get(f2c)->vel_xy.y < 5.2f)
        set_order(0, 0x4a, 0x1000002, 1.0f);
    unsigned fl = in_flags;
    if ((fl & 0x10000) && f2c >= 0 && f57d8 == 0) {
        ((ActiveTimer*)&t57c8)->begin(2.0f);
        set_order(0, 0x50, 0x1000040, 1.0f);
        order1(this, -1, 0x1f);
        active = 0;
        f5610 = 0;
        agent_by_id(f2c)->f30 = -1;
        agent_by_id(f2c)->f2c8 = 0;
        agent_by_id(f2c)->attack_delay() = 10.0f;     // +0x2c0 as a float (0x457367: 0x41200000)
        restart_inl(agent_by_id(f2c)->ai_ta());
        f2c = -1;
        draw_name();
    } else if (!(fl & 0x20000) || f2c != -1) {
        if ((fl >> 24) && (fl & 0xff000000) == 0x1000000 && f10 == 0 && anim[1].cur != 0x127 && f5874 < 3) {
            order1(this, 0x127, 0x1000040);
            f5874++;
            int kind = 6;
            play_class_sound(&kind);
        }
    } else {
        unsigned tg;
        if (f7c == 2)
            tg = target;
        else
            tg = find_talk_target();
        if ((int)tg >= 0 && agent_by_id(tg)->f30 == -1) {
            g_mission->agents.get(tg)->f5830 = 2;
            g_mission->agents.get(tg)->f30 = id;
            f2c = tg;
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
    ((ActiveTimer*)&t57c8)->expired();
}

// ---- Agent::process_blow: the handler of a pending blow ---------------------------------------------------------------------------









#include "../../include/anim_set.h"   // AnimSet g_anim_set (0xa76628): .data is the array of 0x30-byte animation definitions
struct AnimInfo { char pad0[0x3c]; float length; }; // +0x3c
struct AnimDefV { char pad0[0x10]; AnimInfo* info; char pad14[0x30 - 0x14]; };   // g_anim_set.data entry as 0x457b40 reads it (agent_obj.h's AnimDef calls +0x10 `subs`)
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
        if ((int)last_blow().attacker >= 0 && g_mission->agents.get(last_blow().attacker)->f10 == 1) { \
            impulse.x = impulse.x; \
            impulse.y = impulse.y; \
            impulse.z = impulse.z + 1.0f; \
        } \
    }
// @ 0x457b40
void Agent::process_blow(float unused)
{
    int attacker;
    float dmg;
    int hit_part;
    int anim_a, anim_b;
    int atk_troop, atk_rider;
    Mission* m;
    bool stunned;
    bool cA, cB;
    vector4 imp;
    ItemKind* ik;

    if (f14 != 1)
        return;
    if (has_blow == 0)
        return;
    attacker = last_blow().attacker;
    dmg = last_blow().dmg_out;
    hit_part = last_blow().absorbed;
    has_blow = 0;
    anim_a = -1;
    anim_b = -1;
    if (attacker >= 0)
        last_attacker = attacker;
    m = g_mission;
    atk_troop = -1;
    atk_rider = -1;
    if (attacker >= 0) {
        Agent** pg = &m->agents.pages[0];
        if (pg[(unsigned)attacker >> 4][attacker & 0xf].f10 == 0) {
            atk_rider = attacker;
            if (atk_rider >= 0)
                atk_troop = pg[(unsigned)atk_rider >> 4][atk_rider & 0xf].troop;
        } else if (pg[(unsigned)attacker >> 4][attacker & 0xf].f30 >= 0) {
            atk_rider = m->agents.get(attacker)->f30;
            if (atk_rider >= 0)
                atk_troop = pg[(unsigned)atk_rider >> 4][atk_rider & 0xf].troop;
        }
    }
    if ((f10 == 0 && last_blow().bone == 9) || (f10 == 1 && last_blow().bone == 7)) {
        if (last_blow().item >= 0 && g_items[last_blow().item].is_ranged()) {
            dmg = dmg * 1.75f;
            if (attacker == m->player_id && g_options.damage_msgs != 0) {
                tstring k("ui_head_shot");
                g_gui.add(*g_strtable.lookup(k), 0xffffdd11);
                m = g_mission;
            }
        }
    }
    if (id == m->player_id || f30 == m->player_id) {
        if (g_options.dmg_to_player == 1)
            dmg = dmg * 0.5f;
        else if (g_options.dmg_to_player == 0)
            dmg = dmg * 0.25f;
    } else {
        int dd = g_options.dmg_to_friends;
        GameData* g = g_game;
        if (dd != 2 &&
            ((f10 == 0 && f1bc == g->f78) ||
             (f10 == 1 && f30 >= 0 && m->agents.get(f30)->f1bc == g->f78))) {
            if (dd == 1)
                dmg = dmg * 0.75f;
            else if (dd == 0)
                dmg = dmg * 0.5f;
        }
    }
    health = health - dmg;

    // ---- the damage messages ----
    {
        int shown = g_options.damage_msgs;
        if (last_blow().attacker == m->player_id && shown != 0) {
            char msgbuf[512];
            {
                tstring k((f10 != 0 && f30 != -1) ? "ui_delivered_number_damage_to_horse"
                                                        : "ui_delivered_number_damage");
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
            }
            {
                tstring t(msgbuf);
                g_gui.add(t, -261);
            }
        } else {
            int lid = m->player_id;
            if (lid >= 0 && m->agents.pages[(unsigned)lid >> 4][lid & 0xf].f2c >= 0 &&
                last_blow().attacker == m->agents.pages[(unsigned)lid >> 4][lid & 0xf].f2c && shown != 0 &&
                round_float(&dmg) != 0) {
                char hbuf[256];
                {
                    tstring k("ui_horse_charged_for_number_damage");
                    sprintf(hbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
                }
                {
                    tstring t(hbuf);
                    ((MissionLog*)g_mission)->log_message(&t, 20.0f);
                }
            } else {
                const char* key = 0;
                shown = g_options.damage_msgs;
                m = g_mission;
                if (id == m->player_id && shown != 0) {
                    key = "ui_received_number_damage";
                } else if (m->player_id >= 0) {
                    int l2 = m->player_id;
                    if (m->agents.pages[(unsigned)l2 >> 4][l2 & 0xf].f2c >= 0 &&
                        id == m->agents.pages[(unsigned)l2 >> 4][l2 & 0xf].f2c && shown != 0)
                        key = "ui_horse_received_number_damage";
                }
                if (key) {
                    char hbuf[256];
                    {
                        tstring k(key);
                        sprintf(hbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
                    }
                    g_gui.add(tstring(hbuf), 0xffe0c0c0);
                }
            }
        }
    }

    // ---- hit sounds ----
    if (!(last_blow().flags & 0x20)) {
        int kind;
        float var;
#define HIT_SOUND(k) { kind = (k); play_sound_var(&kind, &var); }
        if (last_blow().item >= 0) {
            if (g_items[last_blow().item].is_ranged()) {
                var = 0.1f;
                if (id == g_mission->player_id)
                    HIT_SOUND(0x66)
                else
                    HIT_SOUND(0x64)
            } else {
                bool blunt = (last_blow().item_mod & itp_wooden_attack) != 0 || last_blow().kind == 2;
                var = 0.15f;
                if (dmg < 9.0f) {
                    if (hit_part < 14) {
                        if (blunt) HIT_SOUND(0x79) else HIT_SOUND(0x75)
                    } else {
                        if (blunt) HIT_SOUND(0x7b) else HIT_SOUND(0x77)
                    }
                } else {
                    if (hit_part < 14) {
                        if (blunt) HIT_SOUND(0x7a) else HIT_SOUND(0x76)
                    } else {
                        if (blunt) HIT_SOUND(0x7c) else HIT_SOUND(0x78)
                    }
                }
            }
        } else {
            var = 0.15f;
            HIT_SOUND(0x65)
        }
#undef HIT_SOUND
    }

    // ---- a missile that sticks in the body ----
    if (last_blow().missile != 0 && g_items[last_blow().missile->ammo.item].flags != itcf_throw_stone &&
        !(g_mission->terrain.f158 & 1) && dmg > 7.0f && (health < 1.0f || last_blow().bone != 9 || dmg > 75.0f)) {
        rgl::mat4 M = body->m * *(rgl::mat4*)body->skel->rec(last_blow().bone);
        float len = last_blow().missile->ammo_scale() * 0.7f;
        matrix mm;
        mm.o = last_blow().pos - len * last_blow().dir;
        mm.f = last_blow().dir;
        mm.u = vector4(0.0f, 0.0f, 1.0f);
        mm.orthonormalize();
        if (last_blow().missile != 0) {
            rgl::mesh* ms = ((Wielder*)last_blow().missile)->make_ranged_mesh();
            if (ms != 0) {
                vector4 off;
                off.x = 0.0f;
                off.y = last_blow().missile->ammo_scale();
                off.z = 0.0f;
                ms->translate(&off);
                ((FxDef*)body->skel)->attach_psys(last_blow().bone, (Psys*)ms, &((rgl::mat4*)&M)->relative(*(rgl::mat4*)&mm));
            }
        }
    }

    // ---- blood ----
    if (dmg > 1.0f && !(*(unsigned*)&g_mission->terrain.f158 & 0x100)) {
        float blood = 1.0f;
        if (last_blow().kind == 2)
            blood = 0.2f;
        else if (last_blow().kind == 1)
            blood = 0.7f;
        if (g_options.blood > 0 && last_blow().kind != 2) {
            bool go = false;
            if (g_options.blood == 2) {
                go = true;
            } else if (g_mission->player_id >= 0) {
                if ((last_blow().pos - g_mission->agents.get(g_mission->player_id)->pos).out_length_squared() < 70.0f)
                    go = true;
            }
            if (go)
                g_mission->blood_burst(last_blow().pos.x, last_blow().pos.y, last_blow().pos.z, *(int*)&last_blow().pos.w, blood * dmg);
        }
    }

    if (f10 == 1) {
        float plen;
        if (health < 1.0f) {
            health = 0.0f;
            f14 = 5;
            f5830 = 3;
            tm.start = g_ticks[tm.clock];
            body->skel->f68 = 1;
            draw_name();
            m = g_mission;
            if (f30 == m->player_id && id == m->f60 && a_bit(g_mission_templates[m->mode].flags, 1) &&
                f8c.item >= 0 && !a_bit(g_items[f8c.item].type_flags, 12)) {
                GameData* g = g_game;
                ItemRef* horse = &g->chars[g->player].equip[8];
                if ((horse->m4 & 0xff000000) == (unsigned)imod_lame << 24) {
                    if (frand() < 0.2f) {
                        tstring k("ui_horse_fell_dead");
                        g_gui.add(*g_strtable.lookup(k), 0xffdd6655);
                        horse->item = -1;
                    }
                } else {
                    if (frand() < 0.2f) {
                        {
                            tstring k("ui_horse_crippled");
                            g_gui.add(*g_strtable.lookup(k), 0xffeebbbb);
                        }
                        horse->b.mod = 0;
                        horse->m4 |= (unsigned)imod_lame << 24;
                        horse->reduce(0);
                    }
                }
            }
            plen = 1.0f;
            if (vel_xy.y > 2.5f) {
                set_order(0, 0x1a4, 0x2000080, 1.0f);
                int kind = 4;
                play_hit_sound(&kind);
                ((Timer*)&t40)->set_delay(-2.4f);
                if (f30 >= 0) {
                    g_mission->agents.get(f30)->set_order(0, 0x123, 0x1000081, 1.0f);
                    vector4 z;
                    z.x = 0.0f;
                    z.y = 0.0f;
                    z.z = 0.0f;
                    g_mission->agents.get(f30)->impulse = z;
                    ((Timer*)&g_mission->agents.get(f30)->t40)->set_delay(-2.4f);
                }
            } else {
                if (frand() <= 1.0f) {
                    set_order(0, 0x1a3, 0x2000080, 1.0f);
                    int kind = 4;
                    play_hit_sound(&kind);
                    plen = ((AnimDefV*)g_anim_set.data)[0x1a3].info->length;
                    if (f30 >= 0)
                        g_mission->agents.get(f30)->set_order(0, 0x122, 0x1000081, 1.0f);
                } else {
                    set_order(0, 0x1a2, 0x2000080, 1.0f);
                    plen = ((AnimDefV*)g_anim_set.data)[0x1a2].info->length;
                    int kind = 4;
                    play_hit_sound(&kind);
                    if (f30 >= 0)
                        g_mission->agents.get(f30)->set_order(0, 0x121, 0x1000081, 1.0f);
                }
                if (plen < 0.01f)
                    plen = 1.0f;
            }
            ((Timer*)&t5918)->set_delay(-plen);
            if (f30 >= 0) {
                g_mission->agents.get(f30)->f2c = -1;
                f30 = -1;
            }
            return;
        }
        // the horse survived: a lance couched into it may stagger it
        // the horse survived: a lance couched into it may stagger it
        if (last_blow().item < 0) return;
        if ((g_items[last_blow().item].type_flags & 0xff) != itp_type_polearm) return;
        if (last_blow().kind != 1) return;
        if (!(last_blow().dmg_out > 8.0f)) return;
        if (!(vel_xy.y > 0.1f)) return;
        if ((int)last_blow().attacker < 0) return;
        if (!(rgl::dot(pos - m->agents.get(last_blow().attacker)->pos, body->m.f) < -0.5f)) return;
        if ((int)(anim[0].events & 0x1ffff) >= 0x4a) return;
        float a = 5.0f;
        float x = 5.0f / ((float)f8c.stat_214() + 2.0f);
        float y = fmax_ref(&vel_xy.y, &a);
        x = (x / y) * 5.0f;
        if (frand() < x) {
            int kind = 4;
            play_hit_sound(&kind);
            set_order(0, 0x19c, 0x100004a, 1.0f);
            return;
        }
        a = 0.95f;
        float z = (float)f8c.stat_214() * 0.01f + 0.7f;
        vel_xy.y = fmin_ref(&z, &a) * vel_xy.y;
        return;
    }

    // ---- a human ----
    stunned = false;
    hit_part = 0x80;
    if (health < 1.0f) {
        vector4 apos;
        apos.x = 0.0f;
        apos.y = 0.0f;
        apos.z = 0.0f;
        if ((int)last_blow().attacker >= 0)
            apos = g_mission->agents.get(last_blow().attacker)->pos;
        vector4 av = apos - pos;
        if (f2c != -1) {
            anim_a = 0x120;
        } else {
            if (av.x * vel.x + (av.y * vel.y + av.z * vel.z) > 0.0f) {
                if (last_blow().bone == 9 && (last_blow().missile != 0 || frand() < 0.7f)) {
                    anim_a = 0x11a;
                } else if (last_blow().bone == 9) {
                    anim_a = 0x11d;
                } else if (last_blow().bone == 0 || last_blow().bone == 7) {
                    anim_a = 0x11c;
                } else if (last_blow().missile == 0 && rgl::dot(last_blow().dir2, body->m.s) > 0.3f) {
                    anim_a = 0x11e;
                } else {
                    anim_a = 0x11b;
                }
            } else {
                anim_a = 0x11f;
            }
        }
    } else {
        if (troop >= 0) {
            if (((dmg < 1.0f) && (last_blow().dmg_in < (frand() + 1.0f) * (float)(g_game->chars[troop].f58[1] + 15) * 0.2f)) ||
                dmg < g_module_settings.damage_interrupt_attack_treshold) {
                if ((int)last_blow().attacker >= 0 && g_mission->agents.get(last_blow().attacker)->f10 == 0)
                    stunned = true;
            }
        }
        if (f2c == -1 && last_blow().item >= 0 &&
            (ik = &g_items[last_blow().item], ik->is_ranged() || (ik->type_flags & 0xff) == itp_type_polearm)) {
            if (!stunned) {
                if (((ik->type_flags & 0xff) == itp_type_polearm && frand() < 0.5f && last_blow().dmg_in > 15.0f) ||
                    ((g_items[last_blow().item].type_flags & 0xff) == itp_type_thrown && frand() < 0.4f && last_blow().dmg_in > 15.0f) ||
                    ((g_items[last_blow().item].type_flags & 0xff) == itp_type_bow && frand() < 0.3f && last_blow().dmg_in > 25.0f) ||
                    ((g_items[last_blow().item].type_flags & 0xff) == itp_type_crossbow && frand() < 0.3f && last_blow().dmg_in > 25.0f)) {
                    anim_a = move_anim(2);
                    float k = frand() * 0.2f + 2.1f;
                    hit_part = 0x82;
                    imp.x = k * last_blow().dir.x;
                    imp.y = k * last_blow().dir.y;
                    imp.z = k * last_blow().dir.z;
                    goto impulse;
                }
                anim_a = move_anim(1);
            }
        } else if (f2c != -1) {
            anim_b = move_anim(0);
        } else do {
            if (rgl::dot(vel, last_blow().dir) < 0.0f &&
                (((last_blow().flags & 0x10) && last_blow().dmg_in > 7.0f) || (last_blow().dmg_in > 24.0f && frand() < 0.4f)) &&
                last_blow().kind == 2 && anim[0].cur != 0x125) {
                anim_a = 0x125;
                imp.x = vel.x * -0.6f;
                anim_b = 0x126;
                imp.y = vel.y * -0.6f;
                imp.z = vel.z * -0.6f;
                if (last_blow().flags & 0x10) {
                    float rr = frand() * 0.4f + 1.5f;
                    imp.x = rr * last_blow().dir.x + imp.x;
                    imp.y = rr * last_blow().dir.y + imp.y;
                    imp.z = rr * last_blow().dir.z + imp.z;
                }
            } else {
                anim_b = move_anim(1);
                if (!(last_blow().flags & 0x10)) {
                    if (!(last_blow().dmg_in > 15.0f))
                        break;
                    if (!(frand() < 0.3f))
                        break;
                }
                {
                    float e = (frand() - 0.5f) * 0.5f;
                    float f = (frand() + 0.6f) * (last_blow().dmg_in + 30.0f) * 0.02f;
                    vector4* sv = &body->m.s;
                    imp.x = f * last_blow().dir.x + e * sv->x;
                    imp.y = f * last_blow().dir.y + e * sv->y;
                    imp.z = f * last_blow().dir.z + e * sv->z;
                }
            }
            if (!stunned) {
            impulse:
                PB_IMPULSE()
            }
            } while (0);
    }

    cA = false;
    cB = false;
    if (health < 1.0f) {
        health = 0.0f;
        m = g_mission;
        float surg = 0.0f;
        if (m->player_id >= 0) {
            if (m->agents.get(m->player_id)->is_friend(this)) {
                surg = (float)g_game->parties.get(g_game->f78)->skill_level(10) * 0.04f + 0.25f;
                m = g_mission;
            }
        }
        if (!(m->terrain.f158 & 1) &&
            ((g_game->chars[troop].flags30 & tf_allways_fall_dead) != 0 ||
             (last_blow().kind != 2 && troop >= 0 && !g_game->chars[troop].is_hero() && frand() > surg)))
            cA = true;
        else
            cB = true;
        f5830 = 3;
        tm.start = g_ticks[tm.clock];
        mount_id = f2c;
        body->skel->f68 = 1;
        float len = 0.5f;
        if (anim_a >= 0) {
            len = ((AnimDefV*)g_anim_set.data)[anim_a].info->length;
            if (len < 0.01f)
                len = 0.5f;
        }
        ((Timer*)&t5918)->set_delay(-len);
        hit_bone = last_blow().bone;
        hit_vec = last_blow().dir2;
        hit_vec.normalize();
        float k = (last_blow().dmg_in + 5.0f) * 0.2f;
        hit_vec.x = k * hit_vec.x;
        hit_vec.y = k * hit_vec.y;
        hit_vec.z = k * hit_vec.z;
        hit_pos = last_blow().pos;
        if (last_blow().flags & 0x10) {
            hit_vec.x = hit_vec.x + hit_vec.x;
            hit_vec.y = hit_vec.y + hit_vec.y;
            hit_vec.z = hit_vec.z + hit_vec.z;
        }
        if (last_blow().kind == 2) {
            hit_vec.x = hit_vec.x + hit_vec.x;
            hit_vec.y = hit_vec.y + hit_vec.y;
            hit_vec.z = hit_vec.z + hit_vec.z;
        }
        draw_name();
        m = g_mission;
        if (f2c >= 0) {
            if (m->player_id < 0 || m->agents.get(m->player_id)->f2c != -1 ||
                ((last_blow().attacker != m->player_id || m->dismount_count >= 4) && !(m->terrain.f158 & 1))) {
                m->agents.get(f2c)->f5844 = 1;
                Agent* h = g_mission->agents.get(f2c);
                h->tm.start = g_ticks[h->tm.clock];
            } else {
                Agent* h = m->agents.get(f2c);
                vector4 bf;
                bf.x = h->vel_xy.y * body->m.f.x;
                bf.y = body->m.f.y * h->vel_xy.y;
                bf.z = body->m.f.z * h->vel_xy.y;
                ((Timer*)&t40)->set_delay(-1.4f);
                impulse = bf;
                g_mission->agents.get(f2c)->f30 = -1;
                f2c = -1;
                g_mission->dismount_count++;
            }
        }
    }

    // ---- experience and weapon proficiency for the attacker ----
    if (atk_troop >= 0 && this->is_enemy(PAGE_AGENT(g_mission, atk_rider)) && dmg > 0.01f && (cA || cB)) {
        TroopRec* chars = g_game->chars;
        float xpf = (float)chars[troop].scaled_b0();
        if (g_mission->terrain.f158 & 1)
            xpf = xpf * 0.1f;
        int xp = (int)xpf;
        if (xp != 0) {
            if (chars[g_mission->agents.get(atk_rider)->troop].is_hero()) {
                GameData* g = g_game;
                if (atk_troop == g->player)
                    g->add_experience(atk_troop, (int)((float)xp * g_module_settings.player_xp_multiplier));
                else
                    g->add_experience(atk_troop, (int)((float)xp * g_module_settings.hero_xp_multiplier));
            } else {
                g_mission->agents.get(atk_rider)->xp_pool += (int)((float)xp * g_module_settings.regulars_xp_multiplier);
            }
        }
    }
    if ((int)last_blow().attacker >= 0 && dmg > 0.01f) {
        AgentTable* t = &g_mission->agents;
        if (this->is_enemy(t->get(last_blow().attacker)) && t->get(last_blow().attacker)->troop >= 0 &&
            g_game->chars[t->get(last_blow().attacker)->troop].is_hero() && last_blow().item >= 0) {
            ItemRef sp = t->get(last_blow().attacker)->slot_pair(1);
            int cls = slot_class_impl(last_blow().item, sp.item >= 0);
            int old = (int)g_game->chars[atk_troop].f80[cls];
            float dm = dmg;
            if (g_mission->terrain.f158 & 1)
                dm = dm * 0.33f;
            float amount = (((float)g_game->chars[troop].level + 10.0f) * dm) * 0.05f;
            vector4 vA = v56b4 - t->get(last_blow().attacker)->v56b4;
            vector4 vB = pos - t->get(last_blow().attacker)->pos;
            float lenB = vB.normalize();
            float lenA = vA.normalize();
            float cl = rgl::cross(vA, vB).out_length();
            char msgbuf[512];
            char namebuf[256];
            float w;
            if (cls == 3 || cls == 5 || cls == 4 || cls == 6) {
                float tt = ((cl * lenA + 4.0f) * 0.25f) * ((lenB + 4.0f) * 0.25f) * 0.3f;
                if (tt < 1.0f)
                    tt = 1.0f;
                else if (tt > 12.0f)
                    tt = 12.0f;
                if (last_blow().attacker == g_mission->player_id && g_options.shot_difficulty != 0) {
                    float sd = tt;
                    if (last_blow().bone == 9)
                        sd = tt * 1.2f;
                    {
                        tstring k("ui_shot_difficulty");
                        sprintf(msgbuf, g_strtable.lookup(k)->c_str(), (double)sd);
                    }
                    {
                        g_gui.add(tstring(msgbuf), 0xffaaaaaa);
                    }
                }
                w = tt * amount;
            } else {
                w = amount;
            }
            g_game->chars[atk_troop].adjust(cls, w * 0.02f);
            int nw = (int)g_game->chars[atk_troop].f80[cls];
            if (atk_troop == 0 && nw > old) {
                ui_weapon_class_name(cls, namebuf);
                int diff = nw - old;
                if (diff == 1) {
                    {
                        tstring k("ui_you_have_improved_your_proficiency_in_value_to_number");
                        sprintf(msgbuf, g_strtable.lookup(k)->c_str(), namebuf, nw);
                    }
                } else {
                    tstring k("ui_your_proficiency_in_value_has_improved_by_number_to_number");
                    sprintf(msgbuf, g_strtable.lookup(k)->c_str(), namebuf, diff, nw);
                }
                g_gui.add(tstring(msgbuf), 0xffffdf44);
            }
        }
    }

    // ---- the impact particles ----
    if (dmg > 1.0f && !(g_mission->terrain.f158 & 1) && g_cfg.enable_blood != 0) {
        float hi = 50.0f;
        float lo = 1.0f;
        float k = ((dmg + 5.0f) * g_module_settings.blood_multiplier) * 0.1f + 1.0f;
        float ef = clamp(&k, &lo, &hi);
        matrix m2;
        m2.o = last_blow().pos;
        m2.f = vector4(-last_blow().dir.x, -last_blow().dir.y, -last_blow().dir.z + 0.5f);
        m2.u = vector4(0.0f, 0.0f, 1.0f);
        m2.orthonormalize();
        int vi = (int)(g_game->chars[troop].flags30 & 0xf);
        int e1 = g_anim_table.get_f1f0(vi);
        int e2 = g_anim_table.get_f1f4(vi);
        if (e1 == 0)
            e1 = g_emit_defs.used[2];
        if (e2 == 0)
            e2 = g_emit_defs.used[3];
        if (e1 >= 0) {
            rgl::psys* em = g_mission->spawn_effect(e1, (const rgl::mat4*)&m2, (int)(ef + ef));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e1].b.x;
            v.y = s * g_emit_defs.defs[e1].b.y;
            v.z = s * g_emit_defs.defs[e1].b.z;
            ((Obj3*)em)->set_v184(&v);       // the emitter is a scene object with a placement block (game_util.h Obj3; the code is shared)
            em->f198 = ef * g_emit_defs.defs[e1].f24;
        }
        if (e2 >= 0) {
            rgl::psys* em = g_mission->spawn_effect(e2, (const rgl::mat4*)&m2, (int)(ef * 4.0f));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e2].b.x;
            v.y = s * g_emit_defs.defs[e2].b.y;
            v.z = s * g_emit_defs.defs[e2].b.z;
            ((Obj3*)em)->set_v184(&v);       // the emitter is a scene object with a placement block (game_util.h Obj3; the code is shared)
            em->f198 = ef * g_emit_defs.defs[e2].f24;
        }
    }

    // ---- the outcome ----
    if (cA) {
        if (!(last_blow().flags & 0x20)) {
            int kind = 0;
            play_class_sound(&kind);
        }
        set_order(0, anim_a, 0x28000c8, 1.0f);
        f14 = 5;
        m = g_mission;
        if (last_blow().attacker == m->player_id) {
            g_game->cnt_killed++;
            if (this->is_friend(m->agents.get(g_mission->player_id)))
                g_game->cnt_killed_friend++;
            m = g_mission;
        }
        if ((int)last_blow().attacker >= 0)
            m->agents.get(last_blow().attacker)->kill_count++;
        if (g_options.kill_msgs != 0) {
            char msgbuf[512];
            tstring unused_s;
            if (atk_troop >= 0) {
                tstring k("ui_value_killed_by_value");
                const char* n1 = g_game->chars[atk_troop].display_name()->c_str();
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), n2, n1);
            } else {
                tstring k("ui_value_fell_dead");
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), n2);
            }
            m = g_mission;
            int lid = m->player_id;
            int color = 0xff44ddaa;
            if (lid >= 0) {
                if (m->agents.get(lid)->team == team)
                    color = 0xffcc4411;
                else if (lid >= 0 && m->agents.get(lid)->is_friend(this))
                    color = 0xffce1180;
            }
            g_gui.add(tstring(msgbuf), color);
        }
        draw_name();
        return;
    }
    if (cB) {
        if (!(last_blow().flags & 0x20)) {
            int kind = 0;
            play_class_sound(&kind);
        }
        set_order(0, anim_a, 0x28000c8, 1.0f);
        f14 = 4;
        m = g_mission;
        if (last_blow().attacker == m->player_id) {
            g_game->cnt_ko++;
            if (this->is_friend(m->agents.get(g_mission->player_id)))
                g_game->cnt_ko_friend++;
            m = g_mission;
        }
        if ((int)last_blow().attacker >= 0)
            m->agents.get(last_blow().attacker)->wounded_count++;
        if (g_options.kill_msgs != 0) {
            char namebuf[256];
            if (atk_troop >= 0) {
                tstring k("ui_value_knocked_unconscious_by_value");
                const char* n1 = g_game->chars[atk_troop].display_name()->c_str();
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(namebuf, g_strtable.lookup(k)->c_str(), n2, n1);
            } else {
                tstring k("ui_value_fell_unconscious");
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(namebuf, g_strtable.lookup(k)->c_str(), n2);
            }
            m = g_mission;
            int lid = m->player_id;
            int color = 0xff44ddaa;
            if (lid >= 0) {
                if (m->agents.get(lid)->team == team)
                    color = 0xffcc9911;
                else if (lid >= 0 && m->agents.get(lid)->is_friend(this))
                    color = 0xffb26fdd;
            }
            g_gui.add(tstring(namebuf), color);
        }
        draw_name();
        return;
    }
    if (stunned)
        return;
    if (!(last_blow().flags & 0x20)) {
        int kind = 1;
        play_class_sound(&kind);
    }
    if (anim_a >= 0)
        set_order(0, anim_a, hit_part | 0x5800000, 1.0f);
    if (anim_b >= 0)
        set_order(1, anim_b, hit_part | 0x5800000, 1.0f);
}

// ---- Agent::swing_trace: the melee swing / missile hit test ------------------------------------------------------------------------
// AgentX::f100_is_0x48_and_charged 0x432470: declared in agent3/agent_types.h (agent3: out of line at the damage-doubling site; inlined as couching()); the agent calls it with an unused agent in ecx
int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag);   // 0x43a720 (agent2/agent_stats.cpp)
static __forceinline int round_inl(const float& x)
{
    float fl = (float)floor((double)x);
    int i = (int)fl;
    return (x - fl <= 0.5f) ? i : i + 1;
}
static __forceinline bool rel_is(int a, int b, int v, int* rel)
{
    if (a < 0 || a >= 8 || b < 0 || b >= 8)
        return false;
    *rel = ((const int*)&g_mission->teams[0].relation[0])[a * 0x5b + b];
    return *rel == v;
}
#define SND(who, n) { int sid_ = (n); float var_ = 0.1f; (who)->play_sound_var(&sid_, &var_); }
static __forceinline float sw_vlen(const vector4& v) { return (float)sqrt(v.y * v.y + (v.z * v.z + v.x * v.x)); }
static __forceinline vector4 sw_vmul(const vector4& v, const float& k) { return vector4(v.x * k, v.y * k, v.z * k); }
static __forceinline vector4 sw_smul(const float& k, const vector4& v) { return vector4(k * v.x, k * v.y, k * v.z); }
static __forceinline vector4 sw_vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 sw_vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline unsigned prop_flags(const char* k) { return *(const unsigned*)(k + 0x10); }
// @ 0x45a170
int Agent::swing_trace(int hand, Missile* missile, Blow* out, float* scale)
{
    capsule sweep;
    ItemRef sp = slot_pair(hand);
    float reach = 0.2f;
    float inner = 0.0f;
    vector4 tA, bA, tB, bB, dT, dB, bBs;
    int steps;
    float len;
    if (missile == 0) {
        if (fr2.o.x < -1e7f)
            return 0;
        if (sp.item >= 0) {
            reach = (float)g_items[sp.item].weight * 0.01f - 0.01f;
        } else {
            reach = 0.15f;
            inner = 0.0f;
        }
        if (mode1 == 0) {
            float t = reach - 1.0f;
            inner = (0.0f > t) ? 0.0f : t;
        }
        tA = sw_vadd(sw_smul(reach, fr1.f), fr1.o);
        bA = sw_vadd(sw_smul(inner, fr1.f), fr1.o);
        tB = sw_vadd(sw_smul(reach, fr2.f), fr2.o);
        bB = sw_vadd(sw_smul(inner, fr2.f), fr2.o);
        bBs = bB;
        dT = sw_vsub(tA, tB);
        dB = sw_vsub(bA, bB);
        steps = round_inl((len = sw_vlen(dT)) * 20.0f) + 1;
        sweep.p0 = bB;
        sweep.p1 = tA;
        sweep.radius = 0.1f;
    } else {
        ((FireAgent*)missile)->ray((FireRay*)&sweep);
        steps = 1;
    }

    int result = 0;
    bool ragdoll = false;
    shape_base* found = 0;
    int probed = 0;
    char can;
    if (!prop_done) {
        capsule pc(sweep);
        pc.radius = 0.0f;
        if (missile == 0 && mode1 != 0) {
            can = 1;
            probed = ::probe_shape_contact(&pc, &bB, &found, &can);
        } else {
            can = 0;
            probed = ::probe_shape_contact(&pc, &bB, &found, &can);
        }
    }
    int ncand = 0;
    float best_d2 = 10000.0f;
    int best = -1;
    can = 0;
    bool hit_flag = false;
    float maxdist = 1000.0f;
    if (probed) {
        vector4 d = sw_vsub(bB, sweep.p0);
        maxdist = (float)sqrt(d.x * d.x + (d.y * d.y + d.z * d.z));
    }
    unsigned cand[0x400];
    unsigned i;
    if (!prop_done && (missile != 0 || probed == 0)) {
        vector4 sd = sw_vsub(sweep.p0, sweep.p1);
        float limit2 = (sd.x * sd.x + (sd.y * sd.y + sd.z * sd.z)) + 50.0f;
        unsigned end = g_mission->agents.count;
        for (i = g_mission->agents.first_used(); i < end; g_mission->agents.next_used(&i)) {
            if (ncand >= 0x400)
                break;
            if (i != (unsigned)id && i != (unsigned)f2c) {
                Agent* o = agent_by_id(i);
                bool ok = o->f5830 == 2;
                if (!ok && is_friend(o) && agent_by_id(i)->f5830 == 3 &&
                    g_mission->agents.get(i)->tm.elapsed() < 2.0f)
                    ok = true;
                if (ok) {
                    vector4 dd = sw_vsub(o->pos, sweep.p0);
                    float d2 = dd.x * dd.x + (dd.y * dd.y + dd.z * dd.z);
                    if (d2 < limit2) {
                        cand[ncand] = i;
                        if (d2 < best_d2) {
                            best = ncand;
                            ncand++;
                            best_d2 = d2;
                        } else {
                            ncand++;
                        }
                    }
                }
            }
        }
        if (best >= 0) {
            unsigned t = cand[0];
            cand[0] = cand[best];
            cand[best] = t;
        }
        for (i = 0; (int)i < ncand; i++) {
            Agent* o = agent_by_id(cand[i]);
            bool hit = false;
            if (o->f10 == 0) {
                capsule oc;
                o->get_label_box(&oc);
                contact ct;
                vector4 hitpt;
                if (missile != 0) {
                    hit = shape_contact_count(&sweep, &oc, 1, &ct) != 0;
                    hitpt = ct.point;
                } else {
                    sweep.p0 = bBs;
                    sweep.p1 = tA;
                    sweep.radius = 0.9f;
                    if (shape_contact_count(&oc, &sweep, 0, 0) != 0) {
                        sweep.radius = 0.1f;
                        int n = round_inl(len * 10.0f) + 1;
                        for (int j = 0; j < n && !hit; j++) {
                            float t = (float)j / (float)n;
                            vector4 T = sw_vadd(sw_smul(t, dT), tB);
                            vector4 Bp = sw_vadd(sw_smul(t, dB), bBs);
                            vector4 d = sw_vsub(T, Bp);
                            d.normalize();
                            float k = (reach - inner) - 0.15f;
                            sweep.p0 = Bp;
                            sweep.p1 = sw_vadd(sw_smul(k, d), Bp);
                            hit = shape_contact_count(&oc, &sweep, 1, &ct) != 0;
                            hitpt = ct.point;
                        }
                    }
                }
                if (hit) {
                    int rel;
                    if (!rel_is(team, o->team, -1, &rel)) {
                        if (!rel_is(team, o->team, 1, &rel))
                            return 2;
                    }
                    bool fr = rel == 1;
                    if (fr) {
                        if (missile == 0) {
                            vector4 dv(o->pos.x - pos.x, o->pos.y - pos.y, o->pos.z - pos.z);
                            float dl = dv.normalize();
                            if (f2c == -1 && o->f2c == -1 &&
                                0.8f < dv.x * vel.x + (dv.y * vel.y + dv.z * vel.z) && dl < 1.3f)
                                return 2;
                            goto next;
                        }
                        if (id != g_mission->player_id)
                            return 2;
                    }
                    ItemRef cur = slot_pair_fi(0);
                    sp = cur;
                    int twohand = 0;
                    if (cur.item >= 0 && (g_items[cur.item].type_flags & 0x20000) != 0)
                        twohand = 1;
                    can = can_hit(id, cand[i], hand, (int)missile, &hitpt.x) != 0;
                    if (can) {
                        ItemRef os = o->slot_pair_fi(1);
                        if (os.item >= 0 && (g_items[os.item].type_flags & 0xff) == itp_type_shield) {
                            float dmg;
                            unsigned fl2[2];
                            hit_damage(hand, missile, &dmg, fl2, o->id, scale);
                            ItemRef ms = slot_pair_fi(0);
                            unsigned wf = ms.item < 0 ? 0 : g_items[ms.item].type_flags;
                            if ((wf & 0x4000000) != 0)
                                dmg = dmg + dmg;
                            if (((const AgentX*)this)->f100_is_0x48_and_charged())
                                dmg = dmg + dmg;
                            if (missile != 0 && ((unsigned)g_items[missile->launcher.item].flags & 0xff000) != 0x10000)
                                o->refresh_carried(missile);
                            o->shield_hit((int)dmg);
                            if ((g_items[os.item].type_flags & 0x40000) != 0) {
                                if (twohand) { SND(o, 0x71); } else { SND(o, 0x73); }
                            } else {
                                if (twohand) { SND(o, 0x74); } else { SND(o, 0x72); }
                            }
                        } else {
                            ItemRef os0 = o->slot_pair_fi(0);
                            if (sp.item == -1) {
                                SND(this, 0x6d);
                            } else if (os0.item >= 0 && (g_items[os0.item].type_flags & 0x40000) != 0) {
                                if (twohand) { SND(this, 0x6e); } else { SND(this, 0x70); }
                            } else {
                                if (twohand) { SND(this, 0x70); } else { SND(this, 0x6f); }
                            }
                        }
                        o->set_default_order();
                        result = 2;
                    } else {
                        sweep.radius = 0.1f;
                        for (int j = 0; j < steps && result == 0; j++) {
                            if (missile == 0 && mode1 != 0) {
                                float t = (float)j / (float)steps;
                                vector4 T = sw_vadd(sw_smul(t, dT), tB);
                                vector4 Bp = sw_vadd(sw_smul(t, dB), bBs);
                                vector4 d = sw_vsub(T, Bp);
                                d.normalize();
                                float k = reach - inner;
                                sweep.p0 = Bp;
                                sweep.p1 = sw_vadd(sw_smul(k, d), Bp);
                            }
                            vector4 hp;
                            vector4 nrm;
                            int bone;
                            result = o->capsule_hit(&sweep, &hp, &nrm, &bone) & 0xff;
                            vector4 hv = sw_vsub(hp, sweep.p0);
                            if (result != 0 && sw_vlen(hv) < maxdist) {
                                hit_flag = true;
                                if (id == g_mission->player_id && is_friend(o)) {
                                    g_gui.add(*g_strtable.lookup(tstring("ui_you_hit_a_friendly_troop")),
                                                 0xffff0000);
                                }
                                out->flags = 0;
                                out->attacker = id;
                                out->pos = hp;
                                out->bone = bone;
                                int flg;
                                hit_damage(hand, missile, &out->dmg_in, (unsigned*)&flg, o->id, scale);
                                if (missile != 0) {
                                    out->dir = missile->p40;
                                    out->dir2 = missile->p40;
                                    out->dir2.normalize();
                                    out->item = missile->launcher.item;
                                    out->item_mod = missile->launcher.m4;
                                    out->kind = (g_items[missile->launcher.item].f214 >> 8) & 3;
                                    out->missile = missile;
                                } else {
                                    ItemRef s0 = slot_pair(hand);
                                    out->dir = sw_vsub(o->pos, pos);
                                    if (s0.item < 0)
                                        out->kind = 2;
                                    else if (flg == 0)
                                        out->kind = (g_items[s0.item].f218 >> 8) & 3;
                                    else
                                        out->kind = (g_items[s0.item].f214 >> 8) & 3;
                                    out->dir2 = dT;
                                    out->dir2.normalize();
                                    out->dir.normalize();
                                    out->item = s0.item;
                                    out->item_mod = s0.m4;
                                    if (f100_is_0x48_and_charged())
                                        out->flags |= 0x10;
                                }
                                o->reduce_blow(out);
                                if (o->has_blow == 0 || !(out->dmg_out < o->last_blow().dmg_out)) {
                                    o->last_blow() = *out;
                                    o->has_blow = 1;
                                }
                                result = 1;
                            }
                        }
                    }
                }
            } else {
                char skip = missile == 0 ? hit_once : missile->spent;
                if (skip == 0) {
                    int rd = o->f30;
                    if (rd < 0 || !(char)is_neutral(agent_by_id(rd))) {
                        if (missile != 0 || (rd >= 0 && is_enemy(agent_by_id(rd)))) {
                            bool h = false;
                            vector4 hp;
                            vector4 nrm;
                            int bone;
                            if (missile != 0) {
                                h = o->capsule_hit(&sweep, &hp, &nrm, &bone);
                            } else {
                                float l6 = len * 6.6666665f;
                                sweep.radius = 0.1f;
                                int n = round_float(&l6) + 1;
                                for (int j = 0; j < n && !h; j++) {
                                    float t = (float)j / (float)n;
                                    vector4 T = sw_vadd(sw_smul(t, dT), tB);
                                    vector4 Bp = sw_vadd(sw_smul(t, dB), bBs);
                                    vector4 d = sw_vsub(T, Bp);
                                    d.normalize();
                                    float k = reach - inner;
                                    sweep.p0 = Bp;
                                    sweep.p1 = sw_vadd(sw_smul(k, d), Bp);
                                    h = o->capsule_hit(&sweep, &hp, &nrm, &bone);
                                }
                            }
                            if (h) {
                                out->flags = 0;
                                out->attacker = id;
                                out->pos = hp;
                                out->bone = bone;
                                int flg;
                                hit_damage(hand, missile, &out->dmg_in, (unsigned*)&flg, o->id, scale);
                                if (missile != 0) {
                                    out->dir = missile->p40;
                                    out->dir2 = missile->p40;
                                    out->item = missile->launcher.item;
                                    out->item_mod = missile->launcher.m4;
                                    out->kind = (g_items[missile->launcher.item].f214 >> 8) & 3;
                                    out->missile = missile;
                                } else {
                                    ItemRef s0 = slot_pair(hand);
                                    if (s0.item < 0)
                                        out->kind = 2;
                                    else if (flg == 0)
                                        out->kind = (g_items[s0.item].f218 >> 8) & 3;
                                    else
                                        out->kind = (g_items[s0.item].f214 >> 8) & 3;
                                    out->dir = nrm;
                                    out->dir2 = nrm;
                                    out->item = s0.item;
                                    out->item_mod = s0.m4;
                                }
                                o->reduce_blow(out);
                                if (o->has_blow == 0 || !(out->dmg_out < o->last_blow().dmg_out)) {
                                    o->last_blow() = *out;
                                    o->has_blow = 1;
                                }
                                if (missile != 0) {
                                    missile->spent = 1;
                                    result = 1;
                                } else {
                                    hit_once = 1;
                                    int r = rand() % 0x3dc9;
                                    *scale = ((float)r * 1.2644623e-05f + 0.4f) * *scale;
                                }
                            }
                        }
                    }
                }
            }
        next:
            if (result != 0)
                break;
        }
    }

    if (!prop_done && !hit_flag && !can && probed != 0) {
        int eid = found->mc;
        if (eid >= 0) {
            SceneEnt* e = (SceneEnt*)g_mission->scene->find_entity(eid);
            if (e != 0) {
                int pi = e->id;
                e->type = 0xc;
                if (pi >= 0 && (unsigned)pi < (unsigned)((Vec1c8*)&g_mission->props)->size()) {
                    PlacedObj* pr = &((PlacedObj*)((Vec1c8*)&g_mission->props)->first)[pi];
                    prop_done = 1;
                    if (pr->kind == 0) {
                        unsigned kf = prop_flags((char*)g_propkinds + pr->id * 0x78) & 0x8000;
                        if ((kf != 0 && 0.0f < pr->hp()) || kf == 0) {
                            float dmg = 0.0f;
                            int flg = 0;
                            if (kf != 0 && 0.0f < pr->hp()) {
                                hit_damage(0, missile, &dmg, (unsigned*)&flg, 0xffffffff, scale);
                                pr->hp() = pr->hp() - dmg;
                            }
                            char* kind = (char*)g_propkinds + pr->id * 0x78;
                            ((Placement*)g_game)->set_matrix(&pr->m18);
                            g_game->f1a1f0 = pi;
                            g_script.param1 = pi;
                            g_script.param2 = (__int64)dmg;
                            matrix frame;
                            frame.o = bB;
                            if (missile != 0) {
                                frame.f = missile->p40;
                                frame.f.normalize();
                                frame.u = vector4(0.0f, 0.0f, 1.0f);
                                frame.orthonormalize();
                                if ((prop_flags(kind) & 0x18000) != 0) {
                                    rgl::mesh* m = ((Wielder*)missile)->make_ranged_mesh();
                                    if (m != 0) {
                                        vector4 o2 = sw_vsub(frame.o, sw_vmul(frame.f, 0.2f));
                                        matrix fm2;
                                        *(rgl::mat3*)&fm2 = *(rgl::mat3*)&frame;
                                        fm2.o = o2;
                                        *(rgl::mat4*)&m->m_cc = ((rgl::mat4*)&e->local)->relative((const rgl::mat4&)fm2);
                                        e->add_extra((rgl::mesh*)m);
                                        ragdoll = true;
                                    }
                                }
                            } else {
                                *(rgl::mat3*)&frame = *(const rgl::mat3*)&fr1;
                            }
                            g_script.pos[1] = frame;
                            ((TimerList*)(kind + 0x6c))->update(-0x2a);
                            if ((prop_flags(kind) & 0x8000) != 0 && pr->hp() <= 0.0f)
                                ((TimerList*)(kind + 0x6c))->update(-0x2b);
                        }
                    }
                }
            }
        }
        if (missile != 0) {
            if (ragdoll)
                return 2;
            missile->ent->local.o = bB;
            result = 3;
        }
    }
    return result;
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
static __forceinline void log_timed(const char* s) { tstring msg(s); ((MissionLog*)g_mission)->log_message(g_strtable.lookup(msg), 20.0f); }
#define LOG_TIMED(str) log_timed(str);
// @ 0x45c220
void Agent::update_attack(float dt)
{
    int slot;
    Blow hit;
    hit.init();
    if (((int)(anim[0].events & 0x1ffff) >= 0x20 && anim[0].cur >= 0 && (defs->defs[anim[0].cur].flags & 0x400)) ||
        (int)(PRIO1 & 0x1ffff) >= 0x40) {
        active = 0;
        f5610 = 0;
        return;
    }
    ItemRef cur = slot_pair_fi(0);
    Skeleton* sk = body->skel;
    if (!sk->done) {
        if (sk->has_poses)
            ((FxAnim*)sk)->update_poses();
        else
            ((FxAnim*)sk)->update_poses2();
    }

    fr1 = body->m * *(rgl::mat4*)(sk->recs + 0x23d8);
    if (f10 != 0)
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
            float t = dt + f57b4;
            f57b4 = t;
            if (t > 1.0f)
                f57b4 = 1.0f;
            if (in_flags & 0x100) {
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
                        f57c0 = f57b4;
                    }
                    i++;
                } while (!found);
            }
        } else if (active == 1) {
            if (PROG > 0.99f) {
                float u = dt * 0.8f + f57c0;
                f57c0 = u;
                if (u < 1.0f)
                    f57c0 = dt * 0.6f + u;
                if (f57c0 > 1.3f)
                    f57c0 = 1.3f;
            }
            if (ACT == snd_a && !(in_flags & 0x200)) {
                if (id == g_mission->player_id) {
                    if (in_flags & 0x100) {
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
                    if (in_flags & 0x100)
                        goto melee_tail;
                }
                if (PROG > 0.99f) {
                    if (snd_b == -1) {
                        active = 0;
                    } else {
                        set_order(1, snd_b, 0x0140000a, damage_factor(0));
                        active = 2;
                        { int kind = 3; play_hit_sound(&kind); }
                        if (f2c == -1 && frand() < (f57c0 - 1.0f) * 3.5f) {
                            if (id == g_mission->player_id || frand() < 0.3f) {
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
                f57b4 = 0.8f;
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
            if (in_flags & 0x200) {
                active = 6;
                f57b4 = 0.8f;
                unsigned tt = TYPE;
                int aid = 0x72;
                if (tt == 4) aid = 0x74;
                else if (tt == 3) aid = 0x73;
                set_order(1, aid, 0x0140000c, damage_factor(0));
            }
            if (!f100_is_0x48_and_charged() && (!(PROG < 0.85f) || (mode1 != 0 && !(PROG > 0.2f))))
                goto chk99;
            if (f100_is_0x48_and_charged())
                f57c0 = 1.0f;
            int r = swing_trace(0, 0, &hit, &f57c0);
            if (r != 0 && f100_is_0x48_and_charged())
                order1(this, 0x44, 0x0100007d);
            if (r >= 2) {
                set_order(1, snd_c, 0x1400023, damage_factor(0));
                active = 5;
            }
            if ((in_flags & 0x100) && id != g_mission->player_id) {
                active = 6;
                f57b4 = 0.8f;
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
                    links[slot]->vis = 0xffff;
            } else {
                float var2 = 0.0f;
                links[wield[0]]->cap_all(&var2);
                if (slot >= 0)
                    links[slot]->vis = 0;
            }
        } else if (TYPE == itp_type_bow && links[wield[0]]) {
            float var2 = 0.0f;
            links[wield[0]]->cap_all(&var2);
        }
        int S = active;
        if (S == 0) {
            if (!(in_flags & 0x400))
                goto tail;
            if ((TYPE == itp_type_crossbow || TYPE == itp_type_pistol || TYPE == itp_type_musket) &&
                cur.max_amount() - (cur.m4 & 0xff) == 0) {
                if (missing > 0) {
                    f5620 = -1;
                    if (f2c == -1) {
                        f5620 = 0x7b;
                    } else if (ik->type_flags & itp_cant_reload_on_horseback) {
                        if (id == g_mission->player_id) {
                            tstring msg("ui_cant_reload_this_weapon_on_horseback");
                            g_gui.add(*g_strtable.lookup(msg), -1);
                        }
                    } else {
                        f5620 = 0x7c;
                    }
                    unsigned __int64 rl = ik->flags & itcf_reload_mask;
                    if (rl != 0) {
                        if (rl == itcf_reload_pistol)
                            f5620 = 0x87;
                        else if (rl == itcf_reload_musket)
                            f5620 = 0x8a;
                    }
                    if (f5620 >= 0) {
                        set_order(1, f5620, 0x0140003c, damage_factor(0));
                        update_quiver();
                        active = 2;
                    }
                } else if (id == g_mission->player_id) {
                    if (total > 0) {
                        LOG_TIMED("ui_no_more_bolts")
                    } else {
                        LOG_TIMED("ui_you_are_not_carrying_any_bolts")
                        goto no_ammo;
                    }
                }
            } else if (missing <= 0 && TYPE != itp_type_crossbow && TYPE != itp_type_pistol && TYPE != itp_type_musket) {
                if (id == g_mission->player_id) {
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
                links[slot]->vis = 0xffff;
                update_quiver();
            }
            if (in_flags & 0x200) {
                order1(this, -1, 2);
                if ((unsigned char)ik->type_flags == itp_type_bow && links[slot]) {
                    links[slot]->vis = 0;
                    update_quiver();
                }
                goto clear_state;
            } else if (!(in_flags & 0x100) &&
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
                    links[slot]->vis = 0;
                active = 0;
            } else {
                active = 3;
            }
            if (TYPE == itp_type_bow) {
                if (id == g_mission->player_id) {
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
            g_game->f1a1f4 = id;
            g_game->f1a1f8 = f8c.item;
            g_game->f1a204 = wield[0];
            {
                GameData* g = g_game;
                g->f1a1fc = (int)slot_mesh[ftoi_helper(g->f1a204)];
            }
            g_script.pos[1] = body->m * *(rgl::mat4*)(body->skel->rec(ik->ammo_skill()));
            ((TimerList*)((char*)ik + 0x228))->update(-0x33);
            }
        } else if (S == 2) {
            if (ACT != f5620) {
                active = 0;
            } else if ((in_flags & 0x130200) ||
                       (ACT == 0x7b && PROG < 0.8f && id == g_mission->player_id && (in_flags & 0xf))) {
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
                if (f2c != -1) {
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
                        links[wield[0]]->vis = 0;
                }
            } else {
                active = 0;
            }
        } else if (S == 4) {
            if (ACT == 0x84 || ACT == 0x80 || ACT == 0x82 || ACT == 0x7e) {
                if (!(PROG > 0.98f))
                    goto tail;
                if (wield[0] >= 0)
                    links[wield[0]]->vis = 0xffff;
                update_quiver();
            }
            goto clear_state;
        }
    }
    }
tail:
    if (in_flags & 0x200) {
        if (f5610 == 0) {
            pick_sound_ids(&f5614, &f5618, &f561c);
            if (f5614 != -1 && (int)(PRIO1 & 0x1ffff) <= 0xf) {
                f5610 = 1;
                active = 0;
                float v = damage_factor(0);
                if (slot_pair(1).item >= 0)
                    v = damage_factor(1);
                set_order(1, f5614, 0x0240000e, v);
                this->cur = 0.0f;
                classify(1);
            }
            goto done;
        }
        if (ACT == f5614 || ACT == f561c || ACT == f5618) {
            if (ACT == f5614 && PROG < 0.99f) {
                if (PROG < 0.5f && g_options.stick_mode != 0) {
                    int a, b, c;
                    pick_sound_ids(&a, &b, &c);
                    if (a != f5614) {
                        f5618 = b;
                        f5614 = a;
                        f561c = c;
                        float v = damage_factor(0);
                        if (slot_pair(1).item >= 0)
                            v = damage_factor(1);
                        float p = PROG;
                        set_order(1, f5614, 0x0240000e, v);
                        { AnimSlot& a1 = anim[1]; a1.phase = p * 0.85f; }
                        active = 0;
                        goto done;
                    }
                }
            } else {
                set_order(1, f5618, 0xe, 1.0f);
            }
            active = 0;
        } else {
            order1(this, -1, 0xf);
        }
    } else {
        f5610 = 0;
        if (f5614 != -1) {
            if (ACT == f5614 || (ACT == f561c && PROG > 0.2f))
                order1(this, -1, 0xf);
        }
    }
done:
    fr2 = fr1;
}

// ---- Agent::move_step: the movement / collision step ----------------------------------------------------------------------------------
struct SceneObjV : shape_base {                         // a scene object returned by the box query
    char pad20[0xa8 - 0x20];
    float fa8;                                          // +0xa8
};
struct PartV { char pad0[0xc]; float f0c; char pad10[0x2c - 0x10]; vector4 v2c; char pad3c[4]; };   // 0x40 bytes: a part of an animation record (AnimDef::subs)
struct Vec3;
void __cdecl bounce(Vec3* out, Vec3* v, const Vec3* n);                    // 0x432e30 (the result is *out)
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
// @ 0x45d820
void Agent::move_step(float dt)
{
    vector4 push(0.0f, 0.0f, 0.0f);
    vector4 v(0.0f, 0.0f, 0.0f);
    vector4 gv(0.0f, 0.0f, 0.0f);
    vector4 np;
    bool hit_ok;
    vector4 hits[16];
    SceneObjV* shapes[8188];
    int nh;
    {
    ::pos m = *(::pos*)&body->m;
    if ((flags3c & 1) && elapsed_inl(t40) >= 0.0f) {
        vector4 a(m.s.x * vel_xy.x, m.s.y * vel_xy.x, m.s.z * vel_xy.x);
        vector4 b(m.f.x * vel_xy.y, m.f.y * vel_xy.y, m.f.z * vel_xy.y);
        push = vector4(a.x + b.x, a.y + b.y, a.z + b.z);
        int fid = ((shape_base*)fbc)->mc;
        if (fid >= 0) {
            SceneEnt* ent = (SceneEnt*)g_mission->scene->find_entity(fid);
            if (ent && ent->is_movable()) {
                vector4 loc = ent->p18.transform_point_to_local(pos);
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
        AnimDef* r = &defs->defs[anim[0].cur];
        if (r->flags & 0x4000) {
            PartV* p = &((PartV*)r->subs)[anim[0].sub];
            float inv = 1.0f / p->f0c;
            gv = mv_vscale(inv, ((const ::pos&)body->m).rot(p->v2c));
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
    camera_params((CamParams*)&cap);
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
        EntVec ents;
        ents.first = 0;
        ents.last = 0;
        ents.end = 0;
        g_mission->scene->collect_boxed(&bmin, &bmax, (std::vector<unsigned>*)&ents);
        for (SceneEnt** it = ents.first; it < ents.last; ++it) {
            SceneEnt* e = *it;
            if (e->shape != 0 && e->type == 0xb && e->id != id && e->id != (unsigned)f2c && e->id != (unsigned)f30) {
                Agent* o = &g_mission->agents.pages[e->id >> 4][e->id & 0xf];
                if (o->f14 == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                    vector4 old = np;
                    bounce((Vec3*)&t, (Vec3*)&np, (const Vec3*)&contact.normal);
                    vector4 rv = t;
                    if (f10 == 1 && vel_xy.y > 4.0f && o->f10 == 0 && o->f2c == -1) {
                        int st = f8c.stat_214();
                        float f = (float)st * 0.03f + 0.1f;
                        if (f30 >= 0)
                            f = (float)g_game->skill_level(g_mission->agents.get(f30)->troop, 0x18) * 0.03f + f;
                        if (f < 0.0f)
                            f = 0.0f;
                        else if (f > 0.96f)
                            f = 0.96f;
                        float g = 1.0f - f;
                        rv.x = rv.x * g;
                        rv.y = rv.y * g;
                        rv.z = rv.z * g;
                        np = (np - old) * g + old;
                    } else if (f10 == 1 && vel_xy.y > 3.0f && o->f10 == 1) {
                        hit_ok = !(rgl::dot(body->m.f, o->body->m.f) > 0.0f) || !(o->vel_xy.y > 0.2f);
                    }
                    if (f10 == 0 && o->f10 == 0 && o->f2c == -1) {
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
                    if (f10 == 1 && vel_xy.y > 3.5f && o->f10 == 0 && o->f2c == -1 && (o->flags3c & 1) &&
                        o->t40.elapsed() >= 0.0f) {
                        if (f30 == -1 || g_mission->agents.get(f30)->is_enemy(o)) {
                            Blow b;
                            b.init();
                            vector4 nn(-contact.normal.x, -contact.normal.y, -contact.normal.z);
                            b.bone = 7;
                            int s2 = f8c.stat_214() + 5;
                            float dmg = (((nn.z * v56a4.z + nn.y * v56a4.y + nn.x * v56a4.x) + 0.1f) * (float)s2 * g_module_settings.horse_charge_damage_multiplier) * vel_xy.y * 0.015f;
                            if (dmg < 0.0f)
                                dmg = 0.0f;
                            b.dmg_in = dmg;
                            b.dir = nn;
                            b.dir2 = nn;
                            b.attacker = id;
                            b.pos = contact.point;
                            b.kind = 2;
                            b.flags = 0x10;
                            o->note_blow(&b);
                        }
                    }
                }
            } else if (e->shape != 0 && e->type == 0) {
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
                    e->collide(&contact.point, &mir);
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
        if (ents.first != 0)
            operator delete(ents.first);
    }
    unsigned mask = 0x8002;
    if (id == (unsigned)g_mission->player_id || !(flags3c & 1))
        mask = 0x8102;
    int count;
    g_mission->scene->query_box(&bmin.x, &bmax.x, mask, (shape_base**)shapes, &count);
    nh = 0;
    float best = 0.0f;
    fc0 = (int)0;
    vector4* hp = hits;
    for (int i = 0; i < count; i++) {
        SceneObjV* sh = shapes[i];
        if (shape_contact_count(&cap, sh, 1, &contact) == 0)
            continue;
        bool ok = true;
        float d = np.y * contact.normal.y + contact.normal.x * np.x + contact.normal.z * np.z;
        if (d > 0.0f) {
            ok = false;
        } else if (shapes[i]->shape_type() == 2 && sh->fa8 < 0.05f && d < best) {
            fc0 = (int)sh;
            best = d;
        }
        if (!(flags3c & 1)) {
            if (contact.point.z < pos.z + 0.4f && np.z <= 0.0f && shapes[i]->shape_type() == 2 && sh->fa8 > 0.1f) {
                flags3c |= 1;
                if (f10 == 1)
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
        if (np.x < g_mission->terrain.min_x)
            np.x = g_mission->terrain.min_x;
        if (np.y < g_mission->terrain.min_y)
            np.y = g_mission->terrain.min_y;
        if (np.x > g_mission->terrain.max_x)
            np.x = g_mission->terrain.max_x;
        if (np.y > g_mission->terrain.max_y)
            np.y = g_mission->terrain.max_y;
        push.x = (np.x - raw.x) + push.x;
        push.y = (np.y - raw.y) + push.y;
        nf = 0;
        tries = 0;
    }
    do {
        ((rgl::world*)scene)->floor_at(&np.x, &fz, &nf);
        face* cur = (face*)fbc;
        if (cur == 0) {
            np = mv_vadd(mv_vscale(0.95f, mv_vsub(np, pos)), pos);
        }
        tries++;
        if (tries > 30) {
            if (nf != 0)
                break;
            nf = cur;
            if (nf == 0)
                nf = (face*)*scene->objs._Myfirst;
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
            ((rgl::world*)scene)->floor_at(&probe.x, &fz2, &tf);
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
            if (id == (unsigned)g_mission->player_id || (!(dz2 > 2.5f) && !(fz < g_mission->terrain.base_z - 1.5f))) {
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
            nf = (face*)fbc;
            np = pos;
        }
    } else if (np.z < fz) {
        np.z = fz;
        flags3c |= 1;
        int snd;
        if (f10 == 1) {
            if (defs->count > 0x19e && ((anim[0].events & 0x1ffff) <= 0x21 || (anim[0].cur == -1 && anim[0].next == -1))) {
                anim[0].next = 0x19e;
                anim[0].events = 0x1000021;
                anim[0].rate = 1.0f;
                if ((defs->defs[0x19e].flags & 0x400) && anim[1].cur != -1 && (anim[1].events & 0x1ffff) < 0x21)
                    anim[1].set(-1, 0x21);
            }
            float var = 0.1f;
            if (fz < g_mission->terrain.base_z) {
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
                if ((defs->defs[3].flags & 0x400) && anim[1].cur != -1 && (anim[1].events & 0x1ffff) < 0x21)
                    anim[1].set(-1, 0x21);
            }
            float var = 0.1f;
            if (fz < g_mission->terrain.base_z) {
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
        if ((f10 == 1 && len2 > 11.0f) || len2 > 13.0f) {
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
            b.bone = (f10 != 1) ? 2 : 9;
            b.kind = 2;
            int kind = 5;
            play_hit_sound(&kind);
            if ((f10 != 0 || f2c != -1) && f10 == 1)
                b.dmg_in = b.dmg_in + b.dmg_in;
            reduce_blow(&b);
            if (has_blow == 0 || !(b.dmg_out < last_blow().dmg_out)) {
                last_blow() = b;
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
                if (f10 == 1 && vel_xy.y > 3.0f && sp < 0.5f && hit_ok) {
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
        fbc = (Obj_a0*)nf;
    }
    impulse = vector4(0.0f, 0.0f, 0.0f);
}

// ---- Agent::update, Agent::tick: the per-frame agent update ---------------------------------------------------------------------------
struct SubV { char pad0[8]; unsigned flags; char padc[0x1c - 0xc]; float ev[4]; char pad2c[0x40 - 0x2c]; };   // 0x40 bytes: a part of an animation (flags: 0x100 footstep dust, 0x200 sound)
struct AnimDefW { char pad0[0x10]; SubV* subs; char pad14[0x30 - 0x14]; };   // g_anim_set.data entry as 0x45f660 reads it
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
        Channel* c = &body->skel->chans[i];
        float p = ((AnimRec*)c)->phase();
        if (c->curve != 1) {
            if (c->curve == 2)
                p = (float)sin(p * 1.5707964f);
            else if (c->curve == 5)
                p = (float)sqrt(p);
            else if (c->curve == 4)
                p = p * p;
            else
                p = 0.0f;
        }
        anim[i].scale = p;
    }
    float now = anim[1].scale;
    if (anim[1].cur >= 0 && anim[1].sub >= 0 && (((AnimDefW*)defs->defs)[anim[1].cur].subs[anim[1].sub].flags & 0x200)) {
        for (int k = 0; k < 4; k++) {
            if (old <= ((AnimDefW*)g_anim_set.data)[anim[1].cur].subs[anim[1].sub].ev[k] &&
                ((AnimDefW*)g_anim_set.data)[anim[1].cur].subs[anim[1].sub].ev[k] < now) {
                if (k == 0) {
                    float pitch;
                    switch (anim[1].cur) {
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_sound_mgr.slot[46], 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_sound_mgr.slot[43], 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    }
                } else if (k == 1) {
                    float pitch;
                    switch (anim[1].cur) {
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.4f + 0.8f;
                        g_sound_mgr.play(g_sound_mgr.slot[44], 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_sound_mgr.slot[45], 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    }
                }
            }
        }
    }
    }
    f56e4 = f56e0;
    f56e0 = anim[0].scale;
    float speed = a_len2(vel_xy);
    if (anim[0].cur >= 0) {
        unsigned flags = ((AnimDefW*)defs->defs)[anim[0].cur].subs[anim[0].sub].flags;
        if (disabled == 0 && (flags & 0x300)) {
            int kind = 0;
            int k = 0;
            float pitch;
            do {
                if (f56e4 <= ((AnimDefW*)g_anim_set.data)[anim[0].cur].subs[anim[0].sub].ev[k] &&
                    ((AnimDefW*)g_anim_set.data)[anim[0].cur].subs[anim[0].sub].ev[k] < f56e0) {
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
                            g_sound_mgr.play(g_sound_mgr.slot[73], 0, (const float*)&body->m.o, pitch, 0);
                        }
                        }
                    }
                    if (flags & 0x100) {
                        if (pos.z < g_mission->terrain.base_z) {
                            if (f10 == 0 || k == 3 || k == 4)
                                play_pain_sound(g_mission->terrain.base_z - pos.z);
                        } else {
                            play_hit_sound(&kind);
                        }
                        if (fx1b0 && speed > 0.0f) {
                            if (pos.z < g_mission->terrain.base_z) {
                                float s = 0.2f;
                                if (f10 == 1)
                                    s = 0.9f;
                                ::pos m;
                                identity_pos((::pos*)&m);
                                a_v2 d;
                                d.x = vel_xy.x;
                                d.y = vel_xy.y;
                                ((vec2*)&d)->normalize();
                                *(a_v2*)&m.o = d * s;
                                m.f.x = vel_xy.x;
                                m.f.y = vel_xy.y;
                                m.f.z = 0.0f;
                                m.f.normalize();
                                m.orthonormalize();
                                fx1b0->m134 = m;
                                fx1b4->m134 = m;
                                float v;
                                if (f10 == 1)
                                    v = speed * 10.0f + 20.0f;
                                else
                                    v = speed * 3.0f + 5.0f;
                                ((Obj3*)fx1b0)->reset_pending((int)(v * 0.5f));
                                ((Obj3*)fx1b4)->reset_pending((int)(v * 0.4f));
                                a_v2 u = d * 0.7f;
                                a_add2(&m.o.x, u);
                                fx1b8->m134 = m;
                                ((Obj3*)fx1b8)->reset_pending(1);
                            } else if (fx1ac) {
                                float v = 0.0f;
                                if (f10 == 1) {
                                    if (speed > 5.5f && fbc && (((shape_base*)fbc)->m8 & 0x4000))
                                        v = speed + 10.0f;
                                    if (fx1ac->tag != g_emit_defs.used[4])
                                        v = v * 3.0f;
                                    ((Obj3*)fx1ac)->reset_pending((int)(v * 0.15f));
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
    if (f30 == -1) {
        if (f7c != 2) {
            ai_idle_tick(&dt);
            poll_input(dt0, (InputState*)&in_flags);
        }
        if (f2c >= 0) {
            unsigned id = f2c;
            Agent* o = g_mission->agents.pages[id >> 4] + (id & 0xf);
            *(InputState*)&o->in_flags = *(InputState*)&in_flags;
        }
    }
    if (dt > 0.0f) {
        process_blow(dt0);
        update_controls(dt0);
        update_attack(dt0);
        if (f2c == -1)
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
        if (f2c >= 0) {
            unsigned id = f2c;
            Agent* o = g_mission->agents.pages[id >> 4] + (id & 0xf);
            pos = o->pos;
            vel = o->vel;
            rot[0] = o->rot[0];
            v56b4 = o->v56b4;
            v56a4 = o->v56a4;
            fbc = o->fbc;
            vel_xy.x = o->vel_xy.x;
            vel_xy.y = o->vel_xy.y;
            flags3c = o->flags3c;
            v98 = o->v98;
        }
        if (f5844 && f5830 == 2 && f30 == -1 &&
            (pos.x - g_mission->terrain.min_x < 1.0f || pos.y - g_mission->terrain.min_y < 1.0f ||
             g_mission->terrain.max_x - pos.x < 1.0f || g_mission->terrain.max_y - pos.y < 1.0f)) {
            f5830 = 3;
            restart_inl(tm);
            draw_name();
        }
        tick_death();
        ragdoll_update();
        if (channel >= 0 && f5870 < ((float)(g_ticks[f5860] - stamp) * 1e-05f))
            channel = -1;
    }
}

