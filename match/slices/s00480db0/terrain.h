// Layout of terrain_obj ("world-map terrain"), narrowed to what the water code touches. Every offset comes
// from the disassembly of 0x480db0 / 0x4814a0 / 0x47ff60.
//   cells[x * 251 + y] at 0x288, 0x94 bytes each; the x axis (stride 251) has extent h at +0x278, the y axis
//   extent w at +0x27c.  quads[x * 250 + y] at 0x8e48fc, 0x38 bytes each.  The water-type records are 40
//   bytes each at +0x28 (wt()[t]), 15 of them; create_water/emit_particle test wt()[t].active (int at +0x00)
//   and skip types that are inactive.  The particle-pointer table water_part[16][25] is at 0x845d9f0 (row 0
//   is water type 15, rows 1..15 are types 0..14).
#ifndef TERRAIN_H
#define TERRAIN_H
#include <vector>
#include "rgl.h"

namespace rgl { struct mesh; }

struct particle;                        // 0xa0 river/puddle sprite (only `mat` is used here)
struct entity;

struct terrain_cell {                   // 0x94
    float px, py;                       // 0x00 / 0x04 cell origin in the map plane
    float wet;                          // 0x08 wetness; the water pass runs on cells with wet <= -0.3
    int f0c;                            // 0x0c
    int water_id;                       // 0x10 river-mesh vertex index (-1 = not a vertex yet)
    int rec;                            // 0x14 water record index (-1 = dry); indexed for the light term
    int f18, f1c;                       // 0x18 / 0x1c
    float nx, ny, nz;                   // 0x20 / 0x24 / 0x28 surface normal (light term = -dot(sun_dir, n))
    float f2c;                          // 0x2c
    float u, v;                         // 0x30 / 0x34 texture coordinates (river mesh UV = these * 0.125)
    int f38, f3c;                       // 0x38 / 0x3c
    float light;                        // 0x40 water light scale
    char f44[0x94 - 0x44];
};

struct terrain_quad {                   // 0x38
    int diag;                           // 0x00
    int tri_a;                          // 0x04
    int tri_b;                          // 0x08
    char f0c[0x34 - 0x0c];
    float f34;                          // 0x34
};

struct water_type {                     // 40 bytes, table at terrain_obj+0x28
    int active;                         // 0x00 < 0 = the type draws nothing this frame
    char f04[0x28 - 0x04];
};

struct World {                          // the scene object at terrain_obj+0xc3b0dc
    char pad0[0x68];
    rgl::vector4 pos;                   // 0x68 origin added when flattening a position
    char pad1[0xb0 - 0x78];
    void** store;                       // 0xb0 -> [1] is the float array the light terms are written to
    entity* create_entity(int id);      // 0x428440
};

// The scene entity a mesh is attached to. Vtable slot at +0x10 takes the new mesh; a std::vector<mesh*> at
// +0x118 collects it (the vector's _Myfirst sits at +0x11c because VC7.1 puts the allocator first).
struct entity {
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3();
    virtual void add_mesh(rgl::mesh* m);        // vtable +0x10
    virtual void s5();
    char pad04[0x118 - 0x04];
    std::vector<rgl::mesh*> meshes;             // 0x118
};

struct particle {                       // 0xa0
    char pad0[0x2c];
    void* mat;                          // 0x2c material; 0x4814a0 re-points it at the river material
    char pad2[0xa0 - 0x30];
};

struct psys {                           // particle system, global at 0xa70248
    void* water_material;               // 0x00 the material 0x4814a0 restores
    void flush();                       // 0x463cc0
    void reset();                       // 0x47fef0
};

struct terrain_obj {
    water_type* wt() { return (water_type*)((char*)this + 0x28); }   // 40-byte stride: wt()[t]
    char pad00[0x60];
    void* pub60;                        // 0x60 non-null in the high-quality water path
    char pad64[0x278 - 0x64];
    int h;                              // 0x278 extent along the x axis (cells stride 251)
    int w;                              // 0x27c extent along the y axis (cells stride 1)
    char pad280[0x288 - 0x280];
    terrain_cell cells[251 * 251];      // 0x288 .. 0x8e48fc
    terrain_cell& at(int x, int y) { return cells[x * 251 + y]; }
    terrain_quad quads[250 * 250];      // 0x8e48fc .. 0xc3b0dc
    World* world;                       // 0xc3b0dc
    float spawn_scale;                  // 0xc3b0e0
    char pad_lists[0x843bd0c - 0xc3b0e4];
    float sun_dir[3];                   // 0x843bd0c
    char pad_s1[0x843bd1c - 0x843bd18];
    float map_centre[3];                // 0x843bd1c summed for the river light term
    char pad_s2[0x843bd2c - 0x843bd28];
    rgl::vector4 sun_amb;               // 0x843bd2c
    float sun_dif[3];                   // 0x843bd3c
    char pad_s3[0x845d984 - 0x843bd48];
    int nx;                             // 0x845d984 chunk columns
    int ny;                             // 0x845d988 chunk rows
    entity* ents[25];                   // 0x845d98c chunk entities, index x * 5 + y
    particle* water_part[16][25];       // 0x845d9f0 water particles, index type * 25 + x * 5 + y

    void create_water(void** out);                                   // 0x480db0
    void destroy_water(int a);                                       // 0x4814a0
    void light_particles(void* list, int trace);                     // 0x472020 (thiscall, cleans 8)
    void emit_particle(int type, int v, int x, int y, float z);      // 0x47ff60
    void triangle_corners(int x, int y, int flip, int* c0, int* c1, int* c2);   // 0x466e20
};

#endif
