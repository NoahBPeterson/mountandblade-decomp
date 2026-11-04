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
extern const char g_eol[];                          // 0x54b118 ""

void __cdecl refresh_settings_list(const tstring* page, char detailed);        // 0x4bb3f0
void set_list_scroll(int delta);                    // 0x4bb260
void pump_messages();                               // 0x4bb310
void set_debug_label();                             // 0x4bb390
void set_progress_range(int low, int high);         // 0x4bb3b0
bool __cdecl save_screenshot(const char* file_name, D3DXIMAGE_FILEFORMAT format);   // 0x4bb9c0
void __fastcall unescape(tstring* s);               // 0x4bac70

// ---- the create-game options page (0x4b9e70) ---------------------------------------------------------------------
struct GameConfig {
    char pad0[0x1a060];
    int  debug_file;                                // +0x1a060 the script debugger's file slot (cleared by 0x4bf870 when a script file is missing)
    char pad_d[4];                                  // +0x1a064
    int  paused;                                    // +0x1a068 "Stop"/"Debug" toggle of the script debugger  0x4bf870
    tstring script_text;                            // +0x1a06c text of the selected script    0x4bf870
    int  script_index;                              // +0x1a07c selected index (CB_GETITEMDATA)   0x4bf870
    tstring settings_text;                          // +0x1a080 the settings page text    0x4bb3f0 (the script debugger's module_scripts.py text, 0x4bf870)
    char pad_g[4];                                  // +0x1a090
    int  game_mode;                                 // +0x1a094 selected game mode (0 campaign, 1 quick, 2 custom, 3 client, 4 host)  0x4b9e70
    int  check_a098;                                // +0x1a098 BM_SETCHECK source of control 0x434   0x4bf640
    char pad_a[0x1a0b8 - 0x1a09c];
    int  list_top;                                  // +0x1a0b8 top-visible list entry    0x4bb260
    int  list_sel;                                  // +0x1a0bc selected entry            0x4bb260
    char pad_b[0x1a0cc - 0x1a0c0];
    int  check_a0cc;                                // +0x1a0cc control 0x453   0x4bf640
    int  check_a0d0;                                // +0x1a0d0 control 0x454   0x4bf640
    char pad_c[0x1a1d0 - 0x1a0d4];
    int  sub_option;                                // +0x1a1d0 selected sub-option of the host page   0x4b9e70
};
// g_config: the view ((GameConfig*)g_game) of the game-data pointer g_game (0x5e2930, declared by game_data.h / party/game.h in the including source)
struct CreateGameOptionsPage {
    char pad[0x374];
    HWND hDlg;                                      // +0x374 dialog window of the page
    void refresh();                                 // 0x4b9e70
    void update_checks();                           // 0x4bf640 sets the page's radio / check buttons from ((GameConfig*)g_game), then refresh()
};

// ---- the ragdoll body record (0x4bc170, 0x4bc1f0; this header called it the software cursor): ../rag_body.h ----------
#include "../rag_body.h"

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
    int unk74c;                                     // +0x74c a COM-style object whose vtable slot +0x28 is called by 0x4bc350
    int unk750;                                     // +0x750
};
void __fastcall apply_input_settings(InputSet* self);   // 0x4bc350
struct Slot;
void __fastcall release_input_devices(Slot* self);  // 0x4bc3d0

// ---- the render batch's buffer table (0x4bc540..0x4bd0c0): dev_buffer / buffer_pair
#include "../dev_buffer.h"
#include "../../rgl/resource.h"          // resmgr g_resmgr 0x5d6fa0 (the one declaration)
#include "../render_batch.h"                   // render_batch (shared with terrain_b)
#include "../../include/render_flags.h"
struct font_details {                               // 0x1c: the staged glyph record Font::load 0x4d9aa0 copies into the
    float a, b, c, d, e, f, g;                      // table (a = u/f0, b = h/f4, c = w/f8, d = v/fc, e..g = shifts)
    font_details();                                 // 0x4bd120 (zeroes all seven)
};

// ---- ambient particle ring and screen ray (0x4bd170, 0x4bd2f0) ---------------------------------------------------
struct camera_ray {                                 // scene camera record
    char     pad0[0xe0];
    float    vx, vy;                                // +0xe0, +0xe4 viewport origin
    char     pad2[0xf0 - 0xe8];
    float    vw, vh;                                // +0xf0, +0xf4 viewport size
    char     pad3[0x1ac - 0xf8];
    char     ortho;                                 // +0x1ac
    char     pad4[0x274 - 0x1ad];
    rgl::matrix view;                               // +0x274
    float    zb4, zb8, zbc, zc0, zc4, zc8;          // +0x2b4..+0x2c8
    void screen_to_world(vector4* near_pt, vector4* far_pt);   // 0x4bd2f0
};

// ---- ini file reader (0x4bd4d0..0x4bd740) --------------------------------------------------------------------------
#include "../ini_file.h"   // ini_entry / ini_file (0x4bd4d0..0x4bd740), one definition

// ---- script blocks and the module-file records (0x4bd7a0..0x4be6f0) --------------------------------------------------
// The script block and its 0x98-byte operation record are the shared DialogBlock / Elem (read 0x4607e0, Elem::Elem 0x4607d0,
// copy constructor 0x4bd7a0, defined in config.cpp): one definition, src/include/dialog_rec.h.
#include "../../include/dialog_rec.h"
struct trigger_rec {                                // three floats then the condition and consequence blocks
    float f0, f4, f8;                               // +0, +4, +8 (vtable +0x38 reads)
    char  pad_c[4];
    DialogBlock cond;                                     // +0x10
    DialogBlock cons;                                     // +0x2c
    void read(stream_base& s, const char* name, int idx);   // 0x4bd830
};
struct trigger_entry {
    float f0;                                       // +0
    DialogBlock block;                                    // +4
    int clock;                                      // +0x20 clock index (ctor 0x4c1de0, stamp 0x4c1ee0)
    int pad24;
    __int64 t;                                      // +0x28 the clock reading taken at it
    trigger_entry();                                // 0x4c1de0 (src/game/triggers)
    void read(stream_base& s, const char* name, int idx);   // 0x4bd910
};

struct psys_record {                                // 0xc0 bytes
    tstring name;                                   // +0x00
    int f10;                                        // +0x10
    tstring tex;                                    // +0x14
    char pad24[0xc0 - 0x24];
    void load(stream_base& a);                      // 0x4bd990
    static void* read(stream_base& a, char* o);     // 0x4bdc00 (cdecl (stream, record): reads into o, not this)
    void load_extras(stream_base& a);               // 0x4bdd50
};
struct prsnt_record {                               // 0x24 bytes + extras
    tstring tex;                                    // +0x00
    int f10;                                        // +0x10
    tstring name;                                   // +0x14
    int f24, f28, f2c, f30, f34, f38, f3c, f40, f44;
    void load_extras(stream_base& a);               // 0x4bddf0
    void save(stream_base& a);                      // 0x4bdf60
};
#include "../../include/prsnt_table.h"           // PresentationTable (load_used 0x4bdec0)

struct sound_def {                                  // a sound definition read by 0x4bdff0 (size unknown)
    tstring tex;                                    // +0x00
    tstring name;                                   // +0x10
    unsigned f20;                                   // +0x20
    float f24;                                      // +0x24
    int f28;                                        // +0x28
    int f2c, f30;                                   // +0x2c, +0x30
    int f34, f38, f3c;                              // +0x34..+0x3c (cleared by 0x4bdff0)
    void save(stream_base& a);                      // 0x4bdff0
};
struct sound_record {                               // 0x98 bytes: name, a count/flag, up to 32 sample ids (0x4be200, stride 0x98 in 0x4be270)
    tstring name;                                   // +0x00
    int f10;                                        // +0x10
    int samples[0x20];                              // +0x14
    int nsamples;                                   // +0x94
    void load(stream_base& a);                      // 0x4be200
};
// (the view load_used 0x4be270 has of music_set, src/rgl/mesh2/rec_tables.h: count +8, the 0x98-byte records +0xc,
// the sound-code map +0x50)
struct loaded_snd_table {
    char pad0[8];
    int count;                                      // +0x08
    sound_record* records;                          // +0x0c
    char pad10[0x50 - 0x10];
    int used[0x7d];                                 // +0x50
    void load_used();                               // 0x4be270
};

struct music_res {                                  // 0x20-byte music record, in the player's head (0x4be0e0)
    int v00, v04;
    unsigned flags;                                 // +0x08 (0x4be10d, 0x4be175)
    int v0c;
    int v10, v14;
    int v18;
    const char* name;                               // +0x1c tstring buffer pointer; NULL means the text is inline at +0x1c
};
#include "../music_player.h"            // music_player, g_music (0xa73a10), raw_tstring

struct menu_item {                                  // 0x68 bytes
    tstring name;                                   // +0x00
    tstring f10;                                    // +0x10
    tstring f20;                                    // +0x20
    DialogBlock   cond;                                   // +0x30
    DialogBlock   conseq;                                 // +0x4c
    void read(stream_base* s, const tstring* menu); // 0x4be310
    menu_item();                                    // 0x4be420
};
#include "../../include/rec34.h"                 // rec34 (shared with rgl/mesh2/gui_edit.h)
#include "../../include/ground_type.h"           // GroundType (the 0x20-byte ground-spec record; GroundType::read 0x4be600 is below)
struct clip_slot_raw {                              // 0x18 bytes
    raw_tstring name;                               // +0x00
    int     v10;                                    // +0x10
    char*   v14;                                    // +0x14
};
struct clip_slot {
    raw_tstring name;                               // +0x00
    int     v10;                                    // +0x10
    char*   v14;                                    // +0x14
    clip_slot();                                    // 0x4be720
};
struct clip_table {                                 // 0x14c bytes: 13 slots of 0x18 bytes at +0x0c
    int       v00, v04, v08;                        // +0x00
    clip_slot_raw slots[13];                        // +0x0c
    int       v144, v148;                           // +0x144
    clip_table();                                   // 0x4be6b0
    ~clip_table();                                  // 0x4be6f0
};

#endif // GAME_CONFIG_CONFIG_H
