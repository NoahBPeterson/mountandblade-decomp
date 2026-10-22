// Slice s005257f0: the dialogue "look at" update of the view object (0x5257f0) and its two agent lookups,
// find_speaker_agent (0x526070) and find_listener_agent (0x526130).  The lookup pair scans the mission's
// agent table for the agent whose troop id (+0x88) equals the current dialogue speaker / listener.
// The big function builds a look-at frame from the target and listener bodies' matrices, blends it into
// this+0x1194, copies it out, hands it to Mission::camera_to and advances the blend timer.  Names provisional.
#include <windows.h>
#include <math.h>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

// ---- agent records ---------------------------------------------------------------------------------------
struct AgentBody {                                  // Agent +0x250 (s00440a30/ai_agent.h)
    char pad0[0x58];
    matrix m;                                       // +0x58: s, f (+0x68), u, o (+0x88)
};
struct Agent {                                      // 0x5928 bytes
    char pad0[4];
    int used;                                       // +0x04
    char pad8[0x10 - 8];
    int f10;                                        // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                        // +0x2c
    char pad30[0x50 - 0x30];
    vector4 pos;                                    // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                      // +0x88 (troop / character id)
    char pad8c[0x250 - 0x8c];
    AgentBody* body;                                // +0x250
    float speed_scale() const;                      // 0x432450: 1.5, or 2.4 while standing and not deleted
    char pad254[0x5928 - 0x254];
};
struct AgentTable {                                 // mission+4
    int count;                                      // +0x00
    char pad4[0x14];
    Agent** pages;                                  // +0x18 (mission+0x1c)
    int first_used();                               // 0x43b1b0
    void next_used(unsigned* i);                    // 0x43b160
};
#define AGENT(id) (g_mission->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))

struct Mission {
    char pad0[4];
    AgentTable agents;                              // +0x04
    char pad20[0x5c - 0x20];
    int selected;                                   // +0x5c
    char pad60[0x6624 - 0x60];
    matrix m6624;                                   // +0x6624
    char pad6664[0x6748 - 0x6664];
    char cam[0x100];                                // +0x6748
    void camera_to(matrix* eye, float& far_plane);  // 0x4fa790
};
extern Mission* g_mission;                          // 0x5e292c

struct TroopRec {                                   // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                                 // +0x30 (0x10 = a talking troop?)
    char pad34[0x508 - 0x34];
};
struct Game {
    char pad0[0xc8];
    int player;                                     // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec* chars;                                // +0x13a00
};
extern Game* g_game;                                // 0x5e2930
extern int g_player_agent;                          // 0x8ed50dc (-1 when none)

struct DialogMgr {
    char pad0[0x8000];
    int speaker();                                  // 0x460640
};
extern DialogMgr g_n_dialogs;                       // 0x8ed50c8

int find_listener_agent();                          // 0x526130 (defined in slice s00526130)

extern float g_dt;                                  // 0x5a9c28 (per-frame delta time)
extern float g_look_rate;                           // 0x5935c8 (4.0)

// ---- the view object (only the members this function touches) --------------------------------------------
struct ViewObj {
    char pad0[0x30];
    void* f30;                                      // +0x30 (its +0x1ac receives 0x120 bytes from +0x34)
    char pad34[0xfc - 0x34];
    matrix m_fc;                                    // +0xfc (copy of the look-at frame)
    char pad13c[0x1194 - 0x13c];
    matrix m;                                       // +0x1194 (s, f, u, o)
    float blend;                                    // +0x11d4
    char started;                                   // +0x11d8
    void update();                                  // 0x5257f0
};

struct vec3 {
    float x, y, z;
    void sub_assign(const vec3& o);                 // 0x432770
};

static __forceinline float clamp01(float x)
{
    if (x < 0.0f)
        x = 0.0f;
    else if (x >= 1.0f)
        x = 1.0f;
    return x;
}

// @ 0x526070 -------------------------------------------------------------------------------------------
int find_speaker_agent()
{
    unsigned found = (unsigned)-1;
    int speaker = g_n_dialogs.speaker();
    if ((g_game->chars[speaker].flags & 0x10) || g_player_agent == -1) {
        AgentTable* t = &g_mission->agents;
        Agent** pages = t->pages;
        int any = 0;
        for (unsigned i = t->first_used(); i < (unsigned)t->count; t->next_used(&i)) {
            if ((pages[i >> 4] + (i & 0xf))->troop == speaker) {
                found = i;
                any = 1;
            }
        }
        if (any)
            return found;
    }
    return g_player_agent;
}

// @ 0x5257f0 -------------------------------------------------------------------------------------------
void ViewObj::update()
{
    unsigned target = find_speaker_agent();
    int listener = find_listener_agent();
    if (target == (unsigned)-1)
        return;

    Agent* a = AGENT(target);
    vector4 G = a->body->m.o;
    vector4 F;
    G.z = a->speed_scale() + G.z;

    if (target == (unsigned)g_player_agent) {
        Agent* sel = AGENT(g_mission->selected);
        vector4 dir;
        dir.x = sel->pos.x - a->pos.x;
        dir.y = sel->pos.y - a->pos.y;
        dir.z = sel->pos.z - a->pos.z;
        float len = dir.normalize();
        if (len > 1.0f)
            len = 1.0f;
        G.x = dir.x * len + G.x;
        G.y = dir.y * len + G.y;
        G.z = dir.z * len + G.z;
    } else {
        F = a->body->m.f;
        G.x = F.x + G.x;
        G.y = F.y + G.y;
        G.z = F.z + G.z;
        if (listener != -1) {
            Agent* la = AGENT(listener);
            vector4 A = la->body->m.o;
            A.z = (la->speed_scale() - 0.1f) + A.z;
            A.x = A.x + 0.01f;
            A.y = A.y + 0.01f;
            A.x = A.x - G.x;
            A.y = A.y - G.y;
            A.z = A.z - G.z;
            A.normalize();
            float t = -(F.x * A.x + F.y * A.y + F.z * A.z);
            if (t < 1.4f) {
                vector4 B;
                B.x = A.x + F.x * t;
                B.y = A.y + F.y * t;
                B.z = A.z + F.z * t;
                B.normalize();
                vector4 Q;
                Q.x = G.x - F.x;
                Q.y = G.y - F.y;
                Q.z = G.z - F.z;
                vector4 half;
                half.x = B.x * 0.5f;
                half.y = B.y * 0.5f;
                half.z = B.z * 0.5f;
                ((vec3*)&G)->sub_assign(*(const vec3*)&half);
                F.x = G.x - Q.x;
                F.y = G.y - Q.y;
                F.z = G.z - Q.z;
                F.normalize();
            }
        }
    }

    // orthonormal frame with up = (0,0,1); the original inlined cross(), so spell it out
    vector4 up(0.0f, 0.0f, 1.0f);
    vector4 D;
    D.x = up.y * F.z - up.z * F.y;
    D.y = up.z * F.x - up.x * F.z;
    D.z = up.x * F.y - up.y * F.x;
    D.normalize();
    vector4 E;
    E.x = F.y * D.z - F.z * D.y;
    E.y = F.z * D.x - F.x * D.z;
    E.z = F.x * D.y - F.y * D.x;

    matrix* m = &this->m;
    if (sqrtf((m->o.x - G.x) * (m->o.x - G.x) + (m->o.y - G.y) * (m->o.y - G.y) +
              (m->o.z - G.z) * (m->o.z - G.z)) > 0.3f ||
        sqrtf((m->u.x - F.x) * (m->u.x - F.x) + (m->u.y - F.y) * (m->u.y - F.y) +
              (m->u.z - F.z) * (m->u.z - F.z)) > 0.1f) {
        blend = blend - g_look_rate * g_dt;
    }

    float t = clamp01(1.0f - blend);
    if (!started) {
        started = 1;
        t = t * g_dt * 1.5f;
    }
    t = clamp01(t);
    float inv = 1.0f - t;

    float d0 = D.x * t, d1 = D.y * t, d2 = D.z * t;
    m->s.x = inv * m->s.x + d0;
    m->s.y = inv * m->s.y + d1;
    m->s.z = inv * m->s.z + d2;
    float e0 = E.x * t, e1 = E.y * t, e2 = E.z * t;
    m->f.x = inv * m->f.x + e0;
    m->f.y = inv * m->f.y + e1;
    m->f.z = inv * m->f.z + e2;
    float f0 = F.x * t, f1 = F.y * t, f2 = F.z * t;
    m->u.x = inv * m->u.x + f0;
    m->u.y = inv * m->u.y + f1;
    m->u.z = inv * m->u.z + f2;
    m->o.x = inv * m->o.x + G.x * t;
    m->o.y = inv * m->o.y + G.y * t;
    m->o.z = inv * m->o.z + G.z * t;

    m->orthonormalize();
    m_fc = *m;
    float far_plane = 2.0f;
    g_mission->camera_to(m, far_plane);

    struct Pose { char b[0x120]; };
    *(Pose*)((char*)f30 + 0x1ac) = *(Pose*)((char*)this + 0x34);

    blend = blend + (g_dt + g_dt);
    blend = clamp01(blend);
}
