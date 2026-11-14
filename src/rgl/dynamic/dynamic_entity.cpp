// dynamic_entity.cpp (0x425370..0x426490): the rigid-body entity. dynamic_entity
// (constructor, point velocity, impulses, force step, torques, contact resolution, integration, collide); the world's
// queries, the sort instances, on_entity / on_shape and the Contact vector members from 0x426490 on are in
// world_collide.cpp (collide 0x4263d0 does not see world::query_shapes' body: the unit ends before it).
// Migrated from slices s004251c0 (0x425370..0x425920), s004259a0, s004260a0 (and, for world_collide.cpp,
// s00426a30, s00427000 and s00427900: its first four functions; the rest of it is entity_types.cpp's).
// Boundary: see MIGRATION.md.
#include <math.h>
#include "./dynamic_entity.h"
// the shared class headers again, directly (the resolution tools read the address annotations of the headers a source includes itself)
#include "../../include/frustum.h"
#include "../../include/keyframes.h"
#include "../../include/mat4.h"
#include "../../include/mesh_prims.h"
#include "../../include/vec.h"
#include "../../include/mesh_parts.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../mesh.h"      // (listed directly: the resolution audit scans only the source's own includes)

// std::vector<point20>::iterator::operator+ stays out of line (0x40f880, a COMDAT in mesh_obj.cpp), like the original's callers here
namespace std {
template<> vector<rgl::point20, allocator<rgl::point20> >::iterator vector<rgl::point20, allocator<rgl::point20> >::iterator::operator+(int) const;
}
using namespace rgl;


static inline void scale3(vector4& v, float k) { v.x = k * v.x; v.y = k * v.y; v.z = k * v.z; }
static __forceinline float dot_r(const vector4& a, const vector4& b) { return a.x * b.x + (a.y * b.y + a.z * b.z); }
static __forceinline float dot_yz(const vector4& a, const vector4& b) { return a.y * b.y + a.z * b.z; }
static __forceinline void set3(vector4* o, float x, float y, float z) { o->x = x; o->y = y; o->z = z; }

// @ 0x425370
dynamic_entity::~dynamic_entity() {}

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
    const vector4* d = &cross(k * cross(r, *n), r);   // address of the call's temporary (MSVC extension)
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
            shape->transform(&local);
            vector4 t;
            vac = *shape->get_max(&t);
            v9c = *shape->get_min(&t);
        }
    }
}

// @ 0x4257e0
void dynamic_entity::apply_forces(float t)
{
    dt = t;
    force = mass * vector4(0.0f, 0.0f, -9.8f);
    torque = vector4(0.0f, 0.0f, 0.0f);
    scale3(vel, (1.0f - t * lin_damp));          // the parentheses are load-bearing (OPPAREN tuple: the first fld st(0) then
    scale3(angvel, (1.0f - t * ang_damp));       // schedules after the torque copy, as in the original)
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
        float d1 = -(n.x * force.x + dot_yz(n, force));
        if (d1 > 0.0f) {
            float d2 = dot_r(n, vel);
            if (d2 > 0.0f) {
                float k = (mass / dt) * d2;
                if (d1 < k)
                    k = d1;
                k = -k;
                vector4 g = k * n;
                vector4 dv = (dt / mass) * g;
                vel.x = dv.x + vel.x;
                vel.y = dv.y + vel.y;
                vel.z = dv.z + vel.z;
                force.x = force.x - g.x;
                force.y = force.y - g.y;
                force.z = force.z - g.z;
            }
        }
    }
    int n = 0;
    vector4 fn = force;
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
                            ihi = j;
                            lo = (float)ang;
                        }
                    } else if (s < -0.1) {
                        if (-1 < ilo) {
                            if (ang > hi) {
                                ilo = j;
                                hi = (float)ang;
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
    VEC_CLEAR(&contacts);
    if (vel.length() < 0.2f && angvel.length() < 0.2f && grounded)
        asleep = true;
    matrix m = local;
    vector4 dv = t * vel;
    m.o.x = m.o.x + dv.x;
    m.o.y = m.o.y + dv.y;
    m.o.z = dv.z + m.o.z;
    vector4 cf, cu;
    cf.x = angvel.y * local.f.z - angvel.z * local.f.y;
    cf.y = angvel.z * local.f.x - angvel.x * local.f.z;
    cf.z = angvel.x * local.f.y - angvel.y * local.f.x;
    cu.x = angvel.y * local.u.z - angvel.z * local.u.y;
    cu.y = angvel.z * local.u.x - angvel.x * local.u.z;
    cu.z = angvel.x * local.u.y - angvel.y * local.u.x;
    vector4 sf = t * cf;
    m.f.x = m.f.x + sf.x; m.f.y = sf.y + m.f.y; m.f.z = sf.z + m.f.z;
    vector4 su = t * cu;
    m.u.x = m.u.x + su.x; m.u.y = su.y + m.u.y; m.u.z = su.z + m.u.z;
    normalize(m.f, m.f);
    m.s.x = m.f.y * m.u.z - m.f.z * m.u.y;
    m.s.y = m.f.z * m.u.x - m.f.x * m.u.z;
    m.s.z = m.f.x * m.u.y - m.f.y * m.u.x;
    normalize(m.s, m.s);
    m.u.x = m.s.y * m.f.z - m.s.z * m.f.y;
    m.u.y = m.s.z * m.f.x - m.s.x * m.f.z;
    m.u.z = m.s.x * m.f.y - m.s.y * m.f.x;
    local = (smat&)m;
    vector4& sh = shift;
    local.o.x = local.o.x + sh.x;
    local.o.y = sh.y + local.o.y;
    local.o.z = sh.z + local.o.z;
    shift = vector4(0.0f, 0.0f, 0.0f);
}

// @ 0x4263d0
void dynamic_entity::collide()
{
    shape_base* found[0x2000];
    int count;
    int use3d = 1;
    owner->query_shapes(this, found, &count, &use3d, 0x8106);
    for (int i = 0; i < count; i++)
        on_shape((shape_base*)found[i]);
    std::vector<entity*> others;
    owner->query_entities(this, 3, true, &others);
    for (std::vector<entity*>::iterator it = others.begin(); it < others.end(); ++it)
        if (((collision_entity*)*it)->is_dynamic())
            on_entity(*it);
}

