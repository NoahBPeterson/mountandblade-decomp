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

// ---- the option block (rgl_config.txt), global object g_cfg at 0x5a9f98 ----------------------------------------
// Offsets from the setters 0x4bc020..0x4bc170 and the readers; the block is the int array that
// src/rgl/core/resource_io.cpp (0x415fb0, rgl_config::save) writes out. Field names are the file's keys.
struct Config {
    int first_time;                 // +0x00  0x4bc020, 0x4badf0 (selects the present interval)
    int texture_detail;             // +0x04
    int show_framerate;             // +0x08
    int max_framerate;              // +0x0c
    int start_windowed;             // +0x10
    int skip_tutorial;              // +0x14
    int enable_edit_mode;           // +0x18
    int force_single_threading;     // +0x1c
    int debug_mode;                 // +0x20
    int cheat_mode;                 // +0x24
    int enable_blood;               // +0x28
    int fake_reflections;           // +0x2c
    int disable_frequency_variation;// +0x30
    int use_vertex_shaders;         // +0x34
    int use_pixel_shaders;          // +0x38
    int disable_music;              // +0x3c
    int disable_sound;              // +0x40
    int unk44;                      // +0x44
    int render_buffer_size;         // +0x48  megabytes of buffer memory (0x4bc5b0 reads 0x5a9fe0)
    int display_width;              // +0x4c  0x5a9fe4 (0x4badf0)
    int display_height;             // +0x50  0x5a9fe8
    int display_bits;               // +0x54  0x5a9fec (16: R5G6B5, else X8R8G8B8)
    int antialiasing;               // +0x58  0x5a9ff0 (multisample type 2 / 4)
    int sample_quality;             // +0x5c  0x5a9ff4
    int force_vsync;                // +0x60  0x5a9ff8 (0x4badf0: present interval ONE)
    int unk64;                      // +0x64
    int shadowmap_quality;          // +0x68
    int unk6c;                      // +0x6c
    void init_defaults();                           // 0x4bc020
    void set(tstring& key, tstring& value);         // 0x4bbba0 (fastcall-like: thiscall, ret 8)
    void load_file(FILE* f);                        // 0x4bbf70
    void set_triple_buffering(int t);               // 0x4bc090
    void set_texture_quality(int q);                // 0x4bc100
    void split_line(const char* line, tstring* first, tstring* second);   // 0x4bba90 (thiscall: `this` unused)
    int value_as_int(const tstring& s);             // 0x4bbb70 (thiscall: `this` unused)
};
extern Config g_cfg;                                // 0x5a9f98


// ---- globals ---------------------------------------------------------------------------------------------------
extern HWND g_hwnd_dlg;                             // 0x8ee3fc0 the launcher's dialog (list box 0x4b0, combo 0x433)
extern HWND g_hwnd_page;                            // 0x8ee3fbc the options page window (list 0x431, combo 0x433), set_list_scroll
extern int g_view_x;                                // 0x5d75d4 view rectangle (0x4bace0)
extern int g_view_y;                                // 0x5d75d0
extern int g_view_w;                                // 0x5d75c8
extern int g_view_h;                                // 0x5d75cc
extern char g_screenshot_cropped;                   // 0x8ee3cec (the Core's m_bWindowed byte, see core.h)
extern IDirect3DDevice9* g_d3d_device;              // 0xa17f28
extern int g_dinput_enabled;                        // 0x5e28fc (also g_use_shaders)
extern int g_edit_key;                              // 0x5e28d0 first value of the line being edited
extern int g_edit_value;                            // 0x5e28d4 second value of the line being edited
extern float g_line_pitch_ratio;                    // 0x590d94 0.6f: visible lines vs window height
extern "C" const char g_eq_sep[];                   // 0x593160 " = "
extern const char g_eol[];                          // 0x54b118 ""
extern __int64 g_ticks[];                           // 0x5e2948 clock table, 1e-5 s ticks (same table as gui_manager.h's g_time)

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
extern GameConfig* g_config;                        // 0x5e2930
struct CreateGameOptionsPage {
    char pad[0x374];
    HWND hDlg;                                      // +0x374 dialog window of the page
    void refresh();                                 // 0x4b9e70
    void update_checks();                           // 0x4bf640 sets the page's radio / check buttons from g_config, then refresh()
};

// ---- the engine's software cursor (0x4bc170, 0x4bc1f0) ------------------------------------------------------------
struct Settings {                                   // the named-float settings object (0x42f190)
    int get_float(const char* name, double* out);   // 0x42f190
};
struct Vec4 {                                       // a vector in a 16-byte slot; the 4th lane is left alone by the ctor
    float x, y, z, w;
    Vec4() {}
    Vec4(float a, float b, float c) { x = a; y = b; z = c; }
};
struct Cursor {
    float pos_x, pos_y, pos_z;                      // +0x00
    int   unk0c;                                    // +0x0c
    float pos2_x, pos2_y, pos2_z;                   // +0x10
    int   unk1c;                                    // +0x1c
    float radius;                                   // +0x20 (0.05f)
    unsigned char moving;                           // +0x24
    unsigned char unk25;                            // +0x25
    int   unk28;                                    // +0x28
    Cursor();                                       // 0x4bc170
    void read(Settings* src);                       // 0x4bc1f0
};

// ---- input state and device set (0x4bc2d0..0x4bc430) -------------------------------------------------------------
struct Matrix { float m[16]; };
Matrix* __cdecl make_matrix(Matrix* out);           // 0x40cd00 fills out and returns it
struct DisplayState {                               // the input state prefix of InputSet plus two view matrices
    char unk000[0x618];
    int  unk618;                                    // +0x618 (cleared by 0x4bc2d0, 0x4bc350 reads +0x74c)
    char unk61c[0x130];
    int  unk74c;                                    // +0x74c
    int  unk750;                                    // +0x750
    Matrix mat1;                                    // +0x754
    Matrix mat2;                                    // +0x794
    DisplayState();                                 // 0x4bc2d0
};
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
    int init_devices(HWND hwnd);                    // 0x4bc430
};
void __fastcall apply_input_settings(InputSet* self);   // 0x4bc350
void __fastcall release_input_devices(InputSet* self);  // 0x4bc3d0

// ---- the render batch's buffer table (0x4bc540..0x4bd0c0): dev_buffer / buffer_pair
#include "../dev_buffer.h"
struct shader_res;
struct resource_manager_x {                         // the resource manager (0x5d6fa0), shader lookup only (calls added for src/game/triggers)
    shader_res* get_shader(const tstring& name);    // 0x418b30
    struct texture* get_texture(const tstring& name);   // 0x418870 (rgl/resource.h texture; its d3d_tex at +0x40)
    int  reload_textures();                         // 0x418da0
    void reset_effect_sets();                       // 0x418fb0
    int  restore(ID3DXEffect* fx);                  // 0x419580 (device reset: textures, effects)
    int  invalidate();                              // 0x4195e0 (device lost)
    void linearise_meshes();                        // 0x419640 (called "finalize" by module_info.txt's loader)
    void load_pack(const tstring& name, int user);  // 0x41e150
};
extern resource_manager_x g_resmgr_x;               // 0x5d6fa0
#include "../render_batch.h"                   // render_batch (shared with terrain_b)
struct font_details {                               // seven zeroed dwords
    int a, b, c, d, e, f, g;
    font_details();                                 // 0x4bd120
};

// ---- ambient particle ring and screen ray (0x4bd170, 0x4bd2f0) ---------------------------------------------------
// `weather` is a view of the UI root (gui_manager.h): its meshA/meshB[32] at +0x1728/+0x6328 are `first`/`second`.
struct part {                                       // one particle mesh (0x260 bytes)
    char     pad0[0x3c];
    unsigned color;                                 // +0x3c
    char     pad40[0x260 - 0x40];
    void render(void* pass, const rgl::matrix* m);  // 0x413750
};
struct weather {
    char     pad0[0x171c];
    float    fade;                                  // +0x171c
    int      cur;                                   // +0x1720
    char     pad1[0x1728 - 0x1724];
    part     first[32];                             // +0x1728
    part     second[32];                            // +0x6328
    float    life[32];                              // +0xaf28
    float    delta[32];                             // +0xafa8
    char     pad2[4];
    unsigned colors[32];                            // +0xb02c
    void update(void* pass, const float* depth);    // 0x4bd170
};
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
struct ini_entry {
    tstring key;                                    // +0
    char    pad10[0xc];
    tstring value;                                  // +0x1c
};
struct ini_file {
    ini_entry entries[0x1000];                      // +0
    int       count;                                // +0x2c000
    ini_file();                                     // 0x4c1be0 (src/game/triggers: constructs the 0x1000 key / value string pairs, clears count)
    ~ini_file();                                    // 0x4be920 (destroys them, last pair first)
    void load(FILE* f);                             // 0x4bd630
    int  find(const tstring& key);                  // 0x4bd5a0
    float get_float(const tstring& key);            // 0x4bd5e0
    int  get_hex(int* idx);                         // 0x4e51e0 (sscanf "%x" of entry *idx; 0 when out of range; src/game/cls_map/progress_bar.cpp)
    float get_float(int* idx);                      // 0x4e5230 (atof of entry *idx; -1.0f when out of range)
    int  hex_field(const tstring& key);             // 0x4e5d30 (find, then "%x" of the entry)
    void parse_line(const char* line, tstring& key, tstring& value);   // 0x4bd4d0
    void write(FILE* f, const tstring& key, const tstring& value);     // 0x4bd740
};

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
    void* read(stream_base& a, char* o);            // 0x4bdc00
    void load_extras(stream_base& a);               // 0x4bdd50
};
struct loaded_psys_table {
    psys_record* records;                           // +0x00
    int count;                                      // +0x04
    int used[11];                                   // +0x08
    void load_used();                               // 0x4bdb60
    void clear();                                   // 0x4c1f20 (src/game/triggers: deletes the record array)
    void resize(stream_base& s);                    // 0x4c1f70 (reads the count and every record)
};
struct prsnt_record {                               // 0x24 bytes + extras
    tstring tex;                                    // +0x00
    int f10;                                        // +0x10
    tstring name;                                   // +0x14
    int f24, f28, f2c, f30, f34, f38, f3c, f40, f44;
    void load_extras(stream_base& a);               // 0x4bddf0
    void save(stream_base& a);                      // 0x4bdf60
};
struct loaded_prsnt_table {
    prsnt_record* records;
    int count;
    int used[3];
    void load_used();                               // 0x4bdec0
};

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
struct loaded_snd_table {
    sound_record* records;
    int count;
    int used[0x7d];
    void load_used();                               // 0x4be270
};

struct music_res {                                  // 0x20-byte music record, in the player's head (0x4be0e0)
    unsigned flags;                                 // +0x08? (read at +0 of the record here)
    int v0c;
    int v10, v14;
    float v18, v1c;
    int prim;
    const char* name;
};
struct music_player {
    char pad0[0x2008];
    float start[256];                               // +0x2008 start time per record
    int w[0x26];                                    // +0x2408 state block: per-slot arrays (state, FMOD streams at +0x2428, record index +0x2430...)
    void stop(int i);                               // 0x4be0a0
    void play(int rec, int* slot);                  // 0x4be0e0
    music_player();                                 // 0x4c24c0 (src/game/triggers: constructs the 0x100 record names, the three clocks, the monitor state)
    void update();                                  // 0x4c25e0 per-frame monitor of the two stream slots and the track selection
};

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
struct rec_table {                                  // {records, count} pair of the module-file tables (globals 0xa75eb0, 0xa75f2c, 0x8ed4f58)
    void* records;                                  // +0
    int   count;                                    // +4
    rec_table();                                    // 0x4be520 (the original is bracketed by WinLicense marker records)
};
struct rec34_table {
    rec34* records;                                 // +0x00
    int    count;                                   // +0x04
    int    success;                                 // +0x08
    int    used[28];                                // +0x0c
    void load_used();                               // 0x4be560
};
struct rec20 {                                      // 0x20 bytes
    tstring name;                                   // +0x00
    void*   mat;                                    // +0x10
    void*   mat2;                                   // +0x14
    float   f18;                                    // +0x18
    int     f1c;                                    // +0x1c
    void read(stream_base* s);                      // 0x4be600
};
struct raw_tstring { int pos; char* buf; int cap; int len; };   // tstring layout without the implicit ctor
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
