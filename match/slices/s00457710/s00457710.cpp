// Slice s00457710: Agent death-state machine (names provisional). 0x457710 Agent::tick_death advances a dying agent
// through states 3 (down: hide the skeleton, drop the held item, queue it as dead, start/watch the ragdoll and remove
// the agent when it has rested) and 4 (fade the body out, then release it -> state 5); 0x457af0 Agent::note_blow reduces
// a blow with 0x454e30 and remembers the strongest one.
#include "agent_life.h"

#define TICKS_SINCE(t) ((float)(g_ticks[(t).clock] - (t).start) * 1e-5f)

// @ 0x457710
void Agent::tick_death()
{
    if (f5844 == 1 && f30 >= 0 && f10 == 1 && TICKS_SINCE(t57e0) > 3.0f) {
        g_mission->pages[(unsigned)f30 >> 4][(unsigned)f30 & 0xf].f2c = -1;
        f30 = -1;
        f5844 = 2;
    }
    if (state == 3) {
        if (f5844 != 0) {
            if (!(TICKS_SINCE(t57e0) > 3.0f))
                return;
            state = 4;
            t57e0.start = g_ticks[t57e0.clock];
            return;
        }
        if (ragdoll == 0 && queued == 0 && t57e0.elapsed() > 3.0f) {
            if (f14 != 1) {
                body->skel->set_visible(1);
                if (id != g_mission->local_id)
                    drop_held();
            }
            if (id != g_mission->local_id)
                g_mission->queue_dead(this);
            queued = 1;
        }
        if (g_tick_a > g_tick_b && TICKS_SINCE(t57e0) > 0.0f && t5918.elapsed() > 0.0f) {
            if (ragdoll == 0 && queued == 0)
                start_ragdoll();
        }
        if (ragdoll == 0)
            return;
        if ((f58d8 == 0 || !(t5918.elapsed() > 10.0f)) && !(t5918.elapsed() > 30.0f) &&
            (!(t5918.elapsed() > 1.0f) || !g_phys.none_sleeping(phys_idx)))
            return;
        on_removed();
    } else if (state == 4) {
        if (queued == 0 && f14 != 1)
            return;
        float k = 1.0f - TICKS_SINCE(t57e0) * 0.33333334f;
        if (k < 0.0f)
            k = 0.0f;
        else if (k > 1.0f)
            k = 1.0f;
        float c[4];
        c[0] = 1.0f; c[1] = 1.0f; c[2] = 1.0f; c[3] = k;
        body->set_color(pack_color(c));
        body->f294 = k;
        if (TICKS_SINCE(t57e0) > 3.0f) {
            state = 5;
            release_body();
            if (f14 == 1)
                f14 = 2;
        }
    }
}

// @ 0x457af0
void Agent::note_blow(Blow* b)
{
    reduce_blow(b);
    if (has_blow == 0 || !(b->damage_out < last_blow.damage_out)) {
        last_blow = *b;
        has_blow = 1;
    }
}
