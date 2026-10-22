// Slice s004f2d70: the terrain "edit" page's per-frame update and its "copy terrain code" button.
//   0x4f2d70  update(this, dt): first the six (checkbox, number) pairs of the three terrain slots and the
//             per-slot selection checkboxes, then exactly one changed-widget chain (terrain flags, the six
//             sliders, terrain type/size, copy/paste), then the "regenerate" button, then the world step.
//   0x4f34c0  copy the hex terrain code (the text of the page's hex field) to the Windows clipboard.
// All names provisional. The page layout is the one recovered in slice s004f25a0 (TerrainPage); the page is a
// widget page whose fields start at +0x18 (the option the menu returns to).
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "rgl.h"

using rgl::vector4;
struct vector2;                                             // forward: keeps <math.h> ahead of it (see below)

// NOTE (slot numbering): cl 13.10 numbers a class's virtuals in DECLARATION order, one slot per declaration,
// but two families are numbered from a base of their own: a parameter given by reference (const vector2&)
// starts 0x24 past the (empty) family, and the (int) family starts 0x24 past too.  So a virtual's slot is
// "#declarations since its own last representative of that family", not its declaration index: that is why the
// binary's (int) accessor changed(0x10000) is at +0x44 (the 2nd (int) after activate) while the getters at
// +0x30/+0x38 are the 3rd/4th () after v00.  The accessors are therefore placed a few declarations apart in
// the block below so that the emitted offsets match the binary (see the comments).

// ---- tstring / rgl::string (layouts verified in s004d6b80 / s004f25a0) --------------------------------
struct tstring : rgl::string {
    tstring(const char* s) : rgl::string(s) {}                              // 0x42bbe0
    tstring();                                                              // 0x42bbd0
    tstring(const tstring& o);                                              // 0x42c110
    tstring& operator=(const tstring& o);                                   // 0x42bcb0
    ~tstring();                                                             // 0x42bc40
    const char* c_str() const { return p ? p : (const char*)&p; }
};

extern "C" int __cdecl sscanf(const char*, const char*, ...);               // 0x536f7c

// ---- controls (layouts verified in s004d5ba0 / s004d63b0 / s004d6b80) ---------------------------------
struct vector2 { float x, y; };
// How this block was measured (cl 13.10, /O2):  within one parameter family (no parameters, (int), a
// reference argument, ...) consecutive virtuals get consecutive slots, and every reference argument family is
// numbered 0x24 above the no-parameter family (see the note above).  The declarations below are laid out so
// that each accessor the page calls compiles to the offset the binary uses:
//
//   0x00  v00 (first virtual; v01..v04 share its slot -> the binary's table is unreachable through any of them)
//   0x28  activate(int)
//   0x30  get_value()            3rd ()
//   0x38  get_float()            4th ()
//   0x44  changed(int)           2nd (int) after activate (declared 7th)
//   0x44  v27/v28 (vector2)
//   0x90  set_default()          31st ()
//   0x94  x94
//   0x98  set_colour(int)
//   0x9c  x9c
//   0xa0  xa0
//   0xa4  put (set_text)
//   0xa8  take (get_text)
//   0xac  put (set_text with a position) -- a reference family of its own
//   0xb0  show
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09();
    virtual void activate(int v);                           // +0x28
    virtual void v0c();
    virtual int  get_value();                               // +0x30
    virtual void v0d();
    virtual float get_float();                              // +0x38
    virtual void v10(); virtual void v11();
    virtual char changed(int flag);                         // +0x44
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v1a(); virtual void v1b(); virtual void v1c(); virtual void v1d();
    virtual void v1e(); virtual void v1f(); virtual void v20(); virtual void v21();
    virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25();
    virtual void v26();
    virtual void v27(const vector2& v);                     // +0x44
    virtual void v28(const vector2& v);                     // +0x48
    virtual void v29(const vector2& v);                     // +0x4c
    virtual void v2a(); virtual void v2b(); virtual void v2c(); virtual void v2d();
    virtual void v2e(); virtual void v2f(); virtual void v30(); virtual void v31(); virtual void v32();
    virtual void v33(); virtual void v34();
    virtual void set_default();                             // +0x90
    virtual void x94();                                     // +0x94 (not called by this page)
    virtual void set_colour(int colour);                    // +0x98
    virtual void x9c(); virtual void x9d(); virtual void x9e();     // 0x9c, 0xa0, (0xa4 in the binary)
    virtual void put(const tstring& s);                     // +0xa4  set_text
    virtual void take(const tstring& out);                  // +0xa8  get_text (reads into `out`)
    virtual void put(const tstring& s, const vector4& pos); // +0xac  set_text(s, pos)
    virtual void show();                                    // +0xb0
    virtual void hide();                                    // +0xb4
    virtual void enable();                                  // +0xb8
    virtual void disable();                                 // +0xbc
};

// ---- terrain bit words (the object at 0xa76630) -------------------------------------------------------
extern unsigned __int64 g_pow2[];                           // 0x5a8e70  g_pow2[n] = 2 << n
struct BitWords {
    unsigned w[8];
    void set(const unsigned& lo, const int& hi, const unsigned& v);   // 0x464960
    unsigned get(const unsigned& lo, const int& hi);                  // 0x4649b0
    void build_hex_text(tstring& out);                                // 0x4f29b0
};
extern BitWords g_bits;                                     // 0xa76630
void terrain_set_a(const float* p);                         // 0x464a60
struct Pct { char pad0[0x10]; unsigned bits; void set_pct(const float& f); };   // 0x4649f0
// 0x4649f0 is a method of this object (the percentage field rides in the low 7 bits of bits at +0x10)
extern Pct g_pct;                                           // 0xa76630

// ---- the world / scene --------------------------------------------------------------------------------
// effect object created by 0x478f30 (0x7c bytes; layout from s00494e40)
struct EffObj {
    int f0;                                                 // +0x00
    vector4 pos;                                            // +0x04
    vector4 f14;                                            // +0x14
    vector4 f24;                                            // +0x24
    char pad34[0x44 - 0x34];
    vector4 f44;                                            // +0x44
    float f54;                                              // +0x54
    char pad58[0x7c - 0x58];
};
struct EffMgr {
    EffObj* create();                                       // 0x478f30
};
struct World {                                              // the page's +0x1ac; scene object 0x421390 resets
    char pad00[0x68];
    vector4 f68;                                            // +0x68
    float   f74;                                            // +0x74
    void reset();                                           // 0x421390
    void step(float dt);                                    // 0x427f40
    EffObj* create_effect() { return ((EffMgr*)this)->create(); }
};
struct terrain_obj {                                        // the object at 0xa76630 (see s00481d10)
    void generate(World* w, int b, int c);                  // 0x481d10
    void regenerate_water(int a, int* out);                 // 0x482170
    void destroy_water();                                   // 0x4814a0
};
struct MainMenu {
    void choose(int option);                                // 0x495700
};

float  clamp_float(const float* x, const float* lo, const float* hi);   // 0x40e3d0
int    round_to_int(float v);                               // 0x535a64 (x87 conversion)
void   free_ptr(void* p);                                   // 0x535712

extern int   g_size_modes[8];                               // 0x59394c terrain-type table (index 0..7)
extern float g_clamp_lo;                                    // 0x593948
extern float g_500;                                         // 0x590b40 (500.0f)
extern float g_140;                                         // 0x598e6c (140.0f)
extern float g_0_01;                                        // 0x54ab28 (0.01f)
extern float g_one;                                         // 0x549d74 (1.0f)
extern float g_half;                                        // 0x549d70 (0.5f)
extern int   g_env2;                                        // 0x5e28fc (shaders on)
extern void* g_a17110;                                      // 0xa17110
extern void* g_a17510;                                      // 0xa17510
extern MainMenu g_main_menu;                                // 0x5e2xxx
extern terrain_obj g_terrain_obj;                           // 0xa76630
extern const char g_fmt_d[];                                // 0x54a0f4 "%d"

// One plain virtual in the class: a class with no virtuals at all makes cl 13.10 lay out every *later*
// polymorphic class with a vfptr+vbtable-style prefix, so the Widget slots come out 0x20 too high.
struct Paged {
    virtual void page_update(float dt);                     // 0x4f2d70
};

struct TerrainPage : Paged {
    char    pad04[0x18 - 0x04];
    int     option;                                         // +0x18 (the option the menu returns to)
    char    pad1c[0x2c - 0x1c];
    int     terrain_on;                                     // +0x2c  "Enable terrain" checkbox
    int     terraingen_on;                                  // +0x30  "Enable terrain generation" checkbox
    int     water_on;                                       // +0x34  "Enable water" checkbox
    int     river_on;                                       // +0x38  "Enable river" checkbox
    float   camera_x;                                       // +0x3c  X position
    float   camera_y;                                       // +0x40  Y position
    float   camera_z;                                       // +0x44  camera height above the ground
    float   fog;                                            // +0x48  fog (0..1)
    float   water_height;                                   // +0x4c  water height (0..1)
    int     water_level;                                    // +0x50  water level
    int     layer_bits[3];                                  // +0x54 / +0x58 / +0x5c  random bits of the water layers
    int     terrain_type;                                   // +0x60  terrain type (index into g_size_modes)
    int     map_size;                                       // +0x64  terrain size
    char    pad68[0x190 - 0x68];
    float   view_x;                                         // +0x190 (view position X)
    float   view_y;                                         // +0x194 (view position Y)
    char    pad198[0x1ac - 0x198];
    World*  world;                                          // +0x1ac (the scene object the page drives)
    char    pad1b0[0x1b4 - 0x1b0];
    Widget* hexbox;                                         // +0x1b4  the hex terrain text field
    Widget* copy_btn;                                       // +0x1b8  copy terrain code
    Widget* paste_btn;                                      // +0x1bc  paste terrain code
    Widget* x_slider;                                       // +0x1c0  X position slider
    Widget* y_slider;                                       // +0x1c4  Y position slider
    Widget* z_slider;                                       // +0x1c8  camera height slider
    Widget* fog_slider;                                     // +0x1cc  fog slider
    Widget* level_slider;                                   // +0x1d0  water level slider
    Widget* height_slider;                                  // +0x1d4  water height slider
    char    pad1d8[0x200 - 0x1d8];
    Widget* bit_fields[3];                                  // +0x200  number field of terrain slot 0..2
    Widget* layer_boxes[3];                                 // +0x20c  select checkbox of terrain slot 0..2
    Widget* water_box;                                      // +0x218  Enable water
    Widget* river_box;                                      // +0x21c  Enable river
    Widget* terrain_box;                                    // +0x220  Enable terrain
    Widget* terraingen_box;                                 // +0x224  Enable terrain generation
    Widget* type_box;                                       // +0x228  terrain type
    Widget* size_box;                                       // +0x22c  terrain size
    Widget* regen_btn;                                      // +0x230  regenerate
    Widget* back_btn;                                       // +0x234  back
    char    pad238[0x2c0 - 0x238];

    void update(float dt);                                  // 0x4f2d70
    void read_fields();                                     // 0x4f3570
    void simulate(float dt);                                // 0x4f37f0
    void path(float dt);                                    // 0x4f3cf0
    void copy_code();                                       // 0x4f34c0
    void paste_code();                                      // 0x4f3c30
};

static inline bool page_changed(Widget* w) { return w->changed(0x10000) != 0; }

// the caption / visibility updates that follow a checkbox change (slice s004f25a0)
void set_enabled_text(Widget* w);                           // 0x4f28a0
void set_hex3_text(int i);                                  // 0x4f28f0
void refresh_hex();                                         // 0x4f2960
void sync_bits();                                           // 0x4f2ae0

// @ 0x4f2d70
void TerrainPage::update(float dt)
{
    // the three terrain slots: a "select" checkbox, random bits, the %d caption of the number field
    for (int i = 0; i < 3; i++) {
        if (page_changed(layer_boxes[i])) {
            layer_bits[i] = rand() % 0x1b5781;
            set_hex3_text(i);
            sync_bits();
            tstring text;
            g_bits.build_hex_text(text);
            hexbox->put(text);
        }
        if (page_changed(bit_fields[i])) {
            tstring text;
            sync_bits();
            bit_fields[i]->take(text);
            int v;
            sscanf(text.c_str(), g_fmt_d, &v);
            int lo, hi;
            if (i == 0)      { lo = 0x00; hi = 0x1e; }
            else if (i == 1) { lo = 0x20; hi = 0x3e; }
            else             { lo = 0x40; hi = 0x5e; }
            g_bits.set(lo, hi, v);
            read_fields();
            this->update(dt);       // refresh every widget from the page fields
        }
    }

    // the two "Enable terrain" checkboxes also recaption themselves, so they form a chain of their own
    if (page_changed(terrain_box) || page_changed(terraingen_box)) {
        Widget* w;
        if (page_changed(terrain_box)) {
            terrain_on = terrain_box->get_value();
            w = terrain_box;
        } else {
            terraingen_on = terraingen_box->get_value();
            w = terraingen_box;
        }
        set_enabled_text(w);
        refresh_hex();
        goto done;
    }

    if (page_changed(water_box) || page_changed(river_box)) {
        Widget* w = page_changed(water_box) ? water_box : river_box;
        if (w == water_box)
            water_on = water_box->get_value();
        else
            river_on = river_box->get_value();
        set_enabled_text(w);
        refresh_hex();
    } else if (page_changed(x_slider)) {
        float v = x_slider->get_float() * g_500 + g_140;
        camera_x = clamp_float(&v, &g_clamp_lo, &g_140);
        refresh_hex();
    } else if (page_changed(y_slider)) {
        float v = y_slider->get_float() * g_500 + g_140;
        camera_y = clamp_float(&v, &g_clamp_lo, &g_140);
        refresh_hex();
    } else if (page_changed(z_slider)) {
        camera_z = z_slider->get_float() * g_0_01 + g_one;
        refresh_hex();
    } else if (page_changed(fog_slider)) {
        fog = fog_slider->get_float();
        refresh_hex();
    } else if (page_changed(level_slider)) {
        float v = level_slider->get_float() * g_0_01;
        water_level = round_to_int(v);
        refresh_hex();
    } else if (page_changed(height_slider)) {
        water_height = height_slider->get_float();
        refresh_hex();
    } else if (page_changed(type_box)) {
        terrain_type = g_size_modes[type_box->get_value()];
        refresh_hex();
    } else if (page_changed(size_box)) {
        map_size = size_box->get_value();
        refresh_hex();
    } else if (page_changed(copy_btn)) {
        copy_code();
    } else if (page_changed(paste_btn)) {
        paste_code();
    }
done:
    if (page_changed(regen_btn)) {
        // "regenerate": reset the scene, arm the flash effect, rebuild the terrain and put the camera on it
        world->reset();
        vector4 clear_colour(0.4f, 0.4f, 0.6f);
        world->f68 = clear_colour;
        world->f74 = 1.0f;
        EffObj* flash = world->create_effect();     // 0x478f30
        flash->pos = vector4(0.9f, 0.9f, 0.9f);
        flash->f14 = vector4(1.0f, 1.0f, 1.0f);
        flash->f24 = vector4(-1000.0f, -1000.0f, 1000.0f);
        flash->f0 = 2;
        flash->f54 = 20.0f;
        vector4 dir(1.0f, 1.0f, -1.0f);
        flash->f44 = dir;
        sync_bits();
        g_terrain_obj.generate(world, 0, 0);
        vector4 owner;                              // 16 zeroed bytes; lane 1 is what 0x4814a0 hands back
        owner.x = 0.0f; owner.y = 0.0f; owner.z = 0.0f; owner.w = 0.0f;
        g_terrain_obj.regenerate_water(0, (int*)&owner.y);
        g_terrain_obj.destroy_water();
        float x = camera_x * g_half;
        view_x = x;
        float y = camera_y * g_half;
        view_y = y;
        if (owner.y != 0.0f)
            free_ptr((void*)(int)owner.y);
    }
    simulate(dt);
    world->step(dt);
    path(dt);
    if (page_changed(back_btn) || (g_a17110 != 0 && g_a17510 == 0))
        g_main_menu.choose(option);
}

// @ 0x4f34c0
void TerrainPage::copy_code()
{
    tstring s;
    hexbox->take(s);
    if (OpenClipboard(0)) {
        // GMEM_MOVEABLE | GMEM_ZEROINIT; a failure just leaves the clipboard closed again
        HGLOBAL h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, s.size + 1);
        if (h) {
            EmptyClipboard();
            char* p = (char*)GlobalLock(h);
            const char* src = s.p;
            if (src == 0)
                src = (const char*)&s.p;            // an empty string reads as the string object itself
            memcpy(p, src, s.size);
            p[s.size] = 0;
            GlobalUnlock(h);
            SetClipboardData(CF_TEXT, h);
        }
        CloseClipboard();
    }
}
