// progress_bar_and_load_screens.h: the loading-screen progress bar widget (vtable 0x597f20, 0x198 bytes, constructor 0x4e3eb0) and the
// objects that use it: the meeting / loading window (src/game/app/winmain_and_menu_screens.cpp, 0x4e3780..0x4e3eb0) and the
// functions of progress_bar_and_load_screens.cpp (0x4e3eb0..0x4e87d0). The widget is a MissionUi (../game/ui/widgets/widget_base.h; base constructor
// 0x48ae20): its value is MissionUi::v168, its mesh slot tracks[2] (+0x110). ALL NAMES PROVISIONAL.
#ifndef GAME_CLS_MAP_PROGRESS_BAR_H
#define GAME_CLS_MAP_PROGRESS_BAR_H
#include "game/app/winmain_and_menu_screens.h"
#include "third_party/tinyxml/tinyxml.h"

struct progress_bar : MissionUi {               // vtable 0x597f20
    int idx;                                    // +0x178 index of the scaled component of the rect (new_progress_bar 0x4e3d50 clears it)
    float ratio;                                // +0x17c (new_progress_bar: width * 0.31)
    rgl::mesh* handle;                           // +0x180 the source mesh of the bar (a resource; cloned by update)
    unsigned char f184;                         // +0x184 dirty flag (the ctor sets it)
    unsigned char f185, f186, f187;             // +0x185
    rgl::vector4 rect;                          // +0x188 (1, 1, 1, 0 after the ctor)
    virtual void update_children();             // +0xc4, vtable slot 49 0x4e3fb0 (rebuilds the scaled mesh when dirty; called by render)
    progress_bar();                             // 0x4e3eb0
    // slots of vtable 0x597f20 that differ from the base's
    virtual void render(rgl::scene* sc, float* count, float* c, float* d);   // +0x28 0x4e3f20 (ret 0x10)
    virtual void set168(float r);               // +0x34 0x4e3f60 (clamp to [0.01, 1], mark dirty)
};
progress_bar* new_progress_bar(ContainerBase* o, float x, float y, float a, float b);   // 0x4e3d50

// ---- 0x4e4690: the ragdoll model record (a name and the vector of bones): src/engine/physics/rag_model.h -----------------------------
// The std::vector<RagModel> members the linker placed in this object: _Copy_backward_opt 0x4e42b0, the allocator's construct
// 0x4e42f0, fill 0x4e5f80.
#include "engine/physics/rag_model.h"

// ---- 0x4e4790..0x4e5d90: the save / load screen -----------------------------------------------------------------------------------
// (the save window is C4c31c0, game/ui/screens/screen_records.h: its nine SlotRecs, list_saves 0x4e5270 and slot_button 0x4e55f0)
#include "game/ui/screens/screen_records.h"
void label_refresh(MissionUi* w, tstring key);  // 0x4e4c70 (cdecl, the prefix by value)
__declspec(noinline) UiA* make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4e5570

#include "game/data/mission_item_recs.h"   // ItemB / ItemC: the panels that wrap a menu block (create 0x4e5d90, create2 0x4e5e60)

// ---- 0x4e65e0..0x4e75a0: the map editor's preview builders are PolyMesh's (build_spikes / build_ribbons / polygon_to_mesh): they fill
// Mission::marker_spikes / marker_ribbons / marker_poly from the nav mesh's vertices, edges and polygons (mode +0x78, light +0x80)
#include "game/terrain/poly_mesh.h"

// ---- 0x4e7a70: the map loader's block-list reader ----------------------------------------------------------------------------------------------
// 0x4c83a0 is E68::read_terrain and 0x4c84f0 the fastcall conv_map_codes (game/editor/editor_selection_and_translation.cpp: the map block and the loader that owns them)
struct TableauTbl;
void __fastcall conv_map_codes(TableauTbl* self);               // 0x4c84f0 (ecx = the loader)
// (the loader is the tableau table, game/data/tableau_table.h: the blocks are its records; TableauTbl::read 0x4e7a70, ret 4)

// ---- 0x4e8720: Mission::frame_step (the per-frame rebuild of the order-marker meshes from the nav mesh; 0x4e85d0 is Terrain::init_map)
#include "game/terrain/terrain_desc.h"
#include "game/mission/mission.h"
#include "engine/scene/world.h"                // rgl::world::hf (the height field the nav mesh is built over)

// the meeting window (character sheet) and the loading window (TU 107, 0x4e3780..0x4e3d50)
#include "game/ui/screens/screen_records.h"
typedef Rec48 MeetingWindow;      // the record of screen_records.h (vtable class); its members live there
#endif // GAME_CLS_MAP_PROGRESS_BAR_H
