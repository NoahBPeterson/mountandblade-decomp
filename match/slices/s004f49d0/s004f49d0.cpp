// s004f49d0 (ds1 slice 190): the "face_gen_window" construction (face generator menu).
// Provisional names; every offset comes from the disassembly of 0x4f49d0..0x4f5d17.
#include <windows.h>
#include <new>
#include <math.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;        // 0x42bc50
    void assign(const char* s);                         // 0x42bcb0
    bool equals(const tstring& o) const;                // 0x42bdd0
};

// ---- widgets ---------------------------------------------------------------------------------------------
struct mesh;                                            // 0x260 bytes
mesh* mesh_clone(mesh* m);                              // 0x41c6d0
struct Entity;

// the UI element base (0x178 bytes, ctor 0x48ae20)
struct widget {
    widget() MB_NOTHROW;                                // 0x48ae20
    char pad0[0x85];
    unsigned char dirty;                                // +0x85
    char pad86[0xe8 - 0x86];
    float ex;                                           // +0xe8 (text/widget flavour)
    float ey;                                           // +0xec
    float pex;                                          // +0xf0 (panel flavour)
    float pey;                                          // +0xf4
    char padf8[0x178 - 0xf8];
    virtual void v00();                                 // +0x00
    virtual void v04();                                 // +0x04
    virtual void v08();                                 // +0x08
    virtual void v0c();                                 // +0x0c
    virtual void v10();                                 // +0x10
    virtual void v14();                                 // +0x14
    virtual void v18();                                 // +0x18
    virtual void v1c();                                 // +0x1c
    virtual void v20();                                 // +0x20
    virtual void v24();                                 // +0x24
    virtual void v28();                                 // +0x28
    virtual void v2c();                                 // +0x2c
    virtual void v30();                                 // +0x30
    virtual void v34(float v);                          // +0x34
    virtual void v38();                                 // +0x38
    virtual void v3c();                                 // +0x3c
    virtual void v40();                                 // +0x40
    virtual void v44();                                 // +0x44
    virtual void v48();                                 // +0x48
    virtual void v4c();                                 // +0x4c
    virtual void v50();                                 // +0x50
    virtual void v54();                                 // +0x54
    virtual void v58(const float* ab);                  // +0x58
    virtual void v5c();                                 // +0x5c
    virtual void v60();                                 // +0x60
    virtual void v64(const float* ab);                  // +0x64
    virtual void v68();                                 // +0x68
    virtual void v6c(const float* ab);                  // +0x6c
    virtual void v70();                                 // +0x70
    virtual void v74();                                 // +0x74
    virtual void v78();                                 // +0x78
    virtual void v7c(mesh* m);                          // +0x7c
    virtual void v80(int n, mesh* m);                   // +0x80
    virtual void v84();                                 // +0x84
    virtual void v88();                                 // +0x88
    virtual void v8c();                                 // +0x8c
    virtual void v90();                                 // +0x90
    virtual void v94();                                 // +0x94
    virtual void v98(const int* v);                     // +0x98
    virtual void v9c();                                 // +0x9c
    virtual void va0();                                 // +0xa0
    virtual void va4(tstring* out, tstring* s);         // +0xa4
    virtual void va8();                                 // +0xa8
    virtual void vac();                                 // +0xac
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    virtual void vb8();                                 // +0xb8
    virtual void vbc();                                 // +0xbc
    virtual void vc0();                                 // +0xc0
    virtual void vc4();                                 // +0xc4
    virtual void vc8(widget* o);                        // +0xc8
};

// engine vector of UI elements: {lead, first, last, end}
struct objvec {
    int lead;                                           // +0x00
    widget** first;                                     // +0x04
    widget** last;                                      // +0x08
    widget** end;                                       // +0x0c
    void insert_n(widget** where, unsigned n, widget* const& val);   // 0x4927d0
};

// the 0x188-byte UI element (ctor 0x4931a0; "deform"/"buttons" vptr 0x597050)
struct elem : widget {
    elem() MB_NOTHROW;                                  // 0x4931a0
    objvec items;                                       // +0x178
};

// the 0x190-byte overlay window (vptr 0x592848, ctor 0x4931a0)
struct overlay : elem {
    unsigned char f188;                                 // +0x188
    char pad189[0x190 - 0x189];
};

// the engine view frustum (0x120 bytes)
struct eng_frustum {
    unsigned char on;                                   // +0x000
    char pad1[0xf8 - 1];
    float f_f8[4];                                      // +0x0f8
    float f108;                                         // +0x108
    float f10c;                                         // +0x10c
    float f110;                                         // +0x110
    float f114;                                         // +0x114
    float f118;                                         // +0x118
    float f11c;                                         // +0x11c
    void build_planes();                                // 0x41e4f0
};

struct World;

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + frustum copy
struct face_body : widget {
    World* world;                                       // +0x178
    char pad17c[0x1ac - 0x17c];
    eng_frustum view;                                   // +0x1ac (0x120 bytes)
    char pad2cc[0x308 - 0x2cc];
    unsigned flags;                                     // +0x308
    unsigned color;                                     // +0x30c
    char pad310[0x358 - 0x310];
    face_body() MB_NOTHROW;                             // 0x4ed630
};

// the face-generator window, ctor 0x4f49d0
struct FaceGen {
    char pad0[0x14];
    int troop;                                          // +0x14
    char pad18[0x2c - 0x18];
    overlay* ui;                                        // +0x2c
    face_body* body;                                    // +0x30
    eng_frustum fr;                                     // +0x34 (0x120 bytes)
    float v154[4];                                      // +0x154
    float f164;                                         // +0x164
    float f168;                                         // +0x168
    float f16c;                                         // +0x16c
    unsigned char f170;                                 // +0x170
    char pad171[0x174 - 0x171];
    int skin;                                           // +0x174
    World* world;                                       // +0x178
    elem* slider;                                       // +0x17c
    elem* deform;                                       // +0x180
    elem* buttons;                                      // +0x184
    elem* cam_btn;                                      // +0x188
    elem* deform_rows[0x1c];                            // +0x18c (one row per deform key)
    widget* key_labels[0x1c];                           // +0x24c
    char pad2bc[0x30c - 0x2bc];
    widget* b_next_beard;                               // +0x30c
    widget* b_next_hair;                                // +0x310
    widget* b_next_face;                                // +0x314
    widget* b_prev_beard;                               // +0x318
    widget* b_prev_hair;                                // +0x31c
    widget* b_prev_face;                                // +0x320
    widget* t_change_beard;                             // +0x324
    widget* t_change_hair;                              // +0x328
    widget* t_change_face;                              // +0x32c
    elem* sl_age;                                       // +0x330
    elem* sl_hair_color;                                // +0x334
    widget* t_age;                                      // +0x338
    widget* t_hair_color;                               // +0x33c
    widget* b_done;                                     // +0x340
    widget* b_reset;                                    // +0x344
    widget* b_randomize;                                // +0x348
    Entity* ent;                                        // +0x34c
    int f350;                                           // +0x350
    int f354;                                           // +0x354
    int f358;                                           // +0x358
    int f35c;                                           // +0x35c
    float f360;                                         // +0x360
    float f364;                                         // +0x364
    void init();                                        // 0x4f49d0
    void refresh();                                     // 0x4eb3c0
};

// ---- engine vector of ints (lead dword, then first/last/end) ---------------------------------------------
template <class T>
struct engvec {
    int lead;                                           // +0x00
    T* first;                                           // +0x04
    T* last;                                            // +0x08
    T* end;                                             // +0x0c
    void insert_n(T* where, unsigned n, T const& val);  // 0x40f8a0
    void push_back(T const& v) {
        if (first == 0 || (unsigned)(last - first) >= (unsigned)(end - first))
            insert_n(last, 1, v);
        else
            *last++ = v;
    }
};

// ---- globals ---------------------------------------------------------------------------------------------
struct MsgLog { void reset(); };                        // 0x42d1b0
extern MsgLog g_msglog;                                 // 0x5d7080
extern unsigned char g_msg_tick;                        // 0x5d8788
struct gui_mgr { void set_window(widget** w); };        // 0x49b120
extern gui_mgr g_gui;                                   // 0x5d7080
struct resmgr { mesh* get_object(const tstring& n); };  // 0x419540
extern resmgr g_alloc;                                  // 0x5d6fa0
struct StrTable { tstring* lookup(const tstring* key); };   // 0x43a1e0
extern StrTable g_strtable;                             // 0x5e2a18
struct config_entry { int f0; char* value; char pad[0x2c - 8]; };
struct ini_file {
    config_entry entries[0x1000];
    int count;                                          // +0x2c000
    int find(const tstring& key);                       // 0x4bd5a0
};
extern ini_file g_config;                               // 0xa44240
#define CONFIG_VALUES ((char**)0xa4425c)
extern "C" double atof(const char* s);                  // 0x537352

struct SkinSet { char pad0[0x3c]; int n1; char pad40[0x40]; char* keys; char pad44[0x88 - 0x44];
                 char* first_face; };
extern SkinSet* g_skins_begin;                          // 0x8ed4f7c
struct Game { char pad0[0x13a00]; char* chars; char pad13a04[0x1a090 - 0x13a04]; int f1a090; };
extern Game* g_game;                                    // 0x5e2930
extern int g_no_cam;                                    // 0x5a9fb0

struct Animator {
    char pad0[8];
    int cur;                                            // +0x8
    void set_anim_a(void* a);                           // 0x473800
};
extern Animator g_anim;                                 // 0x8ed4f60
extern float g_dummy_mat;                               // 0x8ed4f8c
extern float g_dummy0;                                  // 0x8ed5088
extern float g_dummy1;                                  // 0x8ed508c
extern int g_char_count;                                // 0x8ed4f88
int gen_face_seed(int* skin, int* flags, int* count);   // 0x432510
extern float g_k0_01;                                   // 0x54a6f0 = 0.01f
extern float g_fov_k;                                   // 0x5e20a8
extern const double g_tan_ang;                          // 0x598e90

// ---- callees ---------------------------------------------------------------------------------------------
void elem_fill(widget* w, tstring name);                // 0x4e4c70
elem* new_label2(widget* p, tstring* n);                // 0x4f5fb0
elem* new_scrollbar(widget* p, tstring* n);             // 0x4f6160
elem* new_slider(widget* p, tstring* n, elem* sl);      // 0x4f62f0
void list_setup(void* s);                               // 0x488d50
widget* new_key_label(widget* p, tstring* key, tstring* val);                   // 0x4f5d20
widget* new_arrow(widget* p, int a, int b, float c, float d, float e);          // 0x4f5ed0
widget* new_arrow_l(widget* p, int a, int b, float c, float d, float e);        // 0x4f5df0
void anim_track(Animator* a, int skin, int key, tstring* out);                  // 0x4f5d90
widget* new_medium_button(widget* p, tstring* label, int a, int b, tstring* n1, tstring* n2,
                          tstring* n3, float f1, float f2, float f3);           // 0x4dcc50

// the 0x344-byte world (ctor 0x478ad0)
struct World {
    char pad0[0x68];
    float f68;                                          // +0x68
    float f6c;                                          // +0x6c
    float f70;                                          // +0x70
    float f74;                                          // +0x74
    char pad78[0xb8 - 0x78];
    engvec<int> tracks;                                 // +0xb8 (first at +0xbc)
    char padc8[0x344 - 0xc8];
    World(const float* lo, const float* hi, const int* a, const int* b, const int* c);   // 0x478ad0
    Entity* create_entity(int type);                    // 0x428440
};

// the 0x7c-byte wind track (ctor 0x461480)
struct wind {
    int type;                                           // +0x00
    float v04[4];                                       // +0x04
    float v14[4];                                       // +0x14
    float v24[4];                                       // +0x24
    char pad34[0x44 - 0x34];
    float v44[4];                                       // +0x44
    float f54;                                          // +0x54
    char pad58[0x7c - 0x58];
    wind() MB_NOTHROW;                                  // 0x461480
};

static void push_item(elem* w, widget* o)
{
    if (w->items.first == 0 ||
        (unsigned)(w->items.last - w->items.first) >= (unsigned)(w->items.end - w->items.first))
        w->items.insert_n(w->items.last, 1, o);
    else
        *w->items.last++ = o;
}

static float config_float(const char* key)
{
    float f = 0.0f;
    tstring t(key);
    int idx = g_config.find(t);
    if (idx >= 0) {
        char* p = CONFIG_VALUES[idx * 0xb];
        f = (float)atof(p ? p : (char*)&CONFIG_VALUES[idx * 0xb]);
    }
    return f;
}

// @ 0x4f49d0
void FaceGen::init()
{
    f360 = 0.5f;
    f364 = 0.5f;
    f354 = 0;
    f350 = 0;
    f358 = 0;
    f35c = 0;
    skin = 0;
    if (troop >= 0)
        skin = *(int*)(g_game->chars + troop * 0x508 + 0x30) & 0xf;
    int keys = g_skins_begin[skin].n1 - 1;
    g_msglog.reset();
    g_game->f1a090 = 0;
    overlay* o = new overlay;
    if (o != 0)
        o->f188 = 0;
    ui = o;
    float f2[2];
    f2[0] = 0.0f;
    f2[1] = 0.0f;
    o->v58(f2);
    f2[0] = 1.0f;
    f2[1] = 0.75f;
    o->v64(f2);
    {
        tstring n("face_gen_window");
        mesh* m = g_alloc.get_object(n);
        mesh* c = mesh_clone(m);
        o->v80(0, c);
    }
    {
        widget* w = ui;
        g_msg_tick = 1;
        g_gui.set_window(&w);
    }
    float deform_k = config_float("face_deform_key_h");
    float ab[2];
    {
        elem* d = new elem;
        deform = d;
        elem_fill(d, tstring("face_bo_deform"));
        ab[0] = d->pex;
        ab[1] = (float)keys * deform_k + d->pey;
        d->v64(ab);
    }
    {
        elem* b = new elem;
        buttons = b;
        elem_fill(b, tstring("face_bo_button"));
        push_item(ui, b);
    }
    tstring btn_text;
    {
        tstring empty("");
        buttons->va4(&btn_text, &empty);
    }
    for (int i = 0; i < keys; i++) {
        tstring row_name("face_sl_deform_key_base");
        elem* row = new_label2(deform, &row_name);
        deform_rows[i] = row;
        float y = (float)i * deform_k + row->ey;
        ab[0] = row->ex;
        ab[1] = y;
        row->v58(ab);
        tstring kt;
        anim_track(&g_anim, skin, i, &kt);
        widget* lab = new widget;
        key_labels[i] = lab;
        elem_fill(lab, tstring("face_t_deform_key_base"));
        ab[0] = lab->ex;
        ab[1] = y + lab->ey;
        lab->v58(ab);
        lab->va4(&btn_text, &kt);
        deform->vc8(lab);
    }
    {
        tstring t("face_s_deform");
        elem* sc = new_scrollbar(ui, &t);
        tstring t2("face_sc_deform");
        elem* sl = new_slider(deform, &t2, sc);
        slider = sl;
        list_setup(sl);
        push_item(ui, sl);
    }
    cam_btn = 0;
    if (g_no_cam) {
        tstring t("key");
        tstring a("medium_button_down");
        tstring b2("medium_button");
        tstring t1("");
        cam_btn = (elem*)new_medium_button(ui, &t, 0, 0, &a, &b2, &t1, 0.014f, 0.5f, 0.72f);
        cam_btn->vb0();
    }
    float arrow_h = config_float("face_button_h");
    (void)arrow_h;
    float arrow_h2 = config_float("face_button_arrow_h");
    float arrow_w = config_float("arrow_button_height");
    float arrow_r = config_float("arrow_button_width_ratio");
    b_next_face = new_arrow(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_next_face, tstring("face_b_next_face"));
    b_next_beard = new_arrow(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_next_beard, tstring("face_b_next_beard"));
    b_next_hair = new_arrow(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_next_hair, tstring("face_b_next_hair"));
    b_prev_face = new_arrow_l(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_prev_face, tstring("face_b_prev_face"));
    b_prev_beard = new_arrow_l(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_prev_beard, tstring("face_b_prev_beard"));
    b_prev_hair = new_arrow_l(buttons, 0, 0, arrow_h2, arrow_r, arrow_w);
    elem_fill(b_prev_hair, tstring("face_b_prev_hair"));
    {
        tstring t("face_t_change_beard");
        tstring v("ui_change_beard");
        tstring* kv = g_strtable.lookup(&v);
        t_change_beard = new_key_label(buttons, kv, &t);
    }
    {
        tstring t("face_t_change_face");
        tstring v("ui_change_skin");
        tstring* kv = g_strtable.lookup(&v);
        t_change_face = new_key_label(buttons, kv, &t);
    }
    {
        tstring t("face_t_change_hair");
        tstring v("ui_change_hair");
        tstring* kv = g_strtable.lookup(&v);
        t_change_hair = new_key_label(buttons, kv, &t);
    }
    {
        tstring n("face_sl_age");
        sl_age = new_label2(buttons, &n);
    }
    {
        tstring t("face_t_age");
        tstring v("ui_age");
        tstring* kv = g_strtable.lookup(&v);
        t_age = new_key_label(buttons, kv, &t);
    }
    {
        tstring n("face_sl_hair_color");
        sl_hair_color = new_label2(buttons, &n);
    }
    {
        tstring t("face_t_hair_color");
        tstring v("ui_change_hair_color");
        tstring* kv = g_strtable.lookup(&v);
        t_hair_color = new_key_label(buttons, kv, &t);
    }
    if (g_skins_begin[skin].first_face == 0) {
        b_next_beard->vb0();
        b_prev_beard->vb0();
        t_change_beard->vb0();
    }
    {
        tstring v("ui_randomize");
        tstring* kv = g_strtable.lookup(&v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = new_medium_button(buttons, kv, 0, 0, &a, &b2, &btn_text, 0.014f, 0.72f, 3.7f);
        b_randomize = btn;
        elem_fill(btn, tstring("face_b_randomize"));
    }
    {
        tstring v("ui_reset");
        tstring* kv = g_strtable.lookup(&v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = new_medium_button(buttons, kv, 0, 0, &a, &b2, &btn_text, 0.014f, 0.72f, 3.7f);
        b_reset = btn;
        elem_fill(btn, tstring("face_b_reset"));
    }
    {
        tstring v("ui_done");
        tstring* kv = g_strtable.lookup(&v);
        tstring a("medium_button_down");
        tstring b2("medium_button");
        widget* btn = new_medium_button(buttons, kv, 0, 0, &a, &b2, &btn_text, 0.014f, 0.72f, 3.7f);
        b_done = btn;
        elem_fill(btn, tstring("face_b_done"));
    }

    // ---- face body, world and wind tracks ----------------------------------------------------------
    face_body* b = new face_body;
    body = b;
    float lo[4];
    lo[0] = -80.0f;
    lo[1] = -80.0f;
    lo[2] = 0.0f;
    lo[3] = 0.0f;
    float hi[4];
    hi[0] = 80.0f;
    hi[1] = 80.0f;
    hi[2] = 0.0f;
    hi[3] = 0.0f;
    int zero = 0;
    World* w = new World(lo, hi, &zero, &zero, &zero);
    world = w;
    w->f68 = 0.3f;
    w->f6c = 0.3f;
    w->f70 = 0.3f;
    w->f74 = 1.0f;
    {
        wind* wd = new wind;
        w->tracks.push_back((int)wd);
        wd->v04[0] = 0.1f;
        wd->v04[1] = 0.15f;
        wd->v04[2] = 0.2f;
        wd->v04[3] = 0.0f;
        wd->v14[0] = 0.2f;
        wd->v14[1] = 0.2f;
        wd->v14[2] = 0.2f;
        wd->v14[3] = 0.0f;
        wd->v24[0] = -2.0f;
        wd->v24[1] = -2.0f;
        wd->v24[2] = 12.0f;
        wd->v24[3] = 0.0f;
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44[0] = -0.5f;
        wd->v44[1] = 0.0f;
        wd->v44[2] = -1.0f;
        wd->v44[3] = 0.0f;
    }
    {
        wind* wd = new wind;
        w->tracks.push_back((int)wd);
        wd->v04[0] = 1.0f;
        wd->v04[1] = 1.0f;
        wd->v04[2] = 1.0f;
        wd->v04[3] = 0.0f;
        wd->v14[0] = 1.0f;
        wd->v14[1] = 1.0f;
        wd->v14[2] = 1.0f;
        wd->v14[3] = 0.0f;
        wd->v24[0] = -2.0f;
        wd->v24[1] = -2.0f;
        wd->v24[2] = 2.0f;
        wd->v24[3] = 0.0f;
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44[0] = 1.0f;
        wd->v44[1] = 1.0f;
        wd->v44[2] = -1.0f;
        wd->v44[3] = 0.0f;
    }
    ent = w->create_entity(9);
    b->world = w;

    float p2[2];
    p2[0] = 0.0513f;
    p2[1] = 0.2669f;
    b->v58(p2);
    float s2[2];
    s2[0] = 0.4585f;
    s2[1] = 0.44f;
    b->v64(s2);
    b->flags |= 0x300;
    b->color = 0xff201815;

    // ---- the camera frustum (16:9 fit around the body) ---------------------------------------------
    float ratio = b->pey / b->pex * g_fov_k;
    float tanv = (float)tan(g_tan_ang);
    fr.on = 1;
    fr.f118 = 0.01f;
    fr.f11c = 20.0f;
    fr.f108 = -(0.01f * tanv);
    fr.f10c = 0.01f * tanv;
    fr.f114 = -(ratio * (0.01f * tanv));
    fr.f110 = ratio * (0.01f * tanv);
    fr.build_planes();

    push_item(ui, b);

    fr.f_f8[0] = 0.0f;
    fr.f_f8[1] = 0.0f;
    fr.f_f8[2] = 10.0f;
    fr.f_f8[3] = 0.0f;
    v154[0] = 0.0f;
    v154[1] = 0.0f;
    v154[2] = 0.0f;
    v154[3] = 0.0f;
    f164 = -0.3f;
    f168 = -0.05f;
    f16c = 0.7f;
    f170 = 0;
    b->view = fr;

    // ---- hook the face up to the troop's anim table ------------------------------------------------
    if (troop >= 0) {
        char* rec = g_game->chars + troop * 0x508;
        int sk = *(int*)(rec + 0x30) & 0xf;
        int flags = 0;
        g_anim.cur = gen_face_seed(&sk, &flags, &g_char_count);
        g_anim.set_anim_a(rec + 0x3c0);
        f354 = *(int*)((char*)&g_anim + 0x04);
        f350 = *(int*)((char*)&g_anim + 0x00);
        f35c = *(int*)((char*)&g_anim + 0x14);
        f358 = *(int*)((char*)&g_anim + 0x0c);
        f360 = g_dummy0;
        f364 = g_dummy1;
        f354++;
        f350++;
        for (int i = 0; i < keys; i++)
            deform_rows[i]->v34((&g_dummy_mat)[i]);
        sl_age->v34(g_dummy0);
        sl_hair_color->v34(g_dummy1);
    }
    refresh();
}
