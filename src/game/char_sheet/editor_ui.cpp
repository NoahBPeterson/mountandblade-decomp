#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include "./editor_ui.h"
#include "../render_batch.h"      // g_batch (0x5fe898): init_formats (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // ContainerBase: add_child 0x493a30
#include "../world_data/thiscall_free.h"   // CALL_STDCALL_ON
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../main_menu.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/mesh2/mesh2_common.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../party2/camera_class.h"      // Camera (the 0x4a8-byte object make_editor_list builds)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_trial_mode (listed directly: the audit reads only the source's own includes)
struct Font;
extern Font* g_font;                                        // 0x5e2934

// ---- data this module owns (docs/standalone_data.md) ----
int g_5e29a4;   // 0x5e29a4 trial-mode flag as engine_init copied it

// @ 0x4f5d20
MissionUi* __cdecl new_key_label(void* parent, const tstring* name, const tstring* text)
{
    MissionUi* w = (MissionUi*)::operator new(sizeof(MissionUi));
    if (w != 0)
        w = new (w, raw_place()) MissionUi;   // 0x48ae20
    else
        w = 0;
    label_refresh(w, *text);
    float out[2];
    w->set_text(out, name);
    ((ContainerBase*)parent)->add_child(w);
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
        m = (mesh*)((mesh_x*)res_manager().get_object((const rgl::string*)&hl))->clone();
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
        m = (mesh*)((mesh_x*)res_manager().get_object((const rgl::string*)&hl))->clone();
    }
    m->scale(x / z);
    ((Widget*)b)->set_track(3, m);
    return b;
}

// @ 0x4f5fb0
UiB* __cdecl make_slider(void* parent, const tstring* name)
{
    UiB* w = (UiB*)::operator new(sizeof(UiB));
    if (w != 0)
        w = new (w, raw_place()) UiB;   // 0x48b4b0 (the base constructor 0x48ae20, the vptr 0x591fb0, the slider fields)
    else
        w = 0;
    label_refresh(w, *name);
    float h = w->screen[0];
    {
        float size[2];
        size[0] = h * 0.25f;
        size[1] = h * 0.03f;
        w->set_xy(size);
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
    ((ContainerBase*)parent)->add_child(w);
    return w;
}

// @ 0x4f6160
UiD* __cdecl make_scrollbar(void* parent, const tstring* name)
{
    UiD* w = (UiD*)::operator new(sizeof(UiD));
    if (w != 0)
        w = new (w, raw_place()) UiD;   // 0x48b890 (the base constructor 0x48ae20, the vptr 0x592140, the scrollbar fields)
    else
        w = 0;
    label_refresh(w, *name);
    float a = w->screen[0] * 50.0f;
    w->f178 = 1;
    float b = w->screen[1] * 1.5576323f;
    w->f17c = b * 0.31f;
    rgl::vector4 col(a, b, 1.0f);
    mesh* m1;
    {
        tstring n("scrollbar");
        m1 = res_manager().get_object((const rgl::string*)&n)->clone();
    }                                           // the name dies before the colour is set (0x4f6160: ~tstring, then set_color)
    m1->set_color(col);
    mesh* m2;
    {
        tstring n("scrollbar_handle");
        m2 = res_manager().get_object((const rgl::string*)&n)->clone();
    }                                           // the name dies before the colour is set (0x4f6160: ~tstring, then set_color)
    m2->set_color(col);
    w->set_track(0, m1);
    w->set_track(2, m2);
    ((ContainerBase*)parent)->add_child(w);
    return w;
}

// @ 0x4f62f0
Camera* __cdecl make_editor_list(void* parent, const tstring* name, int a3)
{
    Camera* w = (Camera*)::operator new(0x4a8);
    if (w != 0)
        w = new (w, raw_place()) Camera;   // 0x49deb0
    else
        w = 0;
    w->f104 |= 0x20;
    if (a3 != 0)
        w->follow[1] = (MissionUi*)a3;
    w->target = (ContainerBase*)parent;
    ((Widget*)w)->add_child(parent);
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

struct GuiCtx;
void __fastcall pick_entity(GuiCtx* self);   // 0x4ed8d0 (char_sheet/pick_entity.cpp)

// @ 0x4f6390
void EditorApp::refresh_dialog()
{
    pick_entity((GuiCtx*)slot_as<EditorGame>(14));     // 0x4ed8d0 (char_sheet/pick_entity.cpp), not FaceGen::refresh
    slot_as<EditorGame>(14)->f2f9 = 1;
    SetFocus(hwnd);
}

// @ 0x4f63c0
int __cdecl engine_init()
{
    srand(timeGetTime());
    g_batch.init_formats();   // 0x4bc5b0
    g_log.name.assign("rgl_log.txt");
    FILE* f = fopen("rgl_log.txt", "w");
    g_log_file = f;
    bool ok = true;
    if (f != 0) {
        g_log_flag94 = 0;
        g_log_flag95 = ok;
    } else {
        g_log_flag94 = ok;
    }
    g_log.autoflush = ok;
    g_log.w_cstr("Starting new log file.\n");   // virtual slot +0x28 (0x4f6441 call [eax+0x28]); text_stream::w_cstr 0x4153a0 at run time
    g_5d705c = g_path_prefix;
    bool b = g_cfg.enable_edit_mode != 0;
    g_5d706c = b;
    g_5d706d = b;
    {
        tstring n("core_shaders");
        res_manager().load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_textures");
        res_manager().load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_materials");
        res_manager().load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_ui_meshes");
        res_manager().load_pack((const rgl::string&)n, 0);
    }
    {
        tstring n("core_pictures");
        res_manager().load_pack((const rgl::string&)n, 0);
    }
    g_strtable.load(&g_module_path);
    {
        FontDef* p = (FontDef*)::operator new(0x386c);
        if (p != 0)
            p = (FontDef*)new (p, raw_place()) Font;   // 0x4d99d0
        g_font = (Font*)p;
    }
    {
        tstring n("Data\\font_data.xml");
        g_font->load((const char*)&n);   // 0x4d9aa0
    }
    ((GuiState*)&orig::g_gui_mgr)->init_widgets();
    init_mission_trigger_records();
    g_5e29a4 = g_trial_mode;
    ((MainMenu*)&orig::g_main_menu_obj)->set_state(2);
    return 0;
}
