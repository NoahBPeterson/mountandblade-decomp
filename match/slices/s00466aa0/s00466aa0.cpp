// Slice s00466aa0: world-map terrain object, second half: quad weight accumulation, erosion noise, particle
// emission into the three per-type lists, quad triangle corners, sun lighting, chunk entity creation. Names provisional.
#include <windows.h>
#include <stdlib.h>
#include "s00466aa0.h"

struct ivec2 { int dx, dy; };
extern ivec2 g_nbr[];                                   // 0x591340 neighbour offsets
ptype* g_ptypes;                                        // 0x5e28f8 (defined elsewhere; declared here)

// Stand-in body for the cell classifier at 0x4646b0 (not part of this slice; it lives in another one): defining it in this
// translation unit lets cl see that the call does not write memory, which is what the original build knew
// (it keeps this->w cached in a register across the call). The harness maps calls to the real function.
__declspec(noinline) int terrain_cell::kind()
{
    float* w = &v4c;
    int r = 0;
    for (int i = 0; i < 14; i++)
        if (0.4f < w[i]) r = i + 1;
    return r;
}

// @ 0x466aa0
void terrain_obj::accumulate_kinds()
{
    for (int y = 0; y < this->h; y++) {
        if (this->w > 0) {
            int order[4] = { 8, 2, 1, 0 };
            for (int x = 0; x < this->w; x++) {
                int v[4];
                int cnt = 0;
                float sum = 0.0f;
                for (int k = 0; k < 4; k++) {
                    int nx = g_nbr[order[k]].dx + x;
                    int ny = g_nbr[order[k]].dy + y;
                    terrain_cell& c = this->at(nx, ny);
                    if (nx >= 0 && nx <= this->w && ny >= 0 && ny <= this->h)
                        v[k] = c.kind();
                    else
                        v[k] = -1;
                    if (v[k] == 2) cnt++;
                    sum += c.v50;
                }
                if (cnt > 2 && sum > 2.75f)
                    this->quad(x, y).f34 = sum * 0.22727272f;
            }
        }
    }
}

// @ 0x466c10
void terrain_obj::erode()
{
    for (int y = 0; y <= this->h; y++) {
        for (int x = 0; x <= this->w; x++) {
            terrain_cell& c = this->at(x, y);
            if (c.v50 + c.v4c < 0.5f) {
                float r = (float)(rand() % 0x3dc9) * 6.322312e-05f;
                float f = ((r - 0.5f) * (1.0f - c.v4c)) * this->spawn_scale * 0.7f;
                c.h2 += f;
                c.pz += f;
            }
        }
    }
}

// @ 0x466cd0
void terrain_obj::emit_particle(particle_list* l, int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim)
{
    if (l->count < 0x40000) {
        particle* e = &l->items[l->count];
        e->f9c = 0;
        e->type = type;
        int c[2];
        cell_of(c, &frame->o.x);
        int cx = c[0], cy = c[1];
        e->anim = anim;
        e->cy = cy;
        e->cx = cx;
        e->f0c = 0;
        ptype* t = &g_ptypes[type];
        anim += t->nframes * 30;
        e->anim = anim;
        e->anim = anim % t->nframes;
        e->dir = *dir;
        e->frame = *frame;
        l->count++;
    }
}

// @ 0x466d90
void terrain_obj::emit_by_type(int type, const rgl::matrix* frame, const rgl::vector4* dir, int anim)
{
    __int64 flags = g_ptypes[type].flags;
    if (flags & 0x80000) {
        emit_particle(&list_c, type, frame, dir, anim);
        return;
    }
    if (flags & 0x400000)
        emit_particle(&list_b, type, frame, dir, anim);
    else
        emit_particle(&list_a, type, frame, dir, anim);
}

// @ 0x466e20
void terrain_obj::triangle_corners(int x, int y, int flip, int* a, int* b, int* c)
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

// @ 0x466ed0
void terrain_obj::sun_light(rgl::vector4* out, const float* dir, const float* scale)
{
    rgl::vector4 amb = sun_ambient;
    float t = -(sun_dir[2] * dir[2] + sun_dir[1] * dir[1] + sun_dir[0] * dir[0]);
    if (0.0f > t) t = 0.0f;
    t = t * scale[0];
    rgl::vector4 d(t * sun_diffuse[0], t * sun_diffuse[1], t * sun_diffuse[2]);
    rgl::vector4 r;                 // r.w is never written: the original copies the stack slot as is
    r.x = amb.x + d.x;
    r.y = amb.y + d.y;
    r.z = amb.z + d.z;
    *out = r;
}

// @ 0x466fa0
void terrain_obj::create_chunk_entities()
{
    entity** p = ents;
    for (int a = 5; a; a--) {
        for (int b = 5; b; b--) {
            p[0] = 0;
            ((int*)p)[25] = 0;                  // arr2[i]
            int* q = (int*)p + 50;              // arr3[0][i]
            for (int r = 15; r; r--) { *q = 0; q += 25; }
            p++;
        }
    }
    int ww = w;
    nx = ww / block;
    if (block / 2 < ww % block) nx++;
    int t = nx;
    if (t < 1) t = 1;
    else if (t >= 5) t = 4;
    nx = t;
    int hh = h;
    ny = hh / block;
    if (block / 2 < hh % block) ny++;
    t = ny;
    if (t < 1) t = 1;
    else if (t >= 5) t = 4;
    ny = t;
    for (int j = 0; j < ny; j++) {
        for (int i = 0; i < nx; i++) {
            entity** slot = &ents[i * 5 + j];
            *slot = world->create_entity(9);
            int ch = block;
            int cw = ch;
            if (i == nx - 1) {
                cw = w;
                if (ch / 2 < cw % ch) cw = cw % ch;
                else cw = cw % block + block;
            }
            if (j == ny - 1) {
                if (block / 2 < h % block) ch = h % ch;
                else ch = h % block + block;
            }
            int zero = 0;
            (*slot)->s6(&zero);
            terrain_cell& cell = cells[block * (i * 251 + j) + (cw / 2) * 251 + ch / 2];
            (*slot)->pos = *(rgl::vector4*)&cell;
            (*slot)->pos.z += 2.0f;
        }
    }
}
