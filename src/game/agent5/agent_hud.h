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
#include "../../rgl/mesh_x.h"        // mesh_x: set_bone_index 0x47b360 (the mesh class that defines it)
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
#include "../ui/mission_ui.h"
#include "../font.h"
#include "../../include/dialog_rec.h"
#include "../shadow_map.h"
#include "../terrain_b/emit.h"
#define vec cell_grid_vec                 // cell_grid.h's vec (no clear) under another name: the global vec below is the one whose clear() 0x41f4b0 is called
#include "../terrain_b/cell_grid.h"
#undef vec

using rgl::vector4;
using rgl::matrix;

// ---- callees declared under the name of the class or function that defines them -------------------------------------------
// The linker folds identical bodies and the modules that define them name the class after another of their users; a call binds to the
// defining declaration's symbol, so these are declared as that definition has them and the object is cast at the call. Each says why.
struct NodeA;
struct NodeVec { void push_back(NodeA* const& n); };               // 0x49b120 (widget_node.cpp): g_gui's "make this window the active root"; takes the address of a pointer to the window
struct ContainerW { void* construct(); };                          // 0x4931a0 (world_data.cpp): the window base constructor body, returns this
struct BlockOwner { void init(struct ContainerBase* c, int id); }; // 0x4dd920 (cls_map.cpp): MenuLayout::build
struct SkelBodyV;
void __cdecl setup_item_meshes(SkelBodyV* b, int kind, int troop, char* look, int f130, ItemRef* items, int id);   // 0x47cfe0 (mesh_scene.cpp)
// vec<mface>::clear 0x41f4b0 (entity.cpp; vec of rgl/mesh.h: the vector release shared by every record vector) is called out of line, through its member pointer
template <class T> struct vec { void clear(); };                   // the game's growable array (cell_grid.h, rgl/mesh.h, entity.h define it with their own members): only clear() 0x41f4b0 is called here
typedef void (vec<mface>::*VecClear)();
#define VEC_CLEAR(list) do { VecClear vc_ = &vec<mface>::clear; (((vec<mface>*)(list))->*vc_)(); } while (0)
struct Buffer { virtual ~Buffer(); };                              // 0x487c00 (slot.cpp: the destructor body the widget base uses)
struct FieldPanel { void update(); };                              // 0x48d470 (mission_ui2/mission_3.cpp): the list widget's layout
struct HudOuter { void scroll_refresh(vector4* v); };              // 0x498420 (world_data/mesh_hud.cpp)
struct PartyModel { void set_fade(const float* v); };              // 0x4b1800 (party2/party_2.cpp)
struct UiD { void set_value(float v); };                           // 0x48baa0 (ui/ui_widgets_c.cpp)
struct SoundW { void stop_all(); };                                // 0x494310 (world_data.cpp)
// level (trace_ray 0x439d60, query_segment 0x439870): terrain_b/cell_grid.h
float __cdecl time_of_day();                                       // 0x4356e0 (agent.cpp)
pos* __cdecl identity_pos(pos* out);                               // 0x40cd00 (shapes.cpp; rgl::identity_pos(matrix*) of the scene headers is the same function)
rgl::mat3* __cdecl identity3(rgl::mat3* out);                      // 0x40cd60 (shapes.cpp)
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
extern GuiManager g_gui;                                              // 0x5d7080
extern char g_gui_dirty;                                          // 0x5d8788 (set after a screen was built)
extern char g_hud_flag;                                           // 0x5e212c (set to 1 by 0x4f7f00)
extern char g_scene_dirty;                                          // 0x5e2938 (set with 5% probability by 0x4f7f00)
extern int g_pending_action;                                            // 0x5a9f18 (set to -1)


extern __int64 g_ticks[];                                         // 0x5e2948 clock table (1e-5 s ticks)
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
extern int g_cam_version;                                         // 0xa17118
extern int g_cam_down;                                            // 0xa17518
extern int g_cam_left;                                            // 0xa17150
extern int g_cam_right;                                           // 0xa1714c
extern int g_cam_in;                                              // 0xa17154
extern int g_cam_out;                                             // 0xa17158
extern int g_cam_roll;                                            // 0xa1718c
extern int g_cam_lock;                                            // 0xa17148
extern int g_cam_lock2;                                           // 0xa17348
extern int g_cam_sens;                                            // 0x5a9fdc

extern float g_screen_a;                                          // 0x5d75dc
extern float g_screen_b;                                          // 0x5d75d8
float __cdecl fmax_ref(const float* a, const float* b);           // 0x4324d0 (the larger of the two)
struct widget;                                                    // char_sheet/quest_log.h: the window widget the scroll helpers take and return (a MissionUi)
widget* __cdecl make_scroll_track(widget* parent, float a, float b, float c, float d);   // 0x4ee910 (cdecl)
widget* __cdecl make_quest_list(widget* content, const float* a, const float* b, int track);   // 0x4eea90 (cdecl; track is the scroll track widget)
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
    void* rec;                                                    // +0x6748 (non-null = scripted camera)
    matrix cur;                                                   // +0x674c rows as stored: f = -forward
    matrix sway;                                                  // +0x678c blend target
    float zoom_cur;                                               // +0x67cc
    float zoom_to;                                                // +0x67d0
    float value;                                                  // +0x67d4 blend duration (seconds), 0 = none
    int clock;                                                    // +0x67d8 tick clock index
    int pad67dc;
    __int64 start;                                                // +0x67e0 blend start tick
    int agent;                                                    // +0x67e8 attached agent id, -1 = none
    int attached;                                                 // +0x67ec 1 = frame is relative to the agent's body
    int num;                                                      // +0x67f0 smoothing strength
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
struct DustTerrain {                                            // the mission object's dust-grid pass (src/game/char_sheet/dust.h declares the same function on its own view of the object)
    void update_particles(const matrix* view);                    // 0x4f3e10 (ret 4)
};
extern DustTerrain g_terrain;                                 // 0xa76630 (the mission object itself; g_mission points to it)
#include "../../include/sound_mgr.h"                             // SoundMgr (play_slot 0x4428a0, stop_all 0x494310)
#include "../../rgl/resource_manager.h"
#include "../../include/renderer.h"
#include "../skill_cap.h"
extern SoundMgr g_sound_mgr;                                          // 0x5aa128
extern int g_listener_off;                                        // 0x5a9fd8 (nonzero: the sound listener does not follow the camera)
extern matrix g_listener;                                         // 0x5aa138 (s, f, u rows and the position o at 0x5aa168)

extern int g_use_shaders;                                      // 0x5e28fc
extern ShadowMap g_shadow_map;                                        // 0xa70248
extern int g_flag_a17108, g_flag_a17148, g_flag_a17348;           // 0xa17108, 0xa17148, 0xa17348 (key states that force the sun to full size)
extern int g_screen_w;                                            // 0x5d75c8
extern int g_screen_h;                                            // 0x5d75cc
extern float g_scale_a;                                           // 0x5aa08c
extern float g_scale_b;                                           // 0x5aa090

int __cdecl clamp_index(const int* v, const int* lo, const int* hi);   // 0x432510 (max(lo, min(v, hi - 1)))
int __stdcall FSOUND_3D_Listener_SetAttributes(float* pos, float* vel, float fx, float fy, float fz, float ux, float uy, float uz);   // 0x5342dc (_FSOUND_3D_Listener_SetAttributes@32)

void __cdecl mb_warning(const char* fmt, ...);                    // 0x42cb80 (logs a warning)
rgl::mesh* __cdecl make_item_mesh(const ItemRef* ref, const float* scale);   // 0x4fba40 (the display mesh of an item, scaled to fit *scale)

extern char* g_menus;                                         // 0x5e29b0 (the game menu records, 0x60 bytes each: +0x20 flags, +0x24 colour, +0x28 background mesh name, +0x38 condition block)
struct TableauTbl {                                            // the object table at 0xa75f40 (world_data/tableau.h: TableauTbl)
    rgl::mesh* make_mesh_from(const int* index, const void* from);   // 0x4991d0 (ret 8)
};
extern TableauTbl g_tableaus;                                    // 0xa75f40
struct MenuLayout { char pad[1]; };                               // the part of the game screen at +0x30; its build (0x4dd920, ret 8) is BlockOwner::init above

// ---- the widget slot view -----------------------------------------------------------------------------------------------
// A pure interface laid over MissionUi's family (vtable 0x591c80 and its derived tables): the slots the HUD builders call, by
// the name the call site shows (mission_ui.h names other slots of the same table with other signatures, as DrawObj does).
// Slot offsets in comments.
struct HudUi {
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14();
    virtual void v18(); virtual void v1c(); virtual void v20(); virtual void v24(); virtual void v28();
    virtual void select(int on);                                  // +0x2c
    virtual void v30(); virtual void v34(); virtual void v38(); virtual void v3c(); virtual void v40(); virtual void v44();
    virtual void flag_cmd(int flags);                             // +0x48
    virtual void v4c(); virtual void v50(); virtual void v54();
    virtual void set_size(float* v);                              // +0x58
    virtual void v5c(); virtual void v60();
    virtual void set_pos(float* v);                               // +0x64
    virtual void v68();
    virtual void set_inset(float* v);                             // +0x6c
    virtual void v70();
    virtual void set_int(int* v);                                 // +0x74
    virtual void v78();
    virtual void add_image_a(int layer, rgl::mesh* m);            // +0x7c
    virtual void add_image(int layer, rgl::mesh* m);              // +0x80
    virtual void v84(); virtual void v88(); virtual void v8c(); virtual void v90(); virtual void v94();
    virtual void set_color98(unsigned* c);                        // +0x98 (ret 4)
    virtual void set_color(unsigned* c);                          // +0x9c
    virtual void va0();
    virtual void set_text(float* out, tstring* text);             // +0xa4
    virtual void va8(); virtual void vac();
    virtual void hide();                                          // +0xb0
    virtual void show();                                          // +0xb4
    virtual void refresh();                                       // +0xb8
    virtual void vbc(); virtual void vc0(); virtual void vc4();
    virtual void add_child(HudUi* c);                             // +0xc8
};
#define UI(p) ((HudUi*)(p))

// ---- widgets: classes of this region ----------------------------------------------------------------------------------
struct ChildVec {                                                 // a widget list: VC7.1 vector layout (alloc slot, first, last, end), push_back as the callers inline it
    int alloc;
    MissionUi** first;
    MissionUi** last;
    MissionUi** end;
    void push_back(MissionUi* const& v)
    {
        if (first != 0 && (unsigned)(last - first) < (unsigned)(end - first)) {
            *last = v;
            ++last;
        } else
            ((VecOps<SceneObject*>*)this)->insert_n(std::vector<SceneObject*>::iterator((SceneObject**)last), 1, (SceneObject* const&)v);   // 0x4927d0 (world_data.cpp: std::vector<SceneObject*>::_Insert_n; the children are SceneObject pointers there)
    }
};
struct GroupWidget : MissionUi {                                  // vtable 0x5924a0, size 0x188: a container of children (party_window.h's UiFrame)
    ChildVec kids;                                                // +0x178
    GroupWidget() { *(void**)this = (void*)vtbl_5924a0; kids.first = 0; kids.last = 0; kids.end = 0; }
};
struct TextGroupWidget : GroupWidget {                            // the same 0x188-byte container with vtable 0x597050 (overlay.h's TextBox)
    TextGroupWidget() { *(void**)this = (void*)vtbl_597050; }
};
struct WindowBase : MissionUi {                                   // the 0x188-byte window base (party_window.h's UiWindow); its constructor body 0x4931a0 is ContainerW::construct
    ChildVec kids;                                                // +0x178 (first +0x17c)
private:
    WindowBase();                                                 // (never defined: windows are built by new_frame_widget)
};
struct FrameWidget : WindowBase {                                 // vtable 0x592848, size 0x190
    char f188;                                                    // +0x188 (cleared by the constructor)
    char pad189[7];
};
// new FrameWidget: the allocation, then the base constructor body 0x4931a0 (ContainerW::construct, which runs MissionUi's 0x48ae20 itself), the vptr and f188
inline FrameWidget* new_frame_widget()
{
    void* mem = operator new(sizeof(FrameWidget));
    if (mem == 0)
        return 0;
    ((ContainerW*)mem)->construct();
    *(void**)mem = (void*)vtbl_592848;
    ((FrameWidget*)mem)->f188 = 0;
    return (FrameWidget*)mem;
}
struct FlagWidget : MissionUi {                                   // vtable 0x592140 (ui_widgets.h's UiD: its constructor 0x48b890 is out of line there, here it is inlined), size 0x188
    int f178;                                                     // +0x178 (0x4fbfa0)
    float f17c;                                                   // +0x17c
    int f180, f184;                                               // +0x180
    FlagWidget() { *(void**)this = (void*)vtbl_UiD; f178 = 0; f17c = 0.1f; v168 = 1.0f; f180 = 0; f184 = 0; }
    void set_value(float x);                                      // 0x48baa0
};
struct ToggleWidget : MissionUi {                                 // vtable 0x591d58 (ui_widgets.h's UiA, constructor inlined), size 0x188
    int f178;                                                     // +0x178 (0x4fbfa0)
    float f17c;                                                   // +0x17c
    char f180;                                                    // +0x180
    char pad181[3];
    int f184;                                                     // +0x184
    ToggleWidget() { f178 = 0; b121 = 0; *(void**)this = (void*)vtbl_591d58; f180 = 0; f184 = 0; }
};
namespace c49deb0 {                                               // party2/camera.cpp names the class of the constructor 0x49deb0 Camera in this namespace
struct Camera : MissionUi {                                       // the scrolling list (constructor 0x49deb0), size 0x4a8; layout 0x48d470 is FieldPanel::update, scroll_refresh 0x498420 (ret 4) is HudOuter's
    char pad178[0x484 - 0x178];
    MissionUi* flag;                                              // +0x484 (0x4fbfa0)
    char pad488[0x490 - 0x488];
    MissionUi* bar;                                               // +0x490
    char pad494[0x4a8 - 0x494];
    Camera();                                                     // 0x49deb0
};
}
typedef c49deb0::Camera ListWidget;
struct NameWidget : MissionUi {                                   // ctor 0x4ed630, vptr 0x598698, size 0x358
    void* mission_scene;                                          // +0x178 (Mission::scene, stored by 0x4f7f00)
    char pad17c[0x1ac - 0x17c];
    char frustum[0x120];                                          // +0x1ac the copied camera frustum (0x4f7f00, 0x4f8c80)
    char pad2cc[0x354 - 0x2cc];
    char f354;                                                    // +0x354 (set when 0x5aa034 != 0)
    char pad355[3];
private:
    NameWidget();                                                 // (never defined: the constructor 0x4ed630 is the fastcall function below)
};
struct MapWidget;                                                 // screen_camera.cpp's name for the class of the constructor 0x4ed630
MapWidget* __fastcall ctor_4ed630(MapWidget* p);                  // 0x4ed630 (ecx = the object; returns it)


// ---- the mission HUD screen ---------------------------------------------------------------------------------------------
// The screen object of the mission view (created and ticked by 0x5261e0 / 0x5284a0 of other objects): refresh() 0x4f7f00 builds its
// widgets, update_camera() 0x4f8c80 moves the camera, update_status() 0x4fb350 keeps the status bars. One object, three
// slices (the slices called it MissionObj, Camera and StatusWidget).
struct MissionHud {
    char pad0[0x2c];
    FrameWidget* frame;                                           // +0x2c (0x4f7f00; the screen's root widget)
    char pad30[0x60 - 0x30];
    NameWidget* name_widget;                                      // +0x60 (0x4f7f00; 0x4f8c80 copies the frustum into it)
    char pad64[4];
    MissionUi* crosshair[3];                                      // +0x68 (0x4f7f00: sized 0.5 x 0.375, meshes at +0xa0)
    MissionUi* arrow[4];                                          // +0x74 (0x4f7f00: meshes at +0xac)
    int f84[7];                                                   // +0x84 (cleared by 0x4f7f00)
    rgl::mesh* crosshair_mesh[3];                                 // +0xa0 (0x4f7f00)
    rgl::mesh* arrow_mesh[4];                                     // +0xac
    MissionUi* ui_font;                                           // +0xbc (0x4f7f00: "ui_font" text, 0x4fb350 shield text cur)
    MissionUi* ui_font_bg;                                        // +0xc0 (0x4f7f00; 0x4fb350 shield text max)
    GroupWidget* status_group;                                    // +0xc4 (0x4f7f00: the status background, vtable 0x5924a0; 0x4fb350 hides / shows it)
    MissionUi* shield_icon;                                       // +0xc8 (0x4f7f00; 0x4fb350 ammo icon)
    MissionUi* status_bar;                                        // +0xcc (0x4f7f00; 0x4fb350 shield piece)
    MissionUi* health_bar;                                        // +0xd0 (0x4f7f00)
    rgl::mesh* shield_mesh;                                       // +0xd4 (0x4f7f00 slot 2 of the status bar; 0x4fb350 shield piece mesh)
    rgl::mesh* shadow_mesh;                                       // +0xd8 (0x4fb350)
    rgl::mesh* health_mesh;                                       // +0xdc (0x4f7f00 "status_health_bar"; 0x4fb350)
    int shown_health;                                             // +0xe0 (0x4f7f00 -1; 0x4fb350 the health shown)
    int shield_max;                                               // +0xe4 (0x4f7f00 -2)
    int shield_cur;                                               // +0xe8
    int shield_aux;                                               // +0xec
    int ammo_count;                                               // +0xf0
    int ammo_stage;                                               // +0xf4
    rgl::frustum fr;                                              // +0xf8 (0x4f7f00 sets the near / far pair and the four edges; 0x4f8c80 refreshes it every tick)
    vector4 head;                                                 // +0x218 (0x4f8c80: the agent's head position plus the camera height)
    posm k;                                                       // +0x228 the camera frame (0x4f8c80; s, f, u, o rows)
    float yaw;                                                    // +0x268 (0x4f7f00, 0x4f8c80)
    float pitch;                                                  // +0x26c (0x4f8c80)
    int f270, f274;                                               // +0x270 (cleared by 0x4f7f00)
    float f278;                                                   // +0x278 (0x4f8c80)
    float f27c;                                                   // +0x27c (0x4f7f00 0 / 0.1; 0x4f8c80)
    float f280;                                                   // +0x280 (0x4f7f00 1.65 / 2.4; 0x4f8c80)
    float f284;                                                   // +0x284 (0x4f8c80)
    float zoom;                                                   // +0x288 (0x4f8c80)
    vector4 lag;                                                  // +0x28c (0x4f8c80: the smoothed head bone position)
    char pad29c[0x2a8 - 0x29c];
    int clock_a;                                                  // +0x2a8 (tick clock index, 0x4f7f00)
    char pad2ac[0x2b0 - 0x2ac];
    __int64 stamp_a;                                              // +0x2b0 (10 s before clock_a's time)
    int clock_b;                                                  // +0x2b8
    char pad2bc[0x2c0 - 0x2bc];
    __int64 stamp_b;                                              // +0x2c0
    int f2c8;                                                     // +0x2c8
    char pad2cc[0x2d4 - 0x2cc];
    int f2d4, f2d8, f2dc;                                         // +0x2d4
    char pad2e0[0x2e4 - 0x2e0];
    int f2e4, f2e8;                                               // +0x2e4
    float f2ec;                                                   // +0x2ec (0x4f8c80: the smoothed camera pull-in distance)
    bool f2f0;                                                    // +0x2f0
    bool refresh_slots;                                           // +0x2f1 (0x4f7f00: the selected agent's slots need re-initialising)
    bool grow_health;                                             // +0x2f2 (0x4f7f00: the status block height must grow)
    char pad2f3;
    int status_height;                                            // +0x2f4
    char pad2f8[0x2fc - 0x2f8];
    int f2fc;                                                     // +0x2fc
    float f300;                                                   // +0x300 (0x4f8c80: the free camera's speed)

    void refresh();                                               // 0x4f7f00
    void update_camera(float dt);                                 // 0x4f8c80 (ret 4)
    void update_status();                                         // 0x4fb350
    void update_equipment();                                      // 0x4f8b40 (this unused)
};

// ---- the game menu screen -----------------------------------------------------------------------------------------------
struct ScreenOps {                                                // the screen objects' vtable (slot 9 begins the screen)
    virtual void v00(); virtual void v04(); virtual void v08(); virtual void v0c(); virtual void v10(); virtual void v14();
    virtual void v18(); virtual void v1c(); virtual void v20();
    virtual void begin();                                         // +0x24
};
struct GameScreen {                                               // the game menu window (built by 0x4fbfa0)
    char pad0[0x2c];
    FrameWidget* frame;                                           // +0x2c
    char pad30[0x40 - 0x30];                                      // +0x30 the MenuLayout part
    int f40;                                                      // +0x40
    char pad44[0x60 - 0x44];
    MissionUi* kids[16];                                          // +0x60 the sixteen option buttons
    int fa0;                                                      // +0xa0
    char pada4[0xe4 - 0xa4];
    MissionUi* panel;                                             // +0xe4
    ListWidget* tree;                                             // +0xe8
    MissionUi* slider;                                            // +0xec
    char padf0[4];
    MissionUi* fill;                                              // +0xf4
    void build();                                                 // 0x4fbfa0
};

// ---- the game-log text widget and the map-text row renderer ------------------------------------------------------------------
extern Font* g_font;                                              // 0x5e2934
struct IntList {                                                  // a VC7.1 vector<int> (alloc slot, first, last, end) as the layout code fills it
    int alloc;
    int* first;
    int* last;
    int* end;
    __declspec(noinline) bool buy(unsigned n);                                         // 0x4fd490 (ret 4: the allocation of the vector<int>'s first block)
    void xlen();                                                  // 0x43e2a0 (std::vector<int>::_Xlen: the original loads `this` in ECX; agent3/agent_combat.cpp defines it as the free function vec_xlen, so this row stays unresolved)
    void free_block() { if (first) operator delete(first); first = 0; last = 0; end = 0; }   // the inlined vector release of ~MWidget
};
// std::vector<int>::push_back called out of line (the original calls the instantiation 0x4fe0b0 where the library would expand it): through its member pointer
typedef void (std::vector<int>::*VecIntPushBack)(const int&);
#define VEC_INT_PUSH_BACK(list, val) do { VecIntPushBack pb_ = &std::vector<int>::push_back; (((std::vector<int>*)(list))->*pb_)(val); } while (0)
struct V4List {                                                   // a VC7.1 vector<vector4>
    int alloc;
    vector4* first;
    vector4* last;
    vector4* end;
    struct iterator { vector4* p; iterator() {} };
    void push_back(const vector4& v);                             // 0x4fdff0 (ret 4)
    __declspec(noinline) iterator insert(iterator where, const vector4& val);          // 0x4fe060 (ret 0xc: the iterator comes back through the hidden pointer)
    void free_block() { if (first) operator delete(first); first = 0; last = 0; end = 0; }
};
struct MapText { int a; float w; };                               // the 8-byte layout result (width, height)
// The game-log / map-text list widget: vtable 0x5995c0, size 0x1d0 (the slices called it LogWidget and MWidget). It derives from MissionUi in
// the original, but its destructor calls the base destructor body out of line (0x487c00) where a derived class of the canonical MissionUi would
// inline the implicit one (the name string's destructor), so the base part is kept as an opaque block that is constructed by 0x48ae20.
struct MWidget {
    void* vptr;
    char body[sizeof(MissionUi) - 4];                             // the MissionUi members (0x4..0x178)
    MissionUi* U() { return (MissionUi*)this; }
    V4List rects_a;                                               // +0x178 tag-open rectangles (0x4fd1d0 scans them; 0x4fd700 fills them)
    V4List rects_b;                                               // +0x188
    IntList list_c;                                               // +0x198 (0x4fd490 builds it; 0x4fd3d0 clears it)
    IntList list_d;                                               // +0x1a8
    IntList list_e;                                               // +0x1b8 the tag index of every rectangle of rects_a
    int cur;                                                      // +0x1c8 selected spot (-1 = none)
    int sel;                                                      // +0x1cc
    MWidget();                                                    // 0x4fd150
    char on_event(unsigned ev);                                   // 0x4fd1d0
    void* destroy(unsigned flags);                                // 0x4fd3b0
    ~MWidget();                                                   // 0x4fd3d0
    MapText text_rect();                                          // 0x4fd4f0
};
extern const float g_cursor_x;                                    // 0x5d7094 (the pointer position the widgets hit-test against)
extern const float g_cursor_y;                                    // 0x5d7098
struct ParamEntry {                                               // 0x2c-byte record of the parameter table at 0xa44240 (the value text at +0x20)
    tstring name;
    char pad10[0x20 - 0x10];
    char* value;                                                  // +0x20 (0xa4425c + 4 + index * 0x2c)
    char pad24[0x2c - 0x24];
};
extern ParamEntry g_game_vars[];                                     // 0xa44240
extern int g_param_count;                                         // 0xa70240
extern tstring* g_msg_begin;                                      // 0x5e23c4 (the recent-messages ring: 0x10-byte tstrings)
extern tstring* g_msg_end;                                        // 0x5e23c8
#include "../str_table.h"                                          // StrTable: lookup 0x43a1e0 (ret 4)
extern StrTable g_strtable;                                       // 0x5e2a18
struct UiAInline : MissionUi {};                                  // cls_map/progress_bar.cpp's name for the widget make_short_button returns
UiAInline* __cdecl make_short_button(MissionUi* frame, const tstring* text, float x, float y, float h);   // 0x4e5570
struct TStringArray {                                             // an array of tstrings allocated with new[] (the count sits in front of the first element)
    __declspec(noinline) void* destroy(unsigned flags);           // 0x4fd0e0 (this = the first element; called, not inlined, by 0x4fc820)
};
struct LogScreen {                                                // the game-log window (built by 0x4fc820)
    char pad0[4];
    int show_log;                                                 // +4 (nonzero: the full game log; 0 = the recent messages: 0x4fc810 passes it on)
    char pad8[0x2c - 8];
    FrameWidget* frame;                                           // +0x2c
    MWidget* log;                                                 // +0x30
    MissionUi* bar;                                               // +0x34
    MissionUi* slider;                                            // +0x38
    MissionUi* tab_return;                                        // +0x3c
    MissionUi* tab_log;                                           // +0x40
    MissionUi* tab_recent;                                        // +0x44
    MissionUi* tab_troops;                                        // +0x48
    MissionUi* tab_locations;                                     // +0x4c
    MissionUi* tab_factions;                                      // +0x50
    MissionUi* tab_quests;                                        // +0x54
    void build(bool show);                                        // 0x4fc820 (ret 4)
    void build_thunk();                                           // 0x4fc810 (builds the log screen from the flag at +4)
};

#endif // AGENT5_HUD_H
