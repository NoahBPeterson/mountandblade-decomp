// Slice s00423ed0: a polygon/vertex store and its object registry (names provisional): a 2D cell-rectangle
// test, a cell's sorted object list, VC7.1 std::vector<Obj*> / vector<E36> instantiations, the registry's
// add/register path, the face-normal helper and the store's add_points (vertex + group records).
// Flags: /O2 /MT /Gy /EHa (the std::vector members only match with /EHa, see s0040f9a0).
#include <vector>
#include <list>
#include <new>
#include <memory>
#include "rgl.h"

using rgl::vector4;

struct Obj;
struct E36 { int v[9]; };       // vertex record, 0x24 bytes: position, face normal, a flag
struct E136 { int v[34]; };     // group (face) record, 0x88 bytes

// ---- scene objects ----------------------------------------------------------------------------------
// vtable slot 1 returns the kind (2 = polygon with up to 9 inline points, 3 = group of children).
struct Obj {
    virtual void v0();
    virtual int kind();
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8();
    virtual void attach(void* world);                           // slot 9
    virtual const float* bbox_min(float* tmp);                  // slot 10
    virtual const float* bbox_max(float* tmp);                  // slot 11
    int f4;                                                     // store index (set when registered)
    char pad8[8];
    int cells[4];                                               // +0x10: cell rectangle (min x, min y, max x, max y)
    union {
        struct { vector4 pts[9]; };                             // +0x20 (kind 2)
        struct { Obj** kids; int nkids; };                      // +0x20 / +0x24 (kind 3)
    };
    int npts;                                                   // +0xb0 (kind 2)
};

// vector<Obj*> stand-in whose push_back/resize stay out of line (VC7.1 layout: allocator pad, first, last, end)
struct ObjVec {
    char pad0[4];
    Obj** first; Obj** last; Obj** end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    struct Iter { Obj** p; };
    unsigned capacity() const { return first == 0 ? 0 : end - first; }
    void push_back(Obj* const& v);                              // 0x423fc0
    Iter insert(Iter where, Obj* const& v);                     // 0x424010 (out of line)
    void resize(unsigned n, Obj* v);                            // 0x41fa00
    int find(Obj* o) const;                                     // 0x4240d0 (defined on ObjVecF below)
    void remove(Obj* o);                                        // 0x424060
};

// std::list<Obj*> header (allocator pad, head sentinel, size); _Insert stays out of line
struct ObjList {
    char pad0[4];
    struct Node { Node* next; Node* prev; Obj* obj; } *head;
    int size;
    void _Insert(Node* where, Obj* const& v);                   // 0x41f640
};

// a grid cell (0x34 bytes): sorted objects and the list of every object touching the cell
struct Cell : ObjVec {
    ObjList objs;                                               // +0x10
    char pad1c[0x34 - 0x1c];
    void add(Obj* x);                                           // 0x423f10
};

// @ 0x423ed0
int __stdcall in_rect(const int* p, const int* lo, const int* hi)
{
    if (p[0] >= lo[0] && p[0] <= hi[0] && p[1] >= lo[1] && p[1] <= hi[1])
        return 1;
    return 0;
}

// @ 0x423f10
void Cell::add(Obj* x)
{
    unsigned pos = (unsigned)-1;
    unsigned i = 0;
    for (;;) {
        if (i >= size())
            break;
        if (first[i] > x)
            pos = i;
        i++;
        if (pos != (unsigned)-1)
            break;
    }
    if (pos == (unsigned)-1) {
        push_back(x);
    } else {
        resize(size() + 1, 0);
        for (int j = size() - 1; j > (int)pos; j--)
            first[j] = first[j - 1];
        first[pos] = x;
    }
    ObjList* l = &objs;
    l->_Insert(l->head, x);
}

// @ 0x423fc0
void ObjVec::push_back(Obj* const& v)
{
    if (size() < capacity()) {
        Obj** p = last;
        *p = v;
        last = p + 1;
    } else
        { Iter w; w.p = last; insert(w, v); }
}

// the same record, only to define find(): a callee in this TU would give remove() interprocedural register info
struct ObjVecF {
    char pad0[4];
    Obj** first; Obj** last; Obj** end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    int find(Obj* o) const;                                     // 0x4240d0
};
// @ 0x4240d0
int ObjVecF::find(Obj* o) const
{
    for (unsigned i = 0; first != 0 && i < size(); i++)
        if (first[i] == o)
            return i;
    return -1;
}

// @ 0x424060
void ObjVec::remove(Obj* o)
{
    int idx = find(o);
    if (idx >= 0) {
        unsigned i = idx;
        while (i < size() - 1) {
            Obj** f = first;
            f[i] = f[i + 1];
            i++;
        }
        resize(size() - 1, 0);
    }
}

// ---- the vertex / group store --------------------------------------------------------------------------
// vertex record (E36): position, face normal, a flag
struct Vert { vector4 p; vector4 n; int f20; };

// group record (E136): up to four corners
struct Group {
    int n;                      // +0
    int vidx[4];                // +4   first vertex index per corner
    char pad14[0x10];
    int f24;                    // +0x24
    int link[4];                // +0x28 (-1)
    int f38[4];                 // +0x38 (0)
    int f48, f4c, f50;          // +0x48 (-1)
    char pad54[0x10];
    float f64;                  // +0x64 (1.0)
    int f68, f6c, f70;          // +0x68 (0)
    vector4 c;                  // +0x74 sum of the corner positions
    int f84;                    // +0x84 (0)
    Group();                    // 0x4246f0
};
// vector<E36> / vector<E136> stand-ins (resize stays out of line)
struct V36 {
    char pad0[4];
    Vert *first, *last, *end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    void resize(unsigned n, Vert v);                            // 0x424740
};
struct V136 {
    char pad0[4];
    Group *first, *last, *end;
    unsigned size() const { return first == 0 ? 0 : last - first; }
    void resize(unsigned n, Group v);                           // 0x424cc0
    struct iter { Group* p; iter operator+(int off) const; };   // 0x424720 (out of line)
    iter begin() const { iter r; r.p = first; return r; }
};
// @ 0x424720
V136::iter V136::iter::operator+(int off) const { iter r; r.p = p + off; return r; }

// @ 0x424650: unit normal of the triangle (a, b, c); returns the vector4 by value (hidden pointer)
vector4 face_normal(const vector4* a, const vector4* b, const vector4* c)
{
    float bx = b->x - a->x;
    float by = b->y - a->y;
    float bz = b->z - a->z;
    vector4 d;
    d.x = c->x - a->x;
    d.y = c->y - a->y;
    d.z = c->z - a->z;
    vector4 n;
    n.x = d.z * by - d.y * bz;
    n.y = bz * d.x - d.z * bx;
    n.z = d.y * bx - d.x * by;
    n.normalize();
    return n;
}

// the caller's view of face_normal: the by-value return written as its out-parameter (same ABI)
vector4* face_normal_x(vector4* out, const vector4* a, const vector4* b, const vector4* c);    // 0x424650

struct Store {
    V36 verts;                  // +0
    V136 groups;                // +0x10
    int add(int n, const vector4* pts);                         // 0x424220
};
// @ 0x424220
int Store::add(int n, const vector4* pts)
{
    unsigned base = verts.size();
    {
        Vert t36;
        verts.resize(base + n, t36);
    }
    vector4 nrm;
    face_normal_x(&nrm, pts, pts + 1, pts + 2);
    vector4 sum;
    sum.x = 0.0f; sum.y = 0.0f; sum.z = 0.0f;
    for (int i = 0; i < n; i++) {
        verts.first[base + i].p = pts[i];
        verts.first[base + i].n = nrm;
        verts.first[base + i].f20 = 0;
        sum.x += pts[i].x;
        sum.y += pts[i].y;
        sum.z += pts[i].z;
    }
    unsigned idx = groups.size();
    Group g;
    groups.resize(idx + 1, g);
    groups.first[idx].n = n;
    groups.first[idx].c = sum;
    groups.first[idx].f24 = 0;
    groups.first[idx].f68 = 0;
    for (int k = 0; k < n; k++) {
        groups.first[idx].link[k] = -1;
        groups.first[idx].vidx[k] = base + k;
        V136::iter it = groups.begin() + idx;
        it.p->f38[k] = 0;
    }
    return idx;
}

// @ 0x4246f0
Group::Group()
{
    f24 = 0;
    f6c = 0;
    f70 = 0;
    f68 = 0;
    f84 = 0;
    f64 = 1.0f;
    f48 = -1;
    f4c = -1;
    f50 = -1;
}

// ---- registry --------------------------------------------------------------------------------------------
// out-of-line callees for the registry (a callee defined in this TU would be inlined / change cl's register use)
struct StoreX { int add(int n, const vector4* pts); };                 // 0x424220
struct ObjVecX {
    char pad0[4];
    Obj** first; Obj** last; Obj** end;
    void push_back(Obj* const& v);                                       // 0x423fc0
};
struct Registry {
    char pad0[0x54];
    ObjVecX items;              // +0x54
    char pad1[0xb0 - 0x54 - sizeof(ObjVecX)];
    StoreX* store;               // +0xb0
    void reg(Obj* o);                                           // 0x424170
    void post(Obj* o);                                          // 0x425240
};
// @ 0x424170
void Registry::reg(Obj* o)
{
    if (o->kind() == 2) {
        o->f4 = store->add(o->npts, o->pts);
    } else if (o->kind() == 3) {
        for (int i = 0; i < o->nkids; i++) {
            if (o->kids[i]->kind() == 2) {
                Obj* k = o->kids[i];
                o->kids[i]->f4 = store->add(k->npts, k->pts);
            }
        }
    } else {
        o->kind();
    }
    Obj* t = o;
    items.push_back(t);
    post(t);
}

struct Scene {
    char pad0[4];
    Registry* reg;              // +4
    char world[0x128 - 8];      // +8
    std::vector<Obj*> objs;     // +0x128
    void add(Obj* o);                                           // 0x424100
};
// @ 0x424100
void Scene::add(Obj* o)
{
    objs.push_back(o);
    o->attach(world);
    reg->reg(o);
}

namespace std {
// @ 0x424010
template vector<Obj*>::iterator vector<Obj*>::insert(vector<Obj*>::iterator, Obj* const&);
// @ 0x424740
template void vector<E36>::resize(size_t, E36);
// @ 0x4247e0
template vector<E36>::iterator vector<E36>::erase(vector<E36>::iterator, vector<E36>::iterator);
}
