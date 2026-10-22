// Slice s004eb270: the face-generator window (FaceGen, ctor 0x4f49d0; layouts shared with s004f49d0):
// the camera/frustum refresh, the skin randomiser/packer, the character-record array erase helper, the
// event forwarding thunk, the save/load UI entry and the "menu variables loaded" handler.
// Names provisional; offsets are from this range.
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;

// ---- engine matrix / frustum (s004f49d0, s004291c0) -----------------------------------------------------
struct mat4 {
    vector4 s, f, u, o;
    void rotate_fu(const float* angle);                 // 0x40e570 (thiscall, ret 4)
};
mat4* identity_pos(mat4* out);                          // 0x40cd00 (cdecl)

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

// ---- tstring --------------------------------------------------------------------------------------------
struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    ~tstring();                                         // 0x42bc40
    void assign(const char* s);                         // 0x42bcb0
    void append(char c);                                // 0x42be70
};
struct MsgLog {
    void add(tstring msg, int color);                   // 0x452c00
    void func_4e6050();                                 // 0x4e6050
};

// ---- save/load menu interfaces --------------------------------------------------------------------------
struct CD3DFont { void DrawText(float, float, unsigned long, const char*, unsigned long); }; // 0x404f00
struct Renderer { void func_47e520(int); };             // 0x47e520, instance at 0x5fe898
extern Renderer g_renderer;                             // 0x5fe898
struct MapEditor {                                      // instance at 0x8ee3c48
    void restart();                                     // 0x4c0520
    void func_4c1a60();                                 // 0x4c1a60
    void func_4c1940();                                 // 0x4c1940
};
#define g_editor (*(MapEditor*)0x8ee3c48)
extern MsgLog g_msglog;                                 // 0x5d7080

// ---- UI widget (vtable family 0x597050; s004f49d0 / s004dada0) ------------------------------------------
struct UiVt {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void s10(); virtual void s11(); virtual void s12(); virtual void s13();
    virtual float value();                              // +0x38
    virtual void s15(); virtual void s16(); virtual void s17(); virtual void s18();
    virtual void refresh_anim();                        // +0x4c
    virtual void s20(); virtual void s21(); virtual void s22(); virtual void s23(); virtual void s24();
    virtual void s25(); virtual void s26(); virtual void s27(); virtual void s28(); virtual void s29();
    virtual void s30(); virtual void s31(); virtual void s32(); virtual void s33(); virtual void s34();
    virtual void s35(); virtual void s36(); virtual void s37(); virtual void s38(); virtual void s39();
    virtual void s40();
    virtual void set_text(const void* a, const void* b);   // +0xa4
};
struct Ui { UiVt* vt; };
struct Entity;
struct World;

// the 0x358-byte camera body (ctor 0x4ed630): widget + world link + frustum copy
struct face_body {
    Ui ui;                                              // +0x000 (vptr)
    char pad004[0x178 - 0x4];
    World* world;                                       // +0x178
    char pad17c[0x1ac - 0x17c];
    eng_frustum view;                                   // +0x1ac (0x120 bytes)
};

struct ui_vec {                                         // engine vector of void*
    int lead; void** first; void** last; void** end;
    void push_back(void* const& v);                     // 0x41b750 (ret 4)
};
struct Entity {                                         // 0x34c entity (EChar-like)
    UiVt* vt;                                           // +0x000
    char pad4[0x108 - 4];
    ui_vec lods;                                        // +0x108
};
struct vector2 { float x, y; };

// ---- skin record table (0x8ed4f7c, 0x1f8-byte records) -------------------------------------------------
struct SkinSet {
    char pad0[0x3c];
    int n;                                              // +0x3c number of deform keys
    char pad40[0x194 - 0x40];
    int div;                                            // +0x194
    char pad198[0x1f8 - 0x198];
    void regen(int arg, float* values);                 // 0x4735e0 (ret 8)
};
extern SkinSet* g_skins_begin;                          // 0x8ed4f7c

// ---- face-gen window (ctor 0x4f49d0) --------------------------------------------------------------------
struct PackedState { unsigned __int64 a, b, c, d; };
struct FaceGen {
    char pad0[0x14];
    int troop;                                          // +0x14
    char pad18[0x2c - 0x18];
    UiVt* ui;                                           // +0x2c
    face_body* body;                                    // +0x30
    eng_frustum fr;                                     // +0x34 (0x120 bytes)
    float v154[4];                                      // +0x154
    float f164;                                         // +0x164
    float f168;                                         // +0x168
    float f16c;                                         // +0x16c zoom
    unsigned char f170;                                 // +0x170
    char pad171[0x174 - 0x171];
    int skin;                                           // +0x174
    World* world;                                       // +0x178
    UiVt* slider;                                       // +0x17c
    UiVt* deform;                                       // +0x180
    UiVt* buttons;                                      // +0x184
    UiVt* cam_btn;                                      // +0x188
    UiVt* deform_rows[0x1c];                            // +0x18c (one row per deform key)
    char pad1fc[0x24c - 0x1fc];
    UiVt* key_labels[0x1c];                             // +0x24c
    char pad2bc[0x30c - 0x2bc];
    UiVt* b_next_beard;                                 // +0x30c
    UiVt* b_next_hair;                                  // +0x310
    UiVt* b_next_face;                                  // +0x314
    UiVt* b_prev_beard;                                 // +0x318
    UiVt* b_prev_hair;                                  // +0x31c
    UiVt* b_prev_face;                                  // +0x320
    UiVt* t_change_beard;                               // +0x324
    UiVt* t_change_hair;                                // +0x328
    UiVt* t_change_face;                                // +0x32c
    UiVt* sl_age;                                       // +0x330
    UiVt* sl_hair_color;                                // +0x334
    UiVt* t_age;                                        // +0x338
    UiVt* t_hair_color;                                 // +0x33c
    UiVt* b_done;                                       // +0x340
    UiVt* b_reset;                                      // +0x344
    UiVt* b_randomize;                                  // +0x348
    Entity* ent;                                        // +0x34c
    int f350;                                           // +0x350
    int f354;                                           // +0x354
    int f358;                                           // +0x358
    int f35c;                                           // +0x35c
    float f360;                                         // +0x360
    float f364;                                         // +0x364
    PackedState state;                                  // +0x368
    char keys[4][0x28];                                 // +0x388
    void update_view(int unused);                       // 0x4eb270 (ret 4)
    void refresh();                                     // 0x4eb3c0
};
struct Game { char pad0[0x13a00]; char* chars; };
extern Game* g_game;                                    // 0x5e2930

// the global character-builder/animation state (0x8ed4f60; s0047f650, s004f49d0)
struct Animator {
    int f0;                                             // +0x00
    int f4;                                             // +0x04
    int cur;                                            // +0x08
    int f0c;                                            // +0x0c
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x1c - 0x18];
    SkinSet* skins;                                     // +0x1c
    char pad20[0x28 - 0x20];
    int char_count;                                     // +0x28
    float values[64];                                   // +0x2c
    float f12c;                                         // +0x12c
    void randomise();                                   // 0x4eb320
    void collect(bool shader, void** a, void** b, void** c);    // 0x47fb10 (ret 0x10)
    void pack_state(PackedState* out);                  // 0x4eb660 (ret 4)
};
extern Animator g_anim;                                 // 0x8ed4f60
extern int g_cur;                                       // 0x8ed4f68
extern int g_x8ed4f64;                                  // 0x8ed4f64
extern int g_x8ed4f74;                                  // 0x8ed4f74
extern int g_char_count;                                // 0x8ed4f88
extern float g_dummy_mat;                               // 0x8ed4f8c
extern float g_dummy0;                                  // 0x8ed5088
extern float g_dummy1;                                  // 0x8ed508c
int gen_face_seed(int* flags, int* zero, int* count);   // 0x432510 (cdecl)

extern unsigned g_xa181ac;                              // 0xa181ac (shader-on flag)
extern const float g_590c54;                            // 0x590c54 = 7.0f
extern const float g_598588;                            // 0x598588 = 63.0f
extern const float g_549d70;                            // 0x549d70 = 0.5f
extern "C" int __cdecl sprintf(char*, const char*, ...);            // 0x536f8f
extern "C" char* __cdecl _itoa(int, char*, int);                    // 0x5371f7
extern "C" double __cdecl floor(double);                            // 0x536b10

// integer part, +1 when the fraction is above 0.5 (same idiom as s004ed6f0)
static inline int grid_count(float v)
{
    double fl = floor((double)v);
    int f = (int)fl;
    if (v - (float)fl <= g_549d70)
        return f;
    return f + 1;
}

// @ 0x4eb270
void FaceGen::update_view(int unused)
{
    mat4 m;
    identity_pos(&m);
    float angle = 1.5707964f;
    m.rotate_fu(&angle);
    vector4 t;
    t.x = m.u.x * f16c;
    t.y = m.u.y * f16c;
    t.z = m.u.z * f16c;
    m.o.x = t.x + m.o.x;
    m.o.y = t.y + m.o.y;
    m.o.z = t.z + m.o.z;
    eng_frustum* f = &fr;
    *(mat4*)((char*)f + 0xc8) = m;
    f->build_planes();
    body->view = *f;
}

// @ 0x4eb320
void Animator::randomise()
{
    int n = 0x3f;
    float* out = values;
    do {
        int r1 = rand() % 0x3dc9;
        float first = (float)r1 * 6.322312e-05f;
        int r2 = rand() % 0x3dc9;
        *out = ((float)r2 * 6.322312e-05f + first) * 0.5f;
        ++out;
        --n;
    } while (n != 0);
    int r = rand() % 0x3dc9;
    f12c = (float)r * 6.322312e-05f;
    SkinSet* base = (SkinSet*)((char*)skins + cur * 0x1f8);
    base->regen(-1, values);
}

// @ 0x4eb3c0
void FaceGen::refresh()
{
    int n = g_skins_begin[skin].n - 1;
    for (int i = 0; i < n; i++)
        (&g_dummy_mat)[i] = deform_rows[i]->value();
    f360 = sl_age->value();
    f364 = sl_hair_color->value();
    ((UiVt*)ent)->refresh_anim();
    int flags = *(int*)(g_game->chars + troop * 0x508 + 0x30) & 0xf;
    int zero = 0;
    g_cur = gen_face_seed(&flags, &zero, &g_char_count);
    g_dummy1 = f364;
    g_dummy0 = f360;
    g_anim.f14 = f35c;
    g_anim.f0 = f350 - 1;
    g_anim.f4 = f354 - 1;
    int r = f358 % g_skins_begin[g_cur].div;
    g_anim.f0c = r;
    g_anim.f10 = r;
    void *a, *b, *c;
    g_anim.collect(g_xa181ac != 0, &a, &b, &c);
    void* p;
    p = a;
    ent->lods.push_back(p);
    p = b;
    if (p)
        ent->lods.push_back(p);
    p = c;
    if (p)
        ent->lods.push_back(p);
    g_anim.pack_state(&state);
    char* kp = keys[0] + 16;
    for (int i = 0; i < 4; i++) {
        unsigned __int64 v = ((unsigned __int64*)&state)[i];
        for (int j = 15; j >= 0; j--) {
            char buf[4];
            _itoa((int)(v & 0xf), buf, 16);
            kp[j - 16] = buf[0];
            v >>= 4;
        }
        *kp = 0;
        kp += 0x28;
    }
    if (cam_btn) {
        char buf[512];
        sprintf(buf, "k=%s%s%s%s", keys[0], keys[1], keys[2], keys[3]);
        tstring text(buf);
        vector2 pos;
        cam_btn->set_text(&pos, &text);
    }
}

// ---- skin-record array helpers (0x4eb810/0x4eb860, used by the map editor) -------------------------------
struct Big {
    char pad0[0x1f8];
    void destroy();                                     // 0x4d3720
};
struct BigArray {
    char pad0[4];
    Big* begin;                                         // +4
    Big* end;                                           // +8
    Big* cap;                                           // +0xc
    void destroy();
    void erase(Big** out, Big* first, Big* last);       // 0x4eb860
};
Big* __cdecl array_copy_range(Big* start, Big* end, Big* dest, Big* out);  // 0x4da880

// @ 0x4eb810
void BigArray::destroy()
{
    if (begin) {
        Big* p = begin;
        Big* stop = end;
        while (p != stop) {
            p->destroy();
            p = (Big*)((char*)p + 0x1f8);
        }
        operator delete(begin);
    }
    begin = 0;
    end = 0;
    cap = 0;
}

// @ 0x4eb860
void BigArray::erase(Big** out, Big* first, Big* last)
{
    Big** fp = &first;
    if (*fp != last) {
        Big* p = array_copy_range(last, this->end, *fp, *fp);
        Big* e = this->end;
        while (p != e) {
            p->destroy();
            p = (Big*)((char*)p + 0x1f8);
        }
        this->end = p;
    }
    *out = *fp;
}

// ---- event forwarding -----------------------------------------------------------------------------------
struct Forwarder {
    void forward(int a, int b, int c);                  // 0x4eb8c0
};
void __cdecl func_4e4340(int a, int b, int c, void* self, int e);   // 0x4e4340

// @ 0x4eb8c0
void Forwarder::forward(int a, int b, int c)
{
    func_4e4340(a, b, c, this, c);
}

// @ 0x4eb8e0 save-load UI entry (whole body reconstructed at 0x4eb8e0; 0x4eb8ed is its interior
// continuation and is not separately callable)
extern char g_5e2938;                                   // 0x5e2938
extern int g_5e28c8;                                    // 0x5e28c8
extern int g_a16c58;                                    // 0xa16c58
extern int g_5d75c8;                                    // 0x5d75c8 (view width)
extern int g_5d75cc;                                    // 0x5d75cc (view height)
extern void* g_a17f28;                                  // 0xa17f28 (IDirect3DDevice9*)
extern char* g_5a9f54;                                  // 0x5a9f54 (save/load text)
extern int g_5a9f5c;                                    // 0x5a9f5c (text length)
extern int g_5a9fa0;                                    // 0x5a9fa0
extern int g_5a9fb8;                                    // 0x5a9fb8
extern int g_5a9fb0;                                    // 0x5a9fb0
extern tstring g_5a9f50;                                // 0x5a9f50
extern const char g_54a480[];                           // 0x54a480 ("")

struct Viewport { int x, y, w, h; float minz, maxz; };
typedef void (__stdcall* DevCall2)(void*, void*);
typedef void (__stdcall* DevCall3i)(void*, int, int);
typedef void (__stdcall* PanelCall7)(void*, int, int, int, int, float, int);
typedef int  (__stdcall* PanelCall1i)(void*);
typedef void (__stdcall* PanelCall1)(void*);

struct SaveLoadUi {
    int run();                                          // 0x4eb8e0
    void start();                                       // 0x4bace0
};

int SaveLoadUi::run()
{
    if (g_5e2938) {
        g_5e2938 = 0;
        g_editor.func_4c1a60();
        g_editor.func_4c1940();
    }
    ++g_5e28c8;
    ++g_a16c58;
    start();

    Viewport vp;
    vp.x = 0; vp.y = 0;
    vp.w = g_5d75c8; vp.h = g_5d75cc;
    vp.minz = 0.0f; vp.maxz = 1.0f;
    ((DevCall2)(*(void***)g_a17f28)[0xbc / 4])(g_a17f28, &vp);

    void* panel = *(void**)((char*)this + 0xf8);
    ((PanelCall7)(*(void***)panel)[0xac / 4])(panel, 0, 0, 2, 0x302010, 1.0f, 0);

    g_msglog.func_4e6050();
    g_renderer.func_47e520(0);

    if (g_5a9fa0 != 0) {
        void* w = *(void**)((char*)this + 0xf8);
        if (((PanelCall1i)(*(void***)w)[0xa4 / 4])(w) >= 0) {
            Viewport vp2;
            vp2.x = 0; vp2.y = 0;
            vp2.w = g_5d75c8; vp2.h = g_5d75cc;
            vp2.minz = 0.0f; vp2.maxz = 1.0f;
            ((DevCall2)(*(void***)g_a17f28)[0xbc / 4])(g_a17f28, &vp2);

            CD3DFont* font = *(CD3DFont**)((char*)this + 0x348);
            font->DrawText(2.0f, 0.0f, 0xffffff00, (const char*)this + 0x2da, 0);

            if (g_5a9fb8 != 0 || g_5a9fb0 != 0) {
                Viewport vp3;
                vp3.x = 0; vp3.y = 0;
                vp3.w = g_5d75c8; vp3.h = g_5d75cc;
                vp3.minz = 0.0f; vp3.maxz = 1.0f;
                ((DevCall2)(*(void***)g_a17f28)[0xbc / 4])(g_a17f28, &vp3);

                tstring ts;
                int y = 40;
                for (int i = 0; i < g_5a9f5c; ++i) {
                    char ch = g_5a9f54[i];
                    if (ch == ';') {
                        char* s = ts.buf ? ts.buf : (char*)&ts.buf;
                        font->DrawText(2.0f, (float)y, 0xff64ff64, s, 0);
                        y += 12;
                        ts.assign("");
                    } else {
                        ts.append(ch);
                    }
                }
                if (ts.len != 0) {
                    char* s = ts.buf ? ts.buf : (char*)&ts.buf;
                    font->DrawText(2.0f, (float)y, 0xff64ff64, s, 0);
                }
            }

            ((DevCall3i)(*(void***)g_a17f28)[0x104 / 4])(g_a17f28, 0, 0);
            ((DevCall3i)(*(void***)g_a17f28)[0x104 / 4])(g_a17f28, 1, 0);
            void* w2 = *(void**)((char*)this + 0xf8);
            ((PanelCall1)(*(void***)w2)[0xa8 / 4])(w2);
        }
    }

    g_5a9f50.assign("");
    return 0;
}

// 0x4eb8ed: interior continuation of SaveLoadUi::run (the 645 bytes after the 13-byte prologue at
// 0x4eb8e0). It is not a separately callable function and has no source symbol of its own; the body is
// the one reconstructed above.

// @ 0x4eb660
void Animator::pack_state(PackedState* out)
{
    out->a = out->b = out->c = out->d = 0;
    out->a = (unsigned __int64)(f4 + 1);
    out->a |= (unsigned __int64)(f0 + 1) << 6;
    out->a |= (unsigned __int64)f14 << 12;
    out->a |= (unsigned __int64)f0c << 18;
    out->a |= (unsigned __int64)(int)(f12c * 63.0f) << 24;
    out->a |= (unsigned __int64)(int)(values[63] * 63.0f) << 30;
    unsigned __int64* w = (unsigned __int64*)out;
    int bit = 0;
    float* v = values;
    for (int i = 0; i < ((SkinSet*)((char*)skins + cur * 0x1f8))->n; i++) {
        int sh = bit;
        int wi = 1;
        if (sh >= 0x3f) {
            sh -= 0x3f;
            wi = 2;
        }
        float fv = *v * 7.0f;
        int fi = grid_count(fv);
        w[wi] |= (unsigned __int64)fi << sh;
        bit += 3;
        ++v;
    }
}

// @ 0x4ebb80
extern int g_no_cam;                                    // 0x5a9fb0
struct main_menu {
    char pad0[0x4c];
    int choice;                                         // +0x4c
    void choose(int option);                            // 0x495700
};
extern main_menu g_main_menu;                           // 0xa73980
struct MenuVars {
    char pad0[4];
    int** rows;                                         // +4
    unsigned count;                                     // +8
};
extern MenuVars g_menu_vars;                            // 0xa739f8
extern unsigned g_a73a04;                               // 0xa73a04
extern unsigned g_a73a08;                               // 0xa73a08

void __cdecl menu_variables_loaded(char flag)
{
    if (!flag && !g_no_cam)
        return;
    g_editor.restart();
    if (flag)
        return;
    unsigned u = g_a73a08 + g_a73a04 - 1;
    unsigned q = u >> 2;
    unsigned r = u - q * 4;
    MenuVars* vars = &g_menu_vars;
    unsigned count = vars->count;
    int** rows = vars->rows;
    if (count <= q)
        q -= count;
    int value = rows[q][r];
    g_main_menu.choose(value);
    g_msglog.add("Menu variables are loaded...", 0xffff0000);
}
