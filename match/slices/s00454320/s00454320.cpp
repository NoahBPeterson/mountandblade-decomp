// Slice s00454320: Agent::on_removed (tear down the agent's physics world, hide its skeleton, queue it as dead).
#include "agent_fx.h"

// @ 0x454320
void Agent::on_removed()
{
    g_phys.destroy_world(phys_idx);
    f5880 = 0;
    if (f14 != 1) {
        body->skel->set_visible(1);
        if (id != g_mission->local_id)
            drop_held();
    }
    if (!queued) {
        if (id != g_mission->local_id)
            g_mission->queue_dead(this);
        queued = 1;
    }
}
