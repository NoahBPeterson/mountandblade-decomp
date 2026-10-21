// Slice s0041ce80: rgl::mesh tangent-frame builder (provisional names).
// For every face the tangent/bitangent are accumulated onto its three vertices from the point positions and
// texture coordinates, then each vertex frame is Gram-Schmidt orthonormalised against the normal, with
// fallback axes for degenerate cases, and the bitangent is flipped to keep the frame's handedness.
#include <vector>
#include <math.h>
#include "s0041ce80.h"

using namespace rgl;

// vector4::normalize, inlined here (rgl.h declares it out of line)
static inline void vnorm(vector4& v)
{
    float l = (float)sqrt((v.x * v.x + v.y * v.y) + v.z * v.z);
    if (l > 1e-10f) {
        float r = 1.0f / l;
        v.x = r * v.x;
        v.y = r * v.y;
        v.z = r * v.z;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}

// the tangent-frame code sums the squares as x*x + z*z + y*y
static inline float len2_xzy(const vector4& v) { return (v.x * v.x + v.z * v.z) + v.y * v.y; }
static inline void vnorm_xzy(vector4& v)
{
    float l = (float)sqrt(len2_xzy(v));
    if (l > 1e-10f) {
        float r = 1.0f / l;
        v.x = r * v.x;
        v.y = r * v.y;
        v.z = r * v.z;
    } else {
        v.x = 0.0f;
        v.y = 1.0f;
        v.z = 0.0f;
    }
}

static inline vector4 vcross(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
static inline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
inline vector4 vmul(const float& s, const vector4& a) { return vector4(s * a.x, s * a.y, s * a.z); }
inline vector4 vmul(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }

// @ 0x41ce80
void mesh::compute_tangents()
{
    ensure_local();
    if (!local_a0) {
        v90 = parent->v90;                 // 0x411630 (vector<face20>::operator=)
        local_a0 = 1;
    }

    for (vertex60* v = v50._Myfirst; v < v50._Mylast; v++) {
        v->tan = vector4(0.0f, 0.0f, 0.0f);
        v->bit = vector4(0.0f, 0.0f, 0.0f);
        vnorm(v->nrm);
    }

    for (face20* f = v90._Myfirst; f < v90._Mylast; f++) {
        vertex60* v1 = &vat(v50, f->b);
        vertex60* v0 = &vat(v50, f->a);
        point20* p0 = &vat(v70, v0->index);
        point20* p1 = &vat(v70, v1->index);
        float e1x = p1->a.x - p0->a.x;
        vertex60* v2 = &vat(v50, f->c);
        point20* p2 = &vat(v70, v2->index);
        float e2x = p2->a.x - p0->a.x;
        float e1y = p1->a.y - p0->a.y;
        float e2y = p2->a.y - p0->a.y;
        float e1z = p1->a.z - p0->a.z;
        float e2z = p2->a.z - p0->a.z;
        float du1 = v1->u - v0->u;
        float du2 = v2->u - v0->u;
        float dv1 = v1->v - v0->v;
        float dv2 = v2->v - v0->v;
        float r = 1.0f / (du1 * dv2 - dv1 * du2);
        float tx = (dv2 * e1x - dv1 * e2x) * r;
        float ty = (e1y * dv2 - dv1 * e2y) * r;
        float tz = (dv2 * e1z - dv1 * e2z) * r;
        float bx = (du1 * e2x - du2 * e1x) * r;
        float by = (du1 * e2y - du2 * e1y) * r;
        float bz = (du1 * e2z - du2 * e1z) * r;
        v0->tan.x = tx + v0->tan.x;
        v0->tan.y = ty + v0->tan.y;
        v0->tan.z = tz + v0->tan.z;
        vertex60* q = &vat(v50, f->b);
        q->tan.x = tx + q->tan.x;
        q->tan.y = ty + q->tan.y;
        q->tan.z = tz + q->tan.z;
        q = &vat(v50, f->c);
        q->tan.x = tx + q->tan.x;
        q->tan.y = ty + q->tan.y;
        q->tan.z = tz + q->tan.z;
        q = &vat(v50, f->a);
        q->bit.x = bx + q->bit.x;
        q->bit.y = by + q->bit.y;
        q->bit.z = bz + q->bit.z;
        q = &vat(v50, f->b);
        q->bit.x = bx + q->bit.x;
        q->bit.y = by + q->bit.y;
        q->bit.z = bz + q->bit.z;
        q = &vat(v50, f->c);
        q->bit.x = bx + q->bit.x;
        q->bit.y = by + q->bit.y;
        q->bit.z = bz + q->bit.z;
    }

    for (vertex60* v = v50._Myfirst; v < v50._Mylast; v++) {
        vector4& n = v->nrm;
        vector4& t = v->tan;
        vector4& b = v->bit;
        // handedness of the accumulated frame
        bool flip = true;
        float h = (t.z * n.y - n.z * t.y) * b.x + (n.z * t.x - t.z * n.x) * b.y + (n.x * t.y - n.y * t.x) * b.z;
        if (!(h < 0.0f))
            flip = false;
        float d = (t.x * n.x + n.y * t.y) + t.z * n.z;
        t = vector4(t.x - d * n.x, t.y - d * n.y, t.z - d * n.z);
        if (len2_xzy(t) < 1e-5f) {
            t = vector4(n.y * 0.7f - n.z * 0.2f, n.z * 0.1f - n.x * 0.7f, n.x * 0.2f - n.y * 0.1f);
            if (len2_xzy(t) < 1e-5f)
                t = vector4(n.y * 0.35f - n.z * 0.17f, n.z * 0.73f - n.x * 0.35f, n.x * 0.17f - n.y * 0.73f);
        }
        vnorm_xzy(t);
        b = vector4(t.z * n.y - n.z * t.y, n.z * t.x - t.z * n.x, n.x * t.y - n.y * t.x);
        if (len2_xzy(b) < 1e-5f) {
            t = vector4(n.y * 0.4f - n.z * 0.8f, n.z * 0.2f - n.x * 0.4f, n.x * 0.8f - n.y * 0.2f);
            if (len2_xzy(t) < 1e-5f)
                t = vector4(n.y * 0.39f - n.z * 0.67f, n.z * 0.76f - n.x * 0.39f, n.x * 0.67f - n.y * 0.76f);
            vnorm_xzy(t);
            b = vector4(t.z * n.y - n.z * t.y, n.z * t.x - t.z * n.x, n.x * t.y - n.y * t.x);
        }
        if (flip)
            b = vector4(-b.x, -b.y, -b.z);
    }
}
