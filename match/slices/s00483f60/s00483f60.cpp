// Slice s00483f60. One member of the mission object (`Game`, the object slices s004832b0/s00483870 work on,
// reached as [0x5e2930] + 0x14014):
//
//   0x483f60  Game::frame(float dt, bool spawn)   the per-frame mission tick: the particle-system water upkeep,
//                                                 the 100 kHz frame clock, the "carried items are frozen" sweep,
//                                                 the spawn pass, the wave sound slots, the prop table (per-entry
//                                                 update + retire, list overflow cull), the prop tweens, three agent
//                                                 passes (non-horses, horses, ticks), the mission timers/sky clock
//                                                 and finally world::step + the frame counter.
//
// ALL NAMES PROVISIONAL; offsets come from the disassembly. Flags: default /O2 /MT /Gy /TP.
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;
using rgl::matrix;


// ---- the 0x5928-byte agent record -------------------------------------------------------------------------
struct Agent {
    char pad0[4];
    int used;                           // +0x04 (0 = slot free)
    int id;                             // +0x08
    char pad0c[4];
    int kind;                           // +0x10 (0 human, 1 horse)
    char pad14[0x5928 - 0x14];
    void update(float dt);              // 0x45f660 (thiscall)
    void tick();                        // 0x45ff30 (thiscall)
    void next_used(unsigned* i);        // 0x43b160 (thiscall)
};

// ---- the 0xb0-byte prop record of the table at Game+0x30 ------------------------------------------------
struct PropRec {
    char pad0[4];
    int used;                           // +0x04 (0 = the prop retires this frame)
    char pad8[0x28 - 8];
    float f28;                          // +0x28 (a life value; < -100 frees the prop)
    char pad2c[0x50 - 0x2c];
    char* f50;                          // +0x50 (the prop's entity; 0 = none)
    char pad54[0x68 - 0x54];
    int state;                          // +0x68 (1/2/3 age out)
    char pad6c[0x70 - 0x6c];
    int clock;                          // +0x70 (index into g_ticks)
    char pad74[4];
    long long start;                    // +0x78
    char pad80[0xb0 - 0x80];
    void update(float dt);              // 0x47d3e0 (thiscall, ret 4)
    void next_used(unsigned* i);        // 0x474300 (thiscall)
};

// ---- the paged pointer table (count; the page vector's first member sits at +0x18) -----------------------
struct PtrTable {
    int count;                          // +0
    char pad4[0x14];
    char** pages;                       // +0x18
    char pad1c[4];
};
typedef char assert_ptrtable[(sizeof(PtrTable) == 0x20) ? 1 : -1];

// ---- the 0x1c8-byte prop tween record -------------------------------------------------------------------
struct PropAnim {
    char pad0[0x1c8];
    void update(float dt);              // 0x467c50 (thiscall, ret 4)
};

// ---- the carried-item records copied by the frame&0xf sweep (0x130 bytes, 0x40 of them at Game+0x58) -----
struct CarryRec {
    matrix m0;                          // +0x00
    matrix m1;                          // +0x40
    matrix m2;                          // +0x80
    int f_c0;                           // +0xc0 (zeroed)
    int f_c4;                           // +0xc4 (zeroed)
    char padc8[0xe8 - 0xc8];
    int f_e8;                           // +0xe8 (zeroed)
    char padec[0x130 - 0xec];
};

// ---- the 0x58-byte wave/sound set at [0x5e2a00] --------------------------------------------------------
struct SndSlot {                        // a 0x78-byte sound slot
    char pad0[0x78];
    void update(int n);                 // 0x4625d0 (thiscall, ret 4)
};
struct SndSet {
    char pad0[0x4c];
    int count;                          // +0x4c
    SndSlot* slots;                     // +0x50
};

// ---- the sky record at Game+0x65a8 ---------------------------------------------------------------------
struct Sky {
    char pad0[0x48];
    float f48;                          // +0x48 (the mission clock in seconds)
    char pad4c[0x1f8 - 0x4c];
    float f1f8;                         // +0x1f8 (the sky time)
    char pad1fc[0x208 - 0x1fc];
    float f208;                         // +0x208 (the sky time again)
};

// ---- the world record and the mission object ------------------------------------------------------------
struct World {
    void step(float dt);                // 0x427f40
    void remove(int id);                // 0x41f0d0
};

struct Game {
    World* world;                       // +0x00
    PtrTable agents;                    // +0x04 (Agent records, pages at +0x1c)
    char pad2c[0x30 - 0x24];
    PtrTable props;                     // +0x30 (PropRec records, pages at +0x48)
    char pad48[0x6b0 - 0x50];
    CarryRec carry[0x40];               // +0x6b0 (0x40 records of 0x130, ends +0x52b0)
    char pad1b0[0x5df0 - 0x52b0];
    int f5db4;                          // +0x5df0 (the sound/wave set index)
    int scene_idx;                      // +0x5df4 (index into the scene table at g_game+0x13a50)
    char pad5df8[0x5dfc - 0x5df8];
    std::vector<char*> retire_list;     // +0x5dfc (props retired this frame; culled over 0x300)
    std::vector<int> cull_list;         // +0x5e0c
    char pad5e1c[0x63c0 - 0x5e1c];
    int water;                          // +0x63c0 (the water record passed to 0x4814a0)
    char pad63c4[0x6550 - 0x63c4];
    unsigned frame_no;                  // +0x6550 (the frame count)
    char pad6554[0x65a8 - 0x6554];
    Sky* sky;                           // +0x65a8
    float sky_time;                     // +0x65ac (advanced with dt*0.04f, wrapped at 100)
    char pad65b0[0x668c - 0x65b0];
    std::vector<PropAnim> anims;        // +0x668c

    void run_spawns();                  // 0x483eb0
    void frame(float dt, bool spawn);   // 0x483f60
};

// ---- externals ---------------------------------------------------------------------------------------------
extern Game* g_game;                    // 0x5e2930
extern long long g_ticks[];             // 0x5e2948 (100 kHz clocks)
extern long long g_tick_cur;            // 0x5e2980 (the last frame's tick count)
extern long long g_tick_sum;            // 0x5e2958 (the accumulated tick count)
extern char* g_snd_sets;                // 0x5e2a00 (the wave/sound set table)
extern char g_paused;                   // 0x5e2a09
extern float g_paused_time;             // 0x5e2a0c
extern float g_blend_rate;              // 0x5a9c28
struct psys_rec {                       // 0xa70248: the particle system (a scene record at +0x48)
    char pad0[0x48];
    char* scene;
};
extern psys_rec* g_psys;                // 0xa70248
extern int g_psys_scene;                // 0xa170c0
extern char g_water_obj[];              // 0xa76630 (the terrain object 0x4814a0 is called on)
extern char g_time_scale[];             // 0xa181f8 (the time-scale object 0x472dc0 is called on)

struct PsysHook { void reset(); };      // 0x47fef0 (ecx = &g_psys)
struct TerrainObj { void destroy_water(int* w); };   // 0x4814a0
struct TimeScale { void apply(float dt); };          // 0x472dc0

void __fastcall g_f472050(void* g);      // 0x472050 (thiscall on the mission)
void __fastcall g_f474e60(void* g);      // 0x474e60
void __fastcall g_f4759c0(void* g);      // 0x4759c0

extern "C" int _rand();                 // 0x536a9f

// @ 0x483f60
void Game::frame(float dt, bool spawn)
{
    psys_rec* ps = g_psys;
    bool scene_changed = ps != 0 && *(int*)(ps->scene + 0x50) != g_psys_scene;
    if (scene_changed) {
        unsigned flags = *(unsigned*)(*(char**)((char*)g_game + 0x13a50) + scene_idx * 0x494 + 0x30);
        int* water = &this->water;
        if (flags & 0x100) {
            ((TerrainObj*)g_water_obj)->destroy_water(water);
        } else if (!(flags & 1)) {
            ((PsysHook*)ps)->reset();
        }
    }

    if (!g_paused && dt > 0.0f)
        ((TimeScale*)g_time_scale)->apply(dt);

    long long t = (long long)(dt * 100000.0f);
    g_tick_cur = t;
    g_tick_sum += t;
    g_blend_rate = dt;

    // every sixteenth frame the carried-item records restart from their first matrix
    if ((frame_no & 0xf) == 1) {
        CarryRec* c = carry;
        int n = 0x40;
        do {
            rgl::normalize(c->m0.s, c->m1.s);
            c->f_c4 = 0;
            c->f_c0 = 0;
            c->f_e8 = 0;
            rgl::normalize(c->m2.s, c->m1.s);
            c++;
        } while (--n);
    }

    if (spawn)
        run_spawns();

    if (frame_no != 0) {
        char* set = g_snd_sets + f5db4 * 0x58;
        int n = *(int*)(set + 0x4c);
        char* slot = *(char**)(set + 0x50);
        for (int i = 0; i < n; i++, slot += 0x78)
            ((SndSlot*)slot)->update(0x10);
    }

    // the prop table: per-entry update, retire the finished ones, and steady the used slots
    unsigned i;
    for (i = 0; i < (unsigned)props.count; i++)
        if (((PropRec*)props.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)props.count) {
        PropRec* p = (PropRec*)props.pages[i >> 4] + (i & 0xf);
        p->update(dt);
        bool dead = false;
        if (p->state == 2 || p->state == 3 || p->state == 1)
            dead = (g_ticks[p->clock] - p->start) * 1e-5f > 1.0f;
        if (!dead && p->f28 < -100.0f)
            dead = true;
        if (dead) {
            if (p->f50 != 0) {
                unsigned n = (unsigned)retire_list.size();
                if (n < retire_list.capacity())
                    retire_list.push_back(p->f50);
                else
                    retire_list.insert(retire_list.end(), p->f50);
            }
            p->used = 0;
            unsigned n2 = (unsigned)cull_list.size();
            if (n2 < cull_list.capacity())
                cull_list.push_back((int)i);
            else {
                int idx = (int)i;
                cull_list.insert(cull_list.end(), idx);
            }
        }
        p->next_used(&i);
    }

    // the retire list overflows: drop a random prop and compact
    if (retire_list.size() > 0x300) {
        unsigned n = (unsigned)retire_list.size();
        unsigned k = n > 1 ? (unsigned)_rand() % n : 0;
        int id = *(int*)(retire_list[k] + 8);
        world->remove(id);
        unsigned last = (unsigned)retire_list.size();
        retire_list[k] = retire_list[last - 1];
        retire_list.resize(last - 1);
    }

    for (unsigned k = 0; k < (unsigned)anims.size(); k++)
        anims[k].update(dt);

    // agents: everything but the horses, then the horses, then every agent's tick
    for (i = 0; i < (unsigned)agents.count; i++)
        if (((Agent*)agents.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)agents.count) {
        Agent* a = (Agent*)agents.pages[i >> 4] + (i & 0xf);
        if (a->kind != 1)
            a->update(dt);
        a->next_used(&i);
    }
    for (i = 0; i < (unsigned)agents.count; i++)
        if (((Agent*)agents.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)agents.count) {
        Agent* a = (Agent*)agents.pages[i >> 4] + (i & 0xf);
        if (a->kind == 1)
            a->update(dt);
        a->next_used(&i);
    }
    for (i = 0; i < (unsigned)agents.count; i++)
        if (((Agent*)agents.pages[i >> 4])[i & 0xf].used != 0)
            break;
    while (i < (unsigned)agents.count) {
        Agent* a = (Agent*)agents.pages[i >> 4] + (i & 0xf);
        a->tick();
        a->next_used(&i);
    }

    g_f472050(this);

    // the sky clock and the mission timers
    if (sky != 0) {
        sky_time = sky_time + dt * 0.04f;
        if (sky_time > 100.0f)
            sky_time = (float)fmod((double)sky_time, 100.0);
        sky->f1f8 = sky_time;
        sky->f208 = sky_time;
        sky->f48 = (float)(g_tick_sum % 1000000000) * 7.5e-6f;
    }

    g_f474e60(this);
    world->step(dt);
    g_f4759c0(this);
    frame_no++;
    if (g_paused)
        g_paused_time += dt;
}
