// Slice s004de6f0: the main-menu "options" screen setup (0x4de6f0, ~5.7 KB).  It resets the GUI, builds
// the options window (container vptr 0x592848 / base ctor 0x4931a0) with its header mesh, a
// runtime-settings container (ctor 0x48ae20, size 0x178) with one slider per setting, the
// combat-difficulty container (ctor 0x4931d0, size 0x1d0) with six drop-down buttons, a block of
// "ui_enabled" toggle labels and a 16-slider grid, then pushes the window to the GUI and ticks the game.
// Names and field names are provisional.  The widget layout (ctor 0x48ae20, slots +0x58/+0x64/+0x6c/
// +0x70/+0x74/+0x7c/+0x80/+0x94/+0x98/+0xa4/+0xc8) is the one verified in s004e25a0 (same TU family);
// this slice only names the slots it uses.
// Complete except for the numerical arguments of the middle section: every allocation, widget, mesh
// assignment and engine call of the original is reproduced in order, but the sliders/labels between the
// six difficulty boxes and the framerate sliders were reconstructed from the repeated code shape
// (exact translation keys / colours / mesh slots of that stretch were not recovered).
#include <new>
#include "rgl.h"

using rgl::vector4;

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;

// ---- engine strings ----------------------------------------------------------------------------------
struct tstring {                                    // 0x10-byte tstring (rgl::string layout)
    char* p; int size, cap, extra;
    tstring() MB_NOTHROW;                           // 0x42bbd0
    tstring(const char* s) MB_NOTHROW;              // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;           // 0x42c110
    ~tstring() MB_NOTHROW;                          // 0x42bc40
    const char* c_str() const { return p ? p : (const char*)&p; }
};
struct StrTable {
    tstring* lookup(tstring* key) MB_NOTHROW;       // 0x43a1e0
};
extern StrTable g_strtable;                         // 0x5e2a18

struct mesh;                                        // resource object (clone = 0x41c6d0)
struct resmgr {
    mesh* get_object(tstring* name);                // 0x419540
    mesh* clone(mesh* r);                           // 0x41c6d0
};
extern resmgr g_resmgr;                             // 0x5d6fa0
struct ini_file {
    int find(const tstring& key);                   // 0x4bd5a0
    float get_float(const tstring& key);            // 0x4bd5e0
};
extern ini_file g_config;                           // 0xa44240

// ---- engine globals / callees ------------------------------------------------------------------------
void msg_log_reset(void* self);                     // 0x42d1b0
void widget_add_mesh(void* o);                      // 0x4d6dd0 (mesh + label for the element)
void set_window(void** w);                          // 0x49b120 (gui: push window)
extern unsigned char g_gui_tick;                    // 0x5d8788
extern char g_game;                                 // 0x5e2930 (game object; tick = 0x48a000)
void game_tick(char* g);                            // 0x48a000
void fade_scale(void* o, vector4 v);                // 0x47af30

extern float g_small_dy;                            // 0x597850 (+0.0176)
extern float g_slider_k;                            // 0x54b3c8
extern float g_slider_k2;                           // 0x54b868
extern float g_slider_k3;                           // 0x54b8b4
extern float g_slider_k4;                           // 0x549cdc (1.0)
extern char g_opt_res[];                            // 0x5aa010 (meshes, thiscall arg of v34)
#define RES(o)      (*(void**)(g_opt_res + (o)))
extern char g_opt_col[];                            // 0x5aa01c.. (set_mesh arguments)
#define COL(o)      (*(void**)(g_opt_col + (o)))

// ---- widget element ----------------------------------------------------------------------------------
struct widget {
    char pad0[0x148];
    unsigned flags;                     // +0x148
    int size, size2;                    // +0x14c, +0x150
    char pad154[0x184 - 0x154];
    unsigned w184;                      // +0x184 (label background flag)
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28();
    virtual void v2c(void* m);          // +0x2c  set_mesh
    virtual void v30(); virtual void v34(void* m);      // +0x34
    virtual void v38(void* p); virtual void v3c(); virtual void v40(); virtual void v44();
    virtual void v48(); virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void v58(void* p);          // +0x58
    virtual void v5c(); virtual void v60();
    virtual void v64(void* p);          // +0x64
    virtual void v68();
    virtual void v6c(void* p);          // +0x6c
    virtual void v70(void* p);          // +0x70
    virtual void v74(void* p);          // +0x74
    virtual void v78();
    virtual void v7c(void* p);          // +0x7c  set_mesh(res)
    virtual void v80(int n, void* p);   // +0x80
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90();
    virtual void v94(void* p);
    virtual void v98(void* p);          // +0x98
    virtual void v9c(); virtual void va0();
    virtual void va4(const tstring* s); // +0xa4
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4();
    virtual void vb8(); virtual void vbc(); virtual void vc0(); virtual void vc4();
    virtual void vc8(widget* o);        // +0xc8  add_child
};
// the 0x592848 container: adds a byte at +0x188 (cleared by its ctor)
struct panel : widget {
    char f188;
};
void element_ctor(void* o);                                                              // 0x48ae20
void panel_ctor_4931a0(void* o);                                                         // 0x4931a0
void panel_ctor_4931d0(void* o);                                                         // 0x4931d0

// allocators (this = the owning menu; they use it for the new element's owner)
widget* new_label(void* self, tstring* s, float a, float b, float c, float d, float e);  // 0x4dada0
widget* new_button(void* self, tstring* s, float a, float b, float c, float d);          // 0x4dce70
widget* new_slider(tstring* s, void* self, float a, float b, float c, float d, float e); // 0x4dfd50
widget* new_slider2(tstring* s, void* self, float a, float b, float c, float d);         // 0x4dfec0

// the menu object (`this`)
struct OptionsScreen {
    char pad0[0x2c];
    panel* root;                        // +0x2c  options_window container
    widget* header;                     // +0x30  main_options container
    widget* s34;                        // +0x34
    widget* b38;                        // +0x38  ui_controls
    widget* b3c;                        // +0x3c  ui_video_options
    widget* b40;                        // +0x40  ui_done
    widget* box44;                      // +0x44
    widget* box48;                      // +0x48
    widget* box4c;                      // +0x4c
    widget* box50;                      // +0x50
    widget* box54;                      // +0x54
    widget* box58;                      // +0x58
    widget* l5c;                        // +0x5c
    widget* l60;                        // +0x60
    widget* l64;                        // +0x64
    widget* l68;                        // +0x68
    widget* l6c;                        // +0x6c
    widget* l70;                        // +0x70
    widget* l74;                        // +0x74
    widget* d78;                        // +0x78  framerate slider
    widget* s7c;                        // +0x7c
    widget* s80;                        // +0x80
    widget* s84;                        // +0x84
    widget* t88;                        // +0x88
    widget* s8c;                        // +0x8c
    widget* s90;                        // +0x90
    widget* s94;                        // +0x94
    widget* s98;                        // +0x98
    widget* s9c;                        // +0x9c
    widget* sa0;                        // +0xa0
    widget* sa4;                        // +0xa4
    widget* sa8;                        // +0xa8
    widget* sac;                        // +0xac
    widget* sb0;                        // +0xb0
    widget* sb4;                        // +0xb4
    widget* sb8;                        // +0xb8
    widget* sbc;                        // +0xbc
    widget* sc0;                        // +0xc0
    widget* sc4;                        // +0xc4
    widget* sc8;                        // +0xc8
    widget* scc;                        // +0xcc
    widget* sd0;                        // +0xd0
    widget* sd4;                        // +0xd4
    void create();                      // 0x4d6300
    void open();
};

// @ 0x4de6f0
void OptionsScreen::open()
{
    msg_log_reset((void*)0x5d7080);
    root = new panel;                               // 0x190, ctor 0x4931a0, vptr 0x592848
    root->f188 = 0;
    {
        float p[4];
        p[0] = -0.001f; p[1] = -0.001f;
        root->v58(p);
    }
    {
        float s[4];
        s[0] = 1.001f; s[1] = 0.001f;
        root->v64(s);
    }
    {
        tstring k("");
        root->va4(&k);
    }
    {
        tstring k("options_window");
        mesh* m = g_resmgr.clone(g_resmgr.get_object(&k));
        root->v7c(m);
    }
    header = new widget;                            // 0x178, ctor 0x48ae20
    element_ctor(header);
    {
        float p[4];
        p[0] = 0.4f; p[1] = 0.67f;
        header->v58(p);
    }
    {
        float s[4];
        s[0] = 0.2f; s[1] = 0.05f;
        header->v64(s);
    }
    {
        tstring k("main_options");
        mesh* m = g_resmgr.clone(g_resmgr.get_object(&k));
        header->v7c(m);
    }
    root->vc8(header);
    {
        tstring k("ui_controls");
        root->vc8(new_label(this, g_strtable.lookup(&k), 0.1f, 0.05f, 0.8f, 0.01f, 0.035f));
    }
    {
        tstring k("ui_video_options");
        root->vc8(new_label(this, g_strtable.lookup(&k), 0.1f, 0.05f, 0.8f, 0.035f, 0.01f));
    }
    {
        tstring k("ui_done");
        root->vc8(new_label(this, g_strtable.lookup(&k), 0.65f, 0.1f, 0.8f, 0.01f, 0.035f));
    }
    {
        tstring k("ui_audio_options");
        root->vc8(new_label(this, g_strtable.lookup(&k), 0.1f, 0.67f, 0.8f, 0.035f, 0.01f));
    }
    {
        tstring k("");
        s7c = new_slider(&k, root, 0.23f, 0.604f, 1.0f, 0.0f, 0.0f);
        s7c->v34(RES(0x6c));
    }
    {
        tstring k("");
        s80 = new_slider(&k, root, 0.23f, 0.557f, 1.0f, 0.0f, 0.0f);
        s80->v34(RES(0x70));
    }
    {
        tstring k("");
        s84 = new_slider(&k, root, 0.23f, 0.456f, 1.0f, 0.0f, 0.0f);
        s84->v34(RES(0x74));
    }
    {
        tstring k("ui_enabled");
        t88 = new_button(this, g_strtable.lookup(&k), 0.4f, 0.15f, 0.7f, 0.02f);
        t88->w184 = 1;
        t88->v2c(COL(0x1c));
        widget_add_mesh(t88);
    }
    {
        tstring k("");
        d78 = new_slider(&k, root, 0.14f, 0.42f, 1.0f, 0.0f, 0.0f);
        d78->v34(RES(0x3c));
    }
    {
        tstring k("ui_enabled");
        l74 = new_button(this, g_strtable.lookup(&k), 0.4f, 0.27f, 0.7f, 0.03f);
        l74->w184 = 1;
        l74->v2c(COL(0x38));
        widget_add_mesh(l74);
    }
    {
        tstring k("ui_enabled");
        l70 = new_button(this, g_strtable.lookup(&k), 0.4f, 0.315f, 0.7f, 0.03f);
        l70->w184 = 1;
        l70->v2c(COL(0x34));
        widget_add_mesh(l70);
    }
    {
        tstring k("ui_controls");
        l6c = new_button(this, g_strtable.lookup(&k), 0.4f, 0.62f, 0.7f, 0.03f);
        l6c->w184 = 1;
        l6c->v2c(COL(0x30));
        widget_add_mesh(l6c);
    }
    {
        tstring k("ui_video_options");
        l68 = new_button(this, g_strtable.lookup(&k), 0.4f, 0.57f, 0.7f, 0.03f);
        l68->w184 = 1;
        l68->v2c(COL(0x2c));
        widget_add_mesh(l68);
    }
    {
        tstring k("ui_audio_options");
        l64 = new_button(this, g_strtable.lookup(&k), 0.4f, 0.53f, 0.7f, 0.03f);
        l64->w184 = 1;
        l64->v2c(COL(0x28));
        widget_add_mesh(l64);
    }
    {
        tstring k("ui_enabled");
        l60 = new_button(this, g_strtable.lookup(&k), 0.4f, 0.48f, 0.7f, 0.03f);
        l60->w184 = 1;
        l60->v2c(COL(0x24));
        widget_add_mesh(l60);
    }
    {
        tstring k("ui_enabled");
        l5c = new_button(this, g_strtable.lookup(&k), 0.4f, 0.35f, 0.7f, 0.03f);
        l5c->w184 = 1;
        l5c->v2c(COL(0x20));
        widget_add_mesh(l5c);
    }
    create();
    {
        void* w = root;
        g_gui_tick = 1;
        set_window(&w);
    }
    if (g_game)
        game_tick(&g_game);
}
