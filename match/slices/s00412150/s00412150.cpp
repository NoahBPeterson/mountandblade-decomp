// Slice s00412150: particle emitter update / Psys destructor / sort key (provisional names).
#include "s00412150.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// 0x414ae0: noise vector for a position (cdecl, 3 args)
void noise3(float scale, const rgl::vector4* in, rgl::vector4* out);       // 0x414ae0
// 0x5e2948: table of 64-bit values indexed by Emitter::noise_index (provisional)
extern __int64 g_noise_table[];                                            // 0x5e2948

// Advances every particle by *pdt, culls dead ones (age >= 1), updates the bounding radius and spawns new
// particles from the remaining budget.
// NONMATCHING (about 24 bytes): everything but three x87 operand orders (k, dp.y, spin) and the placement of
// one lea matches.  `pv` is a second induction pointer into the current Particle (+0x48 = &vel.z) that cl
// derives itself in the original; the in-source pointer reproduces its addressing:
//   pv[-6..-4] = pos.xyz, pv[-2..0] = vel.xyz, pv[2] = age, pv[4] = angle, pv[5] = angvel.
// The in-place helpers (add_to, scale_f) take references: they stop cl forwarding the stored value, as in the
// original, which re-reads vel/pos/angvel from memory after each update.
static inline void add_to(rgl::vector4& a, const rgl::vector4& b) { a.x = b.x + a.x; a.y = b.y + a.y; a.z = b.z + a.z; }
static inline void scale_f(float& a, const float& k) { a = k * a; }
// @ 0x412150
void Emitter::update(const float* pdt, const matrix64& frame)
{
    world = frame * local;
    float maxd2 = 0.0f;
    std::vector<Particle>::iterator it;
    float* pv;
    for (it = particles.begin(), pv = (float*)((char*)&*it + 0x48); it != particles.end(); ++it, pv += 25) {
        rgl::vector4 g = vmul(accel, accel_scale);
        rgl::vector4 gd = vmul(g, *pdt);
        pv[-2] = gd.x + pv[-2];
        pv[-1] = gd.y + pv[-1];
        pv[0] = gd.z + pv[0];
        if (noise_amount > 0.01f) {
            rgl::vector4 in = *(rgl::vector4*)&pv[-6];
            float w = (float)pow((double)((float)g_noise_table[noise_index] * 1e-5f), 4000.0);
            in.z = w * noise_scale * 0.2f + in.z;
            rgl::vector4 out;
            noise3(noise_scale, &in, &out);
            if (flags & 0x10000)
                out.z = 0.0f;
            float f = noise_amount * *pdt;
            rgl::vector4 nd = vmul(out, f);
            pv[-2] = nd.x + pv[-2];
            pv[-1] = nd.y + pv[-1];
            pv[0] = nd.z + pv[0];
        }
        float kd = drag * *pdt;
        float k = 1.0f - kd;
        pv[-2] = k * pv[-2];
        pv[-1] = k * pv[-1];
        pv[0] = k * pv[0];
        rgl::vector4 dp = vmul(*pdt, *(rgl::vector4*)&pv[-2]);
        add_to(*(rgl::vector4*)&pv[-6], dp);
        float sk = 1.0f - spin_drag * *pdt;
        scale_f(pv[5], sk);
        pv[4] = *pdt * pv[5] + pv[4];
        pv[2] = *pdt / lifetime + pv[2];
        float dx = pv[-6] - world.o.x;
        float dy = pv[-5] - world.o.y;
        float dzz = pv[-4] - world.o.z;
        float d2 = dx * dx + (dy * dy + dzz * dzz);
        if (d2 > maxd2)
            maxd2 = d2;
    }
    radius = (float)sqrt(maxd2);

    int i = 0;
    unsigned n = particles.size();
    if (0 < (int)n) {
      do {
        if (particles[i].age >= 1.0f) {
            particles[i] = particles[n - 1];
            --n;
        } else {
            ++i;
        }
      } while (i < (int)n);
    }
    if (n < particles.size())
        particles.erase(particles.begin() + n, particles.end());

    if (spawn_left > 0) {
        float r = ((float)spawn_left + 10.0f) / ((float)spawn_total + 10.0f);
        if (r < 0.0f)
            r = 0.0f;
        else if (r >= 1.0f)
            r = 1.0f;
        float rf = r * f198;
        spawn_accum += rf * *pdt;
        float t = (float)(rand() % 0x3dc9) * 6.3223117e-5f + (float)(rand() % 0x3dc9) * 6.3223117e-5f;
        if (t < spawn_accum) {
            do {
                if (spawn_left <= 0)
                    return;
                spawn_accum -= t;
                --spawn_left;
                Particle np;
                spawn(&np);
                particles.push_back(np);
                t = (float)(rand() % 0x3dc9) * 6.3223117e-5f + (float)(rand() % 0x3dc9) * 6.3223117e-5f;
            } while (t < spawn_accum);
        }
    }
}

// @ 0x4125e0
void MorphSet::assign(const MorphSet& o)
{
    n = o.n;
    if (idx) {
        delete[] idx;
        idx = 0;
    }
    if (sets) {
        delete[] sets;
        sets = 0;
    }
    if (o.n) {
        idx = new int[n];
        sets = new VecPair[n];
        for (int i = 0; i < n; ++i)
            sets[i].copy_from(o.sets[i]);
        memcpy(idx, o.idx, n * 4);
    }
}

// @ 0x4126a0
void Psys::update_emitters(const float* dt, const matrix64& frame)
{
    for (std::vector<Emitter*>::iterator it = emitters.begin(); it != emitters.end(); ++it)
        (*it)->update(dt, frame * xform);
}

// @ 0x412700
void Psys::ensure_skin()
{
    Psys* p = this;
    while (!p->is_root)
        p = p->parent;
    if (p->skin.size() < p->verts.size())
        p->build_skin();
}

// @ 0x412770
template void std::vector<BoneInfluence>::_Destroy(BoneInfluence*, BoneInfluence*);
// @ 0x412790
template BoneInfluence* std::vector<BoneInfluence>::_Ucopy(BoneInfluence*, BoneInfluence*, BoneInfluence*);
// @ 0x4127b0
template void std::vector<BoneInfluence>::_Tidy();

// @ 0x4127f0
Psys::~Psys()
{
    if (def)
        def->refcount--;
    if (p220)
        delete[] p220;
    for (unsigned i = 0; i < emitters.size(); ++i) {
        Emitter* e = emitters[i];
        if (e)
            delete e;
    }
}

// @ 0x412980
void Psys::compute_sortkey()
{
    if (def->flags1c & 0x10000) {
        sortkey = -1000000000LL;
        return;
    }
    int d = own_recs ? a140 : parent->a140;
    __int64 t = (__int64)(a114 * 100000000) + (__int64)a110 * 400000000LL + (__int64)(d * 200000000);
    unsigned __int64 x = ((def->flags1c >> 24) - 8) & 0xf;
    if ((def->flags1c & 0x700) == 0x700 && (color & 0xff000000) != 0xff000000)
        x += 4;
    __int64 u = (__int64)(def->d30->i18 * 1000);
    u += (__int64)(a214 << 20);
    __int64 k = t | (x << 34);
    u += k;
    u += def->i18;
    sortkey = u;
}
