// Slice s00477530: VC7.1 <vector> instantiations (insert / resize / push_back for the 24-, 36-, 52-, 100-byte
// records, Rec1c8 and ArrSet), the E56 range copy, a mesh face-flip helper, the RecZ constructor and an empty
// virtual slot.
#include <vector>
#include <memory>
#include "tstring.h"
#include "mesh.h"

// ---- element types -----------------------------------------------------------------------------------------
struct E24 { int v[6]; };       // 0x18
struct E36 { int v[9]; };       // 0x24
// 0x34-byte record built by a default ctor (callers of resize(n) at 0x477db0): four ints -10000, a zero int and
// byte, two 1.0f at +0x18/+0x1c; +0x20.. is left uninitialised.
struct E52 {
    int a, b, c, d; int e; char f; float g, h; int rest[5];
    E52() { a = -10000; b = -10000; c = -10000; d = -10000; f = 0; e = 0; g = 1.0f; h = 1.0f; }
};
// 0x64-byte record built by a default ctor (0x477ca0): eight ints -100000 at +0x28..+0x44, zeros at +0, +0x48,
// +0x5c and a zero byte at +0x60.
struct E100 {
    int f0; int p4[9]; int lim[8]; int f48; int p4c[4]; int f5c; char f60; char p61[3];
    E100() { f0 = 0; f5c = 0; f60 = 0; f48 = 0; for (int i = 0; i < 8; i++) lim[i] = -100000; }
};

struct ArrSet {
    tstring name;                                          // +0
    int cnt[5];                                            // +0x10
    int* arr[5];                                           // +0x24
    ArrSet(const ArrSet& o);                               // 0x4636d0
    ~ArrSet() { for (int i = 0; i < 5; i++) if (arr[i]) delete[] arr[i]; }       // 0x463760
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

// @ 0x477530
template std::vector<E24>::iterator std::vector<E24>::insert(std::vector<E24>::iterator, const E24&);
// @ 0x4775a0
template void std::vector<E100>::resize(size_t, E100);
// @ 0x477640
template std::vector<E100>::iterator std::vector<E100>::insert(std::vector<E100>::iterator, const E100&);
// @ 0x4776b0
template void std::vector<E52>::resize(size_t, E52);
// @ 0x477750
template std::vector<E52>::iterator std::vector<E52>::insert(std::vector<E52>::iterator, const E52&);
// @ 0x4777c0
template void std::vector<Rec1c8>::resize(size_t, Rec1c8);
// @ 0x477880
template void std::vector<ArrSet>::resize(size_t, ArrSet);

// @ 0x477b80
template void std::vector<E36>::push_back(const E36&);
// @ 0x477c10
template void std::vector<E24>::push_back(const E24&);
// @ 0x477ca0 resize(n) pads with E100()
template void std::vector<E100>::resize(size_t);
// @ 0x477d20
template void std::vector<E100>::push_back(const E100&);
// @ 0x477db0 resize(n) pads with E52()
template void std::vector<E52>::resize(size_t);
// @ 0x477e10
template void std::vector<E52>::push_back(const E52&);

// ---- 0x38-byte record: three words, two vector<int> (+0xc, +0x1c), three words (see s0043e514) -----------------
struct E56 {
    int a, b, c;
    std::vector<int> v1;       // +0x0c
    std::vector<int> v2;       // +0x1c
    int d, e, f;
};
// @ 0x477940 std::copy over E56 (operator= is memberwise: vector<int>::operator= 0x43f0a0 for the two vectors)
E56* __cdecl copy_e56(E56* first, E56* last, E56* dest)
{
    for (; first != last; ++first, ++dest)
        *dest = *first;
    return dest;
}

// ---- mesh: reverse the winding of every face (swap indices a and c) -----------------------------------------
namespace rgl {
// @ 0x4779b0
void mesh::flip_faces()
{
    if (!local_a0) {
        v90 = parent->v90;
        local_a0 = 1;
    }
    for (unsigned i = 0; i < v90.size(); i++) {
        int t = v90[i].a;
        v90[i].a = v90[i].c;
        v90[i].c = t;
    }
    slots[1].f28 = 1;
    slots[0].f28 = 1;
}
}

// ---- empty virtual slot of IfaceB (vtables 0x5908f8, 0x590b84 ...) ------------------------------------------
struct EmptySlot { virtual void slot(); };
// @ 0x477b70
void EmptySlot::slot() {}

// ---- RecZ: the 0x5928-byte IfaceB-derived record (vtable 0x591848); constructor ---------------------------------
struct tick { int lo, hi; };                    // 8-byte 100 kHz timer value (two dwords: no 8-byte alignment)
extern tick g_ticks[];                          // 0x5e2948 clock table; g_ticks[2] is "now"
struct Timed { int state; int pad; tick t; };               // 0x10: state (2 = idle) and the tick it was set
struct RecZSubA { void init(); };               // 0x475e50 (constructs the sub-object at +0x258)
struct RecZSubB { RecZSubB* init(); };          // 0x4353a0 (sub-object at +0x563c)
struct RecZ {
    virtual ~RecZ();                            // vtable 0x591848 (stored first by the constructor)
    virtual void slot1();
    virtual void slot2();
    int used, id, serial;
    char pad10[0x20 - 0x10];
    int f20, f24, f28;
    char pad2c[0x40 - 0x2c];
    Timed t40;                                  // +0x40 / +0x48
    char pad50[0x258 - 0x50];
    RecZSubA subA;                              // +0x258
    char pad259[0x563c - 0x259];
    RecZSubB subB;                              // +0x563c
    char pad563d[0x57c8 - 0x563d];
    Timed t57c8; char c57d8; char pad57d9[7];
    Timed t57e0; Timed t57f0; Timed t5800; Timed t5810; Timed t5820;
    char pad5830[0x5860 - 0x5830];
    int f5860; int pad5864; tick t5868;
    char pad5870[0x5918 - 0x5870];
    Timed t5918;
    RecZ();                                     // 0x477a30
};
// @ 0x477a30
RecZ::RecZ()
{
    f20 = 0; f24 = 0; f28 = 0;
    t40.state = 2; t40.t = g_ticks[2];
    subA.init();
    subB.init();
    t57c8.state = 2; t57c8.t = g_ticks[2]; c57d8 = 0;
    t57e0.state = 2; t57e0.t = g_ticks[2];
    t57f0.state = 2; t57f0.t = g_ticks[2];
    t5800.state = 2; t5800.t = g_ticks[2];
    t5810.state = 2; t5810.t = g_ticks[2];
    t5820.state = 2; t5820.t = g_ticks[2];
    f5860 = 0; t5868 = g_ticks[0];
    t5918.state = 2; t5918.t = g_ticks[2];
}
