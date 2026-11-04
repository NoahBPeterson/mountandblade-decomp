// agent_hud.h: the classes and callee declarations of agent_hud.cpp (0x4f7d70..0x4fe860): the mission's agent
// (re)initialisation, the mission HUD (the screen object that owns the status frame, the name overlay, the crosshairs and
// the free / scripted camera), the sun and flare update of the mission object, the item mesh builder, the game-log and
// game HUD builders with the map-text list widget and the std::vector instantiations kept from this object.
// One definition per class: the shared headers of src/game / src/include / src/rgl are included, what exists nowhere
// (the HUD screen object, the widget slot view, the three small widget classes) is defined here. Every offset cites the
// function that proves it. Names provisional.
#ifndef AGENT5_HUD_H
#define AGENT5_HUD_H
#include "../../include/image_addrs.h"
#include <stddef.h>
#include <math.h>
#include <new>
#include <vector>
#include "../../rgl/rgl_math.h"
#include "../../rgl/rgl_string.h"
#include "../../rgl/mesh.h"
#include "../../rgl/mesh.h"        // rgl::mesh: set_bone_index 0x47b360 (the mesh class that defines it)
#include "../../rgl/core/resmgr.h"
#include "../tstring.h"
#include "../item_ref.h"
#include "../../include/item_kind.h"
#include "../mission.h"
#include "../agent_obj.h"
#include "../mode_rec.h"
#include "../../include/game_data.h"
#include "../../include/fx.h"
#include "../../include/shape.h"
#include "../../include/frustum.h"
#include "../party2/camera_class.h"      // Camera: the scrolling list (constructor 0x49deb0; layout 0x48d470 is FieldPanel::update, scroll_refresh 0x498420 (ret 4) is HudOuter's)
#include "../ui/ui_widgets.h"
#include "../font.h"
#include "../../include/dialog_rec.h"
#include "../shadow_map.h"
#include "../terrain_b/emit.h"
#define vec cell_grid_vec                 // cell_grid.h's vec (no clear) under another name: the global vec below is the one whose clear() 0x41f4b0 is called
#include "../terrain_b/cell_grid.h"
#include "../key_names.h"
#undef vec

using rgl::vector4;
using rgl::matrix;

// ---- callees declared under the name of the class or function that defines them -------------------------------------------
// The linker folds identical bodies and the modules that define them name the class after another of their users; a call binds to the
// defining declaration's symbol, so these are declared as that definition has them and the object is cast at the call. Each says why.
struct NodeA;
struct NodeVec { void push_back(NodeA* const& n); };               // 0x49b120 (widget_node.cpp): g_gui's "make this window the active root"; takes the address of a pointer to the window
struct BlockOwner { void init(struct ContainerBase* c, int id); }; // 0x4dd920 (cls_map.cpp): MenuLayout::build
#include "../mesh_slot/mesh_build.h"        // setup_item_meshes (0x47cfe0)
// vec<mface>::clear 0x41f4b0 (entity.cpp; vec of rgl/mesh.h: the vector release shared by every record vector) is called out of line, through its member pointer
template <class T> struct vec { void clear(); };                   // the game's growable array (cell_grid.h, rgl/mesh.h, entity.h define it with their own members): only clear() 0x41f4b0 is called here
typedef void (vec<mface>::*VecClear)();
#define VEC_CLEAR(list) do { VecClear vc_ = &vec<mface>::clear; (((vec<mface>*)(list))->*vc_)(); } while (0)
struct Buffer { virtual ~Buffer(); };                              // 0x487c00 (slot.cpp: the destructor body the widget base uses)
struct FieldPanel { void update(); };                              // 0x48d470 (mission_ui2/mission_3.cpp): the list widget's layout
struct HudOuter { void scroll_refresh(vector4* v); };              // 0x498420 (world_data/mesh_hud.cpp)
struct PartyModel { void set_fade(const float* v); };              // 0x4b1800 (party2/party_2.cpp)
// level (trace_ray 0x439d60, query_segment 0x439870): terrain_b/cell_grid.h
float __cdecl time_of_day();                                       // 0x4356e0 (agent.cpp)
namespace rgl { vector4 unpack_color(const unsigned* argb); }      // 0x40e4e0 (shapes.cpp: the vector comes back through the hidden pointer; rgl_math.h declares an out-parameter spelling)
template <class T> void __cdecl uninit_fill_n(T* first, unsigned n, const T& val, std::allocator<T>& al, std::_Nonscalar_ptr_iterator_tag tag);   // 0x40df60 (shapes.cpp)
// the protected std::vector<T>::_Insert_n of the vectors the widget code fills by hand (VC7.1 layout: allocator word, first, last, end)
template <class T> struct VecOps : std::vector<T> {
    void insert_n(iterator where, size_type n, const T& val) { _Insert_n(where, n, val); }
};
struct SceneObject;

// ---- the mission object's agent (re)initialisation callees ------------------------------------------------------------
struct AgentEq;                                                   // terrain_a/misc.h (not included: it pulls the other rgl world)
void __stdcall AgentEq_pick_slots(AgentEq* a);                    // 0x46b2d0 (ret 4)
// 0x482240 (cdecl, 12 arguments): the agent's appearance lods; 0x47cfe0 (cdecl, 7 arguments): the equipment bodies
void __cdecl build_appearance(AgentBody* b, int kind, int troop, int f94, ItemRef* f8c, int* look, int* f240, int* f194,
                              int* f1a4, int* f1a8, int* items, int id);       // 0x482240
// 0x47cfe0 is setup_item_meshes (below): the equipment bodies

// ---- globals and engine objects the HUD code uses ---------------------------------------------------------------------
#include "../gui_mgr.h"                                         // GuiManager: the gui manager at 0x5d7080
#include "../../include/render_flags.h"
#include "../ui_globals.h"      // UI / core globals
extern GuiManager g_gui;                                              // 0x5d7080
extern char g_hud_flag;                                           // 0x5e212c (set to 1 by 0x4f7f00)
extern char g_scene_dirty;                                          // 0x5e2938 (set with 5% probability by 0x4f7f00)
extern int g_pending_action;                                            // 0x5a9f18 (set to -1)


struct UiVec {                                                    // a widget list at game +0x1a1b8
    void resize(int a, int b);                                     // 0x43f310 (ret 8)
};

// the game object's fields that only this region reads (g_game is the GameData of game_data.h; the offsets the shared header does not
// name are reached by offset, each with the function that proves it):
//   +0x139e4 spectate flag (0x443aa0, 0x4f8b40)        +0x13a00 troop table (0x4f7f00)
//   +0x1a090 free / script camera flag (0x4f7f00, 0x4f8c80)   +0x1a1b4 reset to -1 (0x4f7f00)   +0x1a1b8 widget list (0x4f7f00)
//   +0x1a1cc reset to 0 (0x4f7f00)   +0x1a20c status mode (0x4fb350)
#define GAME_AT(T, off) (*(T*)((char*)g_game + (off)))

// the agent of an id as the callers inline it: the page table read as a raw pointer array (0x4f7f00, 0x4f8b40)
static __forceinline Agent* agent_raw(unsigned id)
{
    return (Agent*)(((char**)g_mission->agents.pages._Myfirst)[id >> 4] + (id & 0xf) * 0x5928);
}

// the same lookup through the vector's operator[] (0x4f8b40 compiles it so: the page slot is addressed, then indexed)
static __forceinline Agent* agent_by_id(unsigned id)
{
    return &g_mission->agents.pages[id >> 4][id & 0xf];
}

// ---- camera globals and engine objects the camera tick uses -------------------------------------------------------------
struct OriginPairs {                                              // the key table at 0xa73730 (ui/mission.h)
    bool is_down(int key);                                           // 0x48ac50 (ret 4)
};
extern OriginPairs g_game_keys;                                         // 0xa73730


extern float g_screen_a;                                          // 0x5d75dc
extern float g_screen_b;                                          // 0x5d75d8
float __cdecl fmax_ref(const float* a, const float* b);           // 0x4324d0 (the larger of the two)
#include "../ui/ui_widgets.h"                                  // UiD
#include "../party2/camera_class.h"                             // Camera (the list widget)
UiD* __cdecl make_scroll_track(ContainerBase* parent, float a, float b, float c, float d);   // 0x4ee910 (cdecl)
Camera* __cdecl make_quest_list(ContainerBase* content, const float* a, const float* b, UiD* track);   // 0x4eea90 (cdecl; track is the scroll track widget)
float __cdecl clamp(const float* x, const float* lo, const float* hi);   // 0x40e3d0

// the scene's segment query (level, above) and the skeleton's head bone, as the camera tick reaches them
struct Skeleton {                                                 // the skeleton at AgentBody+0x304: skeleton.h's Skeleton (that header cannot be included next to rgl/mesh.h, see MIGRATION.md), the members this module reads
    int nbones;                                                   // +0
    char* recs;                                                   // +4 (0x1dc-byte records: the matrix at +0x1140 is the head bone, +0x1170 its origin)
    char pad8[0x50 - 8];
    char ready;                                                   // +0x50 (0x4f8c80)
    void ensure_updated();                                        // 0x43e740
};

// the mission's scripted camera slot (Mission +0x6748 .. +0x67f4; mission.h names the same words f6748 / frame_a / frame_b / blend_*)
struct CamSlot {
    void* rec;                                                    // Mission+0x6748 (non-null = scripted camera)
    matrix cur;                                                   // Mission+0x674c rows as stored: f = -forward
    matrix sway;                                                  // Mission+0x678c blend target
    float zoom_cur;                                               // Mission+0x67cc
    float zoom_to;                                                // Mission+0x67d0
    float value;                                                  // Mission+0x67d4 blend duration (seconds), 0 = none
    int clock;                                                    // Mission+0x67d8 tick clock index
    int pad67dc;
    __int64 start;                                                // Mission+0x67e0 blend start tick
    int agent;                                                    // Mission+0x67e8 attached agent id, -1 = none
    int attached;                                                 // Mission+0x67ec 1 = frame is relative to the agent's body
    int num;                                                      // Mission+0x67f0 smoothing strength
};
#define CAMSLOT (*(CamSlot*)((char*)g_mission + 0x6748))

// ---- the sun / flare update (Mission::camera_to) -----------------------------------------------------------------------
struct FrameNode {                                                // a scene node whose frame sits at +0x58 (s, f, u, o rows; origin at +0x88): the sun and the flare of the mission
    char pad0[0x58];
    matrix pos;
};
struct SunBody {                                                  // an agent's scene body as the sun pass reaches it (AgentBody's vtable slot 8 and member 0x4b1800)
    virtual void s0(); virtual void s1(); virtual void s2(); virtual void s3(); virtual void s4(); virtual void s5(); virtual void s6();
    virtual void s7();
    virtual void set_intensity(const float* v);                   // +0x20 (ret 4)
    char pad[0x88 - 4];
    vector4 origin;                                               // +0x88 (the body frame's origin)
    void set_fade(const float* v);                                // 0x4b1800 (ret 4)
};
#include "../terrain_a/terrain.h"                          // Terrain, g_terrain (0xa76630): update_particles 0x4f3e10
#include "../../include/sound_mgr.h"                             // SoundMgr (play_slot 0x4428a0, stop_all 0x494310)
#include "../../rgl/resource_manager.h"
#include "../../include/renderer.h"
#include "../skill_cap.h"

extern int g_use_shaders;                                      // 0x5e28fc
extern ShadowMap g_shadow_map;                                        // 0xa70248
#include "../ui_globals.h"      // g_screen_w / g_screen_h (viewport size)



int __cdecl clamp_index(const int* v, const int* lo, const int* hi);   // 0x432510 (max(lo, min(v, hi - 1)))
#include "../../include/fmod_api.h"   // FMOD 3 (one declaration per entry point)

void __cdecl mb_warning(const char* fmt, ...);                    // 0x42cb80 (logs a warning)
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale);   // 0x4fba40 (the display mesh of an item, scaled to fit *scale)

struct TableauTbl {                                            // the object table at 0xa75f40 (world_data/tableau.h: TableauTbl)
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (ret 8)
};
extern TableauTbl g_tableaus;                                    // 0xa75f40
struct MenuLayout { char pad[1]; };                               // the part of the game screen at +0x30; its build (0x4dd920, ret 8) is BlockOwner::init above


// ---- widgets: classes of this region ----------------------------------------------------------------------------------
// the mission start menu's frame window: the NodeA container (vtable 0x592848, size 0x190) with its +0x188 byte cleared
inline NodeA* new_frame_widget()
{
    NodeA* w = new NodeA;
    w->f188 = 0;
    return w;
}
#include "../map_widget.h"                                      // MapWidget: the 3D view widget (ctor 0x4ed630, vptr 0x598698, size 0x358)


// ---- the mission HUD screen ---------------------------------------------------------------------------------------------
// The screen object of the mission view (created and ticked by 0x5261e0 / 0x5284a0 of other objects): refresh() 0x4f7f00 builds its
// widgets, update_camera() 0x4f8c80 moves the camera, update_status() 0x4fb350 keeps the status bars. One object, three
// slices (the slices called it MissionObj, Camera and StatusWidget).
// (the mission HUD screen is the record MissionBlocker of options_panel/mission_item_recs.h: refresh is its slot 2)
#include "../options_panel/mission_item_recs.h"

// ---- the game menu screen -----------------------------------------------------------------------------------------------
struct ScreenOps {                                                // the screen objects' vtable (slot 9 begins the screen)
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14();
    virtual void v18(); virtual void v1c(); virtual void v20();
    virtual void begin();                                         // +0x24
};
// (the game menu window is the record ItemA of options_panel/mission_item_recs.h: its slot 2 is build 0x4fbfa0)
#include "../options_panel/mission_item_recs.h"

// ---- the game-log text widget and the map-text row renderer ------------------------------------------------------------------
extern Font* g_font;                                              // 0x5e2934
#include "../ui/map_list.h"                                       // IntList, V4List, MapText, MWidget (the game-log list widget)
// std::vector<int>::push_back called out of line (the original calls the instantiation 0x4fe0b0 where the library would expand it): through its member pointer
typedef void (std::vector<int>::*VecIntPushBack)(const int&);
#define VEC_INT_PUSH_BACK(list, val) do { VecIntPushBack pb_ = &std::vector<int>::push_back; (((std::vector<int>*)(list))->*pb_)(val); } while (0)
#include "../ini_file.h"                                           // g_game_vars (0xa44240), ini_entry: the parameter table (the value text at +0x20)


#include "../str_table.h"                                          // StrTable: lookup 0x43a1e0 (ret 4)
UiA* __cdecl make_short_button(MissionUi* frame, const tstring* text, float x, float y, float h);   // 0x4e5570
struct TStringArray {                                             // an array of tstrings allocated with new[] (the count sits in front of the first element)
    __declspec(noinline) void* destroy(unsigned flags);           // 0x4fd0e0 (this = the first element; called, not inlined, by 0x4fc820)
};
#include "../screen_records.h"
typedef Rec58 LogScreen;                                          // the game-log window: the record of screen_records.h (built by 0x4fc820)

#endif // AGENT5_HUD_H
