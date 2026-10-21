// Slice s0042b080: the strategic_entity's shadow-mesh builder (0x42b080) and the scene's ambient colour
// (0x42b6d0, in s0042b080_b.cpp: it must be compiled apart from this function, see there; equiv.py needs
// --src match/slices/s0042b080/s0042b080_b.cpp for it). Names provisional.
#include <math.h>
#include "shadow.h"

// @ 0x42b080
// Builds (or refills) the shadow decal mesh from the ground faces under the entity: one vertex per
// ground vertex lifted along its normal, with texture coordinates projected on the shadow camera and an
// alpha that falls off with the distance from the entity; 1..3 sample vertices per ground vertex.
void StratEnt::create_shadow_mesh(mesh** slot, char flag)
{
    float ir;
    int nt;
    float ik;
    int j;
    int pi;
    vector4 p;
    int i;
    vec2 off[3];
    float s0;
    unsigned col;
    float u, v;
    vec2 uv;
    int f5c;
    int q;
    GFace* face;
    const vector4* n;
    float k, dx, dy, dz, shade, c, dx2, dy2, dz2, a, t;
    int al;
    matrix fr;
    vector4 amb;
    int pt[4];
    int vtx[3][4];
    build_shadow_frame(&fr, &amb);
    if (*slot == 0) {
        *slot = new mesh;
        {
            mstr name("shadow_default");
            (*slot)->res = g_resmgr.lookup(name);
        }
        (*slot)->slots[1].f4 = 1;
        (*slot)->slots[0].f4 = 1;
    }
    (*slot)->clear_all();
    vector4 a2 = ctx->ambient();
    nt = 1;
    s0 = ((a2.z + a2.y) + a2.x) * 0.35f;
    if (f15c > 0.5f) {
        nt = 2;
        if (f15c > 0.7f) nt = 3;
    }
    if (nt == 1) {
        off[0] = vec2(0.0f, 0.0f);
    } else if (nt == 2) {
        off[0] = vec2(-0.004296875f, 0.0046875f);
        off[1] = vec2(0.00390625f, -0.004296875f);
    } else {
        off[0] = vec2(0.00078125f, 0.00546875f);
        off[1] = vec2(-0.005078125f, -0.00546875f);
        off[2] = vec2(0.00546875f, -0.005078125f);
    }
    for (i = 0; i < nids; i++) {
        face = &ctx->ground->faces[ids[i]];
        for (j = 0; j < face->n; j++) {
            k = radius * 0.03f;
            p = ctx->ground->verts[face->idx[j]].p;
            n = &ctx->ground->verts[face->idx[j]].n;
            dy = k * n->y;
            dz = k * n->z;
            p.x = k * n->x + p.x;
            p.y = p.y + dy;
            p.z = p.z + dz;
            pi = (*slot)->add_point(&p);
            pt[j] = pi;
            ir = 1.0f / radius;
            ik = ir * 0.5f;
            u = (fr.s.x * (p.x - fr.o.x) + (fr.s.y * (p.y - fr.o.y) + fr.s.z * (p.z - fr.o.z))) * ik;
            v = (fr.f.y * (p.y - fr.o.y) + (fr.f.x * (p.x - fr.o.x) + fr.f.z * (p.z - fr.o.z))) * ik;
            if (!flag) {
                u = u * 1.5f;
                v = v * 1.5f;
            }
            u = u + 0.5f;
            v = v + 0.5f;
            shade = ctx->ground->verts[face->idx[j]].shade;
            c = shade / (s0 + shade);
            if (c < 0.0f) c = 0.0f;
            else if (c > 0.75f) c = 0.75f;
            if (nt == 2) {
                c = 1.0f - (float)sqrt(1.0f - c);
            } else if (nt == 3) {
                c = (3.0f - (float)sqrt(9.0f - (c - 0.01f) * 12.0f)) * 0.125f;
            }
            dx2 = p.x - pos.o.x;
            dy2 = p.y - pos.o.y;
            dz2 = p.z - pos.o.z;
            a = (float)sqrt(dx2 * dx2 + dy2 * dy2 + dz2 * dz2) * ir * 1.3333334f - 3.8f;
            if (a < 0.2f) a = 0.2f;
            else if (a >= 1.0f) a = 1.0f;
            t = c * (1.0f - a) * f294;
            if (flag)
                t = t * f160;
            else
                t = t * f164 * 0.9f;
            al = (int)(t * 256.0f);
            if (al < 0) al = 0;
            else if (al >= 0x100) al = 0xff;
            col = (al << 24) | 0xffffff;
            for (q = 0; q < nt; q++) {
                uv = vec2(u + off[q].x, v + off[q].y);
                f5c = 0;
                vtx[q][j] = (*slot)->add_vertex(pi, &uv, &col, n, &f5c);
            }
        }
        for (q = 0; q < nt; q++) {
            (*slot)->add_face(vtx[q][1], vtx[q][2], vtx[q][0]);
            if (face->n > 3)
                (*slot)->add_face(vtx[q][0], vtx[q][2], vtx[q][3]);
        }
    }
    (*slot)->slots[0].f28 = 1;
    (*slot)->slots[1].f28 = 1;
    (*slot)->color = 0xff000915;
}

