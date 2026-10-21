// Slice s0042a1f0: the strategic_entity's ground-shadow update (0x42a1f0), the ground-face cache built from a
// capsule swept below the entity (0x42a480), and the world's grid box query (0x42aa50). Names provisional.
#include <math.h>
#include <stdlib.h>
#include "strat.h"

// @ 0x42a1f0
void StratEnt::update_shadows(scene* sc, bool a, bool b)
{
    float dx = pos.o.x - last_pos.x;
    float dy = pos.o.y - last_pos.y;
    float dz = pos.o.z - last_pos.z;
    float dist = (float)sqrt(dx * dx + (dz * dz + dy * dy));
    if (dist > radius * 0.2f) {
        update_ground_cache();
        last_pos = pos.o;
    }
    if ((float)(g_time[f2a0] - t2a8) * 1e-05f >= -0.0001f) {
        if (a)
            create_shadow_mesh(&shadow_a, 1);
        if (b) {
            create_shadow_mesh(&shadow_b, 0);
            mstr name("shadow_default");
            shadow_b->res = g_resmgr.lookup(name);
        }
    }
    if (a) {
        if (mode & 2) {
            if ((float)(g_time[f2b0] - t2b8) * 1e-05f >= -0.0001f) {
                float k = -1.0f / (f15c * 25.0f);
                t2b8 = g_time[f2b0] - (int)((k - (float)(rand() % 0x3dc9) * 6.3223115e-07f) * 100000.0f);
                shadow_tick();
                if (p288 != 0) {
                    shadow_a->res = (res_t*)p288;
                } else {
                    mstr name("shadow_default");
                    shadow_a->res = g_resmgr.lookup(name);
                }
            }
        }
        mat4 m;
        shadow_a->render(sc, *identity(&m));
        f170 = 1;
    }
    if (b) {
        mat4 m;
        shadow_b->render(sc, *identity(&m));
    }
}

// @ 0x42a480
void StratEnt::update_ground_cache()
{
    matrix m;
    vector4 amb;
    build_shadow_frame(&m, &amb);
    float reach = radius * 3.0f;
    capsule cap;
    cap.p0 = tp(pos, off);
    vector4 t(m.u.x * reach, m.u.y * reach, m.u.z * reach);
    cap.p1 = vector4(cap.p0.x - t.x, cap.p0.y - t.y, cap.p0.z - t.z);
    cap.radius = radius * 1.1f;
    vector4 tmp;
    const vector4* mn = cap.get_min(&tmp);
    vector4 bmin(mn->x - 0.1f, mn->y - 0.1f, mn->z - 0.1f);
    const vector4* mx = cap.get_max(&tmp);
    vector4 bmax(mx->x + 0.1f, mx->y + 0.1f, mx->z + 0.1f);
    shape* found[0x2000];
    int nfound;
    ctx->query_box(&bmin.x, &bmax.x, 0x104, found, &nfound);
    nids = 0;
    for (int i = 0; i < nfound; i++) {
        if (nids >= 0x40)
            return;
        int id = *(int*)((char*)found[i] + 4);
        if (id < 0)
            continue;
        GFace* face = &ctx->ground->faces[id];
        vector4 nu(-m.u.x, -m.u.y, -m.u.z);
        const float* nn = face->normal;
        if (!(nu.x * nn[0] + nu.y * nn[1] + nu.z * nn[2] < 0.0f))
            continue;
        if (shape_contact_count(&cap, found[i], 0, 0) == 0)
            continue;
        face = &ctx->ground->faces[*(int*)((char*)found[i] + 4)];
        vector4 c[4];
        for (int k = 0; k < face->n; k++) {
            GVert* gv = &ctx->ground->verts[face->idx[k]];
            vector4 v = gv->p;
            c[k] = m.transform_point_to_local(v);
            float inv = 0.5f / radius;
            c[k].x = inv * c[k].x;
            c[k].y = inv * c[k].y;
            c[k].z = inv * c[k].z;
        }
        if ((c[0].z <= -0.5f || c[1].z <= -0.5f || c[2].z <= -0.5f || (face->n == 4 && c[3].z <= -0.5f)) &&
            (c[0].z - c[0].x <= -0.5f || c[1].z - c[1].x <= -0.5f || c[2].z - c[2].x <= -0.5f || (face->n == 4 && c[3].z - c[3].x <= -0.5f)) &&
            (c[0].x + c[0].z <= -0.5f || c[1].x + c[1].z <= -0.5f || c[2].x + c[2].z <= -0.5f || (face->n == 4 && c[3].x + c[3].z <= -0.5f)) &&
            (c[0].z - c[0].y <= -0.5f || c[1].z - c[1].y <= -0.5f || c[2].z - c[2].y <= -0.5f || (face->n == 4 && c[3].z - c[3].y <= -0.5f)) &&
            (c[0].y + c[0].z <= -0.5f || c[1].y + c[1].z <= -0.5f || c[2].y + c[2].z <= -0.5f || (face->n == 4 && c[3].y + c[3].z <= -0.5f))) {
            ids[nids] = *(int*)((char*)found[i] + 4);
            nids++;
        }
    }
}

// @ 0x42aa50
void Ctx::query_box(const float* bmin, const float* bmax, unsigned mask, shape** out, int* count)
{
    int lo[2], hi[2];
    locate(bmin, lo);
    locate(bmax, hi);
    *count = 0;
    for (int i = lo[0]; i <= hi[0]; i++) {
        if (lo[1] <= hi[1]) {
            int flat = 1;
            for (int j = lo[1]; j <= hi[1]; j++)
                cells[i][j].collect(bmin, bmax, &flat, mask, out, count);
        }
    }
}
