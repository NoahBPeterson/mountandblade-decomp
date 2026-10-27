// facegen_init: FaceGen::init (0x4f49d0), the "face_gen_window" construction.
// Everything below is modelled from the disassembly of 0x4f49d0..0x4f5d17; names are provisional.
#include <windows.h>
#include <stdlib.h>
#include <new>
#include <math.h>
#include <vector>
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fptan, fsin, fcos)
#include "rgl.h"

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) --------------------------------------------------------------------------
struct tstring {
    int pos;                                        // +0x00
    char* buf;                                      // +0x04
    int cap;                                        // +0x08
    int len;                                        // +0x0c
    tstring() throw();                              // 0x42bbd0
    tstring(const char* s) throw();                 // 0x42bbe0
    tstring(const tstring& o) throw();              // 0x42c110
    ~tstring() throw();                             // 0x42bc40
    bool equals(const tstring& o) const;            // 0x42bdd0
};

// ---- widgets -------------------------------------------------------------------------------------------
struct WRet { int a, b; WRet(); };                  // 8-byte non-POD result of set_text
struct MissionUi {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30();
    virtual void set_value(float v);                // +0x34
    virtual void v38(); virtual void v3c(); virtual void v40(); virtual void v44();
    virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void set_pos(const float* p);           // +0x58
    virtual void v5c(); virtual void v60();
    virtual void set_size(const float* p);          // +0x64
    virtual void v68(); virtual void v6c(); virtual void v70(); virtual void v74();
    virtual void v78(); virtual void v7c();
    virtual void add_image(int slot, void* m);      // +0x80
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void v94(); virtual void v98(); virtual void v9c(); virtual void va0();
    virtual WRet set_text(const tstring& s);        // +0xa4
    virtual void va8(); virtual void vac();
    virtual void v_b0();                            // +0xb0
    virtual void vb4(); virtual void vb8(); virtual void vbc(); virtual void vc0();
    virtual void vc4();
    virtual void add_child(void* c);                // +0xc8

    char pad[0xe8 - 4];
    float x;                                        // +0xe8
    float y;                                        // +0xec
    float w;                                        // +0xf0
    float h;                                        // +0xf4
    char pad2[0x178 - 0xf8];
    std::vector<MissionUi*> children;               // +0x178
};

// the raw (non-virtual) storage classes: the base ctor runs out of line and the caller stores the vptr
void* __fastcall ctor_4931a0(void* p);              // 0x4931a0
void* __fastcall ctor_48ae20(void* p);              // 0x48ae20

struct Overlay {                                    // 0x190, vptr 0x592848
    char pad0[0x188];
    char f188;                                      // +0x188
    char pad189[0x190 - 0x189];
    Overlay() { ctor_4931a0(this); *(void**)this = (void*)0x592848; f188 = 0; }
};

struct Elem {                                       // 0x188, vptr 0x597050
    char pad0[0x188];
    Elem() { ctor_4931a0(this); *(void**)this = (void*)0x597050; }
};

struct Label {                                      // 0x178 (base widget only)
    char pad0[0x178];
    Label() { ctor_48ae20(this); }
};

// ---- the world (0x344 bytes) and its wind tracks (0x7c) -------------------------------------------------
struct Entity;
struct World {
    char pad0[0x68];
    rgl::vector4 color;                             // +0x68
    char pad78[0xb8 - 0x78];
    std::vector<int> tracks;                        // +0xb8
    char padc8[0x344 - 0xc8];
    World(const float* lo, const float* hi, const int* a, const int* b, const int* c);   // 0x478ad0
    Entity* create_entity(int type);                // 0x428440
};

void* __fastcall ctor_461480(void* p);              // 0x461480

struct wind {
    int type;                                       // +0x00
    rgl::vector4 v04;                               // +0x04
    rgl::vector4 v14;                               // +0x14
    rgl::vector4 v24;                               // +0x24
    char pad34[0x10];
    rgl::vector4 v44;                               // +0x44
    float f54;                                      // +0x54
    char pad58[0x7c - 0x58];
    wind() { ctor_461480(this); }                   // 0x461480
};

// ---- the view frustum (0x120) --------------------------------------------------------------------------
struct Frustum {
    unsigned char perspective;                      // +0x000
    unsigned char left_handed;                      // +0x001
    char pad2[2];
    int nplanes;                                    // +0x004
    char planes[0xc0];                              // +0x008
    rgl::matrix m;                                  // +0x0c8
    float left;                                     // +0x108
    float right;                                    // +0x10c
    float top;                                      // +0x110
    float bottom;                                   // +0x114
    float nearz;                                    // +0x118
    float farz;                                     // +0x11c
    void build_planes();                            // 0x41e4f0
};

// ---- the camera body (0x358) ---------------------------------------------------------------------------
void* __fastcall ctor_4ed630(void* p);              // 0x4ed630

struct MapWidget {
    char pad0e8[0xe8];
    float x;                                        // +0xe8
    float y;                                        // +0xec
    float w;                                        // +0xf0
    float h;                                        // +0xf4
    char pad0f8[0x178 - 0xf8];
    World* world;                                   // +0x178
    char pad17c[0x1ac - 0x17c];
    Frustum view;                                   // +0x1ac
    char pad2cc[0x308 - 0x2cc];
    unsigned flags;                                 // +0x308
    unsigned color;                                 // +0x30c
    char pad310[0x358 - 0x310];
    MapWidget() { ctor_4ed630(this); }              // 0x4ed630
};

// ---- ini table -----------------------------------------------------------------------------------------
struct CfgFile { int find(const tstring& key); };   // 0x4bd5a0
struct cfg_entry {
    tstring key;                                    // +0x00
    char pad10[0x10];
    char* value;                                    // +0x20
    char pad24[0x2c - 0x24];
};
extern int g_cfg_count;                             // 0xa70240
#define g_config (*(CfgFile*)0xa44240)
#define CFG_ENTRIES ((cfg_entry*)0xa44240)

// ---- globals -------------------------------------------------------------------------------------------
struct MsgLog { void reset(); };                    // 0x42d1b0
struct GuiMgr {
    void reset();                                   // 0x42d1b0
    void add_root(void* const& p);                  // 0x49b120
};
extern GuiMgr g_gui;                                // 0x5d7080
extern unsigned char g_5d8788;                      // 0x5d8788

struct mesh;
struct resmgr {
    void* get_object(const tstring& n);             // 0x419540
};
extern resmgr g_alloc;                              // 0x5d6fa0
void* __fastcall mesh_clone(void* m);               // 0x41c6d0

struct StrTable {
    tstring* lookup(const tstring& key);            // 0x43a1e0
};
extern StrTable g_strtab;                           // 0x5e2a18

struct Game {
    char pad0[0x13a00];
    char* chars;                                    // +0x13a00
    char pad13a04[0x1a090 - 0x13a04];
    int f1a090;                                     // +0x1a090
};
extern Game* g_game;                                // 0x5e2930

struct SkinSet {
    char pad0[0x3c];
    int n_keys;                                     // +0x3c
    char pad40[0x88 - 0x40];
    int first_face;                                 // +0x88
    char pad8c[0x1f8 - 0x8c];
};
extern SkinSet* g_skins_begin;                      // 0x8ed4f7c

struct Animator {
    int f00;                                        // +0x00
    int f04;                                        // +0x04
    int cur;                                        // +0x08
    int f0c;                                        // +0x0c
    int f14;                                        // +0x14
    void unpack(void* genome);                      // 0x473800
};
extern Animator g_anim;                             // 0x8ed4f60
extern int g_char_count;                            // 0x8ed4f88
extern float g_dummy_mat;                           // 0x8ed4f8c
extern float g_dummy0;                              // 0x8ed5088
extern float g_dummy1;                              // 0x8ed508c
int __cdecl gen_face_seed(int* skin, int* zero, int* count);   // 0x432510

struct TextPart {
    void get_text(int row, int index, tstring* out);    // 0x4f5d90
};
extern int g_no_cam;                                // 0x5a9fb0
extern float g_fov_k;                               // 0x5e20a8
extern float g_0_01;                                // 0x54a6f0
extern const double g_tan_ang;                      // 0x598e90

// ---- callees -------------------------------------------------------------------------------------------
void __cdecl label_refresh(void* w, tstring name);  // 0x4e4c70
void* __fastcall list_setup(void* p);               // 0x488d50
void* __cdecl make_slider(void* parent, const tstring* name);        // 0x4f5fb0
void* __cdecl make_scrollbar(void* parent, const tstring* name);     // 0x4f6160
void* __cdecl make_editor_list(void* parent, const tstring* name, int a3);   // 0x4f62f0
void* __cdecl make_right_arrow(void* parent, int a2, int a3, float x, int a5, float z);  // 0x4f5ed0
void* __cdecl make_left_arrow(void* parent, int a2, int a3, float x, int a5, float z);   // 0x4f5df0
void* __cdecl make_label(void* parent, const tstring* name, const tstring* text);         // 0x4f5d20
void* __cdecl new_button5(void* parent, const tstring& text, float a, float b, float c, float d);   // 0x4db810
void* __cdecl new_button(void* parent, const tstring& text, float a, float b, float c,
                         const tstring& up, const tstring& down, float k1, float k2, float k3);  // 0x4dcc50

// ---- the face generator window -------------------------------------------------------------------------
struct FaceGen {
    char pad0[0x14];
    int troop;                                      // +0x14
    char pad18[0x2c - 0x18];
    MissionUi* ui;                                         // +0x2c
    MapWidget* body;                                // +0x30
    Frustum fr;                                     // +0x34 (0x120)
    rgl::vector4 v154;                              // +0x154
    float f164;                                     // +0x164
    float f168;                                     // +0x168
    float f16c;                                     // +0x16c
    unsigned char f170;                             // +0x170
    char pad171[0x174 - 0x171];
    int skin;                                       // +0x174
    World* world;                                   // +0x178
    MissionUi* slider;                                     // +0x17c
    MissionUi* deform;                                     // +0x180
    MissionUi* buttons;                                    // +0x184
    MissionUi* cam_btn;                                    // +0x188
    MissionUi* deform_rows[0x1c];                          // +0x18c
    char pad1fc[0x24c - 0x1fc];
    MissionUi* key_labels[0x1c];                           // +0x24c
    char pad2bc[0x30c - 0x2bc];
    MissionUi* b_next_beard;                               // +0x30c
    MissionUi* b_next_hair;                                // +0x310
    MissionUi* b_next_face;                                // +0x314
    MissionUi* b_prev_beard;                               // +0x318
    MissionUi* b_prev_hair;                                // +0x31c
    MissionUi* b_prev_face;                                // +0x320
    MissionUi* t_change_beard;                             // +0x324
    MissionUi* t_change_hair;                              // +0x328
    MissionUi* t_change_face;                              // +0x32c
    MissionUi* sl_age;                                     // +0x330
    MissionUi* sl_hair_color;                              // +0x334
    MissionUi* t_age;                                      // +0x338
    MissionUi* t_hair_color;                               // +0x33c
    MissionUi* b_done;                                     // +0x340
    MissionUi* b_reset;                                    // +0x344
    MissionUi* b_randomize;                                // +0x348
    Entity* ent;                                    // +0x34c
    int f350;                                       // +0x350
    int f354;                                       // +0x354
    int f358;                                       // +0x358
    int f35c;                                       // +0x35c
    float f360;                                     // +0x360
    float f364;                                     // +0x364
    void init();                                    // 0x4f49d0
    void refresh();                                 // 0x4eb3c0
};

// ---- helpers mirroring the original's inlined code -----------------------------------------------------
static __forceinline void* load_mesh(const char* name)
{
    tstring n(name);
    return mesh_clone(g_alloc.get_object(n));
}

static __forceinline void push_item(MissionUi* w, MissionUi* o)
{
    w->children.push_back(o);
}

static __forceinline float config_float_find(const char* key)
{
    float f = 0.0f;
    tstring t(key);
    int idx = g_config.find(t);
    if (idx >= 0) {
        cfg_entry* e = &CFG_ENTRIES[idx];
        char* p = e->value;
        f = (float)atof(p ? p : (char*)&e->value);
    }
    return f;
}

static __forceinline float config_float(const char* key)
{
    float f = 0.0f;
    tstring t(key);
    int idx = -1;
    int i = 0;
    cfg_entry* e = CFG_ENTRIES;
    while (idx == -1) {
        if (i >= g_cfg_count)
            break;
        if (e->key.equals(t))
            idx = i;
        i++;
        e++;
    }
    if (idx >= 0) {
        cfg_entry* p = &CFG_ENTRIES[idx];
        char* s = p->value;
        f = (float)atof(s ? s : (char*)&p->value);
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
    int keys = g_skins_begin[skin].n_keys - 1;

    g_gui.reset();
    g_game->f1a090 = 0;

    Overlay* o = new Overlay;
    ui = (MissionUi*)o;
    float v[2];
    v[0] = 0.0f;
    v[1] = 0.0f;
    ui->set_pos(v);
    v[0] = 1.0f;
    v[1] = 0.75f;
    ui->set_size(v);
    ui->add_image(0, load_mesh("face_gen_window"));
    g_5d8788 = 1;
    g_gui.add_root(*(void**)&ui);

    float deform_k = config_float_find("face_deform_key_h");

    Elem* d = new Elem;
    deform = (MissionUi*)d;
    label_refresh(d, tstring("face_bo_deform"));
    v[0] = deform->w;
    v[1] = (float)keys * deform_k + deform->h;
    deform->set_size(v);

    Elem* b = new Elem;
    buttons = (MissionUi*)b;
    label_refresh(b, tstring("face_bo_button"));
    push_item(ui, buttons);
    deform->set_text(tstring(""));

    for (int i = 0; i < keys; i++) {
        tstring kt;
        MissionUi* row = (MissionUi*)make_slider(d, &tstring("face_sl_deform_key_base"));
        deform_rows[i] = row;
        float y = (float)i * deform_k + row->y;
        v[0] = row->x;
        v[1] = y;
        row->set_pos(v);
        ((TextPart*)&g_anim)->get_text(skin, i, &kt);
        Label* lab = new Label;
        key_labels[i] = (MissionUi*)lab;
        label_refresh(lab, tstring("face_t_deform_key_base"));
        v[0] = ((MissionUi*)lab)->x;
        v[1] = y + ((MissionUi*)lab)->y;
        ((MissionUi*)lab)->set_pos(v);
        ((MissionUi*)lab)->set_text(kt);
        deform->add_child(lab);
    }

    MissionUi* sc = (MissionUi*)make_scrollbar(ui, &tstring("face_s_deform"));
    MissionUi* sl = (MissionUi*)make_editor_list(d, &tstring("face_sc_deform"), (int)sc);
    slider = sl;
    list_setup(sl);
    push_item(ui, sl);

    cam_btn = 0;
    if (g_no_cam != 0) {
        cam_btn = (MissionUi*)new_button5(ui, tstring("key"), 0.0f, 0.72f, 0.5f, 0.028f);
        cam_btn->v_b0();
    }

    float arrow_h = config_float("face_button_h");
    float arrow_h2 = config_float("face_button_arrow_h");
    float arrow_w = config_float("arrow_button_height");
    float arrow_r = config_float("arrow_button_width_ratio");

    b_next_face = (MissionUi*)make_right_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh(b_next_face, tstring("face_b_next_face"));
    b_next_beard = (MissionUi*)make_right_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh(b_next_beard, tstring("face_b_next_beard"));
    b_next_hair = (MissionUi*)make_right_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh(b_next_hair, tstring("face_b_next_hair"));
    b_prev_face = (MissionUi*)make_left_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh(b_prev_face, tstring("face_b_prev_face"));
    b_prev_beard = (MissionUi*)make_left_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh(b_prev_beard, tstring("face_b_prev_beard"));
    b_prev_hair = (MissionUi*)make_left_arrow(buttons, 0, 0, arrow_h2, *(int*)&arrow_r, arrow_w);
    label_refresh(b_prev_hair, tstring("face_b_prev_hair"));

    {
        tstring name("face_t_change_beard");
        tstring key("ui_change_beard");
        t_change_beard = (MissionUi*)make_label(buttons, g_strtab.lookup(key), &name);
    }
    {
        tstring name("face_t_change_face");
        tstring key("ui_change_skin");
        t_change_face = (MissionUi*)make_label(buttons, g_strtab.lookup(key), &name);
    }
    {
        tstring name("face_t_change_hair");
        tstring key("ui_change_hair");
        t_change_hair = (MissionUi*)make_label(buttons, g_strtab.lookup(key), &name);
    }
    sl_age = (MissionUi*)make_slider(buttons, &tstring("face_sl_age"));
    {
        tstring name("face_t_age");
        tstring key("ui_age");
        t_age = (MissionUi*)make_label(buttons, g_strtab.lookup(key), &name);
    }
    sl_hair_color = (MissionUi*)make_slider(buttons, &tstring("face_sl_hair_color"));
    {
        tstring name("face_t_hair_color");
        tstring key("ui_change_hair_color");
        t_hair_color = (MissionUi*)make_label(buttons, g_strtab.lookup(key), &name);
    }

    if (g_skins_begin[skin].first_face == 0) {
        b_next_beard->v_b0();
        b_prev_beard->v_b0();
        t_change_beard->v_b0();
    }

    {
        tstring key("ui_randomize");
        b_randomize = (MissionUi*)new_button(buttons, *g_strtab.lookup(key), 0.0f, 0.0f, arrow_h,
                                             tstring("medium_button_down"), tstring("medium_button"),
                                             3.7f, 0.072f, 0.085f);
    }
    label_refresh(b_randomize, tstring("face_b_randomize"));
    {
        tstring key("ui_reset");
        b_reset = (MissionUi*)new_button(buttons, *g_strtab.lookup(key), 0.0f, 0.0f, arrow_h,
                                         tstring("medium_button_down"), tstring("medium_button"),
                                         3.7f, 0.072f, 0.085f);
    }
    label_refresh(b_reset, tstring("face_b_reset"));
    {
        tstring key("ui_done");
        b_done = (MissionUi*)new_button(buttons, *g_strtab.lookup(key), 0.0f, 0.0f, arrow_h,
                                        tstring("medium_button_down"), tstring("medium_button"),
                                        3.7f, 0.072f, 0.085f);
    }
    label_refresh(b_done, tstring("face_b_done"));

    body = new MapWidget;

    float lo[2];
    float hi[2];
    int z1 = 0;
    int z2 = 0;
    int z3 = 0;
    hi[0] = 80.0f;
    hi[1] = 80.0f;
    lo[0] = -80.0f;
    lo[1] = -80.0f;
    World* w = new World(lo, hi, &z1, &z2, &z3);
    world = w;
    w->color = rgl::vector4(0.3f, 0.3f, 0.3f);
    w->color.w = 1.0f;

    {
        wind* wd = new wind;
        w->tracks.push_back((int)wd);
        wd->v04 = rgl::vector4(0.1f, 0.15f, 0.2f);
        wd->v14 = rgl::vector4(0.2f, 0.2f, 0.2f);
        wd->v24 = rgl::vector4(-2.0f, -2.0f, 12.0f);
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44 = rgl::vector4(-0.5f, 0.0f, -1.0f);
    }
    {
        wind* wd = new wind;
        w->tracks.push_back((int)wd);
        wd->v04 = rgl::vector4(1.0f, 1.0f, 1.0f);
        wd->v14 = rgl::vector4(1.0f, 1.0f, 1.0f);
        wd->v24 = rgl::vector4(-2.0f, -2.0f, 2.0f);
        wd->type = 2;
        wd->f54 = 20.0f;
        wd->v44 = rgl::vector4(1.0f, 1.0f, -1.0f);
    }

    ent = w->create_entity(9);
    body->world = w;

    v[0] = 0.05125f;
    v[1] = 0.266875f;
    ((MissionUi*)body)->set_pos(v);
    v[0] = 0.45875f;
    v[1] = 0.44f;
    ((MissionUi*)body)->set_size(v);
    body->flags |= 0x300;
    body->color = 0xff201815;

    float ratio = body->h / body->w * g_fov_k;
    float tv = (float)tan(g_tan_ang);
    fr.perspective = 1;
    fr.nearz = 0.01f;
    fr.farz = 20.0f;
    fr.left = -(0.01f * tv);
    fr.right = tv * 0.01f;
    fr.bottom = -((0.01f * tv) * ratio);
    fr.top = (tv * 0.01f) * ratio;
    fr.build_planes();

    push_item(ui, (MissionUi*)body);

    fr.m.o = rgl::vector4(0.0f, 0.0f, 10.0f);
    v154 = rgl::vector4(0.0f, 0.0f, 0.0f);
    f164 = -0.3f;
    f168 = -0.05f;
    f16c = 0.7f;
    f170 = 0;
    body->view = fr;

    if (troop >= 0) {
        char* rec = g_game->chars + troop * 0x508;
        int sk = *(int*)(rec + 0x30) & 0xf;
        int zero = 0;
        g_anim.cur = gen_face_seed(&sk, &zero, &g_char_count);
        g_anim.unpack(rec + 0x3c0);
        f354 = g_anim.f04;
        f350 = g_anim.f00;
        f35c = g_anim.f14;
        f358 = g_anim.f0c;
        f360 = g_dummy0;
        f364 = g_dummy1;
        f354++;
        f350++;
        for (int i = 0; i < keys; i++)
            deform_rows[i]->set_value((&g_dummy_mat)[i]);
        sl_age->set_value(g_dummy0);
        sl_hair_color->set_value(g_dummy1);
    }

    refresh();
}
