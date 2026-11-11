// The campaign map screen: MapPanel's per-frame target picker and info panel, with the label and tooltip helpers they call.
// One translation unit, in address order, 0x5191e0..0x51be20 (each function's callees follow their first caller). The original saw neither
// 0x51be20 dist_is_zero nor 0x51be60 party_counts when it compiled update, so those and 0x51c090 are map_tooltip.cpp:
//   0x5191e0  MapPanel::update_target     hover the party-label column, cast a screen ray, pick a party agent or a ground
//                                         item under the cursor, and turn the selection into a move/attack order
//   0x51a2d0  PlaneTrace::intersect       segment/plane intersection
//   0x51a3a0  rgl::world::collect         the entities whose type_id() bit-mask contains the key
//   0x51a420  MapPanel::set_label         rebuild the map party label + tooltip
//   0x51a690  MapWidget::project          matrix transform + viewport mapping, 2 floats out
//   0x51a760  gui_manager::set_pos        clamp a label offset inside the mesh bounds
//   0x51a880  gui_manager::add_label      lay out one text row into the "ui_font" mesh
//   0x51aa20  gui_manager::set_quad       write the 4 corner points of the label background
//   0x51abd0  MapPanel::update            the info panel repaint; (type, id): 0 = nothing, 1 = a party, 3 = a fleet/battle-table
//                                         slot, 4 = a party looked up by id
// Names and field names are provisional; every offset is read off the disassembly.
#include <windows.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../include/vec.h"
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
#include "../shared_vars.h"
#include "../key_names.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;                 // static-CRT float marker

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----
int g_sel_label = -2;                         // 0x5aa3a8 (its label index)
int g_sel_party = -1;                         // 0x5aa3ac (currently selected party id)
int g_sel_id = -1;                            // 0x5aa3b0
int g_sel_type = -1;                          // 0x5aa3b4 last painted selection

#include "../tstring.h"
void* operator new(unsigned size);      // 0x53545e
#include "../str_table.h"
#include "../res_mgr.h"                         // resmgr (0x5d6fa0): the one definition for the rgl.h-universe modules
#include "../font.h"
extern Font* g_font;                    // 0x5e2934
#include "../../include/mesh_prims.h"
using rgl::point20;
#include "../../rgl/mesh.h"                     // rgl::mesh (calc_bounds 0x438d80)
#include "../../rgl/scene/world.h"              // rgl::world: the scene object g_level points to (ground = the strategy-map cell table); collect 0x51a3a0
using rgl::mesh;
// shape_base: 0x20 bytes; slot 0 dtor, slot 1 shape_type (the picker's `v1() != 2` mask key).
#include "../../include/shape.h"
#include "../../include/mat4.h"

// ---- map / game records ----
struct shape_base;
struct AgentRec;                             // 0x98    agent record
struct FactionRec;

#include "../../include/game_data.h"
#include "../../include/party.h"
#include "../../include/troop.h"
#include "../../include/game_recs.h"
#include "../rec98.h"                       // Rec98: the 0x98-byte record, a = f54, b = f58
#include "../module_settings.h"             // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../rgl/text_stream.h"
#include "../battle_rec.h"                  // BattleTable (GameData+0xcc; get 0x48ef20)
#include "../skill_cap.h"
#include "../map_widget.h"                  // MapWidget (offsets +0xe0 rect, +0x1ac frustum, +0x274 its frame)
#include "../ui_widget.h"
#include "map_panel.h"                      // MapPanel / MapView
#include "../../include/level_global.h"     // g_level (0xa732b4)
#include "../ui_globals.h"
#include "../config/config_block.h"         // g_cfg (0x5a9f98); cheat_mode is +0x24 = 0x5a9fbc

struct AgentRec {                            // 0x98
    char   pad00[0x10];
    int    kind;                             // +0x10
    char   pad14[0x54 - 0x14];
    int    party;                            // +0x54
    char   pad58[0x98 - 0x58];
};

struct BigRec {                              // 0x88  world-map cell record
    char     pad0[0x24];
    unsigned mask;                           // +0x24
    char     pad28[0x88 - 0x28];
};



struct GroundItem {                          // 0x5c  fleet entry
    vector4  pos;                            // +0x00
    char     pad10[0x18 - 0x10];
    unsigned flags;                          // +0x18
    char     pad1c[0x5c - 0x1c];
};

// a plane record: point at +0x20, normal at +0xa0 (it is the face shape of include/shape.h, whose intersect() is the same function;
// kept as this local view because the harness cannot build a polymorphic `this` for the verified PASS row of 0x51a2d0)
struct PlaneTrace {
    char pad0[0x20];
    vector4 pt;                         // +0x20
    char pad30[0xa0 - 0x30];
    vector4 nrm;                        // +0xa0
    void intersect(vector4* out, const vector4* a, const vector4* b);   // 0x51a2d0 ret 0xc
};

// 0x407f70, owned by rgl/shapes/shapes.cpp. The original saw this body when compiling project: `in` does not escape, which
// fixes update's pos copy schedule. Not a rgl_math.h inline: visible there (plain or noinline), it breaks terrain_render's
// ShadowMap::extend 0x468c20 (64 bytes), which needs the body opaque, so this TU keeps its local copy.
inline __declspec(noinline) vector4 rgl::matrix::transform_point_to_local(const vector4& p) const
{
    vector4 d(p.x - o.x, p.y - o.y, p.z - o.z);
    return vector4(s.x * d.x + s.y * d.y + s.z * d.z, f.x * d.x + f.y * d.y + f.z * d.z, u.x * d.x + u.y * d.y + u.z * d.z);
}

struct camera_ray {
    void screen_to_world(vector4* near_pt, vector4* far_pt);   // 0x4bd2f0 (game/config/config.h)
};


struct DistObj;
bool __fastcall dist_is_zero(DistObj* o);                                 // 0x51be20 (map_tooltip.cpp)

// ---- layout guards (MSVC 13 has no static_assert: a mismatch is a negative array size) -------------
// Sizes: every record's stride appears in the original as an `imul ..., <stride>` immediate.
typedef char assert_Faction_300  [(sizeof(FactionRec)    == 0x300)   ? 1 : -1];
// Field offsets: taken straight from the original's displacements (see the header comment).
typedef char assert_Faction_f10  [(offsetof(FactionRec, f10)    == 0x10)    ? 1 : -1];
typedef char assert_Faction_key  [(offsetof(FactionRec, key)    == 0x14)    ? 1 : -1];
typedef char assert_Faction_f24  [(offsetof(FactionRec, f24)    == 0x24)    ? 1 : -1];
typedef char assert_Faction_color[(offsetof(FactionRec, color)  == 0x28)    ? 1 : -1];
typedef char assert_Faction_rel  [(offsetof(FactionRec, rel)    == 0x2c)    ? 1 : -1];
typedef char assert_Rec98_a      [(offsetof(Rec98, f54)      == 0x54)    ? 1 : -1];
typedef char assert_Rec98_b      [(offsetof(Rec98, f58)      == 0x58)    ? 1 : -1];
typedef char assert_Panel_view   [(offsetof(MapPanel, view)         == 0x30)    ? 1 : -1];
typedef char assert_Panel_timer  [(offsetof(MapPanel, timer)        == 0x2cc)   ? 1 : -1];
typedef char assert_View_matrix  [(offsetof(MapView, pad04)         == 0x4)     ? 1 : -1];

// ======================================================================================================
// @ 0x5191e0
void MapPanel::update_target()
{
    // ---- 0x5191ee: party-label column hover ---------------------------------------------------------
    int count;
    vector4 nearp, np, fp;
    vector4 dir;
    if (f2d5) {
        // The mouse is copied before the call and the box is moved in place: w and h end up in mx, dy reads mn.y.
        float dx = g_gui.mouse[0];
        float dy = g_gui.mouse[1];
        vector4 mn, mx;
        g_gui.label_obj->calc_bounds(&mn, &mx);
        float x0 = g_gui.box_x + mn.x;
        float x1 = g_gui.box_x + mx.x;
        mn.y += g_gui.box_y;
        mx.y += g_gui.box_y;
        g_game->cur_label = -1;
        dx -= x0;
        dy -= mn.y;
        mx.x = x1 - x0;
        mx.y -= mn.y;

        // The original tests the band with `test ah,1; jne` (C0 = less-than *or* unordered), so a NaN
        // coordinate counts as OUT of the band and the label is dropped; `!(x < lo)` keeps a NaN in.
        if (dx >= -0.01f && mx.x + 0.01f >= dx &&
            dy >= -0.01f && mx.y + 0.01f >= dy) {
            if (dy >= 0.0f) {
                int n = g_game->n_labels;
                // the original handles the hit inside the scan loop (0x519347), so the loop is not unrolled
                for (int i = 0; i < n; i++)
                    if ((float)(i + 1) * 0.015f > dy) {
                    g_game->cur_label = n - i - 1;
                    if (view->is_key_down(8)) {
                        set_label(-1);
                        f2d5 = 0;
                        int sel = g_game->label_target[g_game->cur_label];
                        if (sel == -7) {
                            // "my own party" marker: retarget the player's party
                            if (f2d8 != g_game->f78) {
                                g_game->map_live = 1;
                                g_game->sel_party = g_game->f78;
                                g_game->parties.get(g_game->f78)->action = 4;
                                g_game->parties.get(g_game->f78)->target = f2d8;
                                g_game->parties.get(g_game->f78)->reissue_order();
                            }
                        } else {
                            g_game->emit_event(0x16, f2d8, sel);
                        }
                    }
                    break;
                    }
            }
        } else if (dx < -0.2f || dx > mx.x + 0.2f ||
                   dy < -0.2f || dy > mx.y + 0.2f || view->is_key_down(8)) {
            set_label(-1);
            f2d5 = 0;
        }
        if (f2d5) set_label(f2d8);
    }

    // ---- 0x519496: screen ray ----------------------------------------------------------------------
    // Slots read off the original: the hover box mn/mx above share farp/bestpt (S+0x1c) and nearp (S+0x40); dir shares hp (S+0x2c);
    // the block scopes below are what lets those lifetimes overlay.
    {
    {
        {
            vector4 farp;
            ((camera_ray*)view)->screen_to_world(&nearp, &farp);   // 0x4bd2f0
            dir.x = farp.x - nearp.x;
            dir.y = farp.y - nearp.y;
            dir.z = farp.z - nearp.z;
            np = nearp;
            fp = farp;
            dir.normalize();
        }

        // ---- 0x519517: clip against the world's z range --------------------------------------------
        if (g_script.level->ground) {
            if (np.z < g_script.level->ground->e44.z) {
                float t = fabsf((g_script.level->ground->e44.z - np.z) / dir.z);
                vector4 m = dir * t;
                np.x = np.x + m.x; np.y += m.y; np.z += m.z;
            }
            if (fp.z < g_script.level->ground->e44.z) {
                float t = fabsf((g_script.level->ground->e44.z - fp.z) / dir.z);
                vector4 m = dir * t;
                fp.x -= m.x; fp.y -= m.y; fp.z -= m.z;
            }
            if (np.z > g_script.level->ground->f54.z) {
                float t = fabsf((np.z - g_script.level->ground->f54.z) / dir.z);
                vector4 m = dir * t;
                np.x += m.x; np.y += m.y; np.z += m.z;
            }
            if (fp.z > g_script.level->ground->f54.z) {
                float t = fabsf((fp.z - g_script.level->ground->f54.z) / dir.z);
                vector4 m = dir * t;
                fp.x -= m.x; fp.y -= m.y; fp.z -= m.z;
            }
        }
    }

    // ---- 0x51969b: thin capsule around the clipped segment -----------------------------------------
    {
    capsule cap(0.0f, &np, &fp);
    {
        matrix mtx;
        cap.capsule::transform((const pos*)identity_pos(&mtx));
    }

    // ---- 0x519720: shapes under the cursor ---------------------------------------------------------
    float     bestd = 1000000.0f;
    int       best  = -1;
    shape_base* hits[0x2000];                  // the sweep is told 0x110, but the slot is 0x2000 wide
    vector4 bestpt;
    {
        vector2 seg_a(np.x, np.y);             // the sweep's rectangle corners: x and y only
        vector2 seg_b(fp.x, fp.y);
        g_script.level->query_segment((const float*)&seg_a, (const float*)&seg_b, 0x110, hits, &count);   // 0x439870

        int n = count;
        for (int i = 0; i < n; i++) {
            if (hits[i]->shape_type() != 2) continue;
            shape_base* o = hits[i];
            if (!shape_contact_count(o, &cap, 0, 0)) continue;
            ((PlaneTrace*)o)->intersect(&dir, &np, &fp);   // 0x51a2d0 (the face's intersect is the same code)
            float ddx = dir.x - nearp.x;
            float ddy = dir.y - nearp.y;
            float ddz = dir.z - nearp.z;
            float d = sqrtf(ddz * ddz + ddy * ddy + ddx * ddx);   // the original sums z*z + y*y first, then x*x
            if (!(d < bestd)) continue;
            bestd  = d;
            bestpt = dir;
            best   = i;
        }
    }

    int rec_index = -1;
    int rec_mask  = -1;
    if (best != -1) {
        rec_index = hits[best]->m4;
        rec_mask  = (int)g_script.level->ground->cells[rec_index].ids[8];
    }

    // ---- 0x519829: party agents under the cursor ---------------------------------------------------
    rgl::strategic_entity* bestobj = 0;
    rgl::entity* agents[0x2000];               // world::collect does not bound *count, so the size matters
    int       acount;
    g_script.level->collect(9, agents, &acount);
    bestd += 1.0f;

    for (int i = 0; i < acount; i++) {
        rgl::strategic_entity*& ref = (rgl::strategic_entity*&)agents[i];   // collect(9): the entities of type 9
        if (!ref->f140) continue;
        if (!ref->shape) continue;
        if (!shape_contact_count(ref->shape, &cap, 0, 0)) continue;
        rgl::strategic_entity* o = ref;
        float ddx = nearp.x - o->local.o.x;
        float ddy = nearp.y - o->local.o.y;
        float ddz = nearp.z - o->local.o.z;
        float d = sqrtf(ddz * ddz + ddy * ddy + ddx * ddx);   // the original sums z*z + y*y first, then x*x
        int kind = o->map_kind;
        if (kind == 1) {
            d -= 0.4f;
            if (o->game_id == g_game->f78) d += 0.35f;
        }
        if (!(d < bestd)) continue;
        if (kind == 1 && (g_game->parties.get_w(o->game_id)->flags & 0x80100)) continue;
        bestd   = d;
        bestobj = o;
    }

    // ---- 0x519997: commit the pick -----------------------------------------------------------------
    if (bestobj) {
        if (g_keyboard.down[0xe1] == 0 && g_keyboard.prev[0xe1] != 0 && f2d4 == 0) {
            g_game->n_labels  = 0;
            g_game->cur_label = -1;
            f2d8 = bestobj->game_id;
            g_game->emit_event(0x15, f2d8, 0);
            if (g_game->n_labels > 0) {
                f2d5 = 1;
                set_label(f2d8);
            }
        }
        int kind = bestobj->map_kind;
        if (kind == 1) {
            int id = bestobj->game_id;
            if (!(g_game->parties.get_w(id)->flags & 0x80000))
                update(1, id);
        } else if (kind == 2) {
            update(2, bestobj->game_id);
        }
    } else {
        // ---- 0x519a8b: ground items -----------------------------------------------------------------
        sphere sh;
        int best_item = -1;
        for (unsigned i = 0; g_game->fleet && i < (unsigned)((GroundItem*)g_game->fleet_end - (GroundItem*)g_game->fleet); i++) {   // 0x519b09: jae
            GroundItem* it = (GroundItem*)g_game->fleet + i;
            if (!(g_game->f95 || (it->flags & 0x20000000))) continue;
            float ddx = it->pos.x - nearp.x;
            float ddy = it->pos.y - nearp.y;
            float ddz = it->pos.z - nearp.z;
            float d = sqrtf(ddz * ddz + ddy * ddy + ddx * ddx);   // the original sums z*z + y*y first, then x*x
            if (!(d < bestd)) continue;
            sh.radius = d * 0.01f + 0.03f;
            sh.center = it->pos;
            if (!shape_contact_count(&sh, &cap, 0, 0)) continue;
            bestd     = d;
            best_item = i;
        }
        if (best_item >= 0) update(3, best_item);
        else                update(0, rec_mask);
    }

    // ---- 0x519c01: post-selection (0x519c0d / 0x519c21: a pending label (f2d5) or no key goes straight to 0x519fe8) -----
    if (!f2d5 && view->is_key_down(8)) {
        g_game->map_live = 1;
        g_game->sel_party = g_game->f78;

        if (bestobj) {
            int kind = bestobj->map_kind;
            if (kind == 1) {
                int id = bestobj->game_id;
                int me = g_game->f78;
                if (id != me) {
                    // 0x519c9e..0x519cd5: both arms are the same store (the `0x400` flags test survives
                    // only as a dead select); reproduced verbatim.
                    PartyMgr& pm = g_game->parties;
                    if (pm.get_w(id)->flags & 0x400)
                        pm.get_w(me)->action = 4;
                    else
                        pm.get_w(me)->action = 4;
                    g_game->parties.get_w(g_game->f78)->target = bestobj->game_id;
                    g_game->parties.get_w(g_game->f78)->reissue_order();
                }
            } else if (kind == 3) {
                int id = bestobj->game_id;
                AgentRec* rec = ((AgentRec**)g_game->agent_pages)[(unsigned)id >> 4] + (id & 0xf)   /* 0x519d57: shr */;
                if (rec->kind == 1) {
                    g_game->parties.get_w(g_game->f78)->action = 4;
                    g_game->parties.get_w(g_game->f78)->target = rec->party;
                    g_game->parties.get_w(g_game->f78)->reissue_order();
                } else if (rec->kind == 2) {
                    g_game->parties.get_w(g_game->f78)->action = 9;
                    g_game->parties.get_w(g_game->f78)->target = bestobj->game_id;
                    g_game->parties.get_w(g_game->f78)->reissue_order();
                }
            }
        } else if (best >= 0) {
            bool go = true;
            if (g_keyboard.down[0x1d] && g_cfg.cheat_mode) {
                *(vector4*)&g_game->parties.get_w(g_game->f78)->pos = bestpt;
            } else if (!g_script.level->ground->shares(rec_index,
                                               *(const int*)((char*)g_game->parties.get_w(g_game->f78)->mesh + 4))) {
                go = false;        // 0x519f15: jumps to 0x519fe2 (the target-lost check below), not out of the function
            }
            if (go) {
                g_game->parties.get_w(g_game->f78)->action = 6;
                vector2 xy(bestpt.x, bestpt.y);
                Party* pl = g_game->parties.get_w(g_game->f78);
                pl->order_pos.x = xy.x;
                pl->order_pos.y = xy.y;
                g_game->parties.get_w(g_game->f78)->reissue_order();
            }
        }
    }

    }
    }

    // ---- 0x519fe8: drop the order if the target vanished -------------------------------------------
    GameData* g = g_game;
    {
        int me = g->f78;
        PartyMgr& pm = g->parties;
        if (pm.get_w(me)->action != 4) return;
        if (pm.get_w(me)->target < 0) return;
        // Each access re-runs the party lookup (0x48ad00), exactly as the original does.
        if (!(pm.get_w(pm.get_w(me)->target)->alpha < 0.01f)) {
            Party* tgt = pm.get_w(pm.get_w(me)->target);
            if (tgt->f8c8 < 0 && !(tgt->flags_lo & 0x1080100))
                goto reissue;
        }
        pm.get_w(me)->action = 6;
    }
    {
        int me = g_game->f78;
        PartyMgr& pm = g_game->parties;
        Party* src = pm.get_w(pm.get_w(me)->target);
        Party* dst = pm.get_w(me);
        dst->order_pos.x = src->pos.x;
        dst->order_pos.y = src->pos.y;
        g_game->parties.get_w(g_game->f78)->reissue_order();

        {
            tstring tmp("ui_target_lost");
            tstring* s = g_strtable.lookup(tmp);
            if (g_gui.enabled && g_gui.ring.count < 0x20) {
                int k = (g_gui.ring.head + g_gui.ring.count) % 0x20;
                g_gui.ring.names[k] = *s;
                g_gui.ring.vals[k]   = -1;
                g_gui.ring.count++;
            }
        }
        g = g_game;
    }
reissue:
    g->parties.get_w(g->f78)->reissue_order();
}

// @ 0x51a2d0
void PlaneTrace::intersect(vector4* out, const vector4* a, const vector4* b)
{
    struct V {
        static inline float dot(const vector4& u, const vector4& v) { return u.x * v.x + u.y * v.y + u.z * v.z; }
    };
    float t = V::dot(pt - *a, nrm) / V::dot(*b - *a, nrm);
    out->vector4::vector4((*b - *a) * t + *a);
}

// ---- 0x51a3a0 -------------------------------------------------------------------------------------------


// bit-prefix match: true when every set bit of "key" is also set in "value" (key==0 matches anything).
// The equality case is handled here, so the loop cannot spin on value == key.
static bool mask_contains(unsigned value, unsigned key)
{
    if (value == key)
        return true;
    while (((value ^ key) & 1) == 0) {
        value >>= 1;
        key >>= 1;
    }
    return key == 0;
}

// @ 0x51a3a0
void rgl::world::collect(unsigned key, entity** out, int* count)
{
    *count = 0;
    for (std::vector<entity*>::iterator it = ents.begin(); it < ents.end(); ++it) {
        if (mask_contains((*it)->type_id(), key)) {
            out[*count] = *it;
            ++*count;
        }
    }
}

// @ 0x51a420
void MapPanel::set_label(int id)
{
    tstring unused_a, unused_b;
    char show = 1;
    GameData* g = g_game;
    if (id == g_sel_party) {
        if (g->cur_label == g_sel_label && f2d0 < 1.0f)
            return;
        show = 0;
    }
    g_sel_party = id;
    g_sel_label = g->cur_label;
    f2d0 = 0.0f;
    if (g_gui.label_obj)
        delete g_gui.label_obj;
    g_gui.label_obj = 0;
    g_gui.cursor_y = 0.0f;
    if (id < 0)
        return;

    g = g_game;
    vector4 pos = *(vector4*)&g->parties.get_w(id)->pos;

    unsigned color;
    if (g_module_settings.show_faction_color) {
        color = g->factions[g->parties.get_w(id)->faction].color;
    } else {
        float rel = g->relation(id, g->f78);
        color = g_game->color_of(rel, 1);
        g = g_game;
    }

    Party* rec3 = g->parties.get_w(id);
    const tstring* name;
    if (g_module_settings.show_party_ids_instead_of_names)
        name = &rec3->key;
    else if (rec3->flag == 0 && (name = g_strtable.find(&rec3->key)) != 0)
        ;
    else
        name = &rec3->name;
    g_gui.add_label(name, 0.021f, (int)color);

    for (int i = 0; i < g_game->n_labels; i++)
        g_gui.add_label(&g_game->labels[i], 0.015f, (int)(((g_game->cur_label != i) - 1 & 0xffff0001) - 1));

    if (show) {
        float pt[2];
        ((MapWidget*)view)->project((vector4*)pt, &pos);
        g_gui.set_pos(pt);
    }
}

// ---- 0x51a690 -------------------------------------------------------------------------------------------
// @ 0x51a690
vector4* MapWidget::project(vector4* out, const vector4* in)
{
    vector2 xy;
    vector4 p = scene3d.fr.m.transform_point_to_local(*in);
    if (scene3d.fr.perspective) {
        float invz = 1.0f / p.z;
        float invz2 = invz;
        xy.x = -((invz * scene3d.fr.nearz) * p.x);
        xy.y = -((invz2 * scene3d.fr.nearz) * p.y);
    } else {
        xy = *(vector2*)&p;
    }
    xy.x = ((xy.x - scene3d.fr.left) * screen[0]) / (scene3d.fr.right - scene3d.fr.left);
    xy.y = ((xy.y - scene3d.fr.bottom) * screen[1]) / (scene3d.fr.top - scene3d.fr.bottom);
    xy.x += fE0;
    xy.y += fE4;
    *(vector2*)out = xy;
    return out;
}

// ---- 0x51a760 -------------------------------------------------------------------------------------------
// @ 0x51a760
void gui_manager::set_pos(const float* p)
{
    box_x = p[0];
    box_y = p[1];
    if (box_x > 0.95f)
        box_x = 0.95f;
    if (box_x < 0.05f)
        box_x = 0.05f;
    if (label_obj == 0)
        return;
    vector4 mn, mx;
    label_obj->calc_bounds(&mn, &mx);
    if (mn.x + box_x < 0.011f)
        box_x = 0.011f - mn.x;
    else if (mx.x + box_x > 0.989f)
        box_x = 0.989f - mx.x;
    if (mn.y + box_y < 0.011f)
        box_y = 0.011f - mn.y;
    else if (mx.y + box_y > 0.739f)
        box_y = 0.739f - mx.y;
}

// ---- 0x51a880 -------------------------------------------------------------------------------------------
// @ 0x51a880
void gui_manager::add_label(const tstring* text, float h, int color)
{
    if (label_obj == 0) {
        mesh* m = new mesh;
        if (label_obj != 0)
            delete label_obj;
        label_obj = m;
        cursor_y = 0.0f;
    }
    {
        tstring name("ui_font");
        label_obj->res = g_resmgr.get_material(name);
    }
    {
        float out[2];
        float xmax = 1.0f;
        float xmin = -1.0f;
        int col = color;
        float scale[2];
        scale[0] = h;
        scale[1] = h;
        vector4 pos(0.0f, cursor_y, 0.0f);
        g_font->layout_text(out, text, &pos, scale, label_obj, (int)&col, &xmin, &xmax, 0x10);
    }
    cursor_y -= h;
    {
        vector4 mn, mx;
        label_obj->calc_bounds(&mn, &mx);
        float corner[2];
        float size[2];
        vector4 e = mx - mn;
        size[0] = e.x + 0.02f;
        size[1] = e.y + 0.02f;
        corner[0] = mn.x - 0.01f;
        corner[1] = mn.y - 0.01f;
        set_quad(corner, size);
    }
    {
        float p[2];
        p[0] = box_x;
        p[1] = box_y;
        set_pos(p);
    }
}

// ---- 0x51aa20 -------------------------------------------------------------------------------------------
static inline void set_point(rgl::point20& p, const vector4& v) { p.a = v; p.b = v; }

// @ 0x51aa20
void gui_manager::set_quad(const float* origin, const float* size)
{
    float sy = origin[1] + size[1];
    float sx = origin[0] + size[0];
    m570.ensure_local();
    set_point(m570.v70[0], vector4(origin[0], origin[1], 0.0f));
    set_point(m570.v70[1], vector4(sx, origin[1], 0.0f));
    set_point(m570.v70[2], vector4(sx, sy, 0.0f));
    set_point(m570.v70[3], vector4(origin[0], sy, 0.0f));
    m570.slots[0].f28 = 1;
    {
        tstring name("matte");
        m570.res = g_resmgr.get_material(name);
    }
}

// a record's display string: the key's string-table entry when the key is a token (0x51afb6, 0x51b2cb)
static tstring* record_name(Party* r)
{
    if (g_module_settings.show_party_ids_instead_of_names) return &r->key;
    if (r->flag == 0) {
        tstring* s = g_strtable.find(&r->key);
        if (s) return s;
    }
    return &r->name;
}
static const char* record_name_c(Party* r) { return record_name(r)->c_str(); }

// a troop's display string (no key-mode check, see 0x51bc05)
static const char* troop_name_c(Troop* t)
{
    tstring* s = g_strtable.find(&t->key);
    return (s ? s : &t->name)->c_str();
}

// index of the first prisoner/enemy stack, rescanned on every iteration (0x51bb56)
static int first_enemy_stack(Party* r)
{
    int i = 0;
    while (i < r->count && !(r->stacks[i].flags & 1)) i++;
    return i;
}

// party lookup with the id by const reference: a by-value id takes one more scratch pick and rotates what follows (0x51b87e, 0x51b930)
static inline Party* pget(PartyMgr* m, const int& id) { int idx = m->lookup(id); return (Party*)((char*)m->pages._Myfirst[idx >> 4] + (idx & 0xf) * 0x5ac8); }
struct FleetRec { rgl::vector4 pos; char pad10[0x5c - 0x10]; };   // the 0x5c-byte fleet / battle record: only its position is read here

// -----------------------------------------------------------------------------------------------------
// @ 0x51abd0
void MapPanel::update(int type, int id)
{
    tstring label;                           // [esp+0x2c]  the accumulated order text
    tstring scratch;                         // [esp+0x170] constructed and destroyed, otherwise unused
    rgl::vector4 pos(0.0f, 0.0f, 0.0f);      // [esp+0x40]  world anchor of the selection (w stays unset)

    // ---- anchor the panel -------------------------------------------------------------------------
    if (type == 1) {
        pos = *(rgl::vector4*)&g_game->parties.get_w(id)->pos;
    } else if (type == 3) {
        FleetRec& fr = ((FleetRec*)g_game->fleet)[id];
        pos = fr.pos;
    } else if (type == 4) {
        pos = *(rgl::vector4*)&g_game->parties.get(id)->pos;
    }

    // ---- the same selection is still on screen and the cooldown has not elapsed -------------------
    if (type == g_sel_type && id == g_sel_id && timer < 1.0f) return;

    timer = 0.0f;
    g_gui.set_overlay(0, true);

    if (type != 0) {
        if (type == 1) {
            Party roster;                     // [esp+0xae0] the selected party's own roster (0x4934d0)
            int ncols = 1;                    // [esp+0x28]

            if (g_game->parties.get_party(id)->battle >= 0) {
                g_gui.draw_string(g_strtable.lookup("ui_capital_battle"), 0.02f, -1, 1);
                ncols = 2;
            }

            for (int c = 0; c < ncols; c++) {                       // 0x51adc0
                int agent = id;               // [esp+0x14]  battle side shown in the current column
                char buf[0x100];
                if (ncols > 1) {
                    GameData* g = g_game;
                    int b = g->parties.get_party(agent)->battle;
                    Rec98* slot = (Rec98*)g->agent_pages[(unsigned)b >> 4] + (b & 0xf);
                    if (c == 0) agent = slot->f58;
                    else agent = slot->f54;
                }
                if (c > 0) {
                    g_gui.draw_string(" ", 0.01f, -1, 1);
                    g_gui.draw_string(g_strtable.lookup("ui_capital_versus"), 0.024f, -1, 1);
                }

                PartyMgr& mg = g_game->parties;
                GameData* g = g_game;
                Party* rec = mg.get_party(agent);        // [esp+0x60]  record being painted
                float    rel   = g->relation(agent, g->f78);
                unsigned rel_color = g->color_of(rel, 1);
                unsigned color;
                if (g_module_settings.show_faction_color)
                    color = g_game->factions[mg.get_party(agent)->faction].color;
                else
                    color = rel_color;
                if (c == 0) {
                    g_gui.panel_color = rel_color;          // 0x51af67: the relation colour (edi), not the faction override (esi)
                    if (rel < -0.001f) g_gui.panel_bg = 0xff200000u;
                    else if (rel > 0.001f) g_gui.panel_bg = 0xff002000u;
                    else g_gui.panel_bg = 0xff101010u;
                }

                // ---- the party's own name (or its list of sub-parties) -----------------------------
                if (rec->f6c == 0) {
                    _snprintf(buf, 0x100, "%s", record_name_c(rec));             // 0x535717
                    g_gui.draw_string(buf, 0.021f, color, 1);
                } else {
                    int members[128];                                        // [esp+0x8e0]
                    int count = 0;                                           // [esp+0x18]
                    g_game->list_members(agent, members, &count);                 // 0x490fa0
                    for (int i = 0; i < count; i++) {
                        GameData* g = g_game;
                        int      m    = members[i];
                        float mrel = g->relation(m, g->f78);
                        unsigned mcol = g->color_of(mrel, 1);
                        if (g_module_settings.show_faction_color)
                            mcol = g->factions[g->parties.get_party(m)->faction].color;
                        if (g->parties.get_party(m)->flags & 0x40000)
                            mcol = mrel < -0.01f ? 0xffaa22ddu : 0xff22aaffu;

                        int extra, wounded, size;
                        g->party_counts(m, &size, &wounded, &extra);    // 0x51be60: arg2 = troops, arg3 = wounded, arg4 = prisoners (the "+ N")

                        PartyMgr& pm = g_game->parties;
                        if (pm.get_party(m)->flags & 0x200000) {
                            _snprintf(buf, 0x100, "%s", record_name_c(pm.get_party(m)));
                        } else if (wounded != 0) {
                            if (extra != 0)
                                _snprintf(buf, 0x100, "%s (%d/%d + %d)", record_name_c(pm.get_party(m)), size - wounded, size, extra);
                            else
                                _snprintf(buf, 0x100, "%s (%d/%d)", record_name_c(pm.get_party(m)), size - wounded, size);
                        } else {
                            if (extra != 0)
                                _snprintf(buf, 0x100, "%s (%d + %d)", record_name_c(pm.get_party(m)), size, extra);
                            else
                                _snprintf(buf, 0x100, "%s (%d)", record_name_c(pm.get_party(m)), size);
                        }
                        g_gui.draw_string(buf, 0.021f, mcol, 1);
                    }

                    roster.count = 0;
                    g_game->fill_parties(agent, &roster);                    // 0x48fd60
                    roster.prepare();                                        // 0x4b1eb0
                }

                // ---- faction tag -------------------------------------------------------------------
                GameData* gd;                   // esi: re-read after each call that can kill it; it stays live into the party lookup below
                if (rec->flags & 0x400000) {
                    int fac = rec->faction;
                    if (fac >= 0) {
                        gd = g_game;
                        if (gd->factions[fac].f24 != 0) goto have_gd;
                        FactionRec* f = gd->factions;
                        f += fac;
                        if (f->f10 == 0) {
                            tstring* s = g_strtable.find(&f->key);
                            if (s) f = (FactionRec*)s;
                        }
                        char buf[0x200];
                        mb_snprintf(buf, 0x200, "(%s)", f->name.c_str());        // 0x415100
                        unsigned fcol;
                        if (g_module_settings.show_faction_color) {
                            fcol = g_game->factions[fac].color;
                        } else {
                            fcol = 0xffffffffu;
                            const float& r = g_game->factions[fac].rel[0];
                            if (r < -0.001f) fcol = 0xffff4422u;
                            else if (r > 0.001f) fcol = 0xff44ff22u;
                        }
                        g_gui.draw_string(buf, 0.022f, fcol, 1);     // 0x3cb43958
                    }
                }
                gd = g_game;
            have_gd:
                if (rec->extra_text.len > 2) {
                    g_gui.draw_string(rec->extra_text, 0.023f, 0xffffffff, 1);
                    gd = g_game;
                }

                // ---- the order line (single-column display only) -----------------------------------
                if (ncols == 1) {
                    int action = rec->action;
                    if (action == 0) {
                        label = *g_strtable.lookup("ui_waiting");
                    } else if (action != 4 && action != 1) {
                        if (action == 10) {
                        label = *g_strtable.lookup("ui_accompanying");
                        label.append(*g_game->parties.get(rec->target)->display_name());
                    } else if (action == 5) {
                        label = *g_strtable.lookup("ui_running_from");
                        label.append(*g_game->parties.get(rec->target)->display_name());
                    } else if (action == 2) {
                        label = *g_strtable.lookup("ui_patrolling");
                    } else if (action == 3) {
                        label = *g_strtable.lookup("ui_patrolling_around");
                        label.append(*g_game->parties.get(rec->target)->display_name());
                    } else if (action == 6) {
                        if (dist_is_zero((DistObj*)rec))
                            label = *g_strtable.lookup("ui_holding");
                        else
                            label = *g_strtable.lookup("ui_travelling");
                    } else if (action == 7) {
                        int a = rec->battle;
                        if (a >= 0) {
                            Rec98* slot = (Rec98*)((BattleTable*)((char*)gd + 0xcc))->get(a);         // 0x48ef20
                            int other;
                            if (slot->f54 == agent) other = slot->f58;
                            else other = slot->f54;
                            if (other >= 0) {
                                label = *g_strtable.lookup("ui_fighting_against");
                                label.append(*g_game->parties.get(other)->display_name());
                            }
                        }
                        }
                    } else {
                        if (rec->action == 1 || agent == gd->f78
                            // gd becomes the manager here: one variable across the lookup keeps it in esi (add esi,0xf8), which sets the scratch picks of the whole order chain
                            || (pget((PartyMgr*)(gd = (GameData*)&gd->parties), rec->target)->flags & 0x400))
                            label = *g_strtable.lookup("ui_travelling_to");
                        else
                            label = *g_strtable.lookup("ui_following");
                        label.append(*record_name(pget(&g_game->parties, rec->target)));
                    }

                    if (!(rec->flags & 0x400)) {
                        g_gui.draw_string(label, 0.021f, 0xffc0b0a0, 1);
                        if (!dist_is_zero((DistObj*)rec)) {
                            char buf[0x200];
                            sprintf(buf, g_strtable.lookup("ui_speed_equals")->c_str(), (double)rec->speed_factor());
                            g_gui.draw_string(buf, 0.021f, 0xffb0a090, 1);
                        }
                    }
                }

                // ---- garrison contents -------------------------------------------------------------
                unsigned tcol = 0xffd0b0e0;
                if (!(rec->flags & 0x200000) && rec->f6c != 0) {              // 0x51ba69
                    if (rec->flags & 0x400)
                        g_gui.draw_string(g_strtable.lookup("ui_defenders"), 0.02f, 0xffc08020, 1);
                    else
                        g_gui.draw_string(g_strtable.lookup("ui_troops"), 0.02f, 0xffc08020, 1);
                    for (int i = 0; i < roster.count; i++) {
                        if (i == first_enemy_stack(&roster)) {                // 0x51bb56
                            g_gui.draw_string(g_strtable.lookup("ui_prisoners"), 0.02f, 0xffc08020, 1);
                            tcol = 0xffc0a060;
                        }
                        Stack& s = roster.stacks[i];
                        Troop* t = g_game->troops + s.troop;
                        char   buf[0x200];
                        if (t->flags30 & 0x10) {
                            int base = t->f58[0];
                            int tier = t->nib_b[0x7a - 0x68] & 0xf;
                            if (tier < 0)                tier = 0;
                            else if (tier > (*(int*)&g_skill_tbl[36].max_rank)) tier = (*(int*)&g_skill_tbl[36].max_rank);
                            const char* name = troop_name_c(t);
                            sprintf(buf, "%s (%d%%)", name, (int)(t->hp * 100.0f / (base + tier * 2 + 0x23) + 1e-4f));
                            g_gui.draw_string(buf, 0.015f, tcol | 0x0f0f0f, 0);
                        } else {
                            if (s.wounded == 0)
                                sprintf(buf, "%s (%d)", troop_name_c(t), s.count);
                            else
                                sprintf(buf, "%s (%d/%d)", troop_name_c(t), s.count - s.wounded, s.count);
                            g_gui.draw_string(buf, 0.015f, tcol, 0);
                        }
                    }
                }
            }                                    // for (c ...)
        } else if (type == 3) {
            draw_party_line(id);
        }

        // ---- move the panel to the selection's screen position --------------------------------------
        {
            float out[2];                  // eight bytes: it shares [esp+0x60] with rec, the garrison index and the capital-battle temporary
            rgl::vector4* o = ((MapWidget*)view)->project((rgl::vector4*)out, &pos);
            g_gui.set_view((const float*)&vector2(o->x, o->y));      // 0x4c5190: a by-reference temporary, stored after the argument's lea (0x51bdda)
        }
    }

    g_sel_type = type;
    g_sel_id   = id;
}
