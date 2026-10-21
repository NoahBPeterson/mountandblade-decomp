// Slice s004452f0: navigation-mesh reachability search (A*-style open list on a priority_queue of 12-byte nodes),
// the vector<E56>::_Ucopy helper, and the Agent helpers that pick a reachable goal near a point, plan a path and play
// a troop-class sound. All names provisional; layouts copied from s00439100 (NavMesh), s00438860 (heap node),
// s0043db60 / s00437f00 (Agent) and s004417d0 (E56).
#include <stdlib.h>
#include "nav_search.h"

// ---- E56 (s004417d0 / s0043edf0): 0x38-byte record holding two vector<int> ---------------------------------------------
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
    E56(const E56& o)
        : a(o.a), b(o.b), c(o.c), v1(o.v1), v2(o.v2), d(o.d), e(o.e), f(o.f)
    {
    }
};
namespace std {
template<> E56* _Uninit_copy<E56*, E56*, allocator<E56> >(E56*, E56*, E56*, allocator<E56>&, _Nonscalar_ptr_iterator_tag);        // 0x442080
}
// @ 0x445700
template std::vector<E56>::pointer std::vector<E56>::_Ucopy<E56*>(E56*, E56*, E56*);

// ---- the mission object (g_mission, 0x5e292c) and game data (0x5e2930) ---------------------------------------------------
struct ClassRec {                                       // 0x6c bytes: per team and troop class
    char pad0[0x20];
    vector4 v20;                                        // +0x20
    int face;                                           // +0x30
    char pad34[0x6c - 0x34];
};
struct TeamRec { ClassRec cls[3]; char pad[0x16c - 0x144]; };
struct Mission {
    char pad0[0x5284];
    TeamRec teams[8];                                   // +0x5284
    char pad5dc4[0x6698 - (0x5284 + 8 * 0x16c)];
    NavMesh nav;                                        // +0x6698
};
extern Mission* g_mission;                              // 0x5e292c

struct Troop {                                          // 0x508 bytes
    char pad0[0x30];
    unsigned __int64 flags;                             // +0x30
    char pad38[0x508 - 0x38];
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                                      // +0x13a00
};
extern World* g_world;                                  // 0x5e2930

// 0x1f8-byte voice records (table object at 0x8ed4f60: records at +0x1c, count at +0x28; see s0043ac40)
struct VoiceRec { char pad[0x1cc]; int snd[1]; char pad1d0[0x1f8 - 0x1d0]; };
struct VoiceTable {
    char pad0[0x18];
    std::vector<VoiceRec> recs;                         // +0x18 (first at +0x1c); element size 0x1f8
    unsigned count;                                     // +0x28
    int sound(unsigned cls, int kind)
    {
        if (cls < count)
            return recs[cls].snd[kind];
        return -1;
    }
};
extern VoiceTable g_voices;                             // 0x8ed4f60

// item kind record (0x238 bytes, array at 0x5e29d0) and item reference (index + packed amount word)
struct ItemKind {
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8
    char pad1cc[0x238 - 0x1cc];
};
extern ItemKind* g_items;                               // 0x5e29d0
struct ItemRef { int item; unsigned m4; };
#define itp_two_handed 0x200000

struct Agent {
    char pad0[0x10];
    int f10;                                            // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                            // +0x2c (mount, -1 on foot)
    char pad30[0x88 - 0x30];
    int troop;                                          // +0x88
    char pad8c[0x1c4 - 0x8c];
    int team;                                           // +0x1c4
    char pad1c8[0x1d0 - 0x1c8];
    ItemRef held[8];                                    // +0x1d0
    char pad210[0x22c - 0x210];
    int cur_slot;                                       // +0x22c
    char pad230[0x23c - 0x230];
    int f23c;                                           // +0x23c (two-handed wield)
    char pad240[0x26c - 0x240];
    vector4 goal_dir;                                   // +0x26c
    int goal_face;                                      // +0x27c
    int goal_face_done;                                 // +0x280
    char pad284[0x324 - 0x284];
    int path_face[0x400];                               // +0x324
    vector4 path_pos[0x3ff];                            // +0x1324
    char pad5314[0x5328 - 0x5314];
    int path_cur;                                       // +0x5328
    int path_count;                                     // +0x532c
    char path_flag;                                     // +0x5330
    char pad5331[0x5334 - 0x5331];
    int path_steps;                                     // +0x5334
    char pad5338[0x5764 - 0x5338];
    float f5764;                                        // +0x5764
    char pad5768[0x57a4 - 0x5768];
    float f57a4;                                        // +0x57a4
    char pad57a8[0x5928 - 0x57a8];

    void set_goal_near(const vector4* center, const float* radius);                       // 0x43dd70
    int update_nav_face();                                                                 // 0x43cb30
    void play_sound(int* snd);                                                             // 0x443870
    void pick_reachable_goal(const vector4* center, const float* radius, const int* face, const int* max_steps);   // 0x445720
    void goal_near_for_class(const vector4* center, const float* radius);                  // 0x445850
    bool plan_path();                                                                      // 0x445910
    void play_class_sound(const int* kind);                                                // 0x4459c0
    void on_wield_reset();                                                                 // 0x445a20
    void update_equipment_display();                                                       // 0x443aa0
    void on_wield_changed();                                                               // 0x444370
};

// ---- reachability: can the goal face be reached within `max_steps` hops? ---------------------------------------------
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
                    vector4 dg = vsub(faces[*to].c, faces[nb].c);
                    vector4 ds = vsub(faces[*from].c, faces[nb].c);
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
        c = vadd(vscale(k, vsub(c, f.c)), f.c);
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
        else if ((g_world->troops[troop].flags & 0x4000000) != 0)
            cls = 1;
        else
            cls = 0;
    }
    ClassRec* r = &g_mission->teams[t].cls[cls];
    int max_steps = 7;
    pick_reachable_goal(center, radius, &r->face, &max_steps);
    goal_dir = r->v20;
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
        g_mission->nav.find_path(face, goal_face, path_face, path_pos, &path_count, &path_steps, &width, &width, &width);
        path_cur = 0;
        path_flag = path_count <= 0;
        goal_face_done = goal_face;
    }
    return path_count > 0;
}

// @ 0x4459c0
void Agent::play_class_sound(const int* kind)
{
    unsigned cls = (unsigned)g_world->troops[troop].flags & 0xf;
    int snd = g_voices.sound(cls, *kind);
    if (snd >= 0)
        play_sound(&snd);
}

// @ 0x445a20
void Agent::on_wield_reset()
{
    f23c = 0;
    if (cur_slot >= 0) {
        int item = held[cur_slot].item;
        if (item >= 0 && (g_items[item].type_flags & itp_two_handed) != 0)
            f23c = 1;
    }
    update_equipment_display();
    on_wield_changed();
    f57a4 = -20000000.0f;
    f5764 = -20000000.0f;
}
