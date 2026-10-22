// Slice s004724f0: VC7.1 <vector>/<algorithm> instantiations for assorted record types (uninitialised fills and copies,
// range erase, the stable_sort merge machinery for 8-byte records), the Newton ragdoll velocity clamp (0x472c50) and
// the arrow-vs-ground correction (0x472650). All names provisional.
#include <vector>
#include <algorithm>
#include <math.h>
#include "rgl.h"
using rgl::vector4;

struct ptr_tag {};          // std::_Nonscalar_ptr_iterator_tag: an UNINITIALISED local pushes whatever EAX holds

// ---- _Uninitialized_fill_n wrappers (vector::_Ufill) for four record sizes ------------------------------------------
struct R18 { char b[0x18]; };
struct R64 { char b[0x64]; };
struct R34 { int d[13]; };
struct alloc18 {}; struct alloc64 {}; struct alloc34 {};
R18* __cdecl uninit_fill_n_18(R18* f, unsigned n, const R18& v, alloc18& al, ptr_tag tag);   // 0x46b570
R64* __cdecl uninit_fill_n_64(R64* f, unsigned n, const R64& v, alloc64& al, ptr_tag tag);   // 0x46b5e0
R34* __cdecl uninit_fill_n_34(R34* f, unsigned n, const R34& v, alloc34& al, ptr_tag tag);   // 0x46b650
struct vec18 : alloc18 { R18* first; R18* last; R18* end; R18* ufill(R18* f, unsigned n, const R18& v); };   // 0x4724f0
struct vec64 : alloc64 { R64* first; R64* last; R64* end; R64* ufill(R64* f, unsigned n, const R64& v); };   // 0x472520
struct vec34 : alloc34 { R34* first; R34* last; R34* end; R34* ufill(R34* f, unsigned n, const R34& v); };   // 0x472550
// @ 0x4724f0
R18* vec18::ufill(R18* f, unsigned n, const R18& v)
{
    ptr_tag t;
    uninit_fill_n_18(f, n, v, *this, t);
    return f + n;
}
// @ 0x472520
R64* vec64::ufill(R64* f, unsigned n, const R64& v)
{
    ptr_tag t;
    uninit_fill_n_64(f, n, v, *this, t);
    return f + n;
}
// @ 0x472550
R34* vec34::ufill(R34* f, unsigned n, const R34& v)
{
    ptr_tag t;
    uninit_fill_n_34(f, n, v, *this, t);
    return f + n;
}

// ---- std::vector<R34>::erase(first, last) ----------------------------------------------------------------------------
// @ 0x472580
template std::vector<R34>::iterator std::vector<R34>::erase(std::vector<R34>::iterator, std::vector<R34>::iterator);

// ---- std::vector<PlacedObj>::erase(first, last): the 0x1c8-byte polymorphic record (vtable 0x590b84) -----------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Rec1c8 : IfaceB {
    char body1[0x190 - 0x10];
    tstring name;                               // +0x190
    char body2[0x1c8 - 0x1a0];
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
};
// @ 0x4725d0
template std::vector<Rec1c8>::iterator std::vector<Rec1c8>::erase(std::vector<Rec1c8>::iterator, std::vector<Rec1c8>::iterator);

// ---- ArrSet: name + five (count, int array) pairs (0x38 bytes; see s00462e20) ----------------------------------------
struct ArrSet {
    tstring name;                               // +0
    int cnt[5];                                 // +0x10
    int* arr[5];                                // +0x24
    ~ArrSet();                                  // 0x463760
};
struct ArrSetI {                                // the same record seen with its destructor inlined
    tstring name;
    int cnt[5];
    int* arr[5];
};
// @ 0x4727a0
void __cdecl destroy_arrsets(ArrSetI* first, ArrSetI* last)
{
    for (; first != last; ++first) {
        int** q = first->arr;
        for (int n = 5; n != 0; n--, q++)
            if (*q)
                delete[] *q;
        first->name.~tstring();
    }
}

// std::_Uninit_fill_n for ArrSet with its catch(...) funclet 0x472848
struct alloc_arrset {
    __declspec(noinline) void construct(ArrSet* p, const ArrSet& v);     // 0x46b6c0
    void destroy(ArrSet* p) { p->~ArrSet(); }
};
// @ 0x4727e0
void __cdecl uninit_fill_n_arrset(ArrSet* first, unsigned n, const ArrSet& val, alloc_arrset& al, ptr_tag tag)
{
    ArrSet* next = first;
    try {
        for (; 0 < n; --n, ++first)
            al.construct(first, val);
    } catch (...) {
        for (; next != first; ++next)
            al.destroy(next);
        throw;
    }
}
// @ 0x472848
// (catch funclet of uninit_fill_n_arrset)

// ---- _Ucopy (vector<ArrSet>) -------------------------------------------------------------------------------------------
struct allocArr {};
ArrSet* __cdecl uninit_copy_arrset(ArrSet* f, ArrSet* l, ArrSet* d, allocArr& al, ptr_tag tag);    // 0x46b750
struct vecArr : allocArr { ArrSet* first; ArrSet* last; ArrSet* end; ArrSet* ucopy(ArrSet* f, ArrSet* l, ArrSet* d); };
// @ 0x472630
ArrSet* vecArr::ucopy(ArrSet* f, ArrSet* l, ArrSet* d)
{
    ptr_tag t;
    return uninit_copy_arrset(f, l, d, *this, t);
}

// ---- std::stable_sort of 8-byte records with a function-pointer predicate (VC7.1 <algorithm>) ----------------------------
struct P8 { int key, val; };
typedef bool (__cdecl *Pred8)(const P8&, const P8&);
typedef std::vector<P8>::iterator It8;
// @ 0x472870 (_Chunked_merge: merge adjacent chunks into the temp buffer)
template void std::_Chunked_merge(It8, It8, std::_Temp_iterator<P8>, int, int, Pred8);
// @ 0x472980 (merge)
template It8 std::merge(P8*, P8*, P8*, P8*, It8, Pred8);
// @ 0x472a20 (_Merge_backward)
template It8 std::_Merge_backward(It8, It8, P8*, P8*, It8, Pred8);
// @ 0x472ad0 (_Buffered_rotate)
template It8 std::_Buffered_rotate(It8, It8, It8, int, int, std::_Temp_iterator<P8>&);

// ---- PhysSet::clamp_rec: limit the linear and angular speed of every ragdoll bone of record i ----------------------------
struct NewtonBody; struct NewtonRagDollBone;
extern "C" {
__declspec(dllimport) void __cdecl NewtonBodyGetOmega(const NewtonBody* b, float* omega);                     // 0x549208
__declspec(dllimport) void __cdecl NewtonBodyGetVelocity(const NewtonBody* b, float* v);                      // 0x549210
__declspec(dllimport) void __cdecl NewtonBodySetVelocity(const NewtonBody* b, const float* v);                // 0x549214
__declspec(dllimport) void __cdecl NewtonBodySetOmega(const NewtonBody* b, const float* omega);               // 0x549218
__declspec(dllimport) NewtonBody* __cdecl NewtonRagDollBoneGetBody(const NewtonRagDollBone* b);               // 0x549294
}
struct NewtonWorld;
struct PhysRec {                                  // 0x38 bytes
    int f0;
    const NewtonWorld* world;                     // +4
    char pad8[4];
    std::vector<const NewtonRagDollBone*> bones;  // +0xc (first at +0x10, last at +0x14)
    std::vector<const NewtonBody*> bodies;        // +0x1c
    char pad2c[0x38 - 0x2c];
};
struct PhysSet {
    std::vector<PhysRec> recs;                    // +0 (first at +4)
    void clamp_rec(int i);                        // 0x472c50
};
// @ 0x472c50
void PhysSet::clamp_rec(int i)
{
    if (recs[i].world) {
        for (unsigned j = 0; j < recs[i].bones.size(); j++) {
            NewtonBody* b = NewtonRagDollBoneGetBody(recs[i].bones[j]);
            if (b) {
                vector4 v;
                NewtonBodyGetVelocity(b, &v.x);
                if (v.length() > 30.0f) {
                    v.normalize();
                    v.x *= 30.0f;
                    v.y *= 30.0f;
                    v.z *= 30.0f;
                    NewtonBodySetVelocity(b, &v.x);
                }
                NewtonBodyGetOmega(b, &v.x);
                if (v.length() > 9.0f) {
                    v.normalize();
                    v.x *= 9.0f;
                    v.y *= 9.0f;
                    v.z *= 9.0f;
                    NewtonBodySetOmega(b, &v.x);
                }
            }
        }
    }
}

// ---- 0x472650: push a flying projectile out of the ground it has sunk into ------------------------------------------------
// MissileG is in an anonymous namespace so that cl knows which registers its (in-TU) ammo_scale touches: the original
// keeps the "true" result in DL across that call.
struct HeightField {
    void sample(float x, float y, float* out_h, int* out_id);      // 0x433510 (thiscall, ret 8 x2)
};
struct GroundFace {                                // 0x88 bytes; the plane (normal, distance) sits at +0x54
    char pad0[0x54];
    vector4 plane;
    char pad64[0x88 - 0x64];
};
struct FaceTable { char pad0[0x14]; GroundFace* faces; };
struct SceneG {
    char pad0[0x64];
    HeightField* hf;                               // +0x64
    char pad68[0xb0 - 0x68];
    FaceTable* ground;                             // +0xb0
};
struct ItemKindG { char pad0[0x20c]; int f20c; char pad210[0x238 - 0x210]; };
extern ItemKindG* g_items_g;                       // 0x5e29d0
struct MissileOwner { char pad[0x88]; vector4 hit; };
struct QueryPt { char pad0[0x30]; float x, y, z; };
static inline float dot3(const vector4& a, const vector4& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
namespace {
struct MissileG {
    char pad0[0x20];
    vector4 pos;                                   // +0x20
    char pad30[0x40 - 0x30];
    vector4 dir;                                   // +0x40
    MissileOwner* owner;                           // +0x50
    char pad54[0x88 - 0x54];
    int ammo_item;                                 // +0x88
    __declspec(noinline) float ammo_scale();       // 0x467f80 (a copy of the other TU's function)
    bool settle(const char* q);                    // 0x472650
};
struct MissileGMission { SceneG* scene; };
float MissileG::ammo_scale()
{
    if (ammo_item >= 0)
        return (float)g_items_g[ammo_item].f20c * 0.01f;
    return 0.8f;
}
}
extern MissileGMission* g_mission_g;               // 0x5e292c
// @ 0x472650
bool MissileG::settle(const char* arg)
{
    const QueryPt* q = (const QueryPt*)arg;
    bool r = false;
    HeightField* hf = g_mission_g->scene->hf;
    if (hf) {
        float h;
        int id;
        hf->sample(q->x, q->y, &h, &id);
        if (q->z < h) {
            r = true;
            if (id >= 0) {
                GroundFace* gf = &g_mission_g->scene->ground->faces[id];
                vector4 pl = gf->plane;
                vector4 n;
                n.x = -pl.x; n.y = -pl.y; n.z = -pl.z;
                float t = (ammo_scale() + 0.02f) - ((h - q->z) * pl.z) / dot3(n, dir);
                vector4 d(t * dir.x, t * dir.y, t * dir.z);
                vector4* p = &pos;
                p->x += d.x; p->y += d.y; p->z += d.z;
                vector4* hp = &owner->hit;
                *hp = *p;
            }
        }
    }
    return r;
}
