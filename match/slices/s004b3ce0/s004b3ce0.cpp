// Slice s004b3ce0: the map-order planner of a party (map AI).
//   0x4b3ce0  Party::plan_order  (act on the current map order: finish a move, chase a party or an agent, or
//             sample a random walkable spot around the current position and hand it to 0x4b1780 as a new order)
//   0x4b4320  Party::step_route  (advance along the current route: walk the formation-record chain towards the
//             next waypoint and publish it as the destination)
// This is the party counterpart of Agent::tick (s004b80d0 calls 0x4b3ce0); the layouts follow s004b4a00
// (Party), s004b3420 (WorldMap::can_move) and s004b80d0 (GameData / the 0x88-byte formation record).
#include <math.h>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;

// ---- engine helpers -----------------------------------------------------------------------------------------
int  __cdecl msvc_rand();                               // 0x536a9f  CRT rand()
float __cdecl frand();                                  // 0x40e400  (rand() % 15817) / 15817
char __cdecl f489390(int kind);                         // 0x489390  (is-farm/road terrain kind)
char __cdecl f4893b0(int kind);                         // 0x4893b0  (is-water terrain kind)
extern int g_tick_words[];                              // 0x5e2948  two 32-bit words per clock entry

struct SceneObj;                                        // map agent, see below

struct vec2 {                                           // 8-byte 2-D vector
    float x, y;
    void  rotate(float ang);                            // 0x432600  replace by the unit vector at `ang`
    float normalize();                                  // 0x4367c0  normalise in place, return old length
};

struct BigRec {                                         // 0x88-byte formation record
    char pad0[0x24];
    unsigned mask;                                      // +0x24
    char pad28[0x88 - 0x28];
};

struct SceneObj {                                       // 0x48ea80 / 0x51ee50 result (the map agent of a party)
    char pad0[4];
    int anim;                                           // +0x04  0x88-byte record index
    char pad8[0x30 - 0x08];
    float x;                                            // +0x30
    float y;                                            // +0x34
    char pad38[0xa0 - 0x38];
    unsigned flags;                                     // +0xa0
};

struct WorldMap {
    char pad0[0x14];
    BigRec* recs;                                       // +0x14
    bool linked(const int* a, const int* b);            // 0x48efa0
    bool can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max);   // 0x4b3420
};
struct MapGlobal {
    char pad0[0xb0];
    WorldMap* world;                                    // +0xb0
};
extern MapGlobal* g_map;                                // 0xa732b4

struct PartyMgr {                                       // g_game + 0xf8
    bool      arrived(int id);                          // 0x4903e0
    SceneObj* get(int id);                              // 0x51ee50
};
struct GameData {
    char pad0[0x78];
    int camera_id;                                      // +0x78
    char pad7c[0xf8 - 0x7c];
    PartyMgr agents;                                    // +0xf8
    SceneObj* find_at(const float* p);                  // 0x48ea80
};
extern GameData* g_game;                                // 0x5e2930

// ---- the party ---------------------------------------------------------------------------------------------
struct Party {
    char pad0[4];
    int type;                                           // +0x04
    int id;                                             // +0x08
    char padc[0x30 - 0xc];
    float x;                                            // +0x30
    float y;                                            // +0x34
    char pad38b[0x40 - 0x38];
    float dirx;                                         // +0x40
    float diry;                                         // +0x44
    char pad48[0x50 - 0x48];
    float angle;                                        // +0x50
    char pad54[0xa0 - 0x54];
    unsigned flags;                                     // +0xa0
    char pada4[0x8e0 - 0xa4];
    float interval;                                     // +0x8e0
    char pad8e4[0x8f4 - 0x8e4];
    float f8f4;                                         // +0x8f4
    int f8f8;                                           // +0x8f8
    char pad8fc[0x900 - 0x8fc];
    int f900;                                           // +0x900  (clock slot, always set with 0x904)
    int f904;                                           // +0x904
    char pad908[0x918 - 0x908];
    int action;                                         // +0x918
    int target;                                         // +0x91c
    char pad920[4];
    vector4 order_pos;                                  // +0x924
    char pad934[0x95c - 0x934];
    float f95c;                                         // +0x95c
    float f960;                                         // +0x960
    int f964;                                           // +0x964
    int f968;                                           // +0x968
    char pad96c[0x596c - 0x96c];
    int f596c;                                          // +0x596c  route cursor
    int count;                                          // +0x5970  route length
    SceneObj* mesh;                                     // +0x5974  current map agent
    char pad5978[0x5980 - 0x5978];
    int moving;                                         // +0x5980
    int f5984;                                          // +0x5984
    int f5988;                                          // +0x5988
    char pad598c[0x5998 - 0x598c];
    int f5998;                                          // +0x5998

    float speed_factor();                               // 0x4b20b0
    void set_order(int* pos, int* t);                   // 0x4b1780
    void set_order_party(int* t);                       // 0x4b29d0
    void set_order_agent(int* t);                       // 0x4b2ab0
    void plan_order();                                  // 0x4b3ce0
    void step_route();                                  // 0x4b4320
};

// @ 0x4b3ce0
void Party::plan_order()
{
    if (action == 0) {
        goto done;
    }
    if (action == 7) {
        goto done;
    }
    if (action == 8) {
    done:
        f5998 = 0;
        moving = 0;
        f5988 = 0;
        return;
    }

    if (action == 2 || action == 3) {
        f900 = g_tick_words[f8f8 * 2];
        f904 = g_tick_words[f8f8 * 2 + 1];
        vec2 to;
        if (action == 3 && target >= 0 && g_game->agents.arrived(target)) {
            SceneObj* p = g_game->agents.get(target);
            to.x = p->x;
            to.y = p->y;
        } else {
            to.x = order_pos.x;
            to.y = order_pos.y;
        }
        bool give_up = false;
        int tries = 0;
        float spread = 0.6f;
        vec2 p;
        for (;;) {
            int r = msvc_rand() % 15817;
            float ang = ((float)r * 6.322312e-05f - 0.5f) * spread * 3.1415927f;
            float c = cosf(ang);
            float s = sinf(ang);
            r = msvc_rand();
            float len = speed_factor() * ((float)r * 0.00012644623f + 1.0f);
            p.x = (c * dirx - s * diry) * len + x;
            p.y = (s * dirx + c * diry) * len + y;
            r = msvc_rand();
            float k = (float)r / f8f4 * 6.322312e-05f;
            p.x = (to.x - x) * k + p.x;
            p.y = (to.y - y) * k + p.y;
            SceneObj* a = g_game->find_at(&p.x);
            tries++;
            spread += 0.4f;
            if (tries > 6) {
                f5998 = 0;
                moving = 0;
                f5988 = 0;
                give_up = true;
            }
            if (a) {
                WorldMap* w = g_map->world;
                if (w->linked(&a->anim, &mesh->anim)) {
                    unsigned kind = w->recs[a->anim].mask;
                    if (flags & 0x200) {
                        if (kind == 7 || (kind & 7) == 0) {
                            set_order((int*)&p, (int*)a);
                            return;
                        }
                    } else if (f4893b0(kind)) {
                        set_order((int*)&p, (int*)a);
                        return;
                    }
                }
            }
            if (give_up)
                return;
        }
    }

    if (action == 6) {
        float dx = order_pos.x - x;
        float dy = order_pos.y - y;
        if (sqrtf(dx * dx + dy * dy) < 0.01f) {
            action = 0;
            return;
        }
    }
    if (action == 9) {
        return;
    }

    int t = target;
    if (t >= 0) {
        if (action == 4 || action == 1) {
            if (g_game->agents.arrived(t)) {
                if (g_game->agents.get(t)->flags & 0x400)
                    return;
                set_order_party((int*)&target);
                return;
            }
            action = 0;
            return;
        }
        if (action == 10) {
            if (g_game->agents.arrived(t)) {
                set_order_agent((int*)&target);
                return;
            }
            action = 0;
            return;
        }
    }
    if (action == 5 || action == 11) {
        if (g_game->agents.arrived(t)) {
            goto chase;
        }
        action = 0;
        return;
    }
    return;

chase:
    vec2 dir;
    dir.x = 0.0f;
    dir.y = 1.0f;
    dir.rotate(angle);
    SceneObj* q = g_game->agents.get(t);
    vec2 d;
    d.x = q->x - x;
    d.y = q->y - y;
    d.normalize();
    vec2 v;
    v.x = dir.x * 0.85f - d.x;
    v.y = dir.y * 0.85f - d.y;
    if (v.x * v.x + v.y * v.y < 1e-13f) {
        v.x = 1.0f;
        v.y = 0.0f;
    }
    v.normalize();

    bool give_up = false;
    int tries = 0;
    float spread = 0.6f;
    vec2 p;
    for (;;) {
        float ang = (frand() - 0.5f) * spread;
        v.rotate(ang);
        float len = (interval + interval + 0.5f) * speed_factor();
        len = len + len;
        p.x = v.x * len + x;
        p.y = v.y * len + y;
        SceneObj* a = g_game->find_at(&p.x);
        tries++;
        spread += 0.4f;
        if (tries > 6) {
            f5998 = 0;
            moving = 0;
            f5988 = 0;
            give_up = true;
        }
        if (a) {
            WorldMap* w = g_map->world;
            if (w->linked(&a->anim, &mesh->anim)) {
                unsigned kind = w->recs[a->anim].mask;
                if (flags & 0x200) {
                    if (f489390(kind)) {
                        set_order((int*)&p, (int*)a);
                        return;
                    }
                } else if (f4893b0(kind)) {
                    set_order((int*)&p, (int*)a);
                    return;
                }
            }
        }
        if (give_up)
            return;
    }
}

// @ 0x4b4320
void Party::step_route()
{
    if (moving == 0)
        return;
    int z = 0;
    int n = count;
    int cur = mesh->anim;
    if (cur != *(int*)((char*)this + 0x968 + n * 4)) {
        if (cur >= 0) {
            int i = f596c;
            vec2 d;
            int goal;
            if (i + 1 < n) {
                d = *(vec2*)((char*)this + 0x197c + i * 0x10);
                goal = *(int*)((char*)this + 0x970 + i * 4);
            } else {
                d.y = f960;
                d.x = f95c;
                goal = f5984;
            }
            float lim;
            int tries;
            if (id == g_game->camera_id) {
                lim = 22500.0f;
                tries = 200;
            } else {
                lim = 900.0f;
                tries = 10;
            }
            float ndx = x - d.x;
            float ndy = y - d.y;
            float dist = ndx * ndx + ndy * ndy;
            if (dist < lim && g_map->world->can_move(cur, x, y, goal, &d.x, 0, tries))
                f596c = i + 1;
            goto zz;
        }
    }
zz:
    if (f596c < n) {
        int off = f596c * 0x10;
        int vv = *(int*)((char*)this + 0x196c + off);
        f964 = vv;
        f968 = *(int*)((char*)this + 0x1970 + off);
        return;
    }
    moving = z;
    f5988 = z;
}
