// Slice s00480db0: terrain water.
// 0x480db0 writes the per-vertex river light term, emits one water particle per chunk in every active water
// type, then builds a "river" mesh from the wet terrain triangles and hands it to a new scene entity.
// 0x4814a0 is the teardown: reset the particle system, relight the two particle lists, or (in the low-quality
// path) re-point every water particle's material at the river material.
// terrain_obj layout: terrain.h; mesh layout: mesh.h (both copied from the verified s00466aa0 / s00471b30).
#include <vector>
#include <math.h>
#include "rgl.h"
#include "mesh.h"
#include "terrain.h"

extern int g_use_shaders;                       // 0x5e28fc: programmable pipeline in use
extern int g_x5aa034;                           // 0x5aa034: water quality toggle
extern int g_x5aa084;                           // 0x5aa084: shader quality / mode
extern psys g_psys;                             // 0xa70248 particle system

// @ 0x480db0
void terrain_obj::create_water(void** out)
{
    entity* e = world->create_entity(9);
    float cmap = (map_centre[0] + map_centre[1] + map_centre[2]) * 0.33f;
    float cz = (world->pos.x + world->pos.y + world->pos.z) * 0.3f;

    for (int y = 0; y <= w; y++) {
        for (int x = 0; x <= h; x++) {
            terrain_cell* c = &at(x, y);
            if (c->rec >= 0) {
                float d = -(sun_dir[0] * c->nx + sun_dir[1] * c->ny + sun_dir[2] * c->nz);
                if (d < 0.0f)
                    d = 0.0f;
                float* arr = *(float**)((char*)world->store + 4);
                arr[c->rec * 9 + 8] = d * c->light * cmap;
            }
        }
    }

    int n = 0xf;
    if (g_x5aa034 && g_use_shaders == 0)
        n = 0x10;
    for (int t = 0; t < n; t++) {
        if (t < 0xf && wt()[t].active < 0)
            continue;
        if (t == 0xf)
            g_psys.flush();
        for (int y = 0; y < ny; y++) {
            for (int x = 0; x < nx; x++) {
                int v = -1;
                if (g_use_shaders && pub60) {
                    if (t == 1)
                        v = 2;
                    else if (t == 2)
                        continue;
                }
                emit_particle(t, v, x, y, cz);
            }
        }
    }
    g_psys.flush();
    *out = 0;

    rgl::vector4 col;
    col.x = sun_dif[0] * 0.5f + sun_amb.x * 1.3f;
    col.y = sun_dif[1] * 0.5f + sun_amb.y * 1.3f;
    col.z = sun_dif[2] * 0.5f + sun_amb.z * 1.3f;
    col.w = 0.9f;
    unsigned argb = rgl::pack_color(&col);

    rgl::mesh* m = new rgl::mesh();
    for (int y = 0; y <= w; y++)
        for (int x = 0; x <= h; x++)
            at(x, y).water_id = -1;

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            for (int k = 0; k < 2; k++) {
                int tri[6];
                triangle_corners(j, i, k, &tri[0], &tri[2], &tri[4]);
                if (at(tri[0], tri[1]).wet > -0.3f && at(tri[2], tri[3]).wet > -0.3f && at(tri[4], tri[5]).wet > -0.3f)
                    continue;
                for (int q = 0; q < 3; q++) {
                    terrain_cell* c = &at(tri[q * 2], tri[q * 2 + 1]);
                    if (c->water_id != -1)
                        continue;
                    rgl::vector4 p(c->px, c->py, -0.3f);
                    int pt = m->add_point(&p);
                    rgl::vec2 uv;
                    uv.x = c->u * 0.125f;
                    uv.y = c->v * 0.125f;
                    rgl::vector4 pos(0.0f, 0.0f, 1.0f);
                    int zero = 0;
                    c->water_id = m->add_vertex(pt, &uv, &argb, &pos, &zero);
                    m->v50[c->water_id].f48 = c->px * 0.025f;
                    m->v50[c->water_id].f4c = c->py * 0.025f;
                }
                m->add_face(at(tri[0], tri[1]).water_id, at(tri[2], tri[3]).water_id, at(tri[4], tri[5]).water_id);
                m->add_face(at(tri[0], tri[1]).water_id, at(tri[4], tri[5]).water_id, at(tri[2], tri[3]).water_id);
            }
        }
    }

    {
        rgl::mstr name("river");
        m->res = rgl::g_resmgr.lookup(name);
    }
    rgl::mat4 id;
    rgl::identity3((rgl::mat3*)&id);
    id.s.w = 0.0f; id.f.w = 0.0f; id.u.w = 0.0f;
    id.o.x = 0.0f; id.o.y = 0.0f; id.o.z = 0.0f; id.o.w = 1.0f;
    m->m1d4 = id;
    m->compute_tangents();
    m->finish_load();
    e->meshes.push_back(m);
    e->add_mesh(m);
    *out = m;
}

// @ 0x4814a0
void terrain_obj::destroy_water(int a)
{
    if (g_use_shaders) {
        g_psys.reset();
        if (g_x5aa084 == 1 && g_use_shaders) {
            light_particles((char*)this + 0xc3b0e4, 1);
            light_particles((char*)this + 0x343b4ec, 0);
        }
    } else if (g_x5aa034) {
        g_psys.reset();
        for (int x = 0; x < nx; x++) {
            for (int y = 0; y < ny; y++) {
                particle* p = water_part[0][x * 5 + y];
                if (p)
                    p->mat = g_psys.water_material;
            }
        }
    }
}
