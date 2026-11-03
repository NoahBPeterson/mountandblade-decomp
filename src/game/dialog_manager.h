// dialog_manager.h: classes of dialog_manager.cpp (0x460160..0x461200): the conversation manager (DialogMgr), the
// dialog records read from the module file (DialogRec, Module System `dialogs`), the script blocks they carry
// (DialogBlock / Elem: condition and consequence operation lists), the string-register expander (ScriptState) and the
// parts of the game globals they touch. Game code. All names provisional; every offset is read by a function of
// this module (cited by address).
#ifndef GAME_DIALOG_MANAGER_H
#define GAME_DIALOG_MANAGER_H
#include "./tstring.h"

// ---- binary stream (vtable slots used: 13..21 read, 0..12 write) -----------------------------------------
struct stream_base {
    virtual void reset();                                  // 0
    virtual void rewind();                                 // 1
    virtual stream_base& w_stream(stream_base* o) = 0;     // 2
    virtual stream_base& w_float(float* v) = 0;            // 3
    virtual stream_base& w_bool(char* v) = 0;              // 4
    virtual stream_base& w_u64(unsigned __int64* v) = 0;   // 5
    virtual stream_base& w_i64(__int64* v) = 0;            // 6
    virtual stream_base& w_uint(unsigned* v) = 0;          // 7
    virtual stream_base& w_int(int* v) = 0;                // 8
    virtual stream_base& w_byte(unsigned char* v) = 0;     // 9
    virtual stream_base& w_cstr(const char* s) = 0;        // 10
    virtual stream_base& w_tstr(const tstring* s) = 0;     // 11
    virtual stream_base& slot12(void* v) = 0;              // 12
    virtual stream_base& r_stream(stream_base* o) = 0;     // 13
    virtual stream_base& r_float(float* v) = 0;            // 14
    virtual stream_base& r_bool(char* v) = 0;              // 15
    virtual stream_base& r_uint(unsigned* v) = 0;          // 16  0x4607e0
    virtual stream_base& r_u64(unsigned __int64* v) = 0;   // 17
    virtual stream_base& r_i64(__int64* v) = 0;            // 18  0x4607e0
    virtual stream_base& r_int(int* v) = 0;                // 19  0x4607e0, 0x4608d0, 0x460ab0
    virtual stream_base& r_byte(char* v) = 0;              // 20
    virtual stream_base& r_tstr(tstring* v) = 0;           // 21  0x4608d0
};

void mb_snprintf(char* buf, unsigned n, const char* fmt, ...);   // 0x415100

#include "./str_table.h"

// ---- parties, troops and the game globals: the shared classes (Party / PartyMgr / TroopRec / GameData / g_game 0x5e2930) ----------------
#include "../include/game_data.h"

#include <stddef.h>
// ---- string registers and the {token} expander (object 0xa70440) ------------------------------------------
struct ScriptState {
    char pad0[0x20];
    int fixed_point_multiplier;                            // +0x20 script fixed point scale (1 << 8 .. 1 << 16): the arithmetic ops, positions and times scale by it
    char pad24[0x34 - 0x24];
    int next_track;                                        // +0x34 music track queued by the script (0x4a174a)
    int next_track_options;                                // +0x38
    int cue_track;                                         // +0x3c
    char pad40[0x50 - 0x40];
    __int64 reg[128];                                      // +0x50 registers               0x460160
    char pad450[0x2450 - 0x450];
    tstring str[128];                                      // +0x2450 string registers      0x460160
    tstring expand_token(const tstring& tok);              // 0x460160 (thiscall, ret 8: hidden return + token)
    tstring expand(const tstring& in);                     // 0x4606d0 (thiscall, ret 8)
};
typedef char script_state_check[(offsetof(ScriptState, str) == 0x2450 && offsetof(ScriptState, cue_track) == 0x3c) ? 1 : -1];
extern ScriptState g_script;                               // 0xa70440

// ---- script blocks and dialog records: the shared classes (Elem / DialogBlock / DialogRec) -----------------------------------------------------
#include "../include/dialog_rec.h"

#include "./dialog_mgr.h"                                    // DlgLine, DialogMgr (g_dialogs 0x8ed50c8)

#endif // GAME_DIALOG_MANAGER_H
