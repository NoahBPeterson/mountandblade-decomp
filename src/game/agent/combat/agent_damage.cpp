//-----------------------------------------------------------------------------
// File: agent_damage.cpp  (0x452c00..0x4554c0)
//
// Desc: message log add, label box, shield_hit, hit_damage, on_removed, ragdoll_update, reduce_blow, draw_name
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
static __forceinline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }
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
// Which of the four attack directions (1 left, 2 right, 3 up?, 0 thrust) the agent picks; derived from the
// mouse movement (mode > 0) or from where the target stands relative to the view (mode 0).
// rgl::matrix::transform_point_to_local (0x407f70, src/engine/physics/collision_shapes.cpp) with its body visible here (inline noinline
// duplicate, as map_panel.cpp): its argument does not escape
inline __declspec(noinline) vector4 rgl::matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
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
inline __declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)     // 0x40e3d0
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
float frand();                                                          // 0x40e400
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
// ---- Mission::spawn_missile ---------------------------------------------------------------------------------
// the missile record of an id: pages of 16 records of 0xb0 bytes at Mission+0x30 (Pool<Missile>, engine/core/pool.h)
static __forceinline Agent& mag(Mission* m, unsigned id) { return m->agents.pages[id >> 4][id & 0xf]; }
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
#include "game/mission/placed_obj.h"      // PlacedObj (Mission::props is a std::vector<PlacedObj>)
// rgl::matrix::relative (0x4329d0, src/game/common/game_util.cpp) visible here too (inline noinline duplicate): with both bodies refresh_carried's rel does not escape, so the target-mesh load is hoisted
// above the rel.o.y store as in the original
inline __declspec(noinline) rgl::matrix rgl::matrix::relative(const rgl::matrix& b) const
{
    rgl::matrix m;
    m.o = transform_point_to_local(b.o);
    static_cast<rgl::mat3&>(m) = mul_t(b);
    return m;
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
static __forceinline skin* get_mat(const tstring& n) { return g_resmgr.get_material(n); }

// @ 0x452c00
void gui_manager::add(tstring msg, int color)
{
    rgl::matrix M;
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
            mat4* id = (mat4*)identity_pos(&M);
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
void FxDef::set_visible(bool on)
{
    if (!frozen) {
        if (on) {
            if (!pose_valid) {
                if (pose_src)
                    update_poses();
                else
                    update_poses2();
            }
            for (unsigned i = 0; i < psys.size(); i++)
                skin(psys[i], 1);
            for (unsigned j = 0; j < groups.size(); j++)
                skin_lods(groups[j], 1);
        }
    }
    frozen = on;
}

// Mission::log_message: the caller loads g_mission into ecx and the body ignores it (ret 8). agent_obj/mission.h is shared, so a stub class names the member here.
// @ 0x453020
void Mission::log_message(const tstring* s, float seconds)
{
    g_gui.add(*s, -261);
}

// rgl::matrix::transform_point (0x408460, src/engine/physics/collision_shapes.cpp) with its body visible from here on (inline noinline
// duplicate): its hidden return temporary does not escape, so get_label_box's third result shares p2's dead slot (frame 0x20)
inline __declspec(noinline) vector4 rgl::matrix::transform_point(const vector4& p) const
{
    return vector4(s.x * p.x + f.x * p.y + u.x * p.z + o.x,
                   s.y * p.x + f.y * p.y + u.y * p.z + o.y,
                   s.z * p.x + f.z * p.y + u.z * p.z + o.z);
}
// box above the agent's head (two corner vectors and a scale), for its label: the capsule's segment and radius
// @ 0x453050
void Agent::get_label_box(capsule* out)
{
    if (kind != 0) {
        out->p0 = vector4(pos.x, pos.y - 0.3f, pos.z + 0.8f);
        out->p1 = vector4(pos.x, pos.y + 0.8f, pos.z + 0.85f);
        out->radius = 0.5f;
    } else if (body != 0) {
        vector4 p1 = body->local.transform_point(body->fx->elems[3].m84.o);
        vector4 p2 = body->local.transform_point(body->fx->elems[6].m84.o);
        p1.x = p1.x + p2.x;
        p1.y = p1.y + p2.y;
        p2.z = p1.z + p2.z;
        p1.x = p1.x * 0.5f;
        p1.y = p1.y * 0.5f;
        p1.z = p2.z * 0.5f;
        out->p0 = p1;
        out->p1 = body->local.transform_point(body->fx->elems[9].m84.o);
        out->radius = scale * 0.33f;
    } else {
        g_gui.add(tstring("WARNING:CHECK INVALID ENTITY"), -1);
        // one vector built in each arm: cl hoists the common x / y / pos.z part above the f2c test (the original's order)
        vector4 c;
        if (horse == -1)
            c = vector4(pos.x, pos.y, pos.z + 0.8f);
        else
            c = vector4(pos.x, pos.y, pos.z + 1.5f);
        out->p0 = vector4(c.x, c.y, c.z - 0.5f);
        out->p1 = vector4(c.x, c.y, c.z + 0.5f);
        out->radius = 0.4f;
    }
}

// ---- Agent::shield_hit, Agent::on_removed ---------------------------------------------------------------------------------------------
#define itp_type_shield 7

// 0x434eb0 (game/agent/agent_queries.cpp owns the row): body visible in the unit, so pointers passed to it do not escape
inline __declspec(noinline) int ItemRef::body_armor()      // 0x434eb0
{
    if (item < 0) return 0;
    int v = g_items[item].body_armor;
    if (v != 0) {
        switch (b.mod) {
        case 2:
        case 0x15: v -= 3; break;
        case 7: v -= 1; break;
        case 0x18: v += 1; break;
        case 0x19: v += 2; break;
        case 0x12:
        case 0x1a: v += 3; break;
        case 1: v -= 4; break;
        case 5:
        case 0x16: v -= 2; break;
        case 0x1b: v += 4; break;
        case 0x1d: v += 6; break;
        }
        if (v < 0) v = 0;
    }
    return v;
}

// @ 0x4532f0
void Agent::shield_hit(int damage)
{
    int cur = wield[1];
    if (cur >= 0 && both_hands == 0) {
        ItemRef* it = &pairs[cur];
        if (it && (g_items[it->item].type_flags & 0xff) == itp_type_shield) {
            damage -= it->body_armor();
            float f = 0.0f > (float)damage ? 0.0f : (float)damage;
            int skill = g_game->skill_level(troop, 0x1a);
            int d = (int)((1.0f - (float)skill * 0.08f) * (float)(int)f);

            if ((float)d > 0.0f) {
                unsigned v = (it->m4 & 0xffff) - d;
                it->w.count = 0;
                it->m4 |= ((int)v < 0) - 1 & v;
                if ((it->m4 & 0xffff) == 0) {
                    int old = wield[1];
                    wield[1] = -1;
                    on_wield_reset();
                    set_order(1, move_anim(0), 0x1000010, 1.0f);
                    f = 0.0f;
                    int kind = 0x5e;
                    play_sound_var(&kind, &f);
                    mat4 M = body->local * *body->fx->rec(0xc);
                    int mod = it->b.mod;
                    kind = 7;
                    g_mission->add_placed(&kind, &it->item, &mod, (rgl::matrix*)&M, &pool_id);
                    if (((g_mission_templates[g_mission->mode].flags >> 1) & 1) && old >= 0 &&
                        g_mission->player_id == pool_id && old < 10 && slot_flag[old]) {
                        ItemRef* s = &pairs[old];
                        if (s->item >= 0 && (g_items[s->item].type_flags & 0xff) == itp_type_shield) {
                            unsigned bits;
                            int m = s->b.mod;
                            if (m == 1) {
                                if (frand() < 0.2f) {
                                    { tstring key("ui_shield_broken"); g_gui.add(*g_strtable.lookup(key), 0xddbbbb); }
                                    
                                    if (s->item == g_game->chars[g_game->player].equip[old].item && pairs[old].m4 == g_game->chars[g_game->player].equip[old].m4)
                                        g_game->chars[g_game->player].equip[old].item = -1;
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
                            
                            if (s->item == g_game->chars[g_game->player].equip[old].item && pairs[old].m4 == g_game->chars[g_game->player].equip[old].m4) {
                                ItemRef* r2 = &g_game->chars[troop].equip[old];
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
    if (state != 1) {
        body->fx->set_visible(true);
        if (pool_id != g_mission->player_id)
            drop_held();
    }
    if (!queued) {
        if (pool_id != g_mission->player_id)
            g_mission->queue_dead(this);
        queued = 1;
    }
}

// ---- Agent::hit_damage: the damage of a melee blow or missile hit ------------------------------------------------------------------
// 0x434c50 / 0x434cb0 / 0x434d70 (ItemRef::difficulty / damage / speed_rating, game/agent/agent_queries.cpp owns the rows): bodies that only read *this, so
// Agent::hit_damage (0x453710) keeps sp.item in a register across the calls (no `item` local) and sp does not escape.
inline __declspec(noinline) int ItemRef::difficulty()      // 0x434c50
{
    if (item < 0) return 0;
    int v = g_items[item].difficulty;
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
inline __declspec(noinline) int ItemRef::damage(int which)      // 0x434cb0
{
    if (item < 0) return 0;
    unsigned v;
    if (which == 0) v = g_items[item].swing_damage & 0xff;
    else v = g_items[item].thrust_damage & 0xff;
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
inline __declspec(noinline) int ItemRef::speed_rating()      // 0x434d70
{
    if (item < 0) return 100;
    int v = g_items[item].spd_rtng;
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
        if (v.horse >= 0) {
            A = vmul_sv(ms->agents[v.horse].vel_xy.y, ms->agents[v.horse].body->local.f);
        } else {
            vector4 lv;
            lv.x = v.vel_xy.x; lv.y = v.vel_xy.y; lv.z = 0.0f;
            A = ((const ::pos&)v.body->local).rot(lv);
        }
    }
    vector4 B;
    if (horse >= 0) {
        B = vmul_sv(ms->agents[horse].vel_xy.y, ms->agents[horse].body->local.f);
    } else {
        vector4 lv;
        *(vec2*)&lv = vel_xy;                       // x, y as dwords; z is never stored (rot_xy uses 0)
        B = rot_xy(body->local, lv);
    }
    if (missile) {
        *out_flag = 1;
        float spd = missile->speed;
        if ((int)victim >= 0) {
            vector4 rel = A - missile->vel * missile->speed;
            spd = rel.length();        // 0x4539d7: (dz^2 + dy^2) + dx^2
        }
        *out_dmg = ((float)pow((double)(spd / missile->f60), (double)g_module_settings.missile_damage_speed_power)) * missile->f64 * *scale;
    } else {
        int st = hand ? mode2 : mode1;
        unsigned fl = (st == 0);
        *out_flag = fl;
        if (sp.item >= 0)
            *out_dmg = (float)(int)sp.damage(fl) * *scale;
        else
            *out_dmg = (float)g_game->chars[troop].lvl58 * 0.3f + 2.0f;
        vector4 d;
        d = A - B;
        float len = d.normalize();
        if (f100_is_0x48_and_charged()) {
            *out_dmg = (*out_dmg + 5.0f) * (float)pow((double)len, (double)1.8f);
            vector4 q = ((const ::pos&)body->local).rot(skel_rec_inl(body->fx, 19)->f);
            q.normalize();
            *out_dmg = ((0.2f - (q.x * d.x + d.y * q.y + d.z * q.z)) * *out_dmg) * g_module_settings.couched_lance_damage_multiplier * 0.2f;
            if (g_options.damage_msgs) {
                if (pool_id == g_mission->player_id)
                    { tstring key("ui_delivered_couched_lance_damage"); g_gui.add(*g_strtable.lookup(key), 0xffd03030); }
                else if (victim == (unsigned)g_mission->player_id)
                    { tstring key("ui_received_couched_lance_damage"); g_gui.add(*g_strtable.lookup(key), 0xffd03030); }
            }
        } else {
            if (mode1 == 3)
                *out_dmg = *out_dmg * 1.05f;
            vector4 q = ((const ::pos&)body->local).rot(skel_rec_inl(body->fx, 19)->f);
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
            float w = (float)sp.speed_rating() * 0.154f;
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
                if (n != 0 && g_options.damage_msgs && pool_id == g_mission->player_id) {
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
        int prof = (int)g_game->chars[troop].stat[cls];
        if ((g_items[sp.item].type_flags & 0xff) == itp_type_crossbow) {
            if (g_mission->precip_mode == 1)
                *out_dmg = *out_dmg * 0.75f;
        } else {
            float pct = (float)prof * 0.01f;
            *out_dmg = (pct * 0.15f + 0.85f) * *out_dmg;
            unsigned t = g_items[sp.item].type_flags & 0xff;
            if (t == itp_type_bow) {
                int sk = g_game->skill_level(troop, 0x21);
                int lim = sp.difficulty() + 4;
                if (!(lim < sk)) lim = sk;
                *out_dmg = ((float)lim * 0.14f + 1.0f) * *out_dmg;
                if (horse != -1) {
                    int hs = g_game->skill_level(troop, 0x17);
                    *out_dmg = ((float)hs * 0.019f + 0.8f) * *out_dmg;
                }
                if (g_mission->precip_mode == 1)
                    *out_dmg = *out_dmg * 0.9f;
            } else if (t == itp_type_thrown) {
                int sk = g_game->skill_level(troop, 0x22);
                *out_dmg = ((float)sk * 0.1f + 1.0f) * *out_dmg;
                if (horse != -1) {
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
            if (w2.item >= 0 || horse >= 0)
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
        frame_set* data = body->fx->res;
        rgl::matrix world[0x20];
        if (0 < body->fx->count) {
            do {
                if (i == 0) {
                    rgl::mat4 g;
                    NewtonRagDollBoneGetGlobalMatrix(body->fx->elems[0].ragdoll_bone, (float*)&g);
                    rgl::mat4 chk = g;
                    if (!(-1e6f <= chk.o.x) || !(chk.o.x < 1e6f)) {
                        on_removed();
                        body->fx->pose_src = 0;
                        break;
                    }
                    (rgl::mat4&)body->local = g;
                    rgl::mat4 id0;
                    { rgl::matrix idp; identity_pos(&idp); body->fx->elems[0].m44 = *(rgl::mat4*)&idp; }
                    *(rgl::mat4*)&world[0] = g;
                } else {
                    FxElem* e = &body->fx->elems[i];
                    rgl::mat4 res;
                    mat4* dst;
                    if (e->ragdoll_bone == 0) {
                        res = *(rgl::mat4*)&world[data->bones[i].parent] * *(rgl::mat4*)&e->m44;
                        dst = (rgl::mat4*)&world[i];
                    } else {
                        rgl::mat4 g;
                        NewtonRagDollBoneGetGlobalMatrix(e->ragdoll_bone, (float*)&g);
                        mat4* w = (rgl::mat4*)&world[i];
                        *w = g;
                        if ((kind == 0 && (i == 8 || i == 10 || i == 15)) || (kind == 1 && i == 2)) {
                            float lim = 0.15f;
                            if (kind == 1)
                                lim = 0.4f;
                            else if (i == 8)
                                lim = 0.35f;
                            if (!mount_sync && !floor_sound_done && hold.o.z - w->o.z < 0.0f &&
                                w->o.z - g_mission->scene->floor_z(&w->o) < lim) {
                                g_sound_mgr.play(g_sound_mgr.slot[74], 0, &body->local.o, frand() * 0.4f + 0.8f, 0);
                                floor_sound_done = 1;
                            }
                            hold = *w;
                        }
                        res = *(rgl::mat4*)&((rgl::mat4&)world[data->bones[i].parent]).relative((const rgl::mat4&)world[i]);
                        dst = (rgl::mat4*)&body->fx->elems[i].m44;
                    }
                    *dst = res;
                }
                i++;
            } while (i < body->fx->count);
        }
        float dx = body->local.o.x - anchor.x;
        float dy = body->local.o.y - anchor.y;
        float dz = body->local.o.z - anchor.z;
        if (9.0f < dx * dx + (dy * dy + dz * dz))
            build_static_collision();
        if ((int)mount_id >= 0) {
            unsigned mid = mount_id;
            if (g_mission->agents.pages[mid >> 4][mid & 0xf].body != 0) {
                rgl::mat4 m1, m2, mm;
                NewtonBodyGetMatrix(mount_body, (float*)&m1);
                NewtonBodyGetMatrix(NewtonRagDollBoneGetBody(body->fx->elems[0].ragdoll_bone), (float*)&m2);
                rgl::strategic_entity* mb = g_mission->agents.pages[mount_id >> 4][mount_id & 0xf].body;
                mm = (rgl::mat4&)mb->local * *(rgl::mat4*)&mb->fx->elems[0].m84;
                NewtonBodySetMatrix(mount_body, (float*)&mm);
                if (0.0f < m2.o.z - m1.o.z) {
                    int j = 0;
                    if (0 < body->fx->count) {
                        do {
                            const NewtonRagDollBone* b = body->fx->elems[j].ragdoll_bone;
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
                            j++;
                        } while (j < body->fx->count);
                    }
                    mount_sync = 0;
                } else if (mount_vel_sync != 0) {
                    mount_vel_sync = 0;
                    float* pv = &g_mission->agents.pages[mount_id >> 4][mount_id & 0xf].v56a4.x;
                    int j = 0;
                    if (0 < body->fx->count) {
                        do {
                            const NewtonRagDollBone* b = body->fx->elems[j].ragdoll_bone;
                            if (b != 0) {
                                NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                                float v[4];
                                NewtonBodyGetVelocity(nb, v);
                                v[0] = v[0] + pv[0];
                                v[1] = v[1] + pv[1];
                                NewtonBodySetVelocity(nb, v);
                            }
                            j++;
                        } while (j < body->fx->count);
                    }
                    mount_sync = 0;
                } else
                    mount_sync = 0;
            } else if ((int)mount_id >= 0) {
                int j = 0;
                if (0 < body->fx->count) {
                    do {
                        const NewtonRagDollBone* b = body->fx->elems[j].ragdoll_bone;
                        if (b != 0) {
                            NewtonBody* nb = NewtonRagDollBoneGetBody(b);
                            float v[4];
                            NewtonBodyGetVelocity(nb, v);
                            v[0] = v[0] + link_vel.x;
                            v[1] = v[1] + link_vel.y;
                            NewtonBodySetVelocity(nb, v);
                        }
                        j++;
                    } while (j < body->fx->count);
                }
                mount_sync = 0;
            } else
                mount_sync = 0;
        } else
            mount_sync = 0;
    } else if (!queued) {
        matrix* bm = (matrix*)&body->local;
        bm->f = vel;
        bm->f.z = 0.0f;
        bm->u = vector4(0.0f, 0.0f, 1.0f);
        bm->orthonormalize();
        if (state != 1) {
            floor_blend = g_step_tbl[2] + floor_blend;
            if (floor_blend < 0.0f)
                floor_blend = 0.0f;
            else if (1.0f < floor_blend)
                floor_blend = 1.0f;
            float* d = &floor_face->normal.x;
            float k = 1.0f - floor_blend;
            float a = d[0] * floor_blend;
            float b = d[1] * floor_blend;
            float c = d[2] * floor_blend;
            float zk = k * 0.0f;
            bm->u = vector4(zk + a, zk + b, k + c);
            bm->u.normalize();
            bm->s = rgl::cross(bm->f, bm->u);
            bm->s.normalize();
            bm->f = rgl::cross(bm->u, bm->s);
        }
        bm->rotate_about_f(rot);
        bm->o = vector4(v98.x + pos.x, v98.y + pos.y, v98.z + pos.z);
    }
    camera_params(static_cast<capsule*>(body->shape));     // the body's shape is its capsule (Mission::spawn 0x4832b0)
    if (horse < 0) {
        if (f5830 == 3 || f5830 == 4) {
            body->set_mode(0);
        } else {
            body->set_mode(2);
            if (rider == -1)
                body->shadow_caster = -1;
        }
    } else {
        body->set_mode(0);
        rgl::strategic_entity* mb = g_mission->agents.pages[(unsigned)horse >> 4][(unsigned)horse & 0xf].body;
        if (mb != 0)
            mb->shadow_caster = body->id;
    }
    name_label->mask = 0;
    if (g_options.fx_on != 0)
        name_label->mask = 1;
}

#define itcf_carry_slot1 0x100000000i64
#define itcf_carry_slot2 0x200000000i64
#define itcf_carry_quiver_back_right 0x60000000i64
#define itcf_carry_quiver_back 0x80000000i64
// get_carry_pos (0x433e40, src/game/data/item_kind.cpp) and the two shape constructors (0x4332a0 / 0x433310,
// src/game/agent/agent_item_pick.cpp) with their bodies visible here (inline noinline duplicates): reduce_blow's carry frame,
// bone / aux, lo and hi then do not escape, so the packer shares their slots with the later locals as the original does
inline __declspec(noinline) void ItemKind::get_carry_pos(rgl::matrix* out, int* bone_flags, int* bone)
{
    vector4 v;
    float a;
    *bone = 0;
    rgl::matrix ident;
    *out = *identity_pos(&ident);
    __int64 carry = flags & itcf_carry_mask;
    *bone_flags = 0;
    if (carry == itcf_carry_sword_left_hip) {
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.24f; v.y = 0.1f; v.z = 0.1f;
        out->o = v;
        a = 1.5393804f; out->rotate_about_f(&a);
        a = -2.2f; out->rotate_about_u(&a);
        a = 0.12f; out->rotate_about_s(&a);
        a = -0.2f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_axe_left_hip) {
        vector4& o = out->o;
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.21f; v.y = 0.12f; v.z = 0.14f;
        o = v;
        a = 1.4451327f; out->rotate_about_f(&a);
        a = 1.0f; out->rotate_about_u(&a);
        a = 3.3415928f; out->rotate_about_f(&a);
        float s = (float)weapon_length * 0.01f - 0.3f;
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
        a = 1.4451327f; out->rotate_about_f(&a);
        a = 1.0f; out->rotate_about_u(&a);
        a = 3.3415928f; out->rotate_about_f(&a);
        float s = (float)weapon_length * 0.01f - 0.1f;
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
        a = 0.15707964f; out->rotate_about_f(&a);
        a = -2.5f; out->rotate_about_u(&a);
        a = -0.05f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_dagger_front_right) {
        *bone = 8;
        *bone_flags = 0;
        v.x = -0.04f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = -3.2986722f; out->rotate_about_f(&a);
        a = -2.5f; out->rotate_about_u(&a);
        a = 0.05f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_slot1) {
        vector4& o = out->o;
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        o = v;
        a = 0.5654867f; out->rotate_about_u(&a);
        a = 3.1415927f; out->rotate_about_f(&a);
        float s = (float)weapon_length * 0.01f - 0.3f;
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
        a = -2.576106f; out->rotate_about_u(&a);
    }
    else if (carry == (itcf_carry_quiver_right_vertical | itcf_carry_slot1)) {
        vector4& o = out->o;
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.19f; v.y = 0.29f; v.z = -0.17f;
        o = v;
        a = 0.5654867f; out->rotate_about_u(&a);
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
        a = 3.2986722f; out->rotate_about_f(&a);
        a = 1.0995574f; out->rotate_about_u(&a);
    }
    else if (carry == (itcf_carry_quiver_back | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.06f; v.y = 0.18f; v.z = -0.2f;
        out->o = v;
        a = -3.2986722f; out->rotate_about_f(&a);
        a = 1.5707964f; out->rotate_about_u(&a);
    }
    else if (carry == (itcf_carry_dagger_front_left | itcf_carry_slot1)) {
        *bone = 0;
        *bone_flags = 8;
        v.x = -0.22f; v.y = -0.06f; v.z = -0.2f;
        out->o = v;
        a = 3.138451f; out->rotate_about_f(&a);
        a = 1.0995574f; out->rotate_about_u(&a);
    }
    else if (carry == (itcf_carry_quiver_front_right | itcf_carry_slot1)) {
        vector4& o = out->o;
        *bone = 6;
        *bone_flags = 8;
        v.x = 0.19f; v.y = 0.34f; v.z = -0.155f;
        o = v;
        a = -0.5654867f; out->rotate_about_u(&a);
        a = 1.4137167f; out->rotate_about_f(&a);
        a = 0.1f; out->rotate_about_u(&a);
        float s = (float)weapon_length * 0.01f + 0.5f;
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
        a = -0.5654867f; out->rotate_about_u(&a);
        a = -1.5707964f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_quiver_front_right) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.21f; v.y = 0.15f; v.z = 0.0f;
        out->o = v;
        a = 1.4451327f; out->rotate_about_f(&a);
        a = -2.5f; out->rotate_about_u(&a);
        a = -0.2f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_quiver_back_right) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.05f; v.y = 0.03f; v.z = -0.19f;
        out->o = v;
        a = 0.15707964f; out->rotate_about_f(&a);
        a = 1.1f; out->rotate_about_u(&a);
        a = 3.1415927f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_quiver_right_vertical) {
        *bone = 4;
        *bone_flags = 0;
        v.x = -0.23f; v.y = 0.05f; v.z = 0.0f;
        out->o = v;
        a = 1.4451327f; out->rotate_about_f(&a);
        a = 0.5f; out->rotate_about_u(&a);
        a = -0.05f; out->rotate_about_f(&a);
        a = 0.2f; out->rotate_about_s(&a);
    }
    else if (carry == itcf_carry_quiver_back) {
        *bone = 5;
        *bone_flags = 8;
        v.x = -0.05f; v.y = 0.02f; v.z = -0.17f;
        out->o = v;
        a = 0.37699112f; out->rotate_about_u(&a);
    }
    else if (carry == (itcf_carry_dagger_front_right | itcf_carry_slot1)) {
        *bone = 3;
        *bone_flags = 0;
        v.x = 0.18f; v.y = -0.1f; v.z = -0.1f;
        out->o = v;
        a = 2.1991148f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_revolver_right) {
        *bone = 9;
        *bone_flags = 4;
        v.x = -0.1f; v.y = 0.0f; v.z = 0.05f;
        out->o = v;
        a = -1.5707964f; out->rotate_about_f(&a);
        a = 0.9424779f; out->rotate_about_u(&a);
        a = -0.1f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_pistol_front_left) {
        *bone = 7;
        *bone_flags = 0;
        v.x = -0.07f; v.y = 0.25f; v.z = 0.15f;
        out->o = v;
        a = 0.15707964f; out->rotate_about_f(&a);
        a = -1.5f; out->rotate_about_u(&a);
        a = -0.05f; out->rotate_about_f(&a);
    }
    else if (carry == itcf_carry_bowcase_left) {
        *bone = 10;
        *bone_flags = 0;
        v.x = 0.26f; v.y = 0.03f; v.z = -0.1f;
        out->o = v;
        a = -4.3982296f; out->rotate_about_f(&a);
        a = -0.25f; out->rotate_about_s(&a);
        a = -0.84823006f; out->rotate_about_u(&a);
    }
    else if (carry == (itcf_carry_sword_left_hip | itcf_carry_slot2)) {
        *bone = 1;
        *bone_flags = 0;
        v.x = 0.23f; v.y = 0.15f; v.z = 0.23f;
        out->o = v;
        a = -1.4451327f; out->rotate_about_f(&a);
        a = 1.5f; out->rotate_about_u(&a);
        a = -0.2f; out->rotate_about_f(&a);
    }
    else if (carry == (itcf_carry_axe_left_hip | itcf_carry_slot2)) {
        *bone = 2;
        *bone_flags = 0;
        v.x = 0.05f; v.y = 0.19f; v.z = 0.24f;
        out->o = v;
        a = -2.1991148f; out->rotate_about_f(&a);
        a = 1.8f; out->rotate_about_u(&a);
        a = -0.2f; out->rotate_about_f(&a);
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
    if (kind == 1)
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
    if (kind == 0) {
        ItemRef w = best_weapon_slot();
        if (w.item >= 0 && b->hit) {
            ItemKind* ik = &g_items[w.item];
            rgl::matrix carry;
            ik->get_carry_pos(&carry, &bone, &aux);
            rgl::mat4 r = *body->fx->rec(bone);
            rgl::mat4 u = (rgl::mat4&)body->local * (r * *(rgl::mat4*)&carry);
            HitInfo* h = b->hit;
            if (u.u.y * h->d.y + u.u.z * h->d.z + u.u.x * h->d.x < -0.5f) {
                vector4 hi = h->d * 4.0f + h->p;
                vector4 lo = h->p - h->d * 4.0f;
                capsule cap(0.0f, &lo, &hi);
                float zero = 0.0f;
                sphere sph(*(vector4*)&u.o, &zero);
                float gap = capsule_sphere_gap(&cap, &sph);
                if (gap < (float)ik->weapon_length * 0.01f) {
                    int bonus = w.body_armor() + 10;
                    armor_v = (float)bonus + armor_v;
                    if (g_mission->player_id == pool_id) {
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
    if (kind != 1) {
        if (b->bone == 9)
            b->dmg_out = x * 1.2f;
        else if (b->bone == 1 || b->bone == 4)
            b->dmg_out = x * 0.9f;
    }
    if (b->dmg_out < 0.0f)
        b->dmg_out = 0.0f;
}
// the friend test as an inline on a by-value id: its own id >= 0 test stays after the outer one (0x455342, 0x4553a0)
static __forceinline bool agent_friend(Mission* ms, int id, Agent* a) { return id >= 0 && mag(ms, id).is_friend(a); }
// @ 0x4552c0
void Agent::draw_name()
{
    Mission* ms = g_mission;
    int local = ms->player_id;
    unsigned col = 0xffffffff;
    vector4 origin;
    origin.x = 0.0f; origin.y = 0.0f; origin.z = 0.0f;
    vec2 scale;
    scale.x = 0.14f; scale.y = 0.14f;
    float a = -1.0f;
    float bb = 1.0f;
    if (local >= 0) {
        if (mag(ms, local).team == team)
            col = 0xff44ff44;
        else if (agent_friend(ms, local, this))
            col = 0xff4444ff;
    }
    if (kind == 0 && pool_id != local) {
        bool f1 = (g_mission_templates[ms->mode].flags >> 1) & 1;
        if (f1 && agent_friend(ms, local, this)) {
            char buf[512];
            tstring* nm = g_game->chars[troop].display_name();
            sprintf(buf, "%s", nm->c_str());
            if (state != 1) {
                name_label->frame.o.z = 0.3f;              // the label's height over the origin
                if (f5830 == 3)
                    buf[0] = 0;
            } else if (horse >= 0)
                name_label->frame.o.z = 2.65f;
            else
                name_label->frame.o.z = 1.88f;
            unsigned colp = col;
            vector4 ext;
            tstring text(buf);
            g_font->layout_clear(&ext, &text, &origin, (const float*)&scale, name_label->lists[0][0], (const int*)&colp, &a, &bb, 0x10);
        }
    }
}
