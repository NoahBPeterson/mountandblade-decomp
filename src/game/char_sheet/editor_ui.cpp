#include "../../include/image_addrs.h"
#include "../config/config_block.h"      // Config g_cfg (0x5a9f98)
#include "../window_mgr.h"      // g_windows slots (slot_as<T>(i))
#include "./editor_ui.h"
#include "../animator.h"      // Animator (g_anim 0x8ed4f60): get_text 0x4f5d90 is defined here
#include "../render_batch.h"      // g_batch (0x5fe898): init_formats (listed directly: the resolution audit reads only the source's own includes)
#include "../ui/container.h"      // ContainerBase: add_child 0x493a30
#include "../tstring.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../window_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../res_mgr.h"      // (the shared class header named here too: the resolution audit reads the annotations of directly included headers)
#include "../../rgl/mesh2/mesh2_common.h"      // (listed directly: the resolution audit scans only the source's own includes)
#include "../party2/camera_class.h"      // Camera (the 0x4a8-byte object make_editor_list builds)
#include "../ui_widget.h"      // (listed directly: the resolution audit reads only the source's own includes)
#include "../game_globals.h"      // canonical g_trial_mode (listed directly: the audit reads only the source's own includes)
#include "../config/core.h"      // GameApp: OneTimeSceneInit is vtable slot 4 of the application object
#include "../strat/gui_manager.h"      // g_gui (0x5d7080)
#include "../../include/wl_marker.h"   // WL_MARKER: the original's inline WinLicense region markers
// UiD's constructor 0x48b890 as inlined at the call sites: the base constructor, the vptr, then f178, f17c,
// v168, f180, f184.
__forceinline UiD::UiD() { f178 = 0; f17c = 0.1f; v168 = 1.0f; f180 = 0; f184 = 0; }

struct Font;
extern Font* g_font;                                        // 0x5e2934

// ---- data this module owns (docs/standalone_data.md) ----
int g_5e29a4;   // 0x5e29a4 trial-mode flag as engine_init copied it

// @ 0x4f5d20
MissionUi* __cdecl new_key_label(ContainerBase* parent, const tstring* name, const tstring* text)
{
    MissionUi* w = (MissionUi*)::operator new(sizeof(MissionUi));
    if (w != 0)
        w = new (w, raw_place()) MissionUi;   // 0x48ae20
    else
        w = 0;
    label_refresh(w, *text);
    w->set_text(*name);
    parent->add_child(w);
    return w;
}

// @ 0x4f5d90
void Animator::get_text(int skin, int key, tstring* out)
{
    if (key < skin_vec[skin].n_face_keys) {
        FaceKey* e = &skin_vec[skin].face_keys[key];
        tstring* t = g_strtable.find(&e->key);
        if (t == 0)
            t = &e->name;
        *out = *t;
    } else {
        out->assign("error");
    }
}

// the two arrow-button factories share a shape: build the medium button with the three mesh names, then
// clone the highlighted mesh, scale it by x / z and attach it as slot 3.
// @ 0x4f5df0
UiA* __cdecl make_left_arrow(MissionUi* parent, float a2, float a3, float x, float a5, float z)
{
    UiA* b;
    {
        tstring down("left_arrow_down");
        tstring up("left_arrow");
        tstring empty("");
        b = new_sized_button(parent, empty, a2, a3, x, up, down, a5, z, 0.085f);   // 0x4dcc50
    }
    mesh* m;
    {
        tstring hl("left_arrow_hl");
        m = (mesh*)((rgl::mesh*)g_resmgr.get_object(&hl))->clone();
    }
    m->scale(x / z);
    b->set_track(3, m);
    return b;
}

// @ 0x4f5ed0
UiA* __cdecl make_right_arrow(MissionUi* parent, float a2, float a3, float x, float a5, float z)
{
    UiA* b;
    {
        tstring down("right_arrow_down");
        tstring up("right_arrow");
        tstring empty("");
        b = new_sized_button(parent, empty, a2, a3, x, up, down, a5, z, 0.085f);   // 0x4dcc50
    }
    mesh* m;
    {
        tstring hl("right_arrow_hl");
        m = (mesh*)((rgl::mesh*)g_resmgr.get_object(&hl))->clone();
    }
    m->scale(x / z);
    b->set_track(3, m);
    return b;
}

// the original's inlined mesh loader: get_object/clone with the name passed by pointer.
static __forceinline mesh* load_mesh_arg(const tstring& n)
{
    return (mesh*)((rgl::mesh*)g_resmgr.get_object(&n))->clone();
}

// the original's inlined widget allocator: a named pointer, the allocator's result tested with a plain
// `if (p)`, and the raw-placement constructor (the base constructor 0x48ae20, the vptr 0x591fb0, the slider fields).
static __forceinline UiB* new_widget_188()
{
    void* p = ::operator new(sizeof(UiB));
    if (p)
        return new (p, raw_place()) UiB;   // 0x48b4b0
    return 0;
}

// @ 0x4f5fb0
UiB* __cdecl make_slider(ContainerBase* parent, const tstring* name)
{
    UiB* w = new_widget_188();
    label_refresh(w, *name);
    float h = w->rect_max.x;
    {
        w->set_xy(V2(h * 0.25f, h * 0.03f));
    }
    rgl::mesh* track;
    {
        tstring k("slider_hor");
        track = (rgl::mesh*)load_mesh_arg(k);
    }
    rgl::mesh* handle;
    {
        tstring k("handle_hor");
        handle = (rgl::mesh*)load_mesh_arg(k);
    }
    if (h != 1.0f) {
        ((mesh*)track)->set_color(rgl::vector4(h, h, h));
        ((mesh*)handle)->set_color(rgl::vector4(h, h, h));
    }
    w->set_track(0, (rgl::mesh*)track);
    w->set_track(2, (rgl::mesh*)handle);
    parent->add_child(w);
    return w;
}

// @ 0x4f6160
UiD* __cdecl make_scrollbar(ContainerBase* parent, const tstring* name)
{
    UiD* w = (UiD*)::operator new(sizeof(UiD));
    if (w != 0)
        w = new (w, raw_place()) UiD;   // 0x48b890 (the base constructor 0x48ae20, the vptr 0x592140, the scrollbar fields)
    else
        w = 0;
    label_refresh(w, *name);
    rgl::vector4 col;
    col.x = w->rect_max.x * 50.0f;
    w->f178 = 1;
    float y = w->rect_max.y * 1.5576323f;
    col.y = y;
    col.z = 1.0f;
    w->f17c = y * 0.31f;
    mesh* m1;
    {
        tstring n("scrollbar");
        m1 = g_resmgr.get_object(&n)->clone();
    }                                           // the name dies before the colour is set (0x4f6160: ~tstring, then set_color)
    m1->set_color(col);
    mesh* m2;
    {
        tstring n("scrollbar_handle");
        m2 = g_resmgr.get_object(&n)->clone();
    }                                           // the name dies before the colour is set (0x4f6160: ~tstring, then set_color)
    m2->set_color(col);
    w->set_track(0, m1);
    w->set_track(2, m2);
    parent->add_child(w);
    return w;
}

// @ 0x4f62f0
Camera* __cdecl make_editor_list(ContainerBase* parent, const tstring* name, UiD* scroll)
{
    Camera* w = (Camera*)::operator new(sizeof(Camera));
    if (w != 0)
        w = new (w, raw_place()) Camera;   // 0x49deb0
    else
        w = 0;
    w->f104 |= 0x20;
    if (scroll != 0)
        w->follow[1] = scroll;
    w->target = parent;
    w->add_child(parent);
    label_refresh((MissionUi*)w, *name);
    return w;
}

// RagModel (src/include/rag_model.h): the 0x20-byte ragdoll model record; the two std::vector<RagModel> helpers below are the
// header's templates (explicit specializations with the header bodies: callers in inventory_panel/loading_screen.cpp declare them)
#include "../../include/rag_model.h"
namespace std {
template<> void _Destroy_range(RagdollBone*, RagdollBone*, allocator<RagdollBone>&, _Nonscalar_ptr_iterator_tag);   // 0x4d10f0 (module_load)
}
// (the 0x1f8-byte row copier 0x4e5fc0 is std::_Uninit_fill_n<Skin*,...>, editor_ui.h; the two instantiations were folded together by the linker)

// @ 0x4f6360
struct TextRowVec {
    TextRow* insert_n(TextRow* pos, unsigned n, const TextRow& v);
};
TextRow* TextRowVec::insert_n(TextRow* pos, unsigned n, const TextRow& v)
{
    std::_Nonscalar_ptr_iterator_tag cat;
    std::_Uninit_fill_n((Skin*)pos, n, (const Skin&)v, *(std::allocator<Skin>*)this, cat);
    return pos + n;
}

namespace std {
// @ 0x4f65f0
template<> void _Uninit_fill_n(RagModel* _First, unsigned _Count, const RagModel& _Val, allocator<RagModel>& _Al,
                               _Nonscalar_ptr_iterator_tag)
{
    RagModel* _Next = _First;
    _TRY_BEGIN
    for (; 0 < _Count; --_Count, ++_First)
        _Al.construct(_First, _Val);   // 0x4e42f0
    _CATCH_ALL
    for (; _Next != _First; ++_Next)
        _Al.destroy(_Next);            // 0x4ebdc0
    _RERAISE;
    _CATCH_END
}

// @ 0x4f6658 -- the catch funclet of _Uninit_fill_n (0x4f65f0); emitted by the compiler, no symbol of its own.

// @ 0x4f6680
template<> void _Destroy_range(RagModel* _First, RagModel* _Last, allocator<RagModel>& _Al, _Nonscalar_ptr_iterator_tag)
{
    for (; _First != _Last; ++_First)
        _Al.destroy(_First);           // inline: ~vector<RagdollBone> (_Destroy_range<RagdollBone> 0x4d10f0) and ~tstring
}
}

#include "../options_panel/mission_item_recs.h"   // MissionBlocker (slot 14): pick_entity 0x4ed8d0 (char_sheet/pick_entity.cpp)

// @ 0x4f6390
void GameApp::refresh_dialog()
{
    slot_as<MissionBlocker>(14)->pick_entity();     // 0x4ed8d0 (char_sheet/pick_entity.cpp), not FaceGen::refresh
    slot_as<MissionBlocker>(14)->f2f9 = 1;
    SetFocus(m_hWnd);
}

// @ 0x4f63c0
HRESULT GameApp::OneTimeSceneInit()
{
    srand(timeGetTime());
    WL_MARKER(4)
    g_batch.init_formats();   // 0x4bc5b0
    g_log.name.assign("rgl_log.txt");
    FILE* f = fopen("rgl_log.txt", "w");
    g_log.fp = f;
    bool ok = true;
    if (f != 0) {
        g_log.failed = 0;
        g_log.is_open = ok;
    } else {
        g_log.failed = ok;
    }
    g_log.autoflush = ok;
    ((stream_base&)g_log).w_cstr("Starting new log file.\n");   // virtual slot +0x28 (0x4f6441 call [eax+0x28]); text_stream::w_cstr 0x4153a0 at run time
    g_resmgr.root = g_module_path;
    bool b = g_cfg.enable_edit_mode != 0;
    g_resmgr.editor_cc = b;
    g_resmgr.editor_cd = b;
    {
        tstring n("core_shaders");
        g_resmgr.load_pack(n, 0);
    }
    {
        tstring n("core_textures");
        g_resmgr.load_pack(n, 0);
    }
    {
        tstring n("core_materials");
        g_resmgr.load_pack(n, 0);
    }
    {
        tstring n("core_ui_meshes");
        g_resmgr.load_pack(n, 0);
    }
    {
        tstring n("core_pictures");
        g_resmgr.load_pack(n, 0);
    }
    g_strtable.load(&g_module_path);
    {
        FontDef* p = (FontDef*)::operator new(sizeof(Font));      // 0x386c
        if (p != 0)
            g_font = (Font*)new (p, raw_place()) Font;   // 0x4d99d0
        else
            g_font = (Font*)p;
    }
    {
        tstring n("Data\\font_data.xml");
        g_font->load(n);   // 0x4d9aa0
    }
    g_gui.reset_geometry();   // 0x4f658d calls 0x4e64b0 (the message log's 32+32 meshes), not gui_manager::reset 0x42d1b0
    WL_MARKER(5)
    init_mission_trigger_records();
    WL_MARKER(12)
    g_5e29a4 = g_trial_mode;
    g_windows.set_state(2);
    WL_MARKER(13)
    return 0;
}
