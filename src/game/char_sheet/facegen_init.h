#ifndef FACEGEN_INIT_H
#define FACEGEN_INIT_H
// s004f49d0 (ds1 slice 190): the "face_gen_window" construction (face generator menu).
// Provisional names; every offset comes from the disassembly of 0x4f49d0..0x4f5d17.
#include <windows.h>
#include <new>
#include <math.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine string (16 bytes) ----------------------------------------------------------------------------
#include "../tstring.h"

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
#include "../../include/frustum.h"                   // rgl::frustum (0x120 bytes; the slices called it rgl::frustum)
struct World;

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + frustum copy
#include "../map_widget.h"                 // MapWidget (the slice's face_body)
typedef MapWidget face_body;

// the face-generator window, ctor 0x4f49d0
struct FaceGen {
    char pad0[0x14];
    int troop;                                          // +0x14
    char pad18[0x2c - 0x18];
    overlay* ui;                                        // +0x2c
    face_body* body;                                    // +0x30
    rgl::frustum fr;                                     // +0x34 (0x120 bytes)
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
#include "../engvec.h"                                 // engvec<T>

// ---- globals ---------------------------------------------------------------------------------------------
#include "../../include/msg_log.h"
extern MsgLog g_msglog;                                 // 0x5d7080
extern unsigned char g_msg_tick;                        // 0x5d8788
struct gui_mgr { void set_window(widget** w); };        // 0x49b120
extern gui_mgr g_gui;                                   // 0x5d7080
struct resmgr { mesh* get_object(const tstring& n); };  // 0x419540
extern resmgr g_alloc;                                  // 0x5d6fa0
#include "../str_table.h"   // 0x43a1e0
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

#include "../animator.h"                              // Animator, g_anim
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

#endif
