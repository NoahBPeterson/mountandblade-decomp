// s004dbf7e: loading-screen configuration record helpers.
// Provisional names; every offset and call target comes from the disassembly.
// flags /O2 /MT /Gy /TP /GX
#include <new>
#include <string.h>
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (16 bytes): assignment first, so the copy ctor is the real 0x42c110 -------------
struct tstring {
    tstring& operator=(const tstring& o);       // 0x42bc50
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& s);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
};

// 16-byte container member (vector-like), each with an out-of-line copy ctor
struct R16 {
    void* a; void* b; void* c; void* d;
    R16();
    R16(const R16& o);          // per-field callee: 0x4cc750 / 0x4cc830 / 0x4cc680 / 0x4da300
};

// the configuration record: 0x1f8 bytes.  Field map read from the copy ctor 0x4dc030:
//   tstring +0x00, 6 ints +0x10..+0x27, tstring +0x28, 4 ints +0x38..+0x47,
//   R16 +0x48 (0x4cc750), +0x58, +0x68, R16 +0x78 (0x4cc830), int +0x88,
//   0x40 dwords +0x8c..+0x18b, 4 ints +0x18c..+0x19b,
//   R16 +0x19c (0x4cc680), R16 +0x1ac, R16 +0x1bc (0x4da300), 7 dwords +0x1cc, 4 ints +0x1e8.
struct loading_config {
    tstring s0;                 // +0x000  copy ctor 0x42c110
    int  f10a, f10b, f10c, f10d, f10e, f10f;    // +0x010
    tstring s1;                 // +0x028  copy ctor 0x42c110
    int  f38a, f38b, f38c, f38d;                // +0x038
    R16  r48;                   // +0x048  copy ctor 0x4cc750
    R16  r58;                   // +0x058  copy ctor 0x4cc750
    R16  r68;                   // +0x068  copy ctor 0x4cc750
    R16  r78;                   // +0x078  copy ctor 0x4cc830
    int  f88;                   // +0x088
    int  arr8c[0x40];           // +0x08c
    int  f18ca, f18cb, f18cc, f18cd;            // +0x18c
    R16  r19c;                  // +0x19c  copy ctor 0x4cc680
    R16  r1ac;                  // +0x1ac  copy ctor 0x4cc680
    R16  r1bc;                  // +0x1bc  copy ctor 0x4da300
    int  tail1cc[7];            // +0x1cc
    int  t1e8a, t1e8b, t1e8c, t1e8d;            // +0x1e8
    loading_config();
    // implicit copy ctor: cl emits the member-by-member copy at 0x4dc030
    loading_config& operator=(const loading_config& o);     // 0x4da410
    ~loading_config();
};

// std::vector<loading_config> (first at +4, last at +8, end at +0xc)
struct VecCfg {
    void* alloc;            // +0
    char* first;            // +4
    char* last;             // +8
    char* end;              // +0xc
    VecCfg& operator=(const VecCfg& o);                     // 0x4dc1c0
};

// ---- callees ---------------------------------------------------------------------------------------
void* __cdecl operator new(unsigned n);                     // 0x53545e
void  __cdecl operator delete(void* p);                     // 0x535712
void destroy_range_cfg(loading_config* first, loading_config* last, const loading_config* src);   // 0x4dc190
void erase_range_110(char* first, char* last, const void* src);                   // 0x4cc260

// vector<T> helpers with a 0x50-byte element (see 0x4dc1c0)
char* vec_copy(char* first, char* last, char* dst, const VecCfg* o);      // 0x4cd6e0
char* vec_fill(char* p, char* last, VecCfg* t, const VecCfg* o);          // 0x4d10f0
char* vec_fill_n(char* stop, char* last, char* dlast, VecCfg* t, const VecCfg* o);  // 0x4d11a0
unsigned __fastcall vec_capacity(VecCfg* v, int unused);                   // 0x4be950
void __fastcall vec_tidy(VecCfg* v, int unused);                           // 0x4da840
void __fastcall vec_destroy(VecCfg* v, int unused, char* first, char* last);  // 0x4d9370
bool __fastcall vec_buy(VecCfg* v, int unused, unsigned n);                // 0x4cbb90
char* __fastcall vec_construct(VecCfg* v, int unused, char* first, char* last, char* dst);  // 0x4d93e0

namespace rgl { struct vector2 { float x, y; vector2() {} vector2(float a, float b) : x(a), y(b) {} }; }

// @ 0x4dbf7e
void __fastcall insert_catch(char* block, int unused, int n, int count, char* last, const void* src)
{
    erase_range_110(block + n * 0xb0, last + count * 0xb0, src);
    throw;
}

// the implicit copy ctor (0x4dc030) is emitted out of line; this wrapper forces its emission
loading_config* copy_loading_config(loading_config* self, const loading_config* o)
{
    new (self) loading_config(*o);
    return self;
}

// @ 0x4dc190
void destroy_range_cfg(loading_config* first, loading_config* last, const loading_config* src)
{
    for (; first != last; first = (loading_config*)((char*)first + 0x1f8))
        *first = *src;
}

// @ 0x4dc1c0
// vector<T>::operator= for a 0x50-byte element type (VC7.1 <vector>)
VecCfg& VecCfg::operator=(const VecCfg& o)
{
    if (this == &o)
        return *this;
    if (o.first != 0) {
        int n = (o.last - o.first) / 0x50;
        if (n != 0) {
            int cap = first ? (last - first) / 0x50 : 0;
            if ((unsigned)n <= (unsigned)cap) {
                char* p = vec_copy(o.first, o.last, first, &o);
                vec_fill(p, last, this, &o);
                last = first + (o.first == 0 ? 0 : (o.last - o.first) / 0x50 * 0x50);
                return *this;
            }
            int cap2 = first ? (end - first) / 0x50 : 0;
            if ((unsigned)n <= (unsigned)cap2) {
                char* stop = vec_capacity(this, 0) * 0x50 + o.first;
                vec_copy(o.first, stop, first, &o);
                last = vec_fill_n(stop, (char*)o.last, last, this, &o);
                return *this;
            }
            if (first != 0) {
                vec_destroy(this, 0, first, last);
                operator delete(first);
            }
            unsigned cap3 = vec_capacity((VecCfg*)&o, 0);
            if (!vec_buy(this, 0, cap3))
                return *this;
            last = vec_construct(this, 0, o.first, (char*)o.last, first);
            return *this;
        }
    }
    vec_tidy(this, 0);
    return *this;
}

// +0x1024 holds four pointers to 16-float records
struct Rec { float x0, y0, x8, xc, x10, x14, x18, x1c; };
struct Frame {
    char pad0[0x1024];
    Rec** recs;                 // +0x1024
};

// @ 0x4dc350  (construction of the frame's coordinate records from two rgl::vector2s)
void __fastcall ctor_frame(Frame* self, int unused, const rgl::vector2* a, const rgl::vector2* b)
{
    float sx = a->x + b->x;
    float sy = a->y + b->y;
    Rec** t = self->recs;           // +0x1024: four records of 16 floats
    t[0]->x0 = b->x;  t[0]->y0 = b->y;  t[0]->x8 = 0.0f;  t[0]->xc = a->y;
    t[1]->x0 = sx;    t[1]->y0 = b->y;  t[1]->x8 = 0.0f;  t[1]->xc = a->y;
    t[2]->x0 = sx;    t[2]->y0 = sy;    t[2]->x8 = 0.0f;  t[2]->xc = a->y;
    t[3]->x0 = b->x;  t[3]->y0 = sy;    t[3]->x8 = 0.0f;  t[3]->xc = a->y;
    t[0]->x10 = b->x; t[0]->x14 = b->y; t[0]->x18 = 0.0f; t[0]->x1c = a->y;
    t[1]->x10 = sx;   t[1]->x14 = b->y; t[1]->x18 = 0.0f; t[1]->x1c = a->y;
    t[2]->x10 = sx;   t[2]->x14 = sy;   t[2]->x18 = 0.0f; t[2]->x1c = a->y;
    t[3]->x10 = b->x; t[3]->x14 = sy;   t[3]->x18 = 0.0f; t[3]->x1c = a->y;
}

// @ 0x4dc030 loading_config::loading_config

// @ 0x4dc1c0 VecCfg::operator4
