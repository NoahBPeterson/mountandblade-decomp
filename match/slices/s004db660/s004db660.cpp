// Slice s004db660: the campaign "done / game over" window and the VC7.1 <vector> instantiations beside it.
//  0x4db660  init_done_window  builds the "ui_done" overlay window, a "ui_factions" text button and an empty
//                              button, adds the overlay as the GUI root, clears the overlay text and re-runs
//                              the window's dynamic pass.
//  0x4db810  new_button        allocates a 0x188-byte widget, positions/scales it, gives it the
//                              button5_up / _down / _hl meshes, sets the text and adds it to an overlay.
//  0x4db960  std::vector<E80>::insert(where, val)
//  0x4db9d0  std::vector<E68>::_Insert_n   (cold branches 0x4dbb83 and the catch funclet 0x4dbc3f are in its COMDAT)
//  0x4dbcc0  std::vector<E176>::_Insert_n  (cold branches and catch funclets 0x4dbebf, 0x4dbf7e: same COMDAT)
// The three container functions are plain instantiations of the library template over element types whose
// copy constructor / assignment live elsewhere; the element layouts below are what the inlined destructors
// and member copies need.  flags /O2 /MT /Gy /TP
#include <new>
#include <string.h>
#include "rgl.h"

namespace rgl {
struct vector2 { float x, y; vector2() {} vector2(float a, float b) : x(a), y(b) {} };
}

// ---- engine string (16 bytes, see s0042a1f0/tstring.h) --------------------------------------------------
struct tstring {
    tstring& operator=(const tstring& o);       // 0x42bc50
    int pos;            // +0
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
};

void* __cdecl operator new(unsigned n);         // 0x53545e
void __cdecl operator delete(void* p);          // 0x535712
void __fastcall ctor_4931a0(void* p);           // 0x4931a0 (ecx = overlay)

struct WRet { int a, b; WRet(); };

struct Mesh;
// ---- widgets (vtable slots verified against the call sites) -------------------------------------------
struct Widget {
    Widget();                                   // 0x48ae20
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual void v15();
    virtual void v16();
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual void v21();
    virtual void set_pos(const float* p);   // +0x58
    virtual void v23();
    virtual void v24();
    virtual void set_scale(const float* p);   // +0x64
    virtual void v26();
    virtual void v27();
    virtual void v28();
    virtual void v29();
    virtual void v30();
    virtual void set_mesh(int kind, Mesh* m);   // +0x7c
    virtual void v32();
    virtual void v33();
    virtual void v34();
    virtual void v35();
    virtual void v36();
    virtual void v37();
    virtual void v38();
    virtual void v39();
    virtual void v40();
    virtual WRet set_text(const tstring& s);   // +0xa4
    virtual void v42();
    virtual void v43();
    virtual void v44();
    virtual void v45();
    virtual void v46();
    virtual void v47();
    virtual void v48();
    virtual void v49();
    virtual void add_child(void* c);   // +0xc8
    virtual void v51();

    char pad0[0x121 - 4];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
    int f178;                                   // +0x178
    char pad17c[0x180 - 0x17c];
    unsigned char f180;                         // +0x180
    char pad181[0x184 - 0x181];
    unsigned f184;                              // +0x184 (button flags; bit 0 set by 0x4db660)
};

// the overlay window (vptr 0x592848, base ctor 0x4931a0, 0x190 bytes)
struct Overlay : Widget {
    char f188;                                  // +0x188
};

static inline Overlay* new_overlay()
{
    Overlay* p = (Overlay*)operator new(0x190);
    return p != 0 ? (ctor_4931a0(p), *(void**)p = (void*)0x592848, p->f188 = 0, p) : 0;
}

// the screen manager (`this` of 0x4db660)
struct Screen {
    char pad0[4];
    int f4;                                     // +0x4 (dynamic pass selector)
    char pad8[0x20 - 8];
    int fourth;                                 // +0x20
    char pad24[0x2c - 0x24];
    Overlay* overlay;                           // +0x2c
    Widget* f30;                                // +0x30
    Widget* f34;                                // +0x34
    Widget* f38;                                // +0x38
    void init_done_window();                    // 0x4db660
    void dynamic_4d7e50();                      // 0x4d7e50
};

struct GuiMgr { void reset(); void add_root(void* const& p); };      // 0x42d1b0 / 0x49b120
struct Mesh;
struct ResMgr { Mesh* get_object(const tstring& n); };
struct StrTab {                                 // 0x5e2a18
    char pad0[0xdf34];
    int count;                                  // +0xdf34
    const void* find(const tstring& name);      // 0x43a1e0 (returns the 0x10-byte string record)
};

#define g_gui     (*(GuiMgr*)0x5d7080)
#define g_resmgr  (*(ResMgr*)0x5d6fa0)
#define g_strtab  (*(StrTab*)0x5e2a18)
#define g_5d8788  (*(char*)0x5d8788)

// ---- callees -------------------------------------------------------------------------------------------
void ctor_48ae20(void* p);                      // 0x48ae20 (ecx = object)
struct Mesh { Mesh* clone(); };                 // 0x41c6d0
  // 0x4d7e50 (window dynamic pass)

// @ 0x4db810
Widget* new_button(Overlay* o, const tstring& text, float px, float py, float sx, float sy)
{
    Widget* b = (Widget*)operator new(0x188);
    if (b != 0) {
        new (b) Widget();
        b->f178 = 0;
        b->f121 = 0;
        ((void**)b)[0] = (void*)0x591d58;       // button5 vtable
        b->f180 = 0;
        b->f184 = 0;
    } else {
        b = 0;
    }
    Widget* r = b;
    {
        rgl::vector2 t;
        t.x = px; t.y = py;
        r->set_pos(&t.x);
    }
    {
        rgl::vector2 t;
        t.x = sx; t.y = sy;
        r->set_scale(&t.x);
    }
    {
        tstring n("button5_up");
        r->set_mesh(2, g_resmgr.get_object(n)->clone());
    }
    {
        tstring n("button5_down");
        r->set_mesh(1, g_resmgr.get_object(n)->clone());
    }
    {
        tstring n("button5_hl");
        r->set_mesh(3, g_resmgr.get_object(n)->clone());
    }
    r->set_text(text);
    o->add_child(r);
    return r;
}

// @ 0x4db660
void Screen::init_done_window()
{
    g_gui.reset();
    overlay = new_overlay();
    {
        float mv[2];
        mv[0] = 0.0f;
        mv[1] = 0.0f;
        overlay->set_pos(mv);
    }
    {
        float sv[2];
        sv[0] = 1.0f;
        sv[1] = 0.75f;
        overlay->set_scale(sv);
    }
    {
        tstring n("ui_done");
        const tstring& t = *(tstring*)g_strtab.find(n);
        f30 = new_button(overlay, t, 0.65f, 0.03f, 0.15f, 0.05f);
    }
    {
        tstring none("");
        f38 = new_button(overlay, none, 0.65f, 0.6f, 0.15f, 0.05f);
    }
    f38->f184 |= 1;
    {
        tstring n("ui_factions");
        f38->set_text(*(tstring*)g_strtab.find(n));
    }
    {
        void* self = overlay;
        g_5d8788 = 1;
        g_gui.add_root(self);
    }
    {
        tstring none("");
        overlay->set_text(none);
    }
    if (f4 == 0)
        dynamic_4d7e50();
}

// ---- std::vector instantiations (VC7.1 <vector>) -------------------------------------------------------
// The three container functions are the library's own template code; the cold branches and the catch
// funclets (0x4dbb83/0x4dbc3f for 0x4db9d0, 0x4dbebf for 0x4dbcc0) are part of the same COMDATs.
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

// 0x68-byte element: a name, ten ints, a delete[] array at +0x38, a second string at +0x44 and a
// (begin, end, cap) triple at +0x58 whose block is freed by the destructor; copy ctor 0x4cd100,
// assignment 0x4cd2f0, the destructor's first call is 0x4c8330.
struct E68 {
    tstring name;                               // +0
    int f10[10];                                // +0x10
    int* arr;                                   // +0x38
    int pad3c[2];
    tstring text;                               // +0x44
    int pad54;
    void* vb; void* ve; void* vc;               // +0x58
    int f64;                                    // +0x64
    E68(const E68& o);                          // 0x4cd100
    E68& operator=(const E68& o);               // 0x4cd2f0
    void free_materials();                      // 0x4c8330
    ~E68()
    {
        free_materials();
        if (vb)
            operator delete(vb);
        vb = 0; ve = 0; vc = 0;
        if (arr)
            operator delete[](arr);
    }
};

// 0xb0-byte element: memberwise copy (tstring copy ctor 0x42c110, vector<int> copy ctor 0x4cc280)
struct E176 {
    tstring name;                               // +0
    int a, b, c;                                // +0x10
    std::vector<float> v;                       // +0x1c (4-byte elements; 'float' only keeps it a different instance than the vector<int> at 0x43ed20)
    int blob[32];                               // +0x2c
    int tail;                                   // +0xac
};

// @ 0x4db960
template std::vector<E80>::iterator std::vector<E80>::insert(std::vector<E80>::iterator, const E80&);
// @ 0x4db9d0
template void std::vector<E68>::_Insert_n(std::vector<E68>::iterator, size_t, const E68&);
// @ 0x4dbcc0
template void std::vector<E176>::_Insert_n(std::vector<E176>::iterator, size_t, const E176&);
