// Slice s004db660: the campaign "done / game over" window and the container helpers beside it.
//  0x4db660  init_done_window  builds the "ui_done" overlay window, a "ui_factions" text button and an empty
//                              button, adds the overlay as the GUI root, clears the overlay text and re-runs
//                              the window's dynamic pass.
//  0x4db810  new_button        allocates a 0x188-byte widget, positions/scales it, gives it the
//                              button5_up / _down / _hl meshes, sets the text and adds it to an overlay.
//  0x4db960  insert_           std::vector<0x50>::insert, returns the new element.
//  0x4db9d0  grow_68           std::vector<0x68>::_Insert_n growth; its copy-ctor error path is 0x4dbc3f and
//                              the vector's unspecialised `_Grow` continuation is 0x4dbb83.
//  0x4dbcc0  grow_110          std::vector<0xb0>::_Insert_n growth; error path 0x4dbebf.
// 0x4dbc3f / 0x4dbb83 / 0x4dbebf are funclet bodies inside the two growth functions (catch handler, try-end),
// so they are part of 0x4db9d0 / 0x4dbcc0 here rather than separate definitions - see partial.txt.
// Not byte-exact: the EH funclet frames and the element-index spill differ from the original build.
// flags /O2 /MT /Gy /TP /GX
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

// ---- widgets (slot map as in s004dada0) ----------------------------------------------------------------
struct Widget {
    Widget();                                   // 0x48ae20
    virtual void w00(int); virtual void w01(int); virtual void w02(int); virtual void w03(int);
    virtual void w04(int); virtual void w05(int); virtual void w06(int); virtual void w07(int);
    virtual void w08(int); virtual void w09(int); virtual void w10(int); virtual void w11(int);
    virtual void w12(int); virtual void w13(int); virtual void w14(int); virtual void w15(int);
    virtual void w16(int); virtual void w17(int); virtual void w18(int); virtual void w19(int);
    virtual void w20(int); virtual void w21(int);
    virtual void set_pos(const float* p);       // +0x58
    virtual void w23(int); virtual void w24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void w26(int); virtual void w27(int);
    virtual void set_size(const float* p);      // +0x6c
    virtual void w29(int); virtual void w30(int);
    virtual void set_mesh(void* m, int kind);   // +0x7c
    virtual void w32(int); virtual void w33(int); virtual void w34(int); virtual void w35(int);
    virtual void w36(int); virtual void w37(int); virtual void w38(int); virtual void w39(int);
    virtual void w40(int); virtual void w41(int); virtual void w42(int); virtual void w43(int);
    virtual void w44(int); virtual void w45(int); virtual void w46(int); virtual void w47(int);
    virtual void w48(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void w50(int); virtual void w51(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
    virtual void w53(int); virtual void w54(int); virtual void w55(int); virtual void w56(int);
    virtual void w57(int); virtual void w58(int); virtual void w59(int); virtual void w60(int);
    virtual void w61(int); virtual void w62(int); virtual void w63(int); virtual void w64(int);
    virtual void w65(int); virtual void w66(int); virtual void w67(int); virtual void w68(int);
    virtual void w69(int); virtual void w70(int); virtual void w71(int); virtual void w72(int);
    virtual void w73(int); virtual void w74(int); virtual void w75(int); virtual void w76(int);
    virtual void w77(int); virtual void w78(int); virtual void w79(int); virtual void w80(int);
    virtual void w81(int); virtual void w82(int); virtual void w83(int); virtual void w84(int);
    virtual void w85(int); virtual void w86(int);
    virtual void v87(void* a, const void* b);   // +0xac
    virtual void w88(int); virtual void w89(int); virtual void w90(int); virtual void w91(int);
    virtual void w92(int); virtual void w93(int); virtual void w94(int); virtual void w95(int);
    virtual void w96(int); virtual void w97(int); virtual void w98(int); virtual void w99(int);
    virtual void w100(int); virtual void w101(int); virtual void w102(int); virtual void w103(int);
    virtual void w104(int); virtual void w105(int); virtual void w106(int); virtual void w107(int);
    virtual void w108(int); virtual void w109(int); virtual void w110(int); virtual void w111(int);
    virtual void w112(int); virtual void w113(int); virtual void w114(int); virtual void w115(int);
    virtual void w116(int); virtual void w117(int); virtual void w118(int); virtual void w119(int);
    virtual void w120(int); virtual void add_child(void* c);    // +0xc8

    char pad0[0x121 - 4];
    char f121;                                  // +0x121
    char pad122[0x178 - 0x122];
    int f178;                                   // +0x178
    char pad17c[0x180 - 0x17c];
    unsigned char f180;                         // +0x180
    char pad181[0x184 - 0x181];
    int f184;                                   // +0x184 (button flags; bit 0 set by 0x4db660)
};

// after the virtual table: one flag byte at +0x188 (see 0x4db660)
struct OverlayBase {
    OverlayBase();                              // 0x4931a0
    virtual void n00(int); virtual void n01(int); virtual void n02(int); virtual void n03(int);
    virtual void n04(int); virtual void n05(int); virtual void n06(int); virtual void n07(int);
    virtual void n08(int); virtual void n09(int); virtual void n10(int); virtual void n11(int);
    virtual void n12(int); virtual void n13(int); virtual void n14(int); virtual void n15(int);
    virtual void n16(int); virtual void n17(int); virtual void n18(int); virtual void n19(int);
    virtual void n20(int); virtual void n21(int);
    char pad58[0x184];
    char f188;                                  // +0x188
};

// the overlay window (vptr 0x592848, base ctor 0x4931a0, 0x190 bytes)
struct Overlay : OverlayBase {
    virtual void set_pos(const float* p);       // +0x58
    virtual void n23(int); virtual void n24(int);
    virtual void set_scale(const float* p);     // +0x64
    virtual void n26(int); virtual void n27(int);
    virtual void set_size(const float* p);      // +0x6c
    virtual void n29(int);
    virtual void set_flag(const int* f);        // +0x74
    virtual void n31(int);
    virtual void set_mesh(void* m, int kind);   // +0x7c
    virtual void n33(int); virtual void n34(int); virtual void n35(int); virtual void n36(int);
    virtual void n37(int); virtual void n38(int); virtual void n39(int); virtual void n40(int);
    virtual void n41(int); virtual void n42(int); virtual void n43(int); virtual void n44(int);
    virtual void n45(int); virtual void n46(int); virtual void n47(int); virtual void n48(int);
    virtual void n49(int);
    virtual void set_color(const int* c);       // +0x98
    virtual void n51(int); virtual void n52(int);
    virtual void set_text(const void* a, const void* b);    // +0xa4
    virtual void n54(int);
    virtual void n55(int); virtual void n56(int); virtual void n57(int); virtual void n58(int);
    virtual void n59(int); virtual void n60(int); virtual void n61(int); virtual void n62(int);
    virtual void n63(int); virtual void n64(int); virtual void n65(int); virtual void n66(int);
    virtual void n67(int); virtual void n68(int); virtual void n69(int); virtual void n70(int);
    virtual void n71(int); virtual void n72(int); virtual void n73(int); virtual void n74(int);
    virtual void n75(int); virtual void n76(int); virtual void n77(int); virtual void n78(int);
    virtual void n79(int); virtual void n80(int); virtual void n81(int); virtual void n82(int);
    virtual void n83(int); virtual void n84(int); virtual void n85(int); virtual void n86(int);
    virtual void n87(int); virtual void n88(int); virtual void n89(int); virtual void n90(int);
    virtual void n91(int); virtual void n92(int);
    virtual void set_text2(const void* a, const void* b);   // +0xac
    virtual void add_child(void* c);            // +0xc8
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
};

struct GuiMgr { void reset(); void add_root(void* p); };
struct ResMgr { void* get_object(const tstring& n); };
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
void* clone_41c6d0(void* m);                    // 0x41c6d0
struct WidgetVec {                              // std::vector<T> (0x10 bytes)
    void* alloc; void* first; void* last; void* end;
    void add_root(Overlay* const& x);           // 0x49b120 (ecx = vector)
};
extern WidgetVec g_a18140;                      // 0xa18140 (GUI root list)
void __fastcall dynamic_4d7e50(Screen* s, int unused);  // 0x4d7e50 (window dynamic pass)

// a 0x50-byte-element vector (0x4db960): first at +4, last at +8, end at +0xc
struct Cont50 {
    void* alloc;
    char* first;
    char* last;
    char* end;
    void insert(char* pos, int n, const void* value);   // 0x4d9ed0
};

// @ 0x4db810
Widget* new_button(tstring& text, int unused1, Overlay* o, rgl::vector2 p, rgl::vector2 s)
{
    Widget* b = (Widget*)operator new(0x188);
    if (b != 0) {
        new (b) Widget();
        ((void**)b)[0] = (void*)0x591d58;       // button5 vtable
        b->f178 = 0;
        b->f121 = 0;
        b->f180 = 0;
        b->f184 = 0;
    } else {
        b = 0;
    }
    Widget* r = b;
    r->set_pos(&p.x);
    r->set_scale(&s.x);
    {
        tstring n("button5_up");
        r->set_mesh(clone_41c6d0(g_resmgr.get_object(n)), 2);
    }
    {
        tstring n("button5_down");
        r->set_mesh(clone_41c6d0(g_resmgr.get_object(n)), 1);
    }
    {
        tstring n("button5_hl");
        r->set_mesh(clone_41c6d0(g_resmgr.get_object(n)), 3);
    }
    r->set_text(&text, (const void*)0);
    o->add_child(r);
    return r;
}

// @ 0x4db660
void Screen::init_done_window()
{
    g_gui.reset();
    Overlay* o;
    o = new_overlay();
    overlay = o;
    float mv[2];
    mv[0] = 0.0f;
    mv[1] = 0.0f;
    o->set_pos(mv);
    float sv[2];
    sv[0] = 1.0f;
    sv[1] = 0.75f;
    o->set_scale(sv);
    {
        tstring n("ui_done");
        tstring& hit = *(tstring*)g_strtab.find(n);
        f30 = new_button(hit, 0, overlay, rgl::vector2(0.1f, 0.6f), rgl::vector2(0.65f, 0.15f));
    }
    {
        tstring none("");
        f38 = new_button(none, 0, overlay, rgl::vector2(0.6f, 0.2f), rgl::vector2(0.65f, 0.15f));
    }
    f38->f184 |= 1;
    {
        tstring n("ui_factions");
        const void* t = g_strtab.find(n);
        f38->set_text(&n, t);
    }
    g_5d8788 = 1;
    Overlay* self = overlay;
    g_a18140.add_root(self);
    {
        tstring none("");
        Overlay* ov = overlay;
        ov->set_text(&none, (const void*)0);
    }
    if (f4 == 0 && fourth != 0)
        dynamic_4d7e50(this, 0);
}

// @ 0x4db960
extern "C" int* __fastcall insert_(Cont50* c, int unused1, int* out, char* pos, const void* value)
{
    int n = (c->last - c->first) / 0x50;
    int idx;
    if (c->first != 0 && n != 0)
        idx = (pos - c->first) / 0x50;
    else
        idx = 0;
    c->insert(pos, 1, value);                   // 0x4d9ed0: make room at `pos`
    *out = (int)(c->first + idx * 0x50);
    return out;
}

// ---- std::vector growth (0x4db9d0 / 0x4dbcc0) ---------------------------------------------------------
// Reference spelling of vector<P,E100>::_Insert_n: `_Tmp = _Val` (the element snapshot, built by the element's
// copy ctor), capacity/size, _Xlen / _Grow, allocation, the copy-fill-copy triple in a try block whose catch is
// the slice's 0x4dbc3f / 0x4dbebf, then the old block's destruction.
struct Vec {
    void* alloc;                                // +0x0
    char* first;                                // +0x4
    char* last;                                 // +0x8
    char* end;                                  // +0xc
};

void _Xlen();                                   // 0x43e2a0 (std::_Xlen)
unsigned _Grow_68(Vec* v, int unused);          // 0x48ace0 (_Grow, 0x68 elements; ecx = vector)
unsigned _Grow_110(Vec* v, int unused);         // 0x4be9a0 (_Grow, 0xb0 elements)
void snapshot_68(void* dst, const void* src);   // 0x4cd100 (copy ctor of the 0x68-byte element)
char* copy_before_68(char* first, char* last, char* dst, const void* src);    // 0x4d9400
char* fill_68(char* dst, unsigned n, const void* src);                       // 0x4da2d0
void  erase_first_68(char* first, char* last, Vec* v, const void* src);      // 0x4cd570
char* copy_before_110(char* first, char* last, char* dst, const void* src);   // 0x4d9420
char* fill_110(char* dst, unsigned n, const void* src);                      // 0x4da3e0
void  erase_first_110(char* first, char* last, Vec* v, const void* src);     // 0x4cc260
void  erase_first_110b(char* first, char* last, Vec* v, const void* src);    // 0x4ca7e0 (grow_110's cleanup call)
void  destroy_range_110(char* first, char* last, const void* v);             // 0x4cf0d0
void __stdcall _free(void* p);                                               // 0x535712 (operator delete)

// @ 0x4db9d0  (0x4dbb83 = the unspecialised _Grow continuation at +0x1b3 of the original;
//              0x4dbc3f = the catch funclets' first instruction)
unsigned __fastcall grow_68(Vec* v, int unused, int n, char* pos, const void* src)
{
    char snap[0x88];
    snapshot_68(snap, src);
    unsigned cap = (unsigned)(v->end - v->first) / 0x68;
    if (n != 0) {
        if (0x2762762 - (unsigned)(v->last - v->first) / 0x68 < n)
            _Xlen();
        if (cap < (unsigned)(v->last - v->first) / 0x68 + n) {
            unsigned newc = cap + (cap >> 1);
            if (newc < (unsigned)(v->last - v->first) / 0x68 + n)
                newc = _Grow_68(v, 0) + n;
            char* fresh = (char*)operator new(newc * 0x68);
            try {
                char* mid = copy_before_68(v->first, pos, fresh, src);
                char* after = fill_68(mid, n, src);
                copy_before_68(pos, v->last, after, src);
            } catch (...) {
                erase_first_68(fresh + n * 0x68, v->last, v, src);
                throw;
            }
            n += (unsigned)(v->last - v->first) / 0x68;
            if (v->first != 0) {
                erase_first_68(v->first, v->last, v, src);
                _free(v->first);
            }
            v->end = fresh + newc * 0x68;
            v->last = fresh + n * 0x68;
            v->first = fresh;
        }
    }
    return 0;
}

// @ 0x4dbcc0  (0x4dbebf = the catch funclets' first instruction)
unsigned __fastcall grow_110(Vec* v, int unused, int n, char* pos, const void* src)
{
    unsigned cap = (unsigned)(v->end - v->first) / 0xb0;
    if (n != 0) {
        if (0x1745d17 - (unsigned)(v->last - v->first) / 0xb0 < n)
            _Xlen();
        if (cap < (unsigned)(v->last - v->first) / 0xb0 + n) {
            unsigned newc = cap + (cap >> 1);
            if (newc < (unsigned)(v->last - v->first) / 0xb0 + n)
                newc = _Grow_110(v, 0) + n;
            char* fresh = (char*)operator new(newc * 0xb0);
            try {
                char* mid = copy_before_110(v->first, pos, fresh, src);
                char* after = fill_110(mid, n, src);
                copy_before_110(pos, v->last, after, src);
            } catch (...) {
                erase_first_110(fresh, fresh + n * 0xb0, v, src);
                _free(fresh);
                throw;
            }
            n += (unsigned)(v->last - v->first) / 0xb0;
            if (v->first != 0) {
                erase_first_110b(v->first, v->last, v, src);
                _free(v->first);
            }
            v->end = fresh + newc * 0xb0;
            v->last = fresh + n * 0xb0;
            v->first = fresh;
        }
    }
    return 0;
}
