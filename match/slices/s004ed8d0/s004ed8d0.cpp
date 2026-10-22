// Slice s004ed8d0: the cursor pick.
//   0x4ed8d0  pick_entity - one contact test per scene object against the view cell; the nearest hit is
//             selected (Ctx::f2cc) and its label drawn.
// Class and member names are provisional.
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include "rgl.h"

using rgl::vector4;

// ---- constants / callees ------------------------------------------------------------------------------------
extern const float g_549cdc;            // 1.0f
extern const float g_549d70;            // 0.5f
extern const float g_3f000000;          // 0.5f   (contact radius)
extern const float g_3f8ccccd;          // 1.1f   (pick tolerance)
extern const float g_4e6e6b28;          // 1e9f   (far reach)
extern const float g_3ac49ba6;          // 1.5e-3f (capsule radius)

// ---- globals ------------------------------------------------------------------------------------------------
extern int g_menu_mode;                 // 0x5e2930+0x1a20c
extern int g_sel_entity;                // 0x5e2930+0x1a1b4
extern int g_tasks[];                   // 0x5e292c scene (0x65dc)
extern int g_map_rows[];                // 0x5e28f8 map rows (0x20 bytes)
extern int g_map_count;                 // 0x5e29e8
extern int g_point_rows[];              // 0x5e29dc point rows (0x78 bytes)
extern void* g_overlay;                 // 0x5d75e8
extern int g_cursor_id;                 // 0x5d8760
extern int g_cursor_color;              // 0x5d8764
extern int g_list0;                     // 0x5d8020
extern int g_list1;                     // 0x5d8024
extern float g_list2;                   // 0x5d8028
extern int g_sel_ent;                   // 0x5d8780

// ---- types --------------------------------------------------------------------------------------------------
struct tstring {                        // 16-byte string
    int pos; char* buf; int cap; int len;
    tstring();
    ~tstring();
    tstring& operator=(const tstring& o);
    void assign(const char* s);
};
extern tstring g_strs[];                // 0x5d7850 .. 0x5d7d50

struct shape_base {                     // collision shape base
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base() {}
    virtual int shape_type();
    virtual void s2(void* m);           // set transform
    virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7(); virtual void s8(); virtual void s9();
    virtual vector4* get_min(vector4* out);
    virtual vector4* get_max(vector4* out);
};
struct shape_ext : shape_base { shape_ext(); };
struct capsule : shape_ext {            // size 0x64
    vector4 p0, p1, p2, p3;
    float radius;
    capsule(float r, const vector4* a, const vector4* b);    // 0x433310
};
struct sphere_shape : shape_base {      // size 0x44
    vector4 v20, v30; float radius;
    sphere_shape(const vector4* centre, float r);           // 0x408770
};
struct smat : rgl::matrix {
    void scale_rows(const vector4* k);                      // 0x41fee0
};
void identity_pos(float* m);                                // 0x40cd00
void capsule_ctor_inline(capsule* c, float r, const vector4* a, const vector4* b);  // 0x433310
int __cdecl shape_contact_count(shape_base* a, void* b, int max, void* out);    // 0x40bfe0
void __stdcall make_contact_box(void* out);                 // 0x408420

struct scene {
    char pad000[0x65dc];
    int f65dc;                          // +0x65dc entity being built
    int f65e0;                          // +0x65e0 entity list
    char pad5e4[0x668c - 0x65e4];
    int f668c;                          // +0x668c scene objects
    int f6690;                          // +0x6690 end
};
struct scene_row {                      // 0x1c8 bytes
    char pad000[0x18];
    vector4 mat[4];                     // +0x18 transform
    vector4 origin;                     // +0x58
    float radius;                       // +0x68
    char pad06c[0x188 - 0x6c];
    int kind;                           // +0x188
    int ref;                            // +0x18c
    int caption[4];                     // +0x190 tstring
    int point;                          // +0x1a0
    int f1a4;                           // +0x1a4
};

struct vector2 { float x, y; };

struct ViewRect {                       // this = Ctx::f60
    void cell_rect(int c0[4], int c1[4]);        // 0x4bd2f0
};
struct OverlayItf {                     // vtable 0x590064
    virtual void release(int a);
};
struct Ctx {
    char pad000[0x60];
    int f60;
    char pad064[0xe0 - 0x64];
    float f0e0, f0e4, f0f0, f0f4;
    char pad0f8[0x178 - 0xf8];
    int f178;
    int cell_x0, cell_y0, cell_x1, cell_y1;
    int f18c;
    float f190;
    char pad194[0x2c8 - 0x194];
    int f2c8, f2cc;
    char pad2d0[0x2dc - 0x2d0];
    char f2dc;
    char pad2dd[0x2f8 - 0x2dd];
    char f2f8;
    char pad2f9[0x310 - 0x2f9];
    int f310;
    char pad314[0x32c - 0x314];
    int f32c;
    char pad330[0x354 - 0x330];
    char f354;
    char pad355[0x568 - 0x355];
    void* f568;
    char pad56c[0x16e8 - 0x56c];
    float f16e8, f16ec, f16f0;
    void __fastcall draw_string(tstring* s, float a, int color, char b);    // 0x4e43e0
    void __fastcall select_entity(scene_row* row, int ent, int flags);      // 0x4ea0c0
    void __fastcall reset_all(char reinit);                                 // 0x42cdb0
    void __fastcall set_pair(const vector2* p);                             // 0x4c5190
};

// ------------------------------------------------------------------------------------------------------------
// @ 0x4ed8d0
// ------------------------------------------------------------------------------------------------------------
void __fastcall pick_entity(Ctx* self)
{
    int set_junk = 0;
    scene* sc = (scene*)g_tasks;
    *(char*)(sc->f65dc + 0x140) = 0;
    if (g_menu_mode) {
        self->reset_all(1);
        vector2 pp = { 0.5f, 0.23f };
        self->set_pair(&pp);
        set_junk = 0;
        return;
    }
    int rect0[4], rect1[4];
    ((ViewRect*)(self->f60))->cell_rect(rect0, rect1);
    vector4 lo, hi;
    lo.x = *(float*)&rect0[0];
    lo.y = *(float*)&rect0[1];
    lo.z = *(float*)&rect0[2];
    lo.w = *(float*)&rect0[3];
    hi.x = *(float*)&rect1[0];
    hi.y = *(float*)&rect1[1];
    hi.z = *(float*)&rect1[2];
    hi.w = *(float*)&rect1[3];

    float capbuf[0x64 / 4];
    capsule_ctor_inline((capsule*)capbuf, g_3ac49ba6, &lo, &hi);
    float idpos[16];
    identity_pos(idpos);
    smat probe = *(smat*)idpos;
    float sphbuf[0x44 / 4];
    sphere_shape* sphere = (sphere_shape*)sphbuf;
    sphere->v20 = lo;
    sphere->radius = g_4e6e6b28;
    sphere->v30 = lo;

    float best = g_4e6e6b28;
    int best_i = -1;
    int i = 0;
    int n = (sc->f6690 - sc->f668c) / 0x1c8;
    while (i < n) {
        scene_row* row = (scene_row*)(sc->f668c + i * 0x1c8);
        vector4 o = row->origin;
        shape_base* hit = 0;
        vector4 centre = lo;
        centre.w = 0.0f;
        float rad = g_3f000000;
        make_contact_box(&row->mat[0]);
        if (row->kind == 2) {
            rad = row->mat[3].length() * 0.5f;
        } else if (row->kind == 1) {
            rad = g_3f000000;
        } else if (row->kind == 4) {
            centre.x = 0.0f; centre.y = 0.0f; centre.z = 1.0f; centre.w = o.w;
            rad = row->mat[3].length();
            int map = row->ref;
            if (map >= 0 && map < g_map_count) {
                int* maprow = (int*)((char*)g_map_rows + map * 0x20);
                if (row->point < 0 || maprow[6] <= row->point)
                    row->point = row->point % maprow[6];
                char* pr = (char*)maprow[7] + row->point * 0x48;
                if (*(int*)(pr + 0x20)) {
                    hit = *(shape_base**)(pr + 0x24);
                    smat m2 = probe;
                    m2.scale_rows(&row->mat[0]);
                    hit->s2(&m2);
                }
            }
        } else if (row->kind == 5) {
            rad = (row->mat[1].x + row->mat[0].x) * 0.5f;
        } else if (row->kind == 0) {
            char* pr = (char*)g_point_rows + row->ref * 0x78;
            centre = *(vector4*)(pr + 0x58);
            rad = row->mat[3].length() * *(float*)(pr + 0x68);
            hit = *(shape_base**)(pr + 0x54);
            if (hit) {
                smat m2 = probe;
                m2.scale_rows(&row->mat[0]);
                hit->s2(&m2);
            }
        }
        if (shape_contact_count((shape_base*)capbuf, sphere, 1, &row->mat[1])) {
            if (hit == 0 || shape_contact_count(hit, sphere, 1, &row->mat[1])) {
                vector4 d;
                d.x = lo.x - centre.x;
                d.y = lo.y - centre.y;
                d.z = lo.z - centre.z;
                float len = d.length();
                if (len < rad * g_3f8ccccd + best) {
                    best = len;
                    best_i = i;
                }
            }
        }
        i++;
    }
    self->f2cc = best_i;
    if (g_overlay) {
        ((OverlayItf*)g_overlay)->release(1);
    }
    g_cursor_id = -1;
    g_cursor_color = 0xff101010;
    g_overlay = 0;
    for (tstring* s = g_strs; s < g_strs + ((0x5d7d50 - 0x5d7850) / 0x10); s++)
        s->assign("");
    g_list2 = -0.03f;
    g_list0 = 0;
    g_list1 = 0;
    g_sel_ent = 0;
    vector2 pair = { 0.1f, 0.13f };
    self->set_pair(&pair);
    if (self->f2cc == -1) {
        self->f2c8 = -1;
        return;
    }
    int color = 0xffdf4f;
    if (self->f2cc == g_sel_entity) {
        color = 0xffaaff22;
    }
    scene_row* sel = (scene_row*)(sc->f668c + self->f2cc * 0x1c8);
    tstring s_kind, s_info, s_info2;
    switch (sel->kind) {
    case 4:
        s_kind.assign("FLORA");
        s_info = *(tstring*)sel->caption;
        break;
    case 2:
        s_kind.assign("ITEM");
        s_info = *(tstring*)sel->caption;
        break;
    case 0:
        s_kind.assign("OBJECT");
        s_info = *(tstring*)sel->caption;
        break;
    case 3:
        s_info = *(tstring*)sel->caption;
        break;
    case 1: {
        s_kind.assign("ENTRY");
        char buf[512];
        sprintf(buf, "point no: %d", sel->point);
        s_info.assign(buf);
        break;
    }
    case 5: {
        s_kind.assign("PASSAGE");
        char pbuf[512], pbuf2[512];
        sprintf(pbuf, "Menu item no: %d", sel->f1a4);
        sprintf(pbuf2, "Target Entry Point: %d", sel->point);
        s_info.assign(pbuf);
        s_info2.assign(pbuf2);
        break;
    }
    case 6:
        s_kind.assign("SPAWNED ITEM");
        s_info = *(tstring*)sel->caption;
        break;
    case 7:
        s_kind.assign("SHIELD");
        s_info = *(tstring*)sel->caption;
        break;
    }
    if (s_kind.len) self->draw_string(&s_kind, 0.03f, color, 1);
    if (s_info.len) self->draw_string(&s_info, 0.0269f, 0xffdf4f, 1);
    if (s_info2.len) self->draw_string(&s_info2, 0.0269f, 0xffdf4f, 1);
    memcpy((void*)(sc->f65dc + 0x58), &sel->origin, 16);
    self->select_entity(sel, sc->f65e0, 0x80229933);
    if (!self->f2f8 && !g_menu_mode)
        *(char*)(sc->f65dc + 0x140) = 1;
    self->f2c8 = -1;
}
