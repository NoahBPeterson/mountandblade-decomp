// Slice s00481570: terrain mesh rebuild.
// 0x481570 is terrain_obj::tessellate(): it re-tessellates every terrain quad into two triangles (or into four
// when the two half-quad normals disagree), assigning each touched corner cell a point in the new Geom and each
// quad a 0x88-byte element record with the final lighting, then rebuilds the world mesh, installs a fresh
// cell_rows table and relights the scene.
// 0x481c40 appends a character mesh to the world: it selects the character, sets up the animator, collects
// the lod meshes and returns &g_chars[g_cur] with the mesh count written to *out.
// Layouts: Geom / geom_pt / geom_elem are recovered from the ctor 0x478150, build 0x47ee60 and the pushes at
// 0x4772f0 / 0x477380; terrain_obj / cell_rows come from 0x462420 (cell_rows::alloc) and 0x481570 itself.
#include <vector>
#include "rgl.h"

extern int g_use_shaders;                       // 0x5e28fc: programmable pipeline in use

// ---------------------------------------------------------------- terrain -----------------------------------

struct terrain_cell {                   // 0x94, cells[x * 251 + y] at terrain_obj+0x288
    float px, py;                       // 0x00 / 0x04 cell origin in the map plane
    float wet;                          // 0x08 wetness carried into the point list
    int f0c;                            // 0x0c
    int water_id;                       // 0x10
    int rec;                            // 0x14 point index in the Geom (-1 = no point yet)
    int f18, f1c;                       // 0x18 / 0x1c
    float nx, ny, nz;                   // 0x20 / 0x24 / 0x28 surface normal
    float f2c;                          // 0x2c
    float u, v;                         // 0x30 / 0x34
    int f38, f3c;                       // 0x38 / 0x3c
    float light;                        // 0x40
    int f44;                            // 0x44
    float lum[15];                      // 0x48 light contribution per light slot (slot live = terrain+0x258+m*40)
};

struct terrain_quad {                   // 0x38, quads[y * 250 + x] at 0x8e48fc
    int diag;                           // 0x00
    int tri_a;                          // 0x04
    int tri_b;                          // 0x08
    char f0c[0x34 - 0x0c];
    float f34;                          // 0x34
};

struct vec4d { double x, y, z, w; };    // 0x20: what tri_normal returns

struct geom_pt {                        // 0x24, std::vector<geom_pt> at Geom+0x00
    float px, py, wet;                  // 0x00 / 0x04 / 0x08
    int f0c;                            // 0x0c
    char pad10[0x10];                   // 0x10
    float f20;                          // 0x20 = 1.0 (w)
};

struct geom_elem {                      // 0x88, std::vector<geom_elem> at Geom+0x10
    int n;                              // 0x00 corner count
    int ids[4];                         // 0x04 point ids
    char pad14[0x24 - 0x14];            // 0x14
    int f24;                            // 0x24
    char pad28[0x48 - 0x28];            // 0x28
    int f48, f4c, f50;                  // 0x48 / 0x4c / 0x50 = -1
    char pad54[0x64 - 0x54];            // 0x54
    float f64;                          // 0x64 = 1.0
    unsigned flags;                     // 0x68 (0x10 always, 0x4000 when the light sum is > 1.5)
    int f6c, f70;                       // 0x6c / 0x70
    char pad74[0x84 - 0x74];
    int f84;                            // 0x84
};

struct Geom {                                       // 0x68
    std::vector<geom_elem> elems;                   // 0x00 (the proven layout: elems first, then pts)
    std::vector<geom_pt> pts;                       // 0x10
    char pad20[0x68 - 0x20];
    Geom();                                         // 0x478150
    void build();                                   // 0x47ee60
    void compute_bounds();                          // 0x4704b0
};

struct cell_rows {                      // 0x20, one table of 16-byte records per cell column
    int** rows;                         // 0x00 (h + 1 rows)
    rgl::vector4 bounds;                // 0x04
    int h;                              // 0x14
    int w;                              // 0x18
    float scale;                        // 0x1c
    void alloc(const int* nums);        // 0x462420 (nums[0] = cells along x, nums[1] = along y)
};

struct World {                          // the scene object at terrain_obj+0xc3b0dc
    char pad0[0x64];
    cell_rows* cells;                   // 0x64 current cell-row table
    void set_geom(Geom* g);             // 0x479980
    void relight(int use_shaders);      // 0x472ed0
};

Geom* g_geom;                           // the mesh being rebuilt (a local in 0x481570)

struct terrain_obj {                    // only the offsets 0x481570 reads
    char pad00[0x258];
    char lights[0x278 - 0x258];         // 0x258 40-byte light slots, active int at +0x00
    int h;                              // 0x278 extent along x (cells stride 251)
    int w;                              // 0x27c extent along y
    char pad280[0x288 - 0x280];
    terrain_cell cells[251 * 251];      // 0x288
    terrain_cell& at(int x, int y) { return cells[x * 251 + y]; }
    terrain_quad quads[250 * 250];      // 0x8e48fc .. 0xc3b0dc
    World* world;                       // 0xc3b0dc
    float spawn_scale;                  // 0xc3b0e0
    char pad_scale[0x843bcfc - 0xc3b0e4];
    rgl::vector4 bounds;                // 0x843bcfc copied into the new cell_rows
    void tessellate();                  // 0x481570
};

// 0x466e20 (cdecl): the three corner cells of half-quad `flip` of quad (x, y), as (x, y) pairs
void triangle_corners(int x, int y, int flip, int* c0, int* c1, int* c2);
// 0x4612d0 (cdecl): normal of the triangle spanned by the three 16-byte records at *tri
vec4d* tri_normal(vec4d* out, const void* tri);

// light-slot table at terrain+0x258; the cell lum[] entries are only used for slots flagged in 0x8ed4e14
extern unsigned char g_x8ed4e14[];              // 0x8ed4e14, 0x20-byte records, bit 1 = affects terrain

// ---------------------------------------------------------------- characters --------------------------------

struct EChar {                          // 0x1f8, table at g_chars (0x8ed4f7c)
    char pad0[0x1c0];
    char* lods;                         // 0x1c0
};

struct Animator {                       // at 0x8ed4f60
    void set_anim_a(void* a);           // 0x473800
    void set_anim_b(void* a, void* b, int kind);   // 0x4746e0
    void collect(void** a, void** b, void** c, int shaders);   // 0x47fb10
};

extern Animator g_anim;                 // 0x8ed4f60
extern int g_cur;                       // 0x8ed4f68
extern int g_x8ed4f64;                  // 0x8ed4f64
extern int g_x8ed4f74;                  // 0x8ed4f74
extern EChar* g_chars;                  // 0x8ed4f7c
extern int g_char_count;                // 0x8ed4f88
extern int g_xa181ac;                   // 0xa181ac

// ------------------------------------------------------------------------------------------------------------

// @ 0x481570
void terrain_obj::tessellate()
{
    Geom* g = new Geom;
    g_geom = g;

    cell_rows* rows = new cell_rows;
    cell_rows* cr = rows;
    if (rows) {
        rows->scale = spawn_scale;
        rows->rows = 0;
        *(rgl::vector4*)&rows->bounds = bounds;
        rows->alloc(&h);
    }

    for (int y = 0; y <= w; y++) {
        for (int x = 0; x <= h; x++) {
            terrain_cell* c = &at(x, y);
            c->rec = -1;
            int id = *(int*)c;
            rows->rows[x][y] = id;
        }
    }

    g_geom->pts.reserve((h + 2) * (w + 2));
    g_geom->elems.reserve((h + 1) * (w + 1) * 2);

    for (int y = 0; y < w; y++) {
        for (int x = 0; x < h; x++) {
            terrain_quad* qp = &quads[y * 250 + x];
            int tri[16];
            rgl::vector4 nrm[2];

            for (int k = 0; k < 2; k++) {
                triangle_corners(x, y, k, &tri[k * 8], &tri[k * 8 + 2], &tri[k * 8 + 4]);
                float cell[12];
                for (int m = 0; m < 3; m++) {
                    terrain_cell* c = &at(tri[k * 8 + m * 2], tri[k * 8 + m * 2 + 1]);
                    cell[m * 4 + 0] = c->px;
                    cell[m * 4 + 1] = c->py;
                    cell[m * 4 + 2] = c->wet;
                    *(int*)&cell[m * 4 + 3] = c->f0c;
                }
                vec4d n = *tri_normal(&n, cell);
                nrm[k].x = (float)n.x;
                nrm[k].y = (float)n.y;
                nrm[k].z = (float)n.z;
                nrm[k].w = (float)n.w;
            }

            int ncorner = 4;
            int nquad = 1;
            if (nrm[1].x * nrm[0].x + nrm[1].y * nrm[0].y + nrm[1].z * nrm[0].z > 0.999f) {
                tri[0] = x;     tri[1] = y;
                tri[2] = x + 1; tri[3] = y;
                tri[4] = x + 1; tri[5] = y + 1;
                tri[6] = x;     tri[7] = y + 1;
            } else {
                ncorner = 3;
                nquad = 2;
                for (int k = 0; k < 2; k++)
                    triangle_corners(x, y, k, &tri[k * 8], &tri[k * 8 + 2], &tri[k * 8 + 4]);
                int* row = rows->rows[x];
                *(unsigned*)(row + y * 4 + 1) |= 1;
                if (qp->diag)
                    *(unsigned*)(row + y * 4 + 1) |= 2;
            }

            for (int qn = 0; qn < nquad; qn++) {
                geom_elem rec;
                rec.n = ncorner;
                rec.f24 = 0;
                rec.f48 = -1;
                rec.f4c = -1;
                rec.f50 = -1;
                rec.f64 = 1.0f;
                rec.flags = 0;
                rec.f6c = 0;
                rec.f70 = 0;
                rec.f84 = 0;
                int vbase = g_geom->elems.size();
                float acc = 0.0f;

                int* cid = tri + qn * 8;
                for (int k = 0; k < ncorner; k++) {
                    terrain_cell* c = &at(cid[0], cid[1]);
                    if (c->rec == -1) {
                        c->rec = g_geom->pts.size();
                        geom_pt pt;
                        *(int*)&pt.px = *(int*)&c->px;
                        *(int*)&pt.py = *(int*)&c->py;
                        *(int*)&pt.wet = *(int*)&c->wet;
                        pt.f0c = c->f0c;
                        pt.f20 = 1.0f;
                        g_geom->pts.push_back(pt);
                    }

                    int* slot = (int*)((char*)this + 0x258);
                    float mult = 1.0f;
                    float* l = &c->lum[14];
                    for (int m = 0; m < 15; m++) {
                        if (*slot >= 0 && (g_x8ed4e14[*slot * 0x20] & 2))
                            acc += mult * *l;
                        slot -= 10;
                        l -= 1;
                        mult *= 1.0f - *l;
                    }
                    rec.ids[k] = c->rec;
                    cid += 2;
                }

                rec.flags |= 0x10;
                if (acc > 1.5f)
                    rec.flags |= 0x4000;
                g_geom->elems.push_back(rec);

                int* row = rows->rows[x];
                *(int*)((char*)row + y * 16 + 8 + qn * 4) = vbase;
                if (nquad == 1)
                    *(int*)((char*)row + y * 16 + 12) = vbase;
            }
        }
    }

    g_geom->build();
    g_geom->compute_bounds();
    world->set_geom(g_geom);

    cell_rows* old = world->cells;
    if (old) {
        if (old->rows) {
            for (int i = 0; i <= old->h; i++)
                delete[] old->rows[i];
            delete[] old->rows;
        }
        delete old;
    }
    world->cells = cr;
    world->relight(g_use_shaders != 0);
}

// @ 0x481c40
EChar* append_mesh(int colour, int kind, void** a, void** b, void** c, char carried, int* out)
{
    if (colour < 0)
        g_cur = 0;
    else if (colour >= g_char_count)
        g_cur = g_char_count - 1;
    else
        g_cur = colour;

    if (kind == -1)
        g_anim.set_anim_a(a);
    else
        g_anim.set_anim_b(a, b, kind);
    if (carried)
        g_x8ed4f64 = -1;
    g_anim.collect(c, c + 1, c + 2, g_xa181ac);

    EChar* ec = &g_chars[g_cur];
    int* rec = (int*)(ec->lods + g_x8ed4f74 * 0xb0 + 0x18);
    int* slot = out;
    *slot = *rec;
    return ec;
}
