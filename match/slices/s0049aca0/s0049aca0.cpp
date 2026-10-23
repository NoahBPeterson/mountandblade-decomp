// Slice s0049aca0: a mission-UI "text list" panel.  The panel (`this` of 0x49aca0/0x49ad20/0x49b4f0, a
// MissionUi-derived widget) owns a container object (vptr 0x592848, base ctor 0x4931a0) that it builds in
// create() and a std::vector<E100> of text rows; create() also builds one flag-widget child per row (UiA,
// vptr 0x591d58, base ctor 0x48ae20).  Also here: the container's scalar deleting destructor, the vector
// fast path / insert / _Insert_n, a Mission::add_placed trampoline and the text-rect layout wrapper.
// Names and field names are provisional; every offset is read off the disassembly.
#include <new>
#include <string.h>
#include "rgl.h"

using rgl::vector4;

// ---- shared engine types -------------------------------------------------------------------------
namespace rgl {
struct mesh {
    char pad[0x260];
    mesh();                                     // 0x412a90
    mesh* clone();                              // 0x41c6d0
    void clear_geometry();                      // 0x428e80
};
}
using rgl::mesh;

struct mstr {                                   // 0x10-byte tstring
    char* p; int size, cap, extra;
    mstr(const char* s);                        // 0x42bbe0
    ~mstr();                                    // 0x42bc40
};
struct resmgr { void* lookup(const mstr& name); };                  // 0x418a70
struct Font {                                   // see s004520d0
    void layout_text(float* out, const mstr* text, const vector4* pos_in, const float* scale_in, void* m,
                     int arg, const float* xmin, const float* xmax, unsigned flags);   // 0x4520d0
};
extern Font* g_font;                            // 0x5e2934
struct Fadeable { void fade_scale(vector4 v); };                    // 0x47af30

struct R8 { int a, b; };                        // an 8-byte returned rect (2 floats)
struct E16 { int a, b, c, d; };                 // one 16-byte text-row record

// ---- external callees ----------------------------------------------------------------------------
void __fastcall base_ctor_4931a0(void* o);      // 0x4931a0 (ecx = object)
void __fastcall base_ctor_48ae20(void* o);      // 0x48ae20 (ecx = object)
void __fastcall base_dtor_490520(void* o);      // 0x490520 (ecx = object)
void __fastcall play_sound_48b2d0(void* m);     // 0x48b2d0
extern "C" void __stdcall __CxxThrowException(void*, void*);        // 0x5368e6
extern void* g_5e2930;                          // 0x5e2930 (game-data pointer, matrix arena at +0x14014)
struct Mission { int add_placed(const int* kind, const int* id, const int* value, const void* m, int* spawn_id); }; // 0x44f870

struct NodeA;
struct UiA;
struct PObj;

// ---- std::vector<E100> (4-byte element, `_Myfirst` at +4) -----------------------------------------
typedef NodeA* E4;
struct PVec {
    void* f0;                                   // +0
    E4* first;                                  // +4
    E4* last;                                   // +8
    E4* end;                                    // +0xc
    __declspec(noinline) void push_back(const E4& x);   // 0x49b120
    void insert(E4** out, E4* where, const E4& x);      // 0x49b170
    void _Insert_n(E4* where, unsigned count, const E4& x);  // 0x49b1c0
    void _Xlen() const;                         // 0x43e2a0
};

// ---- class A: the panel (`this` of 0x49aca0 / 0x49ad20 / 0x49b4f0) ---------------------------------
struct PObj {
    virtual void p00(); virtual void p01(); virtual void p02(); virtual void p03(); virtual void p04();
    virtual void p05(); virtual void p06(); virtual void p07(); virtual void p08(); virtual void p09();
    virtual void p10(); virtual void p11(); virtual void p12(); virtual void p13(); virtual void p14();
    virtual void p15(); virtual void p16(); virtual void p17();
    virtual void set_key_flags(int f);          // +0x48
    virtual void p19(); virtual void p20(); virtual void p21(); virtual void p22(); virtual void p23();
    virtual void p24(); virtual void p25(); virtual void p26(); virtual void p27(); virtual void p28();
    virtual void p29(); virtual void p30();
    virtual void v31(int a, void* b);           // +0x7c
    virtual void p32(); virtual void p33(); virtual void p34(); virtual void p35();
    virtual void refresh();                     // +0x90
    virtual void p37();
    virtual void v38(void* p);                  // +0x98

    char pad0[0xd8 - 4];                        // +0x04
    float dx8;                                  // +0xd8
    float dxc;                                  // +0xdc
    float de0;                                  // +0xe0
    float de4;                                  // +0xe4
    char pade8[0xf0 - 0xe8];
    float vf0[4];                               // +0xf0  (vf0[0] = xmax, vf0[2] = xmin)
    char pad100[0x118 - 0x100];
    mesh* m118;                                 // +0x118
    char pad11c[0x122 - 0x11c];
    char f122;                                  // +0x122
    char pad123[0x124 - 0x123];
    int f124;                                   // +0x124
    mstr name;                                  // +0x128
    float f138;                                 // +0x138
    float f13c;                                 // +0x13c
    float v140[2];                              // +0x140
    unsigned f148;                              // +0x148
    int f14c;                                   // +0x14c
    char pad150[0x180 - 0x150];
    char f180;                                  // +0x180
    char pad181[0x188 - 0x181];
    NodeA* f188;                                // +0x188
    char pad18c[0x190 - 0x18c];
    E16* v190_first;                            // +0x190  (std::vector of E16 rows)
    E16* v190_last;                             // +0x194
    E16* v190_end;                              // +0x198
    char pad19c[0x1a0 - 0x19c];
    mesh* f1a0;                                 // +0x1a0
    mesh* f1a4;                                 // +0x1a4
    mesh* f1a8;                                 // +0x1a8
    int f1ac;                                   // +0x1ac (never read here)
    float f1b0, f1b4, f1b8;                     // +0x1b0
    float f1bc, f1c0, f1c4, f1c8;               // +0x1bc

    char on_event(unsigned ev);                 // 0x49aca0
    NodeA* create();                            // 0x49ad20
    void place(int* p);                         // 0x49b4b0
    R8 text_rect();                             // 0x49b4f0
};

// ---- class B: the container (vptr 0x592848, base ctor 0x4931a0, size 0x190) ------------------------
struct NodeA {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18();
    virtual void v19(int a);                    // +0x4c
    virtual void v20(int a);                    // +0x50
    virtual void v21();                         // +0x54
    virtual void v22(float* p);                 // +0x58
    virtual void v23();                         // +0x5c
    virtual void v24();                         // +0x60
    virtual void v25(float* p);                 // +0x64
    virtual void v26(); virtual void v27(); virtual void v28(); virtual void v29();
    virtual void v30(); virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43(); virtual void v44();
    virtual void v45(); virtual void v46(); virtual void v47(); virtual void v48(); virtual void v49();
    virtual void v50(void* p);                  // +0xc8

    char pad0[0x158 - 4];
    void* f158;                                 // +0x158
    char pad15c[0x188 - 0x15c];
    char f188;                                  // +0x188
    char pad189[0x190 - 0x189];

    void* destroy(unsigned flags);              // 0x49b0f0
};

// ---- class C: the flag widget (vptr 0x591d58, base ctor 0x48ae20, size 0x188) ----------------------
struct UiA {
    virtual void u00(); virtual void u01(); virtual void u02(); virtual void u03(); virtual void u04();
    virtual void u05(); virtual void u06(); virtual void u07(); virtual void u08(); virtual void u09();
    virtual void u10(); virtual void u11(); virtual void u12(); virtual void u13(); virtual void u14();
    virtual void u15(); virtual void u16(); virtual void u17();
    virtual void set_key_flags(int f);          // +0x48
    virtual void v19(int a);                    // +0x4c
    virtual void v20(int a);                    // +0x50
    virtual void u21();                         // +0x54
    virtual void v22(float* p);                 // +0x58
    virtual void u23();                         // +0x5c
    virtual void u24();                         // +0x60
    virtual void v25(float* p);                 // +0x64
    virtual void u26();                         // +0x68
    virtual void set_size(float* p);            // +0x6c
    virtual void set_pos(float* p);             // +0x70
    virtual void u29(); virtual void u30(); virtual void u31();
    virtual void v32(int a, void* b);           // +0x80
    virtual void u33(); virtual void u34(); virtual void u35();
    virtual void refresh();                     // +0x90
    virtual void get_xy(float* p);              // +0x94
    virtual void u38(); virtual void u39(); virtual void u40();
    virtual void v41(void* a, void* b);         // +0xa4

    char pad0[0x10c - 4];
    void* f10c;                                 // +0x10c
    void* f110;                                 // +0x110
    void* f114;                                 // +0x114
    char pad118[0x121 - 0x118];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
    int f178;                                   // +0x178
    char pad17c[0x180 - 0x17c];
    char f180;                                  // +0x180
    char pad181[0x184 - 0x181];
    int f184;                                   // +0x184
};

extern PVec g_5d7080;                           // 0x5d7080 (the container list)

// ---- function definitions (in VA order) ----------------------------------------------------------

// @ 0x49aca0
char PObj::on_event(unsigned ev)
{
    char r = 0;
    if (ev == 0x10) {
        r = 1;
        f180 = 1;
        if (v190_first != 0) {
            int n = (int)(v190_last - v190_first);
            if (n != 0) {
                play_sound_48b2d0((void*)0x5d7080);
                NodeA* o = create();
                f188 = o;
                o->f158 = this;
            }
        }
    } else if (ev == 0x10000) {
        f180 = 0;
    }
    set_key_flags(ev);
    refresh();
    return r;
}

// @ 0x49ad20
NodeA* PObj::create()
{
    NodeA* b = (NodeA*)::operator new(0x190);
    if (b != 0) {
        base_ctor_4931a0(b);
        *(void**)b = (void*)0x592848;
        b->f188 = 1;
    } else {
        b = 0;
    }

    float wh[2];
    wh[0] = f1b0 * dx8;
    wh[1] = f1b4 * dxc;
    int n = v190_first ? (int)(v190_last - v190_first) : 0;

    float pos[2];
    float nx = -(f1b8 * dx8);
    pos[0] = de0 - nx;
    pos[1] = de4 - (float)n * wh[1];
    b->v22(pos);

    float sz[2];
    sz[0] = wh[0];
    sz[1] = (float)n * wh[1];
    b->v25(sz);

    *(char*)0x5d8788 = 1;
    NodeA* tmp = b;
    g_5d7080.push_back(tmp);
    b->v19(0x20000);
    b->v20(4);

    for (unsigned u = 0; v190_first != 0 && u < (unsigned)(v190_last - v190_first); u++) {
        UiA* c = (UiA*)::operator new(0x188);
        if (c != 0) {
            base_ctor_48ae20(c);
            *(int*)((char*)c + 0x178) = 0;
            *(char*)((char*)c + 0x121) = 0;
            *(void**)c = (void*)0x591d58;
            *(char*)((char*)c + 0x180) = 0;
            *(int*)((char*)c + 0x184) = 0;
        } else {
            c = 0;
        }

        float d[2];
        d[0] = 0.0f;
        d[1] = (float)(int)u * wh[1];
        c->v22(d);

        c->v25(wh);

        float m1[2];
        m1[0] = f1bc * dx8;
        m1[1] = f1c0 * dxc;
        c->set_size(m1);

        float m2[2];
        m2[0] = f1c4 * dx8;
        m2[1] = f1c8 * dxc;
        c->set_pos(m2);

        c->v32(2, f1a4->clone());
        ((Fadeable*)c->f110)->fade_scale(vector4(dx8, dxc * 0.5f, 1.0f));
        c->v32(1, f1a0->clone());
        ((Fadeable*)c->f10c)->fade_scale(vector4(dx8, dxc * 0.5f, 1.0f));
        c->v32(3, f1a8->clone());
        ((Fadeable*)c->f114)->fade_scale(vector4(dx8, dxc * 0.5f, 1.0f));

        float e[2];
        c->v41(e, v190_first + u);
        c->v19(0x10000);
        c->v20(3);
        b->v50(c);
    }
    return b;
}

// @ 0x49b0f0
void* NodeA::destroy(unsigned flags)
{
    *(void**)this = (void*)0x592848;
    base_dtor_490520(this);
    if (flags & 1)
        ::operator delete(this);
    return this;
}

// @ 0x49b120
void PVec::push_back(const E4& x)
{
    E4* f = first;
    int n = f ? (int)(last - f) : 0;
    if (f != 0 && (unsigned)n < (unsigned)(end - f)) {
        *last = x;
        last = last + 1;
        return;
    }
    E4* tmp;
    insert(&tmp, last, x);
}

// @ 0x49b170
void PVec::insert(E4** out, E4* where, const E4& x)
{
    E4* f = first;
    unsigned idx = 0;
    if (f != 0 && (unsigned)(last - f) != 0)
        idx = (unsigned)(where - f);
    _Insert_n(where, 1, x);
    *out = first + idx;
}

// @ 0x49b1c0
void PVec::_Insert_n(E4* where, unsigned count, const E4& val)
{
    E4 tmp = val;
    unsigned cap = first ? (unsigned)(end - first) : 0u;
    if (count == 0)
        return;
    unsigned size = first ? (unsigned)(last - first) : 0u;
    if (0x3fffffffu - size < count) {
        _Xlen();
        return;
    }
    if (cap < size + count) {
        cap = (0x3fffffffu - cap / 2 < cap) ? 0u : cap + cap / 2;
        if (cap < size + count)
            cap = size + count;
        E4* newvec = (E4*)::operator new(cap * sizeof(E4));
        E4* p = newvec;
        for (E4* q = first; q != where; ) *p++ = *q++;
        for (unsigned k = count; k; k--) *p++ = tmp;
        for (E4* q = where; q != last; ) *p++ = *q++;
        count += (unsigned)(last - first);
        if (first)
            ::operator delete(first);
        end = newvec + cap;
        last = newvec + count;
        first = newvec;
    } else if ((unsigned)(last - where) < count) {
        E4* oldend = last;
        memmove(where + count, where, (unsigned)(oldend - where) * sizeof(E4));
        last = oldend + count;
        for (E4* q = oldend; q != last; ) *q++ = tmp;
        for (E4* q = where; q != oldend; ) *q++ = tmp;
    } else {
        E4* oldend = last;
        memmove(oldend, oldend - count, count * sizeof(E4));
        last = oldend + count;
        memmove(where + count, where, (unsigned)(oldend - count - where) * sizeof(E4));
        for (E4* q = where; q != where + count; ) *q++ = tmp;
    }
}

// 0x49b37d is not a function: it is the catch(...) funclet inside PVec::_Insert_n (0x49b1c0, whose COMDAT really spans
// 0x49b1c0..0x49b4af); see nonmatching.txt.

// @ 0x49b4b0
void PObj::place(int* p)
{
    int a = -1;
    int b = 6;
    ((Mission*)this)->add_placed(&b, p, p + 1, (const void*)((char*)g_5e2930 + 0x14014), &a);
}

// @ 0x49b4f0
R8 PObj::text_rect()
{
    if (m118 == 0) {
        mesh* m = new mesh;
        {
            mstr s("ui_font");
            *(void**)((char*)m + 0x2c) = ((resmgr*)0x5d6fa0)->lookup(s);
        }
        v31(4, m);
    }
    if (f122) {
        *(int*)((char*)m118 + 0x114) = 1;
        *(int*)((char*)m118 + 0x140) = 1;
    }

    R8 r;
    float pos[4];
    pos[1] = f13c;
    pos[2] = 0.0f;
    int arg = -1;
    pos[0] = f138;
    Font* font = f124 ? (Font*)f124 : g_font;

    m118->clear_geometry();
    font->layout_text((float*)&r, &name, (vector4*)pos, (float*)v140, m118, (int)&arg,
                      (float*)&vf0[2], (float*)&vf0[0], f148);
    v38(&f14c);
    return r;
}
