// progress_bar.h: the loading-screen progress bar widget (vtable 0x597f20, 0x198 bytes, constructor 0x4e3eb0) and the
// objects that use it: the meeting / loading window (src/game/cls_map/cls_map.cpp, 0x4e3780..0x4e3eb0) and the
// functions of progress_bar.cpp (0x4e3eb0..0x4e87d0). The widget is a MissionUi (../ui/mission_ui.h; base constructor
// 0x48ae20): its value is MissionUi::v168, its mesh slot tracks[2] (+0x110). ALL NAMES PROVISIONAL.
#ifndef GAME_CLS_MAP_PROGRESS_BAR_H
#define GAME_CLS_MAP_PROGRESS_BAR_H
#include "./cls_map.h"
#include "../../tinyxml/tinyxml.h"

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

// ---- 0x4e4690: the ragdoll model record (a name and the vector of bones): src/include/rag_model.h -----------------------------
// The std::vector<RagModel> members the linker placed in this object: _Copy_backward_opt 0x4e42b0, the allocator's construct
// 0x4e42f0, fill 0x4e5f80.
#include "../../include/rag_model.h"

// ---- 0x4e4790..0x4e5d90: the save / load screen -----------------------------------------------------------------------------------
// (the save window is C4c31c0, game/screen_records.h: its nine SlotRecs, list_saves 0x4e5270 and slot_button 0x4e55f0)
#include "../screen_records.h"
void label_refresh(MissionUi* w, tstring key);  // 0x4e4c70 (cdecl, the prefix by value)
__declspec(noinline) UiA* make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4e5570

#include "../options_panel/mission_item_recs.h"   // ItemB / ItemC: the panels that wrap a menu block (create 0x4e5d90, create2 0x4e5e60)

// ---- 0x4e65e0..0x4e7a70: the shape that owns the point / face / segment records and builds render meshes from them ---------------------------
// (a map polygon set: the three builders take the mesh to fill as their argument; names provisional, every offset is read off the disassembly)
// Shape is PolyMesh (terrain_b/poly_mesh.h: verts / faces / edges at +0x18 / +0x28 / +0x38) as the mesh editor's preview builders see it:
// the element records with their highlight flags, the highlighted kind (+0x78) and the light direction (+0x80). These are not PolyMesh
// members yet: declaring them there (or naming PolyMesh's pad48) moves render_batch::render_scene 0x46e590 (203 -> 157) or
// Mission::start_setup 0x484590 (1795 -> 1706), the TU-wide numbering effect of those rows' nonmatching notes.
struct ShapePt {                                // 0x18 bytes
    rgl::vector4 pos;                           // +0x00
    char flag;                                  // +0x10 (0x4e65e0: highlights the record), the next two words are read by 0x4e75a0 only as padding
    char pad11[3];
    float f14;                                  // +0x14
};
struct ShapeSeg {                               // 0x34 bytes (0x4e6a70): point indices at +0x00 / +0x04, a flag byte at +0x14
    int a, b;                                   // +0x00, +0x04
    char pad8[8];
    unsigned short u16;                         // +0x10
    char pad12[2];
    char flag;                                  // +0x14
    char pad15[0x34 - 0x15];
};
struct ShapeFace {                              // 0x64 bytes (0x4e75a0)
    char pad0[0x28];
    int idx[8];                                 // +0x28 point indices
    int count;                                  // +0x48 corner count
    char pad4c[0x60 - 0x4c];
    char flag;                                  // +0x60 material variant flag
    char pad61[0x64 - 0x61];
};
// 0x4785e0 / 0x4705e0 are PolyMesh::build / compute_face_normals (terrain_b/poly_mesh.h): the polygon set below is that class; only the members are declared
#include "../terrain_b/poly_mesh.h"
struct Shape {
    char pad0[0x18];
    std::vector<ShapePt> pts;                   // +0x18
    std::vector<ShapeFace> faces;               // +0x28
    std::vector<ShapeSeg> segs;                 // +0x38
    char pad48[0x78 - 0x48];
    int mode;                                   // +0x78 (0: spikes; 1: ribbons; 2: the map polygon mesh)
    char pad7c[4];
    float lx, ly, lz;                           // +0x80 light direction
    char pad8c[0x94 - 0x8c];
    // (the meshes the three builders fill are Mission::marker_poly / marker_spikes / marker_ribbons, right behind Mission::nav)
    void build_spikes(rgl::mesh* owner);        // 0x4e65e0
    void build_ribbons(rgl::mesh* owner);       // 0x4e6a70
    void polygon_to_mesh(rgl::mesh* out);       // 0x4e75a0
    void compute_face_normals() { ((PolyMesh*)this)->compute_face_normals(); }       // 0x4705e0
};

// ---- 0x4e7a70: the map loader's block-list reader ----------------------------------------------------------------------------------------------
struct SlotReader;
// 0x4c83a0 is E68::read_terrain and 0x4c84f0 the fastcall conv_map_codes (rgl/mesh2/gui_edit.cpp: the map block and the loader that owns them)
struct TableauTbl;
void __fastcall conv_map_codes(TableauTbl* self);               // 0x4c84f0 (ecx = the loader)
struct MapBlock : E68 {                         // the map block (0x68 bytes): its terrain reader
    void read_terrain(SlotReader* stream) { E68::read_terrain((stream_base*)stream); }      // 0x4c83a0
};
// (the loader is the tableau table, include/tableau_table.h: the blocks are its records; TableauTbl::read 0x4e7a70, ret 4)

// ---- 0x4e8720: the mission's per-frame rebuild of the order-marker meshes from the nav mesh (0x4e85d0 is Terrain::init_map) -----------------
// The object is g_mission (editor_dialog.cpp calls it on g_editor): a Mission method. The name MapScreen stays until mission.h declares it
// (that header is not this module's); MapScreen adds no member to Mission.
#include "../terrain_desc.h"
#include "../mission.h"
#include "../../rgl/scene/world.h"                // rgl::world::hf (the height field the nav mesh is built over)
struct MapScreen : Mission {
    Shape& preview() { return *(Shape*)&nav; }  // the nav mesh as the preview builders see it (above)
    void frame_step();                          // 0x4e8720
};

// the meeting window (character sheet) and the loading window (TU 107, 0x4e3780..0x4e3d50)
#include "../screen_records.h"
typedef Rec48 MeetingWindow;      // the record of screen_records.h (vtable class); its members live there
#endif // GAME_CLS_MAP_PROGRESS_BAR_H
