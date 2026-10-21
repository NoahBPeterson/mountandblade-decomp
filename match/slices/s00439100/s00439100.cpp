// Slice s00439100: shape cloning and the navigation-mesh queries of the level (names provisional): copy a body's
// collision shape by type, point-in-polygon / distance-to-polygon tests on a nav face, the visit-stamp rollover,
// a line walk across neighbouring faces, the sorted merge of a cell's shape list into a result array, the point
// query of a cell, the segment query of the level grid, and the recursive nearest-face search.
#include <string.h>
#include <math.h>
#include "s00439100.h"

// ---- 0x439100: clone a body's shape (dispatch on shape_type) ------------------------------------------
struct body_res {
    char pad0[0x18];
    shape_base* shape;                                  // 0x18
    shape_base* copy_shape();                           // 0x439100
};

// @ 0x439100
shape_base* body_res::copy_shape()
{
    shape_base* r;                                      // (uninitialised when the type is unknown)
    if (shape->shape_type() == 0) {
        r = new sphere_shape(*(sphere_shape*)shape);
    } else if (shape->shape_type() == 1) {
        r = new capsule(*(capsule*)shape);
    } else if (shape->shape_type() == 2) {
        r = new face_shape(*(face_shape*)shape);
    } else if (shape->shape_type() == 3) {
        r = new composite(*(composite*)shape);
    } else if (shape->shape_type() == 4) {
        r = (shape_base*)((manifold*)shape)->build_composite();
    }
    return r;
}

// ---- nav-face tests -------------------------------------------------------------------------------
// @ 0x4391d0
bool NavMesh::inside(const int* face, const float* p)
{
    NavFace* f = &faces[*face];
    int n = f->nverts;
    bool in = true;
    int i = 0;
    const int* vi = f->vert_p();
    do {
        if (i >= n) return in;
        const NavVert* a = &verts[*vi];
        i++;
        const NavVert* b = &verts[f->vert_p()[i % n]];
        float e = b->x - a->x, f_ = b->y - a->y, g = p[0] - a->x, h = p[1] - a->y;
        if (h * e - g * f_ < 0.0f)
            in = false;
        vi++;
    } while (in);
    return in;
}

// @ 0x439280
float NavMesh::dist(const int* face, const float* p)
{
    float out = 0.0f;
    NavFace* f = &faces[*face];
    int n = f->nverts;
    int i = 0;
    bool in = true;
    if (0 < n) {
        const int* vi = f->vert_p();
        do {
            const NavVert* a = &verts[*vi];
            i++;
            const NavVert* b = &verts[f->vert_p()[i % n]];
            float dx = b->x - a->x;
            float dy = b->y - a->y;
            float len = (float)sqrt(dx * dx + dy * dy);
            float ux, uy;
            if (len > 0.0f) {
                float inv = 1.0f / len;
                ux = dx * inv;
                uy = inv * dy;
            } else {
                ux = 0.0f;
                uy = 1.0f;
            }
            float nx = uy, ny = -ux;
            float d = (p[0] - a->x) * nx + (p[1] - a->y) * ny;
            if (!(d < 0.0f)) {
                in = false;
                if (out < d) out = d;
            }
            vi++;
        } while (i < n);
        if (!in) goto have_out;
    }
    out = 0.0f;
have_out:
    const NavVert* a0 = &verts[f->vert_p()[0]];
    float dz = p[2] - (a0->z - ((p[0] - a0->x) * f->nx + (p[1] - a0->y) * f->ny) / f->nz);
    if (-1.0f < dz && dz < 0.1f)
        dz = 0.0f;
    return (float)fabs(dz) + out;
}

// ---- visit stamp ----------------------------------------------------------------------------------
// @ 0x439410
void NavMesh::next_stamp()
{
    if (stamp > 1000000000) {
        for (unsigned i = 0; i < faces.size(); ++i)
            faces[i].stamp = 0;
        stamp = 0;
    }
    stamp += 3;
}

// ---- line walk across faces (0x439460) --------------------------------------------------------------
// @ 0x439460
bool NavMesh::trace(int face, float x, float y, float z, int unused, int end_face, const float* to, int* max_steps)
{
    struct { int use_x; int found; int prev; int cur; int steps; } st;
    bool done;
    st.cur = -1;
    if (face >= 0 && end_face >= 0) {
        float dx = to[0] - x;
        float dy = to[1] - y;
        float dz = to[2] - z;
        if (dx * dx + (dy * dy + dz * dz) < 0.0001f)
            return true;
        st.use_x = 1;
        if (-1e-06f < dx && dx < 1e-06f)
            st.use_x = 0;
        st.steps = 0;
        st.prev = -1;
        done = false;
        st.cur = face;
        while (st.cur != end_face && st.cur >= 0 && !done && st.steps < *max_steps) {
            ++st.steps;
            NavFace* f = &faces[st.cur];
            st.found = -1;
            for (int i = 0; i < faces[st.cur].nverts; ++i) {
                if (f->edge_p()[i] != st.prev) {
                    const NavVert* a = &verts[f->vert_p()[i]];
                    const NavVert* b = &verts[f->vert_p()[(i + 1) % f->nverts]];
                    float ex = b->x - a->x;
                    float ey = b->y - a->y;
                    float t = ((x - a->x) * dy - (y - a->y) * dx) / (ex * dy - ey * dx);
                    float s;
                    if (st.use_x)
                        s = ((ex * t + a->x) - x) / dx;
                    else
                        s = ((ey * t + a->y) - y) / dy;
                    if (0.1f <= t && t <= 0.9f && 0.0f <= s && s <= 1.0f)
                        st.found = i;
                }
                if (st.found != -1) break;
            }
            if (st.found >= 0) {
                st.prev = f->edge_p()[st.found];
                int nb = f->nbr_p()[st.found];
                done = false;
                if (nb < 0) {
                    done = true;
                } else
                    st.cur = nb;
            } else
                done = true;
        }
    }
    return st.cur == end_face;
}

// ---- cell queries ---------------------------------------------------------------------------------
// merge this cell's shapes (those whose group mask does not intersect `mask`) into the sorted array out[0..*count)
// @ 0x4396b0
void cell::collect_masked(unsigned mask, shape_base** out, int* count)
{
    shape_base* buf[0x2000];
    int n = 0;
    int k = 0;
    unsigned i = 0;
    while (i < shapes.size()) {
        shape_base* sh = shapes[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        if (k < *count) {
            shape_base* o = out[k];
            if (sh >= o) {
                if (sh == o) {
                    i++;
                    buf[n++] = sh;
                } else {
                    buf[n++] = o;
                }
                k++;
                continue;
            }
        }
        i++;
        buf[n++] = sh;
    }
    if (k < *count) {
        int m = *count - k;
        memcpy(&buf[n], &out[k], m * sizeof(shape_base*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}

// shapes of this cell containing the point p (xy box, or the 3-D box via in_box when *use3d != 0)
// @ 0x439780
void cell::collect_point(const float* p, int* use3d, unsigned mask, shape_base** out, int* count)
{
    unsigned i = 0;
    *count = 0;
    while (i < shapes.size()) {
        shape_base* sh = shapes[i];
        if (sh->m8 & mask) {
            i++;
            continue;
        }
        vector4 tv[2];
        const vector4* smax = sh->get_max(&tv[0]);
        const vector4* smin = sh->get_min(&tv[1]);
        bool hit;
        if (*use3d == 0)
            hit = smin->x <= p[0] && p[0] <= smax->x && smin->y <= p[1] && p[1] <= smax->y;
        else
            hit = in_box(smin, smax, (const vector4*)p);
        if (hit) {
            out[*count] = shapes[i];
            i++;
            (*count)++;
        } else
            i++;
    }
}

// @ 0x439870
void level::query_segment(const float* a, const float* b, unsigned mask, shape_base** out, int* count)
{
    int pairs[0x2000][2];
    int n;
    walk(a, b, pairs, &n);
    *count = 0;
    for (int i = 0; i < n; i++)
        cells[pairs[i][0]][pairs[i][1]].collect_masked(mask, out, count);
}

// recursive walk over the neighbours of nav face *face, keeping the nearest face whose polygon contains p
// @ 0x439900
void level::nav_search(const float* p, const int* face, const int* max_depth, int depth, int* best, float* best_d)
{
    if (nav != 0) {
        for (int i = 0; i < nav->faces[*face].nverts; i++) {
            int nb = nav->faces[*face].nbr_p()[i];
            if (nb >= 0 && nav->faces[nb].stamp < nav->stamp) {
                nav->faces[nb].stamp = nav->stamp;
                if (nav->inside(&nb, p)) {
                    const NavVert* v = &nav->verts[nav->faces[nb].vert_p()[0]];
                    NavFace* f = &nav->faces[nb];
                    float dz = (float)fabs(p[2] - (v->z - ((p[0] - v->x) * f->nx + (p[1] - v->y) * f->ny) / f->nz));
                    if (dz < 2.0f && dz < *best_d) {
                        *best = nb;
                        *best_d = dz;
                    }
                }
                if (depth < *max_depth)
                    nav_search(p, &nb, max_depth, depth + 1, best, best_d);
            }
        }
    }
}
