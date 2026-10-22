// clang-format off
// Slice s004f1460: the character face-generator window -- its initialiser (0x4f1460) and the terrain
// "region type" name table (0x4f2610).  The rest of the window's members (0x4f2730, 0x4f28a0, 0x4f28f0,
// 0x4f2960, 0x4f29b0, 0x4f2a10, 0x4f2ae0) are incomplete; see partial.txt.  Widget slots, the tstring
// helpers and the resource manager follow the verified neighbours (s0048add0, s004dfd50, s004e05c0,
// s004dd830).  Layouts and names are provisional.
#include <windows.h>
#include <stdlib.h>
#include <math.h>
#include <new>
#include <vector>

using std::vector;

// ---- engine types -----------------------------------------------------------------------------------
// rgl::string: 16 bytes { pos, buf, cap, len }; ctor 0x42bbd0 zeroes the four words.
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring();                                              // 0x42bbd0
    explicit tstring(const char* s);                        // 0x42bbe0
    ~tstring();                                             // 0x42bc40
};

// cloned rgl mesh; only the members this slice reaches
struct mesh {
    mesh* clone();                                          // 0x41c6d0
    void scale(float f);                                    // 0x44f0c0
};

// resource manager object at 0x5d6fa0
struct resmgr {
    mesh* get_object(const tstring& name);                  // 0x419540 (ret 4)
};
extern resmgr g_alloc;                                      // 0x5d6fa0

// world / terrain object at 0xa76630 (the terrain generator)
struct World {
    void gen_reset();                                       // 0x4647e0
};
extern World g_world;                                       // 0xa76630

// the renderer's view frustum: 0x344-byte record, ctor 0x478ad0, set_fov 0x488cd0
struct frustum_t {
    char pad0[0x108];
    float f108, f10c, f110, f114, f118, f11c;               // +0x108
    char pad120[0x344 - 0x120];
    frustum_t(float fov, float aspect, float zmin, float zmax);     // 0x478ad0
    void set_fov(float h, float v, float a, float b, float c);      // 0x488cd0
};

// ---- the widget interface (slot offsets in comments) -----------------------------------------------
struct tvec2 {
    float x, y;
};

struct UiWidget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21();
    virtual bool is_clicked();                              // +0x30
    virtual void d23(); virtual void d24(); virtual void d25();
    virtual void move_a(tvec2* p);                          // +0x58
    virtual void d27(); virtual void d28();
    virtual void move_f0(tvec2* p);                         // +0x64
    virtual void d30();
    virtual void set_value(float v);                        // +0x68
    virtual void set_size(tvec2* p);                        // +0x6c
    virtual void set_pos(tvec2* p);                         // +0x70
    virtual void d33(); virtual void d34(); virtual void d35();
    virtual void set_mesh(mesh* m, int layer);              // +0x7c
    virtual void set_track(int i, void* p);                 // +0x80
    virtual void d38(); virtual void d39(); virtual void d40(); virtual void d41(); virtual void d42();
    virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46(); virtual void d47();
    virtual void d48(); virtual void d49(); virtual void d50(); virtual void d51(); virtual void d52();
    virtual void d53(); virtual void d54(); virtual void d55(); virtual void d56();
    virtual void set_label(const tstring& s, tvec2* pos);   // +0xa0
    virtual void set_text(void* out, const tstring* name);  // +0xa4
    virtual void d58(); virtual void d59(); virtual void d60(); virtual void d61(); virtual void d62();
    virtual void d63(); virtual void d64(); virtual void d65(); virtual void d66();
    virtual void add_child(void* child);                    // +0xc8
};

// slider (ctor 0x48b4b0, 0x188 bytes; see s0048add0)
struct UiSlider : UiWidget {
    char pad[0x188 - 4];
    UiSlider();
};

// a cloned, scaled mesh (the original inlines these two steps)
static __forceinline mesh* load_mesh(const char* name, float s)
{
    tstring n(name);
    mesh* m = g_alloc.get_object(n)->clone();
    m->scale(s);
    return m;
}

// the face-generator window ("this"); every field is reached by its explicit offset
struct facegen_win : UiWidget {
    char pad0[0x68 - 4];
    UiWidget* board;                                        // +0x68
    UiWidget* panel;                                        // +0x6c
    char pad70[0x1ac - 0x70];
    void* terrain;                                          // +0x1ac
    UiWidget* panel_center;                                 // +0x1b0
    UiWidget* key;                                          // +0x1b4
    UiWidget* copy;                                         // +0x1b8
    UiWidget* paste;                                        // +0x1bc
    UiSlider* sliders[6];                                   // +0x1c0
    UiSlider* seeds[3];                                     // +0x200
    float f20c[3];                                          // +0x20c
    UiWidget* w218;                                         // +0x218
    UiWidget* w21c;                                         // +0x21c
    UiWidget* w220;                                         // +0x220
    UiWidget* w224;                                         // +0x224
    UiWidget* region_type;                                  // +0x228
    UiWidget* region_detail;                                // +0x22c
    UiWidget* generate;                                     // +0x230
    UiWidget* close;                                        // +0x234

    void init();                                            // 0x4f1460
};

// ---- the other classes the window is made of -------------------------------------------------------
struct UiWindow : UiWidget {                                // ctor 0x4931a0, vptr 0x592848, 0x190 bytes
    char pad[0x188 - 4];
    char f188;
    UiWindow();
};
struct UiContainer : UiWidget {                             // ctor 0x4931a0, 0x188 bytes
    char pad[0x188 - 4];
    UiContainer();
};
struct FaceBoard : UiWidget {                               // ctor 0x4931a0 (+ vptr 0x597050), 0x188 bytes
    char pad[0x188 - 4];
    FaceBoard();
};

// ---- helpers / globals ------------------------------------------------------------------------------
extern char g_gui[];                                        // 0x5d7080 (the GUI root object)
void gui_reset(char* g);                                    // 0x42d1b0
void gui_push(char* g, void** w);                           // 0x49b120 (appends to the root widget list)
extern unsigned char g_gui_dirty;                           // 0x5d8788
extern int g_region_names[];                                // 0x59394c, 8 entries ending at 0x59396c

extern "C" int __cdecl sprintf(char*, const char*, ...);    // 0x536f8f
extern "C" int __cdecl rand(void);                          // 0x536a9f

UiSlider* make_slider(void* parent, float x, float y);                                  // 0x4dfd50
UiWidget* make_option(void* parent, const tstring* name, float x, float y);             // 0x4dfec0
UiWidget* make_medium(void* parent, tstring* a, float x, float y, float w, float h,
                      tstring* b, tstring* c);                                          // 0x4dcc50
UiWidget* make_short(void* parent, tstring* a, float x, float y, float w, float h,
                     tstring* b, tstring* c);                                           // 0x4dada0

// @ 0x4f1460
void facegen_win::init()
{
    int i;
    for (i = 0; i < 3; i++)
        *(int*)((char*)this + 0x54 + i * 4) = rand() % 0x1b5781;
    g_world.gen_reset();

    *(float*)((char*)this + 0x3c) = 150.0f;
    *(float*)((char*)this + 0x40) = 150.0f;
    *(float*)((char*)this + 0x44) = 100.0f;
    *(float*)((char*)this + 0x48) = 0.5f;
    *(int*)((char*)this + 0x50) = 30;
    *(float*)((char*)this + 0x4c) = 0.8f;
    *(int*)((char*)this + 0x60) = 3;

    gui_reset(g_gui);

    UiWindow* w = new UiWindow;
    board = w;
    { tvec2 p = { 0.0f, 0.0f }; w->move_a(&p); }
    { tvec2 s = { 1.0f, 0.75f }; w->move_f0(&s); }
    gui_push(g_gui, (void**)&board);

    UiContainer* c = new UiContainer;
    panel = c;
    { tvec2 p = { 0.5f, 0.0f }; c->move_a(&p); }
    { tvec2 s = { 0.5f, 0.75f }; c->move_f0(&s); }
    c->set_mesh(load_mesh("facegen_board", 1.0f), 0);
    { tvec2 p = { 0.017f, 0.61f }; c->set_label(tstring(""), &p); }
    { tvec2 s = { 0.02f, 0.02f }; c->set_size(&s); }

    w218 = make_medium(panel, &tstring("Shade occluded:"), 0.24f, 0.62f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));
    w220 = make_medium(panel, &tstring("Place river:"), 0.24f, 0.54f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));
    w21c = make_medium(panel, &tstring("Disable grass:"), 0.24f, 0.58f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));
    w224 = make_medium(panel, &tstring("Deep water:"), 0.24f, 0.50f, 0.7f, 0.3f,
                       &tstring("medium_button_down"), &tstring("medium_button"));

    for (i = 0; i < 3; i++) {
        seeds[i] = make_slider(panel, 0.72f, 0.22f + (float)i * 0.055f);
        seeds[i]->set_text(&tstring(""), &tstring("randomize"));
    }

    for (i = 0; i < 6; i++) {
        sliders[i] = make_slider(panel, 0.1f, 0.83f - (float)i * 0.088f);
        sliders[i]->set_text(&tstring(""), &tstring(""));
    }

    region_type = make_option(panel, &tstring("region type"), 0.79f, 0.9f);
    region_detail = make_option(panel, &tstring("region detail"), 0.79f, 0.9f);
    (void)region_detail;

    key = make_short(panel, &tstring("key"), 0.79f, 0.697f, 0.1f, 0.5f, &tstring(""),
                     &tstring(""));
    copy = make_short(panel, &tstring("Copy"), 0.616f, 0.68f, 3.0f, 0.3f, &tstring(""),
                      &tstring(""));
    paste = make_short(panel, &tstring("Paste"), 0.616f, 0.68f, 3.0f, 0.3f, &tstring(""),
                       &tstring(""));

    generate = make_medium(panel, &tstring("Generate Terrain"), 0.571f, 0.035f, 0.07f, 3.0f,
                           &tstring(""), &tstring(""));
    close = make_medium(panel, &tstring("Close"), 0.07f, 0.028f, 0.33f, 3.0f, &tstring(""),
                        &tstring(""));
}

// @ 0x4f2610
void region_name(int i, char* out)
{
    switch (i) {
    case 0: sprintf(out, "Water"); break;
    case 1: sprintf(out, "Mountain"); break;
    case 2: sprintf(out, "Steppe"); break;
    case 3: sprintf(out, "Plain"); break;
    case 4: sprintf(out, "Snow"); break;
    case 5: sprintf(out, "Desert"); break;
    case 8: sprintf(out, "River"); break;
    case 9: sprintf(out, "Mountain Forest"); break;
    case 10: sprintf(out, "Steppe Forest"); break;
    case 11: sprintf(out, "Forest"); break;
    case 12: sprintf(out, "Snow Forest"); break;
    }
}
