// Slice s0044df30: Agent::ai_idle_tick (0x44df30), the per-frame timer block of the AI controller. Decays the
// "alertness" values, then runs three random-interval timers: choose_action, update_ai and a random input
// event pushed to the input queue. Names provisional; offsets from this function and s004465e0 / s0043db60.
#include <stdlib.h>
#include "rgl.h"

struct E32 { int v[8]; };                       // input record: bits (+0), time (+0x18, 64-bit); see InputRec in s0043db60

// deque<E32> at agent+0x53c0 (allocator pad, map, mapsize, offset, size)
struct InputQueue {
    int pad;
    E32** map;
    unsigned mapsize, off, size;
    void push_back(const E32& e);               // 0x43f1f0
};

struct Troop {
    char pad0[0xb0];
    int fb0;                                    // +0xb0
    char padb4[0x508 - 0xb4];
};
struct World {
    char pad0[0x13a00];
    Troop* troops;                              // +0x13a00 (stride 0x508)
};
extern World* g_world;                          // 0x5e2930
extern __int64 g_ticks[];                       // 0x5e2948: 64-bit clocks in 1e-5 s units (g_ticks[2] = 0x5e2958)
extern unsigned g_flags_5e2470;                 // 0x5e2470
extern unsigned g_flags_5e2474;                 // 0x5e2474
extern float g_dt_scale;                        // 0x5a9bf0
extern int g_int_5aa04c;                        // 0x5aa04c

struct Agent {
    char pad0[0x14];
    int f14;                                    // +0x14 (1 = alive)
    char pad18[0x88 - 0x18];
    int troop;                                  // +0x88
    char pad8c[0x290 - 0x8c];
    float f290;                                 // +0x290
    char pad294[0x2a4 - 0x294];
    float f2a4;                                 // +0x2a4
    char pad2a8[0x2b0 - 0x2a8];
    int clk_a;                                  // +0x2b0 (index into g_ticks)
    char pad2b4[4];
    __int64 last_a;                             // +0x2b8
    float int_a;                                // +0x2c0
    char pad2c4[0x2ec - 0x2c4];
    float int_b;                                // +0x2ec
    int clk_b;                                  // +0x2f0
    char pad2f4[4];
    __int64 last_b;                             // +0x2f8
    int clk_c;                                  // +0x300
    char pad304[4];
    __int64 last_c;                             // +0x308
    float int_c;                                // +0x310
    char pad314[0x53a4 - 0x314];
    unsigned in_bits;                           // +0x53a4
    char pad53a8[0x53bc - 0x53a8];
    unsigned in_cur;                            // +0x53bc
    InputQueue queue;                           // +0x53c0
    char pad53d4[0x55c4 - 0x53d4];
    int rand_seed;                              // +0x55c4
    char pad55c8[0x5928 - 0x55c8];

    void choose_action(int unused);             // 0x44d000
    void update_ai();                           // 0x4465e0
    void update_alarm();                        // 0x443140
    void ai_update_control();                   // 0x448eb0
    void ai_idle_tick(const float* dt);         // 0x44df30
};

// @ 0x44df30
void Agent::ai_idle_tick(const float* dt)
{
    int alive = f14;
    g_flags_5e2470 = 0;
    if (alive != 1) {
        in_bits = 0;
        return;
    }
    float v = f2a4 - *dt * 0.002f;
    f2a4 = v;
    if (v > 1.0f)
        f2a4 = v - *dt * 0.05f;
    if (f2a4 < 0.5f)
        f2a4 = 0.5f;
    if (f290 < 9.0f)
        f290 = f290 + *dt;
    if (int_a < (float)(g_ticks[clk_a] - last_a) * 1e-05f) {
        last_a = g_ticks[clk_a];
        int_a = (float)(rand() % 0x3dc9) * 0.00012644623f + 5.0f;
        choose_action(0);
    }
    if (int_b < (float)(g_ticks[clk_b] - last_b) * 1e-05f) {
        rand_seed = rand();
        update_ai();
        last_b = g_ticks[clk_b];
        int_b = ((float)(rand() % 0x3dc9) * 6.322312e-05f + 0.5f) * g_dt_scale;
    }
    if (int_c <= (float)(g_ticks[clk_c] - last_c) * 1e-05f) {
        update_alarm();
        g_flags_5e2474 = in_cur;
        in_bits = 0;
        in_cur = 0;
        ai_update_control();
        last_c = g_ticks[clk_c];
        E32 e;
        float r1 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        e.v[0] = in_cur;
        float hi = (float)g_int_5aa04c * 0.02f + 0.2f / (float)(g_world->troops[troop].fb0 + 10);
        int_c = (r1 * 0.03f + hi) + 0.05f;
        float r2 = (float)(rand() % 0x3dc9) * 6.322312e-05f;
        float ga = (float)g_int_5aa04c * 0.1f;
        float rb = (r2 + 1.0f) * 0.1f;
        float t = ga + rb;
        if (troop >= 0)
            t = t / ((float)g_world->troops[troop].fb0 + 30.0f) * 40.0f;
        *(__int64*)&e.v[6] = (__int64)(t * 100000.0f + g_ticks[2]);
        in_bits = in_bits | g_flags_5e2470;
        queue.push_back(e);
    }
}
