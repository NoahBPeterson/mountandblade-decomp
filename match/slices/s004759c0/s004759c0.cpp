// Slice s004759c0: Mission::update_team_counts (0x4759c0), VC7.1 <vector>/<algorithm> instantiations for the ArrSet record
// (0x38 bytes) and for the 8-byte sort record (stable_sort), the destructors of a record-set class pair (0x475c60 /
// 0x475d50), a Timer-initialiser (0x475e50) and vector<E36>/vector<E136>::reserve / insert. All names provisional.
#include <vector>
#include <list>
#include <algorithm>
#include <new>

extern __int64 g_ticks[];       // 0x5e2948: 64-bit counters, 100 kHz units

// ---- Mission (see s00474e60) ----------------------------------------------------------------------------------
struct Agent {
    char pad0[4];
    int used;                                           // +4
    char pad8[0x10 - 8];
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x1c4 - 0x18];
    int team;                                           // +0x1c4
    char pad1c8[0x5928 - 0x1c8];
};
struct AgentTable {
    int count;                                          // +0
    char pad4[0x14 - 4];
    std::vector<Agent*> pages;                          // +0x14
    void next_used(unsigned* i);                        // 0x43b160
    __forceinline unsigned first_used()
    {
        unsigned i = 0;
        if (0 < count) {
            do {
                if (pages[i >> 4][i & 0xf].used != 0) break;
                i++;
            } while ((int)i < count);
        }
        return i;
    }
};
struct Mission {
    int scene;                                          // +0
    AgentTable agents;                                  // +4
    char pad28[0x621c - 0x28];
    int counts[8][6];                                   // +0x621c
    float changed_at;                                   // +0x62dc
    void update_team_counts();                          // 0x4759c0 (fastcall)
};

// @ 0x4759c0
void Mission::update_team_counts()
{
    int old[8][6];
    int* dst = &old[0][0];
    int* src = &counts[0][0];
    for (int t = 8; t != 0; t--) {
        for (int j = 6; j != 0; j--) {
            *dst = *src;
            *src = 0;
            dst++;
            src++;
        }
    }
    unsigned i = agents.first_used();
    while (i < (unsigned)agents.count) {
        Agent& a = agents.pages[i >> 4][i & 0xf];
        if (a.f10 == 0 && a.team >= 0 && a.team < 8)
            counts[a.team][a.f14]++;
        agents.next_used(&i);
    }
    for (int t = 0; t < 8; t++) {
        for (int j = 0; j < 6; j++)
            if (old[t][j] != counts[t][j]) changed_at = (float)g_ticks[2] * 1e-5f;
    }
}

// ---- vector<ArrSet> helpers (ArrSet: see s004724f0) ------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring() {}
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    void operator=(const tstring& o);           // 0x42bc50
};
struct ArrSet {                                 // 0x38 bytes
    tstring name;                               // +0
    int cnt[5];                                 // +0x10
    int* arr[5];                                // +0x24
    ~ArrSet();                                  // 0x463760
};
struct ptr_tag {};          // std::_Nonscalar_ptr_iterator_tag: an UNINITIALISED local pushes whatever EAX holds
struct alloc_arrset {
    void construct(ArrSet* p, const ArrSet& v);     // 0x46b6c0
    void destroy(ArrSet* p) { p->~ArrSet(); }
};
void __cdecl destroy_range_arrset(ArrSet* first, ArrSet* last, alloc_arrset& al, ptr_tag tag);     // 0x4727a0
void __cdecl uninit_fill_n_arrset(ArrSet* first, unsigned n, const ArrSet& val, alloc_arrset& al, ptr_tag tag);   // 0x4727e0
ArrSet* __cdecl copy_arrset(ArrSet* f, ArrSet* l, ArrSet* d, ptr_tag tag);       // 0x468050

struct vecArrS : alloc_arrset {
    ArrSet* first; ArrSet* last; ArrSet* end;
    void destroy(ArrSet* f, ArrSet* l);                 // 0x475b60
    ArrSet* ufill(ArrSet* f, unsigned n, const ArrSet& v);   // 0x475b80
};
// @ 0x475b60
void vecArrS::destroy(ArrSet* f, ArrSet* l)
{
    ptr_tag t;
    destroy_range_arrset(f, l, *this, t);
}
// @ 0x475b80
ArrSet* vecArrS::ufill(ArrSet* f, unsigned n, const ArrSet& v)
{
    ptr_tag t;
    uninit_fill_n_arrset(f, n, v, *this, t);
    return f + n;
}
namespace std {
template<> void _Destroy_range<ArrSet, allocator<ArrSet> >(ArrSet*, ArrSet*, allocator<ArrSet>&, _Nonscalar_ptr_iterator_tag);   // 0x4727a0
template<> ArrSet* _Copy_opt<ArrSet*, ArrSet*>(ArrSet*, ArrSet*, ArrSet*, _Nonscalar_ptr_iterator_tag);                         // 0x468050
}
// @ 0x475f20
template std::vector<ArrSet>::iterator std::vector<ArrSet>::erase(std::vector<ArrSet>::iterator, std::vector<ArrSet>::iterator);

// ---- std::stable_sort pieces for 8-byte records (see s004724f0 / s00473a60 / s00474410) --------------------------------
struct P8 { int key, val; };
typedef bool (__cdecl *Pred8)(const P8&, const P8&);
typedef std::vector<P8>::iterator It8;
namespace std {
template<> void _Insertion_sort<It8, Pred8>(It8, It8, Pred8);                                     // 0x474370
template<> void _Chunked_merge<It8, _Temp_iterator<P8>, int, Pred8>(It8, It8, _Temp_iterator<P8>, int, int, Pred8);   // 0x472870
template<> void _Chunked_merge<P8*, It8, int, Pred8>(P8*, P8*, It8, int, int, Pred8);                // 0x474650
template<> void _Buffered_merge<It8, int, P8, Pred8>(It8, It8, It8, int, int, _Temp_iterator<P8>&, Pred8);   // 0x474410
}
// @ 0x475bb0 (_Buffered_merge_sort)
template void std::_Buffered_merge_sort(It8, It8, int, std::_Temp_iterator<P8>&, Pred8);
// @ 0x475f70 (_Stable_sort, recursive)
template void std::_Stable_sort(It8, It8, int, std::_Temp_iterator<P8>&, Pred8);

// ---- record-set classes (0x475c60 base destructor, 0x475d50 derived destructor) -------------------------------------
struct RecP {                   // 0x34-byte record: vector<int> at +0, list<int> at +0x10, vector<int> at +0x20 (see s00431960)
    std::vector<int> a;
    std::list<int> l;
    int pad1c;
    std::vector<int> b;
    int tail;
    ~RecP();                    // 0x431ab0
};
RecP::~RecP() {}
// std::list<int*> whose destructor first frees the pointed-to arrays (0x472460), then the usual node sweep.
struct PList : std::list<int*> {
    int pad0c, x10;
    void free_values();         // 0x472460
    ~PList() { free_values(); }
};
struct RecSetBase {
    char pad0[8];
    int n;                      // +8
    char pad0c[0x28 - 0xc];
    RecP** arr;                 // +0x28
    PList l2c;                  // +0x2c
    PList l40;                  // +0x40
    std::vector<int> v54;       // +0x54
    ~RecSetBase();              // 0x475c60
};
// @ 0x475c60
__declspec(noinline) RecSetBase::~RecSetBase()
{
    for (int i = 0; i < n; i++)
        if (arr[i])
            delete[] arr[i];
    delete[] arr;
    l2c.free_values();
    l40.free_values();
}
struct RecSet : RecSetBase {
    char pad64[0x7c - 0x64];
    std::vector<int> v7c;
    std::vector<int> v8c;
    char pad9c[4];
    std::vector<int> va0;
    char padb0[8];
    std::vector<int> vb8;
    std::vector<int> vc8;
    char padd8[0x32c - 0xd8];
    std::vector<int> v32c;
    void cleanup();             // 0x421390
    ~RecSet();                  // 0x475d50
};
// @ 0x475d50
RecSet::~RecSet()
{
    cleanup();
}

// ---- 0x475e50: arm five timers on clock 2 and clear some state --------------------------------------------------
struct Timer16 {
    int clock;                  // +0 index into g_ticks
    int pad4;
    __int64 start;              // +8
    void init(int c) { clock = c; start = g_ticks[clock]; }
};
struct Vec2 { float x, y; };
struct Vec4z { float x, y, z, w; };
struct TimedObj {
    char pad0[0x58];
    Timer16 t58;                // +0x58 (the state int at +0x58 is the clock id, the tick at +0x60)
    char pad68[0x98 - 0x68];
    Timer16 t98;                // +0x98
    Timer16 ta8;                // +0xa8
    char padb8[0x50e8 - 0xb8];
    Vec2 v50e8;                 // +0x50e8
    char pad50f0[0x5118 - 0x50f0];
    Timer16 t5118;              // +0x5118
    Timer16 t5128;              // +0x5128
    char pad5138[0x516c - 0x5138];
    Vec4z v516c;                // +0x516c
    TimedObj();                 // 0x475e50 (constructor: returns this)
};
// @ 0x475e50
TimedObj::TimedObj()
{
    t58.init(2);
    t98.init(2);
    ta8.init(2);
    t5118.init(2);
    t5128.init(2);
    v516c.x = 0.0f; v516c.y = 0.0f; v516c.z = 0.0f; v516c.w = 0.0f;
    Vec2 z; z.x = 0.0f; z.y = 0.0f;
    v50e8 = z;
}

// ---- vector<E36>/vector<E136>/vector<E1c8> reserve / insert / _Buy: see s004759c0_stl.cpp (/EHa TU) ----
