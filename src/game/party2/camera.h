// camera.h: types and globals of the script block runner in camera.cpp (DialogBlock::run_line / emit, 0x4b0940 / 0x4b09e0, and the
// operand / loop helpers 0x49d500..0x49dd30, 0x49f7c0). The DialogBlock / Elem classes are canonical (src/include/dialog_rec.h);
// this header holds what only the runner reads: the interpreter globals, the emitter table, the try-stack object and the
// per-statement callee shapes. Offsets cite the function that proves them. Include after dialog_rec.h and game_data.h.
#ifndef MB_CAMERA_H
#define MB_CAMERA_H
#include "../../include/dialog_rec.h"
#include "../../include/game_data.h"

extern unsigned g_reg_lo[];                         // 0xa70490 (register pair table: 8 bytes apart)
extern int g_reg_hi[];                              // 0xa70494
struct VarTable {
    int get(int id);                                // 0x48d690
    void set(int id, int v);                        // 0x493db0
};
extern VarTable g_vartab;                           // 0xa70440
extern int g_cur_opcode;                            // 0x5e25a0
extern int g_cur_param;                             // 0x5e2590
extern int g_script_output;                         // 0x5a9fb0
extern int g_lineno;                                // 0x5e2598  script line number
extern int g_script_out;                            // 0x5e259c  "script produced output" flag
extern int g_script_flag;                           // 0x5e25a4  "aborted/returned" latch cleared each run
// mb_error(const char* msg): the message string reaches it in ecx (__fastcall), so it builds its own
// tstring and prints; call sites load the address directly into ecx.
void __fastcall report(const char* msg);            // 0x49cc60
int __fastcall container_size(void* p);             // 0x40f010 (count = (last-first)>>2)

// ---- text log stream (views of the canonical stream_base / text_stream of src/rgl/stream.h, which cannot be included with match/include/rgl.h) ----
// text log stream at 0x5a9f60 (see s00418a70): slot 10 (+0x28) is w_cstr
struct LogStreamBase {
    virtual void reset();
    virtual void rewind();
    virtual LogStreamBase& w_stream(void* o) = 0;
    virtual LogStreamBase& w_float(void* v) = 0;
    virtual LogStreamBase& w_bool(void* v) = 0;
    virtual LogStreamBase& w_u64(void* v) = 0;
    virtual LogStreamBase& w_i64(void* v) = 0;
    virtual LogStreamBase& w_uint(void* v) = 0;
    virtual LogStreamBase& w_int(int* v) = 0;
    virtual LogStreamBase& w_byte(void* v) = 0;
    virtual LogStreamBase& w_cstr(const char* s) = 0;
};
struct LogStream : LogStreamBase {
    virtual LogStreamBase& w_stream(void* o);
    virtual LogStreamBase& w_float(void* v);
    virtual LogStreamBase& w_bool(void* v);
    virtual LogStreamBase& w_u64(void* v);
    virtual LogStreamBase& w_i64(void* v);
    virtual LogStreamBase& w_uint(void* v);
    virtual LogStreamBase& w_int(int* v);
    virtual LogStreamBase& w_byte(void* v);
    virtual LogStreamBase& w_cstr(const char* s);
};
extern LogStream g_log;                        // 0x5a9f60

// keyboard state: per-scan-code ints, held state at 0xa17000 and this frame's press at 0xa17400 (see s0052cf60)
extern int g_key_a[256];                        // 0xa17000
extern int g_key_b[256];                        // 0xa17400

// the game data's "current emitter audio" state at +0x1a068..+0x1a07c (see s00435490.h SoundCtl)
struct EmitAudio {
    int id;                                     // +0x1a068  current emitter id (0 = none)
    tstring name;                               // +0x1a06c  current emitter name
    int tag;                                    // +0x1a07c  current emitter tag
};
#define EMIT_AUDIO (*(EmitAudio*)((char*)g_game + 0x1a068))

// the 0x34-byte emitter records at 0xa75eb0; the script block is at +0x18 and the name at +4
struct ScriptEmitter {
    int pad0;                                   // +0
    tstring name;                               // +4
    char pad14[0x18 - 0x14];
    DialogBlock script;                         // +0x18
    char pad34[0x34 - 0x18 - sizeof(DialogBlock)];
};
extern ScriptEmitter* g_emitters;               // 0xa75eb0
extern int g_emitter_count;                     // 0xa75eb4

// The goto/branch stack object (local_410 of emit): 0x400 bytes of entries, then the depth counter at +0x400
// (read by 0x488e60 / 0x49d7f0 / 0x49f7c0; the emitter only tests it against 0) and one more dword. Both
// trailing dwords are cleared in emit's prologue.
struct ScriptStack {
    int data[256];
    int sp;
    int pad;
    void goto_stmt(int line, int val);          // 0x488e60 (thiscall, ret 8)
};

// ---- per-statement interpreter callees (not decompiled; declared for shape) -----------------------------
struct ScriptStmt {                             // statement pointer in ECX
    char run(int stmt, int which, int* out);    // 0x49fa10 (thiscall, ret 0xc)
};
struct KeyState { void frame(int a); };         // 0x487930 (thiscall, ret 4)
extern KeyState g_keyboard;                     // 0xa170c8
char __fastcall check_script_param(int* value, int* limit);   // 0x49ccc0
void pump_messages();                           // 0x4bb310
void set_debug_label();                         // 0x4bb390
void set_progress_range(int lo, int hi);        // 0x4bb3b0
void refresh_settings_list(const tstring* page, char detailed);   // 0x4bb3f0
void __cdecl mb_error(const char* fmt, ...);    // 0x42cc80

#endif // MB_CAMERA_H
