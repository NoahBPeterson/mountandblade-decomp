#include <windows.h>
#include <math.h>
#include <new>
#include "rgl.h"

using rgl::vector4;

extern "C" int _fltused = 0;

struct tstring {
    int pos; char* buf; int cap; int len;
    tstring();                                  // 0x42bbd0
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
};

struct mesh {
    char pad0[0x10c];
    void* pressed_color; void* normal_color;
    mesh* clone();
    void scale(float s);
};
struct resmgr { mesh* get_object(const tstring* name); };
extern resmgr g_resmgr;

void decode_utf8(void* out, const char* s);

struct widget {
    widget();
    widget(const widget& o);
    char vf0[4];
    char pad4[0x121 - 4];
    char f121;
    char pad122[0x178 - 0x122];
    widget* sub;
    float ratio;
    char f180;
    char pad181[3];
    unsigned f184;
    char pad188[0x1a4 - 0x188];
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c();
    virtual void v10(); virtual void v14(); virtual void v18(); virtual void v1c();
    virtual void v20(); virtual void v24(); virtual void v28(); virtual void v2c();
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c();
    virtual void v40(); virtual void v44(); virtual void v48(); virtual void v4c();
    virtual void v50(); virtual void v54();
    virtual void set_minmax(const float* v);
    virtual void v5c(); virtual void v60();
    virtual void set_size(const float* v);
    virtual void v68();
    virtual void set_scale(const float* v);
    virtual void set_pos(const float* v);
    virtual void v74(); virtual void v78();
    virtual void set_mesh(void* mesh, int layer);
    virtual void set_mesh2(int which, void* mesh);
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void v98(const int* v);
    virtual void set_color(const vector4* c);
    virtual void va0();
    virtual void set_text(tstring* out, const tstring* s);
    virtual void va8(); virtual void vac(); virtual void vb0(); virtual void vb4(); virtual void vb8();
    virtual void vbc(); virtual void vc0(); virtual void vc4();
    virtual void add_child(widget* child);
};
struct button : widget {
};

extern float g_549cdc, g_590b6c, g_54b3b0, g_549d70, g_549d68;

__declspec(noinline)
// @ 0x4dcc50
button* new_button(widget* parent, float x, float y, float z, float u,
                   const tstring& name, const tstring& text, float k1, float k2, float k3)
{
    char buf[0x2000];
    buf[0] = 0;
    button* p = new button;
    float box[2] = { x, y };
    p->set_minmax(box);
    float s[2] = { z, u };
    p->set_size(s);
    float scale = g_549cdc / u * z;
    {
        mesh* m = g_resmgr.get_object(&name);
        m = m->clone();
        m->scale(scale);
        p->set_mesh2(2, m);
    }
    {
        mesh* m = g_resmgr.get_object(&text);
        m = m->clone();
        m->scale(scale);
        p->set_mesh2(1, m);
    }
    {
        vector4 col;
        *(unsigned*)&col = 0xff007700;
        p->set_color(&col);
    }
    p->sub = 0;
    p->f121 = 0;
    p->f180 = 0;
    p->f184 = 0;
    {
        *(int*)(buf + 0x20) = 0x800;
        decode_utf8(buf + 0x20, buf);
        float v = z * g_590b6c;
        float ww = (float)*(int*)(buf + 0x14);
        float t = ww * v * g_54b3b0;
        float e = z * u * 0.6f - ww;
        float s2 = 0.0f;
        if (e > 0.0f)
            s2 = v / e * t;
        float sc[2] = { s2, u * g_549d70 - z * g_549d68 };
        p->set_scale(sc);
    }
    {
        float pb[2] = { 0.0f, u };
        p->set_pos(pb);
    }
    p->set_text((tstring*)&text, 0);
    parent->add_child(p);
    return p;
}

// @ 0x4dcbd0
button* new_long_button(widget* parent, float x, float y, float z, float u)
{
    tstring d("longer_button_down");
    tstring b("longer_button");
    return new_button(parent, x, y, z, u, b, d, 9.7f, 0.072f, 0.085f);
}

// @ 0x4dce70
button* new_medium_button(widget* parent, float x, float y, float z, float u)
{
    tstring d("medium_button_down");
    tstring b("medium_button");
    return new_button(parent, x, y, z, u, b, d, 3.7f, 0.072f, 0.085f);
}

// @ 0x4dc630
// Partial: result_window::init (1432 bytes).  Only the opening block is modelled here; the
// switch over the outcome (this+4) and the button rows are still disassembly notes (see
// partial.txt).  It is not byte-exact.
struct result_window {
    char pad0[4];
    int outcome;                  // +0x04
    char pad8[0x2c - 8];
    widget* win;                  // +0x2c
    widget* panel;                // +0x30
    char pad34[0x78 - 0x34];
    tstring ok_label;             // +0x78
};

void ctor_window(void*);                                // 0x4931a0
void ctor_panel(void*);                                 // 0x48ae20
void tstring_assign(tstring* dst, const tstring* src);  // 0x42bc50
tstring* str_lookup(const tstring* key);                // 0x43a1e0

void __fastcall result_window_init(result_window* self)
{
    if (self->outcome == 0) {
        tstring key("ui_ok");                           // 0x595eec
        tstring_assign(&self->ok_label, str_lookup(&key));
        key.~tstring();
    }

    widget* win = (widget*)operator new(0x190);         // 0x53545e
    if (win) ctor_window(win); else win = 0;            // 0x4931a0
    self->win = win;                                    // +0x2c

    float mm[2] = { 0.25f, 0.2f };                      // 0x3e800000 / 0x3e4ccccd
    win->set_minmax(mm);
    float sz[2] = { 0.5f, 0.44f };                      // 0x3f000000 / 0x3ee147ae
    win->set_size(sz);
    {
        tstring name("message_window");                 // 0x5971b0
        mesh* m = g_resmgr.get_object(&name);
        win->set_mesh(m->clone(), 0);
        name.~tstring();
    }

    widget* panel = (widget*)operator new(0x178);       // 0x53545e
    if (panel) ctor_panel(panel); else panel = 0;       // 0x48ae20
    self->panel = panel;                                // +0x30
    float pm[2] = { 0.05f, 0.34f };                     // 0x3d4ccccd / 0x3eae147b
    panel->set_minmax(pm);
    // ... switch (self->outcome) into the medium/long/yes-no button rows, then the
    //     window-list push (0x49b120) and byte 0x5d8788 = 1.
}
