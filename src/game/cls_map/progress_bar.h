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
    gui_mesh* handle;                           // +0x180 the source mesh of the bar (a resource; cloned by update)
    unsigned char f184;                         // +0x184 dirty flag (the ctor sets it)
    unsigned char f185, f186, f187;             // +0x185
    rgl::vector4 rect;                          // +0x188 (1, 1, 1, 0 after the ctor)
    virtual void update_children();             // +0xc4, vtable slot 49: update 0x4e3fb0 (called by draw_label)
    progress_bar();                             // 0x4e3eb0
    void set_ratio(float r);                    // 0x4e3f60 (clamp to [0.01, 1], mark dirty)
    void update();                              // 0x4e3fb0 (rebuilds the scaled mesh when dirty)
    void draw_label(rgl::scene* sc, float* count, int c, int d);   // 0x4e3f20 (ret 0x10)
    // slots of vtable 0x597f20 that differ from the base's (cls_map/progress_bar_slots.cpp: jmps)
    void d10(rgl::scene* sc, float* count, int c, int d);   // +0x28 -> draw_label
    void d13(int v);                            // +0x34 -> set_ratio
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
// the nine slot records (list_saves 0x4e5270 fills them from "sgDD.sav"); also read by slot_button 0x4e55f0
struct SlotRec {
    tstring name;                               // +0x00 the save's title
    tstring date;                               // +0x10 the formatted date line
    int seconds;                                // +0x20 non-zero when the slot holds a save
    int year;                                   // +0x24 the file version word
    int month;                                 // +0x28 the module version word
    unsigned char bad_type, bad_version, bad_module, pad2f;    // +0x2c the three "incompatible" flags
};
struct SaveWindow {
    char pad0[0x2c];
    SlotRec slots[9];                           // +0x2c (stride 0x30)
    char pad1dc[4];
    ContainerBase* window;                      // +0x1e0
    UiAInline* labels[9];                       // +0x1e4 one long button per slot
    MissionUi* btn_done;                        // +0x208
    MissionUi* btn_cancel;                      // +0x20c
    MissionUi* btn_delete;                      // +0x210
    char pad214[4];
    int sel218, sel21c, sel220;                 // +0x218 (-1)
    char pad224[0x234 - 0x224];
    bool is_load;                               // +0x234
    void init();                                // 0x4e4790
    void list_saves();                          // 0x4e5270
    void slot_button(ContainerBase* parent, int slot, UiAInline** out);   // 0x4e55f0 (ret 0xc)
};
void label_refresh(MissionUi* w, tstring key);  // 0x4e4c70 (cdecl, the prefix by value)
__declspec(noinline) UiAInline* make_short_button(MissionUi* parent, const tstring* text, float a, float b, float c);   // 0x4e5570

// the panel that wraps a menu block (like MainMenu / EscapeMenu): 0x4e5d80 .. 0x4e5f50
struct SaveWinCtl {
    char pad0[0x2c];
    ContainerBase* win;                         // +0x2c
    BlockOwner block;                           // +0x30
    void create_stub();                         // 0x4e5d80 (a jump to create)
    void create();                              // 0x4e5d90
    void create_flagged();                      // 0x4e5e50 (sets the window flag word, then create2)
    void create2();                             // 0x4e5e60
};

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
struct HeightField;
struct PolyMesh { void build(HeightField* hf, int step); void compute_face_normals(); };
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
struct MapLoader {
    std::vector<E68> blocks;                    // +0x00 (first at +4)
    void conv_map_codes() { ::conv_map_codes((conv_rec*)this); }       // 0x4c84f0
    void read(SlotReader* stream);              // 0x4e7a70 (ret 4)
};

// ---- 0x4e85d0 / 0x4e8720: the world-map screen's terrain build and per-frame mesh refresh -----------------------------------------------
// A partial view of the MapScreen class of src/game/char_sheet/save_game.h (0x4e87d0 buttons, which that module owns; its header
// cannot be included here: it pulls match/include/rgl.h). Only the members these two functions touch are named.
#include "../terrain_desc.h"
struct TerrainX {
    void emit_particle(int layerno, int blend, int cx, int cy, float z);   // 0x47ff60 (mesh_slot/mesh_slot.h)
};
struct Terrain {
    void fill_desc_named(TerrainDesc* d);   // 0x4656c0 (terrain_a/terrain.h)
};
struct MapScreen {
    void* view;                                 // +0x00
    char pad04[0x60 - 0x04];
    MissionUi* f60;                             // +0x60
    char pad64[0x278 - 0x64];
    int ncols;                                  // +0x278
    int nrows;                                  // +0x27c
    char pad280[0x288 - 0x280];
    char cells[1];                              // +0x288 (stride 0x94, row 0x911c)
    char pad289[0x5df4 - 0x289];
    int f5df4;                                  // +0x5df4
    char pad5df8[0x6698 - 0x5df8];
    Shape mesh;                                 // +0x6698 (the map mesh: builder state; +0x94 / +0x98 / +0x9c the meshes it fills)
    char pada0[0xc3b0dc - 0x6698 - sizeof(Shape)];
    char* mapview;                              // +0xc3b0dc
    char padview[0x845d984 - 0xc3b0dc - 4];
    int nchunk_rows;                            // +0x0845d984
    int nchunk_cols;                            // +0x0845d988
    // 0x47ff60 is TerrainX::emit_particle (mesh_slot/mesh_terrain.cpp), 0x4656c0 Terrain::fill_desc_named (terrain_a): this object is that terrain
    void emit_particle(int slot, int type, int row, int col, float f) { ((TerrainX*)this)->emit_particle(slot, type, row, col, f); }   // 0x47ff60 (ret 0x14)
    void fill_desc_named(void* desc) { ((Terrain*)this)->fill_desc_named((TerrainDesc*)desc); }       // 0x4656c0 (ret 4)
    void init(void* desc);                      // 0x4e85d0 (ret 4)
    void frame_step();                          // 0x4e8720
};

// the meeting window (character sheet) and the loading window (TU 107, 0x4e3780..0x4e3d50)
struct MeetingWindow {
    char pad0[0x2c];
    ContainerBase* bg;                          // +0x2c the window
    MissionUi* done;                            // +0x30 ui_done (meeting) / the hint label (loading)
    MissionUi* import_btn;                      // +0x34 ui_import_character / the logo
    MissionUi* export_btn;                      // +0x38 ui_export_character / the progress bar
    MissionUi* panel;                           // +0x3c the character list panel (0x597050)
    int f40;                                    // +0x40
    int f44;                                    // +0x44
    int f48;                                    // +0x48
    void init();                                // 0x4e3780
    void init2();                               // 0x4e3aa0
};
#endif // GAME_CLS_MAP_PROGRESS_BAR_H
