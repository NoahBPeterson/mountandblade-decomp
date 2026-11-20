// mission.h: the mission object (global pointer g_mission, 0x5e292c) as far as terrain_b.cpp / mission.cpp read or
// write it: the scene container pointer (+0), the agent table and record pool (+4, +0x30), the mark records (+0x6b0), the
// eight teams with their three class records (+0x5280), the mission clock and spawn-rule state (+0x5de0..), the
// per-team counters (+0x621c), the terrain description with the play-area rectangle (+0x63d0), the ambient wind and
// camera handles, the prop list (+0x6688), the weather blend state (+0x67cc..) and the spawn-done flags (+0x6824).
// The class itself (Mission, TeamRec, ClassInfo, MarkRec) is the shared src/game/mission.h; this header adds what only terrain_b
// needs around it: the pool views of the agent table and record pool, the property kinds and the layout checks.
#ifndef TERRAIN_B_MISSION_H
#define TERRAIN_B_MISSION_H
#include <stddef.h>
#include "../../include/layout32.h"
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../tstring.h"
#include "./cell_grid.h"
#include "../../include/pool.h"      // Pool<T>: the paged record pool
#include "../../include/recj.h"         // RecJ, the record of Pool<RecJ>
#include "../agent_obj.h"                  // Agent: the record of the agent table (Mission::agents)
#include "../../rgl/mesh.h"
#include "./records.h"
#include "./face.h"
#include "./game_data.h"
#include "../mission.h"                           // Mission, TeamRec, ClassInfo, MarkRec, g_mission (one definition for src/game)

struct SpawnSet;
#include "../../include/mission_obj.h"      // MissionObj: the scene prop kind (g_propkinds)


// ---- spawn rules (0x473a60) ---------------------------------------------------------------------------------------
#include "../mode_rec.h"                      // SpawnRec, ModeRec2, g_mission_templates (shared with terrain_a / agent)
int __cdecl rand_below(const int* n);           // 0x432580 (0 when *n < 2, else rand() % *n)

#include "../game_globals.h"

#include "../../include/timer.h"

// the 0xb0-byte record pool (Mission +0x30)
inline Pool<RecJ>& pool_j(Mission* m) { return m->poolJ; }

#define MISSION_OFFSET_CHECK(m, off) MB_LAYOUT32_ASSERT(Mission_##m##_at, (offsetof(Mission, m) == (off)))
MISSION_OFFSET_CHECK(agents, 0x4);
MISSION_OFFSET_CHECK(poolJ, 0x30);
MISSION_OFFSET_CHECK(recs, 0x670);
MISSION_OFFSET_CHECK(teams, 0x527c);
MISSION_OFFSET_CHECK(tm, 0x5de0);
MISSION_OFFSET_CHECK(mode, 0x5df0);
MISSION_OFFSET_CHECK(v5dfc, 0x5dfc);
MISSION_OFFSET_CHECK(list, 0x5e0c);
MISSION_OFFSET_CHECK(counts, 0x621c);
MISSION_OFFSET_CHECK(terrain, 0x63d0);
MISSION_OFFSET_CHECK(f6598, 0x6598);
MISSION_OFFSET_CHECK(wind_a, 0x65b0);
MISSION_OFFSET_CHECK(edit_obj, 0x65e4);
MISSION_OFFSET_CHECK(precip_mode, 0x6604);
MISSION_OFFSET_CHECK(rain_snd, 0x6664);
MISSION_OFFSET_CHECK(dead, 0x6678);
MISSION_OFFSET_CHECK(props, 0x6688);
MISSION_OFFSET_CHECK(marker_poly, 0x672c);
MISSION_OFFSET_CHECK(special, 0x6740);
MISSION_OFFSET_CHECK(frame_a, 0x674c);
MISSION_OFFSET_CHECK(blend_from, 0x67cc);
MISSION_OFFSET_CHECK(cam_target_agent, 0x67e8);
MISSION_OFFSET_CHECK(time_speed_anim_time, 0x6810);
MISSION_OFFSET_CHECK(str, 0x6814);
MISSION_OFFSET_CHECK(done, 0x6824);


#endif // TERRAIN_B_MISSION_H
