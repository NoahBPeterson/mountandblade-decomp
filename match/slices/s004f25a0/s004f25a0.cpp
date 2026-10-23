// Slice s004f25a0: the "edit/scene" terrain page.
//   0x4f2610  name of a terrain ground type ("Water", "Mountain", ... — same table as s00484590)
//   0x4f2730  page update: refresh every terrain widget from the page fields, then the hex field
//   0x4f28a0  caption a terrain checkbox with "Enabled" / "Disabled"
//   0x4f28f0  caption a terrain number field with "%d"
//   0x4f2a10  format one word as eight hex digits (by hand: digits + 'A'..'F')
//   0x4f2960  build the hex terrain string and hand it to the text box
//   0x4f29b0  build the hex terrain string from the six terrain words (most significant first)
//   0x4f2ae0  write the page values into the terrain bit words at 0xa76630 (the s00464a60 terrain object)
// All names provisional. The page layout follows s004d6b80's video page (+0x18 option, +0x1b4 text box).
//
// 0x4f25a0 (a frustum init) is not a member of any class found so far: the function is a __thiscall on an
// object with a flag at +0, four floats at +0x108..+0x114, near/far at +0x118/+0x11c and a plane array built
// by 0x41e4f0. It sits in this slice only because of its address (see partial.txt).
#include <windows.h>
#include <stdio.h>
#include <math.h>
#pragma intrinsic(sqrt, fabs, sin, cos, tan, fsin, fcos, fptan)
#include "rgl.h"

using rgl::vector4;

// ---- tstring / rgl::string ---------------------------------------------------------------------------
struct tstring : rgl::string {
    tstring(const char* s) : rgl::string(s) {}                              // 0x42bbe0
    tstring();                                                              // 0x42bbd0
    tstring& assign(const char* s);                                         // 0x42bcb0 (the const char* assignment)
    tstring& append(const tstring& o);                                      // 0x42c1e0
    ~tstring();                                                             // 0x42bc40
    const char* c_str() const { return p ? p : (const char*)&p; }
};
extern "C" int __cdecl sprintf(char*, const char*, ...);                    // 0x536f8f
extern "C" int __cdecl sscanf(const char*, const char*, ...);               // 0x535fe4

// ---- controls (layouts verified in s004d5ba0 / s004d63b0 / s004d6b80) --------------------------------
// set_text returns an 8-byte object through a hidden pointer that no caller destroys: a non-POD pair (a user
// constructor, no destructor). 0x4f28a0/0x4f28f0/0x4f2960 reserve exactly 8 bytes for it.
struct WRet { int a, b; WRet(); };
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a();
    virtual void activate(int v);                           // +0x2c (index 11)
    virtual int  get_value();                               // +0x30 (index 12)
    virtual void v0d();                                     // +0x34 (index 13)
    virtual float get_float();                              // +0x38 (index 14)
    virtual void v0f(); virtual void v10();                 // indices 15, 16
    virtual char changed(int flag);                         // +0x44 (index 17)
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v1a();
    virtual void set_pos(const vector4& v);                 // +0x6c (index 27)
    virtual void set_size(const vector4& v);                // +0x70 (index 28)
    virtual void v1d(); virtual void v1e(); virtual void v1f(); virtual void v20();
    virtual void v21(); virtual void v22(); virtual void v23();
    virtual void set_default();                             // +0x90 (index 36)
    virtual void v25();                                     // index 37
    virtual void set_colour(int colour);                    // +0x98 (index 38)
    virtual void v27(); virtual void v28();                 // indices 39, 40
    virtual WRet set_text(const tstring& s);                // +0xa4 (index 41)
    virtual void set_text2(const tstring& s, const vector4& pos);  // +0xa8 (index 42)
    virtual void v2b();                                     // index 43
    virtual void show_v();                                  // +0xb0 (index 44)
    virtual void hide_v();                                  // +0xb4 (index 45)
    virtual void enable_v();                                // +0xb8 (index 46)
    virtual void disable_v();                               // +0xbc (index 47)
};
// the range widget: +0x34 is the value setter (index 13)
struct RangeWidget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v0a(); virtual void v0b();
    virtual void v0c();
    virtual void set_value(float v);                        // +0x34 (index 13)
};

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
extern unsigned __int64 g_pow2[];                           // 0x5a8e70  g_pow2[n] = 2 << n
extern const int g_mask_w[1];                               // 0x5a8e78 (= 2: the terrain size is 2 bits wide)
extern const int g_type_mask[1];                            // 0x5a8e88 (= 4: the terrain type is 4 bits wide)
struct BitWords {
    unsigned w[8];
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
    void set_pct(const float* p);                                     // 0x4649f0
    void set_a(const float* p);                                       // 0x464a60
    void set_b(const float* p);                                       // 0x464ac0
    void build_hex_text(tstring& out);                                // 0x4f29b0
};
extern BitWords g_bits;                                     // 0xa76630

extern int g_env2;                                          // 0x5e28fc (shaders on)
extern int g_size_modes[8];                                 // 0x59394c terrain-size mode table (only [0] is ours)

// ---- the page -----------------------------------------------------------------------------------------
struct TerrainPage {
    char    pad00[0x18];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x2c - 0x1c];
    int     enabled1;                                       // +0x2c  "Enable terrain" checkbox
    int     enabled2;                                       // +0x30  "Enable terrain generation" checkbox
    int     enabled3;                                       // +0x34  "Enable water" checkbox
    int     water2;                                         // +0x38  "Enable river" checkbox
    float   f3c;                                            // +0x3c  X position
    float   f40;                                            // +0x40  Y position
    float   f44;                                            // +0x44  camera height above the ground
    float   f48;                                            // +0x48  fog (0..1)
    float   f4c;                                            // +0x4c  water height (0..1)
    int     water_level;                                    // +0x50  water level, written into bits 0x8e..0x94
    int     hex3[3];                                        // +0x54 / +0x58 / +0x5c  random bits of the water layers
    int     type;                                           // +0x60  terrain type (4 bits)
    int     size;                                           // +0x64  terrain size (2 bits)
    char    pad68[0x1ac - 0x68];
    int*    obj1ac;                                         // +0x1ac (the world object, used by FUN_004f2d70 only)
    int     f1b0;                                           // +0x1b0 (unknown; 0x4f2960 reads the text box at +0x1b4)
    Widget* hexbox;                                         // +0x1b4  the hex terrain text field
    char    pad1b8[0x1c0 - 0x1b8];
    RangeWidget* btn1c0;                                    // +0x1c0  X position
    RangeWidget* btn1c4;                                    // +0x1c4  Y position
    RangeWidget* btn1c8;                                    // +0x1c8  camera height
    RangeWidget* btn1cc;                                    // +0x1cc  fog
    RangeWidget* btn1d0;                                    // +0x1d0  water height
    RangeWidget* btn1d4;                                    // +0x1d4  water level
    char    pad1d8[0x200 - 0x1d8];
    Widget* num2[3];                                        // +0x200  number fields for hex3[0..2]
    Widget* box3[3];                                        // +0x20c  the bit-field strips of hex3[0..2]
    Widget* w218;                                           // +0x218  Enable water
    Widget* w21c;                                           // +0x21c  Enable river
    Widget* w220;                                           // +0x220  Enable terrain
    Widget* w224;                                           // +0x224  Enable terrain generation
    Widget* w228;                                           // +0x228  terrain size
    Widget* w22c;                                           // +0x22c  terrain type
    Widget* w230;                                           // +0x230  regenerate
    Widget* w234;                                           // +0x234  back
    char    pad238[0x2c0 - 0x238];

    void update();                                          // 0x4f2730
    __declspec(noinline) void set_enabled_text(Widget* w);                       // 0x4f28a0
    __declspec(noinline) void set_hex3_text(int i);                              // 0x4f28f0
    void refresh_hex();                                     // 0x4f2960
    bool changed(Widget* w) { return w->changed(0x10000); } // +0x44 (the page's change flag)
    void sync_bits();                                       // 0x4f2ae0
    void copy_clipboard();                                  // 0x4f34c0
    void paste_clipboard();                                 // 0x4f3c30
    void read_fields();                                     // 0x4f3570
    void simulate(float dt);                                // 0x4f37f0
    void path(float dt);                                    // 0x4f3cf0
};

void format_hex(unsigned v, char* out);                     // 0x4f2a10 (argument order recovered from the codegen)
void f4f3570(TerrainPage* p);                               // 0x4f3570
void f4f34c0(TerrainPage* p);                               // 0x4f34c0
void f4f3c30(TerrainPage* p);                               // 0x4f3c30
void f4f37f0(TerrainPage* p, float dt);                     // 0x4f37f0
void f4f3cf0(TerrainPage* p, float dt);                     // 0x4f3cf0

extern const float g_half;                                  // 0x549d70 (0.5f)
extern const float g_01;                                    // 0x54a6f0 (0.01f)
extern const float g_140;                                   // 0x598e6c (140.0f)
extern const float g_00143;                                 // 0x598e68 (0.0014285714f)
extern const float g_10;                                    // 0x549d74 (10.0f)

extern const char g_fmt_d[];                                // 0x54a0f4 "%d"
extern const char g_str_enabled[];                          // 0x598e7c "Enabled"
extern const char g_str_disabled[];                         // 0x598e70 "Disabled"
extern const char g_hex_sep[];                              // 0x598e84 " "

// @ 0x4f25a0
// A frustum init (a __thiscall on an object whose first field is a flag, with four parameters at
// +0x108..+0x114, near/far at +0x118/+0x11c and the plane array built by 0x41e4f0), not a member of this page.
// Modelled on a stub class; see partial.txt.
struct Frustum {
    char  flag;                                             // +0x000
    char  pad01[0x107];
    float l, r, b, t;                                       // +0x108  half-extent parameters
    float near_;                                            // +0x118
    float far_;                                             // +0x11c
    void init(float fov, float aspect, float near_, float far_);   // 0x4f25a0
    void build_planes();                                    // 0x41e4f0
};

void Frustum::init(float fov, float aspect, float near_, float far_)
{
    flag = 1;
    this->far_ = far_;
    float v = (float)tan(fov * g_half);
    this->near_ = near_;
    l = -(near_ * v);
    r = v * near_;
    t = -((near_ * v) * aspect);
    b = (v * near_) * aspect;
    build_planes();
}

// @ 0x4f2610
const char* terrain_name(unsigned int type)
{
    switch (type) {
    case 0:  return "Water";
    case 1:  return "Mountain";
    case 2:  return "Steppe";
    case 3:  return "Plain";
    case 4:  return "Snow";
    case 5:  return "Desert";
    case 8:  return "River";
    case 9:  return "Mountain Forest";
    case 10: return "Steppe Forest";
    case 11: return "Forest";
    case 12: return "Snow Forest";
    }
    return 0;
}

// @ 0x4f2730
void TerrainPage::update()
{
    w218->activate(enabled3);
    set_enabled_text(w218);
    w21c->activate(water2);
    set_enabled_text(w21c);
    w220->activate(enabled1);
    set_enabled_text(w220);
    w224->activate(enabled2);
    set_enabled_text(w224);

    int mode = -1;
    for (int i = 0; mode == -1; i++) {
        if (i >= 8)
            break;
        if (g_size_modes[i] == type)
            mode = i;
    }
    if (mode >= 0)
        w228->activate(mode);
    w22c->activate(size);

    btn1c0->set_value((f3c - g_140) * g_00143);
    btn1c4->set_value((f40 - g_140) * g_00143);
    btn1c8->set_value((f44 - g_10) * g_01);
    btn1cc->set_value(f48);
    btn1d0->set_value((float)water_level * g_01);
    btn1d4->set_value(f4c);

    set_hex3_text(0);
    set_hex3_text(1);
    set_hex3_text(2);
}
// @ 0x4f28a0
void TerrainPage::set_enabled_text(Widget* w)
{
    if (w->get_value())
        w->set_text(tstring("Enabled"));
    else
        w->set_text(tstring("Disabled"));
}

// @ 0x4f28f0
void TerrainPage::set_hex3_text(int i)
{
    char buf[512];
    sprintf(buf, g_fmt_d, hex3[i]);
    tstring text(buf);
    num2[i]->set_text(text);
}

// @ 0x4f2960
void TerrainPage::refresh_hex()
{
    sync_bits();
    tstring text;
    g_bits.build_hex_text(text);            // 0x4f29b0, this = the bit-word object at 0xa76630
    hexbox->set_text(text);
}

// @ 0x4f29b0
void BitWords::build_hex_text(tstring& out)
{
    char buf[12];
    out.assign(g_hex_sep);
    for (int i = 5; i >= 0; i--) {
        format_hex(w[i], buf);
        out.append(tstring(buf));
    }
}

// @ 0x4f2a10
void format_hex(unsigned v, char* out)
{
    for (int i = 7; i >= 0; i--) {
        unsigned c = v & 0xf;
        v >>= 4;
        char d = (c < 10) ? (char)(c + '0') : (char)(c + 'W');
        out[i] = d;
    }
    out[8] = 0;
}

// @ 0x4f2ae0
void TerrainPage::sync_bits()
{
    g_bits.set(0x7e, 0x7e, enabled3 != 0);
    g_bits.set(0xa2, 0xa3, water2);
    g_bits.set(0x7f, 0x7f, enabled1 != 0);
    g_bits.set(0x3f, 0x3f, enabled2 != 0);
    g_bits.set(0x60, 0x69, (unsigned)(int)f3c);
    g_bits.set(0x6a, 0x73, (unsigned)(int)f40);
    g_bits.set_a(&f44);
    g_bits.set_pct(&f48);
    g_bits.set(0, 0x1e, hex3[0]);
    g_bits.set(0x20, 0x3e, hex3[1]);
    g_bits.set(0x40, 0x5e, hex3[2]);
    g_bits.set(0x8e, 0x94, water_level);
    g_bits.set_b(&f4c);
    g_bits.set(0x9c, 0x9f, type);
    g_bits.set(0xa0, 0xa1, size);
}
