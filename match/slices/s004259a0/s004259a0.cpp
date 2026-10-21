// Slice s004259a0: dynamic_entity::resolve (vtable 0x54b668 slot 31): contact resolution. It removes the force component
// pressing into each contact, decides whether the contacts surround the body (angular test around the net force) and, if
// not, pushes the contact points with torques; then sets the resting flag and integrates force and torque into the velocities.
#include <math.h>
#include "dyn.h"

using namespace rgl;

// 0x407e00 normalize (rgl.h); virtual add_torque_at (slot 33, 0x425920)

static __forceinline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }

// @ 0x4259a0
void dynamic_entity::resolve()
{
    vector4 dirs[200];
    bool flagB = false;
    for (contact* c = contacts.first; c < contacts.last; c++) {
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
        contact* c = contacts.first;
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
        for (contact* c = contacts.first; c < contacts.last; c++) {
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
    for (contact* c = contacts.first; c < contacts.last; c++) {
        float d = -(force.x * c->n.x + (force.y * c->n.y + force.z * c->n.z));
        if (d > 0.0f) {
            force.x = d * c->n.x + force.x;
            force.y = d * c->n.y + force.y;
            force.z = d * c->n.z + force.z;
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
