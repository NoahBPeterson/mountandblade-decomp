//-----------------------------------------------------------------------------
// File: agent_logic.cpp  (0x444c50..0x460160)
//
// Desc: the agent's logic object: the navigation-mesh path search (NavMesh::find_path, reachable), goal / path planning and
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
#define ANIM_SLOT_SET_OUT_OF_LINE       // 0x4378a0 is a real call here (the body is in agent2/agent_stats.cpp)
// agent/item_kind.h (the agent module's header) declares a view `resmgr` of the resource manager whose members do not carry the defining
// signatures (res_obj / tstring in place of rgl::mesh / rgl::string). Its name is changed while the headers are read so that the canonical
// `resmgr` (rgl/core/resmgr.h: get_object 0x419540, find_meshes 0x41b960) and `resource_manager` (rgl/resource.h: get_material 0x418a70,
// get_object60 0x418c10) can be declared below under their own names; g_resmgr is the one object 0x5d6fa0 and is cast to them.
#define resmgr resmgr_agent_view
#define g_resmgr g_resmgr_agent_view      // its `extern resmgr g_resmgr`: the defining declaration is `resource_manager g_resmgr` (rgl/resource.h), declared below
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <vector>
#include <memory>
#include <new>
#include <algorithm>
#include "rgl.h"
#include "./agent_logic.h"
#include "../rgl_mesh.h"
#include "../agent3/agent_types.h"
#include "../agent3/agent_fx_sound.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../anim_slot.h"
#include "../skeleton.h"
#include "../item_ref.h"
#include "../emit.h"
#include "../../include/fx.h"
#include "../agent/agent.h"
#include "../agent3/agent_combat.h"
#include "../agent_obj.h"
#include "../mission.h"
#include "../nav_mesh.h"
#include "../font.h"
#include "../placed_obj.h"
#include "../str_table.h"
#include "../mode_rec.h"
#include "../../include/msg_log.h"
#include "../../include/phys_set.h"
#include "../../include/item_kind.h"
#include "../../include/game_data.h"
#include "../../include/troop.h"
#include "../../include/mesh_parts.h"
#include "../../include/mesh_prims.h"
#include "../hnode.h"
#include "../../include/height_field.h"
#include "../agent/curve.h"   // Curve: set_rec_phase 0x433b20 (the canonical class of the Skeleton record)
#include "../blow.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)

#undef resmgr
#undef g_resmgr
struct resmgr {                                                 // rgl/core/resmgr.h
    rgl::mesh* get_object(const rgl::string* name);             // 0x419540
    std::vector<rgl::mesh*> find_meshes(const rgl::string* prefix);   // 0x41b960
};
struct skin; struct named_object;
struct resource_manager {                                       // rgl/resource.h
    skin* get_material(const rgl::string& name);                // 0x418a70
    named_object* get_object60(const rgl::string& name);        // 0x418c10
};
extern resource_manager g_resmgr;                               // 0x5d6fa0
#define RESMGR ((resmgr*)&g_resmgr)
#define RESOURCE_MANAGER ((resource_manager*)&g_resmgr)

// ---- canonical classes / forwarders (see agent_logic.h) ----------------------------------------------------------------------------
struct vec2x : vec2 { void rotate_b(float ang); };           // 0x4327f0 (rgl/xmlcfg/game_util.h)
::pos* __cdecl identity_pos(::pos* out);                     // 0x40cd00 (returns out; shapes.cpp)
__forceinline void SceneEnt::add_extra(res_obj* m) { ((Poser*)this)->add_extra((int)m); }     // 0x442110
namespace std { template<> void vector<rgl::scene*, allocator<rgl::scene*> >::push_back(rgl::scene* const&); }   // 0x4119d0 (mesh_obj.cpp's instance, shared by every pointer vector)
__forceinline MeshVec& MeshVec::operator=(const std::vector<res_obj*>& o) { ((std::vector<int>*)this)->operator=(*(const std::vector<int>*)&o); return *this; }   // 0x43f0a0
__forceinline void MeshVec::push_back(res_obj* const& x) { ((std::vector<rgl::scene*>*)this)->push_back(*(rgl::scene* const*)&x); }                         // 0x4119d0

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
template<> void _Destroy_range<E56, allocator<E56> >(E56*, E56*, allocator<E56>&, _Nonscalar_ptr_iterator_tag);                   // 0x43e6e0
template<> void fill<E56*, E56>(E56*, E56*, const E56&);                                                                               // 0x440dc0
template<> E56* _Copy_backward_opt<E56*, E56*>(E56*, E56*, E56*, _Nonscalar_ptr_iterator_tag);                                       // 0x440a30
}

// ---- NavMesh::find_path: A*-style search over the navigation faces ---------------------------------------------------------

// @ 0x444c50
bool NavMesh::find_path(int from, int to, int* out_faces, vector4* out_pos, int* out_count, int* out_steps,
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
        for (int k = 0; k < faces[cur].nverts; k++) {
            int nb = faces[cur].nbr_[k];
            if (nb != -1 && faces[nb].stamp <= stamp) {
                float cost = 1.0f;
                faces[nb].stamp = stamp + 1;
                faces[nb].parent = cur;
                NavEdge* e = &edges[faces[cur].edge_[k]];
                float w = e->face == nb ? e->w_a : e->w_b;
                if (w < 0.65f)
                    cost = (0.65f - w) * 20.0f + 1.0f;
                if (w <= *w0)
                    cost = (1.0f - w) * 50.0f + cost;
                vector4 dg = a_vsub(faces[to].c, faces[nb].c);
                vector4 ds = a_vsub(faces[from].c, faces[nb].c);
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
        for (int j = 0; j < faces[prev].nverts; j++) {
            if (faces[prev].nbr_[j] == cur) {
                k = j;
                break;
            }
        }
        out_pos[i] = edges[faces[prev].edge_[k]].mid;
        prev = out_faces[i];
    }
    for (int i = 1; i < *out_count; i++)
        out_pos[i - 1] = out_pos[i];
    out_pos[*out_count - 1] = faces[out_faces[*out_count - 1]].c;
    return true;
}


// ---- NavMesh::reachable: can the goal face be reached within `max_steps` hops? -----------------------------------------------
// @ 0x4452f0
bool NavMesh::reachable(const int* from, const int* to, const int* max_steps)
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
            for (int k = 0; k < faces[cur].nverts; k++) {
                int nb = faces[cur].nbr_[k];
                if (nb != -1 && faces[nb].stamp <= stamp) {
                    faces[nb].stamp = stamp + 1;
                    faces[nb].parent = cur;
                    vector4 dg = a_vsub(faces[*to].c, faces[nb].c);
                    vector4 ds = a_vsub(faces[*from].c, faces[nb].c);
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
        const NavFace& f = g_mission->nav.faces[*face];
        float k = (float)(rand() % 0x3dc9) * 6.3223115e-06f + 0.7f;
        c = a_vadd(a_vscale(k, a_vsub(c, f.c)), f.c);
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
    float width;
    if (f10 == 0 && f2c == -1)
        width = 0.45f;
    else
        width = 0.6f;
    int face = update_nav_face();
    if (face >= 0 && goal_face >= 0) {
        g_mission->nav.find_path(face, goal_face, pface(), ppos(), &path_count, &path_steps, &width, &width, &width);
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
                            ir_add_amount(s, 1);
                            added++;
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
                        if (rate < 0.1f)
                            rate = 0.1f;
                        else if (rate >= 10.0f)
                            rate = 10.0f;
                        this->anim[1].rate = rate;
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
// @ 0x43b160 / 0x43b1b0
__declspec(noinline) void AgentTable::next_used(unsigned* i)
{
    unsigned v = ++*i;
    if ((int)v < count) {
        do {
            if (pages[v >> 4][v & 0xf].used != 0) break;
        } while ((int)(*i = ++v) < count);
    }
}
__declspec(noinline) int AgentTable::first_used()
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
#define AG(i) (*agent_by_id(i))
#define tf_guarantee_ranged 0x4000000i64
float __cdecl hit_chance(int* who, Agent* a);                          // 0x438cb0 (agent2/agent_stats.cpp)
static __forceinline float hit_chance_ref(const ItemRef& who, Agent* a) { return hit_chance((int*)&who, a); }
int __cdecl team_leader(const Agent* a);                                // 0x435b60
float frand();                                                          // 0x40e400
float clamp(const float* x, const float* lo, const float* hi);          // 0x40e3d0
float __cdecl fmin_ref(const float* a, const float* b);                 // 0x4324b0
float __cdecl fmax_ref(const float* a, const float* b);                 // 0x4324d0
extern float g_dt_scale;                                                // 0x5a9bf0
extern int g_int_5aa04c;                                                // 0x5aa04c
extern unsigned g_input_flags;                                         // 0x5e2470
extern int g_player_agent;                                              // 0x8ed50dc

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


// @ 0x4465e0
void Agent::update_ai()
{
    ClassInfo* cr = class_rec();

    int prev = -1;
    if (f55c0 >= 0 && AG(f55c0).f14 == 1) {
        Agent* o = &AG(f55c0);
        vector4 d;
        d.x = o->pos.x - pos.x;
        d.y = o->pos.y - pos.y;
        d.z = o->pos.z - pos.z;
        float len = d.normalize();
        bool far_ = true;
        if (target >= 0) {
            const vector4& cp = AG(target).pos;
            vector4 dd = a_vsub(cp, pos);
            far_ = !(sqrt(dd.y * dd.y + dd.z * dd.z + dd.x * dd.x) <= len + 1.0f);
        }
        if (far_ && is_enemy(o))
            target = f55c0;
        prev = f55c0;
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
                const vector4& op = AG(t).pos;
                vector4 dd = a_vsub(pos, op);
                float dist = (float)sqrt(dd.y * dd.y + dd.z * dd.z + dd.x * dd.x);
                ItemRef p = AG(t).cur_pair_fi();
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
            vector4 w, q;
            const vector4& op = AG(target).pos;
            w.x = op.x - pos.x;
            w.y = op.y - pos.y;
            w.z = op.z - pos.z;
            float dist = w.normalize();
            bool ranged = false;
            {
                ItemRef sp = AG(target).slot_pair(0);
                if (sp.item >= 0 && g_items[sp.item].is_ranged())
                    ranged = true;
            }
            f533c = 2;
            if (!ranged && dist > 15.0f && f2c < 0)
                f533c = 0;
            if (f314 < 0.0f)
                f314 = 0.0f;
            else if (f314 > 30.0f)
                f314 = 30.0f;
            vector4 acc[2];
            float wsum[2];
            q.x = q.y = q.z = 0.0f;
            acc[0] = q;
            acc[1] = q;
            wsum[0] = wsum[1] = 0.0f;
            float t98 = 0.0f;                       // cnt, later k and the clamp argument (one address-taken scratch)
            int best = -1;
            unsigned it = g_mission->agents.first_used();
            if (it < (unsigned)g_mission->agents.count) {
                do {
                    Agent* a = &AG(it);
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
                        q.z = dv.z * w2;
                        float* ap = &acc[idx].x;
                        ap[0] = dv.x * w2 + ap[0];
                        ap[1] = dv.y * w2 + ap[1];
                        ap[2] = q.z + ap[2];
                    }
                    g_mission->agents.next_used(&it);
                } while (it < (unsigned)g_mission->agents.count);
            }
            if (dist > 10.0f && best >= 0) {
                const vector4& bp = AG(best).pos;
                q.x = bp.x - pos.x;
                q.y = bp.y - pos.y;
                q.z = bp.z - pos.z;
                q.normalize();
                if (q.x * f294() + (q.y * f298() + q.z * f29c()) > -0.3f)
                    target = best;
            }
            bool close_ = false;
            if (cr->kind == 1) {
                vector4 d;
                hand_pos(&q);
                d.x = pos.x - q.x;
                d.y = pos.y - q.y;
                d.z = pos.z - q.z;
                float len = d.normalize();
                if (len > 5.0f) {
                    f2c8 = 1;
                    float r = 0.2f;
                    goal_near_for_class(&q, &r);
                    close_ = true;
                }
            }
            if (cr->kind == 2) {
                int t2 = team;
                if (t2 < 0 || t2 >= 8) t2 = 7;
                if (g_mission->teams[t2].leader != id) {
                    vector4 d;
                    hand_pos(&q);
                    d.x = pos.x - q.x;
                    d.y = pos.y - q.y;
                    d.z = pos.z - q.z;
                    float len = d.normalize();
                    if (len > 8.0f) {
                        f2c8 = 9;
                        goto L_tail4;
                    }
                }
            }
            if (!close_) {
                if (f2e4 == 0) {
                    vector4 d;
                    float diff = wsum[1] - wsum[0];
                    if (diff < 0.05f)
                        diff = 0.05f;
                    else if (diff >= 0.3f)
                        diff = 0.3f;
                    int r = rand() % 0x3dc9;
                    int hf = always_attack_in_melee;
                    float push = (((float)r * (1.0f / 15817.0f)) * diff * g_dt_scale) * 20.0f;
                    if (hf == 0)
                        push = ((float)g_int_5aa04c * 0.5f + (40.0f - (float)g_game->troops[troop].fb0) * 0.012f) + push;
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
                            if (g_mission->scene_world->hf)
                                g_mission->scene_world->hf->sample(pos.x, pos.y, wsum, &fid);
                            if (dist < 4.0f ||
                                (g_mission->scene_world->hf && (fbc == 0 || ((int*)fbc)[1] != fid))) {
                                set_goal_agent(target);
                            } else {
                                float r30 = 30.0f;
                                float m = fmin_ref(&dist, &r30);
                                float fr = frand();
                                AgentTable* tb = &g_mission->agents;
                                int u = target;
                                t98 = ((fr + 2.0f) * m) * 0.2f;
                                const vector4* pv = &tb->get(u)->vel;
                                d.x = t98 * pv->x;
                                d.y = t98 * pv->y;
                                d.z = t98 * pv->z;
                                pv = &tb->get(u)->vel;
                                q = *pv;
                                q.normalize();
                                float f = q.y * w.y + (q.z * w.z + q.x * w.x) + 1.0f;
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
                    vector4 d;
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
                        float z = acc[0].z * 4.0f;
                        q.x = acc[0].x * 4.0f - acc[1].x;
                        q.y = acc[0].y * 4.0f - acc[1].y;
                        q.z = z - acc[1].z;
                        q.normalize();
                        float r16 = 16.0f;
                        q.z = q.z * 15.0f;
                        d.x = q.x * 15.0f + pos.x;
                        d.y = q.y * 15.0f + pos.y;
                        d.z = q.z + pos.z;
                        set_goal_near(&d, &r16);
                    }
                }
            }
        }
    L_tail4:
        {
            unsigned fl = path_state;
            path_state = fl | 0x20;
            float s = 0.5f;
            ItemRef cp = cur_pair_fi();
            if (cp.item != -1)
                s = (float)g_items[cp.item].weight * 0.01f + 0.9f;
            if (fl & 2) {
                f288 = s;
                f28c = s - 0.4f;
            } else {
                f288 = 0.5f;
                f28c = 0.2f;
            }
        }
    } else if (mode_ == 5 && combat_state != 1 && combat_state != 3 && combat_state != 4) {
        float chance = hit_chance_ref(slot_pair(0), this);
        vector4 d, q, w;
        float t80 = 500.0f;
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            d = g_mission->agents.get(e)->pos - pos;
            near_ = d.normalize();
        }
        if (target >= 0 && g_mission->agents.get(target)->f14 != 1)
            target = -1;
        if (target == -1) {
            int c = f2cc;
            if (c >= 0 && g_mission->agents.get(c)->f14 == 1)
                target = c;
        }
        int tg = target;
        float cur;
        if (tg >= 0) {
            d = g_mission->agents.get(tg)->pos - pos;
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
            d = ta->pos - pos;
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
                q.x = pos.x - w.x;
                q.y = pos.y - w.y;
                q.z = pos.z - w.z;
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
                    d.z = d.z * 0.7f;
                    q.x = d.x * 0.7f + pos.x;
                    q.y = d.y * 0.7f + pos.y;
                    q.z = d.z + pos.z;
                    q.w = w.w;
                    w.x = q.x;
                    w.y = q.y;
                    w.z = q.z;
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
        vector4 d;
        hit_chance_ref(cur_pair_fi(), this);
        float near_ = 500.0f;
        int e = pick_target();
        if (e >= 0) {
            d.x = AG(e).pos.x - pos.x;
            d.y = AG(e).pos.y - pos.y;
            d.z = AG(e).pos.z - pos.z;
            near_ = d.normalize();
        }
        float cur;
        if (target >= 0) {
            d.x = AG(target).pos.x - pos.x;
            d.y = AG(target).pos.y - pos.y;
            d.z = AG(target).pos.z - pos.z;
            cur = d.normalize();
        } else {
            cur = 500.0f;
        }
        if (near_ < 8.0f && cur > near_ + near_)
            target = e;
        int r = rand() % 0x3dc9;
        float wsum[2];
        int c;
        if (((float)r * (1.0f / 15817.0f) < 0.2f || target == -1 ||
             g_mission->agents.get(target)->f14 != 1) &&
            (collect_enemies((int)wsum), nthreat > 0 && (c = (int)pick_threat()) >= 0)) {
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
            vector4 gp;
            const vector4* gpp;
            float rr;
            if (g_mission->agents.get(f2c)->fac > 2.0f) {
                gpp = &gp;
                rr = 15.0f;
                gp.z = vel.z * 25.0f;
                gp.x = vel.x * 25.0f + pos.x;
                gp.y = vel.y * 25.0f + pos.y;
                gp.z = gp.z + pos.z;
            } else {
                gpp = &pos;
                rr = 30.0f;
            }
            set_goal_near(gpp, &rr);
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
                vector4 q, w, d, r4;
                Agent* t1 = g_mission->agents.get(u2);
                q.x = k * t1->v56b4.x;
                q.y = k * t1->v56b4.y;
                q.z = k * t1->v56b4.z;
                t1 = g_mission->agents.get(u2);
                w.x = q.x + t1->pos.x;
                w.y = q.y + t1->pos.y;
                w.z = q.z + t1->pos.z;
                t1 = g_mission->agents.get(u2);
                r4 = t1->v56b4;
                r4.normalize();
                float f = r4.y * v.y + r4.x * v.x;
                if (f < 0.0f) {
                    f = f + 1.0f;
                    d.x = q.x * f;
                    d.y = q.y * f;
                    d.z = q.z * f;
                    t1 = g_mission->agents.get(u2);
                    q.x = d.x + t1->pos.x;
                    q.y = d.y + t1->pos.y;
                    q.z = d.z + t1->pos.z;
                    w.w = q.w;
                    w.x = q.x;
                    w.y = q.y;
                    w.z = q.z;
                }
                float val = dist * 0.35f - 0.7f;
                float hi = 25.0f;
                float lo = 0.0f;
                float c = clamp(&val, &lo, &hi);
                set_goal_near(&w, &c);
            }
        }
        unsigned fl = path_state;
        path_state = fl | 0x20;
        if (path_state & 2)
            path_state = fl | 0xa0;
        if (dist < 35.0f)
            path_state |= 0x200;
        f533c = 2;
    } else if (mode_ == 9) {
        flag_b = true;
        vector4 q, d;
        hand_pos(&q);
        f2d0 = q;
        ItemRef sp = slot_pair(0);
        d.x = q.x - pos.x;
        d.y = q.y - pos.y;
        d.z = q.z - pos.z;
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
            d = g_mission->agents.get(tg)->pos - pos;
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
            vector4 v;
            v.z = vel.z * 10.0f;
            float r15 = 15.0f;
            v.x = vel.x * 10.0f + pos.x;
            v.y = vel.y * 10.0f + pos.y;
            v.z = v.z + pos.z;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 0xe) {
        f533c = 0;
        target = -1;
        f288 = 3.0f;
        if (frand() < 0.3f) {
            vector4 v;
            v.z = vel.z * 20.0f;
            float r15 = 15.0f;
            v.x = vel.x * 20.0f + pos.x;
            v.y = vel.y * 20.0f + pos.y;
            v.z = v.z + pos.z;
            set_goal_near(&v, &r15);
        }
    } else if (mode_ == 3) {
        reset();
        if (fac < 0.5f && f2c != -1)
            g_input_flags |= 0x10000;
    } else if (mode_ == 2) {
        if (f2c == -1) {
            int c = f2cc;
            if (c != -1 && g_mission->agents.get(c)->f14 == 1) {
                Agent* ca = g_mission->agents.get(c);
                vector4 d = ca->pos - pos;
                float l = d.out_length();
                vector4 w, q;
                if (l > 0.99f) {
                    flag_a = true;
                    AgentBody* b = ca->body;
                    if (l > 2.0f) {
                        float sz = b->m.s.z;
                        w.x = ca->pos.x - b->m.s.x;
                        w.y = ca->pos.y - b->m.s.y;
                        w.z = ca->pos.z;
                        path_state = 0x21;
                        w.z = w.z - sz;
                    } else {
                        float sz = b->m.s.z * 0.2f;
                        w.x = ca->pos.x - b->m.s.x * 0.2f;
                        w.y = ca->pos.y - b->m.s.y * 0.2f;
                        w.z = ca->pos.z;
                        path_state = 1;
                        w.z = w.z - sz;
                    }
                    q = w;
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
        } else if (attack_delay() > 1.0f) {
            attack_delay() = 1.0f;
        }
    }

done:
    if (g_mission->terrain.f184 != 0 && id == g_player_agent) {
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
                    vector4 d;
                    d.x = ta->pos.x - pos.x;
                    d.y = ta->pos.y - pos.y;
                    d.z = ta->pos.z - pos.z;
                    t->dir = d;
                    float len = t->dir.normalize();
                    t->dist = len;
                    if (len < range) {
                        AgentBody* b = body;
                        score = 10.0f / (len + 1.0f) +
                                (b->m.f.z * t->dir.z + b->m.f.y * t->dir.y + b->m.f.x * t->dir.x) * 0.1f;
                    } else {
                        if (f2c == -1) {
                            ItemRef p1 = slot_pair_fi(1);
                            if (p1.item >= 0) {
                                ItemRef p1b = slot_pair(1);
                                if ((char)g_items[p1b.item].type_flags == itp_type_bow) {
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
        if (bestid < 0)
            target = -1;
        else
            target = bestid;
        f533c = (target == -1) ? 0 : 2;
    }
    if (flag_b && f10 == 0 && target >= 0) {
        vector4 ta_ = a_vadd(AG(target).v56b4, AG(target).pos);
        vector4 tb_ = a_vadd(v56b4, pos);
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

// @ 0x448eb0
void Agent::ai_update_control()
{
    vector4* P = &pos;
    vector4 tgt = *P;                                   // movement target
    vector4 d;                                          // direction to the target
    float dlen;
    vector4 eye;
    float att, mz, mlen, strafe;
    unsigned fl2;
    int mid;
    float angle;
    vector4 op;

    if (fbc == 0)
        return;

    // ---- 1. keep the path / goal up to date ----------------------------------------------------------------------
    if (path_active != 0) {
        if (path_state & 2) {
            const vector4* tp = &agent_by_id(f318)->pos;
            vector4 dd(tp->x - goal.x, tp->y - goal.y, tp->z - goal.z);
            if ((float)sqrt(dd.y * dd.y + (dd.z * dd.z + dd.x * dd.x)) > 4.0f &&
                goal_face != agent_by_id(f318)->update_nav_face()) {
                unsigned fid = f318;
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
        tgt = *P;
        if (fl != 0) {
            if ((fl & 2) != 0) {
                unsigned fid = f318;
                vector4 lv;
                vector4* v = agent_by_id(fid)->local_velocity(&lv);
                float vz = v->z * 0.8f;
                vector4 np(v->x * 0.8f + agent_by_id(fid)->pos.x, v->y * 0.8f + agent_by_id(fid)->pos.y,
                           vz + agent_by_id(fid)->pos.z);
                tgt = np;
                if ((char)fl < 0) {
                    float m1 = -1.0f;
                    float k = 0.4f;
                    if (f100_is_0x48_and_charged())
                        k = 0.25f;
                    float m = m1;
                    if (anim[1].cur == 0xb9 || anim[1].cur == 0xb8)
                        m = 1.0f;
                    float s = m * k;
                    float oy = -vel.x * s;
                    tgt.x = tgt.x + vel.y * s;
                    tgt.y = tgt.y + oy;
                }
                goal = agent_by_id(fid)->pos;
                goal_face = agent_by_id(fid)->update_nav_face();
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
    d.x = tgt.x - P->x;
    d.y = tgt.y - P->y;
    d.z = tgt.z - P->z;
    dlen = d.normalize();

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
            vector4 t1(d.x * a + P->x, d.y * a + P->y, d.z * a + P->z);
            cap.p0 = t1;
            vector4 t2(d.x * b + P->x, d.y * b + P->y, d.z * b + P->z);
            cap.p1 = t2;
            cap.p0.z = cap.p0.z + 1.3f;
            cap.p1.z = cap.p1.z + 1.3f;
            vector4 bmin, bmax;
            cap.get_min(&bmin);
            cap.get_max(&bmax);
            shape_base* shapes[8188];
            int count;
            g_mission->scene_world->query_box(&bmin.x, &bmax.x, 2, shapes, &count);
            shape_base* hit;
            int found = 0;
            int i = 0;
            vector4 nrm;
            contact contact;
            if (count > 0) {
                do {
                    if (found != 0)
                        goto process;
                    {
                        shape_base* sh = shapes[i];
                        found = shape_contact_count(&cap, sh, 1, &contact);
                        if (found != 0) {
                            nrm = contact.normal;
                            hit = sh;
                        }
                    }
                    i++;
                } while (i < count);
                if (found != 0) {
process:
                    if (hit->shape_type() != 2) {
                        float cross = d.x * -nrm.y - d.y * -nrm.x;
                        if (cross > 0.0f) {
                            float t = (frand() + 1.0f) * 0.5f;
                            veer = (-0.7f - veer) * t + veer;
                        } else {
                            float t = (frand() + 1.0f) * 0.5f;
                            veer = (0.7f - veer) * t + veer;
                        }
                    }
                }
            }
            if (f2c >= 0) {
                EntVec ents;
                ents.first = 0;
                ents.last = 0;
                ents.end = 0;
                g_mission->scene_world->collect_boxed(&bmin, &bmax, (std::vector<unsigned>*)&ents);
                SceneEnt** pe = ents.first;
                if (pe < ents.last) {
                    do {
                        SceneEnt* e = *pe;
                        if (e->shape != 0 && e->type == 0xb && e->id != id && e->id != (unsigned)f2c &&
                            e->id != (unsigned)f30) {
                            Agent* o = agent_by_id(e->id);
                            if (o->f10 == 1 && shape_contact_count(&cap, e->shape, 1, &contact) != 0) {
                                posm m = body->m;
                                int flag = 1;
                                float ang = 0.01f;
                                ((rgl::matrix&)m).rotate_about_u(&ang);
                                AgentBody* ob = o->body;
                                float lhs = (float)fabs(m.f.y * ob->m.f.y + m.f.z * ob->m.f.z + m.f.x * ob->m.f.x);
                                if (fabs(rgl::dot(body->m.f, ob->m.f)) > lhs)
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
        float nv;
        if (v56b4.length() < 0.3f) {
            if (f2c >= 0) {
                if (!(speed > 0.9f))
                    goto weave_up;
                if (!(f5570 < -0.95f)) {
                    nv = f5570 - 0.03f;
                } else {
                    f2c8 = 3;
                    nv = f5570 - 0.03f;
                }
            } else {
                if (f10 != 0 || (f5390 & 1) == 0)
                    goto weave_up;
                nv = f5570 - 0.03f;
            }
        } else {
weave_up:
            nv = f5570 + 0.2f;
        }
        f5570 = nv;
        if (f5570 < -1.0f)
            f5570 = -1.0f;
        else if (f5570 > 1.0f)
            f5570 = 1.0f;
        if (f5570 < -0.1f) {
            if ((float)(g_ticks[clock] - deadline) * 1e-5f >= 0.0f) {
                f5570 = 1.0f;
                path_active = 0;
                goal_changed = 1;
                deadline = g_ticks[clock] + 6000000;
            } else {
                if (veer < 0.0f) {
                    int r = rand() % 0x3dc9;
                    veer = veer - ((float)r * 6.3223117e-05f + 0.5f) * (0.3f - f5570);
                } else {
                    int r = rand() % 0x3dc9;
                    veer = ((float)r * 6.3223117e-05f + 0.5f) * (0.3f - f5570) + veer;
                }
                int r = rand() % 0x3dc9;
                if ((float)r * 6.3223117e-05f < 0.01f)
                    veer = -veer;
            }
        }
    }
    if (veer < -0.03f || 0.03f < veer) {
        double ca = cos((double)veer);
        double sa = sin((double)veer);
        vector4 r;
        r.x = (float)(ca * d.x - d.y * sa);
        r.y = (float)((d.y * ca + sa * d.x) * dlen);
        r.z = d.z * dlen;
        vector4 nt(r.x * dlen + P->x, r.y + P->y, r.z + P->z);
        tgt = nt;
    }

    // ---- 4. eye position and move direction ----------------------------------------------------------------------------
    eye = pos;
    {
        float h;
        if (f10 == 0 && f2c != -1)
            h = 2.4f;
        else
            h = 1.5f;
        eye.z = h + eye.z;
    }
    att = attack_speed();
    fl2 = f533c;
    if ((fl2 & 1) != 0) {
        move_dir() = vector4(wp_x - eye.x, wp_y - eye.y, wp_z - eye.z);
    } else if ((fl2 & 2) == 0 || target == 0xffffffff) {
        move_dir() = vector4(tgt.x - P->x, tgt.y - P->y, tgt.z - P->z);
        if (move_dir().length() < 0.001f)
            move_dir() = vel;
    } else {
        Agent* o = agent_by_id(target);
        op = o->pos;
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
        op.z = op.z - (float)(f55c4 & 2) * 0.8f;
lead:
        if ((fl2 & 0x20) != 0 && att > 0.1f) {
            float dx = op.x - eye.x;
            float dy = op.y - eye.y;
            float dz = op.z - eye.z;
            float dist = (float)sqrt(dx * dx + dz * dz + dy * dy);
            float s = 40.0f - (float)g_game->troops[troop].fb0;
            if (0.0f > s)
                s = 0.0f;
            float j = (frand() - 0.5f) * s * 0.01f + 1.0f;
            float k = (float)g_game->troops[troop].fb0 * 0.02f + 0.6f;
            if (!(k < 1.0f))
                k = 1.0f;
            k = (dist / att) * k;
            float ox = (k * o->v56b4.x) * j;
            float oy = (k * o->v56b4.y) * j;
            float oz = (k * o->v56b4.z) * j;
            op.x = ox + op.x;
            op.y = oy + op.y;
            op.z = oz + op.z;
        }
lead_done:
        move_dir() = vector4(op.x - eye.x, op.y - eye.y, op.z - eye.z);
    }
    mz = move_dir().z;
    mlen = move_dir().normalize();
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

    // ---- 5. movement input bits ---------------------------------------------------------------------------------------
    mid = f2c;
    d.x = tgt.x - P->x;
    d.y = tgt.y - P->y;
    d.z = tgt.z - P->z;
    strafe = 0.0f;
    f5368 = d.x;
    f536c = d.y;
    angle = 0.0f;
    if (mid == -1 && f10 == 0) {
        float mx = move_dir().x, my = move_dir().y;
        float ml2 = (float)sqrt(my * my + mx * mx);
        double nx, ny;
        if (ml2 > 0.0f) {
            float inv = 1.0f / ml2;
            nx = mx * inv;
            ny = my * inv;
        } else {
            nx = 0.0;
            ny = 1.0;
        }
        float lx = (float)(d.x * ny - d.y * nx);
        float ly = (float)(d.x * nx + d.y * ny);
        float fa, fb;
        if (path_active == 0) {
            fa = f288 * 1.4f;
            fb = f288 * 0.9f;
        } else {
            fa = 0.0f;
            fb = 0.0f;
        }
        float l2 = (float)sqrt(ly * ly + lx * lx);
        if (l2 > 0.0f) {
            float inv = 1.0f / l2;
            lx = lx * inv;
            ly = ly * inv;
        } else {
            lx = 0.0f;
            ly = 1.0f;
        }
        unsigned f = f5390;
        vec2 mv;
        mv.x = 0.0f;
        mv.y = 0.0f;
        if (((f & 1) != 0 && fb < l2) || ((f & 1) == 0 && fa < l2)) {
            if (((f & 2) != 0 && ly > 0.35f) || ((f & 2) == 0 && ly > 0.45f)) {
                f |= 2;
                in_prev.v[0] |= 1;
                mv.y = 1.0f;
            } else if (((f & 4) != 0 && ly < -0.35f) || ((f & 4) == 0 && ly < -0.45f)) {
                f |= 4;
                in_prev.v[0] |= 2;
                mv.y = -1.0f;
            } else {
                f &= 0xfffffff9;
            }
            f5390 = f;
            if (((f & 8) != 0 && lx > 0.35f) || ((f & 8) == 0 && lx > 0.45f)) {
                f |= 8;
                in_prev.v[0] |= 4;
                mv.x = 1.0f;
            } else if (((f & 0x10) != 0 && lx < -0.35f) || ((f & 0x10) == 0 && lx < -0.45f)) {
                f |= 0x10;
                in_prev.v[0] |= 8;
                mv.x = -1.0f;
            } else {
                f &= 0xffffffe7;
            }
            f5390 = f;
            f5390 = f | 1;
            mv.normalize();
            strafe = lx * mv.y - mv.x * ly;
        } else {
            f5390 = 0;
        }
        if ((path_state & 0x20) != 0 && speed_limit > 2.6f)
            in_prev.v[0] = in_prev.v[0] | 0x200000;
    } else {
        // ---- follow / mount case ----
        vector4 gp = tgt;
        if (path_active != 0)
            gp = goal;
        d.x = gp.x - P->x;
        d.y = gp.y - P->y;
        d.z = gp.z - P->z;
        float gdist = (float)sqrt(d.x * d.x + d.z * d.z + d.y * d.y);
        float hi = 0.8f, lo = 0.5f;
        if (path_active == 0) {
            hi = f288 * 1.2f;
            lo = f288 * 0.8f;
        }
        if ((f5390 != 0 && lo < gdist) || (f5390 == 0 && hi < gdist)) {
            f5390 = 1;
            float t_x = tgt.x - P->x;
            float t_y = tgt.y - P->y;
            float h_y = vel.y;
            double hl = sqrt(h_y * h_y + vel.x * vel.x);
            double hx, hy;
            if (hl > 0.0) {
                float inv = (float)(1.0 / hl);
                hx = vel.x * inv;
                hy = h_y * inv;
            } else {
                hx = 0.0;
                hy = 1.0;
            }
            float tl = (float)sqrt(t_y * t_y + t_x * t_x);
            double tx, ty;
            if (tl > 0.0f) {
                float inv = 1.0f / tl;
                tx = t_x * inv;
                ty = t_y * inv;
            } else {
                tx = 0.0;
                ty = 1.0;
            }
            float ang = (float)atan2(-(hy * tx - ty * hx), hy * ty + hx * tx);
            float sp = gdist * 0.7f;
            if (sp < 1.0f)
                sp = 1.0f;
            else if (speed_limit <= sp)
                sp = speed_limit;
            speed = sp;
            float gv = goal_dir.length();
            if (sp < gv && d.x * goal_dir.x + (d.y * goal_dir.y + d.z * goal_dir.z) > 0.1f)
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
            else if (speed_limit < speed)
                speed = speed_limit;
        } else {
            speed = 0.0f;
            f5390 = 0;
        }
        unsigned who = id;
        if (f10 == 0)
            who = mid;
        Agent* wa = agent_by_id(who);
        if (wa->fac < speed - 0.2f) {
            in_prev.v[0] = in_prev.v[0] | 0x10;
        } else if (speed + 0.2f < wa->fac && (path_state & 0x200) == 0) {
            in_prev.v[0] = in_prev.v[0] | 0x20;
        }
    }

    // ---- 6. combat state machine -----------------------------------------------------------------------------------------
    if (g_mission->terrain.f184 == 0 && f10 == 0 && target != 0xffffffff && (f533c & 2) != 0) {
        Agent* o = agent_by_id(target);
        if (!is_enemy(o) || agent_by_id(target)->f14 != 1)
            goto no_target;
        o = agent_by_id(target);
        vec2 tv;
        tv.x = o->pos.x - P->x;
        tv.y = o->pos.y - P->y;
        float dist = tv.normalize();
        ItemRef p = slot_pair(0);
        float hc, fm, zt, one;
        unsigned itype;
        ItemKind* wk;
        float reach0, facing, ofacing, wprof, r2f, distL;
        int idx;
        ItemRef q, q2, op0, op1;
        bool b3e, b3f, b58, b59, b5a, b5b;
        if (p.item == -1 || !g_items[p.item].is_ranged())
            goto melee;

        // ---- ranged weapon ------------------------------------------------------------------------------------------------
        itype = g_items[p.item].type_flags & 0xff;
        hc = hit_chance_ref(p, this);
        one = 1.0f;
        zt = (o->pos.z - P->z) * 0.7f + dist;
        fm = fmax_ref(&zt, &one);
        if (combat_state == 0) {
            if ((itype == itp_type_crossbow || itype == itp_type_pistol || itype == itp_type_musket) &&
                p.max_amount() == (int)(p.m4 & 0xff)) {
                ai_timer.start = g_ticks[ai_timer.clock];
                combat_state = 4;
                goto bb72;
            }
            if (fm >= hc) {
set7:
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
                    ((gui_timer*)&ai_timer)->set_delay(-delay);
                    goto bb72;
                }
                if (r != 0) {
                    if (r == 3) {
                        combat_state = 8;
                    } else {
                        f2e0 = 2;
                        f2c0 = 0;
                    }
                    goto bb72;
                }
                last_hit = g_hit_shape;
                int t = (int)pick_threat();
                if (t < 0)
                    goto set7;
                target = t;
            }
            goto bb72;
        }
        if (combat_state == 1) {
            // state 1
            f53bc = f53bc | 0x100;
            if (f2c == -1) {
                if (dist < 2.5f && (o->f2c == -1 || o->v56b4.out_length() < 1.5f)) {
                    f2c0 = 0;
                    combat_state = 3;
                }
                if ((itype == itp_type_crossbow || itype == itp_type_bow) && ai_timer.elapsed() > -0.7f)
                    in_prev.v[0] = in_prev.v[0] & 0xfffffff0;
            }
            if (!(ai_timer.elapsed() > 0.0f))
                goto bb72;
            {
                int blocker;
                if (fire_check(target, &blocker) == 1) {
                    ((gui_timer*)&ai_timer)->set_delay(-0.3f);
                    combat_state = 3;
                    goto bb72;
                }
            }
            f53bc = f53bc | 0x200;
            goto bb68;
        }
        if (combat_state == 3) {
            int m = f2c;
            if (m == -1 && dist < 4.0f && (o->f2c == -1 || o->v56b4.out_length() < 1.0f)) {
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
            if (m == -1 && dist < 2.5f && (o->f2c == -1 || o->v56b4.out_length() < 1.5f)) {
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
            Agent* t = agent_by_id(target);
            vec2 tv2;
            tv2.x = t->pos.x - P->x;
            tv2.y = t->pos.y - P->y;
            tv2.normalize();
            float fx = body->m.f.x;
            float fy = body->m.f.y;
            if (f2c == -1 || agent_by_id(f2c)->fac > 0.5f)
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

melee:
        // ---- melee / no ranged weapon -----------------------------------------------------------------------------------
        reach0 = (frand() - 0.5f) * (float)(g_difficulty + 1) * 0.15f + 0.9f;
        facing = tv.y * vel.y + tv.x * vel.x;
        ofacing = -tv.x * o->vel.x + -tv.y * o->vel.y;
        {
            double r1 = frand() * 0.2f + 1.0f;
            float lx = (float)(r1 * v56a4.x + P->x);
            double ly = r1 * v56a4.y + P->y;
            float lz = (float)(r1 * v56a4.z) + P->z;
            float olx = (float)(r1 * o->v56a4.x + o->pos.x);
            double oly = r1 * o->v56a4.y + o->pos.y;
            float olz = (float)(r1 * o->v56a4.z) + o->pos.z;
            vector4 dd;
            dd.x = olx - lx;
            dd.y = (float)(oly - ly);
            dd.z = olz - lz;
            idx = p.item;
            float outlen = dd.out_length();
            if (idx < 0)
                idx = 0;
            wk = &g_items[idx];
            q = slot_pair(1);
            b3f = false;
            wprof = 50.0f;
            if (idx != 0) {
                q2 = slot_pair(1);
                int cls = slot_class_impl(p.item, q2.item >= 0);
                if (cls >= 0 && cls < 7)
                    wprof = g_game->troops[troop].f80[cls];
                reach0 = (100.0f / (float)p.stat_1f0a()) * reach0;
            }
            if (combat_state == 3 && active == 1)
                reach0 = reach0 * 0.5f;
            vector4 l2(reach0 * o->v56a4.x + o->pos.x, reach0 * o->v56a4.y + o->pos.y, reach0 * o->v56a4.z + o->pos.z);
            vector4 dv(l2.x - P->x, l2.y - P->y, l2.z - P->z);
            float dl = rgl::length(dv);
            float vl = rgl::length(v56a4);
            float reach2 = vl * reach0;
            int mc = f2c;
            float distv;
            if (mc < 0) {
                distv = dl - reach2;
            } else {
                float lpx = (float)(reach0 * v56a4.x + P->x);
                double lpy = reach0 * v56a4.y + P->y;
                double lpz = reach0 * v56a4.z + P->z;
                vector4 d3;
                d3.x = l2.x - lpx;
                d3.y = (float)(l2.y - lpy);
                d3.z = (float)(l2.z - lpz);
                float l3 = rgl::length(d3);
                if (l3 < dl - reach2 * 0.5f) {
                    distv = dl - reach2;
                } else {
                    vector4 df = o->pos - pos;
                    distv = rgl::length(df) + 0.2f;
                }
            }
            distL = distv;
            if (q.item >= 0 && (g_items[q.item].type_flags & 0xff) == itp_type_shield)
                b3f = true;
            float reachf = 2.5f;
            if (p.item != -1)
                reachf = (float)g_items[p.item].weight * 0.01f + 1.1f;
            if (mc >= 0)
                reachf = reachf - 0.3f;
            if (o->f2c >= 0)
                reachf = reachf + 0.7f;
            op0 = o->slot_pair(0);
            float r2base = 2.5f;
            if (op0.item != -1)
                r2base = (float)g_items[op0.item].weight * 0.01f + 1.9f;
            r2f = r2base * 1.3f;
            b5b = false;
            op1 = o->slot_pair(1);
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
            int st = combat_state;
            if (st < 2) {
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
                        ((gui_timer*)&ai_timer2)->set_delay(-((frand() + frand()) + 1.3f));
                        combat_state = 7;
                    }
                    goto bb72;
                }
                if (b5a && !b3e && (frand() < wprof * 0.004f + 0.5f || f2c >= 0)) {
                    ((gui_timer*)&ai_timer)->set_delay(0.0f);
                    combat_state = 3;
                    goto bb72;
                }
                if (b3f) {
                    if (op0.item == -1)
                        goto b53e;
                    if (b3e || !g_items[op0.item].is_melee())
                        goto b535;
                    if (ai_timer2.elapsed() > 0.0f &&
                        ((o->active == 1 && frand() < 0.3f - (float)g_game->troops[troop].fb0 * 0.01f) ||
                         (o->active == 2 && frand() < (float)g_game->troops[troop].fb0 * 0.05f + 0.5f)) &&
                        b58) {
                        goto b508;
                    }
                    goto b535;
                }
b535:
                if (op0.item == -1) {
b53e:
                    if (p.item == -1 && ai_timer2.elapsed() > 0.0f && ofacing > 0.0f &&
                        ((o->active == 1 && frand() < (float)g_game->troops[troop].fb0 * 0.01f + 0.5f) ||
                         (o->active == 2 && frand() < (float)g_game->troops[troop].fb0 * 0.05f + 0.5f)) &&
                        outlen < r2f) {
b508:
                        ((gui_timer*)&ai_timer2)->set_delay(-((frand() + frand()) + 1.3f));
                        combat_state = 7;
                        goto bb72;
                    }
                }
                if (!b3f && !b3e && wk->is_melee() && (wk->type_flags & 0x4000) == 0 && ai_timer2.elapsed() > 0.0f &&
                    op0.item != -1 && g_items[op0.item].is_melee() && o->active != 0 && b58 &&
                    frand() < wprof * 0.01f + 0.1f) {
                    ((gui_timer*)&ai_timer2)->set_delay(-(frand() * 0.5f + 0.6f));
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
                    ((gui_timer*)&ai_timer)->set_delay(-0.2f);
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
                        ((gui_timer*)&ai_timer)->set_delay(-((4.9f / (float)sk + (float)g_difficulty * 0.25f + 0.6f) * (100.0f / (float)sv)));
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
                        } else if (frand() < ((4.0f - (float)g_difficulty) * wprof) * 0.0005f) {
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
                        ((gui_timer*)&ai_timer2)->set_delay(0.0f);
                        ((gui_timer*)&ai_timer)->set_delay(-0.2f);
                        combat_state = 3;
                    } else {
                        ((gui_timer*)&ai_timer2)->set_delay(0.0f);
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
    int old = slots[which];
    bool found = false;
    int old_item = -1;
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
                            int cnt = pairs[v].m4 & 0xff;
                            int val = pairs[v].max_amount();
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
                        if ((t == 7 || ((fl & 0x800000) && t == 7)) &&
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
    SceneEnt* e = (SceneEnt*)((rgl::world*)scene_world)->create_entity(9);
    pos2 ident;
    identity_pos((::pos*)&ident);
    res_obj* m;
    // a launcher of type 10 names the flying mesh itself (0x44bed6..0x44befd read the third argument); otherwise
    // the ammo item's own mesh (0x44bf04.. read the fourth)
    if ((g_items[launcher->item].type_flags & 0xff) == 10) {
        tstring tn = launcher->mesh_name();
        m = (res_obj*)RESMGR->get_object((const rgl::string*)&tn);
    } else {
        m = 0;
        if (ammo->item >= 0 && ammo->item < g_item_kind_count)
            m = g_items[ammo->item].mesh_for(0x2000000000000000ui64, ((signed char*)ammo)[7]);
        if (!m) {
            tstring name("flying_missile");
            m = (res_obj*)RESMGR->get_object((const rgl::string*)&name);
        }
    }
    if (m) {
        res_obj* c = m->clone();
        c->xform = ident;
        e->add_extra(c);
    }
    e->local.f = *dir;
    e->local.u = vector4(0.0f, 0.0f, 1.0f);
    e->local.orthonormalize();
    e->local.o = *pos;
    float half = 0.5f;
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
// views of tables of other objects (offsets read here): g_tbl20 entries 0x48 bytes at +0x1c, count at +0x18; g_tbl78 records
// flags +0x10, meshes +0x40, shape +0x50, sounds +0x6c; ItemKind sounds +0x228
struct Rec20Entry { tstring name; char pad10[0x20 - 0x10]; res_shape* shape; char pad24[0x48 - 0x24]; };   // +0x20 the body resource
struct Rec20V { char pad0[0x18]; int count; Rec20Entry* entries; };
static inline Rec20V& r20(int i) { return *(Rec20V*)&g_tbl20[i]; }
static inline unsigned r78_flags(Rec78* r) { return *(unsigned*)((char*)r + 0x10); }
static inline std::vector<res_obj*>& r78_meshes(Rec78* r) { return *(std::vector<res_obj*>*)((char*)r + 0x40); }
static inline res_shape* r78_shape(Rec78* r) { return *(res_shape**)((char*)r + 0x50); }
static inline SoundSet& rsnd(Rec78* r) { return *(SoundSet*)((char*)r + 0x6c); }
static inline SoundSet& isnd(ItemKind* k) { return *(SoundSet*)((char*)k + 0x228); }
extern int g_flag5a9fb0;                                // 0x5a9fb0
#define P_ENT(r) (*(SceneEnt**)&(r)->m1b4)              // PlacedObj +0x1b4 / +0x1b8: the scene entities of the prop
#define P_ENT2(r) (*(SceneEnt**)&(r)->m1b8)
#define ents (*(EntVec*)((char*)this + 0x63c0))         // Mission +0x63c0: the entity list of the props
static inline vector4 vmul(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
// mesh of an item for the prop display class, null for an invalid item index (bounds check inlined)
static inline res_obj* item_mesh(int item, int packed)
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
    res_shape* shape_res = 0;
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
        res_obj* m;
        if (!((k == 2 || k == 3 || k == 4 || k == 6 || k == 5 || k == 0x12) && (m = item_mesh(ref.item, (int)ref.m4)) != 0))
            name = ref.mesh_name();
        else
            meshes.push_back(m);
    } else if (t == 4) {
        int ti = find_in_table20(&r->name);
        r->value = r->value % r20(ti).count;
        name = r20(ti).entries[r->value].name;
        res_shape* rs = r20(ti).entries[r->value].shape;
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
        if (tbl < 0 || tbl >= g_tbl78_count) {
            tbl = -1;
        } else {
            Rec78* rec = &g_tbl78[tbl];
            if (g_flag5a9fb0 != 0 || !(r78_flags(rec) & 0x4000))
                meshes = r78_meshes(rec);
            shape_res = r78_shape(rec);
        }
    } else if (t != 3) {
        if (t == 1) {
            if (g_flag5a9fb0)
                name.assign("entry_arrow");
            else
                name.assign("");
        } else if (t == 5) {
            if (g_flag5a9fb0)
                name.assign("_mesh_passage");
            else
                name.assign("");
            s2.assign("bo_mesh_passage");
        }
    }
after:
    if (name.len > 0)
        {
            std::vector<rgl::mesh*> found = RESMGR->find_meshes((const rgl::string*)&name);
            meshes = *(const std::vector<res_obj*>*)&found;
        }
    if (s2.differs("0") && s2.len != 0)
        shape_res = (res_shape*)RESOURCE_MANAGER->get_object60(*(const rgl::string*)&s2);

    if (r->kind == 7) {
        r->id = find_item(&r->name);
        SceneEnt* e = (SceneEnt*)((rgl::world*)scene_world)->create_entity(7);
        res_obj** first = meshes._Myfirst;
        for (unsigned i = 0; first != 0 && i < (unsigned)(meshes._Mylast - first); i++)
            e->add_extra(first[i]->clone());
        if (shape_res)
            e->shape = ((shape_resource*)shape_res)->clone_shape();
        e->local = r->m98;
        e->type = 0xc;
        e->id = *idx;
        P_ENT2(r) = e;
        e->f170 = 2.0f;
        float a = (float)(rand() % 0x3dc9) * (float)(1.0 / 15817);
        float b = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817) + 1.0f;
        float c = (float)(rand() % 0x3dc9) * (float)(2.0 / 15817);
        e->v19c = a_vadd(a_vadd(vmul(c, e->local.f), vector4(0.0f, 0.0f, b)), vmul(a, e->local.u));
        float a2 = (float)(rand() % 0x3dc9) * (float)(9.0 / 15817);
        float c2 = (float)(rand() % 0x3dc9) * (float)(4.0 / 15817);
        float d = (float)(rand() % 0x3dc9) * (float)(6.0 / 15817);
        e->f15c = 0.4f;
        e->f1d4 = 0;
        e->v1ac = a_vadd(a_vadd(vmul(d, r->m18.s), vmul(c2, r->m18.f)), vmul(a2, r->m18.u));
    } else {
        SceneEnt* e = (SceneEnt*)((rgl::world*)scene_world)->create_entity(9);
        for (unsigned i = 0; i < meshes.size(); i++) {
            res_obj* m = meshes[i]->clone();
            if (r->kind == 4) {
                vector4 dir, col;
                ((level*)scene_world)->light_at(r->m18.o, vector4(0.0f, 0.0f, 1.0f), &dir, &col, 1, 1.0f);
            }
            if (r->kind == 0 && tbl >= 0 && tbl < g_tbl78_count && (r78_flags(&g_tbl78[tbl]) & 0x12000))
                m->f10c = 0xffff;
            else
                m->f10c = 0x1ffff;
            if (r->v158.z * r->v158.y * r->v158.x < 0.0f)
                m->f4c = 1;
            e->add_extra(m);
        }
        if (shape_res) {
            if (r->kind == 3 || r->kind == 5 ||
                (r->kind == 0 && tbl >= 0 && ((r78_flags(&g_tbl78[tbl]) & 0xff) == 5 || (r78_flags(&g_tbl78[tbl]) & 0xff) == 10)))
                e->shape = ((body_res*)shape_res)->copy_shape();
            shape_base* sh = ((shape_resource*)shape_res)->clone_shape();
            int v = 8;
            if (r->kind == 0 && tbl >= 0) {
                unsigned fl = r78_flags(&g_tbl78[tbl]);
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
                ((world_grid*)scene_world)->reg_shape((grid_shape*)sh);
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
        if (ri >= 0 && ri < g_tbl78_count && rsnd(&g_tbl78[ri]).count > 0) {
            SoundSet* ss = &rsnd(&g_tbl78[ri]);
            ((Placement*)g_game)->set_matrix(&r->m18);
            g_game->f1a1f0 = *idx;
            SceneEnt* en = P_ENT(r);
            if (en->meshes.size() > 0)
                g_game->f1a200 = (int)en->meshes[0];
            else
                g_game->f1a200 = 0;
            g_game->f1a1fc = 0;
            g_a70448 = *idx;
            ss->play(-0x28);
        }
    } else if (ty == 2 || ty == 6 || ty == 7) {
        int ri = r->id;
        if (ri >= 0 && ri < g_item_kind_count && isnd(&g_items[ri]).count > 0) {
            SoundSet* ss = &isnd(&g_items[ri]);
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
            g_a70448 = *id;
            g_a70450 = aid;
            ss->play(-0x32);
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

    f55c0 = -1;
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
                            float d = vel.y * threats[i].dir.y + (threats[i].dir.z * vel.z + threats[i].dir.x * vel.x);
                            if (0.0f < d) w = (d * d * 20.0f + 1.0f) * w;
                            float sc = ((float)(rand() % 15817) * 6.3223117e-05f + 0.25f) * w * 0.5f;
                            if ((AG(threats[i].id).f130 & 0x10) || (AG(threats[i].id).f130 & 0x20))
                                sc = sc * 1.5f;
                            if (bests < sc) {
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
                        float dx = o->pos.x - cr->pos.x;
                        float dy = o->pos.y - cr->pos.y;
                        float dz = o->pos.z - cr->pos.z;
                        if (cr->half + cr->half < (float)sqrt(dx * dx + (dy * dy + dz * dz)) && 10.0f < dist)
                            s[7] = s[7] * 0.0f;
                    }
                    if (k == 2 && leader != id) {
                        hand_pos(&hp);
                        float dx = hp.x - pos.x;
                        float dy = hp.y - pos.y;
                        float dz = hp.z - pos.z;
                        float l = (float)sqrt(dy * dy + (dz * dz + dx * dx)) * 0.25f;
                        if (l < 0.15f)
                            l = 0.15f;
                        else if (l >= 4.0f)
                            l = 4.0f;
                        s[9] = l + 1.0f;
                        pv[1] = AG(leader).pos;
                        pv[9] = AG(leader).pos;
                    } else if (k == 1) {
                        hand_pos(&hp);
                        float dx = hp.x - pos.x;
                        float dy = hp.y - pos.y;
                        float dz = hp.z - pos.z;
                        float l = (float)sqrt(dy * dy + (dz * dz + dx * dx)) * 0.25f;
                        if (l < 0.15f)
                            l = 0.15f;
                        else if (l >= 4.0f)
                            l = 4.0f;
                        s[1] = l + 1.0f;
                        pv[1] = hp;
                    }
                    ids[9] = cand;
                    ids[1] = cand;
                } else if (k == 3) {
                    pv[1] = pos;
                    s[1] = 1.5f;
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
                                        float dx = a->pos.x - pos.x;
                                        float dy = a->pos.y - pos.y;
                                        float dz = a->pos.z - pos.z;
                                        float dd = (float)sqrt(dy * dy + (dz * dz + dx * dx));
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
                if (cr->k4 == 2 && f2c != -1 && 5.0f < dist) s[3] = 50.0f;
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
        g_input_mask = f53bc;
        in_prev.v[0] = 0;
        f53bc = 0;
        ai_update_control();
        ai_tc.start = g_ticks[ai_tc.clock];
        E32 e;
        float r1 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        e.v[0] = f53bc;
        float hi = (float)g_difficulty * 0.02f + 0.2f / (float)(g_game->troops[troop].fb0 + 10);
        f310_f() = (r1 * 0.03f + hi) + 0.05f;
        float r2 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        float ga = (float)g_difficulty * 0.1f;
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
static __forceinline vector4 vscale2(const vector4& v, float s) { return vector4(v.x * s, v.y * s, v.z * s); }
static __forceinline vector4 vadd2(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }

// @ 0x44e2b0
void Agent::shoot()
{
    ItemRef curp = cur_pair_fi();
    ItemKind* ik = &g_items[curp.item];

    Skeleton* an = body->skel;
    if (!an->done) {
        if (an->has_poses) an->update_poses(); else ((FxAnim*)an)->update_poses2();
    }
    BoneW* bones = (BoneW*)an->recs;
    const rgl::matrix* W = &bones[9].world;
    vector4 P(W->o.x * 0.65f, W->o.y * 0.65f, W->o.z * 0.65f);
    an = body->skel;
    if (!an->done) {
        if (an->has_poses) an->update_poses(); else ((FxAnim*)an)->update_poses2();
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
    if (id != ms->player_id && tid >= 0) {
        ItemRef c2 = slot_pair_fi(1);
        float cls = (float)slot_class_impl(curp.item, c2.item >= 0);
        Agent* tg = &ms->agents.pages[tid >> 4][tid & 0xf];
        vector4 rel = a_vsub(tg->v56b4, v56b4);
        vector4 diff = a_vsub(tg->pos, pos);
        vector4 dn = diff;
        float dist = dn.normalize();
        vector4 q = a_vadd(diff, rel);
        q.normalize();
        float cx = dn.y * q.z - dn.z * q.y;
        float cy = dn.z * q.x - q.z * dn.x;
        float cz = q.y * dn.x - dn.y * q.x;
        float err = cls + 20.0f;
        float g5 = (float)g_difficulty * 0.5f;
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
    int missing, total, slot_out;
    ItemRef ammo = best_ammo(-1, &missing, &total, &slot_out);
    vector4 shot;
    rand_cone_dir(&shot, d0, spr);
    ItemRef launcher = cur_pair_fi();
    unsigned mid = g_mission->spawn_missile(&P, &shot, &launcher, &ammo, id);
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
    int i0 = add_point(p0);
    vector4 q;
    q.x = p1->x; q.y = p0->y; q.z = p0->z;
    int i1 = add_point(&q);
    q.x = p1->x; q.y = p1->y; q.z = p0->z;
    int i2 = add_point(&q);
    q.x = p0->x; q.y = p1->y; q.z = p0->z;
    int i3 = add_point(&q);
    vector4 n(0.0f, 0.0f, 1.0f);
    int f5c = 0;
    int v0 = add_vertex(i0, uv0, color, &n, &f5c);
    vec2 u1; u1.x = uv1->x; u1.y = uv0->y;
    f5c = 0;
    int v1 = add_vertex(i1, &u1, color, &n, &f5c);
    f5c = 0;
    int v2 = add_vertex(i2, uv1, color, &n, &f5c);
    vec2 u3; u3.x = uv0->x; u3.y = uv1->y;
    f5c = 0;
    int v3 = add_vertex(i3, &u3, color, &n, &f5c);
    add_face(v0, v1, v2);
    add_face(v0, v2, v3);
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
    std::vector<point20>& pts = other->get70();
    std::vector<vertex60>& vs = other->get50();
    std::vector<elem30>& sk = other->get60();
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
            elem30 e = sk[i];
            v60.push_back(e);
        }
    } else {
        if (bone >= 0) {
            if (v60.size() < v50.size())
                build_skin();
            for (unsigned i = 0; i < vs.size(); i++) {
                elem30 e;
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
        vector4 t;
        t.x = n.x * m->s.x + (m->f.x * n.y + m->u.x * n.z);
        t.y = m->f.y * n.y + (m->u.y * n.z + n.x * m->s.y);
        t.z = m->f.z * n.y + (m->u.z * n.z + n.x * m->s.z);
        vertex60 v = vs[i];
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
    std::vector<face20>& fs = other->get90();
    for (unsigned i = 0; i < fs.size(); i++) {
        if (flip)
            add_face(fs[i].a + nverts, fs[i].c + nverts, fs[i].b + nverts);
        else
            add_face(fs[i].a + nverts, fs[i].b + nverts, fs[i].c + nverts);
    }
    slots[0].f28 = 1;
    slots[1].f28 = 1;
    ((rgl::box_ext*)&v22c)->extend_box(&other->v22c, (const rgl::matrix*)m);
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
    v22c.min.x = s * v22c.min.x;
    v22c.min.y = s * v22c.min.y;
    v22c.min.z = s * v22c.min.z;
    v22c.max.x = s * v22c.max.x;
    v22c.max.y = s * v22c.max.y;
    v22c.max.z = s * v22c.max.z;
    v22c.center.x = s * v22c.center.x;
    v22c.center.y = s * v22c.center.y;
    v22c.center.z = s * v22c.center.z;
    v22c.radius = s * v22c.radius;
}
// @ 0x44f240
void rgl::mesh::translate(const vector4* d)
{
    ensure_local();
    for (unsigned i = 0; i < v70.size(); i++) {
        point20* p = &(&*v70.begin())[i];
        p->a.x = p->a.x + d->x;
        p->a.y = d->y + p->a.y;
        p->a.z = d->z + p->a.z;
        p = &(&*v70.begin())[i];
        p->b.x = p->b.x + d->x;
        p->b.y = p->b.y + d->y;
        p->b.z = p->b.z + d->z;
    }
    for (int j = 0; j < kf.count; j++) {
        for (unsigned k = 0; k < kf.frames[j].verts.size(); k++) {
            vector4* q = &(&*kf.frames[j].verts.begin())[k];
            q->x = d->x + q->x;
            q->y = d->y + q->y;
            q->z = d->z + q->z;
        }
    }
    char* dirty = &slots[0].f28;
    *dirty = 1;
    v22c.min.x = v22c.min.x + d->x;
    v22c.min.y = v22c.min.y + d->y;
    v22c.min.z = d->z + v22c.min.z;
    v22c.max.x = v22c.max.x + d->x;
    v22c.max.y = v22c.max.y + d->y;
    v22c.max.z = v22c.max.z + d->z;
    v22c.center.x = v22c.center.x + d->x;
    v22c.center.y = v22c.center.y + d->y;
    v22c.center.z = v22c.center.z + d->z;
}

// ---- Font::draw_glyph, the carried ranged item mesh, add_placed, the Newton world ---------------------------------------------------
// the font glyph record: the slices read it as {u0, v0, u1, v1, xoff, yoff, xadv}; font.h's Glyph has the same seven words
// (key, f4, f8, fc, f10, f14, f18), the key word being the left u of an ascii entry
static __forceinline const Glyph* font_get(Font* f, int c)
{
    if (c >= 0x200) {
        GlyphNode* it;
        f->map.lower_bound(&it, &c);
        if (it != f->map.head)
            return &it->val;
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
        tb.y = p->b.x * m.s.y + m.u.y * p->b.z + m.f.y * p->b.y + m.o.y;
        tb.z = p->b.x * m.s.z + m.u.z * p->b.z + m.f.z * p->b.y + m.o.z;
        p->b = tb;
    }
    for (i = 0; i < v50.size(); i++) {
        vector4 tb;
        vertex60* v = &(&*v50.begin())[i];
        tb.x = m.u.x * v->normal.z + m.f.x * v->normal.y + m.s.x * v->normal.x;
        tb.y = m.u.y * v->normal.z + m.f.y * v->normal.y + m.s.y * v->normal.x;
        tb.z = m.u.z * v->normal.z + m.f.z * v->normal.y + m.s.z * v->normal.x;
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
    float h = dv * scale->y;
    a.y = (g->f14 * scale->y - h) + a.y;
    float w = (g->f8 - (*(float*)&g->key)) * scale->x;
    vector4 b(a.x + w, a.y + h, a.z);
    m->add_quad(&a, &b, (const rgl::vec2*)&g->key, (const rgl::vec2*)&g->f8, (const unsigned*)arg);
}

// item flag field at ItemKind::flags: (flags & 0xff000) selects a visual class (module-system itp_* bits 12..19)
enum { ITEM_CLASS_MASK = 0xff000, ITEM_CLASS_A = 0x10000, ITEM_CLASS_B = 0x20000, ITEM_CLASS_C = 0x30000 };

static inline tstring item_mesh_name(ItemKind* items, const ItemRef& r)
{
    if (r.item >= 0 && r.item < g_item_kind_count)
        return items[r.item].mesh_name_for((int)r.m4 >> 24);
    return tstring("Error: Invalid item");
}
static __forceinline res_obj* get_obj(const tstring& n) { return (res_obj*)RESMGR->get_object((const rgl::string*)&n); }
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
        Rec1c8 rec;                                            // its inlined constructor calls reset (0x4352f0, defined as PlacedObj::reset in agent.cpp): shared rec1c8.h
        ((std::vector<Rec1c8>&)props).push_back(rec);       // 0x440b80 (the explicit instantiation of agent_combat.cpp)
        int off = idx * 0x1c8;
        ::init_placed((PlacedObj*)((char*)&props[0] + off), kind, id, value, m);
        if (*kind == 7 || *kind == 6)
            ((PlacedObj*)((char*)&props[0] + off))->m10 = 2;
        if (*kind == 1) {
            cnt = 0;
            prev = -1;
            for (int k = 0; k < 0x28; k++) {
                if (prev >= cnt)
                    break;
                prev = cnt;
                for (unsigned j = 0; j < props.size(); j++) {
                    if (((PlacedObj&)props[j]).kind == 1 && ((PlacedObj&)props[j]).value == cnt)
                        cnt++;
                }
            }
            if (cnt < 0x40)
                ((PlacedObj*)((char*)&props[0] + off))->value = cnt;
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
// frame whose side axis is `dir`, f/u completed with the cross products (the inline code of s004417d0)
static __forceinline void make_frame(matrix& R, const vector4& dir)
{
    R.s = dir;
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
    V4 wacc[32], vacc[32];
    M4 Wr[32], Ar[32];
    matrix* W = (matrix*)Wr;
    matrix* A = (matrix*)Ar;
    int i;

    // zero the accumulators
    for (i = 0; i < n; i++) {
        *(vector4*)&vacc[i] = vector4(0.0f, 0.0f, 0.0f);
        *(vector4*)&wacc[i] = vector4(0.0f, 0.0f, 0.0f);
    }
    vector4 v, w;                                           // velocity / angular velocity handed to Newton

    // sum the ancestors' angular velocities into every bone
    for (i = 0; i < n; i++) {
        float px, py, pz;
        {
            const vector4& o = b->skel->regs[i].m84.o;
            px = b->m.f.x * o.y + b->m.u.x * o.z + b->m.s.x * o.x + b->m.o.x;
            py = b->m.f.y * o.y + b->m.u.y * o.z + b->m.s.y * o.x + b->m.o.y;
            pz = b->m.f.z * o.y + b->m.u.z * o.z + b->m.s.z * o.x + b->m.o.z;
        }
        int j = sd->items[i].parent;
        if (j >= 0) {
            do {
                const vector4& o = b->skel->regs[j].m84.o;
                float qz = b->m.f.z * o.y + b->m.s.z * o.x + b->m.u.z * o.z + b->m.o.z;
                float rx = px - (b->m.f.x * o.y + b->m.u.x * o.z + b->m.s.x * o.x + b->m.o.x);
                float ry = py - (b->m.f.y * o.y + b->m.s.y * o.x + b->m.u.y * o.z + b->m.o.y);
                float rz = pz - qz;
                Region* e = &b->skel->regs[j];
                float lx = e->m84.s.x * e->wx + e->m84.f.x * e->wy + e->m84.u.x * e->wz;
                float ly = e->m84.s.y * e->wx + e->m84.f.y * e->wy + e->m84.u.y * e->wz;
                float lz = e->m84.s.z * e->wx + e->m84.f.z * e->wy + e->m84.u.z * e->wz;
                float vx = ly * b->m.f.x + (lz * b->m.u.x + lx * b->m.s.x);
                float vy = ly * b->m.f.y + (lz * b->m.u.y + lx * b->m.s.y);
                float vz = ly * b->m.f.z + (lz * b->m.u.z + lx * b->m.s.z);
                float cx = vy * rz - vz * ry;
                float cz = vx * ry - rx * vy;
                vacc[i].x = cx + vacc[i].x;
                vacc[i].y = (rx * vz - vx * rz) + vacc[i].y;
                vacc[i].z = cz + vacc[i].z;
                wacc[i].x = vx + wacc[i].x;
                wacc[i].y = vy + wacc[i].y;
                wacc[i].z = vz + wacc[i].z;
                do {
                    j = sd->items[j].parent;
                } while (sd->items[j].caps.size() == 0);
            } while (j >= 0);
        }
    }

    // create the bones
    for (i = 0; i < n; i++) {
        ChanRec* rec = &sd->items[i];
        int p = rec->parent;
        matrix T;
        if (p < 0) {
            W[i] = b->m.mul(b->skel->regs[i].m84);
            T = b->m.mul(rec->m);
            A[i] = T;
        } else {
            W[i] = ((posm&)W[p]).mul(b->skel->regs[i].m44);
            matrix cur = rec->m;
            int j = p;
            for (;;) {
                if (sd->items[j].caps.size() != 0)
                    break;
                cur = sd->items[j].m.mul(cur);
                j = sd->items[j].parent;
            }
            T = cur;
            float a0 = rec->rot_x * 0.017453292f;
            ((rgl::matrix&)T).rotate_about_s(&a0);
            float a1 = rec->rot_y * 0.017453292f;
            ((rgl::matrix&)T).rotate_about_f(&a1);
            float a2 = rec->rot_z * 0.017453292f;
            ((rgl::matrix&)T).rotate_about_u(&a2);
            A[i] = ((posm&)A[j]).mul(T);
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
            rot3v(cone, A[i], rec->cone.x, rec->cone.y, rec->cone.z);
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
            rot3v(rv, b->m, b->skel->rvx, b->skel->rvy, b->skel->rvz);
            v.x = rv.x + v.x;
            v.y = rv.y + v.y;
            v.z = v.z + rv.z;
            v.x = v.x + vacc[i].x;
            v.y = v.y + vacc[i].y;
            v.z = v.z + vacc[i].z;
            NewtonBody* nb = NewtonRagDollBoneGetBody(b->skel->regs[i].bone);
            Region* e = &b->skel->regs[i];
            vector4 wl;
            rot3v(wl, e->m84, e->wx, e->wy, e->wz);
            rot3v(w, b->m, wl.x, wl.y, wl.z);
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
        vector4 dir(0.0f, 1.2886f, 0.0f);
        float len = vnorm(dir);
        float height = len * h->scale;
        make_frame(R, dir);
        R.o = vector4(0.0f * h->scale, 0.5443f * h->scale, -0.0629f * h->scale);
        matrix m0 = h->body->m.mul(h->body->skel->regs->m84);
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
// @ 0x451710
void dust_vertices(mesh* m, const mat4* xf, const float* center, const float* radius, const float* strength,
                   const vector4* tint)
{
    bool dirty = false;
    char loc = m->local_a1;
    std::vector<rgl::vertex60>& vs = m->get50();
    std::vector<rgl::point20>& ps = m->get70();
    for (unsigned i = 0; i < vs.size(); i++) {
        rgl::point20* pt = &(&*ps.begin())[(&*vs.begin())[i].index];
        vector4 w;
        w.x = xf->s.x * pt->b.x + xf->u.x * pt->b.z + xf->f.x * pt->b.y + xf->o.x;
        w.y = xf->s.y * pt->b.x + xf->u.y * pt->b.z + xf->f.y * pt->b.y + xf->o.y;
        w.z = xf->s.z * pt->b.x + xf->u.z * pt->b.z + xf->f.z * pt->b.y + xf->o.z;
        float dx = w.x - center[0];
        float dy = w.y - center[1];
        float dz = w.z - center[2];
        float dist = (float)sqrt(dx * dx + dy * dy + dz * dz);
        if (dist < *radius && (float)(rand() % 15817) * (1.0f / 15817.0f) < 0.6f) {
            float a = (1.0f - dist / *radius) * 1.5f;
            float s = ((float)(rand() % 15817) * (0.6f / 15817.0f) + 0.4f) * a;
            s = s * *strength;
            if (s < 0.0f)
                continue;
            if (s >= 1.0f)
                s = 1.0f;
            else if (!(s > 0.02f))
                continue;
            m->ensure_local();
            vector4 c;
            c = rgl::unpack_color(&(&*vs.begin())[i].color);
            float omt = 1.0f - s;
            vector4 o = rgl::vadd(rgl::vscale(omt, c), rgl::vscale(s, *tint));
            o.w = c.w;
            (&*m->v50.begin())[i].color = rgl::pack_color(&o);
            dirty = true;
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
                            rgl::mat4 xf = body->m;
                            if (cur == k) {
                                xf = (&*L->lists[cur].v.begin())[j]->m194;
                            } else {
                                std::vector<mesh*>& cl = L->lists[cur].v;
                                if (cl.size() != 0)
                                    xf = (*cl.begin())->m194;
                            }
                            dust_vertices(*slot, &xf, center, &radius, &strength, &tint);
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
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    ensure_local();
    std::vector<vector4> fnorm;
    fnorm.resize(v90.size());
    unsigned i;
    for (i = 0; i < v90.size(); i++) {
        face20* f = &(&*v90.begin())[i];
        vertex60* vs = &*v50.begin();
        point20* ps = &*v70.begin();
        vector4* p0 = &ps[vs[f->a].index].a;
        vector4* p1 = &ps[vs[f->b].index].a;
        vector4* p2 = &ps[vs[f->c].index].a;
        vector4 e1;
        e1.x = p1->x - p0->x; e1.y = p1->y - p0->y; e1.z = p1->z - p0->z;
        vector4 e2;
        e2.x = p2->x - p0->x; e2.y = p2->y - p0->y; e2.z = p2->z - p0->z;
        vector4& n = fnorm[i];
        cross_inl(n, e1, e2);
        float len = (float)sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
        if (len > 1e-10f) {
            float inv = 1.0f / len;
            n.x = inv * n.x;
            n.y = inv * n.y;
            n.z = inv * n.z;
        } else {
            n.x = 0.0f;
            n.y = 1.0f;
            n.z = 0.0f;
        }
    }
    for (i = 0; i < v50.size(); i++) {
        vector4 z;
        z.x = 0.0f; z.y = 0.0f; z.z = 0.0f;
        (&*v50.begin())[i].normal = z;
    }
    for (i = 0; i < v90.size(); i++) {
        vector4* n = &(&*v50.begin())[(&*v90.begin())[i].a].normal;
        n->x = n->x + fnorm[i].x; n->y = fnorm[i].y + n->y; n->z = n->z + fnorm[i].z;
        n = &(&*v50.begin())[(&*v90.begin())[i].b].normal;
        n->x = n->x + fnorm[i].x; n->y = fnorm[i].y + n->y; n->z = n->z + fnorm[i].z;
        n = &(&*v50.begin())[(&*v90.begin())[i].c].normal;
        n->x = n->x + fnorm[i].x; n->y = fnorm[i].y + n->y; n->z = n->z + fnorm[i].z;
    }
    for (i = 0; i < v50.size(); i++) {
        vector4* n = &(&*v50.begin())[i].normal;
        if (n->x * n->x + n->y * n->y + n->z * n->z > 0.0001f) {
            float len = (float)sqrt(n->x * n->x + n->y * n->y + n->z * n->z);
            if (len > 1e-10f) {
                float inv = 1.0f / len;
                n->x = inv * n->x;
                n->y = inv * n->y;
                n->z = inv * n->z;
            } else {
                n->x = 0.0f; n->y = 1.0f; n->z = 0.0f;
            }
        } else {
            vector4 t;
            t.x = 0.0f; t.y = -1.0f; t.z = 0.0f;
            *n = t;
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
                int s = p;
                brk = false;
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
extern int g_blood_enabled;                             // 0x5a9fc0 (provisional)
// @ 0x4525a0
void Mission::blood_burst(float x, float y, float z, int unused, float amount)
{
    if (g_blood_enabled) {
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
        rgl::mat4 fr;
        fr.f = other->p40;
        fr.u.x = 0.0f; fr.u.y = 0.0f; fr.u.z = 1.0f;
        fr.o = other->p20;
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
        d.x = d.x * n;
        rel.o.x = d.x;
        rel.o.w = d.w;
        d.y = d.y * n + 0.23f;
        rel.o.y = d.y;
        d.z = d.z * n;
        rel.o.z = d.z;
        rel.o.z = 0.05f - (frand() * 0.01f + other->f5c * 0.0005f);
        if (rel.o.x < -0.45f)
            rel.o.x = -0.45f;
        else if (rel.o.x > 0.2f)
            rel.o.x = 0.2f;
        rel.o.x = (frand() - 0.5f) * 0.02f + rel.o.x;
        rel.o.y = (frand() - 0.5f) * 0.02f + rel.o.y;
        (*L->lists[0].v.begin())->append_plain(m, (const mat4*)&rel);
    }
    delete m;
}

// ---- Agent::drop_held, the text layout wrapper, the message log, the animator visibility switch, Agent::get_label_box -----------
extern MsgLog g_gui;                                 // 0x5d7080
extern StrTable g_strtable;                             // 0x5e2a18
extern Font* g_font;                                    // 0x5e2934
// the animator's inline accessor of a bone record's world frame (brings the poses up to date first)
static __forceinline char* skel_rec_inl(Skeleton* sk, int i)
{
    if (!sk->done) {
        if (sk->has_poses)
            sk->update_poses();
        else
            ((FxAnim*)sk)->update_poses2();
    }
    return sk->recs + i * 0x1dc + 0x84;
}
static __forceinline tstring* lk(const tstring& k) { return g_strtable.lookup(&k); }
static __forceinline void* get_mat(const tstring& n) { return (void*)RESOURCE_MANAGER->get_material(*(const rgl::string*)&n); }

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
    ((rgl::mesh_ext*)m)->clear_geometry();
    layout_text((float*)out_extent, text, pos, scale, m, (int)a, b, c, flags);
    return out_extent;
}
#define TXT(i) (*(mesh*)(text_buf + (i) * 0x260))
#define BGM(i) (*(mesh*)(bg_buf + (i) * 0x260))
// @ 0x452c00
void MsgLog::add(tstring msg, int color)
{
    pos2 M;
    tstring S = msg.strip_tags();
    while (lines.size() > 100)
        lines.erase(lines.begin());
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
            vec2 scale;
            int zero;
            float f08;
            int minus1;
            vector4 p;
            vector4 ext;
            scale.x = 0.0197f;
            scale.y = 0.0197f;
            f08 = 0.8f;
            cur = (cur + 31) % 32;
            ((rgl::mesh_ext*)&TXT(cur))->clear_geometry();
            ((rgl::mesh_ext*)&BGM(cur))->clear_geometry();
            zero = 0;
            minus1 = -1;
            p.x = 0.02f;
            p.y = -0.03f;
            p.z = 0.0f;
            g_font->layout_clear(&ext, &S, &p, (const float*)&scale, &TXT(cur), &minus1, (const float*)&zero, &f08, 0);
            identity_pos((::pos*)&M);
            BGM(cur).append_skinned(&TXT(cur), (mat4*)&M, -1, -1, 0);
            this->color[cur] = color;
            age[cur] = 0;
            ((mnode*)&TXT(cur))->calc_bounds(&ext, &p);       // mnode (agent2/agent_types.h) is the mesh seen by its node lists
            height[cur] = p.y - ext.y;
            total_h += height[cur];
        }
        TXT(cur).res = (rgl::res_t*)get_mat(tstring("ui_font"));
        BGM(cur).res = (rgl::res_t*)get_mat(tstring("ui_font_background"));
        lines.push_back(msg);
    }
}

// @ 0x452f80
void Skeleton::set_visible(char on)
{
    if (!frozen) {
        if (on) {
            if (!done) {
                if (has_poses)
                    update_poses();
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

// @ 0x453020
void __stdcall log_message(const tstring* s, int unused)
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
        p2.z = p2.z + p1.z;
        p1.x = (p2.x + p1.x) * 0.5f;
        p1.y = (p2.y + p1.y) * 0.5f;
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
        vector4 hi(pos.x, pos.y, z + 0.5f);
        out->a = lo;
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
                    float var = 0.0f;
                    int kind = 0x5e;
                    play_sound_var(&kind, &var);
                    mat4 M = body->m * *(mat4*)body->skel->rec(0xc);
                    int mod = it->b.mod;
                    kind = 7;
                    g_mission->add_placed(&kind, &it->item, &mod, (rgl::matrix*)&M, &id);
                    if (((g_mission_templates[g_mission->mode].flags >> 1) & 1) && old >= 0 &&
                        id == g_mission->player_id && old < 10 && B220(old)) {
                        ItemRef* s = &pairs[old];
                        if (s->item >= 0 && (g_items[s->item].type_flags & 0xff) == itp_type_shield) {
                            unsigned bits;
                            int m = s->b.mod;
                            if (m == 1) {
                                if (frand() < 0.2f) {
                                    { tstring key("ui_shield_broken"); g_gui.add(*g_strtable.lookup(&key), 0xddbbbb); }
                                    
                                    if (s->item == STORED(g_game->player, old).item && pairs[old].m4 == STORED(g_game->player, old).m4)
                                        STORED(g_game->player, old).item = -1;
                                }
                                return;
                            } else if (m == 5) {
                                if (!(frand() < 0.2f))
                                    return;
                                { tstring key("ui_shield_cracked"); g_gui.add(*g_strtable.lookup(&key), 0xddbbbb); }
                                bits = 0x1000000;
                            } else {
                                if (!(frand() < 0.2f))
                                    return;
                                { tstring key("ui_shield_deformed"); g_gui.add(*g_strtable.lookup(&key), 0xddbbbb); }
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
static inline Agent* AGENT(Mission* ms, unsigned id)
{
    return &ms->agents.pages[id >> 4][id & 0xf];
}

static __forceinline vector4 vmul_vs(const vector4& v, const float& s)
{
    vector4 r;
    r.x = v.x * s; r.y = v.y * s; r.z = v.z * s;
    return r;
}
static __forceinline vector4 vmul_sv(const float& s, const vector4& v)
{
    vector4 r;
    r.x = s * v.x; r.y = s * v.y; r.z = s * v.z;
    return r;
}
static __forceinline vector4 vsub(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.x - b.x; r.y = a.y - b.y; r.z = a.z - b.z;
    return r;
}
static __forceinline vector4 rot_xy(const posm& M, vec2 p)
{
    vector4 t;
    t.x = p.x * M.s.x + 0.0f * M.u.x + p.y * M.f.x;
    t.y = 0.0f * M.u.y + p.x * M.s.y + p.y * M.f.y;
    t.z = 0.0f * M.u.z + p.x * M.s.z + p.y * M.f.z;
    return t;
}
static __forceinline vector4 rot_inl(const posm& M, const vector4& p)
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
        unsigned mid = AGENT(ms, victim)->f2c;
        Agent* v = AGENT(ms, victim);
        if ((int)mid >= 0) {
            A = vmul_vs(AGENT(ms, mid)->body->m.f, AGENT(ms, mid)->vel_xy.y);
        } else {
            vector4 lv;
            lv.x = v->vel_xy.x; lv.y = v->vel_xy.y; lv.z = 0.0f;
            A = ((const ::pos&)v->body->m).rot(lv);
        }
    }
    vector4 B;
    if (f2c >= 0) {
        B = vmul_sv(AGENT(ms, f2c)->vel_xy.y, AGENT(ms, f2c)->body->m.f);
    } else {
        vec2 lv = vel_xy;
        B = rot_xy(body->m, lv);
    }
    if (missile) {
        *out_flag = 1;
        float spd = missile->f5c;
        if ((int)victim >= 0) {
            float z = missile->p40.z * missile->f5c;
            float x = A.x - missile->f5c * missile->p40.x;
            float y = A.y - missile->p40.y * missile->f5c;
            spd = (float)sqrt(x * x + y * y + (A.z - z) * (A.z - z));
        }
        *out_dmg = (float)pow((double)(spd / missile->f60), (double)g_missile_damage_speed_power) * missile->f64 * *scale;
    } else {
        int st = hand ? mode2 : mode1;
        unsigned fl = (st == 0);
        *out_flag = fl;
        float base;
        if (sp.item >= 0)
            base = (float)(int)sp.stat_21x(fl) * *scale;
        else
            base = (float)g_game->chars[troop].lvl58 * 0.3f + 2.0f;
        *out_dmg = base;
        vector4 d = vsub(A, B);
        float len = d.normalize();
        if (f100_is_0x48_and_charged()) {
            *out_dmg = (*out_dmg + 5.0f) * (float)pow((double)len, (double)1.8f);
            vector4 q = ((const ::pos&)body->m).rot(*(vector4*)(skel_rec_inl(body->skel, 19) + 0x10));
            q.normalize();
            *out_dmg = ((0.2f - (q.x * d.x + q.y * d.y + q.z * d.z)) * *out_dmg) * g_couched_lance_damage_multiplier * 0.2f;
            if (g_ui_enabled) {
                if (id == g_mission->player_id)
                    g_gui.add(*lk(tstring("ui_delivered_couched_lance_damage")), 0xffd03030);
                else if (victim == (unsigned)g_mission->player_id)
                    g_gui.add(*lk(tstring("ui_received_couched_lance_damage")), 0xffd03030);
            }
        } else {
            if (mode1 == 3)
                *out_dmg = *out_dmg * 1.05f;
            vector4 q = ((const ::pos&)body->m).rot(*(vector4*)(skel_rec_inl(body->skel, 19) + 0x10));
            q.normalize();
            int m = mode1;
            vector4 up;
            up.x = 0.0f; up.y = 0.0f; up.z = 1.0f;
            if (m == 0) {
                A = q;
            } else {
                vector4* ax;
                if (m == 2) {
                    ax = &up;
                } else {
                    if (m == 1) {
                        vector4 c = rgl::cross(q, up);
                        up.x = -c.x; up.y = -c.y; up.z = -c.z;
                        A = up;
                        goto done_dir;
                    }
                    A = rgl::cross(q, up);
                    A.normalize();
                    ax = &A;
                }
                A = rgl::cross(q, *ax);
            }
        done_dir:
            A.normalize();
            float t = 1.0f;
            float h = anim[1].scale;
            if (h < 0.33f) {
                t = h * 3.030303f;
                if (t < 0.0f)
                    t = 0.0f;
                else if (t >= 1.0f)
                    t = 1.0f;
                if (m != 0)
                    t = t * 0.7f + 0.3f;
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
                if (t < 0.1f)
                    t = 0.1f;
                else if (t >= 1.2f)
                    t = 1.2f;
                t = (t * w - (A.x * d.x + A.y * d.y + A.z * d.z) * len) / w;
                if (t < 0.3f)
                    t = 0.3f;
                else if (t > 3.0f)
                    t = 3.0f;
                float p = (float)pow((double)t, (double)g_melee_damage_speed_power);
                *out_dmg = p * *out_dmg;
                float bonus = (p - 1.0f) * 100.0f;
                int n = round_float(&bonus);
                if (n != 0 && g_ui_enabled && id == g_mission->player_id) {
                    char msg[512];
                    if (n > 0)
                        sprintf(msg, lk(tstring("ui_speed_bonus_plus"))->c_str(), n);
                    else
                        sprintf(msg, lk(tstring("ui_speed_bonus"))->c_str(), n);
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
                                w->o.z - ((level*)g_mission->scene_world)->floor_z(&w->o.x) < lim) {
                                g_sound_mgr.play(g_sound_blood, 0, (const float*)&body->m.o, frand() * 0.4f + 0.8f, 0);
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
    if (g_fx_on != 0)
        fxp->visible = 1;
}

// ---- Agent::reduce_blow, draw_name, spawn_corpse --------------------------------------------------------------------------------------
// @ 0x454e30
void Agent::reduce_blow(Blow* b)
{
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
            int bone, aux;
            ik->get_carry_pos(&carry, &bone, &aux);
            rgl::mat4 r = *(rgl::mat4*)body->skel->rec(bone);
            rgl::mat4 u = (rgl::mat4&)body->m * (r * *(rgl::mat4*)&carry);
            HitInfo* h = b->hit;
            if (u.u.y * h->dy + u.u.z * h->dz + u.u.x * h->dx < -0.5f) {
                float hi[3], lo[3];
                hi[0] = h->dx * 4.0f + h->px;
                hi[1] = h->dy * 4.0f + h->py;
                hi[2] = h->dz * 4.0f + h->pz;
                lo[0] = h->px - h->dx * 4.0f;
                lo[1] = h->py - h->dy * 4.0f;
                lo[2] = h->pz - h->dz * 4.0f;
                capsule cap(0.0f, (vector4*)lo, (vector4*)hi);
                float zero = 0.0f;
                sphere sph(*(vector4*)&u.o, &zero);
                float gap = capsule_sphere_gap(&cap, &sph);
                if (gap < (float)ik->weight * 0.01f) {
                    int bonus = w.stat_200() + 10;
                    armor_v = (float)bonus + armor_v;
                    if (id == g_mission->player_id) {
                        tstring key("ui_hit_shield_on_back");
                        g_gui.add(*g_strtable.lookup(&key), -1);
                    }
                }
            }
        }
    }
    float x, y;
    if (b->kind == 0) {
        x = g_armor_soak_factor_against_cut[0] * armor_v;
        armor_v = g_armor_reduction_factor_against_cut[0] * armor_v;
    } else if (b->kind == 1) {
        x = g_armor_soak_factor_against_cut[1] * armor_v;
        armor_v = g_armor_reduction_factor_against_cut[1] * armor_v;
    } else {
        x = g_armor_soak_factor_against_cut[2] * armor_v;
        armor_v = g_armor_reduction_factor_against_cut[2] * armor_v;
    }
    y = ((float)(rand() % 0x3dc9) * 3.4772715e-05f + 0.45f) * x;
    x = dmg - y;
    if (!(0.0f > x)) {
    } else
        x = 0.0f;
    int iv = rand() % 0x3dc9;
    float part = (1.0f - 1.0f / (float)exp((double)(armor_v * 0.014f * ((float)iv * 3.4772715e-05f + 0.45f)))) * x;
    if (armor_v < 0.0001f)
        part = 0.0f;
    y = part + y;
    double fl = ::floor((double)y);
    int n = (int)fl;
    if (!(y - fl <= 0.5))
        n++;
    b->absorbed = n;
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
    if ((int)local >= 0) {
        if (team == ms->agents.pages[local >> 4][local & 0xf].team)
            col = 0xff44ff44;
        else if ((int)local >= 0 && ms->agents.pages[local >> 4][local & 0xf].is_friend(this))
            col = 0xff4444ff;
    }
    if (f10 == 0 && id != (int)local) {
        unsigned fl = g_mission_templates[ms->mode].flags;
        if (((fl >> 1) & 1) && (int)local >= 0 && ms->agents.pages[local >> 4][local & 0xf].is_friend(this)) {
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
            ((level*)g_mission->scene_world)->floor_at(&pos.x, &z, pf);
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
            f5830 = 4;
            tm.start = g_ticks[tm.clock];
            return;
        }
        if (disabled == 0 && queued == 0 && tm.elapsed() > 3.0f) {
            if (f14 != 1) {
                body->skel->set_visible(1);
                if (id != g_mission->player_id)
                    drop_held();
            }
            if (id != g_mission->player_id)
                g_mission->queue_dead(this);
            queued = 1;
        }
        if (g_tick_a > g_tick_b && TICKS_SINCE(tm) > 0.0f && t5918.elapsed() > 0.0f) {
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
extern GaitBand g_gait_walk[];                          // 0x590940
extern GaitBand g_gait_mount[];                         // 0x590910
extern float g_move_fwd;                                // 0x5e2478
extern float g_move_side;                               // 0x5e247c
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
static __forceinline void vset3(float* v, const float& a, const float& b, const float& c) { v[0] = a; v[1] = b; v[2] = c; }
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
    if ((int)(anim[0].events & 0x1ffff) >= 0x40) {
        vec2 z;
        z.x = 0.0f;
        z.y = 0.0f;
        vel_xy = z;
        return;
    }
    vec2 mv;
    float divisor;
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
                    if (::is_night())
                        speed_mul = speed_mul * 0.9f;
                }
            }
            vector4 up(0.0f, 0.0f, 1.0f);
            if (fbc)
                up = fbc->d;

            // ---- input bits -> the two input axes ------------------------------------------------------------------
            unsigned fl = in_flags;
            float side = 0.0f;
            mv.y = 0.0f;
            if (fl & 1) {
                mv.y = 1.0f;
                if (fl & 0x200000)
                    g_move_fwd = 1.0f;
                else
                    g_move_fwd = 0.6f;
            } else if (fl & 2) {
                mv.y = -1.0f;
                g_move_fwd = -0.7f;
                if (!(fl & 0x200000))
                    g_move_fwd = -0.5f;
            }
            if (in_flags & 4) {
                g_move_fwd = g_move_fwd * 0.7f;
                side = 1.0f;
                if (in_flags & 0x200000)
                    g_move_side = 0.9f;
                else
                    g_move_side = 0.55f;
            } else if (in_flags & 8) {
                g_move_fwd = g_move_fwd * 0.7f;
                side = -1.0f;
                g_move_side = -0.9f;
                if (!(in_flags & 0x200000))
                    g_move_side = -0.55f;
            }
            float len2 = mv.y * mv.y + side * side;
            if (len2 > 0.0f) {
                divisor = 1.0f / (float)sqrt(len2);
                float ms = divisor * side;
                float mf = divisor * mv.y;
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
                g_move_side = ms;
                g_move_fwd = mf;
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
                        if (slot_pair(0).item < 0 || g_items[slot_pair(0).item].type_flags % 256 != 9)
                            anim = 7;
                        else
                            anim = 8;
                        break;
                    case 3:
                        anim = 6;
                        break;
                    default:
                        if (!(f2c4 & 1) && (id != (unsigned)g_mission->player_id ||
                                (!((g_mission_templates[g_mission->mode].flags >> 1) & 1) && !(g_mission->terrain.f158 & 1))))
                            anim = stand_animation;
                        else
                            anim = 4;
                        break;
                    }
                    set_order(0, anim, 0, 1.0f);
                }
                mv.x = g_move_side;
                mv.y = g_move_fwd;
                divisor = 1.0f;
                float slope = 1.0f;
                vector4 v;
                v.x = mv.x;
                v.y = mv.y;
                v.z = 0.0f;
                v.normalize();
                v = ((const ::pos&)body->m).rot(v);
                v = cross(up, cross(v, up));
                v.normalize();
                if (v.z > 0.01f)
                    slope = 1.0f - v.z * 0.5f;
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
                int off = troop * 0x508;
                Troop* tr = (Troop*)((char*)g_game->chars + off);
                int ath = tr->nibble(0x19);
                float skill = ((60.0f / (f24c + 60.0f)) * (ath * 5.0f + tr->f58[1]) + 100.0f) * 0.01f;
                v.y = ((mv.y * mag) * 2.8f) * skill;
                if (v.y < 0.0f)
                    v.y = v.y * 0.8f;
                v.x = (((mv.x * mag) * 2.8f) * skill) * 0.95f - vel_xy.x;
                v.y = v.y - vel_xy.y;
                tr = (Troop*)((char*)g_game->chars + off);
                int ath2 = tr->nibble(0x19);
                float step = (((70.0f / (f24c + 70.0f)) * ((25.0f / (f24c + 25.0f)) *
                              ((float)tr->f58[1] * 0.06666667f + (float)ath2 * 0.2f) + 1.0f)) * dt) * 10.0f;
                float nstep = -step;
                float nstep2 = nstep;
                float nvx = clamp(&v.x, &nstep, &step) + vel_xy.x;
                vel_xy.x = nvx;
                float nvy = clamp(&v.y, &nstep2, &step) + vel_xy.y;
                vel_xy.y = nvy;
                unsigned band = fb8 & 7;
                float spd = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
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
                    int ath3 = ((Troop*)((char*)g_game->chars + off))->nibble(0x19);
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
                rskill = g_game->chars[agent_by_id(rid)->troop].nibble(0x18);
                rmul = ((float)rskill + 25.0f) * 0.037037037f;
            }
            float sgn = 0.0f;
            if (f10 == 1) {
                unsigned fl2 = in_flags;
                if (fl2 & 0x40) {
                    sgn = -1.0f;
                } else if ((char)fl2 < 0) {
                    sgn = 1.0f;
                }
                if (rid >= 0) {
                    if (g_mission->agents.get(rid)->f100_is_0x48_and_charged())
                        sgn = sgn * 0.7f;
                }
                float lim = 4.0f;
                if (vel_xy.y > 4.0f)
                    lim = vel_xy.y;
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
                            int s2 = f8c.stat_1f0b();
                            vel_xy.y = (((18.0f - vel_xy.y) * ((float)s2 * ((float)rskill + 3.0f))) * dt) * 0.0006f + vel_xy.y;
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
                        vel_xy.y = -0.9f;
                        fb8 = bb;
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
                float cap = ((float)rskill * 0.02f + 1.0f) * (((float)s3 * speed_mul) * 0.19f);
                vector4 v;
                v.x = vel_xy.y * fr->x;
                v.y = fr->y * vel_xy.y;
                v.z = fr->z * vel_xy.y;
                v = cross(up, cross(v, up));
                v.normalize();
                float lim2;
                if (v.z > 0.01f) {
                    lim2 = (1.0f - v.z * 0.9f) * cap;
                } else {
                    lim2 = cap;
                    if (v.z < -0.27f)
                        lim2 = (1.0f - (-0.27f - v.z) * 1.3f) * cap;
                }
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
                        { float c5 = impulse.z + 5.0f; vset3(&impulse.x, impulse.x, impulse.y, c5); }
                        set_order(0, 2, 0x1000002, 1.0f);
                        float var = 0.1f;
                        if (pos.z < g_mission->terrain.base_z) {
                            int kind = 0x1f;
                            play_sound_var(&kind, &var);
                        } else {
                            int kind = 0x1d;
                            play_sound_var(&kind, &var);
                        }
                        ((gui_timer*)&t5800)->set_delay(-2.0f);
                    }
                } else if (vel_xy.y > 3.5f) {
                    flags3c = flags3c & 0xfffffffe;
                    { float c6 = impulse.z + 6.0f; vset3(&impulse.x, impulse.x, impulse.y, c6); }
                    set_order(0, 0x19d, 0x1000002, 1.0f);
                    float var = 0.1f;
                    if (pos.z < g_mission->terrain.base_z) {
                        int kind = 0x23;
                        play_sound_var(&kind, &var);
                    } else {
                        int kind = 0x21;
                        play_sound_var(&kind, &var);
                    }
                    ((gui_timer*)&t5800)->set_delay(-4.0f);
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
            Agent* h2 = agent_by_id(f2c);
            if (h2->vel_xy.y > 5.5f) {
                order1(this, 0x48, 0);
            } else if (h2->anim[0].cur == 0x196) {
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
        if ((fl & 0xff000000) && (fl & 0xff000000) == 0x1000000 && f10 == 0 && anim[1].cur != 0x127 && f5874 < 3) {
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
extern int g_opt_dmg_to_player;                     // 0x5aa054
extern int g_opt_dmg_to_friends;                    // 0x5aa058
extern int g_opt_damage_msgs;                       // 0x5aa060
extern int g_opt_shot_difficulty;                   // 0x5aa064
extern int g_opt_kill_msgs;                         // 0x5aa068
extern int g_opt_blood;                             // 0x5aa020
extern int g_opt_hit_fx;                            // 0x5a9fc0
extern float g_damage_interrupt_attack_treshold;                            // 0xa18178
extern float g_blood_multiplier;                            // 0xa1811c
extern float g_player_xp_multiplier;                            // 0xa1812c
extern float g_hero_xp_multiplier;                            // 0xa18130
extern float g_regulars_xp_multiplier;                            // 0xa18134
extern int g_voice_default_a;                       // 0x8ed50a0
extern int g_voice_default_b;                       // 0x8ed50a4
struct AnimInfo { char pad0[0x3c]; float length; }; // +0x3c
struct AnimDefV { char pad0[0x10]; AnimInfo* info; char pad14[0x30 - 0x14]; };   // g_anims entry as 0x457b40 reads it (agent_obj.h's AnimDef calls +0x10 `subs`)
extern AnimDefV* g_anims;                           // 0xa76628
void __stdcall log_message(const tstring* s, int seconds_bits);   // 0x453020 (ret 8; the second argument is the float 20.0f it is called with)
#define itp_wooden_attack 0x00020000
#define itcf_throw_stone 0x0000000000010000i64
#define tf_allways_fall_dead 0x80
#define imod_lame 30
#define PAGE_AGENT(m, i) ((m)->agents.pages[(unsigned)(i) >> 4] + ((i) & 0xf))
// 0x43a280 (the register-convention helper: type in EAX, destination buffer in ESI)
static __declspec(noinline) void ui_weapon_class_name(int type, char* out)
{
    if (type == 0) {
        tstring k("ui_one_handed_weapons");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 1) {
        tstring k("ui_two_handed_weapons");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 2) {
        tstring k("ui_polearms");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 3) {
        tstring k("ui_archery");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 4) {
        tstring k("ui_crossbows");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 5) {
        tstring k("ui_throwing");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    } else if (type == 6) {
        tstring k("ui_firearms");
        const char* s = g_strtable.lookup(&k)->c_str();
        { char* d = out; char c; do { c = *s++; *d++ = c; } while (c != 0); }
    }
}

// @ 0x457b40
void Agent::process_blow(int unused)
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
        f55c0 = attacker;
    m = g_mission;
    atk_troop = -1;
    atk_rider = -1;
    if (attacker >= 0) {
        Agent** pg = &m->agents.pages[0];
        int r = attacker;
        if (pg[(unsigned)attacker >> 4][attacker & 0xf].f10 != 0) {
            if (pg[(unsigned)attacker >> 4][attacker & 0xf].f30 < 0)
                goto after_rider;
            r = m->agents.get(attacker)->f30;
        }
        atk_rider = r;
        if (r >= 0)
            atk_troop = pg[(unsigned)r >> 4][r & 0xf].troop;
    }
after_rider:
    if ((f10 == 0 && last_blow().bone == 9) || (f10 == 1 && last_blow().bone == 7)) {
        if (last_blow().item >= 0 && g_items[last_blow().item].is_ranged()) {
            dmg = dmg * 1.75f;
            if (attacker == m->player_id && g_opt_damage_msgs != 0) {
                tstring k("ui_head_shot");
                g_gui.add(*g_strtable.lookup(&k), 0xffffdd11);
                m = g_mission;
            }
        }
    }
    if (id == m->player_id || f30 == m->player_id) {
        if (g_opt_dmg_to_player == 1)
            dmg = dmg * 0.5f;
        else if (g_opt_dmg_to_player == 0)
            dmg = dmg * 0.25f;
    } else {
        int dd = g_opt_dmg_to_friends;
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
        int shown = g_opt_damage_msgs;
        if (last_blow().attacker == m->player_id && shown != 0) {
            char msgbuf[512];
            {
                tstring k((f10 != 0 && f30 != -1) ? "ui_delivered_number_damage_to_horse"
                                                        : "ui_delivered_number_damage");
                sprintf(msgbuf, g_strtable.lookup(&k)->c_str(), round_float(&dmg));
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
                    sprintf(hbuf, g_strtable.lookup(&k)->c_str(), round_float(&dmg));
                }
                {
                    tstring t(hbuf);
                    ::log_message(&t, 0x41a00000);               // 20.0f seconds
                }
            } else {
                const char* key = 0;
                shown = g_opt_damage_msgs;
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
                        sprintf(hbuf, g_strtable.lookup(&k)->c_str(), round_float(&dmg));
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
        if (last_blow().item < 0) {
            var = 0.15f;
            kind = 0x65;
        } else if (g_items[last_blow().item].is_ranged()) {
            var = 0.1f;
            kind = (id == g_mission->player_id) ? 0x66 : 0x64;
        } else {
            bool blunt = (last_blow().item_mod & itp_wooden_attack) != 0 || last_blow().kind == 2;
            var = 0.15f;
            if (dmg < 9.0f) {
                if (hit_part < 14)
                    kind = blunt ? 0x79 : 0x75;
                else
                    kind = blunt ? 0x7b : 0x77;
            } else {
                if (hit_part < 14)
                    kind = blunt ? 0x7a : 0x76;
                else
                    kind = blunt ? 0x7c : 0x78;
            }
        }
        play_sound_var(&kind, &var);
    }

    // ---- a missile that sticks in the body ----
    if (last_blow().missile != 0 && g_items[last_blow().missile->ammo.item].flags != itcf_throw_stone &&
        !(g_mission->terrain.f158 & 1) && dmg > 7.0f && (health < 1.0f || last_blow().bone != 9 || dmg > 75.0f)) {
        rgl::mat4 M = body->m * *(rgl::mat4*)body->skel->rec(last_blow().bone);
        float len = last_blow().missile->ammo_scale() * 0.7f;
        matrix mm;
        mm.o = vector4(last_blow().pos.x - len * last_blow().dir.x, last_blow().pos.y - len * last_blow().dir.y, last_blow().pos.z - len * last_blow().dir.z);
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
    if (dmg > 1.0f && !(g_mission->terrain.f158 & 0x100)) {
        float blood = 1.0f;
        if (last_blow().kind == 2)
            blood = 0.2f;
        else if (last_blow().kind == 1)
            blood = 0.7f;
        if (g_opt_blood > 0 && last_blow().kind != 2) {
            bool go = false;
            if (g_opt_blood == 2) {
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
            if (f30 == m->player_id && id == m->f60 && (g_mission_templates[m->mode].flags >> 1 & 1) &&
                f8c.item >= 0 && !(g_items[f8c.item].type_flags >> 12 & 1)) {
                GameData* g = g_game;
                ItemRef* horse = &g->chars[g->player].equip[8];
                if ((horse->m4 & 0xff000000) == (unsigned)imod_lame << 24) {
                    if (frand() < 0.2f) {
                        tstring k("ui_horse_fell_dead");
                        g_gui.add(*g_strtable.lookup(&k), 0xffdd6655);
                        horse->item = -1;
                    }
                } else {
                    if (frand() < 0.2f) {
                        {
                            tstring k("ui_horse_crippled");
                            g_gui.add(*g_strtable.lookup(&k), 0xffeebbbb);
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
                ((gui_timer*)&t40)->set_delay(-2.4f);
                if (f30 >= 0) {
                    g_mission->agents.get(f30)->set_order(0, 0x123, 0x1000081, 1.0f);
                    vector4 z;
                    z.x = 0.0f;
                    z.y = 0.0f;
                    z.z = 0.0f;
                    g_mission->agents.get(f30)->impulse = z;
                    ((gui_timer*)&g_mission->agents.get(f30)->t40)->set_delay(-2.4f);
                }
            } else {
                if (frand() <= 1.0f) {
                    set_order(0, 0x1a3, 0x2000080, 1.0f);
                    int kind = 4;
                    play_hit_sound(&kind);
                    plen = g_anims[0x1a3].info->length;
                    if (f30 >= 0)
                        g_mission->agents.get(f30)->set_order(0, 0x122, 0x1000081, 1.0f);
                } else {
                    set_order(0, 0x1a2, 0x2000080, 1.0f);
                    plen = g_anims[0x1a2].info->length;
                    int kind = 4;
                    play_hit_sound(&kind);
                    if (f30 >= 0)
                        g_mission->agents.get(f30)->set_order(0, 0x121, 0x1000081, 1.0f);
                }
                if (plen < 0.01f)
                    plen = 1.0f;
            }
            ((gui_timer*)&t5918)->set_delay(-plen);
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
        if (f2c != -1) {
            anim_a = 0x120;
        } else {
            float vx = apos.x - pos.x;
            float vy = apos.y - pos.y;
            float vz = apos.z - pos.z;
            if (vx * vel.x + (vy * vel.y + vz * vel.z) > 0.0f) {
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
        goto common;
    }
    if (troop >= 0) {
        if (dmg < 1.0f) {
            float r = (frand() + 1.0f) * (float)(g_game->chars[troop].f58[1] + 15) * 0.2f;
            if (!(last_blow().dmg_in < r)) goto bee;
        } else {
        bee:
            if (!(dmg < g_damage_interrupt_attack_treshold)) goto c25;
        }
        if ((int)last_blow().attacker >= 0 && g_mission->agents.get(last_blow().attacker)->f10 == 0)
            stunned = true;
    }
c25:
    if (f2c == -1 && last_blow().item >= 0 &&
        (ik = &g_items[last_blow().item], ik->is_ranged() || (ik->type_flags & 0xff) == itp_type_polearm)) {
        if (stunned)
            goto common;
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
        goto common;
    }
    if (f2c != -1)
        goto h_mounted;
ded:
    {
        float d = rgl::dot(vel, last_blow().dir);
        if (!(d < 0.0f))
            goto dfail;
    }
    if (!(((last_blow().flags & 0x10) && last_blow().dmg_in > 7.0f) || (last_blow().dmg_in > 24.0f && frand() < 0.4f)))
        goto dfail;
    if (last_blow().kind != 2)
        goto dfail;
    if (anim[0].cur == 0x125)
        goto dfail;
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
ded_tail:
    if (stunned)
        goto common;
impulse:
    if (flags3c & 1) {
        flags3c &= ~1u;
        ((gui_timer*)&t40)->set_delay(-0.6f);
        impulse.x = imp.x + impulse.x;
        impulse.y = imp.y + impulse.y;
        impulse.z = imp.z + impulse.z;
        if ((int)last_blow().attacker >= 0 && g_mission->agents.get(last_blow().attacker)->f10 == 1) {
            impulse.x = impulse.x;
            impulse.y = impulse.y;
            impulse.z = impulse.z + 1.0f;
        }
    }

common:
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
        if (m->terrain.f158 & 1)
            goto set_cB;
        if ((g_game->chars[troop].flags30 & tf_allways_fall_dead) == 0) {
            if (last_blow().kind == 2)
                goto set_cB;
            if (troop < 0)
                goto set_cB;
            if (g_game->chars[troop].is_hero())
                goto set_cB;
            if (!(frand() > surg))
                goto set_cB;
        }
        cA = true;
        goto ab_done;
    dfail:
        anim_b = move_anim(1);
        if (!(last_blow().flags & 0x10)) {
            if (!(last_blow().dmg_in > 15.0f))
                goto common;
            if (!(frand() < 0.3f))
                goto common;
        }
        {
            float e = (frand() - 0.5f) * 0.5f;
            float f = (frand() + 0.6f) * (last_blow().dmg_in + 30.0f) * 0.02f;
            vector4* sv = &body->m.s;
            imp.x = f * last_blow().dir.x + e * sv->x;
            imp.y = f * last_blow().dir.y + e * sv->y;
            imp.z = f * last_blow().dir.z + e * sv->z;
        }
        goto ded_tail;
    h_mounted:
        anim_b = move_anim(0);
        goto common;
    set_cB:
        cB = true;
    ab_done:
        f5830 = 3;
        tm.start = g_ticks[tm.clock];
        mount_id = f2c;
        body->skel->f68 = 1;
        float len = 0.5f;
        if (anim_a >= 0) {
            len = g_anims[anim_a].info->length;
            if (len < 0.01f)
                len = 0.5f;
        }
        ((gui_timer*)&t5918)->set_delay(-len);
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
                ((gui_timer*)&t40)->set_delay(-1.4f);
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
                    g->add_experience(atk_troop, (int)((float)xp * g_player_xp_multiplier));
                else
                    g->add_experience(atk_troop, (int)((float)xp * g_hero_xp_multiplier));
            } else {
                g_mission->agents.get(atk_rider)->xp_pool += (int)((float)xp * g_regulars_xp_multiplier);
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
                if (last_blow().attacker == g_mission->player_id && g_opt_shot_difficulty != 0) {
                    float sd = tt;
                    if (last_blow().bone == 9)
                        sd = tt * 1.2f;
                    {
                        tstring k("ui_shot_difficulty");
                        sprintf(msgbuf, g_strtable.lookup(&k)->c_str(), (double)sd);
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
                        sprintf(msgbuf, g_strtable.lookup(&k)->c_str(), namebuf, nw);
                    }
                } else {
                    tstring k("ui_your_proficiency_in_value_has_improved_by_number_to_number");
                    sprintf(msgbuf, g_strtable.lookup(&k)->c_str(), namebuf, diff, nw);
                }
                g_gui.add(tstring(msgbuf), 0xffffdf44);
            }
        }
    }

    // ---- the impact particles ----
    if (dmg > 1.0f && !(g_mission->terrain.f158 & 1) && g_opt_hit_fx != 0) {
        float hi = 50.0f;
        float lo = 1.0f;
        float k = ((dmg + 5.0f) * g_blood_multiplier) * 0.1f + 1.0f;
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
            e1 = g_voice_default_a;
        if (e2 == 0)
            e2 = g_voice_default_b;
        if (e1 >= 0) {
            Emit* em = g_mission->spawn_effect(e1, (const rgl::mat4*)&m2, (int)(ef + ef));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e1].b.x;
            v.y = s * g_emit_defs.defs[e1].b.y;
            v.z = s * g_emit_defs.defs[e1].b.z;
            ((Obj3*)em)->set_v184(&v);       // the emitter is a scene object with a placement block (game_util.h Obj3; the code is shared)
            *(float*)&em->f198 = ef * *(float*)&g_emit_defs.defs[e1].f24;
        }
        if (e2 >= 0) {
            Emit* em = g_mission->spawn_effect(e2, (const rgl::mat4*)&m2, (int)(ef * 4.0f));
            float s = (ef + 10.0f) * 0.1f;
            vector4 v;
            v.x = s * g_emit_defs.defs[e2].b.x;
            v.y = s * g_emit_defs.defs[e2].b.y;
            v.z = s * g_emit_defs.defs[e2].b.z;
            ((Obj3*)em)->set_v184(&v);       // the emitter is a scene object with a placement block (game_util.h Obj3; the code is shared)
            *(float*)&em->f198 = ef * *(float*)&g_emit_defs.defs[e2].f24;
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
        if (g_opt_kill_msgs != 0) {
            char msgbuf[512];
            tstring unused_s;
            if (atk_troop >= 0) {
                tstring k("ui_value_killed_by_value");
                const char* n1 = g_game->chars[atk_troop].display_name()->c_str();
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(msgbuf, g_strtable.lookup(&k)->c_str(), n2, n1);
            } else {
                tstring k("ui_value_fell_dead");
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(msgbuf, g_strtable.lookup(&k)->c_str(), n2);
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
        if (g_opt_kill_msgs != 0) {
            char namebuf[256];
            if (atk_troop >= 0) {
                tstring k("ui_value_knocked_unconscious_by_value");
                const char* n1 = g_game->chars[atk_troop].display_name()->c_str();
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(namebuf, g_strtable.lookup(&k)->c_str(), n2, n1);
            } else {
                tstring k("ui_value_fell_unconscious");
                const char* n2 = g_game->chars[troop].display_name()->c_str();
                sprintf(namebuf, g_strtable.lookup(&k)->c_str(), n2);
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
extern char* g_propkinds;                          // 0x5e29dc (PropKind, 0x78 bytes: flags at +0x10, the sound set at +0x6c)
extern rgl::matrix g_last_hit_frame;                // 0xa708d0
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
                                    g_gui.add(*g_strtable.lookup(&tstring("ui_you_hit_a_friendly_troop")),
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
                    if (rd < 0 || !is_neutral(agent_by_id(rd))) {
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
            SceneEnt* e = (SceneEnt*)((Ctx*)g_mission->scene_world)->find_entity(eid);
            if (e != 0) {
                int pi = e->id;
                e->type = 0xc;
                if (pi >= 0 && (unsigned)pi < (unsigned)((Vec1c8*)&g_mission->props)->size()) {
                    PlacedObj* pr = &((PlacedObj*)((Vec1c8*)&g_mission->props)->first)[pi];
                    prop_done = 1;
                    if (pr->kind == 0) {
                        unsigned kf = prop_flags(g_propkinds + pr->id * 0x78) & 0x8000;
                        if ((kf != 0 && 0.0f < pr->hp()) || kf == 0) {
                            float dmg = 0.0f;
                            int flg = 0;
                            if (kf != 0 && 0.0f < pr->hp()) {
                                hit_damage(0, missile, &dmg, (unsigned*)&flg, 0xffffffff, scale);
                                pr->hp() = pr->hp() - dmg;
                            }
                            char* kind = g_propkinds + pr->id * 0x78;
                            ((Placement*)g_game)->set_matrix(&pr->m18);
                            g_game->f1a1f0 = pi;
                            g_a70448 = pi;
                            g_a70450 = (__int64)dmg;
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
                                        e->add_extra((res_obj*)m);
                                        ragdoll = true;
                                    }
                                }
                            } else {
                                *(rgl::mat3*)&frame = *(const rgl::mat3*)&fr1;
                            }
                            g_last_hit_frame = frame;
                            ((SoundSet*)(kind + 0x6c))->play(-0x2a);
                            if ((prop_flags(kind) & 0x8000) != 0 && pr->hp() <= 0.0f)
                                ((SoundSet*)(kind + 0x6c))->play(-0x2b);
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
extern int g_ctl_mode;                              // 0x5aa074
extern int g_stick_mode;                            // 0x5aa078
int __cdecl ftoi_helper(int x);                     // 0x435440
static __forceinline ItemRef* cur_ptr(Agent* a) { ItemRef* p = 0; if (a->wield[0] >= 0) p = &a->pairs[a->wield[0]]; return p; }
#define ACT anim[1].cur          // +0x100 current animation id
#define NEXTACT anim[1].next     // +0x104
#define PRIO1 anim[1].events     // +0x114
#define PROG anim[1].scale       // +0x118 animation progress
#define TYPE (ik->type_flags & 0xff)
#define LOG_TIMED(str) { ::log_message(g_strtable.lookup(&tstring(str)), 0x41a00000); }
// @ 0x45c220
void Agent::update_attack(float dt)
{
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
            sk->update_poses();
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
        int S = active;
        if (S == 0 && f5610 == 0) {
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
        } else if (S == 1) {
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
                        if (PROG < 0.3f && g_ctl_mode > 0) {
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
                        int kind = 3;
                        play_hit_sound(&kind);
                        if (f2c == -1 && frand() < (f57c0 - 1.0f) * 3.5f) {
                            if (id == g_mission->player_id || frand() < 0.3f) {
                                kind = 2;
                                play_class_sound(&kind);
                            } else {
                                kind = 3;
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
            if (r != 1) {
                if (r == 2)
                    goto clear_state;
                goto chk99;
            }
            if (hit.dmg_out < 5.0f && cur.item >= 0) {
                set_order(1, snd_c, 0x1400023, damage_factor(0));
                active = 5;
            } else {
                active = 3;
            }
            goto tail;
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
        int slot = 0, missing, total;
        best_ammo(-1, &missing, &total, &slot);
        if (TYPE == itp_type_crossbow) {
            if (cur.max_amount() - (cur.m4 & 0xff) == 0) {
                float var = 0.0f;
                links[wield[0]]->cap_all(&var);
                if (slot >= 0)
                    links[slot]->vis = 0;
            } else {
                float var = 100.0f;
                links[wield[0]]->cap_all(&var);
                if (slot >= 0)
                    links[slot]->vis = 0xffff;
            }
        } else if (TYPE == itp_type_bow && links[wield[0]] != 0) {
            float var = 0.0f;
            links[wield[0]]->cap_all(&var);
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
                            g_gui.add(*g_strtable.lookup(&tstring("ui_cant_reload_this_weapon_on_horseback")), -1);
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
                    if (total > 0)
                        LOG_TIMED("ui_no_more_bolts")
                    else
                        LOG_TIMED("ui_you_are_not_carrying_any_bolts")
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
            } else {
                hit_once = 0;
                prop_done = 0;
                active = 1;
                timer.start = g_ticks[timer.clock];
                start_shoot(0);
                if (snd_a != -1) {
                    set_order(1, snd_a, 0x02400002, damage_factor(0));
                    unsigned __int64 sm = ik->flags & itcf_shoot_mask;
                    if (sm == itcf_throw_axe || sm == itcf_throw_knife || sm == itcf_throw_stone ||
                        sm == itcf_shoot_musket || TYPE == itp_type_thrown || TYPE == itp_type_crossbow) {
                        variant = 4;
                        this->cur = 0.0f;
                    } else {
                        this->cur = 0.0f;
                        variant = ((TYPE != itp_type_pistol) - 1) & 5;
                    }
                }
            }
        } else if (S == 1) {
            if ((unsigned char)ik->type_flags == itp_type_bow && PROG > 0.3f) {
                if (PROG > 0.46f && links[wield[0]] != 0) {
                    float x = (PROG - 0.46f) * 2.5f;
                    float hi = 1.0f;
                    float lo = 0.0f;
                    float var = clamp(&x, &lo, &hi) * 100.0f;
                    links[wield[0]]->cap_all(&var);
                }
                links[slot]->vis = 0xffff;
                update_quiver();
            }
            if (in_flags & 0x200) {
                order1(this, -1, 2);
                if ((unsigned char)ik->type_flags == itp_type_bow && links[slot] != 0) {
                    links[slot]->vis = 0;
                    update_quiver();
                }
                goto clear_state;
            }
            if (in_flags & 0x100)
                goto tail;
            if (!(PROG > 0.99f) && ((TYPE != itp_type_crossbow && TYPE != itp_type_musket) || !(PROG > 0.2f)))
                goto tail;
            {
                unsigned rel = 0;
                if (TYPE != itp_type_bow) {
                    if (TYPE == itp_type_thrown) {
                        rel = 0x40001b;
                    } else if (TYPE == itp_type_crossbow || TYPE == itp_type_pistol || TYPE == itp_type_musket) {
                        ItemRef* rp = cur_ptr(this);
                        if (rp != 0)
                            rp->add_amount(1);
                    }
                }
                set_order(1, snd_b, rel | 0x1000002, damage_factor(0));
            }
            if ((unsigned char)ik->type_flags != itp_type_thrown) {
                shoot();
                if ((unsigned char)ik->type_flags == itp_type_bow)
                    top_up_ammo(1);
                if (slot >= 0 && links[slot] != 0)
                    links[slot]->vis = 0;
                active = 0;
            } else {
                active = 3;
            }
            if (TYPE == itp_type_bow) {
                int kind = 0x25;
                float var = 0.1f;
                if (id != g_mission->player_id)
                    play_sound_var(&kind, &var);
                else
                    play_sound_var(&kind, &var);
            } else if (TYPE == itp_type_crossbow) {
                float var = 0.1f;
                int kind = 0x26;
                play_sound_var(&kind, &var);
            } else if (TYPE == itp_type_thrown) {
                unsigned __int64 sm = ik->flags & itcf_shoot_mask;
                int kind;
                float var;
                if (sm == itcf_throw_javelin) {
                    kind = 0x27;
                    var = 0.1f;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_axe) {
                    var = 0.1f;
                    kind = 0x28;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_knife) {
                    var = 0.1f;
                    kind = 0x29;
                    play_sound_var(&kind, &var);
                } else if (sm == itcf_throw_stone) {
                    kind = 0x2a;
                    var = 0.1f;
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
            g_last_hit_frame = body->m * *(rgl::mat4*)(body->skel->rec(ik->ammo_skill()));
            ((SoundSet*)((char*)ik + 0x228))->play(-0x33);
        } else if (S == 2) {
            if (ACT != f5620) {
                active = 0;
            } else if ((in_flags & 0x130200) ||
                       (ACT == 0x7b && PROG < 0.8f && id == g_mission->player_id && (in_flags & 0xf))) {
                order1(this, -1, 0x3d);
                active = 0;
            } else if (PROG > 0.99f) {
                ItemRef* rp = cur_ptr(this);
                if (rp != 0) {
                    unsigned u = rp->m4 & 0xff;
                    int n = top_up_ammo(u);
                    ItemRef* rq = cur_ptr(this);
                    rq->b.amount = 0;
                    rq->m4 |= u - n;
                }
                update_quiver();
                goto clear_state;
            } else if (links[wield[0]] != 0) {
                float lo2 = 0.4f, hi2 = 0.66f;
                if (f2c != -1) {
                    lo2 = 0.16f;
                    hi2 = 0.5f;
                }
                float x = (PROG - lo2) / (hi2 - lo2);
                float hi = 1.0f;
                float lo = 0.0f;
                float var = clamp(&x, &lo, &hi) * 100.0f;
                links[wield[0]]->cap_all(&var);
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
                if (PROG < 0.5f && g_stick_mode != 0) {
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
                        anim[1].phase = p * 0.85f;
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
struct AnimSlotCall { bool set(int id, unsigned prio); };   // 0x4378a0 out of line (agent_stats.cpp)
struct Vec3;
void __cdecl bounce(Vec3* out, Vec3* v, const Vec3* n);                    // 0x432e30 (the result is *out)
extern float g_horse_charge_damage_multiplier;                              // 0xa1819c
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
            SceneEnt* ent = (SceneEnt*)((Ctx*)g_mission->scene_world)->find_entity(fid);
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
        g_mission->scene_world->collect_boxed(&bmin, &bmax, (std::vector<unsigned>*)&ents);
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
                        np = vector4((np.x - old.x) * g + old.x, g * (np.y - old.y) + old.y, g * (np.z - old.z) + old.z);
                    } else if (f10 == 1 && vel_xy.y > 3.0f && o->f10 == 1) {
                        hit_ok = !(rgl::dot(body->m.f, o->body->m.f) > 0.0f) || !(o->vel_xy.y > 0.2f);
                    }
                    if (f10 == 0 && o->f10 == 0 && o->f2c == -1) {
                        vec2 d2;
                        d2.x = o->pos.x - (np.x + pos.x);
                        d2.y = o->pos.y - (np.y + pos.y);
                        float len = d2.normalize();
                        if (len < 0.7f) {
                            float pen = (0.7f - len) * -0.6f;
                            vec2 tt;
                            tt.x = d2.x * pen;
                            tt.y = d2.y * pen;
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
                            float dmg = (((nn.z * v56a4.z + nn.y * v56a4.y + nn.x * v56a4.x) + 0.1f) * (float)s2 * g_horse_charge_damage_multiplier) * vel_xy.y * 0.015f;
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
    g_mission->scene_world->query_box(&bmin.x, &bmax.x, mask, (shape_base**)shapes, &count);
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
            } else if (pos.z + 0.4f < contact.point.z && 0.95f < contact.normal.z && np.z <= 0.0f) {
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
    world* scene = g_mission->scene_world;
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
        ((level*)scene)->floor_at(&np.x, &fz, &nf);
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
                nf = *scene->faces;
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
            ((level*)scene)->floor_at(&probe.x, &fz2, &tf);
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
            if (defs->count > 0x19e && ((anim[0].events & 0x1ffff) < 0x22 || (anim[0].cur == -1 && anim[0].next == -1))) {
                anim[0].next = 0x19e;
                anim[0].events = 0x1000021;
                anim[0].rate = 1.0f;
                if ((defs->defs[0x19e].flags & 0x400) && anim[1].cur != -1 && (anim[1].events & 0x1ffff) < 0x21)
                    ((AnimSlotCall*)&anim[1])->set(-1, 0x21);
            }
            snd = 0x22;
            if (fz < g_mission->terrain.base_z)
                snd = 0x24;
        } else {
            if (defs->count > 3 && ((anim[0].events & 0x1ffff) < 0x22 || (anim[0].cur == -1 && anim[0].next == -1))) {
                anim[0].next = 3;
                anim[0].events = 0x1000021;
                anim[0].rate = 1.0f;
                if ((defs->defs[3].flags & 0x400) && anim[1].cur != -1 && (anim[1].events & 0x1ffff) < 0x21)
                    ((AnimSlotCall*)&anim[1])->set(-1, 0x21);
            }
            snd = 0x1e;
            if (fz < g_mission->terrain.base_z)
                snd = 0x20;
        }
        float var = 0.1f;
        play_sound_var(&snd, &var);
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
struct AnimDefW { char pad0[0x10]; SubV* subs; char pad14[0x30 - 0x14]; };   // g_anims entry as 0x45f660 reads it
extern int g_default_fx;                                // 0x8ed50a8
extern int g_snd_224;                                   // 0x5aa224 (footstep sounds)
extern int g_snd_228;                                   // 0x5aa228
extern int g_snd_22c;                                   // 0x5aa22c
extern int g_snd_230;                                   // 0x5aa230
extern int g_snd_2e0;                                   // 0x5aa2e0
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
            if (old <= ((AnimDefW*)g_anims)[anim[1].cur].subs[anim[1].sub].ev[k] &&
                ((AnimDefW*)g_anims)[anim[1].cur].subs[anim[1].sub].ev[k] < now) {
                if (k == 0) {
                    float pitch;
                    switch (anim[1].cur) {
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_snd_230, 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_snd_224, 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    }
                } else if (k == 1) {
                    float pitch;
                    switch (anim[1].cur) {
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sound_mgr.play(g_snd_22c, 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.4f + 0.8f;
                        g_sound_mgr.play(g_snd_228, 0, (const float*)&body->m.o, pitch, 0);
                        break;
                    }
                }
            }
        }
    }
    }
    f56e4 = f56e0;
    f56e0 = anim[0].scale;
    float speed = (float)sqrt(vel_xy.y * vel_xy.y + vel_xy.x * vel_xy.x);
    if (anim[0].cur >= 0) {
        unsigned flags = ((AnimDefW*)defs->defs)[anim[0].cur].subs[anim[0].sub].flags;
        if (disabled == 0 && (flags & 0x300)) {
            float pitch;
            int k = 0;
            int kind = 0;
            do {
                if (f56e4 <= ((AnimDefW*)g_anims)[anim[0].cur].subs[anim[0].sub].ev[k] &&
                    ((AnimDefW*)g_anims)[anim[0].cur].subs[anim[0].sub].ev[k] < f56e0) {
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
                            g_sound_mgr.play(g_snd_2e0, 0, (const float*)&body->m.o, pitch, 0);
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
                                vec2 d;
                                d.x = vel_xy.x;
                                d.y = vel_xy.y;
                                d.normalize();
                                vec2 t;
                                t.x = d.x * s;
                                t.y = d.y * s;
                                m.f.x = vel_xy.x;
                                m.f.y = vel_xy.y;
                                m.f.z = 0.0f;
                                m.o.x = t.x;
                                m.o.y = t.y;
                                m.f.normalize();
                                m.orthonormalize();
                                *(rgl::matrix*)&fx1b0->local = m;
                                *(rgl::matrix*)&fx1b4->local = m;
                                float v;
                                if (f10 == 1)
                                    v = speed * 10.0f + 20.0f;
                                else
                                    v = speed * 3.0f + 5.0f;
                                ((Obj3*)fx1b0)->reset_pending((int)(v * 0.5f));
                                ((Obj3*)fx1b4)->reset_pending((int)(v * 0.4f));
                                float ux = d.x * 0.7f;
                                float uy = d.y * 0.7f;
                                m.o.x = ux + m.o.x;
                                m.o.y = m.o.y + uy;
                                *(rgl::matrix*)&fx1b8->local = m;
                                ((Obj3*)fx1b8)->reset_pending(1);
                            } else if (fx1ac && f10 == 1) {
                                float v = 0.0f;
                                if (speed > 5.5f && fbc && (((shape_base*)fbc)->m8 & 0x4000))
                                    v = speed + 10.0f;
                                if (fx1ac->idx != g_default_fx)
                                    v = v * 3.0f;
                                ((Obj3*)fx1ac)->reset_pending((int)(v * 0.15f));
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
            poll_input(*(int*)&dt0, (InputState*)&in_flags);
        }
        if (f2c >= 0) {
            unsigned id = f2c;
            Agent* o = g_mission->agents.pages[id >> 4] + (id & 0xf);
            *(InputState*)&o->in_flags = *(InputState*)&in_flags;
        }
    }
    if (dt > 0.0f) {
        process_blow(*(int*)&dt0);
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
            if ((&v98.x)[j] > 0.0f) {
            zero:
                (&v98.x)[j] = 0.0f;
            }
        } else if (!((&v98.x)[j] > 0.001f)) {
            goto zero;
        } else {
            (&v98.x)[j] = (&v98.x)[j] - ((&v98.x)[j] + 1.0f) * dt;
            if ((&v98.x)[j] < 0.0f)
                goto zero;
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

