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

// ---- callees ---------------------------------------------------------------------------------------
void* __cdecl operator new(unsigned n);                     // 0x53545e
void  __cdecl operator delete(void* p);                     // 0x535712
void destroy_range_cfg(loading_config* first, loading_config* last, const loading_config* src);   // 0x4dc190
namespace rgl { struct vector2 { float x, y; vector2() {} vector2(float a, float b) : x(a), y(b) {} }; }

// 0x4dbf7e is the catch(...) funclet of std::vector<E176>::_Insert_n (0x4dbcc0, slice s004db660): it is part
// of that COMDAT (the cold tail after 0x4dbebf), so it has no source of its own here.

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

// ---- std::vector<E80>::operator= (0x4dc1c0) --------------------------------------------------------
#include <vector>
// 0x2c-byte-element vector header at +0x40 of the 0x50-byte record (copy ctor 0x4cc350)
struct V44 {
    int alloc;
    void* first; void* last; void* end;         // +4 / +8 / +0xc
    V44(const V44& o);                          // 0x4cc350
};

// 0x50-byte element: name, twelve words, a V44 at +0x40 (copy ctor 0x4cd080, assignment 0x4cd4f0); the
// destructor (inlined into the range destroyer 0x4d10f0) frees and clears the V44 block, then ~tstring.
struct E80 {
    tstring name;                               // +0
    int f10[12];                                // +0x10
    V44 v;                                      // +0x40
    E80(const E80& o);                          // 0x4cd080
    E80& operator=(const E80& o);               // 0x4cd4f0
    ~E80()
    {
        if (v.first)
            operator delete(v.first);
        v.first = 0; v.last = 0; v.end = 0;
    }
};

// @ 0x4dc1c0
template std::vector<E80>& std::vector<E80>::operator=(const std::vector<E80>&);

// ---- 0x4dc350: place the loading-screen frame (a quad mesh at +0xfb0 and four corner meshes) -----------
// The object is the UI root of s0042c9b0 / s0050f440 (mesh at +0xfb0, corner meshes at +0x1210..+0x121c).
// A mesh's position array (+0x74) holds 0x20-byte records (a vector4 pair); +0x2c is its material,
// +0x3c a colour/id word, +0x138 a dirty flag; +0xfc/+0x100 are the uv corner the frame code moves.
struct rec32 { rgl::vector4 p; rgl::vector4 q; };
struct FrameMesh {
    char pad0[0x2c];
    void* material;                             // +0x2c
    char pad30[0x3c - 0x30];
    int f3c;                                    // +0x3c
    char pad40[0x74 - 0x40];
    rec32* pos;                                 // +0x74
    char pad78[0xfc - 0x78];
    float uv_x;                                 // +0xfc
    float uv_y;                                 // +0x100
    char pad104[0x138 - 0x104];
    char dirty;                                 // +0x138
    char pad139[0x22c - 0x139];
    float c22c, c230;                           // +0x22c, +0x230
    char pad234[0x23c - 0x234];
    float c23c, c240;                           // +0x23c, +0x240
    char pad244[0x260 - 0x244];
    void ensure_local();                        // 0x412ee0
};
struct res_mgr {
    void* get_material(const tstring& name);    // 0x418a70 (ret 4)
};
extern res_mgr g_res;                           // 0x5d6fa0
struct FrameGui {
    char pad0[0xfb0];
    FrameMesh quad;                             // +0xfb0
    FrameMesh* corner[4];                       // +0x1210
    char pad1220[0x16e4 - 0x1220];
    int f16e4;                                  // +0x16e4
    void set_rect(const rgl::vector2* a, const rgl::vector2* b);    // 0x4dc350 (ret 8)
};

// @ 0x4dc350
void FrameGui::set_rect(const rgl::vector2* a, const rgl::vector2* b)
{
    float sy = b->y + a->y;
    float sx = a->x + b->x;
    quad.ensure_local();
    { rgl::vector4 v(a->x, a->y, 0.0f); rec32* r = quad.pos + 0; r->q = r->p = v; }
    { rgl::vector4 v(sx, a->y, 0.0f); rec32* r = quad.pos + 1; r->q = r->p = v; }
    { rgl::vector4 v(sx, sy, 0.0f); rec32* r = quad.pos + 2; r->q = r->p = v; }
    { rgl::vector4 v(a->x, sy, 0.0f); rec32* r = quad.pos + 3; r->q = r->p = v; }
    quad.f3c = f16e4;
    quad.dirty = 1;
    {
        tstring n("matte");
        quad.material = g_res.get_material(n);
    }
    corner[0]->uv_x = (a->x - corner[0]->c22c) - 0.003f;
    corner[0]->uv_y = (sy - corner[0]->c240) + 0.003f;
    corner[1]->uv_x = (sx - corner[0]->c23c) + 0.003f;
    corner[1]->uv_y = (sy - corner[0]->c240) + 0.003f;
    corner[2]->uv_x = (sx - corner[0]->c23c) + 0.003f;
    corner[2]->uv_y = (a->y - corner[0]->c230) - 0.003f;
    corner[3]->uv_x = (a->x - corner[0]->c22c) - 0.003f;
    corner[3]->uv_y = (a->y - corner[0]->c230) - 0.003f;
    corner[0]->f3c = -1;
    corner[1]->f3c = -1;
    corner[2]->f3c = -1;
    corner[3]->f3c = -1;
}

// @ 0x4dc030 loading_config::loading_config

