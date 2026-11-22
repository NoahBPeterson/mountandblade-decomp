//-----------------------------------------------------------------------------
// File: agent_blows.cpp  (0x457b40..0x45c220)
//
// Desc: process_blow, swing_trace
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
inline __declspec(noinline) vector4 rgl::cross(const vector4& a, const vector4& b) { return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline __declspec(noinline) float __cdecl fmax_ref(const float* a, const float* b) { return *a > *b ? *a : *b; }
inline __declspec(noinline) float __cdecl clamp(const float* x, const float* lo, const float* hi)     // 0x40e3d0
{
    return *x < *lo ? *lo : (*x >= *hi ? *hi : *x);
}
float frand();                                                          // 0x40e400
float __cdecl fmin_ref(const float* a, const float* b);                 // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
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
static inline bool a_bit(unsigned v, int n) { return ((v >> n) & 1) != 0; }
using rgl::mesh;
using rgl::mat4;
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
// rgl::matrix::relative (0x4329d0, src/game/common/game_util.cpp) visible here too (inline noinline duplicate): with both bodies refresh_carried's rel does not escape, so the target-mesh load is hoisted
// above the rel.o.y store as in the original
inline __declspec(noinline) rgl::matrix rgl::matrix::relative(const rgl::matrix& b) const
{
    rgl::matrix m;
    m.o = transform_point_to_local(b.o);
    static_cast<rgl::mat3&>(m) = mul_t(b);
    return m;
}
// ---- Agent::shield_hit, Agent::on_removed ---------------------------------------------------------------------------------------------
#define itp_type_shield 7
inline __declspec(noinline) capsule::capsule(float r, const vector4* a, const vector4* b) : shape_base(inline_init())
{
    radius = r;
    l0 = *a;
    l1 = *b;
    p0 = l0;
    p1 = l1;
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

    if (state != 1)
        return;
    if (has_blow == 0)
        return;
    attacker = blow.attacker;
    dmg = blow.dmg_out;
    hit_part = blow.absorbed;
    has_blow = 0;
    anim_a = -1;
    anim_b = -1;
    if (attacker >= 0)
        ai.last_attacker = attacker;
    m = g_mission;
    atk_troop = -1;
    atk_rider = -1;
    if (attacker >= 0) {
        Agent** pg = &m->agents.pages[0];
        if (pg[(unsigned)attacker >> 4][attacker & 0xf].kind == 0) {
            atk_rider = attacker;
            if (atk_rider >= 0)
                atk_troop = pg[(unsigned)atk_rider >> 4][atk_rider & 0xf].troop;
        } else if (pg[(unsigned)attacker >> 4][attacker & 0xf].rider >= 0) {
            atk_rider = m->agents.get(attacker)->rider;
            if (atk_rider >= 0)
                atk_troop = pg[(unsigned)atk_rider >> 4][atk_rider & 0xf].troop;
        }
    }
    if ((this->kind == 0 && blow.bone == 9) || (this->kind == 1 && blow.bone == 7)) {
        if (blow.item >= 0 && g_items[blow.item].is_ranged()) {
            dmg = dmg * 1.75f;
            if (attacker == m->player_id && g_options.damage_msgs != 0) {
                tstring k("ui_head_shot");
                g_gui.add(*g_strtable.lookup(k), 0xffffdd11);
                m = g_mission;
            }
        }
    }
    if (pool_id == m->player_id || rider == m->player_id) {
        if (g_options.dmg_to_player == 1)
            dmg = dmg * 0.5f;
        else if (g_options.dmg_to_player == 0)
            dmg = dmg * 0.25f;
    } else {
        int dd = g_options.dmg_to_friends;
        GameData* g = g_game;
        if (dd != 2 &&
            ((this->kind == 0 && party == g->main_party) ||
             (this->kind == 1 && rider >= 0 && m->agents.get(rider)->party == g->main_party))) {
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
        if (blow.attacker == m->player_id && shown != 0) {
            char msgbuf[512];
            {
                tstring k((this->kind != 0 && rider != -1) ? "ui_delivered_number_damage_to_horse"
                                                        : "ui_delivered_number_damage");
                sprintf(msgbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
            }
            {
                tstring t(msgbuf);
                g_gui.add(t, -261);
            }
        } else {
            int lid = m->player_id;
            if (lid >= 0 && m->agents.pages[(unsigned)lid >> 4][lid & 0xf].horse >= 0 &&
                blow.attacker == m->agents.pages[(unsigned)lid >> 4][lid & 0xf].horse && shown != 0 &&
                round_float(&dmg) != 0) {
                char hbuf[256];
                {
                    tstring k("ui_horse_charged_for_number_damage");
                    sprintf(hbuf, g_strtable.lookup(k)->c_str(), round_float(&dmg));
                }
                {
                    tstring t(hbuf);
                    g_mission->log_message(&t, 20.0f);
                }
            } else {
                const char* key = 0;
                shown = g_options.damage_msgs;
                m = g_mission;
                if (pool_id == m->player_id && shown != 0) {
                    key = "ui_received_number_damage";
                } else if (m->player_id >= 0) {
                    int l2 = m->player_id;
                    if (m->agents.pages[(unsigned)l2 >> 4][l2 & 0xf].horse >= 0 &&
                        pool_id == m->agents.pages[(unsigned)l2 >> 4][l2 & 0xf].horse && shown != 0)
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
    if (!(blow.flags & 0x20)) {
        int kind;
        float var;
#define HIT_SOUND(k) { kind = (k); play_sound_var(&kind, &var); }
        if (blow.item >= 0) {
            if (g_items[blow.item].is_ranged()) {
                var = 0.1f;
                if (pool_id == g_mission->player_id)
                    HIT_SOUND(0x66)
                else
                    HIT_SOUND(0x64)
            } else {
                bool blunt = (blow.item_mod & itp_wooden_attack) != 0 || blow.kind == 2;
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
    if (blow.missile != 0 && g_items[blow.missile->ammo.item].flags != itcf_throw_stone &&
        !(g_mission->flags & 1) && dmg > 7.0f && (health < 1.0f || blow.bone != 9 || dmg > 75.0f)) {
        rgl::mat4 M = body->local * *body->fx->rec(blow.bone);
        float len = blow.missile->ammo_scale() * 0.7f;
        matrix mm;
        mm.o = blow.pos - len * blow.dir;
        mm.f = blow.dir;
        mm.u = vector4(0.0f, 0.0f, 1.0f);
        mm.orthonormalize();
        if (blow.missile != 0) {
            rgl::mesh* ms = blow.missile->make_ranged_mesh();
            if (ms != 0) {
                vector4 off;
                off.x = 0.0f;
                off.y = blow.missile->ammo_scale();
                off.z = 0.0f;
                ms->translate(&off);
                ((FxDef*)body->fx)->attach_psys(blow.bone, ms, &((rgl::mat4*)&M)->relative(*(rgl::mat4*)&mm));
            }
        }
    }

    // ---- blood ----
    if (dmg > 1.0f && !(*(unsigned*)&g_mission->flags & 0x100)) {
        float blood = 1.0f;
        if (blow.kind == 2)
            blood = 0.2f;
        else if (blow.kind == 1)
            blood = 0.7f;
        if (g_options.blood > 0 && blow.kind != 2) {
            bool go = false;
            if (g_options.blood == 2) {
                go = true;
            } else if (g_mission->player_id >= 0) {
                if ((blow.pos - g_mission->agents.get(g_mission->player_id)->pos).out_length_squared() < 70.0f)
                    go = true;
            }
            if (go)
                g_mission->blood_burst(blow.pos.x, blow.pos.y, blow.pos.z, 0, blood * dmg);
        }
    }

    if (this->kind == 1) {
        float plen;
        if (health < 1.0f) {
            health = 0.0f;
            state = 5;
            f5830 = 3;
            tm.start = g_ticks[tm.clock];
            body->fx->track_root_vel = 1;
            draw_name();
            m = g_mission;
            if (rider == m->player_id && pool_id == m->f60 && a_bit(g_mission_templates[m->mode].flags, 1) &&
                item.item >= 0 && !a_bit(g_items[item.item].type_flags, 12)) {
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
                if (rider >= 0) {
                    g_mission->agents.get(rider)->set_order(0, 0x123, 0x1000081, 1.0f);
                    vector4 z;
                    z.x = 0.0f;
                    z.y = 0.0f;
                    z.z = 0.0f;
                    g_mission->agents.get(rider)->impulse = z;
                    ((Timer*)&g_mission->agents.get(rider)->t40)->set_delay(-2.4f);
                }
            } else {
                if (frand() <= 1.0f) {
                    set_order(0, 0x1a3, 0x2000080, 1.0f);
                    int kind = 4;
                    play_hit_sound(&kind);
                    plen = g_anim_set.data[0x1a3].subs->offset;
                    if (rider >= 0)
                        g_mission->agents.get(rider)->set_order(0, 0x122, 0x1000081, 1.0f);
                } else {
                    set_order(0, 0x1a2, 0x2000080, 1.0f);
                    plen = g_anim_set.data[0x1a2].subs->offset;
                    int kind = 4;
                    play_hit_sound(&kind);
                    if (rider >= 0)
                        g_mission->agents.get(rider)->set_order(0, 0x121, 0x1000081, 1.0f);
                }
                if (plen < 0.01f)
                    plen = 1.0f;
            }
            ((Timer*)&t5918)->set_delay(-plen);
            if (rider >= 0) {
                g_mission->agents.get(rider)->horse = -1;
                rider = -1;
            }
            return;
        }
        // the horse survived: a lance couched into it may stagger it
        // the horse survived: a lance couched into it may stagger it
        if (blow.item < 0) return;
        if ((g_items[blow.item].type_flags & 0xff) != itp_type_polearm) return;
        if (blow.kind != 1) return;
        if (!(blow.dmg_out > 8.0f)) return;
        if (!(vel_xy.y > 0.1f)) return;
        if ((int)blow.attacker < 0) return;
        if (!(rgl::dot(pos - m->agents.get(blow.attacker)->pos, body->local.f) < -0.5f)) return;
        if ((int)(anim[0].events & 0x1ffff) >= 0x4a) return;
        float a = 5.0f;
        float x = 5.0f / ((float)item.horse_charge() + 2.0f);
        float y = fmax_ref(&vel_xy.y, &a);
        x = (x / y) * 5.0f;
        if (frand() < x) {
            int kind = 4;
            play_hit_sound(&kind);
            set_order(0, 0x19c, 0x100004a, 1.0f);
            return;
        }
        a = 0.95f;
        float z = (float)item.horse_charge() * 0.01f + 0.7f;
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
        if ((int)blow.attacker >= 0)
            apos = g_mission->agents.get(blow.attacker)->pos;
        vector4 av = apos - pos;
        if (this->horse != -1) {
            anim_a = 0x120;
        } else {
            if (av.x * vel.x + (av.y * vel.y + av.z * vel.z) > 0.0f) {
                if (blow.bone == 9 && (blow.missile != 0 || frand() < 0.7f)) {
                    anim_a = 0x11a;
                } else if (blow.bone == 9) {
                    anim_a = 0x11d;
                } else if (blow.bone == 0 || blow.bone == 7) {
                    anim_a = 0x11c;
                } else if (blow.missile == 0 && rgl::dot(blow.dir2, body->local.s) > 0.3f) {
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
            if (((dmg < 1.0f) && (blow.dmg_in < (frand() + 1.0f) * (float)(g_game->chars[troop].attributes[0] + 15) * 0.2f)) ||   // strength (+0x58, 0x458bc1)
                dmg < g_module_settings.damage_interrupt_attack_treshold) {
                if ((int)blow.attacker >= 0 && g_mission->agents.get(blow.attacker)->kind == 0)
                    stunned = true;
            }
        }
        if (this->horse == -1 && blow.item >= 0 &&
            (ik = &g_items[blow.item], ik->is_ranged() || (ik->type_flags & 0xff) == itp_type_polearm)) {
            if (!stunned) {
                if (((ik->type_flags & 0xff) == itp_type_polearm && frand() < 0.5f && blow.dmg_in > 15.0f) ||
                    ((g_items[blow.item].type_flags & 0xff) == itp_type_thrown && frand() < 0.4f && blow.dmg_in > 15.0f) ||
                    ((g_items[blow.item].type_flags & 0xff) == itp_type_bow && frand() < 0.3f && blow.dmg_in > 25.0f) ||
                    ((g_items[blow.item].type_flags & 0xff) == itp_type_crossbow && frand() < 0.3f && blow.dmg_in > 25.0f)) {
                    anim_a = move_anim(2);
                    float k = frand() * 0.2f + 2.1f;
                    hit_part = 0x82;
                    imp.x = k * blow.dir.x;
                    imp.y = k * blow.dir.y;
                    imp.z = k * blow.dir.z;
                    goto impulse;
                }
                anim_a = move_anim(1);
            }
        } else if (this->horse != -1) {
            anim_b = move_anim(0);
        } else do {
            if (rgl::dot(vel, blow.dir) < 0.0f &&
                (((blow.flags & 0x10) && blow.dmg_in > 7.0f) || (blow.dmg_in > 24.0f && frand() < 0.4f)) &&
                blow.kind == 2 && anim[0].cur != 0x125) {
                anim_a = 0x125;
                imp.x = vel.x * -0.6f;
                anim_b = 0x126;
                imp.y = vel.y * -0.6f;
                imp.z = vel.z * -0.6f;
                if (blow.flags & 0x10) {
                    float rr = frand() * 0.4f + 1.5f;
                    imp.x = rr * blow.dir.x + imp.x;
                    imp.y = rr * blow.dir.y + imp.y;
                    imp.z = rr * blow.dir.z + imp.z;
                }
            } else {
                anim_b = move_anim(1);
                if (!(blow.flags & 0x10)) {
                    if (!(blow.dmg_in > 15.0f))
                        break;
                    if (!(frand() < 0.3f))
                        break;
                }
                {
                    float e = (frand() - 0.5f) * 0.5f;
                    float f = (frand() + 0.6f) * (blow.dmg_in + 30.0f) * 0.02f;
                    vector4* sv = &body->local.s;
                    imp.x = f * blow.dir.x + e * sv->x;
                    imp.y = f * blow.dir.y + e * sv->y;
                    imp.z = f * blow.dir.z + e * sv->z;
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
                surg = (float)g_game->parties.get(g_game->main_party)->skill_level(10) * 0.04f + 0.25f;
                m = g_mission;
            }
        }
        if (!(m->flags & 1) &&
            ((g_game->chars[troop].flags30 & tf_allways_fall_dead) != 0 ||
             (blow.kind != 2 && troop >= 0 && !g_game->chars[troop].is_hero() && frand() > surg)))
            cA = true;
        else
            cB = true;
        f5830 = 3;
        tm.start = g_ticks[tm.clock];
        mount_id = this->horse;
        body->fx->track_root_vel = 1;
        float len = 0.5f;
        if (anim_a >= 0) {
            len = g_anim_set.data[anim_a].subs->offset;
            if (len < 0.01f)
                len = 0.5f;
        }
        ((Timer*)&t5918)->set_delay(-len);
        hit_bone = blow.bone;
        hit_vec = blow.dir2;
        hit_vec.normalize();
        float k = (blow.dmg_in + 5.0f) * 0.2f;
        hit_vec.x = k * hit_vec.x;
        hit_vec.y = k * hit_vec.y;
        hit_vec.z = k * hit_vec.z;
        hit_pos = blow.pos;
        if (blow.flags & 0x10) {
            hit_vec.x = hit_vec.x + hit_vec.x;
            hit_vec.y = hit_vec.y + hit_vec.y;
            hit_vec.z = hit_vec.z + hit_vec.z;
        }
        if (blow.kind == 2) {
            hit_vec.x = hit_vec.x + hit_vec.x;
            hit_vec.y = hit_vec.y + hit_vec.y;
            hit_vec.z = hit_vec.z + hit_vec.z;
        }
        draw_name();
        m = g_mission;
        if (this->horse >= 0) {
            if (m->player_id < 0 || m->agents.get(m->player_id)->horse != -1 ||
                ((blow.attacker != m->player_id || m->dismount_count >= 4) && !(m->flags & 1))) {
                m->agents.get(this->horse)->dismount_state = 1;
                Agent* h = g_mission->agents.get(this->horse);
                h->tm.start = g_ticks[h->tm.clock];
            } else {
                Agent* h = m->agents.get(this->horse);
                vector4 bf;
                bf.x = h->vel_xy.y * body->local.f.x;
                bf.y = body->local.f.y * h->vel_xy.y;
                bf.z = body->local.f.z * h->vel_xy.y;
                ((Timer*)&t40)->set_delay(-1.4f);
                impulse = bf;
                g_mission->agents.get(this->horse)->rider = -1;
                this->horse = -1;
                g_mission->dismount_count++;
            }
        }
    }

    // ---- experience and weapon proficiency for the attacker ----
    if (atk_troop >= 0 && this->is_enemy(PAGE_AGENT(g_mission, atk_rider)) && dmg > 0.01f && (cA || cB)) {
        Troop* chars = g_game->chars;
        float xpf = (float)chars[troop].scaled_b0();
        if (g_mission->flags & 1)
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
    if ((int)blow.attacker >= 0 && dmg > 0.01f) {
        AgentTable* t = &g_mission->agents;
        if (this->is_enemy(t->get(blow.attacker)) && t->get(blow.attacker)->troop >= 0 &&
            g_game->chars[t->get(blow.attacker)->troop].is_hero() && blow.item >= 0) {
            ItemRef sp = t->get(blow.attacker)->slot_pair(1);
            int cls = slot_class_impl(blow.item, sp.item >= 0);
            int old = (int)g_game->chars[atk_troop].stat[cls];
            float dm = dmg;
            if (g_mission->flags & 1)
                dm = dm * 0.33f;
            float amount = (((float)g_game->chars[troop].level + 10.0f) * dm) * 0.05f;
            vector4 vA = v56b4 - t->get(blow.attacker)->v56b4;
            vector4 vB = pos - t->get(blow.attacker)->pos;
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
                if (blow.attacker == g_mission->player_id && g_options.shot_difficulty != 0) {
                    float sd = tt;
                    if (blow.bone == 9)
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
            int nw = (int)g_game->chars[atk_troop].stat[cls];
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
    if (dmg > 1.0f && !(g_mission->flags & 1) && g_cfg.enable_blood != 0) {
        float hi = 50.0f;
        float lo = 1.0f;
        float k = ((dmg + 5.0f) * g_module_settings.blood_multiplier) * 0.1f + 1.0f;
        float ef = clamp(&k, &lo, &hi);
        matrix m2;
        m2.o = blow.pos;
        m2.f = vector4(-blow.dir.x, -blow.dir.y, -blow.dir.z + 0.5f);
        m2.u = vector4(0.0f, 0.0f, 1.0f);
        m2.orthonormalize();
        int vi = (int)(g_game->chars[troop].flags30 & 0xf);
        int e1 = g_anim.blood1(vi);
        int e2 = g_anim.blood2(vi);
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
            em->set_v184(&v);       // 0x433740 (game/agent/agent_item_pick.cpp)
            em->spawn_rate = ef * g_emit_defs.defs[e1].spawn_rate;
        }
        if (e2 >= 0) {
            rgl::psys* em = g_mission->spawn_effect(e2, (const rgl::mat4*)&m2, (int)(ef * 4.0f));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e2].b.x;
            v.y = s * g_emit_defs.defs[e2].b.y;
            v.z = s * g_emit_defs.defs[e2].b.z;
            em->set_v184(&v);       // 0x433740 (game/agent/agent_item_pick.cpp)
            em->spawn_rate = ef * g_emit_defs.defs[e2].spawn_rate;
        }
    }

    // ---- the outcome ----
    if (cA) {
        if (!(blow.flags & 0x20)) {
            int kind = 0;
            play_class_sound(&kind);
        }
        set_order(0, anim_a, 0x28000c8, 1.0f);
        state = 5;
        m = g_mission;
        if (blow.attacker == m->player_id) {
            g_game->cnt_killed++;
            if (this->is_friend(m->agents.get(g_mission->player_id)))
                g_game->cnt_killed_friend++;
            m = g_mission;
        }
        if ((int)blow.attacker >= 0)
            m->agents.get(blow.attacker)->kill_count++;
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
        if (!(blow.flags & 0x20)) {
            int kind = 0;
            play_class_sound(&kind);
        }
        set_order(0, anim_a, 0x28000c8, 1.0f);
        state = 4;
        m = g_mission;
        if (blow.attacker == m->player_id) {
            g_game->cnt_ko++;
            if (this->is_friend(m->agents.get(g_mission->player_id)))
                g_game->cnt_ko_friend++;
            m = g_mission;
        }
        if ((int)blow.attacker >= 0)
            m->agents.get(blow.attacker)->wounded_count++;
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
    if (!(blow.flags & 0x20)) {
        int kind = 1;
        play_class_sound(&kind);
    }
    if (anim_a >= 0)
        set_order(0, anim_a, hit_part | 0x5800000, 1.0f);
    if (anim_b >= 0)
        set_order(1, anim_b, hit_part | 0x5800000, 1.0f);
}

// ---- Agent::swing_trace: the melee swing / missile hit test ------------------------------------------------------------------------
// Agent::f100_is_0x48_and_charged 0x432470 (game_util.cpp); the agent calls it with an unused agent in ecx
int __stdcall probe_shape_contact(shape_base* s, vector4* out_point, shape_base** out_shape, const char* flag);   // 0x43a720 (game/agent/agent_stats.cpp)
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
            reach = (float)g_items[sp.item].weapon_length * 0.01f - 0.01f;
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
        missile->ray(&sweep);
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
            if (i != (unsigned)pool_id && i != (unsigned)horse) {
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
            if (o->kind == 0) {
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
                            if (horse == -1 && o->horse == -1 &&
                                0.8f < dv.x * vel.x + (dv.y * vel.y + dv.z * vel.z) && dl < 1.3f)
                                return 2;
                            goto next;
                        }
                        if (pool_id != g_mission->player_id)
                            return 2;
                    }
                    ItemRef cur = slot_pair_fi(0);
                    sp = cur;
                    int twohand = 0;
                    if (cur.item >= 0 && (g_items[cur.item].type_flags & 0x20000) != 0)
                        twohand = 1;
                    can = can_hit(pool_id, cand[i], hand, missile, &hitpt.x) != 0;
                    if (can) {
                        ItemRef os = o->slot_pair_fi(1);
                        if (os.item >= 0 && (g_items[os.item].type_flags & 0xff) == itp_type_shield) {
                            float dmg;
                            unsigned fl2[2];
                            hit_damage(hand, missile, &dmg, fl2, o->pool_id, scale);
                            ItemRef ms = slot_pair_fi(0);
                            unsigned wf = ms.item < 0 ? 0 : g_items[ms.item].type_flags;
                            if ((wf & 0x4000000) != 0)
                                dmg = dmg + dmg;
                            if (f100_is_0x48_and_charged())
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
                                if (pool_id == g_mission->player_id && is_friend(o)) {
                                    g_gui.add(*g_strtable.lookup(tstring("ui_you_hit_a_friendly_troop")),
                                                 0xffff0000);
                                }
                                out->flags = 0;
                                out->attacker = pool_id;
                                out->pos = hp;
                                out->bone = bone;
                                int flg;
                                hit_damage(hand, missile, &out->dmg_in, (unsigned*)&flg, o->pool_id, scale);
                                if (missile != 0) {
                                    out->dir = missile->vel;
                                    out->dir2 = missile->vel;
                                    out->dir2.normalize();
                                    out->item = missile->launcher.item;
                                    out->item_mod = missile->launcher.m4;
                                    out->kind = (g_items[missile->launcher.item].thrust_damage >> 8) & 3;
                                    out->missile = missile;
                                } else {
                                    ItemRef s0 = slot_pair(hand);
                                    out->dir = sw_vsub(o->pos, pos);
                                    if (s0.item < 0)
                                        out->kind = 2;
                                    else if (flg == 0)
                                        out->kind = (g_items[s0.item].swing_damage >> 8) & 3;
                                    else
                                        out->kind = (g_items[s0.item].thrust_damage >> 8) & 3;
                                    out->dir2 = dT;
                                    out->dir2.normalize();
                                    out->dir.normalize();
                                    out->item = s0.item;
                                    out->item_mod = s0.m4;
                                    if (f100_is_0x48_and_charged())
                                        out->flags |= 0x10;
                                }
                                o->reduce_blow(out);
                                if (o->has_blow == 0 || !(out->dmg_out < o->blow.dmg_out)) {
                                    o->blow = *out;
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
                    int rd = o->rider;
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
                                out->attacker = pool_id;
                                out->pos = hp;
                                out->bone = bone;
                                int flg;
                                hit_damage(hand, missile, &out->dmg_in, (unsigned*)&flg, o->pool_id, scale);
                                if (missile != 0) {
                                    out->dir = missile->vel;
                                    out->dir2 = missile->vel;
                                    out->item = missile->launcher.item;
                                    out->item_mod = missile->launcher.m4;
                                    out->kind = (g_items[missile->launcher.item].thrust_damage >> 8) & 3;
                                    out->missile = missile;
                                } else {
                                    ItemRef s0 = slot_pair(hand);
                                    if (s0.item < 0)
                                        out->kind = 2;
                                    else if (flg == 0)
                                        out->kind = (g_items[s0.item].swing_damage >> 8) & 3;
                                    else
                                        out->kind = (g_items[s0.item].thrust_damage >> 8) & 3;
                                    out->dir = nrm;
                                    out->dir2 = nrm;
                                    out->item = s0.item;
                                    out->item_mod = s0.m4;
                                }
                                o->reduce_blow(out);
                                if (o->has_blow == 0 || !(out->dmg_out < o->blow.dmg_out)) {
                                    o->blow = *out;
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
            rgl::entity* e = g_mission->scene->find_entity(eid);
            if (e != 0) {
                int pi = e->game_id;
                e->game_kind = 0xc;
                if (pi >= 0 && (unsigned)pi < g_mission->props.size()) {
                    PlacedObj* pr = &g_mission->props[pi];
                    prop_done = 1;
                    if (pr->kind == 0) {
                        unsigned kf = g_propkinds[pr->id].flags & 0x8000;
                        if ((kf != 0 && 0.0f < pr->hp) || kf == 0) {
                            float dmg = 0.0f;
                            int flg = 0;
                            if (kf != 0 && 0.0f < pr->hp) {
                                hit_damage(0, missile, &dmg, (unsigned*)&flg, 0xffffffff, scale);
                                pr->hp = pr->hp - dmg;
                            }
                            MissionObj* kind = &g_propkinds[pr->id];
                            g_game->set_matrix(&pr->frame);
                            g_game->event_ctx.prop = pi;
                            g_script.param1 = pi;
                            g_script.param2 = (__int64)dmg;
                            matrix frame;
                            frame.o = bB;
                            if (missile != 0) {
                                frame.f = missile->vel;
                                frame.f.normalize();
                                frame.u = vector4(0.0f, 0.0f, 1.0f);
                                frame.orthonormalize();
                                if ((kind->flags & 0x18000) != 0) {
                                    rgl::mesh* m = missile->make_ranged_mesh();
                                    if (m != 0) {
                                        vector4 o2 = sw_vsub(frame.o, sw_vmul(frame.f, 0.2f));
                                        matrix fm2;
                                        static_cast<mat3&>(fm2) = *(rgl::mat3*)&frame;
                                        fm2.o = o2;
                                        *(rgl::mat4*)&m->local_frame = ((rgl::mat4*)&e->local)->relative((const rgl::mat4&)fm2);
                                        e->add_mesh((rgl::mesh*)m);
                                        ragdoll = true;
                                    }
                                }
                            } else {
                                frame.s = fr1.s;
                                frame.f = fr1.f;
                                frame.u = fr1.u;
                            }
                            g_script.pos[1] = frame;
                            kind->triggers.update(-0x2a);
                            if ((kind->flags & 0x8000) != 0 && pr->hp <= 0.0f)
                                kind->triggers.update(-0x2b);
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
