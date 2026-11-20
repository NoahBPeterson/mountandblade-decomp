// nav_data.h: the ground / nav data of the scene (rgl::world::ground, +0xb0): the four-vector container with other element
// types. The same 0x68-byte object is the ground geometry of the strategic scene (strat/strat_entity.h Ground), the map store
// of the world-map path finder (src/game/party2) and the container the terrain tessellator fills (NavCore, poly_mesh.h).
#ifndef TERRAIN_B_NAV_DATA_H
#define TERRAIN_B_NAV_DATA_H
#include "../../rgl/rgl_math.h"
#include <vector>
#include "../../rgl/mesh.h"          // (kept for the includers that reach rgl::mesh / rgl::vec through it)

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
    int gen;                    // +0x6c search generation of the world-map path finder (compared with NavData::m64; 0x4b6b40)
    int parent;                 // +0x70 the record the search came from (0x4b6b40)
    float pos[3];               // +0x74 x, y, z of the record (0x4b6b40)
    float limit;                // +0x80 keep distance of the node (0x4b6b40)
    face* obj;            // +0x84
    // ---- the world map's path finder reads the +4 block field by field (game/party2) ----
    int* verts() { return &ids[0]; }                    // +0x04 vertex indices of the edges (4)   0x4b3420
    int* agents() { return &ids[4]; }                   // +0x14 neighbouring record per edge        0x4b3420
    unsigned& mask() { return *(unsigned*)&ids[8]; }    // +0x24 terrain mask (low 3 bits: kind)    0x4b2770, 0x4b2880, 0x4b3420
    int* targets() { return &ids[9]; }                  // +0x28 record reached through the edge     0x4b3420, 0x4b6b40
    float* dists() { return (float*)&ids[13]; }         // +0x38 keep distance per edge              0x4b3420, 0x4b6b40
    rgl::vector4& pos4() { return *(rgl::vector4*)pos; } // pos and limit as one 16-byte vector (0x4b6b40 copies them together)
};
struct NavData {                // 0x68 bytes
    std::vector<NavVert> verts;      // +0x00
    std::vector<NavCell> cells;      // +0x10
    std::vector<int> v20;            // +0x20
    std::vector<int> v30;            // +0x30
    int m40;                    // +0x40
    rgl::vector4 e44;           // +0x44
    rgl::vector4 f54;           // +0x54
    int m64;                    // +0x64
    NavData();                  // 0x478150 (same constructor as NavCore's, ICF-folded)
    void compute_bounds();      // 0x4704b0 (the bounding box e44 / f54 of the vertices)
    ~NavData();                 // 0x41f5d0 (game/entity.cpp: the member destructors, v30 first)
    // ---- src/game/party2: the world-map path finder (cells = the 0x88-byte formation records, v30 = its open list) ----
    bool shares(const int& a, const int& b) const;   // 0x48efa0 (game/mission_ui2/mission_4.cpp, ret 8: do the cells a and b name a common rider; cells[i].ids[0x11..0x13] are its id / pair, the arguments are cell indices)
    bool can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max);   // 0x4b3420 (ret 0x1c)
    void reset();                               // 0x4b33c0 (new search generation: m64)
    bool find_path(int from, int to, int* path, float* detail, int* count, int* visited, int* guard,
                  unsigned* mask_req, unsigned* mask_ban, char* avoid);       // 0x4b6b40 (ret 0x28)
};

#endif // TERRAIN_B_NAV_DATA_H
