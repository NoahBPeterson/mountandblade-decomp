// agent_ai.h: the classes only agent_ai.cpp (0x43b100..0x43e190) needs beyond agent_types.h: the 0x1f8-byte record
// table, the int-keyed hash map whose lookup lives here and the two-vector record destroyed at the end.
// Offsets are the ones read by a function of this module (the address is cited). Names provisional.
#ifndef AGENT2_AGENT_AI_H
#define AGENT2_AGENT_AI_H
#include <hash_map>
#include "./agent_types.h"

// ---- tables read by the target pickers ------------------------------------------------------------------------
struct Rec1f8 { char pad[0x1f0]; int f1f0; int f1f4; };         // 0x1f8-byte records
struct Table1f8 {
    char pad0[0x18];
    std::vector<Rec1f8> recs;                           // +0x18 (first at +0x1c)
    unsigned count;                                     // +0x28
    int get_f1f0(unsigned i);                           // 0x43b100
    int get_f1f4(unsigned i);                           // 0x43b130
};
// the int-keyed hash map the 0x43b210 lookup belongs to: stdext::hash_map<int, HashVal> (VC7.1 <hash_map>); the body
// is the library's own _Hash::lower_bound, instantiated below (byte-exact with the library source)
struct HashVal { int v; };
typedef stdext::_Hash<stdext::_Hmap_traits<int, HashVal, stdext::hash_compare<int, std::less<int> >,
                                          std::allocator<std::pair<const int, HashVal> >, false> > IntHashBase;

struct VecPtr3 {                                         // (first, last, end) of an engine vector
    void *first, *last, *end;
    void tidy()
    {
        if (first != 0)
            operator delete(first);
        first = 0; last = 0; end = 0;
    }
};
struct VecPair2 {                                        // two vectors at +0x10 and +0x20 (the E56 record's v1, v2)
    char pad0[0x10];
    VecPtr3 a;
    char pad1c[4];
    VecPtr3 b;
};


void deque_xlen();                                                                                               // 0x43e0e0
void __stdcall tidy_vec_pair(VecPair2* p);                                                                       // 0x43e150

#endif // AGENT2_AGENT_AI_H
