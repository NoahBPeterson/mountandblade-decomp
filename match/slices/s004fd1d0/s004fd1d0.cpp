// Slice s004fd1d0: the map-position list widget (class MWidget below; vtable 0x5995c0, base dtor 0x487c00,
// base size 0x1d0).  The four derived members work on the list of map spots (DX/DY rectangles at +0x17c /
// +0x18c, ids at +0x1bc):
//   0x4fd1d0  on_event: on a "no flags" event (ev & 7 == 0) pick the spot under the click (+0xe0/+0xe4) and
//             notify v+0x94 if it changed; on ev & 8 pick it and set the key flags 0x10000.
//   0x4fd3b0  scalar deleting destructor
//   0x4fd3d0  ~MWidget: clear the four lists (0x17c, 0x18c, 0x198 mface vec, 0x1ac, 0x1bc), then the base dtor
//   0x4fd490  vec<E4>::buy(n)  (element 4 bytes, _Xlen 0x43e2a0)
//   0x4fd4f0  text_rect(): lazily create the "ui_font" mesh, then lay the rows out through 0x4fd700
//             (two arms depending on +0x124, with swapped argument order; builds the 0x590e60 / 0x549cdc
//             constants and the -1 locale index).  The call is reproduced through a function pointer because
//             the callee is an __usercall (this in EDI, result in EBP) in this TU.
// The float constants 0x5d7094/0x5d7098 are read from the binary image.
#include <windows.h>
#include <new>
#include <string.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- shared engine types (see s0040d7d0, s00482240, s0049aca0, s004dbf7e) --------------------------------
namespace rgl {
struct mesh {
    char pad0[0x2c];
    void* res;                                      // +0x2c
    char pad30[0x260 - 0x30];
    mesh();                                         // 0x412a90
    void clear_geometry();                          // 0x428e80
};
}
using rgl::mesh;

struct tstring {                                    // 0x10-byte engine string
    int pos;                                        // +0
    char* buf;                                      // +4
    int cap;                                        // +8
    int len;                                        // +0xc
    tstring();                                      // 0x42bbd0
    tstring(const char* s);                         // 0x42bbe0
    ~tstring();                                     // 0x42bc40
};
struct resmgr { void* get_material(const tstring* name); };         // 0x418a70 (ret 4)
extern resmgr g_resmgr;                             // 0x5d6fa0

// ---- vec<T>: one 4-byte element, _Myfirst at +4 (see s0040d7d0) ------------------------------------------
typedef int E100;
struct PVec {
    void* f0;                                       // +0
    E100* first;                                    // +4
    E100* last;                                     // +8
    E100* end;                                      // +0xc
};
void vector_E100_buy(PVec* v, unsigned n);          // 0x4fd490

struct mface {                                      // 0x20 bytes (s0040d7d0)
    int a, flags, n, idx[4], m4;
};
struct MFaceVec {                                   // _Myfirst at +4
    void* f0;
    mface* first;
    mface* last;
    mface* end;
    void clear();                                   // 0x41f4b0
};

struct Region { float x0, y0, x1, y1; };            // one map spot's screen rectangle (stride 0x10)

// the map-spot table renderer (0x4fd700, other TU).  At the call site it is entered with `this` (the font
// singleton) in ECX, 16 stack arguments and an 8-byte by-value result handed back through a hidden pointer
// pushed last; the original keeps the callee's own frame pointer in EBP and returns the result block in EAX.
// Because the true shape is an __usercall, it is modelled here as a member of a Font stub returning MapText
// by value (cl then supplies the same hidden-pointer + stack argument layout).
struct MWidget;
struct MapText { int a; float w; };                 // the 8-byte layout result
struct Font {
    // 0x4fd700: __usercall -- `this` in ECX, 17 stack arguments (the first is the caller's out block), the
    // same pointer handed back in EAX.  Modelled as a member returning the out pointer.
    MapText* layout(void* out, const void* p2, const void* p3, const void* p4, mesh* p5, const void* p6,
                    const void* p7, const void* p8, const void* p9, const void* p10, int p11, int p12,
                    const void* p13, const void* p14, const void* p15, const void* p16, const void* p17);
};
Font* g_font;                                       // 0x5e2934
typedef int* intp;
void _Xlen_E4();                                    // 0x43e2a0 (vector<E4>::_Xlen)
struct BufBase { void dtor(); };                    // 0x487c00 (thiscall, ret 0, tail-called)
void __cdecl operator delete(void* p);              // 0x535712
void _Xlen_thiscall(void* self);                    // 0x43e2a0 (vector<E100>::_Xlen: no `this` use)

// ---- list widget (vtable 0x5995c0) ----------------------------------------------------------------------
struct MWidget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17();
    virtual void set_key_flags(int f);          // +0x48 (slot 18)
    virtual void v19(); virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27(); virtual void v28();
    virtual void v29(); virtual void v30();
    virtual void v37(int a, mesh* m);           // +0x7c (slot 31)
    virtual void v38(); virtual void v39(); virtual void v40(); virtual void v41();
    virtual void refresh();                     // +0x90 (slot 36)
    virtual void v43();                         // +0x94 (slot 37)
    virtual void v44(void* p);                  // +0x98 (slot 38)

    char pad04[0xd8 - 4];
    float dx8, dxc;                             // +0xd8, +0xdc
    float de0;                                  // +0xe0 (click x)
    float de4;                                  // +0xe4 (click y)
    char pade8[0x180 - 0xe8];
    int f180, f184, f188;                       // +0x180 (vector, stride 0x10)
    char pad18c[0x198 - 0x18c];
    MFaceVec v19c;                              // +0x198 (mface vector, first at +0x19c)
    char pad1a8[0x1ac - 0x1a8];
    E100* ids_first;                            // +0x1ac (vector, 4-byte elements)
    E100* ids_last;                             // +0x1b0
    E100* ids_end;                              // +0x1b4
    char pad1b8[0x1bc - 0x1b8];
    E100* r1bc_first;                           // +0x1bc (vector, stride 0x10)
    E100* r1bc_last;                            // +0x1c0
    E100* r1bc_end;                             // +0x1c4
    int cur;                                    // +0x1c8 (selected spot)
    int sel;                                    // +0x1cc
    char pad1d0[0x1d0 - 0x1d0];

    char on_event(unsigned ev);                 // 0x4fd1d0
    void* destroy(unsigned flags);              // 0x4fd3b0
    ~MWidget();                                 // 0x4fd3d0
    MapText text_rect();                        // 0x4fd4f0
};

extern const float g_dy_zero;                       // 0x5d7094
extern const float g_dy_zero2;                      // 0x5d7098
static const unsigned g_col_y = 0xffff0000u;
static const unsigned g_col_z = 0xff0000ffu;
static const unsigned g_col_w = 0xff000000u;

// @ 0x4fd1d0
char MWidget::on_event(unsigned ev)
{
    char r = 0;
    if ((ev & 7) != 0) {
        int id = cur;
        cur = -1;
        E100* ids = r1bc_first;
        Region* a = *(Region**)((char*)this + 0x17c);
        unsigned i = 0, off = 0;
        if (ids != 0) {
            while (i < (unsigned)(r1bc_last - ids) / 4) {
                if (a->x0 + de0 <= g_dy_zero)
                    if (g_dy_zero < de0 + *(float*)((char*)a + 0x10 - off + off))
                        if (a->y0 + de4 <= g_dy_zero2)
                            if (g_dy_zero2 < *(float*)((char*)a + 0x14))
                                break;
                i++;
                off += 0x10;
                a = (Region*)((char*)a + 0x10);
            }
        }
        cur = ids[i];
        if (cur != id)
            v44(&id);
    } else if ((ev & 8) != 0) {
        Region* a = *(Region**)((char*)this + 0x17c);
        E100* ids = r1bc_first;
        r = 1;
        unsigned i = 0, off = 0;
        for (; ids != 0 && i < (unsigned)(r1bc_last - ids) / 4; i++, off += 0x10) {
            if (a->x0 + de0 <= g_dy_zero)
                if (g_dy_zero < *(float*)((char*)this + 0x18c + off + 0x10) + de0)
                    if (a->y0 + de4 <= g_dy_zero2)
                        if (g_dy_zero2 < *(float*)((char*)this + 0x18c + off + 0x14) + de4)
                            break;
            a = (Region*)((char*)a + 0x10);
        }
        set_key_flags(0x10000);
        sel = i;
    }
    set_key_flags(ev);
    refresh();
    return r;
}
// @ 0x4fd3b0
void* MWidget::destroy(unsigned flags)
{
    this->~MWidget();
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x4fd3d0
MWidget::~MWidget()
{
    typedef std::vector<int> IV;
    ((IV*)((char*)this + 0x1b8))->~IV();
    ((IV*)((char*)this + 0x1a8))->~IV();
    v19c.clear();
    ((IV*)((char*)this + 0x188))->~IV();
    ((IV*)((char*)this + 0x178))->~IV();
    ((BufBase*)this)->dtor();
}

// @ 0x4fd490  (vector<E100> where E100 is an empty 4-byte class: buy(n) is thiscall on the allocator,
// with the vector itself in ecx -- the callee never reads ecx)
void vector_E100_buy(PVec* v, unsigned n)
{
    v->first = 0;
    v->last = 0;
    v->end = 0;
    if (n == 0)
        return;
    if (n > 0x3fffffff) {
        _Xlen_thiscall(v);
        return;
    }
    E100* p = (E100*)operator new(n * 4);
    v->end = p + n;
    v->first = p;
    v->last = p;
}

// @ 0x4fd4f0
MapText MWidget::text_rect()
{
    if (*(mesh**)((char*)this + 0x118) == 0) {
        mesh* m = new mesh();
        tstring n("ui_font");
        m->res = g_resmgr.get_material(&n);
        v37(4, m);
    }
    mesh* m = *(mesh**)((char*)this + 0x118);
    m->clear_geometry();

    // two 4-float blocks handed to the renderer: the source rect and the packed colour
    MapText out;
    vector4 rc;
    rc.x = *(float*)((char*)this + 0x138);
    rc.y = *(float*)((char*)this + 0x13c);
    rc.z = 0.0f;
    rc.w = 0.0f;
    vector4 col;
    col.x = 0.0f;
    col.y = *(float*)&g_col_y;
    col.z = *(float*)&g_col_z;
    col.w = *(float*)&g_col_w;
    int zero = 0;

    // the arms differ only in the renderer object (`this` in ECX): the row's own font override, or the global
    Font* f = *(Font**)((char*)this + 0x124);
    if (f == 0)
        f = g_font;
    f->layout(&out, (char*)this + 0x128, &rc, (char*)this + 0x140, m, &zero, &col, &rc,
              (char*)this + 0xf8, (char*)this + 0xf0, *(int*)((char*)this + 0x148),
              *(int*)((char*)this + 0x1c8), (char*)this + 0x178, (char*)this + 0x188,
              (char*)this + 0x1b8, (char*)this + 0x198, (char*)this + 0x1a8);

    int idx = -1;
    v44(&idx);
    return out;
}
