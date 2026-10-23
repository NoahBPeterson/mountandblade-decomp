// dynamic_entity.cpp (0x425370..0x427d80): the rigid-body entity and what its collision pass uses. dynamic_entity
// (constructor, point velocity, impulses, force step, torques, contact resolution, integration, collide), the world's
// per-cell shape and entity queries with their box tests and the group-mask ordering predicate, std::unique and the
// introsort instances behind the query's sort, rigid_base::collide_entity / collide_shape, the std::vector<Contact>
// members and the collision entity's destructor. Migrated from slices s004251c0 (0x425370..0x425920), s004259a0,
// s004260a0, s00426a30, s00427000 and s00427900 (its first four functions; the rest of it is entity_types.cpp's).
// Boundary: see MIGRATION.md.
#include <math.h>
#include "./dynamic_entity.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"

using namespace rgl;

// ---- the ext twins ------------------------------------------------------------------------------------------------
// Callers of these functions saw them as opaque functions (see fx_anim.h / world_grid.cpp): the twin is only declared
// and bound to the real function with /alternatename.
bool box3_b(const vector4* a, const vector4* b, const vector4* c, const vector4* d);       // 0x426690
bool box2_b(const vector4* a, const vector4* b, const vector4* c, const vector4* d);       // 0x426700
void __cdecl ufill_ext(Contact* p, unsigned int n, const Contact& v, std::allocator<Contact>& al);   // 0x427ca0
void pop_heap_0_ext(std::vector<entity*>::iterator first, std::vector<entity*>::iterator last, entity** tag);   // 0x426e90
void sort_range_x(std::vector<entity*>::iterator first, std::vector<entity*>::iterator last, int ideal);          // 0x426a30
#pragma comment(linker, "/alternatename:?pop_heap_0_ext@@YAXViterator@?$vector@PAUentity@rgl@@V?$allocator@PAUentity@rgl@@@std@@@std@@0PAPAUentity@rgl@@@Z=??$_Pop_heap_0@Viterator@?$vector@PAUentity@rgl@@V?$allocator@PAUentity@rgl@@@std@@@std@@PAUentity@rgl@@@std@@YAXViterator@?$vector@PAUentity@rgl@@V?$allocator@PAUentity@rgl@@@std@@@0@0PAPAUentity@rgl@@@Z")
#pragma comment(linker, "/alternatename:?sort_range_x@@YAXViterator@?$vector@PAUentity@rgl@@V?$allocator@PAUentity@rgl@@@std@@@std@@0H@Z=?sort_range@@YAXViterator@?$vector@PAUentity@rgl@@V?$allocator@PAUentity@rgl@@@std@@@std@@0H@Z")
#pragma comment(linker, "/alternatename:?ufill_ext@@YAXPAUContact@rgl@@IABU12@AAV?$allocator@UContact@rgl@@@std@@@Z=??$_Uninitialized_fill_n@PAUContact@rgl@@IU12@V?$allocator@UContact@rgl@@@std@@@std@@YAXPAUContact@rgl@@IABU12@AAV?$allocator@UContact@rgl@@@0@@Z")
#pragma comment(linker, "/alternatename:?_Insert_n@contact_inl@@QAEXPAUContact@rgl@@IABU23@@Z=?_Insert_n@contact_ops@rgl@@QAEXPAUContact@2@IABU32@@Z")
#pragma comment(linker, "/alternatename:?is_dynamic@coll_base@rgl@@UAE_NXZ=?is_dynamic@collision_entity@rgl@@UAE_NXZ")
#pragma comment(linker, "/alternatename:?point_velocity@coll_base@rgl@@UAEPAUvector4@2@PAU32@PBU32@@Z=?point_velocity@collision_entity@rgl@@UAEPAUvector4@2@PAU32@PBU32@@Z")
#pragma comment(linker, "/alternatename:?apply_impulse@coll_base@rgl@@UAEXPBUvector4@2@0@Z=?apply_impulse@collision_entity@rgl@@UAEXPBUvector4@2@0@Z")
#pragma comment(linker, "/alternatename:?impulse_response@coll_base@rgl@@UAEMPBUvector4@2@0@Z=?impulse_response@collision_entity@rgl@@UAEMPBUvector4@2@0@Z")
#pragma comment(linker, "/alternatename:?box3_b@@YA_NPBUvector4@rgl@@000@Z=?box_overlap3@@YAHPBUvector4@rgl@@000@Z")
#pragma comment(linker, "/alternatename:?box2_b@@YA_NPBUvector4@rgl@@000@Z=?box_overlap2@@YAHPBUvector4@rgl@@000@Z")

static __forceinline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static __forceinline vector4 vmul(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }   // inline copy of rgl::operator* (s by reference)
static inline void scale3(vector4* v, float k) { v->x = k * v->x; v->y = k * v->y; v->z = k * v->z; }
static __forceinline void set3(vector4* o, float x, float y, float z) { o->x = x; o->y = y; o->z = z; }

// @ 0x425370
dynamic_entity::dynamic_entity()
{
    mass = 5.0f;
    inertia = 2.0f;
    lin_damp = 0.1f;
    ang_damp = 0.8f;
    grounded = true;
}

// @ 0x425490
string dynamic_entity::type_name() { return string("dynamic_entity"); }

// @ 0x4254b0
bool dynamic_entity::awake() { return asleep == false; }

// @ 0x4254c0
vector4* dynamic_entity::point_velocity(vector4* out, const vector4* p)
{
    vector4 r(p->x - local.o.x, p->y - local.o.y, p->z - local.o.z);
    vector4 c(r.z * angvel.y - r.y * angvel.z, r.x * angvel.z - r.z * angvel.x, r.y * angvel.x - r.x * angvel.y);
    set3(out, c.x + vel.x, c.y + vel.y, c.z + vel.z);
    return out;
}

// @ 0x425550
void dynamic_entity::apply_impulse(const vector4* p, const vector4* j)
{
    asleep = false;
    vector4 r(p->x - local.o.x, p->y - local.o.y, p->z - local.o.z);
    float k = 1.0f / mass;
    vector4 dv(k * j->x, k * j->y, k * j->z);
    vel = vector4(vel.x + dv.x, vel.y + dv.y, vel.z + dv.z);
    vector4 c(r.y * j->z - r.z * j->y, r.z * j->x - r.x * j->z, r.x * j->y - r.y * j->x);
    float k2 = 1.0f / inertia;
    vector4 s(c.x * k2, c.y * k2, c.z * k2);
    angvel.x = s.x + angvel.x;
    angvel.y = s.y + angvel.y;
    angvel.z = s.z + angvel.z;
}

// @ 0x425670
float dynamic_entity::impulse_response(const vector4* p, const vector4* n)
{
    vector4 r(p->x - local.o.x, p->y - local.o.y, p->z - local.o.z);
    float k = 1.0f / inertia;
    const vector4* d = &cross(vscale(k, cross(r, *n)), r);   // address of the call's temporary (MSVC extension)
    return d->x * n->x + (1.0f / mass + (d->y * n->y + d->z * n->z));
}

// @ 0x425730
void dynamic_entity::update(float t)
{
    if (!asleep) {
        apply_forces(t);
        collide();
        integrate(t);
        if (shape) {
            shape->set_frame(&local);
            vac = shape->get_b();
            v9c = shape->get_a();
        }
    }
}

// @ 0x4257e0
void dynamic_entity::apply_forces(float t)
{
    dt = t;
    force = vmul(mass, vector4(0.0f, 0.0f, -9.8f));
    torque = vector4(0.0f, 0.0f, 0.0f);
    scale3(&vel, 1.0f - t * lin_damp);
    scale3(&angvel, 1.0f - t * ang_damp);
}

// @ 0x4258e0
void dynamic_entity::add_torque(const vector4* t)
{
    float* d = &torque.x;           // the original goes through a float pointer (operand order differs otherwise)
    d[0] = d[0] + t->x;
    d[1] = t->y + d[1];
    d[2] = t->z + d[2];
}

// @ 0x425920
void dynamic_entity::add_torque_at(const vector4* p, const vector4* f)
{
    vector4 r(p->x - local.o.x, p->y - local.o.y, p->z - local.o.z);
    vector4 c(r.y * f->z - r.z * f->y, r.z * f->x - r.x * f->z, r.x * f->y - r.y * f->x);
    torque.x = c.x + torque.x;
    torque.y = c.y + torque.y;
    torque.z = c.z + torque.z;
}

// @ 0x4259a0
void dynamic_entity::resolve()
{
    vector4 dirs[200];
    bool flagB = false;
    for (Contact* c = contacts.first; c < contacts.last; c++) {
        const vector4& n = c->n;
        float d1 = -(force.x * n.x + (force.y * n.y + force.z * n.z));
        if (d1 > 0.0f) {
            float d2 = vel.x * n.x + (vel.y * n.y + vel.z * n.z);
            if (d2 > 0.0f) {
                float k = (mass / dt) * d2;
                if (d1 < k)
                    k = d1;
                k = -k;
                vector4 g = vscale(k, n);
                vector4 dv = vscale(dt / mass, g);
                vel.x = dv.x + vel.x;
                vel.y = dv.y + vel.y;
                vel.z = dv.z + vel.z;
                force.x = force.x - g.x;
                force.y = force.y - g.y;
                force.z = force.z - g.z;
            }
        }
    }
    vector4 fn = force;
    int n = 0;
    bool flagA = false;
    vector4 u;
    if (fn.x * fn.x + (fn.y * fn.y + fn.z * fn.z) < 1e-6f)
        u = vector4(1.0f, 0.0f, 0.0f);
    else
        normalize(u, fn);
    {
        Contact* c = contacts.first;
        vector4* dp = dirs;
        for (; c != contacts.last; c++) {
            vector4 r(c->p.x - local.o.x, c->p.y - local.o.y, c->p.z - local.o.z);
            float d = u.y * r.y + (u.z * r.z + r.x * u.x);
            *dp = vector4(r.x - u.x * d, r.y - u.y * d, r.z - u.z * d);
            double len = sqrt(dp->y * dp->y + (dp->x * dp->x + dp->z * dp->z));
            if (len > -0.001 && len < 0.001)
                flagA = true;
            else
                normalize(*dp, *dp);
            n++;
            dp++;
        }
    }
    int j = 0;
    if (!flagA) {
        for (Contact* c = contacts.first; c < contacts.last; c++) {
            bool found = false;
            flagB = false;
            int ihi = -1;
            int ilo = -1;
            float lo = 1000.0f;
            float hi = -1000.0f;
            for (int i = 0; i < n; i++) {
                if (found)
                    break;
                if (i != j) {
                    const vector4& p = dirs[j];
                    const vector4& q = dirs[i];
                    float dotv = q.x * p.x + (p.z * q.z + q.y * p.y);
                    float s = (p.x * q.z - p.z * q.x) * u.y + ((q.x * p.y - q.y * p.x) * u.z + (p.z * q.y - p.y * q.z) * u.x);
                    double ang = atan2(s, dotv);
                    if (s > 0.1) {
                        if (-1 < ihi) {
                            if (ang < lo) {
                                lo = (float)ang;
                                ihi = j;
                            }
                        } else {
                            lo = (float)ang;
                            ihi = j;
                        }
                    } else if (s < -0.1) {
                        if (-1 < ilo) {
                            if (ang > hi) {
                                hi = (float)ang;
                                ilo = j;
                            }
                        } else {
                            hi = (float)ang;
                            ilo = j;
                        }
                    } else if (dotv < 0.0) {
                        found = flagB = true;
                    }
                    if (-1 < ihi && -1 < ilo && (hi + 3.1415927f) + (3.1415927f - lo) < 3.1415927f)
                        found = flagB = true;
                }
            }
            j++;
            if (!flagB && -(fn.x * c->n.x + (fn.y * c->n.y + fn.z * c->n.z)) > 0.0f) {
                float sc = 1.0f / (float)n;
                vector4 g(-fn.x * sc, -fn.y * sc, -fn.z * sc);
                add_torque_at(&c->p, &g);
            }
        }
    }
    for (Contact* c = contacts.first; c < contacts.last; c++) {
        float d = -(force.x * c->n.x + (force.y * c->n.y + force.z * c->n.z));
        if (d > 0.0f) {
            vector4 t(d * c->n.x, d * c->n.y, d * c->n.z);
            force.x = t.x + force.x;
            force.y = t.y + force.y;
            force.z = t.z + force.z;
        }
    }
    grounded = false;
    if (flagA || flagB)
        grounded = true;
    float r = dt / mass;
    vector4 dv(r * force.x, r * force.y, r * force.z);
    vel.x = dv.x + vel.x;
    vel.y = dv.y + vel.y;
    vel.z = dv.z + vel.z;
    float r2 = dt / inertia;
    vector4 dw(r2 * torque.x, r2 * torque.y, r2 * torque.z);
    angvel.x = dw.x + angvel.x;
    angvel.y = dw.y + angvel.y;
    angvel.z = dw.z + angvel.z;
}

// @ 0x4260a0
void dynamic_entity::integrate(float t)
{
    resolve();
    ((contact_ops&)contacts).clear();
    if (vel.length() < 0.2f && angvel.length() < 0.2f && grounded)
        asleep = true;
    matrix m = local;
    vector4 dv = vscale(t, vel);
    m.o.x = dv.x + m.o.x;
    m.o.y = m.o.y + dv.y;
    m.o.z = m.o.z + dv.z;
    vector4 cf, cu;
    cf.x = angvel.y * local.f.z - angvel.z * local.f.y;
    cf.y = angvel.z * local.f.x - angvel.x * local.f.z;
    cf.z = angvel.x * local.f.y - angvel.y * local.f.x;
    cu.x = angvel.y * local.u.z - angvel.z * local.u.y;
    cu.y = angvel.z * local.u.x - angvel.x * local.u.z;
    cu.z = angvel.x * local.u.y - angvel.y * local.u.x;
    vector4 sf = vscale(t, cf);
    vector4 su = vscale(t, cu);
    m.f.x = sf.x + m.f.x; m.f.y = sf.y + m.f.y; m.f.z = sf.z + m.f.z;
    m.u.x = su.x + m.u.x; m.u.y = su.y + m.u.y; m.u.z = su.z + m.u.z;
    normalize(m.f, m.f);
    m.s.x = m.f.y * m.u.z - m.f.z * m.u.y;
    m.s.y = m.f.z * m.u.x - m.f.x * m.u.z;
    m.s.z = m.f.x * m.u.y - m.f.y * m.u.x;
    normalize(m.s, m.s);
    m.u.x = m.s.y * m.f.z - m.s.z * m.f.y;
    m.u.y = m.s.z * m.f.x - m.s.x * m.f.z;
    m.u.z = m.s.x * m.f.y - m.s.y * m.f.x;
    local = (smat&)m;
    local.o.x = local.o.x + shift.x;
    local.o.y = shift.y + local.o.y;
    local.o.z = shift.z + local.o.z;
    shift = vector4(0.0f, 0.0f, 0.0f);
}

// @ 0x4263d0
void dynamic_entity::collide()
{
    grid_shape* found[0x2000];
    volatile int count;                 // volatile: the original re-reads the count after every on_shape call
    int use3d = 1;
    ((world_q*)owner)->query_shapes(this, found, (int*)&count, &use3d, 0x8106);
    for (int i = 0; i < count; i++)
        on_shape(found[i]);
    std::vector<entity*> others;
    ((world_q*)owner)->query_entities(this, 3, true, &others);
    for (std::vector<entity*>::iterator it = others.begin(); it < others.end(); ++it)
        if (((collision_entity*)*it)->is_dynamic())
            on_entity(*it);
}

// ---- the world's queries ------------------------------------------------------------------------------------------
// @ 0x426490
void world_q::query_shapes(entity* e, grid_shape** out, int* count, int* use3d, unsigned mask)
{
    *count = 0;
    for (cellref* r = (cellref*)e->refs; r != 0; r = r->next) {
        cell_q* c = (cell_q*)&rows[r->x][r->y];
        c->collect(e->get_v9c(), e->get_vac(), use3d, mask, out, count);
    }
}

// box tests: 3D (0x426690) and 2D xy (0x426700) overlap of [a, b] with [c, d]. The callers (collect) see them as
// returning a byte (test al, al), the bodies return int: they are called through the bool-returning twins above.
// @ 0x426690
int __declspec(noinline) box_overlap3(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return 1;
    return 0;
}

// the same 3D test inlined (0x426750)
static __forceinline bool box3_inl(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y && a->z <= d->z && c->z <= b->z)
        return true;
    return false;
}

// @ 0x426700
int __declspec(noinline) box_overlap2(const vector4* a, const vector4* b, const vector4* c, const vector4* d)
{
    if (a->x <= d->x && c->x <= b->x && a->y <= d->y && c->y <= b->y)
        return 1;
    return 0;
}

// @ 0x426500
void cell_q::collect(const vector4* bmin, const vector4* bmax, int* use3d, unsigned mask, grid_shape** out, int* count)
{
#define shapes ((std::vector<grid_shape*>&)a)
    grid_shape* buf[0x2000];
    int n = 0;
    struct { unsigned i; int k; } st;               // lives in memory in the original (reloaded every iteration)
    st.i = 0;
    st.k = 0;
    while (shapes.size() > st.i) {
        grid_shape* sh = shapes[st.i];
        if (sh->m8 & mask) {
            st.i++;
            continue;
        }
        vector4 tv[2];                  // both query temporaries live in one block (min below max)
        const vector4* smax = sh->get_max(&tv[1]);
        const vector4* smin = sh->get_min(&tv[0]);
        bool hit;
        if (*use3d == 0)
            hit = box2_b(bmin, bmax, smin, smax);
        else
            hit = box3_b(bmin, bmax, smin, smax);
        if (!hit) {
            st.i++;
            continue;
        }
        if (st.k < *count) {
            grid_shape* o = out[st.k];
            grid_shape* cur = shapes[st.i];
            if (cur >= o) {
                st.k++;
                if (cur == o) {
                    st.i++;
                    buf[n++] = cur;
                } else {
                    buf[n++] = o;
                }
                continue;
            }
        }
        grid_shape* cur2 = shapes[st.i];
        st.i++;
        buf[n++] = cur2;
    }
    if (st.k < *count) {
        int m = *count - st.k;
        memcpy(&buf[n], &out[st.k], m * sizeof(grid_shape*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
#undef shapes
}

// does a type mask a pass the filter b: equal, or the lowest differing bit is clear in b
static __forceinline bool type_ok(unsigned a, unsigned b)
{
    if (a == b)
        return true;
    while (((a ^ b) & 1) == 0) {
        a >>= 1;
        b >>= 1;
    }
    return b == 0;
}

// std::unique on vector<entity*>::iterator (0x4269e0): the original keeps it out of line
namespace std {
// @ 0x4269e0
template<> __declspec(noinline) vector<rgl::entity*>::iterator unique(vector<rgl::entity*>::iterator _First, vector<rgl::entity*>::iterator _Last)
{
    for (vector<rgl::entity*>::iterator _Firstb; (_Firstb = _First) != _Last && ++_First != _Last; )
        if (*_Firstb == *_First) {
            for (; ++_First != _Last; )
                if (!(*_Firstb == *_First))
                    *++_Firstb = *_First;
            return (++_Firstb);
        }
    return (_Last);
}
}

// @ 0x426750
void world_q::query_entities(entity* e, unsigned mask, bool ordered, std::vector<entity*>* out)
{
    out->clear();
    for (cellref* r = (cellref*)e->refs; r != 0; r = r->next) {
        for (cellnode* nd = rows[r->x][r->y].head; nd != 0; nd = nd->next) {
            entity** pe = &nd->obj;
            if (*pe != e) {
                if (type_ok((*pe)->type_id(), mask)) {
                  if (!ordered || less(e, *pe)) {
                    if (box3_inl(e->get_v9c(), e->get_vac(), (*pe)->get_v9c(), (*pe)->get_vac()))
                        out->push_back(*pe);
                  }
                }
            }
        }
    }
    sort_range_x(out->begin(), out->end(), out->end() - out->begin());
    out->erase(std::unique(out->begin(), out->end()), out->end());
}

// @ 0x426960
bool world_q::less(entity* a, entity* b)
{
    int k = 0;
    unsigned ta = a->type_id();
    unsigned tb = b->type_id();
    if (ta == tb)
        return a->id > b->id;
    bool found = false;
    do {
        if (found) break;
        if ((((unsigned char)(tb >> (char)k) ^ (unsigned char)(ta >> (char)k)) & 1) == 0)
            k++;
        else
            found = true;
    } while (k < 32);
    return (ta >> (char)k) & 1;
}

// ---- the introsort instances behind std::sort of the query result -------------------------------------------------
// The VC7.1 <algorithm> instantiations for std::vector<entity*>::iterator (the class std::vector<T>::iterator is what
// puts the address-taken iterator temporaries in _Rotate / _Unguarded_partition). The slices instantiated them for
// std::vector<unsigned> (identical code; the linker folds them). _Adjust_heap at 0x426df0 is the plain-pointer instance.
typedef entity* u32;                                    // the element type (a pointer; the code is the same for any 4-byte element)
typedef std::vector<u32>::iterator It;

// @ 0x426a30
// std::_Sort<It,int> with sort_heap/pop_heap spelled out: in the binary they are inlined into the heap-sort tail (only
// _Pop_heap_0 is called), while cl 13.10 keeps std::sort_heap out of line here. std::_Pop_heap_0 (0x426e90) seen from
// its caller (pop_heap_0_ext) is kept out of line like the original.
void sort_range(It first, It last, int ideal)
{
    int count;
    for (; 32 < (count = last - first) && 0 < ideal; ) {
        std::pair<It, It> mid = std::_Unguarded_partition(first, last);
        ideal /= 2, ideal += ideal / 2;
        if (mid.first - first < last - mid.second)
            sort_range(first, mid.first, ideal), first = mid.second;
        else
            sort_range(mid.second, last, ideal), last = mid.first;
    }
    if (32 < count) {
        std::make_heap(first, last);
        for (; 1 < last - first; --last) {
            if (1 < last - first)
                pop_heap_0_ext(first, last, (u32*)0);
        }
    } else if (1 < count)
        std::_Insertion_sort(first, last);
}
// @ 0x426b20
template std::pair<It, It> std::_Unguarded_partition<It>(It, It);
// @ 0x426c70
template void std::_Median<It>(It, It, It);
// @ 0x426db0
template void std::_Make_heap<It, int, u32>(It, It, int*, u32*);
// @ 0x426df0
template void std::_Adjust_heap<u32*, int, u32>(u32*, int, int, u32);  // the pointer instance is what lives at 0x426df0
// @ 0x426e50
template void std::_Push_heap<It, int, u32>(It, int, int, u32);
// @ 0x426e90
template void std::_Pop_heap_0<It, u32>(It, It, u32*);
// @ 0x426ec0
template void std::_Insertion_sort<It>(It, It);
// @ 0x426f40
template void std::_Rotate<It, int, u32>(It, It, It, int*, u32*);

// ---- rigid_base::collide_entity / collide_shape --------------------------------------------------------------------
static __forceinline vector4 vmulf(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static __forceinline vector4 vsub(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline float vdot(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

// @ 0x427000
bool rigid_base::collide_entity(rigid_base* o)
{
    if (shape == 0)
        return false;
    if (o->shape == 0)
        return false;
    contact_rec c[4];
    int n = shape_contact_count(shape, o->shape, 4, c);
    if (n == 0)
        return false;
    for (int i = 0; i < n; ++i) {
        vector4 nrm = *(vector4*)&c[i].normal;
        vector4 pt = *(vector4*)&c[i].point;
        vector4 tb, ta;
        const vector4* vb = o->point_velocity(&tb, &pt);
        const vector4* va = point_velocity(&ta, &pt);
        vector4 rel(va->x - vb->x, va->y - vb->y, va->z - vb->z);
        Contact rec;
        rec.p = pt;
        rec.n = nrm;
        ((contact_ops&)contacts).push_back(rec);
        vector4 neg(-nrm.x, -nrm.y, -nrm.z);
        Contact rec2;
        rec2.p = pt;
        rec2.n = neg;
        ((contact_ops&)o->contacts).push_back(rec2);
        float vn = vdot(nrm, rel);
        vector4 t = vsub(rel, vmulf(nrm, vn));
        vector4 td;
        normalize(td, t);
        if (vn < 0.0f) {
            double j = -(vdot(nrm, rel) * ((o->restitution + restitution) * 0.5f + 1.0));
            float rb = o->impulse_response(&pt, &nrm);
            double mag = j / (impulse_response(&pt, &nrm) + rb);
            vector4 imp(nrm.x * mag, nrm.y * mag, nrm.z * mag);
            apply_impulse(&pt, &imp);
            vector4 imp2(-imp.x, -imp.y, -imp.z);
            o->apply_impulse(&pt, &imp2);
        }
        double j = vdot(td, rel) * -0.5;
        float rb = o->impulse_response(&pt, &td);
        double mag = j / (impulse_response(&pt, &td) + rb);
        vector4 imp(td.x * mag, td.y * mag, td.z * mag);
        apply_impulse(&pt, &imp);
        vector4 imp2(-imp.x, -imp.y, -imp.z);
        o->apply_impulse(&pt, &imp2);
    }
    return true;
}

// ---- std::vector<Contact> ----------------------------------------------------------------------------------------
namespace std {
// @ 0x427880
template<> Contact* _Uninit_copy<Contact*, Contact*, allocator<Contact> >(Contact* _First, Contact* _Last, Contact* _Dest, allocator<Contact>& _Al, _Nonscalar_ptr_iterator_tag)
{
    Contact* _Next = _Dest;
    try {
        for (; _First != _Last; ++_Dest, ++_First)
            new ((void*)_Dest) Contact(*_First);
    } catch (...) {
        for (; _Next != _Dest; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
    return _Dest;
}
// @ 0x4274c0
template<> void _Uninit_fill_n<Contact*, unsigned int, Contact, allocator<Contact> >(Contact* _First, unsigned int _Count, const Contact& _Val, allocator<Contact>& _Al, _Nonscalar_ptr_iterator_tag)
{
    Contact* _Next = _First;
    try {
        for (; 0 < _Count; --_Count, ++_First)
            new ((void*)_First) Contact(_Val);
    } catch (...) {
        for (; _Next != _First; ++_Next)
            _Al.destroy(_Next);
        throw;
    }
}
}

__forceinline Contact* contact_ops::_Ucopy(Contact* f, Contact* l, Contact* p) { return std::_Uninitialized_copy(f, l, p, *(std::allocator<Contact>*)this); }
__forceinline Contact* contact_ops::_Ufill(Contact* p, size_type n, const Contact& v) { std::_Uninitialized_fill_n(p, n, v, *(std::allocator<Contact>*)this); return p + n; }
// @ 0x427450
void contact_ops::push_back(const Contact& _Val)
{
    if (size() < capacity())
        last = _Ufill(last, 1, _Val);
    else
        insert(end_(), _Val);
}
// @ 0x427540
contact_ops::iterator contact_ops::insert(iterator _Where, const Contact& _Val)
{
    size_type _Off = size() == 0 ? 0 : _Where._Ptr - first;
    _Insert_n(_Where._Ptr, (size_type)1, _Val);
    return iterator(first + _Off);
}
// @ 0x427590
void contact_ops::_Insert_n(Contact* _Where, size_type _Count, const Contact& _Val)
{
    Contact _Tmp = _Val;
    size_type _Capacity = capacity();
    if (_Count == 0)
        ;
    else if (max_size() - size() < _Count)
        _Xlen();
    else if (_Capacity < size() + _Count) {
        _Capacity = max_size() - _Capacity / 2 < _Capacity ? 0 : _Capacity + _Capacity / 2;
        if (_Capacity < size() + _Count)
            _Capacity = size() + _Count;
        std::allocator<Contact>& _Alval = *(std::allocator<Contact>*)this;
        Contact* _Newvec = _Alval.allocate(_Capacity);
        Contact* _Ptr = _Newvec;
        try {
            _Ptr = _Ucopy(first, _Where, _Newvec);
            _Ptr = _Ufill(_Ptr, _Count, _Tmp);
            _Ucopy(_Where, last, _Ptr);
        } catch (...) {
            _Destroy(_Newvec, _Ptr);
            _Alval.deallocate(_Newvec, _Capacity);
            throw;
        }
        _Count += size();
        if (first != 0) {
            _Destroy(first, last);
            _Alval.deallocate(first, end - first);
        }
        end = _Newvec + _Capacity;
        last = _Newvec + _Count;
        first = _Newvec;
    } else if ((size_type)(last - _Where) < _Count) {
        _Ucopy(_Where, last, _Where + _Count);
        try {
            _Ufill(last, _Count - (last - _Where), _Tmp);
        } catch (...) {
            _Destroy(_Where + _Count, last + _Count);
            throw;
        }
        last += _Count;
        {
            Contact* _Fe = last - _Count;
            for (Contact* _Q = _Where; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    } else {
        Contact* _Oldend = last;
        last = _Ucopy(_Oldend - _Count, _Oldend, last);
        {
            Contact* _L = _Oldend - _Count;
            Contact* _D = _Oldend;
            while (_Where != _L)
                *--_D = *--_L;
        }
        {
            Contact* _Q = _Where;
            Contact* _Fe = _Where + _Count;
            for (; _Q != _Fe; ++_Q)
                *_Q = _Tmp;
        }
    }
}

// std::_Uninitialized_fill_n for Contact, called out of line from push_back's fast path (the callers see the twin
// ufill_ext: the instantiation would be inlined into them)
// @ 0x427ca0
template void std::_Uninitialized_fill_n<Contact*, unsigned int, Contact, std::allocator<Contact> >(Contact*, unsigned int, const Contact&, std::allocator<Contact>&);

// The std::vector<Contact> members as collide_shape has them: push_back and insert expanded inline, the fill through the
// out-of-line 0x427ca0 and the insertion through 0x427590 (twins, bound below)
struct contact_inl : contact_vec {
    typedef unsigned size_type;
    struct iterator {
        Contact* _Ptr;
        iterator() {}
        iterator(Contact* p) : _Ptr(p) {}
        iterator operator+(int off) const;                          // 0x40f880
    };
    size_type capacity() const { return first == 0 ? 0 : end - first; }
    size_type size() const { return first == 0 ? 0 : last - first; }
    iterator begin() { return iterator(first); }
    iterator end_() { return iterator(last); }
    Contact* _Ufill(Contact* p, size_type n, const Contact& v) { ufill_ext(p, n, v, *(std::allocator<Contact>*)this); return p + n; }
    void _Insert_n(Contact* _Where, size_type _Count, const Contact& _Val);            // 0x427590
    __forceinline iterator insert(iterator _Where, const Contact& _Val)
    {
        size_type _Off = size() == 0 ? 0 : _Where._Ptr - first;
        _Insert_n(_Where._Ptr, (size_type)1, _Val);
        return begin() + _Off;
    }
    __forceinline void push_back(const Contact& _Val)
    {
        if (size() < capacity())
            last = _Ufill(last, 1, _Val);
        else
            insert(end_(), _Val);
    }
};

// @ 0x427900
bool rigid_base::collide_shape(coll_shape* s)
{
    if (shape == 0)
        return false;
    contact_rec c[4];
    int n = shape_contact_count(shape, s, 4, c);
    if (n == 0)
        return false;
    for (int i = 0; i < n; ++i) {
        vector4 nrm = *(vector4*)&c[i].normal;
        vector4 pt = *(vector4*)&c[i].point;
        Contact rec;
        vector4 v;
        point_velocity(&v, &pt);
        rec.p = pt;
        rec.n = nrm;
        ((contact_inl&)contacts).push_back(rec);
        float vn = nrm.x * v.x + nrm.y * v.y + nrm.z * v.z;
        vector4 t = vsub(v, vmulf(nrm, vn));
        if (0.0f > vn) {
            double j = -((restitution + 1.0) * vn);
            double mag = j / impulse_response(&pt, &nrm);
            vector4 imp(nrm.x * mag, nrm.y * mag, nrm.z * mag);
            apply_impulse(&pt, &imp);
        }
        if (vdot(t, t) > 0.01f) {
            vector4 td;
            normalize(td, t);
            double j = vdot(td, v) * -0.5;
            double mag = j / impulse_response(&pt, &td);
            vector4 imp(td.x * mag, td.y * mag, td.z * mag);
            apply_impulse(&pt, &imp);
        }
        if (c[i].depth > 0.01f) {
            float d = c[i].depth * 0.5f;
            vector4 corr = vmulf(*(vector4*)&c[i].normal, d);
            shift.x += corr.x;
            shift.y += corr.y;
            shift.z += corr.z;
        }
    }
    return true;
}
