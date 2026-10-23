// cell.h: one cell (0x34 bytes) of the scene's collision grid (world.cells, +0x28; rows of cells), the game's view of
// rgl::cell (src/rgl/scene/world.h, which types the +0 vector with the scene's own element and adds the node list).
// ONE definition for src/game: agent2/agent_stats.cpp (0x4396b0, 0x439780), agent3/agent_fx_sound.cpp (0x442180) and
// strat/strat_entity.cpp (0x426500) each use a part of it.
#ifndef GAME_CELL_H
#define GAME_CELL_H
#include <vector>

struct shape_base;
struct EntView;
struct enode { enode* next; int pad4; EntView* ent; };      // list node of the entities in a cell (0x442180: ent at +8)
struct cell {                                           // 0x34 bytes
    std::vector<shape_base*> shapes;                    // +0x00 the shapes in the cell (0x4396b0, 0x439780)
    char pad10[0x30 - 0x10];
    enode* ents;                                        // +0x30 head of the entity list (0x442180)
    void collect_masked(unsigned mask, shape_base** out, int* count);                                     // 0x4396b0
    void collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count);          // 0x439780
    void collect(const float* bmin, const float* bmax, int* flat, unsigned mask, shape_base** out, int* count);  // 0x426500
};

#endif // GAME_CELL_H
