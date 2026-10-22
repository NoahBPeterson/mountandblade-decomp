// clang-format off
// Slice s004f5d20: the last block of the mission/options UI widget factories (the arrow buttons, the
// horizontal slider and the scroll bar), the map editor's dialog refresh hook (0x4f6390), the engine
// start-up sequence (0x4f63c0) and the tail of a std::vector<T> implementation for a 0x20-byte record
// (0x4f6360 insert-n, 0x4f65f0 construct-n with its catch funclet at 0x4f6658, 0x4f6680 destroy-range).
//
// Widget slot offsets (+0x58/+0x64/+0x6c/+0x70/+0x80/+0xa4/+0xc8), the tstring layout and the
// resource-manager / language-table / log-stream globals follow the verified neighbouring slices
// (s004dfd50, s004dc630, s004e05c0, s0049e780, s004bd990).  Names are provisional.
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include "rgl.h"

// ---- engine string --------------------------------------------------------------------------------
struct tstring {
    int pos;            // +0x00
    char* buf;          // +0x04
    int cap;            // +0x08
    int len;            // +0x0c
    tstring(const char* s);                     // 0x42bbe0
    tstring(const tstring& o);                  // 0x42c110
    ~tstring();                                 // 0x42bc40
    tstring& operator=(const tstring& o);       // 0x42bc50
    void assign(const char* s);                 // 0x42bcb0
};

// ---- resource manager (0x5d6fa0) -------------------------------------------------------------------
struct mesh {
    mesh* clone();                              // 0x41c6d0
    void set_color(rgl::vector4 c);             // 0x47af30 (four floats by value)
    void scale(float s);                        // 0x44f0c0
};

struct resmgr {
    mesh* get_object(const tstring& name);      // 0x419540
    void load_pack(const tstring& name, int user);  // 0x41e150
};
extern resmgr g_alloc;                          // 0x5d6fa0

// resource lookup + clone, as the factories do it (the original inlines this twice, giving the two
// tstring temporaries separate stack slots)
__forceinline mesh* load_mesh(const char* name)
{
    tstring n(name);
    return g_alloc.get_object(n)->clone();
}

// ---- language / text tables (0x5e2a18) -------------------------------------------------------------
struct LangTables {
    tstring* lookup(const tstring& key);        // 0x4c6010 (null when missing)
    void load(const char* root);                // 0x4cda90
};
extern LangTables g_lang;                       // 0x5e2a18

// ---- text log stream (0x5a9f60, see s004bd990) -----------------------------------------------------
struct stream_base {
    virtual void s00(); virtual void s01(); virtual void s02(); virtual void s03(); virtual void s04();
    virtual void s05(); virtual void s06(); virtual void s07(); virtual void s08(); virtual void s09();
    virtual void w_cstr(const char* s);         // +0x28 (slot 10)
};
extern stream_base g_log;                       // 0x5a9f60

// ---- widgets ---------------------------------------------------------------------------------------
struct Widget {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03(); virtual void v04();
    virtual void v05(); virtual void v06(); virtual void v07(); virtual void v08(); virtual void v09();
    virtual void v10(); virtual void v11(); virtual void v12(); virtual void v13(); virtual void v14();
    virtual void v15(); virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void v22(float* p);                 // +0x58
    virtual void v23(); virtual void v24();
    virtual void v25(float* p);                 // +0x64
    virtual void v26();
    virtual void v27(float* p);                 // +0x6c
    virtual void v28(float* p);                 // +0x70
    virtual void v29(); virtual void v30(); virtual void v31();
    virtual void v32(int slot, mesh* m);        // +0x80 add_image
    virtual void v33(); virtual void v34(); virtual void v35(); virtual void v36(); virtual void v37();
    virtual void v38(void* c);                  // +0x98
    virtual void v39(void* c);                  // +0x9c
    virtual void v40();
    virtual void v41(void* out, const tstring* name);  // +0xa4
    virtual void v42(); virtual void v43(); virtual void v44(); virtual void v45(); virtual void v46();
    virtual void v47(); virtual void v48(); virtual void v49();
    virtual void v50(void* child);              // +0xc8 add_child

    Widget* base_ctor();                        // 0x48ae20 (returns this in eax)
};

// the plain 0x178-byte widget: only the base ctor runs
struct WidgetA : Widget {
    char pad[0x178 - 4];
};

// the 0x188-byte slider / scrollbar widgets (vptr 0x591fb0 / 0x592140)
struct WidgetB : Widget {
    char pad0[0x168 - 4];
    float f168;                                 // +0x168
    char pad1[0x178 - 0x16c];
    int f178;                                   // +0x178
    float f17c;                                 // +0x17c
    int f180;                                   // +0x180
    int f184;                                   // +0x184
};

// the 0x4a8-byte editor list widget (ctor 0x49deb0)
struct WidgetC : Widget {
    char pad0[0x104 - 4];
    int f104;                                   // +0x104
    char pad1[0x484 - 0x108];
    int f484;                                   // +0x484
    char pad2[0x490 - 0x488];
    int f490;                                   // +0x490
    WidgetC* ctor();                            // 0x49deb0 (returns this)
};

// ---- helpers declared elsewhere --------------------------------------------------------------------
void load_params(Widget* w, tstring name);      // 0x4e4c70 (tstring by value)
void* make_medium_button(void* parent, const tstring* text, int a2, int a3, float a4,
                         const tstring* up, const tstring* down, int a5, float a6, float k);  // 0x4dcc50


// ---- globals ---------------------------------------------------------------------------------------
extern int g_no_cam;                            // 0x5a9fb0
extern tstring g_log_path;                      // 0x5a9f64
extern FILE* g_log_file;                        // 0x5a9f90
extern unsigned char g_log_flag94;              // 0x5a9f94
extern unsigned char g_log_flag95;              // 0x5a9f95
extern unsigned char g_log_flag78;              // 0x5a9f78
extern tstring g_path_prefix;                   // 0x8ee3c28
extern char g_path_prefix_chars[];              // 0x8ee3c28
extern tstring g_5d705c;                        // 0x5d705c
extern unsigned char g_5d706c;                  // 0x5d706c
extern unsigned char g_5d706d;                  // 0x5d706d
extern int g_5e29a4;                            // 0x5e29a4
extern int g_5e29c4;                            // 0x5e29c4
extern void* g_font_def;                        // 0x5e2934

struct Startup { void init(); };                // 0x5fe898
struct GuiState { void init_widgets(); };       // 0x5d7080
struct MainMenu { void choose(int i); };        // 0xa73980
struct EditorGame {
    void refresh();
    char pad[0x2f9];
    unsigned char f2f9;                         // +0x2f9
};
extern EditorGame* g_editor_game;               // 0xa739b8

struct Screen {
    char pad[0xe8];
    HWND hwnd;                                  // +0xe8
    void refresh_dialog();
};

struct FontDef { void load_fonts(tstring* path); };  // 0x4d9aa0
FontDef* ctor_font_def(FontDef* p);             // 0x4d99d0
void init_mission_triggers();                   // 0x4d0620

// ---- per-row text table of the mission-UI widget ----------------------------------------------------
struct TextEntry {                              // 0x34 bytes
    char pad0[0x10];
    tstring text;                               // +0x10
    char pad1[0x34 - 0x20];
};
struct TextRow {                                // 0x1f8 bytes
    char pad0[0x3c];
    int count;                                  // +0x3c
    TextEntry* entries;                         // +0x40
    char pad1[0x1f8 - 0x44];
};

// @ 0x4f5d20
WidgetA* __cdecl make_label(void* parent, const tstring* name, const tstring* text)
{
    WidgetA* w = (WidgetA*)::operator new(0x178);
    if (w != 0)
        w = (WidgetA*)w->base_ctor();
    else
        w = 0;
    load_params(w, *text);
    float out[2];
    w->v41(out, name);
    ((Widget*)parent)->v50(w);
    return w;
}

// @ 0x4f5d90
struct TextPart {
    char pad0[0x1c];
    TextRow* rows;                              // +0x1c
    void get_text(int row, int index, tstring* out);
};
void TextPart::get_text(int row, int index, tstring* out)
{
    if (index < rows[row].count) {
        TextEntry* e = &rows[row].entries[index];
        tstring* t = g_lang.lookup(e->text);
        if (t == 0)
            t = (tstring*)e;
        *out = *t;
    } else {
        out->assign("error");
    }
}

// the two arrow-button factories share a shape: build the medium button with the three mesh names, then
// clone the highlighted mesh, scale it by x / z and attach it as slot 3.
// @ 0x4f5df0
void* __cdecl make_left_arrow(void* parent, int a2, int a3, float x, int a5, float z)
{
    void* b;
    {
        tstring down("left_arrow_down");
        tstring up("left_arrow");
        tstring empty("");
        b = make_medium_button(parent, &empty, a2, a3, x, &up, &down, a5, z, 0.085f);
    }
    mesh* m;
    {
        tstring hl("left_arrow_hl");
        m = g_alloc.get_object(hl)->clone();
    }
    m->scale(x / z);
    ((Widget*)b)->v32(3, m);
    return b;
}

// @ 0x4f5ed0
void* __cdecl make_right_arrow(void* parent, int a2, int a3, float x, int a5, float z)
{
    void* b;
    {
        tstring down("right_arrow_down");
        tstring up("right_arrow");
        tstring empty("");
        b = make_medium_button(parent, &empty, a2, a3, x, &up, &down, a5, z, 0.085f);
    }
    mesh* m;
    {
        tstring hl("right_arrow_hl");
        m = g_alloc.get_object(hl)->clone();
    }
    m->scale(x / z);
    ((Widget*)b)->v32(3, m);
    return b;
}

// @ 0x4f5fb0
WidgetB* __cdecl make_slider(void* parent, const tstring* name)
{
    WidgetB* w = (WidgetB*)::operator new(0x188);
    if (w != 0) {
        w->base_ctor();
        *(void**)w = (void*)0x591fb0;
        w->f178 = 0;
        w->f17c = 0.008f;
        w->f168 = 1.0f;
        w->f180 = 0;
        w->f184 = 0;
    } else {
        w = 0;
    }
    load_params(w, *name);
    float h = *(float*)((char*)w + 0xf0);
    {
        float size[2];
        size[0] = h * 0.25f;
        size[1] = h * 0.03f;
        w->v25(size);
    }
    mesh* track = load_mesh("slider_hor");
    mesh* handle = load_mesh("handle_hor");
    if (h != 1.0f) {
        rgl::vector4 c(h, h, h);
        track->set_color(c);
        handle->set_color(c);
    }
    w->v32(0, track);
    w->v32(2, handle);
    ((Widget*)parent)->v50(w);
    return w;
}

// @ 0x4f6160
WidgetB* __cdecl make_scrollbar(void* parent, const tstring* name)
{
    WidgetB* w = (WidgetB*)::operator new(0x188);
    if (w != 0) {
        w->base_ctor();
        *(void**)w = (void*)0x592140;
        w->f178 = 0;
        w->f17c = 0.1f;
        w->f168 = 1.0f;
        w->f180 = 0;
        w->f184 = 0;
    } else {
        w = 0;
    }
    load_params(w, *name);
    float a = *(float*)((char*)w + 0xf0) * 50.0f;
    w->f178 = 1;
    float b = *(float*)((char*)w + 0xf4) * 1.5576323f;
    w->f17c = b * 0.31f;
    rgl::vector4 col(a, b, 1.0f);
    mesh* m1;
    {
        tstring n("scrollbar");
        m1 = g_alloc.get_object(n)->clone();
        m1->set_color(col);
    }
    mesh* m2;
    {
        tstring n("scrollbar_handle");
        m2 = g_alloc.get_object(n)->clone();
        m2->set_color(col);
    }
    w->v32(0, m1);
    w->v32(2, m2);
    ((Widget*)parent)->v50(w);
    return w;
}

// @ 0x4f62f0
WidgetC* __cdecl make_editor_list(void* parent, const tstring* name, int a3)
{
    WidgetC* w = (WidgetC*)::operator new(0x4a8);
    if (w != 0)
        w = w->ctor();
    else
        w = 0;
    w->f104 |= 0x20;
    if (a3 != 0)
        w->f484 = a3;
    w->f490 = (int)parent;
    w->v50(parent);
    load_params(w, *name);
    return w;
}

// @ 0x4f6360
struct Rec20 {
    tstring name;                               // +0x00
    void* vec_begin;                            // +0x10
    int vec_end;                                // +0x14
    int vec_cap;                                // +0x18
    void* f1c;                                  // +0x1c
};
struct RecAlloc {
    void construct(Rec20* dst, const Rec20& src);   // 0x4e42f0
    void destroy(Rec20* p);                         // 0x4ebdc0
};
// the 0x1f8-byte row copier (4e5fc0 itself only reads the first four arguments; the caller passes the
// value a second time, and the two instantiations were folded together by the linker)
void construct_rows(TextRow* dst, unsigned n, const TextRow& v, void* ctx, const TextRow& v2);  // 0x4e5fc0

struct RowVec {
    TextRow* insert_n(TextRow* pos, unsigned n, const TextRow& v);
};
TextRow* RowVec::insert_n(TextRow* pos, unsigned n, const TextRow& v)
{
    construct_rows(pos, n, v, this, v);
    return pos + n;
}

// @ 0x4f65f0
void construct_n(Rec20* first, unsigned n, const Rec20& v, RecAlloc* a)
{
    Rec20* start = first;
    try {
        while (n > 0) {
            a->construct(first, v);
            --n;
            ++first;
        }
    } catch (...) {
        for (; start != first; ++start)
            a->destroy(start);
        throw;
    }
}

// @ 0x4f6658 -- the catch funclet of construct_n (0x4f65f0); emitted by the compiler, no symbol of its own.

// @ 0x4f6680
// 0x4d10f0 is the out-of-line array destructor for 0x50-byte records; it only reads its first two
// arguments (the two range pointers), the caller passes the sub-object and the range start as well
// (the extra parameters were folded away by the linker).
void free_elem_vec(void* p, void* end, void* sub, void* ctx);   // 0x4d10f0
void __cdecl destroy_range(Rec20* first, Rec20* last)
{
    for (; first != last; ++first) {
        void** v = (void**)((char*)first + 0x14);
        if (*v != 0) {
            free_elem_vec(*v, v[1], (char*)first + 0x10, first);
            ::operator delete(*v);
        }
        *v = 0;
        v[1] = 0;
        v[2] = 0;
        first->name.~tstring();
    }
}

// @ 0x4f6390
void Screen::refresh_dialog()
{
    g_editor_game->refresh();
    g_editor_game->f2f9 = 1;
    SetFocus(hwnd);
}

// @ 0x4f63c0
int __cdecl engine_init()
{
    srand(timeGetTime());
    ((Startup*)0x5fe898)->init();
    g_log_path.assign("rgl_log.txt");
    FILE* f = fopen("rgl_log.txt", "w");
    g_log_file = f;
    bool ok = true;
    if (f != 0) {
        g_log_flag94 = 0;
        g_log_flag95 = ok;
    } else {
        g_log_flag94 = ok;
    }
    g_log_flag78 = ok;
    g_log.w_cstr("Starting new log file.\n");
    g_5d705c = g_path_prefix;
    bool b = g_no_cam != 0;
    g_5d706c = b;
    g_5d706d = b;
    {
        tstring n("core_shaders");
        g_alloc.load_pack(n, 0);
    }
    {
        tstring n("core_textures");
        g_alloc.load_pack(n, 0);
    }
    {
        tstring n("core_materials");
        g_alloc.load_pack(n, 0);
    }
    {
        tstring n("core_ui_meshes");
        g_alloc.load_pack(n, 0);
    }
    {
        tstring n("core_pictures");
        g_alloc.load_pack(n, 0);
    }
    g_lang.load(g_path_prefix_chars);
    {
        FontDef* p = (FontDef*)::operator new(0x386c);
        if (p != 0)
            p = ctor_font_def(p);
        g_font_def = p;
    }
    {
        tstring n("Data\\font_data.xml");
        ((FontDef*)g_font_def)->load_fonts(&n);
    }
    ((GuiState*)0x5d7080)->init_widgets();
    init_mission_triggers();
    g_5e29a4 = g_5e29c4;
    ((MainMenu*)0xa73980)->choose(2);
    return 0;
}
