#include "./editor_ui.h"
#include "../world_data/thiscall_free.h"   // CALL_STDCALL_ON
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/mesh2/mesh2_common.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_trial_mode (listed directly: the audit reads only the source's own includes)

// @ 0x4f5d20
WidgetA* __cdecl new_key_label(void* parent, const tstring* name, const tstring* text)
{
    WidgetA* w = (WidgetA*)::operator new(0x178);
    if (w != 0)
        w = (WidgetA*)new (w, raw_place()) MissionUi;   // 0x48ae20
    else
        w = 0;
    label_refresh((MissionUi*)w, *text);
    float out[2];
    w->set_text(out, *name);
    ((Widget*)parent)->add_child(w);
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
        tstring* t = g_strtable.find(&e->text);
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
        m = (mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&hl))->clone();
    }
    m->scale(x / z);
    ((Widget*)b)->set_track(3, m);
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
        m = (mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&hl))->clone();
    }
    m->scale(x / z);
    ((Widget*)b)->set_track(3, m);
    return b;
}

// @ 0x4f5fb0
WidgetB* __cdecl make_slider(void* parent, const tstring* name)
{
    WidgetB* w = (WidgetB*)::operator new(0x188);
    if (w != 0) {
        new (w, raw_place()) MissionUi;   // 0x48ae20
        *(void**)w = (void*)0x591fb0;
        w->f178 = 0;
        w->f17c = 0.008f;
        w->f168 = 1.0f;
        w->f180 = 0;
        w->f184 = 0;
    } else {
        w = 0;
    }
    label_refresh((MissionUi*)w, *name);
    float h = *(float*)((char*)w + 0xf0);
    {
        float size[2];
        size[0] = h * 0.25f;
        size[1] = h * 0.03f;
        w->set_scale(size);
    }
    mesh* track = load_mesh("slider_hor");
    mesh* handle = load_mesh("handle_hor");
    if (h != 1.0f) {
        rgl::vector4 c(h, h, h);
        track->set_color(c);
        handle->set_color(c);
    }
    w->set_track(0, track);
    w->set_track(2, handle);
    ((Widget*)parent)->add_child(w);
    return w;
}

// @ 0x4f6160
WidgetB* __cdecl make_scrollbar(void* parent, const tstring* name)
{
    WidgetB* w = (WidgetB*)::operator new(0x188);
    if (w != 0) {
        new (w, raw_place()) MissionUi;   // 0x48ae20
        *(void**)w = (void*)0x592140;
        w->f178 = 0;
        w->f17c = 0.1f;
        w->f168 = 1.0f;
        w->f180 = 0;
        w->f184 = 0;
    } else {
        w = 0;
    }
    label_refresh((MissionUi*)w, *name);
    float a = *(float*)((char*)w + 0xf0) * 50.0f;
    w->f178 = 1;
    float b = *(float*)((char*)w + 0xf4) * 1.5576323f;
    w->f17c = b * 0.31f;
    rgl::vector4 col(a, b, 1.0f);
    mesh* m1;
    {
        tstring n("scrollbar");
        m1 = (mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&n))->clone();
    }                                           // the name dies before the colour is set (0x4f6160: ~tstring, then set_color)
    m1->set_color(col);
    mesh* m2;
    {
        tstring n("scrollbar_handle");
        m2 = (mesh*)((mesh_x*)g_alloc.get_object((const rgl::string*)&n))->clone();
    }                                           // the name dies before the colour is set (0x4f6160: ~tstring, then set_color)
    m2->set_color(col);
    w->set_track(0, m1);
    w->set_track(2, m2);
    ((Widget*)parent)->add_child(w);
    return w;
}

// @ 0x4f62f0
WidgetC* __cdecl make_editor_list(void* parent, const tstring* name, int a3)
{
    WidgetC* w = (WidgetC*)::operator new(0x4a8);
    if (w != 0)
        w = (WidgetC*)new (w, raw_place()) c49deb0::Camera;   // 0x49deb0
    else
        w = 0;
    w->f104 |= 0x20;
    if (a3 != 0)
        w->f484 = a3;
    w->f490 = (int)parent;
    w->add_child(parent);
    label_refresh((MissionUi*)w, *name);
    return w;
}

// @ 0x4f6360
struct NameVecRec {                             // 0x20 bytes: a name and a vector (renamed from Rec20: agent/agent.h Rec20 is the g_tbl20 record)
    tstring name;                               // +0x00
    void* vec_begin;                            // +0x10
    int vec_end;                                // +0x14
    int vec_cap;                                // +0x18
    void* f1c;                                  // +0x1c
};
struct RecAlloc {};
struct UiRow;
void __stdcall destroy(UiRow* row);                          // 0x4ebdc0 (screen.cpp: a free stdcall function; the allocator is loaded into ecx first)
// (the 0x1f8-byte row copier 0x4e5fc0 is std::_Uninit_fill_n<loading_config*,...>, editor_ui.h; the two instantiations were folded together by the linker)

struct TextRowVec {
    TextRow* insert_n(TextRow* pos, unsigned n, const TextRow& v);
};
TextRow* TextRowVec::insert_n(TextRow* pos, unsigned n, const TextRow& v)
{
    std::_Uninit_fill_n((loading_config*)pos, n, (const loading_config&)v, *(std::allocator<loading_config>*)this, std::_Nonscalar_ptr_iterator_tag());
    return pos + n;
}

// @ 0x4f65f0
void uninit_fill(NameVecRec* first, unsigned n, const NameVecRec& v, RecAlloc* a)
{
    NameVecRec* start = first;
    try {
        while (n > 0) {
            ((std::allocator<RagModel>*)(void*)a)->construct((RagModel*)first, (const RagModel&)v);
            --n;
            ++first;
        }
    } catch (...) {
        for (; start != first; ++start)
            CALL_STDCALL_ON(destroy, a, start);   // 0x4ebdc0
        throw;
    }
}

// @ 0x4f6658 -- the catch funclet of construct_n (0x4f65f0); emitted by the compiler, no symbol of its own.

// @ 0x4f6680
// 0x4d10f0 is the out-of-line array destructor for 0x50-byte records; it only reads its first two
// arguments (the two range pointers), the caller passes the sub-object and the range start as well
// (the extra parameters were folded away by the linker).
// (0x4d10f0 is item_modifier_array_dtor, editor_ui.h)
void __cdecl destroy_range(NameVecRec* first, NameVecRec* last)
{
    for (; first != last; ++first) {
        void** v = (void**)((char*)first + 0x14);
        if (*v != 0) {
            item_modifier_array_dtor((item_modifier*)*v, (item_modifier*)v[1]);
            ::operator delete(*v);
        }
        *v = 0;
        v[1] = 0;
        v[2] = 0;
        first->name.~tstring();
    }
}

// @ 0x4f6390
void EditorApp::refresh_dialog()
{
    extern bool g_facegen_ready;
    if (g_facegen_ready)                     // the sliders only exist after FaceGen::init (0x4f49d0) ran
        ((FaceGen*)g_editor_game)->refresh();   // 0x4eb3c0 (the editor game is the FaceGen)
    g_editor_game->f2f9 = 1;
    SetFocus(hwnd);
}

// @ 0x4f63c0
int __cdecl engine_init()
{
    srand(timeGetTime());
    ((render_batch*)0x5fe898)->init_formats();   // 0x4bc5b0
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
    ((text_stream*)(void*)&g_log)->text_stream::w_cstr("Starting new log file.\n");   // 0x4153a0 (g_log is a text_stream)
    g_5d705c = g_path_prefix;
    bool b = g_no_cam != 0;
    g_5d706c = b;
    g_5d706d = b;
    {
        tstring n("core_shaders");
        g_alloc.load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_textures");
        g_alloc.load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_materials");
        g_alloc.load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_ui_meshes");
        g_alloc.load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_pictures");
        g_alloc.load_pack((const rgl::string&)n, 0);
    }
    g_strtable.load(g_path_prefix_chars);
    {
        FontDef* p = (FontDef*)::operator new(0x386c);
        if (p != 0)
            p = (FontDef*)new (p, raw_place()) Font;   // 0x4d99d0
        g_font = p;
    }
    {
        tstring n("Data\\font_data.xml");
        ((Font*)g_font)->load((const char*)&n);   // 0x4d9aa0
    }
    ((GuiState*)0x5d7080)->init_widgets();
    init_mission_trigger_records();
    g_5e29a4 = g_trial_mode;
    ((MainMenu*)0xa73980)->set_state(2);
    return 0;
}
