// Slice s0041e470: view-frustum planes and a 2D cell grid (names provisional).
#include <math.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

static inline vector4 sub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }

// A plane given by a point and a normal (0x20 bytes).
struct plane {
    vector4 pos;
    vector4 normal;
    int in_front(const vector4* p, float eps) const;     // 0x41e470
};

// @ 0x41e470
int plane::in_front(const vector4* p, float eps) const
{
    vector4 d;
    d = sub(*p, pos);
    float dist = d.x * normal.x + (d.z * normal.z + d.y * normal.y);
    if (dist < -eps) return 0;
    return 1;
}

// 2D grid of ceil'd cell arrays (provisional): rows of cells indexed by two clamped coordinates.
struct cell_grid {
    char pad0[8];
    int n[2];            // +8
    float lo[2];         // +0x10
    float pad18[2];
    float step[2];       // +0x20
    void locate(const float* p, int* out) const;          // 0x41ed00
};

// @ 0x41ed00
void cell_grid::locate(const float* p, int* out) const
{
    for (int i = 0; i < 2; i++) {
        float f = (p[i] - lo[i]) / step[i];
        if (f <= 0.0f)
            out[i] = 0;
        else if ((float)(n[i] - 1) <= f)
            out[i] = n[i] - 1;
        else
            out[i] = (int)f + 1;
    }
}

struct cell_rows {
    float** rows;        // +0
    int pad[4];
    int last;            // +0x14
    ~cell_rows();        // 0x41ecc0
};

// @ 0x41ecc0
cell_rows::~cell_rows()
{
    if (rows) {
        for (int i = 0; i <= last; i++)
            delete[] rows[i];
        delete[] rows;
    }
}

struct buf3 {
    char pad[0x60];
    float* a;
    float* b;
    float* c;
    char pad2[0xd8 - 0x6c];
    ~buf3();                                              // 0x41ed70 (vector deleting)
};
buf3::~buf3() { delete[] a; delete[] b; delete[] c; }
buf3* make_buf3_array(int n) { return new buf3[n]; }

// ---- view frustum (provisional names) ------------------------------------------------------------
using rgl::matrix;

static inline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static inline vector4 smul(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }
static inline vector4 vadd(const vector4& a, const vector4& b) { return vector4(a.x + b.x, a.y + b.y, a.z + b.z); }
static inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static inline vector4 vneg(const vector4& a) { return vector4(-a.x, -a.y, -a.z); }


struct frustum {
    char perspective;          // +0
    char pad1[3];
    int nplanes;               // +4
    plane planes[6];           // +8: top, bottom, right, left (sides), far, near
    matrix m;                  // +0xc8 (s = right, f = up, u = backwards)
    float left, right, top, bottom, nearz, farz;   // +0x108
    void build_planes();       // 0x41e4f0
};

// @ 0x41e4f0
void frustum::build_planes()
{
    nplanes = 6;
    if (perspective) {
        planes[0].pos = m.o;
        planes[1].pos = m.o;
        planes[2].pos = m.o;
        planes[3].pos = m.o;
        planes[0].normal = vsub(smul(nearz, vneg(m.f)), smul(top, m.u));
        planes[1].normal = vadd(smul(nearz, m.f), smul(bottom, m.u));
        planes[2].normal = vsub(vmul(vneg(m.s), nearz), vmul(m.u, right));
        planes[3].normal = vadd(vmul(m.s, nearz), smul(left, m.u));
        planes[0].normal.normalize();
        planes[1].normal.normalize();
        planes[2].normal.normalize();
        planes[3].normal.normalize();
        planes[4].pos = vsub(m.o, vmul(m.u, farz));
        planes[5].pos = vsub(m.o, smul(nearz, m.u));
        planes[4].normal = m.u;
        planes[5].normal = vneg(m.u);
    } else {
        planes[0].pos = vadd(smul(top, m.f), m.o);
        planes[1].pos = vadd(smul(bottom, m.f), m.o);
        planes[2].pos = vadd(smul(right, m.s), m.o);
        planes[3].pos = vadd(smul(left, m.s), m.o);
        planes[4].pos = vsub(m.o, vmul(m.u, farz));
        planes[5].pos = vsub(m.o, vmul(m.u, nearz));
        planes[0].normal = vneg(m.f);
        planes[1].normal = m.f;
        planes[2].normal = vneg(m.s);
        planes[3].normal = m.s;
        planes[4].normal = m.u;
        planes[5].normal = vneg(m.u);
    }
}
