// tableau.h: the tableau table for world_data.cpp (point / sun lights, 0x493f80, 0x494020) and obj_tbl.cpp (spawners,
// 0x4991d0..0x499540): the table is include/tableau_table.h, its records are E68 (include/e68.h).
#ifndef GAME_WORLD_DATA_TABLEAU_H
#define GAME_WORLD_DATA_TABLEAU_H
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../include/tableau_table.h"   // TableauTbl g_tableaus (0xa75f40)

namespace rgl { struct mesh; struct matrix; }
struct skin;

#endif // GAME_WORLD_DATA_TABLEAU_H
