// tableau.h: the tableau table (g_tableaus, global 0xa75f40, the slices' obj_tbl): the 0x68-byte tableau records at +4
// and the vector of scene worlds the tableaus are rendered into at +0x10 (first at +0x14). Shared by world_data.cpp
// (point / sun lights, 0x493f80, 0x494020) and obj_tbl.cpp (spawners, 0x4991d0..0x499540). Names provisional; offsets cite
// the function that proves them.
#ifndef GAME_WORLD_DATA_TABLEAU_H
#define GAME_WORLD_DATA_TABLEAU_H
#include <vector>
#include "../../rgl/rgl_math.h"

namespace rgl { struct mesh; struct matrix; }
struct skin;
struct TableauWorld;
struct TableauEntity;

struct TableauRec {                                             // 0x68 bytes
    char pad0[0x28];
    float f28, f2c, f30, f34;                                   // +0x28 the quad corners (0x4991d0)
    char padtail[0x68 - 0x38];
};
struct NodeAlloc { void* node_478f30(); };                      // 0x478f30 (thiscall: a new light node of a world)
struct TableauTbl {
    void* f0;                                                   // +0x00
    TableauRec* recs;                                           // +0x04 (0x4991d0)
    char f8[0x10 - 0x8];
    std::vector<TableauWorld*> worlds;                          // +0x10 (first at +0x14)
    void add_point_light(const rgl::matrix* p, const rgl::vector4& c);   // 0x493f80 (thiscall, ret 8)
    void add_sun_light(const rgl::matrix* p, const rgl::vector4& c);     // 0x494020 (thiscall, ret 8)
    skin* make(const int* index, const int* from);         // 0x4b14b0 (thiscall, ret 8)
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (thiscall, ret 8)
    void spawn(int index, int from, const rgl::matrix* frame, const rgl::vector4* scale);   // 0x4992b0 (ret 0x10)
    void spawn_scaled(int index, const rgl::matrix* frame, const rgl::vector4* scale, const float* alpha, const unsigned* color);   // 0x499390 (ret 0x14)
    void spawn_scaled2(int index, const rgl::matrix* frame, const float* scale);   // 0x499540 (ret 0xc)
};
extern TableauTbl g_tableaus;                                   // 0xa75f40

#endif // GAME_WORLD_DATA_TABLEAU_H
