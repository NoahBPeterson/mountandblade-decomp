// Slice s00476bb0: VC7.1 <vector> instantiations: _Insert_n for the 52-byte POD element and for ArrSet
// (0x38 bytes: tstring name + five count/array pairs), plus vector<ArrSet>/vector<Item> member functions.
#include <vector>
#include <memory>
#include "tstring.h"

struct E52 { int v[13]; };     // 0x34

struct ArrSet {
    tstring name;                                          // +0
    int cnt[5];                                            // +0x10
    int* arr[5];                                           // +0x24
    ArrSet(const ArrSet& o);                               // 0x4636d0
    // 0x463760; defined inline here: the 4-arg _Destroy_range (0x4727a0) inlines it, _Insert_n keeps the call
    ~ArrSet() { for (int i = 0; i < 5; i++) if (arr[i]) delete[] arr[i]; }
};

// @ 0x476bb0 (+ 0x476d51: catch funclet and cold branches)
template void std::vector<E52>::_Insert_n(std::vector<E52>::iterator, size_t, const E52&);
// @ 0x476e60 (+ 0x47701d, 0x4770dc)
template void std::vector<ArrSet>::_Insert_n(std::vector<ArrSet>::iterator, size_t, const ArrSet&);

// ---- other element types ---------------------------------------------------------------------------------
struct E24   { int v[6]; };    // 0x18
struct E36   { int v[9]; };    // 0x24 (vector inserted by 0x477410, callee of push_back 0x477b80)
struct E36x  { int v[9]; };    // 0x24 (a second 36-byte record type; push_back 0x4772f0 uses its own helpers 0x424c40/0x476240)
struct E136  { int v[34]; };   // 0x88

// 0x1c8-byte polymorphic record (vtable 0x590b84, copy ctor 0x437bb0, operator= 0x437d60): see s0043e514 / s004375b0.
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

// @ 0x477160 vector<Rec1c8>::operator=
template std::vector<Rec1c8>& std::vector<Rec1c8>::operator=(const std::vector<Rec1c8>&);
// @ 0x4772f0
template void std::vector<E36x>::push_back(const E36x&);
// @ 0x477380
template void std::vector<E136>::push_back(const E136&);
// @ 0x477410
template std::vector<E36>::iterator std::vector<E36>::insert(std::vector<E36>::iterator, const E36&);
// @ 0x477480
template void std::vector<E24>::resize(size_t, E24);
