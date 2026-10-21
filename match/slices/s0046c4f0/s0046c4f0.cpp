// Slice s0046c4f0: filling Direct3D vertex / index buffer data from a mesh (0x46c4f0 / 0x46c920) and
// applying a transform to the child meshes of an lod_mesh (0x46c990 / 0x46c9f0). ALL NAMES PROVISIONAL.
#include <windows.h>
#include "rgl.h"
#include "mesh.h"

using namespace rgl;

// @ 0x46c4f0
#define VS(f) (*(int*)&vs[i].f)
#define PT(f) (*(int*)&ps[vs[i].index].f)
void mesh::fill_vertices(int* p, const unsigned* fmt)
{
    bool b18 = (res->mat->f6c & 0x1f) == 0x18;
    std::vector<vertex60>& vs = get50();
    std::vector<point20>& ps = get70();
    int *p7 = p + 7, *p6 = p + 6, *p5 = p + 5, *p4 = p + 4, *p3 = p + 3, *p2 = p + 2;
    for (unsigned i = 0; i < vs.size(); i++) {
        if (*fmt & 0x2000000) {
            p[0] = PT(b.x);
            p[1] = PT(b.y);
            *p2 = PT(b.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        } else if (*fmt & 0x1000000) {
            p[0] = PT(a.x);
            p[1] = PT(a.y);
            *p2 = PT(a.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        }
        if (*fmt & 1) {
            p[0] = VS(normal.x);
            p[1] = VS(normal.y);
            *p2 = VS(normal.z);
            p += 3; p2 += 3; p3 += 3; p4 += 3; p5 += 3; p6 += 3; p7 += 3;
        }
        if (*fmt & 2) {
            p[0] = b18 ? VS(f58) : VS(color);
            p += 1; p2 += 1; p3 += 1; p4 += 1; p5 += 1; p6 += 1; p7 += 1;
        }
        if (*fmt & 0x40) {
            p[0] = VS(f5c);
            p += 1; p2 += 1; p3 += 1; p4 += 1; p5 += 1; p6 += 1; p7 += 1;
        }
        if (*fmt & 0x10) {
            p[0] = VS(uv.x);
            *(float*)&p[1] = 1.0f - vs[i].uv.y;
            p += 2; p2 += 2; p3 += 2; p4 += 2; p5 += 2; p6 += 2; p7 += 2;
        }
        if (*fmt & 0x20) {
            p[0] = VS(f48);
            *(float*)&p[1] = 1.0f - *(float*)&vs[i].f4c;
            p += 2; p2 += 2; p3 += 2; p4 += 2; p5 += 2; p6 += 2; p7 += 2;
        }
        if (*fmt & 0x100) {
            p[0] = VS(f10.x);
            p[1] = VS(f10.y);
            *p2 = VS(f10.z);
            *p3 = VS(f20.x);
            *p4 = VS(f20.y);
            *p5 = VS(f20.z);
            p += 6; p2 += 6; p3 += 6; p4 += 6; p5 += 6; p6 += 6; p7 += 6;
        }
        if (*fmt & 0x1000) {
            std::vector<elem30>& es = get60();
            p[0] = *(int*)&es[i].w[0];
            p[1] = *(int*)&es[i].w[1];
            *p2 = *(int*)&es[i].w[2];
            *p3 = *(int*)&es[i].w[3];
            *p4 = *(int*)&es[i].bonef[0];
            *p5 = *(int*)&es[i].bonef[1];
            *p6 = *(int*)&es[i].bonef[2];
            *p7 = *(int*)&es[i].bonef[3];
            p += 8; p2 += 8; p3 += 8; p4 += 8; p5 += 8; p6 += 8; p7 += 8;
        }
    }
}

// @ 0x46c920
void mesh::fill_indices(unsigned short* out)
{
    std::vector<face20>& fs = local_a0 ? v90 : parent->get90_();
    for (unsigned i = 0; i < fs.size(); i++) {
        *out++ = (unsigned short)(&fs[i].b)[-1];
        *out++ = (unsigned short)(&fs[i].b)[0];
        *out++ = (unsigned short)(&fs[i].b)[1];
    }
}

// @ 0x46c990
void mesh_list::apply(mat4 m)
{
    mesh** it = v._Myfirst;
    if (it < v._Mylast) {
        mat4 t = m;
        do {
            mesh* x = *it;
            x->m_cc = t * x->m_cc;
            ++it;
        } while (it < v._Mylast);
    }
}

// @ 0x46c9f0
void lod_mesh::apply(const mat4* m)
{
    for (int i = 0; i < count; i++)
        lists[i].apply(*m);
}
