// nav_data.h: the ground / nav data of the scene (rgl::world::ground, +0xb0): the four-vector container with other element
// types. The same 0x68-byte object is the ground geometry of the strategic scene (strat/strat_entity.h Ground), the map store
// of the world-map path finder (src/game/party2) and the container the terrain tessellator fills (Terrain::tessellate).
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
    int npts;                   // +0x00 corners in use (at most 4)
    int vidx[4];                // +0x04 vertex index of each corner                      0x4b3420
    int edges[4];               // +0x14 the edge (NavData::edges) of each side           0x47ee60, 0x4b3420
    int kind;                   // +0x24 terrain mask (low 3 bits: the terrain kind)       0x4b2770, 0x4b2880, 0x4b3420
    int target[4];              // +0x28 the cell reached through each side (-1: none)    0x47ee60, 0x4b6b40
    float dist[4];              // +0x38 keep distance per side                           0x4b3420, 0x4b6b40
    int zone;                   // +0x48 the zone number (0x5f5e0ff: a blocked cell)        0x51d3a0, 0x48efa0
    int zone_pair[2];           // +0x4c a blocked cell's two neighbouring zones (-1: none) 0x51d7c0, 0x48efa0
    rgl::vector4 trans;         // +0x54 plane (normal, distance)
    float scale;                // +0x64
    int f68;                    // +0x68
    int gen;                    // +0x6c search generation of the world-map path finder (compared with NavData::m64; 0x4b6b40)
    int parent;                 // +0x70 the record the search came from (0x4b6b40)
    rgl::vector4 pos;           // +0x74 x, y, z of the record and (w) its keep distance (0x4b6b40 copies them together)
    face* obj;                  // +0x84
    NavCell() { kind = 0; gen = 0; parent = 0; f68 = 0; obj = 0; scale = 1.0f; zone = -1; zone_pair[0] = -1; zone_pair[1] = -1; }   // 0x4246f0
    // ---- the world map's path finder reads the corner / side block field by field (game/party2) ----
    int* verts() { return vidx; }
    int* edge_ids() { return edges; }
    int& mask() { return kind; }
    int* targets() { return target; }
    float* dists() { return dist; }
    rgl::vector4& pos4() { return pos; }
};
struct NavEdge {                // 0x24 bytes: a side shared by at most two cells (0x47ee60)
    int count;                  // +0x00 cells using the edge (1 or 2)
    int cell;                   // +0x04 the cell that created it
    int other;                  // +0x08 the cell on its other side (-1: none)
    int a, b;                   // +0x0c start / end vertex
    rgl::vector4 mid;           // +0x14 midpoint (the world-map path's corner points, 0x4b6b40)
};
#include "../hnode.h"       // hnode / node_gt: the open list's element and comparator
// the world-map search's open list: the members of a std::priority_queue<hnode, vector<hnode>, node_gt> (0x14 bytes: the container,
// then the comparator padded to a word); the heap steps the original expands in place are party2/party_2.cpp's
struct OpenQ {
    std::vector<hnode> c;
    node_gt comp;
    bool empty() const { return c.empty(); }
    const hnode& top() const { return c.front(); }
    void pop_x();
    void pop_y();
    void push_x(const hnode& v);
    void push_y(const hnode& v);
};
struct NavData {                // 0x68 bytes
    std::vector<NavVert> verts;      // +0x00
    std::vector<NavCell> cells;      // +0x10
    std::vector<NavEdge> edges;      // +0x20
    OpenQ open;                 // +0x30 the path search's open list (find_path 0x4b6b40; the comparator is at +0x40)
    rgl::vector4 e44;           // +0x44
    rgl::vector4 f54;           // +0x54
    int m64;                    // +0x64
    NavData();                  // 0x478150 (terrain_b/poly_mesh.cpp)
    int add(int n, const rgl::vector4* pts);    // 0x424220 (rgl/dynamic/world_grid.cpp: one polygon -> n vertices + a cell; returns the cell index)
    void build();               // 0x47ee60 (mesh_slot/mesh_pass.cpp: the edges, neighbours, centres, normals / areas and the bounds)
    // mission5/game_2.cpp: the coast walk from cell `start` around the vertex `target` (each step the last edge of the cell that
    // ends at target and does not lead back), the far vertices of those edges into out[*count], once each
    void build_chain(int target, int start, int* out, int* count);   // 0x5234e0 (ret 0x10)
    void compute_bounds();      // 0x4704b0 (the bounding box e44 / f54 of the vertices)
    ~NavData();                 // 0x41f5d0 (game/entity.cpp: the member destructors, `open` first)
    // ---- src/game/party2: the world-map path finder (cells = the 0x88-byte formation records, open = its open list) ----
    bool shares(const int& a, const int& b) const;   // 0x48efa0 (game/mission_ui2/mission_4.cpp, ret 8: do the cells a and b touch a common zone (NavCell::zone / zone_pair), the arguments are cell indices)
    bool can_move(int from, float x, float y, int to, const float* p, unsigned flags, int max);   // 0x4b3420 (ret 0x1c)
    void reset();                               // 0x4b33c0 (new search generation: m64)
    bool find_path(int from, int to, int* path, rgl::vector4* detail, int* count, int* visited, int* guard,
                  unsigned* mask_req, unsigned* mask_ban, char* avoid);       // 0x4b6b40 (ret 0x28)
    // ---- src/game/party/game.cpp: the new-game zone numbering of the world map (0x51d3a0..0x51db70) ----
    void solve(int* arg1, int* arg2, int* arg3);                // 0x51d3a0 (ret 0xc)
    int* walk(int* a, int* idx, int* seen, int* n);             // 0x51d7c0 (ret 0x10)
    int find_trigger(int* idp, int* a2, int* arr, int* cnt, unsigned* m1, unsigned* m2, int* flag);   // 0x51d920 (ret 0x1c)
    void score_triggers(int* a, int* b, int* c);                // 0x51dac0 (ret 0xc)
    float trigger_score(int row, int idx, unsigned a, unsigned b, unsigned c);                         // 0x51db70 (ret 0x14)
};

#endif // TERRAIN_B_NAV_DATA_H
