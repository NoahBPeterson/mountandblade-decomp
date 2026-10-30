// mission_5.h: declarations of src/game/mission5/mission_5.cpp (TU 116 "mission_5", 0x525160..0x52cf60): the classes, globals
// and helpers the functions of the mission screen use. The slices of this stretch each carried their own partial view of the
// same few objects (the game-loop screen, the mission 0x5e292c, the game data 0x5e2930, the agents, the message log, the menu
// widgets); the views are kept as distinct classes with a per-slice suffix (Mission_s261e0 = the layout s005261e0 saw), and
// every global that was declared with a view type is declared once here as a raw pointer / byte array, the slices' names being
// cast macros over it. Merging the views into mission.h / agent_obj.h / game_data.h is open work (see MIGRATION.md).
// tstring, the streams and rgl::vector4 / rgl::matrix are the shared headers' (src/game/tstring.h, src/rgl/stream.h,
// src/rgl/rgl_math.h); offsets in the views cite the function that proves them, in each view's comments.
#ifndef MISSION5_MISSION_5_H
#define MISSION5_MISSION_5_H
#include "../../include/image_addrs.h"
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <new>
#include "../tstring.h"
#include "../../rgl/rgl_math.h"
#include "../../rgl/stream.h"
#include "../../rgl/mesh.h"
using rgl::vector4;
using rgl::matrix;

// vtable 0x54add8 (stream.h: bin_writer) with its constructor inline, as 0x527c90 builds it on the stack
struct bin_writer_i : file_writer_base {
    bin_writer_i() {}
    STREAM_SLOTS
    virtual int flush();
};

// ---- the globals the slices saw with their own view types (one raw declaration each)
extern void* g_mission; // 0x5e292c
extern void* g_game; // 0x5e2930
extern char g_dialogs[]; // 0x8ed50c8
extern int g_conversation_troop;   // 0x8ed50d4
extern int g_encountered_party;    // 0x8ed50d8
extern int g_conversation_agent;   // 0x8ed50dc
extern char g_strtable[]; // 0x5e2a18
extern char g_gui[]; // 0x5d7080
extern char g_game_keys[]; // 0xa73730
extern char g_keyboard[]; // 0xa170c8
extern char g_windows[]; // 0xa73980
extern char g_map_flags[]; // 0x5e291c
extern char g_mission_ctx[]; // 0xa73300
extern void* g_items; // 0x5e29d0
extern char g_game_keys[]; // 0xa73730
extern char g_keyboard[]; // 0xa170c8
#include "../placed_obj.h"
#include "../../include/shape.h"
#include "../std_vec_out_of_line.h"
#include "../prop_kind.h"
#include "../menu_rec.h"
extern void* g_mission_templates; // 0x5e2a00
extern char g_terrain[]; // 0xa76630
extern char g_app_obj[]; // 0x8ee3c48


// ======================================================= slice s00525160: classes, globals, helpers

// ===========================================================================
// mount&blade.exe (MSVC 7.1 / cl 13.10, x86-32) -- frozen batch ds1, slice 263
//
// The game-loop event handler: one big frame function that drives the
// per-frame camera/sky update, the mission "show/hide" party switches, the
// per-frame scene item pass and the main-menu result dispatch.
// ===========================================================================
typedef unsigned int uint;

// ---------------------------------------------------------------------------
// the engine's generic polymorphic object: only the virtual slots this
// function calls are given real signatures (slot = byte offset / 4)
// ---------------------------------------------------------------------------
struct VObj_s25160 {
    virtual void v0();  virtual void v1();  virtual void v2();  virtual void v3();
    virtual void v4();                                          // 0x10
    virtual void v5();  virtual void v6();  virtual void v7();  virtual void v8();
    virtual void v9();  virtual void v10(); virtual void v11(); virtual void v12();
    virtual void v13(); virtual void v14(); virtual void v15(); virtual void v16();
    virtual char v17(int a);                                    // 0x44
    virtual void v18(int a);                                    // 0x48
    virtual void v19(); virtual void v20(); virtual void v21();
    virtual void v22(void* p);                                  // 0x58
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36();
    virtual void v37(void* p);                                  // 0x94
    virtual void v38(); virtual void v39(); virtual void v40();
    virtual void v41(void* a, void* b);                         // 0xa4
    virtual void v42(); virtual void v43();
    virtual void v44();                                         // 0xb0
    virtual void v45();                                         // 0xb4
};

struct Mgr_s25160 { __forceinline void update(); };                                  // 0x4f1390
struct Dlg_s25160 { __forceinline void open(int a); };                               // 0x495700
struct Mgr513710_s25160 { void m(void* p); };                           // 0x513710
struct Chunk_s25160 { void m_4464d0(int a); void m_4f8bc0(); };
struct Troop_s25160 { void m_4630d0(int i); };
struct Big5e292c_s25160 { void m_483f60(int a, int b); };


#define g_mgr       (*(Mgr_s25160*)&orig::g_gui_mgr)
#define g_dlg       (*(Dlg_s25160*)&orig::g_main_menu_obj)
#define g_513710    (*(Mgr513710_s25160*)&orig::g_map_flags)
#define g_big       (*(Big5e292c_s25160**)&orig::g_mission)

// ---------------------------------------------------------------------------
// the game object (only the members this handler touches)
// ---------------------------------------------------------------------------
struct Game_s25160 {
    char pad0[0x18];                                            // +0x00
    int f18;                                                    // +0x18
    char pad0b[0x30 - 0x1c];
    char* f30;                                                  // +0x30 mission object
    char pad1[0x188 - 0x34];
    char* f188;                                                 // +0x188
    char pad2[0x190 - 0x18c];
    VObj_s25160* arr190[0x400];                                        // +0x190
    int f1190;                                                  // +0x1190
    char pad3[0x11e0 - 0x1194];
    VObj_s25160* f11e0;                                                // +0x11e0
    VObj_s25160* f11e4;                                                // +0x11e4
    VObj_s25160* f11e8;                                                // +0x11e8
    VObj_s25160* f11ec[5];                                             // +0x11ec
    int f1200;                                                  // +0x1200
    int f1204;                                                  // +0x1204
    __forceinline void refresh(int i, int* out);                             // 0x4f73c0
    __forceinline void update();                                            // 0x5257f0
    void handle_event(int code);                                // 0x525160
};

// ===========================================================================
// ======================================================= slice s005257f0: classes, globals, helpers
#define g_mission_s257f0 (*(Mission_s257f0**)&g_mission)
#define g_game_s257f0 (*(Game_s257f0**)&g_game)
#define g_n_dialogs_s257f0 (*(DialogMgr_s257f0*)g_dialogs)
// Slice s005257f0: the dialogue "look at" update of the view object (0x5257f0) and its two agent lookups,
// find_speaker_agent (0x526070) and find_listener_agent (0x526130).  The lookup pair scans the mission's
// agent table for the agent whose troop id (+0x88) equals the current dialogue speaker / listener.
// The big function builds a look-at frame from the target and listener bodies' matrices, blends it into
// this+0x1194, copies it out, hands it to Mission_s257f0::camera_to and advances the blend timer.  Names provisional.
using rgl::vector4;
using rgl::matrix;

// ---- agent records ---------------------------------------------------------------------------------------
struct AgentBody_s257f0 {                                  // Agent_s257f0 +0x250 (s00440a30/ai_agent.h)
    char pad0[0x58];
    matrix m;                                       // +0x58: s, f (+0x68), u, o (+0x88)
};
struct Agent_s257f0 {                                      // 0x5928 bytes
    char pad0[4];
    int used;                                       // +0x04
    char pad8[0x10 - 8];
    int f10;                                        // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                        // +0x2c
    char pad30[0x50 - 0x30];
    vector4 pos;                                    // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                      // +0x88 (troop / character id)
    char pad8c[0x250 - 0x8c];
    AgentBody_s257f0* body;                                // +0x250
    __forceinline float speed_scale() const;                      // 0x432450: 1.5, or 2.4 while standing and not deleted
    char pad254[0x5928 - 0x254];
};
struct AgentTable_s257f0 {                                 // mission+4
    int count;                                      // +0x00
    char pad4[0x14];
    Agent_s257f0** pages;                                  // +0x18 (mission+0x1c)
    __forceinline int first_used();                               // 0x43b1b0
    __forceinline void next_used(unsigned* i);                    // 0x43b160
};
#define AGENT(id) (g_mission_s257f0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))

struct Mission_s257f0 {
    char pad0[4];
    AgentTable_s257f0 agents;                              // +0x04
    char pad20[0x5c - 0x20];
    int selected;                                   // +0x5c
    char pad60[0x6624 - 0x60];
    matrix m6624;                                   // +0x6624
    char pad6664[0x6748 - 0x6664];
    char cam[0x100];                                // +0x6748
    __forceinline void camera_to(matrix* eye, float& far_plane);  // 0x4fa790
};


struct TroopRec_s257f0 {                                   // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                                 // +0x30 (0x10 = a talking troop?)
    char pad34[0x508 - 0x34];
};
struct Game_s257f0 {
    char pad0[0xc8];
    int player;                                     // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec_s257f0* chars;                                // +0x13a00
};


struct DialogMgr_s257f0 {
    char pad0[0x8000];
    __forceinline int speaker();                                  // 0x460640
};


int find_listener_agent();                          // 0x526130 (defined in slice s00526130)

extern float g_look_rate;                           // 0x5935c8 (4.0)

// ---- the view object (only the members this function touches) --------------------------------------------
struct ViewObj_s257f0 {
    char pad0[0x30];
    void* f30;                                      // +0x30 (its +0x1ac receives 0x120 bytes from +0x34)
    char pad34[0xfc - 0x34];
    matrix m_fc;                                    // +0xfc (copy of the look-at frame)
    char pad13c[0x1194 - 0x13c];
    matrix m;                                       // +0x1194 (s, f, u, o)
    float blend;                                    // +0x11d4
    char started;                                   // +0x11d8
    __declspec(noinline) void update();                                  // 0x5257f0
};

struct vec3_s257f0 {
    float x, y, z;
    __forceinline void sub_assign(const vec3_s257f0& o);                 // 0x432770
};

static __forceinline float clamp01_s257f0(float x)
{
    if (x < 0.0f)
        x = 0.0f;
    else if (x >= 1.0f)
        x = 1.0f;
    return x;
}

// ======================================================= slice s00526130: classes, globals, helpers
#define g_mission_s26130 (*(Mission_s26130**)&g_mission)
#define g_game_s26130 (*(Game_s26130**)&g_game)
#define g_n_dialogs_s26130 (*(DialogMgr_s26130*)g_dialogs)
// Slice s00526130: the dialogue-listener agent lookup (0x526130).  It scans the mission's agent table for
// the used agent whose troop id (+0x88) equals DialogMgr_s26130::listener and returns the last match, or -1.
// Names provisional; every callee/global carries its VA.

struct Agent_s26130 {                                      // 0x5928 bytes
    char pad0[4];
    int used;                                       // +0x04
    char pad8[0x10 - 8];
    int f10;                                        // +0x10
    char pad14[0x2c - 0x14];
    int f2c;                                        // +0x2c
    char pad30[0x88 - 0x30];
    int troop;                                      // +0x88 (troop / character id)
    char pad8c[0x5928 - 0x8c];
};
struct AgentTable_s26130 {                                 // mission+4
    int count;                                      // +0x00
    char pad4[0x14];
    Agent_s26130** pages;                                  // +0x18 (mission+0x1c)
    __forceinline int first_used();                               // 0x43b1b0
    __forceinline void next_used(unsigned* i);                    // 0x43b160
};
struct Mission_s26130 {
    char pad0[4];
    AgentTable_s26130 agents;                              // +0x04
};


struct TroopRec_s26130 {                                   // 0x508 bytes
    char pad0[0x30];
    unsigned flags;                                 // +0x30 (0x10 = a talking troop?)
    char pad34[0x508 - 0x34];
};
struct Game_s26130 {
    char pad0[0xc8];
    int player;                                     // +0xc8
    char padcc[0x13a00 - 0xcc];
    TroopRec_s26130* chars;                                // +0x13a00
};


struct DialogMgr_s26130 {
    char pad0[0x8000];
    __forceinline int listener();                                 // 0x460670
};


// ======================================================= slice s005261e0: classes, globals, helpers
#define g_strtable_s261e0 (*(StrTable_s261e0*)g_strtable)
#define g_gui_s261e0 (*(MsgLog_s261e0*)g_gui)
#define g_keytable_s261e0 (*(KeyTable_s261e0*)g_game_keys)
#define g_keyready_s261e0 (*(KeyReady_s261e0*)g_keyboard)
#define g_mission_s261e0 (*(Mission_s261e0**)&g_mission)
#define g_game_s261e0 (*(GameData_s261e0**)&g_game)
#define g_main_menu_s261e0 (*(MainMenu_s261e0*)g_windows)
#define g_w_a739a0_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x20)))
#define g_w_a739bc_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x3c)))
#define g_w_a739b0_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x30)))
#define g_w_a739b4_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x34)))
#define g_w_a739e8_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x68)))
#define g_w_a739e4_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x64)))
#define g_w_a73994_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x14)))
#define g_w_a739c8_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x48)))
#define g_w_a739c0_s261e0 (*(Widget_s261e0**)&(*(void**)((char*)&g_windows + 0x40)))
#define g_flags_s261e0 (*(Flags5e291c_s261e0*)g_map_flags)
#define g_a73300_s261e0 (*(Obj73300_s261e0*)g_mission_ctx)
#define g_deque_s261e0 (*(IntDeque_s261e0*)((char*)((char*)&g_windows + 0x78)))
// Slice s005261e0: the mission screen / game-loop tick (0x5261e0, 6818 bytes, thiscall, ret 4).
//
// The `this` object is the mission HUD/controller: its fields +0x2c8 (mode), +0x2cc (selection),
// +0x2d4 (flag), +0x2d8/+0x2dc, +0x2f1/+0x2f2/+0x2f4 and +0x324 are read/written here; the item source
// object at +0x30 is MissionSource (0x4d5410/0x4dd920, see slice s004d5410).  clobbered globals are the
// mission (0x5e292c), the game data (0x5e2930) and the GUI/message log (0x5d7080).
//
// One frame: lock, key toggles, scene time-scale, renderer "ready" rate, HUD/camera update, the no-camera
// position save, the mission-editor key block (modes 0x15..0x24 and the three group flags), the region
// change/timers (0x29c..0x2c8), the item-use keys (kinds 1/4/5/6/7), the sound-slot ticks, menu actions
// (0xa73980), the conversation auto-close timer, and finally the menu-option / quick-save / dialog handlers.
//
// Offsets were read from this function; names are provisional.  Declarations reuse the verified layouts
// from slices s004d5410 (MissionSource, tstring, MsgLog_s261e0), s004f1390 (MsgLog_s261e0), s0043ac40 (AgentTable_s261e0),
// s004532f0 (GameData_s261e0/Mission_s261e0) and s004d5410 (MainMenu_s261e0/Widget_s261e0), kept local to this slice.

// -----------------------------------------------------------------------------------------------------
// rgl string table (16-byte tstring: read cursor, buffer, capacity, length); ctor 0x42bbe0, copy
// 0x42c110, dtor 0x42bc40.  c_str() is the inline `buf ? buf : (char*)&buf` seen at 0x526b7f.


struct StrTable_s261e0 {
    __forceinline tstring* lookup(tstring* key);                      // 0x43a1e0 (thiscall, ret 4)
};


// -----------------------------------------------------------------------------------------------------
// message log (global object 0x5d7080) -- see s004f1390 / s004529e0
struct MsgLog_s261e0 {
    char pad0[0xb0b8];
    __forceinline void add(tstring msg, int color);                   // 0x452c00 (ret 0x14)
    __forceinline void flush();                                       // 0x4f1390
    __forceinline void set_o568(int a, int b);                        // 0x42cdb0
    __forceinline void set_view(float* v);                            // 0x4c5190
};


// -----------------------------------------------------------------------------------------------------
// key tables: two 256-entry int arrays at 0xa170d4 (down) and 0xa174d4 (up); 0x480 apart.

struct KeyTable_s261e0 {
    __forceinline char clicked(int key);                                 // 0x48ac90 (thiscall, ret 4)
};


struct KeyReady_s261e0 {
    __forceinline char is_ready(int key);                             // 0x487900 (thiscall, ret 4)
};


// 2-entry binding arrays of the mission-editor actions
extern int g_keys_7350[2];                              // 0xa73750
extern int g_keys_7388[2];                              // 0xa73788
extern int g_keys_73a8[2];                              // 0xa737a8
extern int g_keys_73b0[2];                              // 0xa737b0
extern int g_keys_73b8[2];                              // 0xa737b8
extern int g_keys_73c0[2];                              // 0xa737c0
extern int g_keys_73c8[2];                              // 0xa737c8

// -----------------------------------------------------------------------------------------------------
// agents / mission / game data
struct Agent_s261e0;
struct AgentTable_s261e0 {
    int  count;                                         // +0
    char pad4[0x0c];
    __forceinline int  first_used();                                  // 0x43b1b0
    __forceinline void next_used(unsigned* i);                        // 0x43b160 (ret 4)
};

struct Agent_s261e0 {
    char pad0[0x10];
    int  f10;                                           // +0x10
    int  f14;                                           // +0x14
    char pad18[0x50 - 0x18];
    float px, py, pz;                                   // +0x50
    char pad5c[0x88 - 0x5c];
    int  f88;                                           // +0x88
    char pad8c[0x1c4 - 0x8c];
    int  f1c4;                                          // +0x1c4 (kind)
    __forceinline bool is_enemy(const Agent_s261e0* o) const;                // 0x432350 (ret 4)
};

struct MissionSub_s261e0 {
    __forceinline float elapsed() const;                              // 0x432c10 (when this = mission+0x6328)
    __forceinline void  build_geometry();                             // 0x470a00 (when this = mission+0x6698)
};
struct LabelSub_s261e0 {
    __forceinline void make_label(char* out);                         // 0x48d940 (ret 4)
};

// mission object 0x5e292c
struct Mission_s261e0 {
    char  pad0[0x1c];
    void** pages;                                       // +0x1c (pages of 16 agents)
    char  pad20[0x5c - 0x20];
    int   local_id;                                     // +0x5c
    char  pad60[0x5df0 - 0x60];
    int   cur_rec;                                      // +0x5df0
    int   f5df4;                                        // +0x5df4
    char  pad5df8[0x6328 - 0x5df8];
    // +0x6328  MissionSub_s261e0 (elapsed)
    char  pad6328[0x6338 - 0x6328];
    unsigned char b6338;                                // +0x6338
    char  pad6339[0x633c - 0x6339];
    int   f633c;                                        // +0x633c
    char  pad6340[0x6524 - 0x6340];
    int   f6524;                                        // +0x6524
    unsigned char f6528;                                // +0x6528
    char  pad6529[0x6554 - 0x6529];
    unsigned char b6554;                                // +0x6554
    char  pad6555[0x6558 - 0x6555];
    int   f6558;                                        // +0x6558
    char  pad655c[0x6620 - 0x655c];
    int   f6620;                                        // +0x6620
    char  pad6624[0x666c - 0x6624];
    int   f666c;                                        // +0x666c
    int   f6670;                                        // +0x6670
    int   f6674;                                        // +0x6674
    char  pad6678[0x668c - 0x6678];
    char* f668c;                                        // +0x668c (per-item array, 0x1c8 apart)
    char  pad6690[0x6698 - 0x6690];
    // +0x6698  MissionSub_s261e0 (build_geometry)
    char  pad6698[0x67f4 - 0x6698];
    float f67f4;                                        // +0x67f4
    float f67f8;                                        // +0x67f8
    char  pad67fc[0x6800 - 0x67fc];
    int   f6800;                                        // +0x6800
    char  pad6804[0x6808 - 0x6804];
    __int64 f6808;                                      // +0x6808
    float f6810;                                        // +0x6810
    void  mode_flag1_stub();
    __forceinline bool  mode_flag1() const;                           // 0x435840
    __forceinline void  save_scene_data();                                   // 0x527f90
    __forceinline void  fn_529d80();                                   // 0x529d80
    __forceinline void  release_item(int arg);                            // 0x528410 (ret 4)
    __forceinline void  frame(float dt, char spawn);                  // 0x483f60
};


struct GameData_s261e0 {
    char  pad0[0x18];
    unsigned char b18;                                  // +0x18
    char  pad19[0x8c - 0x19];
    int   f8c;                                          // +0x8c
    char  pad90[0xa8 - 0x90];
    int   fa8;                                          // +0xa8
    char  padac[0xc8 - 0xac];
    int   fc8;                                          // +0xc8
    char  padcc[0x139e4 - 0xcc];
    int   f139e4;                                       // +0x139e4
    char  pad139e8[0x13a00 - 0x139e8];
    char* chars;                                        // +0x13a00
    int   f13a04;                                       // +0x13a04
    int   f13a08;                                       // +0x13a08
    char  pad13a0c[0x13a50 - 0x13a0c];
    int*  f13a50;                                       // +0x13a50
    char  pad13a54[0x13ad0 - 0x13a54];
    float f13ad0;                                       // +0x13ad0
    int   f13ad4;                                       // +0x13ad4
    char  pad13ad8[0x13af4 - 0x13ad8];
    int   f13af4;                                       // +0x13af4
    int   f13af8;                                       // +0x13af8
    char  pad13afc[0x13b04 - 0x13afc];
    int   f13b04;                                       // +0x13b04
    int   f13b08;                                       // +0x13b08
    int   f13b0c;                                       // +0x13b0c
    char  pad13b10[0x1a090 - 0x13b10];
    int   f1a090;                                       // +0x1a090
    char  pad1a094[0x1a1a4 - 0x1a094];
    float v1a1a4[4];                                    // +0x1a1a4
    int   f1a1b4;                                       // +0x1a1b4
    char  pad1a1b8[0x1a208 - 0x1a1b8];                  // +0x1a1b8 vector<int>
    int   f1a208;                                       // +0x1a208
    int   f1a20c;                                       // +0x1a20c
    __forceinline void  save_scene(int arg);                            // 0x527c90 (ret 4)
};


struct VectorInt_s261e0 {
    void resize(unsigned n, int v);                     // 0x43f310 (ret 8)
};

// mission-source subobject at Hud+0x30 (slice s004d5410)
struct Src_s261e0 {
    __forceinline void pick(int* arg);                                // 0x4d5410 (ret 4)
    __forceinline void init(int a, int b);                            // 0x4dd920 (ret 8)
};

// menu + widgets (slice s004d5410 / s00494e40)
struct MainMenu_s261e0 {
    char pad0[0x18];
    int  sel;                                           // +0x18
    __forceinline void set_state(int option);                            // 0x495700 (ret 4)
    __forceinline void open(int idx);                             // 0x494260 (ret 4)
    __forceinline void setup2(tstring* a, tstring* b, tstring* c, int* v);  // 0x48a3d0 (ret 0x10)
};


struct Widget_s261e0 {
    char pad0[4];
    int  f4;                                            // +4
    char pad8[0x14 - 8];
    int  f14;                                           // +0x14 (dword: float bits or int)
    int  f18;                                           // +0x18
};










struct DialogMgr_s261e0 {
    __forceinline void start(int a, int b, int c, int d, char f);     // 0x4610b0 (ret 0x14)
};
// The dialog manager is a fixed object at 0x8ed50c8 (outside the tool's annotation range, so it is
// named through a literal cast here).
extern DialogMgr_s261e0 g_dialogmgr;          // 0x8ed50c8

struct Flags5e291c_s261e0 {
    char b0, b1, b2, pad3;
    int  f4, f8;
    __forceinline void commit(char* out);                             // 0x513710 (ret 4)
};


struct SoundSlot_s261e0 {
    __forceinline void update(int arg);                               // 0x4625d0 (ret 4)
};
struct SoundBank_s261e0 {
    __forceinline void update_all(int arg);                           // 0x462760 (ret 4)
};
struct EmitSub_s261e0 {
    __forceinline void play(int idx);                          // 0x4875e0 (ret 4)
};

// ---- plain globals ----------------------------------------------------------------------------------
extern unsigned char g_mt_inited;                    // 0x5e2a09
extern CRITICAL_SECTION g_mt_cs;                           // 0x5e28dc
extern HANDLE g_mt_event;                                  // 0x5e28d8
extern int  g_pending_action;                                  // 0x5a9f18
extern int  g_vert_budget;                                  // 0x5a9c34
extern char g_cfg[]; // 0x5a9f98
extern unsigned char g_a17908;                          // 0xa17908
extern int  g_presentation;                                  // 0x5e299c
#include "../../include/renderer.h"
extern char g_script[]; // 0xa70440
extern int g_skill_clamp;                       // 0x8ee3ac4
#define g_clamp_max g_skill_clamp                  // 0x8ee3ac4
extern int  g_a7333c;                                   // 0xa7333c
struct Obj73300_s261e0 { void f_5295a0(); };                   // 0x5295a0

extern int  g_show_quest_notes;                                   // 0xa181b4
extern int  g_a170d8;                                   // 0xa170d8
extern int  g_a174d8;                                   // 0xa174d8

// the 4-int-block deque-like container at 0xa739f8 (map at +4, mapsize at +8, offset at +0xc, size at +0x10)
struct IntDeque_s261e0 {
    int   pad0;
    void** map;
    int   mapsize;
    int   off;
    int   size;
};


// float constants in .rdata
extern const float g_f54a6e4;                           // 0x54a6e4 = 0.2f
extern const float g_f549ca8;                           // 0x549ca8 = 0.0f
extern const float g_f54a6d0;                           // 0x54a6d0 = 1e-05f
extern const float g_f549cdc;                           // 0x549cdc = 1.0f
extern const float g_f54b8b0;                           // 0x54b8b0 = 1.5f
extern const float g_f590b38;                           // 0x590b38 = 8.0f
extern const float g_f549d7c;                           // 0x549d7c = 0.1f

// ---- plain callees ----------------------------------------------------------------------------------
void __cdecl restore_defaults();                        // 0x4d3a50
void __cdecl quick_save_msg();                                // 0x516a70 (quick save)
void quick_save_msg(); // 0x516a70
void __cdecl operator delete(void* p);                        // 0x535712 (operator delete, ??3@YAXPAX@Z)
extern "C" int __cdecl sprintf(char*, const char*, ...);   // 0x536f8f

static __forceinline bool any_pressed_s261e0(const int* keys, int n)
{
    bool r = false;
    for (int i = 0; i < n; i++) {
        int k = keys[i];
        if (k >= 0 && ((int*)((char*)&g_keyboard + 0xc))[k] != 0 && ((int*)((char*)&g_keyboard + 0x40c))[k] == 0)
            r = true;
    }
    return r;
}

static __forceinline Agent_s261e0* page_agent_s261e0(int id)
{
    char** pages = *(char***)((char*)g_mission_s261e0 + 0x1c);
    return (Agent_s261e0*)(pages[id >> 4] + (id & 0xf) * 0x5928);
}

static __forceinline int* rec_base_s261e0(int id)
{
    return (int*)((char*)g_mission_templates + id * 0x58 + 0x4c);
}

// -----------------------------------------------------------------------------------------------------
// the HUD / mission-screen object (`this` of 0x5261e0)
struct Screen_s261e0 {
    char  pad0[0x18];
    int   f18;                                          // +0x18
    char  pad1c[0x2c - 0x1c];
    int   f2c;                                          // +0x2c
    char  src30[4];                                     // +0x30 item source (Src_s261e0)
    void* f34;                                          // +0x34
    int   f38;                                          // +0x38
    int   f3c;                                          // +0x3c
    char  pad40[0x44 - 0x40];
    unsigned char flags44;                              // +0x44
    char  pad45[0x48 - 0x45];
    int   f48;                                          // +0x48
    char  pad4c[0x50 - 0x4c];
    __int64 f50;                                        // +0x50
    float f58;                                          // +0x58
    void* f5c;                                          // +0x5c
    char  pad60[0x218 - 0x60];
    float v218[4];                                      // +0x218
    char  pad228[0x29c - 0x228];
    int   f29c;                                         // +0x29c
    int   f2a0;                                         // +0x2a0
    char  pad2a4[0x2a8 - 0x2a4];
    int   f2a8;                                         // +0x2a8
    char  pad2ac[0x2b0 - 0x2ac];
    __int64 f2b0;                                       // +0x2b0
    int   f2b8;                                         // +0x2b8
    char  pad2bc[0x2c0 - 0x2bc];
    __int64 f2c0;                                       // +0x2c0
    int   f2c8;                                         // +0x2c8
    int   f2cc;                                         // +0x2cc
    char  pad2d0[0x2d4 - 0x2d0];
    int   f2d4;                                         // +0x2d4
    int   f2d8;                                         // +0x2d8
    int   f2dc;                                         // +0x2dc
    unsigned char f2e0;                                 // +0x2e0
    unsigned char f2e1;                                 // +0x2e1
    char  pad2e2[0x2f1 - 0x2e2];
    unsigned char f2f1;                                 // +0x2f1
    unsigned char f2f2;                                 // +0x2f2
    char  pad2f3[0x2f4 - 0x2f3];
    int   f2f4;                                         // +0x2f4
    char  pad2f8[0x324 - 0x2f8];
    int   f324;                                         // +0x324

    void tick(float dt);                                // 0x5261e0
    __forceinline void update(float dt);                            // 0x5284a0 (ret 4)
    __forceinline void update_marks();                                    // 0x529130
    __forceinline void order_tick();                                    // 0x52b300
    __forceinline void f_529ec0();                                    // 0x529ec0
    __forceinline void update_ui();                                    // 0x52ba10
    __forceinline void init_order_markers();                                    // 0x52ae70
    __forceinline void update_equipment();                                     // 0x4f8b40 (ignores this, but takes ecx)
    __forceinline void update_camera(float dt);                       // 0x4f8c80 (ret 4)
    __forceinline void update_status();                        // 0x4fb350
};

// vtable of the object at Screen_s261e0+0x5c: only slot 0xc4 (index 49) is called here.
struct SrcVtbl_s261e0 {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21(); virtual void v22(); virtual void v23();
    virtual void v24(); virtual void v25(); virtual void v26(); virtual void v27();
    virtual void v28(); virtual void v29(); virtual void v30(); virtual void v31();
    virtual void v32(); virtual void v33(); virtual void v34(); virtual void v35();
    virtual void v36(); virtual void v37(); virtual void v38(); virtual void v39();
    virtual void v40(); virtual void v41(); virtual void v42(); virtual void v43();
    virtual void v44(); virtual void v45(); virtual void v46(); virtual void v47();
    virtual void v48(); virtual void v49();
};

// ======================================================= slice s00527c90: classes, globals, helpers
#define g_game_s27c90 (*(GameData_s27c90**)&g_game)
#define g_mission_s27c90 (*(Mission_s27c90**)&g_mission)
// Slice s00527c90 (batch ds1): the scene/map .sco save path.
//
//   0x527c90  GameData_s27c90::save_scene      -- open Modules\<mod>\SceneObj_s27c90\<name>.sco and serialise the scene record
//   0x527da0  SceneData::save           -- write both object lists of a scene record
//   0x527ef0  SceneObj_s27c90::save            -- write one scene object
//   0x527f90  Mission_s27c90::save_scene_data  -- reset both in-memory streams and serialise the live grids
//   0x528000  GridSet_s27c90::save             -- write the grid count, dimensions and every grid
//   0x528060  Grid_s27c90::save                -- run-length encode one grid
//   0x528410  Mission_s27c90::release_item     -- free the entity behind one per-item slot
//   0x528470  Resource_s27c90::release         -- mark a resource's sub-resources for deletion
//
// Class layouts: tstring / stream_base / bin_writer come from mbbin.h (verified in s00417850..s00418a70);
// the Mission_s27c90 and GameData_s27c90 field offsets are those of s005261e0 (0x5e292c / 0x5e2930).  Names provisional.

// ---- engine string helpers -----------------------------------------------------------------------------
// tstring::c_str() is the inlined `buf ? buf : (char*)&buf` seen at 0x527cc1.
struct GameData_s27c90;
struct Mission_s27c90;

// The scene record (0x494 bytes) in the GameData_s27c90 scene table at +0x13a50.
struct SceneObjRec_s27c90;

struct GameData_s27c90 {
    char pad0[0x13a50];
    SceneObjRec_s27c90* scenes;                    // +0x13a50
    __forceinline tstring scene_path(int idx);            // 0x463950 (returns by hidden pointer; ret 8)
    __forceinline const char* module_path(const char* sub);   // 0x4639d0 (ret 4)
    __declspec(noinline) void save_scene(int idx);               // 0x527c90 (ret 4)
};




struct Mesh_s27c90;

// Resource_s27c90 with a sub-resource vector at +0x12c and a flag word at +8 (0x528470).
struct Resource_s27c90 {
    char pad0[8];
    unsigned flags;                         // +8
};

struct ResourceOwner_s27c90 {
    char pad0[0x12c];
    Resource_s27c90** first;                       // +0x12c
    Resource_s27c90** last;                        // +0x130
    __declspec(noinline) void release();    // 0x528470
};

struct SceneObjRec_s27c90 {
    char pad0[0x30];
    unsigned flags30;                       // +0x30
    char pad34[0x108 - 0x34];
    char* objs_first;                       // +0x108 (vector of 0x1c8-byte scene objects)
    char* objs_last;                        // +0x10c
    char pad110[0x414 - 0x110];
    mem_stream stream_a;                    // +0x414 (0x30 bytes)
    mem_stream stream_b;                    // +0x444
    char pad474[0x494 - 0x474];
    void save(stream_base* s, int arg);     // 0x527da0 (ret 8)
};

// one 0x1c8-byte scene object
struct SceneObj_s27c90 {
    char pad0[0x10];
    int f10;                                // +0x10
    int f14;                                // +0x14
    char pad18[0x98 - 0x18];
    char mat[0x158 - 0x98];                 // +0x98 matrix (0x30 rotation + 3 floats)
    float f158, f15c, f160;                 // +0x158
    char pad164[0x188 - 0x164];
    int f188;                               // +0x188
    int f18c;                               // +0x18c
    tstring name;                           // +0x190
    int f1a0;                               // +0x1a0
    int f1a4;                               // +0x1a4
    char pad1a8[0x1b4 - 0x1a8];
    void* res;                              // +0x1b4 (owned resource)
    char pad1b8[0x1c8 - 0x1b8];
    void save(stream_base* s);              // 0x527ef0 (ret 4)
};

stream_base& w_matrix(stream_base& s, rgl::matrix* m);     // 0x415ef0 (cdecl)

// ---- grid container (Mission_s27c90+0x63d0) -------------------------------------------------------------------
struct Grid_s27c90 {
    tstring name;                           // +0
    int type;                               // +0x10
    float* data;                            // +0x14
    void save(stream_base* s, int* dims);   // 0x528060 (ret 8)
};

struct GridSet_s27c90 {
    int count;                              // +0
    char pad4[0xc - 4];
    Grid_s27c90 grids[1];                          // +0xc
    char pad144[0x144 - 0xc - sizeof(Grid_s27c90)];
    int width;                              // +0x144
    int height;                             // +0x148
    void save(stream_base* s);              // 0x528000 (ret 4)
};

// ---- Mission_s27c90 (0x5e292c) --------------------------------------------------------------------------------
struct MissionSub_s27c90 {
    char pad0[0x1c];
    int count;                              // +0x1c
    char* first;                            // +0x20
    char* last;                             // +0x24
    __forceinline void read(stream_base* s);              // 0x524f00 (ret 4)
};

struct Mission_s27c90 {
    char pad0[0x5df4];
    int cur_scene;                          // +0x5df4 (index into GameData_s27c90::scenes)
    char pad5df8[0x63d0 - 0x5df8];
    GridSet_s27c90 grids;                          // +0x63d0
    char pad668c[0x668c - 0x63d0 - sizeof(GridSet_s27c90)];
    char* item_array;                       // +0x668c (per-item array, 0x1c8 apart)
    char pad6690[0x6698 - 0x6690];
    MissionSub_s27c90 other;                       // +0x6698
    __declspec(noinline) void save_scene_data();                 // 0x527f90
    __declspec(noinline) int release_item(int idx);              // 0x528410 (ret 4)
};

// ---- world (Mission_s27c90+0) ---------------------------------------------------------------------------------
struct World_s27c90 {
    __forceinline void remove_entity(int id);              // 0x41f0d0 (ret 4)
};

struct MissionHdr_s27c90 {
    World_s27c90* world;                           // +0
};

// ======================================================= slice s005284a0: classes, globals, helpers
#define g_items_s284a0 (*(ItemKind_s284a0**)&g_items)
#define g_mission_s284a0 (*(Mission_s284a0**)&g_mission)
#define g_game_s284a0 (*(Game_s284a0**)&g_game)
#define g_keys_s284a0 (*(KeyTable_s284a0*)g_game_keys)
#define g_key_names_s284a0 (*(KeyNames_s284a0*)g_keyboard)
// Slice s005284a0: the mission player-view / aim controller's per-frame update (0x5284a0, 8192 bytes).
//
// It damps the view yaw/pitch (+0x268/+0x26c) from the look input, clamps the pitch, the look speed
// (+0x278) and the field of view (+0x288, 37 .. 90/97 depending on the sheathed flag), builds the view
// frame and hands its forward row to the local agent's look direction (+0x55d8), and (while the game is
// not paused) maps the bound keys into the agent's movement / action bits (+0x55d0), runs the slot
// cycling keys, toggles the drawn-weapon state, and turns the three "action" keys into Blow_s284a0 records for
// the agents under the crosshair.  `this` is the mission view object (same object as s004f7f00's
// MissionObj: +0x268 yaw, +0x2c8 mode, +0x2d4 aim target, +0x2fc snap counter); names are provisional.

using rgl::vector4;
using rgl::matrix;

// ---- item records ------------------------------------------------------------------------------------
struct ItemKind_s284a0 {                                       // 0x238 bytes, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned type_flags;                                // +0x1c8 (low byte = itp_type_*)
    char pad1cc[0x238 - 0x1cc];
};


struct ItemRef; typedef ItemRef ItemRef_s284a0;   // sig s1: canonical ItemRef (src/game/item_ref.h: item, m4)

// ---- the blow record (0x58 bytes, ctor 0x4353a0) -----------------------------------------------------
struct Blow_s284a0 {
    __forceinline Blow_s284a0();                                             // 0x4353a0 (== RecZSubB::init)
    char pad0[4];
    int attacker;                                       // +0x04
    vector4 pos;                                        // +0x08 (position of the agent hit)
    char pad18[0x38 - 0x18];
    int f38;                                            // +0x38
    float damage;                                       // +0x3c
    char pad40[0x48 - 0x40];
    int f48;                                            // +0x48
    char pad4c[0x58 - 0x4c];
};

// ---- the mission / agent objects (layouts reused from s004556d0, s004f7f00) --------------------------
struct Agent_s284a0;
struct AgentTable_s284a0 {
    int count;                                          // +0x00
    char pad4[0x18 - 4];
    Agent_s284a0** pages;                                      // +0x18 (first page pointer; mission+0x1c)
    __forceinline void next_used(unsigned* i);                        // 0x43b160
    __forceinline int first_used();                                   // 0x43b1b0
    Agent_s284a0* get(unsigned id);                            // 0x43b1f0
};

struct Agent_s284a0 {                                          // 0x5928 bytes
    char pad0[4];
    int used;                                           // +0x04
    char pad8[8];
    int f10;                                            // +0x10
    int f14;                                            // +0x14
    char pad18[0x2c - 0x18];
    int mount_id;                                       // +0x2c (-1 on foot)
    char pad30[0x50 - 0x30];
    vector4 pos;                                        // +0x50
    vector4 dir;                                        // +0x60 (facing direction)
    char pad70[0x7c - 0x70];
    int f7c;                                            // +0x7c (drawn-weapon state)
    char pad80[0x100 - 0x80];
    int f100;                                           // +0x100
    char pad104[0x118 - 0x104];
    float f118;                                         // +0x118
    char pad11c[0x22c - 0x11c];
    int wield_a;                                        // +0x22c
    char pad230[0x2c8 - 0x230];
    int mode;                                           // +0x2c8 (aim mode)
    int f2cc;                                           // +0x2cc (aim target)
    char pad2d0[0x2d4 - 0x2d0];
    int f2d4;                                           // +0x2d4
    char pad2d8[0x31c - 0x2d8];
    unsigned target_id;                                 // +0x31c
    char pad320[0x55d0 - 0x320];
    unsigned in_flags;                                  // +0x55d0 (movement / action bits)
    char f55d4;                                         // +0x55d4
    char pad55d5[0x55d8 - 0x55d5];
    vector4 dir55d8;                                    // +0x55d8 (look direction)
    char pad55e8[0x55ec - 0x55e8];
    int f55ec;                                          // +0x55ec
    char pad55f0[0x560c - 0x55f0];
    int f560c;                                          // +0x560c
    int f5610;                                          // +0x5610
    int f5614;                                          // +0x5614
    char pad5618[0x5720 - 0x5618];
    int f5720;                                          // +0x5720
    int f5724;                                          // +0x5724
    char pad5728[0x5928 - 0x5728];

    __forceinline ItemRef_s284a0 slot_pair(int idx);                         // 0x432790 (ret 8)
    __forceinline bool is_enemy(Agent_s284a0* o);                          // 0x432350 (ret 4)
    __forceinline bool is_friend(Agent_s284a0* o);                         // 0x4323a0 (ret 4)
    __forceinline void note_blow(Blow_s284a0* b);                            // 0x457af0 (ret 4)
    __forceinline void cycle_slot(int slot);                          // 0x44bc30 (ret 4)
    __forceinline void on_wield_reset();                              // 0x445a20
};

struct Mission_s284a0 {
    char pad0[4];
    AgentTable_s284a0 agents;                                  // +0x04
    char pad20[0x5c - 0x20];
    int selected;                                       // +0x5c (local agent id)
};


struct Game_s284a0 {
    char pad0[0x139e4];
    int f139e4;                                         // +0x139e4 (weapons sheathed)
    char pad139e8[0x1a090 - 0x139e8];
    int f1a090;                                         // +0x1a090 (mission paused / cutscene)
};


// ---- the view object (`this`) -----------------------------------------------------------------------
struct ViewCtrl_s284a0 {
    char pad0[0x44];
    unsigned char flags44;                              // +0x44 (bit 0: keyboard/analog look input)
    char pad45[0x268 - 0x45];
    float yaw;                                          // +0x268
    float pitch;                                        // +0x26c
    float yaw_rate;                                     // +0x270
    float pitch_rate;                                   // +0x274
    float look_speed;                                   // +0x278 (0.7 .. 10)
    char pad27c[0x288 - 0x27c];
    float fov;                                          // +0x288 (37 .. 90 / 97)
    char pad28c[0x2c8 - 0x28c];
    int mode;                                           // +0x2c8 (aim mode)
    int target;                                         // +0x2cc (aim target)
    char pad2d0[0x2d4 - 0x2d0];
    int aim_valid;                                      // +0x2d4
    char pad2d8[0x2fc - 0x2d8];
    int snap;                                           // +0x2fc (frames left to snap the view)

    __declspec(noinline) void update(float dt);                              // 0x5284a0
    __forceinline void update_equipment();                       // 0x4f8b40
};

// a view of rgl::matrix with the engine's rotation helpers (thiscall, ecx = matrix)
struct MatrixExt_s284a0 {
    __forceinline void rotate_sf(float* angle);                       // 0x40e770
    __forceinline void rotate_fu(float* angle);                       // 0x40e570
};
void identity3(matrix* m);                              // 0x40cd60

// ---- key tables -------------------------------------------------------------------------------------
struct KeyTable_s284a0 {                                       // 0xa73730
    __forceinline char clicked(int key);                      // 0x48ac90 (ret 4)
    __forceinline char is_down(int key);                           // 0x48ac50 (ret 4)
};

struct KeyNames_s284a0 {                                       // 0xa170c8
    __forceinline char is_ready(int action);                             // 0x487900 (ret 4)
};


// ---- globals ----------------------------------------------------------------------------------------
extern int g_a17188;                                    // 0xa17188
extern int g_a17128;                                    // 0xa17128
extern int g_a17184;                                    // 0xa17184
extern int g_a1712c;                                    // 0xa1712c
extern int g_a17190;                                    // 0xa17190
extern int g_a1715c;                                    // 0xa1715c
extern int g_a1717c;                                    // 0xa1717c
extern int g_a171d0;                                    // 0xa171d0
extern int g_a175d0;                                    // 0xa175d0
extern int g_a171d4;                                    // 0xa171d4
extern int g_a175d4;                                    // 0xa175d4
extern int g_a171c8;                                    // 0xa171c8
extern int g_a175c8;                                    // 0xa175c8
extern int g_a17160;                                    // 0xa17160
extern int g_a17560;                                    // 0xa17560
extern int g_a73790;                                    // 0xa73790
extern int g_a73794;                                    // 0xa73794
extern int g_presentation;                                    // 0x5e299c
extern int g_invert;                                    // 0x5aa01c
extern int g_stick_mode;                                // 0x5aa078
#include "../../include/fx.h"
#include "../../rgl/resource_manager.h"

#define AGENT(id) (g_mission_s284a0->agents.pages[(unsigned)(id) >> 4] + ((id) & 0xf))

// ======================================================= slice s00529130: classes, globals, helpers
#define g_scene_s29130 (*(Mission_s29130**)&(*(void**)((char*)&g_mission + 0x0)))
#define g_game_s29130 (*(GameData_s29130**)&g_game)
#define g_items_s29130 (*(ItemKind_s29130**)&g_items)
// Slice s00529130: the mission HUD's per-frame marker update (0x529130), its per-frame tick
// (0x5295a0), the marker enable/disable pass (0x529600) and the party-stack hero reset (0x529670).
// Everything here is a __thiscall member of the HUD object; the marker widgets are reached through
// their vtables (slot 0x58 = set position, 0xb0 = hide, 0xb4 = show).  Names are provisional.

using rgl::vector4;

// ---- vtable-bearing marker widget (slots by byte offset: 0x58 pos, 0xb0 hide, 0xb4 show) ---------------
struct Widget_s29130 {
    virtual void v00(); virtual void v01(); virtual void v02(); virtual void v03();
    virtual void v04(); virtual void v05(); virtual void v06(); virtual void v07();
    virtual void v08(); virtual void v09(); virtual void v10(); virtual void v11();
    virtual void v12(); virtual void v13(); virtual void v14(); virtual void v15();
    virtual void v16(); virtual void v17(); virtual void v18(); virtual void v19();
    virtual void v20(); virtual void v21();
    virtual void set_pos(const float* p);                     // +0x58
    virtual void v23(); virtual void v24(); virtual void v25(); virtual void v26();
    virtual void v27(); virtual void v28(); virtual void v29(); virtual void v30();
    virtual void v31(); virtual void v32(); virtual void v33(); virtual void v34();
    virtual void v35(); virtual void v36(); virtual void v37(); virtual void v38();
    virtual void v39(); virtual void v40(); virtual void v41(); virtual void v42();
    virtual void v43();
    virtual void hide();                                      // +0xb0
    virtual void show();                                      // +0xb4
};

// secondary widget with an ARGB colour dword at +0x3c
struct Mark_s29130 {
    char pad[0x3c];
    unsigned color;                                           // +0x3c
};

// ---- agent record (0x5928 bytes; layout from s0043ac40 / s0043bdb0 / s0043b5d0) -------------------------
struct Agent_s29130;
struct AgentTable_s29130 {
    int count;                                                // +0x00
    char pad4[0x18 - 4];
    Agent_s29130** pages;                                            // +0x18 (id >> 4 -> page of 16)
};
typedef ItemRef ItemRef_s29130;
struct Agent_s29130 {
    char pad0[0x10];
    int f10;                                                  // +0x10
    int f14;                                                  // +0x14
    char pad18[0x2c - 0x18];
    int mount;                                                // +0x2c
    char pad30[0x50 - 0x30];
    int f50;                                                  // +0x50
    char pad54[0x60 - 0x54];
    char blk60[0x88 - 0x60];                                  // +0x60 (aim block, blended with +0x55d8)
    int troop;                                                // +0x88
    char pad8c[0x94 - 0x8c];
    int f94;                                                  // +0x94
    char pad98[0x1bc - 0x98];
    int f1bc;                                                 // +0x1bc (party id)
    int f1c0;                                                 // +0x1c0 (party stack index)
    char pad1c4[0x55d8 - 0x1c4];
    vector4 facing;                                           // +0x55d8
    char pad55e8[0x560c - 0x55e8];
    int f560c;
    char pad5610[0x5624 - 0x5610];
    vector4 prev_facing;                                      // +0x5624
    float aim_error;                                          // +0x5634
    char pad5638[0x56c4 - 0x5638];
    int f56c4;                                                // +0x56c4 (range-table index)
    char pad56c8[0x571c - 0x56c8];
    float f571c;                                              // +0x571c
    int pad5720;                                              // +0x5720
    float f5724;                                              // +0x5724
    int pad5728[2];                                           // +0x5728
    int f5730;                                                // +0x5730
    char pad5734[0x5928 - 0x5734];

    __forceinline ItemRef_s29130 slot_pair(int idx) const;                         // 0x432790 (thiscall, ret 8)
    __forceinline bool f100_is_0x48_and_charged() const;                                // 0x432470 (thiscall)
    __forceinline int pick_front();                                        // 0x43b9e0 (fastcall)
    __forceinline int pick_attack_dir(unsigned target);                     // 0x43f970 (thiscall, ret 4)
};

// ---- mission (the object at 0x5e292c: agent table at +4, current agent at +0x5c) ------------------------
struct Mission_s29130 {
    int f0;                                                   // +0x00
    AgentTable_s29130 agents;                                        // +0x04
    char pad20[0x5c - 0x20];
    unsigned cur_agent_s29130;                                       // +0x5c
    char pad60[0x6524 - 0x60];
    int f6524;                                                // +0x6524
    __forceinline void fn_529840(int mode);                                 // 0x529840 (thiscall, ret 4)
};

// ---- party stack (0x20 bytes, array at party+0xa8; layout from s00490a30) --------------------------------
struct Stack_s29130 {
    int count;                                                // +0x00
    int troop;                                                // +0x04
    int flags;                                                // +0x08 (byte at +0xb0, bit 0)
    int f0c, f10, f14, f18, f1c;
};
struct Party_s29130 {
    char pad0[0xa8];
    Stack_s29130 stacks[64];                                         // +0xa8
    int nstacks;                                              // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];
};
struct PartyMgr_s29130 {
    char pad0[0x138d0];
    Party_s29130** pages;                                            // +0x138d0
    __forceinline int index_of(unsigned id);                                // 0x48ad00 (thiscall, ret 4)
};
struct Troop_s29130 {
    char pad0[0x30];
    unsigned flags;                                           // +0x30 (0x10 = hero)
    char pad34[0xa0 - 0x34];
    float f_a0;                                               // +0xa0
    char pad_a4[0x508 - 0xa4];
};
struct GameData_s29130 {
    char pad0[0x78];
    int cur_party;                                            // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                                            // +0x8c
    __forceinline void more_start();                                         // 0x51f800 (thiscall)
};

// ---- the HUD object -----------------------------------------------------------------------------------
struct MissionHud_s29130 {
    int f0;                                                   // +0x00
    int f4;                                                   // +0x04
    char pad8[0x14 - 8];
    int f14;                                                  // +0x14 (state: 1, 3, 4 tested here)
    char pad18[0x68 - 0x18];
    Widget_s29130* w[7];                                             // +0x68
    float color[7];                                           // +0x84
    Mark_s29130* marks[7];                                           // +0xa0
    char padbc[0x154 - 0xbc];
    int f154;                                                 // +0x154 (party id)
    char pad158[0x288 - 0x158];
    float f288;                                               // +0x288 (field of view, degrees)
    char pad28c[0x2c8 - 0x28c];
    char f2c8;                                                // +0x2c8

    __declspec(noinline) void update_marks();                                      // 0x529130
    void tick();                                              // 0x5295a0
    void refresh_markers();                                   // 0x529600
    void reset_party_heroes(int party);                       // 0x529670
};

// ---- externals -----------------------------------------------------------------------------------------


extern int g_flag5aa06c;                                      // 0x5aa06c
extern int g_flag5aa070;                                      // 0x5aa070
struct ItemKind_s29130 { char pad[0x1c8]; int type; char pad1cc[0x238 - 0x1cc]; };

extern float g_deg2rad;                                       // 0x590ce4 (0.017453292)
extern float g_half;                                          // 0x549d70 (0.5f)
extern float g_0375;                                          // 0x59c310 (0.375f)
extern float g_2p5;                                           // 0x590c4c (2.5f)
extern float g_1p2;                                           // 0x590b94 (1.2f)
extern float g_255;                                           // 0x590cec (255.0f)
extern float g_0;                                             // 0x549ca8 (0.0f)
extern float g_1;                                             // 0x549cdc (1.0f)
extern float g_range_tbl[];                                   // 0x59390c (lo/hi pairs)

float __cdecl signed_angle(void* a, void* b);                 // 0x4381e0

inline PartyMgr_s29130* party_mgr_s29130() { return (PartyMgr_s29130*)((char*)g_game_s29130 + 0xf8); }
inline Troop_s29130* troop_tbl_s29130() { return *(Troop_s29130**)((char*)g_game_s29130 + 0x13a00); }
inline Agent_s29130* cur_agent_s29130() { return &g_scene_s29130->agents.pages[g_scene_s29130->cur_agent_s29130 >> 4][g_scene_s29130->cur_agent_s29130 & 0xf]; }

// ======================================================= slice s00529840: classes, globals, helpers
#define g_scene_s29840 (*(Mission_s29840**)&(*(void**)((char*)&g_mission + 0x0)))
#define g_game_s29840 (*(GameData_s29840**)&g_game)
// Slice s00529840: the mission object's per-agent update (0x529840) and the party-stack helpers it
// dispatches to (0x529b70 / 0x529bc0 / 0x529c30 / 0x529d00), plus the scene/mission record sync (0x529d80).
// Everything here is a member of the object at 0x5e292c (agent table at +4); 0x529bc0 is a free __stdcall.

using rgl::vector4;

// ---- agent record (0x5928 bytes; layout from s0043ac40 / s0043bdb0 / s0043b5d0) -------------------------
struct Agent_s29840;
struct AgentTable_s29840 {
    int count;                                                // +0x00
    char pad4[0x18 - 4];
    Agent_s29840** pages;                                            // +0x18 (id >> 4 -> page of 16)

    __forceinline unsigned first_used();                                    // 0x43b1b0 (thiscall)
    __forceinline void next_used(unsigned* p);                              // 0x43b160 (thiscall, ret 4)
};
struct Agent_s29840 {
    char pad0[0x10];
    int f10;                                                  // +0x10
    int f14;                                                  // +0x14
    char pad18[0x2c - 0x18];
    int mount;                                                // +0x2c
    char pad30[0x50 - 0x30];
    int f50;                                                  // +0x50
    char pad54[0x88 - 0x54];
    int troop;                                                // +0x88
    char pad8c[0x94 - 0x8c];
    int f94;                                                  // +0x94
    char pad98[0x1bc - 0x98];
    int f1bc;                                                 // +0x1bc (party id)
    int f1c0;                                                 // +0x1c0 (party stack slot)
    char pad1c4[0x55d8 - 0x1c4];
    vector4 facing;                                           // +0x55d8
    char pad55e8[0x571c - 0x55e8];
    float f571c;                                              // +0x571c
    int pad5720;                                              // +0x5720
    float f5724;                                              // +0x5724
    int pad5728[2];                                           // +0x5728
    int f5730;                                                // +0x5730
    char pad5734[0x5928 - 0x5734];
};

// ---- party stacks (0x20-byte records at party+0xa8; layout from s00490a30) ------------------------------
struct Stack_s29840 {
    int count;                                                // +0x00
    int troop;                                                // +0x04
    int flags;                                                // +0x08
    int f0c;                                                  // +0x0c
    int f10;                                                  // +0x10
    int f14, f18, f1c;
};
struct Party_s29840 {
    char pad0[0xa8];
    Stack_s29840 stacks[64];                                         // +0xa8
    int nstacks;                                              // +0x8a8
    char pad8ac[0x5ac8 - 0x8ac];

    __forceinline void add_xp_to_stack(int slot, int value);                // 0x4b1d80 (thiscall, ret 8)
    __forceinline int skill_level(int n);                                         // 0x4b2370 (thiscall, ret 4)
};
struct PartyMgr_s29840 {
    char pad0[0x138d0];
    Party_s29840** pages;                                            // +0x138d0

    __forceinline int index_of(unsigned id);                                // 0x48ad00 (thiscall, ret 4)
    __forceinline Party_s29840* get(int id);                                       // 0x51ee50 (thiscall, ret 4)
    Party_s29840* at(int idx) { return &pages[idx >> 4][idx & 0xf]; }
};

// ---- troop records (0x508 bytes in the table at g_game_s29840+0x13a00) ----------------------------------------
struct Troop_s29840 {
    char pad0[0x30];
    unsigned flags;                                           // +0x30 (0x10 = hero)
    char pad34[0x50 - 0x34];
    int f50;                                                  // +0x50
    char pad54[0xa0 - 0x54];
    float f_a0;                                               // +0xa0
    char pad_a4[0xb0 - 0xa4];
    int b0;                                                   // +0xb0 (level)
    char pad_b4[0x508 - 0xb4];
};

struct GameData_s29840 {
    char pad0[0x78];
    int cur_party;                                            // +0x78
    char pad7c[0x8c - 0x7c];
    int cur_troop;                                            // +0x8c
};

// ---- 0x1c8-byte mission/scene record (layout from s00440a30) -------------------------------------------

struct IfaceB_s29840 {
    int f4, f8, fc;
    IfaceB_s29840() {}
    virtual ~IfaceB_s29840() {}
    virtual void slot1() {}
    virtual void slot2() {}
};
// sig s1: the 0x1c8 record is the canonical PlacedObj (ctor 0x437830, copy 0x437bb0, operator= 0x437d60 in agent_stats.cpp)
typedef PlacedObj Rec1c8_s29840;

// ---- scene record list (the std::vector is at scene+0x104) ---------------------------------------------
struct Scene_s29840 {
    char pad0[0x104];
    std::vector<Rec1c8_s29840> recs;                                 // +0x104
    char pad114[0x494 - 0x114];
};

// ---- the mission object at 0x5e292c --------------------------------------------------------------------
struct Mission_s29840 {
    int f0;                                                   // +0x00
    AgentTable_s29840 agents;                                        // +0x04
    char pad20[0x5c - 0x20];
    unsigned cur_agent;                                       // +0x5c
    char pad60[0x5df4 - 0x60];
    int f5df4;                                                // +0x5df4 (scene index)
    char pad5df8[0x6688 - 0x5df8];
    std::vector<Rec1c8_s29840> records;                              // +0x6688

    __declspec(noinline) void fn_529840(int mode);                                 // 0x529840 (thiscall, ret 4)
    void fn_529b70(Agent_s29840* a);                                 // 0x529b70 (thiscall, ret 4)
    void fn_529c30(int* hist, int party, int slot, int value, int flag); // 0x529c30 (thiscall, ret 0x14)
    void fn_529d00(Agent_s29840* a);                                 // 0x529d00 (thiscall, ret 4)
    __declspec(noinline) void fn_529d80();                                         // 0x529d80 (thiscall)
};

// ---- externals -----------------------------------------------------------------------------------------


extern float g_exp_rate;                                      // 0x590c54 (1.0f)

inline PartyMgr_s29840* party_mgr_s29840() { return (PartyMgr_s29840*)((char*)g_game_s29840 + 0xf8); }
inline Troop_s29840* troop_tbl_s29840() { return *(Troop_s29840**)((char*)g_game_s29840 + 0x13a00); }

__declspec(noinline) void __stdcall fn_529bc0(Agent_s29840* a, int add); // 0x529bc0 (ret 8)

// ======================================================= slice s00529ec0: classes, globals, helpers
#define g_gui_s29ec0 (*(gui_manager_s29ec0*)g_gui)
#define g_strtable_s29ec0 (*(StrTable_s29ec0*)g_strtable)
#define g_items_s29ec0 (*(ItemKind_s29ec0**)&g_items)
#define g_game_s29ec0 (*(GameData_s29ec0**)&g_game)
#define g_prop_kinds_s29ec0 (*(PropKind_s29ec0**)&g_propkinds)
#define g_mission_s29ec0 (*(Mission_s29ec0**)&g_mission)
#define g_mapobjs_s29ec0 (*(MapObjList_s29ec0**)&g_menus)
#define g_areas_s29ec0 (*(Area_s29ec0**)&g_mission_templates)
#define g_panel_s29ec0 (*(hud_panel_s29ec0**)&(*(void**)((char*)&g_gui + 0x568)))
// Slice s00529ec0: the mission screen's interaction probe, Screen_s29ec0::f_529ec0 (0x529ec0, 4011 bytes).
//
// Every frame the screen asks "what is the local agent looking at?":
//   1. cast a thin capsule_s29ec0 (1e-4 m) from the agent's eye 10.5 m forward and query the level grid for
//      face shapes (shape_type()==2); the nearest hit gives a preliminary distance `best`;
//   2. re-run the probe as a 0.1 m capsule_s29ec0 extended 0.1 m forward against the scene's *entities*
//      (mask 9) and keep the nearest entity within best+1.0 m: agents on foot (talk), ridden horses
//      (mount/dismount), scene props (chest / baggage / map-object / equipment) and the map edges
//      (leave area / retreat);
//   3. when nothing was hit, fall back to the mount/dismount test and to the four mission-border
//      tests, which use the agent's facing vector;
//   4. validate the selection (enemy/friend test only colours the label; talk and mount additionally
//      need a free look, a riding skill high enough and/or a short distance);
//   5. reset the HUD line cache, localise the label ("ui_talk", "ui_mount", ...) and draw the coloured
//      label at y=0.027 followed by a grey " <action>" at y=0.01/0.035;
//   6. publish {mode, selection, valid} to Screen_s29ec0+0x2c8/+0x2cc/+0x2d4, where the key handler
//      (s005261e0) consumes them.
//
// Layouts: shape/capsule_s29ec0/contact_s29ec0/level from s00439100, Agent_s29ec0/ItemRef_s29ec0/ItemKind_s29ec0 from s00434ad0 +
// s0043b5d0, tstring/gui_manager_s29ec0/StrTable_s29ec0 idioms from s0052ae70, Screen_s29ec0 from s005261e0, the map
// object helpers from s004c6ca0. Names are provisional; every offset is read off this disassembly.

using rgl::vector4;
using rgl::matrix;



// ---- engine string (0x10 bytes) ----------------------------------------------------------------------


// ---- gui manager (0x5d7080) --------------------------------------------------------------------------
struct gui_manager_s29ec0 {
    __forceinline void set_view(const float* v);                              // 0x4c5190 (ret 4)
    __forceinline void draw_string(const tstring* s, float y, unsigned color, int centered);  // 0x4e43e0 (ret 0x10)
};


// the HUD panel object stored in gui_manager_s29ec0+0x568 (0x5d75e8)
struct hud_panel_s29ec0 {
    virtual void reset(int on);                                 // slot 0
};

// ---- localisation tables (0x5e2a18) ------------------------------------------------------------------
struct StrTable_s29ec0 {
    __forceinline tstring* find(const tstring* key);                          // 0x4c6010 (null when absent, ret 4)
    __forceinline tstring* lookup(const tstring& key);                        // 0x43a1e0 (ret 4)
};


// ---- collision shapes (s00439100) --------------------------------------------------------------------
struct shape_base_s29ec0 {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    virtual ~shape_base_s29ec0() {}                                    // slot 0
    virtual int shape_type();                                   // slot 1 (0 sphere, 1 capsule_s29ec0, 2 face, ...)
    virtual void v2(); virtual void v3(); virtual void v4(); virtual void v5(); virtual void v6();
    virtual void v7(); virtual void v8(); virtual void v9();
    virtual vector4* get_min(vector4* out);                     // slot 10
    virtual vector4* get_max(vector4* out);                     // slot 11
};
typedef capsule capsule_s29ec0;   // sig s1: the canonical capsule (src/include/shape.h; ctor 0x433310, transform 0x408770)
struct contact_s29ec0 {                                                // 0x20 bytes
    vector4 pos;                                                // 0x00
    vector4 point;                                              // 0x10
};
matrix* identity_pos(matrix* out);                              // 0x40cd00 (cdecl)

// ---- level / scene -----------------------------------------------------------------------------------
struct SceneEntity_s29ec0 {
    void* vtbl;                                                 // +0x00
    char pad4[0x0c - 0x04];
    int id;                                                     // +0x0c (agent / prop index)
    int type;                                                   // +0x10 (0xb agent, 0xc prop, ...)
    char pad14[0x88 - 0x14];
    vector4 pos;                                                // +0x88
    shape_base_s29ec0* shape;                                          // +0x98
};
struct Scene_s29ec0 {
    __forceinline void query_segment(const float* a, const float* b, unsigned mask,
                       shape_base_s29ec0** out, int* count);           // 0x439870 (ret 0x14)
    char pad0[0x80];
    SceneEntity_s29ec0** ent_begin;                                    // +0x80
    SceneEntity_s29ec0** ent_end;                                      // +0x84
    __forceinline void collect(int type, SceneEntity_s29ec0** out, int* count);   // 0x51a3a0 (ret 0xc)
};

// ---- items (s00434ad0 / s0043b5d0) -------------------------------------------------------------------
struct ItemKind_s29ec0 {                                               // 0x238, array at 0x5e29d0
    char pad0[0x1c8];
    unsigned char type_flags;                                   // +0x1c8 (low byte: itp_type_*)
    char pad1c9[0x1ec - 0x1c9];
    int f1ec;                                                   // +0x1ec
    char pad1f0[0x238 - 0x1f0];
};


// an 8-byte item reference; the modifier sits in byte 7
struct ItemRef_s29ec0 {
    int item;                                                   // +0x0
    unsigned m4;                                                // +0x4 (modifier in byte +7)
    __forceinline int stat_1ec();                                             // 0x434c50 (thiscall)
    __forceinline void name(char* out);                                       // 0x512730 (thiscall, ret 4)
};

// ---- agents (0x5928 bytes) ---------------------------------------------------------------------------
struct Agent_s29ec0 {
    char pad0[0x10];
    int f10;                                                    // +0x10 kind (0 = human, 1 = mount)
    int f14;                                                    // +0x14 (1 = rideable)
    char pad18[0x2c - 0x18];
    int mount;                                                  // +0x2c mounted agent id (-1 = on foot)
    int f30;                                                    // +0x30 (-1 = free)
    char pad34[0x50 - 0x34];
    vector4 pos;                                                // +0x50
    char pad60[0x88 - 0x60];
    int troop;                                                  // +0x88 troop record index
    ItemRef_s29ec0 item;                                               // +0x8c
    char pad94[0xac - 0x94];
    float f9c;                                                  // +0xac
    char padb0[0x55d8 - 0xb0];
    vector4 facing;                                             // +0x55d8
    char pad55e8[0x5830 - 0x55e8];
    int f5830;                                                  // +0x5830 action state (2 = mounted)
    char pad5834[0x5928 - 0x5834];

    __forceinline float speed_scale();                                             // 0x432450 (thiscall, no arg)
    __forceinline bool is_enemy(Agent_s29ec0* o);                                    // 0x432350 (ret 4)
    __forceinline bool is_friend(Agent_s29ec0* o);                                   // 0x4323a0 (ret 4)
};

// ---- game data (0x5e2930) ----------------------------------------------------------------------------
struct GameData_s29ec0 {
    char pad0[0x8c];
    int f8c;                                                    // +0x8c (troop record used for skills)
    char pad90[0x13a00 - 0x90];
    char* chars;                                                // +0x13a00 troop records (0x508 bytes)
    char pad13a04[0x13a50 - 0x13a04];
    char* map_records;                                          // +0x13a50 area records (0x494 bytes)
    int f13a54;                                                 // +0x13a54 number of areas
    char pad13a58[0x13b10 - 0x13a58];
    char f13b10;                                                // +0x13b10
    char f13b11;                                                // +0x13b11
    __forceinline int skill_level(int troop, int skill);                      // 0x437b20 (ret 8)
};


// ---- mission (0x5e292c) ------------------------------------------------------------------------------
struct SceneProp_s29ec0 {                                              // 0x1c8, array at mission+0x668c
    char pad0[0x188];
    int destroyed;                                              // +0x188 (0 intact, 5 map object, 6 equipment)
    int kind;                                                   // +0x18c scene prop kind
    char pad190[0x1a0 - 0x190];
    int inv;                                                    // +0x1a0 inventory / chest index
    int obj;                                                    // +0x1a4 map object / equipment index
    char pad1a8[0x1c8 - 0x1a8];
};
struct PropKind_s29ec0 {                                               // 0x78, array at 0x5e29dc
    char pad0[0x10];
    int flags;                                                  // +0x10 (low byte: 5 chest, 10 exit)
    char pad14[0x78 - 0x14];
};


struct Mission_s29ec0 {
    Scene_s29ec0* scene;                                               // +0x00 (level grid + entity list)
    char pad4[0x1c - 0x04];
    Agent_s29ec0** pages;                                              // +0x1c agent table pages
    char pad20[0x5c - 0x20];
    int local_id;                                               // +0x5c
    char pad60[0x5df0 - 0x60];
    int f5df0;                                                  // +0x5df0 current area record
    int f5df4;                                                  // +0x5df4 current party/area index
    char pad5df8[0x6528 - 0x5df8];
    char f6528;                                                 // +0x6528 flags (bit 0)
    char pad6529[0x653c - 0x6529];
    float f653c;                                                // +0x653c mission border min x
    float f6540;                                                // +0x6540 mission border min y
    float f6544;                                                // +0x6544 mission border max x
    float f6548;                                                // +0x6548 mission border max y
    char pad654c[0x668c - 0x654c];
    SceneProp_s29ec0* props;                                           // +0x668c
};


inline Agent_s29ec0* agent_at_s29ec0(unsigned id)
{
    return (Agent_s29ec0*)((char*)g_mission_s29ec0->pages[id >> 4] + (id & 0x0f) * 0x5928);
}

// ---- map objects (0x5e29b0 / 0xa735c4, helpers in s004c6ca0) -----------------------------------------
struct MapObject_s29ec0 {                                              // 0x68 bytes
    char pad0[0x10];
    tstring name;                                               // +0x10
    tstring mesh;                                               // +0x20
    char pad30[0x68 - 0x30];
    __forceinline tstring* mesh_ptr();                                        // 0x4c6ca0 (thiscall)
};
typedef char MapObject_s29ec0_size_check[sizeof(MapObject_s29ec0) == 0x68 ? 1 : -1];
struct MapObjList_s29ec0 {                                             // 0x60 bytes
    char pad0[0x54];
    int count;                                                  // +0x54
    MapObject_s29ec0* objs;                                            // +0x58
    char pad5c[4];
};
typedef char MapObjList_s29ec0_size_check[sizeof(MapObjList_s29ec0) == 0x60 ? 1 : -1];   // indexed by g_mapobjs[i]: 0x60 stride (0x52a4ec: lea ecx,[ecx+ecx*2]; shl ecx,5)


// ---- area records (0x5e2a00, 0x58 bytes) --------------------------------------------------------------
struct Area_s29ec0 {                                                   // 0x58 bytes
    char pad0[0x20];
    unsigned flags;                                             // +0x20
    char pad24[0x58 - 0x24];
};


// ---- HUD globals -------------------------------------------------------------------------------------


// ---- the mission screen ------------------------------------------------------------------------------
struct Screen_s29ec0 {
    char pad0[0x26c];
    float f26c;                                                 // +0x26c
    char pad270[0x2c8 - 0x270];
    int f2c8;                                                   // +0x2c8 interaction mode
    int f2cc;                                                   // +0x2cc selection id
    char pad2d0[0x2d4 - 0x2d0];
    int f2d4;                                                   // +0x2d4 selection valid
    __declspec(noinline) void f_529ec0();                                            // 0x529ec0
};

// ======================================================= slice s0052ae70: classes, globals, helpers
#define g_resmgr_s2ae70 (*(resmgr_s2ae70*)&g_resmgr)
#define g_gui_s2ae70 (*(gui_manager_s2ae70*)g_gui)
#define g_strtable_s2ae70 (*(StrTable_s2ae70*)g_strtable)
#define g_mission_s2ae70 (*(Mission_s2ae70**)&g_mission)
// Slice s0052ae70: the mission order markers.
//   0x52ae70  build (once) the two "order" entities: the round/tetrahedron marker built from the shape
//             at Screen_s2ae70+0x6698 into three meshes, and the ground rectangle marker (four points, four
//             vertices, two triangles) of the second entity.
//   0x52b300  order tick: derive the current order id from the local agent's kind, hand it to
//             Mission_s2ae70::order, and when the "give order" state is 1 arm the 7.5 s timeout and repaint.
//   0x52b3c0  paint the five order labels (group / hold position / follow / charge / mount / dismount)
//             through the gui manager, tinting the selected one.
// The Screen_s2ae70 layout (+0x29c.. / +0x6698.. / +0x67xx) is shared with slice s005261e0; names are
// provisional and every offset is read off this disassembly.



// ---- engine string (0x10 bytes, ctor 0x42bbe0, dtor 0x42bc40) ------------------------------------------


// ---- resolved material_s2ae70 / resource manager (0x5d6fa0) ---------------------------------------------------
struct material_s2ae70;
struct resmgr_s2ae70 {
    __forceinline material_s2ae70* get_material(tstring* name);              // 0x418a70 (thiscall, ret 4)
};


// ---- gui manager (0x5d7080) ----------------------------------------------------------------------------
struct gui_manager_s2ae70 {
    __forceinline void set_o568(void* obj, int on);                   // 0x42cdb0
    __forceinline void set_view(const float* v);                      // 0x4c5190
    __forceinline void draw_string(const tstring* s, float y, unsigned color, int centered);   // 0x4e43e0
};


// ---- localisation table (0x5e2a18) ---------------------------------------------------------------------
struct StrTable_s2ae70 {
    __forceinline tstring* lookup(const tstring& key);                // 0x43a1e0 (thiscall, ret 4)
};


// ---- rgl::mesh_s2ae70 (0x260 bytes; ctor 0x412a90) ------------------------------------------------------------
namespace rgl {
struct vec2_s2ae70 { float x, y; vec2_s2ae70() {} vec2_s2ae70(float a, float b) : x(a), y(b) {} };

struct mesh_s2ae70 {
    char pad0[0x2c];
    material_s2ae70* res;                                      // +0x2c
    char pad30[0x138 - 0x30];
    char f138;                                          // +0x138 (dirty flag)
    char pad139[0x164 - 0x139];
    char f164;                                          // +0x164 (dirty flag)
    char pad165[0x260 - 0x165];
    __forceinline int add_point(const vector4* p);                    // 0x412f50
    __forceinline int add_vertex(int index, const vec2_s2ae70* uv, const unsigned* color, const vector4* pos,
                   const int* f5c);                     // 0x413c50 (ret 0x14)
    __forceinline int add_face(int a, int b, int c);                  // 0x411920
};
} // namespace rgl

// ---- entity (created by World_s2ae70::create_entity(9)) --------------------------------------------------------
struct Entity_s2ae70 {
    virtual void v0(); virtual void v1(); virtual void v2();
    virtual void v3(); virtual void v4(); virtual void v5();
    virtual void set_scale(const float* s);             // vtable +0x18
    char pad[0x140 - 4];
    unsigned char f140;                                 // +0x140 visible flag
    __forceinline void add_extra(rgl::mesh_s2ae70* m);                        // 0x442110 (thiscall, ret 4)
};

struct World_s2ae70 {
    __forceinline Entity_s2ae70* create_entity(int type);                    // 0x428440 (thiscall, ret 4)
};

// the shape owned by the Screen_s2ae70 at +0x6698 (0x94 bytes; see s004e65e0 / s004e6a70)
struct Shape_s2ae70 {
    char pad[0x94];
    __forceinline void polygon_to_mesh(rgl::mesh_s2ae70* m);                       // 0x4e75a0 (thiscall, ret 4)
    __forceinline void build_ribbons(rgl::mesh_s2ae70* m);                    // 0x4e6a70 (thiscall, ret 4)
    __forceinline void build_spikes(rgl::mesh_s2ae70* m);                     // 0x4e65e0 (thiscall, ret 4)
};

// ---- mission (0x5e292c) and the game clock -------------------------------------------------------------
struct Agent_s2ae70 {
    char pad[0x1c4];
    int kind;                                           // +0x1c4
    char pad1[0x5928 - 0x1c8];
};
struct Mission_s2ae70 {
    char pad0[0x18];
    std::vector<Agent_s2ae70*> pages;                          // +0x18 (std::vector; data at +0x1c)
    char pad2c[0x5c - 0x28];
    int local_id;                                       // +0x5c (-1 when no local agent)
    __forceinline void order(int id, int arg);                        // 0x49c2c0
};


extern int g_a1717c;                                    // 0xa1717c

// ---- the mission screen --------------------------------------------------------------------------------
struct Screen_s2ae70 {
    World_s2ae70* world;                                       // +0
    char pad4[0x29c - 4];
    int f29c;                                           // +0x29c order state
    int f2a0;                                           // +0x2a0 selected order
    char pad2a4[0x2a8 - 0x2a4];
    int f2a8;                                           // +0x2a8 clock index
    char pad2ac[0x2b0 - 0x2ac];
    __int64 f2b0;                                       // +0x2b0 order deadline
    char pad2b8[0x654c - 0x2b8];
    float f654c;                                        // +0x654c marker height
    char pad6550[0x6698 - 0x6550];
    Shape_s2ae70 shape;                                        // +0x6698
    rgl::mesh_s2ae70* f672c;                                   // +0x672c
    rgl::mesh_s2ae70* f6730;                                   // +0x6730
    rgl::mesh_s2ae70* f6734;                                   // +0x6734
    Entity_s2ae70*    f6738;                                   // +0x6738
    rgl::mesh_s2ae70* f673c;                                   // +0x673c
    Entity_s2ae70*    f6740;                                   // +0x6740

    __declspec(noinline) void init_order_markers();                          // 0x52ae70
    __declspec(noinline) void order_tick();                                  // 0x52b300
    void draw_order_menu();                             // 0x52b3c0
};

// shape builders (all take the shape in ecx and the target mesh_s2ae70 on the stack)

// ======================================================= slice s0052b5c0: classes, globals, helpers
#define g_strtable_s2b5c0 (*(StrTable_s2b5c0*)g_strtable)
#define g_gui_s2b5c0 (*(Gui_s2b5c0*)g_gui)
#define g_items_s2b5c0 (*(ItemKind_s2b5c0**)&g_items)
#define g_terrain_s2b5c0 (*(Terrain_s2b5c0*)g_terrain)
#define g_game_s2b5c0 (*(CampaignGame_s2b5c0**)&g_game)
#define g_mission_s2b5c0 (*(Mission_s2b5c0**)&g_mission)
#define g_editor_s2b5c0 (*(Editor_s2b5c0*)g_app_obj)
// Slice s0052b5c0: the mission order and cursor/terrain helpers.
//   0x52b5c0  Mission_s2b5c0::order_agent   -- give an item/order to one of an agent's four order slots
//   0x52ba10  Mission_s2b5c0::update_ui     -- per-frame UI/cursor state machine
//   0x52ba1d  interior continuation of 0x52ba10 (EBX/ZF preset) -- translated in full
//   0x52bc80  Mission_s2b5c0::update_cursor -- ray-pick the ground and drive the terrain brush
//   0x52bdb0  Mission_s2b5c0::find_position -- screen ray -> world position through the collision world
// Layouts and callee signatures are the ones verified in the neighbouring slices
// (s0052ae70 Mission_s2b5c0/Screen, s0052f250 MapEditor, s005191e0 collision shapes, s0052c2b0 brush).

using rgl::vector4;
using rgl::matrix;



// ---- engine string (16 bytes) -------------------------------------------------------------------------


struct StrTable_s2b5c0 {
    __forceinline tstring* lookup(const tstring& key);                // 0x43a1e0 (thiscall, ret 4)
};


// ---- gui / message log (one big object at 0x5d7080) ---------------------------------------------------
struct Gui_s2b5c0 {
    __forceinline void set_o568(void* obj, int on);                   // 0x42cdb0
    __forceinline void set_view(const float* v);                      // 0x4c5190
    __forceinline void add(tstring text, int flag);                   // 0x452c00
};


// ---- item kinds / order references --------------------------------------------------------------------
struct ItemKind_s2b5c0 {
    char pad[0x1c8];
    unsigned kind;                                      // +0x1c8
    char pad2[0x238 - 0x1cc];
};


struct ItemRef_s2b5c0 {
    int id;                                             // +0x00
    int count;                                          // +0x04
    __forceinline int max_amount();                                   // 0x434b70 (thiscall)
    __forceinline void name(char* out);                               // 0x512730 (thiscall, ret 4)
};

// ---- agents -------------------------------------------------------------------------------------------
struct AgentFrame_s2b5c0 {
    char pad[0x58];
    matrix m;                                           // +0x58
};

struct Agent_s2b5c0 {
    char pad0[0x1d0];
    ItemRef_s2b5c0 orders[4];                                  // +0x1d0
    char pad1f0[0x30];
    char active[4];                                     // +0x220
    char pad224[8];
    int f22c;                                           // +0x22c
    int f230;                                           // +0x230
    char pad234[0x1c];
    AgentFrame_s2b5c0* frame;                                  // +0x250
    char pad254[0x5928 - 0x254];
    __forceinline void on_wield_reset();                              // 0x445a20 (thiscall)
};

// ---- collision world (mission->f0) --------------------------------------------------------------------
struct shape_base_s2b5c0 {
    int m4, m8, mc, min_x, min_y, max_x, max_y;
    shape_base_s2b5c0() : m4(-1), m8(8), mc(-1),
                   min_x(100000000), min_y(100000000), max_x(-100000000), max_y(-100000000) {}
    virtual ~shape_base_s2b5c0() {}
    virtual int shape_type();                           // slot 1, 0x408020
    __forceinline void intersect(vector4* out, const vector4* a, const vector4* b);   // 0x51a2d0
};
typedef capsule capsule_s2b5c0;
matrix* identity(matrix* out);                                              // 0x40cd00

struct Scene_s2b5c0 {
    char pad[0x64];
    int f64;                                            // +0x64
    __forceinline void query_segment(const float* a, const float* b, unsigned mask,
                       shape_base_s2b5c0** out, int* count);   // 0x439870
};

struct Camera_s2b5c0 {
    __forceinline void screen_to_world(vector4* near_pt, vector4* far_pt);    // 0x4bd2f0
};

// ---- terrain (0xa76630) -------------------------------------------------------------------------------
struct Terrain_s2b5c0 {
    char pad0[0x60];
    int f60;                                            // +0x60
    char pad64[0x278 - 0x64];
    int f278;                                           // +0x278
    int f27c;                                           // +0x27c
    char pad280[0x288 - 0x280];
    char cells[0xc3b0dc - 0x288];                       // +0x288
    void* f_c3b0dc;                                     // +0xc3b0dc
    float f_c3b0e0;                                     // +0xc3b0e0
    char pad_c3b0e4[0x845d984 - 0xc3b0e4];
    int f845d984;                                       // +0x845d984
    int f845d988;                                       // +0x845d988
    char pad845d98c[0x845e030 - 0x845d98c];
    char f845e030[0x64];                                // +0x845e030
    char f845e094[0x10000];                             // +0x845e094
    void clear();                                       // 0x52c290
    __forceinline void update(int* objs, vector4* pos, int sel, int mode, int flag);  // 0x52bf40
    __forceinline void conv_build_scene();                                       // 0x4c8630 (fastcall free function conv_build_scene(conv_rec*))
    void emit_particle(int layer, int blend, int cx, int cy, float h);     // 0x47ff60
    void load_layer(int* objs, int index);                              // 0x467250
    void brush(int* objs, int* sel, int* col, float* d, int* mode, int* flag, char* place); // 0x52c2b0
};


// ---- the mission / screen object (0x5e292c) -----------------------------------------------------------
struct UiObj_s2b5c0 {
    char pad[0x140];
    char f140;                                          // +0x140
};

struct CampaignGame_s2b5c0 {
    char pad0[0x1a094];
    int game_mode;                                      // +0x1a094
    int f1a098;                                         // +0x1a098
    vector4 color;                                      // +0x1a09c
    int f1a0ac;                                         // +0x1a0ac
    char pad1a0b0[0x1a0c8 - 0x1a0b0];
    char f1a0c8;                                        // +0x1a0c8
    char pad1a0c9[0x1a0d4 - 0x1a0c9];
    float f1a0d4;                                       // +0x1a0d4
    float f1a0d8;                                       // +0x1a0d8
    float f1a0dc;                                       // +0x1a0dc
    char pad1a0e0[0x1a1cc - 0x1a0e0];
    int f1a1cc;                                         // +0x1a1cc
    char pad1a1d0[0x1a20c - 0x1a1d0];
    int f1a20c;                                         // +0x1a20c
};


struct Mission_s2b5c0 {
    Scene_s2b5c0* f0;                                          // +0x00
    char pad4[0x18 - 4];
    std::vector<Agent_s2b5c0*> pages;                          // +0x18 (data pointer at +0x1c)
    char pad28[0x60 - 0x28];
    void* view;                                         // +0x60
    char pad64[0x2b8 - 0x64];
    int f2b8;                                           // +0x2b8
    int f2bc;                                           // +0x2bc
    long long f2c0;                                     // +0x2c0
    int f2c8;                                           // +0x2c8
    int f2cc;                                           // +0x2cc
    char pad2d0[0x2f0 - 0x2d0];
    char f2f0;                                          // +0x2f0
    char pad2f1[0x63d0 - 0x2f1];
    char f63d0[0x651c - 0x63d0];                        // +0x63d0 object array
    int f651c;                                          // +0x651c
    char pad6520[0x65d0 - 0x6520];
    UiObj_s2b5c0* f65d0;                                       // +0x65d0
    char pad65d4[8];
    UiObj_s2b5c0* f65dc;                                       // +0x65dc
    char pad65e0[4];
    UiObj_s2b5c0* f65e4;                                       // +0x65e4
    char pad65e8[0x6738 - 0x65e8];
    UiObj_s2b5c0* f6738;                                       // +0x6738

    __declspec(noinline) bool order_agent(unsigned agent_id, ItemRef_s2b5c0* order); // 0x52b5c0
    __declspec(noinline) void update_ui();                                   // 0x52ba10
    __declspec(noinline) void update_ui_continue();                          // 0x52ba1d (interior continuation)
    void update_cursor();                               // 0x52bc80
    __declspec(noinline) void find_position(char* ok, vector4* pos);         // 0x52bdb0
    __forceinline int add_placed(const int* kind, const int* id, const int* value,
                   const matrix* m, int* spawn_id);     // 0x44f870
    __forceinline void update_map();                                  // 0x52f250
    __forceinline void update_dialog();                               // 0x5305b0
    __forceinline void update_mode4();                                // 0x52cf60
    __forceinline void pick_entity();                                 // 0x4ed8d0 (fastcall free function pick_entity(GuiCtx*))
};


struct Editor_s2b5c0 {
    __forceinline void update_checks();                        // 0x4bf640
    __forceinline void refresh();                                     // 0x4c77c0
};



extern int g_a17160;                                    // 0xa17160
extern int g_a17560;                                    // 0xa17560
extern int g_a171b8;                                    // 0xa171b8
extern int g_a175b8;                                    // 0xa175b8
extern int g_a1745c;                                    // 0xa1745c
extern int g_a1785c;                                    // 0xa1785c
extern int g_use_shaders;                                    // 0x5e28fc

// =====================================================================================================
//  Mission_s2b5c0::order_agent
// =====================================================================================================
// ======================================================= slice s0052bf40: classes, globals, helpers
#define g_game_s2bf40 (*(CampaignGame_s2bf40**)&g_game)
#define g_terrain_s2bf40 (*(Terrain_s2bf40*)g_terrain)
// Slice s0052bf40: the terrain brush/grid update.
//   0x52bf40  Terrain_s2bf40::update   -- clear the brush grid, drive the brush over the grid cells around
//                                 the cursor (0x52c2b0), regenerate the mode-0/1/2 objects and
//                                 rebuild every layer chunk of the touched cells (0x47ff60).
//   0x52c290  Terrain_s2bf40::clear    -- zero the 0x177-byte brush grid at this+0x845e030.
// Layouts follow the neighbouring slices (s0052c2b0 brush, s0052f250 map editor, s0052b5c0 mission).

using rgl::vector4;
using rgl::matrix;



struct CampaignGame_s2bf40 {
    char pad0[0x1a094];
    int game_mode;                                      // +0x1a094
    char pad1a098[0x1a0d4 - 0x1a098];
    float f1a0d4;                                       // +0x1a0d4
    float f1a0d8;                                       // +0x1a0d8
    float f1a0dc;                                       // +0x1a0dc
};

extern int g_use_shaders;                                    // 0x5e28fc

struct Terrain_s2bf40 {
    char pad0[0x60];
    int f60;                                            // +0x60
    char pad64[0x278 - 0x64];
    int f278;                                           // +0x278
    int f27c;                                           // +0x27c
    char pad280[0x288 - 0x280];
    char cells[0xc3b0dc - 0x288];                       // +0x288
    void* f_c3b0dc;                                     // +0xc3b0dc
    float f_c3b0e0;                                     // +0xc3b0e0
    char pad_c3b0e4[0x845d984 - 0xc3b0e4];
    int f845d984;                                       // +0x845d984
    int f845d988;                                       // +0x845d988
    char pad845d98c[0x845e030 - 0x845d98c];
    char f845e030[0x64];                                // +0x845e030
    char f845e094[0x10000];                             // +0x845e094

    __declspec(noinline) void clear();                  // 0x52c290
    __declspec(noinline) void update(int* objs, vector4* pos, int sel, int mode, int flag);  // 0x52bf40
    __forceinline void load_layer(int* objs, int index);                              // 0x467250
    __forceinline void brush(int* objs, int* sel, int* col, float* d, int* mode, int* flag, char* place); // 0x52c2b0
    __forceinline void emit_particle(int layer, int blend, int cx, int cy, float h);     // 0x47ff60
};


// =====================================================================================================
//  Terrain_s2bf40::update
// =====================================================================================================
// ======================================================= slice s0052c2b0: classes, globals, helpers

// Slice s0052c2b0: the terrain "brush" of the world-map editor (0x52c2b0, 2887 bytes, __thiscall,
// ret 0x1c = 7 stack args). It maps the brush amount (arg4) through the per-map-mode response curve
// selected by g_game_c->game_mode (+0x1a094), then either rewrites the height/colour values of one or
// many map tiles (the 24-byte "element" records reached from arg1, whose per-cell value array at +0x14
// is a 0xac440-byte lazily allocated block) or marks the touched grid cells in this[0x845e030].
// Reconstruction of the original control flow (verified against the disassembly); NOT a byte-exact
// match: the original's x87 evaluation order and the compiler's 4x unrolling of the 0.98 scan are
// reproduced only structurally. See nonmatching.txt.

typedef unsigned int uint;

// ---------------------------------------------------------------------------------------------------
// globals (addresses from the annotations in the disassembly)
#define g_game_c ((char*)g_game)
extern float g_00;                   // 0x549ca8  0.0f
extern float g_10;                   // 0x549cdc  1.0f
extern float g_098;                  // 0x5911d8  0.98f (scan threshold)
extern double g_001;                 // 0x59c480  unused here (other slices)

void* __cdecl operator new[](uint size);      // 0x535482  ??_U@YAPAXI@Z operator new[]
unsigned __cdecl pack_color(const float* c);           // 0x40e440

// ---------------------------------------------------------------------------------------------------
// brush element record: 24 bytes. The array header (arg1) is {int count; ...}, element i starts at
// (char*)arg1 + 0xc + i*0x18 and holds the per-cell value pointer at +0x14.
struct BrushElem_s2c2b0 {
    int    f0, f1, f2, f3, f4;
    float* v;                        // +0x14  0xac440-byte value block (0x1a4*0x2a1 dwords), or 0
    __forceinline void clear();                     // 0x52ce00  clear the whole block
    __forceinline void put(int* a, int* b, int* value);   // 0x52ce70  lazy-alloc + store one cell
};

struct SceneBrush_s2c2b0 {
    char pad0[0x278];
    int  w;                          // 0x278  grid width
    int  h;                          // 0x27c  grid height
    char pad280[0x30c - 0x280];
    // per-cell terrain records (0x94 bytes, x stride 0xfb rows) live at +0x30c; two floats used.
    __forceinline void clamp(int* out, int* in);   // 0x52cef0  scale a cell coordinate into the grid
    __declspec(noinline) void brush(int* elems, int* pidx, int* cell, float* amt, int* op, int* fA, char* fB);  // 0x52c2b0
};

// ======================================================= slice s0052ce00: classes, globals, helpers

// Slice s0052ce00: the per-cell value block helpers of the world-map terrain brush.
//   0x52ce00  clear the 0xac440-byte block (0x1a4 x 0x1a4 floats) of one brush element.
//   0x52ce70  lazily allocate that block (filled with -1) and store one cell.
//   0x52cef0  scale a grid coordinate pair into the scene's cell range (divide by the cell size at
//             this+0x843bd70, clamp to [0, size-1] using this+0x845d984 / this+0x845d988).
// The element record is 24 bytes; its block pointer lives at +0x14 and the block is read as float
// cells by the brush (0x52c2b0) and written as int -1 when it is first allocated. Layout and offsets
// are read off this slice's disassembly.

void* __cdecl operator new[](unsigned _Size);          // 0x535482  operator new[] (??_U@YAPAXI@Z)

struct Elem_s2ce00 {
    int f0;
    int f1;
    int f2;
    int f3;
    int f4;
    float* v;                                            // +0x14  0xac440-byte value block
    __declspec(noinline) void clear();                                        // 0x52ce00
    __declspec(noinline) void put(int* ix, int* iy, int* value);              // 0x52ce70
};

struct Scene_s2ce00 {
    __declspec(noinline) int* clamp(int* out, int* in);                       // 0x52cef0
};




// ---- sig s1: canonical declarations (minimal) and forwarders from the per-slice view classes to them.
namespace rgl {
vector4 unpack_color(const unsigned* argb);   // 0x40e4e0
}
struct conv_rec; struct GuiCtx;
void __fastcall conv_build_scene(conv_rec*);   // 0x4c8630
void __fastcall pick_entity(GuiCtx*);   // 0x4ed8d0
struct SlotChan {
    void play(int);   // 0x4875e0
};
struct UiD {
    void set_value(float);   // 0x48baa0
};
struct level {
    void query_segment(const float*, const float*, unsigned, shape_base**, int*);   // 0x439870
};
struct RecZSubB {
    RecZSubB* init();   // 0x4353a0
};
struct Agent; struct AgentTable; struct AgentX; struct BlockOwner; struct Blow; struct ContainerBase; struct ConvScreen; struct CreateGameOptionsPage; struct DialogMgr; struct ForceLabel; struct Game; struct GameData; struct GuiCtx; struct ItemRef; struct LangTables; struct Local; struct Map10007; struct MapEditorDialog; struct MapEditor_s2cf60; struct MapEditor_s2f250; struct MapFlags; struct MapObject; struct Mission; struct MissionG; struct MissionHud; struct MissionSource; struct MsgLog; struct NamedRec; struct OriginPairs; struct Party; struct PartyRecIO; struct Pickable; struct Picker; struct PlacedObj; struct PlaneTrace; struct PolyMesh; struct Poser; struct RecZSubB; struct Scene; struct Shape; struct Slot; struct SlotChan; struct StrTable; struct TerrainX; struct Timer; struct UiD; struct WindowMgrW; struct camera_ray; struct capsule; struct contact; struct conv_rec; struct gui_manager; struct gui_obj; struct gui_view; struct level; struct pos; struct shape_base; struct skin; struct snd_bank; struct snd_slot; struct stream_base; struct terrain_obj; struct tstring; struct vec2;
namespace rgl { struct entity; }

struct ItemRef;
struct Agent {
    int pick_attack_dir(unsigned);   // 0x43f970
    unsigned pick_front();   // 0x43b9e0
    void cycle_slot(int);   // 0x44bc30
    void note_blow(Blow*);   // 0x457af0
    void on_wield_reset();   // 0x445a20
    ItemRef slot_pair(int) const;   // 0x432790
};
struct AgentTable {
    int first_used();   // 0x43b1b0
    void next_used(unsigned*);   // 0x43b160
};
struct AgentX {
    bool f100_is_0x48_and_charged() const;   // 0x432470
    bool is_enemy(Agent const*) const;   // 0x432350
    bool is_friend(Agent const*) const;   // 0x4323a0
    float speed_scale() const;   // 0x432450
};
struct BlockOwner {
    void init(ContainerBase*, int);   // 0x4dd920
};
struct ConvScreen {
    void refresh(int, int*);   // 0x4f73c0
};
struct CreateGameOptionsPage {
    void update_checks();   // 0x4bf640
};
struct DialogMgr {
    int listener();   // 0x460670
    int speaker();   // 0x460640
    void start(int, int, int, int, char);   // 0x4610b0
};
struct ForceLabel {
    void make_label(char*);   // 0x48d940
};
struct Game {
    void more_start();   // 0x51f800
};
struct GameData {
    char const* module_path(char const*);   // 0x4639d0
    int skill_level(int, int);   // 0x437b20
    tstring scene_path(int);   // 0x463950
};
struct ItemRef {
    int item;
    unsigned m4;
    int max_amount() const;   // 0x434b70
    int stat_1ec();   // 0x434c50
    void name(char*);   // 0x512730
};   // layout of src/game/item_ref.h ItemRef (8 bytes: item, m4); that header clashes with this file's g_items
struct LangTables {
    tstring* find(tstring const*);   // 0x4c6010
};
struct Local {
    void read(stream_base&);   // 0x524f00
};
struct Map10007 {
    int lookup(unsigned);   // 0x48ad00
};
struct MapEditorDialog {
    void refresh();   // 0x4c77c0
    void update();   // 0x5305b0
};
struct MapEditor_s2cf60 {
    void update();   // 0x52cf60
};
struct MapEditor_s2f250 {
    void update();   // 0x52f250
};
struct MapFlags {
    void commit(char*);   // 0x513710
};
struct MapObject {
    tstring* mesh_ptr();   // 0x4c6ca0
};
struct Mission {
    char mode_flag1() const;   // 0x435840
    void camera_to(rgl::matrix const*, float const*);   // 0x4fa790
    int add_placed(const int*, const int*, const int*, rgl::matrix const*, int*);   // 0x44f870
    void refresh_agent(Agent*);   // 0x4f7e00
};
struct MissionG {
    void frame(float, bool);   // 0x483f60
};
struct MissionHud {
    void update_camera(float);   // 0x4f8c80
    void update_equipment();   // 0x4f8b40
    void update_status();   // 0x4fb350
};
struct MissionSource {
    void pick(int*);   // 0x4d5410
};
struct MsgLog {
    void add(tstring, int);   // 0x452c00
    void flush();   // 0x4f1390
};
struct NamedRec {
    void setup2(tstring const*, tstring const*, tstring const*, int);   // 0x48a3d0
};
struct OriginPairs {
    bool clicked(int);   // 0x48ac90
    bool is_down(int);   // 0x48ac50
};
struct Party {
    int skill_level(int);   // 0x4b2370
    void add_xp_to_stack(int, int);   // 0x4b1d80
};
struct Picker {
    void collect(unsigned, Pickable**, int*);   // 0x51a3a0
};
struct PlaneTrace {
    void intersect(rgl::vector4*, rgl::vector4 const*, rgl::vector4 const*);   // 0x51a2d0
};
struct PolyMesh {
    void build_geometry();   // 0x470a00
};
struct Poser {
    void add_extra(int);   // 0x442110
};
struct Scene {
    PartyRecIO* get(int);   // 0x51ee50
};
struct Shape {
    void build_ribbons(rgl::mesh*);   // 0x4e6a70
    void build_spikes(rgl::mesh*);   // 0x4e65e0
    void polygon_to_mesh(rgl::mesh*);   // 0x4e75a0
};
struct Slot {
    int is_ready(int);   // 0x487900
};
struct StrTable {
    tstring* lookup(tstring const&);   // 0x43a1e0
};
struct TerrainX {
    void emit_particle(int, int, int, int, float);   // 0x47ff60
    void light_entities(rgl::mesh**, int);   // 0x47ce00
};
struct Timer {
    float elapsed() const;   // 0x432c10
};
struct WindowMgrW {
    void open(int);   // 0x494260
    void set_state(int);   // 0x495700
};
namespace c49c2c0 {
struct Mission {
    void order(int, int);   // 0x49c2c0
};
}
struct camera_ray {
    void screen_to_world(rgl::vector4*, rgl::vector4*);   // 0x4bd2f0
};
struct gui_manager {
    void draw_string(tstring*, float, int, char);   // 0x4e43e0
    void set_o568(gui_obj*, bool);   // 0x42cdb0
};
struct gui_view {
    void set_view(float const*);   // 0x4c5190
};
namespace rgl {
struct world {
    rgl::entity* create_entity(int);   // 0x428440
    void remove_entity(int);   // 0x41f0d0
};
}
struct snd_bank {
    void update_all(int);   // 0x462760
};
struct snd_slot {
    void update(int);   // 0x4625d0
};
struct terrain_obj {
    void load_layer(int*, int);   // 0x467250
};
__forceinline void Gui_s2b5c0::add(tstring a0, int a1) { ((MsgLog*)this)->add(a0, a1); }
__forceinline void MsgLog_s261e0::add(tstring a0, int a1) { ((MsgLog*)this)->add(a0, a1); }
__forceinline void Entity_s2ae70::add_extra(rgl::mesh_s2ae70* a0) { ((Poser*)this)->add_extra((int)a0); }
__forceinline int rgl::mesh_s2ae70::add_face(int a0, int a1, int a2) { return ((rgl::mesh*)this)->add_face(a0, a1, a2); }
__forceinline int rgl::mesh_s2ae70::add_point(rgl::vector4 const* a0) { return ((rgl::mesh*)this)->add_point(a0); }
__forceinline void Party_s29840::add_xp_to_stack(int a0, int a1) { ((Party*)this)->add_xp_to_stack(a0, a1); }
__forceinline void Terrain_s2bf40::brush(int* a0, int* a1, int* a2, float* a3, int* a4, int* a5, char* a6) { ((SceneBrush_s2c2b0*)this)->brush(a0, a1, a2, a3, a4, a5, a6); }
__forceinline void MissionSub_s261e0::build_geometry() { ((PolyMesh*)this)->build_geometry(); }
__forceinline void Shape_s2ae70::build_ribbons(rgl::mesh_s2ae70* a0) { ((Shape*)this)->build_ribbons((rgl::mesh*)a0); }
__forceinline void Shape_s2ae70::build_spikes(rgl::mesh_s2ae70* a0) { ((Shape*)this)->build_spikes((rgl::mesh*)a0); }
__forceinline void Mission_s257f0::camera_to(rgl::matrix* a0, float& a1) { ((Mission*)this)->camera_to((rgl::matrix const*)a0, (float const*)&a1); }
__forceinline void SceneBrush_s2c2b0::clamp(int* a0, int* a1) { ((Scene_s2ce00*)this)->clamp(a0, a1); }
__forceinline void BrushElem_s2c2b0::clear() { ((Elem_s2ce00*)this)->clear(); }
__forceinline char KeyTable_s261e0::clicked(int a0) { return (char)((OriginPairs*)this)->clicked(a0); }
__forceinline char KeyTable_s284a0::clicked(int a0) { return (char)((OriginPairs*)this)->clicked(a0); }
__forceinline void Scene_s29ec0::collect(int a0, SceneEntity_s29ec0** a1, int* a2) { ((Picker*)this)->collect((unsigned)a0, (Pickable**)a1, a2); }
__forceinline void Flags5e291c_s261e0::commit(char* a0) { ((MapFlags*)this)->commit(a0); }
__forceinline Entity_s2ae70* World_s2ae70::create_entity(int a0) { return (Entity_s2ae70*)((rgl::world*)this)->create_entity(a0); }
__forceinline void Agent_s284a0::cycle_slot(int a0) { ((Agent*)this)->cycle_slot(a0); }
__forceinline void gui_manager_s29ec0::draw_string(tstring const* a0, float a1, unsigned a2, int a3) { ((gui_manager*)this)->draw_string((tstring*)a0, a1, (int)a2, (char)a3); }
__forceinline void gui_manager_s2ae70::draw_string(tstring const* a0, float a1, unsigned a2, int a3) { ((gui_manager*)this)->draw_string((tstring*)a0, a1, (int)a2, (char)a3); }
__forceinline float MissionSub_s261e0::elapsed() const { return ((Timer*)this)->elapsed(); }
__forceinline void Terrain_s2bf40::emit_particle(int a0, int a1, int a2, int a3, float a4) { ((TerrainX*)this)->emit_particle(a0, a1, a2, a3, a4); }
__forceinline bool Agent_s29130::f100_is_0x48_and_charged() const { return ((AgentX*)this)->f100_is_0x48_and_charged(); }
__forceinline void Screen_s261e0::f_529ec0() { ((Screen_s29ec0*)this)->f_529ec0(); }
__forceinline tstring* StrTable_s29ec0::find(tstring const* a0) { return ((LangTables*)this)->find(a0); }
__forceinline int AgentTable_s257f0::first_used() { return ((AgentTable*)this)->first_used(); }
__forceinline int AgentTable_s26130::first_used() { return ((AgentTable*)this)->first_used(); }
__forceinline int AgentTable_s261e0::first_used() { return ((AgentTable*)this)->first_used(); }
__forceinline int AgentTable_s284a0::first_used() { return ((AgentTable*)this)->first_used(); }
__forceinline unsigned AgentTable_s29840::first_used() { return (unsigned)((AgentTable*)this)->first_used(); }
__forceinline void MsgLog_s261e0::flush() { ((MsgLog*)this)->flush(); }
__forceinline void Mission_s29130::fn_529840(int a0) { ((Mission_s29840*)this)->fn_529840(a0); }
__forceinline void Mission_s261e0::fn_529d80() { ((Mission_s29840*)this)->fn_529d80(); }
__forceinline void Mission_s261e0::frame(float a0, char a1) { ((MissionG*)this)->frame(a0, (bool)a1); }
__forceinline Party_s29840* PartyMgr_s29840::get(int a0) { return (Party_s29840*)((Scene*)this)->get(a0); }
__forceinline material_s2ae70* resmgr_s2ae70::get_material(tstring* a0) { return (material_s2ae70*)((resource_manager*)this)->get_material(*(rgl::string const*)a0); }
__forceinline int PartyMgr_s29130::index_of(unsigned a0) { return ((Map10007*)this)->lookup(a0); }
__forceinline int PartyMgr_s29840::index_of(unsigned a0) { return ((Map10007*)this)->lookup(a0); }
__forceinline void Src_s261e0::init(int a0, int a1) { ((BlockOwner*)this)->init((ContainerBase*)a0, a1); }
__forceinline void Screen_s261e0::init_order_markers() { ((Screen_s2ae70*)this)->init_order_markers(); }
__forceinline void shape_base_s2b5c0::intersect(rgl::vector4* a0, rgl::vector4 const* a1, rgl::vector4 const* a2) { ((PlaneTrace*)this)->intersect(a0, a1, a2); }
__forceinline char KeyTable_s284a0::is_down(int a0) { return (char)((OriginPairs*)this)->is_down(a0); }
__forceinline bool Agent_s261e0::is_enemy(Agent_s261e0 const* a0) const { return ((AgentX*)this)->is_enemy((Agent const*)a0); }
__forceinline bool Agent_s284a0::is_enemy(Agent_s284a0* a0) { return ((AgentX*)this)->is_enemy((Agent const*)a0); }
__forceinline bool Agent_s29ec0::is_enemy(Agent_s29ec0* a0) { return ((AgentX*)this)->is_enemy((Agent const*)a0); }
__forceinline bool Agent_s284a0::is_friend(Agent_s284a0* a0) { return ((AgentX*)this)->is_friend((Agent const*)a0); }
__forceinline bool Agent_s29ec0::is_friend(Agent_s29ec0* a0) { return ((AgentX*)this)->is_friend((Agent const*)a0); }
__forceinline char KeyNames_s284a0::is_ready(int a0) { return (char)((Slot*)this)->is_ready(a0); }
__forceinline char KeyReady_s261e0::is_ready(int a0) { return (char)((Slot*)this)->is_ready(a0); }
__forceinline int DialogMgr_s26130::listener() { return ((DialogMgr*)this)->listener(); }
__forceinline void Terrain_s2bf40::load_layer(int* a0, int a1) { ((terrain_obj*)this)->load_layer(a0, a1); }
__forceinline tstring* StrTable_s261e0::lookup(tstring* a0) { return ((StrTable*)this)->lookup(*(tstring const*)a0); }
__forceinline tstring* StrTable_s29ec0::lookup(tstring const& a0) { return ((StrTable*)this)->lookup(a0); }
__forceinline tstring* StrTable_s2ae70::lookup(tstring const& a0) { return ((StrTable*)this)->lookup(a0); }
__forceinline tstring* StrTable_s2b5c0::lookup(tstring const& a0) { return ((StrTable*)this)->lookup(a0); }
__forceinline void LabelSub_s261e0::make_label(char* a0) { ((ForceLabel*)this)->make_label(a0); }
__forceinline int ItemRef_s2b5c0::max_amount() { return ((ItemRef*)this)->max_amount(); }
__forceinline tstring* MapObject_s29ec0::mesh_ptr() { return ((MapObject*)this)->mesh_ptr(); }
__forceinline bool Mission_s261e0::mode_flag1() const { return (bool)((Mission*)this)->mode_flag1(); }
__forceinline char const* GameData_s27c90::module_path(char const* a0) { return ((GameData*)this)->module_path(a0); }
__forceinline void GameData_s29130::more_start() { ((Game*)this)->more_start(); }
__forceinline void ItemRef_s29ec0::name(char* a0) { ((ItemRef*)this)->name(a0); }
__forceinline void ItemRef_s2b5c0::name(char* a0) { ((ItemRef*)this)->name(a0); }
__forceinline void AgentTable_s257f0::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void AgentTable_s26130::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void AgentTable_s261e0::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void AgentTable_s284a0::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void AgentTable_s29840::next_used(unsigned* a0) { ((AgentTable*)this)->next_used(a0); }
__forceinline void Agent_s284a0::note_blow(Blow_s284a0* a0) { ((Agent*)this)->note_blow((Blow*)a0); }
__forceinline void Agent_s284a0::on_wield_reset() { ((Agent*)this)->on_wield_reset(); }
__forceinline void Agent_s2b5c0::on_wield_reset() { ((Agent*)this)->on_wield_reset(); }
__forceinline void MainMenu_s261e0::open(int a0) { ((WindowMgrW*)this)->open(a0); }
__forceinline void Mission_s2ae70::order(int a0, int a1) { ((c49c2c0::Mission*)this)->order(a0, a1); }
__forceinline void Screen_s261e0::order_tick() { ((Screen_s2ae70*)this)->order_tick(); }
__forceinline void Src_s261e0::pick(int* a0) { ((MissionSource*)this)->pick(a0); }
__forceinline int Agent_s29130::pick_attack_dir(unsigned a0) { return ((Agent*)this)->pick_attack_dir(a0); }
__forceinline int Agent_s29130::pick_front() { return (int)((Agent*)this)->pick_front(); }
__forceinline void Shape_s2ae70::polygon_to_mesh(rgl::mesh_s2ae70* a0) { ((Shape*)this)->polygon_to_mesh((rgl::mesh*)a0); }
__forceinline void BrushElem_s2c2b0::put(int* a0, int* a1, int* a2) { ((Elem_s2ce00*)this)->put(a0, a1, a2); }
__forceinline void MissionSub_s27c90::read(stream_base* a0) { ((Local*)this)->read(*(stream_base*)a0); }
__forceinline void Editor_s2b5c0::refresh() { ((MapEditorDialog*)this)->refresh(); }
__forceinline void Game_s25160::refresh(int a0, int* a1) { ((ConvScreen*)this)->refresh(a0, a1); }
__forceinline void Mission_s261e0::release_item(int a0) { ((Mission_s27c90*)this)->release_item(a0); }
__forceinline void MatrixExt_s284a0::rotate_fu(float* a0) { ((rgl::matrix*)this)->rotate_about_s((float const*)a0); }
__forceinline void MatrixExt_s284a0::rotate_sf(float* a0) { ((rgl::matrix*)this)->rotate_about_u((float const*)a0); }
__forceinline void GameData_s261e0::save_scene(int a0) { ((GameData_s27c90*)this)->save_scene(a0); }
__forceinline void Mission_s261e0::save_scene_data() { ((Mission_s27c90*)this)->save_scene_data(); }
__forceinline tstring GameData_s27c90::scene_path(int a0) { return ((GameData*)this)->scene_path(a0); }
__forceinline void Camera_s2b5c0::screen_to_world(rgl::vector4* a0, rgl::vector4* a1) { ((camera_ray*)this)->screen_to_world(a0, a1); }
__forceinline void Gui_s2b5c0::set_o568(void* a0, int a1) { ((gui_manager*)this)->set_o568((gui_obj*)a0, (bool)a1); }
__forceinline void MsgLog_s261e0::set_o568(int a0, int a1) { ((gui_manager*)this)->set_o568((gui_obj*)a0, (bool)a1); }
__forceinline void gui_manager_s2ae70::set_o568(void* a0, int a1) { ((gui_manager*)this)->set_o568((gui_obj*)a0, (bool)a1); }
__forceinline void MainMenu_s261e0::set_state(int a0) { ((WindowMgrW*)this)->set_state(a0); }
__forceinline void Gui_s2b5c0::set_view(float const* a0) { ((gui_view*)this)->set_view(a0); }
__forceinline void MsgLog_s261e0::set_view(float* a0) { ((gui_view*)this)->set_view((float const*)a0); }
__forceinline void gui_manager_s29ec0::set_view(float const* a0) { ((gui_view*)this)->set_view(a0); }
__forceinline void gui_manager_s2ae70::set_view(float const* a0) { ((gui_view*)this)->set_view(a0); }
__forceinline void MainMenu_s261e0::setup2(tstring* a0, tstring* a1, tstring* a2, int* a3) { ((NamedRec*)this)->setup2((tstring const*)a0, (tstring const*)a1, (tstring const*)a2, (int)a3); }
__forceinline int GameData_s29ec0::skill_level(int a0, int a1) { return ((GameData*)this)->skill_level(a0, a1); }
__forceinline int Party_s29840::skill_level(int a0) { return ((Party*)this)->skill_level(a0); }
__forceinline int DialogMgr_s257f0::speaker() { return ((DialogMgr*)this)->speaker(); }
__forceinline float Agent_s257f0::speed_scale() const { return ((AgentX*)this)->speed_scale(); }
__forceinline float Agent_s29ec0::speed_scale() { return ((AgentX*)this)->speed_scale(); }
__forceinline void DialogMgr_s261e0::start(int a0, int a1, int a2, int a3, char a4) { ((DialogMgr*)this)->start(a0, a1, a2, a3, a4); }
__forceinline int ItemRef_s29ec0::stat_1ec() { return ((ItemRef*)this)->stat_1ec(); }
__forceinline void vec3_s257f0::sub_assign(vec3_s257f0 const& a0) { ((Vec3*)this)->sub_assign((Vec3 const&)a0); }
__forceinline void Game_s25160::update() { ((ViewObj_s257f0*)this)->update(); }
__forceinline void Screen_s261e0::update(float a0) { ((ViewCtrl_s284a0*)this)->update(a0); }
__forceinline void SoundSlot_s261e0::update(int a0) { ((snd_slot*)this)->update(a0); }
__forceinline void Terrain_s2b5c0::update(int* a0, rgl::vector4* a1, int a2, int a3, int a4) { ((Terrain_s2bf40*)this)->update(a0, a1, a2, a3, a4); }
__forceinline void SoundBank_s261e0::update_all(int a0) { ((snd_bank*)this)->update_all(a0); }
__forceinline void Screen_s261e0::update_camera(float a0) { ((MissionHud*)this)->update_camera(a0); }
__forceinline void Editor_s2b5c0::update_checks() { ((CreateGameOptionsPage*)this)->update_checks(); }
__forceinline void Mission_s2b5c0::update_dialog() { ((MapEditorDialog*)this)->update(); }
__forceinline void Screen_s261e0::update_equipment() { ((MissionHud*)this)->update_equipment(); }
__forceinline void ViewCtrl_s284a0::update_equipment() { ((MissionHud*)this)->update_equipment(); }
__forceinline void Mission_s2b5c0::update_map() { ((MapEditor_s2f250*)this)->update(); }
__forceinline void Screen_s261e0::update_marks() { ((MissionHud_s29130*)this)->update_marks(); }
__forceinline void Mission_s2b5c0::update_mode4() { ((MapEditor_s2cf60*)this)->update(); }
__forceinline void Screen_s261e0::update_status() { ((MissionHud*)this)->update_status(); }
__forceinline void Screen_s261e0::update_ui() { ((Mission_s2b5c0*)this)->update_ui(); }

__forceinline ItemRef Agent_s284a0::slot_pair(int a0) { return ((Agent*)this)->slot_pair(a0); }
__forceinline ItemRef Agent_s29130::slot_pair(int a0) const { return ((Agent*)this)->slot_pair(a0); }
__forceinline Blow_s284a0::Blow_s284a0() { ((RecZSubB*)this)->init(); }
__forceinline int rgl::mesh_s2ae70::add_vertex(int a0, const vec2_s2ae70* a1, const unsigned* a2, const vector4* a3, const int* a4) { return ((rgl::mesh*)this)->add_vertex(a0, (const vec2*)a1, a2, a3, a4); }
__forceinline int Mission_s2b5c0::add_placed(const int* a0, const int* a1, const int* a2, const matrix* a3, int* a4) { return ((Mission*)this)->add_placed(a0, a1, a2, a3, a4); }
__forceinline void Scene_s29ec0::query_segment(const float* a0, const float* a1, unsigned a2, shape_base_s29ec0** a3, int* a4) { ((level*)this)->query_segment(a0, a1, a2, (shape_base**)a3, a4); }
__forceinline void Scene_s2b5c0::query_segment(const float* a0, const float* a1, unsigned a2, shape_base_s2b5c0** a3, int* a4) { ((level*)this)->query_segment(a0, a1, a2, (shape_base**)a3, a4); }
__forceinline void Mgr_s25160::update() { ((MsgLog*)this)->flush(); }
__forceinline void Dlg_s25160::open(int a0) { ((WindowMgrW*)this)->set_state(a0); }
__forceinline void EmitSub_s261e0::play(int a0) { ((SlotChan*)this)->play(a0); }
__forceinline void World_s27c90::remove_entity(int a0) { ((rgl::world*)this)->remove_entity(a0); }
__forceinline void Terrain_s2b5c0::conv_build_scene() { ::conv_build_scene((conv_rec*)this); }
__forceinline void Mission_s2b5c0::pick_entity() { ::pick_entity((GuiCtx*)this); }

#endif // MISSION5_MISSION_5_H
