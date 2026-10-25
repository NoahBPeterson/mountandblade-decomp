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

#include "../../include/ptr_tag.h"

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
struct alloc36 {}; struct alloc52 {}; struct alloc100 {};
// The callees are the cdecl _Uninit_fill_n instances of src/game/terrain_a (misc.h), declared there for the element types elem24 /
// rec36 / elem52 / particle100 with their allocators alloc24 / alloc36 / alloc52 / alloc100 and WITHOUT the empty `tag`
// parameter. The vectors here hold layout-identical records (MVert, MRow36, MEdge, MPoly), and the original still pushes the tag's
// slot: the helpers below take the definitions' addresses and call them through the five-parameter type (cdecl: the caller pops).
struct elem24; struct rec36; struct elem52; struct particle100;
void __cdecl uninit_fill_n24(elem24* f, unsigned n, const elem24& v, alloc24& al);            // 0x46b570
void __cdecl uninit_fill_n36(rec36* f, unsigned n, const rec36& v, alloc36& al);              // 0x46b500
void __cdecl uninit_fill_n52(elem52* f, unsigned n, const elem52& v, alloc52& al);            // 0x46b650
void __cdecl uninit_fill_n100(particle100* f, unsigned n, const particle100& v, alloc100& al); // 0x46b5e0
typedef void (__cdecl *fill24_t)(elem24*, unsigned, const elem24&, alloc24&, ptr_tag);
typedef void (__cdecl *fill36_t)(rec36*, unsigned, const rec36&, alloc36&, ptr_tag);
typedef void (__cdecl *fill52_t)(elem52*, unsigned, const elem52&, alloc52&, ptr_tag);
typedef void (__cdecl *fill100_t)(particle100*, unsigned, const particle100&, alloc100&, ptr_tag);
struct vecMRow36 : alloc24 { MRow36* first; MRow36* last; MRow36* end; MRow36* ufill(MRow36* f, unsigned n, const MRow36& v); };            // 0x4724c0
struct vec18 : alloc18 { MVert* first; MVert* last; MVert* end; MVert* ufill(MVert* f, unsigned n, const MVert& v); };   // 0x4724f0
struct vec64 : alloc64 { MPoly* first; MPoly* last; MPoly* end; MPoly* ufill(MPoly* f, unsigned n, const MPoly& v); };   // 0x472520
struct vec34 : alloc34 { MEdge* first; MEdge* last; MEdge* end; MEdge* ufill(MEdge* f, unsigned n, const MEdge& v); };   // 0x472550

// ---- ArrSet: name + five (count, int array) pairs (0x38 bytes) -----------------------------------------------------
#include "../arr_set.h"

// The range algorithms of vector<ArrSet> are the object's own out-of-line instances (erase / resize call them): terrain_b.cpp's
// destroy_arrsets (0x4727a0) and terrain_a.cpp's copy_arrset (0x468050, declared for its elem56 view of the same record). The
// specialisations forward to them by name through the four-parameter type (the caller still pushes the tag slot), so the
// relink binds them directly (the linker cannot alias a `??$` template name).
struct elem56;
elem56* __cdecl copy_arrset(elem56* f, elem56* l, elem56* d);                                     // 0x468050
void __cdecl destroy_arrsets(ArrSet* first, ArrSet* last);                                       // 0x4727a0
namespace std {
template<> inline void _Destroy_range<ArrSet, allocator<ArrSet> >(ArrSet* f, ArrSet* l, allocator<ArrSet>& al, _Nonscalar_ptr_iterator_tag t)   // 0x4727a0
{
    ((void (__cdecl*)(ArrSet*, ArrSet*, allocator<ArrSet>&, _Nonscalar_ptr_iterator_tag))destroy_arrsets)(f, l, al, t);
}
template<> inline ArrSet* _Copy_opt<ArrSet*, ArrSet*>(ArrSet* f, ArrSet* l, ArrSet* d, _Nonscalar_ptr_iterator_tag t)   // 0x468050
{
    return ((ArrSet* (__cdecl*)(ArrSet*, ArrSet*, ArrSet*, _Nonscalar_ptr_iterator_tag))copy_arrset)(f, l, d, t);
}
}

// ---- std::vector<ArrSet> helpers: the allocator-carrying wrappers around the earlier objects' algorithms ----------------
struct alloc_arrset {
    __declspec(noinline) void construct(ArrSet* p, const ArrSet& v);     // 0x46b6c0
    __declspec(noinline) void construct_arrset(ArrSet* p, const ArrSet& v); // 0x46b6c0
    void destroy(ArrSet* p) { p->~ArrSet(); }
};

// 0x46b750: terrain_a's instance is declared for rec136 / alloc136 (the same 0x38-byte record) with std's own tag type;
// 0x46c1d0 has no tag parameter (the caller still pushes its slot)
struct rec136; struct alloc136;
rec136* __cdecl uninit_copy_arrset(rec136* f, rec136* l, rec136* d, alloc136& al, std::_Nonscalar_ptr_iterator_tag tag);   // 0x46b750
ArrSet* __cdecl uninit_copy_arrset2(ArrSet* f, ArrSet* l, ArrSet* d, allocArr& al);   // 0x46c1d0 (a second instance)
typedef ArrSet* (__cdecl *copy2_t)(ArrSet*, ArrSet*, ArrSet*, allocArr&, ptr_tag);
ArrSet* __cdecl copy_arrset(ArrSet* f, ArrSet* l, ArrSet* d, ptr_tag tag);                          // 0x468050
struct vecArr : allocArr { ArrSet* first; ArrSet* last; ArrSet* end; ArrSet* ucopy(ArrSet* f, ArrSet* l, ArrSet* d); };    // 0x472630
struct vecArr2 : allocArr { ArrSet* first; ArrSet* last; ArrSet* end; ArrSet* ucopy(ArrSet* f, ArrSet* l, ArrSet* d); };   // 0x474350
struct vecArrS : alloc_arrset {
    ArrSet* first; ArrSet* last; ArrSet* end;
    void destroy(ArrSet* f, ArrSet* l);                 // 0x475b60
    ArrSet* ufill(ArrSet* f, unsigned n, const ArrSet& v);   // 0x475b80
};
// The wrappers reach the object's own range algorithms through the same addresses, without seeing their bodies.
void __cdecl destroy_arrsets(ArrSet* first, ArrSet* last);     // 0x4727a0 (the definition has no allocator / tag parameters; the caller still pushes them)
typedef void (__cdecl *destroy_arrsets_t)(ArrSet*, ArrSet*, alloc_arrset&, ptr_tag);
void __cdecl uninit_fill_n_arrset(ArrSet* first, unsigned n, const ArrSet& val, alloc_arrset& al, ptr_tag tag);   // 0x4727e0

// ---- the 0x1c8-byte polymorphic record (vtable 0x590b84; copy ctor 0x437bb0, operator= 0x437d60, reset 0x4352f0) -----
// The slices called it Rec1c8, MAgent, PropRec and PlacedObj (symbols/slices maps ??_7Agent@@, ??_7Rec1c8@@, ??_7PlacedObj@@).
#include "../../include/rec1c8.h"      // Rec1c8, IfaceB, Quad (shared)

#endif // TERRAIN_B_RECORDS_H
