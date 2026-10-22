// Slice s0049a300: preset placement helpers that adjust a placed object's transform-panel position
// (0xd8/0xdc) for a preset id (0..7, 9), plus the text-rect layout wrapper.
#include <math.h>
#include "rgl.h"

using rgl::vector4;

struct tstring {                                       // see s004520d0
    int pos; char* buf; int cap; int len;
    tstring();                                         // 0x42bbd0
};
struct Font {
    void layout_text(float* out, const tstring* text, const vector4* pos_in, const float* scale_in, void* m,
                     int arg, const float* xmin, const float* xmax, unsigned flags);  // 0x4520d0 (ret 0x24)
};
extern Font* g_font;                                  // 0x5e2934

extern const float g_f0c590cd0;                       // 0x590cd0
extern const float g_0c592728;                        // 0x592728
extern const float g_0c59275c;                        // 0x59275c
extern const float g_0c592730;                        // 0x592730
extern const float g_0c54b868;                        // 0x54b868

struct Obj;
void terrain_refresh(Obj* o, const float* pos);       // 0x48f120 (thiscall, ret 4)
void terrain_refresh_mesh(Obj* o, const float* pos);  // 0x498420 (thiscall, ret 4)
void fade_scale(void* sub, vector4 v);                // 0x47af30 (thiscall)

struct Obj {
    virtual void v00v();
    virtual void v04v();
    virtual void v08v();
    virtual void refresh();                           // +0x0c
    virtual void d10(); virtual void d14(); virtual void d18(); virtual void d1c();
    virtual void d20(); virtual void d24(); virtual void d28(); virtual void d2c();
    virtual void d30(); virtual void d34(); virtual void d38(); virtual void d3c();
    virtual void d40(); virtual void d44(); virtual void d48(); virtual void d4c();
    virtual void d50(); virtual void d54(); virtual void d58(); virtual void d5c();
    virtual void d60();
    virtual void copy64(const vector4* p);            // +0x64
    virtual void d68();
    virtual void scale_vec6c(const vector4* p);       // +0x6c
    virtual void scale_vec70(const vector4* p);       // +0x70
    virtual void d74(); virtual void d78(); virtual void d7c();
    virtual void d80(); virtual void d84(); virtual void d88(); virtual void d8c();
    virtual void d90(); virtual void d94(); virtual void d98(); virtual void d9c();
    virtual void da0();
    virtual void copy16(const void* src, void* dst);  // +0xa4
    virtual void da8(); virtual void dac(); virtual void db0();
    virtual void db4(); virtual void db8(); virtual void dbc();
    virtual int kind();                               // +0xc0
    char padc4[0xc8 - 0x4];
    int f_c8;                                         // +0xc8
    int f_cc;                                         // +0xcc
    char padd0[0xd8 - 0xd0];
    float dx8;                                        // +0xd8
    float dxc;                                        // +0xdc
    char pade0[0xf4 - 0xe0];
    float fy4;                                        // +0xf4
    char padf8[0x108 - 0xf8];
    int f_108;                                        // +0x108
    int f_10c;                                        // +0x10c
    int f_110;                                        // +0x110
    int f_114;                                        // +0x114
    char pad118[0x128 - 0x118];
    vector4 v128;                                     // +0x128
    char pad138[0x194 - 0x138];
    int f194;                                         // +0x194
    vector4 v198;                                     // +0x198
};
struct Rec {
    char pad0[0x1c8];
};
struct Rec2 {
    char pad0[0x144];
};
struct V2 {
    float x, y;
};
struct V4 {                                           // plain 4-float vector, 4-byte aligned
    float x, y, z, w;
};
struct Mission {
    char pad0[4];
    Obj** items;                                      // +0x4
    char pad8[0xf0 - 0x8];
    V2 vf0;  // +0xf0 (2 floats)
    V2 vf8;  // +0xf8 (2 floats)
    char pad100[0x118 - 0x100];
    Rec2* m118;                                       // +0x118
    char pad11c[0x122 - 0x11c];
    char f122;                                        // +0x122
    char pad123[0x124 - 0x123];
    int f124;                                         // +0x124: Font* override (0 -> g_font)
    char pad128[0x140 - 0x128];
    V4 v140;                                     // +0x140
    int f14c;                                         // +0x14c
    char pad150[0x1c8 - 0x150];
    void apply_preset(int idx, const float* pos);                // 0x49a300 (ret 8)
    void apply_preset_b(int idx, const float* pos);              // 0x49ab80 (ret 8)
    void layout_text_to(const tstring* text, const float* pos);  // 0x49abc0 (ret 8)
};

static __forceinline void scale_by_old(Obj* o, int suboff, const float* pos)
{
    fade_scale(*(void**)((char*)o + suboff), vector4(pos[0] / o->dx8, pos[1] / o->dxc, 1.0f));
}

// @ 0x49a300
#define ITEM (*(Obj**)((char*)items + i4))
void Mission::apply_preset(int idx, const float* pos)
{
    int i4 = idx * 4;
    if (ITEM->kind() == 0)
        goto base;
    if (ITEM->kind() == 1)
        goto base;
    if (ITEM->kind() == 9) {
        scale_by_old(ITEM, 0x110, pos);
        scale_by_old(ITEM, 0x10c, pos);
        ITEM->copy64((const vector4*)pos);
        ITEM->copy16(&v140, &ITEM->v128);
        ITEM->dx8 = pos[0];
        ITEM->dxc = pos[1];
        return;
    }
    if (ITEM->kind() == 4) {
        scale_by_old(ITEM, 0x108, pos);
        goto refresh_store;
    }
    if (ITEM->kind() == 8) {
        scale_by_old(ITEM, 0x110, pos);
        scale_by_old(ITEM, 0x10c, pos);
        goto refresh_store;
    }
    if (ITEM->kind() == 2) {
        scale_by_old(ITEM, 0x108, pos);
        scale_by_old(ITEM, 0x110, pos);
        goto refresh_copy_store;
    }
    if (ITEM->kind() == 3) {
        ITEM->v198 = vector4(pos[0] * 0.38940808f, pos[1], 1.0f);
        ITEM->f194 = (int)0xcb3c57cb;
        scale_by_old(ITEM, 0x108, pos);
        scale_by_old(ITEM, 0x110, pos);
        goto refresh_copy_store;
    }
    if (ITEM->kind() == 5) {
        scale_by_old(ITEM, 0x110, pos);
        scale_by_old(ITEM, 0x10c, pos);
        scale_by_old(ITEM, 0x114, pos);
        ITEM->scale_vec6c(&vector4(pos[0] * 0.02f, pos[1] * 0.02f, 1.0f));
        ITEM->scale_vec70(&vector4(0.0f, pos[1] * 0.013f, 1.0f));
        ITEM->refresh();
        ITEM->dx8 = pos[0];
        ITEM->dxc = pos[1];
        ITEM->copy16(&v140, &ITEM->v128);
        return;
    }
    if (ITEM->kind() == 6) {
        scale_by_old(ITEM, 0x110, pos);
        scale_by_old(ITEM, 0x10c, pos);
        ITEM->scale_vec6c(&vector4(pos[0] * 0.02f, pos[1] * 0.02f, 1.0f));
        ITEM->scale_vec70(&vector4(0.0f, pos[1] * 0.009f, 1.0f));
        goto refresh_store;
    }
    if (ITEM->kind() == 7) {
        terrain_refresh(ITEM, pos);
        return;
    }
    return;
base:
    ITEM->scale_vec6c(&vector4(pos[0] * 0.02f, pos[1] * 0.02f, 1.0f));
    ITEM->dx8 = pos[0];
    ITEM->dxc = pos[1];
    return;
refresh_store:
    ITEM->refresh();
    ITEM->dx8 = pos[0];
    ITEM->dxc = pos[1];
    return;
refresh_copy_store:
    ITEM->refresh();
    ITEM->copy64(&vector4(pos[0] * 0.25f, ITEM->fy4, 1.0f));
    ITEM->dx8 = pos[0];
    ITEM->dxc = pos[1];
}
#undef ITEM

// @ 0x49ab80
void Mission::apply_preset_b(int idx, const float* pos)
{
    int i4 = idx * 4;
    if ((*(Obj**)((char*)items + i4))->kind() == 7)
        terrain_refresh_mesh(*(Obj**)((char*)items + i4), pos);
}

// @ 0x49abc0
void Mission::layout_text_to(const tstring* text, const float* pos)
{
    float out[2];
    int arg = -1;
    out[1] = 0.0f;
    if (f124) {
        vector4 p;
        p.x = pos[0];
        p.y = pos[1];
        p.z = 0.0f;
        g_font->layout_text(out, text, &p, &v140.x, m118, (int)&arg, &vf8.x, &vf0.x, 0);
    } else {
        vector4 p;
        p.x = pos[0];
        p.y = pos[1];
        p.z = 0.0f;
        g_font->layout_text(out, text, &p, &v140.x, m118, (int)&arg, &vf0.x, &vf8.x, 0);
    }
    if (f122) {
        int one = 1;
        *(int*)((char*)m118 + 0x114) = one;
        *(int*)((char*)m118 + 0x140) = one;
    }
}
