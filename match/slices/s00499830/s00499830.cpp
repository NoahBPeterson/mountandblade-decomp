// Slice s00499830: the scene-editor widget manager's factory methods (medium button, scene button,
// progress bar, drop-down button). Each builds a widget object, fills its mesh slots from the resource
// manager and registers itself with the scene, then appends itself to the manager's vector.
// ALL NAMES PROVISIONAL (layouts recovered from the byte-exact call sites).
#include <windows.h>
#include <math.h>
#include <vector>
#include "rgl.h"
#include "mesh.h"

using namespace rgl;

// ---- the editor's scene table: a 0x48-byte record array at +0 (placed() lives in slice s00499050) ----
struct rec48 {
    mstr name;                      // 0x00
    unsigned char flags;            // 0x10
    char pad1[0x24 - 0x11];
    float px, py, pz;               // 0x24
    float rx, ry, rz;               // 0x30
    float sx, sy, sz;               // 0x3c
};
struct scene_tbl {
    rec48* recs;                    // 0x00
    mesh* placed(int index);                        // 0x499050
};
extern scene_tbl g_scene;           // 0x8ed4f58

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
    virtual void w41(void*, void*);

// 0x499830 -> "medium button": size 0x188, vtable 0x591e20.
// 0x499a20 -> scene button: size 0x188, vtable 0x591ee8.
// The field initialisation sits in an intermediate whose inline ctor the derived classes inherit, so
// cl emits the concrete class's vptr store after the field stores (not before them).
struct ab_base : panel {
    char p0[0xd8 - 4];
    float fd8, fdc;                 // 0xd8, 0xdc
    char p1[0x121 - 0xe0];
    unsigned char b121;             // 0x121
    char p2[0x178 - 0x122];
    int f178;                       // 0x178
    char p3[0x180 - 0x17c];
    unsigned char b180;             // 0x180
    char p4[0x184 - 0x181];
    int f184;                       // 0x184
    ab_base() { f178 = 0; b121 = 0; b180 = 0; f184 = 0; }
    void set_flag() { f184 = 1; }
};
struct cls_a : ab_base { WG_VIRTUALS };
struct cls_b : ab_base { WG_VIRTUALS };

// 0x499b80 -> progress bar: size 0x1b0, ctor at 0x48b760 (installs its own vtable).
struct cls_c : panel { WG_VIRTUALS
    char p0[0xf4 - 4];
    float ff4;                      // 0xf4
    char p1[0x108 - 0xf8];
    mesh* m108;                     // 0x108: mesh slot 0
    char p2[0x16c - 0x10c];
    float f16c, f170;               // 0x16c, 0x170
    char p3[0x18c - 0x174];
    void* m18c;                     // 0x18c
    char p4[0x198 - 0x190];
    vector4 v198;                   // 0x198
    char p5[0x1b0 - 0x1a8];
    cls_c();                                        // 0x48b760
};

// 0x499d90 -> drop-down button: size 0x1d0, ctor at 0x4931d0 (installs its own vtable).
struct cls_d : panel { WG_VIRTUALS
    char p0[0x10c - 4];
    mesh* m10c;                     // 0x10c: mesh slot 1
    mesh* m110;                     // 0x110: mesh slot 2
    mesh* m114;                     // 0x114: mesh slot 3
    char p1[0x1a0 - 0x118];
    mesh* m1a0;                     // 0x1a0
    mesh* m1a4;                     // 0x1a4
    mesh* m1a8;                     // 0x1a8
    char p2[0x1d0 - 0x1ac];
    cls_d();                                        // 0x4931d0
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

    int  add_medium(int id);                        // 0x499830
    int  add_xyz(int a, int b);                     // 0x499a20
    int  add_xyz_clicked(int a, int b);             // 0x499b50
    int  add_progress(int a, int b);                // 0x499b80
    int  add_dropdown();                            // 0x499d90

    void ins_slow(panel** where, int count, panel* const& val);     // 0x4927d0 (vector::_Insert_n)
};

typedef char _chk_ca[sizeof(cls_a) == 0x188 ? 1 : -1];
typedef char _chk_cb[sizeof(cls_b) == 0x188 ? 1 : -1];
typedef char _chk_cc[sizeof(cls_c) == 0x1b0 ? 1 : -1];
typedef char _chk_cd[sizeof(cls_d) == 0x1d0 ? 1 : -1];
typedef char _chk_off_d8[offsetof(cls_a, fd8) == 0xd8 ? 1 : -1];
typedef char _chk_off_f178[offsetof(cls_a, f178) == 0x178 ? 1 : -1];
typedef char _chk_off_cc108[offsetof(cls_c, m108) == 0x108 ? 1 : -1];
typedef char _chk_off_cd110[offsetof(cls_d, m110) == 0x110 ? 1 : -1];

// @ 0x499830
int widget_mgr::add_medium(int id)
{
    cls_a* w = new cls_a;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->w22(&v);
        *(int*)&v.x = 0x10;
        w->w29(&v);
    }

    {
        mesh* m;
        {
            mstr n("medium_button");
            m = g_resmgr.get_object(n)->clone();
        }
        m->scale(0.5555556f);
        w->w32(2, m);
    }
    {
        mesh* m;
        {
            mstr n("medium_button_down");
            m = g_resmgr.get_object(n)->clone();
        }
        m->scale(0.5555556f);
        w->w32(1, m);
    }

    {
        unsigned c = 0xff007700;
        w->w39(&c);
    }

    {
        vec2 v;
        v.x = 0.148f;
        v.y = 0.04f;
        w->w25(&v);
        w->w41(&v, (void*)(size_t)id);
        vec2 s;
        s.x = 0.148f;
        s.y = 0.04f;
        *(vec2*)((char*)w + 0xd8) = s;
    }

    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)(widgets.size() - 1);
}

// @ 0x499a20
int widget_mgr::add_xyz(int a, int b)
{
    cls_b* w = new cls_b;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->w22(&v);
        *(int*)&v.x = 0x700;
        w->w29(&v);
    }

    w->w32(2, g_scene.placed(a));
    w->w32(1, g_scene.placed(b));
    w->w3();

    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)(widgets.size() - 1);
}

// @ 0x499b50
int widget_mgr::add_xyz_clicked(int a, int b)
{
    int i = add_xyz(a, b);
    ((cls_b*)widgets[i])->set_flag();
    return i;
}

// @ 0x499b80
int widget_mgr::add_progress(int a, int b)
{
    cls_c* w = new cls_c;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->w22(&v);
    }

    {
        vector4 sc(0.38940807f, 1.0f, 1.0f);

        {
            mstr n("progressbar");
            w->w32(0, g_resmgr.get_object(n)->clone());
        }
        w->m108->set_scale(sc);

        {
            mstr n("handle_hor");
            w->w32(2, g_resmgr.get_object(n)->clone());
        }
        {
            mstr n("progressbar_handle");
            w->m18c = g_resmgr.get_object(n);
        }
        w->v198 = sc;
    }
    w->w3();

    {
        vec2 h;
        h.x = 0.25f;
        h.y = w->ff4;
        w->w25(&h);
    }

    float fa = (float)a;
    w->f16c = fa;
    w->f170 = (float)b;
    w->w13(fa);

    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)(widgets.size() - 1);
}

// @ 0x499d90
int widget_mgr::add_dropdown()
{
    cls_d* w = new cls_d;

    {
        vec2 v;
        v.x = 0.0f;
        v.y = 0.0f;
        w->w22(&v);
    }

    {
        mstr n("button_drop");
        w->w32(2, g_resmgr.get_object(n)->clone());
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        w->m110->set_scale(c);
    }

    {
        mstr n("button_drop_clicked");
        w->w32(1, g_resmgr.get_object(n)->clone());
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        w->m10c->set_scale(c);
    }

    {
        mstr n("button_drop_hl");
        w->w32(3, g_resmgr.get_object(n)->clone());
    }
    {
        vector4 c(1.0f, 0.5f, 1.0f);
        w->m114->set_scale(c);
    }

    {
        mstr n("button_drop_child");
        w->m1a4 = g_resmgr.get_object(n);
    }
    {
        mstr n("button_drop_child_clicked");
        w->m1a0 = g_resmgr.get_object(n);
    }
    {
        mstr n("button_drop_child_hl");
        w->m1a8 = g_resmgr.get_object(n);
    }

    w->w3();

    {
        vec2 p;
        p.x = 0.02f;
        p.y = 0.02f;
        w->w27(&p);
        p.x = 0.02f;
        p.y = 0.013f;
        w->w28(&p);
        {
            mstr n("");
            w->w41(&p, &n);
        }
    }

    scene->d50(w);
    widgets.push_back(w + 0);
    return (int)(widgets.size() - 1);
}
