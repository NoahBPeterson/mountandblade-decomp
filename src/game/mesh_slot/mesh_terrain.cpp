// mesh_terrain.cpp: TU 63 "mesh", part 5 (0x480db0..0x482240): the world-map terrain's water, re-tessellation and
// (re)generation entry points, and the character-append helper that sits between them. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../terrain_b/cell_grid.h"
#include "../terrain_a/game_state.h"          // LightCam
#include "../../include/vec.h"
#include "../../rgl/mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_anim_table (listed directly: the audit reads only the source's own includes)
#include "../../include/ground_type.h"   // g_ground_types (0x8ed4df8): f1c bit 1 = affects terrain
#include "../module_settings.h"      // ModuleSettings, g_module_settings (0xa180f8, defined in triggers/trigger_vec.cpp)
#include "../../include/render_flags.h"
#include "../config/config_block.h"
#include "../../include/wl_marker.h"

// ---- data this module owns (values read from the image once by tools/relink/gen_data.py; docs/standalone_data.md) ----


using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

int rand();                            // 0x536a9f rand()
#include <stdlib.h>   // srand (0x536a92)
extern unsigned __int64 g_bit[]; // 0x5a8e68
#include "../config/config_block.h"   // Config g_cfg (0x5a9f98)

// Naming note: the slices called the +0x278 extent `h` and the +0x27c extent `w`; terrain_a/terrain.h has them the other
// way round (w = last cell index in x, h = last in y). The terrain cell of terrain_a/terrain.h is the record these
// functions index as cells[x * 251 + y]: the slices' wet = pz, water_id = f10, rec = f14, nx/ny/nz = nrm, u/v = disp,
// light = f40, lum[] = w[].


// 0x4746e0 / 0x473800 are FaceGenome::breed / unpack (terrain_b/face_gen.h), which g_anim_view (animator.h) sees as Animator members: the
// stubs name the defining class (the header would bring the Animator's other layout)
struct Skin;
struct FaceGenome {
    Skin* unpack(const unsigned __int64* key);   // 0x473800 (returns the race record, left in EAX)
    void breed(const unsigned __int64* a, const unsigned __int64* b, int seed);   // 0x4746e0
};
// 0x42ad60 is Ctx::sample_lights (strat/strat_entity.h): the scene object's light sampling
struct Ctx {
    void sample_lights(const vector4* p, vector4* dir, vector4* amb);   // 0x42ad60
};

static __forceinline int add_v(mesh* m, int pt, const vec2& uv, unsigned ca, const vector4& pos, const int& z) { return m->add_vertex(pt, &uv, &ca, &pos, &z); }
static __forceinline void v_neg(vector4& r, const float* d) { r.x = -d[0]; r.y = -d[1]; r.z = -d[2]; }

// Bodies the original's object saw (header inlines left out of line; row owner terrain_a.cpp 0x4644e0): an object whose
// address only reaches a visible body does not escape (docs/codegen.md section 7)
namespace rgl {
static inline unsigned min255(unsigned v) { return v < 255 ? v : 255; }
inline __declspec(noinline) unsigned pack_color(const vector4* c)
{
    unsigned a = (unsigned)(c->w * 256.0f);
    unsigned r = (unsigned)(c->x * 256.0f);
    unsigned g = (unsigned)(c->y * 256.0f);
    unsigned b = (unsigned)(c->z * 256.0f);
    return (((min255(a) << 8 | min255(r)) << 8) | min255(g)) << 8 | min255(b);
}
inline __declspec(noinline) vector4 unpack_color(const unsigned* c)
{
    vector4 v;
    const unsigned char* b = (const unsigned char*)c;
    v.w = b[3] * (1.0f / 255.0f);
    v.x = b[2] * (1.0f / 255.0f);
    v.y = b[1] * (1.0f / 255.0f);
    v.z = (*c & 0xff) * (1.0f / 255.0f);
    return v;
}
}
inline __declspec(noinline) unsigned pack_signed_color(const float* c)
{
    unsigned a = (unsigned)(c[3] * 128.0f + 128.0);
    unsigned r = (unsigned)(c[0] * 128.0f + 128.0);
    unsigned g = (unsigned)(c[1] * 128.0f + 128.0);
    unsigned b = (unsigned)(c[2] * 128.0f + 128.0);
    unsigned ca = 255;
    if (a < 256) ca = a;
    unsigned cr = r < 256 ? r : 255;
    unsigned cg = g < 256 ? g : 255;
    unsigned cb = b < 256 ? b : 255;
    return (((ca << 8 | cr) << 8) | cg) << 8 | cb;
}
static __forceinline vec2 cell_v2(float x, float y) { vec2 r; r.x = x; r.y = y; return r; }
static __forceinline void cell_set_weight(terrain_cell* c, const float& v) { c->w[0] = v; c->f90 = v; }
inline __declspec(noinline) int rgl::mesh::add_vertex(int index, const vec2* uv, const unsigned* color, const vector4* pos, const int* f5c)
{
    ensure_local();
    int n = (int)v50.size();
    v50.resize(n + 1);
    (&*v50.begin())[n].index = index;
    (&*v50.begin())[n].uv = *uv;
    (&*v50.begin())[n].color = *color;
    (&*v50.begin())[n].f5c = *f5c;
    vertex60* vv = v50._Myfirst + n;
    vv->normal = *pos;
    vv->pos = *pos;
    return n;
}
inline __declspec(noinline) int rgl::mesh::add_point(const vector4* p)
{
    ensure_local();
    int n = (int)v70.size();
    v70.resize(n + 1);
    v70[n].a = *p;
    v70[n].b = *p;
    return n;
}
inline __declspec(noinline) void terrain_cell::init(const float* q)
{
    P() = *(const rgl::vector4*)q;
    *(float*)&flags = 0.0f;
    weight = 1.0f;
    f14 = -1;
    f10 = -1;
    disp = cell_v2(q[0] * 0.7f, q[1] * 0.7f);
    dir = cell_v2(0.0f, 1.0f);
    order = -1;
    for (int i = 1; i < 15; i++)
        w[i] = 0.0f;
    f88 = 0.0f;
    cell_set_weight(this, 1.0f);
    h2 = pz;
    f8c = -1;
}

inline __declspec(noinline) float rgl::vector4::normalize()
{
    float len = (float)sqrt(x * x + y * y + z * z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        x *= inv; y *= inv; z *= inv;
    } else {
        x = 0.0f; y = 1.0f; z = 0.0f;
    }
    return len;
}
inline __declspec(noinline) vector4 tri_normal(const vector4* p)
{
    vector4 a = (p[1] - p[0]);
    vector4 b = (p[2] - p[0]);
    vector4 n;
    n.x = a.y * b.z - a.z * b.y; n.y = a.z * b.x - a.x * b.z; n.z = a.x * b.y - a.y * b.x;
    n.normalize();
    return n;
}
inline __declspec(noinline) void Terrain::triangle_corners(int x, int y, int flip, int* a, int* b, int* c)
{
    if (this->quad(x, y).diag != 0) {
        if (flip == 0) {
            a[0] = x; a[1] = y;
            b[1] = y; b[0] = x + 1;
            c[0] = x + 1; c[1] = y + 1;
        } else {
            a[1] = y; a[0] = x;
            b[0] = x + 1; b[1] = y + 1;
            c[0] = x; c[1] = y + 1;
        }
    } else {
        if (flip == 0) {
            a[1] = y; a[0] = x + 1;
            b[1] = y + 1; b[0] = x;
            c[0] = x; c[1] = y;
        } else {
            a[0] = x + 1; a[1] = y;
            b[0] = x + 1; b[1] = y + 1;
            c[0] = x; c[1] = y + 1;
        }
    }
}

// ---- 0x480db0 -------------------------------------------------------------------------------------------------------

// @ 0x480db0 -- the water of the world map: the river light term, one water particle per chunk of every active water
// type, and the "river" mesh over the wet triangles
void Terrain::create_water(void** out)
{
    rgl::entity* e = world->create_entity(9);
    float cmap = (*(float*)((char*)this + 0x843bd24) + *(float*)((char*)this + 0x843bd20) + *(float*)((char*)this + 0x843bd1c)) * 0.33f;   // summed z, y, x as at 0x480dce
    float cz;
    {
        vector4 amb = world->ambient;
        cz = (amb.x + amb.y + amb.z) * 0.3f;   // the code sums z, y, x (0x480e0e): cl rewrites the order of a local's fields
    }

    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            terrain_cell* c = &at(x, y);
            if (c->f14 >= 0) {
                vector4 ld;
                v_neg(ld, sun_dir);
                float d = ld.x * c->nrm.x + ld.y * c->nrm.y + ld.z * c->nrm.z;
                if (0.0f > d)
                    d = 0.0f;
                NavVert& nv = world->ground->verts[c->f14];
                nv.level = d * c->f40;
                nv.level *= cmap;
            }
        }
    }

    int n = 0xf;
    if ((*(int*)((char*)&g_options + 0x2c)) && g_use_shaders == 0)
        n = 0x10;
    for (int t = 0; t < n; t++) {
        if (t < 0xf && layer[t].type < 0)
            continue;
        if (t == 0xf)
            g_shadow_map.fit_bounds();                // 0x463cc0
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                int v = -1;
                if (g_use_shaders && *(void**)((char*)this + 0x60)) {
                    if (t == 1)
                        v = 2;
                    else if (t == 2)
                        continue;
                }
                emit_particle(t, v, x, y, cz);
            }
        }
    }
    g_shadow_map.fit_bounds();
    *out = 0;

    unsigned argb;
    {
    vector4 col;
    col.w = 0.9f;
    vector4 a;
    a.x = sun_diffuse.x * 0.5f; a.y = sun_diffuse.y * 0.5f; a.z = sun_diffuse.z * 0.5f;
    vector4 b;
    b.x = sun_ambient.x * 1.3f; b.y = sun_ambient.y * 1.3f; b.z = sun_ambient.z * 1.3f;
    col.x = b.x + a.x;
    col.y = b.y + a.y;
    col.z = b.z + a.z;
    argb = pack_color(&col);
    }

    mesh* m = new mesh();
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++)
            at(x, y).f10 = -1;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            for (int k = 0; k < 2; k++) {
                int tri[3][2];
                triangle_corners(j, i, k, tri[0], tri[1], tri[2]);
                if (at(tri[0][0], tri[0][1]).pz <= -0.3f || at(tri[1][0], tri[1][1]).pz <= -0.3f || at(tri[2][0], tri[2][1]).pz <= -0.3f) {
                for (int q = 0; q < 3; q++) {
                    terrain_cell* c = &at(tri[q][0], tri[q][1]);
                    if (c->f10 != -1)
                        continue;
                    vector4 p;
                    p.x = c->px;
                    p.y = c->py;
                    p.z = -0.3f;
                    int pt = m->add_point(&p);
                    vec2 uv;
                    uv.x = c->disp.x * 0.125f;
                    uv.y = c->disp.y * 0.125f;
                    c->f10 = add_v(m, pt, uv, argb, vector4(0.0f, 0.0f, 1.0f), 0);
                    vertex60& v = m->v50[c->f10];
                    Vec2 u2(c->px * 0.025f, c->py * 0.025f);
                    *(int*)&v.f48 = *(int*)&u2.x;
                    *(int*)&v.f4c = *(int*)&u2.y;
                }
                int& f2 = at(tri[2][0], tri[2][1]).f10;
                int& f1 = at(tri[1][0], tri[1][1]).f10;
                m->add_face(at(tri[0][0], tri[0][1]).f10, f1, f2);
                m->add_face(at(tri[0][0], tri[0][1]).f10, f2, f1);
                }
            }
        }
    }

    {
        tstring name("river");
        m->res = g_resmgr.get_material(name);
    }
    {
    vector4 o(0.0f, 0.0f, 0.0f);
    matrix id;
    mat3 t;
    *(mat3*)&id = *identity3(&t);
    id.o = o;
    matrix& fr = m->m1d4;
    fr = id;
    fr.s.w = 0.0f; fr.f.w = 0.0f; fr.u.w = 0.0f; fr.o.w = 1.0f;
    }
    ((rgl::mesh*)m)->compute_tangents();
    ((rgl::mesh*)m)->compute_bounds();
    (*(std::vector<int>*)&((rgl::entity*)e)->meshes).push_back((int)m);     // 0x481480: the vector<int> _Insert_n 0x410610
    ((rgl::entity*)e)->recalc();
    *out = m;
}

// @ 0x4814a0
void Terrain::destroy_water(int a)
{
    if (g_use_shaders) {
        ((ShadowMapP*)&g_shadow_map)->render();
        if ((*(int*)((char*)&g_options + 0x7c)) == 1 && g_use_shaders) {
            light_all(&list_a, 1);
            light_all(&list_b, 0);
        }
    } else if ((*(int*)((char*)&g_options + 0x2c))) {
        ((ShadowMapP*)&g_shadow_map)->render();
        for (int x = 0; x < nx; x++) {
            for (int y = 0; y < ny; y++) {
                rgl::mesh* p = (rgl::mesh*)arr2[x * 5 + y];
                if (p)
                    p->res = (skin*)g_shadow_map.target;
            }
        }
    }
}

// ---- 0x481570: re-tessellation ----------------------------------------------------------------------------------------

struct geom_pt {                        // 0x24, std::vector<geom_pt> at Geom+0
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
// 0x47ee60 is PolyGeom::build (mesh_pass.cpp, the polygon-topology rebuild of this container)
struct PolyGeom { void build(); };
// the polygon container built here: a NavCore (terrain_b/poly_mesh.h, constructor 0x478150: folded with the constructors of the same
// container over other element types) whose first two vectors hold geom_pt and geom_elem records
struct Geom_t : NavCore {                           // 0x68
    std::vector<geom_pt>& pts() { return *(std::vector<geom_pt>*)this; }              // 0x00 (reserve 0x476020 / push_back 0x4772f0 on Geom+0, tessellate)
    std::vector<geom_elem>& elems() { return *(std::vector<geom_elem>*)((char*)this + 0x10); }   // 0x10 (reserve 0x476130 / push_back 0x477380 on Geom+0x10)
    void build() { ((PolyGeom*)this)->build(); }                    // 0x47ee60
    void compute_bounds() { ((NavData*)this)->compute_bounds(); }  // 0x4704b0 (NavData: nav_data.h)
};


// the height field with the constructor tessellate inlines into its `new` (0x4815a1..0x4815e7)
struct HF : HeightField {
    HF(double cell_, vector4 b, const int* dim)
    {
        cell = (float)cell_;
        cols = 0;
        bounds = b;
        alloc(dim);
    }
};

// light-slot table at terrain+0x258; the cell lum[] entries are only used for slots flagged in 0x8ed4e14

// @ 0x481570 -- re-tessellate every terrain quad into two triangles (four when the half-quad normals disagree), assigning
// each touched corner cell a point in the new Geom and each quad a 0x88-byte element record with the final lighting, then
// rebuild the world mesh, install a fresh HeightField and relight the scene
void Terrain::tessellate()
{
    // x and y shared by both loop nests and initialised before g: the initialisers are the first mention, so x gets the
    // register-candidate number below g, cr and y; that numbering is what puts the reloads after the corner loop in the
    // original's order (edi, ebx [0x44], esi [0x14]: regalloc.md 3.6.9, split pieces take free-list numbers by parent)
    int x = 0, y = 0;
    Geom_t* g = (Geom_t*)new NavCore;       // the mesh being rebuilt: a local, as in the original ([esp+0x28])

    HeightField* cr = new HF(scale, *(vector4*)((char*)this + 0x843bcfc), &w);
    HeightField* rows = cr;

    for (y = 0; y <= h; y++) {
        for (x = 0; x <= w; x++) {
            terrain_cell* c = &at(x, y);
            int id = *(int*)&c->pz;                 // the height's bits (0x481630 reads cell+8)
            c->f14 = -1;
            *(int*)&rows->cols[x][y].h = id;              // the 16-byte row entries' first dword (0x48163e: [row + y*16])
        }
    }

    int npts = (w + 2) * (h + 2);
    g->elems().reserve((w + 1) * (h + 1) * 2);       // the original reserves the elements first (0x481695)
    g->pts().reserve(npts);

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            terrain_quad* qp = &quads[x * 250 + y];     // 0x4816d6: row stride 0x38, column stride 0x36b0 (index x * 250 + y)
            int tri[2][4][2];
            vector4 nrm[2];

            for (int k = 0; k < 2; k++) {
                triangle_corners(x, y, k, tri[k][0], tri[k][1], tri[k][2]);
                vector4 cell[3];
                { int ix = tri[k][0][0]; cell[0] = *(vector4*)&at(ix, tri[k][0][1]); }
                cell[1] = *(vector4*)&at(tri[k][1][0], tri[k][1][1]);
                cell[2] = *(vector4*)&at(tri[k][2][0], tri[k][2][1]);
                nrm[k] = tri_normal(cell);
            }

            int ncorner = 4;
            int nquad = 1;
            if (nrm[0].x * nrm[1].x + nrm[0].z * nrm[1].z + nrm[0].y * nrm[1].y > 0.999f) {      // cl sums these right to left: y, z, x as at 0x4817f9
                tri[0][0][0] = x;     tri[0][0][1] = y;
                tri[0][1][0] = x + 1; tri[0][1][1] = y;
                tri[0][2][0] = x + 1; tri[0][2][1] = y + 1;
                tri[0][3][0] = x;     tri[0][3][1] = y + 1;
            } else {
                ncorner = 3;
                nquad = 2;
                for (int k = 0; k < 2; k++)
                    triangle_corners(x, y, k, tri[k][0], tri[k][1], tri[k][2]);
                {
                    int& fl = *(int*)&rows->cols[x][y].flags;
                    fl |= 1;
                }
                if (qp->diag) {
                    int& fl = *(int*)&rows->cols[x][y].flags;
                    fl |= 2;
                }
            }

            for (int qn = 0; qn < nquad; qn++) {
                int vbase = g->elems().size();
                float acc = 0.0f;
                geom_elem rec;
                rec.f6c = 0;
                rec.f70 = 0;
                rec.flags = 0;
                rec.f84 = 0;
                rec.f64 = 1.0f;
                rec.f48 = -1;
                rec.f4c = -1;
                rec.f50 = -1;
                rec.f24 = 0;
                rec.n = ncorner;

                for (int k = 0; k < ncorner; k++) {
                    terrain_cell* c = &at(tri[qn][k][0], tri[qn][k][1]);
                    if (c->f14 == -1) {
                        c->f14 = g->pts().size();
                        geom_pt pt;
                        *(vector4*)&pt = *(vector4*)c;
                        pt.f20 = 1.0f;
                        g->pts().push_back(pt);
                    }

                    float mult = 1.0f;
                    float* l = &c->w[14];
                    int* slot = (int*)((char*)this + 0x258);
                    for (int m = 15; m != 0; m--) {     // a countdown, as the original (dec esi; jne at 0x481aa7): cl unrolls an up-count
                        if (*slot >= 0 && (g_ground_types[*slot].f1c & 2))
                            acc += mult * *l;
                        l -= 1;
                        mult *= 1.0f - l[1];        // 0x481aa1: fsub [ecx + 4] after the pointer step, i.e. the entry just used
                        slot -= 10;
                    }
                    rec.ids[k] = c->f14;
                }

                rec.flags |= 0x10;
                if (acc > 1.5f)
                    rec.flags |= 0x4000;
                g->elems().push_back(rec);

                (&rows->cols[x][y].id)[qn] = vbase;
                if (nquad == 1)
                    rows->cols[x][y].id2 = vbase;
            }
        }
    }

    g->build();
    g->compute_bounds();
    world->build_faces((NavData*)g);

    rgl::world* wo = world;
    HeightField* old = wo->hf;
    if (old) {                                 // ~HeightField (0x41ecc0) expanded in place
        if (old->cols) {
            for (int i = 0; i <= old->w; i++)
                delete[] (char*)old->cols[i];
            delete[] (char*)old->cols;
        }
        operator delete(old);
    }
    wo->hf = cr;
    world->update_probes(g_use_shaders != 0);
}

// ---- 0x481c40 ----------------------------------------------------------------------------------------------------------

#define g_anim_view g_anim                              // 0x8ed4f60 (animator.h)

// @ 0x481c40 -- append a character mesh to the world: select the character, set up the animator, collect the lod meshes
// and return &g_anim.skin_vec._Myfirst[g_anim.cur] with the mesh count written to *out
// the third argument receives the three lod meshes and the fourth / fifth are the face codes (0x481c75: unpack [esp+0x10],
// 0x481cad: collect into [esp+0xc] +0/+4/+8); the old a/b/c order decoded the caller's output array and collected the
// meshes over the troop's face code, leaving the head mesh null (crash in build_appearance when agents spawn)
Skin* append_mesh(int colour, int kind, void** c, void** a, void** b, char carried, int* out)
{
    if (colour < 0)
        g_anim.cur = 0;
    else if (colour >= g_anim.char_count)
        g_anim.cur = g_anim.char_count - 1;
    else
        g_anim.cur = colour;

    if (kind == -1)
        g_anim_view.unpack((const unsigned __int64*)a);        // 0x473800
    else
        g_anim_view.breed((const unsigned __int64*)a, (const unsigned __int64*)b, kind);     // 0x4746e0
    if (carried)
        g_anim.item = -1;
    g_anim_view.collect(g_module_settings.limit_hair_colors != 0, (lod_mesh**)c, (lod_mesh**)(c + 1), (lod_mesh**)(c + 2));

    Skin& ec = g_anim.skin_vec[g_anim.cur];
    *out = *(int*)((char*)&ec.face_textures[g_anim.material] + 0x18);
    return &ec;
}

// ---- 0x481d10: world-map (re)generation -------------------------------------------------------------------------------

// @ 0x481d10 -- derive the seed fields from the caller's seed word, size the cell grid from the map scale, place every cell
// at (x, y) * scale, pick the quad diagonals from the cell heights, run the generation chain (layers, rivers, flow,
// placeables, scatter sets, noise offsets) and re-tessellate. `seed` is the scene container handed in (Terrain::world).
#define hword (*(int*)((char*)this + 0xc))
void Terrain::generate(int seed, int* objs, char flag)
{
    reset();

    int smask = (*(int*)((char*)&g_bit + 0xf8)) - 1;
    const int& s0 = *(int*)this & smask;
    const int& cm = (*(int*)((char*)&g_bit + 0x10)) - 1;
    unsigned cell = (flags14 & cm) + 2;
    scale = (float)cell;
    srand(s0 & smask);
    int& hw = hword;
    {
        const int& fm = (*(int*)((char*)&g_bit + 0x8)) - 1;
        flag843bd75 = (char)((char)fm & (char)((unsigned)hw >> 31)) != 0;
    }
    {
 const int& m = (*(int*)((char*)&g_bit + 0x50)) - 1;
        wx = (float)(unsigned)(hw & m);                 // height band -> map extent
    }
    {
 const int& m = (*(int*)((char*)&g_bit + 0x50)) - 1;
        wy = (float)(unsigned)(((unsigned)hw >> 10) & m);
    }
    world = (rgl::world*)seed;
    WL_MARKER(0xc);

    float inv = 1.0f / scale;
    int& rw = w;
    rw = (int)(inv * wx);
    h = (int)(inv * wy);
    if (w > 250)
        rw = 250;
    if (h > 250)
        h = 250;
    if (w < 40)
        w = 40;
    if (h < 40)
        h = 40;

    wx = (float)w * scale;
    wy = (float)h * scale;

    {
        vector4& bd = *(vector4*)((char*)this + 0x843bcfc);
        bd = vector4(0.0f, 0.0f, 0.0f);
    }
    WL_MARKER(0xd);

    init_layers();

    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            vector4 p;
            p.x = (float)x * scale;
            p.y = (float)y * scale;
            p.z = 0.0f;
            cells[x * 251 + y].init(&p.x);
        }
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)
            quads[x * 250 + y].init();
    }

    generate_heights();
    {
        const int& m = (*(int*)((char*)&g_bit + 0xf8)) - 1;
        srand(*(int*)((char*)this + 4) & m);
    }

    ((Terrain*)this)->make_river();
    ((Terrain*)this)->smooth_heights();
    ((Terrain*)this)->compute_normals();
    ((Terrain*)this)->compute_weights();
    ((Terrain*)this)->pick_diagonals();
    ((Terrain*)this)->pick_triangle_types();
    ((Terrain*)this)->erode();

    if (objs != 0 && flag) {
        if (g_cfg.enable_edit_mode)
            fill_desc_named((TerrainDesc*)objs);
        else
            fill_desc_none((TerrainDesc*)objs);
    }

    {
        const int& m = (*(int*)((char*)&g_bit + 0xf8)) - 1;
        srand(*(int*)((char*)this + 8) & m);
    }

    off_b[0] = (float)(rand() % 0x3dc9) * 0.063223116f;
    off_b[1] = (float)(rand() % 0x3dc9) * 0.063223116f;
    off_b[2] = (float)(rand() % 0x3dc9) * 0.063223116f;

    if (objs != 0) {
        for (int i = 0; i < *objs; i++)
            ((Terrain*)this)->load_layer(objs, i);
    }

    ((Terrain*)this)->compute_normals();
    ((Terrain*)this)->accumulate_kinds();
    build_entity();
    scatter_all();
    spawn_particles(&list_b, 0);
    spawn_particles(&list_a, 0);
    spawn_particles(&list_c, 2);
    ((Terrain*)this)->create_chunk_entities();
    g_shadow_map.attach_camera(world);
    {
        vector4 v(0.0f, 0.0f, 0.0f);
        world->sample_lights(&v, (vector4*)sun_dir, (vector4*)((char*)this + 0x843bd1c));
    }
    tessellate();
    refresh_cells();
}
#undef hword

// @ 0x482170 -- regenerate the water of a finished map: drop both scatter sets, rebuild the water mesh, relight both particle
// lists when the high-quality water path is off, and store fresh values into the two height fields of the seed word
void Terrain::regenerate_water(void** out, bool mode)
{
    spawn_bodies((SpawnSet*)&list_b);
    spawn_bodies((SpawnSet*)&list_a);
    *out = 0;
    create_water(out);
    gen_set_init(&list_a);
    gen_set_init(&list_b);
    make_grid();
    if (g_use_shaders == 0 || (*(int*)((char*)&g_options + 0x7c)) == 0) {
        light_all(&list_a, mode);                  // 0x4821cb: the argument dword pushed as it came in
        light_all(&list_b, mode);
    }

#define hword (*(int*)((char*)this + 0xc))
    int mask = (*(int*)((char*)&g_bit + 0x50)) - 1;
    int r = (int)wx;
    if ((unsigned)r > (unsigned)mask)
        r = mask;
    int& h = hword;                 // through a reference: the store is an alias barrier, wy's fld stays after it
    h = (h & ~mask) | r;
    mask = (*(int*)((char*)&g_bit + 0x50)) - 1;
    r = (int)wy;
    if ((unsigned)r > (unsigned)mask)
        r = mask;
    h = (h & ~(mask << 10)) | (r << 10);
}
#undef hword

// ---- 0x47ff60: one terrain layer's mesh for one chunk ---------------------------------------------------------------------
// (The slices called it build_chunk(terrain, layer, blend, cx, cy) and emit_particle(type, v, x, y, z); rewritten from the
// disassembly: ecx = the terrain, five stack arguments (layer, blend, x, y, unused-or-z), `ret 0x14`.)
// Layer 0..14 are the ground layers (resources layer[l].fc / .f10), layer 15 the water/shore layer whose alpha comes from
// the light term table written by create_water (ground array of the scene, 9 floats per record, light at +0x20).
// The mesh of chunk (x, y) lives in arr3[layer * 25 + x * 5 + y] (arr2[x * 5 + y] for layer 15); it is rebuilt from the cells
// whose x/y fall inside the chunk, then registered with the chunk's scene entity ents[x * 5 + y].
static __forceinline int round_i(const float& v)       // fl = floor(v); i = (int)fl; if (v - fl > 0.5) i++
{
    float fl = (float)floor(v);
    int i = (int)fl;
    return v - fl <= 0.5f ? i : i + 1;
}
static __forceinline unsigned clamp255(int i)
{
    return i < 0 ? 0 : (i >= 0x100 ? 0xff : i);
}
static __forceinline unsigned round_clamp255(const float& v)
{
    return clamp255(round_i(v));
}
struct vec_i4 { int pad; int* first; int* last; int* end; };    // std::vector<int>-shaped list in the entity's mesh vector (allocator word at +0)
// the inline vector helpers this function expands
static __forceinline void v_normalize(vector4& v)
{
    float len = (float)sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv;
        v.y = v.y * inv;
        v.z = v.z * inv;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}
static __forceinline float v_dot(const vector4& a, const vector4& b)
{
    return a.z * b.z + a.y * b.y + a.x * b.x;
}

static __forceinline vector4 v_cross(const vector4& a, const vector4& b)
{
    vector4 r;
    r.x = a.y * b.z - b.y * a.z;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}
static __forceinline void v_min(vector4& a, const vector4& p)
{
    float v = a.x; if (!(v < p.x)) v = p.x; a.x = v;
    v = a.y; if (!(v < p.y)) v = p.y; a.y = v;
    v = a.z; if (!(v < p.z)) v = p.z; a.z = v;
}
static __forceinline void v_max(vector4& a, const vector4& p)
{
    float v = a.x; if (!(v > p.x)) v = p.x; a.x = v;
    v = a.y; if (!(v > p.y)) v = p.y; a.y = v;
    v = a.z; if (!(v > p.z)) v = p.z; a.z = v;
}
// @ 0x47ff60
void Terrain::emit_particle(int layerno, int blend, int cx, int cy, float z)
{
    if (layer[layerno].type < 0)
        return;

    rgl::mesh* m;
    if (layerno < 15)
        m = (rgl::mesh*)((int(*)[5][5])arr3)[layerno][cx][cy];
    else
        m = (rgl::mesh*)((int(*)[5])arr2)[cx][cy];
    if (m == 0)
        m = new rgl::mesh();
    ((mesh*)m)->clear_geometry();

    if (layerno < 15) {
        if (blend >= 0)
            m->res = (skin*)layer[layerno].f10;
        else
            m->res = (skin*)layer[layerno].fc;
    } else {
        m->res = 0;
    }

    int yi, xi;
    for (yi = 0; yi <= h; yi++)
        for (xi = 0; xi <= w; xi++)
            cells[xi * 251 + yi].f10 = -1;

    for (yi = 0; yi < h; yi++) {
        for (xi = 0; xi < w; xi++) {
            if (xi < block * cx || (cx != nx - 1 && xi >= (cx + 1) * block) || yi < block * cy ||
                (cy != ny - 1 && yi >= (cy + 1) * block))
                continue;
            for (int flip = 0; flip < 2; flip++) {
                int tri[4][2];
                triangle_corners(xi, yi, flip, tri[0], tri[1], tri[2]);
                char vis[3];
                for (int k = 0; k < 3; k++) {
                    terrain_cell* c = &cells[tri[k][0] * 251 + tri[k][1]];
                    if (layerno < 15) {
                        if (blend < 0)
                            vis[k] = c->visible(layerno);
                        else
                            vis[k] = (c->visible(layerno) || c->visible(blend)) ? 1 : 0;
                        if (layerno == 0) {          // the first layer extends the light camera's bounds
                            float dx = c->px - g_shadow_map.cam->world.o.x;
                            float dy = c->py - g_shadow_map.cam->world.o.y;
                            float dz = c->pz - g_shadow_map.cam->world.o.z;
                            vector4 lp;
                            lp.x = dz * g_shadow_map.cam->world.s.z + dy * g_shadow_map.cam->world.s.y + dx * g_shadow_map.cam->world.s.x;
                            lp.y = dz * g_shadow_map.cam->world.f.z + dy * g_shadow_map.cam->world.f.y + dx * g_shadow_map.cam->world.f.x;
                            lp.z = dz * g_shadow_map.cam->world.u.z + dy * g_shadow_map.cam->world.u.y + dx * g_shadow_map.cam->world.u.x;
                            v_min(g_shadow_map.cam->vmin, lp);
                            v_max(g_shadow_map.cam->vmax, lp);
                        }
                    } else {
                        vis[k] = 1;
                    }
                }
                if (vis[0] == 0 && vis[1] == 0 && vis[2] == 0)
                    continue;

                for (int k = 0; k < 3; k++) {
                    terrain_cell* c = &cells[tri[k][0] * 251 + tri[k][1]];
                    if (c->f10 != -1)
                        continue;
                    int pt = m->add_point((const vector4*)c);
                    unsigned col;
                    unsigned edi = 0xff;             // the cell colour level
                    unsigned balpha = 0xff;          // blend alpha
                    Vec2 uv;
                    float f;
                    if (layerno < 15) {
                        if (blend < 0) {
                            f = c->w[layerno];
                        } else {
                            f = (1.0f - c->w[blend]) * c->w[layerno] + c->w[blend];
                            balpha = round_clamp255(c->w[blend] * 255.0f);
                        }
                        edi = round_clamp255(f * c->f90 * 255.0f);
                        float s = *(float*)&layer[layerno].f14;
                        Vec2 t(c->px * 0.025f, c->py * 0.025f);
                        uv = Vec2(t.x * s, t.y * s);
                    } else {
                        vector4 pu = g_shadow_map.to_unit(*(const vector4*)c);
                        uv = *(Vec2*)&pu;
                    }
                    vector4 rgb(1.0f, 1.0f, 1.0f);     // the constructor, not field stores: rgb.x * tc.x then loads rgb first (0x480553)
                    if (layerno > 0 && layerno < 15) {
                        vector4 tmp;
                        rgb = *ground_color(&tmp, (const float*)c);
                        vector4 tc = rgl::unpack_color((const unsigned*)&c->f8c);
                        rgb.x *= tc.x;
                        rgb.y *= tc.y;
                        rgb.z *= tc.z;
                    }
                    if (g_use_shaders == 0) {
                        if (m->res == 0 || (m->res->shader->stages[0].b & 0x1f) != 0x18) {
                            if (layerno < 15) {
                                vector4 sl = sun_light(&c->nrm.x, &c->f40);
                                rgb.x = sl.x * rgb.x;
                                rgb.y = sl.y * rgb.y;
                                rgb.z = sl.z * rgb.z;
                                col = pack_color(&rgb);
                            }
                        } else {
                            // tangent-space material: light the cell's own frame
                            vector4 n0 = c->N();
                            v_normalize(n0);
                            rot3 fr;                 // the cell's tangent frame: u = normal, s, f orthonormalized against it
                            fr.u = n0;
                            fr.f = vector4(0.0f, 1.0f, 0.0f);
                            fr.s = v_cross(fr.f, fr.u);
                            v_normalize(fr.s);
                            fr.f = v_cross(fr.u, fr.s);
                            v_normalize(fr.f);
                            vector4 Ld;
                            v_neg(Ld, sun_dir);
                            vector4 cv;
                            cv.x = v_dot(Ld, fr.s);
                            cv.y = v_dot(Ld, fr.f);
                            cv.z = v_dot(Ld, fr.u);
                            cv.w = 1.0f;
                            float avg = ((sun_diffuse.x + sun_diffuse.z) + sun_diffuse.y) * 0.33333334f;
                            double s = avg * (c->f40 + 0.02f);     // a double: kept to the end, fstp st(0) at 0x480959
                            cv.x *= s;
                            cv.y *= s;
                            cv.z *= s;
                            cv.y = -cv.y;
                            col = pack_signed_color(&cv.x);
                        }
                    } else {
                        col = pack_color(&rgb);
                    }
                    col = col & 0xffffff;
                    if (layerno == 15) {
                        col = 0x810;
                        NavVert& nv = world->ground->verts[c->f14];
                        float a = nv.level;
                        nv.level = a;                  // stand-in: emits no store, but leaves the dead lea at 0x480a09 (not it: a pointer, a float&, a by-value copy, a const NavVert& helper, nv.level read twice in the division)
                        float r = a / (z + a);
                        if (r < 0.0f) r = 0.0f;
                        else if (r > 0.75f) r = 0.75f;
                        int ia = (int)(r * 255.0f);
                        edi = clamp255(ia);
                    }
                    col = col | (edi << 24);
                    unsigned fg = col;
                    if (blend >= 0)
                        fg = (col & 0xffffff) | (balpha << 24);
                    int f5c = col;
                    unsigned cw = col;
                    c->f10 = m->add_vertex(pt, (const vec2*)&uv, &cw, (const vector4*)&c->nrm, &f5c);
                    { vertex60& v = m->v50[c->f10]; v.f58 = col; v.f58 = fg; }     // stand-in: two uses at folding time give add eax,edx at 0x480ad0 (not it: one store, a vertex60& helper, `= col; if (blend >= 0) = fg`)
                    { vertex60& v = m->v50[c->f10]; v.f48 = *(int*)&uv.x; *(int*)&v.f4c = *(int*)&uv.y; }
                }
                m->add_face(cells[tri[0][0] * 251 + tri[0][1]].f10, cells[tri[1][0] * 251 + tri[1][1]].f10, cells[tri[2][0] * 251 + tri[2][1]].f10);
            }
        }
    }

    if (layerno == 15) {
        ((int(*)[5])arr2)[cx][cy] = (int)m;
        m->color = 0xff000811;
        m->mask = 0xffff;
    } else {
        if (g_use_shaders) {
            ((rgl::mesh*)m)->compute_tangents();
            world->light_mesh(m, (matrix*)((char*)ents[cx * 5 + cy] + 0x58), 1);
        }
        ((int(*)[5][5])arr3)[layerno][cx][cy] = (int)m;
        m->mask = 0x17fff;
    }

    if (g_use_shaders == 0 && m->res != 0 && (m->res->shader->stages[0].b & 0x1f) == 0x18) {
        vector4 c = sun_ambient;
        m->color = pack_color(&c);
    }
    char& a = m->slots[1].f28;
    a = 1;
    char& b = m->slots[0].f28;
    b = 1;
    int& key = m->f214;
    key = layerno + 11;

    rgl::entity* e = (rgl::entity*)ents[cx * 5 + cy];
    std::vector<rgl::mesh*>& em = e->meshes;
    int found = -1;
    for (unsigned k = 0; k < em.size(); k++) {
        rgl::mesh*& r = e->meshes[k];
        if (r == m && r == m)      // stand-in: the second read folds away and leaves the dead lea at 0x480d1d (not it: a value copy, e->meshes[k] == m, a const-ref helper)
            found = k;
    }
    if (found < 0) {
        ((rgl::mesh*)m)->compute_bounds();
        (*(std::vector<int>*)&em).push_back((int)m);     // 0x480d81: the vector<int> _Insert_n 0x410610
        e->recalc();
    }
}
