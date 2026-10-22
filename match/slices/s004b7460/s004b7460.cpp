// Slice s004b7460: the per-frame navigation driver of a map party.
//   0x4b7460  Party::update_nav (advance along the current path while it is being followed; otherwise pick the
//             next destination from the nav state -- current position, stored target position, the target party's
//             position, or a random jitter around it -- then make sure a path to the new target exists and
//             consume the leading path nodes that the party can already move through)
// Names provisional; layouts from this disassembly and the neighbouring slices s004b3420 (WorldMap::can_move,
// Party order fields) and s004b6b40 (Party::start_path, the path/detail arrays at +0x96c/+0x196c).
#include "rgl.h"

// ---- parties ----------------------------------------------------------------------------------------------
struct Party {
    char pad0[4];
    int agent;                                      // +0x04
    int id;                                         // +0x08
    char padc[0x30 - 0xc];
    float x;                                        // +0x30
    float y;                                        // +0x34
    char pad38[0x92c - 0x38];
    float vx;                                       // +0x92c
    float vy;                                       // +0x930
    char pad934[0x964 - 0x934];
    float f95c;                                     // +0x964 (destination x)
    float f960;                                     // +0x968 (destination y)
    int path[0x400];                                // +0x96c node-id chain
    float detail[0x400 * 4];                        // +0x196c per-node positions
    int cursor;                                     // +0x596c path cursor
    int count;                                      // +0x5970 path length
    Party* target;                                  // +0x5974 current target party
    char pad5978[0x5980 - 0x5978];
    int moving;                                     // +0x5980 following the current path
    int last_agent;                                 // +0x5984 agent of the last started path
    int needs_path;                                 // +0x5988 wants a path to `target`
    int arrived;                                    // +0x598c last start_path failed
    char pad5990[0x5998 - 0x5990];
    int state;                                      // +0x5998 0 pos, 1 stored, 2 target party, 3 jitter
    float lx;                                       // +0x599c stored destination
    float ly;                                       // +0x59a0
    Party* dest;                                    // +0x59a4 destination party
    int dest_id;                                    // +0x59a8 destination party id

    void move_along();                              // 0x4b4320
    char start_path(int from, float tx, float ty);  // 0x4b73c0
    void update_nav();                              // 0x4b7460
};

struct PartyMgr {
    char pad0[0x138d0];
    Party** pages;                                  // +0x138d0
    char pad138d4[0x138dc - 0x138d4];
    unsigned count;                                 // +0x138dc
    int is_slot(int id);                            // 0x4903e0
    Party* get(int id);                             // 0x51ee50
};

struct GameData {
    char pad0[0x78];
    int cur_party;                                  // +0x78
    char pad7c[0xf8 - 0x7c];
    PartyMgr parties;                               // +0xf8
};
extern GameData* g_game;                            // 0x5e2930

struct MapGlobal {                                  // object at 0xa732b4
    char pad0[0xb0];
    struct WorldMap* world;                         // +0xb0
};
extern MapGlobal* g_map;                            // 0xa732b4

struct WorldMap {
    char pad0[0x04];
    bool can_move(int from, float x, float y, int to, const float* pos, unsigned flags, int max);  // 0x4b3420
};

extern float frand();                               // 0x40e400 (st0)
extern float g_nav_jitter_chance;                   // 0x54b3c4 (0.35)

// @ 0x4b7460
void Party::update_nav()
{
    if (moving) {
        if (state == 2) {
            int d = dest_id;
            if (!g_game->parties.is_slot(d)) {
                state = 0;
            } else if (g_game->parties.get(d)->target != dest) {
                moving = 0;
                needs_path = 1;
            }
        }
        if (moving) {
            move_along();
            if (moving)
                return;
        }
    }

    if (state == 0) {
        f95c = x;
        f960 = y;
    } else if (state == 1) {
        f95c = lx;
        f960 = ly;
    } else if (state == 2) {
        int d = dest_id;
        if (!g_game->parties.is_slot(d)) {
            state = 0;
        } else {
            float* p = (float*)g_game->parties.get(d) + 0xc;    // the party position
            f95c = p[0];
            f960 = p[1];
            lx = p[0];
            ly = p[1];
        }
    } else if (state == 3) {
        if (!(frand() < g_nav_jitter_chance)) {
            // keep the destination of the previous frame
        } else {
            int d = dest_id;
            if (g_game->parties.is_slot(d)) {
                float* p = (float*)g_game->parties.get(d) + 0xc;
                float dx = p[0] - x;
                float dy = p[1] - y;
                if (vx * vx + vy * vy < dx * dx + dy * dy) {
                    float* q = (float*)g_game->parties.get(d) + 0xc;
                    f95c = q[0];
                    f960 = q[1];
                    f95c = vx + f95c;
                    f960 = vy + f960;
                    lx = f95c;
                    ly = f960;
                }
            }
        }
    }

    Party* tgt;
    if (state == 1) {
        tgt = dest;
    } else {
        int d = dest_id;
        if (state == 2 && g_game->parties.is_slot(d)) {
            dest = g_game->parties.get(d)->target;
            tgt = dest;
        } else if (state == 3 && g_game->parties.is_slot(d)) {
            dest = g_game->parties.get(d)->target;
            tgt = dest;
        } else {
            tgt = target;
        }
    }

    if (needs_path && tgt && target->agent != tgt->agent) {
        if (!start_path(tgt->agent, f95c, f960)) {
            arrived = 1;
            needs_path = 0;
        } else {
            arrived = 0;
            last_agent = tgt->agent;
            int max = 10;
            if (id == g_game->cur_party)
                max = 0x96;
            int progress = 0;
            while (progress < max) {
                int from = target->agent;
                if (from < 0)
                    break;
                int cur = cursor;
                if (cursor + 1 >= count)
                    break;
                float pos[4];
                pos[0] = detail[cur * 4 + 1];
                pos[1] = detail[cur * 4 + 2];
                if (!g_map->world->can_move(from, x, y, path[cur + 1], pos, 0, max))
                    break;
                cursor = cur + 1;
                progress++;
            }
            moving = 1;
            move_along();
        }
    }
}
