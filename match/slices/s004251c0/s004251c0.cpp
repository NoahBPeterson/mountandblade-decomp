// Slice s004251c0: dynamic_entity (vtable 0x54b668): a scene entity with rigid-body state (mass, inertia, linear and
// angular velocity, force/torque accumulators) -- constructor, point velocity, impulse application, force step and the
// frame-source commit; plus a recursive hierarchy walk and the 0x88-byte std::_Uninit_fill_n instantiation.
// 0x4251c0/0x42522d are in s004251c0_eh.cpp (they need /EHa).
#include "dyn.h"

using namespace rgl;

// 0x41f4b0 contact_list::clear (std::vector _Tidy), 0x41fba0 entity::entity, 0x42bbe0 string::string,
// 0x407dc0 cross, 0x407e00 normalize (rgl.h), 0x41ed00 world::locate, 0x423f10 cell::add

// @ 0x425240
void world::add_tree(shape* s)
{
    if (s->type() == 3) {
        composite* c = (composite*)s;
        for (int i = 0; i < c->count; i++) {
            if (c->bodies[i]->type() == 3)
                add_tree(c->bodies[i]);
            else
                add_shape(c->bodies[i]);
        }
    } else if (s->type() != 4) {
        add_shape(s);
    }
}

// @ 0x4252c0
void world::add_shape(shape* s)
{
    int lo[2], hi[2];
    locate(s->get_min(&vector4()), lo);
    locate(s->get_max(&vector4()), hi);
    for (int i = lo[0]; i <= hi[0]; i++)
        for (int j = lo[1]; j <= hi[1]; j++)
            cells[i][j].add(s);
    s->cells[0] = lo[0];
    s->cells[1] = lo[1];
    s->cells[2] = hi[0];
    s->cells[3] = hi[1];
}

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
void dynamic_entity::point_velocity(vector4* out, const vector4* p)
{
    vector4 r(p->x - local.o.x, p->y - local.o.y, p->z - local.o.z);
    vector4 c(r.z * angvel.y - r.y * angvel.z, r.x * angvel.z - r.z * angvel.x, r.y * angvel.x - r.x * angvel.y);
    set3(out, c.x + vel.x, c.y + vel.y, c.z + vel.z);
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
void dynamic_entity::commit(float t)
{
    if (!asleep) {
        apply_forces(t);
        collide();
        integrate(t);
        if (parent) {
            parent->apply(&local);
            vac = parent->get_b();
            v9c = parent->get_a();
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
