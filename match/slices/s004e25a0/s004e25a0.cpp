// s004e25a0: the options (options_window) menu. Fills the menu with the main buttons, the equipment-slot
// rows, the key-binding rows and the screenshot buttons.
// Provisional names; every offset comes from the disassembly of 0x4e25a0..0x4e2ce0.
#include <windows.h>
#include "rgl.h"

#define MB_NOTHROW throw()

extern "C" int _fltused = 0;    // MSVC x87 helper (this TU uses floats)

struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring() MB_NOTHROW;                               // 0x42bbd0
    tstring(const char* s, int n) MB_NOTHROW;           // 0x42bc60 (provisional)
    tstring(const char* s) MB_NOTHROW;                  // 0x42bbe0
    tstring(const tstring& o) MB_NOTHROW;               // 0x42c110
    ~tstring() MB_NOTHROW;                              // 0x42bc40
    void operator=(const tstring& o) MB_NOTHROW;        // 0x42bc50
};

// rgl resource base (ctor 0x48ae20): flags at +0x148, cache size at +0x14c/+0x150 (from the ctor's stores).
struct resource {
    char pad0[0x148];
    unsigned flags;             // +0x148
    int size;                   // +0x14c
    int size2;                  // +0x150
    resource() MB_NOTHROW;                              // 0x48ae20
};

// The button/panel element (allocated by 0x4dada0/0x4dcc50 as 0x178/0x188 bytes; layout from 0x48ae20).
// Only the virtual slots this slice calls and the fields it touches are named.
struct widget : resource {
    unsigned w184;              // +0x184 (bit 0 set by 0x4e25a0)
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
    virtual void v34();                                 // +0x34
    virtual void v38();                                 // +0x38
    virtual void v3c();                                 // +0x3c
    virtual void v40();                                 // +0x40
    virtual void v44();                                 // +0x44
    virtual void v48();                                 // +0x48
    virtual void v4c();                                 // +0x4c
    virtual void v50();                                 // +0x50
    virtual void v54();                                 // +0x54
    virtual void v58(const tstring* s);                 // +0x58
    virtual void v5c();                                 // +0x5c
    virtual void v60();                                 // +0x60
    virtual void v64(const float* ab);                  // +0x64
    virtual void v68();                                 // +0x68
    virtual void v6c(const float* ab);                  // +0x6c
    virtual void v70(const float* abc);                 // +0x70
    virtual void v74(const int* v);                     // +0x74
    virtual void v78();                                 // +0x78
    virtual void v7c(void* p);                          // +0x7c
    virtual void v80(int n, void* p);                   // +0x80
    virtual void v84();                                 // +0x84
    virtual void v88();                                 // +0x88
    virtual void v8c();                                 // +0x8c
    virtual void v90();                                 // +0x90
    virtual void v94();                                 // +0x94
    virtual void v98(const int* v);                     // +0x98
    virtual void v9c();                                 // +0x9c
    virtual void va0();                                 // +0xa0
    virtual void va4(tstring* out, tstring* s);         // +0xa4 (ret 8)
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

// the 0x4dcc50 element: has a +0x94 field holding a sub-widget.
struct list_widget : widget {
    widget* sub;                // +0x178 (0x4dcc50 initialises it to 0) -- provisional
};

// the 0x592848 element created by OptionsMenu::init.
struct tex_quad : widget {
    unsigned char f188;         // +0x188 (cleared by the ctor)
    tex_quad() MB_NOTHROW;                              // 0x4931a0
};

// The options menu object (global 0xa73730). vtable slots named by offset; only the ones called here matter.
struct OptionsMenu {
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
    virtual void v34();                                 // +0x34
    virtual void v38();                                 // +0x38
    virtual void v3c();                                 // +0x3c
    virtual void v40();                                 // +0x40
    virtual void v44();                                 // +0x44
    virtual void v48();                                 // +0x48
    virtual void v4c();                                 // +0x4c
    virtual void v50();                                 // +0x50
    virtual void v54();                                 // +0x54
    virtual void v58(const tstring* s);                 // +0x58
    virtual void v5c();                                 // +0x5c
    virtual void v60();                                 // +0x60
    virtual void v64(const float* ab);                  // +0x64
    virtual void v68();                                 // +0x68
    virtual void v6c(const float* ab);                  // +0x6c
    virtual void v70(const float* abc);                 // +0x70
    virtual void v74(const int* v);                     // +0x74
    virtual void v78();                                 // +0x78
    virtual void v7c(void* p);                          // +0x7c
    virtual void v80(int n, void* p);                   // +0x80
    virtual void v84();                                 // +0x84
    virtual void v88();                                 // +0x88
    virtual void v8c();                                 // +0x8c
    virtual void v90();                                 // +0x90
    virtual void v94();                                 // +0x94
    virtual void v98(const int* v);                     // +0x98
    virtual void v9c();                                 // +0x9c
    virtual void va0();                                 // +0xa0
    virtual void va4(tstring* out, tstring* s);         // +0xa4 (ret 8)
    virtual void va8();                                 // +0xa8
    virtual void vac();                                 // +0xac
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    virtual void vb8();                                 // +0xb8
    virtual void vbc();                                 // +0xbc
    virtual void vc0();                                 // +0xc0
    virtual void vc4();                                 // +0xc4
    virtual void vc8(OptionsMenu* o);                   // +0xc8 (not called; keeps the count right)
    int f04;                                            // +0x04
    int f08;                                            // +0x08
    int f0c;                                            // +0x0c
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    int f18;                                            // +0x18
    int f1c;                                            // +0x1c
    int f20;                                            // +0x20
    int f24;                                            // +0x24
    int f28;                                            // +0x28
    tex_quad* quad;                                     // +0x2c
    int f30;                                            // +0x30
    int f34;                                            // +0x34
    int f38;                                            // +0x38
    int slots[0x25];                                    // +0x3c .. +0xd4
    int lst_count;                                      // +0xd4? (only its size matters here)
    int f164;                                           // +0x164
    int f168;                                           // +0x168
    int f16c;                                           // +0x16c
    int keys[11];                                       // +0x170 .. +0x19c (7 names + 7 key values overlap)
    int f1a8;                                           // +0x1a8
    int f1ac;                                           // +0x1ac
    int f1b0;                                           // +0x1b0
    char f1b4;                                          // +0x1b4
    char pad1b5[3];
    int f1b8;                                           // +0x1b8
    void init();                                        // 0x4e25a0
    void copy_slots();                                  // 0x4e2cb0
    void read_keys();                                   // 0x4d7120 (thiscall)
    void read_key_name(tstring* out, int i);            // 0x4d7370 (thiscall, ret 8)
};

// ---- externals -----------------------------------------------------------------------------------------
struct MsgLog;                                          // same object, see g_gui
struct GuiManager { void set_window(void** w); };       // 0x49b120
extern unsigned char g_msg_tick;                        // 0x5d8788 (bit 0 set at the end)
struct mesh {                                           // 0x260 bytes (see s00419420)
    mesh* clone();                                      // 0x41c6d0
    void scale(float f);                                // 0x44f0c0
};
struct resmgr {
    mesh* get_object(const tstring* name);              // 0x419540
};
extern resmgr g_resmgr;                                 // 0x5d6fa0
struct StrTable {   // 0x5e2a18
    tstring* lookup(tstring* out, const char* s, int n, float r1, float r2, float r3) MB_NOTHROW;   // 0x43a1e0
};
extern StrTable g_strtable;                             // 0x5e2a18
extern OptionsMenu g_menu;                              // 0xa73730
extern GuiManager g_gui;                                // 0x5d7080
void set_window(void** w);                              // 0x49b120 (GuiManager)

void msg_log_reset(MsgLog* self);                       // 0x42d1b0

widget* new_button(OptionsMenu* o, tstring* txt, float a, float b, float c, float d, float e);  // 0x4dada0
widget* new_button2(OptionsMenu* o, tstring* txt, float a, float b, float c, float d);  // 0x4dce70
widget* new_list(OptionsMenu* o, tstring* txt, float a, float b, float c, float d,
                 float e, float f, float g);        // 0x4dcc50
tstring* slot_text(tstring* out, int i);                // 0x4e2ea0
tstring* key_text(tstring* out, int i);                 // 0x4e2ce0

#define LOOKUP(s) g_strtable.lookup(&t, s, sizeof(s) - 1, 0.82f, 0.6f, 0.15f)

// ---- code ----------------------------------------------------------------------------------------------
// @ 0x4e25a0
void OptionsMenu::init()
{
    f1b8 = -1;
    msg_log_reset((MsgLog*)0x5d7080);
    quad = new tex_quad;
    float f2[2];
    f2[0] = -0.001f;
    f2[1] = -0.001f;
    quad->v58((tstring*)f2);
    f2[0] = 1.001f;
    f2[1] = 0.75f;
    quad->v64(f2);
    int v = -1;
    quad->v98(&v);
    {
        tstring name("options_window");
        mesh* m = g_resmgr.get_object(&name);
        mesh* c = m->clone();
        quad->v7c(c);
    }
    {
        tstring t;
        f38 = (int)new_button2(this, LOOKUP("ui_set_default_keys"), 0.82f, 0.6f, 0.15f, 0.1f);
    }
    {
        tstring t;
        f34 = (int)new_button2(this, LOOKUP("ui_undo_changes"), 0.82f, 0.1f, 0.15f, 0.1f);
    }
    {
        tstring t;
        f30 = (int)new_button2(this, LOOKUP("ui_done"), 0.82f, 0.05f, 0.15f, 0.1f);
    }
    {
        tstring t;
        f164 = (int)new_button(this, LOOKUP("ui_press_a_key"), 0.75f, 0.5f, 0.2f, 0.06f, 0.5f);
    }
    ((widget*)(void*)f164)->vb0();
    {
        tstring t;
        f168 = (int)new_button(this, LOOKUP("ui_battle_controls"), 0.72f, 0.3f, 0.15f, 0.1f, 0.5f);
    }
    int iv = 0x10;
    ((widget*)(void*)f168)->v74(&iv);
    iv = -1;
    ((widget*)(void*)f168)->v98(&iv);
    {
        tstring t;
        f16c = (int)new_button(this, LOOKUP("ui_map_controls"), 0.70f, 0.3f, 0.15f, 0.1f, 0.5f);
    }
    iv = 0x10;
    ((widget*)(void*)f16c)->v74(&iv);
    iv = -1;
    ((widget*)(void*)f16c)->v98(&iv);
    {
        tstring t;
        f1a8 = (int)new_button(this, LOOKUP("ui_general_controls"), 0.70f, 0.355f, 0.15f, 0.1f, 0.5f);
    }
    iv = 0x10;
    ((widget*)(void*)f1a8)->v74(&iv);
    iv = -1;
    ((widget*)(void*)f1a8)->v98(&iv);
    for (int i = 0; i < 0x25; i++) {
        float y = 0.7f;
        float c = 0.677f - (float)(i % 0x13) * 0.037f;
        if (i >= 0x13)
            y = 0.35f;
        tstring s;
        slot_text(&s, i);
        widget* tb = new_list(this, &s, y, c, 0.06f, 0.05f, 3.7f, 0.075f, 0.085f);
        slots[i] = (int)tb;
        iv = 8;
        tb->v74(&iv);
        iv = 0xfff0f0f0;
        tb->v98(&iv);
        tstring txt("          ");
        float cb = c + 0.195f;
        float yy = y + 0.005f;
        tstring up("medium_button");
        tstring down("medium_button_down");
        widget* b = new_list(this, &txt, yy, cb, 0.06f, 0.05f, 3.7f, 0.075f, 0.085f);
        ((list_widget*)(void*)tb)->sub = b;
        b->w184 |= 1;
    }
    for (int i = 0; i < 7; i++) {
        float c = 0.677f - (float)i * 0.037f;
        tstring s;
        key_text(&s, i);
        widget* tb = new_button(this, &s, 0.72f, c, 0.13f, 0.055f, 0.037f);
        keys[i] = (int)tb;
        iv = 8;
        tb->v74(&iv);
        iv = 0xfff0f0f0;
        tb->v98(&iv);
        tstring t;
        g_menu.read_key_name(&t, i);
        widget* b = new_button(this, &t, 0.77f, c, 0.13f, 0.055f, 0.037f);
        keys[i + 7] = (int)b;
        iv = 0x10;
        b->v74(&iv);
        iv = 0xfff0f0f0;
        b->v98(&iv);
    }
    {
        tstring t;
        f1ac = (int)new_button(this, LOOKUP("ui_take_screenshot"), 0.72f, 0.205f, 0.13f, 0.055f, 0.037f);
    }
    iv = 8;
    ((widget*)(void*)f1ac)->v74(&iv);
    iv = 0xfff0f0f0;
    ((widget*)(void*)f1ac)->v98(&iv);
    {
        tstring s("Ctrl + Insert");
        f1b0 = (int)new_button(this, &s, 0.77f, 0.205f, 0.13f, 0.055f, 0.037f);
    }
    iv = 0x10;
    ((widget*)(void*)f1b0)->v74(&iv);
    iv = 0xfff0f0f0;
    ((widget*)(void*)f1b0)->v98(&iv);
    void* q = (void*)quad;
    g_msg_tick = 1;
    set_window(&q);
    copy_slots();
    f1b4 = 0;
    read_keys();
}

// @ 0x4e2cb0
void OptionsMenu::copy_slots()
{
    // 37 pairs of dwords at this+0 (the vptr slot included, as in the original) are copied to this+0x128
    int* p = (int*)this + 1;
    int n = 0x25;
    do {
        p[0x49] = p[-1];
        p[0x4a] = p[0];
        p += 2;
    } while (--n != 0);
}

// @ 0x4e2ce0  tstring* key_text(tstring* out, int i)  (ret 8)
tstring* key_text(tstring* out, int i)
{
    tstring s("unknown");
    switch (i) {
    case 0: {
        tstring t("ui_move_forward", 15);
        s = *g_strtable.lookup(&t, "ui_move_forward", 15, 0.82f, 0.6f, 0.15f);
        break;
    }
    case 1: {
        tstring t("ui_move_backward", 16);
        s = *g_strtable.lookup(&t, "ui_move_backward", 16, 0.82f, 0.6f, 0.15f);
        break;
    }
    case 2: {
        tstring t("ui_move_left", 12);
        s = *g_strtable.lookup(&t, "ui_move_left", 12, 0.82f, 0.6f, 0.15f);
        break;
    }
    case 3: {
        tstring t("ui_move_right", 13);
        s = *g_strtable.lookup(&t, "ui_move_right", 13, 0.82f, 0.6f, 0.15f);
        break;
    }
    case 4: {
        tstring t("ui_zoom_in", 10);
        s = *g_strtable.lookup(&t, "ui_zoom_in", 10, 0.82f, 0.6f, 0.15f);
        break;
    }
    case 5: {
        tstring t("ui_zoom_out", 11);
        s = *g_strtable.lookup(&t, "ui_zoom_out", 11, 0.82f, 0.6f, 0.15f);
        break;
    }
    case 6: {
        tstring t("ui_wait", 7);
        s = *g_strtable.lookup(&t, "ui_wait", 7, 0.82f, 0.6f, 0.15f);
        break;
    }
    }
    *out = s;
    return out;
}
