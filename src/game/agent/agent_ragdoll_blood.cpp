//-----------------------------------------------------------------------------
// File: agent_ragdoll_blood.cpp  (0x44f5d0..0x452c00)
//
// Desc: Font::draw_glyph, ranged-weapon mesh, placed-object append, Newton world creation, start_ragdoll, dust, blood, mesh normals, text layout, carried-item refresh, drop_held
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
static inline vector4 vscale(const vector4& v, float k) { return vector4(v.x * k, v.y * k, v.z * k); }
// Which of the four attack directions (1 left, 2 right, 3 up?, 0 thrust) the agent picks; derived from the
// mouse movement (mode > 0) or from where the target stands relative to the view (mode 0).
// rgl::matrix::transform_point_to_local (0x407f70, src/engine/physics/collision_shapes.cpp) with its body visible here (inline noinline
// duplicate, as map_panel.cpp): its argument does not escape
inline __declspec(noinline) vector4 rgl::matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
}
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
static __forceinline vector4 cross3(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - a.z * b.y; r.y = a.z * b.x - a.x * b.z; r.z = a.x * b.y - a.y * b.x;
    return r;
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
#include "game/agent/combat/agent_fx_sound.h"   // after the 0x43e190.. section: its itcf_* macros are i64, that section's enum is int
// 0x440b30 vector<int>::push_back (agent_combat.cpp) is kept out of line


#include "engine/core/std_vec_insert_n.h"      // vector<rgl::mesh*>::_Insert_n 0x410610 (declared only: the call is out of line)
// ---- canonical classes / forwarders (see agent_logic.h) ----------------------------------------------------------------------------

// inline vector helpers of the original object
static inline vector4 a_vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static inline vector4 a_vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
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
float frand();                                                          // 0x40e400
using rgl::mesh;
using rgl::mat4;
#define itcf_carry_mask 0x00000007f0000000i64
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
// ---- Font::draw_glyph, the carried ranged item mesh, add_placed, the Newton world ---------------------------------------------------
// the font glyph record: the slices read it as {u0, v0, u1, v1, xoff, yoff, xadv}; font.h's Glyph has the same seven words
// (key, f4, f8, fc, f10, f14, f18), the key word being the left u of an ascii entry
static __forceinline const Glyph* font_get(Font* f, int c)
{
    if (c >= 0x200) {
        GlyphMap::iterator it = f->map.lower_bound(c);
        if (it != f->map.end())
            return &it->second;
        return &f->dflt;
    }
    return &f->ascii[c];
}

// @ 0x44f5d0
void Font::draw_glyph(int ch, const vector4* pos, const vec2* scale, mesh* m, const unsigned* color)
{
    const Glyph* g = font_get(this, ch);
    float dv = g->hi.y - g->lo.y;
    vector4 a = *pos;
    vec2 s;                            // the glyph size as a two-float struct: its lanes are field views, not substituted temporaries
    s.y = dv * scale->y;
    a.y = (g->f14 * scale->y - s.y) + a.y;
    s.x = (g->hi.x - g->lo.x) * scale->x;
    vector4 b(a.x + s.x, a.y + s.y, a.z);
    m->add_quad(&a, &b, &g->lo, &g->hi, color);
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
// 0x40e670 / 0x40e770 (engine/physics/collision_shapes.cpp owns the rows): the bodies visible, so the angle passed to them does not escape
static __forceinline vector4 rm_mul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 rm_add(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static __forceinline vector4 rm_sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
inline __declspec(noinline) void rgl::mat3::rotate_about_f(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nu;
    ns = rm_sub(rm_mul(s, cos((double)*angle)), rm_mul(u, sn));
    nu = rm_add(rm_mul(u, cos((double)*angle)), rm_mul(s, sn));
    s = ns;
    u = nu;
}
inline __declspec(noinline) void rgl::mat3::rotate_about_u(const float* angle)
{
    double sn = sin((double)*angle);
    vector4 ns, nf;
    ns = rm_add(rm_mul(s, cos((double)*angle)), rm_mul(f, sn));
    nf = rm_sub(rm_mul(f, cos((double)*angle)), rm_mul(s, sn));
    s = ns;
    f = nf;
}

// 0x4351f0 (game/agent/agent_queries.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) ItemRef Missile::ranged_ref()      // 0x4351f0
{
    ItemRef r;
    r.item = -1;
    if (launcher.item >= 0) {
        if ((char)g_items[launcher.item].type_flags == itp_type_thrown)
            r = launcher;
        else
            r = ammo;
    }
    return r;
}

// @ 0x44f6a0
mesh* Missile::make_ranged_mesh()
{
    mesh* result = 0;
    ItemKind* items = g_items;
    int i = -1;
    int a = launcher.item;
    if (a >= 0) {
        if ((char)items[a].type_flags == 10)
            i = a;
        else
            i = ammo.item;
    }
    ItemRef r = ranged_ref();

    if (i >= 0) {
        mesh* obj = (mesh*)get_obj(item_mesh_name(g_items, r));
        float s = (float)g_items[i].weapon_length * 0.01f;
        if (obj) {
            result = obj->clone();
            vector4 d(0.0f, -s, 0.0f);
            result->translate(&d);
            rgl::matrix rot;
            identity_pos(&rot);
            float angle = 1.5707964f;
            rot.rotate_about_f(&angle);
            if ((char)g_items[i].type_flags == 10) {
                if ((g_items[i].flags & ITEM_CLASS_MASK) == ITEM_CLASS_C) {
                    angle = frand() * 0.4f + 0.5f;
                    rot.rotate_about_u(&angle);
                }
                if ((g_items[i].flags & ITEM_CLASS_MASK) == ITEM_CLASS_B) {
                    angle = frand() * 0.4f + 3.2415926f;
                    rot.rotate_about_u(&angle);
                }
            }
            result->transform(*(mat4*)&rot);
        }
    }
    return result;
}

#include "game/mission/placed_obj.h"      // PlacedObj (Mission::props is a std::vector<PlacedObj>)

// @ 0x44f870
int Mission::add_placed(const int* kind, const int* id, const int* value, const matrix* m, int* spawn_id)
{
    int cnt = 0;
    int prev = -1;
    int idx = (int)props.size();
    {
        PlacedObj rec;                                            // its inlined constructor calls reset (0x4352f0, defined as PlacedObj::reset in agent_queries.cpp): shared rec1c8.h
        props.push_back(rec);       // 0x440b80 (the explicit instantiation of agent_combat.cpp)
        ::init_placed(&props[idx], kind, id, value, m);
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

// @ 0x44fa40
template void std::vector<PhysRec>::push_back(const PhysRec&);

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
        PhysRec r;
        r.world = w;
        recs.push_back(r);                                          // 0x44fa40
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

    rgl::strategic_entity* b = body;
    frame_set* sd = b->fx->res;
    int n = sd->nbones;
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
    FxElem* regs = b->fx->elems;
    for (i = 0; i < n; i++) {
        float px, py, pz;
        {
            const vector4& o = regs[i].m84.o;
            px = b->local.f.x * o.y + b->local.u.x * o.z + o.x * b->local.s.x + b->local.o.x;
            py = b->local.f.y * o.y + b->local.u.y * o.z + b->local.s.y * o.x + b->local.o.y;
            pz = b->local.f.z * o.y + b->local.u.z * o.z + b->local.s.z * o.x + b->local.o.z;
        }
        int j = sd->bones[i].parent;
        if (j >= 0) {
            FxElem* regs2 = b->fx->elems;
            Bone* items = sd->bones;
            Bone* jr = &items[j];
            do {
                float qx = b->local.f.x * regs[j].m84.o.y + b->local.u.x * regs[j].m84.o.z + b->local.s.x * regs[j].m84.o.x + b->local.o.x;
                float qy = b->local.f.y * regs[j].m84.o.y + b->local.s.y * regs[j].m84.o.x + b->local.u.y * regs[j].m84.o.z + b->local.o.y;
                float qz = b->local.f.z * regs[j].m84.o.y + b->local.s.z * regs[j].m84.o.x + b->local.u.z * regs[j].m84.o.z + b->local.o.z;
                float rx = px - qx;
                float ry = py - qy;
                float rz = pz - qz;
                float lx = regs2[j].m84.f.x * regs2[j].vf4.y + regs2[j].m84.u.x * regs2[j].vf4.z + regs2[j].m84.s.x * regs2[j].vf4.x;     // 0x4500ac: (f*wy + u*wz) + s*wx
                float ly = regs2[j].m84.f.y * regs2[j].vf4.y + regs2[j].m84.u.y * regs2[j].vf4.z + regs2[j].m84.s.y * regs2[j].vf4.x;
                float lz = regs2[j].m84.f.z * regs2[j].vf4.y + regs2[j].m84.u.z * regs2[j].vf4.z + regs2[j].m84.s.z * regs2[j].vf4.x;
                float vx = lx * b->local.s.x + lz * b->local.u.x + ly * b->local.f.x;
                float vy = lx * b->local.s.y + lz * b->local.u.y + ly * b->local.f.y;
                float vz = lx * b->local.s.z + lz * b->local.u.z + ly * b->local.f.z;
                float cx = vy * rz - vz * ry;
                float cz = vx * ry - rx * vy;
                vacc[i].x = cx + vacc[i].x;
                vacc[i].y = (rx * vz - vx * rz) + vacc[i].y;
                vacc[i].z = cz + vacc[i].z;
                wacc[i].x = vx + wacc[i].x;
                wacc[i].y = vy + wacc[i].y;
                wacc[i].z = vz + wacc[i].z;
                // the next ancestor that has capsules. The original steps to items[-1] when the chain ends and tests whatever
                // bytes precede the array (the allocator's header: non-empty there, so it falls out with j = -1); at 64 bits those
                // bytes are zero and the loop never ended, so the chain's end (parent < 0) ends it here.
                do {
                    j = jr->parent;
                    if (j < 0)
                        break;
                    jr = &items[j];
                } while (jr->bodies.size() == 0);
            } while (j >= 0);
        }
    }

    // create the bones
    for (i = 0; i < n; i++) {
        Bone* rec = &sd->bones[i];
        int p = rec->parent;
        matrix T;
        if (p < 0) {
            W[i] = b->local * b->fx->elems[i].m84;
            T = b->local * rec->local;
            A[i] = T;
        } else {
            W[i] = ((rgl::matrix&)W[p]) * b->fx->elems[i].m44;
            matrix cur = rec->local;
            int j = p;
            for (;;) {
                if (sd->bones[j].bodies.size() != 0)
                    break;
                cur = sd->bones[j].local * cur;
                j = sd->bones[j].parent;
            }
            T = cur;
            float a0 = rec->rotate.x * 0.017453292f;
            T.rotate_about_s(&a0);
            float a1 = rec->rotate.y * 0.017453292f;
            T.rotate_about_f(&a1);
            float a2 = rec->rotate.z * 0.017453292f;
            T.rotate_about_u(&a2);
            A[i] = ((rgl::matrix&)A[j]) * T;
        }
        if (rec->bodies.size() != 0) {
            float radius = 0.0f, height = 0.01f;
            matrix R;
            vector4 z0(0.0f, 0.0f, 0.0f);
            rgl::mat3 t;
            rgl::mat3* ip = identity3(&t);
            static_cast<mat3&>(R) = *ip;
            R.o = z0;
            for (unsigned k = 0; k < rec->bodies.size(); k++) {
                const CapDef& d = rec->bodies[k];
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
                if (b->fx->elems[q].ragdoll_bone == 0) {
                    do {
                        q = sd->bones[q].parent;
                    } while (b->fx->elems[q].ragdoll_bone == 0);
                }
                pbone = b->fx->elems[q].ragdoll_bone;
            }
            vector4 cone;
            rot3v_cone(cone, A[i], rec->socket_dir.x, rec->socket_dir.y, rec->socket_dir.z);
            b->fx->elems[i].ragdoll_bone = g_phys.add_bone(phys_idx, pbone, &rec->bone_mass, T, &radius, &height, R, &cone.x,
                                                     &rec->cone_min, &rec->cone_max, &rec->twist_limit);
            v = v56a4;
            link_vel = vector4(0.0f, 0.0f, 0.0f);
            if ((unsigned)horse != (unsigned)-1) {
                const Agent* other = &g_mission->agents.pages[(unsigned)horse >> 4][(unsigned)horse & 0xf];
                link_vel = other->v56a4;
                v = vector4(0.0f, 0.0f, 0.0f);
            }
            vector4 rv;
            rot3v_x(rv, b->local, b->fx->root_vel.x, b->fx->root_vel.y, b->fx->root_vel.z);
            v.x = rv.x + v.x;
            v.y = rv.y + v.y;
            v.z = v.z + rv.z;
            v.x = v.x + vacc[i].x;
            v.y = v.y + vacc[i].y;
            v.z = v.z + vacc[i].z;
            NewtonBody* nb = NewtonRagDollBoneGetBody(b->fx->elems[i].ragdoll_bone);
            FxElem* e = &b->fx->elems[i];
            vector4 wl;
            rot3v_x(wl, e->m84, e->vf4.x, e->vf4.y, e->vf4.z);
            rot3v_w(w, b->local, wl.x, wl.y, wl.z);
            w.x = w.x + wacc[i].x;
            w.y = w.y + wacc[i].y;
            w.z = w.z + wacc[i].z;
            NewtonBodySetVelocity(nb, &v.x);
            NewtonBodySetOmega(nb, &w.x);
        }
    }

    NewtonRagDollSetForceAndTorqueCallback(g_phys.recs[phys_idx].ragdoll, apply_force_and_torque);
    NewtonRagDollEnd(g_phys.recs[phys_idx].ragdoll);
    for (i = 0; i < sd->nbones; i++) {
        if (body->fx->elems[i].ragdoll_bone != 0)
            NewtonBodySetMatrix(NewtonRagDollBoneGetBody(body->fx->elems[i].ragdoll_bone), &W[i].s.x);
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
        static_cast<mat3&>(R) = *ip;
        float radius = 0.2114f * h->scale;
        vector4 dir(0.0f, 1.0f, 0.0f);                       // 0x45117c: the original folds the unit axis and the 1.2886 length
        float height = 1.2886f * h->scale;
        make_frame(R, dir);
        R.o = vector4(0.0f * h->scale, 0.5443f * h->scale, -0.0629f * h->scale);
        matrix m0 = h->body->local * h->body->fx->elems->m84;
        float zero = 0.0f;
        mount_body = g_phys.add_capsule_body(phys_idx, &radius, &height, &zero, &m0, &R, false);
    }
    disabled = 1;
    floor_sound_done = 0;
    mount_sync = 1;
    mount_vel_sync = 1;
    body->fx->pose_src = 1;
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
                m->vertices[i].color = rgl::pack_color(&o);
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
        body->fx->all_groups(shapes, &n);
        for (int s = 0; s < n; s++) {
            rgl::lod_mesh* L = shapes[s];
            if (L && L->mask && L->ready()) {
                int cur = L->cur;
                for (int k = 0; k < L->count; k++) {
                    std::vector<mesh*>& list = L->lists[k];
                    for (unsigned j = 0; j < list.size(); j++) {
                        mesh** slot = &(&*list.begin())[j];
                        if (*slot) {
                            rgl::matrix xf = body->local;
                            if (cur == k) {
                                xf = (&*L->lists[cur].begin())[j]->world_frame;
                            } else {
                                std::vector<mesh*>& cl = L->lists[cur];
                                if (cl.size() != 0)
                                    xf = (*cl.begin())->world_frame;
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
            z += o.z;
        }
    };
    if (!local_faces) {
        faces = parent->faces;
        local_faces = 1;
    }
    ensure_local();
    std::vector<vector4> fnorm;
    fnorm.resize(faces.size());
    unsigned i;
    for (i = 0; i < faces.size(); i++) {
        vector4 e1 = points[vertices[faces[i].b].index].a - points[vertices[faces[i].a].index].a;
        vector4 e2 = points[vertices[faces[i].c].index].a - points[vertices[faces[i].a].index].a;
        cross_inl(fnorm[i], e1, e2);
        ((vec4n&)fnorm[i]).normalize_inl();
    }
    for (i = 0; i < vertices.size(); i++) {
        vertices[i].normal = vector4(0.0f, 0.0f, 0.0f);
    }
    for (i = 0; i < faces.size(); i++) {
        for (int j = 0; j < 3; j++) {
            ((vec4n&)vertices[(&faces[i].a)[j]].normal).add_inl(fnorm[i]);
        }
    }
    for (i = 0; i < vertices.size(); i++) {
        if (vertices[i].normal.length_squared() > 0.0001f) {
            ((vec4n&)vertices[i].normal).normalize_inl();
        } else {
            vertices[i].normal = vector4(0.0f, -1.0f, 0.0f);
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
                ch = ((((((unsigned char)s->buf[i + 0] & 7) << 6) | ((unsigned char)s->buf[i + 1] & 0x3f)) << 6 | ((unsigned char)s->buf[i + 2] & 0x3f)) << 6) | ((unsigned char)s->buf[i + 3] & 0x3f);
                i += 4;
            } else if (c >= 0xe0 && c < 0xf0) {
                ch = (((((unsigned char)s->buf[i + 0] & 0xf) << 6) | ((unsigned char)s->buf[i + 1] & 0x3f)) << 6) | ((unsigned char)s->buf[i + 2] & 0x3f);
                i += 3;
            } else if (c >= 0xc2 && c < 0xe0) {
                ch = ((((unsigned char)s->buf[i + 0] & 0x1f) << 6) | ((unsigned char)s->buf[i + 1] & 0x3f));
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
void Font::layout_text(float* out, const tstring* text, const vector4* pos_in, const float* scale_in, mesh* m,
                       const unsigned* color, const float* xmin, const float* xmax, unsigned flags)
{
    int buf[8192];
    int len;
    vector4 pos = *pos_in;
    if (flags & TXT_START_X)
        pos.x = *xmin;
    vec2 scale;
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
                        pos.x = scale.x * g->f10 + pos.x;
                        draw_glyph(ch, &pos, &scale, m, color);
                        pos.x = scale.x * g->f18 + pos.x;
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
        std::vector<rgl::mesh*>& v = g->lists[k];
        for (std::vector<rgl::mesh*>::iterator it = v.begin(); it < v.end(); ++it)
            skin(*it, flag);
    }
}
// @ 0x4525a0
void Mission::blood_burst(float x, float y, float z, int unused, float amount)
{
    if (g_cfg.enable_blood) {
        // apply_blood reads the burst centre as a vector4 (x, y, z, w): the original hands it the address of its own four
        // consecutive argument words; here they are gathered into one
        vector4 c(x, y, z);
        memcpy(&c.w, &unused, sizeof c.w);
        unsigned i = agents.first_used();
        if (i < (unsigned)agents.count) {
            do {
                Agent* a = &agents.pages[i >> 4][i & 0xf];
                float dx = a->pos.x - x;
                float dy = a->pos.y - y;
                float dz = a->pos.z - z;
                if ((float)sqrt(dx * dx + dy * dy + dz * dz) < 3.5f)
                    a->apply_blood(&c.x, amount);
                agents.next_used(&i);
            } while (i < (unsigned)agents.count);
        }
    }
}
// rgl::matrix::relative (0x4329d0, src/game/common/game_util.cpp) visible here too (inline noinline duplicate): with both bodies refresh_carried's rel does not escape, so the target-mesh load is hoisted
// above the rel.o.y store as in the original
inline __declspec(noinline) rgl::matrix rgl::matrix::relative(const rgl::matrix& b) const
{
    rgl::matrix m;
    m.o = transform_point_to_local(b.o);
    static_cast<rgl::mat3&>(m) = mul_t(b);
    return m;
}
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
    mesh* m = other->make_ranged_mesh();
    if (m == 0)
        return;
    if (B[slot] == 0) {
        B[slot] = new rgl::lod_mesh;
        mesh* nm = new rgl::mesh;
        nm->res = m->res;
        B[slot]->add_mesh(0, nm);
        body->fx->attach_group(0x13, B[slot]);      // the animator at body+0x304 (fx.h attach_group 0x4424a0)
        update_equipment_display();
    }
    rgl::lod_mesh* L = B[slot];
    if (L) {
        if ((char)g_items[other->launcher.item].type_flags == 8)
            m->scale(0.87f);
        mat4 M;
        M = body->local * *body->fx->rec(0xc);
        // the frame block is its own scope (fr then shares the operator* return temporary's slot: frame 0xe0)
        {
            rgl::mat4 fr;
            fr.o = other->pos;
            fr.f = other->vel;
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
            rel.o.z = 0.05f - (frand() * 0.01f + other->speed * 0.0005f);
            if (rel.o.x < -0.45f)
                rel.o.x = -0.45f;
            else if (rel.o.x > 0.2f)
                rel.o.x = 0.2f;
            rel.o.x = (frand() - 0.5f) * 0.02f + rel.o.x;
            rel.o.y = (frand() - 0.5f) * 0.02f + rel.o.y;
            L->lists[0][0]->append_plain(m, (const mat4*)&rel);
        }
    }
    delete m;
}

// ---- Agent::drop_held, the text layout wrapper, the message log, the animator visibility switch, Agent::get_label_box -----------
extern Font* g_font;                                    // 0x5e2934
// the animator's inline accessor of a bone record's world frame (brings the poses up to date first)
static __forceinline rgl::mat4* skel_rec_inl(FxDef* sk, int i)
{
    if (!sk->pose_valid) {
        if (sk->pose_src)
            sk->update_poses();
        else
            sk->update_poses2();
    }
    return &sk->elems[i].m84;
}
static __forceinline tstring* lk(const tstring& k) { return g_strtable.lookup(k); }
static __forceinline skin* get_mat(const tstring& n) { return g_resmgr.get_material(n); }

// @ 0x4529e0
void Agent::drop_held()
{
    int i = 0;
    ItemRef* it = pairs;
    rgl::lod_mesh** lp = B;
    for (; i < 4; i++, lp++, it++) {
        if (it->item >= 0) {
            int v = rand() % 15817;
            if ((float)v * (1.0f / 15817.0f) < 0.25f) {
                ItemKind* kind = &g_items[it->item];
                rgl::matrix M;
                identity_pos(&M);
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
                FxDef* sk = body->fx;
                mat4 T1 = body->local * *skel_rec_inl(sk, bone);
                mat4 T2 = T1 * *(mat4*)&M;
                T2.o.z += 0.1f;
                if (lp[-4])
                    lp[-4]->mask = 0;
                if (lp[0])
                    lp[0]->mask = 0;
                if (lp[4])
                    lp[4]->mask = 0;
                v = 6;
                g_mission->add_placed(&v, &it->item, &it->word, (rgl::matrix*)&T2, &pool_id);
                it->item = -1;
            }
        }
    }
}
// @ 0x452bb0
rgl::vector4* Font::layout_clear(rgl::vector4* out_extent, const tstring* text, const rgl::vector4* pos, const float* scale, rgl::mesh* m,
                               const int* a, const float* b, const float* c, unsigned flags)
{
    m->clear_geometry();
    layout_text(&out_extent->x, text, pos, scale, m, (const unsigned*)a, b, c, flags);
    return out_extent;
}
