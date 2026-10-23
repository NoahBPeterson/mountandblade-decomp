// dialog_manager.h: classes of dialog_manager.cpp (0x460160..0x461200): the conversation manager (DialogMgr), the
// dialog records read from the module file (DialogRec, Module System `dialogs`), the script blocks they carry
// (Block / Elem: condition and consequence operation lists), the string-register expander (ScriptState) and the
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
extern StrTable g_strtable;                                // 0x5e2a18

// ---- parties ---------------------------------------------------------------------------------------------
struct Party {                                             // 0x5ac8 bytes
    int f0;
    int type;                                              // +4 (1 = ordinary party)      0x460ca0
    int id;                                                // +8                            0x460ca0
    char padc[0x8c - 0xc];
    int owner;                                             // +0x8c                         0x460b90
    char pad90[0x5ac8 - 0x90];
};
struct PartyMgr {                                          // embedded in GameData at +0xf8
    char pad0[0x138b8];
    int count;                                             // +0x138b8                      0x460ca0
    char pad138bc[0x138d0 - 0x138bc];
    Party** pages;                                         // +0x138d0 (index >> 4 -> page of 16)
    char pad138d4[0x138dc - 0x138d4];
    int deflt;                                             // +0x138dc                      0x460ca0, 0x460de0
    int index_of(int id);                                  // 0x48ad00 (thiscall, ret 4)
    Party* get(int id);                                    // 0x51ee50 (thiscall, ret 4)
    void next_party_id(int* id);                           // 0x460ca0
    int first_party_id();                                  // 0x460d20
};

// ---- troops ---------------------------------------------------------------------------------------------
struct TroopRec {                                          // 0x508 bytes
    tstring name;                                          // +0
    char pad10[0x20 - 0x10];
    tstring id;                                            // +0x20 (translation key)       0x460160
    unsigned flags;                                        // +0x30 (low nibble 1 = female) 0x460160
    char pad34[0x508 - 0x34];
};

// ---- the game globals (object behind 0x5e2930) ----------------------------------------------------------
struct GameData {
    char pad0[0x8c];
    int player;                                            // +0x8c player troop            0x460160, 0x460640
    char pad90[0xb0 - 0x90];
    int fb0;                                               // +0xb0                         0x4610b0
    char padb4[0xc8 - 0xb4];
    int fc8;                                               // +0xc8                         0x460620, 0x460d80
    char padcc[0xf8 - 0xcc];
    PartyMgr parties;                                      // +0xf8 .. +0x139d8
    char pad139d8[0x13a00 - 0x139d8];
    TroopRec* chars;                                       // +0x13a00                      0x460160
    int f13a04;                                            // +0x13a04 (repeat count, mode 0x3000)
    char pad13a08[0x13a1c - 0x13a08];
    int f13a1c;                                            // +0x13a1c current repeat index 0x460d80, 0x460de0
    char pad13a20[0x13a4c - 0x13a20];
    int f13a4c;                                            // +0x13a4c (repeat count, mode 0x1000)
    char pad13a50[0x1a23c - 0x13a50];
    int f1a23c;                                            // +0x1a23c                      0x4610b0
};
extern GameData* g_game;                                   // 0x5e2930

// ---- string registers and the {token} expander (object 0xa70440) ------------------------------------------
struct ScriptState {
    char pad0[0x50];
    __int64 reg[128];                                      // +0x50 registers               0x460160
    char pad450[0x2450 - 0x450];
    tstring str[128];                                      // +0x2450 string registers      0x460160
    tstring expand_token(const tstring& tok);              // 0x460160 (thiscall, ret 8: hidden return + token)
    tstring expand(const tstring& in);                     // 0x4606d0 (thiscall, ret 8)
};
extern ScriptState g_script;                               // 0xa70440

// ---- script blocks ---------------------------------------------------------------------------------------
struct Elem {                                              // 0x98 bytes: one operation of a block
    unsigned id;                                           // +0 opcode
    int pad4;
    int n;                                                 // +8 operand count
    int padc;
    __int64 vals[16];                                      // +0x10 operands
    int f90;                                               // +0x90 (-1)
    int pad94;
    Elem() : f90(-1) {}                                    // 0x4607d0
};
struct Block {                                             // 0x1c bytes
    Elem* items;                                           // +0
    int count;                                             // +4
    char done;                                             // +8
    tstring text;                                          // +0xc (debug text)
    Block() { count = 0; items = 0; done = 0; }
    ~Block() { if (items) delete[] items; }
    void read(stream_base* s);                             // 0x4607e0
    char exec(int mode);                                   // 0x4875e0 (thiscall, ret 4)
};

// ---- dialogs ---------------------------------------------------------------------------------------------
struct DialogRec {                                         // 0x64 bytes
    tstring text;                                          // +0x00 (spoken text, underscores -> spaces)
    tstring id;                                            // +0x10 (dlga_ id)
    unsigned flags;                                        // +0x20 speaker / repeat / auto flags
    int f24;                                               // +0x24 input state
    Block cond;                                            // +0x28
    Block conseq;                                          // +0x44
    int f60;                                               // +0x60 output state
    void read(stream_base* s);                             // 0x4608d0
    // implicit destructor: inlined into the vector deleting destructor 0x4609d0
};

struct DlgLine {                                           // 0x1c bytes
    int dialog;
    int idx;
    unsigned flags;
    tstring text;
};

struct DialogMgr {
    int count;                                             // +0
    DialogRec* recs;                                       // +4
    int f8;                                                // +8 current state
    int fc;                                                // +0xc partner troop
    int f10;                                               // +0x10 partner party
    int f14;                                               // +0x14
    int cur;                                               // +0x18 current dialog
    int cur_idx;                                           // +0x1c
    int nres;                                              // +0x20
    DlgLine list[0x400];                                   // +0x24
    DlgLine sel[0x400];                                    // +0x7024
    char fe024;                                            // +0xe024
    tstring cur_text;                                      // +0xe028
    int get_c8();                                          // 0x460620
    int speaker();                                         // 0x460640
    int listener();                                        // 0x460670
    tstring line_text(int i);                              // 0x4606a0 (thiscall, ret 8)
    int line_flags(int i);                                 // 0x4606c0
    void read(stream_base* s);                             // 0x460ab0
    bool available(int idx, char flag);                    // 0x460b90
    void refresh_text();                                   // 0x460c10
    void select(DlgLine* l);                               // 0x460d80
    void collect(DlgLine* out, int max, int* n, char flag);   // 0x460de0
    void advance();                                        // 0x460ff0
    void start(int a, int b, int c, int d, char f);        // 0x4610b0 (ret 0x14)
    void advance_all(int* n);                              // 0x461140
};

#endif // GAME_DIALOG_MANAGER_H
