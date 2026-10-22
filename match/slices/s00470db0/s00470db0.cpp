// Slice s00470db0: scene/level helpers at the end of the mesh-physics block: skin-vertex nearest-point map
// (0x470db0), the level's ground-height query (0x470f40), a weighted wind/sky blend over four anchor
// points (0x471040) and the shadow map's light-camera setup (0x4711e0). ALL NAMES PROVISIONAL.
// Flags: default /O2 /MT /Gy /TP.
#include <stddef.h>
#include <math.h>
#include "rgl.h"
#include "mesh.h"

using rgl::vector4;
using rgl::matrix;
using rgl::mesh;
using rgl::point20;

// ---- 0x470db0: for every point of `obj`, the index of the nearest point of the base mesh -----------------
struct SkinMaps {
    char pad0[0x10];
    int count[5];                     // 0x10 points per mesh
    int* map[5];                      // 0x24 nearest base point per point
};
struct MeshRef { char pad[0x38]; mesh** base; };          // base: pointer to the base mesh pointer at +0x38
struct SkinOwner {
    char pad0[0x38];
    MeshRef* ref;                     // 0x38
    void build_map(SkinMaps* out, int idx, mesh* obj);      // 0x470db0
};

// @ 0x470db0
void SkinOwner::build_map(SkinMaps* out, int idx, mesh* obj)
{
    mesh* base = *ref->base;
    int nbase = base->v70.size();
    int n = obj->get70().size();
    out->count[idx] = n;
    out->map[idx] = new int[n];
    for (int i = 0; i < out->count[idx]; i++) {
        float best = 100000.0f;
        int bi = 0;
        const point20* bp = &*base->v70.begin();
        for (int j = 0; j < nbase; j++) {
            vector4 q = (&*obj->get70().begin())[i].a;
            if (q.x < 0.0f) q.x = -q.x;
            float dx = q.x - bp->a.x;
            float dy = q.y - bp->a.y;
            float dz = q.z - bp->a.z;
            float d = dy * dy + (dx * dx + dz * dz);
            if (d < best) { best = d; bi = j; }
            bp++;
        }
        out->map[idx][i] = bi;
    }
}

// ---- 0x470f40: height of the level's ground under (x, y) ------------------------------------------------
// Pieces of other slices (s00439100 level/cell, s004333c0 face), redeclared here with their addresses.
struct face {                         // collision polygon (s004333c0): vertices from +0x20, plane normal at +0xa0
    int m0, m4;                       // m4 >= 0: the polygon is active
    char pad8[0x20 - 8];
    vector4 v[8];                     // 0x20
    vector4 normal;                   // 0xa0
    int count;                        // 0xb0
    bool contains_xy(const float* p) const;      // 0x4333c0
};
struct cell {                         // grid cell (0x34 bytes, s00439100)
    char pad[0x34];
    void collect_point(const float* p, int* use3d, unsigned mask, face** out, int* count);   // 0x439780
};
struct Level {
    char pad0[0x28];
    cell** cells;                     // 0x28
    void locate(const float* p, int* out) const;     // 0x41ed00
};

// @ 0x470f40
bool __stdcall ground_height(Level* lv, const float* p, float* out)
{
    face* hits[0x2000];
    int count;
    int use3d;
    int cellxy[2];
    vector4 q;
    q.x = p[0];
    q.y = p[1];
    q.z = 0.0f;
    use3d = 0;
    lv->locate(&q.x, cellxy);
    count = 0;
    cell* col = lv->cells[cellxy[0]];
    col[cellxy[1]].collect_point(&q.x, &use3d, 0x104, hits, &count);
    int n = count;
    for (int i = 0; i < n; i++) {
        if (hits[i]->m4 >= 0 && hits[i]->contains_xy(p)) {
            face* f = hits[i];
            *out = f->v[0].z - ((p[0] - f->v[0].x) * f->normal.x + (p[1] - f->v[0].y) * f->normal.y) / f->normal.z;
            return true;
        }
    }
    return false;
}

// ---- 0x471040: sky/wind sample over four anchor points -----------------------------------------------------
vector4 __stdcall wind_noise();                      // 0x463a80 (hidden return pointer)
extern Level* g_level;                               // 0xa732b4
struct Anchors {
    char pad0[0x13b64];
    float val[4][4];                                 // 0x13b64 four records of four weights/values
    float pos[4][2];                                 // 0x13ba4 their xy positions
    void blend(float* out, float x, float y);        // 0x471040
};

// @ 0x471040
void Anchors::blend(float* out, float x, float y)
{
    vector4 wn = wind_noise();
    float s1 = 0.0f, s0 = 0.0f, s2 = 0.0f, s3 = 0.0f, sum = 0.0f;
    float* rec = &val[0][1];
    float* xy = &pos[0][0];
    int n = 4;
    do {
        n--;
        float w = 1.0f / (float)sqrt((xy[0] - x) * (xy[0] - x) + (xy[1] - y) * (xy[1] - y));
        s1 = w * rec[0] + s1;
        s0 = w * rec[-1] + s0;
        s2 = w * rec[1] + s2;
        s3 = s3 + w * rec[2];
        sum = w + sum;
        rec += 4;
        xy += 2;
    } while (n != 0);
    float t = 1.0f / sum;                    // inverse weight sum, then reused for the ground factor (address taken)
    float r0 = wn.x + t * s0;
    float r1 = wn.y + t * s1;
    float r2 = wn.z + s2 * t;
    float r3 = wn.w + t * s3;
    if (g_level == 0)
        t = 1.0f;
    else
        ground_height(g_level, &x, &t);
    out[0] = r0 - t * 10.0f;
    out[1] = r1;
    out[2] = r2 * t + r2;
    out[3] = r3 * t + r3;
}

// ---- 0x4711e0: shadow map: attach the light camera and reset its bounds ------------------------------------
// The scene object (Ctx of s0042aaf0 / LightCam of s00463790) fields used here.
struct LightCtx {
    char pad0[0x22c];
    vector4 vmin;                    // 0x22c
    vector4 vmax;                    // 0x23c
    vector4 size;                    // 0x24c
    vector4 center;                  // 0x25c
    vector4 wcenter;                 // 0x26c
    matrix world;                    // 0x27c
    matrix light;                    // 0x2bc
    vector4 amb;                     // 0x2fc
    void build_light_frame(matrix* m, vector4* amb);    // 0x470c00
};
struct ShadowMap {
    char pad0[0x1f0];
    LightCtx* cam;                   // 0x1f0
    void release_all();              // 0x463b70
    void attach_camera(LightCtx* c); // 0x4711e0
};
extern int g_shadow_quality;         // 0x5aa000
extern int g_shadow_quality_req;     // 0x5aa004

// The original carries two source-location markers in its code stream (right after the prologue and before the
// epilogue): a short jump over 16 bytes of data ("WL  ", 12 or 13, 0, "WL  "). Only inline asm could emit them
// and that forces an ebp frame the original does not have, so they are left out (rows differ by the shift).

// @ 0x4711e0
void ShadowMap::attach_camera(LightCtx* c)
{
    // marker "WL" 12 here in the original
    cam = c;
    cam->vmin = vector4(10000.0f, 10000.0f, 10000.0f);
    cam->vmax = vector4(-10000.0f, -10000.0f, -10000.0f);
    cam->size = vector4(1.0f, 1.0f, 1.0f);
    cam->center = vector4(0.0f, 0.0f, 0.0f);
    cam->wcenter = vector4(0.0f, 0.0f, 0.0f);
    cam->build_light_frame(&cam->world, &cam->amb);
    vector4 z(0.0f, 0.0f, 0.0f);
    cam->world.o = z;
    if (g_shadow_quality != g_shadow_quality_req) {
        release_all();
        g_shadow_quality = g_shadow_quality_req;
    }
    // marker "WL" 13 here in the original
}
