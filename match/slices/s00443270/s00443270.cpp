// Slice s00443270: agent sound / voice helpers (0x443270..0x443a9f). Two pickers (the best agent to shout at,
// the best visible threat) and four "play a sound at the agent's body" helpers that randomise pitch. All names
// provisional; layouts are copied from the neighbours (s00440a30 ai_agent.h, s0043b5d0).
#include <stdlib.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

extern "C" char __stdcall FSOUND_StopSound(int channel);                        // 0x5342a6 (import thunk)

struct Agent;
struct AgentTable {                                     // mission+4
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14 (id >> 4 -> page of 16 agents)
    void next_used(unsigned* i);                        // 0x43b160
    int first_used();                                   // 0x43b1b0
};
struct Mission {
    char pad0[4];
    AgentTable agents;                                  // +4
    char pad28[0x5c - 0x28];
    int player_id;                                      // +0x5c
    char pad60[0x5df4 - 0x60];
    int f5df4;                                          // +0x5df4 (index into the 0x494-byte slot table of g_game)
    char pad5df8[0x654c - 0x5df8];
    float f654c;                                        // +0x654c
    char pad6550[0x6555 - 0x6550];
    bool muted;                                         // +0x6555
};
extern Mission* g_mission;                              // 0x5e292c

struct Slot494 { char pad0[0x30]; unsigned char flags; char pad31[0x494 - 0x31]; };
struct Troop508 { char pad0[0x30]; unsigned flags; char pad34[0x508 - 0x34]; };
struct GameData {
    char pad[0x13a00];
    Troop508* troops;                                   // +0x13a00
    char pad13a04[0x13a50 - 0x13a04];
    Slot494* slots;                                     // +0x13a50
};
extern GameData* g_game;                                // 0x5e2930

struct SoundSystem {                                    // object at 0x5aa128
    int play(int sound, int priority, const vector4* pos, float pitch, float* out);     // 0x442650 (thiscall, ret 0x14)
};
extern SoundSystem g_sound;                             // 0x5aa128
extern int g_sound_ids[];                               // 0x5aa178
extern __int64 g_times[];                               // 0x5e2948

struct AgentBody {
    char pad0[0x58];
    matrix m;                                           // +0x58
};
struct Threat { unsigned id; float dist; vector4 dir; };
struct Agent {
    char pad0[8];
    int id;                                             // +8
    char padc[4];
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x2c - 0x18];
    int f2c;                                            // +0x2c
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                          // +0x88
    char pad8c[0xb8 - 0x8c];
    unsigned fb8;                                       // +0xb8
    char padbc[0x1bc - 0xbc];
    int f1bc;                                           // +0x1bc
    char pad1c0[0x250 - 0x1c0];
    AgentBody* body;                                    // +0x250
    char pad254[0x53ec - 0x254];
    Threat threats[16];                                 // +0x53ec
    int nthreat;                                        // +0x556c
    char pad5570[0x55e8 - 0x5570];
    int f55e8;                                          // +0x55e8
    char pad55ec[0x5858 - 0x55ec];
    int channel;                                        // +0x5858
    char pad585c[0x5860 - 0x585c];
    int f5860;                                          // +0x5860
    char pad5864[0x5868 - 0x5864];
    __int64 stamp;                                      // +0x5868
    float f5870;                                        // +0x5870
    char pad5874[0x5928 - 0x5874];

    bool check(unsigned id, int flags, float maxdist, float* out_dist);     // 0x442ed0 (thiscall, ret 0x10)
    bool line_of_sight(unsigned id, void* ignore);                          // 0x440e30
    int fire_check(unsigned id, int* out_blocker);                          // 0x441020

    unsigned pick_shout_target(int flags, float maxdist);                   // 0x443270
    unsigned pick_threat();                                                 // 0x443430 (fastcall)
    void play_hit_sound(int* kind);                                         // 0x4435c0
    void play_sound_var(int* kind, float* var);                             // 0x443730
    void play_sound(int* snd);                                              // 0x443870
    void play_pain_sound(float health);                                     // 0x443a00
};
// time table at 0x5e2948 (int64 per entry), used by the sound-time throttle in s00444370

// @ 0x443270
unsigned Agent::pick_shout_target(int flags, float maxdist)
{
    float bestv = 0.0f;
    unsigned best = (unsigned)-1;
    int limit = *(int*)&maxdist;
    AgentTable* tbl = &g_mission->agents;
    unsigned i = tbl->first_used();
    if (i < (unsigned)tbl->count) {
        do {
            unsigned id = i;
            bool ok = check(id, flags, *(float*)&limit, &maxdist);
            Mission* m = g_mission;
            if (ok) {
                float x = 3.0f / (maxdist + 2.0f);
                float t = (x < 1.0f) ? x : 1.0f;
                float k = 1.0f;
                float w = 1.0f / ((float)m->agents.pages[id >> 4][id & 0xf].f55e8 + 7.0f);
                if (f2c >= 0 && maxdist < 40.0f) {
                    vector4 d;
                    const vector4& op = m->agents.pages[id >> 4][id & 0xf].pos;
                    d.x = op.x - pos.x;
                    d.y = op.y - pos.y;
                    d.z = op.z - pos.z;
                    maxdist = d.normalize();
                    const vector4& fw = body->m.f;
                    float dp = d.y * fw.y + d.z * fw.z + d.x * fw.x;
                    if (dp > 0.0f)
                        k = ((40.0f - maxdist) * dp) * dp + 1.0f;
                }
                float s = (k * w) * t;
                if (s > bestv) { bestv = s; best = id; }
            }
            tbl = &m->agents;
            tbl->next_used(&i);
        } while (i < (unsigned)tbl->count);
        return best;
    }
    return best;
}

// @ 0x443430
unsigned Agent::pick_threat()
{
    float score;
    unsigned best = (unsigned)-1;
    float bestv = -100.0f;
    unsigned id = (unsigned)-1;
    int n = 0;
    if (0 < nthreat) {
        Threat* r = threats;
        do {
            id = r->id;
            Agent* o = &g_mission->agents.pages[id >> 4][id & 0xf];
            if (o->f14 == 1 && line_of_sight(id, 0)) {
                if (fire_check(id, (int*)&score) == 1) {
                    const vector4& op = g_mission->agents.pages[id >> 4][id & 0xf].pos;
                    float dx = op.x - pos.x;
                    float dy = op.y - pos.y;
                    float dz = op.z - pos.z;
                    float v = 3.0f / (float)sqrt(dy * dy + (dx * dx + dz * dz));
                    if (!(v < 4.0f)) v = 4.0f;
                    score = v / ((float)g_mission->agents.pages[r->id >> 4][r->id & 0xf].f55e8 + 7.0f);
                    float s = ((float)(rand() % 0x3dc9) * 1.2644623e-05f + 1.0f) * score;
                    if (s > bestv) { bestv = s; best = r->id; }
                }
            }
            n++;
            r++;
            id = best;
        } while (n < nthreat);
    }
    return id;
}

// @ 0x4435c0
void Agent::play_hit_sound(int* kind)
{
    Mission* m = g_mission;
    if (m->muted)
        return;
    int snd;
    if (f10 == 0) {
        int k = *kind;
        if (k < 0)
            return;
        if (k > 1) {
            if (k != 3)
                return;
            snd = 0xe;
        } else if (!(pos.z < m->f654c)) {
            snd = (g_game->slots[m->f5df4].flags & 1) ? 0x10 : 0xf;
        } else {
            snd = 0x11;
        }
    } else {
        int k = *kind;
        unsigned c = fb8 & 7;
        if (k == 4) {
            snd = 0x61;
        } else if (pos.z > m->f654c) {
            if (c == 2 || c == 0) {
                if (k != 0 && k != 2) return;
                snd = 0x67;
            } else if (c == 3) {
                if (k != 0 && k != 2) return;
                snd = 0x68;
            } else if (c == 4) {
                if (k != 0) return;
                snd = 0x69;
            } else {
                if (c != 5) return;
                if (k != 0) return;
                snd = 0x6a;
            }
        } else {
            if (k != 2 && k != 3) return;
            snd = 0x11;
        }
    }
    int r = rand();
    float pitch = (float)(r % 0x3dc9) * 2.5289246e-05f + 0.8f;
    g_sound.play(g_sound_ids[snd], 0, &body->m.o, pitch, 0);
}

// @ 0x443730
void Agent::play_sound_var(int* kind, float* var)
{
    if (*kind != -1 && body != 0) {
        float pitch = 1.0f;
        if (*var > 0.001f) {
            float v = ((float)(rand() % 0x3dc9) * 6.322312e-05f - 0.5f) * *var;
            v = v + v;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f)
                { float nv = -v; pitch = 1.0f / (nv + 1.0f); }
        }
        int own = 0;
        if (id == g_mission->player_id)
            own = 1;
        vector4 p = body->m.o;
        float h = (f10 == 0 && f2c != -1) ? 2.4f : 1.5f;
        p.z = (h - 0.3f) + p.z;
        g_sound.play(g_sound_ids[*kind], own, &p, pitch, &pitch);
    }
}

// @ 0x443870
void Agent::play_sound(int* snd)
{
    if (*snd != -1 && body != 0) {
        if (channel >= 0) {
            FSOUND_StopSound(channel);
            channel = -1;
        }
        float pitch = 1.0f;
        Mission* m = g_mission;
        if (f10 == 0 && (g_game->troops[troop].flags & 0xf) != 1 && id != m->player_id) {
            float v = ((float)(((f1bc * 0x244ad + (id + 0x1d) * 0x14fd + troop * 0x76ef) % 0x1b41) * 0x51f % 0x367)
                       * 0.0011481056f - 0.5f) * 0.3f;
            if (v > 0.001f)
                pitch = v + 1.0f;
            else if (v < -0.001f) {
                float nv = -v;
                pitch = 1.0f / (nv + 1.0f);
            }
        }
        int own = 0;
        if (id == m->player_id)
            own = 1;
        int ch = g_sound.play(*snd, own, &body->m.o, pitch, &pitch);
        if (ch >= 0) {
            channel = ch;
            stamp = g_times[f5860];
            f5870 = pitch;
        }
    }
}

// @ 0x443a00
void Agent::play_pain_sound(float health)
{
    health = (1.2f - health) * 0.3f;
    float v = (health < 0.0f) ? 0.0f : ((health >= 0.5f) ? 0.5f : health);
    int r = rand();
    float pitch = (float)(r % 0x3dc9) * 6.3223115e-06f + (v + 0.95f);
    g_sound.play(g_sound_ids[0x11], 0, &body->m.o, pitch, 0);
}
