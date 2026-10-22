// Slice s0047ff60: terrain chunk mesh builder (one mesh per chunk and blend layer, plus the water chunks).
// COMPLETE behaviourally (see nonmatching.txt); not byte-exact. Names are provisional.
// Flags: default (/O2 /MT /Gy /TP).
// Layouts: rgl.h (shared), mesh from s00412a90/mesh.h, terrain from s00466aa0/s00466aa0.h.
#include <math.h>
#include <stdlib.h>
#include <vector>
#include "rgl.h"
#include "terrain.h"

namespace rgl {

struct vec2 { float x, y; };
struct vertex60 {                // 0x60 bytes, vector at mesh+0x50
    vector4 pos;                 // 0x00
    vector4 f10;                 // 0x10
    vector4 f20;                 // 0x20
    vector4 normal;              // 0x30
    vec2 uv;                     // 0x40
    int f48;                     // 0x48
    int f4c;                     // 0x4c
    int index;                   // 0x50: point index
    unsigned color;              // 0x54: packed ARGB
    int f58;
    int f5c;
};
struct mat_t { char pad[0x6c]; int flags; };     // material (flags at +0x6c; 0x1f test)
struct res_t { char pad0[0x30]; mat_t* mat; };   // resource record (from s00412a90/mesh.h)
struct slot { char pad[0x28]; char f28; };       // dirty flags: slots[0].f28 @0x138, slots[1].f28 @0x164
struct mesh {                    // 0x260 bytes, vtable 0x54a6f4
    mesh();                                                                   // 0x412a90
    char pad0[0x2c - 4];
    void* res;                   // 0x2c (res_t*)
    char pad30[0x3c - 0x30];
    unsigned color;              // 0x3c packed ARGB
    char pad40[0x50 - 0x40];
    std::vector<vertex60> v50;   // 0x50
    char pad68[0x10c - 0x68];
    unsigned mask;               // 0x10c
    slot slots[2];               // 0x110 / 0x13c
    char pad168[0x214 - 0x168];
    int f214;                    // 0x214 (layer + 11)
    virtual ~mesh();
    virtual void slot1();
    virtual void slot2();
    virtual void slot3();
    virtual void slot4();        // 0x10: notify the scene
    void clear_geometry();                                                    // 0x428e80
    int add_point(const vector4* p);                                          // 0x412f50
    int add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c);  // 0x413c50
    void add_face(int a, int b, int c);                                       // 0x411920 (Psys::add_rec2)
    void compute_tangents();                                                  // 0x41ce80
    void sort_by_z();                                                         // 0x47b7b0
};

} // namespace rgl

typedef rgl::vector4 vector4;
typedef rgl::mesh mesh;
typedef rgl::mesh entity;        // scene entity / mesh stub (0x260 bytes)
using rgl::vertex60;
using rgl::res_t;

extern int g_use_shaders;                    // 0x5e28fc (0: colours are gamma-corrected on output)

// camera / light state (provisional layout; only these fields are touched here).
struct cam_state {
    char pad0[0x22c];
    float bmin_x;                            // 0x22c
    float bmin_y;                            // 0x230
    float bmin_z;                            // 0x234
    char pad238[4];
    float bmax_x;                            // 0x23c
    float bmax_y;                            // 0x240
    float bmax_z;                            // 0x244
    char pad248[0x27c - 0x248];
    rgl::matrix world;                       // 0x27c (s/f/u rows = camera axes, o = eye)
};
extern cam_state* g_cam;                     // 0xa70438

// scene: chunk meshes are registered in scene->objs (std::vector at +0x3a4)
struct scene_stub {
    char pad0[0x3a4];
    std::vector<mesh*> objs;
    void on_obj_changed();                   // 0x419300
};
struct world_stub { char pad0[0xc3b0dc]; scene_stub* scene; };   // scene at 0xc3b0dc

unsigned pack_color(const rgl::vector4* c);                  // 0x40e440 (cdecl)
unsigned pack_signed_color(const float* c);                  // 0x461370 (cdecl)
void unpack_color(rgl::vector4* out, const unsigned* argb);  // 0x40e4e0 (cdecl)
vector4* to_unit(vector4* out, const vector4* p);            // 0x463c20 (cdecl)

static inline unsigned to_byte(float v)          // (int)(v * 255) rounded half up, clamped to 0..255
{
    double d = (double)(v * 255.0f);
    int i = (int)(v * 255.0f);
    if (!(d - (double)i <= 0.5)) i++;
    if (i < 0) return 0;
    if (i >= 0x100) return 0xff;
    return i;
}

struct terrain_cell {           // 0x94 (layout from slices s00466aa0/s00464a60)
    float px, py, pz;           // 0x00
    char f0c[0x10 - 0x0c];
    int vidx;                   // 0x10 vertex index, -1 = not built in this chunk
    char f14[0x48 - 0x14];
    float blend[10];            // 0x48.. per-layer blend weights
    char f70[0x84 - 0x70];
    float h2;                   // 0x84
    char f88[0x94 - 0x88];
};
struct cell_grid {
    terrain_cell at[251 * 251];
    terrain_cell& operator()(int x, int y) { return at[x * 251 + y]; }
};

// raw access to the parts of the terrain object this slice does not name
static inline int& ti(terrain_obj* t, unsigned off) { return *(int*)((char*)t + off); }
static inline float& tf(terrain_obj* t, unsigned off) { return *(float*)((char*)t + off); }
static inline float* tp(terrain_obj* t, unsigned off) { return (float*)((char*)t + off); }

// scene->objs bookkeeping: append if absent, then tell the scene (0x419300, vector fast path / insert)
static inline void register_mesh(scene_stub* sc, mesh* m)
{
    unsigned found = 0xffffffff;
    for (unsigned i = 0; i < sc->objs.size(); i++) {
        if (sc->objs[i] == m)
            found = i;
    }
    if ((int)found >= 0)
        return;
    sc->on_obj_changed();
    if (sc->objs.size() < sc->objs.capacity()) {
        mesh** p = sc->objs._Mylast;
        *p = m;
        sc->objs._Mylast = p + 1;
    } else {
        sc->objs.insert(sc->objs.end(), m);
    }
    m->slot4();
}

// shader path for one vertex (provisional reconstruction of the original's second loop copy):
// normal of the cell, light direction (sun_dir negated), sun colour scale, packed with pack_signed_color.
static inline void shade_shader(vector4* out, const vector4& uvw, unsigned level, const vector4& pos)
{
    float t = level * (1.0f / 255.0f);
    out->x = uvw.x * t + pos.x;
    out->y = uvw.y * t + pos.y;
    out->z = uvw.z * t + pos.z;
}

// one vertex of a triangle: point + colour/uv/position, then add_vertex
static inline void emit_vertex(terrain_obj* self, mesh* m, terrain_cell* cc, int layer, int blend,
                               float scale, int light, unsigned* alpha_out)
{
    int idx = m->add_point((const vector4*)&cc->px);
    cc->vidx = idx;

    vector4 pos(0.0f, 0.0f, 1.0f);
    unsigned col, alpha = 0xff;
    vector4 uvw(1.0f, 1.0f, 1.0f);
    float f, w = 0.0f;
    if (blend < 0) {
        f = cc->blend[layer];
    } else {
        w = cc->blend[blend];
        f = (1.0f - w) * cc->blend[layer] + w;
        alpha = to_byte(w);
    }
    col = to_byte(f * *(float*)((char*)cc + 0x1c) * 255.0f);
    if (layer == 0) {
        pos.x = cc->py * 0.025f;
        pos.y = (cc->px * 0.025f) * scale;
        pos.z = (cc->py * 0.025f) * scale;
    } else {
        to_unit(&pos, (const vector4*)&cc->px);
    }
    if (layer < 15 && layer > 0) {
        vector4 gr;
        unpack_color(&gr, (const unsigned*)&cc->px);
        uvw = gr;
    }

    unsigned colour;
    if (g_use_shaders) {
        vector4 sh;
        shade_shader(&sh, uvw, col, pos);
        colour = pack_signed_color(&sh.x);
    } else {
        rgl::vector4 lit;
        sun_light(self, &lit, &uvw, &pos);
        colour = pack_color(&lit);
    }
    unsigned fg = colour | 0xff000000;
    if (blend >= 0)
        fg = colour | alpha << 24;

    int vi = m->add_vertex(idx, (const rgl::vec2*)&uvw, &fg, &pos, &light);
    m->v50[vi].color = fg;
    m->v50[vi].index = col;
    m->v50[vi].f4c = layer + 11;
    *alpha_out = alpha;
}

// @ 0x47ff60
void build_chunk(terrain_obj* self, int layer, int blend, int cx, int cy)
{
    if (ti(self, 0x28 + layer * 0x28) < 0)
        return;

    int w = ti(self, 0x278);
    int h = ti(self, 0x27c);
    float scale = tf(self, 0x284);              // (unverified: chunk position scale)
    int blk = ti(self, 0x843bd70);
    int nx = ti(self, 0x845d984), ny = ti(self, 0x845d988);
    entity** ents2 = (entity**)tp(self, 0x845d9f0);
    entity** arr3 = (entity**)tp(self, 0x845da54);
    cell_grid* g = (cell_grid*)tp(self, 0x8439d48);

    entity* e;
    if (layer < 15)
        e = arr3[layer * 25 + cy * 5 + cx];
    else
        e = ents2[cy * 5 + cx];
    if (!e)
        e = new mesh();
    mesh* m = (mesh*)e;
    m->clear_geometry();

    if (layer < 15) {
        if (blend < 0)
            m->res = (void*)ti(self, layer * 0x28 + 0x34);
        else
            m->res = (void*)ti(self, layer * 0x28 + 0x38);
    } else {
        m->res = 0;
    }

    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++)
            g->operator()(x, y).vidx = -1;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (cx * blk > x)
                continue;
            if (cx != nx - 1 && x >= (cx + 1) * blk)
                continue;
            if (cy * blk > y)
                continue;
            if (cy != ny - 1 && y >= (cy + 1) * blk)
                continue;

            for (int flip = 0; flip < 2; flip++) {
                int tri[8];
                triangle_corners(self, x, y, flip, tri, tri + 2, tri + 4);
                bool vis[3];
                for (int k = 0; k < 3; k++) {
                    terrain_cell* cc = &g->operator()(tri[k * 2], tri[k * 2 + 1]);
                    if (layer < 15) {
                        bool v;
                        if (blend < 0)
                            v = cc->blend[layer] > 0.0f;
                        else
                            v = cc->blend[layer] > 0.0f || cc->blend[blend] > 0.0f;
                        vis[k] = v;
                        if (layer == 0) {       // first layer: extend the camera bounds with the local point
                            float dx = cc->px - g_cam->world.o.x;
                            float dy = cc->py - g_cam->world.o.y;
                            float dz = cc->pz - g_cam->world.o.z;
                            float lx = dx * g_cam->world.s.x + (dy * g_cam->world.f.x + dz * g_cam->world.u.x);
                            float ly = dx * g_cam->world.s.y + (dy * g_cam->world.f.y + dz * g_cam->world.u.y);
                            float lz = dx * g_cam->world.s.z + (dy * g_cam->world.f.z + dz * g_cam->world.u.z);
                            if (lx < g_cam->bmin_x) g_cam->bmin_x = lx;
                            if (ly < g_cam->bmin_y) g_cam->bmin_y = ly;
                            if (lz < g_cam->bmin_z) g_cam->bmin_z = lz;
                            if (lx > g_cam->bmax_x) g_cam->bmax_x = lx;
                            if (ly > g_cam->bmax_y) g_cam->bmax_y = ly;
                            if (lz > g_cam->bmax_z) g_cam->bmax_z = lz;
                        }
                    } else {
                        vis[k] = true;
                    }
                }
                if (!vis[0] && !vis[1] && !vis[2])
                    continue;

                for (int k = 0; k < 3; k++) {
                    terrain_cell* cc = &g->operator()(tri[k * 2], tri[k * 2 + 1]);
                    if (cc->vidx >= 0)
                        continue;
                    unsigned alpha;
                    emit_vertex(self, m, cc, layer, blend, scale, 0x17fff, &alpha);
                }
            }
        }
    }

    if (layer == 15) {
        ents2[cy * 5 + cx] = e;
        m->color = 0xff000811;
        m->mask = 0xffff;
        m->slots[0].f28 = 1;
        m->slots[1].f28 = 1;
        m->f214 = layer + 11;
        register_mesh(((world_stub*)self)->scene, m);
        return;
    }
    if (g_use_shaders) {
        m->compute_tangents();
        m->sort_by_z();
    }
    arr3[layer * 25 + cy * 5 + cx] = e;
    m->mask = 0x17fff;

    if (!g_use_shaders) {
        res_t* res = (res_t*)m->res;
        if (res && (res->mat->flags & 0x1f) == 0x18) {
            vector4 c = *(vector4*)tp(self, 0x843bd2c);
            m->color = pack_color(&c);
        }
    }
    m->slots[0].f28 = 1;
    m->slots[1].f28 = 1;
    m->f214 = layer + 11;
    register_mesh(((world_stub*)self)->scene, m);
}
