// cs_common.h: classes and functions of other translation units that the character-sheet / face-generator modules call, declared
// under the defining class's name with only the members they call (the defining headers do not go with these modules' views).
// One declaration per line, each with the address of the function that the definition (named in the comment) provides: the
// equivalence harness resolves a callee from the '// 0x...' comment on its declaration line.
#ifndef CHAR_SHEET_CS_COMMON_H
#define CHAR_SHEET_CS_COMMON_H
struct tstring;                                                     // (complete in the includer: game/tstring.h, or terrain_page.h's own view)

#include "../ui/ui_widgets.h"                                      // MissionUi, MissionPanel, UiB, UiD: the real classes
#include "../window_mgr.h"                                    // WindowMgrW g_windows (0xa73980): the one definition
struct FaceRace {                                                   // terrain_b
    void relax(int index, float* values);                           // 0x4735e0
};
struct FaceGenome {                                                 // terrain_b: the animator g_anim 0x8ed4f60 is one
    FaceRace* unpack(const unsigned __int64* rec);                  // 0x473800
};
struct Core {                                                       // config/config.cpp: the application object 0x8ee3c48
    void update_view_rect();                                        // 0x4bace0
};
void __cdecl load_game_variables();                                 // 0x4c0520 (trigger_vec)
struct loading_config { char pad[0x1f8]; };                         // a SkinSet / TextRow
struct SoundCtl {                                                   // agent/agent.h; the game object's event emitter
    char emit_event(int id, int a, int b);                          // 0x435790
};
struct TroopEq {                                                    // terrain_a/misc.h
    void auto_equip(int seed, int unused, char civilian, char no_helmet);   // 0x4687e0
};

namespace rgl {
struct entity;
struct scene;
struct vector4;
struct mesh;
struct world {                                                      // the scene World
    entity* create_entity(int type);                                // 0x428440 (rgl/scene/entity_types.cpp)
    void render_scene(scene* s, bool a);                            // 0x429890 (render_scene.cpp)
};
}

// ---- widgets: the constructors that the char_sheet views (MissionUi / NodeA / text_box) stand for -------------------
// ui_widget.h's MissionUi() and container.h's NodeA() are declarations of the constructors that the definitions name MissionUi::MissionUi
// (0x48ae20) and ContainerW::construct (0x4931a0, which stores the container vptr 0x5924a0, clears +0x17c..+0x184 and returns this). An
// object is built as the original does: operator new, that constructor in place, then the final vptr when the kind has its own
// (NodeA 0x592848, text_box 0x597050).
#include "../../include/image_addrs.h"
#include <new>
#include "../world_data/raw_place.h"
namespace rgl { struct mesh; }                                     // (the set_track pointer argument; ui/mission_ui.h has the full class)
struct ContainerW {                                                 // world_data/world_data.h
    void* construct();                                              // 0x4931a0
};
extern char vtbl_597050[];                                          // 0x597050 (text_box, FaceBoard)
extern char vtbl_592848[];                                          // 0x592848 (NodeA; the caller also clears the byte at +0x188)
static __forceinline void* new_widget_base(unsigned size)
{
    void* p = ::operator new(size);
    if (p)
        new (p, raw_place()) MissionUi;
    return p;
}
static __forceinline void* new_container(unsigned size, void* vtbl)
{
    void* p = ::operator new(size);
    if (p) {
        ((ContainerW*)p)->construct();
        if (vtbl)
            *(void**)p = vtbl;
    }
    return p;
}
int __cdecl clamp_index(const int* a, const int* b, const int* c); // 0x432510 (game_util)
struct ContainerBase;
struct UiA;
UiA* __cdecl make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4e5570 (progress_bar)
MissionUi* __cdecl make_hint(ContainerBase* parent, const tstring& text, float a, float b, float c, float d);   // 0x4dada0 (cls_map)
#include "../../include/sound_mgr.h"
// the particle / shadow system g_psys (0xa70248)
struct Wind;
struct CellGrid {                                                   // the scene World (scene_world.h; mission)
    CellGrid(const float* lo, const float* hi, const int* a, const int* b, const int* c);   // 0x478ad0
    Wind* spawn_wind();                                             // 0x478f30
};
#include "../shadow_map.h"
struct ShadowMapP {
    void render();                                                  // 0x47fef0
};
UiA* __cdecl new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4dce70 (cls_map)
UiA* __cdecl new_button(ContainerBase* o, const tstring& text, float px, float py, float sx, float sy);   // 0x4db810 (cls_map)
UiA* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
                            const tstring& name, const tstring& down, float k1, float k2, float k3);   // 0x4dcc50 (cls_map)

// out-of-line std instances (the defining modules are named in the comments)
#include <vector>
#include <memory>
namespace std {
template<> loading_config* _Uninit_copy(vector<loading_config>::const_iterator first, vector<loading_config>::const_iterator last,
                                         loading_config* dest, allocator<loading_config>& al, _Nonscalar_ptr_iterator_tag);   // 0x4e4340 (progress_bar)
template<> void _Uninit_fill_n(loading_config* first, unsigned n, const loading_config& v, allocator<loading_config>& al,
                               _Nonscalar_ptr_iterator_tag);                                                                 // 0x4e5fc0 (progress_bar)
}
loading_config* __cdecl copy_configs_forward(loading_config* first, loading_config* last, loading_config* dest);   // 0x4da880 (cls_map; the original passes a dead fourth argument)
struct named_object;
namespace std { template<> void vector<named_object*, allocator<named_object*> >::push_back(named_object* const& v); }   // 0x41b750 (resmgr)
namespace std { template<> void vector<int, allocator<int> >::push_back(const int& x); }   // 0x440b30 (agent_combat)
namespace std { template<> vector<int, allocator<int> >& vector<int, allocator<int> >::operator=(const vector<int, allocator<int> >& x); }   // 0x43f0a0 (agent_combat)
struct Map10007 {                                                   // ui/mission.h: the id hash table that opens the party manager
    int lookup(unsigned id);                                        // 0x48ad00
};
struct SceneObject;
struct RagModel;
namespace std { template<> void allocator<RagModel>::construct(RagModel* p, const RagModel& v); }   // 0x4e42f0 (progress_bar)
struct item_modifier;
void __cdecl item_modifier_array_dtor(item_modifier* first, item_modifier* last);   // 0x4d10f0 (module_load; some callers pass two more arguments it never reads)
// the children vector of the container widgets (std::vector<SceneObject*>, 0x10 bytes: allocator slot, first, last, end): its
// out-of-line _Insert_n instance is 0x4927d0 (world_data); _Insert_n is a protected member, reached through a derived class
struct SceneObjVec : std::vector<SceneObject*> { using std::vector<SceneObject*>::_Insert_n; };
namespace std { template<> void vector<SceneObject*, allocator<SceneObject*> >::_Insert_n(iterator where, size_type n, const value_type& v); }   // 0x4927d0 (world_data)
// a children vector's push_back as the original inlines it: with room, store at last; else _Insert_n(last, 1, &v) (0x4927d0)
template<class V, class T> __forceinline void push_back_inline(V& c, T* const& v)
{
    if (c._Myfirst != 0 && (unsigned)(c._Mylast - c._Myfirst) < (unsigned)(c._Myend - c._Myfirst))
        *c._Mylast++ = v;
    else
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c._Mylast), 1, (SceneObject* const&)v);
}

struct Camera;                                                      // party2/camera_class.h: the 0x4a8-byte view / editor list object (ctor 0x49deb0)
struct MapWidget;
MapWidget* __fastcall ctor_4ed630(MapWidget* self);                 // 0x4ed630 (the 3D view widget's constructor; MapWidget() of map_widget.h names it)
MissionUi* __cdecl new_key_label(void* parent, const tstring* name, const tstring* text);   // 0x4f5d20 (editor_ui)
void __cdecl label_refresh(MissionUi* w, tstring name);             // 0x4e4c70 (progress_bar: sets a freshly built widget up from its ini keys)
UiD* __cdecl make_scrollbar(void* parent, const tstring* name); // 0x4f6160 (editor_ui)
UiB* __cdecl make_slider(void* parent, const tstring* name);    // 0x4f5fb0 (editor_ui)
void* __cdecl make_left_arrow(void* parent, int a2, int a3, float x, int a5, float z);    // 0x4f5df0 (editor_ui)
void* __cdecl make_right_arrow(void* parent, int a2, int a3, float x, int a5, float z);   // 0x4f5ed0 (editor_ui)
Camera* __cdecl make_editor_list(void* parent, const tstring* name, int scroll);   // 0x4f62f0 (editor_ui)
struct gui_obj;
struct gui_manager {                                                // strat/gui_manager.h (the object at 0x5d7080)
    char pad00[0x10];
    void* ui_active;                                                // +0x10 the widget being dragged (g_ui_active 0x5d7090)
    float mouse[2];                                                 // +0x14 the pointer position (0x5d7094 / 0x5d7098)
    void reset();                                                   // 0x42d1b0
    void draw_string(tstring* s, float size, int color, char b);    // 0x4e43e0 (ret 0xc)
    void set_o568(gui_obj* o, bool reinit);                         // 0x42cdb0 (ret 8)
};
struct gui_view {                                                   // the same object, its view part
    void set_view(const float* p);                                  // 0x4c5190
    void wheel_step(int code);                                      // 0x4c52f0 (records a key in the hint tables)
    void clear_selection();                                         // 0x4c5330
};
struct PartyMgrW {                                                  // world_data/world_data.h: the party manager at game+0xf8
    int is_live(unsigned id);                                       // 0x4903e0
};
struct Buffer {                                                     // world_data.h: the widget base
    virtual ~Buffer();                                              // 0x487c00
};
void __cdecl menu_variables_loaded(char flag);                      // 0x4ebb80 (facegen_misc.cpp)
struct skin_owner;
void __stdcall select_entity(rgl::mesh* m, skin_owner* o, unsigned flags);   // 0x4ea0c0 (a free stdcall function)
struct NodeA;
struct NodeVec {                                                    // widget_node: the gui manager's root list
    void push_back(NodeA* const& n);                                // 0x49b120
};
#endif
