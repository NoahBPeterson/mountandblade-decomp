// Slice s004de1b0: the debrief / report menu.  0x4de1b0 and 0x4de5b0 each build an "escape menu"-style
// object: a container widget plus a background image and one row / text widget.  0x4de320 makes the
// background mesh and 0x4de3e0 turns four points into the quad that the mesh is made of.
// Names provisional; every offset is read off the disassembly.
#include <windows.h>
#include <new>

#define MB_NOTHROW throw()

// ---- engine string ---------------------------------------------------------------------------------
struct tstring {
    int pos;            // +0x00  read cursor
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring(const char* s);                     // 0x42bbe0
    ~tstring();                                 // 0x42bc40
    void assign(const char* s);                 // 0x42bcb0
};

struct StrTable {
    tstring* lookup(const tstring& key) MB_NOTHROW;     // 0x43a1e0 (never null)
    const tstring* find(const char* p);                 // 0x4c6010
};
extern StrTable g_strtable;                             // 0x5e2a18

struct vector2 {
    float x, y;
    vector2() {}
    vector2(float x_, float y_) : x(x_), y(y_) {}
};

// ---- widgets ---------------------------------------------------------------------------------------
struct Widget {
    virtual void w00(); virtual void w01(); virtual void w02(); virtual void w03(); virtual void w04();
    virtual void w05(); virtual void w06(); virtual void w07(); virtual void w08(); virtual void w09();
    virtual void w10(); virtual void w11(); virtual void w12(); virtual void w13(); virtual void w14();
    virtual void w15(); virtual void w16(); virtual void w17(); virtual void w18(); virtual void w19();
    virtual void w20(); virtual void w21();
    virtual void set_pos(const vector2& p);     // +0x58
    virtual void w23(); virtual void w24();
    virtual void set_size(const vector2& p);    // +0x64
    virtual void w26();
    virtual void set_scale(const vector2& p);   // +0x6c
    virtual void w28(); virtual void w29(); virtual void w30();
    virtual void add_image(int f, void* m);     // +0x7c
    virtual void add_image2(int f, void* m);    // +0x80
    virtual void w34(); virtual void w35(); virtual void w36(); virtual void w37();
    virtual void w38();
    virtual void set_colour(int* c);            // +0x98
    virtual void set_colour2(int* c);           // +0x9c
    virtual void w40();
    virtual void set_text(const vector2& p, const tstring* s);  // +0xa4
    virtual void w42(); virtual void w43(); virtual void w44(); virtual void w45();
    virtual void hide();                        // +0xb8
    virtual void w46(); virtual void w47(); virtual void w48();
    virtual void add_child(void* c);            // +0xc8

    void base_ctor();                           // 0x48ae20
};

// the container widget (vptr 0x592848, size 0x190)
struct UiContainer : Widget {
    char pad0[0x188 - 0x04];
    char f188;
    void ctor_container();                      // 0x4931a0
};

// ---- globals ---------------------------------------------------------------------------------------
struct gui_manager {
    void set_window(Widget** x);                // 0x49b120
};
extern gui_manager g_gui;                       // 0x5d7080

struct ResMgr {
    void* get_object(const tstring& name);      // 0x419540
    void* get_material(const tstring& name);    // 0x418a70
};
extern ResMgr g_resmgr;                         // 0x5d6fa0

void* __fastcall clone_mesh(void* m);           // 0x41c6d0 (mesh::clone, this in ecx)

// the party record whose id field the report menu prints (g_game + 0xf8)
struct Party {
    int get(int x);                             // 0x48ad00
};

// the mesh the background is built from (rgl::mesh)
struct Mesh {
    char pad0[0x2c];
    void* f2c;                                  // +0x2c  material
    char pad30[0x138 - 0x30];
    char f138;                                  // +0x138
    char pad139[0x164 - 0x139];
    char f164;                                  // +0x164
    Mesh();                                     // 0x412a90 (mesh ctor, returns this)
    void mesh_ctor();                           // 0x412a90
    int add_point(float* p);                    // 0x412f50
    int add_vertex(int idx, float* uv, void* col, float* n, void* f);   // 0x413c50
    void add_rec2(int a, int b, int c);         // 0x411920
    void build_panel(float* base, float* size, float* uv0, float* uv1,
                     int* col);                 // 0x4de3e0
};

Mesh* make_background(float* v, int color);     // 0x4de320

struct SoundMgr {
    void stop_all();                            // 0x494310
};
extern SoundMgr g_sound_mgr;                    // 0x5aa128

extern char* g_game;                            // 0x5e2930
extern char g_frame_flag;                       // 0x5d8788
extern char g_flag_a181b8;                      // 0xa181b8

Widget* build_row6(UiContainer* win, const tstring* s, float x, float y, float w,
                   float h);                    // 0x4db810
Widget* make_text(UiContainer* win, const tstring* s, float x, float y, float w);  // 0x4dce70

// the debrief menu object
struct DebriefMenu {
    char pad0[0x10];
    int f10;                                    // +0x10
    char pad14[0x2c - 0x14];
    UiContainer* win;                           // +0x2c
    Widget* f30;                                // +0x30
    Widget* f34;                                // +0x34
    void create_report();                       // 0x4de1b0
    void create_debrief();                      // 0x4de5b0
};

// ---- code ------------------------------------------------------------------------------------------
// @ 0x4de1b0
void DebriefMenu::create_report()
{
    vector2 tv;
    UiContainer* w;
    {
        UiContainer* p = (UiContainer*)::operator new(0x190);
        if (p != 0) {
            p->ctor_container();
            *(void**)p = (void*)0x592848;
            p->f188 = 0;
            w = p;
        } else {
            w = 0;
        }
    }
    win = w;

    float v[4];
    v[0] = 0.3f;
    v[1] = 0.2f;
    win->set_pos(*(vector2*)v);

    v[2] = 0.5f;
    v[3] = 0.3f;
    win->set_size(*(vector2*)(v + 2));

    win->add_image(0, make_background(v + 2, -1));

    Party* party = (Party*)(g_game + 0xf8);
    int h = party->get(f10);
    int* page = *(int**)((char*)party + 0x138d0);
    char* rec = (char*)((h & 0xf) * 0x5ac8 + page[h >> 4]);
    const tstring* text;
    if (g_flag_a181b8)
        text = (const tstring*)(rec + 0x20);
    else if (*(char*)(rec + 0x1c) != 0)
        text = (const tstring*)(rec + 0xc);
    else {
        const tstring* p = g_strtable.find(rec + 0x20);
        text = p != 0 ? p : (const tstring*)(rec + 0xc);
    }
    win->set_text(tv, text);

    {
        tstring t("ui_done");
        f34 = build_row6(win, g_strtable.lookup(t), 0.2f, 0.03f, 0.15f, 0.05f);
    }

    Widget* wv = win;
    g_frame_flag = 1;
    g_gui.set_window(&wv);
}

// @ 0x4de320
Mesh* make_background(float* v, int color)
{
    Mesh* m;
    {
        Mesh* p = (Mesh*)::operator new(0x260);
        if (p != 0)
            p->mesh_ctor();
        else
            p = 0;
        m = p;
    }
    {
        tstring t("background");
        m->f2c = g_resmgr.get_material(t);
    }

    float a[2];
    a[0] = v[0] + v[0];
    a[1] = -v[1] - v[1];

    float z[5];
    z[0] = 0.0f;
    z[1] = 0.0f;
    z[2] = 0.0f;
    z[3] = 0.0f;
    z[4] = 0.0f;

    m->build_panel(&z[2], v, z, a, &color);
    return m;
}

// @ 0x4de3e0
void Mesh::build_panel(float* base, float* size, float* uv0, float* uv1, int* col)
{
    float p[3];
    int idx[4];
    float uv[2];

    p[0] = base[0];
    p[1] = base[1];
    p[2] = base[2];
    idx[0] = add_point(p);

    p[0] = base[0] + size[0];
    p[1] = base[1];
    p[2] = base[2];
    idx[1] = add_point(p);

    p[0] = base[0] + size[0];
    p[1] = size[1] + base[1];
    p[2] = base[2];
    idx[2] = add_point(p);

    p[0] = base[0];
    p[1] = size[1] + base[1];
    p[2] = base[2];
    idx[3] = add_point(p);

    p[0] = 0.0f;
    p[1] = 0.0f;
    p[2] = 1.0f;
    int zero = 0;

    uv[0] = uv0[0];
    uv[1] = uv0[1] - uv1[1];
    int v1 = add_vertex(idx[0], uv, col, p, &zero);

    uv[0] = uv0[0] + uv1[0];
    uv[1] = uv0[1] - uv1[1];
    int v2 = add_vertex(idx[1], uv, col, p, &zero);

    uv[0] = uv0[0] + uv1[0];
    uv[1] = uv0[1];
    int v3 = add_vertex(idx[2], uv, col, p, &zero);

    int v4 = add_vertex(idx[3], uv0, col, p, &zero);

    add_rec2(v1, v2, v3);
    add_rec2(v1, v3, v4);

    f138 = 1;
    f164 = 1;
}

// @ 0x4de5b0
void DebriefMenu::create_debrief()
{
    g_sound_mgr.stop_all();

    UiContainer* w;
    {
        UiContainer* p = (UiContainer*)::operator new(0x190);
        if (p != 0) {
            p->ctor_container();
            *(void**)p = (void*)0x592848;
            p->f188 = 0;
            w = p;
        } else {
            w = 0;
        }
    }
    win = w;

    win->set_pos(vector2(0.2f, 0.15f));
    win->set_size(vector2(0.6f, 0.45f));

    Widget* m;
    {
        tstring t("debrief_window");
        m = (Widget*)clone_mesh(g_resmgr.get_object(t));
    }
    win->add_image(0, m);

    {
        int c = -1;
        win->set_colour(&c);
    }
    {
        tstring t("ui_done");
        f30 = make_text(win, g_strtable.lookup(t), 0.2f, 0.05f, 0.04f);
    }

    {
        Widget* wv = win;
        g_frame_flag = 1;
        g_gui.set_window(&wv);
    }
}
