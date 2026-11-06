// camera.h: types and globals of the script block runner in script/script_interpreter.cpp (DialogBlock::run_line / emit, 0x4b0940 / 0x4b09e0, and the
// operand / loop helpers 0x49d500..0x49dd30, 0x49f7c0). The DialogBlock / Elem classes are canonical (src/include/dialog_rec.h);
// this header holds what only the runner reads: the interpreter globals, the emitter table, the try-stack object and the
// per-statement callee shapes. Offsets cite the function that proves them. Include after dialog_rec.h and game_data.h.
#ifndef MB_CAMERA_H
#define MB_CAMERA_H
#include "../../include/dialog_rec.h"
#include "../game_globals.h"      // canonical g_script_error
#include "../../include/game_data.h"
#include "../../include/script_state.h"   // ScriptState g_script (0xa70440): the one object

struct ScriptReg { unsigned lo; int hi; };          // one 64-bit script register (the interpreter reads / writes the halves separately)
#include "../config/config_block.h"           // Config g_cfg (0x5a9f98)
extern int g_cur_opcode;                            // 0x5e25a0
extern int g_cur_param;                             // 0x5e2590
struct Config;
extern int g_lineno;                                // 0x5e2598  script line number
// report(msg) (0x49cc60): a file-static of the original translation unit (msg in ecx); defined in script/script_interpreter.cpp.
int __fastcall size(void* p);             // 0x40f010 (count = (last-first)>>2)

#include "../../rgl/stream.h"                // g_log (log_stream, 0x5a9f60)

// keyboard state: g_keyboard.down (held) and g_keyboard.prev (the previous frame's snapshot)


// the game data's "current emitter audio" state at +0x1a068..+0x1a07c (see s00435490.h SoundCtl)
struct EmitAudio {
    int id;                                     // +0 (game+0x1a068)  current emitter id (0 = none)
    tstring name;                               // +4 (game+0x1a06c)  current emitter name
    int tag;                                    // +0x14 (game+0x1a07c)  current emitter tag
};
#define EMIT_AUDIO (*(EmitAudio*)((char*)g_game + 0x1a068))

#include "../../include/rec34.h"                 // rec34_table, g_scripts (0xa75eb0): the script emitter records (name +4, script block +0x18)

// The goto/branch stack object (local_410 of emit): 0x400 bytes of entries, then the depth counter at +0x400
// (read by 0x488e60 / 0x49d7f0 / 0x49f7c0; the emitter only tests it against 0) and one more dword. Both
// trailing dwords are cleared in emit's prologue.
struct ScriptStack {
    int data[256];
    int sp;
    int pad;
    void push(int line, int val);          // 0x488e60 (thiscall, ret 8)
};

#include "../slot.h"                              // Slot, g_keyboard (0xa170c8)
#include "../config/config_block.h"
char __fastcall check_script_param(int* value, int* limit);   // 0x49ccc0
void pump_messages();                           // 0x4bb310
void set_debug_label();                         // 0x4bb390
void set_progress_range(int lo, int hi);        // 0x4bb3b0
void refresh_settings_list(const tstring* page, char detailed);   // 0x4bb3f0
void __cdecl mb_error(const char* fmt, ...);    // 0x42cc80

#endif // MB_CAMERA_H
