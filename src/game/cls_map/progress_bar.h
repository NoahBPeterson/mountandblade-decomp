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
    virtual void render(rgl::scene* sc, float* count, int c, int d);   // +0x28 0x4e3f20 (ret 0x10)
    virtual void set168(float r);               // +0x34 0x4e3f60 (clamp to [0.01, 1], mark dirty)
};
progress_bar* new_progress_bar(ContainerBase* o, float x, float y, float a, float b);   // 0x4e3d50

// ---- 0x4e4690: the ragdoll model record (a name and the vector of bones) -------------------------------------------------------
// 0x20 bytes. The std::vector<RagModel> members the linker placed in this object: _Copy_backward_opt 0x4e42b0, the allocator's construct
// 0x4e42f0 (tstring copy, then the vector<E50> copy constructor 0x4da1f0), fill 0x4e5f80 (calls vector<E50>::operator= 0x4dc1c0).
struct RagModel {
    tstring name;                               // +0x00
    std::vector<E50> bones;                     // +0x10
    RagModel() {}
    // memberwise, spelled out: the implicit one would call tstring's copy constructor (tstring::operator= returns void)
    RagModel& operator=(const RagModel& o) { name = o.name; bones = o.bones; return *this; }
    void load(TiXmlElement* e);                 // 0x4e4690
};

// ---- 0x4e4790..0x4e5d90: the save / load screen -----------------------------------------------------------------------------------
// (the save window is C4c31c0, game/screen_records.h: its nine SlotRecs, list_saves 0x4e5270 and slot_button 0x4e55f0)
#include "../screen_records.h"
void label_refresh(MissionUi* w, tstring key);  // 0x4e4c70 (cdecl, the prefix by value)
__declspec(noinline) UiA* make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4e5570

#include "../options_panel/mission_item_recs.h"   // ItemB / ItemC: the panels that wrap a menu block (create 0x4e5d90, create2 0x4e5e60)

// ---- 0x4e65e0..0x4e7a70: the shape that owns the point / face / segment records and builds render meshes from them ---------------------------
// (a map polygon set: the three builders take the mesh to fill as their argument; names provisional, every offset is read off the disassembly)
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
    char pad0[0x1c];
    ShapePt* pts;                               // +0x1c (a std::vector's first; the allocator word is +0x18)
    ShapePt* pts_end;                           // +0x20
    char pad24[0x2c - 0x24];
    ShapeFace* faces;                           // +0x2c
    ShapeFace* faces_end;                       // +0x30
    char pad34[0x3c - 0x34];
    ShapeSeg* segs;                             // +0x3c
    ShapeSeg* segs_end;                         // +0x40
    char pad44[0x78 - 0x44];
    int mode;                                   // +0x78 (0: spikes; 1: ribbons; 2: the map polygon mesh)
    char pad7c[4];
    float lx, ly, lz;                           // +0x80 light direction
    char pad8c[0x94 - 0x8c];
    rgl::mesh* poly_mesh;                       // +0x94 the render meshes the three builders fill (MapScreen +0x672c / +0x6730 / +0x6734)
    rgl::mesh* spike_mesh;                      // +0x98
    rgl::mesh* ribbon_mesh;                     // +0x9c
    void build(void* grid, int mode) { ((PolyMesh*)this)->build((HeightField*)grid, mode); }     // 0x4785e0 (ret 8; src/game/terrain_b)
    void build_spikes(rgl::mesh* owner);        // 0x4e65e0
    void build_ribbons(rgl::mesh* owner);       // 0x4e6a70
    void polygon_to_mesh(rgl::mesh* out);       // 0x4e75a0
    void compute_face_normals() { ((PolyMesh*)this)->compute_face_normals(); }       // 0x4705e0
};

// ---- 0x4e7a70: the map loader's block-list reader ----------------------------------------------------------------------------------------------
struct SlotReader;
// 0x4c83a0 is Block::read_terrain and 0x4c84f0 the fastcall conv_map_codes (rgl/mesh2/gui_edit.cpp: the map block and the loader that owns them)
struct Block {
    void read_terrain(stream_base* stream);   // 0x4c83a0
};
struct conv_rec;
void __fastcall conv_map_codes(conv_rec* self);               // 0x4c84f0 (ecx = the loader)
struct MapBlock : E68 {                         // the map block (0x68 bytes): its terrain reader
    void read_terrain(SlotReader* stream) { ((Block*)this)->read_terrain((stream_base*)stream); }      // 0x4c83a0
};
// (the loader is the tableau table, include/tableau_table.h: the blocks are its records; TableauTbl::read 0x4e7a70, ret 4)

// ---- 0x4e85d0 / 0x4e8720: the world-map screen's terrain build and per-frame mesh refresh -----------------------------------------------
// A partial view of the MapScreen class of src/game/char_sheet/save_game.h (0x4e87d0 buttons, which that module owns; its header
// cannot be included here: it pulls match/include/rgl.h). Only the members these two functions touch are named.
#include "../terrain_desc.h"
struct MapScreen {
    void* view;                                 // +0x00
    char pad04[0x5df4 - 0x04];
    int f5df4;                                  // +0x5df4
    char pad5df8[0x6698 - 0x5df8];
    Shape mesh;                                 // +0x6698 (the map mesh: builder state; +0x94 / +0x98 / +0x9c the meshes it fills)
    void frame_step();                          // 0x4e8720
};

// the meeting window (character sheet) and the loading window (TU 107, 0x4e3780..0x4e3d50)
#include "../screen_records.h"
typedef Rec48 MeetingWindow;      // the record of screen_records.h (vtable class); its members live there
#endif // GAME_CLS_MAP_PROGRESS_BAR_H
