// Slice s004260a0: dynamic_entity (vtable 0x54b668) integration step and the spatial queries behind its collision pass:
// integrate (position / orientation update), collide (gather overlapping shapes and entities), the world's per-cell
// shape and entity queries with their box tests, a group-mask ordering helper and std::unique for pointer arrays.
#include <algorithm>
#include <string.h>
#include "dyn.h"

using namespace rgl;

// The original keeps vector<Obj*>'s constructor helper out of line (0x43e760); an explicit specialization declared
// without a definition makes every use a call.
// 0x43e760
template<> bool std::vector<Obj*, std::allocator<Obj*> >::_Buy(size_t);
// 0x41f4b0 (shared by every 4-byte-element vector)
template<> void std::vector<Obj*, std::allocator<Obj*> >::_Tidy();

// 0x41f4b0 contact_list::clear, 0x407e00 normalize (rgl.h), 0x43e760 std::vector<Obj*>::_Buy, 0x41f720 vector<Obj*>::_Insert_n,
// 0x426a30 sort of the result array (next slice)

// box tests: 3D (0x426690) and 2D xy (0x426700) overlap of [a, b] with [c, d]
// The callers (0x426500) see these as returning a byte (test al, al), the bodies return int: call them through bool-returning
// declarations (same addresses).
bool box3_b(const vector4* a, const vector4* b, const vector4* c, const vector4* d);   // 0x426690
bool box2_b(const vector4* a, const vector4* b, const vector4* c, const vector4* d);   // 0x426700

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

// @ 0x426960
bool world::less(Obj* a, Obj* b)
{
    int k = 0;
    unsigned ta = a->slot1();
    unsigned tb = b->slot1();
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

// @ 0x426500
void cell::collect(const vector4* bmin, const vector4* bmax, int* use3d, unsigned mask, shape** out, int* count)
{
    shape* buf[0x2000];
    int n = 0;
    struct { unsigned i; int k; } st;               // lives in memory in the original (reloaded every iteration)
    st.i = 0;
    st.k = 0;
    while (shapes.size() > st.i) {
        shape* sh = shapes[st.i];
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
            shape* o = out[st.k];
            shape* cur = shapes[st.i];
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
        shape* cur2 = shapes[st.i];
        st.i++;
        buf[n++] = cur2;
    }
    if (st.k < *count) {
        int m = *count - st.k;
        memcpy(&buf[n], &out[st.k], m * sizeof(shape*));
        n += m;
    }
    *count = n;
    for (int j = 0; j < *count; j++)
        out[j] = buf[j];
}

// @ 0x426490
void world::query_shapes(entity* e, shape** out, int* count, int* use3d, unsigned mask)
{
    *count = 0;
    for (cell_ref* r = e->cells; r != 0; r = r->next) {
        cell* c = &cells[r->grid][r->idx];
        c->collect(e->get_min(), e->get_max(), use3d, mask, out, count);
    }
}

// std::unique on vector<Obj*>::iterator (0x4269e0): the original keeps it out of line
namespace std {
// @ 0x4269e0
template<> __declspec(noinline) vector<Obj*>::iterator unique(vector<Obj*>::iterator _First, vector<Obj*>::iterator _Last)
{
    for (vector<Obj*>::iterator _Firstb; (_Firstb = _First) != _Last && ++_First != _Last; )
        if (*_Firstb == *_First) {
            for (; ++_First != _Last; )
                if (!(*_Firstb == *_First))
                    *++_Firstb = *_First;
            return (++_Firstb);
        }
    return (_Last);
}
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

// @ 0x426750
void world::query_entities(entity* e, unsigned mask, bool ordered, std::vector<Obj*>* out)
{
    out->clear();
    for (cell_ref* r = e->cells; r != 0; r = r->next) {
        for (enode* nd = cells[r->grid][r->idx].ents; nd != 0; nd = nd->next) {
            Obj** pe = &nd->ent;
            if (*pe != e) {
                if (type_ok((*pe)->slot1(), mask)) {
                  if (!ordered || less(e, *pe)) {
                    if (box3_inl(e->get_min(), e->get_max(), (*pe)->get_min(), (*pe)->get_max()))
                        out->push_back(*pe);
                  }
                }
            }
        }
    }
    std::sort(out->begin(), out->end());
    out->erase(std::unique(out->begin(), out->end()), out->end());
}

// @ 0x4263d0
void dynamic_entity::collide()
{
    shape* found[0x2000];
    volatile int count;                 // volatile: the original re-reads the count after every on_shape call
    int use3d = 1;
    owner->query_shapes(this, found, (int*)&count, &use3d, 0x8106);
    for (int i = 0; i < count; i++)
        on_shape(found[i]);
    std::vector<Obj*> others;
    owner->query_entities(this, 3, true, &others);
    for (std::vector<Obj*>::iterator it = others.begin(); it < others.end(); ++it)
        if ((*it)->is_dynamic())
            on_entity(*it);
}

static __forceinline vector4 vscale(float s, const vector4& v) { return vector4(s * v.x, s * v.y, s * v.z); }

// @ 0x4260a0
void dynamic_entity::integrate(float t)
{
    resolve();
    contacts.clear();
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
    local = m;
    local.o.x = local.o.x + shift.x;
    local.o.y = shift.y + local.o.y;
    local.o.z = shift.z + local.o.z;
    shift = vector4(0.0f, 0.0f, 0.0f);
}
