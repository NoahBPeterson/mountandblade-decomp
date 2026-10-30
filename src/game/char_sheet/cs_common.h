// cs_common.h: classes and functions of other translation units that the character-sheet / face-generator modules call, declared
// under the defining class's name with only the members they call (the defining headers do not go with these modules' views).
// One declaration per line, each with the address of the function that the definition (named in the comment) provides: the
// equivalence harness resolves a callee from the '// 0x...' comment on its declaration line.
#ifndef CHAR_SHEET_CS_COMMON_H
#define CHAR_SHEET_CS_COMMON_H
struct tstring;                                                     // (complete in the includer: game/tstring.h, or terrain_page.h's own view)

struct UiD {                                                        // ui/ui_widgets.h: the progress bar / slider
    UiD();                                                          // 0x48b890
    float value();                                                  // 0x48b930
    void set_value(float x);                                        // 0x48baa0
};
struct WindowMgrW {                                                 // world_data/world_data.h; the main menu object 0xa73980
    void open(int window);                                          // 0x494260
};
struct FaceRace {                                                   // terrain_b
    void relax(int index, float* values);                           // 0x4735e0
};
#ifndef CS_REAL_FACEGEN                                             // (facegen_init.h / facegen_misc.h define the whole class)
struct FaceGen {                                                    // facegen_misc.cpp
    void update_view(int unused);                                   // 0x4eb270
    void refresh();                                                 // 0x4eb3c0
};
#endif
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

// the terrain object (g_terrain 0xa76630; terrain_a): the members the char_sheet views call on it
struct particle_list;
struct HexWords {                                                   // terrain bit words at 0xa76630
    void parse(const tstring& s);                                   // 0x4658c0
};
struct Terrain {
    void set_field_a(const float* p);                               // 0x464a60
    void set_field_b(const float* p);                               // 0x464ac0
    void reset();                                                   // 0x4647e0
    rgl::vector4* ground_color(rgl::vector4* out, const float* pos);   // 0x464d50 (ret 8)
    void light_particle(particle_list* list, int idx, bool trace);  // 0x471b30 (ret 0xc)
};

// ---- widgets: the constructors that the char_sheet views (Widget / WidgetObj / Overlay / TextBox) stand for -------------------
// ui_widget.h's Widget() and overlay.h's Overlay() are declarations of the constructors that the definitions name MissionUi::MissionUi
// (0x48ae20) and ContainerW::construct (0x4931a0, which stores the container vptr 0x5924a0, clears +0x17c..+0x184 and returns this). An
// object is built as the original does: operator new, that constructor in place, then the final vptr when the kind has its own
// (Overlay 0x592848, TextBox 0x597050).
#include "../../include/image_addrs.h"
#include <new>
#include "../world_data/raw_place.h"
namespace rgl { struct mesh; }                                     // (the set_track pointer argument; ui/mission_ui.h has the full class)
struct MissionUi {                                                  // ui/mission_ui.h
    MissionUi();                                                    // 0x48ae20
    void render(rgl::scene* s, float* v, int a, int b);             // 0x488720
    void set_track(int i, rgl::mesh* p);                            // 0x488ad0 (vtable slot +0x80)
    void set_fog(const float* ref, const float* lo, const float* hi);   // 0x488cd0 (facegen_win 0x4f1460 calls it on the 0x344-byte preview scene)
};
struct ContainerW {                                                 // world_data/world_data.h
    void* construct();                                              // 0x4931a0
};
extern char vtbl_597050[];                                          // 0x597050 (TextBox, FaceBoard)
extern char vtbl_592848[];                                          // 0x592848 (Overlay; the caller also clears the byte at +0x188)
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
struct UiAInline;
UiAInline* __cdecl make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4e5570 (progress_bar)
MissionUi* __cdecl make_hint(ContainerBase* parent, const tstring& text, float a, float b, float c, float d);   // 0x4dada0 (cls_map)
struct SoundW {                                                     // world_data/world_data.h: the campaign UI root 0x5aa128 is one
    void stop_all();                                                // 0x494310
};
// the particle / shadow system g_psys (0xa70248)
struct Wind;
struct CellGrid {                                                   // the scene World (scene_world.h; mission)
    CellGrid(const float* lo, const float* hi, const int* a, const int* b, const int* c);   // 0x478ad0
    Wind* spawn_wind();                                             // 0x478f30
};
struct ShadowMap {
    void attach_camera(CellGrid* c);                                // 0x4711e0
    void extend(const rgl::vector4* p);                             // 0x468c20
    void fit_bounds();                                              // 0x463cc0
};
struct ShadowMapP {
    void render();                                                  // 0x47fef0
};
UiAInline* __cdecl new_medium_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4dce70 (cls_map)
UiAInline* __cdecl new_button(ContainerBase* o, const tstring& text, float px, float py, float sx, float sy);   // 0x4db810 (cls_map)
UiAInline* new_sized_button(MissionUi* parent, const tstring& text, float a, float b, float c,
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
    if (c.first != 0 && (unsigned)(c.last - c.first) < (unsigned)(c.end - c.first))
        *c.last++ = v;
    else
        ((SceneObjVec*)&c)->_Insert_n(std::vector<SceneObject*>::iterator((SceneObject**)c.last), 1, (SceneObject* const&)v);
}

namespace c49deb0 {
struct Camera {                                                     // the editor / quest list widget
    Camera();                                                       // 0x49deb0
};
}
struct WidgetA;
struct MapWidget;
MapWidget* __fastcall ctor_4ed630(MapWidget* self);                 // 0x4ed630 (the 3D view widget's constructor; MapWidget() of map_widget.h names it)
WidgetA* __cdecl new_key_label(void* parent, const tstring* name, const tstring* text);   // 0x4f5d20 (editor_ui)
void __cdecl label_refresh(MissionUi* w, tstring name);             // 0x4e4c70 (progress_bar: sets a freshly built widget up from its ini keys)
struct WidgetB;
struct WidgetC;
WidgetB* __cdecl make_scrollbar(void* parent, const tstring* name); // 0x4f6160 (editor_ui)
WidgetB* __cdecl make_slider(void* parent, const tstring* name);    // 0x4f5fb0 (editor_ui)
void* __cdecl make_left_arrow(void* parent, int a2, int a3, float x, int a5, float z);    // 0x4f5df0 (editor_ui)
void* __cdecl make_right_arrow(void* parent, int a2, int a3, float x, int a5, float z);   // 0x4f5ed0 (editor_ui)
WidgetC* __cdecl make_editor_list(void* parent, const tstring* name, int scroll);   // 0x4f62f0 (editor_ui)
struct MissionPanel {                                               // ui/mission_ui.h
    void draw();                                                    // 0x488d50
};
struct gui_obj;
struct gui_manager {                                                // strat/gui_manager.h (the object at 0x5d7080)
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
