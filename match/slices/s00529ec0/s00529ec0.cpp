// Slice s00529ec0: the mission screen's interaction probe, Screen::f_529ec0 (0x529ec0, 4011 bytes).
//
// Every frame the screen asks "what is the local agent looking at?":
//   1. cast a thin capsule (1e-4 m) from the agent's eye 10.5 m forward and query the level grid for
//      face shapes (shape_type()==2); the nearest hit gives a preliminary distance `best`;
//   2. re-run the probe as a 0.1 m capsule extended 0.1 m forward against the scene's *entities*
//      (mask 9) and keep the nearest entity within best+1.0 m: agents on foot (talk), ridden horses
//      (mount/dismount), scene props (chest / baggage / map-object / equipment) and the map edges
//      (leave area / retreat);
//   3. when nothing was hit, fall back to the mount/dismount test and to the four mission-border
//      tests, which use the agent's facing vector;
//   4. validate the selection (enemy/friend test only colours the label; talk and mount additionally
//      need a free look, a riding skill high enough and/or a short distance);
//   5. reset the HUD line cache, localise the label ("ui_talk", "ui_mount", ...) and draw the coloured
//      label at y=0.027 followed by a grey " <action>" at y=0.01/0.035;
//   6. publish {mode, selection, valid} to Screen+0x2c8/+0x2cc/+0x2d4, where the key handler
//      (s005261e0) consumes them.
//
// Layouts: shape/capsule/contact/level from s00439100, Agent/ItemRef/ItemKind from s00434ad0 +
// s0043b5d0, tstring/gui_manager/StrTable idioms from s0052ae70, Screen from s005261e0, the map
// object helpers from s004c6ca0. Names are provisional; every offset is read off this disassembly.
#include <windows.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

extern "C" int _fltused = 0;

// ---- engine string (0x10 bytes) ----------------------------------------------------------------------
struct tstring {
    int pos;                                                    // +0x0
    char* buf;                                                  // +0x4
    int cap;                                                    // +0x8
    int len;                                                    // +0xc
    tstring();                                                  // 0x42bbd0
    tstring(const char* s);                                     // 0x42bbe0
    ~tstring();                                                 // 0x42bc40
    tstring& operator=(const tstring& o);                       // 0x42bc50
    void assign(const char* s);                                 // 0x42bcb0
    void append(const tstring& o);                              // 0x42c1e0
    void underscores_to_spaces();                               // 0x42bf80
};

// ---- gui manager (0x5d7080) --------------------------------------------------------------------------
struct gui_manager {
    void set_view(const float* v);                              // 0x4c5190 (ret 4)
    void draw_string(const tstring* s, float y, unsigned color, int centered);  // 0x4e43e0 (ret 0x10)
};
extern gui_manager g_gui;                                       // 0x5d7080

// the HUD panel object stored in gui_manager+0x568 (0x5d75e8)
struct hud_panel {
    virtual void reset(int on);                                 // slot 0
};

// ---- localisation tables (0x5e2a18) ------------------------------------------------------------------
struct StrTable {
    tstring* find(const tstring* key);                          // 0x4c6010 (null when absent, ret 4)
    tstring* lookup(const tstring& key);                        // 0x43a1e0 (ret 4)
};
extern StrTable g_strtable;                                     // 0x5e2a18

// ---- collision shapes (s00439100) --------------------------------------------------------------------
struct shape_base {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}                                    // slot 0
    virtual int shape_type();                                   // slot 1 (0 sphere, 1 capsule, 2 face, ...)
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual vector4* get_min(vector4* out);                     // slot 10
    virtual vector4* get_max(vector4* out);                     // slot 11
};
struct capsule : shape_base {                                   // vtable 0x54a540, size 0x64
    vector4 p0, p1, p2, p3;                                     // 0x20 / 0x30 / 0x40 / 0x50
    float radius;                                               // 0x60
    capsule(float r, const vector4* a, const vector4* b);       // 0x433310 (ret 0xc)
    void compute_bounds(const matrix* m);                       // 0x408770 (ret 4)
};
struct contact {                                                // 0x20 bytes
    vector4 pos;                                                // 0x00
    vector4 point;                                              // 0x10
};
int shape_contact_count(shape_base* a, shape_base* b, int max, contact* out);   // 0x40bfe0 (cdecl)
matrix* identity_pos(matrix* out);                              // 0x40cd00 (cdecl)

// ---- level / scene -----------------------------------------------------------------------------------
struct SceneEntity {
    void* vtbl;                                                 // +0x00
    char pad4[0x0c - 0x04];
    int id;                                                     // +0x0c (agent / prop index)
    int type;                                                   // +0x10 (0xb agent, 0xc prop, ...)
    char pad14[0x88 - 0x14];
    vector4 pos;                                                // +0x88
    shape_base* shape;                                          // +0x98
};
struct Scene {
    void query_segment(const float* a, const float* b, unsigned mask,
                       shape_base** out, int* count);           // 0x439870 (ret 0x14)
    char pad4[0x80 - 0x04];
    SceneEntity** ent_begin;                                    // +0x80
    SceneEntity** ent_end;                                      // +0x84
    void collect_entities(int type, SceneEntity** out, int* count);   // 0x51a3a0 (ret 0xc)
};

// ---- items (s00434ad0 / s0043b5d0) -------------------------------------------------------------------
struct ItemKind {                                               // 0x238, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned char type_flags;                                   // +0x1c8 (low byte: itp_type_*)
    char pad1c9[0x1ec - 0x1c9];
    int f1ec;                                                   // +0x1ec
    char pad1f0[0x238 - 0x1f0];
};
extern ItemKind* g_items;                                       // 0x5e29d0

// an 8-byte item reference; the modifier sits in byte 7
struct ItemRef {
    int item;                                                   // +0x0
    unsigned m4;                                                // +0x4 (modifier in byte +7)
    int stat_1ec();                                             // 0x434c50 (thiscall)
    void name(char* out);                                       // 0x512730 (thiscall, ret 4)
};

// ---- agents (0x5928 bytes) ---------------------------------------------------------------------------
struct Agent {
    char pad0[0x10];
    int f10;                                                    // +0x10 kind (0 = human, 1 = mount)
    int f14;                                                    // +0x14 (1 = rideable)
    char pad18[0x2c - 0x18];
    int mount;                                                  // +0x2c mounted agent id (-1 = on foot)
    int f30;                                                    // +0x30 (-1 = free)
    char pad34[0x50 - 0x34];
    vector4 pos;                                                // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                                  // +0x88 troop record index
    ItemRef item;                                               // +0x8c
    char pad94[0xac - 0x94];
    float f9c;                                                  // +0xac
    char padb0[0x55d8 - 0xb0];
    vector4 facing;                                             // +0x55d8
    char pad55e8[0x5830 - 0x55e8];
    int f5830;                                                  // +0x5830 action state (2 = mounted)
    char pad5834[0x5928 - 0x5834];

    float height();                                             // 0x432450 (thiscall, no arg)
    bool is_enemy(Agent* o);                                    // 0x432350 (ret 4)
    bool is_friend(Agent* o);                                   // 0x4323a0 (ret 4)
};

// ---- game data (0x5e2930) ----------------------------------------------------------------------------
struct GameData {
    char pad0[0x8c];
    int f8c;                                                    // +0x8c (troop record used for skills)
    char pad90[0x13a00 - 0x90];
    char* chars;                                                // +0x13a00 troop records (0x508 bytes)
    char* map_records;                                          // +0x13a50 area records (0x494 bytes)
    int f13a54;                                                 // +0x13a54 number of areas
    char pad13a58[0x13b10 - 0x13a58];
    char f13b10;                                                // +0x13b10
    char f13b11;                                                // +0x13b11
    int skill_level(int troop, int skill);                      // 0x437b20 (ret 8)
};
extern GameData* g_game;                                        // 0x5e2930

// ---- mission (0x5e292c) ------------------------------------------------------------------------------
struct SceneProp {                                              // 0x1c8, array at mission+0x668c
    char pad0[0x188];
    int destroyed;                                              // +0x188 (0 intact, 5 map object, 6 equipment)
    int kind;                                                   // +0x18c scene prop kind
    char pad190[0x1a0 - 0x190];
    int inv;                                                    // +0x1a0 inventory / chest index
    int obj;                                                    // +0x1a4 map object / equipment index
    char pad1a8[0x1c8 - 0x1a8];
};
struct PropKind {                                               // 0x78, array at 0x5e29dc
    char pad0[0x10];
    int flags;                                                  // +0x10 (low byte: 5 chest, 10 exit)
    char pad14[0x78 - 0x14];
};
extern PropKind* g_prop_kinds;                                  // 0x5e29dc

struct Mission {
    Scene* scene;                                               // +0x00 (level grid + entity list)
    char pad4[0x1c - 0x04];
    Agent** pages;                                              // +0x1c agent table pages
    char pad20[0x5c - 0x20];
    int local_id;                                               // +0x5c
    char pad60[0x5df0 - 0x60];
    int f5df0;                                                  // +0x5df0 current area record
    int f5df4;                                                  // +0x5df4 current party/area index
    char pad5df8[0x6528 - 0x5df8];
    char f6528;                                                 // +0x6528 flags (bit 0)
    char pad6529[0x653c - 0x6529];
    float f653c;                                                // +0x653c mission border min x
    float f6540;                                                // +0x6540 mission border min y
    float f6544;                                                // +0x6544 mission border max x
    float f6548;                                                // +0x6548 mission border max y
    char pad654c[0x668c - 0x654c];
    SceneProp* props;                                           // +0x668c
};
extern Mission* g_mission;                                      // 0x5e292c

inline Agent* agent_at(int id)
{
    return (Agent*)((char*)g_mission->pages[id >> 4] + (id & 0x0f) * 0x5928);
}

// ---- map objects (0x5e29b0 / 0xa735c4, helpers in s004c6ca0) -----------------------------------------
struct MapObject {                                              // 0x68 bytes
    char pad0[0x10];
    tstring name;                                               // +0x10
    tstring mesh;                                               // +0x20
    tstring* mesh_ptr();                                        // 0x4c6ca0 (thiscall)
};
struct MapObjList {                                             // 0x60 bytes
    char pad0[0x54];
    int count;                                                  // +0x54
    MapObject* objs;                                            // +0x58
};
extern MapObjList* g_mapobjs;                                   // 0x5e29b0
extern int g_mapobj_idx;                                        // 0xa735c4

// ---- area records (0x5e2a00, 0x58 bytes) --------------------------------------------------------------
struct Area {                                                   // 0x58 bytes
    char pad0[0x20];
    unsigned flags;                                             // +0x20
    char pad24[0x58 - 0x24];
};
extern Area* g_areas;                                           // 0x5e2a00

// ---- HUD globals -------------------------------------------------------------------------------------
extern hud_panel* g_panel;                                      // 0x5d75e8 (gui_manager+0x568)
extern int g_hud_line_count;                                    // 0x5d8024
extern float g_hud_line_dy;                                     // 0x5d8028
extern int g_hud_line_x;                                        // 0x5d8020
extern int g_hud_flag;                                          // 0x5d8780
extern int g_panel_mode;                                        // 0x5d8760
extern int g_panel_color;                                       // 0x5d8764
extern tstring g_hud_lines[80];                                 // 0x5d7850 .. 0x5d7d50

// ---- the mission screen ------------------------------------------------------------------------------
struct Screen {
    char pad0[0x26c];
    float f26c;                                                 // +0x26c
    char pad270[0x2c8 - 0x270];
    int f2c8;                                                   // +0x2c8 interaction mode
    int f2cc;                                                   // +0x2cc selection id
    char pad2d0[0x2d4 - 0x2d0];
    int f2d4;                                                   // +0x2d4 selection valid
    void f_529ec0();                                            // 0x529ec0
};

// @ 0x529ec0
void Screen::f_529ec0()
{
    Mission* m = g_mission;
    Agent* me = agent_at(m->local_id);

    vector4 pos = me->pos;
    int mode = -1;
    int target = -1;
    float best = 1e9f;
    float second = 1e9f;

    // ---- phase A: cast the thin probe 10.5 m ahead and find the nearest face hit ----------------------
    float eye_z = me->height() + pos.z;
    vector4 front(pos.x + me->facing.x * 10.5f,
                  pos.y + me->facing.y * 10.5f,
                  eye_z + me->facing.z * 10.5f);
    vector4 back(pos.x + me->facing.x * 0.05f,
                 pos.y + me->facing.y * 0.05f,
                 eye_z + me->facing.z * 0.05f);
    front.w = pos.w;
    back.w = pos.w;

    capsule cap(1e-4f, &back, &front);
    matrix ident;
    identity_pos(&ident);
    cap.compute_bounds(&ident);

    shape_base* found[0x2000];
    int count = 0;

    m->scene->query_segment((const float*)&back, (const float*)&front, 0x110, found, &count);
    for (int i = 0; i < count; i++) {
        shape_base* s = found[i];
        if (s->shape_type() != 2)
            continue;
        contact c;
        if (!shape_contact_count(s, &cap, 1, &c))
            continue;
        vector4 hit = c.point;
        vector4 d = hit - back;
        float len = d.normalize();
        if (len < best) {
            best = len;
            mode = -1;
            second = len;
        }
    }

    // ---- phase B: the same probe as a 0.1 m capsule, 0.1 m further out, against the entities ---------
    vector4 axis = cap.p1 - cap.p0;
    axis.normalize();
    cap.p0.x += axis.x * 0.1f;
    cap.p0.y += axis.y * 0.1f;
    cap.p0.z += axis.z * 0.1f;
    cap.radius = 0.1f;
    best += 1.0f;

    m->scene->collect_entities(9, (SceneEntity**)found, &count);
    for (int i = 0; i < count; i++) {
        SceneEntity* e = (SceneEntity*)found[i];
        if (!e->shape)
            continue;
        contact c;
        if (!shape_contact_count(e->shape, &cap, 1, &c))
            continue;

        vector4 d = c.point - back;
        float len = d.length();
        if (len > best)
            continue;

        bool keep = false;
        if (e->type == 0x0b) {
            Agent* a = agent_at(e->id);
            if (a->f10 == 0) {
                // a human: talk target, unless it is the player or an enemy
                if (e->id == m->local_id)
                    continue;
                if (me->is_enemy(a))
                    continue;
                mode = 1;
                target = e->id;
                best = len;
                vector4 r = e->pos - back;
                second = r.length();
            } else if (a->f10 == 1) {
                // a ridden mount
                if (g_items[a->item.item].type_flags != 1)
                    continue;
                if (a->f30 != -1 || a->f14 != 1 || a->f5830 != 2)
                    continue;
                mode = 2;
                target = e->id;
                best = d.out_length();
                vector4 r = e->pos - back;
                second = r.length();
            }
        } else if (e->type == 0x0c) {
            SceneProp* p = &m->props[e->id];
            if (p->destroyed == 0) {
                int kind = p->kind;
                int flags = g_prop_kinds[kind].flags & 0xff;
                if (flags == 5) {
                    mode = 5;
                    target = e->id;
                    best = len;
                    second = len;
                } else if (flags == 10 && len < 4.0f) {
                    mode = 8;
                    target = e->id;
                    best = len;
                    second = len;
                } else {
                    keep = true;
                }
            } else if (p->destroyed == 5) {
                if (g_mapobj_idx <= 0 || p->obj < 0 || p->obj >= g_mapobjs[g_mapobj_idx].count) {
                    keep = true;
                } else {
                    mode = 6;
                    target = e->id;
                    best = len;
                    second = len;
                }
            } else if (p->destroyed == 6) {
                mode = 7;
                target = e->id;
                best = len;
                second = len;
            } else {
                keep = true;
            }
        } else {
            keep = true;
        }

        if (keep && mode >= 0) {
            best = len;
            second = len;
        }
    }

    // ---- phase C: nothing picked -> mount/dismount, then the four mission borders ---------------------
    if (mode == -1) {
        if (me->mount != -1 && f26c < -0.5519978f && me->f9c < 1.0f) {
            mode = 3;
        } else if ((m->f6544 - 0.5f < me->pos.x && me->facing.x > 0.5f) ||
                   (m->f6548 - 0.5f < me->pos.y && me->facing.y > 0.5f) ||
                   (me->pos.x < m->f653c + 0.5f && me->facing.x < -0.5f) ||
                   (me->pos.y < m->f6540 + 0.5f && me->facing.y < -0.5f)) {
            mode = 8;
        }
    }

    // ---- phase D: validate --------------------------------------------------------------------------
    int ok = 0;
    if (mode == 1) {
        int flagged = (int)((g_areas[m->f5df0].flags >> 1) & 1);
        if (flagged || (m->f6528 & 1)) {
            if (!g_game->f13b10)
                goto no_selection;
        }
        if (g_game->f13b11)
            goto no_selection;
        if (second < 3.5f)
            ok = 1;
    } else if (mode == 2) {
        if (me->mount != -1)
            goto no_selection;
        {
            Agent* a = agent_at(target);
            int stat = 0;
            if (a->item.item >= 0) {
                ItemKind* k = &g_items[a->item.item];
                stat = k->f1ec;
                if (stat != 0) {
                    int mod = (signed char)(a->item.m4 >> 24);
                    if (k->type_flags == 1) {
                        if (mod == 0x21) stat -= 1;
                        else if (mod == 0x20) stat += 1;
                        else if (mod == 0x24) stat += 2;
                    } else {
                        if (mod == 0x12) stat += 1;
                        else if (mod == 0x13) stat += 2;
                        else if (mod == 0x11) stat += 4;
                    }
                }
            }
            if (stat > g_game->skill_level(g_game->f8c, 0x18))
                goto no_selection;
            vector4 r = a->pos - me->pos;
            if (r.length() < 1.3f)
                ok = 1;
        }
    } else if (mode == 4) {
        if (second < 2.5f)
            ok = 1;
    } else if (mode == 5) {
        if (second < 5.0f)
            ok = 1;
    } else if (mode == 6 || mode == 7) {
        if (second < 2.7f)
            ok = 1;
    }
no_selection:

    // ---- phase E: reset the HUD cache ----------------------------------------------------------------
    tstring name;
    int color = -1;
    tstring action;

    if (g_panel)
        g_panel->reset(1);
    g_panel_mode = -1;
    g_panel_color = 0xff101010;
    for (int i = 0; i < 80; i++)
        g_hud_lines[i].assign("");
    g_hud_line_count = 0;
    g_hud_line_dy = -0.03f;
    g_hud_line_x = 0;
    g_panel = 0;
    g_hud_flag = 0;
    float view[2];
    view[0] = 0.5f;
    view[1] = 0.23f;
    g_gui.set_view(view);

    // ---- phase F: name the selection -----------------------------------------------------------------
    if (mode == 1) {
        Agent* a = agent_at(target);
        char* rec = g_game->chars + a->troop * 0x508;
        tstring* tr = g_strtable.find((tstring*)(rec + 0x20));
        name = tr ? *tr : *(tstring*)rec;
        if (ok) {
            tstring k("ui_talk");
            action = *g_strtable.lookup(k);
        }
        if (me->is_enemy(a))
            color = (int)0xffd06050;
        else if (me->is_friend(a))
            color = (int)0xff60d050;
    } else if (mode == 2) {
        Agent* a = agent_at(target);
        char buf[0x40];
        a->item.name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_mount");
            action = *g_strtable.lookup(k);
        } else if (me->mount == -1) {
            int skill = g_game->skill_level(g_game->f8c, 0x18);
            int stat = agent_at(target)->item.stat_1ec();
            if (stat > skill) {
                tstring k("ui_riding_skill_not_adequate_to_mount");
                action = *g_strtable.lookup(k);
            }
        }
    } else if (mode == 3) {
        tstring k("ui_dismount");
        action = *g_strtable.lookup(k);
    } else if (mode == 4) {
        char* rec = g_game->map_records + m->f5df4 * 0x494;
        int dest = *(int*)(rec + 0x7c + target * 4);
        if (dest > g_game->f13a54) {
            tstring k("ui_exit");
            action = *g_strtable.lookup(k);
        } else if (dest >= 0) {
            tstring k("ui_door_to");
            name = *g_strtable.lookup(k);
            name.append(*(tstring*)(g_game->map_records + dest * 0x494 + 0x20));
            name.underscores_to_spaces();
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable.lookup(k2);
            }
        }
    } else if (mode == 5) {
        SceneProp* p = &m->props[target];
        char* rec = g_game->map_records + m->f5df4 * 0x494;
        int inv = p->inv;
        if (inv < *(int*)(rec + 0x100) && inv >= 0) {
            tstring k("ui_chest");
            name = *g_strtable.lookup(k);
            if (ok) {
                tstring k2("ui_open");
                action = *g_strtable.lookup(k2);
            }
        } else {
            tstring k("ui_baggage");
            name = *g_strtable.lookup(k);
            if (ok) {
                tstring k2("ui_access_inventory");
                action = *g_strtable.lookup(k2);
            }
        }
    } else if (mode == 6) {
        SceneProp* p = &m->props[target];
        MapObject* o = (MapObject*)((char*)g_mapobjs[g_mapobj_idx].objs + p->obj * 0x68);
        name = *o->mesh_ptr();
        if (ok) {
            tstring k("ui_go");
            action = *g_strtable.lookup(k);
        }
    } else if (mode == 7) {
        SceneProp* p = &m->props[target];
        ItemRef ref;
        ref.item = p->kind;
        ref.m4 = p->inv;
        char buf[0x40];
        ref.name(buf);
        name.assign(buf);
        if (ok) {
            tstring k("ui_equip");
            action = *g_strtable.lookup(k);
        }
    } else if (mode == 8) {
        name.assign("");
        tstring k(((g_areas[m->f5df0].flags >> 1) & 1) ? "ui_retreat_battle" : "ui_leave_area");
        action = *g_strtable.lookup(k);
    }

    // ---- phase G: draw -------------------------------------------------------------------------------
    if (name.len)
        g_gui.draw_string(&name, 0.027f, (unsigned)color, 1);
    if (action.len) {
        tstring sep(" ");
        g_gui.draw_string(&sep, 0.01f, 0xffff9d10, 1);
        g_gui.draw_string(&action, 0.035f, 0xffff9d10, 1);
    }

    // ---- phase H: publish ----------------------------------------------------------------------------
    f2c8 = mode;
    f2cc = target;
    f2d4 = ok;
}
