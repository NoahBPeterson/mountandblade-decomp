// Slice s00509980: the character-sheet window's 3D portrait view.
//   0x509980  char_window::open_view  - build the portrait world (3D view) or the flat shadow fallback
//   0x50a140  view_obj::reset         - clear the selection, drop the buffers, rebuild the view
// The widget slots, tstring / eng_frustum / face-body / world / wind layouts and the callee conventions
// follow the verified neighbours s004f49d0 (the same 0x358 face body, 0x344 world and 0x7c wind trio),
// s004f8c80 (frustum) and s00502300 (widget vtables).  Names are provisional; every offset comes from
// this slice's disassembly.
#include <windows.h>
#include <new>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
#include "../tstring.h"

// ---- engine mesh -----------------------------------------------------------------------------------------
#include "../rgl_mesh.h"
using rgl::mesh;

// ---- widgets ---------------------------------------------------------------------------------------------
// 0x178-byte widget base (ctor 0x48ae20 installs vptr 0x591c80).  The slots used here are add_image (+0x7c),
// add_image2 (+0x80) and add_child (+0xc8); the rest are placeholders that keep the offsets honest.
struct widget {
    char pad0[0xe8 - 4];
    float ex;                                               // +0xe8
    float ey;                                               // +0xec
    float pex;                                              // +0xf0
    float pey;                                              // +0xf4
    char padf8[0x178 - 0xf8];
    widget() MB_NOTHROW;                                    // 0x48ae20
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21(); virtual void d22(); virtual void d23(); virtual void d24();
    virtual void d25(); virtual void d26(); virtual void d27(); virtual void d28(); virtual void d29();
    virtual void d30();
    virtual void add_image(int idx, void* m);               // +0x7c
    virtual void add_image2(int idx, void* m);              // +0x80
    virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37();
    virtual void d38(); virtual void d39(); virtual void d40(); virtual void d41(); virtual void d42();
    virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46(); virtual void d47();
    virtual void d48(); virtual void d49();
    virtual void add_child(void* child);                    // +0xc8
};

// the 0x188 icon element (ctor 0x48b1c0 calls the widget base, then the caller installs vptr 0x591ee8)
struct elem2 : widget {
    char pad[0x188 - 0x178];
    elem2() MB_NOTHROW;                                     // 0x48b1c0
};

extern char g_vt_591ee8[];                                  // 0x591ee8

// load the widget's "<name>_*" parameters from the ini table (0x4e4c70)
void elem_fill(widget* w, tstring name);                    // 0x4e4c70 (tstring by value)

// ---- frustum (0x120 bytes) -------------------------------------------------------------------------------
struct eng_frustum {
    char b[0x120];
    void build_planes();                                    // 0x41e4f0
    void init(float fov, float aspect, float znear, float far);  // 0x4f25a0
};

// ---- frame / matrix --------------------------------------------------------------------------------------
struct frame : rgl::matrix {
    void rotate_fu(const float* angle);                     // 0x40e570
    void rotate_su(const float* angle);                     // 0x40e670
};
frame* identity_pos(frame* out);                            // 0x40cd00 (cdecl, returns out)

// ---- entities --------------------------------------------------------------------------------------------
struct Entity {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18();
    virtual void on_anim_end();                             // +0x4c
    virtual void d20();                                     // +0x50
    virtual void set_matrix(void* m);                       // +0x54
};

// entity +0x108: the resource list the animation collect fills
struct ResList {
    void release(void** p);                                 // 0x41b750
};

// ---- wind track (0x7c bytes) -----------------------------------------------------------------------------
struct wind {
    int   type;                                             // +0x00
    rgl::vector4 v04;                                       // +0x04
    rgl::vector4 v14;                                       // +0x14
    rgl::vector4 v24;                                       // +0x24
    char  pad34[0x44 - 0x34];
    rgl::vector4 v44;                                       // +0x44
    float f54;                                              // +0x54
    char  pad58[0x7c - 0x58];
    wind() MB_NOTHROW;                                      // 0x461480
};

// ---- the world (0x344 bytes) -----------------------------------------------------------------------------
template<class T>
struct engvec {
    int lead;                                               // +0
    T*  first;                                              // +4
    T*  last;                                               // +8
    T*  end;                                                // +0xc
    void insert_n(T* where, unsigned n, const T& val);      // 0x410610
    void push_back(const T& v)
    {
        if (first == 0 || (unsigned)(last - first) >= (unsigned)(end - first))
            insert_n(last, 1, v);
        else
            *last++ = v;
    }
};

#include "../scene_world.h"                                   // World: ctor 0x478ad0, set_visual 0x488cd0, destroy 0x475d50

// ---- the 0x358 portrait body (ctor 0x4ed630) -------------------------------------------------------------
struct face_body : widget {
    World* world;                                           // +0x178
    char pad17c2[0x1ac - 0x17c];
    eng_frustum view;                                       // +0x1ac (0x120)
    char pad2cc[0x308 - 0x2cc];
    unsigned flags;                                         // +0x308
    unsigned color;                                         // +0x30c
    char pad310[0x358 - 0x310];
    face_body() MB_NOTHROW;                                 // 0x4ed630
};

// ---- the object table and the resource manager -----------------------------------------------------------
struct obj_tbl {
    mesh* make_mesh_from(const int* def, const int* troop); // 0x4991d0
};
extern obj_tbl g_obj;                                       // 0xa75f40

struct resmgr {
    mesh* get_object(const tstring& n);                     // 0x419540 (ret 4)
};
extern resmgr g_resmgr;                                     // 0x5d6fa0

// ---- the global animator ---------------------------------------------------------------------------------
struct Animator {
    char pad0[8];
    int cur;                                                // +0x08 (0x8ed4f68)
    void set_anim_a(void* rec);                             // 0x473800
    void collect(int flag, void** a, void** b, void** c);   // 0x47fb10
};
extern Animator g_anim;                                     // 0x8ed4f60
extern int g_char_count;                                    // 0x8ed4f88
int gen_face_seed(int* skin, int* flags, int* count);       // 0x432510

// ---- the game singleton ----------------------------------------------------------------------------------
struct Game {
    char pad0[0x13a00];
    char* chars;                                            // +0x13a00 (troop records, stride 0x508)
    char pad13a04[0x13b08 - 0x13a04];
    int f13b08;                                             // +0x13b08
    int f13b0c;                                             // +0x13b0c
};
extern Game* g_game;                                        // 0x5e2930
extern float g_fov_k;                                       // 0x5e20a8
extern int g_a181ac;                                        // 0xa181ac
extern int g_a75f64;                                        // 0xa75f64

void free_535712(void* p);                                  // 0x535712 (operator delete)

// ---- the character-sheet window --------------------------------------------------------------------------
struct char_window {
    char pad0[0x14];
    int  troop;                                             // +0x14 (troop record index)
    char pad18[0x2c - 0x18];
    widget* ui;                                             // +0x2c
    char pad30[0x2dc - 0x30];
    int  f2dc;                                              // +0x2dc (-1 until the portrait is built)
    char pad2e0[0x2e4 - 0x2e0];
    World* world;                                           // +0x2e4
    char pad2e8[0x2ec - 0x2e8];
    eng_frustum fr;                                         // +0x2ec (0x120; the frame sits at +0xc8)
    float rect[4];                                          // +0x40c
    float f41c;                                             // +0x41c
    float f420;                                             // +0x420
    float f424;                                             // +0x424
    char pad428[0x4cc - 0x428];
    elem2*  f4cc;                                           // +0x4cc
    widget* f4d0;                                           // +0x4d0
    void open_view();                                       // 0x509980
};

// @ 0x509980 -----------------------------------------------------------------------------------------------
void char_window::open_view()
{
    f4cc = 0;
    f4d0 = 0;
    int mode = g_a75f64;
    if (g_a75f64 < 0) {
        // ---- 3D portrait: build the body, the world, the wind tracks and the camera ----
        face_body* b = new face_body;
        elem_fill(b, tstring("char_i_character"));
        if (world != 0) {
            world->destroy();
            free_535712(world);
            world = 0;
        }
        float lo[2];
        lo[0] = -80.0f;
        lo[1] = -80.0f;
        float hi[2];
        hi[0] = 80.0f;
        hi[1] = 80.0f;
        int zero = 0;
        int zero2 = 0;
        int zero3 = 0;
        World* w = new World(lo, hi, &zero, &zero2, &zero3);
        world = w;
        w->color.x = 0.23f;
        w->color.y = 0.25f;
        w->color.z = 0.27f;
        w->color.w = 1.0f;
        {
            int   z2 = 0;
            float five = 5.0f;
            int   col = (int)0xff221815;
            w->set_visual(&z2, &five, &col);
        }
        {
            wind* wd = new wind;
            w->tracks.push_back((int)wd);
            wd->v04 = rgl::vector4(0.1f, 0.15f, 0.2f);
            wd->v14 = rgl::vector4(0.1f, 0.15f, 0.2f);
            wd->v24 = rgl::vector4(-2.0f, -2.0f, 12.0f);
            wd->type = 2;
            wd->f54 = 20.0f;
            wd->v44 = rgl::vector4(-0.5f, 0.0f, -1.0f);
        }
        {
            wind* wd = new wind;
            w->tracks.push_back((int)wd);
            wd->v04 = rgl::vector4(0.45f, 0.42f, 0.4f);
            wd->v14 = rgl::vector4(0.4f, 0.4f, 0.4f);
            wd->v24 = rgl::vector4(2.0f, 2.0f, 2.0f);
            wd->type = 2;
            wd->f54 = 20.0f;
            wd->v44 = rgl::vector4(0.6f, -1.0f, -2.0f);
        }
        b->world = w;
        b->flags |= 0x1200;
        b->color = 0xff221815;
        // the camera frame: the body's aspect scaled by the global fov factor
        float aspect = b->pey / b->pex * g_fov_k;
        fr.init(0.2617994f, aspect, 0.01f, 20.0f);
        frame mat;
        identity_pos(&mat);
        // identity_pos (0x40cd00) has its worker 0x40cd60 copy its OWN stack locals into the matrix' w
        // lanes, so u.w (and s.w/f.w) hold whatever the deepest previous callee left on the stack rather
        // than 0.  In the original build frustum::init's prologue spilled the 0x1200 flags value into that
        // slot, and build_planes below propagates it into the frustum, so the two builds only agree if we
        // pin the same leftover.  This is an uninitialised-read artifact of the original, not a value the
        // source computes (the identity matrix' w lanes are meant to be 0).
        *(unsigned*)&mat.u.w = 0x1200u;
        mat.o.x += mat.u.x * 1.2f;
        mat.o.y += mat.u.y * 1.2f;
        mat.o.z += mat.u.z * 1.2f;
        *(frame*)((char*)this + 0x3b4) = mat;
        fr.build_planes();
        rect[0] = 0.0f;
        rect[1] = 0.0f;
        rect[2] = 0.0f;
        rect[3] = 0.0f;
        f41c = -0.3f;
        f420 = -0.09f;
        f424 = 0.7f;
        b->view = fr;
        Entity* e = w->create_entity(9);
        frame m2;
        identity_pos(&m2);
        m2.o.y = -0.12f;
        float ang = -0.2f;
        m2.rotate_su(&ang);
        ang = 0.1f;
        m2.rotate_fu(&ang);
        e->set_matrix(&m2);
        ui->add_child(b);
        if (f2dc == -1) {
            // seed the body's face from the troop record and start the idle animation
            char* rec = g_game->chars + troop * 0x508;
            int skin = *(int*)(rec + 0x30) & 0xf;
            int flags = 0;
            g_anim.cur = gen_face_seed(&skin, &flags, &g_char_count);
            g_anim.set_anim_a(rec + 0x3c0);
            void* ra = 0;
            void* rb = 0;
            void* rc = 0;
            g_anim.collect(g_a181ac != 0, &ra, &rb, &rc);
            e->on_anim_end();
            ResList* rl = (ResList*)((char*)e + 0x108);
            rl->release(&ra);
            if (rc)
                rl->release(&rc);
            if (rb)
                rl->release(&rb);
        }
    } else {
        // ---- flat fallback: a shadow image plus the empty character icon ----
        if (f2dc == -1) {
            widget* a = new widget;
            f4d0 = a;
            elem_fill(a, tstring("char_i_character_shadow"));
            {
                tstring nm("character_window_shadow");
                mesh* m = g_resmgr.get_object(nm);
                m = m->clone();
                a->add_image(0, m);
            }
            ui->add_child(a);
            elem2* c = new elem2;
            *(void**)c = (void*)g_vt_591ee8;
            f4cc = c;
            elem_fill(c, tstring("char_i_character"));
            mesh* m2 = g_obj.make_mesh_from(&mode, &troop);
            c->add_image2(0, m2);
            ui->add_child(c);
        }
    }
}

// ---- the portrait view state (a different class from char_window: +0x7c/+0x8c are heap buffers, not
//      tstrings) --------------------------------------------------------------------------------------------
struct view_obj {
    char pad0[0x7c];
    int*  p7c;                                              // +0x7c (heap buffer)
    int   f80;                                              // +0x80
    int   f84;                                              // +0x84
    char  pad88[0x8c - 0x88];
    int*  p8c;                                              // +0x8c (heap buffer)
    int   f90;                                              // +0x90
    int   f94;                                              // +0x94
    char  pad98[0xa8 - 0x98];
    int   fa8;                                              // +0xa8
    int   fac;                                              // +0xac
    float fb0;                                              // +0xb0
    void  sub_50a1c0(int a, int b);                         // 0x50a1c0
    void  reset();                                          // 0x50a140
};

static void clear_buffer(int*& data, int& a, int& b)
{
    if (data != 0)
        delete data;
    data = 0;
    a = 0;
    b = 0;
}

// @ 0x50a140 -----------------------------------------------------------------------------------------------
void view_obj::reset()
{
    fa8 = -1;
    fac = -1;
    fb0 = -1.0f;
    clear_buffer(p7c, f80, f84);
    clear_buffer(p8c, f90, f94);
    sub_50a1c0(g_game->f13b08, g_game->f13b0c);
}
