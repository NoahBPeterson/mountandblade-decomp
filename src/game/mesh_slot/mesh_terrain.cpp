// mesh_terrain.cpp: TU 63 "mesh", part 5 (0x480db0..0x482240): the world-map terrain's water, re-tessellation and
// (re)generation entry points, and the character-append helper that sits between them. See MIGRATION.md.
#include "./mesh_slot.h"
#include "../terrain_b/cell_grid.h"
#include "../terrain_a/game_state.h"          // LightCam
#include "../../include/vec.h"
#include "../../rgl/mesh_x.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../animator.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_anim_table (listed directly: the audit reads only the source's own includes)

using namespace rgl;
// src/game/terrain_b/poly_mesh.h (pulled in through mesh_slot.h) declares a global ::vat with the same signature
#define vat rgl::vat

int rand();                            // 0x536a9f rand()
void seed_rng(unsigned s);              // 0x536a92 srand
extern unsigned __int64 g_bit[]; // 0x5a8e68
extern char g_cfg[]; // 0x5a9f98

// Naming note: the slices called the +0x278 extent `h` and the +0x27c extent `w`; terrain_a/terrain.h has them the other
// way round (w = last cell index in x, h = last in y). The terrain cell of terrain_a/terrain.h is the record these
// functions index as cells[x * 251 + y]: the slices' wet = pz, water_id = f10, rec = f14, nx/ny/nz = nrm, u/v = disp,
// light = f40, lum[] = w[].

struct TerrainW : TerrainX {                     // members of the world map object past the end of Terrain
};

// 0x4746e0 / 0x473800 are FaceGenome::breed / unpack (terrain_b/face_gen.h), which g_anim_view (animator.h) sees as Animator members: the
// stubs name the defining class (the header would bring the Animator's other layout)
struct FaceRace;
struct FaceGenome {
    FaceRace* unpack(const unsigned __int64* key);   // 0x473800 (returns the race record, left in EAX)
    void breed(const unsigned __int64* a, const unsigned __int64* b, int seed);   // 0x4746e0
};
// 0x42ad60 is Ctx::sample_lights (strat/strat_entity.h): the scene object's light sampling
struct Ctx {
    void sample_lights(const vector4* p, vector4* dir, vector4* amb);   // 0x42ad60
};

// ---- 0x480db0 -------------------------------------------------------------------------------------------------------

// @ 0x480db0 -- the water of the world map: the river light term, one water particle per chunk of every active water
// type, and the "river" mesh over the wet triangles
void TerrainX::create_water(void** out)
{
    Entity* e = (Entity*)((rgl::world*)grid)->create_entity(9);       // CellGrid is the rgl::world of src/rgl/scene/world.h (its reset 0x421390 is rgl::world::reset)
    float cmap = (*(float*)((char*)this + 0x843bd1c) + *(float*)((char*)this + 0x843bd20) + *(float*)((char*)this + 0x843bd24)) * 0.33f;
    rgl::world* wo = (rgl::world*)world;
    float cz = (wo->ambient.x + wo->ambient.y + wo->ambient.z) * 0.3f;

    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            terrain_cell* c = &at(x, y);
            if (c->f14 >= 0) {
                float d = -(sun_dir[0] * c->nrm.x + sun_dir[1] * c->nrm.y + sun_dir[2] * c->nrm.z);
                if (d < 0.0f)
                    d = 0.0f;
                float* arr = *(float**)((char*)wo->ground + 4);
                arr[c->f14 * 9 + 8] = d * c->f40 * cmap;
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

    vector4 col;
    col.x = sun_diffuse[0] * 0.5f + sun_ambient.x * 1.3f;
    col.y = sun_diffuse[1] * 0.5f + sun_ambient.y * 1.3f;
    col.z = sun_diffuse[2] * 0.5f + sun_ambient.z * 1.3f;
    col.w = 0.9f;
    unsigned argb = pack_color(&col);

    mesh* m = new mesh();
    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++)
            at(x, y).f10 = -1;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            for (int k = 0; k < 2; k++) {
                int tri[6];
                triangle_corners(j, i, k, &tri[0], &tri[2], &tri[4]);
                if (at(tri[0], tri[1]).pz > -0.3f && at(tri[2], tri[3]).pz > -0.3f && at(tri[4], tri[5]).pz > -0.3f)
                    continue;
                for (int q = 0; q < 3; q++) {
                    terrain_cell* c = &at(tri[q * 2], tri[q * 2 + 1]);
                    if (c->f10 != -1)
                        continue;
                    vector4 p(c->px, c->py, -0.3f);
                    int pt = m->add_point(&p);
                    vec2 uv;
                    uv.x = c->disp.x * 0.125f;
                    uv.y = c->disp.y * 0.125f;
                    vector4 pos(0.0f, 0.0f, 1.0f);
                    int zero = 0;
                    c->f10 = m->add_vertex(pt, &uv, &argb, &pos, &zero);
                    m->v50[c->f10].f48 = c->px * 0.025f;
                    m->v50[c->f10].f4c = c->py * 0.025f;
                }
                m->add_face(at(tri[0], tri[1]).f10, at(tri[2], tri[3]).f10, at(tri[4], tri[5]).f10);
                m->add_face(at(tri[0], tri[1]).f10, at(tri[4], tri[5]).f10, at(tri[2], tri[3]).f10);
            }
        }
    }

    {
        tstring name("river");
        m->res = g_resmgr.get_material(*(rgl::string*)&name);
    }
    matrix id;
    identity3((mat3*)&id);
    id.s.w = 0.0f; id.f.w = 0.0f; id.u.w = 0.0f;
    id.o.x = 0.0f; id.o.y = 0.0f; id.o.z = 0.0f; id.o.w = 1.0f;
    m->m1d4 = id;
    ((mesh_x*)m)->compute_tangents();
    ((mesh_x*)m)->compute_bounds();
    ((rgl::entity*)e)->meshes.push_back(m);
    ((rgl::entity*)e)->recalc();
    *out = m;
}

// @ 0x4814a0
void TerrainX::destroy_water(int a)
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
    void compute_bounds() { ((PointSet*)this)->compute_bounds(); }  // 0x4704b0 (PointSet: poly_mesh.h)
};
// one table of 16-byte records per cell column: terrain_b's HeightField (src/include/height_field.h) with its allocator
struct cell_rows {
    void alloc(const int* dim);   // 0x462420 (renderer.h: the allocator of the HeightField)
};
struct cell_rows_t {                    // 0x20
    int** rows;                         // 0x00 (h + 1 rows)
    vector4 bounds;                     // 0x04
    int h;                              // 0x14
    int w;                              // 0x18
    float scale;                        // 0x1c
    void alloc(const int* nums) { ((cell_rows*)this)->alloc(nums); }       // 0x462420 (nums[0] = cells along x, nums[1] = along y)
};
struct world_t {                        // the scene object at terrain_obj+0xc3b0dc (CellGrid of terrain_b)
    char pad0[0x64];
    cell_rows_t* cells;                 // 0x64 current cell-row table
    void build_faces(Geom_t* g) { ((CellGrid*)this)->build_faces((NavData*)g); }       // 0x479980 (CellGrid::build_faces, terrain_b/cell_grid.h)
    void update_probes(int use_shaders) { ((CellGrid*)this)->update_probes(use_shaders); }      // 0x472ed0
};

// 0x4612d0 (cdecl): normal of the triangle spanned by the three 16-byte records at *tri (terrain_a/wind.h; hidden return pointer)
rgl::vector4 tri_normal(const rgl::vector4* tri);
// light-slot table at terrain+0x258; the cell lum[] entries are only used for slots flagged in 0x8ed4e14
extern unsigned char g_x8ed4e14[];              // 0x8ed4e14, 0x20-byte records, bit 1 = affects terrain

// @ 0x481570 -- re-tessellate every terrain quad into two triangles (four when the half-quad normals disagree), assigning
// each touched corner cell a point in the new Geom and each quad a 0x88-byte element record with the final lighting, then
// rebuild the world mesh, install a fresh cell_rows table and relight the scene
void TerrainX::tessellate()
{
    Geom_t* g = new Geom_t;                 // the mesh being rebuilt: a local, as in the original ([esp+0x28])

    cell_rows_t* rows = new cell_rows_t;
    cell_rows_t* cr = rows;
    if (rows) {
        rows->scale = scale;
        rows->rows = 0;
        *(vector4*)&rows->bounds = *(vector4*)((char*)this + 0x843bcfc);
        rows->alloc(&w);
    }

    for (int y = 0; y <= h; y++) {
        for (int x = 0; x <= w; x++) {
            terrain_cell* c = &at(x, y);
            c->f14 = -1;
            int id = *(int*)&c->pz;                 // the height's bits (0x481630 reads cell+8)
            rows->rows[x][y * 4] = id;              // the 16-byte row entries' first dword (0x48163e: [row + y*16])
        }
    }

    g->elems().reserve((w + 1) * (h + 1) * 2);       // the original reserves the elements first (0x481695)
    g->pts().reserve((w + 2) * (h + 2));

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            terrain_quad* qp = &quads[y * 250 + x];
            int tri[16];
            vector4 nrm[2];

            for (int k = 0; k < 2; k++) {
                triangle_corners(x, y, k, &tri[k * 8], &tri[k * 8 + 2], &tri[k * 8 + 4]);
                float cell[12];
                for (int m = 0; m < 3; m++) {
                    terrain_cell* c = &at(tri[k * 8 + m * 2], tri[k * 8 + m * 2 + 1]);
                    cell[m * 4 + 0] = c->px;
                    cell[m * 4 + 1] = c->py;
                    cell[m * 4 + 2] = c->pz;
                    *(int*)&cell[m * 4 + 3] = c->ntype;
                }
                nrm[k] = tri_normal((const vector4*)cell);
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
                int vbase = g->elems().size();
                float acc = 0.0f;

                int* cid = tri + qn * 8;
                for (int k = 0; k < ncorner; k++) {
                    terrain_cell* c = &at(cid[0], cid[1]);
                    if (c->f14 == -1) {
                        c->f14 = g->pts().size();
                        geom_pt pt;
                        *(int*)&pt.px = *(int*)&c->px;
                        *(int*)&pt.py = *(int*)&c->py;
                        *(int*)&pt.wet = *(int*)&c->pz;
                        pt.f0c = c->ntype;
                        pt.f20 = 1.0f;
                        g->pts().push_back(pt);
                    }

                    int* slot = (int*)((char*)this + 0x258);
                    float mult = 1.0f;
                    float* l = &c->w[14];
                    for (int m = 0; m < 15; m++) {
                        if (*slot >= 0 && (g_x8ed4e14[*slot * 0x20] & 2))
                            acc += mult * *l;
                        slot -= 10;
                        l -= 1;
                        mult *= 1.0f - *l;
                    }
                    rec.ids[k] = c->f14;
                    cid += 2;
                }

                rec.flags |= 0x10;
                if (acc > 1.5f)
                    rec.flags |= 0x4000;
                g->elems().push_back(rec);

                int* row = rows->rows[x];
                *(int*)((char*)row + y * 16 + 8 + qn * 4) = vbase;
                if (nquad == 1)
                    *(int*)((char*)row + y * 16 + 12) = vbase;
            }
        }
    }

    g->build();
    g->compute_bounds();
    ((world_t*)world)->build_faces(g);

    cell_rows_t* old = ((world_t*)world)->cells;
    if (old) {
        if (old->rows) {
            for (int i = 0; i <= old->h; i++)
                delete[] old->rows[i];
            delete[] old->rows;
        }
        delete old;
    }
    ((world_t*)world)->cells = cr;
    ((world_t*)world)->update_probes(g_use_shaders != 0);
}

// ---- 0x481c40 ----------------------------------------------------------------------------------------------------------

#define g_anim_view (*(Animator*)&g_anim_table)          // g_anim_table (0x8ed4f60, game_globals.h) seen as the Animator
extern int g_cur;                       // 0x8ed4f68
extern int g_x8ed4f64;                  // 0x8ed4f64
extern int g_x8ed4f74;                  // 0x8ed4f74
extern EChar* g_skins_begin;                  // 0x8ed4f7c
extern int g_skin_count;                // 0x8ed4f88
extern int g_limit_hair_colors;                   // 0xa181ac

// @ 0x481c40 -- append a character mesh to the world: select the character, set up the animator, collect the lod meshes
// and return &g_skins_begin[g_cur] with the mesh count written to *out
// the third argument receives the three lod meshes and the fourth / fifth are the face codes (0x481c75: unpack [esp+0x10],
// 0x481cad: collect into [esp+0xc] +0/+4/+8); the old a/b/c order decoded the caller's output array and collected the
// meshes over the troop's face code, leaving the head mesh null (crash in build_appearance when agents spawn)
EChar* append_mesh(int colour, int kind, void** c, void** a, void** b, char carried, int* out)
{
    if (colour < 0)
        g_cur = 0;
    else if (colour >= g_skin_count)
        g_cur = g_skin_count - 1;
    else
        g_cur = colour;

    if (kind == -1)
        ((FaceGenome*)&g_anim_view)->unpack((const unsigned __int64*)a);        // 0x473800
    else
        ((FaceGenome*)&g_anim_view)->breed((const unsigned __int64*)a, (const unsigned __int64*)b, kind);     // 0x4746e0
    if (carried)
        g_x8ed4f64 = -1;
    g_anim_view.collect((char)g_limit_hair_colors, (lod_mesh**)c, (lod_mesh**)(c + 1), (lod_mesh**)(c + 2));

    EChar* ec = &g_skins_begin[g_cur];
    int* rec = (int*)((char*)ec->mats + g_x8ed4f74 * 0xb0 + 0x18);
    int* slot = out;
    *slot = *rec;
    return ec;
}

// ---- 0x481d10: world-map (re)generation -------------------------------------------------------------------------------

// @ 0x481d10 -- derive the seed fields from the caller's seed word, size the cell grid from the map scale, place every cell
// at (x, y) * scale, pick the quad diagonals from the cell heights, run the generation chain (layers, rivers, flow,
// placeables, scatter sets, noise offsets) and re-tessellate. `seed` is the scene container handed in (TerrainX::world).
#define hword (*(int*)((char*)this + 0xc))
void TerrainX::generate(int seed, int* objs, char flag)
{
    reset();

    unsigned cell = (flags14 & ((*(int*)((char*)&g_bit + 0x10)) - 1)) + 2;
    scale = (float)cell;
    seed_rng(*(int*)this & ((*(int*)((char*)&g_bit + 0xf8)) - 1) & ((*(int*)((char*)&g_bit + 0xf8)) - 1));
    flag843bd75 = (char)((char)((*(int*)((char*)&g_bit + 0x8)) - 1) & (char)((unsigned)hword >> 31)) != 0;
    wx = (float)(unsigned)(hword & ((*(int*)((char*)&g_bit + 0x50)) - 1));                 // height band -> map extent
    wy = (float)(unsigned)(((unsigned)hword >> 10) & ((*(int*)((char*)&g_bit + 0x50)) - 1));
    world = (SceneWorld*)seed;

    float inv = 1.0f / scale;
    w = (int)(inv * wx);
    h = (int)(inv * wy);
    if (w > 250)
        w = 250;
    if (h > 250)
        h = 250;
    if (w < 40)
        w = 40;
    if (h < 40)
        h = 40;

    wx = (float)w * scale;
    wy = (float)h * scale;

    float b[4];
    b[0] = 0.0f;
    b[1] = 0.0f;
    b[2] = 0.0f;
    b[3] = 0.0f;
    *(vector4*)((char*)this + 0x843bcfc) = *(vector4*)b;

    init_layers();

    for (int y = 0; y <= h; y++) {
        float p[4];
        for (int x = 0; x <= w; x++) {
            *(int*)&p[2] = 0;
            p[0] = (float)x * scale;
            p[1] = (float)y * scale;
            cells[x * 251 + y].init(p);
        }
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++)
            quads[x * 250 + y].init();
    }

    generate_heights();
    seed_rng(*(int*)((char*)this + 4) & ((*(int*)((char*)&g_bit + 0xf8)) - 1));

    ((terrain_obj*)this)->make_river();
    ((terrain_obj*)this)->smooth_heights();
    ((terrain_obj*)this)->compute_normals();
    ((terrain_obj*)this)->compute_weights();
    ((terrain_obj*)this)->pick_diagonals();
    ((terrain_obj*)this)->pick_triangle_types();
    ((terrain_obj*)this)->erode();

    if (objs != 0 && flag) {
        if ((*(int*)((char*)&g_cfg + 0x18)))
            fill_desc_named((TerrainDesc*)objs);
        else
            fill_desc_none((TerrainDesc*)objs);
    }

    seed_rng(*(int*)((char*)this + 8) & ((*(int*)((char*)&g_bit + 0xf8)) - 1));

    off_b[0] = (float)(rand() % 0x3dc9) * 0.063223116f;
    off_b[1] = (float)(rand() % 0x3dc9) * 0.063223116f;
    off_b[2] = (float)(rand() % 0x3dc9) * 0.063223116f;

    if (objs != 0) {
        for (int i = 0; i < *objs; i++)
            ((terrain_obj*)this)->load_layer(objs, i);
    }

    ((terrain_obj*)this)->compute_normals();
    ((terrain_obj*)this)->accumulate_kinds();
    build_entity();
    scatter_all();
    spawn_particles(&list_b, 0);
    spawn_particles(&list_a, 0);
    spawn_particles(&list_c, 2);
    ((terrain_obj*)this)->create_chunk_entities();
    g_shadow_map.attach_camera((CellGrid*)world);
    float v[3];
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    ((Ctx*)world)->sample_lights((vector4*)v, (vector4*)sun_dir, (vector4*)((char*)this + 0x843bd1c));     // the scene object (Ctx / CellGrid)
    tessellate();
    refresh_cells();
}
#undef hword

// @ 0x482170 -- regenerate the water of a finished map: drop both scatter sets, rebuild the water mesh, relight both particle
// lists when the high-quality water path is off, and store fresh values into the two height fields of the seed word
void TerrainX::regenerate_water(void** out, bool mode)
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
    hword = (hword & ~mask) | r;
    mask = (*(int*)((char*)&g_bit + 0x50)) - 1;
    r = (int)wy;
    if ((unsigned)r > (unsigned)mask)
        r = mask;
    hword = (hword & ~(mask << 10)) | (r << 10);
}
#undef hword

// ---- 0x47ff60: one terrain layer's mesh for one chunk ---------------------------------------------------------------------
// (The slices called it build_chunk(terrain, layer, blend, cx, cy) and emit_particle(type, v, x, y, z); rewritten from the
// disassembly: ecx = the terrain, five stack arguments (layer, blend, x, y, unused-or-z), `ret 0x14`.)
// Layer 0..14 are the ground layers (resources layer[l].fc / .f10), layer 15 the water/shore layer whose alpha comes from
// the light term table written by create_water (ground array of the scene, 9 floats per record, light at +0x20).
// The mesh of chunk (x, y) lives in arr3[layer * 25 + x * 5 + y] (arr2[x * 5 + y] for layer 15); it is rebuilt from the cells
// whose x/y fall inside the chunk, then registered with the chunk's scene entity ents[x * 5 + y].
static __forceinline unsigned round_clamp255(float v)       // fl = floor(v); i = (int)fl; if (v - fl > 0.5) i++; clamp 0..255
{
    double fl = floor((double)v);
    int i = (int)fl;
    if (!((float)(v - fl) <= 0.5f))
        i++;
    if (i < 0) return 0;
    if (i >= 0x100) return 0xff;
    return i;
}
struct vec_i4 { int pad; int* first; int* last; int* end; };    // std::vector<int>-shaped list in the entity's mesh vector (allocator word at +0)
// @ 0x47ff60
void TerrainX::emit_particle(int layerno, int blend, int cx, int cy, float z)
{
    TerrainLayer* L = &layer[layerno];
    if (L->type < 0)
        return;

    rgl::mesh* m;
    if (layerno < 15)
        m = (rgl::mesh*)arr3[layerno][cx * 5 + cy];
    else
        m = (rgl::mesh*)arr2[cx * 5 + cy];
    if (m == 0)
        m = new rgl::mesh();
    ((mesh_ext*)m)->clear_geometry();

    if (layerno < 15) {
        if (blend < 0)
            m->res = (skin*)L->fc;
        else
            m->res = (skin*)L->f10;
    } else {
        m->res = 0;
    }

    for (int y = 0; y <= h; y++)
        for (int x = 0; x <= w; x++)
            cells[x * 251 + y].f10 = -1;

    for (int yi = 0; yi < h; yi++) {
        for (int xi = 0; xi < w; xi++) {
            if (!(block * cx <= xi) ||
                !((cx == nx - 1 || xi < (cx + 1) * block) && block * cy <= yi &&
                  (cy == ny - 1 || yi < (cy + 1) * block)))
                continue;
            for (int flip = 0; flip < 2; flip++) {
                int tri[6];
                triangle_corners(xi, yi, flip, &tri[0], &tri[2], &tri[4]);
                char vis[3];
                for (int k = 0; k < 3; k++) {
                    terrain_cell* c = &cells[tri[k * 2] * 251 + tri[k * 2 + 1]];
                    if (layerno < 15) {
                        char v;
                        if (blend < 0) {
                            v = c->visible(layerno);
                        } else {
                            if (c->visible(layerno) == 0 && c->visible(blend) == 0)
                                v = 0;
                            else
                                v = 1;
                        }
                        vis[k] = v;
                        if (layerno == 0) {          // the first layer extends the light camera's bounds
                            float dx = c->px - g_shadow_map.cam->world.o.x;
                            float dy = c->py - g_shadow_map.cam->world.o.y;
                            float dz = c->pz - g_shadow_map.cam->world.o.z;
                            float lx = dz * g_shadow_map.cam->world.s.z + dy * g_shadow_map.cam->world.s.y + dx * g_shadow_map.cam->world.s.x;
                            float ly = dz * g_shadow_map.cam->world.f.z + dy * g_shadow_map.cam->world.f.y + dx * g_shadow_map.cam->world.f.x;
                            float lz = dz * g_shadow_map.cam->world.u.z + dy * g_shadow_map.cam->world.u.y + dx * g_shadow_map.cam->world.u.x;
                            if (!(g_shadow_map.cam->vmin.x < lx)) g_shadow_map.cam->vmin.x = lx;
                            if (!(g_shadow_map.cam->vmin.y < ly)) g_shadow_map.cam->vmin.y = ly;
                            if (!(g_shadow_map.cam->vmin.z < lz)) g_shadow_map.cam->vmin.z = lz;
                            if (!(lx < g_shadow_map.cam->vmax.x)) g_shadow_map.cam->vmax.x = lx;
                            if (!(ly < g_shadow_map.cam->vmax.y)) g_shadow_map.cam->vmax.y = ly;
                            if (!(lz < g_shadow_map.cam->vmax.z)) g_shadow_map.cam->vmax.z = lz;
                        }
                    } else {
                        vis[k] = 1;
                    }
                }
                if (vis[0] == 0 && vis[1] == 0 && vis[2] == 0)
                    continue;

                for (int k = 0; k < 3; k++) {
                    terrain_cell* c = &cells[tri[k * 2] * 251 + tri[k * 2 + 1]];
                    if (c->f10 != -1)
                        continue;
                    int pt = m->add_point((const vector4*)c);
                    unsigned balpha = 0xff;          // blend alpha
                    unsigned edi = 0xff;             // the cell colour level
                    float uu, vv;
                    if (layerno < 15) {
                        float f;
                        if (blend < 0) {
                            f = c->w[layerno];
                        } else {
                            f = (1.0f - c->w[blend]) * c->w[layerno] + c->w[blend];
                            balpha = round_clamp255(c->w[blend] * 255.0f);
                        }
                        edi = round_clamp255(f * c->f90 * 255.0f);
                        float t = c->py * 0.025f;
                        uu = (c->px * 0.025f) * *(float*)&L->f14;
                        vv = t * *(float*)&L->f14;
                    } else {
                        vector4 pu = g_shadow_map.to_unit(*(const vector4*)c);
                        uu = pu.x;
                        vv = pu.y;
                    }
                    vector4 rgb;
                    rgb.x = 1.0f; rgb.y = 1.0f; rgb.z = 1.0f;
                    if (layerno > 0 && layerno < 15) {
                        vector4 tmp;
                        vector4* gc = ground_color(&tmp, (const float*)c);
                        rgb.x = gc->x; rgb.y = gc->y; rgb.z = gc->z; rgb.w = gc->w;
                        vector4 tc;
                        unpack_color(&tc, (const unsigned*)&c->f8c);
                        rgb.x = rgb.x * tc.x;
                        rgb.y = rgb.y * tc.y;
                        rgb.z = rgb.z * tc.z;
                    }
                    unsigned col = 0;
                    if (g_use_shaders == 0) {
                        if (m->res == 0 || (m->res->shader->stages[0].b & 0x1f) != 0x18) {
                            if (layerno < 15) {
                                vector4 sl;
                                sun_light(&sl, &c->nrm.x, &c->f40);
                                rgb.x = sl.x * rgb.x;
                                rgb.y = sl.y * rgb.y;
                                rgb.z = sl.z * rgb.z;
                                col = pack_color(&rgb);
                            }
                        } else {
                            // tangent-space material: light the cell's own frame
                            vector4 n;
                            n.x = c->nrm.x; n.y = c->nrm.y; n.z = c->nrm.z; n.w = c->nrm.w;
                            float len = (float)sqrt(n.z * n.z + n.y * n.y + n.x * n.x);
                            if (len > 1e-10f) {
                                float inv = 1.0f / len;
                                n.x = n.x * inv; n.y = n.y * inv; n.z = n.z * inv;
                            } else {
                                n.x = 0.0f; n.y = 1.0f; n.z = 0.0f;
                            }
                            float px_ = 0.0f, py_ = 1.0f, pz_ = 0.0f;
                            vector4 t;
                            t.x = n.z * py_ - pz_ * n.y;
                            t.y = pz_ * n.x - px_ * n.z;
                            t.z = n.y * px_ - py_ * n.x;
                            len = (float)sqrt(t.z * t.z + t.y * t.y + t.x * t.x);
                            if (len > 1e-10f) {
                                float inv = 1.0f / len;
                                t.x = t.x * inv; t.y = t.y * inv; t.z = t.z * inv;
                            } else {
                                t.x = 0.0f; t.y = 1.0f; t.z = 0.0f;
                            }
                            vector4 b;
                            b.x = t.z * n.y - t.y * n.z;
                            b.y = n.z * t.x - t.z * n.x;
                            b.z = t.y * n.x - n.y * t.x;
                            len = (float)sqrt(b.z * b.z + b.y * b.y + b.x * b.x);
                            float bx, by, bz;
                            if (len > 1e-10f) {
                                float inv = 1.0f / len;
                                bx = b.x * inv; by = b.y * inv; bz = b.z * inv;
                            } else {
                                bx = 0.0f; by = 1.0f; bz = 0.0f;
                            }
                            vector4 Ld;
                            Ld.x = -sun_dir[0]; Ld.y = -sun_dir[1]; Ld.z = -sun_dir[2];
                            float Lt = Ld.z * t.z + Ld.y * t.y + Ld.x * t.x;
                            float Lb = Ld.z * bz + Ld.y * by + Ld.x * bx;
                            float Ln = Ld.z * n.z + Ld.y * n.y + Ld.x * n.x;
                            float s = ((sun_diffuse[0] + sun_diffuse[2]) + sun_diffuse[1]) * 0.33333334f * (c->f40 + 0.02f);
                            vector4 cv;
                            cv.x = Lt * s;
                            cv.y = -(Lb * s);
                            cv.z = Ln * s;
                            cv.w = 1.0f;
                            col = pack_signed_color(&cv.x);
                        }
                    } else {
                        col = pack_color(&rgb);
                    }
                    col = col & 0xffffff;
                    if (layerno == 15) {
                        col = 0x810;
                        float* arr = *(float**)((char*)((rgl::world*)world)->ground + 4);
                        float li = arr[c->f14 * 9 + 8];
                        float r = li / (z + li);
                        if (!(r >= 0.0f)) r = 0.0f;
                        else if (!(r <= 1.0f)) r = 1.0f;
                        int ia = (int)(r * 255.0f);
                        edi = ia < 0 ? 0 : (ia < 0x100 ? ia : 0xff);
                    }
                    unsigned fg2 = col | (edi << 24);
                    unsigned fg = fg2;
                    if (blend >= 0)
                        fg = (fg2 & 0xffffff) | (balpha << 24);
                    unsigned cw = fg2;
                    int f5c = fg2;
                    vec2 uvv;
                    uvv.x = uu; uvv.y = vv;
                    int vi = m->add_vertex(pt, &uvv, &cw, (const vector4*)&c->nrm, &f5c);
                    c->f10 = vi;
                    m->v50[vi].f58 = fg;
                    m->v50[c->f10].f48 = *(int*)&uu;
                    m->v50[c->f10].f4c = *(int*)&vv;
                }
                m->add_face(cells[tri[0] * 251 + tri[1]].f10, cells[tri[2] * 251 + tri[3]].f10, cells[tri[4] * 251 + tri[5]].f10);
            }
        }
    }

    if (layerno == 15) {
        arr2[cx * 5 + cy] = (int)m;
        m->color = 0xff000811;
        m->mask = 0xffff;
    } else {
        if (g_use_shaders) {
            ((mesh_x*)m)->compute_tangents();
            ((world_x*)world)->light_mesh(m, (matrix*)((char*)ents[cx * 5 + cy] + 0x58), 1);
        }
        arr3[layerno][cx * 5 + cy] = (int)m;
        m->mask = 0x17fff;
    }

    if (g_use_shaders == 0 && m->res != 0 && (m->res->shader->stages[0].b & 0x1f) == 0x18) {
        vector4 c = sun_ambient;
        m->color = pack_color(&c);
    }
    m->slots[1].f28 = 1;
    m->slots[0].f28 = 1;
    m->f214 = layerno + 11;

    rgl::entity* e = (rgl::entity*)ents[cx * 5 + cy];
    std::vector<rgl::mesh*>& em = e->meshes;
    unsigned found = (unsigned)-1;
    for (unsigned k = 0; em._Myfirst != 0 && k < (unsigned)em.size(); k++) {
        if (em._Myfirst[k] == m)
            found = k;
    }
    if ((int)found < 0) {
        ((mesh_x*)m)->compute_bounds();
        vpush(em, m);
        e->recalc();
    }
}
