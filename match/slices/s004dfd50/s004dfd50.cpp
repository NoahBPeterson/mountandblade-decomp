// Slice s004dfd50: the mission-UI horizontal-slider factory (0x4dfd50) and the drop-down button /
// combo-box factory (0x4dfec0).  Both allocate one of the text/slider widgets of s0048add0/s00492b30
// (UiB: vptr 0x591fb0, ctor 0x48b4b0, size 0x188; PanelB: vptr 0x592570, ctor 0x4931d0, size 0x1d0),
// position it, attach cloned meshes, recolour them and register the object with the parent widget.
// Widget slot offsets and the tstring / resource-manager / config-table layouts follow the verified
// neighbouring slices (s0048add0, s004885e0, s00492b30, s004de6f0).  Names are provisional.
#include <windows.h>
#include <math.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

// temporary 2-D value; the original builds these on the stack and passes their address to the
// widget methods (whose parameter is a float*).  A user constructor makes MSVC evaluate the two
// arguments right-to-left, which is what the original code does.
struct vector2 {
    float x, y;
    vector2(float a, float b) { x = a; y = b; }
};

// ---- engine types ------------------------------------------------------------------------------------

// 0x10-byte tstring: { pos, buffer, capacity, length } (ctor 0x42bbe0 zeroes all four, then copies).
struct tstring {
    int pos;
    char* buf;
    int cap;
    int len;
    tstring(const char* s);                                 // 0x42bbe0 (ret 4)
    ~tstring();                                             // 0x42bc40
};

// A cloned rgl mesh; only the two members used here.
struct mesh {
    mesh* clone();                                          // 0x41c6d0
    void set_color(vector4 color);                          // 0x47af30 (vector4 by value, ret 0x10)
};

// resource manager object at 0x5d6fa0
struct resmgr {
    mesh* get_object(const tstring& name);                  // 0x419540 (ret 4)
};
extern resmgr g_alloc;                                      // 0x5d6fa0

// settings table at 0xa44240: find() returns an entry index or -1; entries are 0x2c bytes starting at
// 0xa4425c with the string pointer at +4 (null means the pointer field itself is the string).
struct config_table {
    int find(const tstring& key);                           // 0x4bd5a0 (ret 4)
};
extern config_table g_config;                               // 0xa44240
extern "C" double atof(const char* s);                      // 0x537352

struct config_entry {
    int f0;                                                 // +0
    char* value;                                            // +4
    char pad[0x2c - 8];
};
#define CONFIG_ENTRIES ((config_entry*)0xa4425c)            // 0xa4425c

static const char* config_string(int i)
{
    config_entry* e = &CONFIG_ENTRIES[i];
    char* v = e->value;
    return v ? v : (const char*)&e->value;
}

// load and clone a named mesh (the original's tstring temporary lives inside the inlined helper)
static __forceinline mesh* load_mesh(const char* name)
{
    tstring n(name);
    return g_alloc.get_object(n)->clone();
}

// read one float setting, `def` when the key is missing (the original always passes 0.0f)
static __forceinline float config_float(const char* name, float def)
{
    tstring n(name);
    int i = g_config.find(n);
    float r = def;
    if (i >= 0)
        r = (float)atof(config_string(i));
    return r;
}

// ---- the widget interface (slots from the vtable at 0x591fb0 / 0x592570) ------------------------------
struct UiWidget {
    virtual void d00(); virtual void d01(); virtual void d02(); virtual void d03(); virtual void d04();
    virtual void d05(); virtual void d06(); virtual void d07(); virtual void d08(); virtual void d09();
    virtual void d10(); virtual void d11(); virtual void d12(); virtual void d13(); virtual void d14();
    virtual void d15(); virtual void d16(); virtual void d17(); virtual void d18(); virtual void d19();
    virtual void d20(); virtual void d21();
    virtual void move_a(vector2* p);                        // +0x58
    virtual void d23(); virtual void d24();
    virtual void move_f0(vector2* p);                       // +0x64
    virtual void d26();
    virtual void set_size(vector2* p);                      // +0x6c
    virtual void set_pos(vector2* p);                       // +0x70
    virtual void d29(); virtual void d30(); virtual void d31();
    virtual void set_track(int i, void* p);                 // +0x80
    virtual void d33(); virtual void d34(); virtual void d35(); virtual void d36(); virtual void d37();
    virtual void d38(); virtual void d39(); virtual void d40();
    virtual void set_text(void* out, const tstring* name);  // +0xa4
    virtual void d42(); virtual void d43(); virtual void d44(); virtual void d45(); virtual void d46();
    virtual void d47(); virtual void d48(); virtual void d49();
    virtual void add_child(void* child);                    // +0xc8
};

// slider widget (vptr 0x591fb0); only the ctor is needed here
struct UiB : UiWidget {
    char pad[0x188 - 4];
    UiB();                                                  // 0x48b4b0 (returns this in eax)
};

// drop-down / combo-box widget (vptr 0x592570)
struct PanelB : UiWidget {
    char pad0[0x108 - 4];
    void* track[5];                                         // +0x108 (set_track slots)
    char pad11c[0x1a0 - 0x11c];
    void* child_clicked;                                    // +0x1a0
    void* child;                                            // +0x1a4
    void* child_hl;                                         // +0x1a8
    char pad1ac[4];
    float f1b0, f1b4, f1b8;                                 // +0x1b0
    float f1bc, f1c0;                                       // +0x1bc (child button text size)
    float f1c4, f1c8;                                       // +0x1c4 (child button text position)
    char pad1cc[0x1d0 - 0x1cc];
    PanelB();                                               // 0x4931d0 (returns this in eax)
};

// ---- 0x4dfd50: horizontal slider ----------------------------------------------------------------
// @ 0x4dfd50
UiB* make_slider(void* parent, float x, float y, float k)
{
    UiB* w = new UiB;
    {
        vector2 pos(x, y);
        w->move_a(&pos);
    }
    {
        vector2 size(y * 0.25f, k * 0.03f);
        w->move_f0(&size);
    }
    mesh* track = load_mesh("slider_hor");
    mesh* handle = load_mesh("handle_hor");
    if (k != 1.0f) {
        track->set_color(vector4(k, k, k));
        handle->set_color(vector4(k, k, k));
    }
    w->set_track(0, track);
    w->set_track(2, handle);
    ((UiWidget*)parent)->add_child(w);
    return w;
}

// ---- 0x4dfec0: drop-down button / combo box -----------------------------------------------------
// @ 0x4dfec0
PanelB* make_button(void* parent, const tstring* name, float x, float y)
{
    PanelB* b = new PanelB;
    {
        vector2 pos(x + 0.0176f, y);
        b->move_a(&pos);
    }

    {
        tstring n("button_drop");
        b->set_track(2, g_alloc.get_object(n)->clone());
    }
    ((mesh*)b->track[2])->set_color(vector4(1.0f, 0.5f, 1.0f));

    {
        tstring n("button_drop_clicked");
        b->set_track(1, g_alloc.get_object(n)->clone());
    }
    ((mesh*)b->track[1])->set_color(vector4(1.0f, 0.5f, 1.0f));

    {
        tstring n("button_drop_hl");
        b->set_track(3, g_alloc.get_object(n)->clone());
    }
    ((mesh*)b->track[3])->set_color(vector4(1.0f, 0.5f, 1.0f));

    {
        tstring n("button_drop_child");
        b->child = g_alloc.get_object(n);
    }
    {
        tstring n("button_drop_child_clicked");
        b->child_clicked = g_alloc.get_object(n);
    }
    {
        tstring n("button_drop_child_hl");
        b->child_hl = g_alloc.get_object(n);
    }

    {
        vector2 size(config_float("combobox_button_size_x", 0.0f),
                     config_float("combobox_button_size_y", 0.0f));
        b->move_f0(&size);
    }

    b->f1b0 = config_float("combobox_child_button_size_x", 0.0f);
    b->f1b4 = config_float("combobox_child_button_size_y", 0.0f);
    b->f1b8 = config_float("combobox_child_button_position_x", 0.0f);

    {
        vector2 ts(config_float("combobox_child_button_text_size_x", 0.0f),
                   config_float("combobox_child_button_text_size_y", 0.0f));
        b->f1bc = ts.x;
        b->f1c0 = ts.y;
    }

    {
        vector2 tp(config_float("combobox_child_button_text_position_x", 0.0f),
                   config_float("combobox_child_button_text_position_y", 0.0f));
        b->f1c4 = tp.x;
        b->f1c8 = tp.y;
    }

    {
        float text_size = config_float("combobox_text_size", 0.0f);
        vector2 tsize(text_size, text_size);
        b->set_size(&tsize);
    }

    {
        vector2 tpos(config_float("combobox_text_position_x", 0.0f),
                     config_float("combobox_text_position_y", 0.0f));
        b->set_pos(&tpos);
    }

    {
        vector4 out;
        b->set_text(&out, name);
    }
    ((UiWidget*)parent)->add_child(b);
    return b;
}
