// Slice s00481d10: world-map (re)generation.
// 0x481d10 is the terrain generator entry (thiscall, cleans 0xc): it derives the seed fields from the caller's
// seed word, sizes the cell grid from the map scale, places every cell (x, y) at (x, y) * scale, picks the quad
// diagonals from the cell heights, runs the full generation chain (layer rebuild, rivers, flow, placeables,
// scatter sets, noise offsets) and finally re-tessellates the mesh and finishes the entity pass.
// 0x482170 (thiscall, cleans 8) regenerates the water of a finished map: it drops both scatter sets, rebuilds the
// water mesh, relights both particle lists when the high-quality water path is off, and stores fresh values into
// the two height fields of the caller's seed word.
// Layouts: terrain_obj per s00480db0/terrain.h (cells[x * 251 + y] at 0x288, quads[x * 250 + y] at 0x8e48fc,
// world at 0xc3b0dc, spawn scale at 0xc3b0e0, scatter sets set1/set0/set2 at 0xc3b0e4/0x343b4ec/0x5c3b8f4).
#include <vector>
#include "rgl.h"

namespace rgl { struct mesh; struct entity; }

// ---------------------------------------------------------------- terrain -----------------------------------

struct terrain_quad;

struct terrain_cell {                   // 0x94, cells[x * 251 + y] at terrain_obj+0x288
    float px, py;                       // 0x00 / 0x04 cell origin in the map plane
    float wet;                          // 0x08 wetness
    int f0c;                            // 0x0c
    int water_id;                       // 0x10 index in the water mesh
    int rec;                            // 0x14 water record / point index
    int f18, f1c;                       // 0x18 / 0x1c
    float nx, ny, nz;                   // 0x20 / 0x24 / 0x28 surface normal
    float f2c;                          // 0x2c
    float u, v;                         // 0x30 / 0x34
    int f38, f3c;                       // 0x38 / 0x3c
    float light;                        // 0x40
    char f44[0x94 - 0x44];
    void place(float* at);              // 0x4644e0 positions the cell at `at`
};

struct terrain_quad {                   // 0x38, quads[x * 250 + y] at 0x8e48fc
    int diag;                           // 0x00
    char f04[0x38 - 0x04];
    void set_diag();                    // 0x464760 diagonal from the four corner heights
};

struct scatter_set {                    // generator set at 0xc3b0e4 / 0x343b4ec / 0x5c3b8f4
    char f00[0x18];
};

struct World {                          // the scene object at terrain_obj+0xc3b0dc
    char pad0[0x64];
    void* cells;                        // 0x64 current cell_rows table
    void main_camera(float* v, const float* sun_dir, const float* centre);   // 0x42ad60
};

struct psys {                           // particle system global at 0xa70248
    void finish(World* w);              // 0x4711e0
};

struct terrain_obj {
    char pad000[4];
    int seed0;                          // 0x004 mixed into the first srand
    int seed1;                          // 0x008 mixed into the second srand
    int seed2;                          // 0x00c height word: top bit = negative, bits 10.. = height band
    int f10;
    int f14;                            // 0x014 runs through the cell-scale derivation
    char pad018[0x28 - 0x18];
    char layers[0x258 - 0x28];          // 0x028 14 light records, 0x28 each, active int at +0x00
    int light15;                        // 0x258 the 15th light slot (the lighting loop walks down to 0x28)
    char pad25c[0x270 - 0x25c];
    float size_x;                       // 0x270 map size along x (cells * scale)
    float size_y;                       // 0x274 map size along y
    int h;                              // 0x278 grid extent along x (cells stride 251)
    int w;                              // 0x27c grid extent along y (cells stride 1)
    char pad280[0x288 - 0x280];
    terrain_cell cells[251 * 251];      // 0x288 .. 0x8e48fc
    terrain_quad quads[250 * 250];      // 0x8e48fc .. 0xc3b0dc (x * 250 + y)
    World* world;                       // 0xc3b0dc map object handed in as the seed argument
    float spawn_scale;                  // 0xc3b0e0 world units per cell
    scatter_set set1;                   // 0xc3b0e4 (0x27fc08 bytes of generator data)
    char pad_set1[0x343b4ec - 0xc3b0e4 - 0x18];
    scatter_set set0;                   // 0x343b4ec (0x2800408 bytes)
    char pad_set0[0x5c3b8f4 - 0x343b4ec - 0x18];
    scatter_set set2;                   // 0x5c3b8f4
    char pad_bd0c[0x843bcfc - 0x5c3b8f4 - 0x18];
    rgl::vector4 bounds;                // 0x843bcfc
    float sun_dir[3];                   // 0x843bd0c
    float map_centre[3];                // 0x843bd1c
    char pad_bd28[0x843bd60 - 0x843bd28];
    float noise_a;                      // 0x843bd60
    float noise_b;                      // 0x843bd64
    float noise_c;                      // 0x843bd68
    char pad_bd6c[0x843bd75 - 0x843bd6c];
    char flag_bd75;                     // 0x843bd75 negative-height map flag

    void generate(int seed, int* objs, char flag);          // 0x481d10
    void regenerate_water(void** out, int mode);            // 0x482170

    // generation chain (all thiscall on the terrain object)
    void gen_reset();                                       // 0x4647e0
    void gen_layers();                                      // 0x4713d0
    void gen_rivers();                                      // 0x46bc70
    void gen_4661e0();                                      // 0x4661e0
    void gen_flow();                                        // 0x46ad50
    void gen_4664e0();                                      // 0x4664e0
    void gen_4666b0();                                      // 0x4666b0
    void gen_466930();                                      // 0x466930
    void gen_466c10();                                      // 0x466c10
    void gen_objects_plain(int* objs);                      // 0x4657c0
    void gen_objects_aligned(int* objs);                    // 0x4656c0
    void gen_object(int* objs, int i);                      // 0x467250
    void gen_466aa0();                                      // 0x466aa0
    void gen_47cae0();                                      // 0x47cae0
    void gen_465540();                                      // 0x465540
    void gen_scatter(scatter_set* s, int mode);             // 0x46a270
    void gen_466fa0();                                      // 0x466fa0
    void gen_473980();                                      // 0x473980
    void gen_47a460();                                      // 0x47a460
    void gen_set_init(scatter_set* s);                      // 0x47df80
    void light_particles(scatter_set* s, int mode);         // 0x472020
    void create_water(void** out);                          // 0x480db0
    void tessellate();                                      // 0x481570
};

// ---------------------------------------------------------------- globals ----------------------------------

extern int g_mask_w;                    // 0x5a8e78
extern int g_mask_h;                    // 0x5a8eb8
extern int g_mask_size;                 // 0x5a8f60
extern int g_mask_flag;                 // 0x5a8e70
extern int g_no_cam;                    // 0x5a9fb0
extern int g_env2;                      // 0x5e28fc (shaders on)
extern int g_water_mode;                // 0x5aa084

float ftol(int dummy);                  // 0x535a64: takes its real argument on the x87 stack, returns int in eax
int irand();                            // 0x535a9f rand()
void seed_rng(unsigned s);              // 0x536a92 srand
void __stdcall gen_set_clear(scatter_set* s);   // 0x479da0 (__stdcall, cleans its one argument)

struct scaler {                         // the double -> grid-count helper object (0x535a64 takes no `this`)
    int count(float v) { return ftol(0); }     // v already on the x87 stack at this point
};

// ------------------------------------------------------------------------------------------------------------

// @ 0x481d10
void terrain_obj::generate(int seed, int* objs, char flag)
{
    gen_reset();

    unsigned scale = (f14 & (g_mask_w - 1)) + 2;
    spawn_scale = (float)scale;
    seed_rng(seed0 & (g_mask_size - 1) & (g_mask_size - 1));
    flag_bd75 = (char)((char)(g_mask_flag - 1) & (char)(seed2 >> 31)) != 0;
    size_x = (float)(unsigned)(seed2 & (g_mask_h - 1));                 // height band -> map extent
    size_y = (float)(unsigned)((seed2 >> 10) & (g_mask_h - 1));
    world = (World*)seed;

    float inv = 1.0f / spawn_scale;
    scaler sc;
    h = sc.count(inv * size_x);
    w = sc.count(inv * size_y);
    if (h > 250)
        h = 250;
    if (w > 250)
        w = 250;
    if (h < 40)
        h = 40;
    if (w < 40)
        w = 40;

    size_x = (float)h * spawn_scale;
    size_y = (float)w * spawn_scale;

    float b[4];
    b[0] = 0.0f;
    b[1] = 0.0f;
    b[2] = 0.0f;
    b[3] = 0.0f;

    gen_layers();

    for (int y = 0; y <= w; y++) {
        float p[4];
        for (int x = 0; x <= h; x++) {
            *(int*)&p[2] = 0;
            p[0] = (float)x * spawn_scale;
            p[1] = (float)y * spawn_scale;
            cells[x * 251 + y].place(p);
        }
    }

    for (int y = 0; y < w; y++) {
        for (int x = 0; x < h; x++)
            quads[x * 250 + y].set_diag();
    }

    bounds = *(rgl::vector4*)b;

    gen_rivers();
    gen_4661e0();
    gen_flow();
    gen_4664e0();
    gen_4666b0();
    gen_466930();
    gen_466c10();

    if (objs != 0 && flag) {
        if (g_no_cam == 0)
            gen_objects_plain(objs);
        else
            gen_objects_aligned(objs);
    }

    seed_rng(seed1 & (g_mask_size - 1));

    noise_a = (float)(irand() % 0x3dc9) * 0.063223116f;
    noise_b = (float)(irand() % 0x3dc9) * 0.063223116f;
    noise_c = (float)(irand() % 0x3dc9) * 0.063223116f;

    if (objs != 0) {
        for (int i = 0; i < *objs; i++)
            gen_object(objs, i);
    }

    gen_flow();
    gen_466aa0();
    gen_47cae0();
    gen_465540();
    gen_scatter(&set0, 0);
    gen_scatter(&set1, 0);
    gen_scatter(&set2, 2);
    gen_466fa0();
    ((psys*)0xa70248)->finish(world);
    float v[3];
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    world->main_camera(v, sun_dir, map_centre);
    tessellate();
    gen_473980();
}

// @ 0x482170
void terrain_obj::regenerate_water(void** out, int mode)
{
    gen_set_clear(&set0);
    gen_set_clear(&set1);
    *out = 0;
    create_water(out);
    gen_set_init(&set0);
    gen_set_init(&set1);
    gen_47a460();
    if (g_env2 == 0 || g_water_mode == 0) {
        light_particles(&set0, mode);
        light_particles(&set1, mode);
    }

    int mask = g_mask_h - 1;
    scaler sc;
    int r = sc.count(size_x);
    if ((unsigned)r > (unsigned)mask)
        r = mask;
    seed2 = (seed2 & ~mask) | r;
    mask = g_mask_h - 1;
    r = sc.count(size_y);
    if ((unsigned)r > (unsigned)mask)
        r = mask;
    seed2 = (seed2 & ~(mask << 10)) | (r << 10);
}
