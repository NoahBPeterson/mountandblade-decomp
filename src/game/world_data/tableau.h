// tableau.h: the views of a tableau record and a tableau world the table's members use (the table itself is include/tableau_table.h).
// Shared by world_data.cpp (point / sun lights, 0x493f80, 0x494020) and obj_tbl.cpp (spawners, 0x4991d0..0x499540). Names
// provisional; offsets cite the function that proves them.
#ifndef GAME_WORLD_DATA_TABLEAU_H
#define GAME_WORLD_DATA_TABLEAU_H
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40)

namespace rgl { struct mesh; struct matrix; }
struct skin;
struct TableauWorld;
struct TableauEntity;

struct TableauRec {                                             // 0x68 bytes
    char pad0[0x28];
    float f28, f2c, f30, f34;                                   // +0x28 the quad corners (0x4991d0)
    char padtail[0x68 - 0x38];
};

#endif // GAME_WORLD_DATA_TABLEAU_H
