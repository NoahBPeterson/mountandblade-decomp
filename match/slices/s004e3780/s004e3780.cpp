// s004e3780: the meeting window (character sheet) and the progress-bar widget it uses.
// Provisional names; every offset comes from the disassembly of 0x4e3780..0x4e40e0.
#include <windows.h>
#include "rgl.h"

extern "C" int _fltused = 0;    // MSVC x87 helper (this TU uses floats)

// ---- types ----------------------------------------------------------------------------------------------
struct tstring {
    int pos;            // +0  read cursor
    char* buf;          // +4
    int cap;            // +8
    int len;            // +0xc
    tstring();                                          // 0x42bbd0
    tstring(const char* s);                             // 0x42bbe0
    tstring(const tstring& o);                          // 0x42c110
    ~tstring();                                         // 0x42bc40
    void operator=(const tstring& o);                   // 0x42bc50
    void assign(const char* s);                         // 0x42bcb0
};

// rgl resource base (ctor 0x48ae20): flags at +0x148, cache size at +0x14c/+0x150.
struct resource {
    char pad0[0x148];
    unsigned flags;             // +0x148
    int size;                   // +0x14c
    int size2;                  // +0x150
    resource();                                         // 0x48ae20
};

struct mesh;                // 0x260 bytes (see s00419420)
mesh* mesh_clone(mesh* m);                          // 0x41c6d0
void mesh_scale(mesh* m, const rgl::vector4& v);    // 0x47af30

// The UI element (layout from 0x48ae20). Slots are named by vtable offset; only the ones used here matter.
struct widget : resource {
    unsigned char f154[0x168 - 0x154];
    float f168;                 // +0x168
    int f16c;                   // +0x16c
    int f170;                   // +0x170 (index of the scaled rect component)
    int f174;                   // +0x174
    widget* sub;                // +0x178
    float ratio;                // +0x17c
    mesh* handle;               // +0x180 (source mesh for the sub element)
    unsigned char f184;         // +0x184 (dirty flag)
    unsigned char f185;         // +0x185
    unsigned char f186;         // +0x186
    unsigned char f187;         // +0x187
    rgl::vector4 rect;          // +0x188
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
    virtual void v58(const float* minmax);              // +0x58
    virtual void v5c();                                 // +0x5c
    virtual void v60();                                 // +0x60
    virtual void v64(const float* size);                // +0x64
    virtual void v68();                                 // +0x68
    virtual void v6c(const float* d);                   // +0x6c
    virtual void v70(const float* abc);                 // +0x70
    virtual void v74(const int* v);                     // +0x74
    virtual void v78();                                 // +0x78
    virtual void v7c(void* p, void* q);                 // +0x7c
    virtual void v80(int n, void* p);                   // +0x80
    virtual void v84();                                 // +0x84
    virtual void v88();                                 // +0x88
    virtual void v8c();                                 // +0x8c
    virtual void v90();                                 // +0x90
    virtual void v94();                                 // +0x94
    virtual void v98(const int* v);                     // +0x98
    virtual void v9c();                                 // +0x9c
    virtual void va0();                                 // +0xa0
    virtual void va4(tstring* out, const tstring* s);   // +0xa4 (ret 8)
    virtual void va8();                                 // +0xa8
    virtual void vac(const tstring* s, float x, float y, float z);  // +0xac (ret 0x10)
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    virtual void vb8();                                 // +0xb8
    virtual void vbc();                                 // +0xbc
    virtual void vc0();                                 // +0xc0
    virtual void vc4();                                 // +0xc4
    virtual void vc8(widget* o);                        // +0xc8
};

// the 0x592848 element (also used by 0x4e25a0).
struct tex_quad : widget {
    unsigned char f188;         // +0x188 (cleared by the ctor)
    tex_quad();                                         // ctor of the 0x592848/0x597050 element
};

// the progress bar (vtable 0x597f20, allocated 0x198 bytes by 0x4e3d50).
struct progress_bar : widget {
    progress_bar();                                     // 0x4e3eb0
    void set_ratio(float r);                            // 0x4e3f60
    void update();                                      // 0x4e3fb0
};

// The meeting window object (layout from 0x4e3780 / 0x4e3aa0).
struct MeetingWindow {
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
    virtual void v58(const float* minmax);              // +0x58
    virtual void v5c();                                 // +0x5c
    virtual void v60();                                 // +0x60
    virtual void v64(const float* size);                // +0x64
    virtual void v68();                                 // +0x68
    virtual void v6c(const float* d);                   // +0x6c
    virtual void v70(const float* abc);                 // +0x70
    virtual void v74(const int* v);                     // +0x74
    virtual void v78();                                 // +0x78
    virtual void v7c(void* p, void* q);                 // +0x7c
    virtual void v80(int n, void* p);                   // +0x80
    virtual void v84();                                 // +0x84
    virtual void v88();                                 // +0x88
    virtual void v8c();                                 // +0x8c
    virtual void v90();                                 // +0x90
    virtual void v94();                                 // +0x94
    virtual void v98(const int* v);                     // +0x98
    virtual void v9c();                                 // +0x9c
    virtual void va0();                                 // +0xa0
    virtual void va4(tstring* out, const tstring* s);   // +0xa4 (ret 8)
    virtual void va8();                                 // +0xa8
    virtual void vac(const tstring* s, float x, float y, float z);  // +0xac (ret 0x10)
    virtual void vb0();                                 // +0xb0
    virtual void vb4();                                 // +0xb4
    virtual void vb8();                                 // +0xb8
    virtual void vbc();                                 // +0xbc
    virtual void vc0();                                 // +0xc0
    virtual void vc4();                                 // +0xc4
    virtual void vc8(widget* o);                        // +0xc8
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
    tex_quad* bg;                                       // +0x2c
    widget* button;                                     // +0x30
    widget* logo;                                       // +0x34
    progress_bar* bar;                                  // +0x38
    tex_quad* panel;                                    // +0x3c
    int f40;                                            // +0x40
    int f44;                                            // +0x44
    void init();                                        // 0x4e3780
    void init2();                                       // 0x4e3aa0
};

// the progress bar's key-value row store (element 0x50 bytes; ctor 0x4d1140).
struct pb_rows {
    int* begin;                                         // +0
    int* cur;                                           // +4
    int* end;                                           // +8
    void push(const tstring* v);                        // 0x4e4060 (ret 4)
};

// ---- globals / externals -------------------------------------------------------------------------------
struct MsgLog { void reset(); };                        // 0x42d1b0
struct resmgr { mesh* get_object(const tstring* name); };   // 0x419540
struct StrTable {                                       // 0x5e2a18
    tstring* lookup(tstring* out, const char* s, int n, float r1, float r2, float r3);   // 0x43a1e0
};
struct game { void m435790(int e, int a, int b); };      // 0x435790

extern MsgLog g_msg_log;                                // 0x5d7080
extern unsigned char g_gui_dirty;                       // 0x5d8788
extern tstring g_meeting_name;                          // 0xa70464
extern resmgr g_resmgr;                                 // 0x5d6fa0
extern StrTable g_strtable;                             // 0x5e2a18
extern game* g_game;                                    // 0x5e2930

void* __cdecl operator new(unsigned n);                 // 0x53545e
inline void* operator new(unsigned n, void* p) { return p; }
void gui_set_window(MsgLog* self, void** w);            // 0x49b120

widget* new_button2(widget* o, tstring* txt, float a, float b, float c, float d);          // 0x4dce70
widget* new_long_button(widget* o, tstring* txt, float a, float b, float c, float d, float e);  // 0x4dcbd0
widget* new_button(widget* o, tstring* txt, float a, float b, float c, float d, float e);  // 0x4dada0
progress_bar* new_progress_bar(widget* o, float x, float y, float a, float b);           // 0x4e3d50
void draw_label(widget* q, float x, float y, float z, const tstring* s);                  // 0x4e3f20
void widget_text(widget* w, float x, float y, float z, const tstring* s);                 // 0x488720
void rows_insert(int* where, pb_rows* self, const tstring* v);                            // 0x4d49c0
void rows_grow(pb_rows* self, int* at, const tstring* v);                                 // 0x4db960
int  timer_tick();                                                                       // 0x536a9f

#define LOOKUP(T, S) g_strtable.lookup(&(T), (S), sizeof(S) - 1, 0.82f, 0.6f, 0.15f)

// ---- code ----------------------------------------------------------------------------------------------
// @ 0x4e3780
extern const float g_progressbar_scale;                  // 0x3c23d70a = 0.01f (progress bar ctor)

void MeetingWindow::init()
{
    g_msg_log.reset();
    bg = new tex_quad;
    float f2[2];
    f2[0] = 0.0f;
    f2[1] = 0.0f;
    bg->v58(f2);
    f2[0] = 1.0f;
    f2[1] = 0.75f;
    bg->v64(f2);
    {
        tstring name("meeting_window");
        mesh* m = g_resmgr.get_object(&name);
        mesh* c = mesh_clone(m);
        bg->v7c(c, 0);
    }
    gui_set_window(&g_msg_log, (void**)&bg);
    {
        tstring t;
        button = new_button2(bg, LOOKUP(t, "ui_done", 0.7f, 0.09f, 0.15f), 0.7f, 0.09f, 0.15f, 0.1f);
    }
    logo = 0;
    bar = 0;
    f40 = -1;
    f44 = -1;
    {
        tstring t;
        logo = new_long_button(bg, LOOKUP(t, "ui_export_character", 0.6f, 1.0f, 0.0417f),
                               0.6f, 1.0f, 0.0417f, 0.1f, 0.5f);
    }
    {
        tstring t;
        bar = (progress_bar*)new_long_button(bg, LOOKUP(t, "ui_import_character", 0.5f, 0.5f, 0.0417f),
                                             0.5f, 0.5f, 0.0417f, 0.1f, 0.5f);
    }
    panel = new tex_quad;
    panel->f188 = 0;
    f2[0] = 0.05f;
    f2[1] = 0.0f;
    panel->v58(f2);
    f2[0] = 0.43f;
    f2[1] = 0.63f;
    panel->v64(f2);
    f2[0] = 0.02f;
    f2[1] = 0.02f;
    panel->v6c(f2);
    int iv = 0xff000000;
    panel->v98(&iv);
    {
        tstring s("");
        tstring l;
        l.pos = 0;
        panel->va4(&s, &l);
    }
    for (int i = 0; i < 10; i++) {
        g_meeting_name.assign("");
        g_game->m435790(0xe, i, 0);
        tstring line;
        line.pos = 0;
        float y = 0.9f - (float)i * 0.06f;
        panel->vac(&g_meeting_name, 0.0f, y, y);
    }
    bg->vc8(panel);
}

// @ 0x4e3aa0
void MeetingWindow::init2()
{
    g_msg_log.reset();
    f44 = 10;
    f40 = 0;
    bg = new tex_quad;
    float f2[2];
    f2[0] = -0.001f;
    f2[1] = -0.001f;
    bg->v58(f2);
    f2[0] = 1.001f;
    f2[1] = 0.751f;
    bg->v64(f2);
    {
        tstring name("load_window");
        mesh* c = mesh_clone(g_resmgr.get_object(&name));
        bg->v7c(c, 0);
    }
    int iv = 0xff442323;
    bg->v98(&iv);
    {
        tstring name;
        int t = timer_tick() % 15817;
        float f = (float)t * 0.01f;
        if (f < 10.0f)
            name.assign("ini_pic_mb_warrior_6");
        else if (f < 20.0f)
            name.assign("ini_pic_mb_warrior_7");
        else if (f < 30.0f)
            name.assign("ini_pic_mb_warrior_8");
        {
            mesh* c = mesh_clone(g_resmgr.get_object(&name));
            bg->v80(2, c);
        }
    }
    {
        widget* logo_ = (widget*)operator new(0x178);
        if (logo_ != 0)
            new (logo_) widget();
        logo = logo_;
        f2[0] = 0.05f;
        f2[1] = 0.6f;
        logo->v58(f2);
        f2[0] = 0.4f;
        f2[1] = 0.1f;
        logo->v64(f2);
        tstring name("warrider_logo");
        logo->v7c(mesh_clone(g_resmgr.get_object(&name)), 0);
    }
    bg->vc8(logo);
    bar = new_progress_bar(bg, 0.1f, 0.07f, 0.4f, 0.048f);
    {
        tstring t;
        button = new_button(bg, LOOKUP(t, ""), 0.11f, 0.6f, 0.05f, 0.1f, 0.5f);
    }
    gui_set_window(&g_msg_log, (void**)&bg);
    panel = 0;
    f40 = 0;
}

// @ 0x4e3d50   progress_bar* new_progress_bar(widget* o, float x, float y, float a, float b)
progress_bar* new_progress_bar(widget* o, float x, float y, float a, float b)
{
    progress_bar* p = (progress_bar*)operator new(0x198);
    if (p != 0)
        new (p) progress_bar;
    p->sub = 0;
    float pos[2];
    pos[0] = x;
    pos[1] = y;
    p->v58(pos);
    float size[2];
    size[0] = a * 1.5576323f;
    size[1] = b * 50.0f;
    p->v64(size);
    p->ratio = a * 0.31f;
    float sxy[2];
    sxy[0] = 1.0f;
    sxy[1] = b * 50.0f;
    {
        tstring name("progressbar");
        mesh* c = mesh_clone(g_resmgr.get_object(&name));
        rgl::vector4 v;
        v.x = pos[0];
        v.y = pos[1];
        v.z = sxy[0];
        v.w = sxy[1];
        mesh_scale(c, v);
    }
    {
        tstring name("progressbar_handle");
        p->handle = g_resmgr.get_object(&name);
    }
    p->rect.x = pos[0];
    p->rect.y = pos[1];
    p->rect.z = sxy[0];
    p->rect.w = sxy[1];
    p->v80(0, p->sub);
    o->vc8(p);
    return p;
}

// @ 0x4e3eb0
progress_bar::progress_bar()
{
    rect.x = 1.0f;
    rect.y = 1.0f;
    rect.z = 1.0f;
    sub = 0;
    handle = 0;
    rect.w = 0;
    f168 = g_progressbar_scale;
    f184 = 1;
}

// @ 0x4e3f20   draw_label(widget* q, float x, float y, float z, const tstring* s)  (ret 0x10)
void draw_label(widget* q, float x, float y, float z, const tstring* s)
{
    q->f186 = 1;
    q->f184 = 1;
    q->vc4();
    widget_text(q, x, y, z, s);
}

// @ 0x4e3f60
extern const float g_min_ratio;                          // 0x54a6f0 = 0.01f
extern const float g_max_ratio;                          // 0x549cdc = 1.0f

void progress_bar::set_ratio(float r)
{
    if (r < g_min_ratio) {
        f184 = 1;
        f168 = g_min_ratio;
        return;
    }
    if (r > g_max_ratio)
        r = g_max_ratio;
    f168 = r;
    f184 = 1;
}

// @ 0x4e3fb0
void progress_bar::update()
{
    if (sub != 0) {
        if (f184 == 0)
            return;
        if (sub != 0)
            sub->v00();
    }
    if (handle != 0) {
        mesh* c = mesh_clone(handle);
        float sx = f168;
        rgl::vector4 r = rect;
        float* d = &r.x + f170;
        *d = sx * *d;
        sub = (widget*)c;
        mesh_scale(c, r);
        f184 = 0;
    }
}

// @ 0x4e4060
void pb_rows::push(const tstring* v)
{
    int n = 0;
    if (begin != 0)
        n = (int)(((char*)end - (char*)begin) / 0x50);
    int cap = 0;
    if (begin != 0)
        cap = (int)(((char*)cur - (char*)begin) / 0x50);
    if (begin != 0 && cap < n) {
        rows_insert(end, this, v);
        end = (int*)((char*)end + 0x50);
        return;
    }
    rows_grow(this, end, v);
}
