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
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;                 // static-CRT float marker

// ---------------------------------------------------------------------------------- small engine types --
struct tstring {                             // 16 bytes (see s004bd170)
    int   pos;                               // +0x00
    char* buf;                               // +0x04
    int   cap;                               // +0x08
    int   len;                               // +0x0c
    tstring(const char* s);                  // 0x42bbe0
    ~tstring();                              // 0x42bc40
    void operator=(const tstring& o);        // 0x42bc50
};

struct StrTable {
    tstring* lookup(const tstring& key);     // 0x43a1e0
};
extern StrTable g_strtable;                  // 0x5e2a18

// --------------------------------------------------------------------------------- collision shapes ----
// shape_base: 0x20 bytes; slot 0 dtor, slot 1 shape_type (the picker's `v1() != 2` mask key).
// Layout and engine names are the ones verified in s00439100.h.
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base() : m4(-1), m8(8), mc(-1),
                   min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base() {}                                                  // slot 0   0x40a540 (??_G)
    virtual int shape_type();                                                 // slot 1   0x408020
    void intersect(vector4* out, const vector4* a, const vector4* b);         // 0x51a2d0
};

struct sphere_shape : shape_base {           // 0x44, vtable 0x54a4f0 (the vptr stored into the local one)
    vector4 v20;                             // +0x20
    vector4 center;                          // +0x30
    float   radius;                          // +0x40  (raw float store in the original)
};

struct capsule : shape_base {                // 0x64, vtable 0x54a540
    char    geom[0x44];                      // +0x20  filled in by the ctor
    capsule(float r, const vector4* a, const vector4* b);   // 0x433310
    void transform(const matrix* m);                        // 0x408770
};

int  shape_contact_count(shape_base* a, shape_base* b, int max, void* out);  // 0x40bfe0
matrix* identity(matrix* out);                                               // 0x40cd00

// --------------------------------------------------------------------------------- map / game records --
struct shape_base;

struct Party;                                // 0x5ac8  party record (see s0051abd0)
struct AgentRec;                             // 0x98    agent record

struct PartyMgr {                            // embedded in GameData at +0xf8
    char     pad0[0x138d0];
    Party**  pages;                          // +0x138d0
    int      index_of(int id);               // 0x48ad00
    Party*   get(int id);                    // 0x51ee50
};

struct Party {                               // 0x5ac8
    char      pad00[0x30];
    vector4   pos;                           // +0x30
    char      pad40[0x68 - 0x40];
    float     f68;                           // +0x68
    char      pad6c[0xa0 - 0x6c];
    unsigned  flags;                         // +0xa0
    char      pada4[0x8c8 - 0xa4];
    int       f8c8;                          // +0x8c8
    char      pad8cc[0x918 - 0x8cc];
    int       state;                         // +0x918
    int       target;                        // +0x91c
    char      pad920[0x924 - 0x920];
    vector4   order_pos;                     // +0x924
    char      pad934[0x5974 - 0x934];
    char*     f5974;                         // +0x5974  (pointer to a sub-record; +4 is its group)
    char      pad5978[0x5ac8 - 0x5978];
    void      reissue_order();               // 0x4b3bc0
};

struct AgentRec {                            // 0x98
    char   pad00[0x10];
    int    kind;                             // +0x10
    char   pad14[0x54 - 0x14];
    int    party;                            // +0x54
    char   pad58[0x98 - 0x58];
};

struct MapAgent {                            // a scene object returned by Picker::collect(9, ...)
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

struct WorldMap {
    char     pad00[0x14];
    BigRec*  recs;                           // +0x14
    char     pad18[0x4c - 0x18];
    float    zmin;                           // +0x4c
    char     pad50[0x5c - 0x50];
    float    zmax;                           // +0x5c
    bool     linked(const int* from, const int* to);   // 0x48efa0
};

struct Level {                               // the level/picker singleton at 0xa732b4
    char       pad00[0xb0];
    WorldMap*  world;                        // +0xb0
    void query_segment(const float* a, const float* b, unsigned mask,
                       shape_base** out, int* count);      // 0x439870
    void collect(unsigned key, shape_base** out, int* count);  // 0x51a3a0
};

struct GroundItem {                          // 0x5c  fleet entry
    vector4  pos;                            // +0x00
    char     pad10[0x18 - 0x10];
    unsigned flags;                          // +0x18
    char     pad1c[0x5c - 0x1c];
};

struct GameData {
    char      pad00[0x78];
    int       player_party;                  // +0x78
    char      pad7c[0x84 - 0x7c];
    int       f84;                           // +0x84
    char      f88;                           // +0x88
    char      pad89[0x95 - 0x89];
    char      f95;                           // +0x95
    char      pad96[0xe4 - 0x96];
    AgentRec** agent_pages;                  // +0xe4
    char      pad_e8[0xf8 - 0xe8];
    PartyMgr  parties;                       // +0xf8
    char      pad1[0x13a64 - 0x139cc];
    GroundItem* fleet;                       // +0x13a64
    GroundItem* fleet_end;                   // +0x13a68
    char      pad2[0x13bc4 - 0x13a6c];
    int       n_labels;                      // +0x13bc4
    int       cur_label;                     // +0x13bc8
    tstring   labels[50];                    // +0x13bcc
    int       label_target[64];              // +0x13eec
    void      event(int a, int b, int c);    // 0x435790
};

static inline Party* party_at(GameData* g, int id)
{
    int i = g->parties.index_of(id);
    return g->parties.pages[i >> 4] + (i & 0xf);
}

// --------------------------------------------------------------------------------- view / panel -------
struct MapView {                             // the object at MapPanel+0x30
    virtual void v00(); virtual void v08(); virtual void v10(); virtual void v18();
    virtual void v20(); virtual void v28(); virtual void v30(); virtual void v38();
    virtual void v40(); virtual void v48(); virtual void v50(); virtual void v58();
    virtual void v60(); virtual void v68(); virtual void v70(); virtual void v78();
    virtual void v80();
    virtual char is_key_down(int key);                              // +0x44
    void screen_to_world(vector4* near_pt, vector4* far_pt);        // 0x4bd2f0
};

struct Mesh {
    void calc_bounds(vector4* mn, vector4* mx);                     // 0x438d80
};

struct MapPanel {
    char      pad00[0x30];
    MapView*  view;                          // +0x30
    char      pad34[0x2d0 - 0x34];
    float     timer;                         // +0x2d0
    char      f2d4;                          // +0x2d4
    char      f2d5;                          // +0x2d5  label column is being hovered
    char      pad2d6[0x2d8 - 0x2d6];
    int       f2d8;                          // +0x2d8  currently picked party id
    void set_label(int id);                  // 0x51a420
    void update(int type, int id);           // 0x51abd0
    void update_target();                    // 0x5191e0
};

// --------------------------------------------------------------------------------- globals -------------
extern GameData* g_game;                     // 0x5e2930
extern Level*    g_level;                    // 0xa732b4
extern Mesh*     g_label_obj;                // 0x5d75ec
extern float     g_box_x;                    // 0x5d8778
extern float     g_box_y;                    // 0x5d877c
extern int       g_mouse_x;                  // 0x5d7094
extern int       g_mouse_y;                  // 0x5d7098
extern int       g_a17458;                   // 0xa17458
extern int       g_a17858;                   // 0xa17858
extern int       g_a17148;                   // 0xa17148
extern int       g_5a9fbc;                   // 0x5a9fbc
extern char      g_msg_enabled;              // 0x5d8794
extern int       g_msg_count;                // 0x5e2138
extern int       g_msg_base;                 // 0x5e213c
extern tstring   g_msg_text[32];             // 0x5e2140
extern int       g_msg_id[32];               // 0x5e2340

// ======================================================================================================
// @ 0x5191e0
void MapPanel::update_target()
{
    // ---- 0x5191ee: party-label column hover ---------------------------------------------------------
    if (f2d5) {
        vector4 mn, mx;
        g_label_obj->calc_bounds(&mn, &mx);
        // The original loads both box corners of an axis back to back (x,x,y,y); spelling the extents from the
        // expanded corners keeps that load order, which the resolution audit pairs by instruction shape.
        float w  = (g_box_x + mx.x) - (g_box_x + mn.x);
        float h  = (g_box_y + mx.y) - (g_box_y + mn.y);
        float x0 = g_box_x + mn.x;
        float y0 = g_box_y + mn.y;
        float dx = (float)g_mouse_x - x0;
        float dy = (float)g_mouse_y - y0;
        g_game->cur_label = -1;

        if (!(dx < -0.01f) && !(dx > w + 0.01f) &&
            !(dy < -0.01f) && !(dy > h + 0.01f)) {
            if (!(dy < 0.0f)) {
                int n = g_game->n_labels;
                int i;
                for (i = 0; i < n; i++)
                    if ((float)(i + 1) * 0.015f > dy) break;
                if (i < n) {
                    g_game->cur_label = n - i - 1;
                    if (view->is_key_down(8)) {
                        set_label(-1);
                        f2d5 = 0;
                        int sel = g_game->label_target[g_game->cur_label];
                        if (sel == -7) {
                            // "my own party" marker: retarget the player's party
                            if (f2d8 != g_game->player_party) {
                                g_game->f88 = 1;
                                g_game->f84 = g_game->player_party;
                                g_game->parties.get(g_game->player_party)->state  = 4;
                                g_game->parties.get(g_game->player_party)->target = f2d8;
                                g_game->parties.get(g_game->player_party)->reissue_order();
                            }
                        } else {
                            g_game->event(0x16, f2d8, sel);
                        }
                    }
                }
            }
        } else if (dx < -0.2f || dx > w + 0.2f ||
                   dy < -0.2f || dy > h + 0.2f || view->is_key_down(8)) {
            set_label(-1);
            f2d5 = 0;
        }
        if (f2d5) set_label(f2d8);
    }

    // ---- 0x519496: screen ray ----------------------------------------------------------------------
    vector4 nearp, farp;                                  // nearp -> S+0x40, farp -> S+0x1c
    view->screen_to_world(&nearp, &farp);

    vector4 dir;
    dir.x = farp.x - nearp.x;
    dir.y = farp.y - nearp.y;
    dir.z = farp.z - nearp.z;
    dir.normalize();

    vector4 np = nearp;
    vector4 fp = farp;

    // ---- 0x519517: clip against the world's z range ------------------------------------------------
    if (g_level->world) {
        float zmin = g_level->world->zmin;
        float zmax = g_level->world->zmax;
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
    cap.transform(identity(&mtx));

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
    g_level->query_segment(seg_a, seg_b, 0x110, hits, &count);

    vector4 bestpt;
    for (int i = 0; i < count; i++) {
        shape_base* o = hits[i];
        if (o->shape_type() != 2) continue;
        if (!shape_contact_count(o, &cap, 0, 0)) continue;
        vector4 hp;
        o->intersect(&hp, &np, &fp);
        float ddx = hp.x - nearp.x;
        float ddy = hp.y - nearp.y;
        float ddz = hp.z - nearp.z;
        float d = sqrtf(ddx * ddx + ddy * ddy + ddz * ddz);
        if (!(d < bestd)) continue;
        bestd  = d;
        bestpt = hp;
        best   = i;
    }

    int rec_index = -1;
    int rec_mask  = -1;
    if (best >= 0) {
        rec_index = hits[best]->m4;
        rec_mask  = (int)g_level->world->recs[rec_index].mask;
    }

    // ---- 0x519829: party agents under the cursor ---------------------------------------------------
    shape_base* bestobj = 0;
    shape_base* agents[0x2000];                // Picker::collect does not bound *count, so the size matters
    int       acount;
    g_level->collect(9, agents, &acount);
    bestd += 1.0f;

    for (int i = 0; i < acount; i++) {
        MapAgent* o = (MapAgent*)agents[i];
        if (!o->active) continue;
        if (!o->shape) continue;
        if (!shape_contact_count(o->shape, &cap, 0, 0)) continue;
        float ddx = nearp.x - o->pos.x;
        float ddy = nearp.y - o->pos.y;
        float ddz = nearp.z - o->pos.z;
        float d = sqrtf(ddx * ddx + ddy * ddy + ddz * ddz);
        if (o->kind == 1) {
            d -= 0.4f;
            if (o->party == g_game->player_party) d += 0.35f;
        }
        if (!(d < bestd)) continue;
        if (o->kind == 1 && (party_at(g_game, o->party)->flags & 0x80100)) continue;
        bestd   = d;
        bestobj = (shape_base*)o;
    }

    // ---- 0x519997: commit the pick -----------------------------------------------------------------
    if (bestobj) {
        if (g_a17458 == 0 && g_a17858 != 0 && f2d4 == 0) {
            g_game->n_labels  = 0;
            g_game->cur_label = -1;
            f2d8 = ((MapAgent*)bestobj)->party;
            g_game->event(0x15, f2d8, 0);
            if (g_game->n_labels > 0) {
                f2d5 = 1;
                set_label(f2d8);
            }
        }
        int kind = ((MapAgent*)bestobj)->kind;
        if (kind == 1) {
            int id = ((MapAgent*)bestobj)->party;
            if (!(party_at(g_game, id)->flags & 0x80000))
                update(1, id);
        } else if (kind == 2) {
            update(2, ((MapAgent*)bestobj)->party);
        }
    } else {
        // ---- 0x519a8b: ground items -----------------------------------------------------------------
        sphere_shape sh;
        int best_item = -1;
        for (int i = 0; g_game->fleet && i < g_game->fleet_end - g_game->fleet; i++) {
            GroundItem* it = g_game->fleet + i;
            if (!(g_game->f95 || (it->flags & 0x20000000))) continue;
            float ddx = it->pos.x - nearp.x;
            float ddy = it->pos.y - nearp.y;
            float ddz = it->pos.z - nearp.z;
            float d = sqrtf(ddx * ddx + ddy * ddy + ddz * ddz);
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

    // ---- 0x519c01: post-selection -----------------------------------------------------------------
    if (!f2d5 && view->is_key_down(8)) {
        g_game->f88 = 1;
        g_game->f84 = g_game->player_party;
    }

    if (bestobj) {
        int kind = ((MapAgent*)bestobj)->kind;
        if (kind == 1) {
            if (((MapAgent*)bestobj)->party != g_game->player_party) {
                // 0x519c9e..0x519cd5: both arms are the same store (the `0x400` flags test survives
                // only as a dead select); reproduced verbatim.
                if (party_at(g_game, ((MapAgent*)bestobj)->party)->flags & 0x400)
                    party_at(g_game, g_game->player_party)->state = 4;
                else
                    party_at(g_game, g_game->player_party)->state = 4;
                party_at(g_game, g_game->player_party)->target = ((MapAgent*)bestobj)->party;
                party_at(g_game, g_game->player_party)->reissue_order();
            }
        } else if (kind == 3) {
            int id = ((MapAgent*)bestobj)->party;
            AgentRec* rec = g_game->agent_pages[id >> 4] + (id & 0xf);
            if (rec->kind == 1) {
                Party* pl = party_at(g_game, g_game->player_party);
                pl->state  = 4;
                pl->target = rec->party;
                pl->reissue_order();
            } else if (rec->kind == 2) {
                Party* pl = party_at(g_game, g_game->player_party);
                pl->state  = 9;
                pl->target = id;
                pl->reissue_order();
            }
        }
    } else if (best >= 0) {
        if (g_a17148 && g_5a9fbc) {
            party_at(g_game, g_game->player_party)->pos = bestpt;
        } else if (!g_level->world->linked(&rec_index,
                                           (const int*)(party_at(g_game, g_game->player_party)->f5974 + 4))) {
            return;
        }
        party_at(g_game, g_game->player_party)->state = 6;
        {
            Party* pl = party_at(g_game, g_game->player_party);
            pl->order_pos.x = bestpt.x;
            pl->order_pos.y = bestpt.y;
        }
        party_at(g_game, g_game->player_party)->reissue_order();
    }

    // ---- 0x519fe8: drop the order if the target vanished -------------------------------------------
    if (party_at(g_game, g_game->player_party)->state != 4) return;
    if (party_at(g_game, g_game->player_party)->target < 0) return;

    bool lost;
    {
        Party* tgt = party_at(g_game, party_at(g_game, g_game->player_party)->target);
        // 0x51a099: f68 < 0.01 -> lost; 0x51a0fc: f8c8 >= 0 -> lost; 0x51a10f: (flags & 0x1080100)
        // nonzero -> lost.  All three tests jump *into* the body, i.e. the body runs unless
        // f68 >= 0.01 && f8c8 < 0 && the flags are clear.
        lost = tgt->f68 < 0.01f ||
               tgt->f8c8 >= 0 ||
               (tgt->flags & 0x1080100) != 0;
        if (lost) {
            Party* pl = party_at(g_game, g_game->player_party);
            pl->state       = 6;
            pl->order_pos.x = tgt->pos.x;
            pl->order_pos.y = tgt->pos.y;
            pl->reissue_order();

            {
                tstring tmp("ui_target_lost");
                tstring* s = g_strtable.lookup(tmp);
                if (g_msg_enabled && g_msg_count < 0x20) {
                    int k = (g_msg_base + g_msg_count) % 0x20;
                    g_msg_text[k] = *s;
                    g_msg_id[k]   = -1;
                    g_msg_count++;
                }
            }
        }
    }

    party_at(g_game, g_game->player_party)->reissue_order();
}
