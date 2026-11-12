// script_state.h: the script interpreter's state object g_script (0xa70440, 0x2ec0 bytes: 0x2ebc of members, then the 4 alignment bytes before g_mission_ctx), ONE class for the whole object.
// static_init constructs it with ScriptState::ScriptState (0x4d3580) and registers ScriptState::~ScriptState (0x4cb9e0) with
// atexit. The first 0x2c50 bytes are the interpreter's state proper (spawn / trigger parameters, the trigger result, the
// fixed point scale, the result string, music cues, the registers, the position registers and the string registers); the
// rest is the world the game builds around it (the map page grids, the page grid's bounds, the level, the navigation graph
// and the scene's material lists), which the same constructor clears and the same destructor tears down. Every name that
// used to be a global inside it is a member now (g_level, g_nav, g_mat_vec, g_page_cols, g_registers, g_event_pos,
// g_last_hit_frame, g_trigger_result, g_result_string, ...).
// Deliberately light (forward declarations for the level, the navigation graph and the page entities): terrain_b.h,
// game.h, world_data.h and the char_sheet headers include it and cannot include rgl/scene/world.h.
#ifndef MB_SCRIPT_STATE_H
#define MB_SCRIPT_STATE_H
#include <stddef.h>
#include "../game/tstring.h"
#include "../game/list8.h"                  // List8 : I64Vec (std::vector<__int64> with the allocator word)
#include <vector>                           // std::vector<rgl::mesh*> (the map scene's material lists)
#include "../rgl/rgl_math.h"                // rgl::matrix (the position registers)

namespace rgl { struct world; struct entity; struct mesh; }
struct NavGraph;

// The script engine's trigger result: set_trigger_result (0x4ac3f2) stores one 64-bit value, the event dispatch
// (GameData::emit_event / sl_query) reads and clears it as two dwords.
union TriggerResult { __int64 value; struct { int store0; int store1; }; };

// the script variables: std::vector<__int64> with the allocator word, constructed empty (List8's other users are records the compiler never constructs)
struct VarVec : List8 {
    VarVec() { _Myfirst = 0; _Mylast = 0; _Myend = 0; }
    ~VarVec() { if (_Myfirst) operator delete(_Myfirst); _Myfirst = 0; _Mylast = 0; _Myend = 0; }   // runs after str[]'s destructors (~ScriptState 0x4cb9e0)
};

struct ScriptState {
    // ---- the interpreter's state (+0x00 .. +0x2c50) ----
    int screenshot_count;                                  // +0x00 take_screenshot 0x4ebc10 numbers the files with it; Game::start_new_game resets it
    int pad04;                                             // +0x04
    union {                                                // +0x08 the first event parameter (trigger parameter 1, the spawn slider's key, the sound event's id)
        __int64 param1;
        struct { int param1_lo; int param1_hi; };
    };
    union {                                                // +0x10 the second event parameter
        __int64 param2;
        struct { int param2_lo; int param2_hi; };
    };
    TriggerResult trigger_result;                          // +0x18
    int fixed_point_multiplier;                            // +0x20 script fixed point scale (1 << 8 .. 1 << 16): the arithmetic ops, positions and times scale by it
    tstring result_string;                                 // +0x24 the text helper's scratch / result string (set_result_string 0x49fce6; len at +0x30 is the "valid" count the inventory panel tests)
    int next_track;                                        // +0x34 music track queued by the script (0x4a174a)
    int next_track_options;                                // +0x38
    int cue_track;                                         // +0x3c
    VarVec vars;                                     // +0x40 the script variables (Module System global variables, 8-byte slots, up to 0x10000); saved after the clock (GameData::save_state 0x4e8f10, Game::load_state 0x51dbd0)
    union {                                                // +0x50 registers               0x460160
        __int64 reg[128];
        struct { unsigned lo; int hi; } reg_half[128];     //       the same registers as the halves the camera script ops store
    };
    void set_reg0(__int64 v) { reg[0] = v; }               // inline (spawn_around_party's result)
    rgl::matrix pos[128];                                  // +0x450 position registers (pos[0].o is the party event position emit_event 0x14 reads, pos[1] the last melee hit frame)
    tstring str[128];                                      // +0x2450 string registers      0x460160

    // ---- the world around it (+0x2c50 .. +0x2ebc) ----
    char new_game_done;                                    // +0x2c50 set by step 6 of Game::start 0x51c720; 0x51d2b0 returns at once when set
    char pad2c51[3];
    rgl::entity* map_cell[64];                             // +0x2c54 the map's page grid, [col * 8 + row] (Game::build_scene 0x520d60)
    rgl::entity* map_saved[64];                            // +0x2d54 the same grid parked while the scene is rebuilt (cell + 0x100); released at exit
    int page_cols;                                         // +0x2e54
    int page_rows;                                         // +0x2e58
    float minx, miny, maxx, maxy;                          // +0x2e5c the map's bounds
    float sizex, sizey;                                    // +0x2e6c
    rgl::world* level;                                     // +0x2e74 the current level / scene (built by Game::build_level 0x51d2b0)
    NavGraph* nav;                                         // +0x2e78 the map object (also +0x44/+0x54 are handed to the scene constructor)
    std::vector<rgl::mesh*> mat_type0;                     // +0x2e7c the map scene's per-material mesh lists, filled by Game::build_map (material type 0, 8, 0x16, 0x17)
    std::vector<rgl::mesh*> mat_river;                     // +0x2e8c type 8: the river markers (Game::update_markers drifts them by the river speed)
    std::vector<rgl::mesh*> mat_sea;                       // +0x2e9c type 0x16: the sea markers (sea speed * 3)
    std::vector<rgl::mesh*> mat_sea2;                      // +0x2eac type 0x17: the second sea markers (sea speed * 2, opposite direction)

    ScriptState();                                         // 0x4d3580
    ~ScriptState();                                        // 0x4cb9e0
    int get_var(unsigned index);                           // 0x48d690 (only the first dword of the slot; 0 when out of range)
    void set_var(int index, int value);                    // 0x493db0 (thiscall, ret 8: grows the variable vector through the 0x493c40 reserve)
    void reset();                                          // 0x4c80c0 (the music / spawn / trigger parameters back to their defaults, the saved list freed)
    tstring expand_token(const tstring& tok);              // 0x460160 (thiscall, ret 8: hidden return + token)
    tstring expand(const tstring& in);                     // 0x4606d0 (thiscall, ret 8)
};
typedef char script_state_check[(sizeof(ScriptState) == 0x2ec0 && offsetof(ScriptState, trigger_result) == 0x18 &&
                                 offsetof(ScriptState, fixed_point_multiplier) == 0x20 && offsetof(ScriptState, result_string) == 0x24 &&
                                 offsetof(ScriptState, cue_track) == 0x3c && offsetof(ScriptState, vars) == 0x40 &&
                                 offsetof(ScriptState, reg) == 0x50 && offsetof(ScriptState, pos) == 0x450 &&
                                 offsetof(ScriptState, str) == 0x2450 && offsetof(ScriptState, new_game_done) == 0x2c50 &&
                                 offsetof(ScriptState, map_cell) == 0x2c54 && offsetof(ScriptState, map_saved) == 0x2d54 &&
                                 offsetof(ScriptState, page_cols) == 0x2e54 && offsetof(ScriptState, level) == 0x2e74 &&
                                 offsetof(ScriptState, mat_type0) == 0x2e7c && offsetof(ScriptState, mat_sea2) == 0x2eac) ? 1 : -1];
extern ScriptState g_script;                               // 0xa70440

#endif // MB_SCRIPT_STATE_H
