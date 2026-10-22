// Slice s0049a0b0: the scene-editor widget manager's "party button" factory. Builds a 0x188-byte
// widget, fills its mesh slots (party_button / party_button_down) from the resource manager, sets the
// handle sizes, registers it with the scene and appends it to the manager's vector.
// ALL NAMES PROVISIONAL (layouts recovered from the byte-exact call sites).
#include <windows.h>
#include <math.h>
#include <vector>
#include "rgl.h"
#include "mesh.h"

using namespace rgl;

// ---- shared widget base ("panel", base ctor 0x48ae20) and its 41 virtual slots --------------------
struct panel {
    panel();                                        // 0x48ae20
    virtual void z0();
};

#define WG_VIRTUALS \
    virtual void w1();  virtual void w2();  virtual void w3();  virtual void w4(); \
    virtual void w5();  virtual void w6();  virtual void w7();  virtual void w8(); \
    virtual void w9();  virtual void w10(); virtual void w11(); virtual void w12(); \
    virtual void w13(float); \
    virtual void w14(); virtual void w15(); virtual void w16(); virtual void w17(); \
    virtual void w18(); virtual void w19(); virtual void w20(); virtual void w21(); \
    virtual void w22(void*); \
    virtual void w23(); virtual void w24(); \
    virtual void w25(vec2*); \
    virtual void w26(); \
    virtual void w27(vec2*); \
    virtual void w28(vec2*); \
    virtual void w29(void*); \
    virtual void w30(); virtual void w31(); \
    virtual void w32(int, mesh*); \
    virtual void w33(); virtual void w34(); virtual void w35(); virtual void w36(); \
    virtual void w37(); virtual void w38(); \
    virtual void w39(const void*); \
    virtual void w40(); \
    virtual void w41(const void*, const void*);

// 0x49a0b0 -> "party button": size 0x188, vtable 0x592208.
// The (f178, b121) initialisation sits in an intermediate base whose inline ctor the concrete class
// inherits, so cl emits the concrete class's vptr store after those field stores; the derived ctor
// body (b180, b181, f184 = 0x1c) follows the vptr store.
struct ab_base : panel {
    char p0[0xd8 - 4];
    float fd8, fdc;                 // 0xd8, 0xdc
    char p1[0x10c - 0xe0];
    mesh* m10c;                     // 0x10c: mesh slot 1
    mesh* m110;                     // 0x110: mesh slot 2
    char p2[0x121 - 0x114];
    unsigned char b121;             // 0x121
    char p3[0x178 - 0x122];
    int f178;                       // 0x178
    ab_base() { f178 = 0; b121 = 0; }
};
struct cls_e : ab_base { WG_VIRTUALS
    char p4[0x180 - 0x17c];
    unsigned char b180;             // 0x180
    unsigned char b181;             // 0x181
    char p5[0x184 - 0x182];
    int f184;                       // 0x184
    cls_e() { b180 = 0; b181 = 0; f184 = 0x1c; }
};

// the scene the editor window is attached to (vtable +0xc8 takes the new widget)
struct scene_iface {
    virtual void d0();  virtual void d1();  virtual void d2();  virtual void d3();  virtual void d4();
    virtual void d5();  virtual void d6();  virtual void d7();  virtual void d8();  virtual void d9();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30(); virtual void d31(); virtual void d32(); virtual void d33(); virtual void d34();
    virtual void d35(); virtual void d36(); virtual void d37(); virtual void d38(); virtual void d39();
    virtual void d40(); virtual void d41(); virtual void d42(); virtual void d43(); virtual void d44();
    virtual void d45(); virtual void d46(); virtual void d47(); virtual void d48(); virtual void d49();
    virtual void d50(panel* w);                     // slot 50 (vtable +0xc8)
};

// the widget factory: a std::vector<panel*> (_Myfirst at +4) with the scene at +0x2c.
struct widget_mgr {
    std::vector<panel*> widgets;    // 0x00
    char f10[0x2c - 0x10];
    scene_iface* scene;             // 0x2c

    int  add_party();                               // 0x49a0b0

    void ins_slow(panel** where, int count, panel* const& val);     // 0x4927d0 (vector::_Insert_n)
};

typedef char _chk_ce[sizeof(cls_e) == 0x188 ? 1 : -1];
typedef char _chk_off_e10c[offsetof(cls_e, m10c) == 0x10c ? 1 : -1];
typedef char _chk_off_e110[offsetof(cls_e, m110) == 0x110 ? 1 : -1];
typedef char _chk_off_eb121[offsetof(cls_e, b121) == 0x121 ? 1 : -1];
typedef char _chk_off_ef178[offsetof(cls_e, f178) == 0x178 ? 1 : -1];
typedef char _chk_off_eb180[offsetof(cls_e, b180) == 0x180 ? 1 : -1];
typedef char _chk_off_eb181[offsetof(cls_e, b181) == 0x181 ? 1 : -1];
typedef char _chk_off_ef184[offsetof(cls_e, f184) == 0x184 ? 1 : -1];

// @ 0x49a0b0
int widget_mgr::add_party()
{
    cls_e* w = new cls_e;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->w22(&v);
    }

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.009f;
        w->w28(&v);
    }
    {
        vec2 v;
        v.x = 0.02f;
        v.y = 0.02f;
        w->w27(&v);
    }

    {
        mstr n("party_button");
        w->w32(2, g_resmgr.get_object(n)->clone());
    }
    {
        vector4 c(0.5f, 0.5f, 1.0f);
        w->m110->set_scale(c);
    }

    {
        mstr n("party_button_down");
        w->w32(1, g_resmgr.get_object(n)->clone());
    }
    {
        vector4 c(0.5f, 0.5f, 1.0f);
        w->m10c->set_scale(c);

        w->w3();

        {
            mstr n("");
            w->w41(&c, &n);
        }
    }

    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)(widgets.size() - 1);
}
