// records.h: the small record types of the std::vector / std::stable_sort instantiations of terrain_b.cpp, the
// name + five-array record (ArrSet), and the 0x1c8-byte polymorphic record (Rec1c8) that the same vectors hold.
// Only sizes, offsets and the constructors the instantiations expose are verified (cited per type); the object
// that owns each vector is elsewhere. Names provisional (the slices' names are kept where symbols/slices maps them).
#ifndef TERRAIN_B_RECORDS_H
#define TERRAIN_B_RECORDS_H
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include "../tstring.h"
#include "./poly_mesh.h"

// std::_Nonscalar_ptr_iterator_tag: an UNINITIALISED local pushes whatever EAX holds
struct ptr_tag {};

// ---- plain records --------------------------------------------------------------------------------------------------
struct MRow36  { int v[9]; };      // 0x24: vector inserted by 0x477410, _Insert_n 0x476370, push_back 0x477b80
struct E36x { int v[9]; };      // 0x24: a second 36-byte record type: reserve 0x476020, insert 0x476240 (its _Insert_n is the earlier
                                // object's 0x424830), push_back 0x4772f0 (the slices called it rec36 / E36x)
struct E136 { int v[34]; };     // 0x88: reserve 0x476130, insert 0x4762b0 (_Insert_n 0x424db0), push_back 0x477380 (rec136 / E136)
// 0x38-byte record: three words, two vector<int> (+0xc, +0x1c), three words (copy_e56 0x477940, vector 0x479930 / 0x47a980)
struct MRow56 {
    int a, b, c;
    std::vector<int> v1;        // +0x0c
    std::vector<int> v2;        // +0x1c
    int d, e, f;
};
// 8-byte sort record with a function-pointer predicate (the std::stable_sort instantiation 0x472870 .. 0x475f70)
struct P8 { int key, val; };
typedef bool (__cdecl *Pred8)(const P8&, const P8&);
typedef std::vector<P8>::iterator It8;

// ---- vector::_Ufill wrappers: the callee is the earlier object's _Uninit_fill_n -----------------------------------
// The wrappers stand for the _Ufill members of vector<MRow36>, vector<MVert>, vector<MPoly> and vector<MEdge> that
// _Insert_n (0x476370, 0x476620, 0x4768f0, 0x476bb0) calls; the fill itself is an instance in an earlier object.
struct alloc18 {}; struct alloc24 {}; struct alloc34 {}; struct alloc64 {};
MVert* __cdecl uninit_fill_n_18(MVert* f, unsigned n, const MVert& v, alloc18& al, ptr_tag tag);   // 0x46b570
MRow36* __cdecl uninit_fill_n_24(MRow36* f, unsigned n, const MRow36& v, alloc24& al, ptr_tag tag);         // 0x46b500
MEdge* __cdecl uninit_fill_n_34(MEdge* f, unsigned n, const MEdge& v, alloc34& al, ptr_tag tag);   // 0x46b650
MPoly* __cdecl uninit_fill_n_64(MPoly* f, unsigned n, const MPoly& v, alloc64& al, ptr_tag tag);   // 0x46b5e0
struct vec24 : alloc24 { MRow36* first; MRow36* last; MRow36* end; MRow36* ufill(MRow36* f, unsigned n, const MRow36& v); };            // 0x4724c0
struct vec18 : alloc18 { MVert* first; MVert* last; MVert* end; MVert* ufill(MVert* f, unsigned n, const MVert& v); };   // 0x4724f0
struct vec64 : alloc64 { MPoly* first; MPoly* last; MPoly* end; MPoly* ufill(MPoly* f, unsigned n, const MPoly& v); };   // 0x472520
struct vec34 : alloc34 { MEdge* first; MEdge* last; MEdge* end; MEdge* ufill(MEdge* f, unsigned n, const MEdge& v); };   // 0x472550

// ---- ArrSet: name + five (count, int array) pairs (0x38 bytes) -----------------------------------------------------
// The destructor is an inline function whose out-of-line copy is the one at 0x463760 (the earlier object's COMDAT wins).
struct ArrSet {
    tstring name;                               // +0
    int cnt[5];                                 // +0x10
    int* arr[5];                                // +0x24
    ArrSet(const ArrSet& o);                    // 0x4636d0
    ~ArrSet() { for (int i = 0; i < 5; i++) if (arr[i]) delete[] arr[i]; }       // 0x463760
};

// The range algorithms of vector<ArrSet> are the object's own out-of-line instances (erase / resize call them):
namespace std {
template<> void _Destroy_range<ArrSet, allocator<ArrSet> >(ArrSet*, ArrSet*, allocator<ArrSet>&, _Nonscalar_ptr_iterator_tag);   // 0x4727a0
template<> ArrSet* _Copy_opt<ArrSet*, ArrSet*>(ArrSet*, ArrSet*, ArrSet*, _Nonscalar_ptr_iterator_tag);                         // 0x468050
}

// ---- std::vector<ArrSet> helpers: the allocator-carrying wrappers around the earlier objects' algorithms ----------------
struct alloc_arrset {
    __declspec(noinline) void construct(ArrSet* p, const ArrSet& v);     // 0x46b6c0
    void destroy(ArrSet* p) { p->~ArrSet(); }
};
struct allocArr {};
ArrSet* __cdecl uninit_copy_arrset(ArrSet* f, ArrSet* l, ArrSet* d, allocArr& al, ptr_tag tag);    // 0x46b750
ArrSet* __cdecl uninit_copy_arrset2(ArrSet* f, ArrSet* l, ArrSet* d, allocArr& al, ptr_tag tag);   // 0x46c1d0 (a second instance)
ArrSet* __cdecl copy_arrset(ArrSet* f, ArrSet* l, ArrSet* d, ptr_tag tag);                          // 0x468050
struct vecArr : allocArr { ArrSet* first; ArrSet* last; ArrSet* end; ArrSet* ucopy(ArrSet* f, ArrSet* l, ArrSet* d); };    // 0x472630
struct vecArr2 : allocArr { ArrSet* first; ArrSet* last; ArrSet* end; ArrSet* ucopy(ArrSet* f, ArrSet* l, ArrSet* d); };   // 0x474350
struct vecArrS : alloc_arrset {
    ArrSet* first; ArrSet* last; ArrSet* end;
    void destroy(ArrSet* f, ArrSet* l);                 // 0x475b60
    ArrSet* ufill(ArrSet* f, unsigned n, const ArrSet& v);   // 0x475b80
};
// The wrappers reach the object's own range algorithms through the same addresses, without seeing their bodies.
void __cdecl destroy_range_arrset_x(ArrSet* first, ArrSet* last, alloc_arrset& al, ptr_tag tag);     // 0x4727a0 (destroy_arrsets)
void __cdecl uninit_fill_n_arrset_x(ArrSet* first, unsigned n, const ArrSet& val, alloc_arrset& al, ptr_tag tag);   // 0x4727e0 (uninit_fill_n_arrset)

// ---- the 0x1c8-byte polymorphic record (vtable 0x590b84; copy ctor 0x437bb0, operator= 0x437d60, reset 0x4352f0) -----
extern int g_tick_lo;                           // 0x5e2958 (g_ticks[2], 1e-5 s ticks)
extern int g_tick_hi;                           // 0x5e295c
struct IfaceB {
    int f4, f8, fc;
    IfaceB() {}
    virtual ~IfaceB() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
struct Quad {
    int a, b, c, d;
    Quad() : a(2), c(g_tick_lo), d(g_tick_hi) {}
};
// The slices called it Rec1c8, MAgent, PropRec and PlacedObj (symbols/slices maps ??_7Agent@@, ??_7Rec1c8@@, ??_7PlacedObj@@).
struct EntFlag;
struct stream_base;
struct Rec1c8 : IfaceB {
    int m10;                                    // +0x10 (0x479000 stores the scene id)
    char body1[0x170 - 0x14];
    Quad q170;                                  // +0x170 (constructed by the inlined constructor 0x437830)
    char body2[0x188 - 0x180];
    int destroyed;                              // +0x188 (0 intact, 1 / 5 special states) (0x472050)
    int kind;                                   // +0x18c (0x472050)
    tstring name;                               // +0x190
    char body3[0x1b4 - 0x1a0];
    EntFlag* ent;                               // +0x1b4 scene entity of the prop (0x472050)
    char body4[0x1c8 - 0x1b8];
    Rec1c8() { reset(); }                       // 0x437830 (inlined)
    void reset();                               // 0x4352f0
    Rec1c8(const Rec1c8& o);                    // 0x437bb0
    Rec1c8& operator=(const Rec1c8& o);         // 0x437d60
    void read(stream_base* s, int version);     // 0x4631c0 (the slices' Placed::read)
};

#endif // TERRAIN_B_RECORDS_H
