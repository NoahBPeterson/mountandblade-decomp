// Slice s0042aaf0: the lighting sampler of the scene (0x42ad60) and the shadow-camera frame builder of the
// strategic_entity (0x42aaf0). Names provisional.
#include <math.h>
#include "strat.h"

static __forceinline vector4 vsubv(const vector4& a, const vector4& b) { return vector4(a.x - b.x, a.y - b.y, a.z - b.z); }
static __forceinline vector4 vscl(const float& s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }
static inline vector4 vmulr(const vector4& a, const float& s) { return vector4(a.x * s, a.y * s, a.z * s); }
static inline float nrm(vector4& v)
{
    float len = v.length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        v.x = v.x * inv; v.y = v.y * inv; v.z = v.z * inv;
    } else {
        v.x = 0.0f; v.y = 1.0f; v.z = 0.0f;
    }
    return len;
}

// @ 0x42ad60
void Ctx::sample_lights(const vector4* p, vector4* dir, vector4* amb)
{
    *dir = vector4(0.0f, 0.0f, 0.0f);
    *amb = vector4(0.0f, 0.0f, 0.0f);
    for (unsigned i = 0; i < lights.size(); i++) {
        Light* l = lights[i];
        vector4 d, c;
        if (l->type == 2 && !(l->flags & 1)) {
            d = l->dir;
            c = l->color;
        } else {
            d = vector4(p->x - l->pos.x, p->y - l->pos.y, p->z - l->pos.z);
            float len = nrm(d);
            float k = 1.0f / (len * len);
            c = vector4(k * l->color.x, k * l->color.y, k * l->color.z);
        }
        float s = c.x + c.y + c.z;
        vector4 w = vscl(s, vscl(0.33f, d));
        dir->x = w.x + dir->x;
        dir->y = w.y + dir->y;
        dir->z = w.z + dir->z;
        amb->x = c.x + amb->x;
        amb->y = c.y + amb->y;
        amb->z = c.z + amb->z;
    }
    float len = dir->length();
    if (len > 1e-10f) {
        float inv = 1.0f / len;
        dir->x = inv * dir->x;
        dir->y = inv * dir->y;
        dir->z = inv * dir->z;
    } else {
        dir->x = 0.0f; dir->y = 1.0f; dir->z = 0.0f;
    }
    if (amb->x > 0.95f) amb->x = 0.95f;
    if (amb->y > 0.95f) amb->y = 0.95f;
    if (amb->z > 0.95f) amb->z = 0.95f;
}

static inline vector4 crossv(const vector4& a, const vector4& b)
{
    return vector4(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// @ 0x42aaf0
void StratEnt::build_shadow_frame(matrix* m, vector4* amb)
{
    vector4* o = &m->o;
    *o = pos.o;
    *o = tp(pos, off);
    vector4 d;
    ctx->sample_lights(&pos.o, &d, amb);
    if (d.z > -0.1f) d.z = -0.1f;
    vector4* u = &m->u;
    vector4* f = &m->f;
    *u = vector4(-d.x, -d.y, -d.z);
    *f = vector4(0.0f, 0.0f, 1.0f);
    m->s = crossv(*f, *u);
    m->s.normalize();
    *f = crossv(*u, m->s);
    f->normalize();
    vector4 t;
    t.x = radius * u->x;
    t.y = u->y * radius;
    t.z = u->z * radius;
    o->x = t.x + o->x;
    o->y = t.y + o->y;
    o->z = t.z + o->z;
}
