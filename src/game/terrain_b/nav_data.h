// nav_data.h: the ground / nav data of the scene (rgl::world::ground, +0xb0): the four-vector container with other element
// types. The same 0x68-byte object is the ground geometry of the strategic scene (strat/strat_entity.h Ground), the map store
// of the world-map path finder (src/game/party2) and the container the terrain tessellator fills (NavCore, poly_mesh.h).
#ifndef TERRAIN_B_NAV_DATA_H
#define TERRAIN_B_NAV_DATA_H
#include "../../rgl/rgl_math.h"
#include "../../rgl/mesh.h"          // rgl::vec: the game's growable array (VC7.1 vector layout)

struct face;
struct NavVert {                // 0x24 bytes (0x479980, 0x472ed0: the scene's light probes are these vertices)
    rgl::vector4 p;             // +0x00
    rgl::vector4 n;             // +0x10 accumulated normal (0x479980)
    float level;                // +0x20 accumulated brightness (0x472ed0; the f20 of 0x479980)
};
struct NavCell {                // 0x88 bytes (0x479980, 0x472650: the cell's ground plane is trans)
    int npts;                   // +0x00
    int ids[0x14];              // +0x04 vertex indices
    rgl::vector4 trans;         // +0x54 plane (normal, distance)
    float scale;                // +0x64
    int f68;                    // +0x68
    char pad6c[0x84 - 0x6c];
    face* obj;            // +0x84
};
struct NavData {                // 0x68 bytes
    rgl::vec<NavVert> verts;         // +0x00
    rgl::vec<NavCell> cells;         // +0x10
    rgl::vec<int> v20;               // +0x20
    rgl::vec<int> v30;               // +0x30
    int m40;                    // +0x40
    rgl::vector4 e44;           // +0x44
    rgl::vector4 f54;           // +0x54
    int m64;                    // +0x64
    NavData();                  // 0x478150 (same constructor as NavCore's, ICF-folded)
    ~NavData();                 // 0x41f5d0 (game/entity.cpp)
    // ---- src/game/party2: the world-map path finder (cells = the 0x88-byte formation records, v30 = its open list) ----
    bool shares(const int* a, const int* b);    // 0x48efa0 (other object)
    bool can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max);   // 0x4b3420 (ret 0x1c)
    void reset();                               // 0x4b33c0 (new search generation: m64)
    bool find_path(int from, int to, int* path, float* detail, int* count, int* visited, int* guard,
                  unsigned* mask_req, unsigned* mask_ban, char* avoid);       // 0x4b6b40 (ret 0x28)
};

#endif // TERRAIN_B_NAV_DATA_H
