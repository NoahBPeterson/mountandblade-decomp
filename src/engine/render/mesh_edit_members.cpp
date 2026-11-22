//-----------------------------------------------------------------------------
// File: mesh_edit_members.cpp  (0x44e970..0x44f5d0)
//
// Desc: rgl::mesh members: add_quad, append_skinned, scale, translate, transform
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
#define itp_two_handed 0x200000
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
// rgl::aabb::extend_box (0x4201b0, src/engine/scene/entity_render.cpp) with its body visible here (inline noinline
// duplicate, as transform_point_to_local above): the matrix append_skinned / append_plain receive does not escape
inline __declspec(noinline) void rgl::aabb::extend_box(const aabb* b, const rgl::matrix* m)
{
    if (b->min.x > b->max.x)
        return;
    {
        float c[8][4];
        c[0][0] = b->min.x; c[0][1] = b->min.y; c[0][2] = b->min.z;
        c[1][0] = b->max.x; c[1][1] = b->min.y; c[1][2] = b->min.z;
        c[2][0] = b->min.x; c[2][1] = b->max.y; c[2][2] = b->min.z;
        c[3][0] = b->max.x; c[3][1] = b->max.y; c[3][2] = b->min.z;
        c[4][0] = b->min.x; c[4][1] = b->min.y; c[4][2] = b->max.z;
        c[5][0] = b->max.x; c[5][1] = b->min.y; c[5][2] = b->max.z;
        c[6][0] = b->min.x; c[6][1] = b->max.y; c[6][2] = b->max.z;
        c[7][0] = b->max.x; c[7][1] = b->max.y; c[7][2] = b->max.z;
        for (int i = 0; 8 > i; i++) {
            float x = c[i][1] * m->f.x;
            vector4 p;
            x = x + c[i][2] * m->u.x;
            x = x + c[i][0] * m->s.x;
            p.x = x + m->o.x;
            float y = c[i][2] * m->u.y;
            y = y + c[i][0] * m->s.y;
            y = y + c[i][1] * m->f.y;
            p.y = y + m->o.y;
            float z = m->s.z * c[i][0];
            z = z + c[i][1] * m->f.z;
            z = z + c[i][2] * m->u.z;
            p.z = z + m->o.z;
            extend(&p);
        }
    }
}

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

// @ 0x44eb00
void rgl::mesh::append_skinned(mesh* other, const mat4* m, int bone, unsigned col, char flip)
{
    if (!local_faces) {
        faces = parent->faces;
        local_faces = 1;
    }
    ensure_local();
    int npts = (int)points.size();
    int nverts = (int)vertices.size();
    if (other->nbones > nbones) nbones = other->nbones;
    if (bone + 1 > nbones) nbones = bone + 1;
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
        if (skin_verts.size() < vertices.size())
            build_skin();
        for (unsigned i = 0; i < sk.size(); i++) {
            skin_vert e = sk[i];
            skin_verts.push_back(e);
        }
    } else {
        if (bone >= 0) {
            if (skin_verts.size() < vertices.size())
                build_skin();
            for (unsigned i = 0; i < vs.size(); i++) {
                skin_vert e;
                e.w[0] = 1.0f; e.w[3] = 0.0f; e.w[2] = 0.0f; e.w[1] = 0.0f;
                e.bone[0] = bone; e.bone[3] = 0; e.bone[2] = 0; e.bone[1] = 0;
                e.bonef[0] = (float)bone; e.bonef[3] = 0; e.bonef[2] = 0; e.bonef[1] = 0;
                skin_verts.push_back(e);
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
        vector4 c;
        c = rgl::unpack_color(&v.color);
        c.x = c.x * tint.x; c.y = c.y * tint.y; c.z = c.z * tint.z; c.w = c.w * tint.w;
        v.color = rgl::pack_color(&c);
        v.f58 = v.color;
        vertices.push_back(v);
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
    for (unsigned i = 0; i < points.size(); i++) {
        point20* p = &(&*points.begin())[i];
        p->a.x = s * p->a.x;
        p->a.y = s * p->a.y;
        p->a.z = s * p->a.z;
        p = &(&*points.begin())[i];
        p->b.x = s * p->b.x;
        p->b.y = s * p->b.y;
        p->b.z = s * p->b.z;
    }
    for (unsigned j = 0; j < (unsigned)kf.count; j++) {
        for (std::vector<vector4>::iterator q = kf.frames[j].verts.begin(); q != kf.frames[j].verts.end(); q++) {
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
    for (unsigned i = 0; i < points.size(); i++) {
        vector4& a = points[i].a;
        a += *d;
        points[i].b += *d;
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
        GlyphMap::iterator it = f->map.lower_bound(c);
        if (it != f->map.end())
            return &it->second;
        return &f->dflt;
    }
    return &f->ascii[c];
}

// @ 0x44f3c0
void rgl::mesh::transform(mat4 m)
{
    ensure_local();
    unsigned i;
    for (i = 0; i < points.size(); i++) {
        vector4 ta, tb;
        point20* p = &(&*points.begin())[i];
        ta.x = m.u.x * p->a.z + m.f.x * p->a.y + m.s.x * p->a.x + m.o.x;
        ta.y = m.u.y * p->a.z + m.f.y * p->a.y + m.s.y * p->a.x + m.o.y;
        ta.z = m.u.z * p->a.z + m.f.z * p->a.y + m.s.z * p->a.x + m.o.z;
        p->a = ta;
        p = &(&*points.begin())[i];
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
    for (i = 0; i < vertices.size(); i++) {
        vector4 tb;
        vertex60* v = &(&*vertices.begin())[i];
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
