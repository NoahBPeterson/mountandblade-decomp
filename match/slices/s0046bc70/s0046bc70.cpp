// Slice s0046bc70: terrain river/flow-path generator (0x46bc70: picks a start point by sampling noise, an edge end
// point far enough away, traces the path and spreads the flow), std::list<int>::_Buynode, VC7.1 <vector>/<memory>
// helpers (_Ucopy thunks, _Uninit_copy of 0x38-byte records, copy / copy_backward / _Lower_bound / _Upper_bound
// helpers over 8-byte records). Names provisional.
#include <stdlib.h>
#include "s0046bc70.h"

#define RAND01() ((float)(rand() % 15817) * 6.3223117e-05f)

// @ 0x46bc70
void terrain_obj::make_river()
{
    int a[2];
    int b[2];
    int steps;
    float minz;
    a[1] = 0;
    bool ok = false;
    if (flag843bd75) {
        int tries = 0;
        int bx = 0;
        do {
            if (3 < tries) break;
            int bestx = 0, besty = 0;
            float bestv = -10000.0f;
            int k = 20;
            do {
                int px, py;
                if (RAND01() < 0.5f) {
                    px = rand() % 4 + 3;
                    if (RAND01() < 0.5f) px = w - px;
                    int t = h - 3;
                    if (t < 2) py = 3;
                    else py = rand() % t + 3;
                } else {
                    int t = w - 3;
                    if (t < 2) px = 3;
                    else px = rand() % t + 3;
                    py = rand() % 4 + 3;
                    if (RAND01() < 0.5f) py = h - py;
                }
                float v = noise_at(bx, a[1]);
                if (bestv < v) {
                    bestv = v;
                    bestx = px;
                    besty = py;
                }
            } while (--k != 0);
            a[0] = bestx;
            a[1] = besty;
            int n = 0;
            bool far = false;
            do {
                if (far) goto trace;
                int ex, ey;
                if (RAND01() < 0.5f) {
                    ex = 0;
                    b[0] = 0;
                    if (RAND01() < 0.5f) {
                        ex = w + 1;
                        b[0] = ex;
                    }
                    int t = h + 1;
                    if (t < 2) {
                        ey = 0;
                        b[1] = 0;
                    } else {
                        ey = rand() % t;
                        b[1] = ey;
                    }
                } else {
                    ey = 0;
                    b[1] = 0;
                    if (RAND01() < 0.5f) {
                        ey = h + 1;
                        b[1] = ey;
                    }
                    int t = w + 1;
                    if (t < 2) {
                        ex = 0;
                        b[0] = 0;
                    } else {
                        ex = rand() % t;
                        b[0] = ex;
                    }
                }
                ex = ex - bestx;
                if (ex < 0) ex = -ex;
                ey = ey - besty;
                if (ey < 0) ey = -ey;
                if ((float)(w + h) * 0.7f < (float)(ey + ex)) far = true;
                n++;
            } while (n < 50);
            if (far) {
            trace:
                trace_path(a, b, 2, &steps, &minz, 1.5f, 0.9f, 0, 1);
                if (minz < 3.5f) ok = true;
                else clear_flags(g_mask_a | g_mask_b);
                tries++;
            }
            bx = a[0];
        } while (!ok);
    }
    int r = rand();
    n284 = r % 3;
    if (2.1f < spawn_scale && 1 < r % 3) n284 = 1;
    if (3.1f < spawn_scale) n284 = 0;
    if ((unsigned char)(g_pow2_low - 1) & (unsigned char)((unsigned)f4 >> 31)) n284 = 0;
    if (ok) {
        unsigned mb = g_mask_b;
        unsigned both = g_mask_a | mb;
        spread_flow(mb, 1, both);
        for (int i = 0; i < n284; i++)
            spread_flow(mb, 2, both);
        unsigned ma = g_mask_a;
        spread_flow(mb, 2, ma);
        spread_flow(ma, 2, ma);
        float v = -1.3f;
        if ((unsigned char)(g_pow2_low - 1) & (unsigned char)((unsigned)f4 >> 31)) v = -2.3f;
        set_cells(2, v);
        smooth_cells(2, 0.5f);
    }
}

// ---- 0x46c0c0: std::list<T>::_Buynode() for a 4-byte value type (12-byte node, head links to itself) ---------------
// (the 0x46c12f funclet frees the node and rethrows). Comes out of constructing a list<int>.
#include <list>
struct IntList { std::list<int> l; };
IntList* make_int_list() { return new IntList; }

// ---- std::stable_sort over a vector of 8-byte records: 0x46c260 copy<..., _Temp_iterator>, 0x46c2c0 merge, ------------
// ---- 0x46c3f0 copy, 0x46c420 copy_backward, 0x46c450 _Lower_bound, 0x46c4a0 _Upper_bound -------------------------------
#include <vector>
#include <algorithm>
struct E8 { int a, b; };
typedef bool (*Pr8)(const E8&, const E8&);
void sort_e8(std::vector<E8>& v, Pr8 pred) { std::stable_sort(v.begin(), v.end(), pred); }

// ---- _Ucopy thunks: vector<T>::_Ucopy(first, last, dest) -> _Uninit_copy(first, last, dest, _Alval, tag) -----------------
struct rec36 { int a[9]; };
struct elem24 { int a[6]; };
struct particle { int a[25]; };
struct elem52 { int a[13]; };
struct alloc36 { };
struct alloc24 { };
struct alloc100 { };
struct alloc52 { };
typedef std::_Nonscalar_ptr_iterator_tag nonscalar_tag;
rec36* __cdecl uninit_copy36(rec36*, rec36*, rec36*, alloc36&, nonscalar_tag);           // 0x46b6e0
elem24* __cdecl uninit_copy24(elem24*, elem24*, elem24*, alloc24&, nonscalar_tag);       // 0x4681b0
particle* __cdecl uninit_copy100(particle*, particle*, particle*, alloc100&, nonscalar_tag);   // 0x468220
elem52* __cdecl uninit_copy52(elem52*, elem52*, elem52*, alloc52&, nonscalar_tag);       // 0x468290

struct vec36 { alloc36 _Alval; rec36* _Ucopy(rec36* f, rec36* l, rec36* d); };
struct vec24 { alloc24 _Alval; elem24* _Ucopy(elem24* f, elem24* l, elem24* d); };
struct vec100 { alloc100 _Alval; particle* _Ucopy(particle* f, particle* l, particle* d); };
struct vec52 { alloc52 _Alval; elem52* _Ucopy(elem52* f, elem52* l, elem52* d); };
// @ 0x46c150
rec36* vec36::_Ucopy(rec36* f, rec36* l, rec36* d) { return uninit_copy36(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c170
elem24* vec24::_Ucopy(elem24* f, elem24* l, elem24* d) { return uninit_copy24(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c190
particle* vec100::_Ucopy(particle* f, particle* l, particle* d) { return uninit_copy100(f, l, d, _Alval, std::_Ptr_cat(f, d)); }
// @ 0x46c1b0
elem52* vec52::_Ucopy(elem52* f, elem52* l, elem52* d) { return uninit_copy52(f, l, d, _Alval, std::_Ptr_cat(f, d)); }

// ---- 0x46c1d0: _Uninit_copy of 0x38-byte ArrSet records (name + five count/array pairs) ---------------------------
#include "tstring.h"
struct ArrSet {
    tstring name;
    int cnt[5];
    int* arr[5];
    ArrSet(const ArrSet& o);                               // 0x4636d0
    ~ArrSet();                                             // 0x463760
};
struct allocArr { void construct(ArrSet* p, const ArrSet& v); };       // 0x46b6c0
// @ 0x46c1d0 (+ catch(...) funclet 0x46c23a: ~ArrSet over the constructed prefix, then rethrow)
ArrSet* __cdecl uninit_copy_arrset(ArrSet* first, ArrSet* last, ArrSet* dest, allocArr& al)
{
    ArrSet* next = dest;
    try {
        for (; first != last; ++dest, ++first) al.construct(dest, *first);
    } catch (...) {
        for (; next != dest; ++next) next->~ArrSet();
        throw;
    }
    return dest;
}
