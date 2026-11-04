// Slice s005191e0: the map panel's per-frame target picker / order issuer.
//   0x5191e0  MapPanel::update_target   (hover the party-label column, cast a screen ray, pick a party
//                                       agent or a ground item under the cursor, and turn the resulting
//                                       selection into a move/attack order)
// Names and field names are provisional; every offset was read off the disassembly.  Layouts, callee
// signatures and calling conventions are reused from the neighbouring slices s0051a2d0 (set_label,
// PlaneTrace::intersect, Picker::collect), s0051abd0 (MapPanel / GameData / RecR), s00439100 (collision
// shapes, level::query_segment) and s00483870 (the message ring).
#include <windows.h>
#include <math.h>
#include "../../rgl/rgl_math.h"
#include "../strat/gui_manager.h"
#include "../shared_vars.h"
#include "../key_names.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;                 // static-CRT float marker

// ---------------------------------------------------------------------------------- small engine types --
#include "../tstring.h"

#include "../str_table.h"
#include "../world.h"                          // world (= Level): the scene object g_level points to

// --------------------------------------------------------------------------------- collision shapes ----
// shape_base: 0x20 bytes; slot 0 dtor, slot 1 shape_type (the picker's `v1() != 2` mask key).
// Layout and engine names are the ones verified in s00439100.h.
#include "../../include/shape.h"
#include "../../include/mat4.h"


// --------------------------------------------------------------------------------- map / game records --
struct shape_base;

struct AgentRec;                             // 0x98    agent record

#include "../../include/game_data.h"


struct AgentRec {                            // 0x98
    char   pad00[0x10];
    int    kind;                             // +0x10
    char   pad14[0x54 - 0x14];
    int    party;                            // +0x54
    char   pad58[0x98 - 0x58];
};

struct MapSceneObj {                         // a scene object returned by Picker::collect(9, ...) (not the 0x98 Rec98 record)
    char       pad00[0x0c];
    int        party;                        // +0x0c
    char       pad10[0x88 - 0x10];
    vector4    pos;                          // +0x88
    shape_base* shape;                       // +0x98
    char       pad9c[0x140 - 0x9c];
    char       active;                       // +0x140
    char       pad141[0x168 - 0x141];
    int        kind;                         // +0x168
};

struct BigRec {                              // 0x88  world-map cell record
    char     pad0[0x24];
    unsigned mask;                           // +0x24
    char     pad28[0x88 - 0x28];
};

struct LevelNav {                            // a view of NavData (terrain_b/cell_grid.h; that header cannot be included next to match/include/rgl.h): recs = cells (+0x10), zmin/zmax = e44.z / f54.z
    char     pad00[0x14];
    BigRec*  recs;                           // +0x14
    char     pad18[0x4c - 0x18];
    float    zmin;                           // +0x4c
    char     pad50[0x5c - 0x50];
    float    zmax;                           // +0x5c
    // shares (0x48efa0) is RiderTable::shares (game/mission4): called through a cast below
};


struct GroundItem {                          // 0x5c  fleet entry
    vector4  pos;                            // +0x00
    char     pad10[0x18 - 0x10];
    unsigned flags;                          // +0x18
    char     pad1c[0x5c - 0x1c];
};


// the party-id hash table at the head of PartyMgr (game/ui/mission.h's Map10007; PartyMgr::index_of is the same function, 0x48ad00)
struct Map10007 {
    int lookup(unsigned id);   // 0x48ad00
};
struct PlaneTrace {
    void intersect(vector4* out, const vector4* a, const vector4* b);   // 0x51a2d0 (inventory_panel/map_picker.cpp)
};
struct level {
    void query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* n);   // 0x439870 (game/agent2/agent_stats.h)
};
struct camera_ray {
    void screen_to_world(vector4* near_pt, vector4* far_pt);   // 0x4bd2f0 (game/config/config.h)
};
struct MapPickScreen {
    void set_label(int id);   // 0x51a420 (inventory_panel/map_picker.cpp): the map panel's label owner
};
struct RiderTable {
    int shares(const int& from, const int& to) const;   // 0x48efa0 (game/mission4)
};

static inline Party* party_at(GameData* g, int id)
{
    int i = ((Map10007*)&g->parties)->lookup(id);
    return g->parties.pages[i >> 4] + (i & 0xf);
}

// --------------------------------------------------------------------------------- view / panel -------

#include "../../rgl/mesh.h"                                           // rgl::mesh (calc_bounds 0x438d80)

#include "map_panel.h"                      // MapPanel / MapView (shared with map_info.cpp)
#include "../../include/level_global.h"      // g_level (0xa732b4)
#include "../ui_globals.h"      // UI / core globals

// --------------------------------------------------------------------------------- globals -------------

#include "../config/config_block.h"                // g_cfg (0x5a9f98); cheat_mode is +0x24 = 0x5a9fbc

// ======================================================================================================
// @ 0x5191e0
void MapPanel::update_target()
{
    // ---- 0x5191ee: party-label column hover ---------------------------------------------------------
    if (f2d5) {
        vector4 mn, mx;
        g_gui.label_obj->calc_bounds(&mn, &mx);
        // The original loads both box corners of an axis back to back (x,x,y,y); spelling the extents from the
        // expanded corners keeps that load order, which the resolution audit pairs by instruction shape.
        float w  = (g_gui.box_x + mx.x) - (g_gui.box_x + mn.x);
        float h  = (g_gui.box_y + mx.y) - (g_gui.box_y + mn.y);
        float x0 = g_gui.box_x + mn.x;
        float y0 = g_gui.box_y + mn.y;
        float dx = g_gui.mouse[0] - x0;
        float dy = g_gui.mouse[1] - y0;
        g_game->cur_label = -1;

        // The original tests the band with `test ah,1; jne` (C0 = less-than *or* unordered), so a NaN
        // coordinate counts as OUT of the band and the label is dropped; `!(x < lo)` keeps a NaN in.
        if (dx >= -0.01f && w + 0.01f >= dx &&
            dy >= -0.01f && h + 0.01f >= dy) {
            if (dy >= 0.0f) {
                int n = g_game->n_labels;
                // the original handles the hit inside the scan loop (0x519347), so the loop is not unrolled
                for (int i = 0; i < n; i++)
                    if ((float)(i + 1) * 0.015f > dy) {
                    g_game->cur_label = n - i - 1;
                    if (view->is_key_down(8)) {
                        ((MapPickScreen*)this)->set_label(-1);
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
        } else if (dx < -0.2f || dx > w + 0.2f ||
                   dy < -0.2f || dy > h + 0.2f || view->is_key_down(8)) {
            ((MapPickScreen*)this)->set_label(-1);
            f2d5 = 0;
        }
        if (f2d5) ((MapPickScreen*)this)->set_label(f2d8);
    }

    // ---- 0x519496: screen ray ----------------------------------------------------------------------
    vector4 nearp, farp;                                  // nearp -> S+0x40, farp -> S+0x1c
    ((camera_ray*)view)->screen_to_world(&nearp, &farp);   // 0x4bd2f0

    vector4 dir;
    dir.x = farp.x - nearp.x;
    dir.y = farp.y - nearp.y;
    dir.z = farp.z - nearp.z;
    dir.normalize();

    vector4 np = nearp;
    vector4 fp = farp;

    // ---- 0x519517: clip against the world's z range ------------------------------------------------
    if (((::world*)g_script.level)->wmap) {
        float zmin = ((::world*)g_script.level)->wmap->zmin;
        float zmax = ((::world*)g_script.level)->wmap->zmax;
        if (np.z < zmin) {
            float t = fabsf((zmin - np.z) / dir.z);
            np.x += dir.x * t; np.y += dir.y * t; np.z += dir.z * t;
        }
        if (fp.z < zmin) {
            float t = fabsf((zmin - fp.z) / dir.z);
            fp.x -= dir.x * t; fp.y -= dir.y * t; fp.z -= dir.z * t;
        }
        if (np.z > zmax) {
            float t = fabsf((np.z - zmax) / dir.z);
            np.x += dir.x * t; np.y += dir.y * t; np.z += dir.z * t;
        }
        if (fp.z > zmax) {
            float t = fabsf((fp.z - zmax) / dir.z);
            fp.x -= dir.x * t; fp.y -= dir.y * t; fp.z -= dir.z * t;
        }
    }

    // ---- 0x51969b: thin capsule around the clipped segment -----------------------------------------
    capsule cap(0.0f, &np, &fp);
    matrix mtx;
    cap.transform((const pos*)identity_pos(&mtx));

    // The sweep endpoints are built component-wise and only x/y of each are ever written; the third
    // lane is whatever the stack slot already held (reproduced verbatim -- see the notes).
    float seg_a[3];
    seg_a[0] = np.x;
    seg_a[1] = np.y;
    float seg_b[3];
    seg_b[0] = fp.x;
    seg_b[1] = fp.y;

    // ---- 0x519720: shapes under the cursor ---------------------------------------------------------
    float     bestd = 1000000.0f;
    int       best  = -1;
    shape_base* hits[0x2000];                  // the sweep is told 0x110, but the slot is 0x2000 wide
    int       count;
    ((level*)g_script.level)->query_segment(seg_a, seg_b, 0x110, hits, &count);   // 0x439870

    vector4 bestpt;
    for (int i = 0; i < count; i++) {
        shape_base* o = hits[i];
        if (o->shape_type() != 2) continue;
        if (!shape_contact_count(o, &cap, 0, 0)) continue;
        vector4 hp;
        ((PlaneTrace*)o)->intersect(&hp, &np, &fp);   // 0x51a2d0 (the face's intersect is the same code)
        float ddx = hp.x - nearp.x;
        float ddy = hp.y - nearp.y;
        float ddz = hp.z - nearp.z;
        float d = sqrtf(ddz * ddz + ddy * ddy + ddx * ddx);   // the original sums z*z + y*y first, then x*x
        if (!(d < bestd)) continue;
        bestd  = d;
        bestpt = hp;
        best   = i;
    }

    int rec_index = -1;
    int rec_mask  = -1;
    if (best >= 0) {
        rec_index = hits[best]->m4;
        rec_mask  = (int)((::world*)g_script.level)->wmap->recs[rec_index].mask;
    }

    // ---- 0x519829: party agents under the cursor ---------------------------------------------------
    shape_base* bestobj = 0;
    shape_base* agents[0x2000];                // Picker::collect does not bound *count, so the size matters
    int       acount;
    ((::world*)g_script.level)->collect(9, agents, &acount);
    bestd += 1.0f;

    for (int i = 0; i < acount; i++) {
        MapSceneObj* o = (MapSceneObj*)agents[i];
        if (!o->active) continue;
        if (!o->shape) continue;
        if (!shape_contact_count(o->shape, &cap, 0, 0)) continue;
        float ddx = nearp.x - o->pos.x;
        float ddy = nearp.y - o->pos.y;
        float ddz = nearp.z - o->pos.z;
        float d = sqrtf(ddz * ddz + ddy * ddy + ddx * ddx);   // the original sums z*z + y*y first, then x*x
        if (o->kind == 1) {
            d -= 0.4f;
            if (o->party == g_game->f78) d += 0.35f;
        }
        if (!(d < bestd)) continue;
        if (o->kind == 1 && (party_at(g_game, o->party)->flags_lo & 0x80100)) continue;
        bestd   = d;
        bestobj = (shape_base*)o;
    }

    // ---- 0x519997: commit the pick -----------------------------------------------------------------
    if (bestobj) {
        if (g_keyboard.down[0xe1] == 0 && g_keyboard.prev[0xe1] != 0 && f2d4 == 0) {
            g_game->n_labels  = 0;
            g_game->cur_label = -1;
            f2d8 = ((MapSceneObj*)bestobj)->party;
            g_game->emit_event(0x15, f2d8, 0);
            if (g_game->n_labels > 0) {
                f2d5 = 1;
                ((MapPickScreen*)this)->set_label(f2d8);
            }
        }
        int kind = ((MapSceneObj*)bestobj)->kind;
        if (kind == 1) {
            int id = ((MapSceneObj*)bestobj)->party;
            if (!(party_at(g_game, id)->flags_lo & 0x80000))
                update(1, id);
        } else if (kind == 2) {
            update(2, ((MapSceneObj*)bestobj)->party);
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
            int kind = ((MapSceneObj*)bestobj)->kind;
            if (kind == 1) {
                if (((MapSceneObj*)bestobj)->party != g_game->f78) {
                    // 0x519c9e..0x519cd5: both arms are the same store (the `0x400` flags test survives
                    // only as a dead select); reproduced verbatim.
                    if (party_at(g_game, ((MapSceneObj*)bestobj)->party)->flags_lo & 0x400)
                        party_at(g_game, g_game->f78)->action = 4;
                    else
                        party_at(g_game, g_game->f78)->action = 4;
                    party_at(g_game, g_game->f78)->target = ((MapSceneObj*)bestobj)->party;
                    party_at(g_game, g_game->f78)->reissue_order();
                }
            } else if (kind == 3) {
                int id = ((MapSceneObj*)bestobj)->party;
                AgentRec* rec = ((AgentRec**)g_game->agent_pages)[(unsigned)id >> 4] + (id & 0xf)   /* 0x519d57: shr */;
                if (rec->kind == 1) {
                    Party* pl = party_at(g_game, g_game->f78);
                    pl->action = 4;
                    pl->target = rec->party;
                    pl->reissue_order();
                } else if (rec->kind == 2) {
                    Party* pl = party_at(g_game, g_game->f78);
                    pl->action = 9;
                    pl->target = id;
                    pl->reissue_order();
                }
            }
        } else if (best >= 0) {
            bool go = true;
            if (g_keyboard.down[0x1d] && g_cfg.cheat_mode) {
                *(vector4*)&party_at(g_game, g_game->f78)->pos = bestpt;
            } else if (!((RiderTable*)((::world*)g_script.level)->wmap)->shares(rec_index,
                                               *(const int*)((char*)party_at(g_game, g_game->f78)->mesh + 4))) {
                go = false;        // 0x519f15: jumps to 0x519fe2 (the target-lost check below), not out of the function
            }
            if (go) {
                party_at(g_game, g_game->f78)->action = 6;
                {
                    Party* pl = party_at(g_game, g_game->f78);
                    pl->order_pos.x = bestpt.x;
                    pl->order_pos.y = bestpt.y;
                }
                party_at(g_game, g_game->f78)->reissue_order();
            }
        }
    }

    // ---- 0x519fe8: drop the order if the target vanished -------------------------------------------
    if (party_at(g_game, g_game->f78)->action != 4) return;
    if (party_at(g_game, g_game->f78)->target < 0) return;

    // Each access re-runs the party lookup (0x48ad00), exactly as the original does: alpha test 0x51a099, then
    // f8c8 0x51a0f3 (fresh lookup), flags reuse that pointer; the body re-looks-up the player (target), the
    // target (position) and the player (store).
    bool lost = party_at(g_game, party_at(g_game, g_game->f78)->target)->alpha < 0.01f;
    if (!lost) {
        Party* tgt = party_at(g_game, party_at(g_game, g_game->f78)->target);
        lost = tgt->f8c8 >= 0 || (tgt->flags_lo & 0x1080100) != 0;
    }
    if (lost) {
        party_at(g_game, g_game->f78)->action = 6;
        int tid = party_at(g_game, g_game->f78)->target;
        Party* src = party_at(g_game, tid);
        Party* dst = party_at(g_game, g_game->f78);
        dst->order_pos.x = src->pos.x;
        dst->order_pos.y = src->pos.y;
        party_at(g_game, g_game->f78)->reissue_order();

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
    }

    party_at(g_game, g_game->f78)->reissue_order();
}
