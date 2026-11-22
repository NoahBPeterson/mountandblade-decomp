//-----------------------------------------------------------------------------
// File: agent_attack_checks.cpp  (0x43e190..0x441360)
//
// Desc: font glyph metrics, std::vector / deque instances, std::logic_error family (0x40c2c0..0x40c670), aim point, capsule hit, attack direction, start_shoot, sound ids, can_hit, line of sight, fire check
//       One part of what was game/agent4/agent_logic.cpp (the object's functions in address order; the cut points and the evidence for
//       them are in docs/source_tree.md section 3). The prelude and the helper definitions below are the
//       parts of the old file's prelude and earlier sections this part's code uses.
//-----------------------------------------------------------------------------

#define ANIM_SLOT_SET_OUT_OF_LINE       // 0x4378a0 is a real call here (the body is game/agent/agent_stats.cpp's), written out at two sites of on_wield_changed
#define STD_VECTOR_INT_INSTANTIATED_HERE   // std_vec_out_of_line.h: vector<int>::resize 0x43f310 and operator= 0x43f0a0 are this file's
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
// vector4::out_length 0x407c50: a header inline in the original, left out of line by cl. fire_check's g_mission survives the call, so the body was visible.
inline __declspec(noinline) float rgl::vector4::out_length() const { return length(); }
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

//=============================================================================
// 0x43e190..0x441360 (+ the std::logic_error family, 0x40c2c0..0x40c670), from agent_combat.cpp
// Desc: the game font's glyph metrics, the std::vector / std::deque instantiations the linker kept from this
//       object (tstring, int, E12, E48, PhysRec, PlacedObj, E32 elements), the skeleton accessors, the agent's aim
//       point and capsule hit test, the attack-direction / shoot / sound-id pickers, the "can this hit" test,
//       the agent-table pool and the line-of-sight / fire-check scans. Functions in ascending address order
//       (cl's bottom-up emission). Flags: /O2 /MT /Gy /TP. Names provisional.
//=============================================================================
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
shape_base* g_hit_shape;                                    // 0x5e246c: last shape hit by trace_ray


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
        GlyphMap::iterator it = f->map.lower_bound(key);
        if (it != f->map.end())
            return it->second;
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
// Built WITHOUT an EH switch: the STL try/catch alone makes cl emit the frames (with /EHa cl also keeps the ~PhysRec call
// out of line). The catch(...) funclets and cold tails (0x43e47a, 0x43e514, 0x43e910, 0x43ebc9, 0x43ec91, 0x43edd9,
// 0x43ef97) live inside the COMDAT of the function they belong to.
// @ 0x43e310 (+ 0x43e47a, 0x43e514)
template void std::vector<tstring>::_Insert_n(std::vector<tstring>::iterator, size_t, const tstring&);
// @ 0x43e590
template void std::deque<InputRec>::_Growmap(size_t);
// @ 0x43e6e0
template void std::_Destroy_range(PhysRec*, PhysRec*, std::allocator<PhysRec>&, std::_Nonscalar_ptr_iterator_tag);

// @ 0x43e740
void FxDef::ensure_updated()
{
    if (!pose_valid) {
        if (pose_src)
            update_poses();
        else
            update_poses2();
    }
}

// @ 0x43e760
template bool std::vector<int>::_Buy(size_t);
// (the pointer vectors' _Buy instances are the same code, folded into this one by the original linker: the party manager's and the sprite table's page lists)
template bool std::vector<Party*>::_Buy(size_t);
template bool std::vector<Event*>::_Buy(size_t);
// @ 0x43e7b0 (+ 0x43e910)
template void std::vector<int>::_Insert_n(std::vector<int>::iterator, size_t, const int&);
// @ 0x43ea00 (+ 0x43ebc9, 0x43ec91)
template void std::vector<PlacedObj>::_Insert_n(std::vector<PlacedObj>::iterator, size_t, const PlacedObj&);
// @ 0x43ed20 (+ 0x43edd9): PhysRec's bone vector; its body vector's copy constructor is the same code and the
// original linker folded the two (EH-framed COMDATs fold only among copies kept from one object). vector<int>'s own
// copy constructor is kept from a later object: 0x4cc280 (game/editor/poly_mesh_edge_face_ops.cpp).
template std::vector<const NewtonRagDollBone*>::vector(const std::vector<const NewtonRagDollBone*>&);
// @ 0x43edf0 (+ 0x43ef97)
template void std::vector<E12>::_Insert_n(std::vector<E12>::iterator, size_t, const E12&);
// @ 0x43f0a0
template std::vector<int>& std::vector<int>::operator=(const std::vector<int>&);
// @ 0x43f1f0
template void std::deque<InputRec>::push_back(const InputRec&);

// @ 0x43f260
rgl::mat4* FxDef::rec(int i)
{
    if (!pose_valid) {
        if (pose_src)
            update_poses();
        else
            update_poses2();
    }
    return &elems[i].m84;
}

// @ 0x43f2a0
template void std::vector<tstring>::push_back(const tstring&);
// @ 0x43f310
template void std::vector<int>::resize(size_t, int);
// Mission::prop_ents' resize (MissionV::init_scene 0x5144d0) is the same code from this object, folded into it
template void std::vector<rgl::strategic_entity*>::resize(size_t, rgl::strategic_entity*);
// @ 0x43f3a0
template std::vector<E48>::iterator std::vector<E48>::insert(std::vector<E48>::iterator, const E48&);
// 0x4108b0 is vector<E48>::_Insert_n (an earlier object)
template void std::vector<E48>::_Insert_n(std::vector<E48>::iterator, size_t, const E48&);
// @ 0x43f410
template std::vector<PlacedObj>::iterator std::vector<PlacedObj>::insert(std::vector<PlacedObj>::iterator, const PlacedObj&);
// @ 0x43f480
template void std::vector<PhysRec>::_Destroy(PhysRec*, PhysRec*);
// @ 0x43f4a0
template std::vector<E12>::iterator std::vector<E12>::insert(std::vector<E12>::iterator, const E12&);

// 0x43f510 is PhysRec's implicit copy constructor (allocator<PhysRec>::construct 0x442060 calls it)

// ---- aim point and hit tests ------------------------------------------------------------------------------------
// @ 0x43f560
void Agent::aim_point_and_dir(vector4* pos_out, vector4* dir_out)
{
    FxDef* sk = body->fx;
    if (!sk->pose_valid) {
        if (sk->pose_src)
            sk->update_poses();
        else
            sk->update_poses2();
    }
    const rgl::mat4& r = sk->elems[9].m84;              // the aim bone's world frame
    vector4 t = vadd(vscale(r.f, 0.2f), r.o);
    const matrix& m = body->local;
    *pos_out = to_world(m, t);
    *dir_out = rot_world(body->local, r.u);
}

// @ 0x43f6d0
bool Agent::capsule_hit(shape_base* other, vector4* out_point, vector4* out_a, int* out_bone)
{
    capsule c;
    frame_set* sd = body->fx->res;
    for (int i = 0; i < sd->nbones; i++) {
        for (unsigned j = 0; ; j++) {
            if (j >= sd->bones[i].bodies.size())
                break;
            const CapDef& d = sd->bones[i].bodies[j];
            c.l0 = vmul(d.a, scale);
            c.l1 = vmul(d.b, scale);
            c.radius = d.r * scale;
            FxDef* sk = body->fx;
            if (!sk->pose_valid) {
                if (sk->pose_src)
                    sk->update_poses();
                else
                    sk->update_poses2();
            }
            c.transform((const ::pos*)&(body->local * sk->elems[i].m84));
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
// rgl::matrix::transform_point_to_local (0x407f70, src/engine/physics/collision_shapes.cpp) with its body visible here (inline noinline
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
        int f = horse;
        if (f < 0)
            goto done;
        float v = signed_angle(vel, in_now.move);
        if ((&g_mission->agents.pages[(unsigned)f >> 4][(unsigned)f & 0xf])->vel_xy.y > 1.0f) {
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
        rgl::strategic_entity* tb = t->body;
        if (tb == 0)
            return 0;
        rgl::matrix cam = g_mission->view;
        { const float quarter = -1.5707964f; cam.mat3::rotate_about_s(&quarter); }
        vector4 p = t->pos;
        p.z = p.z + 0.7f;
        if (t->kind == 0)
            p = ((const ::pos&)t->body->local).transform_point(tb->fx->rec(9)->o);
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
        int f = horse;
        if (f < 0)
            goto done;
        float v = signed_angle(vel, in_now.move);
        if ((&g_mission->agents.pages[(unsigned)f >> 4][(unsigned)f & 0xf])->vel_xy.y > 1.0f) {
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
    if (horse >= 0) {
        float v = signed_angle(vel, in_now.move);
        if (!(v < 0.0f)) { dir = 2; } else { dir = 1; }
        alt = 0;
    } else {
        dir = rand() % 4;
        alt = rand() % 4;
        if (dir == alt)
            alt = 3 - dir;
    }
    int tgt = pick_front();
    if (pool_id == g_mission->player_id && (g_options.ctl_mode != 0 || tgt != -1))
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
    int z = -1;
    *a = z;
    *b = z;
    *c = z;
    int mode = 0;
    if (pool_id != g_mission->player_id || g_options.stick_mode == 0) {
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
            rgl::strategic_entity* tb = o->body;
            if (tb != 0) {
                rgl::matrix cam = g_mission->view;
                { const float quarter = -1.5707964f; cam.mat3::rotate_about_s(&quarter); }
                vector4 p = o->pos;
                p.z = p.z + 0.7f;
                if (o->kind == 0)
                    p = ((const ::pos&)o->body->local).transform_point(tb->fx->rec(9)->o);
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

// 0x4378f0 (Agent::get_dir, game/agent/agent_stats.cpp owns the row): the body visible, so the direction pair passed by address does not escape
// in Agent::can_hit (0x4406a0) and the slot_pair temporaries share its frame slot.
inline __declspec(noinline) void Agent::get_dir(vec2* out)
{
    vec2 v;
    v.x = vel.x;
    v.y = vel.y;
    float l = (float)sqrt(v.x * v.x + v.y * v.y);
    if (l > 0.0f) {
        float inv = 1.0f / l;
        v.x = inv * v.x;
        v.y = inv * v.y;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
    }
    if (kind == 0 && horse != -1 && (active != 0 || f5610 != 0)) {
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
// @ 0x4406a0
int Agent::can_hit(unsigned aid, unsigned bid, int alt, Missile* missile, const float* target)
{
    Agent* a = agent_by_id(aid);
    Agent* b = agent_by_id(bid);
    bool strike = missile != 0;
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
    vec2 u, f;
    if (len > 0.0f) {
        float inv = 1.0f / len;
        u.x = inv * dx;
        u.y = inv * dy;
    } else {
        u.x = 0.0f;
        u.y = 1.0f;
    }
    b->get_dir(&f);
    if (strike) {
        if (grp == 199 && f.y * u.y + f.x * u.x < -0.4f) {
            matrix m = b->body->local * *b->body->fx->rec(12);
            vector4 hp = vadd(m.o, vmul(m.f, 0.2f));
            float pz = hp.z;
            float lo = 0.0f, hi = 0.0f;
            ItemRef sp = b->slot_pair_inl(1);
            if (sp.item >= 0) {
                lo = (float)g_items[sp.item].weapon_length * 0.01f;
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
        return id_matches_dir(grp, mode);   // 0x4326d0 (a member that ignores this)
    }
}

// more container instantiations
// @ 0x440a30
template PhysRec* std::_Copy_backward_opt(PhysRec*, PhysRec*, PhysRec*, std::_Nonscalar_ptr_iterator_tag);
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
template<> int Pool<Missile>::alloc()
{
    if (!(free_ids.size() > 0)) {
        Missile* blk = new Missile[16];
        pages.push_back(blk);
        for (int j = 0; j < 16; j++) {
            blk[j].in_use = 0;
            blk[j].pool_id = count + j;
            free_ids.push_back(count + j);
        }
        count += 16;
    }
    unsigned id = free_ids.back();
    free_ids.pop_back();
    Missile* r = &pages[id >> 4][id & 0xf];
    r->in_use = 1;
    r->serial = serial;
    serial++;
    return id;
}

// @ 0x440dc0
template void std::fill(PhysRec*, PhysRec*, const PhysRec&);

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
        vector4 head = ((const ::pos&)o->body->local).transform_point(o->body->fx->elems[9].m84.o);
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
            r = shape_contact_count(&c, ignore, 0, 0) != 0;
            if (r)
                return false;
        }
        if (dp > -0.3f || len < 2.0f) {
            r = g_mission->scene->trace_ray(&p, &head, 400.0f, &g_hit_shape, 1) != 0;
            return r;
        }
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
    if (horse != -1) {
        float v = signed_angle(vel, ai.move_dir);
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
            if (o->kind == 0 && mission_friends(g_mission, team, o->team) && o->state == 1 && o->pool_id != pool_id) {
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
