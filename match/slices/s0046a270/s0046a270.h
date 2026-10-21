// Layout of the world-map terrain object (provisional name `terrain_obj`), recovered from s004661e0/s00466aa0.
// Cells: cells[x * 251 + y] at +0x288 (0x94 bytes each); extent w x h in +0x278 / +0x27c (inclusive indices).
// Quads: quads[x * 250 + y] at +0x8e48fc (0x38 bytes each). Three particle lists follow the quads.
#ifndef H_S0046A270
#define H_S0046A270
#include "rgl.h"

struct vec2 { float x, y; };
struct terrain_cell {           // 0x94
    float px, py, pz;           // 0x00 position; pz is the height
    int ntype;                  // 0x0c (unverified)
    char f10[0x1c - 0x10];
    unsigned flags;             // 0x1c
    char f20[0x28 - 0x20];
    float f28;                  // 0x28
    char f2c[0x30 - 0x2c];
    vec2 disp;                  // 0x30 accumulated displacement (path/flow field), provisional
    vec2 dir;                   // 0x38 flow direction (unit), provisional
    char f40[0x44 - 0x40];
    int order;                  // 0x44 step number when the walk visited it
    char f48[0x4c - 0x48];
    float v4c, v50, v54;        // 0x4c/0x50/0x54
    char f58[0x84 - 0x58];
    float h2;                   // 0x84 copy of pz
    float f88;                  // 0x88
    char f8c[0x94 - 0x8c];
    int kind();                 // 0x4646b0 (thiscall)
};

struct terrain_quad {           // 0x38
    int diag;                   // 0x00
    int tri[2];                 // 0x04 / 0x08 triangle kinds (1/2 = ground)
    rgl::vector4 nrm[2];        // 0x0c / 0x1c triangle normals
    int slot[2];                // 0x2c / 0x30 scratch, set to -1 by 0x46a270
    float f34;                  // 0x34
};

struct entity;
struct World { char pad0[0x9c]; entity* create_entity(int id); };   // 0x428440
struct entity {                 // stub of the scene entity (vtable 0x54b460 family); slot 6 takes a pointer to an int
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5();
    virtual void s6(int* p);
    char pad[0xc4 - 4];
    rgl::vector4 pos;           // 0xc4
};

struct particle {               // 0xa0
    int type;                   // 0x00 index into the particle type table (0x5e28f8)
    int cx, cy;                 // 0x04 / 0x08 source cell
    int f0c;                    // 0x0c
    char f10[0x14 - 0x10];
    rgl::matrix frame;          // 0x14 copy of the source frame
    rgl::vector4 dir;           // 0x54
    int anim;                   // 0x64
    char f68[0x9c - 0x68];
    int f9c;                    // 0x9c
};

struct scatter_src {            // 0x20, source records after the particles (same layout as Terrain's ScatterItem)
    int kind;                   // 0x00 index into g_ptypes
    float freq, thresh;         // 0x04 / 0x08
    rgl::vector4 v;             // 0x0c
    float rate;                 // 0x1c
};
struct particle_list {          // 0x2800408
    int count;
    particle items[0x40000];
    int nsrc;                   // 0x2800004
    scatter_src src[32];        // 0x2800008
};

struct ptype {                  // 0x20, table at *0x5e28f8
    char pad0[0x10];
    __int64 flags;              // 0x10
    int nframes;                // 0x18
    char pad1c[4];
};

struct terrain_obj {
    char pad000[0x10];
    unsigned flags10;           // 0x10 bits 7..13 / 21..27 are fields (see Terrain in s00464a60)
    unsigned flags14;           // 0x14 bits 2.. are a field
    char pad018[0x58 - 0x18];
    int bit58;                  // 0x58 flag bit index or -1
    char pad5c[0x78 - 0x5c];
    int f78;                    // 0x78
    char pad7c[0x278 - 0x7c];
    int w;                      // 0x278
    int h;                      // 0x27c
    char pad280[8];
    terrain_cell cells[251 * 251];      // 0x288 .. index x * 251 + y
    terrain_cell& at(int x, int y) { return cells[x * 251 + y]; }
    terrain_quad quads[250 * 250];     // follows, index x * 250 + y
    terrain_quad& quad(int x, int y) { return quads[x * 250 + y]; }
    World* world;                           // 0xc3b0dc
    float spawn_scale;                      // 0xc3b0e0
    particle_list list_a;                   // 0xc3b0e4
    particle_list list_b;                   // 0x343b4ec  (type flag 0x400000)
    particle_list list_c;                   // 0x5c3b8f4  (type flag 0x80000)
    char pad_843bcfc[0x843bd0c - 0x843bcfc];
    float sun_dir[3];                       // 0x843bd0c
    char pad_843bd18[0x843bd2c - 0x843bd18];
    rgl::vector4 sun_ambient;               // 0x843bd2c
    float sun_diffuse[3];                   // 0x843bd3c
    char pad_843bd48[0x843bd70 - 0x843bd48];
    int block;                              // 0x843bd70 cells per chunk edge
    char pad_843bd74;
    char flag843bd75;
    char pad_843bd76[0x845d984 - 0x843bd76];
    int nx, ny;                             // 0x845d984 / 0x845d988
    entity* ents[25];                       // 0x845d98c chunk entities, index nx_i * 5 + ny_j
    int arr2[25];                           // 0x845d9f0 (cleared with ents)
    int arr3[15][25];                       // 0x845da54

    void smooth_heights();                  // 0x4661e0
    void clear_flags(unsigned mask);        // 0x466480
    void compute_weights();                 // 0x4664e0
    void pick_diagonals();                  // 0x4666b0
    void pick_triangle_types();             // 0x466930
    // s00466aa0
    void accumulate_kinds();                // 0x466aa0
    void erode();                           // 0x466c10
    void emit_particle(particle_list* l, int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim);   // 0x466cd0
    void emit_by_type(int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim);                     // 0x466d90
    void triangle_corners(int x, int y, int flip, int* a, int* b, int* c);   // 0x466e20
    void sun_light(rgl::vector4* out, const float* dir, const float* scale); // 0x466ed0
    void create_chunk_entities();           // 0x466fa0
    void cell_of(int* out, const float* p); // 0x464b40
    // s00468fc0
    void trace_path(const int* from, const int* to, unsigned char bit, int* steps, float* minz, float w1, float w2, int unused, char record);   // 0x468fc0
    void spread_flow(unsigned mask, int mode, unsigned mark);       // 0x469a70
    void spawn_particles(particle_list* l, int unused);                         // 0x46a270
};
extern ptype* g_ptypes;                     // 0x5e28f8
#endif
