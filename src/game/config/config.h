// config.h: classes of config.cpp (0x4b9e70..0x4be730): the launcher's create-game page, user folders, the Direct3D
// start-up display-mode choice (Core, ./core.h), the option block (Config, global at 0x5a9f98) and its file
// reader, the software cursor, the DirectInput device set, the render batch's vertex/index buffer table, the
// ambient particle ring (a view of the UI root), the camera ray, the ini-file reader, and the module-file record
// readers (trigger / particle system / presentation / sound / menu / script records, the music player).
// Every offset is read or written by a function of this module (cited by address). ALL NAMES PROVISIONAL.
// Streams, the engine log and the string class come from ../strat/gui_manager.h (src/rgl/stream.h) and ../tstring.h.
#ifndef GAME_CONFIG_CONFIG_H
#define GAME_CONFIG_CONFIG_H
#ifndef _MBCS
#define _MBCS
#endif
#include <windows.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "./core.h"
#include "../tstring.h"
#include "../strat/gui_manager.h"

#include "./config_block.h"   // Config, g_cfg (0x5a9f98)


// ---- globals ---------------------------------------------------------------------------------------------------
#include "../../include/renderer.h"            // g_render 0xa17910 (dev at +0x618 = 0xa17f28)
extern int g_edit_key;                              // 0x5e28d0 first value of the line being edited
extern int g_edit_value;                            // 0x5e28d4 second value of the line being edited
#include "../../include/clock_table.h"   // g_ticks (the clock table)

void __cdecl refresh_settings_list(const tstring* page, char detailed);        // 0x4bb3f0
void set_list_scroll(int delta);                    // 0x4bb260
void pump_messages();                               // 0x4bb310
void set_debug_label();                             // 0x4bb390
void set_progress_range(int low, int high);         // 0x4bb3b0
bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format);   // 0x4bb9c0
void __fastcall unescape(tstring* s);               // 0x4bac70

// ---- a bone's collision capsule (0x4bc170, 0x4bc1f0; this header called it the software cursor): CapDef ---------------
#include "../../rgl/core/bone.h"

// ---- input state and device set (0x4bc2d0..0x4bc430) -------------------------------------------------------------
struct Matrix { float m[16]; };
namespace rgl { struct matrix; } typedef rgl::matrix pos;                                          // src/include/mat4.h: the frame type the definition of 0x40cd00 names (config.cpp declares the function)
struct InputSet {
    IDirectInput8A* dinput;                         // +0x000 0x4bc430
    IDirectInputDevice8A* keyboard;                 // +0x004
    IDirectInputDevice8A* mouse;                    // +0x008
    int unk[0x1f8 / 4];                             // +0x00c
    int mouse_buttons[0x14 / 4];                    // +0x204
    float scroll_delta;                             // +0x218
    int unk2[0x3f8 / 4];                            // +0x21c
    unsigned char exclusive_ok;                     // +0x614 0x4bc350
    unsigned char background_ok;                    // +0x615
    int unk618;                                     // +0x618
    int unk3[0x130 / 4];                            // +0x61c
    int unk74c;                                     // +0x74c
    int unk750;                                     // +0x750
};
struct Slot;
void __fastcall release_input_devices(Slot* self);  // 0x4bc3d0

// ---- the render batch's buffer table (0x4bc540..0x4bd0c0): dev_buffer / buffer_pair
#include "../dev_buffer.h"
#include "../../rgl/resource.h"          // resmgr g_resmgr 0x5d6fa0 (the one declaration)
#include "../render_batch.h"                   // render_batch (shared with terrain_b)
#include "../../include/render_flags.h"

// ---- the screen ray 0x4bd2f0 is MapWidget::screen_to_world (../map_widget.h) ----

// ---- ini file reader (0x4bd4d0..0x4bd740) --------------------------------------------------------------------------
#include "../ini_file.h"   // ini_entry / ini_file (0x4bd4d0..0x4bd740), one definition

// ---- script blocks and the module-file records (0x4bd7a0..0x4be6f0) --------------------------------------------------
// The script block and its 0x98-byte operation record are the shared DialogBlock / Elem (read 0x4607e0, Elem::Elem 0x4607d0,
// copy constructor 0x4bd7a0, defined in config.cpp): one definition, src/include/dialog_rec.h.
#include "../../include/dialog_rec.h"
struct TriggerRec;                                 // include/trigger_rec.h: the module trigger (0x78 bytes; its read 0x4bd830 is defined in config.cpp)
#include "../../include/trigger_entry.h"   // trigger_entry (the simple trigger, 0x30 bytes)

// (the particle-system record is EmitDef, ../emit.h; its load is 0x4bd990, defined below)
// One record of g_meshes (include/mesh_table.h; Data\meshes.txt: "mesh_id flags resource x y z rx ry rz sx sy sz"): the
// placement the scene editor (MeshTable::placed 0x499050) and set_background_mesh apply to the resource.
struct mesh_record {                                // 0x48 bytes
    tstring resource;                               // +0x00 the resource mesh (resmgr get_object)
    int flags;                                      // +0x10 bit 0: the mesh gets the 0x2000 sort flag
    tstring id;                                     // +0x14 the module's mesh id (read first)
    float pos[3];                                   // +0x24
    float rot[3];                                   // +0x30 degrees about s, f, u
    float scale[3];                                 // +0x3c
    void load(stream_base& a);                      // 0x4bdf60
};
#include "../../include/prsnt_table.h"           // PresentationTable (load_used 0x4bdec0)

#include "../music_player.h"            // music_player, g_music (0xa73a10)

#include "../menu_rec.h"                         // menu_item (0x68 bytes: read 0x4be310, ctor 0x4be420), MenuRec
#include "../../include/rec34.h"                 // rec34 (shared with rgl/mesh2/gui_edit.h)
#include "../../include/ground_type.h"           // GroundType (the 0x20-byte ground-spec record; GroundType::read 0x4be600 is below)
struct clip_slot {
    tstring name;                                   // +0x00
    int     v10;                                    // +0x10
    int     v14;                                    // +0x14
    clip_slot();                                    // 0x4be720
};
// The constructor / destructor 0x4be6b0 / 0x4be6f0 that follow clip_slot's are the terrain description's (terrain_desc.h).

#endif // GAME_CONFIG_CONFIG_H
