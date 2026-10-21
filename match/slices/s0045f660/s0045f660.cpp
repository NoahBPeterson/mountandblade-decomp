// Slice s0045f660: Agent::update, the per-frame agent tick (0x45f660, 2.2 KB). Skips dead agents (state 5), turns
// the two animation channels' phases into blend scales (curve types linear / sine / square / square root), plays
// the footstep sounds on the animation's event times, spawns the dust / splash emitters (rider and mount feet),
// then runs the AI / input poll, the attack and movement updates and finally eases the aim-error vector back to
// zero. Names provisional.
#include <math.h>
#include <vector>
#include "rgl.h"
using rgl::vector4;
using rgl::matrix;

struct pos : matrix {};
pos* __cdecl identity_pos(pos* out);                    // 0x40cd00 (returns out)
float __cdecl frand();                                  // 0x40e400

struct vec2 {
    float x, y;
    float normalize();                                  // 0x4367c0 (returns the old length)
};

struct SubAnim {                                        // 0x40 bytes
    char pad0[8];
    unsigned flags;                                     // +8 (0x100 footstep dust, 0x200 sound, 0x400 ...)
    char padc[0x1c - 0xc];
    float ev[4];                                        // +0x1c event times (phase)
    char pad2c[0x40 - 0x2c];
};
struct AnimDef {                                        // 0x30 bytes
    char pad0[0x10];
    SubAnim* subs;                                      // +0x10
    char pad14[0x30 - 0x14];
};
extern AnimDef* g_anim_defs;                            // 0xa76628

struct Channel {                                        // 0xd8 bytes
    char pad0[0xb4];
    int curve;                                          // +0xb4 (1 linear, 2 sine, 4 square, 5 square root)
    char padb8[0xd8 - 0xb8];
    float phase();                                      // 0x4231f0
};
struct Skeleton {
    char pad0[0x40];
    Channel* chans;                                     // +0x40
};
struct AgentBody {
    char pad0[0x88];
    vector4 spos;                                       // +0x88 (sound position)
    char pad98[0x304 - 0x98];
    Skeleton* skel;                                     // +0x304
};

struct AnimSlot {                                       // 0x2c bytes (see s0043d040)
    int cur;                                            // +0
    int next;                                           // +4
    int sub;                                            // +8
    unsigned prev_mask;                                 // +0xc
    int f10;                                            // +0x10
    unsigned events;                                    // +0x14
    float scale;                                        // +0x18
    int f1c;                                            // +0x1c
    float rate;                                         // +0x20
    float phase;                                        // +0x24
    int f28;                                            // +0x28
};

struct Floor { char pad0[8]; unsigned flags; };         // ground face (flags +8)

struct Emitter {                                        // effect object (see s004333c0 Obj3)
    char pad0[0x134];
    matrix m134;                                        // +0x134
    char pad174[0x1a8 - 0x174];
    int f1a8;                                           // +0x1a8 (effect kind)
    void reset_pending(int v);                          // 0x433810
};
extern int g_default_fx;                                // 0x8ed50a8

struct SoundMgr {
    int play(int sound, int priority, const vector4* pos, float pitch, float* out);   // 0x442650 (thiscall, ret 0x14)
};
extern SoundMgr g_sounds;                               // 0x5aa128
extern int g_snd_224;                                   // 0x5aa224 (footstep sounds)
extern int g_snd_228;                                   // 0x5aa228
extern int g_snd_22c;                                   // 0x5aa22c
extern int g_snd_230;                                   // 0x5aa230
extern int g_snd_2e0;                                   // 0x5aa2e0

struct Agent;
struct Mission {
    char pad0[0x18];
    std::vector<Agent*> vp;                             // +0x18 (first at +0x1c; id >> 4 -> page of 16 agents)
    char pad28[0x654c - 0x28];
    float water_z;                                      // +0x654c
};
extern Mission* g_mission;                              // 0x5e292c

struct InputState { unsigned v[6]; };

struct Agent {                                          // 0x5928 bytes
    char pad0[0x10];
    int f10;                                            // +0x10 (0 human, 1 horse)
    char pad14[0x2c - 0x14];
    int f2c;                                            // +0x2c
    int f30;                                            // +0x30
    char pad34[0x58 - 0x34];
    float z;                                            // +0x58
    char pad5c[0x7c - 0x5c];
    int f7c;                                            // +0x7c
    char pad80[0x98 - 0x80];
    float aim[3];                                       // +0x98 aim error / head sway
    char padA4[0xa8 - 0xa4];
    float fa8, fac;                                     // +0xa8 ground velocity xy
    char padb0[0xbc - 0xb0];
    Floor* floor;                                       // +0xbc
    char padc0[0xd4 - 0xc0];
    AnimSlot slot[2];                                   // +0xd4
    AnimDef** lib;                                      // +0x12c
    char pad130[0x1ac - 0x130];
    Emitter* fx1ac;                                     // +0x1ac
    Emitter* fx1b0;                                     // +0x1b0
    Emitter* fx1b4;                                     // +0x1b4
    Emitter* fx1b8;                                     // +0x1b8
    char pad1bc[0x250 - 0x1bc];
    AgentBody* body;                                    // +0x250
    char pad254[0x55d0 - 0x254];
    InputState in_cur;                                  // +0x55d0
    char pad55e8[0x56e0 - 0x55e8];
    float f56e0, f56e4;                                 // +0x56e0 (previous / current scale of slot 0)
    char pad56e8[0x5830 - 0x56e8];
    int state;                                          // +0x5830
    char pad5834[0x5878 - 0x5834];
    char need_name;                                     // +0x5878
    char pad5879[0x5880 - 0x5879];
    int ragdoll;                                        // +0x5880
    char pad5884[0x5928 - 0x5884];

    void draw_name();                                   // 0x4552c0
    void play_pain_sound(float depth);                  // 0x443a00
    void play_hit_sound(int* kind);                     // 0x4435c0
    void smooth(float dt);                              // 0x438860 (camera_smooth_range_value)
    void ai_idle_tick(const float* dt);                 // 0x44df30
    void poll_input(int unused, InputState* out);       // 0x43c9c0 (ret 8)
    void update_x(float dt);                            // 0x457b40
    void update_controls(float dt);                     // 0x4556d0
    void update_attack(float dt);                       // 0x45c220
    void move_step(float dt);                           // 0x45d820
    void update_pitch(float dt);                        // 0x436130
    void update_aim_error();                            // 0x43bdb0
    void update(float dt);                              // 0x45f660
};

// @ 0x45f660
void Agent::update(float dt)
{
    if (state == 5)
        return;
    if (need_name) {
        draw_name();
        need_name = 0;
    }
    {
    float old = slot[1].scale;
    for (int i = 0; i < 2; i++) {
        Channel* c = &body->skel->chans[i];
        float p = c->phase();
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
        slot[i].scale = p;
    }
    float now = slot[1].scale;
    if (slot[1].cur >= 0 && slot[1].sub >= 0 && ((*lib)[slot[1].cur].subs[slot[1].sub].flags & 0x200)) {
        for (int k = 0; k < 4; k++) {
            if (old <= g_anim_defs[slot[1].cur].subs[slot[1].sub].ev[k] &&
                g_anim_defs[slot[1].cur].subs[slot[1].sub].ev[k] < now) {
                if (k == 0) {
                    float pitch;
                    switch (slot[1].cur) {
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sounds.play(g_snd_230, 0, &body->spos, pitch, 0);
                        break;
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sounds.play(g_snd_224, 0, &body->spos, pitch, 0);
                        break;
                    }
                } else if (k == 1) {
                    float pitch;
                    switch (slot[1].cur) {
                    case 0x75:
                    case 0x77:
                        pitch = frand() * 0.2f + 0.9f;
                        g_sounds.play(g_snd_22c, 0, &body->spos, pitch, 0);
                        break;
                    case 0x7b:
                    case 0x7c:
                        pitch = frand() * 0.4f + 0.8f;
                        g_sounds.play(g_snd_228, 0, &body->spos, pitch, 0);
                        break;
                    }
                }
            }
        }
    }
    }
    f56e4 = f56e0;
    f56e0 = slot[0].scale;
    float speed = (float)sqrt(fac * fac + fa8 * fa8);
    if (slot[0].cur >= 0) {
        unsigned flags = (*lib)[slot[0].cur].subs[slot[0].sub].flags;
        if (ragdoll == 0 && (flags & 0x300)) {
            float pitch;
            int k = 0;
            int kind = 0;
            do {
                if (f56e4 <= g_anim_defs[slot[0].cur].subs[slot[0].sub].ev[k] &&
                    g_anim_defs[slot[0].cur].subs[slot[0].sub].ev[k] < f56e0) {
                    if ((flags & 0x200) && k == 0) {
                        switch (slot[0].cur) {
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
                            g_sounds.play(g_snd_2e0, 0, &body->spos, pitch, 0);
                        }
                        }
                    }
                    if (flags & 0x100) {
                        if (z < g_mission->water_z) {
                            if (f10 == 0 || k == 3 || k == 4)
                                play_pain_sound(g_mission->water_z - z);
                        } else {
                            play_hit_sound(&kind);
                        }
                        if (fx1b0 && speed > 0.0f) {
                            if (z < g_mission->water_z) {
                                float s = 0.2f;
                                if (f10 == 1)
                                    s = 0.9f;
                                pos m;
                                identity_pos(&m);
                                vec2 d;
                                d.x = fa8;
                                d.y = fac;
                                d.normalize();
                                vec2 t;
                                t.x = d.x * s;
                                t.y = d.y * s;
                                m.f.x = fa8;
                                m.f.y = fac;
                                m.f.z = 0.0f;
                                m.o.x = t.x;
                                m.o.y = t.y;
                                m.f.normalize();
                                m.orthonormalize();
                                fx1b0->m134 = m;
                                fx1b4->m134 = m;
                                float v;
                                if (f10 == 1)
                                    v = speed * 10.0f + 20.0f;
                                else
                                    v = speed * 3.0f + 5.0f;
                                fx1b0->reset_pending((int)(v * 0.5f));
                                fx1b4->reset_pending((int)(v * 0.4f));
                                float ux = d.x * 0.7f;
                                float uy = d.y * 0.7f;
                                m.o.x = ux + m.o.x;
                                m.o.y = m.o.y + uy;
                                fx1b8->m134 = m;
                                fx1b8->reset_pending(1);
                            } else if (fx1ac && f10 == 1) {
                                float v = 0.0f;
                                if (speed > 5.5f && floor && (floor->flags & 0x4000))
                                    v = speed + 10.0f;
                                if (fx1ac->f1a8 != g_default_fx)
                                    v = v * 3.0f;
                                fx1ac->reset_pending((int)(v * 0.15f));
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
    smooth(dt0);
    if (f30 == -1) {
        if (f7c != 2) {
            ai_idle_tick(&dt);
            poll_input(*(int*)&dt0, &in_cur);
        }
        if (f2c >= 0) {
            unsigned id = f2c;
            Agent* o = g_mission->vp[id >> 4] + (id & 0xf);
            o->in_cur = in_cur;
        }
    }
    if (dt > 0.0f) {
        update_x(dt0);
        update_controls(dt0);
        update_attack(dt0);
        if (f2c == -1)
            move_step(dt0);
    }
    update_pitch(dt0);
    int j = 0;
    do {
        if (aim[j] < -0.3f)
            aim[j] = -0.3f;
        else if (aim[j] > 0.3f)
            aim[j] = 0.3f;
        if (aim[j] < -0.001f) {
            if (j == 2)
                aim[2] = (((aim[2] - 0.7f) * dt) * 5.0f + 1.0f) * aim[2];
            else
                aim[j] = (1.0f - aim[j]) * dt + aim[j];
            if (aim[j] > 0.0f) {
            zero:
                aim[j] = 0.0f;
            }
        } else if (!(aim[j] > 0.001f)) {
            goto zero;
        } else {
            aim[j] = aim[j] - (aim[j] + 1.0f) * dt;
            if (aim[j] < 0.0f)
                goto zero;
        }
        j++;
    } while (j < 3);
    update_aim_error();
}
